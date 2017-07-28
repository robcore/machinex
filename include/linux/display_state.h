/*
 * include/linux/display_state.h
 *
 * Copyright (c) 2016-2017 Francisco Franco
 * franciscofranco.1990@gmail.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _LINUX_DISPLAY_STATE_H
#define _LINUX_DISPLAY_STATE_H
#include <linux/kernel.h>

bool is_display_on(void);

static bool system_is_restarting(void)
{
	if (system_state == SYSTEM_POWER_OFF ||
		system_state == SYSTEM_RESTART ||
		system_state == SYSTEM_HALT)
		return true;
	return false;
}

#endif /* _LINUX_DISPLAY_STATE_H */
