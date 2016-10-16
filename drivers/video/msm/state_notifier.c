/*
 * State Notifier Driver
 *
 * Copyright (c) 2013-2016, Pranav Vashi <neobuddy89@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/state_notifier.h>
#include <linux/kernel.h>
#include <linux/notifier.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/export.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/delay.h>

/*necessary measure given existing conflictions */
#ifdef pr_fmt(fmt)
#undef pr_fmt(fmt)
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#define DEFAULT_SUSPEND_DEFER_TIME 	10
#define STATE_NOTIFIER			"state_notifier"
#define ENABLED_ON_BOOT 0 /* Toggle this to enable or disable at startup */
/*
 * debug = 1 will print all
 */
static unsigned int debug;
module_param_named(debug_mask, debug, uint, 0644);

/* safety measure despite very unlikely scenario */
#ifdef dprintk
#undef drpintk
#define dprintk(msg...)		\
do {				\
	if (debug)		\
		pr_info(msg);	\
} while (0)

static unsigned int snotifier_enabled = ENABLED_ON_BOOT; /* now a pointer */
static unsigned int suspend_defer_time = DEFAULT_SUSPEND_DEFER_TIME;
module_param_named(suspend_defer_time, suspend_defer_time, uint, 0664);
static struct delayed_work suspend_work;
static struct workqueue_struct *susp_wq;
struct work_struct resume_work;
bool state_suspended;
module_param_named(state_suspended, state_suspended, bool, 0444);
static bool suspend_in_progress;

static BLOCKING_NOTIFIER_HEAD(state_notifier_list);

/**
 *	state_register_client - register a client notifier
 *	@nb: notifier block to callback on events
 */
int state_register_client(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&state_notifier_list, nb);
}
EXPORT_SYMBOL(state_register_client);

/**
 *	state_unregister_client - unregister a client notifier
 *	@nb: notifier block to callback on events
 */
int state_unregister_client(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&state_notifier_list, nb);
}
EXPORT_SYMBOL(state_unregister_client);

/**
 *	state_notifier_call_chain - notify clients on state_events
 *	@val: Value passed unmodified to notifier function
 *	@v: pointer passed unmodified to notifier function
 *
 */
int state_notifier_call_chain(unsigned long val, void *v)
{
	return blocking_notifier_call_chain(&state_notifier_list, val, v);
}
EXPORT_SYMBOL_GPL(state_notifier_call_chain);

static void _suspend_work(struct work_struct *work)
{
	state_suspended = true;
	state_notifier_call_chain(STATE_NOTIFIER_SUSPEND, NULL);
	suspend_in_progress = false;
	dprintk("%s: suspend completed.\n", STATE_NOTIFIER);
}

static void _resume_work(struct work_struct *work)
{
	state_suspended = false;
	state_notifier_call_chain(STATE_NOTIFIER_ACTIVE, NULL);
	dprintk("%s: resume completed.\n", STATE_NOTIFIER);
}

void state_suspend(void)
{
	dprintk("%s: suspend called.\n", STATE_NOTIFIER);
	if (suspend_in_progress || state_suspended) /* No longer need the !enabled condition due to refactored init function below */
		return;

	cancel_work_sync(&resume_work); /* for rapid on/off events */
	suspend_in_progress = true;

	queue_delayed_work(susp_wq, &suspend_work,
		msecs_to_jiffies(suspend_defer_time * 1000));
}

void state_resume(void)
{
	if (!state_suspended) /* This is to address your concern of the driver being disabled during suspend */
		return;

	dprintk("%s: resume called.\n", STATE_NOTIFIER);
	cancel_delayed_work_sync(&suspend_work);
	suspend_in_progress = false;

	if (state_suspended)
		queue_work(susp_wq, &resume_work);
}

static int state_notifier_init(void)
{
	if (!snotifier_enabled) /* Now the workqueue, and thus, the driver, isn't even allocated unless explicitly enabled above or by userspace */
		return 0;

	susp_wq = create_singlethread_workqueue("state_susp_wq");
	if (!susp_wq)
		pr_err("State Notifier failed to allocate suspend workqueue\n");
	return -ENOMEM; /* Without the workqueue, the driver won't work, so halt its creation here if that happens */

	INIT_DELAYED_WORK(&suspend_work, _suspend_work);
	INIT_WORK(&resume_work, _resume_work);
	pr_info("%s: State Notifier Init\n", __func__);

	return 0;
}

static void __ref state_notifier_exit(void)
{
	cancel_work_sync(&resume_work);
	cancel_delayed_work_sync(&suspend_work);
	flush_workqueue(susp_wq);
	destroy_workqueue(susp_wq);

	pr_info("%s: State Notifier Exit\n", __func__);
}

/* Master Switch */
static int set_enabled(const char *val, const struct kernel_param *kp)
{
	int ret = 0;
	unsigned int i;

	ret = kstrtouint(val, 10, &i);
	if (ret)
		return -EINVAL;

	if (i < 0 || i > 1)
		return -ERANGE;

	if (i == snotifier_enabled)
		return ret;

	snotifier_enabled = i;

	if (snotifier_enabled)
		ret = state_notifier_init();
	else
		state_notifier_exit();

	return ret;
}

static struct kernel_param_ops enabled_ops = {
	.set = set_enabled,
	.get = param_get_uint,
};

module_param_cb(enabled, &enabled_ops, &snotifier_enabled, 0644);

subsys_initcall(state_notifier_init);
module_exit(state_notifier_exit);

MODULE_AUTHOR("Pranav Vashi <neobuddy89@gmail.com>");
MODULE_DESCRIPTION("State Notifier Driver");
MODULE_LICENSE("GPLv2");
#endif
#endif
