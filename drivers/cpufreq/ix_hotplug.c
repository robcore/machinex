/* Copyright (c) 2013, Steve Loebrich <sloebric@gmail.com>. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

/*
 * Generic auto hotplug driver for ARM SoCs. Targeted at current generation
 * SoCs with dual and quad core applications processors.
 * Automatically hotplugs online and offline CPUs based on system load.
 * It is also capable of immediately onlining a core based on an external
 * event by calling void hotplug_boostpulse(void)
 *
 * Not recommended for use with OMAP4460 due to the potential for lockups
 * whilst hotplugging.
 *
 * Thanks to Thalamus for the inspiration!
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/cpu.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/mutex.h>
#include <linux/cpufreq.h>
#include <linux/kobject.h>
#ifdef CONFIG_STATE_NOTIFIER
#include <linux/state_notifier.h>
#endif
#include <linux/machinex_defines.h>

#include <linux/powersuspend.h>

#define IX_HOTPLUG "ix_hotplug"

/*
 * Load defines:
 * ENABLE_ALL is a high watermark to rapidly online all CPUs
 *
 * ENABLE is the load which is required to enable 1 extra CPU
 * DISABLE is the load at which a CPU is disabled
 * These two are scaled based on num_online_cpus()
 */

static struct delayed_work hotplug_decision_work;
static struct work_struct suspend;
static struct work_struct resume;
static struct workqueue_struct *ixwq;
static bool hotplug_suspended = false;
static unsigned int hotplug_suspend = 0
static atomic_t ix_hotplug_active = ATOMIC_INIT(0);

static unsigned int enable_all_load = 800;
static unsigned int enable_load[5] = {0, 120, 220, 340, 0};
static unsigned int disable_load[5] = {0, 0, 60, 120, 260};
static unsigned int sample_rate[5] = {0, 25, 50, 100, 50};
static unsigned int online_sampling_periods[5] = {0, 3, 3, 5, 0};
static unsigned int offline_sampling_periods[5] = {0, 0, 8, 3, 4};
static unsigned int online_cpus;
static unsigned int min_cpus_online = DEFAULT_MIN_CPUS_ONLINE;
static unsigned int max_cpus_online = DEFAULT_MAX_CPUS_ONLINE;
static unsigned int sampling_rate;
static unsigned int available_cpus;
static unsigned int online_sample;
static unsigned int offline_sample;


static void hotplug_online_single_work(void)
{
	unsigned int cpuid = 0;

	if (hotplug_suspended)
		return;

	cpuid = cpumask_next_zero(0, cpu_online_mask);
	if (cpu_in_range_hp(cpuid))
		cpu_up(cpuid);
}

static void hotplug_online_all_work(void)
{
	unsigned int cpu;

	for_each_nonboot_offline_cpu(cpu) {
		if (cpu_out_of_range_hp(cpu))
			break;
		if (unlikely(cpu_online(cpu)))
			continue;
			cpu_up(cpu);
			//pr_info("ix_hotplug: CPU%d up.\n", cpu);
	}
	return;
}

static void hotplug_offline_work(void)
{
	unsigned int cpuid = 0;

	if (hotplug_suspended)
		return;

	cpuid = cpumask_next(0, cpu_online_mask);
	if (cpu_in_range_hp(cpuid))
		cpu_down(cpuid);
}

static void __ref hotplug_decision_work_fn(struct work_struct *work)
{
	unsigned int avg_running, io_wait;
	//unsigned int rq_depth;

	if (hotplug_suspended)
		return;

	sched_get_nr_running_avg(&avg_running, &io_wait);
	//rq_depth = rq_info.rq_avg;

	if ((avg_running <= disable_load[online_cpus]) &&
			(online_cpus > min_cpus_online)) {
		//pr_info("ix_hotplug: Disable Exit - %d %d %d %d %d\n", online_cpus, offline_sample, offline_sampling_periods[online_cpus], disable_load[online_cpus], avg_running);
		if (offline_sample >= offline_sampling_periods[online_cpus]) {
			//pr_info("ix_hotplug: Disable Single\n");
			hotplug_offline_work();
			offline_sample = 0;
			online_cpus = num_online_cpus();
			//pr_info("ix_hotplug: Threshold: %d Load: %d Sampling: %d RQ: %d\n", load_disable, avg_running, sampling_rate, rq_depth);
		}
		offline_sample++;
		online_sample = 1;
		goto exit;
	}

	if ((avg_running >= enable_all_load || avg_running >= enable_load[online_cpus]) &&
			(online_cpus < available_cpus)) {
		//pr_info("ix_hotplug: Enable Exit - %d %d %d %d %d\n", online_cpus, online_sample, online_sampling_periods[online_cpus], enable_load[online_cpus], avg_running);
		if (online_sample >= online_sampling_periods[online_cpus]) {
			if (avg_running >= enable_all_load) {
				//pr_info("ix_hotplug: Enable All\n");
				hotplug_online_all_work();
			} else {
				//pr_info("ix_hotplug: Enable Single\n");
				hotplug_online_single_work();
			}
			online_cpus = num_online_cpus();
			online_sample = 0;
		}
		online_sample++;
		offline_sample = 1;
		goto exit;
	}

	//pr_info("ix_hotplug: Idle, CPUs: %d %d\n", online_cpus, num_online_cpus());

exit:

	sampling_rate = sample_rate[online_cpus];

	//pr_info("ix_hotplug: CPUs: %d Load: %d Sampling: %d\n", online_cpus, avg_running, sampling_rate);

	queue_delayed_work_on(0, ixwq, &hotplug_decision_work, msecs_to_jiffies(sampling_rate));
}

static ssize_t show_enable_all_load(struct kobject *kobj,
				     struct attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", enable_all_load);
}

static ssize_t store_enable_all_load(struct kobject *kobj,
				  struct attribute *attr, const char *buf,
				  size_t count)
{
	int ret;
	long unsigned int val;

	ret = strict_strtoul(buf, 0, &val);
	if (ret < 0)
		return ret;
	enable_all_load = val;
	return count;
}

static struct global_attr enable_all_load_attr = __ATTR(enable_all_load, 0666,
		show_enable_all_load, store_enable_all_load);


static ssize_t show_disable_load(struct kobject *kobj,
				     struct attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", disable_load);
}

static ssize_t store_disable_load(struct kobject *kobj,
				  struct attribute *attr, const char *buf,
				  size_t count)
{
	int ret;
	long unsigned int val;

	ret = strict_strtoul(buf, 0, &val);
	if (ret < 0)
		return ret;
	disable_load = val;
	return count;
}

static struct global_attr disable_load_attr = __ATTR(disable_load, 0666,
		show_disable_load, store_disable_load);

static struct attribute *ix_hotplug_attributes[] = {
	&enable_all_load_attr.attr,
	&disable_load_attr.attr,
	NULL,
};

static struct attribute_group ix_hotplug_attr_group = {
	.attrs = ix_hotplug_attributes,
	.name = "ix_hotplug",
};

static void __ref ix_hotplug_suspend(struct work_struct *work)
{
		hotplug_suspended = true;
}

static void __ref ix_hotplug_resume(struct work_struct *work)
{
	offline_sample = 1;
	online_sample = 1;
	cpu_up(1);
	online_cpus = num_online_cpus();
	queue_delayed_work_on(0,  ixwq, &hotplug_decision_work, msecs_to_jiffies(2500));
	pr_info("ix_hotplug: Power Resume\n");
}

#ifdef CONFIG_STATE_NOTIFIER
static int state_notifier_callback(struct notifier_block *this,
				unsigned long event, void *data)
{
	if ((atomic_read(&ix_hotplug_active) == 0) ||
			hotplug_suspended)
		return NOTIFY_OK;

	switch (event) {
		case STATE_NOTIFIER_ACTIVE:
			ix_hotplug_resume();
			break;
		case STATE_NOTIFIER_SUSPEND:
			ix_hotplug_suspend();
			break;
		default:
			break;
	}

	return NOTIFY_OK;
}
#endif

static int __ref ix_hotplug_start(void)
{
	int sysfs_result;

	online_cpus = num_online_cpus();
	available_cpus = num_possible_cpus();

	ix_hotplug_kobj = kobject_create_and_add("ix_hotplug",
		kernel_kobj);

	if (!ix_hotplug_kobj) {
		pr_err("%s kobject create failed!\n", __FUNCTION__);
		return -ENOMEM;
	}

	sysfs_result = sysfs_create_group(ix_hotplug_kobj,
		&ix_hotplug_attr_group);

	if (sysfs_result) {
		pr_info("%s group create failed!\n", __FUNCTION__);
		kobject_put(ix_hotplug_kobj);
		return -ENOMEM;

	ixwq = alloc_workqueue("ix_hotplug_workqueue", WQ_HIGHPRI | WQ_FREEZABLE, 0);

    if (!ixwq)
        return -ENOMEM;

#ifdef CONFIG_STATE_NOTIFIER
	notif.notifier_call = state_notifier_callback;
	if (state_register_client(&notif)) {
		pr_err("%s: Failed to register State notifier callback\n",
			IX_HOTPLUG);
		goto err_dev;
	}
#endif

	mutex_init(&ix_hotplug_mutex);

	INIT_WORK(&suspend, ix_hotplug_suspend);
	INIT_WORK(&resume, ix_hotplug_resume);
	INIT_DELAYED_WORK(&hotplug_decision_work, hotplug_decision_work_fn);

	/*
	 * Give the system time to boot before fiddling with hotplugging.
	 */

	queue_delayed_work_on(0, ixwq, &hotplug_decision_work, msecs_to_jiffies(10000));

	return 0;
err_dev:
	destroy_workqueue(ix_hotplug_workqueue);
err_out:
	atomic_set(&ix_hotplug_active, 0);
	return ret;
}

static void ix_hotplug_stop(void)
{

	cancel_delayed_work_sync(&hotplug_decision_work);
	flush_work(&suspend_work);
	flush_work(&resume_work);
	mutex_destroy(&ix_hotplug_mutex);

#ifdef CONFIG_STATE_NOTIFIER
	state_unregister_client(&notif);
#endif
	notif.notifier_call = NULL;

	destroy_workqueue(ixwq);
}

late_initcall(ix_hotplug_init);
module_exit(ix_hotplug_exit);

MODULE_AUTHOR("InstigatorX, \
		Rob Patershuk <robpatershuk@gmail.com>");
MODULE_DESCRIPTION("'ix_hotplug' - A simple hotplug driver "
	"with full automation as a replacement for mpdecision");
MODULE_LICENSE("GPLv2");
