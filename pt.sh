#!/bin/bash
#
patch -p1 -N -f --dry-run < /media/root/robcore/android/Hulk-Kernel-V2-ref/patches/0275-ARM-mpu-add-PMSA-related-registers-and-bitfields-to-.patch
patch -p1 -N -f --dry-run < /media/root/robcore/android/Hulk-Kernel-V2-ref/patches/0276-ARM-mpu-add-header-for-MPU-register-layouts-and-regi.patch
patch -p1 -N -f --dry-run < /media/root/robcore/android/Hulk-Kernel-V2-ref/patches/0277-Set-which-MPU-region-should-be-programmed.patch
patch -p1 -N -f --dry-run < /media/root/robcore/android/Hulk-Kernel-V2-ref/patches/0278-ARM-mpu-add-MPU-initialisation-for-secondary-cores.patch
patch -p1 -N -f --dry-run < /media/root/robcore/android/Hulk-Kernel-V2-ref/patches/0279-ARM-prefetch-remove-redundant-cc-clobber.patch
patch -p1 -N -f --dry-run < /media/root/robcore/android/Hulk-Kernel-V2-ref/patches/0280-ARM-atomics-prefetch-the-destination-word-for-write-.patch
patch -p1 -N -f --dry-run < /media/root/robcore/android/Hulk-Kernel-V2-ref/patches/0281-ARM-atomic64-fix-endian-ness-in-atomic.h.patch
patch -p1 -N -f --dry-run < /media/root/robcore/android/Hulk-Kernel-V2-ref/patches/0282-ARM-7983-1-atomics-implement-a-better-__atomic_add_u.patch
patch -p1 -N -f --dry-run < /media/root/robcore/android/Hulk-Kernel-V2-ref/patches/0283-ARM-7447-1-rwlocks-remove-unused-branch-labels-from-.patch
patch -p1 -N -f --dry-run < /media/root/robcore/android/Hulk-Kernel-V2-ref/patches/0284-ARM-nommu-define-dummy-TLB-operations-for-nommu-conf.patch
patch -p1 -N -f --dry-run < /media/root/robcore/android/Hulk-Kernel-V2-ref/patches/0285-ARM-nommu-add-stub-local_flush_bp_all-for-CONFIG_MMU.patch
patch -p1 -N -f --dry-run < /media/root/robcore/android/Hulk-Kernel-V2-ref/patches/0286-kgsl-Do-not-return-invalid-power-stats-when-the-devi.patch
patch -p1 -N -f --dry-run < /media/root/robcore/android/Hulk-Kernel-V2-ref/patches/0287-frandom-enable-frandom-support.patch
patch -p1 -N -f --dry-run < /media/root/robcore/android/Hulk-Kernel-V2-ref/patches/0288-tracing-syscalls-combined-patches-from-cyanogenmod.patch
patch -p1 -N -f --dry-run < /media/root/robcore/android/Hulk-Kernel-V2-ref/patches/0289-cpufreq-remove-prevention-of-managing-offline-cpus.patch
patch -p1 -N -f --dry-run < /media/root/robcore/android/Hulk-Kernel-V2-ref/patches/0290-ARM-fix-some-printk-formats-Blacklist-GCC-4.8.0-to-G.patch
patch -p1 -N -f --dry-run < /media/root/robcore/android/Hulk-Kernel-V2-ref/patches/0291-ARM-8180-1-mm-implement-no-highmem-fast-path-in-kmap.patch
patch -p1 -N -f --dry-run < /media/root/robcore/android/Hulk-Kernel-V2-ref/patches/0292-switch-do_fsync-to-fget_light.patch
patch -p1 -N -f --dry-run < /media/root/robcore/android/Hulk-Kernel-V2-ref/patches/0293-cpufreq-Create-a-macro-for-unlock_policy_rwsem-read-.patch
patch -p1 -N -f --dry-run < /media/root/robcore/android/Hulk-Kernel-V2-ref/patches/0294-cpufreq-Add-a-get_current_driver-helper.patch
patch -p1 -N -f --dry-run < /media/root/robcore/android/Hulk-Kernel-V2-ref/patches/0295-cpufreq-handle-cpufreq-being-disabled-for-all-export.patch
patch -p1 -N -f --dry-run < /media/root/robcore/android/Hulk-Kernel-V2-ref/patches/0296-CPUFREQ-use-bool-in-__cpufreq_cpu_get-sysfs.patch
patch -p1 -N -f --dry-run < /media/root/robcore/android/Hulk-Kernel-V2-ref/patches/0297-cpufreq-Make-sure-target-freq-is-within-limits.patch
patch -p1 -N -f --dry-run < /media/root/robcore/android/Hulk-Kernel-V2-ref/patches/0298-ARM-acpuclk-Inline-heavy-used-functions-to-reduce-ov.patch
patch -p1 -N -f --dry-run < /media/root/robcore/android/Hulk-Kernel-V2-ref/patches/0299-ARM-8191-1-decompressor-ensure-I-side-picks-up-reloc.patch
patch -p1 -N -f --dry-run < /media/root/robcore/android/Hulk-Kernel-V2-ref/patches/0300-zram-avoid-kunmap_atomic-of-a-NULL-pointer.patch
