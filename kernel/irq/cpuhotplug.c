/*
 * Generic cpu hotunplug interrupt migration code copied from the
 * arch/arm implementation
 *
 * Copyright (C) Russell King
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/interrupt.h>
#include <linux/ratelimit.h>
#include <linux/irq.h>

#include "internals.h"

static bool migrate_one_irq(struct irq_desc *desc)
{
	struct irq_data *d = irq_desc_get_irq_data(desc);
	const struct cpumask *affinity = d->common->affinity;
	struct irq_chip *c;
	bool ret = false;

	/*
	 * If this is a per-CPU interrupt, or the affinity does not
	 * include this CPU, then we have nothing to do.
	 */
	if (irqd_is_per_cpu(d) ||
	    !cpumask_test_cpu(smp_processor_id(), affinity))
		return false;

	if (cpumask_any_and(affinity, cpu_online_mask) >= nr_cpu_ids) {
		/*
		 * If the interrupt is managed, then shut it down and leave
		 * the affinity untouched.
		 */
		if (irqd_affinity_is_managed(d)) {
			irqd_set_managed_shutdown(d);
			irq_shutdown(desc);
			return false;
		}
		affinity = cpu_online_mask;
		ret = true;
	}

	c = irq_data_get_irq_chip(d);
	if (!c->irq_set_affinity) {
		pr_debug("IRQ%u: unable to set affinity\n", d->irq);
	} else {
		int r = irq_do_set_affinity(d, affinity, false);
		if (r)
			pr_warn_ratelimited("IRQ%u: set affinity failed(%d).\n",
					    d->irq, r);
	}

	return ret;
}

/**
 * irq_migrate_all_off_this_cpu - Migrate irqs away from offline cpu
 *
 * The current CPU has been marked offline.  Migrate IRQs off this CPU.
 * If the affinity settings do not allow other CPUs, force them onto any
 * available CPU.
 *
 * Note: we must iterate over all IRQs, whether they have an attached
 * action structure or not, as we need to get chained interrupts too.
 */
void irq_migrate_all_off_this_cpu(void)
{
	unsigned int irq;
	struct irq_desc *desc;
	unsigned long flags;

	local_irq_save(flags);

	for_each_active_irq(irq) {
		bool affinity_broken;

		desc = irq_to_desc(irq);
		raw_spin_lock(&desc->lock);
		affinity_broken = migrate_one_irq(desc);
		raw_spin_unlock(&desc->lock);

		if (affinity_broken)
			pr_warn_ratelimited("IRQ%u no longer affine to CPU%u\n",
					    irq, smp_processor_id());
	}

	local_irq_restore(flags);
}

static void irq_restore_affinity_of_irq(struct irq_desc *desc, unsigned int cpu)
{
	struct irq_data *data = irq_desc_get_irq_data(desc);
	const struct cpumask *affinity = irq_data_get_affinity_mask(data);

	if (!irqd_affinity_is_managed(data) || !desc->action ||
	    !irq_data_get_irq_chip(data) || !cpumask_test_cpu(cpu, affinity))
		return;

	if (irqd_is_managed_and_shutdown(data))
		irq_startup(desc, IRQ_RESEND, IRQ_START_COND);
	else
		irq_set_affinity_locked(data, affinity, false);
}

/**
 * irq_affinity_online_cpu - Restore affinity for managed interrupts
 * @cpu:	Upcoming CPU for which interrupts should be restored
 */
int irq_affinity_online_cpu(unsigned int cpu)
{
	struct irq_desc *desc;
	unsigned int irq;

	irq_lock_sparse();
	for_each_active_irq(irq) {
		desc = irq_to_desc(irq);
		raw_spin_lock_irq(&desc->lock);
		irq_restore_affinity_of_irq(desc, cpu);
		raw_spin_unlock_irq(&desc->lock);
	}
	irq_unlock_sparse();

	return 0;
}
