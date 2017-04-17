/*
 * linux/kernel/irq/pm.c
 *
 * Copyright (C) 2009 Rafael J. Wysocki <rjw@sisk.pl>, Novell Inc.
 *
 * This file contains power management functions related to interrupts.
 */

#include <linux/irq.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/syscore_ops.h>
#include <linux/wakeup_reason.h>

#include "internals.h"

/*
 * Called from __setup_irq() with desc->lock held after @action has
 * been installed in the action chain.
 */
void irq_pm_install_action(struct irq_desc *desc, struct irqaction *action)
{
	desc->nr_actions++;

	if (action->flags & IRQF_FORCE_RESUME)
		desc->force_resume_depth++;

	WARN_ON_ONCE(desc->force_resume_depth &&
		     desc->force_resume_depth != desc->nr_actions);

	if (action->flags & IRQF_NO_SUSPEND)
		desc->no_suspend_depth++;

	WARN_ON_ONCE(desc->no_suspend_depth &&
		     desc->no_suspend_depth != desc->nr_actions);
}

/*
 * Called from __free_irq() with desc->lock held after @action has
 * been removed from the action chain.
 */
void irq_pm_remove_action(struct irq_desc *desc, struct irqaction *action)
{
	desc->nr_actions--;

	if (action->flags & IRQF_FORCE_RESUME)
		desc->force_resume_depth--;

	if (action->flags & IRQF_NO_SUSPEND)
		desc->no_suspend_depth--;
}

static void suspend_irq(struct irq_desc *desc, int irq)
{
	struct irqaction *action = desc->action;
	unsigned int no_suspend, flags;

	if (!action)
		return false;

	no_suspend = IRQF_NO_SUSPEND;
	flags = 0;
	do {
		no_suspend &= action->flags;
		flags |= action->flags;
		action = action->next;
	} while (action);
	if (no_suspend)
		return;

	if (irqd_is_wakeup_set(&desc->irq_data))
		irqd_set(&desc->irq_data, IRQD_WAKEUP_ARMED);

	desc->istate |= IRQS_SUSPENDED;

	if ((flags & IRQF_NO_SUSPEND) &&
	    !(desc->istate & IRQS_SPURIOUS_DISABLED)) {
		struct irqaction *active = NULL;
		struct irqaction *suspended = NULL;
		struct irqaction *head = desc->action;

		do {
			action = head;
			head = action->next;
			if (action->flags & IRQF_NO_SUSPEND) {
				action->next = active;
				active = action;
			} else {
				action->next = suspended;
				suspended = action;
			}
		} while (head);
		desc->action = active;
		desc->action_suspended = suspended;
		return;
	}
	__disable_irq(desc, irq);

	/*
	 * Hardware which has no wakeup source configuration facility
	 * requires that the non wakeup interrupts are masked at the
	 * chip level. The chip implementation indicates that with
	 * IRQCHIP_MASK_ON_SUSPEND.
	 */
	if (irq_desc_get_chip(desc)->flags & IRQCHIP_MASK_ON_SUSPEND)
		mask_irq(desc);
}

/**
 * suspend_device_irqs - disable all currently enabled interrupt lines
 *
 * During system-wide suspend or hibernation device drivers need to be prevented
 * from receiving interrupts and this function is provided for this purpose.
 * It marks all interrupt lines in use, except for the timer ones, as disabled
 * and sets the IRQS_SUSPENDED flag for each of them.
 */
void suspend_device_irqs(void)
{
	struct irq_desc *desc;
	int irq;

	for_each_irq_desc(irq, desc) {
		unsigned long flags;
		bool sync;

		raw_spin_lock_irqsave(&desc->lock, flags);
		sync = suspend_irq(desc, irq);
		raw_spin_unlock_irqrestore(&desc->lock, flags);
	}

	if (sync)
		synchronize_irq(irq);
}
EXPORT_SYMBOL_GPL(suspend_device_irqs);

static void resume_irq(struct irq_desc *desc, int irq)
{
	irqd_clear(&desc->irq_data, IRQD_WAKEUP_ARMED);
	if (desc->istate & IRQS_SUSPENDED) {
		desc->istate &= ~IRQS_SUSPENDED;
		if (desc->action_suspended) {
			struct irqaction *action = desc->action;

			while (action->next)
				action = action->next;

			action->next = desc->action_suspended;
			desc->action_suspended = NULL;

			if (desc->istate & IRQS_SPURIOUS_DISABLED) {
				pr_err("Re-enabling emergency disabled IRQ %d\n",
				       irq);
				desc->istate &= ~IRQS_SPURIOUS_DISABLED;
			} else {
				return;
			}
		}
	} else {
		if (!desc->action)
			return;

		if (!(desc->action->flags & IRQF_FORCE_RESUME))
			return;

		/* Pretend that it got disabled ! */
		desc->depth++;
	}
	__enable_irq(desc, irq);
}

static void resume_irqs(bool want_early)
{
	struct irq_desc *desc;
	int irq;

	for_each_irq_desc_reverse(irq, desc) {
		unsigned long flags;
		bool is_early = desc->action &&
			desc->action->flags & IRQF_EARLY_RESUME;

		if (!is_early && want_early)
			continue;

		raw_spin_lock_irqsave(&desc->lock, flags);
		resume_irq(desc, irq);
		raw_spin_unlock_irqrestore(&desc->lock, flags);
	}
}

/**
 * irq_pm_syscore_ops - enable interrupt lines early
 *
 * Enable all interrupt lines with %IRQF_EARLY_RESUME set.
 */
static void irq_pm_syscore_resume(void)
{
	resume_irqs(true);
}

static struct syscore_ops irq_pm_syscore_ops = {
	.resume		= irq_pm_syscore_resume,
};

static int __init irq_pm_init_ops(void)
{
	register_syscore_ops(&irq_pm_syscore_ops);
	return 0;
}

device_initcall(irq_pm_init_ops);

/**
 * resume_device_irqs - enable interrupt lines disabled by suspend_device_irqs()
 *
 * Enable all non-%IRQF_EARLY_RESUME interrupt lines previously
 * disabled by suspend_device_irqs() that have the IRQS_SUSPENDED flag
 * set as well as those with %IRQF_FORCE_RESUME.
 */
void resume_device_irqs(void)
{
	resume_irqs(false);
}
EXPORT_SYMBOL_GPL(resume_device_irqs);

/**
 * check_wakeup_irqs - check if any wake-up interrupts are pending
 */
int check_wakeup_irqs(void)
{
	struct irq_desc *desc;
	/* char suspend_abort[MAX_SUSPEND_ABORT_LEN]; */
	int irq;

	for_each_irq_desc(irq, desc) {
		if (irqd_is_wakeup_set(&desc->irq_data)) {
			if (desc->istate & IRQS_PENDING) {
				log_suspend_abort_reason("Wakeup IRQ %d %s pending",
					irq,
					desc->action && desc->action->name ?
					desc->action->name : "");
				pr_info("Wakeup IRQ %d %s pending, suspend aborted\n",
					irq,
					desc->action && desc->action->name ?
					desc->action->name : "");
				return -EBUSY;
			}
		}
	}

	return 0;
}
