/*
 *  fs/ext4/extents_status.c
 *
 * Written by Yongqiang Yang <xiaoqiangnk@gmail.com>
 * Modified by
 *	Allison Henderson <achender@linux.vnet.ibm.com>
 *	Hugh Dickins <hughd@google.com>
 *	Zheng Liu <wenqing.lz@taobao.com>
 *
 * Ext4 extents status tree core functions.
 */
#include <linux/rbtree.h>
#include "ext4.h"
#include "extents_status.h"
#include "ext4_extents.h"

#include <trace/events/ext4.h>

/*
 * According to previous discussion in Ext4 Developer Workshop, we
 * will introduce a new structure called io tree to track all extent
 * status in order to solve some problems that we have met
 * (e.g. Reservation space warning), and provide extent-level locking.
 * Delay extent tree is the first step to achieve this goal.  It is
 * original built by Yongqiang Yang.  At that time it is called delay
 * extent tree, whose goal is only track delayed extents in memory to
 * simplify the implementation of fiemap and bigalloc, and introduce
 * lseek SEEK_DATA/SEEK_HOLE support.  That is why it is still called
 * delay extent tree at the first commit.  But for better understand
 * what it does, it has been rename to extent status tree.
 *
 * Step1:
 * Currently the first step has been done.  All delayed extents are
 * tracked in the tree.  It maintains the delayed extent when a delayed
 * allocation is issued, and the delayed extent is written out or
 * invalidated.  Therefore the implementation of fiemap and bigalloc
 * are simplified, and SEEK_DATA/SEEK_HOLE are introduced.
 *
 * The following comment describes the implemenmtation of extent
 * status tree and future works.
 *
 * Step2:
 * In this step all extent status are tracked by extent status tree.
 * Thus, we can first try to lookup a block mapping in this tree before
 * finding it in extent tree.  Hence, single extent cache can be removed
 * because extent status tree can do a better job.  Extents in status
 * tree are loaded on-demand.  Therefore, the extent status tree may not
 * contain all of the extents in a file.  Meanwhile we define a shrinker
 * to reclaim memory from extent status tree because fragmented extent
 * tree will make status tree cost too much memory.  written/unwritten/-
 * hole extents in the tree will be reclaimed by this shrinker when we
 * are under high memory pressure.  Delayed extents will not be
 * reclimed because fiemap, bigalloc, and seek_data/hole need it.
 */

/*
 * Extent status tree implementation for ext4.
 *
 *
 * ==========================================================================
 * Extent status tree tracks all extent status.
 *
 * 1. Why we need to implement extent status tree?
 *
 * Without extent status tree, ext4 identifies a delayed extent by looking
 * up page cache, this has several deficiencies - complicated, buggy,
 * and inefficient code.
 *
 * FIEMAP, SEEK_HOLE/DATA, bigalloc, and writeout all need to know if a
 * block or a range of blocks are belonged to a delayed extent.
 *
 * Let us have a look at how they do without extent status tree.
 *   --	FIEMAP
 *	FIEMAP looks up page cache to identify delayed allocations from holes.
 *
 *   --	SEEK_HOLE/DATA
 *	SEEK_HOLE/DATA has the same problem as FIEMAP.
 *
 *   --	bigalloc
 *	bigalloc looks up page cache to figure out if a block is
 *	already under delayed allocation or not to determine whether
 *	quota reserving is needed for the cluster.
 *
 *   --	writeout
 *	Writeout looks up whole page cache to see if a buffer is
 *	mapped, If there are not very many delayed buffers, then it is
 *	time comsuming.
 *
 * With extent status tree implementation, FIEMAP, SEEK_HOLE/DATA,
 * bigalloc and writeout can figure out if a block or a range of
 * blocks is under delayed allocation(belonged to a delayed extent) or
 * not by searching the extent tree.
 *
 *
 * ==========================================================================
 * 2. Ext4 extent status tree impelmentation
 *
 *   --	extent
 *	A extent is a range of blocks which are contiguous logically and
 *	physically.  Unlike extent in extent tree, this extent in ext4 is
 *	a in-memory struct, there is no corresponding on-disk data.  There
 *	is no limit on length of extent, so an extent can contain as many
 *	blocks as they are contiguous logically and physically.
 *
 *   --	extent status tree
 *	Every inode has an extent status tree and all allocation blocks
 *	are added to the tree with different status.  The extent in the
 *	tree are ordered by logical block no.
 *
 *   --	operations on a extent status tree
 *	There are three important operations on a delayed extent tree: find
 *	next extent, adding a extent(a range of blocks) and removing a extent.
 *
 *   --	race on a extent status tree
 *	Extent status tree is protected by inode->i_es_lock.
 *
 *   --	memory consumption
 *      Fragmented extent tree will make extent status tree cost too much
 *      memory.  Hence, we will reclaim written/unwritten/hole extents from
 *      the tree under a heavy memory pressure.
 *
 *
 * ==========================================================================
 * 3. Performance analysis
 *
 *   --	overhead
 *	1. There is a cache extent for write access, so if writes are
 *	not very random, adding space operaions are in O(1) time.
 *
 *   --	gain
 *	2. Code is much simpler, more readable, more maintainable and
 *	more efficient.
 *
 *
 * ==========================================================================
 * 4. TODO list
 *
 *   -- Refactor delayed space reservation
 *
 *   -- Extent-level locking
 */

static struct kmem_cache *ext4_es_cachep;

static int __es_insert_extent(struct inode *inode, struct extent_status *newes);
static int __es_remove_extent(struct inode *inode, ext4_lblk_t lblk,
			      ext4_lblk_t end);

int __init ext4_init_es(void)
{
	ext4_es_cachep = KMEM_CACHE(extent_status, SLAB_RECLAIM_ACCOUNT);
	if (ext4_es_cachep == NULL)
		return -ENOMEM;
	return 0;
}

void ext4_exit_es(void)
{
	if (ext4_es_cachep)
		kmem_cache_destroy(ext4_es_cachep);
}

void ext4_es_init_tree(struct ext4_es_tree *tree)
{
	tree->root = RB_ROOT;
	tree->cache_es = NULL;
}

#ifdef ES_DEBUG__
static void ext4_es_print_tree(struct inode *inode)
{
	struct ext4_es_tree *tree;
	struct rb_node *node;

	printk(KERN_DEBUG "status extents for inode %lu:", inode->i_ino);
	tree = &EXT4_I(inode)->i_es_tree;
	node = rb_first(&tree->root);
	while (node) {
		struct extent_status *es;
		es = rb_entry(node, struct extent_status, rb_node);
		printk(KERN_DEBUG " [%u/%u) %llu %llx",
		       es->es_lblk, es->es_len,
		       ext4_es_pblock(es), ext4_es_status(es));
		node = rb_next(node);
	}
	printk(KERN_DEBUG "\n");
}
#else
#define ext4_es_print_tree(inode)
#endif

static inline ext4_lblk_t ext4_es_end(struct extent_status *es)
{
	BUG_ON(es->es_lblk + es->es_len < es->es_lblk);
	return es->es_lblk + es->es_len - 1;
}

/*
 * search through the tree for an delayed extent with a given offset.  If
 * it can't be found, try to find next extent.
 */
static struct extent_status *__es_tree_search(struct rb_root *root,
					      ext4_lblk_t lblk)
{
	struct rb_node *node = root->rb_node;
	struct extent_status *es = NULL;

	while (node) {
		es = rb_entry(node, struct extent_status, rb_node);
		if (lblk < es->es_lblk)
			node = node->rb_left;
		else if (lblk > ext4_es_end(es))
			node = node->rb_right;
		else
			return es;
	}

	if (es && lblk < es->es_lblk)
		return es;

	if (es && lblk > ext4_es_end(es)) {
		node = rb_next(&es->rb_node);
		return node ? rb_entry(node, struct extent_status, rb_node) :
			      NULL;
	}

	return NULL;
}

/*
 * ext4_es_find_delayed_extent: find the 1st delayed extent covering @es->lblk
 * if it exists, otherwise, the next extent after @es->lblk.
 *
 * @inode: the inode which owns delayed extents
 * @lblk: the offset where we start to search
 * @es: delayed extent that we found
 */
void ext4_es_find_delayed_extent(struct inode *inode, ext4_lblk_t lblk,
				 struct extent_status *es)
{
	struct ext4_es_tree *tree = NULL;
	struct extent_status *es1 = NULL;
	struct rb_node *node;

	BUG_ON(es == NULL);
	trace_ext4_es_find_delayed_extent_enter(inode, lblk);

	read_lock(&EXT4_I(inode)->i_es_lock);
	tree = &EXT4_I(inode)->i_es_tree;

	/* find extent in cache firstly */
	es->es_lblk = es->es_len = es->es_pblk = 0;
	if (tree->cache_es) {
		es1 = tree->cache_es;
		if (in_range(lblk, es1->es_lblk, es1->es_len)) {
			es_debug("%u cached by [%u/%u) %llu %llx\n",
				 lblk, es1->es_lblk, es1->es_len,
				 ext4_es_pblock(es1), ext4_es_status(es1));
			goto out;
		}
	}

	es1 = __es_tree_search(&tree->root, lblk);

out:
	if (es1 && !ext4_es_is_delayed(es1)) {
		while ((node = rb_next(&es1->rb_node)) != NULL) {
			es1 = rb_entry(node, struct extent_status, rb_node);
			if (ext4_es_is_delayed(es1))
				break;
		}
	}

	if (es1 && ext4_es_is_delayed(es1)) {
		tree->cache_es = es1;
		es->es_lblk = es1->es_lblk;
		es->es_len = es1->es_len;
		es->es_pblk = es1->es_pblk;
	}

	read_unlock(&EXT4_I(inode)->i_es_lock);

	trace_ext4_es_find_delayed_extent_exit(inode, es);
}

static struct extent_status *
ext4_es_alloc_extent(struct inode *inode, ext4_lblk_t lblk, ext4_lblk_t len,
		     ext4_fsblk_t pblk)
{
	struct extent_status *es;
	es = kmem_cache_alloc(ext4_es_cachep, GFP_ATOMIC);
	if (es == NULL)
		return NULL;
	es->es_lblk = lblk;
	es->es_len = len;
	es->es_pblk = pblk;
	return es;
}

static void ext4_es_free_extent(struct inode *inode, struct extent_status *es)
{
	kmem_cache_free(ext4_es_cachep, es);
}

/*
 * Check whether or not two extents can be merged
 * Condition:
 *  - logical block number is contiguous
 *  - physical block number is contiguous
 *  - status is equal
 */
static int ext4_es_can_be_merged(struct extent_status *es1,
				 struct extent_status *es2)
{
	if (es1->es_lblk + es1->es_len != es2->es_lblk)
		return 0;

	if (ext4_es_status(es1) != ext4_es_status(es2))
		return 0;

	if ((ext4_es_is_written(es1) || ext4_es_is_unwritten(es1)) &&
	    (ext4_es_pblock(es1) + es1->es_len != ext4_es_pblock(es2)))
		return 0;

	return 1;
}

static struct extent_status *
ext4_es_try_to_merge_left(struct inode *inode, struct extent_status *es)
{
	struct ext4_es_tree *tree = &EXT4_I(inode)->i_es_tree;
	struct extent_status *es1;
	struct rb_node *node;

	node = rb_prev(&es->rb_node);
	if (!node)
		return es;

	es1 = rb_entry(node, struct extent_status, rb_node);
	if (ext4_es_can_be_merged(es1, es)) {
		es1->es_len += es->es_len;
		rb_erase(&es->rb_node, &tree->root);
		ext4_es_free_extent(inode, es);
		es = es1;
	}

	return es;
}

static struct extent_status *
ext4_es_try_to_merge_right(struct inode *inode, struct extent_status *es)
{
	struct ext4_es_tree *tree = &EXT4_I(inode)->i_es_tree;
	struct extent_status *es1;
	struct rb_node *node;

	node = rb_next(&es->rb_node);
	if (!node)
		return es;

	es1 = rb_entry(node, struct extent_status, rb_node);
	if (ext4_es_can_be_merged(es, es1)) {
		es->es_len += es1->es_len;
		rb_erase(node, &tree->root);
		ext4_es_free_extent(inode, es1);
	}

	return es;
}

static int __es_insert_extent(struct inode *inode, struct extent_status *newes)
{
	struct ext4_es_tree *tree = &EXT4_I(inode)->i_es_tree;
	struct rb_node **p = &tree->root.rb_node;
	struct rb_node *parent = NULL;
	struct extent_status *es;

	while (*p) {
		parent = *p;
		es = rb_entry(parent, struct extent_status, rb_node);

		if (newes->es_lblk < es->es_lblk) {
			if (ext4_es_can_be_merged(newes, es)) {
				/*
				 * Here we can modify es_lblk directly
				 * because it isn't overlapped.
				 */
				es->es_lblk = newes->es_lblk;
				es->es_len += newes->es_len;
				if (ext4_es_is_written(es) ||
				    ext4_es_is_unwritten(es))
					ext4_es_store_pblock(es,
							     newes->es_pblk);
				es = ext4_es_try_to_merge_left(inode, es);
				goto out;
			}
			p = &(*p)->rb_left;
		} else if (newes->es_lblk > ext4_es_end(es)) {
			if (ext4_es_can_be_merged(es, newes)) {
				es->es_len += newes->es_len;
				es = ext4_es_try_to_merge_right(inode, es);
				goto out;
			}
			p = &(*p)->rb_right;
		} else {
			BUG_ON(1);
			return -EINVAL;
		}
	}

	es = ext4_es_alloc_extent(inode, newes->es_lblk, newes->es_len,
				  newes->es_pblk);
	if (!es)
		return -ENOMEM;
	rb_link_node(&es->rb_node, parent, p);
	rb_insert_color(&es->rb_node, &tree->root);

out:
	tree->cache_es = es;
	return 0;
}

/*
 * ext4_es_insert_extent() adds a space to a extent status tree.
 *
 * ext4_es_insert_extent is called by ext4_da_write_begin and
 * ext4_es_remove_extent.
 *
 * Return 0 on success, error code on failure.
 */
int ext4_es_insert_extent(struct inode *inode, ext4_lblk_t lblk,
			  ext4_lblk_t len, ext4_fsblk_t pblk,
			  unsigned long long status)
{
	struct extent_status newes;
	ext4_lblk_t end = lblk + len - 1;
	int err = 0;

	es_debug("add [%u/%u) %llu %llx to extent status tree of inode %lu\n",
		 lblk, len, pblk, status, inode->i_ino);

	BUG_ON(end < lblk);

	newes.es_lblk = lblk;
	newes.es_len = len;
	ext4_es_store_pblock(&newes, pblk);
	ext4_es_store_status(&newes, status);
	trace_ext4_es_insert_extent(inode, &newes);

	write_lock(&EXT4_I(inode)->i_es_lock);
	err = __es_remove_extent(inode, lblk, end);
	if (err != 0)
		goto error;
	err = __es_insert_extent(inode, &newes);

error:
	write_unlock(&EXT4_I(inode)->i_es_lock);

	ext4_es_print_tree(inode);

	return err;
}

/*
 * ext4_es_lookup_extent() looks up an extent in extent status tree.
 *
 * ext4_es_lookup_extent is called by ext4_map_blocks/ext4_da_map_blocks.
 *
 * Return: 1 on found, 0 on not
 */
int ext4_es_lookup_extent(struct inode *inode, ext4_lblk_t lblk,
			  struct extent_status *es)
{
	struct ext4_es_tree *tree;
	struct extent_status *es1 = NULL;
	struct rb_node *node;
	int found = 0;

	trace_ext4_es_lookup_extent_enter(inode, lblk);
	es_debug("lookup extent in block %u\n", lblk);

	tree = &EXT4_I(inode)->i_es_tree;
	read_lock(&EXT4_I(inode)->i_es_lock);

	/* find extent in cache firstly */
	es->es_lblk = es->es_len = es->es_pblk = 0;
	if (tree->cache_es) {
		es1 = tree->cache_es;
		if (in_range(lblk, es1->es_lblk, es1->es_len)) {
			es_debug("%u cached by [%u/%u)\n",
				 lblk, es1->es_lblk, es1->es_len);
			found = 1;
			goto out;
		}
	}

	node = tree->root.rb_node;
	while (node) {
		es1 = rb_entry(node, struct extent_status, rb_node);
		if (lblk < es1->es_lblk)
			node = node->rb_left;
		else if (lblk > ext4_es_end(es1))
			node = node->rb_right;
		else {
			found = 1;
			break;
		}
	}

out:
	if (found) {
		BUG_ON(!es1);
		es->es_lblk = es1->es_lblk;
		es->es_len = es1->es_len;
		es->es_pblk = es1->es_pblk;
	}

	read_unlock(&EXT4_I(inode)->i_es_lock);

	trace_ext4_es_lookup_extent_exit(inode, es, found);
	return found;
}

static int __es_remove_extent(struct inode *inode, ext4_lblk_t lblk,
			      ext4_lblk_t end)
{
	struct ext4_es_tree *tree = &EXT4_I(inode)->i_es_tree;
	struct rb_node *node;
	struct extent_status *es;
	struct extent_status orig_es;
	ext4_lblk_t len1, len2;
	ext4_fsblk_t block;
	int err = 0;

	es = __es_tree_search(&tree->root, lblk);
	if (!es)
		goto out;
	if (es->es_lblk > end)
		goto out;

	/* Simply invalidate cache_es. */
	tree->cache_es = NULL;

	orig_es.es_lblk = es->es_lblk;
	orig_es.es_len = es->es_len;
	orig_es.es_pblk = es->es_pblk;

	len1 = lblk > es->es_lblk ? lblk - es->es_lblk : 0;
	len2 = ext4_es_end(es) > end ? ext4_es_end(es) - end : 0;
	if (len1 > 0)
		es->es_len = len1;
	if (len2 > 0) {
		if (len1 > 0) {
			struct extent_status newes;

			newes.es_lblk = end + 1;
			newes.es_len = len2;
			if (ext4_es_is_written(&orig_es) ||
			    ext4_es_is_unwritten(&orig_es)) {
				block = ext4_es_pblock(&orig_es) +
					orig_es.es_len - len2;
				ext4_es_store_pblock(&newes, block);
			}
			ext4_es_store_status(&newes, ext4_es_status(&orig_es));
			err = __es_insert_extent(inode, &newes);
			if (err) {
				es->es_lblk = orig_es.es_lblk;
				es->es_len = orig_es.es_len;
				goto out;
			}
		} else {
			es->es_lblk = end + 1;
			es->es_len = len2;
			if (ext4_es_is_written(es) ||
			    ext4_es_is_unwritten(es)) {
				block = orig_es.es_pblk + orig_es.es_len - len2;
				ext4_es_store_pblock(es, block);
			}
		}
		goto out;
	}

	if (len1 > 0) {
		node = rb_next(&es->rb_node);
		if (node)
			es = rb_entry(node, struct extent_status, rb_node);
		else
			es = NULL;
	}

	while (es && ext4_es_end(es) <= end) {
		node = rb_next(&es->rb_node);
		rb_erase(&es->rb_node, &tree->root);
		ext4_es_free_extent(inode, es);
		if (!node) {
			es = NULL;
			break;
		}
		es = rb_entry(node, struct extent_status, rb_node);
	}

	if (es && es->es_lblk < end + 1) {
		ext4_lblk_t orig_len = es->es_len;

		len1 = ext4_es_end(es) - end;
		es->es_lblk = end + 1;
		es->es_len = len1;
		if (ext4_es_is_written(es) || ext4_es_is_unwritten(es)) {
			block = es->es_pblk + orig_len - len1;
			ext4_es_store_pblock(es, block);
		}
	}

out:
	return err;
}

/*
 * ext4_es_remove_extent() removes a space from a extent status tree.
 *
 * Return 0 on success, error code on failure.
 */
int ext4_es_remove_extent(struct inode *inode, ext4_lblk_t lblk,
			  ext4_lblk_t len)
{
	ext4_lblk_t end;
	int err = 0;

	trace_ext4_es_remove_extent(inode, lblk, len);
	es_debug("remove [%u/%u) from extent status tree of inode %lu\n",
		 lblk, len, inode->i_ino);

	end = lblk + len - 1;
	BUG_ON(end < lblk);

	write_lock(&EXT4_I(inode)->i_es_lock);
	err = __es_remove_extent(inode, lblk, end);
	write_unlock(&EXT4_I(inode)->i_es_lock);
	ext4_es_print_tree(inode);
	return err;
}
