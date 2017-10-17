/* CPU Hardplug.
 * (C) 2017 Rob Patershuk "robcore"
 *
 *  Note: This driver is USELESS without the appropriate HOOKS in cpu.c.
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
#include <linux/cpufreq.h>
#include <linux/powersuspend.h>
#include <linux/sysfs_helpers.h>
#include <linux/display_state.h>

#define HARDPLUG_MAJOR 3
#define HARDPLUG_MINOR 0

#if 0
#define DEFAULT_MAX_CPUS 4
static unsigned int cpu_num_limit = DEFAULT_MAX_CPUS;

static void unplug_one_cpu(void)
{
	int cpuid;

	if (num_online_cpus() < cpu_num_limit) {
		cpuid = cpumask_next_zero(0, cpu_online_mask);
		cpu_up(cpuid);
		}
	return;
}

static void plug_one_cpu(void)
{
	unsigned int cpuid = 0;

	if (num_online_cpus() > 1) {
			cpuid = cpumask_next(0, cpu_online_mask);
			cpu_down(cpuid);
	}
	return;
}
#endif
unsigned int limit_screen_on_cpus = 0;
static unsigned int cpu_allowed[NR_CPUS] = { 1, 1, 1, 1 };

unsigned int limit_screen_off_cpus = 0;
unsigned int cpu1_allowed_susp = 1;
unsigned int cpu2_allowed_susp = 1;
unsigned int cpu3_allowed_susp = 1;
static struct delayed_work hardplug_work;
static struct workqueue_struct *hpwq;

static DEFINE_MUTEX(hardplug_mtx);

bool is_cpu_allowed(unsigned int cpu)
{
	if (!limit_screen_on_cpus ||
		!is_display_on() ||
		!hotplug_ready || cpu == 0)
		return true;

	if (!cpu_allowed[cpu])
		return false;

	return true;
}

static void hardplug_cpu(unsigned int cpu)
{
	if (!cpu_allowed[cpu] && cpu_online(cpu))
		cpu_down(cpu);
}

static void unplug_cpu(unsigned int cpu)
{
	if (cpu_allowed[cpu] &&
		cpu_is_offline(cpu))
		cpu_up(cpu);
}

void hardplug_all_cpus(void)
{
	unsigned int cpu;

	for_each_nonboot_online_cpu(cpu) {
		if (cpu_out_of_range_hp(cpu))
			break;
		hardplug_cpu(cpu);
	}
}
EXPORT_SYMBOL(hardplug_all_cpus);

static void unplug_all_cpus(void)
{
	unsigned int cpu;

	for_each_nonboot_offline_cpu(cpu) {
		if (cpu_out_of_range_hp(cpu))
			break;
		unplug_cpu(cpu);
	}
}

unsigned int nr_hardplugged_cpus(void)
{
	unsigned int cpu;
	unsigned int hardplugged_cpus = 0;

	if (!is_display_on() || !limit_screen_on_cpus ||
		!hotplug_ready)
		return hardplugged_cpus;

	for_each_nonboot_cpu(cpu) {
		if (cpu_out_of_range_hp(cpu))
			break;
		if (!is_cpu_allowed(cpu))
			hardplugged_cpus += 1;
	}

	sanitize_min_max(hardplugged_cpus, 0, 3);

	return hardplugged_cpus;
}
EXPORT_SYMBOL(nr_hardplugged_cpus);

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

	if (val)
		hardplug_all_cpus();
	else
		unplug_all_cpus();

	return count;
}

static struct kobj_attribute limit_screen_on_cpus_attribute =
	__ATTR(limit_screen_on_cpus, 0644,
		limit_screen_on_cpus_show,
		limit_screen_on_cpus_store);


static ssize_t cpu1_allowed_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
        return sprintf(buf, "%u\n", cpu_allowed[1]);
}

static ssize_t cpu1_allowed_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned int val;

	sscanf(buf, "%u\n", &val);

	sanitize_min_max(val, 0, 1);

	if (cpu_allowed[1] == val)
		return count;

	cpu_allowed[1] = val;

	if (val)
		hardplug_cpu(1);
	else
		unplug_cpu(1);

	return count;
}

static struct kobj_attribute cpu1_allowed_attribute =
	__ATTR(cpu1_allowed, 0644,
		cpu1_allowed_show,
		cpu1_allowed_store);

static ssize_t cpu2_allowed_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
        return sprintf(buf, "%u\n", cpu_allowed[2]);
}

static ssize_t cpu2_allowed_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned int val;

	sscanf(buf, "%u\n", &val);

	sanitize_min_max(val, 0, 1);

	if (cpu_allowed[2] == val)
		return count;

	cpu_allowed[2] = val;

	if (val)
		hardplug_cpu(2);
	else
		unplug_cpu(2);

	return count;
}

static struct kobj_attribute cpu2_allowed_attribute =
	__ATTR(cpu2_allowed, 0644,
		cpu2_allowed_show,
		cpu2_allowed_store);

static ssize_t cpu3_allowed_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
        return sprintf(buf, "%u\n", cpu_allowed[3]);
}

static ssize_t cpu3_allowed_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned int val;

	sscanf(buf, "%u\n", &val);

	sanitize_min_max(val, 0, 1);

	if (cpu_allowed[3] == val)
		return count;

	cpu_allowed[3] = val;

	if (val)
		hardplug_cpu(3);
	else
		unplug_cpu(3);

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
	.name = "cpu_hardplug",
};

static struct kobject *cpu_hardplug_kobj;

static int __init cpu_hardplug_init(void)
{
	int sysfs_result;

	sysfs_result = sysfs_create_group(kernel_kobj,
		&cpu_hardplug_attr_group);

	if (sysfs_result) {
		pr_info("%s group create failed!\n", __FUNCTION__);
		return -ENOMEM;
	}

	return 0;
}

postcore_initcall(cpu_hardplug_init);

MODULE_AUTHOR("Rob Patershuk <robpatershuk@gmail.com>");
MODULE_DESCRIPTION("Hard Limiting for CPU cores.");
MODULE_LICENSE("GPL v2");
