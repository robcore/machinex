/* Copyright (c) 2010-2012,2014 The Linux Foundation. All rights reserved.
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
#include <linux/interrupt.h>
#include <asm/page.h>
#include <linux/pm_runtime.h>
#include <mach/msm_iomap.h>
#include <mach/msm_bus.h>
#include <linux/ktime.h>

#include "kgsl.h"
#include "kgsl_pwrscale.h"
#include "kgsl_device.h"
#include "kgsl_trace.h"
#include "kgsl_sharedmem.h"

#define KGSL_PWRFLAGS_POWER_ON 0
#define KGSL_PWRFLAGS_CLK_ON   1
#define KGSL_PWRFLAGS_AXI_ON   2
#define KGSL_PWRFLAGS_IRQ_ON   3

#define UPDATE_BUSY_VAL		1000000
#define UPDATE_BUSY		50

#ifdef CONFIG_MSM_KGSL_KERNEL_API_ENABLE
struct device *stored_dev;
#endif

#ifdef CONFIG_CPU_FREQ_GOV_ELECTROACTIVE
int graphics_boost_machinactive;
#endif

#ifdef CONFIG_CPU_FREQ_GOV_ELEMENTALX
int graphics_boost_elementalx = 4;
#endif

struct clk_pair {
	const char *name;
	uint map;
};

struct clk_pair clks[KGSL_MAX_CLKS] = {
	{
		.name = "src_clk",
		.map = KGSL_CLK_SRC,
	},
	{
		.name = "core_clk",
		.map = KGSL_CLK_CORE,
	},
	{
		.name = "iface_clk",
		.map = KGSL_CLK_IFACE,
	},
	{
		.name = "mem_clk",
		.map = KGSL_CLK_MEM,
	},
	{
		.name = "mem_iface_clk",
		.map = KGSL_CLK_MEM_IFACE,
	},
};

#if defined(CONFIG_MSM_KGSL_FPS_NODE_ENABLE)
static int fps = 60;
static int max_fps = 60;
#endif

/* Update the elapsed time at a particular clock level
 * if the device is active(on_time = true).Otherwise
 * store it as sleep time.
 */
static void update_clk_statistics(struct kgsl_device *device,
				bool on_time)
{
	struct kgsl_pwrctrl *pwr = &device->pwrctrl;
	struct kgsl_clk_stats *clkstats = &pwr->clk_stats;
	ktime_t elapsed;
	int elapsed_us;
	if (clkstats->start == 0)
		clkstats->start = ktime_get();
	clkstats->stop = ktime_get();
	elapsed = ktime_sub(clkstats->stop, clkstats->start);
	elapsed_us = ktime_to_us(elapsed);
	clkstats->elapsed += elapsed_us;
	if (on_time)
		clkstats->clock_time[pwr->active_pwrlevel] += elapsed_us;
	else
		clkstats->clock_time[pwr->num_pwrlevels - 1] += elapsed_us;
	clkstats->start = ktime_get();
}

/*
 * Given a requested power level do bounds checking on the constraints and
 * return the nearest possible level
 */

static inline int _adjust_pwrlevel(struct kgsl_pwrctrl *pwr, int level)
{
	int max_pwrlevel = max_t(int, pwr->thermal_pwrlevel, pwr->max_pwrlevel);
	int min_pwrlevel = max_t(int, pwr->thermal_pwrlevel, pwr->min_pwrlevel);

	if (level < max_pwrlevel)
		return max_pwrlevel;
	if (level > min_pwrlevel)
		return min_pwrlevel;

	return level;
}

void kgsl_pwrctrl_pwrlevel_change(struct kgsl_device *device,
				unsigned int new_level)
{
	struct kgsl_pwrctrl *pwr = &device->pwrctrl;
	struct kgsl_pwrlevel *pwrlevel;
	int delta;
	int level;

	/* Adjust the power level to the current constraints */
	new_level = _adjust_pwrlevel(pwr, new_level);

	if (new_level == pwr->active_pwrlevel)
		return;

	delta = new_level < pwr->active_pwrlevel ? -1 : 1;

	update_clk_statistics(device, true);

	level = pwr->active_pwrlevel;

	/*
	 * Set the active powerlevel first in case the clocks are off - if we
	 * don't do this then the pwrlevel change won't take effect when the
	 * clocks come back
	 */

	pwr->active_pwrlevel = new_level;
	pwrlevel = &pwr->pwrlevels[pwr->active_pwrlevel];

	if (test_bit(KGSL_PWRFLAGS_AXI_ON, &pwr->power_flags)) {

		if (pwr->pcl)
			msm_bus_scale_client_update_request(pwr->pcl,
				pwrlevel->bus_freq);
		else if (pwr->ebi1_clk)
			clk_set_rate(pwr->ebi1_clk, pwrlevel->bus_freq);
	}

	if (test_bit(KGSL_PWRFLAGS_CLK_ON, &pwr->power_flags) ||
		(device->state == KGSL_STATE_NAP)) {

		/*
		 * On some platforms, instability is caused on
		 * changing clock freq when the core is busy.
		 * Idle the gpu core before changing the clock freq.
		 */

		if (pwr->idle_needed == true)
			device->ftbl->idle(device);

		/*
		 * Don't shift by more than one level at a time to
		 * avoid glitches.
		 */

		while (level != new_level) {
			level += delta;

			clk_set_rate(pwr->grp_clks[0],
				pwr->pwrlevels[level].gpu_freq);
		}
	}


	trace_kgsl_pwrlevel(device, pwr->active_pwrlevel, pwrlevel->gpu_freq);
}

EXPORT_SYMBOL(kgsl_pwrctrl_pwrlevel_change);

static int kgsl_pwrctrl_thermal_pwrlevel_store(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t count)
{
	struct kgsl_device *device = kgsl_device_from_dev(dev);
	struct kgsl_pwrctrl *pwr;
	int ret;
	unsigned int level = 0;

	if (device == NULL)
		return 0;

	pwr = &device->pwrctrl;

	ret = kgsl_sysfs_store(buf, &level);

	if (ret)
		return ret;

	mutex_lock(&device->mutex);

	if (level > pwr->num_pwrlevels - 2)
		level = pwr->num_pwrlevels - 2;

	pwr->thermal_pwrlevel = level;

	/*
	 * If there is no power policy set the clock to the requested thermal
	 * level - if thermal now happens to be higher than max, then that will
	 * be limited by the pwrlevel change function.  Otherwise if there is
	 * a policy only change the active clock if it is higher then the new
	 * thermal level
	 */

	if (device->pwrscale.policy == NULL ||
		pwr->thermal_pwrlevel > pwr->active_pwrlevel)
		kgsl_pwrctrl_pwrlevel_change(device, pwr->thermal_pwrlevel);

	mutex_unlock(&device->mutex);

	return count;
}

static int kgsl_pwrctrl_thermal_pwrlevel_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{

	struct kgsl_device *device = kgsl_device_from_dev(dev);
	struct kgsl_pwrctrl *pwr;
	if (device == NULL)
		return 0;
	pwr = &device->pwrctrl;
	return snprintf(buf, PAGE_SIZE, "%d\n", pwr->thermal_pwrlevel);
}

static int kgsl_pwrctrl_max_pwrlevel_store(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t count)
{
	struct kgsl_device *device = kgsl_device_from_dev(dev);
	struct kgsl_pwrctrl *pwr;
	int ret, max_level;
	unsigned int level = 0;

	if (device == NULL)
		return 0;

	pwr = &device->pwrctrl;

	ret = kgsl_sysfs_store(buf, &level);
	if (ret)
		return ret;

	mutex_lock(&device->mutex);

	/* You can't set a maximum power level lower than the minimum */
	if (level > pwr->min_pwrlevel)
		level = pwr->min_pwrlevel;

	pwr->max_pwrlevel = level;


	max_level = max_t(int, pwr->thermal_pwrlevel, pwr->max_pwrlevel);

	/*
	 * If there is no policy then move to max by default.  Otherwise only
	 * move max if the current level happens to be higher then the new max
	 */

	if (device->pwrscale.policy == NULL ||
		(max_level > pwr->active_pwrlevel))
		kgsl_pwrctrl_pwrlevel_change(device, max_level);

	mutex_unlock(&device->mutex);

	return count;
}

static int kgsl_pwrctrl_max_pwrlevel_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{

	struct kgsl_device *device = kgsl_device_from_dev(dev);
	struct kgsl_pwrctrl *pwr;
	if (device == NULL)
		return 0;
	pwr = &device->pwrctrl;
	return snprintf(buf, PAGE_SIZE, "%d\n", pwr->max_pwrlevel);
}

static int kgsl_pwrctrl_min_pwrlevel_store(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t count)
{	struct kgsl_device *device = kgsl_device_from_dev(dev);
	struct kgsl_pwrctrl *pwr;
	int ret, min_level;
	unsigned int level = 0;

	if (device == NULL)
		return 0;

	pwr = &device->pwrctrl;

	ret = kgsl_sysfs_store(buf, &level);
	if (ret)
		return ret;

	mutex_lock(&device->mutex);
	if (level > pwr->num_pwrlevels - 2)
		level = pwr->num_pwrlevels - 2;

	/* You can't set a minimum power level lower than the maximum */
	if (level < pwr->max_pwrlevel)
		level = pwr->max_pwrlevel;

	pwr->min_pwrlevel = level;

	min_level = max_t(int, pwr->thermal_pwrlevel, pwr->min_pwrlevel);

	/* Only move the power level higher if minimum is higher then the
	 * current level
	 */

	if (min_level < pwr->active_pwrlevel)
		kgsl_pwrctrl_pwrlevel_change(device, min_level);

	mutex_unlock(&device->mutex);

	return count;
}

static int kgsl_pwrctrl_min_pwrlevel_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct kgsl_device *device = kgsl_device_from_dev(dev);
	struct kgsl_pwrctrl *pwr;
	if (device == NULL)
		return 0;
	pwr = &device->pwrctrl;
	return snprintf(buf, PAGE_SIZE, "%d\n", pwr->min_pwrlevel);
}

static int kgsl_pwrctrl_num_pwrlevels_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{

	struct kgsl_device *device = kgsl_device_from_dev(dev);
	struct kgsl_pwrctrl *pwr;
	if (device == NULL)
		return 0;
	pwr = &device->pwrctrl;
	return snprintf(buf, PAGE_SIZE, "%d\n", pwr->num_pwrlevels - 1);
}

/* Given a GPU clock value, return the lowest matching powerlevel */

static int _get_nearest_pwrlevel(struct kgsl_pwrctrl *pwr, unsigned int clock)
{
	int i;

	for (i = pwr->num_pwrlevels - 1; i >= 0; i--) {
		if (abs(pwr->pwrlevels[i].gpu_freq - clock) < 5000000)
			return i;
	}

	return -ERANGE;
}

static int kgsl_pwrctrl_max_gpuclk_store(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t count)
{
	struct kgsl_device *device = kgsl_device_from_dev(dev);
	struct kgsl_pwrctrl *pwr;
	unsigned int val = 0;
	int ret, level;

	if (device == NULL)
		return 0;

	pwr = &device->pwrctrl;

	ret = kgsl_sysfs_store(buf, &val);
	if (ret)
		return ret;

	mutex_lock(&device->mutex);
	level = _get_nearest_pwrlevel(pwr, val);
	if (level < 0)
		goto done;

	pwr->thermal_pwrlevel = level;

	/*
	 * if the thermal limit is lower than the current setting,
	 * move the speed down immediately
	 */

	if (pwr->thermal_pwrlevel > pwr->active_pwrlevel)
		kgsl_pwrctrl_pwrlevel_change(device, pwr->thermal_pwrlevel);

done:
	mutex_unlock(&device->mutex);
	return count;
}

static int kgsl_pwrctrl_max_gpuclk_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{

	struct kgsl_device *device = kgsl_device_from_dev(dev);
	struct kgsl_pwrctrl *pwr;
	if (device == NULL)
		return 0;
	pwr = &device->pwrctrl;
	return snprintf(buf, PAGE_SIZE, "%d\n",
			pwr->pwrlevels[pwr->thermal_pwrlevel].gpu_freq);
}

static int kgsl_pwrctrl_mix_gpuclk_store(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t count)
{
	struct kgsl_device *device = kgsl_device_from_dev(dev);
	struct kgsl_pwrctrl *pwr;
	unsigned int val = 0;
	int ret, level;

	if (device == NULL)
		return 0;

	pwr = &device->pwrctrl;

	ret = kgsl_sysfs_store(buf, &val);
	if (ret)
		return ret;

	mutex_lock(&device->mutex);
	level = _get_nearest_pwrlevel(pwr, val);
	if (level < 0)
		goto done;

	pwr->min_pwrlevel = level;

	kgsl_pwrctrl_pwrlevel_change(device, pwr->min_pwrlevel);

done:
	mutex_unlock(&device->mutex);
	return count;
}

static int kgsl_pwrctrl_min_gpuclk_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{

	struct kgsl_device *device = kgsl_device_from_dev(dev);
	struct kgsl_pwrctrl *pwr;
	if (device == NULL)
		return 0;
	pwr = &device->pwrctrl;
	return snprintf(buf, PAGE_SIZE, "%d\n",
			pwr->pwrlevels[pwr->min_pwrlevel].gpu_freq);
}

static int kgsl_pwrctrl_gpuclk_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	struct kgsl_device *device = kgsl_device_from_dev(dev);
	struct kgsl_pwrctrl *pwr;
	unsigned int val = 0;
	int ret, level;

	if (device == NULL)
		return 0;

	pwr = &device->pwrctrl;

	ret = kgsl_sysfs_store(buf, &val);
	if (ret)
		return ret;

	mutex_lock(&device->mutex);
	level = _get_nearest_pwrlevel(pwr, val);
	if (level >= 0)
		kgsl_pwrctrl_pwrlevel_change(device, level);

	mutex_unlock(&device->mutex);
	return count;
}

static int kgsl_pwrctrl_gpuclk_show(struct device *dev,
				    struct device_attribute *attr,
				    char *buf)
{
	struct kgsl_device *device = kgsl_device_from_dev(dev);
	struct kgsl_pwrctrl *pwr;
	if (device == NULL)
		return 0;
	pwr = &device->pwrctrl;
	return snprintf(buf, PAGE_SIZE, "%d\n",
			pwr->pwrlevels[pwr->active_pwrlevel].gpu_freq);
}

static int kgsl_pwrctrl_pwrnap_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	unsigned int val = 0;
	struct kgsl_device *device = kgsl_device_from_dev(dev);
	struct kgsl_pwrctrl *pwr;
	int ret;

	if (device == NULL)
		return 0;
	pwr = &device->pwrctrl;

	ret = kgsl_sysfs_store(buf, &val);
	if (ret)
		return ret;

	mutex_lock(&device->mutex);

	if (val == 1)
		pwr->nap_allowed = true;
	else if (val == 0)
		pwr->nap_allowed = false;

	mutex_unlock(&device->mutex);

	return count;
}

static int kgsl_pwrctrl_pwrnap_show(struct device *dev,
				    struct device_attribute *attr,
				    char *buf)
{
	struct kgsl_device *device = kgsl_device_from_dev(dev);
	if (device == NULL)
		return 0;
	return snprintf(buf, PAGE_SIZE, "%d\n", device->pwrctrl.nap_allowed);
}


static int kgsl_pwrctrl_idle_timer_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	unsigned int val = 0;
	struct kgsl_device *device = kgsl_device_from_dev(dev);
	struct kgsl_pwrctrl *pwr;
	static unsigned int org_interval_timeout = 1;
	int ret;

	if (device == NULL)
		return 0;
	pwr = &device->pwrctrl;

	ret = kgsl_sysfs_store(buf, &val);
	if (ret)
		return ret;

	if (org_interval_timeout == 1)
		org_interval_timeout = pwr->interval_timeout;

	mutex_lock(&device->mutex);

	/* Let the timeout be requested in ms, but convert to jiffies. */
	if (val >= org_interval_timeout)
		device->pwrctrl.interval_timeout = msecs_to_jiffies(val);

	mutex_unlock(&device->mutex);

	return count;
}

static int kgsl_pwrctrl_idle_timer_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct kgsl_device *device = kgsl_device_from_dev(dev);
	if (device == NULL)
		return 0;
	return snprintf(buf, PAGE_SIZE, "%d\n",
		jiffies_to_msecs(device->pwrctrl.interval_timeout));
}

static int kgsl_pwrctrl_pmqos_latency_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	char temp[20];
	unsigned long val;
	struct kgsl_device *device = kgsl_device_from_dev(dev);
	int rc;

	if (device == NULL)
		return 0;

	snprintf(temp, sizeof(temp), "%.*s",
			(int)min(count, sizeof(temp) - 1), buf);
	rc = kstrtoul(temp, 0, &val);
	if (rc)
		return rc;

	mutex_lock(&device->mutex);
	device->pwrctrl.pm_qos_latency = val;
	mutex_unlock(&device->mutex);

	return count;
}

static int kgsl_pwrctrl_pmqos_latency_show(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	struct kgsl_device *device = kgsl_device_from_dev(dev);
	if (device == NULL)
		return 0;
	return snprintf(buf, PAGE_SIZE, "%d\n",
		device->pwrctrl.pm_qos_latency);
}

static int kgsl_pwrctrl_gpubusy_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int ret;
	struct kgsl_device *device = kgsl_device_from_dev(dev);
	struct kgsl_clk_stats *clkstats;

	if (device == NULL)
		return 0;
	clkstats = &device->pwrctrl.clk_stats;
	ret = snprintf(buf, PAGE_SIZE, "%7d %7d\n",
			clkstats->on_time_old, clkstats->elapsed_old);
	if (!test_bit(KGSL_PWRFLAGS_AXI_ON, &device->pwrctrl.power_flags)) {
		clkstats->on_time_old = 0;
		clkstats->elapsed_old = 0;
	}
	return ret;
}

static int kgsl_pwrctrl_gputop_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int ret;
	struct kgsl_device *device = kgsl_device_from_dev(dev);
	struct kgsl_clk_stats *clkstats;
	int i = 0;
	char *ptr = buf;

	if (device == NULL)
		return 0;
	clkstats = &device->pwrctrl.clk_stats;
	ret = snprintf(buf, PAGE_SIZE, "%7d %7d ", clkstats->on_time_old,
					clkstats->elapsed_old);
	for (i = 0, ptr += ret; i < device->pwrctrl.num_pwrlevels;
							i++, ptr += ret)
		ret = snprintf(ptr, PAGE_SIZE, "%7d ",
						clkstats->old_clock_time[i]);

	if (!test_bit(KGSL_PWRFLAGS_AXI_ON, &device->pwrctrl.power_flags)) {
		clkstats->on_time_old = 0;
		clkstats->elapsed_old = 0;
		for (i = 0; i < KGSL_MAX_PWRLEVELS ; i++)
			clkstats->old_clock_time[i] = 0;
	}
	return (unsigned int) (ptr - buf);
}

static int kgsl_pwrctrl_gpu_available_frequencies_show(
					struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct kgsl_device *device = kgsl_device_from_dev(dev);
	struct kgsl_pwrctrl *pwr;
	int index, num_chars = 0;

	if (device == NULL)
		return 0;
	pwr = &device->pwrctrl;
	for (index = 0; index < pwr->num_pwrlevels - 1; index++)
		num_chars += snprintf(buf + num_chars, PAGE_SIZE, "%d ",
		pwr->pwrlevels[index].gpu_freq);
	buf[num_chars++] = '\n';
	return num_chars;
}

static int kgsl_pwrctrl_reset_count_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct kgsl_device *device = kgsl_device_from_dev(dev);
	if (device == NULL)
		return 0;
	return snprintf(buf, PAGE_SIZE, "%d\n", device->reset_counter);
}

#ifdef CONFIG_MSM_KGSL_KERNEL_API_ENABLE
int kgsl_pwrctrl_min_pwrlevel_store_kernel(int level)
{
	struct device *dev = stored_dev;
	struct kgsl_device *device;
	int request_level = level;
	char buf_level[2] = {0,};

	if (!dev) {
		printk("%s, dev is null\n", __func__);
		return -EINVAL;
	}

	device = kgsl_device_from_dev(dev);

	if (!device) {
		printk("%s, fail to get device\n", __func__);
		return -EINVAL;
	}

	if (request_level < 0) {
		printk("%s, invalid level : %d\n", __func__, request_level);
		return -EINVAL;
	}

	if (request_level > device->pwrctrl.num_pwrlevels - 2)
		request_level = device->pwrctrl.num_pwrlevels - 2;

	buf_level[0] = (char)(request_level + '0');

	return kgsl_pwrctrl_min_pwrlevel_store(dev, NULL, buf_level, sizeof(buf_level));
}

int kgsl_pwrctrl_num_pwrlevels_show_kernel(void)
{

	struct kgsl_device *device = kgsl_device_from_dev(stored_dev);
	struct kgsl_pwrctrl *pwr;
	if (device == NULL)
		return 0;

	pwr = &device->pwrctrl;
	return pwr->num_pwrlevels - 1;
}
#endif

#if defined(CONFIG_MSM_KGSL_FPS_NODE_ENABLE)
static void fps_store(char *src, int *dst, int count)
{
	u8 i;

	for (i = 0; i <= count; i++) {
		src[i] -= '0';
		if (src[i] >= 0 && src[i] <= 9) {
			*dst += src[i];
		} else {
			*dst /= 10;
			break;
		}
		*dst *= 10;
	}
}

static int kgsl_fps_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	char temp[3];
	struct kgsl_device *device = kgsl_device_from_dev(dev);

	if (device == NULL)
		return 0;

	snprintf(temp, sizeof(temp), "%.*s",
			 (int)min(count, sizeof(temp) - 1), buf);

	mutex_lock(&device->mutex);

	fps = 0;
	fps_store(temp, &fps, (int)min(count, sizeof(temp) - 1));

	mutex_unlock(&device->mutex);

	return count;
}

static int kgsl_fps_show(struct device *dev,
				    struct device_attribute *attr,
				    char *buf)
{
	struct kgsl_device *device = kgsl_device_from_dev(dev);
	if (device == NULL)
		return 0;
	return snprintf(buf, PAGE_SIZE, "%d\n", fps);
}

static int kgsl_max_fps_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	char temp[3];
	struct kgsl_device *device = kgsl_device_from_dev(dev);

	if (device == NULL)
		return 0;

	snprintf(temp, sizeof(temp), "%.*s",
			 (int)min(count, sizeof(temp) - 1), buf);

	mutex_lock(&device->mutex);

	max_fps = 0;
	fps_store(temp, &max_fps, (int)min(count, sizeof(temp) - 1));

	mutex_unlock(&device->mutex);

	return count;
}

static int kgsl_max_fps_show(struct device *dev,
				    struct device_attribute *attr,
				    char *buf)
{
	struct kgsl_device *device = kgsl_device_from_dev(dev);
	if (device == NULL)
		return 0;
	return snprintf(buf, PAGE_SIZE, "%d\n", max_fps);
}

DEVICE_ATTR(fps, 0664, kgsl_fps_show, kgsl_fps_store);
DEVICE_ATTR(max_fps, 0664, kgsl_max_fps_show, kgsl_max_fps_store);
#endif
DEVICE_ATTR(gpuclk, 0644, kgsl_pwrctrl_gpuclk_show, kgsl_pwrctrl_gpuclk_store);
DEVICE_ATTR(max_gpuclk, 0644, kgsl_pwrctrl_max_gpuclk_show,
	kgsl_pwrctrl_max_gpuclk_store);
DEVICE_ATTR(min_gpuclk, 0644, kgsl_pwrctrl_min_gpuclk_show,
	kgsl_pwrctrl_min_gpuclk_store);
DEVICE_ATTR(pwrnap, 0664, kgsl_pwrctrl_pwrnap_show, kgsl_pwrctrl_pwrnap_store);
DEVICE_ATTR(idle_timer, 0644, kgsl_pwrctrl_idle_timer_show,
	kgsl_pwrctrl_idle_timer_store);
DEVICE_ATTR(gpubusy, 0444, kgsl_pwrctrl_gpubusy_show,
	NULL);
DEVICE_ATTR(gputop, 0444, kgsl_pwrctrl_gputop_show,
	NULL);
DEVICE_ATTR(gpu_available_frequencies, 0444,
	kgsl_pwrctrl_gpu_available_frequencies_show,
	NULL);
DEVICE_ATTR(max_pwrlevel, 0644,
	kgsl_pwrctrl_max_pwrlevel_show,
	kgsl_pwrctrl_max_pwrlevel_store);
DEVICE_ATTR(min_pwrlevel, 0644,
	kgsl_pwrctrl_min_pwrlevel_show,
	kgsl_pwrctrl_min_pwrlevel_store);
DEVICE_ATTR(thermal_pwrlevel, 0644,
	kgsl_pwrctrl_thermal_pwrlevel_show,
	kgsl_pwrctrl_thermal_pwrlevel_store);
DEVICE_ATTR(num_pwrlevels, 0444,
	kgsl_pwrctrl_num_pwrlevels_show,
	NULL);
DEVICE_ATTR(reset_count, 0444,
	kgsl_pwrctrl_reset_count_show,
	NULL);
DEVICE_ATTR(pmqos_latency, 0644,
	kgsl_pwrctrl_pmqos_latency_show,
	kgsl_pwrctrl_pmqos_latency_store);

static const struct device_attribute *pwrctrl_attr_list[] = {
	&dev_attr_gpuclk,
	&dev_attr_max_gpuclk,
	&dev_attr_min_gpuclk,
	&dev_attr_pwrnap,
	&dev_attr_idle_timer,
	&dev_attr_gpubusy,
	&dev_attr_gputop,
	&dev_attr_gpu_available_frequencies,
	&dev_attr_max_pwrlevel,
	&dev_attr_min_pwrlevel,
	&dev_attr_thermal_pwrlevel,
	&dev_attr_num_pwrlevels,
	&dev_attr_pmqos_latency,
	&dev_attr_reset_count,
#if defined(CONFIG_MSM_KGSL_FPS_NODE_ENABLE)
	&dev_attr_fps,
	&dev_attr_max_fps,
#endif
	NULL
};

int kgsl_pwrctrl_init_sysfs(struct kgsl_device *device)
{
#ifdef CONFIG_MSM_KGSL_KERNEL_API_ENABLE
	stored_dev = device->dev;
#endif
	return kgsl_create_device_sysfs_files(device->dev, pwrctrl_attr_list);
}

void kgsl_pwrctrl_uninit_sysfs(struct kgsl_device *device)
{
	kgsl_remove_device_sysfs_files(device->dev, pwrctrl_attr_list);
}

static void update_statistics(struct kgsl_device *device)
{
	struct kgsl_clk_stats *clkstats = &device->pwrctrl.clk_stats;
	unsigned int on_time = 0;
	int i;
	int num_pwrlevels = device->pwrctrl.num_pwrlevels - 1;
	/*PER CLK TIME*/
	for (i = 0; i < num_pwrlevels; i++) {
		clkstats->old_clock_time[i] = clkstats->clock_time[i];
		on_time += clkstats->clock_time[i];
		clkstats->clock_time[i] = 0;
	}
	clkstats->old_clock_time[num_pwrlevels] =
				clkstats->clock_time[num_pwrlevels];
	clkstats->clock_time[num_pwrlevels] = 0;
	clkstats->on_time_old = on_time;
	clkstats->elapsed_old = clkstats->elapsed;
	clkstats->elapsed = 0;
}

/* Track the amount of time the gpu is on vs the total system time. *
 * Regularly update the percentage of busy time displayed by sysfs. */
static void kgsl_pwrctrl_busy_time(struct kgsl_device *device, bool on_time)
{
	struct kgsl_clk_stats *clkstats = &device->pwrctrl.clk_stats;
	update_clk_statistics(device, on_time);
	/* Update the output regularly and reset the counters. */
	if ((clkstats->elapsed > UPDATE_BUSY_VAL) ||
		!test_bit(KGSL_PWRFLAGS_AXI_ON, &device->pwrctrl.power_flags)) {
		update_statistics(device);
	}
}

void kgsl_pwrctrl_clk(struct kgsl_device *device, int state,
					  int requested_state)
{
	struct kgsl_pwrctrl *pwr = &device->pwrctrl;
	int i = 0;
	if (state == KGSL_PWRFLAGS_OFF) {
		if (test_and_clear_bit(KGSL_PWRFLAGS_CLK_ON,
			&pwr->power_flags)) {
			trace_kgsl_clk(device, state);
			for (i = KGSL_MAX_CLKS - 1; i > 0; i--)
				if (pwr->grp_clks[i])
					clk_disable(pwr->grp_clks[i]);
			/* High latency clock maintenance. */
			if ((pwr->pwrlevels[0].gpu_freq > 0) &&
				(requested_state != KGSL_STATE_NAP)) {
				for (i = KGSL_MAX_CLKS - 1; i > 0; i--)
					if (pwr->grp_clks[i])
						clk_unprepare(pwr->grp_clks[i]);
				clk_set_rate(pwr->grp_clks[0],
					pwr->pwrlevels[pwr->num_pwrlevels - 1].
					gpu_freq);
			}
			kgsl_pwrctrl_busy_time(device, true);
		} else if (requested_state == KGSL_STATE_SLEEP) {
			/* High latency clock maintenance. */
			for (i = KGSL_MAX_CLKS - 1; i > 0; i--)
				if (pwr->grp_clks[i])
					clk_unprepare(pwr->grp_clks[i]);
			if ((pwr->pwrlevels[0].gpu_freq > 0))
				clk_set_rate(pwr->grp_clks[0],
					pwr->pwrlevels[pwr->num_pwrlevels - 1].
					gpu_freq);
		}
	} else if (state == KGSL_PWRFLAGS_ON) {
		if (!test_and_set_bit(KGSL_PWRFLAGS_CLK_ON,
			&pwr->power_flags)) {
			trace_kgsl_clk(device, state);
			/* High latency clock maintenance. */
			if (device->state != KGSL_STATE_NAP) {
				if (pwr->pwrlevels[0].gpu_freq > 0)
					clk_set_rate(pwr->grp_clks[0],
						pwr->pwrlevels
						[pwr->active_pwrlevel].
						gpu_freq);
				for (i = KGSL_MAX_CLKS - 1; i > 0; i--)
					if (pwr->grp_clks[i])
						clk_prepare(pwr->grp_clks[i]);
			}
			/* as last step, enable grp_clk
			   this is to let GPU interrupt to come */
			for (i = KGSL_MAX_CLKS - 1; i > 0; i--)
				if (pwr->grp_clks[i])
					clk_enable(pwr->grp_clks[i]);
			kgsl_pwrctrl_busy_time(device, false);
		}
	}
}

void kgsl_pwrctrl_axi(struct kgsl_device *device, int state)
{
	struct kgsl_pwrctrl *pwr = &device->pwrctrl;

	if (state == KGSL_PWRFLAGS_OFF) {
		if (test_and_clear_bit(KGSL_PWRFLAGS_AXI_ON,
			&pwr->power_flags)) {
			trace_kgsl_bus(device, state);
			if (pwr->ebi1_clk) {
				clk_set_rate(pwr->ebi1_clk, 0);
				clk_disable_unprepare(pwr->ebi1_clk);
			}
			if (pwr->pcl)
				msm_bus_scale_client_update_request(pwr->pcl,
								    0);
		}
	} else if (state == KGSL_PWRFLAGS_ON) {
		if (!test_and_set_bit(KGSL_PWRFLAGS_AXI_ON,
			&pwr->power_flags)) {
			trace_kgsl_bus(device, state);
			if (pwr->ebi1_clk) {
				clk_prepare_enable(pwr->ebi1_clk);
				clk_set_rate(pwr->ebi1_clk,
					pwr->pwrlevels[pwr->active_pwrlevel].
					bus_freq);
			}
			if (pwr->pcl)
				msm_bus_scale_client_update_request(pwr->pcl,
					pwr->pwrlevels[pwr->active_pwrlevel].
						bus_freq);
		}
	}
}

void kgsl_pwrctrl_pwrrail(struct kgsl_device *device, int state)
{
	struct kgsl_pwrctrl *pwr = &device->pwrctrl;

	if (state == KGSL_PWRFLAGS_OFF) {
		if (test_and_clear_bit(KGSL_PWRFLAGS_POWER_ON,
			&pwr->power_flags)) {
			trace_kgsl_rail(device, state);
			if (pwr->gpu_cx)
				regulator_disable(pwr->gpu_cx);
			if (pwr->gpu_reg)
				regulator_disable(pwr->gpu_reg);
		}
	} else if (state == KGSL_PWRFLAGS_ON) {
		if (!test_and_set_bit(KGSL_PWRFLAGS_POWER_ON,
			&pwr->power_flags)) {
			trace_kgsl_rail(device, state);
			if (pwr->gpu_reg) {
				int status = regulator_enable(pwr->gpu_reg);
				if (status)
					KGSL_DRV_ERR(device,
							"core regulator_enable "
							"failed: %d\n",
							status);
			}
			if (pwr->gpu_cx) {
				int status = regulator_enable(pwr->gpu_cx);
				if (status)
					KGSL_DRV_ERR(device,
							"cx regulator_enable "
							"failed: %d\n",
							status);
			}
		}
	}
}

void kgsl_pwrctrl_irq(struct kgsl_device *device, int state)
{
	struct kgsl_pwrctrl *pwr = &device->pwrctrl;

	if (state == KGSL_PWRFLAGS_ON) {
		if (!test_and_set_bit(KGSL_PWRFLAGS_IRQ_ON,
			&pwr->power_flags)) {
			trace_kgsl_irq(device, state);
			enable_irq(pwr->interrupt_num);
		}
	} else if (state == KGSL_PWRFLAGS_OFF) {
		if (test_and_clear_bit(KGSL_PWRFLAGS_IRQ_ON,
			&pwr->power_flags)) {
			trace_kgsl_irq(device, state);
			if (in_interrupt())
				disable_irq_nosync(pwr->interrupt_num);
			else
				disable_irq(pwr->interrupt_num);
		}
	}
}
EXPORT_SYMBOL(kgsl_pwrctrl_irq);

int kgsl_pwrctrl_init(struct kgsl_device *device)
{
	int i, result = 0;
	struct clk *clk;
	struct platform_device *pdev =
		container_of(device->parentdev, struct platform_device, dev);
	struct kgsl_pwrctrl *pwr = &device->pwrctrl;
	struct kgsl_device_platform_data *pdata = pdev->dev.platform_data;

	/*acquire clocks */
	for (i = 0; i < KGSL_MAX_CLKS; i++) {
		if (pdata->clk_map & clks[i].map) {
			clk = clk_get(&pdev->dev, clks[i].name);
			if (IS_ERR(clk))
				goto clk_err;
			pwr->grp_clks[i] = clk;
		}
	}
	/* Make sure we have a source clk for freq setting */
	if (pwr->grp_clks[0] == NULL)
		pwr->grp_clks[0] = pwr->grp_clks[1];

	/* put the AXI bus into asynchronous mode with the graphics cores */
	if (pdata->set_grp_async != NULL)
		pdata->set_grp_async();

	if (pdata->num_levels > KGSL_MAX_PWRLEVELS ||
	    pdata->num_levels < 1) {
		KGSL_PWR_ERR(device, "invalid power level count: %d\n",
					 pdata->num_levels);
		result = -EINVAL;
		goto done;
	}
	pwr->num_pwrlevels = pdata->num_levels;

	/* Initialize the user and thermal clock constraints */

	pwr->max_pwrlevel = 0;
	pwr->min_pwrlevel = pdata->num_levels - 2;
	pwr->thermal_pwrlevel = 1;

	pwr->active_pwrlevel = pdata->init_level;
	pwr->default_pwrlevel = pdata->init_level;
	pwr->init_pwrlevel = pdata->init_level;
	for (i = 0; i < pdata->num_levels; i++) {
		pwr->pwrlevels[i].gpu_freq =
		(pdata->pwrlevel[i].gpu_freq > 0) ?
		clk_round_rate(pwr->grp_clks[0],
					   pdata->pwrlevel[i].
					   gpu_freq) : 0;
		pwr->pwrlevels[i].bus_freq =
			pdata->pwrlevel[i].bus_freq;
		pwr->pwrlevels[i].io_fraction =
			pdata->pwrlevel[i].io_fraction;
	}
	/* Do not set_rate for targets in sync with AXI */
	if (pwr->pwrlevels[0].gpu_freq > 0)
		clk_set_rate(pwr->grp_clks[0], pwr->
				pwrlevels[pwr->num_pwrlevels - 1].gpu_freq);

	pwr->gpu_reg = regulator_get(&pdev->dev, "vdd");
	if (IS_ERR(pwr->gpu_reg))
		pwr->gpu_reg = NULL;

	if (pwr->gpu_reg) {
		pwr->gpu_cx = regulator_get(&pdev->dev, "vddcx");
		if (IS_ERR(pwr->gpu_cx))
			pwr->gpu_cx = NULL;
	} else
		pwr->gpu_cx = NULL;

	pwr->power_flags = 0;

	pwr->nap_allowed = pdata->nap_allowed;
	pwr->idle_needed = pdata->idle_needed;
	pwr->interval_timeout = msecs_to_jiffies(pdata->idle_timeout);
	pwr->strtstp_sleepwake = pdata->strtstp_sleepwake;
	pwr->ebi1_clk = clk_get(&pdev->dev, "bus_clk");
	if (IS_ERR(pwr->ebi1_clk))
		pwr->ebi1_clk = NULL;
	else
		clk_set_rate(pwr->ebi1_clk,
					 pwr->pwrlevels[pwr->active_pwrlevel].
						bus_freq);
	if (pdata->bus_scale_table != NULL) {
		pwr->pcl = msm_bus_scale_register_client(pdata->
							bus_scale_table);
		if (!pwr->pcl) {
			KGSL_PWR_ERR(device,
					"msm_bus_scale_register_client failed: "
					"id %d table %p", device->id,
					pdata->bus_scale_table);
			result = -EINVAL;
			goto done;
		}
	}

	/* Set the CPU latency to 501usec to allow low latency PC modes */
	pwr->pm_qos_latency = 501;

	pm_runtime_enable(device->parentdev);
	register_power_suspend(&device->display_off);
	return result;

clk_err:
	result = PTR_ERR(clk);
	KGSL_PWR_ERR(device, "clk_get(%s) failed: %d\n",
				 clks[i].name, result);

done:
	return result;
}

void kgsl_pwrctrl_close(struct kgsl_device *device)
{
	struct kgsl_pwrctrl *pwr = &device->pwrctrl;
	int i;

	KGSL_PWR_INFO(device, "close device %d\n", device->id);

	pm_runtime_disable(device->parentdev);
	unregister_power_suspend(&device->display_off);

	clk_put(pwr->ebi1_clk);

	if (pwr->pcl)
		msm_bus_scale_unregister_client(pwr->pcl);

	pwr->pcl = 0;

	if (pwr->gpu_reg) {
		regulator_put(pwr->gpu_reg);
		pwr->gpu_reg = NULL;
	}

	if (pwr->gpu_cx) {
		regulator_put(pwr->gpu_cx);
		pwr->gpu_cx = NULL;
	}

	for (i = 1; i < KGSL_MAX_CLKS; i++)
		if (pwr->grp_clks[i]) {
			clk_put(pwr->grp_clks[i]);
			pwr->grp_clks[i] = NULL;
		}

	pwr->grp_clks[0] = NULL;
	pwr->power_flags = 0;
}

/**
 * kgsl_idle_check() - Work function for GPU interrupts and idle timeouts.
 * @device: The device
 *
 * This function is called for work that is queued by the interrupt
 * handler or the idle timer. It attempts to transition to a clocks
 * off state if the active_cnt is 0 and the hardware is idle.
 */
void kgsl_idle_check(struct work_struct *work)
{
	struct kgsl_device *device = container_of(work, struct kgsl_device,
							idle_check_ws);
	WARN_ON(device == NULL);
	if (device == NULL)
		return;

	mutex_lock(&device->mutex);

	kgsl_pwrscale_idle(device);

	if (device->state == KGSL_STATE_ACTIVE
		   || device->state ==  KGSL_STATE_NAP) {
		if (device->active_cnt > 0 || kgsl_pwrctrl_sleep(device) != 0) {

			kgsl_pwrctrl_request_state(device, KGSL_STATE_NONE);

			mod_timer(&device->idle_timer,
					jiffies +
					device->pwrctrl.interval_timeout);
			/*
			 * If the GPU has been too busy to sleep, make sure
			 * that is acurately reflected in the % busy numbers.
			 */
			device->pwrctrl.clk_stats.no_nap_cnt++;
			if (device->pwrctrl.clk_stats.no_nap_cnt >
							 UPDATE_BUSY) {
				kgsl_pwrctrl_busy_time(device, true);
				device->pwrctrl.clk_stats.no_nap_cnt = 0;
			}
		}
	} else if (device->state & (KGSL_STATE_HUNG |
					KGSL_STATE_DUMP_AND_FT)) {
		kgsl_pwrctrl_request_state(device, KGSL_STATE_NONE);
	}

	mutex_unlock(&device->mutex);
}
EXPORT_SYMBOL(kgsl_idle_check);

void kgsl_timer(unsigned long data)
{
	struct kgsl_device *device = (struct kgsl_device *) data;

	KGSL_PWR_INFO(device, "idle timer expired device %d From %s to %s \n", device->id,
			kgsl_pwrstate_to_str( device->state),
			kgsl_pwrstate_to_str(device->requested_state));

	if (device->requested_state != KGSL_STATE_SUSPEND) {
		if (device->pwrctrl.restore_slumber ||
					device->pwrctrl.strtstp_sleepwake)
			kgsl_pwrctrl_request_state(device, KGSL_STATE_SLUMBER);
		else
			kgsl_pwrctrl_request_state(device, KGSL_STATE_SLEEP);
		/* Have work run in a non-interrupt context. */
		queue_work(device->work_queue, &device->idle_check_ws);
	}
}

bool kgsl_pwrctrl_isenabled(struct kgsl_device *device)
{
	struct kgsl_pwrctrl *pwr = &device->pwrctrl;
	return (test_bit(KGSL_PWRFLAGS_CLK_ON, &pwr->power_flags) != 0);
}

/**
 * kgsl_pre_hwaccess - Enforce preconditions for touching registers
 * @device: The device
 *
 * This function ensures that the correct lock is held and that the GPU
 * clock is on immediately before a register is read or written. Note
 * that this function does not check active_cnt because the registers
 * must be accessed during device start and stop, when the active_cnt
 * may legitimately be 0.
 */
void kgsl_pre_hwaccess(struct kgsl_device *device)
{
	/* In order to touch a register you must hold the device mutex...*/
	BUG_ON(!mutex_is_locked(&device->mutex));
	/* and have the clock on! */
	BUG_ON(!kgsl_pwrctrl_isenabled(device));
}
EXPORT_SYMBOL(kgsl_pre_hwaccess);

static int
_nap(struct kgsl_device *device)
{
	switch (device->state) {
	case KGSL_STATE_ACTIVE:
		if (!device->ftbl->isidle(device)) {
			kgsl_pwrctrl_request_state(device, KGSL_STATE_NONE);
			return -EBUSY;
		}
		del_timer_sync(&device->hang_timer);
		kgsl_pwrctrl_irq(device, KGSL_PWRFLAGS_OFF);
		kgsl_pwrctrl_clk(device, KGSL_PWRFLAGS_OFF, KGSL_STATE_NAP);
		kgsl_pwrctrl_set_state(device, KGSL_STATE_NAP);
	case KGSL_STATE_NAP:
	case KGSL_STATE_SLEEP:
	case KGSL_STATE_SLUMBER:
		break;
	default:
		kgsl_pwrctrl_request_state(device, KGSL_STATE_NONE);
		break;
	}
	return 0;
}

static void
_sleep_accounting(struct kgsl_device *device)
{
	kgsl_pwrctrl_busy_time(device, false);
	device->pwrctrl.clk_stats.start = ktime_set(0, 0);
	device->pwrctrl.time = 0;
	kgsl_pwrscale_sleep(device);
}

static int
_sleep(struct kgsl_device *device)
{
	switch (device->state) {
	case KGSL_STATE_ACTIVE:
		if (!device->ftbl->isidle(device)) {
			kgsl_pwrctrl_request_state(device, KGSL_STATE_NONE);
			return -EBUSY;
		}
		/* fall through */
	case KGSL_STATE_NAP:
		kgsl_pwrctrl_irq(device, KGSL_PWRFLAGS_OFF);
		kgsl_pwrctrl_axi(device, KGSL_PWRFLAGS_OFF);
		_sleep_accounting(device);
		kgsl_pwrctrl_clk(device, KGSL_PWRFLAGS_OFF, KGSL_STATE_SLEEP);
		kgsl_pwrctrl_set_state(device, KGSL_STATE_SLEEP);
		pm_qos_update_request(&device->pwrctrl.pm_qos_req_dma,
					PM_QOS_DEFAULT_VALUE);
		break;
	case KGSL_STATE_SLEEP:
	case KGSL_STATE_SLUMBER:
		break;
	default:
		KGSL_PWR_WARN(device, "unhandled state %s\n",
				kgsl_pwrstate_to_str(device->state));
		break;
	}

	kgsl_mmu_disable_clk_on_ts(&device->mmu, 0, false);

	return 0;
}

static int
_slumber(struct kgsl_device *device)
{
	switch (device->state) {
	case KGSL_STATE_ACTIVE:
		if (!device->ftbl->isidle(device)) {
			kgsl_pwrctrl_request_state(device, KGSL_STATE_NONE);
			return -EBUSY;
		}
		/* fall through */
	case KGSL_STATE_NAP:
	case KGSL_STATE_SLEEP:
		del_timer_sync(&device->idle_timer);
		del_timer_sync(&device->hang_timer);
		/* make sure power is on to stop the device*/
		kgsl_pwrctrl_enable(device);
		kgsl_pwrctrl_irq(device, KGSL_PWRFLAGS_ON);
		device->ftbl->suspend_context(device);
		device->ftbl->stop(device);
		_sleep_accounting(device);
		kgsl_pwrctrl_set_state(device, KGSL_STATE_SLUMBER);
		pm_qos_update_request(&device->pwrctrl.pm_qos_req_dma,
						PM_QOS_DEFAULT_VALUE);
		break;
	case KGSL_STATE_SLUMBER:
		break;
	default:
		KGSL_PWR_WARN(device, "unhandled state %s\n",
				kgsl_pwrstate_to_str(device->state));
		break;
	}
	return 0;
}

/******************************************************************/
/* Caller must hold the device mutex. */
int kgsl_pwrctrl_sleep(struct kgsl_device *device)
{
	int status = 0;
	KGSL_PWR_INFO(device, "sleep device %d From %s to %s \n", device->id,
			kgsl_pwrstate_to_str(device->state), kgsl_pwrstate_to_str(device->requested_state));

	/* Work through the legal state transitions */
	switch (device->requested_state) {
	case KGSL_STATE_NAP:
		status = _nap(device);
		break;
	case KGSL_STATE_SLEEP:
		status = _sleep(device);
		kgsl_mmu_disable_clk_on_ts(&device->mmu, 0, false);
		break;
	case KGSL_STATE_SLUMBER:
		status = _slumber(device);
		break;
	default:
		KGSL_PWR_INFO(device, "bad state request 0x%x\n",
				device->requested_state);
		kgsl_pwrctrl_request_state(device, KGSL_STATE_NONE);
		status = -EINVAL;
		break;
	}
	return status;
}
EXPORT_SYMBOL(kgsl_pwrctrl_sleep);

/******************************************************************/
/* Caller must hold the device mutex. */
int kgsl_pwrctrl_wake(struct kgsl_device *device)
{
	int status = 0;
	unsigned int context_id;
	unsigned int state = device->state;
	unsigned int ts_processed = 0xdeaddead;
	struct kgsl_context *context;

	kgsl_pwrctrl_request_state(device, KGSL_STATE_ACTIVE);
	switch (device->state) {
	case KGSL_STATE_SLUMBER:
		status = device->ftbl->start(device);
		if (status) {
			kgsl_pwrctrl_request_state(device, KGSL_STATE_NONE);
			KGSL_DRV_ERR(device, "start failed %d\n", status);
			break;
		}
		/* fall through */
	case KGSL_STATE_SLEEP:
		kgsl_pwrctrl_axi(device, KGSL_PWRFLAGS_ON);
		kgsl_pwrscale_wake(device);
		kgsl_sharedmem_readl(&device->memstore,
			(unsigned int *) &context_id,
			KGSL_MEMSTORE_OFFSET(KGSL_MEMSTORE_GLOBAL,
				current_context));
		context = kgsl_context_get(device, context_id);
		if (context)
			ts_processed = kgsl_readtimestamp(device, context,
				KGSL_TIMESTAMP_RETIRED);
		KGSL_PWR_INFO(device, "Wake from %s state to %s. CTXT: %d RTRD TS: %08X\n",
			kgsl_pwrstate_to_str(state), kgsl_pwrstate_to_str(device->requested_state),
			context ? context->id : -1, ts_processed);
		kgsl_context_put(context);
		/* fall through */
	case KGSL_STATE_NAP:
		/* Turn on the core clocks */
		kgsl_pwrctrl_clk(device, KGSL_PWRFLAGS_ON, KGSL_STATE_ACTIVE);
		/* Enable state before turning on irq */
		kgsl_pwrctrl_set_state(device, KGSL_STATE_ACTIVE);
		kgsl_pwrctrl_irq(device, KGSL_PWRFLAGS_ON);
		/* Re-enable HW access */
		mod_timer(&device->idle_timer,
				jiffies + device->pwrctrl.interval_timeout);
		mod_timer(&device->hang_timer,
			(jiffies + msecs_to_jiffies(KGSL_TIMEOUT_HANG_DETECT)));
		pm_qos_update_request(&device->pwrctrl.pm_qos_req_dma,
				device->pwrctrl.pm_qos_latency);
	case KGSL_STATE_ACTIVE:
		kgsl_pwrctrl_request_state(device, KGSL_STATE_NONE);
		break;
	case KGSL_STATE_INIT:
		break;
	default:
		KGSL_PWR_WARN(device, "unhandled state %s\n",
				kgsl_pwrstate_to_str(device->state));
		kgsl_pwrctrl_request_state(device, KGSL_STATE_NONE);
		status = -EINVAL;
		break;
	}
	return status;
}
EXPORT_SYMBOL(kgsl_pwrctrl_wake);

void kgsl_pwrctrl_enable(struct kgsl_device *device)
{
	struct kgsl_pwrctrl *pwr = &device->pwrctrl;
	/* Order pwrrail/clk sequence based upon platform */
	kgsl_pwrctrl_pwrrail(device, KGSL_PWRFLAGS_ON);
	kgsl_pwrctrl_pwrlevel_change(device, pwr->default_pwrlevel);

	kgsl_pwrctrl_clk(device, KGSL_PWRFLAGS_ON, KGSL_STATE_ACTIVE);
	kgsl_pwrctrl_axi(device, KGSL_PWRFLAGS_ON);
}
EXPORT_SYMBOL(kgsl_pwrctrl_enable);

void kgsl_pwrctrl_disable(struct kgsl_device *device)
{
	/* Order pwrrail/clk sequence based upon platform */
	kgsl_pwrctrl_axi(device, KGSL_PWRFLAGS_OFF);
	kgsl_pwrctrl_clk(device, KGSL_PWRFLAGS_OFF, KGSL_STATE_SLEEP);
	kgsl_pwrctrl_pwrrail(device, KGSL_PWRFLAGS_OFF);
}
EXPORT_SYMBOL(kgsl_pwrctrl_disable);

void kgsl_pwrctrl_set_state(struct kgsl_device *device, unsigned int state)
{
	trace_kgsl_pwr_set_state(device, state);
	device->state = state;
	device->requested_state = KGSL_STATE_NONE;
}
EXPORT_SYMBOL(kgsl_pwrctrl_set_state);

void kgsl_pwrctrl_request_state(struct kgsl_device *device, unsigned int state)
{
	if (state != KGSL_STATE_NONE && state != device->requested_state)
		trace_kgsl_pwr_request_state(device, state);
	device->requested_state = state;
}
EXPORT_SYMBOL(kgsl_pwrctrl_request_state);

const char *kgsl_pwrstate_to_str(unsigned int state)
{
	switch (state) {
	case KGSL_STATE_NONE:
		return "NONE";
	case KGSL_STATE_INIT:
		return "INIT";
	case KGSL_STATE_ACTIVE:
		return "ACTIVE";
	case KGSL_STATE_NAP:
		return "NAP";
	case KGSL_STATE_SLEEP:
		return "SLEEP";
	case KGSL_STATE_SUSPEND:
		return "SUSPEND";
	case KGSL_STATE_HUNG:
		return "HUNG";
	case KGSL_STATE_DUMP_AND_FT:
		return "DNR";
	case KGSL_STATE_SLUMBER:
		return "SLUMBER";
	default:
		break;
	}
	return "UNKNOWN";
}
EXPORT_SYMBOL(kgsl_pwrstate_to_str);


/**
 * kgsl_active_count_get() - Increase the device active count
 * @device: Pointer to a KGSL device
 *
 * Increase the active count for the KGSL device and turn on
 * clocks if this is the first reference. Code paths that need
 * to touch the hardware or wait for the hardware to complete
 * an operation must hold an active count reference until they
 * are finished. An error code will be returned if waking the
 * device fails. The device mutex must be held while *calling
 * this function.
 */
int kgsl_active_count_get(struct kgsl_device *device)
{
	int ret = 0;
	BUG_ON(!mutex_is_locked(&device->mutex));

	if (device->active_cnt == 0) {

		if (device->state != KGSL_STATE_DUMP_AND_FT) {
			mutex_unlock(&device->mutex);
			wait_for_completion(&device->hwaccess_gate);
			wait_for_completion(&device->ft_gate);
			mutex_lock(&device->mutex);
		}

		ret = kgsl_pwrctrl_wake(device);
	}
	if (ret == 0)
		device->active_cnt++;
	return ret;
}
EXPORT_SYMBOL(kgsl_active_count_get);

/**
 * kgsl_active_count_get_light() - Increase the device active count
 * @device: Pointer to a KGSL device
 *
 * Increase the active count for the KGSL device WITHOUT
 * turning on the clocks. Currently this is only used for creating
 * kgsl_events. The device mutex must be held while calling this function.
 */
int kgsl_active_count_get_light(struct kgsl_device *device)
{
	BUG_ON(!mutex_is_locked(&device->mutex));

	if (device->state != KGSL_STATE_ACTIVE) {
		dev_WARN_ONCE(device->dev, 1, "device in unexpected state %s\n",
				kgsl_pwrstate_to_str(device->state));
		return -EINVAL;
	}

	if (device->active_cnt == 0) {
		dev_WARN_ONCE(device->dev, 1, "active count is 0!\n");
		return -EINVAL;
	}

	device->active_cnt++;
	return 0;
}
EXPORT_SYMBOL(kgsl_active_count_get_light);

/**
 * kgsl_active_count_put() - Decrease the device active count
 * @device: Pointer to a KGSL device
 *
 * Decrease the active count for the KGSL device and turn off
 * clocks if there are no remaining references. This function will
 * transition the device to NAP if there are no other pending state
 * changes. It also completes the suspend gate.  The device mutex must
 * be held while calling this function.
 */
void kgsl_active_count_put(struct kgsl_device *device)
{
	BUG_ON(!mutex_is_locked(&device->mutex));
	BUG_ON(device->active_cnt == 0);

	kgsl_pwrscale_idle(device);
	if (device->active_cnt > 1) {
		device->active_cnt--;
		return;
	}

	INIT_COMPLETION(device->suspend_gate);

	if (device->pwrctrl.nap_allowed == true &&
			(device->state == KGSL_STATE_ACTIVE &&
			device->requested_state == KGSL_STATE_NONE)) {
		kgsl_pwrctrl_request_state(device, KGSL_STATE_NAP);
		if (kgsl_pwrctrl_sleep(device) != 0)
			mod_timer(&device->idle_timer,
					jiffies
					+ device->pwrctrl.interval_timeout);
	}
	device->active_cnt--;

	if (device->active_cnt == 0)
		complete(&device->suspend_gate);
}
EXPORT_SYMBOL(kgsl_active_count_put);

/**
 * kgsl_active_count_wait() - Wait for activity to finish.
 * @device: Pointer to a KGSL device
 *
 * Block until all active_cnt users put() their reference.
 */
void kgsl_active_count_wait(struct kgsl_device *device)
{
	BUG_ON(!mutex_is_locked(&device->mutex));

	if (device->active_cnt != 0) {
		mutex_unlock(&device->mutex);
		wait_for_completion(&device->suspend_gate);
		mutex_lock(&device->mutex);
	}
}
EXPORT_SYMBOL(kgsl_active_count_wait);
