/*
 * leds_an30259a.c - driver for panasonic AN30259A led control chip
 *
 * Copyright (C) 2012, Samsung Electronics Co. Ltd. All Rights Reserved.
 *
 * Contact: Kamaldeep Singla <kamal.singla@samsung.com>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/leds.h>
#include <linux/leds-an30259a.h>
#include <linux/workqueue.h>
#include <linux/wakelock.h>
#include <linux/sec_battery.h>
#include <linux/sysfs.h>
#include "leds-an30259a_reg.h"
#include <linux/i2c/synaptics_rmi.h>
#include <linux/display_state.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))

u8 led_dynamic_current = 0x28;
unsigned int led_lowpower_mode = 0;
unsigned int real_led_lowpower_mode = 0;
struct wake_lock ledlock;

static struct an30259_led_conf led_conf[] = {
	{
		.name = "led_r",
		.brightness = LED_OFF,
		.max_brightness = LED_MAX_CURRENT,
		.flags = 0,
	}, {
		.name = "led_g",
		.brightness = LED_OFF,
		.max_brightness = LED_MAX_CURRENT,
		.flags = 0,
	}, {
		.name = "led_b",
		.brightness = LED_OFF,
		.max_brightness = LED_MAX_CURRENT,
		.flags = 0,
	}
};

enum an30259a_led_enum {
	LED_R = 0,
	LED_G = 1,
	LED_B = 2,
};

enum an30259a_pattern {
	PATTERN_OFF = 0,
	CHARGING,
	CHARGING_ERR,
	MISSED_NOTI,
	LOW_BATTERY,
	FULLY_CHARGED,
	POWERING,
	FAKE_POWERING,
	BOOTING,
	CUSTOM,
};

struct an30259a_led {
	u8	channel;
	u8	brightness;
	struct led_classdev	cdev;
	struct work_struct	brightness_work;
	unsigned long delay_on_time_ms;
	unsigned long delay_off_time_ms;
};

struct an30259a_data {
	struct	i2c_client	*client;
	struct	mutex	mutex;
	struct	an30259a_led	leds[MAX_NUM_LEDS];
	u8		shadow_reg[AN30259A_REG_MAX];
};

struct i2c_client *b_client;

#define SEC_LED_SPECIFIC
#ifdef SEC_LED_SPECIFIC
struct device *led_dev;
/*path : /sys/class/sec/led/led_pattern*/
/*path : /sys/class/leds/led_r/brightness*/
/*path : /sys/class/leds/led_g/brightness*/
/*path : /sys/class/leds/led_b/brightness*/
#endif

enum {
	BATTERY_LOW = 0,
	BATTERY_MIDLOW = 1,
	BATTERY_MID = 2,
	BATTERY_MIDHIGH = 3,
	BATTERY_HIGH = 4,
	BATTERY_FULL = 5,
};
static int battery_level;
static bool booted = false;
static bool pattern_active;
static unsigned int current_led_mode;
static bool is_full_charge;

static void leds_on(enum an30259a_led_enum led, bool on, bool slopemode,
					u8 ledcc);

static inline struct an30259a_led *cdev_to_led(struct led_classdev *cdev)
{
	return container_of(cdev, struct an30259a_led, cdev);
}

static void leds_i2c_write_all(struct i2c_client *client)
{
	struct an30259a_data *data = i2c_get_clientdata(client);
	int ret;

	/*we need to set all the configs setting first, then LEDON later*/
	mutex_lock(&data->mutex);
	ret = i2c_smbus_write_i2c_block_data(client,
			AN30259A_REG_SEL | AN30259A_CTN_RW_FLG,
			AN30259A_REG_MAX - AN30259A_REG_SEL,
			&data->shadow_reg[AN30259A_REG_SEL]);
	if (ret < 0) {
		dev_err(&client->adapter->dev,
			"%s: failure on i2c block write\n",
			__func__);
		goto exit;
	}
	ret = i2c_smbus_write_byte_data(client, AN30259A_REG_LEDON,
					data->shadow_reg[AN30259A_REG_LEDON]);
	if (ret < 0) {
		dev_err(&client->adapter->dev,
			"%s: failure on i2c byte write\n",
			__func__);
		goto exit;
	}
exit:
	mutex_unlock(&data->mutex);
	return;
}

void an30259a_set_brightness(struct led_classdev *cdev,
			enum led_brightness brightness)
{
		struct an30259a_led *led = cdev_to_led(cdev);
		led->brightness = (u8)brightness;
		schedule_work(&led->brightness_work);
}

static void an30259a_led_brightness_work(struct work_struct *work)
{
		struct i2c_client *client = b_client;
		struct an30259a_led *led = container_of(work,
				struct an30259a_led, brightness_work);
		leds_on(led->channel, true, false, led->brightness);
		leds_i2c_write_all(client);
}
static void __inline an30259a_reset(struct i2c_client *client);
/**
* an30259a_set_slope_current - To Set the LED intensity and enable them
**/
static unsigned int breathing_leds = 0;
static unsigned int inhale = 1000;
static unsigned int exhale = 1000;
static void an30259a_set_slope_current(u16 ontime, u16 offtime, bool reset)
{
	struct i2c_client *client;

	struct an30259a_data *data;
	
	u8 delay, dutymax, dutymid, dutymin, slptt1, slptt2, 
			dt1, dt2, dt3, dt4;

	client = b_client;
	if (client == NULL)
		return;

	data = i2c_get_clientdata(client);
	if (data == NULL)
		return;

	if (reset) {
		an30259a_reset(client);
		return;
	}

	if (!breathing_leds)
		return;

	delay = 0;
	
	dutymid = dutymax = 0xF;
	dt2 = dt3 = 0;

	/* Keep duty min always zero, as in blink has to be off for a while */
	dutymin = 0;

	slptt1 = ontime / 500;
	if (!slptt1)
		slptt1 = 1;

	slptt2 = offtime / 500;
	if (!slptt2)
		slptt2 = 1;

	if(slptt1 > 1) {
		slptt2 += (slptt1 >> 1);
		
		// Check if odd number, store ceiling value to slptt1
		// in blink, for breathing effect ontime/2 is used for rasing and falling
		slptt1 = (slptt1 & 1) ? (slptt1 >> 1) + 1 : (slptt1 >> 1);
		
		dt4 = slptt1;
	} else
		dt4 = 0;

	dt1 = slptt1;
	
	data->shadow_reg[AN30259A_REG_LED1CNT1] = 
		data->shadow_reg[AN30259A_REG_LED2CNT1] = 
		data->shadow_reg[AN30259A_REG_LED3CNT1] = dutymax << 4 | dutymid;

	data->shadow_reg[AN30259A_REG_LED1CNT2] =  
		data->shadow_reg[AN30259A_REG_LED2CNT2] = 
		data->shadow_reg[AN30259A_REG_LED3CNT2] = delay << 4 | dutymin;
	
	data->shadow_reg[AN30259A_REG_LED1CNT3] = 
		data->shadow_reg[AN30259A_REG_LED2CNT3] = 	
		data->shadow_reg[AN30259A_REG_LED3CNT3] = dt2 << 4 | dt1;
	
	data->shadow_reg[AN30259A_REG_LED1CNT4] = 
		data->shadow_reg[AN30259A_REG_LED2CNT4] = 
		data->shadow_reg[AN30259A_REG_LED3CNT4] = dt4 << 4 | dt3;
	
	data->shadow_reg[AN30259A_REG_LED1SLP] = 
		data->shadow_reg[AN30259A_REG_LED2SLP] =  
		data->shadow_reg[AN30259A_REG_LED3SLP] = slptt2 << 4 | slptt1;

//#if 1	// TSN: Check.... If current will not be zero, enable only current particular LED	
//	data->shadow_reg[AN30259A_REG_LEDON] |= ALL_LED_SLOPE_MODE;
	
	/* Enable each color LED, if non-zero 
	if(data->shadow_reg[AN30259A_REG_LED_R])
		data->shadow_reg[AN30259A_REG_LEDON] |= LED_ON << LED_R;
	else
		data->shadow_reg[AN30259A_REG_LEDON] &= ~(LED_ON << LED_R);
	
	if(data->shadow_reg[AN30259A_REG_LED_G])
		data->shadow_reg[AN30259A_REG_LEDON] |= LED_ON << LED_G;
	else
		data->shadow_reg[AN30259A_REG_LEDON] &= ~(LED_ON << LED_G);
	
	if(data->shadow_reg[AN30259A_REG_LED_B])
		data->shadow_reg[AN30259A_REG_LEDON] |= LED_ON << LED_B;
	else
		data->shadow_reg[AN30259A_REG_LEDON] &= ~(LED_ON << LED_B);
*/	
//#else // TSN: Check.... If current will be really zero, then, can write Enable ALL
	//data->shadow_reg[AN30259A_REG_LEDON] |= ALL_LED_SLOPE_MODE | ALL_LED_ON;
//#endif

	data->shadow_reg[AN30259A_REG_LEDON] |= LED_ON << LED_R;
	data->shadow_reg[AN30259A_REG_LEDON] |= LED_ON << LED_G;
	data->shadow_reg[AN30259A_REG_LEDON] |= LED_ON << LED_B;
	leds_i2c_write_all(client);
}

/*
 * leds_set_slope_mode() sets correct values to corresponding shadow registers.
 * led: stands for LED_R or LED_G or LED_B.
 * delay: represents for starting delay time in multiple of .5 second.
 * dutymax: led at slope lighting maximum PWM Duty setting.
 * dutymid: led at slope lighting middle PWM Duty setting.
 * dutymin: led at slope lighting minimum PWM Duty Setting.
 * slptt1: total time of slope operation 1 and 2, in multiple of .5 second.
 * slptt2: total time of slope operation 3 and 4, in multiple of .5 second.
 * dt1: detention time at each step in slope operation 1, in multiple of 4ms.
 * dt2: detention time at each step in slope operation 2, in multiple of 4ms.
 * dt3: detention time at each step in slope operation 3, in multiple of 4ms.
 * dt4: detention time at each step in slope operation 4, in multiple of 4ms.
 */
static void leds_set_slope_mode(struct i2c_client *client,
				enum an30259a_led_enum led, u8 delay,
				u8 dutymax, u8 dutymid, u8 dutymin,
				u8 slptt1, u8 slptt2,
				u8 dt1, u8 dt2, u8 dt3, u8 dt4)
{

	struct an30259a_data *data = i2c_get_clientdata(client);

	if ((delay * AN30259A_TIME_UNIT) > SLPTT_MAX_VALUE)
		delay = SLPTT_MAX_VALUE/AN30259A_TIME_UNIT;

	data->shadow_reg[AN30259A_REG_LED1CNT1 + led * 4] =
							dutymax << 4 | dutymid;
	data->shadow_reg[AN30259A_REG_LED1CNT2 + led * 4] =
							delay << 4 | dutymin;
	data->shadow_reg[AN30259A_REG_LED1CNT3 + led * 4] = dt2 << 4 | dt1;
	data->shadow_reg[AN30259A_REG_LED1CNT4 + led * 4] = dt4 << 4 | dt3;
	data->shadow_reg[AN30259A_REG_LED1SLP + led] = slptt2 << 4 | slptt1;
}

static void leds_on(enum an30259a_led_enum led, bool on, bool slopemode,
			u8 ledcc)
{
	struct an30259a_data *data = i2c_get_clientdata(b_client);

	if (on)
		data->shadow_reg[AN30259A_REG_LEDON] |= LED_ON << led;
	else {
		data->shadow_reg[AN30259A_REG_LEDON] &= ~(LED_ON << led);
		data->shadow_reg[AN30259A_REG_LED1CNT2 + led * 4] &=
							~AN30259A_MASK_DELAY;
	}
	if (slopemode)
		data->shadow_reg[AN30259A_REG_LEDON] |= LED_SLOPE_MODE << led;
	else
		data->shadow_reg[AN30259A_REG_LEDON] &=
						~(LED_SLOPE_MODE << led);

	data->shadow_reg[AN30259A_REG_LED1CC + led] = ledcc;
}
static u8 imax_copy;
static void leds_set_imax(struct i2c_client *client, u8 imax)
{
	int ret;
	struct an30259a_data *data = i2c_get_clientdata(client);

	data->shadow_reg[AN30259A_REG_SEL] &= ~AN30259A_MASK_IMAX;
	data->shadow_reg[AN30259A_REG_SEL] |= imax << LED_IMAX_SHIFT;

	ret = i2c_smbus_write_byte_data(client, AN30259A_REG_SEL,
			data->shadow_reg[AN30259A_REG_SEL]);
	if (ret < 0) {
		dev_err(&client->adapter->dev,
			"%s: failure on i2c write\n",
			__func__);
	}
	return;
}

static void __inline an30259a_reset(struct i2c_client *client)
{	
	struct an30259a_data *data = i2c_get_clientdata(client);
	int ret;

	/*we need to set all the configs setting first, then LEDON later*/
	mutex_lock(&data->mutex);
	/* Reset the IC */
	if(unlikely((ret = i2c_smbus_write_byte_data(client, 
			AN30259A_REG_SRESET, AN30259A_SRESET)) < 0)) {
		dev_err(&client->adapter->dev,
			"%s: failure on i2c reset\n",
			__func__);
	}
			
	/* Make a copy of IC register values to the driver */	
	if(unlikely((ret = i2c_smbus_read_i2c_block_data(client,
			AN30259A_REG_SRESET | AN30259A_CTN_RW_FLG,
			AN30259A_REG_MAX, data->shadow_reg)) < 0)) {
		dev_err(&client->adapter->dev,
			"%s: failure on i2c read block(ledxcc)\n",
			__func__);
	}
	mutex_unlock(&data->mutex);	
	/* Set imax */
	leds_set_imax(client, imax_copy);

	return;
}

#ifdef SEC_LED_SPECIFIC
static void an30259a_reset_register_work(struct work_struct *work)
{
	struct i2c_client *client;
	client = b_client;

	leds_on(LED_R, false, false, 0x0);
	leds_on(LED_G, false, false, 0x0);
	leds_on(LED_B, false, false, 0x0);

	leds_i2c_write_all(client);
}

static void an30259a_set_led_blink(enum an30259a_led_enum led,
					unsigned int delay_on_time,
					unsigned int delay_off_time,
					u8 brightness)
{
	struct i2c_client *client;
	client = b_client;

	if (brightness == LED_OFF) {
		leds_on(led, false, false, brightness);
		return;
	}

	if (brightness > LED_MAX_CURRENT)
		brightness = LED_MAX_CURRENT;

	if (led == LED_R || led == LED_G || led == LED_B)
		led_dynamic_current = LED_DEFAULT_CURRENT;

	brightness = (brightness * led_dynamic_current) / LED_MAX_CURRENT;

	if (delay_on_time > SLPTT_MAX_VALUE)
		delay_on_time = SLPTT_MAX_VALUE;

	if (delay_off_time > SLPTT_MAX_VALUE)
		delay_off_time = SLPTT_MAX_VALUE;

	if (delay_off_time == LED_OFF) {
		leds_on(led, true, false, brightness);
		return;
	} else
		leds_on(led, true, true, brightness);

	leds_set_slope_mode(client, led, 0, 15, 15, 0,
				(delay_on_time + AN30259A_TIME_UNIT - 1) /
				AN30259A_TIME_UNIT,
				(delay_off_time + AN30259A_TIME_UNIT - 1) /
				AN30259A_TIME_UNIT,
				0, 0, 0, 0);
}

static void an30259a_set_led_delayed_blink(enum an30259a_led_enum led, unsigned int initial_delay,
					unsigned int delay_on_time,
					unsigned int delay_off_time,
					u8 brightness, bool force_brightness)
{
	struct i2c_client *client;
	client = b_client;

	if (brightness == LED_OFF) {
		leds_on(led, false, false, 0);
		return;
	}

	if (brightness > LED_MAX_CURRENT)
		brightness = LED_MAX_CURRENT;

	if (led == LED_R || led == LED_G || led == LED_B) {
		if (!force_brightness)
			led_dynamic_current = LED_DEFAULT_CURRENT;
		else {
			led_dynamic_current = brightness;
		}
	}

	brightness = (brightness * led_dynamic_current) / LED_MAX_CURRENT;

	if (delay_on_time > SLPTT_MAX_VALUE)
		delay_on_time = SLPTT_MAX_VALUE;

	if (delay_off_time > SLPTT_MAX_VALUE)
		delay_off_time = SLPTT_MAX_VALUE;

	if (delay_off_time == LED_OFF) {
		leds_on(led, true, false, brightness);
		return;
	} else
		leds_on(led, true, true, brightness);

	leds_set_slope_mode(client, led, (initial_delay + AN30259A_TIME_UNIT - 1) /
				AN30259A_TIME_UNIT, 15, 15, 0,
				(delay_on_time + AN30259A_TIME_UNIT - 1) /
				AN30259A_TIME_UNIT,
				(delay_off_time + AN30259A_TIME_UNIT - 1) /
				AN30259A_TIME_UNIT,
				0, 0, 0, 0);

}

static unsigned int custom_r_enabled = 0;
static unsigned int custom_r_delay = 0;
static unsigned int custom_r_dutymax = 15;
static unsigned int custom_r_dutymid = 0;
static unsigned int custom_r_dutymin = 0;
static unsigned int custom_r_total1 = 1;
static unsigned int custom_r_total2 = 1;
static unsigned int custom_r_dt1 = 0;
static unsigned int custom_r_dt2 = 0;
static unsigned int custom_r_dt3 = 0;
static unsigned int custom_r_dt4 = 0;

static unsigned int custom_g_enabled = 0;
static unsigned int custom_g_delay = 0;
static unsigned int custom_g_dutymax = 15;
static unsigned int custom_g_dutymid = 0;
static unsigned int custom_g_dutymin = 0;
static unsigned int custom_g_total1 = 1;
static unsigned int custom_g_total2 = 1;
static unsigned int custom_g_dt1 = 0;
static unsigned int custom_g_dt2 = 0;
static unsigned int custom_g_dt3 = 0;
static unsigned int custom_g_dt4 = 0;

static unsigned int custom_b_enabled = 0;
static unsigned int custom_b_delay = 0;
static unsigned int custom_b_dutymax = 15;
static unsigned int custom_b_dutymid = 0;
static unsigned int custom_b_dutymin = 0;
static unsigned int custom_b_total1 = 1;
static unsigned int custom_b_total2 = 1;
static unsigned int custom_b_dt1 = 0;
static unsigned int custom_b_dt2 = 0;
static unsigned int custom_b_dt3 = 0;
static unsigned int custom_b_dt4 = 0;

static void an30259a_start_led_pattern(unsigned int mode)
{
	int curr_level, retval;
	struct i2c_client *client;
	struct work_struct *reset = 0;
	client = b_client;

	if (is_full_charge && mode == CHARGING)
		mode = FULLY_CHARGED;

	current_led_mode = mode;

	/* Set all LEDs Off */
	an30259a_reset_register_work(reset);
	if (breathing_leds && booted)
		an30259a_reset(client);

	if (mode > CUSTOM ||
		mode <= PATTERN_OFF) {
		pattern_active = false;
		return;
	}

	pattern_active = true;
	/* Set to low power consumption mode */
	if (real_led_lowpower_mode == 1)
		led_dynamic_current = 0x9;
	else
		led_dynamic_current = 0x48;

	if (breathing_leds && booted)
		an30259a_set_slope_current(inhale, exhale, false);

	curr_level = battery_level;

 	switch (mode) {
 	/* leds_set_slope_mode(client, LED_SEL, DELAY,  MAX, MID, MIN,
 		SLPTT1, SLPTT2, DT1, DT2, DT3, DT4) */
	case CHARGING:
		pr_info("LED Battery Charging Pattern on\n");
		switch (curr_level) {
		case BATTERY_LOW:
				if (breathing_leds) {
					leds_on(LED_R, true, true, 0xFF);
					leds_set_slope_mode(client, LED_R,
							0, 15, 10, 0, 2, 2, 1, 1, 1, 1);
				} else
					leds_on(LED_R, true, false, 0xFF);
				break;
		case BATTERY_MIDLOW:
				if (breathing_leds) {
					leds_on(LED_R, true, true, 0xEA);
					leds_on(LED_G, true, true, 0xBE);
					leds_set_slope_mode(client, LED_R,
							0, 15, 10, 0, 2, 2, 1, 1, 1, 1);
					leds_set_slope_mode(client, LED_G,
							0, 15, 10, 0, 2, 2, 1, 1, 1, 1);
				} else {
					leds_on(LED_R, true, false, 0xEA);
					leds_on(LED_G, true, false, 0xBE);
				}
				break;
		case BATTERY_MID:
				if (breathing_leds) {
					leds_on(LED_R, true, true, 0x91);
					leds_on(LED_G, true, true, 0xBF);
					leds_set_slope_mode(client, LED_R,
							0, 15, 10, 0, 2, 2, 1, 1, 1, 1);
					leds_set_slope_mode(client, LED_G,
							0, 15, 10, 0, 2, 2, 1, 1, 1, 1);
				} else {
					leds_on(LED_R, true, false, 0x91);
					leds_on(LED_G, true, false, 0xBF);
				}
				break;
		case BATTERY_MIDHIGH:
				if (breathing_leds) {
					leds_on(LED_R, true, true, 0x70);
					leds_on(LED_G, true, true, 0xD1);
					leds_on(LED_B, true, true, 0x0E);
					leds_set_slope_mode(client, LED_R,
							0, 15, 10, 0, 2, 2, 1, 1, 1, 1);
					leds_set_slope_mode(client, LED_G,
							0, 15, 10, 0, 2, 2, 1, 1, 1, 1);
					leds_set_slope_mode(client, LED_B,
							0, 15, 10, 0, 2, 2, 1, 1, 1, 1);
				} else {
					leds_on(LED_R, true, false, 0x70);
					leds_on(LED_G, true, false, 0xD1);
					leds_on(LED_B, true, false, 0x0E);
				}
				break;
		case BATTERY_HIGH:
				if (breathing_leds) {
					leds_on(LED_R, true, true, 0x0F);
					leds_on(LED_G, true, true, 0xEF);
					leds_on(LED_B, true, true, 0x3F);
					leds_set_slope_mode(client, LED_R,
							0, 15, 10, 0, 2, 2, 1, 1, 1, 1);
					leds_set_slope_mode(client, LED_G,
							0, 15, 10, 0, 2, 2, 1, 1, 1, 1);
					leds_set_slope_mode(client, LED_B,
							0, 15, 10, 0, 2, 2, 1, 1, 1, 1);
				} else {
					leds_on(LED_R, true, false, 0x0F);
					leds_on(LED_G, true, false, 0xEF);
					leds_on(LED_B, true, false, 0x3F);
				}
				break;
		}
		break;
	case CHARGING_ERR:
			pr_info("LED Battery Charging error Pattern on\n");
		if (breathing_leds) {
			leds_on(LED_R, true, true, led_dynamic_current);
			leds_set_slope_mode(client, LED_R,
					0, 15, 15, 0, 2, 2, 1, 1, 1, 1);
		} else {
			leds_on(LED_R, true, true, led_dynamic_current);
			leds_set_slope_mode(client, LED_R,
					0, 15, 15, 0, AN30259A_TIME_UNIT - 450) /
				AN30259A_TIME_UNIT, AN30259A_TIME_UNIT - 450) /
				AN30259A_TIME_UNIT, 0, 0, 0, 0);
		}
 		break;
	case MISSED_NOTI:
		if (poweroff_charging) {
			return;
		}
		pr_info("LED Missed Notifications Pattern on\n");
		if (breathing_leds) {
			leds_on(LED_B, true, true, led_dynamic_current);
			leds_set_slope_mode(client, LED_B,
					0, 15, 10, 0, 2, 2, 1, 1, 1, 1);
		} else {
			leds_on(LED_B, true, true, led_dynamic_current);
			leds_set_slope_mode(client, LED_B,
					4, 15, 15, 0, 2, 8, 0, 0, 0, 0);
		}
		break;
	case LOW_BATTERY:
		if (poweroff_charging) {
			return;
		}
		pr_info("LED Low Battery Pattern on\n");
		if (breathing_leds) {
			leds_on(LED_R, true, true, led_dynamic_current);
			leds_set_slope_mode(client, LED_R,
					4, 15, 10, 0, 2, 4, 1, 1, 1, 1);
		} else {
			leds_on(LED_R, true, true, led_dynamic_current);
			leds_set_slope_mode(client, LED_R,
					4, 15, 15, 0, 2, 8, 1, 1, 1, 1);
		}
 		break;
 	case FULLY_CHARGED:
		pr_info("LED full Charged battery Pattern on\n");
		if (breathing_leds) {
			leds_on(LED_G, true, true, 0xFF);
			leds_set_slope_mode(client, LED_G,
					0, 15, 13, 0, 2, 2, 1, 1, 1, 1);
		} else
			leds_on(LED_G, true, false, 0xFF);
		break;

	case POWERING:
		if (poweroff_charging) {
			return;
		}
		if (!booted) {
			pr_info("LED Powering Pattern ON\n");
			leds_on(LED_R, true, true, 0xEA);
			leds_on(LED_G, true, true, 0xE2);
			leds_on(LED_B, true, true, 0xFF);
			leds_set_slope_mode(client, LED_R,
					0, 15, 1, 0, 4, 4, 1, 1, 1, 1);
			leds_set_slope_mode(client, LED_G,
					0, 15, 1, 1, 4, 4, 1, 1, 1, 1);
			leds_set_slope_mode(client, LED_B,
					4, 15, 1, 0, 4, 4, 1, 1, 1, 1);
			booted = true;
		} else {
			pr_info("Fade to Black\n");
			leds_on(LED_R, true, true, 0xE);
			leds_on(LED_G, true, true, 0xFF);
			leds_on(LED_B, true, true, 0xF6);
			leds_set_slope_mode(client, LED_R,
					0, 1, 1, 0, 2, 6, 12, 10, 10, 12);
			leds_set_slope_mode(client, LED_G,
					0, 16, 2, 0, 2, 6, 12, 10, 10, 12);
			leds_set_slope_mode(client, LED_B,
					0, 16, 2, 0, 2, 6, 12, 10, 10, 12);
		}
			break;
	case FAKE_POWERING:
		if (poweroff_charging) {
			return;
		}
		pr_info("LED Fake Powering Pattern ON\n");
			leds_on(LED_R, true, true, 192);
			leds_on(LED_G, true, true, 221);
			leds_on(LED_B, true, true, 254);
			leds_set_slope_mode(client, LED_R,
					0, 2, 1, 0, 4, 4, 1, 1, 1, 1);
			leds_set_slope_mode(client, LED_G,
					0, 15, 10, 4, 4, 4, 1, 1, 1, 1);
			leds_set_slope_mode(client, LED_B,
					4, 15, 9, 0, 4, 4, 1, 1, 1, 1);
			break;
	case BOOTING:
		if (poweroff_charging) {
			return;
		}
		pr_info("LED Booting Pattern on\n");
		an30259a_set_led_delayed_blink(LED_R, 1, 1, 1, 0xEA, false);
		an30259a_set_led_delayed_blink(LED_G, 1, 1, 1, 0xE2, false);
		an30259a_set_led_delayed_blink(LED_B, 0, 1, 1, 0xFF, false);
		break;

	case CUSTOM:
		if (poweroff_charging) {
			return;
		}
		if (custom_r_enabled) {
			leds_on(LED_R, true, true, led_dynamic_current);
			leds_set_slope_mode(client, LED_R,
					custom_r_delay,
					custom_r_dutymax, custom_r_dutymid, custom_r_dutymin,
					custom_r_total1, custom_r_total2,
					custom_r_dt1, custom_r_dt2, custom_r_dt3, custom_r_dt4);
		}
		if (custom_g_enabled) {
			leds_on(LED_G, true, true, led_dynamic_current);
			leds_set_slope_mode(client, LED_G,
					custom_g_delay,
					custom_g_dutymax, custom_g_dutymid, custom_g_dutymin,
					custom_g_total1, custom_g_total2,
					custom_g_dt1, custom_g_dt2, custom_g_dt3, custom_g_dt4);
		}
		if (custom_b_enabled) {
			leds_on(LED_B, true, true, led_dynamic_current);
			leds_set_slope_mode(client, LED_B,
					custom_b_delay,
					custom_b_dutymax, custom_b_dutymid, custom_b_dutymin,
					custom_b_total1, custom_b_total2,
					custom_b_dt1, custom_b_dt2, custom_b_dt3, custom_b_dt4);
		}
		break;
	default:
		return;
	}
	leds_i2c_write_all(client);
}

void send_led_full_msg(int level)
{
	if (level == battery_level)
		return;

	battery_level = level;

/*
	if ((current_led_mode == FULLY_CHARGED || current_led_mode == CHARGING) 
		&& !is_charger_connected) {
		an30259a_start_led_pattern(PATTERN_OFF);
		return;
	}
*/
	if (is_display_on() || (current_led_mode != CHARGING &&
		current_led_mode != FULLY_CHARGED) || pattern_active || !booted) {
		return;
	}

	wake_lock(&ledlock);
	if (battery_level == BATTERY_FULL && current_led_mode == CHARGING &&
		is_charger_connected) {
		is_full_charge = true;
		an30259a_start_led_pattern(FULLY_CHARGED);
		wake_unlock(&ledlock);
		return;
	}

	is_full_charge = false;

	if (battery_level >= BATTERY_LOW && battery_level <= BATTERY_HIGH &&
		current_led_mode == CHARGING &&
		is_charger_connected) {
		an30259a_start_led_pattern(CHARGING);
		wake_unlock(&ledlock);
		return;
	}

	if (wake_lock_active(&ledlock))
		wake_unlock(&ledlock);
}
		
/* Added for led common class */
static ssize_t show_an30259a_led_lowpower(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct an30259a_data *data = dev_get_drvdata(dev);

	return sprintf(buf, "%u\n", led_lowpower_mode);
}

static ssize_t store_an30259a_led_lowpower(struct device *dev,
					struct device_attribute *devattr,
					const char *buf, size_t count)
{
	int retval;
	u8 led_lowpower;
	struct an30259a_data *data = dev_get_drvdata(dev);

	retval = kstrtou8(buf, 0, &led_lowpower);
	if (retval != 0) {
		dev_err(&data->client->dev, "fail to get led_lowpower.\n");
		return count;
	}

	led_lowpower_mode = led_lowpower;

	return count;
}

/* Added for led common class */
static ssize_t show_an30259a_led_br_lev(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct an30259a_data *data = dev_get_drvdata(dev);

	return sprintf(buf, "%u\n", imax_copy);
}

static ssize_t store_an30259a_led_br_lev(struct device *dev,
					struct device_attribute *devattr,
					const char *buf, size_t count)
{
	int retval;
	unsigned long brightness_lev;
	struct i2c_client *client;
	struct an30259a_data *data = dev_get_drvdata(dev);
	client = b_client;

	retval = kstrtoul(buf, 16, &brightness_lev);
	if (retval != 0) {
		dev_err(&data->client->dev, "fail to get led_br_lev.\n");
		return count;
	}

	leds_set_imax(client, brightness_lev);
	imax_copy = brightness_lev;

	return count;
}

static ssize_t show_an30259a_led_pattern(struct device *dev,
                    struct device_attribute *attr, char *buf)
{
		return sprintf(buf, "%u\n", current_led_mode);
}

static ssize_t store_an30259a_led_pattern(struct device *dev,
					struct device_attribute *devattr,
					const char *buf, size_t count)
{
	int retval;
	unsigned int mode = 0;
	struct an30259a_data *data = dev_get_drvdata(dev);

	retval = sscanf(buf, "%d", &mode);

	if (retval == 0) {
		dev_err(&data->client->dev, "fail to get led_pattern mode.\n");
		return count;
	}

	an30259a_start_led_pattern(mode);

	return count;
}
/*
static int is_blinking;
static ssize_t show_an30259a_led_blink(struct device *dev,
                    struct device_attribute *attr, char *buf)
{
		return sprintf(buf, "%u\n", current_led_mode);
}
*/
static ssize_t store_an30259a_led_blink(struct device *dev,
					struct device_attribute *devattr,
					const char *buf, size_t count)
{
	int retval;
	unsigned int led_brightness = 0;
	unsigned int delay_on_time = 0;
	unsigned int delay_off_time = 0;
	struct an30259a_data *data = dev_get_drvdata(dev);
	u8 led_r_brightness = 0;
	u8 led_g_brightness = 0;
	u8 led_b_brightness = 0;
	struct work_struct *reset = 0;

	retval = sscanf(buf, "0x%x %d %d", &led_brightness,
				&delay_on_time, &delay_off_time);

	if (retval == 0) {
		dev_err(&data->client->dev, "fail to get led_blink value.\n");
		return count;
	}
	/*Reset an30259a*/
	an30259a_reset_register_work(reset);

	/*Set LED blink mode*/
	led_r_brightness = ((u32)led_brightness & LED_R_MASK)
					>> LED_R_SHIFT;
	led_g_brightness = ((u32)led_brightness & LED_G_MASK)
					>> LED_G_SHIFT;
	led_b_brightness = ((u32)led_brightness & LED_B_MASK);

	an30259a_set_led_blink(LED_R, delay_on_time,
				delay_off_time, led_r_brightness);
	an30259a_set_led_blink(LED_G, delay_on_time,
				delay_off_time, led_g_brightness);
	an30259a_set_led_blink(LED_B, delay_on_time,
				delay_off_time, led_b_brightness);

	leds_i2c_write_all(data->client);

	return count;
}

static ssize_t store_led_r(struct device *dev,
	struct device_attribute *devattr, const char *buf, size_t count)
{
	struct an30259a_data *data = dev_get_drvdata(dev);
	char buff[10] = {0,};
	int cnt, ret;
	u8 brightness;

	cnt = count;
	cnt = (buf[cnt-1] == '\n') ? cnt-1 : cnt;
	memcpy(buff, buf, cnt);
	buff[cnt] = '\0';

	ret = kstrtou8(buff, 0, &brightness);
	if (ret != 0) {
		dev_err(&data->client->dev, "fail to get brightness.\n");
		goto out;
	}

	if (brightness == 0)
		leds_on(LED_R, false, false, 0);
	else
		leds_on(LED_R, true, false, brightness);

	leds_i2c_write_all(data->client);
out:
	return count;
}

static ssize_t store_led_g(struct device *dev,
	struct device_attribute *devattr, const char *buf, size_t count)
{
	struct an30259a_data *data = dev_get_drvdata(dev);
	char buff[10] = {0,};
	int cnt, ret;
	u8 brightness;

	cnt = count;
	cnt = (buf[cnt-1] == '\n') ? cnt-1 : cnt;
	memcpy(buff, buf, cnt);
	buff[cnt] = '\0';

	ret = kstrtou8(buff, 0, &brightness);
	if (ret != 0) {
		dev_err(&data->client->dev, "fail to get brightness.\n");
		goto out;
	}

	if (brightness == 0)
		leds_on(LED_G, false, false, 0);
	else
		leds_on(LED_G, true, false, brightness);

	leds_i2c_write_all(data->client);
out:
	return count;
}

static ssize_t store_led_b(struct device *dev,
	struct device_attribute *devattr, const char *buf, size_t count)
{
	struct an30259a_data *data = dev_get_drvdata(dev);
	char buff[10] = {0,};
	int cnt, ret;
	u8 brightness;

	cnt = count;
	cnt = (buf[cnt-1] == '\n') ? cnt-1 : cnt;
	memcpy(buff, buf, cnt);
	buff[cnt] = '\0';

	ret = kstrtou8(buff, 0, &brightness);
	if (ret != 0) {
		dev_err(&data->client->dev, "fail to get brightness.\n");
		goto out;
	}

	if (brightness == 0)
		leds_on(LED_B, false, false, 0);
	else
		leds_on(LED_B, true, false, brightness);

	leds_i2c_write_all(data->client);
out:
	return count;

}
#endif

/* Added for led common class */
static ssize_t led_delay_on_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct an30259a_led *led = cdev_to_led(led_cdev);

	return snprintf(buf, 10, "%lu\n", led->delay_on_time_ms);
}

static ssize_t led_delay_on_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t len)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct an30259a_led *led = cdev_to_led(led_cdev);
	unsigned long time;

	if (kstrtoul(buf, 0, &time))
		return -EINVAL;

	led->delay_on_time_ms = (int)time;
	return len;
}

static ssize_t led_delay_off_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct an30259a_led *led = cdev_to_led(led_cdev);

	return snprintf(buf, 10, "%lu\n", led->delay_off_time_ms);
}

static ssize_t led_delay_off_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t len)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct an30259a_led *led = cdev_to_led(led_cdev);
	unsigned long time;

	if (kstrtoul(buf, 0, &time))
		return -EINVAL;

	led->delay_off_time_ms = (int)time;

	return len;
}

static ssize_t led_blink_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t len)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct an30259a_led *led = cdev_to_led(led_cdev);
	unsigned long blink_set;

	if (kstrtoul(buf, 0, &blink_set))
		return -EINVAL;

	if (!blink_set) {
		led->delay_on_time_ms = LED_OFF;
		an30259a_set_brightness(led_cdev, LED_OFF);
	}

	led_blink_set(led_cdev,
		&led->delay_on_time_ms, &led->delay_off_time_ms);

	return len;
}

/* permission for sysfs node */
static DEVICE_ATTR(delay_on, 0644, led_delay_on_show, led_delay_on_store);
static DEVICE_ATTR(delay_off, 0644, led_delay_off_show, led_delay_off_store);
static DEVICE_ATTR(blink, 0644, NULL, led_blink_store);

#ifdef SEC_LED_SPECIFIC
/* below nodes is SAMSUNG specific nodes */
static DEVICE_ATTR(led_r, 0664, NULL, store_led_r);
static DEVICE_ATTR(led_g, 0664, NULL, store_led_g);
static DEVICE_ATTR(led_b, 0664, NULL, store_led_b);
/* led_pattern node permission is 664 */
/* To access sysfs node from other groups */
static DEVICE_ATTR(led_pattern, 0664, show_an30259a_led_pattern, \
					store_an30259a_led_pattern);
static DEVICE_ATTR(led_blink, 0664, NULL, \
					store_an30259a_led_blink);
static DEVICE_ATTR(led_br_lev, 0664, show_an30259a_led_br_lev, \
					store_an30259a_led_br_lev);
static DEVICE_ATTR(led_lowpower, 0664, show_an30259a_led_lowpower, \
					store_an30259a_led_lowpower);


#endif
static struct attribute *led_class_attrs[] = {
	&dev_attr_delay_on.attr,
	&dev_attr_delay_off.attr,
	&dev_attr_blink.attr,
	NULL,
};

static struct attribute_group common_led_attr_group = {
	.attrs = led_class_attrs,
};

#ifdef SEC_LED_SPECIFIC
static struct attribute *sec_led_attributes[] = {
	&dev_attr_led_r.attr,
	&dev_attr_led_g.attr,
	&dev_attr_led_b.attr,
	&dev_attr_led_pattern.attr,
	&dev_attr_led_blink.attr,
	&dev_attr_led_br_lev.attr,
	&dev_attr_led_lowpower.attr,
	NULL,
};

static struct attribute_group sec_led_attr_group = {
	.attrs = sec_led_attributes,
};
#endif

enum mx_led_combos {
	NONE = 0,
	RED,
	GREEN,
	BLUE,
	RED_GREEN,
	RED_BLUE,
	GREEN_BLUE,
	RED_GREEN_BLUE,
};

static int custom_led_colours = NONE;

static void pick_custom_colour(int colour)
{
	if (colour <= NONE)
		colour = NONE;
	if (colour >= RED_GREEN_BLUE)
		colour = RED_GREEN_BLUE;

	switch(colour) {
	case NONE:
		custom_r_enabled = 0;
		custom_g_enabled = 0;
		custom_b_enabled = 0;
		break;
	case RED:
		custom_r_enabled = 1;
		custom_g_enabled = 0;
		custom_b_enabled = 0;
		break;
	case GREEN:
		custom_r_enabled = 0;
		custom_g_enabled = 1;
		custom_b_enabled = 0;
		break;
	case BLUE:
		custom_r_enabled = 0;
		custom_g_enabled = 0;
		custom_b_enabled = 1;
		break;
	case RED_GREEN:
		custom_r_enabled = 1;
		custom_g_enabled = 1;
		custom_b_enabled = 0;
		break;
	case RED_BLUE:
		custom_r_enabled = 1;
		custom_g_enabled = 0;
		custom_b_enabled = 1;
		break;
	case GREEN_BLUE:
		custom_r_enabled = 0;
		custom_g_enabled = 1;
		custom_b_enabled = 1;
		break;
	case RED_GREEN_BLUE:
		custom_r_enabled = 1;
		custom_g_enabled = 1;
		custom_b_enabled = 1;
	default:
		break;
	}

	an30259a_start_led_pattern(current_led_mode);
}

#define show_one_led(object)				\
static ssize_t show_##object					\
(struct kobject *kobj, struct kobj_attribute *attr, char *buf)	\
{								\
	return sprintf(buf, "%u\n", object);			\
}

#define store_switch(file_name)		\
static ssize_t store_##file_name		\
(struct kobject *kobj,				\
 struct kobj_attribute *attr,			\
 const char *buf, size_t count)			\
{						\
	unsigned int input;			\
	int ret;				\
	ret = sscanf(buf, "%u", &input);	\
	if (ret != 1)		\
		return -EINVAL;			\
	if (input == file_name)			\
		return count;			\
	if (input >= 1)		\
		input = 1;		\
	if (input <= 0)		\
		input = 0;		\
	file_name = input;				\
	an30259a_start_led_pattern(current_led_mode); \
	return count;				\
}

#define store_long(file_name)		\
static ssize_t store_##file_name		\
(struct kobject *kobj,				\
 struct kobj_attribute *attr,			\
 const char *buf, size_t count)			\
{						\
	unsigned int input;			\
	int ret;				\
	ret = sscanf(buf, "%u", &input);	\
	if (ret != 1)		\
		return -EINVAL;			\
	if (input == file_name)			\
		return count;			\
	if (input >= 10)		\
		input = 10;		\
	if (input <= 0)		\
		input = 0;		\
								\
	file_name = input;				\
	an30259a_start_led_pattern(current_led_mode); \
	return count;				\
}

#define store_duty(file_name)		\
static ssize_t store_##file_name		\
(struct kobject *kobj,				\
 struct kobj_attribute *attr,			\
 const char *buf, size_t count)			\
{						\
	unsigned int input;			\
	int ret;				\
	ret = sscanf(buf, "%u", &input);	\
	if (ret != 1)		\
		return -EINVAL;			\
	if (input == file_name)			\
		return count;			\
	if (input >= 15)		\
		input = 15;		\
	if (input <= 0)		\
		input = 0;		\
					\
	file_name = input;				\
	an30259a_start_led_pattern(current_led_mode); \
	return count;				\
}

#define store_slpdt(file_name)		\
static ssize_t store_##file_name		\
(struct kobject *kobj,				\
 struct kobj_attribute *attr,			\
 const char *buf, size_t count)			\
{						\
	unsigned int input;			\
	int ret;				\
	ret = sscanf(buf, "%u", &input);	\
	if (ret != 1)		\
		return -EINVAL;			\
	if (input == file_name)			\
		return count;			\
	if (input >= 15)		\
		input = 15;		\
	if (input <= 0)		\
		input = 0;		\
					\
	file_name = input;				\
	an30259a_start_led_pattern(current_led_mode); \
	return count;				\
}

static ssize_t show_breathing_leds(struct kobject *kobj, 
				struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", breathing_leds);
}

static ssize_t store_breathing_leds(struct kobject *kobj,
			   struct kobj_attribute *attr, const char *buf, size_t count)
{
	int input, ret;
	struct work_struct *reset = 0;
	struct i2c_client *client;
	client = b_client;
	ret = sscanf(buf, "%d", &input);
	if (ret != 1)
		return -EINVAL;

	if (input == breathing_leds)
		return count;
	breathing_leds = input;

	if (breathing_leds)
		an30259a_set_slope_current(inhale, exhale, false);
	else {
		an30259a_reset_register_work(reset);
		an30259a_set_slope_current(0, 0, true);

	}
	return count;
}
MX_ATTR_RW(breathing_leds);

static ssize_t show_inhale(struct kobject *kobj, 
				struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", inhale);
}

static ssize_t store_inhale(struct kobject *kobj,
			   struct kobj_attribute *attr, const char *buf, size_t count)
{
	int input, ret;

	ret = sscanf(buf, "%d", &input);
	if (ret != 1)
		return -EINVAL;

	if (input == inhale)
		return count;
	inhale = input;
	if (breathing_leds)
		an30259a_set_slope_current(inhale, exhale, false);
	return count;
}
MX_ATTR_RW(inhale);
static ssize_t show_exhale(struct kobject *kobj, 
				struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", exhale);
}

static ssize_t store_exhale(struct kobject *kobj,
			   struct kobj_attribute *attr, const char *buf, size_t count)
{
	int input, ret;

	ret = sscanf(buf, "%d", &input);
	if (ret != 1)
		return -EINVAL;

	if (input == exhale)
		return count;
	exhale = input;

	if (breathing_leds)
		an30259a_set_slope_current(inhale, exhale, false);
	return count;
}

MX_ATTR_RW(exhale);

show_one_led(custom_led_colours);
static ssize_t store_custom_led_colours(struct kobject *kobj,
			   struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;
	if (input == custom_led_colours)
		return count;

	if (input <= NONE)
		input = NONE;
	if (input >= RED_GREEN_BLUE)
		input = RED_GREEN_BLUE;

	custom_led_colours = input;
	pick_custom_colour(custom_led_colours);
	return count;
}
MX_ATTR_RW(custom_led_colours);

static ssize_t show_real_led_lowpower_mode(struct kobject *kobj, 
				struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", real_led_lowpower_mode);
}

static ssize_t store_real_led_lowpower_mode(struct kobject *kobj,
			   struct kobj_attribute *attr, const char *buf, size_t count)
{
	int input, ret;

	ret = sscanf(buf, "%d", &input);
	if (ret != 1)
		return -EINVAL;

	if (input == real_led_lowpower_mode)
		return count;

	real_led_lowpower_mode = input;

	return count;
}
MX_ATTR_RW(real_led_lowpower_mode);

show_one_led(custom_r_delay);
show_one_led(custom_r_dutymax);
show_one_led(custom_r_dutymid);
show_one_led(custom_r_dutymin);
show_one_led(custom_r_total1);
show_one_led(custom_r_total2);
show_one_led(custom_r_dt1);
show_one_led(custom_r_dt2);
show_one_led(custom_r_dt3);
show_one_led(custom_r_dt4);

store_long(custom_r_delay);
store_duty(custom_r_dutymax);
store_duty(custom_r_dutymid);
store_duty(custom_r_dutymin);
store_long(custom_r_total1);
store_long(custom_r_total2);
store_slpdt(custom_r_dt1);
store_slpdt(custom_r_dt2);
store_slpdt(custom_r_dt3);
store_slpdt(custom_r_dt4);

MX_ATTR_RW(custom_r_delay);
MX_ATTR_RW(custom_r_dutymax);
MX_ATTR_RW(custom_r_dutymid);
MX_ATTR_RW(custom_r_dutymin);
MX_ATTR_RW(custom_r_total1);
MX_ATTR_RW(custom_r_total2);
MX_ATTR_RW(custom_r_dt1);
MX_ATTR_RW(custom_r_dt2);
MX_ATTR_RW(custom_r_dt3);
MX_ATTR_RW(custom_r_dt4);

show_one_led(custom_g_delay);
show_one_led(custom_g_dutymax);
show_one_led(custom_g_dutymid);
show_one_led(custom_g_dutymin);
show_one_led(custom_g_total1);
show_one_led(custom_g_total2);
show_one_led(custom_g_dt1);
show_one_led(custom_g_dt2);
show_one_led(custom_g_dt3);
show_one_led(custom_g_dt4);

store_long(custom_g_delay);
store_duty(custom_g_dutymax);
store_duty(custom_g_dutymid);
store_duty(custom_g_dutymin);
store_long(custom_g_total1);
store_long(custom_g_total2);
store_slpdt(custom_g_dt1);
store_slpdt(custom_g_dt2);
store_slpdt(custom_g_dt3);
store_slpdt(custom_g_dt4);

MX_ATTR_RW(custom_g_delay);
MX_ATTR_RW(custom_g_dutymax);
MX_ATTR_RW(custom_g_dutymid);
MX_ATTR_RW(custom_g_dutymin);
MX_ATTR_RW(custom_g_total1);
MX_ATTR_RW(custom_g_total2);
MX_ATTR_RW(custom_g_dt1);
MX_ATTR_RW(custom_g_dt2);
MX_ATTR_RW(custom_g_dt3);
MX_ATTR_RW(custom_g_dt4);

show_one_led(custom_b_delay);
show_one_led(custom_b_dutymax);
show_one_led(custom_b_dutymid);
show_one_led(custom_b_dutymin);
show_one_led(custom_b_total1);
show_one_led(custom_b_total2);
show_one_led(custom_b_dt1);
show_one_led(custom_b_dt2);
show_one_led(custom_b_dt3);
show_one_led(custom_b_dt4);

store_long(custom_b_delay);
store_duty(custom_b_dutymax);
store_duty(custom_b_dutymid);
store_duty(custom_b_dutymin);
store_long(custom_b_total1);
store_long(custom_b_total2);
store_slpdt(custom_b_dt1);
store_slpdt(custom_b_dt2);
store_slpdt(custom_b_dt3);
store_slpdt(custom_b_dt4);

MX_ATTR_RW(custom_b_delay);
MX_ATTR_RW(custom_b_dutymax);
MX_ATTR_RW(custom_b_dutymid);
MX_ATTR_RW(custom_b_dutymin);
MX_ATTR_RW(custom_b_total1);
MX_ATTR_RW(custom_b_total2);
MX_ATTR_RW(custom_b_dt1);
MX_ATTR_RW(custom_b_dt2);
MX_ATTR_RW(custom_b_dt3);
MX_ATTR_RW(custom_b_dt4);

static struct attribute *cust_led_r_attrs[] = {
	&custom_r_delay_attr.attr,
	&custom_r_dutymax_attr.attr,
	&custom_r_dutymid_attr.attr,
	&custom_r_dutymin_attr.attr,
	&custom_r_total1_attr.attr,
	&custom_r_total2_attr.attr,
	&custom_r_dt1_attr.attr,
	&custom_r_dt2_attr.attr,
	&custom_r_dt3_attr.attr,
	&custom_r_dt4_attr.attr,
	NULL,
};

static struct attribute *cust_led_g_attrs[] = {
	&custom_g_delay_attr.attr,
	&custom_g_dutymax_attr.attr,
	&custom_g_dutymid_attr.attr,
	&custom_g_dutymin_attr.attr,
	&custom_g_total1_attr.attr,
	&custom_g_total2_attr.attr,
	&custom_g_dt1_attr.attr,
	&custom_g_dt2_attr.attr,
	&custom_g_dt3_attr.attr,
	&custom_g_dt4_attr.attr,
	NULL,
};

static struct attribute *cust_led_b_attrs[] = {
	&custom_b_delay_attr.attr,
	&custom_b_dutymax_attr.attr,
	&custom_b_dutymid_attr.attr,
	&custom_b_dutymin_attr.attr,
	&custom_b_total1_attr.attr,
	&custom_b_total2_attr.attr,
	&custom_b_dt1_attr.attr,
	&custom_b_dt2_attr.attr,
	&custom_b_dt3_attr.attr,
	&custom_b_dt4_attr.attr,
	NULL,
};

static struct attribute_group cust_led_r_attr_group = {
	.attrs = cust_led_r_attrs,
};

static struct attribute_group cust_led_g_attr_group = {
	.attrs = cust_led_g_attrs,
};

static struct attribute_group cust_led_b_attr_group = {
	.attrs = cust_led_b_attrs,
};

static struct attribute *mx_custom_leds_attrs[] = {
	&breathing_leds_attr.attr,
	&inhale_attr.attr,
	&exhale_attr.attr,
	&custom_led_colours_attr.attr,
	&real_led_lowpower_mode_attr.attr,
	NULL
};

static struct attribute_group mx_custom_leds_attr_group = {
	.attrs = mx_custom_leds_attrs,
};

static struct kobject *mx_custom_leds_kobj;
static struct kobject *custom_r_kobj;
static struct kobject *custom_g_kobj;
static struct kobject *custom_b_kobj;

static int __init mx_leds_late_init(void)
{
	int error;

	mx_custom_leds_kobj = kobject_create_and_add("mx_custom_leds", mx_kobj);
	error = sysfs_create_group(mx_custom_leds_kobj, &mx_custom_leds_attr_group);
	if (error) {
		pr_err("Failed to create mx_custom_leds kobject!\n");
		goto exit;
	}

	custom_r_kobj = kobject_create_and_add("custom_r", mx_custom_leds_kobj);
	error = sysfs_create_group(custom_r_kobj, &cust_led_r_attr_group);
	if (error) {
		pr_err("Failed to create custom_r kobject!\n");
		goto rexit;
	}

	custom_g_kobj = kobject_create_and_add("custom_g", mx_custom_leds_kobj);
	error = sysfs_create_group(custom_g_kobj, &cust_led_g_attr_group);
	if (error) {
		pr_err("Failed to create custom_g kobject!\n");
		goto gexit;
	}

	custom_b_kobj = kobject_create_and_add("custom_b", mx_custom_leds_kobj);
	error = sysfs_create_group(custom_b_kobj, &cust_led_b_attr_group);
	if (error) {
		pr_err("Failed to create custom_b kobject!\n");
		goto bexit;
	}
	wake_lock_init(&ledlock, WAKE_LOCK_SUSPEND, "led_wake");

	return 0;

bexit:
	kobject_put(custom_b_kobj);
	sysfs_remove_group(custom_g_kobj, &cust_led_g_attr_group);
gexit:
	kobject_put(custom_g_kobj);
	sysfs_remove_group(custom_r_kobj, &cust_led_r_attr_group);
rexit:
	kobject_put(custom_r_kobj);
	sysfs_remove_group(mx_custom_leds_kobj, &mx_custom_leds_attr_group);
parent_exit:
	kobject_put(mx_custom_leds_kobj);
exit:
	return error;
}

static int an30259a_initialize(struct i2c_client *client,
					struct an30259a_led *led, int channel)
{
	struct an30259a_data *data = i2c_get_clientdata(client);
	struct device *dev = &client->dev;
	int ret;

	/* reset an30259a*/
	ret = i2c_smbus_write_byte_data(client, AN30259A_REG_SRESET,
					AN30259A_SRESET);
	if (unlikely(ret < 0)) {
		dev_err(&client->adapter->dev,
			"%s: failure on i2c write (reg = 0x%2x)\n",
			__func__, AN30259A_REG_SRESET);
		return ret;
	}
	ret = i2c_smbus_read_i2c_block_data(client,
			AN30259A_REG_SRESET | AN30259A_CTN_RW_FLG,
			AN30259A_REG_MAX, data->shadow_reg);
	if (unlikely(ret < 0)) {
		dev_err(&client->adapter->dev,
			"%s: failure on i2c read block(ledxcc)\n",
			__func__);
		return ret;
	}
	led->channel = channel;
	led->cdev.brightness_set = an30259a_set_brightness;
	led->cdev.name = led_conf[channel].name;
	led->cdev.brightness = led_conf[channel].brightness;
	led->cdev.max_brightness = led_conf[channel].max_brightness;
	led->cdev.flags = led_conf[channel].flags;

	ret = led_classdev_register(dev, &led->cdev);

	if (unlikely(ret < 0)) {
		dev_err(dev, "can not register led channel : %d\n", channel);
		return ret;
	}

	ret = sysfs_create_group(&led->cdev.dev->kobj,
			&common_led_attr_group);

	if (unlikely(ret < 0)) {
		dev_err(dev, "can not register sysfs attribute for led channel : %d\n", channel);
		led_classdev_unregister(&led->cdev);
		return ret;
	}

	leds_set_imax(client, 0x00);
	imax_copy = 0x00;

	return 0;
}

//if one led will fail to register than all led registration will fail
static void an30259a_deinitialize(struct an30259a_led *led, int channel)
{
	sysfs_remove_group(&led->cdev.dev->kobj,&common_led_attr_group);
	led_classdev_unregister(&led->cdev);
	cancel_work_sync(&led->brightness_work);
}

static int an30259a_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	struct an30259a_data *data;
	int ret, i;

	dev_dbg(&client->adapter->dev, "%s\n", __func__);
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "need I2C_FUNC_I2C.\n");
		return -ENODEV;
	}

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (!data) {
		dev_err(&client->adapter->dev,
			"failed to allocate driver data.\n");
		return -ENOMEM;
	}

	i2c_set_clientdata(client, data);
	data->client = client;
	b_client = client;

	mutex_init(&data->mutex);
	/* initialize LED */
	for (i = 0; i < MAX_NUM_LEDS; i++) {

		ret = an30259a_initialize(client, &data->leds[i], i);

		if (unlikely(ret < 0)) {
			dev_err(&client->adapter->dev, "failure on initialization at led channel:%d\n", i);
			while(i > 0) {
					i--;
					an30259a_deinitialize(&data->leds[i], i);
			}
			goto exit;
		}
		INIT_WORK(&(data->leds[i].brightness_work),
				 an30259a_led_brightness_work);
	}

#ifdef SEC_LED_SPECIFIC
	led_dev = device_create(sec_class, NULL, 0, data, "led");
	if (IS_ERR(led_dev)) {
		dev_err(&client->dev,
			"Failed to create device for samsung specific led\n");
		ret = -ENODEV;
		goto exit;
	}
	ret = sysfs_create_group(&led_dev->kobj, &sec_led_attr_group);
	if (ret) {
		dev_err(&client->dev,
			"Failed to create sysfs group for samsung specific led\n");
		goto exit1;
	}
#endif
	if (!poweroff_charging)
		an30259a_start_led_pattern(BOOTING);
	return ret;

#ifdef SEC_LED_SPECIFIC
exit1:
	device_destroy(sec_class, 0);
#endif
exit:
	mutex_destroy(&data->mutex);
	kfree(data);
	return ret;
}

static int an30259a_remove(struct i2c_client *client)
{
	struct an30259a_data *data = i2c_get_clientdata(client);
	unsigned int i;
	dev_dbg(&client->adapter->dev, "%s\n", __func__);

	// this is not an ugly hack to shutdown led.
	data->shadow_reg[AN30259A_REG_LEDON] &= ~(LED_ON << 0);
	data->shadow_reg[AN30259A_REG_LEDON] &= ~(LED_ON << 1);
	data->shadow_reg[AN30259A_REG_LEDON] &= ~(LED_ON << 2);
	data->shadow_reg[AN30259A_REG_LED1CNT2 + 0 * 4] &= ~AN30259A_MASK_DELAY;
	data->shadow_reg[AN30259A_REG_LED1CNT2 + 1 * 4] &= ~AN30259A_MASK_DELAY;
	data->shadow_reg[AN30259A_REG_LED1CNT2 + 2 * 4] &= ~AN30259A_MASK_DELAY;
	data->shadow_reg[AN30259A_REG_LEDON] &= ~(LED_SLOPE_MODE << 0);
	data->shadow_reg[AN30259A_REG_LEDON] &= ~(LED_SLOPE_MODE << 1);
	data->shadow_reg[AN30259A_REG_LEDON] &= ~(LED_SLOPE_MODE << 2);
	data->shadow_reg[AN30259A_REG_LED1CC + 0] = 0;
	data->shadow_reg[AN30259A_REG_LED1CC + 1] = 0;
	data->shadow_reg[AN30259A_REG_LED1CC + 2] = 0;
	msleep(200);

#ifdef SEC_LED_SPECIFIC
	sysfs_remove_group(&led_dev->kobj, &sec_led_attr_group);
#endif
	for (i = 0; i < MAX_NUM_LEDS; i++) {
		sysfs_remove_group(&data->leds[i].cdev.dev->kobj,
						&common_led_attr_group);
		led_classdev_unregister(&data->leds[i].cdev);
		cancel_work_sync(&data->leds[i].brightness_work);
	}

	mutex_destroy(&data->mutex);
	kfree(data);
	return 0;
}

static struct i2c_device_id an30259a_id[] = {
	{"an30259a", 0},
	{},
};

MODULE_DEVICE_TABLE(i2c, an30259a_id);

static struct i2c_driver an30259a_i2c_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "an30259a",
	},
	.id_table = an30259a_id,
	.probe = an30259a_probe,
	.remove = an30259a_remove,
};

static int __init an30259a_init(void)
{
	return i2c_add_driver(&an30259a_i2c_driver);
}

static void __exit an30259a_exit(void)
{
	i2c_del_driver(&an30259a_i2c_driver);
}

module_init(an30259a_init);
module_exit(an30259a_exit);
late_initcall(mx_leds_late_init);

MODULE_DESCRIPTION("AN30259A LED driver");
MODULE_AUTHOR("Kamaldeep Singla <kamal.singla@samsung.com");
MODULE_LICENSE("GPL v2");
