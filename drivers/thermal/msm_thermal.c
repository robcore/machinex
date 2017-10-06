/* Copyright (c) 2012-2017, The Linux Foundation. All rights reserved.
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
 * Added code to work as a standalone intelligent thermal throttling driver
 * for many Qualcomm SOCs by Paul Reioux (Faux123)
 * Modifications copyright (c) 2013~2014
 *
 * Further updated in a hacky way by Rob Patershuk (robcore) to integrate hardlimit
 * and the Mainline CPUFREQ Api.
 * Modifications copyright (c) 2013~2017
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/msm_tsens.h>
#include <linux/workqueue.h>
#include <linux/cpu.h>
#include <linux/cpufreq.h>
#include <linux/msm_tsens.h>
#include <linux/msm_thermal.h>
#include <linux/platform_device.h>
#include <linux/suspend.h>
#include <mach/cpufreq.h>
#include <linux/sysfs_helpers.h>
#include "../../arch/arm/mach-msm/acpuclock.h"

#define DEFAULT_POLLING_MS	360
#define SAFE_CONFIRMED 4

extern bool hotplug_ready;
static int limit_init;
static int enabled;
static void evaluate_temp(bool for_cc); /* Forward Declaration */

static struct msm_thermal_data msm_thermal_info = {
	.poll_ms = DEFAULT_POLLING_MS,
	.limit_temp_degC = 65,
	.temp_hysteresis_degC = 5,
	.freq_step = 2,
	.freq_control_mask = 0xf,
	.core_limit_temp_degC = 75,
	.core_temp_hysteresis_degC = 10,
	.core_control_mask = 0xe,
};

extern unsigned int hlimit_max_screen_on;
static struct delayed_work check_temp_work;
static struct workqueue_struct *intellithermal_wq;
bool core_control_enabled;
static uint32_t cpus_offlined;
static DEFINE_MUTEX(core_control_mutex);

static int limit_idx;
static int thermal_limit_low;
static int thermal_limit_high;
static struct cpufreq_frequency_table *table;

static bool therm_freq_limited;

enum {
	CORE_TEMP_SAFE = 0,
	CORE_TEMP_OVER = 1,
};

enum {
	FREQ_TEMP_SAFE = 0,
	FREQ_TEMP_OVER = 1,
};

unsigned long limited_max_freq_thermal = CPUFREQ_HARDLIMIT_MAX_SCREEN_ON_STOCK;
static bool thermal_suspended = false;
/* module parameters */
module_param_named(limit_temp_degC, msm_thermal_info.limit_temp_degC,
			int, 0664);
module_param_named(core_limit_temp_degC, msm_thermal_info.core_limit_temp_degC,
			int, 0664);
module_param_named(core_control_mask, msm_thermal_info.core_control_mask,
			uint, 0664);
module_param_named(freq_step, msm_thermal_info.freq_step, uint, 0644);
module_param_named(freq_limit_hysteresis, msm_thermal_info.temp_hysteresis_degC,
			 int, 0644);
module_param_named(core_limit_hysteresis, msm_thermal_info.core_temp_hysteresis_degC,
			 int, 0644);

bool thermal_core_controlled(unsigned int cpu)
{
	if (core_control_enabled &&
		(msm_thermal_info.core_control_mask & BIT(cpu)) &&
		(cpus_offlined & BIT(cpu)))
		return true;
	return false;
}

static int set_thermal_limit_low(const char *buf, const struct kernel_param *kp)
{
	unsigned int val, i, adjusted_freq;
	struct cpufreq_policy *policy;
	struct cpufreq_frequency_table *table;

	if (!sscanf(buf, "%d", &val))
		return -EINVAL;

	policy = cpufreq_cpu_get_raw(0);
	table = policy->freq_table; /* Get frequency table */
	if (table == NULL)
		return -EINVAL;

	sanitize_min_max(val, policy->cpuinfo.min_freq, (table[thermal_limit_high].frequency - 1));

	for (i = 0; (table[i].frequency != CPUFREQ_TABLE_END); i++)
		if (table[i].frequency == val) {
			thermal_limit_low = cpufreq_frequency_table_get_index(policy, val);
			if (thermal_limit_low < 0)
				thermal_limit_low = 0;
			return 0;
		}
	return -EINVAL;
}

static int get_thermal_limit_low(char *buf, const struct kernel_param *kp)
{
	ssize_t ret;
	struct cpufreq_policy *policy;
	struct cpufreq_frequency_table *table;

	policy = cpufreq_cpu_get_raw(0);
	table = policy->freq_table; /* Get frequency table */
	if (table == NULL)
		return -ENOSYS;

	ret = sprintf(buf, "%u", table[thermal_limit_low].frequency);

	return ret;
}

static const struct kernel_param_ops param_ops_thermal_limit_low = {
	.set = set_thermal_limit_low,
	.get = get_thermal_limit_low,
};

module_param_cb(thermal_limit_low, &param_ops_thermal_limit_low, NULL, 0644);

static int set_poll_ms(const char *buf, const struct kernel_param *kp)
{
	unsigned int val;

	/* single number: apply to all CPUs */
	if (sscanf(buf, "%u", &val) != 1)
		return -EINVAL;

	sanitize_min_max(val, 40, 1000); /*works best if in same multiple as thermal poll, 40ms. 1 sec max for safety*/

	msm_thermal_info.poll_ms = val;

	return 0;
}

static int get_poll_ms(char *buf, const struct kernel_param *kp)
{
	ssize_t ret;

	ret = sprintf(buf, "%d\n", msm_thermal_info.poll_ms);

	return ret;
}

static const struct kernel_param_ops param_ops_poll_ms = {
	.set = set_poll_ms,
	.get = get_poll_ms,
};

module_param_cb(poll_ms, &param_ops_poll_ms, NULL, 0644);

static int msm_thermal_get_freq_table(void)
{
	struct cpufreq_policy *policy;
	unsigned int i;

	policy = cpufreq_cpu_get_raw(0);

	if (policy == NULL || thermal_suspended) {
		return -EINVAL;
	}

	table = policy->freq_table;
	if (table == NULL) {
		return -EINVAL;
	}

	for (i = 0; table[i].frequency != CPUFREQ_TABLE_END; i++)
		thermal_limit_low = 4;
		limit_idx = i - 1;
		thermal_limit_high = limit_idx;

	BUG_ON(thermal_limit_high <= 0);
	pr_info("MSM Thermal: Initial thermal_limit_low is %u\n", table[thermal_limit_low].frequency);
	pr_info("MSM Thermal: Initial thermal_limit_high is %u\n", table[thermal_limit_high].frequency);

	return 0;
}

static void update_cpu_max_freq(unsigned long max_freq)
{
	int ret;
	unsigned int cpu = smp_processor_id();

	if (thermal_suspended)
		return;

	get_online_cpus();
	for_each_possible_cpu(cpu) {
		if (!(msm_thermal_info.freq_control_mask & BIT(cpu)))
			continue;
		reapply_hard_limits(cpu);
		limited_max_freq_thermal = max_freq;
		cpufreq_update_policy(cpu);
	}
	put_online_cpus();

}

static void __ref do_core_control(unsigned int over)
{
	unsigned int cpu = smp_processor_id();
	int ret = 0;
	int core_temp;

	if ((!core_control_enabled) || (intelli_init() ||
		 !hotplug_ready || thermal_suspended)) {
		return;
	}

	mutex_lock(&core_control_mutex);
	switch (core_temp) {
		case CORE_TEMP_OVER:
			if (msm_thermal_info.core_control_mask) {
				for (cpu = 3; cpu > 1; cpu--) {
					if (!(msm_thermal_info.core_control_mask & BIT(cpu)));
						continue;
					if (cpus_offlined & BIT(cpu) && !cpu_online(cpu))
						continue;
					ret = cpu_down(cpu);
					if (ret)
						pr_debug("cpu_down failed. you got problems\n");
					cpus_offlined |= BIT(cpu);
				}
			}
			break;
		case CORE_TEMP_SAFE:
			if (msm_thermal_info.core_control_mask && cpus_offlined) {
				for (cpu = 1; cpu < 3; cpu++) {
					if (!(cpus_offlined & BIT(cpu)))
						continue;
					/* If this core is already online, then bring up the
					 * next offlined core.
					 */
					if (cpu_online(cpu))
						continue;
					if (!is_cpu_allowed(cpu))
						continue;
					ret = cpu_up(cpu);
					if (ret)
						pr_err("%s: Error %d online core %d\n",
								KBUILD_MODNAME, ret, cpu);
					cpus_offlined &= ~BIT(cpu);
				}
			}
			break;
	}
	mutex_unlock(&core_control_mutex);
}

static void __ref do_freq_control(unsigned int over)
{
	int ret = 0;
	struct cpufreq_policy policy;
	unsigned int cpu = smp_processor_id();
	unsigned long max_freq;
	int freq_temp;
	bool hotplug_check_needed = true;

	if (!hotplug_ready || thermal_suspended) {
		pr_err("frequency control not ready!\n");		
		return;
	}

	ret = cpufreq_get_policy(&policy, cpu);
		if (ret)
			return;

	max_freq = limited_max_freq_thermal;

	switch (freq_temp) {
		case FREQ_TEMP_OVER:
			if (limit_idx == thermal_limit_low) {
				goto check_need;
			}
			limit_idx -= msm_thermal_info.freq_step;
			if (limit_idx < thermal_limit_low)
				limit_idx = thermal_limit_low;
			max_freq = table[limit_idx].frequency;
			break;
		case FREQ_TEMP_SAFE:
			if (limit_idx == thermal_limit_high) {
				goto check_need;
			}
			limit_idx += msm_thermal_info.freq_step;
			if (limit_idx >= thermal_limit_high) {
				limit_idx = thermal_limit_high;
				max_freq = policy.hlimit_max_screen_on;
			} else {
				max_freq = table[limit_idx].frequency;
			}
			break;
	}

	if (max_freq == limited_max_freq_thermal) {
		goto check_need;
	}

	update_cpu_max_freq(max_freq);

check_need:
	if (max_freq == policy.hlimit_max_screen_on)
		hotplug_check_needed = false;

	if (likely(msm_thermal_info.limit_temp_degC <
		msm_thermal_info.core_limit_temp_degC)) {
		if (hotplug_check_needed)
			evaluate_temp(true);
	} else {
		evaluate_temp(true);
	}
}

/*
 * for_cc: indicates whether we are calculating
 * temperature for frequency or core control.
 */
static void evaluate_temp(bool for_cc)
{
	struct tsens_device tsens_dev;
	long temp;
	int ret = 0;
	uint32_t i;
	unsigned int safe = 0, fail = 0;
	unsigned long delta;

	if (thermal_suspended)
		return;

	if (for_cc) {
		delta = (msm_thermal_info.core_limit_temp_degC -
				 msm_thermal_info.core_temp_hysteresis_degC);
		for (i = 10; i > 7; i--) {
			tsens_dev.sensor_num = i;
			ret = tsens_get_temp(&tsens_dev, &temp);
			if (!temp) {
				pr_err("%s: Unable to read TSENS sensor %d\n",
						KBUILD_MODNAME, tsens_dev.sensor_num);
				if (i > 7) {
					fail++;
					continue;
				} else
					break;
			}
			if (temp >= msm_thermal_info.core_limit_temp_degC)
				do_core_control(CORE_TEMP_OVER);
			else if (temp <= delta)
				safe++;
		}
		if (!fail || safe > fail)
			do_core_control(CORE_TEMP_SAFE);

		do_core_control(CORE_TEMP_OVER);
	} else {
		delta = (msm_thermal_info.limit_temp_degC - 
				 msm_thermal_info.temp_hysteresis_degC);

		for (i = 7; i < 10; i++) {
			tsens_dev.sensor_num = i;
			ret = tsens_get_temp(&tsens_dev, &temp);
			if (unlikely(!temp)) {
				pr_err("%s: Unable to read TSENS sensor %d\n",
						KBUILD_MODNAME, tsens_dev.sensor_num);
				if (i < 10) {
					fail++;
					continue;
				} else
					break;
			}
	
			if (temp >= msm_thermal_info.limit_temp_degC ||
				temp > delta)
				do_freq_control(FREQ_TEMP_OVER);
			else if (temp <= delta)
				safe++;
		}

		if (safe == SAFE_CONFIRMED)
			return;
	
		if (!fail || safe > fail)
			do_freq_control(FREQ_TEMP_SAFE);
	
		do_freq_control(FREQ_TEMP_OVER);
	}
}

static void __ref check_temp(struct work_struct *work)
{
	int ret = 0;

	if (thermal_suspended)
		return;

	if (!limit_init) {
		ret = msm_thermal_get_freq_table();
		if (ret)
			goto reschedule;
		else
			limit_init = 1;
	}

	evaluate_temp(false);

reschedule:
	mod_delayed_work_on(0, intellithermal_wq, &check_temp_work,
			msecs_to_jiffies(msm_thermal_info.poll_ms));
}

static int __ref msm_thermal_cpu_callback(struct notifier_block *nfb,
		unsigned long action, void *hcpu)
{
	unsigned int cpu = (unsigned long)hcpu;

	if (thermal_suspended || !hotplug_ready)
		return NOTIFY_OK;

	if (action == CPU_UP_PREPARE || action == CPU_UP_PREPARE_FROZEN) {
		if (core_control_enabled &&
			(msm_thermal_info.core_control_mask & BIT(cpu)) &&
			(cpus_offlined & BIT(cpu))) {
			pr_debug(
			"%s: Preventing cpu%d from coming online.\n",
				KBUILD_MODNAME, cpu);
			return NOTIFY_BAD;
		}
	}
	return NOTIFY_OK;
}

static struct notifier_block __refdata msm_thermal_cpu_notifier = {
	.notifier_call = msm_thermal_cpu_callback,
};

/**
 * We will reset the cpu frequencies limits here. The core online/offline
 * status will be carried over to the process stopping the msm_thermal, as
 * we dont want to online a core and bring in the thermal issues.
 */
static void __ref disable_msm_thermal(void)
{
	struct cpufreq_policy *policy;
	unsigned int cpu = smp_processor_id();

	policy = cpufreq_cpu_get_raw(cpu);
		if (!policy)
			return;

	cancel_delayed_work_sync(&check_temp_work);
	destroy_workqueue(intellithermal_wq);

	if (limited_max_freq_thermal == policy->hlimit_max_screen_on)
		return;
	else
		(limited_max_freq_thermal = policy->hlimit_max_screen_on);
	reapply_hard_limits(cpu);
	update_cpu_max_freq(limited_max_freq_thermal);
}

/* Call with core_control_mutex locked */
static void __ref update_offline_cores(int val)
{
	unsigned int cpu = 0;
	int ret = 0;

	if (!core_control_enabled || intelli_init() ||
		thermal_suspended || !hotplug_ready)
		return;

	cpus_offlined = msm_thermal_info.core_control_mask & val;
	for_each_nonboot_online_cpu(cpu) {
		if (cpu < 0 || cpu >= NR_CPUS ||
			!cpu_online(cpu))
			continue;
		if (!(cpus_offlined & BIT(cpu)))
			continue;
		ret = cpu_down(cpu);
		if (ret)
			pr_err("%s: Unable to offline cpu%d\n",
				KBUILD_MODNAME, cpu);
	}
}

static ssize_t show_cc_enabled(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", core_control_enabled);
}

static ssize_t __ref store_cc_enabled(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	int val = 0;

	mutex_lock(&core_control_mutex);
	ret = kstrtoint(buf, 10, &val);
	if (ret) {
		pr_err("%s: Invalid input %s\n", KBUILD_MODNAME, buf);
		goto done_store_cc;
	}

	if (core_control_enabled == !!val)
		goto done_store_cc;

	core_control_enabled = !!val;
	if (core_control_enabled) {
		pr_debug("%s: Core control enabled\n", KBUILD_MODNAME);
		register_cpu_notifier(&msm_thermal_cpu_notifier);
		update_offline_cores(cpus_offlined);
	} else {
		pr_debug("%s: Core control disabled\n", KBUILD_MODNAME);
		unregister_cpu_notifier(&msm_thermal_cpu_notifier);
	}

done_store_cc:
	mutex_unlock(&core_control_mutex);
	return count;
}

static ssize_t show_cpus_offlined(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", cpus_offlined);
}

static ssize_t __ref store_cpus_offlined(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	uint32_t val = 0;

	mutex_lock(&core_control_mutex);
	ret = kstrtouint(buf, 10, &val);
	if (ret) {
		pr_err("%s: Invalid input %s\n", KBUILD_MODNAME, buf);
		goto done_cc;
	}

	if (enabled) {
		pr_err("%s: Ignoring request; polling thread is enabled.\n",
				KBUILD_MODNAME);
		goto done_cc;
	}

	if (cpus_offlined == val)
		goto done_cc;

	update_offline_cores(val);
done_cc:
	mutex_unlock(&core_control_mutex);
	return count;
}

static __refdata struct kobj_attribute cc_enabled_attr =
__ATTR(enabled, 0644, show_cc_enabled, store_cc_enabled);

static __refdata struct kobj_attribute cpus_offlined_attr =
__ATTR(cpus_offlined, 0644, show_cpus_offlined, store_cpus_offlined);

static __refdata struct attribute *cc_attrs[] = {
	&cc_enabled_attr.attr,
	&cpus_offlined_attr.attr,
	NULL,
};

static __refdata struct attribute_group cc_attr_group = {
	.attrs = cc_attrs,
};

static __init int msm_thermal_add_cc_nodes(void)
{
	struct kobject *module_kobj = NULL;
	struct kobject *cc_kobj = NULL;
	int ret = 0;

	module_kobj = kset_find_obj(module_kset, KBUILD_MODNAME);
	if (!module_kobj) {
		pr_err("%s: cannot find kobject for module\n",
			KBUILD_MODNAME);
		ret = -ENOENT;
		goto done_cc_nodes;
	}

	cc_kobj = kobject_create_and_add("core_control", module_kobj);
	if (!cc_kobj) {
		pr_err("%s: cannot create core control kobj\n",
				KBUILD_MODNAME);
		ret = -ENOMEM;
		goto done_cc_nodes;
	}

	ret = sysfs_create_group(cc_kobj, &cc_attr_group);
	if (ret) {
		pr_err("%s: cannot create group\n", KBUILD_MODNAME);
		goto done_cc_nodes;
	}

	return 0;

done_cc_nodes:
	if (cc_kobj)
		kobject_del(cc_kobj);
	return ret;
}

static int msm_thermal_pm_event(struct notifier_block *this,
				unsigned long event, void *ptr)
{
	switch (event) {
	case PM_POST_SUSPEND:
		thermal_suspended = false;
		if (mutex_trylock(&core_control_mutex)) {
			update_offline_cores(cpus_offlined);
			mutex_unlock(&core_control_mutex);
		}
		mod_delayed_work_on(0, intellithermal_wq, &check_temp_work, 0);
		break;
	case PM_SUSPEND_PREPARE:
		thermal_suspended = true;
		break;
	default:
		break;
	}
	return NOTIFY_DONE;
}

static struct notifier_block msm_thermal_pm_notifier = {
	.notifier_call = msm_thermal_pm_event,
};

int __init msm_thermal_init(void)
{
	struct msm_thermal_data *msm_thermal_info;
	msm_thermal_info = kzalloc(sizeof(struct msm_thermal_data), GFP_KERNEL);
	if (!msm_thermal_info)
		return -ENOMEM;
	enabled = 1;
	mutex_init(&core_control_mutex);
	intellithermal_wq = create_hipri_workqueue("intellithermal");
	INIT_DELAYED_WORK(&check_temp_work, check_temp);
	if ((num_possible_cpus() > 1) && (core_control_enabled == true))
		register_cpu_notifier(&msm_thermal_cpu_notifier);
	register_pm_notifier(&msm_thermal_pm_notifier);
	queue_delayed_work_on(0, intellithermal_wq, &check_temp_work, 0);
	return 0;
}

int __init msm_thermal_late_init(void)
{
	if (num_possible_cpus() > 1)
		msm_thermal_add_cc_nodes();

	return 0;
}

late_initcall(msm_thermal_late_init);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Praveen Chidambaram <pchidamb@codeaurora.org>");
MODULE_AUTHOR("Paul Reioux <reioux@gmail.com>");
MODULE_AUTHOR("Rob Patershuk <robpatershuk@gmail.com>");
MODULE_DESCRIPTION("intelligent thermal driver for Qualcomm based SOCs");
MODULE_DESCRIPTION("originally from Qualcomm's open source repo");
