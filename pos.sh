#!/bin/bash
patch -p1 -R < "/root/machinex/patches/0200-REVERT-sched-fair-Rework-sched_fair-time-accounting.patch"
patch -p1 -R < "/root/machinex/patches/0199-REVERT-sched-class-task_dead-method.patch"
patch -p1 -R < "/root/machinex/patches/0198-REVERT-sched-Add-sched_class-task_dead-method.patch"
patch -p1 -R < "/root/machinex/patches/0197-same-with-avoid-NULL-deref-on-sd_busy-looked-familia.patch"
patch -p1 -R < "/root/machinex/patches/0196-REVERT-sched-assign-correct-scheduling-domain-to-sd_.patch"
patch -p1 -R < "/root/machinex/patches/0195-dox.patch"
patch -p1 -R < "/root/machinex/patches/0194-dox.patch"
patch -p1 -R < "/root/machinex/patches/0193-timekeeping-Fix-CLOCK_TAI-timer-nanosleep-delays.patch"
patch -p1 -R < "/root/machinex/patches/0192-timekeeping-Fix-lost-updates-to-tai-adjustment.patch"
patch -p1 -R < "/root/machinex/patches/0191-8064-lt-kernel-patches-5patches-7335-timekeeping-Fix.patch"
patch -p1 -R < "/root/machinex/patches/0190-libata-freezer-avoid-block-device-removal-when-froze.patch"
patch -p1 -R < "/root/machinex/patches/0189-mutexes-Give-more-informative-mutex-warning.patch"
patch -p1 -R < "/root/machinex/patches/0188-sched-Assign-correct-scheduling-domain-to-sd_llc.patch"
patch -p1 -R < "/root/machinex/patches/0187-sched-fair-Rework-sched_fair-time-accounting.patch"
patch -p1 -R < "/root/machinex/patches/0186-remove-extern-it-is-redundant.patch"
patch -p1 -R < "/root/machinex/patches/0185-rcu-Warn-on-allegedly-impossible-rcu_read_unlock.patch"
patch -p1 -R < "/root/machinex/patches/0184-futex-move-user-address-verification-up-to-common-co.patch"
patch -p1 -R < "/root/machinex/patches/0183-dox.patch"
patch -p1 -R < "/root/machinex/patches/0182-posix-timers-Convert-abuses-of-BUG_ON-to-WARN_ON.patch"
patch -p1 -R < "/root/machinex/patches/0181-posix-timers-Remove-remaining-uses-of-tasklist_lock.patch"
patch -p1 -R < "/root/machinex/patches/0180-same.patch"
patch -p1 -R < "/root/machinex/patches/0179-posix-timers-Use-sighand-lock-instead-of-tasklist.patch"
patch -p1 -R < "/root/machinex/patches/0178-posix-timers-Consolidate-posix_cpu_clock_get.patch"
patch -p1 -R < "/root/machinex/patches/0177-posix-timers-Remove-useless-clock-sample-on-timer.patch"
patch -p1 -R < "/root/machinex/patches/0176-posix-timers-Remove-dead-task-special-case.patch"
patch -p1 -R < "/root/machinex/patches/0175-posix-timers-Cleanup-reaped-target-handling.patch"
patch -p1 -R < "/root/machinex/patches/0174-posix-timers-Remove-dead-process-posix-cpu-timers.patch"
patch -p1 -R < "/root/machinex/patches/0173-posix-timers-Remove-dead-thread-posix-cpu-timers-cac.patch"
patch -p1 -R < "/root/machinex/patches/0172-posix-timers-Fix-full-dynticks-CPUs-kick-on-timer-re.patch"
patch -p1 -R < "/root/machinex/patches/0171-nohz-Fix-another-inconsistency-between-CONFIG_NO_HZ-.patch"
patch -p1 -R < "/root/machinex/patches/0170-kernel-extable-fix-address-checks-for-core_kernel.patch"
patch -p1 -R < "/root/machinex/patches/0169-sched-Add-sched_class-task_dead-method.patch"
patch -p1 -R < "/root/machinex/patches/0168-sched-fair-Move-load-idx-selection-in-find_idlest.patch"
patch -p1 -R < "/root/machinex/patches/0167-sched-Check-TASK_DEAD-rather-than-EXIT_DEAD-in-sched.patch"
patch -p1 -R < "/root/machinex/patches/0166-tasks-fork-Remove-unnecessary-child-exit_state.patch"
patch -p1 -R < "/root/machinex/patches/0165-ockdep-Be-nice-about-building-from-userspace.patch"
patch -p1 -R < "/root/machinex/patches/0164-sched-move-PREEMPT-depend.patch"
patch -p1 -R < "/root/machinex/patches/0163-sched-use-if-instead-of-while.patch"
patch -p1 -R < "/root/machinex/patches/0162-workqueue-swap-set_cpus_allowed_ptr-and-PF_NO_SETAFF.patch"
patch -p1 -R < "/root/machinex/patches/0161-dox.patch"
patch -p1 -R < "/root/machinex/patches/0160-sched-Avoid-NULL-dereference-on-sd_busy.patch"
patch -p1 -R < "/root/machinex/patches/0159-sched-Check-sched_domain-before-computing-group-powe.patch"
patch -p1 -R < "/root/machinex/patches/0158-perf-trace-Properly-use-u64-to-hold-event_id.patch"
patch -p1 -R < "/root/machinex/patches/0157-perf-Remove-fragile-swevent-hlist-optimization.patch"
patch -p1 -R < "/root/machinex/patches/0156-NOHZ-Check-for-nohz-active-instead-of-nohz-enabled.patch"
patch -p1 -R < "/root/machinex/patches/0155-kernel-provide-a-__smp_call_function_single-stub-for.patch"
patch -p1 -R < "/root/machinex/patches/0154-Remove-GENERIC_SMP_HELPERS-its-pretty-much-standard.patch"
patch -p1 -R < "/root/machinex/patches/0153-revert-softirq-Add-support-for-triggering-softirq-wo.patch"
patch -p1 -R < "/root/machinex/patches/0152-mm-conver-mm-ptes-to-atomic-long.patch"
patch -p1 -R < "/root/machinex/patches/0151-REVERT-that.patch"
patch -p1 -R < "/root/machinex/patches/0150-taskstats-use-genl_register_family_with_ops.patch"
patch -p1 -R < "/root/machinex/patches/0149-genirq-Prevent-spurious-detection-for-unconditionall.patch"
patch -p1 -R < "/root/machinex/patches/0148-locking-lockdep-Mark-__lockdep_count_forward_deps-as.patch"
