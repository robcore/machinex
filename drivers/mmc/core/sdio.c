/*
 *  linux/drivers/mmc/sdio.c
 *
 *  Copyright 2006-2007 Pierre Ossman
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 */

#include <linux/err.h>
#include <linux/module.h>
#include <linux/pm_runtime.h>

#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sdio.h>
#include <linux/mmc/sdio_func.h>
#include <linux/mmc/sdio_ids.h>

#include "core.h"
#include "bus.h"
#include "sd.h"
#include "sdio_bus.h"
#include "mmc_ops.h"
#include "sd_ops.h"
#include "sdio_ops.h"
#include "sdio_cis.h"

#ifdef CONFIG_MMC_EMBEDDED_SDIO
#include <linux/mmc/sdio_ids.h>
#endif

static int sdio_read_fbr(struct sdio_func *func)
{
	int ret;
	unsigned char data;

	if (mmc_card_nonstd_func_interface(func->card)) {
		func->class = SDIO_CLASS_NONE;
		return 0;
	}

	ret = mmc_io_rw_direct(func->card, 0, 0,
		SDIO_FBR_BASE(func->num) + SDIO_FBR_STD_IF, 0, &data);
	if (ret)
		goto out;

	data &= 0x0f;

	if (data == 0x0f) {
		ret = mmc_io_rw_direct(func->card, 0, 0,
			SDIO_FBR_BASE(func->num) + SDIO_FBR_STD_IF_EXT, 0, &data);
		if (ret)
			goto out;
	}

	func->class = data;

out:
	return ret;
}

static int sdio_init_func(struct mmc_card *card, unsigned int fn)
{
	int ret;
	struct sdio_func *func;

	BUG_ON(fn > SDIO_MAX_FUNCS);

	func = sdio_alloc_func(card);
	if (IS_ERR(func))
		return PTR_ERR(func);

	func->num = fn;

	if (!(card->quirks & MMC_QUIRK_NONSTD_SDIO)) {
		ret = sdio_read_fbr(func);
		if (ret)
			goto fail;

		ret = sdio_read_func_cis(func);
		if (ret)
			goto fail;
	} else {
		func->vendor = func->card->cis.vendor;
		func->device = func->card->cis.device;
		func->max_blksize = func->card->cis.blksize;
	}

	card->sdio_func[fn - 1] = func;

	return 0;

fail:
	/*
	 * It is okay to remove the function here even though we hold
	 * the host lock as we haven't registered the device yet.
	 */
	sdio_remove_func(func);
	return ret;
}

static int sdio_read_cccr(struct mmc_card *card, u32 ocr)
{
	int ret;
	int cccr_vsn;
	int uhs = ocr & R4_18V_PRESENT;
	unsigned char data;
	unsigned char speed;

	memset(&card->cccr, 0, sizeof(struct sdio_cccr));

	ret = mmc_io_rw_direct(card, 0, 0, SDIO_CCCR_CCCR, 0, &data);
	if (ret)
		goto out;

	cccr_vsn = data & 0x0f;

	if (cccr_vsn > SDIO_CCCR_REV_3_00) {
		pr_err("%s: unrecognised CCCR structure version %d\n",
			mmc_hostname(card->host), cccr_vsn);
		return -EINVAL;
	}

	card->cccr.sdio_vsn = (data & 0xf0) >> 4;

	ret = mmc_io_rw_direct(card, 0, 0, SDIO_CCCR_CAPS, 0, &data);
	if (ret)
		goto out;

	if (data & SDIO_CCCR_CAP_SMB)
		card->cccr.multi_block = 1;
	if (data & SDIO_CCCR_CAP_LSC)
		card->cccr.low_speed = 1;
	if (data & SDIO_CCCR_CAP_4BLS)
		card->cccr.wide_bus = 1;

	if (cccr_vsn >= SDIO_CCCR_REV_1_10) {
		ret = mmc_io_rw_direct(card, 0, 0, SDIO_CCCR_POWER, 0, &data);
		if (ret)
			goto out;

		if (data & SDIO_POWER_SMPC)
			card->cccr.high_power = 1;
	}

	if (cccr_vsn >= SDIO_CCCR_REV_1_20) {
		ret = mmc_io_rw_direct(card, 0, 0, SDIO_CCCR_SPEED, 0, &speed);
		if (ret)
			goto out;

		card->scr.sda_spec3 = 0;
		card->sw_caps.sd3_bus_mode = 0;
		card->sw_caps.sd3_drv_type = 0;
		if (cccr_vsn >= SDIO_CCCR_REV_3_00 && uhs) {
			card->scr.sda_spec3 = 1;
			ret = mmc_io_rw_direct(card, 0, 0,
				SDIO_CCCR_UHS, 0, &data);
			if (ret)
				goto out;

			if (mmc_host_uhs(card->host)) {
				if (data & SDIO_UHS_DDR50)
					card->sw_caps.sd3_bus_mode
						|= SD_MODE_UHS_DDR50;

				if (data & SDIO_UHS_SDR50)
					card->sw_caps.sd3_bus_mode
						|= SD_MODE_UHS_SDR50;

				if (data & SDIO_UHS_SDR104)
					card->sw_caps.sd3_bus_mode
						|= SD_MODE_UHS_SDR104;
			}

			ret = mmc_io_rw_direct(card, 0, 0,
				SDIO_CCCR_DRIVE_STRENGTH, 0, &data);
			if (ret)
				goto out;

			if (data & SDIO_DRIVE_SDTA)
				card->sw_caps.sd3_drv_type |= SD_DRIVER_TYPE_A;
			if (data & SDIO_DRIVE_SDTC)
				card->sw_caps.sd3_drv_type |= SD_DRIVER_TYPE_C;
			if (data & SDIO_DRIVE_SDTD)
				card->sw_caps.sd3_drv_type |= SD_DRIVER_TYPE_D;

			ret = mmc_io_rw_direct(card, 0, 0,
				SDIO_CCCR_INTERRUPT_EXTENSION, 0, &data);
			if (ret)
				goto out;
			if (data & SDIO_SUPPORT_ASYNC_INTR) {
				if (card->host->caps2 &
				    MMC_CAP2_ASYNC_SDIO_IRQ_4BIT_MODE) {
					data |= SDIO_ENABLE_ASYNC_INTR;
					ret = mmc_io_rw_direct(card, 1, 0,
						SDIO_CCCR_INTERRUPT_EXTENSION,
						data, NULL);
					if (ret)
						goto out;
					card->cccr.async_intr_sup = 1;
				}
			}
		}

		/* if no uhs mode ensure we check for high speed */
		if (!card->sw_caps.sd3_bus_mode) {
			if (speed & SDIO_SPEED_SHS) {
				card->cccr.high_speed = 1;
				card->sw_caps.hs_max_dtr = 50000000;
			} else {
				card->cccr.high_speed = 0;
				card->sw_caps.hs_max_dtr = 25000000;
			}
		}
	}

out:
	return ret;
}

static int sdio_enable_wide(struct mmc_card *card)
{
	int ret;
	u8 ctrl;

	if (!(card->host->caps & (MMC_CAP_4_BIT_DATA | MMC_CAP_8_BIT_DATA)))
		return 0;

	if (card->cccr.low_speed && !card->cccr.wide_bus)
		return 0;

	ret = mmc_io_rw_direct(card, 0, 0, SDIO_CCCR_IF, 0, &ctrl);
	if (ret)
		return ret;

	if (card->host->caps & MMC_CAP_8_BIT_DATA)
		ctrl |= SDIO_BUS_WIDTH_8BIT;
	else if (card->host->caps & MMC_CAP_4_BIT_DATA)
		ctrl |= SDIO_BUS_WIDTH_4BIT;

	ret = mmc_io_rw_direct(card, 1, 0, SDIO_CCCR_IF, ctrl, NULL);
	if (ret)
		return ret;

	return 1;
}

/*
 * If desired, disconnect the pull-up resistor on CD/DAT[3] (pin 1)
 * of the card. This may be required on certain setups of boards,
 * controllers and embedded sdio device which do not need the card's
 * pull-up. As a result, card detection is disabled and power is saved.
 */
static int sdio_disable_cd(struct mmc_card *card)
{
	int ret;
	u8 ctrl;

	if (!mmc_card_disable_cd(card))
		return 0;

	ret = mmc_io_rw_direct(card, 0, 0, SDIO_CCCR_IF, 0, &ctrl);
	if (ret)
		return ret;

	ctrl |= SDIO_BUS_CD_DISABLE;

	return mmc_io_rw_direct(card, 1, 0, SDIO_CCCR_IF, ctrl, NULL);
}

/*
 * Devices that remain active during a system suspend are
 * put back into 1-bit mode.
 */
static int sdio_disable_wide(struct mmc_card *card)
{
	int ret;
	u8 ctrl;

	if (!(card->host->caps & (MMC_CAP_4_BIT_DATA | MMC_CAP_8_BIT_DATA)))
		return 0;

	if (card->cccr.low_speed && !card->cccr.wide_bus)
		return 0;

	ret = mmc_io_rw_direct(card, 0, 0, SDIO_CCCR_IF, 0, &ctrl);
	if (ret)
		return ret;

	if (!(ctrl & (SDIO_BUS_WIDTH_4BIT | SDIO_BUS_WIDTH_8BIT)))
		return 0;

	ctrl &= ~(SDIO_BUS_WIDTH_4BIT | SDIO_BUS_WIDTH_8BIT);
	ctrl |= SDIO_BUS_ASYNC_INT;

	ret = mmc_io_rw_direct(card, 1, 0, SDIO_CCCR_IF, ctrl, NULL);
	if (ret)
		return ret;

	mmc_set_bus_width(card->host, MMC_BUS_WIDTH_1);

	return 0;
}


static int sdio_enable_4bit_bus(struct mmc_card *card)
{
	int err;

	if (card->type == MMC_TYPE_SDIO)
		return sdio_enable_wide(card);

	if ((card->host->caps & MMC_CAP_4_BIT_DATA) &&
		(card->scr.bus_widths & SD_SCR_BUS_WIDTH_4)) {
		err = mmc_app_set_bus_width(card, MMC_BUS_WIDTH_4);
		if (err)
			return err;
	} else
		return 0;

	err = sdio_enable_wide(card);
	if (err <= 0)
		mmc_app_set_bus_width(card, MMC_BUS_WIDTH_1);

	return err;
}


/*
 * Test if the card supports high-speed mode and, if so, switch to it.
 */
static int mmc_sdio_switch_hs(struct mmc_card *card, int enable)
{
	int ret;
	u8 speed;

	if (!(card->host->caps & MMC_CAP_SD_HIGHSPEED))
		return 0;

	if (!card->cccr.high_speed)
		return 0;

	ret = mmc_io_rw_direct(card, 0, 0, SDIO_CCCR_SPEED, 0, &speed);
	if (ret)
		return ret;

	if (enable)
		speed |= SDIO_SPEED_EHS;
	else
		speed &= ~SDIO_SPEED_EHS;

	ret = mmc_io_rw_direct(card, 1, 0, SDIO_CCCR_SPEED, speed, NULL);
	if (ret)
		return ret;

	return 1;
}

/*
 * Enable SDIO/combo card's high-speed mode. Return 0/1 if [not]supported.
 */
static int sdio_enable_hs(struct mmc_card *card)
{
	int ret;

	ret = mmc_sdio_switch_hs(card, true);
	if (ret <= 0 || card->type == MMC_TYPE_SDIO)
		return ret;

	ret = mmc_sd_switch_hs(card);
	if (ret <= 0)
		mmc_sdio_switch_hs(card, false);

	return ret;
}

static unsigned mmc_sdio_get_max_clock(struct mmc_card *card)
{
	unsigned max_dtr;

	if (mmc_card_highspeed(card)) {
		/*
		 * The SDIO specification doesn't mention how
		 * the CIS transfer speed register relates to
		 * high-speed, but it seems that 50 MHz is
		 * mandatory.
		 */
		max_dtr = 50000000;
	} else {
		max_dtr = card->cis.max_dtr;
	}

	if (card->type == MMC_TYPE_SD_COMBO)
		max_dtr = min(max_dtr, mmc_sd_get_max_clock(card));

	return max_dtr;
}

static unsigned char host_drive_to_sdio_drive(int host_strength)
{
	switch (host_strength) {
	case MMC_SET_DRIVER_TYPE_A:
		return SDIO_DTSx_SET_TYPE_A;
	case MMC_SET_DRIVER_TYPE_B:
		return SDIO_DTSx_SET_TYPE_B;
	case MMC_SET_DRIVER_TYPE_C:
		return SDIO_DTSx_SET_TYPE_C;
	case MMC_SET_DRIVER_TYPE_D:
		return SDIO_DTSx_SET_TYPE_D;
	default:
		return SDIO_DTSx_SET_TYPE_B;
	}
}

static void sdio_select_driver_type(struct mmc_card *card)
{
	int host_drv_type = SD_DRIVER_TYPE_B;
	int card_drv_type = SD_DRIVER_TYPE_B;
	int drive_strength;
	unsigned char card_strength;
	int err;

	/*
	 * If the host doesn't support any of the Driver Types A,C or D,
	 * or there is no board specific handler then default Driver
	 * Type B is used.
	 */
	if (!(card->host->caps &
		(MMC_CAP_DRIVER_TYPE_A |
		 MMC_CAP_DRIVER_TYPE_C |
		 MMC_CAP_DRIVER_TYPE_D)))
		return;

	if (!card->host->ops->select_drive_strength)
		return;

	if (card->host->caps & MMC_CAP_DRIVER_TYPE_A)
		host_drv_type |= SD_DRIVER_TYPE_A;

	if (card->host->caps & MMC_CAP_DRIVER_TYPE_C)
		host_drv_type |= SD_DRIVER_TYPE_C;

	if (card->host->caps & MMC_CAP_DRIVER_TYPE_D)
		host_drv_type |= SD_DRIVER_TYPE_D;

	if (card->sw_caps.sd3_drv_type & SD_DRIVER_TYPE_A)
		card_drv_type |= SD_DRIVER_TYPE_A;

	if (card->sw_caps.sd3_drv_type & SD_DRIVER_TYPE_C)
		card_drv_type |= SD_DRIVER_TYPE_C;

	if (card->sw_caps.sd3_drv_type & SD_DRIVER_TYPE_D)
		card_drv_type |= SD_DRIVER_TYPE_D;

	/*
	 * The drive strength that the hardware can support
	 * depends on the board design.  Pass the appropriate
	 * information and let the hardware specific code
	 * return what is possible given the options
	 */
	drive_strength = card->host->ops->select_drive_strength(
		card->sw_caps.uhs_max_dtr,
		host_drv_type, card_drv_type);

	/* if error just use default for drive strength B */
	err = mmc_io_rw_direct(card, 0, 0, SDIO_CCCR_DRIVE_STRENGTH, 0,
		&card_strength);
	if (err)
		return;

	card_strength &= ~(SDIO_DRIVE_DTSx_MASK<<SDIO_DRIVE_DTSx_SHIFT);
	card_strength |= host_drive_to_sdio_drive(drive_strength);

	err = mmc_io_rw_direct(card, 1, 0, SDIO_CCCR_DRIVE_STRENGTH,
		card_strength, NULL);

	/* if error default to drive strength B */
	if (!err)
		mmc_set_driver_type(card->host, drive_strength);
}


static int sdio_set_bus_speed_mode(struct mmc_card *card)
{
	unsigned int bus_speed, timing;
	int err;
	unsigned char speed;

	/*
	 * If the host doesn't support any of the UHS-I modes, fallback on
	 * default speed.
	 */
	if (!mmc_host_uhs(card->host))
		return 0;

	bus_speed = SDIO_SPEED_SDR12;
	timing = MMC_TIMING_UHS_SDR12;
	if ((card->host->caps & MMC_CAP_UHS_SDR104) &&
	    (card->sw_caps.sd3_bus_mode & SD_MODE_UHS_SDR104)) {
			bus_speed = SDIO_SPEED_SDR104;
			timing = MMC_TIMING_UHS_SDR104;
			card->sw_caps.uhs_max_dtr = UHS_SDR104_MAX_DTR;
			card->sd_bus_speed = UHS_SDR104_BUS_SPEED;
	} else if ((card->host->caps & MMC_CAP_UHS_DDR50) &&
		   (card->sw_caps.sd3_bus_mode & SD_MODE_UHS_DDR50)) {
			bus_speed = SDIO_SPEED_DDR50;
			timing = MMC_TIMING_UHS_DDR50;
			card->sw_caps.uhs_max_dtr = UHS_DDR50_MAX_DTR;
			card->sd_bus_speed = UHS_DDR50_BUS_SPEED;
	} else if ((card->host->caps & (MMC_CAP_UHS_SDR104 |
		    MMC_CAP_UHS_SDR50)) && (card->sw_caps.sd3_bus_mode &
		    SD_MODE_UHS_SDR50)) {
			bus_speed = SDIO_SPEED_SDR50;
			timing = MMC_TIMING_UHS_SDR50;
			card->sw_caps.uhs_max_dtr = UHS_SDR50_MAX_DTR;
			card->sd_bus_speed = UHS_SDR50_BUS_SPEED;
	} else if ((card->host->caps & (MMC_CAP_UHS_SDR104 |
		    MMC_CAP_UHS_SDR50 | MMC_CAP_UHS_SDR25)) &&
		   (card->sw_caps.sd3_bus_mode & SD_MODE_UHS_SDR25)) {
			bus_speed = SDIO_SPEED_SDR25;
			timing = MMC_TIMING_UHS_SDR25;
			card->sw_caps.uhs_max_dtr = UHS_SDR25_MAX_DTR;
			card->sd_bus_speed = UHS_SDR25_BUS_SPEED;
	} else if ((card->host->caps & (MMC_CAP_UHS_SDR104 |
		    MMC_CAP_UHS_SDR50 | MMC_CAP_UHS_SDR25 |
		    MMC_CAP_UHS_SDR12)) && (card->sw_caps.sd3_bus_mode &
		    SD_MODE_UHS_SDR12)) {
			bus_speed = SDIO_SPEED_SDR12;
			timing = MMC_TIMING_UHS_SDR12;
			card->sw_caps.uhs_max_dtr = UHS_SDR12_MAX_DTR;
			card->sd_bus_speed = UHS_SDR12_BUS_SPEED;
	}

	err = mmc_io_rw_direct(card, 0, 0, SDIO_CCCR_SPEED, 0, &speed);
	if (err)
		return err;

	speed &= ~SDIO_SPEED_BSS_MASK;
	speed |= bus_speed;
	err = mmc_io_rw_direct(card, 1, 0, SDIO_CCCR_SPEED, speed, NULL);
	if (err)
		return err;

	if (bus_speed) {
		mmc_set_timing(card->host, timing);
		mmc_set_clock(card->host, card->sw_caps.uhs_max_dtr);
	}

	return 0;
}

/*
 * UHS-I specific initialization procedure
 */
static int mmc_sdio_init_uhs_card(struct mmc_card *card)
{
	int err;

	if (!card->scr.sda_spec3)
		return 0;

	/*
	 * Switch to wider bus (if supported).
	 */
	if (card->host->caps & MMC_CAP_4_BIT_DATA) {
		err = sdio_enable_4bit_bus(card);
		if (err > 0) {
			mmc_set_bus_width(card->host, MMC_BUS_WIDTH_4);
			err = 0;
		}
	}

	/* Set the driver strength for the card */
	sdio_select_driver_type(card);

	/* Set bus speed mode of the card */
	err = sdio_set_bus_speed_mode(card);
	if (err)
		goto out;

#if defined(CONFIG_BCM4334) || defined(CONFIG_BCM4334_MODULE) || defined(CONFIG_BCM4335) || defined(CONFIG_BCM4335_MODULE)
	/*
	* Prevent tuning operation when init a card
	* for WiFi operation with sdmmc.
	*/
	if (!strcmp(mmc_hostname(card->host), "mmc1"))
		printk("%s: remove initialize and start re-tuning timer"
			"to prevent CMD53 request timeout \n", mmc_hostname(card->host));
	else
#endif
	/* Initialize and start re-tuning timer */
	if (!mmc_host_is_spi(card->host) && card->host->ops->execute_tuning)
		err = card->host->ops->execute_tuning(card->host,
						      MMC_SEND_TUNING_BLOCK);

out:

	return err;
}

/*
 * Handle the detection and initialisation of a card.
 *
 * In the case of a resume, "oldcard" will contain the card
 * we're trying to reinitialise.
 */
static int mmc_sdio_init_card(struct mmc_host *host, u32 ocr,
			      struct mmc_card *oldcard, int powered_resume)
{
	struct mmc_card *card;
	int err;

	BUG_ON(!host);
	WARN_ON(!host->claimed);

#if defined(CONFIG_BCM4335) || defined(CONFIG_BCM4335_MODULE)
	/* If host that supports UHS-I sets S18R to 1 in arg of CMD5 to request
	 * change of signaling level to 1.8V
	 */
	if (host->caps &
			(MMC_CAP_UHS_SDR12 | MMC_CAP_UHS_SDR25 |
			 MMC_CAP_UHS_SDR50 | MMC_CAP_UHS_SDR104 |
			 MMC_CAP_UHS_DDR50))
		host->ocr |= R4_18V_PRESENT;
#endif

	/*
	 * Inform the card of the voltage
	 */
	if (!powered_resume) {
		err = mmc_send_io_op_cond(host, host->ocr, &ocr);
		if (err)
			goto err;
	}

	/*
	 * For SPI, enable CRC as appropriate.
	 */
	if (mmc_host_is_spi(host)) {
		err = mmc_spi_set_crc(host, use_spi_crc);
		if (err)
			goto err;
	}

	/*
	 * Allocate card structure.
	 */
	card = mmc_alloc_card(host, NULL);
	if (IS_ERR(card)) {
		err = PTR_ERR(card);
		goto err;
	}

	if ((ocr & R4_MEMORY_PRESENT) &&
	    mmc_sd_get_cid(host, host->ocr & ocr, card->raw_cid, NULL) == 0) {
		card->type = MMC_TYPE_SD_COMBO;

		if (oldcard && (oldcard->type != MMC_TYPE_SD_COMBO ||
		    memcmp(card->raw_cid, oldcard->raw_cid, sizeof(card->raw_cid)) != 0)) {
			mmc_remove_card(card);
			return -ENOENT;
		}
	} else {
		card->type = MMC_TYPE_SDIO;

		if (oldcard && oldcard->type != MMC_TYPE_SDIO) {
			mmc_remove_card(card);
			return -ENOENT;
		}
	}

	/*
	 * Call the optional HC's init_card function to handle quirks.
	 */
	if (host->ops->init_card) {
		mmc_host_clk_hold(host);
		host->ops->init_card(host, card);
		mmc_host_clk_release(host);
	}

	/*
	 * If the host and card support UHS-I mode request the card
	 * to switch to 1.8V signaling level.  No 1.8v signalling if
	 * UHS mode is not enabled to maintain compatibilty and some
	 * systems that claim 1.8v signalling in fact do not support
	 * it.
	 */
	if ((ocr & R4_18V_PRESENT) && mmc_host_uhs(host)) {
		err = mmc_set_signal_voltage(host, MMC_SIGNAL_VOLTAGE_180,
				true);
		if (err) {
			ocr &= ~R4_18V_PRESENT;
			host->ocr &= ~R4_18V_PRESENT;
		}
		err = 0;
	} else {
		ocr &= ~R4_18V_PRESENT;
		host->ocr &= ~R4_18V_PRESENT;
	}

	/*
	 * For native busses:  set card RCA and quit open drain mode.
	 */
	if (!powered_resume && !mmc_host_is_spi(host)) {
		err = mmc_send_relative_addr(host, &card->rca);
		if (err)
			goto remove;

		/*
		 * Update oldcard with the new RCA received from the SDIO
		 * device -- we're doing this so that it's updated in the
		 * "card" struct when oldcard overwrites that later.
		 */
		if (oldcard)
			oldcard->rca = card->rca;
	}

	/*
	 * Read CSD, before selecting the card
	 */
	if (!oldcard && card->type == MMC_TYPE_SD_COMBO) {
		err = mmc_sd_get_csd(host, card);
		if (err)
			return err;

		mmc_decode_cid(card);
	}

	/*
	 * Select card, as all following commands rely on that.
	 */
	if (!powered_resume && !mmc_host_is_spi(host)) {
		err = mmc_select_card(card);
		if (err)
			goto remove;
	}

	if (card->quirks & MMC_QUIRK_NONSTD_SDIO) {
		/*
		 * This is non-standard SDIO device, meaning it doesn't
		 * have any CIA (Common I/O area) registers present.
		 * It's host's responsibility to fill cccr and cis
		 * structures in init_card().
		 */
		mmc_set_clock(host, card->cis.max_dtr);

		if (card->cccr.high_speed) {
			mmc_card_set_highspeed(card);
			mmc_set_timing(card->host, MMC_TIMING_SD_HS);
		}

		goto finish;
	}

#ifdef CONFIG_MMC_EMBEDDED_SDIO
	if (host->embedded_sdio_data.cccr)
		memcpy(&card->cccr, host->embedded_sdio_data.cccr, sizeof(struct sdio_cccr));
	else {
#endif
		/*
		 * Read the common registers.
		 */
		err = sdio_read_cccr(card,  ocr);
		if (err)
			goto remove;
#ifdef CONFIG_MMC_EMBEDDED_SDIO
	}
#endif

#ifdef CONFIG_MMC_EMBEDDED_SDIO
	if (host->embedded_sdio_data.cis)
		memcpy(&card->cis, host->embedded_sdio_data.cis, sizeof(struct sdio_cis));
	else {
#endif
		/*
		 * Read the common CIS tuples.
		 */
		err = sdio_read_common_cis(card);
		if (err)
			goto remove;
#ifdef CONFIG_MMC_EMBEDDED_SDIO
	}
#endif

	if (oldcard) {
		int same = (card->cis.vendor == oldcard->cis.vendor &&
			    card->cis.device == oldcard->cis.device);
		mmc_remove_card(card);
		if (!same)
			return -ENOENT;

		card = oldcard;
	}
	mmc_fixup_device(card, NULL);

	if (card->type == MMC_TYPE_SD_COMBO) {
		err = mmc_sd_setup_card(host, card, oldcard != NULL);
		/* handle as SDIO-only card if memory init failed */
		if (err) {
			mmc_go_idle(host);
			if (mmc_host_is_spi(host))
				/* should not fail, as it worked previously */
				mmc_spi_set_crc(host, use_spi_crc);
			card->type = MMC_TYPE_SDIO;
		} else
			card->dev.type = &sd_type;
	}

	/*
	 * If needed, disconnect card detection pull-up resistor.
	 */
	err = sdio_disable_cd(card);
	if (err)
		goto remove;

	/* Initialization sequence for UHS-I cards */
	/* Only if card supports 1.8v and UHS signaling */
	if ((ocr & R4_18V_PRESENT) && card->sw_caps.sd3_bus_mode) {
		err = mmc_sdio_init_uhs_card(card);
		if (err)
			goto remove;

		/* Card is an ultra-high-speed card */
		mmc_card_set_uhs(card);
	} else {
		/*
		 * Switch to high-speed (if supported).
		 */
		err = sdio_enable_hs(card);
		if (err > 0)
			mmc_sd_go_highspeed(card);
		else if (err)
			goto remove;

		/*
		 * Change to the card's maximum speed.
		 */
		mmc_set_clock(host, mmc_sdio_get_max_clock(card));

		/*
		 * Switch to wider bus (if supported).
		 */
		err = sdio_enable_4bit_bus(card);
		if (err > 0) {
			if (card->host->caps & MMC_CAP_8_BIT_DATA)
				mmc_set_bus_width(card->host, MMC_BUS_WIDTH_8);
			else if (card->host->caps & MMC_CAP_4_BIT_DATA)
				mmc_set_bus_width(card->host, MMC_BUS_WIDTH_4);
		} else if (err)
			goto remove;
	}
finish:
	if (!oldcard)
		host->card = card;
	return 0;

remove:
	if (!oldcard)
		mmc_remove_card(card);

err:
	return err;
}

/*
 * Host is being removed. Free up the current card.
 */
static void mmc_sdio_remove(struct mmc_host *host)
{
	int i;

	BUG_ON(!host);
	BUG_ON(!host->card);

	for (i = 0;i < host->card->sdio_funcs;i++) {
		if (host->card->sdio_func[i]) {
			sdio_remove_func(host->card->sdio_func[i]);
			host->card->sdio_func[i] = NULL;
		}
	}

	mmc_remove_card(host->card);
	host->card = NULL;
}

/*
 * Card detection - card is alive.
 */
static int mmc_sdio_alive(struct mmc_host *host)
{
	return mmc_select_card(host->card);
}

/*
 * Card detection callback from host.
 */
static void mmc_sdio_detect(struct mmc_host *host)
{
	int err;

	BUG_ON(!host);
	BUG_ON(!host->card);

	/* Make sure card is powered before detecting it */
	if (host->caps & MMC_CAP_POWER_OFF_CARD) {
		err = pm_runtime_get_sync(&host->card->dev);
		if (err < 0) {
			pm_runtime_put_noidle(&host->card->dev);
			goto out;
		}
	}

	mmc_claim_host(host);

	/*
	 * Just check if our card has been removed.
	 */
	err = _mmc_detect_card_removed(host);

	mmc_release_host(host);

	/*
	 * Tell PM core it's OK to power off the card now.
	 *
	 * The _sync variant is used in order to ensure that the card
	 * is left powered off in case an error occurred, and the card
	 * is going to be removed.
	 *
	 * Since there is no specific reason to believe a new user
	 * is about to show up at this point, the _sync variant is
	 * desirable anyway.
	 */
	if (host->caps & MMC_CAP_POWER_OFF_CARD)
		pm_runtime_put_sync(&host->card->dev);

out:
	if (err) {
		mmc_sdio_remove(host);

		mmc_claim_host(host);
		mmc_detach_bus(host);
		mmc_power_off(host);
		mmc_release_host(host);
	}
}

/*
 * SDIO suspend.  We need to suspend all functions separately.
 * Therefore all registered functions must have drivers with suspend
 * and resume methods.  Failing that we simply remove the whole card.
 */
static int mmc_sdio_suspend(struct mmc_host *host)
{
	int i, err = 0;

	for (i = 0; i < host->card->sdio_funcs; i++) {
		struct sdio_func *func = host->card->sdio_func[i];
		if (func && sdio_func_present(func) && func->dev.driver) {
			const struct dev_pm_ops *pmops = func->dev.driver->pm;
			if (!pmops || !pmops->suspend || !pmops->resume) {
				/* force removal of entire card in that case */
				err = -ENOSYS;
			} else
				err = pmops->suspend(&func->dev);
			if (err)
				break;
		}
	}
	while (err && --i >= 0) {
		struct sdio_func *func = host->card->sdio_func[i];
		if (func && sdio_func_present(func) && func->dev.driver) {
			const struct dev_pm_ops *pmops = func->dev.driver->pm;
			pmops->resume(&func->dev);
		}
	}

	if (!err && mmc_card_keep_power(host) && mmc_card_wake_sdio_irq(host)) {
		mmc_claim_host(host);
		sdio_disable_wide(host->card);
		mmc_release_host(host);
	}

	return err;
}

static int mmc_sdio_resume(struct mmc_host *host)
{
	int i, err = 0;

	BUG_ON(!host);
	BUG_ON(!host->card);

	/* Basic card reinitialization. */
	mmc_claim_host(host);

	/* No need to reinitialize powered-resumed nonremovable cards */
	if (mmc_card_is_removable(host) || !mmc_card_keep_power(host)) {
		sdio_reset(host);
		mmc_go_idle(host);
		err = mmc_sdio_init_card(host, host->ocr, host->card,
					mmc_card_keep_power(host));
	} else if (mmc_card_keep_power(host) && mmc_card_wake_sdio_irq(host)) {
		/* We may have switched to 1-bit mode during suspend */
		err = sdio_enable_4bit_bus(host->card);
		if (err > 0) {
			if (host->caps & MMC_CAP_8_BIT_DATA)
				mmc_set_bus_width(host, MMC_BUS_WIDTH_8);
			else if (host->caps & MMC_CAP_4_BIT_DATA)
				mmc_set_bus_width(host, MMC_BUS_WIDTH_4);
			err = 0;
		}
	}

	if (!err && host->sdio_irqs)
		mmc_signal_sdio_irq(host);
	mmc_release_host(host);

	/*
	 * If the card looked to be the same as before suspending, then
	 * we proceed to resume all card functions.  If one of them returns
	 * an error then we simply return that error to the core and the
	 * card will be redetected as new.  It is the responsibility of
	 * the function driver to perform further tests with the extra
	 * knowledge it has of the card to confirm the card is indeed the
	 * same as before suspending (same MAC address for network cards,
	 * etc.) and return an error otherwise.
	 */
	for (i = 0; !err && i < host->card->sdio_funcs; i++) {
		struct sdio_func *func = host->card->sdio_func[i];
		if (func && sdio_func_present(func) && func->dev.driver) {
			const struct dev_pm_ops *pmops = func->dev.driver->pm;
			err = pmops->resume(&func->dev);
		}
	}

	return err;
}

static int mmc_sdio_power_restore(struct mmc_host *host)
{
	int ret;
	u32 ocr;

	BUG_ON(!host);
	BUG_ON(!host->card);

	mmc_claim_host(host);

	/*
	 * Reset the card by performing the same steps that are taken by
	 * mmc_rescan_try_freq() and mmc_attach_sdio() during a "normal" probe.
	 *
	 * sdio_reset() is technically not needed. Having just powered up the
	 * hardware, it should already be in reset state. However, some
	 * platforms (such as SD8686 on OLPC) do not instantly cut power,
	 * meaning that a reset is required when restoring power soon after
	 * powering off. It is harmless in other cases.
	 *
	 * The CMD5 reset (mmc_send_io_op_cond()), according to the SDIO spec,
	 * is not necessary for non-removable cards. However, it is required
	 * for OLPC SD8686 (which expects a [CMD5,5,3,7] init sequence), and
	 * harmless in other situations.
	 *
	 * With these steps taken, mmc_select_voltage() is also required to
	 * restore the correct voltage setting of the card.
	 */

	sdio_reset(host);
	mmc_go_idle(host);
	mmc_send_if_cond(host, host->ocr_avail);

	ret = mmc_send_io_op_cond(host, 0, &ocr);
	if (ret)
		goto out;

	if (host->ocr_avail_sdio)
		host->ocr_avail = host->ocr_avail_sdio;

	host->ocr = mmc_select_voltage(host, ocr & ~0x7F);
	if (!host->ocr) {
		ret = -EINVAL;
		goto out;
	}

	if (mmc_host_uhs(host))
		/* to query card if 1.8V signalling is supported */
		host->ocr |= R4_18V_PRESENT;

	if (mmc_host_uhs(host))
		/* to query card if 1.8V signalling is supported */
		host->ocr |= R4_18V_PRESENT;

	ret = mmc_sdio_init_card(host, host->ocr, host->card,
				mmc_card_keep_power(host));
	if (!ret && host->sdio_irqs)
		mmc_signal_sdio_irq(host);

out:
	mmc_release_host(host);

	return ret;
}

static const struct mmc_bus_ops mmc_sdio_ops = {
	.remove = mmc_sdio_remove,
	.detect = mmc_sdio_detect,
	.suspend = mmc_sdio_suspend,
	.resume = mmc_sdio_resume,
	.power_restore = mmc_sdio_power_restore,
	.alive = mmc_sdio_alive,
};


/*
 * Starting point for SDIO card init.
 */
int mmc_attach_sdio(struct mmc_host *host)
{
	int err, i, funcs;
	u32 ocr;
	struct mmc_card *card;

	BUG_ON(!host);
	WARN_ON(!host->claimed);

	err = mmc_send_io_op_cond(host, 0, &ocr);
	if (err)
		return err;

	mmc_attach_bus(host, &mmc_sdio_ops);
	if (host->ocr_avail_sdio)
		host->ocr_avail = host->ocr_avail_sdio;

	/*
	 * Sanity check the voltages that the card claims to
	 * support.
	 */
	if (ocr & 0x7F) {
		pr_warning("%s: card claims to support voltages "
		       "below the defined range. These will be ignored.\n",
		       mmc_hostname(host));
		ocr &= ~0x7F;
	}

	host->ocr = mmc_select_voltage(host, ocr);

	/*
	 * Can we support the voltage(s) of the card(s)?
	 */
	if (!host->ocr) {
		err = -EINVAL;
		goto err;
	}

	/*
	 * Detect and init the card.
	 */
	if (mmc_host_uhs(host))
		/* to query card if 1.8V signalling is supported */
		host->ocr |= R4_18V_PRESENT;

	err = mmc_sdio_init_card(host, host->ocr, NULL, 0);
	if (err) {
		if (err == -EAGAIN) {
			/*
			 * Retry initialization with S18R set to 0.
			 */
			host->ocr &= ~R4_18V_PRESENT;
			err = mmc_sdio_init_card(host, host->ocr, NULL, 0);
		}
		if (err)
			goto err;
	}
	card = host->card;

	/*
	 * Enable runtime PM only if supported by host+card+board
	 */
	if (host->caps & MMC_CAP_POWER_OFF_CARD) {
		/*
		 * Let runtime PM core know our card is active
		 */
		err = pm_runtime_set_active(&card->dev);
		if (err)
			goto remove;

		/*
		 * Enable runtime PM for this card
		 */
		pm_runtime_enable(&card->dev);
	}

	/*
	 * The number of functions on the card is encoded inside
	 * the ocr.
	 */
	funcs = (ocr & 0x70000000) >> 28;
	card->sdio_funcs = 0;

#ifdef CONFIG_MMC_EMBEDDED_SDIO
	if (host->embedded_sdio_data.funcs)
		card->sdio_funcs = funcs = host->embedded_sdio_data.num_funcs;
#endif

	/*
	 * Initialize (but don't add) all present functions.
	 */
	for (i = 0; i < funcs; i++, card->sdio_funcs++) {
#ifdef CONFIG_MMC_EMBEDDED_SDIO
		if (host->embedded_sdio_data.funcs) {
			struct sdio_func *tmp;

			tmp = sdio_alloc_func(host->card);
			if (IS_ERR(tmp))
				goto remove;
			tmp->num = (i + 1);
			card->sdio_func[i] = tmp;
			tmp->class = host->embedded_sdio_data.funcs[i].f_class;
			tmp->max_blksize = host->embedded_sdio_data.funcs[i].f_maxblksize;
			tmp->vendor = card->cis.vendor;
			tmp->device = card->cis.device;
		} else {
#endif
			err = sdio_init_func(host->card, i + 1);
			if (err)
				goto remove;
#ifdef CONFIG_MMC_EMBEDDED_SDIO
		}
#endif
		/*
		 * Enable Runtime PM for this func (if supported)
		 */
		if (host->caps & MMC_CAP_POWER_OFF_CARD)
			pm_runtime_enable(&card->sdio_func[i]->dev);
	}

	/*
	 * First add the card to the driver model...
	 */
	mmc_release_host(host);
	err = mmc_add_card(host->card);
	if (err)
		goto remove_added;

	/*
	 * ...then the SDIO functions.
	 */
	for (i = 0;i < funcs;i++) {
		err = sdio_add_func(host->card->sdio_func[i]);
		if (err)
			goto remove_added;
	}

	mmc_claim_host(host);
	return 0;


remove_added:
	/* Remove without lock if the device has been added. */
	mmc_sdio_remove(host);
	mmc_claim_host(host);
remove:
	/* And with lock if it hasn't been added. */
	mmc_release_host(host);
	if (host->card)
		mmc_sdio_remove(host);
	mmc_claim_host(host);
err:
	mmc_detach_bus(host);

	pr_err("%s: error %d whilst initialising SDIO card\n",
		mmc_hostname(host), err);

	return err;
}

int sdio_reset_comm(struct mmc_card *card)
{
#if defined(CONFIG_BCM4334) || defined(CONFIG_BCM4334_MODULE) || defined(CONFIG_BCM4335) || defined(CONFIG_BCM4335_MODULE)
	struct mmc_host *host = card->host;
	u32 ocr;
	int err;

	printk("%s():\n", __func__);
	mmc_claim_host(host);

	mmc_go_idle(host);

	mmc_set_clock(host, host->f_min);

	err = mmc_send_io_op_cond(host, 0, &ocr);
	if (err)
		goto err;

	host->ocr = mmc_select_voltage(host, ocr);
	if (!host->ocr) {
		err = -EINVAL;
		goto err;
	}

	err = mmc_sdio_init_card(host, host->ocr, card, 0);
	if (err)
		goto err;

	mmc_release_host(host);
	return 0;
err:
	printk("%s: Error resetting SDIO communications (%d)\n",
	       mmc_hostname(host), err);
	mmc_release_host(host);
	return err;
#else
	return mmc_power_restore_host(card->host);
#endif /* CONFIG_BCM4335 || CONFIG_BCM4335_MODULE */
}
EXPORT_SYMBOL(sdio_reset_comm);
