/*
 * Copyright (c) 2010-2012, Code Aurora Forum. All rights reserved.
 * Copyright (c) 2015, Tom G. <roboter972@gmail.com>
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
 */

/*
 * Brief description:
 * The machinactive kgsl policy acts exactly like it's cpufreq scaling driver
 * counterpart. It attemps to scale frequency in small steps depending on the
 * current GPU load (calculated using statistics) every sampling interval.
 * On idle machinactive scales GPU frequency all the way down to reduce power
 * consumption. Compared to qualcomms's trustzone algorithm which tends to
 * inefficiently scale frequencies machinactive offers power savings and heat
 * reduction without sacraficing performance.
 */

#include <linux/export.h>
#include <linux/kernel.h>
#include <linux/stat.h>
#include "a2xx_reg.h"
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/spinlock.h>
#include <mach/socinfo.h>

#include "kgsl.h"
#include "kgsl_device.h"
#include "kgsl_pwrscale.h"

/*
 * Polling interval in us.
 */
#define MIN_POLL_INTERVAL	10000
#define POLL_INTERVAL		100000
#define MAX_POLL_INTERVAL	1000000

static unsigned long polling_interval = POLL_INTERVAL;

/*
 * Total and busytime stats used to calculate the current GPU load.
 */
static unsigned long walltime_total;
static unsigned long busytime_total;

/*
 * Load thresholds.
 */
#define MAX_LOAD		98

struct load_thresholds {
	unsigned int m_up_threshold;
	unsigned int m down_threshold;
};

static struct load_thresholds thresholds[] = {
	{UINT_MAX,	65},	/* 400 MHz @pwrlevel 0 */
	{75,		50},	/* 320 MHz @pwrlevel 1 */
	{55,		30},	/* 200 MHz @pwrlevel 2 */
	{40,		 0},	/* 128 MHz @pwrlevel 3 */
	{ 0,	 	 0}	/*  27 MHz @pwrlevel 4 */
};

static void machinactive_wake(struct kgsl_device *device,
				struct kgsl_pwrscale *pwrscale)
{
	struct kgsl_power_stats stats;

	if (device->state != KGSL_STATE_NAP) {
		/* Reset the power stats counters. */
		device->ftbl->power_stats(device, &stats);
		walltime_total = 0;
		busytime_total = 0;
	}
}

static void machinactive_idle(struct kgsl_device *device,
				struct kgsl_pwrscale *pwrscale)
{
	struct kgsl_pwrctrl *pwr = &device->pwrctrl;
	struct kgsl_power_stats stats;
	unsigned long flags;
	unsigned int load_hist;
	int val = 0;

	device->ftbl->power_stats(device, &stats);

	/*
	 * Break out early if machinactive is running in energy saving
	 * or performance mode.
	 */
	if (!stats.total_time)
		return;

	walltime_total += (unsigned long) stats.total_time;
	busytime_total += (unsigned long) stats.busy_time;

	if (walltime_total > polling_interval) {
		load_hist = (100 * busytime_total) / walltime_total;

		walltime_total = busytime_total = 0;

		if (load_hist < thresholds[pwr->active_pwrlevel].down_threshold)
			val = 1;
		else if (load_hist >
			thresholds[pwr->active_pwrlevel].up_threshold)
			val = -1;
		if (val)
			kgsl_pwrctrl_pwrlevel_change(device,
						pwr->active_pwrlevel + val);
	}
}

static ssize_t machinactive_polling_interval_show(struct kgsl_device *device,
				struct kgsl_pwrscale *pwrscale,
				char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%lu\n", polling_interval);
}

static ssize_t machinactive_polling_interval_store(struct kgsl_device *device,
				struct kgsl_pwrscale *pwrscale,
				const char *buf, size_t count)
{
	unsigned long tmp;
	int err;

	err = kstrtoul(buf, 0, &tmp);
	if (err) {
		pr_err("%s: failed setting new polling interval!\n", __func__);
		return err;
	}

	if (tmp < MIN_POLL_INTERVAL)
		tmp = MIN_POLL_INTERVAL;
	else if (tmp > MAX_POLL_INTERVAL)
		tmp = MAX_POLL_INTERVAL;

	polling_interval = tmp;

	return count;
}

PWRSCALE_POLICY_ATTR(polling_interval, 0644,
					machinactive_polling_interval_show,
					machinactive_polling_interval_store);

static ssize_t thresholds_show(struct kgsl_device *device,
				struct kgsl_pwrscale *pwrscale,
				char *buf)
{
	return sprintf(buf,
				"%u %u \t%u %u \t%u %u \t%u %u \t%u %u\n",
					thresholds[0].up_threshold,
					thresholds[0].down_threshold,
					thresholds[1].up_threshold,
					thresholds[1].down_threshold,
					thresholds[2].up_threshold,
					thresholds[2].down_threshold,
					thresholds[3].up_threshold,
					thresholds[3].down_threshold,
					thresholds[4].up_threshold,
					thresholds[4].down_threshold);
}

static ssize_t thresholds_store(struct kgsl_device *device,
				struct kgsl_pwrscale *pwrscale,
				const char *buf, size_t count)
{
	unsigned int uval[5], dval[5];
	int i, ret;

	ret = sscanf(buf, "%u %u %u %u %u %u %u %u %u %u",
					&uval[0],
					&dval[0],
					&uval[1],
					&dval[1],
					&uval[2],
					&dval[2],
					&uval[3],
					&dval[3],
					&uval[4],
					&dval[4]);

	if (ret < 1)
		ret = 1;
	if (ret > 10)
		ret = 10;

	for (i = 0; i < 5; i++) {
		/*
	 	 * Limit up_threshold to 98.
		 * Anything higher will prevent downscaling a pwrlevel.
	 	 */
		if (uval[i] >= MAX_LOAD)
			uval[i] = MAX_LOAD;

		thresholds[i].up_threshold = uval[i];
		thresholds[i].down_threshold = dval[i];
	}
	return count;
}

PWRSCALE_POLICY_ATTR(pwrlevel_thresholds, 0644,
							thresholds_show,
							thresholds_store);

static ssize_t scale_mode_show(struct kgsl_device *device,
				struct kgsl_pwrscale *pwrscale,
				char *buf)
{
	return sprintf(buf, "%u\n", scale_mode);
}

static ssize_t scale_mode_store(struct kgsl_device *device,
				struct kgsl_pwrscale *pwrscale,
				const char *buf, size_t count)
{
	struct kgsl_pwrctrl *pwr = &device->pwrctrl;
	unsigned int val;

	mutex_lock(&device->mutex);

	sscanf(buf, "%u", &val);

	if (val <= 0) {
		scale_mode = mode[0] = val;
		pr_info("[POLGOV] 0-Conservative Mode\n");
	} else if (val == 1) {
		scale_mode = mode[1] = val;
		kgsl_pwrctrl_pwrlevel_change(device, pwr->min_pwrlevel);
		pr_info("[POLGOV] 1-Energy Saving Mode\n");
	} else if (val >= 2) {
		scale_mode = mode[2] = val;
		kgsl_pwrctrl_pwrlevel_change(device, pwr->max_pwrlevel);
		pr_info("[POLGOV] 2-Machinactive Mode\n");
	} else
		scale_mode = mode[0];

	mutex_unlock(&device->mutex);

	return count;
}

PWRSCALE_POLICY_ATTR(policy_scale_mode, 0644,
							scale_mode_show,
							scale_mode_store);

static struct attribute *machinactive_attrs[] = {
	&policy_attr_polling_interval.attr,
	&policy_attr_pwrlevel_thresholds.attr,
	&policy_attr_policy_scale_mode.attr,
	NULL
};

static struct attribute_group machinactive_attr_group = {
	.attrs = machinactive_attrs,
	.name = "machinactive",
};

static int machinactive_init(struct kgsl_device *device,
				struct kgsl_pwrscale *pwrscale)
{
	scale_mode = mode[2];

	spin_lock_init(&machinactive_lock);

	kgsl_pwrscale_policy_add_files(device, pwrscale,
						&machinactive_attr_group);

	return 0;
}

static void machinactive_close(struct kgsl_device *device,
				struct kgsl_pwrscale *pwrscale)
{
	kgsl_pwrscale_policy_remove_files(device, pwrscale,
						&machinactive_attr_group);
}

struct kgsl_pwrscale_policy kgsl_pwrscale_policy_machinactive = {
	.name = "machinactive",
	.init = machinactive_init,
	.busy = machinactive_busy,
	.idle = machinactive_idle,
	.sleep = machinactive_sleep,
	.wake = machinactive_wake,
	.close = machinactive_close
};
EXPORT_SYMBOL(kgsl_pwrscale_policy_machinactive);
