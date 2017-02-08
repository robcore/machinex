#ifndef UAPI_MMC_IOCTL_H
#define UAPI_MMC_IOCTL_H

#include <linux/types.h>

#define mmc_ioc_cmd_set_data(ic, ptr) ic.data_ptr = (__u64)(unsigned long) ptr

#define MMC_IOC_CMD _IOWR(MMC_BLOCK_MAJOR, 0, struct mmc_ioc_cmd)

/*
 * Since this ioctl is only meant to enhance (and not replace) normal access
 * to the mmc bus device, an upper data transfer limit of MMC_IOC_MAX_BYTES
 * is enforced per ioctl call.  For larger data transfers, use the normal
 * block device operations.
 */
#define MMC_IOC_MAX_BYTES  (512L * 512)
#endif /* UAPI_MMC_IOCTL_H */
