/* Copyright (c) 2008-2009, 2012-2013 The Linux Foundation. All rights reserved.
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
#include <linux/hrtimer.h>
#include <linux/vmalloc.h>
#include <linux/clk.h>

#include <mach/hardware.h>
#include <linux/io.h>

#include <asm/system.h>
#include <asm/mach-types.h>
#include <linux/semaphore.h>
#include <linux/uaccess.h>
#include <mach/gpio.h>

#include "mdp.h"
#include "msm_fb.h"
#include "mddihost.h"

#ifdef CONFIG_FB_MSM_MDP40
#include "mdp4.h"

#define MDP_SYNC_CFG_0		0x100
#define MDP_SYNC_STATUS_0	0x10c
#define MDP_SYNC_CFG_1		0x104
#define MDP_SYNC_STATUS_1	0x110
#define MDP_PRIM_VSYNC_OUT_CTRL	0x118
#define MDP_SEC_VSYNC_OUT_CTRL	0x11C
#define MDP_VSYNC_SEL		0x124
#define MDP_PRIM_VSYNC_INIT_VAL	0x128
#define MDP_SEC_VSYNC_INIT_VAL	0x12C
#else
#define MDP_SYNC_CFG_0		0x300
#define MDP_SYNC_STATUS_0	0x30c
#define MDP_PRIM_VSYNC_OUT_CTRL	0x318
#define MDP_PRIM_VSYNC_INIT_VAL	0x328
#endif

extern mddi_lcd_type mddi_lcd_idx;
extern spinlock_t mdp_spin_lock;
extern struct workqueue_struct *mdp_vsync_wq;
extern int lcdc_mode;
extern int vsync_mode;

#ifdef MDP_HW_VSYNC
int vsync_above_th = 4;
int vsync_start_th = 1;
int vsync_load_cnt;
int vsync_clk_status;
DEFINE_MUTEX(vsync_clk_lock);
static DEFINE_SPINLOCK(vsync_timer_lock);

static struct clk *mdp_vsync_clk;
static struct msm_fb_data_type *vsync_mfd;
static unsigned char timer_shutdown_flag;
static uint32 vsync_cnt_cfg;


void vsync_clk_prepare_enable(void)
{
	if (mdp_vsync_clk)
		clk_prepare_enable(mdp_vsync_clk);
}

void vsync_clk_disable_unprepare(void)
{
	if (mdp_vsync_clk)
		clk_disable_unprepare(mdp_vsync_clk);
}

void mdp_hw_vsync_clk_enable(struct msm_fb_data_type *mfd)
{
	if (vsync_clk_status == 1)
		return;
	mutex_lock(&vsync_clk_lock);
	if (mfd->use_mdp_vsync) {
		clk_prepare_enable(mdp_vsync_clk);
		vsync_clk_status = 1;
	}
	mutex_unlock(&vsync_clk_lock);
}

void mdp_hw_vsync_clk_disable(struct msm_fb_data_type *mfd)
{
	if (vsync_clk_status == 0)
		return;
	mutex_lock(&vsync_clk_lock);
	if (mfd->use_mdp_vsync) {
		clk_disable_unprepare(mdp_vsync_clk);
		vsync_clk_status = 0;
	}
	mutex_unlock(&vsync_clk_lock);
}

static void mdp_set_vsync(unsigned long data);
void mdp_vsync_clk_enable(void)
{
	if (vsync_mfd) {
		mdp_hw_vsync_clk_enable(vsync_mfd);
		if (!vsync_mfd->vsync_resync_timer.function)
			mdp_set_vsync((unsigned long) vsync_mfd);
	}
}

void mdp_vsync_clk_disable(void)
{
	if (vsync_mfd) {
		if (vsync_mfd->vsync_resync_timer.function) {
			spin_lock(&vsync_timer_lock);
			timer_shutdown_flag = 1;
			spin_unlock(&vsync_timer_lock);
			del_timer_sync(&vsync_mfd->vsync_resync_timer);
			spin_lock(&vsync_timer_lock);
			timer_shutdown_flag = 0;
			spin_unlock(&vsync_timer_lock);
			vsync_mfd->vsync_resync_timer.function = NULL;
		}

		mdp_hw_vsync_clk_disable(vsync_mfd);
	}
}
#endif

static void mdp_set_vsync(unsigned long data)
{
	struct msm_fb_data_type *mfd = (struct msm_fb_data_type *)data;
	struct msm_fb_panel_data *pdata = NULL;

	pdata = (struct msm_fb_panel_data *)mfd->pdev->dev.platform_data;

	vsync_mfd = mfd;
	init_timer(&mfd->vsync_resync_timer);

	if ((pdata) && (pdata->set_vsync_notifier == NULL))
		return;

	if ((mfd->panel_info.lcd.vsync_enable) && (mfd->panel_power_on)
	    && (!mfd->vsync_handler_pending)) {
		mfd->vsync_handler_pending = TRUE;
		if (!queue_work(mdp_vsync_wq, &mfd->vsync_resync_worker)) {
			MSM_FB_INFO
			    ("mdp_set_vsync: can't queue_work! -> needs to increase vsync_resync_timer_duration\n");
		}
	} else {
		MSM_FB_DEBUG
		    ("mdp_set_vsync failed!  EN:%d  PWR:%d  PENDING:%d\n",
		     mfd->panel_info.lcd.vsync_enable, mfd->panel_power_on,
		     mfd->vsync_handler_pending);
	}

	spin_lock(&vsync_timer_lock);
	if (!timer_shutdown_flag) {
		mfd->vsync_resync_timer.function = mdp_set_vsync;
		mfd->vsync_resync_timer.data = data;
		mfd->vsync_resync_timer.expires =
			jiffies + mfd->panel_info.lcd.vsync_notifier_period;
		add_timer(&mfd->vsync_resync_timer);
	}
	spin_unlock(&vsync_timer_lock);
}

static void mdp_vsync_handler(void *data)
{
	struct msm_fb_data_type *mfd = (struct msm_fb_data_type *)data;

	if (vsync_clk_status == 0) {
		pr_debug("Warning: vsync clk is disabled\n");
		mfd->vsync_handler_pending = FALSE;
		return;
	}

	if (mfd->use_mdp_vsync) {
#ifdef MDP_HW_VSYNC
		if (mfd->panel_power_on) {
			MDP_OUTP(MDP_BASE + MDP_SYNC_STATUS_0, vsync_load_cnt);

#ifdef CONFIG_FB_MSM_MDP40
			if (mdp_hw_revision < MDP4_REVISION_V2_1)
				MDP_OUTP(MDP_BASE + MDP_SYNC_STATUS_1,
						vsync_load_cnt);
#endif
		}

#endif
	} else {
		mfd->last_vsync_timetick = ktime_get_real();
	}

	mfd->vsync_handler_pending = FALSE;
}

irqreturn_t mdp_hw_vsync_handler_proxy(int irq, void *data)
{
	/*
	 * ToDo: tried enabling/disabling GPIO MDP HW VSYNC interrupt
	 * but getting inaccurate timing in mdp_vsync_handler()
	 * disable_irq(MDP_HW_VSYNC_IRQ);
	 */
	mdp_vsync_handler(data);

	return IRQ_HANDLED;
}

#ifdef MDP_HW_VSYNC
static void mdp_set_sync_cfg_0(struct msm_fb_data_type *mfd, int vsync_cnt)
{
	unsigned long cfg;

	if (mfd->panel_info.lcd.total_lines)
		cfg = mfd->panel_info.lcd.total_lines;
	else
		cfg = mfd->total_lcd_lines - 1;

	cfg <<= MDP_SYNCFG_HGT_LOC;
	if (mfd->panel_info.lcd.hw_vsync_mode)
		cfg |= MDP_SYNCFG_VSYNC_EXT_EN;
	cfg |= (MDP_SYNCFG_VSYNC_INT_EN | vsync_cnt);

	MDP_OUTP(MDP_BASE + MDP_SYNC_CFG_0, cfg);
}

#ifdef CONFIG_FB_MSM_MDP40
static void mdp_set_sync_cfg_1(struct msm_fb_data_type *mfd, int vsync_cnt)
{
	unsigned long cfg;

	if (mfd->panel_info.lcd.total_lines)
		cfg = mfd->panel_info.lcd.total_lines;
	else
		cfg = mfd->total_lcd_lines - 1;

	cfg <<= MDP_SYNCFG_HGT_LOC;
	if (mfd->panel_info.lcd.hw_vsync_mode)
		cfg |= MDP_SYNCFG_VSYNC_EXT_EN;
	cfg |= (MDP_SYNCFG_VSYNC_INT_EN | vsync_cnt);

	MDP_OUTP(MDP_BASE + MDP_SYNC_CFG_1, cfg);
}
#endif

void mdp_vsync_cfg_regs(struct msm_fb_data_type *mfd,
	boolean first_time)
{
	/* MDP cmd block enable */
	mdp_pipe_ctrl(MDP_CMD_BLOCK, MDP_BLOCK_POWER_ON,
			  FALSE);
	if (first_time)
		mdp_hw_vsync_clk_enable(mfd);

	mdp_set_sync_cfg_0(mfd, vsync_cnt_cfg);

#ifdef CONFIG_FB_MSM_MDP40
	if (mdp_hw_revision < MDP4_REVISION_V2_1)
		mdp_set_sync_cfg_1(mfd, vsync_cnt_cfg);
#endif

	/*
	 * load the last line + 1 to be in the
	 * safety zone
	 */
	vsync_load_cnt = mfd->panel_info.yres;

	/* line counter init value at the next pulse */
	MDP_OUTP(MDP_BASE + MDP_PRIM_VSYNC_INIT_VAL,
		vsync_load_cnt);
#ifdef CONFIG_FB_MSM_MDP40
	if (mdp_hw_revision < MDP4_REVISION_V2_1) {
		MDP_OUTP(MDP_BASE +	MDP_SEC_VSYNC_INIT_VAL,
			vsync_load_cnt);
	}
#endif

	/*
	 * external vsync source pulse width and
	 * polarity flip
	 */
	MDP_OUTP(MDP_BASE + MDP_PRIM_VSYNC_OUT_CTRL, BIT(0));
#ifdef CONFIG_FB_MSM_MDP40
	if (mdp_hw_revision < MDP4_REVISION_V2_1) {
		MDP_OUTP(MDP_BASE +	MDP_SEC_VSYNC_OUT_CTRL, BIT(0));
		MDP_OUTP(MDP_BASE +	MDP_VSYNC_SEL, 0x20);
	}
#endif

	/* threshold */
	MDP_OUTP(MDP_BASE + 0x200, (vsync_above_th << 16) |
		 (vsync_start_th));

	if (first_time)
		mdp_hw_vsync_clk_disable(mfd);

	/* MDP cmd block disable */
	mdp_pipe_ctrl(MDP_CMD_BLOCK, MDP_BLOCK_POWER_OFF, FALSE);
}
#endif

void mdp_config_vsync(struct platform_device *pdev,
	struct msm_fb_data_type *mfd)
{
	/* vsync on primary lcd only for now */
	if ((mfd->dest != DISPLAY_LCD) || (mfd->panel_info.pdest != DISPLAY_1)
	    || (!vsync_mode)) {
		goto err_handle;
	}

	vsync_clk_status = 0;
	if (mfd->panel_info.lcd.vsync_enable) {
		mfd->total_porch_lines = mfd->panel_info.lcd.v_back_porch +
		    mfd->panel_info.lcd.v_front_porch +
		    mfd->panel_info.lcd.v_pulse_width;
		mfd->total_lcd_lines =
		    mfd->panel_info.yres + mfd->total_porch_lines;
		mfd->lcd_ref_usec_time =
		    100000000 / mfd->panel_info.lcd.refx100;
		mfd->vsync_handler_pending = FALSE;

		mfd->last_vsync_timetick.tv64 = 0;

#ifdef MDP_HW_VSYNC
		if (mdp_vsync_clk == NULL)
			mdp_vsync_clk = clk_get(&pdev->dev, "vsync_clk");

		if (IS_ERR(mdp_vsync_clk)) {
			printk(KERN_ERR "error: can't get mdp_vsync_clk!\n");
			mfd->use_mdp_vsync = 0;
		} else
			mfd->use_mdp_vsync = 1;

		if (mfd->use_mdp_vsync) {
			uint32 vsync_cnt_cfg_dem;
			uint32 mdp_vsync_clk_speed_hz;

			mdp_vsync_clk_speed_hz = clk_get_rate(mdp_vsync_clk);

			if (mdp_vsync_clk_speed_hz == 0) {
				mfd->use_mdp_vsync = 0;
			} else {
				/*
				 * Do this calculation in 2 steps for
				 * rounding uint32 properly.
				 */
				vsync_cnt_cfg_dem =
				    (mfd->panel_info.lcd.refx100 *
				     mfd->total_lcd_lines) / 100;
				vsync_cnt_cfg =
				    (mdp_vsync_clk_speed_hz) /
				    vsync_cnt_cfg_dem;
				mdp_vsync_cfg_regs(mfd, TRUE);
			}
		}
#else
		mfd->use_mdp_vsync = 0;
		hrtimer_init(&mfd->dma_hrtimer, CLOCK_MONOTONIC,
			     HRTIMER_MODE_REL);
		mfd->dma_hrtimer.function = mdp_dma2_vsync_hrtimer_handler;
		mfd->vsync_width_boundary = vmalloc(mfd->panel_info.xres * 4);
#endif

#ifdef CONFIG_FB_MSM_MDDI
		mfd->channel_irq = 0;
		if (mfd->panel_info.lcd.hw_vsync_mode) {
			u32 vsync_gpio = mfd->vsync_gpio;
			u32 ret;

			if (vsync_gpio == -1) {
				MSM_FB_INFO("vsync_gpio not defined!\n");
				goto err_handle;
			}

			ret = gpio_tlmm_config(GPIO_CFG
					(vsync_gpio,
					(mfd->use_mdp_vsync) ? 1 : 0,
					GPIO_CFG_INPUT,
					GPIO_CFG_PULL_DOWN,
					GPIO_CFG_2MA),
					GPIO_CFG_ENABLE);
			if (ret)
				goto err_handle;

			/*
			 * if use_mdp_vsync, then no interrupt need since
			 * mdp_vsync is feed directly to mdp to reset the
			 * write pointer counter. therefore no irq_handler
			 * need to reset write pointer counter.
			 */
			if (!mfd->use_mdp_vsync) {
				mfd->channel_irq = MSM_GPIO_TO_INT(vsync_gpio);
				if (request_irq
				    (mfd->channel_irq,
				     &mdp_hw_vsync_handler_proxy,
				     IRQF_TRIGGER_FALLING, "VSYNC_GPIO",
				     (void *)mfd)) {
					MSM_FB_INFO
					("irq=%d failed! vsync_gpio=%d\n",
						mfd->channel_irq,
						vsync_gpio);
					goto err_handle;
				}
			}
		}
#endif
		mdp_hw_vsync_clk_enable(mfd);
		mdp_set_vsync((unsigned long)mfd);
	}

	return;

err_handle:
	if (mfd->vsync_width_boundary)
		vfree(mfd->vsync_width_boundary);
	mfd->panel_info.lcd.vsync_enable = FALSE;
	printk(KERN_ERR "%s: failed!\n", __func__);
}

void mdp_vsync_resync_workqueue_handler(struct work_struct *work)
{
	struct msm_fb_data_type *mfd = NULL;
	int vsync_fnc_enabled = FALSE;
	struct msm_fb_panel_data *pdata = NULL;

	mfd = container_of(work, struct msm_fb_data_type, vsync_resync_worker);

	if (mfd) {
		if (mfd->panel_power_on) {
			pdata =
			    (struct msm_fb_panel_data *)mfd->pdev->dev.
			    platform_data;

			if (pdata->set_vsync_notifier != NULL) {
				if (pdata->clk_func && !pdata->clk_func(2)) {
					mfd->vsync_handler_pending = FALSE;
					return;
				}

				pdata->set_vsync_notifier(
						mdp_vsync_handler,
						(void *)mfd);
				vsync_fnc_enabled = TRUE;
			}
		}
	}

	if ((mfd) && (!vsync_fnc_enabled))
		mfd->vsync_handler_pending = FALSE;
}

boolean mdp_hw_vsync_set_handler(msm_fb_vsync_handler_type handler, void *data)
{
	/*
	 * ToDo: tried enabling/disabling GPIO MDP HW VSYNC interrupt
	 * but getting inaccurate timing in mdp_vsync_handler()
	 * enable_irq(MDP_HW_VSYNC_IRQ);
	 */

	return TRUE;
}

uint32 mdp_get_lcd_line_counter(struct msm_fb_data_type *mfd)
{
	uint32 elapsed_usec_time;
	uint32 lcd_line;
	ktime_t last_vsync_timetick_local;
	ktime_t curr_time;
	unsigned long flag;

	if ((!mfd->panel_info.lcd.vsync_enable) || (!vsync_mode))
		return 0;

	spin_lock_irqsave(&mdp_spin_lock, flag);
	last_vsync_timetick_local = mfd->last_vsync_timetick;
	spin_unlock_irqrestore(&mdp_spin_lock, flag);

	curr_time = ktime_get_real();
	elapsed_usec_time = ktime_to_us(ktime_sub(curr_time,
						last_vsync_timetick_local));

	elapsed_usec_time = elapsed_usec_time % mfd->lcd_ref_usec_time;

	/* lcd line calculation referencing to line counter = 0 */
	lcd_line =
	    (elapsed_usec_time * mfd->total_lcd_lines) / mfd->lcd_ref_usec_time;

	/* lcd line adjusment referencing to the actual line counter at vsync */
	lcd_line =
	    (mfd->total_lcd_lines - mfd->panel_info.lcd.v_back_porch +
	     lcd_line) % (mfd->total_lcd_lines + 1);

	if (lcd_line > mfd->total_lcd_lines) {
		MSM_FB_INFO
		    ("mdp_get_lcd_line_counter: mdp_lcd_rd_cnt >= mfd->total_lcd_lines error!\n");
	}

	return lcd_line;
}
