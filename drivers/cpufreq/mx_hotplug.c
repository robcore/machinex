/* Copyright (c) 2017, Rob Patershuk <robpatershuk@gmail.com>. All rights reserved.
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
 * MX Hotplug - A hotplugging driver that plugs cores based on
 * nr_running requests and load averages.
 * Thanks to Thalamus and Steve Loebrich for the inspiration!
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/cpu.h>
#include <linux/workqueue.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/mutex.h>
#include <linux/cpufreq.h>
#include <linux/kobject.h>
#include <linux/sysfs_helpers.h>
#include <linux/machinex_defines.h>
#include <linux/powersuspend.h>

#define MX_SAMPLE_RATE 500UL
static unsigned int mx_hotplug_active;
static bool hotplug_suspended;
static struct task_struct *mx_hp_engine;

static unsigned long boost_threshold = 2500;
static unsigned long upstage = 850;
static unsigned long downstage = 550;
static unsigned long sampling_rate = MX_SAMPLE_RATE;
static unsigned int online_cpus;
static unsigned int min_cpus_online = DEFAULT_MIN_CPUS_ONLINE;
static unsigned int max_cpus_online = DEFAULT_MAX_CPUS_ONLINE;
static unsigned int cpus_boosted = DEFAULT_MAX_CPUS_ONLINE;

static void inject_nos(bool from_input)
{
	unsigned int cpu;
	int ret;

	if (!hotplug_ready)
		return;

	if (from_input) {
		for_each_nonboot_offline_cpu(cpu) {
			if (cpu_out_of_range_hp(cpu) ||
				num_online_cpus() == cpus_boosted)
				break;
			if (cpu_online(cpu) ||
				!is_cpu_allowed(cpu) ||
				thermal_core_controlled(cpu))
				continue;
		cpu_up(cpu);
		}
	} else {
		for_each_nonboot_offline_cpu(cpu) {
			if (cpu_out_of_range_hp(cpu))
				break;
			if (cpu_online(cpu) ||
				!is_cpu_allowed(cpu) ||
				thermal_core_controlled(cpu))
				continue;
		cpu_up(cpu);
		}
	}
}
/*
static int machinex_hotplug_engine(void *data)
{
	unsigned int cpu;
again:
	set_current_state(TASK_INTERRUPTIBLE);

	if (!hotplug_ready)
		schedule();

	if (kthread_should_stop())
		return 0;

	set_current_state(TASK_RUNNING);

	if (avg_nr_running > boost_threshold) {
		inject_nos();
		goto end;
	}

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

end:
	goto again;
}
*/
static void mx_hotplug_suspend(struct power_suspend *h)
{

	if (!hotplug_suspended) {
		hotplug_suspended = true;
	}
}

static void mx_hotplug_resume(struct power_suspend *h)
{
	if (hotplug_suspended) {
		hotplug_suspended = false;
	}
}

static struct power_suspend mx_suspend_data =
{
	.suspend = mx_hotplug_suspend,
	.resume = mx_hotplug_resume,
};

static void mx_hotplug_start(void)
{
	if (!mx_hotplug_active)
		return;

	register_power_suspend(&mx_suspend_data);
}

static void mx_hotplug_stop(void)
{
	if (mx_hotplug_active)
		return;
	unregister_power_suspend(&mx_suspend_data);
}

static void mx_startstop(unsigned int status)
{
	if (status)
		mx_hotplug_start();
	else
		mx_hotplug_stop();
}

mx_show_one(mx_hotplug_active);
mx_show_one(min_cpus_online);
mx_show_one(max_cpus_online);

static ssize_t store_mx_hotplug_active(struct kobject *kobj,
					 struct kobj_attribute *attr,
					 const char *buf, size_t count)
{
	int ret;
	int input;

	ret = sscanf(buf, "%d", &input);
	if (ret < 0)
		return ret;

	sanitize_min_max(input, 0, 1);
	if (input == mx_hotplug_active)
		return count;

	mx_hotplug_active = input;
	//mx_startstop(mx_hotplug_active);

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

MX_ATTR_RW(mx_hotplug_active);
MX_ATTR_RW(min_cpus_online);
MX_ATTR_RW(max_cpus_online);

static struct attribute *mx_hotplug_attributes[] = {
	&mx_hotplug_active_attr.attr,
	&min_cpus_online_attr.attr,
	&max_cpus_online_attr.attr,
	NULL,
};

static struct attribute_group mx_hotplug_attr_group = {
	.attrs = mx_hotplug_attributes,
	.name = "mx_hotplug",
};

static int mx_hotplug_init(void)
{
	int sysfs_result;

	sysfs_result = sysfs_create_group(kernel_kobj,
		&mx_hotplug_attr_group);

	if (sysfs_result) {
		pr_info("%s group create failed!\n", __FUNCTION__);
		return -ENOMEM;
	}
	return 0;
}

late_initcall(mx_hotplug_init);
MODULE_AUTHOR("Rob Patershuk <robpatershuk@gmail.com>");
MODULE_DESCRIPTION("'mx_hotplug' - An rq based hotplug driver");
MODULE_LICENSE("GPLv2");