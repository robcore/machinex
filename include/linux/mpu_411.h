/*
	$License:
	Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
	$
 */

#ifndef __MPU_H_
#define __MPU_H_

#include <sys/ioctl.h>
#include "mltypes.h"
#include <uapi/linux/mpu_411.h>

#define MPU_IOCTL (0x81) /* Magic number for MPU Iocts */
/* IOCTL commands for /dev/mpu */

/*--------------------------------------------------------------------------
 * Deprecated, debugging only
 */
#define MPU_SET_MPU_PLATFORM_DATA	\
	_IOWR(MPU_IOCTL, 0x01, struct mpu_platform_data)
#define MPU_SET_EXT_SLAVE_PLATFORM_DATA	\
	_IOWR(MPU_IOCTL, 0x01, struct ext_slave_platform_data)
/*--------------------------------------------------------------------------*/
#define MPU_GET_EXT_SLAVE_PLATFORM_DATA	\
	_IOWR(MPU_IOCTL, 0x02, struct ext_slave_platform_data)
#define MPU_GET_MPU_PLATFORM_DATA	\
	_IOWR(MPU_IOCTL, 0x02, struct mpu_platform_data)
#define MPU_GET_EXT_SLAVE_DESCR	\
	_IOWR(MPU_IOCTL, 0x02, struct ext_slave_descr)

#define MPU_READ		_IOWR(MPU_IOCTL, 0x10, struct mpu_read_write)
#define MPU_WRITE		_IOW(MPU_IOCTL,  0x10, struct mpu_read_write)
#define MPU_READ_MEM		_IOWR(MPU_IOCTL, 0x11, struct mpu_read_write)
#define MPU_WRITE_MEM		_IOW(MPU_IOCTL,  0x11, struct mpu_read_write)
#define MPU_READ_FIFO		_IOWR(MPU_IOCTL, 0x12, struct mpu_read_write)
#define MPU_WRITE_FIFO		_IOW(MPU_IOCTL,  0x12, struct mpu_read_write)

#define MPU_READ_COMPASS	_IOR(MPU_IOCTL, 0x12, __u8)
#define MPU_READ_ACCEL		_IOR(MPU_IOCTL, 0x13, __u8)
#define MPU_READ_PRESSURE	_IOR(MPU_IOCTL, 0x14, __u8)

#define MPU_CONFIG_GYRO		_IOW(MPU_IOCTL, 0x20, struct ext_slave_config)
#define MPU_CONFIG_ACCEL	_IOW(MPU_IOCTL, 0x21, struct ext_slave_config)
#define MPU_CONFIG_COMPASS	_IOW(MPU_IOCTL, 0x22, struct ext_slave_config)
#define MPU_CONFIG_PRESSURE	_IOW(MPU_IOCTL, 0x23, struct ext_slave_config)

#define MPU_GET_CONFIG_GYRO	_IOWR(MPU_IOCTL, 0x20, struct ext_slave_config)
#define MPU_GET_CONFIG_ACCEL	_IOWR(MPU_IOCTL, 0x21, struct ext_slave_config)
#define MPU_GET_CONFIG_COMPASS	_IOWR(MPU_IOCTL, 0x22, struct ext_slave_config)
#define MPU_GET_CONFIG_PRESSURE	_IOWR(MPU_IOCTL, 0x23, struct ext_slave_config)

#define MPU_SUSPEND		_IOW(MPU_IOCTL, 0x30, __u32)
#define MPU_RESUME		_IOW(MPU_IOCTL, 0x31, __u32)
/* Userspace PM Event response */
#define MPU_PM_EVENT_HANDLED	_IO(MPU_IOCTL, 0x32)

#define MPU_GET_REQUESTED_SENSORS	_IOR(MPU_IOCTL, 0x40, __u8)
#define MPU_SET_REQUESTED_SENSORS	_IOW(MPU_IOCTL, 0x40, __u8)
#define MPU_GET_IGNORE_SYSTEM_SUSPEND	_IOR(MPU_IOCTL, 0x41, __u8)
#define MPU_SET_IGNORE_SYSTEM_SUSPEND	_IOW(MPU_IOCTL, 0x41, __u8)
#define MPU_GET_MLDL_STATUS		_IOR(MPU_IOCTL, 0x42, __u8)
#define MPU_GET_I2C_SLAVES_ENABLED	_IOR(MPU_IOCTL, 0x43, __u8)
#define MPU_READ_ACCEL_OFFSET	_IOR(MPU_IOCTL, 0x44, __u8)
#define MPU_ACTIVATE_SENSORS	_IOW(MPU_IOCTL, 0x45, __u8)

#endif				/* __MPU_H_ */
