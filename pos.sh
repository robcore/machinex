#!/bin/bash
patch -p1 -R < "/root/machinex/patches/0009-gonna-revert-this-one.patch"
patch -p1 -R < "/root/machinex/patches/0008-copy-address-of-proc_ns_ops-into-ns_common.patch"
patch -p1 -R < "/root/machinex/patches/0007-new-helpers-ns_alloc_inum-ns_free_inum.patch"
patch -p1 -R < "/root/machinex/patches/0006-and-others.patch"
patch -p1 -R < "/root/machinex/patches/0005-netns-switch-get-put-install-inum.patch"
patch -p1 -R < "/root/machinex/patches/0004-make-mntns-get-put-install-inum-work-with-mnt_ns.patch"
patch -p1 -R < "/root/machinex/patches/0003-common-object-embedded-into-various-struct-.ns.patch"