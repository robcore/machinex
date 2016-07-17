/*
 * Copyright (c) 2015, Emmanuel Utomi <emmanuelutomi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/cpufreq.h>

#ifndef LINUX_RETAIN_CPUPOLICY_H
#define LINUX_RETAIN_CPUPOLICY_H

struct cpufreq_retain_policy {
	unsigned int min;
	unsigned int max;
	struct cpufreq_governor* governor;
};

enum cpufreq_restore_flags {
	CPUFREQ_RESTORE_ALL,
	CPUFREQ_RESTORE_GOVERNOR,
	CPUFREQ_RESTORE_FREQ,
};

#ifdef CONFIG_CPU_FREQ_RETAIN_POLICY
void retain_cpu_policy(struct cpufreq_policy *policy);
void restore_cpu_policy(struct cpufreq_policy *policy, enum cpufreq_restore_flags);
bool retained_cpu_policy(int cpu);
bool sync_retained_cpu_policy(void);
unsigned int get_retained_min_cpu_freq(int cpu);
unsigned int get_retained_max_cpu_freq(int cpu);
struct cpufreq_governor* get_retained_governor(int cpu);
#else
static inline void retain_cpu_policy(struct cpufreq_policy *policy) {}
static inline void restore_cpu_policy(struct cpufreq_policy *policy, enum cpufreq_restore_flags) {}
static inline bool retained_cpu_policy(int cpu) { return false; }
static inline bool sync_retained_cpu_policy() { return false; }
static inline unsigned int get_retained_min_cpu_freq(int cpu) { return 0; }
static inline unsigned int get_retained_max_cpu_freq(int cpu) { return 0; }
static inline struct cpufreq_governor* get_retained_governor(int cpu) { return NULL; }
#endif

#endif
