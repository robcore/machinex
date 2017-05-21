#ifndef _LINUX_KTHREAD_H
#define _LINUX_KTHREAD_H
/* Simple interface for creating and stopping kernel threads without mess. */
#include <linux/err.h>
#include <linux/sched.h>

__printf(4, 5)
struct task_struct *kthread_create_on_node(int (*threadfn)(void *data),
					   void *data,
					   int node,
					   const char namefmt[], ...);

#define kthread_create(threadfn, data, namefmt, arg...) \
	kthread_create_on_node(threadfn, data, NUMA_NO_NODE, namefmt, ##arg)


struct task_struct *kthread_create_on_cpu(int (*threadfn)(void *data),
					  void *data,
					  unsigned int cpu,
					  const char *namefmt);

/**
 * kthread_run - create and wake a thread.
 * @threadfn: the function to run until signal_pending(current).
 * @data: data ptr for @threadfn.
 * @namefmt: printf-style name for the thread.
 *
 * Description: Convenient wrapper for kthread_create() followed by
 * wake_up_process().  Returns the kthread or ERR_PTR(-ENOMEM).
 */
#define kthread_run(threadfn, data, namefmt, ...)			   \
({									   \
	struct task_struct *__k						   \
		= kthread_create(threadfn, data, namefmt, ## __VA_ARGS__); \
	if (!IS_ERR(__k))						   \
		wake_up_process(__k);					   \
	__k;								   \
})

void kthread_bind(struct task_struct *k, unsigned int cpu);
void kthread_bind_mask(struct task_struct *k, const struct cpumask *mask);
int kthread_stop(struct task_struct *k);
bool kthread_should_stop(void);
bool kthread_should_park(void);
bool kthread_freezable_should_stop(bool *was_frozen);
void *kthread_data(struct task_struct *k);
void *kthread_probe_data(struct task_struct *k);
int kthread_park(struct task_struct *k);
void kthread_unpark(struct task_struct *k);
void kthread_parkme(void);

int kthreadd(void *unused);
extern struct task_struct *kthreadd_task;
extern int tsk_fork_get_node(struct task_struct *tsk);

/*
 * Simple work processor based on kthread.
 *
 * This provides easier way to make use of kthreads.  A kthread_work
 * can be queued and flushed using queue/kthread_flush_work()
 * respectively.  Queued kthread_works are processed by a kthread
 * running kthread_worker_fn().
 */
struct kthread_work;
typedef void (*kthread_work_func_t)(struct kthread_work *work);
void kthread_delayed_work_timer_fn(unsigned long __data);

struct kthread_worker {
	spinlock_t		lock;
	struct list_head	work_list;
	struct list_head	delayed_work_list;
	struct task_struct	*task;
	struct kthread_work	*current_work;
};

struct kthread_work {
	struct list_head	node;
	kthread_work_func_t	func;
	wait_queue_head_t	done;
	struct kthread_worker	*worker;
};

struct kthread_delayed_work {
	struct kthread_work work;
	struct timer_list timer;
};

#define KTHREAD_WORKER_INIT(worker)	{				\
	.lock = __SPIN_LOCK_UNLOCKED((worker).lock),			\
	.work_list = LIST_HEAD_INIT((worker).work_list),		\
	.delayed_work_list = LIST_HEAD_INIT((worker).delayed_work_list),\
	}

#define KTHREAD_WORK_INIT(work, fn)	{				\
	.node = LIST_HEAD_INIT((work).node),				\
	.func = (fn),							\
	.done = __WAIT_QUEUE_HEAD_INITIALIZER((work).done),		\
	}

#define KTHREAD_DELAYED_WORK_INIT(dwork, fn) {				\
	.work = KTHREAD_WORK_INIT((dwork).work, (fn)),			\
	.timer = __TIMER_INITIALIZER(kthread_delayed_work_timer_fn,	\
				     0, (unsigned long)&(dwork),	\
				     TIMER_IRQSAFE),			\
	}

#define DEFINE_KTHREAD_WORKER(worker)					\
	struct kthread_worker worker = KTHREAD_WORKER_INIT(worker)

#define DEFINE_KTHREAD_WORK(work, fn)					\
	struct kthread_work work = KTHREAD_WORK_INIT(work, fn)

#define DEFINE_KTHREAD_DELAYED_WORK(dwork, fn)				\
	struct kthread_delayed_work dwork =				\
		KTHREAD_DELAYED_WORK_INIT(dwork, fn)

/*
 * kthread_worker.lock and kthread_work.done need their own lockdep class
 * keys if they are defined on stack with lockdep enabled.  Use the
 * following macros when defining them on stack.
 */
#ifdef CONFIG_LOCKDEP
# define KTHREAD_WORKER_INIT_ONSTACK(worker)				\
	({ kthread_init_worker(&worker); worker; })
# define DEFINE_KTHREAD_WORKER_ONSTACK(worker)				\
	struct kthread_worker worker = KTHREAD_WORKER_INIT_ONSTACK(worker)
# define KTHREAD_WORK_INIT_ONSTACK(work, fn)				\
	({ init_kthread_work((&work), fn); work; })
# define DEFINE_KTHREAD_WORK_ONSTACK(work, fn)				\
	struct kthread_work work = KTHREAD_WORK_INIT_ONSTACK(work, fn)
#else
# define DEFINE_KTHREAD_WORKER_ONSTACK(worker) DEFINE_KTHREAD_WORKER(worker)
# define DEFINE_KTHREAD_WORK_ONSTACK(work, fn) DEFINE_KTHREAD_WORK(work, fn)
#endif

extern void __kthread_init_worker(struct kthread_worker *worker,
			const char *name, struct lock_class_key *key);

#define kthread_init_worker(worker)					\
	do {								\
		static struct lock_class_key __key;			\
		__kthread_init_worker((worker), "("#worker")->lock", &__key); \
	} while (0)

#define kthread_init_work(work, fn)					\
	do {								\
		memset((work), 0, sizeof(struct kthread_work));		\
		INIT_LIST_HEAD(&(work)->node);				\
		(work)->func = (fn);					\
		init_waitqueue_head(&(work)->done);			\
	} while (0)

#define kthread_init_delayed_work(dwork, fn)				\
	do {								\
		kthread_init_work(&(dwork)->work, (fn));		\
		__setup_timer(&(dwork)->timer,				\
			      kthread_delayed_work_timer_fn,		\
			      (unsigned long)(dwork),			\
			      TIMER_IRQSAFE);				\
	} while (0)

int kthread_worker_fn(void *worker_ptr);

__printf(1, 2)
struct kthread_worker *
kthread_create_worker(const char namefmt[], ...);

struct kthread_worker *
kthread_create_worker_on_cpu(int cpu, const char namefmt[], ...);

bool kthread_queue_work(struct kthread_worker *worker,
			struct kthread_work *work);

bool kthread_queue_delayed_work(struct kthread_worker *worker,
				struct kthread_delayed_work *dwork,
				unsigned long delay);

void kthread_flush_work(struct kthread_work *work);
void kthread_flush_worker(struct kthread_worker *worker);

void kthread_destroy_worker(struct kthread_worker *worker);

#endif /* _LINUX_KTHREAD_H */
