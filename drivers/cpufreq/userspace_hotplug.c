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
static int uspace_cpu[NR_CPUS] = {1, 1, 1, 1};
static unsigned int uplug_enabled;

static ssize_t uplug_list_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return cpumap_print_to_pagebuf(true, buf, &uplug_mask);
}

static struct kobj_attribute uplug_list_attribute =
	__ATTR(uplug_list, 0444,
		uplug_list_show,
		NULL);

static ssize_t uplug_mask_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return cpumap_print_to_pagebuf(false, buf, &uplug_mask);
}

static struct kobj_attribute uplug_mask_attribute =
	__ATTR(uplug_mask, 0444,
		uplug_mask_show,
		NULL);

static ssize_t cpu0_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
		unsigned int temp;

		temp = cpumask_test_cpu(0, &uplug_mask);
        return sprintf(buf, "%u\n", temp);
}

static ssize_t cpu0_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned int val, cpu = 0;

	sscanf(buf, "%u\n", &val);

	sanitize_min_max(val, 0, 1);

	if (val == cpumask_test_cpu(cpu, &uplug_mask))
		return count;

	uspace_cpu[cpu] = val;

	if (uspace_cpu[cpu]) {
		if (!cpu_online(cpu))
			cpu_up(cpu);
		cpumask_set_cpu(cpu, &uplug_mask);
	} else if (!uspace_cpu[cpu]) {
		if (cpu_online(cpu))
			cpu_down(cpu);
		cpumask_clear_cpu(cpu, &uplug_mask);
	}

	return count;
}

static struct kobj_attribute cpu0_attribute =
	__ATTR(cpu0, 0644,
		cpu0_show,
		cpu0_store);

static ssize_t cpu1_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
		unsigned int temp;

		temp = cpumask_test_cpu(1, &uplug_mask);
        return sprintf(buf, "%u\n", temp);
}

static ssize_t cpu1_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned int val, cpu = 1;

	sscanf(buf, "%u\n", &val);

	sanitize_min_max(val, 0, 1);

	if (val == cpumask_test_cpu(cpu, &uplug_mask))
		return count;

	uspace_cpu[cpu] = val;

	if (uspace_cpu[cpu]) {
		if (!cpu_online(cpu))
			cpu_up(cpu);
		cpumask_set_cpu(cpu, &uplug_mask);
	} else if (!uspace_cpu[cpu]) {
		if (cpu_online(cpu))
			cpu_down(cpu);
		cpumask_clear_cpu(cpu, &uplug_mask);
	}

	return count;
}

static struct kobj_attribute cpu1_attribute =
	__ATTR(cpu1, 0644,
		cpu1_show,
		cpu1_store);

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

	uspace_cpu[cpu] = val;

	if (uspace_cpu[cpu]) {
		if (!cpu_online(cpu))
			cpu_up(cpu);
		cpumask_set_cpu(cpu, &uplug_mask);
	} else if (!uspace_cpu[cpu]) {
		if (cpu_online(cpu))
			cpu_down(cpu);
		cpumask_clear_cpu(cpu, &uplug_mask);
	}

	return count;
}

static struct kobj_attribute cpu2_attribute =
	__ATTR(cpu2, 0644,
		cpu2_show,
		cpu2_store);

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

	uspace_cpu[cpu] = val;

	if (uspace_cpu[cpu]) {
		if (!cpu_online(cpu))
			cpu_up(cpu);
		cpumask_set_cpu(cpu, &uplug_mask);
	} else if (!uspace_cpu[cpu]) {
		if (cpu_online(cpu))
			cpu_down(cpu);
		cpumask_clear_cpu(cpu, &uplug_mask);
	}

	return count;
}

static struct kobj_attribute cpu3_attribute =
	__ATTR(cpu3, 0644,
		cpu3_show,
		cpu3_store);

static struct attribute *cpumask_uplug_attrs[] =
{
	&uplug_list_attribute.attr,
	&uplug_mask_attribute.attr,
	&cpu0_attribute.attr,
	&cpu1_attribute.attr,
	&cpu2_attribute.attr,
	&cpu3_attribute.attr,
	NULL,
};

static const struct attribute_group cpumask_uplug_attr_group =
{
	.attrs = cpumask_uplug_attrs,
	.name = "cpumask_uplug",
};

static int __init cpumask_uplug_init(void)
{
	int sysfs_result;

	cpumask_copy(&uplug_mask,
			&__cpu_possible_mask);

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