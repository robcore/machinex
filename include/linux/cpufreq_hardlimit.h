/*
 * Author: Jean-Pierre Rasquin <yank555.lu@gmail.com>
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

#ifndef _LINUX_CPUFREQ_HARDLIMIT_H
#define _LINUX_CPUFREQ_HARDLIMIT_H

#define CPUFREQ_HARDLIMIT_VERSION "v2.2 by Yank555.lu"

/* Default frequencies for hammerhead */
#define CPUFREQ_HARDLIMIT_MAX_SCREEN_ON_STOCK	1890000
#define CPUFREQ_HARDLIMIT_MAX_SCREEN_OFF_STOCK	1890000
#define CPUFREQ_HARDLIMIT_MIN_SCREEN_ON_STOCK	384000
#define CPUFREQ_HARDLIMIT_MIN_SCREEN_OFF_STOCK	384000

#ifdef SUPERFLUOUS
#define CPUFREQ_HARDLIMIT_WAKEUP_KICK_FREQ		1890000

#define CPUFREQ_HARDLIMIT_TOUCHBOOST_LO_DEFAULT	918000
#define CPUFREQ_HARDLIMIT_TOUCHBOOST_HI_DEFAULT	1134000
#endif
#define CPUFREQ_HARDLIMIT_SCREEN_ON	0		/* default, consider we boot with screen on */
#define CPUFREQ_HARDLIMIT_SCREEN_OFF	1

/* Userspace access to scaling min/max */
#ifdef CONFIG_SEC_DVFS
#define CPUFREQ_HARDLIMIT_USERSPACE_DVFS_ALLOW	0
#define CPUFREQ_HARDLIMIT_USERSPACE_DVFS_IGNORE	1
#define CPUFREQ_HARDLIMIT_USERSPACE_DVFS_REFUSE	2
#endif
#ifdef SUPERFLUOUS
#define CPUFREQ_HARDLIMIT_WAKEUP_KICK_DELAY_MAX	10000	/* Don't allow for more than 10 seconds */

#define CPUFREQ_HARDLIMIT_WAKEUP_KICK_INACTIVE	0
#define CPUFREQ_HARDLIMIT_WAKEUP_KICK_ACTIVE	1

#define CPUFREQ_HARDLIMIT_WAKEUP_KICK_DISABLED	0

/* Touchboost */
#define CPUFREQ_HARDLIMIT_TOUCHBOOST_INACTIVE	0
#define CPUFREQ_HARDLIMIT_TOUCHBOOST_ACTIVE_LO	1
#define CPUFREQ_HARDLIMIT_TOUCHBOOST_ACTIVE_HI	2

#define CPUFREQ_HARDLIMIT_TOUCHBOOST_EVENTS	3
#define CPUFREQ_HARDLIMIT_TOUCHBOOST_EVENTS_MIN	1
#define CPUFREQ_HARDLIMIT_TOUCHBOOST_EVENTS_MAX	10

#define CPUFREQ_HARDLIMIT_TOUCHBOOST_DISABLED	0
#define CPUFREQ_HARDLIMIT_TOUCHBOOST_DELAY_MAX	5000	/* Don't allow for more than 5 seconds */
#endif
/* Sanitize cpufreq to hardlimits */
unsigned int check_cpufreq_hardlimit(unsigned int freq);

#ifdef CONFIG_SEC_DVFS
/* Scaling min/max lock */
unsigned int userspace_dvfs_lock_status(void);
#endif

/* Hook in cpufreq for scaling min./max. */
void update_scaling_limits(unsigned int freq_min, unsigned int freq_max);

#endif
