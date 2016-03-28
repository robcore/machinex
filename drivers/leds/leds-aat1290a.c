/*
 * LED driver for AAT1290A - leds-aat1290a.c
 *
 * Copyright (C) 2011 DongHyun Chang <dh348.chang@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/leds-aat1290a.h>
int *aat1290a_ctrl;
struct aat1290a_led_platform_data *led_pdata;

struct class *flash_class;
struct device *aat1290a_dev;

static int aat1290a_setPower(int onoff, int level)
{
	int i;
	if (led_pdata->status == STATUS_AVAILABLE) {
		led_pdata->torch_en(0);
		led_pdata->torch_set(0);
		udelay(10);
		if (onoff) {
			led_pdata->torch_set(1);
			for (i = 1; i < level; i++) {
				udelay(1);
				led_pdata->torch_set(0);
				udelay(1);
				led_pdata->torch_set(1);
			}
		}
	} else {
		LED_ERROR("GPIO is not available!");
		return -ENODEV;
	}

	return 0;
}

static int aat1290a_freeGpio(void)
{
	if (led_pdata->status == STATUS_AVAILABLE) {
		if (led_pdata->freeGpio()) {
			LED_ERROR("Can't free GPIO for led\n");
			return -ENODEV;
		}
		led_pdata->status = STATUS_UNAVAILABLE;
	} else {
		LED_ERROR("GPIO already free!");
		return -1;
	}

	return 0;
}

static int aat1290a_setGpio(void)
{
	if (led_pdata->status != STATUS_AVAILABLE) {
		if (led_pdata->setGpio()) {
			LED_ERROR("Can't set GPIO for led\n");
			return -ENODEV;
		}
		led_pdata->status = STATUS_AVAILABLE;
	} else {
		LED_ERROR("GPIO already set!");
		return -1;
	}

	return 0;
}

static int aat1290a_open(struct inode *inode, struct file *file)
{
	int ret;
	led_pdata->brightness = TORCH_BRIGHTNESS_50; /*normal setting*/
	led_pdata->status = STATUS_INVALID;

	ret = aat1290a_setGpio();

	file->private_data = (int *) aat1290a_ctrl;

	return ret;
}

static int aat1290a_release(struct inode *inode, struct file *file)
{
	if (aat1290a_freeGpio()) {
		LED_ERROR("aat1290a_freeGpio failed!\n");
		return -ENODEV;
	}

	return 0;
}


static long aat1290a_ioctl(struct file *file,
				unsigned int cmd, unsigned long arg)
{
	int *ctrl = (int *)file->private_data;

	if (!ctrl) {
		LED_ERROR("invalid input data\n");
		return -1;
	}

	switch (cmd) {
	case IOCTL_AAT1290A_SET_BRIGHTNESS:
		if (*ctrl < 0 || *ctrl > TORCH_BRIGHTNESS_0)
			return -EINVAL;
		led_pdata->brightness = *ctrl;
		break;
	case IOCTL_AAT1290A_GET_STATUS:
		*ctrl = led_pdata->status;
		break;
	case IOCTL_AAT1290A_SET_POWER:
		aat1290a_setPower(*ctrl, led_pdata->brightness);
		break;
	default:
		LED_ERROR("invalid input data\n");
		return -EINVAL;
	}

	return 0;
}

static ssize_t aat1290a_power(struct device *dev,
			struct device_attribute *attr, const char *buf,
			size_t count)
{
	int brightness = 0;

	LED_ERROR("[%s] buf[0] %d, count %d", __func__, buf[0], count);

	if (count == 1)
		brightness = (int) (buf[0] - 0x30);
	else if (count == 2) {
		brightness = brightness + (int) (buf[0] - 0x30) * 10;
		brightness = brightness + (int) (buf[1] - 0x30);
	} else {
		LED_ERROR("data is too long!");
		return count;
	}

	if (brightness < 0 || brightness >= TORCH_BRIGHTNESS_INVALID) {
		LED_ERROR("brightness data is wrong! %d", brightness);
		return count;
	}

	if (brightness == 0) {
		aat1290a_setPower(0, 0);
		if (aat1290a_freeGpio()) {
			LED_ERROR("aat1290a_freeGpio failed!\n");
			return count;
		}
	} else {
		if (aat1290a_setGpio()) {
			LED_ERROR("aat1290a_setGpio failed!\n");
			return count;
		}
		aat1290a_setPower(1, brightness);
	}

	return count;
}

int aat1290a_flash_power(int onoff)
{
	if (onoff == 0) {
		aat1290a_setPower(0, 0);
		if (aat1290a_freeGpio()) {
			LED_ERROR("aat1290a_freeGpio failed!\n");
			return -EIO;
		}
	} else {
		if (aat1290a_setGpio()) {
			LED_ERROR("aat1290a_setGpio failed!\n");
			return -EIO;
		}
		aat1290a_setPower(1, TORCH_BRIGHTNESS_100);
	}

	return 0;
}

static DEVICE_ATTR(flash_power, S_IWUSR, NULL, aat1290a_power);

static const struct file_operations aat1290a_fops = {
	.owner = THIS_MODULE,
	.open = aat1290a_open,
	.release = aat1290a_release,
	.unlocked_ioctl = aat1290a_ioctl,
};

static struct miscdevice aat1290a_miscdev = {
	.minor = 255,
	.name = "aat1290a_led",
	.fops = &aat1290a_fops,
};

static int aat1290a_led_probe(struct platform_device *pdev)
{
	LED_ERROR("Probe\n");
	led_pdata =
		(struct aat1290a_led_platform_data *) pdev->dev.platform_data;

	if (misc_register(&aat1290a_miscdev)) {
		LED_ERROR("Failed to register misc driver\n");
		return -ENODEV;
	}

	flash_class = class_create(THIS_MODULE, "flash");
	if (IS_ERR(flash_class))
		LED_ERROR("Failed to create class(flash)!\n");
	aat1290a_dev = device_create(flash_class, NULL, 0, NULL, "flash");
	if (IS_ERR(aat1290a_dev))
		LED_ERROR("Failed to create device(flash)!\n");

	if (device_create_file(aat1290a_dev, &dev_attr_flash_power) < 0) {
		LED_ERROR("failed to create device file, %s\n",
				dev_attr_flash_power.attr.name);
	}
	return 0;
}

static int __devexit aat1290a_led_remove(struct platform_device *pdev)
{
	led_pdata->freeGpio();
	misc_deregister(&aat1290a_miscdev);

	device_remove_file(aat1290a_dev, &dev_attr_flash_power);
	device_destroy(flash_class, 0);
	class_destroy(flash_class);

	return 0;
}

static struct platform_driver aat1290a_led_driver = {
	.probe		= aat1290a_led_probe,
	.remove		= __devexit_p(aat1290a_led_remove),
	.driver		= {
		.name	= "aat1290a-led",
		.owner	= THIS_MODULE,
	},
};

static int __init aat1290a_led_init(void)
{
	return platform_driver_register(&aat1290a_led_driver);
}

static void __exit aat1290a_led_exit(void)
{
	platform_driver_unregister(&aat1290a_led_driver);
}

module_init(aat1290a_led_init);
module_exit(aat1290a_led_exit);

MODULE_AUTHOR("DongHyun Chang <dh348.chang@samsung.com.com>");
MODULE_DESCRIPTION("AAT1290A LED driver");
MODULE_LICENSE("GPL");
