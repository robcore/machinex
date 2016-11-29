/*
 *  linux/arch/arm/kernel/signal.h
 *
 *  Copyright (C) 2005-2009 Russell King.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#define KERN_RESTART_CODE	(KERN_SIGRETURN_CODE + sizeof(sigreturn_codes))

extern const unsigned long syscall_restart_code[2];
