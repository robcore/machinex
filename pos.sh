#!/bin/bash
patch -p1 -R < "/root/machinex/patches/0149-cpufreq-Clean-up-header-files-included-in-the-core.patch"
patch -p1 -R < "/root/machinex/patches/0148-cpufreq-Pass-policy-to-cpufreq_add_policy_cpu.patch"
patch -p1 -R < "/root/machinex/patches/0147-cpufreq-Avoid-double-kobject_put-for-the-same-kobjec.patch"
patch -p1 -R < "/root/machinex/patches/0146-cpufreq-Do-not-hold-driver-module-references-for-add.patch"
patch -p1 -R < "/root/machinex/patches/0145-cpufreq-Don-t-pass-CPU-to-cpufreq_add_dev_-symlink.patch"
patch -p1 -R < "/root/machinex/patches/0144-cpufreq-Remove-extra-variables-from-cpufreq_add_dev_.patch"
patch -p1 -R < "/root/machinex/patches/0143-cpufreq-Perform-light-weight-init-teardown.patch"
patch -p1 -R < "/root/machinex/patches/0142-cpufreq-Preserve-policy-structure-across-suspend-res.patch"
patch -p1 -R < "/root/machinex/patches/0141-cpufreq-Introduce-a-flag-frozen-to-separate-full-vs-.patch"
patch -p1 -R < "/root/machinex/patches/0140-cpufreq-Extract-the-handover-of-policy-cpu-to-a-help.patch"
patch -p1 -R < "/root/machinex/patches/0139-cpufreq-Add-helper-to-perform-alloc-free-of-policy-s.patch"
patch -p1 -R < "/root/machinex/patches/0138-cpufreq-Fix-misplaced-call-to-cpufreq_update_policy.patch"
patch -p1 -R < "/root/machinex/patches/0137-cpufreq-ondemand-Change-the-calculation-of-target-fr.patch"
patch -p1 -R < "/root/machinex/patches/0136-cpufreq-Fix-serialization-of-frequency-transitions.patch"
patch -p1 -R < "/root/machinex/patches/0135-fuck-nope.patch"
patch -p1 -R < "/root/machinex/patches/0134-cpufreq-make-sure-frequency-transitions-are-serializ.patch"
patch -p1 -R < "/root/machinex/patches/0133-cpufreq-make-__cpufreq_notify_transition-static.patch"
patch -p1 -R < "/root/machinex/patches/0132-what-did-i-do.patch"
patch -p1 -R < "/root/machinex/patches/0131-that-should-help-our-syscalls.patch"
patch -p1 -R < "/root/machinex/patches/0130-cpufreq-Simplify-userspace-governor.patch"
patch -p1 -R < "/root/machinex/patches/0129-cpufreq-remove-unnecessary-cpufreq_cpu_-get-put-call.patch"
patch -p1 -R < "/root/machinex/patches/0128-cpufreq-Dont-create-empty-sys-devices-system-cpu-cpu.patch"
patch -p1 -R < "/root/machinex/patches/0127-i-knew-this-was-coming-full-circle-but-man-this-was-.patch"
patch -p1 -R < "/root/machinex/patches/0126-cpufreq-governors-Move-get_governor_parent_kobj-to-c.patch"
patch -p1 -R < "/root/machinex/patches/0125-cpufreq-Add-EXPORT_SYMBOL_GPL-for-have_governor_per_.patch"
patch -p1 -R < "/root/machinex/patches/0124-cpufreq-Don-t-use-smp_processor_id-in-preemptible.patch"
patch -p1 -R < "/root/machinex/patches/0123-cpufreq-Fix-timer-workqueue-corruption.patch"
patch -p1 -R < "/root/machinex/patches/0122-cpufreq-Make-the-scaling_governor-sysfs-node-pollabl.patch"
patch -p1 -R < "/root/machinex/patches/0121-cpufreq-don-t-leave-stale-policy-pointer-in-cdbs.patch"
patch -p1 -R < "/root/machinex/patches/0120-msm-cpufreq-Only-apply-driver-limits-for-scaling_min.patch"
patch -p1 -R < "/root/machinex/patches/0119-cpufreq-rename-ignore_nice-as-ignore_nice_load.patch"
patch -p1 -R < "/root/machinex/patches/0118-cpufreq-Fix-cpufreq-driver-module-refcount-balance-a.patch"
patch -p1 -R < "/root/machinex/patches/0117-cpufreq-Revert-commit-2f7021a8-to-fix-CPU-hotplug.patch"
patch -p1 -R < "/root/machinex/patches/0116-cpufreq-Revert-commit-a66b2e-to-fix-suspend-resume.patch"
patch -p1 -R < "/root/machinex/patches/0115-cpufreq-Fix-cpufreq-regression-after-suspend-resume.patch"
patch -p1 -R < "/root/machinex/patches/0114-cpufreq-fix-NULL-pointer-deference-at-od_set_powersa.patch"
patch -p1 -R < "/root/machinex/patches/0113-cpufreq-protect-policy-cpus-from-offlining.patch"
patch -p1 -R < "/root/machinex/patches/0112-cpufreq-Drop-rwsem-lock-around-CPUFREQ_GOV_POLICY_EX.patch"
patch -p1 -R < "/root/machinex/patches/0111-cpufreq-Preserve-sysfs-files-across-suspend-resume.patch"
patch -p1 -R < "/root/machinex/patches/0110-cpufreq-ondemand-Remove-leftover-debug-line.patch"
patch -p1 -R < "/root/machinex/patches/0109-bs.patch"
patch -p1 -R < "/root/machinex/patches/0108-cpufreq-Issue-CPUFREQ_GOV_POLICY_EXIT-notifier-befor.patch"
patch -p1 -R < "/root/machinex/patches/0107-cpufreq-governors-Fix-CPUFREQ_GOV_POLICY_-INIT-EXIT.patch"
patch -p1 -R < "/root/machinex/patches/0106-FUCK-i-forgot-after-all-that-turns-out-it-never-work.patch"
patch -p1 -R < "/root/machinex/patches/0105-big-little-cpu0-bs.patch"
patch -p1 -R < "/root/machinex/patches/0104-cpufreq-Dont-call-__cpufreq_governor-for-drivers-wit.patch"
patch -p1 -R < "/root/machinex/patches/0103-cpufreq-Call-__cpufreq_governor-with-correct-policy.patch"
patch -p1 -R < "/root/machinex/patches/0102-cpufreq-ondemand-allow-custom-powersave_bias_target-.patch"
patch -p1 -R < "/root/machinex/patches/0101-cpufreq-convert-cpufreq_driver-to-using-RCU.patch"
patch -p1 -R < "/root/machinex/patches/0100-cpufreq-ARM-big-LITTLE-Add-generic-cpufreq-driver-an.patch"
patch -p1 -R < "/root/machinex/patches/0099-mostly-bs-some-dox.patch"
patch -p1 -R < "/root/machinex/patches/0098-bs.patch"
patch -p1 -R < "/root/machinex/patches/0097-cpufreq-update-notifier-for-platform-cpu-drivers-and.patch"
patch -p1 -R < "/root/machinex/patches/0096-cpufreq-conservative-Use-an-inline-function.patch"
patch -p1 -R < "/root/machinex/patches/0095-cpu0-bs.patch"
patch -p1 -R < "/root/machinex/patches/0094-cpufreq-conservative-Fix-the-logic-in-frequency-dec.patch"
patch -p1 -R < "/root/machinex/patches/0093-cpufreq-conservative-Fix-sampling_down_factor.patch"
patch -p1 -R < "/root/machinex/patches/0092-io-busy-bringup.patch"
patch -p1 -R < "/root/machinex/patches/0091-conserv-relation.patch"
patch -p1 -R < "/root/machinex/patches/0090-conservative-again-break-out-earlier-on-lowest-freq.patch"
patch -p1 -R < "/root/machinex/patches/0089-fix-a-leftover-derp-in-gov-header.patch"
patch -p1 -R < "/root/machinex/patches/0088-cpufreq-ondemand-Don-t-update-sample_type-if-we-dont.patch"
patch -p1 -R < "/root/machinex/patches/0087-cpufreq-governor-Set-MIN_LATENCY_MULTIPLIER-to-20.patch"
patch -p1 -R < "/root/machinex/patches/0086-cpufreq-governor-Implement-per-policy-instances-of-g.patch"
patch -p1 -R < "/root/machinex/patches/0085-Re-Add-per-policy-governor-init-exit-infrastructure.patch"
patch -p1 -R < "/root/machinex/patches/0084-cpufreq-Convert-the-cpufreq_driver_lock-to-a-rwlock.patch"
patch -p1 -R < "/root/machinex/patches/0083-cpufreq-stats-do-cpufreq_cpu_put-corresponding-to-cp.patch"
patch -p1 -R < "/root/machinex/patches/0082-dox.patch"
patch -p1 -R < "/root/machinex/patches/0081-cpufreq-Do-not-track-governor-name-for-scaling-drive.patch"
patch -p1 -R < "/root/machinex/patches/0080-cpufreq-Only-call-cpufreq_out_of_sync-for-driver-tha.patch"
patch -p1 -R < "/root/machinex/patches/0079-cpufreq-retrieve-current-frequency-from-scaling-driv.patch"
patch -p1 -R < "/root/machinex/patches/0078-cpufreq-Fix-locking-issues.patch"
patch -p1 -R < "/root/machinex/patches/0077-cpufreq-Remove-unused-HOTPLUG_CPU-code.patch"
patch -p1 -R < "/root/machinex/patches/0076-cpufreq-governors-Fix-WARN_ON-for-multi-policy-platf.patch"
patch -p1 -R < "/root/machinex/patches/0075-cpufreq-ondemand-Replace-down_differential-tuner-wit.patch"
patch -p1 -R < "/root/machinex/patches/0074-cpufreq-stats-Get-rid-of-CPUFREQ_STATDEVICE_ATTR.patch"
patch -p1 -R < "/root/machinex/patches/0073-cpufreq-Dont-check-cpu_online-policy-cpu.patch"
patch -p1 -R < "/root/machinex/patches/0072-cpufreq-Don-t-remove-sysfs-link-for-policy-cpu.patch"
patch -p1 -R < "/root/machinex/patches/0071-cpufreq-Remove-unnecessary-use-of-policy-shared_type.patch"
patch -p1 -R < "/root/machinex/patches/0070-cpufreq-Set-all-cpus-in-policy-cpus-for-single-clust.patch"
patch -p1 -R < "/root/machinex/patches/0069-cpufreq-governors-Reset-tunables-only-for-cpufreq_un.patch"
patch -p1 -R < "/root/machinex/patches/0068-cpufreq-governors-Remove-code-redundancy-between-gov.patch"
patch -p1 -R < "/root/machinex/patches/0067-cpufreq-governors-Get-rid-of-dbs_data-enable-field.patch"
patch -p1 -R < "/root/machinex/patches/0066-cpufreq-governors-fix-misuse-of-cdbs.cpu.patch"
patch -p1 -R < "/root/machinex/patches/0065-cpufreq-governors-implement-generic-policy_is_shared.patch"
patch -p1 -R < "/root/machinex/patches/0064-cpufreq-instantiate-cpufreq-cpu0-as-a-platform-drive.patch"
patch -p1 -R < "/root/machinex/patches/0063-cpufreq-governors-clean-timer-init-and-exit-code.patch"
patch -p1 -R < "/root/machinex/patches/0062-cpufreq-Simplify-cpufreq_add_dev.patch"
patch -p1 -R < "/root/machinex/patches/0061-cpufreq-Simplify-__cpufreq_remove_dev.patch"
patch -p1 -R < "/root/machinex/patches/0060-cpufreq-ondemand-use-all-CPUs-in-update_sampling_rat.patch"
patch -p1 -R < "/root/machinex/patches/0059-ditto-for-conservative.patch"
patch -p1 -R < "/root/machinex/patches/0058-cpufreq-ondemand-call-dbs_check_cpu-only-when-necess.patch"
patch -p1 -R < "/root/machinex/patches/0057-cpufreq-handle-SW-coordinated-CPUs.patch"
patch -p1 -R < "/root/machinex/patches/0056-cleanup-makefile-kconfig-gcommon-shit.patch"
patch -p1 -R < "/root/machinex/patches/0055-cpufreq-ondemand-update-sampling-rate-only-on-right-.patch"
patch -p1 -R < "/root/machinex/patches/0054-cpufreq-ondemand-fix-wrong-delay-sampling-rate.patch"
patch -p1 -R < "/root/machinex/patches/0053-prepping-for-updated-demand-based-govs.patch"
