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
				is_cpu_allowed(cpu))
				cpu_up(cpu);
			else if (!cpumask_test_cpu(cpu, &uplug_mask) &&
				cpu_online(cpu) &&
				!thermal_core_controlled(cpu) &&
				is_cpu_allowed(cpu))
				cpu_down(cpu);
		}
	} else {
		for_each_nonboot_offline_cpu(cpu) {
			if (cpu_out_of_range_hp(cpu))
				break;
			if (!cpu_online(cpu) &&
				!thermal_core_controlled(cpu) &&
				is_cpu_allowed(cpu))
				cpu_up(cpu);
		}
	}
}

mx_show_one(uplug_enabled);

static ssize_t store_uplug_enabled(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned int val;

	sscanf(buf, "%u\n", &val);

	sanitize_min_max(val, 0, 1);

	if (val == uplug_enabled)
		return count;

	uplug_enabled = val;

	uplug_start_stop(uplug_enabled);

	return count;
}

static ssize_t show_uplug_list(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return cpumap_print_to_pagebuf(true, buf, &uplug_mask);
}

static ssize_t show_uplug_mask(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return cpumap_print_to_pagebuf(false, buf, &uplug_mask);
}

static ssize_t show_cpu1(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
		unsigned int temp;

		temp = cpumask_test_cpu(1, &uplug_mask);
        return sprintf(buf, "%u\n", temp);
}

static ssize_t store_cpu1(struct kobject *kobj,
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
				is_cpu_allowed(cpu))
			cpu_up(cpu);
		if (cpu_online(cpu))
			cpumask_set_cpu(cpu, &uplug_mask);
	} else if (!val) {
		if (cpu_online(cpu) && uplug_enabled &&
				!thermal_core_controlled(cpu) &&
				is_cpu_allowed(cpu))
			cpu_down(cpu);
		if (!cpu_online(cpu))
			cpumask_clear_cpu(cpu, &uplug_mask);
	}

	return count;
}

static ssize_t show_cpu2(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
		unsigned int temp;

		temp = cpumask_test_cpu(2, &uplug_mask);
        return sprintf(buf, "%u\n", temp);
}

static ssize_t store_cpu2(struct kobject *kobj,
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
				is_cpu_allowed(cpu))
			cpu_up(cpu);
		if (cpu_online(cpu))
			cpumask_set_cpu(cpu, &uplug_mask);
	} else if (!val) {
		if (cpu_online(cpu) && uplug_enabled &&
				!thermal_core_controlled(cpu) &&
				is_cpu_allowed(cpu))
			cpu_down(cpu);
		if (!cpu_online(cpu))
			cpumask_clear_cpu(cpu, &uplug_mask);
	}

	return count;
}

static ssize_t show_cpu3(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
		unsigned int temp;

		temp = cpumask_test_cpu(3, &uplug_mask);
        return sprintf(buf, "%u\n", temp);
}

static ssize_t store_cpu3(struct kobject *kobj,
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
				is_cpu_allowed(cpu))
			cpu_up(cpu);
		if (cpu_online(cpu))
			cpumask_set_cpu(cpu, &uplug_mask);
	} else if (!val) {
		if (cpu_online(cpu) && uplug_enabled &&
				!thermal_core_controlled(cpu) &&
				is_cpu_allowed(cpu))
			cpu_down(cpu);
		if (!cpu_online(cpu))
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

static struct attribute *uplug_attrs[] =
{
	&uplug_enabled_attr.attr,
	&uplug_list_attr.attr,
	&uplug_mask_attr.attr,
	&cpu1_attr.attr,
	&cpu2_attr.attr,
	&cpu3_attr.attr,
	NULL,
};

static const struct attribute_group uplug_attr_group =
{
	.attrs = uplug_attrs,
	.name = "uplug",
};

static int __init cpumask_uplug_init(void)
{
	int sysfs_result;

	cpumask_copy(&uplug_mask,
		&__cpu_nonboot_mask);
	sysfs_result = sysfs_create_group(kernel_kobj,
		&uplug_attr_group);

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