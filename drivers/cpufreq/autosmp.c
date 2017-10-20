/*
 * drivers/soc/qcom/autosmp.c
 *
 * automatically hotplug/unplug multiple cpu cores
 * based on cpu load and suspend state
 *
 * based on the msm_mpdecision code by
 * Copyright (c) 2012-2013, Dennis Rassmann <showp1984@gmail.com>
 *
 * Copyright (C) 2013-2014, Rauf Gungor, http://github.com/mrg666
 * rewrite to simplify and optimize, Jul. 2013, http://goo.gl/cdGw6x
 * optimize more, generalize for n cores, Sep. 2013, http://goo.gl/448qBz
 * generalize for all arch, rename as autosmp, Dec. 2013, http://goo.gl/x5oyhy
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version. For more details, see the GNU
 * General Public License included with the Linux kernel or available
 * at www.gnu.org/licenses
 */

#include <linux/moduleparam.h>
#include <linux/cpufreq.h>
#include <linux/workqueue.h>
#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/slab.h>
#include <linux/hrtimer.h>
#include <linux/input.h>
#include "../../arch/arm/mach-msm/acpuclock.h"
#include <linux/display_state.h>
#include <linux/powersuspend.h>

#define ASMP_TAG			"AutoSMP:"
#define ASMP_MS(x) ((((x) * MSEC_PER_SEC) / MSEC_PER_SEC))
#define DEFAULT_BOOST_LOCK_DUR ASMP_MS(250UL)
#define DEFAULT_NR_CPUS_BOOSTED		4
#define DEFAULT_UPDATE_RATE		ASMP_MS(80UL)
#define MIN_INPUT_INTERVAL		ASMP_MS(200UL)
#define DEFAULT_MIN_BOOST_FREQ		1566000

static DEFINE_SPINLOCK(asmp_lock);
static struct delayed_work asmp_work;
static struct delayed_work prework;
static struct workqueue_struct *asmp_workq;
static unsigned int asmp_enabled = 0;

static unsigned int delay = DEFAULT_UPDATE_RATE;
static unsigned int max_cpus_online = NR_CPUS;
static unsigned int min_cpus_online = 2;
static unsigned int cpufreq_up = 95;
static unsigned int cpufreq_down = 80;
static unsigned int cycle_up = 1;
static unsigned int cycle_down = 1;
static unsigned int cpus_boosted = DEFAULT_NR_CPUS_BOOSTED;
static unsigned int min_boost_freq = DEFAULT_MIN_BOOST_FREQ;
static unsigned long boost_lock_duration = DEFAULT_BOOST_LOCK_DUR;
static ktime_t last_boost_time;
static ktime_t last_input;
static unsigned int cycle;
static unsigned int should_boost;

static void reschedule_hotplug_work(bool from_boost)
{
	unsigned int cpu;
	ktime_t delta;

	if (from_boost)
		WRITE_ONCE(should_boost, 1);
	else
		WRITE_ONCE(should_boost, 0);

	if (!hotplug_ready) {
		mod_delayed_work_on(0, asmp_workq, &prework,
				msecs_to_jiffies(delay));
		return;
	}
	if (!asmp_enabled || !is_display_on())
		return;

	delta = ktime_sub(ktime_get(), last_input);
	if (ktime_compare(delta, ms_to_ktime(boost_lock_duration)) <= 0) {
		mod_delayed_work_on(0, asmp_workq, &prework,
				msecs_to_jiffies(delay));
		return;
	}
	if (!from_boost) {
		mod_delayed_work_on(0, asmp_workq, &asmp_work,
				msecs_to_jiffies(delay));
		return;
	} else {
		if (num_online_cpus() == cpus_boosted)
			return;
		for_each_nonboot_offline_cpu(cpu) {
			if (cpu_out_of_range_hp(cpu))
				break;
			if (cpu_online(cpu))
				continue;
			if (is_cpu_allowed(cpu) || !thermal_core_controlled(cpu))
				cpu_up(cpu);
			if (num_online_cpus() == cpus_boosted)
				return;
		}
		mod_delayed_work_on(0, asmp_workq, &prework,
				msecs_to_jiffies(delay));
	}
}

static void asmp_work_fn(struct work_struct *work)
{
	unsigned int cpu, slow_cpu, \
	slow_rate = UINT_MAX, fast_rate, \
	max_rate, up_rate, down_rate, nr_cpu_online, \
	local_min_boost_freq = min_boost_freq;
	unsigned int rate[NR_CPUS] = { 0, 0, 0, 0 };
	unsigned long flags;

	if (!asmp_enabled || !is_display_on())
		return;

	if (!hotplug_ready)
		goto resched;

	/* get maximum possible hardlimit freq for cpu0 and
	   calculate up/down limits */
	max_rate  = cpufreq_quick_get_max(0);
	up_rate   = (max_rate / 100) * cpufreq_up;
	down_rate = (max_rate / 100) * cpufreq_down;

	/* find current max and min cpu freq to estimate load */
	nr_cpu_online = num_online_cpus();
	rate[0] = cpufreq_quick_get(0);
	fast_rate = rate[0];

	if (nr_cpu_online > min_cpus_online) {
		for_each_nonboot_online_cpu(cpu) {
			if (cpu_out_of_range_hp(cpu))
				break;
			if (!cpu_online(cpu))
				continue;
			if (!is_cpu_allowed(cpu) || thermal_core_controlled(cpu))
				continue;
			rate[cpu] = cpufreq_quick_get(cpu);
			if (rate[cpu] <= slow_rate) {
				slow_cpu = cpu;
				slow_rate = rate[cpu];
			} else if (rate[cpu] > fast_rate)
				fast_rate = rate[cpu];

			if (rate[0] < slow_rate)
				slow_rate = rate[0];

			if (max_rate <= min_boost_freq)
				local_min_boost_freq = max_rate;
			/* unplug slowest core if all online cores are under down_rate limit */
			if (slow_cpu && (fast_rate < down_rate) &&
					   	cycle >= cycle_down) {
					if (cpu_online(slow_cpu)) {
			 			cpu_down(slow_cpu);
						if (cycle > 0)
							cycle--;
					} else
						continue;
			}
		if (nr_cpu_online == min_cpus_online)
			break;
		}
		cycle++;
	} else if (nr_cpu_online < max_cpus_online) {
		for_each_nonboot_online_cpu(cpu) {
			if (cpu_out_of_range_hp(cpu))
				break;
			if (cpu_online(cpu))
				continue;
			if (!is_cpu_allowed(cpu) || thermal_core_controlled(cpu))
				continue;
			rate[cpu] = cpufreq_quick_get(cpu);
			if (rate[cpu] <= slow_rate) {
				slow_cpu = cpu;
				slow_rate = rate[cpu];
			} else if (rate[cpu] > fast_rate)
				fast_rate = rate[cpu];

			if (rate[0] < slow_rate)
				slow_rate = rate[0];

			if (max_rate <= min_boost_freq)
				local_min_boost_freq = max_rate;


			/* hotplug one core if all online cores are over up_rate limit */
			if (slow_rate > up_rate && fast_rate >= local_min_boost_freq &&
				cycle >= cycle_up) {
					if (cpu_is_offline(cpu)) {
						cpu_up(cpu);
						if (cycle > 0)
							cycle--;
					} else
						continue;
			}
		if (nr_cpu_online == max_cpus_online)
			break;
		}
		cycle++;
	}

resched:
	reschedule_hotplug_work(false);
}

static void prework_fn(struct work_struct *work)
{
	if (READ_ONCE(should_boost))
		reschedule_hotplug_work(true);
	else
		reschedule_hotplug_work(false);

}

static void asmp_suspend(struct power_suspend *h)
{
}

static void asmp_resume(struct power_suspend *h)
{
	if (!asmp_enabled)
		return;

	reschedule_hotplug_work(false);
}

static struct power_suspend asmp_suspend_data =
{
	.suspend = asmp_suspend,
	.resume = asmp_resume,
};

void autosmp_input_boost(void)
{
	ktime_t delta;

	if (!asmp_enabled || !is_display_on() || !hotplug_ready)
		return;

	last_input = ktime_get();
	delta = ktime_sub(last_input, last_boost_time);

	if ((ktime_compare(delta, ms_to_ktime(MIN_INPUT_INTERVAL))  < 0) ||
		num_online_cpus() >= cpus_boosted ||
	    cpus_boosted <= min_cpus_online)
		return;
	reschedule_hotplug_work(true);
	last_boost_time = ktime_get();
}

static void hotplug_start_stop(unsigned int enabled)
{
	if (enabled) {
		asmp_workq = create_singlethread_workqueue("autosmp");
		if (WARN_ON_ONCE(!asmp_workq)) {
			pr_err("%s: Failed to allocate hotplug workqueue\n",
						ASMP_TAG);
			asmp_enabled = 0;
			return;
		}

		INIT_DELAYED_WORK(&prework, prework_fn);
		INIT_DELAYED_WORK(&asmp_work, asmp_work_fn);
		register_power_suspend(&asmp_suspend_data);
		reschedule_hotplug_work(false);
	} else {
		cancel_delayed_work(&prework);
		cancel_delayed_work(&asmp_work);
		unregister_power_suspend(&asmp_suspend_data);
		destroy_workqueue(asmp_workq);
	}
}

/***************************** SYSFS START *****************************/
struct kobject *asmp_kobject;

#define asmp_show_one(object)				\
static ssize_t show_##object					\
(struct kobject *kobj, struct kobj_attribute *attr, char *buf)	\
{								\
	return sprintf(buf, "%u\n", object);			\
}

#define asmp_show_long(object)				\
static ssize_t show_##object					\
(struct kobject *kobj, struct kobj_attribute *attr, char *buf)	\
{								\
	return sprintf(buf, "%lu\n", object);			\
}

asmp_show_one(delay);
asmp_show_one(min_cpus_online);
asmp_show_one(max_cpus_online);
asmp_show_one(cpufreq_up);
asmp_show_one(cpufreq_down);
asmp_show_one(cycle_up);
asmp_show_one(cycle_down);
asmp_show_one(cpus_boosted);
asmp_show_long(boost_lock_duration);

#define asmp_store_one(object, min, max)		\
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

#define asmp_store_one_ktimer(object, min, max)		\
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
	object = ASMP_MS(input);				\
	return count;				\
}

asmp_store_one(delay, 10, 250);
asmp_store_one(min_cpus_online, 1, max_cpus_online);
asmp_store_one(max_cpus_online, min_cpus_online, 4);
asmp_store_one(cpufreq_up, 1, 99);
asmp_store_one(cpufreq_down, 1, 99);
asmp_store_one(cycle_up, 1, 3);
asmp_store_one(cycle_down, 1, 3);
asmp_store_one(cpus_boosted, min_cpus_online, max_cpus_online);
asmp_store_one_ktimer(boost_lock_duration, 100, 1000);

static ssize_t show_asmp_enabled(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	int ret;

	ret = sprintf(buf, "%u\n", asmp_enabled);

	return ret;
}

static ssize_t store_asmp_enabled(struct kobject *kobj, 
				struct kobj_attribute *attr, const char *buf,
					 size_t count)
{
	int val;

	if (sscanf(buf, "%u", &val) != 1)
		return -EINVAL;

	sanitize_min_max(val, 0, 1);

	if (val == asmp_enabled)
		return count;

	asmp_enabled = val;

	hotplug_start_stop(asmp_enabled);

	return count;
}

#define ASMP_ATTR_RW(_name) \
static struct kobj_attribute _name##_attr = \
	__ATTR(_name, 0644, show_##_name, store_##_name)

ASMP_ATTR_RW(asmp_enabled);
ASMP_ATTR_RW(delay);
ASMP_ATTR_RW(min_cpus_online);
ASMP_ATTR_RW(max_cpus_online);
ASMP_ATTR_RW(cpufreq_up);
ASMP_ATTR_RW(cpufreq_down);
ASMP_ATTR_RW(cycle_up);
ASMP_ATTR_RW(cycle_down);
ASMP_ATTR_RW(boost_lock_duration);
ASMP_ATTR_RW(cpus_boosted);

static struct attribute *asmp_attributes[] = {
	&asmp_enabled_attr.attr,
	&delay_attr.attr,
	&min_cpus_online_attr.attr,
	&max_cpus_online_attr.attr,
	&cpufreq_up_attr.attr,
	&cpufreq_down_attr.attr,
	&cycle_up_attr.attr,
	&cycle_down_attr.attr,
	&boost_lock_duration_attr.attr,
	&cpus_boosted_attr.attr,
	NULL
};

static struct attribute_group asmp_attr_group = {
	.attrs = asmp_attributes,
	.name = "autosmp",
};

/****************************** SYSFS END ******************************/

static int __init asmp_init(void)
{
	int ret = 0;
	unsigned long flags;

	ret = sysfs_create_group(kernel_kobj, &asmp_attr_group);
	if (ret) {
		ret = -ENOMEM;
		return ret;
	}

	pr_info(ASMP_TAG "Init complete.\n");
	return ret;
}

late_initcall(asmp_init);
