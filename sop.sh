#!/bin/bash

#patch -p1 < "/root/machinex/patches/0001-fuse-fix-uninitialized-variable-warning.patch"
#patch -p1 < "/root/machinex/patches/0002-a-couple-of-exofs-static-funcs.patch"
#patch -p1 < "/root/machinex/patches/0003-backing_dev-fix-hung-task-on-sync.patch"
#patch -p1 < "/root/machinex/patches/0004-bdi-avoid-oops-on-device-removal.patch"
#patch -p1 < "/root/machinex/patches/0005-kmemleak-free-internal-objects-only-if-no-leaks-to-b.patch"
#patch -p1 < "/root/machinex/patches/0006-kmemleak-allow-freeing-internal-objects.patch"
#patch -p1 < "/root/machinex/patches/0007-kmemleak-remove-redundant-code.patch"
#patch -p1 < "/root/machinex/patches/0008-kmemleak-change-some-global-variables-to-int.patch"
#patch -p1 < "/root/machinex/patches/0009-bs.patch"
#patch -p1 < "/root/machinex/patches/0010-fs-direct-io.c-remove-redundant-comparison.patch"
#patch -p1 < "/root/machinex/patches/0011-fs-direct-io.c-remove-some-left-over-checks.patch"
#patch -p1 < "/root/machinex/patches/0012-mm-vmscan-respect-NUMA-policy-mask-when-shrinking.patch"
#patch -p1 < "/root/machinex/patches/0013-mm-vmscan-move-call-to-shrink_slab-to-shrink_zones.patch"
#patch -p1 < "/root/machinex/patches/0014-mm-vmscan-remove-shrink_control-arg-from-do_try.patch"
#patch -p1 < "/root/machinex/patches/0015-mm-compaction-ignore-pageblock-skip-when-manually.patch"
#patch -p1 < "/root/machinex/patches/0016-mm-hugetlb-unify-region-structure-handling.patch"
#patch -p1 < "/root/machinex/patches/0017-mm-hugetlb-improve-cleanup-resv_map-parameters.patch"
#patch -p1 < "/root/machinex/patches/0018-mm-hugetlb-fix-race-in-region-tracking.patch"
#patch -p1 < "/root/machinex/patches/0019-mm-hugetlb-remove-resv_map_put.patch"
#patch -p1 < "/root/machinex/patches/0020-mm-hugetlb-use-vma_resv_map-map-types.patch"
#patch -p1 < "/root/machinex/patches/0021-fix-a-little-derpage-in-mm-filemap.patch"
#patch -p1 < "/root/machinex/patches/0022-prometheus-laying-some-ground-work-for-a-corner-case.patch"
#patch -p1 < "/root/machinex/patches/0023-and-a-little-more.patch"
#patch -p1 < "/root/machinex/patches/0024-possing-the-mm-stuff-also-reverted-the-prometheus-st.patch"
#patch -p1 < "/root/machinex/patches/0025-possed.patch"
#patch -p1 < "/root/machinex/patches/0026-safety-check-in-mm-filemap.patch"
#patch -p1 < "/root/machinex/patches/0027-actually-perform-some-fixes-in-filemap.patch"
#patch -p1 < "/root/machinex/patches/0028-ref.patch"
#patch -p1 < "/root/machinex/patches/0029-fs-cachefiles-use-add_to_page_cache_lru.patch"
#patch -p1 < "/root/machinex/patches/0030-mm-vmscan-respect-NUMA-policy-mask-when-shrinking.patch"
#patch -p1 < "/root/machinex/patches/0031-mm-vmscan-move-call-to-shrink_slab-to-shrink_zones.patch"
#patch -p1 < "/root/machinex/patches/0032-mm-vmscan-remove-shrink_control-arg.patch"
#patch -p1 < "/root/machinex/patches/0033-m-compaction-ignore-pageblock-skip.patch"
#patch -p1 < "/root/machinex/patches/0034-mm-vmscan-shrink_slab-rename-max_pass.patch"
#patch -p1 < "/root/machinex/patches/0035-mm-vmstat-fix-UP-zone-state-accounting.patch"
#patch -p1 < "/root/machinex/patches/0036-mm-shmem-save-one-radix-tree-lookup-when-truncating.patch"
#patch -p1 < "/root/machinex/patches/0037-mm-fs-prepare-for-non-page-entries-in-page-cache.patch"
#patch -p1 < "/root/machinex/patches/0038-fuck.patch"
#patch -p1 < "/root/machinex/patches/0039-reverted.patch"
#patch -p1 < "/root/machinex/patches/0040-mm-fs-store-shadow-entries-in-page-cache.patch"
#patch -p1 < "/root/machinex/patches/0041-mm-thrash-detection-based-file-cache-sizing.patch"
#patch -p1 < "/root/machinex/patches/0042-mm-keep-page-cache-radix-tree-nodes-in-check.patch"
#patch -p1 < "/root/machinex/patches/0043-mm-hugetlb-mark-some-bootstrap-functions-as-__init.patch"
#patch -p1 < "/root/machinex/patches/0044-mm-compaction-avoid-isolating-pinned-pages.patch"
#patch -p1 < "/root/machinex/patches/0045-mark-some-funcs-as-static.patch"
#patch -p1 < "/root/machinex/patches/0046-ext4-missed-a-conversion-to-truncate_inode_pages_fin.patch"
#patch -p1 < "/root/machinex/patches/0047-include-linux-mm.h-remove-ifdef-condition.patch"
#patch -p1 < "/root/machinex/patches/0048-rename-do_fault-for-some-reason.patch"
#patch -p1 < "/root/machinex/patches/0049-kobject-don-t-block-for-each-kobject_uevent.patch"
#patch -p1 < "/root/machinex/patches/0050-slub-do-not-drop-slab_mutex-for-sysfs_slab_add.patch"
#patch -p1 < "/root/machinex/patches/0051-include-linux-syscalls.h-add-sys32_quotactl-prototyp.patch"
#patch -p1 < "/root/machinex/patches/0052-kernel-groups-change-it-to-a-void.patch"
#patch -p1 < "/root/machinex/patches/0053-dox.patch"
#patch -p1 < "/root/machinex/patches/0054-lib-syscall.c-unexport-task_current_syscall.patch"
#patch -p1 < "/root/machinex/patches/0055-backlight-update-bd-state-fb_blank-properties-when-n.patch"
#patch -p1 < "/root/machinex/patches/0056-acklight-update-backlight-status-when-necessary.patch"
#patch -p1 < "/root/machinex/patches/0057-lib-devres.c-fix-some-sparse-warnings.patch"
#patch -p1 < "/root/machinex/patches/0058-lib-decompress_inflate.c-include-appropriate-header.patch"
#patch -p1 < "/root/machinex/patches/0059-fs-efs-super.c-add-__init-to-init_inodecache.patch"
#patch -p1 < "/root/machinex/patches/0060-binfmt_misc-add-missing-break-statement.patch"
#patch -p1 < "/root/machinex/patches/0061-dox.patch"
#patch -p1 < "/root/machinex/patches/0062-rtc-verify-a-critical-argument-to-rtc_update_irq.patch"
#patch -p1 < "/root/machinex/patches/0063-rtc-pm8xxx-fixup-some-checkpatch-issues.patch"
#patch -p1 < "/root/machinex/patches/0064-bs.patch"
#patch -p1 < "/root/machinex/patches/0065-dox.patch"
#patch -p1 < "/root/machinex/patches/0066-ext4-initialize-multi-block-allocator-before-checkin.patch"
#patch -p1 < "/root/machinex/patches/0067-ext4-fix-jbd2-warning-under-heavy-xattr-load.patch"
#patch -p1 < "/root/machinex/patches/0068-mm-hugetlb.c-add-NULL-check-of-return-value-of-huge.patch"
#patch -p1 < "/root/machinex/patches/0069-leds-make-sure-we-unregister-a-trigger-only-once.patch"
#patch -p1 < "/root/machinex/patches/0070-mm-vmscan-restore-sc-gfp_mask-after-promoting-it.patch"
#patch -p1 < "/root/machinex/patches/0071-mm-vmscan-do-not-check-compaction_ready-on-promoted.patch"
#patch -p1 < "/root/machinex/patches/0072-dox.patch"
#patch -p1 < "/root/machinex/patches/0073-mm-compaction-disallow-high-order-page-for-migration.patch"
#patch -p1 < "/root/machinex/patches/0074-mm-compaction-do-not-call-suitable_migration_target.patch"
#patch -p1 < "/root/machinex/patches/0075-mm-compaction-change-the-timing-to-check-to-drop.patch"
#patch -p1 < "/root/machinex/patches/0076-mm-compaction-check-pageblock-suitability-once.patch"
#patch -p1 < "/root/machinex/patches/0077-mm-compaction-clean-up-code-on-success-of-ballon-iso.patch"
#patch -p1 < "/root/machinex/patches/0078-exec-kill-the-unnecessary-mm-def_flags-setting.patch"
#patch -p1 < "/root/machinex/patches/0079-mm-disable-split-page-table-lock-for-MMU.patch"
#patch -p1 < "/root/machinex/patches/0080-drivers-lguest-page_tables.c-rename-do_set_pte.patch"
#patch -p1 < "/root/machinex/patches/0081-mempool-add-unlikely-and-likely-hints.patch"
#patch -p1 < "/root/machinex/patches/0082-mm-mempolicy-rename-slab_node-for-clarity.patch"
#patch -p1 < "/root/machinex/patches/0083-mm-use-macros-from-compiler.h-instead-of-__attribute.patch"
#patch -p1 < "/root/machinex/patches/0084-mm-implement-map_pages-for-page-cache.patch"
#patch -p1 < "/root/machinex/patches/0085-mm-cleanup-size-checks-in-filemap_fault-and-filemap.patch"
#patch -p1 < "/root/machinex/patches/0086-i-think-we-get-rid-of-filemap-eventually-actually.patch"
#patch -p1 < "/root/machinex/patches/0087-fuck-one-sec.patch"
#patch -p1 < "/root/machinex/patches/0088-mm-compaction-determine-isolation-mode-only-once.patch"
#patch -p1 < "/root/machinex/patches/0089-coding-style.patch"
#patch -p1 < "/root/machinex/patches/0090-dox.patch"
#patch -p1 < "/root/machinex/patches/0091-mm-memcg-remove-unnecessary-preemption-disabling-tha.patch"
#patch -p1 < "/root/machinex/patches/0092-mm-readahead.c-inline-ra_submit.patch"
#patch -p1 < "/root/machinex/patches/0093-dox.patch"
#patch -p1 < "/root/machinex/patches/0094-proc-show-mnt_id-in-proc-pid-fdinfo.patch"
#patch -p1 < "/root/machinex/patches/0095-fs-proc-inode.c-use-RCU_INIT_POINTER-x-NULL.patch"
#patch -p1 < "/root/machinex/patches/0096-procfs-make-proc-stack-syscall-personality-0400.patch"
#patch -p1 < "/root/machinex/patches/0097-procfs-make-proc-pagemap-0400.patch"
#patch -p1 < "/root/machinex/patches/0098-exec-kill-bprm-tcomm-simplify-the-basename-logic.patch"
#patch -p1 < "/root/machinex/patches/0099-wait-swap-EXIT_ZOMBIE-and-EXIT_DEAD.patch"
#patch -p1 < "/root/machinex/patches/0100-include-linux-crash_dump.h-add-vmcore_cleanup.patch"
#patch -p1 < "/root/machinex/patches/0101-vmcore-continue-vmcore-initialization-if-PT_NOTE-is-.patch"
#patch -p1 < "/root/machinex/patches/0102-lib-idr.c-use-RCU_INIT_POINTER-x-NULL.patch"
#patch -p1 < "/root/machinex/patches/0103-bs.patch"
#patch -p1 < "/root/machinex/patches/0104-kconfig-make-allnoconfig-disable-options-behind-EMBE.patch"
#patch -p1 < "/root/machinex/patches/0105-include-asm-generic-bug.h-style-fix-s-while-0-while.patch"
#patch -p1 < "/root/machinex/patches/0106-bug-when-CONFIG_BUG-make-WARN-call-no_printk.patch"
#patch -p1 < "/root/machinex/patches/0107-fault-injection-set-bounds.patch"
#patch -p1 < "/root/machinex/patches/0108-initramfs-debug-detected-compression-method.patch"
#patch -p1 < "/root/machinex/patches/0109-ipc-compat.c-remove-sc_semopm-macro.patch"
#patch -p1 < "/root/machinex/patches/0110-ipc-use-device_initcall.patch"
#patch -p1 < "/root/machinex/patches/0111-dox.patch"
#patch -p1 < "/root/machinex/patches/0112-memcg-slab-never-try-to-merge-memcg-caches.patch"
#patch -p1 < "/root/machinex/patches/0113-slub-adjust-memcg-caches-when-creating-cache-alias.patch"
#patch -p1 < "/root/machinex/patches/0114-slub-rework-sysfs-layout-for-memcg-caches.patch"
#patch -p1 < "/root/machinex/patches/0115-slub-fix-leak-of-name-in-sysfs_slab_add.patch"
#patch -p1 < "/root/machinex/patches/0116-mm-use-raw_cpu-ops-for-determining-current-NUMA-node.patch"
#patch -p1 < "/root/machinex/patches/0117-modules-use-raw_cpu_write-for-initialization-of-per-.patch"
#patch -p1 < "/root/machinex/patches/0118-net-replace-__this_cpu_inc-in-route.c-with-raw_cpu.patch"
#patch -p1 < "/root/machinex/patches/0119-slub-use-raw_cpu_inc-for-incrementing-statistics.patch"
#patch -p1 < "/root/machinex/patches/0120-vmstat-use-raw_cpu_ops-to-avoid-false-positives.patch"
#patch -p1 < "/root/machinex/patches/0121-lglock-map-to-spinlock-when-CONFIG_SMP.patch"
#patch -p1 < "/root/machinex/patches/0122-mm-create-generic-early_ioremap-support.patch"
#patch -p1 < "/root/machinex/patches/0123-bs.patch"
#patch -p1 < "/root/machinex/patches/0124-cpuidle-sysfs-Export-target-residency-information.patch"
#patch -p1 < "/root/machinex/patches/0125-V.patch"
#patch -p1 < "/root/machinex/patches/0126-ARM-add-missing-system_misc.h-include-to-process.c.patch"
#patch -p1 < "/root/machinex/patches/0127-scripts-objdiff-detect-object-code-changes.patch"
#patch -p1 < "/root/machinex/patches/0128-bs.patch"
#patch -p1 < "/root/machinex/patches/0129-ext4-update-PF_MEMALLOC-handling-in-ext4_write_inode.patch"
#patch -p1 < "/root/machinex/patches/0130-oh-really-here-i-thought-i-was-being-prudent.patch"
#patch -p1 < "/root/machinex/patches/0131-fix-my-goto.patch"
#patch -p1 < "/root/machinex/patches/0132-we-dont-have-skipped_async_unsuitable.patch"
#patch -p1 < "/root/machinex/patches/0133-REVERT-slub-adjust-memcg-caches-when-creating-cache-.patch"
#patch -p1 < "/root/machinex/patches/0134-REVERT-initramfs-debug-detected-compression-method.patch"
#patch -p1 < "/root/machinex/patches/0135-include-linux-syscalls.h-add-sys_renameat2-prototype.patch"
#patch -p1 < "/root/machinex/patches/0136-drivers-block-loop.c-ratelimit-error-messages.patch"
#patch -p1 < "/root/machinex/patches/0137-mm-vmscan-do-not-swap-anon-pages-just-because-free-f.patch"
#patch -p1 < "/root/machinex/patches/0138-md-bitmap-dont-abuse-i_writecount-for-bitmap-files.patch"
#patch -p1 < "/root/machinex/patches/0139-md-avoid-oops-on-unload-if-some-process-is-in-poll.patch"
#patch -p1 < "/root/machinex/patches/0140-ext4-fix-COLLAPSE_RANGE-test-failure-in-data-journal.patch"
#patch -p1 < "/root/machinex/patches/0141-fuck.patch"
#patch -p1 < "/root/machinex/patches/0142-that-was-fine.patch"
#patch -p1 < "/root/machinex/patches/0143-ext4-return-ENOMEM-rather-than-EIO-when-find_-_page-.patch"
#patch -p1 < "/root/machinex/patches/0144-mm-slab-slub-use-page-list-consistently-instead-of-p.patch"
#patch -p1 < "/root/machinex/patches/0145-ext4-move-ext4_update_i_disksize-into-mpage_map.patch"
#patch -p1 < "/root/machinex/patches/0146-shmem-missing-bits-of-splice-fix-racy-pipe-buffers-u.patch"
#patch -p1 < "/root/machinex/patches/0147-fs-prevent-doing-FALLOC_FL_ZERO_RANGE-on-append-only.patch"
#patch -p1 < "/root/machinex/patches/0148-ext4-use-i_size_read-in-ext4_unaligned_aio.patch"
#patch -p1 < "/root/machinex/patches/0149-ext4-silence-sparse-check-warning.patch"
#patch -p1 < "/root/machinex/patches/0150-mm-Initialize-error-in-shmem_file_aio_read.patch"
#patch -p1 < "/root/machinex/patches/0151-fs-jfs-jfs_inode.c-atomically-set-inode-i_flags.patch"
#patch -p1 < "/root/machinex/patches/0152-ext4-fix-ext4_count_free_clusters.patch"
#patch -p1 < "/root/machinex/patches/0153-aio-block-io_destroy-until-all-context-requests-are-.patch"
#patch -p1 < "/root/machinex/patches/0154-kernfs-protect-lazy-kernfs_iattrs-allocation-with-mu.patch"
#patch -p1 < "/root/machinex/patches/0155-sysfs-driver-core-remove-sysfs-device-_schedule_call.patch"
#patch -p1 < "/root/machinex/patches/0156-vmscan-reclaim_clean_pages_from_list-must-use-mod_zo.patch"
#patch -p1 < "/root/machinex/patches/0157-dox.patch"
#patch -p1 < "/root/machinex/patches/0158-fix-races-between-__d_instantiate-and-checks-of-dent.patch"
#patch -p1 < "/root/machinex/patches/0159-coredump-fix-va_list-corruption.patch"
#patch -p1 < "/root/machinex/patches/0160-Input-Add-INPUT_PROP_TOPBUTTONPAD-device-property.patch"
#patch -p1 < "/root/machinex/patches/0161-bs.patch"
#patch -p1 < "/root/machinex/patches/0162-ext4-inline-generic_file_aio_write-into-ext4_file_wr.patch"
#patch -p1 < "/root/machinex/patches/0163-ext4-move-ext4_file_dio_write-into-ext4_file_write.patch"
#patch -p1 < "/root/machinex/patches/0164-report-cpu_down-while-excluding-secure-processor.patch"
#patch -p1 < "/root/machinex/patches/0165-ext4-factor-out-common-code-in-ext4_file_write.patch"
#patch -p1 < "/root/machinex/patches/0166-ext4-fix-locking-for-O_APPEND-writes.patch"
#patch -p1 < "/root/machinex/patches/0167-ext4-add-a-new-spinlock-i_raw_lock-to-protect-raw-in.patch"
#patch -p1 < "/root/machinex/patches/0168-ext4-remove-obsoleted-check.patch"
#patch -p1 < "/root/machinex/patches/0169-locks-rename-file-private-locks-to-open-file-descrip.patch"
#patch -p1 < "/root/machinex/patches/0170-fs-aio.c-Remove-ctx-parameter-in-kiocb_cancel.patch"
#patch -p1 < "/root/machinex/patches/0171-locks-rename-FL_FILE_PVT-and-IS_FILE_PVT-to-use-_OFD.patch"
#patch -p1 < "/root/machinex/patches/0172-kernfs-implement-kernfs_root-supers-list.patch"
#patch -p1 < "/root/machinex/patches/0173-kernfs-make-kernfs_notify-trigger-inotify-events-too.patch"
#patch -p1 < "/root/machinex/patches/0174-kernfs-fix-a-subdir-count-leak.patch"
#patch -p1 < "/root/machinex/patches/0175-kernfs-add-back-missing-error-check-in-kernfs_fop_mm.patch"
#patch -p1 < "/root/machinex/patches/0176-dox.patch"
#patch -p1 < "/root/machinex/patches/0177-cgroup-remove-orphaned-cgroup_pidlist_seq_operations.patch"
#patch -p1 < "/root/machinex/patches/0178-cgroup-replace-pr_warning-with-preferred-pr_warn.patch"
#patch -p1 < "/root/machinex/patches/0179-cgroup-Use-more-current-logging-style.patch"
#patch -p1 < "/root/machinex/patches/0180-fuse-add-__exit-to-fuse_ctl_cleanup.patch"
#patch -p1 < "/root/machinex/patches/0181-fuse-fix-mtime-update-error-in-fsync.patch"
#patch -p1 < "/root/machinex/patches/0182-fuse-do-not-use-uninitialized-i_mode.patch"
#patch -p1 < "/root/machinex/patches/0183-file_update_time-mtime-updates-for-fuse-squashed.patch"
#patch -p1 < "/root/machinex/patches/0184-fuse-clean-up-fsync.patch"
#patch -p1 < "/root/machinex/patches/0185-fuse-add-.write_inode.patch"
#patch -p1 < "/root/machinex/patches/0186-fuse-fuse-add-time_gran-to-INIT_OUT.patch"
#patch -p1 < "/root/machinex/patches/0187-fuse-allow-ctime-flushing-to-userspace.patch"
#patch -p1 < "/root/machinex/patches/0188-fuse-remove-.update_time.patch"
#patch -p1 < "/root/machinex/patches/0189-fuse-trust-kernel-i_ctime-only.patch"
#patch -p1 < "/root/machinex/patches/0190-fuse-clear-FUSE_I_CTIME_DIRTY-flag-on-setattr.patch"
#patch -p1 < "/root/machinex/patches/0191-fuse-clear-MS_I_VERSION.patch"
#patch -p1 < "/root/machinex/patches/0192-fuse-add-renameat2-support.patch"
#patch -p1 < "/root/machinex/patches/0193-ptp-validate-the-freq-adjustement.patch"
#patch -p1 < "/root/machinex/patches/0194-mm-don-t-pointlessly-use-BUG_ON-for-sanity-check.patch"
#patch -p1 < "/root/machinex/patches/0195-aio-report-error-from-io_destroy-when-threads-race.patch"
#patch -p1 < "/root/machinex/patches/0196-aio-cleanup-flatten-kill_ioctx.patch"
patch -p1 < "/root/machinex/patches/0197-msm-rotator-use-reinit_completion.patch"
patch -p1 < "/root/machinex/patches/0198-rotator-cancel-delayed-work-no-sync.patch"
patch -p1 < "/root/machinex/patches/0199-another.patch"
patch -p1 < "/root/machinex/patches/0200-speed-shit-up-in-rotator-use-system_wq-so-we-can-mod.patch"
patch -p1 < "/root/machinex/patches/0201-fix-pointer-val-for-reinit_completion.patch"
patch -p1 < "/root/machinex/patches/0202-dcache-fold-d_kill-and-d_free.patch"
patch -p1 < "/root/machinex/patches/0203-fold-try_prune_one_dentry.patch"
patch -p1 < "/root/machinex/patches/0204-new-helper-dentry_free.patch"
patch -p1 < "/root/machinex/patches/0205-expand-the-call-of-dentry_lru_del-in-dentry_kill.patch"
patch -p1 < "/root/machinex/patches/0206-block-null_blk-fix-use-after-free.patch"
patch -p1 < "/root/machinex/patches/0207-aio-fix-potential-leak-in-aio_run_iocb.patch"
patch -p1 < "/root/machinex/patches/0208-dentry_kill-don-t-try-to-remove-from-shrink-list.patch"
patch -p1 < "/root/machinex/patches/0209-don-t-remove-from-shrink-list-in-select_collect.patch"
patch -p1 < "/root/machinex/patches/0210-more-graceful-recovery-in-umount_collect.patch"
patch -p1 < "/root/machinex/patches/0211-dcache-don-t-need-rcu-in-shrink_dentry_list.patch"
patch -p1 < "/root/machinex/patches/0212-lib-Export-interval_tree.patch"
patch -p1 < "/root/machinex/patches/0213-dox.patch"
patch -p1 < "/root/machinex/patches/0214-dox.patch"
#patch -p1 < "/root/machinex/patches/0215-slab-fix-the-type-of-the-index-on-freelist-index.patch"
#patch -p1 < "/root/machinex/patches/0216-slab-Fix-off-by-one.patch"
#patch -p1 < "/root/machinex/patches/0217-slub-fix-memcg_propagate_slab_attrs.patch"
#patch -p1 < "/root/machinex/patches/0218-hugetlb-ensure-hugepage-access-is-denied.patch"
#patch -p1 < "/root/machinex/patches/0219-mm-page-writeback.c-fix-divide-by-zero-in-pos_ratio.patch"
#patch -p1 < "/root/machinex/patches/0220-mm-compaction-make-isolate_freepages-start-at-pagebl.patch"
#patch -p1 < "/root/machinex/patches/0221-mm-filemap-update-find_get_pages_tag-to-deal-with-sh.patch"
#patch -p1 < "/root/machinex/patches/0222-autofs-fix-lockref-lookup.patch"
#patch -p1 < "/root/machinex/patches/0224-slub-use-sysfs-release-mechanism-for-kmem_cache.patch"
#patch -p1 < "/root/machinex/patches/0225-fanotify-fix-EOVERFLOW-with-large-files-on-64-bit.patch"
#patch -p1 < "/root/machinex/patches/0226-bvs.patch"
#patch -p1 < "/root/machinex/patches/0227-fs-file.c-use-kvfree.patch"
#patch -p1 < "/root/machinex/patches/0228-kill-iov_iter_copy_from_user.patch"
#patch -p1 < "/root/machinex/patches/0229-generic_file_direct_write-switch-to-iov_iter.patch"
#patch -p1 < "/root/machinex/patches/0230-kill-generic_segment_checks.patch"
#patch -p1 < "/root/machinex/patches/0231-new-helper-copy_page_from_iter.patch"
#patch -p1 < "/root/machinex/patches/0232-optimize-copy_page_-to-from-_iter.patch"
#patch -p1 < "/root/machinex/patches/0233-bandaid-fix-TODO-find-new_sync_write.patch"
#patch -p1 < "/root/machinex/patches/0234-possing-only-back-by-6-commits-the-iov_iter-ones-loo.patch"
#patch -p1 < "/root/machinex/patches/0235-possed.patch"
#patch -p1 < "/root/machinex/patches/0237-need-to-pos-we-have-something-from-this-set-that-fuc.patch"
#patch -p1 < "/root/machinex/patches/0238-possed.patch"
#patch -p1 < "/root/machinex/patches/0239-gonna-sop-a-bit-at-a-time.patch"
#patch -p1 < "/root/machinex/patches/0240-removed-cgroup-shit-because-it-will-probably-fail.patch"
#patch -p1 < "/root/machinex/patches/0241-fix-my-cpu-printing-mechanism-so-that-the-printing-o.patch"
#patch -p1 < "/root/machinex/patches/0242-gonna-sop-commits-131-141.patch"
#patch -p1 < "/root/machinex/patches/0243-sopped.patch"
#patch -p1 < "/root/machinex/patches/0244-REVERT-mm-vmscan-do-not-swap-anon-pages-commit-major.patch"
#patch -p1 < "/root/machinex/patches/0245-and-remove-it-from-sop.patch"
#patch -p1 < "/root/machinex/patches/0246-it-was-something-there.patch"
#patch -p1 < "/root/machinex/patches/0247-fix-up-sop-a-little.patch"
#patch -p1 < "/root/machinex/patches/0248-okay-just-the-first-four-ext4-commits.patch"
#patch -p1 < "/root/machinex/patches/0249-locks-rename-file-private-locks-to-open-file-descrip.patch"
#patch -p1 < "/root/machinex/patches/0250-fs-aio.c-Remove-ctx-parameter-in-kiocb_cancel.patch"
