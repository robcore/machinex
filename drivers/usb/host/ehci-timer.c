/*
 * Copyright (C) 2012 by Alan Stern
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

/* This file is part of ehci-hcd.c */

/*-------------------------------------------------------------------------*/

/* Set a bit in the USBCMD register */
static void ehci_set_command_bit(struct ehci_hcd *ehci, u32 bit)
{
	ehci->command |= bit;
	ehci_writel(ehci, ehci->command, &ehci->regs->command);

	/* unblock posted write */
	ehci_readl(ehci, &ehci->regs->command);
}

/* Clear a bit in the USBCMD register */
static void ehci_clear_command_bit(struct ehci_hcd *ehci, u32 bit)
{
	ehci->command &= ~bit;
	ehci_writel(ehci, ehci->command, &ehci->regs->command);

	/* unblock posted write */
	ehci_readl(ehci, &ehci->regs->command);
}

/*-------------------------------------------------------------------------*/

/*
 * EHCI timer support...  Now using hrtimers.
 *
 * Lots of different events are triggered from ehci->hrtimer.  Whenever
 * the timer routine runs, it checks each possible event; events that are
 * currently enabled and whose expiration time has passed get handled.
 * The set of enabled events is stored as a collection of bitflags in
 * ehci->enabled_hrtimer_events, and they are numbered in order of
 * increasing delay values (ranging between 1 ms and 100 ms).
 *
 * Rather than implementing a sorted list or tree of all pending events,
 * we keep track only of the lowest-numbered pending event, in
 * ehci->next_hrtimer_event.  Whenever ehci->hrtimer gets restarted, its
 * expiration time is set to the timeout value for this event.
 *
 * As a result, events might not get handled right away; the actual delay
 * could be anywhere up to twice the requested delay.  This doesn't
 * matter, because none of the events are especially time-critical.  The
 * ones that matter most all have a delay of 1 ms, so they will be
 * handled after 2 ms at most, which is okay.  In addition to this, we
 * allow for an expiration range of 1 ms.
 */

/*
 * Delay lengths for the hrtimer event types.
 * Keep this list sorted by delay length, in the same order as
 * the event types indexed by enum ehci_hrtimer_event in ehci.h.
 */
static unsigned event_delays_ns[] = {
	1 * NSEC_PER_MSEC,	/* EHCI_HRTIMER_POLL_PSS */
	10 * NSEC_PER_MSEC,	/* EHCI_HRTIMER_DISABLE_PERIODIC */
};

/* Enable a pending hrtimer event */
static void ehci_enable_event(struct ehci_hcd *ehci, unsigned event,
		bool resched)
{
	ktime_t		*timeout = &ehci->hr_timeouts[event];

	if (resched)
		*timeout = ktime_add(ktime_get(),
				ktime_set(0, event_delays_ns[event]));
	ehci->enabled_hrtimer_events |= (1 << event);

	/* Track only the lowest-numbered pending event */
	if (event < ehci->next_hrtimer_event) {
		ehci->next_hrtimer_event = event;
		hrtimer_start_range_ns(&ehci->hrtimer, *timeout,
				NSEC_PER_MSEC, HRTIMER_MODE_ABS);
	}
}


/* Poll the STS_PSS status bit; see when it agrees with CMD_PSE */
static void ehci_poll_PSS(struct ehci_hcd *ehci)
{
	unsigned	actual, want;

	/* Don't do anything if the controller isn't running (e.g., died) */
	if (ehci->rh_state != EHCI_RH_RUNNING)
		return;

	want = (ehci->command & CMD_PSE) ? STS_PSS : 0;
	actual = ehci_readl(ehci, &ehci->regs->status) & STS_PSS;

	if (want != actual) {

		/* Poll again later, but give up after about 20 ms */
		if (ehci->PSS_poll_count++ < 20) {
			ehci_enable_event(ehci, EHCI_HRTIMER_POLL_PSS, true);
			return;
		}
		ehci_warn(ehci, "Waited too long for the periodic schedule status, giving up\n");
	}
	ehci->PSS_poll_count = 0;

	/* The status is up-to-date; restart or stop the schedule as needed */
	if (want == 0) {	/* Stopped */
		free_cached_lists(ehci);
		if (ehci->periodic_count > 0) {

			/* make sure ehci_work scans these */
			ehci->next_uframe = ehci_read_frame_index(ehci)
					& ((ehci->periodic_size << 3) - 1);
			ehci_set_command_bit(ehci, CMD_PSE);
		}

	} else {		/* Running */
		if (ehci->periodic_count == 0) {

			/* Turn off the schedule after a while */
			ehci_enable_event(ehci, EHCI_HRTIMER_DISABLE_PERIODIC,
					true);
		}
	}
}

/* Turn off the periodic schedule after a brief delay */
static void ehci_disable_PSE(struct ehci_hcd *ehci)
{
	ehci_clear_command_bit(ehci, CMD_PSE);

	/* Poll to see when it actually stops */
	ehci_enable_event(ehci, EHCI_HRTIMER_POLL_PSS, true);
}


/*
 * Handler functions for the hrtimer event types.
 * Keep this array in the same order as the event types indexed by
 * enum ehci_hrtimer_event in ehci.h.
 */
static void (*event_handlers[])(struct ehci_hcd *) = {
	ehci_poll_PSS,			/* EHCI_HRTIMER_POLL_PSS */
	ehci_disable_PSE,		/* EHCI_HRTIMER_DISABLE_PERIODIC */
};

static enum hrtimer_restart ehci_hrtimer_func(struct hrtimer *t)
{
	struct ehci_hcd	*ehci = container_of(t, struct ehci_hcd, hrtimer);
	ktime_t		now;
	unsigned long	events;
	unsigned long	flags;
	unsigned	e;

	spin_lock_irqsave(&ehci->lock, flags);

	events = ehci->enabled_hrtimer_events;
	ehci->enabled_hrtimer_events = 0;
	ehci->next_hrtimer_event = EHCI_HRTIMER_NO_EVENT;

	/*
	 * Check each pending event.  If its time has expired, handle
	 * the event; otherwise re-enable it.
	 */
	now = ktime_get();
	for_each_set_bit(e, &events, EHCI_HRTIMER_NUM_EVENTS) {
		if (now.tv64 >= ehci->hr_timeouts[e].tv64)
			event_handlers[e](ehci);
		else
			ehci_enable_event(ehci, e, false);
	}

	spin_unlock_irqrestore(&ehci->lock, flags);
	return HRTIMER_NORESTART;
}
