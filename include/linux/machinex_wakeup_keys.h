/*
 * kobject.h - generic kernel object infrastructure.
 *
 * Copyright (c) 2002-2003 Patrick Mochel
 * Copyright (c) 2002-2003 Open Source Development Labs
 * Copyright (c) 2006-2008 Greg Kroah-Hartman <greg@kroah.com>
 * Copyright (c) 2006-2008 Novell Inc.
 *
 * This file is released under the GPLv2.
 *
 * Please read Documentation/kobject.txt before using the kobject
 * interface, ESPECIALLY the parts about reference counts and object
 * destructors.
 */

#ifndef _MACHINEX_WAKEUP_KEYS_H_
#define _MACHINEX_WAKEUP_KEYS_H_

#include <linux/init.h>
#include <linux/types.h>
#include <linux/module.h>

bool is_volume_wake(void);
bool is_home_wake(void);

#endif