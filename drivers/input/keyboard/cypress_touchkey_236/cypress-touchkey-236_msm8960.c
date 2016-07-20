/*
 * cypress_touchkey.c - Platform data for cypress touchkey driver
 *
 * Copyright (C) 2011 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#define SEC_TOUCHKEY_DEBUG
/* #define SEC_TOUCHKEY_VERBOSE_DEBUG */

#include <linux/module.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#ifdef CONFIG_HAS_POWERSUSPEND
#include <linux/powersuspend.h>
#endif
#include <linux/i2c/cypress_touchkey_msm8960.h>
#include "cypress_tkey_fw.h"
#include <linux/regulator/consumer.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include <linux/leds.h>
#include <asm/mach-types.h>

#define CYPRESS_GEN		0X00
#define CYPRESS_FW_VER		0X01
#define CYPRESS_MODULE_VER	0X02
#define CYPRESS_2ND_HOST	0X03
#define CYPRESS_THRESHOLD	0X04
#define CYPRESS_AUTO_CAL_FLG	0X05

#define CYPRESS_IDAC_MENU	0X07
#define CYPRESS_IDAC_BACK	0X06
#define CYPRESS_IDAC_HOME	0X08

#define CYPRESS_DIFF_MENU	0x0C
#define CYPRESS_DIFF_BACK	0x0A
#define CYPRESS_DIFF_HOME	0x0E

#define CYPRESS_RAW_DATA_MENU	0x10
#define CYPRESS_RAW_DATA_BACK	0x0E
#define CYPRESS_RAW_DATA_HOME	0x12
#define CYPRESS_RAW_DATA_BACK_GOGH	0x14

#define CYPRESS_LED_ON		0X10
#define CYPRESS_LED_OFF		0X20
#define CYPRESS_DATA_UPDATE	0X40
#define CYPRESS_AUTO_CAL	0X50
#define CYPRESS_LED_CONTROL_ON	0X60
#define CYPRESS_LED_CONTROL_OFF	0X70
#define CYPRESS_SLEEP		0X80
static int vol_mv_level = 33;


#define TOUCHKEY_BACKLIGHT	"button-backlight"


/* bit masks*/
#define PRESS_BIT_MASK		0X08
#define KEYCODE_BIT_MASK	0X07

#define TOUCHKEY_LOG(k, v) dev_notice(&info->client->dev, "key[%d] %d\n", k, v);
#define FUNC_CALLED dev_notice(&info->client->dev, "%s: called.\n", __func__);

#define NUM_OF_RETRY_UPDATE	3
#define NUM_OF_KEY		4

struct cypress_touchkey_info {
	struct i2c_client			*client;
	struct cypress_touchkey_platform_data	*pdata;
	struct input_dev			*input_dev;
#ifdef CONFIG_HAS_POWERSUSPEND
	struct power_suspend			power_suspend;
#endif
	char			phys[32];
	unsigned char			keycode[NUM_OF_KEY];
	u8			sensitivity[NUM_OF_KEY];
	int			irq;
	u8			fw_ver;
	void (*power_onoff)(int);
	int			touchkey_update_status;
	struct led_classdev			leds;
	enum led_brightness			brightness;
	struct mutex			touchkey_led_mutex;
	struct workqueue_struct			*led_wq;
	struct work_struct			led_work;
	bool is_powering_on;

};

#ifdef CONFIG_HAS_POWERSUSPEND
static void cypress_touchkey_power_suspend(struct power_suspend *h);
static void cypress_touchkey_power_resume(struct power_suspend *h);
#endif

static int touchkey_led_status;
static int touchled_cmd_reversed;

static void cypress_touchkey_led_work(struct work_struct *work)
{
	struct cypress_touchkey_info *info =
		container_of(work, struct cypress_touchkey_info, led_work);
	u8 buf;
	int ret;

	if (info->brightness == LED_OFF)
		buf = CYPRESS_LED_OFF;
	else
		buf = CYPRESS_LED_ON;

	mutex_lock(&info->touchkey_led_mutex);

	ret = i2c_smbus_write_byte_data(info->client, CYPRESS_GEN, buf);
	if (ret < 0)
		touchled_cmd_reversed = 1;

	touchkey_led_status = buf;

	mutex_unlock(&info->touchkey_led_mutex);
}

static void cypress_touchkey_brightness_set(struct led_classdev *led_cdev,
			enum led_brightness brightness)
{
	/* Must not sleep, use a workqueue if needed */
	struct cypress_touchkey_info *info =
		container_of(led_cdev, struct cypress_touchkey_info, leds);

	info->brightness = brightness;

	queue_work(info->led_wq, &info->led_work);
}

static void change_touch_key_led_voltage(int vol_mv)
{
	struct regulator *tled_regulator;
	int ret;
	vol_mv_level = vol_mv;

	tled_regulator = regulator_get(NULL, "8921_l10");
	if (IS_ERR(tled_regulator)) {
		pr_err("%s: failed to get resource %s\n", __func__,
			"touch_led");
		return;
	}
	ret = regulator_set_voltage(tled_regulator,
		vol_mv * 100000, vol_mv * 100000);
	if (ret)
		printk(KERN_ERR"error setting voltage\n");
	regulator_put(tled_regulator);
}

static ssize_t brightness_control(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int data;

	if (sscanf(buf, "%d\n", &data) == 1) {
		printk(KERN_ERR"[TouchKey] touch_led_brightness: %d\n", data);
		change_touch_key_led_voltage(data);
	} else {
		printk(KERN_ERR "[TouchKey] touch_led_brightness Error\n");
	}
	return size;
}

static ssize_t brightness_level_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int count;

	count = sprintf(buf, "%d\n", vol_mv_level);

	printk(KERN_DEBUG "[TouchKey] Touch LED voltage = %d\n", vol_mv_level);
	return count;
}

static irqreturn_t cypress_touchkey_interrupt(int irq, void *dev_id)
{
	struct cypress_touchkey_info *info = dev_id;
	u8 buf[10] = {0,};
	int code;
	int press;
	int ret;

	ret = gpio_get_value(info->pdata->gpio_int);
	if (ret) {
		dev_err(&info->client->dev, "not real interrupt (%d).\n", ret);
		goto out;
	}

	if (info->is_powering_on) {
		dev_err(&info->client->dev, "%s: ignoring spurious boot "
					"interrupt\n", __func__);
		return IRQ_HANDLED;
	}

#if defined(SEC_TOUCHKEY_VERBOSE_DEBUG)
	ret = i2c_smbus_read_i2c_block_data(info->client,
			CYPRESS_GEN, ARRAY_SIZE(buf), buf);
	if (ret != ARRAY_SIZE(buf)) {
		dev_err(&info->client->dev, "interrupt failed with %d.\n", ret);
		goto out;
	}
	print_hex_dump(KERN_DEBUG, "cypress_touchkey: ",
			DUMP_PREFIX_OFFSET, 32, 1, buf, 10, false);
#else
	buf[0] = i2c_smbus_read_byte_data(info->client, CYPRESS_GEN);
	if (buf[0] < 0) {
		dev_err(&info->client->dev, "interrupt failed with %d.\n", ret);
		goto out;
	}
#endif
	press = !(buf[0] & PRESS_BIT_MASK);
	code = (int)(buf[0] & KEYCODE_BIT_MASK) - 1;
	dev_dbg(&info->client->dev,
		"[TouchKey]press=%d, code=%d\n", press, code);

	if (code < 0) {
		dev_err(&info->client->dev,
				"not profer interrupt 0x%2X.\n", buf[0]);
		goto out;
	}

#if defined(SEC_TOUCHKEY_DEBUG)
	TOUCHKEY_LOG(info->keycode[code], press);
#endif

	if (touch_is_pressed && press) {
		printk(KERN_ERR "[TouchKey] don't send event because touch is pressed.\n");
		printk(KERN_ERR "[TouchKey] touch_pressed = %d\n",
							touch_is_pressed);
	} else {
		input_report_key(info->input_dev, info->keycode[code], press);
		input_sync(info->input_dev);
	}

out:
	return IRQ_HANDLED;
}

static void cypress_touchkey_con_hw(struct cypress_touchkey_info *dev_info,
								bool flag)
{
	struct cypress_touchkey_info *info =  dev_info;

	gpio_set_value(info->pdata->gpio_led_en, flag ? 1 : 0);

#if defined(SEC_TOUCHKEY_DEBUG)
	dev_notice(&info->client->dev,
			"%s : called with flag %d.\n", __func__, flag);
#endif
}


static int cypress_touchkey_auto_cal(struct cypress_touchkey_info *dev_info)
{
	struct cypress_touchkey_info *info = dev_info;
	u8 data[6] = { 0, };
	int count = 0;
	int ret = 0;
	unsigned short retry = 0;
	while (retry < 3) {

		ret = i2c_smbus_read_i2c_block_data(info->client,
				CYPRESS_GEN, 4, data);
		if (ret < 0) {
			printk(KERN_ERR "[TouchKey]i2c read fail.\n");
			return ret;
		}
		data[0] = 0x50;
		data[3] = 0x01;

		count = i2c_smbus_write_i2c_block_data(info->client,
				CYPRESS_GEN, 4, data);
		printk(KERN_DEBUG
				"[TouchKey] data[0]=%x data[1]=%x data[2]=%x data[3]=%x\n",
				data[0], data[1], data[2], data[3]);

		msleep(50);

		ret = i2c_smbus_read_i2c_block_data(info->client,
				CYPRESS_GEN, 6, data);

		if ((data[5] & 0x80)) {
			printk(KERN_DEBUG "[Touchkey] autocal Enabled\n");
			break;
		} else {
			printk(KERN_DEBUG "[Touchkey] autocal disabled, retry %d\n",
					retry);
		}
		retry = retry + 1;
	}

	if (retry == 3)
		printk(KERN_DEBUG "[Touchkey] autocal failed\n");

	return count;
}

static int cypress_touchkey_led_on(struct cypress_touchkey_info *dev_info)
{
	struct cypress_touchkey_info *info = dev_info;
	u8 buf = CYPRESS_LED_ON;

	int ret;
	ret = i2c_smbus_write_byte_data(info->client, CYPRESS_GEN, buf);
	if (ret < 0) {
		dev_err(&info->client->dev,
				"[Touchkey] i2c write error [%d]\n", ret);
	}
	return ret;
}

static int cypress_touchkey_led_off(struct cypress_touchkey_info *dev_info)
{
	struct cypress_touchkey_info *info = dev_info;
	u8 buf = CYPRESS_LED_OFF;

	int ret;
	ret = i2c_smbus_write_byte_data(info->client, CYPRESS_GEN, buf);
	if (ret < 0) {
		dev_err(&info->client->dev,
				"[Touchkey] i2c write error [%d]\n", ret);
	}
	return ret;
}

static ssize_t touch_version_read(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	u8 data;
	int count;

	data = i2c_smbus_read_byte_data(info->client, CYPRESS_FW_VER);
	count = snprintf(buf, 20, "0x%02x\n", data);

	dev_dbg(&info->client->dev,
		"[TouchKey] %s : FW Ver 0x%02x\n", __func__, data);

	return count;
}

static ssize_t touch_version_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	int count;

	count = snprintf(buf, 20, "0x%02x\n", BIN_FW_VERSION);
	return count;
}

static ssize_t touchkey_firm_status_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	int count = 0;
	char buff[16] = {0};
	dev_dbg(&info->client->dev, "[TouchKey] touchkey_update_status: %d\n",
						info->touchkey_update_status);
	if (info->touchkey_update_status == 0)
		count = snprintf(buff, sizeof(buff), "PASS\n");
	else if (info->touchkey_update_status == 1)
		count = snprintf(buff, sizeof(buff), "Downloading\n");
	else if (info->touchkey_update_status == -1)
		count = snprintf(buff, sizeof(buff), "Fail\n");
	count = snprintf(buf, sizeof(buff), "%s\n", buff);
	return count;
}

static ssize_t touch_update_read(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	int count = 0;
	char buff[16] = {0};

	dev_dbg(&info->client->dev, "[TouchKey] touchkey_update_read: %d\n",
						info->touchkey_update_status);
	if (info->touchkey_update_status == 0)
		count = snprintf(buff, sizeof(buff), "PASS\n");
	else if (info->touchkey_update_status == 1)
		count = snprintf(buff, sizeof(buff), "Downloading\n");
	else if (info->touchkey_update_status == -1)
		count = snprintf(buff, sizeof(buff), "Fail\n");
	return count;
}


static ssize_t touch_update_write(struct device *dev,
			 struct device_attribute *attr,
			 const char *buf, size_t size)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	int count = 0;
	int retry = NUM_OF_RETRY_UPDATE;
	char buff[16] = {0};
	u8 data;

	info->touchkey_update_status = 1;
	dev_err(dev, "[TOUCHKEY] touch_update_write!\n");

	disable_irq(info->irq);

	while (retry--) {
		if (ISSP_main() == 0) {
			dev_err(&info->client->dev,
				"[TOUCHKEY] Update success!\n");
			msleep(50);
			cypress_touchkey_auto_cal(info);
			info->touchkey_update_status = 0;
			count = 1;
			enable_irq(info->irq);
			break;
		}
		dev_err(&info->client->dev,
			"[TOUCHKEY] Touchkey_update failed... retry...\n");
	}

	if (retry <= 0) {
		if (info->pdata->gpio_led_en)
			cypress_touchkey_con_hw(info, false);
		msleep(300);
		count = 0;
		dev_err(&info->client->dev, "[TOUCHKEY]Touchkey_update fail\n");
		info->touchkey_update_status = -1;
		return count;
	}

	msleep(500);

	data = i2c_smbus_read_byte_data(info->client, CYPRESS_FW_VER);
	count = snprintf(buff, sizeof(buff), "0x%02x\n", data);
	dev_err(&info->client->dev,
		"[TouchKey] %s : FW Ver 0x%02x\n", __func__, data);

	return count;
}

static ssize_t touch_led_control(struct device *dev,
				 struct device_attribute *attr, const char *buf,
				 size_t size)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	int data;

	dev_dbg(&info->client->dev, "called %s\n", __func__);
	data = kstrtoul(buf, (int)NULL, 0);
	if (data == 1) {
		if (data)
			cypress_touchkey_led_on(info);
		else
			cypress_touchkey_led_off(info);
#if defined(SEC_TOUCHKEY_DEBUG)
		dev_dbg(&info->client->dev,
			"[TouchKey] touch_led_control : %d\n", data);
#endif
	} else
		dev_dbg(&info->client->dev, "[TouchKey] touch_led_control Error\n");

	return size;
}

static ssize_t touch_sensitivity_control(struct device *dev,
				struct device_attribute *attr, const char *buf,
				size_t size)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	int ret;

	ret = i2c_smbus_write_byte_data(info->client,
			CYPRESS_GEN, CYPRESS_DATA_UPDATE);
	if (ret < 0) {
		dev_err(&info->client->dev,
			"[Touchkey] fail to CYPRESS_DATA_UPDATE.\n");
		return ret;
	}
	msleep(150);
	return size;
}

static ssize_t touchkey_menu_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	static u8 menu_sensitivity;
	u8 data[2] = {0,};
	int ret;
	bool touchkey;

	touchkey = info->pdata->touchkey_order;
	if (machine_is_GOGH()) {
		ret = i2c_smbus_read_i2c_block_data(info->client,
			CYPRESS_DIFF_BACK, ARRAY_SIZE(data), data);
	} else {
		ret = i2c_smbus_read_i2c_block_data(info->client,
		touchkey ? CYPRESS_DIFF_BACK : CYPRESS_DIFF_MENU,
		ARRAY_SIZE(data), data);
	}
	if (ret != ARRAY_SIZE(data)) {
		dev_err(&info->client->dev,
			"[TouchKey] fail to read menu sensitivity.\n");
		return ret;
	}
	menu_sensitivity = ((0x00FF & data[0])<<8) | data[1];

	dev_dbg(&info->client->dev, "called %s , data : %d %d\n",
			__func__, data[0], data[1]);
	return snprintf(buf, 20, "%d\n", menu_sensitivity);

}

static ssize_t touchkey_back_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	static u8 back_sensitivity;
	u8 data[2] = {0,};
	int ret;
	bool touchkey;

	touchkey = info->pdata->touchkey_order;
	if (machine_is_GOGH()) {
		ret = i2c_smbus_read_i2c_block_data(info->client,
			CYPRESS_DIFF_HOME, ARRAY_SIZE(data), data);

	} else {
		ret = i2c_smbus_read_i2c_block_data(info->client,
		touchkey ? CYPRESS_DIFF_MENU : CYPRESS_DIFF_BACK,
		ARRAY_SIZE(data), data);
	}
	if (ret != ARRAY_SIZE(data)) {
		dev_err(&info->client->dev,
			"[TouchKey] fail to read back sensitivity.\n");
		return ret;
	}

	back_sensitivity = ((0x00FF & data[0])<<8) | data[1];

	dev_dbg(&info->client->dev, "called %s , data : %d %d\n",
			__func__, data[0], data[1]);
	return snprintf(buf, 20, "%d\n", back_sensitivity);

}

static ssize_t touchkey_home_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	static u8 home_sensitivity;
	u8 data[2] = {0,};
	int ret;

	ret = i2c_smbus_read_i2c_block_data(info->client,
		CYPRESS_DIFF_MENU, ARRAY_SIZE(data), data);
	if (ret != ARRAY_SIZE(data)) {
		dev_err(&info->client->dev,
			"[TouchKey] fail to read home sensitivity.\n");
		return ret;
	}

	home_sensitivity = ((0x00FF & data[0])<<8) | data[1];

	dev_dbg(&info->client->dev, "called %s , data : %d %d\n",
			__func__, data[0], data[1]);
	return snprintf(buf, 20, "%d\n", home_sensitivity);

}

static ssize_t touchkey_raw_data0_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	static u16 raw_data0;
	u8 data[2] = {0,};
	int ret;
	bool touchkey;

	touchkey = info->pdata->touchkey_order;
	if (machine_is_GOGH()) {
		ret = i2c_smbus_read_i2c_block_data(info->client,
			CYPRESS_RAW_DATA_MENU, ARRAY_SIZE(data), data);

	} else {
		ret = i2c_smbus_read_i2c_block_data(info->client,
		touchkey ? CYPRESS_RAW_DATA_BACK : CYPRESS_RAW_DATA_MENU,
		ARRAY_SIZE(data), data);
	}
	if (ret != ARRAY_SIZE(data)) {
		dev_err(&info->client->dev,
			"[TouchKey] fail to read MENU raw data.\n");
		return ret;
	}

	raw_data0 = ((0x00FF & data[0])<<8) | data[1];

	dev_dbg(&info->client->dev, "called %s , data : %d %d\n",
			__func__, data[0], data[1]);
	return snprintf(buf, 20, "%d\n", raw_data0);

}

static ssize_t touchkey_raw_data1_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	static u16 raw_data1;
	u8 data[2] = {0,};
	int ret;
	bool touchkey;

	touchkey = info->pdata->touchkey_order;
	if (machine_is_GOGH()) {
		ret = i2c_smbus_read_i2c_block_data(info->client,
			CYPRESS_RAW_DATA_BACK_GOGH, ARRAY_SIZE(data), data);
	} else {
		ret = i2c_smbus_read_i2c_block_data(info->client,
		touchkey ? CYPRESS_RAW_DATA_MENU : CYPRESS_RAW_DATA_BACK,
		ARRAY_SIZE(data), data);
	}
	if (ret != ARRAY_SIZE(data)) {
		dev_err(&info->client->dev,
			"[TouchKey] fail to read HOME raw data.\n");
		return ret;
	}

	raw_data1 = ((0x00FF & data[0])<<8) | data[1];

	dev_dbg(&info->client->dev, "called %s , data : %d %d\n",
			__func__, data[0], data[1]);
	return snprintf(buf, 20, "%d\n", raw_data1);

}

static ssize_t touchkey_raw_data2_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	static u16 raw_data1;
	u8 data[2] = {0,};
	int ret;

	ret = i2c_smbus_read_i2c_block_data(info->client,
		CYPRESS_RAW_DATA_HOME, ARRAY_SIZE(data), data);

	if (ret != ARRAY_SIZE(data)) {
		dev_err(&info->client->dev,
			"[TouchKey] fail to read HOME raw data.\n");
		return ret;
	}

	raw_data1 = ((0x00FF & data[0])<<8) | data[1];

	dev_dbg(&info->client->dev, "called %s , data : %d %d\n",
			__func__, data[0], data[1]);
	return snprintf(buf, 20, "%d\n", raw_data1);

}


static ssize_t touchkey_idac0_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	static u8 idac0;
	u8 data = 0;
	bool touchkey;

	touchkey = info->pdata->touchkey_order;
	if (machine_is_GOGH()) {
		data = i2c_smbus_read_byte_data(info->client,
					CYPRESS_IDAC_BACK);
	} else {
		data = i2c_smbus_read_byte_data(info->client,
			touchkey ? CYPRESS_IDAC_BACK : CYPRESS_IDAC_MENU);
	}
	dev_dbg(&info->client->dev, "called %s , data : %d\n", __func__, data);
	idac0 = data;
	return snprintf(buf, 20, "%d\n", idac0);

}

static ssize_t touchkey_idac1_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	static u8 idac1;
	u8 data = 0;
	bool touchkey;

	touchkey = info->pdata->touchkey_order;
	if (machine_is_GOGH()) {
		data = i2c_smbus_read_byte_data(info->client,
					CYPRESS_IDAC_MENU);
	} else {
		data = i2c_smbus_read_byte_data(info->client,
			touchkey ? CYPRESS_IDAC_MENU : CYPRESS_IDAC_BACK);
	}
	dev_dbg(&info->client->dev, "called %s , data : %d\n", __func__, data);
	idac1 = data;
	return snprintf(buf, 20, "%d\n", idac1);

}

static ssize_t touchkey_idac2_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	static u8 idac1;
	u8 data = 0;

	data = i2c_smbus_read_byte_data(info->client, CYPRESS_IDAC_HOME);

	dev_dbg(&info->client->dev, "called %s , data : %d\n", __func__, data);
	idac1 = data;
	return snprintf(buf, 20, "%d\n", idac1);

}

static ssize_t touchkey_threshold_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	static u8 touchkey_threshold;
	u8 data = 0;

	data = i2c_smbus_read_byte_data(info->client, CYPRESS_THRESHOLD);

	dev_dbg(&info->client->dev, "called %s , data : %d\n", __func__, data);
	touchkey_threshold = data;
	return snprintf(buf, 20, "%d\n", touchkey_threshold);
}

static ssize_t touch_autocal_testmode(struct device *dev,
		struct device_attribute *attr, const char *buf,
		size_t size)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	int count = 0;
	int on_off;
	if (sscanf(buf, "%d\n", &on_off) == 1) {
		printk(KERN_ERR "[TouchKey] Test Mode : %d\n", on_off);
		if (on_off == 1) {
			count = i2c_smbus_write_byte_data(info->client,
					CYPRESS_GEN, CYPRESS_DATA_UPDATE);
		}
	} else {
		if (info->pdata->gpio_led_en) {
			cypress_touchkey_con_hw(info, false);
			msleep(50);
			cypress_touchkey_con_hw(info, true);
			msleep(50);
		}
		cypress_touchkey_auto_cal(info);
	}

	return count;
}

static ssize_t autocalibration_enable(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t size)
{
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);
	int data;

	sscanf(buf, "%d\n", &data);

	if (data == 1)
		cypress_touchkey_auto_cal(info);
	return 0;
}

static ssize_t autocalibration_status(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	u8 data[6];
	int ret;
	struct cypress_touchkey_info *info = dev_get_drvdata(dev);

	printk(KERN_DEBUG "[Touchkey] %s\n", __func__);

	ret = i2c_smbus_read_i2c_block_data(info->client,
				CYPRESS_GEN, 6, data);
	if ((data[5] & 0x80))
		return sprintf(buf, "Enabled\n");
	else
		return sprintf(buf, "Disabled\n");
}

static DEVICE_ATTR(touchkey_firm_update_status,
		S_IRUGO | S_IWUSR | S_IWGRP, touchkey_firm_status_show, NULL);
static DEVICE_ATTR(touchkey_firm_version_panel, S_IRUGO,
				touch_version_read, NULL);
static DEVICE_ATTR(touchkey_firm_version_phone, S_IRUGO,
				touch_version_show, NULL);
static DEVICE_ATTR(touchkey_firm_update, S_IRUGO | S_IWUSR | S_IWGRP,
				touch_update_read, touch_update_write);
static DEVICE_ATTR(touchkey_brightness, S_IRUGO,
				NULL, touch_led_control);
static DEVICE_ATTR(touch_sensitivity, S_IRUGO | S_IWUSR | S_IWGRP,
				NULL, touch_sensitivity_control);
static DEVICE_ATTR(touchkey_menu, S_IRUGO, touchkey_menu_show, NULL);
static DEVICE_ATTR(touchkey_back, S_IRUGO, touchkey_back_show, NULL);
static DEVICE_ATTR(touchkey_home, S_IRUGO, touchkey_home_show, NULL);
static DEVICE_ATTR(touchkey_raw_data0, S_IRUGO, touchkey_raw_data0_show, NULL);
static DEVICE_ATTR(touchkey_raw_data1, S_IRUGO, touchkey_raw_data1_show, NULL);
static DEVICE_ATTR(touchkey_raw_data2, S_IRUGO, touchkey_raw_data2_show, NULL);
static DEVICE_ATTR(touchkey_idac0, S_IRUGO, touchkey_idac0_show, NULL);
static DEVICE_ATTR(touchkey_idac1, S_IRUGO, touchkey_idac1_show, NULL);
static DEVICE_ATTR(touchkey_idac2, S_IRUGO, touchkey_idac2_show, NULL);
static DEVICE_ATTR(touchkey_threshold, S_IRUGO, touchkey_threshold_show, NULL);
static DEVICE_ATTR(touchkey_autocal_start, S_IRUGO | S_IWUSR | S_IWGRP,
				NULL, touch_autocal_testmode);
static DEVICE_ATTR(autocal_enable, S_IRUGO | S_IWUSR | S_IWGRP, NULL,
		   autocalibration_enable);
static DEVICE_ATTR(autocal_stat, S_IRUGO | S_IWUSR | S_IWGRP,
		   autocalibration_status, NULL);
static DEVICE_ATTR(touchkey_brightness_level, S_IRUGO | S_IWUSR | S_IWGRP,
				brightness_level_show, brightness_control);


static int __devinit cypress_touchkey_probe(struct i2c_client *client,
				  const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct cypress_touchkey_platform_data *pdata =
					client->dev.platform_data;
	struct cypress_touchkey_info *info;
	struct input_dev *input_dev;
	int ret = 0;
	int i;
	int retry = NUM_OF_RETRY_UPDATE;
	int ic_fw_ver;

	struct device *sec_touchkey;
	printk("\n[Cypress Tkey] Inside cypress_touchkey_probe \n");
	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C))
		return -EIO;

	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (!info) {
		dev_err(&client->dev, "fail to memory allocation.\n");
		goto err_mem_alloc;
	}

	input_dev = input_allocate_device();
	if (!input_dev) {
		dev_err(&client->dev, "fail to allocate input device.\n");
		goto err_input_dev_alloc;
	}

	info->client = client;
	info->input_dev = input_dev;
	info->pdata = client->dev.platform_data;
	info->irq = client->irq;
	info->power_onoff = pdata->power_onoff;
	info->touchkey_update_status = 0;
	memcpy(info->keycode, pdata->touchkey_keycode,
			sizeof(pdata->touchkey_keycode));
	snprintf(info->phys, sizeof(info->phys),
			"%s/input0", dev_name(&client->dev));
	input_dev->name = "sec_touchkey";
	input_dev->phys = info->phys;
	input_dev->id.bustype = BUS_I2C;
	input_dev->dev.parent = &client->dev;

	info->is_powering_on = true;

	info->power_onoff(1);
	set_bit(EV_SYN, input_dev->evbit);
	set_bit(EV_KEY, input_dev->evbit);
	set_bit(EV_LED, input_dev->evbit);
	set_bit(LED_MISC, input_dev->ledbit);
	for (i = 0; i < ARRAY_SIZE(info->keycode); i++)
		set_bit(info->keycode[i], input_dev->keybit);

	input_set_drvdata(input_dev, info);
	mutex_init(&info->touchkey_led_mutex);

	ret = input_register_device(input_dev);
	if (ret) {
		dev_err(&client->dev, "[TOUCHKEY] failed to register input dev (%d).\n",
			ret);
		goto err_reg_input_dev;
	}

	i2c_set_clientdata(client, info);

	if (info->pdata->gpio_led_en) {
		ret = gpio_request(info->pdata->gpio_led_en,
						"gpio_touchkey_led");
		if (ret < 0) {
			dev_err(&client->dev,
				"gpio_touchkey_led gpio_request is failed\n");
			goto err_gpio_request;
		}
		gpio_tlmm_config(GPIO_CFG(info->pdata->gpio_led_en, 0,
			GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);

		cypress_touchkey_con_hw(info, true);
	}

	ret = request_threaded_irq(client->irq, NULL,
			cypress_touchkey_interrupt,
			IRQF_TRIGGER_FALLING, client->dev.driver->name, info);
	if (ret < 0) {
		dev_err(&client->dev, "Failed to request IRQ %d (err: %d).\n",
				client->irq, ret);
		goto err_req_irq;
	}

#ifdef CONFIG_HAS_POWERSUSPEND
		info->power_suspend.suspend = cypress_touchkey_power_suspend;
		info->power_suspend.resume = cypress_touchkey_power_resume;
		register_power_suspend(&info->power_suspend);
#endif /* CONFIG_HAS_POWERSUSPEND */

	info->led_wq = create_singlethread_workqueue("cypress_touchkey");
	INIT_WORK(&info->led_work, cypress_touchkey_led_work);

	info->leds.name = TOUCHKEY_BACKLIGHT;
	info->leds.brightness = LED_FULL;
	info->leds.max_brightness = LED_FULL;
	info->leds.brightness_set = cypress_touchkey_brightness_set;

	ret = led_classdev_register(&client->dev, &info->leds);
	if (ret)
		goto err_req_irq;

	msleep(20);
	ic_fw_ver = i2c_smbus_read_byte_data(client, CYPRESS_FW_VER);
	dev_err(&client->dev, "Touchkey FW Version: 0x%02x\n", ic_fw_ver);

#if defined(CONFIG_MACH_M2_ATT) || defined(CONFIG_MACH_M2_DCM) \
	|| defined(CONFIG_MACH_M2_SKT) || defined(CONFIG_MACH_M2_KDI)\
	|| defined(CONFIG_MACH_M2_SPR) || defined(CONFIG_MACH_M2_VZW)

	dev_err(&client->dev, "Touchkey FW Version: 0x%02x\n",
						ic_fw_ver);
	if (0 /* ic_fw_ver < BIN_FW_VERSION */) {
		dev_err(&client->dev, "[TOUCHKEY] touchkey_update Start!!\n");
		disable_irq(client->irq);

		while (retry--) {
			if (ISSP_main() == 0) {
				dev_err(&client->dev, "[TOUCHKEY] Update success!\n");
				enable_irq(client->irq);
				break;
			}
			dev_err(&client->dev,
				"[TOUCHKEY] Touchkey_update failed... retry...\n");
		}

		if (retry <= 0) {
			if (info->pdata->gpio_led_en)
				cypress_touchkey_con_hw(info, false);
			msleep(300);
			dev_err(&client->dev, "[TOUCHKEY]Touchkey_update fail\n");
		}

		msleep(500);

		ic_fw_ver = i2c_smbus_read_byte_data(info->client,
				CYPRESS_FW_VER);
		dev_err(&client->dev,
			"[TouchKey] %s : FW Ver 0x%02x\n", __func__, ic_fw_ver);
	} else {
		dev_err(&client->dev, "[TouchKey] FW update does not need!\n");
	}
#endif
	cypress_touchkey_auto_cal(info);
	sec_touchkey = device_create(sec_class, NULL, 0, NULL, "sec_touchkey");
	if (IS_ERR(sec_touchkey)) {
		pr_err("Failed to create device(sec_touchkey)!\n");
		goto err_sysfs;
	}
	dev_set_drvdata(sec_touchkey, info);


	if (device_create_file(sec_touchkey,
			&dev_attr_touchkey_firm_update_status) < 0) {
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_touchkey_firm_update.attr.name);
		goto err_sysfs;
	}

	if (device_create_file(sec_touchkey,
			&dev_attr_touchkey_firm_update) < 0) {
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_touchkey_firm_update.attr.name);
		goto err_sysfs;
	}

	if (device_create_file(sec_touchkey,
			&dev_attr_touchkey_firm_version_panel) < 0) {
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_touchkey_firm_version_panel.attr.name);
		goto err_sysfs;
	}

	if (device_create_file(sec_touchkey,
			&dev_attr_touchkey_firm_version_phone) < 0) {
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_touchkey_firm_version_phone.attr.name);
		goto err_sysfs;
	}

	if (device_create_file(sec_touchkey,
			&dev_attr_touchkey_brightness) < 0) {
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_touchkey_brightness.attr.name);
		goto err_sysfs;
	}

	if (device_create_file(sec_touchkey,
			&dev_attr_touch_sensitivity) < 0) {
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_touch_sensitivity.attr.name);
		goto err_sysfs;
	}

	if (device_create_file(sec_touchkey,
			&dev_attr_touchkey_menu) < 0) {
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_touchkey_menu.attr.name);
		goto err_sysfs;
	}

	if (device_create_file(sec_touchkey,
			&dev_attr_touchkey_back) < 0) {
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_touchkey_back.attr.name);
		goto err_sysfs;
	}

	if (device_create_file(sec_touchkey,
			&dev_attr_touchkey_home) < 0) {
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_touchkey_home.attr.name);
		goto err_sysfs;
	}

	if (device_create_file(sec_touchkey,
			&dev_attr_touchkey_raw_data0) < 0) {
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_touchkey_raw_data0.attr.name);
		goto err_sysfs;
	}

	if (device_create_file(sec_touchkey,
			&dev_attr_touchkey_raw_data1) < 0) {
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_touchkey_raw_data1.attr.name);
		goto err_sysfs;
	}

	if (device_create_file(sec_touchkey,
			&dev_attr_touchkey_raw_data2) < 0) {
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_touchkey_raw_data2.attr.name);
		goto err_sysfs;
	}

	if (device_create_file(sec_touchkey, &dev_attr_touchkey_idac0) < 0) {
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_touchkey_idac0.attr.name);
		goto err_sysfs;
	}

	if (device_create_file(sec_touchkey, &dev_attr_touchkey_idac1) < 0) {
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_touchkey_idac1.attr.name);
		goto err_sysfs;
	}

	if (device_create_file(sec_touchkey, &dev_attr_touchkey_idac2) < 0) {
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_touchkey_idac2.attr.name);
		goto err_sysfs;
	}

	if (device_create_file(sec_touchkey,
			&dev_attr_touchkey_threshold) < 0) {
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_touchkey_threshold.attr.name);
		goto err_sysfs;
	}

	if (device_create_file(sec_touchkey,
			&dev_attr_touchkey_autocal_start) < 0) {
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_touchkey_autocal_start.attr.name);
		goto err_sysfs;
	}

	if (device_create_file(sec_touchkey,
			&dev_attr_autocal_enable) < 0) {
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_autocal_enable.attr.name);
		goto err_sysfs;
	}

	if (device_create_file(sec_touchkey,
			&dev_attr_autocal_stat) < 0) {
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_autocal_stat.attr.name);
		goto err_sysfs;
	}

	if (device_create_file(sec_touchkey,
		&dev_attr_touchkey_brightness_level) < 0) {
		printk(KERN_ERR "Failed to create device file(%s)!\n",
		dev_attr_touchkey_brightness_level.attr.name);
		goto err_sysfs;
	}
	info->is_powering_on = false;
	return 0;

err_req_irq:
err_gpio_request:
	input_unregister_device(input_dev);
err_reg_input_dev:
	input_free_device(input_dev);
	input_dev = NULL;
	mutex_destroy(&info->touchkey_led_mutex);
err_input_dev_alloc:
	kfree(info);
err_sysfs:
	return -ENXIO;
err_mem_alloc:
	return ret;

}

static int __devexit cypress_touchkey_remove(struct i2c_client *client)
{
	struct cypress_touchkey_info *info = i2c_get_clientdata(client);
	if (info->irq >= 0)
		free_irq(info->irq, info);
	mutex_destroy(&info->touchkey_led_mutex);
	led_classdev_unregister(&info->leds);
	input_unregister_device(info->input_dev);
	input_free_device(info->input_dev);
	kfree(info);
	return 0;
}

#if defined(CONFIG_PM) || defined(CONFIG_HAS_POWERSUSPEND)
static int cypress_touchkey_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cypress_touchkey_info *info = i2c_get_clientdata(client);
	int ret = 0;

	info->is_powering_on = true;
	disable_irq(info->irq);
	if (info->pdata->gpio_led_en)
		cypress_touchkey_con_hw(info, false);
	info->power_onoff(0);
	return ret;
}

static int cypress_touchkey_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cypress_touchkey_info *info = i2c_get_clientdata(client);
	int ret = 0;
	info->power_onoff(1);
	if (info->pdata->gpio_led_en)
		cypress_touchkey_con_hw(info, true);
	msleep(100);

	cypress_touchkey_auto_cal(info);

	if (touchled_cmd_reversed) {
			touchled_cmd_reversed = 0;
			i2c_smbus_write_byte_data(info->client,
					CYPRESS_GEN, touchkey_led_status);
			printk(KERN_DEBUG "LED returned on\n");
		}


	enable_irq(info->irq);


	info->is_powering_on = false;
	return ret;
}
#endif

#ifdef CONFIG_HAS_POWERSUSPEND
static void cypress_touchkey_power_suspend(struct power_suspend *h)
{
	struct cypress_touchkey_info *info;
	info = container_of(h, struct cypress_touchkey_info, power_suspend);
	cypress_touchkey_suspend(&info->client->dev);
}

static void cypress_touchkey_power_resume(struct power_suspend *h)
{
	struct cypress_touchkey_info *info;
	info = container_of(h, struct cypress_touchkey_info, power_suspend);
	cypress_touchkey_resume(&info->client->dev);
}
#endif

static const struct i2c_device_id cypress_touchkey_id[] = {
	{"cypress_touchkey", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, cypress_touchkey_id);

#if defined(CONFIG_PM) && !defined(CONFIG_HAS_POWERSUSPEND)
static const struct dev_pm_ops cypress_touchkey_pm_ops = {
	.suspend	= cypress_touchkey_suspend,
	.resume		= cypress_touchkey_resume,
};
#endif

struct i2c_driver cypress_touchkey_driver = {
	.probe = cypress_touchkey_probe,
	.remove = cypress_touchkey_remove,
	.driver = {
		.name = "cypress_touchkey",
#if defined(CONFIG_PM) && !defined(CONFIG_HAS_POWERSUSPEND)
		.pm	= &cypress_touchkey_pm_ops,
#endif
		   },
	.id_table = cypress_touchkey_id,
};

static int __init cypress_touchkey_init(void)
{
	int ret = 0;

	ret = i2c_add_driver(&cypress_touchkey_driver);
	if (ret) {
		pr_err("[TouchKey] cypress touch keypad registration failed. ret= %d\n",
			ret);
	}

	return ret;
}

static void __exit cypress_touchkey_exit(void)
{
	i2c_del_driver(&cypress_touchkey_driver);
}

late_initcall(cypress_touchkey_init);
module_exit(cypress_touchkey_exit);

MODULE_DESCRIPTION("Touchkey driver for Cypress touchkey controller ");
MODULE_LICENSE("GPL");
