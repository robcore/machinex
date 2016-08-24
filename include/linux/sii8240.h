/*
 * Copyright (C) 2011 Samsung Electronics, Inc.
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

#ifndef _SII8240_H_
#define _SII8240_H_

#ifdef __KERNEL__
/* Different MHL Dongle/bridge which are used; may be
 * these enums are Samsung-Specific,but they do represent various
 * versions of dongles.
 */
enum dongle_type {
	DONGLE_NONE = 0,
	DONGLE_9290,
	DONGLE_9292,
};

enum connector_type {
	CONN_NONE = 0,
	CONN_5_PIN,     /* USB-type */
	CONN_11_PIN,    /* Galaxy-S3? */
	CONN_30_PIN,    /* Galaxy Tabs */
};

struct sii8240_platform_data {
/* Called to setup board-specific power operations */
	void (*power)(bool on);
	/* In case,when connectors are not able to automatically switch the
	* D+/D- Path to SII8240,do the switching manually.
	*/
	void (*enable_path)(bool enable);
	struct i2c_client *tmds_client;
	struct i2c_client *hdmi_client;
	struct i2c_client *disc_client;
	struct i2c_client *tpi_client;
	struct i2c_client *cbus_client;
	/* to handle board-specific connector info & callback */
	enum dongle_type dongle;
	enum connector_type conn;
	int (*reg_notifier)(struct notifier_block *nb);
	int (*unreg_notifier)(struct notifier_block *nb);
	u8 power_state;
	u32 swing_level;
	int ddc_i2c_num;
	void (*mhl_sel)(bool enable);
	int (*get_irq)(void);
	int gpio;
	void (*hw_reset)(void);
	void (*gpio_cfg)(int data);
	/* void (*vbus_present)(bool on, int mhl_charger); */
	void (*vbus_present)(bool on, int charger);
#ifdef CONFIG_SAMSUNG_MHL_UNPOWERED
	int (*get_vbus_status)(void);
	void (*sii9234_otg_control)(bool onoff);
#endif
};
#endif /* __SII8240_H__ */

#endif
