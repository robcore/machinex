/*
 * include/linux/idmx.h
 *
 * 2002-10-18  written by Jim Houston jim.houston@ccur.com
 *	Copyright (C) 2002 by Concurrent Computer Corporation
 *	Distributed under the GNU GPL license version 2.
 *
 * Small id to pointer translation service avoiding fixed sized
 * tables.
 */

#ifndef __IDMX_H__
#define __IDMX_H__

#include <linux/types.h>
#include <linux/bitops.h>
#include <linux/init.h>
#include <linux/rcupdate.h>

#if BITS_PER_LONG == 32
# define IDMX_BITS 5
# define IDMX_FULL 0xfffffffful
/* We can only use two of the bits in the top level because there is
   only one possible bit in the top level (5 bits * 7 levels = 35
   bits, but you only use 31 bits in the id). */
# define TOP_LEVEL_FULL (IDMX_FULL >> 30)
#elif BITS_PER_LONG == 64
# define IDMX_BITS 6
# define IDMX_FULL 0xfffffffffffffffful
/* We can only use two of the bits in the top level because there is
   only one possible bit in the top level (6 bits * 6 levels = 36
   bits, but you only use 31 bits in the id). */
# define TOP_LEVEL_FULL (IDMX_FULL >> 62)
#else
# error "BITS_PER_LONG is not 32 or 64"
#endif

#define IDMX_SIZE (1 << IDMX_BITS)
#define IDMX_MASK ((1 << IDMX_BITS)-1)

#define MAX_IDMX_SHIFT (sizeof(int)*8 - 1)
#define MAX_IDMX_BIT (1U << MAX_IDMX_SHIFT)
#define MAX_IDMX_MASK (MAX_IDMX_BIT - 1)

/* Leave the possibility of an incomplete final layer */
#define MAX_IDMX_LEVEL ((MAX_IDMX_SHIFT + IDMX_BITS - 1) / IDMX_BITS)

/* Number of id_layer structs to leave in free list */
#define MAX_IDMX_FREE (MAX_IDMX_LEVEL * 2)

struct idmx_layer {
	unsigned long		 bitmap; /* A zero bit means "space here" */
	struct idmx_layer __rcu	*ary[1<<IDMX_BITS];
	int			 count;	 /* When zero, we can release it */
	int			 layer;	 /* distance from leaf */
	struct rcu_head		 rcu_head;
};

struct idmx {
	struct idmx_layer __rcu *top;
	struct idmx_layer *id_free;
	int		  layers; /* only valid without concurrent changes */
	int		  id_free_cnt;
	spinlock_t	  lock;
};

#define IDMX_INIT(name)						\
{								\
	.top		= NULL,					\
	.id_free	= NULL,					\
	.layers 	= 0,					\
	.id_free_cnt	= 0,					\
	.lock		= __SPIN_LOCK_UNLOCKED(name.lock),	\
}
#define DEFINE_IDMX(name)	struct idmx name = IDMX_INIT(name)

/* Actions to be taken after a call to _idmx_sub_alloc */
#define IDMX_NEED_TO_GROW -2
#define IDMX_NOMORE_SPACE -3

#define _idmx_rc_to_errno(rc) ((rc) == -1 ? -EAGAIN : -ENOSPC)

/**
 * DOC: idmx sync
 * idmx synchronization (stolen from radix-tree.h)
 *
 * idmx_find() is able to be called locklessly, using RCU. The caller must
 * ensure calls to this function are made within rcu_read_lock() regions.
 * Other readers (lock-free or otherwise) and modifications may be running
 * concurrently.
 *
 * It is still required that the caller manage the synchronization and
 * lifetimes of the items. So if RCU lock-free lookups are used, typically
 * this would mean that the items have their own locks, or are amenable to
 * lock-free access; and that the items are freed by RCU (or only freed after
 * having been deleted from the idmx tree *and* a synchronize_rcu() grace
 * period).
 */

/*
 * This is what we export.
 */

void *idmx_find(struct idmx *idp, int id);
int idmx_pre_get(struct idmx *idp, gfp_t gfp_mask);
int idmx_get_new(struct idmx *idp, void *ptr, int *id);
int idmx_get_new_above(struct idmx *idp, void *ptr, int starting_id, int *id);
int idmx_for_each(struct idmx *idp,
		 int (*fn)(int id, void *p, void *data), void *data);
void *idmx_get_next(struct idmx *idp, int *nextid);
void *idmx_replace(struct idmx *idp, void *ptr, int id);
void idmx_remove(struct idmx *idp, int id);
void idmx_remove_all(struct idmx *idp);
void idmx_destroy(struct idmx *idp);
void idmx_init(struct idmx *idp);

/**
 * backport of idmx idmx_alloc() usage
 *
 * This backports a patch series send by Tejun Heo:
 * https://lkml.org/lkml/2013/2/2/159
 */
static inline void compat_idmx_destroy(struct idmx *idp)
{
	idmx_remove_all(idp);
	idmx_destroy(idp);
}
//#define idmx_destroy(idp) compat_idmx_destroy(idp)

static inline int idmx_alloc(struct idmx *idmx, void *ptr, int start, int end,
			    gfp_t gfp_mask)
{
	int id, ret;

	do {
		if (!idmx_pre_get(idmx, gfp_mask))
			return -ENOMEM;
		ret = idmx_get_new_above(idmx, ptr, start, &id);
		if (!ret && id > end) {
			idmx_remove(idmx, id);
			ret = -ENOSPC;
		}
	} while (ret == -EAGAIN);

	return ret ? ret : id;
}

static inline void idmx_preload(gfp_t gfp_mask)
{
}

static inline void idmx_preload_end(void)
{
}

/*
 * IDA - IDMX based id allocator, use when translation from id to
 * pointer isn't necessary.
 *
 * IDA_BITMAP_LONGS is calculated to be one less to accommodate
 * ida_bitmap->nr_busy so that the whole struct fits in 128 bytes.
 */
#define IDA_CHUNK_SIZE		128	/* 128 bytes per chunk */
#define IDA_BITMAP_LONGS	(IDA_CHUNK_SIZE / sizeof(long) - 1)
#define IDA_BITMAP_BITS 	(IDA_BITMAP_LONGS * sizeof(long) * 8)

struct ida_bitmap {
	long			nr_busy;
	unsigned long		bitmap[IDA_BITMAP_LONGS];
};

struct ida {
	struct idmx		idmx;
	struct ida_bitmap	*free_bitmap;
};

#define IDA_INIT(name)		{ .idmx = IDMX_INIT((name).idmx), .free_bitmap = NULL, }
#define DEFINE_IDA(name)	struct ida name = IDA_INIT(name)

int ida_pre_get(struct ida *ida, gfp_t gfp_mask);
int ida_get_new_above(struct ida *ida, int starting_id, int *p_id);
int ida_get_new(struct ida *ida, int *p_id);
void ida_remove(struct ida *ida, int id);
void ida_destroy(struct ida *ida);
void ida_init(struct ida *ida);

int ida_simple_get(struct ida *ida, unsigned int start, unsigned int end,
		   gfp_t gfp_mask);
void ida_simple_remove(struct ida *ida, unsigned int id);

void __init idmx_init_cache(void);

/**
 * idmx_for_each_entry - iterate over an idmx's elements of a given type
 * @idp:     idmx handle
 * @entry:   the type * to use as cursor
 * @id:      id entry's key
 */
#define idmx_for_each_entry(idp, entry, id)				\
	for (id = 0, entry = (typeof(entry))idmx_get_next((idp), &(id)); \
	     entry != NULL;                                             \
	     ++id, entry = (typeof(entry))idmx_get_next((idp), &(id)))

#endif /* __IDMX_H__ */
