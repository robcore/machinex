/*
 * OF helpers for the GPIO API
 *
 * Copyright (c) 2007-2008  MontaVista Software, Inc.
 *
 * Author: Anton Vorontsov <avorontsov@ru.mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/device.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/slab.h>

/* Private data structure for of_gpiochip_find_and_xlate */
struct gg_data {
	enum of_gpio_flags *flags;
	struct of_phandle_args gpiospec;

	int out_gpio;
};

/* Private function for resolving node pointer to gpio_chip */
static int of_gpiochip_find_and_xlate(struct gpio_chip *gc, void *data)
{
	struct gg_data *gg_data = data;
	int ret;

	if ((gc->of_node != gg_data->gpiospec.np) ||
	    (gc->of_gpio_n_cells != gg_data->gpiospec.args_count) ||
	    (!gc->of_xlate))
		return false;

	ret = gc->of_xlate(gc, &gg_data->gpiospec, gg_data->flags);
	if (ret < 0) {
		/* We've found the gpio chip, but the translation failed.
		 * Return true to stop looking and return the translation
		 * error via out_gpio
		 */
		gg_data->out_gpio = ERR_PTR(ret);
		return true;
	 }

	gg_data->out_gpio = ret + gc->base;
	return true;
}

/**
 * of_get_named_gpio_flags() - Get a GPIO number and flags to use with GPIO API
 * @np:		device node to get GPIO from
 * @propname:	property name containing gpio specifier(s)
 * @index:	index of the GPIO
 * @flags:	a flags pointer to fill in
 *
 * Returns GPIO number to use with Linux generic GPIO API, or one of the errno
 * value on the error condition. If @flags is not NULL the function also fills
 * in flags for the GPIO.
 */
int of_get_named_gpio_flags(struct device_node *np, const char *propname,
			   int index, enum of_gpio_flags *flags)
{
	int ret;
	struct gpio_chip *gc;
	struct of_phandle_args gpiospec;

	ret = of_parse_phandle_with_args(np, propname, "#gpio-cells", index,
					 &gpiospec);
	if (ret) {
		pr_debug("%s: can't parse gpios property\n", __func__);
		goto err0;
	}

	gc = of_node_to_gpiochip(gpiospec.np);
	if (!gc) {
		pr_debug("%s: gpio controller %s isn't registered\n",
			 np->full_name, gpiospec.np->full_name);
		ret = -EPROBE_DEFER;
		goto err1;
	}

	if (gpiospec.args_count != gc->of_gpio_n_cells) {
		pr_debug("%s: wrong #gpio-cells for %s\n",
			 np->full_name, gpiospec.np->full_name);
		ret = -EINVAL;
		goto err1;
	}

	/* .xlate might decide to not fill in the flags, so clear it. */
	if (flags)
		*flags = 0;

	ret = gc->of_xlate(gc, &gpiospec, flags);
	if (ret < 0)
		goto err1;

	ret += gc->base;
err1:
	of_node_put(gpiospec.np);
err0:
	pr_debug("%s exited with status %d\n", __func__, ret);
	return ret;
}
EXPORT_SYMBOL(of_get_named_gpio_flags);

/**
 * of_gpio_named_count - Count GPIOs for a device
 * @np:		device node to count GPIOs for
 * @propname:	property name containing gpio specifier(s)
 *
 * The function returns the count of GPIOs specified for a node.
 *
 * Note that the empty GPIO specifiers counts too. For example,
 *
 * gpios = <0
 *          &pio1 1 2
 *          0
 *          &pio2 3 4>;
 *
 * defines four GPIOs (so this function will return 4), two of which
 * are not specified.
 */
unsigned int of_gpio_named_count(struct device_node *np, const char* propname)
{
	unsigned int cnt = 0;

	do {
		int ret;

		ret = of_parse_phandle_with_args(np, propname, "#gpio-cells",
						 cnt, NULL);
		/* A hole in the gpios = <> counts anyway. */
		if (ret < 0 && ret != -EEXIST)
			break;
	} while (++cnt);

	return cnt;
}
EXPORT_SYMBOL(of_gpio_named_count);

/**
 * of_gpio_simple_xlate - translate gpio_spec to the GPIO number and flags
 * @gc:		pointer to the gpio_chip structure
 * @np:		device node of the GPIO chip
 * @gpio_spec:	gpio specifier as found in the device tree
 * @flags:	a flags pointer to fill in
 *
 * This is simple translation function, suitable for the most 1:1 mapped
 * gpio chips. This function performs only one sanity check: whether gpio
 * is less than ngpios (that is specified in the gpio_chip).
 */
int of_gpio_simple_xlate(struct gpio_chip *gc,
			 const struct of_phandle_args *gpiospec, u32 *flags)
{
	/*
	 * We're discouraging gpio_cells < 2, since that way you'll have to
	 * write your own xlate function (that will have to retrive the GPIO
	 * number and the flags from a single gpio cell -- this is possible,
	 * but not recommended).
	 */
	if (gc->of_gpio_n_cells < 2) {
		WARN_ON(1);
		return -EINVAL;
	}

	if (WARN_ON(gpiospec->args_count < gc->of_gpio_n_cells))
		return -EINVAL;

	if (gpiospec->args[0] >= gc->ngpio)
		return -EINVAL;

	if (flags)
		*flags = gpiospec->args[1];

	return gpiospec->args[0];
}
EXPORT_SYMBOL(of_gpio_simple_xlate);

/**
 * of_mm_gpiochip_add - Add memory mapped GPIO chip (bank)
 * @np:		device node of the GPIO chip
 * @mm_gc:	pointer to the of_mm_gpio_chip allocated structure
 *
 * To use this function you should allocate and fill mm_gc with:
 *
 * 1) In the gpio_chip structure:
 *    - all the callbacks
 *    - of_gpio_n_cells
 *    - of_xlate callback (optional)
 *
 * 3) In the of_mm_gpio_chip structure:
 *    - save_regs callback (optional)
 *
 * If succeeded, this function will map bank's memory and will
 * do all necessary work for you. Then you'll able to use .regs
 * to manage GPIOs from the callbacks.
 */
int of_mm_gpiochip_add(struct device_node *np,
		       struct of_mm_gpio_chip *mm_gc)
{
	int ret = -ENOMEM;
	struct gpio_chip *gc = &mm_gc->gc;

	gc->label = kstrdup(np->full_name, GFP_KERNEL);
	if (!gc->label)
		goto err0;

	mm_gc->regs = of_iomap(np, 0);
	if (!mm_gc->regs)
		goto err1;

	gc->base = -1;

	if (mm_gc->save_regs)
		mm_gc->save_regs(mm_gc);

	mm_gc->gc.of_node = np;

	ret = gpiochip_add(gc);
	if (ret)
		goto err2;

	return 0;
err2:
	iounmap(mm_gc->regs);
err1:
	kfree(gc->label);
err0:
	pr_err("%s: GPIO chip registration failed with status %d\n",
	       np->full_name, ret);
	return ret;
}
EXPORT_SYMBOL(of_mm_gpiochip_add);

/**
 * of_mm_gpiochip_remove - Remove memory mapped GPIO chip (bank)
 * @mm_gc:	pointer to the of_mm_gpio_chip allocated structure
 */
void of_mm_gpiochip_remove(struct of_mm_gpio_chip *mm_gc)
{
	struct gpio_chip *gc = &mm_gc->gc;

	if (!mm_gc)
		return;

	gpiochip_remove(gc);
	iounmap(mm_gc->regs);
	kfree(gc->label);
}
EXPORT_SYMBOL(of_mm_gpiochip_remove);

#ifdef CONFIG_PINCTRL
static void of_gpiochip_add_pin_range(struct gpio_chip *chip)
{
	struct device_node *np = chip->of_node;
	struct of_phandle_args pinspec;
	struct pinctrl_dev *pctldev;
	int index = 0, ret;

	if (!np)
		return;

	for (;; index++) {
		ret = of_parse_phandle_with_fixed_args(np, "gpio-ranges", 3,
				index, &pinspec);
		if (ret)
			break;

		pctldev = of_pinctrl_get(pinspec.np);
		if (!pctldev)
			break;

		ret = gpiochip_add_pin_range(chip,
					     pinctrl_dev_get_devname(pctldev),
					     pinspec.args[0],
					     pinspec.args[1],
					     pinspec.args[2]);

		if (ret)
			break;
	}
}

#else
static void of_gpiochip_add_pin_range(struct gpio_chip *chip) {}
#endif

void of_gpiochip_add(struct gpio_chip *chip)
{
	if ((!chip->of_node) && (chip->dev))
		chip->of_node = chip->dev->of_node;

	if (!chip->of_node)
		return;

	if (!chip->of_xlate) {
		chip->of_gpio_n_cells = 2;
		chip->of_xlate = of_gpio_simple_xlate;
	}

	of_gpiochip_add_pin_range(chip);
	of_node_get(chip->of_node);
}

void of_gpiochip_remove(struct gpio_chip *chip)
{
	gpiochip_remove_pin_ranges(chip);
	of_node_put(chip->of_node);
}
