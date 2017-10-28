/* Copyright (c) 2017, Rob Patershuk <robpatershuk@gmail.com>. All rights reserved.
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
 * MX Hotplug - A hotplugging driver that plugs cores based on
 * nr_running requests and load averages.
 * Thanks to Thalamus and Steve Loebrich for the inspiration!
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/cpu.h>
#include <linux/workqueue.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/mutex.h>
#include <linux/cpufreq.h>
#include <linux/kobject.h>
#include <linux/sysfs_helpers.h>
#include <linux/powersuspend.h>

#define MXMS(x) ((((x) * MSEC_PER_SEC) / MSEC_PER_SEC))
#define MX_SAMPLE_RATE MXMS(100UL)
#define MX_Q_RATE MXMS(100UL)
static unsigned int mx_hotplug_active;
static DEFINE_RWLOCK(mxhp_lock);
static DEFINE_SPINLOCK(timer_lock);
static DEFINE_MUTEX(mx_mutex);

static bool hotplug_suspended;

static struct workqueue_struct *transmission;
static struct delayed_work gearshaft;
static struct work_struct mx_hotplug_start;
static struct work_struct mx_hotplug_stop;
static struct task_struct *mx_hp_engine;

static unsigned long boost_threshold = 2000;
static unsigned long fifthgear = 1000;
static unsigned long reverse = 900;
static unsigned long ebrake = 650;
static unsigned long sampling_rate = MX_SAMPLE_RATE;
static unsigned int min_cpus_online = 2;
static unsigned int max_cpus_online = NR_CPUS;
static unsigned int cpus_boosted = NR_CPUS;
unsigned long air_to_fuel;
unsigned int pistons;
static ktime_t last_fuelcheck;
static bool wip;

static unsigned int mxread(void)
{
	unsigned int ret;
	unsigned long flags;

	read_lock_irqsave(&mxhp_lock, flags);
	ret = mx_hotplug_active;
	read_unlock_irqrestore(&mxhp_lock, flags);

	return ret;
}
static void mx_lock(int lock)
{
	unsigned long flags = 0;

	if (lock)
		write_lock_irqsave(&mxhp_lock, flags);
	else
		write_unlock_irqrestore(&mxhp_lock, flags);
}

static void _mxget(void)
{
	mx_hotplug_active = 1;
}

static void mxget(void)
{
	mx_lock(1);
	_mxget();
	mx_lock(0);
}

static void _mxput(void)
{
	mx_hotplug_active = 0;
}

static void mxput(void)
{
	mx_lock(1);
	_mxput();
	mx_lock(0);
}

void inject_nos(bool from_input)
{
	unsigned int cpu;
	int ret;

	if (!mxread() || hotplug_suspended)
		return;

	if (from_input) {
		for_each_nonboot_offline_cpu(cpu) {
			if (cpu_out_of_range_hp(cpu) ||
				num_online_cpus() == cpus_boosted)
				break;
			if (cpu_online(cpu) ||
				!is_cpu_allowed(cpu) ||
				thermal_core_controlled(cpu))
				continue;
		cpu_up(cpu);
		}
	} else {
		for_each_nonboot_offline_cpu(cpu) {
			if (cpu_out_of_range_hp(cpu) ||
				num_online_cpus() == max_cpus_online)
				break;
			if (cpu_online(cpu) ||
				!is_cpu_allowed(cpu) ||
				thermal_core_controlled(cpu))
				continue;
		cpu_up(cpu);
		}
	}
	
}

static void step_on_it(unsigned int nrcores)
{
	unsigned int cpu;

	if (!mxread() || hotplug_suspended)
		return;

	sanitize_min_max(nrcores, min_cpus_online, max_cpus_online);
	for_each_nonboot_offline_cpu(cpu) {
		if (cpu_out_of_range_hp(cpu) ||
			num_online_cpus() == nrcores)
			break;
		if (cpu_online(cpu) ||
			!is_cpu_allowed(cpu) ||
			thermal_core_controlled(cpu))
			continue;
		cpu_up(cpu);
	}
}

static void hit_the_brakes(unsigned int nrcores)
{
	unsigned int cpu;

	if (!mxread() || hotplug_suspended)
		return;

	sanitize_min_max(nrcores, min_cpus_online, max_cpus_online);
	for_each_nonboot_online_cpu(cpu) {
		if (cpu_out_of_range_hp(cpu) ||
			num_online_cpus() == nrcores)
			break;
		if (!cpu_online(cpu) ||
			!is_cpu_allowed(cpu) ||
			thermal_core_controlled(cpu))
			continue;
		cpu_down(cpu);
	}
}

static void reset_wip(void)
{
	unsigned long flags;
	spin_lock_irqsave(&timer_lock, flags);
	if (wip)
		wip = false;
	spin_unlock_irqrestore(&timer_lock, flags);	
}

static int __ref machinex_hotplug_engine(void *data)
{
	unsigned long flags;
	unsigned int cpu;
	ktime_t delta;

again:
	set_current_state(TASK_INTERRUPTIBLE);
	mutex_lock(&mx_mutex);
	spin_lock_irqsave(&timer_lock, flags);
	delta = ktime_sub(ktime_get(), last_fuelcheck);
	if (ktime_compare(delta, ms_to_ktime(sampling_rate))  < 0 ||
		!wip) {
		spin_unlock_irqrestore(&timer_lock, flags);
		mutex_unlock(&mx_mutex);
		schedule();
	} else {
		spin_unlock_irqrestore(&timer_lock, flags);
		mutex_unlock(&mx_mutex);
	}

	if (kthread_should_stop())
		inject_nos(false);
		return 0;

	if (kthread_should_park()) {
		inject_nos(false);
		kthread_parkme();
	}

	mutex_lock(&mx_mutex);
	set_current_state(TASK_RUNNING);

	pistons = num_online_cpus();
	air_to_fuel = avg_nr_running();

	if (air_to_fuel > boost_threshold) {
		if (pistons < max_cpus_online &&
			pistons >= min_cpus_online) {
			inject_nos(false);
			goto purge;
		}
	}

	if (air_to_fuel > fifthgear) {
		if (pistons < max_cpus_online)
			step_on_it(pistons + 1);
	} else if (air_to_fuel < reverse && air_to_fuel > ebrake) {
		if (pistons > min_cpus_online)
			hit_the_brakes(pistons - 1);
	} else if (air_to_fuel < ebrake) {
		if (pistons > min_cpus_online)
			hit_the_brakes(min_cpus_online);
	}
purge:
	spin_lock_irqsave(&timer_lock, flags);
	wip = false;
	spin_unlock_irqrestore(&timer_lock, flags);
	mutex_unlock(&mx_mutex);
	goto again;
}

static void gearshift(struct work_struct *work)
{
	unsigned long flags;
	if (hotplug_suspended ||
		!mxread())
		return;

	if (!mutex_trylock(&mx_mutex))
		goto out;

	spin_lock_irqsave(&timer_lock, flags);
	if (wip) {
		spin_unlock_irqrestore(&timer_lock, flags);
		mutex_unlock(&mx_mutex);
		goto out;
	}
	last_fuelcheck = ktime_get();
	wip = true;
	spin_unlock_irqrestore(&timer_lock, flags);
	mutex_unlock(&mx_mutex);

	wake_up_process(mx_hp_engine);

out:
	queue_delayed_work_on(0, transmission, &gearshaft, sampling_rate);
}
	

static void mx_hotplug_suspend(struct power_suspend *h)
{
	hotplug_suspended = true;
	cancel_delayed_work_sync(&gearshaft);
	kthread_park(mx_hp_engine);
}

static void mx_hotplug_resume(struct power_suspend *h)
{
	hotplug_suspended = false;
	reset_wip();
	kthread_unpark(mx_hp_engine);
	queue_delayed_work_on(0, transmission, &gearshaft, sampling_rate);
}

static struct power_suspend mx_suspend_data =
{
	.suspend = mx_hotplug_suspend,
	.resume = mx_hotplug_resume,
};

static void ignition(struct work_struct *work)
{
	struct sched_param param = { .sched_priority = MAX_RT_PRIO - 1 };

	mxget();
	reset_wip();
	transmission = create_singlethread_workqueue("tmission");
	if (!transmission) {
		pr_err("MX HOTPLUG: Failed to allocate hotplug workqueue\n");
		mxput();
		return;
	}

	INIT_DELAYED_WORK(&gearshaft, gearshift);

	mx_hp_engine = kthread_create(machinex_hotplug_engine,
					  NULL, "machinex_hp");
	if (IS_ERR(mx_hp_engine)) {
		pr_err("MX Hotplug: Failed to create bound kthread! Driver is broken!\n");
//		return PTR_ERR(mx_hp_engine);
		mxput();
		return;
	}
	kthread_bind(mx_hp_engine, 0);
	sched_setscheduler_nocheck(mx_hp_engine, SCHED_FIFO, &param);
	get_task_struct(mx_hp_engine);
	wake_up_process(mx_hp_engine);
	queue_delayed_work_on(0, transmission, &gearshaft, sampling_rate);
	register_power_suspend(&mx_suspend_data);
	return;
}

static void killswitch(struct work_struct *work)
{
	mxput();
	unregister_power_suspend(&mx_suspend_data);
	cancel_delayed_work_sync(&gearshaft);
	destroy_workqueue(transmission);
	kthread_stop(mx_hp_engine);
	put_task_struct(mx_hp_engine);
}

static void mx_startstop(unsigned int status)
{
	if (status) {
		schedule_work_on(0, &mx_hotplug_start);
	} else {
		schedule_work_on(0, &mx_hotplug_stop);
	}
}

mx_show_one(mx_hotplug_active);
mx_show_one(min_cpus_online);
mx_show_one(max_cpus_online);
mx_show_one(cpus_boosted);

static ssize_t store_mx_hotplug_active(struct kobject *kobj,
					 struct kobj_attribute *attr,
					 const char *buf, size_t count)
{
	int ret;
	int input;
	unsigned int tmpread;

	ret = sscanf(buf, "%d", &input);
	if (ret < 0)
		return ret;

	sanitize_min_max(input, 0, 1);
	tmpread = mxread();

	if (input == tmpread)
		return count;

	mx_startstop(input);

	return count;
}


static ssize_t store_min_cpus_online(struct kobject *kobj,
				     struct kobj_attribute *attr,
				     const char *buf, size_t count)
{
	int ret;
	unsigned int val;

	ret = sscanf(buf, "%u", &val);
	if (ret != 1)
		return -EINVAL;

	if (val <= 1)
		val = 1;
	if (val >= NR_CPUS)
		val = NR_CPUS;
	if (val >= max_cpus_online)
		val = max_cpus_online;

	min_cpus_online = val;

	return count;
}

static ssize_t store_max_cpus_online(struct kobject *kobj,
				     struct kobj_attribute *attr,
				     const char *buf, size_t count)
{
	int ret;
	unsigned int val;

	ret = sscanf(buf, "%u", &val);
	if (ret != 1)
		return -EINVAL;

	if (val <= 1)
		val = 1;
	if (val >= NR_CPUS)
		val = NR_CPUS;
	if (val <= min_cpus_online)
		val = min_cpus_online;

	max_cpus_online = val;

	return count;
}

static ssize_t store_cpus_boosted(struct kobject *kobj,
				     struct kobj_attribute *attr,
				     const char *buf, size_t count)
{
	int ret;
	unsigned int val;

	ret = sscanf(buf, "%u", &val);
	if (ret != 1)
		return -EINVAL;

	sanitize_min_max(val, min_cpus_online, max_cpus_online);

	cpus_boosted = val;

	return count;
}

MX_ATTR_RW(mx_hotplug_active);
MX_ATTR_RW(min_cpus_online);
MX_ATTR_RW(max_cpus_online);
MX_ATTR_RW(cpus_boosted);

static struct attribute *mx_hotplug_attributes[] = {
	&mx_hotplug_active_attr.attr,
	&min_cpus_online_attr.attr,
	&max_cpus_online_attr.attr,
	&cpus_boosted_attr.attr,
	NULL,
};

static struct attribute_group mx_hotplug_attr_group = {
	.attrs = mx_hotplug_attributes,
	.name = "mx_hotplug",
};

static int mx_hotplug_init(void)
{
	int sysfs_result;

	INIT_WORK(&mx_hotplug_start, ignition);
	INIT_WORK(&mx_hotplug_stop, killswitch);

	sysfs_result = sysfs_create_group(kernel_kobj,
		&mx_hotplug_attr_group);

	if (sysfs_result) {
		pr_info("%s group create failed!\n", __FUNCTION__);
		return -ENOMEM;
	}
	return 0;
}

late_initcall(mx_hotplug_init);
MODULE_AUTHOR("Rob Patershuk <robpatershuk@gmail.com>");
MODULE_DESCRIPTION("'mx_hotplug' - An rq based hotplug driver");
MODULE_LICENSE("GPLv2");