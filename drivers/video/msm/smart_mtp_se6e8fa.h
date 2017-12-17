/*
 * =================================================================
 *
 *       Filename:  smart_mtp_se6e8fa.h
 *
 *    Description:  Smart dimming algorithm implementation
 *
 *        Author: jb09.kim
 *        Company:  Samsung Electronics
 *
 * ================================================================
 */
/*
<one line to give the program's name and a brief idea of what it does.>
Copyright (C) 2012, Samsung Electronics. All rights reserved.

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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
*/
#ifndef _SMART_MTP_SE6E8FA_H_
#define _SMART_MTP_SE6E8FA_H_

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/ctype.h>
#include <asm/div64.h>

enum ldi_chip {
	LDI_MAGNA,
	LDI_LSI,
};

extern int get_ldi_chip(void);

/* EL METERIAL. ID2 BIT(0) & BIT(1) field*/
#define EL_METERIAL_MASK 0x03
#define M4 0x0
#define M4P 0x1
#define TULIP 0x2

/* LSI LDI ID */
#define VT232_ID 0x02
#define CCG6_ID 0x03
#define EVT1_ID 0x23
#define EVT1_FIRST_ID 0x24
#define EVT1_SECOND_ID 0x44

/* EVT1_THIRD_ID & EVT1_FOUTRH_ID has same smart-dimming algo */
#define EVT1_THIRD_ID 0x25
#define EVT1_FOUTRH_ID 0x45

/* for REV H LDI */
#define EVT1_REV_H_ID3_1 0x46
#define EVT1_REV_H_ID3_2 0x26

/* for REV I LDI */
#define EVT1_REV_I_ID3_1 0x47
#define EVT1_REV_I_ID3_2 0x27

/* MAGNA LDI ID*/
#define MAGNA_C_FIRST_ID 0x85
#define MAGNA_C_SECOND_ID 0x86


/*
*	From 4.8 inch model use AID function
*	CASE#1 is used for now.
*/
#define AID_OPERATION

#define GAMMA_CURVE_2P25 1
#define GAMMA_CURVE_2P2 2
#define GAMMA_CURVE_2P15 3
#define GAMMA_CURVE_2P1 4
#define GAMMA_CURVE_2P0 5
#define GAMMA_CURVE_1P9 6


#define MTP_START_ADDR 0xC8
#define LUMINANCE_MAX 54
#define GAMMA_SET_MAX 33
#define BIT_SHIFT 22
/*
	it means BIT_SHIFT is 22.  pow(2,BIT_SHIFT) is 4194304.
	BIT_SHIFT is used for right bit shfit
*/
#define BIT_SHFIT_MUL 4194304

#define S6E8FA_GRAY_SCALE_MAX 256

/*6.3*4194304 */
#define S6E8FA_VREG0_REF 26424115

/*V0,V1,V3,V11,V23,V35,V51,V87,V151,V203,V255*/
#define S6E8FA_MAX 11

/* PANEL DEPENDENT THINGS */
#define MAX_CANDELA 300
#define MIN_CANDELA	10

/*
*	ID 0x20
*/
#define V255_300CD_R_MSB_20 0x01
#define V255_300CD_R_LSB_20 0x00

#define V255_300CD_G_MSB_20 0x01
#define V255_300CD_G_LSB_20 0x00

#define V255_300CD_B_MSB_20 0x01
#define V255_300CD_B_LSB_20 0x00

#define V203_300CD_R_20 0x80
#define V203_300CD_G_20 0x80
#define V203_300CD_B_20 0x80

#define V151_300CD_R_20 0x80
#define V151_300CD_G_20 0x80
#define V151_300CD_B_20 0x80

#define V87_300CD_R_20 0x80
#define V87_300CD_G_20 0x80
#define V87_300CD_B_20 0x80

#define V51_300CD_R_20 0x80
#define V51_300CD_G_20 0x80
#define V51_300CD_B_20 0x80

#define V35_300CD_R_20 0x80
#define V35_300CD_G_20 0x80
#define V35_300CD_B_20 0x80

#define V23_300CD_R_20 0x80
#define V23_300CD_G_20 0x80
#define V23_300CD_B_20 0x80

#define V11_300CD_R_20 0x80
#define V11_300CD_G_20 0x80
#define V11_300CD_B_20 0x80

#define V3_300CD_R_20 0x80
#define V3_300CD_G_20 0x80
#define V3_300CD_B_20 0x80

#define VT_300CD_R_20 0x00
#define VT_300CD_G_20 0x00
#define VT_300CD_B_20 0x00


/* PANEL DEPENDENT THINGS END*/

enum {
	V1_INDEX = 0,
	V3_INDEX = 1,
	V11_INDEX = 2,
	V23_INDEX = 3,
	V35_INDEX = 4,
	V51_INDEX = 5,
	V87_INDEX = 6,
	V151_INDEX = 7,
	V203_INDEX = 8,
	V255_INDEX = 9,
};

struct gamma_level {
	int level_0;
	int level_1;
	int level_3;
	int level_11;
	int level_23;
	int level_35;
	int level_51;
	int level_87;
	int level_151;
	int level_203;
	int level_255;
} __packed;

struct rgb_output_voltage {
	struct gamma_level r_voltage;
	struct gamma_level g_voltage;
	struct gamma_level b_voltage;
} __packed;

struct gray_voltage {
	/*
		This voltage value use 14bit right shit
		it means voltage is divied by 16384.
	*/
	int r_gray;
	int g_gray;
	int b_gray;
} __packed;

struct gray_scale {
	struct gray_voltage table[S6E8FA_GRAY_SCALE_MAX];
	struct gray_voltage vt_table;
} __packed;

/*V0,V1,V3,V11,V23,V35,V51,V87,V151,V203,V255*/

struct mtp_set {
	char offset_255_msb;
	char offset_255_lsb;
	char offset_203;
	char offset_151;
	char offset_87;
	char offset_51;
	char offset_35;
	char offset_23;
	char offset_11;
	char offset_3;
	char offset_1;
} __packed;

struct hbm_reg {
	/* LSI */
	char c8_reg_1[7];	/*c8 34~40th reg*/
	char c8_reg_2[15];  /*c8 73~87th reg*/
	char b6_reg_lsi[1];		/*b6 17th reg*/
	/* MAGNA */
	char b1_reg[6];		/*b1 10~15th reg*/
	char b6_reg_magna[23];	/*b6 4~26th reg*/
} __packed;

struct mtp_offset {
	struct mtp_set r_offset;
	struct mtp_set g_offset;
	struct mtp_set b_offset;
} __packed;

struct illuminance_table {
	int lux;
	char gamma_setting[GAMMA_SET_MAX];
} __packed;

struct smart_dim {
	struct hbm_reg hbm_reg;
	struct mtp_offset mtp_origin;
	struct mtp_offset mtp;
	struct rgb_output_voltage rgb_output;
	struct gray_scale gray;

	/* Because of AID funtion, below members are added*/
	int lux_table_max;
	int *plux_table;
	struct illuminance_table gen_table[LUMINANCE_MAX];

	int brightness_level;
	int ldi_revision;
} __packed;

void generate_gamma(struct smart_dim *smart_dim, char *str, int size);
int smart_dimming_init(struct smart_dim *psmart);
void get_min_lux_table(char *str, int size);

#endif

