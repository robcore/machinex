#ifndef VM_EVENT_ITEM_H_INCLUDED
#define VM_EVENT_ITEM_H_INCLUDED

#ifdef CONFIG_ZONE_DMA
#define DMA_ZONE(xx) xx##_DMA,
#else
#define DMA_ZONE(xx)
#endif

#ifdef CONFIG_ZONE_DMA32
#define DMA32_ZONE(xx) xx##_DMA32,
#else
#define DMA32_ZONE(xx)
#endif

#ifdef CONFIG_HIGHMEM
#define HIGHMEM_ZONE(xx) , xx##_HIGH
#else
#define HIGHMEM_ZONE(xx)
#endif

#define FOR_ALL_ZONES(xx) DMA_ZONE(xx) DMA32_ZONE(xx) xx##_NORMAL HIGHMEM_ZONE(xx) , xx##_MOVABLE

enum vm_event_item { PGPGIN, PGPGOUT, PSWPIN, PSWPOUT,
		FOR_ALL_ZONES(PGALLOC),
		PGFREE, PGACTIVATE, PGDEACTIVATE,
		PGFAULT, PGMAJFAULT,
		FOR_ALL_ZONES(PGREFILL),
		FOR_ALL_ZONES(PGSTEAL_KSWAPD),
		FOR_ALL_ZONES(PGSTEAL_DIRECT),
		FOR_ALL_ZONES(PGSCAN_KSWAPD),
		FOR_ALL_ZONES(PGSCAN_DIRECT),
		PGSCAN_DIRECT_THROTTLE,
#ifdef CONFIG_NUMA
		PGSCAN_ZONE_RECLAIM_FAILED,
#endif
		PGINODESTEAL, SLABS_SCANNED, KSWAPD_INODESTEAL,
		KSWAPD_LOW_WMARK_HIT_QUICKLY, KSWAPD_HIGH_WMARK_HIT_QUICKLY,
		PAGEOUTRUN, ALLOCSTALL, PGROTATED,
#ifdef CONFIG_NUMA_BALANCING
		NUMA_PTE_UPDATES,
		NUMA_HINT_FAULTS,
		NUMA_HINT_FAULTS_LOCAL,
		NUMA_PAGE_MIGRATE,
#endif
#ifdef CONFIG_MIGRATION
		PGMIGRATE_SUCCESS, PGMIGRATE_FAIL,
#endif
#ifdef CONFIG_COMPACTION
		COMPACTMIGRATE_SCANNED, COMPACTFREE_SCANNED,
		COMPACTISOLATED,
		COMPACTSTALL, COMPACTFAIL, COMPACTSUCCESS,
#endif
#ifdef CONFIG_HUGETLB_PAGE
		HTLB_BUDDY_PGALLOC, HTLB_BUDDY_PGALLOC_FAIL,
#endif
		UNEVICTABLE_PGCULLED,	/* culled to noreclaim list */
		UNEVICTABLE_PGSCANNED,	/* scanned for reclaimability */
		UNEVICTABLE_PGRESCUED,	/* rescued from noreclaim list */
		UNEVICTABLE_PGMLOCKED,
		UNEVICTABLE_PGMUNLOCKED,
		UNEVICTABLE_PGCLEARED,	/* on COW, page truncate */
		UNEVICTABLE_PGSTRANDED,	/* unable to isolate on unlock */
		UNEVICTABLE_MLOCKFREED,
#ifdef CONFIG_TRANSPARENT_HUGEPAGE
		THP_FAULT_ALLOC,
		THP_FAULT_FALLBACK,
		THP_COLLAPSE_ALLOC,
		THP_COLLAPSE_ALLOC_FAILED,
		THP_SPLIT,
		THP_ZERO_PAGE_ALLOC,
		THP_ZERO_PAGE_ALLOC_FAILED,
#endif
		NR_TLB_REMOTE_FLUSH,	/* cpu tried to flush others' tlbs */
		NR_TLB_REMOTE_FLUSH_RECEIVED,/* cpu received ipi for flush */
		NR_TLB_LOCAL_FLUSH_ALL,
		NR_TLB_LOCAL_FLUSH_ONE,
		NR_TLB_LOCAL_FLUSH_ONE_KERNEL,
		NR_VM_EVENT_ITEMS
};

#endif		/* VM_EVENT_ITEM_H_INCLUDED */
