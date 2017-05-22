#!/bin/bash
patch -p1 < "/root/machinex/patches/0001-turning-rt_runtime_sharing-back-on-by-default.patch"
patch -p1 < "/root/machinex/patches/0008-rcu-Remove-cond_resched-from-Tiny-synchronize_sched.patch"
patch -p1 < "/root/machinex/patches/0009-rcu-Narrow-early-boot-window-of-illegal-synchronous-.patch"
patch -p1 < "/root/machinex/patches/0011-locktorture-Fix-potential-memory-leak-with-rw-lock.patch"
patch -p1 < "/root/machinex/patches/0013-rcu-Abstract-the-dynticks-momentary-idle-operation.patch"
patch -p1 < "/root/machinex/patches/0014-rcu-Abstract-the-dynticks-snapshot-operation.patch"
patch -p1 < "/root/machinex/patches/0015-rcu-update-Make-RCU_EXPEDITE_BOOT-be-the-default.patch"

