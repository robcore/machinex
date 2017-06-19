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
#include "../../../arch/arm/mach-msm/acpuclock.h"

#define DEFAULT_POLLING_MS	200

extern bool hotplug_ready;
static int limit_init;
static int enabled;
bool thermal_core_controlled;
static struct msm_thermal_data msm_thermal_info = {
	.sensor_id_one = 7,
	.sensor_id_two = 8,
	.sensor_id_three = 9,
	.sensor_id_four = 10,
	.poll_ms = DEFAULT_POLLING_MS,
	.limit_temp_degC = 65,
	.temp_hysteresis_degC = 8,
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
static int limit_idx_low;
static int limit_idx_high;
static struct cpufreq_frequency_table *table;
static uint32_t hist_index = 0;
static int cpu0;
static int cpu1;
static int cpu2;
static int cpu3;
static long cpu_thermal_one;
static long cpu_thermal_two;
static long cpu_thermal_three;
static long cpu_thermal_four;

/*static struct thermal_policy thermal_policy = {
	.cpu0,
	.cpu1,
	.cpu2,
	.cpu3,
};
*/
static bool thermal_suspended;
/* module parameters */
module_param_named(poll_ms, msm_thermal_info.poll_ms, uint, 0664);
module_param_named(limit_temp_degC, msm_thermal_info.limit_temp_degC,
			int, 0664);
module_param_named(core_limit_temp_degC, msm_thermal_info.core_limit_temp_degC,
			int, 0664);
module_param_named(core_control_mask, msm_thermal_info.core_control_mask,
			uint, 0664);
module_param_named(freq_limit_hysteresis, msm_thermal_info.temp_hysteresis_degC,
			 int, 0644);
module_param_named(core_limit_hysteresis, msm_thermal_info.core_temp_hysteresis_degC,
			 int, 0644);

static bool therm_freq_limited;
static bool hotplug_check_needed_two;
static bool hotplug_check_needed_three;
static bool hotplug_check_needed_four;
static unsigned int thermal_max_hardlimit = 1350000; /*same as default touchboost*/
module_param(thermal_max_hardlimit, uint, 0644);
static unsigned int thermal_min_hardlimit = 810000; /*best mitigation limit*/
module_param(thermal_min_hardlimit, uint, 0644);
static bool looping = false;

static void get_cpu_temp(void)
{
	struct tsens_device tsens_dev;
	long tempone;
	long temptwo;
	long tempthree;
	long tempfour;
	int ret = 0;

	do {
		tsens_dev.sensor_num = 7;
		ret = tsens_get_temp(&tsens_dev, &tempone);
		if (ret) {
			pr_err("%s: Unable to read TSENS sensor %d\n",
					KBUILD_MODNAME, tsens_dev.sensor_num);
		} else {
			cpu_thermal_one = tempone;
		}
		tsens_dev.sensor_num = 8;
		ret = tsens_get_temp(&tsens_dev, &temptwo);
		if (ret) {
			pr_err("%s: Unable to read TSENS sensor %d\n",
					KBUILD_MODNAME, tsens_dev.sensor_num);
		} else {
			cpu_thermal_two = temptwo;
		}

		tsens_dev.sensor_num = 9;
		ret = tsens_get_temp(&tsens_dev, &tempthree);
		if (ret) {
			pr_err("%s: Unable to read TSENS sensor %d\n",
					KBUILD_MODNAME, tsens_dev.sensor_num);
		} else {
			cpu_thermal_three = tempthree;
		}

		tsens_dev.sensor_num = 10;
		ret = tsens_get_temp(&tsens_dev, &tempfour);
		if (ret) {
			pr_err("%s: Unable to read TSENS sensor %d\n",
					KBUILD_MODNAME, tsens_dev.sensor_num);
		} else {
			cpu_thermal_four = tempfour;
		}
		looping = true;
	} while (!thermal_suspended && enabled);

	looping = false;
}

static void __ref do_freq_control(unsigned int cpu)
{
	int ret = 0;
	struct cpufreq_policy *policy;

	policy = cpufreq_cpu_get_raw(cpu);

	if (policy == NULL || !hotplug_ready ||
		thermal_suspended) {
		hotplug_check_needed_two = false;
		hotplug_check_needed_three = false;
		hotplug_check_needed_four = false;
		pr_err("frequency control not ready!\n");		
		return;
	}

	if (!looping)
		get_cpu_temp();

	if (cpu == 0) {
		ret = cpufreq_get_policy(policy, cpu);
		if (ret)
			return;
		if (cpu_thermal_one >= msm_thermal_info.limit_temp_degC) {
			policy->limited_max_freq_thermal = thermal_min_hardlimit;
		} else if (cpu_thermal_one < (msm_thermal_info.limit_temp_degC -
			msm_thermal_info.temp_hysteresis_degC)) {
			if (policy->limited_max_freq_thermal == thermal_min_hardlimit) {
				policy->limited_max_freq_thermal = thermal_max_hardlimit;
			} else {
				policy->limited_max_freq_thermal = policy->hlimit_max_screen_on;
			}
		}
		reapply_hard_limits(cpu);
		cpufreq_update_policy(cpu);
	} else if (cpu == 1) {
		if (!hotplug_ready || thermal_suspended) {
			hotplug_check_needed_two = false;
			hotplug_check_needed_three = false;
			hotplug_check_needed_four = false;
			return;
		}

		ret = cpufreq_get_policy(policy, cpu);
		if (ret)
			return;

		if (cpu_thermal_two >= msm_thermal_info.limit_temp_degC) {
			policy->limited_max_freq_thermal = thermal_min_hardlimit;
			hotplug_check_needed_two = true;
		} else if (cpu_thermal_two < (msm_thermal_info.limit_temp_degC -
			msm_thermal_info.temp_hysteresis_degC)) {
			if (policy->limited_max_freq_thermal == thermal_min_hardlimit) {
				policy->limited_max_freq_thermal = thermal_max_hardlimit;
			} else {
				policy->limited_max_freq_thermal = policy->hlimit_max_screen_on;
				hotplug_check_needed_two = false;
			}
		}
		
		reapply_hard_limits(cpu);
		cpufreq_update_policy(cpu);
	} else if (cpu == 2) {
		if (!hotplug_ready || thermal_suspended) {
			hotplug_check_needed_two = false;
			hotplug_check_needed_three = false;
			hotplug_check_needed_four = false;
			return;
		}

		ret = cpufreq_get_policy(policy, cpu);
		if (ret)
			return;
		policy->limited_max_freq_thermal = policy->limited_max_freq_thermal;
		if (cpu_thermal_three >= msm_thermal_info.limit_temp_degC) {
			policy->limited_max_freq_thermal = thermal_min_hardlimit;
			hotplug_check_needed_three = true;
		} else if (cpu_thermal_three < (msm_thermal_info.limit_temp_degC -
			msm_thermal_info.temp_hysteresis_degC)) {
			if (policy->limited_max_freq_thermal == thermal_min_hardlimit) {
				policy->limited_max_freq_thermal = thermal_max_hardlimit;
			} else {
				policy->limited_max_freq_thermal = policy->hlimit_max_screen_on;
				hotplug_check_needed_three = false;
			}
		}
		reapply_hard_limits(cpu);
		cpufreq_update_policy(cpu);
	} else if (cpu == 3) {
		if (!hotplug_ready || thermal_suspended) {
			hotplug_check_needed_two = false;
			hotplug_check_needed_three = false;
			hotplug_check_needed_four = false;
			return;
		}

		ret = cpufreq_get_policy(policy, cpu);
		if (ret)
			return;

		if (cpu_thermal_four >= msm_thermal_info.limit_temp_degC) {
			policy->limited_max_freq_thermal = thermal_min_hardlimit;
			hotplug_check_needed_four = true;
		} else if (cpu_thermal_four < (msm_thermal_info.limit_temp_degC -
			msm_thermal_info.temp_hysteresis_degC)) {
			if (policy->limited_max_freq_thermal == thermal_min_hardlimit) {
				policy->limited_max_freq_thermal = thermal_max_hardlimit;
			} else {
				policy->limited_max_freq_thermal = policy->hlimit_max_screen_on;
				hotplug_check_needed_four = false;
			}
		}
		reapply_hard_limits(cpu);
		cpufreq_update_policy(cpu);
	}
}

static void __ref do_core_control(unsigned int cpu)
{
	int ret = 0;

	if ((!core_control_enabled) || (intelli_init() ||
		 !hotplug_ready || thermal_suspended)) {
		thermal_core_controlled = false;
		return;
	}

	if (!looping)
		get_cpu_temp();

	switch (cpu) {
		case 0:
	return;

		case 1:
		mutex_lock(&core_control_mutex);
		if (msm_thermal_info.core_control_mask && 
		   (cpu_thermal_two >= msm_thermal_info.core_limit_temp_degC)) {
			if (!(msm_thermal_info.core_control_mask & BIT(cpu)))
				break;
			if (cpus_offlined & BIT(cpu) && !cpu_online(cpu))
				break;
			ret = cpu_down(cpu);
			if (ret) {
				thermal_core_controlled = false;
			} else {
				thermal_core_controlled = true;
			}
			cpus_offlined |= BIT(cpu);
			break;
		} else if (cpu_thermal_two <= (msm_thermal_info.core_limit_temp_degC -
			msm_thermal_info.core_temp_hysteresis_degC)) {
			if (!(cpus_offlined & BIT(cpu)))
				break;
			cpus_offlined &= ~BIT(cpu);
			/* If this core is already online, then bring up the
			 * next offlined core.
			 */
			if (cpu_online(cpu))
				break;
			ret = cpu_up(cpu);
			if (ret) {
				thermal_core_controlled = true;
				pr_err("%s: Error %d online core %d\n",
						KBUILD_MODNAME, ret, cpu);
			} else {
				thermal_core_controlled = false;
				pr_debug("%s: Success %d online core %d\n",
						KBUILD_MODNAME, ret, cpu);
			}
			break;
		}
		case 2:
		mutex_lock(&core_control_mutex);
		if (msm_thermal_info.core_control_mask && 
		   (cpu_thermal_three >= msm_thermal_info.core_limit_temp_degC)) {
			if (!(msm_thermal_info.core_control_mask & BIT(cpu)))
				break;
			if (cpus_offlined & BIT(cpu) && !cpu_online(cpu))
				break;
			ret = cpu_down(cpu);
			if (ret) {
				thermal_core_controlled = false;
			} else {
				thermal_core_controlled = true;
			}
			cpus_offlined |= BIT(cpu);
			break;
		} else if (cpu_thermal_three <= (msm_thermal_info.core_limit_temp_degC -
			msm_thermal_info.core_temp_hysteresis_degC)) {
			if (!(cpus_offlined & BIT(cpu)))
				break;
			cpus_offlined &= ~BIT(cpu);
			/* If this core is already online, then bring up the
			 * next offlined core.
			 */
			if (cpu_online(cpu))
				break;
			ret = cpu_up(cpu);
			if (ret) {
				thermal_core_controlled = true;
				pr_err("%s: Error %d online core %d\n",
						KBUILD_MODNAME, ret, cpu);
			} else {
				thermal_core_controlled = false;
				pr_debug("%s: Success %d online core %d\n",
						KBUILD_MODNAME, ret, cpu);
			}
			break;
		}
		case 3:
		mutex_lock(&core_control_mutex);
		if (msm_thermal_info.core_control_mask && 
		   (cpu_thermal_four >= msm_thermal_info.core_limit_temp_degC)) {
			if (!(msm_thermal_info.core_control_mask & BIT(cpu)))
				break;
			if (cpus_offlined & BIT(cpu) && !cpu_online(cpu))
				break;
			ret = cpu_down(cpu);
			if (ret) {
				thermal_core_controlled = false;
			} else {
				thermal_core_controlled = true;
			}
			cpus_offlined |= BIT(cpu);
			break;
		} else if (cpu_thermal_four <= (msm_thermal_info.core_limit_temp_degC -
			msm_thermal_info.core_temp_hysteresis_degC)) {
			if (!(cpus_offlined & BIT(cpu)))
				break;
			cpus_offlined &= ~BIT(cpu);
			/* If this core is already online, then bring up the
			 * next offlined core.
			 */
			if (cpu_online(cpu))
				break;
			ret = cpu_up(cpu);
			if (ret) {
				thermal_core_controlled = true;
				pr_err("%s: Error %d online core %d\n",
						KBUILD_MODNAME, ret, cpu);
			} else {
				thermal_core_controlled = false;
				pr_debug("%s: Success %d online core %d\n",
						KBUILD_MODNAME, ret, cpu);
			}
			break;
		}
	}

	mutex_unlock(&core_control_mutex);
}

static void __ref check_temp(struct work_struct *work)
{
	int ret = 0;

	if (thermal_suspended)
		return;

	if (!hotplug_ready)
		goto reschedule;

	if (!looping)
		get_cpu_temp();

	do_freq_control(cpu0);
	do_freq_control(cpu1);
	do_freq_control(cpu2);
	do_freq_control(cpu3);
	if (hotplug_check_needed_two)
		do_core_control(cpu1);
	if (hotplug_check_needed_three)
		do_core_control(cpu2);
	if (hotplug_check_needed_four)
		do_core_control(cpu3);
reschedule:
	if (enabled && !thermal_suspended)
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
			thermal_core_controlled = true;
			pr_debug(
			"%s: Preventing cpu%d from coming online.\n",
				KBUILD_MODNAME, cpu);
			return NOTIFY_BAD;
		}
	}

	thermal_core_controlled = false;

	return NOTIFY_OK;
}

static struct notifier_block __refdata msm_thermal_cpu_notifier = {
	.notifier_call = msm_thermal_cpu_callback,
};

static int msm_thermal_pm_event(struct notifier_block *this,
				unsigned long event, void *ptr)
{
	unsigned int cpu = smp_processor_id();
	switch (event) {
	case PM_POST_HIBERNATION:
	case PM_POST_SUSPEND:
		thermal_suspended = false;
		get_cpu_temp();
		mod_delayed_work_on(0, intellithermal_wq, &check_temp_work, 0);
		break;
	case PM_HIBERNATION_PREPARE:
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

/**
 * We will reset the cpu frequencies limits here. The core online/offline
 * status will be carried over to the process stopping the msm_thermal, as
 * we dont want to online a core and bring in the thermal issues.
 */
static void __ref disable_msm_thermal(void)
{
	unsigned int cpu = smp_processor_id();
	struct cpufreq_policy *policy;

	cancel_delayed_work_sync(&check_temp_work);
	destroy_workqueue(intellithermal_wq);

		policy = cpufreq_cpu_get_raw(cpu);
			if (!policy)
				return;

	for_each_possible_cpu(cpu) {
		policy->limited_max_freq_thermal = policy->hlimit_max_screen_on;
		reapply_hard_limits(cpu);
		cpufreq_update_policy(cpu);
	}	
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
			if (!looping)
				get_cpu_temp();
			queue_delayed_work_on(0, intellithermal_wq,
					   &check_temp_work, 0);
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
static int __ref update_offline_cores(int val)
{
	unsigned int cpu = smp_processor_id();
	int ret = 0;

	cpus_offlined = msm_thermal_info.core_control_mask & val;
	if (!core_control_enabled || intelli_init() ||
		thermal_suspended || !hotplug_ready)
		return ret;

	for_each_possible_cpu(cpu) {
		if (!(cpus_offlined & BIT(cpu)))
			continue;
		if (!cpu_online(cpu))
			continue;
		ret = cpu_down(cpu);
		if (ret) {
			thermal_core_controlled = false;
			pr_err("%s: Unable to offline cpu%d\n",
				KBUILD_MODNAME, cpu);
		} else {
			thermal_core_controlled = true;
			pr_debug("%s: Thermal Offlined CPU%d\n",
				KBUILD_MODNAME, cpu);
		}
	}
	return ret;
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
		if (!looping)
			get_cpu_temp();
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

int __init msm_thermal_init(struct msm_thermal_data *pdata)
{
	BUG_ON(!pdata);
	memcpy(&msm_thermal_info, pdata, sizeof(struct msm_thermal_data));

	cpu0 = cpumask_first(cpu_possible_mask);
	cpu1 = cpumask_next(0, cpu_possible_mask);
	cpu2 = cpumask_next(1, cpu_possible_mask);
	cpu3 = cpumask_next(2, cpu_possible_mask);
	pr_info("%s: Registered CPUs %u, %u, %u, %u\n", KBUILD_MODNAME, cpu0, cpu1, cpu2, cpu3);

	enabled = 1;
	if ((num_possible_cpus() > 1) && (core_control_enabled == true))
		register_cpu_notifier(&msm_thermal_cpu_notifier);

	register_pm_notifier(&msm_thermal_pm_notifier);

	mutex_init(&core_control_mutex);

	intellithermal_wq = create_hipri_workqueue("intellithermal");
	INIT_DELAYED_WORK(&check_temp_work, check_temp);
	queue_delayed_work_on(0, intellithermal_wq, &check_temp_work, 0);

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
	unregister_cpu_notifier(&msm_thermal_cpu_notifier);
	unregister_pm_notifier(&msm_thermal_pm_notifier);
	mutex_destroy(&core_control_mutex);
}

late_initcall(msm_thermal_late_init);
module_exit(msm_thermal_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Praveen Chidambaram <pchidamb@codeaurora.org>");
MODULE_AUTHOR("Paul Reioux <reioux@gmail.com>");
MODULE_DESCRIPTION("intelligent thermal driver for Qualcomm based SOCs");
MODULE_DESCRIPTION("originally from Qualcomm's open source repo");
