/* Copyright (c) 2011, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __MSM_VFE7X_H__
#define __MSM_VFE7X_H__
#include <media/msm_camera.h>
#include <mach/camera.h>

struct vfe_frame_extra {
	uint32_t	bl_evencol:23;
	uint32_t	rvd1:9;
	uint32_t	bl_oddcol:23;
	uint32_t	rvd2:9;

	uint32_t	d_dbpc_stats_hot:16;
	uint32_t	d_dbpc_stats_cold:16;

	uint32_t	d_dbpc_stats_0_hot:10;
	uint32_t	rvd3:6;
	uint32_t	d_dbpc_stats_0_cold:10;
	uint32_t	rvd4:6;
	uint32_t	d_dbpc_stats_1_hot:10;
	uint32_t	rvd5:6;
	uint32_t	d_dbpc_stats_1_cold:10;
	uint32_t	rvd6:6;

	uint32_t	asf_max_edge;

	uint32_t	e_y_wm_pm_stats_0:21;
	uint32_t	rvd7:11;
	uint32_t	e_y_wm_pm_stats_1_bl:8;
	uint32_t	rvd8:8;
	uint32_t	e_y_wm_pm_stats_1_nl:12;
	uint32_t	rvd9:4;

	uint32_t	e_cbcr_wm_pm_stats_0:21;
	uint32_t	rvd10:11;
	uint32_t	e_cbcr_wm_pm_stats_1_bl:8;
	uint32_t	rvd11:8;
	uint32_t	e_cbcr_wm_pm_stats_1_nl:12;
	uint32_t	rvd12:4;

	uint32_t	v_y_wm_pm_stats_0:21;
	uint32_t	rvd13:11;
	uint32_t	v_y_wm_pm_stats_1_bl:8;
	uint32_t	rvd14:8;
	uint32_t	v_y_wm_pm_stats_1_nl:12;
	uint32_t	rvd15:4;

	uint32_t	v_cbcr_wm_pm_stats_0:21;
	uint32_t	rvd16:11;
	uint32_t	v_cbcr_wm_pm_stats_1_bl:8;
	uint32_t	rvd17:8;
	uint32_t	v_cbcr_wm_pm_stats_1_nl:12;
	uint32_t	rvd18:4;

	uint32_t      frame_id;
};

struct vfe_endframe {
	uint32_t      y_address;
	uint32_t      cbcr_address;

	struct vfe_frame_extra extra;
} __packed;

struct vfe_outputack {
	uint32_t  header;
	void      *output2newybufferaddress;
	void      *output2newcbcrbufferaddress;
} __packed;

struct vfe_stats_ack {
	uint32_t header;
	/* MUST BE 64 bit ALIGNED */
	void     *bufaddr;
} __packed;

/* AXI Output Config Command sent to DSP */
struct axiout {
	uint32_t            cmdheader:32;
	int                 outputmode:3;
	uint8_t             format:2;
	uint32_t            /* reserved */ : 27;

	/* AXI Output 1 Y Configuration, Part 1 */
	uint32_t            out1yimageheight:12;
	uint32_t            /* reserved */ : 4;
	uint32_t            out1yimagewidthin64bitwords:10;
	uint32_t            /* reserved */ : 6;

	/* AXI Output 1 Y Configuration, Part 2 */
	uint8_t             out1yburstlen:2;
	uint32_t            out1ynumrows:12;
	uint32_t            out1yrowincin64bitincs:12;
	uint32_t            /* reserved */ : 6;

	/* AXI Output 1 CbCr Configuration, Part 1 */
	uint32_t            out1cbcrimageheight:12;
	uint32_t            /* reserved */ : 4;
	uint32_t            out1cbcrimagewidthin64bitwords:10;
	uint32_t            /* reserved */ : 6;

	/* AXI Output 1 CbCr Configuration, Part 2 */
	uint8_t             out1cbcrburstlen:2;
	uint32_t            out1cbcrnumrows:12;
	uint32_t            out1cbcrrowincin64bitincs:12;
	uint32_t            /* reserved */ : 6;

	/* AXI Output 2 Y Configuration, Part 1 */
	uint32_t            out2yimageheight:12;
	uint32_t            /* reserved */ : 4;
	uint32_t            out2yimagewidthin64bitwords:10;
	uint32_t            /* reserved */ : 6;

	/* AXI Output 2 Y Configuration, Part 2 */
	uint8_t             out2yburstlen:2;
	uint32_t            out2ynumrows:12;
	uint32_t            out2yrowincin64bitincs:12;
	uint32_t            /* reserved */ : 6;

	/* AXI Output 2 CbCr Configuration, Part 1 */
	uint32_t            out2cbcrimageheight:12;
	uint32_t            /* reserved */ : 4;
	uint32_t            out2cbcrimagewidtein64bitwords:10;
	uint32_t            /* reserved */ : 6;

	/* AXI Output 2 CbCr Configuration, Part 2 */
	uint8_t             out2cbcrburstlen:2;
	uint32_t            out2cbcrnumrows:12;
	uint32_t            out2cbcrrowincin64bitincs:12;
	uint32_t            /* reserved */ : 6;

	/* Address configuration:
	 * output1 phisycal address */
	unsigned long   output1buffer1_y_phy;
	unsigned long   output1buffer1_cbcr_phy;
	unsigned long   output1buffer2_y_phy;
	unsigned long   output1buffer2_cbcr_phy;
	unsigned long   output1buffer3_y_phy;
	unsigned long   output1buffer3_cbcr_phy;
	unsigned long   output1buffer4_y_phy;
	unsigned long   output1buffer4_cbcr_phy;
	unsigned long   output1buffer5_y_phy;
	unsigned long   output1buffer5_cbcr_phy;
	unsigned long   output1buffer6_y_phy;
	unsigned long   output1buffer6_cbcr_phy;
	unsigned long   output1buffer7_y_phy;
	unsigned long   output1buffer7_cbcr_phy;
	unsigned long   output1buffer8_y_phy;
	unsigned long   output1buffer8_cbcr_phy;

	/* output2 phisycal address */
	unsigned long   output2buffer1_y_phy;
	unsigned long   output2buffer1_cbcr_phy;
	unsigned long   output2buffer2_y_phy;
	unsigned long   output2buffer2_cbcr_phy;
	unsigned long   output2buffer3_y_phy;
	unsigned long   output2buffer3_cbcr_phy;
	unsigned long   output2buffer4_y_phy;
	unsigned long   output2buffer4_cbcr_phy;
	unsigned long   output2buffer5_y_phy;
	unsigned long   output2buffer5_cbcr_phy;
	unsigned long   output2buffer6_y_phy;
	unsigned long   output2buffer6_cbcr_phy;
	unsigned long   output2buffer7_y_phy;
	unsigned long   output2buffer7_cbcr_phy;
	unsigned long   output2buffer8_y_phy;
	unsigned long   output2buffer8_cbcr_phy;
} __packed;

struct vfe_stats_we_cfg {
	uint32_t       header;

	/* White Balance/Exposure Statistic Selection */
	uint8_t        wb_expstatsenable:1;
	uint8_t        wb_expstatbuspriorityselection:1;
	unsigned int   wb_expstatbuspriorityvalue:4;
	unsigned int   /* reserved */ : 26;

	/* White Balance/Exposure Statistic Configuration, Part 1 */
	uint8_t        exposurestatregions:1;
	uint8_t        exposurestatsubregions:1;
	unsigned int   /* reserved */ : 14;

	unsigned int   whitebalanceminimumy:8;
	unsigned int   whitebalancemaximumy:8;

	/* White Balance/Exposure Statistic Configuration, Part 2 */
	uint8_t wb_expstatslopeofneutralregionline[
		NUM_WB_EXP_NEUTRAL_REGION_LINES];

	/* White Balance/Exposure Statistic Configuration, Part 3 */
	unsigned int   wb_expstatcrinterceptofneutralregionline2:12;
	unsigned int   /* reserved */ : 4;
	unsigned int   wb_expstatcbinterceptofneutralreginnline1:12;
	unsigned int    /* reserved */ : 4;

	/* White Balance/Exposure Statistic Configuration, Part 4 */
	unsigned int   wb_expstatcrinterceptofneutralregionline4:12;
	unsigned int   /* reserved */ : 4;
	unsigned int   wb_expstatcbinterceptofneutralregionline3:12;
	unsigned int   /* reserved */ : 4;

	/* White Balance/Exposure Statistic Output Buffer Header */
	unsigned int   wb_expmetricheaderpattern:8;
	unsigned int   /* reserved */ : 24;

	/* White Balance/Exposure Statistic Output Buffers-MUST
	* BE 64 bit ALIGNED */
	void  *wb_expstatoutputbuffer[NUM_WB_EXP_STAT_OUTPUT_BUFFERS];
} __packed;

struct vfe_stats_af_cfg {
	uint32_t header;

	/* Autofocus Statistic Selection */
	uint8_t       af_enable:1;
	uint8_t       af_busprioritysel:1;
	unsigned int  af_buspriorityval:4;
	unsigned int  /* reserved */ : 26;

	/* Autofocus Statistic Configuration, Part 1 */
	unsigned int  af_singlewinvoffset:12;
	unsigned int  /* reserved */ : 4;
	unsigned int  af_singlewinhoffset:12;
	unsigned int  /* reserved */ : 3;
	uint8_t       af_winmode:1;

	/* Autofocus Statistic Configuration, Part 2 */
	unsigned int  af_singglewinvh:11;
	unsigned int  /* reserved */ : 5;
	unsigned int  af_singlewinhw:11;
	unsigned int  /* reserved */ : 5;

	/* Autofocus Statistic Configuration, Parts 3-6 */
	uint8_t       af_multiwingrid[NUM_AUTOFOCUS_MULTI_WINDOW_GRIDS];

	/* Autofocus Statistic Configuration, Part 7 */
	signed int    af_metrichpfcoefa00:5;
	signed int    af_metrichpfcoefa04:5;
	unsigned int  af_metricmaxval:11;
	uint8_t       af_metricsel:1;
	unsigned int  /* reserved */ : 10;

	/* Autofocus Statistic Configuration, Part 8 */
	signed int    af_metrichpfcoefa20:5;
	signed int    af_metrichpfcoefa21:5;
	signed int    af_metrichpfcoefa22:5;
	signed int    af_metrichpfcoefa23:5;
	signed int    af_metrichpfcoefa24:5;
	unsigned int  /* reserved */ : 7;

	/* Autofocus Statistic Output Buffer Header */
	unsigned int  af_metrichp:8;
	unsigned int  /* reserved */ : 24;

	/* Autofocus Statistic Output Buffers - MUST BE 64 bit ALIGNED!!! */
	void *af_outbuf[NUM_AF_STAT_OUTPUT_BUFFERS];
} __packed; /* VFE_StatsAutofocusConfigCmdType */

struct msm_camera_frame_msg {
	unsigned long   output_y_address;
	unsigned long   output_cbcr_address;

	unsigned int    blacklevelevenColumn:23;
	uint16_t        reserved1:9;
	unsigned int    blackleveloddColumn:23;
	uint16_t        reserved2:9;

	uint16_t        greendefectpixelcount:8;
	uint16_t        reserved3:8;
	uint16_t        redbluedefectpixelcount:8;
	uint16_t        reserved4:8;
} __packed;

/* New one for 7k */
struct msm_vfe_command_7k {
	uint16_t queue;
	uint16_t length;
	void     *value;
};

struct stop_event {
	wait_queue_head_t wait;
	int state;
	int timeout;
};


#endif /* __MSM_VFE7X_H__ */
