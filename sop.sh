#!/bin/bash

patch -p1 < "/root/machinex/patches/0087-cgroup-prepare-migration-path-for-unified-hierarchy.patch"
patch -p1 < "/root/machinex/patches/0088-cgroup-implement-dynamic-subtree-controller.patch"
patch -p1 < "/root/machinex/patches/0089-cgroup-implement-cgroup.populated-for-the-default-hi.patch"
patch -p1 < "/root/machinex/patches/0090-dox.patch"
patch -p1 < "/root/machinex/patches/0091-cgroup-remove-orphaned-cgroup_pidlist_seq_operations.patch"
patch -p1 < "/root/machinex/patches/0092-dox.patch"
patch -p1 < "/root/machinex/patches/0093-better-logging-style.patch"
patch -p1 < "/root/machinex/patches/0094-cgroup-make-flags-and-subsys_masks-unsigned-int.patch"
patch -p1 < "/root/machinex/patches/0095-cgroup-protect-cgroup_root-cgroup_idr-with-a-spinloc.patch"
patch -p1 < "/root/machinex/patches/0096-dcache-add-missing-lockdep-annotation.patch"
patch -p1 < "/root/machinex/patches/0097-locks-ensure-that-fl_owner-is-always-initialized.patch"
patch -p1 < "/root/machinex/patches/0098-fs-locks.c-replace-seq_printf-by-seq_puts.patch"
patch -p1 < "/root/machinex/patches/0099-cgroup-use-RCU-free-in-create_css-failure-path.patch"
patch -p1 < "/root/machinex/patches/0100-cgroup-update-init_css-into-init_and_link_css.patch"
patch -p1 < "/root/machinex/patches/0101-cgroup-memcg-implement-css-id-and-convert-css_from_i.patch"
patch -p1 < "/root/machinex/patches/0102-kernel-cgroup.c-fix-2-kernel-doc-warnings.patch"
patch -p1 < "/root/machinex/patches/0103-cgroup-introduce-task_css_is_root.patch"
patch -p1 < "/root/machinex/patches/0104-cgroup_freezer-replace-freezer-lock-with-freezer_mut.patch"
patch -p1 < "/root/machinex/patches/0105-cgroup-fix-rcu_read_lock-leak-in-update_if_frozen.patch"
patch -p1 < "/root/machinex/patches/0106-cgroup-fix-offlining-child-waiting-in-cgroup_subtree.patch"
patch -p1 < "/root/machinex/patches/0107-cgroup-cgroup_idr_lock-should-be-bh.patch"
patch -p1 < "/root/machinex/patches/0108-cgroup-css_release-shouldn-t-clear-cgroup-subsys.patch"
patch -p1 < "/root/machinex/patches/0109-cgroup-use-restart_syscall-for-retries-after-offline.patch"
patch -p1 < "/root/machinex/patches/0110-cgroup-use-release_agent_path_lock-in-cgroup_release.patch"
patch -p1 < "/root/machinex/patches/0111-some-misc-cleanup-in-cgroup.patch"
patch -p1 < "/root/machinex/patches/0112-another-fix-to-our-idr_alloc-frankencode.patch"
patch -p1 < "/root/machinex/patches/0113-mua-ha-ha-ha-ha.patch"
patch -p1 < "/root/machinex/patches/0114-muahahaha-continued.patch"
patch -p1 < "/root/machinex/patches/0115-fixup-my-makeshift-idr-alloc.patch"
patch -p1 < "/root/machinex/patches/0116-oops-actually-fix-it.patch"
patch -p1 < "/root/machinex/patches/0117-aaand-a-bracket-drp.patch"
patch -p1 < "/root/machinex/patches/0118-fixup-my-assignments.patch"
patch -p1 < "/root/machinex/patches/0119-just-reverting-idr-stuff.patch"
patch -p1 < "/root/machinex/patches/0120-trying-to-hack-this-together-smarter.patch"
patch -p1 < "/root/machinex/patches/0121-we-HAVE-to-ensure-css-id-isnt-below-zero-somehow.patch"
patch -p1 < "/root/machinex/patches/0122-fixin.patch"
patch -p1 < "/root/machinex/patches/0123-cgroup-rename-css_tryget-to-css_tryget_online.patch"
patch -p1 < "/root/machinex/patches/0124-cgroup-implement-cftype-write.patch"
patch -p1 < "/root/machinex/patches/0125-cgroup-replace-cftype-write_string-with-cftype-write.patch"
patch -p1 < "/root/machinex/patches/0126-cgroup-replace-cftype-trigger-with-cftype-write.patch"
patch -p1 < "/root/machinex/patches/0127-cgroup-convert-tasks-and-cgroup.procs-handle.patch"
patch -p1 < "/root/machinex/patches/0128-cgroup-remove-cgroup-control.patch"
patch -p1 < "/root/machinex/patches/0129-cgroup-reorganize-cgroup_create.patch"
patch -p1 < "/root/machinex/patches/0130-cgroup-collapse-cgroup_create-into-croup_mkdir.patch"
patch -p1 < "/root/machinex/patches/0131-cgroup-grab-cgroup_mutex-earlier-in-cgroup_subtree.patch"
patch -p1 < "/root/machinex/patches/0132-cgroup-move-cgroup-kn-priv-clearing-to-cgroup_rmdir.patch"
patch -p1 < "/root/machinex/patches/0133-cgroup-factor-out-cgroup_kn_lock_live.patch"
patch -p1 < "/root/machinex/patches/0134-cgroup-use-cgroup_kn_lock_live-in-other-cgroup-kernf.patch"
patch -p1 < "/root/machinex/patches/0135-cgroup-nest-kernfs-active-protection-under-cgroup_mu.patch"
patch -p1 < "/root/machinex/patches/0136-cgroup-remove-cgroup_tree_mutex.patch"
patch -p1 < "/root/machinex/patches/0137-cgroup-use-restart_syscall-for-mount-retries.patch"
patch -p1 < "/root/machinex/patches/0138-cgroup-rename-cgroup-dummy_css-to-self-and-move-it.patch"
patch -p1 < "/root/machinex/patches/0139-cgroup-separate-out-cgroup_has_live_children.patch"
patch -p1 < "/root/machinex/patches/0140-cgroup-move-check_for_release-parent-call-to-the-end.patch"
patch -p1 < "/root/machinex/patches/0141-cgroup-move-cgroup-sibling-unlinking-to-cgroup_put.patch"
patch -p1 < "/root/machinex/patches/0142-cgroup-remove-cgroup_destory_css_killed.patch"
patch -p1 < "/root/machinex/patches/0143-cgroup-bounce-css-release-through-css-destroy_work.patch"
patch -p1 < "/root/machinex/patches/0144-cgroup-use-cgroup-self.refcnt-for-cgroup-refcnting.patch"
patch -p1 < "/root/machinex/patches/0145-cgroup-skip-refcnting-on-normal-root-csse.patch"
patch -p1 < "/root/machinex/patches/0146-cgroup-remove-css_parent.patch"
patch -p1 < "/root/machinex/patches/0147-device_cgroup-remove-direct-access-to-cgroup-childre.patch"
patch -p1 < "/root/machinex/patches/0148-cgroup-remove-cgroup-parent.-and-some-fixes.patch"
patch -p1 < "/root/machinex/patches/0149-percpu_ref_init-fixes.patch"
patch -p1 < "/root/machinex/patches/0150-fixup-our-percpu_ref_exit-refs.patch"
patch -p1 < "/root/machinex/patches/0151-change-rcu_lockdep_assert-back-to-our-upstreamed-RCU.patch"
patch -p1 < "/root/machinex/patches/0152-fixed-up-some-of-my-own-derpiness.patch"
patch -p1 < "/root/machinex/patches/0153-i-think-this-is-safer-handling-of-our-makeshift-idr-.patch"
patch -p1 < "/root/machinex/patches/0154-make-idr_alloc-a-little-wrapper-for-mx_idr_alloc.patch"
patch -p1 < "/root/machinex/patches/0155-use-mx_idr_alloc-for-cgroup.patch"
patch -p1 < "/root/machinex/patches/0156-use-mx_idr_destroy.patch"
patch -p1 < "/root/machinex/patches/0157-screw-it-lets-use-backport-version-of-idr_alloc-for-.patch"
patch -p1 < "/root/machinex/patches/0158-just-use-the-logic-from-idr_alloc-directly-so-we-can.patch"
patch -p1 < "/root/machinex/patches/0159-use-kernfs-for-mm-slub-kmem_cache.patch"
patch -p1 < "/root/machinex/patches/0160-REVERT-the-check-for-NULL-page.patch"
patch -p1 < "/root/machinex/patches/0161-oops-remove-useless-code.patch"
patch -p1 < "/root/machinex/patches/0162-cgroup-move-cgroup-sibling-and-children-into-cgroup_.patch"
patch -p1 < "/root/machinex/patches/0163-cgroup-link-all-cgroup_subsys_states-in-their-siblin.patch"
patch -p1 < "/root/machinex/patches/0164-cgroup-move-cgroup-serial_nr-into-cgroup_subsys_stat.patch"
patch -p1 < "/root/machinex/patches/0165-cgroup-introduce-CSS_RELEASED-and-reduce-css-iterati.patch"
patch -p1 < "/root/machinex/patches/0166-cgroup-iterate-cgroup_subsys_states-directly.patch"
patch -p1 < "/root/machinex/patches/0167-cgroup-use-CSS_ONLINE-instead-of-CGRP_DEAD.patch"
patch -p1 < "/root/machinex/patches/0168-cgroup-convert-cgroup_has_live_children.patch"
patch -p1 < "/root/machinex/patches/0169-device_cgroup-use-css_has_online_children-instead-of.patch"
patch -p1 < "/root/machinex/patches/0170-cgroup-implement-css_tryget.patch"
patch -p1 < "/root/machinex/patches/0171-cgroup-disallow-debug-controller-on-the-default-hier.patch"
patch -p1 < "/root/machinex/patches/0172-cgroup-don-t-destroy-the-default-root.patch"
patch -p1 < "/root/machinex/patches/0173-cgroup-disallow-disabled-controllers-on-the-default.patch"
patch -p1 < "/root/machinex/patches/0174-cgroup-fix-broken-css_has_online_children.patch"
patch -p1 < "/root/machinex/patches/0175-cgroup-fix-mount-failure-in-a-corner-case.patch"
patch -p1 < "/root/machinex/patches/0176-cgroup-fix-a-race-between-cgroup_mount-and-cgroup_ki.patch"
patch -p1 < "/root/machinex/patches/0177-cgroup-reorganize-cgroup_subtree_control_write.patch"
patch -p1 < "/root/machinex/patches/0178-cgroup-introduce-cgroup-subtree_control.patch"
patch -p1 < "/root/machinex/patches/0179-cgroup-make-interface-files-visible-if-enabled.patch"
patch -p1 < "/root/machinex/patches/0180-cgroup-implement-cgroup_subsys-css_reset.patch"
patch -p1 < "/root/machinex/patches/0181-cgroup-implement-cgroup_subsys-depends_on.patch"
patch -p1 < "/root/machinex/patches/0182-cgroup-remove-CGRP_ROOT_OPTION_MASK.patch"
patch -p1 < "/root/machinex/patches/0183-cgroup-make-interface-file-cgroup.sane_behavior-lega.patch"
patch -p1 < "/root/machinex/patches/0184-cgroup-remove-sane_behavior-support-on-non-default-h.patch"
patch -p1 < "/root/machinex/patches/0185-cgroup-clean-up-sane_behavior-handling.patch"
patch -p1 < "/root/machinex/patches/0186-cgroup-split-cgroup_base_files.patch"
patch -p1 < "/root/machinex/patches/0187-cgroup-rename-cgroup_subsys-base_cftypes-to-legacy_c.patch"
patch -p1 < "/root/machinex/patches/0188-cgroup-replace-cgroup_add_cftypes-with-cgroup_add_le.patch"
patch -p1 < "/root/machinex/patches/0189-cgroup-distinguish-the-default-and-legacy-hierarchie.patch"
patch -p1 < "/root/machinex/patches/0190-cgroup-make-CFTYPE_ONLY_ON_DFL-and-CFTYPE_NO_-intern.patch"
patch -p1 < "/root/machinex/patches/0191-cgroup-initialize-cgrp_dfl_root_inhibit_ss_mask-from.patch"
patch -p1 < "/root/machinex/patches/0192-cgroup-reject-cgroup-names-with-n.patch"
patch -p1 < "/root/machinex/patches/0193-cgroup-Display-legacy-cgroup-files-on-default-hierar.patch"
patch -p1 < "/root/machinex/patches/0194-dox.patch"
patch -p1 < "/root/machinex/patches/0195-cgroup-delay-the-clearing-of-cgrp-kn-priv.patch"
patch -p1 < "/root/machinex/patches/0196-cgroup-check-cgroup-liveliness-before-unbreaking-ker.patch"
patch -p1 < "/root/machinex/patches/0197-cgroup-remove-some-useless-forward-declarations.patch"
patch -p1 < "/root/machinex/patches/0198-cgroup-remove-redundant-code-in-cgroup_rmdir.patch"
patch -p1 < "/root/machinex/patches/0199-cgroup-remove-bogus-comments.patch"
patch -p1 < "/root/machinex/patches/0200-cgroup-fix-unbalanced-locking.patch"
patch -p1 < "/root/machinex/patches/0201-cgroup-use-a-per-cgroup-work-for-release-agent.patch"
patch -p1 < "/root/machinex/patches/0202-cgroup-simplify-proc_cgroup_show.patch"
patch -p1 < "/root/machinex/patches/0203-cgroup-remove-CGRP_RELEASABLE-flag.patch"
patch -p1 < "/root/machinex/patches/0204-cgroup-fix-missing-unlock-in-cgroup_release_agent.patch"
patch -p1 < "/root/machinex/patches/0205-remove-redundant-var.patch"
patch -p1 < "/root/machinex/patches/0206-Revert-cgroup-remove-redundant-variable.patch"
patch -p1 < "/root/machinex/patches/0207-some-fixes-for-our-updated-rcu-setup.patch"
patch -p1 < "/root/machinex/patches/0208-locks-add-some-tracepoints.patch"
patch -p1 < "/root/machinex/patches/0209-fs-jfs-jfs_logmgr.c-remove-NULL-assignment-on-static.patch"
patch -p1 < "/root/machinex/patches/0210-fs-jfs-super.c-remove-0-assignment-to-static-code.patch"
patch -p1 < "/root/machinex/patches/0211-fs-jfs-jfs_dmap.c-replace-min-casting-by-min_t.patch"
patch -p1 < "/root/machinex/patches/0212-fs-jfs-super.c-convert-simple_str.patch"
patch -p1 < "/root/machinex/patches/0213-hugetlb-restrict-hugepage_migration_support-to-x86_6.patch"
patch -p1 < "/root/machinex/patches/0214-dox.patch"
patch -p1 < "/root/machinex/patches/0215-fs-fscache-replace-seq_printf-by-seq_puts.patch"
patch -p1 < "/root/machinex/patches/0216-fanotify-FAN_MARK_FLUSH.patch"
patch -p1 < "/root/machinex/patches/0217-trivial-cleanup.patch"
patch -p1 < "/root/machinex/patches/0218-fs-notify-fanotify-fanotify_user.c-fix-FAN_MARK_FLUS.patch"
patch -p1 < "/root/machinex/patches/0219-fanotify-check-file-flags-passed-in-fanotify_init.patch"
patch -p1 < "/root/machinex/patches/0220-dox.patch"
patch -p1 < "/root/machinex/patches/0221-dox.patch"
patch -p1 < "/root/machinex/patches/0222-dox.patch"
patch -p1 < "/root/machinex/patches/0223-fs-libfs.c-add-generic-data-flush-to-fsync.patch"
patch -p1 < "/root/machinex/patches/0224-dox.patch"
patch -p1 < "/root/machinex/patches/0225-mm-slub.c-convert-vnsprintf-static-to-va_format.patch"
patch -p1 < "/root/machinex/patches/0226-mm-slab-suppress-out-of-memory-warning-unless-debug.patch"
patch -p1 < "/root/machinex/patches/0227-mm-slub-fix-ALLOC_SLOWPATH-stat.patch"
patch -p1 < "/root/machinex/patches/0228-sl-au-b-charge-slabs-to-kmemcg-explicitly.patch"
patch -p1 < "/root/machinex/patches/0229-mm-get-rid-of-__GFP_KMEMCG.patch"
patch -p1 < "/root/machinex/patches/0230-fs-buffer.c-remove-block_write_full_page_endio.patch"
patch -p1 < "/root/machinex/patches/0231-fs-mpage.c-factor-clean_buffers-out-of-__mpage_write.patch"
patch -p1 < "/root/machinex/patches/0232-fs-mpage.c-factor-page_endio-out-of-mpage_end_io.patch"
patch -p1 < "/root/machinex/patches/0233-fs-block_dev.c-add-bdev_read_page-and-bdev_write_pag.patch"
patch -p1 < "/root/machinex/patches/0234-lib-plist-add-plist_requeue.patch"
patch -p1 < "/root/machinex/patches/0235-s-buffer-do-not-use-unnecessary-atomic-operations.patch"
patch -p1 < "/root/machinex/patches/0236-revert-that-one.patch"
patch -p1 < "/root/machinex/patches/0237-fs-superblock-unregister-sb-shrinker-before-kill_sb.patch"
patch -p1 < "/root/machinex/patches/0238-fs-superblock-avoid-locking-counting-inodes-and-dent.patch"
patch -p1 < "/root/machinex/patches/0239-dox.patch"
patch -p1 < "/root/machinex/patches/0240-lib-libcrc32c.c-use-PTR_ERR_OR_ZERO.patch"
patch -p1 < "/root/machinex/patches/0241-lib-plist.c-make-CONFIG_DEBUG_PI_LIST-selectable.patch"
patch -p1 < "/root/machinex/patches/0242-dox.patch"
patch -p1 < "/root/machinex/patches/0243-lib-crc32.c-remove-unnecessary-__constant.patch"
patch -p1 < "/root/machinex/patches/0244-dox.patch"
patch -p1 < "/root/machinex/patches/0245-dox.patch"
patch -p1 < "/root/machinex/patches/0246-binfmt-updates.patch"
patch -p1 < "/root/machinex/patches/0247-autofs-add-__init.patch"
patch -p1 < "/root/machinex/patches/0248-perf-Fix-perf_event_comm-vs.-exec-assumption.patch"
patch -p1 < "/root/machinex/patches/0249-perf-Differentiate-exec-and-non-exec-comm-events.patch"
patch -p1 < "/root/machinex/patches/0250-signals-jffs2-fix-the-wrong-usage-of-disallow_signal.patch"
patch -p1 < "/root/machinex/patches/0251-fs-proc-task_mmu.c-replace-seq_printf-by-seq_puts.patch"
patch -p1 < "/root/machinex/patches/0252-fs-proc-vmcore.c-remove-NULL-assignment-to-static.patch"
patch -p1 < "/root/machinex/patches/0253-dox.patch"
patch -p1 < "/root/machinex/patches/0254-locks-set-fl_owner-for-leases-back-to-current-files.patch"
patch -p1 < "/root/machinex/patches/0255-splice_write-via-write_iter.patch"
patch -p1 < "/root/machinex/patches/0256-fs-splice.c-remove-unneeded-exports.patch"
patch -p1 < "/root/machinex/patches/0257-shmem-switch-to-iter_file_splice_write.patch"
patch -p1 < "/root/machinex/patches/0258-kill-generic_file_splice_write.patch"
patch -p1 < "/root/machinex/patches/0259-lock_parent-don-t-step-on-stale-d_parent.patch"
patch -p1 < "/root/machinex/patches/0260-ext4-Fix-buffer-double-free-in-ext4_alloc_branch.patch"
patch -p1 < "/root/machinex/patches/0261-percpu-Use-ALIGN-macro-instead-of-hand-coding-align.patch"
patch -p1 < "/root/machinex/patches/0262-tmpfs-ZERO_RANGE-and-COLLAPSE_RANGE-not-currently-su.patch"
patch -p1 < "/root/machinex/patches/0263-slab-fix-oops-when-reading-proc-slab_allocators.patch"
patch -p1 < "/root/machinex/patches/0264-aio-squash.patch"
patch -p1 < "/root/machinex/patches/0265-lib-crc32-Greatly-shrink-CRC-combining-code.patch"
patch -p1 < "/root/machinex/patches/0266-lib-crc32-Mark-test-data-__initconst.patch"
patch -p1 < "/root/machinex/patches/0267-lib-crc32-Add-some-additional-__pure-annotations.patch"
patch -p1 < "/root/machinex/patches/0268-fs-mbcache-replace-__builtin_log2-with-ilog2.patch"
patch -p1 < "/root/machinex/patches/0269-block-bitmap-freecluster-dec.patch"
patch -p1 < "/root/machinex/patches/0270-hole-punch-bs.patch"
patch -p1 < "/root/machinex/patches/0271-kernfs-introduce-kernfs_pin_sb.patch"
patch -p1 < "/root/machinex/patches/0272-kernfs-kernfs_notify-must-be-useable-from-non-sleepa.patch"
patch -p1 < "/root/machinex/patches/0273-lib-cpumask-cpumask_set_cpu_local_first-to-use-all-c.patch"
patch -p1 < "/root/machinex/patches/0274-slub-fix-off-by-one-in-number-of-slab-tests.patch"
patch -p1 < "/root/machinex/patches/0275-remove-the-FL_OFDLCK-trace-we-never-added.patch"
patch -p1 < "/root/machinex/patches/0276-proc-stat-convert-to-single_open_size.patch"
patch -p1 < "/root/machinex/patches/0277-ecryptfs-Drop-cast.patch"
patch -p1 < "/root/machinex/patches/0278-ecryptfs-Remove-unnecessary-include-of-syscall.h.patch"
patch -p1 < "/root/machinex/patches/0279-cut-kmem_cache_alloc-tracing-at-its-throat.patch"
patch -p1 < "/root/machinex/patches/0280-dox.patch"
patch -p1 < "/root/machinex/patches/0281-ext4-disable-synchronous-transaction-batching-if-max.patch"
patch -p1 < "/root/machinex/patches/0282-fuse-timeout-comparison-fix.patch"
patch -p1 < "/root/machinex/patches/0283-fuse-ignore-entry-timeout-on-LOOKUP_REVAL.patch"
patch -p1 < "/root/machinex/patches/0284-fuse-inode-drop-cast.patch"
patch -p1 < "/root/machinex/patches/0285-fuse-avoid-scheduling-while-atomic.patch"
patch -p1 < "/root/machinex/patches/0286-blkcg-memcg-make-blkcg-depend-on-memcg-on-the-defaul.patch"
patch -p1 < "/root/machinex/patches/0287-dox.patch"
patch -p1 < "/root/machinex/patches/0288-fs-debugfs-remove-trailing-whitespace.patch"
patch -p1 < "/root/machinex/patches/0289-fuse-restructure-rename2.patch"
patch -p1 < "/root/machinex/patches/0290-ext4-revert-commit-which-was-causing-fs-corruption-a.patch"
patch -p1 < "/root/machinex/patches/0291-ext4-fix-potential-null-pointer-dereference-in-ext4.patch"
patch -p1 < "/root/machinex/patches/0292-fuse-release-temporary-page-if-fuse_writepage_locked.patch"
patch -p1 < "/root/machinex/patches/0293-fuse-replace-count-size-kzalloc-by-kcalloc.patch"
patch -p1 < "/root/machinex/patches/0294-aio-protect-reqs_available-updates-from-changes.patch"
patch -p1 < "/root/machinex/patches/0295-ecryptfs-remove-unnecessary-break-after-goto.patch"
patch -p1 < "/root/machinex/patches/0296-ext4-rearrange-initialization.patch"
patch -p1 < "/root/machinex/patches/0297-ext4-remove-metadata-reservation-checks.patch"
patch -p1 < "/root/machinex/patches/0298-hole-punch-bs.patch"
patch -p1 < "/root/machinex/patches/0299-ext4-remove-readpage-check-in-ext4_mmap_file.patch"
patch -p1 < "/root/machinex/patches/0300-ext4-make-ext4_has_inline_data-as-a-inline-function.patch"
patch -p1 < "/root/machinex/patches/0301-quota-missing-lock-in-dqcache_shrink_scan.patch"
patch -p1 < "/root/machinex/patches/0302-quota-protect-Q_GETFMT-by-dqonoff_mutex.patch"
patch -p1 < "/root/machinex/patches/0303-quota-avoid-unnecessary-dqget-dqput-calls.patch"
patch -p1 < "/root/machinex/patches/0304-quota-simplify-remove_inode_dquot_ref.patch"
patch -p1 < "/root/machinex/patches/0305-quota-remove-dqptr_sem.patch"
patch -p1 < "/root/machinex/patches/0306-fs-ext2-super.c-Drop-memory-allocation-cast.patch"
patch -p1 < "/root/machinex/patches/0307-dox.patch"
patch -p1 < "/root/machinex/patches/0308-aio-remove-no-longer-needed-preempt_disable.patch"
patch -p1 < "/root/machinex/patches/0309-fuse-s_time_gran-fix.patch"
patch -p1 < "/root/machinex/patches/0310-fuse-add-FUSE_NO_OPEN_SUPPORT-flag-to-INIT.patch"
patch -p1 < "/root/machinex/patches/0311-coredump-fix-the-setting-of-PF_DUMPCORE.patch"
patch -p1 < "/root/machinex/patches/0312-simple_xattr-permit-0-size-extended-attributes.patch"
patch -p1 < "/root/machinex/patches/0313-direct-io-fix-uninitialized-warning-in-do_direct_IO.patch"
patch -p1 < "/root/machinex/patches/0314-fs-umount-on-symlink-leaks-mnt-count.patch"
patch -p1 < "/root/machinex/patches/0315-aio-squash.patch"
patch -p1 < "/root/machinex/patches/0316-ext4-add-i_data_sem-sanity-check.patch"
patch -p1 < "/root/machinex/patches/0317-ext4-use-correct-depth-value.patch"
patch -p1 < "/root/machinex/patches/0318-ext4-fix-incorrect-locking-in-move_extent_per_page.patch"
patch -p1 < "/root/machinex/patches/0319-ceph-remove-redundant-memset-0.patch"
patch -p1 < "/root/machinex/patches/0320-dox.patch"
patch -p1 < "/root/machinex/patches/0321-ext4-check-inline-directory-before-converting.patch"
patch -p1 < "/root/machinex/patches/0322-ext4-fix-COLLAPSE-RANGE-test-for-bigalloc-file-syste.patch"
patch -p1 < "/root/machinex/patches/0323-bs.patch"
patch -p1 < "/root/machinex/patches/0324-mnt-Move-the-test-for-MNT_LOCK_READONLY.patch"
patch -p1 < "/root/machinex/patches/0325-mnt-Correct-permission-checks-in-do_remount.patch"
patch -p1 < "/root/machinex/patches/0326-mnt-Change-the-default-remount-atime.patch"
patch -p1 < "/root/machinex/patches/0327-direct-io-fix-AIO-regression.patch"
patch -p1 < "/root/machinex/patches/0328-vfs-fix-check-for-fallocate-on-active-swapfile.patch"
patch -p1 < "/root/machinex/patches/0329-lib-Resizable-Scalable-Concurrent-Hash-Table.patch"
patch -p1 < "/root/machinex/patches/0330-ohhh-thought-it-was-4-args.patch"
patch -p1 < "/root/machinex/patches/0331-god-damn-it.patch"
patch -p1 < "/root/machinex/patches/0332-remove-plist-requeue-for-now.patch"
patch -p1 < "/root/machinex/patches/0333-fucking-tracing-man.patch"
patch -p1 < "/root/machinex/patches/0334-possing-back-to-cgroup-remove-cgroup-name.patch"
patch -p1 < "/root/machinex/patches/0335-possed.patch"
patch -p1 < "/root/machinex/patches/0336-gonna-sop.patch"
patch -p1 < "/root/machinex/patches/0337-sopped.patch"
patch -p1 < "/root/machinex/patches/0338-bring-back-the-NULL-check-for-page.patch"
patch -p1 < "/root/machinex/patches/0339-more-Not-the-right-machine-type-details.patch"
patch -p1 < "/root/machinex/patches/0340-updates-to-reboot-bringing-it-up-to-mainline-there-h.patch"
patch -p1 < "/root/machinex/patches/0341-there-synced-reboot-mostly-with-mainline.patch"
patch -p1 < "/root/machinex/patches/0342-androidify-our-reboot-path.patch"
patch -p1 < "/root/machinex/patches/0343-a-few-minor-cleanups-in-sys.c-also-moved-the-autogro.patch"
patch -p1 < "/root/machinex/patches/0344-import-reset-drivers-from-samsung-gs8.patch"
patch -p1 < "/root/machinex/patches/0345-stole-notifier-logic-from-s8-restart-driver.patch"
patch -p1 < "/root/machinex/patches/0346-fix-my-reboot-mode-in-arm-setup.patch"
patch -p1 < "/root/machinex/patches/0347-fixup-setup-asm-also-use-online-cpu-in-c_show.patch"
patch -p1 < "/root/machinex/patches/0348-another-fix.-motherFUCK-jian-yang.patch"
patch -p1 < "/root/machinex/patches/0349-did-it-just-imported-the-platform-driver-code-ideas-.patch"
patch -p1 < "/root/machinex/patches/0350-remove-improper-reboot-notifier-usage-change-back-to.patch"
patch -p1 < "/root/machinex/patches/0351-put-the-reboot-command-back-in-sbin-not-actually-jus.patch"
patch -p1 < "/root/machinex/patches/0352-i-think-early-is-too-much-for-it-now.patch"
patch -p1 < "/root/machinex/patches/0353-getting-rid-of-reboot-experiments-just-doesnt-work-w.patch"
patch -p1 < "/root/machinex/patches/0354-done.patch"
patch -p1 < "/root/machinex/patches/0355-fs-fscache-make-ctl_table-static.patch"
patch -p1 < "/root/machinex/patches/0356-dox.patch"
patch -p1 < "/root/machinex/patches/0357-squashfs-cleanup.patch"
patch -p1 < "/root/machinex/patches/0358-m-slab.c-add-__init-to-init_lock_keys.patch"
patch -p1 < "/root/machinex/patches/0359-printk-rename-DEFAULT_MESSAGE_LOGLEVEL.patch"
patch -p1 < "/root/machinex/patches/0360-add-useless-glob-libs.patch"
patch -p1 < "/root/machinex/patches/0361-lib-test-kstrtox.c-use-ARRAY_SIZE-instead-of-sizeof.patch"
patch -p1 < "/root/machinex/patches/0362-lib_sort_test-updates.patch"
patch -p1 < "/root/machinex/patches/0363-lib-list_sort.c-convert-to-pr_foo.patch"
patch -p1 < "/root/machinex/patches/0364-VFS-allow-d_manage-to-declare-EISDIR-in-rcu_walk-mod.patch"
patch -p1 < "/root/machinex/patches/0365-update-config.patch"
