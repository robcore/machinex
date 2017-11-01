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
struct kobj_attribute *attr,
const char *buf, size_t count)
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
