/*
 * Register map access API - debugfs
 *
 * Copyright 2011 Wolfson Microelectronics plc
 *
 * Author: Mark Brown <broonie@opensource.wolfsonmicro.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/device.h>

#include "internal.h"

static struct dentry *regmap_debugfs_root;
const char *debugfs_name;

/* Calculate the length of a fixed format  */
static size_t regmap_calc_reg_len(int max_val, char *buf, size_t buf_size)
{
	return snprintf(NULL, 0, "%x", max_val);
}

static ssize_t regmap_name_read_file(struct file *file,
				     char __user *user_buf, size_t count,
				     loff_t *ppos)
{
	struct regmap *map = file->private_data;
	int ret;
	char *buf;

	buf = kmalloc(PAGE_SIZE, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	ret = snprintf(buf, PAGE_SIZE, "%s\n", map->dev->driver->name);
	if (ret < 0) {
		kfree(buf);
		return ret;
	}

	ret = simple_read_from_buffer(user_buf, count, ppos, buf, ret);
	kfree(buf);
	return ret;
}

static const struct file_operations regmap_name_fops = {
	.open = simple_open,
	.read = regmap_name_read_file,
	.llseek = default_llseek,
};

static ssize_t regmap_read_debugfs(struct regmap *map, unsigned int from,
				   unsigned int to, char __user *user_buf,
				   size_t count, loff_t *ppos)
{
	int reg_len, val_len, tot_len;
	size_t buf_pos = 0;
	loff_t p = 0;
	ssize_t ret;
	int i;
	char *buf;
	unsigned int val;

	if (*ppos < 0 || !count)
		return -EINVAL;

	buf = kmalloc(count, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	/* Calculate the length of a fixed format  */
	reg_len = regmap_calc_reg_len(map->max_register, buf, count);
	val_len = 2 * map->format.val_bytes;
	tot_len = reg_len + val_len + 3;      /* : \n */

	for (i = from; i <= to; i += map->reg_stride) {
		if (!regmap_readable(map, i))
			continue;

		if (regmap_precious(map, i))
			continue;

		/* If we're in the region the user is trying to read */
		if (p >= *ppos) {
			/* ...but not beyond it */
			if (buf_pos + 1 + tot_len >= count)
				break;

			/* Format the register */
			snprintf(buf + buf_pos, count - buf_pos, "%.*x: ",
				 reg_len, i - from);
			buf_pos += reg_len + 2;

			/* Format the value, write all X if we can't read */
			ret = regmap_read(map, i, &val);
			if (ret == 0)
				snprintf(buf + buf_pos, count - buf_pos,
					 "%.*x", val_len, val);
			else
				memset(buf + buf_pos, 'X', val_len);
			buf_pos += 2 * map->format.val_bytes;

			buf[buf_pos++] = '\n';
		}
		p += tot_len;
	}

	ret = buf_pos;

	if (copy_to_user(user_buf, buf, buf_pos)) {
		ret = -EFAULT;
		goto out;
	}

	*ppos += buf_pos;

out:
	kfree(buf);
	return ret;
}

static ssize_t regmap_map_read_file(struct file *file, char __user *user_buf,
				    size_t count, loff_t *ppos)
{
	struct regmap *map = file->private_data;

	return regmap_read_debugfs(map, 0, map->max_register, user_buf,
				   count, ppos);
}

#undef REGMAP_ALLOW_WRITE_DEBUGFS
#ifdef REGMAP_ALLOW_WRITE_DEBUGFS
/*
 * This can be dangerous especially when we have clients such as
 * PMICs, therefore don't provide any real compile time configuration option
 * for this feature, people who want to use this will need to modify
 * the source code directly.
 */
static ssize_t regmap_map_write_file(struct file *file,
				     const char __user *user_buf,
				     size_t count, loff_t *ppos)
{
	char buf[32];
	size_t buf_size;
	char *start = buf;
	unsigned long reg, value;
	struct regmap *map = file->private_data;
	int ret;

	buf_size = min(count, (sizeof(buf)-1));
	if (copy_from_user(buf, user_buf, buf_size))
		return -EFAULT;
	buf[buf_size] = 0;

	while (*start == ' ')
		start++;
	reg = simple_strtoul(start, &start, 16);
	while (*start == ' ')
		start++;
	if (strict_strtoul(start, 16, &value))
		return -EINVAL;

	/* Userspace has been fiddling around behind the kernel's back */
	add_taint(TAINT_USER, LOCKDEP_NOW_UNRELIABLE);

	ret = regmap_write(map, reg, value);
	if (ret < 0)
		return ret;
	return buf_size;
}
#else
#define regmap_map_write_file NULL
#endif

static const struct file_operations regmap_map_fops = {
	.open = simple_open,
	.read = regmap_map_read_file,
	.write = regmap_map_write_file,
	.llseek = default_llseek,
};

static ssize_t regmap_access_read_file(struct file *file,
				       char __user *user_buf, size_t count,
				       loff_t *ppos)
{
	int reg_len, tot_len;
	size_t buf_pos = 0;
	loff_t p = 0;
	ssize_t ret;
	int i;
	struct regmap *map = file->private_data;
	char *buf;

	if (*ppos < 0 || !count)
		return -EINVAL;

	buf = kmalloc(count, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	/* Calculate the length of a fixed format  */
	reg_len = regmap_calc_reg_len(map->max_register, buf, count);
	tot_len = reg_len + 10; /* ': R W V P\n' */

	for (i = 0; i <= map->max_register; i += map->reg_stride) {
		/* Ignore registers which are neither readable nor writable */
		if (!regmap_readable(map, i) && !regmap_writeable(map, i))
			continue;

		/* If we're in the region the user is trying to read */
		if (p >= *ppos) {
			/* ...but not beyond it */
			if (buf_pos + tot_len + 1 >= count)
				break;

			/* Format the register */
			snprintf(buf + buf_pos, count - buf_pos,
				 "%.*x: %c %c %c %c\n",
				 reg_len, i,
				 regmap_readable(map, i) ? 'y' : 'n',
				 regmap_writeable(map, i) ? 'y' : 'n',
				 regmap_volatile(map, i) ? 'y' : 'n',
				 regmap_precious(map, i) ? 'y' : 'n');

			buf_pos += tot_len;
		}
		p += tot_len;
	}

	ret = buf_pos;

	if (copy_to_user(user_buf, buf, buf_pos)) {
		ret = -EFAULT;
		goto out;
	}

	*ppos += buf_pos;

out:
	kfree(buf);
	return ret;
}

static const struct file_operations regmap_access_fops = {
	.open = simple_open,
	.read = regmap_access_read_file,
	.llseek = default_llseek,
};

void regmap_debugfs_init(struct regmap *map)
{
	const char *devname = "dummy";

	if (map->dev)
		devname = dev_name(map->dev);

	map->debugfs = debugfs_create_dir(devname,
					  regmap_debugfs_root);
	if (!map->debugfs) {
		dev_warn(map->dev, "Failed to create debugfs directory\n");
		return;
	}

	debugfs_create_file("name", 0400, map->debugfs,
			    map, &regmap_name_fops);

	if (map->max_register) {
		umode_t registers_mode;

		if (IS_ENABLED(REGMAP_ALLOW_WRITE_DEBUGFS))
			registers_mode = 0600;
		else
			registers_mode = 0400;

		debugfs_create_file("registers", registers_mode, map->debugfs,
				    map, &regmap_map_fops);
		debugfs_create_file("access", 0400, map->debugfs,
				    map, &regmap_access_fops);
	}

	if (map->cache_type) {
		debugfs_create_bool("cache_only", 0400, map->debugfs,
				    &map->cache_only);
		debugfs_create_bool("cache_dirty", 0400, map->debugfs,
				    &map->cache_dirty);
		debugfs_create_bool("cache_bypass", 0400, map->debugfs,
				    &map->cache_bypass);
	}
}

void regmap_debugfs_exit(struct regmap *map)
{
	debugfs_remove_recursive(map->debugfs);
	kfree(map->debugfs_name);
}

void regmap_debugfs_initcall(void)
{
	regmap_debugfs_root = debugfs_create_dir("regmap", NULL);
	if (!regmap_debugfs_root) {
		pr_warn("regmap: Failed to create debugfs root\n");
		return;
	}
}
