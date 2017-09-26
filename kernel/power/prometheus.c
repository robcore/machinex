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

#define VERSION 5
#define VERSION_MIN 6

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
static bool bootcomplete;
bool prometheus_override = false;

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
	list_del(&handler->link);
	mutex_unlock(&prometheus_mtx);
}
EXPORT_SYMBOL(unregister_power_suspend);

static void power_suspend(struct work_struct *work)
{
	struct power_suspend *pos;
	unsigned long irqflags;
	unsigned int counter;
	int error;
	unsigned int nrwl;

	prometheus_override = true;

	if (poweroff_charging || (unlikely(system_state != SYSTEM_RUNNING)) ||
		(unlikely(system_is_restarting()))) {
		prometheus_override = false;
		pr_info("[PROMETHEUS] Cannot Suspend! Unsupported System"
				"State!\n");
		return;
	}

	cancel_work_sync(&power_resume_work);
	pr_info("[PROMETHEUS] Entering Suspend\n");
	mutex_lock(&prometheus_mtx);
	spin_lock_irqsave(&ps_state_lock, irqflags);
	if (ps_state == POWER_SUSPEND_INACTIVE) {
		spin_unlock_irqrestore(&ps_state_lock, irqflags);
		mutex_unlock(&prometheus_mtx);
		prometheus_override = false;
		pr_info("[PROMETHEUS] Suspend Aborted! Screen on!\n");
		return;
	}
	spin_unlock_irqrestore(&ps_state_lock, irqflags);

	intelli_suspend_booster();

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

	mutex_unlock(&prometheus_mtx);
	pr_info("[PROMETHEUS] Shallow Suspend Completed.\n");
	return;
}

static void power_resume(struct work_struct *work)
{
	struct power_suspend *pos;
	unsigned long irqflags;

	if ((poweroff_charging)) {
		pr_info("[PROMETHEUS] Cannot Resume! Unsupported System"
				"State!\n");
		return;
	}

	cancel_work_sync(&power_suspend_work);
	pr_info("[PROMETHEUS] Entering Resume\n");
	mutex_lock(&prometheus_mtx);
	spin_lock_irqsave(&ps_state_lock, irqflags);
	if (ps_state == POWER_SUSPEND_ACTIVE) {
		spin_unlock_irqrestore(&ps_state_lock, irqflags);
		mutex_unlock(&prometheus_mtx);
		return;
	}
	spin_unlock_irqrestore(&ps_state_lock, irqflags);

	if (limit_screen_off_cpus)
		unlock_screen_off_cpus();

	hardplug_all_cpus();

	pr_info("[PROMETHEUS] Resuming\n");
	list_for_each_entry(pos, &power_suspend_handlers, link) {
		if (pos->resume != NULL) {
			pos->resume(pos);
		}
	}

	mutex_unlock(&prometheus_mtx);
	pr_info("[PROMETHEUS] Resume Completed.\n");
}

static void set_power_suspend_state(unsigned int new_state)
{
	if (ps_state != new_state) {
		if (ps_state == POWER_SUSPEND_INACTIVE && new_state == POWER_SUSPEND_ACTIVE) {
			ps_state = new_state;
			pr_info("[PROMETHEUS] Suspend State Activated.\n");
			queue_work_on(0, pwrsup_wq, &power_suspend_work);
		} else if (ps_state == POWER_SUSPEND_ACTIVE && new_state == POWER_SUSPEND_INACTIVE) {
			ps_state = new_state;
			pr_info("[PROMETHEUS] Resume State Activated.\n");
			queue_work(pwrsup_wq, &power_resume_work);
		}
	} else {
		if (likely(bootcomplete))
			pr_info("[PROMETHEUS] Request Ignored, no change\n");
		else
			bootcomplete = true;
	}		
}

void prometheus_panel_beacon(unsigned int new_state)
{
	unsigned long irqflags;

	if (likely(bootcomplete))
		pr_info("[PROMETHEUS] Panel Requests %s.\n", new_state == POWER_SUSPEND_ACTIVE ? "Suspend" : "Resume");

	spin_lock_irqsave(&ps_state_lock, irqflags);
	set_power_suspend_state(new_state);
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

	pwrsup_wq = create_hipri_singlethread_workqueue("prometheus_work");
	if (!pwrsup_wq)
		pr_err("[PROMETHEUS] Failed to allocate workqueue\n");

	mutex_init(&prometheus_mtx);
	spin_lock_init(&ps_state_lock);
	wake_lock_init(&prsynclock, WAKE_LOCK_SUSPEND, "prometheus_synclock");

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
	mutex_destroy(&prometheus_mtx);

	if (prometheus_kobj != NULL)
		kobject_put(prometheus_kobj);
}

subsys_initcall(prometheus_init);
module_exit(prometheus_exit);

MODULE_AUTHOR("Paul Reioux <reioux@gmail.com> / Jean-Pierre Rasquin <yank555.lu@gmail.com>"
				"Rob Patershuk <robpatershuk@gmail.com>");
MODULE_DESCRIPTION("Prometheus was punished by the gods for giving the gift of knowledge to man."
			  "He was cast into the bowels of the earth and pecked by birds.");
MODULE_LICENSE("GPL v2");