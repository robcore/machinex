#!/bin/bash
patch -p1 -R < "/root/machinex/patches/0262-smarten-up-my-cpu-hardplug-interface.patch"
patch -p1 -R < "/root/machinex/patches/0261-REVERT-the-proc-hax.patch"
patch -p1 -R < "/root/machinex/patches/0260-leds-speed-up-the-shutdown-and-fix-my-colours.patch"
patch -p1 -R < "/root/machinex/patches/0259-namei-going-hard-on-my-null-pointer-checks.patch"
patch -p1 -R < "/root/machinex/patches/0258-thermal-still-overrides-bringing-cpus-up.patch"
patch -p1 -R < "/root/machinex/patches/0257-move-the-check-to-is_cpu_allowed-before-down_lock-ha.patch"
patch -p1 -R < "/root/machinex/patches/0256-oops-actually-fix-it.patch"
patch -p1 -R < "/root/machinex/patches/0255-bracket-fix.patch"
patch -p1 -R < "/root/machinex/patches/0254-fix-my-header-def-for-cpu_hardplugged_mask.patch"
patch -p1 -R < "/root/machinex/patches/0253-fixed-a-bracket-in-cpu.c.patch"
patch -p1 -R < "/root/machinex/patches/0252-further-integrate-hardplug-into-the-other-drivers.patch"
patch -p1 -R < "/root/machinex/patches/0251-cpu-hardplug-some-preparation-for-controlling-cpus-u.patch"
patch -p1 -R < "/root/machinex/patches/0250-and-more.patch"
patch -p1 -R < "/root/machinex/patches/0249-proc-hacking-part-3.patch"
patch -p1 -R < "/root/machinex/patches/0248-proc-hacking-part-2.patch"
patch -p1 -R < "/root/machinex/patches/0247-proc-hacking.patch"
patch -p1 -R < "/root/machinex/patches/0246-REVERT-removal-of-proc-tty.patch"
patch -p1 -R < "/root/machinex/patches/0245-proc-fix-the-potential-use-after-free-in-first_tid.patch"
patch -p1 -R < "/root/machinex/patches/0244-revert-fs-ramfs-file-nommu.c-replace-count-size-kzal.patch"
patch -p1 -R < "/root/machinex/patches/0243-those-were-fine-new-plan.patch"
patch -p1 -R < "/root/machinex/patches/0242-revert-the-majority-of-the-mm-commits-from-last-nigh.patch"
patch -p1 -R < "/root/machinex/patches/0241-trying-a-nifty-idea-with-leds-to-give-us-an-alternat.patch"
patch -p1 -R < "/root/machinex/patches/0240-do-the-same-for-thermal-and-remove-the-the-thermal-c.patch"
patch -p1 -R < "/root/machinex/patches/0239-Intelliplug-add-a-hardplug-hook-also-a-couple-of-sma.patch"
patch -p1 -R < "/root/machinex/patches/0238-give-our-__init-an-__init.patch"
patch -p1 -R < "/root/machinex/patches/0237-core-control-ALWAYS-wins.patch"
patch -p1 -R < "/root/machinex/patches/0236-syfs-interface-up-and-running.patch"
patch -p1 -R < "/root/machinex/patches/0235-those-gotos-were-unneccessary.patch"
patch -p1 -R < "/root/machinex/patches/0234-gonna-have-to-make-our-initcall-earlier-given-cpu-re.patch"
patch -p1 -R < "/root/machinex/patches/0233-oops-include-display_state.patch"
patch -p1 -R < "/root/machinex/patches/0232-REVERT-switching-vmalloc-to-rcu-loist.patch"
patch -p1 -R < "/root/machinex/patches/0231-add-a-hook-into-__cpu__up-that-theoretically-should-.patch"
patch -p1 -R < "/root/machinex/patches/0230-add-some-SUPER-important-configs.patch"
patch -p1 -R < "/root/machinex/patches/0229-cpu_hardplug-0.2-start-to-put-the-framework-together.patch"
patch -p1 -R < "/root/machinex/patches/0228-REVERT-fs-proc-kcore.c-use-PAGE_ALIGN-instead-of-ALI.patch"
patch -p1 -R < "/root/machinex/patches/0227-revert-proc-commits.patch"
patch -p1 -R < "/root/machinex/patches/0226-intelli-actually-move-my-mutex-init-to-a-saner-place.patch"
patch -p1 -R < "/root/machinex/patches/0225-added-a-little-intro-message-in-intelli-for-my-own-r.patch"
patch -p1 -R < "/root/machinex/patches/0224-include-sysfs_helpers.patch"
patch -p1 -R < "/root/machinex/patches/0223-add-an-extern-ref-to-limit_screen_off_cpus.patch"
patch -p1 -R < "/root/machinex/patches/0222-there.-so-far-cpu_hardplug-is-just-a-dummy-driver-to.patch"
patch -p1 -R < "/root/machinex/patches/0221-just-revert-the-proc-seqfile-conversions.patch"
patch -p1 -R < "/root/machinex/patches/0220-rever-my-proc-revert.patch"
patch -p1 -R < "/root/machinex/patches/0219-working-on-cpu-hardplug.patch"
patch -p1 -R < "/root/machinex/patches/0218-revert-proc-conversions.patch"
patch -p1 -R < "/root/machinex/patches/0217-fuck-it-proc-pid-cmdline-can-stay-the-way-it-is.patch"
patch -p1 -R < "/root/machinex/patches/0216-REVERT-kexec-export-free_huge_page-to-VMCOREINFO.patch"
patch -p1 -R < "/root/machinex/patches/0215-fix-hlist_add_behind_rcu.patch"
patch -p1 -R < "/root/machinex/patches/0214-proportions-add-gfp-to-init-functions.patch"
patch -p1 -R < "/root/machinex/patches/0213-aio-block-exit_aio-until-all-context-requests.patch"
patch -p1 -R < "/root/machinex/patches/0212-lib-rhashtable-allow-user-to-set-the-minimum-shifts.patch"
patch -p1 -R < "/root/machinex/patches/0211-rhashtable-fix-lockdep-splat-in-rhashtable_destroy.patch"
patch -p1 -R < "/root/machinex/patches/0210-aio-add-missing-smp_rmb-in-read_events_ring.patch"
patch -p1 -R < "/root/machinex/patches/0209-ext4-remove-a-duplicate-call-in-ext4_init_new_dir.patch"
patch -p1 -R < "/root/machinex/patches/0208-ext4-fix-same-dir-rename-when-inline-data-directory-.patch"
patch -p1 -R < "/root/machinex/patches/0207-ext4-update-i_disksize-coherently-with-block-alloc.patch"
patch -p1 -R < "/root/machinex/patches/0206-dox.patch"
patch -p1 -R < "/root/machinex/patches/0205-ext4-fix-transaction-issues-for-ext4_fallocate.patch"
patch -p1 -R < "/root/machinex/patches/0204-ext4-fix-incorect-journal-credits-reservation.patch"
patch -p1 -R < "/root/machinex/patches/0203-FS-Cache-Reduce-cookie-ref-count-if-submit-fails.patch"
patch -p1 -R < "/root/machinex/patches/0202-FS-Cache-Timeout-for-releasepage.patch"
patch -p1 -R < "/root/machinex/patches/0201-timerfd-Remove-an-always-true-check.patch"
patch -p1 -R < "/root/machinex/patches/0200-dox.patch"
patch -p1 -R < "/root/machinex/patches/0199-lib-rhashtable-Spelling-s-compuate-compute.patch"
patch -p1 -R < "/root/machinex/patches/0198-aio-fix-reqs_available-handling.patch"
patch -p1 -R < "/root/machinex/patches/0197-ext4-move-i_size-i_disksize-update-routines-to-helpe.patch"
patch -p1 -R < "/root/machinex/patches/0196-locks-pass-correct-before-pointer-to-locks_unlink.patch"
patch -p1 -R < "/root/machinex/patches/0195-ext3-Count-internal-journal-as-bsddf-overhead.patch"
patch -p1 -R < "/root/machinex/patches/0194-rhashtable-unexport-and-make-rht_obj-static.patch"
patch -p1 -R < "/root/machinex/patches/0193-rhashtable-RCU-annotations-for-next-pointers.patch"
patch -p1 -R < "/root/machinex/patches/0192-locks-move-locks_free_lock-calls.patch"
patch -p1 -R < "/root/machinex/patches/0191-locks-defer-freeing-locks-in-locks_delete_lock.patch"
patch -p1 -R < "/root/machinex/patches/0190-locking-arch-Rewrite-generic-atomic-support.patch"
patch -p1 -R < "/root/machinex/patches/0189-bs.patch"
patch -p1 -R < "/root/machinex/patches/0188-locks-don-t-call-locks_release_private.patch"
patch -p1 -R < "/root/machinex/patches/0187-locks-show-delegations-as-DELEG-in-proc-locks.patch"
patch -p1 -R < "/root/machinex/patches/0186-fix-copy_tree-regression.patch"
patch -p1 -R < "/root/machinex/patches/0185-__generic_file_write_iter-fix-handling-of-sync-error.patch"
patch -p1 -R < "/root/machinex/patches/0184-Revert-proc-Point-proc-mounts-net-at-proc-thread-sel.patch"
patch -p1 -R < "/root/machinex/patches/0183-lib-scatterlist-make-ARCH_HAS_SG_CHAIN-an-actual-Kco.patch"
patch -p1 -R < "/root/machinex/patches/0182-proc-convert-everything-to-seq-interface.patch"
patch -p1 -R < "/root/machinex/patches/0181-proc-remove-proc_tty_ldisc-variable.patch"
patch -p1 -R < "/root/machinex/patches/0180-proc-faster-proc-PID-lookup.patch"
patch -p1 -R < "/root/machinex/patches/0179-proc-add-and-remove-proc-entry-create-checks.patch"
patch -p1 -R < "/root/machinex/patches/0178-fs-proc-kcore.c-use-PAGE_ALIGN-instead-of-ALIGN-PAGE.patch"
patch -p1 -R < "/root/machinex/patches/0177-fs-ramfs-file-nommu.c-replace-count-size-kzalloc.patch"
patch -p1 -R < "/root/machinex/patches/0176-fs-efs-namei.c-return-is-not-a-function.patch"
patch -p1 -R < "/root/machinex/patches/0175-switch-iov_iter_get_pages-to-passing-maximal-number.patch"
patch -p1 -R < "/root/machinex/patches/0174-fs-mark-__d_obtain_alias-static.patch"
patch -p1 -R < "/root/machinex/patches/0173-dcache-d_splice_alias-should-detect-loops.patch"
patch -p1 -R < "/root/machinex/patches/0172-dcache-d_find_alias-needn-t-recheck-IS_ROOT-DCACHE_D.patch"
patch -p1 -R < "/root/machinex/patches/0171-dcache-remove-unused-d_find_alias-parameter.patch"
patch -p1 -R < "/root/machinex/patches/0170-dcache-d_obtain_alias-callers-dont-all-want-DISCONNE.patch"
patch -p1 -R < "/root/machinex/patches/0169-dcache-d_splice_alias-should-ignore-DCACHE_DISCONNEC.patch"
patch -p1 -R < "/root/machinex/patches/0168-dcache-d_splice_alias-mustn-t-create-directory-alias.patch"
patch -p1 -R < "/root/machinex/patches/0167-dcache-close-d_move-race-in-d_splice_alias.patch"
patch -p1 -R < "/root/machinex/patches/0166-dcache-move-d_splice_alias.patch"
patch -p1 -R < "/root/machinex/patches/0165-dox.patch"
patch -p1 -R < "/root/machinex/patches/0164-rever-tthat.patch"
patch -p1 -R < "/root/machinex/patches/0163-acct-switch-to-__kernel_write.patch"
patch -p1 -R < "/root/machinex/patches/0162-mm-writeback-prevent-race-when-calculating-dirty-lim.patch"
patch -p1 -R < "/root/machinex/patches/0161-shmem-fix-double-uncharge-in-__shmem_file_setup.patch"
patch -p1 -R < "/root/machinex/patches/0160-vmalloc-use-rcu-list-iterator-to-reduce-vmap_area_lo.patch"
patch -p1 -R < "/root/machinex/patches/0159-proc-Point-proc-mounts-at-proc-thread-self-mounts.patch"
patch -p1 -R < "/root/machinex/patches/0158-proc-Point-proc-net-at-proc-thread-self-net-instead.patch"
patch -p1 -R < "/root/machinex/patches/0157-proc-Implement-proc-thread-self-to-point-at-the-dire.patch"
patch -p1 -R < "/root/machinex/patches/0156-proc-Have-net-show-up-under-proc-tgid-task-tid.patch"
patch -p1 -R < "/root/machinex/patches/0155-kexec-export-free_huge_page-to-VMCOREINFO.patch"
patch -p1 -R < "/root/machinex/patches/0154-memcg-oom_notify-use-after-free-fix.patch"
patch -p1 -R < "/root/machinex/patches/0153-mm-page-writeback.c-fix-divide-by-zero-in-bdi_dirty_.patch"
patch -p1 -R < "/root/machinex/patches/0152-JFS-Check-for-NULL-before-calling-posix_acl_equiv.patch"
patch -p1 -R < "/root/machinex/patches/0151-scatterlist-allow-chaining-to-preallocated-chunks.patch"
patch -p1 -R < "/root/machinex/patches/0150-ROB-HACK-remove-unlikely-from-the-cpu_is_offline-tes.patch"
patch -p1 -R < "/root/machinex/patches/0149-fix-it.patch"
patch -p1 -R < "/root/machinex/patches/0148-locks-typedef-fl_owner_t-to-void.patch"
patch -p1 -R < "/root/machinex/patches/0147-locks-purge-fl_owner_t-from-fs-locks.c.patch"
patch -p1 -R < "/root/machinex/patches/0146-locks-rename-FL_FILE_PVT-and-IS_FILE_PVT-to-use-_OFD.patch"
patch -p1 -R < "/root/machinex/patches/0145-locks-rename-file-private-locks-to-open-file-descrip.patch"
patch -p1 -R < "/root/machinex/patches/0144-Revert-cgroup-remove-redundant-variable-in-cgroup_mo.patch"
patch -p1 -R < "/root/machinex/patches/0143-cgroup-remove-redundant-variable-in-cgroup_mount.patch"
patch -p1 -R < "/root/machinex/patches/0142-cgroup-fix-missing-unlock-in-cgroup_release_agent.patch"
patch -p1 -R < "/root/machinex/patches/0141-cgroup-remove-CGRP_RELEASABLE-flag.patch"
patch -p1 -R < "/root/machinex/patches/0140-cgroup-simplify-proc_cgroup_show.patch"
patch -p1 -R < "/root/machinex/patches/0139-cgroup-use-a-per-cgroup-work-for-release-agent.patch"
patch -p1 -R < "/root/machinex/patches/0138-cgroup-fix-unbalanced-locking.patch"
patch -p1 -R < "/root/machinex/patches/0137-cgroup-remove-bogus-comments.patch"
patch -p1 -R < "/root/machinex/patches/0136-cgroup-remove-redundant-code-in-cgroup_rmdir.patch"
patch -p1 -R < "/root/machinex/patches/0135-cgroup-remove-some-useless-forward-declarations.patch"
patch -p1 -R < "/root/machinex/patches/0134-cgroup-check-cgroup-liveliness-before-unbreaking-ker.patch"
patch -p1 -R < "/root/machinex/patches/0133-cgroup-delay-the-clearing-of-cgrp-kn-priv.patch"
patch -p1 -R < "/root/machinex/patches/0132-dox.patch"
patch -p1 -R < "/root/machinex/patches/0131-cgroup-Display-legacy-cgroup-files-on-default-hierar.patch"
patch -p1 -R < "/root/machinex/patches/0130-cgroup-reject-cgroup-names.patch"
patch -p1 -R < "/root/machinex/patches/0129-cgroup-initialize-cgrp_dfl_root_inhibit_ss_mask.patch"
patch -p1 -R < "/root/machinex/patches/0128-cgroup-make-CFTYPE_ONLY_ON_DFL-and-CFTYPE_NO_-intern.patch"
patch -p1 -R < "/root/machinex/patches/0127-cgroup-distinguish-the-default-and-legacy-hierarchie.patch"
patch -p1 -R < "/root/machinex/patches/0126-cgroup-replace-cgroup_add_cftypes-with-cgroup_add_le.patch"
patch -p1 -R < "/root/machinex/patches/0125-cgroup-rename-cgroup_subsys-base_cftypes-to-legacy_c.patch"
patch -p1 -R < "/root/machinex/patches/0124-debugfs-Fix-corrupted-loop.patch"
patch -p1 -R < "/root/machinex/patches/0123-blkcg-memcg-make-blkcg-depend-on-memcg-on-the-defaul.patch"
patch -p1 -R < "/root/machinex/patches/0122-cgroup-split-cgroup_base_files-into-cgroup_-dfl-lega.patch"
patch -p1 -R < "/root/machinex/patches/0121-cgroup-clean-up-sane_behavior-handling.patch"
patch -p1 -R < "/root/machinex/patches/0120-cgroup-remove-sane_behavior-support-on-non-default-h.patch"
patch -p1 -R < "/root/machinex/patches/0119-cgroup-make-interface-file-cgroup.sane_behavior-lega.patch"
patch -p1 -R < "/root/machinex/patches/0118-cgroup-remove-CGRP_ROOT_OPTION_MASK.patch"
patch -p1 -R < "/root/machinex/patches/0117-cgroup-implement-cgroup_subsys-depends_on.patch"
patch -p1 -R < "/root/machinex/patches/0116-cgroup-implement-cgroup_subsys-css_reset.patch"
patch -p1 -R < "/root/machinex/patches/0115-cgroup-make-interface-files-visible-iff-enabled.patch"
patch -p1 -R < "/root/machinex/patches/0114-cgroup-introduce-cgroup-subtree_control.patch"
patch -p1 -R < "/root/machinex/patches/0113-cgroup-reorganize-cgroup_subtree_control_write.patch"
patch -p1 -R < "/root/machinex/patches/0112-cgroup-fix-a-race-between-cgroup_mount-and-cgroup_ki.patch"
patch -p1 -R < "/root/machinex/patches/0111-cgroup-fix-mount-failure-in-a-corner-case.patch"
patch -p1 -R < "/root/machinex/patches/0110-cgroup-fix-broken-css_has_online_children.patch"
patch -p1 -R < "/root/machinex/patches/0109-cgroup-disallow-disabled-controllers-on-the-default.patch"
patch -p1 -R < "/root/machinex/patches/0108-cgroup-don-t-destroy-the-default-root.patch"
patch -p1 -R < "/root/machinex/patches/0107-cgroup-disallow-debug-controller-on-the-default-hier.patch"
patch -p1 -R < "/root/machinex/patches/0106-cgroup-implement-css_tryget.patch"
patch -p1 -R < "/root/machinex/patches/0105-device_cgroup-use-css_has_online_children-instead.patch"
patch -p1 -R < "/root/machinex/patches/0104-cgroup-convert-cgroup_has_live_children-into-css.patch"
patch -p1 -R < "/root/machinex/patches/0103-cgroup-use-CSS_ONLINE-instead-of-CGRP_DEAD.patch"
patch -p1 -R < "/root/machinex/patches/0102-cgroup-iterate-cgroup_subsys_states-directly.patch"
patch -p1 -R < "/root/machinex/patches/0101-cgroup-introduce-CSS_RELEASED-and-reduce-css-iter.patch"
patch -p1 -R < "/root/machinex/patches/0100-cgroup-move-cgroup-serial_nr-into-cgroup_subsys_stat.patch"
patch -p1 -R < "/root/machinex/patches/0099-cgroup-link-all-cgroup_subsys_states-in-their-siblin.patch"
patch -p1 -R < "/root/machinex/patches/0098-cgroup-move-cgroup-sibling-and-children-into-cgroup_.patch"
patch -p1 -R < "/root/machinex/patches/0097-cgroup-remove-cgroup-parent.patch"
patch -p1 -R < "/root/machinex/patches/0096-device_cgroup-remove-direct-access-to-cgroup-childre.patch"
patch -p1 -R < "/root/machinex/patches/0095-cgroup-remove-css_parent.patch"
patch -p1 -R < "/root/machinex/patches/0094-fix-cgroup_assert_mutex_or_rcu_locked.patch"
patch -p1 -R < "/root/machinex/patches/0093-aaaand-percpu_ref_exit.patch"
patch -p1 -R < "/root/machinex/patches/0092-oops-actually-fix-it.patch"
patch -p1 -R < "/root/machinex/patches/0091-fixup-my-ref-init.patch"
patch -p1 -R < "/root/machinex/patches/0090-cgroup-skip-refcnting-on-normal-root-csses.patch"
patch -p1 -R < "/root/machinex/patches/0089-cgroup-use-cgroup-self.refcnt-for-cgroup-refcnting.patch"
patch -p1 -R < "/root/machinex/patches/0088-cgroup-enable-refcnting-for-root-csses.patch"
patch -p1 -R < "/root/machinex/patches/0087-cgroup-bounce-css-release-through-css-destroy_work.patch"
patch -p1 -R < "/root/machinex/patches/0086-cgroup-remove-cgroup_destory_css_killed.patch"
patch -p1 -R < "/root/machinex/patches/0085-cgroup-move-cgroup-sibling-unlinking-to-cgroup_put.patch"
patch -p1 -R < "/root/machinex/patches/0084-cgroup-move-check_for_release-parent-call-to-the-end.patch"
patch -p1 -R < "/root/machinex/patches/0083-cgroup-separate-out-cgroup_has_live_children-from-cg.patch"
patch -p1 -R < "/root/machinex/patches/0082-cgroup-rename-cgroup-dummy_css-to-self-and-move-it.patch"
patch -p1 -R < "/root/machinex/patches/0081-cgroup-use-restart_syscall-for-mount-retries.patch"
patch -p1 -R < "/root/machinex/patches/0080-cgroup-remove-cgroup_tree_mutex.patch"
patch -p1 -R < "/root/machinex/patches/0079-cgroup-nest-kernfs-active-protection-under-cgroup_mu.patch"
patch -p1 -R < "/root/machinex/patches/0078-cgroup-use-cgroup_kn_lock_live-in-other-cgroup-kernf.patch"
patch -p1 -R < "/root/machinex/patches/0077-cgroup-factor-out-cgroup_kn_lock_live-and-cgroup_kn.patch"
patch -p1 -R < "/root/machinex/patches/0076-cgroup-move-cgroup-kn-priv-clearing-to-cgroup_rmdir.patch"
patch -p1 -R < "/root/machinex/patches/0075-cgroup-grab-cgroup_mutex-earlier-in-cgroup_subtree_c.patch"
patch -p1 -R < "/root/machinex/patches/0074-cgroup-collapse-cgroup_create-into-croup_mkdir.patch"
patch -p1 -R < "/root/machinex/patches/0073-cgroup-reorganize-cgroup_create.patch"
patch -p1 -R < "/root/machinex/patches/0072-cgroup-remove-cgroup-control_kn.patch"
patch -p1 -R < "/root/machinex/patches/0071-cgroup-convert-tasks-and-cgroup.procs-handle-to-use-.patch"
patch -p1 -R < "/root/machinex/patches/0070-cgroup-replace-cftype-trigger-with-cftype-write.patch"
patch -p1 -R < "/root/machinex/patches/0069-cgroup-replace-cftype-write_string-with-cftype-write.patch"
patch -p1 -R < "/root/machinex/patches/0068-cgroup-implement-cftype-write.patch"
patch -p1 -R < "/root/machinex/patches/0067-cgroup-rename-css_tryget-to-css_tryget_online.patch"
patch -p1 -R < "/root/machinex/patches/0066-cgroup-use-release_agent_path_lock-in-cgroup_release.patch"
patch -p1 -R < "/root/machinex/patches/0065-cgroup-use-restart_syscall-for-retries-after-offline.patch"
patch -p1 -R < "/root/machinex/patches/0064-cgroup-update-and-fix-parsing-of-cgroup.subtree_cont.patch"
patch -p1 -R < "/root/machinex/patches/0063-cgroup-css_release-shouldn-t-clear-cgroup-subsys.patch"
patch -p1 -R < "/root/machinex/patches/0062-cgroup-cgroup_idr_lock-should-be-bh.patch"
patch -p1 -R < "/root/machinex/patches/0061-cgroup-fix-offlining-child-waiting-in-cgroup_subtree.patch"
patch -p1 -R < "/root/machinex/patches/0060-cgroup-fix-rcu_read_lock-leak-in-update_if_frozen.patch"
patch -p1 -R < "/root/machinex/patches/0059-cgroup_freezer-replace-freezer-lock-with-freezer_mut.patch"
patch -p1 -R < "/root/machinex/patches/0058-cgroup-introduce-task_css_is_root.patch"
patch -p1 -R < "/root/machinex/patches/0057-dox.patch"
patch -p1 -R < "/root/machinex/patches/0056-cgroup-memcg-implement-css-id-and-convert-css_from_i.patch"
patch -p1 -R < "/root/machinex/patches/0055-okay-now-is-the-moment-of-truth-lets-see-if-we-can-g.patch"
patch -p1 -R < "/root/machinex/patches/0054-at-least-let-me-get-rid-of-the-unneccessary-brackets.patch"
patch -p1 -R < "/root/machinex/patches/0053-kgsl-fuck-it-keep-failing-to-func_end-if-thats-your-.patch"
patch -p1 -R < "/root/machinex/patches/0052-oh-we-DO-need-that-ERR_PTR.patch"
patch -p1 -R < "/root/machinex/patches/0051-kgsl-idr_alloc-make-the-fail-sequence-just-a-billion.patch"
patch -p1 -R < "/root/machinex/patches/0050-revert-kgsl-stuff-keep-my-fixes.patch"
patch -p1 -R < "/root/machinex/patches/0049-proper-err_ptr-return-in-drawctxt.patch"
patch -p1 -R < "/root/machinex/patches/0048-a-little-more-hacking.patch"
patch -p1 -R < "/root/machinex/patches/0047-missing-bracket.patch"
patch -p1 -R < "/root/machinex/patches/0046-more-kgsl-hacking.patch"
patch -p1 -R < "/root/machinex/patches/0045-fixup-my-flags-in-kgsl.patch"
patch -p1 -R < "/root/machinex/patches/0044-thermal.patch"
patch -p1 -R < "/root/machinex/patches/0043-uio.patch"
patch -p1 -R < "/root/machinex/patches/0042-useless.patch"
patch -p1 -R < "/root/machinex/patches/0041-tx.patch"
patch -p1 -R < "/root/machinex/patches/0040-scsi.patch"
patch -p1 -R < "/root/machinex/patches/0039-ppp.patch"
patch -p1 -R < "/root/machinex/patches/0038-slimbus-trying-cyclic.patch"
patch -p1 -R < "/root/machinex/patches/0037-inotify_user.patch"
patch -p1 -R < "/root/machinex/patches/0036-super.patch"
patch -p1 -R < "/root/machinex/patches/0035-kernel-events-core.patch"
patch -p1 -R < "/root/machinex/patches/0034-a-little-wq-cleanup.i-like-some-of-my-custom-changes.patch"
patch -p1 -R < "/root/machinex/patches/0033-workqueue-implicit-ordered-attribute-should-be-overr.patch"
patch -p1 -R < "/root/machinex/patches/0032-workqueue-restore-WQ_UNBOUND-max_active-1-to-be-orde.patch"
patch -p1 -R < "/root/machinex/patches/0031-cgroups-and-workqueues.patch"
patch -p1 -R < "/root/machinex/patches/0030-cgroup.patch"
patch -p1 -R < "/root/machinex/patches/0029-i2c-md-iommu-and-mmc-host.patch"
patch -p1 -R < "/root/machinex/patches/0028-kgsl.patch"
patch -p1 -R < "/root/machinex/patches/0027-gpiolib.patch"
patch -p1 -R < "/root/machinex/patches/0026-loop.patch"
patch -p1 -R < "/root/machinex/patches/0025-block.patch"
patch -p1 -R < "/root/machinex/patches/0024-screw-it-just-went-ahead-and-synced-with-3.18.patch"
patch -p1 -R < "/root/machinex/patches/0023-reverted.patch"
patch -p1 -R < "/root/machinex/patches/0022-hold-up.patch"
patch -p1 -R < "/root/machinex/patches/0021-idr-implement-idr_preload-_end-and-idr_alloc.patch"
patch -p1 -R < "/root/machinex/patches/0020-possed.patch"
patch -p1 -R < "/root/machinex/patches/0019-possing.patch"
patch -p1 -R < "/root/machinex/patches/0018-ohhhh-shit.patch"
patch -p1 -R < "/root/machinex/patches/0017-idr-reorder-the-fields.patch"
patch -p1 -R < "/root/machinex/patches/0016-idr-reduce-the-unneeded-check-in-free_layer.patch"
patch -p1 -R < "/root/machinex/patches/0015-idr-don-t-need-to-shink-the-free-list-when-idr_remov.patch"
patch -p1 -R < "/root/machinex/patches/0014-idr-fix-idr_replace-s-returned-error-code.patch"
patch -p1 -R < "/root/machinex/patches/0013-idr-fix-NULL-pointer-dereference-when-ida_remove.patch"
patch -p1 -R < "/root/machinex/patches/0012-idr-fix-unexpected-ID-removal-when-idr_remove-unallo.patch"
patch -p1 -R < "/root/machinex/patches/0011-idr-fix-overflow-bug-during-maximum-ID-calculation.patch"
patch -p1 -R < "/root/machinex/patches/0010-dox.patch"
patch -p1 -R < "/root/machinex/patches/0009-lib-idr.c-fix-out-of-bounds-pointer-dereference.patch"
patch -p1 -R < "/root/machinex/patches/0008-idr-explain-WARN_ON_ONCE-on-negative-IDs-out-of-rang.patch"
patch -p1 -R < "/root/machinex/patches/0007-idr-implement-lookup-hint.patch"
patch -p1 -R < "/root/machinex/patches/0006-idr-add-idr_layer-prefix.patch"
patch -p1 -R < "/root/machinex/patches/0005-idr-make-idr_layer-larger.patch"
patch -p1 -R < "/root/machinex/patches/0004-idr-remove-length-restriction-from-idr_layer-bitmap.patch"
patch -p1 -R < "/root/machinex/patches/0003-idr-remove-MAX_IDR_MASK-and-move-left-MAX_IDR_-into-.patch"
patch -p1 -R < "/root/machinex/patches/0002-idr-fix-top-layer-handling.patch"
patch -p1 -R < "/root/machinex/patches/0001-idr-implement-idr_preload-_end-and-idr_alloc.patch"
