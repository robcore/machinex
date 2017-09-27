/* drivers/cpufreq/cpufreq_machinex_gov_attr.h
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

#ifndef _CPUFREQ_MACHINEX_GOV_ATTR_H_
#define _CPUFREQ_MACHINEX_GOV_ATTR_H_

/**
 * I have a very specific, hacked kernel configuration.
 * There is a 110% chance that this not only won't work
 * for you, but cause headaches that aren't worth it.
 * Please ask me anything you need to know and I'll fill
 * you in, as I'd rather help you than see people get
 * screwed by my lack of organization.  This is NOT one
 * of those whiny, "ask for permission" rants. This is
 * my concern for your hardware, and sanity, as my drivers
 * are built by a crazy person (me).
 */

/* Machinex OnDemand Tunables */
unsigned int od_cpu0_sampling_rate;
unsigned int od_cpu0_up_threshold;
unsigned int od_cpu0_sampling_down_factor;
unsigned int od_cpu0_ignore_nice_load;
unsigned int od_cpu0_io_is_busy;
unsigned int od_cpu0_powersave_bias;

unsigned int od_cpu1_sampling_rate;
unsigned int od_cpu1_up_threshold;
unsigned int od_cpu1_sampling_down_factor;
unsigned int od_cpu1_ignore_nice_load;
unsigned int od_cpu1_io_is_busy;
unsigned int od_cpu1_powersave_bias;

unsigned int od_cpu2_sampling_rate;
unsigned int od_cpu2_up_threshold;
unsigned int od_cpu2_sampling_down_factor;
unsigned int od_cpu2_ignore_nice_load;
unsigned int od_cpu2_io_is_busy;
unsigned int od_cpu2_powersave_bias;

unsigned int od_cpu3_sampling_rate;
unsigned int od_cpu3_up_threshold;
unsigned int od_cpu3_sampling_down_factor;
unsigned int od_cpu3_ignore_nice_load;
unsigned int od_cpu3_io_is_busy;
unsigned int od_cpu3_powersave_bias;

/* Machinex Conservative Tunables */
unsigned int cs_cpu0_down_threshold;
unsigned int cs_cpu0_up_threshold;
unsigned int cs_cpu0_freq_step;
unsigned int cs_cpu0_sampling_down_factor;
unsigned int cs_cpu0_ignore_nice_load;

unsigned int cs_cpu1_down_threshold;
unsigned int cs_cpu1_up_threshold;
unsigned int cs_cpu1_freq_step;
unsigned int cs_cpu1_sampling_down_factor;
unsigned int cs_cpu1_ignore_nice_load;

unsigned int cs_cpu2_down_threshold;
unsigned int cs_cpu2_up_threshold;
unsigned int cs_cpu2_freq_step;
unsigned int cs_cpu2_sampling_down_factor;
unsigned int cs_cpu2_ignore_nice_load;

unsigned int cs_cpu3_down_threshold;
unsigned int cs_cpu3_up_threshold;
unsigned int cs_cpu3_freq_step;
unsigned int cs_cpu3_sampling_down_factor;
unsigned int cs_cpu3_ignore_nice_load;

/* Machinex SchedUtil tunable */
unsigned int su_cpu0_rate_limit_us;
unsigned int su_cpu1_rate_limit_us;
unsigned int su_cpu2_rate_limit_us;
unsigned int su_cpu3_rate_limit_us;


