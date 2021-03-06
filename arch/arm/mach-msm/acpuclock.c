/* Copyright (c) 2011-2012, The Linux Foundation. All rights reserved.
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
#include <linux/export.h>
#include <linux/cpu.h>
#include <linux/smp.h>
#include "acpuclock.h"
#include <linux/cpufreq.h>
#include <trace/events/power.h>

static struct acpuclk_data *acpuclk_data;

unsigned long acpuclk_get_rate(unsigned int cpu)
{
	if (!acpuclk_data || !acpuclk_data->get_rate)
		return 0;

	return acpuclk_data->get_rate(cpu);
}
EXPORT_SYMBOL(acpuclk_get_rate);

int acpuclk_set_rate(unsigned int cpu, unsigned long rate, enum setrate_reason reason)
{
	if (!acpuclk_data || (acpuclk_data->set_rate < 0))
		return 0;

	return acpuclk_data->set_rate(cpu, rate, reason);
}
EXPORT_SYMBOL(acpuclk_set_rate);

uint32_t acpuclk_get_switch_time(void)
{
	if (!acpuclk_data || !acpuclk_data->switch_time_us)
		return 0;
	return acpuclk_data->switch_time_us;
}

unsigned long acpuclk_power_collapse(void)
{
	unsigned long rate = acpuclk_get_rate(smp_processor_id());
	acpuclk_set_rate(smp_processor_id(), acpuclk_data->power_collapse_khz,
			 SETRATE_PC);
	return rate;
}

unsigned long acpuclk_wait_for_irq(void)
{
	unsigned long rate = acpuclk_get_rate(smp_processor_id());
	acpuclk_set_rate(smp_processor_id(), acpuclk_data->wait_for_irq_khz,
			 SETRATE_SWFI);
	return rate;
}

void acpuclk_register(struct acpuclk_data *data)
{
	acpuclk_data = data;
}
