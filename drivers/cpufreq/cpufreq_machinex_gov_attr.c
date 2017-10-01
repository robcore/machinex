/* drivers/cpufreq/cpufreq_machinex_gov_attr.c
 *
 * Ugly, hacky, code for CPUFreq governor tunable sysfs attributes,
 * with the added bonus of keeping consistent kobjects regardless of
 * hotplug/unplug status.
 *
 * Copyright (C) 2017, Robert Patershuk
 * Author: Robert Patershuk <rob_patershuk@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "cpufreq_governor.h"
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sysfs.h>

/* Machinex OnDemand Tunables */
unsigned int od_cpu0_sampling_rate = 1000;
unsigned int od_cpu0_up_threshold = 80;
unsigned int od_cpu0_micro_up_threshold = 95;
unsigned int od_cpu0_sampling_down_factor = 1;
unsigned int od_cpu0_ignore_nice_load = 0;

unsigned int od_cpu1_sampling_rate = 1000;
unsigned int od_cpu1_up_threshold = 80;
unsigned int od_cpu1_micro_up_threshold = 95;
unsigned int od_cpu1_sampling_down_factor = 1;
unsigned int od_cpu1_ignore_nice_load = 0;

unsigned int od_cpu2_sampling_rate = 1000;
unsigned int od_cpu2_up_threshold = 80;
unsigned int od_cpu2_micro_up_threshold = 95;
unsigned int od_cpu2_sampling_down_factor = 1;
unsigned int od_cpu2_ignore_nice_load = 0;

unsigned int od_cpu3_sampling_rate = 1000;
unsigned int od_cpu3_up_threshold = 80;
unsigned int od_cpu3_micro_up_threshold = 95;
unsigned int od_cpu3_sampling_down_factor = 1;
unsigned int od_cpu3_ignore_nice_load = 0;

/* Machinex Conservative Tunables */
unsigned int cs_cpu0_down_threshold = 20;
unsigned int cs_cpu0_up_threshold = 80;
unsigned int cs_cpu0_freq_step = 5;
unsigned int cs_cpu0_sampling_down_factor = 1;
unsigned int cs_cpu0_ignore_nice_load = 0;

unsigned int cs_cpu1_down_threshold = 20;
unsigned int cs_cpu1_up_threshold = 80;
unsigned int cs_cpu1_freq_step = 5;
unsigned int cs_cpu1_sampling_down_factor = 1;
unsigned int cs_cpu1_ignore_nice_load = 0;

unsigned int cs_cpu2_down_threshold = 20;
unsigned int cs_cpu2_up_threshold = 80;
unsigned int cs_cpu2_freq_step = 5;
unsigned int cs_cpu2_sampling_down_factor = 1;
unsigned int cs_cpu2_ignore_nice_load = 0;

unsigned int cs_cpu3_down_threshold = 20;
unsigned int cs_cpu3_up_threshold = 80;
unsigned int cs_cpu3_freq_step = 5;
unsigned int cs_cpu3_sampling_down_factor = 1;
unsigned int cs_cpu3_ignore_nice_load = 0;

/* Machinex SchedUtil tunable */
unsigned int su_cpu0_rate_limit_us = 1000;
unsigned int su_cpu1_rate_limit_us = 1000;
unsigned int su_cpu2_rate_limit_us = 1000;
unsigned int su_cpu3_rate_limit_us = 1000;

#define show_one_mx(object)				\
static ssize_t show_##object					\
(struct kobject *kobj, struct kobj_attribute *attr, char *buf)	\
{								\
	return sprintf(buf, "%u\n", object);			\
}

show_one_mx(od_cpu0_up_threshold)
show_one_mx(od_cpu0_micro_up_threshold);
show_one_mx(od_cpu0_sampling_down_factor);
show_one_mx(od_cpu0_ignore_nice_load);
show_one_mx(od_cpu1_sampling_rate);
show_one_mx(od_cpu1_up_threshold);
show_one_mx(od_cpu1_micro_up_threshold);
show_one_mx(od_cpu1_sampling_down_factor);
show_one_mx(od_cpu1_ignore_nice_load);
show_one_mx(od_cpu2_sampling_rate);
show_one_mx(od_cpu2_up_threshold);
show_one_mx(od_cpu2_micro_up_threshold);
show_one_mx(od_cpu2_sampling_down_factor);
show_one_mx(od_cpu2_ignore_nice_load);
show_one_mx(od_cpu3_sampling_rate);
show_one_mx(od_cpu3_up_threshold);
show_one_mx(od_cpu3_micro_up_threshold);
show_one_mx(od_cpu3_sampling_down_factor);
show_one_mx(od_cpu3_ignore_nice_load);
show_one_mx(cs_cpu0_down_threshold);
show_one_mx(cs_cpu0_up_threshold);
show_one_mx(cs_cpu0_freq_step);
show_one_mx(cs_cpu0_sampling_down_factor);
show_one_mx(cs_cpu0_ignore_nice_load);
show_one_mx(cs_cpu1_down_threshold);
show_one_mx(cs_cpu1_up_threshold);
show_one_mx(cs_cpu1_freq_step);
show_one_mx(cs_cpu1_sampling_down_factor);
show_one_mx(cs_cpu1_ignore_nice_load);
show_one_mx(cs_cpu2_down_threshold);
show_one_mx(cs_cpu2_up_threshold);
show_one_mx(cs_cpu2_freq_step);
show_one_mx(cs_cpu2_sampling_down_factor);
show_one_mx(cs_cpu2_ignore_nice_load);
show_one_mx(cs_cpu3_down_threshold);
show_one_mx(cs_cpu3_up_threshold);
show_one_mx(cs_cpu3_freq_step);
show_one_mx(cs_cpu3_sampling_down_factor);
show_one_mx(cs_cpu3_ignore_nice_load);
show_one_mx(su_cpu0_rate_limit_us);
show_one_mx(su_cpu1_rate_limit_us);
show_one_mx(su_cpu2_rate_limit_us);
show_one_mx(su_cpu3_rate_limit_us);

store_one_clamp(od_cpu0_up_threshold, 1, 99);
store_one_clamp(od_cpu0_micro_up_threshold, 1, 99);
store_one_clamp(od_cpu0_sampling_down_factor, 1, 100000);
store_one_clamp(od_cpu0_ignore_nice_load, 0, 1);
store_one_clamp(od_cpu1_sampling_rate, 1000, 10000);
store_one_clamp(od_cpu1_up_threshold, 1, 99);
store_one_clamp(od_cpu1_micro_up_threshold, 1, 99);
store_one_clamp(od_cpu1_sampling_down_factor, 1, 100000);
store_one_clamp(od_cpu1_ignore_nice_load, 0, 1);
store_one_clamp(od_cpu2_sampling_rate, 1000, 10000);
store_one_clamp(od_cpu2_up_threshold, 1, 99);
store_one_clamp(od_cpu2_micro_up_threshold, 1, 99);
store_one_clamp(od_cpu2_sampling_down_factor, 1, 100000);
store_one_clamp(od_cpu2_ignore_nice_load, 0, 1);
store_one_clamp(od_cpu3_sampling_rate, 1000, 10000);
store_one_clamp(od_cpu3_up_threshold, 1, 99);
store_one_clamp(od_cpu3_micro_up_threshold, 1, 99);
store_one_clamp(od_cpu3_sampling_down_factor, 1, 100000);
store_one_clamp(od_cpu3_ignore_nice_load, 0, 1);
store_one_clamp(cs_cpu0_down_threshold, 1, 99);
store_one_clamp(cs_cpu0_up_threshold, 1, 99);
store_one_clamp(cs_cpu0_freq_step, 1, 5);
store_one_clamp(cs_cpu0_sampling_down_factor, 1, 100000);
store_one_clamp(cs_cpu0_ignore_nice_load, 0, 1);
store_one_clamp(cs_cpu1_down_threshold, 1, 99);
store_one_clamp(cs_cpu1_up_threshold, 1, 99);
store_one_clamp(cs_cpu1_freq_step, 1, 5);
store_one_clamp(cs_cpu1_sampling_down_factor, 1, 100000);
store_one_clamp(cs_cpu1_ignore_nice_load, 0, 1);
store_one_clamp(cs_cpu2_down_threshold, 1, 99);
store_one_clamp(cs_cpu2_up_threshold, 1, 99);
store_one_clamp(cs_cpu2_freq_step, 1, 5);
store_one_clamp(cs_cpu2_sampling_down_factor, 1, 100000);
store_one_clamp(cs_cpu2_ignore_nice_load, 0, 1);
store_one_clamp(cs_cpu3_down_threshold, 1, 99);
store_one_clamp(cs_cpu3_up_threshold, 1, 99);
store_one_clamp(cs_cpu3_freq_step, 1, 5);
store_one_clamp(cs_cpu3_sampling_down_factor, 1, 100000);
store_one_clamp(cs_cpu3_ignore_nice_load, 0, 1);
store_one_clamp(su_cpu0_rate_limit_us, 1000, 10000);
store_one_clamp(su_cpu1_rate_limit_us, 1000, 10000);
store_one_clamp(su_cpu2_rate_limit_us, 1000, 10000);
store_one_clamp(su_cpu3_rate_limit_us, 1000, 10000);

MX_ATTR_RW(od_cpu0_up_threshold);
MX_ATTR_RW(od_cpu0_micro_up_threshold);
MX_ATTR_RW(od_cpu0_sampling_down_factor);
MX_ATTR_RW(od_cpu0_ignore_nice_load);
MX_ATTR_RW(od_cpu1_sampling_rate);
MX_ATTR_RW(od_cpu1_up_threshold);
MX_ATTR_RW(od_cpu1_micro_up_threshold);
MX_ATTR_RW(od_cpu1_sampling_down_factor);
MX_ATTR_RW(od_cpu1_ignore_nice_load);
MX_ATTR_RW(od_cpu2_sampling_rate);
MX_ATTR_RW(od_cpu2_up_threshold);
MX_ATTR_RW(od_cpu2_micro_up_threshold);
MX_ATTR_RW(od_cpu2_sampling_down_factor);
MX_ATTR_RW(od_cpu2_ignore_nice_load);
MX_ATTR_RW(od_cpu3_sampling_rate);
MX_ATTR_RW(od_cpu3_up_threshold);
MX_ATTR_RW(od_cpu3_micro_up_threshold);
MX_ATTR_RW(od_cpu3_sampling_down_factor);
MX_ATTR_RW(od_cpu3_ignore_nice_load);
MX_ATTR_RW(cs_cpu0_down_threshold);
MX_ATTR_RW(cs_cpu0_up_threshold);
MX_ATTR_RW(cs_cpu0_freq_step);
MX_ATTR_RW(cs_cpu0_sampling_down_factor);
MX_ATTR_RW(cs_cpu0_ignore_nice_load);
MX_ATTR_RW(cs_cpu1_down_threshold);
MX_ATTR_RW(cs_cpu1_up_threshold);
MX_ATTR_RW(cs_cpu1_freq_step);
MX_ATTR_RW(cs_cpu1_sampling_down_factor);
MX_ATTR_RW(cs_cpu1_ignore_nice_load);
MX_ATTR_RW(cs_cpu2_down_threshold);
MX_ATTR_RW(cs_cpu2_up_threshold);
MX_ATTR_RW(cs_cpu2_freq_step);
MX_ATTR_RW(cs_cpu2_sampling_down_factor);
MX_ATTR_RW(cs_cpu2_ignore_nice_load);
MX_ATTR_RW(cs_cpu3_down_threshold);
MX_ATTR_RW(cs_cpu3_up_threshold);
MX_ATTR_RW(cs_cpu3_freq_step);
MX_ATTR_RW(cs_cpu3_sampling_down_factor);
MX_ATTR_RW(cs_cpu3_ignore_nice_load);
MX_ATTR_RW(su_cpu0_rate_limit_us);
MX_ATTR_RW(su_cpu1_rate_limit_us);
MX_ATTR_RW(su_cpu2_rate_limit_us);
MX_ATTR_RW(su_cpu3_rate_limit_us);

static struct attribute *ondemand0_attrs[] = {
	&od_cpu0_up_threshold_attr.attr,
	&od_cpu0_micro_up_threshold_attr.attr,
	&od_cpu0_sampling_down_factor_attr.attr,
	&od_cpu0_ignore_nice_load_attr.attr,
	NULL,
};

static struct attribute_group ondemand0_attr_group = {
	.attrs = ondemand0_attrs,
	.name = "ondemand",
};

static struct attribute *ondemand1_attrs[] = {
	&od_cpu1_up_threshold_attr.attr,
	&od_cpu1_micro_up_threshold_attr.attr,
	&od_cpu1_sampling_down_factor_attr.attr,
	&od_cpu1_ignore_nice_load_attr.attr,
	NULL,
};

static struct attribute_group ondemand1_attr_group = {
	.attrs = ondemand1_attrs,
	.name = "ondemand",
};

static struct attribute *ondemand2_attrs[] = {
	&od_cpu2_up_threshold_attr.attr,
	&od_cpu2_micro_up_threshold_attr.attr,
	&od_cpu2_sampling_down_factor_attr.attr,
	&od_cpu2_ignore_nice_load_attr.attr,
	NULL,
};

static struct attribute_group ondemand2_attr_group = {
	.attrs = ondemand2_attrs,
	.name = "ondemand",
};

static struct attribute *ondemand3_attrs[] = {
	&od_cpu3_up_threshold_attr.attr,
	&od_cpu3_micro_up_threshold_attr.attr,
	&od_cpu3_sampling_down_factor_attr.attr,
	&od_cpu3_ignore_nice_load_attr.attr,
	NULL,
};

static struct attribute_group ondemand3_attr_group = {
	.attrs = ondemand3_attrs,
	.name = "ondemand",
};

static struct attribute *conservative0_attrs[] = {
	&cs_cpu0_down_threshold_attr.attr,
	&cs_cpu0_up_threshold_attr.attr,
	&cs_cpu0_freq_step_attr.attr,
	&cs_cpu0_sampling_down_factor_attr.attr,
	&cs_cpu0_ignore_nice_load_attr.attr,
	NULL,
};

static struct attribute_group conservative0_attr_group = {
	.attrs = conservative0_attrs,
	.name = "conservative",
};

static struct attribute *conservative1_attrs[] = {
	&cs_cpu1_down_threshold_attr.attr,
	&cs_cpu1_up_threshold_attr.attr,
	&cs_cpu1_freq_step_attr.attr,
	&cs_cpu1_sampling_down_factor_attr.attr,
	&cs_cpu1_ignore_nice_load_attr.attr,
	NULL,
};

static struct attribute_group conservative1_attr_group = {
	.attrs = conservative1_attrs,
	.name = "conservative",
};

static struct attribute *conservative2_attrs[] = {
	&cs_cpu2_down_threshold_attr.attr,
	&cs_cpu2_up_threshold_attr.attr,
	&cs_cpu2_freq_step_attr.attr,
	&cs_cpu2_sampling_down_factor_attr.attr,
	&cs_cpu2_ignore_nice_load_attr.attr,
	NULL,
};

static struct attribute_group conservative2_attr_group = {
	.attrs = conservative2_attrs,
	.name = "conservative",
};

static struct attribute *conservative3_attrs[] = {
	&cs_cpu3_down_threshold_attr.attr,
	&cs_cpu3_up_threshold_attr.attr,
	&cs_cpu3_freq_step_attr.attr,
	&cs_cpu3_sampling_down_factor_attr.attr,
	&cs_cpu3_ignore_nice_load_attr.attr,
	NULL,
};

static struct attribute_group conservative3_attr_group = {
	.attrs = conservative3_attrs,
	.name = "conservative",
};

static struct attribute *schedutil0_attrs[] = {
	&su_cpu0_rate_limit_us_attr.attr,
	NULL,
};

static struct attribute_group schedutil0_attr_group = {
	.attrs = schedutil0_attrs,
	.name = "schedutil",
};

static struct attribute *schedutil1_attrs[] = {
	&su_cpu1_rate_limit_us_attr.attr,
	NULL,
};

static struct attribute_group schedutil1_attr_group = {
	.attrs = schedutil1_attrs,
	.name = "schedutil",
};

static struct attribute *schedutil2_attrs[] = {
	&su_cpu2_rate_limit_us_attr.attr,
	NULL,
};

static struct attribute_group schedutil2_attr_group = {
	.attrs = schedutil2_attrs,
	.name = "schedutil",
};

static struct attribute *schedutil3_attrs[] = {
	&su_cpu3_rate_limit_us_attr.attr,
	NULL,
};

static struct attribute_group schedutil3_attr_group = {
	.attrs = schedutil3_attrs,
	.name = "schedutil",
};

static struct kobject *mx_cpufreq_kobj;

static struct kobject *mx_cpu0_kobj;
static struct kobject *mx_cpu1_kobj;
static struct kobject *mx_cpu2_kobj;
static struct kobject *mx_cpu3_kobj;

static int __init cpufreq_mx_attr_init(void)
{
	int ret;

	mx_cpufreq_kobj = kobject_create_and_add("mx_cpufreq", mx_kobj);
	mx_cpu0_kobj = kobject_create_and_add("cpu0", mx_cpufreq_kobj);
	mx_cpu1_kobj = kobject_create_and_add("cpu1", mx_cpufreq_kobj);
	mx_cpu2_kobj = kobject_create_and_add("cpu2", mx_cpufreq_kobj);
	mx_cpu3_kobj = kobject_create_and_add("cpu3", mx_cpufreq_kobj);

	ret = sysfs_create_group(mx_cpu0_kobj, &ondemand0_attr_group);
	if (ret)
		pr_info("fuck\n");
	ret = sysfs_create_group(mx_cpu1_kobj, &ondemand1_attr_group);
	if (ret)
		pr_info("fuck\n");
	ret = sysfs_create_group(mx_cpu2_kobj, &ondemand2_attr_group);
	if (ret)
		pr_info("fuck\n");
	ret = sysfs_create_group(mx_cpu3_kobj, &ondemand3_attr_group);
	if (ret)
		pr_info("fuck\n");
	ret = sysfs_create_group(mx_cpu0_kobj, &conservative0_attr_group);
	if (ret)
		pr_info("fuck\n");
	ret = sysfs_create_group(mx_cpu1_kobj, &conservative1_attr_group);
	if (ret)
		pr_info("fuck\n");
	ret = sysfs_create_group(mx_cpu2_kobj, &conservative2_attr_group);
	if (ret)
		pr_info("fuck\n");
	ret = sysfs_create_group(mx_cpu3_kobj, &conservative3_attr_group);
	if (ret)
		pr_info("fuck\n");
	ret = sysfs_create_group(mx_cpu0_kobj, &schedutil0_attr_group);
	if (ret)
		pr_info("fuck\n");
	ret = sysfs_create_group(mx_cpu1_kobj, &schedutil1_attr_group);
	if (ret)
		pr_info("fuck\n");
	ret = sysfs_create_group(mx_cpu2_kobj, &schedutil2_attr_group);
	if (ret)
		pr_info("fuck\n");
	ret = sysfs_create_group(mx_cpu3_kobj, &schedutil3_attr_group);
	if (ret)
		pr_info("fuck\n");

	return ret;
}
core_initcall(cpufreq_mx_attr_init);
