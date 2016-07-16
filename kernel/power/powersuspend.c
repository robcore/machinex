/* kernel/power/powersuspend.c
 *
 * Copyright (C) 2005-2008 Google, Inc.
 * Copyright (C) 2013 Paul Reioux
 *
 * Modified by Jean-Pierre Rasquin <yank555.lu@gmail.com>
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
 *  v1.5 - Remove hybrid mode! as it's calling too early wakeup and mdss stuck!
 *         screen may not turn on for LG G2 kernel in random cases.
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
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>

#define MAJOR_VERSION	1
#define MINOR_VERSION	6

/*
 * debug = 1 will print all
 */
static unsigned int debug = 1;
module_param_named(debug_mask, debug, uint, 0644);

static unsigned int sleep_state = 0;
module_param_named(sleep_state, sleep_state, uint, 0644);

#define dprintk(msg...)		\
do { 				\
	if (debug)		\
		pr_info(msg);	\
} while (0)

struct workqueue_struct *suspend_work_queue;

static DEFINE_MUTEX(power_suspend_lock);
static LIST_HEAD(power_suspend_handlers);
static void power_suspend(struct work_struct *work);
static void power_resume(struct work_struct *work);
static DECLARE_WORK(power_suspend_work, power_suspend);
static DECLARE_WORK(power_resume_work, power_resume);
static DEFINE_SPINLOCK(state_lock);

/* Yank555.lu : Current powersave state (screen on / off) */
static int state;
/* Yank555.lu : Current powersave mode  (kernel / userspace / panel) */
static int mode;

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

	dprintk("[POWERSUSPEND] entering suspend...\n");
	mutex_lock(&power_suspend_lock);
	spin_lock_irqsave(&state_lock, irqflags);
	if (state == POWER_SUSPEND_INACTIVE)
		abort = 1;
	spin_unlock_irqrestore(&state_lock, irqflags);

	if (abort)
		goto abort_suspend;

	dprintk("[POWERSUSPEND] suspending...\n");
	list_for_each_entry(pos, &power_suspend_handlers, link) {
		if (pos->suspend != NULL) {
			pos->suspend(pos);
		}
	}
	dprintk("[POWERSUSPEND] suspend completed.\n");
	sleep_state = 1;
abort_suspend:
	mutex_unlock(&power_suspend_lock);
}

static void power_resume(struct work_struct *work)
{
	struct power_suspend *pos;
	unsigned long irqflags;
	int abort = 0;

	dprintk("[POWERSUSPEND] entering resume...\n");
	mutex_lock(&power_suspend_lock);
	spin_lock_irqsave(&state_lock, irqflags);
	if (state == POWER_SUSPEND_ACTIVE)
		abort = 1;
	spin_unlock_irqrestore(&state_lock, irqflags);

	if (abort)
		goto abort_resume;

	dprintk("[POWERSUSPEND] resuming...\n");
	sleep_state = 0;
	list_for_each_entry_reverse(pos, &power_suspend_handlers, link) {
		if (pos->resume != NULL) {
			pos->resume(pos);
		}
	}
	dprintk("[POWERSUSPEND] resume completed.\n");
abort_resume:
	mutex_unlock(&power_suspend_lock);
}

void set_power_suspend_state(int new_state)
{
	unsigned long irqflags;

	spin_lock_irqsave(&state_lock, irqflags);
	if (state == POWER_SUSPEND_INACTIVE && new_state == POWER_SUSPEND_ACTIVE) {
		dprintk("[POWERSUSPEND] state activated.\n");
		state = new_state;
		queue_work(suspend_work_queue, &power_suspend_work);
	} else if (state == POWER_SUSPEND_ACTIVE && new_state == POWER_SUSPEND_INACTIVE) {
		dprintk("[POWERSUSPEND] state deactivated.\n");
		state = new_state;
		queue_work(suspend_work_queue, &power_resume_work);
	}
	spin_unlock_irqrestore(&state_lock, irqflags);
}

void set_power_suspend_state_autosleep_hook(int new_state)
{
	dprintk("[POWERSUSPEND] autosleep resquests %s.\n", new_state == POWER_SUSPEND_ACTIVE ? "sleep" : "wakeup");
	/* Yank555.lu : Only allow autosleep hook changes in autosleep */
	if (mode == POWER_SUSPEND_AUTOSLEEP)
		set_power_suspend_state(new_state);
}

EXPORT_SYMBOL(set_power_suspend_state_autosleep_hook);

void set_power_suspend_state_panel_hook(int new_state)
{
	dprintk("[POWERSUSPEND] panel resquests %s.\n", new_state == POWER_SUSPEND_ACTIVE ? "sleep" : "wakeup");
	/* Yank555.lu : Only allow panel hook changes in mdss */
	if (mode == POWER_SUSPEND_PANEL)
		set_power_suspend_state(new_state);
}

EXPORT_SYMBOL(set_power_suspend_state_panel_hook);

/* ------------------------------------------ sysfs interface ------------------------------------------ */

static ssize_t power_suspend_state_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
        return sprintf(buf, "%u\n", state);
}

static ssize_t power_suspend_state_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	int new_state = 0;

	/* Yank555.lu : Only allow sysfs changes from userspace mode */
	if (mode != POWER_SUSPEND_USERSPACE)
		return -EINVAL;

	sscanf(buf, "%d\n", &new_state);

	dprintk("[POWERSUSPEND] userspace resquests %s.\n", new_state == POWER_SUSPEND_ACTIVE ? "sleep" : "wakeup");
	if(new_state == POWER_SUSPEND_ACTIVE || new_state == POWER_SUSPEND_INACTIVE)
		set_power_suspend_state(new_state);

	return count;
}

static struct kobj_attribute power_suspend_state_attribute =
	__ATTR(power_suspend_state, 0666,
		power_suspend_state_show,
		power_suspend_state_store);

static ssize_t power_suspend_mode_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
        return sprintf(buf, "%u\n", mode);
}

static ssize_t power_suspend_mode_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	int data = 0;

	sscanf(buf, "%d\n", &data);

#if 0 /* do not allow user/app to mess with this control */
	switch (data) {
		case POWER_SUSPEND_AUTOSLEEP:
		case POWER_SUSPEND_PANEL:
		case POWER_SUSPEND_USERSPACE:
			mode = data;
			return count;
		default:
			return -EINVAL;
	}
#else
	return count;
#endif
}

static struct kobj_attribute power_suspend_mode_attribute =
	__ATTR(power_suspend_mode, 0666,
		power_suspend_mode_show,
		power_suspend_mode_store);

static ssize_t power_suspend_version_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "version: %d.%d\n", MAJOR_VERSION, MINOR_VERSION);
}

static struct kobj_attribute power_suspend_version_attribute =
	__ATTR(power_suspend_version, 0444,
		power_suspend_version_show,
		NULL);

static struct attribute *power_suspend_attrs[] =
{
	&power_suspend_state_attribute.attr,
	&power_suspend_mode_attribute.attr,
	&power_suspend_version_attribute.attr,
	NULL,
};

static struct attribute_group power_suspend_attr_group =
{
	.attrs = power_suspend_attrs,
};

static struct kobject *power_suspend_kobj;

/* ------------------ sysfs interface ----------------------- */
static int __init power_suspend_init(void)
{

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
	suspend_work_queue = create_singlethread_workqueue("p-suspend");

	if (suspend_work_queue == NULL) {
		return -ENOMEM;
	}

#if 0 /* INFO */
	mode = POWER_SUSPEND_USERSPACE;	/* Yank555.lu : Default to userspace mode */
	mode = POWER_SUSPEND_PANEL;	/* Yank555.lu : Default to display panel mode */
#endif
	mode = POWER_SUSPEND_AUTOSLEEP; /* Yank555.lu : Default to autosleep mode */

	return 0;
}

static void __exit power_suspend_exit(void)
{
	if (power_suspend_kobj != NULL)
		kobject_put(power_suspend_kobj);
	destroy_workqueue(suspend_work_queue);
} 

core_initcall(power_suspend_init);
module_exit(power_suspend_exit);

MODULE_AUTHOR("Paul Reioux <reioux@gmail.com> / Jean-Pierre Rasquin <yank555.lu@gmail.com>");
MODULE_DESCRIPTION("power_suspend - A replacement kernel PM driver for"
        "Android's deprecated early_suspend/late_resume PM driver!");
MODULE_LICENSE("GPL v2");
