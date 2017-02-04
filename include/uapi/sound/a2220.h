/* include/linux/a2220.h - a2220 voice processor driver
 *
 * Copyright (C) 2009 HTC Corporation.
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

#ifndef __UAPI_SND_A2220_H
#define __UAPI_SND_A2220_H

#include <linux/ioctl.h>

struct a2220img {
	unsigned char *buf;
	unsigned img_size;
};

enum A2220_PathID {
	A2220_PATH_SUSPEND,
	A2220_PATH_INCALL_RECEIVER_NSON,
	A2220_PATH_INCALL_RECEIVER_NSONOFF,
	A2220_PATH_INCALL_RECEIVER_NSOFFON,
	A2220_PATH_INCALL_RECEIVER_EXTON,
	A2220_PATH_INCALL_RECEIVER_EXTOFF,
	A2220_PATH_INCALL_RECEIVER_VOL0,
	A2220_PATH_INCALL_RECEIVER_VOL1,
	A2220_PATH_INCALL_RECEIVER_VOL2,
	A2220_PATH_INCALL_RECEIVER_VOL3,
	A2220_PATH_INCALL_RECEIVER_VOL4,
	A2220_PATH_INCALL_RECEIVER_VOL5,
	A2220_PATH_INCALL_RECEIVER_NEA_VOLMIN,
	A2220_PATH_INCALL_RECEIVER_NEA_VOLMAX,
	A2220_PATH_INCALL_RECEIVER_NSON_WB,
	A2220_PATH_INCALL_RECEIVER_NSOFF,
	A2220_PATH_INCALL_HEADSET,
	A2220_PATH_INCALL_SPEAKER,
	A2220_PATH_INCALL_BT,
	A2220_PATH_VR_NO_NS_RECEIVER,
	A2220_PATH_VR_NO_NS_HEADSET,
	A2220_PATH_VR_NO_NS_SPEAKER,
	A2220_PATH_VR_NO_NS_BT,
	A2220_PATH_VR_NS_RECEIVER,
	A2220_PATH_VR_NS_HEADSET,
	A2220_PATH_VR_NS_SPEAKER,
	A2220_PATH_VR_NS_BT,
	A2220_PATH_RECORD_RECEIVER,
	A2220_PATH_RECORD_HEADSET,
	A2220_PATH_RECORD_SPEAKER,
	A2220_PATH_RECORD_BT,
	A2220_PATH_CAMCORDER,
	A2220_PATH_INCALL_TTY,
	A2220_PATH_PCMRESET,
	A2220_PATH_FT_LOOPBACK,
#ifdef AUDIENCE_BYPASS
	A2220_PATH_BYPASS_MULTIMEDIA,
#endif
};

/* noise suppression states */
enum A2220_NS_states {
	A2220_NS_STATE_AUTO,    /* leave mode as selected by driver  */
	A2220_NS_STATE_OFF,     /* disable noise suppression */
	A2220_NS_STATE_CT,      /* force close talk mode */
	A2220_NS_STATE_FT,      /* force far talk mode */
	A2220_NS_NUM_STATES
};

/* indicates if a2220_set_config() performs a full configuration or only
 * a voice processing algorithm configuration */
/* IOCTLs for Audience A2220 */
#define A2220_IOCTL_MAGIC 'u'

#define A2220_BOOTUP_INIT  _IOW(A2220_IOCTL_MAGIC, 0x01, struct a2220img *)
#define A2220_SET_CONFIG   _IOW(A2220_IOCTL_MAGIC, 0x02, enum A2220_PathID)
#define A2220_SET_NS_STATE _IOW(A2220_IOCTL_MAGIC, 0x03, enum A2220_NS_states)
/* For Diag */
#define A2220_SET_MIC_ONOFF     _IOW(A2220_IOCTL_MAGIC, 0x50, unsigned)
#define A2220_SET_MICSEL_ONOFF  _IOW(A2220_IOCTL_MAGIC, 0x51, unsigned)
#define A2220_READ_DATA         _IOR(A2220_IOCTL_MAGIC, 0x52, unsigned)
#define A2220_WRITE_MSG         _IOW(A2220_IOCTL_MAGIC, 0x53, unsigned)
#define A2220_SYNC_CMD          _IO(A2220_IOCTL_MAGIC, 0x54)
#define A2220_SET_CMD_FILE      _IOW(A2220_IOCTL_MAGIC, 0x55, unsigned)

#endif /* __LINUX_A2220_H */

