/*
 * 2002-10-18  written by Jim Houston jim.houston@ccur.com
 *	Copyright (C) 2002 by Concurrent Computer Corporation
 *	Distributed under the GNU GPL license version 2.
 *
 * Modified by George Anzinger to reuse immediately and to use
 * find bit instructions.  Also removed _irq on spinlocks.
 *
 * Modified by Nadia Derbey to make it RCU safe.
 *
 * Small id to pointer translation service.
 *
 * It uses a radix tree like structure as a sparse array indexed
 * by the id to obtain the pointer.  The bitmap makes allocating
 * a new id quick.
 *
 * You call it to allocate an id (an int) an associate with that id a
 * pointer or what ever, we treat it as a (void *).  You can pass this
 * id to a user for him to pass back at a later time.  You then pass
 * that id to this code and it returns your pointer.

 * You can release ids at any time. When all ids are released, most of
 * the memory is returned (we keep MAX_IDMX_FREE) in a local pool so we
 * don't need to go to the memory "store" during an id allocate, just
 * so you don't need to be too concerned about locking and conflicts
 * with the slab allocator.
 */

#ifndef TEST                        // to test in user space...
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/export.h>
#endif
#include <linux/err.h>
#include <linux/string.h>
#include <linux/idmx.h>
#include <linux/spinlock.h>

static struct kmem_cache *idmx_layer_cache;
static DEFINE_SPINLOCK(simple_idamx_lock);

static struct idmx_layer *get_from_free_list(struct idmx *idp)
{
	struct idmx_layer *p;
	unsigned long flags;

	spin_lock_irqsave(&idp->lock, flags);
	if ((p = idp->id_free)) {
		idp->id_free = p->ary[0];
		idp->id_free_cnt--;
		p->ary[0] = NULL;
	}
	spin_unlock_irqrestore(&idp->lock, flags);
	return(p);
}

static void idmx_layer_rcu_free(struct rcu_head *head)
{
	struct idmx_layer *layer;

	layer = container_of(head, struct idmx_layer, rcu_head);
	kmem_cache_free(idmx_layer_cache, layer);
}

static inline void free_layer(struct idmx_layer *p)
{
	call_rcu(&p->rcu_head, idmx_layer_rcu_free);
}

/* only called when idp->lock is held */
static void __move_to_free_list(struct idmx *idp, struct idmx_layer *p)
{
	p->ary[0] = idp->id_free;
	idp->id_free = p;
	idp->id_free_cnt++;
}

static void move_to_free_list(struct idmx *idp, struct idmx_layer *p)
{
	unsigned long flags;

	/*
	 * Depends on the return element being zeroed.
	 */
	spin_lock_irqsave(&idp->lock, flags);
	__move_to_free_list(idp, p);
	spin_unlock_irqrestore(&idp->lock, flags);
}

static void idmx_mark_full(struct idmx_layer **pa, int id)
{
	struct idmx_layer *p = pa[0];
	int l = 0;

	__set_bit(id & IDMX_MASK, &p->bitmap);
	/*
	 * If this layer is full mark the bit in the layer above to
	 * show that this part of the radix tree is full.  This may
	 * complete the layer above and require walking up the radix
	 * tree.
	 */
	while (p->bitmap == IDMX_FULL) {
		if (!(p = pa[++l]))
			break;
		id = id >> IDMX_BITS;
		__set_bit((id & IDMX_MASK), &p->bitmap);
	}
}

/**
 * idmx_pre_get - reserve resources for idmx allocation
 * @idp:	idmx handle
 * @gfp_mask:	memory allocation flags
 *
 * This function should be called prior to calling the idmx_get_new* functions.
 * It preallocates enough memory to satisfy the worst possible allocation. The
 * caller should pass in GFP_KERNEL if possible.  This of course requires that
 * no spinning locks be held.
 *
 * If the system is REALLY out of memory this function returns %0,
 * otherwise %1.
 */
int idmx_pre_get(struct idmx *idp, gfp_t gfp_mask)
{
	while (idp->id_free_cnt < MAX_IDMX_FREE) {
		struct idmx_layer *new;
		new = kmem_cache_zalloc(idmx_layer_cache, gfp_mask);
		if (new == NULL)
			return (0);
		move_to_free_list(idp, new);
	}
	return 1;
}
EXPORT_SYMBOL(idmx_pre_get);

static int sub_alloc(struct idmx *idp, int *starting_id, struct idmx_layer **pa)
{
	int n, m, sh;
	struct idmx_layer *p, *new;
	int l, id, oid;
	unsigned long bm;

	id = *starting_id;
 restart:
	p = idp->top;
	l = idp->layers;
	pa[l--] = NULL;
	while (1) {
		/*
		 * We run around this while until we reach the leaf node...
		 */
		n = (id >> (IDMX_BITS*l)) & IDMX_MASK;
		bm = ~p->bitmap;
		m = find_next_bit(&bm, IDMX_SIZE, n);
		if (m == IDMX_SIZE) {
			/* no space available go back to previous layer. */
			l++;
			oid = id;
			id = (id | ((1 << (IDMX_BITS * l)) - 1)) + 1;

			/* if already at the top layer, we need to grow */
			if (id >= 1 << (idp->layers * IDMX_BITS)) {
				*starting_id = id;
				return IDMX_NEED_TO_GROW;
			}
			p = pa[l];
			BUG_ON(!p);

			/* If we need to go up one layer, continue the
			 * loop; otherwise, restart from the top.
			 */
			sh = IDMX_BITS * (l + 1);
			if (oid >> sh == id >> sh)
				continue;
			else
				goto restart;
		}
		if (m != n) {
			sh = IDMX_BITS*l;
			id = ((id >> sh) ^ n ^ m) << sh;
		}
		if ((id >= MAX_IDMX_BIT) || (id < 0))
			return IDMX_NOMORE_SPACE;
		if (l == 0)
			break;
		/*
		 * Create the layer below if it is missing.
		 */
		if (!p->ary[m]) {
			new = get_from_free_list(idp);
			if (!new)
				return -1;
			new->layer = l-1;
			rcu_assign_pointer(p->ary[m], new);
			p->count++;
		}
		pa[l--] = p;
		p = p->ary[m];
	}

	pa[l] = p;
	return id;
}

static int idmx_get_empty_slot(struct idmx *idp, int starting_id,
			      struct idmx_layer **pa)
{
	struct idmx_layer *p, *new;
	int layers, v, id;
	unsigned long flags;

	id = starting_id;
build_up:
	p = idp->top;
	layers = idp->layers;
	if (unlikely(!p)) {
		if (!(p = get_from_free_list(idp)))
			return -1;
		p->layer = 0;
		layers = 1;
	}
	/*
	 * Add a new layer to the top of the tree if the requested
	 * id is larger than the currently allocated space.
	 */
	while ((layers < (MAX_IDMX_LEVEL - 1)) && (id >= (1 << (layers*IDMX_BITS)))) {
		layers++;
		if (!p->count) {
			/* special case: if the tree is currently empty,
			 * then we grow the tree by moving the top node
			 * upwards.
			 */
			p->layer++;
			continue;
		}
		if (!(new = get_from_free_list(idp))) {
			/*
			 * The allocation failed.  If we built part of
			 * the structure tear it down.
			 */
			spin_lock_irqsave(&idp->lock, flags);
			for (new = p; p && p != idp->top; new = p) {
				p = p->ary[0];
				new->ary[0] = NULL;
				new->bitmap = new->count = 0;
				__move_to_free_list(idp, new);
			}
			spin_unlock_irqrestore(&idp->lock, flags);
			return -1;
		}
		new->ary[0] = p;
		new->count = 1;
		new->layer = layers-1;
		if (p->bitmap == IDMX_FULL)
			__set_bit(0, &new->bitmap);
		p = new;
	}
	rcu_assign_pointer(idp->top, p);
	idp->layers = layers;
	v = sub_alloc(idp, &id, pa);
	if (v == IDMX_NEED_TO_GROW)
		goto build_up;
	return(v);
}

static int idmx_get_new_above_int(struct idmx *idp, void *ptr, int starting_id)
{
	struct idmx_layer *pa[MAX_IDMX_LEVEL];
	int id;

	id = idmx_get_empty_slot(idp, starting_id, pa);
	if (id >= 0) {
		/*
		 * Successfully found an empty slot.  Install the user
		 * pointer and mark the slot full.
		 */
		rcu_assign_pointer(pa[0]->ary[id & IDMX_MASK],
				(struct idmx_layer *)ptr);
		pa[0]->count++;
		idmx_mark_full(pa, id);
	}

	return id;
}

/**
 * idmx_get_new_above - allocate new idmx entry above or equal to a start id
 * @idp: idmx handle
 * @ptr: pointer you want associated with the id
 * @starting_id: id to start search at
 * @id: pointer to the allocated handle
 *
 * This is the allocate id function.  It should be called with any
 * required locks.
 *
 * If allocation from IDMX's private freelist fails, idmx_get_new_above() will
 * return %-EAGAIN.  The caller should retry the idmx_pre_get() call to refill
 * IDMX's preallocation and then retry the idmx_get_new_above() call.
 *
 * If the idmx is full idmx_get_new_above() will return %-ENOSPC.
 *
 * @id returns a value in the range @starting_id ... %0x7fffffff
 */
int idmx_get_new_above(struct idmx *idp, void *ptr, int starting_id, int *id)
{
	int rv;

	rv = idmx_get_new_above_int(idp, ptr, starting_id);
	/*
	 * This is a cheap hack until the IDMX code can be fixed to
	 * return proper error values.
	 */
	if (rv < 0)
		return _idmx_rc_to_errno(rv);
	*id = rv;
	return 0;
}
EXPORT_SYMBOL(idmx_get_new_above);

/**
 * idmx_get_new - allocate new idmx entry
 * @idp: idmx handle
 * @ptr: pointer you want associated with the id
 * @id: pointer to the allocated handle
 *
 * If allocation from IDMX's private freelist fails, idmx_get_new_above() will
 * return %-EAGAIN.  The caller should retry the idmx_pre_get() call to refill
 * IDMX's preallocation and then retry the idmx_get_new_above() call.
 *
 * If the idmx is full idmx_get_new_above() will return %-ENOSPC.
 *
 * @id returns a value in the range %0 ... %0x7fffffff
 */
int idmx_get_new(struct idmx *idp, void *ptr, int *id)
{
	int rv;

	rv = idmx_get_new_above_int(idp, ptr, 0);
	/*
	 * This is a cheap hack until the IDMX code can be fixed to
	 * return proper error values.
	 */
	if (rv < 0)
		return _idmx_rc_to_errno(rv);
	*id = rv;
	return 0;
}
EXPORT_SYMBOL(idmx_get_new);

static void idmx_remove_warning(int id)
{
	printk(KERN_WARNING
		"idmx_remove called for id=%d which is not allocated.\n", id);
	dump_stack();
}

static void sub_remove(struct idmx *idp, int shift, int id)
{
	struct idmx_layer *p = idp->top;
	struct idmx_layer **pa[MAX_IDMX_LEVEL];
	struct idmx_layer ***paa = &pa[0];
	struct idmx_layer *to_free;
	int n;

	*paa = NULL;
	*++paa = &idp->top;

	while ((shift > 0) && p) {
		n = (id >> shift) & IDMX_MASK;
		__clear_bit(n, &p->bitmap);
		*++paa = &p->ary[n];
		p = p->ary[n];
		shift -= IDMX_BITS;
	}
	n = id & IDMX_MASK;
	if (likely(p != NULL && test_bit(n, &p->bitmap))){
		__clear_bit(n, &p->bitmap);
		rcu_assign_pointer(p->ary[n], NULL);
		to_free = NULL;
		while(*paa && ! --((**paa)->count)){
			if (to_free)
				free_layer(to_free);
			to_free = **paa;
			**paa-- = NULL;
		}
		if (!*paa)
			idp->layers = 0;
		if (to_free)
			free_layer(to_free);
	} else
		idmx_remove_warning(id);
}

/**
 * idmx_remove - remove the given id and free its slot
 * @idp: idmx handle
 * @id: unique key
 */
void idmx_remove(struct idmx *idp, int id)
{
	struct idmx_layer *p;
	struct idmx_layer *to_free;

	/* Mask off upper bits we don't use for the search. */
	id &= MAX_IDMX_MASK;

	sub_remove(idp, (idp->layers - 1) * IDMX_BITS, id);
	if (idp->top && idp->top->count == 1 && (idp->layers > 1) &&
	    idp->top->ary[0]) {
		/*
		 * Single child at leftmost slot: we can shrink the tree.
		 * This level is not needed anymore since when layers are
		 * inserted, they are inserted at the top of the existing
		 * tree.
		 */
		to_free = idp->top;
		p = idp->top->ary[0];
		rcu_assign_pointer(idp->top, p);
		--idp->layers;
		to_free->bitmap = to_free->count = 0;
		free_layer(to_free);
	}
	while (idp->id_free_cnt >= MAX_IDMX_FREE) {
		p = get_from_free_list(idp);
		/*
		 * Note: we don't call the rcu callback here, since the only
		 * layers that fall into the freelist are those that have been
		 * preallocated.
		 */
		kmem_cache_free(idmx_layer_cache, p);
	}
	return;
}
EXPORT_SYMBOL(idmx_remove);

/**
 * idmx_remove_all - remove all ids from the given idmx tree
 * @idp: idmx handle
 *
 * idmx_destroy() only frees up unused, cached idp_layers, but this
 * function will remove all id mappings and leave all idp_layers
 * unused.
 *
 * A typical clean-up sequence for objects stored in an idmx tree will
 * use idmx_for_each() to free all objects, if necessay, then
 * idmx_remove_all() to remove all ids, and idmx_destroy() to free
 * up the cached idmx_layers.
 */
void idmx_remove_all(struct idmx *idp)
{
	int n, id, max;
	int bt_mask;
	struct idmx_layer *p;
	struct idmx_layer *pa[MAX_IDMX_LEVEL];
	struct idmx_layer **paa = &pa[0];

	n = idp->layers * IDMX_BITS;
	p = idp->top;
	rcu_assign_pointer(idp->top, NULL);
	max = 1 << n;

	id = 0;
	while (id < max) {
		while (n > IDMX_BITS && p) {
			n -= IDMX_BITS;
			*paa++ = p;
			p = p->ary[(id >> n) & IDMX_MASK];
		}

		bt_mask = id;
		id += 1 << n;
		/* Get the highest bit that the above add changed from 0->1. */
		while (n < fls(id ^ bt_mask)) {
			if (p)
				free_layer(p);
			n += IDMX_BITS;
			p = *--paa;
		}
	}
	idp->layers = 0;
}
EXPORT_SYMBOL(idmx_remove_all);

/**
 * idmx_destroy - release all cached layers within an idmx tree
 * @idp: idmx handle
 */
void idmx_destroy(struct idmx *idp)
{
	while (idp->id_free_cnt) {
		struct idmx_layer *p = get_from_free_list(idp);
		kmem_cache_free(idmx_layer_cache, p);
	}
}
EXPORT_SYMBOL(idmx_destroy);

/**
 * idmx_find - return pointer for given id
 * @idp: idmx handle
 * @id: lookup key
 *
 * Return the pointer given the id it has been registered with.  A %NULL
 * return indicates that @id is not valid or you passed %NULL in
 * idmx_get_new().
 *
 * This function can be called under rcu_read_lock(), given that the leaf
 * pointers lifetimes are correctly managed.
 */
void *idmx_find(struct idmx *idp, int id)
{
	int n;
	struct idmx_layer *p;

	p = rcu_dereference_raw(idp->top);
	if (!p)
		return NULL;
	n = (p->layer+1) * IDMX_BITS;

	/* Mask off upper bits we don't use for the search. */
	id &= MAX_IDMX_MASK;

	if (id >= (1 << n))
		return NULL;
	BUG_ON(n == 0);

	while (n > 0 && p) {
		n -= IDMX_BITS;
		BUG_ON(n != p->layer*IDMX_BITS);
		p = rcu_dereference_raw(p->ary[(id >> n) & IDMX_MASK]);
	}
	return((void *)p);
}
EXPORT_SYMBOL(idmx_find);

/**
 * idmx_for_each - iterate through all stored pointers
 * @idp: idmx handle
 * @fn: function to be called for each pointer
 * @data: data passed back to callback function
 *
 * Iterate over the pointers registered with the given idmx.  The
 * callback function will be called for each pointer currently
 * registered, passing the id, the pointer and the data pointer passed
 * to this function.  It is not safe to modify the idmx tree while in
 * the callback, so functions such as idmx_get_new and idmx_remove are
 * not allowed.
 *
 * We check the return of @fn each time. If it returns anything other
 * than %0, we break out and return that value.
 *
 * The caller must serialize idmx_for_each() vs idmx_get_new() and idmx_remove().
 */
int idmx_for_each(struct idmx *idp,
		 int (*fn)(int id, void *p, void *data), void *data)
{
	int n, id, max, error = 0;
	struct idmx_layer *p;
	struct idmx_layer *pa[MAX_IDMX_LEVEL];
	struct idmx_layer **paa = &pa[0];

	n = idp->layers * IDMX_BITS;
	p = rcu_dereference_raw(idp->top);
	max = 1 << n;

	id = 0;
	while (id < max) {
		while (n > 0 && p) {
			n -= IDMX_BITS;
			*paa++ = p;
			p = rcu_dereference_raw(p->ary[(id >> n) & IDMX_MASK]);
		}

		if (p) {
			error = fn(id, (void *)p, data);
			if (error)
				break;
		}

		id += 1 << n;
		while (n < fls(id)) {
			n += IDMX_BITS;
			p = *--paa;
		}
	}

	return error;
}
EXPORT_SYMBOL(idmx_for_each);

/**
 * idmx_get_next - lookup next object of id to given id.
 * @idp: idmx handle
 * @nextidp:  pointer to lookup key
 *
 * Returns pointer to registered object with id, which is next number to
 * given id. After being looked up, *@nextidp will be updated for the next
 * iteration.
 *
 * This function can be called under rcu_read_lock(), given that the leaf
 * pointers lifetimes are correctly managed.
 */
void *idmx_get_next(struct idmx *idp, int *nextidp)
{
	struct idmx_layer *p, *pa[MAX_IDMX_LEVEL];
	struct idmx_layer **paa = &pa[0];
	int id = *nextidp;
	int n, max;

	/* find first ent */
	p = rcu_dereference_raw(idp->top);
	if (!p)
		return NULL;
	n = (p->layer + 1) * IDMX_BITS;
	max = 1 << n;

	while (id < max) {
		while (n > 0 && p) {
			n -= IDMX_BITS;
			*paa++ = p;
			p = rcu_dereference_raw(p->ary[(id >> n) & IDMX_MASK]);
		}

		if (p) {
			*nextidp = id;
			return p;
		}

		/*
		 * Proceed to the next layer at the current level.  Unlike
		 * idmx_for_each(), @id isn't guaranteed to be aligned to
		 * layer boundary at this point and adding 1 << n may
		 * incorrectly skip IDs.  Make sure we jump to the
		 * beginning of the next layer using round_up().
		 */
		id = round_up(id + 1, 1 << n);
		while (n < fls(id)) {
			n += IDMX_BITS;
			p = *--paa;
		}
	}
	return NULL;
}
EXPORT_SYMBOL(idmx_get_next);


/**
 * idmx_replace - replace pointer for given id
 * @idp: idmx handle
 * @ptr: pointer you want associated with the id
 * @id: lookup key
 *
 * Replace the pointer registered with an id and return the old value.
 * A %-ENOENT return indicates that @id was not found.
 * A %-EINVAL return indicates that @id was not within valid constraints.
 *
 * The caller must serialize with writers.
 */
void *idmx_replace(struct idmx *idp, void *ptr, int id)
{
	int n;
	struct idmx_layer *p, *old_p;

	p = idp->top;
	if (!p)
		return ERR_PTR(-EINVAL);

	n = (p->layer+1) * IDMX_BITS;

	id &= MAX_IDMX_MASK;

	if (id >= (1 << n))
		return ERR_PTR(-EINVAL);

	n -= IDMX_BITS;
	while ((n > 0) && p) {
		p = p->ary[(id >> n) & IDMX_MASK];
		n -= IDMX_BITS;
	}

	n = id & IDMX_MASK;
	if (unlikely(p == NULL || !test_bit(n, &p->bitmap)))
		return ERR_PTR(-ENOENT);

	old_p = p->ary[n];
	rcu_assign_pointer(p->ary[n], ptr);

	return old_p;
}
EXPORT_SYMBOL(idmx_replace);

void __init idmx_init_cache(void)
{
	idmx_layer_cache = kmem_cache_create("idmx_layer_cache",
				sizeof(struct idmx_layer), 0, SLAB_PANIC, NULL);
}

/**
 * idmx_init - initialize idmx handle
 * @idp:	idmx handle
 *
 * This function is use to set up the handle (@idp) that you will pass
 * to the rest of the functions.
 */
void idmx_init(struct idmx *idp)
{
	memset(idp, 0, sizeof(struct idmx));
	spin_lock_init(&idp->lock);
}
EXPORT_SYMBOL(idmx_init);


/**
 * DOC: IDAMX description
 * IDAMX - IDMX based ID allocator
 *
 * This is id allocator without id -> pointer translation.  Memory
 * usage is much lower than full blown idmx because each id only
 * occupies a bit.  idamx uses a custom leaf node which contains
 * IDAMX_BITMAP_BITS slots.
 *
 * 2007-04-25  written by Tejun Heo <htejun@gmail.com>
 */

static void free_bitmap(struct idamx *idamx, struct idamx_bitmap *bitmap)
{
	unsigned long flags;

	if (!idamx->free_bitmap) {
		spin_lock_irqsave(&idamx->idmx.lock, flags);
		if (!idamx->free_bitmap) {
			idamx->free_bitmap = bitmap;
			bitmap = NULL;
		}
		spin_unlock_irqrestore(&idamx->idmx.lock, flags);
	}

	kfree(bitmap);
}

/**
 * idamx_pre_get - reserve resources for idamx allocation
 * @idamx:	idamx handle
 * @gfp_mask:	memory allocation flag
 *
 * This function should be called prior to locking and calling the
 * following function.  It preallocates enough memory to satisfy the
 * worst possible allocation.
 *
 * If the system is REALLY out of memory this function returns %0,
 * otherwise %1.
 */
int idamx_pre_get(struct idamx *idamx, gfp_t gfp_mask)
{
	/* allocate idmx_layers */
	if (!idmx_pre_get(&idamx->idmx, gfp_mask))
		return 0;

	/* allocate free_bitmap */
	if (!idamx->free_bitmap) {
		struct idamx_bitmap *bitmap;

		bitmap = kmalloc(sizeof(struct idamx_bitmap), gfp_mask);
		if (!bitmap)
			return 0;

		free_bitmap(idamx, bitmap);
	}

	return 1;
}
EXPORT_SYMBOL(idamx_pre_get);

/**
 * idamx_get_new_above - allocate new ID above or equal to a start id
 * @idamx:	idamx handle
 * @starting_id: id to start search at
 * @p_id:	pointer to the allocated handle
 *
 * Allocate new ID above or equal to @starting_id.  It should be called
 * with any required locks.
 *
 * If memory is required, it will return %-EAGAIN, you should unlock
 * and go back to the idamx_pre_get() call.  If the idamx is full, it will
 * return %-ENOSPC.
 *
 * @p_id returns a value in the range @starting_id ... %0x7fffffff.
 */
int idamx_get_new_above(struct idamx *idamx, int starting_id, int *p_id)
{
	struct idmx_layer *pa[MAX_IDMX_LEVEL];
	struct idamx_bitmap *bitmap;
	unsigned long flags;
	int idmx_id = starting_id / IDAMX_BITMAP_BITS;
	int offset = starting_id % IDAMX_BITMAP_BITS;
	int t, id;

 restart:
	/* get vacant slot */
	t = idmx_get_empty_slot(&idamx->idmx, idmx_id, pa);
	if (t < 0)
		return _idmx_rc_to_errno(t);

	if (t * IDAMX_BITMAP_BITS >= MAX_IDMX_BIT)
		return -ENOSPC;

	if (t != idmx_id)
		offset = 0;
	idmx_id = t;

	/* if bitmap isn't there, create a new one */
	bitmap = (void *)pa[0]->ary[idmx_id & IDMX_MASK];
	if (!bitmap) {
		spin_lock_irqsave(&idamx->idmx.lock, flags);
		bitmap = idamx->free_bitmap;
		idamx->free_bitmap = NULL;
		spin_unlock_irqrestore(&idamx->idmx.lock, flags);

		if (!bitmap)
			return -EAGAIN;

		memset(bitmap, 0, sizeof(struct idamx_bitmap));
		rcu_assign_pointer(pa[0]->ary[idmx_id & IDMX_MASK],
				(void *)bitmap);
		pa[0]->count++;
	}

	/* lookup for empty slot */
	t = find_next_zero_bit(bitmap->bitmap, IDAMX_BITMAP_BITS, offset);
	if (t == IDAMX_BITMAP_BITS) {
		/* no empty slot after offset, continue to the next chunk */
		idmx_id++;
		offset = 0;
		goto restart;
	}

	id = idmx_id * IDAMX_BITMAP_BITS + t;
	if (id >= MAX_IDMX_BIT)
		return -ENOSPC;

	__set_bit(t, bitmap->bitmap);
	if (++bitmap->nr_busy == IDAMX_BITMAP_BITS)
		idmx_mark_full(pa, idmx_id);

	*p_id = id;

	/* Each leaf node can handle nearly a thousand slots and the
	 * whole idea of idamx is to have small memory foot print.
	 * Throw away extra resources one by one after each successful
	 * allocation.
	 */
	if (idamx->idmx.id_free_cnt || idamx->free_bitmap) {
		struct idmx_layer *p = get_from_free_list(&idamx->idmx);
		if (p)
			kmem_cache_free(idmx_layer_cache, p);
	}

	return 0;
}
EXPORT_SYMBOL(idamx_get_new_above);

/**
 * idamx_get_new - allocate new ID
 * @idamx:	idmx handle
 * @p_id:	pointer to the allocated handle
 *
 * Allocate new ID.  It should be called with any required locks.
 *
 * If memory is required, it will return %-EAGAIN, you should unlock
 * and go back to the idmx_pre_get() call.  If the idmx is full, it will
 * return %-ENOSPC.
 *
 * @p_id returns a value in the range %0 ... %0x7fffffff.
 */
int idamx_get_new(struct idamx *idamx, int *p_id)
{
	return idamx_get_new_above(idamx, 0, p_id);
}
EXPORT_SYMBOL(idamx_get_new);

/**
 * idamx_remove - remove the given ID
 * @idamx:	idamx handle
 * @id:		ID to free
 */
void idamx_remove(struct idamx *idamx, int id)
{
	struct idmx_layer *p = idamx->idmx.top;
	int shift = (idamx->idmx.layers - 1) * IDMX_BITS;
	int idmx_id = id / IDAMX_BITMAP_BITS;
	int offset = id % IDAMX_BITMAP_BITS;
	int n;
	struct idamx_bitmap *bitmap;

	/* clear full bits while looking up the leaf idmx_layer */
	while ((shift > 0) && p) {
		n = (idmx_id >> shift) & IDMX_MASK;
		__clear_bit(n, &p->bitmap);
		p = p->ary[n];
		shift -= IDMX_BITS;
	}

	if (p == NULL)
		goto err;

	n = idmx_id & IDMX_MASK;
	__clear_bit(n, &p->bitmap);

	bitmap = (void *)p->ary[n];
	if (!test_bit(offset, bitmap->bitmap))
		goto err;

	/* update bitmap and remove it if empty */
	__clear_bit(offset, bitmap->bitmap);
	if (--bitmap->nr_busy == 0) {
		__set_bit(n, &p->bitmap);	/* to please idmx_remove() */
		idmx_remove(&idamx->idmx, idmx_id);
		free_bitmap(idamx, bitmap);
	}

	return;

 err:
	printk(KERN_WARNING
	       "idamx_remove called for id=%d which is not allocated.\n", id);
}
EXPORT_SYMBOL(idamx_remove);

/**
 * idamx_destroy - release all cached layers within an idamx tree
 * @idamx:		idamx handle
 */
void idamx_destroy(struct idamx *idamx)
{
	idmx_destroy(&idamx->idmx);
	kfree(idamx->free_bitmap);
}
EXPORT_SYMBOL(idamx_destroy);

/**
 * idamx_simple_get - get a new id.
 * @idamx: the (initialized) idamx.
 * @start: the minimum id (inclusive, < 0x8000000)
 * @end: the maximum id (exclusive, < 0x8000000 or 0)
 * @gfp_mask: memory allocation flags
 *
 * Allocates an id in the range start <= id < end, or returns -ENOSPC.
 * On memory allocation failure, returns -ENOMEM.
 *
 * Use idamx_simple_remove() to get rid of an id.
 */
int idamx_simple_get(struct idamx *idamx, unsigned int start, unsigned int end,
		   gfp_t gfp_mask)
{
	int ret, id;
	unsigned int max;
	unsigned long flags;

	BUG_ON((int)start < 0);
	BUG_ON((int)end < 0);

	if (end == 0)
		max = 0x80000000;
	else {
		BUG_ON(end < start);
		max = end - 1;
	}

again:
	if (!idamx_pre_get(idamx, gfp_mask))
		return -ENOMEM;

	spin_lock_irqsave(&simple_idamx_lock, flags);
	ret = idamx_get_new_above(idamx, start, &id);
	if (!ret) {
		if (id > max) {
			idamx_remove(idamx, id);
			ret = -ENOSPC;
		} else {
			ret = id;
		}
	}
	spin_unlock_irqrestore(&simple_idamx_lock, flags);

	if (unlikely(ret == -EAGAIN))
		goto again;

	return ret;
}
EXPORT_SYMBOL(idamx_simple_get);

/**
 * idamx_simple_remove - remove an allocated id.
 * @idamx: the (initialized) idamx.
 * @id: the id returned by idamx_simple_get.
 */
void idamx_simple_remove(struct idamx *idamx, unsigned int id)
{
	unsigned long flags;

	BUG_ON((int)id < 0);
	spin_lock_irqsave(&simple_idamx_lock, flags);
	idamx_remove(idamx, id);
	spin_unlock_irqrestore(&simple_idamx_lock, flags);
}
EXPORT_SYMBOL(idamx_simple_remove);

/**
 * idamx_init - initialize idamx handle
 * @idamx:	idamx handle
 *
 * This function is use to set up the handle (@idamx) that you will pass
 * to the rest of the functions.
 */
void idamx_init(struct idamx *idamx)
{
	memset(idamx, 0, sizeof(struct idamx));
	idmx_init(&idamx->idmx);

}
EXPORT_SYMBOL(idamx_init);

