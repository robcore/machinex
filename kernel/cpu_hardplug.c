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

#define HARDPLUG_MAJOR 2
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
unsigned int limit_screen_off_cpus = 0;
unsigned int cpu1_allowed_susp = 1;
unsigned int cpu2_allowed_susp = 1;
unsigned int cpu3_allowed_susp = 1;

static DEFINE_MUTEX(hardplug_mtx);
static cpumask_var_t allowed_cpus;

#define cpu_allowed(cpu) cpumask_test_cpu((cpu), allowed_cpus)

unsigned int cpu1_allowed = 1;
unsigned int cpu2_allowed = 1;
unsigned int cpu3_allowed = 1;
			
bool is_cpu_allowed(unsigned int cpu)
{
	if (!is_display_on() || !limit_screen_on_cpus ||
		!hotplug_ready || cpumask_full(allowed_cpus))
		return true;

	if (!cpu_allowed(cpu))
		return false;

	return true;
}

void hardplug_cpus(void)
{
	unsigned int cpu;

	if (!is_display_on() || !limit_screen_on_cpus ||
		!hotplug_ready || cpumask_full(allowed_cpus))
		return;

	for_each_cpu_not(cpu, allowed_cpus) {
		if (cpu == 0)
			continue;
		if (!cpu_online(cpu))
			continue;
		cpu_down(cpu);
	}
}
EXPORT_SYMBOL(hardplug_cpus);

static void update_mask(void)
{
	unsigned int cpu;
	
	if (cpu1_allowed && !cpu_allowed(1))
		cpumask_test_and_set_cpu(1, allowed_cpus);
	if (cpu2_allowed && !cpu_allowed(2))
		cpumask_test_and_set_cpu(2, allowed_cpus);
	if (cpu3_allowed && !cpu_allowed(3))
		cpumask_test_and_set_cpu(3, allowed_cpus);

	if (!cpu1_allowed && cpu_allowed(1))
		cpumask_test_and_clear_cpu(1, allowed_cpus);
	if (!cpu2_allowed && cpu_allowed(2))
		cpumask_test_and_clear_cpu(2, allowed_cpus);
	if (!cpu3_allowed && cpu_allowed(3))
		cpumask_test_and_clear_cpu(3, allowed_cpus);

	hardplug_cpus();
}

static int cpu_hardplug_callback(struct notifier_block *nfb,
					    unsigned long action, void *hcpu)
{
	unsigned int cpu = (unsigned long)hcpu;
	/* Fail hotplug until this driver can get CPU clocks, drivers is disabled
	 * or screen off
	 */
	if (!hotplug_ready)
		return NOTIFY_OK;

	switch (action & ~CPU_TASKS_FROZEN) {
		/* Fall through. */
	case CPU_ONLINE:
	case CPU_DOWN_FAILED:
		if (!is_cpu_allowed(cpu));
			return NOTIFY_BAD;
		break;
	default:
		break;
	}

	return NOTIFY_OK;
}

static struct notifier_block cpu_hardplug_notifier = {
	.notifier_call = cpu_hardplug_callback,
};

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

	hardplug_cpus();

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

	update_mask();

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

	update_mask();

	return count;
}

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

	update_mask();

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

static struct attribute_group cpu_hardplug_attr_group =
{
	.attrs = cpu_hardplug_attrs,
	.name = "cpu_hardplug",
	NULL,
	
};

static int __init cpu_hardplug_init(void)
{
	int sysfs_result;

	if (allowed_cpus)
		cpumask_setall(allowed_cpus);
	else
		return -ENOMEM;

	sysfs_result = sysfs_create_group(kernel_kobj,
		&cpu_hardplug_attr_group);

	if (sysfs_result) {
		pr_info("ERROR! CPU Hardplug Sysfs group create failed!\n");
		return sysfs_result;
	}

	register_hotcpu_notifier(&cpu_hardplug_notifier);

	pr_info("CPU Hardplug Online\n");
	return 0;
}

static int __init alloc_hardplug_cpus(void)
{
	if (!alloc_cpumask_var(&allowed_cpus, GFP_KERNEL|__GFP_ZERO))
		return -ENOMEM;
	return cpu_hardplug_init();
}
core_initcall(alloc_hardplug_cpus);

MODULE_AUTHOR("Rob Patershuk <robpatershuk@gmail.com>");
MODULE_DESCRIPTION("Hard Limiting for CPU cores.");
MODULE_LICENSE("GPL v2");