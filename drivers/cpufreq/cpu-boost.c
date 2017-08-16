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

struct cpu_sync {
	unsigned int cpu;
	unsigned int input_boost_min;
	unsigned int input_boost_freq;
};

static DEFINE_PER_CPU_SHARED_ALIGNED(struct cpu_sync, sync_info);
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

unsigned int input_boost_limit;
extern bool hotplug_ready;

static int set_input_boost_freq(const char *buf, const struct kernel_param *kp)
{
	unsigned int cpu;
	unsigned int val;

	/* single number: apply to all CPUs */
	if (sscanf(buf, "%u\n", &val) != 1)
		return -EINVAL;

	for_each_possible_cpu(cpu)
		per_cpu(sync_info, cpu).input_boost_freq = val;

	return 0;
}

static int get_input_boost_freq(char *buf, const struct kernel_param *kp)
{
	unsigned int cpu = smp_processor_id();
	struct cpu_sync *i_sync_info = &per_cpu(sync_info, cpu);
	ssize_t ret;

	ret = sprintf(buf, "%u\n", i_sync_info->input_boost_freq);

	return ret;
}

static const struct kernel_param_ops param_ops_input_boost_freq = {
	.set = set_input_boost_freq,
	.get = get_input_boost_freq,
};
module_param_cb(input_boost_freq, &param_ops_input_boost_freq, NULL, 0644);

static void update_policy_online(void)
{
	unsigned int cpu;

	if (!is_display_on())
		return;

	for_each_online_cpu(cpu) {
		reapply_hard_limits(cpu);
		cpufreq_update_policy(cpu);
	}
}

static void do_input_boost_rem(struct work_struct *work)
{
	unsigned int cpu;
	struct cpu_sync *i_sync_info;
	struct cpufreq_policy policy;

	if (!is_display_on() || !input_boost_enabled || !input_boost_ms)
		return;

	/* Reset the input_boost_min for all CPUs in the system */
	for_each_possible_cpu(cpu) {
		i_sync_info = &per_cpu(sync_info, cpu);
		if (cpufreq_get_policy(&policy, cpu))
			continue;
		input_boost_limit = i_sync_info->input_boost_min = policy.hlimit_min_screen_on;
	}

	/* Update policies for all online CPUs */
	update_policy_online();
}

static void do_input_boost(struct work_struct *work)
{
	unsigned int cpu;
	struct cpu_sync *i_sync_info;

	if (!input_boost_enabled || !input_boost_ms || !is_display_on())
		return;

	/* Set the input_boost_min for all CPUs in the system */
	for_each_online_cpu(cpu) {
		i_sync_info = &per_cpu(sync_info, cpu);
		input_boost_limit = i_sync_info->input_boost_min = i_sync_info->input_boost_freq;
	}

	/* Update policies for all online CPUs */
	update_policy_online();

	mod_delayed_work_on(0, cpu_boost_wq, &input_boost_rem,
					msecs_to_jiffies(input_boost_ms));
}
#if 0
void cpuboost_keypress_event(unsigned int keytype)
{
	u64 min_interval;
	u64 now;

	if (!input_boost_enabled || !hotplug_ready 
	    || !input_boost_ms || !is_display_on())
		return;

	min_interval = max(min_input_interval, input_boost_ms);
	now = ktime_to_us(ktime_get());

	if (now - last_input_time < min_interval * USEC_PER_MSEC)
		return;

	mod_delayed_work_on(0, cpu_boost_wq, &input_boost_work, 0);
	last_input_time = ktime_to_us(ktime_get());
}
EXPORT_SYMBOL(cpuboost_keypress_event);
#endif
static void cpuboost_input_event(struct input_handle *handle,
		unsigned int type, unsigned int code, int value)
{
	u64 min_interval;
	u64 now;

	if (!input_boost_enabled || !hotplug_ready 
	    || !input_boost_ms || !is_display_on())
		return;

	min_interval = max(min_input_interval, input_boost_ms);
	now = ktime_to_us(ktime_get());

	if (now - last_input_time < min_interval * USEC_PER_MSEC)
		return;

	mod_delayed_work_on(0, cpu_boost_wq, &input_boost_work, 0);
	last_input_time = ktime_to_us(ktime_get());
}

static int cpuboost_input_connect(struct input_handler *handler,
		struct input_dev *dev, const struct input_device_id *id)
{
	struct input_handle *handle;
	int error;

	handle = kzalloc(sizeof(struct input_handle), GFP_KERNEL);
	if (!handle)
		return -ENOMEM;

	handle->dev = dev;
	handle->handler = handler;
	handle->name = "cpufreq";

	error = input_register_handle(handle);
	if (error)
		goto err2;

	error = input_open_device(handle);
	if (error)
		goto err1;

	return 0;
err1:
	input_unregister_handle(handle);
err2:
	kfree(handle);
	return error;
}

static void cpuboost_input_disconnect(struct input_handle *handle)
{
	input_close_device(handle);
	input_unregister_handle(handle);
	kfree(handle);
}

static const struct input_device_id cpuboost_ids[] = {
	/* multi-touch touchscreen */
	{
		.flags = INPUT_DEVICE_ID_MATCH_EVBIT |
			INPUT_DEVICE_ID_MATCH_ABSBIT,
		.evbit = { BIT_MASK(EV_ABS) },
		.absbit = { [BIT_WORD(ABS_MT_POSITION_X)] =
			BIT_MASK(ABS_MT_POSITION_X) |
			BIT_MASK(ABS_MT_POSITION_Y) },
	},
	/* touchpad */
	{
		.flags = INPUT_DEVICE_ID_MATCH_KEYBIT |
			INPUT_DEVICE_ID_MATCH_ABSBIT,
		.keybit = { [BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH) },
		.absbit = { [BIT_WORD(ABS_X)] =
			BIT_MASK(ABS_X) | BIT_MASK(ABS_Y) },
	},
	/* Keypad */
	{
		.flags = INPUT_DEVICE_ID_MATCH_EVBIT,
		.evbit = { BIT_MASK(EV_KEY) },
	},
	{ },
};

static struct input_handler cpuboost_input_handler = {
	.event          = cpuboost_input_event,
	.connect        = cpuboost_input_connect,
	.disconnect     = cpuboost_input_disconnect,
	.name           = "cpu-boost",
	.id_table       = cpuboost_ids,
};

static int cpu_boost_init(void)
{
	int cpu, ret;
	struct cpu_sync *s;

	cpu_boost_wq = alloc_workqueue("cpuboost_wq", WQ_HIGHPRI | WQ_FREEZABLE, 0);
	if (!cpu_boost_wq)
		return -EFAULT;

	INIT_DELAYED_WORK(&input_boost_work, do_input_boost);
	INIT_DELAYED_WORK(&input_boost_rem, do_input_boost_rem);

	for_each_possible_cpu(cpu) {
		s = &per_cpu(sync_info, cpu);
		s->cpu = cpu;
	}

	ret = input_register_handler(&cpuboost_input_handler);
	if (ret)
		pr_err("ERROR! Cpuboost input handler registration failed!\n");

	return ret;
}

late_initcall(cpu_boost_init);
MODULE_AUTHOR("Neobuddy89/upstream/robcore");
MODULE_DESCRIPTION("'cpu_boost' - Does what it says");
MODULE_LICENSE("GPL v2");
