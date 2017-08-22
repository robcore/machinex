#!/bin/bash
patch -p1 -R < "/root/machinex/patches/0008-and-more.patch"
patch -p1 -R < "/root/machinex/patches/0007-more.patch"
patch -p1 -R < "/root/machinex/patches/0006-kuid-bringup.patch"
patch -p1 -R < "/root/machinex/patches/0005-Remove-user-triggerable-BUG-from-mpol_to_str.patch"
patch -p1 -R < "/root/machinex/patches/0004-reveretd-it-alll.patch"
patch -p1 -R < "/root/machinex/patches/0003-fuck-what-am-i-doing.patch"
patch -p1 -R < "/root/machinex/patches/0002-change-up-proportions-and-page-writeback.patch"
patch -p1 -R < "/root/machinex/patches/0001-starting-to-upstream-backing-dev.patch"
