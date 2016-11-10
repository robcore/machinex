#include <linux/export.h>
#include <linux/sched.h>
#include <linux/tsacct_kern.h>
#include <linux/kernel_stat.h>
#include <linux/static_key.h>
#include "sched.h"


#ifdef CONFIG_IRQ_TIME_ACCOUNTING

/*
 * There are no locks covering percpu hardirq/softirq time.
 * They are only modified in account_system_vtime, on corresponding CPU
 * with interrupts disabled. So, writes are safe.
 * They are read and saved off onto struct rq in update_rq_clock().
 * This may result in other CPU reading this CPU's irq time and can
 * race with irq/account_system_vtime on this CPU. We would either get old
 * or new value with a side effect of accounting a slice of irq time to wrong
 * task when irq is in progress while we read rq->clock. That is a worthy
 * compromise in place of having locks on each irq in account_system_time.
 */
DEFINE_PER_CPU(u64, cpu_hardirq_time);
DEFINE_PER_CPU(u64, cpu_softirq_time);

static DEFINE_PER_CPU(u64, irq_start_time);
static int sched_clock_irqtime;

void enable_sched_clock_irqtime(void)
{
	sched_clock_irqtime = 1;
}

void disable_sched_clock_irqtime(void)
{
	sched_clock_irqtime = 0;
}

#ifndef CONFIG_64BIT
DEFINE_PER_CPU(seqcount_t, irq_time_seq);
#endif /* CONFIG_64BIT */

/*
 * Called before incrementing preempt_count on {soft,}irq_enter
 * and before decrementing preempt_count on {soft,}irq_exit.
 */
void account_system_vtime(struct task_struct *curr)
{
	unsigned long flags;
	s64 delta;
	int cpu;

	if (!sched_clock_irqtime)
		return;

	local_irq_save(flags);

	cpu = smp_processor_id();
	delta = sched_clock_cpu(cpu) - __this_cpu_read(irq_start_time);
	__this_cpu_add(irq_start_time, delta);

	irq_time_write_begin();
	/*
	 * We do not account for softirq time from ksoftirqd here.
	 * We want to continue accounting softirq time to ksoftirqd thread
	 * in that case, so as not to confuse scheduler with a special task
	 * that do not consume any time, but still wants to run.
	 */
	if (hardirq_count())
		__this_cpu_add(cpu_hardirq_time, delta);
	else if (in_serving_softirq() && curr != this_cpu_ksoftirqd())
		__this_cpu_add(cpu_softirq_time, delta);

	irq_time_write_end();
	local_irq_restore(flags);
}
EXPORT_SYMBOL_GPL(account_system_vtime);

static int irqtime_account_hi_update(void)
{
	u64 *cpustat = kcpustat_this_cpu->cpustat;
	unsigned long flags;
	u64 latest_ns;
	int ret = 0;

	local_irq_save(flags);
	latest_ns = this_cpu_read(cpu_hardirq_time);
	if (nsecs_to_cputime64(latest_ns) > cpustat[CPUTIME_IRQ])
		ret = 1;
	local_irq_restore(flags);
	return ret;
}

static int irqtime_account_si_update(void)
{
	u64 *cpustat = kcpustat_this_cpu->cpustat;
	unsigned long flags;
	u64 latest_ns;
	int ret = 0;

	local_irq_save(flags);
	latest_ns = this_cpu_read(cpu_softirq_time);
	if (nsecs_to_cputime64(latest_ns) > cpustat[CPUTIME_SOFTIRQ])
		ret = 1;
	local_irq_restore(flags);
	return ret;
}

#else /* CONFIG_IRQ_TIME_ACCOUNTING */

#define sched_clock_irqtime	(0)

#endif /* !CONFIG_IRQ_TIME_ACCOUNTING */

static inline void task_group_account_field(struct task_struct *p, int index,
					    u64 tmp)
{
#ifdef CONFIG_CGROUP_CPUACCT
	struct kernel_cpustat *kcpustat;
	struct cpuacct *ca;
#endif
	/*
	 * Since all updates are sure to touch the root cgroup, we
	 * get ourselves ahead and touch it first. If the root cgroup
	 * is the only cgroup, then nothing else should be necessary.
	 *
	 */
	__this_cpu_add(kernel_cpustat.cpustat[index], tmp);

#ifdef CONFIG_CGROUP_CPUACCT
	if (unlikely(!cpuacct_subsys.active))
		return;

	rcu_read_lock();
	ca = task_ca(p);
	while (ca && (ca != &root_cpuacct)) {
		kcpustat = this_cpu_ptr(ca->cpustat);
		kcpustat->cpustat[index] += tmp;
		ca = parent_ca(ca);
	}
	rcu_read_unlock();
#endif
}

/*
 * Account user cpu time to a process.
 * @p: the process that the cpu time gets accounted to
 * @cputime: the cpu time spent in user space since the last update
 * @cputime_scaled: cputime scaled by cpu frequency
 */
void account_user_time(struct task_struct *p, cputime_t cputime,
		       cputime_t cputime_scaled)
{
	int index;

	/* Add user time to process. */
	p->utime += cputime;
	p->utimescaled += cputime_scaled;
	account_group_user_time(p, cputime);

	index = (TASK_NICE(p) > 0) ? CPUTIME_NICE : CPUTIME_USER;

	/* Add user time to cpustat. */
	task_group_account_field(p, index, (__force u64) cputime);

	/* Account for user time used */
	acct_account_cputime(p);
}

/*
 * Account guest cpu time to a process.
 * @p: the process that the cpu time gets accounted to
 * @cputime: the cpu time spent in virtual machine since the last update
 * @cputime_scaled: cputime scaled by cpu frequency
 */
static void account_guest_time(struct task_struct *p, cputime_t cputime,
			       cputime_t cputime_scaled)
{
	u64 *cpustat = kcpustat_this_cpu->cpustat;

	/* Add guest time to process. */
	p->utime += cputime;
	p->utimescaled += cputime_scaled;
	account_group_user_time(p, cputime);
	p->gtime += cputime;

	/* Add guest time to cpustat. */
	if (TASK_NICE(p) > 0) {
		cpustat[CPUTIME_NICE] += (__force u64) cputime;
		cpustat[CPUTIME_GUEST_NICE] += (__force u64) cputime;
	} else {
		cpustat[CPUTIME_USER] += (__force u64) cputime;
		cpustat[CPUTIME_GUEST] += (__force u64) cputime;
	}
}

/*
 * Account system cpu time to a process and desired cpustat field
 * @p: the process that the cpu time gets accounted to
 * @cputime: the cpu time spent in kernel space since the last update
 * @cputime_scaled: cputime scaled by cpu frequency
 * @target_cputime64: pointer to cpustat field that has to be updated
 */
static inline
void __account_system_time(struct task_struct *p, cputime_t cputime,
			cputime_t cputime_scaled, int index)
{
	/* Add system time to process. */
	p->stime += cputime;
	p->stimescaled += cputime_scaled;
	account_group_system_time(p, cputime);

	/* Add system time to cpustat. */
	task_group_account_field(p, index, (__force u64) cputime);

	/* Account for system time used */
	acct_account_cputime(p);
}

/*
 * Account system cpu time to a process.
 * @p: the process that the cpu time gets accounted to
 * @hardirq_offset: the offset to subtract from hardirq_count()
 * @cputime: the cpu time spent in kernel space since the last update
 * @cputime_scaled: cputime scaled by cpu frequency
 */
void account_system_time(struct task_struct *p, int hardirq_offset,
			 cputime_t cputime, cputime_t cputime_scaled)
{
	int index;

	if ((p->flags & PF_VCPU) && (irq_count() - hardirq_offset == 0)) {
		account_guest_time(p, cputime, cputime_scaled);
		return;
	}

	if (hardirq_count() - hardirq_offset)
		index = CPUTIME_IRQ;
	else if (in_serving_softirq())
		index = CPUTIME_SOFTIRQ;
	else
		index = CPUTIME_SYSTEM;

	__account_system_time(p, cputime, cputime_scaled, index);
}

/*
 * Account for involuntary wait time.
 * @cputime: the cpu time spent in involuntary wait
 */
void account_steal_time(cputime_t cputime)
{
	u64 *cpustat = kcpustat_this_cpu->cpustat;

	cpustat[CPUTIME_STEAL] += (__force u64) cputime;
}

/*
 * Account for idle time.
 * @cputime: the cpu time spent in idle wait
 */
void account_idle_time(cputime_t cputime)
{
	u64 *cpustat = kcpustat_this_cpu->cpustat;
	struct rq *rq = this_rq();

	if (atomic_read(&rq->nr_iowait) > 0)
		cpustat[CPUTIME_IOWAIT] += (__force u64) cputime;
	else
		cpustat[CPUTIME_IDLE] += (__force u64) cputime;
}

static __always_inline bool steal_account_process_tick(void)
{
#ifdef CONFIG_PARAVIRT
	if (static_key_false(&paravirt_steal_enabled)) {
		u64 steal, st = 0;

		steal = paravirt_steal_clock(smp_processor_id());
		steal -= this_rq()->prev_steal_time;

		st = steal_ticks(steal);
		this_rq()->prev_steal_time += st * TICK_NSEC;

		account_steal_time(st);
		return st;
	}
#endif
	return false;
}

/*
 * Accumulate raw cputime values of dead tasks (sig->[us]time) and live
 * tasks (sum on group iteration) belonging to @tsk's group.
 */
void thread_group_cputime(struct task_struct *tsk, struct task_cputime *times)
{
	struct signal_struct *sig = tsk->signal;
	cputime_t utime, stime;
	struct task_struct *t;

	times->utime = sig->utime;
	times->stime = sig->stime;
	times->sum_exec_runtime = sig->sum_sched_runtime;

	rcu_read_lock();
	/* make sure we can trust tsk->thread_group list */
	if (!likely(pid_alive(tsk)))
		goto out;

	t = tsk;
	for_each_thread(tsk, t) {
		task_cputime(t, &utime, &stime);
		times->utime += utime;
		times->stime += stime;
		times->sum_exec_runtime += task_sched_runtime(t);
	}
out:
	rcu_read_unlock();
}

#ifndef CONFIG_VIRT_CPU_ACCOUNTING

#ifdef CONFIG_IRQ_TIME_ACCOUNTING
/*
 * Account a tick to a process and cpustat
 * @p: the process that the cpu time gets accounted to
 * @user_tick: is the tick from userspace
 * @rq: the pointer to rq
 *
 * Tick demultiplexing follows the order
 * - pending hardirq update
 * - pending softirq update
 * - user_time
 * - idle_time
 * - system time
 *   - check for guest_time
 *   - else account as system_time
 *
 * Check for hardirq is done both for system and user time as there is
 * no timer going off while we are on hardirq and hence we may never get an
 * opportunity to update it solely in system time.
 * p->stime and friends are only updated on system time and not on irq
 * softirq as those do not count in task exec_runtime any more.
 */
static void irqtime_account_process_tick(struct task_struct *p, int user_tick,
						struct rq *rq)
{
	cputime_t one_jiffy_scaled = cputime_to_scaled(cputime_one_jiffy);
	u64 *cpustat = kcpustat_this_cpu->cpustat;

	if (steal_account_process_tick())
		return;

	if (irqtime_account_hi_update()) {
		cpustat[CPUTIME_IRQ] += (__force u64) cputime_one_jiffy;
	} else if (irqtime_account_si_update()) {
		cpustat[CPUTIME_SOFTIRQ] += (__force u64) cputime_one_jiffy;
	} else if (this_cpu_ksoftirqd() == p) {
		/*
		 * ksoftirqd time do not get accounted in cpu_softirq_time.
		 * So, we have to handle it separately here.
		 * Also, p->stime needs to be updated for ksoftirqd.
		 */
		__account_system_time(p, cputime_one_jiffy, one_jiffy_scaled,
					CPUTIME_SOFTIRQ);
	} else if (user_tick) {
		account_user_time(p, cputime_one_jiffy, one_jiffy_scaled);
	} else if (p == rq->idle) {
		account_idle_time(cputime_one_jiffy);
	} else if (p->flags & PF_VCPU) { /* System time or guest time */
		account_guest_time(p, cputime_one_jiffy, one_jiffy_scaled);
	} else {
		__account_system_time(p, cputime_one_jiffy, one_jiffy_scaled,
					CPUTIME_SYSTEM);
	}
}

static void irqtime_account_idle_ticks(int ticks)
{
	int i;
	struct rq *rq = this_rq();

	for (i = 0; i < ticks; i++)
		irqtime_account_process_tick(current, 0, rq);
}
#else /* CONFIG_IRQ_TIME_ACCOUNTING */
static void irqtime_account_idle_ticks(int ticks) {}
static void irqtime_account_process_tick(struct task_struct *p, int user_tick,
						struct rq *rq) {}
#endif /* CONFIG_IRQ_TIME_ACCOUNTING */

/*
 * Account a single tick of cpu time.
 * @p: the process that the cpu time gets accounted to
 * @user_tick: indicates if the tick is a user or a system tick
 */
void account_process_tick(struct task_struct *p, int user_tick)
{
	cputime_t one_jiffy_scaled = cputime_to_scaled(cputime_one_jiffy);
	struct rq *rq = this_rq();

	if (sched_clock_irqtime) {
		irqtime_account_process_tick(p, user_tick, rq);
		return;
	}

	if (steal_account_process_tick())
		return;

	if (user_tick)
		account_user_time(p, cputime_one_jiffy, one_jiffy_scaled);
	else if ((p != rq->idle) || (irq_count() != HARDIRQ_OFFSET))
		account_system_time(p, HARDIRQ_OFFSET, cputime_one_jiffy,
				    one_jiffy_scaled);
	else
		account_idle_time(cputime_one_jiffy);
}

/*
 * Account multiple ticks of steal time.
 * @p: the process from which the cpu time has been stolen
 * @ticks: number of stolen ticks
 */
void account_steal_ticks(unsigned long ticks)
{
	account_steal_time(jiffies_to_cputime(ticks));
}

/*
 * Account multiple ticks of idle time.
 * @ticks: number of stolen ticks
 */
void account_idle_ticks(unsigned long ticks)
{

	if (sched_clock_irqtime) {
		irqtime_account_idle_ticks(ticks);
		return;
	}

	account_idle_time(jiffies_to_cputime(ticks));
}

#endif

/*
 * Use precise platform statistics if available:
 */
#ifdef CONFIG_VIRT_CPU_ACCOUNTING
void task_cputime_adjusted(struct task_struct *p, cputime_t *ut, cputime_t *st)
{
	*ut = p->utime;
	*st = p->stime;
}

void thread_group_cputime_adjusted(struct task_struct *p, cputime_t *ut, cputime_t *st)
{
	struct task_cputime cputime;

	thread_group_cputime(p, &cputime);

	*ut = cputime.utime;
	*st = cputime.stime;
}
#else

/*
 * Perform (stime * rtime) / total, but avoid multiplication overflow by
 * loosing precision when the numbers are big.
 */
static cputime_t scale_stime(u64 stime, u64 rtime, u64 total)
{
	u64 scaled;

	for (;;) {
		/* Make sure "rtime" is the bigger of stime/rtime */
		if (stime > rtime)
			swap(rtime, stime);

		/* Make sure 'total' fits in 32 bits */
		if (total >> 32)
			goto drop_precision;

		/* Does rtime (and thus stime) fit in 32 bits? */
		if (!(rtime >> 32))
			break;

		/* Can we just balance rtime/stime rather than dropping bits? */
		if (stime >> 31)
			goto drop_precision;

		/* We can grow stime and shrink rtime and try to make them both fit */
		stime <<= 1;
		rtime >>= 1;
		continue;

drop_precision:
		/* We drop from rtime, it has more bits than stime */
		rtime >>= 1;
		total >>= 1;
	}

	/*
	 * Make sure gcc understands that this is a 32x32->64 multiply,
	 * followed by a 64/32->64 divide.
	 */
	scaled = div_u64((u64) (u32) stime * (u64) (u32) rtime, (u32)total);
	return (__force cputime_t) scaled;
}

/*
 * Adjust tick based cputime random precision against scheduler
 * runtime accounting.
 */
static void cputime_adjust(struct task_cputime *curr,
			   struct cputime *prev,
			   cputime_t *ut, cputime_t *st)
{
	cputime_t rtime, stime, utime;

	/*
	 * Tick based cputime accounting depend on random scheduling
	 * timeslices of a task to be interrupted or not by the timer.
	 * Depending on these circumstances, the number of these interrupts
	 * may be over or under-optimistic, matching the real user and system
	 * cputime with a variable precision.
	 *
	 * Fix this by scaling these tick based values against the total
	 * runtime accounted by the CFS scheduler.
	 */
	rtime = nsecs_to_cputime(curr->sum_exec_runtime);

	/*
	 * Update userspace visible utime/stime values only if actual execution
	 * time is bigger than already exported. Note that can happen, that we
	 * provided bigger values due to scaling inaccuracy on big numbers.
	 */
	if (prev->stime + prev->utime >= rtime)
		goto out;

	stime = curr->stime;
	utime = curr->utime;

	if (utime == 0) {
		stime = rtime;
	} else if (stime == 0) {
		utime = rtime;
	} else {
		cputime_t total = stime + utime;

		stime = scale_stime((__force u64)stime,
				    (__force u64)rtime, (__force u64)total);
		utime = rtime - stime;
	}

	/*
	 * If the tick based count grows faster than the scheduler one,
	 * the result of the scaling may go backward.
	 * Let's enforce monotonicity.
	 */
	prev->stime = max(prev->stime, stime);
	prev->utime = max(prev->utime, utime);

out:
	*ut = prev->utime;
	*st = prev->stime;
}

void task_cputime_adjusted(struct task_struct *p, cputime_t *ut, cputime_t *st)
{
	struct task_cputime cputime = {
		.sum_exec_runtime = p->se.sum_exec_runtime,
	};

	task_cputime(p, &cputime.utime, &cputime.stime);
	cputime_adjust(&cputime, &p->prev_cputime, ut, st);
}

/*
 * Must be called with siglock held.
 */
void thread_group_cputime_adjusted(struct task_struct *p, cputime_t *ut, cputime_t *st)
{
	struct task_cputime cputime;

	thread_group_cputime(p, &cputime);
	cputime_adjust(&cputime, &p->signal->prev_cputime, ut, st);
}
#endif
