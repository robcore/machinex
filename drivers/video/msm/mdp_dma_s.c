/* Copyright (c) 2008-2009, The Linux Foundation. All rights reserved.
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
#include <linux/hrtimer.h>

#include <mach/hardware.h>
#include <linux/io.h>

#include <asm/system.h>
#include <asm/mach-types.h>
#include <linux/semaphore.h>
#include <linux/spinlock.h>

#include <linux/fb.h>

#include "mdp.h"
#include "msm_fb.h"

static void mdp_dma_s_update_lcd(struct msm_fb_data_type *mfd)
{
	MDPIBUF *iBuf = &mfd->ibuf;
	int mddi_dest = FALSE;
	uint32 outBpp = iBuf->bpp;
	uint32 dma_s_cfg_reg;
	uint8 *src;
	struct msm_fb_panel_data *pdata =
	    (struct msm_fb_panel_data *)mfd->pdev->dev.platform_data;

	dma_s_cfg_reg = DMA_PACK_TIGHT | DMA_PACK_ALIGN_LSB |
	    DMA_OUT_SEL_AHB | DMA_IBUF_NONCONTIGUOUS;

	if (mfd->fb_imgType == MDP_BGR_565)
		dma_s_cfg_reg |= DMA_PACK_PATTERN_BGR;
	else
		dma_s_cfg_reg |= DMA_PACK_PATTERN_RGB;

	if (outBpp == 4)
		dma_s_cfg_reg |= DMA_IBUF_C3ALPHA_EN;

	if (outBpp == 2)
		dma_s_cfg_reg |= DMA_IBUF_FORMAT_RGB565;

	if (mfd->panel_info.pdest != DISPLAY_2) {
		printk(KERN_ERR "error: non-secondary type through dma_s!\n");
		return;
	}

	if (mfd->panel_info.type == MDDI_PANEL ||
		mfd->panel_info.type == EXT_MDDI_PANEL) {
		dma_s_cfg_reg |= DMA_OUT_SEL_MDDI;
		mddi_dest = TRUE;
	} else {
		dma_s_cfg_reg |= DMA_AHBM_LCD_SEL_SECONDARY;
		outp32(MDP_EBI2_LCD1, mfd->data_port_phys);
	}

	src = (uint8 *) iBuf->buf;
	/* starting input address */
	src += (iBuf->dma_x + iBuf->dma_y * iBuf->ibuf_width) * outBpp;

	/* MDP cmd block enable */
	mdp_pipe_ctrl(MDP_CMD_BLOCK, MDP_BLOCK_POWER_ON, FALSE);
	/* PIXELSIZE */
	if (mfd->panel_info.type == MDDI_PANEL) {
		MDP_OUTP(MDP_BASE + 0xa0004,
			(iBuf->dma_h << 16 | iBuf->dma_w));
		MDP_OUTP(MDP_BASE + 0xa0008, src);	/* ibuf address */
		MDP_OUTP(MDP_BASE + 0xa000c,
			iBuf->ibuf_width * outBpp);/* ystride */
	} else {
		MDP_OUTP(MDP_BASE + 0xb0004,
			(iBuf->dma_h << 16 | iBuf->dma_w));
		MDP_OUTP(MDP_BASE + 0xb0008, src);	/* ibuf address */
		MDP_OUTP(MDP_BASE + 0xb000c,
			iBuf->ibuf_width * outBpp);/* ystride */
	}

	if (mfd->panel_info.bpp == 18) {
		dma_s_cfg_reg |= DMA_DSTC0G_6BITS |	/* 666 18BPP */
		    DMA_DSTC1B_6BITS | DMA_DSTC2R_6BITS;
	} else {
		dma_s_cfg_reg |= DMA_DSTC0G_6BITS |	/* 565 16BPP */
		    DMA_DSTC1B_5BITS | DMA_DSTC2R_5BITS;
	}

	if (mddi_dest) {
		if (mfd->panel_info.type == MDDI_PANEL) {
			MDP_OUTP(MDP_BASE + 0xa0010,
				(iBuf->dma_y << 16) | iBuf->dma_x);
			MDP_OUTP(MDP_BASE + 0x00090, 1);
		} else {
			MDP_OUTP(MDP_BASE + 0xb0010,
				(iBuf->dma_y << 16) | iBuf->dma_x);
			MDP_OUTP(MDP_BASE + 0x00090, 2);
		}
		MDP_OUTP(MDP_BASE + 0x00094,
				(MDDI_VDO_PACKET_DESC << 16) |
				mfd->panel_info.mddi.vdopkt);
	} else {
		/* setting LCDC write window */
		pdata->set_rect(iBuf->dma_x, iBuf->dma_y, iBuf->dma_w,
				iBuf->dma_h);
	}

	if (mfd->panel_info.type == MDDI_PANEL)
		MDP_OUTP(MDP_BASE + 0xa0000, dma_s_cfg_reg);
	else
		MDP_OUTP(MDP_BASE + 0xb0000, dma_s_cfg_reg);


	/* MDP cmd block disable */
	mdp_pipe_ctrl(MDP_CMD_BLOCK, MDP_BLOCK_POWER_OFF, FALSE);
	if (mfd->panel_info.type == MDDI_PANEL)
		mdp_pipe_kickoff(MDP_DMA_S_TERM, mfd);
	else
		mdp_pipe_kickoff(MDP_DMA_E_TERM, mfd);

}

void mdp_dma_s_update(struct msm_fb_data_type *mfd)
{
	down(&mfd->dma->mutex);
	if ((mfd) && (!mfd->dma->busy) && (!mdp_fb_is_power_off(mfd))) {
		down(&mfd->sem);
		mdp_enable_irq(MDP_DMA_S_TERM);
		if (mfd->panel_info.type == MDDI_PANEL)
			mdp_enable_irq(MDP_DMA_S_TERM);
		else
			mdp_enable_irq(MDP_DMA_E_TERM);
		mfd->dma->busy = TRUE;
		INIT_COMPLETION(mfd->dma->comp);
		mfd->ibuf_flushed = TRUE;
		mdp_dma_s_update_lcd(mfd);
		up(&mfd->sem);

		/* wait until DMA finishes the current job */
		wait_for_completion_killable(&mfd->dma->comp);
		if (mfd->panel_info.type == MDDI_PANEL)
			mdp_disable_irq(MDP_DMA_S_TERM);
		else
			mdp_disable_irq(MDP_DMA_E_TERM);

	/* signal if pan function is waiting for the update completion */
		if (mfd->pan_waiting) {
			mfd->pan_waiting = FALSE;
			complete(&mfd->pan_comp);
		}
	}
	up(&mfd->dma->mutex);
}
