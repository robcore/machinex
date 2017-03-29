/*
 * Generic entry point for the idle threads
 */
#include <linux/sched.h>
#include <linux/cpu.h>
#include <linux/cpuidle.h>
#include <linux/tick.h>
#include <linux/mm.h>
#include <linux/stackprotector.h>

#include <asm/tlb.h>

#include <trace/events/power.h>

int __read_mostly cpu_idle_force_poll;

void cpu_idle_poll_ctrl(bool enable)
{
	if (enable) {
		cpu_idle_force_poll++;
	} else {
		cpu_idle_force_poll--;
		WARN_ON_ONCE(cpu_idle_force_poll < 0);
	}
}

#ifdef CONFIG_GENERIC_IDLE_POLL_SETUP
static int __init cpu_idle_poll_setup(char *__unused)
{
	cpu_idle_force_poll = 1;
	return 1;
}
__setup("nohlt", cpu_idle_poll_setup);

static int __init cpu_idle_nopoll_setup(char *__unused)
{
	cpu_idle_force_poll = 0;
	return 1;
}
__setup("hlt", cpu_idle_nopoll_setup);
#endif

static inline int cpu_idle_poll(void)
{
	rcu_idle_enter();
	trace_cpu_idle_rcuidle(0, smp_processor_id());
	local_irq_enable();
	while (!tif_need_resched())
		cpu_relax();
	trace_cpu_idle_rcuidle(PWR_EVENT_EXIT, smp_processor_id());
	rcu_idle_exit();
	return 1;
}

/* Weak implementations for optional arch specific functions */
void __weak arch_cpu_idle_prepare(void) { }
void __weak arch_cpu_idle_enter(void) { }
void __weak arch_cpu_idle_exit(void) { }
void __weak arch_cpu_idle_dead(void) { }
void __weak arch_cpu_idle(void)
{
	cpu_idle_force_poll = 1;
	local_irq_enable();
}

/**
 * cpuidle_idle_call - the main idle loop
 *
 * NOTE: no locks or semaphores should be used here
 * return non-zero on failure
 */
static int cpuidle_idle_call(void)
{
	struct cpuidle_device *dev = __this_cpu_read(cpuidle_devices);
	struct cpuidle_driver *drv = cpuidle_get_cpu_driver(dev);
	int next_state, entered_state, ret;
	bool broadcast;

	stop_critical_timings();
	rcu_idle_enter();

	ret = cpuidle_enabled(drv, dev);
	if (ret < 0) {
		arch_cpu_idle();
		goto out;
	}

	/* ask the governor for the next state */
	next_state = cpuidle_select(drv, dev);

	if (need_resched()) {
		dev->last_residency = 0;
		/* give the governor an opportunity to reflect on the outcome */
		cpuidle_reflect(dev, next_state);
		local_irq_enable();
		goto out;
	}

	broadcast = !!(drv->states[next_state].flags & CPUIDLE_FLAG_TIMER_STOP);

	if (broadcast)
		clockevents_notify(CLOCK_EVT_NOTIFY_BROADCAST_ENTER, &dev->cpu);

	entered_state = cpuidle_enter(drv, dev, next_state);

	if (broadcast)
		clockevents_notify(CLOCK_EVT_NOTIFY_BROADCAST_EXIT, &dev->cpu);

	/* give the governor an opportunity to reflect on the outcome */
	cpuidle_reflect(dev, entered_state);
out:
	if (WARN_ON_ONCE(irqs_disabled()))
		local_irq_enable();

	rcu_idle_exit();
	start_critical_timings();

	return 0;
}

/*
 * Generic idle loop implementation
 */
static void cpu_idle_loop(void)
{
	int cpu = smp_processor_id();

	while (1) {
		tick_nohz_idle_enter();

		while (!need_resched()) {
			check_pgt_cache();
			rmb();

		if (cpu_is_offline(cpu))
			arch_cpu_idle_dead();

			local_irq_disable();
			arch_cpu_idle_enter();

			/*
			 * In poll mode we reenable interrupts and spin.
			 *
			 * Also if we detected in the wakeup from idle
			 * path that the tick broadcast device expired
			 * for us, we don't want to go deep idle as we
			 * know that the IPI is going to arrive right
			 * away
			 */
			if (cpu_idle_force_poll || tick_check_broadcast_expired()) {
				cpu_idle_poll();
			} else {
				if (!current_clr_polling_and_test()) {
					cpuidle_idle_call();
				} else {
					local_irq_enable();
				}
				__current_set_polling();
			}
			arch_cpu_idle_exit();
		}
		tick_nohz_idle_exit();
		schedule_preempt_disabled();
	}
}

void cpu_startup_entry(enum cpuhp_state state)
{
	/*
	 * This #ifdef needs to die, but it's too late in the cycle to
	 * make this generic (arm and sh have never invoked the canary
	 * init for the non boot cpus!). Will be fixed in 3.11
	 */
#ifdef CONFIG_X86
	/*
	 * If we're the non-boot CPU, nothing set the stack canary up
	 * for us. The boot CPU already has it initialized but no harm
	 * in doing it again. This is a good place for updating it, as
	 * we wont ever return from this function (so the invalid
	 * canaries already on the stack wont ever trigger).
	 */
	boot_init_stack_canary();
#endif
	__current_set_polling();
	arch_cpu_idle_prepare();
	cpu_idle_loop();
}

