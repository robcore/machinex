/*
 * RCU segmented callback lists
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 *
 * Copyright IBM Corporation, 2017
 *
 * Authors: Paul E. McKenney <paulmck@linux.vnet.ibm.com>
 */

#include <linux/rcu_segcblist.h>

/* Initialize simple callback list. */
static inline void rcu_cblist_init(struct rcu_cblist *rclp)
{
	rclp->head = NULL;
	rclp->tail = &rclp->head;
	rclp->len = 0;
	rclp->len_lazy = 0;
}

/* Is simple callback list empty? */
static inline bool rcu_cblist_empty(struct rcu_cblist *rclp)
{
	return !rclp->head;
}

/* Return number of callbacks in simple callback list. */
static inline long rcu_cblist_n_cbs(struct rcu_cblist *rclp)
{
	return rclp->len;
}

/* Return number of lazy callbacks in simple callback list. */
static inline long rcu_cblist_n_lazy_cbs(struct rcu_cblist *rclp)
{
	return rclp->len_lazy;
}

/*
 * Debug function to actually count the number of callbacks.
 * If the number exceeds the limit specified, return -1.
 */
static inline long rcu_cblist_count_cbs(struct rcu_cblist *rclp, long lim)
{
	int cnt = 0;
	struct rcu_head **rhpp = &rclp->head;

	for (;;) {
		if (!*rhpp)
			return cnt;
		if (++cnt > lim)
			return -1;
		rhpp = &(*rhpp)->next;
	}
}

/*
 * Dequeue the oldest rcu_head structure from the specified callback
 * list.  This function assumes that the callback is non-lazy, but
 * the caller can later invoke rcu_cblist_dequeued_lazy() if it
 * finds otherwise (and if it cares about laziness).  This allows
 * different users to have different ways of determining laziness.
 */
static inline struct rcu_head *rcu_cblist_dequeue(struct rcu_cblist *rclp)
{
	struct rcu_head *rhp;

	rhp = rclp->head;
	if (!rhp)
		return NULL;
	rclp->len--;
	rclp->head = rhp->next;
	if (!rclp->head)
		rclp->tail = &rclp->head;
	return rhp;
}

/*
 * Account for the fact that a previously dequeued callback turned out
 * to be marked as lazy.
 */
static inline void rcu_cblist_dequeued_lazy(struct rcu_cblist *rclp)
{
	rclp->len_lazy--;
}

/*
 * Interim function to return rcu_cblist head pointer.  Longer term, the
 * rcu_cblist will be used more pervasively, removing the need for this
 * function.
 */
static inline struct rcu_head *rcu_cblist_head(struct rcu_cblist *rclp)
{
	return rclp->head;
}

/*
 * Interim function to return rcu_cblist head pointer.  Longer term, the
 * rcu_cblist will be used more pervasively, removing the need for this
 * function.
 */
static inline struct rcu_head **rcu_cblist_tail(struct rcu_cblist *rclp)
{
	WARN_ON_ONCE(rcu_cblist_empty(rclp));
	return rclp->tail;
}

/*
 * Initialize an rcu_segcblist structure.
 */
static inline void rcu_segcblist_init(struct rcu_segcblist *rsclp)
{
	int i;

	BUILD_BUG_ON(RCU_NEXT_TAIL + 1 != ARRAY_SIZE(rsclp->gp_seq));
	BUILD_BUG_ON(ARRAY_SIZE(rsclp->tails) != ARRAY_SIZE(rsclp->gp_seq));
	rsclp->head = NULL;
	for (i = 0; i < RCU_CBLIST_NSEGS; i++)
		rsclp->tails[i] = &rsclp->head;
	rsclp->len = 0;
	rsclp->len_lazy = 0;
}

/*
 * Is the specified rcu_segcblist structure empty?
 *
 * But careful!  The fact that the ->head field is NULL does not
 * necessarily imply that there are no callbacks associated with
 * this structure.  When callbacks are being invoked, they are
 * removed as a group.  If callback invocation must be preempted,
 * the remaining callbacks will be added back to the list.  Either
 * way, the counts are updated later.
 *
 * So it is often the case that rcu_segcblist_n_cbs() should be used
 * instead.
 */
static inline bool rcu_segcblist_empty(struct rcu_segcblist *rsclp)
{
	return !rsclp->head;
}

/* Return number of callbacks in segmented callback list. */
static inline long rcu_segcblist_n_cbs(struct rcu_segcblist *rsclp)
{
	return READ_ONCE(rsclp->len);
}

/* Return number of lazy callbacks in segmented callback list. */
static inline long rcu_segcblist_n_lazy_cbs(struct rcu_segcblist *rsclp)
{
	return rsclp->len_lazy;
}

/* Return number of lazy callbacks in segmented callback list. */
static inline long rcu_segcblist_n_nonlazy_cbs(struct rcu_segcblist *rsclp)
{
	return rsclp->len - rsclp->len_lazy;
}

/*
 * Is the specified rcu_segcblist enabled, for example, not corresponding
 * to an offline or callback-offloaded CPU?
 */
static inline bool rcu_segcblist_is_enabled(struct rcu_segcblist *rsclp)
{
	return !!rsclp->tails[RCU_NEXT_TAIL];
}

/*
 * Disable the specified rcu_segcblist structure, so that callbacks can
 * no longer be posted to it.  This structure must be empty.
 */
static inline void rcu_segcblist_disable(struct rcu_segcblist *rsclp)
{
	WARN_ON_ONCE(!rcu_segcblist_empty(rsclp));
	WARN_ON_ONCE(rcu_segcblist_n_cbs(rsclp));
	WARN_ON_ONCE(rcu_segcblist_n_lazy_cbs(rsclp));
	rsclp->tails[RCU_NEXT_TAIL] = NULL;
}

/*
 * Is the specified segment of the specified rcu_segcblist structure
 * empty of callbacks?
 */
static inline bool rcu_segcblist_segempty(struct rcu_segcblist *rsclp, int seg)
{
	if (seg == RCU_DONE_TAIL)
		return &rsclp->head == rsclp->tails[RCU_DONE_TAIL];
	return rsclp->tails[seg - 1] == rsclp->tails[seg];
}

/*
 * Are all segments following the specified segment of the specified
 * rcu_segcblist structure empty of callbacks?  (The specified
 * segment might well contain callbacks.)
 */
static inline bool rcu_segcblist_restempty(struct rcu_segcblist *rsclp, int seg)
{
	return !*rsclp->tails[seg];
}

/*
 * Does the specified rcu_segcblist structure contain callbacks that
 * are ready to be invoked?
 */
static inline bool rcu_segcblist_ready_cbs(struct rcu_segcblist *rsclp)
{
	return rcu_segcblist_is_enabled(rsclp) &&
	       &rsclp->head != rsclp->tails[RCU_DONE_TAIL];
}

/*
 * Does the specified rcu_segcblist structure contain callbacks that
 * are still pending, that is, not yet ready to be invoked?
 */
static inline bool rcu_segcblist_pend_cbs(struct rcu_segcblist *rsclp)
{
	return rcu_segcblist_is_enabled(rsclp) &&
	       !rcu_segcblist_restempty(rsclp, RCU_DONE_TAIL);
}

/*
 * Dequeue and return the first ready-to-invoke callback.  If there
 * are no ready-to-invoke callbacks, return NULL.  Disables interrupts
 * to avoid interference.  Does not protect from interference from other
 * CPUs or tasks.
 */
static inline struct rcu_head *
rcu_segcblist_dequeue(struct rcu_segcblist *rsclp)
{
	unsigned long flags;
	int i;
	struct rcu_head *rhp;

	local_irq_save(flags);
	if (!rcu_segcblist_ready_cbs(rsclp)) {
		local_irq_restore(flags);
		return NULL;
	}
	rhp = rsclp->head;
	BUG_ON(!rhp);
	rsclp->head = rhp->next;
	for (i = RCU_DONE_TAIL; i < RCU_CBLIST_NSEGS; i++) {
		if (rsclp->tails[i] != &rhp->next)
			break;
		rsclp->tails[i] = &rsclp->head;
	}
	smp_mb(); /* Dequeue before decrement for rcu_barrier(). */
	WRITE_ONCE(rsclp->len, rsclp->len - 1);
	local_irq_restore(flags);
	return rhp;
}

/*
 * Account for the fact that a previously dequeued callback turned out
 * to be marked as lazy.
 */
static inline void rcu_segcblist_dequeued_lazy(struct rcu_segcblist *rsclp)
{
	unsigned long flags;

	local_irq_save(flags);
	rsclp->len_lazy--;
	local_irq_restore(flags);
}

/*
 * Return a pointer to the first callback in the specified rcu_segcblist
 * structure.  This is useful for diagnostics.
 */
static inline struct rcu_head *
rcu_segcblist_first_cb(struct rcu_segcblist *rsclp)
{
	if (rcu_segcblist_is_enabled(rsclp))
		return rsclp->head;
	return NULL;
}

/*
 * Return a pointer to the first pending callback in the specified
 * rcu_segcblist structure.  This is useful just after posting a given
 * callback -- if that callback is the first pending callback, then
 * you cannot rely on someone else having already started up the required
 * grace period.
 */
static inline struct rcu_head *
rcu_segcblist_first_pend_cb(struct rcu_segcblist *rsclp)
{
	if (rcu_segcblist_is_enabled(rsclp))
		return *rsclp->tails[RCU_DONE_TAIL];
	return NULL;
}

/*
 * Does the specified rcu_segcblist structure contain callbacks that
 * have not yet been processed beyond having been posted, that is,
 * does it contain callbacks in its last segment?
 */
static inline bool rcu_segcblist_new_cbs(struct rcu_segcblist *rsclp)
{
	return rcu_segcblist_is_enabled(rsclp) &&
	       !rcu_segcblist_restempty(rsclp, RCU_NEXT_READY_TAIL);
}

/*
 * Enqueue the specified callback onto the specified rcu_segcblist
 * structure, updating accounting as needed.  Note that the ->len
 * field may be accessed locklessly, hence the WRITE_ONCE().
 * The ->len field is used by rcu_barrier() and friends to determine
 * if it must post a callback on this structure, and it is OK
 * for rcu_barrier() to sometimes post callbacks needlessly, but
 * absolutely not OK for it to ever miss posting a callback.
 */
static inline void rcu_segcblist_enqueue(struct rcu_segcblist *rsclp,
					 struct rcu_head *rhp, bool lazy)
{
	WRITE_ONCE(rsclp->len, rsclp->len + 1); /* ->len sampled locklessly. */
	if (lazy)
		rsclp->len_lazy++;
	smp_mb(); /* Ensure counts are updated before callback is enqueued. */
	rhp->next = NULL;
	*rsclp->tails[RCU_NEXT_TAIL] = rhp;
	rsclp->tails[RCU_NEXT_TAIL] = &rhp->next;
}

/*
 * Entrain the specified callback onto the specified rcu_segcblist at
 * the end of the last non-empty segment.  If the entire rcu_segcblist
 * is empty, make no change, but return false.
 *
 * This is intended for use by rcu_barrier()-like primitives, -not-
 * for normal grace-period use.  IMPORTANT:  The callback you enqueue
 * will wait for all prior callbacks, NOT necessarily for a grace
 * period.  You have been warned.
 */
static inline bool rcu_segcblist_entrain(struct rcu_segcblist *rsclp,
					 struct rcu_head *rhp, bool lazy)
{
	int i;

	if (rcu_segcblist_n_cbs(rsclp) == 0)
		return false;
	WRITE_ONCE(rsclp->len, rsclp->len + 1);
	if (lazy)
		rsclp->len_lazy++;
	smp_mb(); /* Ensure counts are updated before callback is entrained. */
	rhp->next = NULL;
	for (i = RCU_NEXT_TAIL; i > RCU_DONE_TAIL; i--)
		if (rsclp->tails[i] != rsclp->tails[i - 1])
			break;
	*rsclp->tails[i] = rhp;
	for (; i <= RCU_NEXT_TAIL; i++)
		rsclp->tails[i] = &rhp->next;
	return true;
}

/*
 * Extract only the counts from the specified rcu_segcblist structure,
 * and place them in the specified rcu_cblist structure.  This function
 * supports both callback orphaning and invocation, hence the separation
 * of counts and callbacks.  (Callbacks ready for invocation must be
 * orphaned and adopted separately from pending callbacks, but counts
 * apply to all callbacks.  Locking must be used to make sure that
 * both orphaned-callbacks lists are consistent.)
 */
static inline void rcu_segcblist_extract_count(struct rcu_segcblist *rsclp,
					       struct rcu_cblist *rclp)
{
	rclp->len_lazy += rsclp->len_lazy;
	rclp->len += rsclp->len;
	rsclp->len_lazy = 0;
	WRITE_ONCE(rsclp->len, 0); /* ->len sampled locklessly. */
}

/*
 * Extract only those callbacks ready to be invoked from the specified
 * rcu_segcblist structure and place them in the specified rcu_cblist
 * structure.
 */
static inline void rcu_segcblist_extract_done_cbs(struct rcu_segcblist *rsclp,
						  struct rcu_cblist *rclp)
{
	int i;

	if (!rcu_segcblist_ready_cbs(rsclp))
		return; /* Nothing to do. */
	*rclp->tail = rsclp->head;
	rsclp->head = *rsclp->tails[RCU_DONE_TAIL];
	*rsclp->tails[RCU_DONE_TAIL] = NULL;
	rclp->tail = rsclp->tails[RCU_DONE_TAIL];
	for (i = RCU_CBLIST_NSEGS - 1; i >= RCU_DONE_TAIL; i--)
		if (rsclp->tails[i] == rsclp->tails[RCU_DONE_TAIL])
			rsclp->tails[i] = &rsclp->head;
}

/*
 * Extract only those callbacks still pending (not yet ready to be
 * invoked) from the specified rcu_segcblist structure and place them in
 * the specified rcu_cblist structure.  Note that this loses information
 * about any callbacks that might have been partway done waiting for
 * their grace period.  Too bad!  They will have to start over.
 */
static inline void
rcu_segcblist_extract_pend_cbs(struct rcu_segcblist *rsclp,
			       struct rcu_cblist *rclp)
{
	int i;

	if (!rcu_segcblist_pend_cbs(rsclp))
		return; /* Nothing to do. */
	*rclp->tail = *rsclp->tails[RCU_DONE_TAIL];
	rclp->tail = rsclp->tails[RCU_NEXT_TAIL];
	*rsclp->tails[RCU_DONE_TAIL] = NULL;
	for (i = RCU_DONE_TAIL + 1; i < RCU_CBLIST_NSEGS; i++)
		rsclp->tails[i] = rsclp->tails[RCU_DONE_TAIL];
}

/*
 * Move the entire contents of the specified rcu_segcblist structure,
 * counts, callbacks, and all, to the specified rcu_cblist structure.
 * @@@ Why do we need this???  Moving early-boot CBs to NOCB lists?
 * @@@ Memory barrier needed?  (Not if only used at boot time...)
 */
static inline void rcu_segcblist_extract_all(struct rcu_segcblist *rsclp,
					     struct rcu_cblist *rclp)
{
	rcu_segcblist_extract_done_cbs(rsclp, rclp);
	rcu_segcblist_extract_pend_cbs(rsclp, rclp);
	rcu_segcblist_extract_count(rsclp, rclp);
}

/*
 * Insert counts from the specified rcu_cblist structure in the
 * specified rcu_segcblist structure.
 */
static inline void rcu_segcblist_insert_count(struct rcu_segcblist *rsclp,
					      struct rcu_cblist *rclp)
{
	rsclp->len_lazy += rclp->len_lazy;
	/* ->len sampled locklessly. */
	WRITE_ONCE(rsclp->len, rsclp->len + rclp->len);
	rclp->len_lazy = 0;
	rclp->len = 0;
}

/*
 * Move callbacks from the specified rcu_cblist to the beginning of the
 * done-callbacks segment of the specified rcu_segcblist.
 */
static inline void rcu_segcblist_insert_done_cbs(struct rcu_segcblist *rsclp,
						 struct rcu_cblist *rclp)
{
	int i;

	if (!rclp->head)
		return; /* No callbacks to move. */
	*rclp->tail = rsclp->head;
	rsclp->head = rclp->head;
	for (i = RCU_DONE_TAIL; i < RCU_CBLIST_NSEGS; i++)
		if (&rsclp->head == rsclp->tails[i])
			rsclp->tails[i] = rclp->tail;
		else
			break;
	rclp->head = NULL;
	rclp->tail = &rclp->head;
}

/*
 * Move callbacks from the specified rcu_cblist to the end of the
 * new-callbacks segment of the specified rcu_segcblist.
 */
static inline void rcu_segcblist_insert_pend_cbs(struct rcu_segcblist *rsclp,
						 struct rcu_cblist *rclp)
{
	if (!rclp->head)
		return; /* Nothing to do. */
	*rsclp->tails[RCU_NEXT_TAIL] = rclp->head;
	rsclp->tails[RCU_NEXT_TAIL] = rclp->tail;
	rclp->head = NULL;
	rclp->tail = &rclp->head;
}

/*
 * Advance the callbacks in the specified rcu_segcblist structure based
 * on the current value passed in for the grace-period counter.
 */
static inline void rcu_segcblist_advance(struct rcu_segcblist *rsclp,
					 unsigned long seq)
{
	int i, j;

	WARN_ON_ONCE(!rcu_segcblist_is_enabled(rsclp));
	if (rcu_segcblist_restempty(rsclp, RCU_DONE_TAIL))
		return;

	/*
	 * Find all callbacks whose ->gp_seq numbers indicate that they
	 * are ready to invoke, and put them into the RCU_DONE_TAIL segment.
	 */
	for (i = RCU_WAIT_TAIL; i < RCU_NEXT_TAIL; i++) {
		if (ULONG_CMP_LT(seq, rsclp->gp_seq[i]))
			break;
		rsclp->tails[RCU_DONE_TAIL] = rsclp->tails[i];
	}

	/* If no callbacks moved, nothing more need be done. */
	if (i == RCU_WAIT_TAIL)
		return;

	/* Clean up tail pointers that might have been misordered above. */
	for (j = RCU_WAIT_TAIL; j < i; j++)
		rsclp->tails[j] = rsclp->tails[RCU_DONE_TAIL];

	/*
	 * Callbacks moved, so clean up the misordered ->tails[] pointers
	 * that now point into the middle of the list of ready-to-invoke
	 * callbacks.  The overall effect is to copy down the later pointers
	 * into the gap that was created by the now-ready segments.
	 */
	for (j = RCU_WAIT_TAIL; i < RCU_NEXT_TAIL; i++, j++) {
		if (rsclp->tails[j] == rsclp->tails[RCU_NEXT_TAIL])
			break;  /* No more callbacks. */
		rsclp->tails[j] = rsclp->tails[i];
		rsclp->gp_seq[j] = rsclp->gp_seq[i];
	}
}

/*
 * "Accelerate" callbacks based on more-accurate grace-period information.
 * The reason for this is that RCU does not synchronize the beginnings and
 * ends of grace periods, and that callbacks are posted locally.  This in
 * turn means that the callbacks must be labelled conservatively early
 * on, as getting exact information would degrade both performance and
 * scalability.  When more accurate grace-period information becomes
 * available, previously posted callbacks can be "accelerated", marking
 * them to complete at the end of the earlier grace period.
 *
 * This function operates on an rcu_segcblist structure, and also the
 * grace-period sequence number seq at which new callbacks would become
 * ready to invoke.  Returns true if there are callbacks that won't be
 * ready to invoke until seq, false otherwise.
 */
static inline bool rcu_segcblist_accelerate(struct rcu_segcblist *rsclp,
					    unsigned long seq)
{
	int i;

	WARN_ON_ONCE(!rcu_segcblist_is_enabled(rsclp));
	if (rcu_segcblist_restempty(rsclp, RCU_DONE_TAIL))
		return false;

	/*
	 * Find the segment preceding the oldest segment of callbacks
	 * whose ->gp_seq[] completion is at or after that passed in via
	 * "seq", skipping any empty segments.  This oldest segment, along
	 * with any later segments, can be merged in with any newly arrived
	 * callbacks in the RCU_NEXT_TAIL segment, and assigned "seq"
	 * as their ->gp_seq[] grace-period completion sequence number.
	 */
	for (i = RCU_NEXT_READY_TAIL; i > RCU_DONE_TAIL; i--)
		if (rsclp->tails[i] != rsclp->tails[i - 1] &&
		    ULONG_CMP_LT(rsclp->gp_seq[i], seq))
			break;

	/*
	 * If all the segments contain callbacks that correspond to
	 * earlier grace-period sequence numbers than "seq", leave.
	 * Assuming that the rcu_segcblist structure has enough
	 * segments in its arrays, this can only happen if some of
	 * the non-done segments contain callbacks that really are
	 * ready to invoke.  This situation will get straightened
	 * out by the next call to rcu_segcblist_advance().
	 *
	 * Also advance to the oldest segment of callbacks whose
	 * ->gp_seq[] completion is at or after that passed in via "seq",
	 * skipping any empty segments.
	 */
	if (++i >= RCU_NEXT_TAIL)
		return false;

	/*
	 * Merge all later callbacks, including newly arrived callbacks,
	 * into the segment located by the for-loop above.  Assign "seq"
	 * as the ->gp_seq[] value in order to correctly handle the case
	 * where there were no pending callbacks in the rcu_segcblist
	 * structure other than in the RCU_NEXT_TAIL segment.
	 */
	for (; i < RCU_NEXT_TAIL; i++) {
		rsclp->tails[i] = rsclp->tails[RCU_NEXT_TAIL];
		rsclp->gp_seq[i] = seq;
	}
	return true;
}

/*
 * Scan the specified rcu_segcblist structure for callbacks that need
 * a grace period later than the one specified by "seq".  We don't look
 * at the RCU_DONE_TAIL or RCU_NEXT_TAIL segments because they don't
 * have a grace-period sequence number.
 */
static inline bool rcu_segcblist_future_gp_needed(struct rcu_segcblist *rsclp,
						  unsigned long seq)
{
	int i;

	for (i = RCU_WAIT_TAIL; i < RCU_NEXT_TAIL; i++)
		if (rsclp->tails[i - 1] != rsclp->tails[i] &&
		    ULONG_CMP_LT(seq, rsclp->gp_seq[i]))
			return true;
	return false;
}

/*
 * Interim function to return rcu_segcblist head pointer.  Longer term, the
 * rcu_segcblist will be used more pervasively, removing the need for this
 * function.
 */
static inline struct rcu_head *rcu_segcblist_head(struct rcu_segcblist *rsclp)
{
	return rsclp->head;
}

/*
 * Interim function to return rcu_segcblist head pointer.  Longer term, the
 * rcu_segcblist will be used more pervasively, removing the need for this
 * function.
 */
static inline struct rcu_head **rcu_segcblist_tail(struct rcu_segcblist *rsclp)
{
	WARN_ON_ONCE(rcu_segcblist_empty(rsclp));
	return rsclp->tails[RCU_NEXT_TAIL];
}
