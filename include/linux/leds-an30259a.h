/*
 * Copyright (C) 2011 Samsung Electronics Co. Ltd. All Rights Reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _LEDS_AN30259A_H
#define _LEDS_AN30259A_H

#include <uapi/linux/leds-an30259a.h>

#define AN30259A_PR_SET_LED	_IOW('S', 42, struct an30259a_pr_control)
#define AN30259A_PR_SET_LEDS	_IOW('S', 43, struct an30259a_pr_control[3])
#define AN30259A_PR_SET_IMAX	_IOW('S', 44, __u8)

#endif						/* _LEDS_AN30259A_H */
