/*
 * Copyright (c) 2013-2015, The Linux Foundation. All rights reserved.
 * Copyright (c) 2013-2016, Pranav Vashi <neobuddy89@gmail.com>
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

#define pr_fmt(fmt) "cpu-boost: " fmt

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/notifier.h>
#include <linux/cpufreq.h>
#include <linux/cpu.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/time.h>
#include <linux/display_state.h>

static struct workqueue_struct *cpu_boost_wq;

static struct delayed_work input_boost_work;
static struct delayed_work input_boost_rem;

static bool input_boost_enabled = false;
module_param(input_boost_enabled, bool, 0644);

static unsigned int input_boost_ms = 100;
module_param(input_boost_ms, uint, 0644);

static struct delayed_work input_boost_rem;
static u64 last_input_time;

static unsigned int min_input_interval = 200;
module_param(min_input_interval, uint, 0644);

extern bool hotplug_ready;

static int set_input_boost_freq(const char *buf, const struct kernel_param *kp)
{
	unsigned int cpu;
	unsigned int val;

	/* single number: apply to all CPUs */
	if (sscanf(buf, "%u\n", &val) != 1)
		return -EINVAL;

	for_each_possible_cpu(cpu)
		per_cpu(boostinfo, cpu).input_boost_freq = val;

	return 0;
}

static int get_input_boost_freq(char *buf, const struct kernel_param *kp)
{
	unsigned int cpu = smp_processor_id();
	struct cpuboost *localboost = &per_cpu(boostinfo, cpu);
	ssize_t ret;

	ret = sprintf(buf, "%u\n", localboost->input_boost_freq);

	return ret;
}

static const struct kernel_param_ops param_ops_input_boost_freq = {
	.set = set_input_boost_freq,
	.get = get_input_boost_freq,
};
module_param_cb(input_boost_freq, &param_ops_input_boost_freq, NULL, 0644);

static void update_policy_online(unsigned int cpu)
{
	if (!is_display_on())
		return;

	reapply_hard_limits(cpu);
	cpufreq_update_policy(cpu);
}

static void do_input_boost_rem(struct work_struct *work)
{
	unsigned int cpu;
	struct cpuboost *localboost;
	struct cpufreq_policy policy;

	if (!is_display_on() || !input_boost_enabled || !input_boost_ms)
		return;

	/* Reset the input_boost_min for all CPUs in the system */
	for_each_possible_cpu(cpu) {
		localboost = &per_cpu(boostinfo, cpu);
		if (cpufreq_get_policy(&policy, cpu))
			continue;
		localboost->input_boost_min = policy.hlimit_min_screen_on;
		update_policy_online(cpu);
	}


}

static void do_input_boost(struct work_struct *work)
{
	unsigned int cpu;
	struct cpuboost *localboost;

	if (!input_boost_enabled || !input_boost_ms || !is_display_on())
		return;

	/* Set the input_boost_min for all CPUs in the system */
	for_each_online_cpu(cpu) {
		localboost = &per_cpu(boostinfo, cpu);
		localboost->input_boost_min = localboost->input_boost_freq;
		update_policy_online(cpu);
	}

	mod_delayed_work_on(0, cpu_boost_wq, &input_boost_rem,
					msecs_to_jiffies(input_boost_ms));
}

u64 min_interval;
u64 now;

void cpu_boost_event(void)
{
	if (!input_boost_enabled || !hotplug_ready 
	    || !input_boost_ms || !is_display_on())
		return;

	min_interval = max(min_input_interval, input_boost_ms);
	now = ktime_to_us(ktime_get());

	if (!last_input_time)
		goto first_time;

	if (now - last_input_time < min_interval * USEC_PER_MSEC)
		return;

first_time:
	mod_delayed_work_on(0, cpu_boost_wq, &input_boost_work, 0);
	last_input_time = ktime_to_us(ktime_get());
}
EXPORT_SYMBOL(cpu_boost_event);

static int cpu_boost_init(void)
{
	int cpu, ret;
	struct cpuboost *s;

	cpu_boost_wq = alloc_workqueue("cpuboost_wq", WQ_HIGHPRI | WQ_FREEZABLE, 0);
	if (!cpu_boost_wq)
		return -EFAULT;

	for_each_possible_cpu(cpu) {
		s = &per_cpu(boostinfo, cpu);
		s->cpu = cpu;
	}

	INIT_DELAYED_WORK(&input_boost_work, do_input_boost);
	INIT_DELAYED_WORK(&input_boost_rem, do_input_boost_rem);

	return ret;
}

late_initcall(cpu_boost_init);
MODULE_AUTHOR("Neobuddy89/upstream/robcore");
MODULE_DESCRIPTION("'cpu_boost' - Does what it says");
MODULE_LICENSE("GPL v2");
