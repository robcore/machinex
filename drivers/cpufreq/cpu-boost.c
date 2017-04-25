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
#ifdef CONFIG_STATE_NOTIFIER
#include <linux/state_notifier.h>
#endif
#ifdef CONFIG_CPUFREQ_HARDLIMIT
#include <linux/cpufreq_hardlimit.h>
#endif

struct cpu_sync {
	unsigned int cpu;
	unsigned int input_boost_min;
	unsigned int input_boost_freq;
	unsigned int hotplug_boost_min;
	unsigned int hotplug_boost_freq;
	unsigned int wakeup_boost_min;
	unsigned int wakeup_boost_freq;
};

static DEFINE_PER_CPU(struct cpu_sync, sync_info);
static struct workqueue_struct *cpu_boost_wq;

static struct delayed_work input_boost_work;
static struct delayed_work input_boost_rem;
static struct delayed_work hotplug_boost_work;
static struct delayed_work hotplug_boost_rem;
static struct delayed_work wakeup_boost_work;
static struct delayed_work wakeup_boost_rem;

#ifdef CONFIG_STATE_NOTIFIER
static struct notifier_block notif;
#endif

static bool input_boost_enabled = false;
module_param(input_boost_enabled, bool, 0644);

static unsigned int input_boost_ms = 60;
module_param(input_boost_ms, uint, 0644);

static bool hotplug_boost_enabled = false;
module_param(hotplug_boost_enabled, bool, 0644);

static unsigned int hotplug_boost_ms = 60;
module_param(hotplug_boost_ms, uint, 0644);

static bool wakeup_boost_enabled = false;
module_param(wakeup_boost_enabled, bool, 0644);

static unsigned int wakeup_boost_ms = 60;
module_param(wakeup_boost_ms, uint, 0644);

static u64 last_input_time;
static u64 last_hotplug_time;
static u64 last_wakeup_time;

static unsigned int min_input_interval = 150;
module_param(min_input_interval, uint, 0644);
static unsigned int min_hotplug_interval = 150;
module_param(min_hotplug_interval, uint, 0644);
static unsigned int min_wakeup_interval = 150;
module_param(min_wakeup_interval, uint, 0644);

static int set_input_boost_freq(const char *buf, const struct kernel_param *kp)
{
	int i;
	unsigned int val, cpu;
	struct cpu_sync *i_sync_info;

	/* single number: apply to all CPUs */
	if (sscanf(buf, "%u\n", &val) != 1)
		return -EINVAL;

	for_each_possible_cpu(i)
		i_sync_info = &per_cpu(sync_info, i);

	i_sync_info->input_boost_freq = val;

	return 0;
}

static int get_input_boost_freq(char *buf, const struct kernel_param *kp)
{
	struct cpu_sync *i_sync_info;
	int i;
	ssize_t ret;

	for_each_possible_cpu(i)
		i_sync_info = &per_cpu(sync_info, i);

		ret = sprintf(buf, "%u", i_sync_info->input_boost_freq);

		return ret;
}

static const struct kernel_param_ops param_ops_input_boost_freq = {
	.set = set_input_boost_freq,
	.get = get_input_boost_freq,
};
module_param_cb(input_boost_freq, &param_ops_input_boost_freq, NULL, 0644);

static int set_hotplug_boost_freq(const char *buf, const struct kernel_param *kp)
{
	int i;
	unsigned int val, cpu;
	struct cpu_sync *h_sync_info;

	/* single number: apply to all CPUs */
	if (sscanf(buf, "%u\n", &val) != 1)
		return -EINVAL;

	for_each_possible_cpu(i)
		h_sync_info = &per_cpu(sync_info, i);

		h_sync_info->hotplug_boost_freq = val;

	return 0;
}

static int get_hotplug_boost_freq(char *buf, const struct kernel_param *kp)
{
	struct cpu_sync *h_sync_info;
	int i;
	ssize_t ret;

	for_each_possible_cpu(i)
		h_sync_info = &per_cpu(sync_info, i);

		ret = sprintf(buf, "%u", h_sync_info->hotplug_boost_freq);

		return ret;
}

static const struct kernel_param_ops param_ops_hotplug_boost_freq = {
	.set = set_hotplug_boost_freq,
	.get = get_hotplug_boost_freq,
};
module_param_cb(hotplug_boost_freq, &param_ops_hotplug_boost_freq, NULL, 0644);

static int set_wakeup_boost_freq(const char *buf, const struct kernel_param *kp)
{
	int i;
	unsigned int val, cpu;
	struct cpu_sync *w_sync_info;

	/* single number: apply to all CPUs */
	if (sscanf(buf, "%u\n", &val) != 1)
		return -EINVAL;

	for_each_possible_cpu(i)
		w_sync_info = &per_cpu(sync_info, i);

		w_sync_info->wakeup_boost_freq = val;

	return 0;
}

static int get_wakeup_boost_freq(char *buf, const struct kernel_param *kp)
{
	struct cpu_sync *w_sync_info;
	int i;
	ssize_t ret;

	for_each_possible_cpu(i)
		w_sync_info = &per_cpu(sync_info, i);

		ret = sprintf(buf, "%u", w_sync_info->wakeup_boost_freq);

		return ret;
}

static const struct kernel_param_ops param_ops_wakeup_boost_freq = {
	.set = set_wakeup_boost_freq,
	.get = get_wakeup_boost_freq,
};
module_param_cb(wakeup_boost_freq, &param_ops_wakeup_boost_freq, NULL, 0644);

/*
 * The CPUFREQ_ADJUST notifier is used to override the current policy min to
 * make sure policy min >= boost_min. The cpufreq framework then does the job
 * of enforcing the new policy.
 */
static int input_boost_adjust_notify(struct notifier_block *nb, unsigned long val,
				void *data)
{
	struct cpufreq_policy *policy = data;
	unsigned int cpu = policy->cpu;
	struct cpu_sync *s = &per_cpu(sync_info, cpu);
	unsigned int ib_min = s->input_boost_min;
	unsigned int min;

	if (val != CPUFREQ_ADJUST)
		return NOTIFY_OK;

	if (!ib_min)
		return NOTIFY_OK;

	min = min(ib_min, policy->max);

	pr_debug("CPU%u policy min before boost: %u kHz\n",
		 cpu, policy->min);
	pr_debug("CPU%u boost min: %u kHz\n", cpu, min);

	cpufreq_verify_within_limits(policy, min, check_cpufreq_hardlimit(policy->max)); /* Yank555.lu - Enforce hardlimit */
	//cpufreq_verify_within_limits(policy, min, UINT_MAX);


	pr_debug("CPU%u policy min after boost: %u kHz\n",
		 cpu, policy->min);

	return NOTIFY_OK;
}

static struct notifier_block input_boost_adjust_nb = {
	.notifier_call = input_boost_adjust_notify,
};

static int hotplug_boost_adjust_notify(struct notifier_block *nb, unsigned long val,
				void *data)
{
	struct cpufreq_policy *policy = data;
	unsigned int cpu = policy->cpu;
	struct cpu_sync *s = &per_cpu(sync_info, cpu);
	unsigned int hb_min = s->hotplug_boost_min;
	unsigned int min;

	if (val != CPUFREQ_ADJUST)
		return NOTIFY_OK;

	if (!hb_min)
		return NOTIFY_OK;

	min = min(hb_min, policy->max);

	pr_debug("CPU%u policy min before boost: %u kHz\n",
		 cpu, policy->min);
	pr_debug("CPU%u boost min: %u kHz\n", cpu, min);

	cpufreq_verify_within_limits(policy, min, check_cpufreq_hardlimit(policy->max)); /* Yank555.lu - Enforce hardlimit */
	//cpufreq_verify_within_limits(policy, min, UINT_MAX);


	pr_debug("CPU%u policy min after boost: %u kHz\n",
		 cpu, policy->min);

	return NOTIFY_OK;
}

static struct notifier_block hotplug_boost_adjust_nb = {
	.notifier_call = hotplug_boost_adjust_notify,
};

static int wakeup_boost_adjust_notify(struct notifier_block *nb, unsigned long val,
				void *data)
{
	struct cpufreq_policy *policy = data;
	unsigned int cpu = policy->cpu;
	struct cpu_sync *s = &per_cpu(sync_info, cpu);
	unsigned int wb_min = s->wakeup_boost_min;
	unsigned int min;

	if (val != CPUFREQ_ADJUST)
		return NOTIFY_OK;

	if (!wb_min)
		return NOTIFY_OK;

	min = min(wb_min, policy->max);

	cpufreq_verify_within_limits(policy, min, check_cpufreq_hardlimit(policy->max)); /* Yank555.lu - Enforce hardlimit */
	//cpufreq_verify_within_limits(policy, min, UINT_MAX);

	return NOTIFY_OK;
}

static struct notifier_block wakeup_boost_adjust_nb = {
	.notifier_call = wakeup_boost_adjust_notify,
};

static void update_policy_online(void)
{
	unsigned int i;

	/* Re-evaluate policy to trigger adjust notifier for online CPUs */
	get_online_cpus();
	for_each_online_cpu(i) {
		pr_debug("Updating policy for CPU%d\n", i);
		cpufreq_update_policy(i);
	}
	put_online_cpus();
}

static void do_input_boost_rem(struct work_struct *work)
{
	unsigned int i;
	struct cpu_sync *i_sync_info;

	/* Reset the input_boost_min for all CPUs in the system */
	pr_debug("Resetting input boost min for all CPUs\n");
	for_each_possible_cpu(i) {
		i_sync_info = &per_cpu(sync_info, i);
		i_sync_info->input_boost_min = 0;
	}

	/* Update policies for all online CPUs */
	update_policy_online();
}

static void do_input_boost(struct work_struct *work)
{
	unsigned int i;
	struct cpu_sync *i_sync_info;

	if (!input_boost_ms || !input_boost_enabled)
		return;

	cancel_delayed_work_sync(&input_boost_rem);

	/* Set the input_boost_min for all CPUs in the system */
	pr_debug("Setting input boost min for all CPUs\n");
	for_each_possible_cpu(i) {
		i_sync_info = &per_cpu(sync_info, i);
		i_sync_info->input_boost_min = i_sync_info->input_boost_freq;
	}

	/* Update policies for all online CPUs */
	update_policy_online();

	queue_delayed_work(cpu_boost_wq, &input_boost_rem,
					msecs_to_jiffies(input_boost_ms));
}

static void do_hotplug_boost_rem(struct work_struct *work)
{
	unsigned int i;
	struct cpu_sync *h_sync_info;

	/* Reset the hotplug_boost_min for all CPUs in the system */
	pr_debug("Resetting hotplug boost min for all CPUs\n");
	for_each_possible_cpu(i) {
		h_sync_info = &per_cpu(sync_info, i);
		h_sync_info->hotplug_boost_min = 0;
	}

	/* Update policies for all online CPUs */
	update_policy_online();
}

static void do_hotplug_boost(struct work_struct *work)
{
	unsigned int i;
	struct cpu_sync *h_sync_info;

	if (!hotplug_boost_ms || !hotplug_boost_enabled)
		return;

	cancel_delayed_work_sync(&hotplug_boost_rem);

	/* Set the hotplug_boost_min for all CPUs in the system */
	pr_debug("Setting hotplug boost min for all CPUs\n");
	for_each_possible_cpu(i) {
		h_sync_info = &per_cpu(sync_info, i);
		h_sync_info->hotplug_boost_min = h_sync_info->hotplug_boost_freq;
	}

	/* Update policies for all online CPUs */
	update_policy_online();

	queue_delayed_work(cpu_boost_wq, &hotplug_boost_rem,
					msecs_to_jiffies(hotplug_boost_ms));
}

static void do_wakeup_boost_rem(struct work_struct *work)
{
	unsigned int i;
	struct cpu_sync *w_sync_info;

	/* Reset the wakeup_boost_min for all CPUs in the system */
	pr_debug("Resetting wakeup boost min for all CPUs\n");
	for_each_possible_cpu(i) {
		w_sync_info = &per_cpu(sync_info, i);
		w_sync_info->wakeup_boost_min = 0;
	}

	/* Update policies for all online CPUs */
	update_policy_online();
}

static void do_wakeup_boost(struct work_struct *work)
{
	unsigned int i;
	struct cpu_sync *w_sync_info;

	if (!wakeup_boost_ms || !wakeup_boost_enabled)
		return;

	cancel_delayed_work_sync(&wakeup_boost_rem);

	/* Set the wakeup_boost_min for all CPUs in the system */
	pr_debug("Setting wakeup boost min for all CPUs\n");
	for_each_possible_cpu(i) {
		w_sync_info = &per_cpu(sync_info, i);
		w_sync_info->wakeup_boost_min = w_sync_info->wakeup_boost_freq;
	}

	/* Update policies for all online CPUs */
	update_policy_online();

	queue_delayed_work(cpu_boost_wq, &wakeup_boost_rem,
					msecs_to_jiffies(wakeup_boost_ms));
}

static void cpuboost_input_event(struct input_handle *handle,
		unsigned int type, unsigned int code, int value)
{
	u64 now;
	unsigned int min_interval;

	if (state_suspended || !input_boost_enabled)
		return;

	now = ktime_to_us(ktime_get());
	min_interval = max(min_input_interval, input_boost_ms);

	if (now - last_input_time < min_interval * USEC_PER_MSEC)
		return;

	pr_debug("Input boost for input event.\n");
	mod_delayed_work(cpu_boost_wq, &input_boost_work, 0);
	last_input_time = ktime_to_us(ktime_get());
}

static void hotplug_boost_event(void)
{
	u64 now;
	unsigned int min_interval;

	if (state_suspended || !hotplug_boost_enabled)
		return;

	now = ktime_to_us(ktime_get());
	min_interval = max(min_hotplug_interval, hotplug_boost_ms);

	if (now - last_hotplug_time < min_interval * USEC_PER_MSEC)
		return;

	mod_delayed_work(cpu_boost_wq, &hotplug_boost_work, 0);
	last_hotplug_time = ktime_to_us(ktime_get());
}

static void wakeup_boost_event(void)
{
	u64 now;
	unsigned int min_interval;

	if (state_suspended || !wakeup_boost_enabled)
		return;

	now = ktime_to_us(ktime_get());
	min_interval = max(min_wakeup_interval, wakeup_boost_ms);

	if (now - last_wakeup_time < min_interval * USEC_PER_MSEC)
		return;

	pr_debug("Input boost for input event.\n");
	mod_delayed_work(cpu_boost_wq, &wakeup_boost_work, 0);
	last_wakeup_time = ktime_to_us(ktime_get());
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

static int cpuboost_cpu_callback(struct notifier_block *cpu_nb,
				 unsigned long action, void *hcpu)
{
#ifdef CONFIG_STATE_NOTIFIER
	if (state_suspended)
		return NOTIFY_OK;
#endif

	switch (action & ~CPU_TASKS_FROZEN) {
		case CPU_ONLINE:
			if (input_boost_enabled) {
				mod_delayed_work(cpu_boost_wq, &input_boost_work, 0);
				last_input_time = ktime_to_us(ktime_get());
			}

			if (hotplug_boost_enabled) {
				hotplug_boost_event();
				last_hotplug_time = ktime_to_us(ktime_get());
			}
				break;
			default:
				break;
	}
	return NOTIFY_OK;
}

static struct notifier_block __refdata cpu_nblk = {
        .notifier_call = cpuboost_cpu_callback,
};

static int state_notifier_callback(struct notifier_block *this,
				unsigned long event, void *data)
{
	switch (event) {
		case STATE_NOTIFIER_ACTIVE:
		if (wakeup_boost_enabled) {
			wakeup_boost_event();
			last_wakeup_time = ktime_to_us(ktime_get());
		}
			break;
		case STATE_NOTIFIER_SUSPEND:
		if (input_boost_enabled) {
			cancel_delayed_work_sync(&input_boost_rem);
			cancel_delayed_work_sync(&input_boost_work);
		}
		if (hotplug_boost_enabled) {
			cancel_delayed_work_sync(&hotplug_boost_rem);
			cancel_delayed_work_sync(&hotplug_boost_work);
		}
		if (wakeup_boost_enabled) {
			cancel_delayed_work_sync(&wakeup_boost_rem);
			cancel_delayed_work_sync(&wakeup_boost_work);
		}
			break;
		default:
			break;
	}

	return NOTIFY_OK;
}

static int cpu_boost_init(void)
{
	int cpu, ret;
	struct cpu_sync *s;

	cpu_boost_wq = create_singlethread_workqueue("cpuboost_wq");
	if (!cpu_boost_wq)
		return -EFAULT;

	INIT_DELAYED_WORK(&input_boost_work, do_input_boost);
	INIT_DELAYED_WORK(&input_boost_rem, do_input_boost_rem);
	INIT_DELAYED_WORK(&hotplug_boost_work, do_hotplug_boost);
	INIT_DELAYED_WORK(&hotplug_boost_rem, do_hotplug_boost_rem);
	INIT_DELAYED_WORK(&wakeup_boost_work, do_wakeup_boost);
	INIT_DELAYED_WORK(&wakeup_boost_rem, do_wakeup_boost_rem);

	for_each_possible_cpu(cpu) {
		s = &per_cpu(sync_info, cpu);
		s->cpu = cpu;
	}
	cpufreq_register_notifier(&input_boost_adjust_nb, CPUFREQ_POLICY_NOTIFIER);
	cpufreq_register_notifier(&hotplug_boost_adjust_nb, CPUFREQ_POLICY_NOTIFIER);
	cpufreq_register_notifier(&wakeup_boost_adjust_nb, CPUFREQ_POLICY_NOTIFIER);

	ret = input_register_handler(&cpuboost_input_handler);
	if (ret)
		pr_err("Cannot register cpuboost input handler.\n");

	ret = register_hotcpu_notifier(&cpu_nblk);
	if (ret)
		pr_err("Cannot register cpuboost hotplug handler.\n");

#ifdef CONFIG_STATE_NOTIFIER
	notif.notifier_call = state_notifier_callback;
	if (state_register_client(&notif))
		pr_err("Cannot register State notifier callback for cpuboost.\n");
#endif

	return ret;
}
late_initcall(cpu_boost_init);
MODULE_AUTHOR("Mostly Neobuddy89, and some others");
MODULE_DESCRIPTION("'cpu_boost' - Does what it says");
MODULE_LICENSE("GPL v2");
