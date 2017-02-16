/* Time spent by the tasks of the cpu accounting group executing in ... */
enum cpuacct_stat_index {
	CPUACCT_STAT_USER,	/* ... user mode */
	CPUACCT_STAT_SYSTEM,	/* ... kernel mode */

	CPUACCT_STAT_NSTATS,
};

#ifdef CONFIG_CGROUP_CPUACCT

#include <linux/cgroup.h>
/* track cpu usage of a group of tasks and its child groups */
struct cpuacct {
	struct cgroup_subsys_state css;
	/* cpuusage holds pointer to a u64-type object on every cpu */
	u64 __percpu *cpuusage;
	struct kernel_cpustat __percpu *cpustat;
};

extern struct cgroup_subsys cpuacct_subsys;
extern struct cpuacct root_cpuacct;

/* return cpu accounting group corresponding to this container */
static inline struct cpuacct *cgroup_ca(struct cgroup *cgrp)
{
	return container_of(cgroup_subsys_state(cgrp, cpuacct_subsys_id),
			    struct cpuacct, css);
}

/* return cpu accounting group to which this task belongs */
static inline struct cpuacct *task_ca(struct task_struct *tsk)
{
	return container_of(task_subsys_state(tsk, cpuacct_subsys_id),
			    struct cpuacct, css);
}

static inline struct cpuacct *parent_ca(struct cpuacct *ca)
{
	if (!ca || !ca->css.cgroup->parent)
		return NULL;
	return cgroup_ca(ca->css.cgroup->parent);
}

extern void cpuacct_charge(struct task_struct *tsk, u64 cputime);

#else

static inline void cpuacct_charge(struct task_struct *tsk, u64 cputime)
{
}

#endif
