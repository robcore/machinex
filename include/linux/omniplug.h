/* include/linux/machinex_defines.h
 * This is a little header for any and all
 * definitions that I would like to use
 * on a global scale.
 */

#ifndef __LINUX_OMNIPLUG_H
#define __LINUX_OMNIPLUG_H

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
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/mutex.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/kernel.h>
#include <linux/irq.h>
#include <linux/ioctl.h>
#include <linux/delay.h>
#include <linux/reboot.h>
#include <linux/debugfs.h>
#include <linux/completion.h>
#include <linux/workqueue.h>
#include <linux/clk.h>
#include <asm/mach-types.h>
#include <asm/uaccess.h>
#include <linux/notifier.h>

#define DEFAULT_MIN_CPUS_ONLINE 2
#define DEFAULT_MAX_CPUS_ONLINE NR_CPUS
extern unsigned int min_cpus_online;
extern unsigned int max_cpus_online;
extern unsigned int cpus_boosted;

/*Alucard*/
extern void cpus_hotplugging(unsigned int status);
/*Intelliplug*/
extern void intelli_plug_active_eval_fn(unsigned int status);
/*mx hotplug*/
extern void ignition(unsigned int status);
/*MSM Sleeper*/
extern void start_stop_sleeper(int enabled);
/*Bricked*/
extern int bricked_hotplug_start(void);
extern void bricked_hotplug_stop(void);
/*lazy*/
extern void start_stop_lazy_plug(unsigned int enabled);
/*blu*/
extern void dyn_hp_init_exit(unsigned int enabled);
/*userspace*/
extern void uplug_start_stop(unsigned int enabled);
#endif /* __LINUX_OMNIPLUG_H */