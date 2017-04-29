#ifndef _ASM_X86_QSPINLOCK_H
#define _ASM_X86_QSPINLOCK_H

#include <asm-generic/qspinlock_types.h>

#if !defined(CONFIG_X86_OOSTORE) && !defined(CONFIG_X86_PPRO_FENCE)

#ifdef CONFIG_PARAVIRT_UNFAIR_LOCKS
extern struct static_key paravirt_unfairlocks_enabled;
#endif

#define	queue_spin_unlock queue_spin_unlock
/**
 * queue_spin_unlock - release a queue spinlock
 * @lock : Pointer to queue spinlock structure
 *
 * No special memory barrier other than a compiler one is needed for the
 * x86 architecture. A compiler barrier is added at the end to make sure
 * that the clearing the lock bit is done ASAP without artificial delay
 * due to compiler optimization.
 */
static inline void queue_spin_unlock(struct qspinlock *lock)
{
	barrier();
	ACCESS_ONCE(*(u8 *)lock) = 0;
	barrier();
}

#endif /* !CONFIG_X86_OOSTORE && !CONFIG_X86_PPRO_FENCE */

#include <asm-generic/qspinlock.h>

union arch_qspinlock {
	atomic_t val;
	u8	 locked;
};

#ifdef CONFIG_PARAVIRT_UNFAIR_LOCKS
/**
 * queue_spin_trylock_unfair - try to acquire the queue spinlock unfairly
 * @lock : Pointer to queue spinlock structure
 * Return: 1 if lock acquired, 0 if failed
 */
static __always_inline int queue_spin_trylock_unfair(struct qspinlock *lock)
{
	union arch_qspinlock *qlock = (union arch_qspinlock *)lock;

	if (!qlock->locked && (cmpxchg(&qlock->locked, 0, _Q_LOCKED_VAL) == 0))
		return 1;
	return 0;
}

/**
 * queue_spin_lock_unfair - acquire a queue spinlock unfairly
 * @lock: Pointer to queue spinlock structure
 */
static __always_inline void queue_spin_lock_unfair(struct qspinlock *lock)
{
	union arch_qspinlock *qlock = (union arch_qspinlock *)lock;

	if (likely(cmpxchg(&qlock->locked, 0, _Q_LOCKED_VAL) == 0))
		return;
	/*
	 * Since the lock is now unfair, we should not activate the 2-task
	 * pending bit spinning code path which disallows lock stealing.
	 */
	queue_spin_lock_slowpath(lock, -1);
}

/*
 * Redefine arch_spin_lock and arch_spin_trylock as inline functions that will
 * jump to the unfair versions if the static key paravirt_unfairlocks_enabled
 * is true.
 */
#undef arch_spin_lock
#undef arch_spin_trylock
#undef arch_spin_lock_flags

/**
 * arch_spin_lock - acquire a queue spinlock
 * @lock: Pointer to queue spinlock structure
 */
static inline void arch_spin_lock(struct qspinlock *lock)
{
	if (static_key_false(&paravirt_unfairlocks_enabled))
		queue_spin_lock_unfair(lock);
	else
		queue_spin_lock(lock);
}

/**
 * arch_spin_trylock - try to acquire the queue spinlock
 * @lock : Pointer to queue spinlock structure
 * Return: 1 if lock acquired, 0 if failed
 */
static inline int arch_spin_trylock(struct qspinlock *lock)
{
	if (static_key_false(&paravirt_unfairlocks_enabled))
		return queue_spin_trylock_unfair(lock);
	else
		return queue_spin_trylock(lock);
}

#define arch_spin_lock_flags(l, f)	arch_spin_lock(l)

#endif /* CONFIG_PARAVIRT_UNFAIR_LOCKS */

#endif /* _ASM_X86_QSPINLOCK_H */
