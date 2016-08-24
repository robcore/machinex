/* Copyright (c) 2012, The Linux Foundation. All rights reserved.
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

#ifndef __VIDC_HAL_API_H__
#define __VIDC_HAL_API_H__

#include <linux/types.h>

#define CONTAINS(__a, __sz, __t) ({\
	int __rc = __t >= __a && \
			__t < __a + __sz; \
	__rc; \
})

#define OVERLAPS(__t, __tsz, __a, __asz) ({\
	int __rc = __t <= __a && \
			__t + __tsz >= __a + __asz; \
	__rc; \
})

#define HAL_BUFFERFLAG_EOS              0x00000001
#define HAL_BUFFERFLAG_STARTTIME        0x00000002
#define HAL_BUFFERFLAG_DECODEONLY       0x00000004
#define HAL_BUFFERFLAG_DATACORRUPT      0x00000008
#define HAL_BUFFERFLAG_ENDOFFRAME       0x00000010
#define HAL_BUFFERFLAG_SYNCFRAME        0x00000020
#define HAL_BUFFERFLAG_EXTRADATA        0x00000040
#define HAL_BUFFERFLAG_CODECCONFIG      0x00000080
#define HAL_BUFFERFLAG_TIMESTAMPINVALID 0x00000100
#define HAL_BUFFERFLAG_READONLY         0x00000200
#define HAL_BUFFERFLAG_ENDOFSUBFRAME    0x00000400

#define HAL_DEBUG_MSG_LOW				0x00000001
#define HAL_DEBUG_MSG_MEDIUM			0x00000002
#define HAL_DEBUG_MSG_HIGH				0x00000004
#define HAL_DEBUG_MSG_ERROR				0x00000008
#define HAL_DEBUG_MSG_FATAL				0x00000010

enum vidc_status {
	VIDC_ERR_NONE = 0x0,
	VIDC_ERR_FAIL = 0x80000000,
	VIDC_ERR_ALLOC_FAIL,
	VIDC_ERR_ILLEGAL_OP,
	VIDC_ERR_BAD_PARAM,
	VIDC_ERR_BAD_HANDLE,
	VIDC_ERR_NOT_SUPPORTED,
	VIDC_ERR_BAD_STATE,
	VIDC_ERR_MAX_CLIENT,
	VIDC_ERR_IFRAME_EXPECTED,
	VIDC_ERR_HW_FATAL,
	VIDC_ERR_BITSTREAM_ERR,
	VIDC_ERR_INDEX_NOMORE,
	VIDC_ERR_SEQHDR_PARSE_FAIL,
	VIDC_ERR_INSUFFICIENT_BUFFER,
	VIDC_ERR_BAD_POWER_STATE,
	VIDC_ERR_NO_VALID_SESSION,
	VIDC_ERR_TIMEOUT,
	VIDC_ERR_CMDQFULL,
	VIDC_ERR_CLIENT_PRESENT = 0x90000001,
	VIDC_ERR_CLIENT_FATAL,
	VIDC_ERR_CMD_QUEUE_FULL,
	VIDC_ERR_UNUSED = 0x10000000
};

enum hal_property {
	HAL_CONFIG_FRAME_RATE = 0x04000001,
	HAL_PARAM_UNCOMPRESSED_FORMAT_SELECT,
	HAL_PARAM_UNCOMPRESSED_PLANE_ACTUAL_CONSTRAINTS_INFO,
	HAL_PARAM_UNCOMPRESSED_PLANE_ACTUAL_INFO,
	HAL_PARAM_EXTRA_DATA_HEADER_CONFIG,
	HAL_PARAM_FRAME_SIZE,
	HAL_CONFIG_REALTIME,
	HAL_PARAM_BUFFER_COUNT_ACTUAL,
	HAL_PARAM_NAL_STREAM_FORMAT_SELECT,
	HAL_PARAM_VDEC_OUTPUT_ORDER,
	HAL_PARAM_VDEC_PICTURE_TYPE_DECODE,
	HAL_PARAM_VDEC_OUTPUT2_KEEP_ASPECT_RATIO,
	HAL_CONFIG_VDEC_POST_LOOP_DEBLOCKER,
	HAL_PARAM_VDEC_MULTI_STREAM,
	HAL_PARAM_VDEC_DISPLAY_PICTURE_BUFFER_COUNT,
	HAL_PARAM_DIVX_FORMAT,
	HAL_CONFIG_VDEC_MB_ERROR_MAP_REPORTING,
	HAL_PARAM_VDEC_CONTINUE_DATA_TRANSFER,
	HAL_CONFIG_VDEC_MB_ERROR_MAP,
	HAL_CONFIG_VENC_REQUEST_IFRAME,
	HAL_PARAM_VENC_MPEG4_SHORT_HEADER,
	HAL_PARAM_VENC_MPEG4_AC_PREDICTION,
	HAL_CONFIG_VENC_TARGET_BITRATE,
	HAL_PARAM_PROFILE_LEVEL_CURRENT,
	HAL_PARAM_VENC_H264_ENTROPY_CONTROL,
	HAL_PARAM_VENC_RATE_CONTROL,
	HAL_PARAM_VENC_MPEG4_TIME_RESOLUTION,
	HAL_PARAM_VENC_MPEG4_HEADER_EXTENSION,
	HAL_PARAM_VENC_H264_DEBLOCK_CONTROL,
	HAL_PARAM_VENC_TEMPORAL_SPATIAL_TRADEOFF,
	HAL_PARAM_VENC_SESSION_QP,
	HAL_CONFIG_VENC_INTRA_PERIOD,
	HAL_CONFIG_VENC_IDR_PERIOD,
	HAL_CONFIG_VPE_OPERATIONS,
	HAL_PARAM_VENC_INTRA_REFRESH,
	HAL_PARAM_VENC_MULTI_SLICE_CONTROL,
	HAL_CONFIG_VPE_DEINTERLACE,
	HAL_SYS_DEBUG_CONFIG,
	HAL_CONFIG_BUFFER_REQUIREMENTS,
	HAL_CONFIG_PRIORITY,
	HAL_CONFIG_BATCH_INFO,
	HAL_PARAM_METADATA_PASS_THROUGH,
	HAL_SYS_IDLE_INDICATOR,
	HAL_PARAM_UNCOMPRESSED_FORMAT_SUPPORTED,
	HAL_PARAM_INTERLACE_FORMAT_SUPPORTED,
	HAL_PARAM_CHROMA_SITE,
	HAL_PARAM_PROPERTIES_SUPPORTED,
	HAL_PARAM_PROFILE_LEVEL_SUPPORTED,
	HAL_PARAM_CAPABILITY_SUPPORTED,
	HAL_PARAM_NAL_STREAM_FORMAT_SUPPORTED,
	HAL_PARAM_MULTI_VIEW_FORMAT,
	HAL_PARAM_MAX_SEQUENCE_HEADER_SIZE,
	HAL_PARAM_CODEC_SUPPORTED,
	HAL_PARAM_VDEC_MULTI_VIEW_SELECT,
	HAL_PARAM_VDEC_MB_QUANTIZATION,
	HAL_PARAM_VDEC_NUM_CONCEALED_MB,
	HAL_PARAM_VDEC_H264_ENTROPY_SWITCHING,
	HAL_PARAM_VENC_SLICE_DELIVERY_MODE,
	HAL_PARAM_VENC_MPEG4_DATA_PARTITIONING,
	HAL_CONFIG_BUFFER_COUNT_ACTUAL,
	HAL_CONFIG_VDEC_MULTI_STREAM,
	HAL_PARAM_VENC_MULTI_SLICE_INFO,
	HAL_CONFIG_VENC_TIMESTAMP_SCALE,
	HAL_PARAM_VENC_LOW_LATENCY,
};

enum hal_domain {
	HAL_VIDEO_DOMAIN_VPE,
	HAL_VIDEO_DOMAIN_ENCODER,
	HAL_VIDEO_DOMAIN_DECODER,
	HAL_UNUSED_DOMAIN = 0x10000000,
};

enum hal_video_codec {
	HAL_VIDEO_CODEC_UNKNOWN  = 0x00000000,
	HAL_VIDEO_CODEC_MVC      = 0x00000001,
	HAL_VIDEO_CODEC_H264     = 0x00000002,
	HAL_VIDEO_CODEC_H263     = 0x00000004,
	HAL_VIDEO_CODEC_MPEG1    = 0x00000008,
	HAL_VIDEO_CODEC_MPEG2    = 0x00000010,
	HAL_VIDEO_CODEC_MPEG4    = 0x00000020,
	HAL_VIDEO_CODEC_DIVX_311 = 0x00000040,
	HAL_VIDEO_CODEC_DIVX     = 0x00000080,
	HAL_VIDEO_CODEC_VC1      = 0x00000100,
	HAL_VIDEO_CODEC_SPARK    = 0x00000200,
	HAL_VIDEO_CODEC_VP6      = 0x00000400,
	HAL_VIDEO_CODEC_VP7      = 0x00000800,
	HAL_VIDEO_CODEC_VP8      = 0x00001000,
	HAL_UNUSED_CODEC = 0x10000000,
};

enum hal_h263_profile {
	HAL_H263_PROFILE_BASELINE           = 0x00000001,
	HAL_H263_PROFILE_H320CODING         = 0x00000002,
	HAL_H263_PROFILE_BACKWARDCOMPATIBLE = 0x00000004,
	HAL_H263_PROFILE_ISWV2              = 0x00000008,
	HAL_H263_PROFILE_ISWV3              = 0x00000010,
	HAL_H263_PROFILE_HIGHCOMPRESSION    = 0x00000020,
	HAL_H263_PROFILE_INTERNET           = 0x00000040,
	HAL_H263_PROFILE_INTERLACE          = 0x00000080,
	HAL_H263_PROFILE_HIGHLATENCY        = 0x00000100,
	HAL_UNUSED_H263_PROFILE = 0x10000000,
};

enum hal_h263_level {
	HAL_H263_LEVEL_10 = 0x00000001,
	HAL_H263_LEVEL_20 = 0x00000002,
	HAL_H263_LEVEL_30 = 0x00000004,
	HAL_H263_LEVEL_40 = 0x00000008,
	HAL_H263_LEVEL_45 = 0x00000010,
	HAL_H263_LEVEL_50 = 0x00000020,
	HAL_H263_LEVEL_60 = 0x00000040,
	HAL_H263_LEVEL_70 = 0x00000080,
	HAL_UNUSED_H263_LEVEL = 0x10000000,
};

enum hal_mpeg2_profile {
	HAL_MPEG2_PROFILE_SIMPLE  = 0x00000001,
	HAL_MPEG2_PROFILE_MAIN    = 0x00000002,
	HAL_MPEG2_PROFILE_422     = 0x00000004,
	HAL_MPEG2_PROFILE_SNR     = 0x00000008,
	HAL_MPEG2_PROFILE_SPATIAL = 0x00000010,
	HAL_MPEG2_PROFILE_HIGH    = 0x00000020,
	HAL_UNUSED_MPEG2_PROFILE = 0x10000000,
};

enum hal_mpeg2_level {
	HAL_MPEG2_LEVEL_LL  = 0x00000001,
	HAL_MPEG2_LEVEL_ML  = 0x00000002,
	HAL_MPEG2_LEVEL_H14 = 0x00000004,
	HAL_MPEG2_LEVEL_HL  = 0x00000008,
	HAL_UNUSED_MEPG2_LEVEL = 0x10000000,
};

enum hal_mpeg4_profile {
	HAL_MPEG4_PROFILE_SIMPLE           = 0x00000001,
	HAL_MPEG4_PROFILE_SIMPLESCALABLE   = 0x00000002,
	HAL_MPEG4_PROFILE_CORE             = 0x00000004,
	HAL_MPEG4_PROFILE_MAIN             = 0x00000008,
	HAL_MPEG4_PROFILE_NBIT             = 0x00000010,
	HAL_MPEG4_PROFILE_SCALABLETEXTURE  = 0x00000020,
	HAL_MPEG4_PROFILE_SIMPLEFACE       = 0x00000040,
	HAL_MPEG4_PROFILE_SIMPLEFBA        = 0x00000080,
	HAL_MPEG4_PROFILE_BASICANIMATED    = 0x00000100,
	HAL_MPEG4_PROFILE_HYBRID           = 0x00000200,
	HAL_MPEG4_PROFILE_ADVANCEDREALTIME = 0x00000400,
	HAL_MPEG4_PROFILE_CORESCALABLE     = 0x00000800,
	HAL_MPEG4_PROFILE_ADVANCEDCODING   = 0x00001000,
	HAL_MPEG4_PROFILE_ADVANCEDCORE     = 0x00002000,
	HAL_MPEG4_PROFILE_ADVANCEDSCALABLE = 0x00004000,
	HAL_MPEG4_PROFILE_ADVANCEDSIMPLE   = 0x00008000,
	HAL_UNUSED_MPEG4_PROFILE = 0x10000000,
};

enum hal_mpeg4_level {
	HAL_MPEG4_LEVEL_0  = 0x00000001,
	HAL_MPEG4_LEVEL_0b = 0x00000002,
	HAL_MPEG4_LEVEL_1  = 0x00000004,
	HAL_MPEG4_LEVEL_2  = 0x00000008,
	HAL_MPEG4_LEVEL_3  = 0x00000010,
	HAL_MPEG4_LEVEL_4  = 0x00000020,
	HAL_MPEG4_LEVEL_4a = 0x00000040,
	HAL_MPEG4_LEVEL_5  = 0x00000080,
	HAL_MPEG4_LEVEL_VENDOR_START_UNUSED = 0x7F000000,
	HAL_MPEG4_LEVEL_6  = 0x7F000001,
	HAL_MPEG4_LEVEL_7  = 0x7F000002,
	HAL_MPEG4_LEVEL_8  = 0x7F000003,
	HAL_MPEG4_LEVEL_9  = 0x7F000004,
	HAL_MPEG4_LEVEL_3b = 0x7F000005,
	HAL_UNUSED_MPEG4_LEVEL = 0x10000000,
};

enum hal_h264_profile {
	HAL_H264_PROFILE_BASELINE = 0x00000001,
	HAL_H264_PROFILE_MAIN     = 0x00000002,
	HAL_H264_PROFILE_HIGH     = 0x00000004,
	HAL_H264_PROFILE_EXTENDED = 0x00000008,
	HAL_H264_PROFILE_HIGH10   = 0x00000010,
	HAL_H264_PROFILE_HIGH422  = 0x00000020,
	HAL_H264_PROFILE_HIGH444  = 0x00000040,
	HAL_H264_PROFILE_CONSTRAINED_HIGH  = 0x00000080,
	HAL_UNUSED_H264_PROFILE = 0x10000000,
};

enum hal_h264_level {
	HAL_H264_LEVEL_1  = 0x00000001,
	HAL_H264_LEVEL_1b = 0x00000002,
	HAL_H264_LEVEL_11 = 0x00000004,
	HAL_H264_LEVEL_12 = 0x00000008,
	HAL_H264_LEVEL_13 = 0x00000010,
	HAL_H264_LEVEL_2  = 0x00000020,
	HAL_H264_LEVEL_21 = 0x00000040,
	HAL_H264_LEVEL_22 = 0x00000080,
	HAL_H264_LEVEL_3  = 0x00000100,
	HAL_H264_LEVEL_31 = 0x00000200,
	HAL_H264_LEVEL_32 = 0x00000400,
	HAL_H264_LEVEL_4  = 0x00000800,
	HAL_H264_LEVEL_41 = 0x00001000,
	HAL_H264_LEVEL_42 = 0x00002000,
	HAL_H264_LEVEL_5  = 0x00004000,
	HAL_H264_LEVEL_51 = 0x00008000,
	HAL_UNUSED_H264_LEVEL = 0x10000000,
};

enum hal_vpx_profile {
	HAL_VPX_PROFILE_SIMPLE    = 0x00000001,
	HAL_VPX_PROFILE_ADVANCED  = 0x00000002,
	HAL_VPX_PROFILE_VERSION_0 = 0x00000004,
	HAL_VPX_PROFILE_VERSION_1 = 0x00000008,
	HAL_VPX_PROFILE_VERSION_2 = 0x00000010,
	HAL_VPX_PROFILE_VERSION_3 = 0x00000020,
	HAL_VPX_PROFILE_UNUSED = 0x10000000,
};

enum hal_vc1_profile {
	HAL_VC1_PROFILE_SIMPLE   = 0x00000001,
	HAL_VC1_PROFILE_MAIN     = 0x00000002,
	HAL_VC1_PROFILE_ADVANCED = 0x00000004,
	HAL_UNUSED_VC1_PROFILE = 0x10000000,
};

enum hal_vc1_level {
	HAL_VC1_LEVEL_LOW    = 0x00000001,
	HAL_VC1_LEVEL_MEDIUM = 0x00000002,
	HAL_VC1_LEVEL_HIGH   = 0x00000004,
	HAL_VC1_LEVEL_0      = 0x00000008,
	HAL_VC1_LEVEL_1      = 0x00000010,
	HAL_VC1_LEVEL_2      = 0x00000020,
	HAL_VC1_LEVEL_3      = 0x00000040,
	HAL_VC1_LEVEL_4      = 0x00000080,
	HAL_UNUSED_VC1_LEVEL = 0x10000000,
};

enum hal_divx_format {
	HAL_DIVX_FORMAT_4,
	HAL_DIVX_FORMAT_5,
	HAL_DIVX_FORMAT_6,
	HAL_UNUSED_DIVX_FORMAT = 0x10000000,
};

enum hal_divx_profile {
	HAL_DIVX_PROFILE_QMOBILE  = 0x00000001,
	HAL_DIVX_PROFILE_MOBILE   = 0x00000002,
	HAL_DIVX_PROFILE_MT       = 0x00000004,
	HAL_DIVX_PROFILE_HT       = 0x00000008,
	HAL_DIVX_PROFILE_HD       = 0x00000010,
	HAL_UNUSED_DIVX_PROFILE = 0x10000000,
};

enum hal_mvc_profile {
	HAL_MVC_PROFILE_STEREO_HIGH  = 0x00000001,
	HAL_MVC_PROFILE_MV_HIGH      = 0x00000002,
	HAL_UNUSED_MVC_PROFILE = 0x10000000,
};

enum hal_mvc_level {
	HAL_MVC_LEVEL_1  = 0x00000001,
	HAL_MVC_LEVEL_1b = 0x00000002,
	HAL_MVC_LEVEL_11 = 0x00000004,
	HAL_MVC_LEVEL_12 = 0x00000008,
	HAL_MVC_LEVEL_13 = 0x00000010,
	HAL_MVC_LEVEL_2  = 0x00000020,
	HAL_MVC_LEVEL_21 = 0x00000040,
	HAL_MVC_LEVEL_22 = 0x00000080,
	HAL_MVC_LEVEL_3  = 0x00000100,
	HAL_MVC_LEVEL_31 = 0x00000200,
	HAL_MVC_LEVEL_32 = 0x00000400,
	HAL_MVC_LEVEL_4  = 0x00000800,
	HAL_MVC_LEVEL_41 = 0x00001000,
	HAL_MVC_LEVEL_42 = 0x00002000,
	HAL_MVC_LEVEL_5  = 0x00004000,
	HAL_MVC_LEVEL_51 = 0x00008000,
	HAL_UNUSED_MVC_LEVEL = 0x10000000,
};

enum hal_buffer {
	HAL_BUFFER_INPUT,
	HAL_BUFFER_OUTPUT,
	HAL_BUFFER_OUTPUT2,
	HAL_BUFFER_EXTRADATA_INPUT,
	HAL_BUFFER_EXTRADATA_OUTPUT,
	HAL_BUFFER_EXTRADATA_OUTPUT2,
	HAL_BUFFER_INTERNAL_SCRATCH,
	HAL_BUFFER_INTERNAL_PERSIST,
	HAL_UNUSED_BUFFER = 0x10000000,
};

struct hal_frame_rate {
	enum hal_buffer buffer_type;
	u32 frame_rate;
};

enum hal_uncompressed_format {
	HAL_COLOR_FORMAT_MONOCHROME,
	HAL_COLOR_FORMAT_NV12,
	HAL_COLOR_FORMAT_NV21,
	HAL_COLOR_FORMAT_NV12_4x4TILE,
	HAL_COLOR_FORMAT_NV21_4x4TILE,
	HAL_COLOR_FORMAT_YUYV,
	HAL_COLOR_FORMAT_YVYU,
	HAL_COLOR_FORMAT_UYVY,
	HAL_COLOR_FORMAT_VYUY,
	HAL_COLOR_FORMAT_RGB565,
	HAL_COLOR_FORMAT_BGR565,
	HAL_COLOR_FORMAT_RGB888,
	HAL_COLOR_FORMAT_BGR888,
	HAL_UNUSED_COLOR = 0x10000000,
};

struct hal_uncompressed_format_select {
	enum hal_buffer buffer_type;
	enum hal_uncompressed_format format;
};

struct hal_uncompressed_plane_actual {
	int actual_stride;
	u32 actual_plane_buffer_height;
};

struct hal_uncompressed_plane_actual_info {
	enum hal_buffer buffer_type;
	u32 num_planes;
	struct hal_uncompressed_plane_actual rg_plane_format[1];
};

struct hal_uncompressed_plane_constraints {
	u32 stride_multiples;
	u32 max_stride;
	u32 min_plane_buffer_height_multiple;
	u32 buffer_alignment;
};

struct hal_uncompressed_plane_actual_constraints_info {
	enum hal_buffer buffer_type;
	u32 num_planes;
	struct hal_uncompressed_plane_constraints rg_plane_format[1];
};

struct hal_extra_data_header_config {
	u32 type;
	enum hal_buffer buffer_type;
	u32 version;
	u32 port_index;
	u32 client_extradata_id;
};

struct hal_frame_size {
	enum hal_buffer buffer_type;
	u32 width;
	u32 height;
};

struct hal_enable {
	u32 enable;
};

struct hal_buffer_count_actual {
	enum hal_buffer buffer_type;
	u32 buffer_count_actual;
};

enum hal_nal_stream_format {
	HAL_NAL_FORMAT_STARTCODES         = 0x00000001,
	HAL_NAL_FORMAT_ONE_NAL_PER_BUFFER = 0x00000002,
	HAL_NAL_FORMAT_ONE_BYTE_LENGTH    = 0x00000004,
	HAL_NAL_FORMAT_TWO_BYTE_LENGTH    = 0x00000008,
	HAL_NAL_FORMAT_FOUR_BYTE_LENGTH   = 0x00000010,
};

enum hal_output_order {
	HAL_OUTPUT_ORDER_DISPLAY,
	HAL_OUTPUT_ORDER_DECODE,
	HAL_UNUSED_OUTPUT = 0x10000000,
};

enum hal_picture {
	HAL_PICTURE_I = 0x01,
	HAL_PICTURE_P = 0x02,
	HAL_PICTURE_B = 0x04,
	HAL_PICTURE_IDR = 0x7F001000,
	HAL_FRAME_NOTCODED = 0x7F002000,
	HAL_FRAME_YUV = 0x7F004000,
	HAL_UNUSED_PICT = 0x10000000,
};

struct hal_enable_picture {
	u32 picture_type;
};

struct hal_multi_stream {
	enum hal_buffer buffer_type;
	u32 enable;
	u32 width;
	u32 height;
};

struct hal_display_picture_buffer_count {
	u32 enable;
	u32 count;
};

struct hal_mb_error_map {
	u32 error_map_size;
	u8 rg_error_map[1];
};

struct hal_request_iframe {
	u32 enable;
};

struct hal_bitrate {
	u32 bit_rate;
};

struct hal_profile_level {
	u32 profile;
	u32 level;
};
/*
struct hal_profile_level_range {
	u32 profile;
	u32 min_level;
	u32 max_level;
}

struct hal_profile_level_supported {
	u32 profile_count;
	struct hal_profile_level_range profile_level[1];
};
*/
enum hal_h264_entropy {
	HAL_H264_ENTROPY_CAVLC,
	HAL_H264_ENTROPY_CABAC,
	HAL_UNUSED_ENTROPY = 0x10000000,
};

enum hal_h264_cabac_model {
	HAL_H264_CABAC_MODEL_0,
	HAL_H264_CABAC_MODEL_1,
	HAL_H264_CABAC_MODEL_2,
	HAL_UNUSED_CABAC = 0x10000000,
};

struct hal_h264_entropy_control {
	enum hal_h264_entropy entropy_mode;
	enum hal_h264_cabac_model cabac_model;
};

enum hal_rate_control {
	HAL_RATE_CONTROL_OFF,
	HAL_RATE_CONTROL_VBR_VFR,
	HAL_RATE_CONTROL_VBR_CFR,
	HAL_RATE_CONTROL_CBR_VFR,
	HAL_RATE_CONTROL_CBR_CFR,
	HAL_UNUSED_RC = 0x10000000,
};

struct hal_mpeg4_time_resolution {
	u32 time_increment_resolution;
};

struct hal_mpeg4_header_extension {
	u32 header_extension;
};

enum hal_h264_db_mode {
	HAL_H264_DB_MODE_DISABLE,
	HAL_H264_DB_MODE_SKIP_SLICE_BOUNDARY,
	HAL_H264_DB_MODE_ALL_BOUNDARY,
	HAL_UNUSED_H264_DB = 0x10000000,
};

struct hal_h264_db_control {
	enum hal_h264_db_mode mode;
	int slice_alpha_offset;
	int slice_beta_offset;
};

struct hal_temporal_spatial_tradeoff {
	u32 ts_factor;
};

struct hal_quantization {
	u32 qpi;
	u32 qpp;
	u32 qpb;
};

struct hal_intra_period {
	u32 pframes;
	u32 bframes;
};

struct hal_idr_period {
	u32 idr_period;
};

enum hal_rotate {
	HAL_ROTATE_NONE,
	HAL_ROTATE_90,
	HAL_ROTATE_180,
	HAL_ROTATE_270,
	HAL_UNUSED_ROTATE = 0x10000000,
};

enum hal_flip {
	HAL_FLIP_NONE,
	HAL_FLIP_HORIZONTAL,
	HAL_FLIP_VERTICAL,
	HAL_UNUSED_FLIP = 0x10000000,
};

struct hal_operations {
	enum hal_rotate rotate;
	enum hal_flip flip;
};

enum hal_intra_refresh_mode {
	HAL_INTRA_REFRESH_NONE,
	HAL_INTRA_REFRESH_CYCLIC,
	HAL_INTRA_REFRESH_ADAPTIVE,
	HAL_INTRA_REFRESH_CYCLIC_ADAPTIVE,
	HAL_INTRA_REFRESH_RANDOM,
	HAL_UNUSED_INTRA = 0x10000000,
};

struct hal_intra_refresh {
	enum hal_intra_refresh_mode mode;
	u32 air_mbs;
	u32 air_ref;
	u32 cir_mbs;
};

enum hal_multi_slice {
	HAL_MULTI_SLICE_OFF,
	HAL_MULTI_SLICE_BY_MB_COUNT,
	HAL_MULTI_SLICE_BY_BYTE_COUNT,
	HAL_MULTI_SLICE_GOB,
	HAL_UNUSED_SLICE = 0x10000000,
};

struct hal_multi_slice_control {
	enum hal_multi_slice multi_slice;
	u32 slice_size;
};

struct hal_debug_config {
	u32 debug_config;
};

struct hal_buffer_requirements {
	enum hal_buffer buffer_type;
	u32 buffer_size;
	u32 buffer_region_size;
	u32 buffer_hold_count;
	u32 buffer_count_min;
	u32 buffer_count_actual;
	u32 contiguous;
	u32 buffer_alignment;
};

enum hal_priority {/* Priority increases with number */
	HAL_PRIORITY_LOW = 10,
	HAL_PRIOIRTY_MEDIUM = 20,
	HAL_PRIORITY_HIGH = 30,
	HAL_UNUSED_PRIORITY = 0x10000000,
};

struct hal_batch_info {
	u32 input_batch_count;
	u32 output_batch_count;
};

struct hal_metadata_pass_through {
	u32 enable;
	u32 size;
};

struct hal_uncompressed_format_supported {
	enum hal_buffer buffer_type;
	u32 format_entries;
	u32 rg_format_info[1];
};

enum hal_interlace_format {
	HAL_INTERLACE_FRAME_PROGRESSIVE                 = 0x01,
	HAL_INTERLACE_INTERLEAVE_FRAME_TOPFIELDFIRST    = 0x02,
	HAL_INTERLACE_INTERLEAVE_FRAME_BOTTOMFIELDFIRST = 0x04,
	HAL_INTERLACE_FRAME_TOPFIELDFIRST               = 0x08,
	HAL_INTERLACE_FRAME_BOTTOMFIELDFIRST            = 0x10,
	HAL_UNUSED_INTERLACE = 0x10000000,
};

struct hal_interlace_format_supported {
	enum hal_buffer buffer_type;
	enum hal_interlace_format format;
};

enum hal_chroma_site {
	HAL_CHROMA_SITE_0,
	HAL_CHROMA_SITE_1,
	HAL_UNUSED_CHROMA = 0x10000000,
};

struct hal_properties_supported {
	u32 num_properties;
	u32 rg_properties[1];
};

enum hal_capability {
	HAL_CAPABILITY_FRAME_WIDTH,
	HAL_CAPABILITY_FRAME_HEIGHT,
	HAL_CAPABILITY_MBS_PER_FRAME,
	HAL_CAPABILITY_MBS_PER_SECOND,
	HAL_CAPABILITY_FRAMERATE,
	HAL_CAPABILITY_SCALE_X,
	HAL_CAPABILITY_SCALE_Y,
	HAL_CAPABILITY_BITRATE,
	HAL_UNUSED_CAPABILITY = 0x10000000,
};

struct hal_capability_supported {
	enum hal_capability capability_type;
	u32 min;
	u32 max;
	u32 step_size;
};

struct hal_capability_supported_info {
	u32 num_capabilities;
	struct hal_capability_supported rg_data[1];
};

struct hal_nal_stream_format_supported {
	u32 nal_stream_format_supported;
};

struct hal_multi_view_format {
	u32 views;
	u32 rg_view_order[1];
};

struct hal_seq_header_info {
	u32 nax_header_len;
};

struct hal_codec_supported {
	u32 decoder_codec_supported;
	u32 encoder_codec_supported;
};

struct hal_multi_view_select {
	u32 view_index;
};

struct hal_timestamp_scale {
	u32 time_stamp_scale;
};

enum vidc_resource_id {
	VIDC_RESOURCE_OCMEM = 0x00000001,
	VIDC_UNUSED_RESORUCE = 0x10000000,
};

struct vidc_resource_hdr {
	enum vidc_resource_id resource_id;
	u32 resource_handle;
	u32 size;
};

struct vidc_buffer_addr_info {
	enum hal_buffer buffer_type;
	u32 buffer_size;
	u32 num_buffers;
	u32 align_device_addr;
	u32 extradata_size;
	u32 extradata_addr;
};

struct vidc_frame_plane_config {
	u32 left;
	u32 top;
	u32 width;
	u32 height;
	u32 stride;
	u32 scan_lines;
};

struct vidc_uncompressed_frame_config {
	struct vidc_frame_plane_config luma_plane;
	struct vidc_frame_plane_config chroma_plane;
};

struct vidc_frame_data {
	enum hal_buffer buffer_type;
	u32 device_addr;
	u32 extradata_addr;
	int64_t timestamp;
	u32 flags;
	u32 offset;
	u32 alloc_len;
	u32 filled_len;
	u32 mark_target;
	u32 mark_data;
	u32 clnt_data;
};

struct vidc_seq_hdr {
	u8 *seq_hdr;
	u32 seq_hdr_len;
};

enum hal_flush {
	HAL_FLUSH_INPUT,
	HAL_FLUSH_OUTPUT,
	HAL_FLUSH_OUTPUT2,
	HAL_FLUSH_ALL,
	HAL_UNUSED_FLUSH = 0x10000000,
};

enum hal_event_type {
	HAL_EVENT_SEQ_CHANGED_SUFFICIENT_RESOURCES,
	HAL_EVENT_SEQ_CHANGED_INSUFFICIENT_RESOURCES,
	HAL_UNUSED_SEQCHG = 0x10000000,
};

/* HAL Response */

enum command_response {
/* SYSTEM COMMANDS_DONE*/
	VIDC_EVENT_CHANGE,
	SYS_INIT_DONE,
	SET_RESOURCE_DONE,
	RELEASE_RESOURCE_DONE,
	PING_ACK_DONE,
	PC_PREP_DONE,
	SYS_IDLE,
	SYS_DEBUG,
/* SESSION COMMANDS_DONE */
	SESSION_LOAD_RESOURCE_DONE,
	SESSION_INIT_DONE,
	SESSION_END_DONE,
	SESSION_ABORT_DONE,
	SESSION_START_DONE,
	SESSION_STOP_DONE,
	SESSION_ETB_DONE,
	SESSION_FTB_DONE,
	SESSION_FLUSH_DONE,
	SESSION_SUSPEND_DONE,
	SESSION_RESUME_DONE,
	SESSION_SET_PROP_DONE,
	SESSION_GET_PROP_DONE,
	SESSION_PARSE_SEQ_HDR_DONE,
	SESSION_GET_SEQ_HDR_DONE,
	SESSION_RELEASE_BUFFER_DONE,
	SESSION_RELEASE_RESOURCE_DONE,
	SESSION_PROPERTY_INFO,
	RESPONSE_UNUSED = 0x10000000,
};

/* Command Callback structure */

struct msm_vidc_cb_cmd_done {
	u32 device_id;
	u32 session_id;
	u32 status;
	u32 size;
	void *data;
};

struct msm_vidc_cb_event {
	u32 device_id;
	u32 session_id;
	u32 status;
	u32 height;
	u32 width;
	u32 hal_event_type;
};

/* Data callback structure */

struct vidc_hal_ebd {
	u32 timestamp_hi;
	u32 timestamp_lo;
	u32 flags;
	u32 mark_target;
	u32 mark_data;
	u32 stats;
	u32 offset;
	u32 alloc_len;
	u32 filled_len;
	enum hal_picture picture_type;
	u8 *packet_buffer;
	u8 *extra_data_buffer;
};

struct vidc_hal_fbd {
	u32 stream_id;
	u32 view_id;
	u32 timestamp_hi;
	u32 timestamp_lo;
	u32 flags1;
	u32 mark_target;
	u32 mark_data;
	u32 stats;
	u32 alloc_len1;
	u32 filled_len1;
	u32 offset1;
	u32 frame_width;
	u32 frame_height;
	u32 start_xCoord;
	u32 start_yCoord;
	u32 input_tag;
	u32 input_tag1;
	enum hal_picture picture_type;
	u8 *packet_buffer1;
	u8 *extra_data_buffer;
	u32 flags2;
	u32 alloc_len2;
	u32 filled_len2;
	u32 offset2;
	u8 *packet_buffer2;
	u32 flags3;
	u32 alloc_len3;
	u32 filled_len3;
	u32 offset3;
	u8 *packet_buffer3;
	enum hal_buffer buffer_type;
};

struct msm_vidc_cb_data_done {
	u32 device_id;
	u32 session_id;
	u32 status;
	u32 size;
	void *clnt_data;
	union {
		struct vidc_hal_ebd input_done;
		struct vidc_hal_fbd output_done;
	};
};

struct vidc_hal_sys_init_done {
	u32 enc_codec_supported;
	u32 dec_codec_supported;
};

struct vidc_hal_session_init_done {
	struct hal_capability_supported width;
	struct hal_capability_supported height;
	struct hal_capability_supported mbs_per_frame;
	struct hal_capability_supported mbs_per_sec;
	struct hal_capability_supported frame_rate;
	struct hal_capability_supported scale_x;
	struct hal_capability_supported scale_y;
	struct hal_capability_supported bitrate;
	struct hal_uncompressed_format_supported uncomp_format;
	struct hal_interlace_format_supported HAL_format;
	struct hal_nal_stream_format_supported nal_stream_format;
/*	struct hal_profile_level_supported profile_level;
	// allocate and released memory for above. */
	struct hal_intra_refresh intra_refresh;
	struct hal_seq_header_info seq_hdr_info;
};

struct buffer_requirements {
	struct hal_buffer_requirements buffer[8];
};

/* VIDC_HAL CORE API's */
int vidc_hal_core_init(void *device, int domain);
int vidc_hal_core_release(void *device);
int vidc_hal_core_pc_prep(void *device);
int vidc_hal_core_set_resource(void *device,
	struct vidc_resource_hdr *resource_hdr, void *resource_value);
int vidc_hal_core_release_resource(void *device,
	struct vidc_resource_hdr *resource_hdr);
int vidc_hal_core_ping(void *device);

/* VIDC_HAL SESSION API's */
void *vidc_hal_session_init(void *device, u32 session_id,
	enum hal_domain session_type, enum hal_video_codec codec_type);
int vidc_hal_session_end(void *session);
int vidc_hal_session_abort(void *session);
int vidc_hal_session_set_buffers(void *sess,
	struct vidc_buffer_addr_info *buffer_info);
int vidc_hal_session_release_buffers(void *sess,
	struct vidc_buffer_addr_info *buffer_info);
int vidc_hal_session_load_res(void *sess);
int vidc_hal_session_release_res(void *sess);
int vidc_hal_session_start(void *sess);
int vidc_hal_session_stop(void *sess);
int vidc_hal_session_suspend(void *sess);
int vidc_hal_session_resume(void *sess);
int vidc_hal_session_etb(void *sess,
	struct vidc_frame_data *input_frame);
int vidc_hal_session_ftb(void *sess,
	struct vidc_frame_data *output_frame);
int vidc_hal_session_parse_seq_hdr(void *sess,
	struct vidc_seq_hdr *seq_hdr);
int vidc_hal_session_get_seq_hdr(void *sess,
	struct vidc_seq_hdr *seq_hdr);
int vidc_hal_session_get_buf_req(void *sess);
int vidc_hal_session_flush(void *sess, enum hal_flush flush_mode);
int vidc_hal_session_set_property(void *sess, enum hal_property ptype,
								  void *pdata);
int vidc_hal_session_get_property(void *sess, enum hal_property ptype,
								  void *pdata);
void *vidc_hal_add_device(u32 device_id, u32 base_addr,
	u32 reg_base, u32 reg_size, u32 irq,
	void (*callback) (enum command_response cmd, void *data));
void vidc_hal_delete_device(void *device);

#endif /*__VIDC_HAL_API_H__ */
