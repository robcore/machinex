#!/bin/bash
/root/machinex/patches/0095-hrtimer-Allow-hrtimer-function-to-free-the-timer.patch
/root/machinex/patches/0094-seqcount-Introduce-raw_write_seqcount_barrier.patch
/root/machinex/patches/0093-seqcount-Rename-write_seqcount_barrier.patch
/root/machinex/patches/0092-lockdep-Revert-lockdep-check-in-raw_seqcount_begin.patch
/root/machinex/patches/0091-removed-my-legacy-seqlock-code.patch
/root/machinex/patches/0090-hrtimer-Fix-hrtimer_is_queued-hole.patch
/root/machinex/patches/0089-hrtimer-Remove-HRTIMER_STATE_MIGRATE.patch
/root/machinex/patches/0088-ah-HA-THIS-has-been-the-boot-killer.patch
/root/machinex/patches/0087-irq-Add-irq_set_chained_handler_and_data.patch
/root/machinex/patches/0086-alright.timekeeping-Copy-the-shadow-timekeeper-over.patch
/root/machinex/patches/0085-betcha-this-is-the-one.patch
/root/machinex/patches/0084-mmc-cond_resched-too.patch
/root/machinex/patches/0083-the-plot-thickens.patch
/root/machinex/patches/0082-arm-sleep-commits.patch
/root/machinex/patches/0081-irq-commits-which-i-suspend-are-the-culprit.patch
/root/machinex/patches/0080-genirq-Enhance-irq_data_to_desc-to-support-hierarchy.patch
/root/machinex/patches/0079-leap-commits.patch
/root/machinex/patches/0078-pos-just-using-for-reference-now.patch
/root/machinex/patches/0077-ntp-Introduce-and-use-SECS_PER_DAY-macro.patch
/root/machinex/patches/0076-possed.patch
/root/machinex/patches/0075-possing-so-i-can-find-this-the-old-fashioned-way.patch
/root/machinex/patches/0074-genirq-Remove-bogus-restriction-in-irq_move_mask_irq.patch
/root/machinex/patches/0073-possed.patch
/root/machinex/patches/0072-possing.patch
/root/machinex/patches/0071-timer-Use-hlist-for-the-timer-wheel-hash-buckets.patch
/root/machinex/patches/0070-timer-Remove-FIFO-guarantee.patch
/root/machinex/patches/0069-timers-Sanitize-catchup_timer_jiffies-usage.patch
/root/machinex/patches/0068-sched-deadline-Remove-needless-parameter-in-dl.patch
/root/machinex/patches/0067-sched-Remove-superfluous-resetting-of-the-p-dl_throt.patch
/root/machinex/patches/0066-sched-deadline-Drop-duplicate-init_sched_dl_class.patch
/root/machinex/patches/0065-sched-deadline-Reduce-rq-lock-contention.patch
/root/machinex/patches/0064-sched-deadline-Make-init_sched_dl_class-__init.patch
/root/machinex/patches/0063-sched-deadline-Optimize-pull_dl_task.patch
/root/machinex/patches/0062-sched-add-static-key-to-preempt-notifiers.patch
/root/machinex/patches/0061-sched-stop_machine-Fix-deadlock.patch
/root/machinex/patches/0060-debug.patch
/root/machinex/patches/0059-locking-qrwlock-Don-t-contend-with-readers.patch
/root/machinex/patches/0058-sched-lockdep-Employ-lock-pinning.patch
/root/machinex/patches/0057-lockdep-Implement-lock-pinning.patch
/root/machinex/patches/0056-lockdep-Simplify-lock_release.patch
/root/machinex/patches/0055-sched-Streamline-the-task-migration-locking.patch
/root/machinex/patches/0054-sched-dl-Convert-switched_-from-to-_dl-prio_changed_.patch
/root/machinex/patches/0053-sched-dl-Remove-return-value-from-pull_dl_task.patch
/root/machinex/patches/0052-sched-rt-Convert-switched_-from-to-_rt-prio_changed.patch
/root/machinex/patches/0051-sched-rt-Remove-return-value-from-pull_rt_task.patch
/root/machinex/patches/0050-sched-Allow-balance-callbacks-for-check_class_change.patch
/root/machinex/patches/0049-sched-Use-replace-normalize_task-with-__sched_setsch.patch
/root/machinex/patches/0048-sched-Replace-post_schedule-with-a-balance-callback.patch
/root/machinex/patches/0047-hrtimer-Allow-hrtimer-function-to-free-the-timer.patch
/root/machinex/patches/0046-hrtimer-Fix-hrtimer_is_queued-hole.patch
/root/machinex/patches/0045-those-are-fine.patch
/root/machinex/patches/0044-and-get-rid-of-doubled-next_leap_ktime-again.patch
/root/machinex/patches/0043-REVERT-timekeeping-fix-too.patch
/root/machinex/patches/0042-REVERT-time-Prevent-early-expiry-of-hrtimers-CLOCK_R.patch
/root/machinex/patches/0041-hrtimer-Remove-HRTIMER_STATE_MIGRATE.patch
/root/machinex/patches/0040-irq-Add-irq_set_chained_handler_and_data.patch
/root/machinex/patches/0039-k-that-should-fix-the-tk_update_leap_state-issues.ho.patch
/root/machinex/patches/0038-timekeeping-Copy-the-shadow-timekeeper-over-the-real.patch
/root/machinex/patches/0037-clockevents-Check-state-instead-of-mode-in-suspend-r.patch
/root/machinex/patches/0036-TEST-mmc-queue-prevent-soft-lockups.patch
/root/machinex/patches/0035-arm-fix-implicit-include-linux-init.h-in-entry-asm.patch
/root/machinex/patches/0034-lib-list_sort-use-late_initcall-to-hook-in-self-test.patch
/root/machinex/patches/0033-fs-notify-don-t-use-module_init-for-non-modular.patch
/root/machinex/patches/0032-mm-replace-module_init-usages-with-subsys_initcall.patch
/root/machinex/patches/0031-init-delete-the-__cpuinit-related-stubs.patch
/root/machinex/patches/0030-PM-clk-Print-acquired-clock-name.patch
/root/machinex/patches/0029-regmap-Fix-regmap_bulk_read-in-BE-mode.patch
/root/machinex/patches/0028-Kbuild-Add-ID-files-to-.gitignore.patch
/root/machinex/patches/0027-ARM-fix-new-BSYM-usage-introduced-via-for-arm-soc.patch
/root/machinex/patches/0026-ARM-fix-EFM32-build-breakage-caused-by-cpu_resume_ar.patch
/root/machinex/patches/0025-TEST-ARM-8389-1-Add-cpu_resume_arm-for-firmwares-tha.patch
/root/machinex/patches/0024-genirq-Introduce-helper-function-irq_data_get_affini.patch
/root/machinex/patches/0023-genirq-Introduce-helper-function-irq_data_get_node.patch
/root/machinex/patches/0022-genirq-Introduce-struct-irq_common_data-to-host-shar.patch
/root/machinex/patches/0021-genirq-Prevent-crash-in-irq_move_irq.patch
/root/machinex/patches/0020-genirq-Enhance-irq_data_to_desc-to-support-hierarchy.patch
/root/machinex/patches/0019-time-Prevent-early-expiry-of-hrtimers-CLOCK_REALTIME.patch
/root/machinex/patches/0018-ntp-Introduce-and-use-SECS_PER_DAY-macro.patch
/root/machinex/patches/0017-time-Move-clock_was_set_seq-update-before-updating-s.patch
/root/machinex/patches/0016-cosa-use-msecs_to_jiffies-for-conversions.patch
/root/machinex/patches/0015-sched-numa-Only-consider-less-busy-nodes-as-numa.patch
/root/machinex/patches/0014-lockdep-Fix-a-race-between-proc-lock_stat-and-module.patch
/root/machinex/patches/0013-ARM-8387-1-arm-mm-dma-mapping.c-Add-arm_coherent_dma.patch
/root/machinex/patches/0012-ARM-8388-1-tcm-Don-t-crash-when-TCM-banks-are-protec.patch
/root/machinex/patches/0011-iommu-Remove-function-name-from-pr_fmt.patch
/root/machinex/patches/0010-compat-cleanup-coding-in-compat_get_bitmap-and-compa.patch
/root/machinex/patches/0009-lib-Clarify-the-return-value-of-strnlen_user.patch
/root/machinex/patches/0008-clockevents-Use-set-get-state-helper-functions.patch
/root/machinex/patches/0007-clockevents-Provide-functions-to-set-and-get-the-sta.patch
/root/machinex/patches/0006-clockevents-Use-helpers-to-check-the-state-of-a-cloc.patch
/root/machinex/patches/0005-clockevents-Add-helpers-to-check-the-state-of-a-cloc.patch
/root/machinex/patches/0004-possed.patch
/root/machinex/patches/0003-possing.patch
/root/machinex/patches/0002-REVERT-clockevents-Check-state-instead-of-mode-in-su.patch
/root/machinex/patches/0001-fixup-a-double-declaration.patch
