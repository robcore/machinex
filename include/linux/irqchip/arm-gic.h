/*
 *  include/linux/irqchip/arm-gic.h
 *
 *  Copyright (C) 2002 ARM Limited, All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __LINUX_IRQCHIP_ARM_GIC_H
#define __LINUX_IRQCHIP_ARM_GIC_H

#define GIC_CPU_CTRL			0x00
#define GIC_CPU_PRIMASK			0x04
#define GIC_CPU_BINPOINT		0x08
#define GIC_CPU_INTACK			0x0c
#define GIC_CPU_EOI			0x10
#define GIC_CPU_RUNNINGPRI		0x14
#define GIC_CPU_HIGHPRI			0x18

#define GICC_IAR_INT_ID_MASK		0x3ff

#define GIC_DIST_CTRL			0x000
#define GIC_DIST_CTR			0x004
#define GIC_DIST_ISR			0x080
#define GIC_DIST_ENABLE_SET		0x100
#define GIC_DIST_ENABLE_CLEAR		0x180
#define GIC_DIST_PENDING_SET		0x200
#define GIC_DIST_PENDING_CLEAR		0x280
#define GIC_DIST_ACTIVE_BIT		0x300
#define GIC_DIST_PRI			0x400
#define GIC_DIST_TARGET			0x800
#define GIC_DIST_CONFIG			0xc00
#define GIC_DIST_SOFTINT		0xf00
#define GIC_DIST_SGI_PENDING_CLEAR	0xf10
#define GIC_DIST_SGI_PENDING_SET	0xf20

struct device_node;

extern struct irq_chip gic_arch_extn;

void gic_init_bases(unsigned int, int, void __iomem *, void __iomem *,
		    u32 offset, struct device_node *);
int gic_of_init(struct device_node *node, struct device_node *parent);
void gic_handle_irq(struct pt_regs *regs);
void gic_cascade_irq(unsigned int gic_nr, unsigned int irq);
void gic_cpu_if_down(void);
bool gic_is_irq_pending(unsigned int irq);
void gic_clear_irq_pending(unsigned int irq);
#ifdef CONFIG_ARM_GIC
void gic_set_irq_secure(unsigned int irq);
#else
static inline void gic_set_irq_secure(unsigned int irq) { }
#endif
static inline void gic_init(unsigned int nr, int start,
			    void __iomem *dist , void __iomem *cpu)
{
	gic_init_bases(nr, start, dist, cpu, 0, NULL);
}
void gic_set_irq_secure(unsigned int irq);

void msm_gic_save(bool modem_wake, int from_idle);
void msm_gic_restore(void);
void gic_configure_and_raise(unsigned int irq, unsigned int cpu);
void gic_migrate_target(unsigned int new_cpu_id);
unsigned long gic_get_sgir_physaddr(void);
extern const struct irq_domain_ops *gic_routable_irq_domain_ops;
static inline void __init register_routable_domain_ops
					(const struct irq_domain_ops *ops)
{
	gic_routable_irq_domain_ops = ops;
}
#endif
