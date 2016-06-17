/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2001 - 2005 Tensilica Inc.
 */

#ifndef _XTENSA_SYSTEM_H
#define _XTENSA_SYSTEM_H

#define smp_read_barrier_depends() do { } while(0)
#define read_barrier_depends() do { } while(0)

#define mb()  barrier()
#define rmb() mb()
#define wmb() mb()

#ifdef CONFIG_SMP
#error smp_* not defined
#endif

#include <asm-generic/barrier.h>

#endif /* _XTENSA_SYSTEM_H */
