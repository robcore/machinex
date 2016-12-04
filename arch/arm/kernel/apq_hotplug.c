/*
 * Copyright (C) 2015 Tom G. <roboter972@gmail.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define pr_fmt(fmt) "apq_hotplug: " fmt

#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/powersuspend.h>
#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/stat.h>
#include <linux/sysfs.h>
#include <linux/workqueue.h>

#define DEBUG	0

enum hp {
	SUSPEND_DELAY = (CONFIG_HZ * 2),
	MAX_CPUS = CONFIG_NR_CPUS
};

static struct delayed_work offline_all_work;
static struct work_struct online_all_work;
static struct kobject *apq_hotplug_kobj;

static unsigned int boot_flag = 0;
static unsigned int suspend_delay = SUSPEND_DELAY;
unsigned int apq_hp_max_online_cpus = MAX_CPUS;

static inline void offline_all_fn(struct work_struct *work)
{
	unsigned int cpu;

	for_each_online_cpu(cpu) {
		if (cpu != 0) {
			cpu_down(cpu);
#if DEBUG
			pr_info("CPU%u down.\n", cpu);
			pr_info("CPU(s) running: %u\n", num_online_cpus());
#endif
		}
	}
}

static inline void online_all_fn(struct work_struct *work)
{
	unsigned int cpu;

	for_each_cpu_not_adj(cpu, cpu_online_mask) {
		if (cpu == 0)
			continue;
		cpu_up(cpu);
#if DEBUG
		pr_info("CPU%u up.\n", cpu);
		pr_info("CPU(s) running: %u\n", num_online_cpus());
#endif
	}
}

static void apq_hotplug_power_suspend(struct power_suspend *h)
{
	/*
	 * Init new work on the first suspend call,
	 * skip clearing workqueue as no work has been inited yet.
	 */
	switch (boot_flag) {
	case 0:
		cancel_work_sync(&online_all_work);
		break;
	case 1:
		boot_flag = 0;
		break;
	}

	schedule_delayed_work(&offline_all_work,
					msecs_to_jiffies(suspend_delay));
}

static void apq_hotplug_power_resume(struct power_suspend *h)
{
	/* Clear the workqueue and init new work */
	cancel_delayed_work_sync(&offline_all_work);

	schedule_work(&online_all_work);
}

static struct power_suspend __refdata apq_hotplug_power_suspend_handler = {
	.suspend = apq_hotplug_power_suspend,
	.resume = apq_hotplug_power_resume,
};

/******************************** SYSFS START ********************************/
static ssize_t max_online_cpus_show(struct kobject *kobj,
					struct kobj_attribute *attr,
					char *buf)
{
	return sprintf(buf, "%u\n", apq_hp_max_online_cpus);
}

static ssize_t max_online_cpus_store(struct kobject *kobj,
					struct kobj_attribute *attr,
					const char *buf, size_t count)
{
	unsigned int val;
	int ret;

	ret = sscanf(buf, "%u", &val);

	if (ret != 1 || val > MAX_CPUS)
		return -EINVAL;

	apq_hp_max_online_cpus = val;

	return count;
}

static struct kobj_attribute max_online_cpus_attribute =
	__ATTR(max_online_cpus, S_IRUGO | S_IWUSR, max_online_cpus_show,
						max_online_cpus_store);

static struct attribute *apq_hotplug_attrs[] = {
	&max_online_cpus_attribute.attr,
	NULL
};

static struct attribute_group apq_hotplug_attr_group = {
	.attrs = apq_hotplug_attrs,
};
/********************************* SYSFS END *********************************/

static int __init apq_hotplug_init(void)
{
	int rc;

	INIT_DELAYED_WORK(&offline_all_work, offline_all_fn);
	INIT_WORK(&online_all_work, online_all_fn);

	apq_hotplug_kobj = kobject_create_and_add("apq_hotplug", kernel_kobj);
	if (!apq_hotplug_kobj) {
		pr_err("Failed to create apq_hotplug kobject!\n");
		return -ENOMEM;
	}

	rc = sysfs_create_group(apq_hotplug_kobj, &apq_hotplug_attr_group);
	if (rc) {
		pr_err("Failed to create apq_hotplug sysfs entry!\n");
		kobject_put(apq_hotplug_kobj);
	}

	register_power_suspend(&apq_hotplug_power_suspend_handler);

	boot_flag = 1;

	pr_info("initialized!\n");

	return 0;
}

static void __exit apq_hotplug_exit(void)
{
	cancel_delayed_work_sync(&offline_all_work);
	cancel_work_sync(&online_all_work);

	sysfs_remove_group(apq_hotplug_kobj, &apq_hotplug_attr_group);
	kobject_put(apq_hotplug_kobj);

	unregister_power_suspend(&apq_hotplug_power_suspend_handler);
}

late_initcall(apq_hotplug_init);
module_exit(apq_hotplug_exit);
