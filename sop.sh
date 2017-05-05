#!/bin/bash

patch -p1 < "/root/machinex/patches/0061-sched-stop_machine-Fix-deadlock.patch"
patch -p1 < "/root/machinex/patches/0062-sched-add-static-key-to-preempt-notifiers.patch"
patch -p1 < "/root/machinex/patches/0063-sched-deadline-Optimize-pull_dl_task.patch"
patch -p1 < "/root/machinex/patches/0064-sched-deadline-Make-init_sched_dl_class-__init.patch"
patch -p1 < "/root/machinex/patches/0065-sched-deadline-Reduce-rq-lock-contention.patch"
patch -p1 < "/root/machinex/patches/0066-sched-deadline-Drop-duplicate-init_sched_dl_class.patch"
patch -p1 < "/root/machinex/patches/0067-sched-Remove-superfluous-resetting-of-the-p-dl_throt.patch"
patch -p1 < "/root/machinex/patches/0068-sched-deadline-Remove-needless-parameter-in-dl.patch"
patch -p1 < "/root/machinex/patches/0069-timers-Sanitize-catchup_timer_jiffies-usage.patch"
patch -p1 < "/root/machinex/patches/0070-timer-Remove-FIFO-guarantee.patch"