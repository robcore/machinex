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
static unsigned int floor = 5000;
static unsigned int i_up_threshold = 7500;
static unsigned int i_down_threshold = 4000;
bool debug = 0;

module_param(i_up_threshold, uint, 0664);
module_param(i_down_threshold, uint, 0664);
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

static unsigned int loadview;
module_param(loadview, uint, 0444);

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

static int __secure_tz_entry3(u32 cmd, u32 val1, u32 val2, u32 val3)
{
	int ret;
	spin_lock(&tz_lock);
	/* sync memory before sending the commands to tz*/
	__iowmb();
	ret = scm_call_atomic3(SCM_SVC_IO, cmd, val1, val2, val3);
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
	loadview = priv->bin.busy_time;
	if ((device->state != KGSL_STATE_NAP) &&
		(priv->governor == TZ_GOVERNOR_ONDEMAND ||
		 priv->governor == TZ_GOVERNOR_INTERACTIVE)) {
			if (loadview < 4000)
				kgsl_pwrctrl_pwrlevel_change(device,
					device->pwrctrl.default_pwrlevel + 3);
			else if (loadview >= 4000 && loadview < 6000)
					kgsl_pwrctrl_pwrlevel_change(device,
					device->pwrctrl.default_pwrlevel + 2);
			else if (loadview >= 6000 && loadview < 7000)
					kgsl_pwrctrl_pwrlevel_change(device, 
					device->pwrctrl.default_pwrlevel + 1);
			else if (loadview >= 7000)
				kgsl_pwrctrl_pwrlevel_change(device,
					device->pwrctrl.default_pwrlevel);
	}
}

#define MIN_STEP 3
#define MAX_STEP 0

static void tz_idle(struct kgsl_device *device, struct kgsl_pwrscale *pwrscale)
{
	struct kgsl_pwrctrl *pwr = &device->pwrctrl;
	struct tz_priv *priv = pwrscale->priv;
	struct kgsl_power_stats stats;
	int level;
	int val;

	device->ftbl->power_stats(device, &stats);
	priv->bin.total_time += stats.total_time;
	priv->bin.busy_time += stats.busy_time;

	/* Do not waste CPU cycles running this algorithm if
	 * the GPU just started, or if less than FLOOR time
	 * has passed since the last run.
	 */
	if ((stats.total_time == 0) ||
		(priv->bin.total_time < floor))
		return;

	level = pwr->active_pwrlevel;

	switch (priv->governor) {
		case TZ_GOVERNOR_PERFORMANCE:
		case TZ_GOVERNOR_POWERSAVE:
	/* In "performance" and "powersave" modes the clock speed always stays
	   the same */
			return;
		case TZ_GOVERNOR_ONDEMAND:
			if (priv->bin.busy_time > ceiling) {
				if (level)
					val = (level * -1);
			} else {
				val = __secure_tz_entry3(TZ_UPDATE_ID,
					level,
					priv->bin.total_time,
					priv->bin.busy_time);
			}

			if (val) {
				level += val;
				level = level > pwr->max_pwrlevel ? level : pwr->max_pwrlevel;
				level = level < pwr->min_pwrlevel ? level : pwr->min_pwrlevel;
			}
			kgsl_pwrctrl_pwrlevel_change(device,
						     level);
			break;
		case TZ_GOVERNOR_INTERACTIVE:
			if (stats.busy_time >= 1 << 24 || stats.total_time >= 1 << 24) {
				stats.busy_time >>= 7;
				stats.total_time >>= 7;
			}

			/*
			 * If there is an extended block of busy processing,
			 * increase frequency. Otherwise run the normal algorithm.
			 */
			if (priv->bin.busy_time < ceiling) {
				break;
			}

			gpu_stats.load = priv->bin.busy_time;

			if (level <= MIN_STEP && level > MAX_STEP) {
					if (gpu_stats.load >= i_up_threshold)
						kgsl_pwrctrl_pwrlevel_change(device,
								     level - 1);
			} else if (level == MAX_STEP) {
				if (gpu_stats.load < i_down_threshold)
					kgsl_pwrctrl_pwrlevel_change(device,
							     level + 1);
			}
	}
	priv->bin.total_time = 0;
	priv->bin.busy_time = 0;
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
