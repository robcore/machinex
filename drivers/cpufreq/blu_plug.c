/*
 * Dynamic Hotplug for mako / hammerhead / shamu
 *
 * Copyright (C) 2013 Stratos Karafotis <stratosk@semaphore.gr> (dyn_hotplug for mako)
 *
 * Copyright (C) 2015 engstk <eng.stk@sapo.pt> (hammerhead & shamu implementation, fixes and changes to blu_plug)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#define DEBUG 0
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cpu.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/cpufreq.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/machinex_defines.h>

#define INIT_DELAY		20000
#define DELAY			100
#define UP_THRESHOLD		80
#define DEF_DOWN_TIMER_CNT	6
#define DEF_UP_TIMER_CNT	2
#define MAX_CPUS_ONLINE_SUSP     1
#define MAX_CPUS_ONLINE DEFAULT_MAX_CPUS_ONLINE
#define DEF_PLUG_THRESHOLD      80
#define BLU_PLUG_ENABLED	0

static unsigned int blu_plug_enabled = BLU_PLUG_ENABLED;

static unsigned int up_threshold = UP_THRESHOLD;
static unsigned int delay = DELAY;
static unsigned int min_cpus_online = DEFAULT_MIN_CPUS_ONLINE;
static unsigned int max_cpus_online = DEFAULT_MAX_CPUS_ONLINE;
static unsigned int down_timer;
static unsigned int up_timer;
static unsigned int down_timer_cnt = DEF_DOWN_TIMER_CNT;
static unsigned int up_timer_cnt = DEF_UP_TIMER_CNT;
static unsigned int plug_threshold[MAX_CPUS_ONLINE] = {[0 ... MAX_CPUS_ONLINE-1] = DEF_PLUG_THRESHOLD};

static struct delayed_work dyn_work;
static struct workqueue_struct *dyn_workq;
static struct notifier_block notify;

/* Bring online each possible CPU up to max_cpus_online cores */
static void up_all(void)
{
	unsigned int cpu;

	for_each_nonboot_offline_cpu(cpu) {
		if (cpu_out_of_range_hp(cpu))
			break;
		if (cpu_is_offline(cpu) && num_online_cpus() < max_cpus_online)
			cpu_up(cpu);
	}
	down_timer = 0;
}

/* Iterate through possible CPUs and bring online the first offline found */
static void up_one(void)
{
	unsigned int cpu;

	/* All CPUs are online, return */
	if (num_online_cpus() == max_cpus_online)
		goto out;

	cpu = cpumask_next_zero(0, cpu_online_mask);
	if (cpu < nr_cpu_ids)
		cpu_up(cpu);
out:
	down_timer = 0;
	up_timer = 0;
}

static void __down_one(unsigned int cpu)
{
	if (cpu_in_range_hp(cpu))
		cpu_down(cpu);
}

/* Iterate through online CPUs and take offline the lowest loaded one */
static inline void down_one(void)
{
	unsigned int cpu;
	unsigned int l_cpu = 0;
	unsigned int l_freq = ~0;
	unsigned int p_cpu = 0;
	unsigned int p_thres = 0;
	bool all_equal = false;

	/* Min online CPUs, return */
	if (num_online_cpus() == min_cpus_online)
		goto out;

	for_each_online_cpu(cpu) {
		unsigned int thres = plug_threshold[cpu];

		if (!cpu || thres == p_thres) {
			p_thres = thres;
			p_cpu = cpu;
			all_equal = true;
		} else if (thres > p_thres) {
			p_thres = thres;
			p_cpu = cpu;
			all_equal = false;
		}

		if (cpu) {
			unsigned int cur = cpufreq_quick_get(cpu);

			if (l_freq > cur) {
				l_freq = cur;
				l_cpu = cpu;
			}
		}
	}

	if (all_equal)
		__down_one(l_cpu);
	else
		__down_one(p_cpu);
out:
	down_timer = 0;
	up_timer = 0;
}

/*
 * Every DELAY, check the average load of online CPUs. If the average load
 * is above up_threshold bring online one more CPU if up_timer has expired.
 * If the average load is below up_threshold offline one more CPU if the
 * down_timer has expired.
 */
u64 prev_cpu_wall;
u64 prev_cpu_idle;
static void load_timer(struct work_struct *work)
{
	unsigned int cpu;
	unsigned int avg_load = 0;
	unsigned cur_load;
	unsigned int online_cpus = num_online_cpus();

	if (down_timer < down_timer_cnt)
		down_timer++;

	if (up_timer < up_timer_cnt)
		up_timer++;

	for_each_online_cpu(cpu) {
		u64 cur_wall_time, cur_idle_time;
		unsigned int wall_time, idle_time;


		cur_idle_time = get_cpu_idle_time(cpu, &cur_wall_time, 0);

		wall_time = (unsigned int)
				(cur_wall_time -
					prev_cpu_wall);
		prev_cpu_wall = cur_wall_time;

		idle_time = (unsigned int)(cur_idle_time - prev_cpu_idle);
		prev_cpu_idle = cur_idle_time;
		/* if wall_time < idle_time, evaluate cpu load next time */

		if (wall_time >= idle_time) {
			/*
			 * if wall_time is equal to idle_time,
			 * cpu_load is equal to 0
			 */
			cur_load = wall_time > idle_time ? (100 *
				(wall_time - idle_time)) / wall_time : 0;
		}
	}
		avg_load += cur_load;

	avg_load /= online_cpus;

#if DEBUG
	pr_debug("%s: avg_load: %u, num_online_cpus: %u\n", __func__, avg_load, num_online_cpus());
	pr_debug("%s: up_timer: %u, down_timer: %u\n", __func__, up_timer, down_timer);
#endif

	if ((avg_load >= up_threshold && up_timer >= up_timer_cnt) ||
		online_cpus < min_cpus_online)
		up_one();
	else if (down_timer >= down_timer_cnt || online_cpus > max_cpus_online)
		down_one();

	mod_delayed_work_on(0, dyn_workq, &dyn_work, msecs_to_jiffies(delay));
}

/******************** Module parameters *********************/

/* up_threshold */
static int set_up_threshold(const char *val, const struct kernel_param *kp)
{
	int ret = 0;
	unsigned int i;

	ret = kstrtouint(val, 10, &i);
	if (ret)
		return -EINVAL;

	if (i < 1 || i > 100)
		return -EINVAL;

	up_threshold = i;

	return ret;
}

static struct kernel_param_ops up_threshold_ops = {
	.set = set_up_threshold,
	.get = param_get_uint,
};

module_param_cb(up_threshold, &up_threshold_ops, &up_threshold, 0644);

/* min_cpus_online */
static int set_min_cpus_online(const char *val, const struct kernel_param *kp)
{
	int ret = 0;
	unsigned int i;

	ret = kstrtouint(val, 10, &i);
	if (ret)
		return -EINVAL;

	if (i < 1 || i > max_cpus_online || i > num_possible_cpus())
		return -EINVAL;

	min_cpus_online = i;

	return ret;
}

static struct kernel_param_ops min_cpus_online_ops = {
	.set = set_min_cpus_online,
	.get = param_get_uint,
};

module_param_cb(min_cpus_online, &min_cpus_online_ops, &min_cpus_online, 0644);

/* max_cpus_online */
static int set_max_cpus_online(const char *val, const struct kernel_param *kp)
{
	int ret = 0;
	unsigned int i;

	ret = kstrtouint(val, 10, &i);
	if (ret)
		return -EINVAL;

	if (i < 1 || i < min_cpus_online || i > num_possible_cpus())
		return -EINVAL;

	max_cpus_online = i;

	return ret;
}

static struct kernel_param_ops max_cpus_online_ops = {
	.set = set_max_cpus_online,
	.get = param_get_uint,
};

module_param_cb(max_cpus_online, &max_cpus_online_ops, &max_cpus_online, 0644);

/* down_timer_cnt */
static int set_down_timer_cnt(const char *val, const struct kernel_param *kp)
{
	int ret = 0;
	unsigned int i;

	ret = kstrtouint(val, 10, &i);
	if (ret)
		return -EINVAL;

	if (i < 1 || i > 50)
		return -EINVAL;

	if (i < up_timer_cnt)
		i = up_timer_cnt;

	down_timer_cnt = i;

	return ret;
}

static struct kernel_param_ops down_timer_cnt_ops = {
	.set = set_down_timer_cnt,
	.get = param_get_uint,
};

module_param_cb(down_timer_cnt, &down_timer_cnt_ops, &down_timer_cnt, 0644);

/* up_timer_cnt */
static int set_up_timer_cnt(const char *val, const struct kernel_param *kp)
{
	int ret = 0;
	unsigned int i;

	ret = kstrtouint(val, 10, &i);
	if (ret)
		return -EINVAL;

	if (i < 1 || i > 50)
		return -EINVAL;

	up_timer_cnt = i;

	return ret;
}

static struct kernel_param_ops up_timer_cnt_ops = {
	.set = set_up_timer_cnt,
	.get = param_get_uint,
};

module_param_cb(up_timer_cnt, &up_timer_cnt_ops, &up_timer_cnt, 0644);

/* plug_threshold */
module_param_array(plug_threshold, uint, NULL, 0644);

/***************** end of module parameters *****************/

static int dyn_hp_init(void)
{
	if (!blu_plug_enabled)
		return 0;

	dyn_workq = alloc_workqueue("dyn_hotplug_workqueue", WQ_HIGHPRI | WQ_FREEZABLE, 0);
	if (!dyn_workq)
		return -ENOMEM;

	INIT_DELAYED_WORK(&dyn_work, load_timer);
	queue_delayed_work_on(0, dyn_workq, &dyn_work, msecs_to_jiffies(INIT_DELAY));

	pr_info("%s: activated\n", __func__);

	return 0;
}

static void dyn_hp_exit(void)
{
	int cpu;

	cancel_delayed_work_sync(&dyn_work);

	destroy_workqueue(dyn_workq);

	/* Wake up all the sibling cores */
	for_each_possible_cpu(cpu)
		if (!cpu_online(cpu))
			cpu_up(cpu);

	pr_info("%s: deactivated\n", __func__);
}

/* enabled */
static int set_enabled(const char *val, const struct kernel_param *kp)
{
	int ret = 0;
	unsigned int i;

	ret = kstrtouint(val, 10, &i);
	if (ret)
		return -EINVAL;

	if (i < 0 || i > 1)
		return -EINVAL;

	if (i == blu_plug_enabled)
		return ret;

	blu_plug_enabled = i;

	if (blu_plug_enabled)
		ret = dyn_hp_init();
	else
		dyn_hp_exit();

	return ret;
}

static struct kernel_param_ops enabled_ops = {
	.set = set_enabled,
	.get = param_get_uint,
};

module_param_cb(enabled, &enabled_ops, &blu_plug_enabled, 0644);

MODULE_AUTHOR("Stratos Karafotis <stratosk@semaphore.gr");
MODULE_AUTHOR("engstk <eng.stk@sapo.pt>");
MODULE_DESCRIPTION("'dyn_hotplug' - A dynamic hotplug driver for mako / hammerhead / shamu (blu_plug)");
MODULE_LICENSE("GPLv2");

late_initcall(dyn_hp_init);
module_exit(dyn_hp_exit);
