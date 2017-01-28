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
 * The conservative kgsl policy acts exactly like it's cpufreq scaling driver
 * counterpart. It attemps to scale frequency in small steps depending on the
 * current GPU load (calculated using statistics) every sampling interval.
 * On idle conservative scales GPU frequency all the way down to reduce power
 * consumption. Compared to qualcomms's trustzone algorithm which tends to
 * inefficiently scale frequencies conservative offers power savings and heat
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
 * Without locking i discovered that conservative switches frequencies
 * randomly at times, meaning that it up/downscales even if the load
 * does not reach/cross the corresponding threshold.
 */
static DEFINE_SPINLOCK(conservative_lock);

/*
 * KGSL policy scaling mode.
 * Energy save locks the active pwrlevel to the highest present.
 * Performance locks the active pwrlevel to the lowest present.
 */
static unsigned int mode[] = {
	0,	/* Conservative (default)*/
	1,	/* Energy save */
	2	/* Performance */
};

static unsigned int scale_mode;

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
	unsigned int up_threshold;
	unsigned int down_threshold;
};

static struct load_thresholds thresholds[] = {
	{98,		70},	/* 400 MHz @pwrlevel 0 */
	{80,		50},	/* 320 MHz @pwrlevel 1 */
	{60,		30},	/* 200 MHz @pwrlevel 2 */
	{40,		 0},	/* 128 MHz @pwrlevel 3 */
	{ 0,	 	 0}	/*  27 MHz @pwrlevel 4 */
};

static void conservative_wake(struct kgsl_device *device,
				struct kgsl_pwrscale *pwrscale)
{
	struct kgsl_power_stats stats;

	if (device->state != KGSL_STATE_NAP && scale_mode == mode[0]) {
		/* Reset the power stats counters. */
		device->ftbl->power_stats(device, &stats);
		walltime_total = 0;
		busytime_total = 0;
	}
}

static void conservative_idle(struct kgsl_device *device,
				struct kgsl_pwrscale *pwrscale)
{
	struct kgsl_pwrctrl *pwr = &device->pwrctrl;
	struct kgsl_power_stats stats;
	unsigned long flags;
	unsigned int load_hist;
	int val = 0;

	device->ftbl->power_stats(device, &stats);

	/*
	 * Break out early if conservative is running in energy saving
	 * or performance mode.
	 */
	if (!stats.total_time ||
		scale_mode == mode[1] || scale_mode == mode[2])
		return;

	walltime_total += (unsigned long) stats.total_time;
	busytime_total += (unsigned long) stats.busy_time;

	if (walltime_total > polling_interval) {
		load_hist = (100 * busytime_total) / walltime_total;

		walltime_total = busytime_total = 0;

		/*
		 * Scaling decision is the only part which really
		 * needs locking. Leave it to that to keep overhead
		 * as low as possible.
		 */
		spin_lock_irqsave(&conservative_lock, flags);

		if (load_hist < thresholds[pwr->active_pwrlevel].down_threshold)
			val = 1;
		else if (load_hist >
			thresholds[pwr->active_pwrlevel].up_threshold)
			val = -1;

		spin_unlock_irqrestore(&conservative_lock, flags);

		if (val)
			kgsl_pwrctrl_pwrlevel_change(device,
						pwr->active_pwrlevel + val);
	}
}

static void conservative_busy(struct kgsl_device *device,
				struct kgsl_pwrscale *pwrscale)
{
	device->on_time = ktime_to_us(ktime_get());
}

static void conservative_sleep(struct kgsl_device *device,
				struct kgsl_pwrscale *pwrscale)
{
	struct kgsl_pwrctrl *pwr = &device->pwrctrl;

	/* Bring GPU frequency all the way down on sleep */
	if (scale_mode != mode[2] && pwr->active_pwrlevel != pwr->min_pwrlevel)
		kgsl_pwrctrl_pwrlevel_change(device, pwr->min_pwrlevel);
}

static ssize_t conservative_polling_interval_show(struct kgsl_device *device,
				struct kgsl_pwrscale *pwrscale,
				char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%lu\n", polling_interval);
}

static ssize_t conservative_polling_interval_store(struct kgsl_device *device,
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
					conservative_polling_interval_show,
					conservative_polling_interval_store);

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

	sscanf(buf, "%lu", &val);

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
		pr_info("[POLGOV] 2-Performance Mode\n");
	} else
		scale_mode = mode[0];

	mutex_unlock(&device->mutex);

	return count;
}

PWRSCALE_POLICY_ATTR(policy_scale_mode, 0644,
							scale_mode_show,
							scale_mode_store);

static struct attribute *conservative_attrs[] = {
	&policy_attr_polling_interval.attr,
	&policy_attr_pwrlevel_thresholds.attr,
	&policy_attr_policy_scale_mode.attr,
	NULL
};

static struct attribute_group conservative_attr_group = {
	.attrs = conservative_attrs,
	.name = "conservative",
};

static int conservative_init(struct kgsl_device *device,
				struct kgsl_pwrscale *pwrscale)
{
	scale_mode = mode[2];

	spin_lock_init(&conservative_lock);

	kgsl_pwrscale_policy_add_files(device, pwrscale,
						&conservative_attr_group);

	return 0;
}

static void conservative_close(struct kgsl_device *device,
				struct kgsl_pwrscale *pwrscale)
{
	kgsl_pwrscale_policy_remove_files(device, pwrscale,
						&conservative_attr_group);
}

struct kgsl_pwrscale_policy kgsl_pwrscale_policy_conservative = {
	.name = "conservative",
	.init = conservative_init,
	.busy = conservative_busy,
	.idle = conservative_idle,
	.sleep = conservative_sleep,
	.wake = conservative_wake,
	.close = conservative_close
};
EXPORT_SYMBOL(kgsl_pwrscale_policy_conservative);
