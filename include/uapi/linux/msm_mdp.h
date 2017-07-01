#ifndef _UAPI_MSM_MDP_H_
#define _UAPI_MSM_MDP_H_

#include <linux/types.h>
#include <linux/fb.h>

struct mdp_rect {
	uint32_t x;
	uint32_t y;
	uint32_t w;
	uint32_t h;
};

struct mdp_img {
	uint32_t width;
	uint32_t height;
	uint32_t format;
	uint32_t offset;
	int memory_id;		/* the file descriptor */
	uint32_t priv;
};

/*
 * {3x3} + {3} ccs matrix
 */

struct mdp_ccs {
	int direction;			/* MDP_CCS_RGB2YUV or YUV2RGB */
	uint16_t ccs[MDP_CCS_SIZE];	/* 3x3 color coefficients */
	uint16_t bv[MDP_BV_SIZE];	/* 1x3 bias vector */
};

struct mdp_csc {
	int id;
	uint32_t csc_mv[9];
	uint32_t csc_pre_bv[3];
	uint32_t csc_post_bv[3];
	uint32_t csc_pre_lv[6];
	uint32_t csc_post_lv[6];
};

/* The version of the mdp_blit_req structure so that
 * user applications can selectively decide which functionality
 * to include
 */



struct mdp_blit_req {
	struct mdp_img src;
	struct mdp_img dst;
	struct mdp_rect src_rect;
	struct mdp_rect dst_rect;
	uint32_t alpha;
	uint32_t transp_mask;
	uint32_t flags;
	int sharpening_strength;  /* -127 <--> 127, default 64 */
};

struct mdp_blit_req_list {
	uint32_t count;
	struct mdp_blit_req req[];
};

struct msmfb_data {
	uint32_t offset;
	int memory_id;
	int id;
	uint32_t flags;
	uint32_t priv;
	uint32_t iova;
};

struct msmfb_overlay_data {
	uint32_t id;
	struct msmfb_data data;
	uint32_t version_key;
	struct msmfb_data plane1_data;
	struct msmfb_data plane2_data;
	struct msmfb_data dst_data;
};

struct msmfb_img {
	uint32_t width;
	uint32_t height;
	uint32_t format;
};

struct msmfb_writeback_data {
	struct msmfb_data buf_info;
	struct msmfb_img img;
};

struct mdp_qseed_cfg {
	uint32_t table_num;
	uint32_t ops;
	uint32_t len;
	uint32_t *data;
};

struct mdp_qseed_cfg_data {
	uint32_t block;
	struct mdp_qseed_cfg qseed_data;
};

struct mdp_csc_cfg {
	/* flags for enable CSC, toggling RGB,YUV input/output */
	uint32_t flags;
	uint32_t csc_mv[9];
	uint32_t csc_pre_bv[3];
	uint32_t csc_post_bv[3];
	uint32_t csc_pre_lv[6];
	uint32_t csc_post_lv[6];
};

struct mdp_csc_cfg_data {
	uint32_t block;
	struct mdp_csc_cfg csc_data;
};

struct mdp_overlay_pp_params {
	uint32_t config_ops;
	struct mdp_csc_cfg csc_cfg;
	struct mdp_qseed_cfg qseed_cfg[2];
};

enum {                    
	BLEND_OP_NOT_DEFINED = 0,
	BLEND_OP_OPAQUE,         
	BLEND_OP_PREMULTIPLIED,  
	BLEND_OP_COVERAGE,       
	BLEND_OP_MAX,            
};                        

struct mdp_overlay {
	struct msmfb_img src;
	struct mdp_rect src_rect;
	struct mdp_rect dst_rect;
	uint32_t z_order;	/* stage number */
	uint32_t is_fg;		/* control alpha & transp */
	uint32_t alpha;
	uint32_t transp_mask;
	uint32_t blend_op;
	uint32_t flags;
	uint32_t id;
	uint32_t user_data[8];
	struct mdp_overlay_pp_params overlay_pp_cfg;
};

struct msmfb_overlay_3d {
	uint32_t is_3d;
	uint32_t width;
	uint32_t height;
};


struct msmfb_overlay_blt {
	uint32_t enable;
	uint32_t offset;
	uint32_t width;
	uint32_t height;
	uint32_t bpp;
};

struct mdp_histogram {
	uint32_t frame_cnt;
	uint32_t bin_cnt;
	uint32_t *r;
	uint32_t *g;
	uint32_t *b;
};


/*

	mdp_block_type defines the identifiers for pipes in MDP 4.3 and up

	MDP_BLOCK_RESERVED is provided for backward compatibility and is
	deprecated. It corresponds to DMA_P. So MDP_BLOCK_DMA_P should be used
	instead.

	MDP_LOGICAL_BLOCK_DISP_0 identifies the display pipe which fb0 uses,
	same for others.

*/

enum {
	MDP_BLOCK_RESERVED = 0,
	MDP_BLOCK_OVERLAY_0,
	MDP_BLOCK_OVERLAY_1,
	MDP_BLOCK_VG_1,
	MDP_BLOCK_VG_2,
	MDP_BLOCK_RGB_1,
	MDP_BLOCK_RGB_2,
	MDP_BLOCK_DMA_P,
	MDP_BLOCK_DMA_S,
	MDP_BLOCK_DMA_E,
	MDP_BLOCK_OVERLAY_2,
	MDP_LOGICAL_BLOCK_DISP_0 = 0x1000,
	MDP_LOGICAL_BLOCK_DISP_1,
	MDP_LOGICAL_BLOCK_DISP_2,
	MDP_BLOCK_MAX,
};

/*
 * mdp_histogram_start_req is used to provide the parameters for
 * histogram start request
 */

struct mdp_histogram_start_req {
	uint32_t block;
	uint8_t frame_cnt;
	uint8_t bit_mask;
	uint8_t num_bins;
};

/*
 * mdp_histogram_data is used to return the histogram data, once
 * the histogram is done/stopped/cance
 */

struct mdp_histogram_data {
	uint32_t block;
	uint8_t bin_cnt;
	uint32_t *c0;
	uint32_t *c1;
	uint32_t *c2;
	uint32_t *extra_info;
};

struct mdp_pcc_coeff {
	uint32_t c, r, g, b, rr, gg, bb, rg, gb, rb, rgb_0, rgb_1;
};

struct mdp_pcc_cfg_data {
	uint32_t block;
	uint32_t ops;
	struct mdp_pcc_coeff r, g, b;
};

enum {
	mdp_lut_igc,
	mdp_lut_pgc,
	mdp_lut_hist,
	mdp_lut_max,
};

struct mdp_igc_lut_data {
	uint32_t block;
	uint32_t len, ops;
	uint32_t *c0_c1_data;
	uint32_t *c2_data;
};

struct mdp_ar_gc_lut_data {
	uint32_t x_start;
	uint32_t slope;
	uint32_t offset;
};

struct mdp_pgc_lut_data {
	uint32_t block;
	uint32_t flags;
	uint8_t num_r_stages;
	uint8_t num_g_stages;
	uint8_t num_b_stages;
	struct mdp_ar_gc_lut_data *r_data;
	struct mdp_ar_gc_lut_data *g_data;
	struct mdp_ar_gc_lut_data *b_data;
};


struct mdp_hist_lut_data {
	uint32_t block;
	uint32_t ops;
	uint32_t len;
	uint32_t *data;
};

struct mdp_lut_cfg_data {
	uint32_t lut_type;
	union {
		struct mdp_igc_lut_data igc_lut_data;
		struct mdp_pgc_lut_data pgc_lut_data;
		struct mdp_hist_lut_data hist_lut_data;
	} data;
};

struct mdp_bl_scale_data {
	uint32_t min_lvl;
	uint32_t scale;
};

struct mdp_calib_config_data {
	uint32_t ops;
	uint32_t addr;
	uint32_t data;
};

enum {
	mdp_op_pcc_cfg,
	mdp_op_csc_cfg,
	mdp_op_lut_cfg,
	mdp_op_qseed_cfg,
	mdp_bl_scale_cfg,
	mdp_op_calib_cfg,
	mdp_op_max,
};

struct msmfb_mdp_pp {
	uint32_t op;
	union {
		struct mdp_pcc_cfg_data pcc_cfg_data;
		struct mdp_csc_cfg_data csc_cfg_data;
		struct mdp_lut_cfg_data lut_cfg_data;
		struct mdp_qseed_cfg_data qseed_cfg_data;
		struct mdp_bl_scale_data bl_scale_data;
		struct mdp_calib_config_data calib_cfg;
	} data;
};
enum {
	metadata_op_none,
	metadata_op_base_blend,
	metadata_op_frame_rate,
	metadata_op_max
};

struct mdp_blend_cfg {
	uint32_t is_premultiplied;
};

struct msmfb_metadata {
	uint32_t op;
	uint32_t flags;
	union {
		struct mdp_blend_cfg blend_cfg;
		uint32_t panel_frame_rate;
	} data;
};

struct mdp_buf_sync {
	uint32_t flags;
	uint32_t acq_fen_fd_cnt;
	int *acq_fen_fd;
	int *rel_fen_fd;
};

struct mdp_buf_fence {
	uint32_t flags;
	uint32_t acq_fen_fd_cnt;
	int acq_fen_fd[MDP_MAX_FENCE_FD];
	int rel_fen_fd[MDP_MAX_FENCE_FD];
};


struct mdp_display_commit {
	uint32_t flags;
	uint32_t wait_for_finish;
	struct fb_var_screeninfo var;
};

struct mdp_page_protection {
	uint32_t page_protection;
};


struct mdp_mixer_info {
	int pndx;
	int pnum;
	int ptype;
	int mixer_num;
	int z_order;
};

#define MAX_PIPE_PER_MIXER 5 //ss fix 4-> 5.  this value should be (MDP4_MIXER_STAGE_MAX-MDP4_MIXER_STAGE_BASE)

struct msmfb_mixer_info_req {
	int mixer_num;
	int cnt;
	struct mdp_mixer_info info[MAX_PIPE_PER_MIXER];
};

enum {
	DISPLAY_SUBSYSTEM_ID,
	ROTATOR_SUBSYSTEM_ID,
};

enum {
	MDP_WRITEBACK_MIRROR_OFF,
	MDP_WRITEBACK_MIRROR_ON,
	MDP_WRITEBACK_MIRROR_PAUSE,
	MDP_WRITEBACK_MIRROR_RESUME,
};
#endif /*_UAPI_MSM_MDP_H_*/
