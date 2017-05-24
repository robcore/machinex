#!/bin/bash
patch -p1 -R < "/root/machinex/patches/0009-nope-this-is-fucked.patch"
patch -p1 -R < "/root/machinex/patches/0008-temporary-fix-for-WALT.patch"
patch -p1 -R < "/root/machinex/patches/0007-backported-cpufreq-core-from-3.12-for-now.patch"
patch -p1 -R < "/root/machinex/patches/0006-REVERT-all-that-cpufreq-hacking-gonna-just-squash-3..patch"
patch -p1 -R < "/root/machinex/patches/0005-this-is-the-hackiest-ive-ever-hacked.patch"
patch -p1 -R < "/root/machinex/patches/0004-added-back-last-cpu.patch"
patch -p1 -R < "/root/machinex/patches/0003-reverting-acpuclock-changes-in-lieu-of-a-different-h.patch"
patch -p1 -R < "/root/machinex/patches/0002-brought-back-the-cpu-stuff-to-work-on.patch"
