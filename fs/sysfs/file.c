/*
 * fs/sysfs/file.c - sysfs regular (text) file implementation
 *
 * Copyright (c) 2001-3 Patrick Mochel
 * Copyright (c) 2007 SUSE Linux Products GmbH
 * Copyright (c) 2007 Tejun Heo <teheo@suse.de>
 *
 * This file is released under the GPLv2.
 *
 * Please see Documentation/filesystems/sysfs.txt for more information.
 */

#include <linux/module.h>
#include <linux/kobject.h>
#include <linux/kallsyms.h>
#include <linux/slab.h>
#include <linux/fsnotify.h>
#include <linux/namei.h>
#include <linux/poll.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/limits.h>
#include <asm/uaccess.h>
#include <linux/seq_file.h>

#include "sysfs.h"

/*
 * There's one sysfs_open_file for each open file and one sysfs_open_dirent
 * for each sysfs_dirent with one or more open files.
 *
 * sysfs_dirent->s_attr.open points to sysfs_open_dirent.  s_attr.open is
 * protected by sysfs_open_dirent_lock.
 *
 * filp->private_data points to seq_file whose ->private points to
 * sysfs_open_file.  sysfs_open_files are chained at
 * sysfs_open_dirent->files, which is protected by sysfs_open_file_mutex.
 */
static DEFINE_SPINLOCK(sysfs_open_dirent_lock);
static DEFINE_MUTEX(sysfs_open_file_mutex);

struct sysfs_open_dirent {
	atomic_t		refcnt;
	atomic_t		event;
	wait_queue_head_t	poll;
	struct list_head	files; /* goes through sysfs_open_file.list */
};

struct sysfs_open_file {
	struct sysfs_dirent	*sd;
	struct file		*file;
	struct mutex		mutex;
	int			event;
	struct list_head	list;
};

static struct sysfs_open_file *sysfs_of(struct file *file)
{
	return ((struct seq_file *)file->private_data)->private;
}

/*
 * Determine ktype->sysfs_ops for the given sysfs_dirent.  This function
 * must be called while holding an active reference.
 */
static const struct sysfs_ops *sysfs_file_ops(struct sysfs_dirent *sd)
{
	struct kobject *kobj = sd->s_parent->s_dir.kobj;

	lockdep_assert_held(sd);
	return kobj->ktype ? kobj->ktype->sysfs_ops : NULL;
}

/*
 * Reads on sysfs are handled through seq_file, which takes care of hairy
 * details like buffering and seeking.  The following function pipes
 * sysfs_ops->show() result through seq_file.
 */
static int sysfs_seq_show(struct seq_file *sf, void *v)
{
	struct sysfs_open_file *of = sf->private;
	struct kobject *kobj = of->sd->s_parent->s_dir.kobj;
	const struct sysfs_ops *ops;
	char *buf;
	ssize_t count;

	/* acquire buffer and ensure that it's >= PAGE_SIZE */
	count = seq_get_buf(sf, &buf);
	if (count < PAGE_SIZE) {
		seq_commit(sf, -1);
		return 0;
	}

	/*
	 * Need @of->sd for attr and ops, its parent for kobj.  @of->mutex
	 * nests outside active ref and is just to ensure that the ops
	 * aren't called concurrently for the same open file.
	 */
	mutex_lock(&of->mutex);
	if (!sysfs_get_active(of->sd)) {
		mutex_unlock(&of->mutex);
		return -ENODEV;
	}

	of->event = atomic_read(&of->sd->s_attr.open->event);

	/*
	 * Lookup @ops and invoke show().  Control may reach here via seq
	 * file lseek even if @ops->show() isn't implemented.
	 */
	ops = sysfs_file_ops(of->sd);
	if (ops->show)
		count = ops->show(kobj, of->sd->s_attr.attr, buf);
	else
		count = 0;

	sysfs_put_active(of->sd);
	mutex_unlock(&of->mutex);

	if (count < 0)
		return count;

	/*
	 * The code works fine with PAGE_SIZE return but it's likely to
	 * indicate truncated result or overflow in normal use cases.
	 */
	if (count >= (ssize_t)PAGE_SIZE) {
		print_symbol("fill_read_buffer: %s returned bad count\n",
			(unsigned long)ops->show);
		/* Try to struggle along */
		count = PAGE_SIZE - 1;
	}
	seq_commit(sf, count);
	return 0;
}

/**
 * flush_write_buffer - push buffer to kobject
 * @of: open file
 * @buf: data buffer for file
 * @count: number of bytes
 *
 * Get the correct pointers for the kobject and the attribute we're dealing
 * with, then call the store() method for it with @buf.
 */
static int flush_write_buffer(struct sysfs_open_file *of, char *buf,
			      size_t count)
{
	struct kobject *kobj = of->sd->s_parent->s_dir.kobj;
	const struct sysfs_ops *ops;
	int rc = 0;

	/*
	 * Need @of->sd for attr and ops, its parent for kobj.  @of->mutex
	 * nests outside active ref and is just to ensure that the ops
	 * aren't called concurrently for the same open file.
	 */
	mutex_lock(&of->mutex);
	if (!sysfs_get_active(of->sd)) {
		mutex_unlock(&of->mutex);
		return -ENODEV;
	}

	ops = sysfs_file_ops(of->sd);
	rc = ops->store(kobj, of->sd->s_attr.attr, buf, count);

	sysfs_put_active(of->sd);
	mutex_unlock(&of->mutex);

	return rc;
}

/**
 * sysfs_write_file - write an attribute
 * @file: file pointer
 * @user_buf: data to write
 * @count: number of bytes
 * @ppos: starting offset
 *
 * Copy data in from userland and pass it to the matching
 * sysfs_ops->store() by invoking flush_write_buffer().
 *
 * There is no easy way for us to know if userspace is only doing a partial
 * write, so we don't support them. We expect the entire buffer to come on
 * the first write.  Hint: if you're writing a value, first read the file,
 * modify only the the value you're changing, then write entire buffer
 * back.
 */
static ssize_t sysfs_write_file(struct file *file, const char __user *user_buf,
				size_t count, loff_t *ppos)
{
	struct sysfs_open_file *of = sysfs_of(file);
	ssize_t len = min_t(size_t, count, PAGE_SIZE - 1);
	char *buf;

	if (!len)
		return 0;

	buf = kmalloc(len + 1, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	if (copy_from_user(buf, user_buf, len)) {
		len = -EFAULT;
		goto out_free;
	}
	buf[len] = '\0';	/* guarantee string termination */

	len = flush_write_buffer(of, buf, len);
	if (len > 0)
		*ppos += len;
out_free:
	kfree(buf);
	return len;
}

/**
 *	sysfs_get_open_dirent - get or create sysfs_open_dirent
 *	@sd: target sysfs_dirent
 *	@of: sysfs_open_file for this instance of open
 *
 *	If @sd->s_attr.open exists, increment its reference count;
 *	otherwise, create one.  @of is chained to the files list.
 *
 *	LOCKING:
 *	Kernel thread context (may sleep).
 *
 *	RETURNS:
 *	0 on success, -errno on failure.
 */
static int sysfs_get_open_dirent(struct sysfs_dirent *sd,
				 struct sysfs_open_file *of)
{
	struct sysfs_open_dirent *od, *new_od = NULL;

 retry:
	mutex_lock(&sysfs_open_file_mutex);
	spin_lock_irq(&sysfs_open_dirent_lock);

	if (!sd->s_attr.open && new_od) {
		sd->s_attr.open = new_od;
		new_od = NULL;
	}

	od = sd->s_attr.open;
	if (od) {
		atomic_inc(&od->refcnt);
		list_add_tail(&of->list, &od->files);
	}

	spin_unlock_irq(&sysfs_open_dirent_lock);
	mutex_unlock(&sysfs_open_file_mutex);

	if (od) {
		kfree(new_od);
		return 0;
	}

	/* not there, initialize a new one and retry */
	new_od = kmalloc(sizeof(*new_od), GFP_KERNEL);
	if (!new_od)
		return -ENOMEM;

	atomic_set(&new_od->refcnt, 0);
	atomic_set(&new_od->event, 1);
	init_waitqueue_head(&new_od->poll);
	INIT_LIST_HEAD(&new_od->files);
	goto retry;
}

/**
 *	sysfs_put_open_dirent - put sysfs_open_dirent
 *	@sd: target sysfs_dirent
 *	@of: associated sysfs_open_file
 *
 *	Put @sd->s_attr.open and unlink @of from the files list.  If
 *	reference count reaches zero, disassociate and free it.
 *
 *	LOCKING:
 *	None.
 */
static void sysfs_put_open_dirent(struct sysfs_dirent *sd,
				  struct sysfs_open_file *of)
{
	struct sysfs_open_dirent *od = sd->s_attr.open;
	unsigned long flags;

	mutex_lock(&sysfs_open_file_mutex);
	spin_lock_irqsave(&sysfs_open_dirent_lock, flags);

	list_del(&of->list);
	if (atomic_dec_and_test(&od->refcnt))
		sd->s_attr.open = NULL;
	else
		od = NULL;

	spin_unlock_irqrestore(&sysfs_open_dirent_lock, flags);
	mutex_unlock(&sysfs_open_file_mutex);

	kfree(od);
}

static int sysfs_open_file(struct inode *inode, struct file *file)
{
	struct sysfs_dirent *attr_sd = file->f_path.dentry->d_fsdata;
	struct kobject *kobj = attr_sd->s_parent->s_dir.kobj;
	struct sysfs_open_file *of;
	const struct sysfs_ops *ops;
	int error = -EACCES;

	/* need attr_sd for attr and ops, its parent for kobj */
	if (!sysfs_get_active(attr_sd))
		return -ENODEV;

	/* every kobject with an attribute needs a ktype assigned */
	ops = sysfs_file_ops(attr_sd);
	if (WARN(!ops, KERN_ERR
		 "missing sysfs attribute operations for kobject: %s\n",
		 kobject_name(kobj)))
		goto err_out;

	/* File needs write support.
	 * The inode's perms must say it's ok,
	 * and we must have a store method.
	 */
	if (file->f_mode & FMODE_WRITE) {
		if (!(inode->i_mode & S_IWUGO) || !ops->store)
			goto err_out;
	}

	/* File needs read support.
	 * The inode's perms must say it's ok, and we there
	 * must be a show method for it.
	 */
	if (file->f_mode & FMODE_READ) {
		if (!(inode->i_mode & S_IRUGO) || !ops->show)
			goto err_out;
	}

	/* allocate a sysfs_open_file for the file */
	error = -ENOMEM;
	of = kzalloc(sizeof(struct sysfs_open_file), GFP_KERNEL);
	if (!of)
		goto err_out;

	mutex_init(&of->mutex);
	of->sd = attr_sd;
	of->file = file;

	/*
	 * Always instantiate seq_file even if read access is not
	 * implemented or requested.  This unifies private data access and
	 * most files are readable anyway.
	 */
	error = single_open(file, sysfs_seq_show, of);
	if (error)
		goto err_free;

	/* seq_file clears PWRITE unconditionally, restore it if WRITE */
	if (file->f_mode & FMODE_WRITE)
		file->f_mode |= FMODE_PWRITE;

	/* make sure we have open dirent struct */
	error = sysfs_get_open_dirent(attr_sd, of);
	if (error)
		goto err_close;

	/* open succeeded, put active references */
	sysfs_put_active(attr_sd);
	return 0;

err_close:
	single_release(inode, file);
err_free:
	kfree(of);
err_out:
	sysfs_put_active(attr_sd);
	return error;
}

static int sysfs_release(struct inode *inode, struct file *filp)
{
	struct sysfs_dirent *sd = filp->f_path.dentry->d_fsdata;
	struct sysfs_open_file *of = sysfs_of(filp);

	sysfs_put_open_dirent(sd, of);
	single_release(inode, filp);
	kfree(of);

	return 0;
}

/* Sysfs attribute files are pollable.  The idea is that you read
 * the content and then you use 'poll' or 'select' to wait for
 * the content to change.  When the content changes (assuming the
 * manager for the kobject supports notification), poll will
 * return POLLERR|POLLPRI, and select will return the fd whether
 * it is waiting for read, write, or exceptions.
 * Once poll/select indicates that the value has changed, you
 * need to close and re-open the file, or seek to 0 and read again.
 * Reminder: this only works for attributes which actively support
 * it, and it is not possible to test an attribute from userspace
 * to see if it supports poll (Neither 'poll' nor 'select' return
 * an appropriate error code).  When in doubt, set a suitable timeout value.
 */
static unsigned int sysfs_poll(struct file *filp, poll_table *wait)
{
	struct sysfs_open_file *of = sysfs_of(filp);
	struct sysfs_dirent *attr_sd = filp->f_path.dentry->d_fsdata;
	struct sysfs_open_dirent *od = attr_sd->s_attr.open;

	/* need parent for the kobj, grab both */
	if (!sysfs_get_active(attr_sd))
		goto trigger;

	poll_wait(filp, &od->poll, wait);

	sysfs_put_active(attr_sd);

	if (of->event != atomic_read(&od->event))
		goto trigger;

	return DEFAULT_POLLMASK;

 trigger:
	return DEFAULT_POLLMASK|POLLERR|POLLPRI;
}

void sysfs_notify_dirent(struct sysfs_dirent *sd)
{
	struct sysfs_open_dirent *od;
	unsigned long flags;

	spin_lock_irqsave(&sysfs_open_dirent_lock, flags);

	if (!WARN_ON(sysfs_type(sd) != SYSFS_KOBJ_ATTR)) {
		od = sd->s_attr.open;
		if (od) {
			atomic_inc(&od->event);
			wake_up_interruptible(&od->poll);
		}
	}

	spin_unlock_irqrestore(&sysfs_open_dirent_lock, flags);
}
EXPORT_SYMBOL_GPL(sysfs_notify_dirent);

void sysfs_notify(struct kobject *k, const char *dir, const char *attr)
{
	struct sysfs_dirent *sd = k->sd;

	mutex_lock(&sysfs_mutex);

	if (sd && dir)
		sd = sysfs_find_dirent(sd, dir, NULL);
	if (sd && attr)
		sd = sysfs_find_dirent(sd, attr, NULL);
	if (sd)
		sysfs_notify_dirent(sd);

	mutex_unlock(&sysfs_mutex);
}
EXPORT_SYMBOL_GPL(sysfs_notify);

const struct file_operations sysfs_file_operations = {
	.read		= seq_read,
	.write		= sysfs_write_file,
	.llseek		= seq_lseek,
	.open		= sysfs_open_file,
	.release	= sysfs_release,
	.poll		= sysfs_poll,
};

int sysfs_add_file_mode_ns(struct sysfs_dirent *dir_sd,
			   const struct attribute *attr, int type,
			   umode_t amode, const void *ns)
{
	umode_t mode = (amode & S_IALLUGO) | S_IFREG;
	struct sysfs_addrm_cxt acxt;
	struct sysfs_dirent *sd;
	int rc;

	sd = sysfs_new_dirent(attr->name, mode, type);
	if (!sd)
		return -ENOMEM;

	sd->s_ns = ns;
	sd->s_attr.attr = (void *)attr;
	sysfs_dirent_init_lockdep(sd);

	sysfs_addrm_start(&acxt);
	rc = sysfs_add_one(&acxt, sd, dir_sd);
	sysfs_addrm_finish(&acxt);

	if (rc)
		sysfs_put(sd);

	return rc;
}


int sysfs_add_file(struct sysfs_dirent *dir_sd, const struct attribute *attr,
		   int type)
{
	return sysfs_add_file_mode_ns(dir_sd, attr, type, attr->mode, NULL);
}

/**
 * sysfs_create_file_ns - create an attribute file for an object with custom ns
 * @kobj: object we're creating for
 * @attr: attribute descriptor
 * @ns: namespace the new file should belong to
 */
int sysfs_create_file_ns(struct kobject *kobj, const struct attribute *attr,
			 const void *ns)
{
	BUG_ON(!kobj || !kobj->sd || !attr);

	return sysfs_add_file_mode_ns(kobj->sd, attr, SYSFS_KOBJ_ATTR,
				      attr->mode, ns);

}
EXPORT_SYMBOL_GPL(sysfs_create_file_ns);

int sysfs_create_files(struct kobject *kobj, const struct attribute **ptr)
{
	int err = 0;
	int i;

	for (i = 0; ptr[i] && !err; i++)
		err = sysfs_create_file(kobj, ptr[i]);
	if (err)
		while (--i >= 0)
			sysfs_remove_file(kobj, ptr[i]);
	return err;
}
EXPORT_SYMBOL_GPL(sysfs_create_files);

/**
 * sysfs_add_file_to_group - add an attribute file to a pre-existing group.
 * @kobj: object we're acting for.
 * @attr: attribute descriptor.
 * @group: group name.
 */
int sysfs_add_file_to_group(struct kobject *kobj,
		const struct attribute *attr, const char *group)
{
	struct sysfs_dirent *dir_sd;
	int error;

	if (group)
		dir_sd = sysfs_get_dirent(kobj->sd, group);
	else
		dir_sd = sysfs_get(kobj->sd);

	if (!dir_sd)
		return -ENOENT;

	error = sysfs_add_file(dir_sd, attr, SYSFS_KOBJ_ATTR);
	sysfs_put(dir_sd);

	return error;
}
EXPORT_SYMBOL_GPL(sysfs_add_file_to_group);

/**
 * sysfs_chown_file - modify the ownership of the object
 * @kobj: object we're acting for.
 * @attr: attribute descriptor.
 * @uid: new uid.
 * @gid: new gid.
 *
 */
int sysfs_chown_file(struct kobject *kobj, const struct attribute *attr,
		     uid_t uid, gid_t gid)
{
	struct sysfs_dirent *sd;
	struct iattr newattrs;
	int rc;

	mutex_lock(&sysfs_mutex);

	rc = -ENOENT;
	sd = sysfs_find_dirent(kobj->sd, attr->name, NULL);
	if (!sd)
		goto out;

	memset(&newattrs, 0, sizeof(newattrs));
	newattrs.ia_valid = ATTR_UID | ATTR_GID;
	newattrs.ia_uid = uid;
	newattrs.ia_gid = gid;

	rc = sysfs_sd_setattr(sd, &newattrs);
out:
	mutex_unlock(&sysfs_mutex);
	return rc;
}
EXPORT_SYMBOL_GPL(sysfs_chown_file);

/**
 * sysfs_chmod_file - update the modified mode value on an object attribute.
 * @kobj: object we're acting for.
 * @attr: attribute descriptor.
 * @mode: file permissions.
 *
 */
int sysfs_chmod_file(struct kobject *kobj, const struct attribute *attr,
		     umode_t mode)
{
	struct sysfs_dirent *sd;
	struct iattr newattrs;
	const void *ns;
	int rc;

	rc = sysfs_attr_ns(kobj, attr, &ns);
	if (rc)
		return rc;

	mutex_lock(&sysfs_mutex);

	rc = -ENOENT;
	sd = sysfs_find_dirent(kobj->sd, ns, attr->name);
	if (!sd)
		goto out;

	newattrs.ia_mode = (mode & S_IALLUGO) | (sd->s_mode & ~S_IALLUGO);
	newattrs.ia_valid = ATTR_MODE;
	rc = sysfs_sd_setattr(sd, &newattrs);

 out:
	mutex_unlock(&sysfs_mutex);
	return rc;
}
EXPORT_SYMBOL_GPL(sysfs_chmod_file);

/**
 * sysfs_remove_file_ns - remove an object attribute with a custom ns tag
 * @kobj: object we're acting for
 * @attr: attribute descriptor
 * @ns: namespace tag of the file to remove
 *
 * Hash the attribute name and namespace tag and kill the victim.
 */
void sysfs_remove_file_ns(struct kobject *kobj, const struct attribute *attr,
			  const void *ns)
{
	struct sysfs_dirent *dir_sd = kobj->sd;

	sysfs_hash_and_remove(dir_sd, attr->name, ns);
}
EXPORT_SYMBOL_GPL(sysfs_remove_file_ns);

void sysfs_remove_files(struct kobject *kobj, const struct attribute **ptr)
{
	int i;
	for (i = 0; ptr[i]; i++)
		sysfs_remove_file(kobj, ptr[i]);
}
EXPORT_SYMBOL_GPL(sysfs_remove_files);

/**
 * sysfs_remove_file_from_group - remove an attribute file from a group.
 * @kobj: object we're acting for.
 * @attr: attribute descriptor.
 * @group: group name.
 */
void sysfs_remove_file_from_group(struct kobject *kobj,
		const struct attribute *attr, const char *group)
{
	struct sysfs_dirent *dir_sd;

	if (group)
		dir_sd = sysfs_get_dirent(kobj->sd, group);
	else
		dir_sd = sysfs_get(kobj->sd);
	if (dir_sd) {
		sysfs_hash_and_remove(dir_sd, attr->name, NULL);
		sysfs_put(dir_sd);
	}
}
EXPORT_SYMBOL_GPL(sysfs_remove_file_from_group);

struct sysfs_schedule_callback_struct {
	struct list_head	workq_list;
	struct kobject		*kobj;
	void			(*func)(void *);
	void			*data;
	struct module		*owner;
	struct work_struct	work;
};

static struct workqueue_struct *sysfs_workqueue;
static DEFINE_MUTEX(sysfs_workq_mutex);
static LIST_HEAD(sysfs_workq);
static void sysfs_schedule_callback_work(struct work_struct *work)
{
	struct sysfs_schedule_callback_struct *ss = container_of(work,
			struct sysfs_schedule_callback_struct, work);

	(ss->func)(ss->data);
	kobject_put(ss->kobj);
	module_put(ss->owner);
	mutex_lock(&sysfs_workq_mutex);
	list_del(&ss->workq_list);
	mutex_unlock(&sysfs_workq_mutex);
	kfree(ss);
}

/**
 * sysfs_schedule_callback - helper to schedule a callback for a kobject
 * @kobj: object we're acting for.
 * @func: callback function to invoke later.
 * @data: argument to pass to @func.
 * @owner: module owning the callback code
 *
 * sysfs attribute methods must not unregister themselves or their parent
 * kobject (which would amount to the same thing).  Attempts to do so will
 * deadlock, since unregistration is mutually exclusive with driver
 * callbacks.
 *
 * Instead methods can call this routine, which will attempt to allocate
 * and schedule a workqueue request to call back @func with @data as its
 * argument in the workqueue's process context.  @kobj will be pinned
 * until @func returns.
 *
 * Returns 0 if the request was submitted, -ENOMEM if storage could not
 * be allocated, -ENODEV if a reference to @owner isn't available,
 * -EAGAIN if a callback has already been scheduled for @kobj.
 */
int sysfs_schedule_callback(struct kobject *kobj, void (*func)(void *),
		void *data, struct module *owner)
{
	struct sysfs_schedule_callback_struct *ss, *tmp;

	if (!try_module_get(owner))
		return -ENODEV;

	mutex_lock(&sysfs_workq_mutex);
	list_for_each_entry_safe(ss, tmp, &sysfs_workq, workq_list)
		if (ss->kobj == kobj) {
			module_put(owner);
			mutex_unlock(&sysfs_workq_mutex);
			return -EAGAIN;
		}
	mutex_unlock(&sysfs_workq_mutex);

	if (sysfs_workqueue == NULL) {
		sysfs_workqueue = alloc_workqueue("sysfsd", WQ_UNBOUND | WQ_MEM_RECLAIM, 1);
		if (sysfs_workqueue == NULL) {
			module_put(owner);
			return -ENOMEM;
		}
	}

	ss = kmalloc(sizeof(*ss), GFP_KERNEL);
	if (!ss) {
		module_put(owner);
		return -ENOMEM;
	}
	kobject_get(kobj);
	ss->kobj = kobj;
	ss->func = func;
	ss->data = data;
	ss->owner = owner;
	INIT_WORK(&ss->work, sysfs_schedule_callback_work);
	INIT_LIST_HEAD(&ss->workq_list);
	mutex_lock(&sysfs_workq_mutex);
	list_add_tail(&ss->workq_list, &sysfs_workq);
	mutex_unlock(&sysfs_workq_mutex);
	queue_work(sysfs_workqueue, &ss->work);
	return 0;
}
EXPORT_SYMBOL_GPL(sysfs_schedule_callback);
