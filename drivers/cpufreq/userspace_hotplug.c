#include <linux/module.h>
#include <linux/smp.h>
#include <linux/init.h>
#include <linux/notifier.h>
#include <linux/stop_machine.h>
#include <linux/kernel.h>
#include <linux/smpboot.h>
#include <linux/cpu.h>
#include <linux/cpufreq.h>
#include <linux/sysfs_helpers.h>

static cpumask_t uplug_mask;
static unsigned int uplug_enabled;

mx_show_one(uplug_enabled);
store_one_clamp(uplug_enabled);

static ssize_t uplug_list_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return cpumap_print_to_pagebuf(true, buf, &uplug_mask);
}

static ssize_t uplug_mask_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return cpumap_print_to_pagebuf(false, buf, &uplug_mask);
}

static ssize_t cpu1_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned int val, cpu = 1;

	sscanf(buf, "%u\n", &val);

	sanitize_min_max(val, 0, 1);

	if (val == cpumask_test_cpu(cpu, &uplug_mask))
		return count;

	if (val) {
		if (!cpu_online(cpu) && uplug_enabled &&
				!thermal_core_controlled(cpu) &&
				cpu_allowed(cpu))
			cpu_up(cpu);
		cpumask_set_cpu(cpu, &uplug_mask);
	} else if (!val) {
		if (cpu_online(cpu) && uplug_enabled &&
				!thermal_core_controlled(cpu) &&
				cpu_allowed(cpu))
			cpu_down(cpu);
		cpumask_clear_cpu(cpu, &uplug_mask);
	}

	return count;
}

static ssize_t cpu2_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
		unsigned int temp;

		temp = cpumask_test_cpu(2, &uplug_mask);
        return sprintf(buf, "%u\n", temp);
}

static ssize_t cpu2_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned int val, cpu = 2;

	sscanf(buf, "%u\n", &val);

	sanitize_min_max(val, 0, 1);

	if (val == cpumask_test_cpu(cpu, &uplug_mask))
		return count;

	if (val) {
		if (!cpu_online(cpu) && uplug_enabled &&
				!thermal_core_controlled(cpu) &&
				cpu_allowed(cpu))
			cpu_up(cpu);
		cpumask_set_cpu(cpu, &uplug_mask);
	} else if (!val) {
		if (cpu_online(cpu) && uplug_enabled &&
				!thermal_core_controlled(cpu) &&
				cpu_allowed(cpu))
			cpu_down(cpu);
		cpumask_clear_cpu(cpu, &uplug_mask);
	}

	return count;
}

static ssize_t cpu3_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
		unsigned int temp;

		temp = cpumask_test_cpu(3, &uplug_mask);
        return sprintf(buf, "%u\n", temp);
}

static ssize_t cpu3_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned int val, cpu = 3;

	sscanf(buf, "%u\n", &val);

	sanitize_min_max(val, 0, 1);

	if (val == cpumask_test_cpu(cpu, &uplug_mask))
		return count;

	if (val) {
		if (!cpu_online(cpu) && uplug_enabled &&
				!thermal_core_controlled(cpu) &&
				cpu_allowed(cpu))
			cpu_up(cpu);
		cpumask_set_cpu(cpu, &uplug_mask);
	} else if (!val) {
		if (cpu_online(cpu) && uplug_enabled &&
				!thermal_core_controlled(cpu) &&
				cpu_allowed(cpu))
			cpu_down(cpu);
		cpumask_clear_cpu(cpu, &uplug_mask);
	}

	return count;
}

MX_ATTR_RW(uplug_enabled);
MX_ATTR_RO(uplug_list);
MX_ATTR_RO(uplug_mask);
MX_ATTR_RW(cpu1);
MX_ATTR_RW(cpu2);
MX_ATTR_RW(cpu3);

static struct attribute *cpumask_uplug_attrs[] =
{
	&uplug_enabled_attr.attr,
	&uplug_list_attr.attr,
	&uplug_mask_attr.attr,
	&cpu1_attr.attr,
	&cpu2_attr.attr,
	&cpu3_attr.attr,
	NULL,
};

static const struct attribute_group cpumask_uplug_attr_group =
{
	.attrs = cpumask_uplug_attrs,
	.name = "cpumask_uplug",
};

static void uplug_start_stop(unsigned int enabled)
{
	unsigned int cpu;
	if (enabled) {
		for_each_nonboot_cpu(cpu) {
			if (cpu_out_of_range_hp(cpu))
				break;
			if (cpumask_test_cpu(cpu, &uplug_mask) &&
				!cpu_online(cpu) &&
				!thermal_core_controlled(cpu) &&
				cpu_allowed(cpu))
				cpu_up(cpu);
			else if (!cpumask_test_cpu(cpu, &uplug_mask) &&
				cpu_online(cpu) &&
				!thermal_core_controlled(cpu) &&
				cpu_allowed(cpu))
				cpu_down(cpu);
		}
	} else {
		for_each_nonboot_offline_cpu(cpu) {
			if (cpu_out_of_range_hp(cpu))
				break;
			if (!cpu_online(cpu) &&
				!thermal_core_controlled(cpu) &&
				cpu_allowed(cpu))
				cpu_up(cpu)
		}
	}
}
static int __init cpumask_uplug_init(void)
{
	int sysfs_result;

	cpumask_copy(&uplug_mask,
		&__cpu_nonboot_mask);
	sysfs_result = sysfs_create_group(mx_kobj,
		&cpumask_uplug_attr_group);

	if (sysfs_result) {
		pr_info("%s group create failed!\n", __FUNCTION__);
		return -ENOMEM;
	}

	return 0;
}
late_initcall(cpumask_uplug_init);

MODULE_AUTHOR("Rob Patershuk <robpatershuk@gmail.com>");
MODULE_DESCRIPTION("Simple Userspace Hotplug Driver.");
MODULE_LICENSE("GPL v2");