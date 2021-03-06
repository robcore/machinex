#ifndef __UAPI_LINUX_AVTIMER_H
#define __UAPI_LINUX_AVTIMER_H

#include <linux/ioctl.h>

#define MAJOR_NUM 100

#define IOCTL_GET_AVTIMER_TICK _IOR(MAJOR_NUM, 0, char *)
/*
 * This IOCTL is used to read the avtimer tick value.
 * Avtimer is a 64 bit timer tick, hence the expected
 * argument is of type uint64_t
 */
struct dev_avtimer_data {
	uint32_t avtimer_msw_phy_addr;
	uint32_t avtimer_lsw_phy_addr;
};

#endif
