/* Synaptics Register Mapped Interface (RMI4) I2C Physical Layer Driver.
 * Copyright (c) 2007-2012, Synaptics Incorporated
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <asm/unaligned.h>
#include <mach/cpufreq.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/gpio.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include "synaptics_i2c_rmi.h"

#define CHAR_DEVICE_NAME "rmi"
#define DEVICE_CLASS_NAME "rmidev"
#define DEV_NUMBER 1
#define REG_ADDR_LIMIT 0xFFFF

static ssize_t rmidev_sysfs_data_show(struct file *data_file,
		struct kobject *kobj, struct bin_attribute *attributes,
		char *buf, loff_t pos, size_t count);

static ssize_t rmidev_sysfs_data_store(struct file *data_file,
		struct kobject *kobj, struct bin_attribute *attributes,
		char *buf, loff_t pos, size_t count);

static ssize_t rmidev_sysfs_open_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);

static ssize_t rmidev_sysfs_release_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);

static ssize_t rmidev_sysfs_address_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);

static ssize_t rmidev_sysfs_length_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);

static ssize_t rmidev_sysfs_attn_state_show(struct device *dev,
		struct device_attribute *attr, char *buf);

struct rmidev_handle {
	dev_t dev_no;
	unsigned short address;
	unsigned int length;
	struct device dev;
	struct synaptics_rmi4_data *rmi4_data;
	struct synaptics_rmi4_exp_fn_ptr *fn_ptr;
	struct kobject *sysfs_dir;
	void *data;
};

struct rmidev_data {
	int ref_count;
	struct cdev main_dev;
	struct class *device_class;
	struct mutex file_mutex;
	struct rmidev_handle *rmi_dev;
};

static struct bin_attribute attr_data = {
	.attr = {
		.name = "data",
		.mode = (S_IRUGO | S_IWUSR | S_IWGRP),
	},
	.size = 0,
	.read = rmidev_sysfs_data_show,
	.write = rmidev_sysfs_data_store,
};

static struct device_attribute attrs[] = {
	__ATTR(open, S_IWUSR | S_IWGRP,
			synaptics_rmi4_show_error,
			rmidev_sysfs_open_store),
	__ATTR(release, S_IWUSR | S_IWGRP,
			synaptics_rmi4_show_error,
			rmidev_sysfs_release_store),
	__ATTR(address, S_IWUSR | S_IWGRP,
			synaptics_rmi4_show_error,
			rmidev_sysfs_address_store),
	__ATTR(length, S_IWUSR | S_IWGRP,
			synaptics_rmi4_show_error,
			rmidev_sysfs_length_store),
	__ATTR(attn_state, S_IRUGO,
			rmidev_sysfs_attn_state_show,
			synaptics_rmi4_store_error),
};

static int rmidev_major_num;

static struct class *rmidev_device_class;

static struct rmidev_handle *rmidev;

static ssize_t rmidev_sysfs_data_show(struct file *data_file,
		struct kobject *kobj, struct bin_attribute *attributes,
		char *buf, loff_t pos, size_t count)
{
	int retval;
	unsigned int data_length = rmidev->length;

	if (data_length > (REG_ADDR_LIMIT - rmidev->address))
		data_length = REG_ADDR_LIMIT - rmidev->address;

	if (count < data_length) {
		dev_err(&rmidev->rmi4_data->i2c_client->dev,
				"%s: Not enough space (%d bytes) in buffer\n",
				__func__, count);
		return -EINVAL;
	}

	if (data_length) {
		retval = rmidev->fn_ptr->read(rmidev->rmi4_data,
				rmidev->address,
				(unsigned char *)buf,
				data_length);
		if (retval < 0) {
			dev_err(&rmidev->rmi4_data->i2c_client->dev,
					"%s: Failed to read data\n",
					__func__);
			return retval;
		}
	} else {
		return -EINVAL;
	}

	return data_length;
}

static ssize_t rmidev_sysfs_data_store(struct file *data_file,
		struct kobject *kobj, struct bin_attribute *attributes,
		char *buf, loff_t pos, size_t count)
{
	int retval;
	unsigned int data_length = rmidev->length;

	if (data_length > (REG_ADDR_LIMIT - rmidev->address))
		data_length = REG_ADDR_LIMIT - rmidev->address;

	if (data_length) {
		retval = rmidev->fn_ptr->write(rmidev->rmi4_data,
				rmidev->address,
				(unsigned char *)buf,
				data_length);
		if (retval < 0) {
			dev_err(&rmidev->rmi4_data->i2c_client->dev,
					"%s: Failed to write data\n",
					__func__);
			return retval;
		}
	} else {
		return -EINVAL;
	}

	return count;
}

static ssize_t rmidev_sysfs_open_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int input;

	if (sscanf(buf, "%u", &input) != 1)
			return -EINVAL;

	if (input != 1)
		return -EINVAL;

	rmidev->fn_ptr->enable(rmidev->rmi4_data, false);
	dev_dbg(&rmidev->rmi4_data->i2c_client->dev,
			"%s: Attention interrupt disabled\n",
			__func__);

	return count;
}

static ssize_t rmidev_sysfs_release_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int input;

	if (sscanf(buf, "%u", &input) != 1)
			return -EINVAL;

	if (input != 1)
		return -EINVAL;

	rmidev->fn_ptr->enable(rmidev->rmi4_data, true);
	dev_dbg(&rmidev->rmi4_data->i2c_client->dev,
			"%s: Attention interrupt enabled\n",
			__func__);

	return count;
}

static ssize_t rmidev_sysfs_address_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int input;

	if (sscanf(buf, "%u", &input) != 1)
			return -EINVAL;

	if (input > REG_ADDR_LIMIT)
		return -EINVAL;

	rmidev->address = (unsigned short)input;

	return count;
}

static ssize_t rmidev_sysfs_length_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int input;

	if (sscanf(buf, "%u", &input) != 1)
			return -EINVAL;

	if (input > REG_ADDR_LIMIT)
		return -EINVAL;

	rmidev->length = input;

	return count;
}

static ssize_t rmidev_sysfs_attn_state_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int attn_state;
	const struct synaptics_rmi4_platform_data *platform_data =
			rmidev->rmi4_data->board;

	attn_state = gpio_get_value(platform_data->gpio);

	return snprintf(buf, PAGE_SIZE, "%u\n", attn_state);
}

/*
 * rmidev_llseek - used to set up register address
 *
 * @filp: file structure for seek
 * @off: offset
 *   if whence == SEEK_SET,
 *     high 16 bits: page address
 *     low 16 bits: register address
 *   if whence == SEEK_CUR,
 *     offset from current position
 *   if whence == SEEK_END,
 *     offset from end position (0xFFFF)
 * @whence: SEEK_SET, SEEK_CUR, or SEEK_END
 */
static loff_t rmidev_llseek(struct file *filp, loff_t off, int whence)
{
	loff_t newpos;
	struct rmidev_data *dev_data = filp->private_data;

	if (IS_ERR(dev_data)) {
		pr_debug("rmi-bad thing");
		return -EBADF;
	}

	mutex_lock(&(dev_data->file_mutex));

	switch (whence) {
	case SEEK_SET:
		newpos = off;
		break;
	case SEEK_CUR:
		newpos = filp->f_pos + off;
		break;
	case SEEK_END:
		newpos = REG_ADDR_LIMIT + off;
		break;
	default:
		newpos = -EINVAL;
		goto clean_up;
	}

	if (newpos < 0 || newpos > REG_ADDR_LIMIT) {
		pr_debug("rmi-bad thing");
		newpos = -EINVAL;
		goto clean_up;
	}

	filp->f_pos = newpos;

clean_up:
	mutex_unlock(&(dev_data->file_mutex));

	return newpos;
}

/*
 * rmidev_read: - use to read data from rmi device
 *
 * @filp: file structure for read
 * @buf: user space buffer pointer
 * @count: number of bytes to read
 * @f_pos: offset (starting register address)
 */
static ssize_t rmidev_read(struct file *filp, char __user *buf,
		size_t count, loff_t *f_pos)
{
	ssize_t retval;
	unsigned char tmpbuf[count + 1];
	struct rmidev_data *dev_data = filp->private_data;

	if (IS_ERR(dev_data)) {
		pr_debug("rmi-bad thing");
		return -EBADF;
	}

	if (count == 0)
		return 0;

	if (count > (REG_ADDR_LIMIT - *f_pos))
		count = REG_ADDR_LIMIT - *f_pos;

	mutex_lock(&(dev_data->file_mutex));

	retval = rmidev->fn_ptr->read(rmidev->rmi4_data,
			*f_pos,
			tmpbuf,
			count);
	if (retval < 0)
		goto clean_up;

	if (copy_to_user(buf, tmpbuf, count))
		retval = -EFAULT;
	else
		*f_pos += retval;

clean_up:
	mutex_unlock(&(dev_data->file_mutex));

	return retval;
}

/*
 * rmidev_write: - used to write data to rmi device
 *
 * @filep: file structure for write
 * @buf: user space buffer pointer
 * @count: number of bytes to write
 * @f_pos: offset (starting register address)
 */
static ssize_t rmidev_write(struct file *filp, const char __user *buf,
		size_t count, loff_t *f_pos)
{
	ssize_t retval;
	unsigned char tmpbuf[count + 1];
	struct rmidev_data *dev_data = filp->private_data;

	if (IS_ERR(dev_data)) {
		pr_debug("rmi-bad thing");
		return -EBADF;
	}

	if (count == 0)
		return 0;

	if (count > (REG_ADDR_LIMIT - *f_pos))
		count = REG_ADDR_LIMIT - *f_pos;

	if (copy_from_user(tmpbuf, buf, count))
		return -EFAULT;

	mutex_lock(&(dev_data->file_mutex));

	retval = rmidev->fn_ptr->write(rmidev->rmi4_data,
			*f_pos,
			tmpbuf,
			count);
	if (retval >= 0)
		*f_pos += retval;

	mutex_unlock(&(dev_data->file_mutex));

	return retval;
}

/*
 * rmidev_open: enable access to rmi device
 * @inp: inode struture
 * @filp: file structure
 */
static int rmidev_open(struct inode *inp, struct file *filp)
{
	int retval = 0;
	struct rmidev_data *dev_data =
			container_of(inp->i_cdev, struct rmidev_data, main_dev);

	if (!dev_data)
		return -EACCES;

	filp->private_data = dev_data;

	mutex_lock(&(dev_data->file_mutex));

	rmidev->fn_ptr->enable(rmidev->rmi4_data, false);
	pr_debug("rmi-bad thing");

	if (dev_data->ref_count < 1)
		dev_data->ref_count++;
	else
		retval = -EACCES;

	mutex_unlock(&(dev_data->file_mutex));

	return retval;
}

/*
 * rmidev_release: - release access to rmi device
 * @inp: inode structure
 * @filp: file structure
 */
static int rmidev_release(struct inode *inp, struct file *filp)
{
	struct rmidev_data *dev_data =
			container_of(inp->i_cdev, struct rmidev_data, main_dev);

	if (!dev_data)
		return -EACCES;

	mutex_lock(&(dev_data->file_mutex));

	dev_data->ref_count--;
	if (dev_data->ref_count < 0)
		dev_data->ref_count = 0;

	rmidev->fn_ptr->enable(rmidev->rmi4_data, true);
	pr_debug("rmi-bad thing");

	mutex_unlock(&(dev_data->file_mutex));

	return 0;
}

static const struct file_operations rmidev_fops = {
	.owner = THIS_MODULE,
	.llseek = rmidev_llseek,
	.read = rmidev_read,
	.write = rmidev_write,
	.open = rmidev_open,
	.release = rmidev_release,
};

static void rmidev_device_cleanup(struct rmidev_data *dev_data)
{
	dev_t devno;

	if (dev_data) {
		devno = dev_data->main_dev.dev;

		if (dev_data->device_class)
			device_destroy(dev_data->device_class, devno);

		cdev_del(&dev_data->main_dev);

		unregister_chrdev_region(devno, 1);

		pr_debug("rmi-bad thing");
	}

	return;
}

static char *rmi_char_devnode(struct device *dev, umode_t *mode)
{
	if (!mode)
		return NULL;

	*mode = (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

	return kasprintf(GFP_KERNEL, "rmi/%s", dev_name(dev));
}

static int rmidev_create_device_class(void)
{
	rmidev_device_class = class_create(THIS_MODULE, DEVICE_CLASS_NAME);

	if (IS_ERR(rmidev_device_class)) {
		pr_debug("rmi-bad thing");
		return -ENODEV;
	}

	rmidev_device_class->devnode = rmi_char_devnode;

	return 0;
}

static int rmidev_init_device(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	dev_t dev_no;
	unsigned char attr_count;
	int attr_count_num;
	struct rmidev_data *dev_data;
	struct device *device_ptr;

	rmidev = kzalloc(sizeof(*rmidev), GFP_KERNEL);
	if (!rmidev) {
		pr_debug("rmi-bad thing");
		retval = -ENOMEM;
		goto err_rmidev;
	}

	rmidev->fn_ptr =  kzalloc(sizeof(*(rmidev->fn_ptr)), GFP_KERNEL);
	if (!rmidev) {
		pr_debug("rmi-bad thing");
		retval = -ENOMEM;
		goto err_fn_ptr;
	}

	rmidev->fn_ptr->read = rmi4_data->i2c_read;
	rmidev->fn_ptr->write = rmi4_data->i2c_write;
	rmidev->fn_ptr->enable = rmi4_data->irq_enable;
	rmidev->rmi4_data = rmi4_data;

	retval = rmidev_create_device_class();
	if (retval < 0) {
		pr_debug("rmi-bad thing");
		goto err_device_class;
	}

	if (rmidev_major_num) {
		dev_no = MKDEV(rmidev_major_num, DEV_NUMBER);
		retval = register_chrdev_region(dev_no, 1, CHAR_DEVICE_NAME);
	} else {
		retval = alloc_chrdev_region(&dev_no, 0, 1, CHAR_DEVICE_NAME);
		if (retval < 0) {
			pr_debug("rmi-bad thing");
			goto err_device_region;
		}

		rmidev_major_num = MAJOR(dev_no);
		pr_debug("rmi-bad thing");
	}

	dev_data = kzalloc(sizeof(*dev_data), GFP_KERNEL);
	if (!dev_data) {
		pr_debug("rmi-bad thing");
		retval = -ENOMEM;
		goto err_dev_data;
	}

	mutex_init(&dev_data->file_mutex);
	dev_data->rmi_dev = rmidev;
	rmidev->data = dev_data;

	cdev_init(&dev_data->main_dev, &rmidev_fops);

	retval = cdev_add(&dev_data->main_dev, dev_no, 1);
	if (retval < 0) {
		pr_debug("rmi-bad thing");
		goto err_char_device;
	}

	dev_set_name(&rmidev->dev, "rmidev%d", MINOR(dev_no));
	dev_data->device_class = rmidev_device_class;

	device_ptr = device_create(dev_data->device_class, NULL, dev_no,
			NULL, CHAR_DEVICE_NAME"%d", MINOR(dev_no));
	if (IS_ERR(device_ptr)) {
		pr_debug("rmi-bad thing");
		retval = -ENODEV;
		goto err_char_device;
	}

	retval = gpio_export(rmi4_data->board->gpio, false);
	if (retval < 0) {
		pr_debug("rmi-bad thing");
	} else {
		retval = gpio_export_link(&(rmi4_data->input_dev->dev),
				"attn", rmi4_data->board->gpio);
		if (retval < 0) {
			pr_debug("rmi-bad thing");
		} else {
			pr_debug("rmi-bad thing");
		}
	}

	rmidev->sysfs_dir = kobject_create_and_add("rmidev",
			&rmi4_data->input_dev->dev.kobj);
	if (!rmidev->sysfs_dir) {
		pr_debug("rmi-bad thing");
		goto err_sysfs_dir;
	}

	retval = sysfs_create_bin_file(rmidev->sysfs_dir,
			&attr_data);
	if (retval < 0) {
		pr_debug("rmi-bad thing");
		goto err_sysfs_bin;
	}

	for (attr_count = 0; attr_count < ARRAY_SIZE(attrs); attr_count++) {
		retval = sysfs_create_file(rmidev->sysfs_dir,
				&attrs[attr_count].attr);
		if (retval < 0) {
			pr_debug("rmi-bad thing");
			retval = -ENODEV;
			goto err_sysfs_attrs;
		}
	}

	return 0;

err_sysfs_attrs:
	attr_count_num = (int)attr_count;
	for (attr_count_num--; attr_count_num >= 0; attr_count_num--)
		sysfs_remove_file(rmidev->sysfs_dir, &attrs[attr_count].attr);

	sysfs_remove_bin_file(rmidev->sysfs_dir, &attr_data);

err_sysfs_bin:
	kobject_put(rmidev->sysfs_dir);

err_sysfs_dir:
err_char_device:
	rmidev_device_cleanup(dev_data);
	mutex_destroy(&dev_data->file_mutex);
	kfree(dev_data);

err_dev_data:
	unregister_chrdev_region(dev_no, 1);

err_device_region:
	class_destroy(rmidev_device_class);

err_device_class:
	kfree(rmidev->fn_ptr);

err_fn_ptr:
	kfree(rmidev);
	rmidev = NULL;

err_rmidev:
	return retval;
}

static void rmidev_remove_device(struct synaptics_rmi4_data *rmi4_data)
{
	unsigned char attr_count;
	struct rmidev_data *dev_data;

	if (!rmidev)
		return;

	for (attr_count = 0; attr_count < ARRAY_SIZE(attrs); attr_count++)
		sysfs_remove_file(rmidev->sysfs_dir, &attrs[attr_count].attr);

	sysfs_remove_bin_file(rmidev->sysfs_dir, &attr_data);

	kobject_put(rmidev->sysfs_dir);

	dev_data = rmidev->data;
	if (dev_data) {
		rmidev_device_cleanup(dev_data);
		mutex_destroy(&dev_data->file_mutex);
		kfree(dev_data);
	}

	unregister_chrdev_region(rmidev->dev_no, 1);

	class_destroy(rmidev_device_class);

	kfree(rmidev->fn_ptr);
	kfree(rmidev);
	rmidev = NULL;

	return;
}

int rmidev_module_register(void)
{
	int retval;

	retval = synaptics_rmi4_new_function(RMI_DEV,
			rmidev_init_device,
			rmidev_remove_device,
			NULL);

	return retval;
}
