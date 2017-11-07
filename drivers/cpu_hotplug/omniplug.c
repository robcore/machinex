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
 * Omniplug - A central framework for common hotplug driver variables.
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
#include <linux/powersuspend.h>
#include <linux/omniboost.h>
#include <linux/omniplug.h>

enum {
	MX_HOTPLUG = 0,
	INTELLI_HOTPLUG = 1,
	ALUCARD_HOTPLUG = 2,
	BRICKED_HOTPLUG = 3,
	MSM_SLEEPER = 4,
	LAZYPLUG = 5,
	BLU_PLUG = 6,
	UPLUG = 7,
	DISABLED = 8,
};

unsigned int hotplug_driver = DISABLED;
unsigned int min_cpus_online = DEFAULT_MIN_CPUS_ONLINE;
unsigned int max_cpus_online = DEFAULT_MAX_CPUS_ONLINE;

mx_show_one(hotplug_driver);
mx_show_one(min_cpus_online);
mx_show_one(max_cpus_online);

static ssize_t store_hotplug_driver(struct kobject *kobj,
				     struct kobj_attribute *attr,
				     const char *buf, size_t count)
{
	int ret;
	unsigned int previous_driver, val;

	ret = sscanf(buf, "%u", &val);
	if (ret != 1)
		return -EINVAL;

	sanitize_min_max(val, MX_HOTPLUG, UPLUG);

	if (val == hotplug_driver)
		return count;

	previous_driver = hotplug_driver;

	switch (previous_driver) {
	case MX_HOTPLUG:
		ignition(0);
		break;
	case INTELLI_HOTPLUG:
		intelli_plug_active_eval_fn(0);
		break;
	case ALUCARD_HOTPLUG:
		cpus_hotplugging(0);
		break;
	case BRICKED_HOTPLUG:
		bricked_hotplug_stop();
		break;
	case MSM_SLEEPER:
		start_stop_sleeper(0);
		break;
	case LAZYPLUG:
		start_stop_lazy_plug(0);
		break;
	case BLU_PLUG:
		dyn_hp_init_exit(0);
		break;
	case UPLUG:
		uplug_start_stop(0);
		break;
	case DISABLED:
		break;
	default:
		break;
	}

	hotplug_driver = val;
	switch (hotplug_driver) {
	case MX_HOTPLUG:
		ignition(1);
		break;
	case INTELLI_HOTPLUG:
		intelli_plug_active_eval_fn(1);
		break;
	case ALUCARD_HOTPLUG:
		cpus_hotplugging(1);
		break;
	case BRICKED_HOTPLUG:
		bricked_hotplug_start();
		break;
	case MSM_SLEEPER:
		start_stop_sleeper(1);
		break;
	case LAZYPLUG:
		start_stop_lazy_plug(1);
		break;
	case BLU_PLUG:
		dyn_hp_init_exit(1);
		break;
	case UPLUG:
		uplug_start_stop(1);
		break;
	case DISABLED:
		break;
	default:
		break;
	}
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

MX_ATTR_RW(hotplug_driver);
MX_ATTR_RW(max_cpus_online);
MX_ATTR_RW(min_cpus_online);

static struct attribute *omniplug_attributes[] = {
	&hotplug_driver_attr.attr,
	&min_cpus_online_attr.attr,
	&max_cpus_online_attr.attr,
	NULL,
};

static struct attribute_group omniplug_attr_group = {
	.attrs = omniplug_attributes,
	.name = "omniplug",
};

static int omniplug_init(void)
{
	int sysfs_result;

	sysfs_result = sysfs_create_group(kernel_kobj,
		&omniplug_attr_group);

	if (sysfs_result) {
		pr_info("%s group create failed!\n", __FUNCTION__);
		return -ENOMEM;
	}

	return 0;
}

late_initcall(omniplug_init);
MODULE_AUTHOR("Rob Patershuk <robpatershuk@gmail.com>");
MODULE_DESCRIPTION("'omniplug' - An interface for all common hotplug driver tunables");
MODULE_LICENSE("GPLv2");
