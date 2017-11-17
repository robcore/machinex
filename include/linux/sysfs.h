/*
 * sysfs.h - definitions for the device driver filesystem
 *
 * Copyright (c) 2001,2002 Patrick Mochel
 * Copyright (c) 2004 Silicon Graphics, Inc.
 * Copyright (c) 2007 SUSE Linux Products GmbH
 * Copyright (c) 2007 Tejun Heo <teheo@suse.de>
 *
 * Please see Documentation/filesystems/sysfs.txt for more information.
 */

#ifndef _SYSFS_H_
#define _SYSFS_H_

#include <linux/kernfs.h>
#include <linux/compiler.h>
#include <linux/errno.h>
#include <linux/list.h>
#include <linux/lockdep.h>
#include <linux/kobject_ns.h>
#include <linux/stat.h>
#include <linux/atomic.h>

struct kobject;
struct module;
struct bin_attribute;
enum kobj_ns_type;

struct attribute {
	const char		*name;
	umode_t			mode;
#ifdef CONFIG_DEBUG_LOCK_ALLOC
	bool			ignore_lockdep:1;
	struct lock_class_key	*key;
	struct lock_class_key	skey;
#endif
};

/**
 *	sysfs_attr_init - initialize a dynamically allocated sysfs attribute
 *	@attr: struct attribute to initialize
 *
 *	Initialize a dynamically allocated struct attribute so we can
 *	make lockdep happy.  This is a new requirement for attributes
 *	and initially this is only needed when lockdep is enabled.
 *	Lockdep gives a nice error when your attribute is added to
 *	sysfs if you don't have this.
 */
#ifdef CONFIG_DEBUG_LOCK_ALLOC
#define sysfs_attr_init(attr)				\
do {							\
	static struct lock_class_key __key;		\
							\
	(attr)->key = &__key;				\
} while (0)
#else
#define sysfs_attr_init(attr) do {} while (0)
#endif

/**
 * struct attribute_group - data structure used to declare an attribute group.
 * @name:	Optional: Attribute group name
 *		If specified, the attribute group will be created in
 *		a new subdirectory with this name.
 * @is_visible:	Optional: Function to return permissions associated with an
 *		attribute of the group. Will be called repeatedly for each
 *		non-binary attribute in the group. Only read/write
 *		permissions as well as SYSFS_PREALLOC are accepted. Must
 *		return 0 if an attribute is not visible. The returned value
 *		will replace static permissions defined in struct attribute.
 * @is_bin_visible:
 *		Optional: Function to return permissions associated with a
 *		binary attribute of the group. Will be called repeatedly
 *		for each binary attribute in the group. Only read/write
 *		permissions as well as SYSFS_PREALLOC are accepted. Must
 *		return 0 if a binary attribute is not visible. The returned
 *		value will replace static permissions defined in
 *		struct bin_attribute.
 * @attrs:	Pointer to NULL terminated list of attributes.
 * @bin_attrs:	Pointer to NULL terminated list of binary attributes.
 *		Either attrs or bin_attrs or both must be provided.
 */
struct attribute_group {
	const char		*name;
	umode_t			(*is_visible)(struct kobject *,
					      struct attribute *, int);
	umode_t			(*is_bin_visible)(struct kobject *,
						  struct bin_attribute *, int);
	struct attribute	**attrs;
	struct bin_attribute	**bin_attrs;
};

/**
 * Use these macros to make defining attributes easier. See include/linux/device.h
 * for examples..
 */

#define SYSFS_PREALLOC 010000

#define __ATTR(_name, _mode, _show, _store) {				\
	.attr = {.name = __stringify(_name),				\
		 .mode = VERIFY_OCTAL_PERMISSIONS(_mode) },		\
	.show	= _show,						\
	.store	= _store,						\
}

#define __ATTR_PREALLOC(_name, _mode, _show, _store) {			\
	.attr = {.name = __stringify(_name),				\
		 .mode = SYSFS_PREALLOC | VERIFY_OCTAL_PERMISSIONS(_mode) },\
	.show	= _show,						\
	.store	= _store,						\
}

#define __ATTR_RO(_name) {						\
	.attr	= { .name = __stringify(_name), .mode = S_IRUGO },	\
	.show	= _name##_show,						\
}

#define __ATTR_WO(_name) {						\
	.attr	= { .name = __stringify(_name), .mode = S_IWUSR },	\
	.store	= _name##_store,					\
}

#define __ATTR_RW(_name) __ATTR(_name, (S_IWUSR | S_IRUGO),		\
			 _name##_show, _name##_store)

#define __ATTR_NULL { .attr = { .name = NULL } }

#ifdef CONFIG_DEBUG_LOCK_ALLOC
#define __ATTR_IGNORE_LOCKDEP(_name, _mode, _show, _store) {	\
	.attr = {.name = __stringify(_name), .mode = _mode,	\
			.ignore_lockdep = true },		\
	.show		= _show,				\
	.store		= _store,				\
}
#else
#define __ATTR_IGNORE_LOCKDEP	__ATTR
#endif

#define __ATTRIBUTE_GROUPS(_name)				\
static const struct attribute_group *_name##_groups[] = {	\
	&_name##_group,						\
	NULL,							\
}

#define ATTRIBUTE_GROUPS(_name)					\
static const struct attribute_group _name##_group = {		\
	.attrs = _name##_attrs,					\
};								\
__ATTRIBUTE_GROUPS(_name)

#define mx_show_one(object)				\
static ssize_t show_##object					\
(struct kobject *kobj, struct kobj_attribute *attr, char *buf)	\
{								\
	return sprintf(buf, "%u\n", object);			\
}

#define mx_show_long(object)				\
static ssize_t show_##object					\
(struct kobject *kobj, struct kobj_attribute *attr, char *buf)	\
{								\
	return sprintf(buf, "%lu\n", object);			\
}

#define store_one_clamp(name, min, max)		\
static ssize_t store_##name		\
(struct kobject *kobj,				\
 struct kobj_attribute *attr,			\
 const char *buf, size_t count)			\
{						\
	unsigned int input;			\
	int ret;				\
	ret = sscanf(buf, "%u", &input);	\
	if (ret != 1)			\
		return -EINVAL;			\
	if (input == name)			\
		return count;			\
	if (input <= min)	\
		input = min;	\
	if (input >= max)		\
			input = max;		\
	name = input;				\
	return count;				\
}

#define mx_store_one_long(object, min, max)		\
static ssize_t store_##object		\
(struct kobject *kobj,				\
 struct kobj_attribute *attr,			\
 const char *buf, size_t count)			\
{						\
	unsigned long input;			\
	int ret;				\
	ret = sscanf(buf, "%lu", &input);	\
	if (ret != 1)			\
		return -EINVAL;			\
	if (input <= min)	\
		input = min;	\
	if (input >= max)		\
			input = max;		\
	if (input == object) {			\
		return count;			\
	}					\
	object = input;				\
	return count;				\
}

#define store_one_ktimer(object, min, max)		\
static ssize_t store_##object		\
(struct kobject *kobj,				\
 struct kobj_attribute *attr,			\
 const char *buf, size_t count)			\
{						\
	unsigned long input;			\
	int ret;				\
	ret = sscanf(buf, "%lu", &input);	\
	if (ret != 1)			\
		return -EINVAL;			\
	if (input <= min)	\
		input = min;	\
	if (input >= max)		\
			input = max;		\
	if (input == object) {			\
		return count;			\
	}					\
	object = INTELLI_MS(input);				\
	return count;				\
}


#define show_one_cpu0(object)				\
static ssize_t show_cpu0_##object					\
(struct kobject *kobj, struct kobj_attribute *attr, char *buf)	\
{								\
	return sprintf(buf, "%u\n", object[0]);			\
}

#define show_one_cpu1(object)				\
static ssize_t show_cpu1_##object					\
(struct kobject *kobj, struct kobj_attribute *attr, char *buf)	\
{								\
	return sprintf(buf, "%u\n", object[1]);			\
}

#define show_one_cpu2(object)				\
static ssize_t show_cpu2_##object					\
(struct kobject *kobj, struct kobj_attribute *attr, char *buf)	\
{								\
	return sprintf(buf, "%u\n", object[2]);			\
}

#define show_one_cpu3(object)				\
static ssize_t show_cpu3_##object					\
(struct kobject *kobj, struct kobj_attribute *attr, char *buf)	\
{								\
	return sprintf(buf, "%u\n", object[3]);			\
}

#define store_one_cpu0_clamp(name, min, max)		\
static ssize_t store_cpu0_##name		\
(struct kobject *kobj,				\
 struct kobj_attribute *attr,			\
 const char *buf, size_t count)			\
{						\
	unsigned int input;			\
	int ret;				\
	ret = sscanf(buf, "%u", &input);	\
	if (ret != 1)			\
		return -EINVAL;			\
	if (input == name[0])			\
		return count;			\
	if (input <= min)	\
		input = min;	\
	if (input >= max)		\
			input = max;		\
	name[0] = input;				\
	return count;				\
}

#define store_one_cpu1_clamp(name, min, max)		\
static ssize_t store_cpu1_##name		\
(struct kobject *kobj,				\
 struct kobj_attribute *attr,			\
 const char *buf, size_t count)			\
{						\
	unsigned int input;			\
	int ret;				\
	ret = sscanf(buf, "%u", &input);	\
	if (ret != 1)			\
		return -EINVAL;			\
	if (input == name[1])			\
		return count;			\
	if (input <= min)	\
		input = min;	\
	if (input >= max)		\
			input = max;		\
	name[1] = input;				\
	return count;				\
}

#define store_one_cpu2_clamp(name, min, max)		\
static ssize_t store_cpu2_##name		\
(struct kobject *kobj,				\
 struct kobj_attribute *attr,			\
 const char *buf, size_t count)			\
{						\
	unsigned int input;			\
	int ret;				\
	ret = sscanf(buf, "%u", &input);	\
	if (ret != 1)			\
		return -EINVAL;			\
	if (input == name[2])			\
		return count;			\
	if (input <= min)	\
		input = min;	\
	if (input >= max)		\
			input = max;		\
	name[2] = input;				\
	return count;				\
}

#define store_one_cpu3_clamp(name, min, max)		\
static ssize_t store_cpu3_##name		\
(struct kobject *kobj,				\
 struct kobj_attribute *attr,			\
 const char *buf, size_t count)			\
{						\
	unsigned int input;			\
	int ret;				\
	ret = sscanf(buf, "%u", &input);	\
	if (ret != 1)			\
		return -EINVAL;			\
	if (input == name[3])			\
		return count;			\
	if (input <= min)	\
		input = min;	\
	if (input >= max)		\
			input = max;		\
	name[3] = input;				\
	return count;				\
}

#define show_one_long_cpu0(object)				\
static ssize_t show_cpu0_##object					\
(struct kobject *kobj, struct kobj_attribute *attr, char *buf)	\
{								\
	return sprintf(buf, "%lu\n", object[0]);			\
}

#define show_one_long_cpu1(object)				\
static ssize_t show_cpu1_##object					\
(struct kobject *kobj, struct kobj_attribute *attr, char *buf)	\
{								\
	return sprintf(buf, "%lu\n", object[1]);			\
}

#define show_one_long_cpu2(object)				\
static ssize_t show_cpu2_##object					\
(struct kobject *kobj, struct kobj_attribute *attr, char *buf)	\
{								\
	return sprintf(buf, "%lu\n", object[2]);			\
}

#define show_one_long_cpu3(object)				\
static ssize_t show_cpu3_##object					\
(struct kobject *kobj, struct kobj_attribute *attr, char *buf)	\
{								\
	return sprintf(buf, "%lu\n", object[3]);			\
}

#define store_one_long_cpu0_clamp(name, min, max)		\
static ssize_t store_cpu0_##name		\
(struct kobject *kobj,				\
 struct kobj_attribute *attr,			\
 const char *buf, size_t count)			\
{						\
	unsigned long input;			\
	int ret;				\
	ret = sscanf(buf, "%lu", &input);	\
	if (ret != 1)			\
		return -EINVAL;			\
	if (input == name[0])			\
		return count;			\
	if (input <= min)	\
		input = min;	\
	if (input >= max)		\
			input = max;		\
	name[0] = input;				\
	return count;				\
}

#define store_one_long_cpu1_clamp(name, min, max)		\
static ssize_t store_cpu1_##name		\
(struct kobject *kobj,				\
 struct kobj_attribute *attr,			\
 const char *buf, size_t count)			\
{						\
	unsigned long input;			\
	int ret;				\
	ret = sscanf(buf, "%lu", &input);	\
	if (ret != 1)			\
		return -EINVAL;			\
	if (input == name[1])			\
		return count;			\
	if (input <= min)	\
		input = min;	\
	if (input >= max)		\
			input = max;		\
	name[1] = input;				\
	return count;				\
}

#define store_one_long_cpu2_clamp(name, min, max)		\
static ssize_t store_cpu2_##name		\
(struct kobject *kobj,				\
 struct kobj_attribute *attr,			\
 const char *buf, size_t count)			\
{						\
	unsigned long input;			\
	int ret;				\
	ret = sscanf(buf, "%lu", &input);	\
	if (ret != 1)			\
		return -EINVAL;			\
	if (input == name[2])			\
		return count;			\
	if (input <= min)	\
		input = min;	\
	if (input >= max)		\
			input = max;		\
	name[2] = input;				\
	return count;				\
}

#define store_one_long_cpu3_clamp(name, min, max)		\
static ssize_t store_cpu3_##name		\
(struct kobject *kobj,				\
 struct kobj_attribute *attr,			\
 const char *buf, size_t count)			\
{						\
	unsigned long input;			\
	int ret;				\
	ret = sscanf(buf, "%lu", &input);	\
	if (ret != 1)			\
		return -EINVAL;			\
	if (input == name[3])			\
		return count;			\
	if (input <= min)	\
		input = min;	\
	if (input >= max)		\
			input = max;		\
	name[3] = input;				\
	return count;				\
}

#define __MX_ATTR_RO(_name) {						\
	.attr	= { .name = __stringify(_name), .mode = S_IRUGO },	\
	.show	= show_##_name,						\
}

#define MX_ATTR_RO(_name) \
static struct kobj_attribute _name##_attr = __MX_ATTR_RO(_name)

#define MX_ATTR_RW(_name) \
static struct kobj_attribute _name##_attr = \
	__ATTR(_name, 0644, show_##_name, store_##_name)

#define MX_ATTR_WO(_name) \
static struct kobj_attribute _name##_attr = \
	__ATTR(_name, 0222, NULL, store_##_name)

#define MX_CPU0_ATTR_RW(_name) \
static struct kobj_attribute cpu0_##_name##_attr = \
	__ATTR(cpu0_##_name, 0644, show_cpu0_##_name, store_cpu0_##_name)

#define MX_CPU1_ATTR_RW(_name) \
static struct kobj_attribute cpu1_##_name##_attr = \
	__ATTR(cpu1_##_name, 0644, show_cpu1_##_name, store_cpu1_##_name)

#define MX_CPU2_ATTR_RW(_name) \
static struct kobj_attribute cpu2_##_name##_attr = \
	__ATTR(cpu2_##_name, 0644, show_cpu2_##_name, store_cpu2_##_name)

#define MX_CPU3_ATTR_RW(_name) \
static struct kobj_attribute cpu3_##_name##_attr = \
	__ATTR(cpu3_##_name, 0644, show_cpu3_##_name, store_cpu3_##_name)

struct file;
struct vm_area_struct;

struct bin_attribute {
	struct attribute	attr;
	size_t			size;
	void			*private;
	ssize_t (*read)(struct file *, struct kobject *, struct bin_attribute *,
			char *, loff_t, size_t);
	ssize_t (*write)(struct file *, struct kobject *, struct bin_attribute *,
			 char *, loff_t, size_t);
	int (*mmap)(struct file *, struct kobject *, struct bin_attribute *attr,
		    struct vm_area_struct *vma);
};

/**
 *	sysfs_bin_attr_init - initialize a dynamically allocated bin_attribute
 *	@attr: struct bin_attribute to initialize
 *
 *	Initialize a dynamically allocated struct bin_attribute so we
 *	can make lockdep happy.  This is a new requirement for
 *	attributes and initially this is only needed when lockdep is
 *	enabled.  Lockdep gives a nice error when your attribute is
 *	added to sysfs if you don't have this.
 */
#define sysfs_bin_attr_init(bin_attr) sysfs_attr_init(&(bin_attr)->attr)

/* macros to create static binary attributes easier */
#define __BIN_ATTR(_name, _mode, _read, _write, _size) {		\
	.attr = { .name = __stringify(_name), .mode = _mode },		\
	.read	= _read,						\
	.write	= _write,						\
	.size	= _size,						\
}

#define __BIN_ATTR_RO(_name, _size) {					\
	.attr	= { .name = __stringify(_name), .mode = S_IRUGO },	\
	.read	= _name##_read,						\
	.size	= _size,						\
}

#define __BIN_ATTR_RW(_name, _size) __BIN_ATTR(_name,			\
				   (S_IWUSR | S_IRUGO), _name##_read,	\
				   _name##_write, _size)

#define __BIN_ATTR_NULL __ATTR_NULL

#define BIN_ATTR(_name, _mode, _read, _write, _size)			\
struct bin_attribute bin_attr_##_name = __BIN_ATTR(_name, _mode, _read,	\
					_write, _size)

#define BIN_ATTR_RO(_name, _size)					\
struct bin_attribute bin_attr_##_name = __BIN_ATTR_RO(_name, _size)

#define BIN_ATTR_RW(_name, _size)					\
struct bin_attribute bin_attr_##_name = __BIN_ATTR_RW(_name, _size)

struct sysfs_ops {
	ssize_t	(*show)(struct kobject *, struct attribute *, char *);
	ssize_t	(*store)(struct kobject *, struct attribute *, const char *, size_t);
};

#ifdef CONFIG_SYSFS

int __must_check sysfs_create_dir_ns(struct kobject *kobj, const void *ns);
void sysfs_remove_dir(struct kobject *kobj);
int __must_check sysfs_rename_dir_ns(struct kobject *kobj, const char *new_name,
				     const void *new_ns);
int __must_check sysfs_move_dir_ns(struct kobject *kobj,
				   struct kobject *new_parent_kobj,
				   const void *new_ns);
int __must_check sysfs_create_mount_point(struct kobject *parent_kobj,
					  const char *name);
void sysfs_remove_mount_point(struct kobject *parent_kobj,
			      const char *name);

int __must_check sysfs_create_file_ns(struct kobject *kobj,
				      const struct attribute *attr,
				      const void *ns);
int __must_check sysfs_create_files(struct kobject *kobj,
				   const struct attribute **attr);
int __must_check sysfs_chmod_file(struct kobject *kobj,
				  const struct attribute *attr, umode_t mode);
int __must_check sysfs_chown_file(struct kobject *kobj,
				  const struct attribute *attr, uid_t uid, gid_t gid);
void sysfs_remove_file_ns(struct kobject *kobj, const struct attribute *attr,
			  const void *ns);
bool sysfs_remove_file_self(struct kobject *kobj, const struct attribute *attr);
void sysfs_remove_files(struct kobject *kobj, const struct attribute **attr);

int __must_check sysfs_create_bin_file(struct kobject *kobj,
				       const struct bin_attribute *attr);
void sysfs_remove_bin_file(struct kobject *kobj,
			   const struct bin_attribute *attr);

int __must_check sysfs_create_link(struct kobject *kobj, struct kobject *target,
				   const char *name);
int __must_check sysfs_create_link_nowarn(struct kobject *kobj,
					  struct kobject *target,
					  const char *name);
void sysfs_remove_link(struct kobject *kobj, const char *name);

int sysfs_rename_link_ns(struct kobject *kobj, struct kobject *target,
			 const char *old_name, const char *new_name,
			 const void *new_ns);

void sysfs_delete_link(struct kobject *dir, struct kobject *targ,
			const char *name);

int __must_check sysfs_create_group(struct kobject *kobj,
				    const struct attribute_group *grp);
int __must_check sysfs_create_groups(struct kobject *kobj,
				     const struct attribute_group **groups);
int sysfs_update_group(struct kobject *kobj,
		       const struct attribute_group *grp);
void sysfs_remove_group(struct kobject *kobj,
			const struct attribute_group *grp);
void sysfs_remove_groups(struct kobject *kobj,
			 const struct attribute_group **groups);
int sysfs_add_file_to_group(struct kobject *kobj,
			const struct attribute *attr, const char *group);
void sysfs_remove_file_from_group(struct kobject *kobj,
			const struct attribute *attr, const char *group);
int sysfs_merge_group(struct kobject *kobj,
		       const struct attribute_group *grp);
void sysfs_unmerge_group(struct kobject *kobj,
		       const struct attribute_group *grp);
int sysfs_add_link_to_group(struct kobject *kobj, const char *group_name,
			    struct kobject *target, const char *link_name);
void sysfs_remove_link_from_group(struct kobject *kobj, const char *group_name,
				  const char *link_name);
int __compat_only_sysfs_link_entry_to_kobj(struct kobject *kobj,
				      struct kobject *target_kobj,
				      const char *target_name);

void sysfs_notify(struct kobject *kobj, const char *dir, const char *attr);

int __must_check sysfs_init(void);

static inline void sysfs_enable_ns(struct kernfs_node *kn)
{
	return kernfs_enable_ns(kn);
}

#else /* CONFIG_SYSFS */

static inline int sysfs_create_dir_ns(struct kobject *kobj, const void *ns)
{
	return 0;
}

static inline void sysfs_remove_dir(struct kobject *kobj)
{
}

static inline int sysfs_rename_dir_ns(struct kobject *kobj,
				      const char *new_name, const void *new_ns)
{
	return 0;
}

static inline int sysfs_move_dir_ns(struct kobject *kobj,
				    struct kobject *new_parent_kobj,
				    const void *new_ns)
{
	return 0;
}

static inline int sysfs_create_mount_point(struct kobject *parent_kobj,
					   const char *name)
{
	return 0;
}

static inline void sysfs_remove_mount_point(struct kobject *parent_kobj,
					    const char *name)
{
}

static inline int sysfs_create_file_ns(struct kobject *kobj,
				       const struct attribute *attr,
				       const void *ns)
{
	return 0;
}

static inline int sysfs_create_files(struct kobject *kobj,
				    const struct attribute **attr)
{
	return 0;
}

static inline int sysfs_chmod_file(struct kobject *kobj,
				   const struct attribute *attr, umode_t mode)
{
	return 0;
}

static inline void sysfs_remove_file_ns(struct kobject *kobj,
					const struct attribute *attr,
					const void *ns)
{
}

static inline bool sysfs_remove_file_self(struct kobject *kobj,
					  const struct attribute *attr)
{
	return false;
}

static inline void sysfs_remove_files(struct kobject *kobj,
				     const struct attribute **attr)
{
}

static inline int sysfs_create_bin_file(struct kobject *kobj,
					const struct bin_attribute *attr)
{
	return 0;
}

static inline void sysfs_remove_bin_file(struct kobject *kobj,
					 const struct bin_attribute *attr)
{
}

static inline int sysfs_create_link(struct kobject *kobj,
				    struct kobject *target, const char *name)
{
	return 0;
}

static inline int sysfs_create_link_nowarn(struct kobject *kobj,
					   struct kobject *target,
					   const char *name)
{
	return 0;
}

static inline void sysfs_remove_link(struct kobject *kobj, const char *name)
{
}

static inline int sysfs_rename_link_ns(struct kobject *k, struct kobject *t,
				       const char *old_name,
				       const char *new_name, const void *ns)
{
	return 0;
}

static inline void sysfs_delete_link(struct kobject *k, struct kobject *t,
				     const char *name)
{
}

static inline int sysfs_create_group(struct kobject *kobj,
				     const struct attribute_group *grp)
{
	return 0;
}

static inline int sysfs_create_groups(struct kobject *kobj,
				      const struct attribute_group **groups)
{
	return 0;
}

static inline int sysfs_update_group(struct kobject *kobj,
				const struct attribute_group *grp)
{
	return 0;
}

static inline void sysfs_remove_group(struct kobject *kobj,
				      const struct attribute_group *grp)
{
}

static inline void sysfs_remove_groups(struct kobject *kobj,
				       const struct attribute_group **groups)
{
}

static inline int sysfs_add_file_to_group(struct kobject *kobj,
		const struct attribute *attr, const char *group)
{
	return 0;
}

static inline void sysfs_remove_file_from_group(struct kobject *kobj,
		const struct attribute *attr, const char *group)
{
}

static inline int sysfs_merge_group(struct kobject *kobj,
		       const struct attribute_group *grp)
{
	return 0;
}

static inline void sysfs_unmerge_group(struct kobject *kobj,
		       const struct attribute_group *grp)
{
}

static inline int sysfs_add_link_to_group(struct kobject *kobj,
		const char *group_name, struct kobject *target,
		const char *link_name)
{
	return 0;
}

static inline void sysfs_remove_link_from_group(struct kobject *kobj,
		const char *group_name, const char *link_name)
{
}

static inline int __compat_only_sysfs_link_entry_to_kobj(
	struct kobject *kobj,
	struct kobject *target_kobj,
	const char *target_name)
{
	return 0;
}

static inline void sysfs_notify(struct kobject *kobj, const char *dir,
				const char *attr)
{
}

static inline int __must_check sysfs_init(void)
{
	return 0;
}

static inline void sysfs_enable_ns(struct kernfs_node *kn)
{
}

#endif /* CONFIG_SYSFS */

static inline int __must_check sysfs_create_file(struct kobject *kobj,
						 const struct attribute *attr)
{
	return sysfs_create_file_ns(kobj, attr, NULL);
}

static inline void sysfs_remove_file(struct kobject *kobj,
				     const struct attribute *attr)
{
	sysfs_remove_file_ns(kobj, attr, NULL);
}

static inline int sysfs_rename_link(struct kobject *kobj, struct kobject *target,
				    const char *old_name, const char *new_name)
{
	return sysfs_rename_link_ns(kobj, target, old_name, new_name, NULL);
}

static inline void sysfs_notify_dirent(struct kernfs_node *kn)
{
	kernfs_notify(kn);
}

static inline struct kernfs_node *sysfs_get_dirent(struct kernfs_node *parent,
						   const unsigned char *name)
{
	return kernfs_find_and_get(parent, name);
}

static inline struct kernfs_node *sysfs_get(struct kernfs_node *kn)
{
	kernfs_get(kn);
	return kn;
}

static inline void sysfs_put(struct kernfs_node *kn)
{
	kernfs_put(kn);
}

#endif /* _SYSFS_H_ */
