/* include/linux/msm_mdp.h
 *
 * Copyright (C) 2007 Google Incorporated
 * Copyright (c) 2012-2013 The Linux Foundation. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef _MSM_MDP_H_
#define _MSM_MDP_H_

#include <linux/types.h>
#include <linux/fb.h>

#include <uapi/linux/msm_mdp.h>

#define MSMFB_IOCTL_MAGIC 'm'
#define MSMFB_GRP_DISP          _IOW(MSMFB_IOCTL_MAGIC, 1, unsigned int)
#define MSMFB_BLIT              _IOW(MSMFB_IOCTL_MAGIC, 2, unsigned int)
#define MSMFB_SUSPEND_SW_REFRESHER _IOW(MSMFB_IOCTL_MAGIC, 128, unsigned int)
#define MSMFB_RESUME_SW_REFRESHER _IOW(MSMFB_IOCTL_MAGIC, 129, unsigned int)
#define MSMFB_CURSOR _IOW(MSMFB_IOCTL_MAGIC, 130, struct fb_cursor)
#define MSMFB_SET_LUT _IOW(MSMFB_IOCTL_MAGIC, 131, struct fb_cmap)
#define MSMFB_HISTOGRAM _IOWR(MSMFB_IOCTL_MAGIC, 132, struct mdp_histogram_data)
/* new ioctls's for set/get ccs matrix */
#define MSMFB_GET_CCS_MATRIX  _IOWR(MSMFB_IOCTL_MAGIC, 133, struct mdp_ccs)
#define MSMFB_SET_CCS_MATRIX  _IOW(MSMFB_IOCTL_MAGIC, 134, struct mdp_ccs)
#define MSMFB_OVERLAY_SET       _IOWR(MSMFB_IOCTL_MAGIC, 135, \
						struct mdp_overlay)
#define MSMFB_OVERLAY_UNSET     _IOW(MSMFB_IOCTL_MAGIC, 136, unsigned int)

#define MSMFB_OVERLAY_PLAY      _IOW(MSMFB_IOCTL_MAGIC, 137, \
						struct msmfb_overlay_data)
#define MSMFB_OVERLAY_QUEUE	MSMFB_OVERLAY_PLAY

#define MSMFB_GET_PAGE_PROTECTION _IOR(MSMFB_IOCTL_MAGIC, 138, \
					struct mdp_page_protection)
#define MSMFB_SET_PAGE_PROTECTION _IOW(MSMFB_IOCTL_MAGIC, 139, \
					struct mdp_page_protection)
#define MSMFB_OVERLAY_GET      _IOR(MSMFB_IOCTL_MAGIC, 140, \
						struct mdp_overlay)
#define MSMFB_OVERLAY_PLAY_ENABLE     _IOW(MSMFB_IOCTL_MAGIC, 141, unsigned int)
#define MSMFB_OVERLAY_BLT       _IOWR(MSMFB_IOCTL_MAGIC, 142, \
						struct msmfb_overlay_blt)
#define MSMFB_OVERLAY_BLT_OFFSET     _IOW(MSMFB_IOCTL_MAGIC, 143, unsigned int)
#define MSMFB_HISTOGRAM_START	_IOR(MSMFB_IOCTL_MAGIC, 144, \
						struct mdp_histogram_start_req)
#define MSMFB_HISTOGRAM_STOP	_IOR(MSMFB_IOCTL_MAGIC, 145, unsigned int)
#define MSMFB_NOTIFY_UPDATE	_IOW(MSMFB_IOCTL_MAGIC, 146, unsigned int)

#define MSMFB_OVERLAY_3D       _IOWR(MSMFB_IOCTL_MAGIC, 147, \
						struct msmfb_overlay_3d)

#define MSMFB_MIXER_INFO       _IOWR(MSMFB_IOCTL_MAGIC, 148, \
						struct msmfb_mixer_info_req)
#define MSMFB_OVERLAY_PLAY_WAIT _IOWR(MSMFB_IOCTL_MAGIC, 149, \
						struct msmfb_overlay_data)
#define MSMFB_WRITEBACK_INIT _IO(MSMFB_IOCTL_MAGIC, 150)
#define MSMFB_WRITEBACK_START _IO(MSMFB_IOCTL_MAGIC, 151)
#define MSMFB_WRITEBACK_STOP _IO(MSMFB_IOCTL_MAGIC, 152)
#define MSMFB_WRITEBACK_QUEUE_BUFFER _IOW(MSMFB_IOCTL_MAGIC, 153, \
						struct msmfb_data)
#define MSMFB_WRITEBACK_DEQUEUE_BUFFER _IOW(MSMFB_IOCTL_MAGIC, 154, \
						struct msmfb_data)
#define MSMFB_WRITEBACK_TERMINATE _IO(MSMFB_IOCTL_MAGIC, 155)
#define MSMFB_MDP_PP _IOWR(MSMFB_IOCTL_MAGIC, 156, struct msmfb_mdp_pp)
#define MSMFB_OVERLAY_VSYNC_CTRL _IOW(MSMFB_IOCTL_MAGIC, 160, unsigned int)
#define MSMFB_VSYNC_CTRL  _IOW(MSMFB_IOCTL_MAGIC, 161, unsigned int)
#define MSMFB_BUFFER_SYNC  _IOW(MSMFB_IOCTL_MAGIC, 162, struct mdp_buf_sync)
#define MSMFB_OVERLAY_COMMIT      _IO(MSMFB_IOCTL_MAGIC, 163)
#define MSMFB_DISPLAY_COMMIT      _IOW(MSMFB_IOCTL_MAGIC, 164, \
						struct mdp_display_commit)
#define MSMFB_WRITEBACK_SET_MIRRORING_HINT _IOW(MSMFB_IOCTL_MAGIC, 165, \
						unsigned int)
#define MSMFB_METADATA_SET  _IOW(MSMFB_IOCTL_MAGIC, 165, struct msmfb_metadata)
#define MSMFB_METADATA_GET  _IOW(MSMFB_IOCTL_MAGIC, 166, struct msmfb_metadata)

#define FB_TYPE_3D_PANEL 0x10101010
#define MDP_IMGTYPE2_START 0x10000
#define MSMFB_DRIVER_VERSION	0xF9E8D701

enum {
	NOTIFY_UPDATE_START,
	NOTIFY_UPDATE_STOP,
};

enum {
	MDP_RGB_565,      /* RGB 565 planer */
	MDP_XRGB_8888,    /* RGB 888 padded */
	MDP_Y_CBCR_H2V2,  /* Y and CbCr, pseudo planer w/ Cb is in MSB */
	MDP_Y_CBCR_H2V2_ADRENO,
	MDP_ARGB_8888,    /* ARGB 888 */
	MDP_RGB_888,      /* RGB 888 planer */
	MDP_Y_CRCB_H2V2,  /* Y and CrCb, pseudo planer w/ Cr is in MSB */
	MDP_YCBYCR_H2V1,  /* YCbYCr interleave */
	MDP_YCRYCB_H2V1,  /* YCrYCb interleave */
	MDP_Y_CRCB_H2V1,  /* Y and CrCb, pseduo planer w/ Cr is in MSB */
	MDP_Y_CBCR_H2V1,   /* Y and CrCb, pseduo planer w/ Cr is in MSB */
	MDP_Y_CRCB_H1V2,
	MDP_Y_CBCR_H1V2,
	MDP_RGBA_8888,    /* ARGB 888 */
	MDP_BGRA_8888,	  /* ABGR 888 */
	MDP_RGBX_8888,	  /* RGBX 888 */
	MDP_Y_CRCB_H2V2_TILE,  /* Y and CrCb, pseudo planer tile */
	MDP_Y_CBCR_H2V2_TILE,  /* Y and CbCr, pseudo planer tile */
	MDP_Y_CR_CB_H2V2,  /* Y, Cr and Cb, planar */
	MDP_Y_CR_CB_GH2V2,  /* Y, Cr and Cb, planar aligned to Android YV12 */
	MDP_Y_CB_CR_H2V2,  /* Y, Cb and Cr, planar */
	MDP_Y_CRCB_H1V1,  /* Y and CrCb, pseduo planer w/ Cr is in MSB */
	MDP_Y_CBCR_H1V1,  /* Y and CbCr, pseduo planer w/ Cb is in MSB */
	MDP_YCRCB_H1V1,   /* YCrCb interleave */
	MDP_YCBCR_H1V1,   /* YCbCr interleave */
	MDP_BGR_565,      /* BGR 565 planer */
	MDP_IMGTYPE_LIMIT,
	MDP_RGB_BORDERFILL,	/* border fill pipe */
	MDP_FB_FORMAT = MDP_IMGTYPE2_START,    /* framebuffer format */
	MDP_IMGTYPE_LIMIT2 /* Non valid image type after this enum */
};

enum {
	PMEM_IMG,
	FB_IMG,
};

enum {
	HSIC_HUE = 0,
	HSIC_SAT,
	HSIC_INT,
	HSIC_CON,
	NUM_HSIC_PARAM,
};

#define MDSS_MDP_ROT_ONLY		0x80
#define MDSS_MDP_RIGHT_MIXER		0x100

/* mdp_blit_req flag values */
#define MDP_ROT_NOP 0
#define MDP_FLIP_LR 0x1
#define MDP_FLIP_UD 0x2
#define MDP_ROT_90 0x4
#define MDP_ROT_180 (MDP_FLIP_UD|MDP_FLIP_LR)
#define MDP_ROT_270 (MDP_ROT_90|MDP_FLIP_UD|MDP_FLIP_LR)
#define MDP_DITHER 0x8
#define MDP_BLUR 0x10
#define MDP_BLEND_FG_PREMULT 0x20000
#define MDP_IS_FG 0x40000
#define MDP_DEINTERLACE 0x80000000
#define MDP_SHARPENING  0x40000000
#define MDP_NO_DMA_BARRIER_START	0x20000000
#define MDP_NO_DMA_BARRIER_END		0x10000000
#define MDP_NO_BLIT			0x08000000
#define MDP_BLIT_WITH_DMA_BARRIERS	0x000
#define MDP_BLIT_WITH_NO_DMA_BARRIERS    \
	(MDP_NO_DMA_BARRIER_START | MDP_NO_DMA_BARRIER_END)
#define MDP_BLIT_SRC_GEM                0x04000000
#define MDP_BLIT_DST_GEM                0x02000000
#define MDP_BLIT_NON_CACHED		0x01000000
#define MDP_OV_PIPE_SHARE		0x00800000
#define MDP_DEINTERLACE_ODD		0x00400000
#define MDP_OV_PLAY_NOWAIT		0x00200000
#define MDP_SOURCE_ROTATED_90		0x00100000
#define MDP_OVERLAY_PP_CFG_EN		0x00080000
#define MDP_BACKEND_COMPOSITION		0x00040000
#define MDP_BORDERFILL_SUPPORTED	0x00010000
#define MDP_SECURE_OVERLAY_SESSION      0x00008000
#define MDP_MEMORY_ID_TYPE_FB		0x00001000

#define MDP_TRANSP_NOP 0xffffffff
#define MDP_ALPHA_NOP 0xff

#define MDP_FB_PAGE_PROTECTION_NONCACHED         (0)
#define MDP_FB_PAGE_PROTECTION_WRITECOMBINE      (1)
#define MDP_FB_PAGE_PROTECTION_WRITETHROUGHCACHE (2)
#define MDP_FB_PAGE_PROTECTION_WRITEBACKCACHE    (3)
#define MDP_FB_PAGE_PROTECTION_WRITEBACKWACACHE  (4)
/* Sentinel: Don't use! */
#define MDP_FB_PAGE_PROTECTION_INVALID           (5)
/* Count of the number of MDP_FB_PAGE_PROTECTION_... values. */
#define MDP_NUM_FB_PAGE_PROTECTION_VALUES        (5)

#define MDP_MAX_FENCE_FD	10
#define MDP_BUF_SYNC_FLAG_WAIT	1
#define MDP_DISPLAY_COMMIT_OVERLAY 0x00000001

#define MDP_CCS_RGB2YUV 	0
#define MDP_CCS_YUV2RGB 	1

#define MDP_CCS_SIZE	9
#define MDP_BV_SIZE	3

#define MDP_BLIT_REQ_VERSION 2

#define MSMFB_DATA_VERSION 2

#define MSMFB_NEW_REQUEST -1

#define MSMFB_WRITEBACK_DEQUEUE_BLOCKING 0x1

#define MDP_PP_OPS_ENABLE 0x1
#define MDP_PP_OPS_READ 0x2
#define MDP_PP_OPS_WRITE 0x4
#define MDP_PP_OPS_DISABLE 0x8

#define MDP_OVERLAY_PP_CSC_CFG      0x1
#define MDP_OVERLAY_PP_QSEED_CFG    0x2

#define MDP_CSC_FLAG_ENABLE	0x1
#define MDP_CSC_FLAG_YUV_IN	0x2
#define MDP_CSC_FLAG_YUV_OUT	0x4

/* get the framebuffer physical address information */
int get_fb_phys_info(unsigned long *start, unsigned long *len, int fb_num,
	int subsys_id);
struct fb_info *msm_fb_get_writeback_fb(void);
int msm_fb_writeback_init(struct fb_info *info);
int msm_fb_writeback_start(struct fb_info *info);
int msm_fb_writeback_queue_buffer(struct fb_info *info,
		struct msmfb_data *data);
int msm_fb_writeback_dequeue_buffer(struct fb_info *info,
		struct msmfb_data *data);
int msm_fb_writeback_stop(struct fb_info *info);
int msm_fb_writeback_terminate(struct fb_info *info);

#endif /*_MSM_MDP_H_*/
