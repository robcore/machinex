/*
 * =================================================================
 *
 *       Filename:  smart_dimming.h
 *
 *    Description:  cmc624, lvds control driver
 *
 *        Author:  Krishna Kishor Jha,
 *        Company:  Samsung Electronics
 *
 * ================================================================
 */
/*
<one line to give the program's name and a brief idea of what it does.>
Copyright (C) 2011, Samsung Electronics. All rights reserved.

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

#ifndef __SMART_DIMMING_H__
#define __SMART_DIMMING_H__

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/ctype.h>
#include "mipi_samsung_define.h"

#define GAMMA_300CD_MAX_EA8061	9

int init_table_info_ea8061(struct str_smart_dim *smart);
u8 calc_voltage_table_ea8061(struct str_smart_dim *smart,  u8* mtp);
u32 calc_gamma_table_ea8061(struct str_smart_dim *smart, u32 gv, u8 result[], u8 gamma_curve, const u8 *mtp);
u32 calc_gamma_table_190_ea8061(struct str_smart_dim *smart, u32 gv, u8 result[], u8 gamma_curve, const u8 *mtp);
u32 calc_gamma_table_20_100_ea8061(struct str_smart_dim *smart, u32 gv, u8 result[], u8 gamma_curve, const u8 *mtp);

#endif
