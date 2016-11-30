#!/bin/bash

patch -p1 -R < "/root/machinex/patches/0100-k-reverted-all-the-fuckin-smp-stuff-and-kept-dox-per.patch"
patch -p1 -R < "/root/machinex/patches/0099-there.-please-build.-please-boot.-please-rule.patch"
patch -p1 -R < "/root/machinex/patches/0098-revert-that-data-to-cfd-conversion.patch"
patch -p1 -R < "/root/machinex/patches/0097-MSM-convert-to-SMP-Operations.patch"
patch -p1 -R < "/root/machinex/patches/0096-dox.patch"
patch -p1 -R < "/root/machinex/patches/0095-dox.patch"
patch -p1 -R < "/root/machinex/patches/0094-perf.patch"
patch -p1 -R < "/root/machinex/patches/0093-nother-small-smp-update.patch"
patch -p1 -R < "/root/machinex/patches/0092-arm-mm-asid-and-bitmap-ops-update.patch"
patch -p1 -R < "/root/machinex/patches/0091-qup-i2c-reverted-another-stupid-fucking-patch.patch"
patch -p1 -R < "/root/machinex/patches/0090-speaking-of-which-a-little-cleanup.patch"
patch -p1 -R < "/root/machinex/patches/0089-a-SHIT-ton-of-SMP-bringup-as-well-as-reverting-a-shi.patch"
