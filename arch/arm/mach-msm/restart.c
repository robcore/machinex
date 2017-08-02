/* Copyright (c) 2010-2012, The Linux Foundation. All rights reserved.
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
#include <linux/init.h>
#include <linux/reboot.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/cpu.h>
#include <linux/interrupt.h>
#include <linux/mfd/pmic8058.h>
#include <linux/mfd/pmic8901.h>
#include <linux/mfd/pm8xxx/misc.h>

#include <asm/mach-types.h>
#include <asm/cacheflush.h>
#include <asm/system_misc.h>

#include <mach/msm_iomap.h>
#include <mach/restart.h>
#include <mach/socinfo.h>
#ifdef CONFIG_SEC_DEBUG
#include <mach/sec_debug.h>
#include <linux/notifier.h>
#include <linux/ftrace.h>
#endif
#include <mach/irqs.h>
#include <mach/scm.h>
#include "msm_watchdog.h"
#include "timer.h"

#ifdef CONFIG_KEXEC_HARDBOOT
#include <asm/kexec.h>
#endif

#define WDT0_RST	0x38
#define WDT0_EN		0x40
#define WDT0_BARK_TIME	0x4C
#define WDT0_BITE_TIME	0x5C

#define PSHOLD_CTL_SU (MSM_TLMM_BASE + 0x820)

#define RESTART_REASON_ADDR 0x65C
#define DLOAD_MODE_ADDR     0x0

#define SCM_IO_DISABLE_PMIC_ARBITER	1

static int restart_mode;
void *restart_reason;
static bool scm_pmic_arbiter_disable_supported;
int pmic_reset_irq;
static void __iomem *msm_tmr0_base;

#ifdef CONFIG_MSM_DLOAD_MODE
static int in_panic;
static void *dload_mode_addr;
struct work_struct resout_helper_work;
struct workqueue_struct *restart_wq;
static void resout_helper(struct work_struct *work);

/* Download mode master kill-switch */
static int dload_set(const char *val, struct kernel_param *kp);
static int download_mode = 1;
module_param_call(download_mode, dload_set, param_get_int,
			&download_mode, 0644);



static int panic_prep_restart(struct notifier_block *this,
			      unsigned long event, void *ptr)
{
	in_panic = 1;
	return NOTIFY_DONE;
}

static struct notifier_block panic_blk = {
	.notifier_call	= panic_prep_restart,
};

static void set_dload_mode(int on)
{
	if (dload_mode_addr) {
		__raw_writel(on ? 0xE47B337D : 0, dload_mode_addr);
		__raw_writel(on ? 0xCE14091A : 0,
		       dload_mode_addr + sizeof(unsigned int));
		mb();
	}
}

static int dload_set(const char *val, struct kernel_param *kp)
{
	int ret;
	int old_val = download_mode;

	ret = param_set_int(val, kp);

	if (ret)
		return ret;

	/* If download_mode is not zero or one, ignore. */
	if (download_mode >> 1) {
		download_mode = old_val;
		return -EINVAL;
	}

	set_dload_mode(download_mode);
#ifdef CONFIG_LGE_CRASH_HANDLER
	ssr_magic_number = 0;
#endif

	return 0;
}
#else
#define set_dload_mode(x) do {} while (0)
#endif

void msm_set_restart_mode(int mode)
{
	restart_mode = mode;
#ifdef CONFIG_LGE_CRASH_HANDLER
	if (download_mode == 1 && (mode & 0xFFFF0000) == 0x6D630000)
		panic("LGE crash handler detected panic");
#endif
}
EXPORT_SYMBOL(msm_set_restart_mode);

/*
 * Force the SPMI PMIC arbiter to shutdown so that no more SPMI transactions
 * are sent from the MSM to the PMIC.  This is required in order to avoid an
 * SPMI lockup on certain PMIC chips if PS_HOLD is lowered in the middle of
 * an SPMI transaction.
 */
static void halt_spmi_pmic_arbiter(void)
{
	if (scm_pmic_arbiter_disable_supported) {
		pr_crit("Calling SCM to disable SPMI PMIC arbiter\n");
		preempt_disable();
		__iowmb();
		scm_call_atomic1(SCM_SVC_PWR, SCM_IO_DISABLE_PMIC_ARBITER, 0);
	}
}

static void machinex_pm8xxx_restart_cmds(int enable)
{
	int ret = 0;
	bool failed;

	failed = false;

	ret = pm8xxx_reset_pwr_off(enable);
	if (ret) {
		failed = true;
		pm8xxx_watchdog_reset_control(1);
	}

	if (failed)
		ret = pm8xxx_reset_pwr_off(enable);
		if (ret)
			panic("PM8XXX reset FAILED!!%d\n", ret);
}

static void __msm_power_off(int lower_pshold)
{
	pr_emerg("Powering off the SoC\n");
#ifdef CONFIG_MSM_DLOAD_MODE
	set_dload_mode(0);
#endif
	machinex_pm8xxx_restart_cmds(0);

	if (lower_pshold) {
		halt_spmi_pmic_arbiter();
		__raw_writel(0, PSHOLD_CTL_SU);
		mdelay(10000);
		pr_emerg("Powering off has failed\n");
	}
}

static void msm_power_off(void)
{
	/* MSM initiated power off, lower ps_hold */
	__msm_power_off(1);
}

static void cpu_power_off(void *data)
{
	s32 rc;
	pr_err("PMIC Initiated shutdown %s cpu=%d\n", __func__,
						smp_processor_id());
	if (smp_processor_id() == 0) {
		/*
		 * PMIC initiated power off, do not lower ps_hold, pmic will
		 * shut msm down
		 */
		__msm_power_off(0);

		pet_watchdog();
		pr_emerg("Calling scm to disable arbiter\n");
		/* call secure manager to disable arbiter and never return */
		preempt_disable();
		__iowmb();
		rc = scm_call_atomic1(SCM_SVC_PWR,
						SCM_IO_DISABLE_PMIC_ARBITER, 1);
		panic("SCM returned even when asked to busy loop rc=%d\n", rc);
		pr_emerg("waiting on pmic to shut msm down\n");
	}

	while (1)
		;
}

static int irq_enabled;
static int status;

int resout_irq_control(int enable)
{
	if (!irq_enabled)
		return -EINVAL;

	if (enable ^ status) {
		if (enable) {
			enable_irq(pmic_reset_irq);
			status = 1;
			pr_info("%s : %d\n", __func__, status);
		} else {
			disable_irq_nosync(pmic_reset_irq);
			status = 0;
			pr_info("%s : %d\n", __func__, status);
		}
	} else
		return -1;

	return 0;
}

static void resout_helper(struct work_struct *work)
{
	smp_call_function(cpu_power_off, NULL, 0);
}

static irqreturn_t resout_irq_handler(int irq, void *dev_id)
{

	pr_emerg("%s PMIC Initiated shutdown\n", __func__);
	oops_in_progress = 1;
	queue_work(restart_wq, &resout_helper_work);
	if (smp_processor_id() == 0)
		cpu_power_off(NULL);
	while (1)
		;

	return IRQ_HANDLED;
}

static void msm_restart_prepare(const char *cmd)
{
	unsigned long value;

	set_dload_mode(0);
	set_dload_mode(in_panic);
	if (restart_mode == RESTART_DLOAD)
		set_dload_mode(1);
	if (restart_mode == RESTART_NORMAL)
		set_dload_mode(0);

	/* Kill download mode if master-kill switch is set */
	if (!download_mode)
		set_dload_mode(0);
	else
		set_dload_mode(0);
	pr_info("Going down for restart now\n");

	machinex_pm8xxx_restart_cmds(1);

/*	if (!restart_reason)
		restart_reason = ioremap_nocache((unsigned long)(MSM_IMEM_BASE \
						+ RESTART_REASON_ADDR), SZ_4K);
*/
	if (cmd != NULL) {
		if (!strncmp(cmd, "bootloader", 10)) {
			__raw_writel(0x77665500, restart_reason);
		} else if (!strncmp(cmd, "recovery", 8)) {
			__raw_writel(0x77665502, restart_reason);
		} else if (!strncmp(cmd, "oem-", 4)) {
			unsigned long code;
			code = simple_strtoul(cmd + 4, NULL, 16) & 0xff;
			__raw_writel(0x6f656d00 | code, restart_reason);
#ifdef CONFIG_SEC_DEBUG
		} else if (!strncmp(cmd, "sec_debug_hw_reset", 18)) {
			__raw_writel(0x776655ee, restart_reason);
#endif
		} else if (!strncmp(cmd, "download", 8)) {
			__raw_writel(0x12345671, restart_reason);
		} else if (!strncmp(cmd, "sud", 3)) {
			__raw_writel(0xabcf0000 | (cmd[3] - '0'),
					restart_reason);
		} else if (!strncmp(cmd, "debug", 5)
				&& !kstrtoul(cmd + 5, 0, &value)) {
			__raw_writel(0xabcd0000 | value, restart_reason);
#ifdef CONFIG_SEC_SSR_DEBUG_LEVEL_CHK
		} else if (!strncmp(cmd, "cpdebug", 7) /* set cp debug level */
				&& !kstrtoul(cmd + 7, 0, &value)) {
			__raw_writel(0xfedc0000 | value, restart_reason);
#endif
        } else if (!strncmp(cmd, "diag", 4)
                  && !kstrtoul(cmd + 4, 0, &value)) {
                __raw_writel(0xabcc0000 | value, restart_reason);
		} else if (strlen(cmd) == 0) {
			pr_notice("%s : value of cmd is NULL.\n", __func__);
			__raw_writel(0x12345678, restart_reason);

		} else {
			__raw_writel(0x77665501, restart_reason);
		}
	} else {
		pr_notice("%s : clear reset flag\r\n", __func__);
		__raw_writel(0x12345678, restart_reason);
	}

	flush_cache_all();
	outer_flush_all();
}

static void msm_restart(enum reboot_mode reboot_mode, const char *cmd)
{
	pr_notice("Going down for restart now\n");

	msm_restart_prepare(cmd);

	__raw_writel(0, msm_tmr0_base + WDT0_EN);
	if (!(machine_is_msm8x60_fusion() || machine_is_msm8x60_fusn_ffa())) {
		mb();
		__raw_writel(0, PSHOLD_CTL_SU); /* Actually reset the chip */
		mdelay(5000);
		pr_notice("PS_HOLD didn't work, falling back to watchdog\n");
	}

	__raw_writel(1, msm_tmr0_base + WDT0_RST);
	__raw_writel(5*0x31F3, msm_tmr0_base + WDT0_BARK_TIME);
	__raw_writel(0x31F3, msm_tmr0_base + WDT0_BITE_TIME);
	__raw_writel(1, msm_tmr0_base + WDT0_EN);

	halt_spmi_pmic_arbiter();
	__raw_writel(0, PSHOLD_CTL_SU);


	mdelay(10000);
	pr_err("restarting has failed\n");
}

static int do_msm_poweroff(struct notifier_block *nb, unsigned long action,
			   void *data)
{
	msm_power_off();

	return NOTIFY_DONE;
}

static struct notifier_block restart_nb = {
	.notifier_call = do_msm_poweroff,
	.priority = 128,
};

#ifdef CONFIG_KEXEC_HARDBOOT
void msm_kexec_hardboot(void)
{
#if defined(CONFIG_MSM_DLOAD_MODE) && !defined(CONFIG_SEC_DEBUG)
	/* Do not enter download mode on reboot. */
	set_dload_mode(0);
#endif

	/* Set PM8XXX PMIC to reset on power off. */
	machinex_pm8xxx_restart_cmds(1);

	/* Reboot with the recovery kernel since the boot kernel decompressor may
	 * not support the hardboot jump. */
	__raw_writel(0x77665502, restart_reason);
}
#endif

static int __init msm_pmic_restart_init(void)
{
	int rc;

	if (scm_is_call_available(SCM_SVC_PWR, SCM_IO_DISABLE_PMIC_ARBITER) > 0)
		scm_pmic_arbiter_disable_supported = true;

	restart_wq = create_hipri_workqueue("restart_wq");
	if (!restart_wq) {
		pr_err("%s: out of memory\n", __func__);
		return -ENOMEM;
	}

	INIT_WORK(&resout_helper_work, resout_helper);

	if (pmic_reset_irq != 0) {
		rc = request_any_context_irq(pmic_reset_irq,
					resout_irq_handler, IRQF_TRIGGER_HIGH,
					"restart_from_pmic", NULL);
		if (rc < 0)
			pr_err("pmic restart irq fail rc = %d\n", rc);
		irq_enabled = 1;
		status = 1;
	} else {
		pr_warn("no pmic restart interrupt specified\n");
	}

	return 0;
}
late_initcall(msm_pmic_restart_init);

static int msm_restart_probe(struct platform_device *pdev)
{
#ifdef CONFIG_MSM_DLOAD_MODE
	atomic_notifier_chain_register(&panic_notifier_list, &panic_blk);
	dload_mode_addr = MSM_IMEM_BASE + DLOAD_MODE_ADDR;
	set_dload_mode(download_mode);
#endif
	msm_tmr0_base = msm_timer_get_timer0_base();
	restart_reason = MSM_IMEM_BASE + RESTART_REASON_ADDR;
	register_restart_handler(&restart_nb);
	pm_power_off = msm_power_off;
	arm_pm_restart = msm_restart;
#ifdef CONFIG_KEXEC_HARDBOOT
	kexec_hardboot_hook = msm_kexec_hardboot;
#endif
	return 0;
}

static struct platform_driver msm_restart_driver = {
	.probe = msm_restart_probe,
	.driver = {
		.name = "msm-restart",
	},
};

static int __init msm_restart_init(void)
{
	return platform_driver_register(&msm_restart_driver);
}
device_initcall(msm_restart_init);

