#!/bin/bash
patch -p1 < "/root/machinex/patches/0023-export-unmap_kernel_range.patch"
patch -p1 < "/root/machinex/patches/0024-mm-vmalloc.c-clean-up-map_vm_area-third-argument.patch"
patch -p1 < "/root/machinex/patches/0025-vmalloc-emit-failure-message-before-return.patch"
patch -p1 < "/root/machinex/patches/0030-proportions-add-gfp-to-init-functions.patch"

patch -p1 < "/root/machinex/patches/0046-bring-back-prometheus-change-gonna-add-a-twist.patch"
patch -p1 < "/root/machinex/patches/0047-prometheus-3.6-only-use-the-queue-on-first-boot-then.patch"
patch -p1 < "/root/machinex/patches/0048-set_power_suspend_state-should-be-static.patch"
patch -p1 < "/root/machinex/patches/0049-be-more-informative-if-the-the-state-has-changed.patch"
