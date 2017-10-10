/* Copyright (c) 2010-2012, The Linux Foundation. All rights reserved.
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

#include <linux/export.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/spinlock.h>
#include <mach/socinfo.h>
#include <mach/scm.h>
#include <linux/module.h>
#include <linux/jiffies.h>

#include "kgsl.h"
#include "kgsl_pwrscale.h"
#include "kgsl_device.h"

#define TZ_GOVERNOR_PERFORMANCE 0
#define TZ_GOVERNOR_ONDEMAND    1
#define TZ_GOVERNOR_INTERACTIVE	2
#define TZ_GOVERNOR_POWERSAVE 3

struct tz_priv {
	int governor;
	unsigned int no_switch_cnt;
	unsigned int skip_cnt;
	struct kgsl_power_stats bin;
};
spinlock_t tz_lock;

/* CEILING is 50msec, larger than any standard
 * frame length, but less than the idle timer.
 */
/* FLOOR is 5msec to capture up to 3 re-draws
 * per frame for 60fps content.
 */
#define SWITCH_OFF		200
#define SWITCH_OFF_RESET_TH	40
#define SKIP_COUNTER		500
#define TZ_RESET_ID		0x3
#define TZ_UPDATE_ID		0x4
#define TZ_INIT_ID		0x6

static unsigned int ceiling = 50000;
static unsigned int floor = 15000;
unsigned int up_threshold = 55;
unsigned int down_threshold = 25;
unsigned int up_differential = 5;
bool debug = 0;

module_param(up_threshold, uint, 0664);
module_param(down_threshold, uint, 0664);
module_param(debug, bool, 0664);
module_param(ceiling, uint, 0644);
module_param(floor, uint, 0644);

static struct clk_scaling_stats {
	unsigned long threshold;
	unsigned int load;
} gpu_stats = {
	.threshold = 0,
	.load = 0,
};

/* Trap into the TrustZone, and call funcs there. */
static int __secure_tz_entry(u32 cmd, u32 val, u32 id)
{
	int ret;
	spin_lock(&tz_lock);
	__iowmb();
	ret = scm_call_atomic2(SCM_SVC_IO, cmd, val, id);
	spin_unlock(&tz_lock);
	return ret;
}

static ssize_t tz_governor_show(struct kgsl_device *device,
				struct kgsl_pwrscale *pwrscale,
				char *buf)
{
	struct tz_priv *priv = pwrscale->priv;
	int ret;

	if (priv->governor == TZ_GOVERNOR_ONDEMAND)
		ret = snprintf(buf, 10, "ondemand\n");
	else if (priv->governor == TZ_GOVERNOR_INTERACTIVE)
		ret = snprintf(buf, 13, "interactive\n");
	else if (priv->governor == TZ_GOVERNOR_POWERSAVE)
		ret = snprintf(buf, 11, "powersave\n");
	else
		ret = snprintf(buf, 13, "performance\n");

	return ret;
}

static ssize_t tz_governor_store(struct kgsl_device *device,
				struct kgsl_pwrscale *pwrscale,
				 const char *buf, size_t count)
{
	struct tz_priv *priv = pwrscale->priv;
	struct kgsl_pwrctrl *pwr = &device->pwrctrl;

	mutex_lock(&device->mutex);

	if (!strncmp(buf, "ondemand", 8))
		priv->governor = TZ_GOVERNOR_ONDEMAND;
	else if (!strncmp(buf, "interactive", 11))
		priv->governor = TZ_GOVERNOR_INTERACTIVE;
	else if (!strncmp(buf, "powersave", 9))
		priv->governor = TZ_GOVERNOR_POWERSAVE;
	else if (!strncmp(buf, "performance", 11))
		priv->governor = TZ_GOVERNOR_PERFORMANCE;

	if (priv->governor == TZ_GOVERNOR_PERFORMANCE) {
		kgsl_pwrctrl_pwrlevel_change(device, pwr->max_pwrlevel);
		pwr->default_pwrlevel = pwr->max_pwrlevel;
	} else if (priv->governor == TZ_GOVERNOR_POWERSAVE) {
		kgsl_pwrctrl_pwrlevel_change(device, pwr->min_pwrlevel);
		pwr->default_pwrlevel = pwr->min_pwrlevel;
	} else {
		pwr->default_pwrlevel = pwr->init_pwrlevel;
	}

	mutex_unlock(&device->mutex);
	return count;
}

PWRSCALE_POLICY_ATTR(governor, 0644, tz_governor_show, tz_governor_store);

static struct attribute *tz_attrs[] = {
	&policy_attr_governor.attr,
	NULL
};

static struct attribute_group tz_attr_group = {
	.attrs = tz_attrs,
	.name = "trustzone",
};

static void tz_wake(struct kgsl_device *device, struct kgsl_pwrscale *pwrscale)
{
	struct tz_priv *priv = pwrscale->priv;
	if ((device->state != KGSL_STATE_NAP) &&
		(priv->governor == TZ_GOVERNOR_ONDEMAND ||
		 priv->governor == TZ_GOVERNOR_INTERACTIVE))
			kgsl_pwrctrl_pwrlevel_change(device,
				device->pwrctrl.default_pwrlevel);
}

#define MIN_STEP 3
#define MAX_STEP 0
static void tz_idle(struct kgsl_device *device, struct kgsl_pwrscale *pwrscale)
{
	struct kgsl_pwrctrl *pwr = &device->pwrctrl;
	struct tz_priv *priv = pwrscale->priv;
	struct kgsl_power_stats stats;
	int val, idle;

	/* In "performance" mode the clock speed always stays
	   the same */
	if (priv->governor == TZ_GOVERNOR_PERFORMANCE ||
		priv->governor == TZ_GOVERNOR_POWERSAVE)
		return;

	device->ftbl->power_stats(device, &stats);
	priv->bin.total_time += stats.total_time;
	priv->bin.busy_time += stats.busy_time;
	if (priv->governor == TZ_GOVERNOR_ONDEMAND) {
		idle = priv->bin.total_time - priv->bin.busy_time;
		/* Do not waste CPU cycles running this algorithm if
		 * the GPU just started, or if less than FLOOR time
		 * has passed since the last run.
		 */
		if ((stats.total_time == 0) ||
			(priv->bin.total_time < floor)) {
			if (pwr->active_pwrlevel < MIN_STEP)
				kgsl_pwrctrl_pwrlevel_change(device,
						     pwr->active_pwrlevel + 1);	
			return;
		}
		/* If the GPU has stayed in turbo mode for a while, *
		* stop writing out values. */
		if (pwr->active_pwrlevel == 0) {
			if (priv->no_switch_cnt > SWITCH_OFF) {
				priv->skip_cnt++;
				if (priv->skip_cnt > SKIP_COUNTER) {
					priv->no_switch_cnt -= SWITCH_OFF_RESET_TH;
					priv->skip_cnt = 0;
				}
				return;
			}
			priv->no_switch_cnt++;
		} else {
			priv->no_switch_cnt = 0;
		}

		/* If there is an extended block of busy processing,
		* increase frequency.  Otherwise run the normal algorithm.
		*/
		if (priv->bin.busy_time > ceiling) {
			val = - 1;
		} else {
			idle = priv->bin.total_time - priv->bin.busy_time;
			idle = (idle > 0) ? idle : 0;
			val = __secure_tz_entry(TZ_UPDATE_ID, idle, device->id);
			//SPAM!pr_info("KGSL secure tz step entry: %d idle:%d\n", val, idle);
		}
		priv->bin.total_time = 0;
		priv->bin.busy_time = 0;
		if (val)
			kgsl_pwrctrl_pwrlevel_change(device,
					     pwr->active_pwrlevel + val);

	} else if (priv->governor == TZ_GOVERNOR_INTERACTIVE) {
		if (stats.total_time == 0 || priv->bin.busy_time < floor)
			return;

		if (stats.busy_time >= 1 << 24 || stats.total_time >= 1 << 24) {
			stats.busy_time >>= 7;
			stats.total_time >>= 7;
		}

		/*
		 * If there is an extended block of busy processing,
		 * increase frequency. Otherwise run the normal algorithm.
		 */
		if (priv->bin.busy_time > ceiling) {
			kgsl_pwrctrl_pwrlevel_change(device, pwr->active_pwrlevel - 1);
			goto clear;
		}

		gpu_stats.load = (100 * priv->bin.busy_time);
		if (priv->bin.total_time > 0)
			do_div(gpu_stats.load, priv->bin.total_time);
		else
			gpu_stats.load = priv->bin.total_time - priv->bin.busy_time;

		gpu_stats.threshold = up_threshold;

		if (pwr->active_pwrlevel == MIN_STEP) {
				gpu_stats.threshold = up_threshold / pwr->active_pwrlevel - 1;
		} else if (pwr->active_pwrlevel < MIN_STEP &&
					pwr->active_pwrlevel >= MAX_STEP) {
			gpu_stats.threshold = up_threshold + up_differential;
		}

		if (gpu_stats.load >= gpu_stats.threshold) {

			if (pwr->active_pwrlevel > MAX_STEP)
				kgsl_pwrctrl_pwrlevel_change(device,
						     pwr->active_pwrlevel - 1);

		} else {
			if (pwr->active_pwrlevel < MIN_STEP)
				kgsl_pwrctrl_pwrlevel_change(device,
						     pwr->active_pwrlevel + 1);
		}
clear:
	priv->bin.total_time = 0;
	priv->bin.busy_time = 0;
	}
}

static void tz_busy(struct kgsl_device *device,
	struct kgsl_pwrscale *pwrscale)
{
	device->on_time = ktime_to_us(ktime_get());
}

static void tz_sleep(struct kgsl_device *device,
	struct kgsl_pwrscale *pwrscale)
{
	struct tz_priv *priv = pwrscale->priv;
	struct kgsl_pwrctrl *pwr = &device->pwrctrl;

	kgsl_pwrctrl_pwrlevel_change(device, pwr->min_pwrlevel);
	__secure_tz_entry(TZ_RESET_ID, 0, device->id);
	priv->bin.total_time = 0;
	priv->bin.busy_time = 0;
}

#ifdef CONFIG_MSM_SCM
static int tz_init(struct kgsl_device *device, struct kgsl_pwrscale *pwrscale)
{
	int i = 0, j = 1, ret = 0;
	struct tz_priv *priv;
	struct kgsl_pwrctrl *pwr = &device->pwrctrl;
	unsigned int tz_pwrlevels[KGSL_MAX_PWRLEVELS + 1];

	priv = pwrscale->priv = kzalloc(sizeof(struct tz_priv), GFP_KERNEL);
	if (pwrscale->priv == NULL)
		return -ENOMEM;
	priv->governor = TZ_GOVERNOR_ONDEMAND;
	spin_lock_init(&tz_lock);
	kgsl_pwrscale_policy_add_files(device, pwrscale, &tz_attr_group);
	for (i = 0; i < pwr->num_pwrlevels - 1; i++) {
		if (i == 0)
			tz_pwrlevels[j] = pwr->pwrlevels[i].gpu_freq;
		else if (pwr->pwrlevels[i].gpu_freq !=
				pwr->pwrlevels[i - 1].gpu_freq) {
			j++;
			tz_pwrlevels[j] = pwr->pwrlevels[i].gpu_freq;
		}
	}
	tz_pwrlevels[0] = j;
	return 0;
}
#else
static int tz_init(struct kgsl_device *device, struct kgsl_pwrscale *pwrscale)
{
	return -EINVAL;
}
#endif /* CONFIG_MSM_SCM */

static void tz_close(struct kgsl_device *device, struct kgsl_pwrscale *pwrscale)
{
	kgsl_pwrscale_policy_remove_files(device, pwrscale, &tz_attr_group);
	if (pwrscale->priv != NULL)
		kfree(pwrscale->priv);
	pwrscale->priv = NULL;
}

struct kgsl_pwrscale_policy kgsl_pwrscale_policy_tz = {
	.name = "trustzone",
	.init = tz_init,
	.busy = tz_busy,
	.idle = tz_idle,
	.sleep = tz_sleep,
	.wake = tz_wake,
	.close = tz_close
};
EXPORT_SYMBOL(kgsl_pwrscale_policy_tz);

