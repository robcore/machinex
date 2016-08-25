/*
 * =================================================================
 *
 *       Filename:  smart_mtp_s6e8aa0x01.c
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

#include "smart_dimming_s6evr02.h"
#include "mipi_samsung_define.h"
#include "mipi_samsung_oled.h"

//#include "smart_mtp_2p2_gamma.h"


//#define SMART_DIMMING_DEBUG


#define MAX_GAMMA			300

extern unsigned char SEQ_AOR_CONTROL[];
#define AID_PARAM_SIZE	2

static const unsigned int candela_table[GAMMA_MAX] = {
	 20,  30,  40,  50,  60,  70,  80,  90, 100,
	102, 104, 106, 108,
	110, 120, 130, 140, 150, 160, 170, 180,
	182, 184, 186, 188,
	190, 200, 210, 220, 230, 240, 250, MAX_GAMMA-1
};

#define aid_300nit		0xFF
#define aid_190nit_250nit		0xFF
#define aid_188nit			0xEA
#define aid_186nit			0xD6
#define aid_184nit			0xC2
#define aid_182nit			0xAD
#define aid_110nit_180nit		0x99
#define aid_108nit			0xA6
#define aid_106nit			0xB3
#define aid_104nit			0xC0
#define aid_102nit			0xCD
#define aid_100nit			0xDA
#define aid_90nit				0xC2
#define aid_80nit				0xAB
#define aid_70nit				0x93
#define aid_60nit				0x7D
#define aid_50nit				0x66
#define aid_40nit				0x51
#define aid_30nit				0x3C
#define aid_20nit				0x28
#define AOR40_BASE_188		202
#define AOR40_BASE_186		215
#define AOR40_BASE_184		230
#define AOR40_BASE_182		250
#define AOR40_BASE_180		275
#define AOR40_BASE_170		260
#define AOR40_BASE_160		246
#define AOR40_BASE_150		231
#define AOR40_BASE_140		217
#define AOR40_BASE_130		202
#define AOR40_BASE_120		188
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
	{GAMMA_108CD, IV_11, CI_RED, -1}, {GAMMA_108CD, IV_11, CI_GREEN, -2}, {GAMMA_104CD, IV_11, CI_BLUE, 3},
	{GAMMA_106CD, IV_11, CI_RED, -1}, {GAMMA_106CD, IV_11, CI_GREEN, -1}, {GAMMA_104CD, IV_11, CI_BLUE, 3},
	{GAMMA_104CD, IV_11, CI_RED, -2}, {GAMMA_104CD, IV_11, CI_GREEN, -1}, {GAMMA_104CD, IV_11, CI_BLUE, 4},
	{GAMMA_102CD, IV_11, CI_RED, -2}, {GAMMA_102CD, IV_11, CI_BLUE, 4},
	{GAMMA_100CD, IV_11, CI_RED, -2}, {GAMMA_100CD, IV_11, CI_BLUE, 5},
	{GAMMA_90CD, IV_11, CI_RED, -5}, {GAMMA_90CD, IV_11, CI_BLUE, 6},
	{GAMMA_80CD, IV_11, CI_RED, -6}, {GAMMA_80CD, IV_11, CI_BLUE, 8},
	{GAMMA_70CD, IV_11, CI_RED, -7}, {GAMMA_70CD, IV_11, CI_BLUE, 11},
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
	aid_20nit
};

static unsigned char aid_command_30[] = {
	aid_30nit
};

static unsigned char aid_command_40[] = {
	aid_40nit
};

static unsigned char aid_command_50[] = {
	aid_50nit
};

static unsigned char aid_command_60[] = {
	aid_60nit
};

static unsigned char aid_command_70[] = {
	aid_70nit
};

static unsigned char aid_command_80[] = {
	aid_80nit
};

static unsigned char aid_command_90[] = {
	aid_90nit
};

static unsigned char aid_command_100[] = {
	aid_100nit
};

static unsigned char aid_command_102[] = {
	aid_102nit
};

static unsigned char aid_command_104[] = {
	aid_104nit
};

static unsigned char aid_command_106[] = {
	aid_106nit
};

static unsigned char aid_command_108[] = {
	aid_108nit
};

static unsigned char aid_command_110[] = {
	aid_110nit_180nit
};

static unsigned char aid_command_120[] = {
	aid_110nit_180nit
};

static unsigned char aid_command_130[] = {
	aid_110nit_180nit
};

static unsigned char aid_command_140[] = {
	aid_110nit_180nit
};

static unsigned char aid_command_150[] = {
	aid_110nit_180nit
};

static unsigned char aid_command_160[] = {
	aid_110nit_180nit
};

static unsigned char aid_command_170[] = {
	aid_110nit_180nit
};

static unsigned char aid_command_180[] = {
	aid_110nit_180nit
};

static unsigned char aid_command_182[] = {
	aid_182nit
};

static unsigned char aid_command_184[] = {
	aid_184nit
};

static unsigned char aid_command_186[] = {
	aid_186nit
};

static unsigned char aid_command_188[] = {
	aid_188nit
};

static unsigned char aid_command_190[] = {
	aid_190nit_250nit
};

static unsigned char aid_command_200[] = {
	aid_190nit_250nit
};

static unsigned char aid_command_210[] = {
	aid_190nit_250nit
};

static unsigned char aid_command_220[] = {
	aid_190nit_250nit
};

static unsigned char aid_command_230[] = {
	aid_190nit_250nit
};

static unsigned char aid_command_240[] = {
	aid_190nit_250nit
};

static unsigned char aid_command_250[] = {
	aid_190nit_250nit
};

static unsigned char aid_command_300[] = {
	aid_300nit
};

static unsigned int aid_candela_table[GAMMA_MAX] = {
	base_20to100, base_20to100, base_20to100, base_20to100, base_20to100, base_20to100, base_20to100, base_20to100, base_20to100,
	AOR40_BASE_102, AOR40_BASE_104, AOR40_BASE_106, AOR40_BASE_108,
	AOR40_BASE_110, AOR40_BASE_120, AOR40_BASE_130, AOR40_BASE_140, AOR40_BASE_150,
	AOR40_BASE_160, AOR40_BASE_170, AOR40_BASE_180,
	AOR40_BASE_182, AOR40_BASE_184, AOR40_BASE_186, AOR40_BASE_188,
	190, 200, 210, 220, 230, 240, 250, MAX_GAMMA-1
};

unsigned char *aid_command_table[GAMMA_MAX] = {
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

int init_gamma_table(struct mipi_panel_data *pmpd)
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
			calc_gamma_table_210_20_100(smart, aid_candela_table[i], &psmart->gen_table[i].gamma_setting[1], G_21, mtp_offset);
		else if (candela_table[i] == 30)
			calc_gamma_table_210_20_100(smart, aid_candela_table[i], &psmart->gen_table[i].gamma_setting[1], G_213, mtp_offset);
		else if (candela_table[i] == 40)
			calc_gamma_table_210_20_100(smart, aid_candela_table[i], &psmart->gen_table[i].gamma_setting[1], G_215, mtp_offset);
		else if (candela_table[i] == 50)
			calc_gamma_table_210_20_100(smart, aid_candela_table[i], &psmart->gen_table[i].gamma_setting[1], G_218, mtp_offset);
		else if (candela_table[i] == 60)
			calc_gamma_table_210_20_100(smart, aid_candela_table[i], &psmart->gen_table[i].gamma_setting[1], G_22, mtp_offset);
		else if (candela_table[i] == 70)
			calc_gamma_table_210_20_100(smart, aid_candela_table[i], &psmart->gen_table[i].gamma_setting[1], G_222, mtp_offset);
		else if (candela_table[i] == 80)
			calc_gamma_table_210_20_100(smart, aid_candela_table[i], &psmart->gen_table[i].gamma_setting[1], G_223, mtp_offset);
		else if (candela_table[i] == 90)
			calc_gamma_table_210_20_100(smart, aid_candela_table[i], &psmart->gen_table[i].gamma_setting[1], G_224, mtp_offset);
		else if (candela_table[i] == 100)
			calc_gamma_table_210_20_100(smart, aid_candela_table[i], &psmart->gen_table[i].gamma_setting[1], G_225, mtp_offset);
		else if (candela_table[i] == 102)
			calc_gamma_table(smart, aid_candela_table[i], &psmart->gen_table[i].gamma_setting[1], G_224, mtp_offset);
		else if (candela_table[i] == 104)
			calc_gamma_table(smart, aid_candela_table[i], &psmart->gen_table[i].gamma_setting[1], G_223, mtp_offset);
		else if (candela_table[i] == 106)
			calc_gamma_table(smart, aid_candela_table[i], &psmart->gen_table[i].gamma_setting[1], G_222, mtp_offset);
		else if (candela_table[i] == 108)
			calc_gamma_table(smart, aid_candela_table[i], &psmart->gen_table[i].gamma_setting[1], G_221, mtp_offset);
		else if (candela_table[i] == 182)
			calc_gamma_table(smart, aid_candela_table[i], &psmart->gen_table[i].gamma_setting[1], G_221, mtp_offset);
		else if (candela_table[i] == 184)
			calc_gamma_table(smart, aid_candela_table[i], &psmart->gen_table[i].gamma_setting[1], G_222, mtp_offset);
		else if (candela_table[i] == 186)
			calc_gamma_table(smart, aid_candela_table[i], &psmart->gen_table[i].gamma_setting[1], G_223, mtp_offset);
		else if (candela_table[i] == 188)
			calc_gamma_table(smart, aid_candela_table[i], &psmart->gen_table[i].gamma_setting[1], G_224, mtp_offset);
		else if (candela_table[i] == 190)
			calc_gamma_table_215_190(smart, aid_candela_table[i], &psmart->gen_table[i].gamma_setting[1], G_225, mtp_offset);
		else if ((candela_table[i] > 190) && (candela_table[i] < MAX_GAMMA-1))
			calc_gamma_table(smart, aid_candela_table[i], &psmart->gen_table[i].gamma_setting[1], G_225 , mtp_offset);
		else
			calc_gamma_table(smart, aid_candela_table[i], &psmart->gen_table[i].gamma_setting[1], G_22, mtp_offset);
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

int init_aid_dimming_table(struct mipi_panel_data *pmpd)
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
		memcpy(psmart->gen_table[i].aor, SEQ_AOR_CONTROL, AID_PARAM_SIZE);
		psmart->gen_table[i].aor[0x01] = aid_command_table[i][0];
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

		for (c = CI_RED; c < CI_MAX ; c++)
			psmart->gen_table[i].gamma_setting[31+c] = pmpd->smart.default_gamma[30+c];
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

static void s6evr02_read_mtp(struct mipi_samsung_driver_data *pmsd, char srcReg, int srcLength, char *destBuffer,
		const int isUseMutex, struct msm_fb_data_type *pMFD)
{
	const int one_read_size = 4;
	const int loop_limit = 16;
	/* first byte = read-register */
	static char read_reg[2] = { 0xFF, 0x00 };
	static struct dsi_cmd_desc s6e8aa0_read_reg_cmd = {
		DTYPE_DCS_READ, 1, 0, 1, 5, sizeof(read_reg), read_reg };
	/* first byte is size of Register */
	static char packet_size[] = { 0x04, 0 };
	static struct dsi_cmd_desc s6e8aa0_packet_size_cmd = {
	DTYPE_MAX_PKTSIZE, 1, 0, 0, 0, sizeof(packet_size), packet_size };

	/* second byte is Read-position */
	static char reg_read_pos[] = { 0xB0, 0x00 };
	static struct dsi_cmd_desc s6e8aa0_read_pos_cmd = {
		DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(reg_read_pos),
		reg_read_pos };

	static char reg_pre_set1[] = {  0xF0, 0x5A, 0x5A };
	static struct dsi_cmd_desc preset1_cmd = {
		DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(reg_pre_set1),
		reg_pre_set1 };

	static char reg_pre_set2[] = {  0xFC, 0x5A, 0x5A };
	static struct dsi_cmd_desc preset2_cmd = {
		DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(reg_pre_set2),
		reg_pre_set2 };

	static char reg_post_set1[] = {  0xFC, 0xA5, 0xA5 };
	static struct dsi_cmd_desc postset1_cmd = {
		DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(reg_post_set1),
		reg_post_set1 };

	static char reg_post_set2[] = {  0xF0, 0xA5, 0xA5 };
	static struct dsi_cmd_desc postset2_cmd = {
		DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(reg_post_set2),
		reg_post_set2 };

	int read_pos;
	int readed_size;
	int show_cnt;

	int i, j;
	char show_buffer[256];
	int show_buffer_pos = 0;

	mipi_dsi_buf_init(&pmsd->samsung_tx_buf);
	mipi_dsi_cmds_tx(&pmsd->samsung_tx_buf, &(preset1_cmd), 1);

	mipi_dsi_buf_init(&pmsd->samsung_tx_buf);
	mipi_dsi_cmds_tx(&pmsd->samsung_tx_buf, &(preset2_cmd), 1);

	msleep(20);
	
	read_reg[0] = srcReg;

	show_buffer_pos +=
		snprintf(show_buffer, 256, "read_reg : %X[%d] : ",
				srcReg, srcLength);

	read_pos = 0;
	readed_size = 0;

	packet_size[0] = (char)srcLength;
	mipi_dsi_buf_init(&pmsd->samsung_tx_buf);
	mipi_dsi_cmds_tx(&pmsd->samsung_tx_buf, &(s6e8aa0_packet_size_cmd),
			1);

	show_cnt = 0;
	for (j = 0; j < loop_limit; j++) {
		reg_read_pos[1] = read_pos;
		if (mipi_dsi_cmds_tx
			(&pmsd->samsung_tx_buf, &(s6e8aa0_read_pos_cmd),
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
				&pmsd->samsung_rx_buf, &s6e8aa0_read_reg_cmd,
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

	mipi_dsi_buf_init(&pmsd->samsung_tx_buf);
	mipi_dsi_cmds_tx(&pmsd->samsung_tx_buf, &(postset1_cmd), 1);

	mipi_dsi_buf_init(&pmsd->samsung_tx_buf);
	mipi_dsi_cmds_tx(&pmsd->samsung_tx_buf, &(postset2_cmd), 1);
	
	if (j == loop_limit)
		show_buffer_pos +=
			snprintf(show_buffer + show_buffer_pos, 256, "Overrun");

	printk("%s\n", show_buffer);
}

int smart_dimming_init(struct msm_fb_data_type *mfd, struct mipi_panel_data *pmpd)
{
	int ret;
	
	init_table_info(&pmpd->smart);

	s6evr02_read_mtp(pmpd->msd, MTP_REGISTER, MTP_DATA_SIZE,
			pmpd->lcd_mtp_data, FALSE, mfd);

	calc_voltage_table(&pmpd->smart, pmpd->lcd_mtp_data);

	ret = init_gamma_table(pmpd);
	ret += init_aid_dimming_table(pmpd);
	
	return ret;
}

