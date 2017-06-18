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

static int enabled;
bool thermal_core_controlled;
static struct msm_thermal_data msm_thermal_info = {
	.sensor_id_one = 7,
	.sensor_id_two = 8,
	.sensor_id_three = 9,
	.sensor_id_four = 10,
	.poll_ms = DEFAULT_POLLING_MS,
	.limit_temp_degC = 65,
	.temp_hysteresis_degC = 10,
	.freq_step = 3,
	.freq_control_mask = 0xf,
	.core_limit_temp_degC = 75,
	.core_temp_hysteresis_degC = 10,
	.core_control_mask = 0xe,
};
unsigned long limited_max_freq_thermal = CPUFREQ_HARDLIMIT_MAX_SCREEN_ON_STOCK;
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
static unsigned int cpu0 = 0;
static unsigned int cpu1 = 1;
static unsigned int cpu2 = 2;
static unsigned int cpu3 = 3;
static long cpu_thermal_one;
static long cpu_thermal_two;
static long cpu_thermal_three;
static long cpu_thermal_four;

/* module parameters */
module_param_named(poll_ms, msm_thermal_info.poll_ms, uint, 0664);
module_param_named(limit_temp_degC, msm_thermal_info.limit_temp_degC,
			int, 0664);
module_param_named(core_limit_temp_degC, msm_thermal_info.core_limit_temp_degC,
			int, 0664);
module_param_named(core_control_mask, msm_thermal_info.core_control_mask,
			uint, 0664);
module_param_named(freq_step, msm_thermal_info.freq_step, uint, 0644);
module_param_named(thermal_limit_high, limit_idx_high, int, 0644);
module_param_named(thermal_limit_low, limit_idx_low, int, 0644);
module_param_named(freq_limit_hysteresis, msm_thermal_info.temp_hysteresis_degC,
			 int, 0644);
module_param_named(core_limit_hysteresis, msm_thermal_info.core_temp_hysteresis_degC,
			 int, 0644);

static bool thermal_suspended;
static bool therm_freq_limited;
static bool hotplug_check_needed;
static int msm_thermal_get_freq_table(void)
{
	struct cpufreq_policy *policy;
	int ret = 0;
	int i = 0;

	policy = cpufreq_cpu_get_raw(0);

	if (policy == NULL || thermal_suspended) {
		ret = -EINVAL;
		goto fail;
	}

	table = policy->freq_table;
	if (table == NULL) {
		pr_debug("%s: error reading cpufreq table\n", KBUILD_MODNAME);
		ret = -EINVAL;
		goto fail;
	}

	while (table[i].frequency != CPUFREQ_TABLE_END)
		i++;

	limit_idx_low = 4;
	limit_idx_high = limit_idx = i - 1;
	BUG_ON(limit_idx_high <= 0 || limit_idx_high <= limit_idx_low);
fail:
	return ret;
}

static void update_cpu_max_freq(int cpu, unsigned long max_freq)
{
	struct cpufreq_policy policy;
	int ret;

	if (thermal_suspended)
		return;

	ret = cpufreq_get_policy(&policy, cpu);
		if (ret)
			return;

	if (!cpu_online(cpu) && !thermal_core_controlled) {
		mutex_lock(&core_control_mutex);
		thermal_core_controlled = true;
		ret = cpu_up(cpu);
		if (ret)
			pr_debug("Thermal: Unable to Online Core\n");
		mutex_unlock(&core_control_mutex);
	}

	reapply_hard_limits(cpu);
	limited_max_freq_thermal = max_freq;
	cpufreq_update_policy(cpu);
	thermal_core_controlled = false;
	return;
}

static int get_cpu_temp(unsigned int cpu)
{
	struct tsens_device tsens_dev_one, tsens_dev_two, tsens_dev_three, tsens_dev_four;
	long temp_one = 0;
	long temp_two = 0;
	long temp_three = 0;
	long temp_four = 0;
	int ret = 0;

	if (thermal_suspended)
		return -EINVAL;

	switch (cpu) {
	case 0:
		tsens_dev_one.sensor_num = msm_thermal_info.sensor_id_one;
		ret = tsens_get_temp(&tsens_dev_one, &temp_one);
		cpu_thermal_one = temp_one;
		if (!temp_one) {
			pr_debug("%s: Unable to read TSENS sensor %d\n",
					KBUILD_MODNAME, tsens_dev_one.sensor_num);
			return ret;
		}
		break;
	case 1:
		tsens_dev_two.sensor_num = msm_thermal_info.sensor_id_two;
		ret = tsens_get_temp(&tsens_dev_two, &temp_two);
		cpu_thermal_two = temp_two;
		if (!temp_two) {
			pr_debug("%s: Unable to read TSENS sensor %d\n",
					KBUILD_MODNAME, tsens_dev_two.sensor_num);
			return ret;
		}
		break;
	case 2:
		tsens_dev_three.sensor_num = msm_thermal_info.sensor_id_three;
		ret = tsens_get_temp(&tsens_dev_three, &temp_three);
		cpu_thermal_three = temp_three;
		if (!temp_three) {
			pr_debug("%s: Unable to read TSENS sensor %d\n",
					KBUILD_MODNAME, tsens_dev_three.sensor_num);
			return ret;
		}
		break;
	case 3:
		tsens_dev_four.sensor_num = msm_thermal_info.sensor_id_three;
		ret = tsens_get_temp(&tsens_dev_four, &temp_four);
		cpu_thermal_four = temp_four;
		if (!temp_four) {
			pr_debug("%s: Unable to read TSENS sensor %d\n",
					KBUILD_MODNAME, tsens_dev_four.sensor_num);
			return ret;
		}
		break;
	default:
		break;
	}
		return 0;
}

static void __ref do_core_control(unsigned int cpu)
{
	int cpu = 0;
	int ret = 0;

	if ((!core_control_enabled) || (intelli_init() ||
		 !hotplug_ready || thermal_suspended)) {
		thermal_core_controlled = false;
		return;
	}

	switch (cpu) {
	case 0:
	get_cpu_temp(cpu);
	mutex_lock(&core_control_mutex);
		if (msm_thermal_info.core_control_mask && 
		   (cpu_thermal_one >= msm_thermal_info.core_limit_temp_degC)) {
			if (!(msm_thermal_info.core_control_mask & BIT(cpu)))
				goto complete;
			if (cpus_offlined & BIT(cpu) && !cpu_online(cpu))
				goto complete;
			ret = cpu_down(cpu);
			if (ret) {
				thermal_core_controlled = false;
			} else {
				thermal_core_controlled = true;
			}
			cpus_offlined |= BIT(cpu);
			goto complete;
		} else if (cpu_thermal_one <= (msm_thermal_info.core_limit_temp_degC -
			msm_thermal_info.core_temp_hysteresis_degC)) {
			if (!(cpus_offlined & BIT(cpu)))
				goto complete;
			cpus_offlined &= ~BIT(cpu);
			/* If this core is already online, then bring up the
			 * next offlined core.
			 */
			if (cpu_online(cpu))
				goto complete;
			ret = cpu_up(cpu);
			if (ret) {
				thermal_core_controlled = true;
				pr_err("%s: Error %d online core %d\n",
						KBUILD_MODNAME, ret, i);
			} else {
				thermal_core_controlled = false;
				pr_debug("%s: Success %d online core %d\n",
						KBUILD_MODNAME, ret, i);
			}
			goto complete;
		}
	case 1:
	get_cpu_temp(cpu);
	mutex_lock(&core_control_mutex);
		if (msm_thermal_info.core_control_mask && 
		   (cpu_thermal_two >= msm_thermal_info.core_limit_temp_degC)) {
			if (!(msm_thermal_info.core_control_mask & BIT(cpu)))
				goto complete;
			if (cpus_offlined & BIT(cpu) && !cpu_online(cpu))
				goto complete;
			ret = cpu_down(cpu);
			if (ret) {
				thermal_core_controlled = false;
			} else {
				thermal_core_controlled = true;
			}
			cpus_offlined |= BIT(cpu);
			goto complete;
		} else if (cpu_thermal_two <= (msm_thermal_info.core_limit_temp_degC -
			msm_thermal_info.core_temp_hysteresis_degC)) {
			if (!(cpus_offlined & BIT(cpu)))
				goto complete;
			cpus_offlined &= ~BIT(cpu);
			/* If this core is already online, then bring up the
			 * next offlined core.
			 */
			if (cpu_online(cpu))
				goto complete;
			ret = cpu_up(cpu);
			if (ret) {
				thermal_core_controlled = true;
				pr_err("%s: Error %d online core %d\n",
						KBUILD_MODNAME, ret, i);
			} else {
				thermal_core_controlled = false;
				pr_debug("%s: Success %d online core %d\n",
						KBUILD_MODNAME, ret, i);
			}
			goto complete;
		}
	case 2:
	get_cpu_temp(cpu);
	mutex_lock(&core_control_mutex);
		if (msm_thermal_info.core_control_mask && 
		   (cpu_thermal_three >= msm_thermal_info.core_limit_temp_degC)) {
			if (!(msm_thermal_info.core_control_mask & BIT(cpu)))
				goto complete;
			if (cpus_offlined & BIT(cpu) && !cpu_online(cpu))
				goto complete;
			ret = cpu_down(cpu);
			if (ret) {
				thermal_core_controlled = false;
			} else {
				thermal_core_controlled = true;
			}
			cpus_offlined |= BIT(cpu);
			goto complete;
		} else if (cpu_thermal_three <= (msm_thermal_info.core_limit_temp_degC -
			msm_thermal_info.core_temp_hysteresis_degC)) {
			if (!(cpus_offlined & BIT(cpu)))
				goto complete;
			cpus_offlined &= ~BIT(cpu);
			/* If this core is already online, then bring up the
			 * next offlined core.
			 */
			if (cpu_online(cpu))
				goto complete;
			ret = cpu_up(cpu);
			if (ret) {
				thermal_core_controlled = true;
				pr_err("%s: Error %d online core %d\n",
						KBUILD_MODNAME, ret, i);
			} else {
				thermal_core_controlled = false;
				pr_debug("%s: Success %d online core %d\n",
						KBUILD_MODNAME, ret, i);
			}
			goto complete;
		}
	case 3:
	get_cpu_temp(cpu);
	mutex_lock(&core_control_mutex);
		if (msm_thermal_info.core_control_mask && 
		   (cpu_thermal_four >= msm_thermal_info.core_limit_temp_degC)) {
			if (!(msm_thermal_info.core_control_mask & BIT(cpu)))
				goto complete;
			if (cpus_offlined & BIT(cpu) && !cpu_online(cpu))
				goto complete;
			ret = cpu_down(cpu);
			if (ret) {
				thermal_core_controlled = false;
			} else {
				thermal_core_controlled = true;
			}
			cpus_offlined |= BIT(cpu);
			goto complete;
		} else if (cpu_thermal_four <= (msm_thermal_info.core_limit_temp_degC -
			msm_thermal_info.core_temp_hysteresis_degC)) {
			if (!(cpus_offlined & BIT(cpu)))
				goto complete;
			cpus_offlined &= ~BIT(cpu);
			/* If this core is already online, then bring up the
			 * next offlined core.
			 */
			if (cpu_online(cpu))
				goto complete;
			ret = cpu_up(cpu);
			if (ret) {
				thermal_core_controlled = true;
				pr_err("%s: Error %d online core %d\n",
						KBUILD_MODNAME, ret, i);
			} else {
				thermal_core_controlled = false;
				pr_debug("%s: Success %d online core %d\n",
						KBUILD_MODNAME, ret, i);
			}
			goto complete;
		}
	}
complete:
	mutex_unlock(&core_control_mutex);
	return;
}

static void __ref do_freq_control(unsigned int cpu)
{
	int ret = 0;
	struct cpufreq_policy policy;
	unsigned long max_freq = limited_max_freq_thermal;

	if (!hotplug_ready || thermal_suspended) {
		hotplug_check_needed = false;
		return;
	}
	ret = cpufreq_get_policy(&policy, cpu);
		if (ret)
			return;

	switch (cpu) {
		case 0:
		get_cpu_temp(cpu);
		if (cpu_thermal_one >= msm_thermal_info.limit_temp_degC) {
			if (limit_idx == limit_idx_low) {
				hotplug_check_needed = false;
				return;
			}

			limit_idx -= msm_thermal_info.freq_step;
			if (limit_idx < limit_idx_low)
				limit_idx = limit_idx_low;
			max_freq = table[limit_idx].frequency;
			hotplug_check_needed = true;
		} else if (cpu_thermal_one < msm_thermal_info.limit_temp_degC -
		msm_thermal_info.temp_hysteresis_degC) {
			if (limit_idx == limit_idx_high) {
				hotplug_check_needed = false;
				return;
			}

			limit_idx += msm_thermal_info.freq_step;
			if (limit_idx >= limit_idx_high) {
				limit_idx = limit_idx_high;
				max_freq = policy.hlimit_max_screen_on;
				hotplug_check_needed = false;
			} else
				max_freq = table[limit_idx].frequency;
				hotplug_check_needed = true;
		}

		if (max_freq == limited_max_freq_thermal) {
			hotplug_check_needed = false;
			return;
		}
		update_cpu_max_freq(cpu, max_freq);
		return;

		case 1:
		get_cpu_temp(cpu);

		if (cpu_thermal_two >= msm_thermal_info.limit_temp_degC) {
			if (limit_idx == limit_idx_low) {
				hotplug_check_needed = false;
				return;
			}

			limit_idx -= msm_thermal_info.freq_step;
			if (limit_idx < limit_idx_low)
				limit_idx = limit_idx_low;
			max_freq = table[limit_idx].frequency;
			hotplug_check_needed = true;
		} else if (cpu_thermal_two < msm_thermal_info.limit_temp_degC -
		msm_thermal_info.temp_hysteresis_degC) {
			if (limit_idx == limit_idx_high) {
				hotplug_check_needed = false;
				return;
			}

			limit_idx += msm_thermal_info.freq_step;
			if (limit_idx >= limit_idx_high) {
				limit_idx = limit_idx_high;
				max_freq = policy.hlimit_max_screen_on;
				hotplug_check_needed = false;
			} else
				max_freq = table[limit_idx].frequency;
				hotplug_check_needed = true;
		}

		if (max_freq == limited_max_freq_thermal) {
			hotplug_check_needed = false;
			return;
		}
		update_cpu_max_freq(cpu, max_freq);
		return;

		case 2:
		get_cpu_temp(cpu);

		if (cpu_thermal_three >= msm_thermal_info.limit_temp_degC) {
			if (limit_idx == limit_idx_low) {
				hotplug_check_needed = false;
				return;
			}

			limit_idx -= msm_thermal_info.freq_step;
			if (limit_idx < limit_idx_low)
				limit_idx = limit_idx_low;
			max_freq = table[limit_idx].frequency;
			hotplug_check_needed = true;
		} else if (cpu_thermal_three < msm_thermal_info.limit_temp_degC -
		msm_thermal_info.temp_hysteresis_degC) {
			if (limit_idx == limit_idx_high) {
				hotplug_check_needed = false;
				return;
			}

			limit_idx += msm_thermal_info.freq_step;
			if (limit_idx >= limit_idx_high) {
				limit_idx = limit_idx_high;
				max_freq = policy.hlimit_max_screen_on;
				hotplug_check_needed = false;
			} else
				max_freq = table[limit_idx].frequency;
				hotplug_check_needed = true;
		}

		if (max_freq == limited_max_freq_thermal) {
			hotplug_check_needed = false;
			return;
		}
		update_cpu_max_freq(cpu, max_freq);
		return;

		case 3:
		get_cpu_temp(cpu);

		if (cpu_thermal_four >= msm_thermal_info.limit_temp_degC) {
			if (limit_idx == limit_idx_low) {
				hotplug_check_needed = false;
				return;
			}

			limit_idx -= msm_thermal_info.freq_step;
			if (limit_idx < limit_idx_low)
				limit_idx = limit_idx_low;
			max_freq = table[limit_idx].frequency;
			hotplug_check_needed = true;
		} else if (cpu_thermal_four < msm_thermal_info.limit_temp_degC -
		msm_thermal_info.temp_hysteresis_degC) {
			if (limit_idx == limit_idx_high) {
				hotplug_check_needed = false;
				return;
			}

			limit_idx += msm_thermal_info.freq_step;
			if (limit_idx >= limit_idx_high) {
				limit_idx = limit_idx_high;
				max_freq = policy.hlimit_max_screen_on;
				hotplug_check_needed = false;
			} else
				max_freq = table[limit_idx].frequency;
				hotplug_check_needed = true;
		}

		if (max_freq == limited_max_freq_thermal) {
			hotplug_check_needed = false;
			return;
		}
		update_cpu_max_freq(cpu, max_freq);
		return;

	default:
		return;
	}
}

static void __ref check_temp(struct work_struct *work)
{
	static int limit_init;
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

	if (!thermal_suspended)
		do_freq_control(cpumask_of(cpu0));
	if (!thermal_suspended)
		do_freq_control(cpumask_of(cpu1));
	if (!thermal_suspended)
		do_freq_control(cpumask_of(cpu2));
	if (!thermal_suspended)
		do_freq_control(cpumask_of(cpu3));
	if (hotplug_check_needed || !thermal_suspended)
		do_core_control(cpumask_of(cpu0));
	if (hotplug_check_needed || !thermal_suspended)
		do_core_control(cpumask_of(cpu1));
	if (hotplug_check_needed || !thermal_suspended)
		do_core_control(cpumask_of(cpu2));
	if (hotplug_check_needed || !thermal_suspended)
		do_core_control(cpumask_of(cpu3));
reschedule:
	if (enabled | !thermal_suspended)
		mod_delayed_work_on(0, intellithermal_wq, &check_temp_work,
				msecs_to_jiffies(msm_thermal_info.poll_ms));
}

static int __ref msm_thermal_cpu_callback(struct notifier_block *nfb,
		unsigned long action, void *hcpu)
{
	unsigned int cpu = (unsigned long)hcpu;

	if (thermal_suspended)
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
	switch (event) {
	case PM_POST_HIBERNATION:
	case PM_POST_SUSPEND:
		thermal_suspended = false;
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
	struct cpufreq_policy *policy;
	unsigned int cpu = smp_processor_id();

	policy = cpufreq_cpu_get_raw(cpu);
		if (!policy)
			return;

	cancel_delayed_work_sync(&check_temp_work);
	destroy_workqueue(intellithermal_wq);

	for_each_possible_cpu(cpu) {
		if (limited_max_freq_thermal == policy->hlimit_max_screen_on)
			return;
		else
			(limited_max_freq_thermal = policy->hlimit_max_screen_on);

		update_cpu_max_freq(cpu, limited_max_freq_thermal);
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
	unsigned int cpu = 0;
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
	BUG_ON(pdata->sensor_id_one >= TSENS_MAX_SENSORS ||
	pdata->sensor_id_two >= TSENS_MAX_SENSORS ||
	pdata->sensor_id_three >= TSENS_MAX_SENSORS ||
	pdata->sensor_id_four >= TSENS_MAX_SENSORS);
	memcpy(&msm_thermal_info, pdata, sizeof(*pdata));
	limited_max_freq_thermal = CPUFREQ_HARDLIMIT_MAX_SCREEN_ON_STOCK;

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

