#!/bin/bash
patch -p1 -R < "/root/machinex/patches/0006-naw-this-was-a-bad-idea-we-need-it-all-for-now.patch"
patch -p1 -R < "/root/machinex/patches/0005-more.patch"
patch -p1 -R < "/root/machinex/patches/0004-dma-helper.patch"
patch -p1 -R < "/root/machinex/patches/0003-dox.patch"
patch -p1 -R < "/root/machinex/patches/0002-fix-overflow-of-ending-address-of-mem.patch"
patch -p1 -R < "/root/machinex/patches/0001-genalloc-add-devres-support.patch"
