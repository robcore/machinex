#!/bin/bash

patch -p1 -R < "/root/machinex/patches/0052-fuse-send-poll-events.patch"
patch -p1 -R < "/root/machinex/patches/0051-fuse-avoid-out-of-scope-stack-access.patch"
patch -p1 -R < "/root/machinex/patches/0050-fuse-cleanup-fuse_direct_io.patch"
patch -p1 -R < "/root/machinex/patches/0049-fuse-optimize-__fuse_direct_io.patch"
patch -p1 -R < "/root/machinex/patches/0048-fuse-optimize-fuse_get_user_pages.patch"
patch -p1 -R < "/root/machinex/patches/0047-fuse-pass-iov-to-fuse_get_user_pages.patch"