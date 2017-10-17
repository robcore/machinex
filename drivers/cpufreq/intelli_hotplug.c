/*
 * Intelli Hotplug Driver
 *
 * Copyright (c) 2013-2014, Paul Reioux <reioux@gmail.com>
 * Copyright (c) 2010-2014, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/workqueue.h>
#include <linux/cpu.h>
#include <linux/sched.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/kobject.h>
#include <linux/cpufreq.h>
#include <linux/delay.h>
#include <linux/display_state.h>
#include <linux/powersuspend.h>
#include <linux/spinlock.h>
#include <linux/sysfs_helpers.h>
#include "../../arch/arm/mach-msm/acpuclock.h"

#define INTELLI_PLUG			"intelli_plug"
#define INTELLI_PLUG_MAJOR_VERSION	15
#define INTELLI_PLUG_MINOR_VERSION	7

#define DEFAULT_MAX_CPUS_ONLINE NR_CPUS
#define DEFAULT_MIN_CPUS_ONLINE 2
#define INTELLI_MS(x) ((((x) * MSEC_PER_SEC) / MSEC_PER_SEC))
#define DEFAULT_SAMPLING_RATE INTELLI_MS(250UL)
#define INPUT_INTERVAL INTELLI_MS(500UL)
#define BOOST_LOCK_DUR INTELLI_MS(500UL)
#define DEFAULT_NR_CPUS_BOOSTED (DEFAULT_MAX_CPUS_ONLINE)
#define DEFAULT_NR_FSHIFT (DEFAULT_MAX_CPUS_ONLINE - 1)
#define DEFAULT_DOWN_LOCK_DUR INTELLI_MS(500UL)
#define DEFAULT_HYSTERESIS (NR_CPUS << 1)

/*#define CAPACITY_RESERVE (50)
#define THREAD_CAPACITY (339 - CAPACITY_RESERVE)

#define CPU_NR_THRESHOLD ((THREAD_CAPACITY << 1) | (THREAD_CAPACITY >> 1))
*/

#define HIGH_LOAD_FREQ 1566000
#define MAX_LOAD_FREQ 1890000
#define THREAD_CAPACITY 289
#define CPU_NR_THRESHOLD 722
#define MULT_FACTOR DEFAULT_MAX_CPUS_ONLINE
#define INTELLIPLIER (THREAD_CAPACITY * MULT_FACTOR)
#define DIV_FACTOR 100000
#define INTELLIPLY(x) (x * INTELLIPLIER)
#define INTELLIDIV(x) (DIV_ROUND_UP((x * INTELLIPLIER), DIV_FACTOR))

static unsigned int high_load_threshold = HIGH_LOAD_FREQ;
static int max_load_freq = MAX_LOAD_FREQ;
static ktime_t last_boost_time;
static ktime_t last_input;

static struct delayed_work intelli_plug_work;
static struct delayed_work up_down_work;
static struct workqueue_struct *intelliplug_wq;
static struct workqueue_struct *updown_wq;
static void refresh_cpus(void);

struct ip_cpu_info {
	unsigned long cpu_nr_running;
};
static DEFINE_PER_CPU(struct ip_cpu_info, ip_info);

/* HotPlug Driver controls */
static unsigned int intelli_plug_active = 0;
static DEFINE_RWLOCK(ips_lock);

static unsigned int cpus_boosted = DEFAULT_NR_CPUS_BOOSTED;
static unsigned int min_cpus_online = DEFAULT_MIN_CPUS_ONLINE;
static unsigned int max_cpus_online = DEFAULT_MAX_CPUS_ONLINE;
static unsigned long full_mode_profile = 0;
static unsigned int cpu_nr_run_threshold = CPU_NR_THRESHOLD;
static unsigned int online_cpus;

/* HotPlug Driver Tuning */
static int target_cpus = DEFAULT_MIN_CPUS_ONLINE;
static unsigned long min_input_interval = INPUT_INTERVAL;
static unsigned long boost_lock_duration = BOOST_LOCK_DUR;
static unsigned long def_sampling_ms = DEFAULT_SAMPLING_RATE;
static unsigned int nr_fshift = DEFAULT_NR_FSHIFT;
static unsigned long nr_run_hysteresis = DEFAULT_HYSTERESIS;
static unsigned int debug_intelli_plug = 0;

enum {
	INTELLI_AWAKE = 0,
	INTELLI_SUSPENDED = 1,
};

struct mutex intellisleep_mutex;
unsigned int intelli_suspended;

struct wake_lock ipwlock;

#define dprintk(msg...)		\
do {				\
	if (debug_intelli_plug)		\
		pr_info(msg);	\
} while (0)

static unsigned long nr_run_thresholds_balance[] = {
	INTELLIDIV(791),
	INTELLIDIV(1237),
	INTELLIDIV(1444),
	ULONG_MAX
};

static unsigned long nr_run_thresholds_machinex[] = {
	INTELLIDIV(585),
	INTELLIDIV(825),
	INTELLIDIV(1100),
	ULONG_MAX
};

static unsigned long nr_run_thresholds_performance[] = {
	INTELLIDIV(375),
	INTELLIDIV(625),
	INTELLIDIV(875),
	ULONG_MAX
};

static unsigned long nr_run_thresholds_conservative[] = {
	INTELLIDIV(875),
	INTELLIDIV(1625),
	INTELLIDIV(2125),
	ULONG_MAX
};

static unsigned long nr_run_thresholds_disable[] = {
	0,  0,  0,  ULONG_MAX
};

static unsigned long nr_run_thresholds_tri[] = {
	INTELLIDIV(625),
	INTELLIDIV(875),
	ULONG_MAX
};

static unsigned long nr_run_thresholds_eco[] = {
	INTELLIDIV(380),
	ULONG_MAX
};

static unsigned long nr_run_thresholds_strict[] = {
	ULONG_MAX
};

static unsigned long *nr_run_profiles[] = {
	nr_run_thresholds_balance,
	nr_run_thresholds_machinex,
	nr_run_thresholds_performance,
	nr_run_thresholds_conservative,
	nr_run_thresholds_disable,
	nr_run_thresholds_tri,
	nr_run_thresholds_eco,
	nr_run_thresholds_strict
};

static unsigned int intelliread(void)
{
	unsigned int ret;
	unsigned long flags;

	read_lock_irqsave(&ips_lock, flags);
	ret = intelli_plug_active;
	read_unlock_irqrestore(&ips_lock, flags);

	return ret;
}
static void intelli_lock(int lock)
{
	unsigned long flags = 0;

	if (lock)
		write_lock_irqsave(&ips_lock, flags);
	else
		write_unlock_irqrestore(&ips_lock, flags);
}

static void _intelliget(void)
{
	intelli_plug_active = 1;
}

static void intelliget(void)
{
	intelli_lock(1);
	_intelliget();
	intelli_lock(0);
}

static void _intelliput(void)
{
	intelli_plug_active = 0;
}

static void intelliput(void)
{
	intelli_lock(1);
	_intelliput();
	intelli_lock(0);
}

static bool intellinit;

bool intelli_init(void)
{
	return intellinit;
}

static unsigned int nr_run_last;
static unsigned long down_lock_dur = DEFAULT_DOWN_LOCK_DUR;

struct down_lock {
	bool locked;
	struct delayed_work lock_rem;
};
static DEFINE_PER_CPU(struct down_lock, lock_info);

static void remove_down_lock(struct work_struct *work)
{
	struct down_lock *dl = container_of(work, struct down_lock,
					    lock_rem.work);
	if (likely(dl->locked))
		dl->locked = false;
}

static void rm_down_lock(unsigned int cpu, unsigned long duration)
{
	struct down_lock *dl = &per_cpu(lock_info, cpu);

	if (!duration) {
		if (dl->locked)
			dl->locked = false;
	} else
		mod_delayed_work_on(cpu, intelliplug_wq, &dl->lock_rem,
		      duration);
}

static void apply_down_lock(unsigned int cpu)
{
	struct down_lock *dl = &per_cpu(lock_info, cpu);

	if (!is_display_on())
		return;

	if (likely(!dl->locked)) {
		dl->locked = true;
		rm_down_lock(cpu, down_lock_dur);
	}
}

static void force_down_lock(unsigned int cpu)
{
	struct down_lock *dl = &per_cpu(lock_info, cpu);

	dl->locked = true;
	rm_down_lock(cpu, down_lock_dur);
}
	
static int check_down_lock(unsigned int cpu)
{
	struct down_lock *dl = &per_cpu(lock_info, cpu);

	return dl->locked;
}

static void report_current_cpus(void)
{
	WRITE_ONCE(online_cpus, num_online_cpus());
}

#define INTELLILOAD(x) ((x) >> FSHIFT)

static int measure_freqs(void)
{
	unsigned int cpu, freq_load;

	freq_load = 0;
	get_online_cpus();
	for_each_online_cpu(cpu) {
		if (acpuclk_get_rate(cpu) >=
			high_load_threshold)
			freq_load += 1;
		else
			continue; 
	}
	put_online_cpus();
	return freq_load;
}

static unsigned long *get_current_profile(void)
{
	unsigned long *retrieved_profile;

	if (max_cpus_online == DEFAULT_MAX_CPUS_ONLINE)
		retrieved_profile = nr_run_profiles[full_mode_profile];
	else if (max_cpus_online == 3)
		retrieved_profile = nr_run_profiles[5];
	else if (max_cpus_online == 2)
		retrieved_profile = nr_run_profiles[6];
	else if (max_cpus_online == 1)
		retrieved_profile = nr_run_profiles[7];
	return retrieved_profile;
}
static unsigned int calculate_thread_stats(void)
{
	unsigned int nr_cpus;
	unsigned int intellicounter = 1;
	unsigned long *current_profile = get_current_profile();

	for (nr_cpus = min_cpus_online; nr_cpus < max_cpus_online; nr_cpus++) {
		unsigned long nr_threshold, bigshift;

		nr_threshold = current_profile[nr_cpus - 1];
		nr_fshift = num_offline_cpus() + 1;
		nr_run_hysteresis = DIV_ROUND_CLOSEST((max_cpus_online << 2), num_online_cpus());
		bigshift = FSHIFT - nr_fshift;

		if (nr_cpus >= nr_run_last)
			nr_threshold += nr_run_hysteresis;

		nr_threshold <<= bigshift;

		if (avg_nr_running() <= nr_threshold)
			break;
	}

	if (num_offline_cpus() > 0)
		nr_cpus += measure_freqs();
	else if (measure_freqs() > 0) {
		if (!intellicounter) {
			nr_cpus += 1;
			intellicounter++;
		} else {
			intellicounter = 0;
		}
	}

	sanitize_min_max(nr_cpus, min_cpus_online, max_cpus_online);
	nr_run_last = nr_cpus;
	return nr_cpus;
}

static void update_per_cpu_stat(void)
{
	unsigned int cpu;
	struct ip_cpu_info *l_ip_info;

	for_each_online_cpu(cpu) {
		l_ip_info = &per_cpu(ip_info, cpu);
		l_ip_info->cpu_nr_running = avg_cpu_nr_running(cpu);
	}
}

static void do_override(void)
{
	unsigned int cpu;
	int ret;

	if (!prometheus_override)
		return;

	if (!hotplug_ready)
		return;

	cpu = smp_processor_id();

	for_each_nonboot_offline_cpu(cpu) {
		if (cpu_out_of_range_hp(cpu))
			break;
		if (cpu_online(cpu) ||
			!is_cpu_allowed(cpu) ||
			thermal_core_controlled(cpu))
			continue;
		ret = cpu_up(cpu);
		if (ret)
			pr_debug("Intelliplug - Unable to bring up cpu %u!\n", cpu);
	}
}

static atomic_t from_boost = ATOMIC_INIT(0);
static void cpu_up_down_work(struct work_struct *work)
{
	unsigned int cpu;
	int primary;
	long l_nr_threshold;
	int target;
	struct ip_cpu_info *l_ip_info;
	ktime_t delta;

	if (unlikely(prometheus_override)) {
		return;
	}

	mutex_lock(&intellisleep_mutex);
	if (intelli_suspended) {
		mutex_unlock(&intellisleep_mutex);
		return;
	}
	mutex_unlock(&intellisleep_mutex);

	if (!hotplug_ready)
		goto reschedule;

	cpu = smp_processor_id();
	target = READ_ONCE(target_cpus);
	sanitize_min_max(target, min_cpus_online, max_cpus_online);

	report_current_cpus();
	if (target == online_cpus)
		goto reschedule;

	if (online_cpus > min_cpus_online && target < online_cpus) {
		if ((atomic_read(&from_boost) == 1) && 
			online_cpus == cpus_boosted) {
			delta = ktime_sub(ktime_get(), last_input);
			if (ktime_compare(delta, ms_to_ktime(boost_lock_duration)) <= 0)
				goto reschedule;
		}
		update_per_cpu_stat();
		for_each_nonboot_online_cpu(cpu) {
			if (cpu_out_of_range_hp(cpu))
				break;
			if (cpu_is_offline(cpu) ||
				thermal_core_controlled(cpu) ||
				check_down_lock(cpu))
				continue;
			l_nr_threshold =
				(cpu_nr_run_threshold << 1) / num_online_cpus();
			l_ip_info = &per_cpu(ip_info, cpu);
			if (l_ip_info->cpu_nr_running < l_nr_threshold) {
				cpu_down(cpu);
			}
			if (num_online_cpus() == target)
				break;
		}
	} else if (online_cpus < max_cpus_online && target > online_cpus) {
		for_each_nonboot_offline_cpu(cpu) {
			if (cpu_out_of_range_hp(cpu))
				break;
			if (cpu_online(cpu) ||
				!is_cpu_allowed(cpu) ||
				thermal_core_controlled(cpu))
				continue;
			if (!cpu_up(cpu))
				apply_down_lock(cpu);
			if (num_online_cpus() == target)
				break;
		}
	}
reschedule:
		mod_delayed_work_on(0, intelliplug_wq, &intelli_plug_work,
					def_sampling_ms);
}

static void intelli_plug_work_fn(struct work_struct *work)
{
	if (unlikely(prometheus_override))
		return;

	mutex_lock(&intellisleep_mutex);
	if (intelli_suspended) {
		mutex_unlock(&intellisleep_mutex);
		return;
	}
	mutex_unlock(&intellisleep_mutex);

	if (intelliread()) {
		atomic_set(&from_boost, 0);
		WRITE_ONCE(target_cpus, calculate_thread_stats());
		mod_delayed_work_on(0, updown_wq, &up_down_work, 0);
	}
}

void intelli_boost(void)
{
	ktime_t delta;

	if (!intelliread() || !is_display_on() || unlikely(intellinit) ||
		unlikely(prometheus_override))
		return;

	last_input = ktime_get();
	delta = ktime_sub(last_input, last_boost_time);
	
	if ((ktime_compare(delta, ms_to_ktime(min_input_interval))  < 0) ||
		num_online_cpus() >= cpus_boosted ||
	    cpus_boosted <= min_cpus_online)
		return;

	atomic_set(&from_boost, 1);
	sanitize_min_max(cpus_boosted, min_cpus_online, max_cpus_online);
	WRITE_ONCE(target_cpus, cpus_boosted);
	mod_delayed_work_on(0, updown_wq, &up_down_work, 0);
	last_boost_time = ktime_get();
}

void intelli_suspend_booster(void)
{
	if (!intelliread() || unlikely(intellinit) || (unlikely(!prometheus_override)))
		return;
	do_override();
}

static void cycle_cpus(void)
{
	unsigned int cpu;

	intellinit = true;
	for_each_nonboot_online_cpu(cpu) {
		if (cpu_out_of_range_hp(cpu))
			break;
		if (!cpu_online(cpu))
			continue;
		rm_down_lock(cpu, 0);
		cpu_down(cpu);
	}
	for_each_nonboot_offline_cpu(cpu) {
		if (cpu_out_of_range_hp(cpu))
			break;
		if (!is_cpu_allowed(cpu) ||
			thermal_core_controlled(cpu))
			continue;
		if (!cpu_up(cpu))
			force_down_lock(cpu);
	}
	intellinit = false;
	report_current_cpus();
	mod_delayed_work_on(0, updown_wq, &up_down_work, 0);
	wake_unlock(&ipwlock);
	pr_info("Intelliplug Start: Cycle Cpus Complete\n");
}

static void recycle_cpus(void)
{
	unsigned int cpu;

	wake_lock(&ipwlock);
	intellinit = true;

	for_each_nonboot_online_cpu(cpu) {
		if (!cpu_online(cpu))
			continue;
		if (check_down_lock(cpu))
			rm_down_lock(cpu, 0);
		cpu_down(cpu);
	}

	for_each_nonboot_offline_cpu(cpu) {
		if (!is_cpu_allowed(cpu) ||
			thermal_core_controlled(cpu))
			continue;
		if (!cpu_up(cpu)) {
			if (!check_down_lock(cpu))
				apply_down_lock(cpu);
		}
	}
	intellinit = false;
	mod_delayed_work_on(0, intelliplug_wq, &intelli_plug_work, def_sampling_ms);
	wake_unlock(&ipwlock);
}

static void resume_worker(struct work_struct *work)
{
	recycle_cpus();
}

static DECLARE_DELAYED_WORK(delayed_recycle, resume_worker);

static void intelli_suspend(struct power_suspend *h)
{
	struct down_lock *dl;
	unsigned int cpu;

	if (!intelliread())
		return;

	cancel_delayed_work(&intelli_plug_work);
	mutex_lock(&intellisleep_mutex);
	if (intelli_suspended == INTELLI_AWAKE)
		intelli_suspended = INTELLI_SUSPENDED;
	mutex_unlock(&intellisleep_mutex);

	for_each_possible_cpu(cpu) {
		dl = &per_cpu(lock_info, cpu);
		if (check_down_lock(cpu))
			rm_down_lock(cpu, 0);
	}
}

static void intelli_resume(struct power_suspend * h)
{
	unsigned int cpu;

	if (!intelliread())
		return;

	if (intelli_suspended == INTELLI_SUSPENDED);
		intelli_suspended = INTELLI_AWAKE;

	if (!limit_screen_on_cpus)
		recycle_cpus();
	else
		schedule_delayed_work(&delayed_recycle, 1000);
}

static struct power_suspend intelli_suspend_data =
{
	.suspend = intelli_suspend,
	.resume = intelli_resume,
};

static int intelliplug_cpu_callback(struct notifier_block *nfb,
					    unsigned long action, void *hcpu)
{
	unsigned int cpu = (unsigned long)hcpu;
	/* Fail hotplug until this driver can get CPU clocks, or screen off */
	if (!hotplug_ready || !is_display_on())
		return NOTIFY_OK;

	switch (action & ~CPU_TASKS_FROZEN) {
	case CPU_ONLINE_PREPARE:
	case CPU_ONLINE:
		if (!is_cpu_allowed(cpu) &&
			check_down_lock(cpu))
			rm_down_lock(cpu, 0);
		break;
	case CPU_DEAD:
		if (unlikely(check_down_lock(cpu)))
			rm_down_lock(cpu, 0);
	default:
		break;
	}

	return NOTIFY_OK;
}

static struct notifier_block intelliplug_cpu_notifier = {
	.notifier_call = intelliplug_cpu_callback,
};

static int intelli_plug_start(void)
{
	unsigned int cpu;
	int ret = 0;
	struct down_lock *dl;

	wake_lock(&ipwlock);

	intelliplug_wq = create_singlethread_workqueue("intelliplug");

	if (!intelliplug_wq) {
		pr_err("%s: Failed to allocate hotplug workqueue\n",
		       INTELLI_PLUG);
		ret = -ENOMEM;
		goto err_out;
	}

	updown_wq = create_singlethread_workqueue("updown");

	if (!updown_wq) {
		pr_err("%s: Failed to allocate hotplug workqueue\n",
		       INTELLI_PLUG);
		ret = -ENOMEM;
		goto err_dev;
	}

	mutex_init(&(intellisleep_mutex));
	intelli_suspended = INTELLI_AWAKE;

	for_each_possible_cpu(cpu) {
		dl = &per_cpu(lock_info, cpu);
		INIT_DELAYED_WORK(&dl->lock_rem, remove_down_lock);
	}

	INIT_DELAYED_WORK(&intelli_plug_work, intelli_plug_work_fn);
	INIT_DELAYED_WORK(&up_down_work, cpu_up_down_work);

	register_power_suspend(&intelli_suspend_data);

	register_hotcpu_notifier(&intelliplug_cpu_notifier);

	cycle_cpus();

	return ret;
err_dev:
	destroy_workqueue(intelliplug_wq);
err_out:
	if (intelliread())
		intelliput();

	wake_unlock(&ipwlock);
	return ret;
}

static void intelli_plug_stop(void)
{
	unsigned int cpu;
	struct down_lock *dl;

	cancel_delayed_work(&up_down_work);
	for_each_possible_cpu(cpu) {
		dl = &per_cpu(lock_info, cpu);
		cancel_delayed_work_sync(&dl->lock_rem);
		if (check_down_lock(cpu))
			rm_down_lock(cpu, 0);
	}
	cancel_delayed_work(&intelli_plug_work);
	unregister_power_suspend(&intelli_suspend_data);
	destroy_workqueue(updown_wq);
	destroy_workqueue(intelliplug_wq);
	unregister_hotcpu_notifier(&intelliplug_cpu_notifier);
	mutex_destroy(&(intellisleep_mutex));
}

static void intelli_plug_active_eval_fn(unsigned int status)
{
	int ret = 0;
	if (status) {
		ret = intelli_plug_start();
		if (!ret)
			intelliget();
		else
			status = 0;
	} else
		intelli_plug_stop();

	if (!status)
		intelliput();
}

#define show_one(object)				\
static ssize_t show_##object					\
(struct kobject *kobj, struct kobj_attribute *attr, char *buf)	\
{								\
	return sprintf(buf, "%u\n", object);			\
}

#define show_long(object)				\
static ssize_t show_##object					\
(struct kobject *kobj, struct kobj_attribute *attr, char *buf)	\
{								\
	return sprintf(buf, "%lu\n", object);			\
}

show_one(cpus_boosted);
show_one(min_cpus_online);
show_one(max_cpus_online);
show_long(full_mode_profile);
show_one(cpu_nr_run_threshold);
show_one(debug_intelli_plug);
show_long(min_input_interval);
show_long(boost_lock_duration);
show_long(down_lock_dur);
show_long(nr_run_hysteresis);
show_one(nr_fshift);
show_long(def_sampling_ms);
show_one(high_load_threshold);
show_one(target_cpus);

#define store_one(object, min, max)		\
static ssize_t store_##object		\
(struct kobject *kobj,				\
 struct kobj_attribute *attr,			\
 const char *buf, size_t count)			\
{						\
	unsigned int input;			\
	int ret;				\
	ret = sscanf(buf, "%u", &input);	\
	if (ret != 1)			\
		return -EINVAL;			\
	if (input <= min)	\
		input = min;	\
	if (input >= max)		\
			input = max;		\
	if (input == object) {			\
		return count;			\
	}					\
	object = input;				\
	return count;				\
}

store_one(cpus_boosted, 0, max_cpus_online);
store_one(debug_intelli_plug, 0, 1);
store_one(high_load_threshold, 0, MAX_LOAD_FREQ);

#define store_one_long(object, min, max)		\
static ssize_t store_##object		\
(struct kobject *kobj,				\
 struct kobj_attribute *attr,			\
 const char *buf, size_t count)			\
{						\
	unsigned long input;			\
	int ret;				\
	ret = sscanf(buf, "%lu", &input);	\
	if (ret != 1)			\
		return -EINVAL;			\
	if (input <= min)	\
		input = min;	\
	if (input >= max)		\
			input = max;		\
	if (input == object) {			\
		return count;			\
	}					\
	object = input;				\
	return count;				\
}

store_one_long(full_mode_profile, 0, 4);

#define store_one_ktimer(object, min, max)		\
static ssize_t store_##object		\
(struct kobject *kobj,				\
 struct kobj_attribute *attr,			\
 const char *buf, size_t count)			\
{						\
	unsigned long input;			\
	int ret;				\
	ret = sscanf(buf, "%lu", &input);	\
	if (ret != 1)			\
		return -EINVAL;			\
	if (input <= min)	\
		input = min;	\
	if (input >= max)		\
			input = max;		\
	if (input == object) {			\
		return count;			\
	}					\
	object = INTELLI_MS(input);				\
	return count;				\
}

#define MAX_LOCK_TIMEOUT 1000
store_one_ktimer(min_input_interval, 100, MAX_LOCK_TIMEOUT);
store_one_ktimer(boost_lock_duration, 100, MAX_LOCK_TIMEOUT);
store_one_ktimer(down_lock_dur, 100, MAX_LOCK_TIMEOUT);
store_one_ktimer(def_sampling_ms, 5, MAX_LOCK_TIMEOUT);

static ssize_t show_intelli_plug_active(struct kobject *kobj,
					struct kobj_attribute *attr,
					char *buf)
{
	unsigned int tmp;

	tmp = intelliread();
	return sprintf(buf, "%u\n", tmp);
}

static ssize_t store_intelli_plug_active(struct kobject *kobj,
					 struct kobj_attribute *attr,
					 const char *buf, size_t count)
{
	int ret;
	int input;

	ret = sscanf(buf, "%d", &input);
	if (ret < 0)
		return ret;

	if (input <= 0)
		input = 0;
	if (input >= 1)
		input = 1;
	if (input == intelliread())
		return count;

	intelli_plug_active_eval_fn(input);

	return count;
}

static ssize_t store_min_cpus_online(struct kobject *kobj,
				     struct kobj_attribute *attr,
				     const char *buf, size_t count)
{
	int ret;
	unsigned int val;

	ret = sscanf(buf, "%u", &val);
	if (ret != 1)
		return -EINVAL;

	if (val <= 1)
		val = 1;
	if (val >= NR_CPUS)
		val = NR_CPUS;
	if (val >= max_cpus_online)
		val = max_cpus_online;

	min_cpus_online = val;

	return count;
}

static ssize_t store_max_cpus_online(struct kobject *kobj,
				     struct kobj_attribute *attr,
				     const char *buf, size_t count)
{
	int ret;
	unsigned int val;

	ret = sscanf(buf, "%u", &val);
	if (ret != 1)
		return -EINVAL;

	if (val <= 1)
		val = 1;
	if (val >= NR_CPUS)
		val = NR_CPUS;
	if (val <= min_cpus_online)
		val = min_cpus_online;

	max_cpus_online = val;

	return count;
}

#define KERNEL_ATTR_RW(_name) \
static struct kobj_attribute _name##_attr = \
	__ATTR(_name, 0644, show_##_name, store_##_name)

#define KERNEL_ATTR_RO(_name) \
static struct kobj_attribute _name##_attr = \
	__ATTR(_name, 0444, show_##_name, NULL)

KERNEL_ATTR_RW(intelli_plug_active);
KERNEL_ATTR_RW(cpus_boosted);
KERNEL_ATTR_RW(min_cpus_online);
KERNEL_ATTR_RW(max_cpus_online);
KERNEL_ATTR_RW(full_mode_profile);
KERNEL_ATTR_RO(cpu_nr_run_threshold);
KERNEL_ATTR_RW(boost_lock_duration);
KERNEL_ATTR_RW(def_sampling_ms);
KERNEL_ATTR_RW(debug_intelli_plug);
KERNEL_ATTR_RO(nr_fshift);
KERNEL_ATTR_RO(nr_run_hysteresis);
KERNEL_ATTR_RW(down_lock_dur);
KERNEL_ATTR_RW(min_input_interval);
KERNEL_ATTR_RW(high_load_threshold);
KERNEL_ATTR_RO(target_cpus);

static struct attribute *intelli_plug_attrs[] = {
	&intelli_plug_active_attr.attr,
	&cpus_boosted_attr.attr,
	&min_cpus_online_attr.attr,
	&max_cpus_online_attr.attr,
	&full_mode_profile_attr.attr,
	&cpu_nr_run_threshold_attr.attr,
	&boost_lock_duration_attr.attr,
	&def_sampling_ms_attr.attr,
	&debug_intelli_plug_attr.attr,
	&nr_fshift_attr.attr,
	&nr_run_hysteresis_attr.attr,
	&down_lock_dur_attr.attr,
	&min_input_interval_attr.attr,
	&high_load_threshold_attr.attr,
	&target_cpus_attr.attr,
	NULL,
};

static struct attribute_group intelli_plug_attr_group = {
	.attrs = intelli_plug_attrs,
	.name = "intelli_plug",
};

static int __init intelli_plug_init(void)
{
	int rc;

	rc = sysfs_create_group(kernel_kobj, &intelli_plug_attr_group);
	if (rc)
		return -ENOMEM;

	pr_info("intelli_plug: version %d.%d\n",
		 INTELLI_PLUG_MAJOR_VERSION,
		 INTELLI_PLUG_MINOR_VERSION);
	//spin_lock_init(&ips_lock);
	wake_lock_init(&ipwlock, WAKE_LOCK_SUSPEND, "intelliplug");

	return 0;
}

static void __exit intelli_plug_exit(void)
{
	if (intelliread()) {
		intelli_plug_stop();
		intelliput();
	}

	wake_lock_destroy(&ipwlock);
	sysfs_remove_group(kernel_kobj, &intelli_plug_attr_group);
}

late_initcall_sync(intelli_plug_init);
module_exit(intelli_plug_exit);

MODULE_LICENSE("GPLv2");
MODULE_AUTHOR("Paul Reioux <reioux@gmail.com>, \
		Alucard24, Dorimanx, neobuddy89, Robcore");
MODULE_DESCRIPTION("'intell_plug' - An intelligent cpu hotplug driver for "
	"Low Latency Frequency Transition capable processors");
MODULE_LICENSE("GPLv2");
