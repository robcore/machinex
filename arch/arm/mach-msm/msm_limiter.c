/*
 * MSM CPU Frequency Limiter Driver
 *
 * Copyright (c) 2013-2014, Dorimanx <yuri@bynet.co.il>
 * Copyright (c) 2013-2014, Pranav Vashi <neobuddy89@gmail.com>
 * Copyright (c) 2010-2014, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/module.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/workqueue.h>
#include <linux/cpu.h>
#include <linux/cpufreq.h>
#ifdef CONFIG_LCD_NOTIFY
#include <linux/lcd_notify.h>
#elif defined(CONFIG_POWERSUSPEND)
#include <linux/powersuspend.h>
#elif defined(CONFIG_HAS_EARLYSUSPEND)
#include <linux/earlysuspend.h>
#endif

#define MSM_CPUFREQ_LIMIT_MAJOR		3
#define MSM_CPUFREQ_LIMIT_MINOR		0

#define MSM_LIMIT			"msm_limiter"
#define LIMITER_ENABLED			0
#define DEFAULT_SUSPEND_DEFER_TIME	10
#define DEFAULT_SUSPEND_FREQUENCY	810000
#define DEFAULT_RESUME_FREQUENCY	1890000
#define DEFAULT_MIN_FREQUENCY		384000

static unsigned int debug = 0;
module_param_named(debug_mask, debug, uint, 0644);

#define dprintk(msg...)		\
do { 				\
	if (debug)		\
		pr_info(msg);	\
} while (0)

static struct cpu_limit {
	unsigned int limiter_enabled;
	uint32_t suspend_max_freq;
	uint32_t resume_max_freq[4];
	uint32_t suspend_min_freq[4];
	unsigned int suspended;
	unsigned int suspend_defer_time;
	struct delayed_work suspend_work;
	struct work_struct resume_work;
	struct mutex resume_suspend_mutex;
	struct mutex msm_limiter_mutex[4];
#ifdef CONFIG_LCD_NOTIFY
	struct notifier_block notif;
#endif
} limit = {
	.limiter_enabled = LIMITER_ENABLED,
	.suspend_max_freq = DEFAULT_SUSPEND_FREQUENCY,
	.resume_max_freq[0] = DEFAULT_RESUME_FREQUENCY,
	.resume_max_freq[1] = DEFAULT_RESUME_FREQUENCY,
	.resume_max_freq[2] = DEFAULT_RESUME_FREQUENCY,
	.resume_max_freq[3] = DEFAULT_RESUME_FREQUENCY,
	.suspend_min_freq[0] = DEFAULT_MIN_FREQUENCY,
	.suspend_min_freq[1] = DEFAULT_MIN_FREQUENCY,
	.suspend_min_freq[2] = DEFAULT_MIN_FREQUENCY,
	.suspend_min_freq[3] = DEFAULT_MIN_FREQUENCY,
	.suspend_defer_time = DEFAULT_SUSPEND_DEFER_TIME,
};

static struct workqueue_struct *limiter_wq;

static void update_cpu_max_freq(unsigned int cpu)
{
	struct cpufreq_policy *policy;
	uint32_t max_freq;

	mutex_lock(&limit.msm_limiter_mutex[cpu]);
	if (limit.suspended)
		max_freq = limit.suspend_max_freq;
	else
		max_freq = limit.resume_max_freq[cpu];

	policy = cpufreq_cpu_get(cpu);
	if (policy) {
		if (max_freq > policy->min &&
		    max_freq != policy->max) {
			policy->user_policy.max = max_freq;
			policy->max = max_freq;
			dprintk("%s: Set MAX %uMHz for CPU%u\n", MSM_LIMIT, max_freq / 1000, cpu);
		}
		if (limit.suspended && limit.suspend_min_freq[cpu] <= max_freq) {
			policy->user_policy.min = limit.suspend_min_freq[cpu];
			policy->min = limit.suspend_min_freq[cpu];
			dprintk("%s: Set MIN %uMHz for CPU%u\n", MSM_LIMIT, limit.suspend_min_freq[cpu] / 1000, cpu);
		}
		cpufreq_cpu_put(policy);
	}
	mutex_unlock(&limit.msm_limiter_mutex[cpu]);
}

static void update_cpu_min_freq(unsigned int cpu)
{
	struct cpufreq_policy *policy;
	uint32_t max_freq;

	mutex_lock(&limit.msm_limiter_mutex[cpu]);
	max_freq = min(limit.suspend_max_freq, limit.resume_max_freq[cpu]);
	policy = cpufreq_cpu_get(cpu);
	if (policy) {
		if (limit.suspend_min_freq[cpu] <= max_freq) {
			policy->user_policy.min = limit.suspend_min_freq[cpu];
			policy->min = limit.suspend_min_freq[cpu];
			dprintk("%s: Set %uMHz for CPU%u\n", MSM_LIMIT, limit.suspend_min_freq[cpu] / 1000, cpu);
		}
		cpufreq_cpu_put(policy);
	}
	mutex_unlock(&limit.msm_limiter_mutex[cpu]);
}

static void msm_limit_suspend(struct work_struct *work)
{
	int cpu = 0;

	/* Do not suspend if suspend freq or resume freq not available */
	if (!limit.suspend_max_freq || !limit.resume_max_freq[0])
		return;

	mutex_lock(&limit.resume_suspend_mutex);
	limit.suspended = 1;
	mutex_unlock(&limit.resume_suspend_mutex);

	for_each_possible_cpu(cpu)
		update_cpu_max_freq(cpu);
}

static void msm_limit_resume(struct work_struct *work)
{
	int cpu = 0;

	/* Do not resume if resume freq not available */
	if (!limit.resume_max_freq[0] || !limit.suspended)
		return;

	mutex_lock(&limit.resume_suspend_mutex);
	limit.suspended = 0;
	mutex_unlock(&limit.resume_suspend_mutex);

	/* Restore max allowed freq */
	for_each_possible_cpu(cpu)
		update_cpu_max_freq(cpu);
}

#ifdef CONFIG_LCD_NOTIFY
static void __msm_limit_suspend(void)
#elif defined(CONFIG_POWERSUSPEND)
static void __msm_limit_suspend(struct power_suspend *handler)
#elif defined(CONFIG_HAS_EARLYSUSPEND)
static void __msm_limit_suspend(struct early_suspend *handler)
#endif
{
	if (!limit.limiter_enabled)
		return;

	INIT_DELAYED_WORK(&limit.suspend_work, msm_limit_suspend);
	queue_delayed_work_on(0, limiter_wq, &limit.suspend_work,
			msecs_to_jiffies(limit.suspend_defer_time * 1000));
}

#ifdef CONFIG_LCD_NOTIFY
static void __msm_limit_resume(void)
#elif defined(CONFIG_POWERSUSPEND)
static void __msm_limit_resume(struct power_suspend *handler)
#elif defined(CONFIG_HAS_EARLYSUSPEND)
static void __msm_limit_resume(struct early_suspend *handler)
#endif
{
	if (!limit.limiter_enabled)
		return;

	flush_workqueue(limiter_wq);
	cancel_delayed_work_sync(&limit.suspend_work);
	queue_work_on(0, limiter_wq, &limit.resume_work);
}

#ifdef CONFIG_LCD_NOTIFY
static int lcd_notifier_callback(struct notifier_block *nb,
                                 unsigned long event, void *data)
{
	switch (event) {
	case LCD_EVENT_ON_END:
	case LCD_EVENT_OFF_START:
		break;
	case LCD_EVENT_ON_START:
		__msm_limit_resume();
		break;
	case LCD_EVENT_OFF_END:
		__msm_limit_suspend();
		break;
	default:
		break;
	}

	return NOTIFY_OK;
}
#elif defined(CONFIG_POWERSUSPEND) || defined(CONFIG_HAS_EARLYSUSPEND)
#ifdef CONFIG_POWERSUSPEND
static struct power_suspend msm_limit_power_suspend_driver = {
#else
static struct early_suspend msm_limit_early_suspend_driver = {
	.level = EARLY_SUSPEND_LEVEL_DISABLE_FB + 10,
#endif
	.suspend = __msm_limit_suspend,
	.resume = __msm_limit_resume,
};
#endif

static int msm_cpufreq_limit_start(void)
{
	unsigned int cpu = 0;
	int ret = 0;

	limiter_wq =
	    alloc_workqueue("msm_limiter_wq", WQ_HIGHPRI | WQ_FREEZABLE, 0);
	if (!limiter_wq) {
		pr_err("%s: Failed to allocate limiter workqueue\n",
		       MSM_LIMIT);
		ret = -ENOMEM;
		goto err_out;
	}

#ifdef CONFIG_LCD_NOTIFY
	limit.notif.notifier_call = lcd_notifier_callback;
	ret = lcd_register_client(&limit.notif);
	if (ret != 0) {
		pr_err("%s: Failed to register LCD notifier callback\n",
			MSM_LIMIT);
		goto err_dev;
	}
#elif defined(CONFIG_POWERSUSPEND)
	register_power_suspend(&msm_limit_power_suspend_driver);
#elif defined(CONFIG_HAS_EARLYSUSPEND)
	register_early_suspend(&msm_limit_early_suspend_driver);
#endif

	for_each_possible_cpu(cpu)
		mutex_init(&limit.msm_limiter_mutex[cpu]);
	mutex_init(&limit.resume_suspend_mutex);
	INIT_DELAYED_WORK(&limit.suspend_work, msm_limit_suspend);
	INIT_WORK(&limit.resume_work, msm_limit_resume);

	limit.suspended = 1;
	queue_work_on(0, limiter_wq, &limit.resume_work);

	return ret;
#ifdef CONFIG_LCD_NOTIFY
err_dev:
	destroy_workqueue(limiter_wq);
#endif
err_out:
	limit.limiter_enabled = 0;
	return ret;
}

static void msm_cpufreq_limit_stop(void)
{
	unsigned int cpu = 0;
	limit.suspended = 1;

	flush_workqueue(limiter_wq);

	cancel_work_sync(&limit.resume_work);
	cancel_delayed_work_sync(&limit.suspend_work);
	mutex_destroy(&limit.resume_suspend_mutex);
	for_each_possible_cpu(cpu)
		mutex_destroy(&limit.msm_limiter_mutex[cpu]);

#ifdef CONFIG_LCD_NOTIFY
	lcd_unregister_client(&limit.notif);
	limit.notif.notifier_call = NULL;
#elif defined(CONFIG_POWERSUSPEND)
	unregister_power_suspend(&msm_limit_power_suspend_driver);
#elif defined(CONFIG_HAS_EARLYSUSPEND)
	unregister_early_suspend(&msm_limit_early_suspend_driver);
#endif
	destroy_workqueue(limiter_wq);
}

static ssize_t limiter_enabled_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", limit.limiter_enabled);
}

static ssize_t limiter_enabled_store(struct kobject *kobj,
				      struct kobj_attribute *attr,
				      const char *buf, size_t count)
{
	int ret;
	unsigned int val;

	ret = sscanf(buf, "%u\n", &val);
	if (ret != 1 || val < 0 || val > 1)
		return -EINVAL;

	if (val == limit.limiter_enabled)
		return count;

	limit.limiter_enabled = val;

	if (limit.limiter_enabled)
		msm_cpufreq_limit_start();
	else
		msm_cpufreq_limit_stop();

	return count;
}

static ssize_t suspend_defer_time_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", limit.suspend_defer_time);
}

static ssize_t suspend_defer_time_store(struct kobject *kobj,
				      struct kobj_attribute *attr,
				      const char *buf, size_t count)
{
	int ret;
	unsigned int val;

	ret = sscanf(buf, "%u\n", &val);
	if (ret != 1)
		return -EINVAL;

	limit.suspend_defer_time = val;

	return count;
}

static ssize_t suspend_max_freq_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", limit.suspend_max_freq);
}

static ssize_t suspend_max_freq_store(struct kobject *kobj,
				      struct kobj_attribute *attr,
				      const char *buf, size_t count)
{
	int ret;
	unsigned int val;

	ret = sscanf(buf, "%u\n", &val);
	if (ret != 1)
		return -EINVAL;

	if (val == 0)
		goto out;

	if (val == limit.suspend_max_freq)
		return count;

out:
	limit.suspend_max_freq = val;

	return count;
}


#define multi_cpu(cpu)					\
static ssize_t store_resume_max_freq_##cpu		\
(struct kobject *kobj, 					\
 struct kobj_attribute *attr, 				\
 const char *buf, size_t count)				\
{							\
	int ret;					\
	unsigned int val;				\
	ret = sscanf(buf, "%u\n", &val);		\
	if (ret != 1)					\
		return -EINVAL;				\
	if (val == 0)					\
		goto out;				\
	if (val < limit.suspend_min_freq[cpu])		\
		val = limit.suspend_min_freq[cpu];	\
	if (val == limit.resume_max_freq[cpu])		\
		return count;				\
out:							\
	limit.resume_max_freq[cpu] = val;		\
	if (limit.limiter_enabled)			\
		update_cpu_max_freq(cpu);		\
	return count;					\
}							\
static ssize_t show_resume_max_freq_##cpu		\
(struct kobject *kobj,					\
 struct kobj_attribute *attr, char *buf)		\
{							\
	return sprintf(buf, "%u\n",			\
			limit.resume_max_freq[cpu]);	\
}							\
static ssize_t store_suspend_min_freq_##cpu(		\
 struct kobject *kobj,					\
 struct kobj_attribute *attr,				\
 const char *buf, size_t count)				\
{							\
	int ret;					\
	unsigned int val;				\
	ret = sscanf(buf, "%u\n", &val);		\
	if (ret != 1)					\
		return -EINVAL;				\
	if (val == 0)					\
		goto out;				\
	if (val > limit.resume_max_freq[cpu])		\
		val = limit.resume_max_freq[cpu];	\
	if (val == limit.suspend_min_freq[cpu])		\
		return count;				\
out:							\
	limit.suspend_min_freq[cpu] = val;		\
	if (limit.limiter_enabled)			\
		update_cpu_min_freq(cpu);		\
	return count;					\
}							\
static ssize_t show_suspend_min_freq_##cpu(		\
 struct kobject *kobj,					\
 struct kobj_attribute *attr, char *buf)		\
{							\
	return sprintf(buf, "%u\n",			\
		limit.suspend_min_freq[cpu]);		\
}							\
static struct kobj_attribute resume_max_freq_##cpu =	\
	__ATTR(resume_max_freq_##cpu, 0666,		\
		show_resume_max_freq_##cpu,		\
		store_resume_max_freq_##cpu);		\
static struct kobj_attribute suspend_min_freq_##cpu =	\
	__ATTR(suspend_min_freq_##cpu, 0666,		\
		show_suspend_min_freq_##cpu,		\
		store_suspend_min_freq_##cpu);		\

multi_cpu(0);
multi_cpu(1);
multi_cpu(2);
multi_cpu(3);

static ssize_t msm_cpufreq_limit_version_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "version: %u.%u\n",
			MSM_CPUFREQ_LIMIT_MAJOR, MSM_CPUFREQ_LIMIT_MINOR);
}

static struct kobj_attribute msm_cpufreq_limit_version_attribute =
	__ATTR(msm_cpufreq_limit_version, 0444,
		msm_cpufreq_limit_version_show,
		NULL);

static struct kobj_attribute limiter_enabled_attribute =
	__ATTR(limiter_enabled, 0666,
		limiter_enabled_show,
		limiter_enabled_store);

static struct kobj_attribute suspend_defer_time_attribute =
	__ATTR(suspend_defer_time, 0666,
		suspend_defer_time_show,
		suspend_defer_time_store);

static struct kobj_attribute suspend_max_freq_attribute =
	__ATTR(suspend_max_freq, 0666,
		suspend_max_freq_show,
		suspend_max_freq_store);

static struct attribute *msm_cpufreq_limit_attrs[] =
	{
		&limiter_enabled_attribute.attr,
		&suspend_defer_time_attribute.attr,
		&suspend_max_freq_attribute.attr,
		&resume_max_freq_0.attr,
		&resume_max_freq_1.attr,
		&resume_max_freq_2.attr,
		&resume_max_freq_3.attr,
		&suspend_min_freq_0.attr,
		&suspend_min_freq_1.attr,
		&suspend_min_freq_2.attr,
		&suspend_min_freq_3.attr,
		&msm_cpufreq_limit_version_attribute.attr,
		NULL,
	};

static struct attribute_group msm_cpufreq_limit_attr_group =
	{
		.attrs = msm_cpufreq_limit_attrs,
	};

static struct kobject *msm_cpufreq_limit_kobj;

static int msm_cpufreq_limit_init(void)
{
	int ret;

	msm_cpufreq_limit_kobj =
		kobject_create_and_add(MSM_LIMIT, kernel_kobj);
	if (!msm_cpufreq_limit_kobj) {
		pr_err("%s msm_cpufreq_limit_kobj kobject create failed!\n",
			__func__);
		return -ENOMEM;
        }

	ret = sysfs_create_group(msm_cpufreq_limit_kobj,
			&msm_cpufreq_limit_attr_group);

        if (ret) {
		pr_err("%s msm_cpufreq_limit_kobj create failed!\n",
			__func__);
		goto err_dev;
	}

	if (limit.limiter_enabled)
		msm_cpufreq_limit_start();

	return ret;
err_dev:
	if (msm_cpufreq_limit_kobj != NULL)
		kobject_put(msm_cpufreq_limit_kobj);
	return ret;
}

static void msm_cpufreq_limit_exit(void)
{
	if (msm_cpufreq_limit_kobj != NULL)
		kobject_put(msm_cpufreq_limit_kobj);

	if (limit.limiter_enabled)
		msm_cpufreq_limit_stop();

}

late_initcall(msm_cpufreq_limit_init);
module_exit(msm_cpufreq_limit_exit);

MODULE_AUTHOR("Dorimanx <yuri@bynet.co.il>");
MODULE_AUTHOR("Pranav Vashi <neobuddy89@gmail.com>");
MODULE_DESCRIPTION("MSM CPU Frequency Limiter Driver");
MODULE_LICENSE("GPL v2");
