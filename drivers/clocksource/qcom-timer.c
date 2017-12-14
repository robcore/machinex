/*
 *
 * Copyright (C) 2007 Google, Inc.
 * Copyright (c) 2009-2012,2014, The Linux Foundation. All rights reserved.
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

#include <linux/clocksource.h>
#include <linux/clockchips.h>
#include <linux/cpu.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/sched_clock.h>
#include <asm/user_accessible_timer.h>
#include <linux/mm.h>

#include <asm/delay.h>

#define TIMER_MATCH_VAL			0x0000
#define TIMER_COUNT_VAL			0x0004
#define TIMER_ENABLE			0x0008
#define TIMER_ENABLE_CLR_ON_MATCH_EN	BIT(1)
#define TIMER_ENABLE_EN			BIT(0)
#define TIMER_CLEAR			0x000C
#define DGT_CLK_CTL			0x34
#define DGT_CLK_CTL_DIV_4		0x3
#define TIMER_STS_GPT0_CLR_PEND		BIT(10)

#define GPT_HZ 32768
#define MSM_DGT_SHIFT 5

static void __iomem *event_base;
static void __iomem *sts_base;

static irqreturn_t msm_timer_interrupt(int irq, void *dev_id)
{
	struct clock_event_device *evt = dev_id;
	/* Stop the timer tick */
	if (clockevent_state_oneshot(evt)) {
		u32 ctrl = readl_relaxed(event_base + TIMER_ENABLE);
		ctrl &= ~TIMER_ENABLE_EN;
		writel_relaxed(ctrl, event_base + TIMER_ENABLE);
	}
	evt->event_handler(evt);
	return IRQ_HANDLED;
}

static int msm_timer_set_next_event(unsigned long cycles,
				    struct clock_event_device *evt)
{
	u32 ctrl = readl_relaxed(event_base + TIMER_ENABLE);

	ctrl &= ~TIMER_ENABLE_EN;
	writel_relaxed(ctrl, event_base + TIMER_ENABLE);

	writel_relaxed(ctrl, event_base + TIMER_CLEAR);
	writel_relaxed(cycles, event_base + TIMER_MATCH_VAL);

	if (sts_base)
		while (readl_relaxed(sts_base) & TIMER_STS_GPT0_CLR_PEND)
			cpu_relax();

	writel_relaxed(ctrl | TIMER_ENABLE_EN, event_base + TIMER_ENABLE);
	return 0;
}

static int msm_timer_shutdown(struct clock_event_device *evt)
{
	u32 ctrl;

	ctrl = readl_relaxed(event_base + TIMER_ENABLE);
	ctrl &= ~(TIMER_ENABLE_EN | TIMER_ENABLE_CLR_ON_MATCH_EN);
	writel_relaxed(ctrl, event_base + TIMER_ENABLE);
	return 0;
}

static struct clock_event_device __percpu *msm_evt;

static void __iomem *source_base;

static notrace u64 msm_read_timer_count(struct clocksource *cs)
{
	return readl_relaxed(source_base + TIMER_COUNT_VAL);
}

static struct clocksource msm_clocksource = {
	.name	= "dg_timer",
	.rating	= 100,
	.read	= msm_read_timer_count,
	.mask	= CLOCKSOURCE_MASK(32),
	.flags	= CLOCK_SOURCE_IS_CONTINUOUS,
};

static int msm_timer_irq;
static int msm_timer_has_ppi;

static int msm_local_timer_starting_cpu(unsigned int cpu)
{
	struct clock_event_device *evt = per_cpu_ptr(msm_evt, cpu);
	int err;

	evt->irq = msm_timer_irq;
	evt->name = "msm_timer";
	evt->features = CLOCK_EVT_FEAT_ONESHOT;
	evt->rating = 200;
	evt->set_state_shutdown = msm_timer_shutdown;
	evt->set_state_oneshot = msm_timer_shutdown;
	evt->tick_resume = msm_timer_shutdown;
	evt->set_next_event = msm_timer_set_next_event;
	evt->cpumask = cpumask_of(cpu);

	clockevents_config_and_register(evt, GPT_HZ, 4, 0xf0000000 >> 32);

	if (msm_timer_has_ppi) {
		enable_percpu_irq(evt->irq, IRQ_TYPE_EDGE_RISING);
	} else {
		err = request_irq(evt->irq, msm_timer_interrupt,
				IRQF_TIMER | IRQF_NOBALANCING |
				IRQF_TRIGGER_RISING, "gp_timer", evt);
		if (err)
			pr_err("request_irq failed\n");
	}

	return 0;
}

static int msm_local_timer_dying_cpu(unsigned int cpu)
{
	struct clock_event_device *evt = per_cpu_ptr(msm_evt, cpu);

	evt->set_state_shutdown(evt);
	disable_percpu_irq(evt->irq);
	return 0;
}

static u64 notrace msm_sched_clock_read(void)
{
	return msm_clocksource.read(&msm_clocksource);
}

static unsigned long msm_read_current_timer(void)
{
	return msm_clocksource.read(&msm_clocksource);
}

static struct delay_timer msm_delay_timer = {
	.read_current_timer = msm_read_current_timer,
};

static int __init msm_timer_init(u32 dgt_hz, int sched_bits, int irq,
				  bool percpu)
{
	struct clocksource *cs = &msm_clocksource;
	int res = 0;
	void __iomem *addr;

	msm_timer_irq = irq;
	msm_timer_has_ppi = percpu;

	msm_evt = alloc_percpu(struct clock_event_device);
	if (!msm_evt) {
		pr_err("memory allocation failed for clockevents\n");
		goto err;
	}

	if (percpu)
		res = request_percpu_irq(irq, msm_timer_interrupt,
					 "gp_timer", msm_evt);

	if (res) {
		pr_err("request_percpu_irq failed\n");
	} else {
		/* Install and invoke hotplug callbacks */
		res = cpuhp_setup_state(CPUHP_AP_QCOM_TIMER_STARTING,
					"clockevents/qcom/timer:starting",
					msm_local_timer_starting_cpu,
					msm_local_timer_dying_cpu);
		if (res) {
			free_percpu_irq(irq, msm_evt);
			goto err;
		}
	}

err:
	writel_relaxed(TIMER_ENABLE_EN, source_base + TIMER_ENABLE);
	res = clocksource_register_hz(cs, dgt_hz);
	if (res)
		pr_err("clocksource_register failed\n");
	sched_clock_register(msm_sched_clock_read, sched_bits, dgt_hz);
	msm_delay_timer.freq = dgt_hz;
	register_current_timer_delay(&msm_delay_timer);

	if (use_user_accessible_timers()) {
		addr = event_base + 0x1000;
		setup_user_timer_offset(virt_to_phys(addr)&0xfff);
		set_user_accessible_timer_flag(true);
	}

	return res;
}

static int __init msm_timer_map(phys_addr_t addr, u32 event, u32 source,
				u32 sts)
{
	void __iomem *base;
 
	base = ioremap(addr, SZ_256);
	if (!base) {
		pr_err("Failed to map timer base\n");
		return -ENOMEM;
	}
	event_base = base + event;
	source_base = base + source;
	if (sts)
		sts_base = base + sts;
 
	return 0;
}
 
static notrace cycle_t msm_read_timer_count_shift(struct clocksource *cs)
{
	/*
	 * Shift timer count down by a constant due to unreliable lower bits
	 * on some targets.
	 */
	return msm_read_timer_count(cs) >> MSM_DGT_SHIFT;
}

void __init jf_timer_init(void)
{
	struct clocksource *cs = &msm_clocksource;
	BUG_ON(msm_timer_map(0x0200a000, 0x00000004, 0x00000024, 0x00000088));
		return;

	writel_relaxed(DGT_CLK_CTL_DIV_4, event_base + DGT_CLK_CTL);
	msm_timer_init(6750000, 32, 18, true);
}