#ifndef _ASM_X86_PVQSPINLOCK_H
#define _ASM_X86_PVQSPINLOCK_H

/*
 *	Queue Spinlock Para-Virtualization (PV) Support
 *
 *	+------+	    +-----+   next     +----+
 *	| Lock |	    |Queue|----------->|Next|
 *	|Holder|<-----------|Head |<-----------|Node|
 *	+------+ prev_tail  +-----+ prev_tail  +----+
 *
 * The PV support code for queue spinlock is roughly the same as that
 * of the ticket spinlock. Each CPU waiting for the lock will spin until it
 * reaches a threshold. When that happens, it will put itself to halt so
 * that the hypervisor can reuse the CPU cycles in some other guests as well
 * as returning other hold-up CPUs faster.
 *
 * A major difference between the two versions of PV spinlock is the fact
 * that the spin threshold of the queue spinlock is half of that of the
 * ticket spinlock. However, the queue head will spin twice as long as the
 * other nodes before it puts itself to halt. The reason for that is to
 * increase halting chance of heavily contended locks to favor lightly
 * contended locks (queue depth of 1 or less).
 *
 * There are 2 places where races can happen:
 *  1) Halting of the queue head CPU (in pv_head_spin_check) and the CPU
 *     kicking by the lock holder in the unlock path (in pv_kick_node).
 *  2) Halting of the queue node CPU (in pv_queue_spin_check) and the
 *     the status check by the previous queue head (in pv_halt_check).
 * See the comments on those functions to see how the races are being
 * addressed.
 */

/*
 * Spin threshold for queue spinlock
 */
#define	QSPIN_THRESHOLD		(1U<<14)
#define MAYHALT_THRESHOLD	(QSPIN_THRESHOLD - 0x10)

/*
 * CPU state flags
 */
#define PV_CPU_ACTIVE	1	/* This CPU is active		 */
#define PV_CPU_KICKED   2	/* This CPU is being kicked	 */
#define PV_CPU_HALTED	-1	/* This CPU is halted		 */

/*
 * Additional fields to be added to the qnode structure
 */
#if CONFIG_NR_CPUS >= (1 << 16)
#define _cpuid_t	u32
#else
#define _cpuid_t	u16
#endif

struct qnode;

struct pv_qvars {
	s8	      cpustate;		/* CPU status flag		*/
	s8	      mayhalt;		/* May be halted soon		*/
	_cpuid_t      mycpu;		/* CPU number of this node	*/
	struct qnode *prev;		/* Pointer to previous node	*/
};

/*
 * Macro to be used by the unfair lock code to access the previous node pointer
 * in the pv structure.
 */
#define qprev	pv.prev

/**
 * pv_init_vars - initialize fields in struct pv_qvars
 * @pv : pointer to struct pv_qvars
 * @cpu: current CPU number
 */
static __always_inline void pv_init_vars(struct pv_qvars *pv, int cpu)
{
	pv->cpustate = PV_CPU_ACTIVE;
	pv->prev     = NULL;
	pv->mayhalt  = false;
	pv->mycpu    = cpu;
}

/**
 * pv_head_spin_check - perform para-virtualization checks for queue head
 * @pv    : pointer to struct pv_qvars
 * @count : loop count
 * @qcode : queue code of the supposed lock holder
 * @lock  : pointer to the qspinlock structure
 *
 * The following checks will be done:
 * 1) If it gets a kick signal, reset loop count and flag
 * 2) Halt itself if lock is still not available after QSPIN_THRESHOLD
 */
static __always_inline void pv_head_spin_check(struct pv_qvars *pv, int *count,
				u32 qcode, struct qspinlock *lock)
{
	if (!static_key_false(&paravirt_spinlocks_enabled))
		return;

	if (pv->cpustate == PV_CPU_KICKED) {
		/*
		 * Reset count and flag
		 */
		*count	     = 0;
		pv->cpustate = PV_CPU_ACTIVE;

	} else if (unlikely(*count >= 2*QSPIN_THRESHOLD)) {
		u8 lockval;
		s8 oldstate;

		/*
		 * Set the lock byte to _Q_LOCKED_SLOWPATH before
		 * trying to halt itself. It is possible that the
		 * lock byte had been set to _Q_LOCKED_SLOWPATH
		 * already (spurious wakeup of queue head after a halt
		 * or opportunistic setting in pv_halt_check()).
		 * In this case, just proceeds to sleeping.
		 *
		 *     queue head		    lock holder
		 *     ----------		    -----------
		 *     cpustate = PV_CPU_HALTED
		 * [1] cmpxchg(_Q_LOCKED_VAL	[2] cmpxchg(_Q_LOCKED_VAL => 0)
		 * => _Q_LOCKED_SLOWPATH)	    if (cmpxchg fails &&
		 *     if (cmpxchg succeeds)	    cpustate == PV_CPU_HALTED)
		 *        halt()		       kick()
		 *
		 * Sequence:
		 * 1,2 - slowpath flag set, queue head halted & lock holder
		 *	 will call slowpath
		 * 2,1 - queue head cmpxchg fails, halt is aborted
		 *
		 * If the queue head CPU is woken up by a spurious interrupt
		 * at the same time as the lock holder check the cpustate,
		 * it is possible that the lock holder will try to kick
		 * the queue head CPU which isn't halted.
		 */
		oldstate = cmpxchg(&pv->cpustate, PV_CPU_ACTIVE, PV_CPU_HALTED);
		if (oldstate == PV_CPU_KICKED)
			goto reset;	/* Reset count and state */

		lockval = cmpxchg((u8 *)lock,
				  _Q_LOCKED_VAL, _Q_LOCKED_SLOWPATH);
		if (lockval != 0) {
			__queue_halt_cpu(PV_HALT_QHEAD, &pv->cpustate,
					 PV_CPU_HALTED);
			__queue_lockstat((pv->cpustate == PV_CPU_KICKED)
					? PV_WAKE_KICKED : PV_WAKE_SPURIOUS);
		}
		/*
		 * Else, the lock is free and no halting is needed
		 */
reset:
		ACCESS_ONCE(pv->cpustate) = PV_CPU_ACTIVE;
		*count = 0;	/* Reset count */
	}
}

/**
 * pv_queue_spin_check - perform para-virtualization checks for queue member
 * @pv   : pointer to struct pv_qvars
 * @count: loop count
 */
static __always_inline void
pv_queue_spin_check(struct pv_qvars *pv, struct mcs_spinlock *mcs, int *count)
{
	if (!static_key_false(&paravirt_spinlocks_enabled))
		return;
	/*
	 * Attempt to halt oneself after QSPIN_THRESHOLD spins
	 */
	if (unlikely(*count >= QSPIN_THRESHOLD)) {
		/*
		 * Time to halt itself
		 */
		ACCESS_ONCE(pv->cpustate) = PV_CPU_HALTED;
		/*
		 * One way to avoid the racing between pv_halt_check()
		 * and pv_queue_spin_check() is to use memory barrier or
		 * atomic instruction to synchronize between the two competing
		 * threads. However, that will slow down the queue spinlock
		 * slowpath. One way to eliminate this overhead for normal
		 * cases is to use another flag (mayhalt) to indicate that
		 * racing condition may happen. This flag is set when the
		 * loop count is getting close to the halting threshold.
		 *
		 * When that happens, a 2 variables (cpustate & qhead
		 * [=mcs.locked]) handshake is used to make sure that
		 * pv_halt_check() won't miss setting the _Q_LOCKED_SLOWPATH
		 * when the CPU is about to be halted.
		 *
		 * pv_halt_check		pv_queue_spin_check
		 * -------------		-------------------
		 * [1] qhead = true		[3] cpustate = PV_CPU_HALTED
		 *     smp_mb()			    smp_mb()
		 * [2] if (cpustate		[4] if (qhead)
		 *        == PV_CPU_HALTED)
		 *
		 * Sequence:
		 * *,1,*,4,* - halt is aborted as the qhead flag is set,
		 *	       _Q_LOCKED_SLOWPATH may or may not be set
		 * 3,4,1,2 - the CPU is halt and _Q_LOCKED_SLOWPATH is set
		 */
		smp_mb();
		if (!ACCESS_ONCE(mcs->locked)) {
			/*
			 * Halt the CPU only if it is not the queue head
			 */
			__queue_halt_cpu(PV_HALT_QNODE, &pv->cpustate,
					 PV_CPU_HALTED);
			__queue_lockstat((pv->cpustate == PV_CPU_KICKED)
					 ? PV_WAKE_KICKED : PV_WAKE_SPURIOUS);
		}
		ACCESS_ONCE(pv->cpustate) = PV_CPU_ACTIVE;
		*count	    = 0;	/* Reset count & flag */
		pv->mayhalt = false;
	} else if (*count == MAYHALT_THRESHOLD) {
		pv->mayhalt = true;
		/*
		 * Make sure that the mayhalt flag is visible to others
		 * before proceeding.
		 */
		smp_mb();
	}
}

/**
 * pv_halt_check - check if the CPU has been halted & set _Q_LOCKED_SLOWPATH
 * @pv   : pointer to struct pv_qvars
 * @count: loop count
 *
 * The current CPU should have gotten the lock and the queue head flag set
 * before calling this function.
 */
static __always_inline void
pv_halt_check(struct pv_qvars *pv, struct qspinlock *lock)
{
	if (!static_key_false(&paravirt_spinlocks_enabled))
		return;
	/*
	 * Halt state checking will only be done if the mayhalt flag is set
	 * to avoid the overhead of the memory barrier in normal cases.
	 * It is highly unlikely that the actual writing to the qhead flag
	 * will be more than 0x10 iterations later than the reading of the
	 * mayhalt flag so that it misses seeing the PV_CPU_HALTED state
	 * which causes lost wakeup.
	 */
	if (ACCESS_ONCE(pv->mayhalt)) {
		/*
		 * A memory barrier is used here to make sure that the setting
		 * of queue head flag prior to this function call is visible
		 * to others before checking the cpustate flag.
		 */
		smp_mb();
		if (pv->cpustate == PV_CPU_HALTED)
			ACCESS_ONCE(*(u8 *)lock) = _Q_LOCKED_SLOWPATH;
	}
}

/**
 * pv_set_prev - set previous queue node pointer
 * @pv  : pointer to struct pv_qvars to be set
 * @prev: pointer to the previous node
 */
static __always_inline void pv_set_prev(struct pv_qvars *pv, struct qnode *prev)
{
	ACCESS_ONCE(pv->prev) = prev;
	/*
	 * Make sure the prev field is set up before others
	 */
	smp_wmb();
}

/*
 * The following inlined functions are being used by the
 * queue_spin_unlock_slowpath() function.
 */

/**
 * pv_get_prev - get previous queue node pointer
 * @pv   : pointer to struct pv_qvars to be set
 * Return: the previous queue node pointer
 */
static __always_inline struct qnode *pv_get_prev(struct pv_qvars *pv)
{
	return ACCESS_ONCE(pv->prev);
}

/**
 * pv_kick_node - kick up the CPU of the given node
 * @pv  : pointer to struct pv_qvars of the node to be kicked
 */
static __always_inline void pv_kick_node(struct pv_qvars *pv)
{
	s8 oldstate = xchg(&pv->cpustate, PV_CPU_KICKED);

	/*
	 * Kick up the CPU only if the state was set to PV_CPU_HALTED
	 */
	if (oldstate != PV_CPU_HALTED)
		__queue_lockstat(PV_KICK_NOHALT);
	else
		__queue_kick_cpu(pv->mycpu);
}

#endif /* _ASM_X86_PVQSPINLOCK_H */
