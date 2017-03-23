/*
 * Context tracking: Probe on high level context boundaries such as kernel
 * and userspace. This includes syscalls and exceptions entry/exit.
 *
 * This is used by RCU to remove its dependency on the timer tick while a CPU
 * runs in userspace.
 *
 *  Started by Frederic Weisbecker:
 *
 * Copyright (C) 2012 Red Hat, Inc., Frederic Weisbecker <fweisbec@redhat.com>
 *
 * Many thanks to Gilad Ben-Yossef, Paul McKenney, Ingo Molnar, Andrew Morton,
 * Steven Rostedt, Peter Zijlstra for suggestions and improvements.
 *
 */

#include <linux/context_tracking.h>
#include <linux/rcupdate.h>
#include <linux/sched.h>
#include <linux/hardirq.h>
#include <linux/export.h>

struct static_key context_tracking_enabled = STATIC_KEY_INIT_FALSE;
EXPORT_SYMBOL_GPL(context_tracking_enabled);

DEFINE_PER_CPU(struct context_tracking, context_tracking);
EXPORT_SYMBOL_GPL(context_tracking);

void context_tracking_cpu_set(int cpu)
{
	if (!per_cpu(context_tracking.active, cpu)) {
		per_cpu(context_tracking.active, cpu) = true;
		static_key_slow_inc(&context_tracking_enabled);
	}
}

/**
 * context_tracking_user_enter - Inform the context tracking that the CPU is going to
 *                               enter userspace mode.
 *
 * This function must be called right before we switch from the kernel
 * to userspace, when it's guaranteed the remaining kernel instructions
 * to execute won't use any RCU read side critical section because this
 * function sets RCU in extended quiescent state.
 */
void context_tracking_user_enter(void)
{
	unsigned long flags;

	/*
	 * Some contexts may involve an exception occuring in an irq,
	 * leading to that nesting:
	 * rcu_irq_enter() rcu_user_exit() rcu_user_exit() rcu_irq_exit()
	 * This would mess up the dyntick_nesting count though. And rcu_irq_*()
	 * helpers are enough to protect RCU uses inside the exception. So
	 * just return immediately if we detect we are in an IRQ.
	 */
	if (in_interrupt())
		return;

	/* Kernel threads aren't supposed to go to userspace */
	WARN_ON_ONCE(!current->mm);

	local_irq_save(flags);
	if (__this_cpu_read(context_tracking.active) &&
	    __this_cpu_read(context_tracking.state) != IN_USER) {
		vtime_user_enter(current);
		rcu_user_enter();
		__this_cpu_write(context_tracking.state, IN_USER);
	}
	local_irq_restore(flags);
}

#ifdef CONFIG_PREEMPT
/**
 * preempt_schedule_context - preempt_schedule called by tracing
 *
 * The tracing infrastructure uses preempt_enable_notrace to prevent
 * recursion and tracing preempt enabling caused by the tracing
 * infrastructure itself. But as tracing can happen in areas coming
 * from userspace or just about to enter userspace, a preempt enable
 * can occur before user_exit() is called. This will cause the scheduler
 * to be called when the system is still in usermode.
 *
 * To prevent this, the preempt_enable_notrace will use this function
 * instead of preempt_schedule() to exit user context if needed before
 * calling the scheduler.
 */
void __sched notrace preempt_schedule_context(void)
{
	enum ctx_state prev_ctx;

	if (likely(!preemptible()))
		return;

	/*
	 * Need to disable preemption in case user_exit() is traced
	 * and the tracer calls preempt_enable_notrace() causing
	 * an infinite recursion.
	 */
	preempt_disable_notrace();
	prev_ctx = exception_enter();
	preempt_enable_no_resched_notrace();

	preempt_schedule();

	preempt_disable_notrace();
	exception_exit(prev_ctx);
	preempt_enable_notrace();
}
EXPORT_SYMBOL_GPL(preempt_schedule_context);
#endif /* CONFIG_PREEMPT */

/**
 * context_tracking_user_exit - Inform the context tracking that the CPU is
 *                              exiting userspace mode and entering the kernel.
 *
 * This function must be called after we entered the kernel from userspace
 * before any use of RCU read side critical section. This potentially include
 * any high level kernel code like syscalls, exceptions, signal handling, etc...
 *
 * This call supports re-entrancy. This way it can be called from any exception
 * handler without needing to know if we came from userspace or not.
 */
void context_tracking_user_exit(void)
{
	unsigned long flags;

	if (in_interrupt())
		return;

	local_irq_save(flags);
	if (__this_cpu_read(context_tracking.state) == IN_USER) {
		rcu_user_exit();
		vtime_user_exit(current);
		__this_cpu_write(context_tracking.state, IN_KERNEL);
	}
	local_irq_restore(flags);
}

void context_tracking_task_switch(struct task_struct *prev,
			     struct task_struct *next)
{
	if (__this_cpu_read(context_tracking.active)) {
		clear_tsk_thread_flag(prev, TIF_NOHZ);
		set_tsk_thread_flag(next, TIF_NOHZ);
	}
}

#ifdef CONFIG_CONTEXT_TRACKING_FORCE
void __init context_tracking_init(void)
{
	int cpu;

	for_each_possible_cpu(cpu)
		context_tracking_cpu_set(cpu);
}
#endif
