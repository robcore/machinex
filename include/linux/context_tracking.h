#ifndef _LINUX_CONTEXT_TRACKING_H
#define _LINUX_CONTEXT_TRACKING_H

#include <linux/sched.h>
#include <linux/vtime.h>
#include <linux/context_tracking_state.h>
#include <asm/ptrace.h>


#ifdef CONFIG_CONTEXT_TRACKING
extern void context_tracking_cpu_set(int cpu);

extern void context_tracking_user_enter(void);
extern void context_tracking_user_exit(void);
extern void __context_tracking_task_switch(struct task_struct *prev,
					   struct task_struct *next);

static inline void user_enter(void)
{
	if (context_tracking_is_enabled())
		context_tracking_user_enter();

}
static inline void user_exit(void)
{
	if (context_tracking_is_enabled())
		context_tracking_user_exit();
}

static inline enum ctx_state exception_enter(void)
{
	enum ctx_state prev_ctx;

	if (!context_tracking_is_enabled())
		return 0;

	prev_ctx = this_cpu_read(context_tracking.state);
	context_tracking_user_exit();

	return prev_ctx;
}

static inline void exception_exit(enum ctx_state prev_ctx)
{
	if (context_tracking_is_enabled()) {
		if (prev_ctx == IN_USER)
			context_tracking_user_enter();
	}
}

static inline void context_tracking_task_switch(struct task_struct *prev,
						struct task_struct *next)
{
	if (context_tracking_is_enabled())
		__context_tracking_task_switch(prev, next);
}
#else
static inline void user_enter(void) { }
static inline void user_exit(void) { }
static inline enum ctx_state exception_enter(void) { return 0; }
static inline void exception_exit(enum ctx_state prev_ctx) { }
static inline void context_tracking_task_switch(struct task_struct *prev,
						struct task_struct *next) { }
#endif /* !CONFIG_CONTEXT_TRACKING */

#ifdef CONFIG_VIRT_CPU_ACCOUNTING_GEN
static inline void guest_enter(void)
{
	if (vtime_accounting_enabled())
		vtime_guest_enter(current);
	else
		current->flags |= PF_VCPU;
}

static inline void guest_exit(void)
{
	if (vtime_accounting_enabled())
		vtime_guest_exit(current);
	else
		current->flags &= ~PF_VCPU;
}

#else
static inline void guest_enter(void)
{
	/*
	 * This is running in ioctl context so we can avoid
	 * the call to vtime_account() with its unnecessary idle check.
	 */
	vtime_account_system(current);
	current->flags |= PF_VCPU;
}

static inline void guest_exit(void)
{
	/*
	 * This is running in ioctl context so we can avoid
	 * the call to vtime_account() with its unnecessary idle check.
	 */
	vtime_account_system(current);
	current->flags &= ~PF_VCPU;
}
#endif /* CONFIG_VIRT_CPU_ACCOUNTING_GEN */

#ifdef CONFIG_CONTEXT_TRACKING_FORCE
extern void context_tracking_init(void);
#else
static inline void context_tracking_init(void) { }
#endif /* CONFIG_CONTEXT_TRACKING_FORCE */

#endif
