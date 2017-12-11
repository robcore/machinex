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
#include <linux/cpu.h>

unsigned int mx_cpufreq_governor[NR_CPUS] = {0, 0, 0, 0};
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
unsigned long iactive_go_hispeed_load[NR_CPUS] = {90, 90, 90, 90};
unsigned int iactive_target_load[NR_CPUS] = {85, 85, 85, 85};

/*Machinex Intelliactive tunable*/
unsigned int two_phase_freq[NR_CPUS] = {1674000, 1674000, 1674000, 1674000};
unsigned int up_threshold_any_cpu_load[NR_CPUS] = {80, 80, 80, 80};
unsigned int sync_freq[NR_CPUS] = {1026000, 1026000, 1026000, 1026000};
unsigned int up_threshold_any_cpu_freq[NR_CPUS] = {702000, 702000, 702000, 702000};
extern unsigned int iactive_load_debug;
extern unsigned int iactive_current_load[NR_CPUS];
extern unsigned int iactive_max_load[NR_CPUS];
extern unsigned int iactive_choose_freq[NR_CPUS];
extern unsigned int iactive_raw_loadadjfreq[NR_CPUS];
extern unsigned int iactive_load_over_target[NR_CPUS];

show_one_cpu(dbs_cpu_sampling_rate);
show_one_cpu(dbs_up_threshold);
show_one_cpu(dbs_micro_up_threshold);
show_one_cpu(dbs_sampling_down_factor);
show_one_cpu(dbs_ignore_nice_load);
show_one_cpu(dbs_down_threshold);
show_one_cpu(dbs_freq_step);
show_one_cpu(su_rate_limit_us);
show_one_cpu(iactive_hispeed_freq);
show_one_long_cpu(iactive_go_hispeed_load);
show_one_cpu(iactive_target_load);
show_one_cpu(two_phase_freq);
show_one_cpu(up_threshold_any_cpu_load);
show_one_cpu(sync_freq);
show_one_cpu(up_threshold_any_cpu_freq);
show_one_cpu(mx_cpufreq_governor);

static ssize_t iactive_current_load_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	if (iactive_load_debug)
		return sprintf(buf, "%u\n", iactive_current_load[(dev->id)]);
	return sprintf(buf, "Disabled\n");
}
static ssize_t iactive_max_load_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", iactive_max_load[(dev->id)]);
}

static ssize_t iactive_choose_freq_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	if (iactive_load_debug)
		return sprintf(buf, "%u\n", iactive_choose_freq[(dev->id)]);
	return sprintf(buf, "Disabled\n");
}

static ssize_t iactive_raw_loadadjfreq_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	if (iactive_load_debug)
		return sprintf(buf, "%u\n", iactive_raw_loadadjfreq[(dev->id)]);
	return sprintf(buf, "Disabled\n");
}

static ssize_t iactive_load_over_target_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	if (iactive_load_debug)
		return sprintf(buf, "%u\n", iactive_load_over_target[(dev->id)]);
	return sprintf(buf, "Disabled\n");
}

store_one_cpu_clamp(dbs_cpu_sampling_rate, 1000, 10000);
store_one_cpu_clamp(dbs_up_threshold, 1, 99);
store_one_cpu_clamp(dbs_micro_up_threshold, 1, 99);
store_one_cpu_clamp(dbs_sampling_down_factor, 1, 100000);
store_one_cpu_clamp(dbs_ignore_nice_load, 0, 1);
store_one_cpu_clamp(dbs_down_threshold, 1, 99);
store_one_cpu_clamp(dbs_freq_step, 1, 5);
store_one_cpu_clamp(su_rate_limit_us, 1000, 10000);
store_one_cpu_clamp(iactive_hispeed_freq, 384000, 1890000);
store_one_long_cpu_clamp(iactive_go_hispeed_load, 1, 100);
store_one_cpu_clamp(iactive_target_load, 1, 100);
store_one_cpu_clamp(two_phase_freq, 0, 1890000);
store_one_cpu_clamp(up_threshold_any_cpu_load, 1, 99);
store_one_cpu_clamp(sync_freq, 0, 1890000);
store_one_cpu_clamp(up_threshold_any_cpu_freq, 0, 1890000);

static ssize_t mx_cpufreq_governor_store
(struct device *dev,
struct device_attribute *attr,
const char *buf, size_t count)
{
	unsigned int input;
	int ret;

	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	if (input == mx_cpufreq_governor[dev->id])
		return count;

	sanitize_min_max(input, 0, 7);

	mx_cpufreq_governor[dev->id] = input;

	if (cpu_online(dev->id))
		mx_update_policy(dev->id);

	return count;
}

DEVICE_ATTR_RW(dbs_cpu_sampling_rate);
DEVICE_ATTR_RW(dbs_up_threshold);
DEVICE_ATTR_RW(dbs_micro_up_threshold);
DEVICE_ATTR_RW(dbs_sampling_down_factor);
DEVICE_ATTR_RW(dbs_ignore_nice_load);
DEVICE_ATTR_RW(dbs_down_threshold);
DEVICE_ATTR_RW(dbs_freq_step);
DEVICE_ATTR_RW(su_rate_limit_us);
DEVICE_ATTR_RW(iactive_hispeed_freq);
DEVICE_ATTR_RW(iactive_go_hispeed_load);
DEVICE_ATTR_RW(iactive_target_load);
DEVICE_ATTR_RW(two_phase_freq);
DEVICE_ATTR_RW(up_threshold_any_cpu_load);
DEVICE_ATTR_RW(sync_freq);
DEVICE_ATTR_RW(up_threshold_any_cpu_freq);
DEVICE_ATTR_RW(mx_cpufreq_governor);
DEVICE_ATTR_RO(iactive_current_load);
DEVICE_ATTR_RO(iactive_max_load);
DEVICE_ATTR_RO(iactive_choose_freq);
DEVICE_ATTR_RO(iactive_raw_loadadjfreq);
DEVICE_ATTR_RO(iactive_load_over_target);

static struct attribute *mx_cpu_attrs[] = {
	&dev_attr_dbs_cpu_sampling_rate.attr,
	&dev_attr_dbs_up_threshold.attr,
	&dev_attr_dbs_micro_up_threshold.attr,
	&dev_attr_dbs_sampling_down_factor.attr,
	&dev_attr_dbs_ignore_nice_load.attr,
	&dev_attr_dbs_down_threshold.attr,
	&dev_attr_dbs_freq_step.attr,
	&dev_attr_su_rate_limit_us.attr,
	&dev_attr_iactive_hispeed_freq.attr,
	&dev_attr_iactive_go_hispeed_load.attr,
	&dev_attr_iactive_target_load.attr,
	&dev_attr_two_phase_freq.attr,
	&dev_attr_up_threshold_any_cpu_load.attr,
	&dev_attr_sync_freq.attr,
	&dev_attr_up_threshold_any_cpu_freq.attr,
	&dev_attr_mx_cpufreq_governor.attr,
	&dev_attr_iactive_current_load.attr,
	&dev_attr_iactive_max_load.attr,
	&dev_attr_iactive_choose_freq.attr,
	&dev_attr_iactive_raw_loadadjfreq.attr,
	&dev_attr_iactive_load_over_target.attr,
	NULL,
};

static struct attribute_group mx_cpufreq_attr_group = {
	.attrs = mx_cpu_attrs,
	.name = "mx_cpufreq"
};

static void mx_cpufreq_add_dev(unsigned int cpu)
{
	struct device *dev = get_cpu_device(cpu);
	WARN_ON_ONCE(sysfs_create_group(&dev->kobj, &mx_cpufreq_attr_group));
}

static int __init cpufreq_mx_attr_init(void)
{
	unsigned int cpu;

	for_each_possible_cpu(cpu) {
		if (cpu_out_of_range(cpu))
			break;
		mx_cpufreq_add_dev(cpu);
	}
	return 0;
}
late_initcall(cpufreq_mx_attr_init);