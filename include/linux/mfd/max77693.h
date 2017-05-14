/*
 * max77693.h - Driver for the Maxim 77693
 *
 *  Copyright (C) 2011 Samsung Electrnoics
 *  SangYoung Son <hello.son@samsung.com>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * This driver is based on max8997.h
 *
 * MAX77693 has Charger, Flash LED, Haptic, MUIC devices.
 * The devices share the same I2C bus and included in
 * this mfd driver.
 */

#ifndef __LINUX_MFD_MAX77693_H
#define __LINUX_MFD_MAX77693_H

#include <linux/regulator/consumer.h>
#if defined (CONFIG_SEC_PRODUCT_8930)
#include <linux/battery/sec_charger_8930.h>
#include <linux/battery/charger/max77693_charger.h>
#else
#include <linux/battery/sec_charger.h>
#endif

enum {
	MAX77693_MUIC_DETACHED = 0,
	MAX77693_MUIC_ATTACHED
};

/* MAX77686 regulator IDs */
enum max77693_regulators {
	MAX77693_ESAFEOUT1 = 0,
	MAX77693_ESAFEOUT2,
	MAX77693_CHARGER,
	MAX77693_REG_MAX,
};

struct max77693_charger_reg_data {
	u8 addr;
	u8 data;
};

struct max77693_charger_platform_data {
	struct max77693_charger_reg_data *init_data;
	int num_init_data;
	sec_battery_platform_data_t *sec_battery;
#if defined(CONFIG_WIRELESS_CHARGING) || defined(CONFIG_CHARGER_MAX77803)
	int wpc_irq_gpio;
	int vbus_irq_gpio;
	bool wc_pwr_det;
#endif
};

#ifdef CONFIG_VIBETONZ
#define MAX8997_MOTOR_REG_CONFIG2	0x2
#define MOTOR_LRA			(1<<7)
#define MOTOR_EN			(1<<6)
#define EXT_PWM				(0<<5)
#define DIVIDER_128			(1<<1)
#define DIVIDER_256			0x3

struct max77693_haptic_platform_data {
	u16 max_timeout;
	u16 duty;
	u16 period;
	u16 reg2;
	char *regulator_name;
	unsigned int pwm_id;

	void (*init_hw) (void);
	void (*motor_en) (bool);
};
#endif

#ifdef CONFIG_LEDS_MAX77693
struct max77693_led_platform_data;
#endif

struct max77693_regulator_data {
	int id;
	struct regulator_init_data *initdata;
};

struct max77693_platform_data {
	/* IRQ */
	int irq_base;
	int irq_gpio;
	int wc_irq_gpio;
	int wakeup;
	struct max77693_muic_data *muic;
	struct max77693_regulator_data *regulators;
	int num_regulators;
#ifdef CONFIG_VIBETONZ
	/* haptic motor data */
	struct max77693_haptic_platform_data *haptic_data;
#endif
#ifdef CONFIG_LEDS_MAX77693
	/* led (flash/torch) data */
	struct max77693_led_platform_data *led_data;
#endif
#if defined(CONFIG_CHARGER_MAX77XXX) || defined(CONFIG_CHARGER_MAX77693) || defined(CONFIG_MACH_MELIUS)
	sec_battery_platform_data_t *charger_data;
#endif
};

enum cable_type_muic;
struct max77693_muic_data {
	void (*usb_cb) (u8 attached);
	void (*uart_cb) (u8 attached);
	int (*charger_cb) (enum cable_type_muic);
	void (*deskdock_cb) (bool attached);
	void (*cardock_cb) (bool attached);
#if defined(CONFIG_MACH_MELIUS)
	void (*smartdock_cb) (bool attached, u8 cable_type);
#else
	void (*smartdock_cb) (bool attached);
#endif
	void (*audiodock_cb) (bool attached);
	void (*mhl_cb) (int attached);
	void (*init_cb) (void);
	int (*set_safeout) (int path);
	 bool(*is_mhl_attached) (void);
	int (*cfg_uart_gpio) (void);
	void (*jig_uart_cb) (int path);
#if defined(CONFIG_MUIC_DET_JACK)
	void (*earjack_cb) (int attached);
	void (*earjackkey_cb) (int pressed, unsigned int code);
#endif
	int (*host_notify_cb) (int enable);
	int gpio_usb_sel;
	int sw_path;
	int uart_path;
#ifdef CONFIG_VIDEO_MHL_V2
	struct wake_lock mhl_wake_lock;
#endif
	void (*jig_state) (int jig_state);

};

extern unsigned int machinex_charging_check;

#ifdef CONFIG_MFD_MAX77693
extern struct max77693_muic_data max77693_muic;
extern struct max77693_regulator_data max77693_regulators[];
#else
#ifdef CONFIG_REGULATOR_NEW_MAX77693
struct max77693_regulator_data {
	int id;
	struct regulator_init_data *initdata;
	struct device_node *of_node;
};
#endif
#endif
#ifdef CONFIG_VIDEO_MHL_V2
int acc_register_notifier(struct notifier_block *nb);
#endif
#endif				/* __LINUX_MFD_MAX77693_H */
