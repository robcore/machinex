/* SPDX-License-Identifier: GPL-2.0 */
/*
 * include/linux/cpu.h - generic cpu definition
 *
 * This is mainly for topological representation. We define the
 * basic 'struct cpu' here, which can be embedded in per-arch
 * definitions of processors.
 *
 * Basic handling of the devices is done in drivers/base/cpu.c
 *
 * CPUs are exported via sysfs in the devices/system/cpu
 * directory.
 */
#ifndef _LINUX_CPU_H_
#define _LINUX_CPU_H_

#include <linux/node.h>
#include <linux/compiler.h>
#include <linux/cpumask.h>
#include <linux/cpuhotplug.h>

struct device;

struct cpu {
	int node_id;		/* The node which contains the CPU */
	int hotpluggable;	/* creates sysfs control file if hotpluggable */
	struct device dev;
};

extern void boot_cpu_init(void);
extern void boot_cpu_state_init(void);

extern int register_cpu(struct cpu *cpu, int num);
extern struct device *get_cpu_device(unsigned cpu);
extern bool cpu_is_hotpluggable(unsigned cpu);
extern bool arch_match_cpu_phys_id(int cpu, u64 phys_id);

extern int cpu_add_dev_attr(struct device_attribute *attr);
extern void cpu_remove_dev_attr(struct device_attribute *attr);

extern int cpu_add_dev_attr_group(struct attribute_group *attrs);
extern void cpu_remove_dev_attr_group(struct attribute_group *attrs);

#ifdef CONFIG_HOTPLUG_CPU
extern void unregister_cpu(struct cpu *cpu);
extern ssize_t arch_cpu_probe(const char *, size_t);
extern ssize_t arch_cpu_release(const char *, size_t);
#endif
struct notifier_block;

#ifdef CONFIG_ARCH_HAS_CPU_AUTOPROBE
extern int arch_cpu_uevent(struct device *dev, struct kobj_uevent_env *env);
extern ssize_t arch_print_cpu_modalias(struct device *dev,
				       struct device_attribute *attr,
				       char *bufptr);
#endif

#define CPU_ONLINE		0x0002 /* CPU (unsigned)v is up */
#define CPU_UP_PREPARE		0x0003 /* CPU (unsigned)v coming up */
#define CPU_UP_CANCELED		0x0004 /* CPU (unsigned)v NOT coming up */
#define CPU_DOWN_PREPARE	0x0005 /* CPU (unsigned)v going down */
#define CPU_DOWN_FAILED		0x0006 /* CPU (unsigned)v NOT going down */
#define CPU_DEAD		0x0007 /* CPU (unsigned)v dead */
#define CPU_POST_DEAD		0x0009 /* CPU (unsigned)v dead, cpu_hotplug
					* lock is dropped */
#define CPU_BROKEN		0x000B /* CPU (unsigned)v did not die properly,
					* perhaps due to preemption. */

/* Used for CPU hotplug events occurring while tasks are frozen due to a suspend
 * operation in progress
 */
#define CPU_TASKS_FROZEN	0x0010

#define CPU_ONLINE_FROZEN	(CPU_ONLINE | CPU_TASKS_FROZEN)
#define CPU_UP_PREPARE_FROZEN	(CPU_UP_PREPARE | CPU_TASKS_FROZEN)
#define CPU_UP_CANCELED_FROZEN	(CPU_UP_CANCELED | CPU_TASKS_FROZEN)
#define CPU_DOWN_PREPARE_FROZEN	(CPU_DOWN_PREPARE | CPU_TASKS_FROZEN)
#define CPU_DOWN_FAILED_FROZEN	(CPU_DOWN_FAILED | CPU_TASKS_FROZEN)
#define CPU_DEAD_FROZEN		(CPU_DEAD | CPU_TASKS_FROZEN)


#ifdef CONFIG_SMP
extern bool cpuhp_tasks_frozen;
/* Need to know about CPUs going up/down? */
#if defined(CONFIG_HOTPLUG_CPU) || !defined(MODULE)
#define cpu_notifier(fn, pri) {					\
	static struct notifier_block fn##_nb =			\
		{ .notifier_call = fn, .priority = pri };	\
	register_cpu_notifier(&fn##_nb);			\
}

#define __cpu_notifier(fn, pri) {				\
	static struct notifier_block fn##_nb =			\
		{ .notifier_call = fn, .priority = pri };	\
	__register_cpu_notifier(&fn##_nb);			\
}

extern int register_cpu_notifier(struct notifier_block *nb);
extern int __register_cpu_notifier(struct notifier_block *nb);
extern void unregister_cpu_notifier(struct notifier_block *nb);
extern void __unregister_cpu_notifier(struct notifier_block *nb);

#else /* #if defined(CONFIG_HOTPLUG_CPU) || !defined(MODULE) */
#define cpu_notifier(fn, pri)	do { (void)(fn); } while (0)
#define __cpu_notifier(fn, pri)	do { (void)(fn); } while (0)

static inline int register_cpu_notifier(struct notifier_block *nb)
{
	return 0;
}

static inline int __register_cpu_notifier(struct notifier_block *nb)
{
	return 0;
}

static inline void unregister_cpu_notifier(struct notifier_block *nb)
{
}

static inline void __unregister_cpu_notifier(struct notifier_block *nb)
{
}
#endif

int cpu_up(unsigned int cpu);
void notify_cpu_starting(unsigned int cpu);
extern void cpu_maps_update_begin(void);
int cpu_maps_is_updating(void);
extern void cpu_maps_update_done(void);

#define cpu_notifier_register_begin	cpu_maps_update_begin
#define cpu_notifier_register_done	cpu_maps_update_done

#else	/* CONFIG_SMP */
#define cpuhp_tasks_frozen	0

#define cpu_notifier(fn, pri)	do { (void)(fn); } while (0)
#define __cpu_notifier(fn, pri)	do { (void)(fn); } while (0)

static inline int register_cpu_notifier(struct notifier_block *nb)
{
	return 0;
}

static inline int __register_cpu_notifier(struct notifier_block *nb)
{
	return 0;
}

static inline void unregister_cpu_notifier(struct notifier_block *nb)
{
}

static inline void __unregister_cpu_notifier(struct notifier_block *nb)
{
}

static inline void cpu_maps_update_begin(void)
{
}

static inline int cpu_maps_is_updating(void)
{
	return 0;
}

static inline void cpu_maps_update_done(void)
{
}

static inline void cpu_notifier_register_begin(void)
{
}

static inline void cpu_notifier_register_done(void)
{
}

#endif /* CONFIG_SMP */
extern struct bus_type cpu_subsys;

#ifdef CONFIG_HOTPLUG_CPU

extern void cpus_write_lock(void);
extern void cpus_write_unlock(void);
extern void cpus_read_lock(void);
extern void cpus_read_unlock(void);
extern void lockdep_assert_cpus_held(void);
extern void cpu_hotplug_disable(void);
extern void cpu_hotplug_enable(void);
#define hotcpu_notifier(fn, pri)	cpu_notifier(fn, pri)
#define __hotcpu_notifier(fn, pri)	__cpu_notifier(fn, pri)
#define register_hotcpu_notifier(nb)	register_cpu_notifier(nb)
#define __register_hotcpu_notifier(nb)	__register_cpu_notifier(nb)
#define unregister_hotcpu_notifier(nb)	unregister_cpu_notifier(nb)
#define __unregister_hotcpu_notifier(nb)	__unregister_cpu_notifier(nb)
void clear_tasks_mm_cpumask(int cpu);
int cpu_down(unsigned int cpu);

#else /* CONFIG_HOTPLUG_CPU */

static inline void cpus_write_lock(void) { }
static inline void cpus_write_unlock(void) { }
static inline void cpus_read_lock(void) { }
static inline void cpus_read_unlock(void) { }
static inline void lockdep_assert_cpus_held(void) { }
static inline void cpu_hotplug_disable(void) { }
static inline void cpu_hotplug_enable(void) { }
#endif	/* !CONFIG_HOTPLUG_CPU */

/* Wrappers which go away once all code is converted */
static inline void cpu_hotplug_begin(void) { cpus_write_lock(); }
static inline void cpu_hotplug_done(void) { cpus_write_unlock(); }
static inline void get_online_cpus(void) { cpus_read_lock(); }
static inline void put_online_cpus(void) { cpus_read_unlock(); }

void hardplug_all_cpus(void);
extern unsigned int limit_screen_on_cpus;
unsigned int nr_hardplugged_cpus(void);

#ifdef CONFIG_PM_SLEEP_SMP
extern int freeze_secondary_cpus(int primary);
static inline int disable_nonboot_cpus(void)
{
	return freeze_secondary_cpus(0);
}
extern void enable_nonboot_cpus(void);

extern unsigned int limit_screen_off_cpus;
extern cpumask_t screen_off_allowd_msk;

extern int lock_screen_off_cpus(int primary);
extern void unlock_screen_off_cpus(void);
extern unsigned int is_cpu_allowed(unsigned int cpu);
unsigned int is_cpu_allowed_for_therm(unsigned int cpu);
unsigned int thermal_override(void);
#else /* !CONFIG_PM_SLEEP_SMP */
static inline int disable_nonboot_cpus(void) { return 0; }
static inline void enable_nonboot_cpus(void) {}
#endif /* !CONFIG_PM_SLEEP_SMP */

#define IDLE_START 1
#define IDLE_END 2

void idle_notifier_register(struct notifier_block *n);
void idle_notifier_unregister(struct notifier_block *n);
void idle_notifier_call_chain(unsigned long val);

void cpu_startup_entry(enum cpuhp_state state);
void cpu_idle(void);
void cpu_idle_poll_ctrl(bool enable);
void per_cpu_idle_poll_ctrl(int cpu, bool enable);

void arch_cpu_idle(void);
void arch_cpu_idle_prepare(void);
void arch_cpu_idle_enter(void);
void arch_cpu_idle_exit(void);
void arch_cpu_idle_dead(void);

int cpu_report_state(int cpu);
int cpu_check_up_prepare(int cpu);
void cpu_set_state_online(int cpu);
void play_idle(unsigned long duration_ms);

#ifdef CONFIG_HOTPLUG_CPU
bool cpu_wait_death(unsigned int cpu, int seconds);
bool cpu_report_death(void);
void cpuhp_report_idle_dead(void);
#else
static inline void cpuhp_report_idle_dead(void) { }
#endif /* #ifdef CONFIG_HOTPLUG_CPU */
/* warning shut if msm thermal holding cores off on high temp. */
extern bool thermal_core_controlled(unsigned int cpu);
extern bool intelli_init(void);
extern int cpu_idle_force_poll_hook;
extern void fuel_injector(void);
#endif /* _LINUX_CPU_H_ */

