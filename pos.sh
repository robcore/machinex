#!/bin/bash
patch -p1 -R < "/root/machinex/patches/0014-fixup-pm_qos_work_fn-and-its-relationship-with-_pm_q.patch"
patch -p1 -R < "/root/machinex/patches/0013-smp.h-fix-sparse-warnings.patch"
patch -p1 -R < "/root/machinex/patches/0012-mm-fix-page-leak-at-nfs_symlink.patch"
patch -p1 -R < "/root/machinex/patches/0011-drivers-message-i2o-i2o_config.c-fix-deadlock.patch"
patch -p1 -R < "/root/machinex/patches/0010-PM-QoS-Add-type-to-dev_pm_qos_add_ancestor_request.patch"
patch -p1 -R < "/root/machinex/patches/0009-PM-QoS-Introcuce-latency-tolerance-device-PM-QoS.patch"
patch -p1 -R < "/root/machinex/patches/0008-PM-QoS-Add-no_constraints_value-field.patch"
patch -p1 -R < "/root/machinex/patches/0007-PM-QoS-Rename-device-resume-latency-QoS-items.patch"
patch -p1 -R < "/root/machinex/patches/0006-PM-QoS-Fix-workqueue-deadlock.patch"
patch -p1 -R < "/root/machinex/patches/0005-dox.patch"
patch -p1 -R < "/root/machinex/patches/0004-irqdomain-minor-printing-checkpatch-error.patch"
patch -p1 -R < "/root/machinex/patches/0003-PM-devfreq-Fix-compiler-warnings.patch"
patch -p1 -R < "/root/machinex/patches/0002-PM-QoS-Avoid-possible-deadlock-related-to-sysfs.patch"
patch -p1 -R < "/root/machinex/patches/0001-PM-QoS-Add-return-code-to-pm_qos_get_value-function.patch"
