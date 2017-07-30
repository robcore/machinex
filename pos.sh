#!/bin/bash
patch -p1 -R < "/root/machinex/patches/0205-bandaid-fix-TODO-find-new_sync_write.patch"
patch -p1 -R < "/root/machinex/patches/0204-optimize-copy_page_-to-from-_iter.patch"
patch -p1 -R < "/root/machinex/patches/0203-new-helper-copy_page_from_iter.patch"
patch -p1 -R < "/root/machinex/patches/0202-kill-generic_segment_checks.patch"
patch -p1 -R < "/root/machinex/patches/0201-generic_file_direct_write-switch-to-iov_iter.patch"
patch -p1 -R < "/root/machinex/patches/0200-kill-iov_iter_copy_from_user.patch"