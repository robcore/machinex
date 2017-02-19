#!/bin/bash
patch -p1 -R < "/root/machinex/patches/0150-those-got-doubled.patch"
patch -p1 -R < "/root/machinex/patches/0149-a-bunch-of-freezing-start-file-write-bringup.patch"
patch -p1 -R < "/root/machinex/patches/0148-fs-Fix-hang-with-BSD-accounting-on-frozen-filesystem.patch"
patch -p1 -R < "/root/machinex/patches/0147-one-last-missing-slab-inclusion.patch"
patch -p1 -R < "/root/machinex/patches/0146-staging-speakup-fix-a-bug-when-translate-octal-num.patch"
patch -p1 -R < "/root/machinex/patches/0145-try-a-saner-locking-for-pde_opener.patch"
patch -p1 -R < "/root/machinex/patches/0144-deal-with-races-between-remove_proc_entry-and-proc.patch"
patch -p1 -R < "/root/machinex/patches/0143-procfs-preparations-for-remove_proc_entry-race-fixes.patch"
patch -p1 -R < "/root/machinex/patches/0142-rocfs-Clean-up-huge-if-statement-in-__proc_file.patch"
patch -p1 -R < "/root/machinex/patches/0141-bs.patch"
patch -p1 -R < "/root/machinex/patches/0140-proc-Kill-create_proc_entry.patch"
patch -p1 -R < "/root/machinex/patches/0139-constify-some-structs-yo.patch"
patch -p1 -R < "/root/machinex/patches/0138-x25-use-proc_remove_subtree.patch"
patch -p1 -R < "/root/machinex/patches/0137-procfs-new-helper-PDE_DATA-inode.patch"
patch -p1 -R < "/root/machinex/patches/0136-last_radio_log-switch-to-proc_create.patch"
patch -p1 -R < "/root/machinex/patches/0135-procfs-kill-write_proc.patch"
patch -p1 -R < "/root/machinex/patches/0134-swithc-shit-to-proc-create.patch"
patch -p1 -R < "/root/machinex/patches/0133-scsi_proc-make-proc_scsi_host_open-preallocate.patch"
patch -p1 -R < "/root/machinex/patches/0132-new-helper-single_open_size.patch"
patch -p1 -R < "/root/machinex/patches/0131-Switch-EVERYTHING-to-show-info.patch"
patch -p1 -R < "/root/machinex/patches/0130-cciss-switch-to-show_info.patch"
patch -p1 -R < "/root/machinex/patches/0129-scsi-saner-replacements-for-proc_info.patch"
patch -p1 -R < "/root/machinex/patches/0128-procfs-dont-allow-to-use-proc_create-create_proc_ent.patch"
patch -p1 -R < "/root/machinex/patches/0127-i2o-use-proc_remove_subtree.patch"
patch -p1 -R < "/root/machinex/patches/0126-reiserfs-use-proc_remove_subtree.patch"
patch -p1 -R < "/root/machinex/patches/0125-get-rid-of-a-bunch-of-open-coded-create_proc_read.patch"
patch -p1 -R < "/root/machinex/patches/0124-isp1362-hcd-don-t-reimplement-proc_create_data.patch"
patch -p1 -R < "/root/machinex/patches/0123-rtl8192u-don-t-play-with-reassigning-proc_fops.patch"
patch -p1 -R < "/root/machinex/patches/0122-remove-broken-rtl8192e.patch"
patch -p1 -R < "/root/machinex/patches/0121-rtl8192e-switch-to-proc_create.patch"
patch -p1 -R < "/root/machinex/patches/0120-rtl8192e-don-t-use-create_proc_entry-for-directories.patch"
patch -p1 -R < "/root/machinex/patches/0119-procfs-switch-proc-self-away-from-proc_dir_entry.patch"
patch -p1 -R < "/root/machinex/patches/0118-proc-self-whack-a-mole.patch"
patch -p1 -R < "/root/machinex/patches/0117-dox.patch"
patch -p1 -R < "/root/machinex/patches/0116-procfs-Don-t-cache-a-pid-in-the-root-inode.patch"
patch -p1 -R < "/root/machinex/patches/0115-procfs-Use-the-proc-generic-infrastructure-for-proc.patch"
patch -p1 -R < "/root/machinex/patches/0114-mode_t-whack-a-mole.patch"
patch -p1 -R < "/root/machinex/patches/0113-snd_info_register-switch-to-proc_create_data-proc_mk.patch"
patch -p1 -R < "/root/machinex/patches/0112-hysdn-stash-pointer-to-card-into-proc_dir_entry-data.patch"
patch -p1 -R < "/root/machinex/patches/0111-gadgetfs-don-t-bother-with-fops-owner.patch"
patch -p1 -R < "/root/machinex/patches/0110-get-rid-of-the-last-free_pipe_info-callers.patch"
patch -p1 -R < "/root/machinex/patches/0109-get-rid-of-alloc_pipe_info-argument.patch"
patch -p1 -R < "/root/machinex/patches/0108-get-rid-of-pipe-inode.patch"
patch -p1 -R < "/root/machinex/patches/0107-introduce-variants-of-pipe_lock-pipe_unlock-for-real.patch"
patch -p1 -R < "/root/machinex/patches/0106-pipe-set-file-private_data-to-i_pipe.patch"
patch -p1 -R < "/root/machinex/patches/0105-pipe-don-t-use-i_mutex.patch"
patch -p1 -R < "/root/machinex/patches/0104-pipe-take-allocation-and-freeing-of-pipe_inode_info.patch"
patch -p1 -R < "/root/machinex/patches/0103-pipe-preparation-to-new-locking-rules.patch"
patch -p1 -R < "/root/machinex/patches/0102-pipe-switch-wait_for_partner-and-wake_up_partner.patch"
patch -p1 -R < "/root/machinex/patches/0101-pipe-fold-file_operations-instances-in-one.patch"
patch -p1 -R < "/root/machinex/patches/0100-fold-fifo.c-into-pipe.patch"
patch -p1 -R < "/root/machinex/patches/0099-lift-sb_start_write-into-default_file_splice_write.patch"
patch -p1 -R < "/root/machinex/patches/0098-lift-sb_start_write-out-of-write.patch"
patch -p1 -R < "/root/machinex/patches/0097-switch-compat-readv-writev-variants-to-COMPAT_SYSCAL.patch"
patch -p1 -R < "/root/machinex/patches/0096-f2fs-use-mnt_want_write_file-in-ioctl.patch"
patch -p1 -R < "/root/machinex/patches/0095-lift-sb_start_write-sb_end_write-out-of-aio_write.patch"

