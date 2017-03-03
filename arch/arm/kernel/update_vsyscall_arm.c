/* Copyright (c) 2012, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/export.h>
#include <linux/timekeeper_internal.h>
#include <linux/time.h>
#include "update_vsyscall_arm.h"

static DEFINE_MUTEX(machinex_vsys);
/*
 * This read-write spinlock protects us from races in SMP while
 * updating the kernel user helper-embedded time.
 */
__cacheline_aligned_in_smp DEFINE_SPINLOCK(mxvsys->kuh_time_lock);
/*
 * Updates the kernel user helper area with the current timespec
 * data, as well as additional fields needed to calculate
 * gettimeofday, clock_gettime, etc.
 */
void
update_vsyscall_old(struct timespec *ts, struct timespec *wtm,
	struct clocksource *c, u32 mult)
{
	unsigned long vectors = (unsigned long)vectors_page;
	unsigned long flags;
	unsigned *seqcount = (unsigned *)(vectors + ARM_VSYSCALL_TIMER_SEQ);
	struct kernel_gtod_t *dgtod = (struct kernel_gtod_t *)(vectors +
		ARM_VSYSCALL_TIMER_CYCLE_LAST);
	struct kernel_wtm_t *dgwtm = (struct kernel_wtm_t *)(vectors +
		ARM_VSYSCALL_TIMER_WTM_TV_SEC);

	mutex_lock(&machinex_vsys);
	spin_lock_irqsave(&mxvsys->kuh_time_lock, flags);
	*seqcount = write_seqcount_begin(&mxvsys->seq)
	dgtod->cycle_last = c->cycle_last;
	dgtod->mask = c->mask;
	dgtod->mult = c->mult;
	dgtod->shift = c->shift;
	dgtod->tv_sec = ts->tv_sec;
	dgtod->tv_nsec = ts->tv_nsec;
	dgwtm->tv_sec = wtm->tv_sec;
	dgwtm->tv_nsec = wtm->tv_nsec;
	*seqcount = write_seqcount_end(&mxvsys->seq)
	spin_lock_irqrestore(&mxvsys->kuh_time_lock, flags);
	mutex_unlock(&machinex_vsys);
}
EXPORT_SYMBOL(update_vsyscall_old);

void
update_vsyscall_tz(void)
{
	unsigned long vectors = (unsigned long)vectors_page;
	unsigned long flags;
	unsigned *seqcount = (unsigned *)(vectors + ARM_VSYSCALL_TIMER_SEQ);
	struct kernel_tz_t *dgtod = (struct kernel_tz_t *)(vectors +
		ARM_VSYSCALL_TIMER_TZ);

	mutex_lock(&machinex_vsys);
	spin_lock_irqsave(&mxvsys->kuh_time_lock, flags);
	*seqcount = write_seqcount_begin(&mxvsys->seq)
	dgtod->tz_minuteswest = sys_tz.tz_minuteswest;
	dgtod->tz_dsttime = sys_tz.tz_dsttime;
	*seqcount = write_seqcount_end(&mxvsys->seq)
	spin_lock_irqrestore(&mxvsys->kuh_time_lock, flags);
	mutex_unlock(&machinex_vsys);


}
EXPORT_SYMBOL(update_vsyscall_tz);
