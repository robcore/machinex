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

#define show_one_cpu0(object)				\
static ssize_t show_cpu0_##object					\
(struct kobject *kobj, struct kobj_attribute *attr, char *buf)	\
{								\
	return sprintf(buf, "%u\n", object[0]);			\
}

#define show_one_cpu1(object)				\
static ssize_t show_cpu1_##object					\
(struct kobject *kobj, struct kobj_attribute *attr, char *buf)	\
{								\
	return sprintf(buf, "%u\n", object[1]);			\
}

#define show_one_cpu2(object)				\
static ssize_t show_cpu2_##object					\
(struct kobject *kobj, struct kobj_attribute *attr, char *buf)	\
{								\
	return sprintf(buf, "%u\n", object[2]);			\
}

#define show_one_cpu3(object)				\
static ssize_t show_cpu3_##object					\
(struct kobject *kobj, struct kobj_attribute *attr, char *buf)	\
{								\
	return sprintf(buf, "%u\n", object[3]);			\
}

#define store_one_cpu0_clamp(name, min, max)		\
static ssize_t store_cpu0_##name		\
(struct kobject *kobj,				\
 struct kobj_attribute *attr,			\
 const char *buf, size_t count)			\
{						\
	unsigned int input;			\
	int ret;				\
	ret = sscanf(buf, "%u", &input);	\
	if (ret != 1)			\
		return -EINVAL;			\
	if (input == name[0])			\
		return count;			\
	if (input <= min)	\
		input = min;	\
	if (input >= max)		\
			input = max;		\
	name[0] = input;				\
	return count;				\
}

#define store_one_cpu1_clamp(name, min, max)		\
static ssize_t store_cpu1_##name		\
(struct kobject *kobj,				\
 struct kobj_attribute *attr,			\
 const char *buf, size_t count)			\
{						\
	unsigned int input;			\
	int ret;				\
	ret = sscanf(buf, "%u", &input);	\
	if (ret != 1)			\
		return -EINVAL;			\
	if (input == name[1])			\
		return count;			\
	if (input <= min)	\
		input = min;	\
	if (input >= max)		\
			input = max;		\
	name[1] = input;				\
	return count;				\
}

#define store_one_cpu2_clamp(name, min, max)		\
static ssize_t store_cpu2_##name		\
(struct kobject *kobj,				\
 struct kobj_attribute *attr,			\
 const char *buf, size_t count)			\
{						\
	unsigned int input;			\
	int ret;				\
	ret = sscanf(buf, "%u", &input);	\
	if (ret != 1)			\
		return -EINVAL;			\
	if (input == name[2])			\
		return count;			\
	if (input <= min)	\
		input = min;	\
	if (input >= max)		\
			input = max;		\
	name[2] = input;				\
	return count;				\
}

#define store_one_cpu3_clamp(name, min, max)		\
static ssize_t store_cpu3_##name		\
(struct kobject *kobj,				\
 struct kobj_attribute *attr,			\
 const char *buf, size_t count)			\
{						\
	unsigned int input;			\
	int ret;				\
	ret = sscanf(buf, "%u", &input);	\
	if (ret != 1)			\
		return -EINVAL;			\
	if (input == name[3])			\
		return count;			\
	if (input <= min)	\
		input = min;	\
	if (input >= max)		\
			input = max;		\
	name[3] = input;				\
	return count;				\
}

#define show_one_long_cpu0(object)				\
static ssize_t show_cpu0_##object					\
(struct kobject *kobj, struct kobj_attribute *attr, char *buf)	\
{								\
	return sprintf(buf, "%lu\n", object[0]);			\
}

#define show_one_long_cpu1(object)				\
static ssize_t show_cpu1_##object					\
(struct kobject *kobj, struct kobj_attribute *attr, char *buf)	\
{								\
	return sprintf(buf, "%lu\n", object[1]);			\
}

#define show_one_long_cpu2(object)				\
static ssize_t show_cpu2_##object					\
(struct kobject *kobj, struct kobj_attribute *attr, char *buf)	\
{								\
	return sprintf(buf, "%lu\n", object[2]);			\
}

#define show_one_long_cpu3(object)				\
static ssize_t show_cpu3_##object					\
(struct kobject *kobj, struct kobj_attribute *attr, char *buf)	\
{								\
	return sprintf(buf, "%lu\n", object[3]);			\
}

#define store_one_long_cpu0_clamp(name, min, max)		\
static ssize_t store_cpu0_##name		\
(struct kobject *kobj,				\
 struct kobj_attribute *attr,			\
 const char *buf, size_t count)			\
{						\
	unsigned long input;			\
	int ret;				\
	ret = sscanf(buf, "%lu", &input);	\
	if (ret != 1)			\
		return -EINVAL;			\
	if (input == name[0])			\
		return count;			\
	if (input <= min)	\
		input = min;	\
	if (input >= max)		\
			input = max;		\
	name[0] = input;				\
	return count;				\
}

#define store_one_long_cpu1_clamp(name, min, max)		\
static ssize_t store_cpu1_##name		\
(struct kobject *kobj,				\
 struct kobj_attribute *attr,			\
 const char *buf, size_t count)			\
{						\
	unsigned long input;			\
	int ret;				\
	ret = sscanf(buf, "%lu", &input);	\
	if (ret != 1)			\
		return -EINVAL;			\
	if (input == name[1])			\
		return count;			\
	if (input <= min)	\
		input = min;	\
	if (input >= max)		\
			input = max;		\
	name[1] = input;				\
	return count;				\
}

#define store_one_long_cpu2_clamp(name, min, max)		\
static ssize_t store_cpu2_##name		\
(struct kobject *kobj,				\
 struct kobj_attribute *attr,			\
 const char *buf, size_t count)			\
{						\
	unsigned long input;			\
	int ret;				\
	ret = sscanf(buf, "%lu", &input);	\
	if (ret != 1)			\
		return -EINVAL;			\
	if (input == name[2])			\
		return count;			\
	if (input <= min)	\
		input = min;	\
	if (input >= max)		\
			input = max;		\
	name[2] = input;				\
	return count;				\
}

#define store_one_long_cpu3_clamp(name, min, max)		\
static ssize_t store_cpu3_##name		\
(struct kobject *kobj,				\
 struct kobj_attribute *attr,			\
 const char *buf, size_t count)			\
{						\
	unsigned long input;			\
	int ret;				\
	ret = sscanf(buf, "%lu", &input);	\
	if (ret != 1)			\
		return -EINVAL;			\
	if (input == name[3])			\
		return count;			\
	if (input <= min)	\
		input = min;	\
	if (input >= max)		\
			input = max;		\
	name[3] = input;				\
	return count;				\
}

extern unsigned int dbs_cpu_sampling_rate[NR_CPUS];
extern unsigned int dbs_up_threshold[NR_CPUS];
extern unsigned int dbs_micro_up_threshold[NR_CPUS];
extern unsigned int dbs_sampling_down_factor[NR_CPUS];
extern unsigned int dbs_ignore_nice_load[NR_CPUS];
extern unsigned int dbs_down_threshold[NR_CPUS];
extern unsigned int dbs_freq_step[NR_CPUS];
extern unsigned int su_rate_limit_us[NR_CPUS];
/*Machinex Interactive tunables */
extern unsigned int iactive_hispeed_freq[NR_CPUS];
extern unsigned long iactive_go_hispeed_load[NR_CPUS];
extern int iactive_target_load[NR_CPUS];
#endif