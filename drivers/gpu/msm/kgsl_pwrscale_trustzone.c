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
#include <asm/div64.h>
#include <linux/cpufreq.h>

#include "kgsl.h"
#include "kgsl_pwrscale.h"
#include "kgsl_device.h"

#define TZ_GOVERNOR_PERFORMANCE 0
#define TZ_GOVERNOR_ONDEMAND    1
#define TZ_GOVERNOR_INTERACTIVE	2
#define TZ_GOVERNOR_POWERSAVE 3
#define TZ_GOVERNOR_MACHINACTIVE 4

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

#define FLOOR			5000
/* CEILING is 50msec, larger than any standard
 * frame length, but less than the idle timer.
 */
#define CEILING			CEILING

#define MAX_LOAD		98
#define MIN_POLL_INTERVAL	10000
#define POLL_INTERVAL		100000
#define MAX_POLL_INTERVAL	1000000
/*
static unsigned int thermal_powerlevel = 320000000;
static unsigned int gpu_boost_freq = 200000000;

static int set_gpu_boost_freq(const char *buf, const struct kernel_param *kp)
{
	unsigned int val, cpu = 0;
	int i, temp_low = -1;

	if (!sscanf(buf, "%u", &val))
		return -EINVAL;

	if (val != 450000000 && val != 320000000 &&
		val != 200000000 && val != 128000000)
		return -EINVAL;
	
	gpu_boost_freq = val;
	return 0;
}

static int get_gpu_boost_freq(char *buf, const struct kernel_param *kp)
{
	ssize_t ret;
	unsigned int cpu = 0;

	ret = sprintf(buf, "%u", gpu_boost_freq);

	return ret;
}

static const struct kernel_param_ops param_ops_gpu_boost_freq = {
	.set = set_gpu_boost_freq,
	.get = get_gpu_boost_freq
};

module_param_cb(gpu_boost_freq, &param_ops_gpu_boost_freq, NULL, 0644);
*/
static unsigned long polling_interval = POLL_INTERVAL;

static unsigned long walltime_total;
static unsigned long busytime_total;


struct m_load_thresholds {
	unsigned int m_up_threshold;
	unsigned int m_down_threshold;
};

static struct m_load_thresholds thresholds[] = {
	{UINT_MAX,	65},	/* 400 MHz @pwrlevel 0 */
	{75,		50},	/* 320 MHz @pwrlevel 1 */
	{55,		30},	/* 200 MHz @pwrlevel 2 */
	{40,		 0},	/* 128 MHz @pwrlevel 3 */
	{ 0,	 	 0}	/*  27 MHz @pwrlevel 4 */
};

static s64 ceiling = 50000;
static s64 floor = FLOOR;
static s64 i_up_threshold = 70;
static s64 i_down_threshold = 45;
//static s64 loadview;

static int set_ceiling(const char *buf, const struct kernel_param *kp)
{
	s64 val;

	if (!sscanf(buf, "%lld", &val))
		return -EINVAL;

	sanitize_min_max(val, 10000, 100000);

	ceiling = val;
	return 0;
}

static int get_ceiling(char *buf, const struct kernel_param *kp)
{
	ssize_t ret;

	ret = sprintf(buf, "%lld", ceiling);

	return ret;
}

static const struct kernel_param_ops param_ops_ceiling = {
	.set = set_ceiling,
	.get = get_ceiling,
};

module_param_cb(ceiling, &param_ops_ceiling, NULL, 0644);

static int set_i_up_threshold(const char *buf, const struct kernel_param *kp)
{
	s64 val;

	if (!sscanf(buf, "%lld", &val))
		return -EINVAL;

	sanitize_min_max(val, 1, 99);

	i_up_threshold = val;
	return 0;
}

static int get_i_up_threshold(char *buf, const struct kernel_param *kp)
{
	ssize_t ret;

	ret = sprintf(buf, "%lld", i_up_threshold);

	return ret;
}

static const struct kernel_param_ops param_ops_i_up_threshold = {
	.set = set_i_up_threshold,
	.get = get_i_up_threshold,
};

module_param_cb(i_up_threshold, &param_ops_i_up_threshold, NULL, 0644);

static int set_i_down_threshold(const char *buf, const struct kernel_param *kp)
{
	s64 val;

	if (!sscanf(buf, "%lld", &val))
		return -EINVAL;

	sanitize_min_max(val, 1, 99);

	i_down_threshold = val;
	return 0;
}

static int get_i_down_threshold(char *buf, const struct kernel_param *kp)
{
	ssize_t ret;

	ret = sprintf(buf, "%lld", i_down_threshold);

	return ret;
}

static const struct kernel_param_ops param_ops_i_down_threshold = {
	.set = set_i_down_threshold,
	.get = get_i_down_threshold,
};

module_param_cb(i_down_threshold, &param_ops_i_down_threshold, NULL, 0644);

static int set_floor(const char *buf, const struct kernel_param *kp)
{
	s64 val;

	if (!sscanf(buf, "%lld", &val))
		return -EINVAL;

	sanitize_min_max(val, 1000, 20000);

	floor = val;
	return 0;
}

static int get_floor(char *buf, const struct kernel_param *kp)
{
	ssize_t ret;

	ret = sprintf(buf, "%lld", floor);

	return ret;
}

static const struct kernel_param_ops param_ops_floor = {
	.set = set_floor,
	.get = get_floor,
};

module_param_cb(floor, &param_ops_floor, NULL, 0644);
/*
static int get_loadview(char *buf, const struct kernel_param *kp)
{
	ssize_t ret;

	ret = sprintf(buf, "%lld", loadview);

	return ret;
}

static const struct kernel_param_ops param_ops_loadview = {
	.set = NULL,
	.get = get_loadview,
};

module_param_cb(loadview, &param_ops_loadview, NULL, 0444);
*/
static struct clk_scaling_stats {
	unsigned long threshold;
	s64 load;
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
	else if (priv->governor == TZ_GOVERNOR_PERFORMANCE)
		ret = snprintf(buf, 13, "performance\n");
	else if (priv->governor == TZ_GOVERNOR_MACHINACTIVE)
		ret = snprintf(buf, 14, "machinactive\n");

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
	else if (!strncmp(buf, "machinactive", 12))
		priv->governor = TZ_GOVERNOR_MACHINACTIVE;

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
	if (device->state == KGSL_STATE_NAP)
		return;
/*	if (thermal_is_throttling())
		kgsl_pwrctrl_pwrlevel_change(device, thermal_powerlevel);
	else
*/
		kgsl_pwrctrl_pwrlevel_change(device, device->pwrctrl.max_pwrlevel);
}

#define MIN_STEP 3
#define MAX_STEP 0

static void tz_idle(struct kgsl_device *device, struct kgsl_pwrscale *pwrscale)
{
	struct kgsl_pwrctrl *pwr = &device->pwrctrl;
	struct tz_priv *priv = pwrscale->priv;
	struct kgsl_power_stats stats;
	unsigned int load_hist;
	int level, val;

	device->ftbl->power_stats(device, &stats);
	priv->bin.total_time += stats.total_time;
	priv->bin.busy_time += stats.busy_time;
	/* Do not waste CPU cycles running this algorithm if
	 * the GPU just started, or if less than FLOOR time
	 * has passed since the last run.
	 */
	if (priv->bin.total_time < floor ||
		stats.total_time == 0)
		return;

//		loadview = (priv->bin.busy_time*5243)>>19;
	switch (priv->governor) {
		case TZ_GOVERNOR_PERFORMANCE:
		case TZ_GOVERNOR_POWERSAVE:
	/* In "performance" and "powersave" modes the clock speed always stays
	   the same */
			return;
		case TZ_GOVERNOR_ONDEMAND:
			level = pwr->active_pwrlevel;
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
				sanitize_min_max(level, pwr->max_pwrlevel, pwr->min_pwrlevel);
			} else {
				level += 1;
				kgsl_pwrctrl_pwrlevel_change(device,
							     level);
			}
			break;
		case TZ_GOVERNOR_INTERACTIVE:
			level = pwr->active_pwrlevel;
			/*
			 * If there is an extended block of busy processing,
			 * increase frequency. Otherwise run the normal algorithm.
			 */

			gpu_stats.load = (priv->bin.busy_time*5243)>>19;

			if (level <= MIN_STEP && level > MAX_STEP) {
					if (gpu_stats.load >= i_up_threshold)
						kgsl_pwrctrl_pwrlevel_change(device,
								     level - 1);

			} else if (level == MAX_STEP) {
				if (gpu_stats.load < i_down_threshold) {
					__secure_tz_entry3(TZ_UPDATE_ID,
						level,
						priv->bin.total_time,
						priv->bin.busy_time);
					kgsl_pwrctrl_pwrlevel_change(device,
							     level + 1);
				}
			}
			break;
		case TZ_GOVERNOR_MACHINACTIVE:
			level = pwr->active_pwrlevel;
			walltime_total += (unsigned long) stats.total_time;
			busytime_total += (unsigned long) stats.busy_time;

			if (walltime_total > polling_interval) {
				load_hist = (100 * busytime_total) / walltime_total;

				walltime_total = busytime_total = 0;

				if (load_hist < thresholds[level].m_down_threshold) {
					val = 1;
					__secure_tz_entry3(TZ_UPDATE_ID,
						level,
						priv->bin.total_time,
						priv->bin.busy_time);
				} else if (load_hist >
					thresholds[level].m_up_threshold)
					val = -1;
				if (val)
					kgsl_pwrctrl_pwrlevel_change(device,
								level + val);
			}
			break;
	}
/*
	loadview = priv->bin.total_time > priv->bin.busy_time ?
		(do_div(priv->bin.busy_time, priv->bin.total_time) * 5243)>>19 :
		(priv->bin.busy_time*5243)>>19;
*/
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

	__secure_tz_entry(TZ_RESET_ID, 0, device->id);
	priv->bin.total_time = 0;
	priv->bin.busy_time = 0;
	walltime_total = 0;
	busytime_total = 0;
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
