/*
 * Sleepable Read-Copy Update mechanism for mutual exclusion.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 *
 * Copyright (C) IBM Corporation, 2006
 * Copyright (C) Fujitsu, 2012
 *
 * Author: Paul McKenney <paulmck@us.ibm.com>
 *	   Lai Jiangshan <laijs@cn.fujitsu.com>
 *
 * For detailed explanation of Read-Copy Update mechanism see -
 *		Documentation/RCU/ *.txt
 *
 */

#include <linux/export.h>
#include <linux/mutex.h>
#include <linux/percpu.h>
#include <linux/preempt.h>
#include <linux/rcupdate_wait.h>
#include <linux/sched.h>
#include <linux/smp.h>
#include <linux/delay.h>
#include <linux/srcu.h>

#include "rcu.h"

static void srcu_invoke_callbacks(struct work_struct *work);
static void srcu_reschedule(struct srcu_struct *sp, unsigned long delay);

/*
 * Initialize SRCU combining tree.  Note that statically allocated
 * srcu_struct structures might already have srcu_read_lock() and
 * srcu_read_unlock() running against them.  So if the is_static parameter
 * is set, don't initialize ->srcu_lock_count[] and ->srcu_unlock_count[].
 */
static void init_srcu_struct_nodes(struct srcu_struct *sp, bool is_static)
{
	int cpu;
	int i;
	int level = 0;
	int levelspread[RCU_NUM_LVLS];
	struct srcu_data *sdp;
	struct srcu_node *snp;
	struct srcu_node *snp_first;

	/* Work out the overall tree geometry. */
	sp->level[0] = &sp->node[0];
	for (i = 1; i < rcu_num_lvls; i++)
		sp->level[i] = sp->level[i - 1] + num_rcu_lvl[i - 1];
	rcu_init_levelspread(levelspread, num_rcu_lvl);

	/* Each pass through this loop initializes one srcu_node structure. */
	rcu_for_each_node_breadth_first(sp, snp) {
		spin_lock_init(&snp->lock);
		for (i = 0; i < ARRAY_SIZE(snp->srcu_have_cbs); i++)
			snp->srcu_have_cbs[i] = 0;
		snp->grplo = -1;
		snp->grphi = -1;
		if (snp == &sp->node[0]) {
			/* Root node, special case. */
			snp->srcu_parent = NULL;
			continue;
		}

		/* Non-root node. */
		if (snp == sp->level[level + 1])
			level++;
		snp->srcu_parent = sp->level[level - 1] +
				   (snp - sp->level[level]) /
				   levelspread[level - 1];
	}

	/*
	 * Initialize the per-CPU srcu_data array, which feeds into the
	 * leaves of the srcu_node tree.
	 */
	WARN_ON_ONCE(ARRAY_SIZE(sdp->srcu_lock_count) !=
		     ARRAY_SIZE(sdp->srcu_unlock_count));
	level = rcu_num_lvls - 1;
	snp_first = sp->level[level];
	for_each_possible_cpu(cpu) {
		sdp = per_cpu_ptr(sp->sda, cpu);
		spin_lock_init(&sdp->lock);
		rcu_segcblist_init(&sdp->srcu_cblist);
		sdp->srcu_cblist_invoking = false;
		sdp->srcu_gp_seq_needed = sp->srcu_gp_seq;
		sdp->mynode = &snp_first[cpu / levelspread[level]];
		for (snp = sdp->mynode; snp != NULL; snp = snp->srcu_parent) {
			if (snp->grplo < 0)
				snp->grplo = cpu;
			snp->grphi = cpu;
		}
		sdp->cpu = cpu;
		INIT_DELAYED_WORK(&sdp->work, srcu_invoke_callbacks);
		sdp->sp = sp;
		if (is_static)
			continue;

		/* Dynamically allocated, better be no srcu_read_locks()! */
		for (i = 0; i < ARRAY_SIZE(sdp->srcu_lock_count); i++) {
			sdp->srcu_lock_count[i] = 0;
			sdp->srcu_unlock_count[i] = 0;
		}
	}
}

/*
 * Initialize non-compile-time initialized fields, including the
 * associated srcu_node and srcu_data structures.  The is_static
 * parameter is passed through to init_srcu_struct_nodes(), and
 * also tells us that ->sda has already been wired up to srcu_data.
 */
static int init_srcu_struct_fields(struct srcu_struct *sp, bool is_static)
{
	mutex_init(&sp->srcu_cb_mutex);
	mutex_init(&sp->srcu_gp_mutex);
	sp->srcu_idx = 0;
	sp->srcu_gp_seq = 0;
	atomic_set(&sp->srcu_exp_cnt, 0);
	sp->srcu_barrier_seq = 0;
	mutex_init(&sp->srcu_barrier_mutex);
	atomic_set(&sp->srcu_barrier_cpu_cnt, 0);
	INIT_DELAYED_WORK(&sp->work, process_srcu);
	if (!is_static)
		sp->sda = alloc_percpu(struct srcu_data);
	init_srcu_struct_nodes(sp, is_static);
	smp_store_release(&sp->srcu_gp_seq_needed, 0); /* Init done. */
	return sp->sda ? 0 : -ENOMEM;
}

#ifdef CONFIG_DEBUG_LOCK_ALLOC

int __init_srcu_struct(struct srcu_struct *sp, const char *name,
		       struct lock_class_key *key)
{
	/* Don't re-initialize a lock while it is held. */
	debug_check_no_locks_freed((void *)sp, sizeof(*sp));
	lockdep_init_map(&sp->dep_map, name, key, 0);
	spin_lock_init(&sp->gp_lock);
	return init_srcu_struct_fields(sp, false);
}
EXPORT_SYMBOL_GPL(__init_srcu_struct);

#else /* #ifdef CONFIG_DEBUG_LOCK_ALLOC */

/**
 * init_srcu_struct - initialize a sleep-RCU structure
 * @sp: structure to initialize.
 *
 * Must invoke this on a given srcu_struct before passing that srcu_struct
 * to any other function.  Each srcu_struct represents a separate domain
 * of SRCU protection.
 */
int init_srcu_struct(struct srcu_struct *sp)
{
	spin_lock_init(&sp->gp_lock);
	return init_srcu_struct_fields(sp, false);
}
EXPORT_SYMBOL_GPL(init_srcu_struct);

#endif /* #else #ifdef CONFIG_DEBUG_LOCK_ALLOC */

/*
 * First-use initialization of statically allocated srcu_struct
 * structure.  Wiring up the combining tree is more than can be
 * done with compile-time initialization, so this check is added
 * to each update-side SRCU primitive.  Use ->gp_lock, which -is-
 * compile-time initialized, to resolve races involving multiple
 * CPUs trying to garner first-use privileges.
 */
static void check_init_srcu_struct(struct srcu_struct *sp)
{
	unsigned long flags;

	WARN_ON_ONCE(rcu_scheduler_active == RCU_SCHEDULER_INIT);
	/* The smp_load_acquire() pairs with the smp_store_release(). */
	if (!rcu_seq_state(smp_load_acquire(&sp->srcu_gp_seq_needed))) /*^^^*/
		return; /* Already initialized. */
	spin_lock_irqsave(&sp->gp_lock, flags);
	if (!rcu_seq_state(sp->srcu_gp_seq_needed)) {
		spin_unlock_irqrestore(&sp->gp_lock, flags);
		return;
	}
	init_srcu_struct_fields(sp, true);
	spin_unlock_irqrestore(&sp->gp_lock, flags);
}

/*
 * Returns approximate total of the readers' ->srcu_lock_count[] values
 * for the rank of per-CPU counters specified by idx.
 */
static unsigned long srcu_readers_lock_idx(struct srcu_struct *sp, int idx)
{
	int cpu;
	unsigned long sum = 0;

	for_each_possible_cpu(cpu) {
		struct srcu_data *cpuc = per_cpu_ptr(sp->sda, cpu);

		sum += READ_ONCE(cpuc->srcu_lock_count[idx]);
	}
	return sum;
}

/*
 * Returns approximate total of the readers' ->srcu_unlock_count[] values
 * for the rank of per-CPU counters specified by idx.
 */
static unsigned long srcu_readers_unlock_idx(struct srcu_struct *sp, int idx)
{
	int cpu;
	unsigned long sum = 0;

	for_each_possible_cpu(cpu) {
		struct srcu_data *cpuc = per_cpu_ptr(sp->sda, cpu);

		sum += READ_ONCE(cpuc->srcu_unlock_count[idx]);
	}
	return sum;
}

/*
 * Return true if the number of pre-existing readers is determined to
 * be zero.
 */
static bool srcu_readers_active_idx_check(struct srcu_struct *sp, int idx)
{
	unsigned long unlocks;

	unlocks = srcu_readers_unlock_idx(sp, idx);

	/*
	 * Make sure that a lock is always counted if the corresponding
	 * unlock is counted. Needs to be a smp_mb() as the read side may
	 * contain a read from a variable that is written to before the
	 * synchronize_srcu() in the write side. In this case smp_mb()s
	 * A and B act like the store buffering pattern.
	 *
	 * This smp_mb() also pairs with smp_mb() C to prevent accesses
	 * after the synchronize_srcu() from being executed before the
	 * grace period ends.
	 */
	smp_mb(); /* A */

	/*
	 * If the locks are the same as the unlocks, then there must have
	 * been no readers on this index at some time in between. This does
	 * not mean that there are no more readers, as one could have read
	 * the current index but not have incremented the lock counter yet.
	 *
	 * Possible bug: There is no guarantee that there haven't been
	 * ULONG_MAX increments of ->srcu_lock_count[] since the unlocks were
	 * counted, meaning that this could return true even if there are
	 * still active readers.  Since there are no memory barriers around
	 * srcu_flip(), the CPU is not required to increment ->srcu_idx
	 * before running srcu_readers_unlock_idx(), which means that there
	 * could be an arbitrarily large number of critical sections that
	 * execute after srcu_readers_unlock_idx() but use the old value
	 * of ->srcu_idx.
	 */
	return srcu_readers_lock_idx(sp, idx) == unlocks;
}

/**
 * srcu_readers_active - returns true if there are readers. and false
 *                       otherwise
 * @sp: which srcu_struct to count active readers (holding srcu_read_lock).
 *
 * Note that this is not an atomic primitive, and can therefore suffer
 * severe errors when invoked on an active srcu_struct.  That said, it
 * can be useful as an error check at cleanup time.
 */
static bool srcu_readers_active(struct srcu_struct *sp)
{
	int cpu;
	unsigned long sum = 0;

	for_each_possible_cpu(cpu) {
		struct srcu_data *cpuc = per_cpu_ptr(sp->sda, cpu);

		sum += READ_ONCE(cpuc->srcu_lock_count[0]);
		sum += READ_ONCE(cpuc->srcu_lock_count[1]);
		sum -= READ_ONCE(cpuc->srcu_unlock_count[0]);
		sum -= READ_ONCE(cpuc->srcu_unlock_count[1]);
	}
	return sum;
}

#define SRCU_INTERVAL		1

/**
 * cleanup_srcu_struct - deconstruct a sleep-RCU structure
 * @sp: structure to clean up.
 *
 * Must invoke this after you are finished using a given srcu_struct that
 * was initialized via init_srcu_struct(), else you leak memory.
 */
void cleanup_srcu_struct(struct srcu_struct *sp)
{
	int cpu;

	WARN_ON_ONCE(atomic_read(&sp->srcu_exp_cnt));
	if (WARN_ON(srcu_readers_active(sp)))
		return; /* Leakage unless caller handles error. */
	flush_delayed_work(&sp->work);
	for_each_possible_cpu(cpu)
		flush_delayed_work(&per_cpu_ptr(sp->sda, cpu)->work);
	if (WARN_ON(rcu_seq_state(READ_ONCE(sp->srcu_gp_seq)) != SRCU_STATE_IDLE) ||
	    WARN_ON(srcu_readers_active(sp))) {
		pr_info("cleanup_srcu_struct: Active srcu_struct %p state: %d\n", sp, rcu_seq_state(READ_ONCE(sp->srcu_gp_seq)));
		return; /* Caller forgot to stop doing call_srcu()? */
	}
	free_percpu(sp->sda);
	sp->sda = NULL;
}
EXPORT_SYMBOL_GPL(cleanup_srcu_struct);

/*
 * Counts the new reader in the appropriate per-CPU element of the
 * srcu_struct.  Must be called from process context.
 * Returns an index that must be passed to the matching srcu_read_unlock().
 */
int __srcu_read_lock(struct srcu_struct *sp)
{
	int idx;

	idx = READ_ONCE(sp->srcu_idx) & 0x1;
	__this_cpu_inc(sp->sda->srcu_lock_count[idx]);
	smp_mb(); /* B */  /* Avoid leaking the critical section. */
	return idx;
}
EXPORT_SYMBOL_GPL(__srcu_read_lock);

/*
 * Removes the count for the old reader from the appropriate per-CPU
 * element of the srcu_struct.  Note that this may well be a different
 * CPU than that which was incremented by the corresponding srcu_read_lock().
 * Must be called from process context.
 */
void __srcu_read_unlock(struct srcu_struct *sp, int idx)
{
	smp_mb(); /* C */  /* Avoid leaking the critical section. */
	this_cpu_inc(sp->sda->srcu_unlock_count[idx]);
}
EXPORT_SYMBOL_GPL(__srcu_read_unlock);

/*
 * We use an adaptive strategy for synchronize_srcu() and especially for
 * synchronize_srcu_expedited().  We spin for a fixed time period
 * (defined below) to allow SRCU readers to exit their read-side critical
 * sections.  If there are still some readers after a few microseconds,
 * we repeatedly block for 1-millisecond time periods.
 */
#define SRCU_RETRY_CHECK_DELAY		5

/*
 * Start an SRCU grace period.
 */
static void srcu_gp_start(struct srcu_struct *sp)
{
	struct srcu_data *sdp = this_cpu_ptr(sp->sda);
	int state;

	RCU_LOCKDEP_WARN(!lockdep_is_held(&sp->gp_lock),
			 "Invoked srcu_gp_start() without ->gp_lock!");
	WARN_ON_ONCE(ULONG_CMP_GE(sp->srcu_gp_seq, sp->srcu_gp_seq_needed));
	rcu_segcblist_advance(&sdp->srcu_cblist,
			      rcu_seq_current(&sp->srcu_gp_seq));
	(void)rcu_segcblist_accelerate(&sdp->srcu_cblist,
				       rcu_seq_snap(&sp->srcu_gp_seq));
	rcu_seq_start(&sp->srcu_gp_seq);
	state = rcu_seq_state(READ_ONCE(sp->srcu_gp_seq));
	WARN_ON_ONCE(state != SRCU_STATE_SCAN1);
}

/*
 * Track online CPUs to guide callback workqueue placement.
 */
DEFINE_PER_CPU(bool, srcu_online);

void srcu_online_cpu(unsigned int cpu)
{
	WRITE_ONCE(per_cpu(srcu_online, cpu), true);
}

void srcu_offline_cpu(unsigned int cpu)
{
	WRITE_ONCE(per_cpu(srcu_online, cpu), false);
}

/*
 * Place the workqueue handler on the specified CPU if online, otherwise
 * just run it whereever.  This is useful for placing workqueue handlers
 * that are to invoke the specified CPU's callbacks.
 */
static bool srcu_queue_delayed_work_on(int cpu, struct workqueue_struct *wq,
				       struct delayed_work *dwork,
				       unsigned long delay)
{
	bool ret;

	preempt_disable();
	if (READ_ONCE(per_cpu(srcu_online, cpu)))
		ret = queue_delayed_work_on(cpu, wq, dwork, delay);
	else
		ret = queue_delayed_work(wq, dwork, delay);
	preempt_enable();
	return ret;
}

/*
 * Schedule callback invocation for the specified srcu_data structure,
 * if possible, on the corresponding CPU.
 */
static void srcu_schedule_cbs_sdp(struct srcu_data *sdp, unsigned long delay)
{
	srcu_queue_delayed_work_on(sdp->cpu, system_power_efficient_wq,
				   &sdp->work, delay);
}

/*
 * Schedule callback invocation for all srcu_data structures associated
 * with the specified srcu_node structure, if possible, on the corresponding
 * CPUs.
 */
static void srcu_schedule_cbs_snp(struct srcu_struct *sp, struct srcu_node *snp)
{
	int cpu;

	for (cpu = snp->grplo; cpu <= snp->grphi; cpu++)
		srcu_schedule_cbs_sdp(per_cpu_ptr(sp->sda, cpu), SRCU_INTERVAL);
}

/*
 * Note the end of an SRCU grace period.  Initiates callback invocation
 * and starts a new grace period if needed.
 *
 * The ->srcu_cb_mutex acquisition does not protect any data, but
 * instead prevents more than one grace period from starting while we
 * are initiating callback invocation.  This allows the ->srcu_have_cbs[]
 * array to have a finite number of elements.
 */
static void srcu_gp_end(struct srcu_struct *sp)
{
	bool cbs;
	unsigned long gpseq;
	int idx;
	int idxnext;
	struct srcu_node *snp;

	/* Prevent more than one additional grace period. */
	mutex_lock(&sp->srcu_cb_mutex);

	/* End the current grace period. */
	spin_lock_irq(&sp->gp_lock);
	idx = rcu_seq_state(sp->srcu_gp_seq);
	WARN_ON_ONCE(idx != SRCU_STATE_SCAN2);
	rcu_seq_end(&sp->srcu_gp_seq);
	gpseq = rcu_seq_current(&sp->srcu_gp_seq);
	spin_unlock_irq(&sp->gp_lock);
	mutex_unlock(&sp->srcu_gp_mutex);
	/* A new grace period can start at this point.  But only one. */

	/* Initiate callback invocation as needed. */
	idx = rcu_seq_ctr(gpseq) % ARRAY_SIZE(snp->srcu_have_cbs);
	idxnext = (idx + 1) % ARRAY_SIZE(snp->srcu_have_cbs);
	rcu_for_each_node_breadth_first(sp, snp) {
		spin_lock_irq(&snp->lock);
		cbs = false;
		if (snp >= sp->level[rcu_num_lvls - 1])
			cbs = snp->srcu_have_cbs[idx] == gpseq;
		snp->srcu_have_cbs[idx] = gpseq;
		rcu_seq_set_state(&snp->srcu_have_cbs[idx], 1);
		spin_unlock_irq(&snp->lock);
		if (cbs) {
			smp_mb(); /* GP end before CB invocation. */
			srcu_schedule_cbs_snp(sp, snp);
		}
	}

	/* Callback initiation done, allow grace periods after next. */
	mutex_unlock(&sp->srcu_cb_mutex);

	/* Start a new grace period if needed. */
	spin_lock_irq(&sp->gp_lock);
	gpseq = rcu_seq_current(&sp->srcu_gp_seq);
	if (!rcu_seq_state(gpseq) &&
	    ULONG_CMP_LT(gpseq, sp->srcu_gp_seq_needed)) {
		srcu_gp_start(sp);
		spin_unlock_irq(&sp->gp_lock);
		/* Throttle expedited grace periods: Should be rare! */
		srcu_reschedule(sp, atomic_read(&sp->srcu_exp_cnt) &&
				    rcu_seq_ctr(gpseq) & 0xf
				    ? 0
				    : SRCU_INTERVAL);
	} else {
		spin_unlock_irq(&sp->gp_lock);
	}
}

/*
 * Funnel-locking scheme to scalably mediate many concurrent grace-period
 * requests.  The winner has to do the work of actually starting grace
 * period s.  Losers must either ensure that their desired grace-period
 * number is recorded on at least their leaf srcu_node structure, or they
 * must take steps to invoke their own callbacks.
 */
static void srcu_funnel_gp_start(struct srcu_struct *sp,
				 struct srcu_data *sdp,
				 unsigned long s)
{
	unsigned long flags;
	int idx = rcu_seq_ctr(s) % ARRAY_SIZE(sdp->mynode->srcu_have_cbs);
	struct srcu_node *snp = sdp->mynode;
	unsigned long snp_seq;

	/* Each pass through the loop does one level of the srcu_node tree. */
	for (; snp != NULL; snp = snp->srcu_parent) {
		if (rcu_seq_done(&sp->srcu_gp_seq, s) && snp != sdp->mynode)
			return; /* GP already done and CBs recorded. */
		spin_lock_irqsave(&snp->lock, flags);
		if (ULONG_CMP_GE(snp->srcu_have_cbs[idx], s)) {
			snp_seq = snp->srcu_have_cbs[idx];
			spin_unlock_irqrestore(&snp->lock, flags);
			if (snp == sdp->mynode && snp_seq != s) {
				smp_mb(); /* CBs after GP! */
				srcu_schedule_cbs_sdp(sdp, 0);
			}
			return;
		}
		snp->srcu_have_cbs[idx] = s;
		spin_unlock_irqrestore(&snp->lock, flags);
	}

	/* Top of tree, must ensure the grace period will be started. */
	spin_lock_irqsave(&sp->gp_lock, flags);
	if (ULONG_CMP_LT(sp->srcu_gp_seq_needed, s)) {
		/*
		 * Record need for grace period s.  Pair with load
		 * acquire setting up for initialization.
		 */
		smp_store_release(&sp->srcu_gp_seq_needed, s); /*^^^*/
	}

	/* If grace period not already done and none in progress, start it. */
	if (!rcu_seq_done(&sp->srcu_gp_seq, s) &&
	    rcu_seq_state(sp->srcu_gp_seq) == SRCU_STATE_IDLE) {
		WARN_ON_ONCE(ULONG_CMP_GE(sp->srcu_gp_seq, sp->srcu_gp_seq_needed));
		srcu_gp_start(sp);
		queue_delayed_work(system_power_efficient_wq, &sp->work,
				   atomic_read(&sp->srcu_exp_cnt)
				   ? 0
				   : SRCU_INTERVAL);
	}
	spin_unlock_irqrestore(&sp->gp_lock, flags);
}

/*
 * Wait until all readers counted by array index idx complete, but
 * loop an additional time if there is an expedited grace period pending.
 * The caller must ensure that ->srcu_idx is not changed while checking.
 */
static bool try_check_zero(struct srcu_struct *sp, int idx, int trycount)
{
	for (;;) {
		if (srcu_readers_active_idx_check(sp, idx))
			return true;
		if (--trycount + !!atomic_read(&sp->srcu_exp_cnt) <= 0)
			return false;
		udelay(SRCU_RETRY_CHECK_DELAY);
	}
}

/*
 * Increment the ->srcu_idx counter so that future SRCU readers will
 * use the other rank of the ->srcu_(un)lock_count[] arrays.  This allows
 * us to wait for pre-existing readers in a starvation-free manner.
 */
static void srcu_flip(struct srcu_struct *sp)
{
	WRITE_ONCE(sp->srcu_idx, sp->srcu_idx + 1);

	/*
	 * Ensure that if the updater misses an __srcu_read_unlock()
	 * increment, that task's next __srcu_read_lock() will see the
	 * above counter update.  Note that both this memory barrier
	 * and the one in srcu_readers_active_idx_check() provide the
	 * guarantee for __srcu_read_lock().
	 */
	smp_mb(); /* D */  /* Pairs with C. */
}

/*
 * Enqueue an SRCU callback on the srcu_data structure associated with
 * the current CPU and the specified srcu_struct structure, initiating
 * grace-period processing if it is not already running.
 *
 * Note that all CPUs must agree that the grace period extended beyond
 * all pre-existing SRCU read-side critical section.  On systems with
 * more than one CPU, this means that when "func()" is invoked, each CPU
 * is guaranteed to have executed a full memory barrier since the end of
 * its last corresponding SRCU read-side critical section whose beginning
 * preceded the call to call_rcu().  It also means that each CPU executing
 * an SRCU read-side critical section that continues beyond the start of
 * "func()" must have executed a memory barrier after the call_rcu()
 * but before the beginning of that SRCU read-side critical section.
 * Note that these guarantees include CPUs that are offline, idle, or
 * executing in user mode, as well as CPUs that are executing in the kernel.
 *
 * Furthermore, if CPU A invoked call_rcu() and CPU B invoked the
 * resulting SRCU callback function "func()", then both CPU A and CPU
 * B are guaranteed to execute a full memory barrier during the time
 * interval between the call to call_rcu() and the invocation of "func()".
 * This guarantee applies even if CPU A and CPU B are the same CPU (but
 * again only if the system has more than one CPU).
 *
 * Of course, these guarantees apply only for invocations of call_srcu(),
 * srcu_read_lock(), and srcu_read_unlock() that are all passed the same
 * srcu_struct structure.
 */
void call_srcu(struct srcu_struct *sp, struct rcu_head *rhp,
	       rcu_callback_t func)
{
	unsigned long flags;
	bool needgp = false;
	unsigned long s;
	struct srcu_data *sdp;

	check_init_srcu_struct(sp);
	rhp->func = func;
	local_irq_save(flags);
	sdp = this_cpu_ptr(sp->sda);
	spin_lock(&sdp->lock);
	rcu_segcblist_enqueue(&sdp->srcu_cblist, rhp, false);
	rcu_segcblist_advance(&sdp->srcu_cblist,
			      rcu_seq_current(&sp->srcu_gp_seq));
	s = rcu_seq_snap(&sp->srcu_gp_seq);
	(void)rcu_segcblist_accelerate(&sdp->srcu_cblist, s);
	if (ULONG_CMP_LT(sdp->srcu_gp_seq_needed, s)) {
		sdp->srcu_gp_seq_needed = s;
		needgp = true;
	}
	spin_unlock_irqrestore(&sdp->lock, flags);
	if (needgp)
		srcu_funnel_gp_start(sp, sdp, s);
}
EXPORT_SYMBOL_GPL(call_srcu);

/*
 * Helper function for synchronize_srcu() and synchronize_srcu_expedited().
 */
static void __synchronize_srcu(struct srcu_struct *sp)
{
	struct rcu_synchronize rcu;

	RCU_LOCKDEP_WARN(lock_is_held(&sp->dep_map) ||
			 lock_is_held(&rcu_bh_lock_map) ||
			 lock_is_held(&rcu_lock_map) ||
			 lock_is_held(&rcu_sched_lock_map),
			 "Illegal synchronize_srcu() in same-type SRCU (or in RCU) read-side critical section");

	if (rcu_scheduler_active == RCU_SCHEDULER_INACTIVE)
		return;
	might_sleep();
	check_init_srcu_struct(sp);
	init_completion(&rcu.completion);
	init_rcu_head_on_stack(&rcu.head);
	call_srcu(sp, &rcu.head, wakeme_after_rcu);
	wait_for_completion(&rcu.completion);
	destroy_rcu_head_on_stack(&rcu.head);
}

/**
 * synchronize_srcu_expedited - Brute-force SRCU grace period
 * @sp: srcu_struct with which to synchronize.
 *
 * Wait for an SRCU grace period to elapse, but be more aggressive about
 * spinning rather than blocking when waiting.
 *
 * Note that synchronize_srcu_expedited() has the same deadlock and
 * memory-ordering properties as does synchronize_srcu().
 */
void synchronize_srcu_expedited(struct srcu_struct *sp)
{
	bool do_norm = rcu_gp_is_normal();

	check_init_srcu_struct(sp);
	if (!do_norm) {
		atomic_inc(&sp->srcu_exp_cnt);
		smp_mb__after_atomic(); /* increment before GP. */
	}
	__synchronize_srcu(sp);
	if (!do_norm) {
		smp_mb__before_atomic(); /* GP before decrement. */
		WARN_ON_ONCE(atomic_dec_return(&sp->srcu_exp_cnt) < 0);
	}
}
EXPORT_SYMBOL_GPL(synchronize_srcu_expedited);

/**
 * synchronize_srcu - wait for prior SRCU read-side critical-section completion
 * @sp: srcu_struct with which to synchronize.
 *
 * Wait for the count to drain to zero of both indexes. To avoid the
 * possible starvation of synchronize_srcu(), it waits for the count of
 * the index=((->srcu_idx & 1) ^ 1) to drain to zero at first,
 * and then flip the srcu_idx and wait for the count of the other index.
 *
 * Can block; must be called from process context.
 *
 * Note that it is illegal to call synchronize_srcu() from the corresponding
 * SRCU read-side critical section; doing so will result in deadlock.
 * However, it is perfectly legal to call synchronize_srcu() on one
 * srcu_struct from some other srcu_struct's read-side critical section,
 * as long as the resulting graph of srcu_structs is acyclic.
 *
 * There are memory-ordering constraints implied by synchronize_srcu().
 * On systems with more than one CPU, when synchronize_srcu() returns,
 * each CPU is guaranteed to have executed a full memory barrier since
 * the end of its last corresponding SRCU-sched read-side critical section
 * whose beginning preceded the call to synchronize_srcu().  In addition,
 * each CPU having an SRCU read-side critical section that extends beyond
 * the return from synchronize_srcu() is guaranteed to have executed a
 * full memory barrier after the beginning of synchronize_srcu() and before
 * the beginning of that SRCU read-side critical section.  Note that these
 * guarantees include CPUs that are offline, idle, or executing in user mode,
 * as well as CPUs that are executing in the kernel.
 *
 * Furthermore, if CPU A invoked synchronize_srcu(), which returned
 * to its caller on CPU B, then both CPU A and CPU B are guaranteed
 * to have executed a full memory barrier during the execution of
 * synchronize_srcu().  This guarantee applies even if CPU A and CPU B
 * are the same CPU, but again only if the system has more than one CPU.
 *
 * Of course, these memory-ordering guarantees apply only when
 * synchronize_srcu(), srcu_read_lock(), and srcu_read_unlock() are
 * passed the same srcu_struct structure.
 */
void synchronize_srcu(struct srcu_struct *sp)
{
	if (rcu_gp_is_expedited())
		synchronize_srcu_expedited(sp);
	else
		__synchronize_srcu(sp);
}
EXPORT_SYMBOL_GPL(synchronize_srcu);

/*
 * Callback function for srcu_barrier() use.
 */
static void srcu_barrier_cb(struct rcu_head *rhp)
{
	struct srcu_data *sdp;
	struct srcu_struct *sp;

	sdp = container_of(rhp, struct srcu_data, srcu_barrier_head);
	sp = sdp->sp;
	if (atomic_dec_and_test(&sp->srcu_barrier_cpu_cnt))
		complete(&sp->srcu_barrier_completion);
}

/**
 * srcu_barrier - Wait until all in-flight call_srcu() callbacks complete.
 * @sp: srcu_struct on which to wait for in-flight callbacks.
 */
void srcu_barrier(struct srcu_struct *sp)
{
	int cpu;
	struct srcu_data *sdp;
	unsigned long s = rcu_seq_snap(&sp->srcu_barrier_seq);

	check_init_srcu_struct(sp);
	mutex_lock(&sp->srcu_barrier_mutex);
	if (rcu_seq_done(&sp->srcu_barrier_seq, s)) {
		smp_mb(); /* Force ordering following return. */
		mutex_unlock(&sp->srcu_barrier_mutex);
		return; /* Someone else did our work for us. */
	}
	rcu_seq_start(&sp->srcu_barrier_seq);
	init_completion(&sp->srcu_barrier_completion);

	/* Initial count prevents reaching zero until all CBs are posted. */
	atomic_set(&sp->srcu_barrier_cpu_cnt, 1);

	/*
	 * Each pass through this loop enqueues a callback, but only
	 * on CPUs already having callbacks enqueued.  Note that if
	 * a CPU already has callbacks enqueue, it must have already
	 * registered the need for a future grace period, so all we
	 * need do is enqueue a callback that will use the same
	 * grace period as the last callback already in the queue.
	 */
	for_each_possible_cpu(cpu) {
		sdp = per_cpu_ptr(sp->sda, cpu);
		spin_lock_irq(&sdp->lock);
		atomic_inc(&sp->srcu_barrier_cpu_cnt);
		sdp->srcu_barrier_head.func = srcu_barrier_cb;
		if (!rcu_segcblist_entrain(&sdp->srcu_cblist,
					   &sdp->srcu_barrier_head, 0))
			atomic_dec(&sp->srcu_barrier_cpu_cnt);
		spin_unlock_irq(&sdp->lock);
	}

	/* Remove the initial count, at which point reaching zero can happen. */
	if (atomic_dec_and_test(&sp->srcu_barrier_cpu_cnt))
		complete(&sp->srcu_barrier_completion);
	wait_for_completion(&sp->srcu_barrier_completion);

	rcu_seq_end(&sp->srcu_barrier_seq);
	mutex_unlock(&sp->srcu_barrier_mutex);
}
EXPORT_SYMBOL_GPL(srcu_barrier);

/**
 * srcu_batches_completed - return batches completed.
 * @sp: srcu_struct on which to report batch completion.
 *
 * Report the number of batches, correlated with, but not necessarily
 * precisely the same as, the number of grace periods that have elapsed.
 */
unsigned long srcu_batches_completed(struct srcu_struct *sp)
{
	return sp->srcu_idx;
}
EXPORT_SYMBOL_GPL(srcu_batches_completed);

/*
 * Core SRCU state machine.  Push state bits of ->srcu_gp_seq
 * to SRCU_STATE_SCAN2, and invoke srcu_gp_end() when scan has
 * completed in that state.
 */
static void srcu_advance_state(struct srcu_struct *sp)
{
	int idx;

	mutex_lock(&sp->srcu_gp_mutex);

	/*
	 * Because readers might be delayed for an extended period after
	 * fetching ->srcu_idx for their index, at any point in time there
	 * might well be readers using both idx=0 and idx=1.  We therefore
	 * need to wait for readers to clear from both index values before
	 * invoking a callback.
	 *
	 * The load-acquire ensures that we see the accesses performed
	 * by the prior grace period.
	 */
	idx = rcu_seq_state(smp_load_acquire(&sp->srcu_gp_seq)); /* ^^^ */
	if (idx == SRCU_STATE_IDLE) {
		spin_lock_irq(&sp->gp_lock);
		if (ULONG_CMP_GE(sp->srcu_gp_seq, sp->srcu_gp_seq_needed)) {
			WARN_ON_ONCE(rcu_seq_state(sp->srcu_gp_seq));
			spin_unlock_irq(&sp->gp_lock);
			mutex_unlock(&sp->srcu_gp_mutex);
			return;
		}
		idx = rcu_seq_state(READ_ONCE(sp->srcu_gp_seq));
		if (idx == SRCU_STATE_IDLE)
			srcu_gp_start(sp);
		spin_unlock_irq(&sp->gp_lock);
		if (idx != SRCU_STATE_IDLE) {
			mutex_unlock(&sp->srcu_gp_mutex);
			return; /* Someone else started the grace period. */
		}
	}

	if (rcu_seq_state(READ_ONCE(sp->srcu_gp_seq)) == SRCU_STATE_SCAN1) {
		idx = 1 ^ (sp->srcu_idx & 1);
		if (!try_check_zero(sp, idx, 1)) {
			mutex_unlock(&sp->srcu_gp_mutex);
			return; /* readers present, retry later. */
		}
		srcu_flip(sp);
		rcu_seq_set_state(&sp->srcu_gp_seq, SRCU_STATE_SCAN2);
	}

	if (rcu_seq_state(READ_ONCE(sp->srcu_gp_seq)) == SRCU_STATE_SCAN2) {

		/*
		 * SRCU read-side critical sections are normally short,
		 * so check at least twice in quick succession after a flip.
		 */
		idx = 1 ^ (sp->srcu_idx & 1);
		if (!try_check_zero(sp, idx, 2)) {
			mutex_unlock(&sp->srcu_gp_mutex);
			return; /* readers present, retry later. */
		}
		srcu_gp_end(sp);  /* Releases ->srcu_gp_mutex. */
	}
}

/*
 * Invoke a limited number of SRCU callbacks that have passed through
 * their grace period.  If there are more to do, SRCU will reschedule
 * the workqueue.  Note that needed memory barriers have been executed
 * in this task's context by srcu_readers_active_idx_check().
 */
static void srcu_invoke_callbacks(struct work_struct *work)
{
	bool more;
	struct rcu_cblist ready_cbs;
	struct rcu_head *rhp;
	struct srcu_data *sdp;
	struct srcu_struct *sp;

	sdp = container_of(work, struct srcu_data, work.work);
	sp = sdp->sp;
	rcu_cblist_init(&ready_cbs);
	spin_lock_irq(&sdp->lock);
	smp_mb(); /* Old grace periods before callback invocation! */
	rcu_segcblist_advance(&sdp->srcu_cblist,
			      rcu_seq_current(&sp->srcu_gp_seq));
	if (sdp->srcu_cblist_invoking ||
	    !rcu_segcblist_ready_cbs(&sdp->srcu_cblist)) {
		spin_unlock_irq(&sdp->lock);
		return;  /* Someone else on the job or nothing to do. */
	}

	/* We are on the job!  Extract and invoke ready callbacks. */
	sdp->srcu_cblist_invoking = true;
	rcu_segcblist_extract_done_cbs(&sdp->srcu_cblist, &ready_cbs);
	spin_unlock_irq(&sdp->lock);
	rhp = rcu_cblist_dequeue(&ready_cbs);
	for (; rhp != NULL; rhp = rcu_cblist_dequeue(&ready_cbs)) {
		local_bh_disable();
		rhp->func(rhp);
		local_bh_enable();
	}

	/*
	 * Update counts, accelerate new callbacks, and if needed,
	 * schedule another round of callback invocation.
	 */
	spin_lock_irq(&sdp->lock);
	rcu_segcblist_insert_count(&sdp->srcu_cblist, &ready_cbs);
	(void)rcu_segcblist_accelerate(&sdp->srcu_cblist,
				       rcu_seq_snap(&sp->srcu_gp_seq));
	sdp->srcu_cblist_invoking = false;
	more = rcu_segcblist_ready_cbs(&sdp->srcu_cblist);
	spin_unlock_irq(&sdp->lock);
	if (more)
		srcu_schedule_cbs_sdp(sdp, 0);
}

/*
 * Finished one round of SRCU grace period.  Start another if there are
 * more SRCU callbacks queued, otherwise put SRCU into not-running state.
 */
static void srcu_reschedule(struct srcu_struct *sp, unsigned long delay)
{
	bool pushgp = true;

	spin_lock_irq(&sp->gp_lock);
	if (ULONG_CMP_GE(sp->srcu_gp_seq, sp->srcu_gp_seq_needed)) {
		if (!WARN_ON_ONCE(rcu_seq_state(sp->srcu_gp_seq))) {
			/* All requests fulfilled, time to go idle. */
			pushgp = false;
		}
	} else if (!rcu_seq_state(sp->srcu_gp_seq)) {
		/* Outstanding request and no GP.  Start one. */
		srcu_gp_start(sp);
	}
	spin_unlock_irq(&sp->gp_lock);

	if (pushgp)
		queue_delayed_work(system_power_efficient_wq, &sp->work, delay);
}

/*
 * This is the work-queue function that handles SRCU grace periods.
 */
void process_srcu(struct work_struct *work)
{
	struct srcu_struct *sp;

	sp = container_of(work, struct srcu_struct, work.work);

	srcu_advance_state(sp);
	srcu_reschedule(sp, atomic_read(&sp->srcu_exp_cnt) ? 0 : SRCU_INTERVAL);
}
EXPORT_SYMBOL_GPL(process_srcu);
