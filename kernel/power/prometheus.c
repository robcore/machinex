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
#include "power.h"

#define VERSION 1
#define VERSION_MIN 0

static DEFINE_MUTEX(power_suspend_lock);
static DEFINE_SPINLOCK(ps_state_lock);
static LIST_HEAD(power_suspend_handlers);
static struct workqueue_struct *pwrsup_wq;
struct work_struct power_suspend_work;
struct work_struct power_resume_work;
static void power_suspend(struct work_struct *work);
static void power_resume(struct work_struct *work);
/* Yank555.lu : Current powersuspend ps_state (screen on / off) */
static int ps_state;
/* Robcore: Provide an option to sync the system on powersuspend */
static unsigned int sync_on_powersuspend;
extern int poweroff_charging;
#define GLOBAL_PM 1
static unsigned int use_global_suspend = GLOBAL_PM;

void register_power_suspend(struct power_suspend *handler)
{
	struct list_head *pos;

	mutex_lock(&power_suspend_lock);
	list_for_each(pos, &power_suspend_handlers) {
		struct power_suspend *p;
		p = list_entry(pos, struct power_suspend, link);
	}
	list_add_tail(&handler->link, pos);
	mutex_unlock(&power_suspend_lock);
}
EXPORT_SYMBOL(register_power_suspend);

void unregister_power_suspend(struct power_suspend *handler)
{
	mutex_lock(&power_suspend_lock);
	list_del(&handler->link);
	mutex_unlock(&power_suspend_lock);
}
EXPORT_SYMBOL(unregister_power_suspend);

static void power_suspend(struct work_struct *work)
{
	struct power_suspend *pos;
	unsigned long irqflags;
	unsigned int counter;
	int abort = 0;

	cancel_work_sync(&power_resume_work);

	if ((poweroff_charging) || (system_state == SYSTEM_RESTART)
		|| (system_state == SYSTEM_POWER_OFF)) {
		pr_info("[PROMETHEUS] Ignoring Unsupported System \
				State\n");
		return;
	}

	pr_info("[PROMETHEUS] Entering Suspend...\n");
	mutex_lock(&power_suspend_lock);
	spin_lock_irqsave(&ps_state_lock, irqflags);
	if (ps_state == POWER_SUSPEND_INACTIVE)
		abort = 1;
	spin_unlock_irqrestore(&ps_state_lock, irqflags);

	if (abort) {
		mutex_unlock(&power_suspend_lock);
		return;
	}

	pr_info("[PROMETHEUS] Suspending...\n");
	list_for_each_entry(pos, &power_suspend_handlers, link) {
		if (pos->suspend != NULL) {
			pos->suspend(pos);
		}
	}

	mutex_unlock(&power_suspend_lock);

	if (sync_on_powersuspend) {
		pr_info("[PROMETHEUS] Syncing\n");
		sys_sync();
	}

	pr_info("[PROMETHEUS] Initial Suspend Completed\n");

	if (use_global_suspend) {
		if (!pm_get_wakeup_count(&counter, false) || pm_wakeup_pending()) {
				pr_info("[PROMETHEUS] Skipping PM Suspend. Wakelocks held.\n");
				return;
		}

		if (!mutex_trylock(&pm_mutex)) {
			pr_info("[PROMETHEUS] Skipping PM Suspend. PM Busy.\n");
			return;
		}

		pr_info("[PROMETHEUS] Calling System Suspend!\n");
		pm_suspend(PM_HIBERNATION_PREPARE);
		mutex_unlock(&pm_mutex);
	} else
		pr_info("[PROMETHEUS] Suspend Completed.\n");
}

static void power_resume(struct work_struct *work)
{
	struct power_suspend *pos;
	unsigned long irqflags;
	int abort = 0;

	cancel_work_sync(&power_suspend_work);
	pr_info("[PROMETHEUS] Entering Resume...\n");
	mutex_lock(&power_suspend_lock);
	spin_lock_irqsave(&ps_state_lock, irqflags);
	if (ps_state == POWER_SUSPEND_ACTIVE)
		abort = 1;
	spin_unlock_irqrestore(&ps_state_lock, irqflags);

	if (abort)
		goto abort;

	pr_info("[PROMETHEUS] Resuming...\n");
	list_for_each_entry_reverse(pos, &power_suspend_handlers, link) {
		if (pos->resume != NULL) {
			pos->resume(pos);
		}
	}
	pr_info("[PROMETHEUS] Resume Completed.\n");

abort:
	mutex_unlock(&power_suspend_lock);

}

void set_power_suspend_state(int new_state)
{
	unsigned long irqflags;

	if (ps_state != new_state) {
		spin_lock_irqsave(&ps_state_lock, irqflags);
		if (ps_state == POWER_SUSPEND_INACTIVE && new_state == POWER_SUSPEND_ACTIVE) {
			pr_info("[PROMETHEUS] Suspend State Activated.\n");
			ps_state = new_state;
			queue_work(pwrsup_wq, &power_suspend_work);
		} else if (ps_state == POWER_SUSPEND_ACTIVE && new_state == POWER_SUSPEND_INACTIVE) {
			pr_info("[PROMETHEUS] Resume State Activated.\n");
			ps_state = new_state;
			queue_work(pwrsup_wq, &power_resume_work);
		}
		spin_unlock_irqrestore(&ps_state_lock, irqflags);
	} else {
		pr_info("[PROMETHEUS] Ignoring State Request.\n");
	}
}

void prometheus_panel_beacon(int new_state)
{
	pr_info("[PROMETHEUS] Panel Requests %s.\n", new_state == POWER_SUSPEND_ACTIVE ? "Suspend" : "Resume");
	set_power_suspend_state(new_state);
}

EXPORT_SYMBOL(prometheus_panel_beacon);

// ------------------------------------------ sysfs interface ------------------------------------------

static ssize_t prometheus_sync_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
        return sprintf(buf, "%u\n", sync_on_powersuspend);
}

static ssize_t prometheus_sync_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned int val;

	sscanf(buf, "%u\n", &val);

	if (val <= 0)
		val = 0;
	if (val >= 1)
		val = 1;

	sync_on_powersuspend = val;
	return count;
}

static struct kobj_attribute prometheus_sync_attribute =
	__ATTR(power_suspend_sync, 0644,
		prometheus_sync_show,
		prometheus_sync_store);

static ssize_t global_suspend_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
        return sprintf(buf, "%u\n", use_global_suspend);
}

static ssize_t global_suspend_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned int val;

	sscanf(buf, "%u\n", &val);

	if (val <= 0)
		val = 0;
	if (val >= 1)
		val = 1;

	use_global_suspend = val;
	return count;
}

static struct kobj_attribute global_suspend_attribute =
	__ATTR(global_suspend, 0644,
		global_suspend_show,
		global_suspend_store);

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
	&global_suspend_attribute.attr,
	&prometheus_version_attribute.attr,
	NULL,
};

static struct attribute_group prometheus_attr_group =
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

	pwrsup_wq = alloc_workqueue("prometheus_work", WQ_UNBOUND | WQ_MEM_RECLAIM | WQ_HIGHPRI, 1);
	if (!pwrsup_wq)
		pr_err("[PROMETHEUS] Failed to allocate workqueue\n");

	INIT_WORK(&power_suspend_work, power_suspend);
	INIT_WORK(&power_resume_work, power_resume);

	return 0;
}

/* This should never have to be used except on shutdown */
static void prometheus_exit(void)
{
	flush_work(&power_suspend_work);
	flush_work(&power_resume_work);
	destroy_workqueue(pwrsup_wq);

	if (prometheus_kobj != NULL)
		kobject_put(prometheus_kobj);
}

subsys_initcall(prometheus_init);
module_exit(prometheus_exit);

MODULE_AUTHOR("Paul Reioux <reioux@gmail.com> / Jean-Pierre Rasquin <yank555.lu@gmail.com> \
				Rob Patershuk <robpatershuk@gmail.com>");
MODULE_DESCRIPTION("Prometheus was punished by the gods for giving the gift of knowledge to man." \
			  "He was cast into the bowels of the earth and pecked by birds.");
MODULE_LICENSE("GPL v2");
