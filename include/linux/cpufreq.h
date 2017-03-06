/*
 * linux/include/linux/cpufreq.h
 *
 * Copyright (C) 2001 Russell King
 *           (C) 2002 - 2003 Dominik Brodowski <linux@brodo.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef _LINUX_CPUFREQ_H
#define _LINUX_CPUFREQ_H

#include <linux/mutex.h>
#include <linux/notifier.h>
#include <linux/threads.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/completion.h>
#include <linux/workqueue.h>
#include <linux/cpumask.h>
#include <asm/div64.h>
#ifdef CONFIG_CPUFREQ_HARDLIMIT
#include <linux/cpufreq_hardlimit.h>
#endif
#include <asm/cputime.h>

/*********************************************************************
 *                        CPUFREQ INTERFACE                          *
 *********************************************************************/
/*
 * Frequency values here are CPU kHz
 *
 * Maximum transition latency is in nanoseconds - if it's unknown,
 * CPUFREQ_ETERNAL shall be used.
 */

#define CPUFREQ_ETERNAL			(-1)
#define CPUFREQ_NAME_LEN		16
/* Print length for names. Extra 1 space for accomodating '\n' in prints */
#define CPUFREQ_NAME_PLEN		(CPUFREQ_NAME_LEN + 1)

#define UTIL_THRESHOLD			(25)
/* Minimum frequency cutoff to notify the userspace about cpu utilization
 * changes */
#define MIN_CPU_UTIL_NOTIFY   40
struct cpufreq_governor;

struct cpufreq_freqs {
	unsigned int cpu;	/* cpu nr */
	unsigned int old;
	unsigned int new;
	u8 flags;		/* flags of cpufreq_driver, see below. */
};

struct cpufreq_cpuinfo {
	unsigned int		max_freq;
	unsigned int		min_freq;

	/* in 10^(-9) s = nanoseconds */
	unsigned int		transition_latency;
};

struct cpufreq_real_policy {
	unsigned int		min;    /* in kHz */
	unsigned int		max;    /* in kHz */
	unsigned int		util_thres;
	unsigned int		policy; /* see above */
	struct cpufreq_governor	*governor; /* see below */
};

struct cpufreq_policy {
	/* CPUs sharing clock, require sw coordination */
	cpumask_var_t		cpus;	/* Online CPUs only */
	cpumask_var_t		related_cpus; /* Online + Offline CPUs */

	unsigned int		shared_type; /* ACPI: ANY or ALL affected CPUs
						should set cpufreq */
	unsigned int		cpu;    /* cpu nr of CPU managing this policy */
	unsigned int		last_cpu; /* cpu nr of previous CPU that managed
					   * this policy */
	struct cpufreq_cpuinfo	cpuinfo;/* see above */

	unsigned int		min;    /* in kHz */
	unsigned int		max;    /* in kHz */
	unsigned int		cur;    /* in kHz, only needed if cpufreq
					 * governors are used */
	unsigned int            util;  /* CPU utilization at max frequency */
	unsigned int		util_thres; /* Threshold to increase utilization*/
	unsigned int		policy; /* see above */
	struct cpufreq_governor	*governor; /* see below */
	void			*governor_data;
	bool			governor_enabled; /* governor start/stop flag */

	struct work_struct	update; /* if update_policy() needs to be
					 * called, but you're in IRQ context */

	struct cpufreq_real_policy	user_policy;

	struct list_head        policy_list;
	struct kobject		kobj;
	struct completion	kobj_unregister;

	/*
	 * The rules for this semaphore:
	 * - Any routine that wants to read from the policy structure will
	 *   do a down_read on this semaphore.
	 * - Any routine that will write to the policy structure and/or may take away
	 *   the policy altogether (eg. CPU hotplug), will hold this lock in write
	 *   mode before doing so.
	 *
	 * Additional rules:
	 * - Lock should not be held across
	 *     __cpufreq_governor(data, CPUFREQ_GOV_POLICY_EXIT);
	 */
	struct rw_semaphore	rwsem;
};

/* Only for ACPI */
#define CPUFREQ_SHARED_TYPE_NONE (0) /* None */
#define CPUFREQ_SHARED_TYPE_HW	 (1) /* HW does needed coordination */
#define CPUFREQ_SHARED_TYPE_ALL	 (2) /* All dependent CPUs should set freq */
#define CPUFREQ_SHARED_TYPE_ANY	 (3) /* Freq can be set from any dependent CPU*/

#ifdef CONFIG_CPU_FREQ
struct cpufreq_policy *cpufreq_cpu_get(unsigned int cpu);
void cpufreq_cpu_put(struct cpufreq_policy *policy);
#else
static inline struct cpufreq_policy *cpufreq_cpu_get(unsigned int cpu)
{
	return NULL;
}
static inline void cpufreq_cpu_put(struct cpufreq_policy *policy) { }
#endif

static inline bool policy_is_shared(struct cpufreq_policy *policy)
{
	return cpumask_weight(policy->cpus) > 1;
}

/* /sys/devices/system/cpu/cpufreq: entry point for global variables */
extern struct kobject *cpufreq_global_kobject;
int cpufreq_get_global_kobject(void);
void cpufreq_put_global_kobject(void);
int cpufreq_sysfs_create_file(const struct attribute *attr);
void cpufreq_sysfs_remove_file(const struct attribute *attr);

#ifdef CONFIG_CPU_FREQ
unsigned int cpufreq_get(unsigned int cpu);
unsigned int cpufreq_quick_get(unsigned int cpu);
unsigned int cpufreq_quick_get_max(unsigned int cpu);
unsigned int cpufreq_quick_get_util(unsigned int cpu);
void disable_cpufreq(void);

u64 get_cpu_idle_time(unsigned int cpu, u64 *wall, int io_busy);
int cpufreq_get_policy(struct cpufreq_policy *policy, unsigned int cpu);
int cpufreq_update_policy(unsigned int cpu);
bool have_governor_per_policy(void);
struct kobject *get_governor_parent_kobj(struct cpufreq_policy *policy);
#else
static inline unsigned int cpufreq_get(unsigned int cpu)
{
	return 0;
}
static inline unsigned int cpufreq_quick_get(unsigned int cpu)
{
	return 0;
}
static inline unsigned int cpufreq_quick_get_max(unsigned int cpu)
{
	return 0;
}
static inline void disable_cpufreq(void) { }
#endif

/*********************************************************************
 *                      CPUFREQ DRIVER INTERFACE                     *
 *********************************************************************/

#define CPUFREQ_RELATION_L 0	/* lowest frequency at or above target */
#define CPUFREQ_RELATION_H 1	/* highest frequency below or at target */
#define CPUFREQ_RELATION_C 2	/* closest frequency to target */

struct freq_attr {
	struct attribute attr;
	ssize_t (*show)(struct cpufreq_policy *, char *);
	ssize_t (*store)(struct cpufreq_policy *, const char *, size_t count);
};

#define cpufreq_freq_attr_ro(_name)		\
static struct freq_attr _name =			\
__ATTR(_name, 0444, show_##_name, NULL)

#define cpufreq_freq_attr_ro_perm(_name, _perm)	\
static struct freq_attr _name =			\
__ATTR(_name, _perm, show_##_name, NULL)

#define cpufreq_freq_attr_rw(_name)		\
static struct freq_attr _name =			\
__ATTR(_name, 0644, show_##_name, store_##_name)

struct global_attr {
	struct attribute attr;
	ssize_t (*show)(struct kobject *kobj,
			struct attribute *attr, char *buf);
	ssize_t (*store)(struct kobject *a, struct attribute *b,
			 const char *c, size_t count);
};

#define define_one_global_ro(_name)		\
static struct global_attr _name =		\
__ATTR(_name, 0444, show_##_name, NULL)

#define define_one_global_rw(_name)		\
static struct global_attr _name =		\
__ATTR(_name, 0644, show_##_name, store_##_name)


struct cpufreq_driver {
	struct module           *owner;
	char			name[CPUFREQ_NAME_LEN];
	u8			flags;

	/* needed by all drivers */
	int	(*init)		(struct cpufreq_policy *policy);
	int	(*verify)	(struct cpufreq_policy *policy);

	/* define one out of two */
	int	(*setpolicy)	(struct cpufreq_policy *policy);
	int	(*target)	(struct cpufreq_policy *policy,	/* Deprecated */
				 unsigned int target_freq,
				 unsigned int relation);
	int	(*target_index)	(struct cpufreq_policy *policy,
				 unsigned int index);

	/* should be defined, if possible */
	unsigned int	(*get)	(unsigned int cpu);

	/* optional */
	unsigned int (*getavg)	(struct cpufreq_policy *policy,
				 unsigned int cpu);
	int	(*bios_limit)	(int cpu, unsigned int *limit);

	int	(*exit)		(struct cpufreq_policy *policy);
	int	(*suspend)	(struct cpufreq_policy *policy);
	int	(*resume)	(struct cpufreq_policy *policy);
	struct freq_attr	**attr;
};

/* flags */
#define CPUFREQ_STICKY		(1 << 0)	/* driver isn't removed even if
						   all ->init() calls failed */
#define CPUFREQ_CONST_LOOPS	(1 << 1)	/* loops_per_jiffy or other
						   kernel "constants" aren't
						   affected by frequency
						   transitions */
#define CPUFREQ_PM_NO_WARN	(1 << 2)	/* don't warn on suspend/resume
						   speed mismatches */

/*
 * This should be set by platforms having multiple clock-domains, i.e.
 * supporting multiple policies. With this sysfs directories of governor would
 * be created in cpu/cpu<num>/cpufreq/ directory and so they can use the same
 * governor with different tunables for different clusters.
 */
#define CPUFREQ_HAVE_GOVERNOR_PER_POLICY (1 << 3)

/*
 * Driver will do POSTCHANGE notifications from outside of their ->target()
 * routine and so must set cpufreq_driver->flags with this flag, so that core
 * can handle them specially.
 */
#define CPUFREQ_ASYNC_NOTIFICATION  (1 << 4)

int cpufreq_register_driver(struct cpufreq_driver *driver_data);
int cpufreq_unregister_driver(struct cpufreq_driver *driver_data);

const char *cpufreq_get_current_driver(void);

void cpufreq_notify_utilization(struct cpufreq_policy *policy,
		unsigned int load);

static inline void cpufreq_verify_within_limits(struct cpufreq_policy *policy, unsigned int min, unsigned int max)
{
#ifdef CONFIG_CPUFREQ_HARDLIMIT
	#ifdef CONFIG_CPUFREQ_HARDLIMIT_DEBUG
	pr_info("[HARDLIMIT] cpufreq.h verify_within_limits : min = %u / max = %u / new_min = %u / new_max = %u \n",
			min,
			max,
			check_cpufreq_hardlimit(min),
			check_cpufreq_hardlimit(max)
		);
	#endif
	 /* Yank555.lu - Enforce hardlimit */
	min = check_cpufreq_hardlimit(min);
	max = check_cpufreq_hardlimit(max);
#endif
	if (policy->min < min)
		policy->min = min;
	if (policy->max < min)
		policy->max = min;
	if (policy->min > max)
		policy->min = max;
	if (policy->max > max)
		policy->max = max;
	if (policy->min > policy->max)
		policy->min = policy->max;
	return;
}

static inline void
cpufreq_verify_within_cpu_limits(struct cpufreq_policy *policy)
{
	cpufreq_verify_within_limits(policy, policy->cpuinfo.min_freq,
			policy->cpuinfo.max_freq);
}

/*********************************************************************
 *                     CPUFREQ NOTIFIER INTERFACE                    *
 *********************************************************************/

#define CPUFREQ_TRANSITION_NOTIFIER	(0)
#define CPUFREQ_POLICY_NOTIFIER		(1)

/* Transition notifiers */
#define CPUFREQ_PRECHANGE		(0)
#define CPUFREQ_POSTCHANGE		(1)
#define CPUFREQ_RESUMECHANGE		(8)
#define CPUFREQ_SUSPENDCHANGE		(9)

/* Policy Notifiers  */
#define CPUFREQ_ADJUST			(0)
#define CPUFREQ_INCOMPATIBLE		(1)
#define CPUFREQ_NOTIFY			(2)
#define CPUFREQ_START			(3)
#define CPUFREQ_UPDATE_POLICY_CPU	(4)
#define CPUFREQ_CREATE_POLICY		(5)
#define CPUFREQ_REMOVE_POLICY		(6)

#ifdef CONFIG_CPU_FREQ
int cpufreq_register_notifier(struct notifier_block *nb, unsigned int list);
int cpufreq_unregister_notifier(struct notifier_block *nb, unsigned int list);

void cpufreq_notify_transition(struct cpufreq_policy *policy,
		struct cpufreq_freqs *freqs, unsigned int state);

#else /* CONFIG_CPU_FREQ */
static inline int cpufreq_register_notifier(struct notifier_block *nb,
						unsigned int list)
{
	return 0;
}
static inline int cpufreq_unregister_notifier(struct notifier_block *nb,
						unsigned int list)
{
	return 0;
}
#endif /* !CONFIG_CPU_FREQ */

/**
 * cpufreq_scale - "old * mult / div" calculation for large values (32-bit-arch
 * safe)
 * @old:   old value
 * @div:   divisor
 * @mult:  multiplier
 *
 *
 * new = old * mult / div
 */
static inline unsigned long cpufreq_scale(unsigned long old, u_int div,
		u_int mult)
{
#if BITS_PER_LONG == 32
	u64 result = ((u64) old) * ((u64) mult);
	do_div(result, div);
	return (unsigned long) result;

#elif BITS_PER_LONG == 64
	unsigned long result = old * ((u64) mult);
	result /= div;
	return result;
#endif
}

/*********************************************************************
 *                          CPUFREQ GOVERNORS                        *
 *********************************************************************/

/*
 * If (cpufreq_driver->target) exists, the ->governor decides what frequency
 * within the limits is used. If (cpufreq_driver->setpolicy> exists, these
 * two generic policies are available:
 */
#define CPUFREQ_POLICY_POWERSAVE	(1)
#define CPUFREQ_POLICY_PERFORMANCE	(2)

/* Governor Events */
#define CPUFREQ_GOV_START	1
#define CPUFREQ_GOV_STOP	2
#define CPUFREQ_GOV_LIMITS	3
#define CPUFREQ_GOV_POLICY_INIT	4
#define CPUFREQ_GOV_POLICY_EXIT	5

struct cpufreq_governor {
	char	name[CPUFREQ_NAME_LEN];
	int	initialized;
	int	(*governor)	(struct cpufreq_policy *policy,
				 unsigned int event);
	ssize_t	(*show_setspeed)	(struct cpufreq_policy *policy,
					 char *buf);
	int	(*store_setspeed)	(struct cpufreq_policy *policy,
					 unsigned int freq);
	unsigned int max_transition_latency; /* HW must be able to switch to
			next freq faster than this value in nano secs or we
			will fallback to performance governor */
	struct list_head	governor_list;
	struct module		*owner;
};

/* Pass a target to the cpufreq driver */
int cpufreq_driver_target(struct cpufreq_policy *policy,
				 unsigned int target_freq,
				 unsigned int relation);
int __cpufreq_driver_target(struct cpufreq_policy *policy,
				   unsigned int target_freq,
				   unsigned int relation);
int cpufreq_register_governor(struct cpufreq_governor *governor);
void cpufreq_unregister_governor(struct cpufreq_governor *governor);

/* CPUFREQ DEFAULT GOVERNOR */
/*
 * Performance governor is fallback governor if any other gov failed to auto
 * load due latency restrictions
 */
#ifdef CONFIG_CPU_FREQ_GOV_PERFORMANCE
extern struct cpufreq_governor cpufreq_gov_performance;
#endif
#ifdef CONFIG_CPU_FREQ_GOV_CAFACTIVE
extern unsigned int cpufreq_cafactive_get_hispeed_freq(int cpu);
extern void cafactive_boost_ondemand(int cpu, s64 miliseconds, bool static_switch);
#endif
#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_PERFORMANCE
#define CPUFREQ_DEFAULT_GOVERNOR	(&cpufreq_gov_performance)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_POWERSAVE)
extern struct cpufreq_governor cpufreq_gov_powersave;
#define CPUFREQ_DEFAULT_GOVERNOR	(&cpufreq_gov_powersave)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_USERSPACE)
extern struct cpufreq_governor cpufreq_gov_userspace;
#define CPUFREQ_DEFAULT_GOVERNOR	(&cpufreq_gov_userspace)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_ONDEMAND)
extern struct cpufreq_governor cpufreq_gov_ondemand;
#define CPUFREQ_DEFAULT_GOVERNOR	(&cpufreq_gov_ondemand)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_STOCKDEMAND)
extern struct cpufreq_governor cpufreq_gov_stockdemand;
#define CPUFREQ_DEFAULT_GOVERNOR	(&cpufreq_gov_stockdemand)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_CONSERVATIVE)
extern struct cpufreq_governor cpufreq_gov_conservative;
#define CPUFREQ_DEFAULT_GOVERNOR	(&cpufreq_gov_conservative)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_INTERACTIVE)
extern struct cpufreq_governor cpufreq_gov_interactive;
#define CPUFREQ_DEFAULT_GOVERNOR	(&cpufreq_gov_interactive)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_ELECTROACTIVE)
extern struct cpufreq_governor cpufreq_gov_electroactive;
#define CPUFREQ_DEFAULT_GOVERNOR	(&cpufreq_gov_electroactive)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_ELECTRODEMAND)
extern struct cpufreq_governor cpufreq_gov_electrodemand;
#define CPUFREQ_DEFAULT_GOVERNOR	(&cpufreq_gov_electrodemand)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_INTELLIACTIVE)
extern struct cpufreq_governor cpufreq_gov_intelliactive;
#define CPUFREQ_DEFAULT_GOVERNOR        (&cpufreq_gov_intelliactive)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_INTELLIDEMAND)
extern struct cpufreq_governor cpufreq_gov_intellidemand;
#define CPUFREQ_DEFAULT_GOVERNOR        (&cpufreq_gov_intellidemand)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_ABYSSPLUG)
extern struct cpufreq_governor cpufreq_gov_abyssplug;
#define CPUFREQ_DEFAULT_GOVERNOR        (&cpufreq_gov_abyssplug)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_ASSWAX)
extern struct cpufreq_governor cpufreq_gov_asswax;
#define CPUFREQ_DEFAULT_GOVERNOR        (&cpufreq_gov_asswax)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_DYNAMIC_INTERACTIVE)
extern struct cpufreq_governor cpufreq_gov_dynamic_interactive;
#define CPUFREQ_DEFAULT_GOVERNOR (&cpufreq_gov_dynamic_interactive)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_BADASS)
extern struct cpufreq_governor cpufreq_gov_badass;
#define CPUFREQ_DEFAULT_GOVERNOR        (&cpufreq_gov_badass)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_DANCEDANCE)
extern struct cpufreq_governor cpufreq_gov_dancedance;
#define CPUFREQ_DEFAULT_GOVERNOR        (&cpufreq_gov_dancedance)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_LIGHTNING)
extern struct cpufreq_governor cpufreq_gov_lightning;
#define CPUFREQ_DEFAULT_GOVERNOR (&cpufreq_gov_lightning)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_LIONFISH)
extern struct cpufreq_governor cpufreq_gov_lionfish;
#define CPUFREQ_DEFAULT_GOVERNOR (&cpufreq_gov_lionfish)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_NIGHTMARE)
extern struct cpufreq_governor cpufreq_gov_nightmare;
#define CPUFREQ_DEFAULT_GOVERNOR        (&cpufreq_gov_nightmare)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_SMARTASSH3)
extern struct cpufreq_governor cpufreq_gov_smartassH3;
#define CPUFREQ_DEFAULT_GOVERNOR        (&cpufreq_gov_smartassH3)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_TRIPNDROID)
extern struct cpufreq_governor cpufreq_gov_tripndroid;
#define CPUFREQ_DEFAULT_GOVERNOR (&cpufreq_gov_tripndroid)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_RAGINGMOLASSES)
extern struct cpufreq_governor cpufreq_gov_ragingmolasses;
#define CPUFREQ_DEFAULT_GOVERNOR (&cpufreq_gov_ragingmolasses)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_WHEATLEY)
extern struct cpufreq_governor cpufreq_gov_wheatley;
#define CPUFREQ_DEFAULT_GOVERNOR        (&cpufreq_gov_wheatley)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_LIONHEART)
extern struct cpufreq_governor cpufreq_gov_lionheart;
#define CPUFREQ_DEFAULT_GOVERNOR        (&cpufreq_gov_lionheart)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_DARKNESS)
extern struct cpufreq_governor cpufreq_gov_darkness;
#define CPUFREQ_DEFAULT_GOVERNOR        (&cpufreq_gov_darkness)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_YANKACTIVE)
extern struct cpufreq_governor cpufreq_gov_yankactive;
#define CPUFREQ_DEFAULT_GOVERNOR        (&cpufreq_gov_yankactive)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_YANKDEMAND)
extern struct cpufreq_governor cpufreq_gov_yankdemand;
#define CPUFREQ_DEFAULT_GOVERNOR	(&cpufreq_gov_yankdemand)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_IMPULSE)
extern struct cpufreq_governor cpufreq_gov_impulse;
#define CPUFREQ_DEFAULT_GOVERNOR	(&cpufreq_gov_impulse)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_BIOSHOCK)
extern struct cpufreq_governor cpufreq_gov_bioshock;
#define CPUFREQ_DEFAULT_GOVERNOR        (&cpufreq_gov_bioshock)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_SMARTMAX)
extern struct cpufreq_governor cpufreq_gov_smartmax;
#define CPUFREQ_DEFAULT_GOVERNOR	(&cpufreq_gov_smartmax)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_ZZMOOVE)
extern struct cpufreq_governor cpufreq_gov_zzmoove;
#define CPUFREQ_DEFAULT_GOVERNOR	(&cpufreq_gov_zzmoove)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_PEGASUSQ)
extern struct cpufreq_governor cpufreq_gov_pegasusq;
#define CPUFREQ_DEFAULT_GOVERNOR	(&cpufreq_gov_pegasusq)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_SMARTASSV2)
extern struct cpufreq_governor cpufreq_gov_smartass2;
#define CPUFREQ_DEFAULT_GOVERNOR	(&cpufreq_gov_smartass2)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_ONDEMANDPLUS)
extern struct cpufreq_governor cpufreq_gov_ondemandplus;
#define CPUFREQ_DEFAULT_GOVERNOR	(&cpufreq_gov_ondemandplus)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_HYPERX)
extern struct cpufreq_governor cpufreq_gov_hyperx;
#define CPUFREQ_DEFAULT_GOVERNOR (&cpufreq_gov_hyperx)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_HYPER)
extern struct cpufreq_governor cpufreq_gov_HYPER;
#define CPUFREQ_DEFAULT_GOVERNOR	(&cpufreq_gov_HYPER)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_BLU_ACTIVE)
extern struct cpufreq_governor cpufreq_gov_blu_active;
#define CPUFREQ_DEFAULT_GOVERNOR	(&cpufreq_gov_blu_active)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_OPTIMAX)
extern struct cpufreq_governor cpufreq_gov_optimax;
#define CPUFREQ_DEFAULT_GOVERNOR	(&cpufreq_gov_optimax)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_BARRY_ALLEN)
extern struct cpufreq_governor cpufreq_gov_barry_allen;
#define CPUFREQ_DEFAULT_GOVERNOR	(&cpufreq_gov_barry_allen)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_ARTERACTIVE)
extern struct cpufreq_governor cpufreq_gov_arteractive;
#define CPUFREQ_DEFAULT_GOVERNOR	(&cpufreq_gov_arteractive)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_PRESERVATIVE)
extern struct cpufreq_governor cpufreq_gov_preservative;
#define CPUFREQ_DEFAULT_GOVERNOR	(&cpufreq_gov_preservative)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_TRIPNDROID)
extern struct cpufreq_governor cpufreq_gov_tripndroid;
#define CPUFREQ_DEFAULT_GOVERNOR	(&cpufreq_gov_tripndroid)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_ELEMENTALX)
extern struct cpufreq_governor cpufreq_gov_elementalx;
#define CPUFREQ_DEFAULT_GOVERNOR	(&cpufreq_gov_elementalx)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_LAZY)
extern struct cpufreq_governor cpufreq_gov_lazy;
#define CPUFREQ_DEFAULT_GOVERNOR	(&cpufreq_gov_lazy)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_UBERDEMAND)
extern struct cpufreq_governor cpufreq_gov_uberdemand;
#define CPUFREQ_DEFAULT_GOVERNOR	(&cpufreq_gov_uberdemand)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_LULZACTIVE)
extern struct cpufreq_governor cpufreq_gov_lulzactive;
#define CPUFREQ_DEFAULT_GOVERNOR  (&cpufreq_gov_lulzactive)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_MEDUSA)
extern struct cpufreq_governor cpufreq_gov_medusa;
#define CPUFREQ_DEFAULT_GOVERNOR  (&cpufreq_gov_medusa)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_BACON)
extern struct cpufreq_governor cpufreq_gov_bacon;
#define CPUFREQ_DEFAULT_GOVERNOR  (&cpufreq_gov_bacon)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_HELLSACTIVE)
extern struct cpufreq_governor cpufreq_gov_hellsactive;
#define CPUFREQ_DEFAULT_GOVERNOR  (&cpufreq_gov_hellsactive)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_MACHINACTIVE)
extern struct cpufreq_governor cpufreq_gov_machinactive;
#define CPUFREQ_DEFAULT_GOVERNOR  (&cpufreq_gov_machinactive)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_CAFACTIVE)
extern struct cpufreq_governor cpufreq_gov_cafactive;
#define CPUFREQ_DEFAULT_GOVERNOR	(&cpufreq_gov_cafactive)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_INTERACTIVE_X)
extern struct cpufreq_governor cpufreq_gov_interactive_x;
#define CPUFREQ_DEFAULT_GOVERNOR	(&cpufreq_gov_interactive_x)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_CONSERVATIVE_X)
extern struct cpufreq_governor cpufreq_gov_conservative_x;
#define CPUFREQ_DEFAULT_GOVERNOR	(&cpufreq_gov_conservative_x)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_DESPAIR)
extern struct cpufreq_governor cpufreq_gov_despair;
#define CPUFREQ_DEFAULT_GOVERNOR	(&cpufreq_gov_despair)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_ONDEMAND_X)
extern struct cpufreq_governor cpufreq_gov_ondemand_x;
#define CPUFREQ_DEFAULT_GOVERNOR	(&cpufreq_gov_ondemand_x)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_IRONACTIVE)
extern struct cpufreq_governor cpufreq_gov_ironactive;
#define CPUFREQ_DEFAULT_GOVERNOR	(&cpufreq_gov_ironactive)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_SMARTMAX_EPS)
extern struct cpufreq_governor cpufreq_gov_smartmax_eps;
#define CPUFREQ_DEFAULT_GOVERNOR        (&cpufreq_gov_smartmax_eps)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_XPERIENCE)
extern struct cpufreq_governor cpufreq_gov_xperience;
#define CPUFREQ_DEFAULT_GOVERNOR 	(&cpufreq_gov_xperience)
#elif defined(CONFIG_CPU_FREQ_DEFAULT_GOV_WHISKYACTIVE)
extern struct cpufreq_governor cpufreq_gov_whiskeyactive;
#define CPUFREQ_DEFAULT_GOVERNOR 	(&cpufreq_gov_whiskyactive)
#endif

/*********************************************************************
 *                     FREQUENCY TABLE HELPERS                       *
 *********************************************************************/

#define CPUFREQ_ENTRY_INVALID ~0
#define CPUFREQ_TABLE_END     ~1

struct cpufreq_frequency_table {
	unsigned int	driver_data; /* driver specific data, not used by core */
	unsigned int	frequency; /* kHz - doesn't need to be in ascending
				    * order */
};

int cpufreq_frequency_table_cpuinfo(struct cpufreq_policy *policy,
				    struct cpufreq_frequency_table *table);

int cpufreq_frequency_table_verify(struct cpufreq_policy *policy,
				   struct cpufreq_frequency_table *table);
int cpufreq_generic_frequency_table_verify(struct cpufreq_policy *policy);

int cpufreq_frequency_table_target(struct cpufreq_policy *policy,
				   struct cpufreq_frequency_table *table,
				   unsigned int target_freq,
				   unsigned int relation,
				   unsigned int *index);

void cpufreq_frequency_table_update_policy_cpu(struct cpufreq_policy *policy);
ssize_t cpufreq_show_cpus(const struct cpumask *mask, char *buf);

/* the following funtion is for cpufreq core use only */
struct cpufreq_frequency_table *cpufreq_frequency_get_table(unsigned int cpu);

/* the following are really really optional */
extern struct freq_attr cpufreq_freq_attr_scaling_available_freqs;
extern struct freq_attr *cpufreq_generic_attr[];
void cpufreq_frequency_table_get_attr(struct cpufreq_frequency_table *table,
				      unsigned int cpu);
void cpufreq_frequency_table_put_attr(unsigned int cpu);
int cpufreq_table_validate_and_show(struct cpufreq_policy *policy,
				      struct cpufreq_frequency_table *table);

int cpufreq_generic_init(struct cpufreq_policy *policy,
		struct cpufreq_frequency_table *table,
		unsigned int transition_latency);
static inline int cpufreq_generic_exit(struct cpufreq_policy *policy)
{
	cpufreq_frequency_table_put_attr(policy->cpu);
	return 0;
}

/*
 * The polling frequency depends on the capability of the processor. Default
 * polling frequency is 1000 times the transition latency of the processor. The
 * governor will work on any processor with transition latency <= 10ms, using
 * appropriate sampling rate.
 *
 * For CPUs with transition latency > 10ms (mostly drivers with CPUFREQ_ETERNAL)
 * this governor will not work. All times here are in us (micro seconds).
 */
#define MIN_SAMPLING_RATE_RATIO			(2)
#define LATENCY_MULTIPLIER			(1000)
#define MIN_LATENCY_MULTIPLIER			(20)
#define TRANSITION_LATENCY_LIMIT		(10 * 1000 * 1000)

/* Ondemand Sampling types */
enum {OD_NORMAL_SAMPLE, OD_SUB_SAMPLE};

/*
 * Macro for creating governors sysfs routines
 *
 * - gov_sys: One governor instance per whole system
 * - gov_pol: One governor instance per policy
 */

/* Create attributes */
#define gov_sys_attr_ro(_name)						\
static struct global_attr _name##_gov_sys =				\
__ATTR(_name, 0444, show_##_name##_gov_sys, NULL)

#define gov_sys_attr_rw(_name)						\
static struct global_attr _name##_gov_sys =				\
__ATTR(_name, 0644, show_##_name##_gov_sys, store_##_name##_gov_sys)

#define gov_pol_attr_ro(_name)						\
static struct freq_attr _name##_gov_pol =				\
__ATTR(_name, 0444, show_##_name##_gov_pol, NULL)

#define gov_pol_attr_rw(_name)						\
static struct freq_attr _name##_gov_pol =				\
__ATTR(_name, 0644, show_##_name##_gov_pol, store_##_name##_gov_pol)

#define gov_sys_pol_attr_rw(_name)					\
	gov_sys_attr_rw(_name);						\
	gov_pol_attr_rw(_name)

#define gov_sys_pol_attr_ro(_name)					\
	gov_sys_attr_ro(_name);						\
	gov_pol_attr_ro(_name)

/* Create show/store routines */
#define show_one(_gov, file_name)					\
static ssize_t show_##file_name##_gov_sys				\
(struct kobject *kobj, struct attribute *attr, char *buf)		\
{									\
	struct _gov##_dbs_tuners *tuners = _gov##_dbs_cdata.gdbs_data->tuners; \
	return sprintf(buf, "%u\n", tuners->file_name);			\
}									\
									\
static ssize_t show_##file_name##_gov_pol				\
(struct cpufreq_policy *policy, char *buf)				\
{									\
	struct dbs_data *dbs_data = policy->governor_data;		\
	struct _gov##_dbs_tuners *tuners = dbs_data->tuners;		\
	return sprintf(buf, "%u\n", tuners->file_name);			\
}

#define store_one(_gov, file_name)					\
static ssize_t store_##file_name##_gov_sys				\
(struct kobject *kobj, struct attribute *attr, const char *buf, size_t count) \
{									\
	struct dbs_data *dbs_data = _gov##_dbs_cdata.gdbs_data;		\
	return store_##file_name(dbs_data, buf, count);			\
}									\
									\
static ssize_t store_##file_name##_gov_pol				\
(struct cpufreq_policy *policy, const char *buf, size_t count)		\
{									\
	struct dbs_data *dbs_data = policy->governor_data;		\
	return store_##file_name(dbs_data, buf, count);			\
}

#define show_store_one(_gov, file_name)					\
show_one(_gov, file_name);						\
store_one(_gov, file_name)

/* create helper routines */
#define define_get_cpu_dbs_routines(_dbs_info)				\
static struct cpu_dbs_common_info *get_cpu_cdbs(int cpu)		\
{									\
	return &per_cpu(_dbs_info, cpu).cdbs;				\
}									\
									\
static void *get_cpu_dbs_info_s(int cpu)				\
{									\
	return &per_cpu(_dbs_info, cpu);				\
}

/*
 * Abbreviations:
 * dbs: used as a shortform for demand based switching It helps to keep variable
 *	names smaller, simpler
 * cdbs: common dbs
 * od_*: On-demand governor
 * cs_*: Conservative governor
 */

/* Per cpu structures */
struct cpu_dbs_common_info {
	int cpu;
	u64 prev_cpu_idle;
	u64 prev_cpu_wall;
	u64 prev_cpu_nice;
	struct cpufreq_policy *cur_policy;
	struct delayed_work work;
	/*
	 * percpu mutex that serializes governor limit change with gov_dbs_timer
	 * invocation. We do not want gov_dbs_timer to run when user is changing
	 * the governor or limits.
	 */
	struct mutex timer_mutex;
	ktime_t time_stamp;
};

struct od_cpu_dbs_info_s {
	struct cpu_dbs_common_info cdbs;
	struct cpufreq_frequency_table *freq_table;
	unsigned int freq_lo;
	unsigned int freq_lo_jiffies;
	unsigned int freq_hi_jiffies;
	unsigned int rate_mult;
	unsigned int sample_type:1;
};

struct cs_cpu_dbs_info_s {
	struct cpu_dbs_common_info cdbs;
	unsigned int down_skip;
	unsigned int requested_freq;
	unsigned int enable:1;
};

/* Per policy Governors sysfs tunables */
struct od_dbs_tuners {
	unsigned int ignore_nice_load;
	unsigned int sampling_rate;
	unsigned int sampling_down_factor;
	unsigned int up_threshold;
	unsigned int powersave_bias;
	unsigned int io_is_busy;
};

struct cs_dbs_tuners {
	unsigned int ignore_nice_load;
	unsigned int sampling_rate;
	unsigned int sampling_down_factor;
	unsigned int up_threshold;
	unsigned int down_threshold;
	unsigned int freq_step;
};

/* Common Governor data across policies */
struct dbs_data;
struct common_dbs_data {
	/* Common across governors */
	#define GOV_ONDEMAND		0
	#define GOV_CONSERVATIVE	1
	int governor;
	struct attribute_group *attr_group_gov_sys; /* one governor - system */
	struct attribute_group *attr_group_gov_pol; /* one governor - policy */

	/*
	 * Common data for platforms that don't set
	 * CPUFREQ_HAVE_GOVERNOR_PER_POLICY
	 */
	struct dbs_data *gdbs_data;

	struct cpu_dbs_common_info *(*get_cpu_cdbs)(int cpu);
	void *(*get_cpu_dbs_info_s)(int cpu);
	void (*gov_dbs_timer)(struct work_struct *work);
	void (*gov_check_cpu)(int cpu, unsigned int load);
	int (*init)(struct dbs_data *dbs_data);
	void (*exit)(struct dbs_data *dbs_data);

	/* Governor specific ops, see below */
	void *gov_ops;
};

/* Governor Per policy data */
struct dbs_data {
	struct common_dbs_data *cdata;
	unsigned int min_sampling_rate;
	int usage_count;
	void *tuners;

	/* dbs_mutex protects dbs_enable in governor start/stop */
	struct mutex mutex;
};

/* Governor specific ops, will be passed to dbs_data->gov_ops */
struct od_ops {
	void (*powersave_bias_init_cpu)(int cpu);
	unsigned int (*powersave_bias_target)(struct cpufreq_policy *policy,
			unsigned int freq_next, unsigned int relation);
	void (*freq_increase)(struct cpufreq_policy *policy, unsigned int freq);
};

struct cs_ops {
	struct notifier_block *notifier_block;
};

static inline int delay_for_sampling_rate(unsigned int sampling_rate)
{
	int delay = usecs_to_jiffies(sampling_rate);

	/* We want all CPUs to do sampling nearly on same jiffy */
	if (num_online_cpus() > 1)
		delay -= jiffies % delay;

	return delay;
}

#define declare_show_sampling_rate_min(_gov)				\
static ssize_t show_sampling_rate_min_gov_sys				\
(struct kobject *kobj, struct attribute *attr, char *buf)		\
{									\
	struct dbs_data *dbs_data = _gov##_dbs_cdata.gdbs_data;		\
	return sprintf(buf, "%u\n", dbs_data->min_sampling_rate);	\
}									\
									\
static ssize_t show_sampling_rate_min_gov_pol				\
(struct cpufreq_policy *policy, char *buf)				\
{									\
	struct dbs_data *dbs_data = policy->governor_data;		\
	return sprintf(buf, "%u\n", dbs_data->min_sampling_rate);	\
}

void dbs_check_cpu(struct dbs_data *dbs_data, int cpu);
bool need_load_eval(struct cpu_dbs_common_info *cdbs,
		unsigned int sampling_rate);
int cpufreq_governor_dbs(struct cpufreq_policy *policy,
		struct common_dbs_data *cdata, unsigned int event);
void gov_queue_work(struct dbs_data *dbs_data, struct cpufreq_policy *policy,
		unsigned int delay, bool all_cpus);
void od_register_powersave_bias_handler(unsigned int (*f)
		(struct cpufreq_policy *, unsigned int, unsigned int),
		unsigned int powersave_bias);
void od_unregister_powersave_bias_handler(void);

#endif /* _LINUX_CPUFREQ_H */
