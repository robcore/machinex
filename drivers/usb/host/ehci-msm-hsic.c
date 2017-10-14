/* ehci-msm-hsic.c - HSUSB Host Controller Driver Implementation
 *
 * Copyright (c) 2011-2014, The Linux Foundation. All rights reserved.
 *
 * Partly derived from ehci-fsl.c and ehci-hcd.c
 * Copyright (c) 2000-2004 by David Brownell
 * Copyright (c) 2005 MontaVista Software
 *
 * All source code in this file is licensed under the following license except
 * where indicated.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can find it at http://www.fsf.org
 */

#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/wakelock.h>
#include <linux/pm.h>
#include <linux/pm_runtime.h>
#include <linux/regulator/consumer.h>

#include <linux/usb/msm_hsusb_hw.h>
#include <linux/usb/msm_hsusb.h>
#include <linux/gpio.h>
#include <linux/spinlock.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/pm_qos.h>
#include <linux/irq.h>
#include <linux/ktime.h>

#include <mach/msm_bus.h>
#include <mach/clk.h>
#include <mach/msm_iomap.h>
#include <mach/msm_xo.h>
#include <linux/cpu.h>
#include <mach/rpm-regulator.h>

#define MSM_USB_BASE (hcd->regs)
#define USB_REG_START_OFFSET 0x90
#define USB_REG_END_OFFSET 0x250

#define RESUME_RETRY_LIMIT		3
#define RESUME_SIGNAL_TIME_USEC		(21 * 1000)
#define RESUME_SIGNAL_TIME_SOF_USEC	(23 * 1000)

static struct workqueue_struct  *ehci_wq;
struct ehci_timer {
#define GPT_LD(p)	((p) & 0x00FFFFFF)
	u32	gptimer0_ld;
#define GPT_RUN		BIT(31)
#define GPT_RESET	BIT(30)
#define GPT_MODE	BIT(24)
#define GPT_CNT(p)	((p) & 0x00FFFFFF)
	u32	gptimer0_ctrl;

	u32	gptimer1_ld;
	u32	gptimer1_ctrl;
};

struct msm_hsic_hcd {
	struct ehci_hcd		ehci;
	spinlock_t		wakeup_lock;
	struct device		*dev;
	struct clk		*ahb_clk;
	struct clk		*core_clk;
	struct clk		*alt_core_clk;
	struct clk		*phy_clk;
	struct clk		*cal_clk;
	struct regulator	*hsic_vddcx;
	atomic_t		async_int;
	atomic_t                in_lpm;
	struct wake_lock	wlock;
	int			peripheral_status_irq;
	int			wakeup_irq;
	bool			wakeup_irq_enabled;
	atomic_t		pm_usage_cnt;
	uint32_t		bus_perf_client;
	uint32_t		wakeup_int_cnt;
	enum usb_vdd_type	vdd_type;

	struct work_struct	bus_vote_w;
	bool			bus_vote;

	/* gp timer */
	struct ehci_timer __iomem *timer;
	struct completion	gpt0_completion;
	struct completion	rt_completion;
	int			resume_status;
	int			resume_again;
	int			bus_reset;
	int			reset_again;
	ktime_t			resume_start_t;

	struct pm_qos_request pm_qos_req_dma;
};

struct msm_hsic_hcd *__mehci;

enum event_type {
	EVENT_UNDEF = -1,
	URB_SUBMIT,
	URB_COMPLETE,
	EVENT_NONE,
};

#define EVENT_STR_LEN	5

static enum event_type str_to_event(const char *name)
{
	if (!strncasecmp("S", name, EVENT_STR_LEN))
		return URB_SUBMIT;
	if (!strncasecmp("C", name, EVENT_STR_LEN))
		return URB_COMPLETE;
	if (!strncasecmp("", name, EVENT_STR_LEN))
		return EVENT_NONE;

	return EVENT_UNDEF;
}

static inline struct msm_hsic_hcd *hcd_to_hsic(struct usb_hcd *hcd)
{
	return (struct msm_hsic_hcd *) (hcd->hcd_priv);
}

static inline struct usb_hcd *hsic_to_hcd(struct msm_hsic_hcd *mehci)
{
	return container_of((void *) mehci, struct usb_hcd, hcd_priv);
}

#define ULPI_IO_TIMEOUT_USEC	(10 * 1000)

#define USB_PHY_VDD_DIG_VOL_NONE	0 /*uV */
#define USB_PHY_VDD_DIG_VOL_MIN		945000 /* uV */
#define USB_PHY_VDD_DIG_VOL_MAX		1320000 /* uV */

#define HSIC_DBG1_REG		0x38

static const int vdd_val[VDD_TYPE_MAX][VDD_VAL_MAX] = {
		{   /* VDD_CX CORNER Voting */
			[VDD_NONE]	= RPM_VREG_CORNER_NONE,
			[VDD_MIN]	= RPM_VREG_CORNER_NOMINAL,
			[VDD_MAX]	= RPM_VREG_CORNER_HIGH,
		},
		{   /* VDD_CX Voltage Voting */
			[VDD_NONE]	= USB_PHY_VDD_DIG_VOL_NONE,
			[VDD_MIN]	= USB_PHY_VDD_DIG_VOL_MIN,
			[VDD_MAX]	= USB_PHY_VDD_DIG_VOL_MAX,
		},
};

static int msm_hsic_init_vddcx(struct msm_hsic_hcd *mehci, int init)
{
	int ret = 0;
	int none_vol, min_vol, max_vol;

	if (!mehci->hsic_vddcx) {
		mehci->vdd_type = VDDCX_CORNER;
		mehci->hsic_vddcx = devm_regulator_get(mehci->dev,
			"hsic_vdd_dig");
		if (IS_ERR(mehci->hsic_vddcx)) {
			mehci->hsic_vddcx = devm_regulator_get(mehci->dev,
				"HSIC_VDDCX");
			if (IS_ERR(mehci->hsic_vddcx)) {
				dev_err(mehci->dev, "unable to get hsic vddcx\n");
				return PTR_ERR(mehci->hsic_vddcx);
			}
			mehci->vdd_type = VDDCX;
		}
	}

	none_vol = vdd_val[mehci->vdd_type][VDD_NONE];
	min_vol = vdd_val[mehci->vdd_type][VDD_MIN];
	max_vol = vdd_val[mehci->vdd_type][VDD_MAX];

	if (!init)
		goto disable_reg;

	ret = regulator_set_voltage(mehci->hsic_vddcx, min_vol, max_vol);
	if (ret) {
		dev_err(mehci->dev, "unable to set the voltage"
				"for hsic vddcx\n");
		return ret;
	}

	ret = regulator_enable(mehci->hsic_vddcx);
	if (ret) {
		dev_err(mehci->dev, "unable to enable hsic vddcx\n");
		goto reg_enable_err;
	}

	return 0;

disable_reg:
	regulator_disable(mehci->hsic_vddcx);
reg_enable_err:
	regulator_set_voltage(mehci->hsic_vddcx, none_vol, max_vol);

	return ret;

}

static int __maybe_unused ulpi_read(struct msm_hsic_hcd *mehci, u32 reg)
{
	struct usb_hcd *hcd = hsic_to_hcd(mehci);
	int cnt = 0;

	/* initiate read operation */
	writel_relaxed(ULPI_RUN | ULPI_READ | ULPI_ADDR(reg),
	       USB_ULPI_VIEWPORT);

	/* wait for completion */
	while (cnt < ULPI_IO_TIMEOUT_USEC) {
		if (!(readl_relaxed(USB_ULPI_VIEWPORT) & ULPI_RUN))
			break;
		udelay(1);
		cnt++;
	}

	if (cnt >= ULPI_IO_TIMEOUT_USEC) {
		dev_err(mehci->dev, "ulpi_read: timeout ULPI_VIEWPORT: %08x\n",
				readl_relaxed(USB_ULPI_VIEWPORT));
		dev_err(mehci->dev, "PORTSC: %08x USBCMD: %08x FRINDEX: %08x\n",
				readl_relaxed(USB_PORTSC),
				readl_relaxed(USB_USBCMD),
				readl_relaxed(USB_FRINDEX));

		/*frame counter increments afte 125us*/
		udelay(130);
		dev_err(mehci->dev, "ulpi_read: FRINDEX: %08x\n",
				readl_relaxed(USB_FRINDEX));
		return -ETIMEDOUT;
	}

	return ULPI_DATA_READ(readl_relaxed(USB_ULPI_VIEWPORT));
}

static int ulpi_write(struct msm_hsic_hcd *mehci, u32 val, u32 reg)
{
	struct usb_hcd *hcd = hsic_to_hcd(mehci);
	int cnt = 0;

	/* initiate write operation */
	writel_relaxed(ULPI_RUN | ULPI_WRITE |
	       ULPI_ADDR(reg) | ULPI_DATA(val),
	       USB_ULPI_VIEWPORT);

	/* wait for completion */
	while (cnt < ULPI_IO_TIMEOUT_USEC) {
		if (!(readl_relaxed(USB_ULPI_VIEWPORT) & ULPI_RUN))
			break;
		udelay(1);
		cnt++;
	}

	if (cnt >= ULPI_IO_TIMEOUT_USEC) {
		dev_err(mehci->dev, "ulpi_write: timeout ULPI_VIEWPORT: %08x\n",
				readl_relaxed(USB_ULPI_VIEWPORT));
		dev_err(mehci->dev, "PORTSC: %08x USBCMD: %08x FRINDEX: %08x\n",
				readl_relaxed(USB_PORTSC),
				readl_relaxed(USB_USBCMD),
				readl_relaxed(USB_FRINDEX));

		/*frame counter increments afte 125us*/
		udelay(130);
		dev_err(mehci->dev, "ulpi_write: FRINDEX: %08x\n",
				readl_relaxed(USB_FRINDEX));
		return -ETIMEDOUT;
	}

	return 0;
}

static int msm_hsic_config_gpios(struct msm_hsic_hcd *mehci, int gpio_en)
{
	int rc = 0;
	struct msm_hsic_host_platform_data *pdata;
	static int gpio_status;

	pdata = mehci->dev->platform_data;

	if (!pdata || !pdata->strobe || !pdata->data)
		return rc;

	if (gpio_status == gpio_en)
		return 0;

	gpio_status = gpio_en;

	if (!gpio_en)
		goto free_gpio;

	rc = gpio_request(pdata->strobe, "HSIC_STROBE_GPIO");
	if (rc < 0) {
		dev_err(mehci->dev, "gpio request failed for HSIC STROBE\n");
		return rc;
	}

	rc = gpio_request(pdata->data, "HSIC_DATA_GPIO");
	if (rc < 0) {
		dev_err(mehci->dev, "gpio request failed for HSIC DATA\n");
		goto free_strobe;
	}

	return 0;

free_gpio:
	gpio_free(pdata->data);
free_strobe:
	gpio_free(pdata->strobe);

	return rc;
}

static void msm_hsic_clk_reset(struct msm_hsic_hcd *mehci)
{
	int ret;

	ret = clk_reset(mehci->core_clk, CLK_RESET_ASSERT);
	if (ret) {
		dev_err(mehci->dev, "hsic clk assert failed:%d\n", ret);
		return;
	}
	clk_disable(mehci->core_clk);

	ret = clk_reset(mehci->core_clk, CLK_RESET_DEASSERT);
	if (ret)
		dev_err(mehci->dev, "hsic clk deassert failed:%d\n", ret);

	usleep_range(10000, 12000);

	clk_enable(mehci->core_clk);
}

#define HSIC_STROBE_GPIO_PAD_CTL	(MSM_TLMM_BASE+0x20C0)
#define HSIC_DATA_GPIO_PAD_CTL		(MSM_TLMM_BASE+0x20C4)
#define HSIC_CAL_PAD_CTL       (MSM_TLMM_BASE+0x20C8)
#define HSIC_LV_MODE		0x04
#define HSIC_PAD_CALIBRATION	0xA8
#define HSIC_GPIO_PAD_VAL	0x0A0AAA10
#define LINK_RESET_TIMEOUT_USEC		(250 * 1000)
static int msm_hsic_reset(struct msm_hsic_hcd *mehci)
{
	struct usb_hcd *hcd = hsic_to_hcd(mehci);
	int ret;
	struct msm_hsic_host_platform_data *pdata = mehci->dev->platform_data;
	u32 temp;

	msm_hsic_clk_reset(mehci);

	/* select ulpi phy */
	writel_relaxed(0x80000000, USB_PORTSC);

	mb();

	/* HSIC init sequence when HSIC signals (Strobe/Data) are
	routed via GPIOs */
	if (pdata && pdata->strobe && pdata->data) {

		/* Enable LV_MODE in HSIC_CAL_PAD_CTL register */
		writel_relaxed(HSIC_LV_MODE, HSIC_CAL_PAD_CTL);

		mb();

		/*set periodic calibration interval to ~2.048sec in
		  HSIC_IO_CAL_REG */
		ulpi_write(mehci, 0xFF, 0x33);

		/* Enable periodic IO calibration in HSIC_CFG register */
		ulpi_write(mehci, HSIC_PAD_CALIBRATION, 0x30);

		/* Configure GPIO pins for HSIC functionality mode */
		ret = msm_hsic_config_gpios(mehci, 1);
		if (ret) {
			dev_err(mehci->dev, " gpio configuarion failed\n");
			return ret;
		}
		/* Set LV_MODE=0x1 and DCC=0x2 in HSIC_GPIO PAD_CTL register */
		writel_relaxed(HSIC_GPIO_PAD_VAL, HSIC_STROBE_GPIO_PAD_CTL);
		writel_relaxed(HSIC_GPIO_PAD_VAL, HSIC_DATA_GPIO_PAD_CTL);

		mb();

		/* Enable HSIC mode in HSIC_CFG register */
		ulpi_write(mehci, 0x01, 0x31);
	} else {
		/* HSIC init sequence when HSIC signals (Strobe/Data) are routed
		via dedicated I/O */

		/* programmable length of connect signaling (33.2ns) */
		ret = ulpi_write(mehci, 3, HSIC_DBG1_REG);
		if (ret) {
			pr_err("%s: Unable to program length of connect "
			      "signaling\n", __func__);
		}

		/*set periodic calibration interval to ~2.048sec in
		  HSIC_IO_CAL_REG */
		ulpi_write(mehci, 0xFF, 0x33);

		/* Enable HSIC mode in HSIC_CFG register */
		ulpi_write(mehci, 0xA9, 0x30);
	}

	temp = readl_relaxed(USB_GENCONFIG2);
	temp &= ~GENCFG2_SYS_CLK_HOST_DEV_GATE_EN;
	writel_relaxed(temp, USB_GENCONFIG2);

	/*disable auto resume*/
	ulpi_write(mehci, ULPI_IFC_CTRL_AUTORESUME, ULPI_CLR(ULPI_IFC_CTRL));

	return 0;
}

#define PHY_SUSPEND_TIMEOUT_USEC	(1000 * 1000)
#define PHY_RESUME_TIMEOUT_USEC		(100 * 1000)

#ifdef CONFIG_PM_SLEEP
static int msm_hsic_suspend(struct msm_hsic_hcd *mehci)
{
	struct usb_hcd *hcd = hsic_to_hcd(mehci);
	int cnt = 0, ret;
	u32 val;
	int none_vol, max_vol;
	unsigned long flags;
	struct msm_hsic_host_platform_data *pdata = mehci->dev->platform_data;

	if (atomic_read(&mehci->in_lpm)) {
		return 0;
	}

	disable_irq(hcd->irq);

	/* make sure we don't race against a remote wakeup */
	if (test_bit(HCD_FLAG_WAKEUP_PENDING, &hcd->flags) ||
	    readl_relaxed(USB_PORTSC) & PORT_RESUME) {
		enable_irq(hcd->irq);
		return -EBUSY;
	}

	/*
	 * PHY may take some time or even fail to enter into low power
	 * mode (LPM). Hence poll for 500 msec and reset the PHY and link
	 * in failure case.
	 */
	val = readl_relaxed(USB_PORTSC);
	val &= ~PORT_RWC_BITS;
	val |= PORTSC_PHCD;
	writel_relaxed(val, USB_PORTSC);
	while (cnt < PHY_SUSPEND_TIMEOUT_USEC) {
		if (readl_relaxed(USB_PORTSC) & PORTSC_PHCD)
			break;
		msleep_interruptible(500);
		cnt++;
	}

	if (cnt >= PHY_SUSPEND_TIMEOUT_USEC) {
		dev_err(mehci->dev, "Unable to suspend PHY\n");
		msm_hsic_config_gpios(mehci, 0);
		msm_hsic_reset(mehci);
	}

	/*
	 * PHY has capability to generate interrupt asynchronously in low
	 * power mode (LPM). This interrupt is level triggered. So USB IRQ
	 * line must be disabled till async interrupt enable bit is cleared
	 * in USBCMD register. Assert STP (ULPI interface STOP signal) to
	 * block data communication from PHY.  Enable asynchronous interrupt
	 * only when wakeup gpio IRQ is not present.
	 */
	if (mehci->wakeup_irq)
		writel_relaxed(readl_relaxed(USB_USBCMD) |
				ULPI_STP_CTRL, USB_USBCMD);
	else
		writel_relaxed(readl_relaxed(USB_USBCMD) | ASYNC_INTR_CTRL |
				ULPI_STP_CTRL, USB_USBCMD);

	/*
	 * Ensure that hardware is put in low power mode before
	 * clocks are turned OFF and VDD is allowed to minimize.
	 */
	mb();
	clk_disable_unprepare(mehci->core_clk);
	clk_disable_unprepare(mehci->phy_clk);
	clk_disable_unprepare(mehci->cal_clk);
	clk_disable_unprepare(mehci->ahb_clk);

	none_vol = vdd_val[mehci->vdd_type][VDD_NONE];
	max_vol = vdd_val[mehci->vdd_type][VDD_MAX];

	ret = regulator_set_voltage(mehci->hsic_vddcx, none_vol, max_vol);
	if (ret < 0)
		dev_err(mehci->dev, "unable to set vddcx voltage for VDD MIN\n");

	atomic_set(&mehci->in_lpm, 1);
	enable_irq(hcd->irq);

	spin_lock_irqsave(&mehci->wakeup_lock, flags);
	enable_irq_wake(mehci->wakeup_irq);
	enable_irq(mehci->wakeup_irq);
	mehci->wakeup_irq_enabled = true;
	spin_unlock_irqrestore(&mehci->wakeup_lock, flags);

	wake_unlock(&mehci->wlock);

	return 0;
}

static int msm_hsic_resume(struct msm_hsic_hcd *mehci)
{
	struct usb_hcd *hcd = hsic_to_hcd(mehci);
	int cnt = 0, ret;
	unsigned temp;
	int min_vol, max_vol;
	unsigned long flags;
	struct msm_hsic_host_platform_data *pdata = mehci->dev->platform_data;

	if (!atomic_read(&mehci->in_lpm)) {
		return 0;
	}

	/* Handles race with Async interrupt */
	disable_irq(hcd->irq);

	spin_lock_irqsave(&mehci->wakeup_lock, flags);
	if (mehci->wakeup_irq_enabled) {
		disable_irq_wake(mehci->wakeup_irq);
		disable_irq_nosync(mehci->wakeup_irq);
		mehci->wakeup_irq_enabled = false;
	}
	spin_unlock_irqrestore(&mehci->wakeup_lock, flags);

	wake_lock(&mehci->wlock);

	min_vol = vdd_val[mehci->vdd_type][VDD_MIN];
	max_vol = vdd_val[mehci->vdd_type][VDD_MAX];

	ret = regulator_set_voltage(mehci->hsic_vddcx, min_vol, max_vol);
	if (ret < 0)
		dev_err(mehci->dev, "unable to set nominal vddcx voltage (no VDD MIN)\n");

	clk_prepare_enable(mehci->core_clk);
	clk_prepare_enable(mehci->phy_clk);
	clk_prepare_enable(mehci->cal_clk);
	clk_prepare_enable(mehci->ahb_clk);

	temp = readl_relaxed(USB_USBCMD);
	temp &= ~ASYNC_INTR_CTRL;
	temp &= ~ULPI_STP_CTRL;
	writel_relaxed(temp, USB_USBCMD);

	if (!(readl_relaxed(USB_PORTSC) & PORTSC_PHCD))
		goto skip_phy_resume;

	temp = readl_relaxed(USB_PORTSC);
	temp &= ~(PORT_RWC_BITS | PORTSC_PHCD);
	writel_relaxed(temp, USB_PORTSC);
	while (cnt < PHY_RESUME_TIMEOUT_USEC) {
		if (!(readl_relaxed(USB_PORTSC) & PORTSC_PHCD) &&
			(readl_relaxed(USB_ULPI_VIEWPORT) & ULPI_SYNC_STATE))
			break;
		udelay(1);
		cnt++;
	}

	if (cnt >= PHY_RESUME_TIMEOUT_USEC) {
		/*
		 * This is a fatal error. Reset the link and
		 * PHY to make hsic working.
		 */
		dev_err(mehci->dev, "Unable to resume USB. Reset the hsic\n");
		msm_hsic_config_gpios(mehci, 0);
		msm_hsic_reset(mehci);
	}

skip_phy_resume:

	usb_hcd_resume_root_hub(hcd);

	atomic_set(&mehci->in_lpm, 0);

	if (atomic_read(&mehci->async_int)) {
		atomic_set(&mehci->async_int, 0);
		pm_runtime_put_noidle(mehci->dev);
		enable_irq(hcd->irq);
	}

	if (atomic_read(&mehci->pm_usage_cnt)) {
		atomic_set(&mehci->pm_usage_cnt, 0);
		pm_runtime_put_noidle(mehci->dev);
	}

	enable_irq(hcd->irq);

	return 0;
}
#endif

static void ehci_hsic_bus_vote_w(struct work_struct *w)
{
	struct msm_hsic_hcd *mehci =
			container_of(w, struct msm_hsic_hcd, bus_vote_w);
	int ret;

	ret = msm_bus_scale_client_update_request(mehci->bus_perf_client,
			mehci->bus_vote);
	if (ret)
		dev_err(mehci->dev, "%s: Failed to vote for bus bandwidth %d\n",
				__func__, ret);
}

static int msm_hsic_reset_done(struct usb_hcd *hcd)
{
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);
	u32 __iomem *status_reg = &ehci->regs->port_status[0];
	int ret;

	ehci_writel(ehci, ehci_readl(ehci, status_reg) & ~(PORT_RWC_BITS |
					PORT_RESET), status_reg);

	ret = handshake(ehci, status_reg, PORT_RESET, 0, 1 * 1000);

	if (ret)
		pr_err("reset handshake failed in %s\n", __func__);
	else
		ehci_writel(ehci, ehci_readl(ehci, &ehci->regs->command) |
				CMD_RUN, &ehci->regs->command);

	return ret;
}

#define STS_GPTIMER0_INTERRUPT	BIT(24)
static irqreturn_t msm_hsic_irq(struct usb_hcd *hcd)
{
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);
	struct msm_hsic_hcd *mehci = hcd_to_hsic(hcd);
	u32			status;
	int			ret;

	if (atomic_read(&mehci->in_lpm)) {
		ret = pm_runtime_get(mehci->dev);
		if ((ret == 1) || (ret == -EINPROGRESS)) {
			pm_runtime_put_noidle(mehci->dev);
		} else {
			disable_irq_nosync(hcd->irq);
			atomic_set(&mehci->async_int, 1);
		}

		return IRQ_HANDLED;
	}

	status = ehci_readl(ehci, &ehci->regs->status);

	if (status & STS_GPTIMER0_INTERRUPT) {
		int timeleft;

		timeleft = GPT_CNT(ehci_readl(ehci,
						 &mehci->timer->gptimer1_ctrl));
		if (!mehci->bus_reset) {
			if (ktime_us_delta(ktime_get(), mehci->resume_start_t) >
					RESUME_SIGNAL_TIME_SOF_USEC) {
				pr_err("HSIC GPT timer prog invalid\n");
				timeleft = 0;
			}
		}
		if (timeleft) {
			if (mehci->bus_reset) {
				ret = msm_hsic_reset_done(hcd);
				if (ret)
					mehci->reset_again = 1;
			} else {
				writel_relaxed(readl_relaxed(
					&ehci->regs->command) | CMD_RUN,
					&ehci->regs->command);
				if (ktime_us_delta(ktime_get(),
					mehci->resume_start_t) >
					RESUME_SIGNAL_TIME_SOF_USEC) {
					pr_err("HSIC resume fail. retrying\n");
					mehci->resume_again = 1;
				}
			}
		} else {
			if (mehci->bus_reset)
				mehci->reset_again = 1;
			else
				mehci->resume_again = 1;
		}
		complete(&mehci->gpt0_completion);
		ehci_writel(ehci, STS_GPTIMER0_INTERRUPT, &ehci->regs->status);
	}

	return ehci_irq(hcd);
}

static int ehci_hsic_reset(struct usb_hcd *hcd)
{
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);
	struct msm_hsic_hcd *mehci = hcd_to_hsic(hcd);
	int retval;

	mehci->timer = USB_HS_GPTIMER_BASE;
	ehci->caps = USB_CAPLENGTH;
	hcd->has_tt = 1;

	retval = ehci_setup(hcd);
	if (retval)
		return retval;

	/* bursts of unspecified length. */
	writel_relaxed(0, USB_AHBBURST);
	/* Use the AHB transactor */
	writel_relaxed(0x08, USB_AHBMODE);
	/* Disable streaming mode and select host mode */
	writel_relaxed(0x13, USB_USBMODE);

	ehci_port_power(ehci, 1);
	return 0;
}

#ifdef CONFIG_PM

#define RESET_RETRY_LIMIT 3
#define RESET_SIGNAL_TIME_SOF_USEC (50 * 1000)
#define RESET_SIGNAL_TIME_USEC (20 * 1000)
static void ehci_hsic_reset_sof_bug_handler(struct usb_hcd *hcd, u32 val)
{
	struct ehci_hcd	*ehci = hcd_to_ehci(hcd);
	struct msm_hsic_hcd *mehci = hcd_to_hsic(hcd);
	struct msm_hsic_host_platform_data *pdata = mehci->dev->platform_data;
	u32 __iomem *status_reg = &ehci->regs->port_status[0];
	u32 cmd;
	unsigned long flags;
	int retries = 0, ret, cnt = RESET_SIGNAL_TIME_USEC;
	s32 next_latency = 0;

	if (pdata && pdata->swfi_latency) {
		next_latency = pdata->swfi_latency + 1;
		pm_qos_update_request(&mehci->pm_qos_req_dma, next_latency);
		next_latency = PM_QOS_DEFAULT_VALUE;
	}

	mehci->bus_reset = 1;

	/* Halt the controller */
	cmd = ehci_readl(ehci, &ehci->regs->command);
	cmd &= ~CMD_RUN;
	ehci_writel(ehci, cmd, &ehci->regs->command);
	ret = handshake(ehci, &ehci->regs->status, STS_HALT,
			STS_HALT, 16 * 125);
	if (ret) {
		pr_err("halt handshake fatal error\n");
		goto fail;
	}

retry:
	retries++;
	mehci->reset_again = 0;
	spin_lock_irqsave(&ehci->lock, flags);
	ehci_writel(ehci, val, status_reg);
	ehci_writel(ehci, GPT_LD(RESET_SIGNAL_TIME_USEC - 1),
					&mehci->timer->gptimer0_ld);
	ehci_writel(ehci, GPT_RESET | GPT_RUN,
			&mehci->timer->gptimer0_ctrl);
	ehci_writel(ehci, INTR_MASK | STS_GPTIMER0_INTERRUPT,
			&ehci->regs->intr_enable);

	ehci_writel(ehci, GPT_LD(RESET_SIGNAL_TIME_SOF_USEC - 1),
			&mehci->timer->gptimer1_ld);
	ehci_writel(ehci, GPT_RESET | GPT_RUN,
		&mehci->timer->gptimer1_ctrl);

	spin_unlock_irqrestore(&ehci->lock, flags);
	wait_for_completion(&mehci->gpt0_completion);

	if (!mehci->reset_again)
		goto done;

	if (handshake(ehci, status_reg, PORT_RESET, 0, 10 * 1000))
		goto fail;

	if (retries < RESET_RETRY_LIMIT)
		goto retry;

	spin_lock_irqsave(&ehci->lock, flags);
	ehci_writel(ehci, val, status_reg);
	while (cnt--)
		udelay(1);
	ret = msm_hsic_reset_done(hcd);
	spin_unlock_irqrestore(&ehci->lock, flags);
	if (ret)
		goto fail;

done:
fail:
	mehci->bus_reset = 0;
	if (next_latency)
		pm_qos_update_request(&mehci->pm_qos_req_dma, next_latency);
}

static int ehci_hsic_bus_suspend(struct usb_hcd *hcd)
{
	struct msm_hsic_hcd *mehci = hcd_to_hsic(hcd);

	if (!(readl_relaxed(USB_PORTSC) & PORT_PE))
		return -EAGAIN;

	return ehci_bus_suspend(hcd);
}

static int msm_hsic_resume_thread(void *data)
{
	struct msm_hsic_hcd *mehci = data;
	struct usb_hcd *hcd = hsic_to_hcd(mehci);
	struct ehci_hcd		*ehci = hcd_to_ehci(hcd);
	u32			temp;
	unsigned long		resume_needed = 0;
	int			retry_cnt = 0;
	int			tight_resume = 0;
	int			tight_count = 0;
	struct msm_hsic_host_platform_data *pdata = mehci->dev->platform_data;
	s32 next_latency = 0;

	if (pdata && pdata->swfi_latency) {
		next_latency = pdata->swfi_latency + 1;
		pm_qos_update_request(&mehci->pm_qos_req_dma, next_latency);
		next_latency = PM_QOS_DEFAULT_VALUE;
	}

	/* keep delay between bus states */
	if (time_before_eq(jiffies, ehci->next_statechange))
		mdelay(10);

	spin_lock_irq(&ehci->lock);
	if (!HCD_HW_ACCESSIBLE(hcd)) {
		mehci->resume_status = -ESHUTDOWN;
		goto exit;
	}

	/* at least some APM implementations will try to deliver
	 * IRQs right away, so delay them until we're ready.
	 */
	writel_relaxed(0, &ehci->regs->intr_enable);

	/* re-init operational registers */
	writel_relaxed(0, &ehci->regs->segment);
	writel_relaxed(ehci->periodic_dma, &ehci->regs->frame_list);
	writel_relaxed((u32) ehci->async->qh_dma, &ehci->regs->async_next);

	/*CMD_RUN will be set after, PORT_RESUME gets cleared*/
	if (ehci->resume_sof_bug)
		ehci->command &= ~CMD_RUN;

	/* restore CMD_RUN, framelist size, and irq threshold */
	writel_relaxed(ehci->command, &ehci->regs->command);

	/* manually resume the ports we suspended during bus_suspend() */
resume_again:
	if (retry_cnt >= RESUME_RETRY_LIMIT) {
		pr_info("retry count(%d) reached max, resume in tight loop\n",
					retry_cnt);
		tight_resume = 1;
	}

	temp = readl_relaxed(&ehci->regs->port_status[0]);
	temp &= ~(PORT_RWC_BITS | PORT_WAKE_BITS);
	if (test_bit(0, &ehci->bus_suspended) && (temp & PORT_SUSPEND)) {
		temp |= PORT_RESUME;
		set_bit(0, &resume_needed);
	}
	mehci->resume_start_t = ktime_get();
	writel_relaxed(temp, &ehci->regs->port_status[0]);

	/* HSIC controller has a h/w bug due to which it can try to send SOFs
	 * (start of frames) during port resume resulting in phy lockup. HSIC hw
	 * controller in MSM clears FPR bit after driving the resume signal for
	 * 20ms. Workaround is to stop SOFs before driving resume and then start
	 * sending SOFs immediately. Need to send SOFs within 3ms of resume
	 * completion otherwise peripheral may enter undefined state. As
	 * usleep_range does not gurantee exact sleep time, GPTimer is used to
	 * to time the resume sequence. If driver exceeds allowable time SOFs,
	 * repeat the resume process.
	 */
	if (ehci->resume_sof_bug && resume_needed) {
		if (!tight_resume) {
			mehci->resume_again = 0;
			writel_relaxed(GPT_LD(RESUME_SIGNAL_TIME_USEC - 1),
					&mehci->timer->gptimer0_ld);
			writel_relaxed(GPT_RESET | GPT_RUN,
					&mehci->timer->gptimer0_ctrl);
			writel_relaxed(INTR_MASK | STS_GPTIMER0_INTERRUPT,
					&ehci->regs->intr_enable);

			writel_relaxed(GPT_LD(RESUME_SIGNAL_TIME_SOF_USEC - 1),
					&mehci->timer->gptimer1_ld);
			writel_relaxed(GPT_RESET | GPT_RUN,
				&mehci->timer->gptimer1_ctrl);
			spin_unlock_irq(&ehci->lock);
			wait_for_completion_interruptible(&mehci->gpt0_completion);
			spin_lock_irq(&ehci->lock);
		} else {
			/* do the resume in a tight loop */
			mdelay(20);
			writel_relaxed(readl_relaxed(&ehci->regs->command) |
					CMD_RUN, &ehci->regs->command);
			if (ktime_us_delta(ktime_get(), mehci->resume_start_t) >
					RESUME_SIGNAL_TIME_SOF_USEC) {
				if (++tight_count > 3) {
					pr_err("HSIC resume failed\n");
					mehci->resume_status = -ENODEV;
					goto exit;
				}
				mehci->resume_again = 1;
			}
		}

		if (mehci->resume_again) {
			int temp;
			spin_unlock_irq(&ehci->lock);
			temp = readl_relaxed(&ehci->regs->command);
			if (temp & CMD_RUN) {
				temp &= ~CMD_RUN;
				writel_relaxed(temp, &ehci->regs->command);
			}
			temp = readl_relaxed(&ehci->regs->port_status[0]);
			temp &= ~PORT_RWC_BITS;
			temp |= PORT_SUSPEND;
			writel_relaxed(temp, &ehci->regs->port_status[0]);
			/* Keep the bus idle for 5ms so that peripheral
			 * can detect and initiate suspend
			 */
			mdelay(5);
			spin_lock_irq(&ehci->lock);
			mehci->resume_again = 0;
			retry_cnt++;
			goto resume_again;
		}
	}

	ehci->command |= CMD_RUN;
	mehci->resume_status = 1;
exit:
	spin_unlock_irq(&ehci->lock);
	complete(&mehci->rt_completion);
	if (next_latency)
		pm_qos_update_request(&mehci->pm_qos_req_dma, next_latency);

	return 0;
}

static int ehci_hsic_bus_resume(struct usb_hcd *hcd)
{
	struct msm_hsic_hcd *mehci = hcd_to_hsic(hcd);
	struct ehci_hcd		*ehci = hcd_to_ehci(hcd);
	u32			temp;
	struct task_struct	*resume_thread = NULL;

	mehci->resume_status = 0;
	resume_thread = kthread_run(msm_hsic_resume_thread,
			mehci, "hsic_resume_thread");
	if (IS_ERR(resume_thread)) {
		pr_err("Error creating resume thread:%lu\n",
				PTR_ERR(resume_thread));
		return PTR_ERR(resume_thread);
	}

	wait_for_completion(&mehci->rt_completion);

	if (mehci->resume_status < 0)
		return mehci->resume_status;

	spin_lock_irq(&ehci->lock);
	(void) ehci_readl(ehci, &ehci->regs->command);

	temp = 0;
	if (ehci->async->qh_next.qh)
		temp |= CMD_ASE;
	if (ehci->periodic_sched)
		temp |= CMD_PSE;
	if (temp) {
		ehci->command |= temp;
		ehci_writel(ehci, ehci->command, &ehci->regs->command);
	}

	ehci->next_statechange = jiffies + msecs_to_jiffies(5);
	hcd->state = HC_STATE_RUNNING;
	ehci->rh_state = EHCI_RH_RUNNING;

	/* Now we can safely re-enable irqs */
	ehci_writel(ehci, INTR_MASK, &ehci->regs->intr_enable);

	spin_unlock_irq(&ehci->lock);

	return 0;
}

#else

#define ehci_hsic_bus_suspend	NULL
#define ehci_hsic_bus_resume	NULL

#endif	/* CONFIG_PM */
#if 0
static void ehci_msm_set_autosuspend_delay(struct usb_device *dev)
{
	if (!dev->parent) /*for root hub no delay*/
		pm_runtime_set_autosuspend_delay(&dev->dev, 0);
	else
		pm_runtime_set_autosuspend_delay(&dev->dev, 200);
}
#endif

static struct hc_driver msm_hsic_driver = {
	.description		= hcd_name,
	.product_desc		= "Qualcomm EHCI Host Controller using HSIC",
	.hcd_priv_size		= sizeof(struct msm_hsic_hcd),

	/*
	 * generic hardware linkage
	 */
	.irq			= msm_hsic_irq,
	.flags			= HCD_USB2 | HCD_MEMORY | HCD_OLD_ENUM,

	.reset			= ehci_hsic_reset,
	.start			= ehci_run,

	.stop			= ehci_stop,
	.shutdown		= ehci_shutdown,

	/*
	 * managing i/o requests and associated device resources
	 */
	.urb_enqueue		= ehci_urb_enqueue,
	.urb_dequeue		= ehci_urb_dequeue,
	.endpoint_disable	= ehci_endpoint_disable,
	.endpoint_reset		= ehci_endpoint_reset,
	.clear_tt_buffer_complete	 = ehci_clear_tt_buffer_complete,

	/*
	 * scheduling support
	 */
	.get_frame_number	= ehci_get_frame,

	/*
	 * root hub support
	 */
	.hub_status_data	= ehci_hub_status_data,
	.hub_control		= ehci_hub_control,
	.relinquish_port	= ehci_relinquish_port,
	.port_handed_over	= ehci_port_handed_over,

	/*
	 * PM support
	 */
	.bus_suspend		= ehci_hsic_bus_suspend,
	.bus_resume		= ehci_hsic_bus_resume,

	//.set_autosuspend_delay = ehci_msm_set_autosuspend_delay,
	.reset_sof_bug_handler	= ehci_hsic_reset_sof_bug_handler,
};

static int msm_hsic_init_clocks(struct msm_hsic_hcd *mehci, u32 init)
{
	int ret = 0;

	if (!init)
		goto put_clocks;

	/*core_clk is required for LINK protocol engine
	 *clock rate appropriately set by target specific clock driver */
	mehci->core_clk = clk_get(mehci->dev, "core_clk");
	if (IS_ERR(mehci->core_clk)) {
		dev_err(mehci->dev, "failed to get core_clk\n");
		ret = PTR_ERR(mehci->core_clk);
		return ret;
	}

	/* alt_core_clk is for LINK to be used during PHY RESET
	 * clock rate appropriately set by target specific clock driver */
	mehci->alt_core_clk = clk_get(mehci->dev, "alt_core_clk");
	if (IS_ERR(mehci->alt_core_clk)) {
		dev_err(mehci->dev, "failed to core_clk\n");
		ret = PTR_ERR(mehci->alt_core_clk);
		goto put_core_clk;
	}

	/* phy_clk is required for HSIC PHY operation
	 * clock rate appropriately set by target specific clock driver */
	mehci->phy_clk = clk_get(mehci->dev, "phy_clk");
	if (IS_ERR(mehci->phy_clk)) {
		dev_err(mehci->dev, "failed to get phy_clk\n");
		ret = PTR_ERR(mehci->phy_clk);
		goto put_alt_core_clk;
	}

	/* 10MHz cal_clk is required for calibration of I/O pads */
	mehci->cal_clk = clk_get(mehci->dev, "cal_clk");
	if (IS_ERR(mehci->cal_clk)) {
		dev_err(mehci->dev, "failed to get cal_clk\n");
		ret = PTR_ERR(mehci->cal_clk);
		goto put_phy_clk;
	}
	clk_set_rate(mehci->cal_clk, 10000000);

	/* ahb_clk is required for data transfers */
	mehci->ahb_clk = clk_get(mehci->dev, "iface_clk");
	if (IS_ERR(mehci->ahb_clk)) {
		dev_err(mehci->dev, "failed to get iface_clk\n");
		ret = PTR_ERR(mehci->ahb_clk);
		goto put_cal_clk;
	}

	clk_prepare_enable(mehci->core_clk);
	clk_prepare_enable(mehci->phy_clk);
	clk_prepare_enable(mehci->cal_clk);
	clk_prepare_enable(mehci->ahb_clk);

	return 0;

put_clocks:
	if (!atomic_read(&mehci->in_lpm)) {
		clk_disable_unprepare(mehci->core_clk);
		clk_disable_unprepare(mehci->phy_clk);
		clk_disable_unprepare(mehci->cal_clk);
		clk_disable_unprepare(mehci->ahb_clk);
	}
	clk_put(mehci->ahb_clk);
put_cal_clk:
	clk_put(mehci->cal_clk);
put_phy_clk:
	clk_put(mehci->phy_clk);
put_alt_core_clk:
	clk_put(mehci->alt_core_clk);
put_core_clk:
	clk_put(mehci->core_clk);

	return ret;
}
static irqreturn_t hsic_peripheral_status_change(int irq, void *dev_id)
{
	struct msm_hsic_hcd *mehci = dev_id;

	pr_debug("%s: mehci:%p dev_id:%p\n", __func__, mehci, dev_id);

	if (mehci)
		msm_hsic_config_gpios(mehci, 0);

	return IRQ_HANDLED;
}

static irqreturn_t msm_hsic_wakeup_irq(int irq, void *data)
{
	struct msm_hsic_hcd *mehci = data;
	int ret;

	wake_lock(&mehci->wlock);
	mehci->wakeup_int_cnt++;

	spin_lock(&mehci->wakeup_lock);
	if (mehci->wakeup_irq_enabled) {
		disable_irq_wake(irq);
		disable_irq_nosync(irq);
		mehci->wakeup_irq_enabled = false;
	}
	spin_unlock(&mehci->wakeup_lock);

	if (!atomic_read(&mehci->pm_usage_cnt)) {
		ret = pm_runtime_get(mehci->dev);
		/*
		 * HSIC runtime resume can race with us.
		 * if we are active (ret == 1) or resuming
		 * (ret == -EINPROGRESS), decrement the
		 * PM usage counter before returning.
		 */
		if ((ret == 1) || (ret == -EINPROGRESS))
			pm_runtime_put_noidle(mehci->dev);
		else
			atomic_set(&mehci->pm_usage_cnt, 1);
	}

	return IRQ_HANDLED;
}

static int ehci_hsic_msm_probe(struct platform_device *pdev)
{
	struct usb_hcd *hcd;
	struct resource *res;
	struct msm_hsic_hcd *mehci;
	struct msm_hsic_host_platform_data *pdata;
	int ret;

	/* After parent device's probe is executed, it will be put in suspend
	 * mode. When child device's probe is called, driver core is not
	 * resuming parent device due to which parent will be in suspend even
	 * though child is active. Hence resume the parent device explicitly.
	 */
	if (pdev->dev.parent)
		pm_runtime_get_sync(pdev->dev.parent);

	hcd = usb_create_hcd(&msm_hsic_driver, &pdev->dev,
				dev_name(&pdev->dev));
	if (!hcd) {
		dev_err(&pdev->dev, "Unable to create HCD\n");
		ret = -ENOMEM;
		goto put_parent;
	}

	hcd_to_bus(hcd)->skip_resume = true;

	hcd->irq = platform_get_irq(pdev, 0);
	if (hcd->irq < 0) {
		dev_err(&pdev->dev, "Unable to get IRQ resource\n");
		ret = hcd->irq;
		goto put_hcd;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "Unable to get memory resource\n");
		ret = -ENODEV;
		goto put_hcd;
	}

	hcd->rsrc_start = res->start;
	hcd->rsrc_len = resource_size(res);
	hcd->regs = ioremap(hcd->rsrc_start, hcd->rsrc_len);
	if (!hcd->regs) {
		dev_err(&pdev->dev, "ioremap failed\n");
		ret = -ENOMEM;
		goto put_hcd;
	}

	mehci = hcd_to_hsic(hcd);
	mehci->dev = &pdev->dev;
	pdata = mehci->dev->platform_data;

	spin_lock_init(&mehci->wakeup_lock);

	mehci->ehci.susp_sof_bug = 1;
	mehci->ehci.reset_sof_bug = 1;

	mehci->ehci.resume_sof_bug = 1;

	if (pdata)
		mehci->ehci.log2_irq_thresh = pdata->log2_irq_thresh;

	res = platform_get_resource_byname(pdev,
			IORESOURCE_IRQ,
			"peripheral_status_irq");
	if (res)
		mehci->peripheral_status_irq = res->start;

	res = platform_get_resource_byname(pdev, IORESOURCE_IRQ, "wakeup");
	if (res) {
		mehci->wakeup_irq = res->start;
	}

	ret = msm_hsic_init_clocks(mehci, 1);
	if (ret) {
		dev_err(&pdev->dev, "unable to initialize clocks\n");
		ret = -ENODEV;
		goto unmap;
	}

	ret = msm_hsic_init_vddcx(mehci, 1);
	if (ret) {
		dev_err(&pdev->dev, "unable to initialize VDDCX\n");
		ret = -ENODEV;
		goto deinit_clocks;
	}

	init_completion(&mehci->rt_completion);
	init_completion(&mehci->gpt0_completion);
	ret = msm_hsic_reset(mehci);
	if (ret) {
		dev_err(&pdev->dev, "unable to initialize PHY\n");
		goto deinit_vddcx;
	}

	ehci_wq = alloc_workqueue("ehci_wq", WQ_UNBOUND | WQ_MEM_RECLAIM, 1);
	if (!ehci_wq) {
		dev_err(&pdev->dev, "unable to create workqueue\n");
		ret = -ENOMEM;
		goto deinit_vddcx;
	}

	INIT_WORK(&mehci->bus_vote_w, ehci_hsic_bus_vote_w);

	ret = usb_add_hcd(hcd, hcd->irq, IRQF_SHARED);
	if (ret) {
		dev_err(&pdev->dev, "unable to register HCD\n");
		goto unconfig_gpio;
	}

	device_init_wakeup(&pdev->dev, 1);
	wake_lock_init(&mehci->wlock, WAKE_LOCK_SUSPEND, dev_name(&pdev->dev));
	wake_lock(&mehci->wlock);

	if (mehci->peripheral_status_irq) {
		ret = request_threaded_irq(mehci->peripheral_status_irq,
			NULL, hsic_peripheral_status_change,
			IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING
						| IRQF_SHARED | IRQF_ONESHOT,
			"hsic_peripheral_status", mehci);
		if (ret)
			dev_err(&pdev->dev, "%s:request_irq:%d failed:%d",
				__func__, mehci->peripheral_status_irq, ret);
	}

	/* configure wakeup irq */
	if (mehci->wakeup_irq) {
		/* In case if wakeup gpio is pulled high at this point
		 * remote wakeup interrupt fires right after request_irq.
		 * Remote wake up interrupt only needs to be enabled when
		 * HSIC bus goes to suspend.
		 */
		irq_set_status_flags(mehci->wakeup_irq, IRQ_NOAUTOEN);
		ret = request_irq(mehci->wakeup_irq, msm_hsic_wakeup_irq,
				IRQF_TRIGGER_HIGH,
				"msm_hsic_wakeup", mehci);
		if (ret) {
			dev_err(&pdev->dev, "request_irq(%d) failed: %d\n",
					mehci->wakeup_irq, ret);
			mehci->wakeup_irq = 0;
		}
	}

	if (pdata && pdata->bus_scale_table) {
		mehci->bus_perf_client =
		    msm_bus_scale_register_client(pdata->bus_scale_table);
		/* Configure BUS performance parameters for MAX bandwidth */
		if (mehci->bus_perf_client) {
			mehci->bus_vote = true;
			queue_work(ehci_wq, &mehci->bus_vote_w);
		} else {
			dev_err(&pdev->dev, "%s: Failed to register BUS "
						"scaling client!!\n", __func__);
		}
	}

	__mehci = mehci;

	if (pdata && pdata->swfi_latency)
		pm_qos_add_request(&mehci->pm_qos_req_dma,
			PM_QOS_CPU_DMA_LATENCY, PM_QOS_DEFAULT_VALUE);

	/*
	 * This pdev->dev is assigned parent of root-hub by USB core,
	 * hence, runtime framework automatically calls this driver's
	 * runtime APIs based on root-hub's state.
	 */
	pm_runtime_set_active(&pdev->dev);
	pm_runtime_enable(&pdev->dev);
	/* Decrement the parent device's counter after probe.
	 * As child is active, parent will not be put into
	 * suspend mode.
	 */
	if (pdev->dev.parent)
		pm_runtime_put_sync(pdev->dev.parent);

	return 0;

unconfig_gpio:
	destroy_workqueue(ehci_wq);
	msm_hsic_config_gpios(mehci, 0);
deinit_vddcx:
	msm_hsic_init_vddcx(mehci, 0);
deinit_clocks:
	msm_hsic_init_clocks(mehci, 0);
unmap:
	iounmap(hcd->regs);
put_hcd:
	usb_put_hcd(hcd);
put_parent:
	if (pdev->dev.parent)
		pm_runtime_put_sync(pdev->dev.parent);
	return ret;
}

static int ehci_hsic_msm_remove(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);
	struct msm_hsic_hcd *mehci = hcd_to_hsic(hcd);
	struct msm_hsic_host_platform_data *pdata = mehci->dev->platform_data;

	/* Remove the HCD prior to releasing our resources. */
	usb_remove_hcd(hcd);

	if (pdata && pdata->swfi_latency)
		pm_qos_remove_request(&mehci->pm_qos_req_dma);

	if (mehci->peripheral_status_irq)
		free_irq(mehci->peripheral_status_irq, mehci);

	if (mehci->wakeup_irq) {
		if (mehci->wakeup_irq_enabled) {
			disable_irq_wake(mehci->wakeup_irq);
			mehci->wakeup_irq_enabled = false;
		}
		free_irq(mehci->wakeup_irq, mehci);
	}

	/*
	 * If the update request is called after unregister, the request will
	 * fail. Results are undefined if unregister is called in the middle of
	 * update request.
	 */
	mehci->bus_vote = false;
	cancel_work_sync(&mehci->bus_vote_w);

	if (mehci->bus_perf_client)
		msm_bus_scale_unregister_client(mehci->bus_perf_client);

	device_init_wakeup(&pdev->dev, 0);
	wake_lock_destroy(&mehci->wlock);
	pm_runtime_set_suspended(&pdev->dev);

	destroy_workqueue(ehci_wq);

	msm_hsic_config_gpios(mehci, 0);
	msm_hsic_init_vddcx(mehci, 0);

	msm_hsic_init_clocks(mehci, 0);
	iounmap(hcd->regs);
	usb_put_hcd(hcd);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static bool hcd_irq_awake;
static int msm_hsic_pm_suspend(struct device *dev)
{
	struct usb_hcd *hcd = dev_get_drvdata(dev);
	struct msm_hsic_hcd *mehci = hcd_to_hsic(hcd);

	if (!atomic_read(&mehci->in_lpm)) {
		dev_info(dev, "abort suspend\n");
		return -EBUSY;
	}

	if (device_may_wakeup(dev) && !hcd_irq_awake) {
		enable_irq_wake(hcd->irq);
		hcd_irq_awake = true;
	}

	return 0;
}

static int msm_hsic_pm_suspend_noirq(struct device *dev)
{
	struct usb_hcd *hcd = dev_get_drvdata(dev);
	struct msm_hsic_hcd *mehci = hcd_to_hsic(hcd);

	if (atomic_read(&mehci->async_int)) {
		return -EBUSY;
	}

	return 0;
}

static int msm_hsic_pm_resume(struct device *dev)
{
	int ret;
	struct usb_hcd *hcd = dev_get_drvdata(dev);
	struct msm_hsic_hcd *mehci = hcd_to_hsic(hcd);

	if (hcd_irq_awake && device_may_wakeup(dev)) {
		disable_irq_wake(hcd->irq);
		hcd_irq_awake = false;
	}

	/*
	 * Keep HSIC in Low Power Mode if system is resumed
	 * by any other wakeup source.  HSIC is resumed later
	 * when remote wakeup is received or interface driver
	 * start I/O.
	 */
	if (!atomic_read(&mehci->pm_usage_cnt) &&
			!atomic_read(&mehci->async_int) &&
			pm_runtime_suspended(dev))
		return 0;

	ret = msm_hsic_resume(mehci);
	if (ret)
		return ret;

	/* Bring the device to full powered state upon system resume */
	pm_runtime_disable(dev);
	pm_runtime_set_active(dev);
	pm_runtime_enable(dev);

	return 0;
}
#endif

#ifdef CONFIG_PM
static int msm_hsic_runtime_idle(struct device *dev)
{
	return 0;
}

static int msm_hsic_runtime_suspend(struct device *dev)
{
	struct usb_hcd *hcd = dev_get_drvdata(dev);
	struct msm_hsic_hcd *mehci = hcd_to_hsic(hcd);

	return msm_hsic_suspend(mehci);
}

static int msm_hsic_runtime_resume(struct device *dev)
{
	struct usb_hcd *hcd = dev_get_drvdata(dev);
	struct msm_hsic_hcd *mehci = hcd_to_hsic(hcd);

	return msm_hsic_resume(mehci);
}
#endif

#ifdef CONFIG_PM
static const struct dev_pm_ops msm_hsic_dev_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(msm_hsic_pm_suspend, msm_hsic_pm_resume)
	.suspend_noirq = msm_hsic_pm_suspend_noirq, \
	.suspend = msm_hsic_pm_suspend, \
	.resume = msm_hsic_pm_resume,
	SET_RUNTIME_PM_OPS(msm_hsic_runtime_suspend, msm_hsic_runtime_resume,
				msm_hsic_runtime_idle)
	.runtime_suspend = msm_hsic_runtime_suspend, \
	.runtime_resume = msm_hsic_runtime_resume, \
	.runtime_idle = msm_hsic_runtime_idle,
};
#endif

static struct platform_driver ehci_msm_hsic_driver = {
	.probe	= ehci_hsic_msm_probe,
	.remove	= ehci_hsic_msm_remove,
	.driver = {
		.name = "msm_hsic_host",
#ifdef CONFIG_PM
		.pm = &msm_hsic_dev_pm_ops,
#endif
	},
};
