#!/bin/bash
patch -p1 -R < "/root/machinex/patches/0049-my-locks-need-work.patch"
patch -p1 -R < "/root/machinex/patches/0048-nevermind-current-owner-is-correct.patch"
patch -p1 -R < "/root/machinex/patches/0047-made-open-easier.patch"
patch -p1 -R < "/root/machinex/patches/0046-get-rid-of-__locks_copy_lock.patch"
patch -p1 -R < "/root/machinex/patches/0045-made-setown-a-void.patch"
patch -p1 -R < "/root/machinex/patches/0044-bs.patch"
patch -p1 -R < "/root/machinex/patches/0043-locks-remove-lock_may_read-and-lock_may_write.patch"
patch -p1 -R < "/root/machinex/patches/0042-lockd-rip-out-deferred-lock-handling-from-testlock-c.patch"
patch -p1 -R < "/root/machinex/patches/0041-locks-Copy-fl_lmops-information-for-conflock.patch"
patch -p1 -R < "/root/machinex/patches/0040-locks-New-ops-in-lock_manager_operations-for-get-put.patch"
patch -p1 -R < "/root/machinex/patches/0039-rename-locks_copy_conflock.patch"
patch -p1 -R < "/root/machinex/patches/0038-locks-Remove-unused-conf-argument-from-lm_grant.patch"
patch -p1 -R < "/root/machinex/patches/0037-locks-pass-correct-before-pointer-to-locks_unlink.patch"