/* SPDX-License-Identifier: GPL-2.0 */
#include <uapi/asm-generic/unistd.h>

/*
 * These are required system calls, we should
 * invert the logic eventually and let them
 * be selected by default.
 */
#if __BITS_PER_LONG == 32
#define __ARCH_WANT_STAT64
#define __ARCH_WANT_SYS_LLSEEKzz
#endif
