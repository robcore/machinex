/*
 * async.c: Asynchronous function calls for boot performance
 *
 * (C) Copyright 2009 Intel Corporation
 * Author: Arjan van de Ven <arjan@linux.intel.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2
 * of the License.
 */


/*

Goals and Theory of Operation

The primary goal of this feature is to reduce the kernel boot time,
by doing various independent hardware delays and discovery operations
decoupled and not strictly serialized.

More specifically, the asynchronous function call concept allows
certain operations (primarily during system boot) to happen
asynchronously, out of order, while these operations still
have their externally visible parts happen sequentially and in-order.
(not unlike how out-of-order CPUs retire their instructions in order)

Key to the asynchronous function call implementation is the concept of
a "sequence cookie" (which, although it has an abstracted type, can be
thought of as a monotonically incrementing number).

The async core will assign each scheduled event such a sequence cookie and
pass this to the called functions.

The asynchronously called function should before doing a globally visible
operation, such as registering device numbers, call the
async_synchronize_cookie() function and pass in its own cookie. The
async_synchronize_cookie() function will make sure that all asynchronous
operations that were scheduled prior to the operation corresponding with the
cookie have completed.

Subsystem/driver initialization code that scheduled asynchronous probe
functions, but which shares global resources with other drivers/subsystems
that do not use the asynchronous call feature, need to do a full
synchronization with the async_synchronize_full() function, before returning
from their init function. This is to maintain strict ordering between the
asynchronous and synchronous parts of the kernel.

*/

#include <linux/async.h>
#include <linux/atomic.h>
#include <linux/ktime.h>
#include <linux/export.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/workqueue.h>

#include "workqueue_internal.h"

static async_cookie_t next_cookie = 1;

#define MAX_WORK	32768

static LIST_HEAD(async_pending);
static ASYNC_DOMAIN(async_dfl_domain);
static DEFINE_SPINLOCK(async_lock);

struct async_entry {
	struct list_head	list;
	struct work_struct	work;
	async_cookie_t		cookie;
	async_func_ptr		*func;
	void			*data;
	struct async_domain	*domain;
};

static DECLARE_WAIT_QUEUE_HEAD(async_done);

static atomic_t entry_count;


/*
 * MUST be called with the lock held!
 */
static async_cookie_t __lowest_in_progress(struct async_domain *domain)
{
	async_cookie_t first_running = next_cookie;	/* infinity value */
	async_cookie_t first_pending = next_cookie;	/* ditto */
	struct async_entry *entry;

	if (!running) { /* just check the entry count */
		if (atomic_read(&entry_count))
			return 0; /* smaller than any cookie */
		else
			return next_cookie;
	}

	/*
	 * Both running and pending lists are sorted but not disjoint.
	 * Take the first cookies from both and return the min.
	 */
	if (!list_empty(&domain->running)) {
		entry = list_first_entry(&domain->running, typeof(*entry), list);
		first_running = entry->cookie;
	}

	list_for_each_entry(entry, &async_pending, list) {
		if (entry->domain == domain) {
			first_pending = entry->cookie;
			break;
		}
	}

	return min(first_running, first_pending);
}

static async_cookie_t lowest_in_progress(struct async_domain *domain)
{
	unsigned long flags;
	async_cookie_t ret;

	spin_lock_irqsave(&async_lock, flags);
	ret = __lowest_in_progress(domain);
	spin_unlock_irqrestore(&async_lock, flags);
	return ret;
}

/*
 * pick the first pending entry and run it
 */
static void async_run_entry_fn(struct work_struct *work)
{
	struct async_entry *entry =
		container_of(work, struct async_entry, work);
	struct async_entry *pos;
	unsigned long flags;
	ktime_t uninitialized_var(calltime), delta, rettime;
	struct async_domain *domain = entry->domain;

	/* 1) move self to the running queue, make sure it stays sorted */
	spin_lock_irqsave(&async_lock, flags);
	list_for_each_entry_reverse(pos, &domain->running, list)
		if (entry->cookie < pos->cookie)
			break;
	list_move_tail(&entry->list, &pos->list);
	spin_unlock_irqrestore(&async_lock, flags);

	/* 2) run (and print duration) */
	if (initcall_debug && system_state == SYSTEM_BOOTING) {
		printk(KERN_DEBUG "calling  %lli_%pF @ %i\n",
			(long long)entry->cookie,
			entry->func, task_pid_nr(current));
		calltime = ktime_get();
	}
	entry->func(entry->data, entry->cookie);
	if (initcall_debug && system_state == SYSTEM_BOOTING) {
		rettime = ktime_get();
		delta = ktime_sub(rettime, calltime);
		printk(KERN_DEBUG "initcall %lli_%pF returned 0 after %lld usecs\n",
			(long long)entry->cookie,
			entry->func,
			(long long)ktime_to_ns(delta) >> 10);
	}

	/* 3) remove self from the running queue */
	spin_lock_irqsave(&async_lock, flags);
	list_del(&entry->list);

	/* 4) free the entry */
	kfree(entry);
	atomic_dec(&entry_count);

	spin_unlock_irqrestore(&async_lock, flags);

	/* 5) wake up any waiters */
	wake_up(&async_done);
}

static async_cookie_t __async_schedule(async_func_ptr *ptr, void *data, struct async_domain *domain)
{
	struct async_entry *entry;
	unsigned long flags;
	async_cookie_t newcookie;

	/* allow irq-off callers */
	entry = kzalloc(sizeof(struct async_entry), GFP_ATOMIC);

	/*
	 * If we're out of memory or if there's too much work
	 * pending already, we execute synchronously.
	 */
	if (!entry || atomic_read(&entry_count) > MAX_WORK) {
		kfree(entry);
		spin_lock_irqsave(&async_lock, flags);
		newcookie = next_cookie++;
		spin_unlock_irqrestore(&async_lock, flags);

		/* low on memory.. run synchronously */
		ptr(data, newcookie);
		return newcookie;
	}
	INIT_WORK(&entry->work, async_run_entry_fn);
	entry->func = ptr;
	entry->data = data;
	entry->domain = domain;

	spin_lock_irqsave(&async_lock, flags);
	newcookie = entry->cookie = next_cookie++;
	list_add_tail(&entry->list, &async_pending);
	atomic_inc(&entry_count);
	spin_unlock_irqrestore(&async_lock, flags);

	/* mark that this task has queued an async job, used by module init */
	current->flags |= PF_USED_ASYNC;

	/* schedule for execution */
	queue_work(system_unbound_wq, &entry->work);

	return newcookie;
}

/**
 * async_schedule - schedule a function for asynchronous execution
 * @ptr: function to execute asynchronously
 * @data: data pointer to pass to the function
 *
 * Returns an async_cookie_t that may be used for checkpointing later.
 * Note: This function may be called from atomic or non-atomic contexts.
 */
async_cookie_t async_schedule(async_func_ptr *ptr, void *data)
{
	return __async_schedule(ptr, data, &async_dfl_domain);
}
EXPORT_SYMBOL_GPL(async_schedule);

/**
 * async_schedule_domain - schedule a function for asynchronous execution within a certain domain
 * @ptr: function to execute asynchronously
 * @data: data pointer to pass to the function
 * @domain: the domain
 *
 * Returns an async_cookie_t that may be used for checkpointing later.
 * @domain may be used in the async_synchronize_*_domain() functions to
 * wait within a certain synchronization domain rather than globally.  A
 * synchronization domain is specified via @domain.  Note: This function
 * may be called from atomic or non-atomic contexts.
 */
async_cookie_t async_schedule_domain(async_func_ptr *ptr, void *data,
				     struct async_domain *domain)
{
	return __async_schedule(ptr, data, domain);
}
EXPORT_SYMBOL_GPL(async_schedule_domain);

/**
 * async_synchronize_full - synchronize all asynchronous function calls
 *
 * This function waits until all asynchronous function calls have been done.
 */
void async_synchronize_full(void)
{
	async_synchronize_cookie_domain(next_cookie, NULL);
}
EXPORT_SYMBOL_GPL(async_synchronize_full);

/**
 * async_synchronize_full_domain - synchronize all asynchronous function within a certain domain
 * @domain: the domain to synchronize
 *
 * This function waits until all asynchronous function calls for the
 * synchronization domain specified by @domain have been done.
 */
void async_synchronize_full_domain(struct async_domain *domain)
{
	async_synchronize_cookie_domain(next_cookie, domain);
}
EXPORT_SYMBOL_GPL(async_synchronize_full_domain);

/**
 * async_synchronize_cookie_domain - synchronize asynchronous function calls within a certain domain with cookie checkpointing
 * @cookie: async_cookie_t to use as checkpoint
 * @domain: the domain to synchronize
 *
 * This function waits until all asynchronous function calls for the
 * synchronization domain specified by @domain submitted prior to @cookie
 * have been done.
 */
void async_synchronize_cookie_domain(async_cookie_t cookie, struct async_domain *domain)
{
	ktime_t uninitialized_var(starttime), delta, endtime;

	if (initcall_debug && system_state == SYSTEM_BOOTING) {
		printk(KERN_DEBUG "async_waiting @ %i\n", task_pid_nr(current));
		starttime = ktime_get();
	}

	wait_event(async_done, lowest_in_progress(domain) >= cookie);

	if (initcall_debug && system_state == SYSTEM_BOOTING) {
		endtime = ktime_get();
		delta = ktime_sub(endtime, starttime);

		printk(KERN_DEBUG "async_continuing @ %i after %lli usec\n",
			task_pid_nr(current),
			(long long)ktime_to_ns(delta) >> 10);
	}
}
EXPORT_SYMBOL_GPL(async_synchronize_cookie_domain);

/**
 * async_synchronize_cookie - synchronize asynchronous function calls with cookie checkpointing
 * @cookie: async_cookie_t to use as checkpoint
 *
 * This function waits until all asynchronous function calls prior to @cookie
 * have been done.
 */
void async_synchronize_cookie(async_cookie_t cookie)
{
	async_synchronize_cookie_domain(cookie, &async_dfl_domain);
}
EXPORT_SYMBOL_GPL(async_synchronize_cookie);

/**
 * current_is_async - is %current an async worker task?
 *
 * Returns %true if %current is an async worker task.
 */
bool current_is_async(void)
{
	struct worker *worker = current_wq_worker();

	return worker && worker->current_func == async_run_entry_fn;
}
