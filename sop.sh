#!/bin/bash

patch -p1 < "/root/machinex/patches/0083-sched-Fix-crash-trying-to-dequeue-enqueue-the-idle-t.patch"
patch -p1 < "/root/machinex/patches/0084-sched-core-Make-policy-testing-consistent.patch"
patch -p1 < "/root/machinex/patches/0085-numa-bs.patch"
patch -p1 < "/root/machinex/patches/0086-sched-fair-Remove-unnecessary-parameter-for-group_cl.patch"
patch -p1 < "/root/machinex/patches/0089-locking-pvqspinlock-Kick-the-PV-CPU-unconditionally.patch"
patch -p1 < "/root/machinex/patches/0090-rcu-Suppress-lockdep-false-positive-for-rcp-exp.patch"
patch -p1 < "/root/machinex/patches/0091-rcu-Use-rsp-expedited_wq-instead-of-sync_rcu_preempt.patch"
patch -p1 < "/root/machinex/patches/0092-rcu-Move-rcu_report_exp_rnp-to-allow-consolidation.patch"
patch -p1 < "/root/machinex/patches/0093-rcu-Consolidate-tree-setup-for-synchronize_rcu_exp.patch"
patch -p1 < "/root/machinex/patches/0094-rcu-Use-single-stage-IPI-algorithm-for-RCU-expedited.patch"
patch -p1 < "/root/machinex/patches/0095-rcu-Move-synchronize_sched_expedited.patch"
patch -p1 < "/root/machinex/patches/0096-rcu-Rename-qs_pending-to-core_needs_qs.patch"
patch -p1 < "/root/machinex/patches/0097-rcu-Invert-passed_quiesce-and-rename-to-cpu_no_qs.patch"
patch -p1 < "/root/machinex/patches/0098-rcu-Make-cpu_no_qs-be-a-union-for-aggregate-OR.patch"
patch -p1 < "/root/machinex/patches/0099-dox.patch"
patch -p1 < "/root/machinex/patches/0100-timers-Fix-data-race-in-timer_stats_account_timer.patch"
patch -p1 < "/root/machinex/patches/0101-futex-Force-hot-variables-into-a-single-cache-line.patch"

