#!/bin/bash
patch -p1 -R < "/root/machinex/patches/0018-ohhhh-shit.patch"
patch -p1 -R < "/root/machinex/patches/0017-idr-reorder-the-fields.patch"
patch -p1 -R < "/root/machinex/patches/0016-idr-reduce-the-unneeded-check-in-free_layer.patch"
patch -p1 -R < "/root/machinex/patches/0015-idr-don-t-need-to-shink-the-free-list-when-idr_remov.patch"
patch -p1 -R < "/root/machinex/patches/0014-idr-fix-idr_replace-s-returned-error-code.patch"
patch -p1 -R < "/root/machinex/patches/0013-idr-fix-NULL-pointer-dereference-when-ida_remove.patch"
patch -p1 -R < "/root/machinex/patches/0012-idr-fix-unexpected-ID-removal-when-idr_remove-unallo.patch"
patch -p1 -R < "/root/machinex/patches/0011-idr-fix-overflow-bug-during-maximum-ID-calculation.patch"
patch -p1 -R < "/root/machinex/patches/0010-dox.patch"
patch -p1 -R < "/root/machinex/patches/0009-lib-idr.c-fix-out-of-bounds-pointer-dereference.patch"
patch -p1 -R < "/root/machinex/patches/0008-idr-explain-WARN_ON_ONCE-on-negative-IDs-out-of-rang.patch"
patch -p1 -R < "/root/machinex/patches/0007-idr-implement-lookup-hint.patch"
patch -p1 -R < "/root/machinex/patches/0006-idr-add-idr_layer-prefix.patch"
patch -p1 -R < "/root/machinex/patches/0005-idr-make-idr_layer-larger.patch"
patch -p1 -R < "/root/machinex/patches/0004-idr-remove-length-restriction-from-idr_layer-bitmap.patch"
patch -p1 -R < "/root/machinex/patches/0003-idr-remove-MAX_IDR_MASK-and-move-left-MAX_IDR_-into-.patch"
patch -p1 -R < "/root/machinex/patches/0002-idr-fix-top-layer-handling.patch"
patch -p1 -R < "/root/machinex/patches/0001-idr-implement-idr_preload-_end-and-idr_alloc.patch"
