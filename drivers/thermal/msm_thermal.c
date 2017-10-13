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

static int enabled;

static struct msm_thermal_data msm_thermal_info = {
	.poll_ms = 360,
	.limit_temp_degC = 65,
	.temp_hysteresis_degC = 5,
	.freq_step = 2,
	.core_limit_temp_degC = 75,
	.core_temp_hysteresis_degC = 10,
	.core_control_mask = 0xe,
};

static int limit_idx[NR_CPUS];
static int thermal_limit_low[NR_CPUS];
static int thermal_limit_high[NR_CPUS];
static unsigned int local_max_freq_thermal[NR_CPUS];
static unsigned int resolve_max_freq[NR_CPUS];

static uint32_t msm_sens_id[NR_CPUS] = { 7, 8, 9, 10 };

static struct delayed_work check_temp_work;
static struct work_struct get_table_work;
static struct workqueue_struct *intellithermal_wq;
static bool core_control_enabled;
static uint32_t cpus_offlined;
static DEFINE_MUTEX(core_control_mutex);

static struct cpufreq_frequency_table *table;
static bool thermal_suspended = false;

/*************************************************************************
 *                          Module Parameters                            *
 *************************************************************************/
module_param_named(core_control_mask, msm_thermal_info.core_control_mask,
			uint, 0664);

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
	unsigned int val, cpu = 0;
	unsigned int i;
	bool should_apply;
	struct cpufreq_policy *policy;
	struct cpufreq_frequency_table *table;
	unsigned int temp_low;

	if (!sscanf(buf, "%u", &val))
		return -EINVAL;

	sanitize_min_max(val, 0, 14);

	policy = cpufreq_cpu_get_raw(cpu);
	if (policy == NULL)
		return -ENOMEM;
	table = policy->freq_table; /* Get frequency table */
	if (table == NULL)
		return -ENOMEM;


	for (i = 0; (table[i].frequency != CPUFREQ_TABLE_END); i++) {
		if (table[i].frequency == val) {
			temp_low = cpufreq_frequency_table_get_index(policy, val);
			break;
		}
	}
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
	struct cpufreq_policy *policy;
	struct cpufreq_frequency_table *table;
	unsigned int cpu = 0;

	policy = cpufreq_cpu_get_raw(cpu);
	if (policy == NULL)
		return -ENOSYS;
	table = policy->freq_table; /* Get frequency table */
	if (table == NULL)
		return -ENOSYS;

	ret = sprintf(buf, "%u", table[thermal_limit_low[cpu]].frequency);

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

	msm_thermal_info.poll_ms = val;

	return 0;
}

static int get_poll_ms(char *buf, const struct kernel_param *kp)
{
	ssize_t ret;

	ret = sprintf(buf, "%u\n", msm_thermal_info.poll_ms);

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
#define DEFAULT_THERMIN 810000
static int msm_thermal_get_freq_table(void)
{
	struct cpufreq_policy *policy;
	unsigned int templow, cpu;
	int i;

	if (!hotplug_ready || thermal_suspended)
		return -EINVAL;

	policy = cpufreq_cpu_get_raw(0);
	if (policy == NULL)
		return -ENOMEM;

	table = policy->freq_table;
	if (!table)
		return -ENOMEM;

	for (i = 0; (table[i].frequency != CPUFREQ_TABLE_END); i++)
			if (table[i].frequency == DEFAULT_THERMIN)
				templow = i;

	for_each_possible_cpu(cpu) {
		if (cpu_out_of_range(cpu))
			break;
		thermal_limit_low[cpu] = templow;
		limit_idx[cpu] = i - 1;
		thermal_limit_high[cpu] = limit_idx[cpu];
		sanitize_min_max(limit_idx[cpu], 0, 14);
		sanitize_min_max(thermal_limit_high[cpu], 1, 14);
		sanitize_min_max(thermal_limit_low[cpu], 0, 14);
	}

	pr_info("MSM Thermal: Initial thermal_limit_low is %u\n", table[thermal_limit_low[0]].frequency);
	pr_info("MSM Thermal: Initial thermal_limit_high is %u\n", table[thermal_limit_high[0]].frequency);	

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
	return temp;
}

static int __ref do_freq_control(void)
{
	int ret = 0;
	unsigned int cpu = smp_processor_id();
	long freq_temp, delta;
	unsigned int hotplug_check_needed = 0;


	if (!hotplug_ready || thermal_suspended) {
		pr_err("frequency control not ready!\n");		
		return -EINVAL;
	}

	delta = (msm_thermal_info.limit_temp_degC - 
			 msm_thermal_info.temp_hysteresis_degC);

	get_online_cpus();
	for_each_possible_cpu(cpu) {
		if (cpu_out_of_range(cpu))
			break;
		resolve_max_freq[cpu] = local_max_freq_thermal[cpu];
		freq_temp = evaluate_temp(cpu);
		if (freq_temp <= 0) {
			hotplug_check_needed++;
			continue;
		}
		if (freq_temp >= msm_thermal_info.limit_temp_degC) {
				if (limit_idx[cpu] == thermal_limit_low[cpu]) {
					hotplug_check_needed++;
					continue;
				}
				limit_idx[cpu] -= msm_thermal_info.freq_step;
				if (limit_idx[cpu] < thermal_limit_low[cpu])
					limit_idx[cpu] = thermal_limit_low[cpu];
				resolve_max_freq[cpu] = table[limit_idx[cpu]].frequency;
				hotplug_check_needed++;
		} else if (freq_temp <= delta) {
				if (limit_idx[cpu] == thermal_limit_high[cpu]) {
					continue;
				}
				limit_idx[cpu] += msm_thermal_info.freq_step;
				if (limit_idx[cpu] >= thermal_limit_high[cpu]) {
					limit_idx[cpu] = thermal_limit_high[cpu];
					resolve_max_freq[cpu] = table[thermal_limit_high[cpu]].frequency;
				} else {
					resolve_max_freq[cpu] = table[limit_idx[cpu]].frequency;
					hotplug_check_needed++;
				}
		}

		if (resolve_max_freq[cpu] == local_max_freq_thermal[cpu])
			continue;

		set_thermal_policy(cpu, resolve_max_freq[cpu]);
		local_max_freq_thermal[cpu] = resolve_max_freq[cpu];
	}
	put_online_cpus();
	return hotplug_check_needed;
}

static void __ref do_core_control(void)
{
	unsigned int cpu = smp_processor_id();
	int ret = 0;
	long delta;
	long core_temp;

	if ((!core_control_enabled) || (intelli_init() ||
		 !hotplug_ready || thermal_suspended)) {
		return;
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
			msm_thermal_info.core_control_mask) {
				if (!(msm_thermal_info.core_control_mask & BIT(cpu)));
					continue;
				if (cpus_offlined & BIT(cpu) && !cpu_online(cpu))
					continue;
				ret = cpu_down(cpu);
				if (ret)
					pr_debug("cpu_down failed. you got problems\n");
				cpus_offlined |= BIT(cpu);
		} else if (core_temp <= delta &&
			msm_thermal_info.core_control_mask && cpus_offlined) {
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
					pr_err("%s: Error %d online core %u\n",
							KBUILD_MODNAME, ret, cpu);
				cpus_offlined &= ~BIT(cpu);
		}
	}
	mutex_unlock(&core_control_mutex);
}

static void __ref check_temp(struct work_struct *work)
{
	int ret = 0;

	if (thermal_suspended)
		return;

	ret = do_freq_control();
	if (ret <= 0)
		goto reschedule;
	else if (ret > 0)
		do_core_control();

reschedule:
	if (thermal_suspended)
		return;

	if (likely(enabled))
		mod_delayed_work(intellithermal_wq, &check_temp_work,
				msecs_to_jiffies(msm_thermal_info.poll_ms));
}

static void __ref get_table(struct work_struct *work)
{
	int ret;
	ret = msm_thermal_get_freq_table();
	if (ret)
		goto reschedule;
	else {
		queue_delayed_work(intellithermal_wq, &check_temp_work, 0);
		return;
	}	

reschedule:
		schedule_work(&get_table_work);
}

static int __ref msm_thermal_cpu_callback(struct notifier_block *nfb,
		unsigned long action, void *hcpu)
{
	unsigned int cpu = (unsigned long)hcpu;
 
	if (thermal_suspended || !hotplug_ready)
		return NOTIFY_OK;
 
	if (action == CPU_UP_PREPARE || action == CPU_UP_PREPARE_FROZEN) {
		if (thermal_core_controlled(cpu)) {
			pr_debug(
			"%s: Preventing cpu%u from coming online.\n",
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
	unsigned int cpu = smp_processor_id();

	cancel_delayed_work_sync(&check_temp_work);
	destroy_workqueue(intellithermal_wq);

	get_online_cpus();
	for_each_possible_cpu(cpu) {
		if (cpu_out_of_range(cpu))
			break;
		if (local_max_freq_thermal[cpu] == table[thermal_limit_high[cpu]].frequency)
			continue;
		else
			local_max_freq_thermal[cpu] = table[thermal_limit_high[cpu]].frequency;
		set_thermal_policy(cpu, local_max_freq_thermal[cpu]);
	}
	put_online_cpus();
}

static int __ref set_enabled(const char *val, const struct kernel_param *kp)
{
	int ret = 0;

	if (*val == '0' || *val == 'n' || *val == 'N') {
		enabled = 0;
		disable_msm_thermal();
		pr_debug("msm_thermal: disabling...\n");
	} else {
		if (!enabled) {
			enabled = 1;
			intellithermal_wq = create_hipri_workqueue("intellithermal");
			INIT_WORK(&get_table_work, get_table);
			INIT_DELAYED_WORK(&check_temp_work, check_temp);
			schedule_work(&get_table_work);
			pr_debug("msm_thermal: rescheduling...\n");
		} else
			pr_debug("msm_thermal: already running...\n");
	}
	pr_debug("%s: enabled = %d\n", KBUILD_MODNAME, enabled);
	ret = param_set_bool(val, kp);

	return ret;
}

static struct kernel_param_ops module_ops = {
	.set = set_enabled,
	.get = param_get_bool,
};

module_param_cb(enabled, &module_ops, &enabled, 0644);
MODULE_PARM_DESC(enabled, "enforce thermal limit on cpu");

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
		if (cpu_out_of_range_hp(cpu))
			break;
		if (!cpu_online(cpu))
			continue;
		if (!(cpus_offlined & BIT(cpu)))
			continue;
		ret = cpu_down(cpu);
		if (ret)
			pr_err("%s: Unable to offline cpu%u\n",
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
		update_offline_cores(cpus_offlined);
	} else {
		pr_debug("%s: Core control disabled\n", KBUILD_MODNAME);
	}

done_store_cc:
	mutex_unlock(&core_control_mutex);
	return count;
}

static ssize_t show_cpus_offlined(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%u\n", cpus_offlined);
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
		mod_delayed_work(intellithermal_wq, &check_temp_work, 0);
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
	INIT_WORK(&get_table_work, get_table);
	INIT_DELAYED_WORK(&check_temp_work, check_temp);
	register_pm_notifier(&msm_thermal_pm_notifier);
	schedule_work(&get_table_work);
	return 0;
}

int __init msm_thermal_late_init(void)
{
	if (num_possible_cpus() > 1)
		msm_thermal_add_cc_nodes();

	return 0;
}
static void msm_thermal_exit(void)
{
	if (core_control_enabled)
		core_control_enabled = false;
	enabled = 0;
	disable_msm_thermal();
	unregister_pm_notifier(&msm_thermal_pm_notifier);
	mutex_destroy(&core_control_mutex);
}

late_initcall(msm_thermal_late_init);
module_exit(msm_thermal_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Praveen Chidambaram <pchidamb@codeaurora.org>");
MODULE_AUTHOR("Paul Reioux <reioux@gmail.com>");
MODULE_AUTHOR("Rob Patershuk <robpatershuk@gmail.com>");
MODULE_DESCRIPTION("intelligent thermal driver for Qualcomm based SOCs");
MODULE_DESCRIPTION("originally from Qualcomm's open source repo");
