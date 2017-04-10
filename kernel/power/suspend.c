/*
 * kernel/power/suspend.c - Suspend to RAM and standby functionality.
 *
 * Copyright (c) 2003 Patrick Mochel
 * Copyright (c) 2003 Open Source Development Lab
 * Copyright (c) 2009 Rafael J. Wysocki <rjw@sisk.pl>, Novell Inc.
 *
 * This file is released under the GPLv2.
 */

#include <linux/string.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/console.h>
#include <linux/cpu.h>
#include <linux/cpuidle.h>
#include <linux/syscalls.h>
#include <linux/gfp.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/export.h>
#include <linux/suspend.h>
#include <linux/syscore_ops.h>
#include <linux/rtc.h>
#include <linux/wakeup_reason.h>
#include <linux/partialresume.h>
#include <linux/ftrace.h>
#include <trace/events/power.h>
#include <linux/compiler.h>

#include "power.h"
#include <linux/mx_freeze.h>
bool mx_freezing_in_progress;
bool freezing_in_progress()
{
	return mx_freezing_in_progress;
}

static const char *pm_labels[] = { "mem", "standby", "freeze", };
const char *pm_states[PM_SUSPEND_MAX];

static const struct platform_suspend_ops *suspend_ops;
static const struct platform_freeze_ops *freeze_ops;

static DECLARE_WAIT_QUEUE_HEAD(suspend_freeze_wait_head);
static bool suspend_freeze_wake;

void freeze_set_ops(const struct platform_freeze_ops *ops)
{
	lock_system_sleep();
	freeze_ops = ops;
	unlock_system_sleep();
}

static void freeze_begin(void)
{
	suspend_freeze_wake = false;
}

static void freeze_enter(void)
{
	cpuidle_use_deepest_state(true);
	cpuidle_resume();
	wait_event(suspend_freeze_wait_head, suspend_freeze_wake);
	cpuidle_pause();
	cpuidle_use_deepest_state(false);
}

void freeze_wake(void)
{
	suspend_freeze_wake = true;
	wake_up(&suspend_freeze_wait_head);
}
EXPORT_SYMBOL_GPL(freeze_wake);

static bool valid_state(suspend_state_t state)
{
	/*
	 * PM_SUSPEND_STANDBY and PM_SUSPEND_MEM states need low level
	 * support and need to be valid to the low level
	 * implementation, no valid callback implies that none are valid.
	 */
	return suspend_ops && suspend_ops->valid && suspend_ops->valid(state);
}

/*
 * If this is set, the "mem" label always corresponds to the deepest sleep state
 * available, the "standby" label corresponds to the second deepest sleep state
 * available (if any), and the "freeze" label corresponds to the remaining
 * available sleep state (if there is one).
 */
static bool relative_states;

static int __init sleep_states_setup(char *str)
{
	relative_states = !strncmp(str, "1", 1);
	pm_states[PM_SUSPEND_FREEZE] = pm_labels[relative_states ? 0 : 2];
	return 1;
}

__setup("relative_sleep_states=", sleep_states_setup);

/**
 * suspend_set_ops - Set the global suspend method table.
 * @ops: Suspend operations to use.
 */
void suspend_set_ops(const struct platform_suspend_ops *ops)
{
	suspend_state_t i;
	int j = 0;

	lock_system_sleep();

	suspend_ops = ops;
	for (i = PM_SUSPEND_MEM; i >= PM_SUSPEND_STANDBY; i--)
		if (valid_state(i)) {
			pm_states[i] = pm_labels[j++];
		} else if (!relative_states) {
			pm_states[i] = NULL;
			j++;
		}

	pm_states[PM_SUSPEND_FREEZE] = pm_labels[j];

	unlock_system_sleep();
}
EXPORT_SYMBOL_GPL(suspend_set_ops);

/**
 * suspend_valid_only_mem - Generic memory-only valid callback.
 *
 * Platform drivers that implement mem suspend only and only need to check for
 * that in their .valid() callback can use this instead of rolling their own
 * .valid() callback.
 */
int suspend_valid_only_mem(suspend_state_t state)
{
	return state == PM_SUSPEND_MEM;
}
EXPORT_SYMBOL_GPL(suspend_valid_only_mem);

static bool sleep_state_supported(suspend_state_t state)
{
	return state == PM_SUSPEND_FREEZE || (suspend_ops && suspend_ops->enter);
}

static int platform_suspend_prepare(suspend_state_t state)
{
	return state != PM_SUSPEND_FREEZE && suspend_ops->prepare ?
		suspend_ops->prepare() : 0;
}

static int platform_suspend_prepare_late(suspend_state_t state)
{
	return state != PM_SUSPEND_FREEZE && suspend_ops->prepare_late ?
		suspend_ops->prepare_late() : 0;
}

static void platform_suspend_wake(suspend_state_t state)
{
	if (state != PM_SUSPEND_FREEZE && suspend_ops->wake)
		suspend_ops->wake();
}

static void platform_suspend_finish(suspend_state_t state)
{
	if (state != PM_SUSPEND_FREEZE && suspend_ops->finish)
		suspend_ops->finish();
}

static int platform_suspend_begin(suspend_state_t state)
{
	if (state == PM_SUSPEND_FREEZE && freeze_ops && freeze_ops->begin)
		return freeze_ops->begin();
	else if (suspend_ops->begin)
		return suspend_ops->begin(state);
	else
		return 0;
}

static void platform_suspend_end(suspend_state_t state)
{
	if (state == PM_SUSPEND_FREEZE && freeze_ops && freeze_ops->end)
		freeze_ops->end();
	else if (suspend_ops->end)
		suspend_ops->end();
}

static void platform_suspend_recover(suspend_state_t state)
{
	if (state != PM_SUSPEND_FREEZE && suspend_ops->recover)
		suspend_ops->recover();
}

static bool platform_suspend_again(void)
{
	int count;
	bool suspend = suspend_ops->suspend_again ?
		suspend_ops->suspend_again() : false;

	if (suspend) {
		/*
		 * pm_get_wakeup_count() gets an updated count of wakeup events
		 * that have occured and will return false (i.e. abort suspend)
		 * if a wakeup event has been started during suspend_again() and
		 * is still active. pm_save_wakeup_count() stores the count
		 * and enables pm_wakeup_pending() to properly analyze wakeup
		 * events before entering suspend in suspend_enter().
		 */
		suspend = pm_get_wakeup_count(&count, false) &&
			  pm_save_wakeup_count(count);

		if (!suspend)
			pr_debug("%s: wakeup occurred during suspend_again\n",
				__func__);
	}

	pr_debug("%s: votes for: %s\n", __func__,
		suspend ? "suspend" : "resume");

	return suspend;
}

#ifdef CONFIG_PM_DEBUG
static unsigned int pm_test_delay = 5;
module_param(pm_test_delay, uint, 0644);
MODULE_PARM_DESC(pm_test_delay,
		 "Number of seconds to wait before resuming from suspend test");
#endif

static int suspend_test(int level)
{
#ifdef CONFIG_PM_DEBUG
	if (pm_test_level == level) {
		pr_info("suspend debug: Waiting for %d second(s).\n",
				pm_test_delay);
		mdelay(pm_test_delay * 1000);
		return 1;
	}
#endif /* !CONFIG_PM_DEBUG */
	return 0;
}

/**
 * suspend_prepare - Prepare for entering system sleep state.
 *
 * Common code run for every system sleep state that can be entered (except for
 * hibernation).  Run suspend notifiers, allocate the "suspend" console and
 * freeze processes.
 */
static int suspend_prepare(suspend_state_t state)
{
	int error;

	if (!sleep_state_supported(state))
		return -EPERM;

	pm_prepare_console();

	error = pm_notifier_call_chain(PM_SUSPEND_PREPARE);
	if (error)
		goto Finish;

	error = suspend_freeze_processes();
	if (!error)
		return 0;
	log_suspend_abort_reason("One or more tasks refusing to freeze");
	suspend_stats.failed_freeze++;
	dpm_save_failed_step(SUSPEND_FREEZE);
 Finish:
	pm_notifier_call_chain(PM_POST_SUSPEND);
	pm_restore_console();
	return error;
}

/* default implementation */
void __weak arch_suspend_disable_irqs(void)
{
	local_irq_disable();
}

/* default implementation */
void __weak arch_suspend_enable_irqs(void)
{
	local_irq_enable();
}

/**
 * suspend_enter - Make the system enter the given sleep state.
 * @state: System sleep state to enter.
 * @wakeup: Returns information that the sleep state should not be re-entered.
 *
 * This function should be called after devices have been suspended.
 */
static int suspend_enter(suspend_state_t state, bool *wakeup)
{
	char suspend_abort[MAX_SUSPEND_ABORT_LEN];
	int error, last_dev;

	error = platform_suspend_prepare(state);
	if (error)
		goto Platform_finish;

	error = dpm_suspend_late(PMSG_SUSPEND);
	if (error) {
		last_dev = suspend_stats.last_failed_dev + REC_FAILED_NUM - 1;
		last_dev %= REC_FAILED_NUM;
		printk(KERN_ERR "PM: late suspend of devices failed\n");
		log_suspend_abort_reason("%s device failed to power down",
			suspend_stats.failed_devs[last_dev]);
		goto Platform_finish;
	}
	error = dpm_suspend_noirq(PMSG_SUSPEND);
	if (error) {
		printk(KERN_ERR "PM: noirq suspend of devices failed\n");
		goto Devices_early_resume;
	}

	error = platform_suspend_prepare_late(state);
	if (error)
		goto Platform_wake;

	if (suspend_test(TEST_PLATFORM))
		goto Platform_wake;

	/*
	 * PM_SUSPEND_FREEZE equals
	 * frozen processes + suspended devices + idle processors.
	 * Thus we should invoke freeze_enter() soon after
	 * all the devices are suspended.
	 */
	if (state == PM_SUSPEND_FREEZE) {
		freeze_enter();
		goto Platform_wake;
	}

	error = disable_nonboot_cpus();
	if (error || suspend_test(TEST_CPUS)) {
		log_suspend_abort_reason("Disabling non-boot cpus failed");
		goto Enable_cpus;
	}

	arch_suspend_disable_irqs();
	BUG_ON(!irqs_disabled());

	error = syscore_suspend();
	if (!error) {
		*wakeup = pm_wakeup_pending();
		if (!(suspend_test(TEST_CORE) || *wakeup)) {
			error = suspend_ops->enter(state);
			events_check_enabled = false;
		} else if (*wakeup) {
			pm_get_active_wakeup_sources(suspend_abort,
				MAX_SUSPEND_ABORT_LEN);
			log_suspend_abort_reason(suspend_abort);
			error = -EBUSY;
		}

		start_logging_wakeup_reasons();
		syscore_resume();
	}

	arch_suspend_enable_irqs();
	BUG_ON(irqs_disabled());

 Enable_cpus:
	enable_nonboot_cpus();

 Platform_wake:
	platform_suspend_wake(state);
	dpm_resume_noirq(PMSG_RESUME);

 Devices_early_resume:
	dpm_resume_early(PMSG_RESUME);

 Platform_finish:
	platform_suspend_finish(state);

	return error;
}

#ifdef CONFIG_PARTIALRESUME
static bool suspend_again(bool *drivers_resumed)
{
	const struct list_head *irqs;
	struct list_head unfinished;

	*drivers_resumed = false;

	/* If a platform suspend_again handler is defined, when it decides to
	 * not suspend again, this takes precedence over drivers.  If a
	 * platform's suspend_again callback returns true, then we proceed to
	 * check the drivers as well.
	 */
	if (suspend_ops->suspend_again && !suspend_ops->suspend_again())
		return false;

	/* TODO: resume only the drivers associated with the wakeup interrupts!
	 */
	dpm_resume_end(PMSG_RESUME);
	*drivers_resumed = true;

	/* Thaw kernel threads opportunistically, to allow get_wakeup_reasons
	 * to block while the wakeup interrupt list is being assembled.  Calls
	 * schedule() internally.
	 */
	thaw_kernel_threads();

	/* Look for a match between the wakeup reasons and the registered
	 * callbacks.  Don't bother thawing the kernel threads if a match is
	 * not found.
         */
	irqs = get_wakeup_reasons(msecs_to_jiffies(100), &unfinished);
	if (!suspend_again_match(irqs, &unfinished))
		return false;

	if (suspend_again_consensus() &&
		       !freeze_kernel_threads()) {
		clear_wakeup_reasons();
		*drivers_resumed = false;
		if (dpm_suspend_start(PMSG_SUSPEND)) {
			printk(KERN_ERR "PM: Some devices failed to suspend\n");
			log_suspend_abort_reason("Some devices failed to suspend");
			if (suspend_ops->recover)
				suspend_ops->recover();
			return false;
		}
		return true;
	}

	return false;
}
#else
static __always_inline bool
suspend_again(bool *drivers_resumed __attribute__((unused)))
{
	return suspend_ops->suspend_again && suspend_ops->suspend_again();
}
#endif /* CONFIG_PARTIALRESUME */

/**
 * suspend_devices_and_enter - Suspend devices and enter system sleep state.
 * @state: System sleep state to enter.
 */
int suspend_devices_and_enter(suspend_state_t state)
{
	int error;
	bool wakeup = false;
	bool resumed = false;

	if (!sleep_state_supported(state))
		return -ENOSYS;

	error = platform_suspend_begin(state);
	if (error)
		goto Close;

	suspend_console();
	suspend_test_start();
	error = dpm_suspend_start(PMSG_SUSPEND);
	if (error) {
		pr_err("PM: Some devices failed to suspend, or early wake event detected\n");
		log_suspend_abort_reason("Some devices failed to suspend, or early wake event detected");
		goto Recover_platform;
	}
	suspend_test_finish("suspend devices");
	if (suspend_test(TEST_DEVICES))
		goto Recover_platform;

	do {
		error = suspend_enter(state, &wakeup);
	} while (!error && !wakeup && suspend_again(&resumed));

 Resume_devices:
	suspend_test_start();
	if (!resumed)
		dpm_resume_end(PMSG_RESUME);
	suspend_test_finish("resume devices");
	resume_console();
 Close:
	platform_suspend_end(state);
	return error;

 Recover_platform:
	platform_suspend_recover(state);
	goto Resume_devices;
}

/**
 * suspend_finish - Clean up before finishing the suspend sequence.
 *
 * Call platform code to clean up, restart processes, and free the console that
 * we've allocated. This routine is not called for hibernation.
 */
static void suspend_finish(void)
{
	suspend_thaw_processes();
	pm_notifier_call_chain(PM_POST_SUSPEND);
	pm_restore_console();
}

/**
 * enter_state - Do common work needed to enter system sleep state.
 * @state: System sleep state to enter.
 *
 * Make sure that no one else is trying to put the system into a sleep state.
 * Fail if that's not the case.  Otherwise, prepare for system suspend, make the
 * system enter the given sleep state and clean up after wakeup.
 */
static int enter_state(suspend_state_t state)
{
	int error;

	if (state == PM_SUSPEND_FREEZE) {
#ifdef CONFIG_PM_DEBUG
		if (pm_test_level != TEST_NONE && pm_test_level <= TEST_CPUS) {
			pr_warn("PM: Unsupported test mode for suspend to idle, please choose none/freezer/devices/platform.\n");
			return -EAGAIN;
		}
#endif
	} else if (!valid_state(state)) {
		return -EINVAL;
	}
	if (!mutex_trylock(&pm_mutex))
		return -EBUSY;

	if (state == PM_SUSPEND_FREEZE)
		freeze_begin();

	if (suspendsync) {
		printk(KERN_INFO "PM: Syncing filesystems ... ");
		sys_sync();
		printk("done.\n");
	}

	pr_debug("PM: Preparing system for %s sleep\n", pm_states[state]);
	error = suspend_prepare(state);
	if (error)
		goto Unlock;

	if (suspend_test(TEST_FREEZER))
		goto Finish;

	pr_debug("PM: Entering %s sleep\n", pm_states[state]);
	pm_restrict_gfp_mask();
	error = suspend_devices_and_enter(state);
	pm_restore_gfp_mask();

 Finish:
	pr_debug("PM: Finishing wakeup.\n");
	suspend_finish();
 Unlock:
	mutex_unlock(&pm_mutex);
	return error;
}

static void pm_suspend_marker(char *annotation)
{
	struct timespec ts;
	struct rtc_time tm;

	getnstimeofday(&ts);
	rtc_time_to_tm(ts.tv_sec, &tm);
	pr_info("PM: suspend %s %d-%02d-%02d %02d:%02d:%02d.%09lu UTC\n",
		annotation, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec, ts.tv_nsec);
}

/**
 * pm_suspend - Externally visible function for suspending the system.
 * @state: System sleep state to enter.
 *
 * Check if the value of @state represents one of the supported states,
 * execute enter_state() and update system suspend statistics.
 */
int pm_suspend(suspend_state_t state)
{
	int error;

	if (state <= PM_SUSPEND_ON || state >= PM_SUSPEND_MAX)
		return -EINVAL;

	pm_suspend_marker("entry");
	error = enter_state(state);
	if (error) {
		suspend_stats.fail++;
		dpm_save_failed_errno(error);
	} else {
		suspend_stats.success++;
	}
	pm_suspend_marker("exit");
	return error;
}
EXPORT_SYMBOL(pm_suspend);
