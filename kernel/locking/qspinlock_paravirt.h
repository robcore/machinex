#ifndef _GEN_PV_LOCK_SLOWPATH
#error "do not include this file"
#endif

#include <linux/hash.h>
#include <linux/bootmem.h>
#include <linux/debug_locks.h>

/*
 * Implement paravirt qspinlocks; the general idea is to halt the vcpus instead
 * of spinning them.
 *
 * This relies on the architecture to provide two paravirt hypercalls:
 *
 *   pv_wait(u8 *ptr, u8 val) -- suspends the vcpu if *ptr == val
 *   pv_kick(cpu)             -- wakes a suspended vcpu
 *
 * Using these we implement __pv_queued_spin_lock_slowpath() and
 * __pv_queued_spin_unlock() to replace native_queued_spin_lock_slowpath() and
 * native_queued_spin_unlock().
 */

#define _Q_SLOW_VAL	(3U << _Q_LOCKED_OFFSET)

enum vcpu_state {
	vcpu_running = 0,
	vcpu_halted,
};

struct pv_node {
	struct mcs_spinlock	mcs;
	struct mcs_spinlock	__res[3];

	int			cpu;
	u8			state;
};

/*
 * Lock and MCS node addresses hash table for fast lookup
 *
 * Hashing is done on a per-cacheline basis to minimize the need to access
 * more than one cacheline.
 *
 * Dynamically allocate a hash table big enough to hold at least 4X the
 * number of possible cpus in the system. Allocation is done on page
 * granularity. So the minimum number of hash buckets should be at least
 * 256 (64-bit) or 512 (32-bit) to fully utilize a 4k page.
 *
 * Since we should not be holding locks from NMI context (very rare indeed) the
 * max load factor is 0.75, which is around the point where open addressing
 * breaks down.
 *
 */
struct pv_hash_entry {
	struct qspinlock *lock;
	struct pv_node   *node;
};

#define PV_HE_PER_LINE	(SMP_CACHE_BYTES / sizeof(struct pv_hash_entry))
#define PV_HE_MIN	(PAGE_SIZE / sizeof(struct pv_hash_entry))

static struct pv_hash_entry *pv_lock_hash;
static unsigned int pv_lock_hash_bits __read_mostly;

/*
 * Allocate memory for the PV qspinlock hash buckets
 *
 * This function should be called from the paravirt spinlock initialization
 * routine.
 */
void __init __pv_init_lock_hash(void)
{
	int pv_hash_size = ALIGN(4 * num_possible_cpus(), PV_HE_PER_LINE);

	if (pv_hash_size < PV_HE_MIN)
		pv_hash_size = PV_HE_MIN;

	/*
	 * Allocate space from bootmem which should be page-size aligned
	 * and hence cacheline aligned.
	 */
	pv_lock_hash = alloc_large_system_hash("PV qspinlock",
					       sizeof(struct pv_hash_entry),
					       pv_hash_size, 0, HASH_EARLY,
					       &pv_lock_hash_bits, NULL,
					       pv_hash_size, pv_hash_size);
}

#define for_each_hash_entry(he, offset, hash)						\
	for (hash &= ~(PV_HE_PER_LINE - 1), he = &pv_lock_hash[hash], offset = 0;	\
	     offset < (1 << pv_lock_hash_bits);						\
	     offset++, he = &pv_lock_hash[(hash + offset) & ((1 << pv_lock_hash_bits) - 1)])

static struct qspinlock **pv_hash(struct qspinlock *lock, struct pv_node *node)
{
	unsigned long offset, hash = hash_ptr(lock, pv_lock_hash_bits);
	struct pv_hash_entry *he;

	for_each_hash_entry(he, offset, hash) {
		if (!cmpxchg(&he->lock, NULL, lock)) {
			WRITE_ONCE(he->node, node);
			return &he->lock;
		}
	}
	/*
	 * Hard assume there is a free entry for us.
	 *
	 * This is guaranteed by ensuring every blocked lock only ever consumes
	 * a single entry, and since we only have 4 nesting levels per CPU
	 * and allocated 4*nr_possible_cpus(), this must be so.
	 *
	 * The single entry is guaranteed by having the lock owner unhash
	 * before it releases.
	 */
	BUG();
}

static struct pv_node *pv_unhash(struct qspinlock *lock)
{
	unsigned long offset, hash = hash_ptr(lock, pv_lock_hash_bits);
	struct pv_hash_entry *he;
	struct pv_node *node;

	for_each_hash_entry(he, offset, hash) {
		if (READ_ONCE(he->lock) == lock) {
			node = READ_ONCE(he->node);
			WRITE_ONCE(he->lock, NULL);
			return node;
		}
	}
	/*
	 * Hard assume we'll find an entry.
	 *
	 * This guarantees a limited lookup time and is itself guaranteed by
	 * having the lock owner do the unhash -- IFF the unlock sees the
	 * SLOW flag, there MUST be a hash entry.
	 */
	BUG();
}

/*
 * Initialize the PV part of the mcs_spinlock node.
 */
static void pv_init_node(struct mcs_spinlock *node)
{
	struct pv_node *pn = (struct pv_node *)node;

	BUILD_BUG_ON(sizeof(struct pv_node) > 5*sizeof(struct mcs_spinlock));

	pn->cpu = smp_processor_id();
	pn->state = vcpu_running;
}

/*
 * Wait for node->locked to become true, halt the vcpu after a short spin.
 * pv_kick_node() is used to wake the vcpu again.
 */
static void pv_wait_node(struct mcs_spinlock *node)
{
	struct pv_node *pn = (struct pv_node *)node;
	int loop;

	for (;;) {
		for (loop = SPIN_THRESHOLD; loop; loop--) {
			if (READ_ONCE(node->locked))
				return;
			cpu_relax();
		}

		/*
		 * Order pn->state vs pn->locked thusly:
		 *
		 * [S] pn->state = vcpu_halted	  [S] next->locked = 1
		 *     MB			      MB
		 * [L] pn->locked		[RmW] pn->state = vcpu_running
		 *
		 * Matches the xchg() from pv_kick_node().
		 */
		smp_store_mb(pn->state, vcpu_halted);

		if (!READ_ONCE(node->locked))
			pv_wait(&pn->state, vcpu_halted);

		/*
		 * Reset the vCPU state to avoid unncessary CPU kicking
		 */
		WRITE_ONCE(pn->state, vcpu_running);

		/*
		 * If the locked flag is still not set after wakeup, it is a
		 * spurious wakeup and the vCPU should wait again. However,
		 * there is a pretty high overhead for CPU halting and kicking.
		 * So it is better to spin for a while in the hope that the
		 * MCS lock will be released soon.
		 */
	}
	/*
	 * By now our node->locked should be 1 and our caller will not actually
	 * spin-wait for it. We do however rely on our caller to do a
	 * load-acquire for us.
	 */
}

/*
 * Called after setting next->locked = 1, used to wake those stuck in
 * pv_wait_node().
 */
static void pv_kick_node(struct mcs_spinlock *node)
{
	struct pv_node *pn = (struct pv_node *)node;

	/*
	 * Note that because node->locked is already set, this actual
	 * mcs_spinlock entry could be re-used already.
	 *
	 * This should be fine however, kicking people for no reason is
	 * harmless.
	 *
	 * See the comment in pv_wait_node().
	 */
	if (xchg(&pn->state, vcpu_running) == vcpu_halted)
		pv_kick(pn->cpu);
}

/*
 * Wait for l->locked to become clear; halt the vcpu after a short spin.
 * __pv_queued_spin_unlock() will wake us.
 */
static void pv_wait_head(struct qspinlock *lock, struct mcs_spinlock *node)
{
	struct pv_node *pn = (struct pv_node *)node;
	struct __qspinlock *l = (void *)lock;
	struct qspinlock **lp = NULL;
	int loop;

	for (;;) {
		for (loop = SPIN_THRESHOLD; loop; loop--) {
			if (!READ_ONCE(l->locked))
				return;
			cpu_relax();
		}

		WRITE_ONCE(pn->state, vcpu_halted);
		if (!lp) { /* ONCE */
			lp = pv_hash(lock, pn);
			/*
			 * lp must be set before setting _Q_SLOW_VAL
			 *
			 * [S] lp = lock                [RmW] l = l->locked = 0
			 *     MB                             MB
			 * [S] l->locked = _Q_SLOW_VAL  [L]   lp
			 *
			 * Matches the cmpxchg() in __pv_queued_spin_unlock().
			 */
			if (!cmpxchg(&l->locked, _Q_LOCKED_VAL, _Q_SLOW_VAL)) {
				/*
				 * The lock is free and _Q_SLOW_VAL has never
				 * been set. Therefore we need to unhash before
				 * getting the lock.
				 */
				WRITE_ONCE(*lp, NULL);
				return;
			}
		}
		pv_wait(&l->locked, _Q_SLOW_VAL);

		/*
		 * The unlocker should have freed the lock before kicking the
		 * CPU. So if the lock is still not free, it is a spurious
		 * wakeup and so the vCPU should wait again after spinning for
		 * a while.
		 */
	}

	/*
	 * Lock is unlocked now; the caller will acquire it without waiting.
	 * As with pv_wait_node() we rely on the caller to do a load-acquire
	 * for us.
	 */
}

/*
 * PV version of the unlock function to be used in stead of
 * queued_spin_unlock().
 */
__visible void __pv_queued_spin_unlock(struct qspinlock *lock)
{
	struct __qspinlock *l = (void *)lock;
	struct pv_node *node;
	u8 lockval = cmpxchg(&l->locked, _Q_LOCKED_VAL, 0);

	/*
	 * We must not unlock if SLOW, because in that case we must first
	 * unhash. Otherwise it would be possible to have multiple @lock
	 * entries, which would be BAD.
	 */
	if (likely(lockval == _Q_LOCKED_VAL))
		return;

	if (unlikely(lockval != _Q_SLOW_VAL)) {
		if (debug_locks_silent)
			return;
		WARN(1, "pvqspinlock: lock %p has corrupted value 0x%x!\n", lock, atomic_read(&lock->val));
		return;
	}

	/*
	 * Since the above failed to release, this must be the SLOW path.
	 * Therefore start by looking up the blocked node and unhashing it.
	 */
	node = pv_unhash(lock);

	/*
	 * Now that we have a reference to the (likely) blocked pv_node,
	 * release the lock.
	 */
	smp_store_release(&l->locked, 0);

	/*
	 * At this point the memory pointed at by lock can be freed/reused,
	 * however we can still use the pv_node to kick the CPU.
	 */
	if (READ_ONCE(node->state) == vcpu_halted)
		pv_kick(node->cpu);
}
/*
 * Include the architecture specific callee-save thunk of the
 * __pv_queued_spin_unlock(). This thunk is put together with
 * __pv_queued_spin_unlock() near the top of the file to make sure
 * that the callee-save thunk and the real unlock function are close
 * to each other sharing consecutive instruction cachelines.
 */
#include <asm/qspinlock_paravirt.h>

