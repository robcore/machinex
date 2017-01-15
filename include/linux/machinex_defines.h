/* include/linux/machinex_defines.h
 * This is a little header for any and all
 * definitions that I would like to use
 * on a global scale.
 */

#ifndef __LINUX_MACHINEX_DEFINES_H
#define __LINUX_MACHINEX_DEFINES_H

#include <linux/cpufreq.h>
#include <linux/cpu.h>
#include <linux/jiffies.h>
#include <linux/kernel_stat.h>
#include <linux/tick.h>
#include <linux/sched.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/cpumask.h>
#include <linux/threads.h>

#define DEFAULT_MIN_CPUS_ONLINE 2
#define DEFAULT_MAX_CPUS_ONLINE NR_CPUS

#endif