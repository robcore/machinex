#!/bin/bash
patch -p1 -R < "/root/machinex/patches/0070-hoooold-up.patch
patch -p1 -R < "/root/machinex/patches/0069-fuck-it-all-that-did-was-break-the-compiler.patch
patch -p1 -R < "/root/machinex/patches/0067-some-more-syncing-from-4.7.patch
patch -p1 -R < "/root/machinex/patches/0066-some-fixes-in-locking.patch
patch -p1 -R < "/root/machinex/patches/0061-hrtimer-Remove-redundant-ifdef-block.patch
patch -p1 -R < "/root/machinex/patches/0060-dox.patch
patch -p1 -R < "/root/machinex/patches/0059-timerfd-Reject-ALARM-timerfds-without-CAP_WAKE_ALARM.patch
patch -p1 -R < "/root/machinex/patches/0058-revert-that.patch
patch -p1 -R < "/root/machinex/patches/0057-futex-Calculate-the-futex-key-based-on-a-tail-page-f.patch
patch -p1 -R < "/root/machinex/patches/0056-locking-qspinlock-Use-atomic_sub_return_release.patch
patch -p1 -R < "/root/machinex/patches/0055-locking-mutex-Optimize-mutex_trylock-fast-path.patch
patch -p1 -R < "/root/machinex/patches/0054-locking-rwsem-Streamline-the-rwsem_optimistic_spin-c.patch
patch -p1 -R < "/root/machinex/patches/0053-locking-rwsem-Improve-reader-wakeup-code.patch
patch -p1 -R < "/root/machinex/patches/0052-locking-rwsem-Protect-all-writes-to-owner-by-WRITE_O.patch
patch -p1 -R < "/root/machinex/patches/0051-locking-rwsem-Add-reader-owned-state-to-the-owner-fi.patch
patch -p1 -R < "/root/machinex/patches/0050-locking-rwsem-Remove-rwsem_atomic_add-and-rwsem_atom.patch
patch -p1 -R < "/root/machinex/patches/0049-locking-rwsem-Convert-sem-count-to-atomic_long_t.patch
patch -p1 -R < "/root/machinex/patches/0048-locking-qspinlock-Add-comments.patch
patch -p1 -R < "/root/machinex/patches/0047-locking-qspinlock-Clarify-xchg_tail-ordering.patch
patch -p1 -R < "/root/machinex/patches/0046-locking-barriers-Validate-lockless_dereference-is-us.patch