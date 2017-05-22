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
#include <linux/seqlock.h>

/*
 * See entry-armv.S for the offsets into the kernel user helper for
 * these fields.
 */
#define ARM_VSYSCALL_TIMER_TZ			0xf20
#define ARM_VSYSCALL_TIMER_SEQ			0xf28
#define ARM_VSYSCALL_TIMER_OFFSET		0xf30
#define ARM_VSYSCALL_TIMER_WTM_TV_SEC		0xf38
#define ARM_VSYSCALL_TIMER_WTM_TV_NSEC		0xf3c
#define ARM_VSYSCALL_TIMER_CYCLE_LAST		0xf40
#define ARM_VSYSCALL_TIMER_MASK			0xf48
#define ARM_VSYSCALL_TIMER_MULT			0xf50
#define ARM_VSYSCALL_TIMER_SHIFT		0xf54
#define ARM_VSYSCALL_TIMER_TV_SEC		0xf58
#define ARM_VSYSCALL_TIMER_TV_NSEC		0xf5c

static struct {
	struct timekeeper	timekeeper;
} tk_core ____cacheline_aligned;

static seqlock_t vsys_seq;
extern void *vectors_page;
extern struct timezone sys_tz;

struct kernel_gtod_t {
	u64  cycle_last;
	u64  mask;
	u32  mult;
	u32  shift;
	uint32_t tv_sec;
	uint32_t tv_nsec;
};

struct kernel_tz_t {
	u32  tz_minuteswest;
	u32  tz_dsttime;
};

struct kernel_wtm_t {
	uint32_t  tv_sec;
	uint32_t  tv_nsec;
};

/*
 * Updates the kernel user helper area with the current timespec
 * data, as well as additional fields needed to calculate
 * gettimeofday, clock_gettime, etc.
 */
void
update_vsyscall_old(struct timespec *ts, struct timespec *wtm,
						struct clocksource *c, u32 mult,
						u64 cycle_last)
{
	unsigned long vectors = (unsigned long)vectors_page;
	unsigned long flags;
	unsigned long *seqnum = (unsigned long *)(vectors + ARM_VSYSCALL_TIMER_SEQ);
	struct kernel_gtod_t *dgtod = (struct kernel_gtod_t *)(vectors +
		ARM_VSYSCALL_TIMER_CYCLE_LAST);
	struct kernel_wtm_t *dgwtm = (struct kernel_wtm_t *)(vectors +
		ARM_VSYSCALL_TIMER_WTM_TV_SEC);
	struct timekeeper *tk = &tk_core.timekeeper;
	struct tk_read_base *tkr_mono;

	write_seqlock_irqsave(&vsys_seq, flags);
	*seqnum = vsys_seq.seqcount.sequence;
	dgtod->cycle_last = tk->tkr_mono.cycle_last;
	dgtod->mask = c->mask;
	dgtod->mult = c->mult;
	dgtod->shift = c->shift;
	dgtod->tv_sec = ts->tv_sec;
	dgtod->tv_nsec = ts->tv_nsec;
	dgwtm->tv_sec = wtm->tv_sec;
	dgwtm->tv_nsec = wtm->tv_nsec;
	*seqnum = vsys_seq.seqcount.sequence + 1;
	write_sequnlock_irqrestore(&vsys_seq, flags);
}
EXPORT_SYMBOL(update_vsyscall_old);

void
update_vsyscall_tz(void)
{
	unsigned long vectors = (unsigned long)vectors_page;
	unsigned long flags;
	unsigned long *seqnum = (unsigned long *)(vectors + ARM_VSYSCALL_TIMER_SEQ);
	struct kernel_tz_t *dgtod = (struct kernel_tz_t *)(vectors +
		ARM_VSYSCALL_TIMER_TZ);

	write_seqlock_irqsave(&vsys_seq, flags);
	*seqnum = vsys_seq.seqcount.sequence;
	dgtod->tz_minuteswest = sys_tz.tz_minuteswest;
	dgtod->tz_dsttime = sys_tz.tz_dsttime;
	*seqnum = vsys_seq.seqcount.sequence + 1;
	write_sequnlock_irqrestore(&vsys_seq, flags);
}
EXPORT_SYMBOL(update_vsyscall_tz);
