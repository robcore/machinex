#!/bin/bash
patch -p1 -R < "/root/machinex/patches/0054-naw-fuck-this-for-tonight.patch"
patch -p1 -R < "/root/machinex/patches/0053-add-some-generic-numa-shit.patch"
patch -p1 -R < "/root/machinex/patches/0052-add-pmd-lock.patch"
patch -p1 -R < "/root/machinex/patches/0051-mm-factor-commit-limit-calculation.patch"
patch -p1 -R < "/root/machinex/patches/0050-bs.patch"
patch -p1 -R < "/root/machinex/patches/0049-bs.patch"
patch -p1 -R < "/root/machinex/patches/0048-memcg-kmem-use-is_root_cache-instead-of-hard-code.patch"
patch -p1 -R < "/root/machinex/patches/0047-mm-ensure-get_unmapped_area-returns-higher-address.patch"
patch -p1 -R < "/root/machinex/patches/0046-mm-__rmqueue_fallback-should-respect-pageblock-type.patch"
patch -p1 -R < "/root/machinex/patches/0045-mm-get-rid-of-unnecessary-overhead-of-trace_mm_page.patch"
patch -p1 -R < "/root/machinex/patches/0044-mm-fix-page_group_by_mobility_disabled-breakage.patch"
patch -p1 -R < "/root/machinex/patches/0043-mm-do-not-walk-all-of-system-memory-during-show_mem.patch"
patch -p1 -R < "/root/machinex/patches/0042-mm-clear-N_CPU-from-node_states-at-CPU-offline.patch"
patch -p1 -R < "/root/machinex/patches/0041-mem-hotplug-introduce-movable_node-boot-option.patch"
patch -p1 -R < "/root/machinex/patches/0040-mm-memblock.c-introduce-bottom-up-allocation-mode.patch"
patch -p1 -R < "/root/machinex/patches/0039-mmap-arch_get_unmapped_area-use-proper-mmap-base.patch"
patch -p1 -R < "/root/machinex/patches/0038-writeback-do-not-sync-data-dirtied-after-sync-start.patch"