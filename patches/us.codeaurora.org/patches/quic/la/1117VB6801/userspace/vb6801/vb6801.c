/* Copyright (c) 2009, Code Aurora Forum. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Code Aurora Forum nor
 *       the names of its contributors may be used to endorse or promote
 *       products derived from this software without specific prior written
 *       permission.
 *
 * Alternatively, provided that this notice is retained in full, this software
 * may be relicensed by the recipient under the terms of the GNU General Public
 * License version 2 ("GPL") and only version 2, in which case the provisions of
 * the GPL apply INSTEAD OF those given above.  If the recipient relicenses the
 * software under the GPL, then the identification text in the MODULE_LICENSE
 * macro must be changed to reflect "GPLv2" instead of "Dual BSD/GPL".  Once a
 * recipient changes the license terms to the GPL, subsequent recipients shall
 * not relicense under alternate licensing terms, including the BSD or dual
 * BSD/GPL terms.  In addition, the following license statement immediately
 * below and between the words START and END shall also then apply when this
 * software is relicensed under the GPL:
 *
 * START
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 2 and only version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * END
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include "sensor.h"
#include "config.h"
#include "vb6801.h"

#ifdef LOCAL
#undef LOCAL
#endif
#define LOCAL static

/* AF Total steps parameters */
#define VB6801_STEPS_NEAR_TO_CLOSEST_INF  25
#define VB6801_TOTAL_STEPS_NEAR_TO_FAR    25

typedef enum 
{
	VB6801_RES_PREVIEW,
	VB6801_RES_CAPTURE
} resolution_type;

/* Sensor Output */
static uint16 VB6801_QTR_SIZE_WIDTH   = 1032;
static uint16 VB6801_QTR_SIZE_HEIGHT  =  774;

static uint16 VB6801_FULL_SIZE_WIDTH  = 2064;
static uint16 VB6801_FULL_SIZE_HEIGHT = 1546;

/* DUMMY_PIXELS is set to 1 to compensate for mis-matched bayer pattern */
static uint16 VB6801_QTR_SIZE_DUMMY_PIXELS     = 0;
static uint16 VB6801_QTR_SIZE_DUMMY_LINES      = 0;
static uint16 VB6801_FULL_SIZE_DUMMY_PIXELS    = 0;
static uint16 VB6801_FULL_SIZE_DUMMY_LINES     = 0;

/*============================================================================
                        EXTERNAL VARIABLES DECLARATIONS
============================================================================*/
/* r/w copy of chromatix data. Must be initialized with
 * active sensor chromatix data everytime camera is started. 
 */
extern chromatix_parms_type camsensor_chromatix_parms;

/*****************************************************************************
 *                          RUN TIME VARIABLES
 ****************************************************************************/

static chromatix_parms_type chromatix_vb6801_parms =
{
#include "chromatix_vb6801.h"
};

/*===========================================================================
 * FUNCTION    - x -
 *
 * DESCRIPTION:
 *==========================================================================*/
static int8_t vb6801_write_exposure_gain(void *sctrl,
	uint16 gain, uint32 line)
{
	struct sensor_cfg_data_t cfg;

  sensor_ctrl_t *ctrl =
		(sensor_ctrl_t *)sctrl;

	if (ctrl->sfd <= 0)
		return FALSE;

  if (ctrl->sensor.op_mode == SENSOR_OP_MODE_PREVIEW ||
	    ctrl->sensor.op_mode == SENSOR_OP_MODE_INIT) {
    ctrl->sensor.my_reg_gain = gain;
    ctrl->sensor.my_reg_line_count = (uint16)line;
	}

  if (ctrl->sensor.op_mode == SENSOR_OP_MODE_PREVIEW)
    cfg.cfgtype = CFG_SET_EXP_GAIN;	
  else if (ctrl->sensor.op_mode == SENSOR_OP_MODE_SNAPSHOT)
    cfg.cfgtype = CFG_SET_PICT_EXP_GAIN;

	cfg.cfg.exp_gain.gain = gain;
	cfg.cfg.exp_gain.line = line;
	if(ctrl->sensor.op_mode == SENSOR_OP_MODE_PREVIEW) {
		cfg.mode = SENSOR_PREVIEW_MODE;
	}
	if(ctrl->sensor.op_mode == SENSOR_OP_MODE_SNAPSHOT) {
		cfg.mode = SENSOR_SNAPSHOT_MODE;
	}

  CDBG("vb6801_write_exposure_gain: ctrl->sensor.op_mode %d\n", ctrl->sensor.op_mode);
	CDBG("vb6801_write_exposure_gain: cfg.mode %d, cfg.cfg.exp_gain.gain %d, cfg.cfg.exp_gain.line %d\n",
	  cfg.mode, cfg.cfg.exp_gain.gain, cfg.cfg.exp_gain.line);

	if (ioctl(ctrl->sfd, MSM_CAM_IOCTL_SENSOR_IO_CFG, &cfg))
		return FALSE;

  return TRUE;
} /* vb6801_write_exposure_gain */

/*===========================================================================
 * FUNCTION    - x -
 *
 * DESCRIPTION:
 *==========================================================================*/
static int8_t vb6801_video_config(void *sctrl)
{
	struct sensor_cfg_data_t cfg;

  sensor_ctrl_t *ctrl =
		(sensor_ctrl_t *)sctrl;

	if (ctrl->sfd <= 0)
		return FALSE;

	cfg.cfgtype = CFG_SET_MODE; 
	cfg.mode    = SENSOR_PREVIEW_MODE;

  switch (ctrl->sensor.prev_res) {
  case SENSOR_QTR_SIZE: {
    /* Sensor output data format */
    ctrl->sensor.format = CAMIF_BAYER_G_R;
	
    /* Set the current dimensions */
    ctrl->sensor.sensor_width  = ctrl->sensor.qtr_size_width;
    ctrl->sensor.sensor_height = ctrl->sensor.qtr_size_height;

    /* CAMIF frame */
    ctrl->sensor.camif_frame_config.pixelsPerLine = VB6801_QTR_SIZE_WIDTH + VB6801_QTR_SIZE_DUMMY_PIXELS; 
    ctrl->sensor.camif_frame_config.linesPerFrame = VB6801_QTR_SIZE_HEIGHT + VB6801_QTR_SIZE_DUMMY_LINES;

    /* CAMIF window */
    ctrl->sensor.camif_window_width_config.firstPixel =
			VB6801_QTR_SIZE_DUMMY_PIXELS;

    ctrl->sensor.camif_window_width_config.lastPixel  =
			ctrl->sensor.camif_window_width_config.firstPixel +
			ctrl->sensor.sensor_width - 1; 

    ctrl->sensor.camif_window_height_config.firstLine =
			VB6801_QTR_SIZE_DUMMY_LINES;

    ctrl->sensor.camif_window_height_config.lastLine  =
			ctrl->sensor.camif_window_height_config.firstLine +
			ctrl->sensor.sensor_height - 1;

	  cfg.rs      = SENSOR_QTR_SIZE;
	} /* CAMSENSOR_QTR_SIZE */
    break;

  case SENSOR_FULL_SIZE: {
    /* Sensor output data format */
    ctrl->sensor.format = CAMIF_BAYER_R_G;

    ctrl->sensor.sensor_width  = ctrl->sensor.full_size_width;
    ctrl->sensor.sensor_height = ctrl->sensor.full_size_height;

    /* CAMIF frame */
    ctrl->sensor.camif_frame_config.pixelsPerLine =
			VB6801_FULL_SIZE_WIDTH;

    ctrl->sensor.camif_frame_config.linesPerFrame =
			VB6801_FULL_SIZE_HEIGHT;

    /* CAMIF window */
    ctrl->sensor.camif_window_width_config.firstPixel =
			VB6801_FULL_SIZE_DUMMY_PIXELS;

    ctrl->sensor.camif_window_width_config.lastPixel  =
      ctrl->sensor.camif_window_width_config.firstPixel +
      ctrl->sensor.sensor_width - 1;

    ctrl->sensor.camif_window_height_config.firstLine =
			VB6801_FULL_SIZE_DUMMY_LINES;

    ctrl->sensor.camif_window_height_config.lastLine  =
      ctrl->sensor.camif_window_height_config.firstLine +
      ctrl->sensor.sensor_height - 1;

	  cfg.rs      = SENSOR_FULL_SIZE;
	  CDBG("Configuring FULL size\n");
	} /* CAMSENSOR_FULL_SIZE */
    break;

  default:
    return FALSE;
  } /* switch */

	if (ioctl(ctrl->sfd, MSM_CAM_IOCTL_SENSOR_IO_CFG, &cfg))
		return FALSE;

  ctrl->sensor.curr_res = ctrl->sensor.prev_res;

  ctrl->sensor.dynamic_params.snapshot_changed_fps = FALSE;
 
  /* Disable the Epoch Interrupt by setting more
   * number of lines than in a frame */
  ctrl->sensor.epoch_line = 10000;

  return TRUE;
} /* vb6801_video_config */

/*===========================================================================
 * FUNCTION    - x -
 *
 * DESCRIPTION:
 *==========================================================================*/
static uint16 vb6801_real_to_register_gain(float gain)
{
	if (gain < 1.067)
		return 0x0000; //gain = 1.0

	if (gain >= 1.067 && gain < 1.143)
		return 0x0010; //gain = 1.07

	if (gain >= 1.143 && gain < 1.231)
		return 0x0020; //gain = 1.14

	if (gain >= 1.231 && gain < 1.333)
		return 0x0030; //gain = 1.23

	if (gain >= 1.333 && gain < 1.455)
		return 0x0040; //gain = 1.33

	if (gain >= 1.455 && gain < 1.600)
		return 0x0050; //gain = 1.45

	if (gain >= 1.600 && gain < 1.778)
		return 0x0060; //gain = 1.60

	if (gain >= 1.778 && gain < 2.000)
		return 0x0070; //gain = 1.78

	if (gain >= 2.000 && gain < 2.286)
		return 0x0080; //gain = 2.00

	if (gain >= 2.286 && gain < 2.667)
		return 0x0090; //gain = 2.29

	if (gain >= 2.667 && gain < 3.200)
		return 0x00A0; //gain = 2.66

	if (gain >= 3.200 && gain < 4.000)
		return 0x00B0; //gain = 3.20

	if (gain >= 4.000 && gain < 5.333)
		return 0x00C0; //gain = 4.00

	if (gain >= 5.333 && gain < 8.000)
		return 0x00D0; //gain = 5.31

	if (gain >= 8.000 && gain < 9.143)
		return 0x00E0; //gain = 8.03

	if (gain >= 9.143 && gain < 10.667)
		return 0x00E4; //gain = 9.12

	if (gain >= 10.667 && gain < 12.800)
		return 0x00E8; //gain = 10.72

	if (gain >= 12.800 && gain < 16.000)
		return 0x00EC; //gain = 12.74

	if (gain >= 16.000)
	  return 0x00F0; //gain = 16.03

  CDBG("vb6801_real_to_register_gain error\n");
  return 0;
} /* vb6801_real_to_register_gain */

/*===========================================================================
 * FUNCTION    - x -
 *
 * DESCRIPTION:
 *==========================================================================*/
static float vb6801_register_to_real_gain(uint16 reg_gain)
{
    if (reg_gain < 0x0010)
        return 1.0;

    if (reg_gain >= 0x0010 && reg_gain < 0x0020)
        return 1.067;

    if (reg_gain >= 0x0020 && reg_gain < 0x0030)
        return 1.143;

    if (reg_gain >= 0x0030 && reg_gain < 0x0040)
        return 1.231;

    if (reg_gain >= 0x0040 && reg_gain < 0x0050)
        return 1.333;

    if (reg_gain >= 0x0050 && reg_gain < 0x0060)
        return 1.455;

    if (reg_gain >= 0x0060 && reg_gain < 0x0070)
        return 1.600;

    if (reg_gain >= 0x0070 && reg_gain < 0x0080)
        return 1.778;

    if (reg_gain >= 0x0080 && reg_gain < 0x0090)
        return 2.000;

    if (reg_gain >= 0x0090 && reg_gain < 0x00A0)
        return 2.286;

    if (reg_gain >= 0x00A0 && reg_gain < 0x00B0)
        return 2.667;

    if (reg_gain >= 0x00B0 && reg_gain < 0x00C0)
        return 3.200;

    if (reg_gain >= 0x00C0 && reg_gain < 0x00D0)
        return 4.000;

    if (reg_gain >= 0x00D0 && reg_gain < 0x00E0)
        return 5.333;

    if (reg_gain >= 0x00E0 && reg_gain < 0x00E4)
        return 8.000;

    if (reg_gain >= 0x00E4 && reg_gain < 0x00E8)
        return 9.143;

    if (reg_gain >= 0x00E8 && reg_gain < 0x00EC)
        return 10.667;

    if (reg_gain >= 0x00EC && reg_gain < 0x00F0)
        return 12.800;

    if (reg_gain >= 0x00F0)
        return 16.000;

		CDBG("vb6801_register_to_real_gain error\n");
		return 0.0;
} /* vb6801_register_to_real_gain */

/*===========================================================================
 * FUNCTION    - x -
 *
 * DESCRIPTION:
 *==========================================================================*/
static int8_t vb6801_snapshot_config(void *sctrl)
{
  float my_preview_gain = 0;
  uint16 snap_line_count;
  uint16 snap_reg_gain;

	struct sensor_cfg_data_t cfg;

  sensor_ctrl_t *ctrl =
		(sensor_ctrl_t *)sctrl;

	if (ctrl->sfd <= 0)
		return FALSE;

	cfg.cfgtype = CFG_SET_MODE; 
	cfg.mode    = SENSOR_SNAPSHOT_MODE;

  ctrl->sensor.format = CAMIF_BAYER_G_R;

  /* Select discard first frame. */
  ctrl->sensor.discardFirstFrame =  TRUE;

  /* Set the current dimensions */
  ctrl->sensor.sensor_width  = ctrl->sensor.full_size_width;
  ctrl->sensor.sensor_height = ctrl->sensor.full_size_height;

  /* CAMIF frame */
  ctrl->sensor.camif_frame_config.pixelsPerLine =
		VB6801_FULL_SIZE_WIDTH;

  ctrl->sensor.camif_frame_config.linesPerFrame =
		VB6801_FULL_SIZE_HEIGHT;

  /* CAMIF window */
  ctrl->sensor.camif_window_width_config.firstPixel =
		VB6801_FULL_SIZE_DUMMY_PIXELS;

  ctrl->sensor.camif_window_width_config.lastPixel  =
		ctrl->sensor.camif_window_width_config.firstPixel +
    ctrl->sensor.sensor_width - 1 ;

  ctrl->sensor.camif_window_height_config.firstLine =
		VB6801_FULL_SIZE_DUMMY_LINES;

  ctrl->sensor.camif_window_height_config.lastLine  =
		ctrl->sensor.camif_window_height_config.firstLine +
    ctrl->sensor.sensor_height - 1;

  /* Active window skip pixels */
  ctrl->sensor.active_window_skip_end_pixels = 4;

	if (ioctl(ctrl->sfd, MSM_CAM_IOCTL_SENSOR_IO_CFG, &cfg))
		return FALSE;

  ctrl->sensor.curr_res = ctrl->sensor.pict_res;

  ctrl->sensor.camif_epoch_intr.epoch_line = 1192;

  my_preview_gain = vb6801_register_to_real_gain(ctrl->sensor.my_reg_gain);

  if (my_preview_gain > 2) {
    snap_line_count = (uint16)(ctrl->sensor.my_reg_line_count *2);
    my_preview_gain = my_preview_gain /2;
    snap_reg_gain = vb6801_real_to_register_gain(my_preview_gain);

    if (vb6801_write_exposure_gain(sctrl,snap_reg_gain,snap_line_count) == FALSE)
	{
		CDBG("FAILED vb6801_write_exposure_gain\n");
		return FALSE;
	}

    usleep(1000 *15);

  } else {

    if (vb6801_write_exposure_gain(sctrl, ctrl->sensor.my_reg_gain,
											ctrl->sensor.my_reg_line_count) == FALSE)
	{
		CDBG("FAILED vb6801_write_exposure_gain\n");
        return FALSE;
	}

    usleep(1000 *15);
  }

  return TRUE;
} /* vb6801_snapshot_config */

/*===========================================================================
 * FUNCTION    - x -
 *
 * DESCRIPTION:
 *==========================================================================*/
static int8_t vb6801_raw_snapshot_config(void *sctrl)
{
  float my_preview_gain = 0;
  uint16 snap_line_count;
  uint16 snap_reg_gain;
	struct sensor_cfg_data_t cfg;

  sensor_ctrl_t *ctrl =
		(sensor_ctrl_t *)sctrl;

	if (ctrl->sfd <= 0)
		return FALSE;

	cfg.cfgtype = CFG_SET_MODE; 
	cfg.mode    = SENSOR_RAW_SNAPSHOT_MODE;

  ctrl->sensor.discardFirstFrame = TRUE;

  /* Set the current dimensions */
  ctrl->sensor.sensor_width  =
		ctrl->sensor.full_size_width;

  ctrl->sensor.sensor_height =
		ctrl->sensor.full_size_height;

  /* CAMIF frame */
  ctrl->sensor.camif_frame_config.pixelsPerLine =
		VB6801_FULL_SIZE_WIDTH;

  ctrl->sensor.camif_frame_config.linesPerFrame =
		VB6801_FULL_SIZE_HEIGHT;

  /* CAMIF window */
  ctrl->sensor.camif_window_width_config.firstPixel =
		VB6801_FULL_SIZE_DUMMY_PIXELS;

  ctrl->sensor.camif_window_width_config.lastPixel  =
		ctrl->sensor.camif_window_width_config.firstPixel +
    ctrl->sensor.sensor_width - 1;

  ctrl->sensor.camif_window_height_config.firstLine =
		VB6801_FULL_SIZE_DUMMY_LINES;

  ctrl->sensor.camif_window_height_config.lastLine  =
		ctrl->sensor.camif_window_height_config.firstLine +
    ctrl->sensor.sensor_height - 1;

  ctrl->sensor.format = CAMIF_BAYER_G_R;

	if (ioctl(ctrl->sfd, MSM_CAM_IOCTL_SENSOR_IO_CFG, &cfg))
		return FALSE;

  ctrl->sensor.curr_res = ctrl->sensor.pict_res;

  ctrl->sensor.camif_epoch_intr.epoch_line = 1192;

  my_preview_gain = vb6801_register_to_real_gain(ctrl->sensor.my_reg_gain);

  if (my_preview_gain > 2) {

    snap_line_count = (uint16)(ctrl->sensor.my_reg_line_count * 2);
    my_preview_gain = my_preview_gain / 2;
    snap_reg_gain = vb6801_real_to_register_gain(my_preview_gain);

    if (vb6801_write_exposure_gain(sctrl, snap_reg_gain,
                                            snap_line_count) == FALSE)
      return FALSE;

    usleep(1000 * 15);

  } else {
   
    if (vb6801_write_exposure_gain(sctrl,ctrl->sensor.my_reg_gain,
											ctrl->sensor.my_reg_line_count) == FALSE)
      return FALSE;

    usleep(1000 * 15);
  }

  return TRUE;
} /* vb6801_raw_snapshot_config */

/*===========================================================================
 * FUNCTION    - x -
 *
 * DESCRIPTION:
 *==========================================================================*/
static int8_t vb6801_power_down(void *sctrl)
{
	struct sensor_cfg_data_t cfg;
  sensor_ctrl_t *ctrl =
		(sensor_ctrl_t *)sctrl;

	if (ctrl->sfd <= 0)
		return FALSE;

	cfg.cfgtype = CFG_PWR_DOWN;

	if (ioctl(ctrl->sfd, MSM_CAM_IOCTL_SENSOR_IO_CFG, &cfg))
		return FALSE;

	return TRUE;
} /* vb6801_power_down */

/*===========================================================================
 * FUNCTION    - x -
 *
 * DESCRIPTION:
 *==========================================================================*/
static int8_t vb6801_set_default_focus(void *sctrl, int32 af_step)
{
	struct sensor_cfg_data_t cfg;
  sensor_ctrl_t *ctrl =
		(sensor_ctrl_t *)sctrl;

	if (ctrl->sfd <= 0)
		return FALSE;

	cfg.cfgtype = CFG_SET_DEFAULT_FOCUS;
	cfg.cfg.focus.steps = af_step;

	if (ioctl(ctrl->sfd, MSM_CAM_IOCTL_SENSOR_IO_CFG, &cfg))
		return FALSE;

  return TRUE;
} /* vb6801_set_default_focus */

/*===========================================================================
 * FUNCTION    - vb6801_move_focus -
 *
 * DESCRIPTION:
 *==========================================================================*/
static int8_t vb6801_move_focus(void *sctrl,
	enum sensor_move_focus_t direction, int32 num_steps)
{
	struct sensor_cfg_data_t cfg;
  sensor_ctrl_t *ctrl =
		(sensor_ctrl_t *)sctrl;

	if (ctrl->sfd <= 0)
		return FALSE;

	cfg.cfgtype = CFG_MOVE_FOCUS;
	cfg.cfg.focus.dir   = direction;
	cfg.cfg.focus.steps = num_steps;

	if (ioctl(ctrl->sfd, MSM_CAM_IOCTL_SENSOR_IO_CFG, &cfg))
		return FALSE;

  return TRUE;
} /* vb6801_move_focus */

/*===========================================================================
 * FUNCTION    - x -
 *
 * DESCRIPTION:
 *==========================================================================*/
static int8_t vb6801_set_frame_rate(void *sctrl, uint16 fps)
{
	/*Needs to enable for camcorder */
	uint32_t frame_mult = 1 * Q10;
	uint32_t fps_divider          = 1 * Q10;
	uint32_t snapshot_fps_divider = 1 * Q10;

	struct sensor_cfg_data_t cfg;

  sensor_ctrl_t *ctrl =
		(sensor_ctrl_t *)sctrl;

	if (ctrl->sfd <= 0)
		return FALSE;

	if (fps > ctrl->sensor.max_fps)
		fps = ctrl->sensor.max_fps;

	if (fps < ctrl->sensor.min_fps)
		fps = ctrl->sensor.min_fps;

	fps_divider =
		(ctrl->sensor.max_fps * Q10) / fps;

	if (fps_divider == (1 * Q10))
		snapshot_fps_divider = 1 * Q10;
	else
		snapshot_fps_divider = 1 * Q10;

	/*Preview mode update only */
	if (ctrl->sensor.curr_res == ctrl->sensor.prev_res)
		frame_mult = fps_divider;
	else
		frame_mult = snapshot_fps_divider;

	/*use normal exposure time for antibanding to work properly. */
	if (fps == 15 * Q8)
		fps_divider = 1 * Q10;

	cfg.cfgtype = CFG_SET_FPS;
	cfg.cfg.fps.f_mult  = frame_mult;
	cfg.cfg.fps.fps_div = fps_divider;
	cfg.cfg.fps.pict_fps_div = snapshot_fps_divider;

	if (ioctl(ctrl->sfd, MSM_CAM_IOCTL_SENSOR_IO_CFG, &cfg))
		return FALSE;

	/* Preview mode update only */
	if (ctrl->sensor.curr_res == ctrl->sensor.prev_res)
		ctrl->sensor.current_fps = fps;

	return TRUE;
} /* vb6801_set_frame_rate */

/*===========================================================================
 * FUNCTION    - x -
 *
 * DESCRIPTION:
 *==========================================================================*/
static uint16 vb6801_get_preview_lines_per_frame(void *sctrl)
{
	struct sensor_cfg_data_t cfg;

  sensor_ctrl_t *ctrl =
		(sensor_ctrl_t *)sctrl;

	if (ctrl->sfd <= 0)
		return FALSE;

	cfg.cfgtype = CFG_GET_PREV_L_PF;	
	cfg.cfg.prevl_pf = 0;

	if (ioctl(ctrl->sfd, MSM_CAM_IOCTL_SENSOR_IO_CFG, &cfg))
		return 0;

  return (cfg.cfg.prevl_pf);
}

/*===========================================================================
 * FUNCTION    - x -
 *
 * DESCRIPTION:
 *==========================================================================*/
static uint16 vb6801_get_preview_pixels_per_line(void *sctrl)
{
	struct sensor_cfg_data_t cfg;

  sensor_ctrl_t *ctrl =
		(sensor_ctrl_t *)sctrl;

	if (ctrl->sfd <= 0)
		return FALSE;

	cfg.cfgtype = CFG_GET_PREV_P_PL;	
	cfg.cfg.prevp_pl = 0;

	if (ioctl(ctrl->sfd, MSM_CAM_IOCTL_SENSOR_IO_CFG, &cfg))
		return 0;

  return (cfg.cfg.prevp_pl);
}

/*===========================================================================
 * FUNCTION    - x -
 *
 * DESCRIPTION:
 *==========================================================================*/
static  uint16 vb6801_get_snapshot_lines_per_frame(void *sctrl)
{
	struct sensor_cfg_data_t cfg;

  sensor_ctrl_t *ctrl =
		(sensor_ctrl_t *)sctrl;

	if (ctrl->sfd <= 0)
		return FALSE;

	cfg.cfgtype = CFG_GET_PICT_L_PF;	
	cfg.cfg.pictl_pf = 0;

	if (ioctl(ctrl->sfd, MSM_CAM_IOCTL_SENSOR_IO_CFG, &cfg))
		return 0;

  return (cfg.cfg.pictl_pf);
}

/*===========================================================================
 * FUNCTION    - x -
 *
 * DESCRIPTION:
 *==========================================================================*/
static uint16 vb6801_get_snapshot_pixels_per_line(void *sctrl)
{
	struct sensor_cfg_data_t cfg;

  sensor_ctrl_t *ctrl =
		(sensor_ctrl_t *)sctrl;

	if (ctrl->sfd <= 0)
		return FALSE;

	cfg.cfgtype = CFG_GET_PICT_P_PL;	
	cfg.cfg.pictp_pl = 0;

	if (ioctl(ctrl->sfd, MSM_CAM_IOCTL_SENSOR_IO_CFG, &cfg))
		return 0;

  return (cfg.cfg.pictp_pl);
}

/*===========================================================================
 * FUNCTION    - x -
 *
 * DESCRIPTION:
 *==========================================================================*/
static uint16 vb6801_get_snapshot_fps(void *sctrl, uint16 fps)
{
	struct sensor_cfg_data_t cfg;

  sensor_ctrl_t *ctrl =
		(sensor_ctrl_t *)sctrl;

	if (ctrl->sfd <= 0)
		return FALSE;

	cfg.cfgtype = CFG_GET_PICT_FPS;	
	cfg.cfg.gfps.prevfps = fps;
	cfg.cfg.gfps.pictfps = 0;

	CDBG("vb6801_get_snapshot_fps: %d\n", cfg.cfg.gfps.prevfps);
	if (ioctl(ctrl->sfd, MSM_CAM_IOCTL_SENSOR_IO_CFG, &cfg))
		return 0;

  return (cfg.cfg.gfps.pictfps);
}

/*===========================================================================
 * FUNCTION    - x -
 *
 * DESCRIPTION:
 *==========================================================================*/
static  uint32 vb6801_get_snapshot_max_exposure_line_count(void *sctrl)
{
	struct sensor_cfg_data_t cfg;

  sensor_ctrl_t *ctrl =
		(sensor_ctrl_t *)sctrl;

	if (ctrl->sfd <= 0)
		return FALSE;

	cfg.cfgtype = CFG_GET_PICT_MAX_EXP_LC;	
	cfg.cfg.pict_max_exp_lc = 0;

	if (ioctl(ctrl->sfd, MSM_CAM_IOCTL_SENSOR_IO_CFG, &cfg))
		return 0;

  return (cfg.cfg.pict_max_exp_lc);
}

/*===========================================================================
 * FUNCTION    - x -
 *
 * DESCRIPTION:
 *==========================================================================*/
static int8_t vb6801_set_snapshot_frame_rate(void *sctrl, uint16 fps)
{
  sensor_ctrl_t *ctrl = (sensor_ctrl_t *)sctrl;

	if (vb6801_set_frame_rate(sctrl, fps) == FALSE)
		return FALSE;

  ctrl->sensor.dynamic_params.snapshot_changed_fps = TRUE;

	return TRUE;
}

/*===========================================================================
 * FUNCTION    - x -
 *
 * DESCRIPTION:
 *==========================================================================*/
static int8_t vb6801_set_snapshot_exposure_gain(void *sctrl, void *snapshot_config)
{
  uint32 snapshot_wait;
  int8_t ret_val = TRUE;
  float exposure_ratio = 1.0;
  float real_snap_gain;
  float ll_gain_ratio;
  uint32 snap_line_count;
  uint16 snap_reg_gain;
  uint8 current_luma;
  uint8 luma_target;

  sensor_ctrl_t *ctrl =
		(sensor_ctrl_t *)sctrl;

  sensor_snapshot_config_t *snapshot_config_ptr =
  (sensor_snapshot_config_t *) snapshot_config;

  snapshot_wait = 2* Q8 *1000 /
		(vb6801_get_snapshot_fps(ctrl, ctrl->sensor.current_fps));

  /*if auto we can maximize capture exposure*/
  if (snapshot_config_ptr->auto_mode == TRUE) {

    current_luma = snapshot_config_ptr->current_luma;

    if (current_luma < 1)
      current_luma = 1;

    luma_target = snapshot_config_ptr->luma_target;

    real_snap_gain =
			vb6801_register_to_real_gain(snapshot_config_ptr->gain);

    if (real_snap_gain > 10) {
      /* Update gain for low light to match luma target. */
      ll_gain_ratio = ((float)luma_target) / current_luma;

      if (ll_gain_ratio < 1.0)
        ll_gain_ratio = 1.0;
    } else
      ll_gain_ratio = 1.0;

    if (real_snap_gain > 2) {

      real_snap_gain = (real_snap_gain / 2) * ll_gain_ratio;
      if (real_snap_gain > ctrl->sensor.max_snap_gain) {
       
        /*For nightshot mode usage*/
        if (ctrl->sensor.current_fps == ctrl->sensor.nightshot_fps) {
					/* Boost Exposure time at expense of snapshot fps *
					 * Reduce brightness by 20% for nightshot in very low light */
          exposure_ratio = real_snap_gain / ctrl->sensor.max_snap_gain * 0.8;
        } else
          exposure_ratio = real_snap_gain / ctrl->sensor.max_snap_gain;

        real_snap_gain = ctrl->sensor.max_snap_gain;  /* 5.0 rve 7-19-07 */
      }

      if (exposure_ratio > ctrl->sensor.max_exp_ratio)  /* 5 */
        exposure_ratio = ctrl->sensor.max_exp_ratio; /* 5; */

      snap_line_count = snapshot_config_ptr->linecount * 2;
      snap_reg_gain   =
				vb6801_real_to_register_gain(real_snap_gain);

      ret_val = vb6801_write_exposure_gain(sctrl, snap_reg_gain,
                                                    (uint32)(exposure_ratio*snap_line_count));

      snapshot_config_ptr->gain = snap_reg_gain;
      snapshot_config_ptr->linecount = (uint32) (exposure_ratio*snap_line_count);

    } else {

       ret_val =
				 vb6801_write_exposure_gain(sctrl, snapshot_config_ptr->gain, snapshot_config_ptr->linecount);
    }
  } else {
   
    ret_val = vb6801_write_exposure_gain(sctrl, snapshot_config_ptr->gain, snapshot_config_ptr->linecount);
  }

  usleep(1000*snapshot_wait*exposure_ratio);
  return ret_val;
}

/*=========================================================================
 * FUNCTION    - x -
 *
 * DESCRIPTION:
 *==========================================================================*/
static void vb6801_setup_camctrl_tbl(sensor_camctrl_tbl_t *camctrl_tbl_ptr)
{
  /* enable digital gain */
  camctrl_tbl_ptr->aec_digital_gain_is_supported = TRUE;

  /* Exposure Related 3A function */
  camctrl_tbl_ptr->fast_convergence_is_supported = TRUE;
  camctrl_tbl_ptr->linear_afr_support = TRUE;
  camctrl_tbl_ptr->fixed_fps_max_exp_index = 292; 

  /* 3A handler for nightshot mode */
  camctrl_tbl_ptr->nightshot_is_supported = TRUE;

  /* Set camctrl_tbl static variables */
  camctrl_tbl_ptr->iso100_gain  = (uint32)(2.0 * Q8);

  camctrl_tbl_ptr->enable_rolloff_correction = FALSE;

  /* AF common tuning parameters -- start */
  camctrl_tbl_ptr->undershoot_protect = FALSE;
  camctrl_tbl_ptr->change_context     = TRUE;
  camctrl_tbl_ptr->af_is_supported    = TRUE;
  camctrl_tbl_ptr->hjr_bayer_filtering_enable = FALSE;
  camctrl_tbl_ptr->reset_lens_after_snap = TRUE;

  /* AEC common tuning parameter -- start */
  camctrl_tbl_ptr->high_luma_region_threshold = 130;
  /* AEC common tuning parameter -- end */

  /* AWB parameters */
  camctrl_tbl_ptr->noon_rg = 0.674;
  camctrl_tbl_ptr->noon_bg = 0.862;
}

/*===========================================================================
 * FUNCTION    - vb6801_register -
 *
 * DESCRIPTION:
 *==========================================================================*/
static void vb6801_register(sensor_function_table_t *table)
{
  table->sensor_video_config          = vb6801_video_config;
  table->sensor_snapshot_config       = vb6801_snapshot_config;
  table->sensor_raw_snapshot_config   = vb6801_raw_snapshot_config;
  table->sensor_power_down            = vb6801_power_down;
  table->sensor_write_exposure_gain   = vb6801_write_exposure_gain;
  table->sensor_set_default_focus     = vb6801_set_default_focus;
  table->sensor_move_focus            = vb6801_move_focus;
  table->sensor_register_to_real_gain = vb6801_register_to_real_gain;
  table->sensor_real_to_register_gain = vb6801_real_to_register_gain;
  table->sensor_set_frame_rate        = vb6801_set_frame_rate;  

  /* Set camctrl_tbl function pointers */
  table->get_preview_lines_per_frame  = vb6801_get_preview_lines_per_frame;
  table->get_preview_pixels_per_line  = vb6801_get_preview_pixels_per_line;
  table->get_snapshot_lines_per_frame = vb6801_get_snapshot_lines_per_frame;
  table->get_snapshot_pixels_per_line = vb6801_get_snapshot_pixels_per_line;
  table->get_snapshot_frame_per_sec	  = vb6801_get_snapshot_fps;

  table->get_snapshot_max_exposure_line_count =
		vb6801_get_snapshot_max_exposure_line_count;

  table->set_snapshot_frame_rate    =
		vb6801_set_snapshot_frame_rate;

  table->set_snapshot_exposure_gain =
		vb6801_set_snapshot_exposure_gain;
} /* vb6801_register */

/*===========================================================================

FUNCTION      VB6801_START 

DESCRIPTION
              Initialize the camsensor parameters.

DEPENDENCIES
  None

RETURN VALUE
  if successful
    TRUE
  else
    FALSE

SIDE EFFECTS
  None

===========================================================================*/
int8_t vb6801_process_start(void *ctrl)
{
  CDBG("Inside vb6801_process_start \n");  

  sensor_ctrl_t *sensor = 
    (sensor_ctrl_t *)ctrl; 

  if (chromatix_vb6801_parms.chromatix_version !=
      CHROMATIX_DMSS7500_VERSION)
    return(FALSE);

  sensor->sensor.prev_res  = SENSOR_QTR_SIZE;
  sensor->sensor.pict_res = SENSOR_FULL_SIZE;
  sensor->sensor.curr_res  = sensor->sensor.prev_res;

  /* ------------------  Sensor-specific Config -------------- */
  /* Make/model of sensor */
  sensor->sensor.sensor_model  = SENSOR_VB6801;
  /* CCD or CMOS */
  sensor->sensor.sensor_type   = SENSOR_CMOS;
  /* BAYER or YCbCr */
  sensor->sensor.output_format = SENSOR_BAYER;

  /* Initialize CAMIF operation mode */
  sensor->sensor.camif_config.syncmode    = CAMIF_APS;
  sensor->sensor.camif_config.hsyncedge   = CAMIF_LOW;
  sensor->sensor.camif_config.vsyncedge   = CAMIF_LOW;
  sensor->sensor.camif_efs_config.efs_eol = 0x0000;
  sensor->sensor.camif_efs_config.efs_sol = 0x0000;
  sensor->sensor.camif_efs_config.efs_eof = 0x0000;
  sensor->sensor.camif_efs_config.efs_sof = 0x0000;

  if (sensor->sensor.prev_res  == SENSOR_QTR_SIZE) {
    /* What is the maximum FPS that can be supported by this sensor in video mode? */
    sensor->sensor.max_video_fps   = 30 * Q8;
    /* Application assigned FPS in video mode */
    sensor->sensor.video_fps     = 30 * Q8;
    /* Snapshot mode operation */
    sensor->sensor.max_preview_fps = 30 * Q8;

    sensor->sensor.nightshot_fps   = 10 * Q8;
    sensor->sensor.nightshot_fps = sensor->sensor.nightshot_fps;

    /* May be assigned with max_preview_fps or nightshot_fps. */
    sensor->sensor.preview_fps   = 30 * Q8;
    sensor->sensor.current_fps = sensor->sensor.preview_fps;

    /* VFE's perception of Sensor output capability */

    /* Full size dimensions - 2060x1544 */
    sensor->sensor.full_size_width  = VB6801_FULL_SIZE_WIDTH;
    sensor->sensor.full_size_height = VB6801_FULL_SIZE_HEIGHT;

    /* Quarter size dimensions - 1024x768 */
    sensor->sensor.qtr_size_width  = VB6801_QTR_SIZE_WIDTH; 
    sensor->sensor.qtr_size_height = VB6801_QTR_SIZE_HEIGHT; 

  } else {
		/*full resolution preview support.*/
    /* What is the maximum FPS that can be supported by this sensor in video mode? */
    sensor->sensor.max_video_fps   = 30 * Q8;

    /* Application assigned FPS in video mode */
    sensor->sensor.video_fps     = 30 * Q8;

    /* Snapshot mode operation */
    sensor->sensor.max_preview_fps = 30 * Q8;

    sensor->sensor.nightshot_fps   = 10 * Q8;
    sensor->sensor.nightshot_fps = sensor->sensor.nightshot_fps;

    /* May be assigned with max_preview_fps or nightshot_fps. */
    sensor->sensor.preview_fps   = 30 * Q8;
    sensor->sensor.current_fps = sensor->sensor.preview_fps;

    /* VFE's perception of Sensor output capability */

    /* Full size dimensions - 2060x1544 */
    sensor->sensor.full_size_width  = VB6801_FULL_SIZE_WIDTH;
    sensor->sensor.full_size_height = VB6801_FULL_SIZE_HEIGHT;

    /* Quarter size dimensions - 1024x768 */
    sensor->sensor.qtr_size_width  = VB6801_FULL_SIZE_WIDTH; 
    sensor->sensor.qtr_size_height = VB6801_FULL_SIZE_HEIGHT; 
  }

  /* Bayer sensor must enable gamma correction. */
  sensor->sensor.gammaCorrection = TRUE;

  /* Defect pixel correction */
  sensor->chromatixAnd3a.defectPixelCorrection.enable = TRUE;
  sensor->chromatixAnd3a.defectPixelCorrection.minThreshold = 1 * Q6;
  sensor->chromatixAnd3a.defectPixelCorrection.maxThreshold = 1 * Q6;

  /* VFE HW Black level configuration */
  sensor->sensor.blackCorrection.enable      = FALSE;

  switch (sensor->sensor.prev_res) {
  case SENSOR_QTR_SIZE:
    sensor->sensor.preview_dx_decimation =
			sensor->sensor.full_size_width * Q12 / sensor->sensor.qtr_size_width;

    sensor->sensor.preview_dy_decimation =
			sensor->sensor.full_size_height * Q12 / sensor->sensor.qtr_size_height;     

      /* Set the current dimensions */
      sensor->sensor.sensor_width  =
				sensor->sensor.qtr_size_width;

      sensor->sensor.sensor_height =
				sensor->sensor.qtr_size_height;
      break;

  case SENSOR_FULL_SIZE:
      sensor->sensor.preview_dx_decimation = Q12;
      sensor->sensor.preview_dy_decimation = Q12;

      /* Set the current dimensions */
      sensor->sensor.sensor_width  = sensor->sensor.full_size_width;
      sensor->sensor.sensor_height = sensor->sensor.full_size_height;
      break;

  default:
      sensor->sensor.preview_dx_decimation = Q12;
      sensor->sensor.preview_dy_decimation = Q12;
      break;
  }

  /* This tells camera service the minimum decimation that is supported
  * by the sensor. 
  * Ex: if preview is in quarter size mode, then there is a 
  * sensor decimation of 2, so the minimum is 2 
  */
  sensor->sensor.preview_dx_decimation = 
  (sensor->sensor.preview_dx_decimation < Q12) ? Q12 : sensor->sensor.preview_dx_decimation;

  sensor->sensor.preview_dy_decimation = 
  (sensor->sensor.preview_dy_decimation < Q12) ? Q12 : sensor->sensor.preview_dy_decimation;

  /* ------------  Auto Exposure Control Config -------------- */
  sensor->chromatixAnd3a.aec_enable = TRUE;

  sensor->chromatixAnd3a.luma_target = 44;

  /* ------------  Auto Frame Rate Config (AFR) -------------- */
  /* A subset of Auto Exposure                                 */
  /* Number of entries or possible frame rates on this sensor
    Represents the depth used in the array below */
  sensor->sensor.afr_enable = TRUE;

  /* Define these frame rates */
  /* By convention, the highest frame rate will be first in the
    array (zeroth index) and the lowest last (in order). */
  sensor->sensor.num_possible_frame_rates = 7;

  sensor->sensor.frame_rate_array[0].fps = (uint16) (30 * 256); /* Q8 */
  sensor->sensor.frame_rate_array[0].use_in_auto_frame_rate = TRUE;
  sensor->sensor.frame_rate_array[0].faster_fps_gain_trigger = 0; 
  sensor->sensor.frame_rate_array[0].slower_fps_gain_trigger = 5.2;
  sensor->sensor.frame_rate_array[0].faster_fps_exp_table_index_mod =  0;
  sensor->sensor.frame_rate_array[0].slower_fps_exp_table_index_mod  = -23;

  sensor->sensor.frame_rate_array[1].fps = (uint16) (30 * 256 / 2); /* Q8 */
  sensor->sensor.frame_rate_array[1].use_in_auto_frame_rate = TRUE;
  sensor->sensor.frame_rate_array[1].faster_fps_gain_trigger = 2;
  sensor->sensor.frame_rate_array[1].slower_fps_gain_trigger = 9;
  sensor->sensor.frame_rate_array[1].faster_fps_exp_table_index_mod =  23;
  sensor->sensor.frame_rate_array[1].slower_fps_exp_table_index_mod  = -13;

  sensor->sensor.frame_rate_array[2].fps = (uint16) (30 * 256 / 3); /* Q8 */
  sensor->sensor.frame_rate_array[2].use_in_auto_frame_rate = TRUE;
  sensor->sensor.frame_rate_array[2].faster_fps_gain_trigger = 4;
  sensor->sensor.frame_rate_array[2].slower_fps_gain_trigger = 9;
  sensor->sensor.frame_rate_array[2].faster_fps_exp_table_index_mod =  13;
  sensor->sensor.frame_rate_array[2].slower_fps_exp_table_index_mod  = -9;

  sensor->sensor.frame_rate_array[3].fps = (uint16) (30 * 256 / 4); /* Q8 */
  sensor->sensor.frame_rate_array[3].use_in_auto_frame_rate = TRUE;
  sensor->sensor.frame_rate_array[3].faster_fps_gain_trigger = 5;
  sensor->sensor.frame_rate_array[3].slower_fps_gain_trigger = 100; 
  sensor->sensor.frame_rate_array[3].faster_fps_exp_table_index_mod =  9;
  sensor->sensor.frame_rate_array[3].slower_fps_exp_table_index_mod  = 0;

  sensor->sensor.frame_rate_array[4].fps = (uint16) (30 * 256 / 5); /* Q8 */
  sensor->sensor.frame_rate_array[4].use_in_auto_frame_rate = FALSE;
  sensor->sensor.frame_rate_array[4].faster_fps_gain_trigger = 0;
  sensor->sensor.frame_rate_array[4].slower_fps_gain_trigger = 100;
  sensor->sensor.frame_rate_array[4].faster_fps_exp_table_index_mod =  0;
  sensor->sensor.frame_rate_array[4].slower_fps_exp_table_index_mod  = 0;

  sensor->sensor.frame_rate_array[5].fps = (uint16) (30 * 256 / 6); /* Q8 */
  sensor->sensor.frame_rate_array[5].use_in_auto_frame_rate = FALSE;
  sensor->sensor.frame_rate_array[5].faster_fps_gain_trigger = 0;
  sensor->sensor.frame_rate_array[5].slower_fps_gain_trigger = 100; 
  sensor->sensor.frame_rate_array[5].faster_fps_exp_table_index_mod =  0;
  sensor->sensor.frame_rate_array[5].slower_fps_exp_table_index_mod  = 0;

  sensor->sensor.frame_rate_array[6].fps = (uint16) (4 * 256); /* Q8 */
  sensor->sensor.frame_rate_array[6].use_in_auto_frame_rate = FALSE;
  sensor->sensor.frame_rate_array[6].faster_fps_gain_trigger = 0;
  sensor->sensor.frame_rate_array[6].slower_fps_gain_trigger = 100; 
  sensor->sensor.frame_rate_array[6].faster_fps_exp_table_index_mod =  0;
  sensor->sensor.frame_rate_array[6].slower_fps_exp_table_index_mod  = 0;

  sensor->sensor.max_fps = sensor->sensor.frame_rate_array[0].fps;
  sensor->sensor.min_fps =
		sensor->sensor.frame_rate_array[sensor->sensor.num_possible_frame_rates-1].fps;

  if (sensor->sensor.prev_res  == SENSOR_FULL_SIZE) {
    sensor->sensor.max_fps = 10;
    sensor->sensor.min_fps = 2.5;
  }

  sensor->chromatixAnd3a.chromatix_ptr =
    &chromatix_vb6801_parms;

  /* ------------  Auto White Balance Config ----------------- */
  /* AWB -Auto White Balance Parameters */
  sensor->chromatixAnd3a.awb_enable = TRUE;

  /* ------------  Low Light Config ----------------- */
  /* Low Light Color Correction */
  sensor->chromatixAnd3a.low_light_color_correction_enable = TRUE;

  /* ------------  Auto Focus Config ------------------------- */
  /* AF -Auto Focus Parameters */
  sensor->chromatixAnd3a.af_enable = TRUE;

  /* option of process type */
  sensor->chromatixAnd3a.af_process_type =
		AF_EXHAUSTIVE_FAST;

  /* Num steps to go across whole range */
  sensor->sensor.num_steps_near_to_far =
		VB6801_TOTAL_STEPS_NEAR_TO_FAR;

  /* Default position at optimum general focus: nearest infinity
   point.  This is in terms of number of steps from near side.
   This, with the number above, defines the search space. */
  sensor->sensor.num_steps_near_to_closest_infinity =
		VB6801_STEPS_NEAR_TO_CLOSEST_INF;

  /* Num steps for coarse first phase sweep search */
  sensor->sensor.num_gross_steps_between_stat_points = 3;  /* 4; */

  /* Num steps for fine second phase sweep search */
  sensor->sensor.num_fine_steps_between_stat_points = 1;

  /* Num search points to gather when doing 2nd pass fine search */
  sensor->sensor.num_fine_search_points =
		2*sensor->sensor.num_gross_steps_between_stat_points + 1;

  /* possible closest position of lens */
  sensor->sensor.position_near_end = 1;
  /* default lens position in macro search mode */
  sensor->sensor.position_default_in_macro = 14; /* 4; */
  /* boundary between macro and normal search mode */
  sensor->sensor.position_boundary = 14;
  /* default lens position in normal search mode */
  sensor->sensor.position_default_in_normal = VB6801_STEPS_NEAR_TO_CLOSEST_INF;
  /* possible farthest position of lens */
  sensor->sensor.position_far_end = VB6801_STEPS_NEAR_TO_CLOSEST_INF;

	/* ------------  VFE Config -------------------------------- */
  /* Config information for AEC and AWB Stats coming from the VFE */

  /* Width of focus window is half of today width for a 25% area */
  sensor->chromatixAnd3a.vfeAFConfig.firstPixel =
    sensor->sensor.sensor_width >> 2;
  sensor->chromatixAnd3a.vfeAFConfig.lastPixel  = 
    (sensor->sensor.sensor_width >> 2) + 
    (sensor->sensor.sensor_width >> 1);

  sensor->chromatixAnd3a.vfeAFConfig.firstLine  =
    sensor->sensor.sensor_height >> 2;
  sensor->chromatixAnd3a.vfeAFConfig.lastLine   = 
    (sensor->sensor.sensor_height >> 2) +
    (sensor->sensor.sensor_height >> 1);

  /* ------------  Default Misc Parmas Config ---------------- */

  /* Does the sensor need/use a flash  */
  sensor->sensor.support_auto_flash = TRUE;

  sensor->sensor.pclk_invert = FALSE;
  sensor->sensor.ignore_camif_error = TRUE;

	sensor->sensor.max_snap_gain = 4;
	sensor->sensor.max_exp_ratio = 5;

  /* Register function table: */
  vb6801_register(&(sensor->fn_table));
  
  /* Setup camctrl_tbl */
  vb6801_setup_camctrl_tbl(&(sensor->camctrl_tbl));

  sensor_post_init(sensor);
  return TRUE;
} /* vb6801_start */
