/*
 * Queue spinlock
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
 * (C) Copyright 2013-2014 Hewlett-Packard Development Company, L.P.
 *
 * Authors: Waiman Long <waiman.long@hp.com>
 *          Peter Zijlstra <pzijlstr@redhat.com>
 */
#include <linux/smp.h>
#include <linux/bug.h>
#include <linux/cpumask.h>
#include <linux/percpu.h>
#include <linux/hardirq.h>
#include <linux/mutex.h>
#include <asm/qspinlock.h>

/*
 * The basic principle of a queue-based spinlock can best be understood
 * by studying a classic queue-based spinlock implementation called the
 * MCS lock. The paper below provides a good description for this kind
 * of lock.
 *
 * http://www.cise.ufl.edu/tr/DOC/REP-1992-71.pdf
 *
 * This queue spinlock implementation is based on the MCS lock, however to make
 * it fit the 4 bytes we assume spinlock_t to be, and preserve its existing
 * API, we must modify it some.
 *
 * In particular; where the traditional MCS lock consists of a tail pointer
 * (8 bytes) and needs the next pointer (another 8 bytes) of its own node to
 * unlock the next pending (next->locked), we compress both these: {tail,
 * next->locked} into a single u32 value.
 *
 * Since a spinlock disables recursion of its own context and there is a limit
 * to the contexts that can nest; namely: task, softirq, hardirq, nmi, we can
 * encode the tail as and index indicating this context and a cpu number.
 *
 * We can further change the first spinner to spin on a bit in the lock word
 * instead of its node; whereby avoiding the need to carry a node from lock to
 * unlock, and preserving API.
 */

#include "mcs_spinlock.h"

/*
 * Per-CPU queue node structures; we can never have more than 4 nested
 * contexts: task, softirq, hardirq, nmi.
 *
 * Exactly fits one cacheline.
 */
static DEFINE_PER_CPU_ALIGNED(struct mcs_spinlock, mcs_nodes[4]);

/*
 * We must be able to distinguish between no-tail and the tail at 0:0,
 * therefore increment the cpu number by one.
 */

static inline u32 encode_tail(int cpu, int idx)
{
	u32 tail;

	tail  = (cpu + 1) << _Q_TAIL_CPU_OFFSET;
	tail |= idx << _Q_TAIL_IDX_OFFSET; /* assume < 4 */

	return tail;
}

static inline struct mcs_spinlock *decode_tail(u32 tail)
{
	int cpu = (tail >> _Q_TAIL_CPU_OFFSET) - 1;
	int idx = (tail &  _Q_TAIL_IDX_MASK) >> _Q_TAIL_IDX_OFFSET;

	return per_cpu_ptr(&mcs_nodes[idx], cpu);
}

#define _Q_LOCKED_PENDING_MASK	(_Q_LOCKED_MASK | _Q_PENDING_MASK)

/**
 * trylock_pending - try to acquire queue spinlock using the pending bit
 * @lock : Pointer to queue spinlock structure
 * @pval : Pointer to value of the queue spinlock 32-bit word
 * Return: 1 if lock acquired, 0 otherwise
 */
static inline int trylock_pending(struct qspinlock *lock, u32 *pval)
{
	u32 old, new, val = *pval;

	/*
	 * trylock || pending
	 *
	 * 0,0,0 -> 0,0,1 ; trylock
	 * 0,0,1 -> 0,1,1 ; pending
	 */
	for (;;) {
		/*
		 * If we observe any contention; queue.
		 */
		if (val & ~_Q_LOCKED_MASK)
			return 0;

		new = _Q_LOCKED_VAL;
		if (val == new)
			new |= _Q_PENDING_VAL;

		old = atomic_cmpxchg(&lock->val, val, new);
		if (old == val)
			break;

		*pval = val = old;
	}

	/*
	 * we won the trylock
	 */
	if (new == _Q_LOCKED_VAL)
		return 1;

	/*
	 * we're pending, wait for the owner to go away.
	 *
	 * *,1,1 -> *,1,0
	 */
	while ((val = atomic_read(&lock->val)) & _Q_LOCKED_MASK)
		arch_mutex_cpu_relax();

	/*
	 * take ownership and clear the pending bit.
	 *
	 * *,1,0 -> *,0,1
	 */
	for (;;) {
		new = (val & ~_Q_PENDING_MASK) | _Q_LOCKED_VAL;

		old = atomic_cmpxchg(&lock->val, val, new);
		if (old == val)
			break;

		val = old;
	}
	return 1;
}

/**
 * queue_spin_lock_slowpath - acquire the queue spinlock
 * @lock: Pointer to queue spinlock structure
 * @val: Current value of the queue spinlock 32-bit word
 *
 * (queue tail, pending bit, lock bit)
 *
 *              fast     :    slow                                  :    unlock
 *                       :                                          :
 * uncontended  (0,0,0) -:--> (0,0,1) ------------------------------:--> (*,*,0)
 *                       :       | ^--------.------.             /  :
 *                       :       v           \      \            |  :
 * pending               :    (0,1,1) +--> (0,1,0)   \           |  :
 *                       :       | ^--'              |           |  :
 *                       :       v                   |           |  :
 * uncontended           :    (n,x,y) +--> (n,0,0) --'           |  :
 *   queue               :       | ^--'                          |  :
 *                       :       v                               |  :
 * contended             :    (*,x,y) +--> (*,0,0) ---> (*,0,1) -'  :
 *   queue               :         ^--'                             :
 *
 */
void queue_spin_lock_slowpath(struct qspinlock *lock, u32 val)
{
	struct mcs_spinlock *prev, *next, *node;
	u32 new, old, tail;
	int idx;

	BUILD_BUG_ON(CONFIG_NR_CPUS >= (1U << _Q_TAIL_CPU_BITS));

	if (trylock_pending(lock, &val))
		return;	/* Lock acquired */

	node = this_cpu_ptr(&mcs_nodes[0]);
	idx = node->count++;
	tail = encode_tail(smp_processor_id(), idx);

	node += idx;
	node->locked = 0;
	node->next = NULL;

	/*
	 * we already touched the queueing cacheline; don't bother with pending
	 * stuff.
	 *
	 * trylock || xchg(lock, node)
	 *
	 * 0,0,0 -> 0,0,1 ; trylock
	 * p,y,x -> n,y,x ; prev = xchg(lock, node)
	 */
	for (;;) {
		new = _Q_LOCKED_VAL;
		if (val)
			new = tail | (val & _Q_LOCKED_PENDING_MASK);

		old = atomic_cmpxchg(&lock->val, val, new);
		if (old == val)
			break;

		val = old;
	}

	/*
	 * we won the trylock; forget about queueing.
	 */
	if (new == _Q_LOCKED_VAL)
		goto release;

	/*
	 * if there was a previous node; link it and wait.
	 */
	if (old & ~_Q_LOCKED_PENDING_MASK) {
		prev = decode_tail(old);
		ACCESS_ONCE(prev->next) = node;

		arch_mcs_spin_lock_contended(&node->locked);
	}

	/*
	 * we're at the head of the waitqueue, wait for the owner & pending to
	 * go away.
	 *
	 * *,x,y -> *,0,0
	 */
	while ((val = atomic_read(&lock->val)) & _Q_LOCKED_PENDING_MASK)
		arch_mutex_cpu_relax();

	/*
	 * claim the lock:
	 *
	 * n,0,0 -> 0,0,1 : lock, uncontended
	 * *,0,0 -> *,0,1 : lock, contended
	 */
	for (;;) {
		new = _Q_LOCKED_VAL;
		if (val != tail)
			new |= val;

		old = atomic_cmpxchg(&lock->val, val, new);
		if (old == val)
			break;

		val = old;
	}

	/*
	 * contended path; wait for next, release.
	 */
	if (new != _Q_LOCKED_VAL) {
		while (!(next = ACCESS_ONCE(node->next)))
			arch_mutex_cpu_relax();

		arch_mcs_spin_unlock_contended(&next->locked);
	}

release:
	/*
	 * release the node
	 */
	this_cpu_dec(mcs_nodes[0].count);
}
EXPORT_SYMBOL(queue_spin_lock_slowpath);
