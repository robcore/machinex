/*
 * Omniboost.
 * Copyright (c) 2017, Robert Patershuk. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/notifier.h>
#include <linux/cpu.h>
#include <linux/sched.h>
#include <linux/jiffies.h>
#include <linux/input.h>
#include <linux/time.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/cpufreq.h>
#include <linux/display_state.h>

static bool __read_mostly boost_cores_first = true;
module_param(boost_cores_first, bool, 0644);

static void omniboost_input_event(struct input_handle *handle,
		unsigned int type, unsigned int code, int value)
{
	if (!is_display_on())
		return;
/*boost functions go here*/
	if (likely(boost_cores_first)) {
		intelli_boost();
		autosmp_input_boost();
		cpu_boost_event();
	} else {
		cpu_boost_event();
		intelli_boost();
		autosmp_input_boost();
	}
}

static int input_dev_filter(struct input_dev *dev) {
	if (strstr(dev->name, "pmic8xxx_pwrkey") ||
		strstr(dev->name, "sec_touchscreen") ||
		strstr(dev->name, "apq8064-tabla-snd-card Button Jack") ||
		strstr(dev->name, "apq8064-tabla-snd-card Headset Jack") ||
		strstr(dev->name, "gpio-keys") ||
		strstr(dev->name, "sec_touchkey")) {
		return 0;
	} else {
		return 1;
	}
}

static int omniboost_input_connect(struct input_handler *handler,
				 struct input_dev *dev,
				 const struct input_device_id *id)
{
	struct input_handle *handle;
	int err;


	if (input_dev_filter(dev))
		return -ENODEV;

	handle = kzalloc(sizeof(struct input_handle), GFP_KERNEL);
	if (!handle)
		return -ENOMEM;

	handle->dev = dev;
	handle->handler = handler;
	handle->name = handler->name;

	err = input_register_handle(handle);
	if (err)
		goto err_register;

	err = input_open_device(handle);
	if (err)
		goto err_open;

	return 0;
err_open:
	input_unregister_handle(handle);
err_register:
	kfree(handle);
	return err;
}

static void omniboost_input_disconnect(struct input_handle *handle)
{
	input_close_device(handle);
	input_unregister_handle(handle);
	kfree(handle);
}

static const struct input_device_id omniboost_ids[] = {
	{ .driver_info = 1 },
	{ },
};

static struct input_handler omniboost_input_handler = {
	.event          = omniboost_input_event,
	.connect        = omniboost_input_connect,
	.disconnect     = omniboost_input_disconnect,
	.name           = "omniboost",
	.id_table       = omniboost_ids,
};

static int omniboost_init(void)
{
	int ret;

	ret = input_register_handler(&omniboost_input_handler);
	if (ret)
		pr_err("Cannot register omniboost input handler.\n");

	return ret;
}
late_initcall(omniboost_init);
