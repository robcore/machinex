/*
 * Bricked Hotplug Driver
 *
 * Copyright (c) 2013-2014, Dennis Rassmann <showp1984@gmail.com>
 * Copyright (c) 2013-2014, Pranav Vashi <neobuddy89@gmail.com>
 * Copyright (c) 2010-2014, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/init.h>
#include <linux/cpufreq.h>
#include <linux/workqueue.h>
#include <linux/completion.h>
#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/cputime.h>
#include <linux/hrtimer.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/display_state.h>
#include <linux/powersuspend.h>
#include <linux/omniplug.h>

#define DEBUG 0

#define MPDEC_TAG			"bricked_hotplug"
#define HOTPLUG_ENABLED			0
#define MSM_MPDEC_STARTDELAY		20000
#define MSM_MPDEC_DELAY			130
#define DEFAULT_MIN_CPUS_ONLINE		2
#define DEFAULT_MAX_CPUS_ONLINE		NR_CPUS
#define DEFAULT_SUSPEND_DEFER_TIME	10
#define DEFAULT_DOWN_LOCK_DUR		500

#define MSM_MPDEC_IDLE_FREQ		384000

enum {
	MSM_MPDEC_DISABLED = 0,
	MSM_MPDEC_IDLE,
	MSM_MPDEC_DOWN,
	MSM_MPDEC_UP,
};

static struct delayed_work hotplug_work;
static struct workqueue_struct *hotplug_wq;

static struct cpu_hotplug {
	unsigned int startdelay;
	unsigned int delay;
	unsigned int down_lock_dur;
	unsigned long int idle_freq;
	struct mutex bricked_hotplug_mutex;
	struct mutex bricked_cpu_mutex;
} hotplug = {
	.startdelay = MSM_MPDEC_STARTDELAY,
	.delay = MSM_MPDEC_DELAY,
	.down_lock_dur = DEFAULT_DOWN_LOCK_DUR,
	.idle_freq = MSM_MPDEC_IDLE_FREQ,
};

static unsigned int bricked_enabled = HOTPLUG_ENABLED;
bool brickenabled;
bool is_bricked_enabled(void)
{
	if (bricked_enabled > 0)
		brickenabled = true;
	else
		brickenabled = false;

	return brickenabled;
}

static unsigned int NwNs_Threshold[8] = {12, 0, 25, 7, 30, 10, 0, 18};
static unsigned int TwTs_Threshold[8] = {140, 0, 140, 190, 140, 190, 0, 190};

struct down_lock {
	unsigned int locked;
	struct delayed_work lock_rem;
};
static DEFINE_PER_CPU(struct down_lock, lock_info);

static void apply_down_lock(unsigned int cpu)
{
	struct down_lock *dl = &per_cpu(lock_info, cpu);

	dl->locked = 1;
	queue_delayed_work_on(0, hotplug_wq, &dl->lock_rem,
			      msecs_to_jiffies(hotplug.down_lock_dur));
}

static void remove_down_lock(struct work_struct *work)
{
	struct down_lock *dl = container_of(work, struct down_lock,
					    lock_rem.work);
	dl->locked = 0;
}

static int check_down_lock(unsigned int cpu)
{
	struct down_lock *dl = &per_cpu(lock_info, cpu);
	return dl->locked;
}

extern unsigned int get_rq_info(void);

static unsigned int state = MSM_MPDEC_DISABLED;

static int get_slowest_cpu(void) {
	unsigned int cpu, slow_cpu = 0, rate, slow_rate = 0;

	for_each_online_cpu(cpu) {
		rate = cpufreq_quick_get(cpu);
		if (rate > 0 && slow_rate <= rate) {
			slow_rate = rate;
			slow_cpu = cpu;
		}
	}

	return slow_cpu;
}

static unsigned int get_slowest_cpu_rate(void) {
	unsigned int cpu, rate, slow_rate = 0;

	for_each_online_cpu(cpu) {
		rate = cpufreq_quick_get(cpu);
		if (rate > 0 && slow_rate <= rate)
			slow_rate = rate;
	}

	return slow_rate;
}

static int mp_decision(void) {
	static bool first_call = true;
	int new_state = MSM_MPDEC_IDLE;
	int nr_cpu_online;
	int index;
	unsigned int rq_depth;
	static u64 total_time = 0;
	static u64 last_time;
	u64 current_time;
	u64 this_time = 0;

	if (bricked_enabled)
		return MSM_MPDEC_DISABLED;

	current_time = ktime_to_ms(ktime_get());

	if (first_call) {
		first_call = false;
	} else {
		this_time = current_time - last_time;
	}
	total_time += this_time;

	rq_depth = get_rq_info();
	nr_cpu_online = num_online_cpus();

	index = (nr_cpu_online - 1) * 2;
	if ((nr_cpu_online < DEFAULT_MAX_CPUS_ONLINE) && (rq_depth >= NwNs_Threshold[index])) {
		if ((total_time >= TwTs_Threshold[index]) &&
			(nr_cpu_online < max_cpus_online)) {
			new_state = MSM_MPDEC_UP;
			if (get_slowest_cpu_rate() <=  hotplug.idle_freq)
				new_state = MSM_MPDEC_IDLE;
		}
	} else if ((nr_cpu_online > 1) && (rq_depth <= NwNs_Threshold[index+1])) {
		if ((total_time >= TwTs_Threshold[index+1]) &&
			(nr_cpu_online > min_cpus_online)) {
			new_state = MSM_MPDEC_DOWN;
			if (get_slowest_cpu_rate() > hotplug.idle_freq)
				new_state = MSM_MPDEC_IDLE;
		}
	} else {
		new_state = MSM_MPDEC_IDLE;
		total_time = 0;
	}

	if (new_state != MSM_MPDEC_IDLE) {
		total_time = 0;
	}

	last_time = ktime_to_ms(ktime_get());
#if DEBUG
	pr_info(MPDEC_TAG"[DEBUG] rq: %u, new_state: %i | Mask=[%d%d%d%d]\n",
			rq_depth, new_state, cpu_online(0), cpu_online(1), cpu_online(2), cpu_online(3));
#endif
	return new_state;
}

static void bricked_hotplug_work(struct work_struct *work) {
	unsigned int cpu;
	
	if (!is_display_on())
		return;

	if (!mutex_trylock(&hotplug.bricked_cpu_mutex))
		goto out;

	state = mp_decision();
	switch (state) {
	case MSM_MPDEC_DISABLED:
	case MSM_MPDEC_IDLE:
		break;
	case MSM_MPDEC_DOWN:
		cpu = get_slowest_cpu();
		if (cpu > 0) {
			if (cpu_online(cpu) && !check_down_lock(cpu))
				cpu_down(cpu);
		}
		break;
	case MSM_MPDEC_UP:
		cpu = cpumask_next_zero(0, cpu_online_mask);
		if (cpu < DEFAULT_MAX_CPUS_ONLINE) {
			if (!cpu_online(cpu) &&
			is_cpu_allowed(cpu) && !thermal_core_controlled(cpu)) {
				cpu_up(cpu);
				apply_down_lock(cpu);
			}
		}
		break;
	default:
		pr_err(MPDEC_TAG": %s: invalid mpdec hotplug state %d\n",
			__func__, state);
	}
	mutex_unlock(&hotplug.bricked_cpu_mutex);

out:
	if (bricked_enabled)
		queue_delayed_work(hotplug_wq, &hotplug_work,
					msecs_to_jiffies(hotplug.delay));
	return;
}

static void bricked_suspend(struct power_suspend * h)
{
}
static void bricked_resume(struct power_suspend * h)
{
	queue_delayed_work(hotplug_wq, &hotplug_work,
				msecs_to_jiffies(hotplug.delay));
}

static struct power_suspend bricked_suspend_data =
{
	.suspend = bricked_suspend,
	.resume = bricked_resume,
};


int bricked_hotplug_start(void)
{
	int cpu, ret = 0;
	struct down_lock *dl;

	bricked_enabled = 1;
	state = MSM_MPDEC_IDLE;

	hotplug_wq = alloc_workqueue("bricked_hotplug", WQ_HIGHPRI | WQ_FREEZABLE, 0);
	if (!hotplug_wq) {
		ret = -ENOMEM;
		goto err_out;
	}

	mutex_init(&hotplug.bricked_cpu_mutex);
	mutex_init(&hotplug.bricked_hotplug_mutex);

	INIT_DELAYED_WORK(&hotplug_work, bricked_hotplug_work);

	for_each_possible_cpu(cpu) {
		dl = &per_cpu(lock_info, cpu);
		INIT_DELAYED_WORK(&dl->lock_rem, remove_down_lock);
	}

	register_power_suspend(&bricked_suspend_data);

	if (bricked_enabled)
		queue_delayed_work(hotplug_wq, &hotplug_work,
					msecs_to_jiffies(hotplug.startdelay));

	return ret;
err_dev:
	destroy_workqueue(hotplug_wq);
err_out:
	bricked_enabled = 0;
	return ret;
}

void bricked_hotplug_stop(void)
{
	int cpu;
	struct down_lock *dl;

	bricked_enabled = 0;
	state = MSM_MPDEC_DISABLED;
	unregister_power_suspend(&bricked_suspend_data);
	for_each_possible_cpu(cpu) {
		dl = &per_cpu(lock_info, cpu);
		cancel_delayed_work_sync(&dl->lock_rem);
	}

	cancel_delayed_work_sync(&hotplug_work);
	mutex_destroy(&hotplug.bricked_hotplug_mutex);
	mutex_destroy(&hotplug.bricked_cpu_mutex);
	destroy_workqueue(hotplug_wq);
}

/**************************** SYSFS START ****************************/

#define show_one(file_name, object)					\
static ssize_t show_##file_name						\
(struct device *dev, struct device_attribute *bricked_hotplug_attrs,	\
 char *buf)								\
{									\
	return sprintf(buf, "%u\n", hotplug.object);			\
}

show_one(startdelay, startdelay);
show_one(delay, delay);
show_one(down_lock_duration, down_lock_dur);

#define define_one_twts(file_name, arraypos)				\
static ssize_t show_##file_name						\
(struct device *dev, struct device_attribute *bricked_hotplug_attrs,	\
 char *buf)								\
{									\
	return sprintf(buf, "%u\n", TwTs_Threshold[arraypos]);		\
}									\
static ssize_t store_##file_name					\
(struct device *dev, struct device_attribute *bricked_hotplug_attrs,	\
 const char *buf, size_t count)						\
{									\
	unsigned int input;						\
	int ret;							\
	ret = sscanf(buf, "%u", &input);				\
	if (ret != 1)							\
		return -EINVAL;						\
	TwTs_Threshold[arraypos] = input;				\
	return count;							\
}									\
static DEVICE_ATTR(file_name, 0644, show_##file_name, store_##file_name);
define_one_twts(twts_threshold_0, 0);
define_one_twts(twts_threshold_1, 1);
define_one_twts(twts_threshold_2, 2);
define_one_twts(twts_threshold_3, 3);
define_one_twts(twts_threshold_4, 4);
define_one_twts(twts_threshold_5, 5);
define_one_twts(twts_threshold_6, 6);
define_one_twts(twts_threshold_7, 7);

#define define_one_nwns(file_name, arraypos)				\
static ssize_t show_##file_name						\
(struct device *dev, struct device_attribute *bricked_hotplug_attrs,	\
 char *buf)								\
{									\
	return sprintf(buf, "%u\n", NwNs_Threshold[arraypos]);		\
}									\
static ssize_t store_##file_name					\
(struct device *dev, struct device_attribute *bricked_hotplug_attrs,	\
 const char *buf, size_t count)						\
{									\
	unsigned int input;						\
	int ret;							\
	ret = sscanf(buf, "%u", &input);				\
	if (ret != 1)							\
		return -EINVAL;						\
	NwNs_Threshold[arraypos] = input;				\
	return count;							\
}									\
static DEVICE_ATTR(file_name, 0644, show_##file_name, store_##file_name);
define_one_nwns(nwns_threshold_0, 0);
define_one_nwns(nwns_threshold_1, 1);
define_one_nwns(nwns_threshold_2, 2);
define_one_nwns(nwns_threshold_3, 3);
define_one_nwns(nwns_threshold_4, 4);
define_one_nwns(nwns_threshold_5, 5);
define_one_nwns(nwns_threshold_6, 6);
define_one_nwns(nwns_threshold_7, 7);

static ssize_t show_idle_freq (struct device *dev,
				struct device_attribute *bricked_hotplug_attrs,
				char *buf)
{
	return sprintf(buf, "%lu\n", hotplug.idle_freq);
}

static ssize_t store_startdelay(struct device *dev,
				struct device_attribute *bricked_hotplug_attrs,
				const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	hotplug.startdelay = input;

	return count;
}

static ssize_t store_delay(struct device *dev,
				struct device_attribute *bricked_hotplug_attrs,
				const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	hotplug.delay = input;

	return count;
}

static ssize_t store_down_lock_duration(struct device *dev,
				struct device_attribute *bricked_hotplug_attrs,
				const char *buf, size_t count)
{
	int ret;
	unsigned int val;

	ret = sscanf(buf, "%u", &val);
	if (ret != 1)
		return -EINVAL;

	hotplug.down_lock_dur = val;

	return count;
}

static ssize_t store_idle_freq(struct device *dev,
				struct device_attribute *bricked_hotplug_attrs,
				const char *buf, size_t count)
{
	long unsigned int input;
	int ret;
	ret = sscanf(buf, "%lu", &input);
	if (ret != 1)
		return -EINVAL;

	hotplug.idle_freq = input;

	return count;
}

static DEVICE_ATTR(startdelay, 0644, show_startdelay, store_startdelay);
static DEVICE_ATTR(delay, 0644, show_delay, store_delay);
static DEVICE_ATTR(down_lock_duration, 0644, show_down_lock_duration, store_down_lock_duration);
static DEVICE_ATTR(idle_freq, 0644, show_idle_freq, store_idle_freq);

static struct attribute *bricked_hotplug_attrs[] = {
	&dev_attr_startdelay.attr,
	&dev_attr_delay.attr,
	&dev_attr_down_lock_duration.attr,
	&dev_attr_idle_freq.attr,
	&dev_attr_twts_threshold_0.attr,
	&dev_attr_twts_threshold_1.attr,
	&dev_attr_twts_threshold_2.attr,
	&dev_attr_twts_threshold_3.attr,
	&dev_attr_twts_threshold_4.attr,
	&dev_attr_twts_threshold_5.attr,
	&dev_attr_twts_threshold_6.attr,
	&dev_attr_twts_threshold_7.attr,
	&dev_attr_nwns_threshold_0.attr,
	&dev_attr_nwns_threshold_1.attr,
	&dev_attr_nwns_threshold_2.attr,
	&dev_attr_nwns_threshold_3.attr,
	&dev_attr_nwns_threshold_4.attr,
	&dev_attr_nwns_threshold_5.attr,
	&dev_attr_nwns_threshold_6.attr,
	&dev_attr_nwns_threshold_7.attr,
	NULL,
};

static struct attribute_group attr_group = {
	.attrs = bricked_hotplug_attrs,
	.name = "conf",
};

/**************************** SYSFS END ****************************/

static int bricked_hotplug_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct kobject *bricked_kobj;

	bricked_kobj =
		kobject_create_and_add("msm_mpdecision", kernel_kobj);
	if (!bricked_kobj) {
		pr_err("%s kobject create failed!\n",
			__func__);
		return -ENOMEM;
        }

	ret = sysfs_create_group(bricked_kobj,
			&attr_group);

        if (ret) {
		pr_err("%s bricked_kobj create failed!\n",
			__func__);
		goto err_dev;
	}

	if (bricked_enabled) {
		ret = bricked_hotplug_start();
		if (ret != 0)
			goto err_dev;
	}

	return ret;
err_dev:
	if (bricked_kobj != NULL)
		kobject_put(bricked_kobj);
	return ret;
}

static struct platform_device bricked_hotplug_device = {
	.name = MPDEC_TAG,
	.id = -1,
};

static int bricked_hotplug_remove(struct platform_device *pdev)
{
	if (bricked_enabled)
		bricked_hotplug_stop();

	return 0;
}

static struct platform_driver bricked_hotplug_driver = {
	.probe = bricked_hotplug_probe,
	.remove = bricked_hotplug_remove,
	.driver = {
		.name = MPDEC_TAG,
		.owner = THIS_MODULE,
	},
};

static int __init msm_mpdec_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&bricked_hotplug_driver);
	if (ret) {
		pr_err("%s: Driver register failed: %d\n", MPDEC_TAG, ret);
		return ret;
	}

	ret = platform_device_register(&bricked_hotplug_device);
	if (ret) {
		pr_err("%s: Device register failed: %d\n", MPDEC_TAG, ret);
		return ret;
	}

	pr_info(MPDEC_TAG": %s init complete.", __func__);

	return ret;
}

void msm_mpdec_exit(void)
{
	platform_device_unregister(&bricked_hotplug_device);
	platform_driver_unregister(&bricked_hotplug_driver);
}

late_initcall(msm_mpdec_init);
module_exit(msm_mpdec_exit);

MODULE_AUTHOR("Dennis Rassmann <showp1984@gmail.com>");
MODULE_DESCRIPTION("Bricked Hotplug Driver");
MODULE_LICENSE("GPLv2");
