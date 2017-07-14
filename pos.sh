#!/bin/bash
patch -p1 -R < "/root/machinex/patches/0021-do-some-fixup-work-for-touch-wake.patch"
patch -p1 -R < "/root/machinex/patches/0020-fix-another-ifdef.patch"
patch -p1 -R < "/root/machinex/patches/0019-fix-up-some-ifdefs-around-some-kt-wake-func-stuff.patch"
patch -p1 -R < "/root/machinex/patches/0018-fix-an-include-for-our-source-in-the-touch-wake-head.patch"
patch -p1 -R < "/root/machinex/patches/0017-removed.patch"
patch -p1 -R < "/root/machinex/patches/0016-yet-another-interesting-idea-touch-wake-by-yank555_l.patch"