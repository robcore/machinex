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

static bool enabled = true;
module_param_named(enabled, enabled, bool, 0664);
static struct work_struct suspend_work;
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
	printk("[STATE_NOTIFIER]suspend completed\n");
}

static void _resume_work(struct work_struct *work)
{
	state_suspended = false;
	state_notifier_call_chain(STATE_NOTIFIER_ACTIVE, NULL);
	printk("[STATE_NOTIFIER] resume completed\n");
}

void state_suspend(void)
{
	printk("[STATE_NOTIFIER] Suspend Called.\n");
	if (state_suspended || suspend_in_progress || !enabled)
		return;

	suspend_in_progress = true;

	schedule_work_on(0, &suspend_work);
}

/* Rob Note: I am still adding the condition that the state should only be changed
 * if the previous state wasn't already set to suspend, as my init and screen-on functions are still tied together.
 * This ensures that the driver's functions are ONLY activated upon the initial suspend.
 */
void state_resume(void)
{
	if (!enabled)
		return;

	printk("[STATE_NOTIFIER] Resume Called.\n");
	cancel_work_sync(&suspend_work);
	suspend_in_progress = false;

	if (state_suspended)
		schedule_work_on(0, &resume_work);
	else
		printk("[STATE_NOTIFIER] State change requested but unchanged - Ignored\n");
}

static int __init state_notifier_init(void)
{

	INIT_WORK(&suspend_work, _suspend_work);
	INIT_WORK(&resume_work, _resume_work);

	return 0;
}

subsys_initcall(state_notifier_init);

MODULE_AUTHOR("Pranav Vashi <neobuddy89@gmail.com>");
MODULE_DESCRIPTION("State Notifier Driver");
MODULE_LICENSE("GPLv2");
