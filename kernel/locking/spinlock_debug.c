/*
 * Copyright 2005, Red Hat, Inc., Ingo Molnar
 * Released under the General Public License (GPL).
 *
 * This file contains the spinlock/rwlock implementations for
 * DEBUG_SPINLOCK.
 */

#include <linux/spinlock.h>
#include <linux/nmi.h>
#include <linux/interrupt.h>
#include <linux/debug_locks.h>
#include <linux/delay.h>
#include <linux/export.h>
#include <linux/bug.h>

void __raw_spin_lock_init(raw_spinlock_t *lock, const char *name,
			  struct lock_class_key *key)
{
#ifdef CONFIG_DEBUG_LOCK_ALLOC
	/*
	 * Make sure we are not reinitializing a held lock:
	 */
	debug_check_no_locks_freed((void *)lock, sizeof(*lock));
	lockdep_init_map(&lock->dep_map, name, key, 0);
#endif
	lock->raw_lock = (arch_spinlock_t)__ARCH_SPIN_LOCK_UNLOCKED;
	lock->magic = SPINLOCK_MAGIC;
	lock->owner = SPINLOCK_OWNER_INIT;
	lock->owner_cpu = -1;
}

EXPORT_SYMBOL(__raw_spin_lock_init);

void __rwlock_init(rwlock_t *lock, const char *name,
		   struct lock_class_key *key)
{
#ifdef CONFIG_DEBUG_LOCK_ALLOC
	/*
	 * Make sure we are not reinitializing a held lock:
	 */
	debug_check_no_locks_freed((void *)lock, sizeof(*lock));
	lockdep_init_map(&lock->dep_map, name, key, 0);
#endif
	lock->raw_lock = (arch_rwlock_t) __ARCH_RW_LOCK_UNLOCKED;
	lock->magic = RWLOCK_MAGIC;
	lock->owner = SPINLOCK_OWNER_INIT;
	lock->owner_cpu = -1;
}

EXPORT_SYMBOL(__rwlock_init);

static void spin_dump(raw_spinlock_t *lock, const char *msg)
{
	struct task_struct *owner = NULL;

	if (lock->owner && lock->owner != SPINLOCK_OWNER_INIT)
		owner = lock->owner;
	printk(KERN_EMERG "BUG: spinlock %s on CPU#%d, %s/%d\n",
		msg, raw_smp_processor_id(),
		current->comm, task_pid_nr(current));
	printk(KERN_EMERG " lock: %pS, .magic: %08x, .owner: %s/%d, "
			".owner_cpu: %d\n",
		lock, lock->magic,
		owner ? owner->comm : "<none>",
		owner ? task_pid_nr(owner) : -1,
		lock->owner_cpu);
	BUG_ON(PANIC_CORRUPTION);
	dump_stack();
}

static void spin_bug(raw_spinlock_t *lock, const char *msg)
{
	if (!debug_locks_off())
		return;

	spin_dump(lock, msg);
}

#define SPIN_BUG_ON(cond, lock, msg) if (unlikely(cond)) spin_bug(lock, msg)

static inline void
debug_spin_lock_before(raw_spinlock_t *lock)
{
	SPIN_BUG_ON(lock->magic != SPINLOCK_MAGIC, lock, "bad magic");
	SPIN_BUG_ON(lock->owner == current, lock, "recursion");
	SPIN_BUG_ON(lock->owner_cpu == raw_smp_processor_id(),
							lock, "cpu recursion");
}

static inline void debug_spin_lock_after(raw_spinlock_t *lock)
{
	lock->owner_cpu = raw_smp_processor_id();
	lock->owner = current;
}

static inline void debug_spin_unlock(raw_spinlock_t *lock)
{
	SPIN_BUG_ON(lock->magic != SPINLOCK_MAGIC, lock, "bad magic");
	SPIN_BUG_ON(!raw_spin_is_locked(lock), lock, "already unlocked");
	SPIN_BUG_ON(lock->owner != current, lock, "wrong owner");
	SPIN_BUG_ON(lock->owner_cpu != raw_smp_processor_id(),
							lock, "wrong CPU");
	lock->owner = SPINLOCK_OWNER_INIT;
	lock->owner_cpu = -1;
}

static void __spin_lock_debug(raw_spinlock_t *lock)
{
	u64 i;
	u64 loops = (loops_per_jiffy * HZ);

	for (i = 0; i < loops; i++) {
		if (arch_spin_trylock(&lock->raw_lock))
			return;
		__delay(1);
	}
	/* lockup suspected */
	spin_dump(lock, "lockup");
#ifdef CONFIG_SMP
	trigger_all_cpu_backtrace();
#endif

       /*
        * The trylock above was causing a livelock.  Give the lower level arch
        * specific lock code a chance to acquire the lock. We have already
        * printed a warning/backtrace at this point. The non-debug arch
        * specific code might actually succeed in acquiring the lock.  If it is
        * not successful, the end-result is the same - there is no forward
        * progress.
        */
       arch_spin_lock(&lock->raw_lock);
}

void do_raw_spin_lock(raw_spinlock_t *lock)
{
	debug_spin_lock_before(lock);
	if (unlikely(!arch_spin_trylock(&lock->raw_lock)))
		__spin_lock_debug(lock);
	debug_spin_lock_after(lock);
}

int do_raw_spin_trylock(raw_spinlock_t *lock)
{
	int ret = arch_spin_trylock(&lock->raw_lock);

	if (ret)
		debug_spin_lock_after(lock);
#ifndef CONFIG_SMP
	/*
	 * Must not happen on UP:
	 */
	SPIN_BUG_ON(!ret, lock, "trylock failure on UP");
#endif
	return ret;
}

void do_raw_spin_unlock(raw_spinlock_t *lock)
{
	debug_spin_unlock(lock);
	arch_spin_unlock(&lock->raw_lock);
}

static void rwlock_bug(rwlock_t *lock, const char *msg)
{
	if (!debug_locks_off())
		return;

	printk(KERN_EMERG "BUG: rwlock %s on CPU#%d, %s/%d, %p\n",
		msg, raw_smp_processor_id(), current->comm,
		task_pid_nr(current), lock);
	dump_stack();
}

#define RWLOCK_BUG_ON(cond, lock, msg) if (unlikely(cond)) rwlock_bug(lock, msg)

#if 0		/* __write_lock_debug() can lock up - maybe this can too? */
static void __read_lock_debug(rwlock_t *lock)
{
	u64 i;
	u64 loops = loops_per_jiffy * HZ;
	int print_once = 1;

	for (;;) {
		for (i = 0; i < loops; i++) {
			if (arch_read_trylock(&lock->raw_lock))
				return;
			__delay(1);
		}
		/* lockup suspected: */
		if (print_once) {
			print_once = 0;
			printk(KERN_EMERG "BUG: read-lock lockup on CPU#%d, "
					"%s/%d, %p\n",
				raw_smp_processor_id(), current->comm,
				current->pid, lock);
			dump_stack();
		}
	}
}
#endif

void do_raw_read_lock(rwlock_t *lock)
{
	RWLOCK_BUG_ON(lock->magic != RWLOCK_MAGIC, lock, "bad magic");
	arch_read_lock(&lock->raw_lock);
}

int do_raw_read_trylock(rwlock_t *lock)
{
	int ret = arch_read_trylock(&lock->raw_lock);

#ifndef CONFIG_SMP
	/*
	 * Must not happen on UP:
	 */
	RWLOCK_BUG_ON(!ret, lock, "trylock failure on UP");
#endif
	return ret;
}

void do_raw_read_unlock(rwlock_t *lock)
{
	RWLOCK_BUG_ON(lock->magic != RWLOCK_MAGIC, lock, "bad magic");
	arch_read_unlock(&lock->raw_lock);
}

static inline void debug_write_lock_before(rwlock_t *lock)
{
	RWLOCK_BUG_ON(lock->magic != RWLOCK_MAGIC, lock, "bad magic");
	RWLOCK_BUG_ON(lock->owner == current, lock, "recursion");
	RWLOCK_BUG_ON(lock->owner_cpu == raw_smp_processor_id(),
							lock, "cpu recursion");
}

static inline void debug_write_lock_after(rwlock_t *lock)
{
	lock->owner_cpu = raw_smp_processor_id();
	lock->owner = current;
}

static inline void debug_write_unlock(rwlock_t *lock)
{
	RWLOCK_BUG_ON(lock->magic != RWLOCK_MAGIC, lock, "bad magic");
	RWLOCK_BUG_ON(lock->owner != current, lock, "wrong owner");
	RWLOCK_BUG_ON(lock->owner_cpu != raw_smp_processor_id(),
							lock, "wrong CPU");
	lock->owner = SPINLOCK_OWNER_INIT;
	lock->owner_cpu = -1;
}

#if 0		/* This can cause lockups */
static void __write_lock_debug(rwlock_t *lock)
{
	u64 i;
	u64 loops = loops_per_jiffy * HZ;
	int print_once = 1;

	for (;;) {
		for (i = 0; i < loops; i++) {
			if (arch_write_trylock(&lock->raw_lock))
				return;
			__delay(1);
		}
		/* lockup suspected: */
		if (print_once) {
			print_once = 0;
			printk(KERN_EMERG "BUG: write-lock lockup on CPU#%d, "
					"%s/%d, %p\n",
				raw_smp_processor_id(), current->comm,
				current->pid, lock);
			dump_stack();
		}
	}
}
#endif

void do_raw_write_lock(rwlock_t *lock)
{
	debug_write_lock_before(lock);
	arch_write_lock(&lock->raw_lock);
	debug_write_lock_after(lock);
}

int do_raw_write_trylock(rwlock_t *lock)
{
	int ret = arch_write_trylock(&lock->raw_lock);

	if (ret)
		debug_write_lock_after(lock);
#ifndef CONFIG_SMP
	/*
	 * Must not happen on UP:
	 */
	RWLOCK_BUG_ON(!ret, lock, "trylock failure on UP");
#endif
	return ret;
}

void do_raw_write_unlock(rwlock_t *lock)
{
	debug_write_unlock(lock);
	arch_write_unlock(&lock->raw_lock);
}
