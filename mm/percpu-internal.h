#ifndef _MM_PERCPU_INTERNAL_H
#define _MM_PERCPU_INTERNAL_H

#include <linux/types.h>
#include <linux/percpu.h>

struct pcpu_chunk {
#ifdef CONFIG_PERCPU_STATS
	int			nr_alloc;	/* # of allocations */
	size_t			max_alloc_size; /* largest allocation size */
#endif

	struct list_head	list;		/* linked to pcpu_slot lists */
	int			free_size;	/* free bytes in the chunk */
	int			contig_hint;	/* max contiguous size hint */
	void			*base_addr;	/* base address of this chunk */

	int			map_used;	/* # of map entries used before the sentry */
	int			map_alloc;	/* # of map entries allocated */
	int			*map;		/* allocation map */
	struct list_head	map_extend_list;/* on pcpu_map_extend_chunks */

	void			*data;		/* chunk data */
	int			first_free;	/* no free below this */
	bool			immutable;	/* no [de]population allowed */
	bool			has_reserved;	/* Indicates if chunk has reserved space
						   at the beginning. Reserved chunk will
						   contain reservation for static chunk.
						   Dynamic chunk will contain reservation
						   for static and reserved chunks. */
	int			start_offset;	/* the overlap with the previous
						   region to have a page aligned
						   base_addr */
	int			nr_populated;	/* # of populated pages */
	unsigned long		populated[];	/* populated bitmap */
};

extern spinlock_t pcpu_lock;

extern struct list_head *pcpu_slot;
extern int pcpu_nr_slots;
extern int pcpu_nr_empty_pop_pages;

extern struct pcpu_chunk *pcpu_first_chunk;
extern struct pcpu_chunk *pcpu_reserved_chunk;

#ifdef CONFIG_PERCPU_STATS

#include <linux/spinlock.h>

struct percpu_stats {
	u64 nr_alloc;		/* lifetime # of allocations */
	u64 nr_dealloc;		/* lifetime # of deallocations */
	u64 nr_cur_alloc;	/* current # of allocations */
	u64 nr_max_alloc;	/* max # of live allocations */
	u32 nr_chunks;		/* current # of live chunks */
	u32 nr_max_chunks;	/* max # of live chunks */
	size_t min_alloc_size;	/* min allocaiton size */
	size_t max_alloc_size;	/* max allocation size */
};

extern struct percpu_stats pcpu_stats;
extern struct pcpu_alloc_info pcpu_stats_ai;

/*
 * For debug purposes. We don't care about the flexible array.
 */
static inline void pcpu_stats_save_ai(const struct pcpu_alloc_info *ai)
{
	memcpy(&pcpu_stats_ai, ai, sizeof(struct pcpu_alloc_info));

	/* initialize min_alloc_size to unit_size */
	pcpu_stats.min_alloc_size = pcpu_stats_ai.unit_size;
}

/*
 * pcpu_stats_area_alloc - increment area allocation stats
 * @chunk: the location of the area being allocated
 * @size: size of area to allocate in bytes
 *
 * CONTEXT:
 * pcpu_lock.
 */
static inline void pcpu_stats_area_alloc(struct pcpu_chunk *chunk, size_t size)
{
	lockdep_assert_held(&pcpu_lock);

	pcpu_stats.nr_alloc++;
	pcpu_stats.nr_cur_alloc++;
	pcpu_stats.nr_max_alloc =
		max(pcpu_stats.nr_max_alloc, pcpu_stats.nr_cur_alloc);
	pcpu_stats.min_alloc_size =
		min(pcpu_stats.min_alloc_size, size);
	pcpu_stats.max_alloc_size =
		max(pcpu_stats.max_alloc_size, size);

	chunk->nr_alloc++;
	chunk->max_alloc_size = max(chunk->max_alloc_size, size);
}

/*
 * pcpu_stats_area_dealloc - decrement allocation stats
 * @chunk: the location of the area being deallocated
 *
 * CONTEXT:
 * pcpu_lock.
 */
static inline void pcpu_stats_area_dealloc(struct pcpu_chunk *chunk)
{
	lockdep_assert_held(&pcpu_lock);

	pcpu_stats.nr_dealloc++;
	pcpu_stats.nr_cur_alloc--;

	chunk->nr_alloc--;
}

/*
 * pcpu_stats_chunk_alloc - increment chunk stats
 */
static inline void pcpu_stats_chunk_alloc(void)
{
	unsigned long flags;
	spin_lock_irqsave(&pcpu_lock, flags);

	pcpu_stats.nr_chunks++;
	pcpu_stats.nr_max_chunks =
		max(pcpu_stats.nr_max_chunks, pcpu_stats.nr_chunks);

	spin_unlock_irqrestore(&pcpu_lock, flags);
}

/*
 * pcpu_stats_chunk_dealloc - decrement chunk stats
 */
static inline void pcpu_stats_chunk_dealloc(void)
{
	unsigned long flags;
	spin_lock_irqsave(&pcpu_lock, flags);

	pcpu_stats.nr_chunks--;

	spin_unlock_irqrestore(&pcpu_lock, flags);
}

#else

static inline void pcpu_stats_save_ai(const struct pcpu_alloc_info *ai)
{
}

static inline void pcpu_stats_area_alloc(struct pcpu_chunk *chunk, size_t size)
{
}

static inline void pcpu_stats_area_dealloc(struct pcpu_chunk *chunk)
{
}

static inline void pcpu_stats_chunk_alloc(void)
{
}

static inline void pcpu_stats_chunk_dealloc(void)
{
}

#endif /* !CONFIG_PERCPU_STATS */

#endif
