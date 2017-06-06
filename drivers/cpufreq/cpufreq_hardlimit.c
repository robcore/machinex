/*
 * Author: Jean-Pierre Rasquin <yank555.lu@gmail.com>
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

/*
 * CPU freq hard limit - SysFS interface :
 * ---------------------------------------
 *
 * /sys/kernel/cpufreq_hardlimit/scaling_max_freq_screen_on (rw)
 *
 *   set or show the real hard CPU max frequency limit when screen is on
 *
 * /sys/kernel/cpufreq_hardlimit/scaling_max_freq_screen_off (rw)
 *
 *   set or show the real hard CPU max frequency limit when screen is off
 *
 * /sys/kernel/cpufreq_hardlimit/scaling_min_freq_screen_on (rw)
 *
 *   set or show the real hard CPU min frequency limit when screen is on
 *
 * /sys/kernel/cpufreq_hardlimit/scaling_min_freq_screen_off (rw)
 *
 *   set or show the real hard CPU min frequency limit when screen is off
 *
 * /sys/kernel/cpufreq_hardlimit/wakeup_kick_freq (rw)
 *
 *   set or show the wakeup kick frequency (scaling_min for delay time)
 *
 * /sys/kernel/cpufreq_hardlimit/wakeup_kick_delay (rw)
 *
 *   set or show the wakeup kick duration (in ms)
 *
 * /sys/kernel/cpufreq_hardlimit/touchboost_lo_freq (rw)
 *
 *   set or show touchboost low frequency
 *
 * /sys/kernel/cpufreq_hardlimit/touchboost_hi_freq (rw)
 *
 *   set or show touchboost high frequency
 *
 * /sys/kernel/cpufreq_hardlimit/touchboost_delay (rw)
 *
 *   set or show touchboost delay (0 = disabled, up to 10000ms)
 *
 * /sys/kernel/cpufreq_hardlimit/touchboost_eventcount (rw)
 *
 *   set or show touchboost eventcount necessary to go into high frequency (1-10)
 *
 * /sys/kernel/cpufreq_hardlimit/touchinput_dev_name (ro)
 *
 *   display the used touch device name (only if debug set in defconfig)
 *
 * #ifdef CONFIG_SEC_DVFS
 * /sys/kernel/cpufreq_hardlimit/userspace_dvfs_lock (rw)
 *
 *   0 = allow changes to scaling min/max
 *   1 = ignore (don't apply, but don't return an error)
 *   2 = refuse (don't apply, return EINVAL)
 * #endif
 *
 * /sys/kernel/cpufreq_hardlimit/current_limit_max (ro)
 *
 *   display current applied hardlimit for CPU max
 *
 * /sys/kernel/cpufreq_hardlimit/current_limit_min (ro)
 *
 *   display current applied hardlimit for CPU min
 *
 * /sys/kernel/cpufreq_hardlimit/version (ro)
 *
 *   display CPU freq hard limit version information
 *
 */

#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/cpufreq_hardlimit.h>
#include <linux/cpufreq.h>
#include <linux/powersuspend.h>
#include <linux/jiffies.h>
#include <linux/workqueue.h>
#include <linux/module.h>
#include <linux/slab.h>
#ifdef SUPERFLUOUS
#include <linux/input.h>
#endif
#if 0
#include <linux/thermal.h>
#endif

unsigned int hardlimit_max_screen_on  = CPUFREQ_HARDLIMIT_MAX_SCREEN_ON_STOCK;  /* default to stock behaviour */
unsigned int hardlimit_max_screen_off = CPUFREQ_HARDLIMIT_MAX_SCREEN_OFF_STOCK; /* default to stock behaviour */
unsigned int hardlimit_min_screen_on  = CPUFREQ_HARDLIMIT_MIN_SCREEN_ON_STOCK;  /* default to stock behaviour */
unsigned int hardlimit_min_screen_off = CPUFREQ_HARDLIMIT_MIN_SCREEN_OFF_STOCK; /* default to stock behaviour */

unsigned int current_limit_max        = CPUFREQ_HARDLIMIT_MAX_SCREEN_ON_STOCK;
unsigned int current_limit_min        = CPUFREQ_HARDLIMIT_MIN_SCREEN_ON_STOCK;
unsigned int current_screen_state     = CPUFREQ_HARDLIMIT_SCREEN_ON;		/* default to screen on */

struct cpufreq_frequency_table *cpufreq_frequency_get_table(unsigned int cpu)
{
	struct cpufreq_policy *policy;

	for_each_possible_cpu(cpu) {
		policy = cpufreq_cpu_get_raw(cpu);
	}
	if (policy != NULL)
		return policy->freq_table;
	else
		return NULL;
}
EXPORT_SYMBOL_GPL(cpufreq_frequency_get_table);
/* ------------------------------------------------------------------------------ */
/* Externally reachable function                                                  */
/* ------------------------------------------------------------------------------ */

/* Sanitize cpufreq to hardlimits */
unsigned int check_cpufreq_hardlimit(unsigned int freq)
{
#if 0
	thermal_hardlimit = limited_max_freq_thermal;
#endif
// Called way too often, even when debugging
//	#ifdef CONFIG_CPUFREQ_HARDLIMIT_DEBUG
//	pr_info("[HARDLIMIT] check_cpufreq_hardlimit : min = %u / max = %u / freq = %u / result = %u \n",
//			current_limit_min,
//			current_limit_max,
//			freq,
//			max(current_limit_min, min(current_limit_max, freq))
//		);
//	#endif
#if 0
	if (freq_is_therm_limited())
		current_limit_max = thermal_hardlimit;
#endif

	return max(current_limit_min, min(current_limit_max, freq));
}

/* Update limits in cpufreq */
void reapply_hard_limits(void)
{
	/* Recalculate the currently applicable min/max */
	if (current_screen_state == CPUFREQ_HARDLIMIT_SCREEN_ON) {
			current_limit_min  = hardlimit_min_screen_on;
			current_limit_max  = hardlimit_max_screen_on;
	} else {
		current_limit_min  = hardlimit_min_screen_off;
		current_limit_max  = hardlimit_max_screen_off;
	}

	if (limited_max_freq_thermal > current_limit_min && current_limit_max > limited_max_freq_thermal)
		update_scaling_limits(current_limit_min, limited_max_freq_thermal);
	else
		update_scaling_limits(current_limit_min, current_limit_max);
}

/* ------------------------------------------------------------------------------ */
/* Powersuspend callback functions                                                */
/* ------------------------------------------------------------------------------ */

static void cpufreq_hardlimit_suspend(struct power_suspend * h)
{
	current_screen_state = CPUFREQ_HARDLIMIT_SCREEN_OFF;
	reapply_hard_limits();
	return;
}

static void cpufreq_hardlimit_resume(struct power_suspend * h)
{
	current_screen_state = CPUFREQ_HARDLIMIT_SCREEN_ON;
	reapply_hard_limits();
	return;
}

static struct power_suspend cpufreq_hardlimit_suspend_data =
{
	.suspend = cpufreq_hardlimit_suspend,
	.resume = cpufreq_hardlimit_resume,
};

static int cpufreq_hardlimit_policy_notifier(
	struct notifier_block *nb, unsigned long val, void *data)
{
	switch (val) {
		case CPUFREQ_ADJUST:
			reapply_hard_limits();
			break;
		default:
			break;
	}

		return NOTIFY_OK;
}

static struct notifier_block cpufreq_policy_notifier_block = {
	.notifier_call = cpufreq_hardlimit_policy_notifier,
};

/* ------------------------------------------------------------------------------ */
/* sysfs interface functions                                                      */
/* ------------------------------------------------------------------------------ */

/* sysfs interface for "hardlimit_max_screen_on" */
static ssize_t hardlimit_max_screen_on_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", hardlimit_max_screen_on);
}

static ssize_t hardlimit_max_screen_on_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{

	unsigned int new_hardlimit, i;

	struct cpufreq_frequency_table *table;

	if (!sscanf(buf, "%du", &new_hardlimit))
		return -EINVAL;

	if (new_hardlimit == hardlimit_max_screen_on)
		return count;

	table = cpufreq_frequency_get_table(0); /* Get frequency table */

	for (i = 0; (table[i].frequency != CPUFREQ_TABLE_END); i++)
		if (table[i].frequency == new_hardlimit) {
			hardlimit_max_screen_on = new_hardlimit;
			reapply_hard_limits();
			return count;
		}

	return -EINVAL;

}

/* sysfs interface for "hardlimit_max_screen_off" */
static ssize_t hardlimit_max_screen_off_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", hardlimit_max_screen_off);
}

static ssize_t hardlimit_max_screen_off_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{

	unsigned int new_hardlimit, i;

	struct cpufreq_frequency_table *table;

	if (!sscanf(buf, "%du", &new_hardlimit))
		return -EINVAL;

	if (new_hardlimit == hardlimit_max_screen_off)
		return count;

	table = cpufreq_frequency_get_table(0); /* Get frequency table */

	for (i = 0; (table[i].frequency != CPUFREQ_TABLE_END); i++)
		if (table[i].frequency == new_hardlimit) {
			hardlimit_max_screen_off = new_hardlimit;
			reapply_hard_limits();
			return count;
		}

	return -EINVAL;

}

/* sysfs interface for "hardlimit_min_screen_on" */
static ssize_t hardlimit_min_screen_on_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", hardlimit_min_screen_on);
}

static ssize_t hardlimit_min_screen_on_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{

	unsigned int new_hardlimit, i;

	struct cpufreq_frequency_table *table;

	if (!sscanf(buf, "%du", &new_hardlimit))
		return -EINVAL;

	if (new_hardlimit == hardlimit_min_screen_on)
		return count;

	table = cpufreq_frequency_get_table(0); /* Get frequency table */

	for (i = 0; (table[i].frequency != CPUFREQ_TABLE_END); i++)
		if (table[i].frequency == new_hardlimit) {
			hardlimit_min_screen_on = new_hardlimit;
			/* Wakeup kick can never be higher than CPU max. hardlimit */
			reapply_hard_limits();
			return count;
		}

	return -EINVAL;

}

/* sysfs interface for "hardlimit_max_screen_off" */
static ssize_t hardlimit_min_screen_off_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", hardlimit_min_screen_off);
}

static ssize_t hardlimit_min_screen_off_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{

	unsigned int new_hardlimit, i;

	struct cpufreq_frequency_table *table;

	if (!sscanf(buf, "%du", &new_hardlimit))
		return -EINVAL;

	if (new_hardlimit == hardlimit_min_screen_off)
		return count;

	table = cpufreq_frequency_get_table(0); /* Get frequency table */

	for (i = 0; (table[i].frequency != CPUFREQ_TABLE_END); i++)
		if (table[i].frequency == new_hardlimit) {
			hardlimit_min_screen_off = new_hardlimit;
			reapply_hard_limits();
			return count;
		}

	return -EINVAL;

}

/* sysfs interface for "current_limit_min" */
static ssize_t current_limit_min_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", current_limit_min);
}

/* sysfs interface for "current_limit_max" */
static ssize_t current_limit_max_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", current_limit_max);
}

/* sysfs interface for "version" */
static ssize_t version_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", CPUFREQ_HARDLIMIT_VERSION);
}

/* ------------------------------------------------------------------------------ */
/* sysfs interface structure                                                      */
/* ------------------------------------------------------------------------------ */

static struct kobject *hardlimit_kobj;

/* Define sysfs entry attributes */

static struct kobj_attribute hardlimit_max_screen_on_attribute =
__ATTR(scaling_max_freq_screen_on, 0666, hardlimit_max_screen_on_show, hardlimit_max_screen_on_store);

static struct kobj_attribute hardlimit_max_screen_off_attribute =
__ATTR(scaling_max_freq_screen_off, 0666, hardlimit_max_screen_off_show, hardlimit_max_screen_off_store);

static struct kobj_attribute hardlimit_min_screen_on_attribute =
__ATTR(scaling_min_freq_screen_on, 0666, hardlimit_min_screen_on_show, hardlimit_min_screen_on_store);

static struct kobj_attribute hardlimit_min_screen_off_attribute =
__ATTR(scaling_min_freq_screen_off, 0666, hardlimit_min_screen_off_show, hardlimit_min_screen_off_store);

static struct kobj_attribute current_limit_min_attribute =
__ATTR(current_limit_min, 0444, current_limit_min_show, NULL);

static struct kobj_attribute current_limit_max_attribute =
__ATTR(current_limit_max, 0444, current_limit_max_show, NULL);

static struct kobj_attribute version_attribute =
__ATTR(version, 0444, version_show, NULL);

static struct attribute *hardlimit_attrs[] = {
	&hardlimit_max_screen_on_attribute.attr,
	&hardlimit_max_screen_off_attribute.attr,
	&hardlimit_min_screen_on_attribute.attr,
	&hardlimit_min_screen_off_attribute.attr,
	&current_limit_min_attribute.attr,
	&current_limit_max_attribute.attr,
	&version_attribute.attr,
	NULL,
};

static struct attribute_group hardlimit_attr_group = {
.attrs = hardlimit_attrs,
};

/* ------------------------------------------------------------------------------ */
/* Init and exit                                                                  */
/* ------------------------------------------------------------------------------ */

int hardlimit_init(void)
{
	int hardlimit_retval, hardlimit_input_retval;

	/* Enable the sysfs interface */
        hardlimit_kobj = kobject_create_and_add("cpufreq_hardlimit", kernel_kobj);
        if (!hardlimit_kobj) {
                return -ENOMEM;
        }
        hardlimit_retval = sysfs_create_group(hardlimit_kobj, &hardlimit_attr_group);
        if (hardlimit_retval)
                kobject_put(hardlimit_kobj);

        if (!hardlimit_retval) {
		/* Only register to powersuspend and delayed work if we were able to create the sysfs interface */
		register_power_suspend(&cpufreq_hardlimit_suspend_data);
		cpufreq_register_notifier(
			&cpufreq_policy_notifier_block, CPUFREQ_POLICY_NOTIFIER);
	}

        return (hardlimit_retval);
}

void hardlimit_exit(void)
{
	unregister_power_suspend(&cpufreq_hardlimit_suspend_data);
		cpufreq_unregister_notifier(
			&cpufreq_policy_notifier_block, CPUFREQ_POLICY_NOTIFIER);
}

module_init(hardlimit_init);
module_exit(hardlimit_exit);

MODULE_AUTHOR("Jean-Pierre Rasquin <yank555.lu@gmail.com>");
MODULE_DESCRIPTION("'cpufreq_hardlimit' - A cpufreq controlling framework with "
	"screen on/off min/max, wakeup kick and 2-step touchboost");
MODULE_LICENSE("GPL v2");
