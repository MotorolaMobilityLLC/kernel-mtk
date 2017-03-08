/*
 * buffered writeback throttling. losely based on CoDel. We can't drop
 * packets for IO scheduling, so the logic is something like this:
 *
 * - Monitor latencies in a defined window of time.
 * - If the minimum latency in the above window exceeds some target, increment
 *   scaling step and scale down queue depth by a factor of 2x. The monitoring
 *   window is then shrunk to 100 / sqrt(scaling step + 1).
 * - For any window where we don't have solid data on what the latencies
 *   look like, retain status quo.
 * - If latencies look good, decrement scaling step.
 *
 * Copyright (C) 2016 Jens Axboe
 *
 * Things that (may) need changing:
 *
 *	- Different scaling of background/normal/high priority writeback.
 *	  We may have to violate guarantees for max.
 *	- We can have mismatches between the stat window and our window.
 *
 */
#include <linux/kernel.h>
#include <linux/blk_types.h>
#include <linux/slab.h>
#include <linux/backing-dev.h>
#include <linux/wbt.h>

#define CREATE_TRACE_POINTS
#include <trace/events/wbt.h>

enum {
	/*
	 * Might need to be higher
	 */
	RWB_MAX_DEPTH	= 64,

	/*
	 * 100msec window
	 */
	RWB_WINDOW_NSEC		= 100 * 1000 * 1000ULL,

	/*
	 * Disregard stats, if we don't meet these minimums
	 */
	RWB_MIN_WRITE_SAMPLES	= 3,
	RWB_MIN_READ_SAMPLES	= 1,

	/*
	 * If we have this number of consecutive windows with not enough
	 * information to scale up or down, scale up.
	 */
	RWB_UNKNOWN_BUMP	= 5,
};

static inline bool rwb_enabled(struct rq_wb *rwb)
{
	return rwb && rwb->wb_normal != 0;
}

/*
 * Increment 'v', if 'v' is below 'below'. Returns true if we succeeded,
 * false if 'v' + 1 would be bigger than 'below'.
 */
static bool atomic_inc_below(atomic_t *v, int below)
{
	int cur = atomic_read(v);

	for (;;) {
		int old;

		if (cur >= below)
			return false;
		old = atomic_cmpxchg(v, cur, cur + 1);
		if (old == cur)
			break;
		cur = old;
	}

	return true;
}

static void wb_timestamp(struct rq_wb *rwb, unsigned long *var)
{
	if (rwb_enabled(rwb)) {
		const unsigned long cur = jiffies;

		if (cur != *var)
			*var = cur;
	}
}

void __wbt_done(struct rq_wb *rwb)
{
	int inflight, limit;

	inflight = atomic_dec_return(&rwb->inflight);

	/*
	 * wbt got disabled with IO in flight. Wake up any potential
	 * waiters, we don't have to do more than that.
	 */
	if (unlikely(!rwb_enabled(rwb))) {
		wake_up_all(&rwb->wait);
		return;
	}

	/*
	 * If the device does write back caching, drop further down
	 * before we wake people up.
	 */
	if (rwb->wc && !atomic_read(&rwb->bdi->wb.dirty_sleeping))
		limit = 0;
	else
		limit = rwb->wb_normal;

	/*
	 * Don't wake anyone up if we are above the normal limit.
	 */
	if (inflight && inflight >= limit)
		return;

	if (waitqueue_active(&rwb->wait)) {
		int diff = limit - inflight;

		if (!inflight || diff >= rwb->wb_background / 2)
			wake_up_nr(&rwb->wait, 1);
	}
}

/*
 * Called on completion of a request. Note that it's also called when
 * a request is merged, when the request gets freed.
 */
void wbt_done(struct rq_wb *rwb, struct wb_issue_stat *stat)
{
	if (!rwb)
		return;

	if (!wbt_tracked(stat)) {
		if (rwb->sync_cookie == stat) {
			rwb->sync_issue = 0;
			rwb->sync_cookie = NULL;
		}

		wb_timestamp(rwb, &rwb->last_comp);
	} else {
		WARN_ON_ONCE(stat == rwb->sync_cookie);
		__wbt_done(rwb);
		wbt_clear_tracked(stat);
	}
}

static void calc_wb_limits(struct rq_wb *rwb)
{
	unsigned int depth;

	if (!rwb->min_lat_nsec) {
		rwb->wb_max = rwb->wb_normal = rwb->wb_background = 0;
		return;
	}

	/*
	 * For QD=1 devices, this is a special case. It's important for those
	 * to have one request ready when one completes, so force a depth of
	 * 2 for those devices. On the backend, it'll be a depth of 1 anyway,
	 * since the device can't have more than that in flight. If we're
	 * scaling down, then keep a setting of 1/1/1.
	 */
	if (rwb->queue_depth == 1) {
		if (rwb->scale_step)
			rwb->wb_max = rwb->wb_normal = 1;
		else
			rwb->wb_max = rwb->wb_normal = 2;
		rwb->wb_background = 1;
	} else {
		depth = min_t(unsigned int, RWB_MAX_DEPTH, rwb->queue_depth);

		/*
		 * Set our max/normal/bg queue depths based on how far
		 * we have scaled down (->scale_step).
		 */
		rwb->wb_max = 1 + ((depth - 1) >> min(31U, rwb->scale_step));
		rwb->wb_normal = (rwb->wb_max + 1) / 2;
		rwb->wb_background = (rwb->wb_max + 3) / 4;
	}
}

static bool inline stat_sample_valid(struct blk_rq_stat *stat)
{
	/*
	 * We need at least one read sample, and a minimum of
	 * RWB_MIN_WRITE_SAMPLES. We require some write samples to know
	 * that it's writes impacting us, and not just some sole read on
	 * a device that is in a lower power state.
	 */
	return stat[0].nr_samples >= 1 &&
		stat[1].nr_samples >= RWB_MIN_WRITE_SAMPLES;
}

static u64 rwb_sync_issue_lat(struct rq_wb *rwb)
{
	u64 now, issue = ACCESS_ONCE(rwb->sync_issue);

	if (!issue || !rwb->sync_cookie)
		return 0;

	now = ktime_to_ns(ktime_get());
	return now - issue;
}

enum {
	LAT_OK,
	LAT_UNKNOWN,
	LAT_EXCEEDED,
};

static int __latency_exceeded(struct rq_wb *rwb, struct blk_rq_stat *stat)
{
	u64 thislat;

	/*
	 * If our stored sync issue exceeds the window size, or it
	 * exceeds our min target AND we haven't logged any entries,
	 * flag the latency as exceeded. wbt works off completion latencies,
	 * but for a flooded device, a single sync IO can take a long time
	 * to complete after being issued. If this time exceeds our
	 * monitoring window AND we didn't see any other completions in that
	 * window, then count that sync IO as a violation of the latency.
	 */
	thislat = rwb_sync_issue_lat(rwb);
	if (thislat > rwb->cur_win_nsec ||
	    (thislat > rwb->min_lat_nsec && !stat[0].nr_samples)) {
		trace_wbt_lat(rwb->bdi, thislat);
		return LAT_EXCEEDED;
	}

	if (!stat_sample_valid(stat))
		return LAT_UNKNOWN;

	/*
	 * If the 'min' latency exceeds our target, step down.
	 */
	if (stat[0].min > rwb->min_lat_nsec) {
		trace_wbt_lat(rwb->bdi, stat[0].min);
		trace_wbt_stat(rwb->bdi, stat);
		return LAT_EXCEEDED;
	}

	if (rwb->scale_step)
		trace_wbt_stat(rwb->bdi, stat);

	return LAT_OK;
}

static int latency_exceeded(struct rq_wb *rwb)
{
	struct blk_rq_stat stat[2];

	rwb->stat_ops->get(rwb->ops_data, stat);
	return __latency_exceeded(rwb, stat);
}

static void rwb_trace_step(struct rq_wb *rwb, const char *msg)
{
	trace_wbt_step(rwb->bdi, msg, rwb->scale_step, rwb->cur_win_nsec,
			rwb->wb_background, rwb->wb_normal, rwb->wb_max);
}

static void scale_up(struct rq_wb *rwb)
{
	/*
	 * If we're at 0, we can't go lower.
	 */
	if (!rwb->scale_step)
		return;

	rwb->scale_step--;
	rwb->unknown_cnt = 0;
	rwb->stat_ops->clear(rwb->ops_data);
	calc_wb_limits(rwb);

	if (waitqueue_active(&rwb->wait))
		wake_up_all(&rwb->wait);

	rwb_trace_step(rwb, "step up");
}

static void scale_down(struct rq_wb *rwb)
{
	/*
	 * Stop scaling down when we've hit the limit. This also prevents
	 * ->scale_step from going to crazy values, if the device can't
	 * keep up.
	 */
	if (rwb->wb_max == 1)
		return;

	rwb->scale_step++;
	rwb->unknown_cnt = 0;
	rwb->stat_ops->clear(rwb->ops_data);
	calc_wb_limits(rwb);
	rwb_trace_step(rwb, "step down");
}

static void rwb_arm_timer(struct rq_wb *rwb)
{
	unsigned long expires;

	/*
	 * We should speed this up, using some variant of a fast integer
	 * inverse square root calculation. Since we only do this for
	 * every window expiration, it's not a huge deal, though.
	 */
	rwb->cur_win_nsec = div_u64(rwb->win_nsec << 4,
					int_sqrt((rwb->scale_step + 1) << 8));
	expires = jiffies + nsecs_to_jiffies(rwb->cur_win_nsec);
	mod_timer(&rwb->window_timer, expires);
}

static void wb_timer_fn(unsigned long data)
{
	struct rq_wb *rwb = (struct rq_wb *) data;
	int status;

	/*
	 * If we exceeded the latency target, step down. If we did not,
	 * step one level up. If we don't know enough to say either exceeded
	 * or ok, then don't do anything.
	 */
	status = latency_exceeded(rwb);
	switch (status) {
	case LAT_EXCEEDED:
		scale_down(rwb);
		break;
	case LAT_OK:
		scale_up(rwb);
		break;
	case LAT_UNKNOWN:
		/*
		 * We had no read samples, start bumping up the write
		 * depth slowly
		 */
		if (++rwb->unknown_cnt >= RWB_UNKNOWN_BUMP)
			scale_up(rwb);
		break;
	default:
		break;
	}

	/*
	 * Re-arm timer, if we have IO in flight
	 */
	if (rwb->scale_step || atomic_read(&rwb->inflight))
		rwb_arm_timer(rwb);
}

void wbt_update_limits(struct rq_wb *rwb)
{
	rwb->scale_step = 0;
	calc_wb_limits(rwb);

	if (waitqueue_active(&rwb->wait))
		wake_up_all(&rwb->wait);
}

static bool close_io(struct rq_wb *rwb)
{
	const unsigned long now = jiffies;

	return time_before(now, rwb->last_issue + HZ / 10) ||
		time_before(now, rwb->last_comp + HZ / 10);
}

#define REQ_HIPRIO	(REQ_SYNC | REQ_META | REQ_PRIO)

static inline unsigned int get_limit(struct rq_wb *rwb, unsigned long rw)
{
	unsigned int limit;

	/*
	 * At this point we know it's a buffered write. If REQ_SYNC is
	 * set, then it's WB_SYNC_ALL writeback, and we'll use the max
	 * limit for that. If the write is marked as a background write,
	 * then use the idle limit, or go to normal if we haven't had
	 * competing IO for a bit.
	 */
	if ((rw & REQ_HIPRIO) || atomic_read(&rwb->bdi->wb.dirty_sleeping))
		limit = rwb->wb_max;
	else if ((rw & REQ_BG) || close_io(rwb)) {
		/*
		 * If less than 100ms since we completed unrelated IO,
		 * limit us to half the depth for background writeback.
		 */
		limit = rwb->wb_background;
	} else
		limit = rwb->wb_normal;

	return limit;
}

static inline bool may_queue(struct rq_wb *rwb, unsigned long rw)
{
	/*
	 * inc it here even if disabled, since we'll dec it at completion.
	 * this only happens if the task was sleeping in __wbt_wait(),
	 * and someone turned it off at the same time.
	 */
	if (!rwb_enabled(rwb)) {
		atomic_inc(&rwb->inflight);
		return true;
	}

	return atomic_inc_below(&rwb->inflight, get_limit(rwb, rw));
}

/*
 * Block if we will exceed our limit, or if we are currently waiting for
 * the timer to kick off queuing again.
 */
static void __wbt_wait(struct rq_wb *rwb, unsigned long rw, spinlock_t *lock)
{
	DEFINE_WAIT(wait);

	if (may_queue(rwb, rw))
		return;

	do {
		prepare_to_wait_exclusive(&rwb->wait, &wait,
						TASK_UNINTERRUPTIBLE);

		if (may_queue(rwb, rw))
			break;

		if (lock)
			spin_unlock_irq(lock);

		io_schedule();

		if (lock)
			spin_lock_irq(lock);
	} while (1);

	finish_wait(&rwb->wait, &wait);
}

static inline bool wbt_should_throttle(struct rq_wb *rwb, unsigned int rw)
{
	/*
	 * If not a WRITE (or a discard), do nothing
	 */
	if (!(rw & REQ_WRITE) || (rw & REQ_DISCARD))
		return false;

	/*
	 * Don't throttle WRITE_ODIRECT
	 */
	if ((rw & (REQ_SYNC | REQ_NOIDLE)) == REQ_SYNC)
		return false;

	return true;
}

/*
 * Returns true if the IO request should be accounted, false if not.
 * May sleep, if we have exceeded the writeback limits. Caller can pass
 * in an irq held spinlock, if it holds one when calling this function.
 * If we do sleep, we'll release and re-grab it.
 */
bool wbt_wait(struct rq_wb *rwb, unsigned int rw, spinlock_t *lock)
{
	if (!rwb_enabled(rwb))
		return false;

	if (!wbt_should_throttle(rwb, rw)) {
		wb_timestamp(rwb, &rwb->last_issue);
		return false;
	}

	__wbt_wait(rwb, rw, lock);

	if (!timer_pending(&rwb->window_timer))
		rwb_arm_timer(rwb);

	return true;
}

void wbt_issue(struct rq_wb *rwb, struct wb_issue_stat *stat)
{
	if (!rwb_enabled(rwb))
		return;

	wbt_issue_stat_set_time(stat);

	/*
	 * Track sync issue, in case it takes a long time to complete. Allows
	 * us to react quicker, if a sync IO takes a long time to complete.
	 * Note that this is just a hint. 'stat' can go away when the
	 * request completes, so it's important we never dereference it. We
	 * only use the address to compare with, which is why we store the
	 * sync_issue time locally.
	 */
	if (!wbt_tracked(stat) && !rwb->sync_issue) {
		rwb->sync_cookie = stat;
		rwb->sync_issue = wbt_issue_stat_get_time(stat);
	}
}

void wbt_requeue(struct rq_wb *rwb, struct wb_issue_stat *stat)
{
	if (!rwb_enabled(rwb))
		return;
	if (stat == rwb->sync_cookie) {
		rwb->sync_issue = 0;
		rwb->sync_cookie = NULL;
	}
}

void wbt_set_queue_depth(struct rq_wb *rwb, unsigned int depth)
{
	if (rwb) {
		rwb->queue_depth = depth;
		wbt_update_limits(rwb);
	}
}

void wbt_set_write_cache(struct rq_wb *rwb, bool write_cache_on)
{
	if (rwb)
		rwb->wc = write_cache_on;
}

void wbt_disable(struct rq_wb *rwb)
{
	del_timer_sync(&rwb->window_timer);
	rwb->win_nsec = rwb->min_lat_nsec = 0;
	wbt_update_limits(rwb);
}
EXPORT_SYMBOL_GPL(wbt_disable);

struct rq_wb *wbt_init(struct backing_dev_info *bdi, struct wb_stat_ops *ops,
		       void *ops_data)
{
	struct rq_wb *rwb;

	rwb = kzalloc(sizeof(*rwb), GFP_KERNEL);
	if (!rwb)
		return ERR_PTR(-ENOMEM);

	atomic_set(&rwb->inflight, 0);
	init_waitqueue_head(&rwb->wait);
	setup_timer(&rwb->window_timer, wb_timer_fn, (unsigned long) rwb);
	rwb->wc = 1;
	rwb->queue_depth = RWB_MAX_DEPTH;
	rwb->last_comp = rwb->last_issue = jiffies;
	rwb->bdi = bdi;
	rwb->win_nsec = RWB_WINDOW_NSEC;
	rwb->stat_ops = ops,
	rwb->ops_data = ops_data;
	wbt_update_limits(rwb);
	return rwb;
}

void wbt_exit(struct rq_wb *rwb)
{
	if (rwb) {
		del_timer_sync(&rwb->window_timer);
		kfree(rwb);
	}
}

