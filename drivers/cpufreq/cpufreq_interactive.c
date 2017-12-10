/*
 * drivers/cpufreq/cpufreq_interactive.c
 *
 * Copyright (C) 2010-2016 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Author: Mike Chan (mike@android.com)
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/cpufreq.h>
#include <linux/irq_work.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/rwsem.h>
#include <linux/sched.h>
#include <linux/sched/rt.h>
#include <linux/tick.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/suspend.h>
#include <linux/cpu_pm.h>

#include "cpufreq_machinex_gov_attr.h"

#define gov_attr_ro(_name)						\
static struct governor_attr _name =					\
__ATTR(_name, 0444, show_##_name, NULL)

#define gov_attr_wo(_name)						\
static struct governor_attr _name =					\
__ATTR(_name, 0200, NULL, store_##_name)

#define gov_attr_rw(_name)						\
static struct governor_attr _name =					\
__ATTR(_name, 0644, show_##_name, store_##_name)

#define DEFAULT_SAMPLING_RATE (20 * USEC_PER_MSEC)
#define DEFAULT_ABOVE_HISPEED_DELAY DEFAULT_SAMPLING_RATE
#define DEFAULT_TIMER_SLACK DEFAULT_SAMPLING_RATE
#define DEFAULT_MIN_SAMPLE_TIME DEFAULT_SAMPLING_RATE

static unsigned int interactive_suspended;
unsigned int iactive_load_debug __read_mostly;
module_param(iactive_load_debug, uint, 0644);
unsigned int iactive_choose_freq[NR_CPUS];
unsigned int iactive_load_over_target[NR_CPUS];
/* Separate instance required for each 'interactive' directory in sysfs */
struct interactive_tunables {
	struct gov_attr_set attr_set;

	/* Hi speed to bump to from lo speed when load burst (default max) */
	unsigned int hispeed_freq;

	/* Go to hi speed when CPU load at or above this value. */
	unsigned long go_hispeed_load;

	/* Target load. Lower values result in higher CPU speeds. */
	spinlock_t target_loads_lock;
	unsigned int *target_loads;
	int ntarget_loads;

	/*
	 * The minimum amount of time to spend at a frequency before we can ramp
	 * down.
	 */
	unsigned long min_sample_time;

	/* The sample rate of the timer used to increase frequency */
	unsigned long sampling_rate;

	/*
	 * Wait this long before raising speed above hispeed, by default a
	 * single timer interval.
	 */
	spinlock_t above_hispeed_delay_lock;
	unsigned int *above_hispeed_delay;
	int nabove_hispeed_delay;

	/*
	 * Max additional time to wait in idle, beyond sampling_rate, at speeds
	 * above minimum before wakeup to reduce speed, or -1 if unnecessary.
	 */
	unsigned long timer_slack_delay;
	unsigned long timer_slack;
};

/* Separate instance required for each 'struct cpufreq_policy' */
struct interactive_policy {
	struct cpufreq_policy *policy;
	struct interactive_tunables *tunables;
	struct list_head tunables_hook;
};

/* Separate instance required for each CPU */
struct interactive_cpu {
	struct update_util_data update_util;
	struct interactive_policy *ipolicy;

	struct irq_work irq_work;
	u64 last_sample_time;
	unsigned long next_sample_jiffies;
	bool work_in_progress;
	bool timer_is_busy;

	struct rw_semaphore enable_sem;
	struct timer_list slack_timer;

	spinlock_t load_lock; /* protects the next 4 fields */
	u64 time_in_idle;
	u64 time_in_idle_timestamp;
	u64 cputime_speedadj;
	u64 cputime_speedadj_timestamp;

	spinlock_t target_freq_lock; /*protects target freq */
	unsigned int target_freq;

	unsigned int floor_freq;
	u64 pol_floor_val_time; /* policy floor_validate_time */
	u64 loc_floor_val_time; /* per-cpu floor_validate_time */
	u64 pol_hispeed_val_time; /* policy hispeed_validate_time */
	u64 loc_hispeed_val_time; /* per-cpu hispeed_validate_time */
};

static DEFINE_PER_CPU(struct interactive_cpu, interactive_cpu);

/* Realtime thread handles frequency scaling */
static struct task_struct *speedchange_task;
static cpumask_t speedchange_cpumask;
static spinlock_t speedchange_cpumask_lock;

unsigned int iactive_current_load[NR_CPUS];
unsigned int iactive_raw_loadadjfreq[NR_CPUS];
unsigned int full_speed_load = 85;
#define hlimit_hispeed(cpu) check_cpufreq_hardlimit(cpu, iactive_hispeed_freq[cpu]) 
/* Target load. Lower values result in higher CPU speeds. */
#define DEFAULT_TARGET_LOAD 75
static unsigned int default_target_loads[] = {DEFAULT_TARGET_LOAD};

static unsigned int default_above_hispeed_delay[] = {
	DEFAULT_ABOVE_HISPEED_DELAY
};

/* Iterate over interactive policies for tunables */
#define for_each_ipolicy(__ip)	\
	list_for_each_entry(__ip, &tunables->attr_set.policy_list, tunables_hook)

static DEFINE_MUTEX(tunables_lock);
static DEFINE_MUTEX(ilock);
static inline void update_slack_delay(struct interactive_tunables *tunables)
{
	tunables->timer_slack_delay = usecs_to_jiffies(tunables->timer_slack +
						       tunables->sampling_rate);
}

static bool timer_slack_required(struct interactive_cpu *icpu)
{
	struct interactive_policy *ipolicy = icpu->ipolicy;
	struct interactive_tunables *tunables = ipolicy->tunables;

	if (icpu->timer_is_busy)
		return false;

	if (tunables->timer_slack < 0)
		return false;

	if (icpu->target_freq > ipolicy->policy->min)
		return true;

	return false;
}

static void gov_slack_timer_start(struct interactive_cpu *icpu, unsigned int cpu)
{
	struct interactive_tunables *tunables = icpu->ipolicy->tunables;

	if (!timer_pending(&icpu->slack_timer)) {
		icpu->slack_timer.expires = jiffies + tunables->timer_slack_delay;
		add_timer_on(&icpu->slack_timer, cpu);
	} else {
		mod_timer(&icpu->slack_timer, jiffies + tunables->timer_slack_delay);
	}
}

static void gov_slack_timer_modify(struct interactive_cpu *icpu)
{
	struct interactive_tunables *tunables = icpu->ipolicy->tunables;

	mod_timer(&icpu->slack_timer, jiffies + tunables->timer_slack_delay);
}

static void slack_timer_resched(struct interactive_cpu *icpu, unsigned int cpu,
				bool modify)
{
	struct interactive_tunables *tunables = icpu->ipolicy->tunables;
	unsigned long flags;

	spin_lock_irqsave(&icpu->load_lock, flags);

	icpu->time_in_idle = get_cpu_idle_time(cpu,
					       &icpu->time_in_idle_timestamp);
	icpu->cputime_speedadj = 0;
	icpu->cputime_speedadj_timestamp = icpu->time_in_idle_timestamp;

	if (timer_slack_required(icpu)) {
		if (modify)
			gov_slack_timer_modify(icpu);
		else
			gov_slack_timer_start(icpu, cpu);
	}

	spin_unlock_irqrestore(&icpu->load_lock, flags);
}

static unsigned int
freq_to_above_hispeed_delay(struct interactive_tunables *tunables,
			    unsigned int freq)
{
	unsigned long flags;
	unsigned int ret;
	int i;

	spin_lock_irqsave(&tunables->above_hispeed_delay_lock, flags);

	for (i = 0; i < tunables->nabove_hispeed_delay - 1 &&
	     freq >= tunables->above_hispeed_delay[i + 1]; i += 2)
		;

	ret = tunables->above_hispeed_delay[i];
	spin_unlock_irqrestore(&tunables->above_hispeed_delay_lock, flags);

	return ret;
}

static unsigned int freq_to_targetload(struct interactive_tunables *tunables,
				       unsigned int freq)
{
	unsigned long flags;
	unsigned int ret;
	int i;

	spin_lock_irqsave(&tunables->target_loads_lock, flags);

	for (i = 0; i < tunables->ntarget_loads - 1 &&
	     freq >= tunables->target_loads[i + 1]; i += 2)
		;

	ret = tunables->target_loads[i];
	spin_unlock_irqrestore(&tunables->target_loads_lock, flags);
	return ret;
}

/*
 * If increasing frequencies never map to a lower target load then
 * choose_freq() will find the minimum frequency that does not exceed its
 * target load given the current load.
 */

static unsigned int get_load_over_target(unsigned int tget)
{
		if (tget >= 10000 && tget < 100000)
			tget *= 10;
		else if (tget >= 1000 && tget < 10000)
			tget *= 100;
		else if (tget >= 100 && tget < 1000)
			tget *= 1000;
		else
			return tget;

	return tget;
}

static unsigned int choose_freq(struct interactive_cpu *icpu,
				unsigned int loadadjfreq)
{
	struct cpufreq_policy *policy = icpu->ipolicy->policy;
	struct cpufreq_frequency_table *freq_table = policy->freq_table;
	unsigned int prevfreq, freqmin = 0, freqmax = UINT_MAX, tl,
				 freq = policy->cur, load_over_target, index;

	do {
		prevfreq = freq;
		tl = freq_to_targetload(icpu->ipolicy->tunables, freq);
		//tl = iactive_target_load[policy->cpu];
		/*
		 * Find the lowest frequency where the computed load is less
		 * than or equal to the target load.
		 */
		load_over_target = get_load_over_target(DIV_ROUND_CLOSEST(loadadjfreq, tl));
		clamp_val(load_over_target, 0, DEFAULT_HARD_MAX);
		index = cpufreq_frequency_table_target(policy, load_over_target,
						       CPUFREQ_RELATION_C);
		if (iactive_load_debug) {
			iactive_load_over_target[policy->cpu] = load_over_target;
		}

		freq = freq_table[index].frequency;
		if (freq > prevfreq) {
			/* The previous frequency is too low */
			freqmin = prevfreq;

			if (freq < freqmax)
				continue;

			/* Find highest frequency that is less than freqmax */
			index = cpufreq_frequency_table_target(policy,
					freqmax - 1, CPUFREQ_RELATION_H);

			freq = freq_table[index].frequency;

			if (freq == freqmin) {
				/*
				 * The first frequency below freqmax has already
				 * been found to be too low. freqmax is the
				 * lowest speed we found that is fast enough.
				 */
				freq = freqmax;
				break;
			}
		} else if (freq < prevfreq) {
			/* The previous frequency is high enough. */
			freqmax = prevfreq;

			if (freq > freqmin)
				continue;

			/* Find lowest frequency that is higher than freqmin */
			index = cpufreq_frequency_table_target(policy,
					freqmin + 1, CPUFREQ_RELATION_L);

			freq = freq_table[index].frequency;

			/*
			 * If freqmax is the first frequency above
			 * freqmin then we have already found that
			 * this speed is fast enough.
			 */
			if (freq == freqmax)
				break;
		}

		/* If same frequency chosen as previous then done. */
	} while (freq != prevfreq);

	return freq;
}

static u64 update_load(struct interactive_cpu *icpu, unsigned int cpu)
{
	struct interactive_tunables *tunables = icpu->ipolicy->tunables;
	u64 now_idle, now, active_time, delta_idle, delta_time;

	now_idle = get_cpu_idle_time(cpu, &now);
	delta_idle = (now_idle - icpu->time_in_idle);
	now = ktime_to_us(ktime_get());
	delta_time = (now - icpu->time_in_idle_timestamp);

	if (delta_time > delta_idle) {
		active_time = delta_time - delta_idle;
		icpu->cputime_speedadj += active_time * icpu->ipolicy->policy->cur;
	}

	icpu->time_in_idle = now_idle;
	icpu->time_in_idle_timestamp = now;

	return now;
}

/* Re-evaluate load to see if a frequency change is required or not */
static void eval_target_freq(struct interactive_cpu *icpu)
{
	struct interactive_tunables *tunables = icpu->ipolicy->tunables;
	struct cpufreq_policy *policy = icpu->ipolicy->policy;
	struct cpufreq_frequency_table *freq_table = policy->freq_table;
	u64 cputime_speedadj, now, max_fvtime, delta_time;
	unsigned int new_freq, loadadjfreq, index,
	cpu = smp_processor_id(), cpu_load;
	unsigned long flags;

	spin_lock_irqsave(&icpu->load_lock, flags);
	now = update_load(icpu, cpu);
	delta_time = (now - icpu->cputime_speedadj_timestamp);
	cputime_speedadj = icpu->cputime_speedadj;
	spin_unlock_irqrestore(&icpu->load_lock, flags);

	if (!delta_time)
		return;

	spin_lock_irqsave(&icpu->target_freq_lock, flags);
	do_div(cputime_speedadj, delta_time);
	loadadjfreq = (unsigned int)cputime_speedadj * 100;
	if (iactive_load_debug)
		iactive_raw_loadadjfreq[cpu] = loadadjfreq;

	cpu_load = DIV_ROUND_UP(loadadjfreq, policy->cur);
	if (iactive_load_debug)
		iactive_current_load[cpu] = cpu_load;
	if (cpu_load >= full_speed_load) {
		if (policy->cur < policy->max)
			new_freq = policy->max;
	} else if (cpu_load >= iactive_go_hispeed_load[cpu] && cpu_load < full_speed_load) {
		if (policy->cur < iactive_hispeed_freq[cpu] &&
			iactive_hispeed_freq[cpu] < policy->max)
			new_freq = iactive_hispeed_freq[cpu];
		else
			new_freq = policy->max;
	} else {
		new_freq = choose_freq(icpu, loadadjfreq);
		if (new_freq >= iactive_hispeed_freq[cpu] &&
		    policy->cur < iactive_hispeed_freq[cpu])
			new_freq = iactive_hispeed_freq[cpu];
	}

	if (policy->cur >= iactive_hispeed_freq[cpu] &&
	    new_freq > policy->cur &&
	    now - icpu->pol_hispeed_val_time < freq_to_above_hispeed_delay(tunables, policy->cur))
		goto exit;

	icpu->loc_hispeed_val_time = now;

	index = cpufreq_frequency_table_target(policy, new_freq,
					       CPUFREQ_RELATION_L);
	new_freq = freq_table[index].frequency;
	iactive_choose_freq[cpu] = new_freq;

	/*
	 * Do not scale below floor_freq unless we have been at or above the
	 * floor frequency for the minimum sample time since last validated.
	 */
	max_fvtime = max(icpu->pol_floor_val_time, icpu->loc_floor_val_time);
	if (new_freq < icpu->floor_freq && icpu->target_freq >= policy->cur) {
		if (now - max_fvtime < tunables->min_sample_time)
			goto exit;
	}

	/*
	 * Update the timestamp for checking whether speed has been held at
	 * or above the selected frequency for a minimum of min_sample_time
	 */

	if (new_freq > iactive_hispeed_freq[cpu]) {
		icpu->floor_freq = new_freq;
		if (icpu->target_freq >= policy->cur || new_freq >= policy->cur)
			icpu->loc_floor_val_time = now;
	}

	if (icpu->target_freq == new_freq &&
	    icpu->target_freq == policy->cur)
		goto exit;

	icpu->target_freq = new_freq;
	spin_unlock_irqrestore(&icpu->target_freq_lock, flags);

	spin_lock_irqsave(&speedchange_cpumask_lock, flags);
	cpumask_set_cpu(cpu, &speedchange_cpumask);
	spin_unlock_irqrestore(&speedchange_cpumask_lock, flags);

	wake_up_process(speedchange_task);
	return;

exit:
	spin_unlock_irqrestore(&icpu->target_freq_lock, flags);
}

static void cpufreq_interactive_update(struct interactive_cpu *icpu)
{
	eval_target_freq(icpu);
	slack_timer_resched(icpu, smp_processor_id(), true);
}

static void cpufreq_interactive_idle_end(void)
{
	struct interactive_cpu *icpu = &per_cpu(interactive_cpu,
						smp_processor_id());

	if (!down_read_trylock(&icpu->enable_sem))
		return;

	if (icpu->ipolicy) {
		/*
		 * We haven't sampled load for more than sampling_rate time, do
		 * it right now.
		 */
		if (time_after_eq(jiffies, icpu->next_sample_jiffies))
			cpufreq_interactive_update(icpu);
	}

	up_read(&icpu->enable_sem);
}

static void cpufreq_interactive_get_policy_info(struct cpufreq_policy *policy,
						unsigned int *pmax_freq,
						u64 *phvt, u64 *pfvt)
{
	struct interactive_cpu *icpu;
	u64 hvt = ~0ULL, fvt = 0;
	unsigned int max_freq = 0, i;

	for_each_cpu(i, policy->cpus) {
		icpu = &per_cpu(interactive_cpu, i);

		fvt = max(fvt, icpu->loc_floor_val_time);
		if (icpu->target_freq > max_freq) {
			max_freq = icpu->target_freq;
			hvt = icpu->loc_hispeed_val_time;
		} else if (icpu->target_freq == max_freq) {
			hvt = min(hvt, icpu->loc_hispeed_val_time);
		}
	}

	*pmax_freq = max_freq;
	*phvt = hvt;
	*pfvt = fvt;
}

static void cpufreq_interactive_adjust_cpu(unsigned int cpu,
					   struct cpufreq_policy *policy)
{
	struct interactive_cpu *icpu;
	struct interactive_policy *ipolicy = policy->governor_data;
	u64 hvt, fvt;
	unsigned int max_freq;
	int i;

	cpufreq_interactive_get_policy_info(policy, &max_freq, &hvt, &fvt);

	for_each_cpu(i, policy->cpus) {
		icpu = &per_cpu(interactive_cpu, i);
		icpu->pol_floor_val_time = fvt;
	}

	if (max_freq != policy->cur) {
		mutex_lock(&ilock);
		__cpufreq_driver_target(policy, max_freq, CPUFREQ_RELATION_H);
		mutex_unlock(&ilock);
		for_each_cpu(i, policy->cpus) {
			icpu = &per_cpu(interactive_cpu, i);
			icpu->pol_hispeed_val_time = hvt;
		}
	}
}

static int cpufreq_interactive_speedchange_task(void *data)
{
	unsigned int cpu;
	cpumask_t tmp_mask;
	unsigned long flags;

again:
	set_current_state(TASK_INTERRUPTIBLE);

	if (interactive_suspended) {
		spin_lock_irqsave(&speedchange_cpumask_lock, flags);
		cpumask_clear(&speedchange_cpumask);
		spin_unlock_irqrestore(&speedchange_cpumask_lock, flags);
		schedule();

		if (kthread_should_stop())
			return 0;
	}

	spin_lock_irqsave(&speedchange_cpumask_lock, flags);

	if (cpumask_empty(&speedchange_cpumask)) {
		spin_unlock_irqrestore(&speedchange_cpumask_lock, flags);
		schedule();

		if (kthread_should_stop())
			return 0;

		spin_lock_irqsave(&speedchange_cpumask_lock, flags);
	}

	set_current_state(TASK_RUNNING);
	tmp_mask = speedchange_cpumask;
	cpumask_clear(&speedchange_cpumask);
	spin_unlock_irqrestore(&speedchange_cpumask_lock, flags);

	for_each_cpu(cpu, &tmp_mask) {
		struct interactive_cpu *icpu = &per_cpu(interactive_cpu, cpu);
		struct cpufreq_policy *policy;
		if (cpu_out_of_range(cpu))
			break;
		if (!cpu_online(cpu))
			continue;

		if (unlikely(!down_read_trylock(&icpu->enable_sem)))
			continue;

		if (likely(icpu->ipolicy)) {
			policy = icpu->ipolicy->policy;
			cpufreq_interactive_adjust_cpu(cpu, policy);
		}

		up_read(&icpu->enable_sem);
	}

	goto again;
}

static int cpufreq_interactive_notifier(struct notifier_block *nb,
					unsigned long val, void *data)
{
	struct cpufreq_freqs *freq = data;
	struct interactive_cpu *icpu = &per_cpu(interactive_cpu, freq->cpu);
	unsigned long flags;

	if (val != CPUFREQ_POSTCHANGE)
		return 0;

	if (!down_read_trylock(&icpu->enable_sem))
		return 0;

	if (!icpu->ipolicy) {
		up_read(&icpu->enable_sem);
		return 0;
	}

	spin_lock_irqsave(&icpu->load_lock, flags);
	update_load(icpu, freq->cpu);
	spin_unlock_irqrestore(&icpu->load_lock, flags);

	up_read(&icpu->enable_sem);

	return 0;
}

static struct notifier_block cpufreq_notifier_block = {
	.notifier_call = cpufreq_interactive_notifier,
};

static unsigned int *get_tokenized_data(const char *buf, int *num_tokens)
{
	const char *cp = buf;
	int ntokens = 1, i = 0;
	unsigned int *tokenized_data;
	int err = -EINVAL;

	while ((cp = strpbrk(cp + 1, " :")))
		ntokens++;

	if (!(ntokens & 0x1))
		goto err;

	tokenized_data = kcalloc(ntokens, sizeof(*tokenized_data), GFP_KERNEL);
	if (!tokenized_data) {
		err = -ENOMEM;
		goto err;
	}

	cp = buf;
	while (i < ntokens) {
		if (kstrtouint(cp, 0, &tokenized_data[i++]) < 0)
			goto err_kfree;

		cp = strpbrk(cp, " :");
		if (!cp)
			break;
		cp++;
	}

	if (i != ntokens)
		goto err_kfree;

	*num_tokens = ntokens;
	return tokenized_data;

err_kfree:
	kfree(tokenized_data);
err:
	return ERR_PTR(err);
}

/* Interactive governor sysfs interface */
static struct interactive_tunables *to_tunables(struct gov_attr_set *attr_set)
{
	return container_of(attr_set, struct interactive_tunables, attr_set);
}

#define show_one(file_name, type)					\
static ssize_t show_##file_name(struct gov_attr_set *attr_set, char *buf) \
{									\
	struct interactive_tunables *tunables = to_tunables(attr_set);	\
	return sprintf(buf, type "\n", tunables->file_name);		\
}

static ssize_t show_target_loads(struct gov_attr_set *attr_set, char *buf)
{
	struct interactive_tunables *tunables = to_tunables(attr_set);
	unsigned long flags;
	ssize_t ret = 0;
	int i;

	spin_lock_irqsave(&tunables->target_loads_lock, flags);

	for (i = 0; i < tunables->ntarget_loads; i++)
		ret += sprintf(buf + ret, "%u%s", tunables->target_loads[i],
			       i & 0x1 ? ":" : " ");

	sprintf(buf + ret - 1, "\n");
	spin_unlock_irqrestore(&tunables->target_loads_lock, flags);

	return ret;
}

static ssize_t store_target_loads(struct gov_attr_set *attr_set,
				  const char *buf, size_t count)
{
	struct interactive_tunables *tunables = to_tunables(attr_set);
	unsigned int *new_target_loads;
	unsigned long flags;
	int ntokens;

	new_target_loads = get_tokenized_data(buf, &ntokens);
	if (IS_ERR(new_target_loads))
		return PTR_ERR(new_target_loads);

	spin_lock_irqsave(&tunables->target_loads_lock, flags);
	if (tunables->target_loads != default_target_loads)
		kfree(tunables->target_loads);
	tunables->target_loads = new_target_loads;
	tunables->ntarget_loads = ntokens;
	spin_unlock_irqrestore(&tunables->target_loads_lock, flags);

	return count;
}

static ssize_t show_above_hispeed_delay(struct gov_attr_set *attr_set,
					char *buf)
{
	struct interactive_tunables *tunables = to_tunables(attr_set);
	unsigned long flags;
	ssize_t ret = 0;
	int i;

	spin_lock_irqsave(&tunables->above_hispeed_delay_lock, flags);

	for (i = 0; i < tunables->nabove_hispeed_delay; i++)
		ret += sprintf(buf + ret, "%u%s",
			       tunables->above_hispeed_delay[i],
			       i & 0x1 ? ":" : " ");

	sprintf(buf + ret - 1, "\n");
	spin_unlock_irqrestore(&tunables->above_hispeed_delay_lock, flags);

	return ret;
}

static ssize_t store_above_hispeed_delay(struct gov_attr_set *attr_set,
					 const char *buf, size_t count)
{
	struct interactive_tunables *tunables = to_tunables(attr_set);
	unsigned int *new_above_hispeed_delay = NULL;
	unsigned long flags;
	int ntokens;

	new_above_hispeed_delay = get_tokenized_data(buf, &ntokens);
	if (IS_ERR(new_above_hispeed_delay))
		return PTR_ERR(new_above_hispeed_delay);

	spin_lock_irqsave(&tunables->above_hispeed_delay_lock, flags);
	if (tunables->above_hispeed_delay != default_above_hispeed_delay)
		kfree(tunables->above_hispeed_delay);
	tunables->above_hispeed_delay = new_above_hispeed_delay;
	tunables->nabove_hispeed_delay = ntokens;
	spin_unlock_irqrestore(&tunables->above_hispeed_delay_lock, flags);

	return count;
}

static ssize_t store_hispeed_freq(struct gov_attr_set *attr_set,
				  const char *buf, size_t count)
{
	struct interactive_tunables *tunables = to_tunables(attr_set);
	unsigned long int val;
	int ret;

	ret = kstrtoul(buf, 0, &val);
	if (ret < 0)
		return ret;

	tunables->hispeed_freq = val;

	return count;
}

static ssize_t store_go_hispeed_load(struct gov_attr_set *attr_set,
				     const char *buf, size_t count)
{
	struct interactive_tunables *tunables = to_tunables(attr_set);
	unsigned long val;
	int ret;

	ret = kstrtoul(buf, 0, &val);
	if (ret < 0)
		return ret;

	tunables->go_hispeed_load = val;

	return count;
}

static ssize_t store_min_sample_time(struct gov_attr_set *attr_set,
				     const char *buf, size_t count)
{
	struct interactive_tunables *tunables = to_tunables(attr_set);
	unsigned long val;
	int ret;

	ret = kstrtoul(buf, 0, &val);
	if (ret < 0)
		return ret;

	tunables->min_sample_time = val;

	return count;
}

static ssize_t show_timer_rate(struct gov_attr_set *attr_set, char *buf)
{
	struct interactive_tunables *tunables = to_tunables(attr_set);

	return sprintf(buf, "%lu\n", tunables->sampling_rate);
}

static ssize_t store_timer_rate(struct gov_attr_set *attr_set, const char *buf,
				size_t count)
{
	struct interactive_tunables *tunables = to_tunables(attr_set);
	unsigned long val, val_round;
	int ret;

	ret = kstrtoul(buf, 0, &val);
	if (ret < 0)
		return ret;

	val_round = jiffies_to_usecs(usecs_to_jiffies(val));
	if (val != val_round)
		pr_warn("timer_rate not aligned to jiffy. Rounded up to %lu\n",
			val_round);

	tunables->sampling_rate = val_round;

	return count;
}

static ssize_t store_timer_slack(struct gov_attr_set *attr_set, const char *buf,
				 size_t count)
{
	struct interactive_tunables *tunables = to_tunables(attr_set);
	unsigned long val;
	int ret;

	ret = kstrtol(buf, 10, &val);
	if (ret < 0)
		return ret;

	tunables->timer_slack = val;
	update_slack_delay(tunables);

	return count;
}

show_one(hispeed_freq, "%u");
show_one(go_hispeed_load, "%lu");
show_one(min_sample_time, "%lu");
show_one(timer_slack, "%lu");

gov_attr_rw(target_loads);
gov_attr_rw(above_hispeed_delay);
gov_attr_rw(hispeed_freq);
gov_attr_rw(go_hispeed_load);
gov_attr_rw(min_sample_time);
gov_attr_rw(timer_rate);
gov_attr_rw(timer_slack);

static struct attribute *interactive_attributes[] = {
	&target_loads.attr,
	&above_hispeed_delay.attr,
	&hispeed_freq.attr,
	&go_hispeed_load.attr,
	&min_sample_time.attr,
	&timer_rate.attr,
	&timer_slack.attr,
	NULL
};

static struct kobj_type interactive_tunables_ktype = {
	.default_attrs = interactive_attributes,
	.sysfs_ops = &governor_sysfs_ops,
};

static int cpufreq_interactive_idle_notifier(struct notifier_block *nb,
					     unsigned long val, void *data)
{
	if (val == CPU_PM_EXIT && !interactive_suspended)
		cpufreq_interactive_idle_end();

	return 0;
}

static struct notifier_block cpufreq_interactive_idle_nb = {
	.notifier_call = cpufreq_interactive_idle_notifier,
};

/* Interactive Governor callbacks */
struct interactive_governor {
	struct cpufreq_governor gov;
	unsigned int usage_count;
};

static struct interactive_governor interactive_gov;

#define CPU_FREQ_GOV_INTERACTIVE	(&interactive_gov.gov)

static void irq_work(struct irq_work *irq_work)
{
	struct interactive_cpu *icpu = container_of(irq_work, struct
						    interactive_cpu, irq_work);

	cpufreq_interactive_update(icpu);
	icpu->work_in_progress = false;
}

static void update_util_handler(struct update_util_data *data, u64 time,
				unsigned int flags)
{
	struct interactive_cpu *icpu = container_of(data,
					struct interactive_cpu, update_util);
	struct interactive_policy *ipolicy = icpu->ipolicy;
	struct interactive_tunables *tunables = ipolicy->tunables;
	u64 delta_ns;

	/*
	 * The irq-work may not be allowed to be queued up right now.
	 * Possible reasons:
	 * - Work has already been queued up or is in progress.
	 * - It is too early (too little time from the previous sample).
	 */
	if (icpu->work_in_progress)
		return;

	delta_ns = time - icpu->last_sample_time;
	if ((s64)delta_ns < tunables->sampling_rate * NSEC_PER_USEC)
		return;

	icpu->last_sample_time = time;
	icpu->next_sample_jiffies = usecs_to_jiffies(tunables->sampling_rate) +
				    jiffies;

	icpu->work_in_progress = true;
	irq_work_queue(&icpu->irq_work);
}

static void gov_set_update_util(struct interactive_policy *ipolicy)
{
	struct cpufreq_policy *policy = ipolicy->policy;
	struct interactive_cpu *icpu;
	unsigned int cpu;

	for_each_cpu(cpu, policy->cpus) {
		icpu = &per_cpu(interactive_cpu, cpu);
		icpu->last_sample_time = 0;
		icpu->next_sample_jiffies = 0;
		cpufreq_add_update_util_hook(cpu, &icpu->update_util,
					     update_util_handler);
	}
}

static inline void gov_clear_update_util(struct cpufreq_policy *policy)
{
	int i;

	for_each_cpu(i, policy->cpus)
		cpufreq_remove_update_util_hook(i);

	synchronize_sched();
}

static void icpu_cancel_work(struct interactive_cpu *icpu)
{
	irq_work_sync(&icpu->irq_work);
	icpu->work_in_progress = false;
	icpu->timer_is_busy = true;
	WARN_ON_ONCE(del_timer(&icpu->slack_timer) < 0);
	icpu->timer_is_busy = false;
}

static struct interactive_policy *
interactive_policy_alloc(struct cpufreq_policy *policy)
{
	struct interactive_policy *ipolicy;

	ipolicy = kzalloc(sizeof(*ipolicy), GFP_KERNEL);
	if (!ipolicy)
		return NULL;

	ipolicy->policy = policy;

	return ipolicy;
}

static void interactive_policy_free(struct interactive_policy *ipolicy)
{
	kfree(ipolicy);
}

static struct interactive_tunables *
interactive_tunables_alloc(struct interactive_policy *ipolicy)
{
	struct interactive_tunables *tunables;

	tunables = kzalloc(sizeof(*tunables), GFP_KERNEL);
	if (!tunables)
		return NULL;

	gov_attr_set_init(&tunables->attr_set, &ipolicy->tunables_hook);

	ipolicy->tunables = tunables;

	return tunables;
}

static void interactive_tunables_free(struct interactive_tunables *tunables)
{
	kfree(tunables);
}

static int interactive_suspend_handler(struct notifier_block *nb,
				unsigned long val, void *data)
{
	switch (val) {
	case PM_PROACTIVE_RESUME:
		interactive_suspended = 0;
		break;
	case PM_PROACTIVE_SUSPEND:
		interactive_suspended = 1;
		break;
	default:
		return NOTIFY_DONE;
	}
	return NOTIFY_OK;
}

static struct notifier_block iactive_pm_notifier = {
	.notifier_call = interactive_suspend_handler,
};

static int interactive_kthread_create(void)
{
	//struct sched_param param = { .sched_priority =  MAX_USER_RT_PRIO / 2 };
	speedchange_task = kthread_create(cpufreq_interactive_speedchange_task,
					  NULL, "cfinteractive");
	if (IS_ERR(speedchange_task))
		return PTR_ERR(speedchange_task);

	//sched_setscheduler_nocheck(speedchange_task, SCHED_FIFO, &param);
	get_task_struct(speedchange_task);

	/* wake up so the thread does not look hung to the freezer */
	wake_up_process(speedchange_task);
	return 0;
}

static void interactive_kthread_destroy(void)
{
	kthread_stop(speedchange_task);
	put_task_struct(speedchange_task);
}

int cpufreq_interactive_init(struct cpufreq_policy *policy)
{
	struct interactive_policy *ipolicy;
	struct interactive_tunables *tunables;
	int ret;

	/* State should be equivalent to EXIT */
	if (policy->governor_data)
		return -EBUSY;

	ipolicy = interactive_policy_alloc(policy);
	if (!ipolicy)
		return -ENOMEM;

	mutex_lock(&tunables_lock);

	tunables = interactive_tunables_alloc(ipolicy);
	if (!tunables) {
		ret = -ENOMEM;
		goto free_int_policy;
	}

	tunables->hispeed_freq = iactive_hispeed_freq[policy->cpu];
	tunables->above_hispeed_delay = default_above_hispeed_delay;
	tunables->nabove_hispeed_delay =
		ARRAY_SIZE(default_above_hispeed_delay);
	tunables->go_hispeed_load = iactive_go_hispeed_load[policy->cpu];
	tunables->target_loads = &iactive_target_load[policy->cpu];
	tunables->ntarget_loads = ARRAY_SIZE(default_target_loads);
	tunables->min_sample_time = DEFAULT_MIN_SAMPLE_TIME;
	tunables->sampling_rate = DEFAULT_SAMPLING_RATE;
	tunables->timer_slack = DEFAULT_TIMER_SLACK;
	update_slack_delay(tunables);

	spin_lock_init(&tunables->target_loads_lock);
	spin_lock_init(&tunables->above_hispeed_delay_lock);

	policy->governor_data = ipolicy;

	ret = kobject_init_and_add(&tunables->attr_set.kobj,
				   &interactive_tunables_ktype,
				   get_governor_parent_kobj(policy), "%s",
				   interactive_gov.gov.name);
	if (ret)
		goto fail;

	/* One time initialization for governor */
	if (!interactive_gov.usage_count++) {
		ret = interactive_kthread_create();
		if (ret)
			goto fail;
		cpufreq_register_notifier(&cpufreq_notifier_block,
					  CPUFREQ_TRANSITION_NOTIFIER);
		register_pm_notifier(&iactive_pm_notifier);
		cpu_pm_register_notifier(&cpufreq_interactive_idle_nb);
	}

 out:
	mutex_unlock(&tunables_lock);
	return 0;

 fail:
	policy->governor_data = NULL;
	interactive_tunables_free(tunables);

 free_int_policy:
	mutex_unlock(&tunables_lock);

	interactive_policy_free(ipolicy);
	pr_err("governor initialization failed (%d)\n", ret);

	return ret;
}

void cpufreq_interactive_exit(struct cpufreq_policy *policy)
{
	struct interactive_policy *ipolicy = policy->governor_data;
	struct interactive_tunables *tunables = ipolicy->tunables;
	unsigned int count;

	mutex_lock(&tunables_lock);

	/* Last policy using the governor ? */
	if (!--interactive_gov.usage_count) {
		unregister_pm_notifier(&iactive_pm_notifier);
		cpu_pm_unregister_notifier(&cpufreq_interactive_idle_nb);
		cpufreq_unregister_notifier(&cpufreq_notifier_block,
					    CPUFREQ_TRANSITION_NOTIFIER);
		interactive_kthread_destroy();
	}

	count = gov_attr_set_put(&tunables->attr_set, &ipolicy->tunables_hook);
	policy->governor_data = NULL;
	if (!count)
		interactive_tunables_free(tunables);

	mutex_unlock(&tunables_lock);
	interactive_policy_free(ipolicy);
}

int cpufreq_interactive_start(struct cpufreq_policy *policy)
{
	struct interactive_policy *ipolicy = policy->governor_data;
	struct interactive_cpu *icpu;
	unsigned int cpu;

	for_each_cpu(cpu, policy->cpus) {
		icpu = &per_cpu(interactive_cpu, cpu);

		icpu->target_freq = policy->cur;
		icpu->floor_freq = icpu->target_freq;
		icpu->pol_floor_val_time = ktime_to_us(ktime_get());
		icpu->loc_floor_val_time = icpu->pol_floor_val_time;
		icpu->pol_hispeed_val_time = icpu->pol_floor_val_time;
		icpu->loc_hispeed_val_time = icpu->pol_floor_val_time;
		down_write(&icpu->enable_sem);
		icpu->ipolicy = ipolicy;
		up_write(&icpu->enable_sem);

		slack_timer_resched(icpu, cpu, false);
	}

	gov_set_update_util(ipolicy);
	return 0;
}

void cpufreq_interactive_stop(struct cpufreq_policy *policy)
{
	struct interactive_policy *ipolicy = policy->governor_data;
	struct interactive_cpu *icpu;
	unsigned int cpu;

	gov_clear_update_util(ipolicy->policy);
	for_each_cpu(cpu, policy->cpus) {
		icpu = &per_cpu(interactive_cpu, cpu);
		icpu_cancel_work(icpu);
		down_write(&icpu->enable_sem);
		icpu->ipolicy = NULL;
		up_write(&icpu->enable_sem);
	}
}

void cpufreq_interactive_limits(struct cpufreq_policy *policy)
{
	struct interactive_policy *ipolicy = policy->governor_data;
	struct interactive_cpu *icpu;
	unsigned int cpu;
	unsigned long flags;

	mutex_lock(&ilock);
	cpufreq_policy_apply_limits(policy);
	mutex_unlock(&ilock);
	for_each_cpu(cpu, policy->cpus) {
		icpu = &per_cpu(interactive_cpu, cpu);
		spin_lock_irqsave(&icpu->target_freq_lock, flags);
		if (policy->max < icpu->target_freq)
			icpu->target_freq = policy->max;
		else if (policy->min > icpu->target_freq)
			icpu->target_freq = policy->min;
		spin_unlock_irqrestore(&icpu->target_freq_lock, flags);
	}
}

static struct interactive_governor interactive_gov = {
	.gov = {
		.name			= "interactive",
		.owner			= THIS_MODULE,
		.dynamic_switching = true,
		.init			= cpufreq_interactive_init,
		.exit			= cpufreq_interactive_exit,
		.start			= cpufreq_interactive_start,
		.stop			= cpufreq_interactive_stop,
		.limits			= cpufreq_interactive_limits,
	}
};

static void cpufreq_interactive_nop_timer(struct timer_list *unused)
{
	/*
	 * The purpose of slack-timer is to wake up the CPU from IDLE, in order
	 * to decrease its frequency if it is not set to minimum already.
	 *
	 * This is important for platforms where CPU with higher frequencies
	 * consume higher power even at IDLE.
	 */
}

static int __init cpufreq_interactive_gov_init(void)
{
	struct interactive_cpu *icpu;
	unsigned int cpu;

	for_each_possible_cpu(cpu) {
		icpu = &per_cpu(interactive_cpu, cpu);

		init_irq_work(&icpu->irq_work, irq_work);
		spin_lock_init(&icpu->load_lock);
		spin_lock_init(&icpu->target_freq_lock);
		init_rwsem(&icpu->enable_sem);

		/* Initialize per-cpu slack-timer */
		timer_setup(&icpu->slack_timer, cpufreq_interactive_nop_timer,
			    TIMER_PINNED);
	}

	spin_lock_init(&speedchange_cpumask_lock);
	return cpufreq_register_governor(CPU_FREQ_GOV_INTERACTIVE);
}

struct cpufreq_governor *mx_gov_interactive(void)
{
	return CPU_FREQ_GOV_INTERACTIVE;
}

#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_INTERACTIVE
struct cpufreq_governor *cpufreq_default_governor(void)
{
	return CPU_FREQ_GOV_INTERACTIVE;
}

fs_initcall(cpufreq_interactive_gov_init);
#else
module_init(cpufreq_interactive_gov_init);
#endif

static void __exit cpufreq_interactive_gov_exit(void)
{
	cpufreq_unregister_governor(CPU_FREQ_GOV_INTERACTIVE);
}
module_exit(cpufreq_interactive_gov_exit);

MODULE_AUTHOR("Mike Chan <mike@android.com>");
MODULE_DESCRIPTION("'cpufreq_interactive' - A dynamic cpufreq governor for Latency sensitive workloads");
MODULE_LICENSE("GPL");
