/*
 *  linux/fs/ext4/file.c
 *
 * Copyright (C) 1992, 1993, 1994, 1995
 * Remy Card (card@masi.ibp.fr)
 * Laboratoire MASI - Institut Blaise Pascal
 * Universite Pierre et Marie Curie (Paris VI)
 *
 *  from
 *
 *  linux/fs/minix/file.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 *  ext4 fs regular file handling primitives
 *
 *  64-bit file support on 64-bit platforms by Jakub Jelinek
 *	(jj@sunsite.ms.mff.cuni.cz)
 */

#include <linux/time.h>
#include <linux/fs.h>
#include <linux/jbd2.h>
#include <linux/mount.h>
#include <linux/path.h>
#include <linux/aio.h>
#include <linux/quotaops.h>
#include <linux/pagevec.h>
#include "ext4.h"
#include "ext4_jbd2.h"
#include "xattr.h"
#include "acl.h"

/*
 * Called when an inode is released. Note that this is different
 * from ext4_file_open: open gets called at every open, but release
 * gets called only when /all/ the files are closed.
 */
static int ext4_release_file(struct inode *inode, struct file *filp)
{
	if (ext4_test_inode_state(inode, EXT4_STATE_DA_ALLOC_CLOSE)) {
		ext4_alloc_da_blocks(inode);
		ext4_clear_inode_state(inode, EXT4_STATE_DA_ALLOC_CLOSE);
	}
	/* if we are the last writer on the inode, drop the block reservation */
	if ((filp->f_mode & FMODE_WRITE) &&
			(atomic_read(&inode->i_writecount) == 1) &&
		        !EXT4_I(inode)->i_reserved_data_blocks)
	{
		down_write(&EXT4_I(inode)->i_data_sem);
		ext4_discard_preallocations(inode);
		up_write(&EXT4_I(inode)->i_data_sem);
	}
	if (is_dx(inode) && filp->private_data)
		ext4_htree_free_dir_info(filp->private_data);

	return 0;
}

void ext4_unwritten_wait(struct inode *inode)
{
	wait_queue_head_t *wq = ext4_ioend_wq(inode);

	wait_event(*wq, (atomic_read(&EXT4_I(inode)->i_unwritten) == 0));
}

/*
 * This tests whether the IO in question is block-aligned or not.
 * Ext4 utilizes unwritten extents when hole-filling during direct IO, and they
 * are converted to written only after the IO is complete.  Until they are
 * mapped, these blocks appear as holes, so dio_zero_block() will assume that
 * it needs to zero out portions of the start and/or end block.  If 2 AIO
 * threads are at work on the same unwritten block, they must be synchronized
 * or one thread will zero the other's data, causing corruption.
 */
static int
ext4_unaligned_aio(struct inode *inode, struct iov_iter *iter, loff_t pos)
{
	struct super_block *sb = inode->i_sb;
	int blockmask = sb->s_blocksize - 1;
	size_t count = iov_iter_count(iter);
	loff_t final_size = pos + count;

	if (pos >= i_size_read(inode))
		return 0;

	if ((pos & blockmask) || (final_size & blockmask))
		return 1;

	return 0;
}

static ssize_t
ext4_file_write_iter(struct kiocb *iocb, struct iov_iter *iter, loff_t pos)
{
	struct inode *inode = file_inode(iocb->ki_filp);
	int unaligned_aio = 0;
	ssize_t ret;

	/*
	 * If we have encountered a bitmap-format file, the size limit
	 * is smaller than s_maxbytes, which is for extent-mapped files.
	 */

	if (!(ext4_test_inode_flag(inode, EXT4_INODE_EXTENTS))) {
		struct ext4_sb_info *sbi = EXT4_SB(inode->i_sb);
		size_t length = iov_iter_count(iter);

		if ((pos > sbi->s_bitmap_maxbytes ||
		    (pos == sbi->s_bitmap_maxbytes && length > 0)))
			return -EFBIG;

		if (pos + length > sbi->s_bitmap_maxbytes) {
			ret = iov_iter_shorten(iter,
					       sbi->s_bitmap_maxbytes - pos);
			if (ret)
				return ret;
		}
	} else if (unlikely((iocb->ki_filp->f_flags & O_DIRECT) &&
		   !is_sync_kiocb(iocb))) {
		unaligned_aio = ext4_unaligned_aio(inode, iter, pos);
	}

	/* Unaligned direct AIO must be serialized; see comment above */
	if (unaligned_aio) {
		mutex_lock(ext4_aio_mutex(inode));
		ext4_unwritten_wait(inode);
	}

	ret = generic_file_write_iter(iocb, iter, pos);

	if (unaligned_aio)
		mutex_unlock(ext4_aio_mutex(inode));

	return ret;
}

static ssize_t
ext4_file_write(struct kiocb *iocb, const struct iovec *iov,
		unsigned long nr_segs, loff_t pos)
{
	struct inode *inode = iocb->ki_filp->f_path.dentry->d_inode;
	ssize_t ret;

	/*
	 * If we have encountered a bitmap-format file, the size limit
	 * is smaller than s_maxbytes, which is for extent-mapped files.
	 */

	if (!(ext4_test_inode_flag(inode, EXT4_INODE_EXTENTS))) {
		struct ext4_sb_info *sbi = EXT4_SB(inode->i_sb);
		size_t length = iov_length(iov, nr_segs);

		if ((pos > sbi->s_bitmap_maxbytes ||
		    (pos == sbi->s_bitmap_maxbytes && length > 0)))
			return -EFBIG;

		if (pos + length > sbi->s_bitmap_maxbytes) {
			nr_segs = iov_shorten((struct iovec *)iov, nr_segs,
					      sbi->s_bitmap_maxbytes - pos);
		}
	}

	if (unlikely(iocb->ki_filp->f_flags & O_DIRECT))
		ret = ext4_file_dio_write(iocb, iov, nr_segs, pos);
	else
		ret = generic_file_aio_write(iocb, iov, nr_segs, pos);

	return ret;
}

static const struct vm_operations_struct ext4_file_vm_ops = {
	.fault		= filemap_fault,
	.page_mkwrite   = ext4_page_mkwrite,
	.remap_pages	= generic_file_remap_pages,
};

static int ext4_file_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct address_space *mapping = file->f_mapping;

	if (!mapping->a_ops->readpage)
		return -ENOEXEC;
	file_accessed(file);
	vma->vm_ops = &ext4_file_vm_ops;
	return 0;
}

static int ext4_file_open(struct inode * inode, struct file * filp)
{
	struct super_block *sb = inode->i_sb;
	struct ext4_sb_info *sbi = EXT4_SB(inode->i_sb);
	struct ext4_inode_info *ei = EXT4_I(inode);
	struct vfsmount *mnt = filp->f_path.mnt;
	struct path path;
	char buf[64], *cp;

	if (unlikely(!(sbi->s_mount_flags & EXT4_MF_MNTDIR_SAMPLED) &&
		     !(sb->s_flags & MS_RDONLY))) {
		sbi->s_mount_flags |= EXT4_MF_MNTDIR_SAMPLED;
		/*
		 * Sample where the filesystem has been mounted and
		 * store it in the superblock for sysadmin convenience
		 * when trying to sort through large numbers of block
		 * devices or filesystem images.
		 */
		memset(buf, 0, sizeof(buf));
		path.mnt = mnt;
		path.dentry = mnt->mnt_root;
		cp = d_path(&path, buf, sizeof(buf));
		if (!IS_ERR(cp)) {
			handle_t *handle;
			int err;

			handle = ext4_journal_start_sb(sb, 1);
			if (IS_ERR(handle))
				return PTR_ERR(handle);
			err = ext4_journal_get_write_access(handle, sbi->s_sbh);
			if (err) {
				ext4_journal_stop(handle);
				return err;
			}
			strlcpy(sbi->s_es->s_last_mounted, cp,
				sizeof(sbi->s_es->s_last_mounted));
			ext4_handle_dirty_super(handle, sb);
			ext4_journal_stop(handle);
		}
	}
	/*
	 * Set up the jbd2_inode if we are opening the inode for
	 * writing and the journal is present
	 */
	if (sbi->s_journal && !ei->jinode && (filp->f_mode & FMODE_WRITE)) {
		struct jbd2_inode *jinode = jbd2_alloc_inode(GFP_KERNEL);

		spin_lock(&inode->i_lock);
		if (!ei->jinode) {
			if (!jinode) {
				spin_unlock(&inode->i_lock);
				return -ENOMEM;
			}
			ei->jinode = jinode;
			jbd2_journal_init_jbd_inode(ei->jinode, inode);
			jinode = NULL;
		}
		spin_unlock(&inode->i_lock);
		if (unlikely(jinode != NULL))
			jbd2_free_inode(jinode);
	}
	return dquot_file_open(inode, filp);
}

/*
 * Here we use ext4_map_blocks() to get a block mapping for a extent-based
 * file rather than ext4_ext_walk_space() because we can introduce
 * SEEK_DATA/SEEK_HOLE for block-mapped and extent-mapped file at the same
 * function.  When extent status tree has been fully implemented, it will
 * track all extent status for a file and we can directly use it to
 * retrieve the offset for SEEK_DATA/SEEK_HOLE.
 */

/*
 * When we retrieve the offset for SEEK_DATA/SEEK_HOLE, we would need to
 * lookup page cache to check whether or not there has some data between
 * [startoff, endoff] because, if this range contains an unwritten extent,
 * we determine this extent as a data or a hole according to whether the
 * page cache has data or not.
 */
static int ext4_find_unwritten_pgoff(struct inode *inode,
				     int whence,
				     struct ext4_map_blocks *map,
				     loff_t *offset)
{
	struct pagevec pvec;
	unsigned int blkbits;
	pgoff_t index;
	pgoff_t end;
	loff_t endoff;
	loff_t startoff;
	loff_t lastoff;
	int found = 0;

	blkbits = inode->i_sb->s_blocksize_bits;
	startoff = *offset;
	lastoff = startoff;
	endoff = (loff_t)(map->m_lblk + map->m_len) << blkbits;

	index = startoff >> PAGE_CACHE_SHIFT;
	end = endoff >> PAGE_CACHE_SHIFT;

	pagevec_init(&pvec, 0);
	do {
		int i, num;
		unsigned long nr_pages;

		num = min_t(pgoff_t, end - index, PAGEVEC_SIZE);
		nr_pages = pagevec_lookup(&pvec, inode->i_mapping, index,
					  (pgoff_t)num);
		if (nr_pages == 0) {
			if (whence == SEEK_DATA)
				break;

			BUG_ON(whence != SEEK_HOLE);
			/*
			 * If this is the first time to go into the loop and
			 * offset is not beyond the end offset, it will be a
			 * hole at this offset
			 */
			if (lastoff == startoff || lastoff < endoff)
				found = 1;
			break;
		}

		/*
		 * If this is the first time to go into the loop and
		 * offset is smaller than the first page offset, it will be a
		 * hole at this offset.
		 */
		if (lastoff == startoff && whence == SEEK_HOLE &&
		    lastoff < page_offset(pvec.pages[0])) {
			found = 1;
			break;
		}

		for (i = 0; i < nr_pages; i++) {
			struct page *page = pvec.pages[i];
			struct buffer_head *bh, *head;

			/*
			 * If the current offset is not beyond the end of given
			 * range, it will be a hole.
			 */
			if (lastoff < endoff && whence == SEEK_HOLE &&
			    page->index > end) {
				found = 1;
				*offset = lastoff;
				goto out;
			}

			lock_page(page);

			if (unlikely(page->mapping != inode->i_mapping)) {
				unlock_page(page);
				continue;
			}

			if (!page_has_buffers(page)) {
				unlock_page(page);
				continue;
			}

			if (page_has_buffers(page)) {
				lastoff = page_offset(page);
				bh = head = page_buffers(page);
				do {
					if (buffer_uptodate(bh) ||
					    buffer_unwritten(bh)) {
						if (whence == SEEK_DATA)
							found = 1;
					} else {
						if (whence == SEEK_HOLE)
							found = 1;
					}
					if (found) {
						*offset = max_t(loff_t,
							startoff, lastoff);
						unlock_page(page);
						goto out;
					}
					lastoff += bh->b_size;
					bh = bh->b_this_page;
				} while (bh != head);
			}

			lastoff = page_offset(page) + PAGE_SIZE;
			unlock_page(page);
		}

		/*
		 * The no. of pages is less than our desired, that would be a
		 * hole in there.
		 */
		if (nr_pages < num && whence == SEEK_HOLE) {
			found = 1;
			*offset = lastoff;
			break;
		}

		index = pvec.pages[i - 1]->index + 1;
		pagevec_release(&pvec);
	} while (index <= end);

out:
	pagevec_release(&pvec);
	return found;
}

/*
 * ext4_seek_data() retrieves the offset for SEEK_DATA.
 */
static loff_t ext4_seek_data(struct file *file, loff_t offset, loff_t maxsize)
{
	struct inode *inode = file->f_mapping->host;
	struct ext4_map_blocks map;
	struct extent_status es;
	ext4_lblk_t start, last, end;
	loff_t dataoff, isize;
	int blkbits;
	int ret = 0;

	mutex_lock(&inode->i_mutex);

	isize = i_size_read(inode);
	if (offset >= isize) {
		mutex_unlock(&inode->i_mutex);
		return -ENXIO;
	}

	blkbits = inode->i_sb->s_blocksize_bits;
	start = offset >> blkbits;
	last = start;
	end = isize >> blkbits;
	dataoff = offset;

	do {
		map.m_lblk = last;
		map.m_len = end - last + 1;
		ret = ext4_map_blocks(NULL, inode, &map, 0);
		if (ret > 0 && !(map.m_flags & EXT4_MAP_UNWRITTEN)) {
			if (last != start)
				dataoff = (loff_t)last << blkbits;
			break;
		}

		/*
		 * If there is a delay extent at this offset,
		 * it will be as a data.
		 */
		es.start = last;
		(void)ext4_es_find_extent(inode, &es);
		if (last >= es.start &&
		    last < es.start + es.len) {
			if (last != start)
				dataoff = (loff_t)last << blkbits;
			break;
		}

		/*
		 * If there is a unwritten extent at this offset,
		 * it will be as a data or a hole according to page
		 * cache that has data or not.
		 */
		if (map.m_flags & EXT4_MAP_UNWRITTEN) {
			int unwritten;
			unwritten = ext4_find_unwritten_pgoff(inode, SEEK_DATA,
							      &map, &dataoff);
			if (unwritten)
				break;
		}

		last++;
		dataoff = (loff_t)last << blkbits;
	} while (last <= end);

	mutex_unlock(&inode->i_mutex);

	if (dataoff > isize)
		return -ENXIO;

	if (dataoff < 0 && !(file->f_mode & FMODE_UNSIGNED_OFFSET))
		return -EINVAL;
	if (dataoff > maxsize)
		return -EINVAL;

	if (dataoff != file->f_pos) {
		file->f_pos = dataoff;
		file->f_version = 0;
	}

	return dataoff;
}

/*
 * ext4_seek_hole() retrieves the offset for SEEK_HOLE.
 */
static loff_t ext4_seek_hole(struct file *file, loff_t offset, loff_t maxsize)
{
	struct inode *inode = file->f_mapping->host;
	struct ext4_map_blocks map;
	struct extent_status es;
	ext4_lblk_t start, last, end;
	loff_t holeoff, isize;
	int blkbits;
	int ret = 0;

	mutex_lock(&inode->i_mutex);

	isize = i_size_read(inode);
	if (offset >= isize) {
		mutex_unlock(&inode->i_mutex);
		return -ENXIO;
	}

	blkbits = inode->i_sb->s_blocksize_bits;
	start = offset >> blkbits;
	last = start;
	end = isize >> blkbits;
	holeoff = offset;

	do {
		map.m_lblk = last;
		map.m_len = end - last + 1;
		ret = ext4_map_blocks(NULL, inode, &map, 0);
		if (ret > 0 && !(map.m_flags & EXT4_MAP_UNWRITTEN)) {
			last += ret;
			holeoff = (loff_t)last << blkbits;
			continue;
		}

		/*
		 * If there is a delay extent at this offset,
		 * we will skip this extent.
		 */
		es.start = last;
		(void)ext4_es_find_extent(inode, &es);
		if (last >= es.start &&
		    last < es.start + es.len) {
			last = es.start + es.len;
			holeoff = (loff_t)last << blkbits;
			continue;
		}

		/*
		 * If there is a unwritten extent at this offset,
		 * it will be as a data or a hole according to page
		 * cache that has data or not.
		 */
		if (map.m_flags & EXT4_MAP_UNWRITTEN) {
			int unwritten;
			unwritten = ext4_find_unwritten_pgoff(inode, SEEK_HOLE,
							      &map, &holeoff);
			if (!unwritten) {
				last += ret;
				holeoff = (loff_t)last << blkbits;
				continue;
			}
		}

		/* find a hole */
		break;
	} while (last <= end);

	mutex_unlock(&inode->i_mutex);

	if (holeoff > isize)
		holeoff = isize;

	if (holeoff < 0 && !(file->f_mode & FMODE_UNSIGNED_OFFSET))
		return -EINVAL;
	if (holeoff > maxsize)
		return -EINVAL;

	if (holeoff != file->f_pos) {
		file->f_pos = holeoff;
		file->f_version = 0;
	}

	return holeoff;
}

/*
 * ext4_llseek() handles both block-mapped and extent-mapped maxbytes values
 * by calling generic_file_llseek_size() with the appropriate maxbytes
 * value for each.
 */
loff_t ext4_llseek(struct file *file, loff_t offset, int whence)
{
	struct inode *inode = file->f_mapping->host;
	loff_t maxbytes;

	if (!(ext4_test_inode_flag(inode, EXT4_INODE_EXTENTS)))
		maxbytes = EXT4_SB(inode->i_sb)->s_bitmap_maxbytes;
	else
		maxbytes = inode->i_sb->s_maxbytes;

	switch (whence) {
	case SEEK_SET:
	case SEEK_CUR:
	case SEEK_END:
		return generic_file_llseek_size(file, offset, whence,
						maxbytes, i_size_read(inode));
	case SEEK_DATA:
		return ext4_seek_data(file, offset, maxbytes);
	case SEEK_HOLE:
		return ext4_seek_hole(file, offset, maxbytes);
	}

	return -EINVAL;
}

const struct file_operations ext4_file_operations = {
	.llseek		= ext4_llseek,
	.read		= do_sync_read,
	.write		= do_sync_write,
	.read_iter	= generic_file_read_iter,
	.write_iter	= ext4_file_write_iter,
	.unlocked_ioctl = ext4_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= ext4_compat_ioctl,
#endif
	.mmap		= ext4_file_mmap,
	.open		= ext4_file_open,
	.release	= ext4_release_file,
	.fsync		= ext4_sync_file,
	.splice_read	= generic_file_splice_read,
	.splice_write	= generic_file_splice_write,
	.fallocate	= ext4_fallocate,
};

const struct inode_operations ext4_file_inode_operations = {
	.setattr	= ext4_setattr,
	.getattr	= ext4_getattr,
#ifdef CONFIG_EXT4_FS_XATTR
	.setxattr	= generic_setxattr,
	.getxattr	= generic_getxattr,
	.listxattr	= ext4_listxattr,
	.removexattr	= generic_removexattr,
#endif
	.get_acl	= ext4_get_acl,
	.fiemap		= ext4_fiemap,
};

