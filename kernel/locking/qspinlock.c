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
 * To have additional features for better virtualization support, it is
 * necessary to store additional data in the queue node structure. So
 * a new queue node structure will have to be defined and used here.
 */
struct qnode {
	struct mcs_spinlock mcs;
#ifdef CONFIG_PARAVIRT_UNFAIR_LOCKS
	int		lsteal_mask;	/* Lock stealing frequency mask	*/
	u32		prev_tail;	/* Tail code of previous node	*/
	struct qnode   *qprev;		/* Previous queue node addr	*/
#endif
};
#define qhead	mcs.locked	/* The queue head flag */

/*
 * Per-CPU queue node structures; we can never have more than 4 nested
 * contexts: task, softirq, hardirq, nmi.
 *
 * Exactly fits one cacheline.
 */
static DEFINE_PER_CPU_ALIGNED(struct qnode, qnodes[4]);

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

static inline struct qnode *decode_tail(u32 tail)
{
	int cpu = (tail >> _Q_TAIL_CPU_OFFSET) - 1;
	int idx = (tail &  _Q_TAIL_IDX_MASK) >> _Q_TAIL_IDX_OFFSET;

	return per_cpu_ptr(&qnodes[idx], cpu);
}

#define _Q_LOCKED_PENDING_MASK	(_Q_LOCKED_MASK | _Q_PENDING_MASK)

/*
 * By using the whole 2nd least significant byte for the pending bit, we
 * can allow better optimization of the lock acquisition for the pending
 * bit holder.
 */
struct __qspinlock {
	union {
		atomic_t val;
#ifdef __LITTLE_ENDIAN
		u8	 locked;
		struct {
			u16	locked_pending;
			u16	tail;
		};
#else
		struct {
			u16	tail;
			u16	locked_pending;
		};
		struct {
			u8	reserved[3];
			u8	locked;
		};
#endif
	};
};

#if _Q_PENDING_BITS == 8
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

/*
 ************************************************************************
 * Inline functions for supporting unfair queue lock			*
 ************************************************************************
 */
/*
 * Unfair lock support in a virtualized guest
 *
 * An unfair lock can be implemented using a simple test-and-set lock like
 * what is being done in a read-write lock. This simple scheme has 2 major
 * problems:
 *  1) It needs constant reading and occasionally writing to the lock word
 *     thus putting a lot of cacheline contention traffic on the affected
 *     cacheline.
 *  2) Lock starvation is a real possibility especially if the number of
 *     virtual CPUs is large.
 *
 * To reduce the undesirable side effects of an unfair lock, the queue
 * unfair spinlock implements a more elaborate scheme.  Lock stealing is
 * allowed in the following places:
 *  1) In the spin_lock and spin_trylock fastpaths
 *  2) When spinning in the waiter queue before becoming the queue head
 *
 * A lock acquirer has only one chance of stealing the lock in the spin_lock
 * and spin_trylock fastpath. If the attempt fails for spin_lock, the task
 * will be queued in the wait queue.
 *
 * Even in the wait queue, the task can still attempt to steal the lock
 * periodically at a frequency about inversely and logarithmically proportional
 * to its distance from the queue head. In other word, the closer it is to
 * the queue head, the higher a chance it has of stealing the lock. This
 * scheme reduces the load on the lock cacheline while trying to maintain
 * a somewhat FIFO way of getting the lock so as to reduce the chance of lock
 * starvation.
 */
#ifdef CONFIG_PARAVIRT_UNFAIR_LOCKS
#define DEF_LOOP_CNT(c)		int c = 0
#define INC_LOOP_CNT(c)		(c)++
#define LOOP_CNT(c)		c
#define LSTEAL_MIN		(1 << 3)
#define LSTEAL_MAX		(1 << 10)
#define LSTEAL_MIN_MASK		(LSTEAL_MIN - 1)
#define LSTEAL_MAX_MASK		(LSTEAL_MAX - 1)

/**
 * unfair_init_vars - initialize unfair relevant fields in queue node structure
 * @node: Current queue node address
 */
static inline void unfair_init_vars(struct qnode *node)
{
	node->qprev	  = NULL;
	node->prev_tail   = 0;
	node->lsteal_mask = LSTEAL_MIN_MASK;
}

/**
 * unfair_set_vars - set unfair related fields in the queue node structure
 * @node     : Current queue node address
 * @prev     : Previous queue node address
 * @prev_tail: Previous tail code
 */
static inline void
unfair_set_vars(struct qnode *node, struct qnode *prev, u32 prev_tail)
{
	if (!static_key_false(&paravirt_unfairlocks_enabled))
		return;

	node->qprev	= prev;
	node->prev_tail = prev_tail;
	/*
	 * This node will spin double the number of time of the previous node
	 * before attempting to steal the lock until it reaches a maximum.
	 */
	node->lsteal_mask = prev->qhead ? LSTEAL_MIN_MASK :
			    (prev->lsteal_mask << 1) + 1;
	if (node->lsteal_mask > LSTEAL_MAX_MASK)
		node->lsteal_mask = LSTEAL_MAX_MASK;
	/* Make sure the new fields are visible to others */
	smp_wmb();
}

/**
 * unfair_get_lock - try to steal the lock periodically
 * @lock : Pointer to queue spinlock structure
 * @node : Current queue node address
 * @tail : My tail code value
 * @count: Loop count
 * Return: true if the lock has been stolen, false otherwise
 *
 * When a true value is returned, the caller will have to notify the next
 * node only if the qhead flag is set and the next pointer in the queue
 * node is not NULL.
 */
static noinline int
unfair_get_lock(struct qspinlock *lock, struct qnode *node, u32 tail, int count)
{
	u32	     prev_tail;
	int	     isqhead;
	struct qnode *next;

	if (!static_key_false(&paravirt_unfairlocks_enabled) ||
	   ((count & node->lsteal_mask) != node->lsteal_mask))
		return false;

	if (!queue_spin_trylock_unfair(lock)) {
		/*
		 * Lock stealing fails, re-adjust the lsteal mask so that
		 * it is about double of the previous node.
		 */
		struct qnode *prev = node->qprev;

		node->lsteal_mask = prev->qhead ? LSTEAL_MIN_MASK :
				    (prev->lsteal_mask << 1) + 1;
		if (node->lsteal_mask > LSTEAL_MAX_MASK)
			node->lsteal_mask = LSTEAL_MAX_MASK;
		return false;
	}
	queue_spin_unlock(lock);
	return false;
}

#else /* CONFIG_PARAVIRT_UNFAIR_LOCKS */
#define	DEF_LOOP_CNT(c)
#define	INC_LOOP_CNT(c)
#define	LOOP_CNT(c)	0

static void unfair_init_vars(struct qnode *node)	{}
static void unfair_set_vars(struct qnode *node, struct qnode *prev,
		u32 prev_tail)				{}
static int unfair_get_lock(struct qspinlock *lock, struct qnode *node,
		u32 tail, int count)			{ return false; }
#endif /* CONFIG_PARAVIRT_UNFAIR_LOCKS */

/**
 * get_qlock - Set the lock bit and own the lock
 * @lock : Pointer to queue spinlock structure
 * Return: 1 if lock acquired, 0 otherwise
 *
 * This routine should only be called when the caller is the only one
 * entitled to acquire the lock.
 */
static __always_inline int get_qlock(struct qspinlock *lock)
{
	struct __qspinlock *l = (void *)lock;

#ifdef CONFIG_PARAVIRT_UNFAIR_LOCKS
	if (static_key_false(&paravirt_unfairlocks_enabled))
		/*
		 * Need to use atomic operation to get the lock when
		 * lock stealing can happen.
		 */
		return cmpxchg(&l->locked, 0, _Q_LOCKED_VAL) == 0;
#endif
	barrier();
	ACCESS_ONCE(l->locked) = _Q_LOCKED_VAL;
	barrier();
	return 1;
}

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
 * queue_spin_lock_slowerpath - a slower path for acquiring queue spinlock
 * @lock: Pointer to queue spinlock structure
 * @node: Pointer to the queue node
 * @tail: The tail code
 *
 * The reason for splitting a slowerpath from slowpath is to avoid the
 * unnecessary overhead of non-scratch pad register pushing and popping
 * due to increased complexity with unfair and PV spinlock from slowing
 * down the nominally faster pending bit and trylock code path. So this
 * function is not inlined.
 */
static noinline void
queue_spin_lock_slowerpath(struct qspinlock *lock, struct qnode *node, u32 tail)
{
	struct qnode *prev, *next;
	u32 old, val;

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
		DEF_LOOP_CNT(cnt);

		prev = decode_tail(old);
		unfair_set_vars(node, prev, old);
		ACCESS_ONCE(prev->mcs.next) = (struct mcs_spinlock *)node;

		while (!smp_load_acquire(&node->qhead)) {
			INC_LOOP_CNT(cnt);
			unfair_get_lock(lock, node, tail, LOOP_CNT(cnt));
			arch_mutex_cpu_relax();
		}
	}

	/*
	 * we're at the head of the waitqueue, wait for the owner & pending to
	 * go away.
	 * Load-acquired is used here because the get_qlock()
	 * function below may not be a full memory barrier.
	 *
	 * *,x,y -> *,0,0
	 */
retry_queue_wait:
	while ((val = smp_load_acquire(&lock->val.counter))
				       & _Q_LOCKED_PENDING_MASK)
		arch_mutex_cpu_relax();

	/*
	 * claim the lock:
	 *
	 * n,0,0 -> 0,0,1 : lock, uncontended
	 * *,0,0 -> *,0,1 : lock, contended
	 *
	 * If the queue head is the only one in the queue (lock value == tail),
	 * clear the tail code and grab the lock. Otherwise, we only need
	 * to grab the lock.
	 */
	for (;;) {
		if (val != tail) {
			/*
			 * The get_qlock function will only failed if the
			 * lock was stolen.
			 */
			if (get_qlock(lock))
				break;
			else
				goto retry_queue_wait;
		}
		old = atomic_cmpxchg(&lock->val, val, _Q_LOCKED_VAL);
		if (old == val)
			return;	/* No contention */
		else if (old & _Q_LOCKED_MASK)
			goto retry_queue_wait;

		val = old;
	}

	/*
	 * contended path; wait for next, return.
	 */
	while (!(next = (struct qnode *)ACCESS_ONCE(node->mcs.next)))
		arch_mutex_cpu_relax();

	arch_mcs_spin_unlock_contended(&next->qhead);
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
 * This slowpath only contains the faster pending bit and trylock codes.
 * The slower queuing code is in the slowerpath function.
 */
void queue_spin_lock_slowpath(struct qspinlock *lock, u32 val)
{
	struct qnode *node;
	u32 tail, idx;

	BUILD_BUG_ON(CONFIG_NR_CPUS >= (1U << _Q_TAIL_CPU_BITS));

	if (trylock_pending(lock, &val))
		return;	/* Lock acquired */

	node = this_cpu_ptr(&qnodes[0]);
	idx = node->mcs.count++;
	tail = encode_tail(smp_processor_id(), idx);

	node += idx;
	node->qhead = 0;
	node->mcs.next = NULL;
	unfair_init_vars(node);

	/*
	 * We touched a (possibly) cold cacheline; attempt the trylock once
	 * more in the hope someone let go while we weren't watching as long
	 * as no one was queuing.
	 */
	if ((val & _Q_TAIL_MASK) || !queue_spin_trylock(lock))
		queue_spin_lock_slowerpath(lock, node, tail);

	/*
	 * release the node
	 */
	this_cpu_dec(qnodes[0].mcs.count);
}
EXPORT_SYMBOL(queue_spin_lock_slowpath);
