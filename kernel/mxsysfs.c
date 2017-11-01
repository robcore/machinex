/*
 * mx/ksysfs.c - sysfs attributes in /sys/machinex
 *
 * Copyright (C) 2004 Kay Sievers <kay.sievers@vrfy.org>
 * Copyright (C) 2017 Rob Patershuk "robcore"
 *
 * This file is release under the GPLv2
 *
 */

#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/export.h>
#include <linux/init.h>
#include <linux/capability.h>
#include <linux/compiler.h>

unsigned int userspace_ready = 0;
mx_show_one(userspace_ready);
static ssize_t store_userspace_ready(struct kobject *kobj,
	struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	sanitize_min_max(input, 1, 1);

	if (input == userspace_ready)
		return count;

	userspace_ready = input;
	pr_info("[MACHINEX] USER SPACE READY\n");
	return count;
}
MX_ATTR_RW(userspace_ready);

unsigned int init_tracker = 0;
static void print_init_level(unsigned int level)
{
	if (level > 13)
		return;

	switch (level) {
	case 1:
		pr_info("[INIT TRACKER]: early-init\n");
		init_tracker++;
		break;
	case 2:
		pr_info("[INIT TRACKER]: init\n");
		init_tracker++;
		break;
	case 3:
		pr_info("[INIT TRACKER]: late-init\n");
		init_tracker++;
		break;
	case 4:
		pr_info("[INIT TRACKER]: early-fs\n");
		init_tracker++;
		break;
	case 5:
		pr_info("[INIT TRACKER]: fs\n");
		init_tracker++;
		break;
	case 6:
		pr_info("[INIT TRACKER]: post-fs\n");
		init_tracker++;
		break;
	case 7:
		pr_info("[INIT TRACKER]: post-fs-data\n");
		init_tracker++;
		break;
	case 8:
		pr_info("[INIT TRACKER]: firmware_mounts_complete\n");
		init_tracker++;
		break;
	case 9:
		pr_info("[INIT TRACKER]: early-boot\n");
		init_tracker++;
		break;
	case 10:
		pr_info("[INIT TRACKER]: boot\n");
		init_tracker++;
		break;
	case 11:
		pr_info("[INIT TRACKER]: class_start core\n");
		init_tracker++;
		break;
	case 12:
		pr_info("[INIT TRACKER]: class_start main\n");
		init_tracker++;
		break;
	case 13:
		pr_info("[INIT TRACKER]: class_start late_start\n");
		init_tracker++;
		break;
	default:
		break;
	}
}

static ssize_t store_init_tracker(struct kobject *kobj,
	struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	sanitize_min_max(input, 0, 99);

	print_init_level(input);
	return count;
}
MX_ATTR_WO(init_tracker);

static unsigned char *mx_version = CONFIG_MACHINEX_VERSION;
/* whether file capabilities are enabled */
static ssize_t show_mx_version(struct kobject *kobj,
				  struct kobj_attribute *attr, char *buf)
{
	ssize_t ret = 0;

	ret = sprintf(buf, "Mark%s\n", mx_version);
	return ret;
}
MX_ATTR_RO(mx_version);

struct kobject *mx_kobj;
EXPORT_SYMBOL_GPL(mx_kobj);

static struct attribute * mx_attrs[] = {
	&userspace_ready_attr.attr,
	&init_tracker_attr.attr,
	&mx_version_attr.attr,
	NULL
};

static const struct attribute_group mx_attr_group = {
	.attrs = mx_attrs,
};

static int __init mxsysfs_init(void)
{
	int error;

	mx_kobj = kobject_create_and_add("machinex", NULL);
	if (!mx_kobj) {
		error = -ENOMEM;
		goto exit;
	}
	error = sysfs_create_group(mx_kobj, &mx_attr_group);
	if (error)
		goto kset_exit;

	return 0;

kset_exit:
	kobject_put(mx_kobj);
exit:
	return error;
}

core_initcall(mxsysfs_init);
