#!/bin/bash
patch -p1 -R < "/root/machinex/patches/0016-went-too-far.patch"
patch -p1 -R < "/root/machinex/patches/0015-possed.patch"
patch -p1 -R < "/root/machinex/patches/0014-forget-the-tty-io-one-for-now.patch"
patch -p1 -R < "/root/machinex/patches/0013-need-to-define-superblock-there.patch"
patch -p1 -R < "/root/machinex/patches/0012-revert-those-two.patch"
patch -p1 -R < "/root/machinex/patches/0011-ummmm-i-think-this-is-deprecated.patch"
patch -p1 -R < "/root/machinex/patches/0010-ext4-use-percpu-counter-for-extent-cache-count.patch"
patch -p1 -R < "/root/machinex/patches/0009-eCryptfs-allow-userspace-messaging-to-be-disabled.patch"
patch -p1 -R < "/root/machinex/patches/0008-ext4-super-convert-number-of-blocks-to-clusters-prop.patch"
patch -p1 -R < "/root/machinex/patches/0007-ext4-fix-possible-memory-leak-in-ext4_remount.patch"
patch -p1 -R < "/root/machinex/patches/0006-fat-mark-fs-as-dirty-on-mount-and-clean-on-umount.patch"
patch -p1 -R < "/root/machinex/patches/0005-some-file-inode-open-coded-conversions.patch"
patch -p1 -R < "/root/machinex/patches/0004-eCryptfs-Fix-redundant-error-check-on-ecryptfs_find.patch"


