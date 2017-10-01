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

#include "cpufreq_machinex_gov_attr.h"

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
unsigned int cs_cpu0_down_threshold;
unsigned int cs_cpu0_up_threshold;
unsigned int cs_cpu0_freq_step;
unsigned int cs_cpu0_sampling_down_factor;
unsigned int cs_cpu0_ignore_nice_load = 0;

unsigned int cs_cpu1_down_threshold;
unsigned int cs_cpu1_up_threshold;
unsigned int cs_cpu1_freq_step;
unsigned int cs_cpu1_sampling_down_factor;
unsigned int cs_cpu1_ignore_nice_load = 0;

unsigned int cs_cpu2_down_threshold;
unsigned int cs_cpu2_up_threshold;
unsigned int cs_cpu2_freq_step;
unsigned int cs_cpu2_sampling_down_factor;
unsigned int cs_cpu2_ignore_nice_load = 0;

unsigned int cs_cpu3_down_threshold;
unsigned int cs_cpu3_up_threshold;
unsigned int cs_cpu3_freq_step;
unsigned int cs_cpu3_sampling_down_factor;
unsigned int cs_cpu3_ignore_nice_load = 0;

/* Machinex SchedUtil tunable */
unsigned int su_cpu0_rate_limit_us;
unsigned int su_cpu1_rate_limit_us;
unsigned int su_cpu2_rate_limit_us;
unsigned int su_cpu3_rate_limit_us;

static struct kobject *mx_cpufreq_kobj;

static struct kobject *mx_cpu0_kobj;
static struct kobject *mx_cpu1_kobj;
static struct kobject *mx_cpu2_kobj;
static struct kobject *mx_cpu3_kobj;

static struct kobject *mx_ondemand0_kobj;
static struct kobject *mx_ondemand1_kobj;
static struct kobject *mx_ondemand2_kobj;
static struct kobject *mx_ondemand3_kobj;

static struct kobject *mx_cs0_kobj;
static struct kobject *mx_cs1_kobj;
static struct kobject *mx_cs2_kobj;
static struct kobject *mx_cs3_kobj;

static struct kobject *mx_su0_kobj;
static struct kobject *mx_su1_kobj;
static struct kobject *mx_su2_kobj;
static struct kobject *mx_su3_kobj;