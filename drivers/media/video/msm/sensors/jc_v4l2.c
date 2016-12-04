/* Copyright (c) 2011, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/delay.h>
#include <linux/debugfs.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/vmalloc.h>
#include <linux/firmware.h>
#include <media/msm_camera.h>
#include <media/v4l2-subdev.h>
#include <mach/gpio.h>
#include <mach/camera.h>
#include <mach/msm_bus.h>
#include <mach/msm_bus_board.h>

#include <asm/mach-types.h>
#include <mach/vreg.h>
#include <linux/io.h>
#include <linux/ctype.h>

#include "msm_sensor_common.h"
#include "msm.h"
#include "jc.h"
#include "jc_regs.h"
#include "msm.h"
#include "msm_ispif.h"
#include "msm_sensor.h"

#define JC_ISP_TIMEOUT		3000

#define JC_LOAD_FW_MAIN	1
#define JC_DUMP_FW	1
#define JC_CHECK_FW	1
#define JC_MEM_READ	1
#define ISP_DEBUG_LOG	0
#define JC_SPI_WRITE
#define FW_WRITE_SIZE 262144 /*2097152*/
#define VERIFY_CHIP_ERASED 32
#define ISP_FROM_ERASED 1


#define jc_readb(g, b, v) jc_read(__LINE__, 1, g, b, v, true)
#define jc_readw(g, b, v) jc_read(__LINE__, 2, g, b, v, true)
#define jc_readl(g, b, v) jc_read(__LINE__, 4, g, b, v, true)

#define jc_writeb(g, b, v) jc_write(__LINE__, 1, g, b, v, true)
#define jc_writew(g, b, v) jc_write(__LINE__, 2, g, b, v, true)
#define jc_writel(g, b, v) jc_write(__LINE__, 4, g, b, v, true)

#define jc_readb2(g, b, v) jc_read(__LINE__, 1, g, b, v, false)
#define jc_readw2(g, b, v) jc_read(__LINE__, 2, g, b, v, false)
#define jc_readl2(g, b, v) jc_read(__LINE__, 4, g, b, v, false)

#define jc_writeb2(g, b, v) jc_write(__LINE__, 1, g, b, v, false)
#define jc_writew2(g, b, v) jc_write(__LINE__, 2, g, b, v, false)
#define jc_writel2(g, b, v) jc_write(__LINE__, 4, g, b, v, false)

#ifdef JC_SPI_WRITE
//static char *Fbuf; /*static QCTK*/
//static char FW_buf[2197152] = {0}; /*static QCTK 2MB*/
#endif
#define CHECK_ERR(x)	if ((x) < 0) { \
				pr_debug("I LOVE TO PRINT THINGS\n"); \
				return x; \
			}

struct jc_work {
	struct work_struct work;
};

static struct i2c_client *jc_client;
static struct msm_sensor_ctrl_t jc_s_ctrl;
static struct device jc_dev;
struct class *camera_class;
#if JC_CHECK_FW
static u8 sysfs_sensor_fw_str[12] = {0,};
static u8 sysfs_isp_fw_str[12] = {0,};
static u8 sysfs_phone_fw_str[12] = {0,};
static u8 sysfs_sensor_type[25] = {0,};
#endif
#if JC_LOAD_FW_MAIN
bool firmware_update_sdcard;
bool version_checked;
#endif

struct jc_ctrl_t {
	const struct msm_camera_sensor_info *sensordata;
	struct msm_camera_i2c_client *sensor_i2c_client;
	struct v4l2_subdev *sensor_dev;
	struct v4l2_subdev sensor_v4l2_subdev;
	struct v4l2_subdev_info *sensor_v4l2_subdev_info;
	uint8_t sensor_v4l2_subdev_info_size;
	struct v4l2_subdev_ops *sensor_v4l2_subdev_ops;
	struct jc_isp isp;

	int op_mode;
	int dtp_mode;
	int cam_mode;
	int vtcall_mode;
	int started;
	int flash_mode;
	int lowLight;
	int dtpTest;
	int af_mode;
	int af_status;
	int preview_size;
	int capture_size;
	unsigned int lux;
	int awb_mode;
	int samsungapp;
	u8 sensor_ver[10];
	u8 phone_ver[10];
	u8 isp_ver[10];
	u8 sensor_ver_str[12];
	u8 isp_ver_str[12];
	u8 phone_ver_str[12];
	u8 sensor_type[25];
	bool burst_mode;
	bool stream_on;
	bool touch_af_mode;
	bool hw_vdis_mode;
	bool torch_mode;
	bool fw_update;
	int system_rev;
	bool movie_mode;
	int shot_mode;
	bool anti_stream_off;
	int sensor_combination;
	bool need_restart_caf;
	bool is_isp_null;
	bool isp_null_read_sensor_fw;
	bool samsung_app;
	bool factory_bin;
	int fw_retry_cnt;
};

static struct jc_ctrl_t *jc_ctrl;

struct jc_format {
	enum v4l2_mbus_pixelcode code;
	enum v4l2_colorspace colorspace;
	u16 fmt;
	u16 order;
};
static int32_t jc_sensor_setting(struct msm_sensor_ctrl_t *s_ctrl,
			int update_type, int res);
static u32 jc_wait_interrupt(unsigned int timeout);
static int jc_sensor_power_reset(struct msm_sensor_ctrl_t *s_ctrl);
static int jc_sensor_power_down(struct msm_sensor_ctrl_t *s_ctrl);
static int jc_set_sizes(void);
static int jc_check_chip_erase(void);

static DECLARE_WAIT_QUEUE_HEAD(jc_wait_queue);
static int jc_sensor_config(struct msm_sensor_ctrl_t *s_ctrl,
	void __user *argp);
DEFINE_MUTEX(jc_mut);


static int jc_read(int _line, u8 len,
	u8 category, u8 byte, int *val, bool log)
{
	struct i2c_msg msg;
	unsigned char data[5];
	unsigned char recv_data[len + 1];
	int i, err = 0;

	if (!jc_client->adapter)
		return -ENODEV;

	if (len != 0x01 && len != 0x02 && len != 0x04)
		return -EINVAL;

	msg.addr = jc_client->addr >> 1;
	msg.flags = 0;
	msg.len = sizeof(data);
	msg.buf = data;

	/* high byte goes out first */
	data[0] = msg.len;
	data[1] = 0x01;			/* Read category parameters */
	data[2] = category;
	data[3] = byte;
	data[4] = len;

	for (i = JC_I2C_RETRY; i; i--) {
		err = i2c_transfer(jc_client->adapter, &msg, 1);
		if (err == 1)
			break;
		msleep(20);
	}

	if (err != 1) {
		return err;
	}

	msg.flags = I2C_M_RD;
	msg.len = sizeof(recv_data);
	msg.buf = recv_data;
	for (i = JC_I2C_RETRY; i; i--) {
		err = i2c_transfer(jc_client->adapter, &msg, 1);
		if (err == 1)
			break;
		msleep(20);
	}

	if (err != 1) {
		return err;
	}

	if (recv_data[0] != sizeof(recv_data))
		pr_debug("8===D");

	if (len == 0x01)
		*val = recv_data[1];
	else if (len == 0x02)
		*val = recv_data[1] << 8 | recv_data[2];
	else
		*val = recv_data[1] << 24 | recv_data[2] << 16 |
				recv_data[3] << 8 | recv_data[4];

	return err;
}

static int jc_write(int _line, u8 len,
	u8 category, u8 byte, int val, bool log)
{
	struct i2c_msg msg;
	unsigned char data[len + 4];
	int i, err;

	if (!jc_client->adapter)
		return -ENODEV;

	if (len != 0x01 && len != 0x02 && len != 0x04)
		return -EINVAL;

	msg.addr = jc_client->addr >> 1;
	msg.flags = 0;
	msg.len = sizeof(data);
	msg.buf = data;

	data[0] = msg.len;
	data[1] = 0x02;			/* Write category parameters */
	data[2] = category;
	data[3] = byte;
	if (len == 0x01) {
		data[4] = (val & 0xFF);
	} else if (len == 0x02) {
		data[4] = ((val >> 8) & 0xFF);
		data[5] = (val & 0xFF);
	} else {
		data[4] = ((val >> 24) & 0xFF);
		data[5] = ((val >> 16) & 0xFF);
		data[6] = ((val >> 8) & 0xFF);
		data[7] = (val & 0xFF);
	}

	for (i = JC_I2C_RETRY; i; i--) {
		err = i2c_transfer(jc_client->adapter, &msg, 1);
		if (err == 1)
			break;
		msleep(20);
	}

	if (err != 1) {
		return err;
	}

	return err;
}

#if JC_DUMP_FW
static int jc_mem_dump(u16 len, u32 addr, u8 *val)
{
	struct i2c_msg msg;
	unsigned char data[8];
	unsigned char recv_data[len + 3];
	int i, err = 0;

	if (!jc_client->adapter)
		return -ENODEV;

	if (len <= 0)
		return -EINVAL;

	msg.addr = jc_client->addr >> 1;
	msg.flags = 0;
	msg.len = sizeof(data);
	msg.buf = data;

	/* high byte goes out first */
	data[0] = 0x00;
	data[1] = 0x03;
	data[2] = 0x18;
	data[3] = (addr >> 16) & 0xFF;
	data[4] = (addr >> 8) & 0xFF;
	data[5] = addr & 0xFF;
	data[6] = (len >> 8) & 0xFF;
	data[7] = len & 0xFF;

	for (i = JC_I2C_RETRY; i; i--) {
		err = i2c_transfer(jc_client->adapter, &msg, 1);
		if (err == 1)
			break;
		else

		msleep(20);
	}

	if (err != 1)
		return err;

	msg.flags = I2C_M_RD;
	msg.len = sizeof(recv_data);
	msg.buf = recv_data;
	for (i = JC_I2C_RETRY; i; i--) {
		err = i2c_transfer(jc_client->adapter, &msg, 1);
		if (err == 1)
			break;
		else

		msleep(20);
	}

	if (err != 1)
		return err;

	if (len != (recv_data[1] << 8 | recv_data[2]))

	memcpy(val, recv_data + 3, len);

	/*cam_i2c_dbg("address %#x, length %d\n", addr, len);*/	/*for debug*/
	return err;
}
#endif

#if JC_MEM_READ
static int jc_mem_read(u16 len, u32 addr, u8 *val)
{
	struct i2c_msg msg;
	unsigned char data[8];
	unsigned char recv_data[len + 3];
	int i, err = 0;

	if (!jc_client->adapter)
		return -ENODEV;

	if (len <= 0)
		return -EINVAL;

	msg.addr = jc_client->addr >> 1;
	msg.flags = 0;
	msg.len = sizeof(data);
	msg.buf = data;

	/* high byte goes out first */
	data[0] = 0x00;
	data[1] = 0x03;
	data[2] = (addr >> 24) & 0xFF;
	data[3] = (addr >> 16) & 0xFF;
	data[4] = (addr >> 8) & 0xFF;
	data[5] = addr & 0xFF;
	data[6] = (len >> 8) & 0xFF;
	data[7] = len & 0xFF;

	for (i = JC_I2C_RETRY; i; i--) {
		err = i2c_transfer(jc_client->adapter, &msg, 1);
		if (err == 1)
			break;
		msleep(20);
	}

	if (err != 1)
		return err;

	msg.flags = I2C_M_RD;
	msg.len = sizeof(recv_data);
	msg.buf = recv_data;
	for (i = JC_I2C_RETRY; i; i--) {
		err = i2c_transfer(jc_client->adapter, &msg, 1);
		if (err == 1)
			break;
		msleep(20);
	}

	if (err != 1)
		return err;

	if (len != (recv_data[1] << 8 | recv_data[2]))

	memcpy(val, recv_data + 3, len);

	return err;
}
#endif

#if JC_LOAD_FW_MAIN || JC_DUMP_FW
static int jc_mem_write(u8 cmd,
		u16 len, u32 addr, u8 *val)
{
	struct i2c_msg msg;
	unsigned char data[len + 8];
	int i, err = 0;

	if (!jc_client->adapter)
		return -ENODEV;

	msg.addr = jc_client->addr >> 1;
	msg.flags = 0;
	msg.len = sizeof(data);
	msg.buf = data;

	/* high byte goes out first */
	data[0] = 0x00;
	data[1] = cmd;
	data[2] = ((addr >> 24) & 0xFF);
	data[3] = ((addr >> 16) & 0xFF);
	data[4] = ((addr >> 8) & 0xFF);
	data[5] = (addr & 0xFF);
	data[6] = ((len >> 8) & 0xFF);
	data[7] = (len & 0xFF);
	memcpy(data + 2 + sizeof(addr) + sizeof(len), val, len);

	for (i = JC_I2C_RETRY; i; i--) {
		err = i2c_transfer(jc_client->adapter, &msg, 1);
		if (err == 1)
			break;
		msleep(20);
	}

	return err;
}
#endif

#if JC_LOAD_FW_MAIN
#ifndef JC_SPI_WRITE
static int jc_program_fw(u8 *buf,
	u32 addr, u32 unit, u32 count)
{
	u32 val;
	int i, err = 0;
	int erase = 0x01;
	int test_count = 0;
	int retries = 0;

	for (i = 0; i < unit*count; i += unit) {
		/* Set Flash ROM memory address */
		err = jc_writel(JC_CATEGORY_FLASH,
			JC_FLASH_ADDR, addr + i);

		/* Erase FLASH ROM entire memory */
		err = jc_writeb(JC_CATEGORY_FLASH,
			JC_FLASH_ERASE, erase);
		/* Response while sector-erase is operating */
		retries = 0;
		do {
			msleep(300);
			err = jc_readb(JC_CATEGORY_FLASH,
				JC_FLASH_ERASE, &val);
		} while (val == erase && retries++ < JC_I2C_VERIFY);

		if (val != 0) {
			return -EFAULT;
		}

		/* Set FLASH ROM programming size */
		err = jc_writew(JC_CATEGORY_FLASH,
				JC_FLASH_BYTE, unit);

		err = jc_mem_write(0x04, unit,
				JC_INT_RAM_BASE_ADDR, buf + i);

	/* Start Programming */
		err = jc_writeb(JC_CATEGORY_FLASH, JC_FLASH_WR, 0x01);

		/* Confirm programming has been completed */
		retries = 0;
		do {
			msleep(30);
			err = jc_readb(JC_CATEGORY_FLASH,
				JC_FLASH_WR, &val);
		} while (val && retries++ < JC_I2C_VERIFY);

		if (val != 0) {
			return -EFAULT;
		}
	}
	return 0;
}
#endif
#endif

#if JC_DUMP_FW
static int jc_dump_fw(void)
{
	struct file *fp;
	mm_segment_t old_fs;
	u8 *buf/*, val*/;
	u32 addr, unit, count, intram_unit = 0x1000;
	int i, /*j,*/ err;


	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fp = filp_open(JC_FW_DUMP_PATH,
		O_WRONLY|O_CREAT|O_TRUNC, S_IRUGO|S_IWUGO|S_IXUSR);
	if (IS_ERR(fp)) {
		err = -ENOENT;
		goto file_out;
	}

	buf = kmalloc(intram_unit, GFP_KERNEL);
	if (!buf) {
		err = -ENOMEM;
		goto out;
	}

	err = jc_mem_write(0x04, SZ_64,
				0x90001200 , buf_port_seting0);
	usleep(10*1000);

	err = jc_mem_write(0x04, SZ_64,
				0x90001000 , buf_port_seting1);
	usleep(10*1000);

	err = jc_mem_write(0x04, SZ_64,
				0x90001100 , buf_port_seting2);
	usleep(10*1000);

	err = jc_writel(JC_CATEGORY_FLASH,
				0x1C, 0x0247036D);

	err = jc_writeb(JC_CATEGORY_FLASH,
				0x57, 0x01);

	addr = JC_FLASH_READ_BASE_ADDR;
	unit = 0x80;
	count = 1024*16;
	for (i = 0; i < count; i++) {
			err = jc_mem_dump(unit, addr + (i * unit), buf);
			if (err < 0) {
				goto out;
			}
			vfs_write(fp, buf, unit, &fp->f_pos);
	}

out:
	if (buf)
		kfree(buf);

	if (!IS_ERR(fp))
		filp_close(fp, current->files);

file_out:
	set_fs(old_fs);

	return err;
}
#endif

#if JC_LOAD_FW_MAIN
static int jc_load_fw_main(void)
{
#ifndef JC_SPI_WRITE
	u8 *buf_m9mo = NULL;
#else
	int txSize = 0;
#endif
	unsigned int count = 0;
	/*int offset;*/
	int err = 0;
	struct file *fp = NULL;
	mm_segment_t old_fs;
	long fsize, nread;
	u8 *buf_m10mo = NULL;
	int i;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	if (firmware_update_sdcard == true) {
		fp = filp_open(JC_M10MO_FW_PATH_SD, O_RDONLY, 0);
		if (IS_ERR(fp)) {
			goto out;
		}
	} else {
		if (jc_ctrl->sensor_combination == 0)
			fp = filp_open(JC_M10MO_FW_PATH_SS, O_RDONLY, 0);
		else if (jc_ctrl->sensor_combination == 1)
			fp = filp_open(JC_M10MO_FW_PATH_OS, O_RDONLY, 0);
		else if (jc_ctrl->sensor_combination == 2)
			fp = filp_open(JC_M10MO_FW_PATH_SL, O_RDONLY, 0);
		else if (jc_ctrl->sensor_combination == 3)
			fp = filp_open(JC_M10MO_FW_PATH_OL, O_RDONLY, 0);
		else
			fp = filp_open(JC_M10MO_FW_PATH_SS, O_RDONLY, 0);

		if (IS_ERR(fp)) {
			goto out;
		}
	}

	fsize = fp->f_path.dentry->d_inode->i_size;
	count = fsize / SZ_4K;

#ifndef JC_SPI_WRITE
	buf_m9mo = vmalloc(fsize);
	if (!buf_m9mo) {
		err = -ENOMEM;
		goto out;
	}

	nread = vfs_read(fp, (char __user *)buf_m9mo, fsize, &fp->f_pos);
	if (nread != fsize) {
		err = -EIO;
		goto out;
	}

	filp_close(fp, current->files);

	err = jc_mem_write(0x04, SZ_64,
			0x90001200 , buf_port_seting0);
	usleep(10*1000);

	err = jc_mem_write(0x04, SZ_64,
			0x90001000 , buf_port_seting1);
	usleep(10*1000);

	err = jc_mem_write(0x04, SZ_64,
			0x90001100 , buf_port_seting2);
	usleep(10*1000);

	err = jc_writel(JC_CATEGORY_FLASH,
			0x1C, 0x0247036D);

	err = jc_writeb(JC_CATEGORY_FLASH,
			0x4A, 0x01);
	usleep(10*1000);

	/* program FLASH ROM */
	err = jc_program_fw(buf_m9mo, JC_FLASH_BASE_ADDR, SZ_4K, count);
	if (err < 0)
		goto out;

	jc_ctrl->isp.bad_fw = 0;

out:
	if (buf_m9mo)
		vfree(buf_m9mo);

	if (!IS_ERR(fp))
		filp_close(fp, current->files);

	set_fs(old_fs);
#else

      txSize = FW_WRITE_SIZE;
      count = fsize / txSize;

	buf_m10mo = vmalloc(txSize);

	if (!buf_m10mo) {
		err = -ENOMEM;
		goto out;
	}

      for (i = 0 ; i < count ; i++) {
		nread = vfs_read(fp, (char __user *)buf_m10mo, txSize, &fp->f_pos);
		err = spi_xmit(buf_m10mo, txSize);
      	}

	if (buf_m10mo)
		vfree(buf_m10mo);

	filp_close(fp, current->files);

out:
	if (!IS_ERR(fp))
		filp_close(fp, current->files);
	set_fs(old_fs);
#endif

	return err;
}
#endif

static int jc_check_sum(void)
{
	int err;
	int retries;
	int isp_val, factarea_val;
	u8 flash_controller[] = {0x7f};

	err = jc_mem_write(0x04, 1,
		    0x13000005 , flash_controller);
	usleep(1000);

	/* Checksum */
	err = jc_writeb(JC_CATEGORY_FLASH, 0x09, 0x04);

	retries = 0;
	do {
		msleep(300);
		err = jc_readb(JC_CATEGORY_FLASH,
			0x09, &isp_val);

		if (isp_val == 0)
			break;
	} while (isp_val == 0x04 && retries++ < JC_I2C_VERIFY);

	/* Get Checksum of ISP */
	err = jc_readw(JC_CATEGORY_FLASH, 0x0A, &isp_val);

	if (jc_ctrl->isp_null_read_sensor_fw == true) {
		err = jc_writel(JC_CATEGORY_FLASH,
				0x0, 0x001E7000);
		err = jc_writel(JC_CATEGORY_FLASH,
				0x5C, 0x00018000);

		/* Checksum */
		err = jc_writeb(JC_CATEGORY_FLASH, 0x09, 0x02);

		retries = 0;
		do {
			msleep(300);
			err = jc_readb(JC_CATEGORY_FLASH,
				0x09, &factarea_val);

			if (factarea_val == 0)
				break;
		} while (factarea_val == 0x02 && retries++ < JC_I2C_VERIFY);

		/* Get Checksum of FactArea */
		err = jc_readw(JC_CATEGORY_FLASH, 0x0A, &factarea_val);

		return isp_val-factarea_val;
	}

	return isp_val;
}

static int jc_phone_fw_to_isp(void)
{
	int err = 0;
	int retries;
	int val;
	int chip_erase;

retry:
	/* Set SIO receive mode : 0x4C, rising edge*/
	err = jc_writeb(JC_CATEGORY_FLASH, 0x4B, 0x4C);

	err = jc_readb(JC_CATEGORY_FLASH,
			0x4B, &val);

	/* SIO mode start */
	err = jc_writeb(JC_CATEGORY_FLASH, 0x4A, 0x02);

	retries = 0;
	do {
		msleep(300);
		err = jc_readb(JC_CATEGORY_FLASH,
			0x4A, &val);
	} while (val == 0x02 && retries++ < JC_I2C_VERIFY);

	usleep(30*1000);

	/* Send firmware by SIO transmission */
	err = jc_load_fw_main();

	chip_erase = jc_check_chip_erase();

	if (chip_erase == ISP_FROM_ERASED) {
	/* Erase Flash ROM */
		err = jc_writeb(JC_CATEGORY_FLASH, 0x06, 0x02);

		retries = 0;
		do {
			msleep(300);
			err = jc_readb(JC_CATEGORY_FLASH,
				0x06, &val);
		} while (val == 0x02 && retries++ < JC_I2C_VERIFY);
	}

	/* Start Programming */
	err = jc_writeb(JC_CATEGORY_FLASH, 0x07, 0x01);
	retries = 0;
	do {
		msleep(300);
		err = jc_readb(JC_CATEGORY_FLASH,
			0x07, &val);
	} while (val == 0x01 && retries++ < JC_I2C_VERIFY);

	err = jc_check_sum();

	if (err != 0 && jc_ctrl->fw_retry_cnt < 2) {
		jc_ctrl->fw_retry_cnt++;
		goto retry;
	}

	return 0;
}

static int jc_read_from_sensor_fw(void)
{
	int err = 0;
	int retries;
	int val = 0;
	int chip_erase;

retry:
	/* Read Sensor Flash */
	err = jc_writeb(JC_CATEGORY_FLASH, 0x63, 0x01);
	retries = 0;
	do {
		msleep(300);
		err = jc_readb(JC_CATEGORY_FLASH,
			0x63, &val);
	} while (val == 0xff && retries++ < JC_I2C_VERIFY);

	chip_erase = jc_check_chip_erase();

	if (chip_erase == ISP_FROM_ERASED) {
		/* Chip Erase */
		err = jc_writeb(JC_CATEGORY_FLASH, 0x06, 0x02);

		retries = 0;
		do {
			msleep(300);
			err = jc_readb(JC_CATEGORY_FLASH,
				0x06, &val);
		} while (val == 0x02 && retries++ < JC_I2C_VERIFY);
	}

	/* Start Programming */
	err = jc_writeb(JC_CATEGORY_FLASH, 0x07, 0x01);
	retries = 0;
	do {
		msleep(300);
		err = jc_readb(JC_CATEGORY_FLASH,
			0x07, &val);
	} while (val == 0x01 && retries++ < JC_I2C_VERIFY);

	err = jc_check_sum();

	if (err != 0 && jc_ctrl->fw_retry_cnt < 2) {
		jc_ctrl->fw_retry_cnt++;
		goto retry;
	}

	return 0;
}

static int jc_load_SIO_fw(void)
{
	struct file *fp;
	mm_segment_t old_fs;
	long fsize, nread;

	u8 *buf_m9mo = NULL;
	int err = 0;
	unsigned int count = 0;

	/* Port Setting */
	err = jc_mem_write(0x08, SZ_64,
			0x90001200 , buf_port_seting0_m10mo);

	usleep(10*1000);

	err = jc_mem_write(0x08, SZ_64,
			0x90001000 , buf_port_seting1_m10mo);
	usleep(10*1000);

	err = jc_mem_write(0x08, SZ_64,
			0x90001100 , buf_port_seting2_m10mo);

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fp = filp_open(JC_SIO_LOADER_PATH_M10MO, O_RDONLY, 0);
	if (IS_ERR(fp)) {
		goto out;
	}

	fsize = fp->f_path.dentry->d_inode->i_size;
	count = fsize / SZ_4K;

	buf_m9mo = vmalloc(fsize);
	if (!buf_m9mo) {
		err = -ENOMEM;
		goto out;
	}

	nread = vfs_read(fp, (char __user *)buf_m9mo, fsize, &fp->f_pos);
	if (nread != fsize) {
		err = -EIO;
		goto out;
	}

	/* Send SIO Loader Program */
	err = jc_mem_write(0x04, SZ_4K,
			0x01000100 , buf_m9mo);

	err = jc_mem_write(0x04, SZ_4K,
			0x01001100 , buf_m9mo+SZ_4K);

	err = jc_mem_write(0x04, SZ_4K,
			0x01002100 , buf_m9mo+SZ_8K);

	err = jc_mem_write(0x04, fsize-(SZ_4K|SZ_8K),
			0x01003100 , buf_m9mo+(SZ_4K|SZ_8K));

	/* Set start address of SIO Loader Program */
	err = jc_writel(JC_CATEGORY_FLASH,
			0x0C, 0x01000100);

	/* Start SIO Loader Program */
	err = jc_writeb(JC_CATEGORY_FLASH, 0x12, 0x02);

out:

	if (buf_m9mo)
		vfree(buf_m9mo);

	usleep(3*1000);

	if (!IS_ERR(fp))
		filp_close(fp, current->files);

	set_fs(old_fs);
	return 0;
}

static int jc_get_sensor_version(void)
{
	int i;
	int err;
	int sensor_ver[11];
	u8 sensor_type[] = "M10MO";

	/* Read Sensor Ver String */

      if (jc_ctrl->is_isp_null == true) {
		for (i = 0 ; i < 11 ; i++) {
			err = jc_readb(JC_CATEGORY_FLASH,
				0x2C + i, &sensor_ver[i]);
		}
      	} else {
		for (i = 0 ; i < 11 ; i++) {
			err = jc_readb(0x00,
				0x32 + i, &sensor_ver[i]);
		}
      	}

	sprintf(sysfs_sensor_fw_str, "%c%c%c%c%c%c%c%c%c%c%c",
		sensor_ver[0], sensor_ver[1], sensor_ver[2], sensor_ver[3],
		sensor_ver[4], sensor_ver[5], sensor_ver[6], sensor_ver[7],
		sensor_ver[8], sensor_ver[9], sensor_ver[10]);

	sprintf(sysfs_sensor_type, "%s", sensor_type);

	return 0;
}

static int jc_check_sensor_phone(void)
{
	if ((strncmp(sysfs_sensor_fw_str, "S13F0", 5) != 0)
	    && (strncmp(sysfs_sensor_fw_str, "O13F0", 5) != 0)) {
		return -1;
	} else if (strncmp(sysfs_sensor_fw_str, sysfs_phone_fw_str, 6) != 0) {
		return 1;
	} else
		return strcmp(sysfs_sensor_fw_str, sysfs_phone_fw_str);
}

static int jc_check_sensor_isp(void)
{
	if ((strncmp(sysfs_isp_fw_str, "S13F0", 5) != 0)
	    && (strncmp(sysfs_isp_fw_str, "O13F0", 5) != 0)) {
		return 1;
	} else if ((strncmp(sysfs_sensor_fw_str, "S13F0", 5) != 0)
	    && (strncmp(sysfs_sensor_fw_str, "O13F0", 5) != 0)) {
		return -1;
	} else if (strncmp(sysfs_sensor_fw_str, sysfs_isp_fw_str, 6) != 0) {
		return 1;
	} else
		return strcmp(sysfs_sensor_fw_str, sysfs_isp_fw_str);
}

static int jc_check_isp_phone(void)
{
	if ((strncmp(sysfs_isp_fw_str, "S13F0", 5) != 0)
	    && (strncmp(sysfs_isp_fw_str, "O13F0", 5) != 0)) {
		return -1;
	} else if (strncmp(sysfs_isp_fw_str, sysfs_phone_fw_str, 6) != 0) {
		return -1;
	} else
		return strcmp(sysfs_isp_fw_str, sysfs_phone_fw_str);
}

static int jc_check_chip_erase(void)
{
      int val, chip_erase;
      int ret = -1;
      int retries;

	jc_writeb(JC_CATEGORY_FLASH,
			0x5B, 0x01);

	/* Checksum */
	jc_writeb(JC_CATEGORY_FLASH, 0x09, 0x02);

	retries = 0;
	do {
		msleep(300);
		jc_readb(JC_CATEGORY_FLASH,
			0x09, &val);

		if (val == 0)
			break;
	} while (val == 0x02 && retries++ < JC_I2C_VERIFY);

	/* Get Checksum of ISP */
	jc_readw(JC_CATEGORY_FLASH, 0x0A, &val);

      jc_readb(JC_CATEGORY_FLASH,
			0x13, &chip_erase);

      if (chip_erase == 0x00) {
		ret = 0;
      	} else if (chip_erase == 0x01) {
		ret = 1;
      	}

      return ret;
}

static int jc_check_sensor_validation(void)
{
	if ((strncmp(sysfs_sensor_fw_str, "S13L0", 5) == 0)
	    || (strncmp(sysfs_sensor_fw_str, "O13L0", 5) == 0)) {
		return 0;
	} else
	      return 1;
}

#if 0
static int jc_check_sensor_spi_port(void)
{
	int check_spi_port = -1;

       jc_readb(0x0D, 0xEF, &check_spi_port);
	cam_info("check_spi_port : %d\n", check_spi_port);

	if (check_spi_port == 1) {
	    return 0;
	} else
	    return 1;
}
#endif

static int jc_get_isp_version(void)
{
	int err;
	u8 isp_ver[22] = {0, };

	/* Read Sensor Ver String */

	err = jc_writeb(JC_CATEGORY_FLASH,
			0x57, 0x01);

      err = jc_mem_read(11, 0x181EF080, isp_ver);

	sprintf(sysfs_isp_fw_str, "%s", isp_ver);

	err = jc_writeb(JC_CATEGORY_FLASH,
			0x57, 0x00);

	if ((strncmp(sysfs_isp_fw_str, "S13F0", 5) != 0)
	    && (strncmp(sysfs_isp_fw_str, "O13F0", 5) != 0))  {
		jc_ctrl->is_isp_null = true;
		return -1;
	} else {
		return 0;
	}
}

static int jc_get_phone_version(void)
{
	int err = 0;

	struct file *fp = NULL;
	mm_segment_t old_fs;
	long nread;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fp = filp_open(JC_M10MO_FW_PATH_SD, O_RDONLY, 0);
	if (IS_ERR(fp)) {
		firmware_update_sdcard = false;

		if (strncmp(sysfs_sensor_fw_str, "S13F0S", 6) == 0) {
			jc_ctrl->sensor_combination = 0;
			fp = filp_open(JC_M10MO_FW_PATH_SS, O_RDONLY, 0);
		} else if (strncmp(sysfs_sensor_fw_str, "O13F0S", 6) == 0) {
			jc_ctrl->sensor_combination = 1;
			fp = filp_open(JC_M10MO_FW_PATH_OS, O_RDONLY, 0);
		} else if (strncmp(sysfs_sensor_fw_str, "S13F0L", 6) == 0) {
			jc_ctrl->sensor_combination = 2;
			fp = filp_open(JC_M10MO_FW_PATH_SL, O_RDONLY, 0);
		} else if (strncmp(sysfs_sensor_fw_str, "O13F0L", 6) == 0) {
			jc_ctrl->sensor_combination = 3;
			fp = filp_open(JC_M10MO_FW_PATH_OL, O_RDONLY, 0);
		} else {
			jc_ctrl->sensor_combination = 0;
			fp = filp_open(JC_M10MO_FW_PATH_SS, O_RDONLY, 0);
		}

		if (IS_ERR(fp))
			goto out;
	} else {
		firmware_update_sdcard = true;
	}

	/* Read Version String addr */
	err = vfs_llseek(fp, JC_FW_VER_STR, SEEK_SET);
	if (err < 0) {
		goto out;
	}

	nread = vfs_read(fp, (char __user *)jc_ctrl->phone_ver_str,
			JC_FW_VER_STR_LEN, &fp->f_pos);

	if (nread != JC_FW_VER_STR_LEN) {
		err = -EIO;
		goto out;
	}

out:
	filp_close(fp, current->files);
	set_fs(old_fs);

	return 0;
}

#if 0
static int jc_fw_check_for_force_update(void)
{
	int err = 0;

	struct file *fp = NULL;
	mm_segment_t old_fs;
	long nread;

	cam_info("E\n");

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fp = filp_open(JC_M10MO_FW_PATH_SD, O_RDONLY, 0);
	if (IS_ERR(fp)) {
		cam_err("failed to open %s, err %ld\n",
			JC_M10MO_FW_PATH_SD, PTR_ERR(fp));
		firmware_update_sdcard = false;
	} else {
		firmware_update_sdcard = true;

		/* Read Version String addr */
		err = vfs_llseek(fp, JC_FW_VER_STR, SEEK_SET);
		if (err < 0) {
			goto out;
		}
		cam_info("ASWOOGI TEST : %d\n", err);
		nread = vfs_read(fp, (char __user *)jc_ctrl->phone_ver_str,
				JC_FW_VER_STR_LEN, &fp->f_pos);

		if (nread != JC_FW_VER_STR_LEN) {
			err = -EIO;
			goto out;
		}
	}

out:
	filp_close(fp, current->files);
	set_fs(old_fs);

	return 0;
}
#endif

static int jc_isp_boot(void)
{
	int err;
	u32 int_factor;
	u8 flash_controller[] = {0x7f};

	err = jc_mem_write(0x04, 1,
		0x13000005 , flash_controller);
	usleep(1*1000);

	/* start camera program(parallel FLASH ROM) */
	err = jc_writeb(JC_CATEGORY_FLASH,
			JC_FLASH_CAM_START, 0x01);

	int_factor = jc_wait_interrupt(JC_ISP_TIMEOUT);
	if (!(int_factor & JC_INT_MODE)) {
		jc_ctrl->isp.bad_fw = 1;
		/* return -ENOSYS; */
	}
	return 0;
}

void jc_isp_reset(struct msm_sensor_ctrl_t *s_ctrl)
{
	struct msm_camera_sensor_info *data = s_ctrl->sensordata;

	/* ISP_RESET */
	data->sensor_platform_info->
		sensor_pmic_gpio_ctrl(data->sensor_platform_info->reset, 0);

	usleep(3*1000);

	data->sensor_platform_info->
		sensor_pmic_gpio_ctrl(data->sensor_platform_info->reset, 1);

	if (wait_event_interruptible_timeout(jc_ctrl->isp.wait,
		jc_ctrl->isp.issued == 1,
		msecs_to_jiffies(5000)) == 0) {
	} else
	    jc_ctrl->isp.issued = 0;
}

#if 0
static u8 log_cnt;
u8 buf[4096];
static bool isp_debug_flag;
void jc_save_isp_debug_log1(void)
{
	struct file *fp = NULL;
	mm_segment_t old_fs;
	char filepath[256];
	int ptr = 0, err;
	int addr = 0, len = 0xff;
	u32 i, intram_unit = 4096;

	/* Just Disable this stupid fucking log.  As if this driver doesn't babble needlessly enough */
	jc_writeb(JC_CATEGORY_TEST, JC_TEST_LOG_ACT, 0x02);
	jc_writeb(JC_CATEGORY_TEST, JC_TEST_ADD_SHOW, 0x00);
}

void jc_save_isp_debug_log2(void)
{
	struct file *fp = NULL;
	mm_segment_t old_fs;
	char filepath[256];
	int ptr = 0, err;
	int addr = 0, len = 0xff;
	u32 i, intram_unit = 4096;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	sprintf(filepath, "/mnt/shell/emulated/0/ISPD/isp_dbg_%d_log2.txt", log_cnt);

	fp = filp_open(filepath,
		O_WRONLY|O_CREAT|O_TRUNC, S_IRUGO|S_IWUGO|S_IXUSR);
	if (IS_ERR(fp)) {
		cam_err("failed to open %s, err %ld\n",
			filepath, PTR_ERR(fp));
		err = -ENOENT;
		goto file_out;
	}

	cam_err("start, file path %s\n", filepath);

	jc_writeb(JC_CATEGORY_TEST, JC_TEST_LOG_MODE, 0x03);

	while (ptr < 10000) {
		jc_writew(JC_CATEGORY_TEST, JC_TEST_LOG_SEL, ptr);
		jc_writeb(JC_CATEGORY_TEST, JC_TEST_LOG_ACT, 0x03);

		while (len == 0xff)
			jc_readw(JC_CATEGORY_TEST, JC_TEST_LOG_STR_LEN, &len);

		if (len == 0x00)
			break;

		jc_readl(JC_CATEGORY_TEST, JC_TEST_LOG_STR_ADD, &addr);

		if (len > 0xFFFF)
			len = 0xFFFF;

		i = 0;
		while(i <= len) {
			/*cam_err("### i %d, len %d, (len - i = %d)", i, len, len - i);*/
			if ((len - i) <= intram_unit) {
				err = jc_mem_read(len - i, addr + i, buf);
				if (err < 0) {
					cam_err("i2c falied, err %d\n", err);
					goto out;
				}
				buf[len] = '\n';
				vfs_write(fp, buf, len - i + 1,  &fp->f_pos);
				break;
			}
			err = jc_mem_read(intram_unit, addr + i, buf);
			if (err < 0) {
				cam_err("i2c falied, err %d\n", err);
				goto out;
			}
			vfs_write(fp, buf, intram_unit,  &fp->f_pos);
			i += intram_unit;
		}
		len = 0xff;
		ptr++;
	}

	/* Enable log */
	jc_writeb(JC_CATEGORY_TEST, JC_TEST_LOG_ACT, 0x02);
out:
	if (!IS_ERR(fp))
		filp_close(fp, current->files);

file_out:
	set_fs(old_fs);
}

void jc_get_isp_debug_log(void)
{
	if (isp_debug_flag == 1) {
		if (log_cnt > 20)
			log_cnt = 0;

		jc_save_isp_debug_log1();
		jc_save_isp_debug_log2();

		log_cnt++;
	}
}
#endif

static int jc_verify_writedata(unsigned char category,
					unsigned char byte, u32 value)
{
	u32 val = 0;
	unsigned char i;

	for (i = 0; i < JC_I2C_VERIFY; i++) {
		jc_readb(category, byte, &val);
		if (val == value) {
			return 0;
		}
		usleep(20000);/*20ms*/
	}
	return -EBUSY;
}

static int jc_set_mode(u32 mode)
{
	int i, err;
	u32 old_mode, val;
	u32 int_factor;

	err = jc_readb(JC_CATEGORY_SYS, JC_SYS_MODE, &old_mode);
	if (err < 0)
		return err;

	if (old_mode == mode) {
		return old_mode;
	}

	switch (mode) {
	case JC_SYSINIT_MODE:
		err = -EBUSY;
		break;

	case JC_PARMSET_MODE:
		jc_writeb(JC_CATEGORY_LENS,
				0x02, 0x00);
		msleep(30);
		err = jc_writeb(JC_CATEGORY_SYS, JC_SYS_MODE, mode);
		int_factor = jc_wait_interrupt(JC_ISP_TIMEOUT);
		if (!(int_factor & JC_INT_MODE)) {
			return -ENOSYS;
		}
		jc_ctrl->need_restart_caf = true;
		jc_ctrl->stream_on = false;
		break;

	case JC_MONITOR_MODE:
		err = jc_writeb(JC_CATEGORY_SYS, JC_SYS_MODE, mode);

		int_factor = jc_wait_interrupt(JC_ISP_TIMEOUT);
		if (!(int_factor & JC_INT_MODE)) {
			return -ENOSYS;
		}
		jc_ctrl->stream_on = true;
		if (jc_ctrl->need_restart_caf == true) {
			jc_ctrl->need_restart_caf = false;
			if (jc_ctrl->shot_mode == 4 ||jc_ctrl->shot_mode == 15) { //drama, cine photo
				if (jc_ctrl->af_mode == 3) {
					jc_writeb(JC_CATEGORY_LENS,
							0x01, 0x03);
					jc_writeb(JC_CATEGORY_LENS,
							0x02, 0x01);
				} else if (jc_ctrl->af_mode == 4) {
					jc_writeb(JC_CATEGORY_LENS,
							0x01, 0x07);
					jc_writeb(JC_CATEGORY_LENS,
							0x02, 0x01);
				} else if (jc_ctrl->af_mode == 5) {
					msleep(50);
					jc_writeb(JC_CATEGORY_LENS,
							0x01, 0x04);
					jc_writeb(JC_CATEGORY_LENS,
							0x02, 0x01);
				} else if (jc_ctrl->af_mode == 6) {
					msleep(50);
					jc_writeb(JC_CATEGORY_LENS,
							0x01, 0x05);
					jc_writeb(JC_CATEGORY_LENS,
							0x02, 0x01);
				}
			}
		}
		break;

	default:
		err = 0;/* -EINVAL; */
	}

	if (err < 0)
		return err;

	for (i = JC_I2C_VERIFY; i; i--) {
		err = jc_readb(JC_CATEGORY_SYS, JC_SYS_MODE, &val);
		if (val == mode)
			break;
		usleep(20000);/*20ms*/
	}
	return mode;
}

static irqreturn_t jc_isp_isr(int irq, void *dev_id)
{
	jc_ctrl->isp.issued = 1;
	wake_up_interruptible(&jc_ctrl->isp.wait);

	return IRQ_HANDLED;
}

static u32 jc_wait_interrupt(unsigned int timeout)
{
	int i = 0;

	if (wait_event_interruptible_timeout(jc_ctrl->isp.wait,
		jc_ctrl->isp.issued == 1,
		msecs_to_jiffies(timeout)) == 0) {
		return 0;
	}

	jc_ctrl->isp.issued = 0;

	jc_readw(JC_CATEGORY_SYS,
		JC_SYS_INT_FACTOR, &jc_ctrl->isp.int_factor);
	for (i = 0 ; i < 10 ; i++) {
		if (jc_ctrl->isp.int_factor != 0x100) {
			msleep(30);
			jc_readw(JC_CATEGORY_SYS,
				JC_SYS_INT_FACTOR, &jc_ctrl->isp.int_factor);
		} else {
			break;
		}
	}
	return jc_ctrl->isp.int_factor;
}

void jc_set_preview(void)
{
}

void jc_set_capture(void)
{
}

static int jc_set_capture_size(int width, int height)
{
	u32 isp_mode;
	int err;

	if (width == 4128 && height == 3096)
		jc_ctrl->capture_size = 0x2C;
	else if (width == 4128 && height == 2322)
		jc_ctrl->capture_size = 0x2B;
	else if (width == 4096 && height == 3072)
		jc_ctrl->capture_size = 0x38;
	else if (width == 4096 && height == 2304)
		jc_ctrl->capture_size = 0x37;
	else if (width == 3264 && height == 2448)
		jc_ctrl->capture_size = 0x25;
	else if (width == 3200 && height == 1920)
		jc_ctrl->capture_size = 0x24;
	else if (width == 3264 && height == 1836)
		jc_ctrl->capture_size = 0x21;
	else if (width == 2560 && height == 1920)
		jc_ctrl->capture_size = 0x1F;
	else if (width == 2048 && height == 1536)
		jc_ctrl->capture_size = 0x1B;
	else if (width == 2048 && height == 1152)
		jc_ctrl->capture_size = 0x1A;
	else if (width == 1280 && height == 720)
		jc_ctrl->capture_size = 0x10;
	else if (width == 640 && height == 480)
		jc_ctrl->capture_size = 0X09;
	else
		jc_ctrl->capture_size = 0x2C;

	err = jc_readb(JC_CATEGORY_SYS, JC_SYS_MODE, &isp_mode);

	if (err < 0) {
		return err;
	}

	if( isp_mode == JC_MONITOR_MODE ) {
		jc_set_sizes();

		err = jc_readb(JC_CATEGORY_SYS, JC_SYS_MODE, &isp_mode);

		if (err < 0) {
			return err;
		}

		if( isp_mode == JC_PARMSET_MODE ) {
			jc_set_mode(JC_MONITOR_MODE);
		}
	}
	return 0;
}

static int jc_set_preview_size(int32_t width, int32_t height)
{
	if (width == 1728 && height == 1296)
		jc_ctrl->preview_size = 0x40;
	else if (width == 384 && height == 288)
		jc_ctrl->preview_size = 0x3F;
	else if (width == 768 && height == 576)
		jc_ctrl->preview_size = 0x3E;
	else if (width == 864 && height == 576)
		jc_ctrl->preview_size = 0x3D;
	else if (width == 1536 && height == 864)
		jc_ctrl->preview_size = 0x3C;
	else if (width == 2304 && height == 1296)
		jc_ctrl->preview_size = 0x3B;
	else if (width == 1440 && height == 1080)
		jc_ctrl->preview_size = 0x37;
	else if (width == 960 && height == 720)
		jc_ctrl->preview_size = 0x34;
	else if (width == 1920 && height == 1080)
		jc_ctrl->preview_size = 0x28;
	else if (width == 1152 && height == 864)
		jc_ctrl->preview_size = 0x23;
	else if (width == 1280 && height == 720)
		jc_ctrl->preview_size = 0x21;
	else if (width == 720 && height == 480)
		jc_ctrl->preview_size = 0x18;
	else if (width == 640 && height == 480)
		jc_ctrl->preview_size = 0x17;
	else if (width == 320 && height == 240)
		jc_ctrl->preview_size = 0x09;
	else if (width == 176 && height == 144)
		jc_ctrl->preview_size = 0x05;
	else if (width == 4128 && height == 3096)
		jc_ctrl->preview_size = 0x27;
	else if (width == 800 && height == 480)
		jc_ctrl->preview_size = 0x41;
	else if (width == 800 && height == 450)
		jc_ctrl->preview_size = 0x44;
	else if (width == 3264 && height == 1836)
		jc_ctrl->preview_size = 0x43;
	else if (width == 1056 && height == 864)
		jc_ctrl->preview_size = 0x47;
	else
		jc_ctrl->preview_size = 0x17;

	return 0;
}

static int jc_set_sizes(void)
{
	int prev_preview_size, prev_capture_size;

	jc_readb(JC_CATEGORY_PARM,
			JC_PARM_MON_SIZE, &prev_preview_size);
	jc_readb(JC_CATEGORY_CAPPARM,
			JC_CAPPARM_MAIN_IMG_SIZE, &prev_capture_size);

	/* set monitor(preview) size */
	if (jc_ctrl->preview_size != prev_preview_size) {

		/* change to parameter mode */
		jc_set_mode(JC_PARMSET_MODE);

		jc_writeb(JC_CATEGORY_PARM,
				JC_PARM_MON_SIZE,
				jc_ctrl->preview_size);

		if (jc_ctrl->preview_size == 0x43 ||
			jc_ctrl->preview_size == 0x44 || jc_ctrl->preview_size == 0x45) {
			usleep(10*1000);
			jc_writeb(JC_CATEGORY_CAPCTRL, 0x0, 0x0);
			usleep(10*1000);
			jc_writeb(0x1, 0x5, 0x1);
			usleep(10*1000);
			jc_writeb(0x1, 0x6, 0x1);
		} else
			jc_writeb(JC_CATEGORY_CAPCTRL, 0x0, 0x0f);
		jc_verify_writedata(JC_CATEGORY_PARM,
				JC_PARM_MON_SIZE,
				jc_ctrl->preview_size);
	} else

	/* set capture size */
	if (jc_ctrl->capture_size != prev_capture_size) {
		jc_writeb(JC_CATEGORY_CAPPARM,
				JC_CAPPARM_MAIN_IMG_SIZE,
				jc_ctrl->capture_size);
		jc_verify_writedata(JC_CATEGORY_CAPPARM,
				JC_CAPPARM_MAIN_IMG_SIZE,
				jc_ctrl->capture_size);
	} else
		pr_debug ("if you can see me, you are needlessly debugging");

	return 0;
}

static int jc_set_snapshot_mode(int mode)
{
	int32_t rc = 0;
	int val = -1;
	int retries = 0;

	if (jc_ctrl->burst_mode == true
		&& mode == 1) {
		return rc;
	}

	if (mode == 0) {
		do {
			jc_readb(JC_CATEGORY_CAPCTRL,
				        0x1F, &val);
		} while (val != 0 && retries++ < JC_I2C_VERIFY);
		if ((jc_ctrl->shot_mode == 0 ||jc_ctrl->shot_mode == 1)
			&& (jc_ctrl->movie_mode == false)){
			jc_writeb(JC_CATEGORY_CAPCTRL,
					JC_CAPCTRL_START_DUALCAP, 0x07);
		} else {
			jc_writeb(JC_CATEGORY_CAPCTRL,
					JC_CAPCTRL_START_DUALCAP, 0x01);
		}
	} else if (mode == 1) {
		jc_ctrl->burst_mode = true;
		jc_writeb(JC_CATEGORY_CAPCTRL,
				JC_CAPCTRL_START_DUALCAP, 0x04);
	} else if (mode == 2) {
		jc_ctrl->burst_mode = false;
		jc_writeb(JC_CATEGORY_CAPCTRL,
				JC_CAPCTRL_START_DUALCAP, 0x05);
	}
	return rc;
}

static int jc_set_zoom(int level)
{
	int32_t rc = 0;

	if (level < 1 || level > 31) {
		return rc;
	}
	jc_writeb(JC_CATEGORY_MON,
			JC_MON_ZOOM, level);
	return rc;
}

static int jc_set_autofocus(int status)
{
	int32_t rc = 0;

	if (status < 0 || status > 1) {
		return rc;
	}
	jc_writeb(JC_CATEGORY_LENS,
			0x02, status);
	return rc;
}

static int jc_set_af_mode(int status)
{
	int32_t rc = 0;

	if (status < 1 || status > 6) {
		return rc;
	}
	jc_ctrl->af_mode = status;

	if (jc_ctrl->touch_af_mode == true
		&& (status == 1 || status == 2)) {
		jc_ctrl->touch_af_mode = false;
		return rc;
	}
	jc_ctrl->touch_af_mode = false;

	if (status == 1) {
		jc_writeb(JC_CATEGORY_LENS,
				0x01, 0x01);
	} else if (status == 2) {
		jc_writeb(JC_CATEGORY_LENS,
				0x01, 0x00);
	} else if (status == 3) {
		jc_writeb(JC_CATEGORY_LENS,
				0x01, 0x03);
		if (jc_ctrl->stream_on == true) {
			jc_writeb(JC_CATEGORY_LENS,
					0x02, 0x01);
		}
	} else if (status == 4) {
		jc_writeb(JC_CATEGORY_LENS,
				0x01, 0x07);
		if (jc_ctrl->stream_on == true) {
			jc_writeb(JC_CATEGORY_LENS,
					0x02, 0x01);
		}
	} else if (status == 5) {
		jc_writeb(JC_CATEGORY_LENS,
				0x01, 0x04);
		msleep(50);
		if (jc_ctrl->stream_on == true) {
			jc_writeb(JC_CATEGORY_LENS,
					0x02, 0x01);
		}
	} else if (status == 6) {
		jc_writeb(JC_CATEGORY_LENS,
				0x01, 0x05);
		if (jc_ctrl->stream_on == true) {
			jc_writeb(JC_CATEGORY_LENS,
					0x02, 0x01);
		}
	}

	return rc;
}

static int jc_set_touch_af_pos(int x, int y)
{
	int32_t rc = 0;
	int h_x, l_x, h_y, l_y;

	if ((jc_ctrl->af_mode >= 3
		&& jc_ctrl->af_mode <=6)
		&& jc_ctrl->touch_af_mode == true) {
		return rc;
	}

	jc_ctrl->touch_af_mode = true;

	h_x = x >> 8;
	l_x = x & 0xFF;
	h_y = y >> 8;
	l_y = y & 0xFF;

	if (jc_ctrl->movie_mode == true) {
		jc_writeb(JC_CATEGORY_LENS,
				0x01, 0x07);
	} else {
		jc_writeb(JC_CATEGORY_LENS,
				0x01, 0x02);
	}

	jc_writeb(JC_CATEGORY_LENS,
			0x30, h_x);
	jc_writeb(JC_CATEGORY_LENS,
			0x31, l_x);
	jc_writeb(JC_CATEGORY_LENS,
			0x32, h_y);
	jc_writeb(JC_CATEGORY_LENS,
			0x33, l_y);

	return rc;
}

static int jc_set_metering(int mode)
{
	int32_t rc = 0;

	if (mode < 0 || mode > 2) {
		return rc;
	}
	if (mode == 0) {
		jc_writeb(JC_CATEGORY_AE,
				JC_AE_MODE, 0x02);
	} else if (mode == 1) {
		jc_writeb(JC_CATEGORY_AE,
				JC_AE_MODE, 0x00);
	} else if (mode == 2) {
		jc_writeb(JC_CATEGORY_AE,
				JC_AE_MODE, 0x01);
	}
	return rc;
}

static int jc_set_wb(int mode)
{
	int32_t rc = 0;

	if (mode < 1 || mode > 6) {
		return rc;
	}
	if (mode == 1) {
		jc_writeb(JC_CATEGORY_WB,
				JC_WB_AWB_MODE, 0x01);
	} else {
		jc_writeb(JC_CATEGORY_WB,
				JC_WB_AWB_MODE, 0x02);

		if (mode == 3) {
			jc_writeb(JC_CATEGORY_WB,
					JC_WB_AWB_MANUAL, 0x01);
		} else if (mode == 4) {
			jc_writeb(JC_CATEGORY_WB,
					JC_WB_AWB_MANUAL, 0x02);
		} else if (mode == 5) {
			jc_writeb(JC_CATEGORY_WB,
					JC_WB_AWB_MANUAL, 0x04);
		} else if (mode == 6) {
			jc_writeb(JC_CATEGORY_WB,
					JC_WB_AWB_MANUAL, 0x05);
		}
	}
	return rc;
}

static int jc_set_effect(int mode)
{
	int32_t rc = 0;

	if (mode == 0) {
		jc_writeb(JC_CATEGORY_MON,
				JC_MON_COLOR_EFFECT, 0x00);
	} else if (mode == 1) {
		jc_writeb(JC_CATEGORY_MON,
				JC_MON_COLOR_EFFECT, 0x01);
		jc_writeb(JC_CATEGORY_MON,
				JC_MON_CFIXB, 0x00);
		jc_writeb(JC_CATEGORY_MON,
				JC_MON_CFIXR, 0x00);
	} else if (mode == 2) {
		jc_writeb(JC_CATEGORY_MON,
				JC_MON_COLOR_EFFECT, 0x02);
	} else if (mode == 4) {
		jc_writeb(JC_CATEGORY_MON,
				JC_MON_COLOR_EFFECT, 0x01);
		jc_writeb(JC_CATEGORY_MON,
				JC_MON_CFIXB, 0xD8);
		jc_writeb(JC_CATEGORY_MON,
				JC_MON_CFIXR, 0x18);
	} else if (mode == 12) {
		jc_writeb(JC_CATEGORY_MON,
				JC_MON_COLOR_EFFECT, 0x04);
	} else if (mode == 13) {
		jc_writeb(JC_CATEGORY_MON,
				JC_MON_COLOR_EFFECT, 0x05);
	} else if (mode == 14) {
		jc_writeb(JC_CATEGORY_MON,
				JC_MON_COLOR_EFFECT, 0x06);
	}
	return rc;
}

static int jc_set_quality(int mode)
{
	int32_t rc = 0;

	if (mode < 0 || mode > 2) {
		return rc;
	}

	if (mode == 0) {
		jc_writeb(JC_CATEGORY_CAPPARM,
				JC_CAPPARM_JPEG_RATIO_OFS, 0x02);
	} else if (mode == 1) {
		jc_writeb(JC_CATEGORY_CAPPARM,
				JC_CAPPARM_JPEG_RATIO_OFS, 0x09);
	} else if (mode == 2) {
		jc_writeb(JC_CATEGORY_CAPPARM,
				JC_CAPPARM_JPEG_RATIO_OFS, 0x14);
	}
	return rc;
}

static int jc_set_iso(int mode)
{
	int32_t rc = 0;

	if (mode < 0 || mode > 6) {
		return rc;
	}

	if (mode == 0) {
		jc_writeb(JC_CATEGORY_AE,
			JC_AE_ISOSEL, 0x00);
	} else if (mode == 2) {
		jc_writeb(JC_CATEGORY_AE,
			JC_AE_ISOSEL, 0x01);
	} else if (mode == 3) {
		jc_writeb(JC_CATEGORY_AE,
			JC_AE_ISOSEL, 0x02);
	} else if (mode == 4) {
		jc_writeb(JC_CATEGORY_AE,
			JC_AE_ISOSEL, 0x03);
	} else if (mode == 5) {
		jc_writeb(JC_CATEGORY_AE,
			JC_AE_ISOSEL, 0x04);
	} else if (mode == 6) {
		jc_writeb(JC_CATEGORY_AE,
			JC_AE_ISOSEL, 0x05);
	}
	return rc;
}

static int jc_set_ev(int mode)
{
	int32_t rc = 0;

	if (mode < 0 || mode > 8) {
		return rc;
	}

	if (mode == 0) {
		jc_writeb(JC_CATEGORY_AE,
				JC_AE_INDEX, 0x00);
	} else if (mode == 1) {
		jc_writeb(JC_CATEGORY_AE,
				JC_AE_INDEX, 0x01);
	} else if (mode == 2) {
		jc_writeb(JC_CATEGORY_AE,
				JC_AE_INDEX, 0x02);
	} else if (mode == 3) {
		jc_writeb(JC_CATEGORY_AE,
				JC_AE_INDEX, 0x03);
	} else if (mode == 4) {
		jc_writeb(JC_CATEGORY_AE,
				JC_AE_INDEX, 0x04);
	} else if (mode == 5) {
		jc_writeb(JC_CATEGORY_AE,
				JC_AE_INDEX, 0x05);
	} else if (mode == 6) {
		jc_writeb(JC_CATEGORY_AE,
				JC_AE_INDEX, 0x06);
	} else if (mode == 7) {
		jc_writeb(JC_CATEGORY_AE,
				JC_AE_INDEX, 0x07);
	} else if (mode == 8) {
		jc_writeb(JC_CATEGORY_AE,
				JC_AE_INDEX, 0x08);
	}
	return rc;
}

static int jc_set_hjr(int mode)
{
	int32_t rc = 0;

	if (mode < 0 || mode > 1) {
		return rc;
	}

	if (mode == 0) {
		jc_writeb(JC_CATEGORY_CAPCTRL,
				0x0B, 0x00);
	} else if (mode == 1) {
		jc_writeb(JC_CATEGORY_CAPCTRL,
				0x0B, 0x01);
	}

	return rc;
}

static int jc_set_wdr(int mode)
{
	int32_t rc = 0;

	if (mode < 0 || mode > 1) {
		return rc;
	}

	if (mode == 0) {
		jc_writeb(JC_CATEGORY_CAPPARM,
				JC_CAPPARM_WDR_EN, 0x00);
	} else if (mode == 1) {
		jc_writeb(JC_CATEGORY_CAPPARM,
				JC_CAPPARM_WDR_EN, 0x01);
	}
	return rc;
}

static int jc_set_movie_mode(int mode)
{
	int32_t rc = 0;

	jc_set_mode(JC_PARMSET_MODE);

	if (mode == 0) {
		jc_ctrl->movie_mode = false;
		jc_writeb(JC_CATEGORY_PARM,
				JC_PARM_MON_MOVIE_SELECT, 0x00);

		jc_writeb(0x02, 0xCF, 0x01); /*zsl mode*/
	} else if (mode == 1) {
		jc_ctrl->movie_mode = true;
		jc_writeb(JC_CATEGORY_PARM,
				JC_PARM_MON_MOVIE_SELECT, 0x01);
	}
	return rc;
}

static int jc_set_antibanding(int mode)
{
	int32_t rc = 0;

	if (mode == 0) {
		jc_writeb(JC_CATEGORY_AE,
				0x06, 0x05);
	} if (mode == 1) {
		jc_writeb(JC_CATEGORY_AE,
				0x06, 0x02);
	} if (mode == 2) {
		jc_writeb(JC_CATEGORY_AE,
				0x06, 0x01);
	} if (mode == 3) {
		jc_writeb(JC_CATEGORY_AE,
				0x06, 0x00);
	}

	return rc;
}

static int jc_set_shot_mode(int mode)
{
	int32_t rc = 0;
	u32 isp_mode;

	jc_readb(JC_CATEGORY_SYS, JC_SYS_MODE, &isp_mode);

	jc_ctrl->shot_mode = mode;

	if (isp_mode == JC_MONITOR_MODE) {
		jc_set_mode(JC_PARMSET_MODE);

		if (mode == 10) { //night mode
			jc_writeb(JC_CATEGORY_CAPCTRL,
					0x0B, 0x00);
		}

		jc_writeb(JC_CATEGORY_PARM,
				0x0E, mode);

		jc_set_mode(JC_MONITOR_MODE);
	} else {

		if (mode == 10) { //night mode
			jc_writeb(JC_CATEGORY_CAPCTRL,
					0x0B, 0x00);
		}

		jc_writeb(JC_CATEGORY_PARM,
				0x0E, mode);
	}

	return rc;
}

static int jc_set_ocr_focus_mode(int mode)
{
	int32_t rc = 0;

	jc_writeb(JC_CATEGORY_LENS, 0x18, mode);

	return rc;
}

static int jc_set_different_ratio_capture(int mode)
{
	int32_t rc = 0;

	if (mode == 1)
		jc_writeb(JC_CATEGORY_CAPPARM, 0x77, 0x1);
	else
		jc_writeb(JC_CATEGORY_CAPPARM, 0x77, 0x0);

	return rc;
}

static int jc_set_af_window(int mode)
{
	int32_t rc = 0;

	jc_writeb(JC_CATEGORY_TEST, 0x8D, mode);

	return rc;
}

static int jc_set_softlanding(void)
{
	int32_t rc = 0;
	int check_softlanding = 0;
	int retry = 0;

	jc_writeb(JC_CATEGORY_LENS, 0x16, 0x01);

	for (retry = 2; retry > 0; retry--) {
		jc_readb(JC_CATEGORY_LENS,
					0x17, &check_softlanding);
		if (check_softlanding == 0x30) {
			break;
		} else {
			msleep(33);
		}
	}

	return rc;
}

static int jc_set_fps(int mode, int min, int max)
{
	int32_t rc = 0;

	if (mode == 0) {
		jc_writeb(0x02, 0xCF, 0x01);	/*zsl mode*/

		if (min == 10000 && max == 30000) { /*LLS*/
			jc_writeb(JC_CATEGORY_AE,
					JC_AE_EP_MODE_CAP, 0x15);
		} else if (max == 24000) {
			jc_writeb(JC_CATEGORY_AE,
					JC_AE_EP_MODE_CAP, 0x0C);
		} else {
			jc_writeb(JC_CATEGORY_AE,
					JC_AE_EP_MODE_CAP, 0x00);
		}
	} else if (mode == 1) {
		jc_writeb(0x02, 0xCF, 0x00);	/*non-zsl mode*/

		if (max == 7000) {
			jc_writeb(JC_CATEGORY_AE,
					JC_AE_EP_MODE_CAP, 0x09);
		} else if (max == 15000) {
			if (jc_ctrl->shot_mode == 0x0F) {/*CinePic*/
				jc_writeb(JC_CATEGORY_AE,
						JC_AE_EP_MODE_CAP, 0x17);
			} else {
				jc_writeb(JC_CATEGORY_AE,
						JC_AE_EP_MODE_CAP, 0x04);
			}
		} else if (max == 24000) {
			jc_writeb(JC_CATEGORY_AE,
					JC_AE_EP_MODE_CAP, 0x05);
		} else if (max == 30000) {
			jc_writeb(JC_CATEGORY_AE,
					JC_AE_EP_MODE_CAP, 0x02);
		} else if (max == 60000) {
			jc_writeb(JC_CATEGORY_AE,
					JC_AE_EP_MODE_CAP, 0x07);
		} else if (max == 90000) {
			jc_writeb(JC_CATEGORY_AE,
					JC_AE_EP_MODE_CAP, 0x0B);
		} else if (max == 120) { /*120fps*/
			jc_writeb(JC_CATEGORY_AE,
					JC_AE_EP_MODE_CAP, 0x08);
		}
	} else if (mode == 2) {
		jc_writeb(JC_CATEGORY_AE,
				JC_AE_EP_MODE_CAP, 0x16);
	}
	return rc;
}

static int jc_set_flash(int mode, int mode2)
{
	int32_t rc = 0;

	if (jc_ctrl->torch_mode == true
		&& mode == 1
		&& mode2 == 0) {
		/*torch off on camera */
		mode = 0;
	}

	if (mode == 0) {
		if (mode2 == 0) {
			jc_ctrl->torch_mode = false;
			jc_writeb(JC_CATEGORY_TEST,
					0x29, 0x00);
		} else if (mode2 == 1) {
			jc_ctrl->torch_mode = true;
			jc_writeb(JC_CATEGORY_TEST,
					0x29, 0x01);
		}
	} else if (mode == 1) {
		jc_ctrl->torch_mode = false;
		if (mode2 == 0) {
			jc_writeb(JC_CATEGORY_TEST,
					0xB6, 0x00);
		} else if (mode2 == 1) {
			jc_writeb(JC_CATEGORY_TEST,
					0xB6, 0x02);
		} else if (mode2 == 2) {
			jc_writeb(JC_CATEGORY_TEST,
					0xB6, 0x01);
		}
	}
	return rc;
}

static int jc_set_hw_vdis(void)
{
	int32_t rc = 0, prev_vdis_mode;

	jc_readb(JC_CATEGORY_MON,
			0x00, &prev_vdis_mode);

	if (jc_ctrl->hw_vdis_mode != prev_vdis_mode) {
		if (jc_ctrl->hw_vdis_mode == 1)
			jc_writeb(JC_CATEGORY_MON,
					0x00, 0x01);
		else
			jc_writeb(JC_CATEGORY_MON,
					0x00, 0x00);
  	} else
		pr_debug("too distracted by the finger in my ass");
	return rc;
}

void sensor_native_control(void __user *arg)
{
	struct ioctl_native_cmd ctrl_info;
	struct msm_camera_v4l2_ioctl_t *ioctl_ptr = arg;

	if (copy_from_user(&ctrl_info,
		(void __user *)ioctl_ptr->ioctl_ptr,
		sizeof(ctrl_info))) {
		goto FAIL_END;
	}

	switch (ctrl_info.mode) {

	case EXT_CAM_SNAPSHOT_MODE:
		jc_set_snapshot_mode(ctrl_info.value_1);
		break;

	case EXT_CAM_AF:
		jc_set_autofocus(ctrl_info.value_1);
		break;

	case EXT_CAM_FOCUS_MODE:
		jc_set_af_mode(ctrl_info.value_1);
		break;

	case EXT_CAM_SET_TOUCHAF_POS:
		jc_set_touch_af_pos(ctrl_info.value_1, ctrl_info.value_2);
		break;

	case EXT_CAM_PREVIEW_SIZE:
		break;

	case EXT_CAM_CAPTURE_SIZE:
		break;

	case EXT_CAM_METERING:
		jc_set_metering(ctrl_info.value_1);
		break;

	case EXT_CAM_WB:
		jc_set_wb(ctrl_info.value_1);
		break;

	case EXT_CAM_EFFECT:
		jc_set_effect(ctrl_info.value_1);
		break;

	case EXT_CAM_QUALITY:
		jc_set_quality(ctrl_info.value_1);
		break;

	case EXT_CAM_SET_ZOOM:
		jc_set_zoom(ctrl_info.value_1);
		break;

	case EXT_CAM_ISO:
		jc_set_iso(ctrl_info.value_1);
		break;

	case EXT_CAM_EV:
		jc_set_ev(ctrl_info.value_1);
		break;

	case EXT_CAM_HJ_REDUCTION:
		jc_set_hjr(ctrl_info.value_1);
		break;

	case EXT_CAM_SET_WDR:
		jc_set_wdr(ctrl_info.value_1);
		break;

	case EXT_CAM_LED:
		jc_set_flash(ctrl_info.value_1, ctrl_info.value_2);
		break;

	case EXT_CAM_UPDATE_FW:
		if (ctrl_info.value_1 == CAM_FW_MODE_DUMP) {
			jc_sensor_power_reset(&jc_s_ctrl);
#if JC_DUMP_FW
			jc_dump_fw();
#endif
			jc_sensor_power_down(&jc_s_ctrl);
		} else if (ctrl_info.value_1 == CAM_FW_MODE_UPDATE) {
			jc_sensor_power_reset(&jc_s_ctrl);
			jc_get_phone_version();
			jc_load_SIO_fw();
			jc_phone_fw_to_isp();
			jc_sensor_power_reset(&jc_s_ctrl);
			jc_get_isp_version();
			jc_isp_boot();

			jc_writeb(JC_CATEGORY_CAPCTRL,
					0x0, 0x0f);
		}
		break;

	case EXT_CAM_SET_PREVIEW_SIZE:
		jc_set_preview_size(ctrl_info.value_1, ctrl_info.value_2);
		break;

	case EXT_CAM_SET_CAPTURE_SIZE:
		jc_set_capture_size(ctrl_info.value_1,
					    ctrl_info.value_2);
		break;

	case EXT_CAM_SET_HDR:
		jc_writeb(0x0C, 0x0A, 0x02);
		break;

	case EXT_CAM_SET_LLS:
		jc_writeb(0x0C, 0x0A, 0x04);
		break;

	case EXT_CAM_SET_RAW:
		jc_writeb(0x0C, 0x0A, 0x10);
		break;

	case EXT_CAM_SET_BEST:
		/* 0x20(YUV420) or 0x21(YUV422) */

	      jc_writeb(0x0C, 0x0A, 0x20);
	      /* jc_writeb(0x0C, 0x0A, 0x21); */
		break;

	case EXT_CAM_START_HDR:
		if (ctrl_info.value_1 == 0)
			jc_writeb(0x0C, 0x05, 0x08); /*LLS*/
		else
			jc_writeb(0x0C, 0x05, 0x01); /*HDR*/
		break;

	case EXT_CAM_RESET_HDR:
		jc_writeb(0x0C, 0x0A, 0x00);
		break;

	case EXT_CAM_RESUME_PREVIEW:
		jc_writeb(0x0C, 0x05, 0x02);
		break;

	case EXT_CAM_SET_FPS:
		jc_set_fps(ctrl_info.address, ctrl_info.value_1, ctrl_info.value_2);
		break;

	case EXT_CAM_MOVIE_MODE:
		jc_set_movie_mode(ctrl_info.value_1);
		break;

	case EXT_CAM_SET_ANTIBANDING:
		jc_set_antibanding(ctrl_info.value_1);
		break;

#if 0
	case EXT_CAM_GET_ISP_DBG_LOG:
		jc_get_isp_debug_log();
		break;
#endif

	case EXT_CAM_FD_MODE:
		if (ctrl_info.value_1 == 0) {
			jc_writeb(JC_CATEGORY_FD,
					0x4A, 0x00);
		} else if (ctrl_info.value_1 == 1) {
			jc_writeb(JC_CATEGORY_FD,
					0x4A, 0x01);
		}
		break;

	case EXT_CAM_START_AE_AWB_LOCK:
		if (ctrl_info.value_1 == 0) {
			jc_writeb(JC_CATEGORY_AE,
					0x00, 0x00);
		} else if (ctrl_info.value_1 == 1) {
			jc_writeb(JC_CATEGORY_AE,
					0x00, 0x01);
		}

		if (ctrl_info.value_2 == 0) {
			jc_writeb(JC_CATEGORY_WB,
					0x00, 0x00);
		} else if (ctrl_info.value_2 == 1) {
			jc_writeb(JC_CATEGORY_WB,
					0x00, 0x01);
		}
		break;

	case EXT_CAM_SET_HW_VDIS:
		jc_ctrl->hw_vdis_mode = ctrl_info.value_1;
		break;

	case EXT_CAM_SET_SHOT_MODE:
		jc_set_shot_mode(ctrl_info.value_1);
		break;

	case EXT_CAM_SET_ANTI_STREAMOFF:
		if (ctrl_info.value_1 == 1)
			jc_ctrl->anti_stream_off = true;
		else
			jc_ctrl->anti_stream_off = false;
		break;

	case EXT_CAM_SET_OCR_FOCUS_MODE:
		jc_set_ocr_focus_mode(ctrl_info.value_1);
		break;

	case EXT_CAM_SET_AF_WINDOW:
		jc_set_af_window(ctrl_info.value_1);
		break;

	case EXT_CAM_SET_FACTORY_BIN:
		if (ctrl_info.value_1 == 1)
			jc_ctrl->factory_bin = true;
		else
			jc_ctrl->factory_bin = false;
		break;

	case EXT_CAM_START_GOLF_SHOT:
		jc_writeb(0x03, 0x0B, 0x18);
		break;

	case EXT_CAM_STOP_GOLF_SHOT:
		jc_writeb(0x03, 0x0B, 0x08);
		break;

	default:
		break;
	}

	if (copy_to_user((void __user *)ioctl_ptr->ioctl_ptr,
		  (const void *)&ctrl_info,
			sizeof(ctrl_info))) {
		goto FAIL_END;
	}

	return;

FAIL_END:
	pr_debug("these people are SO FUCKING addicted to logging");
	pr_debug("that this goto statement was just to print");
	pr_debug("another fucking log message");
}

static int32_t jc_sensor_setting(struct msm_sensor_ctrl_t *s_ctrl,
			int update_type, int res)
{
	int32_t rc = 0;
	int temp = 0;
	uint32_t op_pixel_clk = 320000000;

	switch (update_type) {
	case MSM_SENSOR_REG_INIT:
		break;

	case MSM_SENSOR_UPDATE_PERIODIC:
		v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
			NOTIFY_PCLK_CHANGE, &op_pixel_clk);

		/* set sizes - preview, capture */
		jc_set_sizes();

		/* set hw vdis mode */
		jc_set_hw_vdis();

		/* start YUV-output */
		jc_writeb(JC_CATEGORY_SYS,
				JC_SYS_INT_EN, 0x01);
		jc_readb(JC_CATEGORY_SYS,
			JC_SYS_INT_EN, &temp);
#if 0
		isp_debug_flag = 1;
#endif
		break;

	default:
		rc = -EINVAL;
		break;
	}

	return rc;
}

int32_t jc_set_sensor_mode_init(struct msm_sensor_ctrl_t *s_ctrl,
	int mode, int res)
{
	int32_t rc = 0;
	version_checked = false;

	if (mode != s_ctrl->cam_mode) {
		s_ctrl->curr_res = MSM_SENSOR_INVALID_RES;
		s_ctrl->cam_mode = mode;

		rc = jc_sensor_setting(s_ctrl,
			MSM_SENSOR_REG_INIT, 0);
	}

	return rc;
	}

int32_t jc_set_sensor_mode(struct msm_sensor_ctrl_t *s_ctrl,
	int mode, int res)
{
	int32_t rc = 0;

	rc = jc_sensor_setting(s_ctrl,
		MSM_SENSOR_UPDATE_PERIODIC, res);

	if (rc < 0)
		return rc;
	s_ctrl->curr_res = res;

	return rc;
}

static struct msm_cam_clk_info cam_clk_info[] = {
	{"cam_clk", MSM_SENSOR_MCLK_24HZ},
};

static struct msm_sensor_output_info_t jc_dimensions[] = {
	{
		.x_output = 0x1070,
		.y_output = 0xC30,
		.line_length_pclk = 0x1178,
		.frame_length_lines = 0xC90,
		.vt_pixel_clk = 182400000,
		.op_pixel_clk = 182400000,
		.binning_factor = 1,
	},
	{
		.x_output = 0x838,
		.y_output = 0x618,
		.line_length_pclk = 0x1178,
		.frame_length_lines = 0x634,
		.vt_pixel_clk = 216000000,
		.op_pixel_clk = 108000000,
		.binning_factor = 2,
	},
};

static int jc_sensor_power_up(struct msm_sensor_ctrl_t *s_ctrl)
{
	int rc = 0;
	int err;
	int result_sensor_phone, result_isp_phone, result_sensor_isp;
	int isp_revision = 0;
	int isp_ret;

	struct msm_camera_sensor_info *data = s_ctrl->sensordata;
	firmware_update_sdcard = false;
	jc_ctrl->burst_mode = false;
	jc_ctrl->stream_on = false;
	jc_ctrl->torch_mode = false;
	jc_ctrl->movie_mode = false;
	jc_ctrl->anti_stream_off = false;
	jc_ctrl->need_restart_caf = false;
	jc_ctrl->is_isp_null = false;
	jc_ctrl->isp_null_read_sensor_fw = false;
	jc_ctrl->touch_af_mode = false;
	jc_ctrl->fw_retry_cnt = 0;

	rc = msm_camera_request_gpio_table(data, 1);
	if (rc < 0)
		pr_debug("fuck");

	data->sensor_platform_info->
		sensor_pmic_gpio_ctrl(data->sensor_platform_info->irq_gpio, 2);

	/* Power on */
	data->sensor_platform_info->sensor_power_on();

	if (data->sensor_platform_info->i2c_conf &&
		data->sensor_platform_info->i2c_conf->use_i2c_mux)
		msm_sensor_enable_i2c_mux(data->sensor_platform_info->i2c_conf);

	/* MCLK */
	if (s_ctrl->clk_rate != 0)
		cam_clk_info->clk_rate = s_ctrl->clk_rate;

	rc = msm_cam_clk_enable(&s_ctrl->sensor_i2c_client->client->dev,
		cam_clk_info, s_ctrl->cam_clk, ARRAY_SIZE(cam_clk_info), 1);

	usleep(500);

	/* ISP_RESET */
	data->sensor_platform_info->
		sensor_pmic_gpio_ctrl(data->sensor_platform_info->reset, 1);

	if (wait_event_interruptible_timeout(jc_ctrl->isp.wait,
		jc_ctrl->isp.issued == 1,
		msecs_to_jiffies(5000)) == 0) {
		return 0;
	} else
	    jc_ctrl->isp.issued = 0;

#ifndef JC_SPI_WRITE
	jc_load_fw_main();
#endif

	if (version_checked == false && jc_ctrl->fw_update == true) {
		isp_ret = jc_get_isp_version();
		jc_isp_boot();
		jc_get_sensor_version();
		jc_get_phone_version();

		if(!jc_check_sensor_validation() /*|| !jc_check_sensor_spi_port()*/) {
			jc_sensor_power_down(&jc_s_ctrl);
			return -ENOSYS;
		}

		if (isp_ret == 0 && jc_ctrl->samsung_app == false && jc_ctrl->factory_bin == false) {
		    goto start;
		}

		jc_ctrl->fw_update = false;

		if (firmware_update_sdcard == true) {

			result_isp_phone = jc_check_isp_phone();

			if (result_isp_phone != 0) {
				jc_isp_reset(s_ctrl);
				jc_load_SIO_fw();
				jc_phone_fw_to_isp();
				jc_sensor_power_reset(s_ctrl);
				jc_get_isp_version();
				jc_isp_boot();
			}
		} else if (isp_ret < 0) {
			    jc_isp_reset(s_ctrl);
                       jc_load_SIO_fw();
                       jc_get_sensor_version();

                       if(!jc_check_sensor_validation() /*|| !jc_check_sensor_spi_port()*/) {
                           jc_sensor_power_down(&jc_s_ctrl);
                           return -ENOSYS;
                       }
                       result_sensor_phone = jc_check_sensor_phone();

                       if (result_sensor_phone > 0) {
			        jc_ctrl->isp_null_read_sensor_fw = true;
                           jc_read_from_sensor_fw();
                           jc_sensor_power_reset(s_ctrl);
                           jc_get_isp_version();
                           jc_isp_boot();

                       } else {
                           jc_phone_fw_to_isp();
                           jc_sensor_power_reset(s_ctrl);
                           jc_get_isp_version();
                           jc_isp_boot();
                       }
                   }else {
			result_sensor_phone = jc_check_sensor_phone();

			if (result_sensor_phone > 0) {
				result_sensor_isp = jc_check_sensor_isp();

				if (result_sensor_isp > 0) {
					jc_isp_reset(s_ctrl);
					jc_load_SIO_fw();
					jc_read_from_sensor_fw();
					jc_sensor_power_reset(s_ctrl);
					jc_get_isp_version();
					jc_isp_boot();
				} else
					pr_debug("Sensor <= ISP, Do not anything\n");
			} else {
				pr_debug("Sensor < Phone, compare with ISP\n");

			result_isp_phone = jc_check_isp_phone();

			if (result_isp_phone < 0) {
				jc_isp_reset(s_ctrl);
				jc_load_SIO_fw();
				jc_phone_fw_to_isp();
				jc_sensor_power_reset(s_ctrl);
				jc_get_isp_version();
				jc_isp_boot();
			} else
				pr_debug("Phone <= ISP, Do not anything\n");
			}
		}
	}else {
		jc_isp_boot();

		if(!jc_check_sensor_validation() /*|| !jc_check_sensor_spi_port()*/) {
			jc_sensor_power_down(&jc_s_ctrl);
			return -ENOSYS;
		}
	}

start:
	err = jc_writeb(JC_CATEGORY_CAPCTRL,
			0x0, 0x0f);

	if (jc_ctrl->samsung_app != 1) {
		jc_set_different_ratio_capture(1);
	}

	err = jc_readb(0x01, 0x3F, &isp_revision);

	return rc;
}
static int jc_sensor_power_down(struct msm_sensor_ctrl_t *s_ctrl)
{
	int rc = 0;

	struct msm_camera_sensor_info *data = s_ctrl->sensordata;

	jc_set_softlanding();

	if (data->sensor_platform_info->i2c_conf &&
		data->sensor_platform_info->i2c_conf->use_i2c_mux)
		msm_sensor_disable_i2c_mux(
			data->sensor_platform_info->i2c_conf);

	/* ISP_RESET */
	data->sensor_platform_info->
		sensor_pmic_gpio_ctrl(data->sensor_platform_info->reset, 0);

	/* AF off */
	data->sensor_platform_info->sensor_af_power_off();

	usleep(10*1000); /* Add 1ms delay for off timing */

	/* MCLK */
	rc = msm_cam_clk_enable(&s_ctrl->sensor_i2c_client->client->dev,
		cam_clk_info, s_ctrl->cam_clk, ARRAY_SIZE(cam_clk_info), 0);

	/* Power off */
	data->sensor_platform_info->sensor_power_off();

	data->sensor_platform_info->
		sensor_pmic_gpio_ctrl(data->sensor_platform_info->irq_gpio, 3);

	msm_camera_request_gpio_table(data, 0);

	return rc;
}

static int jc_sensor_power_reset(struct msm_sensor_ctrl_t *s_ctrl)
{
	int rc = 0;

	struct msm_camera_sensor_info *data = s_ctrl->sensordata;

	s_ctrl->func_tbl->sensor_power_down(s_ctrl);
	rc = msm_camera_request_gpio_table(data, 1);

	data->sensor_platform_info->
		sensor_pmic_gpio_ctrl(data->sensor_platform_info->irq_gpio, 2);

	/* Power on */
	data->sensor_platform_info->sensor_power_on();

	if (data->sensor_platform_info->i2c_conf &&
		data->sensor_platform_info->i2c_conf->use_i2c_mux)
		msm_sensor_enable_i2c_mux(data->sensor_platform_info->i2c_conf);

	/* MCLK */
	if (s_ctrl->clk_rate != 0)
		cam_clk_info->clk_rate = s_ctrl->clk_rate;

	rc = msm_cam_clk_enable(&s_ctrl->sensor_i2c_client->client->dev,
		cam_clk_info, s_ctrl->cam_clk, ARRAY_SIZE(cam_clk_info), 1);

	usleep(500);

	/* ISP_RESET */
	data->sensor_platform_info->
		sensor_pmic_gpio_ctrl(data->sensor_platform_info->reset, 0);

	usleep(3*1000);

	data->sensor_platform_info->
		sensor_pmic_gpio_ctrl(data->sensor_platform_info->reset, 1);

	if (wait_event_interruptible_timeout(jc_ctrl->isp.wait,
		jc_ctrl->isp.issued == 1,
		msecs_to_jiffies(5000)) == 0) {
		return 0;
	} else
	      jc_ctrl->isp.issued = 0;

	msleep(100);

	return rc;
}

void jc_sensor_start_stream(struct msm_sensor_ctrl_t *s_ctrl)
{
	if (jc_ctrl->anti_stream_off == true) {
		jc_ctrl->anti_stream_off = false;
	} else {
		/* change to monitor mode */
		jc_set_mode(JC_MONITOR_MODE);
		jc_ctrl->stream_on = true;
		if (jc_ctrl->af_mode == 3) {
			jc_writeb(JC_CATEGORY_LENS,
					0x01, 0x03);
			jc_writeb(JC_CATEGORY_LENS,
					0x02, 0x01);
		} else if (jc_ctrl->af_mode == 4) {
			jc_writeb(JC_CATEGORY_LENS,
					0x01, 0x07);
			jc_writeb(JC_CATEGORY_LENS,
					0x02, 0x01);
		} else if (jc_ctrl->af_mode == 5) {
			msleep(50);
			jc_writeb(JC_CATEGORY_LENS,
					0x01, 0x04);
			jc_writeb(JC_CATEGORY_LENS,
					0x02, 0x01);
		} else if (jc_ctrl->af_mode == 6) {
			msleep(50);
			jc_writeb(JC_CATEGORY_LENS,
					0x01, 0x05);
			jc_writeb(JC_CATEGORY_LENS,
					0x02, 0x01);
		}
	}
}

static int jc_sensor_start_stream_internal(struct msm_sensor_ctrl_t *s_ctrl)
{
	int rc = 0;

	if (jc_ctrl->anti_stream_off == true) {
		jc_ctrl->anti_stream_off = false;
	} else {
		/* change to monitor mode */
		rc = jc_set_mode(JC_MONITOR_MODE);
		jc_ctrl->stream_on = true;
		if (jc_ctrl->af_mode == 3) {
			jc_writeb(JC_CATEGORY_LENS,
					0x01, 0x03);
			jc_writeb(JC_CATEGORY_LENS,
					0x02, 0x01);
		} else if (jc_ctrl->af_mode == 4) {
			jc_writeb(JC_CATEGORY_LENS,
					0x01, 0x07);
			jc_writeb(JC_CATEGORY_LENS,
					0x02, 0x01);
		} else if (jc_ctrl->af_mode == 5) {
			msleep(50);
			jc_writeb(JC_CATEGORY_LENS,
					0x01, 0x04);
			jc_writeb(JC_CATEGORY_LENS,
					0x02, 0x01);
		} else if (jc_ctrl->af_mode == 6) {
			msleep(50);
			jc_writeb(JC_CATEGORY_LENS,
					0x01, 0x05);
			jc_writeb(JC_CATEGORY_LENS,
					0x02, 0x01);
		}
	}

	return rc;
}

void jc_sensor_stop_stream(struct msm_sensor_ctrl_t *s_ctrl)
{
	if (jc_ctrl->anti_stream_off == false) {
		/* change to parameter mode */
		jc_set_mode(JC_PARMSET_MODE);
		jc_ctrl->stream_on = false;
	}
}

static int jc_sensor_stop_stream_internal(struct msm_sensor_ctrl_t *s_ctrl)
{
	int rc = 0;

	if (jc_ctrl->anti_stream_off == false) {
		/* change to parameter mode */
		rc = jc_set_mode(JC_PARMSET_MODE);
		jc_ctrl->stream_on = false;
	}

	return rc;
}

long jc_sensor_subdev_ioctl(struct v4l2_subdev *sd,
			unsigned int cmd, void *arg)
{
	struct msm_sensor_ctrl_t *s_ctrl = get_sctrl(sd);
	void __user *argp = (void __user *)arg;

	switch (cmd) {
	case VIDIOC_MSM_SENSOR_CFG:
		return jc_sensor_config(&jc_s_ctrl, argp);
	case VIDIOC_MSM_SENSOR_CSID_INFO: {
		struct msm_sensor_csi_info *csi_info =
			(struct msm_sensor_csi_info *)arg;
		s_ctrl->is_csic = csi_info->is_csic;
		return 0;
	}
	default:
		return 0; /*-ENOIOCTLCMD;*/
	}
}

int jc_sensor_config(struct msm_sensor_ctrl_t *s_ctrl,
	void __user *argp)
{
	struct sensor_cfg_data cfg_data;
	long rc = 0;

	if (copy_from_user(&cfg_data,
			   (void *)argp, sizeof(struct sensor_cfg_data)))
		return -EFAULT;

	switch (cfg_data.cfgtype) {
	case CFG_SENSOR_INIT:
		rc = jc_set_sensor_mode_init(s_ctrl, cfg_data.mode,
			cfg_data.rs);
		break;

	case CFG_SET_MODE:
		rc = jc_set_sensor_mode(s_ctrl, cfg_data.mode,
			cfg_data.rs);
		break;

	case CFG_GET_CSI_PARAMS:
		if (s_ctrl->func_tbl->sensor_get_csi_params == NULL) {
			rc = -EFAULT;
			break;
		}
		rc = s_ctrl->func_tbl->sensor_get_csi_params(
			s_ctrl,
			&cfg_data.cfg.csi_lane_params);

		if (copy_to_user((void *)argp,
			&cfg_data,
			sizeof(struct sensor_cfg_data)))
			rc = -EFAULT;
		break;


	case CFG_POWER_UP:
		if (s_ctrl->func_tbl->sensor_power_up)
			rc = s_ctrl->func_tbl->sensor_power_up(s_ctrl);
		else
			rc = -EFAULT;
		break;

	case CFG_POWER_DOWN:
		if (s_ctrl->func_tbl->sensor_power_down)
			rc = s_ctrl->func_tbl->sensor_power_down(
				s_ctrl);
		else
			rc = -EFAULT;
		break;

	case CFG_START_STREAM:
		rc = jc_sensor_start_stream_internal(s_ctrl);
		break;

	case CFG_STOP_STREAM:
		rc = jc_sensor_stop_stream_internal(s_ctrl);
		break;

	case CFG_GET_AF_MAX_STEPS:
	default:
		rc = 0;
		break;
	}

	return rc;
}

struct v4l2_subdev_info jc_subdev_info[] = {
	{
	 .code = V4L2_MBUS_FMT_YUYV8_2X8,
	 .colorspace = V4L2_COLORSPACE_JPEG,
	 .fmt = 1,
	 .order = 0,
	 },
	/* more can be supported, to be added later */
};

static int jc_enum_fmt(struct v4l2_subdev *sd, unsigned int index,
			   enum v4l2_mbus_pixelcode *code)
{
	if ((unsigned int)index >= ARRAY_SIZE(jc_subdev_info))
		return -EINVAL;

	*code = jc_subdev_info[index].code;
	return 0;
}

static struct v4l2_subdev_core_ops jc_subdev_core_ops = {
	.s_ctrl = msm_sensor_v4l2_s_ctrl,
	.queryctrl = msm_sensor_v4l2_query_ctrl,
	.ioctl = jc_sensor_subdev_ioctl,
	.s_power = msm_sensor_power,
};

static struct v4l2_subdev_video_ops jc_subdev_video_ops = {
	.enum_mbus_fmt = jc_enum_fmt,
};

static struct v4l2_subdev_ops jc_subdev_ops = {
	.core = &jc_subdev_core_ops,
	.video = &jc_subdev_video_ops,
};

static ssize_t jc_camera_type_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", sysfs_sensor_type);
}

static ssize_t jc_camera_fw_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s %s\n", sysfs_isp_fw_str, sysfs_phone_fw_str);
}

static ssize_t jc_camera_check_fw_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s %s %s\n", sysfs_isp_fw_str, sysfs_phone_fw_str, sysfs_sensor_fw_str);
}

static ssize_t jc_camera_check_app_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", jc_ctrl->samsung_app);
}

static ssize_t jc_camera_check_app_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	unsigned long value = simple_strtoul(buf, NULL, 0);

	if (value == 0) {
	    jc_ctrl->samsung_app = false;
	} else {
	    jc_ctrl->samsung_app = true;
	}

	return size;
}

static DEVICE_ATTR(rear_camtype, S_IRUGO, jc_camera_type_show, NULL);
static DEVICE_ATTR(rear_camfw, S_IRUGO, jc_camera_fw_show, NULL);
static DEVICE_ATTR(rear_checkfw, S_IRUGO, jc_camera_check_fw_show, NULL);
static DEVICE_ATTR(rear_checkApp, S_IRUGO|S_IWUSR|S_IWGRP,
    jc_camera_check_app_show, jc_camera_check_app_store);

int32_t jc_init_vreg_port(struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;
	struct msm_camera_sensor_info *data = s_ctrl->sensordata;

	s_ctrl->reg_ptr = kzalloc(sizeof(struct regulator *)
			* data->sensor_platform_info->num_vreg, GFP_KERNEL);
	if (!s_ctrl->reg_ptr) {
		return -ENOMEM;
	}

	// vreg on setting
	rc = msm_camera_config_vreg(
		&s_ctrl->sensor_i2c_client->client->dev,
		s_ctrl->sensordata->sensor_platform_info->cam_vreg,
		s_ctrl->sensordata->sensor_platform_info->num_vreg,
		s_ctrl->vreg_seq,
		s_ctrl->num_vreg_seq,
		s_ctrl->reg_ptr, 1);
	if (rc < 0) {
		goto config_vreg_failed;
	}

	rc = msm_camera_enable_vreg(
		&s_ctrl->sensor_i2c_client->client->dev,
		s_ctrl->sensordata->sensor_platform_info->cam_vreg,
		s_ctrl->sensordata->sensor_platform_info->num_vreg,
		s_ctrl->vreg_seq,
		s_ctrl->num_vreg_seq,
		s_ctrl->reg_ptr, 1);
	if (rc < 0) {
		goto enable_vreg_failed;
	}

	// vreg off setting
	msm_camera_enable_vreg(
		&s_ctrl->sensor_i2c_client->client->dev,
		s_ctrl->sensordata->sensor_platform_info->cam_vreg,
		s_ctrl->sensordata->sensor_platform_info->num_vreg,
		s_ctrl->vreg_seq,
		s_ctrl->num_vreg_seq,
		s_ctrl->reg_ptr, 0);
enable_vreg_failed:
	msm_camera_config_vreg(
		&s_ctrl->sensor_i2c_client->client->dev,
		s_ctrl->sensordata->sensor_platform_info->cam_vreg,
		s_ctrl->sensordata->sensor_platform_info->num_vreg,
		s_ctrl->vreg_seq,
		s_ctrl->num_vreg_seq,
		s_ctrl->reg_ptr, 0);
config_vreg_failed:
	kfree(s_ctrl->reg_ptr);
	return rc;
}

static int jc_i2c_probe(struct i2c_client *client,
			    const struct i2c_device_id *id)
{
	int rc = 0;
	int err;
	struct msm_sensor_ctrl_t *s_ctrl;


	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		rc = -ENOTSUPP;
		goto probe_failure;
	}

	rc = jc_spi_init();
	if (rc)
		pr_debug("fuckity fucking hell fuck");

	s_ctrl = (struct msm_sensor_ctrl_t *)(id->driver_data);
	if (s_ctrl->sensor_i2c_client != NULL) {
		s_ctrl->sensor_i2c_client->client = client;
		if (s_ctrl->sensor_i2c_addr != 0)
			s_ctrl->sensor_i2c_client->client->addr =
				s_ctrl->sensor_i2c_addr;
	} else {
		rc = -EFAULT;
		return rc;
	}

	s_ctrl->sensordata = client->dev.platform_data;
	if (s_ctrl->sensordata == NULL) {
		return -EFAULT;
	}

	jc_client = client;
	jc_dev = s_ctrl->sensor_i2c_client->client->dev;

	jc_ctrl = kzalloc(sizeof(struct jc_ctrl_t), GFP_KERNEL);
	if (!jc_ctrl) {
		return -ENOMEM;
	}

	memset(jc_ctrl, 0, sizeof(*jc_ctrl));

	snprintf(s_ctrl->sensor_v4l2_subdev.name,
		sizeof(s_ctrl->sensor_v4l2_subdev.name), "%s", id->name);

	v4l2_i2c_subdev_init(&s_ctrl->sensor_v4l2_subdev, client,
		&jc_subdev_ops);

	s_ctrl->sensor_v4l2_subdev.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	media_entity_init(&s_ctrl->sensor_v4l2_subdev.entity, 0, NULL, 0);
	s_ctrl->sensor_v4l2_subdev.entity.type = MEDIA_ENT_T_V4L2_SUBDEV;
	s_ctrl->sensor_v4l2_subdev.entity.group_id = SENSOR_DEV;
	s_ctrl->sensor_v4l2_subdev.entity.name =
		s_ctrl->sensor_v4l2_subdev.name;

	jc_ctrl->sensor_dev = &s_ctrl->sensor_v4l2_subdev;
	jc_ctrl->sensordata = client->dev.platform_data;

	/* wait queue initialize */
	init_waitqueue_head(&jc_ctrl->isp.wait);

	if (s_ctrl->sensordata->sensor_platform_info->config_isp_irq)
		s_ctrl->sensordata->sensor_platform_info->config_isp_irq();

	err = request_irq(s_ctrl->sensordata->sensor_platform_info->irq,
		jc_isp_isr, IRQF_TRIGGER_RISING, "jc isp", NULL);
	if (err) {
		kfree(jc_ctrl);
		return err;
	}
	jc_ctrl->isp.irq = s_ctrl->sensordata->sensor_platform_info->irq;
	jc_ctrl->isp.issued = 0;

	msm_sensor_register(&s_ctrl->sensor_v4l2_subdev);

	s_ctrl->sensor_v4l2_subdev.entity.revision =
		s_ctrl->sensor_v4l2_subdev.devnode->num;

	jc_ctrl->fw_update = true;
	jc_ctrl->samsung_app = false;
	jc_ctrl->factory_bin = false;
	jc_ctrl->system_rev = s_ctrl->sensordata->sensor_platform_info->sys_rev();

	/* to solve sleep current issue */
	jc_init_vreg_port(s_ctrl);

	/* VDDIO off */
	s_ctrl->sensordata->sensor_platform_info->sensor_vddio_power_off();

	/* MCLK */
	rc = msm_cam_clk_enable(&s_ctrl->sensor_i2c_client->client->dev,
		cam_clk_info, s_ctrl->cam_clk, ARRAY_SIZE(cam_clk_info), 0);

	return 0;

probe_failure:
	return rc;
}

static const struct i2c_device_id jc_i2c_id[] = {
	{"jc", (kernel_ulong_t)&jc_s_ctrl},
	{},
};

static struct msm_camera_i2c_client jc_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};

static struct i2c_driver jc_i2c_driver = {
	.id_table = jc_i2c_id,
	.probe = jc_i2c_probe,
	.driver = {
		   .name = "jc",
	},
};

static int __init jc_init(void)
{
	struct device *cam_dev_rear = NULL;

	camera_class = class_create(THIS_MODULE, "camera");
	if (IS_ERR(camera_class)) {
		cam_err("Failed to create class(camera)!\n");
		return 0;
	}

	cam_dev_rear =
	device_create(camera_class, NULL, 0, NULL, "rear");
	if (IS_ERR(cam_dev_rear)) {
		cam_err("failed to create device cam_dev_rear!\n");
		return 0;
	}

	if (device_create_file
	(cam_dev_rear, &dev_attr_rear_camtype) < 0) {
		cam_err("failed to create device file, %s\n",
		dev_attr_rear_camtype.attr.name);
	}

	if (device_create_file
	(cam_dev_rear, &dev_attr_rear_camfw) < 0) {
		cam_err("failed to create device file, %s\n",
		dev_attr_rear_camfw.attr.name);
	}

	if (device_create_file
	(cam_dev_rear, &dev_attr_rear_checkfw) < 0) {
		cam_err("failed to create device file, %s\n",
		dev_attr_rear_checkfw.attr.name);
	}

	if (device_create_file
	(cam_dev_rear, &dev_attr_rear_checkApp) < 0) {
		cam_err("failed to create device file, %s\n",
		dev_attr_rear_checkApp.attr.name);
	}

	return i2c_add_driver(&jc_i2c_driver);
}

static struct msm_sensor_fn_t jc_func_tbl = {
	.sensor_config = jc_sensor_config,
	.sensor_power_up = jc_sensor_power_up,
	.sensor_power_down = jc_sensor_power_down,
	.sensor_get_csi_params = msm_sensor_get_csi_params,
	.sensor_start_stream = jc_sensor_start_stream,
	.sensor_stop_stream = jc_sensor_stop_stream,
};

static struct msm_sensor_reg_t jc_regs = {
	.default_data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.output_settings = &jc_dimensions[0],
};

static struct msm_sensor_ctrl_t jc_s_ctrl = {
	.msm_sensor_reg = &jc_regs,
	.sensor_i2c_client = &jc_sensor_i2c_client,
	.sensor_i2c_addr = 0x3E,
	.cam_mode = MSM_SENSOR_MODE_INVALID,
	.msm_sensor_mutex = &jc_mut,
	.sensor_i2c_driver = &jc_i2c_driver,
	.sensor_v4l2_subdev_info = jc_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(jc_subdev_info),
	.sensor_v4l2_subdev_ops = &jc_subdev_ops,
	.func_tbl = &jc_func_tbl,
	.clk_rate = MSM_SENSOR_MCLK_24HZ,
};

module_init(jc_init);
