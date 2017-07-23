#!/bin/bash

patch -p1 < "/root/machinex/patches/0031-ecryptfs-fix-32-bit-corruption.patch"
patch -p1 < "/root/machinex/patches/0032-ecryptfs-check-DCACHE_OP_REVALIDATE-instead-of-d_op.patch"
patch -p1 < "/root/machinex/patches/0033-ecryptfs-don-t-leave-RCU-pathwalk-immediately.patch"
patch -p1 < "/root/machinex/patches/0034-ecryptfs-get-rid-of-ecryptfs_set_dentry_lower-_mnt.patch"
patch -p1 < "/root/machinex/patches/0035-ecryptfs-lower_path.dentry-is-never-NULL.patch"
patch -p1 < "/root/machinex/patches/0036-libfs-get-exports-to-definitions-of-objects-being-ex.patch"
patch -p1 < "/root/machinex/patches/0037-new-helper-kfree_put_link.patch"
patch -p1 < "/root/machinex/patches/0038-bs.patch"