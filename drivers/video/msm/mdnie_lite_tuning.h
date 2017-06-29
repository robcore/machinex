/* Copyright (c) 2009-2011, Code Aurora Forum. All rights reserved.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

#ifndef _MDNIE_LITE_TUNING_H_
#define _MDNIE_LITE_TUNING_H_

#define LDI_COORDINATE_REG 0xA1

#define MDNIE_TUNE_FIRST_SIZE 108
#define MDNIE_TUNE_SECOND_SIZE 5

#define MDNIE_COLOR_BLIND_FIRST_SIZE 118
#define MDNIE_COLOR_BLIND_SECOND_SIZE 5
#define MDNIE_COLOR_BLINDE_CMD 18

#define BROWSER_COLOR_TONE_SET

#define SIG_MDNIE_UI_MODE				0
#define SIG_MDNIE_VIDEO_MODE			1
#define SIG_MDNIE_VIDEO_WARM_MODE		2
#define SIG_MDNIE_VIDEO_COLD_MODE		3
#define SIG_MDNIE_CAMERA_MODE			4
#define SIG_MDNIE_NAVI					5
#define SIG_MDNIE_GALLERY				6
#define SIG_MDNIE_VT					7
#define SIG_MDNIE_BROWSER				8
#define SIG_MDNIE_eBOOK					9

#define SIG_MDNIE_DYNAMIC				0
#define SIG_MDNIE_STANDARD				1
#define SIG_MDNIE_NATURAL				2
#define SIG_MDNIE_MOVIE				3
#define SIG_MDNIE_AUTO				4

#define SIG_MDNIE_ISDBT_MODE		30
#define SIG_MDNIE_ISDBT_WARM_MODE	31
#define SIG_MDNIE_ISDBT_COLD_MODE	32

#ifdef BROWSER_COLOR_TONE_SET
#define SIG_MDNIE_BROWSER_TONE1	40
#define SIG_MDNIE_BROWSER_TONE2	41
#define SIG_MDNIE_BROWSER_TONE3	42
#endif

#ifdef CONFIG_MDNIE_LITE_CONTROL
#define MDNIE_VERSION "1.1 by Yank555.lu"
#define HIJACK_DISABLED	0
#define HIJACK_ENABLED	1
#endif

enum Lcd_mDNIe_UI {
	mDNIe_UI_MODE = 0,
	mDNIe_VIDEO_MODE,
	mDNIe_VIDEO_WARM_MODE,
	mDNIe_VIDEO_COLD_MODE,
	mDNIe_CAMERA_MODE,
	mDNIe_NAVI,
	mDNIe_GALLERY,
	mDNIe_VT_MODE,
	mDNIe_BROWSER_MODE,
	mDNIe_eBOOK_MODE,
	mDNIE_BLINE_MODE,
	mDNIE_DARK_SCREEN_MODE,
	MAX_mDNIe_MODE,
};

enum Lcd_mDNIe_Negative {
	mDNIe_NEGATIVE_OFF = 0,
	mDNIe_NEGATIVE_ON,
};

enum Background_Mode {
	DYNAMIC_MODE = 0,
	STANDARD_MODE,
	NATURAL_MODE,
	MOVIE_MODE,
	AUTO_MODE,
	MAX_BACKGROUND_MODE,
};

enum Outdoor_Mode {
	OUTDOOR_OFF_MODE = 0,
	OUTDOOR_ON_MODE,
	MAX_OUTDOOR_MODE,
};

enum ACCESSIBILITY {
        ACCESSIBILITY_OFF = 0,
        NEGATIVE,
        COLOR_BLIND,
        DARK_SCREEN,
        ACCESSIBILITY_MAX,
};

struct mdnie_lite_tun_type {
	bool mdnie_enable;
	enum Background_Mode background;
	enum Background_Mode real_background;
	enum Outdoor_Mode outdoor;
	enum Lcd_mDNIe_UI scenario;
	enum Lcd_mDNIe_UI real_scenario;
	enum Lcd_mDNIe_Negative negative;
	enum ACCESSIBILITY blind;
};

void mdnie_lite_tuning_init(void);
void init_mdnie_class(void);
void is_negative_on(void);
#if defined(CONFIG_FB_MSM_MIPI_RENESAS_TFT_VIDEO_FULL_HD_PT_PANEL)
int is_cabc_on ( void );
#endif

void coordinate_tunning(int x, int y);
extern unsigned int mdnie_lock;

#endif /*_MDNIE_LITE_TUNING_H_*/
