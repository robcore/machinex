/* Copyright (c) 2011-2012, The Linux Foundation. All rights reserved.
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

#ifndef __ASM_ARCH_MSM_IRQS_8064_H
#define __ASM_ARCH_MSM_IRQS_8064_H

/* MSM ACPU Interrupt Numbers */

/*
 * 0-15:  STI/SGI (software triggered/generated interrupts)
 * 16-31: PPI (private peripheral interrupts)
 * 32+:   SPI (shared peripheral interrupts)
 */

#define GIC_PPI_START 16
#define GIC_SPI_START 32

#define INT_VGIC				(GIC_PPI_START + 0)
#define INT_DEBUG_TIMER_EXP			(GIC_PPI_START + 1)
#define INT_GP_TIMER_EXP			(GIC_PPI_START + 2)
#define INT_GP_TIMER2_EXP			(GIC_PPI_START + 3)
#define WDT0_ACCSCSSNBARK_INT			(GIC_PPI_START + 4)
#define WDT1_ACCSCSSNBARK_INT			(GIC_PPI_START + 5)
#define AVS_SVICINT				(GIC_PPI_START + 6)
#define AVS_SVICINTSWDONE			(GIC_PPI_START + 7)
#define CPU_DBGCPUXCOMMRXFULL			(GIC_PPI_START + 8)
#define CPU_DBGCPUXCOMMTXEMPTY			(GIC_PPI_START + 9)
#define INT_ARMQC_PERFMON			(GIC_PPI_START + 10)
#define SC_AVSCPUXDOWN				(GIC_PPI_START + 11)
#define SC_AVSCPUXUP				(GIC_PPI_START + 12)
#define SC_SICCPUXACGIRPTREQ			(GIC_PPI_START + 13)
#define SC_SICCPUXEXTFAULTIRPTREQ		(GIC_PPI_START + 14)
/* PPI 15 is unused */

#define APCC_QGICACGIRPTREQ			(GIC_SPI_START + 0)
#define APCC_QGICL2PERFMONIRPTREQ		(GIC_SPI_START + 1)
#define SC_SICL2PERFMONIRPTREQ			APCC_QGICL2PERFMONIRPTREQ
#define APCC_QGICL2IRPTREQ			(GIC_SPI_START + 2)
#define APCC_QGICMPUIRPTREQ			(GIC_SPI_START + 3)
#define TLMM_MSM_DIR_CONN_IRQ_0			(GIC_SPI_START + 4)
#define TLMM_MSM_DIR_CONN_IRQ_1			(GIC_SPI_START + 5)
#define TLMM_MSM_DIR_CONN_IRQ_2			(GIC_SPI_START + 6)
#define TLMM_MSM_DIR_CONN_IRQ_3			(GIC_SPI_START + 7)
#define TLMM_MSM_DIR_CONN_IRQ_4			(GIC_SPI_START + 8)
#define TLMM_MSM_DIR_CONN_IRQ_5			(GIC_SPI_START + 9)
#define TLMM_MSM_DIR_CONN_IRQ_6			(GIC_SPI_START + 10)
#define TLMM_MSM_DIR_CONN_IRQ_7			(GIC_SPI_START + 11)
#define TLMM_MSM_DIR_CONN_IRQ_8			(GIC_SPI_START + 12)
#define TLMM_MSM_DIR_CONN_IRQ_9			(GIC_SPI_START + 13)
#define PM8921_SEC_IRQ_N			(GIC_SPI_START + 14)
#define PM8821_SEC_IRQ_N			(GIC_SPI_START + 15)
#define TLMM_MSM_SUMMARY_IRQ			(GIC_SPI_START + 16)
#define SPDM_RT_1_IRQ				(GIC_SPI_START + 17)
#define SPDM_DIAG_IRQ				(GIC_SPI_START + 18)
#define RPM_APCC_CPU0_GP_HIGH_IRQ		(GIC_SPI_START + 19)
#define RPM_APCC_CPU0_GP_MEDIUM_IRQ		(GIC_SPI_START + 20)
#define RPM_APCC_CPU0_GP_LOW_IRQ		(GIC_SPI_START + 21)
#define RPM_APCC_CPU0_WAKE_UP_IRQ		(GIC_SPI_START + 22)
#define RPM_APCC_CPU1_GP_HIGH_IRQ		(GIC_SPI_START + 23)
#define RPM_APCC_CPU1_GP_MEDIUM_IRQ		(GIC_SPI_START + 24)
#define RPM_APCC_CPU1_GP_LOW_IRQ		(GIC_SPI_START + 25)
#define RPM_APCC_CPU1_WAKE_UP_IRQ		(GIC_SPI_START + 26)
#define SSBI2_2_SC_CPU0_SECURE_IRQ		(GIC_SPI_START + 27)
#define SSBI2_2_SC_CPU0_NON_SECURE_IRQ		(GIC_SPI_START + 28)
#define SSBI2_1_SC_CPU0_SECURE_IRQ		(GIC_SPI_START + 29)
#define SSBI2_1_SC_CPU0_NON_SECURE_IRQ		(GIC_SPI_START + 30)
#define MSMC_SC_SEC_CE_IRQ			(GIC_SPI_START + 31)
#define MSMC_SC_PRI_CE_IRQ			(GIC_SPI_START + 32)
#define SLIMBUS0_CORE_EE1_IRQ			(GIC_SPI_START + 33)
#define SLIMBUS0_BAM_EE1_IRQ			(GIC_SPI_START + 34)
#define KPSS_SPARE_0				(GIC_SPI_START + 35)
#define GSS_A5_WDOG_EXPIRED			(GIC_SPI_START + 36)
#define GSS_TO_APPS_IRQ_0			(GIC_SPI_START + 37)
#define GSS_TO_APPS_IRQ_1			(GIC_SPI_START + 38)
#define GSS_TO_APPS_IRQ_2			(GIC_SPI_START + 39)
#define GSS_TO_APPS_IRQ_3			(GIC_SPI_START + 40)
#define GSS_TO_APPS_IRQ_4			(GIC_SPI_START + 41)
#define GSS_TO_APPS_IRQ_5			(GIC_SPI_START + 42)
#define GSS_TO_APPS_IRQ_6			(GIC_SPI_START + 43)
#define GSS_TO_APPS_IRQ_7			(GIC_SPI_START + 44)
#define GSS_TO_APPS_IRQ_8			(GIC_SPI_START + 45)
#define GSS_TO_APPS_IRQ_9			(GIC_SPI_START + 46)
#define VPE_IRQ					(GIC_SPI_START + 47)
#define VFE_IRQ					(GIC_SPI_START + 48)
#define VCODEC_IRQ				(GIC_SPI_START + 49)
#define KPSS_SPARE_1				(GIC_SPI_START + 50)
#define SMMU_VPE_CB_SC_SECURE_IRQ		(GIC_SPI_START + 51)
#define SMMU_VPE_CB_SC_NON_SECURE_IRQ		(GIC_SPI_START + 52)
#define SMMU_VFE_CB_SC_SECURE_IRQ		(GIC_SPI_START + 53)
#define SMMU_VFE_CB_SC_NON_SECURE_IRQ		(GIC_SPI_START + 54)
#define SMMU_VCODEC_B_CB_SC_SECURE_IRQ		(GIC_SPI_START + 55)
#define SMMU_VCODEC_B_CB_SC_NON_SECURE_IRQ	(GIC_SPI_START + 56)
#define SMMU_VCODEC_A_CB_SC_SECURE_IRQ		(GIC_SPI_START + 57)
#define SMMU_VCODEC_A_CB_SC_NON_SECURE_IRQ	(GIC_SPI_START + 58)
#define SMMU_ROT_CB_SC_SECURE_IRQ		(GIC_SPI_START + 59)
#define SMMU_ROT_CB_SC_NON_SECURE_IRQ		(GIC_SPI_START + 60)
#define SMMU_MDP1_CB_SC_SECURE_IRQ		(GIC_SPI_START + 61)
#define SMMU_MDP1_CB_SC_NON_SECURE_IRQ		(GIC_SPI_START + 62)
#define SMMU_MDP0_CB_SC_SECURE_IRQ		(GIC_SPI_START + 63)
#define SMMU_MDP0_CB_SC_NON_SECURE_IRQ		(GIC_SPI_START + 64)
#define SMMU_JPEGD_CB_SC_SECURE_IRQ		(GIC_SPI_START + 65)
#define SMMU_JPEGD_CB_SC_NON_SECURE_IRQ		(GIC_SPI_START + 66)
#define SMMU_IJPEG_CB_SC_SECURE_IRQ		(GIC_SPI_START + 67)
#define SMMU_IJPEG_CB_SC_NON_SECURE_IRQ		(GIC_SPI_START + 68)
#define SMMU_GFX3D_CB_SC_SECURE_IRQ		(GIC_SPI_START + 69)
#define SMMU_GFX3D_CB_SC_NON_SECURE_IRQ		(GIC_SPI_START + 70)
#define VCAP_VP					(GIC_SPI_START + 71)
#define VCAP_VC					(GIC_SPI_START + 72)
#define ROT_IRQ					(GIC_SPI_START + 73)
#define MMSS_FABRIC_IRQ				(GIC_SPI_START + 74)
#define MDP_IRQ					(GIC_SPI_START + 75)
#define JPEGD_IRQ				(GIC_SPI_START + 76)
#define JPEG_IRQ				(GIC_SPI_START + 77)
#define MMSS_IMEM_IRQ				(GIC_SPI_START + 78)
#define HDMI_IRQ				(GIC_SPI_START + 79)
#define GFX3D_IRQ				(GIC_SPI_START + 80)
#define GFX3d_VBIF_IRQ				(GIC_SPI_START + 81)
#define DSI1_IRQ				(GIC_SPI_START + 82)
#define CSI_1_IRQ				(GIC_SPI_START + 83)
#define CSI_0_IRQ				(GIC_SPI_START + 84)
#define LPASS_SCSS_AUDIO_IF_OUT0_IRQ		(GIC_SPI_START + 85)
#define LPASS_SCSS_MIDI_IRQ			(GIC_SPI_START + 86)
#define LPASS_Q6SS_WDOG_EXPIRED			(GIC_SPI_START + 87)
#define LPASS_SCSS_GP_LOW_IRQ			(GIC_SPI_START + 88)
#define LPASS_SCSS_GP_MEDIUM_IRQ		(GIC_SPI_START + 89)
#define LPASS_SCSS_GP_HIGH_IRQ			(GIC_SPI_START + 90)
#define TOP_IMEM_IRQ				(GIC_SPI_START + 91)
#define FABRIC_SYS_IRQ				(GIC_SPI_START + 92)
#define FABRIC_APPS_IRQ				(GIC_SPI_START + 93)
#define USB1_HS_BAM_IRQ				(GIC_SPI_START + 94)
#define SDC4_BAM_IRQ				(GIC_SPI_START + 95)
#define SDC3_BAM_IRQ				(GIC_SPI_START + 96)
#define SDC2_BAM_IRQ				(GIC_SPI_START + 97)
#define SDC1_BAM_IRQ				(GIC_SPI_START + 98)
#define FABRIC_SPS_IRQ				(GIC_SPI_START + 99)
#define USB1_HS_IRQ				(GIC_SPI_START + 100)
#define SDC4_IRQ_0				(GIC_SPI_START + 101)
#define SDC3_IRQ_0				(GIC_SPI_START + 102)
#define SDC2_IRQ_0				(GIC_SPI_START + 103)
#define SDC1_IRQ_0				(GIC_SPI_START + 104)
#define SPS_BAM_DMA_IRQ				(GIC_SPI_START + 105)
#define SPS_SEC_VIOL_IRQ			(GIC_SPI_START + 106)
#define SPS_MTI_0				(GIC_SPI_START + 107)
#define SPS_MTI_1				(GIC_SPI_START + 108)
#define SPS_MTI_2				(GIC_SPI_START + 109)
#define SPS_MTI_3				(GIC_SPI_START + 110)
#define SPS_MTI_4				(GIC_SPI_START + 111)
#define SPS_MTI_5				(GIC_SPI_START + 112)
#define SPS_MTI_6				(GIC_SPI_START + 113)
#define SPS_MTI_7				(GIC_SPI_START + 114)
#define SPS_MTI_8				(GIC_SPI_START + 115)
#define SPS_MTI_9				(GIC_SPI_START + 116)
#define SPS_MTI_10				(GIC_SPI_START + 117)
#define SPS_MTI_11				(GIC_SPI_START + 118)
#define SPS_MTI_12				(GIC_SPI_START + 119)
#define SPS_MTI_13				(GIC_SPI_START + 120)
#define SPS_MTI_14				(GIC_SPI_START + 121)
#define SPS_MTI_15				(GIC_SPI_START + 122)
#define SPS_MTI_16				(GIC_SPI_START + 123)
#define SPS_MTI_17				(GIC_SPI_START + 124)
#define SPS_MTI_18				(GIC_SPI_START + 125)
#define SPS_MTI_19				(GIC_SPI_START + 126)
#define SPS_MTI_20				(GIC_SPI_START + 127)
#define SPS_MTI_21				(GIC_SPI_START + 128)
#define SPS_MTI_22				(GIC_SPI_START + 129)
#define SPS_MTI_23				(GIC_SPI_START + 130)
#define SPS_MTI_24				(GIC_SPI_START + 131)
#define SPS_MTI_25				(GIC_SPI_START + 132)
#define SPS_MTI_26				(GIC_SPI_START + 133)
#define SPS_MTI_27				(GIC_SPI_START + 134)
#define SPS_MTI_28				(GIC_SPI_START + 135)
#define SPS_MTI_29				(GIC_SPI_START + 136)
#define SPS_MTI_30				(GIC_SPI_START + 137)
#define SPS_MTI_31				(GIC_SPI_START + 138)
#define CSIPHY_0_4LN_IRQ			(GIC_SPI_START + 139)
#define CSIPHY_1_2LN_IRQ			(GIC_SPI_START + 140)
#define KPSS_SPARE_2				(GIC_SPI_START + 141)
#define USB1_IRQ				(GIC_SPI_START + 142)
#define TSSC_SSBI_IRQ				(GIC_SPI_START + 143)
#define TSSC_SAMPLE_IRQ				(GIC_SPI_START + 144)
#define TSSC_PENUP_IRQ				(GIC_SPI_START + 145)
#define KPSS_SPARE_3				(GIC_SPI_START + 146)
#define KPSS_SPARE_4				(GIC_SPI_START + 147)
#define KPSS_SPARE_5				(GIC_SPI_START + 148)
#define KPSS_SPARE_6			        (GIC_SPI_START + 149)
#define GSBI3_UARTDM_IRQ			(GIC_SPI_START + 150)
#define GSBI3_QUP_IRQ				(GIC_SPI_START + 151)
#define GSBI4_UARTDM_IRQ			(GIC_SPI_START + 152)
#define GSBI4_QUP_IRQ				(GIC_SPI_START + 153)
#define GSBI5_UARTDM_IRQ			(GIC_SPI_START + 154)
#define GSBI5_QUP_IRQ				(GIC_SPI_START + 155)
#define GSBI6_UARTDM_IRQ			(GIC_SPI_START + 156)
#define GSBI6_QUP_IRQ				(GIC_SPI_START + 157)
#define GSBI7_UARTDM_IRQ			(GIC_SPI_START + 158)
#define GSBI7_QUP_IRQ				(GIC_SPI_START + 159)
#define KPSS_SPARE_7				(GIC_SPI_START + 160)
#define KPSS_SPARE_8				(GIC_SPI_START + 161)
#define TSIF_TSPP_IRQ				(GIC_SPI_START + 162)
#define TSIF_BAM_IRQ				(GIC_SPI_START + 163)
#define TSIF2_IRQ				(GIC_SPI_START + 164)
#define TSIF1_IRQ				(GIC_SPI_START + 165)
#define DSI2_IRQ				(GIC_SPI_START + 166)
#define ISPIF_IRQ				(GIC_SPI_START + 167)
#define MSMC_SC_SEC_TMR_IRQ			(GIC_SPI_START + 168)
#define MSMC_SC_SEC_WDOG_BARK_IRQ		(GIC_SPI_START + 169)
#define ADM_0_SCSS_0_IRQ			(GIC_SPI_START + 170)
#define ADM_0_SCSS_1_IRQ			(GIC_SPI_START + 171)
#define ADM_0_SCSS_2_IRQ			(GIC_SPI_START + 172)
#define ADM_0_SCSS_3_IRQ			(GIC_SPI_START + 173)
#define CC_SCSS_WDT1CPU1BITEEXPIRED		(GIC_SPI_START + 174)
#define CC_SCSS_WDT1CPU0BITEEXPIRED		(GIC_SPI_START + 175)
#define CC_SCSS_WDT0CPU1BITEEXPIRED		(GIC_SPI_START + 176)
#define CC_SCSS_WDT0CPU0BITEEXPIRED		(GIC_SPI_START + 177)
#define TSENS_UPPER_LOWER_INT			(GIC_SPI_START + 178)
#define SSBI2_2_SC_CPU1_SECURE_INT		(GIC_SPI_START + 179)
#define SSBI2_2_SC_CPU1_NON_SECURE_INT		(GIC_SPI_START + 180)
#define SSBI2_1_SC_CPU1_SECURE_INT		(GIC_SPI_START + 181)
#define SSBI2_1_SC_CPU1_NON_SECURE_INT		(GIC_SPI_START + 182)
#define XPU_SUMMARY_IRQ				(GIC_SPI_START + 183)
#define BUS_EXCEPTION_SUMMARY_IRQ		(GIC_SPI_START + 184)
#define HSDDRX_EBI1CH0_IRQ			(GIC_SPI_START + 185)
#define HSDDRX_EBI1CH1_IRQ			(GIC_SPI_START + 186)
#define USB3_HS_BAM_IRQ				(GIC_SPI_START + 187)
#define USB3_HS_IRQ				(GIC_SPI_START + 188)
#define CC_SCSS_WDT1CPU3BITEEXPIRED		(GIC_SPI_START + 189)
#define CC_SCSS_WDT1CPU2BITEEXPIRED		(GIC_SPI_START + 190)
#define CC_SCSS_WDT0CPU3BITEEXPIRED		(GIC_SPI_START + 191)
#define CC_SCSS_WDT0CPU2BITEEXPIRED		(GIC_SPI_START + 192)
#define APQ8064_GSBI1_UARTDM_IRQ		(GIC_SPI_START + 193)
#define APQ8064_GSBI1_QUP_IRQ			(GIC_SPI_START + 194)
#define APQ8064_GSBI2_UARTDM_IRQ		(GIC_SPI_START + 195)
#define APQ8064_GSBI2_QUP_IRQ			(GIC_SPI_START + 196)
#define RIVA_APSS_LTECOEX_IRQ			(GIC_SPI_START + 197)
#define RIVA_APSS_SPARE_IRQ			(GIC_SPI_START + 198)
#define RIVA_APSS_WDOG_BITE_RESET_RDY_IRQ	(GIC_SPI_START + 199)
#define RIVA_APSS_RESET_DONE_IRQ		(GIC_SPI_START + 200)
#define RIVA_APSS_ASIC_IRQ			(GIC_SPI_START + 201)
#define RIVA_APPS_WLAN_RX_DATA_AVAIL_IRQ	(GIC_SPI_START + 202)
#define RIVA_APPS_WLAN_DATA_XFER_DONE_IRQ	(GIC_SPI_START + 203)
#define RIVA_APPS_WLAN_SMSM_IRQ			(GIC_SPI_START + 204)
#define RIVA_APPS_LOG_CTRL_IRQ			(GIC_SPI_START + 205)
#define RIVA_APPS_FM_CTRL_IRQ			(GIC_SPI_START + 206)
#define RIVA_APPS_HCI_IRQ			(GIC_SPI_START + 207)
#define RIVA_APPS_WLAN_CTRL_IRQ			(GIC_SPI_START + 208)
#define SATA_CONTROLLER_IRQ			(GIC_SPI_START + 209)
#define SMMU_GFX3D1_CB_SC_SECURE_IRQ		(GIC_SPI_START + 210)
#define SMMU_GFX3D1_CB_SC_NON_SECURE_IRQ	(GIC_SPI_START + 211)
#define KPSS_SPARE_9				(GIC_SPI_START + 212)
#define PPSS_WDOG_TIMER_IRQ			(GIC_SPI_START + 213)
#define USB4_HS_BAM_IRQ				(GIC_SPI_START + 214)
#define USB4_HS_IRQ				(GIC_SPI_START + 215)
#define QDSS_ETB_IRQ				(GIC_SPI_START + 216)
#define QDSS_CTI2KPSS_CPU1_IRQ			(GIC_SPI_START + 217)
#define QDSS_CTI2KPSS_CPU0_IRQ			(GIC_SPI_START + 218)
#define TLMM_MSM_DIR_CONN_IRQ_16		(GIC_SPI_START + 219)
#define TLMM_MSM_DIR_CONN_IRQ_17		(GIC_SPI_START + 220)
#define TLMM_MSM_DIR_CONN_IRQ_18		(GIC_SPI_START + 221)
#define TLMM_MSM_DIR_CONN_IRQ_19		(GIC_SPI_START + 222)
#define TLMM_MSM_DIR_CONN_IRQ_20		(GIC_SPI_START + 223)
#define TLMM_MSM_DIR_CONN_IRQ_21		(GIC_SPI_START + 224)
#define PM8921_USR_IRQ_N			(GIC_SPI_START + 225)
#define PM8821_USR_IRQ_N			(GIC_SPI_START + 226)

#define	CSI_2_IRQ				(GIC_SPI_START + 227)
#define	APQ8064_CSIPHY_2LN_IRQ			(GIC_SPI_START + 228)
#define	USB2_HSIC_IRQ				(GIC_SPI_START + 229)
#define	CE2_BAM_XPU_IRQ				(GIC_SPI_START + 230)
#define	CE1_BAM_XPU_IRQ				(GIC_SPI_START + 231)
#define	RPM_SCSS_CPU2_WAKE_UP_IRQ		(GIC_SPI_START + 232)
#define	RPM_SCSS_CPU3_WAKE_UP_IRQ		(GIC_SPI_START + 233)
#define	CS3_BAM_XPU_IRQ				(GIC_SPI_START + 234)
#define	CE3_IRQ					(GIC_SPI_START + 235)
#define	SMMU_VCAP_CB_SC_SECURE_IRQ		(GIC_SPI_START + 236)
#define	SMMU_VCAP_CB_SC_NON_SECURE_IRQ		(GIC_SPI_START + 237)
#define	PCIE20_INT_MSI				(GIC_SPI_START + 238)
#define	PCIE20_INTA				(GIC_SPI_START + 239)
#define	PCIE20_INTB				(GIC_SPI_START + 240)
#define	PCIE20_INTC				(GIC_SPI_START + 241)
#define	PCIE20_INTD				(GIC_SPI_START + 242)
#define	PCIE20_INT_PLS_HP			(GIC_SPI_START + 243)
#define	PCIE20_INT_PLS_ERR			(GIC_SPI_START + 244)
#define	PCIE20_INT_PLS_PME			(GIC_SPI_START + 245)
#define	PCIE20_INT_LINK_UP			(GIC_SPI_START + 246)
#define	PCIE20_INT_LINK_DOWN			(GIC_SPI_START + 247)
#define	PCIE20_INT_HP_LEGACY			(GIC_SPI_START + 248)
#define	PCIE20_INT_AER_LEGACY			(GIC_SPI_START + 249)
#define	PCIE20_INT_PME_LEGACY			(GIC_SPI_START + 250)
#define	PCIE20_INT_BRIDGE_FLUSH_N		(GIC_SPI_START + 251)

/* Backwards compatible IRQ macros. */
#define INT_ADM_AARM				ADM_0_SCSS_0_IRQ

/* smd/smsm interrupts */
#define INT_A9_M2A_0		(GIC_SPI_START + 37) /*GSS_TO_APPS_IRQ_0*/
#define INT_A9_M2A_5		(GIC_SPI_START + 38) /*GSS_TO_APPS_IRQ_1*/
#define INT_ADSP_A11		LPASS_SCSS_GP_HIGH_IRQ
#define INT_ADSP_A11_SMSM	LPASS_SCSS_GP_MEDIUM_IRQ
#define INT_DSPS_A11		SPS_MTI_31
#define INT_DSPS_A11_SMSM	SPS_MTI_30
#define INT_WCNSS_A11		RIVA_APSS_SPARE_IRQ
#define INT_WCNSS_A11_SMSM	RIVA_APPS_WLAN_SMSM_IRQ

#endif

