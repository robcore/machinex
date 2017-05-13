/* kernel/power/powersuspend.c
 *
 * Copyright (C) 2005-2008 Google, Inc.
 * Copyright (C) 2013 Paul Reioux
 *
 * Modified by Jean-Pierre Rasquin <yank555.lu@gmail.com>
 * Further modified by Rob Patershuk <robpatershuk@gmail.com>
 *
 *  v1.1 - make powersuspend not depend on a userspace initiator anymore,
 *         but use a hook in autosleep instead.
 *
 *  v1.2 - make kernel / userspace mode switchable
 *
 *  v1.3 - add a hook in display panel driver as alternative kernel trigger
 *
 *  v1.4 - add a hybrid-kernel mode, accepting both kernel hooks (first wins)
 *
 *  v1.5 - fix hybrid-kernel mode cannot be set through sysfs
 *
 *  v1.6 - remove autosleep and hybrid modes (autosleep not working on shamu)
 *
 *  v1.7 - do only run state change if change actually requests a new state
 *
 * v1.7.1 - Add autosleep and hybrid modes back
 *
 * v1.7.2 Cleanup
 *
 * v1.8.0 Changed "userspace" mode to disabled, and removed the ability to store
 *        to the syfs. Now The driver is autonomous, without any userspace
 *        interaction required/allowed. Also a little bit of sysfs
 *		  ordering cleanup.
 *
 * v1.8.1 Replaced deprecated singlethread workqueue with updated schedule_work.
 *
 * v1.9.1 Updated the depecrated method of declaring work but simply declaring
 *		  the two work structs.  Also actually INITialized the work on init, and
 *        flushed it on exit.
 *
 * v1.9.2 Remove unneccessary "MODE" variable as we only have one mechanism of
 *		  action remaining. Also removed the useless state sysfs entry.  Like
 *		  state notifier, we can only see "state" when the screen is on, so
 *		  it is pointless to expose to userspace. Topped off with some cleanup.
 *
 * v2.0   Final cleanup to functionality.  For faster response to screen on/off events,
 *		  ensure that the previous work is cancelled upon a valid requested state change.
 *		  Switched back to a single thread workqueue but allocated properly. Topped off
 *		  with some driver cleanup and a config option for using the SUB_MINOR_VERISON.
 *
 * v2.1   Provide a user-configurable option to sync the system on powersuspend.
 *
 * v2.2   Remove the mutex unlock mistakingly added during system sync setup.
 *
 * v2.3   Move cancelling previous state's work to the current state change work item.
 *
 * v2.4   Back out of suspend if the device is in the process of rebooting, powering
 *		  off, or poweroff_charging. Additionally, refactor abort handling so that
 *		  sys_sync isnt called within a mutex-locked region.  Additionally, set sysfs
 *		  up in a way that doesn't allow for failure. No functional change
 *		  to actual suspend/resume handling other than the fixes mentioned.
 *
 * v2.5	  Added a big, scary hook to call pm_suspend.
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
#include <linux/powersuspend.h>
#include "power.h"

#define MAJOR_VERSION	2
#define MINOR_VERSION	5
#ifdef  CONFIG_POWERSUSPEND_BETA_VERSION
#define SUB_MINOR_VERSION
#endif

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
static unsigned int use_global_suspend;

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
	int abort = 0;

	cancel_work_sync(&power_resume_work);

	if ((poweroff_charging) || (system_state == SYSTEM_RESTART)
		|| (system_state == SYSTEM_POWER_OFF)) {
		pr_info("[POWERSUSPEND] Ignoring Unsupported System \
				State\n");
		return;
	}

	pr_info("[POWERSUSPEND] Entering Suspend...\n");
	mutex_lock(&power_suspend_lock);
	spin_lock_irqsave(&ps_state_lock, irqflags);
	if (ps_state == POWER_SUSPEND_INACTIVE)
		abort = 1;
	spin_unlock_irqrestore(&ps_state_lock, irqflags);

	if (abort) {
		mutex_unlock(&power_suspend_lock);
		return;
	}

	pr_info("[POWERSUSPEND] Suspending...\n");
	list_for_each_entry(pos, &power_suspend_handlers, link) {
		if (pos->suspend != NULL) {
			pos->suspend(pos);
		}
	}

	mutex_unlock(&power_suspend_lock);

	if (sync_on_powersuspend) {
		pr_info("[POWERSUSPEND] Syncing\n");
		sys_sync();
	}

	if (use_global_suspend) {
		if (!mutex_trylock(&pm_mutex)) {
			pr_info("[POWERSUSPEND] Global Suspend Busy!\n");
			return;
		}

		pr_info("[POWERSUSPEND] Suspend Completed\n");
		pr_info("[POWERSUSPEND] Calling System Suspend!\n");
		pm_suspend(PM_HIBERNATION_PREPARE);
		mutex_unlock(&pm_mutex);
	} else
		pr_info("[POWERSUSPEND] Suspend Completed.\n");
}

static void power_resume(struct work_struct *work)
{
	struct power_suspend *pos;
	unsigned long irqflags;
	int abort = 0;

	cancel_work_sync(&power_suspend_work);
	pr_info("[POWERSUSPEND] Entering Resume...\n");
	mutex_lock(&power_suspend_lock);
	spin_lock_irqsave(&ps_state_lock, irqflags);
	if (ps_state == POWER_SUSPEND_ACTIVE)
		abort = 1;
	spin_unlock_irqrestore(&ps_state_lock, irqflags);

	if (abort)
		goto abort;

	pr_info("[POWERSUSPEND] Resuming...\n");
	list_for_each_entry_reverse(pos, &power_suspend_handlers, link) {
		if (pos->resume != NULL) {
			pos->resume(pos);
		}
	}
	pr_info("[POWERSUSPEND] Resume Completed.\n");

abort:
	mutex_unlock(&power_suspend_lock);

}

void set_power_suspend_state(int new_state)
{
	unsigned long irqflags;

	if (ps_state != new_state) {
		spin_lock_irqsave(&ps_state_lock, irqflags);
		if (ps_state == POWER_SUSPEND_INACTIVE && new_state == POWER_SUSPEND_ACTIVE) {
			pr_info("[POWERSUSPEND] Suspend State Activated.\n");
			ps_state = new_state;
			queue_work(pwrsup_wq, &power_suspend_work);
		} else if (ps_state == POWER_SUSPEND_ACTIVE && new_state == POWER_SUSPEND_INACTIVE) {
			pr_info("[POWERSUSPEND] Resume State Activated.\n");
			ps_state = new_state;
			queue_work(pwrsup_wq, &power_resume_work);
		}
		spin_unlock_irqrestore(&ps_state_lock, irqflags);
	} else {
		pr_info("[POWERSUSPEND] Ignoring State Request.\n");
	}
}

void set_power_suspend_state_panel_hook(int new_state)
{
	pr_info("[POWERSUSPEND] Panel Requests %s.\n", new_state == POWER_SUSPEND_ACTIVE ? "Suspend" : "Resume");
	set_power_suspend_state(new_state);
}

EXPORT_SYMBOL(set_power_suspend_state_panel_hook);

// ------------------------------------------ sysfs interface ------------------------------------------

static ssize_t power_suspend_sync_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
        return sprintf(buf, "%u\n", sync_on_powersuspend);
}

static ssize_t power_suspend_sync_store(struct kobject *kobj,
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

static struct kobj_attribute power_suspend_sync_attribute =
	__ATTR(power_suspend_sync, 0644,
		power_suspend_sync_show,
		power_suspend_sync_store);

static ssize_t power_suspend_use_global_suspend_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
        return sprintf(buf, "%u\n", use_global_suspend);
}

static ssize_t power_suspend_use_global_suspend_store(struct kobject *kobj,
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

static struct kobj_attribute power_suspend_use_global_suspend_attribute =
	__ATTR(power_suspend_use_global_suspend, 0644,
		power_suspend_use_global_suspend_show,
		power_suspend_use_global_suspend_store);

static ssize_t power_suspend_version_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
#ifdef CONFIG_POWERSUSPEND_BETA_VERSION
	return sprintf(buf, "Powersuspend Version: %d.%d.%d\n", MAJOR_VERSION, MINOR_VERSION, SUB_MINOR_VERSION);
#else
	return sprintf(buf, "Powersuspend Version: %d.%d\n", MAJOR_VERSION, MINOR_VERSION);
#endif
}

static struct kobj_attribute power_suspend_version_attribute =
	__ATTR(power_suspend_version, 0444,
		power_suspend_version_show,
		NULL);

static struct attribute *power_suspend_attrs[] =
{
	&power_suspend_sync_attribute.attr,
	&power_suspend_use_global_suspend_attribute.attr,
	&power_suspend_version_attribute.attr,
	NULL,
};

static struct attribute_group power_suspend_attr_group =
{
	.attrs = power_suspend_attrs,
};

static struct kobject *power_suspend_kobj;

static int power_suspend_init(void)
{
	struct power_suspend *pos;
	int sysfs_result;

	power_suspend_kobj = kobject_create_and_add("power_suspend",
		kernel_kobj);

	if (!power_suspend_kobj) {
		pr_err("%s kobject create failed!\n", __FUNCTION__);
		return -ENOMEM;
	}

	sysfs_result = sysfs_create_group(power_suspend_kobj,
		&power_suspend_attr_group);

	if (sysfs_result) {
		pr_info("%s group create failed!\n", __FUNCTION__);
		kobject_put(power_suspend_kobj);
		return -ENOMEM;
	}

	pwrsup_wq = alloc_workqueue("ps_pwrsup_wq", WQ_UNBOUND | WQ_MEM_RECLAIM | WQ_HIGHPRI, 1);
	if (!pwrsup_wq)
		pr_err("[POWERSUSPEND] Failed to allocate workqueue\n");

	INIT_WORK(&power_suspend_work, power_suspend);
	INIT_WORK(&power_resume_work, power_resume);

	return 0;
}

/* This should never have to be used except on shutdown */
static void power_suspend_exit(void)
{
	flush_work(&power_suspend_work);
	flush_work(&power_resume_work);
	destroy_workqueue(pwrsup_wq);

	if (power_suspend_kobj != NULL)
		kobject_put(power_suspend_kobj);
}

subsys_initcall(power_suspend_init);
module_exit(power_suspend_exit);

MODULE_AUTHOR("Paul Reioux <reioux@gmail.com> / Jean-Pierre Rasquin <yank555.lu@gmail.com> \
				Rob Patershuk <robpatershuk@gmail.com>");
MODULE_DESCRIPTION("power_suspend - A replacement kernel PM driver for"
        "Android's deprecated early_suspend/late_resume PM driver!");
MODULE_LICENSE("GPL v2");
