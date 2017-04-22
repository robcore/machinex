#!/bin/bash
patch -p1 -R < "/root/machinex/patches/0107-done-fucked-it-alllll-up.patch"
patch -p1 -R < "/root/machinex/patches/0106-cgroup-convert-to-kernfs.patch"
patch -p1 -R < "/root/machinex/patches/0105-sysfs-kernfs-drop-unused-params-from-sysfs_fill_sup.patch"
patch -p1 -R < "/root/machinex/patches/0104-sysfs-kernfs-move-symlink-core-code.patch"
patch -p1 -R < "/root/machinex/patches/0103-sysfs-kernfs-move-file-core-code-to-fs-kernfs-file.c.patch"
patch -p1 -R < "/root/machinex/patches/0102-sysfs-kernfs-move-dir-core-code-to-fs-kernfs-dir.c.patch"
patch -p1 -R < "/root/machinex/patches/0101-sysfs-kernfs-move-internal-decls-to-fs-kernfs-kernfs.patch"
patch -p1 -R < "/root/machinex/patches/0100-sysfs-kernfs-introduce-kernfs-_find_and-_get.patch"
patch -p1 -R < "/root/machinex/patches/0099-sysfs-kernfs-prepare-open-release-poll-paths.patch"
patch -p1 -R < "/root/machinex/patches/0098-sysfs-kernfs-prepare-mmap-path-for-kernfs.patch"
patch -p1 -R < "/root/machinex/patches/0097-sysfs-kernfs-prepare-write-path-for-kernfs.patch"
patch -p1 -R < "/root/machinex/patches/0096-sysfs-kernfs-prepare-read-path-for-kernfs.patch"
patch -p1 -R < "/root/machinex/patches/0095-sysfs-kernfs-introduce-kernfs_create_dir-_ns.patch"
patch -p1 -R < "/root/machinex/patches/0094-sysfs-kernfs-replace-sysfs_dirent-s_dir.kobj.patch"
patch -p1 -R < "/root/machinex/patches/0093-sysfs-kernfs-introduce-kernfs_setattr.patch"
patch -p1 -R < "/root/machinex/patches/0092-sysfs-kernfs-introduce-kernfs_rename-_ns.patch"
patch -p1 -R < "/root/machinex/patches/0091-sysfs-kernfs-introduce-kernfs_create_link.patch"
patch -p1 -R < "/root/machinex/patches/0090-sysfs-kernfs-introduce-kernfs_remove-_by_name-_ns.patch"
patch -p1 -R < "/root/machinex/patches/0089-sysfs-make-__sysfs_add_one-fail-if-the-parent-isn-t-.patch"
patch -p1 -R < "/root/machinex/patches/0088-sysfs-drop-kobj_ns_type-handling-take-2.patch"
patch -p1 -R < "/root/machinex/patches/0087-revert.patch"
patch -p1 -R < "/root/machinex/patches/0086-sysfs-use-a-separate-locking-class-for-open-files.patch"
patch -p1 -R < "/root/machinex/patches/0085-sysfs-handle-duplicate-removal-attempts.patch"
patch -p1 -R < "/root/machinex/patches/0084-Revert-sysfs-drop-kobj_ns_type-handling.patch"
patch -p1 -R < "/root/machinex/patches/0083-sysfs-rename-sysfs_assoc_lock-and-explain.patch"
patch -p1 -R < "/root/machinex/patches/0082-sysfs-use-generic_file_llseek-for-sysfs_file_ops.patch"
patch -p1 -R < "/root/machinex/patches/0081-sysfs-return-correct-error-code-on-unimplemented-mma.patch"
patch -p1 -R < "/root/machinex/patches/0080-sysfs-separate-out-dup-filename-warning.patch"
patch -p1 -R < "/root/machinex/patches/0079-sysfs-move-sysfs_hash_and_remove-to-fs-sysfs-dir.c.patch"
patch -p1 -R < "/root/machinex/patches/0078-sysfs-remove-unused-sysfs_get_dentry-prototype.patch"
patch -p1 -R < "/root/machinex/patches/0077-sysfs-honor-bin_attr.attr.ignore_lockdep.patch"
patch -p1 -R < "/root/machinex/patches/0076-sysfs-merge-sysfs_elem_bin_attr-into-sysfs_elem_attr.patch"
patch -p1 -R < "/root/machinex/patches/0075-sysfs-fix-sysfs_write_file-for-bin-file.patch"
patch -p1 -R < "/root/machinex/patches/0074-sysfs-bin-Fix-size-handling-overflow-for-bin_attr.patch"
patch -p1 -R < "/root/machinex/patches/0073-sysfs-make-sysfs_file_ops-follow-ignore_lockdep-flag.patch"
patch -p1 -R < "/root/machinex/patches/0072-ysfs-merge-regular-and-bin-file-handling.patch"
patch -p1 -R < "/root/machinex/patches/0071-sysfs-prepare-open-path-for-unified-regular-bin-file.patch"
patch -p1 -R < "/root/machinex/patches/0070-sysfs-copy-bin-mmap-support-from-fs-sysfs-bin.c.patch"
patch -p1 -R < "/root/machinex/patches/0069-sysfs-add-sysfs_bin_read.patch"
patch -p1 -R < "/root/machinex/patches/0068-sysfs-prepare-path-write-for-unified.patch"
patch -p1 -R < "/root/machinex/patches/0067-sysfs-collapse-fs-sysfs-bin.c-fill_read-into-read.patch"
patch -p1 -R < "/root/machinex/patches/0066-sysfs-skip-bin_buffer-buffer-while-reading.patch"
patch -p1 -R < "/root/machinex/patches/0065-sysfs-use-seq_file-when-reading-regular-files.patch"
patch -p1 -R < "/root/machinex/patches/0064-sysfs-use-transient-write-buffer.patch"
patch -p1 -R < "/root/machinex/patches/0063-sysfs-add-sysfs_open_file-sd-and-file.patch"
patch -p1 -R < "/root/machinex/patches/0062-sysfs-rename-sysfs_buffer-to-sysfs_open_file.patch"
patch -p1 -R < "/root/machinex/patches/0061-sysfs-remove-sysfs_buffer-ops.patch"
patch -p1 -R < "/root/machinex/patches/0060-sysfs-remove-sysfs_buffer-needs_read_fill.patch"
patch -p1 -R < "/root/machinex/patches/0059-sysfs-remove-unused-sysfs_buffer-pos.patch"
patch -p1 -R < "/root/machinex/patches/0058-sysfs-introduce-__-sysfs_remove.patch"
patch -p1 -R < "/root/machinex/patches/0057-sysfs-make-__sysfs_remove_dir-recursive.patch"
patch -p1 -R < "/root/machinex/patches/0056-kobject-grab-an-extra-reference-on-kobject-sd-to-all.patch"
patch -p1 -R < "/root/machinex/patches/0055-sysfs-remove-sysfs_addrm_cxt-parent_sd.patch"
patch -p1 -R < "/root/machinex/patches/0054-sysfs-name-comes-before-ns.patch"
patch -p1 -R < "/root/machinex/patches/0053-sysfs-clean-up-sysfs_get_dirent.patch"
patch -p1 -R < "/root/machinex/patches/0052-sysfs-drop-kobj_ns_type-handling.patch"
patch -p1 -R < "/root/machinex/patches/0051-sysfs-remove-ktype-namespace-invocations-in-symlink.patch"
patch -p1 -R < "/root/machinex/patches/0050-sysfs-remove-ktype-namespace-invocations-in-director.patch"
patch -p1 -R < "/root/machinex/patches/0049-sysfs-make-attr-namespace-interface-less-convoluted.patch"
patch -p1 -R < "/root/machinex/patches/0048-sysfs-use-check_submounts_and_drop.patch"
patch -p1 -R < "/root/machinex/patches/0047-sysfs-Restrict-mounting-sysfs.patch"
patch -p1 -R < "/root/machinex/patches/0046-coding-style-and-userns-Better-restrictions-on-when-.patch"