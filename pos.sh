#!/bin/bash

patch -p1 -R < "/root/machinex/patches/0012-dox.patch"
patch -p1 -R < "/root/machinex/patches/0011-stats.patch"
patch -p1 -R < "/root/machinex/patches/0010-stats.patch"
patch -p1 -R < "/root/machinex/patches/0009-percpu-pcpu-stats-change-void-buffer-to-int-buffer.patch"
patch -p1 -R < "/root/machinex/patches/0008-percpu-fix-static-checker-warnings-in-pcpu_destroy.patch"
patch -p1 -R < "/root/machinex/patches/0007-percpu-fix-early-calls-for-spinlock-in-pcpu_stats.patch"
patch -p1 -R < "/root/machinex/patches/0006-percpu-resolve-err-may-not-be-initialized.patch"
patch -p1 -R < "/root/machinex/patches/0005-tracing-yuck.patch"
patch -p1 -R < "/root/machinex/patches/0004-add-percpu-stats.patch"
patch -p1 -R < "/root/machinex/patches/0003-percpu-migrate-percpu-data-structures-to-internal-he.patch"
patch -p1 -R < "/root/machinex/patches/0002-percpu-add-missing-lockdep_assert_held.patch"
