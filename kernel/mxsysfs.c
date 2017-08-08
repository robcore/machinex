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

#define MARK "mark"

#define MX_ATTR_RO(_name) \
static struct kobj_attribute _name##_attr = __ATTR_RO(_name)

#define MX_ATTR_RW(_name) \
static struct kobj_attribute _name##_attr = \
	__ATTR(_name, 0644, _name##_show, _name##_store)

static unsigned int mx_version = CONFIG_MACHINEX_VERSION;
/* whether file capabilities are enabled */
static ssize_t mx_version_show(struct kobject *kobj,
				  struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "MARK%d\n", mx_version);
}

MX_ATTR_RO(mx_version);

struct kobject *mx_kobj;
EXPORT_SYMBOL_GPL(mx_kobj);

static struct attribute * mx_attrs[] = {
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
