#!/bin/bash

patch -p1 < "/root/machinex/patches/0080-lib-crc32-clean-up-spacing.patch"
patch -p1 < "/root/machinex/patches/0081-lib-crc32-add-functionality-to-combine-two-crc32.patch"
patch -p1 < "/root/machinex/patches/0082-lib-crc32-add-test-cases-for-crc32-c-_combine-routin.patch"
patch -p1 < "/root/machinex/patches/0083-quota-info-leak-in-quota_getquota.patch"
patch -p1 < "/root/machinex/patches/0084-lib-crc32-conditionally-resched.patch"
patch -p1 < "/root/machinex/patches/0085-lib-crc32-reduce-number-of-cases-for-crc32-c-_combin.patch"
patch -p1 < "/root/machinex/patches/0086-fuse-writepages-roll-back-changes-if-request-not-fou.patch"
patch -p1 < "/root/machinex/patches/0087-fuse-writepages-crop-secondary-requests.patch"
patch -p1 < "/root/machinex/patches/0088-fuse-writepages-update-bdi-writeout-when-deleting.patch"
patch -p1 < "/root/machinex/patches/0089-fuse-writepages-protect-secondary-requests-from-fuse.patch"
patch -p1 < "/root/machinex/patches/0090-audit-add-child-record-before-the-create.patch"
patch -p1 < "/root/machinex/patches/0091-Revert-sysfs-drop-kobj_ns_type-handling.patch"
patch -p1 < "/root/machinex/patches/0092-ext4-use-prandom_u32-instead-of-get_random_bytes.patch"
patch -p1 < "/root/machinex/patches/0093-bdi-test-bdi_init-failure.patch"
patch -p1 < "/root/machinex/patches/0094-revert-me.patch"
patch -p1 < "/root/machinex/patches/0095-done.patch"
patch -p1 < "/root/machinex/patches/0096-RCU-vfsmounts.patch"
patch -p1 < "/root/machinex/patches/0097-what-the-fuck-patch.patch"
patch -p1 < "/root/machinex/patches/0098-reverted.patch"
patch -p1 < "/root/machinex/patches/0099-get-rid-of-useless-wrapper-lock-unlock-rcu-walk-just.patch"
patch -p1 < "/root/machinex/patches/0100-remaining-debris.patch"
patch -p1 < "/root/machinex/patches/0101-dcache-fold-_d_shrink-into-its-only-caller-again.patch"
patch -p1 < "/root/machinex/patches/0102-new-helper-dump_emit.patch"
patch -p1 < "/root/machinex/patches/0103-convert-stuff-to-dump-emit-add-dump-align-squashed-o.patch"
patch -p1 < "/root/machinex/patches/0104-take-anon-inode-allocation-to-libfs.c.patch"
patch -p1 < "/root/machinex/patches/0105-rework-aio-migrate-pages-to-use-aio-fs.patch"
patch -p1 < "/root/machinex/patches/0106-kill-anon_inode_getfile_private.patch"
patch -p1 < "/root/machinex/patches/0107-constify-do_coredump-argument.patch"
patch -p1 < "/root/machinex/patches/0108-elf-_fdpic-coredump-get-rid-of-pointless-if-siginfo.patch"
patch -p1 < "/root/machinex/patches/0109-VFS-Put-a-small-type-field-into-struct-dentry-d_flag.patch"
patch -p1 < "/root/machinex/patches/0110-iget-iget5-don-t-bother-with-i_lock.patch"
patch -p1 < "/root/machinex/patches/0111-vfs-split-out-vfs_getattr_nosec.patch"
patch -p1 < "/root/machinex/patches/0112-export-stuff.patch"
patch -p1 < "/root/machinex/patches/0113-dcache-use-IS_ROOT-to-decide-where-dentry-is-hashed.patch"
patch -p1 < "/root/machinex/patches/0114-dcache-dont-clear-DCACHE_DISCONNECTED-too-early.patch"
patch -p1 < "/root/machinex/patches/0115-dox.patch"
patch -p1 < "/root/machinex/patches/0116-exportfs-squash.patch"
patch -p1 < "/root/machinex/patches/0117-vfs-pull-ext4-s-double-i_mutex-locking-into-common-c.patch"
patch -p1 < "/root/machinex/patches/0118-vfs-don-t-use-PARENT-CHILD-lock-classes-for-non-dire.patch"
patch -p1 < "/root/machinex/patches/0119-vfs-rename-I_MUTEX_QUOTA.patch"
patch -p1 < "/root/machinex/patches/0120-vfs-take-i_mutex-on-renamed-file.patch"
patch -p1 < "/root/machinex/patches/0121-new-flag.patch"
patch -p1 < "/root/machinex/patches/0122-locks-implement-delegations.patch"
patch -p1 < "/root/machinex/patches/0123-namei-minor-vfs_unlink-cleanup.patch"
patch -p1 < "/root/machinex/patches/0124-locks-break-delegations-on-unlink.patch"
patch -p1 < "/root/machinex/patches/0125-locks-helper-functions-for-delegation-breaking.patch"
patch -p1 < "/root/machinex/patches/0126-locks-break-delegations-on-rename.patch"
patch -p1 < "/root/machinex/patches/0127-and-on-link.patch"
patch -p1 < "/root/machinex/patches/0128-locks-break-delegations-on-any-attribute-modificatio.patch"
patch -p1 < "/root/machinex/patches/0129-typo.patch"
patch -p1 < "/root/machinex/patches/0130-ext4-return-non-zero-st_blocks-for-inline-data.patch"
patch -p1 < "/root/machinex/patches/0131-ext4-add-prototypes-for-macro-generated-functions.patch"
patch -p1 < "/root/machinex/patches/0132-smp-cpumask-Make-CONFIG_CPUMASK_OFFSTACK-y-usable.patch"
patch -p1 < "/root/machinex/patches/0133-dox.patch"
patch -p1 < "/root/machinex/patches/0134-more-dox.patch"
patch -p1 < "/root/machinex/patches/0135-mm-use-pgdat_end_pfn-to-simplify-the-code-in-others.patch"
patch -p1 < "/root/machinex/patches/0136-revert.patch"
patch -p1 < "/root/machinex/patches/0137-reverted.patch"
patch -p1 < "/root/machinex/patches/0138-update-config.patch"
patch -p1 < "/root/machinex/patches/0139-fix-sdcardfs.h-notify_change.patch"
patch -p1 < "/root/machinex/patches/0140-whoaaaaa-get-rid-of-some-bs-logspam.patch"
patch -p1 < "/root/machinex/patches/0141-namei-bandaid.patch"
patch -p1 < "/root/machinex/patches/0142-ecryptfs-missing-vfs_unlink-conversion.patch"
patch -p1 < "/root/machinex/patches/0143-scfs-extend-notify_change.patch"
patch -p1 < "/root/machinex/patches/0144-more-arguement-fixes-for-scfs-and-sdcardfs.patch"
patch -p1 < "/root/machinex/patches/0145-and-vfs-rename.patch"
patch -p1 < "/root/machinex/patches/0146-fix-hopefully-scfs-vfs-link.patch"
patch -p1 < "/root/machinex/patches/0147-mm-use-populated_zone-instead-of-if-zone-present_pag.patch"
patch -p1 < "/root/machinex/patches/0148-mm-memory_hotplug.c-rename-the-function-is_memblock.patch"
patch -p1 < "/root/machinex/patches/0149-mm-memory_hotplug.c-use-pfn_to_nid-instead-of-page_t.patch"
patch -p1 < "/root/machinex/patches/0150-mm-add-a-helper-function-to-check-may-oom-condition.patch"
patch -p1 < "/root/machinex/patches/0151-mm-nobootmem.c-have-__free_pages_memory-free.patch"
patch -p1 < "/root/machinex/patches/0152-mm-memory-failure.c-move-set_migratetype_isolate.patch"
patch -p1 < "/root/machinex/patches/0153-arch-use-NUMA_NO_NODE.patch"
patch -p1 < "/root/machinex/patches/0154-mm-mempolicy-make-mpol_to_str-robust-and-always-succ.patch"
patch -p1 < "/root/machinex/patches/0155-mm-mempolicy-use-NUMA_NO_NODE.patch"
patch -p1 < "/root/machinex/patches/0156-memcg-support-hierarchical-memory.numa_stats.patch"
patch -p1 < "/root/machinex/patches/0157-mm-sparsemem-use-PAGES_PER_SECTION-to-remove-redunda.patch"
patch -p1 < "/root/machinex/patches/0158-mm-sparsemem-fix-a-bug-in-free_map_bootmem.patch"
patch -p1 < "/root/machinex/patches/0159-dox.patch"
patch -p1 < "/root/machinex/patches/0160-frontswap-enable-call-to-invalidate-area-on-swapoff.patch"
patch -p1 < "/root/machinex/patches/0161-mm-page_alloc.c-remove-unused-macro-LONG_ALIGN.patch"
patch -p1 < "/root/machinex/patches/0162-proc-pid-smaps-show-VM_SOFTDIRTY-flag-in-VmFlags-lin.patch"
patch -p1 < "/root/machinex/patches/0163-writeback-do-not-sync-data-dirtied-after-sync-start.patch"
patch -p1 < "/root/machinex/patches/0164-mm-zswap-avoid-unnecessary-page-scanning.patch"
patch -p1 < "/root/machinex/patches/0165-mmap-arch_get_unmapped_area-use-proper-mmap-base.patch"
patch -p1 < "/root/machinex/patches/0166-mm-memblock.c-introduce-bottom-up-allocation.patch"
patch -p1 < "/root/machinex/patches/0167-movable-node-boot-option.patch"
patch -p1 < "/root/machinex/patches/0168-mm-set-N_CPU-to-node_states-during-boot.patch"
patch -p1 < "/root/machinex/patches/0169-mm-clear-N_CPU-from-node_states-at-CPU-offline.patch"
patch -p1 < "/root/machinex/patches/0170-mm-do-not-walk-all-of-system-memory-during-show_mem.patch"
patch -p1 < "/root/machinex/patches/0171-readahead-fix-sequential-read-cache-miss-detection.patch"
patch -p1 < "/root/machinex/patches/0172-mm-fix-page_group_by_mobility_disabled-breakage.patch"
patch -p1 < "/root/machinex/patches/0173-tracing.patch"
patch -p1 < "/root/machinex/patches/0174-mm-__rmqueue_fallback-should-respect-pageblock-type.patch"
patch -p1 < "/root/machinex/patches/0175-mm-ensure-get_unmapped_area-returns-higher-address.patch"
patch -p1 < "/root/machinex/patches/0176-memcg-kmem-use-is_root_cache-instead-of-hard-code.patch"
patch -p1 < "/root/machinex/patches/0177-memcg-kmem-rename-cache_from_memcg-to-cache_from_mem.patch"
patch -p1 < "/root/machinex/patches/0178-memcg-kmem-use-cache_from_memcg_idx.patch"
patch -p1 < "/root/machinex/patches/0179-mm-zswap-bugfix-memory-leak.patch"
patch -p1 < "/root/machinex/patches/0180-dox.patch"
patch -p1 < "/root/machinex/patches/0181-mm-factor-commit-limit-calculation.patch"
patch -p1 < "/root/machinex/patches/0182-mm-numa-return-the-number-of-base-pages-altered.patch"
patch -p1 < "/root/machinex/patches/0183-drivers-char-hpet.c-allow-user-controlled-mmap.patch"
patch -p1 < "/root/machinex/patches/0184-percpu-add-test-module-for-various-percpu-operations.patch"
patch -p1 < "/root/machinex/patches/0185-mark-cramfs-as-obsolete.patch"
patch -p1 < "/root/machinex/patches/0186-syscalls.h-use-gcc-alias-instead-of-assembler-aliase.patch"
patch -p1 < "/root/machinex/patches/0187-kernel-sys.c-remove-obsolete-include-linux-kexec.h.patch"
patch -p1 < "/root/machinex/patches/0188-init-main.c-remove-prototype-for-sofirq_init.patch"
patch -p1 < "/root/machinex/patches/0189-backlight-use-dev_get_platdata.patch"
patch -p1 < "/root/machinex/patches/0190-our-panel-OCTA-use-dev_get_platdata.patch"
patch -p1 < "/root/machinex/patches/0191-backlight-generic_bl-use-devm_backlight_device_reg.patch"
patch -p1 < "/root/machinex/patches/0192-backlight-platform_lcd-use-devm_lcd_device_register.patch"
patch -p1 < "/root/machinex/patches/0193-dox.patch"
patch -p1 < "/root/machinex/patches/0194-init-do_mounts_rd.c-fix-NULL-pointer-dereference-whi.patch"
patch -p1 < "/root/machinex/patches/0195-init-make-init-failures-more-explicit.patch"
patch -p1 < "/root/machinex/patches/0196-drivers-message-i2o-driver.c-add-missing-destroy_wor.patch"
patch -p1 < "/root/machinex/patches/0197-drivers-rtc-rtc-cmos.c-remove-redundant-dev_set_drvd.patch"
patch -p1 < "/root/machinex/patches/0198-dito-mrst.patch"
patch -p1 < "/root/machinex/patches/0199-drivers-rtc-rtc-cmos.c-use-dev_get_platdata.patch"
patch -p1 < "/root/machinex/patches/0200-bs.patch"
patch -p1 < "/root/machinex/patches/0201-dox.patch"
patch -p1 < "/root/machinex/patches/0202-kernel-sysctl_binary.c-use-scnprintf.patch"
patch -p1 < "/root/machinex/patches/0203-kernel-taskstats.c-add-nla_nest_cancel-for-failure.patch"
patch -p1 < "/root/machinex/patches/0204-gcov-move-gcov-structs-definitions-to-a-gcc-version.patch"
patch -p1 < "/root/machinex/patches/0205-gcov-add-support-for-gcc-4.7-gcov-format.patch"
patch -p1 < "/root/machinex/patches/0206-gcov-compile-specific-gcov-implementation-based.patch"
patch -p1 < "/root/machinex/patches/0207-dox.patch"
patch -p1 < "/root/machinex/patches/0208-gcov-reuse-kbasename-helper.patch"
patch -p1 < "/root/machinex/patches/0209-kconfig-allow-disabling-compression.patch"
patch -p1 < "/root/machinex/patches/0210-Makefile-export-initial-ramdisk-compression-config.patch"
patch -p1 < "/root/machinex/patches/0211-aio-checking-for-NULL-instead-of-IS_ERR.patch"
patch -p1 < "/root/machinex/patches/0212-locks-missing-unlock-on-error-in-generic_add_lease.patch"
patch -p1 < "/root/machinex/patches/0213-fix-unpaired-rcu-lock-in-prepend_path.patch"
patch -p1 < "/root/machinex/patches/0214-bs.patch"
patch -p1 < "/root/machinex/patches/0215-regulator-fixed-fix-regulator_list_voltage-for-regre.patch"
patch -p1 < "/root/machinex/patches/0216-dont-need-it.patch"
patch -p1 < "/root/machinex/patches/0217-ARM-7882-1-mm-fix-__phys_to_virt.patch"
patch -p1 < "/root/machinex/patches/0218-execve-use-struct-filename-for-executable-name-passi.patch"
patch -p1 < "/root/machinex/patches/0219-run-acpi-init-before-timekeeping.patch"
patch -p1 < "/root/machinex/patches/0220-other-half-of-a-format-string-bug.patch"
patch -p1 < "/root/machinex/patches/0221-little-hack-for-us.patch"
patch -p1 < "/root/machinex/patches/0222-update-config.patch"
patch -p1 < "/root/machinex/patches/0223-need-to-define-a-ret.patch"
patch -p1 < "/root/machinex/patches/0224-fix-my-sync.patch"
patch -p1 < "/root/machinex/patches/0225-add-useless-but-necessary-unsigned-long-nr_huge_upda.patch"
patch -p1 < "/root/machinex/patches/0226-and-another.patch"
patch -p1 < "/root/machinex/patches/0227-add-missing-part-of-bottom-up.patch"
patch -p1 < "/root/machinex/patches/0228-for-real-this-time.patch"
patch -p1 < "/root/machinex/patches/0229-im-forced-to-pos.patch"
patch -p1 < "/root/machinex/patches/0230-possed.patch"
patch -p1 < "/root/machinex/patches/0231-revert-possed.patch"
