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
#include <linux/device.h>
#include <linux/sysfs.h>

/**
 * I have a very specific, hacked kernel configuration.
 * There is a 110% chance that this not only won't work
 * for you, but will cause headaches that aren't worth it.
 * Please ask me anything you need to know and I'll fill
 * you in, as I'd rather help you than see people get
 * screwed by my lack of organization.  This is NOT one
 * of those whiny, "ask for permission" rants. This is
 * my concern for your hardware, and sanity, as my drivers
 * are built by a crazy person (me).
 */

extern unsigned int mx_cpufreq_governor[NR_CPUS];

extern unsigned int dbs_cpu_sampling_rate[NR_CPUS];
extern unsigned int dbs_up_threshold[NR_CPUS];
extern unsigned int dbs_micro_up_threshold[NR_CPUS];
extern unsigned int dbs_sampling_down_factor[NR_CPUS];
extern unsigned int dbs_ignore_nice_load[NR_CPUS];
extern unsigned int dbs_down_threshold[NR_CPUS];
extern unsigned int dbs_freq_step[NR_CPUS];
extern unsigned int su_rate_limit_us[NR_CPUS];
extern unsigned int iactive_hispeed_freq[NR_CPUS];
extern unsigned long iactive_go_hispeed_load[NR_CPUS];
extern int iactive_target_load[NR_CPUS];
extern int iactive_current_load[NR_CPUS];
extern unsigned int iactive_choose_freq[NR_CPUS];
extern unsigned int iactive_raw_loadadjfreq[NR_CPUS];
extern unsigned int two_phase_freq[NR_CPUS];
extern unsigned int up_threshold_any_cpu_load[NR_CPUS];
extern unsigned int sync_freq[NR_CPUS];
extern unsigned int up_threshold_any_cpu_freq[NR_CPUS];
#endif