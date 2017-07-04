/*
 *  linux/include/linux/cpufreq.h
 *
 *  Copyright (C) 2001 Russell King
 *            (C) 2002 - 2003 Dominik Brodowski <linux@brodo.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef _LINUX_FAKE_DVFS_H
#define _LINUX_FAKE_DVFS_H

enum {
	BOOT_CPU = 0,
};

int get_max_freq(void);
int get_min_freq(void);

#define FAKEFREQ_ENTRY_INVALID ~0
#define FAKEFREQ_TABLE_END     ~1
#define MSM_CPUFREQ_NO_LIMIT 0xFFFFFFFF
#define MAX_FREQ_LIMIT		get_max_freq() /* 1890000 */
#define MIN_FREQ_LIMIT		get_min_freq() /* 384000 */

#define MIN_TOUCH_LIMIT		1134000
#define MIN_TOUCH_HIGH_LIMIT		1890000
#define MIN_TOUCH_LIMIT_SECOND	810000
#define MIN_TOUCH_HIGH_LIMIT_SECOND	1566000

#define MAX_UNICPU_LIMIT	1242000
#define UPDATE_NOW_BITS		0xFF

enum {
	DVFS_NO_ID			= 0,

	/* need to update now */
	DVFS_TOUCH_ID			= 0x00000001,
	DVFS_APPS_MIN_ID		= 0x00000002,
	DVFS_APPS_MAX_ID		= 0x00000004,
	DVFS_UNICPU_ID			= 0x00000008,

	/* DO NOT UPDATE NOW */
	DVFS_THERMALD_ID		= 0x00000100,

	DVFS_MAX_ID
};


int set_freq_limit(unsigned long id, unsigned int freq);

unsigned int get_min_lock(void);
unsigned int get_max_lock(void);
void set_min_lock(int freq);
void set_max_lock(int freq);
#endif