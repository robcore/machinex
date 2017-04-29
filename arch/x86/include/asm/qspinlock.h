#ifndef _ASM_X86_QSPINLOCK_H
#define _ASM_X86_QSPINLOCK_H

#include <asm-generic/qspinlock_types.h>

#if !defined(CONFIG_X86_OOSTORE) && !defined(CONFIG_X86_PPRO_FENCE)

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

#endif /* _ASM_X86_QSPINLOCK_H */
