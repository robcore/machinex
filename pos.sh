#!/bin/bash


patch -p1 -R < "/root/machinex/patches/0019-mm-softdirty-enable-write-notifications-on-VMAs-afte.patch"
patch -p1 -R < "/root/machinex/patches/0018-mm-remove-misleading-ARCH_USES_NUMA_PROT_NONE.patch"
patch -p1 -R < "/root/machinex/patches/0017-mm-use-paravirt-friendly-ops-for-NUMA-hinting-ptes.patch"
patch -p1 -R < "/root/machinex/patches/0016-mm-use-ptep-pmdp_set_numa-for-updating-PAGE_NUMA.patch"
patch -p1 -R < "/root/machinex/patches/0015-mm-thp-move-ptl-taking-inside-page_check_address_pmd.patch"
patch -p1 -R < "/root/machinex/patches/0014-mm-thp-change-pmd_trans_huge_lock-to-return-taken-lo.patch"
patch -p1 -R < "/root/machinex/patches/0013-introduce-pte-lock.patch"
patch -p1 -R < "/root/machinex/patches/0012-enhance-swap-readahead.patch"
patch -p1 -R < "/root/machinex/patches/0011-update-mprotect.patch"
patch -p1 -R < "/root/machinex/patches/0010-mm-swap.c-clean-up-lru_cache_add-functions.patch"
patch -p1 -R < "/root/machinex/patches/0009-mm-swap.c-introduce-put_-un-refcounted_compound_page.patch"
patch -p1 -R < "/root/machinex/patches/0008-mm-numa-recheck-for-trranshuge-pages-under-lock.patch"