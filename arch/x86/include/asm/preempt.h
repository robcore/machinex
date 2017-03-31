#ifndef __ASM_PREEMPT_H
#define __ASM_PREEMPT_H

#include <asm/rmwcc.h>
#include <asm/percpu.h>
#include <linux/thread_info.h>

DECLARE_PER_CPU(int, __preempt_count);

/*
 * We use the PREEMPT_NEED_RESCHED bit as an inverted NEED_RESCHED such
 * that a decrement hitting 0 means we can and should reschedule.
 */
#define PREEMPT_ENABLED	(0 + PREEMPT_NEED_RESCHED)

/*
 * We mask the PREEMPT_NEED_RESCHED bit so as not to confuse all current users
 * that think a non-zero value indicates we cannot preempt.
 */
static __always_inline int preempt_count(void)
{
	return __this_cpu_read_4(__preempt_count) & ~PREEMPT_NEED_RESCHED;
}

static __always_inline void preempt_count_set(int pc)
{
	__this_cpu_write_4(__preempt_count, pc);
}

/*
 * must be macros to avoid header recursion hell
 */
#define task_preempt_count(p) \
	(task_thread_info(p)->saved_preempt_count & ~PREEMPT_NEED_RESCHED)

#define init_task_preempt_count(p) do { \
	task_thread_info(p)->saved_preempt_count = PREEMPT_DISABLED; \
} while (0)

#define init_idle_preempt_count(p, cpu) do { \
	task_thread_info(p)->saved_preempt_count = PREEMPT_ENABLED; \
	per_cpu(__preempt_count, (cpu)) = PREEMPT_ENABLED; \
} while (0)

/*
 * We fold the NEED_RESCHED bit into the preempt count such that
 * preempt_enable() can decrement and test for needing to reschedule with a
 * single instruction.
 *
 * We invert the actual bit, so that when the decrement hits 0 we know we both
 * need to resched (the bit is cleared) and can resched (no preempt count).
 */

static __always_inline void set_preempt_need_resched(void)
{
	__this_cpu_and_4(__preempt_count, ~PREEMPT_NEED_RESCHED);
}

static __always_inline void clear_preempt_need_resched(void)
{
	__this_cpu_or_4(__preempt_count, PREEMPT_NEED_RESCHED);
}

static __always_inline bool test_preempt_need_resched(void)
{
	return !(__this_cpu_read_4(__preempt_count) & PREEMPT_NEED_RESCHED);
}

/*
 * The various preempt_count add/sub methods
 */

static __always_inline void __preempt_count_add(int val)
{
	__this_cpu_add_4(__preempt_count, val);
}

static __always_inline void __preempt_count_sub(int val)
{
	__this_cpu_add_4(__preempt_count, -val);
}

/*
 * Because we keep PREEMPT_NEED_RESCHED set when we do _not_ need to reschedule
 * a decrement which hits zero means we have no preempt_count and should
 * reschedule.
 */
static __always_inline bool __preempt_count_dec_and_test(void)
{
	GEN_UNARY_RMWcc("decl", __preempt_count, __percpu_arg(0), "e");
}

/*
 * Returns true when we need to resched -- even if we can not.
 */
static __always_inline bool need_resched(void)
{
	return unlikely(test_preempt_need_resched());
}

/*
 * Returns true when we need to resched and can (barring IRQ state).
 */
static __always_inline bool should_resched(void)
{
	return unlikely(!__this_cpu_read_4(__preempt_count));
}

#endif /* __ASM_PREEMPT_H */
