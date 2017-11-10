/* kernel/power/prometheus.c
 *
 * Copyright (C) 2005-2008 Google, Inc.
 * Copyright (C) 2013 Paul Reioux
 * Copyright (C) 2017 Rob Patershuk
 *
 * Modified by Jean-Pierre Rasquin <yank555.lu@gmail.com>
 * Further modified by Rob Patershuk <robpatershuk@gmail.com>
 *
 * See include/linux/prometheus.h for legacy (powersuspend) changelog.
 *
 * Prometheus was punished by the gods for giving the gift of knowledge to man.
 * He was cast into the bowels of the earth and pecked by birds.
 *
 *
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
#include <linux/prometheus.h>
#include <linux/sysfs_helpers.h>
#include <linux/cpu.h>
#include <linux/display_state.h>
#include "power.h"

#define VERSION 6
#define VERSION_MIN 3

static DEFINE_MUTEX(prometheus_mtx);
static DEFINE_SPINLOCK(ps_state_lock);
static LIST_HEAD(power_suspend_handlers);
static struct workqueue_struct *pwrsup_wq;
struct work_struct power_suspend_work;
struct work_struct power_resume_work;
static void power_suspend(struct work_struct *work);
static void power_resume(struct work_struct *work);
/* Yank555.lu : Current powersuspend ps_state (screen on / off) */
static unsigned int ps_state = POWER_SUSPEND_INACTIVE;
/* Robcore: Provide an option to sync the system on panel suspend
 * accompanied by a wakelock to ensure stability
 * at the cost of battery life
 */
struct wake_lock prsynclock;
static unsigned int sync_on_panel_suspend;
/* For Samsung devices that use it, disable completely when poweroff charging */
extern int poweroff_charging;
extern bool mx_is_cable_attached(void);
extern unsigned int limit_screen_off_cpus;
extern unsigned int limit_screen_on_cpus;
bool prometheus_override = false;
static unsigned int shallow_suspended = SHALLOW_AWAKE;

/*
 * 0 means screen on (resume)
 * 1 means screen off (suspend)
*/

unsigned int report_state(void)
{
	unsigned long flg;
	unsigned int currstate;

	spin_lock_irqsave(&ps_state_lock, flg);
	currstate = ps_state;
	spin_unlock_irqrestore(&ps_state_lock, flg);
	return currstate;
}

unsigned int report_shallow(void)
{
	unsigned long flags;
	unsigned int tmp;
	spin_lock_irqsave(&ps_state_lock, flags);
	tmp = shallow_suspended;
	spin_unlock_irqrestore(&ps_state_lock, flags);
	return tmp;
}

static void set_shallow_suspend(void)
{
	unsigned long flags;
	spin_lock_irqsave(&ps_state_lock, flags);
	if (shallow_suspended == SHALLOW_AWAKE)
		shallow_suspended = SHALLOW_SUSPENDED;
	spin_unlock_irqrestore(&ps_state_lock, flags);
}

static void set_shallow_resume(void)
{
	unsigned long flags;
	spin_lock_irqsave(&ps_state_lock, flags);
	if (shallow_suspended == SHALLOW_SUSPENDED)
		shallow_suspended = SHALLOW_AWAKE;
	spin_unlock_irqrestore(&ps_state_lock, flags);
}

bool prometheus_disabled_oom __read_mostly = false;
static void prometheus_control_oom(bool disable)
{
	if (disable != prometheus_disabled_oom);
		prometheus_disabled_oom = disable;
}

void register_power_suspend(struct power_suspend *handler)
{
	struct list_head *pos;

	mutex_lock(&prometheus_mtx);
	list_for_each(pos, &power_suspend_handlers) {
		struct power_suspend *p;
		p = list_entry(pos, struct power_suspend, link);
	}
	list_add_tail(&handler->link, pos);
	mutex_unlock(&prometheus_mtx);
}
EXPORT_SYMBOL(register_power_suspend);

void unregister_power_suspend(struct power_suspend *handler)
{
	mutex_lock(&prometheus_mtx);
	list_del_init(&handler->link);
	mutex_unlock(&prometheus_mtx);
}
EXPORT_SYMBOL(unregister_power_suspend);

static void power_suspend(struct work_struct *work)
{
	struct power_suspend *pos;

	if (poweroff_charging || (unlikely(system_state != SYSTEM_RUNNING)) ||
		(unlikely(system_is_restarting()))) {
		pr_info("[PROMETHEUS] Cannot Suspend! Unsupported System"
				"State!\n");
		return;
	}

	cancel_work_sync(&power_resume_work);
	pr_info("[PROMETHEUS] Entering Suspend\n");
	mutex_lock(&prometheus_mtx);
	prometheus_override = true;
	if (!report_state()) {
		mutex_unlock(&prometheus_mtx);
		prometheus_override = false;
		pr_info("[PROMETHEUS] Suspend Aborted! Screen on!\n");
		return;
	}

	intelli_suspend_booster();
	cpufreq_hardlimit_suspend();

	pr_info("[PROMETHEUS] Suspending\n");
	list_for_each_entry_reverse(pos, &power_suspend_handlers, link) {
		if (pos->suspend != NULL) {
			pos->suspend(pos);
		}
	}
	prometheus_override = false;

	if (limit_screen_off_cpus)
		lock_screen_off_cpus(0);

	if (sync_on_panel_suspend) {
		wake_lock(&prsynclock);
		pr_info("[PROMETHEUS] Syncing\n");
		sys_sync();
		wake_unlock(&prsynclock);
	}
	set_shallow_suspend();
	pr_info("[PROMETHEUS] Shallow Suspend Completed.\n");
	mutex_unlock(&prometheus_mtx);
}

static void power_resume(struct work_struct *work)
{
	struct power_suspend *pos;

	if ((poweroff_charging)) {
		pr_info("[PROMETHEUS] Cannot Resume! Unsupported System"
				"State!\n");
		return;
	}

	cancel_work_sync(&power_suspend_work);
	pr_info("[PROMETHEUS] Entering Resume\n");
	mutex_lock(&prometheus_mtx);
	if (report_state()) {
		mutex_unlock(&prometheus_mtx);
		return;
	}

	if (limit_screen_off_cpus)
		unlock_screen_off_cpus();

	cpufreq_hardlimit_resume();

	pr_info("[PROMETHEUS] Resuming\n");
	list_for_each_entry(pos, &power_suspend_handlers, link) {
		if (pos->resume != NULL) {
			pos->resume(pos);
		}
	}
	set_shallow_resume();
	pr_info("[PROMETHEUS] Resume Completed.\n");
	mutex_unlock(&prometheus_mtx);

}

void prometheus_panel_beacon(unsigned int new_state)
{
	unsigned long irqflags;

	spin_lock_irqsave(&ps_state_lock, irqflags);
	if (ps_state != new_state) {
		if (!ps_state && new_state) {
			ps_state = new_state;
			pr_info("[PROMETHEUS] Suspend State Activated.\n");
			queue_work_on(0, pwrsup_wq, &power_suspend_work);
		} else if (ps_state && !new_state) {
			ps_state = new_state;
			pr_info("[PROMETHEUS] Resume State Activated.\n");
			queue_work_on(0, pwrsup_wq, &power_resume_work);
		}
	}
	spin_unlock_irqrestore(&ps_state_lock, irqflags);
}
EXPORT_SYMBOL(prometheus_panel_beacon);

// ------------------------------------------ sysfs interface ------------------------------------------

static ssize_t prometheus_sync_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
        return sprintf(buf, "%u\n", sync_on_panel_suspend);
}

static ssize_t prometheus_sync_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	int val;

	sscanf(buf, "%d\n", &val);

	sanitize_min_max(val, 0, 1);

	sync_on_panel_suspend = val;
	return count;
}

static struct kobj_attribute prometheus_sync_attribute =
	__ATTR(prometheus_sync, 0644,
		prometheus_sync_show,
		prometheus_sync_store);

static ssize_t prometheus_version_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "Prometheus Version: %u.%u\n", VERSION, VERSION_MIN);
}

static struct kobj_attribute prometheus_version_attribute =
	__ATTR(prometheus_version, 0444,
		prometheus_version_show,
		NULL);

static struct attribute *prometheus_attrs[] =
{
	&prometheus_sync_attribute.attr,
	&prometheus_version_attribute.attr,
	NULL,
};

static const struct attribute_group prometheus_attr_group =
{
	.attrs = prometheus_attrs,
};

static struct kobject *prometheus_kobj;

static int prometheus_init(void)
{
	struct power_suspend *pos;
	int sysfs_result;

	prometheus_kobj = kobject_create_and_add("prometheus",
		kernel_kobj);

	if (!prometheus_kobj) {
		pr_err("%s kobject create failed!\n", __FUNCTION__);
		return -ENOMEM;
	}

	sysfs_result = sysfs_create_group(prometheus_kobj,
		&prometheus_attr_group);

	if (sysfs_result) {
		pr_info("%s group create failed!\n", __FUNCTION__);
		kobject_put(prometheus_kobj);
		return -ENOMEM;
	}

	mutex_init(&prometheus_mtx);
	spin_lock_init(&ps_state_lock);
	wake_lock_init(&prsynclock, WAKE_LOCK_SUSPEND, "prometheus_synclock");

	pwrsup_wq = create_hipri_singlethread_workqueue("prometheus_work");
	if (pwrsup_wq == NULL) {
		pr_err("[PROMETHEUS] Failed to allocate workqueue\n");
		sysfs_remove_group(prometheus_kobj, &prometheus_attr_group);
		kobject_put(prometheus_kobj);
		return -ENOMEM;
	}

	INIT_WORK(&power_suspend_work, power_suspend);
	INIT_WORK(&power_resume_work, power_resume);

	return 0;
}

subsys_initcall(prometheus_init);
MODULE_AUTHOR("Paul Reioux <reioux@gmail.com> / Jean-Pierre Rasquin <yank555.lu@gmail.com>"
				"Rob Patershuk <robpatershuk@gmail.com>");
MODULE_DESCRIPTION("Prometheus was punished by the gods for giving the gift of knowledge to man."
			  "He was cast into the bowels of the earth and pecked by birds.");
MODULE_LICENSE("GPL v2");
