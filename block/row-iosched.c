/*
 * ROW (Read Over Write) I/O scheduler.
 *
 * Copyright (c) 2012, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/* See Documentation/block/row-iosched.txt */

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/compiler.h>
#include <linux/blktrace_api.h>
#include <linux/jiffies.h>

/*
 * enum row_queue_prio - Priorities of the ROW queues
 *
 * This enum defines the priorities (and the number of queues)
 * the requests will be disptributed to. The higher priority -
 * the bigger is the dispatch quantum given to that queue.
 * ROWQ_PRIO_HIGH_READ - is the higher priority queue.
 *
 */
enum row_queue_prio {
	ROWQ_PRIO_HIGH_READ = 0,
	ROWQ_PRIO_REG_READ,
	ROWQ_PRIO_HIGH_SWRITE,
	ROWQ_PRIO_REG_SWRITE,
	ROWQ_PRIO_REG_WRITE,
	ROWQ_PRIO_LOW_READ,
	ROWQ_PRIO_LOW_SWRITE,
	ROWQ_MAX_PRIO,
};

/**
 * struct row_queue_params - ROW queue parameters
 * @idling_enabled: Flag indicating whether idling is enable on
 *			the queue
 * @quantum: Number of requests to be dispatched from this queue
 *			in a dispatch cycle
 * @is_urgent: Flags indicating whether the queue can notify on
 *			urgent requests
 *
 */
struct row_queue_params {
	bool idling_enabled;
	int quantum;
	bool is_urgent;
};

/*
 * This array holds the default values of the different configurables
 * for each ROW queue. Each row of the array holds the following values:
 * {idling_enabled, quantum, is_urgent}
 * Each row corresponds to a queue with the same index (according to
 * enum row_queue_prio)
 * Note: The quantums are valid inside their priority type. For example:
 *       For every 10 high priority read requests, 1 high priority sync
 *       write will be dispatched.
 *       For every 100 regular read requests 1 regular write request will
 *       be dispatched.
 */
static const struct row_queue_params row_queues_def[] = {
/* idling_enabled, quantum, is_urgent */
	{true, 100, true},	/* ROWQ_PRIO_HIGH_READ */
	{false, 5, false},	/* ROWQ_PRIO_HIGH_SWRITE */
	{true, 75, true},	/* ROWQ_PRIO_REG_READ */
	{false, 4, false},	/* ROWQ_PRIO_REG_SWRITE */
	{false, 4, false},	/* ROWQ_PRIO_REG_WRITE */
	{false, 3, false},	/* ROWQ_PRIO_LOW_READ */
	{false, 2, false}	/* ROWQ_PRIO_LOW_SWRITE */
};

/* Default values for idling on read queues (in msec) */
#define ROW_IDLE_TIME_MSEC 10
#define ROW_READ_FREQ_MSEC 25

/**
 * struct rowq_idling_data -  parameters for idling on the queue
 * @last_insert_time:	time the last request was inserted
 *			to the queue
 * @begin_idling:	flag indicating wether we should idle
 *
 */
struct rowq_idling_data {
	ktime_t			last_insert_time;
	bool			begin_idling;
};

/**
 * struct row_queue - requests grouping structure
 * @rdata:		parent row_data structure
 * @fifo:		fifo of requests
 * @prio:		queue priority (enum row_queue_prio)
 * @nr_dispatched:	number of requests already dispatched in
 *			the current dispatch cycle
 * @slice:		number of requests to dispatch in a cycle
 * @idle_data:		data for idling on queues
 *
 */
struct row_queue {
	struct row_data		*rdata;
	struct list_head	fifo;
	enum row_queue_prio	prio;

	unsigned int		nr_dispatched;
	unsigned int		slice;

	/* used only for READ queues */
	struct rowq_idling_data	idle_data;
};

/**
 * struct idling_data - data for idling on empty rqueue
 * @idle_time:		idling duration (msec)
 * @freq:		min time between two requests that
 *			triger idling (msec)
 * @idle_work:		pointer to struct delayed_work
 *
 */
struct idling_data {
	unsigned long			idle_time;
	unsigned long			freq;

	struct workqueue_struct	*idle_workqueue;
	struct delayed_work		idle_work;
};

/**
 * struct row_queue - Per block device rqueue structure
 * @dispatch_queue:	dispatch rqueue
 * @row_queues:		array of priority request queues with
 *			dispatch quantum per rqueue
 * @curr_queue:		index in the row_queues array of the
 *			currently serviced rqueue
 * @read_idle:		data for idling after READ request
 * @nr_reqs: nr_reqs[0] holds the number of all READ requests in
 *			scheduler, nr_reqs[1] holds the number of all WRITE
 *			requests in scheduler
 * @cycle_flags:	used for marking unserved queueus
 *
 */
struct row_data {
	struct request_queue		*dispatch_queue;

	struct {
		struct row_queue	rqueue;
		int			disp_quantum;
	} row_queues[ROWQ_MAX_PRIO];

	enum row_queue_prio		curr_queue;

	struct idling_data		read_idle;
	unsigned int			nr_reqs[2];

	unsigned int			cycle_flags;
};

#define RQ_ROWQ(rq) ((struct row_queue *) ((rq)->elv.priv[0]))

#define row_log(q, fmt, args...)   \
	blk_add_trace_msg(q, "%s():" fmt , __func__, ##args)
#define row_log_rowq(rdata, rowq_id, fmt, args...)		\
	blk_add_trace_msg(rdata->dispatch_queue, "rowq%d " fmt, \
		rowq_id, ##args)

static inline void row_mark_rowq_unserved(struct row_data *rd,
					 enum row_queue_prio qnum)
{
	rd->cycle_flags |= (1 << qnum);
}

static inline void row_clear_rowq_unserved(struct row_data *rd,
					  enum row_queue_prio qnum)
{
	rd->cycle_flags &= ~(1 << qnum);
}

static inline int row_rowq_unserved(struct row_data *rd,
				   enum row_queue_prio qnum)
{
	return rd->cycle_flags & (1 << qnum);
}

/******************** Static helper functions ***********************/
/*
 * kick_queue() - Wake up device driver queue thread
 * @work:	pointer to struct work_struct
 *
 * This is a idling delayed work function. It's purpose is to wake up the
 * device driver in order for it to start fetching requests.
 *
 */
static void kick_queue(struct work_struct *work)
{
	struct delayed_work *idle_work = to_delayed_work(work);
	struct idling_data *read_data =
		container_of(idle_work, struct idling_data, idle_work);
	struct row_data *rd =
		container_of(read_data, struct row_data, read_idle);

	row_log_rowq(rd, rd->curr_queue, "Performing delayed work");
	/* Mark idling process as done */
	rd->row_queues[rd->curr_queue].rqueue.idle_data.begin_idling = false;

	if (!(rd->nr_reqs[0] + rd->nr_reqs[1]))
		row_log(rd->dispatch_queue, "No requests in scheduler");
	else {
		spin_lock_irq(rd->dispatch_queue->queue_lock);
		__blk_run_queue(rd->dispatch_queue);
		spin_unlock_irq(rd->dispatch_queue->queue_lock);
	}
}

/*
 * row_restart_disp_cycle() - Restart the dispatch cycle
 * @rd:	pointer to struct row_data
 *
 * This function restarts the dispatch cycle by:
 * - Setting current queue to ROWQ_PRIO_HIGH_READ
 * - For each queue: reset the number of requests dispatched in
 *   the cycle
 */
static inline void row_restart_disp_cycle(struct row_data *rd)
{
	int i;

	for (i = 0; i < ROWQ_MAX_PRIO; i++)
		rd->row_queues[i].rqueue.nr_dispatched = 0;

	rd->curr_queue = ROWQ_PRIO_HIGH_READ;
	row_log(rd->dispatch_queue, "Restarting cycle");
}

static inline void row_get_next_queue(struct row_data *rd)
{
	rd->curr_queue++;
	if (rd->curr_queue == ROWQ_MAX_PRIO)
		row_restart_disp_cycle(rd);
}

/******************* Elevator callback functions *********************/

/*
 * row_add_request() - Add request to the scheduler
 * @q:	requests queue
 * @rq:	request to add
 *
 */
static void row_add_request(struct request_queue *q,
			    struct request *rq)
{
	struct row_data *rd = (struct row_data *)q->elevator->elevator_data;
	struct row_queue *rqueue = RQ_ROWQ(rq);
	s64 diff_ms;
	bool queue_was_empty = list_empty(&rqueue->fifo);
	unsigned long bv_page_flags = 0;

	if (rq->bio && rq->bio->bi_io_vec && rq->bio->bi_io_vec->bv_page)
		bv_page_flags = rq->bio->bi_io_vec->bv_page->flags;

	list_add_tail(&rq->queuelist, &rqueue->fifo);
	rd->nr_reqs[rq_data_dir(rq)]++;
	rq_set_fifo_time(rq, jiffies); /* for statistics*/

	if (row_queues_def[rqueue->prio].idling_enabled) {
		if (rd->rd_idle_data.idling_queue_idx == rqueue->prio &&
		    hrtimer_active(&rd->rd_idle_data.hr_timer)) {
			if (hrtimer_try_to_cancel(
				&rd->rd_idle_data.hr_timer) >= 0) {
				row_log_rowq(rd, rqueue->prio,
				    "Canceled delayed work on %d",
				    rd->rd_idle_data.idling_queue_idx);
				rd->rd_idle_data.idling_queue_idx =
					ROWQ_MAX_PRIO;
			}
		}
		diff_ms = ktime_to_ms(ktime_sub(ktime_get(),
				rqueue->idle_data.last_insert_time));
		if (unlikely(diff_ms < 0)) {
			pr_err("ROW BUG: %s diff_ms < 0", __func__);
			rqueue->idle_data.begin_idling = false;
			return;
		}

		if ((bv_page_flags & (1L << PG_readahead)) ||
		    (diff_ms < rd->rd_idle_data.freq_ms)) {
			rqueue->idle_data.begin_idling = true;
			row_log_rowq(rd, rqueue->prio, "Enable idling");
		} else {
			rqueue->idle_data.begin_idling = false;
			row_log_rowq(rd, rqueue->prio, "Disable idling (%ldms)",
				(long)diff_ms);
		}

		rqueue->idle_data.last_insert_time = ktime_get();
	}
	if (row_queues_def[rqueue->prio].is_urgent &&
	    !rd->pending_urgent_rq && !rd->urgent_in_flight) {
		/* Handle High Priority queues */
		if (rqueue->prio < ROWQ_REG_PRIO_IDX &&
		    rd->last_served_ioprio_class != IOPRIO_CLASS_RT) {
			row_log_rowq(rd, rqueue->prio,
				"added (high prio) urgent request");
			rq->cmd_flags |= REQ_URGENT;
			rd->pending_urgent_rq = rq;
		} else  if (row_rowq_unserved(rd, rqueue->prio)) {
			/* Handle Regular priotity queues */
			row_log_rowq(rd, rqueue->prio,
				"added urgent request (total on queue=%d)",
				rqueue->nr_req);
			rq->cmd_flags |= REQ_URGENT;
			rd->pending_urgent_rq = rq;
		}
	} else
		row_log_rowq(rd, rqueue->prio,
			"added request (total on queue=%d)", rqueue->nr_req);
}

/*
 * row_remove_request() -  Remove given request from scheduler
 * @q:	requests queue
 * @rq:	request to remove
 *
 */
static void row_remove_request(struct request_queue *q,
			       struct request *rq)
{
	struct row_data *rd = (struct row_data *)q->elevator->elevator_data;

	rq_fifo_clear(rq);
	rd->nr_reqs[rq_data_dir(rq)]--;
}

/*
 * row_dispatch_insert() - move request to dispatch queue
 * @rd:		pointer to struct row_data
 * @queue_idx:	index of the row_queue to dispatch from
 *
 * This function moves the next request to dispatch from
 * the given queue (row_queues[queue_idx]) to the dispatch queue
 *
 */
static void row_dispatch_insert(struct row_data *rd, int queue_idx)
{
	struct request *rq;

	rq = rq_entry_fifo(rd->row_queues[queue_idx].fifo.next);
	row_remove_request(rd->dispatch_queue, rq);
	elv_dispatch_add_tail(rd->dispatch_queue, rq);
	rd->row_queues[queue_idx].nr_dispatched++;
	row_clear_rowq_unserved(rd, queue_idx);
	row_log_rowq(rd, queue_idx, " Dispatched request nr_disp = %d",
		     rd->row_queues[queue_idx].nr_dispatched);
}

/*
 * row_get_ioprio_class_to_serve() - Return the next I/O priority
 *				      class to dispatch requests from
 * @rd:	pointer to struct row_data
 * @force:	flag indicating if forced dispatch
 *
 * This function returns the next I/O priority class to serve
 * {IOPRIO_CLASS_NONE, IOPRIO_CLASS_RT, IOPRIO_CLASS_BE, IOPRIO_CLASS_IDLE}.
 * If there are no more requests in scheduler or if we're idling on some queue
 * IOPRIO_CLASS_NONE will be returned.
 * If idling is scheduled on a lower priority queue than the one that needs
 * to be served, it will be canceled.
 *
 */
static int row_get_ioprio_class_to_serve(struct row_data *rd, int force)
{
	int i;
	int ret = IOPRIO_CLASS_NONE;

	if (!rd->nr_reqs[READ] && !rd->nr_reqs[WRITE]) {
		row_log(rd->dispatch_queue, "No more requests in scheduler");
		goto check_idling;
	}

	/* First, go over the high priority queues */
	for (i = 0; i < ROWQ_REG_PRIO_IDX; i++) {
		if (!list_empty(&rd->row_queues[i].fifo)) {
			if (hrtimer_active(&rd->rd_idle_data.hr_timer)) {
				if (hrtimer_try_to_cancel(
					&rd->rd_idle_data.hr_timer) >= 0) {
					row_log(rd->dispatch_queue,
					"Canceling delayed work on %d. RT pending",
					     rd->rd_idle_data.idling_queue_idx);
					rd->rd_idle_data.idling_queue_idx =
						ROWQ_MAX_PRIO;
				}
			}
			ret = IOPRIO_CLASS_RT;
			goto done;
		}
	}

	/*
	 * At the moment idling is implemented only for READ queues.
	 * If enabled on WRITE, this needs updating
	 */
	if (delayed_work_pending(&rd->rd_idle_data.idle_work)) {
		row_log(rd->dispatch_queue, "Delayed work pending. Exiting");
		goto done;
	}
check_idling:
	/* Check for (high priority) idling and enable if needed */
	for (i = 0; i < ROWQ_REG_PRIO_IDX && !force; i++) {
		if (rd->row_queues[i].idle_data.begin_idling &&
		    row_queues_def[i].idling_enabled)
			goto initiate_idling;
	}

	/* Regular priority queues */
	for (i = ROWQ_REG_PRIO_IDX; i < ROWQ_LOW_PRIO_IDX; i++) {
		if (list_empty(&rd->row_queues[i].fifo)) {
			/* We can idle only if this is not a forced dispatch */
			if (rd->row_queues[i].idle_data.begin_idling &&
			    !force && row_queues_def[i].idling_enabled)
				goto initiate_idling;
		} else {
			ret = IOPRIO_CLASS_BE;
			goto done;
		}
	}

	if (rd->nr_reqs[READ] || rd->nr_reqs[WRITE])
		ret = IOPRIO_CLASS_IDLE;
	goto done;

initiate_idling:
	if (!queue_delayed_work(rd->rd_idle_data.idle_wq,
	    &rd->rd_idle_data.idle_work, rd->rd_idle_data.idle_time)) {
		row_log(rd->dispatch_queue, "Work already on queue!");
		pr_err("ROW_BUG: Work already on queue!");
	} else {
		rd->rd_idle_data.idling_queue_idx = i;
		row_log_rowq(rd, i, "Scheduled delayed work on %d. exiting", i);
	}
done:
	return ret;
}

static void row_restart_cycle(struct row_data *rd,
				int start_idx, int end_idx)
{
	int i;

	row_dump_queues_stat(rd);
	for (i = start_idx; i < end_idx; i++) {
		if (rd->row_queues[i].nr_dispatched <
		    rd->row_queues[i].disp_quantum)
			row_mark_rowq_unserved(rd, i);
		rd->row_queues[i].nr_dispatched = 0;
	}
	row_log(rd->dispatch_queue, "Restarting cycle for class @ %d-%d",
		start_idx, end_idx);
}

/*
 * row_get_next_queue() - selects the next queue to dispatch from
 * @q:		requests queue
 * @rd:		pointer to struct row_data
 * @start_idx/end_idx: indexes in the row_queues array to select a queue
 *                 from.
 *
 * Return index of the queues to dispatch from. Error code if fails.
 *
 */
static int row_get_next_queue(struct request_queue *q, struct row_data *rd,
				int start_idx, int end_idx)
{
	int i = start_idx;
	bool restart = true;
	int ret = -EIO;

	do {
		if (list_empty(&rd->row_queues[i].fifo) ||
		    rd->row_queues[i].nr_dispatched >=
		    rd->row_queues[i].disp_quantum) {
			i++;
			if (i == end_idx && restart) {
				/* Restart cycle for this priority class */
				row_restart_cycle(rd, start_idx, end_idx);
				i = start_idx;
				restart = false;
			}
		} else {
			ret = i;
			break;
		}
	} while (i < end_idx);

	return ret;
}

/*
 * row_dispatch_requests() - selects the next request to dispatch
 * @q:		requests queue
 * @force:		flag indicating if forced dispatch
 *
 * Return 0 if no requests were moved to the dispatch queue.
 *	  1 otherwise
 *
 */
static int row_dispatch_requests(struct request_queue *q, int force)
{
	struct row_data *rd = (struct row_data *)q->elevator->elevator_data;
	int ret = 0, currq, ioprio_class_to_serve, start_idx, end_idx;

	if (force && hrtimer_active(&rd->rd_idle_data.hr_timer)) {
		if (hrtimer_try_to_cancel(&rd->rd_idle_data.hr_timer) >= 0) {
			row_log(rd->dispatch_queue,
				"Canceled delayed work on %d - forced dispatch",
				rd->rd_idle_data.idling_queue_idx);
			rd->rd_idle_data.idling_queue_idx = ROWQ_MAX_PRIO;
		}
	}

	ioprio_class_to_serve = row_get_ioprio_class_to_serve(rd, force);
	row_log(rd->dispatch_queue, "Dispatching from %d priority class",
		ioprio_class_to_serve);

	switch (ioprio_class_to_serve) {
	case IOPRIO_CLASS_NONE:
		goto done;
	case IOPRIO_CLASS_RT:
		start_idx = ROWQ_HIGH_PRIO_IDX;
		end_idx = ROWQ_REG_PRIO_IDX;
		break;
	case IOPRIO_CLASS_BE:
		start_idx = ROWQ_REG_PRIO_IDX;
		end_idx = ROWQ_LOW_PRIO_IDX;
		break;
	case IOPRIO_CLASS_IDLE:
		start_idx = ROWQ_LOW_PRIO_IDX;
		end_idx = ROWQ_MAX_PRIO;
		break;
	default:
		pr_err("%s(): Invalid I/O priority class", __func__);
		goto done;
	}

	currq = row_get_next_queue(q, rd, start_idx, end_idx);

	/* Dispatch */
	if (currq >= 0) {
		row_dispatch_insert(rd, currq);
		ret = 1;
	}
done:
	return ret;
}

/*
 * row_init_queue() - Init scheduler data structures
 * @q:	requests queue
 *
 * Return pointer to struct row_data to be saved in elevator for
 * this dispatch queue
 *
 */
static void *row_init_queue(struct request_queue *q)
{

	struct row_data *rdata;
	int i;

	rdata = kmalloc_node(sizeof(*rdata),
			     GFP_KERNEL | __GFP_ZERO, q->node);
	if (!rdata)
		return NULL;

	for (i = 0; i < ROWQ_MAX_PRIO; i++) {
		INIT_LIST_HEAD(&rdata->row_queues[i].rqueue.fifo);
		rdata->row_queues[i].disp_quantum = queue_quantum[i];
		rdata->row_queues[i].rqueue.rdata = rdata;
		rdata->row_queues[i].rqueue.prio = i;
		rdata->row_queues[i].rqueue.idle_data.begin_idling = false;
	}

	/*
	 * Currently idling is enabled only for READ queues. If we want to
	 * enable it for write queues also, note that idling frequency will
	 * be the same in both cases
	 */
	rdata->read_idle.idle_time = ROW_IDLE_TIME;
	rdata->read_idle.freq = ROW_READ_FREQ;
	rdata->read_idle.idle_workqueue = alloc_workqueue("row_idle_work",
					    WQ_MEM_RECLAIM | WQ_HIGHPRI, 0);
	if (!rdata->read_idle.idle_workqueue)
		panic("Failed to create idle workqueue\n");
	INIT_DELAYED_WORK(&rdata->read_idle.idle_work, kick_queue);

	rdata->curr_queue = ROWQ_PRIO_HIGH_READ;
	rdata->dispatch_queue = q;

	rdata->nr_reqs[READ] = rdata->nr_reqs[WRITE] = 0;

	return rdata;
}

/*
 * row_exit_queue() - called on unloading the RAW scheduler
 * @e:	poiner to struct elevator_queue
 *
 */
static void row_exit_queue(struct elevator_queue *e)
{
	struct row_data *rd = (struct row_data *)e->elevator_data;
	int i;

	for (i = 0; i < ROWQ_MAX_PRIO; i++)
		BUG_ON(!list_empty(&rd->row_queues[i].rqueue.fifo));
	(void)cancel_delayed_work_sync(&rd->read_idle.idle_work);
	kfree(rd);
}

/*
 * row_merged_requests() - Called when 2 requests are merged
 * @q:		requests queue
 * @rq:		request the two requests were merged into
 * @next:	request that was merged
 */
static void row_merged_requests(struct request_queue *q, struct request *rq,
				 struct request *next)
{
	struct row_queue   *rqueue = RQ_ROWQ(next);

	list_del_init(&next->queuelist);

	rqueue->rdata->nr_reqs[rq_data_dir(rq)]--;
}

/*
 * get_queue_type() - Get queue type for a given request
 *
 * This is a helping function which purpose is to determine what
 * ROW queue the given request should be added to (and
 * dispatched from leter on)
 *
 * TODO: Right now only 3 queues are used REG_READ, REG_WRITE
 * and REG_SWRITE
 */
static enum row_queue_prio get_queue_type(struct request *rq)
{
	const int data_dir = rq_data_dir(rq);
	const bool is_sync = rq_is_sync(rq);

	if (data_dir == READ)
		return ROWQ_PRIO_REG_READ;
	else if (is_sync)
		return ROWQ_PRIO_REG_SWRITE;
	else
		return ROWQ_PRIO_REG_WRITE;
}

/*
 * row_set_request() - Set ROW data structures associated with this request.
 * @q:		requests queue
 * @rq:		pointer to the request
 * @gfp_mask:	ignored
 *
 */
static int
row_set_request(struct request_queue *q, struct request *rq, gfp_t gfp_mask)
{
	struct row_data *rd = (struct row_data *)q->elevator->elevator_data;
	unsigned long flags;

	spin_lock_irqsave(q->queue_lock, flags);
	rq->elv.priv[0] =
		(void *)(&rd->row_queues[get_queue_type(rq)]);
	spin_unlock_irqrestore(q->queue_lock, flags);

	return 0;
}

/********** Helping sysfs functions/defenitions for ROW attributes ******/
static ssize_t row_var_show(int var, char *page)
{
	return snprintf(page, 100, "%d\n", var);
}

static ssize_t row_var_store(int *var, const char *page, size_t count)
{
	int err;
	err = kstrtoul(page, 10, (unsigned long *)var);

	return count;
}

#define SHOW_FUNCTION(__FUNC, __VAR, __CONV)				\
static ssize_t __FUNC(struct elevator_queue *e, char *page)		\
{									\
	struct row_data *rowd = e->elevator_data;			\
	int __data = __VAR;						\
	if (__CONV)							\
		__data = jiffies_to_msecs(__data);			\
	return row_var_show(__data, (page));			\
}
SHOW_FUNCTION(row_hp_read_quantum_show,
	rowd->row_queues[ROWQ_PRIO_HIGH_READ].disp_quantum, 0);
SHOW_FUNCTION(row_rp_read_quantum_show,
	rowd->row_queues[ROWQ_PRIO_REG_READ].disp_quantum, 0);
SHOW_FUNCTION(row_hp_swrite_quantum_show,
	rowd->row_queues[ROWQ_PRIO_HIGH_SWRITE].disp_quantum, 0);
SHOW_FUNCTION(row_rp_swrite_quantum_show,
	rowd->row_queues[ROWQ_PRIO_REG_SWRITE].disp_quantum, 0);
SHOW_FUNCTION(row_rp_write_quantum_show,
	rowd->row_queues[ROWQ_PRIO_REG_WRITE].disp_quantum, 0);
SHOW_FUNCTION(row_lp_read_quantum_show,
	rowd->row_queues[ROWQ_PRIO_LOW_READ].disp_quantum, 0);
SHOW_FUNCTION(row_lp_swrite_quantum_show,
	rowd->row_queues[ROWQ_PRIO_LOW_SWRITE].disp_quantum, 0);
SHOW_FUNCTION(row_read_idle_show, rowd->read_idle.idle_time, 1);
SHOW_FUNCTION(row_read_idle_freq_show, rowd->read_idle.freq, 1);
#undef SHOW_FUNCTION

#define STORE_FUNCTION(__FUNC, __PTR, MIN, MAX, __CONV)			\
static ssize_t __FUNC(struct elevator_queue *e,				\
		const char *page, size_t count)				\
{									\
	struct row_data *rowd = e->elevator_data;			\
	int __data;						\
	int ret = row_var_store(&__data, (page), count);		\
	if (__CONV)							\
		__data = (int)msecs_to_jiffies(__data);			\
	if (__data < (MIN))						\
		__data = (MIN);						\
	else if (__data > (MAX))					\
		__data = (MAX);						\
	*(__PTR) = __data;						\
	return ret;							\
}
STORE_FUNCTION(row_hp_read_quantum_store,
		&rowd->row_queues[ROWQ_PRIO_HIGH_READ].disp_quantum, 0,
		INT_MAX, 0);
STORE_FUNCTION(row_rp_read_quantum_store,
		&rowd->row_queues[ROWQ_PRIO_REG_READ].disp_quantum, 0,
		INT_MAX, 0);
STORE_FUNCTION(row_hp_swrite_quantum_store,
		&rowd->row_queues[ROWQ_PRIO_HIGH_SWRITE].disp_quantum, 0,
		INT_MAX, 0);
STORE_FUNCTION(row_rp_swrite_quantum_store,
		&rowd->row_queues[ROWQ_PRIO_REG_SWRITE].disp_quantum, 0,
		INT_MAX, 0);
STORE_FUNCTION(row_rp_write_quantum_store,
		&rowd->row_queues[ROWQ_PRIO_REG_WRITE].disp_quantum, 0,
		INT_MAX, 0);
STORE_FUNCTION(row_lp_read_quantum_store,
		&rowd->row_queues[ROWQ_PRIO_LOW_READ].disp_quantum, 0,
		INT_MAX, 0);
STORE_FUNCTION(row_lp_swrite_quantum_store,
		&rowd->row_queues[ROWQ_PRIO_LOW_SWRITE].disp_quantum, 0,
		INT_MAX, 1);
STORE_FUNCTION(row_read_idle_store, &rowd->read_idle.idle_time, 1, INT_MAX, 1);
STORE_FUNCTION(row_read_idle_freq_store, &rowd->read_idle.freq,
				1, INT_MAX, 1);

#undef STORE_FUNCTION

#define ROW_ATTR(name) \
	__ATTR(name, S_IRUGO|S_IWUSR, row_##name##_show, \
				      row_##name##_store)

static struct elv_fs_entry row_attrs[] = {
	ROW_ATTR(hp_read_quantum),
	ROW_ATTR(rp_read_quantum),
	ROW_ATTR(hp_swrite_quantum),
	ROW_ATTR(rp_swrite_quantum),
	ROW_ATTR(rp_write_quantum),
	ROW_ATTR(lp_read_quantum),
	ROW_ATTR(lp_swrite_quantum),
	ROW_ATTR(read_idle),
	ROW_ATTR(read_idle_freq),
	__ATTR_NULL
};

static struct elevator_type iosched_row = {
	.ops = {
		.elevator_merge_req_fn		= row_merged_requests,
		.elevator_dispatch_fn		= row_dispatch_requests,
		.elevator_add_req_fn		= row_add_request,
		.elevator_former_req_fn		= elv_rb_former_request,
		.elevator_latter_req_fn		= elv_rb_latter_request,
		.elevator_set_req_fn		= row_set_request,
		.elevator_init_fn		= row_init_queue,
		.elevator_exit_fn		= row_exit_queue,
	},

	.elevator_attrs = row_attrs,
	.elevator_name = "row",
	.elevator_owner = THIS_MODULE,
};

static int __init row_init(void)
{
	elv_register(&iosched_row);
	return 0;
}

static void __exit row_exit(void)
{
	elv_unregister(&iosched_row);
}

module_init(row_init);
module_exit(row_exit);

MODULE_LICENSE("GPLv2");
MODULE_DESCRIPTION("Read Over Write IO scheduler");
