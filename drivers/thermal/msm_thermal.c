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
#include <linux/kthread.h>
#include <linux/cpu.h>
#include <linux/cpufreq.h>
#include <linux/msm_tsens.h>
#include <linux/msm_thermal.h>
#include <linux/platform_device.h>
#include <linux/suspend.h>
#include <mach/cpufreq.h>
#include <linux/sysfs_helpers.h>
#include <linux/display_state.h>
#include <linux/reboot.h>
#include "../../arch/arm/mach-msm/acpuclock.h"

#define MAX_IDX 14
#define SHUTOFF_TEMP 85

static struct msm_thermal_data msm_thermal_info = {
	.poll_ms = 240,
	.limit_temp_degC = 65,
	.temp_hysteresis_degC = 5,
	.freq_step = 2,
	.core_limit_temp_degC = 75,
	.core_temp_hysteresis_degC = 10,
};

static int limit_idx[NR_CPUS];
static int thermal_limit_low[NR_CPUS];

static const uint32_t soc_sens_id = 0;
static const uint32_t msm_sens_id[NR_CPUS] = { 7, 8, 9, 10 };

static struct delayed_work check_temp_work;
static struct work_struct get_table_work;
static struct workqueue_struct *intellithermal_wq;
static bool core_control_enabled;
static cpumask_t core_control_mask;
static cpumask_t cores_offlined_mask;
static DEFINE_MUTEX(core_control_mutex);

static struct cpufreq_frequency_table *therm_table;
static bool thermal_suspended = false;

bool thermal_core_controlled(unsigned int cpu)
{
	if (core_control_enabled &&
		cpumask_test_cpu(cpu, &core_control_mask) &&
		cpumask_test_cpu(cpu, &cores_offlined_mask))
		return true;
	return false;
}

/*************************************************************************
 *                          Module Parameters                            *
 *************************************************************************/

static int set_thermal_limit_low(const char *buf, const struct kernel_param *kp)
{
	unsigned int val, cpu = 0;
	int i, temp_low = -1;

	if (!sscanf(buf, "%u", &val))
		return -EINVAL;

	sanitize_min_max(val, 384000, 1782000);

	for (i = 0; (therm_table[i].frequency != CPUFREQ_TABLE_END); i++) {
		if (therm_table[i].frequency == val) {
			temp_low = i;
			break;
		}
	}

	if (temp_low < 0)
		return -EINVAL;

	for_each_possible_cpu(cpu) {
		if (cpu_out_of_range(cpu))
			break;
		thermal_limit_low[cpu] = temp_low;
	}

	return 0;
}

static int get_thermal_limit_low(char *buf, const struct kernel_param *kp)
{
	ssize_t ret;
	unsigned int cpu = 0;

	ret = sprintf(buf, "%u", therm_table[thermal_limit_low[cpu]].frequency);

	return ret;
}

static const struct kernel_param_ops param_ops_thermal_limit_low = {
	.set = set_thermal_limit_low,
	.get = get_thermal_limit_low,
};

module_param_cb(thermal_limit_low, &param_ops_thermal_limit_low, NULL, 0644);

static int set_limit_temp_degC(const char *buf, const struct kernel_param *kp)
{
	int val;
	if (!sscanf(buf, "%d", &val))
		return -EINVAL;

	sanitize_min_max(val, 35, msm_thermal_info.core_limit_temp_degC + 1);

	msm_thermal_info.limit_temp_degC = val;
	return 0;
}

static int get_limit_temp_degC(char *buf, const struct kernel_param *kp)
{
	ssize_t ret = 0;

	ret = sprintf(buf, "%d", msm_thermal_info.limit_temp_degC);

	return ret;
}

static const struct kernel_param_ops param_ops_limit_temp_degC = {
	.set = set_limit_temp_degC,
	.get = get_limit_temp_degC,
};
module_param_cb(limit_temp_degC, &param_ops_limit_temp_degC, NULL, 0664);

static int set_temp_hysteresis_degC(const char *buf, const struct kernel_param *kp)
{
	int val;
	if (!sscanf(buf, "%d", &val))
		return -EINVAL;

	sanitize_min_max(val, 1, 20);

	msm_thermal_info.temp_hysteresis_degC = val;
	return 0;
}

static int get_temp_hysteresis_degC(char *buf, const struct kernel_param *kp)
{
	ssize_t ret = 0;

	ret = sprintf(buf, "%d", msm_thermal_info.temp_hysteresis_degC);

	return ret;
}

static const struct kernel_param_ops param_ops_temp_hysteresis_degC = {
	.set = set_temp_hysteresis_degC,
	.get = get_temp_hysteresis_degC,
};
module_param_cb(freq_limit_hysteresis, &param_ops_temp_hysteresis_degC, NULL, 0644);

static int set_core_limit_temp_degC(const char *buf, const struct kernel_param *kp)
{
	int val;
	if (!sscanf(buf, "%d", &val))
		return -EINVAL;

	sanitize_min_max(val, msm_thermal_info.limit_temp_degC - 1, 80);

	msm_thermal_info.core_limit_temp_degC = val;
	return 0;
}

static int get_core_limit_temp_degC(char *buf, const struct kernel_param *kp)
{
	ssize_t ret = 0;

	ret = sprintf(buf, "%d", msm_thermal_info.core_limit_temp_degC);

	return ret;
}

static const struct kernel_param_ops param_ops_core_limit_temp_degC = {
	.set = set_core_limit_temp_degC,
	.get = get_core_limit_temp_degC,
};
module_param_cb(core_limit_temp_degC, &param_ops_core_limit_temp_degC, NULL, 0664);

static int set_core_temp_hysteresis_degC(const char *buf, const struct kernel_param *kp)
{
	int val;
	if (!sscanf(buf, "%d", &val))
		return -EINVAL;

	sanitize_min_max(val, 1, 20);

	msm_thermal_info.core_temp_hysteresis_degC = val;
	return 0;
}

static int get_core_temp_hysteresis_degC(char *buf, const struct kernel_param *kp)
{
	ssize_t ret = 0;

	ret = sprintf(buf, "%d", msm_thermal_info.core_temp_hysteresis_degC);

	return ret;
}

static const struct kernel_param_ops param_ops_core_temp_hysteresis_degC = {
	.set = set_core_temp_hysteresis_degC,
	.get = get_core_temp_hysteresis_degC,
};
module_param_cb(core_limit_hysteresis, &param_ops_core_temp_hysteresis_degC, NULL, 0644);

static int set_freq_step(const char *buf, const struct kernel_param *kp)
{
	unsigned int val;

	if (sscanf(buf, "%u", &val) != 1)
		return -EINVAL;

	/*15 frequencies, 5 steps max seems reasonable*/
	sanitize_min_max(val, 1, 5);

	msm_thermal_info.freq_step = val;

	return 0;
}

static int get_freq_step(char *buf, const struct kernel_param *kp)
{
	ssize_t ret;

	ret = sprintf(buf, "%u\n", msm_thermal_info.freq_step);

	return ret;
}

static const struct kernel_param_ops param_ops_freq_step = {
	.set = set_freq_step,
	.get = get_freq_step,
};

module_param_cb(freq_step, &param_ops_freq_step, NULL, 0644);

static int set_poll_ms(const char *buf, const struct kernel_param *kp)
{
	unsigned int val;

	if (sscanf(buf, "%u", &val) != 1)
		return -EINVAL;

	sanitize_min_max(val, 40, 1000); /*works best if in same multiple as thermal poll, 40ms. 1 sec max for safety*/

	msm_thermal_info.poll_ms = (long long unsigned int)val;

	return 0;
}

static int get_poll_ms(char *buf, const struct kernel_param *kp)
{
	ssize_t ret;

	ret = sprintf(buf, "%llu\n", msm_thermal_info.poll_ms);

	return ret;
}

static const struct kernel_param_ops param_ops_poll_ms = {
	.set = set_poll_ms,
	.get = get_poll_ms,
};

module_param_cb(poll_ms, &param_ops_poll_ms, NULL, 0644);

/*************************************************************************
 *                       END Module Parameters                           *
 *************************************************************************/
static int msm_thermal_get_freq_table(void)
{
	struct cpufreq_policy *policy;
	unsigned int templow, cpu, smartcheck, smartlow;
	int i;

	if (thermal_suspended)
		return -EINVAL;

	policy = cpufreq_cpu_get_raw(0);
	if (policy == NULL)
		return -ENOMEM;

	therm_table = policy->freq_table;
	if (!therm_table)
		return -ENOMEM;

	for (i = 0; (therm_table[i].frequency != CPUFREQ_TABLE_END); i++)
			if (therm_table[i].frequency == DEFAULT_THERMIN)
				templow = i;

	if (templow == 4) {
		pr_info("MSM Thermal: Initial thermal_limit_low is %u\n", therm_table[templow].frequency);
		smartlow = templow;
	}

	smartcheck = i - 1;

	for_each_possible_cpu(cpu) {
		if (cpu_out_of_range(cpu))
			break;
		if (smartlow) {
			thermal_limit_low[cpu] = smartlow;
			sanitize_min_max(thermal_limit_low[cpu], smartlow, smartlow);
		} else {
			thermal_limit_low[cpu] = 4;
			sanitize_min_max(thermal_limit_low[cpu], 4, 4);
			pr_info("MSM Thermal: WARNING! Initial freq count FAILED! Value was %u\n and is fixed", templow);
		}
		if (unlikely(smartcheck != MAX_IDX)) {
			limit_idx[cpu] = MAX_IDX;
			sanitize_min_max(limit_idx[cpu], MAX_IDX, MAX_IDX);
			continue;
		} else {
			limit_idx[cpu] = smartcheck;
			sanitize_min_max(limit_idx[cpu], smartcheck, smartcheck);
		}
	}
	return 0;
}

static long evaluate_temp(unsigned int cpu)
{
	struct tsens_device tsens_dev;
	long temp;
	int ret = 0;

	if (thermal_suspended)
		return -EINVAL;

	tsens_dev.sensor_num = msm_sens_id[cpu];
	ret = tsens_get_temp(&tsens_dev, &temp);
	if (!temp || ret) {
		pr_err("%s: Unable to read TSENS sensor %u\n",
				KBUILD_MODNAME, tsens_dev.sensor_num);
		return -EINVAL;
	}

	if (unlikely(temp > SHUTOFF_TEMP))
		orderly_poweroff(true);

	return temp;
}

static long evaluate_any_temp(uint32_t sens_id)
{
	struct tsens_device tsens_dev;
	long temp;
	int ret = 0;

	if (thermal_suspended)
		return -EINVAL;

	tsens_dev.sensor_num = sens_id;
	ret = tsens_get_temp(&tsens_dev, &temp);
	if (!temp || ret) {
		pr_err("%s: Unable to read TSENS sensor %u\n",
				KBUILD_MODNAME, tsens_dev.sensor_num);
		return -EINVAL;
	}

	if (unlikely(temp > SHUTOFF_TEMP))
		orderly_poweroff(true);

	return temp;
}
static int __ref mitigation_control(void)
{
	int ret = 0;
	unsigned int cpu = smp_processor_id();
	long freq_temp, core_temp, delta;
	unsigned int resolve_max_freq[NR_CPUS];

	if (thermal_suspended)	
		return -EINVAL;

	delta = (msm_thermal_info.limit_temp_degC -
			 msm_thermal_info.temp_hysteresis_degC);

	for_each_possible_cpu(cpu) {
		if (cpu_out_of_range(cpu))
			break;
		freq_temp = evaluate_temp(cpu);
		if (freq_temp <= 0) {
			continue;
		}
		resolve_max_freq[cpu] = limited_max_freq_thermal[cpu];
		if (freq_temp >= msm_thermal_info.limit_temp_degC) {
				if (limit_idx[cpu] <= thermal_limit_low[cpu]) {
					limit_idx[cpu] = thermal_limit_low[cpu];
					if (unlikely(limited_max_freq_thermal[cpu] > resolve_max_freq[cpu]))
						set_thermal_policy(cpu, resolve_max_freq[cpu]);
					continue;
				}
				if (limit_idx[cpu] >= thermal_limit_low[cpu] + msm_thermal_info.freq_step)
					limit_idx[cpu] -= msm_thermal_info.freq_step;
				if (limit_idx[cpu] <= thermal_limit_low[cpu])
					limit_idx[cpu] = thermal_limit_low[cpu];
				resolve_max_freq[cpu] = therm_table[limit_idx[cpu]].frequency;
		} else if (freq_temp < delta) {
				if (limit_idx[cpu] >= MAX_IDX) {
					limit_idx[cpu] = MAX_IDX;
					resolve_max_freq[cpu] = get_hardlimit_max(cpu);
					/* Satisfy suspend/resume type cases where we haven't updated the
					 * thermal limit in time. ie. suspend/resume
					 */
					if (unlikely(limited_max_freq_thermal[cpu] < resolve_max_freq[cpu]))
						set_thermal_policy(cpu, resolve_max_freq[cpu]);
					continue;
				}
				if (limit_idx[cpu] <= MAX_IDX - msm_thermal_info.freq_step)
					limit_idx[cpu] += msm_thermal_info.freq_step;
				if (limit_idx[cpu] >= MAX_IDX) {
					limit_idx[cpu] = MAX_IDX;
					resolve_max_freq[cpu] = get_hardlimit_max(cpu);
				} else {
					resolve_max_freq[cpu] = therm_table[limit_idx[cpu]].frequency;
				}
		}

		if (resolve_max_freq[cpu] != limited_max_freq_thermal[cpu])
			set_thermal_policy(cpu, resolve_max_freq[cpu]);
	}

	if (thermal_suspended)
		return -EINVAL;

	if (!core_control_enabled || intelli_init() ||
		 !thermal_is_throttling() ||
		 cpumask_empty(&core_control_mask)) {
		return 0;
	}

	delta = (msm_thermal_info.core_limit_temp_degC -
			 msm_thermal_info.core_temp_hysteresis_degC);

	mutex_lock(&core_control_mutex);
	for_each_possible_cpu(cpu) {
		if (cpu_out_of_range_hp(cpu))
			break;
		core_temp = evaluate_temp(cpu);
		if (core_temp < 0)
			continue;
		if (core_temp >= msm_thermal_info.core_limit_temp_degC &&
			cpumask_test_cpu(cpu, &core_control_mask)) {
				cpumask_set_cpu(cpu, &cores_offlined_mask);
				if (cpumask_test_cpu(cpu, &cores_offlined_mask) &&
					!cpu_online(cpu))
					continue;
				ret = cpu_down(cpu);
				if (ret)
					pr_debug("cpu_down failed. you got problems\n");
		} else if (core_temp < delta &&
				   cpumask_test_cpu(cpu, &core_control_mask) &&
				   cpumask_test_cpu(cpu, &cores_offlined_mask)) {
				/* If this core is already online, then bring up the
				 * next offlined core.
				 */
				cpumask_clear_cpu(cpu, &cores_offlined_mask);
				if (cpu_online(cpu))
					continue;
				if (!is_cpu_allowed(cpu))
					continue;
				ret = cpu_up(cpu);
				if (ret)
					pr_err("%s: Error %d online core %u\n",
							KBUILD_MODNAME, ret, cpu);
		}
	}
	mutex_unlock(&core_control_mutex);
	return 0;
}

static void __ref check_temp(struct work_struct *work)
{
	int ret = 0;

	if (thermal_suspended)
		return;
	ret = mitigation_control();
	if (ret)
		return;

	queue_delayed_work(intellithermal_wq, &check_temp_work,
			msecs_to_jiffies(msm_thermal_info.poll_ms));
}

static void __ref get_table(struct work_struct *work)
{
	int ret;
	unsigned int i;
	if (system_state != SYSTEM_RUNNING)
		goto reschedule;

	ret = msm_thermal_get_freq_table();
	if (ret)
		goto reschedule;

	queue_delayed_work(intellithermal_wq, &check_temp_work, 0);
	return;
reschedule:
		schedule_work(&get_table_work);
}

/* Call with core_control_mutex locked */
static void __ref update_offline_cores(void)
{
	unsigned int cpu = 0;
	int ret = 0;

	if (!core_control_enabled || intelli_init() ||
		thermal_suspended)
		return;

	for_each_nonboot_online_cpu(cpu) {
		if (cpu_out_of_range_hp(cpu))
			break;
		if (!cpu_online(cpu))
			continue;
		if (cpumask_test_cpu(cpu, &core_control_mask) &&
			cpumask_test_cpu(cpu, &cores_offlined_mask))
		ret = cpu_down(cpu);
		if (ret)
			pr_err("%s: Unable to offline cpu%u\n",
				KBUILD_MODNAME, cpu);
	}
}

static int show_cc_mask(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return cpumap_print_to_pagebuf(true, buf, &core_control_mask);
}

static __refdata struct kobj_attribute cc_mask =
__ATTR(core_control_mask, 0444, show_cc_mask, NULL);

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
		update_offline_cores();
	} else {
		pr_debug("%s: Core control disabled\n", KBUILD_MODNAME);
		cpumask_clear(&cores_offlined_mask);
	}

done_store_cc:
	mutex_unlock(&core_control_mutex);
	return count;
}

static __refdata struct kobj_attribute cc_enabled =
__ATTR(enabled, 0644, show_cc_enabled, store_cc_enabled);

#define show_therm_cpu(object, cpu_core)				\
static ssize_t object##_show(struct kobject *kobj,	\
		struct kobj_attribute *attr, char *buf)	\
{								\
	unsigned int tmp;	\
	tmp = cpumask_test_cpu((cpu_core), &core_control_mask);	\
	return sprintf(buf, "%u", tmp);	\
}

#define store_therm_cpu(name, cpu_core)		\
static ssize_t name##_store		\
(struct kobject *kobj,	\
		struct kobj_attribute *attr, const char *buf, size_t count)	\
{	\
	unsigned int val;	\
	\
	if (!sscanf(buf, "%u", &val))	\
		return -EINVAL;	\
	\
		sanitize_min_max(val, 0, 1);	\
	\
	if (val == cpumask_test_cpu((cpu_core), &core_control_mask))	\
		return count;	\
	else	\
		val ? cpumask_set_cpu((cpu_core), &core_control_mask) :	\
			cpumask_clear_cpu((cpu_core), &core_control_mask);	\
	\
	return count;	\
}

show_therm_cpu(core1, 1);
show_therm_cpu(core2, 2);
show_therm_cpu(core3, 3);
store_therm_cpu(core1, 1);
store_therm_cpu(core2, 2);
store_therm_cpu(core3, 3);

static __refdata struct kobj_attribute core1 =
__ATTR(core1, 0644, core1_show, core1_store);

static __refdata struct kobj_attribute core2 =
__ATTR(core2, 0644, core2_show, core2_store);

static __refdata struct kobj_attribute core3 =
__ATTR(core3, 0644, core3_show, core3_store);

static __refdata struct attribute *cc_attrs[] = {
	&cc_mask.attr,
	&cc_enabled.attr,
	&core1.attr,
	&core2.attr,
	&core3.attr,
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
	unsigned int cpu;

	switch (event) {
	case PM_PROACTIVE_SUSPEND:
		thermal_suspended = true;
		break;
	case PM_PROACTIVE_RESUME:
		thermal_suspended = false;
		if (mutex_trylock(&core_control_mutex)) {
			update_offline_cores();
			mutex_unlock(&core_control_mutex);
		}
		mod_delayed_work(intellithermal_wq, &check_temp_work, 0);
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
	unsigned int cpu;

	msm_thermal_info = kzalloc(sizeof(struct msm_thermal_data), GFP_KERNEL);
	if (!msm_thermal_info)
		return -ENOMEM;

	cpumask_copy(&core_control_mask, cpu_nonboot_mask);
	cpumask_clear(&cores_offlined_mask);

	mutex_init(&core_control_mutex);
	intellithermal_wq = create_hipri_workqueue("intellithermal");
	INIT_WORK(&get_table_work, get_table);
	INIT_DELAYED_WORK(&check_temp_work, check_temp);
	register_pm_notifier(&msm_thermal_pm_notifier);
	schedule_work(&get_table_work);
	return 0;
}

int __init msm_thermal_late_init(void)
{
	msm_thermal_add_cc_nodes();

	return 0;
}

late_initcall(msm_thermal_late_init);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Praveen Chidambaram <pchidamb@codeaurora.org>");
MODULE_AUTHOR("Paul Reioux <reioux@gmail.com>");
MODULE_AUTHOR("Rob Patershuk <robpatershuk@gmail.com>");
MODULE_DESCRIPTION("intelligent thermal driver for Qualcomm based SOCs");
MODULE_DESCRIPTION("originally from Qualcomm's open source repo, further");
MODULE_DESCRIPTION("modified by Rob Patershuk");
