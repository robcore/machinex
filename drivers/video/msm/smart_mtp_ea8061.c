/*
 * =================================================================
 *
 *       Filename:  smart_mtp_ea8061.c
 *
 *    Description:  Smart dimming algorithm implementation
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

#include "smart_dimming_ea8061.h"
#include "mipi_samsung_define.h"
#include "mipi_samsung_oled.h"

//#include "smart_mtp_2p2_gamma.h"


//#define SMART_DIMMING_DEBUG


#define MAX_GAMMA			300

extern unsigned char SEQ_AOR_CONTROL[];
#define AID_PARAM_SIZE	3

static const unsigned int candela_table[GAMMA_MAX] = {
	 20,  30,  40,  50,  60,  70,  80,  90, 100,
	102, 104, 106, 108,
	110, 120, 130, 140, 150, 160, 170, 180,
	182, 184, 186, 188,
	190, 200, 210, 220, 230, 240, 250, MAX_GAMMA-1
};

#define aid_300nit_260nit_B3_1st	0x00
#define aid_300nit_260nit_B3_2nd	0x0A
#define aid_250nit_190nit_B3_1st	0x00
#define aid_250nit_190nit_B3_2nd	0x0A
#define aid_188nit_186nit_B3_1st	0x00
#define aid_184nit_182nit_B3_1st	0x01
#define aid_188nit_B3_2nd		0x68
#define aid_186nit_B3_2nd		0xCF
#define aid_184nit_B3_2nd		0x37
#define aid_182nit_B3_2nd		0x9F
#define aid_180nit_110nit_B3_1st	0x02
#define aid_180nit_110nit_B3_2nd	0x06
#define aid_102nit_B3_1st		0x00
#define aid_108nit_104nit_B3_1st	0x01
#define aid_108nit_B3_2nd		0xC4
#define aid_106nit_B3_2nd		0x82
#define aid_104nit_B3_2nd		0x40
#define aid_102nit_B3_2nd		0xFE
#define aid_100nit_B3_1st	0x00
#define aid_90nit_80nit_B3_1st	0x01
#define aid_70nit_60nit_B3_1st	0x02
#define aid_50nit_30nit_B3_1st	0x03
#define aid_20nit_B3_1st	0x04
#define aid_100nit_B3_2nd		0xBC
#define aid_90nit_B3_2nd		0x36
#define aid_80nit_B3_2nd		0xAB
#define aid_70nit_B3_2nd		0x25
#define aid_60nit_B3_2nd		0x95
#define aid_50nit_B3_2nd		0x0A
#define aid_40nit_B3_2nd		0x74
#define aid_30nit_B3_2nd		0xDF
#define aid_20nit_B3_2nd		0x45
#define AOR40_BASE_188		202
#define AOR40_BASE_186		215
#define AOR40_BASE_184		230
#define AOR40_BASE_182		250
#define AOR40_BASE_180		275
#define AOR40_BASE_170		260
#define AOR40_BASE_160		246
#define AOR40_BASE_150		230
#define AOR40_BASE_140		217
#define AOR40_BASE_130		202
#define AOR40_BASE_120		184
#define AOR40_BASE_110		169
#define AOR40_BASE_108		154
#define AOR40_BASE_106		141
#define AOR40_BASE_104		130
#define AOR40_BASE_102		120
#define base_20to100			110

static const struct rgb_offset_info aid_rgb_fix_table[] = {
	{GAMMA_184CD, IV_11, CI_BLUE, 1},
	{GAMMA_182CD, IV_11, CI_GREEN, -1}, {GAMMA_182CD, IV_11, CI_BLUE, 2},
	{GAMMA_180CD, IV_11, CI_RED, -1}, {GAMMA_180CD, IV_11, CI_GREEN, -2}, {GAMMA_180CD, IV_11, CI_BLUE, 3},
	{GAMMA_170CD, IV_11, CI_RED, -1}, {GAMMA_170CD, IV_11, CI_GREEN, -2}, {GAMMA_170CD, IV_11, CI_BLUE, 3},
	{GAMMA_160CD, IV_11, CI_RED, -1}, {GAMMA_160CD, IV_11, CI_GREEN, -2}, {GAMMA_160CD, IV_11, CI_BLUE, 3},
	{GAMMA_150CD, IV_11, CI_RED, -1}, {GAMMA_150CD, IV_11, CI_GREEN, -2}, {GAMMA_150CD, IV_11, CI_BLUE, 3},
	{GAMMA_140CD, IV_11, CI_RED, -1}, {GAMMA_140CD, IV_11, CI_GREEN, -2}, {GAMMA_140CD, IV_11, CI_BLUE, 3},
	{GAMMA_130CD, IV_11, CI_RED, -1}, {GAMMA_130CD, IV_11, CI_GREEN, -2}, {GAMMA_130CD, IV_11, CI_BLUE, 3},
	{GAMMA_120CD, IV_11, CI_RED, -1}, {GAMMA_120CD, IV_11, CI_GREEN, -2}, {GAMMA_120CD, IV_11, CI_BLUE, 3},
	{GAMMA_110CD, IV_11, CI_RED, -1}, {GAMMA_110CD, IV_11, CI_GREEN, -2}, {GAMMA_110CD, IV_11, CI_BLUE, 3},
	{GAMMA_108CD, IV_11, CI_RED, -1}, {GAMMA_108CD, IV_11, CI_GREEN, -2}, {GAMMA_108CD, IV_11, CI_BLUE, 3},
	{GAMMA_106CD, IV_11, CI_RED, -1}, {GAMMA_106CD, IV_11, CI_GREEN, -1}, {GAMMA_106CD, IV_11, CI_BLUE, 3},
	{GAMMA_104CD, IV_11, CI_RED, -2}, {GAMMA_104CD, IV_11, CI_GREEN, -1}, {GAMMA_104CD, IV_11, CI_BLUE, 4},
	{GAMMA_102CD, IV_11, CI_RED, -2}, {GAMMA_102CD, IV_11, CI_BLUE, 4},
	{GAMMA_100CD, IV_11, CI_RED, -2}, {GAMMA_100CD, IV_11, CI_BLUE, 5},
	{GAMMA_90CD, IV_11, CI_RED, -5},  {GAMMA_90CD, IV_11, CI_BLUE, 6},
	{GAMMA_80CD, IV_11, CI_RED, -6},  {GAMMA_80CD, IV_11, CI_BLUE, 8},
	{GAMMA_70CD, IV_11, CI_RED, -7},	{GAMMA_70CD, IV_11, CI_BLUE, 11},
	{GAMMA_60CD, IV_11, CI_RED, -10}, {GAMMA_60CD, IV_11, CI_BLUE, 14},
	{GAMMA_50CD, IV_11, CI_RED, -12}, {GAMMA_50CD, IV_11, CI_BLUE, 19},
	{GAMMA_40CD, IV_11, CI_RED, -18}, {GAMMA_40CD, IV_11, CI_BLUE, 24},
	{GAMMA_30CD, IV_11, CI_RED, -18}, {GAMMA_30CD, IV_11, CI_BLUE, 31},
	{GAMMA_20CD, IV_11, CI_RED, -18}, {GAMMA_20CD, IV_11, CI_BLUE, 39},
	{GAMMA_90CD, IV_23, CI_GREEN, -3},
	{GAMMA_80CD, IV_23, CI_RED, -1}, {GAMMA_80CD, IV_23, CI_GREEN, -4},
	{GAMMA_70CD, IV_23, CI_RED, -3}, {GAMMA_70CD, IV_23, CI_GREEN, -6},
	{GAMMA_60CD, IV_23, CI_RED, -4}, {GAMMA_60CD, IV_23, CI_GREEN, -9},
	{GAMMA_50CD, IV_23, CI_RED, -7}, {GAMMA_50CD, IV_23, CI_GREEN, -9},
	{GAMMA_40CD, IV_23, CI_RED, -12}, {GAMMA_40CD, IV_23, CI_GREEN, -16},
	{GAMMA_30CD, IV_23, CI_RED, -17}, {GAMMA_30CD, IV_23, CI_GREEN, -16}, {GAMMA_30CD, IV_23, CI_BLUE, 2},
	{GAMMA_20CD, IV_23, CI_RED, -22}, {GAMMA_20CD, IV_23, CI_GREEN, -16}, {GAMMA_20CD, IV_23, CI_BLUE, 9},
	{GAMMA_30CD, IV_35, CI_RED, -3}, {GAMMA_30CD, IV_35, CI_GREEN, -14},
	{GAMMA_20CD, IV_35, CI_RED, -11}, {GAMMA_20CD, IV_35, CI_GREEN, -30},
};

static unsigned char aid_command_20[] = {
	aid_20nit_B3_1st,
	aid_20nit_B3_2nd,
};

static unsigned char aid_command_30[] = {
	aid_50nit_30nit_B3_1st,
	aid_30nit_B3_2nd,
};

static unsigned char aid_command_40[] = {
	aid_50nit_30nit_B3_1st,
	aid_40nit_B3_2nd,
};

static unsigned char aid_command_50[] = {
	aid_50nit_30nit_B3_1st,
	aid_50nit_B3_2nd,
};

static unsigned char aid_command_60[] = {
	aid_70nit_60nit_B3_1st,
	aid_60nit_B3_2nd,
};

static unsigned char aid_command_70[] = {
	aid_70nit_60nit_B3_1st,
	aid_70nit_B3_2nd,
};

static unsigned char aid_command_80[] = {
	aid_90nit_80nit_B3_1st,
	aid_80nit_B3_2nd,
};

static unsigned char aid_command_90[] = {
	aid_90nit_80nit_B3_1st,
	aid_90nit_B3_2nd,
};

static unsigned char aid_command_100[] = {
	aid_100nit_B3_1st,
	aid_100nit_B3_2nd,
};

static unsigned char aid_command_102[] = {
	aid_102nit_B3_1st,
	aid_102nit_B3_2nd,
};

static unsigned char aid_command_104[] = {
	aid_108nit_104nit_B3_1st,
	aid_104nit_B3_2nd,
};

static unsigned char aid_command_106[] = {
	aid_108nit_104nit_B3_1st,
	aid_106nit_B3_2nd,
};

static unsigned char aid_command_108[] = {
	aid_108nit_104nit_B3_1st,
	aid_108nit_B3_2nd,
};

static unsigned char aid_command_110[] = {
	aid_180nit_110nit_B3_1st,
	aid_180nit_110nit_B3_2nd,
};

static unsigned char aid_command_120[] = {
	aid_180nit_110nit_B3_1st,
	aid_180nit_110nit_B3_2nd,
};

static unsigned char aid_command_130[] = {
	aid_180nit_110nit_B3_1st,
	aid_180nit_110nit_B3_2nd,
};

static unsigned char aid_command_140[] = {
	aid_180nit_110nit_B3_1st,
	aid_180nit_110nit_B3_2nd,
};

static unsigned char aid_command_150[] = {
	aid_180nit_110nit_B3_1st,
	aid_180nit_110nit_B3_2nd,
};

static unsigned char aid_command_160[] = {
	aid_180nit_110nit_B3_1st,
	aid_180nit_110nit_B3_2nd,
};

static unsigned char aid_command_170[] = {
	aid_180nit_110nit_B3_1st,
	aid_180nit_110nit_B3_2nd,
};

static unsigned char aid_command_180[] = {
	aid_180nit_110nit_B3_1st,
	aid_180nit_110nit_B3_2nd,
};

static unsigned char aid_command_182[] = {
	aid_184nit_182nit_B3_1st,
	aid_182nit_B3_2nd,
};

static unsigned char aid_command_184[] = {
	aid_184nit_182nit_B3_1st,
	aid_184nit_B3_2nd,
};

static unsigned char aid_command_186[] = {
	aid_188nit_186nit_B3_1st,
	aid_186nit_B3_2nd,
};

static unsigned char aid_command_188[] = {
	aid_188nit_186nit_B3_1st,
	aid_188nit_B3_2nd,
};

static unsigned char aid_command_190[] = {
	aid_250nit_190nit_B3_1st,
	aid_250nit_190nit_B3_2nd,
};

static unsigned char aid_command_200[] = {
	aid_250nit_190nit_B3_1st,
	aid_250nit_190nit_B3_2nd,
};

static unsigned char aid_command_210[] = {
	aid_250nit_190nit_B3_1st,
	aid_250nit_190nit_B3_2nd,
};

static unsigned char aid_command_220[] = {
	aid_250nit_190nit_B3_1st,
	aid_250nit_190nit_B3_2nd,
};

static unsigned char aid_command_230[] = {
	aid_250nit_190nit_B3_1st,
	aid_250nit_190nit_B3_2nd,
};

static unsigned char aid_command_240[] = {
	aid_250nit_190nit_B3_1st,
	aid_250nit_190nit_B3_2nd,
};

static unsigned char aid_command_250[] = {
	aid_250nit_190nit_B3_1st,
	aid_250nit_190nit_B3_2nd,
};

static unsigned char aid_command_300[] = {
	aid_300nit_260nit_B3_1st,
	aid_300nit_260nit_B3_2nd,

};

static unsigned int aid_candela_table[GAMMA_MAX] = {
	base_20to100, base_20to100, base_20to100, base_20to100, base_20to100, base_20to100, base_20to100, base_20to100, base_20to100,
	AOR40_BASE_102, AOR40_BASE_104, AOR40_BASE_106, AOR40_BASE_108,
	AOR40_BASE_110, AOR40_BASE_120, AOR40_BASE_130, AOR40_BASE_140, AOR40_BASE_150,
	AOR40_BASE_160, AOR40_BASE_170, AOR40_BASE_180, AOR40_BASE_182, AOR40_BASE_184,
	AOR40_BASE_186, AOR40_BASE_188,
	190, 200, 210, 220, 230, 240, 250, MAX_GAMMA-1
};

static const unsigned char SEQ_LTPS_AID[] = {
	0xB3,
	0x00, 0x0A,
};

static unsigned char *aid_command_table[GAMMA_MAX] = {
	aid_command_20,
	aid_command_30,
	aid_command_40,
	aid_command_50,
	aid_command_60,
	aid_command_70,
	aid_command_80,
	aid_command_90,
	aid_command_100,
	aid_command_102,
	aid_command_104,
	aid_command_106,
	aid_command_108,
	aid_command_110,
	aid_command_120,
	aid_command_130,
	aid_command_140,
	aid_command_150,
	aid_command_160,
	aid_command_170,
	aid_command_180,
	aid_command_182,
	aid_command_184,
	aid_command_186,
	aid_command_188,
	aid_command_190,
	aid_command_200,
	aid_command_210,
	aid_command_220,
	aid_command_230,
	aid_command_240,
	aid_command_250,
	aid_command_300
};

int init_gamma_table_ea8061(struct mipi_panel_data *pmpd)
{
	int i;
	struct SMART_DIM *psmart = &(pmpd->smart_s6e8aa0x01);
	struct str_smart_dim *smart = &(pmpd->smart);
	u8* mtp_offset = pmpd->lcd_mtp_data;
	
	for (i = 0; i < GAMMA_MAX; i++) {
		psmart->gen_table[i].gamma_setting[0] = 0xCA;
	}

	for (i = 0; i < GAMMA_MAX; i++) {
		if (candela_table[i] == 20)
			calc_gamma_table_20_100_ea8061(smart, aid_candela_table[i], &psmart->gen_table[i].gamma_setting[1], G_21, mtp_offset);
		else if (candela_table[i] == 30)
			calc_gamma_table_20_100_ea8061(smart, aid_candela_table[i], &psmart->gen_table[i].gamma_setting[1], G_213, mtp_offset);
		else if (candela_table[i] == 40)
			calc_gamma_table_20_100_ea8061(smart, aid_candela_table[i], &psmart->gen_table[i].gamma_setting[1], G_215, mtp_offset);
		else if (candela_table[i] == 50)
			calc_gamma_table_20_100_ea8061(smart, aid_candela_table[i], &psmart->gen_table[i].gamma_setting[1], G_218, mtp_offset);
		else if (candela_table[i] == 60)
			calc_gamma_table_20_100_ea8061(smart, aid_candela_table[i], &psmart->gen_table[i].gamma_setting[1], G_22, mtp_offset);
		else if (candela_table[i] == 70)
			calc_gamma_table_20_100_ea8061(smart, aid_candela_table[i], &psmart->gen_table[i].gamma_setting[1], G_222, mtp_offset);
		else if (candela_table[i] == 80)
			calc_gamma_table_20_100_ea8061(smart, aid_candela_table[i], &psmart->gen_table[i].gamma_setting[1], G_223, mtp_offset);
		else if (candela_table[i] == 90)
			calc_gamma_table_20_100_ea8061(smart, aid_candela_table[i], &psmart->gen_table[i].gamma_setting[1], G_224, mtp_offset);
		else if (candela_table[i] == 100)
			calc_gamma_table_20_100_ea8061(smart, aid_candela_table[i], &psmart->gen_table[i].gamma_setting[1], G_225, mtp_offset);
		else if (candela_table[i] == 102)
			calc_gamma_table_ea8061(smart, aid_candela_table[i], &psmart->gen_table[i].gamma_setting[1], G_224, mtp_offset);
		else if (candela_table[i] == 104)
			calc_gamma_table_ea8061(smart, aid_candela_table[i], &psmart->gen_table[i].gamma_setting[1], G_223, mtp_offset);
		else if (candela_table[i] == 106)
			calc_gamma_table_ea8061(smart, aid_candela_table[i], &psmart->gen_table[i].gamma_setting[1], G_222, mtp_offset);
		else if (candela_table[i] == 108)
			calc_gamma_table_ea8061(smart, aid_candela_table[i], &psmart->gen_table[i].gamma_setting[1], G_221, mtp_offset);
		else if (candela_table[i] == 182)
			calc_gamma_table_ea8061(smart, aid_candela_table[i], &psmart->gen_table[i].gamma_setting[1], G_221, mtp_offset);
		else if (candela_table[i] == 184)
			calc_gamma_table_ea8061(smart, aid_candela_table[i], &psmart->gen_table[i].gamma_setting[1], G_222, mtp_offset);
		else if (candela_table[i] == 186)
			calc_gamma_table_ea8061(smart, aid_candela_table[i], &psmart->gen_table[i].gamma_setting[1], G_223, mtp_offset);
		else if (candela_table[i] == 188)
			calc_gamma_table_ea8061(smart, aid_candela_table[i], &psmart->gen_table[i].gamma_setting[1], G_224, mtp_offset);
		else if (candela_table[i] == 190)
			calc_gamma_table_190_ea8061(smart, aid_candela_table[i], &psmart->gen_table[i].gamma_setting[1], G_225, mtp_offset);
//		else if ((candela_table[i] > 190) && (candela_table[i] < MAX_GAMMA-1))
//			calc_gamma_table_ea8061(smart, aid_candela_table[i], &psmart->gen_table[i].gamma_setting[1], G_22, mtp_offset);
		else
			calc_gamma_table_ea8061(smart, aid_candela_table[i], &psmart->gen_table[i].gamma_setting[1], G_22, mtp_offset);
	}


#ifdef SMART_DIMMING_DEBUG
{
	int j;
	for (i = 0; i < GAMMA_MAX; i++) {
		for (j = 0; j < GAMMA_PARAM_SIZE; j++)
			printk("%d,", psmart->gen_table[i].gamma_setting[j]);
		printk("\n");
	}
}
#endif
	return 0;
}

int init_aid_dimming_table_ea8061(struct mipi_panel_data *pmpd)
{
	unsigned int i, j, c;
	u16 reverse_seq[] = {0, 28, 29, 30, 31, 32, 33, 25, 26, 27, 22, 23, 24, 19, 20, 21, 16, 17, 18, 13, 14, 15, 10, 11, 12, 7, 8, 9, 4, 5, 6, 1, 2, 3};
	u16 temp[GAMMA_PARAM_SIZE];
	struct SMART_DIM *psmart = &(pmpd->smart_s6e8aa0x01);

	for (i = 0; i < ARRAY_SIZE(aid_rgb_fix_table); i++) {
		j = (aid_rgb_fix_table[i].gray * 3 + aid_rgb_fix_table[i].rgb) + 1;
		c = psmart->gen_table[aid_rgb_fix_table[i].candela_idx].gamma_setting[j] + aid_rgb_fix_table[i].offset;
		if (c > 0xff)
			psmart->gen_table[aid_rgb_fix_table[i].candela_idx].gamma_setting[j] = 0xff;
		else
			psmart->gen_table[aid_rgb_fix_table[i].candela_idx].gamma_setting[j] += aid_rgb_fix_table[i].offset;
	}

	for (i = 0; i < GAMMA_MAX; i++) {
		memcpy(psmart->gen_table[i].b3, SEQ_LTPS_AID, AID_PARAM_SIZE);
		psmart->gen_table[i].b3[0x02] = aid_command_table[i][1];
		psmart->gen_table[i].b3[0x01] = aid_command_table[i][0];
	}

#ifdef SMART_DIMMING_DEBUG
	for (i = 0; i < GAMMA_MAX; i++) {
		for (j = 0; j < GAMMA_PARAM_SIZE; j++)
			printk("%d,", psmart->gen_table[i].gamma_setting[j]);
		printk("\n");
	}
	printk("\n");
#endif

	for (i = 0; i < GAMMA_MAX; i++) {
		for (j = 0; j < GAMMA_PARAM_SIZE; j++)
			temp[j] = psmart->gen_table[i].gamma_setting[reverse_seq[j]];

		for (j = 0; j < GAMMA_PARAM_SIZE; j++)
			psmart->gen_table[i].gamma_setting[j] = temp[j];

		psmart->gen_table[i].gamma_setting[31] = pmpd->smart.default_gamma[31]<<4|pmpd->smart.default_gamma[30];
		/*default_gamma 30,31th range 0000~1111 */
		/*31,30th gamma need to reverse order , because of normal order is R,G,B
		   but Magna DDI VT G,R,B so R,G order change to G,R order*/
		psmart->gen_table[i].gamma_setting[32] = pmpd->smart.default_gamma[32];
	}

#ifdef SMART_DIMMING_DEBUG
	for (i = 0; i < GAMMA_MAX; i++) {
		for (j = 0; j < GAMMA_PARAM_SIZE; j++)
			printk("%d,", psmart->gen_table[i].gamma_setting[j]);
		printk("\n");
	}
#endif

	return 0;
}


static void ea8061_read_mtp(struct mipi_samsung_driver_data *pmsd, char srcReg, int srcLength, char *destBuffer,
		const int isUseMutex, struct msm_fb_data_type *pMFD)
{
	const int one_read_size = 4;
	const int loop_limit = 16;
	/* first byte = read-register */
	static char read_reg[2] = { 0xFE, 0x00 };
	static struct dsi_cmd_desc ea8061_read_reg_cmd = {
		DTYPE_DCS_READ, 1, 0, 1, 5, sizeof(read_reg), read_reg };
	/* first byte is size of Register */
	static char packet_size[] = { 0x04, 0 };
	static struct dsi_cmd_desc ea8061_packet_size_cmd = {
	DTYPE_MAX_PKTSIZE, 1, 0, 0, 0, sizeof(packet_size), packet_size };

	/* second byte is Read-position */
	static char reg_read_pos[] = { 0xB0, 0x00 };
	static struct dsi_cmd_desc ea8061_read_pos_cmd = {
		DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(reg_read_pos),
		reg_read_pos };

	static char reg_pre_set[] = {  0xFD, 0xDA };
	static struct dsi_cmd_desc preset_cmd = {
		DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(reg_pre_set),
		reg_pre_set };

	int read_pos;
	int readed_size;
	int show_cnt;

	int i, j;
	char show_buffer[256];
	int show_buffer_pos = 0;

	reg_pre_set[1] = srcReg;

	mipi_dsi_buf_init(&pmsd->samsung_tx_buf);
	mipi_dsi_cmds_tx(&pmsd->samsung_tx_buf, &(preset_cmd), 1);
	
	msleep(20);

	show_buffer_pos +=
		snprintf(show_buffer, 256, "read_reg : %X[%d] : ",
				srcReg, srcLength);

	read_pos = 0;
	readed_size = 0;

//	packet_size[0] = (char)srcLength;
	mipi_dsi_buf_init(&pmsd->samsung_tx_buf);
	mipi_dsi_cmds_tx(&pmsd->samsung_tx_buf, &(ea8061_packet_size_cmd), 1);

	show_cnt = 0;
	for (j = 0; j < loop_limit; j++) {
		reg_read_pos[1] = read_pos;
		if (mipi_dsi_cmds_tx
			(&pmsd->samsung_tx_buf, &(ea8061_read_pos_cmd),
				 1) < 1) {
			show_buffer_pos +=
			snprintf(show_buffer + show_buffer_pos, 256,
					"Tx command FAILED");
			break;
		}
		mipi_dsi_buf_init(&pmsd->samsung_tx_buf);
		mipi_dsi_buf_init(&pmsd->samsung_rx_buf);
		readed_size =
			mipi_dsi_cmds_rx(pMFD, &pmsd->samsung_tx_buf,
				&pmsd->samsung_rx_buf, &ea8061_read_reg_cmd,
					one_read_size);
		for (i = 0; i < readed_size; i++, show_cnt++) {
			show_buffer_pos +=
			snprintf(show_buffer + show_buffer_pos, 256, "%02x ",
					pmsd->samsung_rx_buf.data[i]);
			if (destBuffer != NULL && show_cnt < srcLength) {
				destBuffer[show_cnt] =
					pmsd->samsung_rx_buf.data[i];
			}
		}
		show_buffer_pos += snprintf(show_buffer +
				show_buffer_pos, 256, ".");
		read_pos += readed_size;
		if (read_pos > srcLength)
			break;
	}
	
	if (j == loop_limit)
		show_buffer_pos +=
			snprintf(show_buffer + show_buffer_pos, 256, "Overrun");

	printk("%s\n", show_buffer);
}

int smart_dimming_init_ea8061(struct msm_fb_data_type *mfd, struct mipi_panel_data *pmpd)
{
	int ret;
	
	init_table_info_ea8061(&pmpd->smart);

	ea8061_read_mtp(pmpd->msd, 0xDA, 32,
			pmpd->lcd_mtp_data, FALSE, mfd);

	calc_voltage_table_ea8061(&pmpd->smart, pmpd->lcd_mtp_data);

	ret = init_gamma_table_ea8061(pmpd);
	ret += init_aid_dimming_table_ea8061(pmpd);
	
	return ret;
}
