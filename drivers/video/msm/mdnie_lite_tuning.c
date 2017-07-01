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
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/clk.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/irq.h>
#include <linux/mm.h>
#include <linux/fb.h>
#include <linux/ctype.h>
#include <linux/miscdevice.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fb.h>
#include <linux/msm_mdp.h>
#include <linux/ioctl.h>
#include <linux/lcd.h>
#include <linux/sysfs_helpers.h>
#include "msm_fb.h"
#include "msm_fb_panel.h"
#include "mipi_dsi.h"
#include "mdp.h"
#include "mdp4.h"
#include "mdnie_lite_tuning.h"
#if defined(CONFIG_SUPPORT_DISPLAY_OCTA_TFT)
#include "mdnie_lite_tuning_data_jactiveltexx.h"
#elif defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_QHD_PT)
#include "mdnie_lite_tuning_data_serrano.h"
#include "mipi_samsung_oled-8930.h"
#elif defined(CONFIG_FB_MSM_MIPI_HX8389B_TFT_VIDEO_QHD_PT_PANEL)
#include "mdnie_lite_tuning_data_wilcox.h"
#else
#include "mdnie_lite_tuning_data.h"
#endif
#if defined(CONFIG_FB_MSM_MIPI_HX8389B_TFT_VIDEO_QHD_PT_PANEL)
#include "mipi_hx8389b_tft.h"
#else
#include "mipi_samsung_octa.h"
#endif
#if defined(CONFIG_TDMB)
#include "mdnie_lite_tuning_data_dmb.h"
#endif

#if defined(CONFIG_MDNIE_LITE_CONTROL)
#define MDNIE_HIJACK "By: Wootever, Whathub, Yank555, robcore"
#endif

#ifdef MDNIE_LITE_TUN_DEBUG
#define DPRINT(x...)	printk(KERN_ERR "[mdnie lite] " x)
#else
#define DPRINT(x...)
#endif

#define MAX_LUT_SIZE	256

#if defined(CONFIG_FB_MSM_MIPI_HX8389B_TFT_VIDEO_QHD_PT_PANEL)
#define PAYLOAD1 mdni_tune_cmd[1]
#define PAYLOAD2 mdni_tune_cmd[0]

#define INPUT_PAYLOAD1(x) PAYLOAD1.payload = x
#define INPUT_PAYLOAD2(x) PAYLOAD2.payload = x
#else
#define PAYLOAD1 mdni_tune_cmd[2]
#define PAYLOAD2 mdni_tune_cmd[1]

#define INPUT_PAYLOAD1(x) PAYLOAD1.payload = x
#define INPUT_PAYLOAD2(x) PAYLOAD2.payload = x
#endif

unsigned int mdnie_locked = 0;
unsigned int hijack = HIJACK_DISABLED; /* By default, do not enable hijacking */
int curve = 0;
int black = 0;
int black_r = 0;
int black_g = 0;
int black_b = 0;

int play_speed_1_5;
#if defined(CONFIG_FB_MSM_MIPI_RENESAS_TFT_VIDEO_FULL_HD_PT_PANEL)
#if defined (CONFIG_MACH_JACTIVE_EUR) || defined (CONFIG_MACH_JACTIVE_ATT)
static int cabc = -1;
#else
static int cabc = 0;
#endif

extern int mipi_samsung_cabc_onoff ( int enable );
#endif

struct dsi_buf mdnie_tun_tx_buf;
struct dsi_buf mdnie_tun_rx_buf;

const char scenario_name[MAX_mDNIe_MODE][16] = {
	"UI_MODE",
	"VIDEO_MODE",
	"VIDEO_WARM_MODE",
	"VIDEO_COLD_MODE",
	"CAMERA_MODE",
	"NAVI",
	"GALLERY_MODE",
	"VT_MODE",
	"BROWSER",
	"eBOOK",
};

const char background_name[MAX_BACKGROUND_MODE][10] = {
	"DYNAMIC",
	"STANDARD",
	"NATURAL",
	"MOVIE",
	"AUTO",
};

struct mdnie_lite_tun_type mdnie_tun_state = {
	.mdnie_enable = false,
	.scenario = mDNIe_UI_MODE,
	.background = DYNAMIC_MODE,
	.real_scenario = mDNIe_UI_MODE,
	.real_background = DYNAMIC_MODE,
	.outdoor = OUTDOOR_OFF_MODE,
	.negative = mDNIe_NEGATIVE_OFF,
	.blind = ACCESSIBILITY_OFF,
};

const char accessibility_name[ACCESSIBILITY_MAX][20] = {
	"ACCESSIBILITY_OFF",
	"NEGATIVE_MODE",
	"COLOR_BLIND_MODE",
};

static char tune_data1[MDNIE_TUNE_FIRST_SIZE] = {0,};
static char tune_data2[MDNIE_TUNE_SECOND_SIZE] = {0,};
#if defined(CONFIG_DISPLAY_DISABLE_TEST_KEY)
static char level1_key_enable[] = {
	0xF0,
	0x5A, 0x5A,
};

static char level1_key_disable[] = {
	0xF0,
	0xA5, 0xA5,
};

static struct dsi_cmd_desc mdni_tune_cmd[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(level1_key_enable), level1_key_enable},

	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(tune_data1), tune_data1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(tune_data2), tune_data2},

	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(level1_key_disable), level1_key_disable},
};
#elif defined(CONFIG_FB_MSM_MIPI_HX8389B_TFT_VIDEO_QHD_PT_PANEL)
static struct dsi_cmd_desc mdni_tune_cmd[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(tune_data1), tune_data1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
                sizeof(tune_data2), tune_data2},
};
#else
static char level1_key[] = {
	0xF0,
	0x5A, 0x5A,
};

static struct dsi_cmd_desc mdni_tune_cmd[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(level1_key), level1_key},

	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(tune_data1), tune_data1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0,
		sizeof(tune_data2), tune_data2},
};
#endif

#if defined(CONFIG_MDNIE_LITE_CONTROL)
void update_mdnie_curve(void)
{
	char	*source;
	int	i;

	// Determine the source to copy the curves from
	switch (curve) {
		case DYNAMIC_MODE:	source = DYNAMIC_UI_2;
					break;
		case STANDARD_MODE:	source = STANDARD_UI_2;
					break;
		case NATURAL_MODE:	source = NATURAL_UI_2;
					break;
		case MOVIE_MODE:	source = MOVIE_UI_2;
					break;
		case AUTO_MODE:		source = AUTO_UI_2;
					break;
		default: return;
	}

	for (i = 42; i < 108; i++)
		LITE_CONTROL_2[i] = source[i];

	pr_debug(" = update curve values =\n");
}

void update_mdnie_mode(void)
{
	char	*source_1, *source_2;
	int	i;

	// Determine the source to copy the mode from
	switch (curve) {
		case DYNAMIC_MODE:	source_1 = DYNAMIC_UI_1;
					source_2 = DYNAMIC_UI_2;
					break;
		case STANDARD_MODE:	source_1 = STANDARD_UI_1;
					source_2 = STANDARD_UI_2;
					break;
		case NATURAL_MODE:	source_1 = NATURAL_UI_1;
					source_2 = NATURAL_UI_2;
					break;
		case MOVIE_MODE:	source_1 = MOVIE_UI_1;
					source_2 = MOVIE_UI_2;
					break;
		case AUTO_MODE:		source_1 = AUTO_UI_1;
					source_2 = AUTO_UI_2;
					break;
		default: return;
	}

	LITE_CONTROL_1[4] = source_1[4]; // Copy sharpen

	for (i = 18; i < 108; i++)
		LITE_CONTROL_2[i] = source_2[i]; // Copy mode

	// Apply black crush delta
	LITE_CONTROL_2[37] = max(0,min(255, LITE_CONTROL_2[37] + black));
	LITE_CONTROL_2[39] = max(0,min(255, LITE_CONTROL_2[39] + black));
	LITE_CONTROL_2[41] = max(0,min(255, LITE_CONTROL_2[41] + black));

	pr_debug(" = update mode values =\n");
}
#endif

void print_tun_data(void)
{
}

void free_tun_cmd(void)
{
	memset(tune_data1, 0, MDNIE_TUNE_FIRST_SIZE);
	memset(tune_data2, 0, MDNIE_TUNE_SECOND_SIZE);
}

void sending_tuning_cmd(void)
{
	struct msm_fb_data_type *mfd;
	struct dcs_cmd_req cmdreq;
#if defined (CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_QHD_PT)
		if(get_lcd_attached() == 0)
			return;
#endif

	mfd = (struct msm_fb_data_type *) registered_fb[0]->par;

	if (mfd == NULL || !mfd)
		return;

	if (mfd->resume_state == MIPI_SUSPEND_STATE)
		return;

	if (mfd->panel.type == MIPI_VIDEO_PANEL)
		mutex_lock(&dsi_tx_mutex);
	else
		mutex_lock(&mfd->dma->ov_mutex);

	cmdreq.cmds = mdni_tune_cmd;
	cmdreq.cmds_cnt = ARRAY_SIZE(mdni_tune_cmd);
	cmdreq.flags = CMD_REQ_COMMIT;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;

	mipi_dsi_cmdlist_put(&cmdreq);

	if (mfd->panel.type == MIPI_VIDEO_PANEL)
		mutex_unlock(&dsi_tx_mutex);
	else
		mutex_unlock(&mfd->dma->ov_mutex);

	free_tun_cmd();
}

void mDNIe_Set_Mode(enum Lcd_mDNIe_UI mode)
{
	struct msm_fb_data_type *mfd;

	mfd = (struct msm_fb_data_type *) registered_fb[0]->par;

	if (mfd == NULL || !mfd)
		return;

	if (mfd->resume_state == MIPI_SUSPEND_STATE)
		return;

	if (!mdnie_tun_state.mdnie_enable ||
		mdnie_tun_state.negative ||
		mode < mDNIe_UI_MODE ||
		mode >= MAX_mDNIe_MODE)
		return;

	play_speed_1_5 = 0;

	/*
	*	Blind mode & Screen mode has separated menu.
	*	To make a sync below code added.
	*	Bline mode has priority than Screen mode
	*/
	if (mdnie_tun_state.blind == COLOR_BLIND)
		mode = mDNIE_BLINE_MODE;
	else if (mdnie_tun_state.blind == DARK_SCREEN)
		mode = mDNIE_DARK_SCREEN_MODE;

	if (mdnie_locked) {
		mdnie_tun_state.scenario = mdnie_tun_state.real_scenario;
		mode = mdnie_tun_state.real_scenario;
	} else {
		mode = mdnie_tun_state.scenario;
	}

	switch (mode) {
	case mDNIe_UI_MODE:
	if (hijack == HIJACK_ENABLED) {
		goto jacked;
	} else if (mdnie_locked) {
		if (mdnie_tun_state.real_background == DYNAMIC_MODE) {
			DPRINT(" = DYNAMIC MODE =\n");
			INPUT_PAYLOAD1(DYNAMIC_UI_1);
			INPUT_PAYLOAD2(DYNAMIC_UI_2);
		} else if (mdnie_tun_state.real_background == STANDARD_MODE) {
			DPRINT(" = STANDARD MODE =\n");
			INPUT_PAYLOAD1(STANDARD_UI_1);
			INPUT_PAYLOAD2(STANDARD_UI_2);
		} else if (mdnie_tun_state.real_background == NATURAL_MODE) {
			DPRINT(" = NATURAL MODE =\n");
			INPUT_PAYLOAD1(NATURAL_UI_1);
			INPUT_PAYLOAD2(NATURAL_UI_2);
		} else if (mdnie_tun_state.real_background == MOVIE_MODE) {
			DPRINT(" = MOVIE MODE =\n");
			INPUT_PAYLOAD1(MOVIE_UI_1);
			INPUT_PAYLOAD2(MOVIE_UI_2);
		} else if (mdnie_tun_state.real_background == AUTO_MODE) {
			DPRINT(" = AUTO MODE =\n");
			INPUT_PAYLOAD1(AUTO_UI_1);
			INPUT_PAYLOAD2(AUTO_UI_2);
		}
		break;
	} else {
		if (mdnie_tun_state.background == DYNAMIC_MODE) {
			DPRINT(" = DYNAMIC MODE =\n");
			INPUT_PAYLOAD1(DYNAMIC_UI_1);
			INPUT_PAYLOAD2(DYNAMIC_UI_2);
		} else if (mdnie_tun_state.background == STANDARD_MODE) {
			DPRINT(" = STANDARD MODE =\n");
			INPUT_PAYLOAD1(STANDARD_UI_1);
			INPUT_PAYLOAD2(STANDARD_UI_2);
		} else if (mdnie_tun_state.background == NATURAL_MODE) {
			DPRINT(" = NATURAL MODE =\n");
			INPUT_PAYLOAD1(NATURAL_UI_1);
			INPUT_PAYLOAD2(NATURAL_UI_2);
		} else if (mdnie_tun_state.background == MOVIE_MODE) {
			DPRINT(" = MOVIE MODE =\n");
			INPUT_PAYLOAD1(MOVIE_UI_1);
			INPUT_PAYLOAD2(MOVIE_UI_2);
		} else if (mdnie_tun_state.background == AUTO_MODE) {
			DPRINT(" = AUTO MODE =\n");
			INPUT_PAYLOAD1(AUTO_UI_1);
			INPUT_PAYLOAD2(AUTO_UI_2);
		}
		break;
	}

	case mDNIe_VIDEO_MODE:
	if (hijack == HIJACK_ENABLED) {
		goto jacked;
	} else if (mdnie_locked) {
		if (mdnie_tun_state.outdoor == OUTDOOR_ON_MODE) {
			DPRINT(" = OUTDOOR ON MODE =\n");
			INPUT_PAYLOAD1(OUTDOOR_VIDEO_1);
			INPUT_PAYLOAD2(OUTDOOR_VIDEO_2);
		} else if (mdnie_tun_state.outdoor == OUTDOOR_OFF_MODE) {
			DPRINT(" = OUTDOOR OFF MODE =\n");
			if (mdnie_tun_state.real_background == DYNAMIC_MODE) {
				DPRINT(" = DYNAMIC MODE =\n");
				INPUT_PAYLOAD1(DYNAMIC_VIDEO_1);
				INPUT_PAYLOAD2(DYNAMIC_VIDEO_2);
			} else if (mdnie_tun_state.real_background == STANDARD_MODE) {
				DPRINT(" = STANDARD MODE =\n");
				INPUT_PAYLOAD1(STANDARD_VIDEO_1);
				INPUT_PAYLOAD2(STANDARD_VIDEO_2);
			} else if (mdnie_tun_state.real_background == NATURAL_MODE) {
				DPRINT(" = NATURAL MODE =\n");
				INPUT_PAYLOAD1(NATURAL_VIDEO_1);
				INPUT_PAYLOAD2(NATURAL_VIDEO_2);
			} else if (mdnie_tun_state.real_background == MOVIE_MODE) {
				DPRINT(" = MOVIE MODE =\n");
				INPUT_PAYLOAD1(MOVIE_VIDEO_1);
				INPUT_PAYLOAD2(MOVIE_VIDEO_2);
			} else if (mdnie_tun_state.real_background == AUTO_MODE) {
				DPRINT(" = AUTO MODE =\n");
				INPUT_PAYLOAD1(AUTO_VIDEO_1);
				INPUT_PAYLOAD2(AUTO_VIDEO_2);
			}
		}
		break;
	} else {
		if (mdnie_tun_state.outdoor == OUTDOOR_ON_MODE) {
			DPRINT(" = OUTDOOR ON MODE =\n");
			INPUT_PAYLOAD1(OUTDOOR_VIDEO_1);
			INPUT_PAYLOAD2(OUTDOOR_VIDEO_2);
		} else if (mdnie_tun_state.outdoor == OUTDOOR_OFF_MODE) {
			DPRINT(" = OUTDOOR OFF MODE =\n");
			if (mdnie_tun_state.background == DYNAMIC_MODE) {
				DPRINT(" = DYNAMIC MODE =\n");
				INPUT_PAYLOAD1(DYNAMIC_VIDEO_1);
				INPUT_PAYLOAD2(DYNAMIC_VIDEO_2);
			} else if (mdnie_tun_state.background == STANDARD_MODE) {
				DPRINT(" = STANDARD MODE =\n");
				INPUT_PAYLOAD1(STANDARD_VIDEO_1);
				INPUT_PAYLOAD2(STANDARD_VIDEO_2);
			} else if (mdnie_tun_state.background == NATURAL_MODE) {
				DPRINT(" = NATURAL MODE =\n");
				INPUT_PAYLOAD1(NATURAL_VIDEO_1);
				INPUT_PAYLOAD2(NATURAL_VIDEO_2);
			} else if (mdnie_tun_state.background == MOVIE_MODE) {
				DPRINT(" = MOVIE MODE =\n");
				INPUT_PAYLOAD1(MOVIE_VIDEO_1);
				INPUT_PAYLOAD2(MOVIE_VIDEO_2);
			} else if (mdnie_tun_state.background == AUTO_MODE) {
				DPRINT(" = AUTO MODE =\n");
				INPUT_PAYLOAD1(AUTO_VIDEO_1);
				INPUT_PAYLOAD2(AUTO_VIDEO_2);
			}
		}
		break;
	}

	case mDNIe_VIDEO_WARM_MODE:
	if (hijack == HIJACK_ENABLED)
		goto jacked;
		if (mdnie_tun_state.outdoor == OUTDOOR_ON_MODE) {
			DPRINT(" = OUTDOOR ON MODE =\n");
			INPUT_PAYLOAD1(WARM_OUTDOOR_1);
			INPUT_PAYLOAD2(WARM_OUTDOOR_2);
		} else if (mdnie_tun_state.outdoor == OUTDOOR_OFF_MODE) {
			DPRINT(" = OUTDOOR OFF MODE =\n");
			INPUT_PAYLOAD1(WARM_1);
			INPUT_PAYLOAD2(WARM_2);
		}
		break;

	case mDNIe_VIDEO_COLD_MODE:
	if (hijack == HIJACK_ENABLED)
		goto jacked;
		if (mdnie_tun_state.outdoor == OUTDOOR_ON_MODE) {
			DPRINT(" = OUTDOOR ON MODE =\n");
			INPUT_PAYLOAD1(COLD_OUTDOOR_1);
			INPUT_PAYLOAD2(COLD_OUTDOOR_2);
		} else if (mdnie_tun_state.outdoor == OUTDOOR_OFF_MODE) {
			DPRINT(" = OUTDOOR OFF MODE =\n");
			INPUT_PAYLOAD1(COLD_1);
			INPUT_PAYLOAD2(COLD_2);
		}
		break;

	case mDNIe_CAMERA_MODE:
	if (hijack == HIJACK_ENABLED) {
		goto jacked;
	} else if (mdnie_locked) {
		if (mdnie_tun_state.outdoor == OUTDOOR_OFF_MODE) {
			if (mdnie_tun_state.real_background == AUTO_MODE) {
				DPRINT(" = AUTO MODE =\n");
				INPUT_PAYLOAD1(AUTO_CAMERA_1);
				INPUT_PAYLOAD2(AUTO_CAMERA_2);
			} else {
				DPRINT(" = STANDARD MODE =\n");
				INPUT_PAYLOAD1(CAMERA_1);
				INPUT_PAYLOAD2(CAMERA_2);
			}
		} else if (mdnie_tun_state.outdoor == OUTDOOR_ON_MODE) {
			DPRINT(" = NATURAL MODE =\n");
			INPUT_PAYLOAD1(CAMERA_OUTDOOR_1);
			INPUT_PAYLOAD2(CAMERA_OUTDOOR_2);
		}
		break;
	} else {
		if (mdnie_tun_state.outdoor == OUTDOOR_OFF_MODE) {
			if (mdnie_tun_state.background == AUTO_MODE) {
				DPRINT(" = AUTO MODE =\n");
				INPUT_PAYLOAD1(AUTO_CAMERA_1);
				INPUT_PAYLOAD2(AUTO_CAMERA_2);
			} else {
				DPRINT(" = STANDARD MODE =\n");
				INPUT_PAYLOAD1(CAMERA_1);
				INPUT_PAYLOAD2(CAMERA_2);
			}
		} else if (mdnie_tun_state.outdoor == OUTDOOR_ON_MODE) {
			DPRINT(" = NATURAL MODE =\n");
			INPUT_PAYLOAD1(CAMERA_OUTDOOR_1);
			INPUT_PAYLOAD2(CAMERA_OUTDOOR_2);
		}
		break;
	}
	case mDNIe_NAVI:
	if (hijack == HIJACK_ENABLED) {
		goto jacked;
	} else
		break;

	case mDNIe_GALLERY:
	if (hijack == HIJACK_ENABLED) {
		goto jacked;
	} else if (mdnie_locked) {
		if (mdnie_tun_state.real_background == DYNAMIC_MODE) {
			DPRINT(" = DYNAMIC MODE =\n");
			INPUT_PAYLOAD1(DYNAMIC_GALLERY_1);
			INPUT_PAYLOAD2(DYNAMIC_GALLERY_2);
		} else if (mdnie_tun_state.real_background == STANDARD_MODE) {
			DPRINT(" = STANDARD MODE =\n");
			INPUT_PAYLOAD1(STANDARD_GALLERY_1);
			INPUT_PAYLOAD2(STANDARD_GALLERY_2);
		} else if (mdnie_tun_state.real_background == NATURAL_MODE) {
			DPRINT(" = NATURAL MODE =\n");
			INPUT_PAYLOAD1(NATURAL_GALLERY_1);
			INPUT_PAYLOAD2(NATURAL_GALLERY_2);
		} else if (mdnie_tun_state.real_background == MOVIE_MODE) {
			DPRINT(" = MOVIE MODE =\n");
			INPUT_PAYLOAD1(MOVIE_GALLERY_1);
			INPUT_PAYLOAD2(MOVIE_GALLERY_2);
		} else if (mdnie_tun_state.real_background == AUTO_MODE) {
			DPRINT(" = AUTO MODE =\n");
			INPUT_PAYLOAD1(AUTO_GALLERY_1);
			INPUT_PAYLOAD2(AUTO_GALLERY_2);
		}
		break;
	} else {
		if (mdnie_tun_state.background == DYNAMIC_MODE) {
			DPRINT(" = DYNAMIC MODE =\n");
			INPUT_PAYLOAD1(DYNAMIC_GALLERY_1);
			INPUT_PAYLOAD2(DYNAMIC_GALLERY_2);
		} else if (mdnie_tun_state.background == STANDARD_MODE) {
			DPRINT(" = STANDARD MODE =\n");
			INPUT_PAYLOAD1(STANDARD_GALLERY_1);
			INPUT_PAYLOAD2(STANDARD_GALLERY_2);
		} else if (mdnie_tun_state.background == NATURAL_MODE) {
			DPRINT(" = NATURAL MODE =\n");
			INPUT_PAYLOAD1(NATURAL_GALLERY_1);
			INPUT_PAYLOAD2(NATURAL_GALLERY_2);
		} else if (mdnie_tun_state.background == MOVIE_MODE) {
			DPRINT(" = MOVIE MODE =\n");
			INPUT_PAYLOAD1(MOVIE_GALLERY_1);
			INPUT_PAYLOAD2(MOVIE_GALLERY_2);
		} else if (mdnie_tun_state.background == AUTO_MODE) {
			DPRINT(" = AUTO MODE =\n");
			INPUT_PAYLOAD1(AUTO_GALLERY_1);
			INPUT_PAYLOAD2(AUTO_GALLERY_2);
		}
		break;
	}

	case mDNIe_VT_MODE:
	if (hijack == HIJACK_ENABLED) {
		goto jacked;
	} else if (mdnie_locked) {
		if (mdnie_tun_state.real_background == DYNAMIC_MODE) {
			DPRINT(" = DYNAMIC MODE =\n");
			INPUT_PAYLOAD1(DYNAMIC_VT_1);
			INPUT_PAYLOAD2(DYNAMIC_VT_2);
		} else if (mdnie_tun_state.real_background == STANDARD_MODE) {
			DPRINT(" = STANDARD MODE =\n");
			INPUT_PAYLOAD1(STANDARD_VT_1);
			INPUT_PAYLOAD2(STANDARD_VT_2);
		} else if (mdnie_tun_state.real_background == NATURAL_MODE) {
			DPRINT(" = NATURAL MODE =\n");
			INPUT_PAYLOAD1(NATURAL_VT_1);
			INPUT_PAYLOAD2(NATURAL_VT_2);
		} else if (mdnie_tun_state.real_background == MOVIE_MODE) {
			DPRINT(" = MOVIE MODE =\n");
			INPUT_PAYLOAD1(MOVIE_VT_1);
			INPUT_PAYLOAD2(MOVIE_VT_2);
		} else if (mdnie_tun_state.real_background == AUTO_MODE) {
			DPRINT(" = AUTO MODE =\n");
			INPUT_PAYLOAD1(AUTO_VT_1);
			INPUT_PAYLOAD2(AUTO_VT_2);
		}
		break;
	} else {
		if (mdnie_tun_state.background == DYNAMIC_MODE) {
			DPRINT(" = DYNAMIC MODE =\n");
			INPUT_PAYLOAD1(DYNAMIC_VT_1);
			INPUT_PAYLOAD2(DYNAMIC_VT_2);
		} else if (mdnie_tun_state.background == STANDARD_MODE) {
			DPRINT(" = STANDARD MODE =\n");
			INPUT_PAYLOAD1(STANDARD_VT_1);
			INPUT_PAYLOAD2(STANDARD_VT_2);
		} else if (mdnie_tun_state.background == NATURAL_MODE) {
			DPRINT(" = NATURAL MODE =\n");
			INPUT_PAYLOAD1(NATURAL_VT_1);
			INPUT_PAYLOAD2(NATURAL_VT_2);
		} else if (mdnie_tun_state.background == MOVIE_MODE) {
			DPRINT(" = MOVIE MODE =\n");
			INPUT_PAYLOAD1(MOVIE_VT_1);
			INPUT_PAYLOAD2(MOVIE_VT_2);
		} else if (mdnie_tun_state.background == AUTO_MODE) {
			DPRINT(" = AUTO MODE =\n");
			INPUT_PAYLOAD1(AUTO_VT_1);
			INPUT_PAYLOAD2(AUTO_VT_2);
		}
		break;
	}

	case mDNIe_BROWSER_MODE:
	if (hijack == HIJACK_ENABLED) {
		goto jacked;
	} else if (mdnie_locked) {
		if (mdnie_tun_state.real_background == DYNAMIC_MODE) {
			DPRINT(" = DYNAMIC MODE =\n");
			INPUT_PAYLOAD1(DYNAMIC_BROWSER_1);
			INPUT_PAYLOAD2(DYNAMIC_BROWSER_2);
		} else if (mdnie_tun_state.real_background == STANDARD_MODE) {
			DPRINT(" = STANDARD MODE =\n");
			INPUT_PAYLOAD1(STANDARD_BROWSER_1);
			INPUT_PAYLOAD2(STANDARD_BROWSER_2);
		} else if (mdnie_tun_state.real_background == NATURAL_MODE) {
			DPRINT(" = NATURAL MODE =\n");
			INPUT_PAYLOAD1(NATURAL_BROWSER_1);
			INPUT_PAYLOAD2(NATURAL_BROWSER_2);

		} else if (mdnie_tun_state.real_background == MOVIE_MODE) {
			DPRINT(" = MOVIE MODE =\n");
			INPUT_PAYLOAD1(MOVIE_BROWSER_1);
			INPUT_PAYLOAD2(MOVIE_BROWSER_2);
		} else if (mdnie_tun_state.real_background == AUTO_MODE) {
			DPRINT(" = AUTO MODE =\n");
			INPUT_PAYLOAD1(AUTO_BROWSER_1);
			INPUT_PAYLOAD2(AUTO_BROWSER_2);
		}
		break;
	} else {
		if (mdnie_tun_state.background == DYNAMIC_MODE) {
			DPRINT(" = DYNAMIC MODE =\n");
			INPUT_PAYLOAD1(DYNAMIC_BROWSER_1);
			INPUT_PAYLOAD2(DYNAMIC_BROWSER_2);
		} else if (mdnie_tun_state.background == STANDARD_MODE) {
			DPRINT(" = STANDARD MODE =\n");
			INPUT_PAYLOAD1(STANDARD_BROWSER_1);
			INPUT_PAYLOAD2(STANDARD_BROWSER_2);
		} else if (mdnie_tun_state.background == NATURAL_MODE) {
			DPRINT(" = NATURAL MODE =\n");
			INPUT_PAYLOAD1(NATURAL_BROWSER_1);
			INPUT_PAYLOAD2(NATURAL_BROWSER_2);

		} else if (mdnie_tun_state.background == MOVIE_MODE) {
			DPRINT(" = MOVIE MODE =\n");
			INPUT_PAYLOAD1(MOVIE_BROWSER_1);
			INPUT_PAYLOAD2(MOVIE_BROWSER_2);
		} else if (mdnie_tun_state.background == AUTO_MODE) {
			DPRINT(" = AUTO MODE =\n");
			INPUT_PAYLOAD1(AUTO_BROWSER_1);
			INPUT_PAYLOAD2(AUTO_BROWSER_2);
		}
		break;
	}

	case mDNIe_eBOOK_MODE:
	if (hijack == HIJACK_ENABLED) {
		goto jacked;
	} else if (mdnie_locked) {
			if (mdnie_tun_state.real_background == DYNAMIC_MODE) {
			DPRINT(" = DYNAMIC MODE =\n");
			INPUT_PAYLOAD1(DYNAMIC_EBOOK_1);
			INPUT_PAYLOAD2(DYNAMIC_EBOOK_2);
		} else if (mdnie_tun_state.real_background == STANDARD_MODE) {
			DPRINT(" = STANDARD MODE =\n");
			INPUT_PAYLOAD1(STANDARD_EBOOK_1);
			INPUT_PAYLOAD2(STANDARD_EBOOK_2);
		} else if (mdnie_tun_state.real_background == NATURAL_MODE) {
			DPRINT(" = NATURAL MODE =\n");
			INPUT_PAYLOAD1(NATURAL_EBOOK_1);
			INPUT_PAYLOAD2(NATURAL_EBOOK_2);
			} else if (mdnie_tun_state.real_background == MOVIE_MODE) {
			DPRINT(" = MOVIE MODE =\n");
			INPUT_PAYLOAD1(MOVIE_EBOOK_1);
			INPUT_PAYLOAD2(MOVIE_EBOOK_2);
		} else if (mdnie_tun_state.real_background == AUTO_MODE) {
			DPRINT(" = AUTO MODE =\n");
			INPUT_PAYLOAD1(AUTO_EBOOK_1);
			INPUT_PAYLOAD2(AUTO_EBOOK_2);
		}
		break;
	} else {
		if (mdnie_tun_state.background == DYNAMIC_MODE) {
			DPRINT(" = DYNAMIC MODE =\n");
			INPUT_PAYLOAD1(DYNAMIC_EBOOK_1);
			INPUT_PAYLOAD2(DYNAMIC_EBOOK_2);
		} else if (mdnie_tun_state.background == STANDARD_MODE) {
			DPRINT(" = STANDARD MODE =\n");
			INPUT_PAYLOAD1(STANDARD_EBOOK_1);
			INPUT_PAYLOAD2(STANDARD_EBOOK_2);
		} else if (mdnie_tun_state.background == NATURAL_MODE) {
			DPRINT(" = NATURAL MODE =\n");
			INPUT_PAYLOAD1(NATURAL_EBOOK_1);
			INPUT_PAYLOAD2(NATURAL_EBOOK_2);
			} else if (mdnie_tun_state.background == MOVIE_MODE) {
			DPRINT(" = MOVIE MODE =\n");
			INPUT_PAYLOAD1(MOVIE_EBOOK_1);
			INPUT_PAYLOAD2(MOVIE_EBOOK_2);
		} else if (mdnie_tun_state.background == AUTO_MODE) {
			DPRINT(" = AUTO MODE =\n");
			INPUT_PAYLOAD1(AUTO_EBOOK_1);
			INPUT_PAYLOAD2(AUTO_EBOOK_2);
		}
		break;
	}
	case mDNIE_BLINE_MODE:
		INPUT_PAYLOAD1(COLOR_BLIND_1);
		INPUT_PAYLOAD2(COLOR_BLIND_2);
		break;

	case mDNIE_DARK_SCREEN_MODE:
		INPUT_PAYLOAD1(DARK_SCREEN_BLIND_1);
		INPUT_PAYLOAD2(DARK_SCREEN_BLIND_2);
		break;

	default:
		break;
	}

jacked:
	if (hijack == HIJACK_ENABLED) {
		DPRINT(" = CONTROL MODE =\n");
		INPUT_PAYLOAD1(LITE_CONTROL_1);
		INPUT_PAYLOAD2(LITE_CONTROL_2);
	}

	sending_tuning_cmd();
}

void mDNIe_set_negative(enum Lcd_mDNIe_Negative negative)
{

	if (negative == 0) {
		if (mdnie_locked)
			mDNIe_Set_Mode(mdnie_tun_state.real_scenario);
		else
			mDNIe_Set_Mode(mdnie_tun_state.scenario);
	} else {
			INPUT_PAYLOAD1(NEGATIVE_1);
			INPUT_PAYLOAD2(NEGATIVE_2);

			sending_tuning_cmd();
	}
}

void is_play_speed_1_5(int enable)
{
	play_speed_1_5 = enable;
}

/* ##########################################################
 * #
 * # MDNIE BG Sysfs node
 * #
 * ##########################################################*/

/* ##########################################################
 * #
 * #	0. Dynamic
 * #	1. Standard
 * #	2. Natural
 * #	3. Movie
 * #	4. Auto
 * #
 * ##########################################################*/

static ssize_t mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	if (mdnie_locked)
		return snprintf(buf, 256, "Current Background Mode : %s\n",
			background_name[mdnie_tun_state.real_background]);
	else
		return snprintf(buf, 256, "Current Background Mode : %s\n",
			background_name[mdnie_tun_state.background]);
}

static ssize_t mode_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int value;
	int backup;

	sscanf(buf, "%d", &value);

	if (value < DYNAMIC_MODE || value >= MAX_BACKGROUND_MODE) {
		return size;
	}

	if (mdnie_locked) {
		backup = mdnie_tun_state.real_background;
		mdnie_tun_state.background = mdnie_tun_state.real_background;
		mDNIe_Set_Mode(mdnie_tun_state.real_scenario);
	} else {
		backup = mdnie_tun_state.background;
		mdnie_tun_state.background = value;
		mDNIe_Set_Mode(mdnie_tun_state.scenario);
	}

	return size;
}

static DEVICE_ATTR(mode, 0664, mode_show, mode_store);

static ssize_t real_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, 256, "Current Background Mode : %s\n",
		background_name[mdnie_tun_state.real_background]);
}

static ssize_t real_mode_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int value;
	int backup;

	sscanf(buf, "%d", &value);

	if (value < DYNAMIC_MODE || value >= MAX_BACKGROUND_MODE) {
		return size;
	}

	backup = mdnie_tun_state.real_background;
	mdnie_tun_state.real_background = value;
	mDNIe_Set_Mode(mdnie_tun_state.real_scenario);

	return size;
}

static DEVICE_ATTR(real_mode, 0664, real_mode_show, real_mode_store);

static ssize_t scenario_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	if (mdnie_locked)
		return snprintf(buf, 256, "Current Scenario Mode : %s\n",
			scenario_name[mdnie_tun_state.real_scenario]);
	else
		return snprintf(buf, 256, "Current Scenario Mode : %s\n",
			scenario_name[mdnie_tun_state.scenario]);
}

static ssize_t scenario_store(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t size)
{
	int value;
	int backup;

	sscanf(buf, "%d", &value);

	if ((value < mDNIe_UI_MODE || value >= MAX_mDNIe_MODE))
		return size;

	switch (value) {
	case SIG_MDNIE_UI_MODE:
		mdnie_tun_state.scenario = mDNIe_UI_MODE;
		break;

	case SIG_MDNIE_VIDEO_MODE:
		mdnie_tun_state.scenario = mDNIe_VIDEO_MODE;
		break;

	case SIG_MDNIE_VIDEO_WARM_MODE:
		mdnie_tun_state.scenario = mDNIe_VIDEO_WARM_MODE;
		break;

	case SIG_MDNIE_VIDEO_COLD_MODE:
		mdnie_tun_state.scenario = mDNIe_VIDEO_COLD_MODE;
		break;

	case SIG_MDNIE_CAMERA_MODE:
		mdnie_tun_state.scenario = mDNIe_CAMERA_MODE;
		break;

	case SIG_MDNIE_NAVI:
		mdnie_tun_state.scenario = mDNIe_NAVI;
		break;

	case SIG_MDNIE_GALLERY:
		mdnie_tun_state.scenario = mDNIe_GALLERY;
		break;

	case SIG_MDNIE_VT:
		mdnie_tun_state.scenario = mDNIe_VT_MODE;
		break;

	case SIG_MDNIE_BROWSER:
		mdnie_tun_state.scenario = mDNIe_BROWSER_MODE;
		break;

	case SIG_MDNIE_eBOOK:
		mdnie_tun_state.scenario = mDNIe_eBOOK_MODE;
		break;

	default:
		break;
	}

	if (mdnie_tun_state.negative)
		return size;

	if (mdnie_locked) {
		mdnie_tun_state.scenario = mdnie_tun_state.real_scenario;
		mDNIe_Set_Mode(mdnie_tun_state.real_scenario);
	} else {
		mDNIe_Set_Mode(mdnie_tun_state.scenario);
	}

	return size;
}
static DEVICE_ATTR(scenario, 0664, scenario_show,
		   scenario_store);

static ssize_t real_scenario_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	return snprintf(buf, 256, "Current Scenario Mode : %s\n",
		scenario_name[mdnie_tun_state.real_scenario]);
}

static ssize_t real_scenario_store(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t size)
{
	int value;
	int backup;

	sscanf(buf, "%d", &value);

	if ((value < mDNIe_UI_MODE || value >= MAX_mDNIe_MODE))
		return size;

	switch (value) {
	case SIG_MDNIE_UI_MODE:
		mdnie_tun_state.real_scenario = mDNIe_UI_MODE;
		break;

	case SIG_MDNIE_VIDEO_MODE:
		mdnie_tun_state.real_scenario = mDNIe_VIDEO_MODE;
		break;

	case SIG_MDNIE_VIDEO_WARM_MODE:
		mdnie_tun_state.real_scenario = mDNIe_VIDEO_WARM_MODE;
		break;

	case SIG_MDNIE_VIDEO_COLD_MODE:
		mdnie_tun_state.real_scenario = mDNIe_VIDEO_COLD_MODE;
		break;

	case SIG_MDNIE_CAMERA_MODE:
		mdnie_tun_state.real_scenario = mDNIe_CAMERA_MODE;
		break;

	case SIG_MDNIE_NAVI:
		mdnie_tun_state.real_scenario = mDNIe_NAVI;
		break;

	case SIG_MDNIE_GALLERY:
		mdnie_tun_state.real_scenario = mDNIe_GALLERY;
		break;

	case SIG_MDNIE_VT:
		mdnie_tun_state.real_scenario = mDNIe_VT_MODE;
		break;

	case SIG_MDNIE_BROWSER:
		mdnie_tun_state.real_scenario = mDNIe_BROWSER_MODE;
		break;

	case SIG_MDNIE_eBOOK:
		mdnie_tun_state.real_scenario = mDNIe_eBOOK_MODE;
		break;

	default:
		break;
	}

	if (!mdnie_tun_state.negative)
		mDNIe_Set_Mode(mdnie_tun_state.real_scenario);

	return size;
}
static DEVICE_ATTR(real_scenario, 0664, real_scenario_show,
		   real_scenario_store);

static ssize_t mdnieset_user_select_file_cmd_show(struct device *dev,
						  struct device_attribute *attr,
						  char *buf)
{
	int mdnie_ui = 0;

	return snprintf(buf, 256, "%u\n", mdnie_ui);
}

static ssize_t mdnieset_user_select_file_cmd_store(struct device *dev,
						   struct device_attribute
						   *attr, const char *buf,
						   size_t size)
{
	int value;

	sscanf(buf, "%d", &value);

	return size;
}

static DEVICE_ATTR(mdnieset_user_select_file_cmd, 0664,
		   mdnieset_user_select_file_cmd_show,
		   mdnieset_user_select_file_cmd_store);

static ssize_t mdnieset_init_file_cmd_show(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	char temp[] = "mdnieset_init_file_cmd_show\n\0";
	strcat(buf, temp);
	return strlen(buf);
}

static ssize_t mdnieset_init_file_cmd_store(struct device *dev,
					    struct device_attribute *attr,
					    const char *buf, size_t size)
{
	int value;

	sscanf(buf, "%d", &value);

	switch (value) {
	case 0:
		mdnie_tun_state.scenario = mDNIe_UI_MODE;
		break;

	default:
		break;
	}
		if (mdnie_locked) {
			mdnie_tun_state.scenario = mdnie_tun_state.real_scenario;
			mDNIe_Set_Mode(mdnie_tun_state.real_scenario);
		} else {
			mDNIe_Set_Mode(mdnie_tun_state.scenario);
		}

	return size;
}

static DEVICE_ATTR(mdnieset_init_file_cmd, 0664, mdnieset_init_file_cmd_show,
		   mdnieset_init_file_cmd_store);

static ssize_t outdoor_show(struct device *dev,
					      struct device_attribute *attr,
					      char *buf)
{
	return snprintf(buf, 256, "%s\n",
		(mdnie_tun_state.outdoor == 0) ? "Disabled" : "Enabled");
}

static ssize_t outdoor_store(struct device *dev,
					       struct device_attribute *attr,
					       const char *buf, size_t size)
{
	int value;
	int backup;

	sscanf(buf, "%d", &value);

	if (value < OUTDOOR_OFF_MODE || value >= MAX_OUTDOOR_MODE) {
		pr_debug("[ERROR] : wrong outdoor mode value");
	}

	backup = mdnie_tun_state.outdoor;
	mdnie_tun_state.outdoor = value;

	if (mdnie_tun_state.negative) {
		pr_debug("dummy\n");
		goto skipper;
	} else {
		if (mdnie_locked) {
			mdnie_tun_state.scenario = mdnie_tun_state.real_scenario;
			mDNIe_Set_Mode(mdnie_tun_state.real_scenario);
		 } else {
			mDNIe_Set_Mode(mdnie_tun_state.scenario);
		}
	}
skipper:
	return size;
}

static DEVICE_ATTR(outdoor, 0664, outdoor_show, outdoor_store);

static ssize_t negative_show(struct device *dev,
					      struct device_attribute *attr,
					      char *buf)
{
	DPRINT("called %s\n", __func__);
	return snprintf(buf, 256, "Current negative Value : %s\n",
		(mdnie_tun_state.negative == 0) ? "Disabled" : "Enabled");
}

static ssize_t negative_store(struct device *dev,
					       struct device_attribute *attr,
					       const char *buf, size_t size)
{
	int value;

	sscanf(buf, "%d", &value);

	mdnie_tun_state.negative = value;

	mDNIe_set_negative(mdnie_tun_state.negative);

	return size;
}

void is_negative_on(void)
{
	if (mdnie_tun_state.negative) {
		DPRINT("mDNIe_Set_Negative = %d\n", mdnie_tun_state.negative);
		DPRINT(" = NEGATIVE MODE =\n");

		INPUT_PAYLOAD1(NEGATIVE_1);
		INPUT_PAYLOAD2(NEGATIVE_2);

		sending_tuning_cmd();
	} else {
		/* check the mode and tuning again when wake up*/
		if (mdnie_locked) {
			mdnie_tun_state.scenario = mdnie_tun_state.real_scenario;
			mDNIe_Set_Mode(mdnie_tun_state.real_scenario);
		 } else {
			mDNIe_Set_Mode(mdnie_tun_state.scenario);
		}
	}
}

static DEVICE_ATTR(negative, 0664,
		   negative_show,
		   negative_store);

static ssize_t playspeed_show(struct device *dev,
			struct device_attribute *attr,
			char *buf)
{
	DPRINT("called %s\n", __func__);
	return snprintf(buf, 256, "%d\n", play_speed_1_5);
}

static ssize_t playspeed_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t size)
{
	int value;
	sscanf(buf, "%d", &value);

	is_play_speed_1_5(value);
	return size;
}

static DEVICE_ATTR(playspeed, 0664,
			playspeed_show,
			playspeed_store);

static ssize_t accessibility_show(struct device *dev,
			struct device_attribute *attr,
			char *buf)
{
	DPRINT("called %s\n", __func__);
	return snprintf(buf, 256, "Current accessibility Value : %s\n",
		accessibility_name[mdnie_tun_state.blind]);
	//return snprintf(buf, 256, "%d\n", play_speed_1_5);
}

static ssize_t accessibility_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t size)
{
	int cmd_value;
	char buffer[MDNIE_COLOR_BLINDE_CMD] = {0,};
	int buffer2[MDNIE_COLOR_BLINDE_CMD/2] = {0,};
	int loop;
	char temp;
	int backup;
	int backup2;

	sscanf(buf, "%d %x %x %x %x %x %x %x %x %x", &cmd_value,
		&buffer2[0], &buffer2[1], &buffer2[2], &buffer2[3], &buffer2[4],
		&buffer2[5], &buffer2[6], &buffer2[7], &buffer2[8]);

	for(loop = 0; loop < MDNIE_COLOR_BLINDE_CMD/2; loop++) {
		buffer2[loop] = buffer2[loop] & 0xFFFF;

		buffer[loop * 2] = (buffer2[loop] & 0xFF00) >> 8;
		buffer[loop * 2 + 1] = buffer2[loop] & 0xFF;
	}

	for(loop = 0; loop < MDNIE_COLOR_BLINDE_CMD; loop+=2) {
		temp = buffer[loop];
		buffer[loop] = buffer[loop + 1];
		buffer[loop + 1] = temp;
	}

	backup = mdnie_tun_state.negative;
	backup2 = mdnie_tun_state.blind;

	if (cmd_value == NEGATIVE) {
		mdnie_tun_state.negative = mDNIe_NEGATIVE_ON;
		mdnie_tun_state.blind = ACCESSIBILITY_OFF;
	} else if (cmd_value == COLOR_BLIND) {
		mdnie_tun_state.negative = mDNIe_NEGATIVE_OFF;
		mdnie_tun_state.blind = COLOR_BLIND;
#if defined(CONFIG_FB_MSM_MIPI_HX8389B_TFT_VIDEO_QHD_PT_PANEL)
		for (loop = 0; loop < MDNIE_COLOR_BLINDE_CMD; loop++){
			COLOR_BLIND_2[28+loop] = buffer[MDNIE_COLOR_BLINDE_CMD - 1 - loop];
		}
#else
		memcpy(&COLOR_BLIND_2[MDNIE_COLOR_BLINDE_CMD],
				buffer, MDNIE_COLOR_BLINDE_CMD);
#endif
	}
#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OCTA_VIDEO_FULL_HD_PT_PANEL) || defined(CONFIG_FB_MSM_MIPI_RENESAS_TFT_VIDEO_FULL_HD_PT_PANEL)
	else if  (cmd_value == DARK_SCREEN) {
		mdnie_tun_state.negative = mDNIe_NEGATIVE_OFF;
		mdnie_tun_state.blind = DARK_SCREEN;
	}
#endif
	else if (cmd_value == ACCESSIBILITY_OFF) {
		mdnie_tun_state.blind = ACCESSIBILITY_OFF;
		mdnie_tun_state.negative = mDNIe_NEGATIVE_OFF;
	} else
		pr_info("%s ACCESSIBILITY_MAX", __func__);

	is_negative_on();

	pr_info("%s cmd_value : %d size : %d", __func__, cmd_value, size);

	return size;
}

static DEVICE_ATTR(accessibility, 0664,
			accessibility_show,
			accessibility_store);

#if defined(CONFIG_FB_MSM_MIPI_RENESAS_TFT_VIDEO_FULL_HD_PT_PANEL)
static ssize_t cabc_show(struct device *dev,
			struct device_attribute *attr,
			char *buf)
{
	DPRINT("called %s\n", __func__);
	return snprintf(buf, 256, "%d\n", cabc);
}

static ssize_t cabc_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t size)
{
	int value;

	sscanf(buf, "%d", &value);

	cabc = value? 1: 0;
	mipi_samsung_cabc_onoff ( cabc );

	DPRINT ( "cabc_store, input value = %d\n", value);

	return size;
}
#if defined(CONFIG_FB_MSM_MIPI_RENESAS_TFT_VIDEO_FULL_HD_PT_PANEL)
int is_cabc_on ( void )
{
	return cabc;
}
#endif
static DEVICE_ATTR(cabc, 0664,
			cabc_show,
			cabc_store);
#endif

#if defined(CONFIG_MDNIE_LITE_CONTROL)
/* hijack */

static ssize_t hijack_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", hijack);
}

static ssize_t hijack_store(struct device * dev, struct device_attribute * attr, const char * buf, size_t size)
{
	int new_val;
	sscanf(buf, "%d", &new_val);

	switch (new_val) {
		case HIJACK_DISABLED:
		case HIJACK_ENABLED:
			hijack = new_val;
		if (mdnie_locked) {
			mdnie_tun_state.scenario = mdnie_tun_state.real_scenario;
			mDNIe_Set_Mode(mdnie_tun_state.real_scenario);
		 } else {
			mDNIe_Set_Mode(mdnie_tun_state.scenario);
		}
			return size;
		default:
			return -EINVAL;
	}
}

static ssize_t locked_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", mdnie_locked);
}

static ssize_t locked_store(struct device * dev, struct device_attribute * attr, const char * buf, size_t size)
{
	int new_val;
	sscanf(buf, "%d", &new_val);

	sanitize_min_max(new_val, 0, 1);

	mdnie_locked = new_val;

		if (mdnie_locked) {
			mdnie_tun_state.scenario = mdnie_tun_state.real_scenario;
			mDNIe_Set_Mode(mdnie_tun_state.real_scenario);
		 } else {
			mDNIe_Set_Mode(mdnie_tun_state.scenario);
		}

	return size;
}

/* curve */

static ssize_t curve_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", curve);
}

static ssize_t curve_store(struct device * dev, struct device_attribute * attr, const char * buf, size_t size)
{
	int new_val;
	sscanf(buf, "%d", &new_val);

	if (new_val == curve)
			return size;

	switch (new_val) {
		case DYNAMIC_MODE:
		case STANDARD_MODE:
		case NATURAL_MODE:
		case MOVIE_MODE:
		case AUTO_MODE:
			curve = new_val;
			update_mdnie_curve();
			if (hijack == HIJACK_ENABLED) {
				if (mdnie_locked) {
					mdnie_tun_state.scenario = mdnie_tun_state.real_scenario;
					mDNIe_Set_Mode(mdnie_tun_state.real_scenario);
				 } else {
					mDNIe_Set_Mode(mdnie_tun_state.scenario);
				}
			}
			return size;
		default:
			return -EINVAL;
	}
}

/* copy_mode */

static ssize_t copy_mode_store(struct device * dev, struct device_attribute * attr, const char * buf, size_t size)
{
	int new_val;
	sscanf(buf, "%d", &new_val);

	switch (new_val) {
		case DYNAMIC_MODE:
		case STANDARD_MODE:
#if !defined(CONFIG_SUPPORT_DISPLAY_OCTA_TFT)
		case NATURAL_MODE:
#endif
		case MOVIE_MODE:
		case AUTO_MODE:
			curve = new_val;
			update_mdnie_mode();
			if (hijack == HIJACK_ENABLED) {
				if (mdnie_locked) {
					mdnie_tun_state.scenario = mdnie_tun_state.real_scenario;
					mDNIe_Set_Mode(mdnie_tun_state.real_scenario);
				 } else {
					mDNIe_Set_Mode(mdnie_tun_state.scenario);
				}
			}
			return size;
		default:
			return -EINVAL;
	}
}

/* sharpen */

static ssize_t sharpen_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", LITE_CONTROL_1[4]);
}

static ssize_t sharpen_store(struct device * dev, struct device_attribute * attr, const char * buf, size_t size)
{
	int new_val;
	sscanf(buf, "%d", &new_val);

	if (new_val != LITE_CONTROL_1[4]) {
		if (new_val < 0 || new_val > 11)
			return -EINVAL;
		DPRINT("new sharpen: %d\n", new_val);
		LITE_CONTROL_1[4] = new_val;
			if (hijack == HIJACK_ENABLED) {
				if (mdnie_locked) {
					mdnie_tun_state.scenario = mdnie_tun_state.real_scenario;
					mDNIe_Set_Mode(mdnie_tun_state.real_scenario);
				 } else {
					mDNIe_Set_Mode(mdnie_tun_state.scenario);
				}
			}
	}
	return size;
}

/* red */

static ssize_t red_red_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", LITE_CONTROL_2[19]);
}

static ssize_t red_red_store(struct device * dev, struct device_attribute * attr, const char * buf, size_t size)
{
	int new_val;
	sscanf(buf, "%d", &new_val);

	if (new_val != LITE_CONTROL_2[19]) {
		if (new_val < 0 || new_val > 255)
			return -EINVAL;
		DPRINT("new red_red: %d\n", new_val);
		LITE_CONTROL_2[19] = new_val;
			if (hijack == HIJACK_ENABLED) {
				if (mdnie_locked) {
					mdnie_tun_state.scenario = mdnie_tun_state.real_scenario;
					mDNIe_Set_Mode(mdnie_tun_state.real_scenario);
				 } else {
					mDNIe_Set_Mode(mdnie_tun_state.scenario);
				}
			}
	}
	return size;
}

static ssize_t red_green_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", LITE_CONTROL_2[21]);
}

static ssize_t red_green_store(struct device * dev, struct device_attribute * attr, const char * buf, size_t size)
{
	int new_val;
	sscanf(buf, "%d", &new_val);

	if (new_val != LITE_CONTROL_2[21]) {
		if (new_val < 0 || new_val > 255)
			return -EINVAL;
		DPRINT("new red_green: %d\n", new_val);
		LITE_CONTROL_2[21] = new_val;
			if (hijack == HIJACK_ENABLED) {
				if (mdnie_locked) {
					mdnie_tun_state.scenario = mdnie_tun_state.real_scenario;
					mDNIe_Set_Mode(mdnie_tun_state.real_scenario);
				 } else {
					mDNIe_Set_Mode(mdnie_tun_state.scenario);
				}
			}
	}
	return size;
}

static ssize_t red_blue_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", LITE_CONTROL_2[23]);
}

static ssize_t red_blue_store(struct device * dev, struct device_attribute * attr, const char * buf, size_t size)
{
	int new_val;
	sscanf(buf, "%d", &new_val);

	if (new_val != LITE_CONTROL_2[23]) {
		if (new_val < 0 || new_val > 255)
			return -EINVAL;
		DPRINT("new red_blue: %d\n", new_val);
		LITE_CONTROL_2[23] = new_val;
			if (hijack == HIJACK_ENABLED) {
				if (mdnie_locked) {
					mdnie_tun_state.scenario = mdnie_tun_state.real_scenario;
					mDNIe_Set_Mode(mdnie_tun_state.real_scenario);
				 } else {
					mDNIe_Set_Mode(mdnie_tun_state.scenario);
				}
			}
	}
	return size;
}

/* cyan */

static ssize_t cyan_red_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", LITE_CONTROL_2[18]);
}

static ssize_t cyan_red_store(struct device * dev, struct device_attribute * attr, const char * buf, size_t size)
{
	int new_val;
	sscanf(buf, "%d", &new_val);

	if (new_val != LITE_CONTROL_2[18]) {
		if (new_val < 0 || new_val > 255)
			return -EINVAL;
		DPRINT("new cyan_red: %d\n", new_val);
		LITE_CONTROL_2[18] = new_val;
			if (hijack == HIJACK_ENABLED) {
				if (mdnie_locked) {
					mdnie_tun_state.scenario = mdnie_tun_state.real_scenario;
					mDNIe_Set_Mode(mdnie_tun_state.real_scenario);
				 } else {
					mDNIe_Set_Mode(mdnie_tun_state.scenario);
				}
			}
	}
	return size;
}

static ssize_t cyan_green_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", LITE_CONTROL_2[20]);
}

static ssize_t cyan_green_store(struct device * dev, struct device_attribute * attr, const char * buf, size_t size)
{
    int new_val;
	sscanf(buf, "%d", &new_val);

	if (new_val != LITE_CONTROL_2[20]) {
		if (new_val < 0 || new_val > 255)
			return -EINVAL;
		DPRINT("new cyan_green: %d\n", new_val);
		LITE_CONTROL_2[20] = new_val;
			if (hijack == HIJACK_ENABLED) {
				if (mdnie_locked) {
					mdnie_tun_state.scenario = mdnie_tun_state.real_scenario;
					mDNIe_Set_Mode(mdnie_tun_state.real_scenario);
				 } else {
					mDNIe_Set_Mode(mdnie_tun_state.scenario);
				}
			}
	}
	return size;
}

static ssize_t cyan_blue_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", LITE_CONTROL_2[22]);
}

static ssize_t cyan_blue_store(struct device * dev, struct device_attribute * attr, const char * buf, size_t size)
{
	int new_val;
	sscanf(buf, "%d", &new_val);

	if (new_val != LITE_CONTROL_2[22]) {
		if (new_val < 0 || new_val > 255)
			return -EINVAL;
		DPRINT("new cyan_blue: %d\n", new_val);
		LITE_CONTROL_2[22] = new_val;
			if (hijack == HIJACK_ENABLED) {
				if (mdnie_locked) {
					mdnie_tun_state.scenario = mdnie_tun_state.real_scenario;
					mDNIe_Set_Mode(mdnie_tun_state.real_scenario);
				 } else {
					mDNIe_Set_Mode(mdnie_tun_state.scenario);
				}
			}
	}
	return size;
}

/* green */

static ssize_t green_red_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", LITE_CONTROL_2[25]);
}

static ssize_t green_red_store(struct device * dev, struct device_attribute * attr, const char * buf, size_t size)
{
	int new_val;
	sscanf(buf, "%d", &new_val);

	if (new_val != LITE_CONTROL_2[25]) {
		if (new_val < 0 || new_val > 255)
			return -EINVAL;
		DPRINT("new green_red: %d\n", new_val);
		LITE_CONTROL_2[25] = new_val;
			if (hijack == HIJACK_ENABLED) {
				if (mdnie_locked) {
					mdnie_tun_state.scenario = mdnie_tun_state.real_scenario;
					mDNIe_Set_Mode(mdnie_tun_state.real_scenario);
				 } else {
					mDNIe_Set_Mode(mdnie_tun_state.scenario);
				}
			}
	}
	return size;
}

static ssize_t green_green_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", LITE_CONTROL_2[27]);
}

static ssize_t green_green_store(struct device * dev, struct device_attribute * attr, const char * buf, size_t size)
{
	int new_val;
	sscanf(buf, "%d", &new_val);

	if (new_val != LITE_CONTROL_2[27]) {
		if (new_val < 0 || new_val > 255)
			return -EINVAL;
		DPRINT("new green_green: %d\n", new_val);
		LITE_CONTROL_2[27] = new_val;
			if (hijack == HIJACK_ENABLED) {
				if (mdnie_locked) {
					mdnie_tun_state.scenario = mdnie_tun_state.real_scenario;
					mDNIe_Set_Mode(mdnie_tun_state.real_scenario);
				 } else {
					mDNIe_Set_Mode(mdnie_tun_state.scenario);
				}
			}
	}
	return size;
}

static ssize_t green_blue_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", LITE_CONTROL_2[29]);
}

static ssize_t green_blue_store(struct device * dev, struct device_attribute * attr, const char * buf, size_t size)
{
	int new_val;
	sscanf(buf, "%d", &new_val);

	if (new_val != LITE_CONTROL_2[29]) {
		if (new_val < 0 || new_val > 255)
			return -EINVAL;
		DPRINT("new green_blue: %d\n", new_val);
		LITE_CONTROL_2[29] = new_val;
			if (hijack == HIJACK_ENABLED) {
				if (mdnie_locked) {
					mdnie_tun_state.scenario = mdnie_tun_state.real_scenario;
					mDNIe_Set_Mode(mdnie_tun_state.real_scenario);
				 } else {
					mDNIe_Set_Mode(mdnie_tun_state.scenario);
				}
			}
	}
	return size;
}

/* magenta */

static ssize_t magenta_red_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", LITE_CONTROL_2[24]);
}

static ssize_t magenta_red_store(struct device * dev, struct device_attribute * attr, const char * buf, size_t size)
{
	int new_val;
	sscanf(buf, "%d", &new_val);

	if (new_val != LITE_CONTROL_2[24]) {
		if (new_val < 0 || new_val > 255)
			return -EINVAL;
		DPRINT("new magenta_red: %d\n", new_val);
		LITE_CONTROL_2[24] = new_val;
			if (hijack == HIJACK_ENABLED) {
				if (mdnie_locked) {
					mdnie_tun_state.scenario = mdnie_tun_state.real_scenario;
					mDNIe_Set_Mode(mdnie_tun_state.real_scenario);
				 } else {
					mDNIe_Set_Mode(mdnie_tun_state.scenario);
				}
			}
	}
	return size;
}

static ssize_t magenta_green_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", LITE_CONTROL_2[26]);
}

static ssize_t magenta_green_store(struct device * dev, struct device_attribute * attr, const char * buf, size_t size)
{
	int new_val;
	sscanf(buf, "%d", &new_val);

	if (new_val != LITE_CONTROL_2[26]) {
		if (new_val < 0 || new_val > 255)
			return -EINVAL;
		DPRINT("new magenta_green: %d\n", new_val);
		LITE_CONTROL_2[26] = new_val;
			if (hijack == HIJACK_ENABLED) {
				if (mdnie_locked) {
					mdnie_tun_state.scenario = mdnie_tun_state.real_scenario;
					mDNIe_Set_Mode(mdnie_tun_state.real_scenario);
				 } else {
					mDNIe_Set_Mode(mdnie_tun_state.scenario);
				}
			}
	}
	return size;
}

static ssize_t magenta_blue_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", LITE_CONTROL_2[28]);
}

static ssize_t magenta_blue_store(struct device * dev, struct device_attribute * attr, const char * buf, size_t size)
{
	int new_val;
	sscanf(buf, "%d", &new_val);

	if (new_val != LITE_CONTROL_2[28]) {
		if (new_val < 0 || new_val > 255)
			return -EINVAL;
		DPRINT("new magenta_blue: %d\n", new_val);
		LITE_CONTROL_2[28] = new_val;
			if (hijack == HIJACK_ENABLED) {
				if (mdnie_locked) {
					mdnie_tun_state.scenario = mdnie_tun_state.real_scenario;
					mDNIe_Set_Mode(mdnie_tun_state.real_scenario);
				 } else {
					mDNIe_Set_Mode(mdnie_tun_state.scenario);
				}
			}
	}
	return size;
}

/* blue */

static ssize_t blue_red_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", LITE_CONTROL_2[31]);
}

static ssize_t blue_red_store(struct device * dev, struct device_attribute * attr, const char * buf, size_t size)
{
	int new_val;
	sscanf(buf, "%d", &new_val);

	if (new_val != LITE_CONTROL_2[31]) {
		if (new_val < 0 || new_val > 255)
			return -EINVAL;
		DPRINT("new blue_red: %d\n", new_val);
		LITE_CONTROL_2[31] = new_val;
			if (hijack == HIJACK_ENABLED) {
				if (mdnie_locked) {
					mdnie_tun_state.scenario = mdnie_tun_state.real_scenario;
					mDNIe_Set_Mode(mdnie_tun_state.real_scenario);
				 } else {
					mDNIe_Set_Mode(mdnie_tun_state.scenario);
				}
			}
	}
	return size;
}

static ssize_t blue_green_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", LITE_CONTROL_2[33]);
}

static ssize_t blue_green_store(struct device * dev, struct device_attribute * attr, const char * buf, size_t size)
{
	int new_val;
	sscanf(buf, "%d", &new_val);

	if (new_val != LITE_CONTROL_2[33]) {
		if (new_val < 0 || new_val > 255)
			return -EINVAL;
		DPRINT("new blue_green: %d\n", new_val);
		LITE_CONTROL_2[33] = new_val;
			if (hijack == HIJACK_ENABLED) {
				if (mdnie_locked) {
					mdnie_tun_state.scenario = mdnie_tun_state.real_scenario;
					mDNIe_Set_Mode(mdnie_tun_state.real_scenario);
				 } else {
					mDNIe_Set_Mode(mdnie_tun_state.scenario);
				}
			}
	}
	return size;
}

static ssize_t blue_blue_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", LITE_CONTROL_2[35]);
}

static ssize_t blue_blue_store(struct device * dev, struct device_attribute * attr, const char * buf, size_t size)
{
	int new_val;
	sscanf(buf, "%d", &new_val);

	if (new_val != LITE_CONTROL_2[35]) {
		if (new_val < 0 || new_val > 255)
			return -EINVAL;
		DPRINT("new blue_blue: %d\n", new_val);
		LITE_CONTROL_2[35] = new_val;
			if (hijack == HIJACK_ENABLED) {
				if (mdnie_locked) {
					mdnie_tun_state.scenario = mdnie_tun_state.real_scenario;
					mDNIe_Set_Mode(mdnie_tun_state.real_scenario);
				 } else {
					mDNIe_Set_Mode(mdnie_tun_state.scenario);
				}
			}
	}
	return size;
}

/* yellow */

static ssize_t yellow_red_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", LITE_CONTROL_2[30]);
}

static ssize_t yellow_red_store(struct device * dev, struct device_attribute * attr, const char * buf, size_t size)
{
	int new_val;
	sscanf(buf, "%d", &new_val);

	if (new_val != LITE_CONTROL_2[30]) {
		if (new_val < 0 || new_val > 255)
			return -EINVAL;
		DPRINT("new yellow_red: %d\n", new_val);
		LITE_CONTROL_2[30] = new_val;
			if (hijack == HIJACK_ENABLED) {
				if (mdnie_locked) {
					mdnie_tun_state.scenario = mdnie_tun_state.real_scenario;
					mDNIe_Set_Mode(mdnie_tun_state.real_scenario);
				 } else {
					mDNIe_Set_Mode(mdnie_tun_state.scenario);
				}
			}
	}
	return size;
}

static ssize_t yellow_green_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", LITE_CONTROL_2[32]);
}

static ssize_t yellow_green_store(struct device * dev, struct device_attribute * attr, const char * buf, size_t size)
{
	int new_val;
	sscanf(buf, "%d", &new_val);

	if (new_val != LITE_CONTROL_2[32]) {
		if (new_val < 0 || new_val > 255)
			return -EINVAL;
		DPRINT("new yellow_green: %d\n", new_val);
		LITE_CONTROL_2[32] = new_val;
			if (hijack == HIJACK_ENABLED) {
				if (mdnie_locked) {
					mdnie_tun_state.scenario = mdnie_tun_state.real_scenario;
					mDNIe_Set_Mode(mdnie_tun_state.real_scenario);
				 } else {
					mDNIe_Set_Mode(mdnie_tun_state.scenario);
				}
			}
	}
	return size;
}

static ssize_t yellow_blue_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", LITE_CONTROL_2[34]);
}

static ssize_t yellow_blue_store(struct device * dev, struct device_attribute * attr, const char * buf, size_t size)
{
	int new_val;
	sscanf(buf, "%d", &new_val);

	if (new_val != LITE_CONTROL_2[34]) {
		if (new_val < 0 || new_val > 255)
			return -EINVAL;
		DPRINT("new yellow_blue: %d\n", new_val);
		LITE_CONTROL_2[34] = new_val;
			if (hijack == HIJACK_ENABLED) {
				if (mdnie_locked) {
					mdnie_tun_state.scenario = mdnie_tun_state.real_scenario;
					mDNIe_Set_Mode(mdnie_tun_state.real_scenario);
				 } else {
					mDNIe_Set_Mode(mdnie_tun_state.scenario);
				}
			}
	}
	return size;
}

/* black */

static ssize_t black_crush_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", black);
}

static ssize_t black_crush_store(struct device * dev, struct device_attribute * attr, const char * buf, size_t size)
{
	int new_val;
	sscanf(buf, "%d", &new_val);

	if (new_val != black) {
		if (new_val < -128 || new_val > 128)
			return -EINVAL;
		DPRINT("new black: %d\n", new_val);
		black = new_val;
		LITE_CONTROL_2[37] = max(0,min(255, black_r + black));
		LITE_CONTROL_2[39] = max(0,min(255, black_g + black));
		LITE_CONTROL_2[41] = max(0,min(255, black_b + black));
			if (hijack == HIJACK_ENABLED) {
				if (mdnie_locked) {
					mdnie_tun_state.scenario = mdnie_tun_state.real_scenario;
					mDNIe_Set_Mode(mdnie_tun_state.real_scenario);
				 } else {
					mDNIe_Set_Mode(mdnie_tun_state.scenario);
				}
			}
	}
	return size;
}

static ssize_t black_red_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", black_r);
}

static ssize_t black_red_store(struct device * dev, struct device_attribute * attr, const char * buf, size_t size)
{
	int new_val;
	sscanf(buf, "%d", &new_val);

	if (new_val != black_r) {
		if (new_val < 0 || new_val > 255)
			return -EINVAL;
		DPRINT("new black_red: %d\n", new_val);
		black_r = new_val;
		LITE_CONTROL_2[37] = max(0,min(255, black_r + black));
			if (hijack == HIJACK_ENABLED) {
				if (mdnie_locked) {
					mdnie_tun_state.scenario = mdnie_tun_state.real_scenario;
					mDNIe_Set_Mode(mdnie_tun_state.real_scenario);
				 } else {
					mDNIe_Set_Mode(mdnie_tun_state.scenario);
				}
			}
	}
	return size;
}

static ssize_t black_green_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", black_g);
}

static ssize_t black_green_store(struct device * dev, struct device_attribute * attr, const char * buf, size_t size)
{
	int new_val;
	sscanf(buf, "%d", &new_val);

	if (new_val != black_g) {
		if (new_val < 0 || new_val > 255)
			return -EINVAL;
		DPRINT("new black_green: %d\n", new_val);
		black_g = new_val;
		LITE_CONTROL_2[39] = max(0,min(255, black_g + black));
			if (hijack == HIJACK_ENABLED) {
				if (mdnie_locked) {
					mdnie_tun_state.scenario = mdnie_tun_state.real_scenario;
					mDNIe_Set_Mode(mdnie_tun_state.real_scenario);
				 } else {
					mDNIe_Set_Mode(mdnie_tun_state.scenario);
				}
			}
	}
	return size;
}

static ssize_t black_blue_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", max(0, black_b));
}

static ssize_t black_blue_store(struct device * dev, struct device_attribute * attr, const char * buf, size_t size)
{
	int new_val;
	sscanf(buf, "%d", &new_val);

	if (new_val != black_b) {
		if (new_val < 0 || new_val > 255)
			return -EINVAL;
		DPRINT("new black_blue: %d\n", new_val);
		black_b = new_val;
		LITE_CONTROL_2[41] = max(0,min(255, black_b + black));
			if (hijack == HIJACK_ENABLED) {
				if (mdnie_locked) {
					mdnie_tun_state.scenario = mdnie_tun_state.real_scenario;
					mDNIe_Set_Mode(mdnie_tun_state.real_scenario);
				 } else {
					mDNIe_Set_Mode(mdnie_tun_state.scenario);
				}
			}
	}
	return size;
}

/* white */

static ssize_t white_red_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", LITE_CONTROL_2[36]);
}

static ssize_t white_red_store(struct device * dev, struct device_attribute * attr, const char * buf, size_t size)
{
	int new_val;
	sscanf(buf, "%d", &new_val);

	if (new_val != LITE_CONTROL_2[36]) {
		if (new_val < 0 || new_val > 255)
			return -EINVAL;
		DPRINT("new white_red: %d\n", new_val);
		LITE_CONTROL_2[36] = new_val;
			if (hijack == HIJACK_ENABLED) {
				if (mdnie_locked) {
					mdnie_tun_state.scenario = mdnie_tun_state.real_scenario;
					mDNIe_Set_Mode(mdnie_tun_state.real_scenario);
				 } else {
					mDNIe_Set_Mode(mdnie_tun_state.scenario);
				}
			}
	}
	return size;
}

static ssize_t white_green_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", LITE_CONTROL_2[38]);
}

static ssize_t white_green_store(struct device * dev, struct device_attribute * attr, const char * buf, size_t size)
{
	int new_val;
	sscanf(buf, "%d", &new_val);

	if (new_val != LITE_CONTROL_2[38]) {
		if (new_val < 0 || new_val > 255)
			return -EINVAL;
		DPRINT("new white_green: %d\n", new_val);
		LITE_CONTROL_2[38] = new_val;
			if (hijack == HIJACK_ENABLED) {
				if (mdnie_locked) {
					mdnie_tun_state.scenario = mdnie_tun_state.real_scenario;
					mDNIe_Set_Mode(mdnie_tun_state.real_scenario);
				 } else {
					mDNIe_Set_Mode(mdnie_tun_state.scenario);
				}
			}
	}
	return size;
}

static ssize_t white_blue_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", LITE_CONTROL_2[40]);
}

static ssize_t white_blue_store(struct device * dev, struct device_attribute * attr, const char * buf, size_t size)
{
	int new_val;
	sscanf(buf, "%d", &new_val);

	if (new_val != LITE_CONTROL_2[40]) {
		if (new_val < 0 || new_val > 255)
			return -EINVAL;
		DPRINT("new white_blue: %d\n", new_val);
		LITE_CONTROL_2[40] = new_val;
			if (hijack == HIJACK_ENABLED) {
				if (mdnie_locked) {
					mdnie_tun_state.scenario = mdnie_tun_state.real_scenario;
					mDNIe_Set_Mode(mdnie_tun_state.real_scenario);
				 } else {
					mDNIe_Set_Mode(mdnie_tun_state.scenario);
				}
			}
	}
	return size;
}

static DEVICE_ATTR(hijack, 0666, hijack_show, hijack_store);
static DEVICE_ATTR(locked, 0666, locked_show, locked_store);
static DEVICE_ATTR(curve, 0666, curve_show, curve_store);
static DEVICE_ATTR(copy_mode, 0222, NULL, copy_mode_store);
static DEVICE_ATTR(sharpen, 0666, sharpen_show, sharpen_store);
static DEVICE_ATTR(red_red, 0666, red_red_show, red_red_store);
static DEVICE_ATTR(red_green, 0666, red_green_show, red_green_store);
static DEVICE_ATTR(red_blue, 0666, red_blue_show, red_blue_store);
static DEVICE_ATTR(cyan_red, 0666, cyan_red_show, cyan_red_store);
static DEVICE_ATTR(cyan_green, 0666, cyan_green_show, cyan_green_store);
static DEVICE_ATTR(cyan_blue, 0666, cyan_blue_show, cyan_blue_store);
static DEVICE_ATTR(green_red, 0666, green_red_show, green_red_store);
static DEVICE_ATTR(green_green, 0666, green_green_show, green_green_store);
static DEVICE_ATTR(green_blue, 0666, green_blue_show, green_blue_store);
static DEVICE_ATTR(magenta_red, 0666, magenta_red_show, magenta_red_store);
static DEVICE_ATTR(magenta_green, 0666, magenta_green_show, magenta_green_store);
static DEVICE_ATTR(magenta_blue, 0666, magenta_blue_show, magenta_blue_store);
static DEVICE_ATTR(blue_red, 0666, blue_red_show, blue_red_store);
static DEVICE_ATTR(blue_green, 0666, blue_green_show, blue_green_store);
static DEVICE_ATTR(blue_blue, 0666, blue_blue_show, blue_blue_store);
static DEVICE_ATTR(yellow_red, 0666, yellow_red_show, yellow_red_store);
static DEVICE_ATTR(yellow_green, 0666, yellow_green_show, yellow_green_store);
static DEVICE_ATTR(yellow_blue, 0666, yellow_blue_show, yellow_blue_store);
static DEVICE_ATTR(black, 0666, black_crush_show, black_crush_store);
static DEVICE_ATTR(black_red, 0666, black_red_show, black_red_store);
static DEVICE_ATTR(black_green, 0666, black_green_show, black_green_store);
static DEVICE_ATTR(black_blue, 0666, black_blue_show, black_blue_store);
static DEVICE_ATTR(white_red, 0666, white_red_show, white_red_store);
static DEVICE_ATTR(white_green, 0666, white_green_show, white_green_store);
static DEVICE_ATTR(white_blue, 0666, white_blue_show, white_blue_store);
#endif

/* -------------------------------------------------------
 * Add whathub's new interface
 *
 * NB: Current interface is kept as more app friendly.
 *
 * (Yank555.lu)
 * ------------------------------------------------------- */
#if defined(CONFIG_MDNIE_LITE_CONTROL)
/* red */

static ssize_t red_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d %d %d\n", LITE_CONTROL_2[19], LITE_CONTROL_2[21], LITE_CONTROL_2[23]);
}

static ssize_t red_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    int red, green, blue;
	sscanf(buf, "%d %d %d", &red, &green, &blue);

	if (red != LITE_CONTROL_2[19] || green != LITE_CONTROL_2[21] || blue != LITE_CONTROL_2[23]) {
		if (red < 0 || red > 255 || green < 0 || green > 255 || blue < 0 || blue > 255)
			return -EINVAL;
		DPRINT("[RED] (red: %d) (green: %d) (blue: %d)\n", red, green, blue);
		LITE_CONTROL_2[19] = red;
		LITE_CONTROL_2[21] = green;
		LITE_CONTROL_2[23] = blue;
			if (hijack == HIJACK_ENABLED) {
				if (mdnie_locked) {
					mdnie_tun_state.scenario = mdnie_tun_state.real_scenario;
					mDNIe_Set_Mode(mdnie_tun_state.real_scenario);
				 } else {
					mDNIe_Set_Mode(mdnie_tun_state.scenario);
				}
			}
	}
    return size;
}

/* green */

static ssize_t green_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d %d %d\n", LITE_CONTROL_2[25], LITE_CONTROL_2[27], LITE_CONTROL_2[29]);
}

static ssize_t green_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    int red, green, blue;
	sscanf(buf, "%d %d %d", &red, &green, &blue);

	if (red != LITE_CONTROL_2[25] || green != LITE_CONTROL_2[27] || blue != LITE_CONTROL_2[29]) {
		if (red < 0 || red > 255 || green < 0 || green > 255 || blue < 0 || blue > 255)
			return -EINVAL;
		DPRINT("[GREEN] (red: %d) (green: %d) (blue: %d)\n", red, green, blue);
		LITE_CONTROL_2[25] = red;
		LITE_CONTROL_2[27] = green;
		LITE_CONTROL_2[29] = blue;
			if (hijack == HIJACK_ENABLED) {
				if (mdnie_locked) {
					mdnie_tun_state.scenario = mdnie_tun_state.real_scenario;
					mDNIe_Set_Mode(mdnie_tun_state.real_scenario);
				 } else {
					mDNIe_Set_Mode(mdnie_tun_state.scenario);
				}
			}
	}
    return size;
}

/* blue */

static ssize_t blue_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d %d %d\n", LITE_CONTROL_2[31], LITE_CONTROL_2[33], LITE_CONTROL_2[35]);
}

static ssize_t blue_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    int red, green, blue;
	sscanf(buf, "%d %d %d", &red, &green, &blue);

	if (red != LITE_CONTROL_2[31] || green != LITE_CONTROL_2[33] || blue != LITE_CONTROL_2[35]) {
		if (red < 0 || red > 255 || green < 0 || green > 255 || blue < 0 || blue > 255)
			return -EINVAL;
		DPRINT("[BLUE] (red: %d) (green: %d) (blue: %d)\n", red, green, blue);
		LITE_CONTROL_2[31] = red;
		LITE_CONTROL_2[33] = green;
		LITE_CONTROL_2[35] = blue;
			if (hijack == HIJACK_ENABLED) {
				if (mdnie_locked) {
					mdnie_tun_state.scenario = mdnie_tun_state.real_scenario;
					mDNIe_Set_Mode(mdnie_tun_state.real_scenario);
				 } else {
					mDNIe_Set_Mode(mdnie_tun_state.scenario);
				}
			}
	}
    return size;
}

/* cyan */

static ssize_t cyan_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d %d %d\n", LITE_CONTROL_2[18], LITE_CONTROL_2[20], LITE_CONTROL_2[22]);
}

static ssize_t cyan_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    int red, green, blue;
	sscanf(buf, "%d %d %d", &red, &green, &blue);

	if (red != LITE_CONTROL_2[18] || green != LITE_CONTROL_2[20] || blue != LITE_CONTROL_2[22]) {
		if (red < 0 || red > 255 || green < 0 || green > 255 || blue < 0 || blue > 255)
			return -EINVAL;
		DPRINT("[CYAN] (red: %d) (green: %d) (blue: %d)\n", red, green, blue);
		LITE_CONTROL_2[18] = red;
		LITE_CONTROL_2[20] = green;
		LITE_CONTROL_2[22] = blue;
			if (hijack == HIJACK_ENABLED) {
				if (mdnie_locked) {
					mdnie_tun_state.scenario = mdnie_tun_state.real_scenario;
					mDNIe_Set_Mode(mdnie_tun_state.real_scenario);
				 } else {
					mDNIe_Set_Mode(mdnie_tun_state.scenario);
				}
			}
	}
    return size;
}

/* magenta */

static ssize_t magenta_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d %d %d\n", LITE_CONTROL_2[24], LITE_CONTROL_2[26], LITE_CONTROL_2[28]);
}

static ssize_t magenta_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    int red, green, blue;
	sscanf(buf, "%d %d %d", &red, &green, &blue);

	if (red != LITE_CONTROL_2[24] || green != LITE_CONTROL_2[26] || blue != LITE_CONTROL_2[28]) {
		if (red < 0 || red > 255 || green < 0 || green > 255 || blue < 0 || blue > 255)
			return -EINVAL;
		DPRINT("[MAGENTA] (red: %d) (green: %d) (blue: %d)\n", red, green, blue);
		LITE_CONTROL_2[24] = red;
		LITE_CONTROL_2[26] = green;
		LITE_CONTROL_2[28] = blue;
			if (hijack == HIJACK_ENABLED) {
				if (mdnie_locked) {
					mdnie_tun_state.scenario = mdnie_tun_state.real_scenario;
					mDNIe_Set_Mode(mdnie_tun_state.real_scenario);
				 } else {
					mDNIe_Set_Mode(mdnie_tun_state.scenario);
				}
			}
	}
    return size;
}

/* yellow */

static ssize_t yellow_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d %d %d\n", LITE_CONTROL_2[30], LITE_CONTROL_2[32], LITE_CONTROL_2[34]);
}

static ssize_t yellow_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    int red, green, blue;
	sscanf(buf, "%d %d %d", &red, &green, &blue);

	if (red != LITE_CONTROL_2[30] || green != LITE_CONTROL_2[32] || blue != LITE_CONTROL_2[34]) {
		if (red < 0 || red > 255 || green < 0 || green > 255 || blue < 0 || blue > 255)
			return -EINVAL;
		DPRINT("[YELLOW] (red: %d) (green: %d) (blue: %d)\n", red, green, blue);
		LITE_CONTROL_2[30] = red;
		LITE_CONTROL_2[32] = green;
		LITE_CONTROL_2[34] = blue;
			if (hijack == HIJACK_ENABLED) {
				if (mdnie_locked) {
					mdnie_tun_state.scenario = mdnie_tun_state.real_scenario;
					mDNIe_Set_Mode(mdnie_tun_state.real_scenario);
				 } else {
					mDNIe_Set_Mode(mdnie_tun_state.scenario);
				}
			}
	}
    return size;
}

/* white */

static ssize_t white_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d %d %d\n", LITE_CONTROL_2[36], LITE_CONTROL_2[38], LITE_CONTROL_2[40]);
}

static ssize_t white_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    int red, green, blue;
	sscanf(buf, "%d %d %d", &red, &green, &blue);

	if (red != LITE_CONTROL_2[36] || green != LITE_CONTROL_2[38] || blue != LITE_CONTROL_2[40]) {
		if (red < 0 || red > 255 || green < 0 || green > 255 || blue < 0 || blue > 255)
			return -EINVAL;
		DPRINT("[WHITE] (red: %d) (green: %d) (blue: %d)\n", red, green, blue);
		LITE_CONTROL_2[36] = red;
		LITE_CONTROL_2[38] = green;
		LITE_CONTROL_2[40] = blue;
			if (hijack == HIJACK_ENABLED) {
				if (mdnie_locked) {
					mdnie_tun_state.scenario = mdnie_tun_state.real_scenario;
					mDNIe_Set_Mode(mdnie_tun_state.real_scenario);
				 } else {
					mDNIe_Set_Mode(mdnie_tun_state.scenario);
				}
			}
	}
    return size;
}

/* black */

static ssize_t black_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d %d %d\n", LITE_CONTROL_2[37], LITE_CONTROL_2[39], LITE_CONTROL_2[41]);
}

static ssize_t black_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    int red, green, blue;
	sscanf(buf, "%d %d %d", &red, &green, &blue);

	if (red != LITE_CONTROL_2[37] || green != LITE_CONTROL_2[39] || blue != LITE_CONTROL_2[41]) {
		if (red < 0 || red > 255 || green < 0 || green > 255 || blue < 0 || blue > 255)
			return -EINVAL;
		DPRINT("[BLACK] (red: %d) (green: %d) (blue: %d)\n", red, green, blue);
		LITE_CONTROL_2[37] = red;
		LITE_CONTROL_2[39] = green;
		LITE_CONTROL_2[41] = blue;
			if (hijack == HIJACK_ENABLED) {
				if (mdnie_locked) {
					mdnie_tun_state.scenario = mdnie_tun_state.real_scenario;
					mDNIe_Set_Mode(mdnie_tun_state.real_scenario);
				 } else {
					mDNIe_Set_Mode(mdnie_tun_state.scenario);
				}
			}
	}
    return size;
}

static ssize_t version_show(struct device *dev, struct device_attribute *attr, char *buf)
{
        return sprintf(buf, "%s\n", MDNIE_VERSION);
}

static DEVICE_ATTR(control_override, 0666, hijack_show, hijack_store);
static DEVICE_ATTR(control_sharpen, 0666, sharpen_show, sharpen_store);
static DEVICE_ATTR(control_red, 0666, red_show, red_store);
static DEVICE_ATTR(control_green, 0666, green_show, green_store);
static DEVICE_ATTR(control_blue, 0666, blue_show, blue_store);
static DEVICE_ATTR(control_cyan, 0666, cyan_show, cyan_store);
static DEVICE_ATTR(control_magenta, 0666, magenta_show, magenta_store);
static DEVICE_ATTR(control_yellow, 0666, yellow_show, yellow_store);
static DEVICE_ATTR(control_white, 0666, white_show, white_store);
static DEVICE_ATTR(control_black, 0666, black_show, black_store);
static DEVICE_ATTR(control_version, 0444, version_show, NULL);
#endif


static struct class *mdnie_class;
struct device *tune_mdnie_dev;

void init_mdnie_class(void)
{
	if (mdnie_tun_state.mdnie_enable) {
		pr_err("%s : mdnie already enable.. \n",__func__);
		return;
	}

	DPRINT("start!\n");

	mdnie_class = class_create(THIS_MODULE, "mdnie");
	if (IS_ERR(mdnie_class))
		pr_err("Failed to create class(mdnie)!\n");

	tune_mdnie_dev =
	    device_create(mdnie_class, NULL, 0, NULL,
		  "mdnie");
	if (IS_ERR(tune_mdnie_dev))
		pr_err("Failed to create device(mdnie)!\n");

	if (device_create_file
	    (tune_mdnie_dev, &dev_attr_scenario) < 0)
		pr_err("Failed to create device file(%s)!\n",
	       dev_attr_scenario.attr.name);

	if (device_create_file
	    (tune_mdnie_dev, &dev_attr_real_scenario) < 0)
		pr_err("Failed to create device file(%s)!\n",
	       dev_attr_real_scenario.attr.name);

	if (device_create_file
	    (tune_mdnie_dev,
	     &dev_attr_mdnieset_user_select_file_cmd) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_mdnieset_user_select_file_cmd.attr.name);

	if (device_create_file
	    (tune_mdnie_dev, &dev_attr_mdnieset_init_file_cmd) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_mdnieset_init_file_cmd.attr.name);

	if (device_create_file
		(tune_mdnie_dev, &dev_attr_mode) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_mode.attr.name);

	if (device_create_file
		(tune_mdnie_dev, &dev_attr_real_mode) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_real_mode.attr.name);

	if (device_create_file
		(tune_mdnie_dev, &dev_attr_outdoor) < 0)
		pr_err("Failed to create device file(%s)!\n",
	       dev_attr_outdoor.attr.name);

	if (device_create_file
		(tune_mdnie_dev, &dev_attr_negative) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_negative.attr.name);

	if (device_create_file
		(tune_mdnie_dev, &dev_attr_playspeed) < 0)
		pr_err("Failed to create device file(%s)!=n",
			dev_attr_playspeed.attr.name);

	if (device_create_file
		(tune_mdnie_dev, &dev_attr_accessibility) < 0)
		pr_err("Failed to create device file(%s)!=n",
			dev_attr_accessibility.attr.name);

#if defined(CONFIG_FB_MSM_MIPI_RENESAS_TFT_VIDEO_FULL_HD_PT_PANEL)
	if (device_create_file
		(tune_mdnie_dev, &dev_attr_cabc) < 0)
		pr_err("Failed to create device file(%s)!=n",
			dev_attr_cabc.attr.name);
#endif

#if defined(CONFIG_MDNIE_LITE_CONTROL)
	device_create_file(tune_mdnie_dev, &dev_attr_locked);
	device_create_file(tune_mdnie_dev, &dev_attr_hijack);
	device_create_file(tune_mdnie_dev, &dev_attr_curve);
	device_create_file(tune_mdnie_dev, &dev_attr_copy_mode);
	device_create_file(tune_mdnie_dev, &dev_attr_sharpen);
	device_create_file(tune_mdnie_dev, &dev_attr_red_red);
	device_create_file(tune_mdnie_dev, &dev_attr_red_green);
	device_create_file(tune_mdnie_dev, &dev_attr_red_blue);
	device_create_file(tune_mdnie_dev, &dev_attr_cyan_red);
	device_create_file(tune_mdnie_dev, &dev_attr_cyan_green);
	device_create_file(tune_mdnie_dev, &dev_attr_cyan_blue);
	device_create_file(tune_mdnie_dev, &dev_attr_green_red);
	device_create_file(tune_mdnie_dev, &dev_attr_green_green);
	device_create_file(tune_mdnie_dev, &dev_attr_green_blue);
	device_create_file(tune_mdnie_dev, &dev_attr_magenta_red);
	device_create_file(tune_mdnie_dev, &dev_attr_magenta_green);
	device_create_file(tune_mdnie_dev, &dev_attr_magenta_blue);
	device_create_file(tune_mdnie_dev, &dev_attr_blue_red);
	device_create_file(tune_mdnie_dev, &dev_attr_blue_green);
	device_create_file(tune_mdnie_dev, &dev_attr_blue_blue);
	device_create_file(tune_mdnie_dev, &dev_attr_yellow_red);
	device_create_file(tune_mdnie_dev, &dev_attr_yellow_green);
	device_create_file(tune_mdnie_dev, &dev_attr_yellow_blue);
	device_create_file(tune_mdnie_dev, &dev_attr_black);
	device_create_file(tune_mdnie_dev, &dev_attr_black_red);
	device_create_file(tune_mdnie_dev, &dev_attr_black_green);
	device_create_file(tune_mdnie_dev, &dev_attr_black_blue);
	device_create_file(tune_mdnie_dev, &dev_attr_white_red);
	device_create_file(tune_mdnie_dev, &dev_attr_white_green);
	device_create_file(tune_mdnie_dev, &dev_attr_white_blue);
	/* -------------------------------------------------------
	 * Add whathub's new interface
	 *
	 * NB: Current interface is kept as more app friendly.
	 *
	 * (Yank555.lu)
	 * ------------------------------------------------------- */
	device_create_file(tune_mdnie_dev, &dev_attr_control_override);
	device_create_file(tune_mdnie_dev, &dev_attr_control_sharpen);
	device_create_file(tune_mdnie_dev, &dev_attr_control_red);
	device_create_file(tune_mdnie_dev, &dev_attr_control_green);
	device_create_file(tune_mdnie_dev, &dev_attr_control_blue);
	device_create_file(tune_mdnie_dev, &dev_attr_control_cyan);
	device_create_file(tune_mdnie_dev, &dev_attr_control_magenta);
	device_create_file(tune_mdnie_dev, &dev_attr_control_yellow);
	device_create_file(tune_mdnie_dev, &dev_attr_control_white);
	device_create_file(tune_mdnie_dev, &dev_attr_control_black);
	device_create_file(tune_mdnie_dev, &dev_attr_control_version);
#endif

	mdnie_tun_state.mdnie_enable = true;
}

void mdnie_lite_tuning_init(void)
{
	init_mdnie_class();

	mipi_dsi_buf_alloc(&mdnie_tun_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&mdnie_tun_rx_buf, DSI_BUF_SIZE);
}

#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_QHD_PT)

#define coordinate_data_size 6
#define scr_wr_addr 36

#define F1(x,y) ((y)-((99*(x))/91)-6)
#define F2(x,y) ((y)-((164*(x))/157)-8)
#define F3(x,y) ((y)+((218*(x))/39)-20166)
#define F4(x,y) ((y)+((23*(x))/8)-11610)

static char coordinate_data[][coordinate_data_size] = {
	{0xff, 0x00, 0xff, 0x00, 0xff, 0x00},
	{0xff, 0x00, 0xf9, 0x00, 0xfa, 0x00},
	{0xff, 0x00, 0xfa, 0x00, 0xfe, 0x00},
	{0xfb, 0x00, 0xf9, 0x00, 0xff, 0x00},
	{0xff, 0x00, 0xfe, 0x00, 0xfb, 0x00},
	{0xff, 0x00, 0xff, 0x00, 0xff, 0x00},
	{0xfa, 0x00, 0xfb, 0x00, 0xff, 0x00},
	{0xfd, 0x00, 0xff, 0x00, 0xf9, 0x00},
	{0xfc, 0x00, 0xff, 0x00, 0xfc, 0x00},
	{0xfa, 0x00, 0xff, 0x00, 0xff, 0x00},
};

#else
#define coordinate_data_size 6
#define scr_wr_addr 36

#define F1(x,y) ((y)-((107*(x))/100)-60)
#define F2(x,y) ((y)-((44*(x))/43)-72)
#define F3(x,y) ((y)+((57*(x))/8)-25161)
#define F4(x,y) ((y)+((19*(x))/6)-12613)

static char coordinate_data[][coordinate_data_size] = {
	{0xff, 0x00, 0xff, 0x00, 0xff, 0x00}, /* dummy */
	{0xff, 0x00, 0xf7, 0x00, 0xf8, 0x00}, /* Tune_1 */
	{0xff, 0x00, 0xf9, 0x00, 0xfe, 0x00}, /* Tune_2 */
	{0xfa, 0x00, 0xf8, 0x00, 0xff, 0x00}, /* Tune_3 */
	{0xff, 0x00, 0xfc, 0x00, 0xf9, 0x00}, /* Tune_4 */
	{0xff, 0x00, 0xff, 0x00, 0xff, 0x00}, /* Tune_5 */
	{0xf8, 0x00, 0xfa, 0x00, 0xff, 0x00}, /* Tune_6 */
	{0xfc, 0x00, 0xff, 0x00, 0xf8, 0x00}, /* Tune_7 */
	{0xfb, 0x00, 0xff, 0x00, 0xfb, 0x00}, /* Tune_8 */
	{0xf9, 0x00, 0xff, 0x00, 0xff, 0x00}, /* Tune_9 */
};
#endif

void coordinate_tunning(int x, int y)
{
	int tune_number;

	tune_number = 0;

	if (F1(x,y) > 0) {
		if (F3(x,y) > 0) {
			tune_number = 3;
		} else {
			if (F4(x,y) < 0)
				tune_number = 1;
			else
				tune_number = 2;
		}
	} else {
		if (F2(x,y) < 0) {
			if (F3(x,y) > 0) {
				tune_number = 9;
			} else {
				if (F4(x,y) < 0)
					tune_number = 7;
				else
					tune_number = 8;
			}
		} else {
			if (F3(x,y) > 0)
				tune_number = 6;
			else {
				if (F4(x,y) < 0)
					tune_number = 4;
				else
					tune_number = 5;
			}
		}
	}

	pr_info("%s x : %d, y : %d, tune_number : %d", __func__, x, y, tune_number);

	memcpy(&DYNAMIC_BROWSER_2[scr_wr_addr], &coordinate_data[tune_number][0], coordinate_data_size);
	memcpy(&DYNAMIC_GALLERY_2[scr_wr_addr], &coordinate_data[tune_number][0], coordinate_data_size);
	memcpy(&DYNAMIC_UI_2[scr_wr_addr], &coordinate_data[tune_number][0], coordinate_data_size);
	memcpy(&DYNAMIC_VIDEO_2[scr_wr_addr], &coordinate_data[tune_number][0], coordinate_data_size);
	memcpy(&DYNAMIC_VT_2[scr_wr_addr], &coordinate_data[tune_number][0], coordinate_data_size);
	memcpy(&DYNAMIC_EBOOK_2[scr_wr_addr], &coordinate_data[tune_number][0], coordinate_data_size);

	memcpy(&STANDARD_BROWSER_2[scr_wr_addr], &coordinate_data[tune_number][0], coordinate_data_size);
	memcpy(&STANDARD_GALLERY_2[scr_wr_addr], &coordinate_data[tune_number][0], coordinate_data_size);
	memcpy(&STANDARD_UI_2[scr_wr_addr], &coordinate_data[tune_number][0], coordinate_data_size);
	memcpy(&STANDARD_VIDEO_2[scr_wr_addr], &coordinate_data[tune_number][0], coordinate_data_size);
	memcpy(&STANDARD_VT_2[scr_wr_addr], &coordinate_data[tune_number][0], coordinate_data_size);
	memcpy(&STANDARD_EBOOK_2[scr_wr_addr], &coordinate_data[tune_number][0], coordinate_data_size);

	memcpy(&AUTO_BROWSER_2[scr_wr_addr], &coordinate_data[tune_number][0], coordinate_data_size);
	memcpy(&AUTO_CAMERA_2[scr_wr_addr], &coordinate_data[tune_number][0], coordinate_data_size);
	memcpy(&AUTO_GALLERY_2[scr_wr_addr], &coordinate_data[tune_number][0], coordinate_data_size);
	memcpy(&AUTO_UI_2[scr_wr_addr], &coordinate_data[tune_number][0], coordinate_data_size);
	memcpy(&AUTO_VIDEO_2[scr_wr_addr], &coordinate_data[tune_number][0], coordinate_data_size);
	memcpy(&AUTO_VT_2[scr_wr_addr], &coordinate_data[tune_number][0], coordinate_data_size);

	memcpy(&CAMERA_2[scr_wr_addr], &coordinate_data[tune_number][0], coordinate_data_size);
}

MODULE_LICENSE("GPL v2");
