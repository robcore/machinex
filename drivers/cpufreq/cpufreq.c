/*
 *  linux/drivers/cpufreq/cpufreq.c
 *
 *  Copyright (C) 2001 Russell King
 *            (C) 2002 - 2003 Dominik Brodowski <linux@brodo.de>
 *            (C) 2013 Viresh Kumar <viresh.kumar@linaro.org>
 *
 *  Oct 2005 - Ashok Raj <ashok.raj@intel.com>
 *	Added handling for CPU hotplug
 *  Feb 2006 - Jacob Shin <jacob.shin@amd.com>
 *	Fix handling for CPU hotplug -- affected CPUs
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/cpu.h>
#include <linux/cpufreq.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/kernel_stat.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/suspend.h>
#include <linux/syscore_ops.h>
#include <linux/tick.h>
#include <linux/prometheus.h>
#include <linux/display_state.h>

extern unsigned long acpuclk_get_rate(int cpu);
extern ssize_t get_gpu_vdd_levels_str(char *buf);
extern void set_gpu_vdd_levels(int uv_tbl[]);

static unsigned int current_screen_state = CPUFREQ_HARDLIMIT_SCREEN_ON;

struct hardlimit_policy {
	unsigned int hardlimit_max_screen_on;
	unsigned int hardlimit_max_screen_off;
	unsigned int hardlimit_min_screen_on;
	unsigned int hardlimit_min_screen_off;
	unsigned int current_limit_max;
	unsigned int current_limit_min;
	unsigned int input_boost_limit;
	unsigned int input_boost_frequency;
};

static DEFINE_PER_CPU(struct hardlimit_policy, hdata);

unsigned int limited_max_freq_thermal[NR_CPUS] = { DEFAULT_HARD_MAX, DEFAULT_HARD_MAX, DEFAULT_HARD_MAX, DEFAULT_HARD_MAX };
static struct workqueue_struct *cpu_boost_wq;
static struct delayed_work input_boost_work;
static struct delayed_work input_boost_rem;

static bool input_boost_enabled = false;
module_param(input_boost_enabled, bool, 0644);

static unsigned int input_boost_ms = 350;
module_param(input_boost_ms, uint, 0644);

static struct delayed_work input_boost_rem;
static u64 last_input_time;

static unsigned int throttle_state;
static cpumask_t throttled_mask;
bool thermal_is_throttling(void)
{
	return !cpumask_empty(&throttled_mask);
}

static RAW_NOTIFIER_HEAD(thermal_notifier_chain);

int register_thermal_notifier(struct notifier_block *nb)
{
	return raw_notifier_chain_register(&thermal_notifier_chain, nb);
}
EXPORT_SYMBOL(register_thermal_notifier);

int unregister_thermal_notifier(struct notifier_block *nb)
{
	return raw_notifier_chain_unregister(&thermal_notifier_chain, nb);
}
EXPORT_SYMBOL(unregister_thermal_notifier);

static int thermal_notifier_call_chain(unsigned long val)
{
	void *v = NULL;
	return raw_notifier_call_chain(&thermal_notifier_chain, val, v);
}
#if 0
static void thermal_notifier_update(void)
{
	bool state_changed = true;
	if (!is_display_on())
		return;

	if (thermal_is_throttling() && throttle_state == THROTTLING_OFF)
		throttle_state = THROTTLING_ON;
	else if (!thermal_is_throttling() && throttle_state == THROTTLING_ON)
		throttle_state = THROTTLING_OFF;
	else
		state_changed = false;

	if (state_changed)
		thermal_notifier_call_chain(throttle_state);
}
#endif
static LIST_HEAD(cpufreq_policy_list);

static inline bool policy_is_inactive(struct cpufreq_policy *policy)
{
	return cpumask_empty(policy->cpus);
}

/* Macros to iterate over CPU policies */
#define for_each_suitable_policy(__policy, __active)			 \
	list_for_each_entry(__policy, &cpufreq_policy_list, policy_list) \
		if ((__active) == !policy_is_inactive(__policy))

#define for_each_active_policy(__policy)		\
	for_each_suitable_policy(__policy, true)
#define for_each_inactive_policy(__policy)		\
	for_each_suitable_policy(__policy, false)

#define for_each_policy(__policy)			\
	list_for_each_entry(__policy, &cpufreq_policy_list, policy_list)

/* Iterate over governors */
static LIST_HEAD(cpufreq_governor_list);
#define for_each_governor(__governor)				\
	list_for_each_entry(__governor, &cpufreq_governor_list, governor_list)

/**
 * The "cpufreq driver" - the arch- or hardware-dependent low
 * level driver of CPUFreq support, and its spinlock. This lock
 * also protects the cpufreq_cpu_data array.
 */
static struct cpufreq_driver *cpufreq_driver;
static DEFINE_PER_CPU(struct cpufreq_policy *, cpufreq_cpu_data);
static DEFINE_RWLOCK(cpufreq_driver_lock);

/* Flag to suspend/resume CPUFreq governors */
bool cpufreq_suspended;

static inline bool has_target(void)
{
	return cpufreq_driver->target_index || cpufreq_driver->target;
}

/* internal prototypes */
static unsigned int __cpufreq_get(struct cpufreq_policy *policy);
static int cpufreq_init_governor(struct cpufreq_policy *policy);
static void cpufreq_exit_governor(struct cpufreq_policy *policy);
static int cpufreq_start_governor(struct cpufreq_policy *policy);
static void cpufreq_stop_governor(struct cpufreq_policy *policy);
static void cpufreq_governor_limits(struct cpufreq_policy *policy, bool from_start);
extern unsigned int mx_cpufreq_governor[NR_CPUS];

/**
 * Two notifier lists: the "policy" list is involved in the
 * validation process for a new CPU frequency policy; the
 * "transition" list for kernel code that needs to handle
 * changes to devices when the CPU clock speed changes.
 * The mutex locks both lists.
 */
static BLOCKING_NOTIFIER_HEAD(cpufreq_policy_notifier_list);
static struct srcu_notifier_head cpufreq_transition_notifier_list;
static bool init_cpufreq_transition_notifier_list_called;
static int __init init_cpufreq_transition_notifier_list(void)
{
	srcu_init_notifier_head(&cpufreq_transition_notifier_list);
	init_cpufreq_transition_notifier_list_called = true;
	return 0;
}
pure_initcall(init_cpufreq_transition_notifier_list);

static DEFINE_MUTEX(cpufreq_governor_mutex);

struct kobject *get_governor_parent_kobj(struct cpufreq_policy *policy)
{
		return &policy->kobj;
}
EXPORT_SYMBOL_GPL(get_governor_parent_kobj);

enum {
	ONDEMAND = 0,
	CONSERVATIVE = 1,
	INTERACTIVE = 2,
	INTELLIACTIVE = 3,
	PERFORMANCE = 4,
	POWERSAVE = 5,
	SCHEDUTIL = 6,
	USERSPACE = 7,
};

struct cpufreq_governor *get_mx_governor(unsigned int cpu)
{
	switch(mx_cpufreq_governor[cpu]) {
		case ONDEMAND:
			return mx_gov_ondemand();
		case CONSERVATIVE:
			return mx_gov_conservative();
		case INTERACTIVE:
			return mx_gov_interactive();
		case INTELLIACTIVE:
			return mx_gov_intelliactive();
		case PERFORMANCE:
			return mx_gov_performance();
		case POWERSAVE:
			return mx_gov_powersave();
		case SCHEDUTIL:
			return mx_gov_schedutil();
		case USERSPACE:
			return mx_gov_userspace();
		default:
			return cpufreq_default_governor();
	}
}

static inline u64 get_cpu_idle_time_jiffy(unsigned int cpu, u64 *wall)
{
	u64 idle_time;
	u64 cur_wall_time;
	u64 busy_time;

	cur_wall_time = jiffies64_to_nsecs(get_jiffies_64());

	busy_time = kcpustat_cpu(cpu).cpustat[CPUTIME_USER];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_SYSTEM];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_IRQ];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_SOFTIRQ];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_STEAL];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_NICE];

	idle_time = cur_wall_time - busy_time;
	if (wall)
		*wall = div_u64(cur_wall_time, NSEC_PER_USEC);

	return div_u64(idle_time, NSEC_PER_USEC);
}

u64 get_cpu_idle_time(unsigned int cpu, u64 *wall)
{
	u64 idle_time = get_cpu_idle_time_us(cpu, NULL);

	if (idle_time == -1ULL)
		return get_cpu_idle_time_jiffy(cpu, wall);
	else
		idle_time += get_cpu_iowait_time_us(cpu, wall);

	return idle_time;
}
EXPORT_SYMBOL_GPL(get_cpu_idle_time);

/*
 * This is a generic cpufreq init() routine which can be used by cpufreq
 * drivers of SMP systems. It will do following:
 * - validate & show freq table passed
 * - set policies transition latency
 * - policy->cpus with all possible CPUs
 */
int cpufreq_generic_init(struct cpufreq_policy *policy,
		struct cpufreq_frequency_table *table,
		unsigned int transition_latency)
{
	int ret;

	ret = cpufreq_table_validate_and_show(policy, table);
	if (ret) {
		pr_err("%s: invalid frequency table: %d\n", __func__, ret);
		return ret;
	}

	policy->cpuinfo.transition_latency = transition_latency;

	/*
	 * The driver only supports the SMP configuration where all processors
	 * share the clock and voltage and clock.
	 */
	cpumask_setall(policy->cpus);

	return 0;
}
EXPORT_SYMBOL_GPL(cpufreq_generic_init);

struct cpufreq_policy *cpufreq_cpu_get_raw(unsigned int cpu)
{
	struct cpufreq_policy *policy = per_cpu(cpufreq_cpu_data, cpu);

	return policy && cpumask_test_cpu(cpu, policy->cpus) ? policy : NULL;
}
EXPORT_SYMBOL_GPL(cpufreq_cpu_get_raw);

struct hardlimit_policy *hardlimit_get_raw(unsigned int cpu)
{
	struct hardlimit_policy *hpolicy = &per_cpu(hdata, cpu);

	return hpolicy ? hpolicy : NULL;
}

unsigned int cpufreq_generic_get(unsigned int cpu)
{
	if (!cpu_online(cpu))
		return 0;

	return acpuclk_get_rate(cpu);
}
EXPORT_SYMBOL_GPL(cpufreq_generic_get);

/**
 * cpufreq_cpu_get: returns policy for a cpu and marks it busy.
 *
 * @cpu: cpu to find policy for.
 *
 * This returns policy for 'cpu', returns NULL if it doesn't exist.
 * It also increments the kobject reference count to mark it busy and so would
 * require a corresponding call to cpufreq_cpu_put() to decrement it back.
 * If corresponding call cpufreq_cpu_put() isn't made, the policy wouldn't be
 * freed as that depends on the kobj count.
 *
 * Return: A valid policy on success, otherwise NULL on failure.
 */
struct cpufreq_policy *cpufreq_cpu_get(unsigned int cpu)
{
	struct cpufreq_policy *policy = NULL;
	unsigned long flags;

	if (WARN_ON(cpu >= nr_cpu_ids))
		return NULL;

	/* get the cpufreq driver */
	read_lock_irqsave(&cpufreq_driver_lock, flags);

	if (cpufreq_driver) {
		/* get the CPU */
		policy = cpufreq_cpu_get_raw(cpu);
		if (policy)
			kobject_get(&policy->kobj);
	}

	read_unlock_irqrestore(&cpufreq_driver_lock, flags);

	return policy;
}
EXPORT_SYMBOL_GPL(cpufreq_cpu_get);

/**
 * cpufreq_cpu_put: Decrements the usage count of a policy
 *
 * @policy: policy earlier returned by cpufreq_cpu_get().
 *
 * This decrements the kobject reference count incremented earlier by calling
 * cpufreq_cpu_get().
 */
void cpufreq_cpu_put(struct cpufreq_policy *policy)
{
	kobject_put(&policy->kobj);
}
EXPORT_SYMBOL_GPL(cpufreq_cpu_put);
/*********************************************************************
 *            EXTERNALLY AFFECTING FREQUENCY CHANGES                 *
 *********************************************************************/
static int cpufreq_set_policy(struct cpufreq_policy *policy,
				struct cpufreq_policy *new_policy);

struct cpufreq_frequency_table *cpufreq_frequency_get_table(void)
{
	struct cpufreq_policy *policy;
	policy = cpufreq_cpu_get_raw(0);
	return policy != NULL ? policy->freq_table : NULL;
}
EXPORT_SYMBOL_GPL(cpufreq_frequency_get_table);

/* Yank555.lu : CPU Hardlimit - Hook to force scaling_min/max_freq to be updated on Hardlimit change */
#ifdef CONFIG_CPUFREQ_HARDLIMIT
/* Disable Input boost while thermal limiting */
static bool thermal_disables_boost __read_mostly = true;
module_param(thermal_disables_boost, bool, 0644);

/* Update limits in cpufreq */
static void reapply_hard_limits(unsigned int cpu, bool update_policy)
{
	struct cpufreq_policy *policy;
	struct hardlimit_policy *hpolicy = hardlimit_get_raw(cpu);

	if (!hpolicy) {
		return;
	}
	/* Recalculate the currently applicable min/max */
	switch(current_screen_state) {
		case CPUFREQ_HARDLIMIT_SCREEN_ON:
			if (limited_max_freq_thermal[cpu] >= DEFAULT_HARD_MIN &&
				limited_max_freq_thermal[cpu] < hpolicy->hardlimit_max_screen_on) {
				hpolicy->current_limit_max = limited_max_freq_thermal[cpu];
				cpumask_test_and_set_cpu(cpu, &throttled_mask);
			} else {
				hpolicy->current_limit_max = hpolicy->hardlimit_max_screen_on;
				cpumask_test_and_clear_cpu(cpu, &throttled_mask);
			}
			if (thermal_disables_boost) {
				if (hpolicy->input_boost_limit > hpolicy->hardlimit_min_screen_on &&
					hpolicy->input_boost_limit <= hpolicy->current_limit_max &&
					limited_max_freq_thermal[cpu] == hpolicy->hardlimit_max_screen_on)
					hpolicy->current_limit_min = hpolicy->input_boost_limit;
				else
					hpolicy->current_limit_min = hpolicy->hardlimit_min_screen_on;
			} else {
				if (hpolicy->input_boost_limit > hpolicy->hardlimit_min_screen_on &&
					hpolicy->input_boost_limit <= hpolicy->current_limit_max)
					hpolicy->current_limit_min = hpolicy->input_boost_limit;
				else
					hpolicy->current_limit_min = hpolicy->hardlimit_min_screen_on;
			}
			break;
		case CPUFREQ_HARDLIMIT_SCREEN_OFF:
			if (limited_max_freq_thermal[cpu] >= DEFAULT_HARD_MIN &&
				limited_max_freq_thermal[cpu] < hpolicy->hardlimit_max_screen_off) {
				hpolicy->current_limit_max = limited_max_freq_thermal[cpu];
				cpumask_test_and_set_cpu(cpu, &throttled_mask);
			} else {
				hpolicy->current_limit_max = hpolicy->hardlimit_max_screen_off;
				cpumask_test_and_clear_cpu(cpu, &throttled_mask);
			}
			hpolicy->current_limit_min = hpolicy->hardlimit_min_screen_off;
			break;
		default:
			break;
	}
#if 0
	thermal_notifier_update();
#endif
	if (!cpu_online(cpu))
		return;

	policy = cpufreq_cpu_get_raw(cpu);
	if (!policy)
		return;

	if (!update_policy) {
		policy->user_policy.min = policy->min = hpolicy->current_limit_min;
		policy->user_policy.max = policy->max = hpolicy->current_limit_max;
	} else {
		down_write(&policy->rwsem);
		policy->user_policy.min = policy->min = hpolicy->current_limit_min;
		policy->user_policy.max = policy->max = hpolicy->current_limit_max;
		up_write(&policy->rwsem);
		cpufreq_update_policy(cpu);
	}
}

/* Sanitize cpufreq to hardlimits */
unsigned int check_cpufreq_hardlimit(unsigned int cpu, unsigned int freq)
{
	struct hardlimit_policy *hpolicy = hardlimit_get_raw(cpu);

	if (!hpolicy) {
		return freq;
	}

	sanitize_min_max(freq, hpolicy->current_limit_min, hpolicy->current_limit_max);
	return freq;
}
EXPORT_SYMBOL(check_cpufreq_hardlimit);
#endif

void set_thermal_policy(unsigned int cpu, unsigned int freq)
{
	limited_max_freq_thermal[cpu] = freq;
	reapply_hard_limits(cpu, true);
}

unsigned int get_hardlimit_max(unsigned int cpu)
{
	struct hardlimit_policy *hpolicy = hardlimit_get_raw(cpu);

	if (!hpolicy) {
		return DEFAULT_HARD_MAX;
	}
	return is_display_on() ? hpolicy->hardlimit_max_screen_on :
		    hpolicy->hardlimit_max_screen_off;
}

static void do_input_boost_rem(struct work_struct *work)
{
	unsigned int cpu;
	struct hardlimit_policy *hpolicy;

	if (!is_display_on() || !input_boost_enabled || !input_boost_ms)
		return;

	for_each_possible_cpu(cpu) {
		if (cpu_out_of_range(cpu))
			break;
		hpolicy = hardlimit_get_raw(cpu);
		if (!hpolicy) {
			continue;
		}
		/* Reset the input_boost_limit for all CPUs in the system */
		hpolicy->input_boost_limit = hpolicy->hardlimit_min_screen_on;
		reapply_hard_limits(cpu, true);
	}
}

static void do_input_boost(struct work_struct *work)
{
	unsigned int cpu;
	struct hardlimit_policy *hpolicy;

	if (!input_boost_enabled || !input_boost_ms || !is_display_on())
		return;

	/* Set the input_boost_limit for all CPUs in the system */
	for_each_possible_cpu(cpu) {
		if (cpu_out_of_range(cpu))
			break;
		hpolicy = hardlimit_get_raw(cpu);
		if (!hpolicy) {
			continue;
		}
		hpolicy->input_boost_limit = hpolicy->input_boost_frequency;
		reapply_hard_limits(cpu, true);
	}

	mod_delayed_work_on(0, cpu_boost_wq, &input_boost_rem,
					msecs_to_jiffies(input_boost_ms));
}

void cpu_boost_event(void)
{
	u64 delta;

	if (!input_boost_enabled ||
		!input_boost_ms || !is_display_on())
		return;

	if (!last_input_time)
		goto first_time;

	delta = ktime_to_us(ktime_get()) - last_input_time;
	if (delta < input_boost_ms * USEC_PER_MSEC)
		return;

first_time:
	mod_delayed_work_on(0, cpu_boost_wq, &input_boost_work, 0);
	last_input_time = ktime_to_us(ktime_get());
}
EXPORT_SYMBOL(cpu_boost_event);

/**
 * adjust_jiffies - adjust the system "loops_per_jiffy"
 *
 * This function alters the system "loops_per_jiffy" for the clock
 * speed change. Note that loops_per_jiffy cannot be updated on SMP
 * systems as each CPU might be scaled differently. So, use the arch
 * per-CPU loops_per_jiffy value wherever possible.
 */
#ifndef CONFIG_SMP
static unsigned long l_p_j_ref;
static unsigned int l_p_j_ref_freq;

static void adjust_jiffies(unsigned long val, struct cpufreq_freqs *ci)
{
	if (ci->flags & CPUFREQ_CONST_LOOPS)
		return;

	if (!l_p_j_ref_freq) {
		l_p_j_ref = loops_per_jiffy;
		l_p_j_ref_freq = ci->old;
		pr_debug("saving %lu as reference value for loops_per_jiffy; freq is %u kHz\n",
			 l_p_j_ref, l_p_j_ref_freq);
	}
	if (val == CPUFREQ_POSTCHANGE && ci->old != ci->new) {
		loops_per_jiffy = cpufreq_scale(l_p_j_ref, l_p_j_ref_freq,
								ci->new);
		pr_debug("scaling loops_per_jiffy to %lu for frequency %u kHz\n",
			 loops_per_jiffy, ci->new);
	}
}
#else
static inline void adjust_jiffies(unsigned long val, struct cpufreq_freqs *ci)
{
	return;
}
#endif

static void __cpufreq_notify_transition(struct cpufreq_policy *policy,
		struct cpufreq_freqs *freqs, unsigned int state)
{
	BUG_ON(irqs_disabled());

	freqs->flags = cpufreq_driver->flags;
	pr_debug("notification %u of frequency transition to %u kHz\n",
		 state, freqs->new);

	switch (state) {

	case CPUFREQ_PRECHANGE:
		/* detect if the driver reported a value as "old frequency"
		 * which is not equal to what the cpufreq core thinks is
		 * "old frequency".
		 */
		if (!(cpufreq_driver->flags & CPUFREQ_CONST_LOOPS)) {
			if ((policy) && (policy->cpu == freqs->cpu) &&
			    (policy->cur) && (policy->cur != freqs->old)) {
				freqs->old = policy->cur;
			}
		}
		srcu_notifier_call_chain(&cpufreq_transition_notifier_list,
				CPUFREQ_PRECHANGE, freqs);
		adjust_jiffies(CPUFREQ_PRECHANGE, freqs);
		break;

	case CPUFREQ_POSTCHANGE:
		adjust_jiffies(CPUFREQ_POSTCHANGE, freqs);
		cpufreq_stats_record_transition(policy, freqs->new);
		srcu_notifier_call_chain(&cpufreq_transition_notifier_list,
				CPUFREQ_POSTCHANGE, freqs);
		if (likely(policy) && likely(policy->cpu == freqs->cpu))
			policy->cur = freqs->new;
		break;
	}
}

/**
 * cpufreq_notify_transition - call notifier chain and adjust_jiffies
 * on frequency transition.
 *
 * This function calls the transition notifiers and the "adjust_jiffies"
 * function. It is called twice on all CPU frequency changes that have
 * external effects.
 */
static void cpufreq_notify_transition(struct cpufreq_policy *policy,
		struct cpufreq_freqs *freqs, unsigned int state)
{
	for_each_cpu(freqs->cpu, policy->cpus)
		__cpufreq_notify_transition(policy, freqs, state);
}

/* Do post notifications when there are chances that transition has failed */
static void cpufreq_notify_post_transition(struct cpufreq_policy *policy,
		struct cpufreq_freqs *freqs, int transition_failed)
{
	cpufreq_notify_transition(policy, freqs, CPUFREQ_POSTCHANGE);
	if (!transition_failed)
		return;

	swap(freqs->old, freqs->new);
	cpufreq_notify_transition(policy, freqs, CPUFREQ_PRECHANGE);
	cpufreq_notify_transition(policy, freqs, CPUFREQ_POSTCHANGE);
}

void cpufreq_freq_transition_begin(struct cpufreq_policy *policy,
		struct cpufreq_freqs *freqs)
{

	/*
	 * Catch double invocations of _begin() which lead to self-deadlock.
	 * ASYNC_NOTIFICATION drivers are left out because the cpufreq core
	 * doesn't invoke _begin() on their behalf, and hence the chances of
	 * double invocations are very low. Moreover, there are scenarios
	 * where these checks can emit false-positive warnings in these
	 * drivers; so we avoid that by skipping them altogether.
	 */
	WARN_ON(!(cpufreq_driver->flags & CPUFREQ_ASYNC_NOTIFICATION)
				&& current == policy->transition_task);

wait:
	wait_event(policy->transition_wait, !policy->transition_ongoing);

	spin_lock(&policy->transition_lock);

	if (unlikely(policy->transition_ongoing)) {
		spin_unlock(&policy->transition_lock);
		goto wait;
	}

	policy->transition_ongoing = true;
	policy->transition_task = current;

	spin_unlock(&policy->transition_lock);

	cpufreq_notify_transition(policy, freqs, CPUFREQ_PRECHANGE);
}
EXPORT_SYMBOL_GPL(cpufreq_freq_transition_begin);

void cpufreq_freq_transition_end(struct cpufreq_policy *policy,
		struct cpufreq_freqs *freqs, int transition_failed)
{
	if (unlikely(WARN_ON(!policy->transition_ongoing)))
		return;

	cpufreq_notify_post_transition(policy, freqs, transition_failed);

	policy->transition_ongoing = false;
	policy->transition_task = NULL;

	wake_up(&policy->transition_wait);
}
EXPORT_SYMBOL_GPL(cpufreq_freq_transition_end);

/*
 * Fast frequency switching status count.  Positive means "enabled", negative
 * means "disabled" and 0 means "not decided yet".
 */
static int cpufreq_fast_switch_count;
static DEFINE_MUTEX(cpufreq_fast_switch_lock);

static void cpufreq_list_transition_notifiers(void)
{
	struct notifier_block *nb;

	pr_info("Registered transition notifiers:\n");

	mutex_lock(&cpufreq_transition_notifier_list.mutex);

	for (nb = cpufreq_transition_notifier_list.head; nb; nb = nb->next)
		pr_info("%pF\n", nb->notifier_call);

	mutex_unlock(&cpufreq_transition_notifier_list.mutex);
}

/**
 * cpufreq_enable_fast_switch - Enable fast frequency switching for policy.
 * @policy: cpufreq policy to enable fast frequency switching for.
 *
 * Try to enable fast frequency switching for @policy.
 *
 * The attempt will fail if there is at least one transition notifier registered
 * at this point, as fast frequency switching is quite fundamentally at odds
 * with transition notifiers.  Thus if successful, it will make registration of
 * transition notifiers fail going forward.
 */
void cpufreq_enable_fast_switch(struct cpufreq_policy *policy)
{
	lockdep_assert_held(&policy->rwsem);

	if (!policy->fast_switch_possible)
		return;

	mutex_lock(&cpufreq_fast_switch_lock);
	if (cpufreq_fast_switch_count >= 0) {
		cpufreq_fast_switch_count++;
		policy->fast_switch_enabled = true;
	} else {
		pr_warn("CPU%u: Fast frequency switching not enabled\n",
			policy->cpu);
		cpufreq_list_transition_notifiers();
	}
	mutex_unlock(&cpufreq_fast_switch_lock);
}
EXPORT_SYMBOL_GPL(cpufreq_enable_fast_switch);

/**
 * cpufreq_disable_fast_switch - Disable fast frequency switching for policy.
 * @policy: cpufreq policy to disable fast frequency switching for.
 */
void cpufreq_disable_fast_switch(struct cpufreq_policy *policy)
{
	mutex_lock(&cpufreq_fast_switch_lock);
	if (policy->fast_switch_enabled) {
		policy->fast_switch_enabled = false;
		if (!WARN_ON(cpufreq_fast_switch_count <= 0))
			cpufreq_fast_switch_count--;
	}
	mutex_unlock(&cpufreq_fast_switch_lock);
}
EXPORT_SYMBOL_GPL(cpufreq_disable_fast_switch);

/**
 * cpufreq_driver_resolve_freq - Map a target frequency to a driver-supported
 * one.
 * @target_freq: target frequency to resolve.
 *
 * The target to driver frequency mapping is cached in the policy.
 *
 * Return: Lowest driver-supported frequency greater than or equal to the
 * given target_freq, subject to policy (min/max) and driver limitations.
 */
unsigned int cpufreq_driver_resolve_freq(struct cpufreq_policy *policy,
					 unsigned int target_freq)
{
	target_freq = clamp_val(target_freq, check_cpufreq_hardlimit(policy->cpu, policy->min), check_cpufreq_hardlimit(policy->cpu, policy->max));
	policy->cached_target_freq = target_freq;

	if (cpufreq_driver->target_index) {
		int idx;

		idx = cpufreq_frequency_table_target(policy, target_freq,
						     CPUFREQ_RELATION_L);
		policy->cached_resolved_idx = idx;
		return policy->freq_table[idx].frequency;
	}

	if (cpufreq_driver->resolve_freq)
		return cpufreq_driver->resolve_freq(policy, target_freq);

	return target_freq;
}
EXPORT_SYMBOL_GPL(cpufreq_driver_resolve_freq);

unsigned int cpufreq_policy_transition_delay_us(struct cpufreq_policy *policy)
{
	unsigned int latency;

	if (policy->transition_delay_us)
		return policy->transition_delay_us;

	latency = policy->cpuinfo.transition_latency / NSEC_PER_USEC;
	if (latency) {
		/*
		 * For platforms that can change the frequency very fast (< 10
		 * us), the above formula gives a decent transition delay. But
		 * for platforms where transition_latency is in milliseconds, it
		 * ends up giving unrealistic values.
		 *
		 * Cap the default transition delay to 10 ms, which seems to be
		 * a reasonable amount of time after which we should reevaluate
		 * the frequency.
		 */
		return min(latency * LATENCY_MULTIPLIER, (unsigned int)10000);
	}

	return LATENCY_MULTIPLIER;
}
EXPORT_SYMBOL_GPL(cpufreq_policy_transition_delay_us);

/*********************************************************************
 *                          SYSFS INTERFACE                          *
 *********************************************************************/
static ssize_t show_boost(struct kobject *kobj,
				 struct attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", cpufreq_driver->boost_enabled);
}

static ssize_t store_boost(struct kobject *kobj, struct attribute *attr,
				  const char *buf, size_t count)
{
	int ret, enable;

	ret = sscanf(buf, "%d", &enable);
	if (ret != 1 || enable < 0 || enable > 1)
		return -EINVAL;

	if (cpufreq_boost_trigger_state(enable))
		return -EINVAL;

	return count;
}
define_one_global_rw(boost);

static struct cpufreq_governor *find_governor(const char *str_governor)
{
	struct cpufreq_governor *t;

	for_each_governor(t)
		if (!strncasecmp(str_governor, t->name, CPUFREQ_NAME_LEN))
			return t;

	return NULL;
}

/**
 * cpufreq_parse_governor - parse a governor string
 */
static int cpufreq_parse_governor(char *str_governor,
				struct cpufreq_governor **governor)
{
	int err = -EINVAL;

	struct cpufreq_governor *t;

	mutex_lock(&cpufreq_governor_mutex);

	t = find_governor(str_governor);

	if (t == NULL) {
		int ret;

		mutex_unlock(&cpufreq_governor_mutex);
		ret = request_module("cpufreq_%s", str_governor);
		mutex_lock(&cpufreq_governor_mutex);

		if (ret == 0)
			t = find_governor(str_governor);
	}

	if (t != NULL) {
		*governor = t;
		err = 0;
	}

	mutex_unlock(&cpufreq_governor_mutex);

	return err;
}

void cpufreq_verify_within_limits(struct cpufreq_policy *policy,
		unsigned int min, unsigned int max)
{
	min = check_cpufreq_hardlimit(policy->cpu, min);
	max = check_cpufreq_hardlimit(policy->cpu, max);
	if (policy->min < min)
		policy->min = min;
	if (policy->max < min)
		policy->max = min;
	if (policy->min > max)
		policy->min = max;
	if (policy->max > max)
		policy->max = max;
	if (policy->min > policy->max)
		policy->min = policy->max;
	return;
}
EXPORT_SYMBOL(cpufreq_verify_within_limits);

void
cpufreq_verify_within_cpu_limits(struct cpufreq_policy *policy)
{
	cpufreq_verify_within_limits(policy, policy->cpuinfo.min_freq,
			policy->cpuinfo.max_freq);
}
EXPORT_SYMBOL(cpufreq_verify_within_cpu_limits);

#ifdef CONFIG_CPUFREQ_HARDLIMIT

/* ------------------------------------------------------------------------------ */
/* Powersuspend callback functions                                                */
/* ------------------------------------------------------------------------------ */

void cpufreq_hardlimit_suspend(void)
{
	unsigned int cpu;
	current_screen_state = CPUFREQ_HARDLIMIT_SCREEN_OFF;
	for_each_possible_cpu(cpu)
		reapply_hard_limits(cpu, true);
}

void cpufreq_hardlimit_resume(void)
{
	unsigned int cpu;
	current_screen_state = CPUFREQ_HARDLIMIT_SCREEN_ON;
	for_each_possible_cpu(cpu)
		reapply_hard_limits(cpu, true);
}
#endif /*CONFIG_CPUFREQ_HARDLIMIT*/
/**
 * cpufreq_per_cpu_attr_read() / show_##file_name() -
 * print out cpufreq information
 *
 * Write out information from cpufreq_driver->policy[cpu]; object must be
 * "unsigned int".
 */

#define show_one(file_name, object)			\
static ssize_t show_##file_name				\
(struct cpufreq_policy *policy, char *buf)		\
{							\
	return sprintf(buf, "%u\n", policy->object);	\
}

show_one(cpuinfo_min_freq, cpuinfo.min_freq);
show_one(cpuinfo_max_freq, cpuinfo.max_freq);
show_one(cpuinfo_transition_latency, cpuinfo.transition_latency);
show_one(scaling_min_freq, min);
show_one(scaling_max_freq, max);
#ifdef CONFIG_CPUFREQ_HARDLIMIT

static ssize_t hard_cpu_online_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	
	return sprintf(buf, "%u\n", cpu_online(dev->id) ? 1 : 0);
}

#define show_one_hardlimit(object)				\
static ssize_t object##_show(struct device *dev,			\
		struct device_attribute *attr, char *buf)	\
{								\
	struct hardlimit_policy *hpolicy = hardlimit_get_raw(dev->id);	\
	if (!hpolicy)	\
		return sprintf(buf, "ERROR\n");	\
	return sprintf(buf, "%u\n", hpolicy->object);	\
}

show_one_hardlimit(hardlimit_max_screen_on);
show_one_hardlimit(hardlimit_max_screen_off);
show_one_hardlimit(hardlimit_min_screen_on);
show_one_hardlimit(hardlimit_min_screen_off);
show_one_hardlimit(current_limit_min);
show_one_hardlimit(current_limit_max);
show_one_hardlimit(input_boost_frequency);
#endif

/*WARNING! HACK!*/
static ssize_t show_scaling_cur_freq(struct cpufreq_policy *policy, char *buf)
{
	ssize_t ret;

	if (cpu_online(policy->cpu))
		ret = sprintf(buf, "%u\n", policy->cur);
	else
		return -ENOSYS;
	return ret;
}

/**
 * cpufreq_per_cpu_attr_write() / store_##file_name() - sysfs write access
 */
#define store_one(file_name, object)			\
static ssize_t store_##file_name					\
(struct cpufreq_policy *policy, const char *buf, size_t count)		\
{									\
	int ret, temp;							\
	struct cpufreq_policy new_policy;				\
									\
	memcpy(&new_policy, policy, sizeof(*policy));			\
									\
	ret = sscanf(buf, "%u", &new_policy.object);			\
	if (ret != 1)							\
		return -EINVAL;						\
									\
	temp = new_policy.object;					\
	ret = cpufreq_set_policy(policy, &new_policy);		\
	if (!ret)							\
		policy->user_policy.object = temp;			\
									\
	return ret ? ret : count;					\
}

#ifdef CONFIG_CPUFREQ_HARDLIMIT

#define store_one_hardlimit(name)		\
static ssize_t name##_store		\
(struct device *dev,	\
struct device_attribute *attr,	\
const char *buf, size_t count)			\
{	\
	unsigned int new_hardlimit, i;	\
	struct cpufreq_frequency_table *permtable;	\
	struct hardlimit_policy *hpolicy = hardlimit_get_raw(dev->id);	\
	if (!hpolicy)	\
		return -EINVAL;	\
	if (!sscanf(buf, "%u", &new_hardlimit))	\
		return -EINVAL;	\
	permtable = cpufreq_frequency_get_table();	\
	if (permtable == NULL)	\
		return -EINVAL;	\
	for (i = 0; (permtable[i].frequency != CPUFREQ_TABLE_END); i++)	\
		if (permtable[i].frequency == new_hardlimit) {	\
				hpolicy->name = new_hardlimit;	\
				reapply_hard_limits(dev->id, true);	\
				return count;	\
		}	\
	return -EINVAL;	\
}

#define store_one_hardlimit_paneloff(name)		\
static ssize_t name##_store		\
(struct device *dev,	\
struct device_attribute *attr,	\
const char *buf, size_t count)			\
{	\
	unsigned int new_hardlimit, i;	\
	struct cpufreq_frequency_table *permtable;	\
	struct hardlimit_policy *hpolicy = hardlimit_get_raw(dev->id);	\
	if (!hpolicy)	\
		return -EINVAL;	\
	if (!sscanf(buf, "%u", &new_hardlimit))	\
		return -EINVAL;	\
	permtable = cpufreq_frequency_get_table();	\
	if (permtable == NULL)	\
		return -EINVAL;	\
	for (i = 0; (permtable[i].frequency != CPUFREQ_TABLE_END); i++)	\
		if (permtable[i].frequency == new_hardlimit) {	\
				hpolicy->name = new_hardlimit;	\
				reapply_hard_limits(dev->id, true);	\
				return count;	\
		}	\
	return -EINVAL;	\
}

store_one_hardlimit(hardlimit_max_screen_on);
store_one_hardlimit_paneloff(hardlimit_max_screen_off);
store_one_hardlimit(hardlimit_min_screen_on);
store_one_hardlimit_paneloff(hardlimit_min_screen_off);
store_one_hardlimit(input_boost_frequency);
#endif /*CONFIG_CPUFREQ_HARDLIMIT*/
/**
 * show_cpuinfo_cur_freq - current CPU frequency as detected by hardware
 */
static ssize_t show_cpuinfo_cur_freq(struct cpufreq_policy *policy,
					char *buf)
{
	unsigned int cur_freq = __cpufreq_get(policy);

	if (cur_freq)
		return sprintf(buf, "%u\n", cur_freq);

	return sprintf(buf, "<unknown>\n");
}

/**
 * show_scaling_governor - show the current policy for the specified CPU
 */
static ssize_t show_scaling_governor(struct cpufreq_policy *policy, char *buf)
{
	if (policy->governor) {
		if (cpu_online(policy->cpu))
			return scnprintf(buf, CPUFREQ_NAME_PLEN, "%s\n",
					policy->governor->name);
		else
			return scnprintf(buf, CPUFREQ_NAME_PLEN, "%s\n",
					policy->last_governor);
	}
	
	return -EINVAL;
}

/**
 * show_scaling_driver - show the cpufreq driver currently loaded
 */
static ssize_t show_scaling_driver(struct cpufreq_policy *policy, char *buf)
{
	return scnprintf(buf, CPUFREQ_NAME_PLEN, "%s\n", cpufreq_driver->name);
}

/**
 * show_scaling_available_governors - show the available CPUfreq governors
 */
static ssize_t show_scaling_available_governors(struct cpufreq_policy *policy,
						char *buf)
{
	ssize_t i = 0;
	struct cpufreq_governor *t;

	if (!has_target()) {
		i += sprintf(buf, "performance powersave");
		goto out;
	}

	for_each_governor(t) {
		if (i >= (ssize_t) ((PAGE_SIZE / sizeof(char))
		    - (CPUFREQ_NAME_LEN + 2)))
			goto out;
		i += scnprintf(&buf[i], CPUFREQ_NAME_PLEN, "%s ", t->name);
	}
out:
	i += sprintf(&buf[i], "\n");
	return i;
}

ssize_t cpufreq_show_cpus(const struct cpumask *mask, char *buf)
{
	ssize_t i = 0;
	unsigned int cpu;

	for_each_cpu(cpu, mask) {
		if (i)
			i += scnprintf(&buf[i], (PAGE_SIZE - i - 2), " ");
		i += scnprintf(&buf[i], (PAGE_SIZE - i - 2), "%u", cpu);
		if (i >= (PAGE_SIZE - 5))
			break;
	}
	i += sprintf(&buf[i], "\n");
	return i;
}
EXPORT_SYMBOL_GPL(cpufreq_show_cpus);

/**
 * show_related_cpus - show the CPUs affected by each transition even if
 * hw coordination is in use
 */
static ssize_t show_related_cpus(struct cpufreq_policy *policy, char *buf)
{
	return cpufreq_show_cpus(policy->related_cpus, buf);
}

/**
 * show_affected_cpus - show the CPUs affected by each transition
 */
static ssize_t show_affected_cpus(struct cpufreq_policy *policy, char *buf)
{
	return cpufreq_show_cpus(policy->cpus, buf);
}

static ssize_t store_scaling_setspeed(struct cpufreq_policy *policy,
					const char *buf, size_t count)
{
	unsigned int freq = 0;
	unsigned int ret;

	if (!policy->governor || !policy->governor->store_setspeed)
		return -EINVAL;

	ret = sscanf(buf, "%u", &freq);
	if (ret != 1)
		return -EINVAL;

	policy->governor->store_setspeed(policy, freq);

	return count;
}

static ssize_t show_scaling_setspeed(struct cpufreq_policy *policy, char *buf)
{
	if (!policy->governor || !policy->governor->show_setspeed)
		return sprintf(buf, "<unsupported>\n");

	return policy->governor->show_setspeed(policy, buf);
}

/**
 * show_bios_limit - show the current cpufreq HW/BIOS limitation
 */
static ssize_t show_bios_limit(struct cpufreq_policy *policy, char *buf)
{
	return sprintf(buf, "%u\n", policy->cpuinfo.max_freq);
}

#ifdef CONFIG_CPU_VOLTAGE_TABLE
extern ssize_t acpuclk_get_vdd_levels_str(char *buf);
extern void acpuclk_set_vdd(unsigned acpu_khz, int vdd);

static ssize_t show_vdd_levels(struct kobject *a, struct attribute *b, char *buf)
{
	return acpuclk_get_vdd_levels_str(buf);
}

static ssize_t store_vdd_levels(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{

	int i = 0, j;
	int pair[2] = { 0, 0 };
	int sign = 0;

	if (count < 1)
		return 0;

	if (buf[0] == '-') {
		sign = -1;
		i++;
	} else if (buf[0] == '+') {
		sign = 1;
		i++;
	}

	for (j = 0; i < count; i++) {

		char c = buf[i];

		if ((c >= '0') && (c <= '9')) {
			pair[j] *= 10;
			pair[j] += (c - '0');
		}
		else if ((c == ' ') || (c == '\t')) {
			if (pair[j] != 0) {
				j++;

				if ((sign != 0) || (j > 1))
					break;
			}
		}
		else
			break;
	}

	if (sign != 0) {
		if (pair[0] > 0)
			acpuclk_set_vdd(0, sign * pair[0]);
	} else if ((pair[0] > 0) && (pair[1] > 0))
			acpuclk_set_vdd((unsigned)pair[0], pair[1]);
	else
			return -EINVAL;

	return count;
}

#endif	/* CONFIG_CPU_VOLTAGE_TABLE */

ssize_t show_GPU_mV_table(struct kobject *a, struct attribute *b, char *buf)
{
	return get_gpu_vdd_levels_str(buf);
}

ssize_t store_GPU_mV_table(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
	unsigned int ret = -EINVAL;
	unsigned int u[3];
	ret = sscanf(buf, "%d %d %d", &u[0], &u[1], &u[2]);
	set_gpu_vdd_levels(u);
	return count;
}

cpufreq_freq_attr_ro_perm(cpuinfo_cur_freq, 0400);
cpufreq_freq_attr_ro(cpuinfo_min_freq);
cpufreq_freq_attr_ro(cpuinfo_max_freq);
cpufreq_freq_attr_ro(cpuinfo_transition_latency);
cpufreq_freq_attr_ro(scaling_available_governors);
cpufreq_freq_attr_ro(scaling_driver);
cpufreq_freq_attr_ro(scaling_cur_freq);
cpufreq_freq_attr_ro(bios_limit);
cpufreq_freq_attr_ro(related_cpus);
cpufreq_freq_attr_ro(affected_cpus);
cpufreq_freq_attr_ro(scaling_min_freq);
cpufreq_freq_attr_ro(scaling_max_freq);
cpufreq_freq_attr_ro(scaling_governor);
cpufreq_freq_attr_rw(scaling_setspeed);
#ifdef CONFIG_CPU_VOLTAGE_TABLE
define_one_global_rw(vdd_levels);
#endif
define_one_global_rw(GPU_mV_table);
#ifdef CONFIG_CPUFREQ_HARDLIMIT
DEVICE_ATTR_RO(hard_cpu_online);
DEVICE_ATTR_RW(hardlimit_max_screen_on);
DEVICE_ATTR_RW(hardlimit_max_screen_off);
DEVICE_ATTR_RW(hardlimit_min_screen_on);
DEVICE_ATTR_RW(hardlimit_min_screen_off);
DEVICE_ATTR_RO(current_limit_min);
DEVICE_ATTR_RO(current_limit_max);
DEVICE_ATTR_RW(input_boost_frequency);
#endif

static struct attribute *default_attrs[] = {
	&cpuinfo_min_freq.attr,
	&cpuinfo_max_freq.attr,
	&cpuinfo_transition_latency.attr,
	&scaling_min_freq.attr,
	&scaling_max_freq.attr,
	&affected_cpus.attr,
	&related_cpus.attr,
	&scaling_governor.attr,
	&scaling_driver.attr,
	&scaling_available_governors.attr,
	&scaling_setspeed.attr,
	NULL
};

static struct attribute *hardlimit_attrs[] = {
	&dev_attr_hard_cpu_online.attr,
	&dev_attr_hardlimit_max_screen_on.attr,
	&dev_attr_hardlimit_max_screen_off.attr,
	&dev_attr_hardlimit_min_screen_on.attr,
	&dev_attr_hardlimit_min_screen_off.attr,
	&dev_attr_current_limit_min.attr,
	&dev_attr_current_limit_max.attr,
	&dev_attr_input_boost_frequency.attr,
	NULL
};

static struct attribute_group hardlimit_attr_group = {
	.attrs = hardlimit_attrs,
	.name = "hardlimit",
};

#ifdef CONFIG_CPU_VOLTAGE_TABLE
static struct attribute *vddtbl_attrs[] = {
	&vdd_levels.attr,
	&GPU_mV_table.attr,
	NULL
};

static struct attribute_group vddtbl_attr_group = {
	.attrs = vddtbl_attrs,
	.name = "vdd_table",
};
#endif	/* CONFIG_CPU_VOLTAGE_TABLE */

#define to_policy(k) container_of(k, struct cpufreq_policy, kobj)
#define to_attr(a) container_of(a, struct freq_attr, attr)

static ssize_t show(struct kobject *kobj, struct attribute *attr, char *buf)
{
	struct cpufreq_policy *policy = to_policy(kobj);
	struct freq_attr *fattr = to_attr(attr);
	ssize_t ret;

	down_read(&policy->rwsem);
	ret = fattr->show(policy, buf);
	up_read(&policy->rwsem);

	return ret;
}

static ssize_t store(struct kobject *kobj, struct attribute *attr,
		     const char *buf, size_t count)
{
	struct cpufreq_policy *policy = to_policy(kobj);
	struct freq_attr *fattr = to_attr(attr);
	ssize_t ret = -EINVAL;

	cpus_read_lock();

	if (cpu_online(policy->cpu)) {
		down_write(&policy->rwsem);
		ret = fattr->store(policy, buf, count);
		up_write(&policy->rwsem);
	}

	cpus_read_unlock();

	return ret;
}

static void cpufreq_sysfs_release(struct kobject *kobj)
{
	struct cpufreq_policy *policy = to_policy(kobj);
	pr_debug("last reference is dropped\n");
	complete(&policy->kobj_unregister);
}

static const struct sysfs_ops sysfs_ops = {
	.show	= show,
	.store	= store,
};

static struct kobj_type ktype_cpufreq = {
	.sysfs_ops	= &sysfs_ops,
	.default_attrs	= default_attrs,
	.release	= cpufreq_sysfs_release,
};

static void add_cpu_dev_symlink(struct cpufreq_policy *policy, unsigned int cpu)
{
	struct device *dev = get_cpu_device(cpu);

	if (!dev)
		return;

	if (cpumask_test_and_set_cpu(cpu, policy->real_cpus))
		return;

	dev_dbg(dev, "%s: Adding symlink\n", __func__);
	if (sysfs_create_link(&dev->kobj, &policy->kobj, "cpufreq"))
		dev_err(dev, "cpufreq symlink creation failed\n");
}

static void remove_cpu_dev_symlink(struct cpufreq_policy *policy,
				   struct device *dev)
{
	dev_dbg(dev, "%s: Removing symlink\n", __func__);
	sysfs_remove_link(&dev->kobj, "cpufreq");
}

static int cpufreq_add_dev_interface(struct cpufreq_policy *policy)
{
	struct freq_attr **drv_attr;
	int ret = 0;

	/* set up files for this cpu device */
	drv_attr = cpufreq_driver->attr;
	while (drv_attr && *drv_attr) {
		ret = sysfs_create_file(&policy->kobj, &((*drv_attr)->attr));
		if (ret)
			return ret;
		drv_attr++;
	}
	if (cpufreq_driver->get) {
		ret = sysfs_create_file(&policy->kobj, &cpuinfo_cur_freq.attr);
		if (ret)
			return ret;
	}

	ret = sysfs_create_file(&policy->kobj, &scaling_cur_freq.attr);
	if (ret)
		return ret;

	if (cpufreq_driver->bios_limit) {
		ret = sysfs_create_file(&policy->kobj, &bios_limit.attr);
		if (ret)
			return ret;
	}

	return 0;
}

__weak struct cpufreq_governor *cpufreq_default_governor(void)
{
	return NULL;
}

int mx_update_policy(unsigned int cpu)
{
	struct cpufreq_policy *policy;
	struct cpufreq_policy new_policy;
	unsigned int ret = -ENOMEM;

	policy = cpufreq_cpu_get(cpu);
	if (!policy)
		return ret;

	cpus_read_lock();
	if (cpu_online(policy->cpu)) {
		down_write(&policy->rwsem);
		memcpy(&new_policy, policy, sizeof(*policy));
		new_policy.governor = get_mx_governor(policy->cpu);
		ret = cpufreq_set_policy(policy, &new_policy);
		up_write(&policy->rwsem);
	}
	cpus_read_unlock();
	cpufreq_cpu_put(policy);
	return ret;
}

static int cpufreq_init_policy(struct cpufreq_policy *policy)
{
	struct cpufreq_policy new_policy;

	memcpy(&new_policy, policy, sizeof(*policy));
	new_policy.governor = get_mx_governor(policy->cpu);
	/* set default policy */
	return cpufreq_set_policy(policy, &new_policy);
}

static int cpufreq_add_policy_cpu(struct cpufreq_policy *policy, unsigned int cpu)
{
	int ret = 0;

	/* Has this CPU been taken care of already? */
	if (cpumask_test_cpu(cpu, policy->cpus))
		return 0;

	down_write(&policy->rwsem);
	if (has_target())
		cpufreq_stop_governor(policy);

	cpumask_set_cpu(cpu, policy->cpus);

	if (has_target()) {
		ret = cpufreq_start_governor(policy);
		if (ret)
			pr_err("%s: Failed to start governor\n", __func__);
	}
	up_write(&policy->rwsem);
	return ret;
}

static void handle_update(struct work_struct *work)
{
	struct cpufreq_policy *policy =
		container_of(work, struct cpufreq_policy, update);
	unsigned int cpu = policy->cpu;
	pr_debug("handle_update for cpu %u called\n", cpu);
	cpufreq_update_policy(cpu);
}

static struct cpufreq_policy *cpufreq_policy_alloc(unsigned int cpu)
{
	struct cpufreq_policy *policy;
	int ret;

	policy = kzalloc(sizeof(*policy), GFP_KERNEL);
	if (!policy)
		return NULL;

	if (!alloc_cpumask_var(&policy->cpus, GFP_KERNEL))
		goto err_free_policy;

	if (!zalloc_cpumask_var(&policy->related_cpus, GFP_KERNEL))
		goto err_free_cpumask;

	if (!zalloc_cpumask_var(&policy->real_cpus, GFP_KERNEL))
		goto err_free_rcpumask;

	ret = kobject_init_and_add(&policy->kobj, &ktype_cpufreq,
				   cpufreq_global_kobject, "policy%u", cpu);
	if (ret) {
		pr_err("%s: failed to init policy->kobj: %d\n", __func__, ret);
		goto err_free_real_cpus;
	}

	INIT_LIST_HEAD(&policy->policy_list);
	init_rwsem(&policy->rwsem);
	spin_lock_init(&policy->transition_lock);
	init_waitqueue_head(&policy->transition_wait);
	init_completion(&policy->kobj_unregister);
	INIT_WORK(&policy->update, handle_update);

	policy->cpu = cpu;
	return policy;

err_free_real_cpus:
	free_cpumask_var(policy->real_cpus);
err_free_rcpumask:
	free_cpumask_var(policy->related_cpus);
err_free_cpumask:
	free_cpumask_var(policy->cpus);
err_free_policy:
	kfree(policy);

	return NULL;
}

static void cpufreq_policy_put_kobj(struct cpufreq_policy *policy)
{
	struct kobject *kobj;
	struct completion *cmp;

	down_write(&policy->rwsem);
	cpufreq_stats_free_table(policy);
	kobj = &policy->kobj;
	cmp = &policy->kobj_unregister;
	up_write(&policy->rwsem);
	kobject_put(kobj);

	/*
	 * We need to make sure that the underlying kobj is
	 * actually not referenced anymore by anybody before we
	 * proceed with unloading.
	 */
	pr_debug("waiting for dropping of refcount\n");
	wait_for_completion(cmp);
	pr_debug("wait complete\n");
}

static void cpufreq_policy_free(struct cpufreq_policy *policy)
{
	unsigned long flags;
	int cpu;

	/* Remove policy from list */
	write_lock_irqsave(&cpufreq_driver_lock, flags);
	list_del(&policy->policy_list);

	for_each_cpu(cpu, policy->related_cpus)
		per_cpu(cpufreq_cpu_data, cpu) = NULL;
	write_unlock_irqrestore(&cpufreq_driver_lock, flags);

	cpufreq_policy_put_kobj(policy);
	free_cpumask_var(policy->real_cpus);
	free_cpumask_var(policy->related_cpus);
	free_cpumask_var(policy->cpus);
	kfree(policy);
}

static unsigned int hardlimit_ready[NR_CPUS] = {0, 0, 0, 0};
static void hardlimit_policy_alloc(unsigned int cpu)
{
	struct hardlimit_policy *hpolicy = hardlimit_get_raw(cpu);
	BUG_ON(!hpolicy);
	hpolicy->hardlimit_max_screen_on = DEFAULT_HARD_MAX;
	hpolicy->hardlimit_max_screen_off = DEFAULT_HARD_MAX;
	hpolicy->hardlimit_min_screen_on = DEFAULT_HARD_MIN;
	hpolicy->hardlimit_min_screen_off = DEFAULT_HARD_MIN;
	hpolicy->current_limit_max = DEFAULT_HARD_MAX;
	hpolicy->current_limit_min = DEFAULT_HARD_MIN;
	hpolicy->input_boost_limit = DEFAULT_HARD_MIN;
	hpolicy->input_boost_frequency = DEFAULT_INPUT_FREQ;
}

static int hardlimit_attr_init(unsigned int cpu)
{
	struct device *dev = get_cpu_device(cpu);
	WARN_ON_ONCE(sysfs_create_group(&dev->kobj, &hardlimit_attr_group));
	return 0;
}

static int cpufreq_online(unsigned int cpu)
{
	struct cpufreq_policy *policy;
	bool new_policy;
	unsigned long flags;
	unsigned int j;
	int ret;
	struct hardlimit_policy *hpolicy = &per_cpu(hdata, cpu);

	BUG_ON(!hpolicy);
	pr_debug("%s: bringing CPU%u online\n", __func__, cpu);

	/* Check if this CPU already has a policy to manage it */
	policy = per_cpu(cpufreq_cpu_data, cpu);
	if (policy) {
		WARN_ON(!cpumask_test_cpu(cpu, policy->related_cpus));
		if (!policy_is_inactive(policy))
			return cpufreq_add_policy_cpu(policy, cpu);

		/* This is the only online CPU for the policy.  Start over. */
		new_policy = false;
		down_write(&policy->rwsem);
		policy->cpu = cpu;
		policy->governor = NULL;
		up_write(&policy->rwsem);
	} else {
		new_policy = true;
		policy = cpufreq_policy_alloc(cpu);
		if (!policy)
			return -ENOMEM;
	}

	cpumask_copy(policy->cpus, cpumask_of(cpu));

	if (!hardlimit_ready[policy->cpu]) {
		hardlimit_policy_alloc(policy->cpu);
		BUG_ON(hardlimit_attr_init(policy->cpu));
		hardlimit_ready[cpu] = 1;
	}

	/* call driver. From then on the cpufreq must be able
	 * to accept all calls to ->verify and ->setpolicy for this CPU
	 */
	ret = cpufreq_driver->init(policy);
	if (ret) {
		pr_debug("initialization failed\n");
		goto out_free_policy;
	}

	down_write(&policy->rwsem);

	/* related_cpus should at least include policy->cpus. */
	if (new_policy)
		cpumask_copy(policy->related_cpus, policy->cpus);

	/*
	 * affected cpus must always be the one, which are online. We aren't
	 * managing offline cpus here.
	 */
	cpumask_and(policy->cpus, policy->cpus, cpu_online_mask);

	reapply_hard_limits(policy->cpu, false);

	if (new_policy) {
		policy->user_policy.min = check_cpufreq_hardlimit(policy->cpu, policy->min);
		policy->user_policy.max = check_cpufreq_hardlimit(policy->cpu, policy->max);

		for_each_cpu(j, policy->related_cpus) {
			per_cpu(cpufreq_cpu_data, j) = policy;
			add_cpu_dev_symlink(policy, j);
		}
	} else {
		policy->min = check_cpufreq_hardlimit(policy->cpu, policy->user_policy.min);
		policy->max = check_cpufreq_hardlimit(policy->cpu, policy->user_policy.max);
	}

	policy->cur = cpufreq_driver->get(policy->cpu);
	if (!policy->cur) {
		pr_err("%s: ->get() failed\n", __func__);
		goto out_exit_policy;
	}

	if (new_policy) {
		ret = cpufreq_add_dev_interface(policy);
		if (ret)
			goto out_exit_policy;

		cpufreq_stats_create_table(policy);

		write_lock_irqsave(&cpufreq_driver_lock, flags);
		list_add(&policy->policy_list, &cpufreq_policy_list);
		write_unlock_irqrestore(&cpufreq_driver_lock, flags);
	}

	ret = cpufreq_init_policy(policy);
	if (ret) {
		pr_err("%s: Failed to initialize policy for cpu: %d (%d)\n",
		       __func__, cpu, ret);
		/* cpufreq_policy_free() will notify based on this */
		new_policy = false;
		goto out_exit_policy;
	}

	up_write(&policy->rwsem);

	kobject_uevent(&policy->kobj, KOBJ_ADD);

	/* Callback for handling stuff after policy is ready */
	if (cpufreq_driver->ready) {
		cpufreq_driver->ready(policy);
	}

	pr_debug("CPUFREQ %u: Init Complete!\n", cpu);

	return 0;

out_exit_policy:
	up_write(&policy->rwsem);

	if (cpufreq_driver->exit)
		cpufreq_driver->exit(policy);

	for_each_cpu(j, policy->real_cpus)
		remove_cpu_dev_symlink(policy, get_cpu_device(j));

out_free_policy:
	cpufreq_policy_free(policy);
	return ret;
}

/**
 * cpufreq_add_dev - the cpufreq interface for a CPU device.
 * @dev: CPU device.
 * @sif: Subsystem interface structure pointer (not used)
 */
static int cpufreq_add_dev(struct device *dev, struct subsys_interface *sif)
{
	struct cpufreq_policy *policy;
	unsigned cpu = dev->id;
	int ret;

	dev_dbg(dev, "%s: adding CPU%u\n", __func__, cpu);

	if (cpu_online(cpu)) {
		ret = cpufreq_online(cpu);
		if (ret)
			return ret;
	}

	/* Create sysfs link on CPU registration */
	policy = per_cpu(cpufreq_cpu_data, cpu);
	if (policy)
		add_cpu_dev_symlink(policy, cpu);

	return 0;
}

static int cpufreq_offline(unsigned int cpu)
{
	struct cpufreq_policy *policy;
	int ret;

	pr_debug("%s: unregistering CPU %u\n", __func__, cpu);

	policy = cpufreq_cpu_get_raw(cpu);
	if (!policy) {
		pr_debug("%s: No cpu_data found\n", __func__);
		return 0;
	}

	down_write(&policy->rwsem);
	cpufreq_stop_governor(policy);

	cpumask_clear_cpu(cpu, policy->cpus);

	if (policy_is_inactive(policy)) {
			strncpy(policy->last_governor, policy->governor->name,
				CPUFREQ_NAME_LEN);
	} else if (cpu == policy->cpu) {
		/* Nominate new CPU */
		policy->cpu = cpumask_any(policy->cpus);
	}

	/* Start governor again for active policy */
	if (!policy_is_inactive(policy)) {
			ret = cpufreq_start_governor(policy);
			if (ret)
				pr_err("%s: Failed to start governor\n", __func__);
		goto unlock;
	}

	if (cpufreq_driver->stop_cpu)
		cpufreq_driver->stop_cpu(policy);

	cpufreq_exit_governor(policy);

unlock:
	up_write(&policy->rwsem);
	return 0;
}

/**
 * cpufreq_remove_dev - remove a CPU device
 *
 * Removes the cpufreq interface for a CPU device.
 */
static void cpufreq_remove_dev(struct device *dev, struct subsys_interface *sif)
{
	unsigned int cpu = dev->id;
	struct cpufreq_policy *policy = per_cpu(cpufreq_cpu_data, cpu);

	if (!policy)
		return;

	if (cpu_online(cpu))
		cpufreq_offline(cpu);

	cpumask_clear_cpu(cpu, policy->real_cpus);
	remove_cpu_dev_symlink(policy, dev);

	if (cpumask_empty(policy->real_cpus))
		cpufreq_policy_free(policy);
}

/**
 *	cpufreq_out_of_sync - If actual and saved CPU frequency differs, we're
 *	in deep trouble.
 *	@policy: policy managing CPUs
 *	@new_freq: CPU frequency the CPU actually runs at
 *
 *	We adjust to current frequency first, and need to clean up later.
 *	So either call to cpufreq_update_policy() or schedule handle_update()).
 */
static void cpufreq_out_of_sync(struct cpufreq_policy *policy,
				unsigned int new_freq)
{
	struct cpufreq_freqs freqs;

	pr_debug("Warning: CPU frequency out of sync: cpufreq and timing core thinks of %u, is %u kHz\n",
		 policy->cur, new_freq);

	freqs.old = policy->cur;
	freqs.new = new_freq;

	cpufreq_freq_transition_begin(policy, &freqs);
	cpufreq_freq_transition_end(policy, &freqs, 0);
}

/**
 * cpufreq_quick_get - get the CPU frequency (in kHz) from policy->cur
 * @cpu: CPU number
 *
 * This is the last known freq, without actually getting it from the driver.
 * Return value will be same as what is shown in scaling_cur_freq in sysfs.
 */
unsigned int cpufreq_quick_get(unsigned int cpu)
{
	struct cpufreq_policy *policy;
	unsigned int ret_freq = 0;
	unsigned long flags;

	policy = cpufreq_cpu_get(cpu);
	if (policy) {
		ret_freq = policy->cur;
		cpufreq_cpu_put(policy);
	}

	return ret_freq;
}
EXPORT_SYMBOL(cpufreq_quick_get);

/**
 * cpufreq_quick_get_max - get the max reported CPU frequency for this CPU
 * @cpu: CPU number
 *
 * Just return the max possible frequency for a given CPU.
 */
unsigned int cpufreq_quick_get_max(unsigned int cpu)
{
	struct cpufreq_policy *policy = cpufreq_cpu_get(cpu);
	unsigned int ret_freq = 0;

	if (policy) {
		ret_freq = policy->max;
		cpufreq_cpu_put(policy);
	}

	return ret_freq;
}
EXPORT_SYMBOL(cpufreq_quick_get_max);

/**
 * cpufreq_quick_get_min - get the min reported CPU frequency for this CPU
 * @cpu: CPU number
 *
 * Just return the min possible frequency for a given CPU.
 */
unsigned int cpufreq_quick_get_min(unsigned int cpu)
{
	struct cpufreq_policy *policy = cpufreq_cpu_get(cpu);
	unsigned int ret_freq = 0;

	if (policy) {
		ret_freq = policy->min;
		cpufreq_cpu_put(policy);
	}

	return ret_freq;
}
EXPORT_SYMBOL(cpufreq_quick_get_min);

static unsigned int __cpufreq_get(struct cpufreq_policy *policy)
{
	unsigned int ret_freq = 0;

	if (!cpufreq_driver->get)
		return ret_freq;

	ret_freq = cpufreq_driver->get(policy->cpu);

	/*
	 * Updating inactive policies is invalid, so avoid doing that.  Also
	 * if fast frequency switching is used with the given policy, the check
	 * against policy->cur is pointless, so skip it in that case too.
	 */
	if (unlikely(policy_is_inactive(policy)))
		return ret_freq;

	if (ret_freq && policy->cur &&
		!(cpufreq_driver->flags & CPUFREQ_CONST_LOOPS)) {
		/* verify no discrepancy between actual and
					saved value exists */
		if (unlikely(ret_freq != policy->cur)) {
			cpufreq_out_of_sync(policy, ret_freq);
			schedule_work(&policy->update);
		}
	}

	return ret_freq;
}

/**
 * cpufreq_get - get the current CPU frequency (in kHz)
 * @cpu: CPU number
 *
 * Get the CPU current (static) CPU frequency
 */
unsigned int cpufreq_get(unsigned int cpu)
{
	struct cpufreq_policy *policy = cpufreq_cpu_get(cpu);
	unsigned int ret_freq = 0;

	if (policy) {
		down_read(&policy->rwsem);

		if (!policy_is_inactive(policy))
			ret_freq = __cpufreq_get(policy);

		up_read(&policy->rwsem);

		cpufreq_cpu_put(policy);
	}

	return ret_freq;
}
EXPORT_SYMBOL(cpufreq_get);

static unsigned int cpufreq_update_current_freq(struct cpufreq_policy *policy)
{
	unsigned int new_freq;

	new_freq = cpufreq_driver->get(policy->cpu);
	if (!new_freq)
		return 0;

	if (!policy->cur) {
		pr_debug("cpufreq: Driver did not initialize current freq\n");
		policy->cur = new_freq;
	} else if (policy->cur != new_freq && has_target()) {
		cpufreq_out_of_sync(policy, new_freq);
	}

	return new_freq;
}

static struct subsys_interface cpufreq_interface = {
	.name		= "cpufreq",
	.subsys		= &cpu_subsys,
	.add_dev	= cpufreq_add_dev,
	.remove_dev	= cpufreq_remove_dev,
};

/*
 * In case platform wants some specific frequency to be configured
 * during suspend..
 */
int cpufreq_generic_suspend(struct cpufreq_policy *policy)
{
	int ret;

	if (!policy->suspend_freq) {
		pr_debug("%s: suspend_freq not defined\n", __func__);
		return 0;
	}

	pr_debug("%s: Setting suspend-freq: %u\n", __func__,
			policy->suspend_freq);

	ret = __cpufreq_driver_target(policy, policy->suspend_freq,
			CPUFREQ_RELATION_H);
	if (ret)
		pr_err("%s: unable to set suspend-freq: %u. err: %d\n",
				__func__, policy->suspend_freq, ret);

	return ret;
}
EXPORT_SYMBOL(cpufreq_generic_suspend);

/**
 * cpufreq_suspend() - Suspend CPUFreq governors
 *
 * Called during system wide Suspend/Hibernate cycles for suspending governors
 * as some platforms can't change frequency after this point in suspend cycle.
 * Because some of the devices (like: i2c, regulators, etc) they use for
 * changing frequency are suspended quickly after this point.
 */
void cpufreq_suspend(void)
{
	struct cpufreq_policy *policy;

	if (!cpufreq_driver)
		return;

	if (!has_target() && !cpufreq_driver->suspend)
		goto suspend;

	pr_debug("%s: Suspending Governors\n", __func__);

	for_each_active_policy(policy) {
		if (has_target()) {
			down_write(&policy->rwsem);
			cpufreq_stop_governor(policy);
			up_write(&policy->rwsem);
		}

		if (cpufreq_driver->suspend && cpufreq_driver->suspend(policy))
			pr_err("%s: Failed to suspend driver: %p\n", __func__,
				policy);
	}

suspend:
	cpufreq_suspended = true;
}

/**
 * cpufreq_resume() - Resume CPUFreq governors
 *
 * Called during system wide Suspend/Hibernate cycle for resuming governors that
 * are suspended with cpufreq_suspend().
 */
void cpufreq_resume(void)
{
	struct cpufreq_policy *policy;
	int ret;

	if (!cpufreq_driver)
		return;

	cpufreq_suspended = false;

	if (!has_target() && !cpufreq_driver->resume)
		return;

	pr_debug("%s: Resuming Governors\n", __func__);

	for_each_active_policy(policy) {
		if (cpufreq_driver->resume && cpufreq_driver->resume(policy)) {
			pr_err("%s: Failed to resume driver: %p\n", __func__,
				policy);
		} else if (has_target()) {
			down_write(&policy->rwsem);
			ret = cpufreq_start_governor(policy);
			up_write(&policy->rwsem);

			if (ret)
				pr_err("%s: Failed to start governor for policy: %p\n",
				       __func__, policy);
		}
	}
}

/**
 *	cpufreq_get_current_driver - return current driver's name
 *
 *	Return the name string of the currently loaded cpufreq driver
 *	or NULL, if none.
 */
const char *cpufreq_get_current_driver(void)
{
	if (cpufreq_driver)
		return cpufreq_driver->name;

	return NULL;
}
EXPORT_SYMBOL_GPL(cpufreq_get_current_driver);

/**
 *	cpufreq_get_driver_data - return current driver data
 *
 *	Return the private data of the currently loaded cpufreq
 *	driver, or NULL if no cpufreq driver is loaded.
 */
void *cpufreq_get_driver_data(void)
{
	if (cpufreq_driver)
		return cpufreq_driver->driver_data;

	return NULL;
}
EXPORT_SYMBOL_GPL(cpufreq_get_driver_data);

/*********************************************************************
 *                     NOTIFIER LISTS INTERFACE                      *
 *********************************************************************/

/**
 *	cpufreq_register_notifier - register a driver with cpufreq
 *	@nb: notifier function to register
 *      @list: CPUFREQ_TRANSITION_NOTIFIER or CPUFREQ_POLICY_NOTIFIER
 *
 *	Add a driver to one of two lists: either a list of drivers that
 *      are notified about clock rate changes (once before and once after
 *      the transition), or a list of drivers that are notified about
 *      changes in cpufreq policy.
 *
 *	This function may sleep, and has the same return conditions as
 *	blocking_notifier_chain_register.
 */
int cpufreq_register_notifier(struct notifier_block *nb, unsigned int list)
{
	int ret;

	WARN_ON(!init_cpufreq_transition_notifier_list_called);

	switch (list) {
	case CPUFREQ_TRANSITION_NOTIFIER:
		mutex_lock(&cpufreq_fast_switch_lock);

		if (cpufreq_fast_switch_count > 0) {
			mutex_unlock(&cpufreq_fast_switch_lock);
			return -EBUSY;
		}
		ret = srcu_notifier_chain_register(
				&cpufreq_transition_notifier_list, nb);
		if (!ret)
			cpufreq_fast_switch_count--;

		mutex_unlock(&cpufreq_fast_switch_lock);
		break;
	case CPUFREQ_POLICY_NOTIFIER:
		ret = blocking_notifier_chain_register(
				&cpufreq_policy_notifier_list, nb);
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}
EXPORT_SYMBOL(cpufreq_register_notifier);

/**
 *	cpufreq_unregister_notifier - unregister a driver with cpufreq
 *	@nb: notifier block to be unregistered
 *	@list: CPUFREQ_TRANSITION_NOTIFIER or CPUFREQ_POLICY_NOTIFIER
 *
 *	Remove a driver from the CPU frequency notifier list.
 *
 *	This function may sleep, and has the same return conditions as
 *	blocking_notifier_chain_unregister.
 */
int cpufreq_unregister_notifier(struct notifier_block *nb, unsigned int list)
{
	int ret;

	switch (list) {
	case CPUFREQ_TRANSITION_NOTIFIER:
		mutex_lock(&cpufreq_fast_switch_lock);

		ret = srcu_notifier_chain_unregister(
				&cpufreq_transition_notifier_list, nb);
		if (!ret && !WARN_ON(cpufreq_fast_switch_count >= 0))
			cpufreq_fast_switch_count++;

		mutex_unlock(&cpufreq_fast_switch_lock);
		break;
	case CPUFREQ_POLICY_NOTIFIER:
		ret = blocking_notifier_chain_unregister(
				&cpufreq_policy_notifier_list, nb);
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}
EXPORT_SYMBOL(cpufreq_unregister_notifier);


/*********************************************************************
 *                              GOVERNORS                            *
 *********************************************************************/

/* Must set freqs->new to intermediate frequency */
static int __target_intermediate(struct cpufreq_policy *policy,
				 struct cpufreq_freqs *freqs, int index)
{
	int ret;

	freqs->new = cpufreq_driver->get_intermediate(policy, index);

	/* We don't need to switch to intermediate freq */
	if (!freqs->new)
		return 0;

	pr_debug("%s: cpu: %d, switching to intermediate freq: oldfreq: %u, intermediate freq: %u\n",
		 __func__, policy->cpu, freqs->old, freqs->new);

	cpufreq_freq_transition_begin(policy, freqs);
	ret = cpufreq_driver->target_intermediate(policy, index);
	cpufreq_freq_transition_end(policy, freqs, ret);

	if (ret)
		pr_err("%s: Failed to change to intermediate frequency: %d\n",
		       __func__, ret);

	return ret;
}

static int __target_index(struct cpufreq_policy *policy, int index)
{
	struct cpufreq_freqs freqs = {.old = policy->cur, .flags = 0};
	unsigned int intermediate_freq = 0;
	unsigned int newfreq = policy->freq_table[index].frequency;
	int retval = -EINVAL;
	bool notify;

	if (newfreq == policy->cur)
		return 0;

	notify = !(cpufreq_driver->flags & CPUFREQ_ASYNC_NOTIFICATION);
	if (notify) {
		/* Handle switching to intermediate frequency */
		if (cpufreq_driver->get_intermediate) {
			retval = __target_intermediate(policy, &freqs, index);
			if (retval)
				return retval;

			intermediate_freq = freqs.new;
			/* Set old freq to intermediate */
			if (intermediate_freq)
				freqs.old = freqs.new;
		}

		freqs.new = newfreq;
		pr_debug("%s: cpu: %d, oldfreq: %u, new freq: %u\n",
			 __func__, policy->cpu, freqs.old, freqs.new);

		cpufreq_freq_transition_begin(policy, &freqs);
	}

	retval = cpufreq_driver->target_index(policy, index);
	if (retval)
		pr_err("%s: Failed to change cpu frequency: %d\n", __func__,
		       retval);

	if (notify) {
		cpufreq_freq_transition_end(policy, &freqs, retval);

		/*
		 * Failed after setting to intermediate freq? Driver should have
		 * reverted back to initial frequency and so should we. Check
		 * here for intermediate_freq instead of get_intermediate, in
		 * case we haven't switched to intermediate freq at all.
		 */
		if (unlikely(retval && intermediate_freq)) {
			freqs.old = intermediate_freq;
			freqs.new = policy->restore_freq;
			cpufreq_freq_transition_begin(policy, &freqs);
			cpufreq_freq_transition_end(policy, &freqs, 0);
		}
	}

	return retval;
}

int __cpufreq_driver_target(struct cpufreq_policy *policy,
			    unsigned int target_freq,
			    unsigned int relation)
{
	unsigned int old_target_freq = target_freq;
	int index;

	/* Make sure that target_freq is within supported range */
	target_freq = clamp_val(target_freq, check_cpufreq_hardlimit(policy->cpu, policy->min), check_cpufreq_hardlimit(policy->cpu, policy->max));

	pr_debug("target for CPU %u: %u kHz, relation %u, requested %u kHz\n",
		 policy->cpu, target_freq, relation, old_target_freq);

	/*
	 * This might look like a redundant call as we are checking it again
	 * after finding index. But it is left intentionally for cases where
	 * exactly same freq is called again and so we can save on few function
	 * calls.
	 */
	if (target_freq == policy->cur)
		return 0;

	/* Save last value to restore later on errors */
	policy->restore_freq = policy->cur;

	if (cpufreq_driver->target)
		return cpufreq_driver->target(policy, target_freq, relation);

	if (!cpufreq_driver->target_index)
		return -EINVAL;

	index = cpufreq_frequency_table_target(policy, target_freq, relation);

	return __target_index(policy, index);
}
EXPORT_SYMBOL_GPL(__cpufreq_driver_target);

int cpufreq_driver_target(struct cpufreq_policy *policy,
			  unsigned int target_freq,
			  unsigned int relation)
{
	int ret = -EINVAL;

	down_write(&policy->rwsem);

	ret = __cpufreq_driver_target(policy, target_freq, relation);

	up_write(&policy->rwsem);

	return ret;
}
EXPORT_SYMBOL_GPL(cpufreq_driver_target);

__weak struct cpufreq_governor *cpufreq_fallback_governor(void)
{
	return NULL;
}

static int cpufreq_init_governor(struct cpufreq_policy *policy)
{
	int ret;

	/* Don't start any governor operations if we are entering suspend */
	if (cpufreq_suspended)
		return 0;
	/*
	 * Governor might not be initiated here if ACPI _PPC changed
	 * notification happened, so check it.
	 */
	if (!policy->governor)
		return -EINVAL;

	/* Platform doesn't want dynamic frequency switching ? */
	if (policy->governor->dynamic_switching &&
	    cpufreq_driver->flags & CPUFREQ_NO_AUTO_DYNAMIC_SWITCHING) {
		struct cpufreq_governor *gov = cpufreq_fallback_governor();

		if (gov) {
			pr_warn("Can't use %s governor as dynamic switching is disallowed. Fallback to %s governor\n",
				policy->governor->name, gov->name);
			policy->governor = gov;
		} else {
			return -EINVAL;
		}
	}

	if (!try_module_get(policy->governor->owner))
		return -EINVAL;

	pr_debug("%s: for CPU %u\n", __func__, policy->cpu);

	if (policy->governor->init) {
		ret = policy->governor->init(policy);
		if (ret) {
			module_put(policy->governor->owner);
			return ret;
		}
	}

	return 0;
}

static void cpufreq_exit_governor(struct cpufreq_policy *policy)
{
	if (cpufreq_suspended || !policy->governor)
		return;

	pr_debug("%s: for CPU %u\n", __func__, policy->cpu);

	if (policy->governor->exit)
		policy->governor->exit(policy);

	module_put(policy->governor->owner);
}

static int cpufreq_start_governor(struct cpufreq_policy *policy)
{
	int ret;

	if (cpufreq_suspended)
		return 0;

	if (!policy->governor)
		return -EINVAL;

	pr_debug("%s: for CPU %u\n", __func__, policy->cpu);

	cpufreq_update_current_freq(policy);

	if (policy->governor->start) {
		ret = policy->governor->start(policy);
		if (ret)
			return ret;
	}

	cpufreq_governor_limits(policy, true);

	return 0;
}

static void cpufreq_stop_governor(struct cpufreq_policy *policy)
{
	if (cpufreq_suspended || !policy->governor)
		return;

	pr_debug("%s: for CPU %u\n", __func__, policy->cpu);

	if (policy->governor->stop)
		policy->governor->stop(policy);
}

static void cpufreq_governor_limits(struct cpufreq_policy *policy, bool from_start)
{
	if (!from_start) {
		if (cpufreq_suspended || !policy->governor)
			return;
	}

	if (policy->governor->limits)
		policy->governor->limits(policy);
}

int cpufreq_register_governor(struct cpufreq_governor *governor)
{
	int err;

	if (!governor)
		return -EINVAL;

	mutex_lock(&cpufreq_governor_mutex);

	err = -EBUSY;
	if (!find_governor(governor->name)) {
		err = 0;
		list_add(&governor->governor_list, &cpufreq_governor_list);
	}

	mutex_unlock(&cpufreq_governor_mutex);
	return err;
}
EXPORT_SYMBOL_GPL(cpufreq_register_governor);

void cpufreq_unregister_governor(struct cpufreq_governor *governor)
{
	struct cpufreq_policy *policy;
	unsigned long flags;

	if (!governor)
		return;

	/* clear last_governor for all inactive policies */
	read_lock_irqsave(&cpufreq_driver_lock, flags);
	for_each_inactive_policy(policy) {
		if (!strcmp(policy->last_governor, governor->name)) {
			policy->governor = NULL;
			strcpy(policy->last_governor, "\0");
		}
	}
	read_unlock_irqrestore(&cpufreq_driver_lock, flags);

	mutex_lock(&cpufreq_governor_mutex);
	list_del(&governor->governor_list);
	mutex_unlock(&cpufreq_governor_mutex);
	return;
}
EXPORT_SYMBOL_GPL(cpufreq_unregister_governor);


/*********************************************************************
 *                          POLICY INTERFACE                         *
 *********************************************************************/

/**
 * cpufreq_get_policy - get the current cpufreq_policy
 * @policy: struct cpufreq_policy into which the current cpufreq_policy
 *	is written
 *
 * Reads the current cpufreq policy.
 */
int cpufreq_get_policy(struct cpufreq_policy *policy, unsigned int cpu)
{
	struct cpufreq_policy *cpu_policy;
	if (!policy)
		return -EINVAL;

	cpu_policy = cpufreq_cpu_get(cpu);
	if (!cpu_policy)
		return -EINVAL;

	memcpy(policy, cpu_policy, sizeof(*policy));

	cpufreq_cpu_put(cpu_policy);
	return 0;
}
EXPORT_SYMBOL(cpufreq_get_policy);

/*
 * policy : current policy.
 * new_policy: policy to be set.
 */
static int cpufreq_set_policy(struct cpufreq_policy *policy,
				struct cpufreq_policy *new_policy)
{
	struct cpufreq_governor *old_gov;
	int ret;
	struct hardlimit_policy *hpolicy;

	pr_debug("setting new policy for CPU %u: %u - %u kHz\n",
		 new_policy->cpu, new_policy->min, new_policy->max);

	memcpy(&new_policy->cpuinfo, &policy->cpuinfo, sizeof(policy->cpuinfo));

	/*
	* This check works well when we store new min/max freq attributes,
	* because new_policy is a copy of policy with one field updated.
	*/
	if (new_policy->min > new_policy->max)
		return -EINVAL;

	/* verify the cpu speed can be set within this limit */
	ret = cpufreq_driver->verify(new_policy);
	if (ret)
		return ret;

	/* adjust if necessary - all reasons */
	blocking_notifier_call_chain(&cpufreq_policy_notifier_list,
			CPUFREQ_ADJUST, new_policy);

	/*
	 * verify the cpu speed can be set within this limit, which might be
	 * different to the first one
	 */
	ret = cpufreq_driver->verify(new_policy);
	if (ret)
		return ret;

	/* notification of the new policy */
	blocking_notifier_call_chain(&cpufreq_policy_notifier_list,
			CPUFREQ_NOTIFY, new_policy);

	policy->min = new_policy->min;
	policy->max = new_policy->max;

	hpolicy = hardlimit_get_raw(policy->cpu);
	if (hpolicy) {
		if (unlikely(policy->min != hpolicy->current_limit_min ||
			policy->max != hpolicy->current_limit_max))
			reapply_hard_limits(policy->cpu, false);
	}

	policy->cached_target_freq = UINT_MAX;

	pr_debug("new min and max freqs are %u - %u kHz\n",
		 policy->min, policy->max);

	if (new_policy->governor == policy->governor) {
		pr_debug("cpufreq: governor limits update\n");
		cpufreq_governor_limits(policy, false);
		return 0;
	}

	/* save old, working values */
	old_gov = policy->governor;
	/* end old governor */
	if (old_gov) {
		cpufreq_stop_governor(policy);
		cpufreq_exit_governor(policy);
	}

	/* start new governor */
	policy->governor = new_policy->governor;
	ret = cpufreq_init_governor(policy);
	if (!ret) {
		ret = cpufreq_start_governor(policy);
		if (!ret) {
			pr_debug("cpufreq: governor change\n");
			return 0;
		}
		cpufreq_exit_governor(policy);
	}

	/* new governor failed, so re-start old one */
	pr_debug("starting governor %s failed\n", policy->governor->name);
	if (old_gov) {
		policy->governor = old_gov;
		if (cpufreq_init_governor(policy)) {
			policy->governor = NULL;
		} else {
			cpufreq_start_governor(policy);
			old_gov = NULL;
		}
	}

	return ret;
}

/**
 *	cpufreq_update_policy - re-evaluate an existing cpufreq policy
 *	@cpu: CPU which shall be re-evaluated
 *
 *	Useful for policy notifiers which have different necessities
 *	at different times.
 */
void cpufreq_update_policy(unsigned int cpu)
{
	struct cpufreq_policy *policy = cpufreq_cpu_get(cpu);
	struct cpufreq_policy new_policy;

	if (!policy)
		return;

	down_write(&policy->rwsem);

	if (policy_is_inactive(policy))
		goto unlock;

	pr_debug("updating policy for CPU %u\n", cpu);
	memcpy(&new_policy, policy, sizeof(*policy));
	/* Yank555.lu - Enforce hardlimit */
	new_policy.min = check_cpufreq_hardlimit(policy->cpu, policy->user_policy.min);
	new_policy.max = check_cpufreq_hardlimit(policy->cpu, policy->user_policy.max);
	/*
	 * BIOS might change freq behind our back
	 * -> ask driver for current freq and notify governors about a change
	 */
	if (cpufreq_suspended)
		goto unlock;

	new_policy.cur = cpufreq_update_current_freq(policy);
	if (WARN_ON(!new_policy.cur))
		goto unlock;

	cpufreq_set_policy(policy, &new_policy);

unlock:
	up_write(&policy->rwsem);

	cpufreq_cpu_put(policy);
}
EXPORT_SYMBOL(cpufreq_update_policy);

/*********************************************************************
 *               BOOST						     *
 *********************************************************************/
static int cpufreq_boost_set_sw(int state)
{
	struct cpufreq_policy *policy;
	int ret = -EINVAL;

	for_each_active_policy(policy) {
		if (!policy->freq_table)
			continue;

		ret = cpufreq_frequency_table_cpuinfo(policy,
						      policy->freq_table);
		if (ret) {
			pr_err("%s: Policy frequency update failed\n",
			       __func__);
			break;
		}

		down_write(&policy->rwsem);
		policy->user_policy.max = policy->max;
		cpufreq_governor_limits(policy, false);
		up_write(&policy->rwsem);
	}

	return ret;
}

int cpufreq_boost_trigger_state(int state)
{
	unsigned long flags;
	int ret = 0;

	if (cpufreq_driver->boost_enabled == state)
		return 0;

	write_lock_irqsave(&cpufreq_driver_lock, flags);
	cpufreq_driver->boost_enabled = state;
	write_unlock_irqrestore(&cpufreq_driver_lock, flags);

	ret = cpufreq_driver->set_boost(state);
	if (ret) {
		write_lock_irqsave(&cpufreq_driver_lock, flags);
		cpufreq_driver->boost_enabled = !state;
		write_unlock_irqrestore(&cpufreq_driver_lock, flags);

		pr_err("%s: Cannot %s BOOST\n",
		       __func__, state ? "enable" : "disable");
	}

	return ret;
}

static bool cpufreq_boost_supported(void)
{
	return likely(cpufreq_driver) && cpufreq_driver->set_boost;
}

static int create_boost_sysfs_file(void)
{
	int ret;

	ret = sysfs_create_file(cpufreq_global_kobject, &boost.attr);
	if (ret)
		pr_err("%s: cannot register global BOOST sysfs file\n",
		       __func__);

	return ret;
}

static void remove_boost_sysfs_file(void)
{
	if (cpufreq_boost_supported())
		sysfs_remove_file(cpufreq_global_kobject, &boost.attr);
}

int cpufreq_enable_boost_support(void)
{
	if (!cpufreq_driver)
		return -EINVAL;

	if (cpufreq_boost_supported())
		return 0;

	cpufreq_driver->set_boost = cpufreq_boost_set_sw;

	/* This will get removed on driver unregister */
	return create_boost_sysfs_file();
}
EXPORT_SYMBOL_GPL(cpufreq_enable_boost_support);

int cpufreq_boost_enabled(void)
{
	return cpufreq_driver->boost_enabled;
}
EXPORT_SYMBOL_GPL(cpufreq_boost_enabled);


static enum cpuhp_state hp_online;

static int cpuhp_cpufreq_online(unsigned int cpu)
{
	if (!cpufreq_suspended)
		cpufreq_online(cpu);

	return 0;
}

static int cpuhp_cpufreq_offline(unsigned int cpu)
{
	if (!cpufreq_suspended)
		cpufreq_offline(cpu);

	return 0;
}
/*********************************************************************
 *               REGISTER / UNREGISTER CPUFREQ DRIVER                *
 *********************************************************************/
/**
 * cpufreq_register_driver - register a CPU Frequency driver
 * @driver_data: A struct cpufreq_driver containing the values#
 * submitted by the CPU Frequency driver.
 *
 * Registers a CPU Frequency driver to this core code. This code
 * returns zero on success, -EEXIST when another driver got here first
 * (and isn't unregistered in the meantime).
 *
 */

static void hardlimit_memalloc(void)
{
	struct hardlimit_policy *hpolicy;
	unsigned int cpu;
	for_each_possible_cpu(cpu) {
		if (cpu_out_of_range(cpu))
			break;
		hpolicy = kzalloc(sizeof(struct hardlimit_policy), GFP_KERNEL);
		BUG_ON(!hpolicy);
	}
}
int cpufreq_register_driver(struct cpufreq_driver *driver_data)
{
	unsigned long flags;
	int ret;

	if (!driver_data || !driver_data->verify || !driver_data->init ||
	    !(driver_data->setpolicy || driver_data->target_index ||
		    driver_data->target) ||
	     (driver_data->setpolicy && (driver_data->target_index ||
		    driver_data->target)) ||
	     (!!driver_data->get_intermediate != !!driver_data->target_intermediate))
		return -EINVAL;

	pr_debug("trying to register driver %s\n", driver_data->name);

	/* Protect against concurrent CPU online/offline. */
	cpus_read_lock();

	write_lock_irqsave(&cpufreq_driver_lock, flags);
	if (cpufreq_driver) {
		write_unlock_irqrestore(&cpufreq_driver_lock, flags);
		ret = -EEXIST;
		goto out;
	}
	cpufreq_driver = driver_data;
	write_unlock_irqrestore(&cpufreq_driver_lock, flags);

	if (cpufreq_boost_supported()) {
		ret = create_boost_sysfs_file();
		if (ret)
			goto err_null_driver;
	}

	ret = subsys_interface_register(&cpufreq_interface);
	if (ret)
		goto err_boost_unreg;

	if (!(cpufreq_driver->flags & CPUFREQ_STICKY) &&
	    list_empty(&cpufreq_policy_list)) {
		/* if all ->init() calls failed, unregister */
		ret = -ENODEV;
		pr_debug("%s: No CPU initialized for driver %s\n", __func__,
			 driver_data->name);
		goto err_if_unreg;
	}

	ret = cpuhp_setup_state_nocalls_cpuslocked(CPUHP_AP_ONLINE_DYN,
						   "cpufreq:online",
						   cpuhp_cpufreq_online,
						   cpuhp_cpufreq_offline);
	if (ret < 0)
		goto err_if_unreg;
	hp_online = ret;
	ret = 0;

	pr_info("driver %s up and running\n", driver_data->name);
	
	goto out;

err_if_unreg:
	subsys_interface_unregister(&cpufreq_interface);
err_boost_unreg:
	remove_boost_sysfs_file();
err_null_driver:
	write_lock_irqsave(&cpufreq_driver_lock, flags);
	cpufreq_driver = NULL;
	write_unlock_irqrestore(&cpufreq_driver_lock, flags);
out:
	cpus_read_unlock();
	hardlimit_memalloc();
	return ret;
}
EXPORT_SYMBOL_GPL(cpufreq_register_driver);

/**
 * cpufreq_unregister_driver - unregister the current CPUFreq driver
 *
 * Unregister the current CPUFreq driver. Only call this if you have
 * the right to do so, i.e. if you have succeeded in initialising before!
 * Returns zero if successful, and -EINVAL if the cpufreq_driver is
 * currently not initialised.
 */
int cpufreq_unregister_driver(struct cpufreq_driver *driver)
{
	unsigned long flags;

	if (!cpufreq_driver || (driver != cpufreq_driver))
		return -EINVAL;

	pr_debug("unregistering driver %s\n", driver->name);

	/* Protect against concurrent cpu hotplug */
	cpus_read_lock();
	subsys_interface_unregister(&cpufreq_interface);
	remove_boost_sysfs_file();
	cpuhp_remove_state_nocalls_cpuslocked(hp_online);

	write_lock_irqsave(&cpufreq_driver_lock, flags);

	cpufreq_driver = NULL;

	write_unlock_irqrestore(&cpufreq_driver_lock, flags);
	cpus_read_unlock();

	return 0;
}
EXPORT_SYMBOL_GPL(cpufreq_unregister_driver);

/*
 * Stop cpufreq at shutdown to make sure it isn't holding any locks
 * or mutexes when secondary CPUs are halted.
 */
static struct syscore_ops cpufreq_syscore_ops = {
	.shutdown = cpufreq_suspend,
};

struct kobject *cpufreq_global_kobject;
EXPORT_SYMBOL(cpufreq_global_kobject);

static int __init cpufreq_core_init(void)
{
#ifdef CONFIG_CPU_VOLTAGE_TABLE
	int rc;
#endif	/* CONFIG_CPU_VOLTAGE_TABLE */

	cpufreq_global_kobject = kobject_create_and_add("cpufreq", &cpu_subsys.dev_root->kobj);
	BUG_ON(!cpufreq_global_kobject);

	register_syscore_ops(&cpufreq_syscore_ops);
#ifdef CONFIG_CPU_VOLTAGE_TABLE
	rc = sysfs_create_group(cpufreq_global_kobject, &vddtbl_attr_group);
#endif	/* CONFIG_CPU_VOLTAGE_TABLE */
	cpumask_copy(&throttled_mask, cpu_possible_mask);
	cpumask_clear(&throttled_mask);
	cpu_boost_wq = alloc_workqueue("cpuboost_wq", WQ_HIGHPRI | WQ_FREEZABLE, 0);
	if (!cpu_boost_wq)
		return -EFAULT;

	INIT_DELAYED_WORK(&input_boost_work, do_input_boost);
	INIT_DELAYED_WORK(&input_boost_rem, do_input_boost_rem);

	return 0;
}
core_initcall(cpufreq_core_init);
