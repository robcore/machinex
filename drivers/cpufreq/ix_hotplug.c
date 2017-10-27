/* Copyright (c) 2013, Steve Loebrich <sloebric@gmail.com>. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

/*
 * Generic auto hotplug driver for ARM SoCs. Targeted at current generation
 * SoCs with dual and quad core applications processors.
 * Automatically hotplugs online and offline CPUs based on system load.
 * It is also capable of immediately onlining a core based on an external
 * event by calling void hotplug_boostpulse(void)
 *
 * Not recommended for use with OMAP4460 due to the potential for lockups
 * whilst hotplugging.
 *
 * Thanks to Thalamus for the inspiration!
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/cpu.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/mutex.h>
#include <linux/cpufreq.h>
#include <linux/kobject.h>
#include <linux/sysfs_helpers.h>
#include <linux/machinex_defines.h>

#include <linux/powersuspend.h>

#define IX_HOTPLUG "ix_hotplug"

/*
 * Load defines:
 * ENABLE_ALL is a high watermark to rapidly online all CPUs
 *
 * ENABLE is the load which is required to enable 1 extra CPU
 * DISABLE is the load at which a CPU is disabled
 * These two are scaled based on num_online_cpus()
 */

static unsigned int ix_hotplug_active;
static bool hotplug_suspended;

static struct delayed_work hotplug_decision_work;
static struct workqueue_struct *ixwq;

static unsigned int enable_all_load = 702;
static unsigned int enable_load[4] = {80, 220, 340, UINT_MAX};
static unsigned int disable_load[4] = {12, 60, 120, 260};
static unsigned int sample_rate[4] = {25, 50, 100, 50};
static unsigned int online_sampling_periods[4] = {3, 3, 5, UINT_MAX};
static unsigned int offline_sampling_periods[5] = {0, 8, 3, 4};
static unsigned int online_cpus;
static unsigned int min_cpus_online = DEFAULT_MIN_CPUS_ONLINE;
static unsigned int max_cpus_online = DEFAULT_MAX_CPUS_ONLINE;

#define DEF_SRATE 100
static unsigned int sampling_rate = DEF_SRATE;
static unsigned int online_sample;
static unsigned int offline_sample;


static void hotplug_online_single_work(void)
{
	unsigned int cpuinc, cpuid = 0;

	if (hotplug_suspended || !hotplug_ready ||
		!ix_hotplug_active)
		return;

	cpuinc = 0;

retry:
	cpuid = cpumask_next_zero(cpuinc, cpu_online_mask);
	if (cpu_out_of_range_hp(cpuid))
		return;

	if (cpu_online(cpuid) || !is_cpu_allowed(cpuid) ||
		thermal_core_controlled(cpuid)) {
		cpuinc++;
		goto retry;
	}

	cpu_up(cpuid);
}

static void hotplug_online_all_work(void)
{
	unsigned int cpu;

	if (hotplug_suspended || !hotplug_ready ||
		!ix_hotplug_active)
		return;

	for_each_nonboot_offline_cpu(cpu) {
		if (cpu_out_of_range_hp(cpu))
			break;
		if (cpu_online(cpu) || !is_cpu_allowed(cpu) ||
			thermal_core_controlled(cpu))
			continue;
			cpu_up(cpu);
			//pr_info("ix_hotplug: CPU%d up.\n", cpu);
	}
	return;
}

static void hotplug_offline_work(void)
{
	unsigned int cpu;

	if (hotplug_suspended || !hotplug_ready ||
		!ix_hotplug_active)
		return;
	for_each_nonboot_online_cpu(cpu) {
		if (cpu_out_of_range_hp(cpu))
			break;
		if (!cpu_online(cpu) || !is_cpu_allowed(cpu) ||
			thermal_core_controlled(cpu))
			continue;
		if (num_online_cpus() > min_cpus_online)
			cpu_down(cpu);
	}
}

static void __ref hotplug_decision_work_fn(struct work_struct *work)
{
	unsigned long avg_running;

	if (hotplug_suspended || !ix_hotplug_active)
		return;

	if (!hotplug_ready)
		goto resched;

	online_cpus = num_online_cpus();

	avg_running = avg_nr_running() / online_cpus;

	if ((avg_running < disable_load[online_cpus]) &&
			(online_cpus > min_cpus_online)) {
		if (offline_sample > offline_sampling_periods[online_cpus]) {
			hotplug_offline_work();
			offline_sample = 0;
		}
		offline_sample++;
		online_sample = 1;
	} else if ((avg_running > enable_all_load ||
				avg_running > enable_load[online_cpus]) &&
			(online_cpus < max_cpus_online)) {
		if (online_sample > online_sampling_periods[online_cpus]) {
			if (avg_running > enable_all_load) {
				//pr_info("ix_hotplug: Enable All\n");
				hotplug_online_all_work();
			} else {
				//pr_info("ix_hotplug: Enable Single\n");
				hotplug_online_single_work();
			}
			online_sample = 0;
		}
		online_sample++;
		offline_sample = 1;
	}
	if (online_cpus > 1)
		sampling_rate = sample_rate[online_cpus];
	else
		sampling_rate = sample_rate[online_cpus - 1];
resched:
	queue_delayed_work_on(0, ixwq, &hotplug_decision_work, sampling_rate);
}

static void ix_hotplug_suspend(struct power_suspend *h)
{

	if (!hotplug_suspended) {
		hotplug_suspended = true;
		cancel_delayed_work(&hotplug_decision_work);
	}
}

static void ix_hotplug_resume(struct power_suspend *h)
{
	if (hotplug_suspended) {
		hotplug_suspended = false;
		offline_sample = 1;
		online_sample = 1;
		queue_delayed_work_on(0, ixwq, &hotplug_decision_work, sampling_rate);
	}
}

static struct power_suspend ix_suspend_data =
{
	.suspend = ix_hotplug_suspend,
	.resume = ix_hotplug_resume,
};

static void ix_hotplug_start(void)
{
	if (!ix_hotplug_active)
		return;
	ixwq = create_singlethread_workqueue("ix_hotplug_workqueue");

    if (!ixwq)
        return;

	INIT_DELAYED_WORK(&hotplug_decision_work, hotplug_decision_work_fn);
	register_power_suspend(&ix_suspend_data);
	queue_delayed_work_on(0, ixwq, &hotplug_decision_work, sampling_rate);
}

static void ix_hotplug_stop(void)
{
	if (!ixwq || ix_hotplug_active)
		return;
	cancel_delayed_work_sync(&hotplug_decision_work);
	unregister_power_suspend(&ix_suspend_data);
	destroy_workqueue(ixwq);
}

static void ix_startstop(unsigned int status)
{
	if (status)
		ix_hotplug_start();
	else
		ix_hotplug_stop();
}

#define ix_show_one(object)				\
static ssize_t show_##object					\
(struct kobject *kobj, struct kobj_attribute *attr, char *buf)	\
{								\
	return sprintf(buf, "%u\n", object);			\
}

ix_show_one(ix_hotplug_active);
ix_show_one(min_cpus_online);
ix_show_one(max_cpus_online);

static ssize_t store_ix_hotplug_active(struct kobject *kobj,
					 struct kobj_attribute *attr,
					 const char *buf, size_t count)
{
	int ret;
	int input;

	ret = sscanf(buf, "%d", &input);
	if (ret < 0)
		return ret;

	sanitize_min_max(input, 0, 1);
	if (input == ix_hotplug_active)
		return count;

	ix_hotplug_active = input;
	ix_startstop(ix_hotplug_active);

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

#define IX_ATTR_RW(_name) \
static struct kobj_attribute _name##_attr = \
	__ATTR(_name, 0644, show_##_name, store_##_name)

IX_ATTR_RW(ix_hotplug_active);
IX_ATTR_RW(min_cpus_online);
IX_ATTR_RW(max_cpus_online);

static struct attribute *ix_hotplug_attributes[] = {
	&ix_hotplug_active_attr.attr,
	&min_cpus_online_attr.attr,
	&max_cpus_online_attr.attr,
	NULL,
};

static struct attribute_group ix_hotplug_attr_group = {
	.attrs = ix_hotplug_attributes,
	.name = "ix_hotplug",
};

static int ix_hotplug_init(void)
{
	int sysfs_result;

	sysfs_result = sysfs_create_group(kernel_kobj,
		&ix_hotplug_attr_group);

	if (sysfs_result) {
		pr_info("%s group create failed!\n", __FUNCTION__);
		return -ENOMEM;
	}
	return 0;
}

late_initcall(ix_hotplug_init);
MODULE_AUTHOR("InstigatorX, \
		Rob Patershuk <robpatershuk@gmail.com>");
MODULE_DESCRIPTION("'ix_hotplug' - A simple hotplug driver "
	"with full automation as a replacement for mpdecision");
MODULE_LICENSE("GPLv2");