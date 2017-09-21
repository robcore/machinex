/* include/linux/prometheus.h
 *
 * Copyright (C) 2007-2008 Google, Inc.
 * Copyright (C) 2013 Paul Reioux
 *
 * Modified by Jean-Pierre Rasquin <yank555.lu@gmail.com>
 *
 * Autosleep and Hybird put back (eliminater74@gmail.com
 *
 * Further Enhanced by Rob Patershuk <robpatershuk@gmail.com>
 *
 *  v1.1 - make powersuspend not depend on a userspace initiator anymore,
 *         but use a hook in autosleep instead.
 *
 *  v1.2 - make kernel / userspace mode switchable
 *
 *  v1.3 - add a hook in display panel driver as alternative kernel trigger
 *
 *  v1.4 - add a hybrid-kernel mode, accepting both kernel hooks (first wins)
 *
 *  v1.5 - fix hybrid-kernel mode cannot be set through sysfs
 *
 *  v1.6 - remove autosleep and hybrid modes (autosleep not working on shamu)
 *
 *  v1.7 - do only run state change if change actually requests a new state
 *
 * v1.7.1 - Add autosleep and hybrid modes back
 *
 * v1.7.2 Cleanup
 *
 * v1.8.0 Changed "userspace" mode to disabled, and removed the ability to store
 *        to the syfs. Now The driver is autonomous, without any userspace
 *        interaction required/allowed. Also a little bit of sysfs
 *		  ordering cleanup.
 *
 * v1.8.1 Replaced deprecated singlethread workqueue with updated schedule_work.
 *
 * v1.9.1 Updated the depecrated method of declaring work but simply declaring
 *		  the two work structs.  Also actually INITialized the work on init, and
 *        flushed it on exit.
 *
 * v1.9.2 Remove unneccessary "MODE" variable as we only have one mechanism of
 *		  action remaining. Also removed the useless state sysfs entry.  Like
 *		  state notifier, we can only see "state" when the screen is on, so
 *		  it is pointless to expose to userspace. Topped off with some cleanup.
 *
 * v2.0   Final cleanup to functionality.  For faster response to screen on/off events,
 *		  ensure that the previous work is cancelled upon a valid requested state change.
 *		  Switched back to a single thread workqueue but allocated properly. Topped off
 *		  with some driver cleanup and a config option for using the SUB_MINOR_VERISON.
 *
 * v2.1   Provide a user-configurable option to sync the system on powersuspend.
 *
 * v2.2   Remove the mutex unlock mistakingly added during system sync setup.
 *
 * v2.3   Move cancelling previous state's work to the current state change work item.
 *
 * v2.4   Back out of suspend if the device is in the process of rebooting, powering
 *		  off, or poweroff_charging. Additionally, refactor abort handling so that
 *		  sys_sync isnt called within a mutex-locked region.  Additionally, set sysfs
 *		  up in a way that doesn't allow for failure. No functional change
 *		  to actual suspend/resume handling other than the fixes mentioned.
 *
 * v2.5	  Added a big, scary hook to call pm_suspend.
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

#ifndef _LINUX_PROMETHEUS_H
#define _LINUX_PROMETHEUS_H

#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include <linux/syscalls.h> /* sys_sync */
#include <linux/power_supply.h>
#include <linux/suspend.h>

struct power_suspend {
	struct list_head link;
	void (*suspend)(struct power_suspend *h);
	void (*resume)(struct power_suspend *h);
};

void register_power_suspend(struct power_suspend *handler);
void unregister_power_suspend(struct power_suspend *handler);

enum {
	POWER_SUSPEND_INACTIVE = 0,
	POWER_SUSPEND_ACTIVE = 1,
};

void prometheus_panel_beacon(unsigned int new_state);
bool prometheus_sec_jack(void);
bool android_os_ws(void);
#endif
