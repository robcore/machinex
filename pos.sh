#!/bin/bash
patch -p1 -R < "/root/machinex/patches/0007-fuck-me.patch"
patch -p1 -R < "/root/machinex/patches/0006-ext4_init_new_dir.patch"
patch -p1 -R < "/root/machinex/patches/0005-more.patch"
patch -p1 -R < "/root/machinex/patches/0004-fix-my-inode.patch"
patch -p1 -R < "/root/machinex/patches/0003-more-shit.patch"
patch -p1 -R < "/root/machinex/patches/0002-more-.wedging-more-than-commiting.patch"
patch -p1 -R < "/root/machinex/patches/0001-i-like-to-fix-and-break-stuff-at-the-same-time.patch"
