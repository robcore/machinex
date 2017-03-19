/*
 * Copyright (C) 2013 Red Hat
 * Author: Rob Clark <robdclark@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/gpio.h>

#include "msm_connector.h"
#include "hdmi.h"

struct hdmi_connector {
	struct msm_connector base;
	struct hdmi hdmi;
	unsigned long int pixclock;
	bool enabled;
};
#define to_hdmi_connector(x) container_of(x, struct hdmi_connector, base)

static int gpio_config(struct hdmi *hdmi, bool on)
{
	struct drm_device *dev = hdmi->dev;
	struct hdmi_platform_config *config =
			hdmi->pdev->dev.platform_data;
	int ret;

	if (on) {
		ret = gpio_request(config->ddc_clk_gpio, "HDMI_DDC_CLK");
		if (ret) {
			dev_err(dev->dev, "'%s'(%d) gpio_request failed: %d\n",
				"HDMI_DDC_CLK", config->ddc_clk_gpio, ret);
			goto error1;
		}
		ret = gpio_request(config->ddc_data_gpio, "HDMI_DDC_DATA");
		if (ret) {
			dev_err(dev->dev, "'%s'(%d) gpio_request failed: %d\n",
				"HDMI_DDC_DATA", config->ddc_data_gpio, ret);
			goto error2;
		}
		ret = gpio_request(config->hpd_gpio, "HDMI_HPD");
		if (ret) {
			dev_err(dev->dev, "'%s'(%d) gpio_request failed: %d\n",
				"HDMI_HPD", config->hpd_gpio, ret);
			goto error3;
		}
		if (config->pmic_gpio != -1) {
			ret = gpio_request(config->pmic_gpio, "PMIC_HDMI_MUX_SEL");
			if (ret) {
				dev_err(dev->dev, "'%s'(%d) gpio_request failed: %d\n",
					"PMIC_HDMI_MUX_SEL", config->pmic_gpio, ret);
				goto error4;
			}
			gpio_set_value_cansleep(config->pmic_gpio, 0);
		}
		DBG("gpio on");
	} else {
		gpio_free(config->ddc_clk_gpio);
		gpio_free(config->ddc_data_gpio);
		gpio_free(config->hpd_gpio);

		if (config->pmic_gpio != -1) {
			gpio_set_value_cansleep(config->pmic_gpio, 1);
			gpio_free(config->pmic_gpio);
		}
		DBG("gpio off");
	}

	return 0;

error4:
	gpio_free(config->hpd_gpio);
error3:
	gpio_free(config->ddc_data_gpio);
error2:
	gpio_free(config->ddc_clk_gpio);
error1:
	return ret;
}

static int hpd_enable(struct hdmi_connector *hdmi_connector)
{
	struct hdmi *hdmi = &hdmi_connector->hdmi;
	struct drm_device *dev = hdmi_connector->base.base.dev;
	struct hdmi_phy *phy = hdmi->phy;
	uint32_t hpd_ctrl;
	int ret;

	ret = gpio_config(hdmi, true);
	if (ret) {
		dev_err(dev->dev, "failed to configure GPIOs: %d\n", ret);
		goto fail;
	}

	ret = clk_prepare_enable(hdmi->clk);
	if (ret) {
		dev_err(dev->dev, "failed to enable 'clk': %d\n", ret);
		goto fail;
	}

	ret = clk_prepare_enable(hdmi->m_pclk);
	if (ret) {
		dev_err(dev->dev, "failed to enable 'm_pclk': %d\n", ret);
		goto fail;
	}

	ret = clk_prepare_enable(hdmi->s_pclk);
	if (ret) {
		dev_err(dev->dev, "failed to enable 's_pclk': %d\n", ret);
		goto fail;
	}

	if (hdmi->mpp0)
		ret = regulator_enable(hdmi->mpp0);
	if (!ret)
		ret = regulator_enable(hdmi->mvs);
	if (ret) {
		dev_err(dev->dev, "failed to enable regulators: %d\n", ret);
		goto fail;
	}

	hdmi_set_mode(hdmi, false);
	phy->funcs->reset(phy);
	hdmi_set_mode(hdmi, true);

	hdmi_write(hdmi, REG_HDMI_USEC_REFTIMER, 0x0001001b);

	/* enable HPD events: */
	hdmi_write(hdmi, REG_HDMI_HPD_INT_CTRL,
			HDMI_HPD_INT_CTRL_INT_CONNECT |
			HDMI_HPD_INT_CTRL_INT_EN);

	/* set timeout to 4.1ms (max) for hardware debounce */
	hpd_ctrl = hdmi_read(hdmi, REG_HDMI_HPD_CTRL);
	hpd_ctrl |= HDMI_HPD_CTRL_TIMEOUT(0x1fff);

	/* Toggle HPD circuit to trigger HPD sense */
	hdmi_write(hdmi, REG_HDMI_HPD_CTRL,
			~HDMI_HPD_CTRL_ENABLE & hpd_ctrl);
	hdmi_write(hdmi, REG_HDMI_HPD_CTRL,
			HDMI_HPD_CTRL_ENABLE | hpd_ctrl);

	return 0;

fail:
	return ret;
}

static int hdp_disable(struct hdmi_connector *hdmi_connector)
{
	struct hdmi *hdmi = &hdmi_connector->hdmi;
	struct drm_device *dev = hdmi_connector->base.base.dev;
	int ret = 0;

	/* Disable HPD interrupt */
	hdmi_write(hdmi, REG_HDMI_HPD_INT_CTRL, 0);

	hdmi_set_mode(hdmi, false);

	if (hdmi->mpp0)
		ret = regulator_disable(hdmi->mpp0);
	if (!ret)
		ret = regulator_disable(hdmi->mvs);
	if (ret) {
		dev_err(dev->dev, "failed to enable regulators: %d\n", ret);
		goto fail;
	}

	clk_disable_unprepare(hdmi->clk);
	clk_disable_unprepare(hdmi->m_pclk);
	clk_disable_unprepare(hdmi->s_pclk);

	ret = gpio_config(hdmi, false);
	if (ret) {
		dev_err(dev->dev, "failed to unconfigure GPIOs: %d\n", ret);
		goto fail;
	}

	return 0;

fail:
	return ret;
}

void hdmi_connector_irq(struct drm_connector *connector)
{
	struct msm_connector *msm_connector = to_msm_connector(connector);
	struct hdmi_connector *hdmi_connector = to_hdmi_connector(msm_connector);
	struct hdmi *hdmi = &hdmi_connector->hdmi;
	uint32_t hpd_int_status, hpd_int_ctrl;

	/* Process HPD: */
	hpd_int_status = hdmi_read(hdmi, REG_HDMI_HPD_INT_STATUS);
	hpd_int_ctrl   = hdmi_read(hdmi, REG_HDMI_HPD_INT_CTRL);

	if ((hpd_int_ctrl & HDMI_HPD_INT_CTRL_INT_EN) &&
			(hpd_int_status & HDMI_HPD_INT_STATUS_INT)) {
		bool detected = !!(hpd_int_status & HDMI_HPD_INT_STATUS_CABLE_DETECTED);

		DBG("status=%04x, ctrl=%04x", hpd_int_status, hpd_int_ctrl);

		/* ack the irq: */
		hdmi_write(hdmi, REG_HDMI_HPD_INT_CTRL,
				hpd_int_ctrl | HDMI_HPD_INT_CTRL_INT_ACK);

		drm_helper_hpd_irq_event(connector->dev);

		/* detect disconnect if we are connected or visa versa: */
		hpd_int_ctrl = HDMI_HPD_INT_CTRL_INT_EN;
		if (!detected)
			hpd_int_ctrl |= HDMI_HPD_INT_CTRL_INT_CONNECT;
		hdmi_write(hdmi, REG_HDMI_HPD_INT_CTRL, hpd_int_ctrl);
	}
}

static enum drm_connector_status hdmi_connector_detect(
		struct drm_connector *connector, bool force)
{
	struct msm_connector *msm_connector = to_msm_connector(connector);
	struct hdmi_connector *hdmi_connector = to_hdmi_connector(msm_connector);
	struct hdmi *hdmi = &hdmi_connector->hdmi;
	uint32_t hpd_int_status;
	int retry = 20;

	hpd_int_status = hdmi_read(hdmi, REG_HDMI_HPD_INT_STATUS);

	/* sense seems to in some cases be momentarily de-asserted, don't
	 * let that trick us into thinking the monitor is gone:
	 */
	while (retry-- && !(hpd_int_status & HDMI_HPD_INT_STATUS_CABLE_DETECTED)) {
		mdelay(10);
		hpd_int_status = hdmi_read(hdmi, REG_HDMI_HPD_INT_STATUS);
		DBG("status=%08x", hpd_int_status);
	}

	return (hpd_int_status & HDMI_HPD_INT_STATUS_CABLE_DETECTED) ?
			connector_status_connected : connector_status_disconnected;
}

static void hdmi_connector_destroy(struct drm_connector *connector)
{
	struct msm_connector *msm_connector = to_msm_connector(connector);
	struct hdmi_connector *hdmi_connector = to_hdmi_connector(msm_connector);

	hdp_disable(hdmi_connector);

	drm_sysfs_connector_remove(connector);
	drm_connector_cleanup(connector);

	hdmi_destroy(&hdmi_connector->hdmi);

	kfree(hdmi_connector);
}

static int hdmi_connector_get_modes(struct drm_connector *connector)
{
	struct msm_connector *msm_connector = to_msm_connector(connector);
	struct hdmi_connector *hdmi_connector = to_hdmi_connector(msm_connector);
	struct hdmi *hdmi = &hdmi_connector->hdmi;
	struct edid *edid;
	uint32_t hdmi_ctrl;
	int ret = 0;

	hdmi_ctrl = hdmi_read(hdmi, REG_HDMI_CTRL);
	hdmi_write(hdmi, REG_HDMI_CTRL, hdmi_ctrl | HDMI_CTRL_ENABLE);

	edid = drm_get_edid(connector, hdmi->i2c);

	hdmi_write(hdmi, REG_HDMI_CTRL, hdmi_ctrl);

	drm_mode_connector_update_edid_property(connector, edid);

	if (edid) {
		ret = drm_add_edid_modes(connector, edid);
		kfree(edid);
	}

	return ret;
}

static int hdmi_connector_mode_valid(struct drm_connector *connector,
				 struct drm_display_mode *mode)
{
	struct msm_connector *msm_connector = to_msm_connector(connector);
	struct msm_drm_private *priv = connector->dev->dev_private;
	struct msm_kms *kms = priv->kms;
	long actual, requested;

	requested = 1000 * mode->clock;
	actual = kms->funcs->round_pixclk(kms,
			requested, msm_connector->encoder);

	DBG("requested=%ld, actual=%ld", requested, actual);

	if (actual != requested)
		return MODE_CLOCK_RANGE;

	return 0;
}

static const struct drm_connector_funcs hdmi_connector_funcs = {
	.dpms = drm_helper_connector_dpms,
	.detect = hdmi_connector_detect,
	.fill_modes = drm_helper_probe_single_connector_modes,
	.destroy = hdmi_connector_destroy,
};

static const struct drm_connector_helper_funcs hdmi_connector_helper_funcs = {
	.get_modes = hdmi_connector_get_modes,
	.mode_valid = hdmi_connector_mode_valid,
	.best_encoder = msm_connector_attached_encoder,
};

static void hdmi_connector_dpms(struct msm_connector *msm_connector, int mode)
{
	struct hdmi_connector *hdmi_connector = to_hdmi_connector(msm_connector);
	struct hdmi *hdmi = &hdmi_connector->hdmi;
	struct hdmi_phy *phy = hdmi->phy;
	bool enabled = (mode == DRM_MODE_DPMS_ON);

	DBG("mode=%d", mode);

	if (enabled == hdmi_connector->enabled)
		return;

	if (enabled) {
		phy->funcs->powerup(phy, hdmi_connector->pixclock);
		hdmi_set_mode(hdmi, true);
	} else {
		hdmi_set_mode(hdmi, false);
		phy->funcs->powerdown(phy);
	}

	hdmi_connector->enabled = enabled;
}

static void hdmi_connector_mode_set(struct msm_connector *msm_connector,
		struct drm_display_mode *mode)
{
	struct hdmi_connector *hdmi_connector = to_hdmi_connector(msm_connector);
	struct hdmi *hdmi = &hdmi_connector->hdmi;
	int hstart, hend, vstart, vend;
	uint32_t frame_ctrl;

	hdmi_connector->pixclock = mode->clock * 1000;

	hdmi->hdmi_mode = drm_match_cea_mode(mode) > 1;

	hstart = mode->htotal - mode->hsync_start;
	hend   = mode->htotal - mode->hsync_start + mode->hdisplay;

	vstart = mode->vtotal - mode->vsync_start - 1;
	vend   = mode->vtotal - mode->vsync_start + mode->vdisplay - 1;

	DBG("htotal=%d, vtotal=%d, hstart=%d, hend=%d, vstart=%d, vend=%d",
			mode->htotal, mode->vtotal, hstart, hend, vstart, vend);

	hdmi_write(hdmi, REG_HDMI_TOTAL,
			HDMI_TOTAL_H_TOTAL(mode->htotal - 1) |
			HDMI_TOTAL_V_TOTAL(mode->vtotal - 1));

	hdmi_write(hdmi, REG_HDMI_ACTIVE_HSYNC,
			HDMI_ACTIVE_HSYNC_START(hstart) |
			HDMI_ACTIVE_HSYNC_END(hend));
	hdmi_write(hdmi, REG_HDMI_ACTIVE_VSYNC,
			HDMI_ACTIVE_VSYNC_START(vstart) |
			HDMI_ACTIVE_VSYNC_END(vend));

	if (mode->flags & DRM_MODE_FLAG_INTERLACE) {
		hdmi_write(hdmi, REG_HDMI_VSYNC_TOTAL_F2,
				HDMI_VSYNC_TOTAL_F2_V_TOTAL(mode->vtotal));
		hdmi_write(hdmi, REG_HDMI_VSYNC_ACTIVE_F2,
				HDMI_VSYNC_ACTIVE_F2_START(vstart + 1) |
				HDMI_VSYNC_ACTIVE_F2_END(vend + 1));
	} else {
		hdmi_write(hdmi, REG_HDMI_VSYNC_TOTAL_F2,
				HDMI_VSYNC_TOTAL_F2_V_TOTAL(0));
		hdmi_write(hdmi, REG_HDMI_VSYNC_ACTIVE_F2,
				HDMI_VSYNC_ACTIVE_F2_START(0) |
				HDMI_VSYNC_ACTIVE_F2_END(0));
	}

	frame_ctrl = 0;
	if (mode->flags & DRM_MODE_FLAG_NHSYNC)
		frame_ctrl |= HDMI_FRAME_CTRL_HSYNC_LOW;
	if (mode->flags & DRM_MODE_FLAG_NVSYNC)
		frame_ctrl |= HDMI_FRAME_CTRL_VSYNC_LOW;
	if (mode->flags & DRM_MODE_FLAG_INTERLACE)
		frame_ctrl |= HDMI_FRAME_CTRL_INTERLACED_EN;
	DBG("frame_ctrl=%08x", frame_ctrl);
	hdmi_write(hdmi, REG_HDMI_FRAME_CTRL, frame_ctrl);

	// TODO until we have audio, this might be safest:
	if (hdmi->hdmi_mode)
		hdmi_write(hdmi, REG_HDMI_GC, HDMI_GC_MUTE);
}

static const struct msm_connector_funcs msm_connector_funcs = {
		.dpms = hdmi_connector_dpms,
		.mode_set = hdmi_connector_mode_set,
};

/* initialize connector */
struct drm_connector *hdmi_connector_init(struct drm_device *dev,
		struct drm_encoder *encoder)
{
	struct drm_connector *connector = NULL;
	struct hdmi_connector *hdmi_connector;
	int ret;

	hdmi_connector = kzalloc(sizeof(*hdmi_connector), GFP_KERNEL);
	if (!hdmi_connector) {
		ret = -ENOMEM;
		goto fail;
	}

	connector = &hdmi_connector->base.base;

	msm_connector_init(&hdmi_connector->base,
			&msm_connector_funcs, encoder);
	drm_connector_init(dev, connector, &hdmi_connector_funcs,
			DRM_MODE_CONNECTOR_HDMIA);
	drm_connector_helper_add(connector, &hdmi_connector_helper_funcs);

	connector->polled = DRM_CONNECTOR_POLL_HPD;

	connector->interlace_allowed = 1;
	connector->doublescan_allowed = 0;

	drm_sysfs_connector_add(connector);

	ret = hdmi_init(&hdmi_connector->hdmi, dev, connector);
	if (ret)
		goto fail;

	ret = hpd_enable(hdmi_connector);
	if (ret) {
		dev_err(dev->dev, "failed to enable HPD: %d\n", ret);
		goto fail;
	}

	drm_mode_connector_attach_encoder(connector, encoder);

	return connector;

fail:
	if (connector)
		hdmi_connector_destroy(connector);

	return ERR_PTR(ret);
}
