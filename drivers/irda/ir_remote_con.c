/*
 * driver/irda IRDA driver
 *
 * Copyright (C) 2012 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/device.h>
#include <linux/ir_remote_con.h>
#include <linux/powersuspend.h>
#include "irda_fw.h"

#define MAX_SIZE 2048
#define MC96_READ_LENGTH	8

#define USE_STOP_MODE

struct ir_remocon_data {
	struct mutex			mutex;
	struct i2c_client		*client;
	struct mc96_platform_data	*pdata;
	struct power_suspend		power_suspend;
	char signal[MAX_SIZE];
	int length;
	int count;
	int dev_id;
	int ir_freq;
	int ir_sum;
	int on_off;
};

#ifdef CONFIG_POWERSUSPEND
static void ir_remocon_power_suspend(struct power_suspend *h);
static void ir_remocon_power_resume(struct power_suspend *h);
#endif

static int irda_fw_update(struct ir_remocon_data *ir_data)
{
	struct ir_remocon_data *data = ir_data;
	struct i2c_client *client = data->client;
	int i = 0, ret = 0;
	u8 buf_ir_test[8];

	data->pdata->ir_vdd_onoff(0);
	data->pdata->ir_wake_en(1);
	data->pdata->ir_vdd_onoff(1);
	msleep(1000);

	ret = i2c_master_recv(client, buf_ir_test, MC96_READ_LENGTH);
	if (ret < 0) {
		printk(KERN_ERR "%s: err %d\n", __func__, ret);
		ret = i2c_master_recv(client, buf_ir_test, MC96_READ_LENGTH);
		if (ret < 0)
			goto err_i2c_fail;
	}
	ret = buf_ir_test[2] << 8 | buf_ir_test[3];
	if (ret < FW_VERSION) {
		printk(KERN_INFO "%s: chip : %04x, bin : %04x, need update!\n",
						__func__, ret, FW_VERSION);
		data->pdata->ir_vdd_onoff(0);
		data->pdata->ir_wake_en(0);
		data->pdata->ir_vdd_onoff(1);
		msleep(100);

		ret = i2c_master_recv(client, buf_ir_test, MC96_READ_LENGTH);
		if (ret < 0)
			printk(KERN_ERR "%s: err %d\n", __func__, ret);

		ret = buf_ir_test[6] << 8 | buf_ir_test[7];

		if (ret == 0x01fe)
			printk(KERN_INFO "%s: boot mode, FW download start\n",
							__func__);
		else
			goto err_bootmode;
		msleep(30);

		for (i = 0; i < FRAME_COUNT; i++) {
			if (i == FRAME_COUNT-1) {
				ret = i2c_master_send(client,
						&IRDA_binary[i * 70], 6);
				if (ret < 0)
					goto err_update;
			} else {
				ret = i2c_master_send(client,
						&IRDA_binary[i * 70], 70);
				if (ret < 0)
					goto err_update;
			}
			msleep(30);
		}

		ret = i2c_master_recv(client, buf_ir_test, MC96_READ_LENGTH);
		if (ret < 0)
			printk(KERN_ERR "%s: err %d\n", __func__, ret);

		ret = buf_ir_test[6] << 8 | buf_ir_test[7];

		if (ret == 0x02a3)
			printk(KERN_INFO "%s: boot down complete\n", __func__);
		else
			goto err_bootmode;

		data->pdata->ir_vdd_onoff(0);
		data->pdata->ir_wake_en(1);
		data->pdata->ir_vdd_onoff(1);
		msleep(60);
		ret = i2c_master_recv(client, buf_ir_test, MC96_READ_LENGTH);

		ret = buf_ir_test[2] << 8 | buf_ir_test[3];
		printk(KERN_INFO "%s: user mode dev: %04x\n", __func__, ret);
		data->pdata->ir_vdd_onoff(0);
		data->on_off = 0;

	} else {
		printk(KERN_INFO "%s: chip : %04x, bin : %04x, not update\n",
						__func__, ret, FW_VERSION);
		data->pdata->ir_wake_en(0);
		data->pdata->ir_vdd_onoff(0);
		data->on_off = 0;
	}

	return 0;
err_i2c_fail:
	printk(KERN_ERR "%s: update fail! i2c ret : %x\n",
							__func__, ret);
err_update:
	printk(KERN_ERR "%s: update fail! count : %x, ret = %x\n",
							__func__, i, ret);
err_bootmode:
	printk(KERN_ERR "%s: update fail, ret = %x\n", __func__, ret);
	data->pdata->ir_vdd_onoff(0);
	data->on_off = 0;
	return ret;
}

static void irda_add_checksum_length(struct ir_remocon_data *ir_data, int count)
{
	struct ir_remocon_data *data = ir_data;
	int i = 0, csum = 0;

	printk(KERN_INFO "%s: length: %04x\n", __func__, count);

	data->signal[0] = count >> 8;
	data->signal[1] = count & 0xff;

	while (i < count) {
		csum += data->signal[i];
		i++;
	}

	printk(KERN_INFO "%s: checksum: %04x\n", __func__, csum);

	data->signal[count] = csum >> 8;
	data->signal[count+1] = csum & 0xff;

}

static int irda_read_device_info(struct ir_remocon_data *ir_data)
{
	struct ir_remocon_data *data = ir_data;
	struct i2c_client *client = data->client;
	u8 buf_ir_test[8];
	int ret;

	printk(KERN_INFO"%s called\n", __func__);
	data->pdata->ir_wake_en(1);
	data->pdata->ir_vdd_onoff(1);
	msleep(60);
	ret = i2c_master_recv(client, buf_ir_test, MC96_READ_LENGTH);

	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	printk(KERN_INFO "%s: buf_ir dev_id: 0x%02x, 0x%02x\n", __func__,
			buf_ir_test[2], buf_ir_test[3]);
	ret = data->dev_id = (buf_ir_test[2] << 8 | buf_ir_test[3]);

	data->pdata->ir_wake_en(0);
	data->pdata->ir_vdd_onoff(0);
	data->on_off = 0;
	return ret;
}

static void ir_remocon_work(struct ir_remocon_data *ir_data, int count)
{

	struct ir_remocon_data *data = ir_data;
	struct i2c_client *client = data->client;

	int buf_size = count+2;
	int ret;
	int sleep_timing;
	int end_data;
	int emission_time;

	printk(KERN_INFO "%s: total buf_size: %d\n", __func__, buf_size);

	irda_add_checksum_length(data, count);

	mutex_lock(&data->mutex);
	ret = i2c_master_send(client, data->signal, buf_size);
	if (ret < 0) {
		dev_err(&client->dev, "%s: err1 %d\n", __func__, ret);
		ret = i2c_master_send(client, data->signal, buf_size);
		if (ret < 0) {
			dev_err(&client->dev, "%s: err2 %d\n", __func__, ret);
			data->pdata->ir_vdd_onoff(0);
			data->pdata->ir_vdd_onoff(1);
			/* FUCK YOU FOR SLEEPING WITH LOCKS HELD!!!! msleep(60); */
			mdelay(60); //not great, but better
			data->on_off = 1;
		}
	}
	mutex_unlock(&data->mutex);

/*
	for (i = 0; i < buf_size; i++) {
		printk(KERN_INFO "%s: data[%d] : 0x%02x\n", __func__, i,
				data->signal[i]);
	}
*/
	data->count = 2;


	end_data = data->signal[count-2] << 8 | data->signal[count-1];
	emission_time = \
		(1000 * (data->ir_sum - end_data) / (data->ir_freq)) + 10;
	sleep_timing = emission_time - 130;
	if (sleep_timing > 0)
		msleep(sleep_timing);
	printk(KERN_INFO "%s: sleep_timing = %d\n", __func__, sleep_timing);
#ifndef USE_STOP_MODE
	data->pdata->ir_vdd_onoff(0);
	data->on_off = 0;
	data->pdata->ir_wake_en(0);
#endif
	data->ir_freq = 0;
	data->ir_sum = 0;
}

static ssize_t remocon_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	struct ir_remocon_data *data = dev_get_drvdata(dev);
	unsigned int _data;
	int count, i;

	for (i = 0; i < MAX_SIZE; i++) {
		if (sscanf(buf++, "%u", &_data) == 1) {
			if (_data == 0 || buf == '\0')
				break;

			if (data->count == 2) {
				data->ir_freq = _data;
				if (data->on_off) {
					data->pdata->ir_wake_en(0);
					data->pdata->ir_wake_en(1);
					msleep(20);
				} else {
					data->pdata->ir_wake_en(1);
					data->pdata->ir_vdd_onoff(1);
					msleep(60);
					data->on_off = 1;
				}
				data->signal[2] = _data >> 16;
				data->signal[3] = (_data >> 8) & 0xFF;
				data->signal[4] = _data & 0xFF;
				data->count += 3;
			} else {
				data->ir_sum += _data;
				count = data->count;
				data->signal[count] = _data >> 8;
				data->signal[count+1] = _data & 0xFF;
				data->count += 2;
			}

			while (_data > 0) {
				buf++;
				_data /= 10;
			}
		} else {
			break;
		}
	}

	ir_remocon_work(data, data->count);
	return size;
}

static ssize_t remocon_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct ir_remocon_data *data = dev_get_drvdata(dev);
	int i;
	char *bufp = buf;

	for (i = 5; i < MAX_SIZE - 1; i++) {
		if (data->signal[i] == 0 && data->signal[i+1] == 0)
			break;
		else
			bufp += sprintf(bufp, "%u,", data->signal[i]);
	}
	return strlen(buf);
}

static DEVICE_ATTR(ir_send, 0664, remocon_show, remocon_store);

static ssize_t check_ir_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct ir_remocon_data *data = dev_get_drvdata(dev);
	int ret;

	ret = irda_read_device_info(data);
	return snprintf(buf, 4, "%d\n", ret);
}

static DEVICE_ATTR(check_ir, 0664, check_ir_show, NULL);


static int ir_remocon_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct ir_remocon_data *data;
	struct device *ir_remocon_dev;
	int error;

	printk(KERN_INFO "%s probe!\n", __func__);

	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C))
		return -EIO;

	data = kzalloc(sizeof(struct ir_remocon_data), GFP_KERNEL);
	if (NULL == data) {
		pr_err("Failed to data allocate %s\n", __func__);
		error = -ENOMEM;
		goto err_free_mem;
	}

	data->client = client;
	data->pdata = client->dev.platform_data;

	data->pdata->ir_remote_init();
	mutex_init(&data->mutex);
	data->count = 2;
	data->on_off = 0;

	i2c_set_clientdata(client, data);

	irda_fw_update(data);

/*
	irda_read_device_info(data);
*/
	ir_remocon_dev = device_create(sec_class, NULL, 0, data, "sec_ir");

	if (IS_ERR(ir_remocon_dev))
		pr_err("Failed to create ir_remocon_dev device\n");

	if (device_create_file(ir_remocon_dev, &dev_attr_ir_send) < 0)
		pr_err("Failed to create device file(%s)!\n",
				dev_attr_ir_send.attr.name);

	if (device_create_file(ir_remocon_dev, &dev_attr_check_ir) < 0)
		pr_err("Failed to create device file(%s)!\n",
				dev_attr_check_ir.attr.name);

#ifdef CONFIG_POWERSUSPEND
//	data->power_suspend.level = POWER_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	data->power_suspend.suspend = ir_remocon_power_suspend;
	data->power_suspend.resume = ir_remocon_power_resume;
	register_power_suspend(&data->power_suspend);
#endif

	return 0;

err_free_mem:
	kfree(data);
	return error;
}

#if defined(CONFIG_PM) || defined(CONFIG_POWERSUSPEND)
static int ir_remocon_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct ir_remocon_data *data = i2c_get_clientdata(client);

	data->pdata->ir_vdd_onoff(0);
	data->on_off = 0;
	data->pdata->ir_wake_en(0);

	return 0;
}

static int ir_remocon_resume(struct device *dev)
{
	return 0;
}
#endif

#ifdef CONFIG_POWERSUSPEND
static void ir_remocon_power_suspend(struct power_suspend *h)
{
	struct ir_remocon_data *data;
	data = container_of(h, struct ir_remocon_data, power_suspend);
	ir_remocon_suspend(&data->client->dev);
}

static void ir_remocon_power_resume(struct power_suspend *h)
{
	struct ir_remocon_data *data;
	data = container_of(h, struct ir_remocon_data, power_suspend);
	ir_remocon_resume(&data->client->dev);
}
#endif

#if defined(CONFIG_PM) && !defined(CONFIG_POWERSUSPEND)
static const struct dev_pm_ops ir_remocon_pm_ops = {
	.suspend	= ir_remocon_suspend,
	.resume	= ir_remocon_resume,
};
#endif

static int ir_remocon_remove(struct i2c_client *client)
{
	struct ir_remocon_data *data = i2c_get_clientdata(client);

	mutex_destroy(&data->mutex);
	i2c_set_clientdata(client, NULL);
	kfree(data);
	return 0;
}

static const struct i2c_device_id mc96_id[] = {
	{"mc96", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, mc96_id);

static struct i2c_driver mc96_i2c_driver = {
	.driver = {
		.name = "mc96",
	},
	.probe = ir_remocon_probe,
	.remove = __devexit_p(ir_remocon_remove),
#if defined(CONFIG_PM) && !defined(CONFIG_POWERSUSPEND)
	.pm	= &ir_remocon_pm_ops,
#endif

	.id_table = mc96_id,
};

static int __init ir_remocon_init(void)
{
	return i2c_add_driver(&mc96_i2c_driver);
}
module_init(ir_remocon_init);

static void __exit ir_remocon_exit(void)
{
	i2c_del_driver(&mc96_i2c_driver);
}
module_exit(ir_remocon_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("SEC IR remote controller");
