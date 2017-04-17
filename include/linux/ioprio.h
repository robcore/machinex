#ifndef IOPRIO_H
#define IOPRIO_H

#include <linux/sched.h>
#include <linux/iocontext.h>
#include <uapi/linux/ioprio.h>

/*
 * if process has set io priority explicitly, use that. if not, convert
 * the cpu scheduler nice value to an io priority
 */
#define IOPRIO_NORM	(4)
static inline int task_ioprio(struct io_context *ioc)
{
	if (ioprio_valid(ioc->ioprio))
		return IOPRIO_PRIO_DATA(ioc->ioprio);

	return IOPRIO_NORM;
}

static inline int task_ioprio_class(struct io_context *ioc)
{
	if (ioprio_valid(ioc->ioprio))
		return IOPRIO_PRIO_CLASS(ioc->ioprio);

	return IOPRIO_CLASS_BE;
}

static inline int task_nice_ioprio(struct task_struct *task)
{
	return (task_nice(task) + 20) / 5;
}

/*
 * This is for the case where the task hasn't asked for a specific IO class.
 * Check for idle and rt task process, and return appropriate IO class.
 */
static inline int task_nice_ioclass(struct task_struct *task)
{
	if (task->policy == SCHED_IDLE)
		return IOPRIO_CLASS_IDLE;
	else if (task->policy == SCHED_FIFO || task->policy == SCHED_RR)
		return IOPRIO_CLASS_RT;
	else
		return IOPRIO_CLASS_BE;
}

/*
 * For inheritance, return the highest of the two given priorities
 */
extern int ioprio_best(unsigned short aprio, unsigned short bprio);

extern int set_task_ioprio(struct task_struct *task, int ioprio);

#endif
