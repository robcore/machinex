/*
 * Misc utility routines for accessing the SOC Interconnects
 * of Broadcom HNBU chips.
 *
 * Copyright (C) 1999-2014, Broadcom Corporation
 * 
 *      Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed to you
 * under the terms of the GNU General Public License version 2 (the "GPL"),
 * available at http://www.broadcom.com/licenses/GPLv2.php, with the
 * following added to such license:
 * 
 *      As a special exception, the copyright holders of this software give you
 * permission to link this software with independent modules, and to copy and
 * distribute the resulting executable under terms of your choice, provided that
 * you also meet, for each linked independent module, the terms and conditions of
 * the license of that module.  An independent module is a module which is not
 * derived from this software.  The special exception does not apply to any
 * modifications of the software.
 * 
 *      Notwithstanding the above, under no circumstances may you combine this
 * software in any way with any other Broadcom software provided under a license
 * other than the GPL, without Broadcom's express prior written consent.
 *
 * $Id: siutils.h 509862 2014-10-22 06:25:12Z $
 */

#ifndef	_siutils_h_
#define	_siutils_h_


#include <bcmutils.h>
/*
 * Data structure to export all chip specific common variables
 *   public (read-only) portion of siutils handle returned by si_attach()/si_kattach()
 */
struct si_pub {
	uint	socitype;		/* SOCI_SB, SOCI_AI */

	uint	bustype;		/* SI_BUS, PCI_BUS */
	uint	buscoretype;		/* PCI_CORE_ID, PCIE_CORE_ID, PCMCIA_CORE_ID */
	uint	buscorerev;		/* buscore rev */
	uint	buscoreidx;		/* buscore index */
	int	ccrev;			/* chip common core rev */
	uint32	cccaps;			/* chip common capabilities */
	uint32  cccaps_ext;			/* chip common capabilities extension */
	int	pmurev;			/* pmu core rev */
	uint32	pmucaps;		/* pmu capabilities */
	uint	boardtype;		/* board type */
	uint    boardrev;               /* board rev */
	uint	boardvendor;		/* board vendor */
	uint	boardflags;		/* board flags */
	uint	boardflags2;		/* board flags2 */
	uint	chip;			/* chip number */
	uint	chiprev;		/* chip revision */
	uint	chippkg;		/* chip package option */
	uint32	chipst;			/* chip status */
	bool	issim;			/* chip is in simulation or emulation */
	uint    socirev;		/* SOC interconnect rev */
	bool	pci_pr32414;

};

/* for HIGH_ONLY driver, the si_t must be writable to allow states sync from BMAC to HIGH driver
 * for monolithic driver, it is readonly to prevent accident change
 */
typedef const struct si_pub si_t;


/*
 * Many of the routines below take an 'sih' handle as their first arg.
 * Allocate this by calling si_attach().  Free it by calling si_detach().
 * At any one time, the sih is logically focused on one particular si core
 * (the "current core").
 * Use si_setcore() or si_setcoreidx() to change the association to another core.
 */
#define	SI_OSH		NULL	/* Use for si_kattach when no osh is available */

#define	BADIDX		(SI_MAXCORES + 1)

/* clkctl xtal what flags */
#define	XTAL			0x1	/* primary crystal oscillator (2050) */
#define	PLL			0x2	/* main chip pll */

/* clkctl clk mode */
#define	CLK_FAST		0	/* force fast (pll) clock */
#define	CLK_DYNAMIC		2	/* enable dynamic clock control */

/* GPIO usage priorities */
#define GPIO_DRV_PRIORITY	0	/* Driver */
#define GPIO_APP_PRIORITY	1	/* Application */
#define GPIO_HI_PRIORITY	2	/* Highest priority. Ignore GPIO reservation */

/* GPIO pull up/down */
#define GPIO_PULLUP		0
#define GPIO_PULLDN		1

/* GPIO event regtype */
#define GPIO_REGEVT		0	/* GPIO register event */
#define GPIO_REGEVT_INTMSK	1	/* GPIO register event int mask */
#define GPIO_REGEVT_INTPOL	2	/* GPIO register event int polarity */

/* device path */
#define SI_DEVPATH_BUFSZ	16	/* min buffer size in bytes */

/* SI routine enumeration: to be used by update function with multiple hooks */
#define	SI_DOATTACH	1
#define SI_PCIDOWN	2
#define SI_PCIUP	3

#define	ISSIM_ENAB(sih)	FALSE

/* PMU clock/power control */
#if defined(BCMPMUCTL)
#define PMUCTL_ENAB(sih)	(BCMPMUCTL)
#else
#define PMUCTL_ENAB(sih)	((sih)->cccaps & CC_CAP_PMU)
#endif

/* chipcommon clock/power control (exclusive with PMU's) */
#if defined(BCMPMUCTL) && BCMPMUCTL
#define CCCTL_ENAB(sih)		(0)
#define CCPLL_ENAB(sih)		(0)
#else
#define CCCTL_ENAB(sih)		((sih)->cccaps & CC_CAP_PWR_CTL)
#define CCPLL_ENAB(sih)		((sih)->cccaps & CC_CAP_PLL_MASK)
#endif

typedef void (*gpio_handler_t)(uint32 stat, void *arg);
typedef void (*gci_gpio_handler_t)(uint32 stat, void *arg);
/* External BT Coex enable mask */
#define CC_BTCOEX_EN_MASK  0x01
/* External PA enable mask */
#define GPIO_CTRL_EPA_EN_MASK 0x40
/* WL/BT control enable mask */
#define GPIO_CTRL_5_6_EN_MASK 0x60
#define GPIO_CTRL_7_6_EN_MASK 0xC0
#define GPIO_OUT_7_EN_MASK 0x80


/* CR4 specific defines used by the host driver */
#define SI_CR4_CAP			(0x04)
#define SI_CR4_BANKIDX		(0x40)
#define SI_CR4_BANKINFO		(0x44)
#define SI_CR4_BANKPDA		(0x4C)

#define	ARMCR4_TCBBNB_MASK	0xf0
#define	ARMCR4_TCBBNB_SHIFT	4
#define	ARMCR4_TCBANB_MASK	0xf
#define	ARMCR4_TCBANB_SHIFT	0

#define	SICF_CPUHALT		(0x0020)
#define	ARMCR4_BSZ_MASK		0x3f
#define	ARMCR4_BSZ_MULT		8192

/* === exported functions === */
extern si_t *si_attach(uint pcidev, osl_t *osh, void *regs, uint bustype,
                       void *sdh, char **vars, uint *varsz);
extern si_t *si_kattach(osl_t *osh);
extern void si_detach(si_t *sih);
extern bool si_pci_war16165(si_t *sih);

extern uint si_corelist(si_t *sih, uint coreid[]);
extern uint si_coreid(si_t *sih);
extern uint si_flag(si_t *sih);
extern uint si_flag_alt(si_t *sih);
extern uint si_intflag(si_t *sih);
extern uint si_coreidx(si_t *sih);
extern uint si_coreunit(si_t *sih);
extern uint si_corevendor(si_t *sih);
extern uint si_corerev(si_t *sih);
extern void *si_osh(si_t *sih);
extern void si_setosh(si_t *sih, osl_t *osh);
extern uint si_corereg(si_t *sih, uint coreidx, uint regoff, uint mask, uint val);
extern uint32 *si_corereg_addr(si_t *sih, uint coreidx, uint regoff);
extern void *si_coreregs(si_t *sih);
extern uint si_wrapperreg(si_t *sih, uint32 offset, uint32 mask, uint32 val);
extern uint si_core_wrapperreg(si_t *sih, uint32 coreidx, uint32 offset, uint32 mask, uint32 val);
extern void *si_wrapperregs(si_t *sih);
extern uint32 si_core_cflags(si_t *sih, uint32 mask, uint32 val);
extern void si_core_cflags_wo(si_t *sih, uint32 mask, uint32 val);
extern uint32 si_core_sflags(si_t *sih, uint32 mask, uint32 val);
extern bool si_iscoreup(si_t *sih);
extern uint si_numcoreunits(si_t *sih, uint coreid);
extern uint si_findcoreidx(si_t *sih, uint coreid, uint coreunit);
extern void *si_setcoreidx(si_t *sih, uint coreidx);
extern void *si_setcore(si_t *sih, uint coreid, uint coreunit);
extern void *si_switch_core(si_t *sih, uint coreid, uint *origidx, uint *intr_val);
extern void si_restore_core(si_t *sih, uint coreid, uint intr_val);
extern int si_numaddrspaces(si_t *sih);
extern uint32 si_addrspace(si_t *sih, uint asidx);
extern uint32 si_addrspacesize(si_t *sih, uint asidx);
extern void si_coreaddrspaceX(si_t *sih, uint asidx, uint32 *addr, uint32 *size);
extern int si_corebist(si_t *sih);
extern void si_core_reset(si_t *sih, uint32 bits, uint32 resetbits);
extern void si_core_disable(si_t *sih, uint32 bits);
extern uint32 si_clock_rate(uint32 pll_type, uint32 n, uint32 m);
extern uint si_chip_hostif(si_t *sih);
extern bool si_read_pmu_autopll(si_t *sih);
extern uint32 si_clock(si_t *sih);
extern uint32 si_alp_clock(si_t *sih);
extern uint32 si_ilp_clock(si_t *sih);
extern void si_pci_setup(si_t *sih, uint coremask);
extern void si_pcmcia_init(si_t *sih);
extern void si_setint(si_t *sih, int siflag);
extern bool si_backplane64(si_t *sih);
extern void si_register_intr_callback(si_t *sih, void *intrsoff_fn, void *intrsrestore_fn,
	void *intrsenabled_fn, void *intr_arg);
extern void si_deregister_intr_callback(si_t *sih);
extern void si_clkctl_init(si_t *sih);
extern uint16 si_clkctl_fast_pwrup_delay(si_t *sih);
extern bool si_clkctl_cc(si_t *sih, uint mode);
extern int si_clkctl_xtal(si_t *sih, uint what, bool on);
extern uint32 si_gpiotimerval(si_t *sih, uint32 mask, uint32 val);
extern void si_btcgpiowar(si_t *sih);
extern bool si_deviceremoved(si_t *sih);
extern uint32 si_socram_size(si_t *sih);
extern uint32 si_socdevram_size(si_t *sih);
extern uint32 si_socram_srmem_size(si_t *sih);
extern void si_socram_set_bankpda(si_t *sih, uint32 bankidx, uint32 bankpda);
extern void si_socdevram(si_t *sih, bool set, uint8 *ennable, uint8 *protect, uint8 *remap);
extern bool si_socdevram_pkg(si_t *sih);
extern bool si_socdevram_remap_isenb(si_t *sih);
extern uint32 si_socdevram_remap_size(si_t *sih);

extern void si_watchdog(si_t *sih, uint ticks);
extern void si_watchdog_ms(si_t *sih, uint32 ms);
extern uint32 si_watchdog_msticks(void);
extern void *si_gpiosetcore(si_t *sih);
extern uint32 si_gpiocontrol(si_t *sih, uint32 mask, uint32 val, uint8 priority);
extern uint32 si_gpioouten(si_t *sih, uint32 mask, uint32 val, uint8 priority);
extern uint32 si_gpioout(si_t *sih, uint32 mask, uint32 val, uint8 priority);
extern uint32 si_gpioin(si_t *sih);
extern uint32 si_gpiointpolarity(si_t *sih, uint32 mask, uint32 val, uint8 priority);
extern uint32 si_gpiointmask(si_t *sih, uint32 mask, uint32 val, uint8 priority);
extern uint32 si_gpioled(si_t *sih, uint32 mask, uint32 val);
extern uint32 si_gpioreserve(si_t *sih, uint32 gpio_num, uint8 priority);
extern uint32 si_gpiorelease(si_t *sih, uint32 gpio_num, uint8 priority);
extern uint32 si_gpiopull(si_t *sih, bool updown, uint32 mask, uint32 val);
extern uint32 si_gpioevent(si_t *sih, uint regtype, uint32 mask, uint32 val);
extern uint32 si_gpio_int_enable(si_t *sih, bool enable);
extern void si_gci_enable_gpio(si_t *sih, uint8 gpio, uint32 mask, uint32 value);
extern uint8 si_gci_host_wake_gpio_init(si_t *sih);
extern void si_gci_host_wake_gpio_enable(si_t *sih, uint8 gpio, bool state);

/* GPIO event handlers */
extern void *si_gpio_handler_register(si_t *sih, uint32 e, bool lev, gpio_handler_t cb, void *arg);
extern void si_gpio_handler_unregister(si_t *sih, void* gpioh);
extern void si_gpio_handler_process(si_t *sih);

/* GCI interrupt handlers */
extern void si_gci_handler_process(si_t *sih);

/* GCI GPIO event handlers */
extern void *si_gci_gpioint_handler_register(si_t *sih, uint8 gpio, uint8 sts,
	gci_gpio_handler_t cb, void *arg);
extern void si_gci_gpioint_handler_unregister(si_t *sih, void* gci_i);

/* Wake-on-wireless-LAN (WOWL) */
extern bool si_pci_pmecap(si_t *sih);
struct osl_info;
extern bool si_pci_fastpmecap(struct osl_info *osh);
extern bool si_pci_pmestat(si_t *sih);
extern void si_pci_pmeclr(si_t *sih);
extern void si_pci_pmeen(si_t *sih);
extern void si_pci_pmestatclr(si_t *sih);
extern uint si_pcie_readreg(void *sih, uint addrtype, uint offset);
extern uint si_pcie_writereg(void *sih, uint addrtype, uint offset, uint val);


extern void si_sdio_init(si_t *sih);

extern uint16 si_d11_devid(si_t *sih);
extern int si_corepciid(si_t *sih, uint func, uint16 *pcivendor, uint16 *pcidevice,
	uint8 *pciclass, uint8 *pcisubclass, uint8 *pciprogif, uint8 *pciheader);

#define si_eci(sih) 0
static INLINE void * si_eci_init(si_t *sih) {return NULL;}
#define si_eci_notify_bt(sih, type, val)  (0)
#define si_seci(sih) 0
#define si_seci_upd(sih, a)	do {} while (0)
static INLINE void * si_seci_init(si_t *sih, uint8 use_seci) {return NULL;}
static INLINE void * si_gci_init(si_t *sih) {return NULL;}
#define si_seci_down(sih) do {} while (0)
#define si_gci(sih) 0

/* OTP status */
extern bool si_is_otp_disabled(si_t *sih);
extern bool si_is_otp_powered(si_t *sih);
extern void si_otp_power(si_t *sih, bool on, uint32* min_res_mask);

/* SPROM availability */
extern bool si_is_sprom_available(si_t *sih);
extern bool si_is_sprom_enabled(si_t *sih);
extern void si_sprom_enable(si_t *sih, bool enable);

/* OTP/SROM CIS stuff */
extern int si_cis_source(si_t *sih);
#define CIS_DEFAULT	0
#define CIS_SROM	1
#define CIS_OTP		2

/* Fab-id information */
#define	DEFAULT_FAB	0x0	/* Original/first fab used for this chip */
#define	CSM_FAB7	0x1	/* CSM Fab7 chip */
#define	TSMC_FAB12	0x2	/* TSMC Fab12/Fab14 chip */
#define	SMIC_FAB4	0x3	/* SMIC Fab4 chip */

extern int si_otp_fabid(si_t *sih, uint16 *fabid, bool rw);
extern uint16 si_fabid(si_t *sih);

/*
 * Build device path. Path size must be >= SI_DEVPATH_BUFSZ.
 * The returned path is NULL terminated and has trailing '/'.
 * Return 0 on success, nonzero otherwise.
 */
extern int si_devpath(si_t *sih, char *path, int size);
/* Read variable with prepending the devpath to the name */
extern char *si_getdevpathvar(si_t *sih, const char *name);
extern int si_getdevpathintvar(si_t *sih, const char *name);
extern char *si_coded_devpathvar(si_t *sih, char *varname, int var_len, const char *name);


extern uint8 si_pcieclkreq(si_t *sih, uint32 mask, uint32 val);
extern uint32 si_pcielcreg(si_t *sih, uint32 mask, uint32 val);
extern uint8 si_pcieltrenable(si_t *sih, uint32 mask, uint32 val);
extern uint8 si_pcieobffenable(si_t *sih, uint32 mask, uint32 val);
extern uint32 si_pcieltr_reg(si_t *sih, uint32 reg, uint32 mask, uint32 val);
extern uint32 si_pcieltrspacing_reg(si_t *sih, uint32 mask, uint32 val);
extern uint32 si_pcieltrhysteresiscnt_reg(si_t *sih, uint32 mask, uint32 val);
extern void si_pcie_set_error_injection(si_t *sih, uint32 mode);
extern void si_pcie_set_L1substate(si_t *sih, uint32 substate);
extern uint32 si_pcie_get_L1substate(si_t *sih);
extern void si_war42780_clkreq(si_t *sih, bool clkreq);
extern void si_pci_down(si_t *sih);
extern void si_pci_up(si_t *sih);
extern void si_pci_sleep(si_t *sih);
extern void si_pcie_war_ovr_update(si_t *sih, uint8 aspm);
extern void si_pcie_power_save_enable(si_t *sih, bool enable);
extern void si_pcie_extendL1timer(si_t *sih, bool extend);
extern int si_pci_fixcfg(si_t *sih);
extern void si_chippkg_set(si_t *sih, uint);

extern void si_chipcontrl_btshd0_4331(si_t *sih, bool on);
extern void si_chipcontrl_restore(si_t *sih, uint32 val);
extern uint32 si_chipcontrl_read(si_t *sih);
extern void si_chipcontrl_epa4331(si_t *sih, bool on);
extern void si_chipcontrl_epa4331_wowl(si_t *sih, bool enter_wowl);
extern void si_chipcontrl_srom4360(si_t *sih, bool on);
/* Enable BT-COEX & Ex-PA for 4313 */
extern void si_epa_4313war(si_t *sih);
extern void si_btc_enable_chipcontrol(si_t *sih);
/* BT/WL selection for 4313 bt combo >= P250 boards */
extern void si_btcombo_p250_4313_war(si_t *sih);
extern void si_btcombo_43228_war(si_t *sih);
extern void si_clk_pmu_htavail_set(si_t *sih, bool set_clear);
extern void si_pmu_synth_pwrsw_4313_war(si_t *sih);
extern uint si_pll_reset(si_t *sih);
/* === debug routines === */

extern bool si_taclear(si_t *sih, bool details);



extern uint32 si_ccreg(si_t *sih, uint32 offset, uint32 mask, uint32 val);
extern uint32 si_pciereg(si_t *sih, uint32 offset, uint32 mask, uint32 val, uint type);
extern uint32 si_pcieserdesreg(si_t *sih, uint32 mdioslave, uint32 offset, uint32 mask, uint32 val);
extern void si_pcie_set_request_size(si_t *sih, uint16 size);
extern uint16 si_pcie_get_request_size(si_t *sih);
extern void si_pcie_set_maxpayload_size(si_t *sih, uint16 size);
extern uint16 si_pcie_get_maxpayload_size(si_t *sih);
extern uint16 si_pcie_get_ssid(si_t *sih);
extern uint32 si_pcie_get_bar0(si_t *sih);
extern int si_pcie_configspace_cache(si_t *sih);
extern int si_pcie_configspace_restore(si_t *sih);
extern int si_pcie_configspace_get(si_t *sih, uint8 *buf, uint size);

char *si_getnvramflvar(si_t *sih, const char *name);


extern uint32 si_tcm_size(si_t *sih);
extern bool si_has_flops(si_t *sih);

extern int si_set_sromctl(si_t *sih, uint32 value);
extern uint32 si_get_sromctl(si_t *sih);

extern uint32 si_gci_direct(si_t *sih, uint offset, uint32 mask, uint32 val);
extern uint32 si_gci_indirect(si_t *sih, uint regidx, uint offset, uint32 mask, uint32 val);
extern uint32 si_gci_output(si_t *sih, uint reg, uint32 mask, uint32 val);
extern uint32 si_gci_input(si_t *sih, uint reg);
extern uint32 si_gci_int_enable(si_t *sih, bool enable);
extern void si_gci_reset(si_t *sih);
extern void si_ercx_init(si_t *sih);
extern void si_wci2_init(si_t *sih, uint baudrate);
extern void si_gci_seci_init(si_t *sih);
extern void si_gci_set_functionsel(si_t *sih, uint32 pin, uint8 fnsel);
extern uint8 si_gci_get_chipctrlreg_idx(uint32 pin, uint32 *regidx, uint32 *pos);
extern uint32 si_gci_chipcontrol(si_t *sih, uint reg, uint32 mask, uint32 val);
extern uint32 si_gci_chipstatus(si_t *sih, uint reg);
extern uint16 si_cc_get_reg16(uint32 reg_offs);
extern uint32 si_cc_get_reg32(uint32 reg_offs);
extern uint32 si_cc_set_reg32(uint32 reg_offs, uint32 val);
extern uint32 si_gci_preinit_upd_indirect(uint32 regidx, uint32 setval, uint32 mask);
extern uint8 si_enable_device_wake(si_t *sih, uint8 *wake_status, uint8 *cur_status);

#define CHIPCTRLREG1 0x1
#define CHIPCTRLREG2 0x2
#define CHIPCTRLREG3 0x3
#define CHIPCTRLREG4 0x4
#define CHIPCTRLREG5 0x5
#define MINRESMASKREG 0x618
#define MAXRESMASKREG 0x61c
#define CHIPCTRLADDR 0x650
#define CHIPCTRLDATA 0x654
#define RSRCTABLEADDR 0x620
#define RSRCUPDWNTIME 0x628
#define PMUREG_RESREQ_MASK 0x68c

void si_update_masks(si_t *sih);
void si_force_islanding(si_t *sih, bool enable);
extern uint32 si_pmu_res_req_timer_clr(si_t *sih);
extern void si_pmu_rfldo(si_t *sih, bool on);
extern void si_survive_perst_war(si_t *sih, bool reset, uint32 sperst_mask, uint32 spert_val);
extern void si_pcie_ltr_war(si_t *sih);

/* Macro to enable clock gating changes in different cores */
#define MEM_CLK_GATE_BIT 	5
#define GCI_CLK_GATE_BIT 	18

#define USBAPP_CLK_BIT		0
#define PCIE_CLK_BIT		3
#define ARMCR4_DBG_CLK_BIT	4
#define SAMPLE_SYNC_CLK_BIT 	17
#define PCIE_TL_CLK_BIT		18
#define HQ_REQ_BIT		24
#define PLL_DIV2_BIT_START	9
#define PLL_DIV2_MASK		(0x37 << PLL_DIV2_BIT_START)
#define PLL_DIV2_DIS_OP		(0x37 << PLL_DIV2_BIT_START)

#endif	/* _siutils_h_ */
