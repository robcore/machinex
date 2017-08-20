/*
 * Driver for keys on GPIO lines capable of generating interrupts.
 *
 * Copyright 2005 Phil Blundell
 * Copyright 2010, 2011 David Jander <david@protonic.nl>
 * Copyright (c) 2015, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>

#include <linux/init.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/pm.h>
#include <linux/slab.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/input.h>
#include <linux/gpio_keys.h>
#include <linux/workqueue.h>
#include <linux/gpio.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/spinlock.h>
#include <linux/sec_class.h>
#include <linux/i2c/synaptics_rmi.h>
#include <linux/timed_output.h>
#include <linux/display_state.h>
#include <mach/jf_eur-gpio.h>
#include <linux/cpufreq.h>

struct gpio_button_data {
	struct gpio_keys_button *button;
	struct input_dev *input;

	struct timer_list release_timer;
	unsigned int release_delay;	/* in msecs, for IRQ-only buttons */

	struct delayed_work work;
	unsigned int software_debounce;	/* in msecs, for GPIO-driven buttons */

	unsigned int irq;
	spinlock_t lock;
	bool disabled;
	bool key_pressed;
	bool suspended;
};

static struct workqueue_struct *flipqueue;

struct gpio_keys_drvdata {
	const struct gpio_keys_platform_data *pdata;
	struct input_dev *input;
	struct device *sec_key;
	struct mutex disable_lock;
#if defined(CONFIG_SENSORS_HALL)
	int gpio_flip_cover;
	bool flip_cover;
	struct delayed_work flip_cover_dwork;
	//struct wake_lock flip_wake_lock;
#endif
	struct gpio_button_data data[0];
};

static struct device *global_dev;

/*
 * SYSFS interface for enabling/disabling keys and switches:
 *
 * There are 4 attributes under /sys/devices/platform/gpio-keys/
 *	keys [ro]              - bitmap of keys (EV_KEY) which can be
 *	                         disabled
 *	switches [ro]          - bitmap of switches (EV_SW) which can be
 *	                         disabled
 *	disabled_keys [rw]     - bitmap of keys currently disabled
 *	disabled_switches [rw] - bitmap of switches currently disabled
 *
 * Userland can change these values and hence disable event generation
 * for each key (or switch). Disabling a key means its interrupt line
 * is disabled.
 *
 * For example, if we have following switches set up as gpio-keys:
 *	SW_DOCK = 5
 *	SW_CAMERA_LENS_COVER = 9
 *	SW_KEYPAD_SLIDE = 10
 *	SW_FRONT_PROXIMITY = 11
 * This is read from switches:
 *	11-9,5
 * Next we want to disable proximity (11) and dock (5), we write:
 *	11,5
 * to file disabled_switches. Now proximity and dock IRQs are disabled.
 * This can be verified by reading the file disabled_switches:
 *	11,5
 * If we now want to enable proximity (11) switch we write:
 *	5
 * to disabled_switches.
 *
 * We can disable only those keys which don't allow sharing the irq.
 */

/**
 * get_n_events_by_type() - returns maximum number of events per @type
 * @type: type of button (%EV_KEY, %EV_SW)
 *
 * Return value of this function can be used to allocate bitmap
 * large enough to hold all bits for given type.
 */
static int get_n_events_by_type(int type)
{
	BUG_ON(type != EV_SW && type != EV_KEY);

	return (type == EV_KEY) ? KEY_CNT : SW_CNT;
}

/**
 * get_bm_events_by_type() - returns bitmap of supported events per @type
 * @input: input device from which bitmap is retrieved
 * @type: type of button (%EV_KEY, %EV_SW)
 *
 * Return value of this function can be used to allocate bitmap
 * large enough to hold all bits for given type.
 */
static const unsigned long *get_bm_events_by_type(struct input_dev *dev,
						  int type)
{
	BUG_ON(type != EV_SW && type != EV_KEY);

	return (type == EV_KEY) ? dev->keybit : dev->swbit;
}

/**
 * gpio_keys_disable_button() - disables given GPIO button
 * @bdata: button data for button to be disabled
 *
 * Disables button pointed by @bdata. This is done by masking
 * IRQ line. After this function is called, button won't generate
 * input events anymore. Note that one can only disable buttons
 * that don't share IRQs.
 *
 * Make sure that @bdata->disable_lock is locked when entering
 * this function to avoid races when concurrent threads are
 * disabling buttons at the same time.
 */
static void gpio_keys_disable_button(struct gpio_button_data *bdata)
{
	if (!bdata->disabled) {
		/*
		 * Disable IRQ and associated timer/work structure.
		 */
		disable_irq(bdata->irq);

		if (gpio_is_valid(bdata->button->gpio))
			cancel_delayed_work_sync(&bdata->work);
		else
			del_timer_sync(&bdata->release_timer);

		bdata->disabled = true;
	}
}

/**
 * gpio_keys_enable_button() - enables given GPIO button
 * @bdata: button data for button to be disabled
 *
 * Enables given button pointed by @bdata.
 *
 * Make sure that @bdata->disable_lock is locked when entering
 * this function to avoid races with concurrent threads trying
 * to enable the same button at the same time.
 */
static void gpio_keys_enable_button(struct gpio_button_data *bdata)
{
	if (bdata->disabled) {
		enable_irq(bdata->irq);
		bdata->disabled = false;
	}
}

/**
 * gpio_keys_attr_show_helper() - fill in stringified bitmap of buttons
 * @ddata: pointer to drvdata
 * @buf: buffer where stringified bitmap is written
 * @type: button type (%EV_KEY, %EV_SW)
 * @only_disabled: does caller want only those buttons that are
 *                 currently disabled or all buttons that can be
 *                 disabled
 *
 * This function writes buttons that can be disabled to @buf. If
 * @only_disabled is true, then @buf contains only those buttons
 * that are currently disabled. Returns 0 on success or negative
 * errno on failure.
 */
static ssize_t gpio_keys_attr_show_helper(struct gpio_keys_drvdata *ddata,
					  char *buf, unsigned int type,
					  bool only_disabled)
{
	int n_events = get_n_events_by_type(type);
	unsigned long *bits;
	ssize_t ret;
	int i;

	bits = kcalloc(BITS_TO_LONGS(n_events), sizeof(*bits), GFP_KERNEL);
	if (!bits)
		return -ENOMEM;

	for (i = 0; i < ddata->pdata->nbuttons; i++) {
		struct gpio_button_data *bdata = &ddata->data[i];

		if (bdata->button->type != type)
			continue;

		if (only_disabled && !bdata->disabled)
			continue;

		__set_bit(bdata->button->code, bits);
	}

	ret = scnprintf(buf, PAGE_SIZE - 1, "%*pbl", n_events, bits);
	buf[ret++] = '\n';
	buf[ret] = '\0';

	kfree(bits);

	return ret;
}

/**
 * gpio_keys_attr_store_helper() - enable/disable buttons based on given bitmap
 * @ddata: pointer to drvdata
 * @buf: buffer from userspace that contains stringified bitmap
 * @type: button type (%EV_KEY, %EV_SW)
 *
 * This function parses stringified bitmap from @buf and disables/enables
 * GPIO buttons accordingly. Returns 0 on success and negative error
 * on failure.
 */
static ssize_t gpio_keys_attr_store_helper(struct gpio_keys_drvdata *ddata,
					   const char *buf, unsigned int type)
{
	int n_events = get_n_events_by_type(type);
	const unsigned long *bitmap = get_bm_events_by_type(ddata->input, type);
	unsigned long *bits;
	ssize_t error;
	int i;

	bits = kcalloc(BITS_TO_LONGS(n_events), sizeof(*bits), GFP_KERNEL);
	if (!bits)
		return -ENOMEM;

	error = bitmap_parselist(buf, bits, n_events);
	if (error)
		goto out;

	/* First validate */
	if (!bitmap_subset(bits, bitmap, n_events)) {
		error = -EINVAL;
		goto out;
	}

	for (i = 0; i < ddata->pdata->nbuttons; i++) {
		struct gpio_button_data *bdata = &ddata->data[i];

		if (bdata->button->type != type)
			continue;

		if (test_bit(bdata->button->code, bits) &&
		    !bdata->button->can_disable) {
			error = -EINVAL;
			goto out;
		}
	}

	mutex_lock(&ddata->disable_lock);

	for (i = 0; i < ddata->pdata->nbuttons; i++) {
		struct gpio_button_data *bdata = &ddata->data[i];

		if (bdata->button->type != type)
			continue;

		if (test_bit(bdata->button->code, bits))
			gpio_keys_disable_button(bdata);
		else
			gpio_keys_enable_button(bdata);
	}

	mutex_unlock(&ddata->disable_lock);

out:
	kfree(bits);
	return error;
}

#define ATTR_SHOW_FN(name, type, only_disabled)				\
static ssize_t gpio_keys_show_##name(struct device *dev,		\
				     struct device_attribute *attr,	\
				     char *buf)				\
{									\
	struct platform_device *pdev = to_platform_device(dev);		\
	struct gpio_keys_drvdata *ddata = platform_get_drvdata(pdev);	\
									\
	return gpio_keys_attr_show_helper(ddata, buf,			\
					  type, only_disabled);		\
}

ATTR_SHOW_FN(keys, EV_KEY, false);
ATTR_SHOW_FN(switches, EV_SW, false);
ATTR_SHOW_FN(disabled_keys, EV_KEY, true);
ATTR_SHOW_FN(disabled_switches, EV_SW, true);

/*
 * ATTRIBUTES:
 *
 * /sys/devices/platform/gpio-keys/keys [ro]
 * /sys/devices/platform/gpio-keys/switches [ro]
 */
static DEVICE_ATTR(keys, S_IRUGO, gpio_keys_show_keys, NULL);
static DEVICE_ATTR(switches, S_IRUGO, gpio_keys_show_switches, NULL);

#define ATTR_STORE_FN(name, type)					\
static ssize_t gpio_keys_store_##name(struct device *dev,		\
				      struct device_attribute *attr,	\
				      const char *buf,			\
				      size_t count)			\
{									\
	struct platform_device *pdev = to_platform_device(dev);		\
	struct gpio_keys_drvdata *ddata = platform_get_drvdata(pdev);	\
	ssize_t error;							\
									\
	error = gpio_keys_attr_store_helper(ddata, buf, type);		\
	if (error)							\
		return error;						\
									\
	return count;							\
}

ATTR_STORE_FN(disabled_keys, EV_KEY);
ATTR_STORE_FN(disabled_switches, EV_SW);

/*
 * ATTRIBUTES:
 *
 * /sys/devices/platform/gpio-keys/disabled_keys [rw]
 * /sys/devices/platform/gpio-keys/disables_switches [rw]
 */
static DEVICE_ATTR(disabled_keys, S_IWUSR | S_IRUGO,
		   gpio_keys_show_disabled_keys,
		   gpio_keys_store_disabled_keys);
static DEVICE_ATTR(disabled_switches, S_IWUSR | S_IRUGO,
		   gpio_keys_show_disabled_switches,
		   gpio_keys_store_disabled_switches);

static struct attribute *gpio_keys_attrs[] = {
	&dev_attr_keys.attr,
	&dev_attr_switches.attr,
	&dev_attr_disabled_keys.attr,
	&dev_attr_disabled_switches.attr,
	NULL,
};

static struct attribute_group gpio_keys_attr_group = {
	.attrs = gpio_keys_attrs,
};

static bool fakepressed = false;
static void gpio_keys_gpio_report_event(struct gpio_button_data *bdata)
{
	const struct gpio_keys_button *button = bdata->button;
	struct input_dev *input = bdata->input;
	unsigned int type = button->type ?: EV_KEY;
	int state = (gpio_get_value_cansleep(button->gpio));

	if (state < 0)
		return;

	state = (state ? 1 : 0) ^ button->active_low;

	if (type == EV_ABS) {
		if (state)
			input_event(input, type, button->code, button->value);
	} else {
		input_event(input, type, button->code, !!state);
	}
	input_sync(input);
}

static void gpio_keys_gpio_work_func(struct work_struct *work)
{
	struct gpio_button_data *bdata =
		container_of(work, struct gpio_button_data, work.work);

	gpio_keys_gpio_report_event(bdata);
	cpuboost_keypress_event();

	if (bdata->button->wakeup) {
		if (fakepressed) {
			const struct gpio_keys_button *button = bdata->button;
			input_report_key(bdata->input, button->code, 0);
			gpio_keys_gpio_report_event(bdata);
			fakepressed = false;
		}
		pm_relax(bdata->input->dev.parent);
	}
}

static irqreturn_t gpio_keys_gpio_isr(int irq, void *dev_id)
{
	struct gpio_button_data *bdata = dev_id;

	BUG_ON(irq != bdata->irq);

	if (bdata->button->wakeup) {
		const struct gpio_keys_button *button = bdata->button;

		pm_stay_awake(bdata->input->dev.parent);
		if (bdata->suspended  &&
		    (button->type == 0 || button->type == EV_KEY)) {
			/*
			 * Simulate wakeup key press in case the key has
			 * already released by the time we got interrupt
			 * handler to run.
			 */
			input_report_key(bdata->input, button->code, 1);
			fakepressed = true;
		}
	}

	mod_delayed_work(system_wq,
			 &bdata->work,
			 msecs_to_jiffies(bdata->software_debounce));

	return IRQ_HANDLED;
}

static void gpio_keys_irq_timer(unsigned long _data)
{
	struct gpio_button_data *bdata = (struct gpio_button_data *)_data;
	struct input_dev *input = bdata->input;
	unsigned long flags;

	spin_lock_irqsave(&bdata->lock, flags);
	if (bdata->key_pressed) {
		input_event(input, EV_KEY, bdata->button->code, 0);
		input_sync(input);
		bdata->key_pressed = false;
	}
	spin_unlock_irqrestore(&bdata->lock, flags);
}

static irqreturn_t gpio_keys_irq_isr(int irq, void *dev_id)
{
	struct gpio_button_data *bdata = dev_id;
	const struct gpio_keys_button *button = bdata->button;
	struct input_dev *input = bdata->input;
	unsigned long flags;

	BUG_ON(irq != bdata->irq);

	spin_lock_irqsave(&bdata->lock, flags);

	if (!bdata->key_pressed) {
		if (bdata->button->wakeup)
			pm_wakeup_event(bdata->input->dev.parent, 0);

		input_event(input, EV_KEY, button->code, 1);
		input_sync(input);

		if (!bdata->release_delay) {
			input_event(input, EV_KEY, button->code, 0);
			input_sync(input);
			goto out;
		}

		bdata->key_pressed = true;
	}

	if (bdata->release_delay)
		mod_timer(&bdata->release_timer,
			jiffies + msecs_to_jiffies(bdata->release_delay));
out:
	spin_unlock_irqrestore(&bdata->lock, flags);
	return IRQ_HANDLED;
}

static void gpio_keys_quiesce_key(void *data)
{
	struct gpio_button_data *bdata = data;

	if (gpio_is_valid(bdata->button->gpio))
		cancel_delayed_work_sync(&bdata->work);
	else
		del_timer_sync(&bdata->release_timer);
}

static int gpio_keys_setup_key(struct platform_device *pdev,
				struct input_dev *input,
				struct gpio_button_data *bdata,
				struct gpio_keys_button *button)
{
	const char *desc = button->desc ? button->desc : "gpio_keys";
	struct device *dev = &pdev->dev;
	irq_handler_t isr;
	unsigned long irqflags;
	int irq;
	int error;

	bdata->input = input;
	bdata->button = button;
	spin_lock_init(&bdata->lock);

	if (gpio_is_valid(button->gpio)) {

		unsigned flags = GPIOF_IN;

		error = devm_gpio_request_one(dev, button->gpio, flags, desc);
		if (error < 0) {
			dev_dbg(dev, "Failed to request GPIO %d, error %d\n",
				button->gpio, error);
			return error;
		}

		if (button->debounce_interval) {
			error = gpio_set_debounce(button->gpio,
					button->debounce_interval * 1000);
			/* use timer if gpiolib doesn't provide debounce */
			if (error < 0)
				bdata->software_debounce =
						button->debounce_interval;
		}

		if (button->irq) {
			bdata->irq = button->irq;
		} else {
			irq = gpio_to_irq(button->gpio);
			if (irq < 0) {
				error = irq;
				dev_err(dev,
					"Unable to get irq number for GPIO %d, error %d\n",
					button->gpio, error);
				return error;
			}
			bdata->irq = irq;
		}

		INIT_DELAYED_WORK(&bdata->work, gpio_keys_gpio_work_func);

		isr = gpio_keys_gpio_isr;
		irqflags = IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING;

	} else {
		if (!button->irq) {
			dev_dbg(dev, "No IRQ specified\n");
			return -EINVAL;
		}
		bdata->irq = button->irq;

		if (button->type && button->type != EV_KEY) {
			dev_dbg(dev, "Only EV_KEY allowed for IRQ buttons.\n");
			return -EINVAL;
		}

		bdata->release_delay = button->debounce_interval;
		setup_timer(&bdata->release_timer,
			    gpio_keys_irq_timer, (unsigned long)bdata);

		isr = gpio_keys_irq_isr;
		irqflags = 0;
	}

	input_set_capability(input, button->type ?: EV_KEY, button->code);

	/*
	 * Install custom action to cancel release timer and
	 * workqueue item.
	 */
	error = devm_add_action(dev, gpio_keys_quiesce_key, bdata);
	if (error) {
		dev_err(dev, "failed to register quiesce action, error: %d\n",
			error);
		return error;
	}

	/*
	 * If platform has specified that the button can be disabled,
	 * we don't want it to share the interrupt line.
	 */
	if (!button->can_disable)
		irqflags |= IRQF_SHARED;

	error = devm_request_any_context_irq(dev, bdata->irq, isr, irqflags,
					     desc, bdata);
	if (error < 0) {
		dev_err(dev, "Unable to claim irq %d; error %d\n",
			bdata->irq, error);
		goto fail;
	}

	return 0;

fail:
	if (gpio_is_valid(button->gpio))
		gpio_free(button->gpio);

	return error;
}

static void gpio_keys_report_state(struct gpio_keys_drvdata *ddata)
{
	struct input_dev *input = ddata->input;
	int i;

	for (i = 0; i < ddata->pdata->nbuttons; i++) {
		struct gpio_button_data *bdata = &ddata->data[i];
		if (gpio_is_valid(bdata->button->gpio)) {
			gpio_keys_gpio_report_event(bdata);
		}
	}
	input_sync(input);
}

unsigned int flip_bypass;
module_param_named(flip_cover_bypass, flip_bypass, uint, 0644);

static void flip_cover_work(struct work_struct *work)
{
	struct gpio_keys_drvdata *ddata =
		container_of(work, struct gpio_keys_drvdata,
				flip_cover_dwork.work);

	if (flip_bypass)
		return;
	ddata->flip_cover = gpio_get_value(ddata->gpio_flip_cover);

	input_report_switch(ddata->input,
		SW_FLIP, ddata->flip_cover);
	input_sync(ddata->input);
}

static irqreturn_t flip_cover_detect(int irq, void *dev_id)
{
	struct gpio_keys_drvdata *ddata = dev_id;

	if (flip_bypass)
		return IRQ_HANDLED;

	if (gpio_to_irq(ddata->gpio_flip_cover))
		mod_delayed_work(flipqueue, &ddata->flip_cover_dwork, msecs_to_jiffies(4));
	return IRQ_HANDLED;
}

static int gpio_keys_open(struct input_dev *input)
{
	struct gpio_keys_drvdata *ddata = input_get_drvdata(input);
	const struct gpio_keys_platform_data *pdata = ddata->pdata;
	int error;
	int ret = 0;
	int irq = gpio_to_irq(ddata->gpio_flip_cover);

	if(ddata->gpio_flip_cover == 0 || (flip_bypass)) {
		printk(KERN_DEBUG"[HALL_IC] : %s skip flip\n", __func__);
		goto skip_flip;
	}

	INIT_DELAYED_WORK(&ddata->flip_cover_dwork, flip_cover_work);

	ret = request_threaded_irq(
			irq, NULL,
			flip_cover_detect,
			IRQF_DISABLED | IRQF_TRIGGER_RISING |
			IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
			"flip_cover", ddata);
		if (ret < 0)
			pr_debug("flip cover is fucked\n");

		ret = enable_irq_wake(irq);
		if (ret < 0)
			pr_debug("flip cover is fucked\n");

		/* update the current status */
		mod_delayed_work(flipqueue, &ddata->flip_cover_dwork, msecs_to_jiffies(4));

skip_flip:
	if (pdata->enable) {
		error = pdata->enable(input->dev.parent);
		if (error)
			return error;
	}

	/* Report current state of buttons that are connected to GPIOs */
	gpio_keys_report_state(ddata);

	return 0;
}

static void gpio_keys_close(struct input_dev *input)
{
	struct gpio_keys_drvdata *ddata = input_get_drvdata(input);
	const struct gpio_keys_platform_data *pdata = ddata->pdata;

	if (pdata->disable)
		pdata->disable(input->dev.parent);
}

static ssize_t hall_detect_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct gpio_keys_drvdata *ddata = dev_get_drvdata(dev);


	if (flip_bypass) {
		sprintf(buf, "OPEN");
		return strlen(buf);
	} else if (ddata->flip_cover)
		sprintf(buf, "OPEN");
	else
		sprintf(buf, "CLOSE");

	return strlen(buf);
}
static DEVICE_ATTR(hall_detect, 0664, hall_detect_show, NULL);

static ssize_t  sysfs_key_onoff_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct gpio_keys_drvdata *ddata = dev_get_drvdata(dev);
	struct gpio_button_data *button;
	int index ;
	int state = 0;
	for (index = 0; index < ddata->pdata->nbuttons; index++) {
		struct gpio_button_data *button;
		button = &ddata->data[index];
		state = (__gpio_get_value(button->button->gpio) ? 1 : 0)\
			^ button->button->active_low;
		if (state == 1)
			break;
	}
	pr_info("key state:%d\n",  state);
	return snprintf(buf, 5, "%d\n", state);
}
static DEVICE_ATTR(sec_key_pressed, 0444 , sysfs_key_onoff_show, NULL);

static ssize_t wakeup_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct gpio_keys_drvdata *ddata = dev_get_drvdata(dev);
	int index;
	int state, keycode;
	char *buff;
	char tmp[7] = {0};
	ssize_t count;
	int len = (ddata->pdata->nbuttons + 2) * 7 + 2;

	buff = kmalloc(len, GFP_KERNEL);
	if (!buff) {
		pr_debug("gpio-failed to mem alloc\n");
		return snprintf(buf, 5, "NG\n");
	}

	for (index = 0; index < ddata->pdata->nbuttons; index++) {
		struct gpio_button_data *button;

		button = &ddata->data[index];
		state = (__gpio_get_value(button->button->wakeup) ? 1 : 0);
		keycode = button->button->code;
		if (index == 0) {
			snprintf(buff, 7, "%d:%d", keycode, state);
		} else {
			snprintf(tmp, 7, ",%d:%d", keycode, state);
			strncat(buff, tmp, 7);
		}
	}

	count = snprintf(buf, strnlen(buff, len - 2) + 2, "%s\n", buff);

	kfree(buff);

	return count;
}

/* the volume keys can be the wakeup keys in special case */
static ssize_t wakeup_enable(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct gpio_keys_drvdata *ddata = dev_get_drvdata(dev);
	int n_events = get_n_events_by_type(EV_KEY);
	unsigned long *bits;
	ssize_t error;
	int i;

	bits = kcalloc(BITS_TO_LONGS(n_events),
			sizeof(*bits), GFP_KERNEL);
	if (!bits)
		return -ENOMEM;

	error = bitmap_parselist(buf, bits, n_events);
	if (error)
		goto out;

	for (i = 0; i < ddata->pdata->nbuttons; i++) {
		struct gpio_button_data *button = &ddata->data[i];
		if (button->button->type == EV_KEY) {
			if (test_bit(button->button->code, bits))
				button->button->wakeup = 1;
			else
				button->button->wakeup = 0;
			pr_info("%s wakeup status %d\n", button->button->desc,\
						button->button->wakeup);
		}
	}

out:
	kfree(bits);
	return count;
}
static DEVICE_ATTR(wakeup_keys, 0644, wakeup_enable_show, wakeup_enable);


static ssize_t keycode_pressed_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct gpio_keys_drvdata *ddata = dev_get_drvdata(dev);
	int index;
	int state, keycode;
	char *buff;
	char tmp[7] = {0};
	ssize_t count;
	int len = (ddata->pdata->nbuttons + 2) * 7 + 2;

	buff = kmalloc(len, GFP_KERNEL);
	if (!buff) {
		pr_debug("gpio-failed to mem alloc\n");
		return snprintf(buf, 5, "NG\n");
	}

	for (index = 0; index < ddata->pdata->nbuttons; index++) {
		struct gpio_button_data *button;

		button = &ddata->data[index];
		state = (__gpio_get_value(button->button->gpio) ? 1 : 0)
			^ button->button->active_low;
		keycode = button->button->code;
		if (index == 0) {
			snprintf(buff, 7, "%d:%d", keycode, state);
		} else {
			snprintf(tmp, 7, ",%d:%d", keycode, state);
			strncat(buff, tmp, 7);
		}
	}

	count = snprintf(buf, strnlen(buff, len - 2) + 2, "%s\n", buff);

	kfree(buff);

	return count;
}
static DEVICE_ATTR(keycode_pressed, 0444 , keycode_pressed_show, NULL);

static struct attribute *sec_key_attrs[] = {
	&dev_attr_sec_key_pressed.attr,
	&dev_attr_wakeup_keys.attr,
	&dev_attr_hall_detect.attr,
	&dev_attr_keycode_pressed.attr,
	NULL,
};
static struct attribute_group sec_key_attr_group = {
	.attrs = sec_key_attrs,
};
/*
 * Handlers for alternative sources of platform_data
 */

static void gpio_remove_key(struct gpio_button_data *bdata)
{
	free_irq(bdata->irq, bdata);
	if (bdata->release_delay)
		del_timer_sync(&bdata->release_timer);
	cancel_delayed_work_sync(&bdata->work);
	if (gpio_is_valid(bdata->button->gpio))
		gpio_free(bdata->button->gpio);
}

extern struct class *sec_class;
static int gpio_keys_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	const struct gpio_keys_platform_data *pdata = dev_get_platdata(dev);
	struct gpio_keys_drvdata *ddata;
	struct input_dev *input;
	size_t size;
	int i, error;
	int wakeup = 0;

	size = sizeof(struct gpio_keys_drvdata) +
			pdata->nbuttons * sizeof(struct gpio_button_data);
	ddata = devm_kzalloc(dev, size, GFP_KERNEL);
	if (!ddata) {
		dev_err(dev, "failed to allocate state\n");
		return -ENOMEM;
	}

	input = devm_input_allocate_device(dev);
	if (!input) {
		dev_err(dev, "failed to allocate input device\n");
		return -ENOMEM;
	}

	ddata->gpio_flip_cover = pdata->gpio_flip_cover;
	//ddata->irq_flip_cover = gpio_to_irq(ddata->gpio_flip_cover);
	//wake_lock_init(&ddata->flip_wake_lock, WAKE_LOCK_SUSPEND, "flip_wake_lock");
	if (!flip_bypass) {
		if(ddata->gpio_flip_cover != 0) {
			input->evbit[0] |= BIT_MASK(EV_SW);
			input_set_capability(input, EV_SW, SW_FLIP);
		}
	}

	flipqueue = alloc_workqueue("flipcov", WQ_UNBOUND | WQ_MEM_RECLAIM, 1);

	global_dev = dev;
	ddata->pdata = pdata;
	ddata->input = input;
	mutex_init(&ddata->disable_lock);

	platform_set_drvdata(pdev, ddata);
	input_set_drvdata(input, ddata);

	input->name = pdata->name ? : pdev->name;
	input->phys = "gpio-keys/input0";
	input->dev.parent = dev;
	input->evbit[0] |= BIT_MASK(EV_SW);
	input_set_capability(input, EV_SW, SW_FLIP);
	input->open = gpio_keys_open;
	input->close = gpio_keys_close;

	input->id.bustype = BUS_HOST;
	input->id.vendor = 0x0001;
	input->id.product = 0x0001;
	input->id.version = 0x0100;

	/* Enable auto repeat feature of Linux input subsystem */
	if (pdata->rep)
		__set_bit(EV_REP, input->evbit);

	for (i = 0; i < pdata->nbuttons; i++) {
		struct gpio_keys_button *button = &pdata->buttons[i];
		struct gpio_button_data *bdata = &ddata->data[i];

		error = gpio_keys_setup_key(pdev, input, bdata, button);
		if (error)
			goto fail2;

		if (button->wakeup)
			wakeup = 1;
	}

	error = sysfs_create_group(&dev->kobj, &gpio_keys_attr_group);
	if (error) {
		dev_err(dev, "Unable to export keys/switches, error: %d\n",
			error);
		goto fail2;
	}

	ddata->sec_key =
	    device_create(sec_class, NULL, 0, ddata, "sec_key");
	if (IS_ERR(ddata->sec_key))
		dev_err(dev, "Failed to create sec_key device\n");

	error = sysfs_create_group(&ddata->sec_key->kobj, &sec_key_attr_group);
	if (error) {
		dev_err(dev, "Failed to create the test sysfs: %d\n",
			error);
		goto fail2;
	}
	error = input_register_device(input);
	if (error) {
		dev_err(dev, "Unable to register input device, error: %d\n",
			error);
		goto fail3;
	}

	device_init_wakeup(dev, wakeup);

	return 0;

 fail3:
	sysfs_remove_group(&dev->kobj, &gpio_keys_attr_group);
 fail2:
	while (--i >= 0)
		gpio_remove_key(&ddata->data[i]);

	platform_set_drvdata(pdev, NULL);
 fail1:
	input_free_device(input);
	kfree(ddata);
	/* If we have no platform data, we allocated pdata dynamically. */
	if (!dev_get_platdata(dev))
		kfree(pdata);

	return error;
}

static int gpio_keys_remove(struct platform_device *pdev)
{
	struct gpio_keys_drvdata *ddata = platform_get_drvdata(pdev);
	struct input_dev *input = ddata->input;
	int i;

	sysfs_remove_group(&pdev->dev.kobj, &gpio_keys_attr_group);

	device_init_wakeup(&pdev->dev, 0);

	for (i = 0; i < ddata->pdata->nbuttons; i++)
		gpio_remove_key(&ddata->data[i]);

	input_unregister_device(input);

	destroy_workqueue(flipqueue);

	/* If we have no platform data, we allocated pdata dynamically. */
	if (!dev_get_platdata(&pdev->dev))
		kfree(ddata->pdata);

	kfree(ddata);

	return 0;
}

static int gpio_keys_suspend(struct device *dev)
{
	struct gpio_keys_drvdata *ddata = dev_get_drvdata(dev);
	struct input_dev *input = ddata->input;
	int i;

	if (device_may_wakeup(dev)) {
		for (i = 0; i < ddata->pdata->nbuttons; i++) {
			struct gpio_button_data *bdata = &ddata->data[i];
			if (bdata->button->wakeup)
				enable_irq_wake(bdata->irq);
			bdata->suspended = true;
		}
	} else {
		mutex_lock(&input->mutex);
		if (input->users)
			gpio_keys_close(input);
		mutex_unlock(&input->mutex);
	}
	if (!flip_bypass) {
		if (ddata->gpio_flip_cover != 0)
			enable_irq_wake(gpio_to_irq(ddata->gpio_flip_cover));
	}

	return 0;
}

static int gpio_keys_resume(struct device *dev)
{
	struct gpio_keys_drvdata *ddata = dev_get_drvdata(dev);
	struct input_dev *input = ddata->input;
	int error = 0;
	int i;

	if (device_may_wakeup(dev)) {
		for (i = 0; i < ddata->pdata->nbuttons; i++) {
			struct gpio_button_data *bdata = &ddata->data[i];
			if (bdata->button->wakeup)
				disable_irq_wake(bdata->irq);
			bdata->suspended = false;
		}
	} else {
		mutex_lock(&input->mutex);
		if (input->users)
			error = gpio_keys_open(input);
		mutex_unlock(&input->mutex);
	}

#if defined(CONFIG_SENSORS_HALL)
	if (!flip_bypass) {
		if (device_may_wakeup(global_dev) && ddata->gpio_flip_cover != 0)
			disable_irq_wake(gpio_to_irq(ddata->gpio_flip_cover));
	}
#endif

	if (error)
		return error;

	gpio_keys_report_state(ddata);
	return 0;
}

static SIMPLE_DEV_PM_OPS(gpio_keys_pm_ops, gpio_keys_suspend, gpio_keys_resume);

static struct platform_driver gpio_keys_device_driver = {
	.probe		= gpio_keys_probe,
	.remove		= gpio_keys_remove,
	.driver		= {
		.name	= "gpio-keys",
		.pm	= &gpio_keys_pm_ops,
	}
};

static int __init gpio_keys_init(void)
{
	return platform_driver_register(&gpio_keys_device_driver);
}

static void __exit gpio_keys_exit(void)
{
	platform_driver_unregister(&gpio_keys_device_driver);
}

late_initcall(gpio_keys_init);
module_exit(gpio_keys_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Phil Blundell <pb@handhelds.org>");
MODULE_DESCRIPTION("Keyboard driver for GPIOs");
MODULE_ALIAS("platform:gpio-keys");
