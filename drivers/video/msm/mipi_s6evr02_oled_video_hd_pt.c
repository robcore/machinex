/* Copyright (c) 2010-2011, Code Aurora Forum. All rights reserved.
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

#include "msm_fb.h"
#include "msm_fb_panel.h"
#include "mipi_dsi.h"
#include "mipi_s6evr02_param.h"
#include "mipi_s6evr02_oled.h"

#ifdef CONFIG_SAMSUNG_CMC624
#include "samsung_cmc624.h"
#endif

static struct msm_panel_info pinfo;
static struct mipi_panel_data mipi_pd;

unsigned char SEQ_APPLY_LEVEL_2_KEY[] = {
	 0xF0,
	 0x5A, 0x5A
};

unsigned char SEQ_APPLY_LEVEL_2_KEY_DISABLE[] = {
	 0xF0,
	 0xA5, 0xA5
};

unsigned char SEQ_SLEEP_OUT[] = {
	0x11,
};

unsigned char SEQ_SLEEP_IN[] = {
	0x10,
};

static char all_pixel_off[] = { 0x22, /* no param */ };

unsigned char SEQ_GAMMA_CONDITION_SET_LSI[] = {
	0xCA,
	0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 	0x80, 0x80, 0x80,
	0x02, 0x03, 0x02
};

unsigned char SEQ_GAMMA_UPDATE[] = {
	0xF7,
	0x03,
};

unsigned char SEQ_BRIGHTNESS_CONTROL_ON[] = {
	0x53,
	0x20
};

unsigned char SEQ_ELVSS_CONDITION_SET_UB[] = {
	0xB6,
	0x08, 0x07
};

unsigned char SEQ_AVC2_SET[] = {
	0xF4,
	0x6B, 0x18, 0x95, 0x02, 0x11, 0x8C, 0x77, 0x01, 0x01
};

unsigned char SEQ_ELVSS_CONDITION_SET[] = {
	0xB6,
	0x08, 0x07
};

unsigned char SEQ_DISPLAY_ON[] = {
	0x29,
};

unsigned char SEQ_DISPLAY_OFF[] = {
	0x28,
};

///////////////////////////////////
// MAGNA LDI

unsigned char SEQ_APPLY_LEVEL_3_KEY[] = {
	 0xFC,
	 0x5A, 0x5A
};

unsigned char SEQ_APPLY_LEVEL_3_KEY_DISABLE[] = {
	 0xFC,
	 0xA5, 0xA5
};

unsigned char SEQ_PANEL_CONDITION_SET_MAGNA[] = {
	0xC4,
	0x4E, 0xBD, 0x00, 0x00, 0x58, 0xA7, 0x0B, 0x34, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x0B, 0x92, 0x0B, 0x92, 0x08, 0x08, 0x07,
	0x30, 0x30, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x04,
	0x04
};

unsigned char SEQ_DISPLAY_CONDITION_SET[] = {
	 0x36,
	 0x02
};

unsigned char SEQ_BRIGHTNESS_CONDITION_SET_PASS_WORD[] = {
	0xF7,
	0x5A, 0x5A
};

unsigned char SEQ_GAMMA_CONDITION_SET_MAGNA[] = {
	0xCA,
	0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x31, 0x02
};

unsigned char SEQ_BRIGHTNESS_CONDITION_SET_PASS_WORD2[] = {
	0xF7,
	0xA5, 0xA5
};

unsigned char SEQ_AID_SETTING[] = {
	0xB3,
	0x00, 0x0A, 0x00, 0x0A
};

unsigned char SEQ_ELVSS_CONTROL_SET[] = {
	0xB2, 
	0x0E, 0xB4, 0xA0, 0x00, 0x00, 0x00, 0x00
};

unsigned char SEQ_SLEW[] = {
	0xB4,
	0x33, 0x0A, 0x00
};

//////////////////////////////////////
// ELVSS

unsigned char GET_ELVSS_ID[] = {
	0x20, /* 0 = 20_dimming,*/
	0x20,/* 1 = 30*/
	0x20,/* 2 = 40*/
	0x1F,/* 3 = 50,*/
	0x1F,/* 4 = 60,*/
	0x1F,/* 5 = 70,*/
	0x1E,/* 6 = 80,*/
	0x1E,/* 7 = 90,*/
	0x1C,/* 8 = 100,*/
	0x1B,/* 9 = 110,*/
	0x19,/* 10 = 120,*/
	0x17,/* 11 = 130,*/
	0x16,/* 12 = 140,*/
	0x14,/* 13 = 150,*/
	0x12,/* 14 = 160,*/
	0x10,/* 15 = 170,*/
	0x0F,/* 16 = 180,*/
	0x15,/* 17 = 190,*/
	0x14,/* 18 = 200,*/
	0x13,/* 19 = 210,*/
	0x12,/* 20 = 220,*/
	0x11,/* 21 = 230,*/
	0x10,/* 22 = 240,*/
	0x10,/* 23 = 250,*/
	0x0B,/* 24 = 260,*/
	0x0B,/* 25 = 270,*/
	0x0B,/* 26 = 280,*/
	0x0B,/* 27 = 290,*/
	0x0B/* 28 = 300,*/
};
unsigned char elvss_cond_set[] = {
	0xB6, 0x08,
	0x07
};

struct dsi_cmd_desc elvss_update_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0,
	 sizeof(elvss_cond_set), elvss_cond_set},
};

//////////////////////////////////////
// ACL

unsigned char SEQ_ACL_OFF[] = {
	0x55,
	0x00,
};
unsigned char acl_cutoff_33[] = {
	0x55,
	0x01,
};
unsigned char acl_cutoff_40[] = {
	0x55,
	0x02,
};
unsigned char acl_cutoff_50[] = {
	0x55,
	0x03,
};

struct dsi_cmd_desc acl_on_cmds[] = {
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0,
		sizeof(acl_cutoff_40), acl_cutoff_40},
};
struct dsi_cmd_desc acl_off_cmds[] = {
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0,
		sizeof(SEQ_ACL_OFF), SEQ_ACL_OFF},
};

unsigned char SEQ_AOR_CONTROL[] = {
	0x51,
	0xFF,
};

struct dsi_cmd_desc aid_ctrl_cmds[] = {
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0,
		0, NULL},
};

static struct dsi_cmd_desc samsung_panel_ready_to_on_lsi_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0,
		sizeof(SEQ_APPLY_LEVEL_2_KEY), SEQ_APPLY_LEVEL_2_KEY},
	{DTYPE_GEN_WRITE, 1, 0, 0, 20,
		sizeof(SEQ_SLEEP_OUT), SEQ_SLEEP_OUT},
};

static struct dsi_cmd_desc samsung_panel_ready_to_on2_lsi_cmds[] = {
	{DTYPE_GEN_WRITE1, 1, 0, 0, 120,
		sizeof(SEQ_BRIGHTNESS_CONTROL_ON), SEQ_BRIGHTNESS_CONTROL_ON},
};

static struct dsi_cmd_desc samsung_panel_ready_to_on_magna_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(SEQ_APPLY_LEVEL_2_KEY), SEQ_APPLY_LEVEL_2_KEY},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(SEQ_APPLY_LEVEL_3_KEY), SEQ_APPLY_LEVEL_3_KEY},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(SEQ_PANEL_CONDITION_SET_MAGNA), SEQ_PANEL_CONDITION_SET_MAGNA},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(SEQ_DISPLAY_CONDITION_SET), SEQ_DISPLAY_CONDITION_SET},
};

static struct dsi_cmd_desc samsung_panel_ready_to_on2_magna_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(SEQ_AID_SETTING), SEQ_AID_SETTING},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(SEQ_ELVSS_CONTROL_SET), SEQ_ELVSS_CONTROL_SET},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(SEQ_ACL_OFF), SEQ_ACL_OFF},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(SEQ_SLEW), SEQ_SLEW},
	{DTYPE_DCS_WRITE, 1, 0, 0, 150,
		sizeof(SEQ_SLEEP_OUT), SEQ_SLEEP_OUT},
};

static struct dsi_cmd_desc samsung_panel_ready_to_off_cmds[] = {
	{DTYPE_GEN_WRITE, 1, 0, 0, 50,
		sizeof(SEQ_DISPLAY_OFF), SEQ_DISPLAY_OFF},
};

static struct dsi_cmd_desc samsung_panel_on_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 0,
		sizeof(SEQ_DISPLAY_ON), SEQ_DISPLAY_ON},
};

static struct dsi_cmd_desc samsung_panel_off_cmds[] = {
	{DTYPE_GEN_WRITE, 1, 0, 0, 100,
		sizeof(SEQ_SLEEP_IN), SEQ_SLEEP_IN},
};

static struct dsi_cmd_desc samsung_panel_early_off_cmds[] = {
	{DTYPE_GEN_WRITE, 1, 0, 0, 0,
		sizeof(all_pixel_off), all_pixel_off},
};

static struct dsi_cmd_desc gamma_update_cmds[] = {
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0,
	 sizeof(SEQ_GAMMA_UPDATE), SEQ_GAMMA_UPDATE},
};

static int get_candela_index(int bl_level)
{
	int backlightlevel;

	/* brightness setting from platform is from 0 to 255
	 * But in this driver, brightness is only supported from 0 to 24 */

	switch (bl_level) {
	case 0 ... 20:
		backlightlevel = GAMMA_20CD;
		break;
	case 21 ... 39:
		backlightlevel = GAMMA_30CD;
		break;
	case 40 ... 49:
		backlightlevel = GAMMA_40CD;
		break;
	case 50 ... 59:
		backlightlevel = GAMMA_50CD;
		break;
	case 60 ... 69:
		backlightlevel = GAMMA_60CD;
		break;
	case 70 ... 79:
		backlightlevel = GAMMA_70CD;
		break;
	case 80 ... 89:
		backlightlevel = GAMMA_80CD;
		break;
	case 90 ... 99:
		backlightlevel = GAMMA_90CD;
		break;
	case 100 ... 109:
		backlightlevel = GAMMA_100CD;
		break;
	case 110 ... 119:
		backlightlevel = GAMMA_110CD;
		break;
	case 120 ... 129:
		backlightlevel = GAMMA_120CD;
		break;
	case 130 ... 139:
		backlightlevel = GAMMA_130CD;
		break;
	case 140 ... 149:
		backlightlevel = GAMMA_140CD;
		break;
	case 150 ... 159:
		backlightlevel = GAMMA_150CD;
		break;
	case 160 ... 169:
		backlightlevel = GAMMA_160CD;
		break;
	case 170 ... 179:
		backlightlevel = GAMMA_170CD;
		break;
	case 180 ... 189:
		backlightlevel = GAMMA_180CD;
		break;
	case 190 ... 199:
		backlightlevel = GAMMA_190CD;
		break;
	case 200 ... 209:
		backlightlevel = GAMMA_200CD;
		break;
	case 210 ... 214:
		backlightlevel = GAMMA_210CD;
		break;
	case 215 ... 219:
		backlightlevel = GAMMA_210CD;
		break;
	case 220 ... 224:
		backlightlevel = GAMMA_220CD;
		break;
	case 225 ... 229:
		backlightlevel = GAMMA_220CD;
		break;
	case 230 ... 234:
		backlightlevel = GAMMA_230CD;
		break;
	case 235 ... 239:
		backlightlevel = GAMMA_230CD;
		break;
	case 240 ... 244:
		backlightlevel = GAMMA_240CD;
		break;
	case 245 ... 249:
		backlightlevel = GAMMA_240CD;
		break;
	case 250 ... 254:
		backlightlevel = GAMMA_250CD;
		break;
	case 255:
		if (mipi_pd.msd->dstat.auto_brightness == 1)
			backlightlevel = GAMMA_300CD;
		else
			backlightlevel = GAMMA_250CD;
		break;
	default:
		backlightlevel = GAMMA_20CD;
		break;
	}
	return backlightlevel;
}

static int set_elvss_level(int bl_level)
{
	int cd;
	
	cd = get_candela_index(bl_level);

	elvss_update_cmds[0].payload[2] = GET_ELVSS_ID[cd];

	return 0;
}

static int set_acl_on_level(int bl_level)
{
	int cd;
	
	cd = get_candela_index(bl_level);

	if(cd < GAMMA_50CD) {
		return 1;
	} else if (cd < GAMMA_300CD) {
		acl_on_cmds[0].payload = acl_cutoff_40;
	} else {
		acl_on_cmds[0].payload = acl_cutoff_50;
	}

	return 0;
}

static struct dsi_cmd_desc lsi_backlight_ctrl_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0,
	 sizeof(SEQ_GAMMA_CONDITION_SET_LSI), SEQ_GAMMA_CONDITION_SET_LSI},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0,
	 sizeof(SEQ_GAMMA_UPDATE), SEQ_GAMMA_UPDATE},
	{DTYPE_GEN_WRITE1, 1, 0, 0, 0,
	 sizeof(SEQ_AOR_CONTROL), SEQ_AOR_CONTROL},
};

static struct dsi_cmd_desc magna_backlight_ctrl_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(SEQ_BRIGHTNESS_CONDITION_SET_PASS_WORD), SEQ_BRIGHTNESS_CONDITION_SET_PASS_WORD},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		 sizeof(SEQ_GAMMA_CONDITION_SET_MAGNA), SEQ_GAMMA_CONDITION_SET_MAGNA},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(SEQ_BRIGHTNESS_CONDITION_SET_PASS_WORD2), SEQ_BRIGHTNESS_CONDITION_SET_PASS_WORD2},
};

static int prepare_brightness_control_cmd_array(struct illuminance_table *bright_table, int bl_level, int lcd_type)
{
	int cd;
	
	cd = get_candela_index(bl_level);
	
	if(lcd_type == LDI_TYPE_MAGNA) {
		aid_ctrl_cmds.payload = bright_table[cd].b3;
		aid_ctrl_cmds.dlen = 3;
			
		magna_backlight_ctrl_cmds[1].payload = bright_table[cd].gamma_setting;
		magna_backlight_ctrl_cmds[1].dlen =  sizeof(SEQ_GAMMA_CONDITION_SET_MAGNA);
	} else {
		lsi_backlight_ctrl_cmds[0].payload = bright_table[cd].gamma_setting;
		lsi_backlight_ctrl_cmds[0].dlen = sizeof(SEQ_GAMMA_CONDITION_SET_LSI);

		lsi_backlight_ctrl_cmds[2].payload = bright_table[cd].aor;
		lsi_backlight_ctrl_cmds[2].dlen = sizeof(SEQ_AOR_CONTROL);
	}

//	samsung_panel_ready_to_on_lsi_cmds[2].payload = bright_table[cd].gamma_setting;
//	samsung_panel_ready_to_on_lsi_cmds[2].dlen = 34;

	return 0;
}

static struct mipi_panel_data mipi_pd = {
	.panel_name	= "SMD_S6E8AA3X01\n",
	.ready_to_on_lsi	= {samsung_panel_ready_to_on_lsi_cmds
				, ARRAY_SIZE(samsung_panel_ready_to_on_lsi_cmds)},
	.ready_to_on2_lsi = {samsung_panel_ready_to_on2_lsi_cmds
				, ARRAY_SIZE(samsung_panel_ready_to_on2_lsi_cmds)},				
	.ready_to_on_magna = {samsung_panel_ready_to_on_magna_cmds
				, ARRAY_SIZE(samsung_panel_ready_to_on_magna_cmds)},
	.ready_to_on2_magna = {samsung_panel_ready_to_on2_magna_cmds
				, ARRAY_SIZE(samsung_panel_ready_to_on2_magna_cmds)},
	.ready_to_off	= {samsung_panel_ready_to_off_cmds
				, ARRAY_SIZE(samsung_panel_ready_to_off_cmds)},
	.on		= {samsung_panel_on_cmds
				, ARRAY_SIZE(samsung_panel_on_cmds)},
	.off		= {samsung_panel_off_cmds
				, ARRAY_SIZE(samsung_panel_off_cmds)},
	.early_off	= {samsung_panel_early_off_cmds
				, ARRAY_SIZE(samsung_panel_early_off_cmds)},
	.gamma_update = {gamma_update_cmds,
				ARRAY_SIZE(gamma_update_cmds)},
	.elvss_update = {	elvss_update_cmds,
				ARRAY_SIZE(elvss_update_cmds)},
	.acl_on = {acl_on_cmds
			, ARRAY_SIZE(acl_on_cmds)},
	.acl_off = {acl_off_cmds
			, ARRAY_SIZE(acl_off_cmds)},
	.set_acl = set_acl_on_level,
	.set_elvss = set_elvss_level,
	.aid_ctrl = {aid_ctrl_cmds
			, ARRAY_SIZE(aid_ctrl_cmds)},
	.lcd_current_cd_idx = -1,
	.backlight_ctrl_lsi = {lsi_backlight_ctrl_cmds, ARRAY_SIZE(lsi_backlight_ctrl_cmds)},
	.backlight_ctrl_magna = {magna_backlight_ctrl_cmds, ARRAY_SIZE(magna_backlight_ctrl_cmds)},
	.prepare_brightness_control_cmd_array =
		prepare_brightness_control_cmd_array,
};

static struct mipi_dsi_phy_ctrl dsi_video_mode_phy_db = {
	/* DSI_BIT_CLK at 500MHz, 4 lane, RGB888 */
	{0x0F, 0x0a, 0x04, 0x00, 0x20}, /* regulator */
	/* timing   */
	{0xB8, 0x8E, 0x1F, 0x00, 0x97, 0x99, 0x22, 0x90,
	 0x23, 0x03, 0x04, 0xa0},
	/* phy ctrl */
	{0x5f, 0x00, 0x00, 0x10},
	/* strength */
	{0xff, 0x00, 0x06, 0x00},
	/* pll control */
	{0x0, 0x7f, 0x1, 0x1a, 0x00, 0x50, 0x48, 0x63,
	 0x31, 0x0F, 0x03,/* 4 lane */
	 0x00, 0x14, 0x03, 0x00, 0x02, 0x00, 0x20, 0x00, 0x01},
};

static int __init mipi_cmd_samsung_oled_hd_pt_init(void)
{
	int ret;

	printk(KERN_DEBUG "[lcd] mipi_cmd_samsung_oled_hd_pt_init start\n");

#ifdef CONFIG_FB_MSM_MIPI_PANEL_DETECT
	if (msm_fb_detect_client("mipi_cmd_samsung_oled_hd"))
		return 0;
#endif
	pinfo.xres = 720;
	pinfo.yres = 1280;
	pinfo.mode2_xres = 0;
	pinfo.mode2_yres = 0;
	pinfo.mode2_bpp = 0;
	/*
	 *
	 * Panel's Horizontal input timing requirement is to
	 * include dummy(pad) data of 200 clk in addition to
	 * width and porch/sync width values
	 */

	pinfo.type = MIPI_VIDEO_PANEL;
	pinfo.pdest = DISPLAY_1;
	pinfo.wait_cycle = 0;
	pinfo.bpp = 24;

	pinfo.lcdc.h_back_porch = 175;
	pinfo.lcdc.h_front_porch = 173;

	pinfo.lcdc.h_pulse_width = 2;
	pinfo.lcdc.v_back_porch = 2;
	pinfo.lcdc.v_front_porch = 13;
	pinfo.lcdc.v_pulse_width = 1;
	pinfo.lcdc.border_clr = 0;	/* blk */
	pinfo.lcdc.underflow_clr = 0xff;	/* blue */
	pinfo.lcdc.hsync_skew = 0;

	pinfo.lcdc.xres_pad = 0;
	pinfo.lcdc.yres_pad = 0;

	pinfo.bl_max = 255;
	pinfo.bl_min = 1;
	pinfo.fb_num = 2;

	pinfo.clk_rate = 499000000;

	pinfo.lcd.v_back_porch = pinfo.lcdc.v_back_porch;
	pinfo.lcd.v_front_porch = pinfo.lcdc.v_front_porch;
	pinfo.lcd.v_pulse_width = pinfo.lcdc.v_pulse_width;

	pinfo.mipi.mode = DSI_VIDEO_MODE;
	pinfo.mipi.pulse_mode_hsa_he = FALSE;
	pinfo.mipi.hfp_power_stop = FALSE;
	pinfo.mipi.hbp_power_stop = FALSE;
	pinfo.mipi.hsa_power_stop = FALSE;
	pinfo.mipi.eof_bllp_power_stop = TRUE;
	pinfo.mipi.bllp_power_stop = TRUE;
	pinfo.mipi.traffic_mode = DSI_BURST_MODE;
	pinfo.mipi.dst_format = DSI_VIDEO_DST_FORMAT_RGB888;
	pinfo.mipi.vc = 0;
	pinfo.mipi.rgb_swap = DSI_RGB_SWAP_RGB;
	pinfo.mipi.data_lane0 = TRUE;
	pinfo.mipi.data_lane1 = TRUE;
	pinfo.mipi.data_lane2 = TRUE;
	pinfo.mipi.data_lane3 = TRUE;
	pinfo.mipi.t_clk_post = 0x19;
	pinfo.mipi.t_clk_pre = 0x30;
	pinfo.mipi.stream = 0; /* dma_p */
	pinfo.mipi.mdp_trigger = DSI_CMD_TRIGGER_SW;
	pinfo.mipi.dma_trigger = DSI_CMD_TRIGGER_SW;
	pinfo.mipi.frame_rate = 60;
	pinfo.mipi.dsi_phy_db = &dsi_video_mode_phy_db;
	pinfo.mipi.force_clk_lane_hs = 1;
	pinfo.mipi.esc_byte_ratio = 2;

	ret = mipi_samsung_device_register(&pinfo, MIPI_DSI_PRIM,
						MIPI_DSI_PANEL_720P_PT,
						&mipi_pd);
	if (ret)
		pr_err("%s: failed to register device!\n", __func__);

	printk(KERN_DEBUG "[lcd] mipi_cmd_samsung_oled_hd_pt_init end\n");

	return ret;
}
module_init(mipi_cmd_samsung_oled_hd_pt_init);
