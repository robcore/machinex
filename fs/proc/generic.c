/*
 * proc/fs/generic.c --- generic routines for the proc-fs
 *
 * This file contains generic proc-fs routines for handling
 * directories and files.
 * 
 * Copyright (C) 1991, 1992 Linus Torvalds.
 * Copyright (C) 1997 Theodore Ts'o
 */

#include <linux/errno.h>
#include <linux/time.h>
#include <linux/proc_fs.h>
#include <linux/stat.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/printk.h>
#include <linux/mount.h>
#include <linux/init.h>
#include <linux/idr.h>
#include <linux/namei.h>
#include <linux/bitops.h>
#include <linux/spinlock.h>
#include <linux/completion.h>
#include <asm/uaccess.h>

#include "internal.h"

DEFINE_SPINLOCK(proc_subdir_lock);

static int proc_match(unsigned int len, const char *name, struct proc_dir_entry *de)
{
	if (len < de->namelen)
		return -1;
	if (len > de->namelen)
		return 1;

	return memcmp(name, de->name, len);
}

static struct proc_dir_entry *pde_subdir_first(struct proc_dir_entry *dir)
{
	struct rb_node *node = rb_first(&dir->subdir);

	if (node == NULL)
		return NULL;

	return rb_entry(node, struct proc_dir_entry, subdir_node);
}

static struct proc_dir_entry *pde_subdir_next(struct proc_dir_entry *dir)
{
	struct rb_node *node = rb_next(&dir->subdir_node);

	if (node == NULL)
		return NULL;

	return rb_entry(node, struct proc_dir_entry, subdir_node);
}

static struct proc_dir_entry *pde_subdir_find(struct proc_dir_entry *dir,
					      const char *name,
					      unsigned int len)
{
	struct rb_node *node = dir->subdir.rb_node;

	while (node) {
		struct proc_dir_entry *de = container_of(node,
							 struct proc_dir_entry,
							 subdir_node);
		int result = proc_match(len, name, de);

		if (result < 0)
			node = node->rb_left;
		else if (result > 0)
			node = node->rb_right;
		else
			return de;
	}
	return NULL;
}

static bool pde_subdir_insert(struct proc_dir_entry *dir,
			      struct proc_dir_entry *de)
{
	struct rb_root *root = &dir->subdir;
	struct rb_node **new = &root->rb_node, *parent = NULL;

	/* Figure out where to put new node */
	while (*new) {
		struct proc_dir_entry *this =
			container_of(*new, struct proc_dir_entry, subdir_node);
		int result = proc_match(de->namelen, de->name, this);

		parent = *new;
		if (result < 0)
			new = &(*new)->rb_left;
		else if (result > 0)
			new = &(*new)->rb_right;
		else
			return false;
	}

	/* Add new node and rebalance tree. */
	rb_link_node(&de->subdir_node, parent, new);
	rb_insert_color(&de->subdir_node, root);
	return true;
}

/* buffer size is one page but our output routines use some slack for overruns */
#define PROC_BLOCK_SIZE	(PAGE_SIZE - 1024)

ssize_t
__proc_file_read(struct file *file, char __user *buf, size_t nbytes,
	       loff_t *ppos)
{
	struct inode * inode = file_inode(file);
	char 	*page;
	ssize_t	retval=0;
	int	eof=0;
	ssize_t	n, count;
	char	*start;
	struct proc_dir_entry * dp;
	unsigned long long pos;

	/*
	 * Gaah, please just use "seq_file" instead. The legacy /proc
	 * interfaces cut loff_t down to off_t for reads, and ignore
	 * the offset entirely for writes..
	 */
	pos = *ppos;
	if (pos > MAX_NON_LFS)
		return 0;
	if (nbytes > MAX_NON_LFS - pos)
		nbytes = MAX_NON_LFS - pos;

	dp = PDE(inode);
	if (!(page = (char*) __get_free_page(GFP_TEMPORARY)))
		return -ENOMEM;

	while ((nbytes > 0) && !eof) {
		count = min_t(size_t, PROC_BLOCK_SIZE, nbytes);

		start = NULL;
		if (!dp->read_proc)
			break;

		/* How to be a proc read function
		 * ------------------------------
		 * Prototype:
		 *    int f(char *buffer, char **start, off_t offset,
		 *          int count, int *peof, void *dat)
		 *
		 * Assume that the buffer is "count" bytes in size.
		 *
		 * If you know you have supplied all the data you have, set
		 * *peof.
		 *
		 * You have three ways to return data:
		 *
		 * 0) Leave *start = NULL.  (This is the default.)  Put the
		 *    data of the requested offset at that offset within the
		 *    buffer.  Return the number (n) of bytes there are from
		 *    the beginning of the buffer up to the last byte of data.
		 *    If the number of supplied bytes (= n - offset) is greater
		 *    than zero and you didn't signal eof and the reader is
		 *    prepared to take more data you will be called again with
		 *    the requested offset advanced by the number of bytes
		 *    absorbed.  This interface is useful for files no larger
		 *    than the buffer.
		 *
		 * 1) Set *start = an unsigned long value less than the buffer
		 *    address but greater than zero.  Put the data of the
		 *    requested offset at the beginning of the buffer.  Return
		 *    the number of bytes of data placed there.  If this number
		 *    is greater than zero and you didn't signal eof and the
		 *    reader is prepared to take more data you will be called
		 *    again with the requested offset advanced by *start.  This
		 *    interface is useful when you have a large file consisting
		 *    of a series of blocks which you want to count and return
		 *    as wholes.
		 *    (Hack by Paul.Russell@rustcorp.com.au)
		 *
		 * 2) Set *start = an address within the buffer.  Put the data
		 *    of the requested offset at *start.  Return the number of
		 *    bytes of data placed there.  If this number is greater
		 *    than zero and you didn't signal eof and the reader is
		 *    prepared to take more data you will be called again with
		 *    the requested offset advanced by the number of bytes
		 *    absorbed.
		 */
		n = dp->read_proc(page, &start, *ppos, count, &eof, dp->data);

		if (n == 0)   /* end of file */
			break;
		if (n < 0) {  /* error */
			if (retval == 0)
				retval = n;
			break;
		}

		if (start == NULL) {
			if (n > PAGE_SIZE)	/* Apparent buffer overflow */
				n = PAGE_SIZE;
			n -= *ppos;
			if (n <= 0)
				break;
			if (n > count)
				n = count;
			start = page + *ppos;
		} else if (start < page) {
			if (n > PAGE_SIZE)	/* Apparent buffer overflow */
				n = PAGE_SIZE;
			if (n > count) {
				/*
				 * Don't reduce n because doing so might
				 * cut off part of a data block.
				 */
				pr_warn("proc_file_read: count exceeded\n");
			}
		} else /* start >= page */ {
			unsigned long startoff = (unsigned long)(start - page);
			if (n > (PAGE_SIZE - startoff))	/* buffer overflow? */
				n = PAGE_SIZE - startoff;
			if (n > count)
				n = count;
		}
		
 		n -= copy_to_user(buf, start < page ? page : start, n);
		if (n == 0) {
			if (retval == 0)
				retval = -EFAULT;
			break;
		}

		*ppos += start < page ? (unsigned long)start : n;
		nbytes -= n;
		buf += n;
		retval += n;
	}
	free_page((unsigned long) page);
	return retval;
}

static int proc_notify_change(struct dentry *dentry, struct iattr *iattr)
{
	struct inode *inode = dentry->d_inode;
	struct proc_dir_entry *de = PDE(inode);
	int error;

	error = inode_change_ok(inode, iattr);
	if (error)
		return error;

	setattr_copy(inode, iattr);
	mark_inode_dirty(inode);

	proc_set_user(de, inode->i_uid, inode->i_gid);
	de->mode = inode->i_mode;
	return 0;
}

static int proc_getattr(struct vfsmount *mnt, struct dentry *dentry,
			struct kstat *stat)
{
	struct inode *inode = dentry->d_inode;
	struct proc_dir_entry *de = PROC_I(inode)->pde;
	if (de && de->nlink)
		set_nlink(inode, de->nlink);

	generic_fillattr(inode, stat);
	return 0;
}

static const struct inode_operations proc_file_inode_operations = {
	.setattr	= proc_notify_change,
};

/*
 * This function parses a name such as "tty/driver/serial", and
 * returns the struct proc_dir_entry for "/proc/tty/driver", and
 * returns "serial" in residual.
 */
static int __xlate_proc_name(const char *name, struct proc_dir_entry **ret,
			     const char **residual)
{
	const char     		*cp = name, *next;
	struct proc_dir_entry	*de;
	unsigned int		len;

	de = *ret;
	if (!de)
		de = &proc_root;

	while (1) {
		next = strchr(cp, '/');
		if (!next)
			break;

		len = next - cp;
		de = pde_subdir_find(de, cp, len);
		if (!de) {
			WARN(1, "name '%s'\n", name);
			return -ENOENT;
		}
		cp += len + 1;
	}
	*residual = cp;
	*ret = de;
	return 0;
}

static int xlate_proc_name(const char *name, struct proc_dir_entry **ret,
			   const char **residual)
{
	int rv;

	spin_lock(&proc_subdir_lock);
	rv = __xlate_proc_name(name, ret, residual);
	spin_unlock(&proc_subdir_lock);
	return rv;
}

static DEFINE_IDA(proc_inum_ida);
static DEFINE_SPINLOCK(proc_inum_lock); /* protects the above */

#define PROC_DYNAMIC_FIRST 0xF0000000U

/*
 * Return an inode number between PROC_DYNAMIC_FIRST and
 * 0xffffffff, or zero on failure.
 */
int proc_alloc_inum(unsigned int *inum)
{
	unsigned int i;
	int error;

retry:
	if (!ida_pre_get(&proc_inum_ida, GFP_KERNEL))
		return -ENOMEM;

	spin_lock_irq(&proc_inum_lock);
	error = ida_get_new(&proc_inum_ida, &i);
	spin_unlock_irq(&proc_inum_lock);
	if (error == -EAGAIN)
		goto retry;
	else if (error)
		return error;

	if (i > UINT_MAX - PROC_DYNAMIC_FIRST) {
		spin_lock_irq(&proc_inum_lock);
		ida_remove(&proc_inum_ida, i);
		spin_unlock_irq(&proc_inum_lock);
		return -ENOSPC;
	}
	*inum = PROC_DYNAMIC_FIRST + i;
	return 0;
}

void proc_free_inum(unsigned int inum)
{
	unsigned long flags;
	spin_lock_irqsave(&proc_inum_lock, flags);
	ida_remove(&proc_inum_ida, inum - PROC_DYNAMIC_FIRST);
	spin_unlock_irqrestore(&proc_inum_lock, flags);
}

static void *proc_follow_link(struct dentry *dentry, struct nameidata *nd)
{
	nd_set_link(nd, __PDE_DATA(dentry->d_inode));
	return NULL;
}

static const struct inode_operations proc_link_inode_operations = {
	.readlink	= generic_readlink,
	.follow_link	= proc_follow_link,
};

/*
 * Don't create negative dentries here, return -ENOENT by hand
 * instead.
 */
struct dentry *proc_lookup_de(struct proc_dir_entry *de, struct inode *dir,
		struct dentry *dentry)
{
	struct inode *inode;

	spin_lock(&proc_subdir_lock);
	de = pde_subdir_find(de, dentry->d_name.name, dentry->d_name.len);
	if (de) {
		pde_get(de);
		spin_unlock(&proc_subdir_lock);
		inode = proc_get_inode(dir->i_sb, de);
		if (!inode)
			return ERR_PTR(-ENOMEM);
		d_set_d_op(dentry, &simple_dentry_operations);
		d_add(dentry, inode);
		return NULL;
	}
	spin_unlock(&proc_subdir_lock);
	return ERR_PTR(-ENOENT);
}

struct dentry *proc_lookup(struct inode *dir, struct dentry *dentry,
		unsigned int flags)
{
	return proc_lookup_de(PDE(dir), dir, dentry);
}

/*
 * This returns non-zero if at EOF, so that the /proc
 * root directory can use this and check if it should
 * continue with the <pid> entries..
 *
 * Note that the VFS-layer doesn't care about the return
 * value of the readdir() call, as long as it's non-negative
 * for success..
 */
int proc_readdir_de(struct proc_dir_entry *de, struct file *file,
		    struct dir_context *ctx)
{
	int i;

	if (!dir_emit_dots(file, ctx))
		return 0;

	spin_lock(&proc_subdir_lock);
	de = pde_subdir_first(de);
	i = ctx->pos - 2;
	for (;;) {
		if (!de) {
			spin_unlock(&proc_subdir_lock);
			return 0;
		}
		if (!i)
			break;
		de = pde_subdir_next(de);
		i--;
	}

	do {
		struct proc_dir_entry *next;
		pde_get(de);
		spin_unlock(&proc_subdir_lock);
		if (!dir_emit(ctx, de->name, de->namelen,
			    de->low_ino, de->mode >> 12)) {
			pde_put(de);
			return 0;
		}
		spin_lock(&proc_subdir_lock);
		ctx->pos++;
		next = pde_subdir_next(de);
		pde_put(de);
		de = next;
	} while (de);
	spin_unlock(&proc_subdir_lock);
	return 1;
}

int proc_readdir(struct file *file, struct dir_context *ctx)
{
	struct inode *inode = file_inode(file);

	return proc_readdir_de(PDE(inode), file, ctx);
}

/*
 * These are the generic /proc directory operations. They
 * use the in-memory "struct proc_dir_entry" tree to parse
 * the /proc directory.
 */
static const struct file_operations proc_dir_operations = {
	.llseek			= generic_file_llseek,
	.read			= generic_read_dir,
	.iterate		= proc_readdir,
};

/*
 * proc directories can do almost nothing..
 */
static const struct inode_operations proc_dir_inode_operations = {
	.lookup		= proc_lookup,
	.getattr	= proc_getattr,
	.setattr	= proc_notify_change,
};

static int proc_register(struct proc_dir_entry * dir, struct proc_dir_entry * dp)
{
	int ret;

	ret = proc_alloc_inum(&dp->low_ino);
	if (ret)
		return ret;

	if (S_ISDIR(dp->mode)) {
		dp->proc_fops = &proc_dir_operations;
		dp->proc_iops = &proc_dir_inode_operations;
		dir->nlink++;
	} else if (S_ISLNK(dp->mode)) {
		dp->proc_iops = &proc_link_inode_operations;
	} else if (S_ISREG(dp->mode)) {
		if (dp->proc_fops == NULL)
			dp->proc_fops = &proc_file_operations;
		dp->proc_iops = &proc_file_inode_operations;
	} else {
		WARN_ON(1);
		proc_free_inum(dp->low_ino);
		return -EINVAL;
	}

	spin_lock(&proc_subdir_lock);
	dp->parent = dir;
	if (pde_subdir_insert(dir, dp) == false) {
		WARN(1, "proc_dir_entry '%s/%s' already registered\n",
		     dir->name, dp->name);
		spin_unlock(&proc_subdir_lock);
		if (S_ISDIR(dp->mode))
			dir->nlink--;
		proc_free_inum(dp->low_ino);
		return -EEXIST;
	}
	spin_unlock(&proc_subdir_lock);

	return 0;
}

static struct proc_dir_entry *__proc_create(struct proc_dir_entry **parent,
					  const char *name,
					  umode_t mode,
					  nlink_t nlink)
{
	struct proc_dir_entry *ent = NULL;
	const char *fn;
	struct qstr qstr;

	if (xlate_proc_name(name, parent, &fn) != 0)
		goto out;
	qstr.name = fn;
	qstr.len = strlen(fn);
	if (qstr.len == 0 || qstr.len >= 256) {
		WARN(1, "name len %u\n", qstr.len);
		return NULL;
	}
	if (*parent == &proc_root && name_to_int(&qstr) != ~0U) {
		WARN(1, "create '/proc/%s' by hand\n", qstr.name);
		return NULL;
	}

	ent = kzalloc(sizeof(struct proc_dir_entry) + qstr.len + 1, GFP_KERNEL);
	if (!ent)
		goto out;

	memcpy(ent->name, fn, qstr.len + 1);
	ent->namelen = qstr.len;
	ent->mode = mode;
	ent->nlink = nlink;
	ent->subdir = RB_ROOT;
	atomic_set(&ent->count, 1);
	spin_lock_init(&ent->pde_unload_lock);
	INIT_LIST_HEAD(&ent->pde_openers);
out:
	return ent;
}

struct proc_dir_entry *proc_symlink(const char *name,
		struct proc_dir_entry *parent, const char *dest)
{
	struct proc_dir_entry *ent;

	ent = __proc_create(&parent, name,
			  (S_IFLNK | S_IRUGO | S_IWUGO | S_IXUGO),1);

	if (ent) {
		ent->data = kmalloc((ent->size=strlen(dest))+1, GFP_KERNEL);
		if (ent->data) {
			strcpy((char*)ent->data,dest);
			if (proc_register(parent, ent) < 0) {
				kfree(ent->data);
				kfree(ent);
				ent = NULL;
			}
		} else {
			kfree(ent);
			ent = NULL;
		}
	}
	return ent;
}
EXPORT_SYMBOL(proc_symlink);

struct proc_dir_entry *proc_mkdir_data(const char *name, umode_t mode,
		struct proc_dir_entry *parent, void *data)
{
	struct proc_dir_entry *ent;

	if (mode == 0)
		mode = S_IRUGO | S_IXUGO;

	ent = __proc_create(&parent, name, S_IFDIR | mode, 2);
	if (ent) {
		ent->data = data;
		if (proc_register(parent, ent) < 0) {
			kfree(ent);
			ent = NULL;
		}
	}
	return ent;
}
EXPORT_SYMBOL_GPL(proc_mkdir_data);

struct proc_dir_entry *proc_mkdir_mode(const char *name, umode_t mode,
				       struct proc_dir_entry *parent)
{
	return proc_mkdir_data(name, mode, parent, NULL);
}
EXPORT_SYMBOL(proc_mkdir_mode);

struct proc_dir_entry *proc_mkdir(const char *name,
		struct proc_dir_entry *parent)
{
	return proc_mkdir_data(name, 0, parent, NULL);
}
EXPORT_SYMBOL(proc_mkdir);

struct proc_dir_entry *create_proc_read_entry(
	const char *name, umode_t mode, struct proc_dir_entry *parent, 
	read_proc_t *read_proc, void *data)
{
	struct proc_dir_entry *ent;

	if ((mode & S_IFMT) == 0)
		mode |= S_IFREG;

	if (!S_ISREG(mode)) {
		WARN_ON(1);	/* use proc_mkdir(), damnit */
		return NULL;
	}

	if ((mode & S_IALLUGO) == 0)
		mode |= S_IRUGO;

	ent = __proc_create(&parent, name, mode, 1);
	if (ent) {
		ent->read_proc = read_proc;
		ent->data = data;
		if (proc_register(parent, ent) < 0) {
			kfree(ent);
			ent = NULL;
		}
	}
	return ent;
}
EXPORT_SYMBOL(create_proc_read_entry);

struct proc_dir_entry *proc_create_data(const char *name, umode_t mode,
					struct proc_dir_entry *parent,
					const struct file_operations *proc_fops,
					void *data)
{
	struct proc_dir_entry *pde;
	if ((mode & S_IFMT) == 0)
		mode |= S_IFREG;

	if (!S_ISREG(mode)) {
		WARN_ON(1);	/* use proc_mkdir() */
		return NULL;
	}

	if ((mode & S_IALLUGO) == 0)
		mode |= S_IRUGO;
	pde = __proc_create(&parent, name, mode, 1);
	if (!pde)
		goto out;
	pde->proc_fops = proc_fops;
	pde->data = data;
	if (proc_register(parent, pde) < 0)
		goto out_free;
	return pde;
out_free:
	kfree(pde);
out:
	return NULL;
}
EXPORT_SYMBOL(proc_create_data);
 
void proc_set_size(struct proc_dir_entry *de, loff_t size)
{
	de->size = size;
}
EXPORT_SYMBOL(proc_set_size);

void proc_set_user(struct proc_dir_entry *de, uid_t uid, gid_t gid)
{
	de->uid = uid;
	de->gid = gid;
}
EXPORT_SYMBOL(proc_set_user);

static void free_proc_entry(struct proc_dir_entry *de)
{
	proc_free_inum(de->low_ino);

	if (S_ISLNK(de->mode))
		kfree(de->data);
	kfree(de);
}

void pde_put(struct proc_dir_entry *pde)
{
	if (atomic_dec_and_test(&pde->count))
		free_proc_entry(pde);
}

/*
 * Remove a /proc entry and free it if it's not currently in use.
 */
void remove_proc_entry(const char *name, struct proc_dir_entry *parent)
{
	struct proc_dir_entry *de = NULL;
	const char *fn = name;
	unsigned int len;

	spin_lock(&proc_subdir_lock);
	if (__xlate_proc_name(name, &parent, &fn) != 0) {
		spin_unlock(&proc_subdir_lock);
		return;
	}
	len = strlen(fn);

	de = pde_subdir_find(parent, fn, len);
	if (de)
		rb_erase(&de->subdir_node, &parent->subdir);
	spin_unlock(&proc_subdir_lock);
	if (!de) {
		WARN(1, "name '%s'\n", name);
		return;
	}

	proc_entry_rundown(de);

	if (S_ISDIR(de->mode))
		parent->nlink--;
	de->nlink = 0;
	WARN(pde_subdir_first(de),
	     "%s: removing non-empty directory '%s/%s', leaking at least '%s'\n",
	     __func__, de->parent->name, de->name, pde_subdir_first(de)->name);
	pde_put(de);
}
EXPORT_SYMBOL(remove_proc_entry);

int remove_proc_subtree(const char *name, struct proc_dir_entry *parent)
{
	struct proc_dir_entry *root = NULL, *de, *next;
	const char *fn = name;
	unsigned int len;

	spin_lock(&proc_subdir_lock);
	if (__xlate_proc_name(name, &parent, &fn) != 0) {
		spin_unlock(&proc_subdir_lock);
		return -ENOENT;
	}
	len = strlen(fn);

	root = pde_subdir_find(parent, fn, len);
	if (!root) {
		spin_unlock(&proc_subdir_lock);
		return -ENOENT;
	}
	rb_erase(&root->subdir_node, &parent->subdir);

	de = root;
	while (1) {
		next = pde_subdir_first(de);
		if (next) {
			rb_erase(&next->subdir_node, &de->subdir);
			de = next;
			continue;
		}
		spin_unlock(&proc_subdir_lock);

		proc_entry_rundown(de);
		next = de->parent;
		if (S_ISDIR(de->mode))
			next->nlink--;
		de->nlink = 0;
		if (de == root)
			break;
		pde_put(de);

		spin_lock(&proc_subdir_lock);
		de = next;
	}
	pde_put(root);
	return 0;
}
EXPORT_SYMBOL(remove_proc_subtree);

void *proc_get_parent_data(const struct inode *inode)
{
	struct proc_dir_entry *de = PDE(inode);
	return de->parent->data;
}
EXPORT_SYMBOL_GPL(proc_get_parent_data);

void proc_remove(struct proc_dir_entry *de)
{
	if (de)
		remove_proc_subtree(de->name, de->parent);
}
EXPORT_SYMBOL(proc_remove);

void *PDE_DATA(const struct inode *inode)
{
	return __PDE_DATA(inode);
}
EXPORT_SYMBOL(PDE_DATA);
