/*  linux/include/linux/clockchips.h
 *
 *  This file contains the structure definitions for clockchips.
 *
 *  If you are not a clockchip, or the time of day code, you should
 *  not be including this file!
 */
#ifndef _LINUX_CLOCKCHIPS_H
#define _LINUX_CLOCKCHIPS_H

#ifdef CONFIG_GENERIC_CLOCKEVENTS

#include <linux/clocksource.h>
#include <linux/cpumask.h>
#include <linux/ktime.h>
#include <linux/notifier.h>

struct clock_event_device;
struct module;

/* Clock event mode commands for legacy ->set_mode(): OBSOLETE */
enum clock_event_mode {
	CLOCK_EVT_MODE_UNUSED = 0,
	CLOCK_EVT_MODE_SHUTDOWN,
	CLOCK_EVT_MODE_PERIODIC,
	CLOCK_EVT_MODE_ONESHOT,
	CLOCK_EVT_MODE_RESUME,
};

/*
 * Possible states of a clock event device.
 *
 * DETACHED:	Device is not used by clockevents core. Initial state or can be
 *		reached from SHUTDOWN.
 * SHUTDOWN:	Device is powered-off. Can be reached from PERIODIC or ONESHOT.
 * PERIODIC:	Device is programmed to generate events periodically. Can be
 *		reached from DETACHED or SHUTDOWN.
 * ONESHOT:	Device is programmed to generate event only once. Can be reached
 *		from DETACHED or SHUTDOWN.
 */
enum clock_event_state {
	CLOCK_EVT_STATE_DETACHED = 0,
	CLOCK_EVT_STATE_SHUTDOWN,
	CLOCK_EVT_STATE_PERIODIC,
	CLOCK_EVT_STATE_ONESHOT,
};

/* Clock event notification values */
enum clock_event_nofitiers {
	CLOCK_EVT_NOTIFY_ADD,
	CLOCK_EVT_NOTIFY_BROADCAST_ON,
	CLOCK_EVT_NOTIFY_BROADCAST_OFF,
	CLOCK_EVT_NOTIFY_BROADCAST_FORCE,
	CLOCK_EVT_NOTIFY_BROADCAST_ENTER,
	CLOCK_EVT_NOTIFY_BROADCAST_EXIT,
	CLOCK_EVT_NOTIFY_SUSPEND,
	CLOCK_EVT_NOTIFY_RESUME,
	CLOCK_EVT_NOTIFY_CPU_DYING,
	CLOCK_EVT_NOTIFY_CPU_DEAD,
};

/*
 * Clock event features
 */
#define CLOCK_EVT_FEAT_PERIODIC		0x000001
#define CLOCK_EVT_FEAT_ONESHOT		0x000002
#define CLOCK_EVT_FEAT_KTIME		0x000004
/*
 * x86(64) specific misfeatures:
 *
 * - Clockevent source stops in C3 State and needs broadcast support.
 * - Local APIC timer is used as a dummy device.
 */
#define CLOCK_EVT_FEAT_C3STOP		0x000008
#define CLOCK_EVT_FEAT_DUMMY		0x000010

/*
 * Core shall set the interrupt affinity dynamically in broadcast mode
 */
#define CLOCK_EVT_FEAT_DYNIRQ		0x000020
#define CLOCK_EVT_FEAT_PERCPU		0x000040

/*
 * Clockevent device is based on a hrtimer for broadcast
 */
#define CLOCK_EVT_FEAT_HRTIMER		0x000080

/**
 * struct clock_event_device - clock event device descriptor
 * @event_handler:	Assigned by the framework to be called by the low
 *			level handler of the event source
 * @set_next_event:	set next event function using a clocksource delta
 * @set_next_ktime:	set next event function using a direct ktime value
 * @next_event:		local storage for the next event in oneshot mode
 * @max_delta_ns:	maximum delta value in ns
 * @min_delta_ns:	minimum delta value in ns
 * @mult:		nanosecond to cycles multiplier
 * @shift:		nanoseconds to cycles divisor (power of two)
 * @mode:		operating mode, relevant only to ->set_mode(), OBSOLETE
 * @state:		current state of the device, assigned by the core code
 * @features:		features
 * @retries:		number of forced programming retries
 * @set_mode:		legacy set mode function, only for modes <= CLOCK_EVT_MODE_RESUME.
 * @set_state_periodic:	switch state to periodic, if !set_mode
 * @set_state_oneshot:	switch state to oneshot, if !set_mode
 * @set_state_shutdown:	switch state to shutdown, if !set_mode
 * @tick_resume:	resume clkevt device, if !set_mode
 * @broadcast:		function to broadcast events
 * @min_delta_ticks:	minimum delta value in ticks stored for reconfiguration
 * @max_delta_ticks:	maximum delta value in ticks stored for reconfiguration
 * @name:		ptr to clock event name
 * @rating:		variable to rate clock event devices
 * @irq:		IRQ number (only for non CPU local devices)
 * @bound_on:		Bound on CPU
 * @cpumask:		cpumask to indicate for which CPUs this device works
 * @list:		list head for the management code
 * @owner:		module reference
 */
struct clock_event_device {
	void			(*event_handler)(struct clock_event_device *);
	int			(*set_next_event)(unsigned long evt,
						  struct clock_event_device *);
	int			(*set_next_ktime)(ktime_t expires,
						  struct clock_event_device *);
	ktime_t			next_event;
	u64			max_delta_ns;
	u64			min_delta_ns;
	u32			mult;
	u32			shift;
	enum clock_event_mode	mode;
	enum clock_event_state	state;
	unsigned int		features;
	unsigned long		retries;

	/*
	 * State transition callback(s): Only one of the two groups should be
	 * defined:
	 * - set_mode(), only for modes <= CLOCK_EVT_MODE_RESUME.
	 * - set_state_{shutdown|periodic|oneshot}(), tick_resume().
	 */
	void			(*set_mode)(enum clock_event_mode mode,
					    struct clock_event_device *);
	int			(*set_state_periodic)(struct clock_event_device *);
	int			(*set_state_oneshot)(struct clock_event_device *);
	int			(*set_state_shutdown)(struct clock_event_device *);
	int			(*tick_resume)(struct clock_event_device *);

	void			(*broadcast)(const struct cpumask *mask);
	void			(*suspend)(struct clock_event_device *);
	void			(*resume)(struct clock_event_device *);
	unsigned long		min_delta_ticks;
	unsigned long		max_delta_ticks;

	const char		*name;
	int			rating;
	int			irq;
	int			bound_on;
	const struct cpumask	*cpumask;
	struct list_head	list;
	struct module		*owner;
} ____cacheline_aligned;

/*
 * Calculate a multiplication factor for scaled math, which is used to convert
 * nanoseconds based values to clock ticks:
 *
 * clock_ticks = (nanoseconds * factor) >> shift.
 *
 * div_sc is the rearranged equation to calculate a factor from a given clock
 * ticks / nanoseconds ratio:
 *
 * factor = (clock_ticks << shift) / nanoseconds
 */
static inline unsigned long div_sc(unsigned long ticks, unsigned long nsec,
				   int shift)
{
	uint64_t tmp = ((uint64_t)ticks) << shift;

	do_div(tmp, nsec);
	return (unsigned long) tmp;
}

/* Clock event layer functions */
extern u64 clockevent_delta2ns(unsigned long latch,
			       struct clock_event_device *evt);
extern void clockevents_register_device(struct clock_event_device *dev);
extern int clockevents_unbind_device(struct clock_event_device *ced, int cpu);

extern void clockevents_config(struct clock_event_device *dev, u32 freq);
extern void clockevents_config_and_register(struct clock_event_device *dev,
					    u32 freq, unsigned long min_delta,
					    unsigned long max_delta);

extern int clockevents_update_freq(struct clock_event_device *ce, u32 freq);

static inline void
clockevents_calc_mult_shift(struct clock_event_device *ce, u32 freq, u32 minsec)
{
	return clocks_calc_mult_shift(&ce->mult, &ce->shift, NSEC_PER_SEC,
				      freq, minsec);
}

#if defined(CONFIG_GENERIC_CLOCKEVENTS_BROADCAST) && defined(CONFIG_TICK_ONESHOT)
extern void tick_setup_hrtimer_broadcast(void);
extern int tick_check_broadcast_expired(void);
#else
static inline int tick_check_broadcast_expired(void) { return 0; }
static void tick_setup_hrtimer_broadcast(void) {};
#endif

#ifdef CONFIG_GENERIC_CLOCKEVENTS_BROADCAST
#ifdef CONFIG_ARCH_HAS_TICK_BROADCAST
extern void tick_broadcast(const struct cpumask *mask);
#else
#define tick_broadcast	NULL
#endif
extern int tick_receive_broadcast(void);
#endif

/* Should be core only, but is abused by arm bl_switcher */
extern void clockevents_set_state(struct clock_event_device *dev,
				 enum clock_event_state state);
extern int clockevents_program_event(struct clock_event_device *dev,
				     ktime_t expires, bool force);

extern void clockevents_suspend(void);
extern void clockevents_resume(void);

#ifdef CONFIG_GENERIC_CLOCKEVENTS
extern int clockevents_notify(unsigned long reason, void *arg);
#else
static inline int clockevents_notify(unsigned long reason, void *arg) { return 0; }
static inline int tick_check_broadcast_expired(void) { return 0; }
#endif

#else /* CONFIG_GENERIC_CLOCKEVENTS */

static inline int clockevents_notify(unsigned long reason, void *arg) { return 0; }
static inline void tick_setup_hrtimer_broadcast(void) {};
static inline void clockevents_suspend(void) {}
static inline void clockevents_resume(void) {}

#endif

#endif
