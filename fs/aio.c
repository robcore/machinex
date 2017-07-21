/*
 *	An async IO implementation for Linux
 *	Written by Benjamin LaHaise <bcrl@kvack.org>
 *
 *	Implements an efficient asynchronous io interface.
 *
 *	Copyright 2000, 2001, 2002 Red Hat, Inc.  All Rights Reserved.
 *
 *	See ../COPYING for licensing terms.
 */
#define pr_fmt(fmt) "%s: " fmt, __func__

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <linux/aio_abi.h>
#include <linux/export.h>
#include <linux/syscalls.h>
#include <linux/backing-dev.h>
#include <linux/uio.h>

#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/mmu_context.h>
#include <linux/percpu.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/aio.h>
#include <linux/highmem.h>
#include <linux/workqueue.h>
#include <linux/security.h>
#include <linux/eventfd.h>
#include <linux/blkdev.h>
#include <linux/compat.h>
#include <linux/anon_inodes.h>
#include <linux/migrate.h>
#include <linux/ramfs.h>
#include <linux/percpu-refcount.h>

#include <asm/kmap_types.h>
#include <asm/uaccess.h>

#include "read_write.h"

#include "internal.h"

#define AIO_RING_MAGIC			0xa10a10a1
#define AIO_RING_COMPAT_FEATURES	1
#define AIO_RING_INCOMPAT_FEATURES	0
struct aio_ring {
	unsigned	id;	/* kernel internal index number */
	unsigned	nr;	/* number of io_events */
	unsigned	head;
	unsigned	tail;

	unsigned	magic;
	unsigned	compat_features;
	unsigned	incompat_features;
	unsigned	header_length;	/* size of aio_ring */


	struct io_event		io_events[0];
}; /* 128 bytes + ring size */

#define AIO_RING_PAGES	8

struct kioctx_cpu {
	unsigned		reqs_available;
};

struct kioctx {
	struct percpu_ref	users;
	atomic_t		dead;

	/* This needs improving */
	unsigned long		user_id;
	struct hlist_node	list;

	struct __percpu kioctx_cpu *cpu;

	/*
	 * For percpu reqs_available, number of slots we move to/from global
	 * counter at a time:
	 */
	unsigned		req_batch;
	/*
	 * This is what userspace passed to io_setup(), it's not used for
	 * anything but counting against the global max_reqs quota.
	 *
	 * The real limit is nr_events - 1, which will be larger (see
	 * aio_setup_ring())
	 */
	unsigned		max_reqs;

	/* Size of ringbuffer, in units of struct io_event */
	unsigned		nr_events;

	unsigned long		mmap_base;
	unsigned long		mmap_size;

	struct page		**ring_pages;
	long			nr_pages;

	struct rcu_head		rcu_head;
	struct work_struct	free_work;

	struct {
		/*
		 * This counts the number of available slots in the ringbuffer,
		 * so we avoid overflowing it: it's decremented (if positive)
		 * when allocating a kiocb and incremented when the resulting
		 * io_event is pulled off the ringbuffer.
		 *
		 * We batch accesses to it with a percpu version.
		 */
		atomic_t	reqs_available;
	} ____cacheline_aligned_in_smp;

	struct {
		spinlock_t	ctx_lock;
		struct list_head active_reqs;	/* used for cancellation */
	} ____cacheline_aligned_in_smp;

	struct {
		struct mutex	ring_lock;
		wait_queue_head_t wait;
	} ____cacheline_aligned_in_smp;

	struct {
		unsigned	tail;
		spinlock_t	completion_lock;
	} ____cacheline_aligned_in_smp;

	struct page		*internal_pages[AIO_RING_PAGES];
	struct file		*aio_ring_file;
};

/*------ sysctl variables----*/
static DEFINE_SPINLOCK(aio_nr_lock);
unsigned long aio_nr;		/* current system wide number of aio requests */
unsigned long aio_max_nr = 0x10000; /* system wide maximum number of aio requests */
/*----end sysctl variables---*/

static struct kmem_cache	*kiocb_cachep;
static struct kmem_cache	*kioctx_cachep;

/* aio_setup
 *	Creates the slab caches used by the aio routines, panic on
 *	failure as this is done early during the boot sequence.
 */
static int __init aio_setup(void)
{
	kiocb_cachep = KMEM_CACHE(kiocb, SLAB_HWCACHE_ALIGN|SLAB_PANIC);
	kioctx_cachep = KMEM_CACHE(kioctx,SLAB_HWCACHE_ALIGN|SLAB_PANIC);

	pr_debug("sizeof(struct page) = %zu\n", sizeof(struct page));

	return 0;
}
__initcall(aio_setup);

static void aio_free_ring(struct kioctx *ctx)
{
	int i;
	struct file *aio_ring_file = ctx->aio_ring_file;

	for (i = 0; i < ctx->nr_pages; i++) {
		pr_debug("pid(%d) [%d] page->count=%d\n", current->pid, i,
				page_count(ctx->ring_pages[i]));
		put_page(ctx->ring_pages[i]);
	}

	if (ctx->ring_pages && ctx->ring_pages != ctx->internal_pages)
		kfree(ctx->ring_pages);

	if (aio_ring_file) {
		truncate_setsize(aio_ring_file->f_inode, 0);
		pr_debug("pid(%d) i_nlink=%u d_count=%d d_unhashed=%d i_count=%d\n",
			current->pid, aio_ring_file->f_inode->i_nlink,
			aio_ring_file->f_path.dentry->d_count,
			d_unhashed(aio_ring_file->f_path.dentry),
			atomic_read(&aio_ring_file->f_inode->i_count));
		fput(aio_ring_file);
		ctx->aio_ring_file = NULL;
	}
}

static int aio_ring_mmap(struct file *file, struct vm_area_struct *vma)
{
	vma->vm_ops = &generic_file_vm_ops;
	return 0;
}

static const struct file_operations aio_ring_fops = {
	.mmap = aio_ring_mmap,
};

static int aio_set_page_dirty(struct page *page)
{
	return 0;
}

#if IS_ENABLED(CONFIG_MIGRATION)
static int aio_migratepage(struct address_space *mapping, struct page *new,
			struct page *old, enum migrate_mode mode)
{
	struct kioctx *ctx = mapping->private_data;
	unsigned long flags;
	unsigned idx = old->index;
	int rc;

	/* Writeback must be complete */
	BUG_ON(PageWriteback(old));
	put_page(old);

	rc = migrate_page_move_mapping(mapping, new, old, NULL, mode);
	if (rc != MIGRATEPAGE_SUCCESS) {
		get_page(old);
		return rc;
	}

	get_page(new);

	spin_lock_irqsave(&ctx->completion_lock, flags);
	migrate_page_copy(new, old);
	ctx->ring_pages[idx] = new;
	spin_unlock_irqrestore(&ctx->completion_lock, flags);

	return rc;
}
#endif

static const struct address_space_operations aio_ctx_aops = {
	.set_page_dirty = aio_set_page_dirty,
#if IS_ENABLED(CONFIG_MIGRATION)
	.migratepage	= aio_migratepage,
#endif
};

static int aio_setup_ring(struct kioctx *ctx)
{
	struct aio_ring *ring;
	unsigned nr_events = ctx->max_reqs;
	struct mm_struct *mm = current->mm;
	unsigned long size, populate;
	int nr_pages;
	int i;
	struct file *file;

	/* Compensate for the ring buffer's head/tail overlap entry */
	nr_events += 2;	/* 1 is required, 2 for good luck */

	size = sizeof(struct aio_ring);
	size += sizeof(struct io_event) * nr_events;

	nr_pages = PFN_UP(size);
	if (nr_pages < 0)
		return -EINVAL;

	file = anon_inode_getfile_private("[aio]", &aio_ring_fops, ctx, O_RDWR);
	if (IS_ERR(file)) {
		ctx->aio_ring_file = NULL;
		return -EAGAIN;
	}

	file->f_inode->i_mapping->a_ops = &aio_ctx_aops;
	file->f_inode->i_mapping->private_data = ctx;
	file->f_inode->i_size = PAGE_SIZE * (loff_t)nr_pages;

	for (i = 0; i < nr_pages; i++) {
		struct page *page;
		page = find_or_create_page(file->f_inode->i_mapping,
					   i, GFP_HIGHUSER | __GFP_ZERO);
		if (!page)
			break;
		pr_debug("pid(%d) page[%d]->count=%d\n",
			 current->pid, i, page_count(page));
		SetPageUptodate(page);
		SetPageDirty(page);
		unlock_page(page);
	}
	ctx->aio_ring_file = file;
	nr_events = (PAGE_SIZE * nr_pages - sizeof(struct aio_ring))
			/ sizeof(struct io_event);

	ctx->ring_pages = ctx->internal_pages;
	if (nr_pages > AIO_RING_PAGES) {
		ctx->ring_pages = kcalloc(nr_pages, sizeof(struct page *),
					  GFP_KERNEL);
		if (!ctx->ring_pages)
			return -ENOMEM;
	}

	ctx->mmap_size = nr_pages * PAGE_SIZE;
	pr_debug("attempting mmap of %lu bytes\n", ctx->mmap_size);

	down_write(&mm->mmap_sem);
	ctx->mmap_base = do_mmap_pgoff(ctx->aio_ring_file, 0, ctx->mmap_size,
				       PROT_READ | PROT_WRITE,
				       MAP_SHARED | MAP_POPULATE, 0, &populate);
	if (IS_ERR((void *)ctx->mmap_base)) {
		up_write(&mm->mmap_sem);
		ctx->mmap_size = 0;
		aio_free_ring(ctx);
		return -EAGAIN;
	}
	up_write(&mm->mmap_sem);

	mm_populate(ctx->mmap_base, populate);

	pr_debug("mmap address: 0x%08lx\n", ctx->mmap_base);
	ctx->nr_pages = get_user_pages(current, mm, ctx->mmap_base, nr_pages,
				       1, 0, ctx->ring_pages, NULL);
	for (i = 0; i < ctx->nr_pages; i++)
		put_page(ctx->ring_pages[i]);

	if (unlikely(ctx->nr_pages != nr_pages)) {
		aio_free_ring(ctx);
		return -EAGAIN;
	}

	ctx->user_id = ctx->mmap_base;
	ctx->nr_events = nr_events; /* trusted copy */

	ring = kmap_atomic(ctx->ring_pages[0]);
	ring->nr = nr_events;	/* user copy */
	ring->id = ctx->user_id;
	ring->head = ring->tail = 0;
	ring->magic = AIO_RING_MAGIC;
	ring->compat_features = AIO_RING_COMPAT_FEATURES;
	ring->incompat_features = AIO_RING_INCOMPAT_FEATURES;
	ring->header_length = sizeof(struct aio_ring);
	kunmap_atomic(ring);
	flush_dcache_page(ctx->ring_pages[0]);

	return 0;
}

#define AIO_EVENTS_PER_PAGE	(PAGE_SIZE / sizeof(struct io_event))
#define AIO_EVENTS_FIRST_PAGE	((PAGE_SIZE - sizeof(struct aio_ring)) / sizeof(struct io_event))
#define AIO_EVENTS_OFFSET	(AIO_EVENTS_PER_PAGE - AIO_EVENTS_FIRST_PAGE)

void kiocb_set_cancel_fn(struct kiocb *req, kiocb_cancel_fn *cancel)
{
	struct kioctx *ctx = req->ki_ctx;
	unsigned long flags;

	spin_lock_irqsave(&ctx->ctx_lock, flags);

	if (!req->ki_list.next)
		list_add(&req->ki_list, &ctx->active_reqs);

	req->ki_cancel = cancel;

	spin_unlock_irqrestore(&ctx->ctx_lock, flags);
}
EXPORT_SYMBOL(kiocb_set_cancel_fn);

static int kiocb_cancel(struct kioctx *ctx, struct kiocb *kiocb)
{
	kiocb_cancel_fn *old, *cancel;

	/*
	 * Don't want to set kiocb->ki_cancel = KIOCB_CANCELLED unless it
	 * actually has a cancel function, hence the cmpxchg()
	 */

	cancel = ACCESS_ONCE(kiocb->ki_cancel);
	do {
		if (!cancel || cancel == KIOCB_CANCELLED)
			return -EINVAL;

		old = cancel;
		cancel = cmpxchg(&kiocb->ki_cancel, old, KIOCB_CANCELLED);
	} while (cancel != old);

	return cancel(kiocb);
}

static void free_ioctx_rcu(struct rcu_head *head)
{
	struct kioctx *ctx = container_of(head, struct kioctx, rcu_head);

	free_percpu(ctx->cpu);
	kmem_cache_free(kioctx_cachep, ctx);
}

/*
 * When this function runs, the kioctx has been removed from the "hash table"
 * and ctx->users has dropped to 0, so we know no more kiocbs can be submitted -
 * now it's safe to cancel any that need to be.
 */
static void free_ioctx(struct work_struct *work)
{
	struct kioctx *ctx = container_of(work, struct kioctx, free_work);
	struct aio_ring *ring;
	struct kiocb *req;
	unsigned cpu, avail;
	DEFINE_WAIT(wait);

	spin_lock_irq(&ctx->ctx_lock);

	while (!list_empty(&ctx->active_reqs)) {
		req = list_first_entry(&ctx->active_reqs,
				       struct kiocb, ki_list);

		list_del_init(&req->ki_list);
		kiocb_cancel(ctx, req);
	}

	spin_unlock_irq(&ctx->ctx_lock);

	for_each_possible_cpu(cpu) {
		struct kioctx_cpu *kcpu = per_cpu_ptr(ctx->cpu, cpu);

		atomic_add(kcpu->reqs_available, &ctx->reqs_available);
		kcpu->reqs_available = 0;
	}

	while (1) {
		prepare_to_wait(&ctx->wait, &wait, TASK_UNINTERRUPTIBLE);

		ring = kmap_atomic(ctx->ring_pages[0]);
		avail = (ring->head <= ring->tail)
			 ? ring->tail - ring->head
			 : ctx->nr_events - ring->head + ring->tail;

		atomic_add(avail, &ctx->reqs_available);
		ring->head = ring->tail;
		kunmap_atomic(ring);

		if (atomic_read(&ctx->reqs_available) >= ctx->nr_events - 1)
			break;

		schedule();
	}
	finish_wait(&ctx->wait, &wait);

	WARN_ON(atomic_read(&ctx->reqs_available) > ctx->nr_events - 1);

	aio_free_ring(ctx);

	pr_debug("freeing %p\n", ctx);

	/*
	 * Here the call_rcu() is between the wait_event() for reqs_active to
	 * hit 0, and freeing the ioctx.
	 *
	 * aio_complete() decrements reqs_active, but it has to touch the ioctx
	 * after to issue a wakeup so we use rcu.
	 */
	call_rcu(&ctx->rcu_head, free_ioctx_rcu);
}

static void free_ioctx_ref(struct percpu_ref *ref)
{
	struct kioctx *ctx = container_of(ref, struct kioctx, users);

	INIT_WORK(&ctx->free_work, free_ioctx);
	schedule_work(&ctx->free_work);
}

/* ioctx_alloc
 *	Allocates and initializes an ioctx.  Returns an ERR_PTR if it failed.
 */
static struct kioctx *ioctx_alloc(unsigned nr_events)
{
	struct mm_struct *mm = current->mm;
	struct kioctx *ctx;
	int err = -ENOMEM;

	/*
	 * We keep track of the number of available ringbuffer slots, to prevent
	 * overflow (reqs_available), and we also use percpu counters for this.
	 *
	 * So since up to half the slots might be on other cpu's percpu counters
	 * and unavailable, double nr_events so userspace sees what they
	 * expected: additionally, we move req_batch slots to/from percpu
	 * counters at a time, so make sure that isn't 0:
	 */
	nr_events = max(nr_events, num_possible_cpus() * 4);
	nr_events *= 2;

	/* Prevent overflows */
	if ((nr_events > (0x10000000U / sizeof(struct io_event))) ||
	    (nr_events > (0x10000000U / sizeof(struct kiocb)))) {
		pr_debug("ENOMEM: nr_events too high\n");
		return ERR_PTR(-EINVAL);
	}

	if (!nr_events || (unsigned long)nr_events > (aio_max_nr * 2UL))
		return ERR_PTR(-EAGAIN);

	ctx = kmem_cache_zalloc(kioctx_cachep, GFP_KERNEL);
	if (!ctx)
		return ERR_PTR(-ENOMEM);

	ctx->max_reqs = nr_events;

	if (percpu_ref_init(&ctx->users, free_ioctx_ref, 0, GFP_KERNEL))
		goto out_freectx;

	spin_lock_init(&ctx->ctx_lock);
	spin_lock_init(&ctx->completion_lock);
	mutex_init(&ctx->ring_lock);
	init_waitqueue_head(&ctx->wait);

	INIT_LIST_HEAD(&ctx->active_reqs);

	ctx->cpu = alloc_percpu(struct kioctx_cpu);
	if (!ctx->cpu)
		goto out_freeref;

	if (aio_setup_ring(ctx) < 0)
		goto out_freepcpu;

	atomic_set(&ctx->reqs_available, ctx->nr_events - 1);
	ctx->req_batch = (ctx->nr_events - 1) / (num_possible_cpus() * 4);
	BUG_ON(!ctx->req_batch);

	/* limit the number of system wide aios */
	spin_lock(&aio_nr_lock);
	if (aio_nr + nr_events > (aio_max_nr * 2UL) ||
	    aio_nr + nr_events < aio_nr) {
		spin_unlock(&aio_nr_lock);
		goto out_cleanup;
	}
	aio_nr += ctx->max_reqs;
	spin_unlock(&aio_nr_lock);

	percpu_ref_get(&ctx->users); /* io_setup() will drop this ref */

	/* now link into global list. */
	spin_lock(&mm->ioctx_lock);
	hlist_add_head_rcu(&ctx->list, &mm->ioctx_list);
	spin_unlock(&mm->ioctx_lock);

	pr_debug("allocated ioctx %p[%ld]: mm=%p mask=0x%x\n",
		 ctx, ctx->user_id, mm, ctx->nr_events);
	return ctx;

out_cleanup:
	err = -EAGAIN;
	aio_free_ring(ctx);
out_freepcpu:
	free_percpu(ctx->cpu);
out_freeref:
	//percpu_ref_exit(&ctx->reqs);
	free_percpu(&ctx->users);
out_freectx:
	if (ctx->aio_ring_file)
		fput(ctx->aio_ring_file);
	kmem_cache_free(kioctx_cachep, ctx);
	pr_debug("error allocating ioctx %d\n", err);
	return ERR_PTR(err);
}

/* kill_ioctx
 *	Cancels all outstanding aio requests on an aio context.  Used
 *	when the processes owning a context have all exited to encourage
 *	the rapid destruction of the kioctx.
 */
static void kill_ioctx(struct kioctx *ctx)
{
	if (!atomic_xchg(&ctx->dead, 1)) {
		hlist_del_rcu(&ctx->list);
		/* percpu_ref_kill() will do the necessary call_rcu() */
		wake_up_all(&ctx->wait);

		/*
		 * It'd be more correct to do this in free_ioctx(), after all
		 * the outstanding kiocbs have finished - but by then io_destroy
		 * has already returned, so io_setup() could potentially return
		 * -EAGAIN with no ioctxs actually in use (as far as userspace
		 *  could tell).
		 */
		spin_lock(&aio_nr_lock);
		BUG_ON(aio_nr - ctx->max_reqs > aio_nr);
		aio_nr -= ctx->max_reqs;
		spin_unlock(&aio_nr_lock);

		if (ctx->mmap_size)
			vm_munmap(ctx->mmap_base, ctx->mmap_size);

		percpu_ref_kill(&ctx->users);
	}
}

/* wait_on_sync_kiocb:
 *	Waits on the given sync kiocb to complete.
 */
ssize_t wait_on_sync_kiocb(struct kiocb *req)
{
	while (!req->ki_ctx) {
		set_current_state(TASK_UNINTERRUPTIBLE);
		if (req->ki_ctx)
			break;
		io_schedule();
	}
	__set_current_state(TASK_RUNNING);
	return req->ki_user_data;
}
EXPORT_SYMBOL(wait_on_sync_kiocb);

/*
 * exit_aio: called when the last user of mm goes away.  At this point, there is
 * no way for any new requests to be submited or any of the io_* syscalls to be
 * called on the context.
 *
 * There may be outstanding kiocbs, but free_ioctx() will explicitly wait on
 * them.
 */
void exit_aio(struct mm_struct *mm)
{
	struct kioctx *ctx;
	struct hlist_node *n;

	hlist_for_each_entry_safe(ctx, n, &mm->ioctx_list, list) {
		/*
		 * We don't need to bother with munmap() here -
		 * exit_mmap(mm) is coming and it'll unmap everything.
		 * Since aio_free_ring() uses non-zero ->mmap_size
		 * as indicator that it needs to unmap the area,
		 * just set it to 0; aio_free_ring() is the only
		 * place that uses ->mmap_size, so it's safe.
		 */
		ctx->mmap_size = 0;

		kill_ioctx(ctx);
	}
}

static void put_reqs_available(struct kioctx *ctx, unsigned nr)
{
	struct kioctx_cpu *kcpu;

	preempt_disable();
	kcpu = this_cpu_ptr(ctx->cpu);

	kcpu->reqs_available += nr;
	while (kcpu->reqs_available >= ctx->req_batch * 2) {
		kcpu->reqs_available -= ctx->req_batch;
		atomic_add(ctx->req_batch, &ctx->reqs_available);
	}

	preempt_enable();
}

static bool get_reqs_available(struct kioctx *ctx)
{
	struct kioctx_cpu *kcpu;
	bool ret = false;

	preempt_disable();
	kcpu = this_cpu_ptr(ctx->cpu);

	if (!kcpu->reqs_available) {
		int old, avail = atomic_read(&ctx->reqs_available);

		do {
			if (avail < ctx->req_batch)
				goto out;

			old = avail;
			avail = atomic_cmpxchg(&ctx->reqs_available,
					       avail, avail - ctx->req_batch);
		} while (avail != old);

		kcpu->reqs_available += ctx->req_batch;
	}

	ret = true;
	kcpu->reqs_available--;
out:
	preempt_enable();
	return ret;
}

/* aio_get_req
 *	Allocate a slot for an aio request.
 * Returns NULL if no requests are free.
 */
static inline struct kiocb *aio_get_req(struct kioctx *ctx)
{
	struct kiocb *req;

	if (!get_reqs_available(ctx))
		return NULL;

	req = kmem_cache_alloc(kiocb_cachep, GFP_KERNEL|__GFP_ZERO);
	if (unlikely(!req))
		goto out_put;

	req->ki_ctx = ctx;
	return req;
out_put:
	put_reqs_available(ctx, 1);
	return NULL;
}

static void kiocb_free(struct kiocb *req)
{
	if (req->ki_filp)
		fput(req->ki_filp);
	if (req->ki_eventfd != NULL)
		eventfd_ctx_put(req->ki_eventfd);
	if (req->ki_dtor)
		req->ki_dtor(req);
	kmem_cache_free(kiocb_cachep, req);
}

static struct kioctx *lookup_ioctx(unsigned long ctx_id)
{
	struct mm_struct *mm = current->mm;
	struct kioctx *ctx, *ret = NULL;

	rcu_read_lock();

	hlist_for_each_entry_rcu(ctx, &mm->ioctx_list, list) {
		if (ctx->user_id == ctx_id) {
			percpu_ref_get(&ctx->users);
			ret = ctx;
			break;
		}
	}

	rcu_read_unlock();
	return ret;
}

/* aio_complete
 *	Called when the io request on the given iocb is complete.
 */
void aio_complete(struct kiocb *iocb, long res, long res2)
{
	struct kioctx	*ctx = iocb->ki_ctx;
	struct aio_ring	*ring;
	struct io_event	*ev_page, *event;
	unsigned long	flags;
	unsigned tail, pos;

	/*
	 * Special case handling for sync iocbs:
	 *  - events go directly into the iocb for fast handling
	 *  - the sync task with the iocb in its stack holds the single iocb
	 *    ref, no other paths have a way to get another ref
	 *  - the sync task helpfully left a reference to itself in the iocb
	 */
	if (is_sync_kiocb(iocb)) {
		iocb->ki_user_data = res;
		smp_wmb();
		iocb->ki_ctx = ERR_PTR(-EXDEV);
		wake_up_process(iocb->ki_obj.tsk);
		return;
	}

	/*
	 * Take rcu_read_lock() in case the kioctx is being destroyed, as we
	 * need to issue a wakeup after incrementing reqs_available.
	 */
	rcu_read_lock();

	if (iocb->ki_list.next) {
		unsigned long flags;

		spin_lock_irqsave(&ctx->ctx_lock, flags);
		list_del(&iocb->ki_list);
		spin_unlock_irqrestore(&ctx->ctx_lock, flags);
	}

	/*
	 * Add a completion event to the ring buffer. Must be done holding
	 * ctx->completion_lock to prevent other code from messing with the tail
	 * pointer since we might be called from irq context.
	 */
	spin_lock_irqsave(&ctx->completion_lock, flags);

	tail = ctx->tail;
	pos = tail + AIO_EVENTS_OFFSET;

	if (++tail >= ctx->nr_events)
		tail = 0;

	ev_page = kmap_atomic(ctx->ring_pages[pos / AIO_EVENTS_PER_PAGE]);
	event = ev_page + pos % AIO_EVENTS_PER_PAGE;

	event->obj = (u64)(unsigned long)iocb->ki_obj.user;
	event->data = iocb->ki_user_data;
	event->res = res;
	event->res2 = res2;

	kunmap_atomic(ev_page);
	flush_dcache_page(ctx->ring_pages[pos / AIO_EVENTS_PER_PAGE]);

	pr_debug("%p[%u]: %p: %p %Lx %lx %lx\n",
		 ctx, tail, iocb, iocb->ki_obj.user, iocb->ki_user_data,
		 res, res2);

	/* after flagging the request as done, we
	 * must never even look at it again
	 */
	smp_wmb();	/* make event visible before updating tail */

	ctx->tail = tail;

	ring = kmap_atomic(ctx->ring_pages[0]);
	ring->tail = tail;
	kunmap_atomic(ring);
	flush_dcache_page(ctx->ring_pages[0]);

	spin_unlock_irqrestore(&ctx->completion_lock, flags);

	pr_debug("added to ring %p at [%u]\n", iocb, tail);

	/*
	 * Check if the user asked us to deliver the result through an
	 * eventfd. The eventfd_signal() function is safe to be called
	 * from IRQ context.
	 */
	if (iocb->ki_eventfd != NULL)
		eventfd_signal(iocb->ki_eventfd, 1);

	/* everything turned out well, dispose of the aiocb. */
	kiocb_free(iocb);

	/*
	 * We have to order our ring_info tail store above and test
	 * of the wait list below outside the wait lock.  This is
	 * like in wake_up_bit() where clearing a bit has to be
	 * ordered with the unlocked test.
	 */
	smp_mb();

	if (waitqueue_active(&ctx->wait))
		wake_up(&ctx->wait);

	rcu_read_unlock();
}
EXPORT_SYMBOL(aio_complete);

/* aio_read_events
 *	Pull an event off of the ioctx's event ring.  Returns the number of
 *	events fetched
 */
static long aio_read_events_ring(struct kioctx *ctx,
				 struct io_event __user *event, long nr)
{
	struct aio_ring *ring;
	unsigned head, tail, pos;
	long ret = 0;
	int copy_ret;

	mutex_lock(&ctx->ring_lock);

	ring = kmap_atomic(ctx->ring_pages[0]);
	head = ring->head;
	tail = ring->tail;
	kunmap_atomic(ring);

	pr_debug("h%u t%u m%u\n", head, tail, ctx->nr_events);

	if (head == tail)
		goto out;

	while (ret < nr) {
		long avail;
		struct io_event *ev;
		struct page *page;

		avail = (head <= tail ?  tail : ctx->nr_events) - head;
		if (head == tail)
			break;

		avail = min(avail, nr - ret);
		avail = min_t(long, avail, AIO_EVENTS_PER_PAGE -
			    ((head + AIO_EVENTS_OFFSET) % AIO_EVENTS_PER_PAGE));

		pos = head + AIO_EVENTS_OFFSET;
		page = ctx->ring_pages[pos / AIO_EVENTS_PER_PAGE];
		pos %= AIO_EVENTS_PER_PAGE;

		ev = kmap(page);
		copy_ret = copy_to_user(event + ret, ev + pos,
					sizeof(*ev) * avail);
		kunmap(page);

		if (unlikely(copy_ret)) {
			ret = -EFAULT;
			goto out;
		}

		ret += avail;
		head += avail;
		head %= ctx->nr_events;
	}

	ring = kmap_atomic(ctx->ring_pages[0]);
	ring->head = head;
	kunmap_atomic(ring);
	flush_dcache_page(ctx->ring_pages[0]);

	pr_debug("%li  h%u t%u\n", ret, head, tail);

	put_reqs_available(ctx, ret);
out:
	mutex_unlock(&ctx->ring_lock);

	return ret;
}

static bool aio_read_events(struct kioctx *ctx, long min_nr, long nr,
			    struct io_event __user *event, long *i)
{
	long ret = aio_read_events_ring(ctx, event + *i, nr - *i);

	if (ret > 0)
		*i += ret;

	if (unlikely(atomic_read(&ctx->dead)))
		ret = -EINVAL;

	if (!*i)
		*i = ret;

	return ret < 0 || *i >= min_nr;
}

static long read_events(struct kioctx *ctx, long min_nr, long nr,
			struct io_event __user *event,
			struct timespec __user *timeout)
{
	ktime_t until = KTIME_MAX;
	long ret = 0;

	if (timeout) {
		struct timespec	ts;

		if (unlikely(copy_from_user(&ts, timeout, sizeof(ts))))
			return -EFAULT;

		until = timespec_to_ktime(ts);
	}

	if (until == 0)
		aio_read_events(ctx, min_nr, nr, event, &ret);
	else
		wait_event_interruptible_hrtimeout(ctx->wait,
			aio_read_events(ctx, min_nr, nr, event, &ret), until);

	if (!ret && signal_pending(current))
		ret = -EINTR;

	return ret;
}

/* sys_io_setup:
 *	Create an aio_context capable of receiving at least nr_events.
 *	ctxp must not point to an aio_context that already exists, and
 *	must be initialized to 0 prior to the call.  On successful
 *	creation of the aio_context, *ctxp is filled in with the resulting 
 *	handle.  May fail with -EINVAL if *ctxp is not initialized,
 *	if the specified nr_events exceeds internal limits.  May fail 
 *	with -EAGAIN if the specified nr_events exceeds the user's limit 
 *	of available events.  May fail with -ENOMEM if insufficient kernel
 *	resources are available.  May fail with -EFAULT if an invalid
 *	pointer is passed for ctxp.  Will fail with -ENOSYS if not
 *	implemented.
 */
SYSCALL_DEFINE2(io_setup, unsigned, nr_events, aio_context_t __user *, ctxp)
{
	struct kioctx *ioctx = NULL;
	unsigned long ctx;
	long ret;

	ret = get_user(ctx, ctxp);
	if (unlikely(ret))
		goto out;

	ret = -EINVAL;
	if (unlikely(ctx || nr_events == 0)) {
		pr_debug("EINVAL: io_setup: ctx %lu nr_events %u\n",
		         ctx, nr_events);
		goto out;
	}

	ioctx = ioctx_alloc(nr_events);
	ret = PTR_ERR(ioctx);
	if (!IS_ERR(ioctx)) {
		ret = put_user(ioctx->user_id, ctxp);
		if (ret)
			kill_ioctx(ioctx);
		percpu_ref_put(&ioctx->users);
	}

out:
	return ret;
}

/* sys_io_destroy:
 *	Destroy the aio_context specified.  May cancel any outstanding 
 *	AIOs and block on completion.  Will fail with -ENOSYS if not
 *	implemented.  May fail with -EINVAL if the context pointed to
 *	is invalid.
 */
SYSCALL_DEFINE1(io_destroy, aio_context_t, ctx)
{
	struct kioctx *ioctx = lookup_ioctx(ctx);
	if (likely(NULL != ioctx)) {
		kill_ioctx(ioctx);
		percpu_ref_put(&ioctx->users);
		return 0;
	}
	pr_debug("EINVAL: io_destroy: invalid context id\n");
	return -EINVAL;
}

typedef ssize_t (aio_rw_op)(struct kiocb *, const struct iovec *,
			    unsigned long, loff_t);

static ssize_t aio_setup_vectored_rw(struct kiocb *kiocb,
				     int rw, char __user *buf,
				     unsigned long *nr_segs,
				     struct iovec **iovec,
				     bool compat)
{
	ssize_t ret;

	*nr_segs = kiocb->ki_nbytes;

#ifdef CONFIG_COMPAT
	if (compat)
		ret = compat_rw_copy_check_uvector(rw,
				(struct compat_iovec __user *)buf,
				*nr_segs, 1, *iovec, iovec);
	else
#endif
		ret = rw_copy_check_uvector(rw,
				(struct iovec __user *)buf,
				*nr_segs, 1, *iovec, iovec);
	if (ret < 0)
		return ret;

	/* ki_nbytes now reflect bytes instead of segs */
	kiocb->ki_nbytes = ret;
	return 0;
}

static ssize_t aio_setup_single_vector(struct kiocb *kiocb,
				       int rw, char __user *buf,
				       unsigned long *nr_segs,
				       struct iovec *iovec)
{
	if (unlikely(!access_ok(!rw, buf, kiocb->ki_nbytes)))
		return -EFAULT;

	iovec->iov_base = buf;
	iovec->iov_len = kiocb->ki_nbytes;
	*nr_segs = 1;
	return 0;
}

static ssize_t aio_read_iter(struct kiocb *iocb)
{
	struct file *file = iocb->ki_filp;
	ssize_t ret = -EINVAL;

	if (file->f_op->read_iter)
		ret = file->f_op->read_iter(iocb, iocb->ki_iter, iocb->ki_pos);
	return ret;
}

static ssize_t aio_write_iter(struct kiocb *iocb)
{
	struct file *file = iocb->ki_filp;
	ssize_t ret = -EINVAL;

	if (file->f_op->write_iter)
		ret = file->f_op->write_iter(iocb, iocb->ki_iter, iocb->ki_pos);
	return ret;
}

/*
 * aio_setup_iocb:
 *	Performs the initial checks and aio retry method
 *	setup for the kiocb at the time of io submission.
 */
static ssize_t aio_run_iocb(struct kiocb *req, unsigned opcode,
			    char __user *buf, bool compat)
{
	struct file *file = req->ki_filp;
	ssize_t ret;
	unsigned long nr_segs;
	int rw;
	fmode_t mode;
	aio_rw_op *rw_op;
	struct iovec inline_vec, *iovec = &inline_vec;

	switch (opcode) {
	case IOCB_CMD_PREAD:
	case IOCB_CMD_PREADV:
		mode	= FMODE_READ;
		rw	= READ;
		rw_op	= file->f_op->aio_read;
		goto rw_common;

	case IOCB_CMD_PWRITE:
	case IOCB_CMD_PWRITEV:
		mode	= FMODE_WRITE;
		rw	= WRITE;
		rw_op	= file->f_op->aio_write;
		goto rw_common;
rw_common:
		if (unlikely(!(file->f_mode & mode)))
			return -EBADF;

		if (!rw_op)
			return -EINVAL;

		ret = (opcode == IOCB_CMD_PREADV ||
		       opcode == IOCB_CMD_PWRITEV)
			? aio_setup_vectored_rw(req, rw, buf, &nr_segs,
						&iovec, compat)
			: aio_setup_single_vector(req, rw, buf, &nr_segs,
						  iovec);
		if (ret)
			return ret;

		ret = rw_verify_area(rw, file, &req->ki_pos, req->ki_nbytes);
		if (ret < 0) {
			if (iovec != &inline_vec)
				kfree(iovec);
			return ret;
		}

		req->ki_nbytes = ret;

		/* XXX: move/kill - rw_verify_area()? */
		/* This matches the pread()/pwrite() logic */
		if (req->ki_pos < 0) {
			ret = -EINVAL;
			break;
		}

		if (rw == WRITE)
			file_start_write(file);

		ret = rw_op(req, iovec, nr_segs, req->ki_pos);

		if (rw == WRITE)
			file_end_write(file);
		break;

	case IOCB_CMD_FDSYNC:
		if (!file->f_op->aio_fsync)
			return -EINVAL;

		ret = file->f_op->aio_fsync(req, 1);
		break;

	case IOCB_CMD_FSYNC:
		if (!file->f_op->aio_fsync)
			return -EINVAL;

		ret = file->f_op->aio_fsync(req, 0);
		break;

	default:
		pr_debug("EINVAL: no operation provided\n");
		return -EINVAL;
	}

	if (iovec != &inline_vec)
		kfree(iovec);

	if (ret != -EIOCBQUEUED) {
		/*
		 * There's no easy way to restart the syscall since other AIO's
		 * may be already running. Just fail this IO with EINTR.
		 */
		if (unlikely(ret == -ERESTARTSYS || ret == -ERESTARTNOINTR ||
			     ret == -ERESTARTNOHAND ||
			     ret == -ERESTART_RESTARTBLOCK))
			ret = -EINTR;
		aio_complete(req, ret, 0);
	}

	return 0;
}

/*
 * This allocates an iocb that will be used to submit and track completion of
 * an IO that is issued from kernel space.
 *
 * The caller is expected to call the appropriate aio_kernel_init_() functions
 * and then call aio_kernel_submit().  From that point forward progress is
 * guaranteed by the file system aio method.  Eventually the caller's
 * completion callback will be called.
 *
 * These iocbs are special.  They don't have a context, we don't limit the
 * number pending, they can't be canceled, and can't be retried.  In the short
 * term callers need to be careful not to call operations which might retry by
 * only calling new ops which never add retry support.  In the long term
 * retry-based AIO should be removed.
 */
struct kiocb *aio_kernel_alloc(gfp_t gfp)
{
	struct kiocb *iocb = kzalloc(sizeof(struct kiocb), gfp);
	if (iocb)
		iocb->ki_key = KIOCB_KERNEL_KEY;
	return iocb;
}
EXPORT_SYMBOL_GPL(aio_kernel_alloc);

void aio_kernel_free(struct kiocb *iocb)
{
	kfree(iocb);
}
EXPORT_SYMBOL_GPL(aio_kernel_free);

/*
 * ptr and count can be a buff and bytes or an iov and segs.
 */
void aio_kernel_init_rw(struct kiocb *iocb, struct file *filp,
			unsigned short op, void *ptr, size_t nr, loff_t off)
{
	iocb->ki_filp = filp;
	iocb->ki_opcode = op;
	iocb->ki_buf = (char __user *)(unsigned long)ptr;
	iocb->ki_left = nr;
	iocb->ki_nbytes = nr;
	iocb->ki_pos = off;
}
EXPORT_SYMBOL_GPL(aio_kernel_init_rw);

/*
 * The iter count must be set before calling here.  Some filesystems uses
 * iocb->ki_left as an indicator of the size of an IO.
 */
void aio_kernel_init_iter(struct kiocb *iocb, struct file *filp,
			  unsigned short op, struct iov_iter *iter, loff_t off)
{
	iocb->ki_filp = filp;
	iocb->ki_iter = iter;
	iocb->ki_opcode = op;
	iocb->ki_pos = off;
	iocb->ki_nbytes = iov_iter_count(iter);
	iocb->ki_left = iocb->ki_nbytes;
}
EXPORT_SYMBOL_GPL(aio_kernel_init_iter);

void aio_kernel_init_callback(struct kiocb *iocb,
			      void (*complete)(u64 user_data, long res),
			      u64 user_data)
{
	iocb->ki_obj.complete = complete;
	iocb->ki_user_data = user_data;
}
EXPORT_SYMBOL_GPL(aio_kernel_init_callback);

/*
 * The iocb is our responsibility once this is called.  The caller must not
 * reference it.  This comes from aio_setup_iocb() modifying the iocb.
 *
 * Callers must be prepared for their iocb completion callback to be called the
 * moment they enter this function.  The completion callback may be called from
 * any context.
 *
 * Returns: 0: the iocb completion callback will be called with the op result
 * negative errno: the operation was not submitted and the iocb was freed
 */
int aio_kernel_submit(struct kiocb *iocb)
{
	int ret;

	BUG_ON(!is_kernel_kiocb(iocb));
	BUG_ON(!iocb->ki_obj.complete);
	BUG_ON(!iocb->ki_filp);

	ret = aio_run_iocb(iocb, 0);
	if (ret)
		aio_kernel_free(iocb);

	return ret;
}
EXPORT_SYMBOL_GPL(aio_kernel_submit);

static int io_submit_one(struct kioctx *ctx, struct iocb __user *user_iocb,
			 struct iocb *iocb, bool compat)
{
	struct kiocb *req;
	ssize_t ret;

	/* enforce forwards compatibility on users */
	if (unlikely(iocb->aio_reserved1 || iocb->aio_reserved2)) {
		pr_debug("EINVAL: reserve field set\n");
		return -EINVAL;
	}

	/* prevent overflows */
	if (unlikely(
	    (iocb->aio_buf != (unsigned long)iocb->aio_buf) ||
	    (iocb->aio_nbytes != (size_t)iocb->aio_nbytes) ||
	    ((ssize_t)iocb->aio_nbytes < 0)
	   )) {
		pr_debug("EINVAL: io_submit: overflow check\n");
		return -EINVAL;
	}

	req = aio_get_req(ctx);
	if (unlikely(!req))
		return -EAGAIN;

	req->ki_filp = fget(iocb->aio_fildes);
	if (unlikely(!req->ki_filp)) {
		ret = -EBADF;
		goto out_put_req;
	}

	if (iocb->aio_flags & IOCB_FLAG_RESFD) {
		/*
		 * If the IOCB_FLAG_RESFD flag of aio_flags is set, get an
		 * instance of the file* now. The file descriptor must be
		 * an eventfd() fd, and will be signaled for each completed
		 * event using the eventfd_signal() function.
		 */
		req->ki_eventfd = eventfd_ctx_fdget((int) iocb->aio_resfd);
		if (IS_ERR(req->ki_eventfd)) {
			ret = PTR_ERR(req->ki_eventfd);
			req->ki_eventfd = NULL;
			goto out_put_req;
		}
	}

	ret = put_user(KIOCB_KEY, &user_iocb->aio_key);
	if (unlikely(ret)) {
		pr_debug("EFAULT: aio_key\n");
		goto out_put_req;
	}

	req->ki_obj.user = user_iocb;
	req->ki_user_data = iocb->aio_data;
	req->ki_pos = iocb->aio_offset;
	req->ki_nbytes = iocb->aio_nbytes;

	ret = aio_run_iocb(req, iocb->aio_lio_opcode,
			   (char __user *)(unsigned long)iocb->aio_buf,
			   compat);
	if (ret)
		goto out_put_req;

	return 0;
out_put_req:
	put_reqs_available(ctx, 1);
	kiocb_free(req);
	return ret;
}

long do_io_submit(aio_context_t ctx_id, long nr,
		  struct iocb __user *__user *iocbpp, bool compat)
{
	struct kioctx *ctx;
	long ret = 0;
	int i = 0;

	if (unlikely(nr < 0))
		return -EINVAL;

	if (unlikely(nr > LONG_MAX/sizeof(*iocbpp)))
		nr = LONG_MAX/sizeof(*iocbpp);

	if (unlikely(!access_ok(VERIFY_READ, iocbpp, (nr*sizeof(*iocbpp)))))
		return -EFAULT;

	ctx = lookup_ioctx(ctx_id);
	if (unlikely(!ctx)) {
		pr_debug("EINVAL: invalid context id\n");
		return -EINVAL;
	}

	/*
	 * AKPM: should this return a partial result if some of the IOs were
	 * successfully submitted?
	 */
	for (i=0; i<nr; i++) {
		struct iocb __user *user_iocb;
		struct iocb tmp;

		if (unlikely(__get_user(user_iocb, iocbpp + i))) {
			ret = -EFAULT;
			break;
		}

		if (unlikely(copy_from_user(&tmp, user_iocb, sizeof(tmp)))) {
			ret = -EFAULT;
			break;
		}

		ret = io_submit_one(ctx, user_iocb, &tmp, compat);
		if (ret)
			break;
	}

	percpu_ref_put(&ctx->users);
	return i ? i : ret;
}

/* sys_io_submit:
 *	Queue the nr iocbs pointed to by iocbpp for processing.  Returns
 *	the number of iocbs queued.  May return -EINVAL if the aio_context
 *	specified by ctx_id is invalid, if nr is < 0, if the iocb at
 *	*iocbpp[0] is not properly initialized, if the operation specified
 *	is invalid for the file descriptor in the iocb.  May fail with
 *	-EFAULT if any of the data structures point to invalid data.  May
 *	fail with -EBADF if the file descriptor specified in the first
 *	iocb is invalid.  May fail with -EAGAIN if insufficient resources
 *	are available to queue any iocbs.  Will return 0 if nr is 0.  Will
 *	fail with -ENOSYS if not implemented.
 */
SYSCALL_DEFINE3(io_submit, aio_context_t, ctx_id, long, nr,
		struct iocb __user * __user *, iocbpp)
{
	return do_io_submit(ctx_id, nr, iocbpp, 0);
}

/* lookup_kiocb
 *	Finds a given iocb for cancellation.
 */
static struct kiocb *lookup_kiocb(struct kioctx *ctx, struct iocb __user *iocb,
				  u32 key)
{
	struct list_head *pos;

	assert_spin_locked(&ctx->ctx_lock);

	if (key != KIOCB_KEY)
		return NULL;

	/* TODO: use a hash or array, this sucks. */
	list_for_each(pos, &ctx->active_reqs) {
		struct kiocb *kiocb = list_kiocb(pos);
		if (kiocb->ki_obj.user == iocb)
			return kiocb;
	}
	return NULL;
}

/* sys_io_cancel:
 *	Attempts to cancel an iocb previously passed to io_submit.  If
 *	the operation is successfully cancelled, the resulting event is
 *	copied into the memory pointed to by result without being placed
 *	into the completion queue and 0 is returned.  May fail with
 *	-EFAULT if any of the data structures pointed to are invalid.
 *	May fail with -EINVAL if aio_context specified by ctx_id is
 *	invalid.  May fail with -EAGAIN if the iocb specified was not
 *	cancelled.  Will fail with -ENOSYS if not implemented.
 */
SYSCALL_DEFINE3(io_cancel, aio_context_t, ctx_id, struct iocb __user *, iocb,
		struct io_event __user *, result)
{
	struct kioctx *ctx;
	struct kiocb *kiocb;
	u32 key;
	int ret;

	ret = get_user(key, &iocb->aio_key);
	if (unlikely(ret))
		return -EFAULT;

	ctx = lookup_ioctx(ctx_id);
	if (unlikely(!ctx))
		return -EINVAL;

	spin_lock_irq(&ctx->ctx_lock);

	kiocb = lookup_kiocb(ctx, iocb, key);
	if (kiocb)
		ret = kiocb_cancel(ctx, kiocb);
	else
		ret = -EINVAL;

	spin_unlock_irq(&ctx->ctx_lock);

	if (!ret) {
		/*
		 * The result argument is no longer used - the io_event is
		 * always delivered via the ring buffer. -EINPROGRESS indicates
		 * cancellation is progress:
		 */
		ret = -EINPROGRESS;
	}

	percpu_ref_put(&ctx->users);

	return ret;
}

/* io_getevents:
 *	Attempts to read at least min_nr events and up to nr events from
 *	the completion queue for the aio_context specified by ctx_id. If
 *	it succeeds, the number of read events is returned. May fail with
 *	-EINVAL if ctx_id is invalid, if min_nr is out of range, if nr is
 *	out of range, if timeout is out of range.  May fail with -EFAULT
 *	if any of the memory specified is invalid.  May return 0 or
 *	< min_nr if the timeout specified by timeout has elapsed
 *	before sufficient events are available, where timeout == NULL
 *	specifies an infinite timeout. Note that the timeout pointed to by
 *	timeout is relative.  Will fail with -ENOSYS if not implemented.
 */
SYSCALL_DEFINE5(io_getevents, aio_context_t, ctx_id,
		long, min_nr,
		long, nr,
		struct io_event __user *, events,
		struct timespec __user *, timeout)
{
	struct kioctx *ioctx = lookup_ioctx(ctx_id);
	long ret = -EINVAL;

	if (likely(ioctx)) {
		if (likely(min_nr <= nr && min_nr >= 0))
			ret = read_events(ioctx, min_nr, nr, events, timeout);
		percpu_ref_put(&ioctx->users);
	}
	return ret;
}
