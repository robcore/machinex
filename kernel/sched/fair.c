/*
 * Completely Fair Scheduling (CFS) Class (SCHED_NORMAL/SCHED_BATCH)
 *
 *  Copyright (C) 2007 Red Hat, Inc., Ingo Molnar <mingo@redhat.com>
 *
 *  Interactivity improvements by Mike Galbraith
 *  (C) 2007 Mike Galbraith <efault@gmx.de>
 *
 *  Various enhancements by Dmitry Adamushko.
 *  (C) 2007 Dmitry Adamushko <dmitry.adamushko@gmail.com>
 *
 *  Group scheduling enhancements by Srivatsa Vaddagiri
 *  Copyright IBM Corporation, 2007
 *  Author: Srivatsa Vaddagiri <vatsa@linux.vnet.ibm.com>
 *
 *  Scaled math optimizations by Thomas Gleixner
 *  Copyright (C) 2007, Thomas Gleixner <tglx@linutronix.de>
 *
 *  Adaptive scheduling granularity, math enhancements by Peter Zijlstra
 *  Copyright (C) 2007 Red Hat, Inc., Peter Zijlstra <pzijlstr@redhat.com>
 */

#include <linux/latencytop.h>
#include <linux/sched.h>
#include <linux/cpumask.h>
#include <linux/cpuidle.h>
#include <linux/slab.h>
#include <linux/profile.h>
#include <linux/interrupt.h>
#include <linux/mempolicy.h>
#include <linux/migrate.h>
#include <linux/task_work.h>
#include <linux/module.h>

#include <trace/events/sched.h>

#include "sched.h"
#include "tune.h"
#include "walt.h"

/*
 * Targeted preemption latency for CPU-bound tasks:
 * (default: 6ms * (1 + ilog(ncpus)), units: nanoseconds)
 *
 * NOTE: this latency value is not the same as the concept of
 * 'timeslice length' - timeslices in CFS are of variable length
 * and have no persistent notion like in traditional, time-slice
 * based scheduling concepts.
 *
 * (to see the precise effective timeslice length of your workload,
 *  run vmstat and monitor the context-switches (cs) field)
 */
unsigned int sysctl_sched_latency = 6000000ULL;
unsigned int normalized_sysctl_sched_latency = 6000000ULL;

unsigned int sysctl_sched_is_big_little = 0;
unsigned int sysctl_sched_sync_hint_enable = 1;
unsigned int sysctl_sched_initial_task_util = 0;
unsigned int sysctl_sched_cstate_aware = 1;

#ifdef CONFIG_SCHED_WALT
unsigned int sysctl_sched_use_walt_cpu_util = 1;
unsigned int sysctl_sched_use_walt_task_util = 1;
__read_mostly unsigned int sysctl_sched_walt_cpu_high_irqload =
    (10 * NSEC_PER_MSEC);
#endif
/*
 * The initial- and re-scaling of tunables is configurable
 * (default SCHED_TUNABLESCALING_LOG = *(1+ilog(ncpus))
 *
 * Options are:
 * SCHED_TUNABLESCALING_NONE - unscaled, always *1
 * SCHED_TUNABLESCALING_LOG - scaled logarithmical, *1+ilog(ncpus)
 * SCHED_TUNABLESCALING_LINEAR - scaled linear, *ncpus
 */
enum sched_tunable_scaling sysctl_sched_tunable_scaling
	= SCHED_TUNABLESCALING_LOG;

/*
 * Minimal preemption granularity for CPU-bound tasks:
 * (default: 0.75 msec * (1 + ilog(ncpus)), units: nanoseconds)
 */
unsigned int sysctl_sched_min_granularity = 750000ULL;
unsigned int normalized_sysctl_sched_min_granularity = 750000ULL;

/*
 * is kept at sysctl_sched_latency / sysctl_sched_min_granularity
 */
static unsigned int sched_nr_latency = 8;

/*
 * After fork, child runs first. If set to 0 (default) then
 * parent will (try to) run first.
 */
unsigned int sysctl_sched_child_runs_first __read_mostly;

/*
 * Controls whether, when SD_SHARE_PKG_RESOURCES is on, if all
 * tasks go to idle CPUs when woken. If this is off, note that the
 * per-task flag PF_WAKE_ON_IDLE can still cause a task to go to an
 * idle CPU upon being woken.
 */
unsigned int __read_mostly sysctl_sched_wake_to_idle;

/*
 * SCHED_OTHER wake-up granularity.
 * (default: 1 msec * (1 + ilog(ncpus)), units: nanoseconds)
 *
 * This option delays the preemption effects of decoupled workloads
 * and reduces their over-scheduling. Synchronous workloads will still
 * have immediate wakeup/sleep latencies.
 */
unsigned int sysctl_sched_wakeup_granularity = 1000000UL;
unsigned int normalized_sysctl_sched_wakeup_granularity = 1000000UL;

const_debug unsigned int sysctl_sched_migration_cost = 500000UL;

#ifdef CONFIG_CFS_BANDWIDTH
/*
 * Amount of runtime to allocate from global (tg) to local (per-cfs_rq) pool
 * each time a cfs_rq requests quota.
 *
 * Note: in the case that the slice exceeds the runtime remaining (either due
 * to consumption or the quota being specified to be smaller than the slice)
 * we will always only issue the remaining available time.
 *
 * default: 5 msec, units: microseconds
  */
unsigned int sysctl_sched_cfs_bandwidth_slice = 5000UL;
#endif

static inline void update_load_add(struct load_weight *lw, unsigned long inc)
{
	lw->weight += inc;
	lw->inv_weight = 0;
}

static inline void update_load_sub(struct load_weight *lw, unsigned long dec)
{
	lw->weight -= dec;
	lw->inv_weight = 0;
}

static inline void update_load_set(struct load_weight *lw, unsigned long w)
{
	lw->weight = w;
	lw->inv_weight = 0;
}

/*
 * Increase the granularity value when there are more CPUs,
 * because with more CPUs the 'effective latency' as visible
 * to users decreases. But the relationship is not linear,
 * so pick a second-best guess by going with the log2 of the
 * number of CPUs.
 *
 * This idea comes from the SD scheduler of Con Kolivas:
 */
static unsigned int get_update_sysctl_factor(void)
{
	unsigned int cpus = min_t(unsigned int, num_online_cpus(), 4);
	unsigned int factor;

	switch (sysctl_sched_tunable_scaling) {
	case SCHED_TUNABLESCALING_NONE:
		factor = 1;
		break;
	case SCHED_TUNABLESCALING_LINEAR:
		factor = cpus;
		break;
	case SCHED_TUNABLESCALING_LOG:
	default:
		factor = 1 + ilog2(cpus);
		break;
	}

	return factor;
}

static void update_sysctl(void)
{
	unsigned int factor = get_update_sysctl_factor();

#define SET_SYSCTL(name) \
	(sysctl_##name = (factor) * normalized_sysctl_##name)
	SET_SYSCTL(sched_min_granularity);
	SET_SYSCTL(sched_latency);
	SET_SYSCTL(sched_wakeup_granularity);
#undef SET_SYSCTL
}

void sched_init_granularity(void)
{
	update_sysctl();
}

#define WMULT_CONST	(~0U)
#define WMULT_SHIFT	32

static void __update_inv_weight(struct load_weight *lw)
{
	unsigned long w;

	if (likely(lw->inv_weight))
		return;

	w = scale_load_down(lw->weight);

	if (BITS_PER_LONG > 32 && unlikely(w >= WMULT_CONST))
		lw->inv_weight = 1;
	else if (unlikely(!w))
		lw->inv_weight = WMULT_CONST;
	else
		lw->inv_weight = WMULT_CONST / w;
}

/*
 * delta_exec * weight / lw.weight
 *   OR
 * (delta_exec * (weight * lw->inv_weight)) >> WMULT_SHIFT
 *
 * Either weight := NICE_0_LOAD and lw \e sched_prio_to_wmult[], in which case
 * we're guaranteed shift stays positive because inv_weight is guaranteed to
 * fit 32 bits, and NICE_0_LOAD gives another 10 bits; therefore shift >= 22.
 *
 * Or, weight =< lw.weight (because lw.weight is the runqueue weight), thus
 * weight/lw.weight <= 1, and therefore our shift will also be positive.
 */
static u64 __calc_delta(u64 delta_exec, unsigned long weight, struct load_weight *lw)
{
	u64 fact = scale_load_down(weight);
	int shift = WMULT_SHIFT;

	__update_inv_weight(lw);

	if (unlikely(fact >> 32)) {
		while (fact >> 32) {
			fact >>= 1;
			shift--;
		}
	}

	/* hint to use a 32x32->64 mul */
	fact = (u64)(u32)fact * lw->inv_weight;

	while (fact >> 32) {
		fact >>= 1;
		shift--;
	}

	return mul_u64_u32_shr(delta_exec, fact, shift);
}


const struct sched_class fair_sched_class;

/**************************************************************
 * CFS operations on generic schedulable entities:
 */

#ifdef CONFIG_FAIR_GROUP_SCHED

/* cpu runqueue to which this cfs_rq is attached */
static inline struct rq *rq_of(struct cfs_rq *cfs_rq)
{
	return cfs_rq->rq;
}

/* An entity is a task if it doesn't "own" a runqueue */
#define entity_is_task(se)	(!se->my_q)

static inline struct task_struct *task_of(struct sched_entity *se)
{
#ifdef CONFIG_SCHED_DEBUG
	WARN_ON_ONCE(!entity_is_task(se));
#endif
	return container_of(se, struct task_struct, se);
}

/* Walk up scheduling entities hierarchy */
#define for_each_sched_entity(se) \
		for (; se; se = se->parent)

static inline struct cfs_rq *task_cfs_rq(struct task_struct *p)
{
	return p->se.cfs_rq;
}

/* runqueue on which this entity is (to be) queued */
static inline struct cfs_rq *cfs_rq_of(struct sched_entity *se)
{
	return se->cfs_rq;
}

/* runqueue "owned" by this group */
static inline struct cfs_rq *group_cfs_rq(struct sched_entity *grp)
{
	return grp->my_q;
}

static inline void list_add_leaf_cfs_rq(struct cfs_rq *cfs_rq)
{
	if (!cfs_rq->on_list) {
		/*
		 * Ensure we either appear before our parent (if already
		 * enqueued) or force our parent to appear after us when it is
		 * enqueued.  The fact that we always enqueue bottom-up
		 * reduces this to two cases.
		 */
		if (cfs_rq->tg->parent &&
		    cfs_rq->tg->parent->cfs_rq[cpu_of(rq_of(cfs_rq))]->on_list) {
			list_add_rcu(&cfs_rq->leaf_cfs_rq_list,
				&rq_of(cfs_rq)->leaf_cfs_rq_list);
		} else {
			list_add_tail_rcu(&cfs_rq->leaf_cfs_rq_list,
				&rq_of(cfs_rq)->leaf_cfs_rq_list);
		}

		cfs_rq->on_list = 1;
	}
}

static inline void list_del_leaf_cfs_rq(struct cfs_rq *cfs_rq)
{
	if (cfs_rq->on_list) {
		list_del_rcu(&cfs_rq->leaf_cfs_rq_list);
		cfs_rq->on_list = 0;
	}
}

/* Iterate thr' all leaf cfs_rq's on a runqueue */
#define for_each_leaf_cfs_rq(rq, cfs_rq) \
	list_for_each_entry_rcu(cfs_rq, &rq->leaf_cfs_rq_list, leaf_cfs_rq_list)

/* Do the two (enqueued) entities belong to the same group ? */
static inline struct cfs_rq *
is_same_group(struct sched_entity *se, struct sched_entity *pse)
{
	if (se->cfs_rq == pse->cfs_rq)
		return se->cfs_rq;

	return NULL;
}

static inline struct sched_entity *parent_entity(struct sched_entity *se)
{
	return se->parent;
}

static void
find_matching_se(struct sched_entity **se, struct sched_entity **pse)
{
	int se_depth, pse_depth;

	/*
	 * preemption test can be made between sibling entities who are in the
	 * same cfs_rq i.e who have a common parent. Walk up the hierarchy of
	 * both tasks until we find their ancestors who are siblings of common
	 * parent.
	 */

	/* First walk up until both entities are at same depth */
	se_depth = (*se)->depth;
	pse_depth = (*pse)->depth;

	while (se_depth > pse_depth) {
		se_depth--;
		*se = parent_entity(*se);
	}

	while (pse_depth > se_depth) {
		pse_depth--;
		*pse = parent_entity(*pse);
	}

	while (!is_same_group(*se, *pse)) {
		*se = parent_entity(*se);
		*pse = parent_entity(*pse);
	}
}

#else	/* !CONFIG_FAIR_GROUP_SCHED */

static inline struct task_struct *task_of(struct sched_entity *se)
{
	return container_of(se, struct task_struct, se);
}

static inline struct rq *rq_of(struct cfs_rq *cfs_rq)
{
	return container_of(cfs_rq, struct rq, cfs);
}

#define entity_is_task(se)	1

#define for_each_sched_entity(se) \
		for (; se; se = NULL)

static inline struct cfs_rq *task_cfs_rq(struct task_struct *p)
{
	return &task_rq(p)->cfs;
}

static inline struct cfs_rq *cfs_rq_of(struct sched_entity *se)
{
	struct task_struct *p = task_of(se);
	struct rq *rq = task_rq(p);

	return &rq->cfs;
}

/* runqueue "owned" by this group */
static inline struct cfs_rq *group_cfs_rq(struct sched_entity *grp)
{
	return NULL;
}

static inline void list_add_leaf_cfs_rq(struct cfs_rq *cfs_rq)
{
}

static inline void list_del_leaf_cfs_rq(struct cfs_rq *cfs_rq)
{
}

#define for_each_leaf_cfs_rq(rq, cfs_rq) \
		for (cfs_rq = &rq->cfs; cfs_rq; cfs_rq = NULL)

static inline struct cfs_rq *
is_same_group(struct sched_entity *se, struct sched_entity *pse)
{
	return cfs_rq_of(se); /* always the same rq */
}

static inline struct sched_entity *parent_entity(struct sched_entity *se)
{
	return NULL;
}

static inline void
find_matching_se(struct sched_entity **se, struct sched_entity **pse)
{
}

#endif	/* CONFIG_FAIR_GROUP_SCHED */

static __always_inline
void account_cfs_rq_runtime(struct cfs_rq *cfs_rq, u64 delta_exec);

/**************************************************************
 * Scheduling class tree data structure manipulation methods:
 */

static inline u64 max_vruntime(u64 max_vruntime, u64 vruntime)
{
	s64 delta = (s64)(vruntime - max_vruntime);
	if (delta > 0)
		max_vruntime = vruntime;

	return max_vruntime;
}

static inline u64 min_vruntime(u64 min_vruntime, u64 vruntime)
{
	s64 delta = (s64)(vruntime - min_vruntime);
	if (delta < 0)
		min_vruntime = vruntime;

	return min_vruntime;
}

static inline int entity_before(struct sched_entity *a,
				struct sched_entity *b)
{
	return (s64)(a->vruntime - b->vruntime) < 0;
}

static void update_min_vruntime(struct cfs_rq *cfs_rq)
{
	struct sched_entity *curr = cfs_rq->curr;

	u64 vruntime = cfs_rq->min_vruntime;

	if (curr) {
		if (curr->on_rq)
			vruntime = curr->vruntime;
		else
			curr = NULL;
	}

	if (cfs_rq->rb_leftmost) {
		struct sched_entity *se = rb_entry(cfs_rq->rb_leftmost,
						   struct sched_entity,
						   run_node);

		if (!curr)
			vruntime = se->vruntime;
		else
			vruntime = min_vruntime(vruntime, se->vruntime);
	}

	/* ensure we never gain time by being placed backwards. */
	cfs_rq->min_vruntime = max_vruntime(cfs_rq->min_vruntime, vruntime);
#ifndef CONFIG_64BIT
	smp_wmb();
	cfs_rq->min_vruntime_copy = cfs_rq->min_vruntime;
#endif
}

/*
 * Enqueue an entity into the rb-tree:
 */
static void __enqueue_entity(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	struct rb_node **link = &cfs_rq->tasks_timeline.rb_node;
	struct rb_node *parent = NULL;
	struct sched_entity *entry;
	int leftmost = 1;

	/*
	 * Find the right place in the rbtree:
	 */
	while (*link) {
		parent = *link;
		entry = rb_entry(parent, struct sched_entity, run_node);
		/*
		 * We dont care about collisions. Nodes with
		 * the same key stay together.
		 */
		if (entity_before(se, entry)) {
			link = &parent->rb_left;
		} else {
			link = &parent->rb_right;
			leftmost = 0;
		}
	}

	/*
	 * Maintain a cache of leftmost tree entries (it is frequently
	 * used):
	 */
	if (leftmost)
		cfs_rq->rb_leftmost = &se->run_node;

	rb_link_node(&se->run_node, parent, link);
	rb_insert_color(&se->run_node, &cfs_rq->tasks_timeline);
}

static void __dequeue_entity(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	if (cfs_rq->rb_leftmost == &se->run_node) {
		struct rb_node *next_node;

		next_node = rb_next(&se->run_node);
		cfs_rq->rb_leftmost = next_node;
	}

	rb_erase(&se->run_node, &cfs_rq->tasks_timeline);
}

struct sched_entity *__pick_first_entity(struct cfs_rq *cfs_rq)
{
	struct rb_node *left = cfs_rq->rb_leftmost;

	if (!left)
		return NULL;

	return rb_entry(left, struct sched_entity, run_node);
}

static struct sched_entity *__pick_next_entity(struct sched_entity *se)
{
	struct rb_node *next = rb_next(&se->run_node);

	if (!next)
		return NULL;

	return rb_entry(next, struct sched_entity, run_node);
}

#ifdef CONFIG_SCHED_DEBUG
struct sched_entity *__pick_last_entity(struct cfs_rq *cfs_rq)
{
	struct rb_node *last = rb_last(&cfs_rq->tasks_timeline);

	if (!last)
		return NULL;

	return rb_entry(last, struct sched_entity, run_node);
}

/**************************************************************
 * Scheduling class statistics methods:
 */

int sched_proc_update_handler(struct ctl_table *table, int write,
		void __user *buffer, size_t *lenp,
		loff_t *ppos)
{
	int ret = proc_dointvec_minmax(table, write, buffer, lenp, ppos);
	unsigned int factor = get_update_sysctl_factor();

	if (ret || !write)
		return ret;

	sched_nr_latency = DIV_ROUND_UP(sysctl_sched_latency,
					sysctl_sched_min_granularity);

#define WRT_SYSCTL(name) \
	(normalized_sysctl_##name = sysctl_##name / (factor))
	WRT_SYSCTL(sched_min_granularity);
	WRT_SYSCTL(sched_latency);
	WRT_SYSCTL(sched_wakeup_granularity);
#undef WRT_SYSCTL

	return 0;
}
#endif

/*
 * delta /= w
 */
static inline u64 calc_delta_fair(u64 delta, struct sched_entity *se)
{
	if (unlikely(se->load.weight != NICE_0_LOAD))
		delta = __calc_delta(delta, NICE_0_LOAD, &se->load);

	return delta;
}

/*
 * The idea is to set a period in which each task runs once.
 *
 * When there are too many tasks (sched_nr_latency) we have to stretch
 * this period because otherwise the slices get too small.
 *
 * p = (nr <= nl) ? l : l*nr/nl
 */
static u64 __sched_period(unsigned long nr_running)
{
	if (unlikely(nr_running > sched_nr_latency))
		return nr_running * sysctl_sched_min_granularity;
	else
		return sysctl_sched_latency;
}

/*
 * We calculate the wall-time slice from the period by taking a part
 * proportional to the weight.
 *
 * s = p*P[w/rw]
 */
static u64 sched_slice(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	u64 slice = __sched_period(cfs_rq->nr_running + !se->on_rq);

	for_each_sched_entity(se) {
		struct load_weight *load;
		struct load_weight lw;

		cfs_rq = cfs_rq_of(se);
		load = &cfs_rq->load;

		if (unlikely(!se->on_rq)) {
			lw = cfs_rq->load;

			update_load_add(&lw, se->load.weight);
			load = &lw;
		}
		slice = __calc_delta(slice, se->load.weight, load);
	}
	return slice;
}

/*
 * We calculate the vruntime slice of a to-be-inserted task.
 *
 * vs = s/w
 */
static u64 sched_vslice(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	return calc_delta_fair(sched_slice(cfs_rq, se), se);
}

#ifdef CONFIG_SMP
static int select_idle_sibling(struct task_struct *p, int prev_cpu, int cpu);
static unsigned long task_h_load(struct task_struct *p);

/*
 * We choose a half-life close to 1 scheduling period.
 * Note: The tables below are dependent on this value.
 */
#define LOAD_AVG_PERIOD 16
#define LOAD_AVG_MAX 24117 /* maximum possible load avg */
#define LOAD_AVG_MAX_N 172 /* number of full periods to produce LOAD_AVG_MAX */

/* Give new sched_entity start runnable values to heavy its load in infant time */
void init_entity_runnable_average(struct sched_entity *se)
{
	struct sched_avg *sa = &se->avg;

	sa->last_update_time = 0;
	/*
	 * sched_avg's period_contrib should be strictly less then 1024, so
	 * we give it 1023 to make sure it is almost a period (1024us), and
	 * will definitely be update (after enqueue).
	 */
	sa->period_contrib = 1023;
	/*
	 * Tasks are intialized with full load to be seen as heavy tasks until
	 * they get a chance to stabilize to their real load level.
	 * Group entities are intialized with zero load to reflect the fact that
	 * nothing has been attached to the task group yet.
	 */
	if (entity_is_task(se))
		sa->load_avg = scale_load_down(se->load.weight);
	sa->load_sum = sa->load_avg * LOAD_AVG_MAX;
	/*
	 * At this point, util_avg won't be used in select_task_rq_fair anyway
	 */
	sa->util_avg = 0;
	sa->util_sum = 0;
	/* when this task enqueue'ed, it will contribute to its cfs_rq's load_avg */
}

static inline u64 cfs_rq_clock_task(struct cfs_rq *cfs_rq);
static int update_cfs_rq_load_avg(u64 now, struct cfs_rq *cfs_rq);
static void update_tg_load_avg(struct cfs_rq *cfs_rq, int force);
static void attach_entity_load_avg(struct cfs_rq *cfs_rq, struct sched_entity *se);

/*
 * With new tasks being created, their initial util_avgs are extrapolated
 * based on the cfs_rq's current util_avg:
 *
 *   util_avg = cfs_rq->util_avg / (cfs_rq->load_avg + 1) * se.load.weight
 *
 * However, in many cases, the above util_avg does not give a desired
 * value. Moreover, the sum of the util_avgs may be divergent, such
 * as when the series is a harmonic series.
 *
 * To solve this problem, we also cap the util_avg of successive tasks to
 * only 1/2 of the left utilization budget:
 *
 *   util_avg_cap = (1024 - cfs_rq->avg.util_avg) / 2^n
 *
 * where n denotes the nth task.
 *
 * For example, a simplest series from the beginning would be like:
 *
 *  task  util_avg: 512, 256, 128,  64,  32,   16,    8, ...
 * cfs_rq util_avg: 512, 768, 896, 960, 992, 1008, 1016, ...
 *
 * Finally, that extrapolated util_avg is clamped to the cap (util_avg_cap)
 * if util_avg > util_avg_cap.
 */
void post_init_entity_util_avg(struct sched_entity *se)
{
	struct cfs_rq *cfs_rq = cfs_rq_of(se);
	struct sched_avg *sa = &se->avg;
	long cap = (long)(SCHED_CAPACITY_SCALE - cfs_rq->avg.util_avg) / 2;
	u64 now = cfs_rq_clock_task(cfs_rq);

	if (cap > 0) {
		if (cfs_rq->avg.util_avg != 0) {
			sa->util_avg  = cfs_rq->avg.util_avg * se->load.weight;
			sa->util_avg /= (cfs_rq->avg.load_avg + 1);

			if (sa->util_avg > cap)
				sa->util_avg = cap;
		} else {
			sa->util_avg = cap;
		}
		sa->util_sum = sa->util_avg * LOAD_AVG_MAX;
	}

	if (entity_is_task(se)) {
		struct task_struct *p = task_of(se);
		if (p->sched_class != &fair_sched_class) {
			/*
			 * For !fair tasks do:
			 *
			update_cfs_rq_load_avg(now, cfs_rq, false);
			attach_entity_load_avg(cfs_rq, se);
			switched_from_fair(rq, p);
			 *
			 * such that the next switched_to_fair() has the
			 * expected state.
			 */
			se->avg.last_update_time = now;
			return;
		}
	}

	update_cfs_rq_load_avg(now, cfs_rq);
	attach_entity_load_avg(cfs_rq, se);
	update_tg_load_avg(cfs_rq, false);
}

#else /* !CONFIG_SMP */
void init_entity_runnable_average(struct sched_entity *se)
{
}
void post_init_entity_util_avg(struct sched_entity *se)
{
}
static void update_tg_load_avg(struct cfs_rq *cfs_rq, int force)
{
}
#endif /* CONFIG_SMP */

/*
 * Update the current task's runtime statistics.
 */
static inline void
__update_curr(struct cfs_rq *cfs_rq, struct sched_entity *curr,
	      unsigned long delta_exec)
{
	unsigned long delta_exec_weighted;

	schedstat_set(curr->statistics.exec_max,
		      max((u64)delta_exec, curr->statistics.exec_max));

	curr->sum_exec_runtime += delta_exec;
	schedstat_add(cfs_rq, exec_clock, delta_exec);
	delta_exec_weighted = calc_delta_fair(delta_exec, curr);

	curr->vruntime += delta_exec_weighted;
	update_min_vruntime(cfs_rq);
}

static void update_curr(struct cfs_rq *cfs_rq)
{
	struct sched_entity *curr = cfs_rq->curr;
	u64 now = rq_clock_task(rq_of(cfs_rq));
	u64 delta_exec;

	if (unlikely(!curr))
		return;

	delta_exec = now - curr->exec_start;
	if (unlikely((s64)delta_exec <= 0))
		return;

	curr->exec_start = now;

	schedstat_set(curr->statistics.exec_max,
		      max(delta_exec, curr->statistics.exec_max));

	curr->sum_exec_runtime += delta_exec;
	schedstat_add(cfs_rq, exec_clock, delta_exec);

	curr->vruntime += calc_delta_fair(delta_exec, curr);
	update_min_vruntime(cfs_rq);

	if (entity_is_task(curr)) {
		struct task_struct *curtask = task_of(curr);

		trace_sched_stat_runtime(curtask, delta_exec, curr->vruntime);
		cpuacct_charge(curtask, delta_exec);
		account_group_exec_runtime(curtask, delta_exec);
	}

	account_cfs_rq_runtime(cfs_rq, delta_exec);
}

static void update_curr_fair(struct rq *rq)
{
	update_curr(cfs_rq_of(&rq->curr->se));
}

#ifdef CONFIG_SCHEDSTATS
static inline void
update_stats_wait_start(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	u64 wait_start = rq_clock(rq_of(cfs_rq));

	if (entity_is_task(se) && task_on_rq_migrating(task_of(se)) &&
	    likely(wait_start > se->statistics.wait_start))
		wait_start -= se->statistics.wait_start;

	se->statistics.wait_start = wait_start;
}

static void
update_stats_wait_end(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	struct task_struct *p;
	u64 delta = rq_clock(rq_of(cfs_rq)) - se->statistics.wait_start;

	if (entity_is_task(se)) {
		p = task_of(se);
		if (task_on_rq_migrating(p)) {
			/*
			 * Preserve migrating task's wait time so wait_start
			 * time stamp can be adjusted to accumulate wait time
			 * prior to migration.
			 */
			se->statistics.wait_start = delta;
			return;
		}
		trace_sched_stat_wait(p, delta);
	}

	se->statistics.wait_max = max(se->statistics.wait_max, delta);
	se->statistics.wait_count++;
	se->statistics.wait_sum += delta;
	se->statistics.wait_start = 0;
}
#else
static inline void
update_stats_wait_start(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
}

static inline void
update_stats_wait_end(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
}
#endif

/*
 * Task is being enqueued - update stats:
 */
static void update_stats_enqueue(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	/*
	 * Are we enqueueing a waiting task? (for current tasks
	 * a dequeue/enqueue event is a NOP)
	 */
	if (se != cfs_rq->curr)
		update_stats_wait_start(cfs_rq, se);
}

static inline void
update_stats_dequeue(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	/*
	 * Mark the end of the wait period if dequeueing a
	 * waiting task:
	 */
	if (se != cfs_rq->curr)
		update_stats_wait_end(cfs_rq, se);
}

/*
 * We are picking a new current task - update its stats:
 */
static inline void
update_stats_curr_start(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	/*
	 * We are starting a new run period:
	 */
	se->exec_start = rq_clock_task(rq_of(cfs_rq));
}

/**************************************************
 * Scheduling class queueing methods:
 */

#ifdef CONFIG_NUMA_BALANCING
/*
 * Approximate time to scan a full NUMA task in ms. The task scan period is
 * calculated based on the tasks virtual memory size and
 * numa_balancing_scan_size.
 */
unsigned int sysctl_numa_balancing_scan_period_min = 1000;
unsigned int sysctl_numa_balancing_scan_period_max = 60000;

/* Portion of address space to scan in MB */
unsigned int sysctl_numa_balancing_scan_size = 256;

/* Scan @scan_size MB every @scan_period after an initial @scan_delay in ms */
unsigned int sysctl_numa_balancing_scan_delay = 1000;

static unsigned int task_nr_scan_windows(struct task_struct *p)
{
	unsigned long rss = 0;
	unsigned long nr_scan_pages;

	/*
	 * Calculations based on RSS as non-present and empty pages are skipped
	 * by the PTE scanner and NUMA hinting faults should be trapped based
	 * on resident pages
	 */
	nr_scan_pages = sysctl_numa_balancing_scan_size << (20 - PAGE_SHIFT);
	rss = get_mm_rss(p->mm);
	if (!rss)
		rss = nr_scan_pages;

	rss = round_up(rss, nr_scan_pages);
	return rss / nr_scan_pages;
}

/* For sanitys sake, never scan more PTEs than MAX_SCAN_WINDOW MB/sec. */
#define MAX_SCAN_WINDOW 2560

static unsigned int task_scan_min(struct task_struct *p)
{
	unsigned int scan_size = READ_ONCE(sysctl_numa_balancing_scan_size);
	unsigned int scan, floor;
	unsigned int windows = 1;

	if (scan_size < MAX_SCAN_WINDOW)
		windows = MAX_SCAN_WINDOW / scan_size;
	floor = 1000 / windows;

	scan = sysctl_numa_balancing_scan_period_min / task_nr_scan_windows(p);
	return max_t(unsigned int, floor, scan);
}

static unsigned int task_scan_max(struct task_struct *p)
{
	unsigned int smin = task_scan_min(p);
	unsigned int smax;

	/* Watch for min being lower than max due to floor calculations */
	smax = sysctl_numa_balancing_scan_period_max / task_nr_scan_windows(p);
	return max(smin, smax);
}

static void account_numa_enqueue(struct rq *rq, struct task_struct *p)
{
	rq->nr_numa_running += (p->numa_preferred_nid != -1);
	rq->nr_preferred_running += (p->numa_preferred_nid == task_node(p));
}

static void account_numa_dequeue(struct rq *rq, struct task_struct *p)
{
	rq->nr_numa_running -= (p->numa_preferred_nid != -1);
	rq->nr_preferred_running -= (p->numa_preferred_nid == task_node(p));
}

struct numa_group {
	atomic_t refcount;

	spinlock_t lock; /* nr_tasks, tasks */
	int nr_tasks;
	pid_t gid;
	int active_nodes;

	struct rcu_head rcu;
	unsigned long total_faults;
	unsigned long max_faults_cpu;
	/*
	 * Faults_cpu is used to decide whether memory should move
	 * towards the CPU. As a consequence, these stats are weighted
	 * more by CPU use than by memory faults.
	 */
	unsigned long *faults_cpu;
	unsigned long faults[0];
};

/* Shared or private faults. */
#define NR_NUMA_HINT_FAULT_TYPES 2

/* Memory and CPU locality */
#define NR_NUMA_HINT_FAULT_STATS (NR_NUMA_HINT_FAULT_TYPES * 2)

/* Averaged statistics, and temporary buffers. */
#define NR_NUMA_HINT_FAULT_BUCKETS (NR_NUMA_HINT_FAULT_STATS * 2)

pid_t task_numa_group_id(struct task_struct *p)
{
	return p->numa_group ? p->numa_group->gid : 0;
}

static inline int task_faults_idx(int nid, int priv)
{
	return NR_NUMA_HINT_FAULT_TYPES * nid + priv;
}

static inline unsigned long task_faults(struct task_struct *p, int nid)
{
	if (!p->numa_faults_memory)
		return 0;

	return p->numa_faults_memory[task_faults_idx(nid, 0)] +
		p->numa_faults_memory[task_faults_idx(nid, 1)];
}

static inline unsigned long group_faults(struct task_struct *p, int nid)
{
	if (!p->numa_group)
		return 0;

	return p->numa_group->faults[task_faults_idx(nid, 0)] +
		p->numa_group->faults[task_faults_idx(nid, 1)];
}

static inline unsigned long group_faults_cpu(struct numa_group *group, int nid)
{
	return group->faults_cpu[task_faults_idx(nid, 0)] +
		group->faults_cpu[task_faults_idx(nid, 1)];
}

/*
 * A node triggering more than 1/3 as many NUMA faults as the maximum is
 * considered part of a numa group's pseudo-interleaving set. Migrations
 * between these nodes are slowed down, to allow things to settle down.
 */
#define ACTIVE_NODE_FRACTION 3

static bool numa_is_active_node(int nid, struct numa_group *ng)
{
	return group_faults_cpu(ng, nid) * ACTIVE_NODE_FRACTION > ng->max_faults_cpu;
}

/*
 * These return the fraction of accesses done by a particular task, or
 * task group, on a particular numa node.  The group weight is given a
 * larger multiplier, in order to group tasks together that are almost
 * evenly spread out between numa nodes.
 */
static inline unsigned long task_weight(struct task_struct *p, int nid)
{
	unsigned long total_faults;

	if (!p->numa_faults_memory)
		return 0;

	total_faults = p->total_numa_faults;

	if (!total_faults)
		return 0;

	return 1000 * task_faults(p, nid) / total_faults;
}

static inline unsigned long group_weight(struct task_struct *p, int nid)
{
	if (!p->numa_group || !p->numa_group->total_faults)
		return 0;

	return 1000 * group_faults(p, nid) / p->numa_group->total_faults;
}

bool should_numa_migrate_memory(struct task_struct *p, struct page * page,
				int src_nid, int dst_cpu)
{
	struct numa_group *ng = p->numa_group;
	int dst_nid = cpu_to_node(dst_cpu);
	int last_cpupid, this_cpupid;

	this_cpupid = cpu_pid_to_cpupid(dst_cpu, current->pid);

	/*
	 * Multi-stage node selection is used in conjunction with a periodic
	 * migration fault to build a temporal task<->page relation. By using
	 * a two-stage filter we remove short/unlikely relations.
	 *
	 * Using P(p) ~ n_p / n_t as per frequentist probability, we can equate
	 * a task's usage of a particular page (n_p) per total usage of this
	 * page (n_t) (in a given time-span) to a probability.
	 *
	 * Our periodic faults will sample this probability and getting the
	 * same result twice in a row, given these samples are fully
	 * independent, is then given by P(n)^2, provided our sample period
	 * is sufficiently short compared to the usage pattern.
	 *
	 * This quadric squishes small probabilities, making it less likely we
	 * act on an unlikely task<->page relation.
	 */
	last_cpupid = page_cpupid_xchg_last(page, this_cpupid);
	if (!cpupid_pid_unset(last_cpupid) &&
				cpupid_to_nid(last_cpupid) != dst_nid)
		return false;

	/* Always allow migrate on private faults */
	if (cpupid_match_pid(p, last_cpupid))
		return true;

	/* A shared fault, but p->numa_group has not been set up yet. */
	if (!ng)
		return true;

	/*
	 * Destination node is much more heavily used than the source
	 * node? Allow migration.
	 */
	if (group_faults_cpu(ng, dst_nid) > group_faults_cpu(ng, src_nid) *
					ACTIVE_NODE_FRACTION)
		return true;

	/*
	 * Distribute memory according to CPU & memory use on each node,
	 * with 3/4 hysteresis to avoid unnecessary memory migrations:
	 *
	 * faults_cpu(dst)   3   faults_cpu(src)
	 * --------------- * - > ---------------
	 * faults_mem(dst)   4   faults_mem(src)
	 */
	return group_faults_cpu(ng, dst_nid) * group_faults(p, src_nid) * 3 >
	       group_faults_cpu(ng, src_nid) * group_faults(p, dst_nid) * 4;
}

static unsigned long weighted_cpuload(const int cpu);
static unsigned long source_load(int cpu, int type);
static unsigned long target_load(int cpu, int type);
static unsigned long capacity_of(int cpu);
static long effective_load(struct task_group *tg, int cpu, long wl, long wg);

/* Cached statistics for all CPUs within a node */
struct numa_stats {
	unsigned long nr_running;
	unsigned long load;

	/* Total compute capacity of CPUs on a node */
	unsigned long compute_capacity;

	/* Approximate capacity in terms of runnable tasks on a node */
	unsigned long task_capacity;
	int has_free_capacity;
};

/*
 * XXX borrowed from update_sg_lb_stats
 */
static void update_numa_stats(struct numa_stats *ns, int nid)
{
	int smt, cpu, cpus = 0;
	unsigned long capacity;

	memset(ns, 0, sizeof(*ns));
	for_each_cpu(cpu, cpumask_of_node(nid)) {
		struct rq *rq = cpu_rq(cpu);

		ns->nr_running += rq->nr_running;
		ns->load += weighted_cpuload(cpu);
		ns->compute_capacity += capacity_of(cpu);

		cpus++;
	}

	/*
	 * If we raced with hotplug and there are no CPUs left in our mask
	 * the @ns structure is NULL'ed and task_numa_compare() will
	 * not find this node attractive.
	 *
	 * We'll either bail at !has_free_capacity, or we'll detect a huge
	 * imbalance and bail there.
	 */
	if (!cpus)
		return;

	/* smt := ceil(cpus / capacity), assumes: 1 < smt_capacity < 2 */
	smt = DIV_ROUND_UP(SCHED_CAPACITY_SCALE * cpus, ns->compute_capacity);
	capacity = cpus / smt; /* cores */

	ns->task_capacity = min_t(unsigned, capacity,
		DIV_ROUND_CLOSEST(ns->compute_capacity, SCHED_CAPACITY_SCALE));
	ns->has_free_capacity = (ns->nr_running < ns->task_capacity);
}

struct task_numa_env {
	struct task_struct *p;

	int src_cpu, src_nid;
	int dst_cpu, dst_nid;

	struct numa_stats src_stats, dst_stats;

	int imbalance_pct;

	struct task_struct *best_task;
	long best_imp;
	int best_cpu;
};

static void task_numa_assign(struct task_numa_env *env,
			     struct task_struct *p, long imp)
{
	if (env->best_task)
		put_task_struct(env->best_task);
	if (p)
		get_task_struct(p);

	env->best_task = p;
	env->best_imp = imp;
	env->best_cpu = env->dst_cpu;
}

static bool load_too_imbalanced(long src_load, long dst_load,
				struct task_numa_env *env)
{
	long imb, old_imb;
	long orig_src_load, orig_dst_load;
	long src_capacity, dst_capacity;

	/*
	 * The load is corrected for the CPU capacity available on each node.
	 *
	 * src_load        dst_load
	 * ------------ vs ---------
	 * src_capacity    dst_capacity
	 */
	src_capacity = env->src_stats.compute_capacity;
	dst_capacity = env->dst_stats.compute_capacity;

	/* We care about the slope of the imbalance, not the direction. */
	if (dst_load < src_load)
		swap(dst_load, src_load);

	/* Is the difference below the threshold? */
	imb = dst_load * src_capacity * 100 -
	      src_load * dst_capacity * env->imbalance_pct;
	if (imb <= 0)
		return false;

	/*
	 * The imbalance is above the allowed threshold.
	 * Compare it with the old imbalance.
	 */
	orig_src_load = env->src_stats.load;
	orig_dst_load = env->dst_stats.load;

	if (orig_dst_load < orig_src_load)
		swap(orig_dst_load, orig_src_load);

	old_imb = orig_dst_load * src_capacity * 100 -
		  orig_src_load * dst_capacity * env->imbalance_pct;

	/* Would this change make things worse? */
	return (imb > old_imb);
}

/*
 * This checks if the overall compute and NUMA accesses of the system would
 * be improved if the source tasks was migrated to the target dst_cpu taking
 * into account that it might be best if task running on the dst_cpu should
 * be exchanged with the source task
 */
static void task_numa_compare(struct task_numa_env *env,
			      long taskimp, long groupimp)
{
	struct rq *src_rq = cpu_rq(env->src_cpu);
	struct rq *dst_rq = cpu_rq(env->dst_cpu);
	struct task_struct *cur;
	long src_load, dst_load;
	long load;
	long imp = env->p->numa_group ? groupimp : taskimp;
	long moveimp = imp;

	rcu_read_lock();

	cur = task_rcu_dereference(&dst_rq->curr);
	if (cur && ((cur->flags & PF_EXITING) || is_idle_task(cur)))
		cur = NULL;

	/*
	 * Because we have preemption enabled we can get migrated around and
	 * end try selecting ourselves (current == env->p) as a swap candidate.
	 */
	if (cur == env->p)
		goto unlock;

	/*
	 * "imp" is the fault differential for the source task between the
	 * source and destination node. Calculate the total differential for
	 * the source task and potential destination task. The more negative
	 * the value is, the more rmeote accesses that would be expected to
	 * be incurred if the tasks were swapped.
	 */
	if (cur) {
		/* Skip this swap candidate if cannot move to the source cpu */
		if (!cpumask_test_cpu(env->src_cpu, tsk_cpus_allowed(cur)))
			goto unlock;

		/*
		 * If dst and source tasks are in the same NUMA group, or not
		 * in any group then look only at task weights.
		 */
		if (cur->numa_group == env->p->numa_group) {
			imp = taskimp + task_weight(cur, env->src_nid) -
			      task_weight(cur, env->dst_nid);
			/*
			 * Add some hysteresis to prevent swapping the
			 * tasks within a group over tiny differences.
			 */
			if (cur->numa_group)
				imp -= imp/16;
		} else {
			/*
			 * Compare the group weights. If a task is all by
			 * itself (not part of a group), use the task weight
			 * instead.
			 */
			if (cur->numa_group)
				imp += group_weight(cur, env->src_nid) -
				       group_weight(cur, env->dst_nid);
			else
				imp += task_weight(cur, env->src_nid) -
				       task_weight(cur, env->dst_nid);
		}
	}

	if (imp <= env->best_imp && moveimp <= env->best_imp)
		goto unlock;

	if (!cur) {
		/* Is there capacity at our destination? */
		if (env->src_stats.nr_running <= env->src_stats.task_capacity &&
		    !env->dst_stats.has_free_capacity)
			goto unlock;

		goto balance;
	}

	/* Balance doesn't matter much if we're running a task per cpu */
	if (imp > env->best_imp && src_rq->nr_running == 1 &&
			dst_rq->nr_running == 1)
		goto assign;

	/*
	 * In the overloaded case, try and keep the load balanced.
	 */
balance:
	load = task_h_load(env->p);
	dst_load = env->dst_stats.load + load;
	src_load = env->src_stats.load - load;

	if (moveimp > imp && moveimp > env->best_imp) {
		/*
		 * If the improvement from just moving env->p direction is
		 * better than swapping tasks around, check if a move is
		 * possible. Store a slightly smaller score than moveimp,
		 * so an actually idle CPU will win.
		 */
		if (!load_too_imbalanced(src_load, dst_load, env)) {
			imp = moveimp - 1;
			cur = NULL;
			goto assign;
		}
	}

	if (imp <= env->best_imp)
		goto unlock;

	if (cur) {
		load = task_h_load(cur);
		dst_load -= load;
		src_load += load;
	}

	if (load_too_imbalanced(src_load, dst_load, env))
		goto unlock;

	/*
	 * One idle CPU per node is evaluated for a task numa move.
	 * Call select_idle_sibling to maybe find a better one.
	 */
	if (!cur)
		env->dst_cpu = select_idle_sibling(env->p, env->src_cpu,
						   env->dst_cpu);

assign:
	task_numa_assign(env, cur, imp);
unlock:
	rcu_read_unlock();
}

static void task_numa_find_cpu(struct task_numa_env *env,
				long taskimp, long groupimp)
{
	int cpu;

	for_each_cpu(cpu, cpumask_of_node(env->dst_nid)) {
		/* Skip this CPU if the source task cannot migrate */
		if (!cpumask_test_cpu(cpu, tsk_cpus_allowed(env->p)))
			continue;

		env->dst_cpu = cpu;
		task_numa_compare(env, taskimp, groupimp);
	}
}

/* Only move tasks to a NUMA node less busy than the current node. */
static bool numa_has_capacity(struct task_numa_env *env)
{
	struct numa_stats *src = &env->src_stats;
	struct numa_stats *dst = &env->dst_stats;

	if (src->has_free_capacity && !dst->has_free_capacity)
		return false;

	/*
	 * Only consider a task move if the source has a higher load
	 * than the destination, corrected for CPU capacity on each node.
	 *
	 *      src->load                dst->load
	 * --------------------- vs ---------------------
	 * src->compute_capacity    dst->compute_capacity
	 */
	if (src->load * dst->compute_capacity >
	    dst->load * src->compute_capacity)
		return true;

	return false;
}

static int task_numa_migrate(struct task_struct *p)
{
	struct task_numa_env env = {
		.p = p,

		.src_cpu = task_cpu(p),
		.src_nid = task_node(p),

		.imbalance_pct = 112,

		.best_task = NULL,
		.best_imp = 0,
		.best_cpu = -1,
	};
	struct sched_domain *sd;
	unsigned long taskweight, groupweight;
	int nid, ret;
	long taskimp, groupimp;

	/*
	 * Pick the lowest SD_NUMA domain, as that would have the smallest
	 * imbalance and would be the first to start moving tasks about.
	 *
	 * And we want to avoid any moving of tasks about, as that would create
	 * random movement of tasks -- counter the numa conditions we're trying
	 * to satisfy here.
	 */
	rcu_read_lock();
	sd = rcu_dereference(per_cpu(sd_numa, env.src_cpu));
	if (sd)
		env.imbalance_pct = 100 + (sd->imbalance_pct - 100) / 2;
	rcu_read_unlock();

	/*
	 * Cpusets can break the scheduler domain tree into smaller
	 * balance domains, some of which do not cross NUMA boundaries.
	 * Tasks that are "trapped" in such domains cannot be migrated
	 * elsewhere, so there is no point in (re)trying.
	 */
	if (unlikely(!sd)) {
		p->numa_preferred_nid = task_node(p);
		return -EINVAL;
	}

	taskweight = task_weight(p, env.src_nid);
	groupweight = group_weight(p, env.src_nid);
	update_numa_stats(&env.src_stats, env.src_nid);
	env.dst_nid = p->numa_preferred_nid;
	taskimp = task_weight(p, env.dst_nid) - taskweight;
	groupimp = group_weight(p, env.dst_nid) - groupweight;
	update_numa_stats(&env.dst_stats, env.dst_nid);

	/* Try to find a spot on the preferred nid. */
	if (numa_has_capacity(&env))
		task_numa_find_cpu(&env, taskimp, groupimp);

	/* No space available on the preferred nid. Look elsewhere. */
	if (env.best_cpu == -1 || (p->numa_group && p->numa_group->active_nodes > 1)) {
		for_each_online_node(nid) {
			if (nid == env.src_nid || nid == p->numa_preferred_nid)
				continue;

			/* Only consider nodes where both task and groups benefit */
			taskimp = task_weight(p, nid) - taskweight;
			groupimp = group_weight(p, nid) - groupweight;
			if (taskimp < 0 && groupimp < 0)
				continue;

			env.dst_nid = nid;
			update_numa_stats(&env.dst_stats, env.dst_nid);
			if (numa_has_capacity(&env))
				task_numa_find_cpu(&env, taskimp, groupimp);
		}
	}

	/*
	 * If the task is part of a workload that spans multiple NUMA nodes,
	 * and is migrating into one of the workload's active nodes, remember
	 * this node as the task's preferred numa node, so the workload can
	 * settle down.
	 * A task that migrated to a second choice node will be better off
	 * trying for a better one later. Do not set the preferred node here.
	 */
	if (p->numa_group) {
		struct numa_group *ng = p->numa_group;

		if (env.best_cpu == -1)
			nid = env.src_nid;
		else
			nid = env.dst_nid;

		if (ng->active_nodes > 1 && numa_is_active_node(env.dst_nid, ng))
			sched_setnuma(p, env.dst_nid);
	}

	/* No better CPU than the current one was found. */
	if (env.best_cpu == -1)
		return -EAGAIN;

	/*
	 * Reset the scan period if the task is being rescheduled on an
	 * alternative node to recheck if the tasks is now properly placed.
	 */
	p->numa_scan_period = task_scan_min(p);

	if (env.best_task == NULL) {
		ret = migrate_task_to(p, env.best_cpu);
		if (ret != 0)
			trace_sched_stick_numa(p, env.src_cpu, env.best_cpu);
		return ret;
	}

	ret = migrate_swap(p, env.best_task);
	if (ret != 0)
		trace_sched_stick_numa(p, env.src_cpu, task_cpu(env.best_task));
	put_task_struct(env.best_task);
	return ret;
}

/* Attempt to migrate a task to a CPU on the preferred node. */
static void numa_migrate_preferred(struct task_struct *p)
{
	unsigned long interval = HZ;

	/* This task has no NUMA fault statistics yet */
	if (unlikely(p->numa_preferred_nid == -1 || !p->numa_faults_memory))
		return;

	/* Periodically retry migrating the task to the preferred node */
	interval = min(interval, msecs_to_jiffies(p->numa_scan_period) / 16);
	p->numa_migrate_retry = jiffies + interval;

	/* Success if task is already running on preferred CPU */
	if (task_node(p) == p->numa_preferred_nid)
		return;

	/* Otherwise, try migrate to a CPU on the preferred node */
	task_numa_migrate(p);
}

/*
 * Find out how many nodes on the workload is actively running on. Do this by
 * tracking the nodes from which NUMA hinting faults are triggered. This can
 * be different from the set of nodes where the workload's memory is currently
 * located.
 */
static void numa_group_count_active_nodes(struct numa_group *numa_group)
{
	unsigned long faults, max_faults = 0;
	int nid, active_nodes = 0;

	for_each_online_node(nid) {
		faults = group_faults_cpu(numa_group, nid);
		if (faults > max_faults)
			max_faults = faults;
	}

	for_each_online_node(nid) {
		faults = group_faults_cpu(numa_group, nid);
		if (faults * ACTIVE_NODE_FRACTION > max_faults)
			active_nodes++;
	}

	numa_group->max_faults_cpu = max_faults;
	numa_group->active_nodes = active_nodes;
}

/*
 * When adapting the scan rate, the period is divided into NUMA_PERIOD_SLOTS
 * increments. The more local the fault statistics are, the higher the scan
 * period will be for the next scan window. If local/(local+remote) ratio is
 * below NUMA_PERIOD_THRESHOLD (where range of ratio is 1..NUMA_PERIOD_SLOTS)
 * the scan period will decrease. Aim for 70% local accesses.
 */
#define NUMA_PERIOD_SLOTS 10
#define NUMA_PERIOD_THRESHOLD 7

/*
 * Increase the scan period (slow down scanning) if the majority of
 * our memory is already on our local node, or if the majority of
 * the page accesses are shared with other processes.
 * Otherwise, decrease the scan period.
 */
static void update_task_scan_period(struct task_struct *p,
			unsigned long shared, unsigned long private)
{
	unsigned int period_slot;
	int ratio;
	int diff;

	unsigned long remote = p->numa_faults_locality[0];
	unsigned long local = p->numa_faults_locality[1];

	/*
	 * If there were no record hinting faults then either the task is
	 * completely idle or all activity is areas that are not of interest
	 * to automatic numa balancing. Scan slower
	 */
	if (local + shared == 0) {
		p->numa_scan_period = min(p->numa_scan_period_max,
			p->numa_scan_period << 1);

		p->mm->numa_next_scan = jiffies +
			msecs_to_jiffies(p->numa_scan_period);

		return;
	}

	/*
	 * Prepare to scale scan period relative to the current period.
	 *	 == NUMA_PERIOD_THRESHOLD scan period stays the same
	 *       <  NUMA_PERIOD_THRESHOLD scan period decreases (scan faster)
	 *	 >= NUMA_PERIOD_THRESHOLD scan period increases (scan slower)
	 */
	period_slot = DIV_ROUND_UP(p->numa_scan_period, NUMA_PERIOD_SLOTS);
	ratio = (local * NUMA_PERIOD_SLOTS) / (local + remote);
	if (ratio >= NUMA_PERIOD_THRESHOLD) {
		int slot = ratio - NUMA_PERIOD_THRESHOLD;
		if (!slot)
			slot = 1;
		diff = slot * period_slot;
	} else {
		diff = -(NUMA_PERIOD_THRESHOLD - ratio) * period_slot;

		/*
		 * Scale scan rate increases based on sharing. There is an
		 * inverse relationship between the degree of sharing and
		 * the adjustment made to the scanning period. Broadly
		 * speaking the intent is that there is little point
		 * scanning faster if shared accesses dominate as it may
		 * simply bounce migrations uselessly
		 */
		ratio = DIV_ROUND_UP(private * NUMA_PERIOD_SLOTS, (private + shared + 1));
		diff = (diff * ratio) / NUMA_PERIOD_SLOTS;
	}

	p->numa_scan_period = clamp(p->numa_scan_period + diff,
			task_scan_min(p), task_scan_max(p));
	memset(p->numa_faults_locality, 0, sizeof(p->numa_faults_locality));
}

/*
 * Get the fraction of time the task has been running since the last
 * NUMA placement cycle. The scheduler keeps similar statistics, but
 * decays those on a 32ms period, which is orders of magnitude off
 * from the dozens-of-seconds NUMA balancing period. Use the scheduler
 * stats only if the task is so new there are no NUMA statistics yet.
 */
static u64 numa_get_avg_runtime(struct task_struct *p, u64 *period)
{
	u64 runtime, delta, now;
	/* Use the start of this time slice to avoid calculations. */
	now = p->se.exec_start;
	runtime = p->se.sum_exec_runtime;

	if (p->last_task_numa_placement) {
		delta = runtime - p->last_sum_exec_runtime;
		*period = now - p->last_task_numa_placement;
	} else {
		delta = p->se.avg.load_sum / p->se.load.weight;
		*period = LOAD_AVG_MAX;
	}

	p->last_sum_exec_runtime = runtime;
	p->last_task_numa_placement = now;

	return delta;
}

static unsigned long weighted_cpuload(const int cpu);


static int
find_idlest_cpu_node(int this_cpu, int nid)
{
	unsigned long load, min_load = ULONG_MAX;
	int i, idlest_cpu = this_cpu;

	BUG_ON(cpu_to_node(this_cpu) == nid);

	rcu_read_lock();
	for_each_cpu(i, cpumask_of_node(nid)) {
		load = weighted_cpuload(i);

		if (load < min_load) {
			min_load = load;
			idlest_cpu = i;
		}
	}
	rcu_read_unlock();

	return idlest_cpu;
}

static void task_numa_placement(struct task_struct *p)
{
	int seq, nid, max_nid = -1, max_group_nid = -1;
	unsigned long max_faults = 0, max_group_faults = 0;
	unsigned long fault_types[2] = { 0, 0 };
	unsigned long total_faults;
	u64 runtime, period;
	spinlock_t *group_lock = NULL;

	/*
	 * The p->mm->numa_scan_seq field gets updated without
	 * exclusive access. Use READ_ONCE() here to ensure
	 * that the field is read in a single access:
	 */
	seq = READ_ONCE(p->mm->numa_scan_seq);
	if (p->numa_scan_seq == seq)
		return;
	p->numa_scan_seq = seq;
	p->numa_scan_period_max = task_scan_max(p);

	total_faults = p->numa_faults_locality[0] +
		       p->numa_faults_locality[1];
	runtime = numa_get_avg_runtime(p, &period);

	/* If the task is part of a group prevent parallel updates to group stats */
	if (p->numa_group) {
		group_lock = &p->numa_group->lock;
		spin_lock_irq(group_lock);
	}

	/* Find the node with the highest number of faults */
	for_each_online_node(nid) {
		unsigned long faults = 0, group_faults = 0;
		int priv, i;

		for (priv = 0; priv < NR_NUMA_HINT_FAULT_TYPES; priv++) {
			long diff, f_diff, f_weight;

			i = task_faults_idx(nid, priv);

			/* Decay existing window, copy faults since last scan */
			diff = p->numa_faults_buffer_memory[i] - p->numa_faults_memory[i] / 2;
			fault_types[priv] += p->numa_faults_buffer_memory[i];
			p->numa_faults_buffer_memory[i] = 0;

			/*
			 * Normalize the faults_from, so all tasks in a group
			 * count according to CPU use, instead of by the raw
			 * number of faults. Tasks with little runtime have
			 * little over-all impact on throughput, and thus their
			 * faults are less important.
			 */
			f_weight = div64_u64(runtime << 16, period + 1);
			f_weight = (f_weight * p->numa_faults_buffer_cpu[i]) /
				   (total_faults + 1);
			f_diff = f_weight - p->numa_faults_cpu[i] / 2;
			p->numa_faults_buffer_cpu[i] = 0;

			p->numa_faults_memory[i] += diff;
			p->numa_faults_cpu[i] += f_diff;
			faults += p->numa_faults_memory[i];
			p->total_numa_faults += diff;
			if (p->numa_group) {
				/* safe because we can only change our own group */
				p->numa_group->faults[i] += diff;
				p->numa_group->faults_cpu[i] += f_diff;
				p->numa_group->total_faults += diff;
				group_faults += p->numa_group->faults[i];
			}
		}

		if (faults > max_faults) {
			max_faults = faults;
			max_nid = nid;
		}

		if (group_faults > max_group_faults) {
			max_group_faults = group_faults;
			max_group_nid = nid;
		}
	}

	update_task_scan_period(p, fault_types[0], fault_types[1]);

	if (p->numa_group) {
		numa_group_count_active_nodes(p->numa_group);
		spin_unlock_irq(group_lock);
		max_nid = max_group_nid;
	}

	if (max_faults) {
		/* Set the new preferred node */
		if (max_nid != p->numa_preferred_nid)
			sched_setnuma(p, max_nid);

		if (task_node(p) != p->numa_preferred_nid)
			numa_migrate_preferred(p);
	}
}

static inline int get_numa_group(struct numa_group *grp)
{
	return atomic_inc_not_zero(&grp->refcount);
}

static inline void put_numa_group(struct numa_group *grp)
{
	if (atomic_dec_and_test(&grp->refcount))
		kfree_rcu(grp, rcu);
}

static void task_numa_group(struct task_struct *p, int cpupid, int flags,
			int *priv)
{
	struct numa_group *grp, *my_grp;
	struct task_struct *tsk;
	bool join = false;
	int cpu = cpupid_to_cpu(cpupid);
	int i;

	if (unlikely(!p->numa_group)) {
		unsigned int size = sizeof(struct numa_group) +
				    4*nr_node_ids*sizeof(unsigned long);

		grp = kzalloc(size, GFP_KERNEL | __GFP_NOWARN);
		if (!grp)
			return;

		atomic_set(&grp->refcount, 1);
		grp->active_nodes = 1;
		grp->max_faults_cpu = 0;
		spin_lock_init(&grp->lock);
		grp->gid = p->pid;
		/* Second half of the array tracks nids where faults happen */
		grp->faults_cpu = grp->faults + NR_NUMA_HINT_FAULT_TYPES *
						nr_node_ids;

		for (i = 0; i < NR_NUMA_HINT_FAULT_STATS * nr_node_ids; i++)
			grp->faults[i] = p->numa_faults_memory[i];

		grp->total_faults = p->total_numa_faults;

		grp->nr_tasks++;
		rcu_assign_pointer(p->numa_group, grp);
	}

	rcu_read_lock();
	tsk = READ_ONCE(cpu_rq(cpu)->curr);

	if (!cpupid_match_pid(tsk, cpupid))
		goto no_join;

	grp = rcu_dereference(tsk->numa_group);
	if (!grp)
		goto no_join;

	my_grp = p->numa_group;
	if (grp == my_grp)
		goto no_join;

	/*
	 * Only join the other group if its bigger; if we're the bigger group,
	 * the other task will join us.
	 */
	if (my_grp->nr_tasks > grp->nr_tasks)
		goto no_join;

	/*
	 * Tie-break on the grp address.
	 */
	if (my_grp->nr_tasks == grp->nr_tasks && my_grp > grp)
		goto no_join;

	/* Always join threads in the same process. */
	if (tsk->mm == current->mm)
		join = true;

	/* Simple filter to avoid false positives due to PID collisions */
	if (flags & TNF_SHARED)
		join = true;

	/* Update priv based on whether false sharing was detected */
	*priv = !join;

	if (join && !get_numa_group(grp))
		goto no_join;

	rcu_read_unlock();

	if (!join)
		return;

	BUG_ON(irqs_disabled());
	double_lock_irq(&my_grp->lock, &grp->lock);

	for (i = 0; i < NR_NUMA_HINT_FAULT_STATS * nr_node_ids; i++) {
		my_grp->faults[i] -= p->numa_faults_memory[i];
		grp->faults[i] += p->numa_faults_memory[i];
	}
	my_grp->total_faults -= p->total_numa_faults;
	grp->total_faults += p->total_numa_faults;

	my_grp->nr_tasks--;
	grp->nr_tasks++;

	spin_unlock(&my_grp->lock);
	spin_unlock_irq(&grp->lock);

	rcu_assign_pointer(p->numa_group, grp);

	put_numa_group(my_grp);
	return;

no_join:
	rcu_read_unlock();
	return;
}

void task_numa_free(struct task_struct *p)
{
	struct numa_group *grp = p->numa_group;
	void *numa_faults = p->numa_faults_memory;
	unsigned long flags;
	int i;

	if (grp) {
		spin_lock_irqsave(&grp->lock, flags);
		for (i = 0; i < NR_NUMA_HINT_FAULT_STATS * nr_node_ids; i++)
			grp->faults[i] -= p->numa_faults_memory[i];
		grp->total_faults -= p->total_numa_faults;

		grp->nr_tasks--;
		spin_unlock_irqrestore(&grp->lock, flags);
		RCU_INIT_POINTER(p->numa_group, NULL);
		put_numa_group(grp);
	}

	p->numa_faults_memory = NULL;
	p->numa_faults_buffer_memory = NULL;
	p->numa_faults_cpu= NULL;
	p->numa_faults_buffer_cpu = NULL;
	kfree(numa_faults);
}

/*
 * Got a PROT_NONE fault for a page on @node.
 */
void task_numa_fault(int last_cpupid, int mem_node, int pages, int flags)
{
	struct task_struct *p = current;
	bool migrated = flags & TNF_MIGRATED;
	int cpu_node = task_node(current);
	int local = !!(flags & TNF_FAULT_LOCAL);
	struct numa_group *ng;
	int priv;

	if (!sched_numa_balancing)
		return;

	/* for example, ksmd faulting in a user's mm */
	if (!p->mm)
		return;

	/* Allocate buffer to track faults on a per-node basis */
	if (unlikely(!p->numa_faults_memory)) {
		int size = sizeof(*p->numa_faults_memory) *
			   NR_NUMA_HINT_FAULT_BUCKETS * nr_node_ids;

		p->numa_faults_memory = kzalloc(size, GFP_KERNEL|__GFP_NOWARN);
		if (!p->numa_faults_memory)
			return;

		BUG_ON(p->numa_faults_buffer_memory);
		/*
		 * The averaged statistics, shared & private, memory & cpu,
		 * occupy the first half of the array. The second half of the
		 * array is for current counters, which are averaged into the
		 * first set by task_numa_placement.
		 */
		p->numa_faults_cpu = p->numa_faults_memory + (2 * nr_node_ids);
		p->numa_faults_buffer_memory = p->numa_faults_memory + (4 * nr_node_ids);
		p->numa_faults_buffer_cpu = p->numa_faults_memory + (6 * nr_node_ids);
		p->total_numa_faults = 0;
		memset(p->numa_faults_locality, 0, sizeof(p->numa_faults_locality));
	}

	/*
	 * First accesses are treated as private, otherwise consider accesses
	 * to be private if the accessing pid has not changed
	 */
	if (unlikely(last_cpupid == (-1 & LAST_CPUPID_MASK))) {
		priv = 1;
	} else {
		priv = cpupid_match_pid(p, last_cpupid);
		if (!priv && !(flags & TNF_NO_GROUP))
			task_numa_group(p, last_cpupid, flags, &priv);
	}

	/*
	 * If a workload spans multiple NUMA nodes, a shared fault that
	 * occurs wholly within the set of nodes that the workload is
	 * actively using should be counted as local. This allows the
	 * scan rate to slow down when a workload has settled down.
	 */
	ng = p->numa_group;
	if (!priv && !local && ng && ng->active_nodes > 1 &&
				numa_is_active_node(cpu_node, ng) &&
				numa_is_active_node(mem_node, ng))
		local = 1;

	task_numa_placement(p);

	/*
	 * Retry task to preferred node migration periodically, in case it
	 * case it previously failed, or the scheduler moved us.
	 */
	if (time_after(jiffies, p->numa_migrate_retry))
		numa_migrate_preferred(p);

	if (migrated)
		p->numa_pages_migrated += pages;

	p->numa_faults_buffer_memory[task_faults_idx(mem_node, priv)] += pages;
	p->numa_faults_buffer_cpu[task_faults_idx(cpu_node, priv)] += pages;
	p->numa_faults_locality[local] += pages;
}

static void reset_ptenuma_scan(struct task_struct *p)
{
	/*
	 * We only did a read acquisition of the mmap sem, so
	 * p->mm->numa_scan_seq is written to without exclusive access
	 * and the update is not guaranteed to be atomic. That's not
	 * much of an issue though, since this is just used for
	 * statistical sampling. Use READ_ONCE/WRITE_ONCE, which are not
	 * expensive, to avoid any form of compiler optimizations:
	 */
	WRITE_ONCE(p->mm->numa_scan_seq, READ_ONCE(p->mm->numa_scan_seq) + 1);
	p->mm->numa_scan_offset = 0;
}

/*
 * The expensive part of numa migration is done from task_work context.
 * Triggered from task_tick_numa().
 */
void task_numa_work(struct callback_head *work)
{
	unsigned long migrate, next_scan, now = jiffies;
	struct task_struct *p = current;
	struct mm_struct *mm = p->mm;
	u64 runtime = p->se.sum_exec_runtime;
	struct vm_area_struct *vma;
	unsigned long start, end;
	unsigned long nr_pte_updates = 0;
	long pages, virtpages;

	WARN_ON_ONCE(p != container_of(work, struct task_struct, numa_work));

	work->next = work; /* protect against double add */
	/*
	 * Who cares about NUMA placement when they're dying.
	 *
	 * NOTE: make sure not to dereference p->mm before this check,
	 * exit_task_work() happens _after_ exit_mm() so we could be called
	 * without p->mm even though we still had it when we enqueued this
	 * work.
	 */
	if (p->flags & PF_EXITING)
		return;

	if (!mm->numa_next_scan) {
		mm->numa_next_scan = now +
			msecs_to_jiffies(sysctl_numa_balancing_scan_delay);
	}

	/*
	 * Enforce maximal scan/migration frequency..
	 */
	migrate = mm->numa_next_scan;
	if (time_before(now, migrate))
		return;

	if (p->numa_scan_period == 0) {
		p->numa_scan_period_max = task_scan_max(p);
		p->numa_scan_period = task_scan_min(p);
	}

	next_scan = now + msecs_to_jiffies(p->numa_scan_period);
	if (cmpxchg(&mm->numa_next_scan, migrate, next_scan) != migrate)
		return;

	/*
	 * Delay this task enough that another task of this mm will likely win
	 * the next time around.
	 */
	p->node_stamp += 2 * TICK_NSEC;

	start = mm->numa_scan_offset;
	pages = sysctl_numa_balancing_scan_size;
	pages <<= 20 - PAGE_SHIFT; /* MB in pages */
	virtpages = pages * 8;	   /* Scan up to this much virtual space */
	if (!pages)
		return;


	down_read(&mm->mmap_sem);
	vma = find_vma(mm, start);
	if (!vma) {
		reset_ptenuma_scan(p);
		start = 0;
		vma = mm->mmap;
	}
	for (; vma; vma = vma->vm_next) {
		if (!vma_migratable(vma) || !vma_policy_mof(vma) ||
		    is_vm_hugetlb_page(vma) || (vma->vm_flags & VM_MIXEDMAP)) {
			continue;
		}

		/*
		 * Shared library pages mapped by multiple processes are not
		 * migrated as it is expected they are cache replicated. Avoid
		 * hinting faults in read-only file-backed mappings or the vdso
		 * as migrating the pages will be of marginal benefit.
		 */
		if (!vma->vm_mm ||
		    (vma->vm_file && (vma->vm_flags & (VM_READ|VM_WRITE)) == (VM_READ)))
			continue;

		/*
		 * Skip inaccessible VMAs to avoid any confusion between
		 * PROT_NONE and NUMA hinting ptes
		 */
		if (!(vma->vm_flags & (VM_READ | VM_EXEC | VM_WRITE)))
			continue;

		do {
			start = max(start, vma->vm_start);
			end = ALIGN(start + (pages << PAGE_SHIFT), HPAGE_SIZE);
			end = min(end, vma->vm_end);
			nr_pte_updates = change_prot_numa(vma, start, end);

			/*
			 * Try to scan sysctl_numa_balancing_size worth of
			 * hpages that have at least one present PTE that
			 * is not already pte-numa. If the VMA contains
			 * areas that are unused or already full of prot_numa
			 * PTEs, scan up to virtpages, to skip through those
			 * areas faster.
			 */
			if (nr_pte_updates)
				pages -= (end - start) >> PAGE_SHIFT;
			virtpages -= (end - start) >> PAGE_SHIFT;

			start = end;
			if (pages <= 0 || virtpages <= 0)
				goto out;

			cond_resched();
		} while (end != vma->vm_end);
	}

out:
	/*
	 * It is possible to reach the end of the VMA list but the last few
	 * VMAs are not guaranteed to the vma_migratable. If they are not, we
	 * would find the !migratable VMA on the next scan but not reset the
	 * scanner to the start so check it now.
	 */
	if (vma)
		mm->numa_scan_offset = start;
	else
		reset_ptenuma_scan(p);
	up_read(&mm->mmap_sem);

	/*
	 * Make sure tasks use at least 32x as much time to run other code
	 * than they used here, to limit NUMA PTE scanning overhead to 3% max.
	 * Usually update_task_scan_period slows down scanning enough; on an
	 * overloaded system we need to limit overhead on a per task basis.
	 */
	if (unlikely(p->se.sum_exec_runtime != runtime)) {
		u64 diff = p->se.sum_exec_runtime - runtime;
		p->node_stamp += 32 * diff;
	}
}

/*
 * Drive the periodic memory faults..
 */
void task_tick_numa(struct rq *rq, struct task_struct *curr)
{
	struct callback_head *work = &curr->numa_work;
	u64 period, now;

	/*
	 * We don't care about NUMA placement if we don't have memory.
	 */
	if (!curr->mm || (curr->flags & PF_EXITING) || work->next != work)
		return;

	/*
	 * Using runtime rather than walltime has the dual advantage that
	 * we (mostly) drive the selection from busy threads and that the
	 * task needs to have done some actual work before we bother with
	 * NUMA placement.
	 */
	now = curr->se.sum_exec_runtime;
	period = (u64)curr->numa_scan_period * NSEC_PER_MSEC;

	if (now > curr->node_stamp + period) {
		if (!curr->node_stamp)
			curr->numa_scan_period = task_scan_min(curr);
		curr->node_stamp += period;

		if (!time_before(jiffies, curr->mm->numa_next_scan)) {
			init_task_work(work, task_numa_work); /* TODO: move this into sched_fork() */
			task_work_add(curr, work, true);
		}
	}
}
#else
static void task_tick_numa(struct rq *rq, struct task_struct *curr)
{
}

static inline void account_numa_enqueue(struct rq *rq, struct task_struct *p)
{
}

static inline void account_numa_dequeue(struct rq *rq, struct task_struct *p)
{
}
#endif /* CONFIG_NUMA_BALANCING */

static void
account_entity_enqueue(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	update_load_add(&cfs_rq->load, se->load.weight);
	if (!parent_entity(se))
		update_load_add(&rq_of(cfs_rq)->load, se->load.weight);
#ifdef CONFIG_SMP
	if (entity_is_task(se)) {
		struct rq *rq = rq_of(cfs_rq);

		account_numa_enqueue(rq, task_of(se));
		list_add(&se->group_node, &rq->cfs_tasks);
	}
#endif
	cfs_rq->nr_running++;
}

static void
account_entity_dequeue(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	update_load_sub(&cfs_rq->load, se->load.weight);
	if (!parent_entity(se))
		update_load_sub(&rq_of(cfs_rq)->load, se->load.weight);
#ifdef CONFIG_SMP
	if (entity_is_task(se)) {
		account_numa_dequeue(rq_of(cfs_rq), task_of(se));
		list_del_init(&se->group_node);
	}
#endif
	cfs_rq->nr_running--;
}

#ifdef CONFIG_FAIR_GROUP_SCHED
# ifdef CONFIG_SMP
static long calc_cfs_shares(struct cfs_rq *cfs_rq, struct task_group *tg)
{
	long tg_weight, load, shares;

	/*
	 * This really should be: cfs_rq->avg.load_avg, but instead we use
	 * cfs_rq->load.weight, which is its upper bound. This helps ramp up
	 * the shares for small weight interactive tasks.
	 */
	load = scale_load_down(cfs_rq->load.weight);

	tg_weight = atomic_long_read(&tg->load_avg);

	/* Ensure tg_weight >= load */
	tg_weight -= cfs_rq->tg_load_avg_contrib;
	tg_weight += load;

	shares = (tg->shares * load);
	if (tg_weight)
		shares /= tg_weight;

	if (shares < MIN_SHARES)
		shares = MIN_SHARES;
	if (shares > tg->shares)
		shares = tg->shares;

	return shares;
}
# else /* CONFIG_SMP */
static inline long calc_cfs_shares(struct cfs_rq *cfs_rq, struct task_group *tg)
{
	return tg->shares;
}
# endif /* CONFIG_SMP */

static void reweight_entity(struct cfs_rq *cfs_rq, struct sched_entity *se,
			    unsigned long weight)
{
	if (se->on_rq) {
		/* commit outstanding execution time */
		if (cfs_rq->curr == se)
			update_curr(cfs_rq);
		account_entity_dequeue(cfs_rq, se);
	}

	update_load_set(&se->load, weight);

	if (se->on_rq)
		account_entity_enqueue(cfs_rq, se);
}

static inline int throttled_hierarchy(struct cfs_rq *cfs_rq);

static void update_cfs_shares(struct cfs_rq *cfs_rq)
{
	struct task_group *tg;
	struct sched_entity *se;
	long shares;

	tg = cfs_rq->tg;
	se = tg->se[cpu_of(rq_of(cfs_rq))];
	if (!se || throttled_hierarchy(cfs_rq))
		return;
#ifndef CONFIG_SMP
	if (likely(se->load.weight == tg->shares))
		return;
#endif
	shares = calc_cfs_shares(cfs_rq, tg);

	reweight_entity(cfs_rq_of(se), se, shares);
}
#else /* CONFIG_FAIR_GROUP_SCHED */
static inline void update_cfs_shares(struct cfs_rq *cfs_rq)
{
}
#endif /* CONFIG_FAIR_GROUP_SCHED */

#ifdef CONFIG_SMP
/* Precomputed fixed inverse multiplies for multiplication by y^n */
static const u32 runnable_avg_yN_inv[] = {
	0xffff, 0xf524, 0xeabf, 0xe0cb, 0xd744, 0xce23, 0xc566, 0xbd07,
	0xb504, 0xad57, 0xa5fe, 0x9ef4, 0x9837, 0x91c3, 0x8b95, 0x85aa,
};

/*
 * Precomputed \Sum y^k { 1<=k<=n }.  These are floor(true_value) to prevent
 * over-estimates when re-combining.
 */
static const u32 runnable_avg_yN_sum[] = {
	    0,  980, 1919, 2818, 3679, 4503, 5292, 6048, 6772, 7465, 8129,
	 8764, 9373, 9956,10514,11048,11560,
};

/*
 * Precomputed \Sum y^k { 1<=k<=n, where n%32=0). Values are rolled down to
 * lower integers. See Documentation/scheduler/sched-avg.txt how these
 * were generated:
 */
static const u32 __accumulated_sum_N32[] = {
	    0, 23371, 35056, 40899, 43820, 45281,
	46011, 46376, 46559, 46650, 46696, 46719,
};

/*
 * Approximate:
 *   val * y^n,    where y^32 ~= 0.5 (~1 scheduling period)
 */
static __always_inline u64 decay_load(u64 val, u64 n)
{
	unsigned int local_n;

	if (!n)
		return val;
	else if (unlikely(n > LOAD_AVG_PERIOD * 63))
		return 0;

	/* after bounds checking we can collapse to 32-bit */
	local_n = n;

	/*
	 * As y^PERIOD = 1/2, we can combine
	 *    y^n = 1/2^(n/PERIOD) * y^(n%PERIOD)
	 * With a look-up table which covers y^n (n<PERIOD)
	 *
	 * To achieve constant time decay_load.
	 */
	if (unlikely(local_n >= LOAD_AVG_PERIOD)) {
		val >>= local_n / LOAD_AVG_PERIOD;
		local_n %= LOAD_AVG_PERIOD;
	}

	val = mul_u64_u32_shr(val, runnable_avg_yN_inv[local_n],
			LOAD_AVG_PERIOD);
	return val;
}

/*
 * For updates fully spanning n periods, the contribution to runnable
 * average will be: \Sum 1024*y^n
 *
 * We can compute this reasonably efficiently by combining:
 *   y^PERIOD = 1/2 with precomputed \Sum 1024*y^n {for  n <PERIOD}
 */
static u32 __compute_runnable_contrib(u64 n)
{
	u32 contrib = 0;

	if (likely(n <= LOAD_AVG_PERIOD))
		return runnable_avg_yN_sum[n];
	else if (unlikely(n >= LOAD_AVG_MAX_N))
		return LOAD_AVG_MAX;

	/* Since n < LOAD_AVG_MAX_N, n/LOAD_AVG_PERIOD < 11 */
	contrib = __accumulated_sum_N32[n/LOAD_AVG_PERIOD];
	n %= LOAD_AVG_PERIOD;
	contrib = decay_load(contrib, n);
	return contrib + runnable_avg_yN_sum[n];
}

#define cap_scale(v, s) ((v)*(s) >> SCHED_CAPACITY_SHIFT)

/*
 * We can represent the historical contribution to runnable average as the
 * coefficients of a geometric series.  To do this we sub-divide our runnable
 * history into segments of approximately 1ms (1024us); label the segment that
 * occurred N-ms ago p_N, with p_0 corresponding to the current period, e.g.
 *
 * [<- 1024us ->|<- 1024us ->|<- 1024us ->| ...
 *      p0            p1           p2
 *     (now)       (~1ms ago)  (~2ms ago)
 *
 * Let u_i denote the fraction of p_i that the entity was runnable.
 *
 * We then designate the fractions u_i as our co-efficients, yielding the
 * following representation of historical load:
 *   u_0 + u_1*y + u_2*y^2 + u_3*y^3 + ...
 *
 * We choose y based on the with of a reasonably scheduling period, fixing:
 *   y^32 = 0.5
 *
 * This means that the contribution to load ~32ms ago (u_32) will be weighted
 * approximately half as much as the contribution to load within the last ms
 * (u_0).
 *
 * When a period "rolls over" and we have new u_0`, multiplying the previous
 * sum again by y is sufficient to update:
 *   load_avg = u_0` + y*(u_0 + u_1*y + u_2*y^2 + ... )
 *            = u_0 + u_1*y + u_2*y^2 + ... [re-labeling u_i --> u_{i+1}]
 */
static __always_inline int
__update_load_avg(u64 now, int cpu, struct sched_avg *sa,
		  unsigned long weight, int running, struct cfs_rq *cfs_rq)
{
	u64 delta, scaled_delta, periods;
	u32 contrib;
	unsigned int delta_w, scaled_delta_w, decayed = 0;
	unsigned long arch_scale_freq_capacity(struct sched_domain *sd, int cpu);
	unsigned long scale_freq, scale_cpu;

	delta = now - sa->last_update_time;
	/*
	 * This should only happen when time goes backwards, which it
	 * unfortunately does during sched clock init when we swap over to TSC.
	 */
	if ((s64)delta < 0) {
		sa->last_update_time = now;
		return 0;
	}

	/*
	 * Use 1024ns as the unit of measurement since it's a reasonable
	 * approximation of 1us and fast to compute.
	 */
	delta >>= 10;
	if (!delta)
		return 0;
	sa->last_update_time = now;

	scale_freq = arch_scale_freq_capacity(NULL, cpu);
	scale_cpu = arch_scale_cpu_capacity(NULL, cpu);

	/* delta_w is the amount already accumulated against our next period */
	delta_w = sa->period_contrib;
	if (delta + delta_w >= 1024) {
		decayed = 1;

		/* how much left for next period will start over, we don't know yet */
		sa->period_contrib = 0;

		/*
		 * Now that we know we're crossing a period boundary, figure
		 * out how much from delta we need to complete the current
		 * period and accrue it.
		 */
		delta_w = 1024 - delta_w;
		scaled_delta_w = cap_scale(delta_w, scale_freq);
		if (weight) {
			sa->load_sum += weight * scaled_delta_w;
			if (cfs_rq) {
				cfs_rq->runnable_load_sum +=
						weight * scaled_delta_w;
			}
		}
		if (running)
			sa->util_sum += scaled_delta_w * scale_cpu;

		delta -= delta_w;

		/* Figure out how many additional periods this update spans */
		periods = delta / 1024;
		delta %= 1024;

		sa->load_sum = decay_load(sa->load_sum, periods + 1);
		if (cfs_rq) {
			cfs_rq->runnable_load_sum =
				decay_load(cfs_rq->runnable_load_sum, periods + 1);
		}
		sa->util_sum = decay_load((u64)(sa->util_sum), periods + 1);

		/* Efficiently calculate \sum (1..n_period) 1024*y^i */
		contrib = __compute_runnable_contrib(periods);
		contrib = cap_scale(contrib, scale_freq);
		if (weight) {
			sa->load_sum += weight * contrib;
			if (cfs_rq)
				cfs_rq->runnable_load_sum += weight * contrib;
		}
		if (running)
			sa->util_sum += contrib * scale_cpu;
	}

	/* Remainder of delta accrued against u_0` */
	scaled_delta = cap_scale(delta, scale_freq);
	if (weight) {
		sa->load_sum += weight * scaled_delta;
		if (cfs_rq)
			cfs_rq->runnable_load_sum += weight * scaled_delta;
	}
	if (running)
		sa->util_sum += scaled_delta * scale_cpu;

	sa->period_contrib += delta;

	if (decayed) {
		sa->load_avg = div_u64(sa->load_sum, LOAD_AVG_MAX);
		if (cfs_rq) {
			cfs_rq->runnable_load_avg =
				div_u64(cfs_rq->runnable_load_sum, LOAD_AVG_MAX);
		}
		sa->util_avg = sa->util_sum / LOAD_AVG_MAX;
	}

	return decayed;
}

#ifdef CONFIG_FAIR_GROUP_SCHED
/**
 * update_tg_load_avg - update the tg's load avg
 * @cfs_rq: the cfs_rq whose avg changed
 * @force: update regardless of how small the difference
 *
 * This function 'ensures': tg->load_avg := \Sum tg->cfs_rq[]->avg.load.
 * However, because tg->load_avg is a global value there are performance
 * considerations.
 *
 * In order to avoid having to look at the other cfs_rq's, we use a
 * differential update where we store the last value we propagated. This in
 * turn allows skipping updates if the differential is 'small'.
 *
 * Updating tg's load_avg is necessary before update_cfs_share() (which is
 * done) and effective_load() (which is not done because it is too costly).
 */
static inline void update_tg_load_avg(struct cfs_rq *cfs_rq, int force)
{
	long delta = cfs_rq->avg.load_avg - cfs_rq->tg_load_avg_contrib;

	/*
	 * No need to update load_avg for root_task_group as it is not used.
	 */
	if (cfs_rq->tg == &root_task_group)
		return;

	if (force || abs(delta) > cfs_rq->tg_load_avg_contrib / 64) {
		atomic_long_add(delta, &cfs_rq->tg->load_avg);
		cfs_rq->tg_load_avg_contrib = cfs_rq->avg.load_avg;
	}
}

/*
 * Called within set_task_rq() right before setting a task's cpu. The
 * caller only guarantees p->pi_lock is held; no other assumptions,
 * including the state of rq->lock, should be made.
 */
void set_task_rq_fair(struct sched_entity *se,
		      struct cfs_rq *prev, struct cfs_rq *next)
{
	if (!sched_feat(ATTACH_AGE_LOAD))
		return;

	/*
	 * We are supposed to update the task to "current" time, then its up to
	 * date and ready to go to new CPU/cfs_rq. But we have difficulty in
	 * getting what current time is, so simply throw away the out-of-date
	 * time. This will result in the wakee task is less decayed, but giving
	 * the wakee more load sounds not bad.
	 */
	if (se->avg.last_update_time && prev) {
		u64 p_last_update_time;
		u64 n_last_update_time;

#ifndef CONFIG_64BIT
		u64 p_last_update_time_copy;
		u64 n_last_update_time_copy;

		do {
			p_last_update_time_copy = prev->load_last_update_time_copy;
			n_last_update_time_copy = next->load_last_update_time_copy;

			smp_rmb();

			p_last_update_time = prev->avg.last_update_time;
			n_last_update_time = next->avg.last_update_time;

		} while (p_last_update_time != p_last_update_time_copy ||
			 n_last_update_time != n_last_update_time_copy);
#else
		p_last_update_time = prev->avg.last_update_time;
		n_last_update_time = next->avg.last_update_time;
#endif
		__update_load_avg(p_last_update_time, cpu_of(rq_of(prev)),
				  &se->avg, 0, 0, NULL);
		se->avg.last_update_time = n_last_update_time;
	}
}
#else /* CONFIG_FAIR_GROUP_SCHED */
static inline void update_tg_load_avg(struct cfs_rq *cfs_rq, int force) {}
#endif /* CONFIG_FAIR_GROUP_SCHED */

/*
 * Unsigned subtract and clamp on underflow.
 *
 * Explicitly do a load-store to ensure the intermediate value never hits
 * memory. This allows lockless observations without ever seeing the negative
 * values.
 */
#define sub_positive(_ptr, _val) do {				\
	typeof(_ptr) ptr = (_ptr);				\
	typeof(*ptr) val = (_val);				\
	typeof(*ptr) res, var = READ_ONCE(*ptr);		\
	res = var - val;					\
	if (res > var)						\
		res = 0;					\
	WRITE_ONCE(*ptr, res);					\
} while (0)

/**
 * update_cfs_rq_load_avg - update the cfs_rq's load/util averages
 * @now: current time, as per cfs_rq_clock_task()
 * @cfs_rq: cfs_rq to update
 * @update_freq: should we call cfs_rq_util_change() or will the call do so
 *
 * The cfs_rq avg is the direct sum of all its entities (blocked and runnable)
 * avg. The immediate corollary is that all (fair) tasks must be attached, see
 * post_init_entity_util_avg().
 *
 * cfs_rq->avg is used for task_h_load() and update_cfs_share() for example.
 *
 * Returns true if the load decayed or we removed load.
 *
 * Since both these conditions indicate a changed cfs_rq->avg.load we should
 * call update_tg_load_avg() when this function returns true.
 */
static inline int update_cfs_rq_load_avg(u64 now, struct cfs_rq *cfs_rq)
{
	struct sched_avg *sa = &cfs_rq->avg;
	int decayed, removed = 0;

	if (atomic_long_read(&cfs_rq->removed_load_avg)) {
		s64 r = atomic_long_xchg(&cfs_rq->removed_load_avg, 0);
		sub_positive(&sa->load_avg, r);
		sub_positive(&sa->load_sum, r * LOAD_AVG_MAX);
		removed = 1;
	}

	if (atomic_long_read(&cfs_rq->removed_util_avg)) {
		long r = atomic_long_xchg(&cfs_rq->removed_util_avg, 0);
		sub_positive(&sa->util_avg, r);
		sub_positive(&sa->util_sum, r * LOAD_AVG_MAX);
	}

	decayed = __update_load_avg(now, cpu_of(rq_of(cfs_rq)), sa,
		scale_load_down(cfs_rq->load.weight), cfs_rq->curr != NULL, cfs_rq);

#ifndef CONFIG_64BIT
	smp_wmb();
	cfs_rq->load_last_update_time_copy = sa->last_update_time;
#endif

	return decayed || removed;
}

/* Update task and its cfs_rq load average */
static inline void update_load_avg(struct sched_entity *se, int update_tg)
{
	struct cfs_rq *cfs_rq = cfs_rq_of(se);
	u64 now = cfs_rq_clock_task(cfs_rq);
	int cpu = cpu_of(rq_of(cfs_rq));

	/*
	 * Track task load average for carrying it to new CPU after migrated, and
	 * track group sched_entity load average for task_h_load calc in migration
	 */
	__update_load_avg(now, cpu, &se->avg,
			  se->on_rq * scale_load_down(se->load.weight),
			  cfs_rq->curr == se, NULL);

	if (update_cfs_rq_load_avg(now, cfs_rq) && update_tg)
		update_tg_load_avg(cfs_rq, 0);
}

/**
 * attach_entity_load_avg - attach this entity to its cfs_rq load avg
 * @cfs_rq: cfs_rq to attach to
 * @se: sched_entity to attach
 *
 * Must call update_cfs_rq_load_avg() before this, since we rely on
 * cfs_rq->avg.last_update_time being current.
 */
static void attach_entity_load_avg(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	if (!sched_feat(ATTACH_AGE_LOAD))
		goto skip_aging;

	/*
	 * If we got migrated (either between CPUs or between cgroups) we'll
	 * have aged the average right before clearing @last_update_time.
	 *
	 * Or we're fresh through post_init_entity_util_avg().
	 */
	if (se->avg.last_update_time) {
		__update_load_avg(cfs_rq->avg.last_update_time, cpu_of(rq_of(cfs_rq)),
				  &se->avg, 0, 0, NULL);

		/*
		 * XXX: we could have just aged the entire load away if we've been
		 * absent from the fair class for too long.
		 */
	}

skip_aging:
	se->avg.last_update_time = cfs_rq->avg.last_update_time;
	cfs_rq->avg.load_avg += se->avg.load_avg;
	cfs_rq->avg.load_sum += se->avg.load_sum;
	cfs_rq->avg.util_avg += se->avg.util_avg;
	cfs_rq->avg.util_sum += se->avg.util_sum;
}

/**
 * detach_entity_load_avg - detach this entity from its cfs_rq load avg
 * @cfs_rq: cfs_rq to detach from
 * @se: sched_entity to detach
 *
 * Must call update_cfs_rq_load_avg() before this, since we rely on
 * cfs_rq->avg.last_update_time being current.
 */
static void detach_entity_load_avg(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	__update_load_avg(cfs_rq->avg.last_update_time, cpu_of(rq_of(cfs_rq)),
			  &se->avg, se->on_rq * scale_load_down(se->load.weight),
			  cfs_rq->curr == se, NULL);

	sub_positive(&cfs_rq->avg.load_avg, se->avg.load_avg);
	sub_positive(&cfs_rq->avg.load_sum, se->avg.load_sum);
	sub_positive(&cfs_rq->avg.util_avg, se->avg.util_avg);
	sub_positive(&cfs_rq->avg.util_sum, se->avg.util_sum);
}

/* Add the load generated by se into cfs_rq's load average */
static inline void
enqueue_entity_load_avg(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	struct sched_avg *sa = &se->avg;
	u64 now = cfs_rq_clock_task(cfs_rq);
	int migrated, decayed;

	migrated = !sa->last_update_time;
	if (!migrated) {
		__update_load_avg(now, cpu_of(rq_of(cfs_rq)), sa,
			se->on_rq * scale_load_down(se->load.weight),
			cfs_rq->curr == se, NULL);
	}

	decayed = update_cfs_rq_load_avg(now, cfs_rq);

	cfs_rq->runnable_load_avg += sa->load_avg;
	cfs_rq->runnable_load_sum += sa->load_sum;

	if (migrated)
		attach_entity_load_avg(cfs_rq, se);

	if (decayed || migrated)
		update_tg_load_avg(cfs_rq, 0);
}

/* Remove the runnable load generated by se from cfs_rq's runnable load average */
static inline void
dequeue_entity_load_avg(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	update_load_avg(se, 1);

	cfs_rq->runnable_load_avg =
		max_t(long, cfs_rq->runnable_load_avg - se->avg.load_avg, 0);
	cfs_rq->runnable_load_sum =
		max_t(s64, cfs_rq->runnable_load_sum - se->avg.load_sum, 0);
}

#ifndef CONFIG_64BIT
static inline u64 cfs_rq_last_update_time(struct cfs_rq *cfs_rq)
{
	u64 last_update_time_copy;
	u64 last_update_time;

	do {
		last_update_time_copy = cfs_rq->load_last_update_time_copy;
		smp_rmb();
		last_update_time = cfs_rq->avg.last_update_time;
	} while (last_update_time != last_update_time_copy);

	return last_update_time;
}
#else
static inline u64 cfs_rq_last_update_time(struct cfs_rq *cfs_rq)
{
	return cfs_rq->avg.last_update_time;
}
#endif

/*
 * Task first catches up with cfs_rq, and then subtract
 * itself from the cfs_rq (task must be off the queue now).
 */
void remove_entity_load_avg(struct sched_entity *se)
{
	struct cfs_rq *cfs_rq = cfs_rq_of(se);
	u64 last_update_time;

	/*
	 * tasks cannot exit without having gone through wake_up_new_task() ->
	 * post_init_entity_util_avg() which will have added things to the
	 * cfs_rq, so we can remove unconditionally.
	 *
	 * Similarly for groups, they will have passed through
	 * post_init_entity_util_avg() before unregister_sched_fair_group()
	 * calls this.
	 */

	last_update_time = cfs_rq_last_update_time(cfs_rq);

	__update_load_avg(last_update_time, cpu_of(rq_of(cfs_rq)), &se->avg, 0, 0, NULL);
	atomic_long_add(se->avg.load_avg, &cfs_rq->removed_load_avg);
	atomic_long_add(se->avg.util_avg, &cfs_rq->removed_util_avg);
}

static inline unsigned long cfs_rq_runnable_load_avg(struct cfs_rq *cfs_rq)
{
	return cfs_rq->runnable_load_avg;
}

static inline unsigned long cfs_rq_load_avg(struct cfs_rq *cfs_rq)
{
	return cfs_rq->avg.load_avg;
}

static int idle_balance(struct rq *this_rq);

#else /* CONFIG_SMP */

static inline int
update_cfs_rq_load_avg(u64 now, struct cfs_rq *cfs_rq)
{
	return 0;
}

static inline void update_load_avg(struct sched_entity *se, int not_used)
{
	struct cfs_rq *cfs_rq = cfs_rq_of(se);
	struct rq *rq = rq_of(cfs_rq);

	cpufreq_trigger_update(rq_clock(rq));
}

static inline void
enqueue_entity_load_avg(struct cfs_rq *cfs_rq, struct sched_entity *se) {}
static inline void
dequeue_entity_load_avg(struct cfs_rq *cfs_rq, struct sched_entity *se) {}
static inline void remove_entity_load_avg(struct sched_entity *se) {}

static inline void
attach_entity_load_avg(struct cfs_rq *cfs_rq, struct sched_entity *se) {}
static inline void
detach_entity_load_avg(struct cfs_rq *cfs_rq, struct sched_entity *se) {}

static inline int idle_balance(struct rq *rq)
{
	return 0;
}

#endif /* CONFIG_SMP */

static void enqueue_sleeper(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
#ifdef CONFIG_SCHEDSTATS
	struct task_struct *tsk = NULL;

	if (entity_is_task(se))
		tsk = task_of(se);

	if (se->statistics.sleep_start) {
		u64 delta = rq_clock(rq_of(cfs_rq)) - se->statistics.sleep_start;

		if ((s64)delta < 0)
			delta = 0;

		if (unlikely(delta > se->statistics.sleep_max))
			se->statistics.sleep_max = delta;

		se->statistics.sleep_start = 0;
		se->statistics.sum_sleep_runtime += delta;

		if (tsk) {
			account_scheduler_latency(tsk, delta >> 10, 1);
			trace_sched_stat_sleep(tsk, delta);
		}
	}
	if (se->statistics.block_start) {
		u64 delta = rq_clock(rq_of(cfs_rq)) - se->statistics.block_start;

		if ((s64)delta < 0)
			delta = 0;

		if (unlikely(delta > se->statistics.block_max))
			se->statistics.block_max = delta;

		se->statistics.block_start = 0;
		se->statistics.sum_sleep_runtime += delta;

		if (tsk) {
			if (tsk->in_iowait) {
				se->statistics.iowait_sum += delta;
				se->statistics.iowait_count++;
				trace_sched_stat_iowait(tsk, delta);
			}

			trace_sched_stat_blocked(tsk, delta);
			trace_sched_blocked_reason(tsk);

			/*
			 * Blocking time is in units of nanosecs, so shift by
			 * 20 to get a milliseconds-range estimation of the
			 * amount of time that the task spent sleeping:
			 */
			if (unlikely(prof_on == SLEEP_PROFILING)) {
				profile_hits(SLEEP_PROFILING,
						(void *)get_wchan(tsk),
						delta >> 20);
			}
			account_scheduler_latency(tsk, delta >> 10, 0);
		}
	}
#endif
}

static void check_spread(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
#ifdef CONFIG_SCHED_DEBUG
	s64 d = se->vruntime - cfs_rq->min_vruntime;

	if (d < 0)
		d = -d;

	if (d > 3*sysctl_sched_latency)
		schedstat_inc(cfs_rq, nr_spread_over);
#endif
}

unsigned int Lgentle_fair_sleepers = 1;
unsigned int Larch_power = 0;

void relay_gfs(unsigned int gfs)
{
	Lgentle_fair_sleepers = gfs;
}

void relay_ap(unsigned int ap)
{
	Larch_power = ap;
}

static void
place_entity(struct cfs_rq *cfs_rq, struct sched_entity *se, int initial)
{
	u64 vruntime = cfs_rq->min_vruntime;

	/*
	 * The 'current' period is already promised to the current tasks,
	 * however the extra weight of the new task will slow them down a
	 * little, place the new task so that it fits in the slot that
	 * stays open at the end.
	 */
	if (initial && sched_feat(START_DEBIT))
		vruntime += sched_vslice(cfs_rq, se);

	/* sleeps up to a single latency don't count. */
	if (!initial) {
		unsigned long thresh = sysctl_sched_latency;

		/*
		 * Halve their sleep time's effect, to allow
		 * for a gentler effect of sleepers:
		 */
		if (Lgentle_fair_sleepers)
			thresh >>= 1;

		vruntime -= thresh;
	}

	/* ensure we never gain time by being placed backwards. */
	se->vruntime = max_vruntime(se->vruntime, vruntime);
}

static void check_enqueue_throttle(struct cfs_rq *cfs_rq);

static void
enqueue_entity(struct cfs_rq *cfs_rq, struct sched_entity *se, int flags)
{
	bool renorm = !(flags & ENQUEUE_WAKEUP) || (flags & ENQUEUE_MIGRATED);
	bool curr = cfs_rq->curr == se;

	/*
	 * If we're the current task, we must renormalise before calling
	 * update_curr().
	 */
	if (renorm && curr)
		se->vruntime += cfs_rq->min_vruntime;

	update_curr(cfs_rq);

	/*
	 * Otherwise, renormalise after, such that we're placed at the current
	 * moment in time, instead of some random moment in the past. Being
	 * placed in the past could significantly boost this task to the
	 * fairness detriment of existing tasks.
	 */
	if (renorm && !curr)
		se->vruntime += cfs_rq->min_vruntime;

	enqueue_entity_load_avg(cfs_rq, se);
	account_entity_enqueue(cfs_rq, se);
	update_cfs_shares(cfs_rq);

	if (flags & ENQUEUE_WAKEUP) {
		place_entity(cfs_rq, se, 0);
		enqueue_sleeper(cfs_rq, se);
	}

	update_stats_enqueue(cfs_rq, se);
	check_spread(cfs_rq, se);
	if (!curr)
		__enqueue_entity(cfs_rq, se);
	se->on_rq = 1;

	if (cfs_rq->nr_running == 1) {
		list_add_leaf_cfs_rq(cfs_rq);
		check_enqueue_throttle(cfs_rq);
	}
}

static void __clear_buddies_last(struct sched_entity *se)
{
	for_each_sched_entity(se) {
		struct cfs_rq *cfs_rq = cfs_rq_of(se);
		if (cfs_rq->last != se)
			break;

		cfs_rq->last = NULL;
	}
}

static void __clear_buddies_next(struct sched_entity *se)
{
	for_each_sched_entity(se) {
		struct cfs_rq *cfs_rq = cfs_rq_of(se);
		if (cfs_rq->next != se)
			break;

		cfs_rq->next = NULL;
	}
}

static void __clear_buddies_skip(struct sched_entity *se)
{
	for_each_sched_entity(se) {
		struct cfs_rq *cfs_rq = cfs_rq_of(se);
		if (cfs_rq->skip != se)
			break;

		cfs_rq->skip = NULL;
	}
}

static void clear_buddies(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	if (cfs_rq->last == se)
		__clear_buddies_last(se);

	if (cfs_rq->next == se)
		__clear_buddies_next(se);

	if (cfs_rq->skip == se)
		__clear_buddies_skip(se);
}

static __always_inline void return_cfs_rq_runtime(struct cfs_rq *cfs_rq);

static void
dequeue_entity(struct cfs_rq *cfs_rq, struct sched_entity *se, int flags)
{
	/*
	 * Update run-time statistics of the 'current'.
	 */
	update_curr(cfs_rq);
	dequeue_entity_load_avg(cfs_rq, se);

	update_stats_dequeue(cfs_rq, se);
	if (flags & DEQUEUE_SLEEP) {
#ifdef CONFIG_SCHEDSTATS
		if (entity_is_task(se)) {
			struct task_struct *tsk = task_of(se);

			if (tsk->state & TASK_INTERRUPTIBLE)
				se->statistics.sleep_start = rq_clock(rq_of(cfs_rq));
			if (tsk->state & TASK_UNINTERRUPTIBLE)
				se->statistics.block_start = rq_clock(rq_of(cfs_rq));
		}
#endif
	}

	clear_buddies(cfs_rq, se);

	if (se != cfs_rq->curr)
		__dequeue_entity(cfs_rq, se);
	se->on_rq = 0;
	account_entity_dequeue(cfs_rq, se);

	/*
	 * Normalize after update_curr(); which will also have moved
	 * min_vruntime if @se is the one holding it back. But before doing
	 * update_min_vruntime() again, which will discount @se's position and
	 * can move min_vruntime forward still more.
	 */
	if (!(flags & DEQUEUE_SLEEP))
		se->vruntime -= cfs_rq->min_vruntime;

	/* return excess runtime on last dequeue */
	return_cfs_rq_runtime(cfs_rq);

	update_cfs_shares(cfs_rq);

	/*
	 * Now advance min_vruntime if @se was the entity holding it back,
	 * except when: DEQUEUE_SAVE && !DEQUEUE_MOVE, in this case we'll be
	 * put back on, and if we advance min_vruntime, we'll be placed back
	 * further than we started -- ie. we'll be penalized.
	 */
	if ((flags & (DEQUEUE_SAVE | DEQUEUE_MOVE)) == DEQUEUE_SAVE)
		update_min_vruntime(cfs_rq);
}

/*
 * Preempt the current task with a newly woken task if needed:
 */
static void
check_preempt_tick(struct cfs_rq *cfs_rq, struct sched_entity *curr)
{
	unsigned long ideal_runtime, delta_exec;
	struct sched_entity *se;
	s64 delta;

	ideal_runtime = sched_slice(cfs_rq, curr);
	delta_exec = curr->sum_exec_runtime - curr->prev_sum_exec_runtime;
	if (delta_exec > ideal_runtime) {
		resched_curr(rq_of(cfs_rq));
		/*
		 * The current task ran long enough, ensure it doesn't get
		 * re-elected due to buddy favours.
		 */
		clear_buddies(cfs_rq, curr);
		return;
	}

	/*
	 * Ensure that a task that missed wakeup preemption by a
	 * narrow margin doesn't have to wait for a full slice.
	 * This also mitigates buddy induced latencies under load.
	 */
	if (delta_exec < sysctl_sched_min_granularity)
		return;

	se = __pick_first_entity(cfs_rq);
	delta = curr->vruntime - se->vruntime;

	if (delta < 0)
		return;

	if (delta > ideal_runtime)
		resched_curr(rq_of(cfs_rq));
}


/*
 * MIGRATION
 *
 *	dequeue
 *	  update_curr()
 *	    update_min_vruntime()
 *	  vruntime -= min_vruntime
 *
 *	enqueue
 *	  update_curr()
 *	    update_min_vruntime()
 *	  vruntime += min_vruntime
 *
 * this way the vruntime transition between RQs is done when both
 * min_vruntime are up-to-date.
 *
 * WAKEUP (remote)
 *
 *	->migrate_task_rq_fair() (p->state == TASK_WAKING)
 *	  vruntime -= min_vruntime
 *
 *	enqueue
 *	  update_curr()
 *	    update_min_vruntime()
 *	  vruntime += min_vruntime
 *
 * this way we don't have the most up-to-date min_vruntime on the originating
 * CPU and an up-to-date min_vruntime on the destination CPU.
 */

static void
set_next_entity(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	/* 'current' is not kept within the tree. */
	if (se->on_rq) {
		/*
		 * Any task has to be enqueued before it get to execute on
		 * a CPU. So account for the time it spent waiting on the
		 * runqueue.
		 */
		update_stats_wait_end(cfs_rq, se);
		__dequeue_entity(cfs_rq, se);
		update_load_avg(se, 1);
	}

	update_stats_curr_start(cfs_rq, se);
	cfs_rq->curr = se;
#ifdef CONFIG_SCHEDSTATS
	/*
	 * Track our maximum slice length, if the CPU's load is at
	 * least twice that of our own weight (i.e. dont track it
	 * when there are only lesser-weight tasks around):
	 */
	if (rq_of(cfs_rq)->load.weight >= 2*se->load.weight) {
		se->statistics.slice_max = max(se->statistics.slice_max,
			se->sum_exec_runtime - se->prev_sum_exec_runtime);
	}
#endif
	se->prev_sum_exec_runtime = se->sum_exec_runtime;
}

static int
wakeup_preempt_entity(struct sched_entity *curr, struct sched_entity *se);

/*
 * Pick the next process, keeping these things in mind, in this order:
 * 1) keep things fair between processes/task groups
 * 2) pick the "next" process, since someone really wants that to run
 * 3) pick the "last" process, for cache locality
 * 4) do not run the "skip" process, if something else is available
 */
static struct sched_entity *
pick_next_entity(struct cfs_rq *cfs_rq, struct sched_entity *curr)
{
	struct sched_entity *left = __pick_first_entity(cfs_rq);
	struct sched_entity *se;

	/*
	 * If curr is set we have to see if its left of the leftmost entity
	 * still in the tree, provided there was anything in the tree at all.
	 */
	if (!left || (curr && entity_before(curr, left)))
		left = curr;

	se = left; /* ideally we run the leftmost entity */

	/*
	 * Avoid running the skip buddy, if running something else can
	 * be done without getting too unfair.
	 */
	if (cfs_rq->skip == se) {
		struct sched_entity *second;

		if (se == curr) {
			second = __pick_first_entity(cfs_rq);
		} else {
			second = __pick_next_entity(se);
			if (!second || (curr && entity_before(curr, second)))
				second = curr;
		}

		if (second && wakeup_preempt_entity(second, left) < 1)
			se = second;
	}

	/*
	 * Prefer last buddy, try to return the CPU to a preempted task.
	 */
	if (cfs_rq->last && wakeup_preempt_entity(cfs_rq->last, left) < 1)
		se = cfs_rq->last;

	/*
	 * Someone really wants this to run. If it's not unfair, run it.
	 */
	if (cfs_rq->next && wakeup_preempt_entity(cfs_rq->next, left) < 1)
		se = cfs_rq->next;

	clear_buddies(cfs_rq, se);

	return se;
}

static bool check_cfs_rq_runtime(struct cfs_rq *cfs_rq);

static void put_prev_entity(struct cfs_rq *cfs_rq, struct sched_entity *prev)
{
	/*
	 * If still on the runqueue then deactivate_task()
	 * was not called and update_curr() has to be done:
	 */
	if (prev->on_rq)
		update_curr(cfs_rq);

	/* throttle cfs_rqs exceeding runtime */
	check_cfs_rq_runtime(cfs_rq);

	check_spread(cfs_rq, prev);
	if (prev->on_rq) {
		update_stats_wait_start(cfs_rq, prev);
		/* Put 'current' back into the tree. */
		__enqueue_entity(cfs_rq, prev);
		/* in !on_rq case, update occurred at dequeue */
		update_load_avg(prev, 0);
	}
	cfs_rq->curr = NULL;
}

static void
entity_tick(struct cfs_rq *cfs_rq, struct sched_entity *curr, int queued)
{
	/*
	 * Update run-time statistics of the 'current'.
	 */
	update_curr(cfs_rq);

	/*
	 * Ensure that runnable average is periodically updated.
	 */
	update_load_avg(curr, 1);
	update_cfs_shares(cfs_rq);

#ifdef CONFIG_SCHED_HRTICK
	/*
	 * queued ticks are scheduled to match the slice, so don't bother
	 * validating it and just reschedule.
	 */
	if (queued) {
		resched_curr(rq_of(cfs_rq));
		return;
	}
	/*
	 * don't let the period tick interfere with the hrtick preemption
	 */
	if (!sched_feat(DOUBLE_TICK) &&
			hrtimer_active(&rq_of(cfs_rq)->hrtick_timer))
		return;
#endif

	if (cfs_rq->nr_running > 1)
		check_preempt_tick(cfs_rq, curr);
}


/**************************************************
 * CFS bandwidth control machinery
 */

#ifdef CONFIG_CFS_BANDWIDTH

#ifdef HAVE_JUMP_LABEL
static struct static_key __cfs_bandwidth_used;

static inline bool cfs_bandwidth_used(void)
{
	return static_key_false(&__cfs_bandwidth_used);
}

void cfs_bandwidth_usage_inc(void)
{
	static_key_slow_inc(&__cfs_bandwidth_used);
}

void cfs_bandwidth_usage_dec(void)
{
	static_key_slow_dec(&__cfs_bandwidth_used);
}
#else /* HAVE_JUMP_LABEL */
static bool cfs_bandwidth_used(void)
{
	return true;
}

void cfs_bandwidth_usage_inc(void) {}
void cfs_bandwidth_usage_dec(void) {}
#endif /* HAVE_JUMP_LABEL */

/*
 * default period for cfs group bandwidth.
 * default: 0.1s, units: nanoseconds
 */
static inline u64 default_cfs_period(void)
{
	return 100000000ULL;
}

static inline u64 sched_cfs_bandwidth_slice(void)
{
	return (u64)sysctl_sched_cfs_bandwidth_slice * NSEC_PER_USEC;
}

/*
 * Replenish runtime according to assigned quota and update expiration time.
 * We use sched_clock_cpu directly instead of rq->clock to avoid adding
 * additional synchronization around rq->lock.
 *
 * requires cfs_b->lock
 */
void __refill_cfs_bandwidth_runtime(struct cfs_bandwidth *cfs_b)
{
	u64 now;

	if (cfs_b->quota == RUNTIME_INF)
		return;

	now = sched_clock_cpu(smp_processor_id());
	cfs_b->runtime = cfs_b->quota;
	cfs_b->runtime_expires = now + ktime_to_ns(cfs_b->period);
}

static inline struct cfs_bandwidth *tg_cfs_bandwidth(struct task_group *tg)
{
	return &tg->cfs_bandwidth;
}

/* rq->task_clock normalized against any time this cfs_rq has spent throttled */
static inline u64 cfs_rq_clock_task(struct cfs_rq *cfs_rq)
{
	if (unlikely(cfs_rq->throttle_count))
		return cfs_rq->throttled_clock_task - cfs_rq->throttled_clock_task_time;

	return rq_clock_task(rq_of(cfs_rq)) - cfs_rq->throttled_clock_task_time;
}

/* returns 0 on failure to allocate runtime */
static int assign_cfs_rq_runtime(struct cfs_rq *cfs_rq)
{
	struct task_group *tg = cfs_rq->tg;
	struct cfs_bandwidth *cfs_b = tg_cfs_bandwidth(tg);
	u64 amount = 0, min_amount, expires;

	/* note: this is a positive sum as runtime_remaining <= 0 */
	min_amount = sched_cfs_bandwidth_slice() - cfs_rq->runtime_remaining;

	raw_spin_lock(&cfs_b->lock);
	if (cfs_b->quota == RUNTIME_INF)
		amount = min_amount;
	else {
		start_cfs_bandwidth(cfs_b);

		if (cfs_b->runtime > 0) {
			amount = min(cfs_b->runtime, min_amount);
			cfs_b->runtime -= amount;
			cfs_b->idle = 0;
		}
	}
	expires = cfs_b->runtime_expires;
	raw_spin_unlock(&cfs_b->lock);

	cfs_rq->runtime_remaining += amount;
	/*
	 * we may have advanced our local expiration to account for allowed
	 * spread between our sched_clock and the one on which runtime was
	 * issued.
	 */
	if ((s64)(expires - cfs_rq->runtime_expires) > 0)
		cfs_rq->runtime_expires = expires;

	return cfs_rq->runtime_remaining > 0;
}

/*
 * Note: This depends on the synchronization provided by sched_clock and the
 * fact that rq->clock snapshots this value.
 */
static void expire_cfs_rq_runtime(struct cfs_rq *cfs_rq)
{
	struct cfs_bandwidth *cfs_b = tg_cfs_bandwidth(cfs_rq->tg);

	/* if the deadline is ahead of our clock, nothing to do */
	if (likely((s64)(rq_clock(rq_of(cfs_rq)) - cfs_rq->runtime_expires) < 0))
		return;

	if (cfs_rq->runtime_remaining < 0)
		return;

	/*
	 * If the local deadline has passed we have to consider the
	 * possibility that our sched_clock is 'fast' and the global deadline
	 * has not truly expired.
	 *
	 * Fortunately we can check determine whether this the case by checking
	 * whether the global deadline has advanced. It is valid to compare
	 * cfs_b->runtime_expires without any locks since we only care about
	 * exact equality, so a partial write will still work.
	 */

	if (cfs_rq->runtime_expires != cfs_b->runtime_expires) {
		/* extend local deadline, drift is bounded above by 2 ticks */
		cfs_rq->runtime_expires += TICK_NSEC;
	} else {
		/* global deadline is ahead, expiration has passed */
		cfs_rq->runtime_remaining = 0;
	}
}

static void __account_cfs_rq_runtime(struct cfs_rq *cfs_rq, u64 delta_exec)
{
	/* dock delta_exec before expiring quota (as it could span periods) */
	cfs_rq->runtime_remaining -= delta_exec;
	expire_cfs_rq_runtime(cfs_rq);

	if (likely(cfs_rq->runtime_remaining > 0))
		return;

	/*
	 * if we're unable to extend our runtime we resched so that the active
	 * hierarchy can be throttled
	 */
	if (!assign_cfs_rq_runtime(cfs_rq) && likely(cfs_rq->curr))
		resched_curr(rq_of(cfs_rq));
}

static __always_inline
void account_cfs_rq_runtime(struct cfs_rq *cfs_rq, u64 delta_exec)
{
	if (!cfs_bandwidth_used() || !cfs_rq->runtime_enabled)
		return;

	__account_cfs_rq_runtime(cfs_rq, delta_exec);
}

static inline int cfs_rq_throttled(struct cfs_rq *cfs_rq)
{
	return cfs_bandwidth_used() && cfs_rq->throttled;
}

/* check whether cfs_rq, or any parent, is throttled */
static inline int throttled_hierarchy(struct cfs_rq *cfs_rq)
{
	return cfs_bandwidth_used() && cfs_rq->throttle_count;
}

/*
 * Ensure that neither of the group entities corresponding to src_cpu or
 * dest_cpu are members of a throttled hierarchy when performing group
 * load-balance operations.
 */
static inline int throttled_lb_pair(struct task_group *tg,
				    int src_cpu, int dest_cpu)
{
	struct cfs_rq *src_cfs_rq, *dest_cfs_rq;

	src_cfs_rq = tg->cfs_rq[src_cpu];
	dest_cfs_rq = tg->cfs_rq[dest_cpu];

	return throttled_hierarchy(src_cfs_rq) ||
	       throttled_hierarchy(dest_cfs_rq);
}

/* updated child weight may affect parent so we have to do this bottom up */
static int tg_unthrottle_up(struct task_group *tg, void *data)
{
	struct rq *rq = data;
	struct cfs_rq *cfs_rq = tg->cfs_rq[cpu_of(rq)];

	cfs_rq->throttle_count--;
	if (!cfs_rq->throttle_count) {
		/* adjust cfs_rq_clock_task() */
		cfs_rq->throttled_clock_task_time += rq_clock_task(rq) -
					     cfs_rq->throttled_clock_task;
	}

	return 0;
}

static int tg_throttle_down(struct task_group *tg, void *data)
{
	struct rq *rq = data;
	struct cfs_rq *cfs_rq = tg->cfs_rq[cpu_of(rq)];

	/* group is entering throttled state, stop time */
	if (!cfs_rq->throttle_count)
		cfs_rq->throttled_clock_task = rq_clock_task(rq);
	cfs_rq->throttle_count++;

	return 0;
}

static void throttle_cfs_rq(struct cfs_rq *cfs_rq)
{
	struct rq *rq = rq_of(cfs_rq);
	struct cfs_bandwidth *cfs_b = tg_cfs_bandwidth(cfs_rq->tg);
	struct sched_entity *se;
	long task_delta, dequeue = 1;
	bool empty;

	se = cfs_rq->tg->se[cpu_of(rq_of(cfs_rq))];

	/* freeze hierarchy runnable averages while throttled */
	rcu_read_lock();
	walk_tg_tree_from(cfs_rq->tg, tg_throttle_down, tg_nop, (void *)rq);
	rcu_read_unlock();

	task_delta = cfs_rq->h_nr_running;
	for_each_sched_entity(se) {
		struct cfs_rq *qcfs_rq = cfs_rq_of(se);
		/* throttled entity or throttle-on-deactivate */
		if (!se->on_rq)
			break;

		if (dequeue)
			dequeue_entity(qcfs_rq, se, DEQUEUE_SLEEP);
		qcfs_rq->h_nr_running -= task_delta;

		if (qcfs_rq->load.weight)
			dequeue = 0;
	}

	if (!se)
		sub_nr_running(rq, task_delta);

	cfs_rq->throttled = 1;
	cfs_rq->throttled_clock = rq_clock(rq);
	raw_spin_lock(&cfs_b->lock);
	empty = list_empty(&cfs_b->throttled_cfs_rq);

	/*
	 * Add to the _head_ of the list, so that an already-started
	 * distribute_cfs_runtime will not see us
	 */
	list_add_rcu(&cfs_rq->throttled_list, &cfs_b->throttled_cfs_rq);

	/*
	 * If we're the first throttled task, make sure the bandwidth
	 * timer is running.
	 */
	if (empty)
		start_cfs_bandwidth(cfs_b);

	raw_spin_unlock(&cfs_b->lock);
}

void unthrottle_cfs_rq(struct cfs_rq *cfs_rq)
{
	struct rq *rq = rq_of(cfs_rq);
	struct cfs_bandwidth *cfs_b = tg_cfs_bandwidth(cfs_rq->tg);
	struct sched_entity *se;
	int enqueue = 1;
	long task_delta;

	se = cfs_rq->tg->se[cpu_of(rq)];

	cfs_rq->throttled = 0;

	update_rq_clock(rq);

	raw_spin_lock(&cfs_b->lock);
	cfs_b->throttled_time += rq_clock(rq) - cfs_rq->throttled_clock;
	list_del_rcu(&cfs_rq->throttled_list);
	raw_spin_unlock(&cfs_b->lock);

	/* update hierarchical throttle state */
	walk_tg_tree_from(cfs_rq->tg, tg_nop, tg_unthrottle_up, (void *)rq);

	if (!cfs_rq->load.weight)
		return;

	task_delta = cfs_rq->h_nr_running;
	for_each_sched_entity(se) {
		if (se->on_rq)
			enqueue = 0;

		cfs_rq = cfs_rq_of(se);
		if (enqueue)
			enqueue_entity(cfs_rq, se, ENQUEUE_WAKEUP);
		cfs_rq->h_nr_running += task_delta;

		if (cfs_rq_throttled(cfs_rq))
			break;
	}

	if (!se)
		add_nr_running(rq, task_delta);

	/* determine whether we need to wake up potentially idle cpu */
	if (rq->curr == rq->idle && rq->cfs.nr_running)
		resched_curr(rq);
}

static u64 distribute_cfs_runtime(struct cfs_bandwidth *cfs_b,
		u64 remaining, u64 expires)
{
	struct cfs_rq *cfs_rq;
	u64 runtime;
	u64 starting_runtime = remaining;

	rcu_read_lock();
	list_for_each_entry_rcu(cfs_rq, &cfs_b->throttled_cfs_rq,
				throttled_list) {
		struct rq *rq = rq_of(cfs_rq);

		raw_spin_lock(&rq->lock);
		if (!cfs_rq_throttled(cfs_rq))
			goto next;

		runtime = -cfs_rq->runtime_remaining + 1;
		if (runtime > remaining)
			runtime = remaining;
		remaining -= runtime;

		cfs_rq->runtime_remaining += runtime;
		cfs_rq->runtime_expires = expires;

		/* we check whether we're throttled above */
		if (cfs_rq->runtime_remaining > 0)
			unthrottle_cfs_rq(cfs_rq);

next:
		raw_spin_unlock(&rq->lock);

		if (!remaining)
			break;
	}
	rcu_read_unlock();

	return starting_runtime - remaining;
}

/*
 * Responsible for refilling a task_group's bandwidth and unthrottling its
 * cfs_rqs as appropriate. If there has been no activity within the last
 * period the timer is deactivated until scheduling resumes; cfs_b->idle is
 * used to track this state.
 */
static int do_sched_cfs_period_timer(struct cfs_bandwidth *cfs_b, int overrun)
{
	u64 runtime, runtime_expires;
	int throttled;

	/* no need to continue the timer with no bandwidth constraint */
	if (cfs_b->quota == RUNTIME_INF)
		goto out_deactivate;

	throttled = !list_empty(&cfs_b->throttled_cfs_rq);
	cfs_b->nr_periods += overrun;

	/*
	 * idle depends on !throttled (for the case of a large deficit), and if
	 * we're going inactive then everything else can be deferred
	 */
	if (cfs_b->idle && !throttled)
		goto out_deactivate;

	__refill_cfs_bandwidth_runtime(cfs_b);

	if (!throttled) {
		/* mark as potentially idle for the upcoming period */
		cfs_b->idle = 1;
		return 0;
	}

	/* account preceding periods in which throttling occurred */
	cfs_b->nr_throttled += overrun;

	runtime_expires = cfs_b->runtime_expires;

	/*
	 * This check is repeated as we are holding onto the new bandwidth while
	 * we unthrottle. This can potentially race with an unthrottled group
	 * trying to acquire new bandwidth from the global pool. This can result
	 * in us over-using our runtime if it is all used during this loop, but
	 * only by limited amounts in that extreme case.
	 */
	while (throttled && cfs_b->runtime > 0) {
		runtime = cfs_b->runtime;
		raw_spin_unlock(&cfs_b->lock);
		/* we can't nest cfs_b->lock while distributing bandwidth */
		runtime = distribute_cfs_runtime(cfs_b, runtime,
						 runtime_expires);
		raw_spin_lock(&cfs_b->lock);

		throttled = !list_empty(&cfs_b->throttled_cfs_rq);

		cfs_b->runtime -= min(runtime, cfs_b->runtime);
	}

	/*
	 * While we are ensured activity in the period following an
	 * unthrottle, this also covers the case in which the new bandwidth is
	 * insufficient to cover the existing bandwidth deficit.  (Forcing the
	 * timer to remain active while there are any throttled entities.)
	 */
	cfs_b->idle = 0;

	return 0;

out_deactivate:
	return 1;
}

/* a cfs_rq won't donate quota below this amount */
static const u64 min_cfs_rq_runtime = 1 * NSEC_PER_MSEC;
/* minimum remaining period time to redistribute slack quota */
static const u64 min_bandwidth_expiration = 2 * NSEC_PER_MSEC;
/* how long we wait to gather additional slack before distributing */
static const u64 cfs_bandwidth_slack_period = 5 * NSEC_PER_MSEC;

/*
 * Are we near the end of the current quota period?
 *
 * Requires cfs_b->lock for hrtimer_expires_remaining to be safe against the
 * hrtimer base being cleared by hrtimer_start. In the case of
 * migrate_hrtimers, base is never cleared, so we are fine.
 */
static int runtime_refresh_within(struct cfs_bandwidth *cfs_b, u64 min_expire)
{
	struct hrtimer *refresh_timer = &cfs_b->period_timer;
	u64 remaining;

	/* if the call-back is running a quota refresh is already occurring */
	if (hrtimer_callback_running(refresh_timer))
		return 1;

	/* is a quota refresh about to occur? */
	remaining = ktime_to_ns(hrtimer_expires_remaining(refresh_timer));
	if (remaining < min_expire)
		return 1;

	return 0;
}

static void start_cfs_slack_bandwidth(struct cfs_bandwidth *cfs_b)
{
	u64 min_left = cfs_bandwidth_slack_period + min_bandwidth_expiration;

	/* if there's a quota refresh soon don't bother with slack */
	if (runtime_refresh_within(cfs_b, min_left))
		return;

	hrtimer_start(&cfs_b->slack_timer,
			ns_to_ktime(cfs_bandwidth_slack_period),
			HRTIMER_MODE_REL);
}

/* we know any runtime found here is valid as update_curr() precedes return */
static void __return_cfs_rq_runtime(struct cfs_rq *cfs_rq)
{
	struct cfs_bandwidth *cfs_b = tg_cfs_bandwidth(cfs_rq->tg);
	s64 slack_runtime = cfs_rq->runtime_remaining - min_cfs_rq_runtime;

	if (slack_runtime <= 0)
		return;

	raw_spin_lock(&cfs_b->lock);
	if (cfs_b->quota != RUNTIME_INF &&
	    cfs_rq->runtime_expires == cfs_b->runtime_expires) {
		cfs_b->runtime += slack_runtime;

		/* we are under rq->lock, defer unthrottling using a timer */
		if (cfs_b->runtime > sched_cfs_bandwidth_slice() &&
		    !list_empty(&cfs_b->throttled_cfs_rq))
			start_cfs_slack_bandwidth(cfs_b);
	}
	raw_spin_unlock(&cfs_b->lock);

	/* even if it's not valid for return we don't want to try again */
	cfs_rq->runtime_remaining -= slack_runtime;
}

static __always_inline void return_cfs_rq_runtime(struct cfs_rq *cfs_rq)
{
	if (!cfs_bandwidth_used())
		return;

	if (!cfs_rq->runtime_enabled || cfs_rq->nr_running)
		return;

	__return_cfs_rq_runtime(cfs_rq);
}

/*
 * This is done with a timer (instead of inline with bandwidth return) since
 * it's necessary to juggle rq->locks to unthrottle their respective cfs_rqs.
 */
static void do_sched_cfs_slack_timer(struct cfs_bandwidth *cfs_b)
{
	u64 runtime = 0, slice = sched_cfs_bandwidth_slice();
	u64 expires;

	/* confirm we're still not at a refresh boundary */
	raw_spin_lock(&cfs_b->lock);
	if (runtime_refresh_within(cfs_b, min_bandwidth_expiration)) {
		raw_spin_unlock(&cfs_b->lock);
		return;
	}

	if (cfs_b->quota != RUNTIME_INF && cfs_b->runtime > slice)
		runtime = cfs_b->runtime;

	expires = cfs_b->runtime_expires;
	raw_spin_unlock(&cfs_b->lock);

	if (!runtime)
		return;

	runtime = distribute_cfs_runtime(cfs_b, runtime, expires);

	raw_spin_lock(&cfs_b->lock);
	if (expires == cfs_b->runtime_expires)
		cfs_b->runtime -= min(runtime, cfs_b->runtime);
	raw_spin_unlock(&cfs_b->lock);
}

/*
 * When a group wakes up we want to make sure that its quota is not already
 * expired/exceeded, otherwise it may be allowed to steal additional ticks of
 * runtime as update_curr() throttling can not not trigger until it's on-rq.
 */
static void check_enqueue_throttle(struct cfs_rq *cfs_rq)
{
	if (!cfs_bandwidth_used())
		return;

	/* an active group must be handled by the update_curr()->put() path */
	if (!cfs_rq->runtime_enabled || cfs_rq->curr)
		return;

	/* ensure the group is not already throttled */
	if (cfs_rq_throttled(cfs_rq))
		return;

	/* update runtime allocation */
	account_cfs_rq_runtime(cfs_rq, 0);
	if (cfs_rq->runtime_remaining <= 0)
		throttle_cfs_rq(cfs_rq);
}

static void sync_throttle(struct task_group *tg, int cpu)
{
	struct cfs_rq *pcfs_rq, *cfs_rq;

	if (!cfs_bandwidth_used())
		return;

	if (!tg->parent)
		return;

	cfs_rq = tg->cfs_rq[cpu];
	pcfs_rq = tg->parent->cfs_rq[cpu];

	cfs_rq->throttle_count = pcfs_rq->throttle_count;
	cfs_rq->throttled_clock_task = rq_clock_task(cpu_rq(cpu));
}

/* conditionally throttle active cfs_rq's from put_prev_entity() */
static bool check_cfs_rq_runtime(struct cfs_rq *cfs_rq)
{
	if (!cfs_bandwidth_used())
		return false;

	if (likely(!cfs_rq->runtime_enabled || cfs_rq->runtime_remaining > 0))
		return false;

	/*
	 * it's possible for a throttled entity to be forced into a running
	 * state (e.g. set_curr_task), in this case we're finished.
	 */
	if (cfs_rq_throttled(cfs_rq))
		return true;

	throttle_cfs_rq(cfs_rq);
	return true;
}

static enum hrtimer_restart sched_cfs_slack_timer(struct hrtimer *timer)
{
	struct cfs_bandwidth *cfs_b =
		container_of(timer, struct cfs_bandwidth, slack_timer);

	do_sched_cfs_slack_timer(cfs_b);

	return HRTIMER_NORESTART;
}

static enum hrtimer_restart sched_cfs_period_timer(struct hrtimer *timer)
{
	struct cfs_bandwidth *cfs_b =
		container_of(timer, struct cfs_bandwidth, period_timer);
	int overrun;
	int idle = 0;

	raw_spin_lock(&cfs_b->lock);
	for (;;) {
		overrun = hrtimer_forward_now(timer, cfs_b->period);
		if (!overrun)
			break;

		idle = do_sched_cfs_period_timer(cfs_b, overrun);
	}
	if (idle)
		cfs_b->period_active = 0;
	raw_spin_unlock(&cfs_b->lock);

	return idle ? HRTIMER_NORESTART : HRTIMER_RESTART;
}

void init_cfs_bandwidth(struct cfs_bandwidth *cfs_b)
{
	raw_spin_lock_init(&cfs_b->lock);
	cfs_b->runtime = 0;
	cfs_b->quota = RUNTIME_INF;
	cfs_b->period = ns_to_ktime(default_cfs_period());

	INIT_LIST_HEAD(&cfs_b->throttled_cfs_rq);
	hrtimer_init(&cfs_b->period_timer, CLOCK_MONOTONIC, HRTIMER_MODE_ABS_PINNED);
	cfs_b->period_timer.function = sched_cfs_period_timer;
	hrtimer_init(&cfs_b->slack_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	cfs_b->slack_timer.function = sched_cfs_slack_timer;
}

static void init_cfs_rq_runtime(struct cfs_rq *cfs_rq)
{
	cfs_rq->runtime_enabled = 0;
	INIT_LIST_HEAD(&cfs_rq->throttled_list);
}

void start_cfs_bandwidth(struct cfs_bandwidth *cfs_b)
{
	lockdep_assert_held(&cfs_b->lock);

	if (!cfs_b->period_active) {
		cfs_b->period_active = 1;
		hrtimer_forward_now(&cfs_b->period_timer, cfs_b->period);
		hrtimer_start_expires(&cfs_b->period_timer, HRTIMER_MODE_ABS_PINNED);
	}
}

static void destroy_cfs_bandwidth(struct cfs_bandwidth *cfs_b)
{
	/* init_cfs_bandwidth() was not called */
	if (!cfs_b->throttled_cfs_rq.next)
		return;

	hrtimer_cancel(&cfs_b->period_timer);
	hrtimer_cancel(&cfs_b->slack_timer);
}

static void __maybe_unused update_runtime_enabled(struct rq *rq)
{
	struct cfs_rq *cfs_rq;

	for_each_leaf_cfs_rq(rq, cfs_rq) {
		struct cfs_bandwidth *cfs_b = &cfs_rq->tg->cfs_bandwidth;

		raw_spin_lock(&cfs_b->lock);
		cfs_rq->runtime_enabled = cfs_b->quota != RUNTIME_INF;
		raw_spin_unlock(&cfs_b->lock);
	}
}

static void __maybe_unused unthrottle_offline_cfs_rqs(struct rq *rq)
{
	struct cfs_rq *cfs_rq;

	for_each_leaf_cfs_rq(rq, cfs_rq) {
		if (!cfs_rq->runtime_enabled)
			continue;

		/*
		 * clock_task is not advancing so we just need to make sure
		 * there's some valid quota amount
		 */
		cfs_rq->runtime_remaining = 1;
		/*
		 * Offline rq is schedulable till cpu is completely disabled
		 * in take_cpu_down(), so we prevent new cfs throttling here.
		 */
		cfs_rq->runtime_enabled = 0;

		if (cfs_rq_throttled(cfs_rq))
			unthrottle_cfs_rq(cfs_rq);
	}
}

#else /* CONFIG_CFS_BANDWIDTH */
static inline u64 cfs_rq_clock_task(struct cfs_rq *cfs_rq)
{
	return rq_clock_task(rq_of(cfs_rq));
}

static void account_cfs_rq_runtime(struct cfs_rq *cfs_rq,
					u64 delta_exec) {}
static bool check_cfs_rq_runtime(struct cfs_rq *cfs_rq) { return false; }
static void check_enqueue_throttle(struct cfs_rq *cfs_rq) {}
static inline void sync_throttle(struct task_group *tg, int cpu) {}
static __always_inline void return_cfs_rq_runtime(struct cfs_rq *cfs_rq) {}

static inline int cfs_rq_throttled(struct cfs_rq *cfs_rq)
{
	return 0;
}

static inline int throttled_hierarchy(struct cfs_rq *cfs_rq)
{
	return 0;
}

static inline int throttled_lb_pair(struct task_group *tg,
				    int src_cpu, int dest_cpu)
{
	return 0;
}

void init_cfs_bandwidth(struct cfs_bandwidth *cfs_b) {}

#ifdef CONFIG_FAIR_GROUP_SCHED
static void init_cfs_rq_runtime(struct cfs_rq *cfs_rq) {}
#endif

static inline struct cfs_bandwidth *tg_cfs_bandwidth(struct task_group *tg)
{
	return NULL;
}
static inline void destroy_cfs_bandwidth(struct cfs_bandwidth *cfs_b) {}
static inline void update_runtime_enabled(struct rq *rq) {}
static inline void unthrottle_offline_cfs_rqs(struct rq *rq) {}

#endif /* CONFIG_CFS_BANDWIDTH */

/**************************************************
 * CFS operations on tasks:
 */

#ifdef CONFIG_SCHED_HRTICK
static void hrtick_start_fair(struct rq *rq, struct task_struct *p)
{
	struct sched_entity *se = &p->se;
	struct cfs_rq *cfs_rq = cfs_rq_of(se);

	WARN_ON(task_rq(p) != rq);

	if (rq->cfs.h_nr_running > 1) {
		u64 slice = sched_slice(cfs_rq, se);
		u64 ran = se->sum_exec_runtime - se->prev_sum_exec_runtime;
		s64 delta = slice - ran;

		if (delta < 0) {
			if (rq->curr == p)
				resched_curr(rq);
			return;
		}
		hrtick_start(rq, delta);
	}
}

/*
 * called from enqueue/dequeue and updates the hrtick when the
 * current task is from our class.
 */
static void hrtick_update(struct rq *rq)
{
	struct task_struct *curr = rq->curr;

	if (!hrtick_enabled(rq) || curr->sched_class != &fair_sched_class)
		return;

	if (cfs_rq_of(&curr->se)->nr_running < sched_nr_latency)
		hrtick_start_fair(rq, curr);
}
#else /* !CONFIG_SCHED_HRTICK */
static inline void
hrtick_start_fair(struct rq *rq, struct task_struct *p)
{
}

static inline void hrtick_update(struct rq *rq)
{
}
#endif

#ifdef CONFIG_SMP
static bool cpu_overutilized(int cpu);
static inline unsigned long boosted_cpu_util(int cpu);
#else
#define boosted_cpu_util(cpu) cpu_util(cpu)
#endif

#ifdef CONFIG_SMP
static void update_capacity_of(int cpu)
{
	unsigned long req_cap;

	if (!sched_freq())
		return;

	/* Convert scale-invariant capacity to cpu. */
	req_cap = boosted_cpu_util(cpu);
	req_cap = req_cap * SCHED_CAPACITY_SCALE / capacity_orig_of(cpu);
	set_cfs_cpu_capacity(cpu, true, req_cap);
}
#endif

/*
 * The enqueue_task method is called before nr_running is
 * increased. Here we update the fair scheduling stats and
 * then put the task into the rbtree:
 */
static void
enqueue_task_fair(struct rq *rq, struct task_struct *p, int flags)
{
	struct cfs_rq *cfs_rq;
	struct sched_entity *se = &p->se;
#ifdef CONFIG_SMP
	int task_new = flags & ENQUEUE_WAKEUP_NEW;
	int task_wakeup = flags & ENQUEUE_WAKEUP;
#endif

	for_each_sched_entity(se) {
		if (se->on_rq)
			break;
		cfs_rq = cfs_rq_of(se);
		enqueue_entity(cfs_rq, se, flags);

		/*
		 * end evaluation on encountering a throttled cfs_rq
		 *
		 * note: in the case of encountering a throttled cfs_rq we will
		 * post the final h_nr_running increment below.
		 */
		if (cfs_rq_throttled(cfs_rq))
			break;
		cfs_rq->h_nr_running++;
		walt_inc_cfs_cumulative_runnable_avg(cfs_rq, p);

		flags = ENQUEUE_WAKEUP;
	}

	for_each_sched_entity(se) {
		cfs_rq = cfs_rq_of(se);
		cfs_rq->h_nr_running++;
		walt_inc_cfs_cumulative_runnable_avg(cfs_rq, p);

		if (cfs_rq_throttled(cfs_rq))
			break;

		update_load_avg(se, 1);
		update_cfs_shares(cfs_rq);
	}

	if (!se)
		add_nr_running(rq, 1);

#ifdef CONFIG_SMP

	/*
	 * Update SchedTune accounting.
	 *
	 * We do it before updating the CPU capacity to ensure the
	 * boost value of the current task is accounted for in the
	 * selection of the OPP.
	 *
	 * We do it also in the case where we enqueue a throttled task;
	 * we could argue that a throttled task should not boost a CPU,
	 * however:
	 * a) properly implementing CPU boosting considering throttled
	 *    tasks will increase a lot the complexity of the solution
	 * b) it's not easy to quantify the benefits introduced by
	 *    such a more complex solution.
	 * Thus, for the time being we go for the simple solution and boost
	 * also for throttled RQs.
	 */
	schedtune_enqueue_task(p, cpu_of(rq));

	if (!se) {
		walt_inc_cumulative_runnable_avg(rq, p);
		if (!task_new && !rq->rd->overutilized &&
		    cpu_overutilized(rq->cpu))
			rq->rd->overutilized = true;

		/*
		 * We want to potentially trigger a freq switch
		 * request only for tasks that are waking up; this is
		 * because we get here also during load balancing, but
		 * in these cases it seems wise to trigger as single
		 * request after load balancing is done.
		 */
		if (task_new || task_wakeup)
			update_capacity_of(cpu_of(rq));
	}

#endif /* CONFIG_SMP */
	hrtick_update(rq);
}

static void set_next_buddy(struct sched_entity *se);

/*
 * The dequeue_task method is called before nr_running is
 * decreased. We remove the task from the rbtree and
 * update the fair scheduling stats:
 */
static void dequeue_task_fair(struct rq *rq, struct task_struct *p, int flags)
{
	struct cfs_rq *cfs_rq;
	struct sched_entity *se = &p->se;
	int task_sleep = flags & DEQUEUE_SLEEP;

	for_each_sched_entity(se) {
		cfs_rq = cfs_rq_of(se);
		dequeue_entity(cfs_rq, se, flags);

		/*
		 * end evaluation on encountering a throttled cfs_rq
		 *
		 * note: in the case of encountering a throttled cfs_rq we will
		 * post the final h_nr_running decrement below.
		*/
		if (cfs_rq_throttled(cfs_rq))
			break;
		cfs_rq->h_nr_running--;
		walt_dec_cfs_cumulative_runnable_avg(cfs_rq, p);

		/* Don't dequeue parent if it has other entities besides us */
		if (cfs_rq->load.weight) {
			/* Avoid re-evaluating load for this entity: */
			se = parent_entity(se);
			/*
			 * Bias pick_next to pick a task from this cfs_rq, as
			 * p is sleeping when it is within its sched_slice.
			 */
			if (task_sleep && se && !throttled_hierarchy(cfs_rq))
				set_next_buddy(se);
			break;
		}
		flags |= DEQUEUE_SLEEP;
	}

	for_each_sched_entity(se) {
		cfs_rq = cfs_rq_of(se);
		cfs_rq->h_nr_running--;
		walt_dec_cfs_cumulative_runnable_avg(cfs_rq, p);

		if (cfs_rq_throttled(cfs_rq))
			break;

		update_load_avg(se, 1);
		update_cfs_shares(cfs_rq);
	}

	if (!se)
		sub_nr_running(rq, 1);

#ifdef CONFIG_SMP

	/*
	 * Update SchedTune accounting
	 *
	 * We do it before updating the CPU capacity to ensure the
	 * boost value of the current task is accounted for in the
	 * selection of the OPP.
	 */
	schedtune_dequeue_task(p, cpu_of(rq));

	if (!se) {
		walt_dec_cumulative_runnable_avg(rq, p);

		/*
		 * We want to potentially trigger a freq switch
		 * request only for tasks that are going to sleep;
		 * this is because we get here also during load
		 * balancing, but in these cases it seems wise to
		 * trigger as single request after load balancing is
		 * done.
		 */
		if (task_sleep) {
			if (rq->cfs.nr_running)
				update_capacity_of(cpu_of(rq));
			else if (sched_freq())
				set_cfs_cpu_capacity(cpu_of(rq), false, 0);
		}
	}

#endif /* CONFIG_SMP */

	hrtick_update(rq);
}

#ifdef CONFIG_SMP

/*
 * per rq 'load' arrray crap; XXX kill this.
 */

/*
 * The exact cpuload calculated at every tick would be:
 *
 *   load' = (1 - 1/2^i) * load + (1/2^i) * cur_load
 *
 * If a cpu misses updates for n ticks (as it was idle) and update gets
 * called on the n+1-th tick when cpu may be busy, then we have:
 *
 *   load_n   = (1 - 1/2^i)^n * load_0
 *   load_n+1 = (1 - 1/2^i)   * load_n + (1/2^i) * cur_load
 *
 * decay_load_missed() below does efficient calculation of
 *
 *   load' = (1 - 1/2^i)^n * load
 *
 * Because x^(n+m) := x^n * x^m we can decompose any x^n in power-of-2 factors.
 * This allows us to precompute the above in said factors, thereby allowing the
 * reduction of an arbitrary n in O(log_2 n) steps. (See also
 * fixed_power_int())
 *
 * The calculation is approximated on a 128 point scale.
 */
#define DEGRADE_SHIFT		7

static const u8 degrade_zero_ticks[CPU_LOAD_IDX_MAX] = {0, 8, 32, 64, 128};
static const u8 degrade_factor[CPU_LOAD_IDX_MAX][DEGRADE_SHIFT + 1] = {
	{   0,   0,  0,  0,  0,  0, 0, 0 },
	{  64,  32,  8,  0,  0,  0, 0, 0 },
	{  96,  72, 40, 12,  1,  0, 0, 0 },
	{ 112,  98, 75, 43, 15,  1, 0, 0 },
	{ 120, 112, 98, 76, 45, 16, 2, 0 }
};

/*
 * Update cpu_load for any missed ticks, due to tickless idle. The backlog
 * would be when CPU is idle and so we just decay the old load without
 * adding any new load.
 */
static unsigned long
decay_load_missed(unsigned long load, unsigned long missed_updates, int idx)
{
	int j = 0;

	if (!missed_updates)
		return load;

	if (missed_updates >= degrade_zero_ticks[idx])
		return 0;

	if (idx == 1)
		return load >> missed_updates;

	while (missed_updates) {
		if (missed_updates % 2)
			load = (load * degrade_factor[idx][j]) >> DEGRADE_SHIFT;

		missed_updates >>= 1;
		j++;
	}
	return load;
}

/**
 * __update_cpu_load - update the rq->cpu_load[] statistics
 * @this_rq: The rq to update statistics for
 * @this_load: The current load
 * @pending_updates: The number of missed updates
 * @active: !0 for NOHZ_FULL
 *
 * Update rq->cpu_load[] statistics. This function is usually called every
 * scheduler tick (TICK_NSEC).
 *
 * This function computes a decaying average:
 *
 *   load[i]' = (1 - 1/2^i) * load[i] + (1/2^i) * load
 *
 * Because of NOHZ it might not get called on every tick which gives need for
 * the @pending_updates argument.
 *
 *   load[i]_n = (1 - 1/2^i) * load[i]_n-1 + (1/2^i) * load_n-1
 *             = A * load[i]_n-1 + B ; A := (1 - 1/2^i), B := (1/2^i) * load
 *             = A * (A * load[i]_n-2 + B) + B
 *             = A * (A * (A * load[i]_n-3 + B) + B) + B
 *             = A^3 * load[i]_n-3 + (A^2 + A + 1) * B
 *             = A^n * load[i]_0 + (A^(n-1) + A^(n-2) + ... + 1) * B
 *             = A^n * load[i]_0 + ((1 - A^n) / (1 - A)) * B
 *             = (1 - 1/2^i)^n * (load[i]_0 - load) + load
 *
 * In the above we've assumed load_n := load, which is true for NOHZ_FULL as
 * any change in load would have resulted in the tick being turned back on.
 *
 * For regular NOHZ, this reduces to:
 *
 *   load[i]_n = (1 - 1/2^i)^n * load[i]_0
 *
 * see decay_load_misses(). For NOHZ_FULL we get to subtract and add the extra
 * term. See the @active paramter.
 */
static void __update_cpu_load(struct rq *this_rq, unsigned long this_load,
			      unsigned long pending_updates, int active)
{
	unsigned long tickless_load = active ? this_rq->cpu_load[0] : 0;
	int i, scale;

	this_rq->nr_load_updates++;

	/* Update our load: */
	this_rq->cpu_load[0] = this_load; /* Fasttrack for idx 0 */
	for (i = 1, scale = 2; i < CPU_LOAD_IDX_MAX; i++, scale += scale) {
		unsigned long old_load, new_load;

		/* scale is effectively 1 << i now, and >> i divides by scale */

		old_load = this_rq->cpu_load[i];
		old_load = decay_load_missed(old_load, pending_updates - 1, i);
		if (tickless_load) {
			old_load -= decay_load_missed(tickless_load, pending_updates - 1, i);
			/*
			 * old_load can never be a negative value because a
			 * decayed tickless_load cannot be greater than the
			 * original tickless_load.
			 */
			old_load += tickless_load;
		}
		new_load = this_load;
		/*
		 * Round up the averaging division if load is increasing. This
		 * prevents us from getting stuck on 9 if the load is 10, for
		 * example.
		 */
		if (new_load > old_load)
			new_load += scale - 1;

		this_rq->cpu_load[i] = (old_load * (scale - 1) + new_load) >> i;
	}

	sched_avg_update(this_rq);
}

/* Used instead of source_load when we know the type == 0 */
static unsigned long weighted_cpuload(const int cpu)
{
	return cfs_rq_runnable_load_avg(&cpu_rq(cpu)->cfs);
}

#ifdef CONFIG_NO_HZ_COMMON
static void __update_cpu_load_nohz(struct rq *this_rq,
				   unsigned long curr_jiffies,
				   unsigned long load,
				   int active)
{
	unsigned long pending_updates;

	pending_updates = curr_jiffies - this_rq->last_load_update_tick;
	if (pending_updates) {
		this_rq->last_load_update_tick = curr_jiffies;
		/*
		 * In the regular NOHZ case, we were idle, this means load 0.
		 * In the NOHZ_FULL case, we were non-idle, we should consider
		 * its weighted load.
		 */
		__update_cpu_load(this_rq, load, pending_updates, active);
	}
}

/*
 * There is no sane way to deal with nohz on smp when using jiffies because the
 * cpu doing the jiffies update might drift wrt the cpu doing the jiffy reading
 * causing off-by-one errors in observed deltas; {0,2} instead of {1,1}.
 *
 * Therefore we cannot use the delta approach from the regular tick since that
 * would seriously skew the load calculation. However we'll make do for those
 * updates happening while idle (nohz_idle_balance) or coming out of idle
 * (tick_nohz_idle_exit).
 *
 * This means we might still be one tick off for nohz periods.
 */

/*
 * Called from nohz_idle_balance() to update the load ratings before doing the
 * idle balance.
 */
static void update_cpu_load_idle(struct rq *this_rq)
{
	/*
	 * bail if there's load or we're actually up-to-date.
	 */
	if (weighted_cpuload(cpu_of(this_rq)))
		return;

	__update_cpu_load_nohz(this_rq, READ_ONCE(jiffies), 0, 0);
}

/*
 * Called from tick_nohz_idle_exit() -- try and fix up the ticks we missed.
 */
void update_cpu_load_nohz(int active)
{
	struct rq *this_rq = this_rq();
	unsigned long curr_jiffies = READ_ONCE(jiffies);
	unsigned long load = active ? weighted_cpuload(cpu_of(this_rq)) : 0;

	if (curr_jiffies == this_rq->last_load_update_tick)
		return;

	raw_spin_lock(&this_rq->lock);
	__update_cpu_load_nohz(this_rq, curr_jiffies, load, active);
	raw_spin_unlock(&this_rq->lock);
}
#endif /* CONFIG_NO_HZ_COMMON */

/*
 * Called from scheduler_tick()
 */
void update_cpu_load_active(struct rq *this_rq)
{
	unsigned long load = weighted_cpuload(cpu_of(this_rq));
	/*
	 * See the mess around update_cpu_load_idle() / update_cpu_load_nohz().
	 */
	this_rq->last_load_update_tick = jiffies;
	__update_cpu_load(this_rq, load, 1, 1);
}

/*
 * Return a low guess at the load of a migration-source cpu weighted
 * according to the scheduling class and "nice" value.
 *
 * We want to under-estimate the load of migration sources, to
 * balance conservatively.
 */
static unsigned long source_load(int cpu, int type)
{
	struct rq *rq = cpu_rq(cpu);
	unsigned long total = weighted_cpuload(cpu);

	if (type == 0 || !sched_feat(LB_BIAS))
		return total;

	return min(rq->cpu_load[type-1], total);
}

static inline bool energy_aware(void)
{
	return sched_feat(ENERGY_AWARE);
}

static inline unsigned long task_util(struct task_struct *p)
{
#ifdef CONFIG_SCHED_WALT
	if (!walt_disabled && sysctl_sched_use_walt_task_util) {
		unsigned long demand = p->ravg.demand;
		return (demand << 10) / walt_ravg_window;
	}
#endif
	return p->se.avg.util_avg;
}

unsigned int capacity_margin = 1280; /* ~20% margin */

static inline unsigned long boosted_task_util(struct task_struct *task);

static inline bool __task_fits(struct task_struct *p, int cpu, int util)
{
	unsigned long capacity = capacity_of(cpu);

	util += boosted_task_util(p);

	return (capacity * 1024) > (util * capacity_margin);
}

static inline bool task_fits_max(struct task_struct *p, int cpu)
{
	unsigned long capacity = capacity_of(cpu);
	unsigned long max_capacity = cpu_rq(cpu)->rd->max_cpu_capacity.val;

	if (capacity == max_capacity)
		return true;

	if (capacity * capacity_margin > max_capacity * 1024)
		return true;

	return __task_fits(p, cpu, 0);
}

static inline bool task_fits_spare(struct task_struct *p, int cpu)
{
	return __task_fits(p, cpu, cpu_util(cpu));
}

static bool cpu_overutilized(int cpu)
{
	return (capacity_of(cpu) * 1024) < (cpu_util(cpu) * capacity_margin);
}

/*
 * Return a high guess at the load of a migration-target cpu weighted
 * according to the scheduling class and "nice" value.
 */
static unsigned long target_load(int cpu, int type)
{
	struct rq *rq = cpu_rq(cpu);
	unsigned long total = weighted_cpuload(cpu);

	if (type == 0 || !sched_feat(LB_BIAS))
		return total;

	return max(rq->cpu_load[type-1], total);
}

static unsigned long cpu_avg_load_per_task(int cpu)
{
	struct rq *rq = cpu_rq(cpu);
	unsigned long nr_running = READ_ONCE(rq->cfs.h_nr_running);
	unsigned long load_avg = weighted_cpuload(cpu);

	if (nr_running)
		return load_avg / nr_running;

	return 0;
}

#ifdef CONFIG_FAIR_GROUP_SCHED
/*
 * effective_load() calculates the load change as seen from the root_task_group
 *
 * Adding load to a group doesn't make a group heavier, but can cause movement
 * of group shares between cpus. Assuming the shares were perfectly aligned one
 * can calculate the shift in shares.
 *
 * Calculate the effective load difference if @wl is added (subtracted) to @tg
 * on this @cpu and results in a total addition (subtraction) of @wg to the
 * total group weight.
 *
 * Given a runqueue weight distribution (rw_i) we can compute a shares
 * distribution (s_i) using:
 *
 *   s_i = rw_i / \Sum rw_j						(1)
 *
 * Suppose we have 4 CPUs and our @tg is a direct child of the root group and
 * has 7 equal weight tasks, distributed as below (rw_i), with the resulting
 * shares distribution (s_i):
 *
 *   rw_i = {   2,   4,   1,   0 }
 *   s_i  = { 2/7, 4/7, 1/7,   0 }
 *
 * As per wake_affine() we're interested in the load of two CPUs (the CPU the
 * task used to run on and the CPU the waker is running on), we need to
 * compute the effect of waking a task on either CPU and, in case of a sync
 * wakeup, compute the effect of the current task going to sleep.
 *
 * So for a change of @wl to the local @cpu with an overall group weight change
 * of @wl we can compute the new shares distribution (s'_i) using:
 *
 *   s'_i = (rw_i + @wl) / (@wg + \Sum rw_j)				(2)
 *
 * Suppose we're interested in CPUs 0 and 1, and want to compute the load
 * differences in waking a task to CPU 0. The additional task changes the
 * weight and shares distributions like:
 *
 *   rw'_i = {   3,   4,   1,   0 }
 *   s'_i  = { 3/8, 4/8, 1/8,   0 }
 *
 * We can then compute the difference in effective weight by using:
 *
 *   dw_i = S * (s'_i - s_i)						(3)
 *
 * Where 'S' is the group weight as seen by its parent.
 *
 * Therefore the effective change in loads on CPU 0 would be 5/56 (3/8 - 2/7)
 * times the weight of the group. The effect on CPU 1 would be -4/56 (4/8 -
 * 4/7) times the weight of the group.
 */
static long effective_load(struct task_group *tg, int cpu, long wl, long wg)
{
	struct sched_entity *se = tg->se[cpu];

	if (!tg->parent)	/* the trivial, non-cgroup case */
		return wl;

	for_each_sched_entity(se) {
		struct cfs_rq *cfs_rq = se->my_q;
		long W, w = cfs_rq_load_avg(cfs_rq);

		tg = cfs_rq->tg;

		/*
		 * W = @wg + \Sum rw_j
		 */
		W = wg + atomic_long_read(&tg->load_avg);

		/* Ensure \Sum rw_j >= rw_i */
		W -= cfs_rq->tg_load_avg_contrib;
		W += w;

		/*
		 * w = rw_i + @wl
		 */
		w += wl;

		/*
		 * wl = S * s'_i; see (2)
		 */
		if (W > 0 && w < W)
			wl = (w * (long)scale_load_down(tg->shares)) / W;
		else
			wl = scale_load_down(tg->shares);

		/*
		 * Per the above, wl is the new se->load.weight value; since
		 * those are clipped to [MIN_SHARES, ...) do so now. See
		 * calc_cfs_shares().
		 */
		if (wl < MIN_SHARES)
			wl = MIN_SHARES;

		/*
		 * wl = dw_i = S * (s'_i - s_i); see (3)
		 */
		wl -= se->avg.load_avg;

		/*
		 * Recursively apply this logic to all parent groups to compute
		 * the final effective load change on the root group. Since
		 * only the @tg group gets extra weight, all parent groups can
		 * only redistribute existing shares. @wl is the shift in shares
		 * resulting from this level per the above.
		 */
		wg = 0;
	}

	return wl;
}
#else

static inline unsigned long effective_load(struct task_group *tg, int cpu,
		unsigned long wl, unsigned long wg)
{
	return wl;
}

#endif

struct energy_env {
	struct sched_group	*sg_top;
	struct sched_group	*sg_cap;
	int			cap_idx;
	int			util_delta;
	int			src_cpu;
	int			dst_cpu;
	int			energy;
	int			payoff;
	struct task_struct	*task;
	struct {
		int before;
		int after;
		int delta;
		int diff;
	} nrg;
	struct {
		int before;
		int after;
		int delta;
	} cap;
};

/*
 * __cpu_norm_util() returns the cpu util relative to a specific capacity,
 * i.e. it's busy ratio, in the range [0..SCHED_CAPACITY_SCALE] which is useful for
 * energy calculations. Using the scale-invariant util returned by
 * cpu_util() and approximating scale-invariant util by:
 *
 *   util ~ (curr_freq/max_freq)*1024 * capacity_orig/1024 * running_time/time
 *
 * the normalized util can be found using the specific capacity.
 *
 *   capacity = capacity_orig * curr_freq/max_freq
 *
 *   norm_util = running_time/time ~ util/capacity
 */
static unsigned long __cpu_norm_util(int cpu, unsigned long capacity, int delta)
{
	int util = __cpu_util(cpu, delta);

	if (util >= capacity)
		return SCHED_CAPACITY_SCALE;

	return (util << SCHED_CAPACITY_SHIFT)/capacity;
}

static int calc_util_delta(struct energy_env *eenv, int cpu)
{
	if (cpu == eenv->src_cpu)
		return -eenv->util_delta;
	if (cpu == eenv->dst_cpu)
		return eenv->util_delta;
	return 0;
}

static
unsigned long group_max_util(struct energy_env *eenv)
{
	int i, delta;
	unsigned long max_util = 0;

	for_each_cpu(i, sched_group_cpus(eenv->sg_cap)) {
		delta = calc_util_delta(eenv, i);
		max_util = max(max_util, __cpu_util(i, delta));
	}

	return max_util;
}

/*
 * group_norm_util() returns the approximated group util relative to it's
 * current capacity (busy ratio) in the range [0..SCHED_CAPACITY_SCALE] for use in
 * energy calculations. Since task executions may or may not overlap in time in
 * the group the true normalized util is between max(cpu_norm_util(i)) and
 * sum(cpu_norm_util(i)) when iterating over all cpus in the group, i. The
 * latter is used as the estimate as it leads to a more pessimistic energy
 * estimate (more busy).
 */
static unsigned
long group_norm_util(struct energy_env *eenv, struct sched_group *sg)
{
	int i, delta;
	unsigned long util_sum = 0;
	unsigned long capacity = sg->sge->cap_states[eenv->cap_idx].cap;

	for_each_cpu(i, sched_group_cpus(sg)) {
		delta = calc_util_delta(eenv, i);
		util_sum += __cpu_norm_util(i, capacity, delta);
	}

	if (util_sum > SCHED_CAPACITY_SCALE)
		return SCHED_CAPACITY_SCALE;
	return util_sum;
}

static int find_new_capacity(struct energy_env *eenv,
	const struct sched_group_energy const *sge)
{
	int idx;
	unsigned long util = group_max_util(eenv);

	for (idx = 0; idx < sge->nr_cap_states; idx++) {
		if (sge->cap_states[idx].cap >= util)
			break;
	}

	eenv->cap_idx = idx;

	return idx;
}

static int group_idle_state(struct sched_group *sg)
{
	int i, state = INT_MAX;

	/* Find the shallowest idle state in the sched group. */
	for_each_cpu(i, sched_group_cpus(sg))
		state = min(state, idle_get_state_idx(cpu_rq(i)));

	/* Take non-cpuidle idling into account (active idle/arch_cpu_idle()) */
	state++;

	return state;
}

/*
 * sched_group_energy(): Computes the absolute energy consumption of cpus
 * belonging to the sched_group including shared resources shared only by
 * members of the group. Iterates over all cpus in the hierarchy below the
 * sched_group starting from the bottom working it's way up before going to
 * the next cpu until all cpus are covered at all levels. The current
 * implementation is likely to gather the same util statistics multiple times.
 * This can probably be done in a faster but more complex way.
 * Note: sched_group_energy() may fail when racing with sched_domain updates.
 */
static int sched_group_energy(struct energy_env *eenv)
{
	struct sched_domain *sd;
	int cpu, total_energy = 0;
	struct cpumask visit_cpus;
	struct sched_group *sg;

	WARN_ON(!eenv->sg_top->sge);

	cpumask_copy(&visit_cpus, sched_group_cpus(eenv->sg_top));

	while (!cpumask_empty(&visit_cpus)) {
		struct sched_group *sg_shared_cap = NULL;

		cpu = cpumask_first(&visit_cpus);

		/*
		 * Is the group utilization affected by cpus outside this
		 * sched_group?
		 */
		sd = rcu_dereference(per_cpu(sd_scs, cpu));

		if (!sd)
			/*
			 * We most probably raced with hotplug; returning a
			 * wrong energy estimation is better than entering an
			 * infinite loop.
			 */
			return -EINVAL;

		if (sd->parent)
			sg_shared_cap = sd->parent->groups;

		for_each_domain(cpu, sd) {
			sg = sd->groups;

			/* Has this sched_domain already been visited? */
			if (sd->child && group_first_cpu(sg) != cpu)
				break;

			do {
				unsigned long group_util;
				int sg_busy_energy, sg_idle_energy;
				int cap_idx, idle_idx;

				if (sg_shared_cap && sg_shared_cap->group_weight >= sg->group_weight)
					eenv->sg_cap = sg_shared_cap;
				else
					eenv->sg_cap = sg;

				cap_idx = find_new_capacity(eenv, sg->sge);

				if (sg->group_weight == 1) {
					/* Remove capacity of src CPU (before task move) */
					if (eenv->util_delta == 0 &&
					    cpumask_test_cpu(eenv->src_cpu, sched_group_cpus(sg))) {
						eenv->cap.before = sg->sge->cap_states[cap_idx].cap;
						eenv->cap.delta -= eenv->cap.before;
					}
					/* Add capacity of dst CPU  (after task move) */
					if (eenv->util_delta != 0 &&
					    cpumask_test_cpu(eenv->dst_cpu, sched_group_cpus(sg))) {
						eenv->cap.after = sg->sge->cap_states[cap_idx].cap;
						eenv->cap.delta += eenv->cap.after;
					}
				}

				idle_idx = group_idle_state(sg);
				group_util = group_norm_util(eenv, sg);
				sg_busy_energy = (group_util * sg->sge->cap_states[cap_idx].power)
								>> SCHED_CAPACITY_SHIFT;
				sg_idle_energy = ((SCHED_CAPACITY_SCALE-group_util)
								* sg->sge->idle_states[idle_idx].power)
								>> SCHED_CAPACITY_SHIFT;

				total_energy += sg_busy_energy + sg_idle_energy;

				if (!sd->child)
					cpumask_xor(&visit_cpus, &visit_cpus, sched_group_cpus(sg));

				if (cpumask_equal(sched_group_cpus(sg), sched_group_cpus(eenv->sg_top)))
					goto next_cpu;

			} while (sg = sg->next, sg != sd->groups);
		}
next_cpu:
		cpumask_clear_cpu(cpu, &visit_cpus);
		continue;
	}

	eenv->energy = total_energy;
	return 0;
}

static inline bool cpu_in_sg(struct sched_group *sg, int cpu)
{
	return cpu != -1 && cpumask_test_cpu(cpu, sched_group_cpus(sg));
}

/*
 * energy_diff(): Estimate the energy impact of changing the utilization
 * distribution. eenv specifies the change: utilisation amount, source, and
 * destination cpu. Source or destination cpu may be -1 in which case the
 * utilization is removed from or added to the system (e.g. task wake-up). If
 * both are specified, the utilization is migrated.
 */
static inline int __energy_diff(struct energy_env *eenv)
{
	struct sched_domain *sd;
	struct sched_group *sg;
	int sd_cpu = -1, energy_before = 0, energy_after = 0;

	struct energy_env eenv_before = {
		.util_delta	= 0,
		.src_cpu	= eenv->src_cpu,
		.dst_cpu	= eenv->dst_cpu,
		.nrg		= { 0, 0, 0, 0},
		.cap		= { 0, 0, 0 },
	};

	if (eenv->src_cpu == eenv->dst_cpu)
		return 0;

	sd_cpu = (eenv->src_cpu != -1) ? eenv->src_cpu : eenv->dst_cpu;
	sd = rcu_dereference(per_cpu(sd_ea, sd_cpu));

	if (!sd)
		return 0; /* Error */

	sg = sd->groups;

	do {
		if (cpu_in_sg(sg, eenv->src_cpu) || cpu_in_sg(sg, eenv->dst_cpu)) {
			eenv_before.sg_top = eenv->sg_top = sg;

			if (sched_group_energy(&eenv_before))
				return 0; /* Invalid result abort */
			energy_before += eenv_before.energy;

			/* Keep track of SRC cpu (before) capacity */
			eenv->cap.before = eenv_before.cap.before;
			eenv->cap.delta = eenv_before.cap.delta;

			if (sched_group_energy(eenv))
				return 0; /* Invalid result abort */
			energy_after += eenv->energy;
		}
	} while (sg = sg->next, sg != sd->groups);

	eenv->nrg.before = energy_before;
	eenv->nrg.after = energy_after;
	eenv->nrg.diff = eenv->nrg.after - eenv->nrg.before;
	eenv->payoff = 0;

	return eenv->nrg.diff;
}

#ifdef CONFIG_SCHED_TUNE

struct target_nrg schedtune_target_nrg;

/*
 * System energy normalization
 * Returns the normalized value, in the range [0..SCHED_CAPACITY_SCALE],
 * corresponding to the specified energy variation.
 */
static inline int
normalize_energy(int energy_diff)
{
	u32 normalized_nrg;
	int max_delta;

#ifdef CONFIG_SCHED_DEBUG
	/* Check for boundaries */
	max_delta  = schedtune_target_nrg.max_power;
	max_delta -= schedtune_target_nrg.min_power;
	WARN_ON(abs(energy_diff) >= max_delta);
#endif

	/* Do scaling using positive numbers to increase the range */
	normalized_nrg = (energy_diff < 0) ? -energy_diff : energy_diff;

	/* Scale by energy magnitude */
	normalized_nrg <<= NICE_0_LOAD_SHIFT;

	/* Normalize on max energy for target platform */
	normalized_nrg = reciprocal_divide(
			normalized_nrg, schedtune_target_nrg.rdiv);

	return (energy_diff < 0) ? -normalized_nrg : normalized_nrg;
}

static inline int
energy_diff(struct energy_env *eenv)
{
	int boost = schedtune_task_boost(eenv->task);
	int nrg_delta;

	/* Conpute "absolute" energy diff */
	__energy_diff(eenv);

	/* Return energy diff when boost margin is 0 */
	if (boost == 0)
		return eenv->nrg.diff;

	/* Compute normalized energy diff */
	nrg_delta = normalize_energy(eenv->nrg.diff);
	eenv->nrg.delta = nrg_delta;

	eenv->payoff = schedtune_accept_deltas(
			eenv->nrg.delta,
			eenv->cap.delta,
			eenv->task);

	/*
	 * When SchedTune is enabled, the energy_diff() function will return
	 * the computed energy payoff value. Since the energy_diff() return
	 * value is expected to be negative by its callers, this evaluation
	 * function return a negative value each time the evaluation return a
	 * positive payoff, which is the condition for the acceptance of
	 * a scheduling decision
	 */
	return -eenv->payoff;
}
#else /* CONFIG_SCHED_TUNE */
#define energy_diff(eenv) __energy_diff(eenv)
#endif

static void record_wakee(struct task_struct *p)
{
	/*
	 * Only decay a single time; tasks that have less then 1 wakeup per
	 * jiffy will not have built up many flips.
	 */
	if (time_after(jiffies, current->wakee_flip_decay_ts + HZ)) {
		current->wakee_flips >>= 1;
		current->wakee_flip_decay_ts = jiffies;
	}

	if (current->last_wakee != p) {
		current->last_wakee = p;
		current->wakee_flips++;
	}
}

/*
 * Detect M:N waker/wakee relationships via a switching-frequency heuristic.
 *
 * A waker of many should wake a different task than the one last awakened
 * at a frequency roughly N times higher than one of its wakees.
 *
 * In order to determine whether we should let the load spread vs consolidating
 * to shared cache, we look for a minimum 'flip' frequency of llc_size in one
 * partner, and a factor of lls_size higher frequency in the other.
 *
 * With both conditions met, we can be relatively sure that the relationship is
 * non-monogamous, with partner count exceeding socket size.
 *
 * Waker/wakee being client/server, worker/dispatcher, interrupt source or
 * whatever is irrelevant, spread criteria is apparent partner count exceeds
 * socket size.
 */
static int wake_wide(struct task_struct *p)
{
	unsigned int master = current->wakee_flips;
	unsigned int slave = p->wakee_flips;
	int factor = this_cpu_read(sd_llc_size);

	if (master < slave)
		swap(master, slave);
	if (slave < factor || master < slave * factor)
		return 0;
	return 1;
}

static int wake_affine(struct sched_domain *sd, struct task_struct *p,
		       int prev_cpu, int sync)
{
	s64 this_load, load;
	s64 this_eff_load, prev_eff_load;
	int idx, this_cpu;
	struct task_group *tg;
	unsigned long weight;
	int balanced;

	idx	  = sd->wake_idx;
	this_cpu  = smp_processor_id();
	load	  = source_load(prev_cpu, idx);
	this_load = target_load(this_cpu, idx);

	/*
	 * If sync wakeup then subtract the (maximum possible)
	 * effect of the currently running task from the load
	 * of the current CPU:
	 */
	if (sync) {
		tg = task_group(current);
		weight = current->se.avg.load_avg;

		this_load += effective_load(tg, this_cpu, -weight, -weight);
		load += effective_load(tg, prev_cpu, 0, -weight);
	}

	tg = task_group(p);
	weight = p->se.avg.load_avg;

	/*
	 * In low-load situations, where prev_cpu is idle and this_cpu is idle
	 * due to the sync cause above having dropped this_load to 0, we'll
	 * always have an imbalance, but there's really nothing you can do
	 * about that, so that's good too.
	 *
	 * Otherwise check if either cpus are near enough in load to allow this
	 * task to be woken on this_cpu.
	 */
	this_eff_load = 100;
	this_eff_load *= capacity_of(prev_cpu);

	prev_eff_load = 100 + (sd->imbalance_pct - 100) / 2;
	prev_eff_load *= capacity_of(this_cpu);

	if (this_load > 0) {
		this_eff_load *= this_load +
			effective_load(tg, this_cpu, weight, weight);

		prev_eff_load *= load + effective_load(tg, prev_cpu, 0, weight);
	}

	balanced = this_eff_load <= prev_eff_load;

	schedstat_inc(p, se.statistics.nr_wakeups_affine_attempts);

	if (!balanced)
		return 0;

	schedstat_inc(sd, ttwu_move_affine);
	schedstat_inc(p, se.statistics.nr_wakeups_affine);

	return 1;
}

static DEFINE_PER_CPU(bool, dbs_boost_needed);

#ifdef CONFIG_SCHED_TUNE

static long
schedtune_margin(unsigned long signal, long boost)
{
	long long margin = 0;

	/*
	 * Signal proportional compensation (SPC)
	 *
	 * The Boost (B) value is used to compute a Margin (M) which is
	 * proportional to the complement of the original Signal (S):
	 *   M = B * (SCHED_CAPACITY_SCALE - S), if B is positive
	 *   M = B * S, if B is negative
	 * The obtained M could be used by the caller to "boost" S.
	 */
	if (boost >= 0) {
		margin  = SCHED_CAPACITY_SCALE - signal;
		margin *= boost;
	} else
		margin = -signal * boost;
	/*
	 * Fast integer division by constant:
	 *  Constant   :                 (C) = 100
	 *  Precision  : 0.1%            (P) = 0.1
	 *  Reference  : C * 100 / P     (R) = 100000
	 *
	 * Thus:
	 *  Shift bits : ceil(log(R,2))  (S) = 17
	 *  Mult const : round(2^S/C)    (M) = 1311
	 *
	 *
	 */
	margin  *= 1311;
	margin >>= 17;

	if (boost < 0)
		margin *= -1;
	return margin;
}

static inline int
schedtune_cpu_margin(unsigned long util, int cpu)
{
	int boost = schedtune_cpu_boost(cpu);

	if (boost == 0)
		return 0;

	return schedtune_margin(util, boost);
}

static inline long
schedtune_task_margin(struct task_struct *task)
{
	int boost = schedtune_task_boost(task);
	unsigned long util;
	long margin;

	if (boost == 0)
		return 0;

	util = task_util(task);
	margin = schedtune_margin(util, boost);

	return margin;
}

#else /* CONFIG_SCHED_TUNE */

static inline int
schedtune_cpu_margin(unsigned long util, int cpu)
{
	return 0;
}

static inline int
schedtune_task_margin(struct task_struct *task)
{
	return 0;
}

#endif /* CONFIG_SCHED_TUNE */

static inline unsigned long
boosted_cpu_util(int cpu)
{
	unsigned long util = cpu_util(cpu);
	long margin = schedtune_cpu_margin(util, cpu);

	return util + margin;
}

static inline unsigned long
boosted_task_util(struct task_struct *task)
{
	unsigned long util = task_util(task);
	long margin = schedtune_task_margin(task);

	return util + margin;
}

/*
 * find_idlest_group finds and returns the least busy CPU group within the
 * domain.
 */
static struct sched_group *
find_idlest_group(struct sched_domain *sd, struct task_struct *p,
		  int this_cpu, int sd_flag)
{
	struct sched_group *idlest = NULL, *group = sd->groups;
	struct sched_group *fit_group = NULL, *spare_group = NULL;
	unsigned long min_load = ULONG_MAX, this_load = 0;
	unsigned long fit_capacity = ULONG_MAX;
	unsigned long max_spare_capacity = capacity_margin - SCHED_CAPACITY_SCALE;
	int load_idx = sd->forkexec_idx;
	int imbalance = 100 + (sd->imbalance_pct-100)/2;

	if (sd_flag & SD_BALANCE_WAKE)
		load_idx = sd->wake_idx;

	do {
		unsigned long load, avg_load, spare_capacity;
		int local_group;
		int i;

		/* Skip over this group if it has no CPUs allowed */
		if (!cpumask_intersects(sched_group_cpus(group),
					tsk_cpus_allowed(p)))
			continue;

		local_group = cpumask_test_cpu(this_cpu,
					       sched_group_cpus(group));

		/* Tally up the load of all CPUs in the group */
		avg_load = 0;

		for_each_cpu(i, sched_group_cpus(group)) {
			/* Bias balancing toward cpus of our domain */
			if (local_group)
				load = source_load(i, load_idx);
			else
				load = target_load(i, load_idx);

			avg_load += load;

			/*
			 * Look for most energy-efficient group that can fit
			 * that can fit the task.
			 */
			if (capacity_of(i) < fit_capacity && task_fits_spare(p, i)) {
				fit_capacity = capacity_of(i);
				fit_group = group;
			}

			/*
			 * Look for group which has most spare capacity on a
			 * single cpu.
			 */
			spare_capacity = capacity_of(i) - cpu_util(i);
			if (spare_capacity > max_spare_capacity) {
				max_spare_capacity = spare_capacity;
				spare_group = group;
			}
		}

		/* Adjust by relative CPU capacity of the group */
		avg_load = (avg_load * SCHED_CAPACITY_SCALE) / group->sgc->capacity;

		if (local_group) {
			this_load = avg_load;
		} else if (avg_load < min_load) {
			min_load = avg_load;
			idlest = group;
		}
	} while (group = group->next, group != sd->groups);

	if (fit_group)
		return fit_group;

	if (spare_group)
		return spare_group;

	if (!idlest || 100*this_load < imbalance*min_load)
		return NULL;
	return idlest;
}

/*
 * find_idlest_cpu - find the idlest cpu among the cpus in group.
 */
static int
find_idlest_cpu(struct sched_group *group, struct task_struct *p, int this_cpu)
{
	unsigned long load, min_load = ULONG_MAX;
	unsigned int min_exit_latency = UINT_MAX;
	u64 latest_idle_timestamp = 0;
	int least_loaded_cpu = this_cpu;
	int shallowest_idle_cpu = -1;
	int i;

	/* Check if we have any choice: */
	if (group->group_weight == 1)
		return cpumask_first(sched_group_cpus(group));

	/* Traverse only the allowed CPUs */
	for_each_cpu_and(i, sched_group_cpus(group), tsk_cpus_allowed(p)) {
		if (task_fits_spare(p, i)) {
			struct rq *rq = cpu_rq(i);
			struct cpuidle_state *idle = idle_get_state(rq);
			if (idle && idle->exit_latency < min_exit_latency) {
				/*
				 * We give priority to a CPU whose idle state
				 * has the smallest exit latency irrespective
				 * of any idle timestamp.
				 */
				min_exit_latency = idle->exit_latency;
				latest_idle_timestamp = rq->idle_stamp;
				shallowest_idle_cpu = i;
			} else if (idle_cpu(i) &&
				   (!idle || idle->exit_latency == min_exit_latency) &&
				   rq->idle_stamp > latest_idle_timestamp) {
				/*
				 * If equal or no active idle state, then
				 * the most recently idled CPU might have
				 * a warmer cache.
				 */
				latest_idle_timestamp = rq->idle_stamp;
				shallowest_idle_cpu = i;
			} else if (shallowest_idle_cpu == -1) {
				/*
				 * If we haven't found an idle CPU yet
				 * pick a non-idle one that can fit the task as
				 * fallback.
				 */
				shallowest_idle_cpu = i;
			}
		} else if (shallowest_idle_cpu == -1) {
			load = weighted_cpuload(i);
			if (load < min_load || (load == min_load && i == this_cpu)) {
				min_load = load;
				least_loaded_cpu = i;
			}
		}
	}

	return shallowest_idle_cpu != -1 ? shallowest_idle_cpu : least_loaded_cpu;
}

/*
 * Try and locate an idle CPU in the sched_domain.
 */
static int select_idle_sibling(struct task_struct *p, int prev, int target)
{
	struct sched_domain *sd;
	struct sched_group *sg;
	int best_idle = -1;
	int best_idle_cstate = -1;
	int best_idle_capacity = INT_MAX;

	if (!sysctl_sched_cstate_aware) {
		if (idle_cpu(target))
			return target;

		/*
		 * If the prevous cpu is cache affine and idle, don't be stupid.
		 */
		if (prev != target && cpus_share_cache(prev, target) && idle_cpu(prev))
			return prev;
	}

	if (!sysctl_sched_wake_to_idle &&
	    !(current->flags & PF_WAKE_UP_IDLE) &&
	    !(p->flags & PF_WAKE_UP_IDLE))
		return target;

	/*
	 * Otherwise, iterate the domains and find an eligible idle cpu.
	 *
	 * A completely idle sched group at higher domains is more
	 * desirable than an idle group at a lower level, because lower
	 * domains have smaller groups and usually share hardware
	 * resources which causes tasks to contend on them, e.g. x86
	 * hyperthread siblings in the lowest domain (SMT) can contend
	 * on the shared cpu pipeline.
	 *
	 * However, while we prefer idle groups at higher domains
	 * finding an idle cpu at the lowest domain is still better than
	 * returning 'target', which we've already established, isn't
	 * idle.
	 */
	sd = rcu_dereference(per_cpu(sd_llc, target));
	for_each_lower_domain(sd) {
		sg = sd->groups;
		do {
			int i;

			if (!cpumask_intersects(sched_group_cpus(sg),
						tsk_cpus_allowed(p)))
				goto next;


			if (sysctl_sched_cstate_aware) {
				for_each_cpu_and(i, tsk_cpus_allowed(p), sched_group_cpus(sg)) {
					struct rq *rq = cpu_rq(i);
					int idle_idx = idle_get_state_idx(rq);
					unsigned long new_usage = boosted_task_util(p);
					unsigned long capacity_orig = capacity_orig_of(i);
					if (new_usage > capacity_orig || !idle_cpu(i))
						goto next;

					if (i == target && new_usage <= capacity_curr_of(target))
						return target;

					if (best_idle < 0 || (idle_idx < best_idle_cstate && capacity_orig <= best_idle_capacity)) {
						best_idle = i;
						best_idle_cstate = idle_idx;
						best_idle_capacity = capacity_orig;
					}
				}
			} else {
				/* Ensure the entire group is idle */
				for_each_cpu(i, sched_group_cpus(sg)) {
					if (i == target || !idle_cpu(i))
						goto next;
				}
			/*
			 * It doesn't matter which cpu we pick, the
			 * whole group is idle.
			 */
				target = cpumask_first_and(sched_group_cpus(sg),
					tsk_cpus_allowed(p));
				goto done;
			}
next:
			sg = sg->next;
		} while (sg != sd->groups);
	}
	if (best_idle > 0)
		target = best_idle;

done:
	return target;
}

static inline int find_best_target(struct task_struct *p, bool boosted, bool prefer_idle)
{
	int iter_cpu;
	int target_cpu = -1;
	int target_util = 0;
	int backup_capacity = 0;
	int best_idle_cpu = -1;
	int best_idle_cstate = INT_MAX;
	int backup_cpu = -1;
	unsigned long task_util_boosted, new_util;

	task_util_boosted = boosted_task_util(p);
	for (iter_cpu = 0; iter_cpu < NR_CPUS; iter_cpu++) {
		int cur_capacity;
		struct rq *rq;
		int idle_idx;

		/*
		 * Iterate from higher cpus for boosted tasks.
		 */
		int i = boosted ? NR_CPUS-iter_cpu-1 : iter_cpu;

		if (!cpu_online(i) || !cpumask_test_cpu(i, tsk_cpus_allowed(p)))
			continue;

		/*
		 * p's blocked utilization is still accounted for on prev_cpu
		 * so prev_cpu will receive a negative bias due to the double
		 * accounting. However, the blocked utilization may be zero.
		 */
		new_util = cpu_util(i) + task_util_boosted;

		/*
		 * Ensure minimum capacity to grant the required boost.
		 * The target CPU can be already at a capacity level higher
		 * than the one required to boost the task.
		 */
		if (new_util > capacity_orig_of(i))
			continue;

#ifdef CONFIG_SCHED_WALT
		if (walt_cpu_high_irqload(i))
			continue;
#endif
		/*
		 * Unconditionally favoring tasks that prefer idle cpus to
		 * improve latency.
		 */
		if (idle_cpu(i) && prefer_idle) {
			if (best_idle_cpu < 0)
				best_idle_cpu = i;
			continue;
		}

		cur_capacity = capacity_curr_of(i);
		rq = cpu_rq(i);
		idle_idx = idle_get_state_idx(rq);

		if (new_util < cur_capacity) {
			if (cpu_rq(i)->nr_running) {
				if (prefer_idle) {
					/* Find a target cpu with highest
					 * utilization.
					 */
					if (target_util == 0 ||
						target_util < new_util) {
						target_cpu = i;
						target_util = new_util;
					}
				} else {
					/* Find a target cpu with lowest
					 * utilization.
					 */
					if (target_util == 0 ||
						target_util > new_util) {
						target_cpu = i;
						target_util = new_util;
					}
				}
			} else if (!prefer_idle) {
				if (best_idle_cpu < 0 ||
					(sysctl_sched_cstate_aware &&
						best_idle_cstate > idle_idx)) {
					best_idle_cstate = idle_idx;
					best_idle_cpu = i;
				}
			}
		} else if (backup_capacity == 0 ||
				backup_capacity > cur_capacity) {
			// Find a backup cpu with least capacity.
			backup_capacity = cur_capacity;
			backup_cpu = i;
		}
	}

	if (prefer_idle && best_idle_cpu >= 0)
		target_cpu = best_idle_cpu;
	else if (target_cpu < 0)
		target_cpu = best_idle_cpu >= 0 ? best_idle_cpu : backup_cpu;

	return target_cpu;
}

static int energy_aware_wake_cpu(struct task_struct *p, int target, int sync)
{
	struct sched_domain *sd;
	struct sched_group *sg, *sg_target;
	int target_max_cap = INT_MAX;
	int target_cpu = task_cpu(p);
	unsigned long task_util_boosted, new_util;
	int i;

	if (sysctl_sched_sync_hint_enable && sync) {
		int cpu = smp_processor_id();
		cpumask_t search_cpus;
		cpumask_and(&search_cpus, tsk_cpus_allowed(p), cpu_online_mask);
		if (cpumask_test_cpu(cpu, &search_cpus))
			return cpu;
	}

	sd = rcu_dereference(per_cpu(sd_ea, task_cpu(p)));

	if (!sd)
		return target;

	sg = sd->groups;
	sg_target = sg;

	if (sysctl_sched_is_big_little) {

		/*
		 * Find group with sufficient capacity. We only get here if no cpu is
		 * overutilized. We may end up overutilizing a cpu by adding the task,
		 * but that should not be any worse than select_idle_sibling().
		 * load_balance() should sort it out later as we get above the tipping
		 * point.
		 */
		do {
			/* Assuming all cpus are the same in group */
			int max_cap_cpu = group_first_cpu(sg);

			/*
			 * Assume smaller max capacity means more energy-efficient.
			 * Ideally we should query the energy model for the right
			 * answer but it easily ends up in an exhaustive search.
			 */
			if (capacity_of(max_cap_cpu) < target_max_cap &&
			    task_fits_max(p, max_cap_cpu)) {
				sg_target = sg;
				target_max_cap = capacity_of(max_cap_cpu);
			}
		} while (sg = sg->next, sg != sd->groups);

		task_util_boosted = boosted_task_util(p);
		/* Find cpu with sufficient capacity */
		for_each_cpu_and(i, tsk_cpus_allowed(p), sched_group_cpus(sg_target)) {
			/*
			 * p's blocked utilization is still accounted for on prev_cpu
			 * so prev_cpu will receive a negative bias due to the double
			 * accounting. However, the blocked utilization may be zero.
			 */
			new_util = cpu_util(i) + task_util_boosted;

			/*
			 * Ensure minimum capacity to grant the required boost.
			 * The target CPU can be already at a capacity level higher
			 * than the one required to boost the task.
			 */
			if (new_util > capacity_orig_of(i))
				continue;

			if (new_util < capacity_curr_of(i)) {
				target_cpu = i;
				if (cpu_rq(i)->nr_running)
					break;
			}

			/* cpu has capacity at higher OPP, keep it as fallback */
			if (target_cpu == task_cpu(p))
				target_cpu = i;
		}
	} else {
		/*
		 * Find a cpu with sufficient capacity
		 */
#ifdef CONFIG_CGROUP_SCHEDTUNE
		bool boosted = schedtune_task_boost(p) > 0;
		bool prefer_idle = schedtune_prefer_idle(p) > 0;
#else
		bool boosted = 0;
		bool prefer_idle = 0;
#endif
		int tmp_target = find_best_target(p, boosted, prefer_idle);
		if (tmp_target >= 0) {
			target_cpu = tmp_target;
			if ((boosted || prefer_idle) && idle_cpu(target_cpu))
				return target_cpu;
		}
	}

	if (target_cpu != task_cpu(p)) {
		struct energy_env eenv = {
			.util_delta	= task_util(p),
			.src_cpu	= task_cpu(p),
			.dst_cpu	= target_cpu,
			.task		= p,
		};

		/* Not enough spare capacity on previous cpu */
		if (cpu_overutilized(task_cpu(p)))
			return target_cpu;

		if (energy_diff(&eenv) >= 0)
			return task_cpu(p);
	}

	return target_cpu;
}

/*
 * select_task_rq_fair: Select target runqueue for the waking task in domains
 * that have the 'sd_flag' flag set. In practice, this is SD_BALANCE_WAKE,
 * SD_BALANCE_FORK, or SD_BALANCE_EXEC.
 *
 * Balances load by selecting the idlest cpu in the idlest group, or under
 * certain conditions an idle sibling cpu if the domain has SD_WAKE_AFFINE set.
 *
 * Returns the target cpu number.
 *
 * preempt must be disabled.
 */
static int
select_task_rq_fair(struct task_struct *p, int prev_cpu, int sd_flag, int wake_flags)
{
	struct sched_domain *tmp, *affine_sd = NULL, *sd = NULL;
	int cpu = smp_processor_id();
	int new_cpu = prev_cpu;
	int want_affine = 0;
	int sync = wake_flags & WF_SYNC;

	if (sd_flag & SD_BALANCE_WAKE) {
		record_wakee(p);
		want_affine = (!wake_wide(p) && task_fits_max(p, cpu) &&
			      cpumask_test_cpu(cpu, tsk_cpus_allowed(p))) ||
			      energy_aware();
	}

	rcu_read_lock();
	for_each_domain(cpu, tmp) {
		if (!(tmp->flags & SD_LOAD_BALANCE))
			break;

		/*
		 * If both cpu and prev_cpu are part of this domain,
		 * cpu is a valid SD_WAKE_AFFINE target.
		 */
		if (want_affine && (tmp->flags & SD_WAKE_AFFINE) &&
		    cpumask_test_cpu(prev_cpu, sched_domain_span(tmp))) {
			affine_sd = tmp;
			break;
		}

		if (tmp->flags & sd_flag)
			sd = tmp;
		else if (!want_affine)
			break;
	}

	if (affine_sd) {
		sd = NULL; /* Prefer wake_affine over balance flags */
		if (cpu != prev_cpu && wake_affine(affine_sd, p, prev_cpu, sync))
			new_cpu = cpu;
	}

	if (!sd) {
		if (energy_aware() && !cpu_rq(cpu)->rd->overutilized)
			new_cpu = energy_aware_wake_cpu(p, prev_cpu, sync);
		else if (sd_flag & SD_BALANCE_WAKE) /* XXX always ? */
			new_cpu = select_idle_sibling(p, prev_cpu, new_cpu);

	} else while (sd) {
		struct sched_group *group;
		int weight;

		if (!(sd->flags & sd_flag)) {
			sd = sd->child;
			continue;
		}

		group = find_idlest_group(sd, p, cpu, sd_flag);
		if (!group) {
			sd = sd->child;
			continue;
		}

		new_cpu = find_idlest_cpu(group, p, cpu);
		if (new_cpu == -1 || new_cpu == cpu) {
			/* Now try balancing at a lower domain level of cpu */
			sd = sd->child;
			continue;
		}

		/* Now try balancing at a lower domain level of new_cpu */
		cpu = new_cpu;
		weight = sd->span_weight;
		sd = NULL;
		for_each_domain(cpu, tmp) {
			if (weight <= tmp->span_weight)
				break;
			if (tmp->flags & sd_flag)
				sd = tmp;
		}
		/* while loop will break here if sd == NULL */
	}
	rcu_read_unlock();

	return new_cpu;
}

/*
 * Called immediately before a task is migrated to a new cpu; task_cpu(p) and
 * cfs_rq_of(p) references at time of call are still valid and identify the
 * previous cpu. The caller guarantees p->pi_lock or task_rq(p)->lock is held.
 */
static void migrate_task_rq_fair(struct task_struct *p)
{
	/*
	 * As blocked tasks retain absolute vruntime the migration needs to
	 * deal with this by subtracting the old and adding the new
	 * min_vruntime -- the latter is done by enqueue_entity() when placing
	 * the task on the new runqueue.
	 */
	if (p->state == TASK_WAKING) {
		struct sched_entity *se = &p->se;
		struct cfs_rq *cfs_rq = cfs_rq_of(se);
		u64 min_vruntime;

#ifndef CONFIG_64BIT
		u64 min_vruntime_copy;

		do {
			min_vruntime_copy = cfs_rq->min_vruntime_copy;
			smp_rmb();
			min_vruntime = cfs_rq->min_vruntime;
		} while (min_vruntime != min_vruntime_copy);
#else
		min_vruntime = cfs_rq->min_vruntime;
#endif

		se->vruntime -= min_vruntime;
	}

	/*
	 * We are supposed to update the task to "current" time, then its up to date
	 * and ready to go to new CPU/cfs_rq. But we have difficulty in getting
	 * what current time is, so simply throw away the out-of-date time. This
	 * will result in the wakee task is less decayed, but giving the wakee more
	 * load sounds not bad.
	 */
	remove_entity_load_avg(&p->se);

	/* Tell new CPU we are migrated */
	p->se.avg.last_update_time = 0;

	/* We have migrated, no longer consider this task hot */
	p->se.exec_start = 0;
}

static void task_dead_fair(struct task_struct *p)
{
	remove_entity_load_avg(&p->se);
}
#else
#define task_fits_max(p, cpu) true
#endif /* CONFIG_SMP */

static unsigned long
wakeup_gran(struct sched_entity *curr, struct sched_entity *se)
{
	unsigned long gran = sysctl_sched_wakeup_granularity;

	/*
	 * Since its curr running now, convert the gran from real-time
	 * to virtual-time in his units.
	 *
	 * By using 'se' instead of 'curr' we penalize light tasks, so
	 * they get preempted easier. That is, if 'se' < 'curr' then
	 * the resulting gran will be larger, therefore penalizing the
	 * lighter, if otoh 'se' > 'curr' then the resulting gran will
	 * be smaller, again penalizing the lighter task.
	 *
	 * This is especially important for buddies when the leftmost
	 * task is higher priority than the buddy.
	 */
	return calc_delta_fair(gran, se);
}

/*
 * Should 'se' preempt 'curr'.
 *
 *             |s1
 *        |s2
 *   |s3
 *         g
 *      |<--->|c
 *
 *  w(c, s1) = -1
 *  w(c, s2) =  0
 *  w(c, s3) =  1
 *
 */
static int
wakeup_preempt_entity(struct sched_entity *curr, struct sched_entity *se)
{
	s64 gran, vdiff = curr->vruntime - se->vruntime;

	if (vdiff <= 0)
		return -1;

	gran = wakeup_gran(curr, se);
	if (vdiff > gran)
		return 1;

	return 0;
}

static void set_last_buddy(struct sched_entity *se)
{
	if (entity_is_task(se) && unlikely(task_of(se)->policy == SCHED_IDLE))
		return;

	for_each_sched_entity(se)
		cfs_rq_of(se)->last = se;
}

static void set_next_buddy(struct sched_entity *se)
{
	if (entity_is_task(se) && unlikely(task_of(se)->policy == SCHED_IDLE))
		return;

	for_each_sched_entity(se)
		cfs_rq_of(se)->next = se;
}

static void set_skip_buddy(struct sched_entity *se)
{
	for_each_sched_entity(se)
		cfs_rq_of(se)->skip = se;
}

/*
 * Preempt the current task with a newly woken task if needed:
 */
static void check_preempt_wakeup(struct rq *rq, struct task_struct *p, int wake_flags)
{
	struct task_struct *curr = rq->curr;
	struct sched_entity *se = &curr->se, *pse = &p->se;
	struct cfs_rq *cfs_rq = task_cfs_rq(curr);
	int scale = cfs_rq->nr_running >= sched_nr_latency;
	int next_buddy_marked = 0;

	if (unlikely(se == pse))
		return;

	/*
	 * This is possible from callers such as attach_tasks(), in which we
	 * unconditionally check_prempt_curr() after an enqueue (which may have
	 * lead to a throttle).  This both saves work and prevents false
	 * next-buddy nomination below.
	 */
	if (unlikely(throttled_hierarchy(cfs_rq_of(pse))))
		return;

	if (sched_feat(NEXT_BUDDY) && scale && !(wake_flags & WF_FORK)) {
		set_next_buddy(pse);
		next_buddy_marked = 1;
	}

	/*
	 * We can come here with TIF_NEED_RESCHED already set from new task
	 * wake up path.
	 *
	 * Note: this also catches the edge-case of curr being in a throttled
	 * group (e.g. via set_curr_task), since update_curr() (in the
	 * enqueue of curr) will have resulted in resched being set.  This
	 * prevents us from potentially nominating it as a false LAST_BUDDY
	 * below.
	 */
	if (test_tsk_need_resched(curr))
		return;

	/* Idle tasks are by definition preempted by non-idle tasks. */
	if (unlikely(curr->policy == SCHED_IDLE) &&
	    likely(p->policy != SCHED_IDLE))
		goto preempt;

	/*
	 * Batch and idle tasks do not preempt non-idle tasks (their preemption
	 * is driven by the tick):
	 */
	if (unlikely(p->policy != SCHED_NORMAL) || !sched_feat(WAKEUP_PREEMPTION))
		return;

	find_matching_se(&se, &pse);
	update_curr(cfs_rq_of(se));
	BUG_ON(!pse);
	if (wakeup_preempt_entity(se, pse) == 1) {
		/*
		 * Bias pick_next to pick the sched entity that is
		 * triggering this preemption.
		 */
		if (!next_buddy_marked)
			set_next_buddy(pse);
		goto preempt;
	}

	return;

preempt:
	resched_curr(rq);
	/*
	 * Only set the backward buddy when the current task is still
	 * on the rq. This can happen when a wakeup gets interleaved
	 * with schedule on the idle_balance() point, which can
	 * drop the rq lock.
	 *
	 * Also, during early boot the idle thread is in the fair class,
	 * for obvious reasons its a bad idea to schedule back to it.
	 */
	if (unlikely(!se->on_rq || curr == rq->idle))
		return;

	if (sched_feat(LAST_BUDDY) && scale && entity_is_task(se))
		set_last_buddy(se);
}

static struct task_struct *
pick_next_task_fair(struct rq *rq, struct task_struct *prev, struct pin_cookie cookie)
{
	struct cfs_rq *cfs_rq = &rq->cfs;
	struct sched_entity *se;
	struct task_struct *p;
	int new_tasks;

again:
#ifdef CONFIG_FAIR_GROUP_SCHED
	if (!cfs_rq->nr_running)
		goto idle;

	if (prev->sched_class != &fair_sched_class)
		goto simple;

	/*
	 * Because of the set_next_buddy() in dequeue_task_fair() it is rather
	 * likely that a next task is from the same cgroup as the current.
	 *
	 * Therefore attempt to avoid putting and setting the entire cgroup
	 * hierarchy, only change the part that actually changes.
	 */

	do {
		struct sched_entity *curr = cfs_rq->curr;

		/*
		 * Since we got here without doing put_prev_entity() we also
		 * have to consider cfs_rq->curr. If it is still a runnable
		 * entity, update_curr() will update its vruntime, otherwise
		 * forget we've ever seen it.
		 */
		if (curr) {
			if (curr->on_rq)
				update_curr(cfs_rq);
			else
				curr = NULL;

			/*
			 * This call to check_cfs_rq_runtime() will do the
			 * throttle and dequeue its entity in the parent(s).
			 * Therefore the 'simple' nr_running test will indeed
			 * be correct.
			 */
			if (unlikely(check_cfs_rq_runtime(cfs_rq)))
				goto simple;
		}

		se = pick_next_entity(cfs_rq, curr);
		cfs_rq = group_cfs_rq(se);
	} while (cfs_rq);

	p = task_of(se);

	/*
	 * Since we haven't yet done put_prev_entity and if the selected task
	 * is a different task than we started out with, try and touch the
	 * least amount of cfs_rqs.
	 */
	if (prev != p) {
		struct sched_entity *pse = &prev->se;

		while (!(cfs_rq = is_same_group(se, pse))) {
			int se_depth = se->depth;
			int pse_depth = pse->depth;

			if (se_depth <= pse_depth) {
				put_prev_entity(cfs_rq_of(pse), pse);
				pse = parent_entity(pse);
			}
			if (se_depth >= pse_depth) {
				set_next_entity(cfs_rq_of(se), se);
				se = parent_entity(se);
			}
		}

		put_prev_entity(cfs_rq, pse);
		set_next_entity(cfs_rq, se);
	}

	if (hrtick_enabled(rq))
		hrtick_start_fair(rq, p);

	rq->misfit_task = !task_fits_max(p, rq->cpu);

	return p;
simple:
	cfs_rq = &rq->cfs;
#endif

	if (!cfs_rq->nr_running)
		goto idle;

	put_prev_task(rq, prev);

	do {
		se = pick_next_entity(cfs_rq, NULL);
		set_next_entity(cfs_rq, se);
		cfs_rq = group_cfs_rq(se);
	} while (cfs_rq);

	p = task_of(se);

	if (hrtick_enabled(rq))
		hrtick_start_fair(rq, p);

	rq->misfit_task = !task_fits_max(p, rq->cpu);

	return p;

idle:
	rq->misfit_task = 0;
	lockdep_unpin_lock(&rq->lock, cookie);
	new_tasks = idle_balance(rq);
	lockdep_repin_lock(&rq->lock, cookie);
	/*
	 * Because idle_balance() releases (and re-acquires) rq->lock, it is
	 * possible for any higher priority task to appear. In that case we
	 * must re-start the pick_next_entity() loop.
	 */
	if (new_tasks < 0)
		return RETRY_TASK;

	if (new_tasks > 0)
		goto again;

	return NULL;
}

/*
 * Account for a descheduled task:
 */
static void put_prev_task_fair(struct rq *rq, struct task_struct *prev)
{
	struct sched_entity *se = &prev->se;
	struct cfs_rq *cfs_rq;

	for_each_sched_entity(se) {
		cfs_rq = cfs_rq_of(se);
		put_prev_entity(cfs_rq, se);
	}
}

/*
 * sched_yield() is very simple
 *
 * The magic of dealing with the ->skip buddy is in pick_next_entity.
 */
static void yield_task_fair(struct rq *rq)
{
	struct task_struct *curr = rq->curr;
	struct cfs_rq *cfs_rq = task_cfs_rq(curr);
	struct sched_entity *se = &curr->se;

	/*
	 * Are we the only task in the tree?
	 */
	if (unlikely(rq->nr_running == 1))
		return;

	clear_buddies(cfs_rq, se);

	if (curr->policy != SCHED_BATCH) {
		update_rq_clock(rq);
		/*
		 * Update run-time statistics of the 'current'.
		 */
		update_curr(cfs_rq);
		/*
		 * Tell update_rq_clock() that we've just updated,
		 * so we don't do microscopic update in schedule()
		 * and double the fastpath cost.
		 */
		rq_clock_skip_update(rq, true);
	}

	set_skip_buddy(se);
}

static bool yield_to_task_fair(struct rq *rq, struct task_struct *p, bool preempt)
{
	struct sched_entity *se = &p->se;

	/* throttled hierarchies are not runnable */
	if (!se->on_rq || throttled_hierarchy(cfs_rq_of(se)))
		return false;

	/* Tell the scheduler that we'd really like pse to run next. */
	set_next_buddy(se);

	yield_task_fair(rq);

	return true;
}

#ifdef CONFIG_SMP
/**************************************************
 * Fair scheduling class load-balancing methods.
 *
 * BASICS
 *
 * The purpose of load-balancing is to achieve the same basic fairness the
 * per-cpu scheduler provides, namely provide a proportional amount of compute
 * time to each task. This is expressed in the following equation:
 *
 *   W_i,n/P_i == W_j,n/P_j for all i,j                               (1)
 *
 * Where W_i,n is the n-th weight average for cpu i. The instantaneous weight
 * W_i,0 is defined as:
 *
 *   W_i,0 = \Sum_j w_i,j                                             (2)
 *
 * Where w_i,j is the weight of the j-th runnable task on cpu i. This weight
 * is derived from the nice value as per sched_prio_to_weight[].
 *
 * The weight average is an exponential decay average of the instantaneous
 * weight:
 *
 *   W'_i,n = (2^n - 1) / 2^n * W_i,n + 1 / 2^n * W_i,0               (3)
 *
 * C_i is the compute capacity of cpu i, typically it is the
 * fraction of 'recent' time available for SCHED_OTHER task execution. But it
 * can also include other factors [XXX].
 *
 * To achieve this balance we define a measure of imbalance which follows
 * directly from (1):
 *
 *   imb_i,j = max{ avg(W/C), W_i/C_i } - min{ avg(W/C), W_j/C_j }    (4)
 *
 * We them move tasks around to minimize the imbalance. In the continuous
 * function space it is obvious this converges, in the discrete case we get
 * a few fun cases generally called infeasible weight scenarios.
 *
 * [XXX expand on:
 *     - infeasible weights;
 *     - local vs global optima in the discrete case. ]
 *
 *
 * SCHED DOMAINS
 *
 * In order to solve the imbalance equation (4), and avoid the obvious O(n^2)
 * for all i,j solution, we create a tree of cpus that follows the hardware
 * topology where each level pairs two lower groups (or better). This results
 * in O(log n) layers. Furthermore we reduce the number of cpus going up the
 * tree to only the first of the previous level and we decrease the frequency
 * of load-balance at each level inv. proportional to the number of cpus in
 * the groups.
 *
 * This yields:
 *
 *     log_2 n     1     n
 *   \Sum       { --- * --- * 2^i } = O(n)                            (5)
 *     i = 0      2^i   2^i
 *                               `- size of each group
 *         |         |     `- number of cpus doing load-balance
 *         |         `- freq
 *         `- sum over all levels
 *
 * Coupled with a limit on how many tasks we can migrate every balance pass,
 * this makes (5) the runtime complexity of the balancer.
 *
 * An important property here is that each CPU is still (indirectly) connected
 * to every other cpu in at most O(log n) steps:
 *
 * The adjacency matrix of the resulting graph is given by:
 *
 *             log_2 n
 *   A_i,j = \Union     (i % 2^k == 0) && i / 2^(k+1) == j / 2^(k+1)  (6)
 *             k = 0
 *
 * And you'll find that:
 *
 *   A^(log_2 n)_i,j != 0  for all i,j                                (7)
 *
 * Showing there's indeed a path between every cpu in at most O(log n) steps.
 * The task movement gives a factor of O(m), giving a convergence complexity
 * of:
 *
 *   O(nm log n),  n := nr_cpus, m := nr_tasks                        (8)
 *
 *
 * WORK CONSERVING
 *
 * In order to avoid CPUs going idle while there's still work to do, new idle
 * balancing is more aggressive and has the newly idle cpu iterate up the domain
 * tree itself instead of relying on other CPUs to bring it work.
 *
 * This adds some complexity to both (5) and (8) but it reduces the total idle
 * time.
 *
 * [XXX more?]
 *
 *
 * CGROUPS
 *
 * Cgroups make a horror show out of (2), instead of a simple sum we get:
 *
 *                                s_k,i
 *   W_i,0 = \Sum_j \Prod_k w_k * -----                               (9)
 *                                 S_k
 *
 * Where
 *
 *   s_k,i = \Sum_j w_i,j,k  and  S_k = \Sum_i s_k,i                 (10)
 *
 * w_i,j,k is the weight of the j-th runnable task in the k-th cgroup on cpu i.
 *
 * The big problem is S_k, its a global sum needed to compute a local (W_i)
 * property.
 *
 * [XXX write more on how we solve this.. _after_ merging pjt's patches that
 *      rewrite all of this once again.]
 */

static unsigned long __read_mostly max_load_balance_interval = HZ/10;

enum group_type {
	group_other = 0,
	group_misfit_task,
	group_imbalanced,
	group_overloaded,
};

#define LBF_ALL_PINNED	0x01
#define LBF_NEED_BREAK	0x02
#define LBF_DST_PINNED  0x04
#define LBF_SOME_PINNED	0x08

struct lb_env {
	struct sched_domain	*sd;

	struct rq		*src_rq;
	int			src_cpu;

	int			dst_cpu;
	struct rq		*dst_rq;

	struct cpumask		*dst_grpmask;
	int			new_dst_cpu;
	enum cpu_idle_type	idle;
	long			imbalance;
	unsigned int		src_grp_nr_running;
	/* The set of CPUs under consideration for load-balancing */
	struct cpumask		*cpus;

	unsigned int		flags;

	unsigned int		loop;
	unsigned int		loop_break;
	unsigned int		loop_max;
	enum group_type		busiest_group_type;
	struct list_head	tasks;
};

static DEFINE_PER_CPU(bool, dbs_boost_needed);
static DEFINE_PER_CPU(int, dbs_boost_load_moved);

/*
 * Is this task likely cache-hot:
 */
static int task_hot(struct task_struct *p, struct lb_env *env)
{
	s64 delta;

	lockdep_assert_held(&env->src_rq->lock);

	if (p->sched_class != &fair_sched_class)
		return 0;

	if (unlikely(p->policy == SCHED_IDLE))
		return 0;

	/*
	 * Buddy candidates are cache hot:
	 */
	if (sched_feat(CACHE_HOT_BUDDY) && env->dst_rq->nr_running &&
			(&p->se == cfs_rq_of(&p->se)->next ||
			 &p->se == cfs_rq_of(&p->se)->last))
		return 1;

	if (sysctl_sched_migration_cost == -1)
		return 1;
	if (sysctl_sched_migration_cost == 0)
		return 0;

	delta = rq_clock_task(env->src_rq) - p->se.exec_start;

	return delta < (s64)sysctl_sched_migration_cost;
}

/*
 * can_migrate_task - may task p from runqueue rq be migrated to this_cpu?
 */
static
int can_migrate_task(struct task_struct *p, struct lb_env *env)
{
	int tsk_cache_hot = 0;

	lockdep_assert_held(&env->src_rq->lock);

	/*
	 * We do not migrate tasks that are:
	 * 1) throttled_lb_pair, or
	 * 2) cannot be migrated to this CPU due to cpus_allowed, or
	 * 3) running (obviously), or
	 * 4) are cache-hot on their current CPU.
	 */
	if (throttled_lb_pair(task_group(p), env->src_cpu, env->dst_cpu))
		return 0;

	if (!cpumask_test_cpu(env->dst_cpu, tsk_cpus_allowed(p))) {
		int cpu;

		schedstat_inc(p, se.statistics.nr_failed_migrations_affine);

		env->flags |= LBF_SOME_PINNED;

		/*
		 * Remember if this task can be migrated to any other cpu in
		 * our sched_group. We may want to revisit it if we couldn't
		 * meet load balance goals by pulling other tasks on src_cpu.
		 *
		 * Also avoid computing new_dst_cpu if we have already computed
		 * one in current iteration.
		 */
		if (!env->dst_grpmask || (env->flags & LBF_DST_PINNED))
			return 0;

		/* Prevent to re-select dst_cpu via env's cpus */
		for_each_cpu_and(cpu, env->dst_grpmask, env->cpus) {
			if (cpumask_test_cpu(cpu, tsk_cpus_allowed(p))) {
				env->flags |= LBF_DST_PINNED;
				env->new_dst_cpu = cpu;
				break;
			}
		}

		return 0;
	}

	/* Record that we found atleast one task that could run on dst_cpu */
	env->flags &= ~LBF_ALL_PINNED;

	if (task_running(env->src_rq, p)) {
		schedstat_inc(p, se.statistics.nr_failed_migrations_running);
		return 0;
	}

	/*
	 * Aggressive migration if:
	 * 1) destination numa is preferred
	 * 2) task is cache cold, or
	 * 3) too many balance attempts have failed.
	 */

	tsk_cache_hot = task_hot(p, env);
	if (!tsk_cache_hot ||
		env->sd->nr_balance_failed > env->sd->cache_nice_tries) {

		if (tsk_cache_hot) {
			schedstat_inc(env->sd, lb_hot_gained[env->idle]);
			schedstat_inc(p, se.statistics.nr_forced_migrations);
		}

		return 1;
	}

	schedstat_inc(p, se.statistics.nr_failed_migrations_hot);
	return 0;
}

/*
 * detach_task() -- detach the task for the migration specified in env
 */
static void detach_task(struct task_struct *p, struct lb_env *env)
{
	lockdep_assert_held(&env->src_rq->lock);

	p->on_rq = TASK_ON_RQ_MIGRATING;
	deactivate_task(env->src_rq, p, 0);
	double_lock_balance(env->src_rq, env->dst_rq);
	set_task_cpu(p, env->dst_cpu);
	double_unlock_balance(env->src_rq, env->dst_rq);
}

/*
 * detach_one_task() -- tries to dequeue exactly one task from env->src_rq, as
 * part of active balancing operations within "domain".
 *
 * Returns a task if successful and NULL otherwise.
 */
static struct task_struct *detach_one_task(struct lb_env *env)
{
	struct task_struct *p, *n;

	lockdep_assert_held(&env->src_rq->lock);

	list_for_each_entry_safe(p, n, &env->src_rq->cfs_tasks, se.group_node) {
		if (!can_migrate_task(p, env))
			continue;

		detach_task(p, env);

		/*
		 * Right now, this is only the second place where
		 * lb_gained[env->idle] is updated (other is detach_tasks)
		 * so we can safely collect stats here rather than
		 * inside detach_tasks().
		 */
		schedstat_inc(env->sd, lb_gained[env->idle]);
		return p;
	}
	return NULL;
}

static const unsigned int sched_nr_migrate_break = 32;

/*
 * detach_tasks() -- tries to detach up to imbalance weighted load from
 * busiest_rq, as part of a balancing operation within domain "sd".
 *
 * Returns number of detached tasks if successful and 0 otherwise.
 */
static int detach_tasks(struct lb_env *env)
{
	struct list_head *tasks = &env->src_rq->cfs_tasks;
	struct task_struct *p;
	unsigned long load;
	int detached = 0;

	lockdep_assert_held(&env->src_rq->lock);

	if (env->imbalance <= 0)
		return 0;

	while (!list_empty(tasks)) {
		/*
		 * We don't want to steal all, otherwise we may be treated likewise,
		 * which could at worst lead to a livelock crash.
		 */
		if (env->idle != CPU_NOT_IDLE && env->src_rq->nr_running <= 1)
			break;

		p = list_first_entry(tasks, struct task_struct, se.group_node);

		env->loop++;
		/* We've more or less seen every task there is, call it quits */
		if (env->loop > env->loop_max)
			break;

		/* take a breather every nr_migrate tasks */
		if (env->loop > env->loop_break) {
			env->loop_break += sched_nr_migrate_break;
			env->flags |= LBF_NEED_BREAK;
			break;
		}

		if (!can_migrate_task(p, env))
			goto next;

		load = task_h_load(p);

		if (sched_feat(LB_MIN) && load < 16 && !env->sd->nr_balance_failed)
			goto next;

		if ((load / 2) > env->imbalance)
			goto next;

		detach_task(p, env);
		list_add(&p->se.group_node, &env->tasks);

		detached++;
		env->imbalance -= load;

#ifdef CONFIG_PREEMPT
		/*
		 * NEWIDLE balancing is a source of latency, so preemptible
		 * kernels will stop after the first task is detached to minimize
		 * the critical section.
		 */
		if (env->idle == CPU_NEWLY_IDLE)
			break;
#endif

		/*
		 * We only want to steal up to the prescribed amount of
		 * weighted load.
		 */
		if (env->imbalance <= 0)
			break;

		continue;
next:
		list_move_tail(&p->se.group_node, tasks);
	}

	/*
	 * Right now, this is one of only two places we collect this stat
	 * so we can safely collect detach_one_task() stats here rather
	 * than inside detach_one_task().
	 */
	schedstat_add(env->sd, lb_gained[env->idle], detached);

	return detached;
}

/*
 * attach_task() -- attach the task detached by detach_task() to its new rq.
 */
static void attach_task(struct rq *rq, struct task_struct *p)
{
	lockdep_assert_held(&rq->lock);

	BUG_ON(task_rq(p) != rq);
	activate_task(rq, p, 0);
	p->on_rq = TASK_ON_RQ_QUEUED;
	check_preempt_curr(rq, p, 0);
}

/*
 * attach_one_task() -- attaches the task returned from detach_one_task() to
 * its new rq.
 */
static void attach_one_task(struct rq *rq, struct task_struct *p)
{
	raw_spin_lock(&rq->lock);
	attach_task(rq, p);
	/*
	 * We want to potentially raise target_cpu's OPP.
	 */
	update_capacity_of(cpu_of(rq));
	raw_spin_unlock(&rq->lock);
}

/*
 * attach_tasks() -- attaches all tasks detached by detach_tasks() to their
 * new rq.
 */
static void attach_tasks(struct lb_env *env)
{
	struct list_head *tasks = &env->tasks;
	struct task_struct *p;

	raw_spin_lock(&env->dst_rq->lock);

	while (!list_empty(tasks)) {
		p = list_first_entry(tasks, struct task_struct, se.group_node);
		list_del_init(&p->se.group_node);

		attach_task(env->dst_rq, p);
	}

	/*
	 * We want to potentially raise env.dst_cpu's OPP.
	 */
	update_capacity_of(env->dst_cpu);

	raw_spin_unlock(&env->dst_rq->lock);
}

#ifdef CONFIG_FAIR_GROUP_SCHED
static void update_blocked_averages(int cpu)
{
	struct rq *rq = cpu_rq(cpu);
	struct cfs_rq *cfs_rq;
	unsigned long flags;

	raw_spin_lock_irqsave(&rq->lock, flags);
	update_rq_clock(rq);

	/*
	 * Iterates the task_group tree in a bottom up fashion, see
	 * list_add_leaf_cfs_rq() for details.
	 */
	for_each_leaf_cfs_rq(rq, cfs_rq) {
		/* throttled entities do not contribute to load */
		if (throttled_hierarchy(cfs_rq))
			continue;

		if (update_cfs_rq_load_avg(cfs_rq_clock_task(cfs_rq), cfs_rq))
			update_tg_load_avg(cfs_rq, 0);
	}
	raw_spin_unlock_irqrestore(&rq->lock, flags);
}

/*
 * Compute the hierarchical load factor for cfs_rq and all its ascendants.
 * This needs to be done in a top-down fashion because the load of a child
 * group is a fraction of its parents load.
 */
static void update_cfs_rq_h_load(struct cfs_rq *cfs_rq)
{
	struct rq *rq = rq_of(cfs_rq);
	struct sched_entity *se = cfs_rq->tg->se[cpu_of(rq)];
	unsigned long now = jiffies;
	unsigned long load;

	if (cfs_rq->last_h_load_update == now)
		return;

	cfs_rq->h_load_next = NULL;
	for_each_sched_entity(se) {
		cfs_rq = cfs_rq_of(se);
		cfs_rq->h_load_next = se;
		if (cfs_rq->last_h_load_update == now)
			break;
	}

	if (!se) {
		cfs_rq->h_load = cfs_rq_load_avg(cfs_rq);
		cfs_rq->last_h_load_update = now;
	}

	while ((se = cfs_rq->h_load_next) != NULL) {
		load = cfs_rq->h_load;
		load = div64_ul(load * se->avg.load_avg,
			cfs_rq_load_avg(cfs_rq) + 1);
		cfs_rq = group_cfs_rq(se);
		cfs_rq->h_load = load;
		cfs_rq->last_h_load_update = now;
	}
}

static unsigned long task_h_load(struct task_struct *p)
{
	struct cfs_rq *cfs_rq = task_cfs_rq(p);

	update_cfs_rq_h_load(cfs_rq);
	return div64_ul(p->se.avg.load_avg * cfs_rq->h_load,
			cfs_rq_load_avg(cfs_rq) + 1);
}
#else
static inline void update_blocked_averages(int cpu)
{
	struct rq *rq = cpu_rq(cpu);
	struct cfs_rq *cfs_rq = &rq->cfs;
	unsigned long flags;

	raw_spin_lock_irqsave(&rq->lock, flags);
	update_rq_clock(rq);
	update_cfs_rq_load_avg(cfs_rq_clock_task(cfs_rq), cfs_rq);
	raw_spin_unlock_irqrestore(&rq->lock, flags);
}

static unsigned long task_h_load(struct task_struct *p)
{
	return p->se.avg.load_avg;
}
#endif

/********** Helpers for find_busiest_group ************************/

/*
 * sg_lb_stats - stats of a sched_group required for load_balancing
 */
struct sg_lb_stats {
	unsigned long avg_load; /*Avg load across the CPUs of the group */
	unsigned long group_load; /* Total load over the CPUs of the group */
	unsigned long sum_weighted_load; /* Weighted load of group's tasks */
	unsigned long load_per_task;
	unsigned long group_capacity;
	unsigned long group_util; /* Total utilization of the group */
	unsigned int sum_nr_running; /* Nr tasks running in the group */
	unsigned int idle_cpus;
	unsigned int group_weight;
	enum group_type group_type;
	int group_no_capacity;
	int group_misfit_task; /* A cpu has a task too big for its capacity */
};

/*
 * sd_lb_stats - Structure to store the statistics of a sched_domain
 *		 during load balancing.
 */
struct sd_lb_stats {
	struct sched_group *busiest;	/* Busiest group in this sd */
	struct sched_group *local;	/* Local group in this sd */
	unsigned long total_load;	/* Total load of all groups in sd */
	unsigned long total_capacity;	/* Total capacity of all groups in sd */
	unsigned long avg_load;	/* Average load across all groups in sd */

	struct sg_lb_stats busiest_stat;/* Statistics of the busiest group */
	struct sg_lb_stats local_stat;	/* Statistics of the local group */
};

static inline void init_sd_lb_stats(struct sd_lb_stats *sds)
{
	/*
	 * Skimp on the clearing to avoid duplicate work. We can avoid clearing
	 * local_stat because update_sg_lb_stats() does a full clear/assignment.
	 * We must however clear busiest_stat::avg_load because
	 * update_sd_pick_busiest() reads this before assignment.
	 */
	*sds = (struct sd_lb_stats){
		.busiest = NULL,
		.local = NULL,
		.total_load = 0UL,
		.total_capacity = 0UL,
		.busiest_stat = {
			.avg_load = 0UL,
			.sum_nr_running = 0,
			.group_type = group_other,
		},
	};
}

/**
 * get_sd_load_idx - Obtain the load index for a given sched domain.
 * @sd: The sched_domain whose load_idx is to be obtained.
 * @idle: The Idle status of the CPU for whose sd load_icx is obtained.
 *
 * Return: The load index.
 */
static inline int get_sd_load_idx(struct sched_domain *sd,
					enum cpu_idle_type idle)
{
	int load_idx;

	switch (idle) {
	case CPU_NOT_IDLE:
		load_idx = sd->busy_idx;
		break;

	case CPU_NEWLY_IDLE:
		load_idx = sd->newidle_idx;
		break;
	default:
		load_idx = sd->idle_idx;
		break;
	}

	return load_idx;
}

static unsigned long scale_rt_capacity(int cpu)
{
	struct rq *rq = cpu_rq(cpu);
	u64 total, used, age_stamp, avg;
	s64 delta;

	/*
	 * Since we're reading these variables without serialization make sure
	 * we read them once before doing sanity checks on them.
	 */
	age_stamp = READ_ONCE(rq->age_stamp);
	avg = READ_ONCE(rq->rt_avg);
	delta = __rq_clock_broken(rq) - age_stamp;

	if (unlikely(delta < 0))
		delta = 0;

	total = sched_avg_period() + delta;

	used = div_u64(avg, total);

	/*
	 * deadline bandwidth is defined at system level so we must
	 * weight this bandwidth with the max capacity of the system.
	 * As a reminder, avg_bw is 20bits width and
	 * scale_cpu_capacity is 10 bits width
	 */
	used += div_u64(rq->dl.avg_bw, arch_scale_cpu_capacity(NULL, cpu));

	if (likely(used < SCHED_CAPACITY_SCALE))
		return SCHED_CAPACITY_SCALE - used;

	return 1;
}

void init_max_cpu_capacity(struct max_cpu_capacity *mcc)
{
	raw_spin_lock_init(&mcc->lock);
	mcc->val = 0;
	mcc->cpu = -1;
}

static void update_cpu_capacity(struct sched_domain *sd, int cpu)
{
	unsigned long capacity = arch_scale_cpu_capacity(sd, cpu);
	struct sched_group *sdg = sd->groups;
	struct max_cpu_capacity *mcc;
	unsigned long max_capacity;
	int max_cap_cpu;
	unsigned long flags;

	cpu_rq(cpu)->cpu_capacity_orig = capacity;

	mcc = &cpu_rq(cpu)->rd->max_cpu_capacity;

	raw_spin_lock_irqsave(&mcc->lock, flags);
	max_capacity = mcc->val;
	max_cap_cpu = mcc->cpu;

	if ((max_capacity > capacity && max_cap_cpu == cpu) ||
	    (max_capacity < capacity)) {
		mcc->val = capacity;
		mcc->cpu = cpu;
#ifdef CONFIG_SCHED_DEBUG
		raw_spin_unlock_irqrestore(&mcc->lock, flags);
		pr_info("CPU%d: update max cpu_capacity %lu\n", cpu, capacity);
		goto skip_unlock;
#endif
	}
	raw_spin_unlock_irqrestore(&mcc->lock, flags);

skip_unlock: __attribute__ ((unused));
	capacity *= scale_rt_capacity(cpu);
	capacity >>= SCHED_CAPACITY_SHIFT;

	if (!capacity)
		capacity = 1;

	cpu_rq(cpu)->cpu_capacity = capacity;
	sdg->sgc->capacity = capacity;
	sdg->sgc->max_capacity = capacity;
}

void update_group_capacity(struct sched_domain *sd, int cpu)
{
	struct sched_domain *child = sd->child;
	struct sched_group *group, *sdg = sd->groups;
	unsigned long capacity, max_capacity;
	unsigned long interval;

	interval = msecs_to_jiffies(sd->balance_interval);
	interval = clamp(interval, 1UL, max_load_balance_interval);
	sdg->sgc->next_update = jiffies + interval;

	if (!child) {
		update_cpu_capacity(sd, cpu);
		return;
	}

	capacity = 0;
	max_capacity = 0;

	if (child->flags & SD_OVERLAP) {
		/*
		 * SD_OVERLAP domains cannot assume that child groups
		 * span the current group.
		 */

		for_each_cpu(cpu, sched_group_cpus(sdg)) {
			struct sched_group_capacity *sgc;
			struct rq *rq = cpu_rq(cpu);

			/*
			 * build_sched_domains() -> init_sched_groups_capacity()
			 * gets here before we've attached the domains to the
			 * runqueues.
			 *
			 * Use capacity_of(), which is set irrespective of domains
			 * in update_cpu_capacity().
			 *
			 * This avoids capacity from being 0 and
			 * causing divide-by-zero issues on boot.
			 */
			if (unlikely(!rq->sd)) {
				capacity += capacity_of(cpu);
			} else {
				sgc = rq->sd->groups->sgc;
				capacity += sgc->capacity;
			}

			max_capacity = max(capacity, max_capacity);
		}
	} else  {
		/*
		 * !SD_OVERLAP domains can assume that child groups
		 * span the current group.
		 */

		group = child->groups;
		do {
			struct sched_group_capacity *sgc = group->sgc;

			capacity += sgc->capacity;
			max_capacity = max(sgc->max_capacity, max_capacity);
			group = group->next;
		} while (group != child->groups);
	}

	sdg->sgc->capacity = capacity;
	sdg->sgc->max_capacity = max_capacity;
}

/*
 * Check whether the capacity of the rq has been noticeably reduced by side
 * activity. The imbalance_pct is used for the threshold.
 * Return true is the capacity is reduced
 */
static inline int
check_cpu_capacity(struct rq *rq, struct sched_domain *sd)
{
	return ((rq->cpu_capacity * sd->imbalance_pct) <
				(rq->cpu_capacity_orig * 100));
}

/*
 * Group imbalance indicates (and tries to solve) the problem where balancing
 * groups is inadequate due to tsk_cpus_allowed() constraints.
 *
 * Imagine a situation of two groups of 4 cpus each and 4 tasks each with a
 * cpumask covering 1 cpu of the first group and 3 cpus of the second group.
 * Something like:
 *
 * 	{ 0 1 2 3 } { 4 5 6 7 }
 * 	        *     * * *
 *
 * If we were to balance group-wise we'd place two tasks in the first group and
 * two tasks in the second group. Clearly this is undesired as it will overload
 * cpu 3 and leave one of the cpus in the second group unused.
 *
 * The current solution to this issue is detecting the skew in the first group
 * by noticing the lower domain failed to reach balance and had difficulty
 * moving tasks due to affinity constraints.
 *
 * When this is so detected; this group becomes a candidate for busiest; see
 * update_sd_pick_busiest(). And calculate_imbalance() and
 * find_busiest_group() avoid some of the usual balance conditions to allow it
 * to create an effective group imbalance.
 *
 * This is a somewhat tricky proposition since the next run might not find the
 * group imbalance and decide the groups need to be balanced again. A most
 * subtle and fragile situation.
 */

static inline int sg_imbalanced(struct sched_group *group)
{
	return group->sgc->imbalance;
}

/*
 * group_has_capacity returns true if the group has spare capacity that could
 * be used by some tasks.
 * We consider that a group has spare capacity if the  * number of task is
 * smaller than the number of CPUs or if the utilization is lower than the
 * available capacity for CFS tasks.
 * For the latter, we use a threshold to stabilize the state, to take into
 * account the variance of the tasks' load and to return true if the available
 * capacity in meaningful for the load balancer.
 * As an example, an available capacity of 1% can appear but it doesn't make
 * any benefit for the load balance.
 */
static inline bool
group_has_capacity(struct lb_env *env, struct sg_lb_stats *sgs)
{
	if (sgs->sum_nr_running < sgs->group_weight)
		return true;

	if ((sgs->group_capacity * 100) >
			(sgs->group_util * env->sd->imbalance_pct))
		return true;

	return false;
}

/*
 *  group_is_overloaded returns true if the group has more tasks than it can
 *  handle.
 *  group_is_overloaded is not equals to !group_has_capacity because a group
 *  with the exact right number of tasks, has no more spare capacity but is not
 *  overloaded so both group_has_capacity and group_is_overloaded return
 *  false.
 */
static inline bool
group_is_overloaded(struct lb_env *env, struct sg_lb_stats *sgs)
{
	if (sgs->sum_nr_running <= sgs->group_weight)
		return false;

	if ((sgs->group_capacity * 100) <
			(sgs->group_util * env->sd->imbalance_pct))
		return true;

	return false;
}


/*
 * group_smaller_cpu_capacity: Returns true if sched_group sg has smaller
 * per-cpu capacity than sched_group ref.
 */
static inline bool
group_smaller_cpu_capacity(struct sched_group *sg, struct sched_group *ref)
{
	return sg->sgc->max_capacity + capacity_margin - SCHED_CAPACITY_SCALE <
							ref->sgc->max_capacity;
}

static inline enum
group_type group_classify(struct sched_group *group,
			  struct sg_lb_stats *sgs)
{
	if (sgs->group_no_capacity)
		return group_overloaded;

	if (sg_imbalanced(group))
		return group_imbalanced;

	if (sgs->group_misfit_task)
		return group_misfit_task;

	return group_other;
}

/**
 * update_sg_lb_stats - Update sched_group's statistics for load balancing.
 * @env: The load balancing environment.
 * @group: sched_group whose statistics are to be updated.
 * @idle: Idle status of this_cpu
 * @load_idx: Load index of sched_domain of this_cpu for load calc.
 * @local_group: Does group contain this_cpu.
 * @sgs: variable to hold the statistics for this group.
 * @overload: Indicate more than one runnable task for any CPU.
 * @overutilized: Indicate overutilization for any CPU.
 */
static inline void update_sg_lb_stats(struct lb_env *env,
			struct sched_group *group, int load_idx,
			int local_group, struct sg_lb_stats *sgs,
			bool *overload, bool *overutilized)
{
	unsigned long nr_running, max_nr_running, min_nr_running;
	unsigned long scaled_load, load, max_cpu_load, min_cpu_load;
	unsigned long balance_load = ~0UL;
	unsigned long avg_load_per_task = 0;
	int i;

	/* Tally up the load of all CPUs in the group */
	max_cpu_load = 0;
	min_cpu_load = ~0UL;
	max_nr_running = 0;

	memset(sgs, 0, sizeof(*sgs));

	for_each_cpu_and(i, sched_group_cpus(group), env->cpus) {
		struct rq *rq = cpu_rq(i);

		nr_running = rq->cfs.h_nr_running;

		/* Bias balancing toward cpus of our domain */
		if (local_group)
			load = target_load(i, load_idx);
		else
			load = source_load(i, load_idx);

		sgs->group_load += load;
		sgs->group_util += cpu_util(i);
		sgs->sum_nr_running += rq->cfs.h_nr_running;

		nr_running = rq->nr_running;
		if (nr_running > 1)
			*overload = true;

#ifdef CONFIG_NUMA_BALANCING
		sgs->nr_numa_running += rq->nr_numa_running;
		sgs->nr_preferred_running += rq->nr_preferred_running;
#endif
		sgs->sum_weighted_load += weighted_cpuload(i);
		/*
		 * No need to call idle_cpu() if nr_running is not 0
		 */
		if (!nr_running && idle_cpu(i))
			sgs->idle_cpus++;

		if (cpu_overutilized(i)) {
			*overutilized = true;
			if (!sgs->group_misfit_task && rq->misfit_task)
				sgs->group_misfit_task = capacity_of(i);
		}
	}

	if (local_group && (env->idle != CPU_NEWLY_IDLE ||
			time_after_eq(jiffies, group->sgc->next_update)))
		update_group_capacity(env->sd, env->dst_cpu);

	/* Adjust by relative CPU capacity of the group */
	sgs->group_capacity = group->sgc->capacity;
	sgs->avg_load = (sgs->group_load*SCHED_CAPACITY_SCALE) / sgs->group_capacity;

	/*
	 * Consider the group unbalanced when the imbalance is larger
	 * than the average weight of a task.
	 *
	 * APZ: with cgroup the avg task weight can vary wildly and
	 *      might not be a suitable number - should we keep a
	 *      normalized nr_running number somewhere that negates
	 *      the hierarchy?
	 */
	if (sgs->sum_nr_running) {
		avg_load_per_task = sgs->sum_weighted_load * SCHED_CAPACITY_SCALE
					/ group->sgc->capacity;
		avg_load_per_task /= sgs->sum_nr_running;
	}

	sgs->group_capacity =
		DIV_ROUND_CLOSEST(group->sgc->capacity, SCHED_CAPACITY_SCALE);

	sgs->group_weight = group->group_weight;

	sgs->group_no_capacity = group_is_overloaded(env, sgs);
	sgs->group_type = group_classify(group, sgs);
}

/**
 * update_sd_pick_busiest - return 1 on busiest group
 * @env: The load balancing environment.
 * @sds: sched_domain statistics
 * @sg: sched_group candidate to be checked for being the busiest
 * @sgs: sched_group statistics
 *
 * Determine if @sg is a busier group than the previously selected
 * busiest group.
 *
 * Return: %true if @sg is a busier group than the previously selected
 * busiest group. %false otherwise.
 */
static bool update_sd_pick_busiest(struct lb_env *env,
				   struct sd_lb_stats *sds,
				   struct sched_group *sg,
				   struct sg_lb_stats *sgs)
{
	struct sg_lb_stats *busiest = &sds->busiest_stat;

	if (sgs->group_type > busiest->group_type)
		return true;

	if (sgs->group_type < busiest->group_type)
		return false;

	/*
	 * Candidate sg doesn't face any serious load-balance problems
	 * so don't pick it if the local sg is already filled up.
	 */
	if (sgs->group_type == group_other &&
	    !group_has_capacity(env, &sds->local_stat))
		return false;

	if (sgs->avg_load <= busiest->avg_load)
		return false;

	/*
	 * Candiate sg has no more than one task per cpu and has higher
	 * per-cpu capacity. No reason to pull tasks to less capable cpus.
	 */
	if (sgs->sum_nr_running <= sgs->group_weight &&
	    group_smaller_cpu_capacity(sds->local, sg))
		return false;

	/* This is the busiest node in its class. */
	if (!(env->sd->flags & SD_ASYM_PACKING))
		return true;

	/* No ASYM_PACKING if target cpu is already busy */
	if (env->idle == CPU_NOT_IDLE)
		return true;
	/*
	 * ASYM_PACKING needs to move all the work to the lowest
	 * numbered CPUs in the group, therefore mark all groups
	 * higher than ourself as busy.
	 */
	if (sgs->sum_nr_running && env->dst_cpu < group_first_cpu(sg)) {
		if (!sds->busiest)
			return true;

		/* Prefer to move from highest possible cpu's work */
		if (group_first_cpu(sds->busiest) < group_first_cpu(sg))
			return true;
	}

	return false;
}

#ifdef CONFIG_NUMA_BALANCING
static inline enum fbq_type fbq_classify_group(struct sg_lb_stats *sgs)
{
	if (sgs->sum_nr_running > sgs->nr_numa_running)
		return regular;
	if (sgs->sum_nr_running > sgs->nr_preferred_running)
		return remote;
	return all;
}

static inline enum fbq_type fbq_classify_rq(struct rq *rq)
{
	if (rq->nr_running > rq->nr_numa_running)
		return regular;
	if (rq->nr_running > rq->nr_preferred_running)
		return remote;
	return all;
}
#endif
#if 0
static inline enum fbq_type fbq_classify_group(struct sg_lb_stats *sgs)
{
	return all;
}

static inline enum fbq_type fbq_classify_rq(struct rq *rq)
{
	return regular;
}
#endif /* CONFIG_NUMA_BALANCING */

/**
 * update_sd_lb_stats - Update sched_domain's statistics for load balancing.
 * @env: The load balancing environment.
 * @sds: variable to hold the statistics for this sched_domain.
 */
static inline void update_sd_lb_stats(struct lb_env *env, struct sd_lb_stats *sds)
{
	struct sched_domain *child = env->sd->child;
	struct sched_group *sg = env->sd->groups;
	struct sg_lb_stats tmp_sgs;
	int load_idx, prefer_sibling = 0;
	bool overload = false, overutilized = false;

	if (child && child->flags & SD_PREFER_SIBLING)
		prefer_sibling = 1;

	load_idx = get_sd_load_idx(env->sd, env->idle);

	do {
		struct sg_lb_stats *sgs = &tmp_sgs;
		int local_group;

		local_group = cpumask_test_cpu(env->dst_cpu, sched_group_cpus(sg));
		if (local_group) {
			sds->local = sg;
			sgs = &sds->local_stat;

			if (env->idle != CPU_NEWLY_IDLE ||
			    time_after_eq(jiffies, sg->sgc->next_update))
				update_group_capacity(env->sd, env->dst_cpu);
		}

		memset(sgs, 0, sizeof(*sgs));
		update_sg_lb_stats(env, sg, load_idx, local_group, sgs,
						&overload, &overutilized);

		if (local_group)
			goto next_group;

		/*
		 * In case the child domain prefers tasks go to siblings
		 * first, lower the sg capacity factor to one so that we'll try
		 * and move all the excess tasks away. We lower the capacity
		 * of a group only if the local group has the capacity to fit
		 * these excess tasks, i.e. nr_running < group_capacity_factor. The
		 * extra check prevents the case where you always pull from the
		 * heaviest group when it is already under-utilized (possible
		 * with a large weight task outweighs the tasks on the system).
		 */
		if (prefer_sibling && sds->local &&
		    group_has_capacity(env, &sds->local_stat) &&
		    (sgs->sum_nr_running > 1)) {
			sgs->group_no_capacity = 1;
			sgs->group_type = group_classify(sg, sgs);
		}

		/*
		 * Ignore task groups with misfit tasks if local group has no
		 * capacity or if per-cpu capacity isn't higher.
		 */
		if (sgs->group_type == group_misfit_task &&
		    (!group_has_capacity(env, &sds->local_stat) ||
		     !group_smaller_cpu_capacity(sg, sds->local)))
			sgs->group_type = group_other;
		if (update_sd_pick_busiest(env, sds, sg, sgs)) {
			sds->busiest = sg;
			sds->busiest_stat = *sgs;
		}

next_group:
		/* Now, start updating sd_lb_stats */
		sds->total_load += sgs->group_load;
		sds->total_capacity += sgs->group_capacity;

		sg = sg->next;
	} while (sg != env->sd->groups);

	env->src_grp_nr_running = sds->busiest_stat.sum_nr_running;

	if (!env->sd->parent) {
		/* update overload indicator if we are at root domain */
		if (env->dst_rq->rd->overload != overload)
			env->dst_rq->rd->overload = overload;

		/* Update over-utilization (tipping point, U >= 0) indicator */
		if (env->dst_rq->rd->overutilized != overutilized)
			env->dst_rq->rd->overutilized = overutilized;
	} else {
		if (!env->dst_rq->rd->overutilized && overutilized)
			env->dst_rq->rd->overutilized = true;
	}
}

/**
 * check_asym_packing - Check to see if the group is packed into the
 *			sched doman.
 *
 * This is primarily intended to used at the sibling level.  Some
 * cores like POWER7 prefer to use lower numbered SMT threads.  In the
 * case of POWER7, it can move to lower SMT modes only when higher
 * threads are idle.  When in lower SMT modes, the threads will
 * perform better since they share less core resources.  Hence when we
 * have idle threads, we want them to be the higher ones.
 *
 * This packing function is run on idle threads.  It checks to see if
 * the busiest CPU in this domain (core in the P7 case) has a higher
 * CPU number than the packing function is being run on.  Here we are
 * assuming lower CPU number will be equivalent to lower a SMT thread
 * number.
 *
 * Return: 1 when packing is required and a task should be moved to
 * this CPU.  The amount of the imbalance is returned in *imbalance.
 *
 * @env: The load balancing environment.
 * @sds: Statistics of the sched_domain which is to be packed
 */
static int check_asym_packing(struct lb_env *env, struct sd_lb_stats *sds)
{
	int busiest_cpu;

	if (!(env->sd->flags & SD_ASYM_PACKING))
		return 0;

	if (env->idle == CPU_NOT_IDLE)
		return 0;

	if (!sds->busiest)
		return 0;

	busiest_cpu = group_first_cpu(sds->busiest);
	if (env->dst_cpu > busiest_cpu)
		return 0;

	env->imbalance = DIV_ROUND_CLOSEST(
		sds->busiest_stat.avg_load * sds->busiest->sgc->capacity,
		SCHED_CAPACITY_SCALE);

	return 1;
}

/**
 * fix_small_imbalance - Calculate the minor imbalance that exists
 *			amongst the groups of a sched_domain, during
 *			load balancing.
 * @env: The load balancing environment.
 * @sds: Statistics of the sched_domain whose imbalance is to be calculated.
 */
static inline
void fix_small_imbalance(struct lb_env *env, struct sd_lb_stats *sds)
{
	unsigned long tmp, capa_now = 0, capa_move = 0;
	unsigned int imbn = 2;
	unsigned long scaled_busy_load_per_task;
	struct sg_lb_stats *local, *busiest;

	local = &sds->local_stat;
	busiest = &sds->busiest_stat;

	if (!local->sum_nr_running)
		local->load_per_task = cpu_avg_load_per_task(env->dst_cpu);
	else if (busiest->load_per_task > local->load_per_task)
		imbn = 1;

	scaled_busy_load_per_task =
		(busiest->load_per_task * SCHED_CAPACITY_SCALE) /
		sds->busiest->sgc->capacity;

	if (busiest->avg_load + scaled_busy_load_per_task >=
	    local->avg_load + (scaled_busy_load_per_task * imbn)) {
		env->imbalance = busiest->load_per_task;
		return;
	}

	/*
	 * OK, we don't have enough imbalance to justify moving tasks,
	 * however we may be able to increase total CPU capacity used by
	 * moving them.
	 */

	capa_now += sds->busiest->sgc->capacity *
			min(busiest->load_per_task, busiest->avg_load);
	capa_now += sds->local->sgc->capacity *
			min(local->load_per_task, local->avg_load);
	capa_now /= SCHED_CAPACITY_SCALE;

	/* Amount of load we'd subtract */
	if (busiest->avg_load > scaled_busy_load_per_task) {
		capa_move += sds->busiest->sgc->capacity *
			    min(busiest->load_per_task,
				busiest->avg_load - scaled_busy_load_per_task);
	}

	/* Amount of load we'd add */
	if (busiest->avg_load * sds->busiest->sgc->capacity <
	    busiest->load_per_task * SCHED_CAPACITY_SCALE) {
		tmp = (busiest->avg_load * sds->busiest->sgc->capacity) /
			sds->local->sgc->capacity;
	} else {
		tmp = (busiest->load_per_task * SCHED_CAPACITY_SCALE) /
			sds->local->sgc->capacity;
	}
	capa_move += sds->local->sgc->capacity *
		min(local->load_per_task, local->avg_load + tmp);
	capa_move /= SCHED_CAPACITY_SCALE;

	/* Move if we gain throughput */
	if (capa_move > capa_now)
		env->imbalance = busiest->load_per_task;
}

/**
 * calculate_imbalance - Calculate the amount of imbalance present within the
 *			 groups of a given sched_domain during load balance.
 * @env: load balance environment
 * @sds: statistics of the sched_domain whose imbalance is to be calculated.
 */
static inline void calculate_imbalance(struct lb_env *env, struct sd_lb_stats *sds)
{
	unsigned long max_pull, load_above_capacity = ~0UL;
	struct sg_lb_stats *local, *busiest;

	local = &sds->local_stat;
	if (local->sum_nr_running) {
		local->load_per_task =
			local->sum_weighted_load / local->sum_nr_running;
	}

	busiest = &sds->busiest_stat;
	/* busiest must have some tasks */
	busiest->load_per_task =
		busiest->sum_weighted_load / busiest->sum_nr_running;

	if (busiest->group_type == group_imbalanced) {
		busiest->load_per_task =
			min(busiest->load_per_task, sds->avg_load);
	}

	/*
	 * Avg load of busiest sg can be less and avg load of local sg can
	 * be greater than avg load across all sgs of sd because avg load
	 * factors in sg capacity and sgs with smaller group_type are
	 * skipped when updating the busiest sg:
	 */
	if (busiest->avg_load <= sds->avg_load ||
	    local->avg_load >= sds->avg_load) {
		/* Misfitting tasks should be migrated in any case */
		if (busiest->group_type == group_misfit_task) {
			env->imbalance = busiest->group_misfit_task;
			return;
		}

		/*
		 * Busiest group is overloaded, local is not, use the spare
		 * cycles to maximize throughput
		 */
		if (busiest->group_type == group_overloaded &&
		    local->group_type <= group_misfit_task) {
			env->imbalance = busiest->load_per_task;
			return;
		}

		env->imbalance = 0;
		return fix_small_imbalance(env, sds);
	}
	/*
	 * If there aren't any idle cpus, avoid creating some.
	 */
	if (busiest->group_type == group_overloaded &&
	    local->group_type   == group_overloaded) {
		load_above_capacity = busiest->sum_nr_running * SCHED_CAPACITY_SCALE;
		if (load_above_capacity > busiest->group_capacity) {
			load_above_capacity -= busiest->group_capacity;
			load_above_capacity *= scale_load_down(NICE_0_LOAD);
			load_above_capacity /= busiest->group_capacity;
		} else
			load_above_capacity = ~0UL;
	}

	/*
	 * We're trying to get all the cpus to the average_load, so we don't
	 * want to push ourselves above the average load, nor do we wish to
	 * reduce the max loaded cpu below the average load. At the same time,
	 * we also don't want to reduce the group load below the group
	 * capacity. Thus we look for the minimum possible imbalance.
	 */
	max_pull = min(busiest->avg_load - sds->avg_load, load_above_capacity);

	/* How much load to actually move to equalise the imbalance */
	env->imbalance = min(
		max_pull * sds->busiest->sgc->capacity,
		(sds->avg_load - local->avg_load) * sds->local->sgc->capacity
	) / SCHED_CAPACITY_SCALE;

	/* Boost imbalance to allow misfit task to be balanced. */
	if (busiest->group_type == group_misfit_task)
		env->imbalance = max_t(long, env->imbalance,
				     busiest->group_misfit_task);

	/*
	 * if *imbalance is less than the average load per runnable task
	 * there is no guarantee that any tasks will be moved so we'll have
	 * a think about bumping its value to force at least one task to be
	 * moved
	 */
	if (env->imbalance < busiest->load_per_task)
		return fix_small_imbalance(env, sds);
}

/******* find_busiest_group() helpers end here *********************/

/**
 * find_busiest_group - Returns the busiest group within the sched_domain
 * if there is an imbalance.
 *
 * Also calculates the amount of weighted load which should be moved
 * to restore balance.
 *
 * @env: The load balancing environment.
 *
 * Return:	- The busiest group if imbalance exists.
 */
static struct sched_group *find_busiest_group(struct lb_env *env)
{
	struct sg_lb_stats *local, *busiest;
	struct sd_lb_stats sds;

	init_sd_lb_stats(&sds);

	/*
	 * Compute the various statistics relavent for load balancing at
	 * this level.
	 */
	update_sd_lb_stats(env, &sds);

	if (energy_aware() && !env->dst_rq->rd->overutilized)
		goto out_balanced;

	local = &sds.local_stat;
	busiest = &sds.busiest_stat;

	/* ASYM feature bypasses nice load balance check */
	if (check_asym_packing(env, &sds))
		return sds.busiest;

	/* There is no busy sibling group to pull tasks from */
	if (!sds.busiest || busiest->sum_nr_running == 0)
		goto out_balanced;

	sds.avg_load = (SCHED_CAPACITY_SCALE * sds.total_load)
						/ sds.total_capacity;

	/*
	 * If the busiest group is imbalanced the below checks don't
	 * work because they assumes all things are equal, which typically
	 * isn't true due to cpus_allowed constraints and the like.
	 */
	if (busiest->group_type == group_imbalanced)
		goto force_balance;

	/* SD_BALANCE_NEWIDLE trumps SMP nice when underutilized */
	if (env->idle == CPU_NEWLY_IDLE && group_has_capacity(env, local) &&
	    busiest->group_no_capacity)
		goto force_balance;

	/* Misfitting tasks should be dealt with regardless of the avg load */
	if (busiest->group_type == group_misfit_task) {
		goto force_balance;
	}

	/*
	 * If the local group is busier than the selected busiest group
	 * don't try and pull any tasks.
	 */
	if (local->avg_load >= busiest->avg_load)
		goto out_balanced;

	/*
	 * Don't pull any tasks if this group is already above the domain
	 * average load.
	 */
	if (local->avg_load >= sds.avg_load)
		goto out_balanced;

	if (env->idle == CPU_IDLE) {
		/*
		 * This cpu is idle. If the busiest group is not overloaded
		 * and there is no imbalance between this and busiest group
		 * wrt idle cpus, it is balanced. The imbalance becomes
		 * significant if the diff is greater than 1 otherwise we
		 * might end up to just move the imbalance on another group
		 */
		if ((busiest->group_type != group_overloaded) &&
		    (local->idle_cpus <= (busiest->idle_cpus + 1)) &&
		    !group_smaller_cpu_capacity(sds.busiest, sds.local))
			goto out_balanced;
	} else {
		/*
		 * In the CPU_NEWLY_IDLE, CPU_NOT_IDLE cases, use
		 * imbalance_pct to be conservative.
		 */
		if (100 * busiest->avg_load <=
				env->sd->imbalance_pct * local->avg_load)
			goto out_balanced;
	}

force_balance:
	env->busiest_group_type = busiest->group_type;
	/* Looks like there is an imbalance. Compute it */
	calculate_imbalance(env, &sds);
	return sds.busiest;

out_balanced:
	env->imbalance = 0;
	return NULL;
}

/*
 * find_busiest_queue - find the busiest runqueue among the cpus in group.
 */
static struct rq *find_busiest_queue(struct lb_env *env,
				     struct sched_group *group)
{
	struct rq *busiest = NULL, *rq;
	unsigned long busiest_load = 0, busiest_capacity = 1;
	int i;

	for_each_cpu_and(i, sched_group_cpus(group), env->cpus) {
		unsigned long capacity, wl;

		rq = cpu_rq(i);

		capacity = capacity_of(i);

		wl = weighted_cpuload(i);

		/*
		 * When comparing with imbalance, use weighted_cpuload()
		 * which is not scaled with the cpu capacity.
		 */
		if (rq->nr_running == 1 && wl > env->imbalance &&
		    !check_cpu_capacity(rq, env->sd) &&
		    env->busiest_group_type != group_misfit_task)
			continue;

		/*
		 * For the load comparisons with the other cpu's, consider
		 * the weighted_cpuload() scaled with the cpu capacity, so
		 * that the load can be moved away from the cpu that is
		 * potentially running at a lower capacity.
		 *
		 * Thus we're looking for max(wl_i / capacity_i), crosswise
		 * multiplication to rid ourselves of the division works out
		 * to: wl_i * capacity_j > wl_j * capacity_i;  where j is
		 * our previous maximum.
		 */
		if (wl * busiest_capacity > busiest_load * capacity) {
			busiest_load = wl;
			busiest_capacity = capacity;
			busiest = rq;
		}
	}

	return busiest;
}

/*
 * Max backoff if we encounter pinned tasks. Pretty arbitrary value, but
 * so long as it is large enough.
 */
#define MAX_PINNED_INTERVAL	512

/* Working cpumask for load_balance and load_balance_newidle. */
DEFINE_PER_CPU(cpumask_var_t, load_balance_mask);

static int need_active_balance(struct lb_env *env)
{
	struct sched_domain *sd = env->sd;

	if (env->idle == CPU_NEWLY_IDLE) {

		/*
		 * ASYM_PACKING needs to force migrate tasks from busy but
		 * higher numbered CPUs in order to pack all tasks in the
		 * lowest numbered CPUs.
		 */
		if ((sd->flags & SD_ASYM_PACKING) && env->src_cpu > env->dst_cpu)
			return 1;
	}

	/*
	 * The dst_cpu is idle and the src_cpu CPU has only 1 CFS task.
	 * It's worth migrating the task if the src_cpu's capacity is reduced
	 * because of other sched_class or IRQs if more capacity stays
	 * available on dst_cpu.
	 */
	if ((env->idle != CPU_NOT_IDLE) &&
	    (env->src_rq->cfs.h_nr_running == 1)) {
		if ((check_cpu_capacity(env->src_rq, sd)) &&
		    (capacity_of(env->src_cpu)*sd->imbalance_pct < capacity_of(env->dst_cpu)*100))
			return 1;
	}

	if ((capacity_of(env->src_cpu) < capacity_of(env->dst_cpu)) &&
				env->src_rq->cfs.h_nr_running == 1 &&
				cpu_overutilized(env->src_cpu) &&
				!cpu_overutilized(env->dst_cpu)) {
			return 1;
	}

	return unlikely(sd->nr_balance_failed > sd->cache_nice_tries+2);
}

static int active_load_balance_cpu_stop(void *data);

static int should_we_balance(struct lb_env *env)
{
	struct sched_group *sg = env->sd->groups;
	struct cpumask *sg_cpus, *sg_mask;
	int cpu, balance_cpu = -1;

	/*
	 * In the newly idle case, we will allow all the cpu's
	 * to do the newly idle load balance.
	 */
	if (env->idle == CPU_NEWLY_IDLE)
		return 1;

	sg_cpus = sched_group_cpus(sg);
	sg_mask = sched_group_mask(sg);
	/* Try to find first idle cpu */
	for_each_cpu_and(cpu, sg_cpus, env->cpus) {
		if (!cpumask_test_cpu(cpu, sg_mask) || !idle_cpu(cpu))
			continue;

		balance_cpu = cpu;
		break;
	}

	if (balance_cpu == -1)
		balance_cpu = group_balance_cpu(sg);

	/*
	 * First idle cpu or the first cpu(busiest) in this sched group
	 * is eligible for doing load balancing at this and above domains.
	 */
	return balance_cpu == env->dst_cpu;
}

/*
 * Check this_cpu to ensure it is balanced within domain. Attempt to move
 * tasks if there is an imbalance.
 */
static int load_balance(int this_cpu, struct rq *this_rq,
			struct sched_domain *sd, enum cpu_idle_type idle,
			int *continue_balancing)
{
	int ld_moved, cur_ld_moved, active_balance = 0;
	struct sched_domain *sd_parent = sd->parent;
	struct sched_group *group;
	struct rq *busiest = NULL;
	unsigned long flags;
	struct cpumask *cpus = this_cpu_cpumask_var_ptr(load_balance_mask);

	struct lb_env env = {
		.sd		= sd,
		.dst_cpu	= this_cpu,
		.dst_rq		= this_rq,
		.dst_grpmask    = sched_group_cpus(sd->groups),
		.idle		= idle,
		.loop_break	= sched_nr_migrate_break,
		.cpus		= cpus,
		.tasks		= LIST_HEAD_INIT(env.tasks),
	};

	/*
	 * For NEWLY_IDLE load_balancing, we don't need to consider
	 * other cpus in our group
	 */
	if (idle == CPU_NEWLY_IDLE)
		env.dst_grpmask = NULL;

	cpumask_copy(cpus, cpu_active_mask);

	per_cpu(dbs_boost_load_moved, this_cpu) = 0;
	schedstat_inc(sd, lb_count[idle]);

redo:
	if (!should_we_balance(&env)) {
		*continue_balancing = 0;
		goto out_balanced;
	}

	group = find_busiest_group(&env);
	if (!group) {
		schedstat_inc(sd, lb_nobusyg[idle]);
		goto out_balanced;
	}

	busiest = find_busiest_queue(&env, group);
	if (!busiest) {
		schedstat_inc(sd, lb_nobusyq[idle]);
		goto out_balanced;
	}

	BUG_ON(busiest == env.dst_rq);

	schedstat_add(sd, lb_imbalance[idle], env.imbalance);

	env.src_cpu = busiest->cpu;
	env.src_rq = busiest;

	ld_moved = 0;
	/* load balance only apply to CFS task, we use h_nr_running here */
	if (busiest->cfs.h_nr_running > 1) {
		/*
		 * Attempt to move tasks. If find_busiest_group has found
		 * an imbalance but busiest->cfs.h_nr_running <= 1, the group is
		 * still unbalanced. ld_moved simply stays zero, so it is
		 * correctly treated as an imbalance.
		 */
		env.flags |= LBF_ALL_PINNED;
		env.loop_max  = min(sysctl_sched_nr_migrate,
					busiest->cfs.h_nr_running);

more_balance:
		raw_spin_lock_irqsave(&busiest->lock, flags);

		/*
		 * cur_ld_moved - load moved in current iteration
		 * ld_moved     - cumulative load moved across iterations
		 */
		cur_ld_moved = detach_tasks(&env);
		/*
		 * We want to potentially lower env.src_cpu's OPP.
		 */
		if (cur_ld_moved)
			update_capacity_of(env.src_cpu);

		/*
		 * We've detached some tasks from busiest_rq. Every
		 * task is masked "TASK_ON_RQ_MIGRATING", so we can safely
		 * unlock busiest->lock, and we are able to be sure
		 * that nobody can manipulate the tasks in parallel.
		 * See task_rq_lock() family for the details.
		 */

		raw_spin_unlock(&busiest->lock);

		if (cur_ld_moved) {
			attach_tasks(&env);
			ld_moved += cur_ld_moved;
		}

		local_irq_restore(flags);

		if (env.flags & LBF_NEED_BREAK) {
			env.flags &= ~LBF_NEED_BREAK;
			goto more_balance;
		}

		/*
		 * Revisit (affine) tasks on src_cpu that couldn't be moved to
		 * us and move them to an alternate dst_cpu in our sched_group
		 * where they can run. The upper limit on how many times we
		 * iterate on same src_cpu is dependent on number of cpus in our
		 * sched_group.
		 *
		 * This changes load balance semantics a bit on who can move
		 * load to a given_cpu. In addition to the given_cpu itself
		 * (or a ilb_cpu acting on its behalf where given_cpu is
		 * nohz-idle), we now have balance_cpu in a position to move
		 * load to given_cpu. In rare situations, this may cause
		 * conflicts (balance_cpu and given_cpu/ilb_cpu deciding
		 * _independently_ and at _same_ time to move some load to
		 * given_cpu) causing exceess load to be moved to given_cpu.
		 * This however should not happen so much in practice and
		 * moreover subsequent load balance cycles should correct the
		 * excess load moved.
		 */
		if ((env.flags & LBF_DST_PINNED) && env.imbalance > 0) {

			/* Prevent to re-select dst_cpu via env's cpus */
			cpumask_clear_cpu(env.dst_cpu, env.cpus);

			env.dst_rq	 = cpu_rq(env.new_dst_cpu);
			env.dst_cpu	 = env.new_dst_cpu;
			env.flags	&= ~LBF_DST_PINNED;
			env.loop	 = 0;
			env.loop_break	 = sched_nr_migrate_break;

			/*
			 * Go back to "more_balance" rather than "redo" since we
			 * need to continue with same src_cpu.
			 */
			goto more_balance;
		}

		/*
		 * We failed to reach balance because of affinity.
		 */
		if (sd_parent) {
			int *group_imbalance = &sd_parent->groups->sgc->imbalance;

			if ((env.flags & LBF_SOME_PINNED) && env.imbalance > 0)
				*group_imbalance = 1;
		}

		/* All tasks on this runqueue were pinned by CPU affinity */
		if (unlikely(env.flags & LBF_ALL_PINNED)) {
			cpumask_clear_cpu(cpu_of(busiest), cpus);
			if (!cpumask_empty(cpus)) {
				env.loop = 0;
				env.loop_break = sched_nr_migrate_break;
				goto redo;
			}
			goto out_all_pinned;
		}
	}

	if (!ld_moved) {
		schedstat_inc(sd, lb_failed[idle]);
		/*
		 * Increment the failure counter only on periodic balance.
		 * We do not want newidle balance, which can be very
		 * frequent, pollute the failure counter causing
		 * excessive cache_hot migrations and active balances.
		 */
		if (idle != CPU_NEWLY_IDLE)
			if (env.src_grp_nr_running > 1)
				sd->nr_balance_failed++;

		if (need_active_balance(&env)) {
			raw_spin_lock_irqsave(&busiest->lock, flags);

			/* don't kick the active_load_balance_cpu_stop,
			 * if the curr task on busiest cpu can't be
			 * moved to this_cpu
			 */
			if (!cpumask_test_cpu(this_cpu,
					tsk_cpus_allowed(busiest->curr))) {
				raw_spin_unlock_irqrestore(&busiest->lock,
							    flags);
				env.flags |= LBF_ALL_PINNED;
				goto out_one_pinned;
			}

			/*
			 * ->active_balance synchronizes accesses to
			 * ->active_balance_work.  Once set, it's cleared
			 * only after active load balance is finished.
			 */
			if (!busiest->active_balance) {
				busiest->active_balance = 1;
				busiest->push_cpu = this_cpu;
				active_balance = 1;
			}
			raw_spin_unlock_irqrestore(&busiest->lock, flags);

			if (active_balance) {
				stop_one_cpu_nowait(cpu_of(busiest),
					active_load_balance_cpu_stop, busiest,
					&busiest->active_balance_work);
			}

			/* We've kicked active balancing, force task migration. */
			sd->nr_balance_failed = sd->cache_nice_tries+1;
		}
	} else
		sd->nr_balance_failed = 0;

	if (likely(!active_balance)) {
		/* We were unbalanced, so reset the balancing interval */
		sd->balance_interval = sd->min_interval;
	} else {
		/*
		 * If we've begun active balancing, start to back off. This
		 * case may not be covered by the all_pinned logic if there
		 * is only 1 task on the busy runqueue (because we don't call
		 * detach_tasks).
		 */
		if (sd->balance_interval < sd->max_interval)
			sd->balance_interval *= 2;
	}

	goto out;

out_balanced:
	/*
	 * We reach balance although we may have faced some affinity
	 * constraints. Clear the imbalance flag if it was set.
	 */
	if (sd_parent) {
		int *group_imbalance = &sd_parent->groups->sgc->imbalance;

		if (*group_imbalance)
			*group_imbalance = 0;
	}

out_all_pinned:
	/*
	 * We reach balance because all tasks are pinned at this level so
	 * we can't migrate them. Let the imbalance flag set so parent level
	 * can try to migrate them.
	 */
	schedstat_inc(sd, lb_balanced[idle]);

	sd->nr_balance_failed = 0;

out_one_pinned:
	/* tune up the balancing interval */
	if (((env.flags & LBF_ALL_PINNED) &&
			sd->balance_interval < MAX_PINNED_INTERVAL) ||
			(sd->balance_interval < sd->max_interval))
		sd->balance_interval *= 2;

	ld_moved = 0;
out:
	return ld_moved;
}

static inline unsigned long
get_sd_balance_interval(struct sched_domain *sd, int cpu_busy)
{
	unsigned long interval = sd->balance_interval;

	if (cpu_busy)
		interval *= sd->busy_factor;

	/* scale ms to jiffies */
	interval = msecs_to_jiffies(interval);
	interval = clamp(interval, 1UL, max_load_balance_interval);

	return interval;
}

static inline void
update_next_balance(struct sched_domain *sd, unsigned long *next_balance)
{
	unsigned long interval, next;

	/* used by idle balance, so cpu_busy = 0 */
	interval = get_sd_balance_interval(sd, 0);
	next = sd->last_balance + interval;

	if (time_after(*next_balance, next))
		*next_balance = next;
}

/*
 * idle_balance is called by schedule() if this_cpu is about to become
 * idle. Attempts to pull tasks from other CPUs.
 */
static int idle_balance(struct rq *this_rq)
{
	unsigned long next_balance = jiffies + HZ;
	int this_cpu = this_rq->cpu;
	struct sched_domain *sd;
	int pulled_task = 0;
	u64 curr_cost = 0;
	long removed_util=0;

	/*
	 * We must set idle_stamp _before_ calling idle_balance(), such that we
	 * measure the duration of idle_balance() as idle time.
	 */
	this_rq->idle_stamp = rq_clock(this_rq);

	if (!energy_aware() &&
	    (this_rq->avg_idle < sysctl_sched_migration_cost ||
	     !this_rq->rd->overload)) {
		rcu_read_lock();
		sd = rcu_dereference(per_cpu(sd_llc, this_cpu));
		if (sd)
			update_next_balance(sd, &next_balance);
		rcu_read_unlock();

		goto out;
	}

	raw_spin_unlock(&this_rq->lock);

	/*
	 * If removed_util_avg is !0 we most probably migrated some task away
	 * from this_cpu. In this case we might be willing to trigger an OPP
	 * update, but we want to do so if we don't find anybody else to pull
	 * here (we will trigger an OPP update with the pulled task's enqueue
	 * anyway).
	 *
	 * Record removed_util before calling update_blocked_averages, and use
	 * it below (before returning) to see if an OPP update is required.
	 */
	removed_util = atomic_long_read(&(this_rq->cfs).removed_util_avg);
	update_blocked_averages(this_cpu);
	rcu_read_lock();
	for_each_domain(this_cpu, sd) {
		int continue_balancing = 1;
		u64 t0, domain_cost;

		if (!(sd->flags & SD_LOAD_BALANCE))
			continue;

		if (this_rq->avg_idle < curr_cost + sd->max_newidle_lb_cost) {
			update_next_balance(sd, &next_balance);
			break;
		}

		if (sd->flags & SD_BALANCE_NEWIDLE) {
			t0 = sched_clock_cpu(this_cpu);

			pulled_task = load_balance(this_cpu, this_rq,
						   sd, CPU_NEWLY_IDLE,
						   &continue_balancing);

			domain_cost = sched_clock_cpu(this_cpu) - t0;
			if (domain_cost > sd->max_newidle_lb_cost)
				sd->max_newidle_lb_cost = domain_cost;

			curr_cost += domain_cost;
		}

		update_next_balance(sd, &next_balance);

		/*
		 * Stop searching for tasks to pull if there are
		 * now runnable tasks on this rq.
		 */
		if (pulled_task || this_rq->nr_running > 0)
			break;
	}
	rcu_read_unlock();

	raw_spin_lock(&this_rq->lock);

	if (curr_cost > this_rq->max_idle_balance_cost)
		this_rq->max_idle_balance_cost = curr_cost;

	/*
	 * While browsing the domains, we released the rq lock, a task could
	 * have been enqueued in the meantime. Since we're not going idle,
	 * pretend we pulled a task.
	 */
	if (this_rq->cfs.h_nr_running && !pulled_task)
		pulled_task = 1;

out:
	/* Move the next balance forward */
	if (time_after(this_rq->next_balance, next_balance))
		this_rq->next_balance = next_balance;

	/* Is there a task of a high priority class? */
	if (this_rq->nr_running != this_rq->cfs.h_nr_running)
		pulled_task = -1;

	if (pulled_task) {
		this_rq->idle_stamp = 0;
	} else if (removed_util) {
		/*
		 * No task pulled and someone has been migrated away.
		 * Good case to trigger an OPP update.
		 */
		update_capacity_of(this_cpu);
	}

	return pulled_task;
}

/*
 * active_load_balance_cpu_stop is run by cpu stopper. It pushes
 * running tasks off the busiest CPU onto idle CPUs. It requires at
 * least 1 task to be running on each physical CPU where possible, and
 * avoids physical / logical imbalances.
 */
static int active_load_balance_cpu_stop(void *data)
{
	struct rq *busiest_rq = data;
	int busiest_cpu = cpu_of(busiest_rq);
	int target_cpu = busiest_rq->push_cpu;
	struct rq *target_rq = cpu_rq(target_cpu);
	struct sched_domain *sd;
	struct task_struct *p = NULL;

	raw_spin_lock_irq(&busiest_rq->lock);

	per_cpu(dbs_boost_load_moved, target_cpu) = 0;

	/* make sure the requested cpu hasn't gone down in the meantime */
	if (unlikely(busiest_cpu != smp_processor_id() ||
		     !busiest_rq->active_balance))
		goto out_unlock;

	/* Is there any task to move? */
	if (busiest_rq->cfs.h_nr_running <= 1)
		goto out_unlock;

	/*
	 * This condition is "impossible", if it occurs
	 * we need to fix it. Originally reported by
	 * Bjorn Helgaas on a 128-cpu setup.
	 */
	BUG_ON(busiest_rq == target_rq);

	/* Search for an sd spanning us and the target CPU. */
	rcu_read_lock();
	for_each_domain(target_cpu, sd) {
		if ((sd->flags & SD_LOAD_BALANCE) &&
		    cpumask_test_cpu(busiest_cpu, sched_domain_span(sd)))
				break;
	}

	if (likely(sd)) {
		struct lb_env env = {
			.sd		= sd,
			.dst_cpu	= target_cpu,
			.dst_rq		= target_rq,
			.src_cpu	= busiest_rq->cpu,
			.src_rq		= busiest_rq,
			.idle		= CPU_IDLE,
		};

		schedstat_inc(sd, alb_count);

		p = detach_one_task(&env);
		if (p) {
			schedstat_inc(sd, alb_pushed);
			/*
			 * We want to potentially lower env.src_cpu's OPP.
			 */
			update_capacity_of(env.src_cpu);
			/* Active balancing done, reset the failure counter. */
			sd->nr_balance_failed = 0;
		} else {
			schedstat_inc(sd, alb_failed);
		}
	}
	rcu_read_unlock();
out_unlock:
	busiest_rq->active_balance = 0;
	raw_spin_unlock(&busiest_rq->lock);

	if (p)
		attach_one_task(target_rq, p);

	local_irq_enable();

	return 0;
}

static inline int on_null_domain(struct rq *rq)
{
	return unlikely(!rcu_dereference_sched(rq->sd));
}

#ifdef CONFIG_NO_HZ_COMMON
/*
 * idle load balancing details
 * - When one of the busy CPUs notice that there may be an idle rebalancing
 *   needed, they will kick the idle load balancer, which then does idle
 *   load balancing for all the idle CPUs.
 */
static struct {
	cpumask_var_t idle_cpus_mask;
	atomic_t nr_cpus;
	unsigned long next_balance;     /* in jiffy units */
} nohz ____cacheline_aligned;

static inline int find_new_ilb(void)
{
	int ilb = cpumask_first(nohz.idle_cpus_mask);

	if (ilb < nr_cpu_ids && idle_cpu(ilb))
		return ilb;

	return nr_cpu_ids;
}

/*
 * Kick a CPU to do the nohz balancing, if it is time for it. We pick the
 * nohz_load_balancer CPU (if there is one) otherwise fallback to any idle
 * CPU (if there is one).
 */
static void nohz_balancer_kick(void)
{
	int ilb_cpu;

	nohz.next_balance++;

	ilb_cpu = find_new_ilb();

	if (ilb_cpu >= nr_cpu_ids)
		return;

	if (test_and_set_bit(NOHZ_BALANCE_KICK, nohz_flags(ilb_cpu)))
		return;
	/*
	 * Use smp_send_reschedule() instead of resched_cpu().
	 * This way we generate a sched IPI on the target cpu which
	 * is idle. And the softirq performing nohz idle load balance
	 * will be run before returning from the IPI.
	 */
	smp_send_reschedule(ilb_cpu);
	return;
}

void nohz_balance_exit_idle(unsigned int cpu)
{
	if (unlikely(test_bit(NOHZ_TICK_STOPPED, nohz_flags(cpu)))) {
		/*
		 * Completely isolated CPUs don't ever set, so we must test.
		 */
		if (likely(cpumask_test_cpu(cpu, nohz.idle_cpus_mask))) {
			cpumask_clear_cpu(cpu, nohz.idle_cpus_mask);
			atomic_dec(&nohz.nr_cpus);
		}
		clear_bit(NOHZ_TICK_STOPPED, nohz_flags(cpu));
	}
}

static inline void set_cpu_sd_state_busy(void)
{
	struct sched_domain *sd;
	int cpu = smp_processor_id();

	rcu_read_lock();
	sd = rcu_dereference_check_sched_domain(this_rq()->sd);

	if (!sd || !sd->nohz_idle)
		goto unlock;
	sd->nohz_idle = 0;

	for (; sd; sd = sd->parent)
		atomic_inc(&sd->shared->nr_busy_cpus);
unlock:
	rcu_read_unlock();
}

void set_cpu_sd_state_idle(void)
{
	struct sched_domain *sd;
	int cpu = smp_processor_id();

	rcu_read_lock();
	sd = rcu_dereference(per_cpu(sd_llc, cpu));

	if (!sd || sd->nohz_idle)
		goto unlock;
	sd->nohz_idle = 1;

	atomic_dec(&sd->shared->nr_busy_cpus);
unlock:
	rcu_read_unlock();
}

/*
 * This routine will record that the cpu is going idle with tick stopped.
 * This info will be used in performing idle load balancing in the future.
 */
void nohz_balance_enter_idle(int cpu)
{
	/*
	 * If this cpu is going down, then nothing needs to be done.
	 */
	if (!cpu_active(cpu))
		return;

	if (test_bit(NOHZ_TICK_STOPPED, nohz_flags(cpu)))
		return;

	/*
	 * If we're a completely isolated CPU, we don't play.
	 */
	if (on_null_domain(cpu_rq(cpu)))
		return;

	cpumask_set_cpu(cpu, nohz.idle_cpus_mask);
	atomic_inc(&nohz.nr_cpus);
	set_bit(NOHZ_TICK_STOPPED, nohz_flags(cpu));
}
#endif

static DEFINE_SPINLOCK(balancing);

/*
 * Scale the max load_balance interval with the number of CPUs in the system.
 * This trades load-balance latency on larger machines for less cross talk.
 */
void update_max_interval(void)
{
	max_load_balance_interval = HZ*num_online_cpus()/10;
}

/*
 * It checks each scheduling domain to see if it is due to be balanced,
 * and initiates a balancing operation if so.
 *
 * Balancing parameters are set up in init_sched_domains.
 */
static void rebalance_domains(struct rq *rq, enum cpu_idle_type idle)
{
	int continue_balancing = 1;
	int cpu = rq->cpu;
	unsigned long interval;
	struct sched_domain *sd, *last = NULL;
	/* Earliest time when we have to do rebalance again */
	unsigned long next_balance = jiffies + 60*HZ;
	int update_next_balance = 0;
	int need_serialize, need_decay = 0;
	u64 max_cost = 0;

	update_blocked_averages(cpu);

	rcu_read_lock();
	for_each_domain(cpu, sd) {
		/*
		 * Decay the newidle max times here because this is a regular
		 * visit to all the domains. Decay ~1% per second.
		 */
		if (time_after(jiffies, sd->next_decay_max_lb_cost)) {
			sd->max_newidle_lb_cost =
				(sd->max_newidle_lb_cost * 253) / 256;
			sd->next_decay_max_lb_cost = jiffies + HZ;
			need_decay = 1;
		}
		max_cost += sd->max_newidle_lb_cost;
		last = sd;
		if (!(sd->flags & SD_LOAD_BALANCE))
			continue;

		/*
		 * Stop the load balance at this level. There is another
		 * * CPU in our sched group which is doing load balancing more
		 * actively.
		 */
		if (!continue_balancing) {
			if (need_decay)
				continue;
			break;
		}

		interval = get_sd_balance_interval(sd, idle != CPU_IDLE);

		need_serialize = sd->flags & SD_SERIALIZE;
		if (need_serialize) {
			if (!spin_trylock(&balancing))
				goto out;
		}

		if (time_after_eq(jiffies, sd->last_balance + interval)) {
			if (load_balance(cpu, rq, sd, idle, &continue_balancing)) {
				/*
				 * The LBF_DST_PINNED logic could have changed
				 * env->dst_cpu, so we can't know our idle
				 * state even if we migrated tasks. Update it.
				 */
				idle = idle_cpu(cpu) ? CPU_IDLE : CPU_NOT_IDLE;
			}
			sd->last_balance = jiffies;
			interval = get_sd_balance_interval(sd, idle != CPU_IDLE);
		}
		if (need_serialize)
			spin_unlock(&balancing);
out:
		if (time_after(next_balance, sd->last_balance + interval)) {
			next_balance = sd->last_balance + interval;
			update_next_balance = 1;
		}
	}
	if (need_decay) {
		/*
		 * Ensure the rq-wide value also decays but keep it at a
		 * reasonable floor to avoid funnies with rq->avg_idle.
		 */
		rq->max_idle_balance_cost =
			max((u64)sysctl_sched_migration_cost, max_cost);
	}
	rcu_read_unlock();

	/*
	 * next_balance will be updated only when there is a need.
	 * When the cpu is attached to null domain for ex, it will not be
	 * updated.
	 */
	if (likely(update_next_balance)) {
		rq->next_balance = next_balance;

#ifdef CONFIG_NO_HZ_COMMON
		/*
		 * If this CPU has been elected to perform the nohz idle
		 * balance. Other idle CPUs have already rebalanced with
		 * nohz_idle_balance() and nohz.next_balance has been
		 * updated accordingly. This CPU is now running the idle load
		 * balance for itself and we need to update the
		 * nohz.next_balance accordingly.
		 */
		if ((idle == CPU_IDLE) && time_after(nohz.next_balance, rq->next_balance))
			nohz.next_balance = rq->next_balance;
#endif
	}
}

#ifdef CONFIG_NO_HZ_COMMON
/*
 * In CONFIG_NO_HZ_COMMON case, the idle balance kickee will do the
 * rebalancing for all the cpus for whom scheduler ticks are stopped.
 */
static void nohz_idle_balance(struct rq *this_rq, enum cpu_idle_type idle)
{
	int this_cpu = this_rq->cpu;
	struct rq *rq;
	int balance_cpu;
	/* Earliest time when we have to do rebalance again */
	unsigned long next_balance = jiffies + 60*HZ;
	int update_next_balance = 0;

	if (idle != CPU_IDLE ||
	    !test_bit(NOHZ_BALANCE_KICK, nohz_flags(this_cpu)))
		goto end;

	for_each_cpu(balance_cpu, nohz.idle_cpus_mask) {
		if (balance_cpu == this_cpu || !idle_cpu(balance_cpu))
			continue;

		/*
		 * If this cpu gets work to do, stop the load balancing
		 * work being done for other cpus. Next load
		 * balancing owner will pick it up.
		 */
		if (need_resched())
			break;

		rq = cpu_rq(balance_cpu);

		/*
		 * If time for next balance is due,
		 * do the balance.
		 */
		if (time_after_eq(jiffies, rq->next_balance)) {
			raw_spin_lock_irq(&rq->lock);
			update_rq_clock(rq);
			update_cpu_load_idle(rq);
			raw_spin_unlock_irq(&rq->lock);
			rebalance_domains(rq, CPU_IDLE);
		}

		if (time_after(next_balance, rq->next_balance)) {
			next_balance = rq->next_balance;
			update_next_balance = 1;
		}
	}

	/*
	 * next_balance will be updated only when there is a need.
	 * When the CPU is attached to null domain for ex, it will not be
	 * updated.
	 */
	if (likely(update_next_balance))
		nohz.next_balance = next_balance;
end:
	clear_bit(NOHZ_BALANCE_KICK, nohz_flags(this_cpu));
}

/*
 * Current heuristic for kicking the idle load balancer in the presence
 * of an idle cpu in the system.
 *   - This rq has more than one task.
 *   - This rq has at least one CFS task and the capacity of the CPU is
 *     significantly reduced because of RT tasks or IRQs.
 *   - At parent of LLC scheduler domain level, this cpu's scheduler group has
 *     multiple busy cpu.
 *   - For SD_ASYM_PACKING, if the lower numbered cpu's in the scheduler
 *     domain span are idle.
 */
static inline bool nohz_kick_needed(struct rq *rq)
{
	unsigned long now = jiffies;
	struct sched_domain_shared *sds;
	struct sched_domain *sd;
	int nr_busy, cpu = rq->cpu;
	bool kick = false;

	if (unlikely(rq->idle_balance))
		return false;

       /*
	* We may be recently in ticked or tickless idle mode. At the first
	* busy tick after returning from idle, we will update the busy stats.
	*/
	set_cpu_sd_state_busy();
	nohz_balance_exit_idle(cpu);

	/*
	 * None are in tickless mode and hence no need for NOHZ idle load
	 * balancing.
	 */
	if (likely(!atomic_read(&nohz.nr_cpus)))
		return false;

	if (time_before(now, nohz.next_balance))
		return false;

	if (rq->nr_running >= 2 &&
	    (!energy_aware() || cpu_overutilized(cpu)))
		return true;

	rcu_read_lock();
	sds = rcu_dereference(per_cpu(sd_llc_shared, cpu));
	if (sds && !energy_aware()) {
		/*
		 * XXX: write a coherent comment on why we do this.
		 * See also: http://lkml.kernel.org/r/20111202010832.602203411@sbsiddha-desk.sc.intel.com
		 */
		nr_busy = atomic_read(&sds->nr_busy_cpus);

		if (nr_busy > 1) {
			kick = true;
			goto unlock;
		}

	}

	sd = rcu_dereference(rq->sd);
	if (sd) {
		if ((rq->cfs.h_nr_running >= 1) &&
				check_cpu_capacity(rq, sd)) {
			kick = true;
			goto unlock;
		}
	}

	sd = rcu_dereference(per_cpu(sd_asym, cpu));
	if (sd && (cpumask_first_and(nohz.idle_cpus_mask,
				  sched_domain_span(sd)) < cpu)) {
		kick = true;
		goto unlock;
	}

unlock:
	rcu_read_unlock();
	return kick;
}
#else
static void nohz_idle_balance(struct rq *this_rq, enum cpu_idle_type idle) { }
#endif

/*
 * run_rebalance_domains is triggered when needed from the scheduler tick.
 * Also triggered for nohz idle balancing (with nohz_balancing_kick set).
 */
static void run_rebalance_domains(struct softirq_action *h)
{
	struct rq *this_rq = this_rq();
	enum cpu_idle_type idle = this_rq->idle_balance ?
						CPU_IDLE : CPU_NOT_IDLE;

	/*
	 * If this cpu has a pending nohz_balance_kick, then do the
	 * balancing on behalf of the other idle cpus whose ticks are
	 * stopped. Do nohz_idle_balance *before* rebalance_domains to
	 * give the idle cpus a chance to load balance. Else we may
	 * load balance only within the local sched_domain hierarchy
	 * and abort nohz_idle_balance altogether if we pull some load.
	 */
	nohz_idle_balance(this_rq, idle);
	rebalance_domains(this_rq, idle);
}

/*
 * Trigger the SCHED_SOFTIRQ if it is time to do periodic load balancing.
 */
void trigger_load_balance(struct rq *rq)
{
	/* Don't need to rebalance while attached to NULL domain */
	if (unlikely(on_null_domain(rq)))
		return;

	if (time_after_eq(jiffies, rq->next_balance))
		raise_softirq(SCHED_SOFTIRQ);
#ifdef CONFIG_NO_HZ_COMMON
	if (nohz_kick_needed(rq))
		nohz_balancer_kick();
#endif
}

static void rq_online_fair(struct rq *rq)
{
	update_sysctl();

	update_runtime_enabled(rq);
}

static void rq_offline_fair(struct rq *rq)
{
	update_sysctl();

	/* Ensure any throttled groups are reachable by pick_next_task */
	unthrottle_offline_cfs_rqs(rq);
}

#endif /* CONFIG_SMP */

/*
 * scheduler tick hitting a task of our scheduling class:
 */
static void task_tick_fair(struct rq *rq, struct task_struct *curr, int queued)
{
	struct cfs_rq *cfs_rq;
	struct sched_entity *se = &curr->se;

	for_each_sched_entity(se) {
		cfs_rq = cfs_rq_of(se);
		entity_tick(cfs_rq, se, queued);
	}

#ifdef CONFIG_SMP
	if (!rq->rd->overutilized && cpu_overutilized(task_cpu(curr)))
		rq->rd->overutilized = true;

	rq->misfit_task = !task_fits_max(curr, rq->cpu);
#endif

}

/*
 * called on fork with the child task as argument from the parent's context
 *  - child not yet on the tasklist
 *  - preemption disabled
 */
static void task_fork_fair(struct task_struct *p)
{
	struct cfs_rq *cfs_rq;
	struct sched_entity *se = &p->se, *curr;
	struct rq *rq = this_rq();

	raw_spin_lock(&rq->lock);
	update_rq_clock(rq);

	cfs_rq = task_cfs_rq(current);
	curr = cfs_rq->curr;
	if (curr) {
		update_curr(cfs_rq);
		se->vruntime = curr->vruntime;
	}
	place_entity(cfs_rq, se, 1);

	if (sysctl_sched_child_runs_first && curr && entity_before(curr, se)) {
		/*
		 * Upon rescheduling, sched_class::put_prev_task() will place
		 * 'current' within the tree based on its new key value.
		 */
		swap(curr->vruntime, se->vruntime);
		resched_curr(rq);
	}

	se->vruntime -= cfs_rq->min_vruntime;
	raw_spin_unlock(&rq->lock);
}

/*
 * Priority of the task has changed. Check to see if we preempt
 * the current task.
 */
static void
prio_changed_fair(struct rq *rq, struct task_struct *p, int oldprio)
{
	if (!task_on_rq_queued(p))
		return;

	/*
	 * Reschedule if we are currently running on this runqueue and
	 * our priority decreased, or if we are not currently running on
	 * this runqueue and our priority is higher than the current's
	 */
	if (rq->curr == p) {
		if (p->prio > oldprio)
			resched_curr(rq);
	} else
		check_preempt_curr(rq, p, 0);
}

static inline bool vruntime_normalized(struct task_struct *p)
{
	struct sched_entity *se = &p->se;

	/*
	 * In both the TASK_ON_RQ_QUEUED and TASK_ON_RQ_MIGRATING cases,
	 * the dequeue_entity(.flags=0) will already have normalized the
	 * vruntime.
	 */
	if (p->on_rq)
		return true;

	/*
	 * When !on_rq, vruntime of the task has usually NOT been normalized.
	 * But there are some cases where it has already been normalized:
	 *
	 * - A forked child which is waiting for being woken up by
	 *   wake_up_new_task().
	 * - A task which has been woken up by try_to_wake_up() and
	 *   waiting for actually being woken up by sched_ttwu_pending().
	 */
	if (!se->sum_exec_runtime || p->state == TASK_WAKING)
		return true;

	return false;
}

static void detach_task_cfs_rq(struct task_struct *p)
{
	struct sched_entity *se = &p->se;
	struct cfs_rq *cfs_rq = cfs_rq_of(se);
	u64 now = cfs_rq_clock_task(cfs_rq);

	if (!vruntime_normalized(p)) {
		/*
		 * Fix up our vruntime so that the current sleep doesn't
		 * cause 'unlimited' sleep bonus.
		 */
		place_entity(cfs_rq, se, 0);
		se->vruntime -= cfs_rq->min_vruntime;
	}

	/* Catch up with the cfs_rq and remove our load when we leave */
	update_cfs_rq_load_avg(now, cfs_rq);
	detach_entity_load_avg(cfs_rq, se);
	update_tg_load_avg(cfs_rq, false);
}

/*
 * We switched to the sched_fair class.
 */
static void attach_task_cfs_rq(struct task_struct *p)
{
	struct sched_entity *se = &p->se;
	struct cfs_rq *cfs_rq = cfs_rq_of(se);
	u64 now = cfs_rq_clock_task(cfs_rq);

#ifdef CONFIG_FAIR_GROUP_SCHED
	/*
	 * Since the real-depth could have been changed (only FAIR
	 * class maintain depth value), reset depth properly.
	 */
	se->depth = se->parent ? se->parent->depth + 1 : 0;
#endif

	/* Synchronize task with its cfs_rq */
	update_cfs_rq_load_avg(now, cfs_rq);
	attach_entity_load_avg(cfs_rq, se);
	update_tg_load_avg(cfs_rq, false);

	if (!vruntime_normalized(p))
		se->vruntime += cfs_rq->min_vruntime;
}

static void switched_from_fair(struct rq *rq, struct task_struct *p)
{
	detach_task_cfs_rq(p);
}

static void switched_to_fair(struct rq *rq, struct task_struct *p)
{
	attach_task_cfs_rq(p);

	if (task_on_rq_queued(p)) {
		/*
		 * We were most likely switched from sched_rt, so
		 * kick off the schedule if running, otherwise just see
		 * if we can still preempt the current task.
		 */
		if (rq->curr == p)
			resched_curr(rq);
		else
			check_preempt_curr(rq, p, 0);
	}
}

/* Account for a task changing its policy or group.
 *
 * This routine is mostly called to set cfs_rq->curr field when a task
 * migrates between groups/classes.
 */
static void set_curr_task_fair(struct rq *rq)
{
	struct sched_entity *se = &rq->curr->se;

	for_each_sched_entity(se) {
		struct cfs_rq *cfs_rq = cfs_rq_of(se);

		set_next_entity(cfs_rq, se);
		/* ensure bandwidth has been allocated on our new cfs_rq */
		account_cfs_rq_runtime(cfs_rq, 0);
	}
}

void init_cfs_rq(struct cfs_rq *cfs_rq)
{
	cfs_rq->tasks_timeline = RB_ROOT;
	cfs_rq->min_vruntime = (u64)(-(1LL << 20));
#ifndef CONFIG_64BIT
	cfs_rq->min_vruntime_copy = cfs_rq->min_vruntime;
#endif
#ifdef CONFIG_SMP
	atomic_long_set(&cfs_rq->removed_load_avg, 0);
	atomic_long_set(&cfs_rq->removed_util_avg, 0);
#endif
}

#ifdef CONFIG_FAIR_GROUP_SCHED
static void task_set_group_fair(struct task_struct *p)
{
	struct sched_entity *se = &p->se;

	set_task_rq(p, task_cpu(p));
	se->depth = se->parent ? se->parent->depth + 1 : 0;
}

static void task_move_group_fair(struct task_struct *p)
{
	detach_task_cfs_rq(p);
	set_task_rq(p, task_cpu(p));

#ifdef CONFIG_SMP
	/* Tell se's cfs_rq has been changed -- migrated */
	p->se.avg.last_update_time = 0;
#endif
	attach_task_cfs_rq(p);
}

static void task_change_group_fair(struct task_struct *p, int type)
{
	switch (type) {
	case TASK_SET_GROUP:
		task_set_group_fair(p);
		break;

	case TASK_MOVE_GROUP:
		task_move_group_fair(p);
		break;
	}
}

void free_fair_sched_group(struct task_group *tg)
{
	int i;

	destroy_cfs_bandwidth(tg_cfs_bandwidth(tg));

	for_each_possible_cpu(i) {
		if (tg->cfs_rq)
			kfree(tg->cfs_rq[i]);
		if (tg->se)
			kfree(tg->se[i]);
	}

	kfree(tg->cfs_rq);
	kfree(tg->se);
}

int alloc_fair_sched_group(struct task_group *tg, struct task_group *parent)
{
	struct sched_entity *se;
	struct cfs_rq *cfs_rq;
	int i;

	tg->cfs_rq = kzalloc(sizeof(cfs_rq) * nr_cpu_ids, GFP_KERNEL);
	if (!tg->cfs_rq)
		goto err;
	tg->se = kzalloc(sizeof(se) * nr_cpu_ids, GFP_KERNEL);
	if (!tg->se)
		goto err;

	tg->shares = NICE_0_LOAD;

	init_cfs_bandwidth(tg_cfs_bandwidth(tg));

	for_each_possible_cpu(i) {
		cfs_rq = kzalloc_node(sizeof(struct cfs_rq),
				      GFP_KERNEL, cpu_to_node(i));
		if (!cfs_rq)
			goto err;

		se = kzalloc_node(sizeof(struct sched_entity),
				  GFP_KERNEL, cpu_to_node(i));
		if (!se)
			goto err_free_rq;

		init_cfs_rq(cfs_rq);
		init_tg_cfs_entry(tg, cfs_rq, se, i, parent->se[i]);
		init_entity_runnable_average(se);
	}

	return 1;

err_free_rq:
	kfree(cfs_rq);
err:
	return 0;
}

void online_fair_sched_group(struct task_group *tg)
{
	struct sched_entity *se;
	struct rq *rq;
	int i;

	for_each_possible_cpu(i) {
		rq = cpu_rq(i);
		se = tg->se[i];

		raw_spin_lock_irq(&rq->lock);
		post_init_entity_util_avg(se);
		sync_throttle(tg, i);
		raw_spin_unlock_irq(&rq->lock);
	}
}

void unregister_fair_sched_group(struct task_group *tg)
{
	unsigned long flags;
	struct rq *rq;
	int cpu;

	for_each_possible_cpu(cpu) {
		if (tg->se[cpu])
			remove_entity_load_avg(tg->se[cpu]);

		/*
		 * Only empty task groups can be destroyed; so we can speculatively
		 * check on_list without danger of it being re-added.
		 */
		if (!tg->cfs_rq[cpu]->on_list)
			continue;

		rq = cpu_rq(cpu);

		raw_spin_lock_irqsave(&rq->lock, flags);
		list_del_leaf_cfs_rq(tg->cfs_rq[cpu]);
		raw_spin_unlock_irqrestore(&rq->lock, flags);
	}
}

void init_tg_cfs_entry(struct task_group *tg, struct cfs_rq *cfs_rq,
			struct sched_entity *se, int cpu,
			struct sched_entity *parent)
{
	struct rq *rq = cpu_rq(cpu);

	cfs_rq->tg = tg;
	cfs_rq->rq = rq;
	init_cfs_rq_runtime(cfs_rq);

	tg->cfs_rq[cpu] = cfs_rq;
	tg->se[cpu] = se;

	/* se could be NULL for root_task_group */
	if (!se)
		return;

	if (!parent) {
		se->cfs_rq = &rq->cfs;
		se->depth = 0;
	} else {
		se->cfs_rq = parent->my_q;
		se->depth = parent->depth + 1;
	}

	se->my_q = cfs_rq;
	/* guarantee group entities always have weight */
	update_load_set(&se->load, NICE_0_LOAD);
	se->parent = parent;
}

static DEFINE_MUTEX(shares_mutex);

int sched_group_set_shares(struct task_group *tg, unsigned long shares)
{
	int i;
	unsigned long flags;

	/*
	 * We can't change the weight of the root cgroup.
	 */
	if (!tg->se[0])
		return -EINVAL;

	shares = clamp(shares, scale_load(MIN_SHARES), scale_load(MAX_SHARES));

	mutex_lock(&shares_mutex);
	if (tg->shares == shares)
		goto done;

	tg->shares = shares;
	for_each_possible_cpu(i) {
		struct rq *rq = cpu_rq(i);
		struct sched_entity *se;

		se = tg->se[i];
		/* Propagate contribution to hierarchy */
		raw_spin_lock_irqsave(&rq->lock, flags);

		/* Possible calls to update_curr() need rq clock */
		update_rq_clock(rq);
		for_each_sched_entity(se)
			update_cfs_shares(group_cfs_rq(se));
		raw_spin_unlock_irqrestore(&rq->lock, flags);
	}

done:
	mutex_unlock(&shares_mutex);
	return 0;
}
#else /* CONFIG_FAIR_GROUP_SCHED */

void free_fair_sched_group(struct task_group *tg) { }

int alloc_fair_sched_group(struct task_group *tg, struct task_group *parent)
{
	return 1;
}

void online_fair_sched_group(struct task_group *tg) { }

void unregister_fair_sched_group(struct task_group *tg) { }

#endif /* CONFIG_FAIR_GROUP_SCHED */


static unsigned int get_rr_interval_fair(struct rq *rq, struct task_struct *task)
{
	struct sched_entity *se = &task->se;
	unsigned int rr_interval = 0;

	/*
	 * Time slice is 0 for SCHED_OTHER tasks that are on an otherwise
	 * idle runqueue:
	 */
	if (rq->cfs.load.weight)
		rr_interval = NS_TO_JIFFIES(sched_slice(cfs_rq_of(se), se));

	return rr_interval;
}

/*
 * All the scheduling class methods:
 */
const struct sched_class fair_sched_class = {
	.next			= &idle_sched_class,
	.enqueue_task		= enqueue_task_fair,
	.dequeue_task		= dequeue_task_fair,
	.yield_task		= yield_task_fair,
	.yield_to_task		= yield_to_task_fair,

	.check_preempt_curr	= check_preempt_wakeup,

	.pick_next_task		= pick_next_task_fair,
	.put_prev_task		= put_prev_task_fair,

#ifdef CONFIG_SMP
	.select_task_rq		= select_task_rq_fair,
	.migrate_task_rq	= migrate_task_rq_fair,

	.rq_online		= rq_online_fair,
	.rq_offline		= rq_offline_fair,

	.task_dead		= task_dead_fair,
	.set_cpus_allowed	= set_cpus_allowed_common,
#endif

	.set_curr_task          = set_curr_task_fair,
	.task_tick		= task_tick_fair,
	.task_fork		= task_fork_fair,

	.prio_changed		= prio_changed_fair,
	.switched_from		= switched_from_fair,
	.switched_to		= switched_to_fair,

	.get_rr_interval	= get_rr_interval_fair,

	.update_curr		= update_curr_fair,

#ifdef CONFIG_FAIR_GROUP_SCHED
	.task_change_group	= task_change_group_fair,
#endif
};

#ifdef CONFIG_SCHED_DEBUG
void print_cfs_stats(struct seq_file *m, int cpu)
{
	struct cfs_rq *cfs_rq;

	rcu_read_lock();
	for_each_leaf_cfs_rq(cpu_rq(cpu), cfs_rq)
		print_cfs_rq(m, cpu, cfs_rq);
	rcu_read_unlock();
}
#endif

__init void init_sched_fair_class(void)
{
#ifdef CONFIG_SMP
	open_softirq(SCHED_SOFTIRQ, run_rebalance_domains);

#ifdef CONFIG_NO_HZ_COMMON
	nohz.next_balance = jiffies;
	zalloc_cpumask_var(&nohz.idle_cpus_mask, GFP_NOWAIT);
#endif
#endif /* SMP */

}
