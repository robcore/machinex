#!/bin/bash
patch -p1 -R < "/root/machinex/patches/0100-fixed.patch"
patch -p1 -R < "/root/machinex/patches/0099-nah.patch"
patch -p1 -R < "/root/machinex/patches/0098-REVERT-all-of-that-we-need-this-code-for-now.patch"
patch -p1 -R < "/root/machinex/patches/0097-there-we-go-knew-i-was-missing-something.patch"
patch -p1 -R < "/root/machinex/patches/0096-name-changed-back-to-runnable_avg_period.patch"
patch -p1 -R < "/root/machinex/patches/0095-some-more-hmp-removal.patch"
patch -p1 -R < "/root/machinex/patches/0094-REVERT-sched-Add-sched_avg-utilization_avg_contrib.patch"
patch -p1 -R < "/root/machinex/patches/0093-revert-that.patch"
patch -p1 -R < "/root/machinex/patches/0092-sched-Calculate-CPUs-usage-statistic.patch"
patch -p1 -R < "/root/machinex/patches/0091-some-fixups-to-the-reversions-including-our-hacked-i.patch"
patch -p1 -R < "/root/machinex/patches/0090-REVERT-more-tinkering.patch"
patch -p1 -R < "/root/machinex/patches/0089-removed-the-entirety-of-HMP-shit.patch"
patch -p1 -R < "/root/machinex/patches/0088-right-i-think-kfree-handles-everything-now.patch"
patch -p1 -R < "/root/machinex/patches/0087-futex-Unlock-hb-lock-in-futex_wait_requeue_pi-error.patch"
patch -p1 -R < "/root/machinex/patches/0086-dox.patch"
patch -p1 -R < "/root/machinex/patches/0085-module-add-within_module-function.patch"
patch -p1 -R < "/root/machinex/patches/0084-drivers-input-evdev.c-dont-kfree-a-vmalloc-address.-.patch"
patch -p1 -R < "/root/machinex/patches/0083-mm-frontswap-invalidate-expired-data-on-a-dup-store-.patch"
patch -p1 -R < "/root/machinex/patches/0082-TEST-sdcardfs-Remove-stale-dentries-when-reusing-an-.patch"
patch -p1 -R < "/root/machinex/patches/0081-msm-kgsl-fix-sync-file-error-handling.patch"
patch -p1 -R < "/root/machinex/patches/0080-use-seq_puts-instead-of-prints-in-proc-vmstat.patch"
patch -p1 -R < "/root/machinex/patches/0079-Revert-sched-Avoid-throttle_cfs_rq-racing-with-perio.patch"
patch -p1 -R < "/root/machinex/patches/0078-kbuild-Remove-useless-warning-while-appending-KCFLAG.patch"