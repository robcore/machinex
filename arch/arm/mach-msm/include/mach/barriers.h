/* Copyright (c) 2010-2011, The Linux Foundation. All rights reserved.
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
#include <mach/memory.h>

#define mb() do \
	{ \
		dsb();\
		outer_sync(); \
		write_to_strongly_ordered_memory(); \
	} while (0)
#define rmb()	do { dmb(); write_to_strongly_ordered_memory(); } while (0)
#define wmb()	mb()
