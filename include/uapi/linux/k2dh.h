#ifndef __UAPI_k2dh_ACC_HEADER__
#define __UAPI_k2dh__ACC_HEADER__

#include <linux/types.h>
#include <linux/ioctl.h>

/* dev info */
#define ACC_DEV_NAME "accelerometer"

/* k2dh ioctl command label */
#define k2dh_IOCTL_BASE 'a'
#define k2dh_IOCTL_SET_DELAY		_IOW(k2dh_IOCTL_BASE, 0, int64_t)
#define k2dh_IOCTL_GET_DELAY		_IOR(k2dh_IOCTL_BASE, 1, int64_t)
#define k2dh_IOCTL_READ_ACCEL_XYZ	_IOR(k2dh_IOCTL_BASE, 8, \
						struct k2dh_acceldata)
#define k2dh_IOCTL_SET_ENABLE		_IOW(k2dh_IOCTL_BASE, 9, int)
#endif