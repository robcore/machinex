/* CPU Hardplug.
 * (C) 2017 Rob Patershuk "robcore"
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */
#include <linux/suspend.h>
#include <linux/smp.h>
#include <linux/init.h>
#include <linux/notifier.h>
#include <linux/stop_machine.h>
#include <linux/mutex.h>
#include <linux/tick.h>
#include <linux/irq.h>
#include <linux/smpboot.h>
#include <linux/cpu.h>
#include <linux/powersuspend.h>
#include <linux/sysfs_helpers.h>
#include <linux/display_state.h>

#define HARDPLUG_MAJOR 1
#define HARDPLUG_MINOR 5

unsigned int limit_screen_on_cpus = 0;
unsigned int cpu1_allowed = 1;
unsigned int cpu2_allowed = 1;
unsigned int cpu3_allowed = 1;

unsigned int limit_screen_off_cpus = 0;
unsigned int cpu1_allowed_susp = 1;
unsigned int cpu2_allowed_susp = 1;
unsigned int cpu3_allowed_susp = 1;

static bool hardplug_up = false;

static bool hardplug_alive = true;
static bool is_hardplug_alive(void)
{
	return hardplug_alive;
}

bool is_cpu_allowed(unsigned int cpu)
{
	if (!is_hardplug_alive() || !limit_screen_on_cpus)
		return true;

	switch (cpu) {
	case 0:
		break;
	case 1:
		if (!cpu1_allowed)
			return false;
		break;
	case 2:
		if (!cpu2_allowed)
			return false;
		break;
	case 3:
		if (!cpu3_allowed)
			return false;
		break;

	default:
		break;
	}

	return true;
}

static void hardplug_cpu(unsigned int cpu)
{
	if (!is_hardplug_alive() || !limit_screen_on_cpus)
		return;

	switch (cpu) {
	case 0:
		break;
	case 1:
		if (!cpu1_allowed && cpu_online(1))
			cpu_down(1);
		break;
	case 2:
		if (!cpu2_allowed && cpu_online(2))
			cpu_down(2);
		break;
	case 3:
		if (!cpu3_allowed && cpu_online(3))
			cpu_down(3);
		break;

	default:
		break;
	}
}

static ssize_t limit_screen_on_cpus_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
        return sprintf(buf, "%u\n", limit_screen_on_cpus);
}

static ssize_t limit_screen_on_cpus_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned int cpu;
	unsigned int val;

	sscanf(buf, "%u\n", &val);

	sanitize_min_max(val, 0, 1);

	if (limit_screen_on_cpus == val)
		return count;

	limit_screen_on_cpus = val;

	if (limit_screen_on_cpus) {
		for_each_online_cpu(cpu)
			hardplug_cpu(cpu);
	}

	return count;
}

static struct kobj_attribute limit_screen_on_cpus_attribute =
	__ATTR(limit_screen_on_cpus, 0644,
		limit_screen_on_cpus_show,
		limit_screen_on_cpus_store);


static ssize_t cpu1_allowed_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
        return sprintf(buf, "%u\n", cpu1_allowed);
}

static ssize_t cpu1_allowed_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned int val;

	sscanf(buf, "%u\n", &val);

	sanitize_min_max(val, 0, 1);

	if (cpu1_allowed == val)
		return count;

	cpu1_allowed = val;

	if (!cpu1_allowed && limit_screen_on_cpus)
		hardplug_cpu(1);

	return count;
}

static struct kobj_attribute cpu1_allowed_attribute =
	__ATTR(cpu1_allowed, 0644,
		cpu1_allowed_show,
		cpu1_allowed_store);

static ssize_t cpu2_allowed_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
        return sprintf(buf, "%u\n", cpu2_allowed);
}

static ssize_t cpu2_allowed_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned int val;

	sscanf(buf, "%u\n", &val);

	sanitize_min_max(val, 0, 1);

	if (cpu2_allowed == val)
		return count;

	cpu2_allowed = val;

	if (!cpu2_allowed && limit_screen_on_cpus)
		hardplug_cpu(2);

	return count;
}

static void hardplug_wake(struct power_suspend * h)
{
	unsigned int cpu;

	hardplug_alive = true;

	for_each_online_cpu(cpu)
		hardplug_cpu(cpu);
}

static void hardplug_sleep(struct power_suspend * h)
{
	hardplug_alive = false;
}

static struct power_suspend hardplug_suspend =
{
	.suspend = hardplug_wake,
	.resume = hardplug_sleep,
};

static struct kobj_attribute cpu2_allowed_attribute =
	__ATTR(cpu2_allowed, 0644,
		cpu2_allowed_show,
		cpu2_allowed_store);

static ssize_t cpu3_allowed_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
        return sprintf(buf, "%u\n", cpu3_allowed);
}

static ssize_t cpu3_allowed_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned int val;

	sscanf(buf, "%u\n", &val);

	sanitize_min_max(val, 0, 1);

	if (cpu3_allowed == val)
		return count;

	cpu3_allowed = val;

	if (!cpu3_allowed && limit_screen_on_cpus)
		hardplug_cpu(3);

	return count;
}

static struct kobj_attribute cpu3_allowed_attribute =
	__ATTR(cpu3_allowed, 0644,
		cpu3_allowed_show,
		cpu3_allowed_store);

static ssize_t limit_screen_off_cpus_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
        return sprintf(buf, "%u\n", limit_screen_off_cpus);
}

static ssize_t limit_screen_off_cpus_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned int val;

	sscanf(buf, "%u\n", &val);

	sanitize_min_max(val, 0, 1);

	if (limit_screen_off_cpus == val)
		return count;

	limit_screen_off_cpus = val;
	return count;
}

static struct kobj_attribute limit_screen_off_cpus_attribute =
	__ATTR(limit_screen_off_cpus, 0644,
		limit_screen_off_cpus_show,
		limit_screen_off_cpus_store);

static ssize_t cpu1_allowed_susp_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
        return sprintf(buf, "%u\n", cpu1_allowed_susp);
}

static ssize_t cpu1_allowed_susp_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned int val;

	sscanf(buf, "%u\n", &val);

	sanitize_min_max(val, 0, 1);

	if (cpu1_allowed_susp == val)
		return count;

	cpu1_allowed_susp = val;
	return count;
}

static struct kobj_attribute cpu1_allowed_susp_attribute =
	__ATTR(cpu1_allowed_susp, 0644,
		cpu1_allowed_susp_show,
		cpu1_allowed_susp_store);

static ssize_t cpu2_allowed_susp_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
        return sprintf(buf, "%u\n", cpu2_allowed_susp);
}

static ssize_t cpu2_allowed_susp_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned int val;

	sscanf(buf, "%u\n", &val);

	sanitize_min_max(val, 0, 1);

	if (cpu2_allowed_susp == val)
		return count;

	cpu2_allowed_susp = val;
	return count;
}

static struct kobj_attribute cpu2_allowed_susp_attribute =
	__ATTR(cpu2_allowed_susp, 0644,
		cpu2_allowed_susp_show,
		cpu2_allowed_susp_store);

static ssize_t cpu3_allowed_susp_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
        return sprintf(buf, "%u\n", cpu3_allowed_susp);
}

static ssize_t cpu3_allowed_susp_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned int val;

	sscanf(buf, "%u\n", &val);

	sanitize_min_max(val, 0, 1);

	if (cpu3_allowed_susp == val)
		return count;

	cpu3_allowed_susp = val;
	return count;
}

static struct kobj_attribute cpu3_allowed_susp_attribute =
	__ATTR(cpu3_allowed_susp, 0644,
		cpu3_allowed_susp_show,
		cpu3_allowed_susp_store);

static ssize_t cpu_hardplug_version_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "Hardplug Version: %u.%u\n", HARDPLUG_MAJOR, HARDPLUG_MINOR);
}

static struct kobj_attribute cpu_hardplug_version_attribute =
	__ATTR(cpu_hardplug_version, 0444,
		cpu_hardplug_version_show,
		NULL);

static struct attribute *cpu_hardplug_attrs[] =
{
	&cpu_hardplug_version_attribute.attr,
	&limit_screen_on_cpus_attribute.attr,
	&cpu1_allowed_attribute.attr,
	&cpu2_allowed_attribute.attr,
	&cpu3_allowed_attribute.attr,
	&limit_screen_off_cpus_attribute.attr,
	&cpu1_allowed_susp_attribute.attr,
	&cpu2_allowed_susp_attribute.attr,
	&cpu3_allowed_susp_attribute.attr,
	NULL,
};

static const struct attribute_group cpu_hardplug_attr_group =
{
	.attrs = cpu_hardplug_attrs,
};

static struct kobject *cpu_hardplug_kobj;

static int __init cpu_hardplug_init(void)
{
	int sysfs_result;

	cpu_hardplug_kobj = kobject_create_and_add("cpu_hardplug",
		kernel_kobj);

	if (!cpu_hardplug_kobj) {
		pr_err("%s kobject create failed!\n", __FUNCTION__);
		return -ENOMEM;
	}

	sysfs_result = sysfs_create_group(cpu_hardplug_kobj,
		&cpu_hardplug_attr_group);

	if (sysfs_result) {
		pr_info("%s group create failed!\n", __FUNCTION__);
		kobject_put(cpu_hardplug_kobj);
		return -ENOMEM;
	}

	hardplug_up = true;

	return 0;
}

/* This should never have to be used except on shutdown */
static void cpu_hardplug_exit(void)
{
	if (cpu_hardplug_kobj != NULL)
		kobject_put(cpu_hardplug_kobj);

	unregister_power_suspend(&hardplug_suspend);
}

core_initcall(cpu_hardplug_init);
module_exit(cpu_hardplug_exit);

static int __init cpu_hardplug_late_init(void)
{
	if (hardplug_up)
		register_power_suspend(&hardplug_suspend);
	else
		return -EINVAL;

	return 0;
}

device_initcall(cpu_hardplug_late_init);

MODULE_AUTHOR("Rob Patershuk <robpatershuk@gmail.com>");
MODULE_DESCRIPTION("Hard Limiting for CPU cores.");
MODULE_LICENSE("GPL v2");