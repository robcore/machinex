/* Copyright (c) 2013, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <sound/soc.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/mfd/wcd9xxx/wcd9xxx_registers.h>
#include "wcd9xxx-common.h"


#define CLSH_COMPUTE_EAR 0x01
#define CLSH_COMPUTE_HPH_L 0x02
#define CLSH_COMPUTE_HPH_R 0x03

#define BUCK_VREF_0P494V 0x3F
#define BUCK_VREF_2V 0xFF
#define BUCK_VREF_1P8V 0xE6

#define BUCK_SETTLE_TIME_US 50
#define NCP_SETTLE_TIME_US 50

static inline void
wcd9xxx_enable_clsh_block(struct snd_soc_codec *codec,
			  struct wcd9xxx_clsh_cdc_data *clsh_d, bool enable)
{
	if ((enable && ++clsh_d->clsh_users == 1) ||
	    (!enable && --clsh_d->clsh_users == 0))
		snd_soc_update_bits(codec, WCD9XXX_A_CDC_CLSH_B1_CTL,
				    0x01, enable ? 0x01 : 0x00);
	dev_dbg(codec->dev, "%s: clsh_users %d, enable %d", __func__,
		clsh_d->clsh_users, enable);
}


static inline void wcd9xxx_enable_anc_delay(
	struct snd_soc_codec *codec,
	bool on)
{
	snd_soc_update_bits(codec, WCD9XXX_A_CDC_CLSH_B1_CTL,
		0x02, on ? 0x02 : 0x00);
}

static inline void
wcd9xxx_enable_buck(struct snd_soc_codec *codec,
		    struct wcd9xxx_clsh_cdc_data *clsh_d, bool enable)
{
	if ((enable && ++clsh_d->buck_users == 1) ||
	    (!enable && --clsh_d->buck_users == 0))
		snd_soc_update_bits(codec, WCD9XXX_A_BUCK_MODE_1,
				    0x80, enable ? 0x80 : 0x00);
	dev_dbg(codec->dev, "%s: buck_users %d, enable %d", __func__,
		clsh_d->buck_users, enable);
}

static int cdc_lo_count;
static void (*clsh_state_fp[NUM_CLSH_STATES])(struct snd_soc_codec *,
					      struct wcd9xxx_clsh_cdc_data *,
					      u8 req_state, bool req_type);

static const char *state_to_str(u8 state, char *buf, size_t buflen)
{
	int i;
	int cnt = 0;
	/*
	 * This array of strings should match with enum wcd9xxx_clsh_state_bit.
	 */
	const char *states[] = {
		"STATE_EAR",
		"STATE_HPH_L",
		"STATE_HPH_R",
		"STATE_LO",
	};

	if (state == WCD9XXX_CLSH_STATE_IDLE) {
		snprintf(buf, buflen, "[STATE_IDLE]");
		goto done;
	}

	buf[0] = '\0';
	for (i = 0; i < ARRAY_SIZE(states); i++) {
		if (!(state & (1 << i)))
			continue;
		cnt = snprintf(buf, buflen - cnt - 1, "%s%s%s", buf,
			       buf[0] == '\0' ? "[" : "|",
			       states[i]);
	}
	if (cnt > 0)
		strlcat(buf + cnt, "]", buflen);

done:
	if (buf[0] == '\0')
		snprintf(buf, buflen, "[STATE_UNKNOWN]");
	return buf;
}

static void wcd9xxx_cfg_clsh_param_common(
		struct snd_soc_codec *codec)
{
	int i;
	const struct wcd9xxx_reg_mask_val reg_set[] = {
		{WCD9XXX_A_CDC_CLSH_BUCK_NCP_VARS, 0x3 << 0, 0},
		{WCD9XXX_A_CDC_CLSH_BUCK_NCP_VARS, 0x3 << 2, 1 << 2},
		{WCD9XXX_A_CDC_CLSH_BUCK_NCP_VARS, (0x1 << 4), 0},
		{WCD9XXX_A_CDC_CLSH_B2_CTL, (0x3 << 0), 0x01},
		{WCD9XXX_A_CDC_CLSH_B2_CTL, (0x3 << 2), (0x01 << 2)},
		{WCD9XXX_A_CDC_CLSH_B2_CTL, (0xf << 4), (0x03 << 4)},
		{WCD9XXX_A_CDC_CLSH_B3_CTL, (0xf << 4), (0x03 << 4)},
		{WCD9XXX_A_CDC_CLSH_B3_CTL, (0xf << 0), (0x0B)},
		{WCD9XXX_A_CDC_CLSH_B1_CTL, (0x1 << 5), (0x01 << 5)},
		{WCD9XXX_A_CDC_CLSH_B1_CTL, (0x1 << 1), (0x01 << 1)},
	};

	for (i = 0; i < ARRAY_SIZE(reg_set); i++)
		snd_soc_update_bits(codec, reg_set[i].reg, reg_set[i].mask,
						    reg_set[i].val);

	dev_dbg(codec->dev, "%s: Programmed class H controller common parameters",
			 __func__);
}

static void wcd9xxx_chargepump_request(
	struct snd_soc_codec *codec, bool on)
{
	static int cp_count;

	if (on && (++cp_count == 1)) {
		snd_soc_update_bits(codec, WCD9XXX_A_CDC_CLK_OTHR_CTL,
							0x01, 0x01);
		dev_info(codec->dev, "%s: Charge Pump enabled, count = %d\n",
				__func__, cp_count);
	}

	else if (!on) {
		if (--cp_count < 0) {
			dev_dbg(codec->dev,
				"%s: Unbalanced disable for charge pump\n",
				__func__);
			if (snd_soc_read(codec, WCD9XXX_A_CDC_CLK_OTHR_CTL) &
			    0x01) {
				dev_dbg(codec->dev,
					"%s: Actual chargepump is ON\n",
					__func__);
			}
			cp_count = 0;
			WARN_ON(1);
		}

		if (cp_count == 0) {
			snd_soc_update_bits(codec, WCD9XXX_A_CDC_CLK_OTHR_CTL,
					    0x01, 0x00);
			dev_dbg(codec->dev,
				"%s: Charge pump disabled, count = %d\n",
				__func__, cp_count);
		}
	}
}

static inline void wcd9xxx_clsh_computation_request(
	struct snd_soc_codec *codec, int compute_pa, bool on)
{
	u8 reg_val, reg_mask;

	switch (compute_pa) {
	case CLSH_COMPUTE_EAR:
		reg_mask = 0x10;
		reg_val = (on ? 0x10 : 0x00);
		break;
	case CLSH_COMPUTE_HPH_L:
		reg_mask = 0x08;
		reg_val = (on ? 0x08 : 0x00);
		break;
	case CLSH_COMPUTE_HPH_R:
		reg_mask = 0x04;
		reg_val = (on ? 0x04 : 0x00);
		break;
	default:
		dev_err(codec->dev, "%s: class h computation PA request incorrect\n",
			   __func__);
		return;
	}

	snd_soc_update_bits(codec, WCD9XXX_A_CDC_CLSH_B1_CTL,
						reg_mask, reg_val);

}

static void wcd9xxx_set_buck_mode(struct snd_soc_codec *codec, u8 buck_vref)
{
	int i;
	const struct wcd9xxx_reg_mask_val reg_set[] = {
		{WCD9XXX_A_BUCK_MODE_5, 0x02, 0x02},
		{WCD9XXX_A_BUCK_MODE_4, 0xFF, buck_vref},
		{WCD9XXX_A_BUCK_MODE_1, 0x04, 0x04},
		{WCD9XXX_A_BUCK_MODE_3, 0x04, 0x00},
		{WCD9XXX_A_BUCK_MODE_3, 0x08, 0x00},
	};

	for (i = 0; i < ARRAY_SIZE(reg_set); i++)
		snd_soc_update_bits(codec, reg_set[i].reg,
					reg_set[i].mask, reg_set[i].val);

	dev_dbg(codec->dev, "%s: Done\n", __func__);
	usleep_range(BUCK_SETTLE_TIME_US, BUCK_SETTLE_TIME_US);
}

static void wcd9xxx_clsh_enable_post_pa(struct snd_soc_codec *codec)
{
	int i;
	const struct wcd9xxx_reg_mask_val reg_set[] = {
		{WCD9XXX_A_BUCK_MODE_5, 0x02, 0x00},
		{WCD9XXX_A_NCP_STATIC, 0x20, 0x00},
		{WCD9XXX_A_BUCK_MODE_3, 0x04, 0x04},
		{WCD9XXX_A_BUCK_MODE_3, 0x08, 0x08},
	};

	for (i = 0; i < ARRAY_SIZE(reg_set); i++)
		snd_soc_update_bits(codec, reg_set[i].reg,
					reg_set[i].mask, reg_set[i].val);

	dev_dbg(codec->dev, "%s: completed clsh mode settings after PA enable\n",
		   __func__);

}

static void wcd9xxx_set_fclk_get_ncp(struct snd_soc_codec *codec,
				     struct wcd9xxx_clsh_cdc_data *clsh_d,
				     enum ncp_fclk_level fclk_level)
{
	clsh_d->ncp_users[fclk_level]++;

	pr_debug("%s: enter ncp type %d users fclk8 %d, fclk5 %d\n", __func__,
		 fclk_level, clsh_d->ncp_users[NCP_FCLK_LEVEL_8],
		 clsh_d->ncp_users[NCP_FCLK_LEVEL_5]);

	snd_soc_update_bits(codec, WCD9XXX_A_NCP_STATIC, 0x10, 0x00);
	/* fclk level 8 dominates level 5 */
	if (clsh_d->ncp_users[NCP_FCLK_LEVEL_8] > 0)
		snd_soc_update_bits(codec, WCD9XXX_A_NCP_STATIC, 0x0F, 0x08);
	else if (clsh_d->ncp_users[NCP_FCLK_LEVEL_5] > 0)
		snd_soc_update_bits(codec, WCD9XXX_A_NCP_STATIC, 0x0F, 0x05);
	else
		WARN_ONCE(1, "Unexpected users %d,%d\n",
			  clsh_d->ncp_users[NCP_FCLK_LEVEL_8],
			  clsh_d->ncp_users[NCP_FCLK_LEVEL_5]);
	snd_soc_update_bits(codec, WCD9XXX_A_NCP_STATIC, 0x20, 0x20);

	/* enable NCP and wait until settles down */
	if (snd_soc_update_bits(codec, WCD9XXX_A_NCP_EN, 0x01, 0x01))
		usleep_range(NCP_SETTLE_TIME_US, NCP_SETTLE_TIME_US);
	pr_debug("%s: leave\n", __func__);
}

static void wcd9xxx_set_fclk_put_ncp(struct snd_soc_codec *codec,
				     struct wcd9xxx_clsh_cdc_data *clsh_d,
				     enum ncp_fclk_level fclk_level)
{
	clsh_d->ncp_users[fclk_level]--;

	pr_debug("%s: enter ncp type %d users fclk8 %d, fclk5 %d\n", __func__,
		 fclk_level, clsh_d->ncp_users[NCP_FCLK_LEVEL_8],
		 clsh_d->ncp_users[NCP_FCLK_LEVEL_5]);

	if (clsh_d->ncp_users[NCP_FCLK_LEVEL_8] == 0 &&
	    clsh_d->ncp_users[NCP_FCLK_LEVEL_5] == 0)
		snd_soc_update_bits(codec, WCD9XXX_A_NCP_EN, 0x01, 0x00);
	else if (clsh_d->ncp_users[NCP_FCLK_LEVEL_8] == 0)
		/* if dominating level 8 has gone, switch to 5 */
		snd_soc_update_bits(codec, WCD9XXX_A_NCP_STATIC, 0x0F, 0x05);
	pr_debug("%s: leave\n", __func__);
}

static void wcd9xxx_cfg_clsh_param_ear(struct snd_soc_codec *codec)
{
	int i;
	const struct wcd9xxx_reg_mask_val reg_set[] = {
		{WCD9XXX_A_CDC_CLSH_B1_CTL, (0x1 << 7), 0},
		{WCD9XXX_A_CDC_CLSH_V_PA_HD_EAR, (0x3f << 0), 0x0D},
		{WCD9XXX_A_CDC_CLSH_V_PA_MIN_EAR, (0x3f << 0), 0x3A},

		/* Under assumption that EAR load is 10.7ohm */
		{WCD9XXX_A_CDC_CLSH_IDLE_EAR_THSD, (0x3f << 0), 0x26},
		{WCD9XXX_A_CDC_CLSH_FCLKONLY_EAR_THSD, (0x3f << 0), 0x2C},
		{WCD9XXX_A_CDC_CLSH_I_PA_FACT_EAR_L, 0xff, 0xA9},
		{WCD9XXX_A_CDC_CLSH_I_PA_FACT_EAR_U, 0xff, 0x07},
		{WCD9XXX_A_CDC_CLSH_K_ADDR, (0x1 << 7), 0},
		{WCD9XXX_A_CDC_CLSH_K_ADDR, (0xf << 0), 0x08},
		{WCD9XXX_A_CDC_CLSH_K_DATA, 0xff, 0x1b},
		{WCD9XXX_A_CDC_CLSH_K_DATA, 0xff, 0x00},
		{WCD9XXX_A_CDC_CLSH_K_DATA, 0xff, 0x2d},
		{WCD9XXX_A_CDC_CLSH_K_DATA, 0xff, 0x00},
		{WCD9XXX_A_CDC_CLSH_K_DATA, 0xff, 0x36},
		{WCD9XXX_A_CDC_CLSH_K_DATA, 0xff, 0x00},
		{WCD9XXX_A_CDC_CLSH_K_DATA, 0xff, 0x37},
		{WCD9XXX_A_CDC_CLSH_K_DATA, 0xff, 0x00},
	};

	for (i = 0; i < ARRAY_SIZE(reg_set); i++)
		snd_soc_update_bits(codec, reg_set[i].reg,
					reg_set[i].mask, reg_set[i].val);

	dev_dbg(codec->dev, "%s: Programmed Class H controller EAR specific params\n",
			 __func__);
}

static void wcd9xxx_cfg_clsh_param_hph(struct snd_soc_codec *codec)
{
	int i;
	const struct wcd9xxx_reg_mask_val reg_set[] = {
		{WCD9XXX_A_CDC_CLSH_B1_CTL, (0x1 << 6), 0},
		{WCD9XXX_A_CDC_CLSH_V_PA_HD_HPH, 0x3f, 0x0D},
		{WCD9XXX_A_CDC_CLSH_V_PA_MIN_HPH, 0x3f, 0x1D},

		/* Under assumption that HPH load is 16ohm per channel */
		{WCD9XXX_A_CDC_CLSH_IDLE_HPH_THSD, 0x3f, 0x13},
		{WCD9XXX_A_CDC_CLSH_FCLKONLY_HPH_THSD, 0x1f, 0x19},
		{WCD9XXX_A_CDC_CLSH_I_PA_FACT_HPH_L, 0xff, 0x97},
		{WCD9XXX_A_CDC_CLSH_I_PA_FACT_HPH_U, 0xff, 0x05},
		{WCD9XXX_A_CDC_CLSH_K_ADDR, (0x1 << 7), 0},
		{WCD9XXX_A_CDC_CLSH_K_ADDR, 0x0f, 0},
		{WCD9XXX_A_CDC_CLSH_K_DATA, 0xff, 0xAE},
		{WCD9XXX_A_CDC_CLSH_K_DATA, 0xff, 0x01},
		{WCD9XXX_A_CDC_CLSH_K_DATA, 0xff, 0x1C},
		{WCD9XXX_A_CDC_CLSH_K_DATA, 0xff, 0x00},
		{WCD9XXX_A_CDC_CLSH_K_DATA, 0xff, 0x24},
		{WCD9XXX_A_CDC_CLSH_K_DATA, 0xff, 0x00},
		{WCD9XXX_A_CDC_CLSH_K_DATA, 0xff, 0x25},
		{WCD9XXX_A_CDC_CLSH_K_DATA, 0xff, 0x00},
	};

	for (i = 0; i < ARRAY_SIZE(reg_set); i++)
		snd_soc_update_bits(codec, reg_set[i].reg, reg_set[i].mask,
							reg_set[i].val);
	dev_dbg(codec->dev, "%s: Programmed Class H controller HPH specific params\n",
			 __func__);
}

static void wcd9xxx_clsh_state_ear(struct snd_soc_codec *codec,
			struct wcd9xxx_clsh_cdc_data *clsh_d,
			u8 req_state, bool is_enable)
{
	if (is_enable) {
		wcd9xxx_cfg_clsh_param_common(codec);
		wcd9xxx_cfg_clsh_param_ear(codec);
		wcd9xxx_enable_clsh_block(codec, clsh_d, true);
		wcd9xxx_chargepump_request(codec, true);
		wcd9xxx_enable_anc_delay(codec, true);
		wcd9xxx_clsh_computation_request(codec,
				CLSH_COMPUTE_EAR, true);
		wcd9xxx_set_buck_mode(codec, BUCK_VREF_2V);
		wcd9xxx_enable_buck(codec, clsh_d, true);
		wcd9xxx_set_fclk_get_ncp(codec, clsh_d, NCP_FCLK_LEVEL_8);

		dev_dbg(codec->dev, "%s: Enabled ear mode class h\n", __func__);
	} else {
		dev_dbg(codec->dev, "%s: stub fallback to ear\n", __func__);
		wcd9xxx_set_fclk_put_ncp(codec, clsh_d, NCP_FCLK_LEVEL_8);
		wcd9xxx_enable_buck(codec, clsh_d, false);
        wcd9xxx_clsh_computation_request(codec,
				CLSH_COMPUTE_EAR, true);
		wcd9xxx_chargepump_request(codec, false);
		wcd9xxx_enable_clsh_block(codec, clsh_d, false);
	}
}

static void wcd9xxx_clsh_state_hph_l(struct snd_soc_codec *codec,
		struct wcd9xxx_clsh_cdc_data *clsh_d,
		u8 req_state, bool is_enable)
{
	if (is_enable) {
		wcd9xxx_cfg_clsh_param_common(codec);
		wcd9xxx_cfg_clsh_param_hph(codec);
		wcd9xxx_enable_clsh_block(codec, clsh_d, true);
		wcd9xxx_chargepump_request(codec, true);
		wcd9xxx_enable_anc_delay(codec, true);
		wcd9xxx_clsh_computation_request(codec,
				CLSH_COMPUTE_HPH_L, true);
		wcd9xxx_set_buck_mode(codec, BUCK_VREF_0P494V);
		wcd9xxx_enable_buck(codec, clsh_d, true);
		wcd9xxx_set_fclk_get_ncp(codec, clsh_d, NCP_FCLK_LEVEL_8);

		dev_dbg(codec->dev, "%s: Done\n", __func__);
	} else {
		wcd9xxx_set_fclk_put_ncp(codec, clsh_d, NCP_FCLK_LEVEL_8);
		wcd9xxx_enable_buck(codec, clsh_d, false);
		wcd9xxx_clsh_computation_request(codec,
        CLSH_COMPUTE_HPH_R, false);
		wcd9xxx_chargepump_request(codec, false);
		wcd9xxx_enable_clsh_block(codec, clsh_d, false);
	}
}

static void wcd9xxx_clsh_state_hph_r(struct snd_soc_codec *codec,
		struct wcd9xxx_clsh_cdc_data *clsh_d,
		u8 req_state, bool is_enable)
{
	if (is_enable) {
		wcd9xxx_cfg_clsh_param_common(codec);
		wcd9xxx_cfg_clsh_param_hph(codec);
		wcd9xxx_enable_clsh_block(codec, clsh_d, true);
		wcd9xxx_chargepump_request(codec, true);
		wcd9xxx_enable_anc_delay(codec, true);
		wcd9xxx_clsh_computation_request(codec,
				CLSH_COMPUTE_HPH_R, true);
		wcd9xxx_set_buck_mode(codec, BUCK_VREF_0P494V);
		wcd9xxx_enable_buck(codec, clsh_d, true);
		wcd9xxx_set_fclk_get_ncp(codec, clsh_d, NCP_FCLK_LEVEL_8);

		dev_dbg(codec->dev, "%s: Done\n", __func__);
	} else {
		wcd9xxx_set_fclk_put_ncp(codec, clsh_d, NCP_FCLK_LEVEL_8);
		wcd9xxx_enable_buck(codec, clsh_d, false);
		wcd9xxx_clsh_computation_request(codec,
				CLSH_COMPUTE_HPH_L, false);
		wcd9xxx_chargepump_request(codec, false);
		wcd9xxx_enable_clsh_block(codec, clsh_d, false);
	}
}

static void wcd9xxx_clsh_state_hph_st(struct snd_soc_codec *codec,
		struct wcd9xxx_clsh_cdc_data *clsh_d,
		u8 req_state, bool is_enable)
{
	pr_debug("%s: enter %s\n", __func__, is_enable ? "enable" : "disable");

	if (is_enable) {
		wcd9xxx_clsh_computation_request(codec,
				CLSH_COMPUTE_HPH_L, true);
		wcd9xxx_clsh_computation_request(codec,
				CLSH_COMPUTE_HPH_R, true);
	} else {
		dev_dbg(codec->dev, "%s: stub fallback to hph_st\n", __func__);
	}
}

static void wcd9xxx_clsh_state_lo(struct snd_soc_codec *codec,
		struct wcd9xxx_clsh_cdc_data *clsh_d,
		u8 req_state, bool is_enable)
{
	/*	TODO. Read from device tree */
	clsh_d->buck_mv = WCD9XXX_CDC_BUCK_MV_2P15;

	if (is_enable) {
		wcd9xxx_set_buck_mode(codec, BUCK_VREF_1P8V);
		wcd9xxx_enable_buck(codec, clsh_d, true);
		wcd9xxx_set_fclk_get_ncp(codec, clsh_d, NCP_FCLK_LEVEL_5);

		if (clsh_d->buck_mv == WCD9XXX_CDC_BUCK_MV_1P8) {
			wcd9xxx_enable_buck(codec, clsh_d, false);
			snd_soc_update_bits(codec, WCD9XXX_A_NCP_STATIC,
					    1 << 4, 1 << 4);
			msleep(NCP_SETTLE_TIME_US);
		} else {
            msleep(NCP_SETTLE_TIME_US);
			snd_soc_update_bits(codec, WCD9XXX_A_BUCK_MODE_5,
							0x01, 0x01);
			snd_soc_update_bits(codec, WCD9XXX_A_BUCK_MODE_5,
							0xFB, (0x02 << 2));
		}
		snd_soc_update_bits(codec, WCD9XXX_A_BUCK_MODE_1, 0x04, 0x00);
	} else {
		dev_dbg(codec->dev, "%s: stub fallback to lineout\n", __func__);
		wcd9xxx_set_fclk_put_ncp(codec, clsh_d, NCP_FCLK_LEVEL_5);
		if (clsh_d->buck_mv != WCD9XXX_CDC_BUCK_MV_1P8)
			wcd9xxx_enable_buck(codec, clsh_d, false);
	}
}

static void wcd9xxx_clsh_state_err(struct snd_soc_codec *codec,
		struct wcd9xxx_clsh_cdc_data *clsh_d,
		u8 req_state, bool is_enable)
{
	char msg[128];

	dev_dbg(codec->dev,
		"%s Wrong request for class H state machine requested to %s %s",
		__func__, is_enable ? "enable" : "disable",
		state_to_str(req_state, msg, sizeof(msg)));
	WARN_ON(1);
}

void wcd9xxx_clsh_fsm(struct snd_soc_codec *codec,
		struct wcd9xxx_clsh_cdc_data *cdc_clsh_d,
		u8 req_state, bool req_type, u8 clsh_event)
{
	u8 old_state, new_state;
    char msg0[128], msg1[128];

	switch (clsh_event) {
	case WCD9XXX_CLSH_EVENT_PRE_DAC:
		/* PRE_DAC event should be used only for Enable */
		BUG_ON(req_type != WCD9XXX_CLSH_REQ_ENABLE);

		old_state = cdc_clsh_d->state;
		new_state = old_state | req_state;

		(*clsh_state_fp[req_state]) (codec, cdc_clsh_d, req_state,
					     req_type);
		cdc_clsh_d->state = new_state;
		dev_info(codec->dev, "%s: ClassH state transition from %s to %s\n",
				__func__, state_to_str(old_state, msg0, sizeof(msg0)),
				state_to_str((cdc_clsh_d->state), msg1, sizeof(msg1)));

		break;

	case WCD9XXX_CLSH_EVENT_POST_PA:

		if (req_type == WCD9XXX_CLSH_REQ_DISABLE) {
			if (req_state == WCD9XXX_CLSH_STATE_LO
					&& --cdc_lo_count > 0)
					break;

			old_state = cdc_clsh_d->state;
			new_state = old_state & (~req_state);

			if (new_state < NUM_CLSH_STATES) {
				(*clsh_state_fp[req_state]) (codec, cdc_clsh_d,
							     req_state,
							     req_type);
				cdc_clsh_d->state = new_state;
				dev_info(codec->dev, "%s: ClassH state transition from %s to %s\n",
					__func__, state_to_str(old_state, msg0, sizeof(msg0)),
					state_to_str((cdc_clsh_d->state), msg1, sizeof(msg1)));

			} else {
				dev_err(codec->dev, "%s: wrong new state = %x\n",
						__func__, new_state);
			}
		} else if (!(cdc_clsh_d->state & WCD9XXX_CLSH_STATE_LO)) {
				wcd9xxx_clsh_enable_post_pa(codec);
		}

		break;
	}

}
EXPORT_SYMBOL_GPL(wcd9xxx_clsh_fsm);

void wcd9xxx_clsh_init(struct wcd9xxx_clsh_cdc_data *clsh)
{
	int i;
	clsh->state = WCD9XXX_CLSH_STATE_IDLE;

	for (i = 0; i < NUM_CLSH_STATES; i++)
		clsh_state_fp[i] = wcd9xxx_clsh_state_err;

	clsh_state_fp[WCD9XXX_CLSH_STATE_EAR] = wcd9xxx_clsh_state_ear;
	clsh_state_fp[WCD9XXX_CLSH_STATE_HPHL] =
						wcd9xxx_clsh_state_hph_l;
	clsh_state_fp[WCD9XXX_CLSH_STATE_HPHR] =
						wcd9xxx_clsh_state_hph_r;
	clsh_state_fp[WCD9XXX_CLSH_STATE_HPH_ST] =
						wcd9xxx_clsh_state_hph_st;
	clsh_state_fp[WCD9XXX_CLSH_STATE_LO] = wcd9xxx_clsh_state_lo;

}
EXPORT_SYMBOL_GPL(wcd9xxx_clsh_init);

MODULE_DESCRIPTION("WCD9XXX Common");
MODULE_LICENSE("GPL v2");
