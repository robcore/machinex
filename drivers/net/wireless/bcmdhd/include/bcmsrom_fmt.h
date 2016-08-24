/*
 * SROM format definition.
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
 * $Id: bcmsrom_fmt.h 427005 2013-10-02 00:15:10Z $
 */

#ifndef	_bcmsrom_fmt_h_
#define	_bcmsrom_fmt_h_

#define SROM_MAXREV		11	/* max revisiton supported by driver */

/* Maximum srom: 6 Kilobits == 768 bytes */
#define	SROM_MAX		768
#define SROM_MAXW		384
#define VARS_MAX		4096

/* PCI fields */
#define PCI_F0DEVID		48


#define	SROM_WORDS		64

#define SROM3_SWRGN_OFF		28	/* s/w region offset in words */

#define	SROM_SSID		2
#define	SROM_SVID		3

#define	SROM_WL1LHMAXP		29

#define	SROM_WL1LPAB0		30
#define	SROM_WL1LPAB1		31
#define	SROM_WL1LPAB2		32

#define	SROM_WL1HPAB0		33
#define	SROM_WL1HPAB1		34
#define	SROM_WL1HPAB2		35

#define	SROM_MACHI_IL0		36
#define	SROM_MACMID_IL0		37
#define	SROM_MACLO_IL0		38
#define	SROM_MACHI_ET0		39
#define	SROM_MACMID_ET0		40
#define	SROM_MACLO_ET0		41
#define	SROM_MACHI_ET1		42
#define	SROM_MACMID_ET1		43
#define	SROM_MACLO_ET1		44
#define	SROM3_MACHI		37
#define	SROM3_MACMID		38
#define	SROM3_MACLO		39

#define	SROM_BXARSSI2G		40
#define	SROM_BXARSSI5G		41

#define	SROM_TRI52G		42
#define	SROM_TRI5GHL		43

#define	SROM_RXPO52G		45

#define	SROM2_ENETPHY		45

#define	SROM_AABREV		46
/* Fields in AABREV */
#define	SROM_BR_MASK		0x00ff
#define	SROM_CC_MASK		0x0f00
#define	SROM_CC_SHIFT		8
#define	SROM_AA0_MASK		0x3000
#define	SROM_AA0_SHIFT		12
#define	SROM_AA1_MASK		0xc000
#define	SROM_AA1_SHIFT		14

#define	SROM_WL0PAB0		47
#define	SROM_WL0PAB1		48
#define	SROM_WL0PAB2		49

#define	SROM_LEDBH10		50
#define	SROM_LEDBH32		51

#define	SROM_WL10MAXP		52

#define	SROM_WL1PAB0		53
#define	SROM_WL1PAB1		54
#define	SROM_WL1PAB2		55

#define	SROM_ITT		56

#define	SROM_BFL		57
#define	SROM_BFL2		28
#define	SROM3_BFL2		61

#define	SROM_AG10		58

#define	SROM_CCODE		59

#define	SROM_OPO		60

#define	SROM3_LEDDC		62

#define	SROM_CRCREV		63

/* SROM Rev 4: Reallocate the software part of the srom to accomodate
 * MIMO features. It assumes up to two PCIE functions and 440 bytes
 * of useable srom i.e. the useable storage in chips with OTP that
 * implements hardware redundancy.
 */

#define	SROM4_WORDS		220

#define	SROM4_SIGN		32
#define	SROM4_SIGNATURE		0x5372

#define	SROM4_BREV		33

#define	SROM4_BFL0		34
#define	SROM4_BFL1		35
#define	SROM4_BFL2		36
#define	SROM4_BFL3		37
#define	SROM5_BFL0		37
#define	SROM5_BFL1		38
#define	SROM5_BFL2		39
#define	SROM5_BFL3		40

#define	SROM4_MACHI		38
#define	SROM4_MACMID		39
#define	SROM4_MACLO		40
#define	SROM5_MACHI		41
#define	SROM5_MACMID		42
#define	SROM5_MACLO		43

#define	SROM4_CCODE		41
#define	SROM4_REGREV		42
#define	SROM5_CCODE		34
#define	SROM5_REGREV		35

#define	SROM4_LEDBH10		43
#define	SROM4_LEDBH32		44
#define	SROM5_LEDBH10		59
#define	SROM5_LEDBH32		60

#define	SROM4_LEDDC		45
#define	SROM5_LEDDC		45

#define	SROM4_AA		46
#define	SROM4_AA2G_MASK		0x00ff
#define	SROM4_AA2G_SHIFT	0
#define	SROM4_AA5G_MASK		0xff00
#define	SROM4_AA5G_SHIFT	8

#define	SROM4_AG10		47
#define	SROM4_AG32		48

#define	SROM4_TXPID2G		49
#define	SROM4_TXPID5G		51
#define	SROM4_TXPID5GL		53
#define	SROM4_TXPID5GH		55

#define SROM4_TXRXC		61
#define SROM4_TXCHAIN_MASK	0x000f
#define SROM4_TXCHAIN_SHIFT	0
#define SROM4_RXCHAIN_MASK	0x00f0
#define SROM4_RXCHAIN_SHIFT	4
#define SROM4_SWITCH_MASK	0xff00
#define SROM4_SWITCH_SHIFT	8


/* Per-path fields */
#define	MAX_PATH_SROM		4
#define	SROM4_PATH0		64
#define	SROM4_PATH1		87
#define	SROM4_PATH2		110
#define	SROM4_PATH3		133

#define	SROM4_2G_ITT_MAXP	0
#define	SROM4_2G_PA		1
#define	SROM4_5G_ITT_MAXP	5
#define	SROM4_5GLH_MAXP		6
#define	SROM4_5G_PA		7
#define	SROM4_5GL_PA		11
#define	SROM4_5GH_PA		15

/* Fields in the ITT_MAXP and 5GLH_MAXP words */
#define	B2G_MAXP_MASK		0xff
#define	B2G_ITT_SHIFT		8
#define	B5G_MAXP_MASK		0xff
#define	B5G_ITT_SHIFT		8
#define	B5GH_MAXP_MASK		0xff
#define	B5GL_MAXP_SHIFT		8

/* All the miriad power offsets */
#define	SROM4_2G_CCKPO		156
#define	SROM4_2G_OFDMPO		157
#define	SROM4_5G_OFDMPO		159
#define	SROM4_5GL_OFDMPO	161
#define	SROM4_5GH_OFDMPO	163
#define	SROM4_2G_MCSPO		165
#define	SROM4_5G_MCSPO		173
#define	SROM4_5GL_MCSPO		181
#define	SROM4_5GH_MCSPO		189
#define	SROM4_CDDPO		197
#define	SROM4_STBCPO		198
#define	SROM4_BW40PO		199
#define	SROM4_BWDUPPO		200

#define	SROM4_CRCREV		219


/* SROM Rev 8: Make space for a 48word hardware header for PCIe rev >= 6.
 * This is acombined srom for both MIMO and SISO boards, usable in
 * the .130 4Kilobit OTP with hardware redundancy.
 */

#define	SROM8_SIGN		64

#define	SROM8_BREV		65

#define	SROM8_BFL0		66
#define	SROM8_BFL1		67
#define	SROM8_BFL2		68
#define	SROM8_BFL3		69

#define	SROM8_MACHI		70
#define	SROM8_MACMID		71
#define	SROM8_MACLO		72

#define	SROM8_CCODE		73
#define	SROM8_REGREV		74

#define	SROM8_LEDBH10		75
#define	SROM8_LEDBH32		76

#define	SROM8_LEDDC		77

#define	SROM8_AA		78

#define	SROM8_AG10		79
#define	SROM8_AG32		80

#define	SROM8_TXRXC		81

#define	SROM8_BXARSSI2G		82
#define	SROM8_BXARSSI5G		83
#define	SROM8_TRI52G		84
#define	SROM8_TRI5GHL		85
#define	SROM8_RXPO52G		86

#define SROM8_FEM2G		87
#define SROM8_FEM5G		88
#define SROM8_FEM_ANTSWLUT_MASK		0xf800
#define SROM8_FEM_ANTSWLUT_SHIFT	11
#define SROM8_FEM_TR_ISO_MASK		0x0700
#define SROM8_FEM_TR_ISO_SHIFT		8
#define SROM8_FEM_PDET_RANGE_MASK	0x00f8
#define SROM8_FEM_PDET_RANGE_SHIFT	3
#define SROM8_FEM_EXTPA_GAIN_MASK	0x0006
#define SROM8_FEM_EXTPA_GAIN_SHIFT	1
#define SROM8_FEM_TSSIPOS_MASK		0x0001
#define SROM8_FEM_TSSIPOS_SHIFT		0

#define SROM8_THERMAL		89

/* Temp sense related entries */
#define SROM8_MPWR_RAWTS		90
#define SROM8_TS_SLP_OPT_CORRX	91
/* FOC: freiquency offset correction, HWIQ: H/W IOCAL enable, IQSWP: IQ CAL swap disable */
#define SROM8_FOC_HWIQ_IQSWP	92

#define SROM8_EXTLNAGAIN        93

/* Temperature delta for PHY calibration */
#define SROM8_PHYCAL_TEMPDELTA	94

/* Measured power 1 & 2, 0-13 bits at offset 95, MSB 2 bits are unused for now. */
#define SROM8_MPWR_1_AND_2	95


/* Per-path offsets & fields */
#define	SROM8_PATH0		96
#define	SROM8_PATH1		112
#define	SROM8_PATH2		128
#define	SROM8_PATH3		144

#define	SROM8_2G_ITT_MAXP	0
#define	SROM8_2G_PA		1
#define	SROM8_5G_ITT_MAXP	4
#define	SROM8_5GLH_MAXP		5
#define	SROM8_5G_PA		6
#define	SROM8_5GL_PA		9
#define	SROM8_5GH_PA		12

/* All the miriad power offsets */
#define	SROM8_2G_CCKPO		160

#define	SROM8_2G_OFDMPO		161
#define	SROM8_5G_OFDMPO		163
#define	SROM8_5GL_OFDMPO	165
#define	SROM8_5GH_OFDMPO	167

#define	SROM8_2G_MCSPO		169
#define	SROM8_5G_MCSPO		177
#define	SROM8_5GL_MCSPO		185
#define	SROM8_5GH_MCSPO		193

#define	SROM8_CDDPO		201
#define	SROM8_STBCPO		202
#define	SROM8_BW40PO		203
#define	SROM8_BWDUPPO		204

/* SISO PA parameters are in the path0 spaces */
#define	SROM8_SISO		96

/* Legacy names for SISO PA paramters */
#define	SROM8_W0_ITTMAXP	(SROM8_SISO + SROM8_2G_ITT_MAXP)
#define	SROM8_W0_PAB0		(SROM8_SISO + SROM8_2G_PA)
#define	SROM8_W0_PAB1		(SROM8_SISO + SROM8_2G_PA + 1)
#define	SROM8_W0_PAB2		(SROM8_SISO + SROM8_2G_PA + 2)
#define	SROM8_W1_ITTMAXP	(SROM8_SISO + SROM8_5G_ITT_MAXP)
#define	SROM8_W1_MAXP_LCHC	(SROM8_SISO + SROM8_5GLH_MAXP)
#define	SROM8_W1_PAB0		(SROM8_SISO + SROM8_5G_PA)
#define	SROM8_W1_PAB1		(SROM8_SISO + SROM8_5G_PA + 1)
#define	SROM8_W1_PAB2		(SROM8_SISO + SROM8_5G_PA + 2)
#define	SROM8_W1_PAB0_LC	(SROM8_SISO + SROM8_5GL_PA)
#define	SROM8_W1_PAB1_LC	(SROM8_SISO + SROM8_5GL_PA + 1)
#define	SROM8_W1_PAB2_LC	(SROM8_SISO + SROM8_5GL_PA + 2)
#define	SROM8_W1_PAB0_HC	(SROM8_SISO + SROM8_5GH_PA)
#define	SROM8_W1_PAB1_HC	(SROM8_SISO + SROM8_5GH_PA + 1)
#define	SROM8_W1_PAB2_HC	(SROM8_SISO + SROM8_5GH_PA + 2)

#define	SROM8_CRCREV		219

/* SROM REV 9 */
#define SROM9_2GPO_CCKBW20	160
#define SROM9_2GPO_CCKBW20UL	161
#define SROM9_2GPO_LOFDMBW20	162
#define SROM9_2GPO_LOFDMBW20UL	164

#define SROM9_5GLPO_LOFDMBW20	166
#define SROM9_5GLPO_LOFDMBW20UL	168
#define SROM9_5GMPO_LOFDMBW20	170
#define SROM9_5GMPO_LOFDMBW20UL	172
#define SROM9_5GHPO_LOFDMBW20	174
#define SROM9_5GHPO_LOFDMBW20UL	176

#define SROM9_2GPO_MCSBW20	178
#define SROM9_2GPO_MCSBW20UL	180
#define SROM9_2GPO_MCSBW40	182

#define SROM9_5GLPO_MCSBW20	184
#define SROM9_5GLPO_MCSBW20UL	186
#define SROM9_5GLPO_MCSBW40	188
#define SROM9_5GMPO_MCSBW20	190
#define SROM9_5GMPO_MCSBW20UL	192
#define SROM9_5GMPO_MCSBW40	194
#define SROM9_5GHPO_MCSBW20	196
#define SROM9_5GHPO_MCSBW20UL	198
#define SROM9_5GHPO_MCSBW40	200

#define SROM9_PO_MCS32		202
#define SROM9_PO_LOFDM40DUP	203
#define SROM8_RXGAINERR_2G	205
#define SROM8_RXGAINERR_5GL	206
#define SROM8_RXGAINERR_5GM	207
#define SROM8_RXGAINERR_5GH	208
#define SROM8_RXGAINERR_5GU	209
#define SROM8_SUBBAND_PPR	210
#define SROM8_PCIEINGRESS_WAR	211
#define SROM9_SAR		212

#define SROM8_NOISELVL_2G	213
#define SROM8_NOISELVL_5GL	214
#define SROM8_NOISELVL_5GM	215
#define SROM8_NOISELVL_5GH	216
#define SROM8_NOISELVL_5GU	217
#define SROM8_NOISECALOFFSET	218

#define SROM9_REV_CRC		219

#define SROM10_CCKPWROFFSET	218
#define SROM10_SIGN		219
#define SROM10_SWCTRLMAP_2G	220
#define SROM10_CRCREV		229

#define	SROM10_WORDS		230
#define	SROM10_SIGNATURE	SROM4_SIGNATURE


/* SROM REV 11 */
#define SROM11_BREV			65

#define SROM11_BFL0			66
#define SROM11_BFL1			67
#define SROM11_BFL2			68
#define SROM11_BFL3			69
#define SROM11_BFL4			70
#define SROM11_BFL5			71

#define SROM11_MACHI			72
#define SROM11_MACMID			73
#define SROM11_MACLO			74

#define SROM11_CCODE			75
#define SROM11_REGREV			76

#define SROM11_LEDBH10			77
#define SROM11_LEDBH32			78

#define SROM11_LEDDC			79

#define SROM11_AA			80

#define SROM11_AGBG10			81
#define SROM11_AGBG2A0			82
#define SROM11_AGA21			83

#define SROM11_TXRXC			84

#define SROM11_FEM_CFG1			85
#define SROM11_FEM_CFG2			86

/* Masks and offsets for FEM_CFG */
#define SROM11_FEMCTRL_MASK		0xf800
#define SROM11_FEMCTRL_SHIFT		11
#define SROM11_PAPDCAP_MASK		0x0400
#define SROM11_PAPDCAP_SHIFT		10
#define SROM11_TWORANGETSSI_MASK	0x0200
#define SROM11_TWORANGETSSI_SHIFT	9
#define SROM11_PDGAIN_MASK		0x01f0
#define SROM11_PDGAIN_SHIFT		4
#define SROM11_EPAGAIN_MASK		0x000e
#define SROM11_EPAGAIN_SHIFT		1
#define SROM11_TSSIPOSSLOPE_MASK	0x0001
#define SROM11_TSSIPOSSLOPE_SHIFT	0
#define SROM11_GAINCTRLSPH_MASK		0xf800
#define SROM11_GAINCTRLSPH_SHIFT	11

#define SROM11_THERMAL			87
#define SROM11_MPWR_RAWTS		88
#define SROM11_TS_SLP_OPT_CORRX		89
#define SROM11_XTAL_FREQ		90
#define SROM11_5GB0_4080_W0_A1          91
#define SROM11_PHYCAL_TEMPDELTA  	92
#define SROM11_MPWR_1_AND_2 		93
#define SROM11_5GB0_4080_W1_A1          94
#define SROM11_TSSIFLOOR_2G 		95
#define SROM11_TSSIFLOOR_5GL 		96
#define SROM11_TSSIFLOOR_5GM 		97
#define SROM11_TSSIFLOOR_5GH 		98
#define SROM11_TSSIFLOOR_5GU 		99

/* Masks and offsets for Terrmal parameters */
#define SROM11_TEMPS_PERIOD_MASK	0xf0
#define SROM11_TEMPS_PERIOD_SHIFT	4
#define SROM11_TEMPS_HYSTERESIS_MASK	0x0f
#define SROM11_TEMPS_HYSTERESIS_SHIFT	0
#define SROM11_TEMPCORRX_MASK		0xfc
#define SROM11_TEMPCORRX_SHIFT		2
#define SROM11_TEMPSENSE_OPTION_MASK	0x3
#define SROM11_TEMPSENSE_OPTION_SHIFT	0

#define SROM11_PDOFF_2G_40M_A0_MASK     0x000f
#define SROM11_PDOFF_2G_40M_A0_SHIFT    0
#define SROM11_PDOFF_2G_40M_A1_MASK     0x00f0
#define SROM11_PDOFF_2G_40M_A1_SHIFT    4
#define SROM11_PDOFF_2G_40M_A2_MASK     0x0f00
#define SROM11_PDOFF_2G_40M_A2_SHIFT    8
#define SROM11_PDOFF_2G_40M_VALID_MASK  0x8000
#define SROM11_PDOFF_2G_40M_VALID_SHIFT 15

#define SROM11_PDOFF_2G_40M     	100
#define SROM11_PDOFF_40M_A0		101
#define SROM11_PDOFF_40M_A1		102
#define SROM11_PDOFF_40M_A2		103
#define SROM11_5GB0_4080_W2_A1          103
#define SROM11_PDOFF_80M_A0		104
#define SROM11_PDOFF_80M_A1		105
#define SROM11_PDOFF_80M_A2		106
#define SROM11_5GB1_4080_W0_A1          106

#define SROM11_SUBBAND5GVER 		107

/* Per-path fields and offset */
#define	MAX_PATH_SROM_11		3
#define SROM11_PATH0			108
#define SROM11_PATH1			128
#define SROM11_PATH2			148

#define	SROM11_2G_MAXP			0
#define SROM11_5GB1_4080_PA             0
#define	SROM11_2G_PA			1
#define SROM11_5GB2_4080_PA             2
#define	SROM11_RXGAINS1			4
#define	SROM11_RXGAINS			5
#define SROM11_5GB3_4080_PA             5
#define	SROM11_5GB1B0_MAXP		6
#define	SROM11_5GB3B2_MAXP		7
#define	SROM11_5GB0_PA			8
#define	SROM11_5GB1_PA			11
#define	SROM11_5GB2_PA			14
#define	SROM11_5GB3_PA			17

/* Masks and offsets for rxgains */
#define SROM11_RXGAINS5GTRELNABYPA_MASK		0x8000
#define SROM11_RXGAINS5GTRELNABYPA_SHIFT	15
#define SROM11_RXGAINS5GTRISOA_MASK		0x7800
#define SROM11_RXGAINS5GTRISOA_SHIFT		11
#define SROM11_RXGAINS5GELNAGAINA_MASK		0x0700
#define SROM11_RXGAINS5GELNAGAINA_SHIFT		8
#define SROM11_RXGAINS2GTRELNABYPA_MASK		0x0080
#define SROM11_RXGAINS2GTRELNABYPA_SHIFT	7
#define SROM11_RXGAINS2GTRISOA_MASK		0x0078
#define SROM11_RXGAINS2GTRISOA_SHIFT		3
#define SROM11_RXGAINS2GELNAGAINA_MASK		0x0007
#define SROM11_RXGAINS2GELNAGAINA_SHIFT		0
#define SROM11_RXGAINS5GHTRELNABYPA_MASK	0x8000
#define SROM11_RXGAINS5GHTRELNABYPA_SHIFT	15
#define SROM11_RXGAINS5GHTRISOA_MASK		0x7800
#define SROM11_RXGAINS5GHTRISOA_SHIFT		11
#define SROM11_RXGAINS5GHELNAGAINA_MASK		0x0700
#define SROM11_RXGAINS5GHELNAGAINA_SHIFT	8
#define SROM11_RXGAINS5GMTRELNABYPA_MASK	0x0080
#define SROM11_RXGAINS5GMTRELNABYPA_SHIFT	7
#define SROM11_RXGAINS5GMTRISOA_MASK		0x0078
#define SROM11_RXGAINS5GMTRISOA_SHIFT		3
#define SROM11_RXGAINS5GMELNAGAINA_MASK		0x0007
#define SROM11_RXGAINS5GMELNAGAINA_SHIFT	0

/* Power per rate */
#define SROM11_CCKBW202GPO		168
#define SROM11_CCKBW20UL2GPO		169
#define SROM11_MCSBW202GPO		170
#define SROM11_MCSBW202GPO_1		171
#define SROM11_MCSBW402GPO		172
#define SROM11_MCSBW402GPO_1		173
#define SROM11_DOT11AGOFDMHRBW202GPO	174
#define SROM11_OFDMLRBW202GPO		175

#define SROM11_MCSBW205GLPO 		176
#define SROM11_MCSBW205GLPO_1		177
#define SROM11_MCSBW405GLPO 		178
#define SROM11_MCSBW405GLPO_1		179
#define SROM11_MCSBW805GLPO 		180
#define SROM11_MCSBW805GLPO_1		181
#define SROM11_RPCAL_2G			182
#define SROM11_RPCAL_5GL		183
#define SROM11_MCSBW205GMPO 		184
#define SROM11_MCSBW205GMPO_1		185
#define SROM11_MCSBW405GMPO 		186
#define SROM11_MCSBW405GMPO_1		187
#define SROM11_MCSBW805GMPO 		188
#define SROM11_MCSBW805GMPO_1		189
#define SROM11_RPCAL_5GM		190
#define SROM11_RPCAL_5GH		191
#define SROM11_MCSBW205GHPO 		192
#define SROM11_MCSBW205GHPO_1		193
#define SROM11_MCSBW405GHPO 		194
#define SROM11_MCSBW405GHPO_1		195
#define SROM11_MCSBW805GHPO 		196
#define SROM11_MCSBW805GHPO_1		197
#define SROM11_RPCAL_5GU		198
#define SROM11_PDOFF_2G_CCK	        199
#define SROM11_MCSLR5GLPO		200
#define SROM11_MCSLR5GMPO		201
#define SROM11_MCSLR5GHPO		202

#define SROM11_SB20IN40HRPO		203
#define SROM11_SB20IN80AND160HR5GLPO 	204
#define SROM11_SB40AND80HR5GLPO		205
#define SROM11_SB20IN80AND160HR5GMPO 	206
#define SROM11_SB40AND80HR5GMPO		207
#define SROM11_SB20IN80AND160HR5GHPO 	208
#define SROM11_SB40AND80HR5GHPO		209
#define SROM11_SB20IN40LRPO 		210
#define SROM11_SB20IN80AND160LR5GLPO	211
#define SROM11_SB40AND80LR5GLPO		212
#define SROM11_TXIDXCAP2G               212
#define SROM11_SB20IN80AND160LR5GMPO	213
#define SROM11_SB40AND80LR5GMPO		214
#define SROM11_TXIDXCAP5G               214
#define SROM11_SB20IN80AND160LR5GHPO	215
#define SROM11_SB40AND80LR5GHPO		216

#define SROM11_DOT11AGDUPHRPO 		217
#define SROM11_DOT11AGDUPLRPO		218

/* MISC */
#define SROM11_PCIEINGRESS_WAR		220
#define SROM11_SAR			221

#define SROM11_NOISELVL_2G		222
#define SROM11_NOISELVL_5GL 		223
#define SROM11_NOISELVL_5GM 		224
#define SROM11_NOISELVL_5GH 		225
#define SROM11_NOISELVL_5GU 		226

#define SROM11_RXGAINERR_2G		227
#define SROM11_RXGAINERR_5GL		228
#define SROM11_RXGAINERR_5GM		229
#define SROM11_RXGAINERR_5GH		230
#define SROM11_RXGAINERR_5GU		231

#define SROM11_SIGN 			64
#define SROM11_CRCREV 			233

#define	SROM11_WORDS			234
#define	SROM11_SIGNATURE		0x0634

typedef struct {
	uint8 tssipos;		/* TSSI positive slope, 1: positive, 0: negative */
	uint8 extpagain;	/* Ext PA gain-type: full-gain: 0, pa-lite: 1, no_pa: 2 */
	uint8 pdetrange;	/* support 32 combinations of different Pdet dynamic ranges */
	uint8 triso;		/* TR switch isolation */
	uint8 antswctrllut;	/* antswctrl lookup table configuration: 32 possible choices */
} srom_fem_t;

#endif	/* _bcmsrom_fmt_h_ */
