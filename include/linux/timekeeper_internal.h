/*
 * You SHOULD NOT be including this unless you're vsyscall
 * handling code or timekeeping internal code!
 */

#ifndef _LINUX_TIMEKEEPER_INTERNAL_H
#define _LINUX_TIMEKEEPER_INTERNAL_H

#include <linux/clocksource.h>
#include <linux/jiffies.h>
#include <linux/time.h>

extern ktime_t ntp_get_next_leap(void);

/*
 * Structure holding internal timekeeping values.
 *
 * Note: wall_to_monotonic is what we need to add to xtime (or xtime
 * corrected for sub jiffie times) to get to monotonic time.
 * Monotonic is pegged at zero at system boot time, so
 * wall_to_monotonic will be negative, however, we will ALWAYS keep
 * the tv_nsec part positive so we can use the usual normalization.
 *
 * wall_to_monotonic is moved after resume from suspend for the
 * monotonic time not to jump. We need to add total_sleep_time to
 * wall_to_monotonic to get the real boot based time offset.
 *
 * - wall_to_monotonic is no longer the boot time, getboottime must be
 * used instead.
 */
struct tk_read_base {
	/* Current clocksource used for timekeeping. */
	struct clocksource	*clock;
	/* Read function of @clock */
	cycle_t			(*read)(struct clocksource *cs);
	/* Bitmask for two's complement subtraction of non 64bit counters */
	cycle_t			mask;
	/* Last cycle value */
	cycle_t			cycle_last;
	/* NTP adjusted clock multiplier */
	u32			mult;
	/* The shift value of the current clocksource. */
	u32			shift;
	/* Clock shifted nano seconds */
	u64			xtime_nsec;

	/* Monotonic base time */
	ktime_t			base_mono;
};

/**
 * struct timekeeper - Structure holding internal timekeeping values.
 * @tkr:		The readout base structure
 * @xtime_sec:		Current CLOCK_REALTIME time in seconds
 * @wall_to_monotonic:	CLOCK_REALTIME to CLOCK_MONOTONIC offset
 * @offs_real:		Offset clock monotonic -> clock realtime
 * @offs_boot:		Offset clock monotonic -> clock boottime
 * @offs_tai:		Offset clock monotonic -> clock tai
 * @tai_offset:		The current UTC to TAI offset in seconds
 * @base_raw:		Monotonic raw base time in ktime_t format
 * @raw_time:		Monotonic raw base time in timespec64 format
 * @cycle_interval:	Number of clock cycles in one NTP interval
 * @xtime_interval:	Number of clock shifted nano seconds in one NTP
 *			interval.
 * @xtime_remainder:	Shifted nano seconds left over when rounding
 *			@cycle_interval
 * @raw_interval:	Raw nano seconds accumulated per NTP interval.
 * @ntp_error:		Difference between accumulated time and NTP time in ntp
 *			shifted nano seconds.
 * @ntp_error_shift:	Shift conversion between clock shifted nano seconds and
 *			ntp shifted nano seconds.
 *
 * Note: For timespec(64) based interfaces wall_to_monotonic is what
 * we need to add to xtime (or xtime corrected for sub jiffie times)
 * to get to monotonic time.  Monotonic is pegged at zero at system
 * boot time, so wall_to_monotonic will be negative, however, we will
 * ALWAYS keep the tv_nsec part positive so we can use the usual
 * normalization.
 *
 * wall_to_monotonic is moved after resume from suspend for the
 * monotonic time not to jump. We need to add total_sleep_time to
 * wall_to_monotonic to get the real boot based time offset.
 *
 * wall_to_monotonic is no longer the boot time, getboottime must be
 * used instead.
 */
struct timekeeper {
	struct tk_read_base	tkr;
	/* Current CLOCK_REALTIME time in seconds */
	u64			xtime_sec;
	/* CLOCK_REALTIME to CLOCK_MONOTONIC offset */
	struct timespec64	wall_to_monotonic;

	/* Offset clock monotonic -> clock realtime */
	ktime_t			offs_real;
	/* Offset clock monotonic -> clock boottime */
	ktime_t			offs_boot;
	/* Offset clock monotonic -> clock tai */
	ktime_t			offs_tai;

	/* time spent in suspend */
	struct timespec64	total_sleep_time;
	/* The current UTC to TAI offset in seconds */
	s32			tai_offset;

	/* Monotonic raw base time */
	ktime_t			base_raw;

	/* The raw monotonic time for the CLOCK_MONOTONIC_RAW posix clock. */
	struct timespec64	raw_time;
	/* CLOCK_MONOTONIC time value of a pending leap-second*/
	ktime_t	next_leap_ktime;
	/* Number of clock cycles in one NTP interval. */
	cycle_t			cycle_interval;
	/* Number of clock shifted nano seconds in one NTP interval. */
	u64			xtime_interval;
	/* shifted nano seconds left over when rounding cycle_interval */
	s64			xtime_remainder;
	/* Raw nano seconds accumulated per NTP interval. */
	u32			raw_interval;

	/*
	 * Difference between accumulated time and NTP time in ntp
	 * shifted nano seconds.
	 */
	s64			ntp_error;
	/* Shift conversion between clock shifted nano seconds and
	 * ntp shifted nano seconds. */
	u32			ntp_error_shift;
	u32			ntp_err_mult;

	/* The current time */
	struct timespec xtime;
	/* The ntp_tick_length() value currently being used.
	 * This cached copy ensures we consistently apply the tick
	 * length for an entire tick, as ntp_tick_length may change
	 * mid-tick, and we don't want to apply that new value to
	 * the tick in progress.
	 */
	u64			ntp_tick;
	/* Difference between accumulated time and NTP time in ntp
	 * shifted nano seconds. */
};

#ifdef CONFIG_GENERIC_TIME_VSYSCALL

extern void update_vsyscall(struct timekeeper *tk);
extern void update_vsyscall_tz(void);

#elif defined(CONFIG_GENERIC_TIME_VSYSCALL_OLD)

extern void update_vsyscall_old(struct timespec *ts, struct timespec *wtm,
						struct clocksource *c, u32 mult,
						struct timekeeper *tk);

extern void update_vsyscall_tz(void);

#else

static inline void update_vsyscall(struct timekeeper *tk)
{
}
static inline void update_vsyscall_tz(void)
{
}
#endif

#endif /* _LINUX_TIMEKEEPER_INTERNAL_H */
