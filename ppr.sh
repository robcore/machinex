#!/bin/bash
function ppr() {
	patch -p1 -R <
}

./2381-vfs-rename-end_writeback-to-clear_inode.patch
./2380-vfs-Move-waiting-for-inode-writebac.patch
./2379-REVERT-vfs-rename-end_writeback-to-clear_inode.patch
./2378-CPU-hotplug-cpusets-suspend-Don-t-modify-cpusets-dur.patch
./2377-writeback-Fix-some-comment-errors.patch
./2376-mm-page-writeback.c-local-functions-should-not-be-ex.patch
./2375-writeback-Avoid-iput-from-flusher-thread.patch
./2374-writeback-rename-end_writeback-to-clear_inode.patch
./2373-writeback-Refactor-writeback_single_inode.patch
./2372-writeback-Remove-wb-list_lock-from-writeback_single_.patch
./2371-writeback-initialize-global_dirty_limit.patch
./2364-misc-fs-sync.c-fix.patch
./2363-epoll-fix-use-after-free-in-eventpoll_release_file.patch
./2362-file-add-new-fd-helpers.patch