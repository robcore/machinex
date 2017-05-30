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

#define CPUFREQ_HARDLIMIT_VERSION "v2.3 by Yank555.lu, with updates by Robcore."

/* Default frequencies for MACH_JF */
#define CPUFREQ_HARDLIMIT_MAX_SCREEN_ON_STOCK	1890000
#define CPUFREQ_HARDLIMIT_MAX_SCREEN_OFF_STOCK	1890000
#define CPUFREQ_HARDLIMIT_MIN_SCREEN_ON_STOCK	384000
#define CPUFREQ_HARDLIMIT_MIN_SCREEN_OFF_STOCK	384000
#define CPUFREQ_HARDLIMIT_SCREEN_ON	0		/* default, consider we boot with screen on */
#define CPUFREQ_HARDLIMIT_SCREEN_OFF	1

/* Sanitize cpufreq to hardlimits */
unsigned int check_cpufreq_hardlimit(unsigned int freq);

/* Hook in cpufreq for scaling min./max. */
unsigned int current_limit_max;
unsigned int current_limit_max;

void update_scaling_limits(unsigned int freq_min, unsigned int freq_max);
void reapply_hard_limits(void);

#endif
