/* Copyright (c) 2008-2012, The Linux Foundation. All rights reserved.
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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/semaphore.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <asm/system.h>
#include <asm/mach-types.h>
#include <mach/hardware.h>
#include <mach/gpio.h>
#include <mach/clk.h>
#ifdef CONFIG_LCD_NOTIFY
#include <linux/lcd_notify.h>
#endif
#ifdef CONFIG_POWERSUSPEND
#include <linux/powersuspend.h>
#endif
#ifdef CONFIG_STATE_NOTIFIER
#include <linux/state_notifier.h>
#endif
#include <linux/display_state.h>

#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mdp.h"
#include "mdp4.h"
#if defined(CONFIG_MIPI_SAMSUNG_ESD_REFRESH)
#include "mipi_samsung_esd_refresh-8960.h"
#endif

#define DSI_VIDEO_BASE	0xE0000
#if defined(CONFIG_MIPI_SAMSUNG_ESD_REFRESH)
#include "mipi_samsung_esd_refresh.h"
#endif
#if defined (CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_QHD_PT) || defined (CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_WVGA_PT)
/* Check if LCD was connected. */
#include "mipi_samsung_oled-8930.h"
#endif

static bool display_on = true;
bool is_display_on()
{
	return display_on;
}

u32 dsi_irq;
u32 esc_byte_ratio;

static boolean tlmm_settings = FALSE;

static int mipi_dsi_probe(struct platform_device *pdev);
static int mipi_dsi_remove(struct platform_device *pdev);

static int mipi_dsi_off(struct platform_device *pdev);
static int mipi_dsi_on(struct platform_device *pdev);

static struct platform_device *pdev_list[MSM_FB_MAX_DEV_LIST];
static int pdev_list_cnt;
static struct mipi_dsi_platform_data *mipi_dsi_pdata;

static int vsync_gpio = -1;

static struct platform_driver mipi_dsi_driver = {
	.probe = mipi_dsi_probe,
	.remove = mipi_dsi_remove,
	.shutdown = NULL,
	.driver = {
		   .name = "mipi_dsi",
		   },
};

struct device dsi_dev;

#if defined(CONFIG_MIPI_SAMSUNG_ESD_REFRESH) || defined(CONFIG_ESD_ERR_FG_RECOVERY)
extern struct mutex power_state_change;
static struct platform_device *pdev_for_esd;
#endif
#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_WVGA_PT) \
|| defined (CONFIG_MACH_LT02_SPR) || defined (CONFIG_MACH_LT02_ATT) || defined(CONFIG_MACH_LT02_TMO)
void pull_reset_low(void){
if (mipi_dsi_pdata && mipi_dsi_pdata->active_reset)
	mipi_dsi_pdata->active_reset(0);
}
#endif
static int mipi_dsi_off(struct platform_device *pdev)
{
	int ret = 0;
	struct msm_fb_data_type *mfd;
	struct msm_panel_info *pinfo;
	uint32 dsi_ctrl;

	pr_debug("%s+:\n", __func__);

	mfd = platform_get_drvdata(pdev);
	pinfo = &mfd->panel_info;

	if (mdp_rev >= MDP_REV_41)
		mutex_lock(&mfd->dma->ov_mutex);
	else
		down(&mfd->dma->mutex);

	if (mfd->panel_info.type == MIPI_CMD_PANEL) {
		mipi_dsi_prepare_ahb_clocks();
		mipi_dsi_ahb_ctrl(1);
		mipi_dsi_clk_enable();

		/* make sure dsi_cmd_mdp is idle */
		mipi_dsi_cmd_mdp_busy();
	}

	/*
	 * Desctiption: change to DSI_CMD_MODE since it needed to
	 * tx DCS dsiplay off comamnd to panel
	 * mipi_dsi_op_mode_config(DSI_CMD_MODE);
	 */

	if (mfd->panel_info.type == MIPI_CMD_PANEL) {
		if (pinfo->lcd.vsync_enable) {
			if (pinfo->lcd.hw_vsync_mode && vsync_gpio >= 0) {
				if (MDP_REV_303 != mdp_rev)
					gpio_free(vsync_gpio);
			}
			mipi_dsi_set_tear_off(mfd);
		}
	}

	ret = panel_next_off(pdev);

#ifdef CONFIG_LCD_NOTIFY
	lcd_notifier_call_chain(LCD_EVENT_OFF_START, NULL);
#endif

	spin_lock_bh(&dsi_clk_lock);

	mipi_dsi_clk_disable();

	/* disbale dsi engine */
	dsi_ctrl = MIPI_INP(MIPI_DSI_BASE + 0x0000);
	dsi_ctrl &= ~0x01;
	MIPI_OUTP(MIPI_DSI_BASE + 0x0000, dsi_ctrl);

	MIPI_OUTP(MIPI_DSI_BASE + 0x010c, 0); /* DSI_INTL_CTRL */
	MDP_OUTP(MDP_BASE + DSI_VIDEO_BASE, 0);

	mipi_dsi_phy_ctrl(0);

	mipi_dsi_ahb_ctrl(0);
	spin_unlock_bh(&dsi_clk_lock);

	mipi_dsi_unprepare_clocks();
	mipi_dsi_unprepare_ahb_clocks();

	usleep(5000);
#if defined (CONFIG_MIPI_DSI_RESET_LP11)

	if (mipi_dsi_pdata && mipi_dsi_pdata->active_reset)
		mipi_dsi_pdata->active_reset(0); /* low */
#endif

	usleep(2000); /*1ms delay(minimum) required between reset low and AVDD off*/
#if defined(CONFIG_SUPPORT_SECOND_POWER)
	if (mipi_dsi_pdata && mipi_dsi_pdata->panel_power_save)
		mipi_dsi_pdata->panel_power_save(0);
#endif
	if (mipi_dsi_pdata && mipi_dsi_pdata->dsi_power_save)
		mipi_dsi_pdata->dsi_power_save(0);

	if (mdp_rev >= MDP_REV_41)
		mutex_unlock(&mfd->dma->ov_mutex);
	else
		up(&mfd->dma->mutex);

	printk("Rob's DSI OFF HOOK\n");

#if 0
	state_suspend();
#endif

#ifdef CONFIG_POWERSUSPEND
	 /*Yank555.lu : hook to handle powersuspend tasks (sleep)*/
	set_power_suspend_state_panel_hook(POWER_SUSPEND_ACTIVE);
#endif

#ifdef CONFIG_LCD_NOTIFY
	lcd_notifier_call_chain(LCD_EVENT_OFF_END, NULL);
#endif
	display_on = false;

	return ret;
}

static int mipi_dsi_on(struct platform_device *pdev)
{
	int ret = 0;
	u32 clk_rate;
	struct msm_fb_data_type *mfd;
	struct fb_info *fbi;
	struct fb_var_screeninfo *var;
	struct msm_panel_info *pinfo;
	struct mipi_panel_info *mipi;
	u32 hbp, hfp, vbp, vfp, hspw, vspw, width, height;
	u32 ystride, bpp, data;
	u32 dummy_xres, dummy_yres;
	int target_type = 0;
	u32 tmp;
#if defined(CONFIG_FB_MSM_MIPI_RENESAS_TFT_VIDEO_FULL_HD_PT_PANEL)
	static int is_booting = 1;
#endif

	pr_debug("%s+:\n", __func__);

#if defined(CONFIG_MIPI_SAMSUNG_ESD_REFRESH) || defined(CONFIG_ESD_ERR_FG_RECOVERY)
	pdev_for_esd = pdev;
#endif
#if defined (CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_QHD_PT)
	if (get_lcd_attached() == 0)
		return ENODEV;
#endif
	mfd = platform_get_drvdata(pdev);
	fbi = mfd->fbi;
	var = &fbi->var;
	pinfo = &mfd->panel_info;

	if (mipi_dsi_pdata && mipi_dsi_pdata->power_common)
		mipi_dsi_pdata->power_common();

#if defined(CONFIG_SUPPORT_SECOND_POWER)
#if defined(CONFIG_FB_MSM_MIPI_RENESAS_TFT_VIDEO_FULL_HD_PT_PANEL)
	if( is_booting == 1 ) {
		is_booting = 0;
	}
#endif

	if (mipi_dsi_pdata && mipi_dsi_pdata->panel_power_save)
		mipi_dsi_pdata->panel_power_save(1);
#endif

#if !defined(CONFIG_SEC_PRODUCT_8930) && !defined(CONFIG_SEC_PRODUCT_8960)
	if (system_rev == 6)
		mdelay(500);
#endif

	if (mipi_dsi_pdata && mipi_dsi_pdata->dsi_power_save)
		mipi_dsi_pdata->dsi_power_save(1);

	cont_splash_clk_ctrl(0);
	mipi_dsi_prepare_ahb_clocks();

	mipi_dsi_ahb_ctrl(1);

	clk_rate = mfd->fbi->var.pixclock;
	clk_rate = min(clk_rate, mfd->panel_info.clk_max);

	mipi_dsi_phy_ctrl(1);

	if (mdp_rev == MDP_REV_42 && mipi_dsi_pdata)
		target_type = mipi_dsi_pdata->target_type;

	mipi_dsi_phy_init(0, &(mfd->panel_info), target_type);

	mipi_dsi_clk_enable();
#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_QHD_PT_PANEL)
	mipi_dsi_configure_dividers(60);
#endif


	MIPI_OUTP(MIPI_DSI_BASE + 0x114, 1);
	MIPI_OUTP(MIPI_DSI_BASE + 0x114, 0);

	hbp = var->left_margin;
	hfp = var->right_margin;
	vbp = var->upper_margin;
	vfp = var->lower_margin;
	hspw = var->hsync_len;
	vspw = var->vsync_len;
	width = mfd->panel_info.xres;
	height = mfd->panel_info.yres;

	mipi  = &mfd->panel_info.mipi;
	if (mfd->panel_info.type == MIPI_VIDEO_PANEL) {
		dummy_xres = mfd->panel_info.lcdc.xres_pad;
		dummy_yres = mfd->panel_info.lcdc.yres_pad;

		if (mdp_rev >= MDP_REV_41) {
			MIPI_OUTP(MIPI_DSI_BASE + 0x20,
				((hspw + hbp + width + dummy_xres) << 16 |
				(hspw + hbp)));
			MIPI_OUTP(MIPI_DSI_BASE + 0x24,
				((vspw + vbp + height + dummy_yres) << 16 |
				(vspw + vbp)));
			MIPI_OUTP(MIPI_DSI_BASE + 0x28,
				(vspw + vbp + height + dummy_yres +
					vfp - 1) << 16 | (hspw + hbp +
					width + dummy_xres + hfp - 1));
		} else {
			/* DSI_LAN_SWAP_CTRL */
			MIPI_OUTP(MIPI_DSI_BASE + 0x00ac, mipi->dlane_swap);

			MIPI_OUTP(MIPI_DSI_BASE + 0x20,
				((hbp + width + dummy_xres) << 16 | (hbp)));
			MIPI_OUTP(MIPI_DSI_BASE + 0x24,
				((vbp + height + dummy_yres) << 16 | (vbp)));
			MIPI_OUTP(MIPI_DSI_BASE + 0x28,
				(vbp + height + dummy_yres + vfp) << 16 |
					(hbp + width + dummy_xres + hfp));
		}

		MIPI_OUTP(MIPI_DSI_BASE + 0x2c, (hspw << 16));
		MIPI_OUTP(MIPI_DSI_BASE + 0x30, 0);
		MIPI_OUTP(MIPI_DSI_BASE + 0x34, (vspw << 16));

	} else {		/* command mode */
		if (mipi->dst_format == DSI_CMD_DST_FORMAT_RGB888)
			bpp = 3;
		else if (mipi->dst_format == DSI_CMD_DST_FORMAT_RGB666)
			bpp = 3;
		else if (mipi->dst_format == DSI_CMD_DST_FORMAT_RGB565)
			bpp = 2;
		else
			bpp = 3;	/* Default format set to RGB888 */

		ystride = width * bpp + 1;

		/* DSI_COMMAND_MODE_MDP_STREAM_CTRL */
		data = (ystride << 16) | (mipi->vc << 8) | DTYPE_DCS_LWRITE;
		MIPI_OUTP(MIPI_DSI_BASE + 0x5c, data);
		MIPI_OUTP(MIPI_DSI_BASE + 0x54, data);

		/* DSI_COMMAND_MODE_MDP_STREAM_TOTAL */
		data = height << 16 | width;
		MIPI_OUTP(MIPI_DSI_BASE + 0x60, data);
		MIPI_OUTP(MIPI_DSI_BASE + 0x58, data);
	}

	mipi_dsi_host_init(mipi);
#if defined(CONFIG_FB_MSM_MIPI_SAMSUNG_OLED_VIDEO_HD_PT_PANEL)
	{
		u32 tmp_reg0c, tmp_rega8;
		udelay(200);
		/* backup register values */
		tmp_reg0c = MIPI_INP(MIPI_DSI_BASE + 0x000c);
		tmp_rega8 = MIPI_INP(MIPI_DSI_BASE + 0xA8);
		/* Clear HS  mode assertion and related flags */
		MIPI_OUTP(MIPI_DSI_BASE + 0x0c, 0x8000);
		MIPI_OUTP(MIPI_DSI_BASE + 0xA8, 0x0);
		wmb();
		mdelay(10);
		if (mipi_dsi_pdata && mipi_dsi_pdata->lcd_rst_up)
		mipi_dsi_pdata->lcd_rst_up();
		/* restore previous values */
		MIPI_OUTP(MIPI_DSI_BASE + 0x0c, tmp_reg0c);
		MIPI_OUTP(MIPI_DSI_BASE + 0xa8, tmp_rega8);
		wmb();
	}
#else
	msleep(10);
#if defined (CONFIG_MIPI_DSI_RESET_LP11)

	/* LP11 */
	tmp = MIPI_INP(MIPI_DSI_BASE + 0xA8);
	tmp &= ~(1<<28);
	MIPI_OUTP(MIPI_DSI_BASE + 0xA8, tmp);
	wmb();
	/* LP11 */

	usleep(5000);
	if (mipi_dsi_pdata && mipi_dsi_pdata->active_reset)
			mipi_dsi_pdata->active_reset(1); /* high */
	usleep(10000);
#endif
#if defined(CONFIG_MACH_LT02_SPR) || defined(CONFIG_MACH_LT02_ATT) || defined(CONFIG_MACH_LT02_TMO)
	if(system_rev)
		ret = panel_next_on(pdev);
#elif defined(CONFIG_MACH_LT02_CHN_CTC)
        ret = panel_next_on(pdev);
#endif
#endif
	/* always high */
	if (mipi->force_clk_lane_hs) {
		tmp = MIPI_INP(MIPI_DSI_BASE + 0xA8);
		tmp |= (1<<28);
		MIPI_OUTP(MIPI_DSI_BASE + 0xA8, tmp);
		wmb();
	}

	if (mdp_rev >= MDP_REV_41)
		mutex_lock(&mfd->dma->ov_mutex);
	else
		down(&mfd->dma->mutex);

#if !defined(CONFIG_MACH_LT02_CHN_CTC)
#if defined(CONFIG_MACH_LT02_SPR) || defined(CONFIG_MACH_LT02_ATT) || defined(CONFIG_MACH_LT02_TMO)
	if(!system_rev)
		ret = panel_next_on(pdev);
#else
	ret = panel_next_on(pdev);
#endif
#endif
	mipi_dsi_op_mode_config(mipi->mode);

	if (mfd->panel_info.type == MIPI_CMD_PANEL) {
		if (pinfo->lcd.vsync_enable) {
			if (pinfo->lcd.hw_vsync_mode && vsync_gpio >= 0) {
				if (mdp_rev >= MDP_REV_41) {
					if (gpio_request(vsync_gpio,
						"MDP_VSYNC") == 0)
						gpio_direction_input(
							vsync_gpio);
					else
						pr_err("%s: unable to \
							request gpio=%d\n",
							__func__, vsync_gpio);
				} else if (mdp_rev == MDP_REV_303) {
					if (!tlmm_settings && gpio_request(
						vsync_gpio, "MDP_VSYNC") == 0) {
						ret = gpio_tlmm_config(
							GPIO_CFG(
							vsync_gpio, 1,
							GPIO_CFG_INPUT,
							GPIO_CFG_PULL_DOWN,
							GPIO_CFG_2MA),
							GPIO_CFG_ENABLE);

						if (ret) {
							pr_err(
							"%s: unable to config \
							tlmm = %d\n",
							__func__, vsync_gpio);
						}
						tlmm_settings = TRUE;

						gpio_direction_input(
							vsync_gpio);
					} else {
						if (!tlmm_settings) {
							pr_err(
							"%s: unable to request \
							gpio=%d\n",
							__func__, vsync_gpio);
						}
					}
				}
			}
			mipi_dsi_set_tear_on(mfd);
		}
		mipi_dsi_clk_disable();
		mipi_dsi_unprepare_clocks();
		mipi_dsi_ahb_ctrl(0);
		mipi_dsi_unprepare_ahb_clocks();
	}

	if (mdp_rev >= MDP_REV_41)
		mutex_unlock(&mfd->dma->ov_mutex);
	else
		up(&mfd->dma->mutex);

	printk("Rob's DSI ON HOOK\n");

#ifdef CONFIG_STATE_NOTIFIER
		state_resume();
#endif

#ifdef CONFIG_POWERSUSPEND
		/* Yank555.lu : hook to handle powersuspend tasks (wakeup) */
		set_power_suspend_state_panel_hook(POWER_SUSPEND_INACTIVE);
#endif

#ifdef CONFIG_LCD_NOTIFY
		lcd_notifier_call_chain(LCD_EVENT_ON_END, NULL);
#endif
		display_on = true;

	return ret;
}

static int mipi_dsi_late_init(struct platform_device *pdev)
{
	return panel_next_late_init(pdev);
}

#if defined(CONFIG_MIPI_SAMSUNG_ESD_REFRESH) || defined(CONFIG_ESD_ERR_FG_RECOVERY)
void esd_recovery(void)
{
	struct msm_fb_data_type *mfd;
	u32 tmp, tmp2;

	if (pdev_for_esd) {
		mfd = platform_get_drvdata(pdev_for_esd);

		if (mfd->panel_power_on == TRUE) {
			mutex_lock(&power_state_change);

			panel_next_off(pdev_for_esd);

			if (mipi_dsi_pdata && mipi_dsi_pdata->active_reset)
				mipi_dsi_pdata->active_reset(0); /* low */
#if defined(CONFIG_SUPPORT_SECOND_POWER)
			if (mipi_dsi_pdata && mipi_dsi_pdata->panel_power_save)
				mipi_dsi_pdata->panel_power_save(0);
#endif
			msleep(10);
#if defined(CONFIG_SUPPORT_SECOND_POWER)
			if (mipi_dsi_pdata && mipi_dsi_pdata->panel_power_save)
				mipi_dsi_pdata->panel_power_save(1);
#endif
			/* LP11 */
			tmp2 = tmp = MIPI_INP(MIPI_DSI_BASE + 0xA8);
			tmp &= ~(1<<28);
			MIPI_OUTP(MIPI_DSI_BASE + 0xA8, tmp);
			wmb();
			/* LP11 */

			usleep(5000);
			if (mipi_dsi_pdata && mipi_dsi_pdata->active_reset)
				mipi_dsi_pdata->active_reset(1); /* high */
			msleep(10);
			if (mipi_dsi_pdata && mipi_dsi_pdata->active_reset)
				mipi_dsi_pdata->active_reset(0); /* low */
			msleep(10);
			if (mipi_dsi_pdata && mipi_dsi_pdata->active_reset)
				mipi_dsi_pdata->active_reset(1); /* high */
			msleep(10);

			MIPI_OUTP(MIPI_DSI_BASE + 0xA8, tmp2);
			wmb();

			panel_next_on(pdev_for_esd);
			mipi_dsi_late_init(pdev_for_esd);

#if defined(CONFIG_MDNIE_LITE_TUNING)
			is_negative_on();
#endif
			mutex_unlock(&power_state_change);
		}
	}
}
#endif

static int mipi_dsi_resource_initialized;

static int mipi_dsi_probe(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct fb_info *fbi;
	struct msm_panel_info *pinfo;
	struct mipi_panel_info *mipi;
	struct platform_device *mdp_dev = NULL;
	struct msm_fb_panel_data *pdata = NULL;
	int rc;
	uint8 lanes = 0, bpp;
	uint32 h_period, v_period, dsi_pclk_rate;

	resource_size_t size ;

	if ((pdev->id == 1) && (pdev->num_resources >= 0)) {
		mipi_dsi_pdata = pdev->dev.platform_data;

		size =  resource_size(&pdev->resource[0]);
		mipi_dsi_base =  ioremap(pdev->resource[0].start, size);

		MSM_FB_INFO("mipi_dsi base phy_addr = 0x%x virt = 0x%x\n",
				pdev->resource[0].start, (int) mipi_dsi_base);

		if (!mipi_dsi_base)
			return -ENOMEM;

		if (mdp_rev >= MDP_REV_41) {
			mmss_sfpb_base =  ioremap(MMSS_SFPB_BASE_PHY, 0x100);
			MSM_FB_INFO("mmss_sfpb  base phy_addr = 0x%x,"
				"virt = 0x%x\n", MMSS_SFPB_BASE_PHY,
				(int) mmss_sfpb_base);

			if (!mmss_sfpb_base)
				return -ENOMEM;
		}

		dsi_irq = platform_get_irq(pdev, 0);
		if (dsi_irq < 0) {
			pr_err("mipi_dsi: can not get mdp irq\n");
			return -ENOMEM;
		}

		rc = request_irq(dsi_irq, mipi_dsi_isr, IRQF_DISABLED,
						"MIPI_DSI", 0);
		if (rc) {
			pr_err("mipi_dsi_host request_irq() failed!\n");
			return rc;
		}

		disable_irq(dsi_irq);

		if (mdp_rev == MDP_REV_42 && mipi_dsi_pdata &&
			mipi_dsi_pdata->target_type == 1) {
			/* Target type is 1 for device with (De)serializer
			 * 0x4f00000 is the base for TV Encoder.
			 * Unused Offset 0x1000 is used for
			 * (de)serializer on emulation platform
			 */
			periph_base = ioremap(MMSS_SERDES_BASE_PHY, 0x100);

			if (periph_base) {
				pr_debug("periph_base %p\n", periph_base);
				writel(0x4, periph_base + 0x28);
				writel(0xc, periph_base + 0x28);
			} else {
				pr_err("periph_base is NULL\n");
				free_irq(dsi_irq, 0);
				return -ENOMEM;
			}
		}

		if (mipi_dsi_pdata) {
			vsync_gpio = mipi_dsi_pdata->vsync_gpio;
			pr_debug("%s: vsync_gpio=%d\n", __func__, vsync_gpio);

			if (mdp_rev == MDP_REV_303 &&
				mipi_dsi_pdata->dsi_client_reset) {
				if (mipi_dsi_pdata->dsi_client_reset())
					pr_err("%s: DSI Client Reset failed!\n",
						__func__);
				else
					pr_debug("%s: DSI Client Reset success\n",
						__func__);
			}
		}

		if (mipi_dsi_clk_init(pdev))
			return -EPERM;

		if (mipi_dsi_pdata->splash_is_enabled &&
			!mipi_dsi_pdata->splash_is_enabled()) {
			mipi_dsi_prepare_ahb_clocks();
			mipi_dsi_ahb_ctrl(1);
			MIPI_OUTP(MIPI_DSI_BASE + 0x118, 0);
			MIPI_OUTP(MIPI_DSI_BASE + 0x0, 0);
			MIPI_OUTP(MIPI_DSI_BASE + 0x200, 0);
			mipi_dsi_ahb_ctrl(0);
			mipi_dsi_unprepare_ahb_clocks();
		}

		mipi_dsi_resource_initialized = 1;

		return 0;
	}

	if (!mipi_dsi_resource_initialized)
		return -EPERM;

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;

	if (mfd->key != MFD_KEY)
		return -EINVAL;

	if (pdev_list_cnt >= MSM_FB_MAX_DEV_LIST)
		return -ENOMEM;

	if (!mfd->cont_splash_done) {
		if (mipi_dsi_pdata->splash_is_enabled &&
			mipi_dsi_pdata->splash_is_enabled())
			cont_splash_clk_ctrl(1);
	} else
		cont_splash_clk_ctrl(0);

	mdp_dev = platform_device_alloc("mdp", pdev->id);
	if (!mdp_dev)
		return -ENOMEM;

	/*
	 * link to the latest pdev
	 */
	mfd->pdev = mdp_dev;

	/*
	 * alloc panel device data
	 */
	if (platform_device_add_data
	    (mdp_dev, pdev->dev.platform_data,
	     sizeof(struct msm_fb_panel_data))) {
		pr_err("mipi_dsi_probe: platform_device_add_data failed!\n");
		platform_device_put(mdp_dev);
		return -ENOMEM;
	}
	/*
	 * data chain
	 */
	pdata = mdp_dev->dev.platform_data;
	pdata->on = mipi_dsi_on;
	pdata->off = mipi_dsi_off;
	pdata->late_init = mipi_dsi_late_init;
	pdata->next = pdev;

	/*
	 * get/set panel specific fb info
	 */
	mfd->panel_info = pdata->panel_info;
	pinfo = &mfd->panel_info;

	if (mfd->panel_info.type == MIPI_VIDEO_PANEL)
		mfd->dest = DISPLAY_LCDC;
	else
		mfd->dest = DISPLAY_LCD;

	if (mdp_rev == MDP_REV_303 &&
		mipi_dsi_pdata->get_lane_config) {
		if (mipi_dsi_pdata->get_lane_config() != 2) {
			pr_info("Changing to DSI Single Mode Configuration\n");
#ifdef CONFIG_FB_MSM_MDP303
			update_lane_config(pinfo);
#endif
		}
	}

	if (mfd->index == 0)
		mfd->fb_imgType = MSMFB_DEFAULT_TYPE;
	else
		mfd->fb_imgType = MDP_RGB_565;

	fbi = mfd->fbi;
	fbi->var.pixclock = mfd->panel_info.clk_rate;
	fbi->var.left_margin = mfd->panel_info.lcdc.h_back_porch;
	fbi->var.right_margin = mfd->panel_info.lcdc.h_front_porch;
	fbi->var.upper_margin = mfd->panel_info.lcdc.v_back_porch;
	fbi->var.lower_margin = mfd->panel_info.lcdc.v_front_porch;
	fbi->var.hsync_len = mfd->panel_info.lcdc.h_pulse_width;
	fbi->var.vsync_len = mfd->panel_info.lcdc.v_pulse_width;

	h_period = ((mfd->panel_info.lcdc.h_pulse_width)
			+ (mfd->panel_info.lcdc.h_back_porch)
			+ (mfd->panel_info.xres)
			+ (mfd->panel_info.lcdc.h_front_porch));

	v_period = ((mfd->panel_info.lcdc.v_pulse_width)
			+ (mfd->panel_info.lcdc.v_back_porch)
			+ (mfd->panel_info.yres)
			+ (mfd->panel_info.lcdc.v_front_porch));

	mipi  = &mfd->panel_info.mipi;

	if (mipi->data_lane3)
		lanes += 1;
	if (mipi->data_lane2)
		lanes += 1;
	if (mipi->data_lane1)
		lanes += 1;
	if (mipi->data_lane0)
		lanes += 1;

	if ((mipi->dst_format == DSI_CMD_DST_FORMAT_RGB888)
	    || (mipi->dst_format == DSI_VIDEO_DST_FORMAT_RGB888)
	    || (mipi->dst_format == DSI_VIDEO_DST_FORMAT_RGB666_LOOSE))
		bpp = 3;
	else if ((mipi->dst_format == DSI_CMD_DST_FORMAT_RGB565)
		 || (mipi->dst_format == DSI_VIDEO_DST_FORMAT_RGB565))
		bpp = 2;
	else
		bpp = 3;		/* Default format set to RGB888 */

	if (mfd->panel_info.type == MIPI_VIDEO_PANEL &&
		!mfd->panel_info.clk_rate) {
		h_period += mfd->panel_info.lcdc.xres_pad;
		v_period += mfd->panel_info.lcdc.yres_pad;

		if (lanes > 0) {
			mfd->panel_info.clk_rate =
			((h_period * v_period * (mipi->frame_rate) * bpp * 8)
			   / lanes);
		} else {
			pr_err("%s: forcing mipi_dsi lanes to 1\n", __func__);
			mfd->panel_info.clk_rate =
				(h_period * v_period
					 * (mipi->frame_rate) * bpp * 8);
		}
	}
	pll_divider_config.clk_rate = mfd->panel_info.clk_rate;

	rc = mipi_dsi_clk_div_config(bpp, lanes, &dsi_pclk_rate);
	if (rc)
		goto mipi_dsi_probe_err;

	if ((dsi_pclk_rate < 3300000) || (dsi_pclk_rate > 223000000)) {
		pr_err("%s: Pixel clock not supported\n", __func__);
		dsi_pclk_rate = 35000000;
	}
	mipi->dsi_pclk_rate = dsi_pclk_rate;

	/*
	 * set driver data
	 */
	platform_set_drvdata(mdp_dev, mfd);

	/*
	 * register in mdp driver
	 */
	rc = platform_device_add(mdp_dev);
	if (rc)
		goto mipi_dsi_probe_err;

	pdev_list[pdev_list_cnt++] = pdev;

#if defined(CONFIG_MIPI_SAMSUNG_ESD_REFRESH)
	register_mipi_dev(pdev);
#endif

	esc_byte_ratio = pinfo->mipi.esc_byte_ratio;

return 0;

mipi_dsi_probe_err:
	platform_device_put(mdp_dev);
	return rc;
}

static int mipi_dsi_remove(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	mfd = platform_get_drvdata(pdev);
	iounmap(mipi_dsi_base);
	return 0;
}

static int mipi_dsi_register_driver(void)
{
	return platform_driver_register(&mipi_dsi_driver);
}

static int __init mipi_dsi_driver_init(void)
{
	int ret;

	mipi_dsi_init();

	ret = mipi_dsi_register_driver();

	device_initialize(&dsi_dev);

	if (ret) {
		pr_err("mipi_dsi_register_driver() failed!\n");
		return ret;
	}

	return ret;
}

module_init(mipi_dsi_driver_init);
MODULE_LICENSE("GPL v2");
