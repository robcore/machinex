/*
 * Author: Alucard_24@XDA
 *
 * Copyright 2012 Alucard_24@XDA
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
#include <linux/cpu.h>
#include <linux/cpufreq.h>
#include <linux/jiffies.h>
#include <linux/kernel_stat.h>
#include <linux/tick.h>
#include <linux/sched.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/slab.h>

#ifdef CONFIG_POWERSUSPEND
#include <linux/powersuspend.h>
#else
#include <linux/fb.h>
#endif

struct hotplug_cpuinfo {
	u64 prev_cpu_wall;
	u64 prev_cpu_idle;
	unsigned int up_load;
	unsigned int down_load;
	unsigned int up_freq;
	unsigned int down_freq;
	unsigned int up_rq;
	unsigned int down_rq;
	unsigned int up_rate;
	unsigned int down_rate;
	unsigned int cur_up_rate;
	unsigned int cur_down_rate;
};

static DEFINE_PER_CPU(struct hotplug_cpuinfo, od_hotplug_cpuinfo);

#ifndef CONFIG_POWERSUSPEND
static struct notifier_block notif;
#endif
static struct delayed_work alucard_hotplug_work;

static struct hotplug_tuners {
	unsigned int hotplug_sampling_rate;
	unsigned int hotplug_enable;
	unsigned int min_cpus_online;
	unsigned int max_cpus_online;
	unsigned int max_cpus_online_susp;
	unsigned int hp_io_is_busy;
	unsigned int hotplug_suspend;
	bool suspended;
	bool force_cpu_up;
} hotplug_tuners_ins = {
	.hotplug_sampling_rate = 30,
	.hotplug_enable = 1,
	.min_cpus_online = 1,
	.max_cpus_online = NR_CPUS,
	.max_cpus_online_susp = 2,
	.hp_io_is_busy = 0,
	.hotplug_suspend = 0,
	.suspended = false,
	.force_cpu_up = false,
};

#define DOWN_INDEX		(0)
#define UP_INDEX		(1)

static void __ref hotplug_work_fn(struct work_struct *work)
{
	unsigned int upmax_cpus_online = 0;
	unsigned int min_cpus_online = hotplug_tuners_ins.min_cpus_online;
	unsigned int cpu = 0;
	unsigned int rq_avg = 0;
	bool rq_avg_calc = true;
	int online_cpus;
	cpumask_var_t cpus;

	if (hotplug_tuners_ins.suspended) {
		upmax_cpus_online = hotplug_tuners_ins.max_cpus_online_susp;
		rq_avg_calc = false;
	} else
		upmax_cpus_online = hotplug_tuners_ins.max_cpus_online;

	/* get nr online cpus */
	online_cpus = num_online_cpus();

	cpumask_copy(cpus, cpu_online_mask);

	if (!hotplug_tuners_ins.force_cpu_up) {
		if (rq_avg_calc) {
			rq_avg = (avg_nr_running() * 100) >> FSHIFT;
			/*printk(KERN_ERR "online_cpus[%u], rq_avg[%u]\n", online_cpus, rq_avg);*/
		}

		for_each_cpu(cpu, cpus) {
			struct hotplug_cpuinfo *pcpu_info =
					&per_cpu(od_hotplug_cpuinfo, cpu);
			unsigned int upcpu = (cpu + 1);
			u64 cur_wall_time, cur_idle_time;
			unsigned int wall_time, idle_time;
			unsigned int cur_load = 0;
			unsigned int cur_freq = 0;
			int ret;

			cur_idle_time = get_cpu_idle_time(
					cpu, &cur_wall_time,
					hotplug_tuners_ins.hp_io_is_busy);

			wall_time = (unsigned int)
					(cur_wall_time -
						pcpu_info->prev_cpu_wall);
			pcpu_info->prev_cpu_wall = cur_wall_time;

			idle_time = (unsigned int)
					(cur_idle_time -
						pcpu_info->prev_cpu_idle);
			pcpu_info->prev_cpu_idle = cur_idle_time;

			/* if wall_time < idle_time or wall_time == 0, evaluate cpu load next time */
			if (unlikely(!wall_time || wall_time < idle_time))
				continue;

			cur_load = 100 * (wall_time - idle_time) / wall_time;

			/* get the cpu current frequency */
			cur_freq = cpufreq_quick_get(cpu);

			/* get nr online cpus */
			online_cpus = num_online_cpus();

			if (cpu > 0	&&
				 online_cpus > upmax_cpus_online) {
					ret = cpu_down(cpu);
					if (!ret) {
						online_cpus--;
					}
			} else if (online_cpus < min_cpus_online
						&& upcpu < upmax_cpus_online) {
					if (cpu_is_offline(upcpu)) {
						ret = cpu_up(upcpu);
						if (!ret) {
							pcpu_info->cur_up_rate = 1;
							pcpu_info->cur_down_rate = 1;
							online_cpus++;
						}
					}
			} else if (upcpu > 0
				&& upcpu < upmax_cpus_online
				&& (cpu_is_offline(upcpu))
				&& online_cpus < upmax_cpus_online
				&& cur_load >= pcpu_info->up_load
				&& cur_freq >= pcpu_info->up_freq
				&& (rq_avg > pcpu_info->up_rq
					 || rq_avg_calc == false)) {
					if (pcpu_info->cur_up_rate %
							pcpu_info->up_rate == 0) {
	#if 0
						pr_info("CPU[%u], UPCPU[%u], \
							cur_freq[%u], cur_load[%u], \
							rq_avg[%u], up_rate[%u]\n",
							cpu, upcpu, cur_freq,
							cur_load, rq_avg,
							pcpu_info->cur_up_rate);
	#endif
						ret = cpu_up(upcpu);
						if (!ret) {
							pcpu_info->cur_up_rate = 1;
							pcpu_info->cur_down_rate = 1;
							online_cpus++;
						}
					} else {
						if (pcpu_info->cur_up_rate < pcpu_info->up_rate)
							++pcpu_info->cur_up_rate;
						else
							pcpu_info->cur_up_rate = 1;
					}
			} else if (cpu >= min_cpus_online && (
					cur_load < pcpu_info->down_load
					|| (cur_freq <= pcpu_info->down_freq
					&& (rq_avg <= pcpu_info->down_rq
						 || rq_avg_calc == false)))) {
						if (pcpu_info->cur_down_rate %
								pcpu_info->down_rate == 0) {
	#if 0
							pr_info("CPU[%u], \
								cur_freq[%u], \
								cur_load[%u], \
								rq_avg[%u], \
								down_rate[%u]\n",
								cpu, cur_freq,
								cur_load, rq_avg,
								pcpu_info->
								cur_down_rate);
	#endif
							ret = cpu_down(cpu);
							if (!ret) {
								online_cpus--;
							}
						} else {
							if (pcpu_info->cur_down_rate < pcpu_info->down_rate)
								++pcpu_info->cur_down_rate;
							else
								pcpu_info->cur_down_rate = 1;
						}
			} else {
				pcpu_info->cur_up_rate = 1;
				pcpu_info->cur_down_rate = 1;
			}
		}
	} else {
		for_each_cpu_not(cpu, cpus) {
			if (cpu < (upmax_cpus_online - 1)) {
				cpu_up(cpu);
			}
		}
		hotplug_tuners_ins.force_cpu_up = false;
	}

	mod_delayed_work_on(BOOT_CPU, system_wq,
				&alucard_hotplug_work,
				msecs_to_jiffies(
				hotplug_tuners_ins.hotplug_sampling_rate));
}

#ifdef CONFIG_POWERSUSPEND
static void __alucard_hotplug_suspend(struct power_suspend *handler)
#else
static void __alucard_hotplug_suspend(void)
#endif
{
	if (hotplug_tuners_ins.hotplug_enable > 0
				&& hotplug_tuners_ins.hotplug_suspend == 1 &&
				hotplug_tuners_ins.suspended == false) {
			hotplug_tuners_ins.suspended = true;
	}
}

#ifdef CONFIG_POWERSUSPEND
static void __ref __alucard_hotplug_resume(struct power_suspend *handler)
#else
static void __ref __alucard_hotplug_resume(void)
#endif
{
	if (hotplug_tuners_ins.hotplug_enable > 0
		&& hotplug_tuners_ins.suspended == true) {
			hotplug_tuners_ins.suspended = false;
			/* wake up everyone */
			if (hotplug_tuners_ins.hotplug_suspend == 1)
				hotplug_tuners_ins.force_cpu_up = true;
	}
}

#ifdef CONFIG_POWERSUSPEND
static struct power_suspend alucard_hotplug_power_suspend_driver = {
	.suspend = __alucard_hotplug_suspend,
	.resume = __alucard_hotplug_resume,
};
#else
static int prev_fb = FB_BLANK_UNBLANK;

static int fb_notifier_callback(struct notifier_block *self,
				unsigned long event, void *data)
{
	struct fb_event *evdata = data;
	int *blank;

	if (evdata && evdata->data && event == FB_EVENT_BLANK) {
		blank = evdata->data;
		switch (*blank) {
			case FB_BLANK_UNBLANK:
				if (prev_fb == FB_BLANK_POWERDOWN) {
					/* display on */
					__alucard_hotplug_resume();
					prev_fb = FB_BLANK_UNBLANK;
				}
				break;
			case FB_BLANK_POWERDOWN:
				if (prev_fb == FB_BLANK_UNBLANK) {
					/* display off */
					__alucard_hotplug_suspend();
					prev_fb = FB_BLANK_POWERDOWN;
				}
				break;
		}
	}

	return NOTIFY_OK;
}
#endif

static int alucard_hotplug_callback(struct notifier_block *nb,
			unsigned long action, void *data)
{
	struct hotplug_cpuinfo *pcpu_info;
	unsigned int cpu = (int)data;

	switch (action) {
	case CPU_ONLINE:
		pcpu_info = &per_cpu(od_hotplug_cpuinfo, cpu);
		pcpu_info->prev_cpu_idle = get_cpu_idle_time(cpu,
				&pcpu_info->prev_cpu_wall,
				hotplug_tuners_ins.hp_io_is_busy);
		pcpu_info->cur_up_rate = 1;
		pcpu_info->cur_down_rate = 1;
		break;
	}

	return NOTIFY_OK;
}

static struct notifier_block alucard_hotplug_nb =
{
   .notifier_call = alucard_hotplug_callback,
};

static void hotplug_start(void)
{
	unsigned int cpu;

	hotplug_tuners_ins.suspended = false;
	hotplug_tuners_ins.force_cpu_up = false;

	get_online_cpus();
	register_hotcpu_notifier(&alucard_hotplug_nb);
	for_each_online_cpu(cpu) {
		struct hotplug_cpuinfo *pcpu_info =
				&per_cpu(od_hotplug_cpuinfo, cpu);

		pcpu_info->prev_cpu_idle = get_cpu_idle_time(cpu,
				&pcpu_info->prev_cpu_wall,
				hotplug_tuners_ins.hp_io_is_busy);
		pcpu_info->cur_up_rate = 1;
		pcpu_info->cur_down_rate = 1;
	}
	put_online_cpus();

	INIT_DEFERRABLE_WORK(&alucard_hotplug_work, hotplug_work_fn);
	mod_delayed_work_on(BOOT_CPU, system_wq,
				&alucard_hotplug_work,
				msecs_to_jiffies(
				hotplug_tuners_ins.hotplug_sampling_rate));

#ifdef CONFIG_POWERSUSPEND
	register_power_suspend(&alucard_hotplug_power_suspend_driver);
#else
	notif.notifier_call = fb_notifier_callback;
	if (fb_register_client(&notif))
		pr_err("Failed to register FB notifier callback for Alucard Hotplug\n");
#endif
}

static void hotplug_stop(void)
{
#ifdef CONFIG_POWERSUSPEND
	unregister_power_suspend(&alucard_hotplug_power_suspend_driver);
#else
	fb_unregister_client(&notif);
	notif.notifier_call = NULL;
#endif
	cancel_delayed_work_sync(&alucard_hotplug_work);
	get_online_cpus();
	unregister_hotcpu_notifier(&alucard_hotplug_nb);
	put_online_cpus();
}

#define show_one(file_name, object)					\
static ssize_t show_##file_name						\
(struct kobject *kobj, struct attribute *attr, char *buf)		\
{									\
	return sprintf(buf, "%d\n",					\
			hotplug_tuners_ins.object);			\
}

show_one(hotplug_sampling_rate, hotplug_sampling_rate);
show_one(hotplug_enable, hotplug_enable);
show_one(min_cpus_online, min_cpus_online);
show_one(max_cpus_online, max_cpus_online);
show_one(max_cpus_online_susp, max_cpus_online_susp);
show_one(hp_io_is_busy, hp_io_is_busy);
show_one(hotplug_suspend, hotplug_suspend);

#define show_pcpu_param(file_name, var_name, num_core)			\
static ssize_t show_##file_name						\
(struct kobject *kobj, struct attribute *attr, char *buf)		\
{									\
	struct hotplug_cpuinfo *pcpu_info =				\
			&per_cpu(od_hotplug_cpuinfo, num_core - 1);	\
	return sprintf(buf, "%u\n",					\
			pcpu_info->var_name);				\
}

show_pcpu_param(hotplug_freq_1_1, up_freq, 1);
show_pcpu_param(hotplug_freq_2_1, up_freq, 2);
show_pcpu_param(hotplug_freq_3_1, up_freq, 3);
show_pcpu_param(hotplug_freq_2_0, down_freq, 2);
show_pcpu_param(hotplug_freq_3_0, down_freq, 3);
show_pcpu_param(hotplug_freq_4_0, down_freq, 4);

show_pcpu_param(hotplug_load_1_1, up_load, 1);
show_pcpu_param(hotplug_load_2_1, up_load, 2);
show_pcpu_param(hotplug_load_3_1, up_load, 3);
show_pcpu_param(hotplug_load_2_0, down_load, 2);
show_pcpu_param(hotplug_load_3_0, down_load, 3);
show_pcpu_param(hotplug_load_4_0, down_load, 4);

show_pcpu_param(hotplug_rq_1_1, up_rq, 1);
show_pcpu_param(hotplug_rq_2_1, up_rq, 2);
show_pcpu_param(hotplug_rq_3_1, up_rq, 3);
show_pcpu_param(hotplug_rq_2_0, down_rq, 2);
show_pcpu_param(hotplug_rq_3_0, down_rq, 3);
show_pcpu_param(hotplug_rq_4_0, down_rq, 4);

show_pcpu_param(hotplug_rate_1_1, up_rate, 1);
show_pcpu_param(hotplug_rate_2_1, up_rate, 2);
show_pcpu_param(hotplug_rate_3_1, up_rate, 3);
show_pcpu_param(hotplug_rate_2_0, down_rate, 2);
show_pcpu_param(hotplug_rate_3_0, down_rate, 3);
show_pcpu_param(hotplug_rate_4_0, down_rate, 4);

#define store_pcpu_param(file_name, var_name, num_core)			\
static ssize_t store_##file_name					\
(struct kobject *kobj, struct attribute *attr,				\
	const char *buf, size_t count)					\
{									\
	unsigned int input;						\
	struct hotplug_cpuinfo *pcpu_info;				\
	int ret;							\
									\
	ret = sscanf(buf, "%u", &input);				\
	if (ret != 1)							\
		return -EINVAL;						\
									\
	pcpu_info = &per_cpu(od_hotplug_cpuinfo, num_core - 1);		\
									\
	if (input == pcpu_info->var_name) {				\
		return count;						\
	}								\
									\
	pcpu_info->var_name = input;					\
	return count;							\
}

store_pcpu_param(hotplug_freq_1_1, up_freq, 1);
store_pcpu_param(hotplug_freq_2_1, up_freq, 2);
store_pcpu_param(hotplug_freq_3_1, up_freq, 3);
store_pcpu_param(hotplug_freq_2_0, down_freq, 2);
store_pcpu_param(hotplug_freq_3_0, down_freq, 3);
store_pcpu_param(hotplug_freq_4_0, down_freq, 4);

store_pcpu_param(hotplug_load_1_1, up_load, 1);
store_pcpu_param(hotplug_load_2_1, up_load, 2);
store_pcpu_param(hotplug_load_3_1, up_load, 3);
store_pcpu_param(hotplug_load_2_0, down_load, 2);
store_pcpu_param(hotplug_load_3_0, down_load, 3);
store_pcpu_param(hotplug_load_4_0, down_load, 4);

store_pcpu_param(hotplug_rq_1_1, up_rq, 1);
store_pcpu_param(hotplug_rq_2_1, up_rq, 2);
store_pcpu_param(hotplug_rq_3_1, up_rq, 3);
store_pcpu_param(hotplug_rq_2_0, down_rq, 2);
store_pcpu_param(hotplug_rq_3_0, down_rq, 3);
store_pcpu_param(hotplug_rq_4_0, down_rq, 4);

store_pcpu_param(hotplug_rate_1_1, up_rate, 1);
store_pcpu_param(hotplug_rate_2_1, up_rate, 2);
store_pcpu_param(hotplug_rate_3_1, up_rate, 3);
store_pcpu_param(hotplug_rate_2_0, down_rate, 2);
store_pcpu_param(hotplug_rate_3_0, down_rate, 3);
store_pcpu_param(hotplug_rate_4_0, down_rate, 4);

define_one_global_rw(hotplug_freq_1_1);
define_one_global_rw(hotplug_freq_2_0);
define_one_global_rw(hotplug_freq_2_1);
define_one_global_rw(hotplug_freq_3_0);
define_one_global_rw(hotplug_freq_3_1);
define_one_global_rw(hotplug_freq_4_0);

define_one_global_rw(hotplug_load_1_1);
define_one_global_rw(hotplug_load_2_0);
define_one_global_rw(hotplug_load_2_1);
define_one_global_rw(hotplug_load_3_0);
define_one_global_rw(hotplug_load_3_1);
define_one_global_rw(hotplug_load_4_0);

define_one_global_rw(hotplug_rq_1_1);
define_one_global_rw(hotplug_rq_2_0);
define_one_global_rw(hotplug_rq_2_1);
define_one_global_rw(hotplug_rq_3_0);
define_one_global_rw(hotplug_rq_3_1);
define_one_global_rw(hotplug_rq_4_0);

define_one_global_rw(hotplug_rate_1_1);
define_one_global_rw(hotplug_rate_2_0);
define_one_global_rw(hotplug_rate_2_1);
define_one_global_rw(hotplug_rate_3_0);
define_one_global_rw(hotplug_rate_3_1);
define_one_global_rw(hotplug_rate_4_0);

static void cpus_hotplugging(int status)
{
	if (status) {
		hotplug_start();
	} else {
		hotplug_stop();
	}

	hotplug_tuners_ins.hotplug_enable = status;
}

/* hotplug_sampling_rate */
static ssize_t store_hotplug_sampling_rate(struct kobject *a,
				struct attribute *b,
				const char *buf, size_t count)
{
	int input;
	int ret;

	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	input = max(input, 10);

	if (input == hotplug_tuners_ins.hotplug_sampling_rate)
		return count;

	hotplug_tuners_ins.hotplug_sampling_rate = input;

	return count;
}

/* hotplug_enable */
static ssize_t store_hotplug_enable(struct kobject *a, struct attribute *b,
				  const char *buf, size_t count)
{
	int input;
	int ret;

	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	input = input > 0;

	if (hotplug_tuners_ins.hotplug_enable == input)
		return count;

	if (input > 0)
		cpus_hotplugging(1);
	else
		cpus_hotplugging(0);

	return count;
}

/* min_cpus_online */
static ssize_t store_min_cpus_online(struct kobject *a, struct attribute *b,
				  const char *buf, size_t count)
{
	int input;
	int ret;

	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	input = max(input > NR_CPUS ? NR_CPUS : input, 1);

	if (hotplug_tuners_ins.min_cpus_online == input)
		return count;

	hotplug_tuners_ins.min_cpus_online = input;

	return count;
}

/* max_cpus_online */
static ssize_t store_max_cpus_online(struct kobject *a, struct attribute *b,
				  const char *buf, size_t count)
{
	int input;
	int ret;

	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	input = max(input > NR_CPUS ? NR_CPUS : input, 1);

	if (hotplug_tuners_ins.max_cpus_online == input)
		return count;

	hotplug_tuners_ins.max_cpus_online = input;

	return count;
}

/* max_cpus_online_susp */
static ssize_t store_max_cpus_online_susp(struct kobject *a,
				struct attribute *b,
				const char *buf, size_t count)
{
	int input;
	int ret;

	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	input = max(input > NR_CPUS ? NR_CPUS : input, 1);

	if (hotplug_tuners_ins.max_cpus_online_susp == input)
		return count;

	hotplug_tuners_ins.max_cpus_online_susp = input;

	return count;
}

/* hp_io_is_busy */
static ssize_t store_hp_io_is_busy(struct kobject *a, struct attribute *b,
				   const char *buf, size_t count)
{
	unsigned int input, j;
	int ret;

	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	if (input > 1)
		input = 1;

	if (input == hotplug_tuners_ins.hp_io_is_busy)
		return count;

	hotplug_tuners_ins.hp_io_is_busy = !!input;
	/* we need to re-evaluate prev_cpu_idle */
	if (hotplug_tuners_ins.hotplug_enable > 0) {
		for_each_online_cpu(j) {
			struct hotplug_cpuinfo *pcpu_info =
					&per_cpu(od_hotplug_cpuinfo, j);
			pcpu_info->prev_cpu_idle = get_cpu_idle_time(j,
					&pcpu_info->prev_cpu_wall,
					hotplug_tuners_ins.hp_io_is_busy);
		}
	}

	return count;
}

/*
 * hotplug_suspend control
 * if set = 1 hotplug will sleep,
 * if set = 0, then hoplug will be active all the time.
 */
static ssize_t store_hotplug_suspend(struct kobject *a,
				struct attribute *b,
				const char *buf, size_t count)
{
	int input;
	int ret;

	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	input = input > 0;

	if (hotplug_tuners_ins.hotplug_suspend == input)
		return count;

	if (input > 0)
		hotplug_tuners_ins.hotplug_suspend = 1;
	else
		hotplug_tuners_ins.hotplug_suspend = 0;

	return count;
}

define_one_global_rw(hotplug_sampling_rate);
define_one_global_rw(hotplug_enable);
define_one_global_rw(min_cpus_online);
define_one_global_rw(max_cpus_online);
define_one_global_rw(max_cpus_online_susp);
define_one_global_rw(hp_io_is_busy);
define_one_global_rw(hotplug_suspend);

static struct attribute *alucard_hotplug_attributes[] = {
	&hotplug_sampling_rate.attr,
	&hotplug_enable.attr,
	&hotplug_freq_1_1.attr,
	&hotplug_freq_2_0.attr,
	&hotplug_freq_2_1.attr,
	&hotplug_freq_3_0.attr,
	&hotplug_freq_3_1.attr,
	&hotplug_freq_4_0.attr,
	&hotplug_load_1_1.attr,
	&hotplug_load_2_0.attr,
	&hotplug_load_2_1.attr,
	&hotplug_load_3_0.attr,
	&hotplug_load_3_1.attr,
	&hotplug_load_4_0.attr,
	&hotplug_rq_1_1.attr,
	&hotplug_rq_2_0.attr,
	&hotplug_rq_2_1.attr,
	&hotplug_rq_3_0.attr,
	&hotplug_rq_3_1.attr,
	&hotplug_rq_4_0.attr,
	&hotplug_rate_1_1.attr,
	&hotplug_rate_2_0.attr,
	&hotplug_rate_2_1.attr,
	&hotplug_rate_3_0.attr,
	&hotplug_rate_3_1.attr,
	&hotplug_rate_4_0.attr,
	&min_cpus_online.attr,
	&max_cpus_online.attr,
	&max_cpus_online_susp.attr,
	&hp_io_is_busy.attr,
	&hotplug_suspend.attr,
	NULL
};

static struct attribute_group alucard_hotplug_attr_group = {
	.attrs = alucard_hotplug_attributes,
	.name = "alucard_hotplug",
};

static int __init alucard_hotplug_init(void)
{
	int ret;
	unsigned int cpu;
	unsigned int hotplug_freq[NR_CPUS][2] = {
		{0, 1350000},
		{918000, 1566000},
		{1026000, 1674000},
		{1134000, 0}
	};
	unsigned int hotplug_load[NR_CPUS][2] = {
		{0, 60},
		{30, 65},
		{30, 65},
		{30, 0}
	};
	unsigned int hotplug_rq[NR_CPUS][2] = {
		{0, 100},
		{100, 200},
		{200, 300},
		{300, 0}
	};
	unsigned int hotplug_rate[NR_CPUS][2] = {
		{1, 1},
		{4, 1},
		{4, 1},
		{4, 1}
	};

	ret = sysfs_create_group(kernel_kobj, &alucard_hotplug_attr_group);
	if (ret) {
		printk(KERN_ERR "failed at(%d)\n", __LINE__);
		return ret;
	}

	/* INITIALIZE PCPU VARS */
	for_each_possible_cpu(cpu) {
		struct hotplug_cpuinfo *pcpu_info =
				&per_cpu(od_hotplug_cpuinfo, cpu);

		pcpu_info->up_freq = hotplug_freq[cpu][UP_INDEX];
		pcpu_info->down_freq = hotplug_freq[cpu][DOWN_INDEX];
		pcpu_info->up_load = hotplug_load[cpu][UP_INDEX];
		pcpu_info->down_load = hotplug_load[cpu][DOWN_INDEX];
		pcpu_info->up_rq = hotplug_rq[cpu][UP_INDEX];
		pcpu_info->down_rq = hotplug_rq[cpu][DOWN_INDEX];
		pcpu_info->up_rate = hotplug_rate[cpu][UP_INDEX];
		pcpu_info->down_rate = hotplug_rate[cpu][DOWN_INDEX];
	}

	if (hotplug_tuners_ins.hotplug_enable > 0) {
		hotplug_start();
	}

	return ret;
}

static void __exit alucard_hotplug_exit(void)
{
	if (hotplug_tuners_ins.hotplug_enable > 0) {
		hotplug_stop();
	}

	sysfs_remove_group(kernel_kobj, &alucard_hotplug_attr_group);
}
MODULE_AUTHOR("Alucard_24@XDA");
MODULE_DESCRIPTION("'alucard_hotplug' - A cpu hotplug driver for "
	"capable processors");
MODULE_LICENSE("GPL");

late_initcall(alucard_hotplug_init);
