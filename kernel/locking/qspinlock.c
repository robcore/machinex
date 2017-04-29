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
#include <asm/byteorder.h>
#include <asm/qspinlock.h>

#if !defined(__LITTLE_ENDIAN) && !defined(__BIG_ENDIAN)
#error "Missing either LITTLE_ENDIAN or BIG_ENDIAN definition."
#endif

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
 *
 * N.B. The current implementation only supports architectures that allow
 *      atomic operations on smaller 8-bit and 16-bit data types.
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

/*
 * By using the whole 2nd least significant byte for the pending bit, we
 * can allow better optimization of the lock acquisition for the pending
 * bit holder.
 */
#if _Q_PENDING_BITS == 8

struct __qspinlock {
	union {
		atomic_t val;
		struct {
#ifdef __LITTLE_ENDIAN
			u16	locked_pending;
			u16	tail;
#else
			u16	tail;
			u16	locked_pending;
#endif
		};
	};
};

/**
 * clear_pending_set_locked - take ownership and clear the pending bit.
 * @lock: Pointer to queue spinlock structure
 * @val : Current value of the queue spinlock 32-bit word
 *
 * *,1,0 -> *,0,1
 */
static __always_inline void
clear_pending_set_locked(struct qspinlock *lock, u32 val)
{
	struct __qspinlock *l = (void *)lock;

	ACCESS_ONCE(l->locked_pending) = 1;
}

/*
 * xchg_tail - Put in the new queue tail code word & retrieve previous one
 * @lock : Pointer to queue spinlock structure
 * @tail : The new queue tail code word
 * @pval : Pointer to current value of the queue spinlock 32-bit word
 * Return: The previous queue tail code word
 *
 * xchg(lock, tail)
 *
 * p,*,* -> n,*,* ; prev = xchg(lock, node)
 */
static __always_inline u32
xchg_tail(struct qspinlock *lock, u32 tail, u32 *pval)
{
	struct __qspinlock *l = (void *)lock;

	return (u32)xchg(&l->tail, tail >> _Q_TAIL_OFFSET) << _Q_TAIL_OFFSET;
}

#else /* _Q_PENDING_BITS == 8 */

/**
 * clear_pending_set_locked - take ownership and clear the pending bit.
 * @lock: Pointer to queue spinlock structure
 * @val : Current value of the queue spinlock 32-bit word
 *
 * *,1,0 -> *,0,1
 */
static __always_inline void
clear_pending_set_locked(struct qspinlock *lock, u32 val)
{
	u32 new, old;

	for (;;) {
		new = (val & ~_Q_PENDING_MASK) | _Q_LOCKED_VAL;

		old = atomic_cmpxchg(&lock->val, val, new);
		if (old == val)
			break;

		val = old;
	}
}

/**
 * xchg_tail - Put in the new queue tail code word & retrieve previous one
 * @lock : Pointer to queue spinlock structure
 * @tail : The new queue tail code word
 * @pval : Pointer to current value of the queue spinlock 32-bit word
 * Return: The previous queue tail code word
 *
 * xchg(lock, tail)
 *
 * p,*,* -> n,*,* ; prev = xchg(lock, node)
 */
static __always_inline u32
xchg_tail(struct qspinlock *lock, u32 tail, u32 *pval)
{
	u32 old, new, val = *pval;

	for (;;) {
		new = (val & _Q_LOCKED_PENDING_MASK) | tail;
		old = atomic_cmpxchg(&lock->val, val, new);
		if (old == val)
			break;

		val = old;
	}
	*pval = new;
	return old;
}
#endif /* _Q_PENDING_BITS == 8 */

/**
 * trylock_pending - try to acquire queue spinlock using the pending bit
 * @lock : Pointer to queue spinlock structure
 * @pval : Pointer to value of the queue spinlock 32-bit word
 * Return: 1 if lock acquired, 0 otherwise
 *
 * The pending bit won't be set as soon as one or more tasks queue up.
 * This function should only be called when lock stealing will not happen.
 * Otherwise, it has to be disabled.
 */
static inline int trylock_pending(struct qspinlock *lock, u32 *pval)
{
	u32 old, new, val = *pval;
	int retry = 1;

	/*
	 * trylock || pending
	 *
	 * 0,0,0 -> 0,0,1 ; trylock
	 * 0,0,1 -> 0,1,1 ; pending
	 */
	for (;;) {
		/*
		 * If we observe that the queue is not empty,
		 * return and be queued.
		 */
		if (val & _Q_TAIL_MASK)
			return 0;

		if ((val & _Q_LOCKED_PENDING_MASK) ==
		    (_Q_LOCKED_VAL|_Q_PENDING_VAL)) {
			/*
			 * If both the lock and pending bits are set, we wait
			 * a while to see if that either bit will be cleared.
			 * If that is no change, we return and be queued.
			 */
			if (!retry)
				return 0;
			retry--;
			cpu_relax();
			cpu_relax();
			*pval = val = atomic_read(&lock->val);
			continue;
		} else if ((val & _Q_LOCKED_PENDING_MASK) == _Q_PENDING_VAL) {
			/*
			 * Pending bit is set, but not the lock bit.
			 * Assuming that the pending bit holder is going to
			 * set the lock bit and clear the pending bit soon,
			 * it is better to wait than to exit at this point.
			 */
			cpu_relax();
			*pval = val = atomic_read(&lock->val);
			continue;
		}

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
	 *
	 * this wait loop must be a load-acquire such that we match the
	 * store-release that clears the locked bit and create lock
	 * sequentiality; this because not all try_clear_pending_set_locked()
	 * implementations imply full barriers.
	 */
	while ((val = smp_load_acquire(&lock->val.counter)) & _Q_LOCKED_MASK)
		arch_mutex_cpu_relax();

	/*
	 * take ownership and clear the pending bit.
	 *
	 * *,1,0 -> *,0,1
	 */
	clear_pending_set_locked(lock, val);
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
	 * We touched a (possibly) cold cacheline; attempt the trylock once
	 * more in the hope someone let go while we weren't watching as long
	 * as no one was queuing.
	 */
	if (!(val & _Q_TAIL_MASK) && queue_spin_trylock(lock))
		goto release;

	/*
	 * we already touched the queueing cacheline; don't bother with pending
	 * stuff.
	 *
	 * p,*,* -> n,*,*
	 */
	old = xchg_tail(lock, tail, &val);

	/*
	 * if there was a previous node; link it and wait.
	 */
	if (old & _Q_TAIL_MASK) {
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
