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

/* Machinex OnDemand and Conservative Tunables */
unsigned int dbs_cpu_sampling_rate[NR_CPUS] = { 1000, 1000, 1000, 1000 };
unsigned int dbs_up_threshold[NR_CPUS] = { 80, 80, 80, 80 };
unsigned int dbs_micro_up_threshold[NR_CPUS] = { 95, 95, 95, 95 };
unsigned int dbs_sampling_down_factor[NR_CPUS] = { 1, 1, 1, 1 };
unsigned int dbs_ignore_nice_load[NR_CPUS] = { 0, 0, 0, 0 };
unsigned int dbs_down_threshold[NR_CPUS] = { 20, 20, 20, 20 };
unsigned int dbs_freq_step[NR_CPUS] = { 5, 5, 5, 5};
/* Machinex SchedUtil tunable */
unsigned int su_rate_limit_us[NR_CPUS] = { 1000, 1000, 1000, 1000 };

/*Machinex Interactive tunables */
unsigned int iactive_hispeed_freq[NR_CPUS] = {1782000, 1782000, 1782000, 1782000};
unsigned long iactive_go_hispeed_load[NR_CPUS] = {99, 99, 99, 99};
int iactive_target_load[NR_CPUS] = {90, 90, 90, 90};

/*Machinex Intelliactive tunable*/
unsigned int two_phase_freq[NR_CPUS] = {1674000, 1674000, 1674000, 1674000};
unsigned int up_threshold_any_cpu_load[NR_CPUS] = {80, 80, 80, 80};
unsigned int sync_freq[NR_CPUS] = {1026000, 1026000, 1026000, 1026000};
unsigned int up_threshold_any_cpu_freq[NR_CPUS] = {1242000, 1242000, 1242000, 1242000};

show_one_cpu0(dbs_cpu_sampling_rate);
show_one_cpu0(dbs_up_threshold);
show_one_cpu0(dbs_micro_up_threshold);
show_one_cpu0(dbs_sampling_down_factor);
show_one_cpu0(dbs_ignore_nice_load);
show_one_cpu0(dbs_down_threshold);
show_one_cpu0(dbs_freq_step);
show_one_cpu0(su_rate_limit_us);
show_one_cpu0(iactive_hispeed_freq);
show_one_long_cpu0(iactive_go_hispeed_load);
show_one_cpu0(iactive_target_load);
show_one_cpu0(two_phase_freq);
show_one_cpu0(up_threshold_any_cpu_load);
show_one_cpu0(sync_freq);
show_one_cpu0(up_threshold_any_cpu_freq);

show_one_cpu1(dbs_cpu_sampling_rate);
show_one_cpu1(dbs_up_threshold);
show_one_cpu1(dbs_micro_up_threshold);
show_one_cpu1(dbs_sampling_down_factor);
show_one_cpu1(dbs_ignore_nice_load);
show_one_cpu1(dbs_down_threshold);
show_one_cpu1(dbs_freq_step);
show_one_cpu1(su_rate_limit_us);
show_one_cpu1(iactive_hispeed_freq);
show_one_long_cpu1(iactive_go_hispeed_load);
show_one_cpu1(iactive_target_load);
show_one_cpu1(two_phase_freq);
show_one_cpu1(up_threshold_any_cpu_load);
show_one_cpu1(sync_freq);
show_one_cpu1(up_threshold_any_cpu_freq);

show_one_cpu2(dbs_cpu_sampling_rate);
show_one_cpu2(dbs_up_threshold);
show_one_cpu2(dbs_micro_up_threshold);
show_one_cpu2(dbs_sampling_down_factor);
show_one_cpu2(dbs_ignore_nice_load);
show_one_cpu2(dbs_down_threshold);
show_one_cpu2(dbs_freq_step);
show_one_cpu2(su_rate_limit_us);
show_one_cpu2(iactive_hispeed_freq);
show_one_long_cpu2(iactive_go_hispeed_load);
show_one_cpu2(iactive_target_load);
show_one_cpu2(two_phase_freq);
show_one_cpu2(up_threshold_any_cpu_load);
show_one_cpu2(sync_freq);
show_one_cpu2(up_threshold_any_cpu_freq);

show_one_cpu3(dbs_cpu_sampling_rate);
show_one_cpu3(dbs_up_threshold);
show_one_cpu3(dbs_micro_up_threshold);
show_one_cpu3(dbs_sampling_down_factor);
show_one_cpu3(dbs_ignore_nice_load);
show_one_cpu3(dbs_down_threshold);
show_one_cpu3(dbs_freq_step);
show_one_cpu3(su_rate_limit_us);
show_one_cpu3(iactive_hispeed_freq);
show_one_long_cpu3(iactive_go_hispeed_load);
show_one_cpu3(iactive_target_load);
show_one_cpu3(two_phase_freq);
show_one_cpu3(up_threshold_any_cpu_load);
show_one_cpu3(sync_freq);
show_one_cpu3(up_threshold_any_cpu_freq);

store_one_cpu0_clamp(dbs_cpu_sampling_rate, 1000, 10000);
store_one_cpu0_clamp(dbs_up_threshold, 1, 99);
store_one_cpu0_clamp(dbs_micro_up_threshold, 1, 99);
store_one_cpu0_clamp(dbs_sampling_down_factor, 1, 100000);
store_one_cpu0_clamp(dbs_ignore_nice_load, 0, 1);
store_one_cpu0_clamp(dbs_down_threshold, 1, 99);
store_one_cpu0_clamp(dbs_freq_step, 1, 5);
store_one_cpu0_clamp(su_rate_limit_us, 1000, 10000);
store_one_cpu0_clamp(iactive_hispeed_freq, 384000, 1890000);
store_one_long_cpu0_clamp(iactive_go_hispeed_load, 1, 99);
store_one_cpu0_clamp(iactive_target_load, 1, 99);
store_one_cpu0_clamp(two_phase_freq, 0, 1890000);
store_one_cpu0_clamp(up_threshold_any_cpu_load, 1, 80);
store_one_cpu0_clamp(sync_freq, 0, 1890000);
store_one_cpu0_clamp(up_threshold_any_cpu_freq, 0, 1890000);

store_one_cpu1_clamp(dbs_cpu_sampling_rate, 1000, 10000);
store_one_cpu1_clamp(dbs_up_threshold, 1, 99);
store_one_cpu1_clamp(dbs_micro_up_threshold, 1, 99);
store_one_cpu1_clamp(dbs_sampling_down_factor, 1, 100000);
store_one_cpu1_clamp(dbs_ignore_nice_load, 0, 1);
store_one_cpu1_clamp(dbs_down_threshold, 1, 99);
store_one_cpu1_clamp(dbs_freq_step, 1, 5);
store_one_cpu1_clamp(su_rate_limit_us, 1000, 10000);
store_one_cpu1_clamp(iactive_hispeed_freq, 384000, 1890000);
store_one_long_cpu1_clamp(iactive_go_hispeed_load, 1, 99);
store_one_cpu1_clamp(iactive_target_load, 1, 99);
store_one_cpu1_clamp(two_phase_freq, 0, 1890000);
store_one_cpu1_clamp(up_threshold_any_cpu_load, 1, 80);
store_one_cpu1_clamp(sync_freq, 0, 1890000);
store_one_cpu1_clamp(up_threshold_any_cpu_freq, 0, 1890000);

store_one_cpu2_clamp(dbs_cpu_sampling_rate, 1000, 10000);
store_one_cpu2_clamp(dbs_up_threshold, 1, 99);
store_one_cpu2_clamp(dbs_micro_up_threshold, 1, 99);
store_one_cpu2_clamp(dbs_sampling_down_factor, 1, 100000);
store_one_cpu2_clamp(dbs_ignore_nice_load, 0, 1);
store_one_cpu2_clamp(dbs_down_threshold, 1, 99);
store_one_cpu2_clamp(dbs_freq_step, 1, 5);
store_one_cpu2_clamp(su_rate_limit_us, 1000, 10000);
store_one_cpu2_clamp(iactive_hispeed_freq, 384000, 1890000);
store_one_long_cpu2_clamp(iactive_go_hispeed_load, 1, 99);
store_one_cpu2_clamp(iactive_target_load, 1, 99);
store_one_cpu2_clamp(two_phase_freq, 0, 1890000);
store_one_cpu2_clamp(up_threshold_any_cpu_load, 1, 80);
store_one_cpu2_clamp(sync_freq, 0, 1890000);
store_one_cpu2_clamp(up_threshold_any_cpu_freq, 0, 1890000);

store_one_cpu3_clamp(dbs_cpu_sampling_rate, 1000, 10000);
store_one_cpu3_clamp(dbs_up_threshold, 1, 99);
store_one_cpu3_clamp(dbs_micro_up_threshold, 1, 99);
store_one_cpu3_clamp(dbs_sampling_down_factor, 1, 100000);
store_one_cpu3_clamp(dbs_ignore_nice_load, 0, 1);
store_one_cpu3_clamp(dbs_down_threshold, 1, 99);
store_one_cpu3_clamp(dbs_freq_step, 1, 5);
store_one_cpu3_clamp(su_rate_limit_us, 1000, 10000);
store_one_cpu3_clamp(iactive_hispeed_freq, 384000, 1890000);
store_one_long_cpu3_clamp(iactive_go_hispeed_load, 1, 99);
store_one_cpu3_clamp(iactive_target_load, 1, 99);
store_one_cpu3_clamp(two_phase_freq, 0, 1890000);
store_one_cpu3_clamp(up_threshold_any_cpu_load, 1, 80);
store_one_cpu3_clamp(sync_freq, 0, 1890000);
store_one_cpu3_clamp(up_threshold_any_cpu_freq, 0, 1890000);

#define MX_CPU0_ATTR_RW(_name) \
static struct kobj_attribute cpu0_##_name##_attr = \
	__ATTR(cpu0_##_name, 0644, show_cpu0_##_name, store_cpu0_##_name)

#define MX_CPU1_ATTR_RW(_name) \
static struct kobj_attribute cpu1_##_name##_attr = \
	__ATTR(cpu1_##_name, 0644, show_cpu1_##_name, store_cpu1_##_name)

#define MX_CPU2_ATTR_RW(_name) \
static struct kobj_attribute cpu2_##_name##_attr = \
	__ATTR(cpu2_##_name, 0644, show_cpu2_##_name, store_cpu2_##_name)

#define MX_CPU3_ATTR_RW(_name) \
static struct kobj_attribute cpu3_##_name##_attr = \
	__ATTR(cpu3_##_name, 0644, show_cpu3_##_name, store_cpu3_##_name)
MX_CPU0_ATTR_RW(dbs_cpu_sampling_rate);
MX_CPU0_ATTR_RW(dbs_up_threshold);
MX_CPU0_ATTR_RW(dbs_micro_up_threshold);
MX_CPU0_ATTR_RW(dbs_sampling_down_factor);
MX_CPU0_ATTR_RW(dbs_ignore_nice_load);
MX_CPU0_ATTR_RW(dbs_down_threshold);
MX_CPU0_ATTR_RW(dbs_freq_step);
MX_CPU0_ATTR_RW(su_rate_limit_us);
MX_CPU0_ATTR_RW(iactive_hispeed_freq);
MX_CPU0_ATTR_RW(iactive_go_hispeed_load);
MX_CPU0_ATTR_RW(iactive_target_load);
MX_CPU0_ATTR_RW(two_phase_freq);
MX_CPU0_ATTR_RW(up_threshold_any_cpu_load);
MX_CPU0_ATTR_RW(sync_freq);
MX_CPU0_ATTR_RW(up_threshold_any_cpu_freq);

MX_CPU1_ATTR_RW(dbs_cpu_sampling_rate);
MX_CPU1_ATTR_RW(dbs_up_threshold);
MX_CPU1_ATTR_RW(dbs_micro_up_threshold);
MX_CPU1_ATTR_RW(dbs_sampling_down_factor);
MX_CPU1_ATTR_RW(dbs_ignore_nice_load);
MX_CPU1_ATTR_RW(dbs_down_threshold);
MX_CPU1_ATTR_RW(dbs_freq_step);
MX_CPU1_ATTR_RW(su_rate_limit_us);
MX_CPU1_ATTR_RW(iactive_hispeed_freq);
MX_CPU1_ATTR_RW(iactive_go_hispeed_load);
MX_CPU1_ATTR_RW(iactive_target_load);
MX_CPU1_ATTR_RW(two_phase_freq);
MX_CPU1_ATTR_RW(up_threshold_any_cpu_load);
MX_CPU1_ATTR_RW(sync_freq);
MX_CPU1_ATTR_RW(up_threshold_any_cpu_freq);

MX_CPU2_ATTR_RW(dbs_cpu_sampling_rate);
MX_CPU2_ATTR_RW(dbs_up_threshold);
MX_CPU2_ATTR_RW(dbs_micro_up_threshold);
MX_CPU2_ATTR_RW(dbs_sampling_down_factor);
MX_CPU2_ATTR_RW(dbs_ignore_nice_load);
MX_CPU2_ATTR_RW(dbs_down_threshold);
MX_CPU2_ATTR_RW(dbs_freq_step);
MX_CPU2_ATTR_RW(su_rate_limit_us);
MX_CPU2_ATTR_RW(iactive_hispeed_freq);
MX_CPU2_ATTR_RW(iactive_go_hispeed_load);
MX_CPU2_ATTR_RW(iactive_target_load);
MX_CPU2_ATTR_RW(two_phase_freq);
MX_CPU2_ATTR_RW(up_threshold_any_cpu_load);
MX_CPU2_ATTR_RW(sync_freq);
MX_CPU2_ATTR_RW(up_threshold_any_cpu_freq);

MX_CPU3_ATTR_RW(dbs_cpu_sampling_rate);
MX_CPU3_ATTR_RW(dbs_up_threshold);
MX_CPU3_ATTR_RW(dbs_micro_up_threshold);
MX_CPU3_ATTR_RW(dbs_sampling_down_factor);
MX_CPU3_ATTR_RW(dbs_ignore_nice_load);
MX_CPU3_ATTR_RW(dbs_down_threshold);
MX_CPU3_ATTR_RW(dbs_freq_step);
MX_CPU3_ATTR_RW(su_rate_limit_us);
MX_CPU3_ATTR_RW(iactive_hispeed_freq);
MX_CPU3_ATTR_RW(iactive_go_hispeed_load);
MX_CPU3_ATTR_RW(iactive_target_load);
MX_CPU3_ATTR_RW(two_phase_freq);
MX_CPU3_ATTR_RW(up_threshold_any_cpu_load);
MX_CPU3_ATTR_RW(sync_freq);
MX_CPU3_ATTR_RW(up_threshold_any_cpu_freq);

static struct attribute *cpu0_attrs[] = {
	&cpu0_dbs_cpu_sampling_rate_attr.attr,
	&cpu0_dbs_up_threshold_attr.attr,
	&cpu0_dbs_micro_up_threshold_attr.attr,
	&cpu0_dbs_sampling_down_factor_attr.attr,
	&cpu0_dbs_ignore_nice_load_attr.attr,
	&cpu0_dbs_down_threshold_attr.attr,
	&cpu0_dbs_freq_step_attr.attr,
	&cpu0_su_rate_limit_us_attr.attr,
	&cpu0_iactive_hispeed_freq_attr.attr,
	&cpu0_iactive_go_hispeed_load_attr.attr,
	&cpu0_iactive_target_load_attr.attr,
	&cpu0_two_phase_freq_attr.attr,
	&cpu0_up_threshold_any_cpu_load_attr.attr,
	&cpu0_sync_freq_attr.attr,
	&cpu0_up_threshold_any_cpu_freq_attr.attr,
	NULL,
};

static struct attribute *cpu1_attrs[] = {
	&cpu1_dbs_cpu_sampling_rate_attr.attr,
	&cpu1_dbs_up_threshold_attr.attr,
	&cpu1_dbs_micro_up_threshold_attr.attr,
	&cpu1_dbs_sampling_down_factor_attr.attr,
	&cpu1_dbs_ignore_nice_load_attr.attr,
	&cpu1_dbs_down_threshold_attr.attr,
	&cpu1_dbs_freq_step_attr.attr,
	&cpu1_su_rate_limit_us_attr.attr,
	&cpu1_iactive_hispeed_freq_attr.attr,
	&cpu1_iactive_go_hispeed_load_attr.attr,
	&cpu1_iactive_target_load_attr.attr,
	&cpu1_two_phase_freq_attr.attr,
	&cpu1_up_threshold_any_cpu_load_attr.attr,
	&cpu1_sync_freq_attr.attr,
	&cpu1_up_threshold_any_cpu_freq_attr.attr,
	NULL,
};

static struct attribute *cpu2_attrs[] = {
	&cpu2_dbs_cpu_sampling_rate_attr.attr,
	&cpu2_dbs_up_threshold_attr.attr,
	&cpu2_dbs_micro_up_threshold_attr.attr,
	&cpu2_dbs_sampling_down_factor_attr.attr,
	&cpu2_dbs_ignore_nice_load_attr.attr,
	&cpu2_dbs_down_threshold_attr.attr,
	&cpu2_dbs_freq_step_attr.attr,
	&cpu2_su_rate_limit_us_attr.attr,
	&cpu2_iactive_hispeed_freq_attr.attr,
	&cpu2_iactive_go_hispeed_load_attr.attr,
	&cpu2_iactive_target_load_attr.attr,
	&cpu2_two_phase_freq_attr.attr,
	&cpu2_up_threshold_any_cpu_load_attr.attr,
	&cpu2_sync_freq_attr.attr,
	&cpu2_up_threshold_any_cpu_freq_attr.attr,
	NULL,
};

static struct attribute *cpu3_attrs[] = {
	&cpu3_dbs_cpu_sampling_rate_attr.attr,
	&cpu3_dbs_up_threshold_attr.attr,
	&cpu3_dbs_micro_up_threshold_attr.attr,
	&cpu3_dbs_sampling_down_factor_attr.attr,
	&cpu3_dbs_ignore_nice_load_attr.attr,
	&cpu3_dbs_down_threshold_attr.attr,
	&cpu3_dbs_freq_step_attr.attr,
	&cpu3_su_rate_limit_us_attr.attr,
	&cpu3_iactive_hispeed_freq_attr.attr,
	&cpu3_iactive_go_hispeed_load_attr.attr,
	&cpu3_iactive_target_load_attr.attr,
	&cpu3_two_phase_freq_attr.attr,
	&cpu3_up_threshold_any_cpu_load_attr.attr,
	&cpu3_sync_freq_attr.attr,
	&cpu3_up_threshold_any_cpu_freq_attr.attr,
	NULL,
};

static struct attribute_group cpu0_attr_group = {
	.attrs = cpu0_attrs,
	.name = "cpu0",
};

static struct attribute_group cpu1_attr_group = {
	.attrs = cpu1_attrs,
	.name = "cpu1",
};

static struct attribute_group cpu2_attr_group = {
	.attrs = cpu2_attrs,
	.name = "cpu2",
};

static struct attribute_group cpu3_attr_group = {
	.attrs = cpu3_attrs,
	.name = "cpu3",
};

static struct kobject *mx_cpufreq_kobj;

static int __init cpufreq_mx_attr_init(void)
{
	int ret;

	mx_cpufreq_kobj = kobject_create_and_add("mx_cpufreq", mx_kobj);

	ret = sysfs_create_group(mx_cpufreq_kobj, &cpu0_attr_group);
	if (ret)
		pr_info("fuck\n");

	ret = sysfs_create_group(mx_cpufreq_kobj, &cpu1_attr_group);
	if (ret)
		pr_info("fuck\n");

	ret = sysfs_create_group(mx_cpufreq_kobj, &cpu2_attr_group);
	if (ret)
		pr_info("fuck\n");

	ret = sysfs_create_group(mx_cpufreq_kobj, &cpu3_attr_group);
	if (ret)
		pr_info("fuck\n");

	return ret;
}
core_initcall(cpufreq_mx_attr_init);
