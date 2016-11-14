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
#define MDNIE_VERSION "Version: 1.3 (by Wootever)"
#endif

#define MDNIE_LITE_TUN_DEBUG

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

#if defined(CONFIG_MDNIE_LITE_CONTROL)
char CONTROL_1[] = {0xEB, 0x01, 0x00, 0x33, 0x01,};
char CONTROL_2[107];
int override = 0;
int copy_mode = 0;
int gamma_curve = 0;
#endif

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

struct mdnie_lite_tun_type mdnie_tun_state = {
	.mdnie_enable = FALSE,
	.scenario = mDNIe_UI_MODE,
	.background = DYNAMIC_MODE,
	.outdoor = OUTDOOR_OFF_MODE,
	.negative = mDNIe_NEGATIVE_OFF,
	.blind = ACCESSIBILITY_OFF,
};

const char accessibility_name[ACCESSIBILITY_MAX][20] = {
	"ACCESSIBILITY_OFF",
	"NEGATIVE_MODE",
	"COLOR_BLIND_MODE",
#if defined(CONFIG_FB_MSM_MIPI_RENESAS_TFT_VIDEO_FULL_HD_PT_PANEL)
	"SCREEN_CURTAIN_MODE",
#endif
};

const char background_name[MAX_BACKGROUND_MODE][16] = {
	"DYNAMIC",
	"STANDARD",
	"MOVIE",
	"NATURAL",
	"AUTO",
};

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
#if defined(CONFIG_TDMB)
	"DMB_MODE",
	"DMB_WARM_MODE",
	"DMB_COLD_MODE",
#endif
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
void update_mdnie_copy_mode(void)
{
	char *source;
	int i;

	if (copy_mode == 0) {
		source = DYNAMIC_UI_2;
		DPRINT("(mode: Dynamic)\n");
	}
	else if (copy_mode == 1) {
		source = STANDARD_UI_2;
		DPRINT("(mode: Standard)\n");
	}
	else if (copy_mode == 2) {
		source = NATURAL_UI_2;
		DPRINT("(mode: Natural)\n");
	}
	else if (copy_mode == 3) {
		source = MOVIE_UI_2;
		DPRINT("(mode: Movie)\n");
	}

	for (i = 0; i < 41; i++)
	CONTROL_2[i] = source[i];
}

void update_mdnie_gamma_curve(void)
{
	char *source;
	int i;

	if (gamma_curve == 0) {
		source = MOVIE_UI_2;
		DPRINT("(gamma: Movie)\n");
	}
	else if (gamma_curve == 1) {
		source = DYNAMIC_UI_2;
		DPRINT("(gamma: Dynamic)\n");
	}

	for (i = 42; i < 107; i++)
	CONTROL_2[i] = source[i];
}
#endif

void print_tun_data(void)
{
	int i;

	DPRINT("\n");
	DPRINT("---- size1 : %d", PAYLOAD1.dlen);
	for (i = 0; i < MDNIE_TUNE_SECOND_SIZE ; i++)
		DPRINT("0x%x ", PAYLOAD1.payload[i]);
	DPRINT("\n");
	DPRINT("---- size2 : %d", PAYLOAD2.dlen);
	for (i = 0; i < MDNIE_TUNE_FIRST_SIZE ; i++)
		DPRINT("0x%x ", PAYLOAD2.payload[i]);
	DPRINT("\n");
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

	if (mfd->panel.type == MIPI_VIDEO_PANEL)
		mutex_lock(&dsi_tx_mutex);
	else
		mutex_lock(&mfd->dma->ov_mutex);

	if (mfd->resume_state == MIPI_SUSPEND_STATE) {
		if (mfd->panel.type == MIPI_VIDEO_PANEL)
			mutex_unlock(&dsi_tx_mutex);
		else
			mutex_unlock(&mfd->dma->ov_mutex);
	} else {
#ifdef MDNIE_LITE_TUN_DATA_DEBUG
		print_tun_data();
#else
		DPRINT(" send tuning cmd!!\n");
#endif
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
	}
}

void mDNIe_Set_Mode(enum Lcd_mDNIe_UI mode)
{
	struct msm_fb_data_type *mfd;
	mfd = (struct msm_fb_data_type *) registered_fb[0]->par;


	if (!mfd) {
		DPRINT("[ERROR] mfd is null!\n");
		return;
	}

	if (mfd->resume_state == MIPI_SUSPEND_STATE) {
		return;
	}

	if (!mdnie_tun_state.mdnie_enable) {
		return;
	}

	if (mode < mDNIe_UI_MODE || mode >= MAX_mDNIe_MODE) {
		return;
	}

	if (mdnie_tun_state.negative) {
		return;
	}

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

	switch (mode) {
	case mDNIe_UI_MODE:
#if defined(CONFIG_MDNIE_LITE_CONTROL)
    if (override == 1) {
	    DPRINT(" = CONTROL MODE =\n");
	    INPUT_PAYLOAD1(CONTROL_1);
	    INPUT_PAYLOAD2(CONTROL_2);
    } else {
#endif
		DPRINT(" = UI MODE =\n");
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

	case mDNIe_VIDEO_MODE:
#if defined(CONFIG_MDNIE_LITE_CONTROL)
    if (override == 1) {
	    DPRINT(" = CONTROL MODE =\n");
	    INPUT_PAYLOAD1(CONTROL_1);
	    INPUT_PAYLOAD2(CONTROL_2);
    } else {
#endif
		DPRINT(" = VIDEO MODE =\n");
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

	case mDNIe_VIDEO_WARM_MODE:
#if defined(CONFIG_MDNIE_LITE_CONTROL)
    if (override == 1) {
	    DPRINT(" = CONTROL MODE =\n");
	    INPUT_PAYLOAD1(CONTROL_1);
	    INPUT_PAYLOAD2(CONTROL_2);
    } else {
#endif
		DPRINT(" = VIDEO WARM MODE =\n");
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
#if defined(CONFIG_MDNIE_LITE_CONTROL)
    if (override == 1) {
	    DPRINT(" = CONTROL MODE =\n");
	    INPUT_PAYLOAD1(CONTROL_1);
	    INPUT_PAYLOAD2(CONTROL_2);
    } else {
#endif
		DPRINT(" = VIDEO COLD MODE =\n");
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
#if defined(CONFIG_MDNIE_LITE_CONTROL)
    if (override == 1) {
	    DPRINT(" = CONTROL MODE =\n");
	    INPUT_PAYLOAD1(CONTROL_1);
	    INPUT_PAYLOAD2(CONTROL_2);
    } else {
#endif
		DPRINT(" = CAMERA MODE =\n");
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

	case mDNIe_NAVI:
#if defined(CONFIG_MDNIE_LITE_CONTROL)
    if (override == 1) {
	    DPRINT(" = CONTROL MODE =\n");
	    INPUT_PAYLOAD1(CONTROL_1);
	    INPUT_PAYLOAD2(CONTROL_2);
    } else {
#endif
		DPRINT(" = NAVI MODE =\n");
		DPRINT("no data for NAVI MODE..\n");
		break;

	case mDNIe_GALLERY:
#if defined(CONFIG_MDNIE_LITE_CONTROL)
    if (override == 1) {
	    DPRINT(" = CONTROL MODE =\n");
	    INPUT_PAYLOAD1(CONTROL_1);
	    INPUT_PAYLOAD2(CONTROL_2);
    } else {
#endif
		DPRINT(" = GALLERY MODE =\n");
#if defined(CONFIG_MDNIE_LITE_CONTROL)
    if (override == 1) {
	    DPRINT(" = CONTROL MODE =\n");
	    INPUT_PAYLOAD1(CONTROL_1);
	    INPUT_PAYLOAD2(CONTROL_2);
    } else {
#endif
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

	case mDNIe_VT_MODE:
#if defined(CONFIG_MDNIE_LITE_CONTROL)
    if (override == 1) {
	    DPRINT(" = CONTROL MODE =\n");
	    INPUT_PAYLOAD1(CONTROL_1);
	    INPUT_PAYLOAD2(CONTROL_2);
    } else {
#endif
		DPRINT(" = VT MODE =\n");
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

#if defined(CONFIG_TDMB)
	case mDNIe_DMB_MODE:
#if defined(CONFIG_MDNIE_LITE_CONTROL)
    if (override == 1) {
	    DPRINT(" = CONTROL MODE =\n");
	    INPUT_PAYLOAD1(CONTROL_1);
	    INPUT_PAYLOAD2(CONTROL_2);
    } else {
#endif
		DPRINT(" = DMB MODE =\n");
		if (mdnie_tun_state.outdoor == OUTDOOR_ON_MODE) {
			DPRINT(" = OUTDOOR ON MODE =\n");
			INPUT_PAYLOAD1(OUTDOOR_DMB_1);
			INPUT_PAYLOAD2(OUTDOOR_DMB_2);
		} else if (mdnie_tun_state.outdoor == OUTDOOR_OFF_MODE) {
			DPRINT(" = OUTDOOR OFF MODE =\n");
			if (mdnie_tun_state.background == STANDARD_MODE) {
				DPRINT(" = STANDARD MODE =\n");
				INPUT_PAYLOAD1(STANDARD_DMB_1);
				INPUT_PAYLOAD2(STANDARD_DMB_2);
#if !defined(CONFIG_SUPPORT_DISPLAY_OCTA_TFT)
			} else if (mdnie_tun_state.background == NATURAL_MODE) {
				DPRINT(" = NATURAL MODE =\n");
				INPUT_PAYLOAD1(NATURAL_DMB_1);
				INPUT_PAYLOAD2(NATURAL_DMB_2);
#endif
			} else if (mdnie_tun_state.background == DYNAMIC_MODE) {
				DPRINT(" = DYNAMIC MODE =\n");
				INPUT_PAYLOAD1(DYNAMIC_DMB_1);
				INPUT_PAYLOAD2(DYNAMIC_DMB_2);
			} else if (mdnie_tun_state.background == MOVIE_MODE) {
				DPRINT(" = MOVIE MODE =\n");
				INPUT_PAYLOAD1(MOVIE_DMB_1);
				INPUT_PAYLOAD2(MOVIE_DMB_2);
			} else if (mdnie_tun_state.background == AUTO_MODE) {
				DPRINT(" = AUTO MODE =\n");
				INPUT_PAYLOAD1(AUTO_DMB_1);
				INPUT_PAYLOAD2(AUTO_DMB_2);
			}
		}
		break;

	case mDNIe_DMB_WARM_MODE:
#if defined(CONFIG_MDNIE_LITE_CONTROL)
    if (override == 1) {
	    DPRINT(" = CONTROL MODE =\n");
	    INPUT_PAYLOAD1(CONTROL_1);
	    INPUT_PAYLOAD2(CONTROL_2);
    } else {
#endif
		DPRINT(" = DMB WARM MODE =\n");
		if (mdnie_tun_state.outdoor == OUTDOOR_ON_MODE) {
			DPRINT(" = OUTDOOR ON MODE =\n");
			INPUT_PAYLOAD1(WARM_OUTDOOR_DMB_1);
			INPUT_PAYLOAD2(WARM_OUTDOOR_DMB_2);
		} else if (mdnie_tun_state.outdoor == OUTDOOR_OFF_MODE) {
			DPRINT(" = OUTDOOR OFF MODE =\n");
			INPUT_PAYLOAD1(WARM_DMB_1);
			INPUT_PAYLOAD2(WARM_DMB_2);
		}
		break;

	case mDNIe_DMB_COLD_MODE:
#if defined(CONFIG_MDNIE_LITE_CONTROL)
    if (override == 1) {
	    DPRINT(" = CONTROL MODE =\n");
	    INPUT_PAYLOAD1(CONTROL_1);
	    INPUT_PAYLOAD2(CONTROL_2);
    } else {
#endif
		DPRINT(" = DMB COLD MODE =\n");
		if (mdnie_tun_state.outdoor == OUTDOOR_ON_MODE) {
			DPRINT(" = OUTDOOR ON MODE =\n");
			INPUT_PAYLOAD1(COLD_OUTDOOR_DMB_1);
			INPUT_PAYLOAD2(COLD_OUTDOOR_DMB_2);
		} else if (mdnie_tun_state.outdoor == OUTDOOR_OFF_MODE) {
			DPRINT(" = OUTDOOR OFF MODE =\n");
			INPUT_PAYLOAD1(COLD_DMB_1);
			INPUT_PAYLOAD2(COLD_DMB_2);
		}
		break;
#endif

	case mDNIe_BROWSER_MODE:
#if defined(CONFIG_MDNIE_LITE_CONTROL)
    if (override == 1) {
	    DPRINT(" = CONTROL MODE =\n");
	    INPUT_PAYLOAD1(CONTROL_1);
	    INPUT_PAYLOAD2(CONTROL_2);
    } else {
#endif
		DPRINT(" = BROWSER MODE =\n");
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

	case mDNIe_eBOOK_MODE:
#if defined(CONFIG_MDNIE_LITE_CONTROL)
    if (override == 1) {
	    DPRINT(" = CONTROL MODE =\n");
	    INPUT_PAYLOAD1(CONTROL_1);
	    INPUT_PAYLOAD2(CONTROL_2);
    } else {
#endif
		DPRINT(" = eBOOK MODE =\n");
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

	case mDNIE_BLINE_MODE:
#if defined(CONFIG_MDNIE_LITE_CONTROL)
    if (override == 1) {
	    DPRINT(" = CONTROL MODE =\n");
	    INPUT_PAYLOAD1(CONTROL_1);
	    INPUT_PAYLOAD2(CONTROL_2);
    } else {
#endif
		DPRINT(" = BLIND MODE =\n");
		INPUT_PAYLOAD1(COLOR_BLIND_1);
		INPUT_PAYLOAD2(COLOR_BLIND_2);
		break;

	case mDNIE_DARK_SCREEN_MODE:
#if defined(CONFIG_MDNIE_LITE_CONTROL)
    if (override == 1) {
	    DPRINT(" = CONTROL MODE =\n");
	    INPUT_PAYLOAD1(CONTROL_1);
	    INPUT_PAYLOAD2(CONTROL_2);
    } else {
#endif
		DPRINT(" = DARK SCREEN MODE =\n");
		INPUT_PAYLOAD1(DARK_SCREEN_BLIND_1);
		INPUT_PAYLOAD2(DARK_SCREEN_BLIND_2);
		break;

	default:
		DPRINT("[%s] no option (%d)\n", __func__, mode);
		return;
	}

	sending_tuning_cmd();
	free_tun_cmd();

	DPRINT("mDNIe_Set_Mode end , mode(%d), background(%d)\n",
		mode, mdnie_tun_state.background);
}

void mDNIe_set_negative(enum Lcd_mDNIe_Negative negative)
{
	DPRINT("mDNIe_Set_Negative START\n");

	if (negative == 0) {
		DPRINT("Negative mode(%d) -> reset mode(%d)\n",
			mdnie_tun_state.negative, mdnie_tun_state.scenario);
		mDNIe_Set_Mode(mdnie_tun_state.scenario);

	} else {

		DPRINT("mDNIe_Set_Negative = %d\n", mdnie_tun_state.negative);
		DPRINT(" = NEGATIVE MODE =\n");

		INPUT_PAYLOAD1(NEGATIVE_1);
		INPUT_PAYLOAD2(NEGATIVE_2);

		sending_tuning_cmd();
		free_tun_cmd();
	}

	DPRINT("mDNIe_Set_Negative END\n");
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
	return snprintf(buf, 256, "%s\n",
		background_name[mdnie_tun_state.background]);
}

static ssize_t mode_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int value;

	sscanf(buf, "%d", &value);
	//DPRINT("set background mode : %d\n", value);

	if (value < DYNAMIC_MODE || value >= MAX_BACKGROUND_MODE) {
		//DPRINT("[ERROR] wrong backgound mode value : %d\n",
			//value);
		return size;
	}

	mdnie_tun_state.background = value;
	mDNIe_Set_Mode(mdnie_tun_state.scenario);

	return size;
}

static DEVICE_ATTR(mode, 0666, mode_show, mode_store);

static ssize_t scenario_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	//DPRINT("called %s\n", __func__);

	//DPRINT("%s\n",
		//scenario_name[mdnie_tun_state.scenario]);

	return snprintf(buf, 256, "%s\n",
		scenario_name[mdnie_tun_state.scenario]);
}

static ssize_t scenario_store(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t size)
{
	int value;

	sscanf(buf, "%d", &value);

	if (value < mDNIe_UI_MODE || value >= MAX_mDNIe_MODE) {
		//DPRINT("[ERROR] wrong Scenario mode value : %d\n",
			//value);
		return size;
	}

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

#ifdef BROWSER_COLOR_TONE_SET
	case SIG_MDNIE_BROWSER_TONE1:
		mdnie_tun_state.scenario = mDNIe_BROWSER_TONE1;
		break;
	case SIG_MDNIE_BROWSER_TONE2:
		mdnie_tun_state.scenario = mDNIe_BROWSER_TONE2;
		break;
	case SIG_MDNIE_BROWSER_TONE3:
		mdnie_tun_state.scenario = mDNIe_BROWSER_TONE3;
		break;
#endif


#if defined(CONFIG_TDMB)
	case SIG_MDNIE_DMB_MODE:
		mdnie_tun_state.scenario = mDNIe_DMB_MODE;
		break;
	case SIG_MDNIE_DMB_WARM_MODE:
		mdnie_tun_state.scenario = mDNIe_DMB_WARM_MODE;
		break;
	case SIG_MDNIE_DMB_COLD_MODE:
		mdnie_tun_state.scenario = mDNIe_DMB_COLD_MODE;
		break;
#endif

	default:
		//DPRINT("scenario_store value is wrong : value(%d)\n",
		       //value);
		break;
	}

	if (mdnie_tun_state.negative) {
		DPRINT("already negative mode(%d), do not set mode(%d)\n",
			mdnie_tun_state.negative, mdnie_tun_state.scenario);
	} else {
		//DPRINT(" %s, input value = %d\n", __func__, value);
		mDNIe_Set_Mode(mdnie_tun_state.scenario);
	}
	return size;
}
static DEVICE_ATTR(scenario, 0666, scenario_show,
		   scenario_store);

static ssize_t mdnieset_user_select_file_cmd_show(struct device *dev,
						  struct device_attribute *attr,
						  char *buf)
{
	int mdnie_ui = 0;
	//DPRINT("called %s\n", __func__);

	return snprintf(buf, 256, "%u\n", mdnie_ui);
}

static ssize_t mdnieset_user_select_file_cmd_store(struct device *dev,
						   struct device_attribute
						   *attr, const char *buf,
						   size_t size)
{
	int value;

	sscanf(buf, "%d", &value);
	//DPRINT
	//("inmdnieset_user_select_file_cmd_store, input value = %d\n",
	     //value);

	return size;
}

static DEVICE_ATTR(mdnieset_user_select_file_cmd, 0666,
		   mdnieset_user_select_file_cmd_show,
		   mdnieset_user_select_file_cmd_store);

static ssize_t mdnieset_init_file_cmd_show(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	char temp[] = "mdnieset_init_file_cmd_show\n\0";
	//DPRINT("called %s\n", __func__);
	strcat(buf, temp);
	return strlen(buf);
}

static ssize_t mdnieset_init_file_cmd_store(struct device *dev,
					    struct device_attribute *attr,
					    const char *buf, size_t size)
{
	int value;

	sscanf(buf, "%d", &value);
	//DPRINT("mdnieset_init_file_cmd_store  : value(%d)\n", value);

	switch (value) {
	case 0:
		mdnie_tun_state.scenario = mDNIe_UI_MODE;
		break;

	default:
		//printk(KERN_ERR
		       //"mdnieset_init_file_cmd_store value is wrong : value(%d)\n",
		       //value);
		break;
	}
	mDNIe_Set_Mode(mdnie_tun_state.scenario);

	return size;
}

static DEVICE_ATTR(mdnieset_init_file_cmd, 0666, mdnieset_init_file_cmd_show,
		   mdnieset_init_file_cmd_store);

static ssize_t outdoor_show(struct device *dev,
					      struct device_attribute *attr,
					      char *buf)
{
	//DPRINT("called %s\n", __func__);
	return snprintf(buf, 256, "%s\n",
		(mdnie_tun_state.outdoor == 0) ? "Disabled" : "Enabled");
}

static ssize_t outdoor_store(struct device *dev,
					       struct device_attribute *attr,
					       const char *buf, size_t size)
{
	int value;

	sscanf(buf, "%d", &value);

	DPRINT("outdoor value = %d, scenario = %d\n",
		value, mdnie_tun_state.scenario);

	if (value < OUTDOOR_OFF_MODE || value >= MAX_OUTDOOR_MODE) {
		DPRINT("[ERROR] : wrong outdoor mode value : %d\n",
				value);
	}

	mdnie_tun_state.outdoor = value;

	if (mdnie_tun_state.negative) {
		DPRINT("already negative mode(%d), do not outdoor mode(%d)\n",
			mdnie_tun_state.negative, mdnie_tun_state.outdoor);
	} else {
		mDNIe_Set_Mode(mdnie_tun_state.scenario);
	}

	return size;
}

static DEVICE_ATTR(outdoor, 0666, outdoor_show, outdoor_store);

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

	//DPRINT
	    //("negative_store, input value = %d\n",
	     //value);

	mdnie_tun_state.negative = value;

	mDNIe_set_negative(mdnie_tun_state.negative);

	return size;
}

void is_negative_on(void)
{
	DPRINT("is negative Mode On = %d\n", mdnie_tun_state.negative);

	if (mdnie_tun_state.negative) {
		DPRINT("mDNIe_Set_Negative = %d\n", mdnie_tun_state.negative);
		DPRINT(" = NEGATIVE MODE =\n");

		INPUT_PAYLOAD1(NEGATIVE_1);
		INPUT_PAYLOAD2(NEGATIVE_2);

		sending_tuning_cmd();
		free_tun_cmd();
	} else {
		/* check the mode and tuning again when wake up*/
		//DPRINT("negative off when resume, tuning again!\n");
		mDNIe_Set_Mode(mdnie_tun_state.scenario);
	}
}
static DEVICE_ATTR(negative, 666,
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

	//DPRINT("[Play Speed Set]play speed value = %d\n", value);

	is_play_speed_1_5(value);
	return size;
}
static DEVICE_ATTR(playspeed, 0666,
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

static DEVICE_ATTR(accessibility, 0666,
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
static DEVICE_ATTR(cabc, 0666,
			cabc_show,
			cabc_store);
#endif

#if defined(CONFIG_MDNIE_LITE_CONTROL)
/* override */
static ssize_t override_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", override);
}

static ssize_t override_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    int val;
	sscanf(buf, "%d", &val);

	if (val != override) {
		if (val < 0 || val > 1)
			return -EINVAL;
		DPRINT("(override: %d)\n", val);
		override = val;
		mDNIe_Set_Mode();
	}
    return size;
}

/* copy_mode */
static ssize_t copy_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", copy_mode);
}

static ssize_t copy_mode_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
   int val;
	sscanf(buf, "%d", &val);

	if (val != copy_mode) {
		if (val < 0 || val > 2)
			return -EINVAL;
		copy_mode = val;
		update_mdnie_copy_mode();
		if (override == 1)
			mDNIe_Set_Mode();
	}
    return size;
}

/* gamma_curve */
static ssize_t gamma_curve_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", gamma_curve);
}

static ssize_t gamma_curve_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    int val;
	sscanf(buf, "%d", &val);

	if (val != gamma_curve) {
		if (val < 0 || val > 1)
			return -EINVAL;
		gamma_curve = val;
		update_mdnie_gamma_curve();
		if (override == 1)
			mDNIe_Set_Mode();
	}
    return size;
}

/* sharpen */
static ssize_t sharpen_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", CONTROL_1[4]);
}

static ssize_t sharpen_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    int val;
	sscanf(buf, "%d", &val);

	if (val != CONTROL_1[4]) {
		if (val < 0 || val > 11)
			return -EINVAL;
		DPRINT("(sharpen: %d)\n", val);
		CONTROL_1[4] = val;
		if (override == 1)
			mDNIe_Set_Mode();
	}
    return size;
}

/* red */
static ssize_t red_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d %d %d\n", CONTROL_2[19], CONTROL_2[21], CONTROL_2[23]);
}

static ssize_t red_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    int red, green, blue;
	sscanf(buf, "%d %d %d", &red, &green, &blue);

	if (red != CONTROL_2[19] || green != CONTROL_2[21] || blue != CONTROL_2[23]) {
		if (red < 0 || red > 255 || green < 0 || green > 255 || blue < 0 || blue > 255)
			return -EINVAL;
		DPRINT("[RED] (red: %d) (green: %d) (blue: %d)\n", red, green, blue);
		CONTROL_2[19] = red;
		CONTROL_2[21] = green;
		CONTROL_2[23] = blue;
		if (override == 1)
			mDNIe_Set_Mode();
	}
    return size;
}

/* green */
static ssize_t green_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d %d %d\n", CONTROL_2[25], CONTROL_2[27], CONTROL_2[29]);
}

static ssize_t green_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    int red, green, blue;
	sscanf(buf, "%d %d %d", &red, &green, &blue);

	if (red != CONTROL_2[25] || green != CONTROL_2[27] || blue != CONTROL_2[29]) {
		if (red < 0 || red > 255 || green < 0 || green > 255 || blue < 0 || blue > 255)
			return -EINVAL;
		DPRINT("[GREEN] (red: %d) (green: %d) (blue: %d)\n", red, green, blue);
		CONTROL_2[25] = red;
		CONTROL_2[27] = green;
		CONTROL_2[29] = blue;
		if (override == 1)
			mDNIe_Set_Mode();
	}
    return size;
}

/* blue */
static ssize_t blue_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d %d %d\n", CONTROL_2[31], CONTROL_2[33], CONTROL_2[35]);
}

static ssize_t blue_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    int red, green, blue;
	sscanf(buf, "%d %d %d", &red, &green, &blue);

	if (red != CONTROL_2[31] || green != CONTROL_2[33] || blue != CONTROL_2[35]) {
		if (red < 0 || red > 255 || green < 0 || green > 255 || blue < 0 || blue > 255)
			return -EINVAL;
		DPRINT("[BLUE] (red: %d) (green: %d) (blue: %d)\n", red, green, blue);
		CONTROL_2[31] = red;
		CONTROL_2[33] = green;
		CONTROL_2[35] = blue;
		if (override == 1)
			mDNIe_Set_Mode();
	}
    return size;
}

/* cyan */
static ssize_t cyan_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d %d %d\n", CONTROL_2[18], CONTROL_2[20], CONTROL_2[22]);
}

static ssize_t cyan_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    int red, green, blue;
	sscanf(buf, "%d %d %d", &red, &green, &blue);

	if (red != CONTROL_2[18] || green != CONTROL_2[20] || blue != CONTROL_2[22]) {
		if (red < 0 || red > 255 || green < 0 || green > 255 || blue < 0 || blue > 255)
			return -EINVAL;
		DPRINT("[CYAN] (red: %d) (green: %d) (blue: %d)\n", red, green, blue);
		CONTROL_2[18] = red;
		CONTROL_2[20] = green;
		CONTROL_2[22] = blue;
		if (override == 1)
			mDNIe_Set_Mode();
	}
    return size;
}

/* magenta */
static ssize_t magenta_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d %d %d\n", CONTROL_2[24], CONTROL_2[26], CONTROL_2[28]);
}

static ssize_t magenta_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    int red, green, blue;
	sscanf(buf, "%d %d %d", &red, &green, &blue);

	if (red != CONTROL_2[24] || green != CONTROL_2[26] || blue != CONTROL_2[28]) {
		if (red < 0 || red > 255 || green < 0 || green > 255 || blue < 0 || blue > 255)
			return -EINVAL;
		DPRINT("[MAGENTA] (red: %d) (green: %d) (blue: %d)\n", red, green, blue);
		CONTROL_2[24] = red;
		CONTROL_2[26] = green;
		CONTROL_2[28] = blue;
		if (override == 1)
			mDNIe_Set_Mode();
	}
    return size;
}

/* yellow */
static ssize_t yellow_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d %d %d\n", CONTROL_2[30], CONTROL_2[32], CONTROL_2[34]);
}

static ssize_t yellow_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    int red, green, blue;
	sscanf(buf, "%d %d %d", &red, &green, &blue);

	if (red != CONTROL_2[30] || green != CONTROL_2[32] || blue != CONTROL_2[34]) {
		if (red < 0 || red > 255 || green < 0 || green > 255 || blue < 0 || blue > 255)
			return -EINVAL;
		DPRINT("[YELLOW] (red: %d) (green: %d) (blue: %d)\n", red, green, blue);
		CONTROL_2[30] = red;
		CONTROL_2[32] = green;
		CONTROL_2[34] = blue;
		if (override == 1)
			mDNIe_Set_Mode();
	}
    return size;
}

/* white */
static ssize_t white_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d %d %d\n", CONTROL_2[36], CONTROL_2[38], CONTROL_2[40]);
}

static ssize_t white_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    int red, green, blue;
	sscanf(buf, "%d %d %d", &red, &green, &blue);

	if (red != CONTROL_2[36] || green != CONTROL_2[38] || blue != CONTROL_2[40]) {
		if (red < 0 || red > 255 || green < 0 || green > 255 || blue < 0 || blue > 255)
			return -EINVAL;
		DPRINT("[WHITE] (red: %d) (green: %d) (blue: %d)\n", red, green, blue);
		CONTROL_2[36] = red;
		CONTROL_2[38] = green;
		CONTROL_2[40] = blue;
		if (override == 1)
			mDNIe_Set_Mode();
	}
    return size;
}

/* black */
static ssize_t black_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d %d %d\n", CONTROL_2[37], CONTROL_2[39], CONTROL_2[41]);
}

static ssize_t black_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    int red, green, blue;
	sscanf(buf, "%d %d %d", &red, &green, &blue);

	if (red != CONTROL_2[37] || green != CONTROL_2[39] || blue != CONTROL_2[41]) {
		if (red < 0 || red > 255 || green < 0 || green > 255 || blue < 0 || blue > 255)
			return -EINVAL;
		DPRINT("[BLACK] (red: %d) (green: %d) (blue: %d)\n", red, green, blue);
		CONTROL_2[37] = red;
		CONTROL_2[39] = green;
		CONTROL_2[41] = blue;
		if (override == 1)
			mDNIe_Set_Mode();
	}
    return size;
}

static ssize_t version_show(struct device *dev, struct device_attribute *attr, char *buf)
{
        return sprintf(buf, "%s\n", MDNIE_VERSION);
}

static DEVICE_ATTR(override, 0664, override_show, override_store);
static DEVICE_ATTR(copy_mode, 0664, copy_mode_show, copy_mode_store);
static DEVICE_ATTR(gamma_curve, 0664, gamma_curve_show, gamma_curve_store);
static DEVICE_ATTR(sharpen, 0664, sharpen_show, sharpen_store);
static DEVICE_ATTR(red, 0664, red_show, red_store);
static DEVICE_ATTR(green, 0664, green_show, green_store);
static DEVICE_ATTR(blue, 0664, blue_show, blue_store);
static DEVICE_ATTR(cyan, 0664, cyan_show, cyan_store);
static DEVICE_ATTR(magenta, 0664, magenta_show, magenta_store);
static DEVICE_ATTR(yellow, 0664, yellow_show, yellow_store);
static DEVICE_ATTR(white, 0664, white_show, white_store);
static DEVICE_ATTR(black, 0664, black_show, black_store);
static DEVICE_ATTR(version, 0444, version_show, NULL);
#endif

static struct class *mdnie_class;
struct device *tune_mdnie_dev;

void init_mdnie_class(void)
{

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
	device_create_file(tune_mdnie_dev, &dev_attr_override);
	device_create_file(tune_mdnie_dev, &dev_attr_copy_mode);
	device_create_file(tune_mdnie_dev, &dev_attr_gamma_curve);
	device_create_file(tune_mdnie_dev, &dev_attr_sharpen);
	device_create_file(tune_mdnie_dev, &dev_attr_red);
	device_create_file(tune_mdnie_dev, &dev_attr_green);
	device_create_file(tune_mdnie_dev, &dev_attr_blue);
	device_create_file(tune_mdnie_dev, &dev_attr_cyan);
	device_create_file(tune_mdnie_dev, &dev_attr_magenta);
	device_create_file(tune_mdnie_dev, &dev_attr_yellow);
	device_create_file(tune_mdnie_dev, &dev_attr_white);
	device_create_file(tune_mdnie_dev, &dev_attr_black);
	device_create_file(tune_mdnie_dev, &dev_attr_version);
#endif

	mdnie_tun_state.mdnie_enable = true;

#if defined(CONFIG_MDNIE_LITE_CONTROL)
	update_mdnie_copy_mode();
	update_mdnie_gamma_curve();
#endif

	DPRINT("end!\n");
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
#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_QHD_PT)
	memcpy(&DYNAMIC_eBOOK_2[scr_wr_addr], &coordinate_data[tune_number][0], coordinate_data_size);
#else
	memcpy(&DYNAMIC_EBOOK_2[scr_wr_addr], &coordinate_data[tune_number][0], coordinate_data_size);
#endif

	memcpy(&STANDARD_BROWSER_2[scr_wr_addr], &coordinate_data[tune_number][0], coordinate_data_size);
	memcpy(&STANDARD_GALLERY_2[scr_wr_addr], &coordinate_data[tune_number][0], coordinate_data_size);
	memcpy(&STANDARD_UI_2[scr_wr_addr], &coordinate_data[tune_number][0], coordinate_data_size);
	memcpy(&STANDARD_VIDEO_2[scr_wr_addr], &coordinate_data[tune_number][0], coordinate_data_size);
	memcpy(&STANDARD_VT_2[scr_wr_addr], &coordinate_data[tune_number][0], coordinate_data_size);
#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_QHD_PT)
	memcpy(&STANDARD_eBOOK_2[scr_wr_addr], &coordinate_data[tune_number][0], coordinate_data_size);
#else
	memcpy(&STANDARD_EBOOK_2[scr_wr_addr], &coordinate_data[tune_number][0], coordinate_data_size);
#endif
	memcpy(&AUTO_BROWSER_2[scr_wr_addr], &coordinate_data[tune_number][0], coordinate_data_size);
	memcpy(&AUTO_CAMERA_2[scr_wr_addr], &coordinate_data[tune_number][0], coordinate_data_size);
	memcpy(&AUTO_GALLERY_2[scr_wr_addr], &coordinate_data[tune_number][0], coordinate_data_size);
	memcpy(&AUTO_UI_2[scr_wr_addr], &coordinate_data[tune_number][0], coordinate_data_size);
	memcpy(&AUTO_VIDEO_2[scr_wr_addr], &coordinate_data[tune_number][0], coordinate_data_size);
	memcpy(&AUTO_VT_2[scr_wr_addr], &coordinate_data[tune_number][0], coordinate_data_size);

	memcpy(&CAMERA_2[scr_wr_addr], &coordinate_data[tune_number][0], coordinate_data_size);

}

