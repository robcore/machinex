#ifndef _BLK_CGROUP_H
#define _BLK_CGROUP_H
/*
 * Common Block IO controller cgroup interface
 *
 * Based on ideas and code from CFQ, CFS and BFQ:
 * Copyright (C) 2003 Jens Axboe <axboe@kernel.dk>
 *
 * Copyright (C) 2008 Fabio Checconi <fabio@gandalf.sssup.it>
 *		      Paolo Valente <paolo.valente@unimore.it>
 *
 * Copyright (C) 2009 Vivek Goyal <vgoyal@redhat.com>
 * 	              Nauman Rafique <nauman@google.com>
 */

#include <linux/cgroup.h>
#include <linux/u64_stats_sync.h>

enum blkio_policy_id {
	BLKIO_POLICY_PROP = 0,		/* Proportional Bandwidth division */
	BLKIO_POLICY_THROTL,		/* Throttling */

	BLKIO_NR_POLICIES,
};

/* Max limits for throttle policy */
#define THROTL_IOPS_MAX		UINT_MAX

#if defined(CONFIG_BLK_CGROUP) || defined(CONFIG_BLK_CGROUP_MODULE)

#ifndef CONFIG_BLK_CGROUP
/* When blk-cgroup is a module, its subsys_id isn't a compile-time constant */
extern struct cgroup_subsys blkio_subsys;
#define blkio_subsys_id blkio_subsys.subsys_id
#endif

enum stat_type {
	/* Total time spent (in ns) between request dispatch to the driver and
	 * request completion for IOs doen by this cgroup. This may not be
	 * accurate when NCQ is turned on. */
	BLKIO_STAT_SERVICE_TIME = 0,
	/* Total time spent waiting in scheduler queue in ns */
	BLKIO_STAT_WAIT_TIME,
	/* Number of IOs queued up */
	BLKIO_STAT_QUEUED,
	/* All the single valued stats go below this */
	BLKIO_STAT_TIME,
#ifdef CONFIG_DEBUG_BLK_CGROUP
	/* Time not charged to this cgroup */
	BLKIO_STAT_UNACCOUNTED_TIME,
	BLKIO_STAT_AVG_QUEUE_SIZE,
	BLKIO_STAT_IDLE_TIME,
	BLKIO_STAT_EMPTY_TIME,
	BLKIO_STAT_GROUP_WAIT_TIME,
	BLKIO_STAT_DEQUEUE
#endif
};

/* Per cpu stats */
enum stat_type_cpu {
	BLKIO_STAT_CPU_SECTORS,
	/* Total bytes transferred */
	BLKIO_STAT_CPU_SERVICE_BYTES,
	/* Total IOs serviced, post merge */
	BLKIO_STAT_CPU_SERVICED,
	/* Number of IOs merged */
	BLKIO_STAT_CPU_MERGED,
	BLKIO_STAT_CPU_NR
};

enum stat_sub_type {
	BLKIO_STAT_READ = 0,
	BLKIO_STAT_WRITE,
	BLKIO_STAT_SYNC,
	BLKIO_STAT_ASYNC,
	BLKIO_STAT_TOTAL
};

/* blkg state flags */
enum blkg_state_flags {
	BLKG_waiting = 0,
	BLKG_idling,
	BLKG_empty,
};

/* cgroup files owned by proportional weight policy */
enum blkcg_file_name_prop {
	BLKIO_PROP_weight = 1,
	BLKIO_PROP_weight_device,
	BLKIO_PROP_io_service_bytes,
	BLKIO_PROP_io_serviced,
	BLKIO_PROP_time,
	BLKIO_PROP_sectors,
	BLKIO_PROP_unaccounted_time,
	BLKIO_PROP_io_service_time,
	BLKIO_PROP_io_wait_time,
	BLKIO_PROP_io_merged,
	BLKIO_PROP_io_queued,
	BLKIO_PROP_avg_queue_size,
	BLKIO_PROP_group_wait_time,
	BLKIO_PROP_idle_time,
	BLKIO_PROP_empty_time,
	BLKIO_PROP_dequeue,
};

/* cgroup files owned by throttle policy */
enum blkcg_file_name_throtl {
	BLKIO_THROTL_read_bps_device,
	BLKIO_THROTL_write_bps_device,
	BLKIO_THROTL_read_iops_device,
	BLKIO_THROTL_write_iops_device,
	BLKIO_THROTL_io_service_bytes,
	BLKIO_THROTL_io_serviced,
};

struct blkio_cgroup {
	struct cgroup_css css;
	unsigned int weight;
	spinlock_t lock;
	struct hlist_head blkg_list;
};

struct blkio_group_stats {
	/* total disk time and nr sectors dispatched by this group */
	uint64_t time;
	uint64_t stat_arr[BLKIO_STAT_QUEUED + 1][BLKIO_STAT_TOTAL];
#ifdef CONFIG_DEBUG_BLK_CGROUP
	/* Time not charged to this cgroup */
	uint64_t unaccounted_time;

	/* Sum of number of IOs queued across all samples */
	uint64_t avg_queue_size_sum;
	/* Count of samples taken for average */
	uint64_t avg_queue_size_samples;
	/* How many times this group has been removed from service tree */
	unsigned long dequeue;

	/* Total time spent waiting for it to be assigned a timeslice. */
	uint64_t group_wait_time;
	uint64_t start_group_wait_time;

	/* Time spent idling for this blkio_group */
	uint64_t idle_time;
	uint64_t start_idle_time;
	/*
	 * Total time when we have requests queued and do not contain the
	 * current active queue.
	 */
	uint64_t empty_time;
	uint64_t start_empty_time;
	uint16_t flags;
#endif
};

/* Per cpu blkio group stats */
struct blkio_group_stats_cpu {
	uint64_t sectors;
	uint64_t stat_arr_cpu[BLKIO_STAT_CPU_NR][BLKIO_STAT_TOTAL];
	struct u64_stats_sync syncp;
};

struct blkio_group_conf {
	unsigned int weight;
	unsigned int iops[2];
	u64 bps[2];
};

/* per-blkg per-policy data */
struct blkg_policy_data {
	/* the blkg this per-policy data belongs to */
	struct blkio_group *blkg;

	/* Configuration */
	struct blkio_group_conf conf;

	struct blkio_group_stats stats;
	/* Per cpu stats pointer */
	struct blkio_group_stats_cpu __percpu *stats_cpu;

	/* pol->pdata_size bytes of private data used by policy impl */
	char pdata[] __aligned(__alignof__(unsigned long long));
};

struct blkio_group {
	/* Pointer to the associated request_queue, RCU protected */
	struct request_queue __rcu *q;
	struct list_head q_node[BLKIO_NR_POLICIES];
	struct hlist_node blkcg_node;
	struct blkio_cgroup *blkcg;
	/* Store cgroup path */
	char path[128];
	/* policy which owns this blk group */
	enum blkio_policy_id plid;
	/* reference count */
	int refcnt;

	/* Need to serialize the stats in the case of reset/update */
	spinlock_t stats_lock;
	struct blkg_policy_data *pd[BLKIO_NR_POLICIES];

	struct rcu_head rcu_head;
};

typedef void (blkio_init_group_fn)(struct blkio_group *blkg);
typedef void (blkio_link_group_fn)(struct request_queue *q,
			struct blkio_group *blkg);
typedef void (blkio_unlink_group_fn)(struct request_queue *q,
			struct blkio_group *blkg);
typedef bool (blkio_clear_queue_fn)(struct request_queue *q);
typedef void (blkio_update_group_weight_fn)(struct request_queue *q,
			struct blkio_group *blkg, unsigned int weight);
typedef void (blkio_update_group_read_bps_fn)(struct request_queue *q,
			struct blkio_group *blkg, u64 read_bps);
typedef void (blkio_update_group_write_bps_fn)(struct request_queue *q,
			struct blkio_group *blkg, u64 write_bps);
typedef void (blkio_update_group_read_iops_fn)(struct request_queue *q,
			struct blkio_group *blkg, unsigned int read_iops);
typedef void (blkio_update_group_write_iops_fn)(struct request_queue *q,
			struct blkio_group *blkg, unsigned int write_iops);

struct blkio_policy_ops {
	blkio_init_group_fn *blkio_init_group_fn;
	blkio_link_group_fn *blkio_link_group_fn;
	blkio_unlink_group_fn *blkio_unlink_group_fn;
	blkio_clear_queue_fn *blkio_clear_queue_fn;
	blkio_update_group_weight_fn *blkio_update_group_weight_fn;
	blkio_update_group_read_bps_fn *blkio_update_group_read_bps_fn;
	blkio_update_group_write_bps_fn *blkio_update_group_write_bps_fn;
	blkio_update_group_read_iops_fn *blkio_update_group_read_iops_fn;
	blkio_update_group_write_iops_fn *blkio_update_group_write_iops_fn;
};

struct blkio_policy_type {
	struct list_head list;
	struct blkio_policy_ops ops;
	enum blkio_policy_id plid;
	size_t pdata_size;		/* policy specific private data size */
};

extern int blkcg_init_queue(struct request_queue *q);
extern void blkcg_drain_queue(struct request_queue *q);
extern void blkcg_exit_queue(struct request_queue *q);

/* Blkio controller policy registration */
extern void blkio_policy_register(struct blkio_policy_type *);
extern void blkio_policy_unregister(struct blkio_policy_type *);
extern void blkg_destroy_all(struct request_queue *q);

/**
 * blkg_to_pdata - get policy private data
 * @blkg: blkg of interest
 * @pol: policy of interest
 *
 * Return pointer to private data associated with the @blkg-@pol pair.
 */
static inline void *blkg_to_pdata(struct blkio_group *blkg,
			      struct blkio_policy_type *pol)
{
	return blkg ? blkg->pd[pol->plid]->pdata : NULL;
}

/**
 * pdata_to_blkg - get blkg associated with policy private data
 * @pdata: policy private data of interest
 * @pol: policy @pdata is for
 *
 * @pdata is policy private data for @pol.  Determine the blkg it's
 * associated with.
 */
static inline struct blkio_group *pdata_to_blkg(void *pdata,
						struct blkio_policy_type *pol)
{
	if (pdata) {
		struct blkg_policy_data *pd =
			container_of(pdata, struct blkg_policy_data, pdata);
		return pd->blkg;
	}
	return NULL;
}

static inline char *blkg_path(struct blkio_group *blkg)
{
	return blkg->path;
}

/**
 * blkg_get - get a blkg reference
 * @blkg: blkg to get
 *
 * The caller should be holding queue_lock and an existing reference.
 */
static inline void blkg_get(struct blkio_group *blkg)
{
	lockdep_assert_held(blkg->q->queue_lock);
	WARN_ON_ONCE(!blkg->refcnt);
	blkg->refcnt++;
}

void __blkg_release(struct blkio_group *blkg);

/**
 * blkg_put - put a blkg reference
 * @blkg: blkg to put
 *
 * The caller should be holding queue_lock.
 */
static inline void blkg_put(struct blkio_group *blkg)
{
	lockdep_assert_held(blkg->q->queue_lock);
	WARN_ON_ONCE(blkg->refcnt <= 0);
	if (!--blkg->refcnt)
		__blkg_release(blkg);
}

#else

struct blkio_group {
};

struct blkio_policy_type {
};

static inline int blkcg_init_queue(struct request_queue *q) { return 0; }
static inline void blkcg_drain_queue(struct request_queue *q) { }
static inline void blkcg_exit_queue(struct request_queue *q) { }
static inline void blkio_policy_register(struct blkio_policy_type *blkiop) { }
static inline void blkio_policy_unregister(struct blkio_policy_type *blkiop) { }
static inline void blkg_destroy_all(struct request_queue *q) { }

static inline void *blkg_to_pdata(struct blkio_group *blkg,
				struct blkio_policy_type *pol) { return NULL; }
static inline struct blkio_group *pdata_to_blkg(void *pdata,
				struct blkio_policy_type *pol) { return NULL; }
static inline char *blkg_path(struct blkio_group *blkg) { return NULL; }
static inline void blkg_get(struct blkio_group *blkg) { }
static inline void blkg_put(struct blkio_group *blkg) { }

#endif

#define BLKIO_WEIGHT_MIN	10
#define BLKIO_WEIGHT_MAX	1000
#define BLKIO_WEIGHT_DEFAULT	500

#ifdef CONFIG_DEBUG_BLK_CGROUP
void blkiocg_update_avg_queue_size_stats(struct blkio_group *blkg,
					 struct blkio_policy_type *pol);
void blkiocg_update_dequeue_stats(struct blkio_group *blkg,
				  struct blkio_policy_type *pol,
				  unsigned long dequeue);
void blkiocg_update_set_idle_time_stats(struct blkio_group *blkg,
					struct blkio_policy_type *pol);
void blkiocg_update_idle_time_stats(struct blkio_group *blkg,
				    struct blkio_policy_type *pol);
void blkiocg_set_start_empty_time(struct blkio_group *blkg,
				  struct blkio_policy_type *pol);

#define BLKG_FLAG_FNS(name)						\
static inline void blkio_mark_blkg_##name(				\
		struct blkio_group_stats *stats)			\
{									\
	stats->flags |= (1 << BLKG_##name);				\
}									\
static inline void blkio_clear_blkg_##name(				\
		struct blkio_group_stats *stats)			\
{									\
	stats->flags &= ~(1 << BLKG_##name);				\
}									\
static inline int blkio_blkg_##name(struct blkio_group_stats *stats)	\
{									\
	return (stats->flags & (1 << BLKG_##name)) != 0;		\
}									\

BLKG_FLAG_FNS(waiting)
BLKG_FLAG_FNS(idling)
BLKG_FLAG_FNS(empty)
#undef BLKG_FLAG_FNS
#else
static inline void blkiocg_update_avg_queue_size_stats(struct blkio_group *blkg,
			struct blkio_policy_type *pol) { }
static inline void blkiocg_update_dequeue_stats(struct blkio_group *blkg,
			struct blkio_policy_type *pol, unsigned long dequeue) { }
static inline void blkiocg_update_set_idle_time_stats(struct blkio_group *blkg,
			struct blkio_policy_type *pol) { }
static inline void blkiocg_update_idle_time_stats(struct blkio_group *blkg,
			struct blkio_policy_type *pol) { }
static inline void blkiocg_set_start_empty_time(struct blkio_group *blkg,
			struct blkio_policy_type *pol) { }
#endif

#if defined(CONFIG_BLK_CGROUP) || defined(CONFIG_BLK_CGROUP_MODULE)
extern struct blkio_cgroup blkio_root_cgroup;
extern struct blkio_cgroup *cgroup_to_blkio_cgroup(struct cgroup *cgroup);
extern struct blkio_cgroup *task_blkio_cgroup(struct task_struct *tsk);
extern int blkiocg_del_blkio_group(struct blkio_group *blkg);
extern struct blkio_group *blkg_lookup(struct blkio_cgroup *blkcg,
				       struct request_queue *q,
				       enum blkio_policy_id plid);
struct blkio_group *blkg_lookup_create(struct blkio_cgroup *blkcg,
				       struct request_queue *q,
				       enum blkio_policy_id plid,
				       bool for_root);
void blkiocg_update_timeslice_used(struct blkio_group *blkg,
				   struct blkio_policy_type *pol,
				   unsigned long time,
				   unsigned long unaccounted_time);
void blkiocg_update_dispatch_stats(struct blkio_group *blkg,
				   struct blkio_policy_type *pol,
				   uint64_t bytes, bool direction, bool sync);
void blkiocg_update_completion_stats(struct blkio_group *blkg,
				     struct blkio_policy_type *pol,
				     uint64_t start_time,
				     uint64_t io_start_time, bool direction,
				     bool sync);
void blkiocg_update_io_merged_stats(struct blkio_group *blkg,
				    struct blkio_policy_type *pol,
				    bool direction, bool sync);
void blkiocg_update_io_add_stats(struct blkio_group *blkg,
				 struct blkio_policy_type *pol,
				 struct blkio_group *curr_blkg, bool direction,
				 bool sync);
void blkiocg_update_io_remove_stats(struct blkio_group *blkg,
				    struct blkio_policy_type *pol,
				    bool direction, bool sync);
#else
struct cgroup;
static inline struct blkio_cgroup *
cgroup_to_blkio_cgroup(struct cgroup *cgroup) { return NULL; }
static inline struct blkio_cgroup *
task_blkio_cgroup(struct task_struct *tsk) { return NULL; }

static inline int
blkiocg_del_blkio_group(struct blkio_group *blkg) { return 0; }

static inline struct blkio_group *blkg_lookup(struct blkio_cgroup *blkcg,
					      void *key) { return NULL; }
static inline void blkiocg_update_timeslice_used(struct blkio_group *blkg,
			struct blkio_policy_type *pol, unsigned long time,
			unsigned long unaccounted_time) { }
static inline void blkiocg_update_dispatch_stats(struct blkio_group *blkg,
			struct blkio_policy_type *pol, uint64_t bytes,
			bool direction, bool sync) { }
static inline void blkiocg_update_completion_stats(struct blkio_group *blkg,
			struct blkio_policy_type *pol, uint64_t start_time,
			uint64_t io_start_time, bool direction, bool sync) { }
static inline void blkiocg_update_io_merged_stats(struct blkio_group *blkg,
			struct blkio_policy_type *pol, bool direction,
			bool sync) { }
static inline void blkiocg_update_io_add_stats(struct blkio_group *blkg,
			struct blkio_policy_type *pol,
			struct blkio_group *curr_blkg, bool direction,
			bool sync) { }
static inline void blkiocg_update_io_remove_stats(struct blkio_group *blkg,
			struct blkio_policy_type *pol, bool direction,
			bool sync) { }
#endif
#endif /* _BLK_CGROUP_H */
