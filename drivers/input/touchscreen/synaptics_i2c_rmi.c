/* Synaptics Register Mapped Interface (RMI4) I2C Physical Layer Driver.
 * Copyright (c) 2007-2012, Synaptics Incorporated
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
#include <linux/kernel.h>
#include <linux/module.h>
#include <asm/unaligned.h>
#include <mach/cpufreq.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#ifdef CONFIG_POWERSUSPEND
#include <linux/powersuspend.h>
#endif
#include "synaptics_i2c_rmi.h"
#ifdef CONFIG_WAKE_GESTURES
#include <linux/wake_gestures.h>
#endif
#ifdef CONFIG_KT_WAKE_FUNCS
#include "../../sensorhub/ssp_defs.h"
#include <linux/wakelock.h>
#include <linux/pm.h>
#include <linux/pm_runtime.h>
#endif
#ifdef CONFIG_TOUCH_WAKE
#include <linux/touch_wake.h>
#endif

#define DRIVER_NAME "synaptics_rmi4_i2c"

#define PROXIMITY
#define TYPE_B_PROTOCOL
#define SURFACE_TOUCH

#ifdef SURFACE_TOUCH
#define EDGE_SWIPE
#endif
#undef USE_OPEN_CLOSE
#undef USE_SENSOR_SLEEP

/*
#define REPORT_2D_Z
*/
#define REPORT_2D_W

#define F12_FINGERS_TO_SUPPORT 10

#define RPT_TYPE (1 << 0)
#define RPT_X_LSB (1 << 1)
#define RPT_X_MSB (1 << 2)
#define RPT_Y_LSB (1 << 3)
#define RPT_Y_MSB (1 << 4)
#define RPT_Z (1 << 5)
#define RPT_WX (1 << 6)
#define RPT_WY (1 << 7)
#define RPT_DEFAULT (RPT_TYPE | RPT_X_LSB | RPT_X_MSB | RPT_Y_LSB | RPT_Y_MSB)

#ifdef CONFIG_GLOVE_TOUCH
#define GLOVE_FEATURE_EN	0x20
#define GLOVE_CLEAR_DEFAULT	0x01
#define F12_CTRL26_OFFSET (8)
#endif

#ifdef PROXIMITY
#define USE_CUSTOM_REZERO

#define HOVER_Z_MAX (255)
#define FINGER_HOVER_EN (1 << 0)
#define FINGER_HOVER_DIS (0 << 0)
#define AIR_SWIPE_EN (1 << 1)
#define LARGE_OBJ_EN (1 << 2)
#define HOVER_PINCH_EN (1 << 3)
#define NO_PROXIMITY_ON_TOUCH_EN (1 << 5)
#define CONTINUOUS_LOAD_REPORT_EN (1 << 6)
#define SLEEP_PROXIMITY (1 << 7)

#define PROXIMITY_DEFAULT (NO_PROXIMITY_ON_TOUCH_EN)
#define PROXIMITY_ENABLE (PROXIMITY_DEFAULT | FINGER_HOVER_EN)

#define HAS_FINGER_HOVER (1 << 0)
#define HAS_AIR_SWIPE (1 << 1)
#define HAS_LARGE_OBJ (1 << 2)
#define HAS_HOVER_PINCH (1 << 3)
#define HAS_EDGE_SWIPE (1 << 4)
#define HAS_SINGLE_FINGER (1 << 5)
#define F51_VERSION	0x41
#define F51_PROXIMITY_ENABLES_OFFSET (0)
#define F51_CTRL54_OFFSET 99
#ifdef USE_CUSTOM_REZERO
#define F51_GENERAL_CONTROL_OFFSET (1)
#define F51_CTRL78_OFFSET	115
#endif

#ifdef EDGE_SWIPE
#define EDGE_SWIPE_DATA_OFFSET	8

#define EDGE_SWIPE_WIDTH_MAX	255
//#define EDGE_SWIPE_ANGLE_MIN	(-90)
//#define EDGE_SWIPE_ANGLE_MAX	90
#define EDGE_SWIPE_PALM_MAX		1
#endif

#define F51_FINGER_TIMEOUT 50 /* ms */
#endif

#define POLLING_PERIOD 1 /* ms */
#define SYN_I2C_RETRY_TIMES 5
#define MAX_F11_TOUCH_WIDTH 15

#define CHECK_STATUS_TIMEOUT_MS 200
#define STATUS_NO_ERROR 0x00
#define STATUS_RESET_OCCURRED 0x01
#define STATUS_INVALID_CONFIG 0x02
#define STATUS_DEVICE_FAILURE 0x03
#define STATUS_CONFIG_CRC_FAILURE 0x04
#define STATUS_FIRMWARE_CRC_FAILURE 0x05
#define STATUS_CRC_IN_PROGRESS 0x06

#define F01_STD_QUERY_LEN 21
#define F01_BUID_ID_OFFSET 18
#define F11_STD_QUERY_LEN 9
#define F11_STD_CTRL_LEN 10
#define F11_STD_DATA_LEN 12

#define NORMAL_OPERATION (0 << 0)
#define SENSOR_SLEEP (1 << 0)
#define NO_SLEEP_OFF (0 << 2)
#define NO_SLEEP_ON (1 << 2)
#define CHARGER_CONNECTED (1 << 5)
#define CHARGER_DISCONNECTED	0xDF
#define CONFIGURED (1 << 7)

static int synaptics_rmi4_i2c_read(struct synaptics_rmi4_data *rmi4_data,
		unsigned short addr, unsigned char *data,
		unsigned short length);

static int synaptics_rmi4_i2c_write(struct synaptics_rmi4_data *rmi4_data,
		unsigned short addr, unsigned char *data,
		unsigned short length);

static int synaptics_rmi4_reinit_device(struct synaptics_rmi4_data *rmi4_data);

#ifdef CONFIG_POWERSUSPEND
static ssize_t synaptics_rmi4_full_pm_cycle_show(struct device *dev,
		struct device_attribute *attr, char *buf);

static ssize_t synaptics_rmi4_full_pm_cycle_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);

static void synaptics_rmi4_power_suspend(struct power_suspend *h);

static void synaptics_rmi4_power_resume(struct power_suspend *h);

#else

static int synaptics_rmi4_suspend(struct device *dev);

static int synaptics_rmi4_resume(struct device *dev);
#endif

#ifdef PROXIMITY
static void synaptics_rmi4_f51_finger_timer(unsigned long data);

static ssize_t synaptics_rmi4_f51_enables_show(struct device *dev,
		struct device_attribute *attr, char *buf);

static ssize_t synaptics_rmi4_f51_enables_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);
#endif

#ifdef USE_OPEN_CLOSE
static int synaptics_rmi4_input_open(struct input_dev *dev);
static void synaptics_rmi4_input_close(struct input_dev *dev);
#endif
static ssize_t synaptics_rmi4_glove_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf);

static ssize_t synaptics_rmi4_glove_enable_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);

static ssize_t synaptics_rmi4_show_device_status(struct device *dev,
		struct device_attribute *attr, char *buf);

static ssize_t synaptics_rmi4_f01_reset_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);

static ssize_t synaptics_rmi4_f01_productinfo_show(struct device *dev,
		struct device_attribute *attr, char *buf);

static ssize_t synaptics_rmi4_f01_buildid_show(struct device *dev,
		struct device_attribute *attr, char *buf);

static ssize_t synaptics_rmi4_f01_flashprog_show(struct device *dev,
		struct device_attribute *attr, char *buf);

static ssize_t synaptics_rmi4_0dbutton_show(struct device *dev,
		struct device_attribute *attr, char *buf);

static ssize_t synaptics_rmi4_0dbutton_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);

#ifdef CONFIG_KT_WAKE_FUNCS
static ssize_t synaptics_rmi4_screen_wake_options_show(struct device *dev,
		struct device_attribute *attr, char *buf);

static ssize_t synaptics_rmi4_screen_wake_options_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);

static ssize_t synaptics_rmi4_screen_wake_options_prox_max_show(struct device *dev,
		struct device_attribute *attr, char *buf);

static ssize_t synaptics_rmi4_screen_wake_options_prox_max_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);

static ssize_t synaptics_rmi4_screen_wake_options_debug_show(struct device *dev,
		struct device_attribute *attr, char *buf);

static ssize_t synaptics_rmi4_screen_wake_options_debug_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);

static ssize_t synaptics_rmi4_screen_wake_options_hold_wlock_show(struct device *dev,
		struct device_attribute *attr, char *buf);

static ssize_t synaptics_rmi4_screen_wake_options_hold_wlock_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);

static ssize_t synaptics_rmi4_screen_sleep_options_show(struct device *dev,
		struct device_attribute *attr, char *buf);

static ssize_t synaptics_rmi4_screen_sleep_options_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);
#endif

struct synaptics_rmi4_f01_device_status {
	union {
		struct {
			unsigned char status_code:4;
			unsigned char reserved:2;
			unsigned char flash_prog:1;
			unsigned char unconfigured:1;
		} __packed;
		unsigned char data[1];
	};
};

struct synaptics_rmi4_f12_query_5 {
	union {
		struct {
			unsigned char size_of_query6;
			struct {
				unsigned char ctrl0_is_present:1;
				unsigned char ctrl1_is_present:1;
				unsigned char ctrl2_is_present:1;
				unsigned char ctrl3_is_present:1;
				unsigned char ctrl4_is_present:1;
				unsigned char ctrl5_is_present:1;
				unsigned char ctrl6_is_present:1;
				unsigned char ctrl7_is_present:1;
			} __packed;
			struct {
				unsigned char ctrl8_is_present:1;
				unsigned char ctrl9_is_present:1;
				unsigned char ctrl10_is_present:1;
				unsigned char ctrl11_is_present:1;
				unsigned char ctrl12_is_present:1;
				unsigned char ctrl13_is_present:1;
				unsigned char ctrl14_is_present:1;
				unsigned char ctrl15_is_present:1;
			} __packed;
			struct {
				unsigned char ctrl16_is_present:1;
				unsigned char ctrl17_is_present:1;
				unsigned char ctrl18_is_present:1;
				unsigned char ctrl19_is_present:1;
				unsigned char ctrl20_is_present:1;
				unsigned char ctrl21_is_present:1;
				unsigned char ctrl22_is_present:1;
				unsigned char ctrl23_is_present:1;
			} __packed;
			struct {
				unsigned char ctrl24_is_present:1;
				unsigned char ctrl25_is_present:1;
				unsigned char ctrl26_is_present:1;
				unsigned char ctrl27_is_present:1;
				unsigned char ctrl28_is_present:1;
				unsigned char ctrl29_is_present:1;
				unsigned char ctrl30_is_present:1;
				unsigned char ctrl31_is_present:1;
			} __packed;
		};
		unsigned char data[5];
	};
};

struct synaptics_rmi4_f12_query_8 {
	union {
		struct {
			unsigned char size_of_query9;
			struct {
				unsigned char data0_is_present:1;
				unsigned char data1_is_present:1;
				unsigned char data2_is_present:1;
				unsigned char data3_is_present:1;
				unsigned char data4_is_present:1;
				unsigned char data5_is_present:1;
				unsigned char data6_is_present:1;
				unsigned char data7_is_present:1;
			} __packed;
		};
		unsigned char data[2];
	};
};

struct synaptics_rmi4_f12_ctrl_8 {
	union {
		struct {
			unsigned char max_x_coord_lsb;
			unsigned char max_x_coord_msb;
			unsigned char max_y_coord_lsb;
			unsigned char max_y_coord_msb;
			unsigned char rx_pitch_lsb;
			unsigned char rx_pitch_msb;
			unsigned char tx_pitch_lsb;
			unsigned char tx_pitch_msb;
			unsigned char low_rx_clip;
			unsigned char high_rx_clip;
			unsigned char low_tx_clip;
			unsigned char high_tx_clip;
			unsigned char num_of_rx;
			unsigned char num_of_tx;
		};
		unsigned char data[14];
	};
};

struct synaptics_rmi4_f12_ctrl_9 {
	union {
		struct {
			unsigned char touch_threshold;
			unsigned char lift_hysteresis;
			unsigned char small_z_scale_factor_lsb;
			unsigned char small_z_scale_factor_msb;
			unsigned char large_z_scale_factor_lsb;
			unsigned char large_z_scale_factor_msb;
			unsigned char small_large_boundary;
			unsigned char wx_scale;
			unsigned char wx_offset;
			unsigned char wy_scale;
			unsigned char wy_offset;
			unsigned char x_size_lsb;
			unsigned char x_size_msb;
			unsigned char y_size_lsb;
			unsigned char y_size_msb;
			unsigned char gloved_finger;
		};
		unsigned char data[16];
	};
};

struct synaptics_rmi4_f12_ctrl_23 {
	union {
		struct {
			unsigned char obj_type_enable;
			unsigned char max_reported_objects;
		};
		unsigned char data[2];
	};
};

#ifdef CONFIG_GLOVE_TOUCH
struct synaptics_rmi4_f12_ctrl_26 {
	union {
		struct {
			unsigned char glove_feature_enable;
		};
		unsigned char data[1];
	};
};
#endif

struct synaptics_rmi4_f12_finger_data {
	unsigned char object_type_and_status;
	unsigned char x_lsb;
	unsigned char x_msb;
	unsigned char y_lsb;
	unsigned char y_msb;
#ifdef REPORT_2D_Z
	unsigned char z;
#endif
#ifdef REPORT_2D_W
	unsigned char wx;
	unsigned char wy;
#endif
};

struct synaptics_rmi4_f1a_query {
	union {
		struct {
			unsigned char max_button_count:3;
			unsigned char reserved:5;
			unsigned char has_general_control:1;
			unsigned char has_interrupt_enable:1;
			unsigned char has_multibutton_select:1;
			unsigned char has_tx_rx_map:1;
			unsigned char has_perbutton_threshold:1;
			unsigned char has_release_threshold:1;
			unsigned char has_strongestbtn_hysteresis:1;
			unsigned char has_filter_strength:1;
		} __packed;
		unsigned char data[2];
	};
};

struct synaptics_rmi4_f1a_control_0 {
	union {
		struct {
			unsigned char multibutton_report:2;
			unsigned char filter_mode:2;
			unsigned char reserved:4;
		} __packed;
		unsigned char data[1];
	};
};

struct synaptics_rmi4_f1a_control_3_4 {
	unsigned char transmitterbutton;
	unsigned char receiverbutton;
};

struct synaptics_rmi4_f1a_control {
	struct synaptics_rmi4_f1a_control_0 general_control;
	unsigned char *button_int_enable;
	unsigned char *multi_button;
	struct synaptics_rmi4_f1a_control_3_4 *electrode_map;
	unsigned char *button_threshold;
	unsigned char button_release_threshold;
	unsigned char strongest_button_hysteresis;
	unsigned char filter_strength;
};

struct synaptics_rmi4_f1a_handle {
	int button_bitmask_size;
	unsigned char button_count;
	unsigned char valid_button_count;
	unsigned char *button_data_buffer;
	unsigned char *button_map;
	struct synaptics_rmi4_f1a_query button_query;
	struct synaptics_rmi4_f1a_control button_control;
};

struct synaptics_rmi4_f34_ctrl_3 {
	union {
		struct {
			unsigned char fw_release_month;
			unsigned char fw_release_date;
			unsigned char fw_release_revision;
			unsigned char fw_release_version;
		};
		unsigned char data[4];
	};
};

struct synaptics_rmi4_f34_fn_ptr {
	int (*read)(struct synaptics_rmi4_data *rmi4_data, unsigned short addr,
			unsigned char *data, unsigned short length);
	int (*write)(struct synaptics_rmi4_data *rmi4_data, unsigned short addr,
			unsigned char *data, unsigned short length);
	int (*enable)(struct synaptics_rmi4_data *rmi4_data, bool enable);
};

struct synaptics_rmi4_f34_handle {
	unsigned char status;
	unsigned char cmd;
	unsigned short bootloaderid;
	unsigned short blocksize;
	unsigned short imageblockcount;
	unsigned short configblockcount;
	unsigned short blocknum;
	unsigned short query_base_addr;
	unsigned short control_base_addr;
	unsigned short data_base_addr;
	bool inflashprogmode;
	unsigned char intr_mask;
	struct mutex attn_mutex;
	struct synaptics_rmi4_f34_fn_ptr *fn_ptr;
};

#ifdef PROXIMITY
struct synaptics_rmi4_f51_query {
	union {
		struct {
			unsigned char query_register_count;
			unsigned char data_register_count;
			unsigned char control_register_count;
			unsigned char command_register_count;
			unsigned char proximity_controls;
		};
		unsigned char data[5];
	};
};

struct synaptics_rmi4_f51_data {
	union {
		struct {
			unsigned char finger_hover_det:1;
			unsigned char air_swipe_det:1;
			unsigned char large_obj_det:1;
			unsigned char f1a_data0_b3:1;
			unsigned char hover_pinch_det:1;
			unsigned char f1a_data0_b5__7:3;
			unsigned char hover_finger_x_4__11;
			unsigned char hover_finger_y_4__11;
			unsigned char hover_finger_xy_0__3;
			unsigned char hover_finger_z;
			unsigned char air_swipe_dir_0:1;
			unsigned char air_swipe_dir_1:1;
			unsigned char f1a_data3_b2__4:3;
			unsigned char object_present:1;
			unsigned char large_obj_act:2;
		} __packed;
		unsigned char proximity_data[6];
	};
#ifdef EDGE_SWIPE
	union {
		struct {
			unsigned char edge_swipe_x_lsb;
			unsigned char edge_swipe_x_msb;
			unsigned char edge_swipe_y_lsb;
			unsigned char edge_swipe_y_msb;
			unsigned char edge_swipe_z;
			unsigned char edge_swipe_wx;
			unsigned char edge_swipe_wy;
			unsigned char edge_swipe_mm;
			unsigned char edge_swipe_dg;
		} __packed;
		unsigned char edge_swipe_data[9];
	};
#endif
};

#ifdef EDGE_SWIPE
struct synaptics_rmi4_surface {
	int width_major;
	int palm;
	int angle;
	int wx;
	int wy;
};
#endif

struct synaptics_rmi4_f51_handle {
	unsigned char proximity_enables;
	unsigned short proximity_enables_addr;
#ifdef USE_CUSTOM_REZERO
	unsigned char num_of_data_sources;
	unsigned short proximity_custom_rezero_addr;
#endif
	unsigned char proximity_controls;
#ifdef EDGE_SWIPE
	struct synaptics_rmi4_surface surface_data;
#endif
	struct synaptics_rmi4_data *rmi4_data;
};
#endif

struct synaptics_rmi4_exp_fn {
	enum exp_fn fn_type;
	bool initialized;
	int (*func_init)(struct synaptics_rmi4_data *rmi4_data);
	void (*func_remove)(struct synaptics_rmi4_data *rmi4_data);
	void (*func_attn)(struct synaptics_rmi4_data *rmi4_data,
			unsigned char intr_mask);
	struct list_head link;
};

static struct device_attribute attrs[] = {
#ifdef CONFIG_POWERSUSPEND
	__ATTR(full_pm_cycle, (S_IRUGO | S_IWUSR | S_IWGRP),
			synaptics_rmi4_full_pm_cycle_show,
			synaptics_rmi4_full_pm_cycle_store),
#endif
#ifdef PROXIMITY
	__ATTR(proximity_enables, (S_IRUGO | S_IWUSR | S_IWGRP),
			synaptics_rmi4_f51_enables_show,
			synaptics_rmi4_f51_enables_store),
#endif
	__ATTR(glove_enable, (S_IRUGO | S_IWUSR | S_IWGRP),
			synaptics_rmi4_glove_enable_show,
			synaptics_rmi4_glove_enable_store),
	__ATTR(device_status, S_IRUGO,
			synaptics_rmi4_show_device_status,
			synaptics_rmi4_store_error),
	__ATTR(reset, S_IWUSR | S_IWGRP,
			synaptics_rmi4_show_error,
			synaptics_rmi4_f01_reset_store),
	__ATTR(productinfo, S_IRUGO,
			synaptics_rmi4_f01_productinfo_show,
			synaptics_rmi4_store_error),
	__ATTR(buildid, S_IRUGO,
			synaptics_rmi4_f01_buildid_show,
			synaptics_rmi4_store_error),
	__ATTR(flashprog, S_IRUGO,
			synaptics_rmi4_f01_flashprog_show,
			synaptics_rmi4_store_error),
	__ATTR(0dbutton, (S_IRUGO | S_IWUSR | S_IWGRP),
			synaptics_rmi4_0dbutton_show,
			synaptics_rmi4_0dbutton_store),
#ifdef CONFIG_KT_WAKE_FUNCS
	__ATTR(screen_wake_options, (S_IRUGO | S_IWUSR | S_IWGRP),
			synaptics_rmi4_screen_wake_options_show,
			synaptics_rmi4_screen_wake_options_store),
	__ATTR(screen_wake_options_debug, (S_IRUGO | S_IWUSR | S_IWGRP),
			synaptics_rmi4_screen_wake_options_debug_show,
			synaptics_rmi4_screen_wake_options_debug_store),
	__ATTR(screen_wake_options_hold_wlock, (S_IRUGO | S_IWUSR | S_IWGRP),
			synaptics_rmi4_screen_wake_options_hold_wlock_show,
			synaptics_rmi4_screen_wake_options_hold_wlock_store),
	__ATTR(screen_wake_options_prox_max, (S_IRUGO | S_IWUSR | S_IWGRP),
			synaptics_rmi4_screen_wake_options_prox_max_show,
			synaptics_rmi4_screen_wake_options_prox_max_store),
	__ATTR(screen_sleep_options, (S_IRUGO | S_IWUSR | S_IWGRP),
			synaptics_rmi4_screen_sleep_options_show,
			synaptics_rmi4_screen_sleep_options_store),
#endif
};

static struct list_head exp_fn_list;

#ifdef PROXIMITY
static struct synaptics_rmi4_f51_handle *f51;
#endif
#ifdef CONFIG_TOUCH_WAKE
static struct synaptics_rmi4_data *touchwake_data = NULL;
int previous_touch_state = false;
#endif
#ifdef CONFIG_KT_WAKE_FUNCS
/********** START KT wake functions **********/
static void synaptics_rmi4_release_all_finger(struct synaptics_rmi4_data *rmi4_data);
static int synaptics_rmi4_reinit_device(struct synaptics_rmi4_data *rmi4_data);
extern unsigned char get_proximity_rawdata(struct ssp_data *data);
int send_instruction(struct ssp_data *data, u8 uInst, u8 uSensorType, u8 *uSendBuf, u8 uLength);
struct ssp_data *main_prox_data;

/**
 * struct pmic8xxx_pwrkey - pmic8xxx pwrkey information
 * @key_press_irq: key press irq number
 * @pdata: platform data
 */
struct pmic8xxx_pwrkey {
	struct input_dev *pwr;
	int key_press_irq;
	u32	powerkey_state ;
	int key_release_irq;
	bool press;
	const struct pm8xxx_pwrkey_platform_data *pdata;
};

static struct device *gdev;
static bool ischarging = false;
extern void ischarging_relay(bool status);
extern void prox_max_relay(unsigned int val);
static struct wake_lock wakelock;
static struct delayed_work wakelock_monitor;
static unsigned long wakelock_time_remaining = 0;
static bool cancel_monitor_work = false;
static unsigned long last_touch_time = 0;
static unsigned int wake_start = 0;
static unsigned int x_lo;
static unsigned int y_lo;
static unsigned int x_onethird;
static unsigned int x_twothird;
static unsigned int x_hi;
static unsigned int y_hi;
static bool screen_is_off = false;
static unsigned int screen_wake_options = 0; // 0 = disabled; 1 = s2w; 2 = s2w only while charging; 3 = dtap2wake; 4 = dtap2wake only while charging; 5 = both
static unsigned int screen_wake_options_prox_max = 55;
static unsigned int screen_wake_options_debug = 0;
static unsigned int screen_wake_options_hold_wlock = 0;
static unsigned int screen_wake_options_when_off = 0;
static unsigned int screen_sleep_options = 0; // 0 = disabled; 1 = dtap2sleep
static struct pmic8xxx_pwrkey *screenwake_pwrdev;
static DEFINE_MUTEX(scr_lock);

void screenwake_setdev(struct pmic8xxx_pwrkey *input_device) {
	screenwake_pwrdev = input_device;
	return;
}
extern irqreturn_t pwrkey_press_irq(int irq, void *_pwrkey);
extern irqreturn_t pwrkey_release_irq(int irq, void *_pwrkey);

static void screenwake_presspwr(struct work_struct *screenwake_presspwr_work)
{
	/*input_event(screenwake_pwrdev, EV_KEY, KEY_POWER, 1);
	msleep(100);
	input_event(screenwake_pwrdev, EV_SYN, 0, 0);
	msleep(1000);
	input_event(screenwake_pwrdev, EV_KEY, KEY_POWER, 0);
	msleep(100);
	input_event(screenwake_pwrdev, EV_SYN, 0, 0);
	msleep(1000);*/
	//if (screen_wake_options_debug) pr_alert("POWER TRIGGER CALLED");
	//input_report_key(screenwake_pwrdev->pwr, KEY_POWER, 1);
	//input_sync(screenwake_pwrdev->pwr);
	pwrkey_press_irq(screenwake_pwrdev->key_press_irq, screenwake_pwrdev);
	msleep(200);
	pwrkey_release_irq(screenwake_pwrdev->key_release_irq, screenwake_pwrdev);
	//input_report_key(screenwake_pwrdev->pwr, KEY_POWER, 0);
	//input_sync(screenwake_pwrdev->pwr);
	//msleep(500);
	wake_start = 0;
	last_touch_time = 0;
	
	mutex_unlock(&scr_lock);
}
static DECLARE_WORK(screenwake_presspwr_work, screenwake_presspwr);

void pwr_trig_fscreen(void)
{
	if (mutex_trylock(&scr_lock)) 
		schedule_work(&screenwake_presspwr_work);
}
/********** STOP KT wake functions **********/
#endif
#ifdef CONFIG_POWERSUSPEND
static ssize_t synaptics_rmi4_full_pm_cycle_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%u\n",
			rmi4_data->full_pm_cycle);
}

static ssize_t synaptics_rmi4_full_pm_cycle_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int input;
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

	if (sscanf(buf, "%u", &input) != 1)
		return -EINVAL;

	rmi4_data->full_pm_cycle = input > 0 ? 1 : 0;

	return count;
}
#endif

#ifdef CONFIG_FAKE_DVFS
static void synaptics_change_dvfs_lock(struct synaptics_rmi4_data *rmi4_data)
{
	int retval = 0;

	mutex_lock(&rmi4_data->dvfs_lock);
	if (rmi4_data->dvfs_boost_mode == DVFS_STAGE_DUAL) {
		if (!rmi4_data->stay_awake) {
		retval = set_freq_limit(DVFS_TOUCH_ID,
				MIN_TOUCH_LIMIT_SECOND);
		rmi4_data->dvfs_freq = MIN_TOUCH_LIMIT_SECOND;
		}
	} else if (rmi4_data->dvfs_boost_mode == DVFS_STAGE_NINTH) {
		if (!rmi4_data->stay_awake) {
			retval = set_freq_limit(DVFS_TOUCH_ID,
				MIN_TOUCH_HIGH_LIMIT_SECOND);
			rmi4_data->dvfs_freq = MIN_TOUCH_HIGH_LIMIT_SECOND;
		}
	} else if (rmi4_data->dvfs_boost_mode == DVFS_STAGE_SINGLE) {
		retval = set_freq_limit(DVFS_TOUCH_ID, -1);
		rmi4_data->dvfs_freq = -1;
	}

	if (retval < 0)
		pr_debug("rmi-couldn't do the thing");
	mutex_unlock(&rmi4_data->dvfs_lock);

}

static void synaptics_set_dvfs_off(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;

	if (!rmi4_data->stay_awake) {
	mutex_lock(&rmi4_data->dvfs_lock);

	retval = set_freq_limit(DVFS_TOUCH_ID, -1);
	rmi4_data->dvfs_freq = -1;

	if (retval < 0)
		pr_debug("rmi-couldn't do the thing");
	rmi4_data->dvfs_lock_status = false;

	mutex_unlock(&rmi4_data->dvfs_lock);
}
}

static void synaptics_set_dvfs_lock(struct synaptics_rmi4_data *rmi4_data,
					int on)
{
	int ret = 0;

	if (rmi4_data->dvfs_boost_mode == DVFS_STAGE_NONE) {
		pr_debug("rmi-couldn't do the thing");
		return;
	}

	mutex_lock(&rmi4_data->dvfs_lock);
	if (on == 0) {
		if (rmi4_data->dvfs_lock_status) {
				synaptics_set_dvfs_off(rmi4_data);
		}
	} else if (on > 0) {

		if (rmi4_data->dvfs_old_stauts != on) {
			if (1/*!rmi4_data->dvfs_lock_status*/) {
				if ((rmi4_data->dvfs_freq != MIN_TOUCH_LIMIT)  &&
					(rmi4_data->dvfs_boost_mode != DVFS_STAGE_NINTH)) {
					ret = set_freq_limit(DVFS_TOUCH_ID,
							MIN_TOUCH_LIMIT);
					rmi4_data->dvfs_freq = MIN_TOUCH_LIMIT;
					if (ret < 0)
						pr_debug("rmi-couldn't do the thing");
				} else if ((rmi4_data->dvfs_freq != MIN_TOUCH_HIGH_LIMIT) &&
						(rmi4_data->dvfs_boost_mode == DVFS_STAGE_NINTH)) {
					ret = set_freq_limit(DVFS_TOUCH_ID,
								MIN_TOUCH_HIGH_LIMIT);
					rmi4_data->dvfs_freq = MIN_TOUCH_HIGH_LIMIT;
					if (ret < 0)
						pr_debug("rmi-couldn't do the thing");
				}

				synaptics_change_dvfs_lock(rmi4_data);
				rmi4_data->dvfs_lock_status = true;
			}
		}
	} else if (on < 0) {
		if (rmi4_data->dvfs_lock_status) {
			synaptics_set_dvfs_off(rmi4_data);
		}
	}
	rmi4_data->dvfs_old_stauts = on;
	mutex_unlock(&rmi4_data->dvfs_lock);
}

static void synaptics_init_dvfs(struct synaptics_rmi4_data *rmi4_data)
{
	mutex_init(&rmi4_data->dvfs_lock);

	rmi4_data->dvfs_boost_mode = DVFS_STAGE_DUAL;

	rmi4_data->dvfs_lock_status = false;
}
#endif

#ifdef PROXIMITY
static ssize_t synaptics_rmi4_f51_enables_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	unsigned char hover_enables;
	int retval;

	if (!f51)
		return -ENODEV;
	retval = synaptics_rmi4_i2c_read(f51->rmi4_data,
			f51->proximity_enables_addr,
			&hover_enables,
			sizeof(hover_enables));

	return snprintf(buf, PAGE_SIZE, "0x%02x\n",
			f51->proximity_enables);
}

static ssize_t synaptics_rmi4_f51_enables_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned int input;
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

	if (!f51)
		return -ENODEV;

	if (sscanf(buf, "%x", &input) != 1)
		return -EINVAL;

	f51->proximity_enables = (unsigned char)input;

	retval = synaptics_rmi4_i2c_write(rmi4_data,
			f51->proximity_enables_addr,
			&f51->proximity_enables,
			sizeof(f51->proximity_enables));
	if (retval < 0) {
		pr_debug("rmi-couldn't do the thing");
		return retval;
	}

	return count;
}
#endif
#ifdef CONFIG_KT_WAKE_FUNCS

static void set_wakelock_options(bool setWork)
{
	if (!wake_lock_active(&wakelock) && screen_is_off)
	{
		if (screen_wake_options_hold_wlock == 1 || (screen_wake_options_hold_wlock == 3 && ischarging) || (screen_wake_options_hold_wlock >= 4 && screen_wake_options_hold_wlock <= 11))
		{
			wakelock_time_remaining = 0;
			if (screen_wake_options_hold_wlock == 1 || (screen_wake_options_hold_wlock == 3 && ischarging))
				wake_lock(&wakelock);
			else if (screen_wake_options_hold_wlock == 4)
				wakelock_time_remaining = 30000;
			else if (screen_wake_options_hold_wlock == 5)
				wakelock_time_remaining = 60000;
			else if (screen_wake_options_hold_wlock == 6)
				wakelock_time_remaining = 120000;
			else if (screen_wake_options_hold_wlock == 7)
				wakelock_time_remaining = 300000;
			else if (screen_wake_options_hold_wlock == 8)
				wakelock_time_remaining = 600000;
			else if (screen_wake_options_hold_wlock == 9)
				wakelock_time_remaining = 1800000;
			else if (screen_wake_options_hold_wlock == 10)
				wakelock_time_remaining = 3600000;
			else if (screen_wake_options_hold_wlock == 11)
				wakelock_time_remaining = 7200000;
			
			if (wakelock_time_remaining)
				wake_lock_timeout(&wakelock, msecs_to_jiffies(wakelock_time_remaining));
			if (setWork)
			{
				schedule_delayed_work_on(0, &wakelock_monitor, msecs_to_jiffies(5000));
				cancel_monitor_work = false;
			}
		}
	}
}

static void wakelock_monitor_func(struct work_struct *work)
{
	int did_jumpstart = 0;
	bool wloc_active = wake_lock_active(&wakelock);
	long timeout = wakelock.expires - jiffies;

	if (wakelock_time_remaining >= 5000)
		wakelock_time_remaining -= 5000;
	else if (wakelock_time_remaining < 5000)
		wakelock_time_remaining = 0;
	
	if (screen_is_off && !cancel_monitor_work)
	{
		if (!wloc_active && (screen_wake_options_hold_wlock == 1 || (screen_wake_options_hold_wlock == 2 && !cancel_monitor_work) || (screen_wake_options_hold_wlock == 3 && ischarging && !cancel_monitor_work)))
		{
			wake_lock(&wakelock);
			did_jumpstart = 1;
		}
		else if (screen_wake_options_hold_wlock >= 4)
		{
			if (!wloc_active && wakelock_time_remaining > 0)
			{
				wake_lock_timeout(&wakelock, msecs_to_jiffies(wakelock_time_remaining));
				did_jumpstart = 2;
			}
			else if (wakelock_time_remaining <= 0)
				cancel_monitor_work = true;
		}
	}

	if (screen_wake_options_debug) pr_alert("KT WAKE MONITOR: LockActive-%d TimeRemain-%ld Expires-%ld CancelWork-%d Jumpstart-%s\n", wloc_active, wakelock_time_remaining, timeout, cancel_monitor_work, did_jumpstart == 1 ? "Timed mode" : did_jumpstart == 2 ? "Permanent mode" : "NONE");

	if (screen_is_off && !cancel_monitor_work)
		schedule_delayed_work_on(0, &wakelock_monitor, msecs_to_jiffies(5000));
	if (cancel_monitor_work || !screen_is_off)
	{
		cancel_monitor_work = false;
		wakelock_time_remaining = 0;
	}
}

static void check_options_while_soff(struct device *dev)
{
	set_wakelock_options(true);
	if (screen_wake_options && !screen_wake_options_when_off && screen_is_off)
	{
		int retval;
		struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);
		char chTempbuf[2] = { 1, 20};
		retval = gpio_request(rmi4_data->board->gpio, "tsp_int");
		if (retval != 0) {
			dev_info(&rmi4_data->i2c_client->dev, "%s: tsp int request failed, ret=%d", __func__, retval);
		}
		if (!rmi4_data->irq_enabled)
		{
			enable_irq(rmi4_data->i2c_client->irq);
			rmi4_data->irq_enabled = true;
		}
		
		if (main_prox_data != NULL)
		{
			send_instruction(main_prox_data, ADD_SENSOR, PROXIMITY_RAW, chTempbuf, 2);
			main_prox_data->bProximityRawEnabled = true;
		}

		enable_irq_wake(rmi4_data->i2c_client->irq);
		
		retval = synaptics_rmi4_reset_device(rmi4_data);
		if (retval < 0) {
			dev_err(dev,
					"%s: Failed to issue reset command, error = %d\n",
					__func__, retval);
		}
		
		screen_wake_options_when_off = screen_wake_options;
	}
}

static ssize_t synaptics_rmi4_screen_wake_options_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret;
	ret = sprintf(buf, "%d\n", screen_wake_options);
	return ret;
}
static ssize_t synaptics_rmi4_screen_wake_options_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned int val = 0;

	retval = sscanf(buf, "%d", &val);
	if (retval != 0 && val >= 0 && val <= 6) {
		screen_wake_options = val;
	}
	check_options_while_soff(dev);

	return count;
}

static ssize_t synaptics_rmi4_screen_wake_options_prox_max_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret;
	ret = sprintf(buf, "%d\n", screen_wake_options_prox_max);
	return ret;
}
static ssize_t synaptics_rmi4_screen_wake_options_prox_max_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned int val = 0;

	retval = sscanf(buf, "%d", &val);
	if (retval != 0 && val >= 0 && val <= 255) {
		screen_wake_options_prox_max = val;
		prox_max_relay(val);
	}

	return count;
}

static ssize_t synaptics_rmi4_screen_wake_options_debug_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret;
	ret = sprintf(buf, "%d\n", screen_wake_options_debug);
	return ret;
}
static ssize_t synaptics_rmi4_screen_wake_options_debug_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned int val = 0;

	retval = sscanf(buf, "%d", &val);
	if (retval != 0 && val >= 0 && val <= 6) {
		screen_wake_options_debug = val;
	}
	return count;
}

static ssize_t synaptics_rmi4_screen_wake_options_hold_wlock_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret;
	ret = sprintf(buf, "%d\n", screen_wake_options_hold_wlock);
	return ret;
}
static ssize_t synaptics_rmi4_screen_wake_options_hold_wlock_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned int val = 0;

	retval = sscanf(buf, "%d", &val);
	if (retval != 0 && val >= 0 && val <= 19) {
		screen_wake_options_hold_wlock = val;
	}
	check_options_while_soff(dev);
	return count;

}

static ssize_t synaptics_rmi4_screen_sleep_options_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret;
	ret = sprintf(buf, "%d\n", screen_sleep_options);
	return ret;
}
static ssize_t synaptics_rmi4_screen_sleep_options_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned int val = 0;

	retval = sscanf(buf, "%d", &val);
	if (retval != 0 && val >= 0 && val <= 6) {
		screen_sleep_options = val;
	}
	return count;
}
#endif
#ifdef CONFIG_GLOVE_TOUCH
static ssize_t synaptics_rmi4_glove_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);
	unsigned char glove_enable = 0;
	int retval;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->glove_touch_addr,
			&glove_enable,
			sizeof(glove_enable));

	return snprintf(buf, PAGE_SIZE, "glove touch enable =0x %x\n",
			glove_enable);
}

int synaptics_rmi4_glove_mode_enables(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;

	if (GLOVE_FEATURE_EN != rmi4_data->glove_mode_feature) {
		pr_debug("rmi-couldn't do the thing");
		return 0;
	}

	retval = synaptics_rmi4_i2c_write(rmi4_data,
			rmi4_data->glove_mode_enables_addr,
			&rmi4_data->glove_mode_enables,
			sizeof(rmi4_data->glove_mode_enables));
	if (retval < 0)
		return retval;

	return 0;
}

static int synaptics_rmi4_glove_mode_set(struct synaptics_rmi4_data *rmi4_data,
		int mode, int enable)
{
	int retval = 0;
	unsigned char glove_enable = 0;

	if (rmi4_data->panel_revision < OCTA_PANEL_REVISION_43) {
		pr_debug("rmi-couldn't do the thing");
		return 0;
	}

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->glove_touch_addr,
			&glove_enable,
			sizeof(glove_enable));

	if (retval <= 0)
		glove_enable = rmi4_data->glove_mode_enables;

	if (mode == 0x01) {
		if (enable)
			glove_enable |= 0x01;
		else
			glove_enable &= 0xFE;
	} else if (mode == 0x02) {
		if (enable)
			glove_enable |= 0x02;
		else
			glove_enable &= 0xFD;
	}

	retval = synaptics_rmi4_i2c_write(rmi4_data,
			rmi4_data->glove_touch_addr,
			&glove_enable,
			sizeof(glove_enable));

	rmi4_data->glove_mode_enables = glove_enable;

	return retval;

}

static ssize_t synaptics_rmi4_glove_enable_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);
	unsigned int input;
	int retval;

	if (sscanf(buf, "%x", &input) != 1)
		return -EINVAL;

	retval = synaptics_rmi4_glove_mode_set(rmi4_data, 1, input);
	if (retval < 0)
		pr_debug("rmi-couldn't do the thing");

	return count;
}

static ssize_t synaptics_rmi4_show_device_status(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);
	int retval;
	unsigned char device_status = 0;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->f01_ctrl_base_addr,
			&device_status,
			sizeof(device_status));

	return snprintf(buf, PAGE_SIZE, "device_status = %x\n",
			device_status);
}
#endif

static ssize_t synaptics_rmi4_f01_reset_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned int reset;
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

	if (sscanf(buf, "%u", &reset) != 1)
		return -EINVAL;

	if (reset != 1)
		return -EINVAL;

	retval = synaptics_rmi4_reset_device(rmi4_data);
	if (retval < 0) {
		pr_debug("rmi-couldn't do the thing");
		return retval;
	}

	return count;
}

static ssize_t synaptics_rmi4_f01_productinfo_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "0x%02x 0x%02x\n",
			(rmi4_data->rmi4_mod_info.product_info[0]),
			(rmi4_data->rmi4_mod_info.product_info[1]));
}

static ssize_t synaptics_rmi4_f01_buildid_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	unsigned int build_id;
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);
	struct synaptics_rmi4_device_info *rmi;

	rmi = &(rmi4_data->rmi4_mod_info);

	build_id = (unsigned int)rmi->build_id[0] +
			(unsigned int)rmi->build_id[1] * 0x100 +
			(unsigned int)rmi->build_id[2] * 0x10000;

	return snprintf(buf, PAGE_SIZE, "%u\n",
			build_id);
}

static ssize_t synaptics_rmi4_f01_flashprog_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int retval;
	struct synaptics_rmi4_f01_device_status device_status;
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->f01_data_base_addr,
			device_status.data,
			sizeof(device_status.data));
	if (retval < 0)
		return retval;

	return snprintf(buf, PAGE_SIZE, "%u\n",
			device_status.flash_prog);
}

static ssize_t synaptics_rmi4_0dbutton_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%u\n",
			rmi4_data->button_0d_enabled);
}

static ssize_t synaptics_rmi4_0dbutton_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned int input;
	unsigned char ii;
	unsigned char intr_enable;
	struct synaptics_rmi4_fn *fhandler;
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);
	struct synaptics_rmi4_device_info *rmi;

	rmi = &(rmi4_data->rmi4_mod_info);

	if (sscanf(buf, "%u", &input) != 1)
		return -EINVAL;

	input = input > 0 ? 1 : 0;

	if (rmi4_data->button_0d_enabled == input)
		return count;

	if (list_empty(&rmi->support_fn_list))
		return -ENODEV;

	list_for_each_entry(fhandler, &rmi->support_fn_list, link) {
		if (fhandler->fn_number == SYNAPTICS_RMI4_F1A) {
			ii = fhandler->intr_reg_num;

			retval = synaptics_rmi4_i2c_read(rmi4_data,
					rmi4_data->f01_ctrl_base_addr + 1 + ii,
					&intr_enable,
					sizeof(intr_enable));
			if (retval < 0)
				return retval;

			if (input == 1)
				intr_enable |= fhandler->intr_mask;
			else
				intr_enable &= ~fhandler->intr_mask;

			retval = synaptics_rmi4_i2c_write(rmi4_data,
					rmi4_data->f01_ctrl_base_addr + 1 + ii,
					&intr_enable,
					sizeof(intr_enable));
			if (retval < 0)
				return retval;
		}
	}

	rmi4_data->button_0d_enabled = input;

	return count;
}

 /**
 * synaptics_rmi4_set_page()
 *
 * Called by synaptics_rmi4_i2c_read() and synaptics_rmi4_i2c_write().
 *
 * This function writes to the page select register to switch to the
 * assigned page.
 */
static int synaptics_rmi4_set_page(struct synaptics_rmi4_data *rmi4_data,
		unsigned int address)
{
	int retval = 0;
	unsigned char retry;
	unsigned char buf[PAGE_SELECT_LEN];
	unsigned char page;
	struct i2c_client *i2c = rmi4_data->i2c_client;

	page = ((address >> 8) & MASK_8BIT);
	if (page != rmi4_data->current_page) {
		buf[0] = MASK_8BIT;
		buf[1] = page;
		for (retry = 0; retry < SYN_I2C_RETRY_TIMES; retry++) {
			retval = i2c_master_send(i2c, buf, PAGE_SELECT_LEN);
			if (retval != PAGE_SELECT_LEN) {
				pr_debug("rmi-couldn't do the thing");
				msleep(20);
			} else {
				rmi4_data->current_page = page;
				break;
			}
		}
	} else {
		retval = PAGE_SELECT_LEN;
	}

	return retval;
}

 /**
 * synaptics_rmi4_i2c_read()
 *
 * Called by various functions in this driver, and also exported to
 * other expansion Function modules such as rmi_dev.
 *
 * This function reads data of an arbitrary length from the sensor,
 * starting from an assigned register address of the sensor, via I2C
 * with a retry mechanism.
 */
static int synaptics_rmi4_i2c_read(struct synaptics_rmi4_data *rmi4_data,
		unsigned short addr, unsigned char *data, unsigned short length)
{
	int retval;
	unsigned char retry;
	unsigned char buf;
	struct i2c_msg msg[] = {
		{
			.addr = rmi4_data->i2c_client->addr,
			.flags = 0,
			.len = 1,
			.buf = &buf,
		},
		{
			.addr = rmi4_data->i2c_client->addr,
			.flags = I2C_M_RD,
			.len = length,
			.buf = data,
		},
	};

	buf = addr & MASK_8BIT;

	mutex_lock(&(rmi4_data->rmi4_io_ctrl_mutex));

	if (rmi4_data->touch_stopped) {
		pr_debug("rmi-couldn't do the thing");
		retval = 0;
		goto exit;
	}

	retval = synaptics_rmi4_set_page(rmi4_data, addr);
	if (retval != PAGE_SELECT_LEN)
		goto exit;

	for (retry = 0; retry < SYN_I2C_RETRY_TIMES; retry++) {
		if (i2c_transfer(rmi4_data->i2c_client->adapter, msg, 2) == 2) {
			retval = length;
			break;
		}
		pr_debug("rmi-couldn't do the thing");
		msleep(20);
	}

	if (retry == SYN_I2C_RETRY_TIMES) {
		pr_debug("rmi-couldn't do the thing");
		retval = -EIO;
	}

exit:
	mutex_unlock(&(rmi4_data->rmi4_io_ctrl_mutex));

	return retval;
}

 /**
 * synaptics_rmi4_i2c_write()
 *
 * Called by various functions in this driver, and also exported to
 * other expansion Function modules such as rmi_dev.
 *
 * This function writes data of an arbitrary length to the sensor,
 * starting from an assigned register address of the sensor, via I2C with
 * a retry mechanism.
 */
static int synaptics_rmi4_i2c_write(struct synaptics_rmi4_data *rmi4_data,
		unsigned short addr, unsigned char *data, unsigned short length)
{
	int retval;
	unsigned char retry;
	unsigned char buf[length + 1];
	struct i2c_msg msg[] = {
		{
			.addr = rmi4_data->i2c_client->addr,
			.flags = 0,
			.len = length + 1,
			.buf = buf,
		}
	};

	mutex_lock(&(rmi4_data->rmi4_io_ctrl_mutex));

	if (rmi4_data->touch_stopped) {
		pr_debug("rmi-couldn't do the thing");
		retval = 0;
		goto exit;
	}

	retval = synaptics_rmi4_set_page(rmi4_data, addr);
	if (retval != PAGE_SELECT_LEN)
		goto exit;

	buf[0] = addr & MASK_8BIT;
	memcpy(&buf[1], &data[0], length);

	for (retry = 0; retry < SYN_I2C_RETRY_TIMES; retry++) {
		if (i2c_transfer(rmi4_data->i2c_client->adapter, msg, 1) == 1) {
			retval = length;
			break;
		}
		pr_debug("rmi-couldn't do the thing");
		msleep(20);
	}

	if (retry == SYN_I2C_RETRY_TIMES) {
		pr_debug("rmi-couldn't do the thing");
		retval = -EIO;
	}

exit:
	mutex_unlock(&(rmi4_data->rmi4_io_ctrl_mutex));

	return retval;
}

 /**
 * synaptics_rmi4_f11_abs_report()
 *
 * Called by synaptics_rmi4_report_touch() when valid Function $11
 * finger data has been detected.
 *
 * This function reads the Function $11 data registers, determines the
 * status of each finger supported by the Function, processes any
 * necessary coordinate manipulation, reports the finger data to
 * the input subsystem, and returns the number of fingers detected.
 */
static int synaptics_rmi4_f11_abs_report(struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn *fhandler)
{
	int retval;
	unsigned char touch_count = 0; /* number of touch points */
	unsigned char reg_index;
	unsigned char finger;
	unsigned char fingers_supported;
	unsigned char num_of_finger_status_regs;
	unsigned char finger_shift;
	unsigned char finger_status;
	unsigned char data_reg_blk_size;
	unsigned char finger_status_reg[3];
	unsigned char data[F11_STD_DATA_LEN];
	unsigned short data_addr;
	unsigned short data_offset;
	int x;
	int y;
	int wx;
	int wy;

	/*
	 * The number of finger status registers is determined by the
	 * maximum number of fingers supported - 2 bits per finger. So
	 * the number of finger status registers to read is:
	 * register_count = ceil(max_num_of_fingers / 4)
	 */
	fingers_supported = fhandler->num_of_data_points;
	num_of_finger_status_regs = (fingers_supported + 3) / 4;
	data_addr = fhandler->full_addr.data_base;
	data_reg_blk_size = fhandler->size_of_data_register_block;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			data_addr,
			finger_status_reg,
			num_of_finger_status_regs);
	if (retval < 0)
		return 0;

	for (finger = 0; finger < fingers_supported; finger++) {
		reg_index = finger / 4;
		finger_shift = (finger % 4) * 2;
		finger_status = (finger_status_reg[reg_index] >> finger_shift)
				& MASK_2BIT;

		/*
		 * Each 2-bit finger status field represents the following:
		 * 00 = finger not present
		 * 01 = finger present and data accurate
		 * 10 = finger present but data may be inaccurate
		 * 11 = reserved
		 */
#ifdef TYPE_B_PROTOCOL
		input_mt_slot(rmi4_data->input_dev, finger);
		input_mt_report_slot_state(rmi4_data->input_dev,
				MT_TOOL_FINGER, finger_status);
#endif

		if (finger_status) {
			data_offset = data_addr +
					num_of_finger_status_regs +
					(finger * data_reg_blk_size);
			retval = synaptics_rmi4_i2c_read(rmi4_data,
					data_offset,
					data,
					data_reg_blk_size);
			if (retval < 0)
				return 0;

			x = (data[0] << 4) | (data[2] & MASK_4BIT);
			y = (data[1] << 4) | ((data[2] >> 4) & MASK_4BIT);
			wx = (data[3] & MASK_4BIT);
			wy = (data[3] >> 4) & MASK_4BIT;

			if (rmi4_data->board->x_flip)
				x = rmi4_data->sensor_max_x - x;
			if (rmi4_data->board->y_flip)
				y = rmi4_data->sensor_max_y - y;

			input_report_key(rmi4_data->input_dev,
					BTN_TOUCH, 1);
			input_report_key(rmi4_data->input_dev,
					BTN_TOOL_FINGER, 1);
			input_report_abs(rmi4_data->input_dev,
					ABS_MT_POSITION_X, x);
			input_report_abs(rmi4_data->input_dev,
					ABS_MT_POSITION_Y, y);
#ifdef REPORT_2D_W
			input_report_abs(rmi4_data->input_dev,
					ABS_MT_TOUCH_MAJOR, max(wx, wy));
			input_report_abs(rmi4_data->input_dev,
					ABS_MT_TOUCH_MINOR, min(wx, wy));
#endif
#ifndef TYPE_B_PROTOCOL
			input_mt_sync(rmi4_data->input_dev);
#endif
			touch_count++;
		}
	}

#ifndef TYPE_B_PROTOCOL
	if (touch_count == 0) {
		input_mt_sync(rmi4_data->input_dev);
	}
#endif

	input_sync(rmi4_data->input_dev);

	return touch_count;
}

 /**
 * synaptics_rmi4_f12_abs_report()
 *
 * Called by synaptics_rmi4_report_touch() when valid Function $12
 * finger data has been detected.
 *
 * This function reads the Function $12 data registers, determines the
 * status of each finger supported by the Function, processes any
 * necessary coordinate manipulation, reports the finger data to
 * the input subsystem, and returns the number of fingers detected.
 */
static int synaptics_rmi4_f12_abs_report(struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn *fhandler)
{
	int retval;
	unsigned char touch_count = 0; /* number of touch points */
	unsigned char finger;
	unsigned char fingers_supported;
	unsigned char finger_status;
	unsigned short data_addr;
	int x;
	int y;
	int wx;
	int wy;
	struct synaptics_rmi4_f12_finger_data *data;
	struct synaptics_rmi4_f12_finger_data *finger_data;

	fingers_supported = fhandler->num_of_data_points;
	data_addr = fhandler->full_addr.data_base;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			data_addr + fhandler->data1_offset,
			(unsigned char *)fhandler->data,
			fhandler->data_size);
	if (retval < 0) {
#ifdef CONFIG_KT_WAKE_FUNCS
		synaptics_rmi4_release_all_finger(rmi4_data);
#endif
		return 0;
}

	data = (struct synaptics_rmi4_f12_finger_data *)fhandler->data;

	for (finger = 0; finger < fingers_supported; finger++) {
		finger_data = data + finger;
		finger_status = finger_data->object_type_and_status;

		/*
		 * Each 3-bit finger status field represents the following:
		 * 000 = finger not present
		 * 001 = finger present and data accurate
		 * 010 = finger present but data may be inaccurate
		 * 011 = palm
		 * 100 = glove touch
		 */
/* if reject finger status 0x03, do not work screen capture. */
/*
		if (0x03 == finger_status)
			continue;
*/
#ifdef TYPE_B_PROTOCOL
		input_mt_slot(rmi4_data->input_dev, finger);
		input_mt_report_slot_state(rmi4_data->input_dev,
				MT_TOOL_FINGER, finger_status);
#endif

		if (finger_status) {
#ifdef CONFIG_GLOVE_TOUCH
			if ((finger_status == 0x06) &&
				!rmi4_data->touchkey_glove_mode_status) {
				rmi4_data->touchkey_glove_mode_status = true;
				input_report_switch(rmi4_data->input_dev,
					SW_GLOVE, true);
			} else if ((finger_status != 0x06) &&
				rmi4_data->touchkey_glove_mode_status) {
				rmi4_data->touchkey_glove_mode_status = false;
				input_report_switch(rmi4_data->input_dev,
					SW_GLOVE, false);
			}
#endif
			x = (finger_data->x_msb << 8) | (finger_data->x_lsb);
			y = (finger_data->y_msb << 8) | (finger_data->y_lsb);
#ifdef REPORT_2D_W
			wx = finger_data->wx;
			wy = finger_data->wy;
#ifdef EDGE_SWIPE
			if (f51) {
				if ((f51->proximity_controls & HAS_EDGE_SWIPE)){
//					&& f51->surface_data.palm) {					Delta merge from JBP, for palm swipe
					wx = f51->surface_data.wx;
					wy = f51->surface_data.wy;
				}
			}
#endif
#endif
			if (rmi4_data->board->x_flip)
				x = rmi4_data->sensor_max_x - x;
			if (rmi4_data->board->y_flip)
				y = rmi4_data->sensor_max_y - y;

			input_report_key(rmi4_data->input_dev,
					BTN_TOUCH, 1);
			input_report_key(rmi4_data->input_dev,
					BTN_TOOL_FINGER, 1);
			input_report_abs(rmi4_data->input_dev,
					ABS_MT_POSITION_X, x);
			input_report_abs(rmi4_data->input_dev,
					ABS_MT_POSITION_Y, y);
#ifdef REPORT_2D_W
			input_report_abs(rmi4_data->input_dev,
					ABS_MT_TOUCH_MAJOR, max(wx, wy));
			input_report_abs(rmi4_data->input_dev,
					ABS_MT_TOUCH_MINOR, min(wx, wy));
#endif
#ifdef EDGE_SWIPE
			if (f51) {
				if (f51->proximity_controls & HAS_EDGE_SWIPE) {
					input_report_abs(rmi4_data->input_dev,
							ABS_MT_WIDTH_MAJOR, f51->surface_data.width_major);
					/*input_report_abs(rmi4_data->input_dev,
							ABS_MT_ANGLE, f51->surface_data.angle); */
					input_report_abs(rmi4_data->input_dev,
							ABS_MT_PALM, f51->surface_data.palm);
				}
			}
#endif
#ifndef TYPE_B_PROTOCOL
			input_mt_sync(rmi4_data->input_dev);
#endif
#ifdef CONFIG_KT_WAKE_FUNCS
			
			if (screen_is_off && screen_wake_options)
			{
				unsigned char prox;
				if (main_prox_data != NULL)
				{
					prox = main_prox_data->buf[PROXIMITY_RAW].prox[0];//get_proximity_rawdata(main_prox_data);
				}
				if (prox <= screen_wake_options_prox_max && !call_in_progress)
					{
					//sweep2wake
					if ((screen_wake_options == 1 || (screen_wake_options == 2 && ischarging) || screen_wake_options == 5 || (screen_wake_options == 6 && ischarging)) && !rmi4_data->finger[finger].state)
					{
						if (screen_wake_options_debug) pr_alert("WAKE_START TOUCH %d-%d-%d\n", x, x_lo, x_hi);
						//Left to right
						if (x < x_lo)
							wake_start = 1;
						//Right to left
						if (x > x_hi)
							wake_start = 4;
					}	
					if ((screen_wake_options == 1 || (screen_wake_options == 2 && ischarging) || screen_wake_options == 5 || (screen_wake_options == 6 && ischarging)) && rmi4_data->finger[finger].state)
					{
						//Left to right
						if (wake_start == 1 && x >= (x_onethird-30) && x <= (x_onethird+30))
						{
							wake_start = 2;
							if (screen_wake_options_debug) pr_alert("WAKE_START ON2 %d-%d\n", x, x_lo);
						}
						if (wake_start == 2 && x >= (x_twothird-30) && x <= (x_twothird+30))
						{
							wake_start = 3;
							if (screen_wake_options_debug) pr_alert("WAKE_START ON3 %d-%d\n", x, x_lo);
						}
						if (wake_start == 3 && x > x_hi) {
							pwr_trig_fscreen();
							if (screen_wake_options_debug) pr_alert("WAKE_START OFF-1 %d-%d\n", x, x_hi);
						}				
						//Right to left
						if (wake_start == 4 && x >= (x_twothird-30) && x <= (x_twothird+30))
						{
							wake_start = 5;
							if (screen_wake_options_debug) pr_alert("WAKE_START ON3 %d-%d\n", x, x_lo);
						}
						if (wake_start == 5 && x >= (x_onethird-30) && x <= (x_onethird+30))
						{
							wake_start = 6;
							if (screen_wake_options_debug) pr_alert("WAKE_START ON2 %d-%d\n", x, x_lo);
						}
						if (wake_start == 6 && x < x_lo) {
							pwr_trig_fscreen();
							if (screen_wake_options_debug) pr_alert("WAKE_START OFF-1 %d-%d\n", x, x_hi);
						}				
					}
					//Double Tap 2 wake
					if ((screen_wake_options == 3 || (screen_wake_options == 4 && ischarging) || screen_wake_options == 5 || (screen_wake_options == 6 && ischarging)) && !rmi4_data->finger[finger].state)
					{
						bool block_store = false;
						if (last_touch_time)
						{
							if (screen_wake_options_debug) pr_alert("DOUBLE TAP WAKE TOUCH %d-%d-%ld-%ld-%d\n", x, y, jiffies, last_touch_time, touch_count);
							if (!touch_count && jiffies_to_msecs(jiffies - last_touch_time) < 2000) //(x < x_lo) && (y > y_hi) && //jiffies_to_msecs(jiffies - last_touch_time) > 50
							{
								if (screen_wake_options_debug) pr_alert("DOUBLE TAP WAKE POWER BTN CALLED %d-%d\n", x, y);
								pwr_trig_fscreen();
								last_touch_time = 0;
								block_store = true;
							}
							else
							{
								if (screen_wake_options_debug) pr_alert("DOUBLE TAP WAKE DELETE %d-%d-%ld-%ld\n", x, y, jiffies, last_touch_time);
								last_touch_time = 0;
								block_store = true;
							}
						}
						if (!last_touch_time && !block_store)
							last_touch_time = jiffies;
					}
				}
			}
			if (!screen_is_off)
			{
				//Double Tap 2 Sleep
				if (screen_sleep_options == 1 && !rmi4_data->finger[finger].state)
				{
					bool block_store = false;
					if (last_touch_time)
					{
						if (screen_wake_options_debug) pr_alert("DOUBLE TAP SLEEP TOUCH %d-%d-%ld-%ld-%d\n", x, y, jiffies, last_touch_time, touch_count);
						if (!touch_count && (y < 100) && jiffies_to_msecs(jiffies - last_touch_time) < 1000) //(x < x_lo) && (y > y_hi) && //jiffies_to_msecs(jiffies - last_touch_time) > 50
						{
							if (screen_wake_options_debug) pr_alert("DOUBLE TAP SLEEP POWER BTN CALLED %d-%d\n", x, y);
							pwr_trig_fscreen();
							last_touch_time = 0;
							block_store = true;
						}
						else
						{
							if (screen_wake_options_debug) pr_alert("DOUBLE TAP SLEEP DELETE %d-%d-%ld-%ld\n", x, y, jiffies, last_touch_time);
							last_touch_time = 0;
							block_store = true;
						}
					}
					if (!last_touch_time && !block_store && (y < 100))
						last_touch_time = jiffies;
				}
			}
#endif

			if (rmi4_data->finger[finger].state)
				rmi4_data->finger[finger].mcount++;

			touch_count++;
		}

		if (rmi4_data->finger[finger].state && !finger_status) {
#ifdef CONFIG_KT_WAKE_FUNCS
			wake_start = 0;
#endif
			rmi4_data->finger[finger].mcount = 0;
		}

		rmi4_data->finger[finger].state = finger_status;
	}


	if (touch_count == 0) {
		/* Clear BTN_TOUCH when All touch are released  */
		input_report_key(rmi4_data->input_dev,
				BTN_TOUCH, 0);
#ifndef TYPE_B_PROTOCOL
		input_mt_sync(rmi4_data->input_dev);
#endif
	}

	input_sync(rmi4_data->input_dev);

#ifdef CONFIG_FAKE_DVFS
	if (touch_count)
		synaptics_set_dvfs_lock(rmi4_data, touch_count);
	else
		synaptics_set_dvfs_lock(rmi4_data, 0);
#endif

#ifdef CONFIG_TOUCH_WAKE
	// Read touchscreen digitizer
	if (touchwake_is_active() && touch_count > 0) {
		if (previous_touch_state == false) {
			#ifdef TOUCHWAKE_DEBUG_PRINT
			pr_info("[TOUCHWAKE] Synaptics pressed\n");
			#endif
			touch_press(); // Yank555.lu - Screen touched
			previous_touch_state = true; // once is enough
		}
	} else {
		previous_touch_state = false; // touch released, restart listening
	}
#endif

	return touch_count;
}

static void synaptics_rmi4_f1a_report(struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn *fhandler)
{
	int retval;
	unsigned char touch_count = 0;
	unsigned char button;
	unsigned char index;
	unsigned char shift;
	unsigned char status;
	unsigned char *data;
	unsigned short data_addr = fhandler->full_addr.data_base;
	struct synaptics_rmi4_f1a_handle *f1a = fhandler->data;
	static unsigned char do_once = 1;
	static bool current_status[MAX_NUMBER_OF_BUTTONS];
#ifdef NO_0D_WHILE_2D
	static bool before_2d_status[MAX_NUMBER_OF_BUTTONS];
	static bool while_2d_status[MAX_NUMBER_OF_BUTTONS];
#endif

	if (do_once) {
		memset(current_status, 0, sizeof(current_status));
#ifdef NO_0D_WHILE_2D
		memset(before_2d_status, 0, sizeof(before_2d_status));
		memset(while_2d_status, 0, sizeof(while_2d_status));
#endif
		do_once = 0;
	}

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			data_addr,
			f1a->button_data_buffer,
			f1a->button_bitmask_size);
	if (retval < 0) {
		pr_debug("rmi-couldn't do the thing");
		return;
	}

	data = f1a->button_data_buffer;

	for (button = 0; button < f1a->valid_button_count; button++) {
		index = button / 8;
		shift = button % 8;
		status = ((data[index] >> shift) & MASK_1BIT);

		if (current_status[button] == status)
			continue;
		else
			current_status[button] = status;

#ifdef NO_0D_WHILE_2D
		if (rmi4_data->fingers_on_2d == false) {
			if (status == 1) {
				before_2d_status[button] = 1;
			} else {
				if (while_2d_status[button] == 1) {
					while_2d_status[button] = 0;
					continue;
				} else {
					before_2d_status[button] = 0;
				}
			}
			touch_count++;
			input_report_key(rmi4_data->input_dev,
					f1a->button_map[button],
					status);
		} else {
			if (before_2d_status[button] == 1) {
				before_2d_status[button] = 0;
				touch_count++;
				input_report_key(rmi4_data->input_dev,
						f1a->button_map[button],
						status);
			} else {
				if (status == 1)
					while_2d_status[button] = 1;
				else
					while_2d_status[button] = 0;
			}
		}
#else
		touch_count++;
		input_report_key(rmi4_data->input_dev,
				f1a->button_map[button],
				status);
#endif
	}

	if (touch_count)
		input_sync(rmi4_data->input_dev);

	return;
}

#ifdef PROXIMITY
#ifdef EDGE_SWIPE
static int synaptics_rmi4_f51_edge_swipe(struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn *fhandler)
{
	int retval;
	unsigned short data_base_addr;
	struct synaptics_rmi4_f51_data *data;

	data_base_addr = fhandler->full_addr.data_base;
	data = (struct synaptics_rmi4_f51_data *)fhandler->data;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			data_base_addr + EDGE_SWIPE_DATA_OFFSET,
			data->edge_swipe_data,
			sizeof(data->edge_swipe_data));
	if (retval < 0) {
#ifdef CONFIG_KT_WAKE_FUNCS
		return retval;
#endif
	}
	if (!f51)
		return -ENODEV;

	/*if (data->edge_swipe_dg >= 90 && data->edge_swipe_dg <= 180)
#if defined(CONFIG_MACH_JACTIVE_EUR)
		f51->surface_data.angle = data->edge_swipe_dg - 90;
#else
		f51->surface_data.angle = data->edge_swipe_dg - 180;
#endif
	else if (data->edge_swipe_dg < 90)
#if defined(CONFIG_MACH_JACTIVE_EUR) || defined(CONFIG_MACH_JACTIVE_ATT)
		f51->surface_data.angle = 90 - data->edge_swipe_dg;
#else
		f51->surface_data.angle = data->edge_swipe_dg;
#endif
	else
		dev fuckmyself(&rmi4_data->i2c_client->dev,
				"Skip wrong edge swipe angle [%d]\n",
				data->edge_swipe_dg);
*/
	f51->surface_data.width_major = data->edge_swipe_mm;
	f51->surface_data.wx = data->edge_swipe_wx;
	f51->surface_data.wy = data->edge_swipe_wy;
	f51->surface_data.palm = data->edge_swipe_z;

	return retval;
}
#endif

static void synaptics_rmi4_f51_report(struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn *fhandler)
{
	int retval;
	unsigned short data_base_addr;
	int x;
	int y;
	int z;
	struct synaptics_rmi4_f51_data *data;

#ifdef EDGE_SWIPE
	/* Read edge swipe data */
	if (f51->proximity_controls & HAS_EDGE_SWIPE) {
		retval = synaptics_rmi4_f51_edge_swipe(rmi4_data, fhandler);
		if (retval < 0) {
#ifdef CONFIG_KT_WAKE_FUNCS
			synaptics_rmi4_release_all_finger(rmi4_data);
#endif
			return;
		}
	}
#endif

	data_base_addr = fhandler->full_addr.data_base;
	data = (struct synaptics_rmi4_f51_data *)fhandler->data;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			data_base_addr,
			data->proximity_data,
			sizeof(data->proximity_data));
	if (retval < 0) {
#ifdef CONFIG_KT_WAKE_FUNCS
		synaptics_rmi4_release_all_finger(rmi4_data);
#endif
		return;
	}

	if (data->proximity_data[0] == 0x00)
		return;

	if (data->finger_hover_det && (data->hover_finger_z > 0)) {
		x = (data->hover_finger_x_4__11 << 4) |
				(data->hover_finger_xy_0__3 & 0x0f);
		y = (data->hover_finger_y_4__11 << 4) |
				(data->hover_finger_xy_0__3 >> 4);
		z = HOVER_Z_MAX - data->hover_finger_z;

#ifdef TYPE_B_PROTOCOL
		input_mt_slot(rmi4_data->input_dev, 0);
		input_mt_report_slot_state(rmi4_data->input_dev,
				MT_TOOL_FINGER, 1);
#endif
		input_report_key(rmi4_data->input_dev,
				BTN_TOUCH, 0);
		input_report_key(rmi4_data->input_dev,
				BTN_TOOL_FINGER, 1);
		input_report_abs(rmi4_data->input_dev,
				ABS_MT_POSITION_X, x);
		input_report_abs(rmi4_data->input_dev,
				ABS_MT_POSITION_Y, y);
		input_report_abs(rmi4_data->input_dev,
				ABS_MT_DISTANCE, z);

#ifndef TYPE_B_PROTOCOL
		input_mt_sync(rmi4_data->input_dev);
#endif
		input_sync(rmi4_data->input_dev);

#if defined(CONFIG_TOUCHSCREEN_FACTORY_PLATFORM)
		if (!rmi4_data->hover.state) {
			pr_debug("rmi-couldn't do the thing");
			rmi4_data->hover.state = 1;
		} else {
			rmi4_data->hover.mcount++;
		}
#endif
		rmi4_data->f51_finger = true;
		rmi4_data->fingers_on_2d = false;
		synaptics_rmi4_f51_finger_timer((unsigned long)rmi4_data);
	}

	if (data->air_swipe_det) {
		pr_debug("rmi-couldn't do the thing");
	}

	if (data->large_obj_det) {
		pr_debug("rmi-couldn't do the thing");
	}
/*
	if (data->hover_pinch_det) {
		pr_debug("rmi-couldn't do the thing");
	}
*/
	if (data->object_present) {
		pr_debug("rmi-couldn't do the thing");
	}

	return;
}
#endif

 /**
 * synaptics_rmi4_report_touch()
 *
 * Called by synaptics_rmi4_sensor_report().
 *
 * This function calls the appropriate finger data reporting function
 * based on the function handler it receives and returns the number of
 * fingers detected.
 */
static void synaptics_rmi4_report_touch(struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn *fhandler)
{
	unsigned char touch_count_2d;

	switch (fhandler->fn_number) {
	case SYNAPTICS_RMI4_F11:
		touch_count_2d = synaptics_rmi4_f11_abs_report(rmi4_data,
				fhandler);

		if (touch_count_2d)
			rmi4_data->fingers_on_2d = true;
		else
			rmi4_data->fingers_on_2d = false;
		break;
	case SYNAPTICS_RMI4_F12:
		touch_count_2d = synaptics_rmi4_f12_abs_report(rmi4_data,
				fhandler);

		if (touch_count_2d)
			rmi4_data->fingers_on_2d = true;
		else
			rmi4_data->fingers_on_2d = false;
		break;
	case SYNAPTICS_RMI4_F1A:
		synaptics_rmi4_f1a_report(rmi4_data, fhandler);
		break;
#ifdef PROXIMITY
	case SYNAPTICS_RMI4_F51:
		synaptics_rmi4_f51_report(rmi4_data, fhandler);
		break;
#endif
	default:
		break;
	}

	return;
}

 /**
 * synaptics_rmi4_sensor_report()
 *
 * Called by synaptics_rmi4_irq().
 *
 * This function determines the interrupt source(s) from the sensor
 * and calls synaptics_rmi4_report_touch() with the appropriate
 * function handler for each function with valid data inputs.
 */
static int synaptics_rmi4_sensor_report(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned char data[MAX_INTR_REGISTERS + 1];
	unsigned char *intr = &data[1];
	struct synaptics_rmi4_f01_device_status status;
	struct synaptics_rmi4_fn *fhandler;
	struct synaptics_rmi4_exp_fn *exp_fhandler;
	struct synaptics_rmi4_device_info *rmi;

	rmi = &(rmi4_data->rmi4_mod_info);

	/*
	 * Get interrupt status information from F01 Data1 register to
	 * determine the source(s) that are flagging the interrupt.
	 */
	retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->f01_data_base_addr,
			data,
			rmi4_data->num_of_intr_regs + 1);
	if (retval < 0) {
		pr_debug("rmi-couldn't do the thing");
		return retval;
	}

	status.data[0] = data[0];
	if (status.unconfigured) {
		if (rmi4_data->doing_reflash) {
			pr_debug("rmi-couldn't do the thing");
			return 0;
		}

		retval = synaptics_rmi4_reinit_device(rmi4_data);
		if (retval < 0) {
			pr_debug("rmi-couldn't do the thing");
		}
#ifdef CONFIG_KT_WAKE_FUNCS
		synaptics_rmi4_release_all_finger(rmi4_data);
#endif
		return 0;
	}

	/*
	 * Traverse the function handler list and service the source(s)
	 * of the interrupt accordingly.
	 */
	if (!list_empty(&rmi->support_fn_list)) {
		list_for_each_entry(fhandler, &rmi->support_fn_list, link) {
			if (fhandler->num_of_data_sources) {
				if (fhandler->intr_mask &
						intr[fhandler->intr_reg_num]) {
					synaptics_rmi4_report_touch(rmi4_data,
							fhandler);
				}
			}
		}
	}

	if (!list_empty(&exp_fn_list)) {
		list_for_each_entry(exp_fhandler, &exp_fn_list, link) {
			if (exp_fhandler->func_attn != NULL)
				exp_fhandler->func_attn(rmi4_data, intr[0]);
		}
	}

	return 0;
}

 /**
 * synaptics_rmi4_irq()
 *
 * Called by the kernel when an interrupt occurs (when the sensor
 * asserts the attention irq).
 *
 * This function is the ISR thread and handles the acquisition
 * and the reporting of finger data when the presence of fingers
 * is detected.
 */
static irqreturn_t synaptics_rmi4_irq(int irq, void *data)
{
	int retval;
	struct synaptics_rmi4_data *rmi4_data = data;

	do {
		retval = synaptics_rmi4_sensor_report(rmi4_data);
		if (retval < 0) {
			pr_debug("rmi-couldn't do the thing");
			goto out;
		}

		if (!rmi4_data->touch_stopped)
			goto out;

	} while (!gpio_get_value(rmi4_data->board->gpio));

out:
	return IRQ_HANDLED;
}

 /**
 * synaptics_rmi4_irq_enable()
 *
 * Called by synaptics_rmi4_probe() and the power management functions
 * in this driver and also exported to other expansion Function modules
 * such as rmi_dev.
 *
 * This function handles the enabling and disabling of the attention
 * irq including the setting up of the ISR thread.
 */
static int synaptics_rmi4_irq_enable(struct synaptics_rmi4_data *rmi4_data,
		bool enable)
{
	int retval = 0;
	unsigned char intr_status;
	const struct synaptics_rmi4_platform_data *platform_data =
			rmi4_data->i2c_client->dev.platform_data;

	if (enable) {
		if (rmi4_data->irq_enabled)
			return retval;

		/* Clear interrupts first */
		retval = synaptics_rmi4_i2c_read(rmi4_data,
				rmi4_data->f01_data_base_addr + 1,
				&intr_status,
				rmi4_data->num_of_intr_regs);
		if (retval < 0)
			return retval;

		retval = request_threaded_irq(rmi4_data->irq, NULL,
				synaptics_rmi4_irq, platform_data->irq_type,
				DRIVER_NAME, rmi4_data);
		if (retval < 0) {
			pr_debug("rmi-couldn't do the thing");
			return retval;
		}

		rmi4_data->irq_enabled = true;
	} else {
		if (rmi4_data->irq_enabled) {
			disable_irq(rmi4_data->irq);
			free_irq(rmi4_data->irq, rmi4_data);
			rmi4_data->irq_enabled = false;
		}
	}

	return retval;
}

 /**
 * synaptics_rmi4_f11_init()
 *
 * Called by synaptics_rmi4_query_device().
 *
 * This funtion parses information from the Function 11 registers
 * and determines the number of fingers supported, x and y data ranges,
 * offset to the associated interrupt status register, interrupt bit
 * mask, and gathers finger data acquisition capabilities from the query
 * registers.
 */
static int synaptics_rmi4_f11_init(struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn *fhandler,
		struct synaptics_rmi4_fn_desc *fd,
		unsigned int intr_count)
{
	int retval;
	unsigned char ii;
	unsigned char intr_offset;
	unsigned char abs_data_size;
	unsigned char abs_data_blk_size;
	unsigned char query[F11_STD_QUERY_LEN];
	unsigned char control[F11_STD_CTRL_LEN];

	fhandler->fn_number = fd->fn_number;
	fhandler->num_of_data_sources = fd->intr_src_count;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			fhandler->full_addr.query_base,
			query,
			sizeof(query));
	if (retval < 0)
		return retval;

	/* Maximum number of fingers supported */
	if ((query[1] & MASK_3BIT) <= 4)
		fhandler->num_of_data_points = (query[1] & MASK_3BIT) + 1;
	else if ((query[1] & MASK_3BIT) == 5)
		fhandler->num_of_data_points = 10;

	rmi4_data->num_of_fingers = fhandler->num_of_data_points;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			fhandler->full_addr.ctrl_base,
			control,
			sizeof(control));
	if (retval < 0)
		return retval;

	/* Maximum x and y */
	rmi4_data->sensor_max_x = ((control[6] & MASK_8BIT) << 0) |
			((control[7] & MASK_4BIT) << 8);
	rmi4_data->sensor_max_y = ((control[8] & MASK_8BIT) << 0) |
			((control[9] & MASK_4BIT) << 8);

	rmi4_data->max_touch_width = MAX_F11_TOUCH_WIDTH;

	fhandler->intr_reg_num = (intr_count + 7) / 8;
	if (fhandler->intr_reg_num != 0)
		fhandler->intr_reg_num -= 1;

	/* Set an enable bit for each data source */
	intr_offset = intr_count % 8;
	fhandler->intr_mask = 0;
	for (ii = intr_offset;
			ii < ((fd->intr_src_count & MASK_3BIT) +
			intr_offset);
			ii++)
		fhandler->intr_mask |= 1 << ii;

	abs_data_size = query[5] & MASK_2BIT;
	abs_data_blk_size = 3 + (2 * (abs_data_size == 0 ? 1 : 0));
	fhandler->size_of_data_register_block = abs_data_blk_size;
	fhandler->data = NULL;

	return retval;
}

static int synaptics_rmi4_f12_set_enables(struct synaptics_rmi4_data *rmi4_data,
		unsigned short address)
{
	int retval;
	unsigned char enable_mask;
	static unsigned short ctrl_28_address;

	if (address)
		ctrl_28_address = address;

	enable_mask = RPT_DEFAULT;
#ifdef REPORT_2D_Z
	enable_mask |= RPT_Z;
#endif
#ifdef REPORT_2D_W
	enable_mask |= (RPT_WX | RPT_WY);
#endif

	retval = synaptics_rmi4_i2c_write(rmi4_data,
			ctrl_28_address,
			&enable_mask,
			sizeof(enable_mask));
	if (retval < 0)
		return retval;

	return retval;
}

 /**
 * synaptics_rmi4_f12_init()
 *
 * Called by synaptics_rmi4_query_device().
 *
 * This funtion parses information from the Function 12 registers and
 * determines the number of fingers supported, offset to the data1
 * register, x and y data ranges, offset to the associated interrupt
 * status register, interrupt bit mask, and allocates memory resources
 * for finger data acquisition.
 */
static int synaptics_rmi4_f12_init(struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn *fhandler,
		struct synaptics_rmi4_fn_desc *fd,
		unsigned int intr_count)
{
	int retval;
	unsigned char ii;
	unsigned char intr_offset;
	unsigned char size_of_2d_data;
	unsigned char ctrl_8_offset;
	unsigned char ctrl_9_offset;
	unsigned char ctrl_23_offset;
	unsigned char ctrl_26_offset;
	unsigned char ctrl_28_offset;
	unsigned char fingers_to_support = F12_FINGERS_TO_SUPPORT;
	struct synaptics_rmi4_f12_query_5 query_5;
	struct synaptics_rmi4_f12_query_8 query_8;
	struct synaptics_rmi4_f12_ctrl_8 ctrl_8;
	struct synaptics_rmi4_f12_ctrl_9 ctrl_9;
	struct synaptics_rmi4_f12_ctrl_23 ctrl_23;
	struct synaptics_rmi4_f12_ctrl_26 ctrl_26;

	fhandler->fn_number = fd->fn_number;
	fhandler->num_of_data_sources = fd->intr_src_count;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			fhandler->full_addr.query_base + 5,
			query_5.data,
			sizeof(query_5.data));
	if (retval < 0)
		return retval;

	ctrl_8_offset = query_5.ctrl0_is_present +
			query_5.ctrl1_is_present +
			query_5.ctrl2_is_present +
			query_5.ctrl3_is_present +
			query_5.ctrl4_is_present +
			query_5.ctrl5_is_present +
			query_5.ctrl6_is_present +
			query_5.ctrl7_is_present;

	ctrl_9_offset = ctrl_8_offset +
			query_5.ctrl8_is_present;

	ctrl_23_offset = ctrl_9_offset +
			query_5.ctrl9_is_present +
			query_5.ctrl10_is_present +
			query_5.ctrl11_is_present +
			query_5.ctrl12_is_present +
			query_5.ctrl13_is_present +
			query_5.ctrl14_is_present +
			query_5.ctrl15_is_present +
			query_5.ctrl16_is_present +
			query_5.ctrl17_is_present +
			query_5.ctrl18_is_present +
			query_5.ctrl19_is_present +
			query_5.ctrl20_is_present +
			query_5.ctrl21_is_present +
			query_5.ctrl22_is_present;

	ctrl_26_offset = ctrl_23_offset +
			query_5.ctrl23_is_present +
			query_5.ctrl24_is_present +
			query_5.ctrl25_is_present;

	ctrl_28_offset = ctrl_26_offset +
			query_5.ctrl26_is_present +
			query_5.ctrl27_is_present;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			fhandler->full_addr.ctrl_base + ctrl_26_offset,
			ctrl_26.data,
			sizeof(ctrl_26.data));
	rmi4_data->glove_touch_addr = fhandler->full_addr.ctrl_base + ctrl_26_offset;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			fhandler->full_addr.ctrl_base + ctrl_23_offset,
			ctrl_23.data,
			sizeof(ctrl_23.data));
	if (retval < 0)
		return retval;

	/* Maximum number of fingers supported */
	fhandler->num_of_data_points = min(ctrl_23.max_reported_objects,
			fingers_to_support);

	rmi4_data->num_of_fingers = fhandler->num_of_data_points;

	retval = synaptics_rmi4_f12_set_enables(rmi4_data,
			fhandler->full_addr.ctrl_base + ctrl_28_offset);
	if (retval < 0)
		return retval;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			fhandler->full_addr.query_base + 8,
			query_8.data,
			sizeof(query_8.data));
	if (retval < 0)
		return retval;

	/* Determine the presence of the Data0 register */
	fhandler->data1_offset = query_8.data0_is_present;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			fhandler->full_addr.ctrl_base + ctrl_8_offset,
			ctrl_8.data,
			sizeof(ctrl_8.data));
	if (retval < 0)
		return retval;

	/* Maximum x and y */
	rmi4_data->sensor_max_x =
			((unsigned short)ctrl_8.max_x_coord_lsb << 0) |
			((unsigned short)ctrl_8.max_x_coord_msb << 8);
	rmi4_data->sensor_max_y =
			((unsigned short)ctrl_8.max_y_coord_lsb << 0) |
			((unsigned short)ctrl_8.max_y_coord_msb << 8);

	if (!rmi4_data->board->num_of_rx && !rmi4_data->board->num_of_tx) {
		rmi4_data->num_of_rx = ctrl_8.num_of_rx;
		rmi4_data->num_of_tx = ctrl_8.num_of_tx;
		rmi4_data->max_touch_width = max(rmi4_data->num_of_rx,
				rmi4_data->num_of_tx);
		rmi4_data->num_of_node = ctrl_8.num_of_rx * ctrl_8.num_of_tx;
	} else {
		rmi4_data->num_of_rx = rmi4_data->board->num_of_rx;
		rmi4_data->num_of_tx = rmi4_data->board->num_of_tx;
		rmi4_data->max_touch_width = max(rmi4_data->num_of_rx,
				rmi4_data->num_of_tx);
		rmi4_data->num_of_node = rmi4_data->num_of_rx * rmi4_data->num_of_tx;
	}
	retval = synaptics_rmi4_i2c_read(rmi4_data,
			fhandler->full_addr.ctrl_base + ctrl_9_offset,
			ctrl_9.data,
			sizeof(ctrl_9.data));

	if (retval < 0)
		return retval;

	rmi4_data->touch_threshold = (int)ctrl_9.touch_threshold;
	rmi4_data->gloved_sensitivity = (int)ctrl_9.gloved_finger;

	fhandler->intr_reg_num = (intr_count + 7) / 8;
	if (fhandler->intr_reg_num != 0)
		fhandler->intr_reg_num -= 1;

	/* Set an enable bit for each data source */
	intr_offset = intr_count % 8;
	fhandler->intr_mask = 0;
	for (ii = intr_offset;
			ii < ((fd->intr_src_count & MASK_3BIT) +
			intr_offset);
			ii++)
		fhandler->intr_mask |= 1 << ii;

	size_of_2d_data = sizeof(struct synaptics_rmi4_f12_finger_data);

	/* Allocate memory for finger data storage space */
	fhandler->data_size = fhandler->num_of_data_points * size_of_2d_data;
	fhandler->data = kzalloc(fhandler->data_size, GFP_KERNEL);

#ifdef CONFIG_GLOVE_TOUCH
	rmi4_data->glove_mode_enables_addr = fhandler->full_addr.ctrl_base +
				ctrl_26_offset;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			fhandler->full_addr.query_base + 10,
			ctrl_26.data,
			sizeof(ctrl_26.data));
	if (retval < 0)
		return retval;

	rmi4_data->glove_mode_feature = ctrl_26.glove_feature_enable;
	synaptics_rmi4_glove_mode_enables(rmi4_data);
#endif

	return retval;
}

static int synaptics_rmi4_f1a_alloc_mem(struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn *fhandler)
{
	int retval;
	struct synaptics_rmi4_f1a_handle *f1a;

	f1a = kzalloc(sizeof(*f1a), GFP_KERNEL);
	if (!f1a) {
		pr_debug("rmi-couldn't do the thing");
		return -ENOMEM;
	}

	fhandler->data = (void *)f1a;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			fhandler->full_addr.query_base,
			f1a->button_query.data,
			sizeof(f1a->button_query.data));
	if (retval < 0) {
		pr_debug("rmi-couldn't do the thing");
		return retval;
	}

	f1a->button_count = f1a->button_query.max_button_count + 1;
	f1a->button_bitmask_size = (f1a->button_count + 7) / 8;

	f1a->button_data_buffer = kcalloc(f1a->button_bitmask_size,
			sizeof(*(f1a->button_data_buffer)), GFP_KERNEL);
	if (!f1a->button_data_buffer) {
		pr_debug("rmi-couldn't do the thing");
		return -ENOMEM;
	}

	f1a->button_map = kcalloc(f1a->button_count,
			sizeof(*(f1a->button_map)), GFP_KERNEL);
	if (!f1a->button_map) {
		pr_debug("rmi-couldn't do the thing");
		return -ENOMEM;
	}

	return 0;
}

static int synaptics_rmi4_f1a_button_map(struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn *fhandler)
{
	unsigned char ii;
	struct synaptics_rmi4_f1a_handle *f1a = fhandler->data;
	const struct synaptics_rmi4_platform_data *pdata = rmi4_data->board;

	if (!pdata->f1a_button_map) {
		pr_debug("rmi-couldn't do the thing");
		return -ENODEV;
	} else if (!pdata->f1a_button_map->map) {
		pr_debug("rmi-couldn't do the thing");
		return -ENODEV;
	} else {
		if (pdata->f1a_button_map->nbuttons != f1a->button_count) {
			f1a->valid_button_count = min(f1a->button_count,
					pdata->f1a_button_map->nbuttons);
		} else {
			f1a->valid_button_count = f1a->button_count;
		}

		for (ii = 0; ii < f1a->valid_button_count; ii++)
			f1a->button_map[ii] = pdata->f1a_button_map->map[ii];
	}

	return 0;
}

static void synaptics_rmi4_f1a_kfree(struct synaptics_rmi4_fn *fhandler)
{
	struct synaptics_rmi4_f1a_handle *f1a = fhandler->data;

	if (f1a) {
		kfree(f1a->button_data_buffer);
		kfree(f1a->button_map);
		kfree(f1a);
		fhandler->data = NULL;
	}

	return;
}

static int synaptics_rmi4_f1a_init(struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn *fhandler,
		struct synaptics_rmi4_fn_desc *fd,
		unsigned int intr_count)
{
	int retval;
	unsigned char ii;
	unsigned short intr_offset;

	fhandler->fn_number = fd->fn_number;
	fhandler->num_of_data_sources = fd->intr_src_count;

	fhandler->intr_reg_num = (intr_count + 7) / 8;
	if (fhandler->intr_reg_num != 0)
		fhandler->intr_reg_num -= 1;

	/* Set an enable bit for each data source */
	intr_offset = intr_count % 8;
	fhandler->intr_mask = 0;
	for (ii = intr_offset;
			ii < ((fd->intr_src_count & MASK_3BIT) +
			intr_offset);
			ii++)
		fhandler->intr_mask |= 1 << ii;

	retval = synaptics_rmi4_f1a_alloc_mem(rmi4_data, fhandler);
	if (retval < 0)
		goto error_exit;

	retval = synaptics_rmi4_f1a_button_map(rmi4_data, fhandler);
	if (retval < 0)
		goto error_exit;

	rmi4_data->button_0d_enabled = 1;

	return 0;

error_exit:
	synaptics_rmi4_f1a_kfree(fhandler);

	return retval;
}

static int synaptics_rmi4_f34_init(struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn *fhandler,
		struct synaptics_rmi4_fn_desc *fd,
		unsigned int intr_count)
{
	int retval;
	struct synaptics_rmi4_f34_ctrl_3 ctrl_3;

	fhandler->fn_number = fd->fn_number;
	fhandler->num_of_data_sources = fd->intr_src_count;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			fhandler->full_addr.ctrl_base,
			ctrl_3.data,
			sizeof(ctrl_3.data));
	if (retval < 0)
		return retval;

	rmi4_data->fw_release_date_of_ic =
		(ctrl_3.fw_release_month << 8) | ctrl_3.fw_release_date;
	rmi4_data->ic_revision_of_ic = ctrl_3.fw_release_revision;
	rmi4_data->fw_version_of_ic = ctrl_3.fw_release_version;
	fhandler->data = NULL;

	return retval;
}

#ifdef PROXIMITY
#ifdef USE_CUSTOM_REZERO
static void synaptics_work_rezero(struct work_struct *work)
{
	struct synaptics_rmi4_data *rmi4_data =
		container_of(work,
			struct synaptics_rmi4_data, work_rezero.work);
	int retval;
	unsigned char custom_rezero;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			f51->proximity_custom_rezero_addr,
			&custom_rezero, sizeof(custom_rezero));

	if (F51_VERSION == f51->num_of_data_sources)
		custom_rezero |= 0x34;
	else
		custom_rezero |= 0x01;

	retval = synaptics_rmi4_i2c_write(rmi4_data,
			f51->proximity_custom_rezero_addr,
			&custom_rezero, sizeof(custom_rezero));

	if (retval < 0)
		pr_debug("rmi-couldn't do the thing");
}

void synaptics_rmi4_f51_set_custom_rezero(struct synaptics_rmi4_data *rmi4_data)
{
	schedule_delayed_work(&rmi4_data->work_rezero,
				msecs_to_jiffies(300));
}
#endif

int synaptics_proximity_no_sleep_set(bool enables)
{
	int retval;
	unsigned char no_sleep = 0;

	if (!f51)
		return -ENODEV;

	retval = synaptics_rmi4_i2c_read(f51->rmi4_data,
			f51->rmi4_data->f01_ctrl_base_addr,
			&no_sleep,
			sizeof(no_sleep));
	if (retval <= 0)
		pr_debug("rmi-couldn't do the thing");

	if (enables)
		no_sleep |= 0x04;
	else
		no_sleep &= 0xFB;

	retval = synaptics_rmi4_i2c_write(f51->rmi4_data,
			f51->rmi4_data->f01_ctrl_base_addr,
			&no_sleep,
			sizeof(no_sleep));
	if (retval <= 0)
		pr_debug("rmi-couldn't do the thing");

	return retval;
}
EXPORT_SYMBOL(synaptics_proximity_no_sleep_set);

static int synaptics_rmi4_f51_set_enables(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned char hover_enable;

	/* if fw version is larger than 0x39,
	 * when IC go into DOZE mode,
	 * disable proximity sensing mode(hover).
	 */
	if (rmi4_data->panel_revision >= OCTA_PANEL_REVISION_43) {
		if (f51->proximity_enables & 0x01)
			hover_enable = 0x03;
		else
			hover_enable = 0x82;
	} else {
		hover_enable = f51->proximity_enables;
	}

	retval = synaptics_rmi4_i2c_write(rmi4_data,
			f51->proximity_enables_addr,
			&hover_enable,
			sizeof(hover_enable));
	if (retval < 0)
		return retval;

#ifdef USE_CUSTOM_REZERO
	if (f51->proximity_enables & 0x01)
		synaptics_rmi4_f51_set_custom_rezero(rmi4_data);
#endif

	return 0;
}

static int synaptics_rmi4_f51_init(struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn *fhandler,
		struct synaptics_rmi4_fn_desc *fd,
		unsigned int intr_count)
{
	int retval;
	unsigned char ii;
	unsigned short intr_offset;
	struct synaptics_rmi4_f51_data *data_register;
	struct synaptics_rmi4_f51_query query_register;

	fhandler->fn_number = fd->fn_number;
	fhandler->num_of_data_sources = fd->intr_src_count;

	fhandler->intr_reg_num = (intr_count + 7) / 8;
	if (fhandler->intr_reg_num != 0)
		fhandler->intr_reg_num -= 1;

	/* Set an enable bit for each data source */
	intr_offset = intr_count % 8;
	fhandler->intr_mask = 0;
	for (ii = intr_offset;
			ii < ((fd->intr_src_count & MASK_3BIT) + intr_offset);
			ii++)
		fhandler->intr_mask |= 1 << ii;

	fhandler->data_size = sizeof(*data_register);
	data_register = kzalloc(fhandler->data_size, GFP_KERNEL);
	fhandler->data = (void *)data_register;

	f51 = kzalloc(sizeof(*f51), GFP_KERNEL);
	f51->rmi4_data = rmi4_data;

	if (F51_VERSION == fhandler->num_of_data_sources) {
		f51->num_of_data_sources = fhandler->num_of_data_sources;
		f51->proximity_enables = AIR_SWIPE_EN | SLEEP_PROXIMITY;
		f51->proximity_enables_addr = fhandler->full_addr.ctrl_base +
			F51_PROXIMITY_ENABLES_OFFSET;
#ifdef USE_CUSTOM_REZERO
		f51->proximity_custom_rezero_addr = fhandler->full_addr.ctrl_base +
			F51_GENERAL_CONTROL_OFFSET;
#endif
	} else {
		f51->proximity_enables = PROXIMITY_DEFAULT;
		f51->proximity_enables_addr = fhandler->full_addr.ctrl_base +
				F51_CTRL54_OFFSET;
#ifdef USE_CUSTOM_REZERO
		f51->proximity_custom_rezero_addr = fhandler->full_addr.ctrl_base +
			F51_CTRL78_OFFSET;
#endif
	}
	retval = synaptics_rmi4_i2c_read(rmi4_data,
			fhandler->full_addr.query_base,
			query_register.data,
			sizeof(query_register));
	if (retval < 0)
		return retval;

	/* Save proximity controls(query 4 register) to get which functions
	 * are supported by firmware.
	 */
	f51->proximity_controls = query_register.proximity_controls;
	pr_debug("rmi-couldn't do the thing");

	retval = synaptics_rmi4_f51_set_enables(rmi4_data);
	if (retval < 0)
		return retval;

	return 0;
}

int synaptics_rmi4_proximity_enables(unsigned char enables)
{
	int retval;

	if (!f51)
		return -ENODEV;

	if (F51_VERSION == f51->num_of_data_sources) {
		if (enables)
			f51->proximity_enables = AIR_SWIPE_EN | FINGER_HOVER_EN;
		else
			f51->proximity_enables = AIR_SWIPE_EN | SLEEP_PROXIMITY;
	} else {
		if (enables)
			f51->proximity_enables = PROXIMITY_ENABLE;
		else
			f51->proximity_enables = PROXIMITY_DEFAULT;
	}

	if (f51->rmi4_data->touch_stopped)
		return 0;

	retval = synaptics_rmi4_f51_set_enables(f51->rmi4_data);
	if (retval < 0)
		return retval;

	return 0;
}
EXPORT_SYMBOL(synaptics_rmi4_proximity_enables);
#endif

static int synaptics_rmi4_check_status(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	int timeout = CHECK_STATUS_TIMEOUT_MS;
	unsigned char data;
	struct synaptics_rmi4_f01_device_status status;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->f01_data_base_addr,
			status.data,
			sizeof(status.data));
	if (retval < 0)
		return retval;

	while (status.status_code == STATUS_CRC_IN_PROGRESS) {
		if (timeout > 0)
			msleep(20);
		else
			return -1;

		retval = synaptics_rmi4_i2c_read(rmi4_data,
				rmi4_data->f01_data_base_addr,
				status.data,
				sizeof(status.data));
		if (retval < 0)
			return retval;

		timeout -= 20;
	}

	if (status.flash_prog == 1) {
		rmi4_data->flash_prog_mode = true;
		pr_debug("rmi-couldn't do the thing");
	} else {
		rmi4_data->flash_prog_mode = false;
	}

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->f01_data_base_addr,
			&data,
			sizeof(data));
	if (retval < 0) {
		pr_debug("rmi-couldn't do the thing");
		return retval;
	}

	return 0;
}

static void synaptics_rmi4_set_configured(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned char device_ctrl;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->f01_ctrl_base_addr,
			&device_ctrl,
			sizeof(device_ctrl));
	if (retval < 0) {
		pr_debug("rmi-couldn't do the thing");
		return;
	}

	device_ctrl |= CONFIGURED;

	retval = synaptics_rmi4_i2c_write(rmi4_data,
			rmi4_data->f01_ctrl_base_addr,
			&device_ctrl,
			sizeof(device_ctrl));
	if (retval < 0) {
		pr_debug("rmi-couldn't do the thing");
	}

	return;
}

static int synaptics_rmi4_alloc_fh(struct synaptics_rmi4_fn **fhandler,
		struct synaptics_rmi4_fn_desc *rmi_fd, int page_number)
{
	*fhandler = kzalloc(sizeof(**fhandler), GFP_KERNEL);
	if (!(*fhandler))
		return -ENOMEM;

	(*fhandler)->full_addr.data_base =
			(rmi_fd->data_base_addr |
			(page_number << 8));
	(*fhandler)->full_addr.ctrl_base =
			(rmi_fd->ctrl_base_addr |
			(page_number << 8));
	(*fhandler)->full_addr.cmd_base =
			(rmi_fd->cmd_base_addr |
			(page_number << 8));
	(*fhandler)->full_addr.query_base =
			(rmi_fd->query_base_addr |
			(page_number << 8));

	return 0;
}

 /**
 * synaptics_rmi4_query_device()
 *
 * Called by synaptics_rmi4_probe().
 *
 * This funtion scans the page description table, records the offsets
 * to the register types of Function $01, sets up the function handlers
 * for Function $11 and Function $12, determines the number of interrupt
 * sources from the sensor, adds valid Functions with data inputs to the
 * Function linked list, parses information from the query registers of
 * Function $01, and enables the interrupt sources from the valid Functions
 * with data inputs.
 */
static int synaptics_rmi4_query_device(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned char ii = 0;
	unsigned char page_number;
	unsigned char intr_count = 0;
	unsigned char f01_query[F01_STD_QUERY_LEN];
	unsigned short pdt_entry_addr;
	unsigned short intr_addr;
	struct synaptics_rmi4_fn_desc rmi_fd;
	struct synaptics_rmi4_fn *fhandler;
	struct synaptics_rmi4_device_info *rmi;

	rmi = &(rmi4_data->rmi4_mod_info);

	INIT_LIST_HEAD(&rmi->support_fn_list);

	/* Scan the page description tables of the pages to service */
	for (page_number = 0; page_number < PAGES_TO_SERVICE; page_number++) {
		for (pdt_entry_addr = PDT_START; pdt_entry_addr > PDT_END;
				pdt_entry_addr -= PDT_ENTRY_SIZE) {
			pdt_entry_addr |= (page_number << 8);

			retval = synaptics_rmi4_i2c_read(rmi4_data,
					pdt_entry_addr,
					(unsigned char *)&rmi_fd,
					sizeof(rmi_fd));
			if (retval < 0)
				return retval;

			fhandler = NULL;

			if (rmi_fd.fn_number == 0) {
				pr_debug("rmi-couldn't do the thing");
				break;
			}

			switch (rmi_fd.fn_number) {
			case SYNAPTICS_RMI4_F01:
				rmi4_data->f01_query_base_addr =
						rmi_fd.query_base_addr;
				rmi4_data->f01_ctrl_base_addr =
						rmi_fd.ctrl_base_addr;
				rmi4_data->f01_data_base_addr =
						rmi_fd.data_base_addr;
				rmi4_data->f01_cmd_base_addr =
						rmi_fd.cmd_base_addr;

				retval = synaptics_rmi4_check_status(rmi4_data);
				if (retval < 0) {
					pr_debug("rmi-couldn't do the thing");
					return retval;
				}

				if (rmi4_data->flash_prog_mode)
					goto flash_prog_mode;

				break;
			case SYNAPTICS_RMI4_F11:
				if (rmi_fd.intr_src_count == 0)
					break;

				retval = synaptics_rmi4_alloc_fh(&fhandler,
						&rmi_fd, page_number);
				if (retval < 0) {
					pr_debug("rmi-couldn't do the thing");
					return retval;
				}

				retval = synaptics_rmi4_f11_init(rmi4_data,
						fhandler, &rmi_fd, intr_count);
				if (retval < 0)
					return retval;
				break;
			case SYNAPTICS_RMI4_F12:
				if (rmi_fd.intr_src_count == 0)
					break;

				retval = synaptics_rmi4_alloc_fh(&fhandler,
						&rmi_fd, page_number);
				if (retval < 0) {
					pr_debug("rmi-couldn't do the thing");
					return retval;
				}

				retval = synaptics_rmi4_f12_init(rmi4_data,
						fhandler, &rmi_fd, intr_count);
				if (retval < 0)
					return retval;
				break;
			case SYNAPTICS_RMI4_F1A:
				if (rmi_fd.intr_src_count == 0)
					break;

				retval = synaptics_rmi4_alloc_fh(&fhandler,
						&rmi_fd, page_number);
				if (retval < 0) {
					pr_debug("rmi-couldn't do the thing");
					return retval;
				}

				retval = synaptics_rmi4_f1a_init(rmi4_data,
						fhandler, &rmi_fd, intr_count);
				if (retval < 0)
					return retval;
				break;
			case SYNAPTICS_RMI4_F34:
				if (rmi_fd.intr_src_count == 0)
					break;

				retval = synaptics_rmi4_alloc_fh(&fhandler,
						&rmi_fd, page_number);
				if (retval < 0) {
					pr_debug("rmi-couldn't do the thing");
					return retval;
				}

				retval = synaptics_rmi4_f34_init(rmi4_data,
						fhandler, &rmi_fd, intr_count);
				if (retval < 0)
					return retval;
				break;

#ifdef PROXIMITY
			case SYNAPTICS_RMI4_F51:
				if (rmi_fd.intr_src_count == 0)
					break;

				retval = synaptics_rmi4_alloc_fh(&fhandler,
						&rmi_fd, page_number);
				if (retval < 0) {
					pr_debug("rmi-couldn't do the thing");
					return retval;
				}

				retval = synaptics_rmi4_f51_init(rmi4_data,
						fhandler, &rmi_fd, intr_count);
				if (retval < 0)
					return retval;
				break;
#endif
			}

			/* Accumulate the interrupt count */
			intr_count += (rmi_fd.intr_src_count & MASK_3BIT);

			if (fhandler && rmi_fd.intr_src_count) {
				list_add_tail(&fhandler->link,
						&rmi->support_fn_list);
			}
		}
	}

flash_prog_mode:
	rmi4_data->num_of_intr_regs = (intr_count + 7) / 8;
	pr_debug("rmi-couldn't do the thing");

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->f01_query_base_addr,
			f01_query,
			sizeof(f01_query));
	if (retval < 0)
		return retval;

	/* RMI Version 4.0 currently supported */
	rmi->version_major = 4;
	rmi->version_minor = 0;

	rmi->manufacturer_id = f01_query[0];
	rmi->product_props = f01_query[1];
	rmi->product_info[0] = f01_query[2] & MASK_7BIT;
	rmi->product_info[1] = f01_query[3] & MASK_7BIT;
	rmi->date_code[0] = f01_query[4] & MASK_5BIT;
	rmi->date_code[1] = f01_query[5] & MASK_4BIT;
	rmi->date_code[2] = f01_query[6] & MASK_5BIT;
	rmi->tester_id = ((f01_query[7] & MASK_7BIT) << 8) |
			(f01_query[8] & MASK_7BIT);
	rmi->serial_number = ((f01_query[9] & MASK_7BIT) << 8) |
			(f01_query[10] & MASK_7BIT);
	memcpy(rmi->product_id_string, &f01_query[11], 10);

	if (rmi->manufacturer_id != 1) {
		pr_debug("rmi-couldn't do the thing");
	}

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->f01_query_base_addr + F01_BUID_ID_OFFSET,
			rmi->build_id,
			sizeof(rmi->build_id));
	if (retval < 0)
		return retval;

	memset(rmi4_data->intr_mask, 0x00, sizeof(rmi4_data->intr_mask));

	/*
	 * Map out the interrupt bit masks for the interrupt sources
	 * from the registered function handlers.
	 */
	if (!list_empty(&rmi->support_fn_list)) {
		list_for_each_entry(fhandler, &rmi->support_fn_list, link) {
			if (fhandler->num_of_data_sources) {
				rmi4_data->intr_mask[fhandler->intr_reg_num] |=
						fhandler->intr_mask;
			}
		}
	}

	/* Enable the interrupt sources */
	for (ii = 0; ii < rmi4_data->num_of_intr_regs; ii++) {
		if (rmi4_data->intr_mask[ii] != 0x00) {
			pr_debug("rmi-couldn't do the thing");
			intr_addr = rmi4_data->f01_ctrl_base_addr + 1 + ii;
			retval = synaptics_rmi4_i2c_write(rmi4_data,
					intr_addr,
					&(rmi4_data->intr_mask[ii]),
					sizeof(rmi4_data->intr_mask[ii]));
			if (retval < 0)
				return retval;
		}
	}

	synaptics_rmi4_set_configured(rmi4_data);

	return 0;
}

static void synaptics_rmi4_release_support_fn(struct synaptics_rmi4_data *rmi4_data)
{
	struct synaptics_rmi4_fn *fhandler, *n;
	struct synaptics_rmi4_device_info *rmi;

	rmi = &(rmi4_data->rmi4_mod_info);

	if (list_empty(&rmi->support_fn_list)) {
		pr_debug("rmi-couldn't do the thing");
		goto out;
	}

	list_for_each_entry_safe(fhandler, n, &rmi->support_fn_list, link) {
		pr_debug("rmi-couldn't do the thing");
		if (fhandler->fn_number == SYNAPTICS_RMI4_F1A)
			synaptics_rmi4_f1a_kfree(fhandler);
		else
			kfree(fhandler->data);

		kfree(fhandler);
	}

out:
#ifdef PROXIMITY
	kfree(f51);
	f51 = NULL;
#endif
}

static int synaptics_rmi4_set_input_device
		(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned char ii;
	struct synaptics_rmi4_f1a_handle *f1a;
	struct synaptics_rmi4_fn *fhandler;
	struct synaptics_rmi4_device_info *rmi;

	rmi = &(rmi4_data->rmi4_mod_info);

	rmi4_data->input_dev = input_allocate_device();
	if (rmi4_data->input_dev == NULL) {
		pr_debug("rmi-couldn't do the thing");
		retval = -ENOMEM;
		goto err_input_device;
	}

	retval = synaptics_rmi4_query_device(rmi4_data);
	if (retval < 0) {
		pr_debug("rmi-couldn't do the thing");
		goto err_query_device;
	}

	rmi4_data->input_dev->name = "sec_touchscreen";
	rmi4_data->input_dev->id.bustype = BUS_I2C;
	rmi4_data->input_dev->dev.parent = &rmi4_data->i2c_client->dev;
#ifdef USE_OPEN_CLOSE
	rmi4_data->input_dev->open = synaptics_rmi4_input_open;
	rmi4_data->input_dev->close = synaptics_rmi4_input_close;
#endif
	input_set_drvdata(rmi4_data->input_dev, rmi4_data);

#ifdef CONFIG_GLOVE_TOUCH
	input_set_capability(rmi4_data->input_dev, EV_SW, SW_GLOVE);
#endif
	set_bit(EV_SYN, rmi4_data->input_dev->evbit);
	set_bit(EV_KEY, rmi4_data->input_dev->evbit);
	set_bit(EV_ABS, rmi4_data->input_dev->evbit);
	set_bit(BTN_TOUCH, rmi4_data->input_dev->keybit);
	set_bit(BTN_TOOL_FINGER, rmi4_data->input_dev->keybit);
#ifdef INPUT_PROP_DIRECT
	set_bit(INPUT_PROP_DIRECT, rmi4_data->input_dev->propbit);
#endif

	input_set_abs_params(rmi4_data->input_dev,
			ABS_MT_POSITION_X, 0,
			rmi4_data->board->max_x, 0, 0);
	input_set_abs_params(rmi4_data->input_dev,
			ABS_MT_POSITION_Y, 0,
			rmi4_data->board->max_y, 0, 0);
#ifdef REPORT_2D_W
#ifdef EDGE_SWIPE
	input_set_abs_params(rmi4_data->input_dev,
			ABS_MT_TOUCH_MAJOR, 0,
			EDGE_SWIPE_WIDTH_MAX, 0, 0);
	input_set_abs_params(rmi4_data->input_dev,
			ABS_MT_TOUCH_MINOR, 0,
			EDGE_SWIPE_WIDTH_MAX, 0, 0);
#else
	input_set_abs_params(rmi4_data->input_dev,
			ABS_MT_TOUCH_MAJOR, 0,
			rmi4_data->board->max_width, 0, 0);
	input_set_abs_params(rmi4_data->input_dev,
			ABS_MT_TOUCH_MINOR, 0,
			rmi4_data->board->max_width, 0, 0);
#endif
#endif
#ifdef PROXIMITY
	input_set_abs_params(rmi4_data->input_dev,
			ABS_MT_DISTANCE, 0,
			HOVER_Z_MAX, 0, 0);
#ifdef EDGE_SWIPE
	input_set_abs_params(rmi4_data->input_dev,
			ABS_MT_WIDTH_MAJOR, 0,
			EDGE_SWIPE_WIDTH_MAX, 0, 0);
	/*input_set_abs_params(rmi4_data->input_dev,
			ABS_MT_ANGLE, 0,
			EDGE_SWIPE_ANGLE_MAX, 0, 0);
	*/
	input_set_abs_params(rmi4_data->input_dev,
			ABS_MT_PALM, 0,
			EDGE_SWIPE_PALM_MAX, 0, 0);
#endif
	setup_timer(&rmi4_data->f51_finger_timer,
			synaptics_rmi4_f51_finger_timer,
			(unsigned long)rmi4_data);
#endif

#ifdef TYPE_B_PROTOCOL
	input_mt_init_slots(rmi4_data->input_dev,
			rmi4_data->num_of_fingers);
#endif

	f1a = NULL;
	if (!list_empty(&rmi->support_fn_list)) {
		list_for_each_entry(fhandler, &rmi->support_fn_list, link) {
			if (fhandler->fn_number == SYNAPTICS_RMI4_F1A)
				f1a = fhandler->data;
		}
	}

	if (f1a) {
		for (ii = 0; ii < f1a->valid_button_count; ii++) {
			set_bit(f1a->button_map[ii],
					rmi4_data->input_dev->keybit);
			input_set_capability(rmi4_data->input_dev,
					EV_KEY, f1a->button_map[ii]);
		}
	}

	retval = input_register_device(rmi4_data->input_dev);
	if (retval) {
		pr_debug("rmi-couldn't do the thing");
		goto err_register_input;
	}

	return 0;

err_register_input:
	input_free_device(rmi4_data->input_dev);

err_query_device:
	synaptics_rmi4_release_support_fn(rmi4_data);

err_input_device:
	return retval;
}

#ifdef CONFIG_TOUCHSCREEN_FACTORY_PLATFORM
#include <asm/uaccess.h>
#define LCD_LDI_FILE_PATH	"/sys/class/lcd/panel/window_type"
static int synaptics_rmi4_open_lcd_ldi(struct synaptics_rmi4_data *rmi4_data)
{
	int iRet = 0;
	mm_segment_t old_fs;
	struct file *window_type;
	unsigned char lcdtype[4] = {0,};

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	window_type = filp_open(LCD_LDI_FILE_PATH, O_RDONLY, 0666);
	if (IS_ERR(window_type)) {
		iRet = PTR_ERR(window_type);
		if (iRet != -ENOENT)
			pr_debug("rmi-couldn't do the thing");
		set_fs(old_fs);
		goto exit;
	}

	iRet = window_type->f_op->read(window_type, (u8 *)lcdtype, sizeof(u8) * 4, &window_type->f_pos);
	if (iRet != (sizeof(u8) * 4)) {
		pr_debug("rmi-couldn't do the thing");
		iRet = -EIO;
	}

	/* The variable of lcdtype has ASCII values(40 81 45) at 0x08 OCTA,
	  * so if someone need a TSP panel revision then to read third parameter.*/
	rmi4_data->factory_read_panel_wakeup = lcdtype[3] & 0x0F;

	filp_close(window_type, current->files);
	set_fs(old_fs);

exit:
	return iRet;
}
#endif

static int synaptics_rmi4_reinit_device(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned char ii = 0;
	unsigned short intr_addr;
	struct synaptics_rmi4_fn *fhandler;
	struct synaptics_rmi4_device_info *rmi;

	rmi = &(rmi4_data->rmi4_mod_info);

	if (!list_empty(&rmi->support_fn_list)) {
		list_for_each_entry(fhandler, &rmi->support_fn_list, link) {
			if (fhandler->fn_number == SYNAPTICS_RMI4_F12) {
				retval = synaptics_rmi4_f12_set_enables(rmi4_data, 0);
				if (retval < 0)
					return retval;
				break;
			}
		}
	}
#ifdef CONFIG_GLOVE_TOUCH
	synaptics_rmi4_glove_mode_enables(rmi4_data);
#endif
#ifdef PROXIMITY
	if (!f51) {
		pr_debug("rmi-couldn't do the thing");
		return -ENODEV;
	}
	pr_debug("rmi-couldn't do the thing");

#ifdef USE_CUSTOM_REZERO
	synaptics_rmi4_f51_set_custom_rezero(rmi4_data);

	msleep(100);
#endif

	retval = synaptics_rmi4_f51_set_enables(rmi4_data);
	if (retval < 0)
		return retval;

#endif
	for (ii = 0; ii < rmi4_data->num_of_intr_regs; ii++) {
		if (rmi4_data->intr_mask[ii] != 0x00) {
			pr_debug("rmi-couldn't do the thing");
			intr_addr = rmi4_data->f01_ctrl_base_addr + 1 + ii;
			retval = synaptics_rmi4_i2c_write(rmi4_data,
					intr_addr,
					&(rmi4_data->intr_mask[ii]),
					sizeof(rmi4_data->intr_mask[ii]));
			if (retval < 0)
				return retval;
		}
	}

	synaptics_rmi4_set_configured(rmi4_data);

	return 0;
}

static void synaptics_rmi4_release_all_finger(
	struct synaptics_rmi4_data *rmi4_data)
{
	int ii;

#ifdef TYPE_B_PROTOCOL
	for (ii = 0; ii < rmi4_data->num_of_fingers; ii++) {
		input_mt_slot(rmi4_data->input_dev, ii);
		input_mt_report_slot_state(rmi4_data->input_dev,
				MT_TOOL_FINGER, 0);
	}
#else
	input_mt_sync(rmi4_data->input_dev);
#endif
	input_report_key(rmi4_data->input_dev,
			BTN_TOUCH, 0);
#ifdef CONFIG_GLOVE_TOUCH
	input_report_switch(rmi4_data->input_dev,
		SW_GLOVE, false);
	rmi4_data->touchkey_glove_mode_status = false;
#endif
	input_sync(rmi4_data->input_dev);

	rmi4_data->fingers_on_2d = false;
#ifdef PROXIMITY
	rmi4_data->f51_finger = false;
#endif

#ifdef CONFIG_FAKE_DVFS
	synaptics_set_dvfs_lock(rmi4_data, -1);
#endif
}

int synaptics_rmi4_reset_device(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned char command = 0x01;

	mutex_lock(&(rmi4_data->rmi4_reset_mutex));

#ifdef CONFIG_KT_WAKE_FUNCS
	if (screen_is_off && screen_wake_options_when_off)
		disable_irq_wake(rmi4_data->i2c_client->irq);
#endif
	disable_irq(rmi4_data->i2c_client->irq);

	synaptics_rmi4_release_all_finger(rmi4_data);

	if (!rmi4_data->stay_awake) {
		retval = synaptics_rmi4_i2c_write(rmi4_data,
				rmi4_data->f01_cmd_base_addr,
				&command,
				sizeof(command));
		if (retval < 0) {
			pr_debug("rmi-couldn't do the thing");
			goto out;
		}

		/* A1(400msec) need more sleep time than B0(min 60msec) */
		if (rmi4_data->ic_revision_of_ic == 0xB0)
			msleep(SYNAPTICS_HW_RESET_TIME_B0);
		else
			msleep(SYNAPTICS_HW_RESET_TIME);

	} else {
#if defined(CONFIG_TOUCHSCREEN_SYNAPTICS_PREVENT_HSYNC_LEAKAGE)
		rmi4_data->board->hsync_onoff(false);
#endif
		rmi4_data->board->power(false);
		msleep(30);
		rmi4_data->board->power(true);
		rmi4_data->current_page = MASK_8BIT;

		/* A1(400msec) need more sleep time than B0(min 60msec) */
		if (rmi4_data->ic_revision_of_ic == 0xB0)
			msleep(SYNAPTICS_HW_RESET_TIME_B0);
		else
			msleep(SYNAPTICS_HW_RESET_TIME);

#if defined(CONFIG_TOUCHSCREEN_SYNAPTICS_PREVENT_HSYNC_LEAKAGE)
		rmi4_data->board->hsync_onoff(true);
#endif
		retval = synaptics_rmi4_f54_set_control(rmi4_data);
		if (retval < 0)
			pr_debug("rmi-couldn't do the thing");
	}

	synaptics_rmi4_release_support_fn(rmi4_data);

	retval = synaptics_rmi4_query_device(rmi4_data);
	if (retval < 0) {
		pr_debug("rmi-couldn't do the thing");
	}

out:
	enable_irq(rmi4_data->i2c_client->irq);
#ifdef CONFIG_KT_WAKE_FUNCS
	if (screen_is_off && screen_wake_options_when_off)
		enable_irq_wake(rmi4_data->i2c_client->irq);
#endif
	mutex_unlock(&(rmi4_data->rmi4_reset_mutex));

	return 0;
}

static void synaptics_charger_conn(struct synaptics_rmi4_data *rmi4_data,
				int ta_status)
{
	int retval;
	unsigned char charger_connected;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->f01_ctrl_base_addr,
			&charger_connected,
			sizeof(charger_connected));
	if (retval < 0)
		return;

	if (ta_status == 0x01 || ta_status == 0x03) {
		charger_connected |= CHARGER_CONNECTED;
#ifdef CONFIG_KT_WAKE_FUNCS
		ischarging = true;
		cancel_monitor_work = false;
		set_wakelock_options(true);
#endif
	} else {
		charger_connected &= CHARGER_DISCONNECTED;
#ifdef CONFIG_KT_WAKE_FUNCS
		ischarging = false;
		if (screen_wake_options_hold_wlock == 3) {
			cancel_monitor_work = true;
			wake_unlock(&wakelock);
		}
#endif
	}
#ifdef CONFIG_KT_WAKE_FUNCS
	ischarging_relay(ischarging);
#endif
	retval = synaptics_rmi4_i2c_write(rmi4_data,
		rmi4_data->f01_ctrl_base_addr,
		&charger_connected,
		sizeof(charger_connected));

	if (retval < 0) {
		pr_debug("rmi-couldn't do the thing");
	}
}

static void synaptics_ta_cb(struct synaptics_rmi_callbacks *cb, int ta_status)
{
	struct synaptics_rmi4_data *rmi4_data =
			container_of(cb, struct synaptics_rmi4_data, callbacks);

	rmi4_data->ta_status = ta_status;

	/* if do not completed driver loading, ta_cb will not run. */
	if (!rmi4_data->init_done.done)
		return;

	if (rmi4_data->touch_stopped || rmi4_data->doing_reflash)
		return;

	synaptics_charger_conn(rmi4_data, ta_status);

}

#ifdef PROXIMITY
static void synaptics_rmi4_f51_finger_timer(unsigned long data)
{
	struct synaptics_rmi4_data *rmi4_data =
			(struct synaptics_rmi4_data *)data;

	if (rmi4_data->f51_finger) {
		rmi4_data->f51_finger = false;
		mod_timer(&rmi4_data->f51_finger_timer,
				jiffies + msecs_to_jiffies(F51_FINGER_TIMEOUT));
	} else if (!rmi4_data->fingers_on_2d) {
#ifdef TYPE_B_PROTOCOL
		input_mt_slot(rmi4_data->input_dev, 0);
		input_mt_report_slot_state(rmi4_data->input_dev,
				MT_TOOL_FINGER, 0);
#if defined(CONFIG_TOUCHSCREEN_FACTORY_PLATFORM)
		if (rmi4_data->hover.state) {
			pr_debug("rmi-couldn't do the thing");
			rmi4_data->hover.mcount = 0;
			rmi4_data->hover.state = 0;
		}
#endif
#else
		input_mt_sync(rmi4_data->input_dev);
#endif
		input_sync(rmi4_data->input_dev);
	}

	return;
}
#endif

static void synaptics_rmi4_remove_exp_fn(struct synaptics_rmi4_data *rmi4_data)
{
	struct synaptics_rmi4_exp_fn *exp_fhandler, *n;

	if (list_empty(&exp_fn_list)) {
		pr_debug("rmi-couldn't do the thing");
		return;
	}

	list_for_each_entry_safe(exp_fhandler, n, &exp_fn_list, link) {
		if (exp_fhandler->initialized &&
				(exp_fhandler->func_remove != NULL)) {
			pr_debug("rmi-couldn't do the thing");
			exp_fhandler->func_remove(rmi4_data);
		}
		list_del(&exp_fhandler->link);
		kfree(exp_fhandler);
	}
}

static int synaptics_rmi4_init_exp_fn(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	struct synaptics_rmi4_exp_fn *exp_fhandler;

	INIT_LIST_HEAD(&exp_fn_list);

	retval = rmidev_module_register();
	if (retval < 0) {
		pr_debug("rmi-couldn't do the thing");
		goto error_exit;
	}

	retval = rmi4_f54_module_register();
	if (retval < 0) {
		pr_debug("rmi-couldn't do the thing");
		goto error_exit;
	}

	retval = rmi4_fw_update_module_register();
	if (retval < 0) {
		pr_debug("rmi-couldn't do the thing");
		goto error_exit;
	}

	if (list_empty(&exp_fn_list))
		return -ENODEV;

	list_for_each_entry(exp_fhandler, &exp_fn_list, link) {
		if (exp_fhandler->func_init != NULL) {
			pr_debug("rmi-couldn't do the thing");
			retval = exp_fhandler->func_init(rmi4_data);
			if (retval < 0) {
				pr_debug("rmi-couldn't do the thing");
				goto error_exit;
			} else {
				exp_fhandler->initialized = true;
			}
		}
	}

	return 0;

error_exit:
	synaptics_rmi4_remove_exp_fn(rmi4_data);

	return retval;
}

/**
* synaptics_rmi4_new_function()
*
* Called by other expansion Function modules in their module init and
* module exit functions.
*
* This function is used by other expansion Function modules such as
* rmi_dev to register themselves with the driver by providing their
* initialization and removal callback function pointers so that they
* can be inserted or removed dynamically at module init and exit times,
* respectively.
*/
int synaptics_rmi4_new_function(enum exp_fn fn_type,
		int (*func_init)(struct synaptics_rmi4_data *rmi4_data),
		void (*func_remove)(struct synaptics_rmi4_data *rmi4_data),
		void (*func_attn)(struct synaptics_rmi4_data *rmi4_data,
		unsigned char intr_mask))
{
	struct synaptics_rmi4_exp_fn *exp_fhandler;

	exp_fhandler = kzalloc(sizeof(*exp_fhandler), GFP_KERNEL);
	if (!exp_fhandler) {
		pr_debug("rmi-couldn't do the thing");
		return -ENOMEM;
	}
	exp_fhandler->fn_type = fn_type;
	exp_fhandler->func_init = func_init;
	exp_fhandler->func_attn = func_attn;
	exp_fhandler->func_remove = func_remove;
	list_add_tail(&exp_fhandler->link, &exp_fn_list);

	return 0;
}

#ifdef CONFIG_POWERSUSPEND
static void synaptics_init_power_on(struct work_struct *work)
{
	struct synaptics_rmi4_data *rmi4_data =
		container_of(work,
			struct synaptics_rmi4_data, work_init_power_on.work);

#ifdef CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_FULL_HD_PT_PANEL
	if (!touch_display_status) {
			pr_debug("rmi-couldn't do the thing");
			schedule_delayed_work(&rmi4_data->work_init_power_on,
					msecs_to_jiffies(1000));
	} else {
		synaptics_rmi4_power_resume(&rmi4_data->power_suspend);
	}
#else
	synaptics_rmi4_power_resume(&rmi4_data->power_suspend);
#endif
}
#endif

 /**
 * synaptics_rmi4_probe()
 *
 * Called by the kernel when an association with an I2C device of the
 * same name is made (after doing i2c_add_driver).
 *
 * This funtion allocates and initializes the resources for the driver
 * as an input driver, turns on the power to the sensor, queries the
 * sensor for its supported Functions and characteristics, registers
 * the driver to the input subsystem, sets up the interrupt, handles
 * the registration of the power_suspend and power_resume functions,
 * and creates a work queue for detection of other expansion Function
 * modules.
 */
static int synaptics_rmi4_probe(struct i2c_client *client,
		const struct i2c_device_id *dev_id)
{
	int retval;
#ifdef CONFIG_KT_WAKE_FUNCS
	int ret;
#endif
	unsigned char attr_count;
	int attr_count_num;
	struct synaptics_rmi4_data *rmi4_data;
	struct synaptics_rmi4_device_info *rmi;
	const struct synaptics_rmi4_platform_data *platform_data =
			client->dev.platform_data;
#ifdef CONFIG_KT_WAKE_FUNCS
	gdev = &client->dev;
#endif
	if (!i2c_check_functionality(client->adapter,
			I2C_FUNC_SMBUS_BYTE_DATA)) {
		pr_debug("rmi-couldn't do the thing");
		return -EIO;
	}

	if (!platform_data) {
		pr_debug("rmi-couldn't do the thing");
		return -EINVAL;
	}

	rmi4_data = kzalloc(sizeof(*rmi4_data), GFP_KERNEL);
	if (!rmi4_data) {
		pr_debug("rmi-couldn't do the thing");
		return -ENOMEM;
	}
#ifdef CONFIG_KT_WAKE_FUNCS
	ret = pm_runtime_set_active(&client->dev);
	if (ret < 0)
		pr_alert("KT_PM unable to set runtime pm state\n");
	pm_runtime_enable(&client->dev);
#endif
	rmi = &(rmi4_data->rmi4_mod_info);

	rmi4_data->i2c_client = client;
	rmi4_data->current_page = MASK_8BIT;
	rmi4_data->board = platform_data;
	rmi4_data->touch_stopped = false;
	rmi4_data->sensor_sleep = false;
	rmi4_data->irq_enabled = false;

	/* define panel version : M4 / M4+ */
	rmi4_data->panel_revision = rmi4_data->board->panel_revision;

#if defined(CONFIG_TOUCHSCREEN_SYNAPTICS_PREVENT_HSYNC_LEAKAGE)
	rmi4_data->board->hsync_onoff(false);
#endif
	rmi4_data->board->power(true);

#if defined(CONFIG_TOUCHSCREEN_SYNAPTICS_PREVENT_HSYNC_LEAKAGE)
	rmi4_data->board->hsync_onoff(true);
#endif
	rmi4_data->i2c_read = synaptics_rmi4_i2c_read;
	rmi4_data->i2c_write = synaptics_rmi4_i2c_write;
	rmi4_data->irq_enable = synaptics_rmi4_irq_enable;
	rmi4_data->reset_device = synaptics_rmi4_reset_device;
	rmi4_data->irq = rmi4_data->i2c_client->irq;

	mutex_init(&(rmi4_data->rmi4_io_ctrl_mutex));
	mutex_init(&(rmi4_data->rmi4_reset_mutex));
	mutex_init(&(rmi4_data->rmi4_reflash_mutex));
#ifdef CONFIG_KT_WAKE_FUNCS
	mutex_init(&scr_lock);
	
	x_lo = 1040 / 10;
	y_lo = 1700 / 20;
	x_onethird = (950 / 10) * 3;
	x_twothird = (950 / 10) * 6;
	x_hi = (950 / 10) * 9;
	y_hi = (1700 / 20) * 19;
	wake_lock_init(&wakelock, WAKE_LOCK_SUSPEND, "kt_wake_funcs");
	INIT_DELAYED_WORK(&wakelock_monitor, wakelock_monitor_func);
#endif
	init_completion(&rmi4_data->init_done);

	i2c_set_clientdata(client, rmi4_data);

	/*Direct IRQ for secure input*/
	retval = msm_gpio_install_direct_irq(rmi4_data->board->gpio, 0, 0);
	if (retval)
		pr_debug("rmi-couldn't do the thing");

#ifdef USE_CUSTOM_REZERO
	INIT_DELAYED_WORK(&rmi4_data->work_rezero, synaptics_work_rezero);
#endif

#ifdef CONFIG_FAKE_DVFS
	synaptics_init_dvfs(rmi4_data);
#endif

	retval = synaptics_rmi4_set_input_device(rmi4_data);
	if (retval < 0) {
		pr_debug("rmi-couldn't do the thing");
		goto err_set_input_device;
	}


	retval = synaptics_rmi4_init_exp_fn(rmi4_data);
	if (retval < 0) {
		pr_debug("rmi-couldn't do the thing");
		goto err_init_exp_fn;
	}

	retval = synaptics_rmi4_irq_enable(rmi4_data, true);
	if (retval < 0) {
		pr_debug("rmi-couldn't do the thing");
		goto err_enable_irq;
	}
#ifdef CONFIG_KT_WAKE_FUNCS
	device_init_wakeup(&client->dev, 1);
#endif
	for (attr_count = 0; attr_count < ARRAY_SIZE(attrs); attr_count++) {
		retval = sysfs_create_file(&rmi4_data->input_dev->dev.kobj,
				&attrs[attr_count].attr);
		if (retval < 0) {
			pr_debug("rmi-couldn't do the thing");
			goto err_sysfs;
		}
	}

	if (rmi4_data->board->factory_flatform) {
		retval = synaptics_fw_updater(NULL, false, true);
	} else {
		if (rmi4_data->board->panel_revision >= OCTA_PANEL_REVISION_43)
			retval = synaptics_fw_updater(NULL, false, false);
#if defined(CONFIG_MACH_JACTIVE_EUR) || defined(CONFIG_MACH_JACTIVE_ATT)
		else if (rmi4_data->board->panel_revision == OCTA_PANEL_REVISION_34) {
			retval = synaptics_fw_updater(NULL, false, false);
		}
#endif
		else
			pr_debug("rmi-couldn't do the thing");
	}
	if (retval < 0) {
		pr_debug("rmi-couldn't do the thing");
	}

#ifdef CONFIG_POWERSUSPEND
	rmi4_data->power_suspend.suspend = synaptics_rmi4_power_suspend;
	rmi4_data->power_suspend.resume = synaptics_rmi4_power_resume;
	register_power_suspend(&rmi4_data->power_suspend);
#endif

	rmi4_data->register_cb = rmi4_data->board->register_cb;

	rmi4_data->callbacks.inform_charger = synaptics_ta_cb;
	if (rmi4_data->register_cb)
		rmi4_data->register_cb(&rmi4_data->callbacks);

#ifdef CONFIG_POWERSUSPEND
	/* turn off TSP power. after LCD module on, TSP power will turn on */
	synaptics_rmi4_power_suspend(&rmi4_data->power_suspend);

	INIT_DELAYED_WORK(&rmi4_data->work_init_power_on,
					synaptics_init_power_on);
	schedule_delayed_work(&rmi4_data->work_init_power_on,
					msecs_to_jiffies(6000));
#endif

	/* for blocking to be excuted open function until probing */
	complete_all(&rmi4_data->init_done);

#ifdef CONFIG_TOUCH_WAKE
	// Yank555.lu - Store the data for touchwake
	touchwake_data = rmi4_data;
	if (touchwake_data == NULL)
		pr_err("[TOUCHWAKE] Failed to set Synaptics touchwake_data\n");
#endif 

	return retval;

err_sysfs:
	attr_count_num = (int)attr_count;
	for (attr_count_num--; attr_count_num >= 0; attr_count_num--) {
		sysfs_remove_file(&rmi4_data->input_dev->dev.kobj,
				&attrs[attr_count].attr);
	}
	synaptics_rmi4_irq_enable(rmi4_data, false);

err_enable_irq:
	synaptics_rmi4_remove_exp_fn(rmi4_data);

err_init_exp_fn:
	synaptics_rmi4_release_support_fn(rmi4_data);

	input_unregister_device(rmi4_data->input_dev);
	rmi4_data->input_dev = NULL;

err_set_input_device:
	rmi4_data->board->power(false);
	complete_all(&rmi4_data->init_done);
	kfree(rmi4_data);

	return retval;
}

 /**
 * synaptics_rmi4_remove()
 *
 * Called by the kernel when the association with an I2C device of the
 * same name is broken (when the driver is unloaded).
 *
 * This funtion terminates the work queue, stops sensor data acquisition,
 * frees the interrupt, unregisters the driver from the input subsystem,
 * turns off the power to the sensor, and frees other allocated resources.
 */
static int synaptics_rmi4_remove(struct i2c_client *client)
{
	unsigned char attr_count;
	struct synaptics_rmi4_data *rmi4_data = i2c_get_clientdata(client);
	struct synaptics_rmi4_device_info *rmi;
	/*const struct synaptics_rmi4_platform_data *platform_data =
			rmi4_data->board;*/

	rmi = &(rmi4_data->rmi4_mod_info);
#ifdef CONFIG_POWERSUSPEND
	unregister_power_suspend(&rmi4_data->power_suspend);
#endif

	synaptics_rmi4_irq_enable(rmi4_data, false);

	for (attr_count = 0; attr_count < ARRAY_SIZE(attrs); attr_count++) {
		sysfs_remove_file(&rmi4_data->input_dev->dev.kobj,
				&attrs[attr_count].attr);
	}

	input_unregister_device(rmi4_data->input_dev);

	synaptics_rmi4_remove_exp_fn(rmi4_data);

	rmi4_data->board->power(false);
	rmi4_data->touch_stopped = true;

	synaptics_rmi4_release_support_fn(rmi4_data);

	input_free_device(rmi4_data->input_dev);

	kfree(rmi4_data);

	return 0;
}

#ifdef USE_SENSOR_SLEEP
 /**
 * synaptics_rmi4_sensor_sleep()
 *
 * Called by synaptics_rmi4_power_suspend() and synaptics_rmi4_suspend().
 *
 * This function stops finger data acquisition and puts the sensor to sleep.
 */
static void synaptics_rmi4_sensor_sleep(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned char device_ctrl;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->f01_ctrl_base_addr,
			&device_ctrl,
			sizeof(device_ctrl));
	if (retval < 0) {
		pr_debug("rmi-couldn't do the thing");
		rmi4_data->sensor_sleep = false;
		return;
	}

	device_ctrl = (device_ctrl & ~MASK_3BIT);
	device_ctrl = (device_ctrl | NO_SLEEP_OFF | SENSOR_SLEEP);

	retval = synaptics_rmi4_i2c_write(rmi4_data,
			rmi4_data->f01_ctrl_base_addr,
			&device_ctrl,
			sizeof(device_ctrl));
	if (retval < 0) {
		pr_debug("rmi-couldn't do the thing");
		rmi4_data->sensor_sleep = false;
		return;
	} else {
		rmi4_data->sensor_sleep = true;
	}

	return;
}

 /**
 * synaptics_rmi4_sensor_wake()
 *
 * Called by synaptics_rmi4_resume() and synaptics_rmi4_power_resume().
 *
 * This function wakes the sensor from sleep.
 */
static void synaptics_rmi4_sensor_wake(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned char device_ctrl;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->f01_ctrl_base_addr,
			&device_ctrl,
			sizeof(device_ctrl));
	if (retval < 0) {
		pr_debug("rmi-couldn't do the thing");
		rmi4_data->sensor_sleep = true;
		return;
	}

	device_ctrl = (device_ctrl & ~MASK_3BIT);
	device_ctrl = (device_ctrl | NO_SLEEP_OFF | NORMAL_OPERATION);

	retval = synaptics_rmi4_i2c_write(rmi4_data,
			rmi4_data->f01_ctrl_base_addr,
			&device_ctrl,
			sizeof(device_ctrl));
	if (retval < 0) {
		pr_debug("rmi-couldn't do the thing");
		rmi4_data->sensor_sleep = true;
		return;
	} else {
		rmi4_data->sensor_sleep = false;
	}

	return;
}
#endif

#ifdef USE_OPEN_CLOSE
static int synaptics_rmi4_input_open(struct input_dev *dev)
{
	struct synaptics_rmi4_data *rmi4_data = input_get_drvdata(dev);
	int ret;

	ret = wait_for_completion_interruptible_timeout(&rmi4_data->init_done,
			msecs_to_jiffies(90 * MSEC_PER_SEC));

	if (ret < 0) {
		ret = -ENXIO;
		goto err_open;
	}
	if (ret == 0) {
		ret = -ENXIO;
		goto err_open;
	}

	if (rmi4_data->touch_stopped) {

#if defined(CONFIG_TOUCHSCREEN_SYNAPTICS_PREVENT_HSYNC_LEAKAGE)
		rmi4_data->board->hsync_onoff(false);
#endif
		rmi4_data->board->power(true);
		rmi4_data->touch_stopped = false;

#if defined(CONFIG_TOUCHSCREEN_SYNAPTICS_PREVENT_HSYNC_LEAKAGE)
		rmi4_data->board->hsync_onoff(true);
#endif
		ret = synaptics_rmi4_reinit_device(rmi4_data);
		if (ret < 0) {
			pr_debug("rmi-couldn't do the thing");
		}

		enable_irq(rmi4_data->i2c_client->irq);
	}

	return 0;

err_open:
	return ret;
}

static void synaptics_rmi4_input_close(struct input_dev *dev)
{
	struct synaptics_rmi4_data *rmi4_data = input_get_drvdata(dev);
#ifdef CONFIG_TOUCH_WAKE
	// Don't change state if touchwake handles this
	if (!touchwake_is_active()) {
		#ifdef TOUCHWAKE_DEBUG_PRINT
		pr_info("[TOUCHWAKE] Synaptics input close\n");
		#endif
#endif
	if (!rmi4_data->touch_stopped) {
		disable_irq(rmi4_data->i2c_client->irq);
#ifdef CONFIG_KT_WAKE_FUNCS
		rmi4_data->irq_enabled = false;
#endif
		rmi4_data->board->power(false);
		rmi4_data->touch_stopped = true;
		synaptics_rmi4_release_all_finger(rmi4_data);
	} else {
		pr_debug("rmi-couldn't do the thing");
	}
#ifdef CONFIG_TOUCH_WAKE
	} else {
		#ifdef TOUCHWAKE_DEBUG_PRINT
		pr_info("[TOUCHWAKE] Synaptics suspend not allowed at the moment\n");
		#endif
	}
}
#endif

#ifdef CONFIG_POWERSUSPEND
#define synaptics_rmi4_suspend NULL
#define synaptics_rmi4_resume NULL
#ifdef CONFIG_KT_WAKE_FUNCS
void notif_wakelock_forwake_funcs(bool state)
{
	if (screen_wake_options_hold_wlock == 2 || (screen_wake_options_hold_wlock >= 12 && screen_wake_options_hold_wlock <= 19))
	{
		if (state && !wake_lock_active(&wakelock))
		{
			schedule_delayed_work_on(0, &wakelock_monitor, msecs_to_jiffies(5000));
			cancel_monitor_work = false;
			wakelock_time_remaining = 0;
			
			if (screen_wake_options_hold_wlock == 2)
				wake_lock(&wakelock);
			else if (screen_wake_options_hold_wlock == 12)
				wakelock_time_remaining = 30000;
			else if (screen_wake_options_hold_wlock == 13)
				wakelock_time_remaining = 60000;
			else if (screen_wake_options_hold_wlock == 14)
				wakelock_time_remaining = 120000;
			else if (screen_wake_options_hold_wlock == 15)
				wakelock_time_remaining = 300000;
			else if (screen_wake_options_hold_wlock == 16)
				wakelock_time_remaining = 600000;
			else if (screen_wake_options_hold_wlock == 17)
				wakelock_time_remaining = 1800000;
			else if (screen_wake_options_hold_wlock == 18)
				wakelock_time_remaining = 3600000;
			else if (screen_wake_options_hold_wlock == 19)
				wakelock_time_remaining = 7200000;

			if (wakelock_time_remaining)
				wake_lock_timeout(&wakelock, msecs_to_jiffies(wakelock_time_remaining));
		}
		else if (!state && wake_lock_active(&wakelock))
		{
			cancel_monitor_work = true;
			wake_unlock(&wakelock);
		}
	}
}

void set_screen_synaptic_off(void)
{
	int retval;
	struct synaptics_rmi4_data *rmi4_data;
	
	if (gdev == NULL)
		return;
	rmi4_data = dev_get_drvdata(gdev);
	
	screen_is_off = true;
	
	mutex_lock(&rmi4_data->input_dev->mutex);
	set_wakelock_options(true);
	if (screen_wake_options && !call_in_progress)
	{
		char chTempbuf[2] = { 1, 20};

		if (screen_wake_options_debug) pr_alert("SCREEN POWER OFF1 - %d - %d - %d", screen_wake_options, ischarging, call_in_progress);

		if (main_prox_data != NULL)
		{
			send_instruction(main_prox_data, ADD_SENSOR, PROXIMITY_RAW, chTempbuf, 2);
			main_prox_data->bProximityRawEnabled = true;
		}

		retval = synaptics_rmi4_reset_device(rmi4_data);
		if (retval < 0) {
			pr_alert("%s: Failed to issue reset command, error = %d\n",
					__func__, retval);
		}
		enable_irq_wake(rmi4_data->i2c_client->irq);
		screen_wake_options_when_off = screen_wake_options;
	}
	else
	{
		if (screen_wake_options_debug) pr_alert("SCREEN POWER OFF2 - %d - %d - %d", screen_wake_options, ischarging, call_in_progress);
		rmi4_data->stay_awake = false;
		rmi4_data->staying_awake = false;
		disable_irq(rmi4_data->i2c_client->irq);
		rmi4_data->irq_enabled = false;
		rmi4_data->board->power(false);
		rmi4_data->touch_stopped = true;
		gpio_free(rmi4_data->board->gpio);
		
		screen_wake_options_when_off = 0;
	}
	// release all finger when entered suspend 
	synaptics_rmi4_release_all_finger(rmi4_data);
	
	mutex_unlock(&rmi4_data->input_dev->mutex);
}


void set_screen_synaptic_on(void)
{
	int retval;
	int ret;
	struct synaptics_rmi4_data *rmi4_data;
	
	if (gdev == NULL)
		return;
	rmi4_data = dev_get_drvdata(gdev);

	mutex_lock(&rmi4_data->input_dev->mutex);

	wake_start = 0;
	last_touch_time = 0;
	screen_is_off = false;
	
	if (screen_wake_options_hold_wlock || wake_lock_active(&wakelock))
		wake_unlock(&wakelock);

	rmi4_data->board->power(true);
	rmi4_data->touch_stopped = false;
	rmi4_data->current_page = MASK_8BIT;

	if (screen_wake_options_when_off && !call_in_progress)
	{
		char chTempbuf[2] = { 1, 20};
		if (screen_wake_options_debug) pr_alert("SCREEN POWER ON1 - %d - %d - %d - %d", screen_wake_options, screen_wake_options_when_off, ischarging, call_in_progress);
		if (main_prox_data != NULL)
		{
			send_instruction(main_prox_data, REMOVE_SENSOR, PROXIMITY_RAW, chTempbuf, 2);
			main_prox_data->bProximityRawEnabled = false;
		}

		disable_irq_wake(rmi4_data->i2c_client->irq);
	}
	else
	{
		if (screen_wake_options_debug)
			pr_alert("SCREEN POWER ON2 - %d - %d - %d - %d",
					 screen_wake_options, screen_wake_options_when_off,
					 ischarging, call_in_progress);
		retval = gpio_request(rmi4_data->board->gpio, "tsp_int");
		if (retval != 0) {
			dev_info(&rmi4_data->i2c_client->dev,
					 "%s: tsp int request failed, ret=%d", __func__, retval);
		}
	}
	
	ret = synaptics_rmi4_reinit_device(rmi4_data);
	if (ret < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to reinit device\n",
				__func__);
	}

	if (rmi4_data->ta_status)
		synaptics_charger_conn(rmi4_data, rmi4_data->ta_status);
	if (!rmi4_data->irq_enabled)
	{
		enable_irq(rmi4_data->i2c_client->irq);
		if (screen_wake_options_when_off)
			pr_alert("IRQ DISABLED FROM EXTERNAL");
	}
	rmi4_data->irq_enabled = true;

	retval = rmi4_data->board->tout1_on();
	if (retval)
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: touch_tout1_on failed\n", __func__);
	mutex_unlock(&rmi4_data->input_dev->mutex);
}
#endif
 /**
 * synaptics_rmi4_power_suspend()
 *
 * Called by the kernel during the power suspend phase when the system
 * enters suspend.
 *
 * This function calls synaptics_rmi4_sensor_sleep() to stop finger
 * data acquisition and put the sensor to sleep.
 */
static void synaptics_rmi4_power_suspend(struct power_suspend *h)
{
	struct synaptics_rmi4_data *rmi4_data =
			container_of(h, struct synaptics_rmi4_data,
			power_suspend);

		if (rmi4_data->stay_awake) {
			rmi4_data->staying_awake = true;
			return;
		} else {
			rmi4_data->staying_awake = false;
		}

		if (!rmi4_data->touch_stopped) {
			pr_debug("rmi-couldn't do the thing");

			disable_irq(rmi4_data->i2c_client->irq);
			rmi4_data->board->power(false);
			rmi4_data->touch_stopped = true;

			gpio_free(rmi4_data->board->gpio);
			/* release all finger when entered suspend */
			synaptics_rmi4_release_all_finger(rmi4_data);
		}
}

 /**
 * synaptics_rmi4_power_resume()
 *
 * Called by the kernel during the late resume phase when the system
 * wakes up from suspend.
 *
 * This function goes through the sensor wake process if the system wakes
 * up from power suspend (without going into suspend).
 */
static void synaptics_rmi4_power_resume(struct power_suspend *h)
{
	struct synaptics_rmi4_data *rmi4_data =
			container_of(h, struct synaptics_rmi4_data,
			power_suspend);
	int retval;

		if (rmi4_data->staying_awake)
			return;

		if (rmi4_data->touch_stopped) {
			pr_debug("rmi-couldn't do the thing");

#if defined(CONFIG_TOUCHSCREEN_SYNAPTICS_PREVENT_HSYNC_LEAKAGE)
			rmi4_data->board->hsync_onoff(false);
#endif
			rmi4_data->board->power(true);
			rmi4_data->touch_stopped = false;
			rmi4_data->current_page = MASK_8BIT;

#if defined(CONFIG_TOUCHSCREEN_SYNAPTICS_PREVENT_HSYNC_LEAKAGE)
			rmi4_data->board->hsync_onoff(true);
#endif
			retval = gpio_request(rmi4_data->board->gpio, "tsp_int");
			if (retval != 0) {
				pr_debug("rmi-couldn't do the thing");
				return ;
			}
#ifdef CONFIG_TOUCHSCREEN_FACTORY_PLATFORM
			retval = synaptics_rmi4_query_device(rmi4_data);
			if (retval < 0)
				pr_debug("rmi-couldn't do the thing");
			retval = synaptics_rmi4_open_lcd_ldi(rmi4_data);
			if (retval < 0)
				pr_debug("rmi-couldn't do the thing");

#else
			retval = synaptics_rmi4_reinit_device(rmi4_data);
			if (retval < 0) {
				pr_debug("rmi-couldn't do the thing");
			}
#endif
			if (rmi4_data->ta_status)
				synaptics_charger_conn(rmi4_data, rmi4_data->ta_status);

			enable_irq(rmi4_data->i2c_client->irq);
		}
#ifdef CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_FULL_HD_PT_PANEL
		retval = rmi4_data->board->tout1_on();
		if (retval)
			pr_debug("rmi-couldn't do the thing");
#endif
		return;
}
#else

 /**
 * synaptics_rmi4_suspend()
 *
 * Called by the kernel during the suspend phase when the system
 * enters suspend.
 *
 * This function stops finger data acquisition and puts the sensor to
 * sleep (if not already done so during the power suspend phase),
 * disables the interrupt, and turns off the power to the sensor.
 */
static int synaptics_rmi4_suspend(struct device *dev)
{
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

	if (rmi4_data->staying_awake)
		return 0;

	mutex_lock(&rmi4_data->input_dev->mutex);

	if (rmi4_data->input_dev->users) {
		if (!rmi4_data->touch_stopped) {
			disable_irq(rmi4_data->i2c_client->irq);
#ifdef CONFIG_KT_WAKE_FUNCS
			rmi4_data->irq_enabled = false;
#endif
			synaptics_rmi4_release_all_finger(rmi4_data);
			rmi4_data->board->power(false);
			rmi4_data->touch_stopped = true;
		} else {
			pr_debug("rmi-couldn't do the thing");
		}
	}
	mutex_unlock(&rmi4_data->input_dev->mutex);

	return 0;
}

 /**
 * synaptics_rmi4_resume()
 *
 * Called by the kernel during the resume phase when the system
 * wakes up from suspend.
 *
 * This function turns on the power to the sensor, wakes the sensor
 * from sleep, enables the interrupt, and starts finger data
 * acquisition.
 */
static int synaptics_rmi4_resume(struct device *dev)
{
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);
	int retval;

	mutex_lock(&rmi4_data->input_dev->mutex);

	if (rmi4_data->input_dev->users) {
		if (rmi4_data->touch_stopped) {
			rmi4_data->board->power(true);
			rmi4_data->touch_stopped = false;

			retval = synaptics_rmi4_reinit_device(rmi4_data);
			if (retval < 0) {
				pr_debug("rmi-couldn't do the thing");
			}
			enable_irq(rmi4_data->i2c_client->irq);

			pr_debug("rmi-couldn't do the thing");
		} else {
			pr_debug("rmi-couldn't do the thing");
		}
	}

	mutex_unlock(&rmi4_data->input_dev->mutex);

	return 0;
}
#endif

#ifdef CONFIG_TOUCH_WAKE
// Yank555.lu - Add hooks to enable / disable the digitizer for touchwake
void touchscreen_disable(void)
{
	#ifdef TOUCHWAKE_DEBUG_PRINT
	pr_info("[TOUCHWAKE] Synaptics disable\n");
	#endif

	if (touchwake_data != NULL)
		synaptics_rmi4_input_close(touchwake_data->input_dev);

	return;
}
EXPORT_SYMBOL(touchscreen_disable);

void touchscreen_enable(void)
{
	#ifdef TOUCHWAKE_DEBUG_PRINT
	pr_info("[TOUCHWAKE] Synaptics enable\n");
	#endif

	if (touchwake_data != NULL)
		synaptics_rmi4_input_open(touchwake_data->input_dev);

	return;
}
EXPORT_SYMBOL(touchscreen_enable);
#endif

#if 0
static const struct dev_pm_ops synaptics_rmi4_dev_pm_ops = {
	.suspend = synaptics_rmi4_suspend,
	.resume  = synaptics_rmi4_resume,
};
#endif

static const struct i2c_device_id synaptics_rmi4_id_table[] = {
	{DRIVER_NAME, 0},
	{},
};
MODULE_DEVICE_TABLE(i2c, synaptics_rmi4_id_table);

static struct i2c_driver synaptics_rmi4_driver = {
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
#if 0
		.pm = &synaptics_rmi4_dev_pm_ops,
#endif
	},
	.probe = synaptics_rmi4_probe,
	.remove = synaptics_rmi4_remove,
	.id_table = synaptics_rmi4_id_table,
};

 /**
 * synaptics_rmi4_init()
 *
 * Called by the kernel during do_initcalls (if built-in)
 * or when the driver is loaded (if a module).
 *
 * This function registers the driver to the I2C subsystem.
 *
 */
static int __init synaptics_rmi4_init(void)
{
#ifdef CONFIG_SAMSUNG_LPM_MODE
	if (poweroff_charging) {
		pr_notice("%s : LPM Charging Mode!!\n", __func__);
		return 0;
	}
#endif
	return i2c_add_driver(&synaptics_rmi4_driver);
}

 /**
 * synaptics_rmi4_exit()
 *
 * Called by the kernel when the driver is unloaded.
 *
 * This funtion unregisters the driver from the I2C subsystem.
 *
 */
static void __exit synaptics_rmi4_exit(void)
{
	i2c_del_driver(&synaptics_rmi4_driver);
}

module_init(synaptics_rmi4_init);
module_exit(synaptics_rmi4_exit);

MODULE_AUTHOR("Synaptics, Inc.");
MODULE_DESCRIPTION("Synaptics RMI4 I2C Touch Driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION(SYNAPTICS_RMI4_DRIVER_VERSION);
