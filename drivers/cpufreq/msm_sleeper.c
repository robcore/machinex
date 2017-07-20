/*
 * msm-sleeper.c
 *
 * Copyright (C) 2015 Aaron Segaert <asegaert@gmail.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/workqueue.h>
#include <linux/cpu.h>
#include <linux/cpufreq.h>
#include <linux/platform_device.h>
#include <linux/module.h>

#define MSM_SLEEPER "msm_sleeper"
#define MSM_SLEEPER_MAJOR_VERSION	4
#define MSM_SLEEPER_MINOR_VERSION	2
#define MSM_SLEEPER_ENABLED		0
#define MSM_SLEEPER_DEBUG		0
#define DELAY				1000
#define DEF_UP_THRESHOLD		85
#define DEF_MAX_CPUS_ONLINE			4
#define DEF_DOWN_COUNT_MAX		10 /* 1 sec */
#define DEF_UP_COUNT_MAX		5 /* 0.5 sec */
#define DEF_MAX_CPUS_ONLINE_SUSP		1
#define DEF_PLUG_ALL			0

struct msm_sleeper_data {
	unsigned int enabled;
	unsigned int delay;
	unsigned int up_threshold;
	unsigned int max_cpus_online;
	unsigned int down_count;
	unsigned int up_count;
	unsigned int down_count_max;
	unsigned int up_count_max;
	bool plug_all;
	struct notifier_block notif;
} sleeper_data = {
	.enabled = MSM_SLEEPER_ENABLED,
	.delay = DELAY,
	.up_threshold = DEF_UP_THRESHOLD,
	.max_cpus_online = DEF_MAX_CPUS_ONLINE,
	.down_count_max = DEF_DOWN_COUNT_MAX,
	.up_count_max = DEF_UP_COUNT_MAX,
	.plug_all = DEF_PLUG_ALL
};

static struct workqueue_struct *sleeper_wq;
static struct delayed_work sleeper_work;

static inline void plug_cpu(void)
{
	unsigned int cpu;

	if (num_online_cpus() == sleeper_data.max_cpus_online)
		goto reset;

	cpu = cpumask_next_zero(0, cpu_online_mask);
	if (cpu < nr_cpu_ids)
		cpu_up(cpu);

reset:
	sleeper_data.down_count = 0;
	sleeper_data.up_count = 0;
}

static inline void unplug_cpu(void)
{
	unsigned int cpu, low_cpu = 0, low_freq = ~0;

	if (num_online_cpus() == 2)
		goto reset;

	get_online_cpus();
	for_each_online_cpu(cpu) {
		if (cpu > 1) {
			unsigned int curfreq = cpufreq_quick_get(cpu);
			if (low_freq > curfreq) {
				low_freq = curfreq;
				low_cpu = cpu;
			}
		}
	}
	put_online_cpus();

	cpu_down(low_cpu);

reset:
	sleeper_data.down_count = 0;
	sleeper_data.up_count = 0;
}

static void reschedule_timer (void)
{
	queue_delayed_work_on(0, sleeper_wq, &sleeper_work, msecs_to_jiffies(sleeper_data.delay));
}

static void hotplug_func(struct work_struct *work)
{
	unsigned int cpu, loadavg = 0;

	if (sleeper_data.max_cpus_online == 2)
		goto reschedule;

	if (sleeper_data.plug_all) {
		if (num_online_cpus() < nr_cpu_ids)
			plug_cpu();
		goto reschedule;
	}

	for_each_online_cpu(cpu)
		loadavg += cpufreq_quick_get(cpu);

	loadavg /= num_online_cpus();

	if (loadavg >= sleeper_data.up_threshold) {
		++sleeper_data.up_count;
		if (sleeper_data.up_count > sleeper_data.up_count_max)
			plug_cpu();
	} else if (loadavg > 95 && sleeper_data.up_count >= 2) {
		++sleeper_data.up_count;
		plug_cpu();
	} else {
		++sleeper_data.down_count;
		if (sleeper_data.down_count > sleeper_data.down_count_max)
			unplug_cpu();
	}

#if MSM_SLEEPER_DEBUG
	pr_info("msm-sleeper: loadavg: %u, online: %u, up_count: %u, down_count: %u\n",
		loadavg, num_online_cpus(), sleeper_data.up_count, sleeper_data.down_count);
#endif

reschedule:
	reschedule_timer();
}

static ssize_t show_enable_hotplug(struct device *dev,
				   struct device_attribute *msm_sleeper_attrs, char *buf)
{
	return sprintf(buf, "%u\n", sleeper_data.enabled);
}

static ssize_t store_enable_hotplug(struct device *dev,
				    struct device_attribute *msm_sleeper_attrs,
				    const char *buf, size_t count)
{
	int ret, cpu;
	unsigned long val;

	ret = kstrtoul(buf, 0, &val);
	if (ret < 0)
		return ret;

	sleeper_data.enabled = val;

	if (sleeper_data.enabled) {
		reschedule_timer();
	} else {
		flush_workqueue(sleeper_wq);
		cancel_delayed_work_sync(&sleeper_work);

		for_each_possible_cpu(cpu)
			if (cpu_is_offline(cpu))
				cpu_up(cpu);
	}

	return count;
}

static ssize_t show_plug_all(struct device *dev,
				    struct device_attribute *msm_sleeper_attrs,
				    char *buf)
{
	return sprintf(buf, "%u\n", sleeper_data.plug_all);
}

static ssize_t store_plug_all(struct device *dev,
				     struct device_attribute *msm_sleeper_attrs,
				     const char *buf, size_t count)
{
	int ret;
	unsigned long val;

	ret = kstrtoul(buf, 0, &val);
	if (ret < 0 || val > 1)
		return -EINVAL;

	sleeper_data.plug_all = val;

	return count;
}

static ssize_t show_max_cpus_online(struct device *dev,
				    struct device_attribute *msm_sleeper_attrs,
				    char *buf)
{
	return sprintf(buf, "%u\n", sleeper_data.max_cpus_online);
}

static ssize_t store_max_cpus_online(struct device *dev,
				     struct device_attribute *msm_sleeper_attrs,
				     const char *buf, size_t count)
{
	int ret, cpu;
	unsigned long val;

	ret = kstrtoul(buf, 0, &val);
	if (ret < 0 || val < 2 || val > NR_CPUS)
		return -EINVAL;

	for_each_possible_cpu(cpu) {
		if (cpu >= val)
			if (cpu_online(cpu))
				cpu_down(cpu);
	}

	sleeper_data.max_cpus_online = val;

	return count;
}

static ssize_t show_up_threshold(struct device *dev,
				    struct device_attribute *msm_sleeper_attrs,
				    char *buf)
{
	return sprintf(buf, "%u\n", sleeper_data.up_threshold);
}

static ssize_t store_up_threshold(struct device *dev,
				     struct device_attribute *msm_sleeper_attrs,
				     const char *buf, size_t count)
{
	int ret;
	unsigned long val;

	ret = kstrtoul(buf, 0, &val);
	if (ret < 0)
		return -EINVAL;

	sleeper_data.up_threshold = val > 100 ? 100 : val;

	return count;
}

static ssize_t show_up_count_max(struct device *dev,
				    struct device_attribute *msm_sleeper_attrs,
				    char *buf)
{
	return sprintf(buf, "%u\n", sleeper_data.up_count_max);
}

static ssize_t store_up_count_max(struct device *dev,
				     struct device_attribute *msm_sleeper_attrs,
				     const char *buf, size_t count)
{
	int ret;
	unsigned long val;

	ret = kstrtoul(buf, 0, &val);
	if (ret < 0)
		return -EINVAL;

	sleeper_data.up_count_max = val > 40 ? 40 : val;

	return count;
}

static ssize_t show_down_count_max(struct device *dev,
				    struct device_attribute *msm_sleeper_attrs,
				    char *buf)
{
	return sprintf(buf, "%u\n", sleeper_data.down_count_max);
}


static ssize_t store_down_count_max(struct device *dev,
				     struct device_attribute *msm_sleeper_attrs,
				     const char *buf, size_t count)
{
	int ret;
	unsigned long val;

	ret = kstrtoul(buf, 0, &val);
	if (ret < 0)
		return -EINVAL;

	sleeper_data.down_count_max = val > 40 ? 40 : val;

	return count;
}

static DEVICE_ATTR(enabled, 644, show_enable_hotplug, store_enable_hotplug);
static DEVICE_ATTR(up_threshold, 644, show_up_threshold, store_up_threshold);
static DEVICE_ATTR(plug_all, 644, show_plug_all, store_plug_all);
static DEVICE_ATTR(max_cpus_online, 644, show_max_cpus_online, store_max_cpus_online);
static DEVICE_ATTR(up_count_max, 644, show_up_count_max, store_up_count_max);
static DEVICE_ATTR(down_count_max, 644, show_down_count_max, store_down_count_max);

static struct attribute *msm_sleeper_attrs[] = {
	&dev_attr_up_threshold.attr,
	&dev_attr_plug_all.attr,
	&dev_attr_max_cpus_online.attr,
	&dev_attr_up_count_max.attr,
	&dev_attr_down_count_max.attr,
	&dev_attr_enabled.attr,
	NULL
};

static struct attribute_group attr_group = {
	.attrs = msm_sleeper_attrs,
};

static struct platform_device msm_sleeper_device = {
	.name = MSM_SLEEPER,
	.id = -1,
};

static int msm_sleeper_probe(struct platform_device *pdev)
{
	int ret = 0;

	pr_info("msm-sleeper version %d.%d\n",
		MSM_SLEEPER_MAJOR_VERSION,
		MSM_SLEEPER_MINOR_VERSION);

	sleeper_wq = alloc_workqueue("msm_sleeper_wq",
				WQ_HIGHPRI, 1);
	if (!sleeper_wq) {
		ret = -ENOMEM;
		goto err_out;
	}

	ret = sysfs_create_group(&pdev->dev.kobj, &attr_group);
	if (ret) {
		ret = -EINVAL;
		goto err_dev;
	}

	INIT_DELAYED_WORK(&sleeper_work, hotplug_func);

	if (sleeper_data.enabled)
		queue_delayed_work_on(0, sleeper_wq, &sleeper_work, msecs_to_jiffies(60000));

	return ret;

err_dev:
	destroy_workqueue(sleeper_wq);
err_out:
	sleeper_data.enabled = 0;
	return ret;
}

static int msm_sleeper_remove(struct platform_device *pdev)
{
	destroy_workqueue(sleeper_wq);

	return 0;
}

static struct platform_driver msm_sleeper_driver = {
	.probe = msm_sleeper_probe,
	.remove = msm_sleeper_remove,
	.driver = {
		.name = MSM_SLEEPER,
	},
};

static int __init msm_sleeper_init(void)
{
	int ret;

	ret = platform_driver_register(&msm_sleeper_driver);
	if (ret) {
		pr_err("%s: Driver register failed: %d\n", MSM_SLEEPER, ret);
		return ret;
	}

	ret = platform_device_register(&msm_sleeper_device);
	if (ret) {
		pr_err("%s: Device register failed: %d\n", MSM_SLEEPER, ret);
		return ret;
	}

	return ret;
}

static void __exit msm_sleeper_exit(void)
{
	platform_device_unregister(&msm_sleeper_device);
	platform_driver_unregister(&msm_sleeper_driver);
}

late_initcall(msm_sleeper_init);
module_exit(msm_sleeper_exit);
