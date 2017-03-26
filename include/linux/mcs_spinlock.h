/*
 * MCS lock defines
 *
 * This file contains the main data structure and API definitions of MCS lock.
 *
 * The MCS lock (proposed by Mellor-Crummey and Scott) is a simple spin-lock
 * with the desirable properties of being fair, and with each cpu trying
 * to acquire the lock spinning on a local variable.
 * It avoids expensive cache bouncings that common test-and-set spin-lock
 * implementations incur.
 */
#ifndef __LINUX_MCS_SPINLOCK_H
#define __LINUX_MCS_SPINLOCK_H

struct mcs_spinlock {
	struct mcs_spinlock *next;
	int locked; /* 1 if lock acquired */
};

/*
 * Note: the smp_load_acquire/smp_store_release pair is not
 * sufficient to form a full memory barrier across
 * cpus for many architectures (except x86) for mcs_unlock and mcs_lock.
 * For applications that need a full barrier across multiple cpus
 * with mcs_unlock and mcs_lock pair, smp_mb__after_unlock_lock() should be
 * used after mcs_lock.
 */
static inline
void mcs_spin_lock(struct mcs_spinlock **lock, struct mcs_spinlock *node)
{
	struct mcs_spinlock *prev;

	/* Init node */
	node->locked = 0;
	node->next   = NULL;

	prev = xchg(lock, node);
	if (likely(prev == NULL)) {
		/* Lock acquired */
		node->locked = 1;
		return;
	}
	ACCESS_ONCE(prev->next) = node;
	/*
	 * Wait until the lock holder passes the lock down.
	 * Using smp_load_acquire() provides a memory barrier that
	 * ensures subsequent operations happen after the lock is acquired.
	 */
	while (!(smp_load_acquire(&node->locked)))
		arch_mutex_cpu_relax();
}

static inline
void mcs_spin_unlock(struct mcs_spinlock **lock, struct mcs_spinlock *node)
{
	struct mcs_spinlock *next = ACCESS_ONCE(node->next);

	if (likely(!next)) {
		/*
		 * Release the lock by setting it to NULL
		 */
		if (cmpxchg(lock, node, NULL) == node)
			return;
		/* Wait until the next pointer is set */
		while (!(next = ACCESS_ONCE(node->next)))
			arch_mutex_cpu_relax();
	}
	/*
	 * Pass lock to next waiter.
	 * smp_store_release() provides a memory barrier to ensure
	 * all operations in the critical section has been completed
	 * before unlocking.
	 */
	smp_store_release(&next->locked, 1);
}

#endif /* __LINUX_MCS_SPINLOCK_H */
