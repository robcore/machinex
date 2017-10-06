#!/bin/bash
patch -p1 -R < "/root/machinex/patches/0007-i-need-to-revert-all-of-this-for-now-except-for-the-.patch"
patch -p1 -R < "/root/machinex/patches/0006-more-of-the-hacking.patch"
patch -p1 -R < "/root/machinex/patches/0005-some-others.patch"
patch -p1 -R < "/root/machinex/patches/0004-netns-switch-get-put-install-inum.patch"
patch -p1 -R < "/root/machinex/patches/0003-make-mntns-get-put-install-inum-work-with-mnt_ns-ns.patch"
patch -p1 -R < "/root/machinex/patches/0002-half-ass-move-proc-inum-into-ns-common.patch"