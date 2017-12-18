/*
 * i2c-mux-gpio interface to platform code
 *
 * Peter Korsgaard <peter.korsgaard@barco.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _LINUX_I2C_MUX_GPIO_H
#define _LINUX_I2C_MUX_GPIO_H

/* MUX has no specific idle mode */
#define I2C_MUX_GPIO_NO_IDLE	((unsigned)-1)

/**
 * struct i2c_mux_gpio_platform_data - Platform-dependent data for i2c-mux-gpio
 * @parent: Parent I2C bus adapter number
 * @base_nr: Base I2C bus number to number adapters from or zero for dynamic
 * @values: Array of bitmasks of GPIO settings (low/high) for each
 *	position
 * @n_values: Number of multiplexer positions (busses to instantiate)
 * @classes: Optional I2C auto-detection classes
 * @gpios: Array of GPIO numbers used to control MUX
 * @n_gpios: Number of GPIOs used to control MUX
 * @idle: Bitmask to write to MUX when idle or GPIO_I2CMUX_NO_IDLE if not used
 */
struct i2c_mux_gpio_platform_data {
	int parent;
	int base_nr;
	const unsigned *values;
	int n_values;
	const unsigned *classes;
	const unsigned *gpios;
	int n_gpios;
	unsigned idle;
};

#endif /* _LINUX_I2C_MUX_GPIO_H */
