#!/bin/bash

patch -p1 -R < "/root/machinex/patches/0199-while-we-are-at-it-fuck-debug-too.patch"
patch -p1 -R < "/root/machinex/patches/0198-i-am-a-fucking-idiot-we-didnt-even-need-those-option.patch"
patch -p1 -R < "/root/machinex/patches/0197-revert-a-couple-ext4-experiments-too.patch"
patch -p1 -R < "/root/machinex/patches/0196-fuck-it-dont-build-then.patch"
patch -p1 -R < "/root/machinex/patches/0195-dang-hjad-to-revert-the-kickass-sb-start-write-commi.patch"
patch -p1 -R < "/root/machinex/patches/0193-fix-a-leak-in-replace_fd-users.patch"
patch -p1 -R < "/root/machinex/patches/0189-super.c-unused-variable-warning-without-CONFIG_QUOTA.patch"
patch -p1 -R < "/root/machinex/patches/0188-ext4-set-bg_itable_unused-when-resizing.patch"
patch -p1 -R < "/root/machinex/patches/0187-ext4-fix-double-unlock-buffer-mess-during-fs-resize.patch"
patch -p1 -R < "/root/machinex/patches/0186-ext4-use-proper-csum-calculation-in-ext4_rename.patch"
patch -p1 -R < "/root/machinex/patches/0185-ext4-fix-mtime-update-in-nodelalloc-mode.patch"
patch -p1 -R < "/root/machinex/patches/0184-s-Protect-write-paths-by-sb_start_write-sb_end_write.patch"
patch -p1 -R < "/root/machinex/patches/0183-ext4-honor-O_-D-SYNC-semantic-in-ext4_fallocate.patch"
patch -p1 -R < "/root/machinex/patches/0182-ext4-add-a-new-nolock-flag-in-ext4_map_blocks.patch"
patch -p1 -R < "/root/machinex/patches/0181-dox.patch"
patch -p1 -R < "/root/machinex/patches/0180-ext4-Convert-to-new-freezing-mechanism.patch"
patch -p1 -R < "/root/machinex/patches/0179-net-netprio-fd-passed-in-SCM_RIGHTS.patch"
patch -p1 -R < "/root/machinex/patches/0178-word-at-a-time-simplify-big-endian-zero_bytemask.patch"
patch -p1 -R < "/root/machinex/patches/0177-word-at-a-time-avoid-undefined-behaviour.patch"
patch -p1 -R < "/root/machinex/patches/0176-word-at-a-time-provide-generic-big-endian-zero_bytem.patch"
patch -p1 -R < "/root/machinex/patches/0175-vfs-switch-i_dentry-d_alias-to-hlist.patch"
patch -p1 -R < "/root/machinex/patches/0174-reverted.patch"
patch -p1 -R < "/root/machinex/patches/0173-man-i-dont-know-what-to-do-with-this.patch"
patch -p1 -R < "/root/machinex/patches/0172-time-for-this-old-man-to-to-do-some-hefty-block-work.patch"
patch -p1 -R < "/root/machinex/patches/0171-block-add-q-nr_rqs-and-move-q-rq.elvpriv-to-q-nr_rqs.patch"
patch -p1 -R < "/root/machinex/patches/0170-block-refactor-get_request-_wait.patch"
patch -p1 -R < "/root/machinex/patches/0169-block-drop-custom-queue-draining-used-by-scsi_transp.patch"
patch -p1 -R < "/root/machinex/patches/0168-mempool-add-gfp_mask-to-mempool_create_node.patch"
patch -p1 -R < "/root/machinex/patches/0167-dont-think-highmem-is-great-for-zswap-yet.patch"
patch -p1 -R < "/root/machinex/patches/0166-fuck-everyone-i-am-maxing-out-boot-with-performance-.patch"
patch -p1 -R < "/root/machinex/patches/0165-fixup-zswap.patch"
patch -p1 -R < "/root/machinex/patches/0164-revert-those-and-am-gonna-adjust-my-ramdisk-i-do-not.patch"
patch -p1 -R < "/root/machinex/patches/0163-missed-one.patch"
patch -p1 -R < "/root/machinex/patches/0162-a-little-more-dhd-hacking.patch"
patch -p1 -R < "/root/machinex/patches/0161-more-dhd-hacking-cmon-current-sammy-devices-save-the.patch"
patch -p1 -R < "/root/machinex/patches/0160-bcm-hacking.patch"
patch -p1 -R < "/root/machinex/patches/0159-bcmdhd-return-custom-sec-values-to-original-for.reas.patch"
patch -p1 -R < "/root/machinex/patches/0158-my-warnings-like-these-goggles-do-nothing.patch"
patch -p1 -R < "/root/machinex/patches/0157-holy-shit-hacking-worked.to-build-at-least.patch"
patch -p1 -R < "/root/machinex/patches/0156-revertsky.patch"
patch -p1 -R < "/root/machinex/patches/0155-reverto.patch"
patch -p1 -R < "/root/machinex/patches/0154-lets-see-what-happens-if-i-enable-blk-cgroup.patch"
patch -p1 -R < "/root/machinex/patches/0153-fixed-up-my-warn-onces-and-simplified-more-file-read.patch"
patch -p1 -R < "/root/machinex/patches/0152-before-i-build-this-probably-smarter-idea-to-warn-on.patch"

