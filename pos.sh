#!/bin/bash
patch -p1 -R < "/root/machinex/patches/0028-sysfs-name-comes-before-ns.patch"
patch -p1 -R < "/root/machinex/patches/0027-sysfs-clean-up-sysfs_get_dirent.patch"
patch -p1 -R < "/root/machinex/patches/0026-sysfs-drop-kobj_ns_type-handling.patch"
patch -p1 -R < "/root/machinex/patches/0025-sysfs-remove-ktype-namespace-invocations-in-symlink.patch"
patch -p1 -R < "/root/machinex/patches/0024-sysfs-remove-ktype-namespace-invocations-in-director.patch"
patch -p1 -R < "/root/machinex/patches/0023-sysfs-make-attr-namespace-interface-less-convoluted.patch"
patch -p1 -R < "/root/machinex/patches/0022-sysfs-use-check_submounts_and_drop.patch"
patch -p1 -R < "/root/machinex/patches/0021-sysfs-Restrict-mounting-sysfs.patch"
patch -p1 -R < "/root/machinex/patches/0020-userns-Better-restrictions-on-when-proc-and-sysfs-ca.patch"