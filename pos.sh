#!/bin/bash

patch -p1 -R < "/root/machinex/patches/0064-idr-implement-idr_preload-_end-and-idr_alloc.patch"
patch -p1 -R < "/root/machinex/patches/0063-idr-refactor-idr_get_new_above.patch"
patch -p1 -R < "/root/machinex/patches/0062-idr-remove-_idr_rc_to_errno-hack.patch"
patch -p1 -R < "/root/machinex/patches/0061-idr-relocate-idr_for_each_entry-and-reorganize-id-r-.patch"
patch -p1 -R < "/root/machinex/patches/0060-idr-cosmetic-updates-to-struct-initializer-defs.patch"
patch -p1 -R < "/root/machinex/patches/0059-remove-and-deprecate-instances-of-idr_remove_all.patch"
patch -p1 -R < "/root/machinex/patches/0058-idr-make-idr_destroy-imply-idr_remove_all.patch"
patch -p1 -R < "/root/machinex/patches/0057-idr-fix-a-subtle-bug-in-idr_get_next.patch"