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
#include <linux/omniboost.h>
#include <linux/omniplug.h>

#define MX_SAMPLE_RATE 500UL
#define BOOST_LENGTH 350UL
static unsigned int mx_hotplug_active;
static DEFINE_RWLOCK(mxhp_lock);
static DEFINE_MUTEX(mx_mutex);

static bool hotplug_suspended;
static struct workqueue_struct *mx_hp_engine;
static struct delayed_work motor;
static struct task_struct *transmission;

static unsigned long sixthgear = 975ul;
static unsigned long thirdgear = 280ul;
static unsigned long secondgear = 190ul;
static unsigned long firstgear = 95ul;
static unsigned long sixthgear_rpm = 65ul;
static unsigned long thirdgear_rpm = 50ul;
static unsigned long secondgear_rpm = 35ul;
static unsigned long firstgear_rpm = 20ul;

static unsigned long sampling_rate = MX_SAMPLE_RATE;
unsigned long air_to_fuel;
unsigned long current_rpm;
static unsigned long boost_timeout = BOOST_LENGTH;
static ktime_t last_fuelcheck;
static ktime_t last_boost;
static bool clutch;
static bool should_boost;

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

void inject_nos(bool from_input, bool last_uptick)
{
	unsigned int cpu, cylinders;
	int ret;

	if (!last_uptick && (!mxread() || hotplug_suspended))
		return;

	if (!from_input) {
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
	} else {
		cylinders = cpus_boosted;
		sanitize_min_max(cylinders, min_cpus_online, max_cpus_online);
		for_each_nonboot_offline_cpu(cpu) {
			if (cpu_out_of_range_hp(cpu) ||
				num_online_cpus() == cylinders)
				break;
			if (cpu_online(cpu) ||
				!is_cpu_allowed(cpu) ||
				thermal_core_controlled(cpu))
				continue;
		cpu_up(cpu);
		}
	}
}

static void upshift(void)
{
	unsigned int cpu;

	if (!mxread() || hotplug_suspended)
		return;

	cpu = cpumask_next_zero(0, &__cpu_online_mask);
	if (cpu_out_of_range_hp(cpu) ||
		num_online_cpus() == max_cpus_online)
		return;
	if (cpu_online(cpu) ||
		!is_cpu_allowed(cpu) ||
		thermal_core_controlled(cpu))
		return;
	cpu_up(cpu);
}

static void downshift(void)
{
	unsigned int cpu;

	if (!mxread() || hotplug_suspended)
		return;

	cpu = cpumask_next(0, &__cpu_online_mask);
	if (cpu_out_of_range_hp(cpu) ||
		num_online_cpus() == min_cpus_online)
		return;
	if (!cpu_online(cpu))
		return;
	cpu_down(cpu);
}

static void hit_the_brakes(void)
{
	unsigned int cpu;

	if (!mxread() || hotplug_suspended)
		return;

	for_each_nonboot_online_cpu(cpu) {
		if (cpu_out_of_range_hp(cpu) ||
			num_online_cpus() == min_cpus_online)
			break;
		if (!cpu_online(cpu))
			continue;
		cpu_down(cpu);
	}
}

static void release_clutch(void)
{
	mutex_lock(&mx_mutex);
	clutch = false;
	mutex_unlock(&mx_mutex);
}

static int __ref mx_gearbox(void *data)
{
	ktime_t delta;

again:
	set_current_state(TASK_INTERRUPTIBLE);

	if (kthread_should_stop()) {
		mutex_lock(&mx_mutex);
		inject_nos(false, true);
		mutex_unlock(&mx_mutex);
		return 0;
	}

	mutex_lock(&mx_mutex);
	delta = ktime_sub(ktime_get(), last_fuelcheck);
	if ((!should_boost && ktime_compare(delta, ms_to_ktime(sampling_rate))  < 0) ||
		!clutch || hotplug_suspended) {
		mutex_unlock(&mx_mutex);
		schedule();
		mutex_lock(&mx_mutex);
	}

	set_current_state(TASK_RUNNING);

	if (should_boost) {
		inject_nos(true, false);
		last_boost = ktime_get();
		should_boost = false;
		goto purge;
	}

	WRITE_ONCE(air_to_fuel, avg_nr_running());
	WRITE_ONCE(current_rpm, all_cpu_load());
	if (air_to_fuel >= sixthgear ||
		current_rpm >= sixthgear_rpm)
		inject_nos(false, false);
	else if ((air_to_fuel >= thirdgear &&
				air_to_fuel < sixthgear) ||
			   (current_rpm >= thirdgear_rpm &&
				current_rpm < sixthgear_rpm))
		upshift();
	else if ((air_to_fuel > firstgear &&
				air_to_fuel <= secondgear) &&
			   (current_rpm > firstgear_rpm &&
				current_rpm <= secondgear_rpm))
		downshift();
	else if ((air_to_fuel <= firstgear) &&
			   (current_rpm <= firstgear_rpm))
		hit_the_brakes();
purge:
	clutch = false;
	mutex_unlock(&mx_mutex);
	goto again;
}

static void mx_motor(struct work_struct *work)
{
	unsigned long flags;

	if (!mxread())
		return;

	if (!mutex_trylock(&mx_mutex))
		goto out;

	if (hotplug_suspended) {
		mutex_unlock(&mx_mutex);
		return;
	}

	if (clutch) {
		mutex_unlock(&mx_mutex);
		goto out;
	}

	last_fuelcheck = ktime_get();
	clutch = true;
	mutex_unlock(&mx_mutex);
	wake_up_process(transmission);
out:
	queue_delayed_work_on(0, mx_hp_engine, &motor, msecs_to_jiffies(sampling_rate));
}

void fuel_injector(void)
{
	ktime_t delta;

	if (!mxread() || hotplug_suspended)
		return;

	if (!mutex_trylock(&mx_mutex))
		return;

	delta = ktime_sub(ktime_get(), last_boost);
	if (ktime_compare(delta, ms_to_ktime(boost_timeout)) < 0) {
		mutex_unlock(&mx_mutex);
		return;
	}

	if (!should_boost) {
		should_boost = true;
		mutex_unlock(&mx_mutex);
		mod_delayed_work_on(0, mx_hp_engine, &motor, 0);
	} else
		mutex_unlock(&mx_mutex);
}
	

static void mx_hotplug_suspend(struct power_suspend *h)
{
	mutex_lock(&mx_mutex);
	inject_nos(false, true);
	hotplug_suspended = true;
	mutex_unlock(&mx_mutex);
	cancel_delayed_work_sync(&motor);
}

static void mx_hotplug_resume(struct power_suspend *h)
{
	mutex_lock(&mx_mutex);
	hotplug_suspended = false;
	mutex_unlock(&mx_mutex);
	release_clutch();
	queue_delayed_work_on(0, mx_hp_engine, &motor, msecs_to_jiffies(sampling_rate));
}

static struct power_suspend mx_suspend_data =
{
	.suspend = mx_hotplug_suspend,
	.resume = mx_hotplug_resume,
};

static int mx_omniboost_notifier(struct notifier_block *self, unsigned long val,
		void *v)
{
	switch (val) {
	case BOOST_ON:
		fuel_injector();
		break;
	case BOOST_OFF:
		break;
	default:
		break;
	}
	return NOTIFY_OK;
}

static struct notifier_block mx_nb = {
	.notifier_call = mx_omniboost_notifier,
};

static int mx_thermal_notifier(struct notifier_block *self, unsigned long val,
		void *v)
{
	struct sched_param defparam = { .sched_priority = DEFAULT_PRIO };
	struct sched_param rtparam = { .sched_priority = MAX_USER_RT_PRIO / 2 };

	switch (val) {
	case THROTTLING_ON:
		sched_setscheduler_nocheck(transmission, SCHED_NORMAL, &defparam);
		break;
	case THROTTLING_OFF:
		sched_setscheduler_nocheck(transmission, SCHED_FIFO, &rtparam);
		break;
	default:
		break;
	}
	return NOTIFY_OK;
}

static struct notifier_block mxtherm_nb = {
	.notifier_call = mx_thermal_notifier,
};

void ignition(unsigned int status)
{
	if (status) {
		mxget();
		transmission = kthread_create(mx_gearbox,
						  NULL, "mx_transmission");
		if (IS_ERR(transmission)) {
			pr_err("MX Hotplug: Failed to create bound kthread! Driver is broken!\n");
			mxput();
			return;
		}
		kthread_bind(transmission, 0);
		get_task_struct(transmission);
		wake_up_process(transmission);
		mx_hp_engine = create_singlethread_workqueue("mx_engine");
		if (!mx_hp_engine) {
			pr_err("MX HOTPLUG: Failed to allocate hotplug workqueue\n");
			mxput();
			kthread_stop(transmission);
			put_task_struct(transmission);
			return;
		}

		INIT_DELAYED_WORK(&motor, mx_motor);
		queue_delayed_work_on(0, mx_hp_engine, &motor, msecs_to_jiffies(sampling_rate));
		register_power_suspend(&mx_suspend_data);
		register_omniboost(&mx_nb);
		register_thermal_notifier(&mxtherm_nb);
	} else {
		mxput();
		unregister_thermal_notifier(&mxtherm_nb);
		unregister_omniboost(&mx_nb);
		unregister_power_suspend(&mx_suspend_data);
		cancel_delayed_work_sync(&motor);
		destroy_workqueue(mx_hp_engine);
		kthread_stop(transmission);
		put_task_struct(transmission);
		release_clutch();
	}
}

static ssize_t store_external_fuel_injection(struct kobject *kobj,
					 struct kobj_attribute *attr,
					 const char *buf, size_t count)
{
	int ret;
	int input;

	ret = sscanf(buf, "%d", &input);
	if (ret < 0)
		return ret;

	sanitize_min_max(input, 0, 1);

	if (input)
		fuel_injector();

	return count;
}

mx_store_one_long(sixthgear, 25, 10000);
mx_store_one_long(thirdgear, 25, 10000);
mx_store_one_long(secondgear, 25, 10000);
mx_store_one_long(firstgear, 25, 10000);
mx_store_one_long(sixthgear_rpm, 5, 100);
mx_store_one_long(thirdgear_rpm, 5, 100);
mx_store_one_long(secondgear_rpm, 5, 100);
mx_store_one_long(firstgear_rpm, 5, 100);

mx_show_long(sixthgear);
mx_show_long(thirdgear);
mx_show_long(secondgear);
mx_show_long(firstgear);
mx_show_long(sixthgear_rpm);
mx_show_long(thirdgear_rpm);
mx_show_long(secondgear_rpm);
mx_show_long(firstgear_rpm);

MX_ATTR_WO(external_fuel_injection);
MX_ATTR_RW(sixthgear);
MX_ATTR_RW(thirdgear);
MX_ATTR_RW(secondgear);
MX_ATTR_RW(firstgear);
MX_ATTR_RW(sixthgear_rpm);
MX_ATTR_RW(thirdgear_rpm);
MX_ATTR_RW(secondgear_rpm);
MX_ATTR_RW(firstgear_rpm);

static struct attribute *mx_hotplug_attributes[] = {
	&external_fuel_injection_attr.attr,
	&sixthgear_attr.attr,
	&thirdgear_attr.attr,
	&secondgear_attr.attr,
	&firstgear_attr.attr,
	&sixthgear_rpm_attr.attr,
	&thirdgear_rpm_attr.attr,
	&secondgear_rpm_attr.attr,
	&firstgear_rpm_attr.attr,
	NULL,
};

static struct attribute_group mx_hotplug_attr_group = {
	.attrs = mx_hotplug_attributes,
	.name = "mx_hotplug",
};

static int mx_hotplug_init(void)
{
	int sysfs_result;

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
