#!/bin/bash
patch -p1 -R < "/root/machinex/patches/0006-this-is-more-work-than-i-signed-up-for.patch"
patch -p1 -R < "/root/machinex/patches/0005-hacked-around-some-const-ipation.patch"
patch -p1 -R < "/root/machinex/patches/0004-throw-procinfo-into-mmu.patch"
patch -p1 -R < "/root/machinex/patches/0003-dont-need-const-in-memblock.patch"
patch -p1 -R < "/root/machinex/patches/0002-ARM-mm-Recreate-kernel-mappings-in-early_paging_init.patch"
patch -p1 -R < "/root/machinex/patches/0001-constify-machinex-description.patch"
