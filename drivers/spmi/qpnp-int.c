/* Copyright (c) 2012, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define pr_fmt(fmt) "%s: " fmt, __func__

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/list.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/irqdomain.h>
#include <linux/interrupt.h>
#include <linux/spmi.h>
#include <linux/radix-tree.h>
#include <linux/slab.h>
#include <linux/printk.h>

#include <asm/irq.h>
#include <asm/mach/irq.h>
#include <mach/qpnp-int.h>

/* 16 slave_ids, 256 per_ids per slave, and 8 ints per per_id */
#define QPNPINT_NR_IRQS (16 * 256 * 8)

enum qpnpint_regs {
	QPNPINT_REG_RT_STS		= 0x10,
	QPNPINT_REG_SET_TYPE		= 0x11,
	QPNPINT_REG_POLARITY_HIGH	= 0x12,
	QPNPINT_REG_POLARITY_LOW	= 0x13,
	QPNPINT_REG_LATCHED_CLR		= 0x14,
	QPNPINT_REG_EN_SET		= 0x15,
	QPNPINT_REG_EN_CLR		= 0x16,
	QPNPINT_REG_LATCHED_STS		= 0x18,
};

struct q_perip_data {
	uint8_t type;	    /* bitmap */
	uint8_t pol_high;   /* bitmap */
	uint8_t pol_low;    /* bitmap */
	uint8_t int_en;     /* bitmap */
	uint8_t use_count;
};

struct q_irq_data {
	uint32_t priv_d; /* data to optimize arbiter interactions */
	struct q_chip_data *chip_d;
	struct q_perip_data *per_d;
	uint8_t mask_shift;
	uint8_t spmi_slave;
	uint16_t spmi_offset;
};

struct q_chip_data {
	int bus_nr;
	struct irq_domain *domain;
	struct qpnp_local_int cb;
	struct spmi_controller *spmi_ctrl;
	struct radix_tree_root per_tree;
	struct list_head list;
};

static LIST_HEAD(qpnpint_chips);
static DEFINE_MUTEX(qpnpint_chips_mutex);

#define QPNPINT_MAX_BUSSES 4
struct q_chip_data *chip_lookup[QPNPINT_MAX_BUSSES];

/**
 * qpnpint_encode_hwirq - translate between qpnp_irq_spec and
 *			  hwirq representation.
 *
 * slave_offset = (addr->slave * 256 * 8);
 * perip_offset = slave_offset + (addr->perip * 8);
 * return perip_offset + addr->irq;
 */
static inline int qpnpint_encode_hwirq(struct qpnp_irq_spec *spec)
{
	uint32_t hwirq;

	if (spec->slave > 15 || spec->irq > 7)
		return -EINVAL;

	hwirq = (spec->slave << 11);
	hwirq |= (spec->per << 3);
	hwirq |= spec->irq;

	return hwirq;
}
/**
 * qpnpint_decode_hwirq - translate between hwirq and
 *			  qpnp_irq_spec representation.
 */
static inline int qpnpint_decode_hwirq(unsigned long hwirq,
					struct qpnp_irq_spec *spec)
{
	if (hwirq > 65535)
		return -EINVAL;

	spec->slave = (hwirq >> 11) & 0xF;
	spec->per = (hwirq >> 3) & 0xFF;
	spec->irq = hwirq & 0x7;
	return 0;
}

static int qpnpint_spmi_write(struct q_irq_data *irq_d, uint8_t reg,
			      void *buf, uint32_t len)
{
	struct q_chip_data *chip_d = irq_d->chip_d;
	int rc;

	if (!chip_d->spmi_ctrl)
		return -ENODEV;

	rc = spmi_ext_register_writel(chip_d->spmi_ctrl, irq_d->spmi_slave,
				      irq_d->spmi_offset + reg, buf, len);
	return rc;
}

static void qpnpint_irq_mask(struct irq_data *d)
{
	struct q_irq_data *irq_d = irq_data_get_irq_chip_data(d);
	struct q_chip_data *chip_d = irq_d->chip_d;
	struct q_perip_data *per_d = irq_d->per_d;
	struct qpnp_irq_spec q_spec;
	int rc;

	pr_debug("hwirq %lu irq: %d\n", d->hwirq, d->irq);

	if (chip_d->cb.mask) {
		rc = qpnpint_decode_hwirq(d->hwirq, &q_spec);
		if (rc)
			pr_err("decode failed on hwirq %lu\n", d->hwirq);
		else
			chip_d->cb.mask(chip_d->spmi_ctrl, &q_spec,
								irq_d->priv_d);
	}

	per_d->int_en &= ~irq_d->mask_shift;

	rc = qpnpint_spmi_write(irq_d, QPNPINT_REG_EN_CLR,
					(u8 *)&irq_d->mask_shift, 1);
	if (rc)
		pr_err("spmi failure on irq %d\n", d->irq);
}

static void qpnpint_irq_mask_ack(struct irq_data *d)
{
	struct q_irq_data *irq_d = irq_data_get_irq_chip_data(d);
	struct q_chip_data *chip_d = irq_d->chip_d;
	struct q_perip_data *per_d = irq_d->per_d;
	struct qpnp_irq_spec q_spec;
	int rc;

	pr_debug("hwirq %lu irq: %d mask: 0x%x\n", d->hwirq, d->irq,
							irq_d->mask_shift);

	if (chip_d->cb.mask) {
		rc = qpnpint_decode_hwirq(d->hwirq, &q_spec);
		if (rc)
			pr_err("decode failed on hwirq %lu\n", d->hwirq);
		else
			chip_d->cb.mask(chip_d->spmi_ctrl, &q_spec,
								irq_d->priv_d);
	}

	per_d->int_en &= ~irq_d->mask_shift;

	rc = qpnpint_spmi_write(irq_d, QPNPINT_REG_EN_CLR,
							&irq_d->mask_shift, 1);
	if (rc)
		pr_err("spmi failure on irq %d\n", d->irq);

	rc = qpnpint_spmi_write(irq_d, QPNPINT_REG_LATCHED_CLR,
							&irq_d->mask_shift, 1);
	if (rc)
		pr_err("spmi failure on irq %d\n", d->irq);
}

static void qpnpint_irq_unmask(struct irq_data *d)
{
	struct q_irq_data *irq_d = irq_data_get_irq_chip_data(d);
	struct q_chip_data *chip_d = irq_d->chip_d;
	struct q_perip_data *per_d = irq_d->per_d;
	struct qpnp_irq_spec q_spec;
	int rc;

	pr_debug("hwirq %lu irq: %d\n", d->hwirq, d->irq);

	if (chip_d->cb.unmask) {
		rc = qpnpint_decode_hwirq(d->hwirq, &q_spec);
		if (rc)
			pr_err("decode failed on hwirq %lu\n", d->hwirq);
		else
			chip_d->cb.unmask(chip_d->spmi_ctrl, &q_spec,
								irq_d->priv_d);
	}

	per_d->int_en |= irq_d->mask_shift;
	rc = qpnpint_spmi_write(irq_d, QPNPINT_REG_EN_SET,
					&irq_d->mask_shift, 1);
	if (rc)
		pr_err("spmi failure on irq %d\n", d->irq);
}

static int qpnpint_irq_set_type(struct irq_data *d, unsigned int flow_type)
{
	struct q_irq_data *irq_d = irq_data_get_irq_chip_data(d);
	struct q_perip_data *per_d = irq_d->per_d;
	int rc;
	u8 buf[3];

	pr_debug("hwirq %lu irq: %d flow: 0x%x\n", d->hwirq,
							d->irq, flow_type);

	per_d->pol_high &= ~irq_d->mask_shift;
	per_d->pol_low &= ~irq_d->mask_shift;
	if (flow_type & (IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING)) {
		per_d->type |= irq_d->mask_shift; /* edge trig */
		if (flow_type & IRQF_TRIGGER_RISING)
			per_d->pol_high |= irq_d->mask_shift;
		if (flow_type & IRQF_TRIGGER_FALLING)
			per_d->pol_low |= irq_d->mask_shift;
	} else {
		if ((flow_type & IRQF_TRIGGER_HIGH) &&
		    (flow_type & IRQF_TRIGGER_LOW))
			return -EINVAL;
		per_d->type &= ~irq_d->mask_shift; /* level trig */
		if (flow_type & IRQF_TRIGGER_HIGH)
			per_d->pol_high |= irq_d->mask_shift;
		else
			per_d->pol_low |= irq_d->mask_shift;
	}

	buf[0] = per_d->type;
	buf[1] = per_d->pol_high;
	buf[2] = per_d->pol_low;

	rc = qpnpint_spmi_write(irq_d, QPNPINT_REG_SET_TYPE, &buf, 3);
	if (rc)
		pr_err("spmi failure on irq %d\n", d->irq);
	return rc;
}

static int qpnpint_irq_set_wake(struct irq_data *d, unsigned int on)
{
	return 0;
}

static struct irq_chip qpnpint_chip = {
	.name		= "qpnp-int",
	.irq_mask	= qpnpint_irq_mask,
	.irq_mask_ack	= qpnpint_irq_mask_ack,
	.irq_unmask	= qpnpint_irq_unmask,
	.irq_set_type	= qpnpint_irq_set_type,
	.irq_set_wake	= qpnpint_irq_set_wake,
	.flags		= IRQCHIP_MASK_ON_SUSPEND,
};

static int qpnpint_init_irq_data(struct q_chip_data *chip_d,
				 struct q_irq_data *irq_d,
				 unsigned long hwirq)
{
	struct qpnp_irq_spec q_spec;
	int rc;

	irq_d->mask_shift = 1 << (hwirq & 0x7);
	rc = qpnpint_decode_hwirq(hwirq, &q_spec);
	if (rc < 0)
		return rc;
	irq_d->spmi_slave = q_spec.slave;
	irq_d->spmi_offset = q_spec.per << 8;
	irq_d->chip_d = chip_d;

	if (chip_d->cb.register_priv_data)
		rc = chip_d->cb.register_priv_data(chip_d->spmi_ctrl, &q_spec,
							&irq_d->priv_d);
		if (rc)
			return rc;

	irq_d->per_d->use_count++;
	return 0;
}

static struct q_irq_data *qpnpint_alloc_irq_data(
					struct q_chip_data *chip_d,
					unsigned long hwirq)
{
	struct q_irq_data *irq_d;
	struct q_perip_data *per_d;

	irq_d = kzalloc(sizeof(struct q_irq_data), GFP_KERNEL);
	if (!irq_d)
		return ERR_PTR(-ENOMEM);

	/**
	 * The Peripheral Tree is keyed from the slave + per_id. We're
	 * ignoring the irq bits here since this peripheral structure
	 * should be common for all irqs on the same peripheral.
	 */
	per_d = radix_tree_lookup(&chip_d->per_tree, (hwirq & ~0x7));
	if (!per_d) {
		per_d = kzalloc(sizeof(struct q_perip_data), GFP_KERNEL);
		if (!per_d) {
			kfree(irq_d);
			return ERR_PTR(-ENOMEM);
		}
		radix_tree_insert(&chip_d->per_tree,
				  (hwirq & ~0x7), per_d);
	}
	irq_d->per_d = per_d;

	return irq_d;
}

static int qpnpint_irq_domain_dt_translate(struct irq_domain *d,
				       struct device_node *controller,
				       const u32 *intspec, unsigned int intsize,
				       unsigned long *out_hwirq,
				       unsigned int *out_type)
{
	struct qpnp_irq_spec addr;
	int ret;

	pr_debug("intspec[0] 0x%x intspec[1] 0x%x intspec[2] 0x%x\n",
				intspec[0], intspec[1], intspec[2]);

	if (d->of_node != controller)
		return -EINVAL;
	if (intsize != 3)
		return -EINVAL;

	addr.irq = intspec[2] & 0x7;
	addr.per = intspec[1] & 0xFF;
	addr.slave = intspec[0] & 0xF;

	ret = qpnpint_encode_hwirq(&addr);
	if (ret < 0) {
		pr_err("invalid intspec\n");
		return ret;
	}
	*out_hwirq = ret;
	*out_type = IRQ_TYPE_NONE;

	return 0;
}

static void qpnpint_free_irq_data(struct q_irq_data *irq_d)
{
	if (irq_d->per_d->use_count == 1)
		kfree(irq_d->per_d);
	else
		irq_d->per_d->use_count--;
	kfree(irq_d);
}

static int qpnpint_irq_domain_map(struct irq_domain *d,
				  unsigned int virq, irq_hw_number_t hwirq)
{
	struct q_chip_data *chip_d = d->host_data;
	struct q_irq_data *irq_d;
	int rc;

	pr_debug("hwirq = %lu\n", hwirq);

	if (hwirq < 0 || hwirq >= 32768) {
		pr_err("hwirq %lu out of bounds\n", hwirq);
		return -EINVAL;
	}

	irq_radix_revmap_insert(d, virq, hwirq);

	irq_d = qpnpint_alloc_irq_data(chip_d, hwirq);
	if (IS_ERR(irq_d)) {
		pr_err("failed to alloc irq data for hwirq %lu\n", hwirq);
		return PTR_ERR(irq_d);
	}

	rc = qpnpint_init_irq_data(chip_d, irq_d, hwirq);
	if (rc) {
		pr_err("failed to init irq data for hwirq %lu\n", hwirq);
		goto map_err;
	}

	irq_set_chip_and_handler(virq,
			&qpnpint_chip,
			handle_level_irq);
	irq_set_chip_data(virq, irq_d);
#ifdef CONFIG_ARM
	set_irq_flags(virq, IRQF_VALID);
#else
	irq_set_noprobe(virq);
#endif
	return 0;

map_err:
	qpnpint_free_irq_data(irq_d);
	return rc;
}

void qpnpint_irq_domain_unmap(struct irq_domain *d, unsigned int virq)
{
	struct q_irq_data *irq_d = irq_get_chip_data(virq);

	if (WARN_ON(!irq_d))
		return;

	qpnpint_free_irq_data(irq_d);
}

const struct irq_domain_ops qpnpint_irq_domain_ops = {
	.map = qpnpint_irq_domain_map,
	.unmap = qpnpint_irq_domain_unmap,
	.xlate = qpnpint_irq_domain_dt_translate,
};

int qpnpint_register_controller(struct device_node *node,
				struct spmi_controller *ctrl,
				struct qpnp_local_int *li_cb)
{
	struct q_chip_data *chip_d;

	if (!node || !ctrl || ctrl->nr >= QPNPINT_MAX_BUSSES)
		return -EINVAL;

	list_for_each_entry(chip_d, &qpnpint_chips, list)
		if (node == chip_d->domain->of_node) {
			chip_d->cb = *li_cb;
			chip_d->spmi_ctrl = ctrl;
			chip_lookup[ctrl->nr] = chip_d;
			return 0;
		}

	return -ENOENT;
}
EXPORT_SYMBOL(qpnpint_register_controller);

int qpnpint_handle_irq(struct spmi_controller *spmi_ctrl,
		       struct qpnp_irq_spec *spec)
{
	struct irq_domain *domain;
	unsigned long hwirq, busno;
	int irq;

	if (!spec || !spmi_ctrl)
		return -EINVAL;

	pr_debug("spec slave = %u per = %u irq = %u\n",
					spec->slave, spec->per, spec->irq);

	busno = spmi_ctrl->nr;
	if (busno >= QPNPINT_MAX_BUSSES)
		return -EINVAL;

	hwirq = qpnpint_encode_hwirq(spec);
	if (hwirq < 0) {
		pr_err("invalid irq spec passed\n");
		return -EINVAL;
	}

	domain = chip_lookup[busno]->domain;
	irq = irq_radix_revmap_lookup(domain, hwirq);

	generic_handle_irq(irq);

	return 0;
}
EXPORT_SYMBOL(qpnpint_handle_irq);

int __init qpnpint_of_init(struct device_node *node, struct device_node *parent)
{
	struct q_chip_data *chip_d;

	chip_d = kzalloc(sizeof(struct q_chip_data), GFP_KERNEL);
	if (!chip_d)
		return -ENOMEM;

	chip_d->domain = irq_domain_add_tree(node,
					&qpnpint_irq_domain_ops, chip_d);
	if (!chip_d->domain) {
		pr_err("Unable to allocate irq_domain\n");
		kfree(chip_d);
		return -ENOMEM;
	}

	INIT_RADIX_TREE(&chip_d->per_tree, GFP_ATOMIC);
	list_add(&chip_d->list, &qpnpint_chips);

	return 0;
}
EXPORT_SYMBOL(qpnpint_of_init);
