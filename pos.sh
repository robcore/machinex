#!/bin/bash
patch -p1 -R < "/root/machinex/patches/0065-timer-Use-hlist-for-the-timer-wheel-hash-buckets.patch"
patch -p1 -R < "/root/machinex/patches/0064-timer-Remove-FIFO-guarantee.patch"
patch -p1 -R < "/root/machinex/patches/0063-timers-Sanitize-catchup_timer_jiffies-usage.patch"
patch -p1 -R < "/root/machinex/patches/0062-sched-deadline-Remove-needless-parameter-in-dl.patch"
patch -p1 -R < "/root/machinex/patches/0061-sched-Remove-superfluous-resetting-of-the-p-dl_throt.patch"
patch -p1 -R < "/root/machinex/patches/0060-sched-deadline-Drop-duplicate-init_sched_dl_class.patch"
patch -p1 -R < "/root/machinex/patches/0059-sched-deadline-Reduce-rq-lock-contention.patch"
patch -p1 -R < "/root/machinex/patches/0058-sched-deadline-Make-init_sched_dl_class-__init.patch"
patch -p1 -R < "/root/machinex/patches/0057-sched-deadline-Optimize-pull_dl_task.patch"
patch -p1 -R < "/root/machinex/patches/0056-sched-add-static-key-to-preempt-notifiers.patch"
patch -p1 -R < "/root/machinex/patches/0055-sched-stop_machine-Fix-deadlock.patch"
patch -p1 -R < "/root/machinex/patches/0054-debug.patch"
patch -p1 -R < "/root/machinex/patches/0053-locking-qrwlock-Don-t-contend-with-readers.patch"
patch -p1 -R < "/root/machinex/patches/0052-sched-lockdep-Employ-lock-pinning.patch"
patch -p1 -R < "/root/machinex/patches/0051-lockdep-Implement-lock-pinning.patch"
patch -p1 -R < "/root/machinex/patches/0050-lockdep-Simplify-lock_release.patch"
patch -p1 -R < "/root/machinex/patches/0049-sched-Streamline-the-task-migration-locking.patch"
patch -p1 -R < "/root/machinex/patches/0048-sched-dl-Convert-switched_-from-to-_dl-prio_changed_.patch"
patch -p1 -R < "/root/machinex/patches/0047-sched-dl-Remove-return-value-from-pull_dl_task.patch"
patch -p1 -R < "/root/machinex/patches/0046-sched-rt-Convert-switched_-from-to-_rt-prio_changed.patch"
patch -p1 -R < "/root/machinex/patches/0045-sched-rt-Remove-return-value-from-pull_rt_task.patch"
patch -p1 -R < "/root/machinex/patches/0044-sched-Allow-balance-callbacks-for-check_class_change.patch"
patch -p1 -R < "/root/machinex/patches/0043-sched-Use-replace-normalize_task-with-__sched_setsch.patch"
patch -p1 -R < "/root/machinex/patches/0042-sched-Replace-post_schedule-with-a-balance-callback.patch"
patch -p1 -R < "/root/machinex/patches/0041-hrtimer-Allow-hrtimer-function-to-free-the-timer.patch"
patch -p1 -R < "/root/machinex/patches/0040-hrtimer-Fix-hrtimer_is_queued-hole.patch"