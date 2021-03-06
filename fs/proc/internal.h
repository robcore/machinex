/* internal.h: internal procfs definitions
 *
 * Copyright (C) 2004 Red Hat, Inc. All Rights Reserved.
 * Written by David Howells (dhowells@redhat.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <linux/sched.h>
#include <linux/proc_fs.h>
#include <linux/binfmts.h>
struct  ctl_table_header;
struct  mempolicy;

extern struct proc_dir_entry proc_root;
extern void proc_self_init(void);
#ifdef CONFIG_PROC_SYSCTL
extern int proc_sys_init(void);
extern void sysctl_head_put(struct ctl_table_header *head);
#else
static inline void proc_sys_init(void) { }
static inline void sysctl_head_put(struct ctl_table_header *head) { }
#endif
#ifdef CONFIG_NET
extern int proc_net_init(void);
#else
static inline int proc_net_init(void) { return 0; }
#endif

extern int proc_tid_stat(struct seq_file *m, struct pid_namespace *ns,
				struct pid *pid, struct task_struct *task);
extern int proc_tgid_stat(struct seq_file *m, struct pid_namespace *ns,
				struct pid *pid, struct task_struct *task);
extern int proc_pid_status(struct seq_file *m, struct pid_namespace *ns,
				struct pid *pid, struct task_struct *task);
extern int proc_pid_statm(struct seq_file *m, struct pid_namespace *ns,
				struct pid *pid, struct task_struct *task);
extern loff_t mem_lseek(struct file *file, loff_t offset, int orig);

extern const struct file_operations proc_tid_children_operations;
extern const struct file_operations proc_pid_maps_operations;
extern const struct file_operations proc_tid_maps_operations;
extern const struct file_operations proc_pid_numa_maps_operations;
extern const struct file_operations proc_tid_numa_maps_operations;
extern const struct file_operations proc_pid_smaps_operations;
extern const struct file_operations proc_tid_smaps_operations;
extern const struct file_operations proc_clear_refs_operations;
extern const struct file_operations proc_pagemap_operations;
extern const struct file_operations proc_net_operations;
extern const struct inode_operations proc_net_inode_operations;
extern const struct inode_operations proc_pid_link_inode_operations;

struct proc_maps_private {
	struct inode *inode;
	struct task_struct *task;
	struct mm_struct *mm;
#ifdef CONFIG_MMU
	struct vm_area_struct *tail_vma;
#endif
#ifdef CONFIG_NUMA
	struct mempolicy *task_mempolicy;
#endif
};

struct mm_struct *proc_mem_open(struct inode *inode, unsigned int mode);


void proc_init_inodecache(void);

/*
 * General functions
 */
static inline struct proc_inode *PROC_I(const struct inode *inode)
{
	return container_of(inode, struct proc_inode, vfs_inode);
}

static inline struct proc_dir_entry *PDE(const struct inode *inode)
{
	return PROC_I(inode)->pde;
}

static inline void *__PDE_DATA(const struct inode *inode)
{
	return PDE(inode)->data;
}

static inline struct pid *proc_pid(struct inode *inode)
{
	return PROC_I(inode)->pid;
}

static inline struct task_struct *get_proc_task(struct inode *inode)
{
	return get_pid_task(proc_pid(inode), PIDTYPE_PID);
}

static inline int task_dumpable(struct task_struct *task)
{
	int dumpable = 0;
	struct mm_struct *mm;

	task_lock(task);
	mm = task->mm;
	if (mm)
		dumpable = get_dumpable(mm);
	task_unlock(task);
	if (dumpable == SUID_DUMP_USER)
		return 1;
	return 0;
}

static inline unsigned name_to_int(const struct qstr *qstr)
{
	const char *name = qstr->name;
	int len = qstr->len;
	unsigned n = 0;

	if (len > 1 && *name == '0')
		goto out;
	while (len-- > 0) {
		unsigned c = *name++ - '0';
		if (c > 9)
			goto out;
		if (n >= (~0U-9)/10)
			goto out;
		n *= 10;
		n += c;
	}
	return n;
out:
	return ~0U;
}

/*
 * base.c
 */
extern int pid_delete_dentry(const struct dentry *);

extern struct dentry *proc_lookup_de(struct proc_dir_entry *, struct inode *,
				     struct dentry *);

struct pde_opener {
	struct file *file;
	struct list_head lh;
	int closing;
	struct completion *c;
};

ssize_t __proc_file_read(struct file *, char __user *, size_t, loff_t *);
extern const struct file_operations proc_file_operations;
void proc_entry_rundown(struct proc_dir_entry *);

extern rwlock_t proc_subdir_lock;

struct dentry *proc_pid_lookup(struct inode *dir, struct dentry * dentry, unsigned int);
extern int proc_pid_readdir(struct file *, struct dir_context *);
unsigned long task_vsize(struct mm_struct *);
unsigned long task_statm(struct mm_struct *,
	unsigned long *, unsigned long *, unsigned long *, unsigned long *);
void task_mem(struct seq_file *, struct mm_struct *);

static inline struct proc_dir_entry *pde_get(struct proc_dir_entry *pde)
{
	atomic_inc(&pde->count);
	return pde;
}
void pde_put(struct proc_dir_entry *pde);

static inline bool is_empty_pde(const struct proc_dir_entry *pde)
{
	return S_ISDIR(pde->mode) && !pde->proc_iops;
}
struct proc_dir_entry *proc_create_mount_point(const char *name);
int proc_fill_super(struct super_block *);
struct inode *proc_get_inode(struct super_block *, struct proc_dir_entry *);
int proc_remount(struct super_block *sb, int *flags, char *data);

/*
 * These are generic /proc routines that use the internal
 * "struct proc_dir_entry" tree to traverse the filesystem.
 *
 * The /proc root directory has extended versions to take care
 * of the /proc/<pid> subdirectories.
 */
extern int proc_readdir(struct file *, struct dir_context *);
extern int proc_readdir_de(struct proc_dir_entry *, struct file *, struct dir_context *);
struct dentry *proc_lookup(struct inode *, struct dentry *, unsigned int);
extern const struct inode_operations proc_link_inode_operations;


/* Lookups */
typedef int instantiate_t(struct inode *, struct dentry *,
				struct task_struct *, const void *);
extern bool proc_fill_cache(struct file *, struct dir_context *, const char *, int,
			   instantiate_t, struct task_struct *, const void *);
int pid_revalidate(struct dentry *dentry, unsigned int flags);
struct inode *proc_pid_make_inode(struct super_block * sb, struct task_struct *task);
extern const struct dentry_operations pid_dentry_operations;
int pid_getattr(struct vfsmount *mnt, struct dentry *dentry, struct kstat *stat);
int proc_setattr(struct dentry *dentry, struct iattr *attr);

extern const struct inode_operations proc_ns_dir_inode_operations;
extern const struct file_operations proc_ns_dir_operations;

extern int proc_setup_self(struct super_block *);

/*
 * proc_thread_self.c
 */
extern int proc_setup_thread_self(struct super_block *);
extern void proc_thread_self_init(void);

/*
 * proc_devtree.c
 */
#ifdef CONFIG_PROC_DEVICETREE
extern void proc_device_tree_init(void);
#endif /* CONFIG_PROC_DEVICETREE */

/*
 * proc_tty.c
 */
#ifdef CONFIG_TTY
extern void proc_tty_init(void);
#else
static inline void proc_tty_init(void) {}
#endif

/*
 * root.c
 */
extern struct proc_dir_entry proc_root;

extern void proc_self_init(void);
extern int proc_remount(struct super_block *, int *, char *);
