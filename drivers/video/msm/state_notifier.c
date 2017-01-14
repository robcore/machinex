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

#include <linux/export.h>
#include <linux/module.h>
#include <linux/state_notifier.h>

#define STATE_NOTIFIER			"state_notifier"

static struct delayed_work suspend_work;
static struct work_struct resume_work;
static struct workqueue_struct *susp_wq;
static unsigned int suspend_defer_time = 10;
module_param_named(suspend_defer_time, suspend_defer_time, uint, 0664);
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
	printk("[STATE_NOTIFIER] SUSPENDING\n");
	state_suspended = true;
	state_notifier_call_chain(STATE_NOTIFIER_SUSPEND, NULL);
	suspend_in_progress = false;
}

static void _resume_work(struct work_struct *work)
{
	printk("[STATE_NOTIFIER] RESUMING\n");
	state_suspended = false;
	state_notifier_call_chain(STATE_NOTIFIER_ACTIVE, NULL);
}

void state_suspend(void)
{
	if (state_suspended || suspend_in_progress)
		return;

	printk("[STATE NOTIFIER] - Suspend Called\n");
	suspend_in_progress = true;

	queue_delayed_work(susp_wq, &suspend_work,
		msecs_to_jiffies(suspend_defer_time * 1000));
}

void state_resume(void)
{
	if (suspend_in_progress)
		printk("[STATE NOTIFIER] - Suspend Work Cancelled by Resume\n");
	else
		printk("[STATE NOTIFIER] - Resume Called\n");

	cancel_delayed_work_sync(&suspend_work);
	suspend_in_progress = false;

	if (state_suspended)
		queue_work(susp_wq, &resume_work);
	else
		printk("[STATE_NOTIFIER] Skipping Resume\n");
}

static int state_notifier_init(void)
{
	susp_wq = alloc_workqueue("state_susp_wq", WQ_UNBOUND | WQ_MEM_RECLAIM, 1);
	if (!susp_wq)
		pr_err("[State_Notifier] failed to allocate suspend workqueue\n");

	INIT_DELAYED_WORK(&suspend_work, _suspend_work);
	INIT_WORK(&resume_work, _resume_work);

	return 0;
}

static void state_notifier_exit(void)
{
	flush_work(&resume_work);
	flush_delayed_work(&suspend_work);
	destroy_workqueue(susp_wq);
}

subsys_initcall(state_notifier_init);
module_exit(state_notifier_exit);

MODULE_AUTHOR("Pranav Vashi <neobuddy89@gmail.com>");
MODULE_DESCRIPTION("State Notifier Driver");
MODULE_LICENSE("GPLv2");
