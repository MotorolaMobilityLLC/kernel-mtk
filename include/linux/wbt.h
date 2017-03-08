#ifndef WB_THROTTLE_H
#define WB_THROTTLE_H

#include <linux/atomic.h>
#include <linux/wait.h>
#include <linux/timer.h>
#include <linux/ktime.h>

#define ISSUE_STAT_MASK		(1ULL << 63)
#define ISSUE_STAT_TIME_MASK	~ISSUE_STAT_MASK

struct wb_issue_stat {
	u64 time;
};

static inline void wbt_issue_stat_set_time(struct wb_issue_stat *stat)
{
	stat->time = (stat->time & ISSUE_STAT_MASK) |
			(ktime_to_ns(ktime_get()) & ISSUE_STAT_TIME_MASK);
}

static inline u64 wbt_issue_stat_get_time(struct wb_issue_stat *stat)
{
	return stat->time & ISSUE_STAT_TIME_MASK;
}

static inline void wbt_mark_tracked(struct wb_issue_stat *stat)
{
	stat->time |= ISSUE_STAT_MASK;
}

static inline void wbt_clear_tracked(struct wb_issue_stat *stat)
{
	stat->time &= ~ISSUE_STAT_MASK;
}

static inline bool wbt_tracked(struct wb_issue_stat *stat)
{
	return (stat->time & ISSUE_STAT_MASK) != 0;
}

struct wb_stat_ops {
	void (*get)(void *, struct blk_rq_stat *);
	void (*clear)(void *);
};

struct rq_wb {
	/*
	 * Settings that govern how we throttle
	 */
	unsigned int wb_background;		/* background writeback */
	unsigned int wb_normal;			/* normal writeback */
	unsigned int wb_max;			/* max throughput writeback */
	unsigned int scale_step;

	u64 win_nsec;				/* default window size */
	u64 cur_win_nsec;			/* current window size */

	/*
	 * Number of consecutive periods where we don't have enough
	 * information to make a firm scale up/down decision.
	 */
	unsigned int unknown_cnt;

	struct timer_list window_timer;

	s64 sync_issue;
	void *sync_cookie;

	unsigned int wc;
	unsigned int queue_depth;

	unsigned long last_issue;		/* last non-throttled issue */
	unsigned long last_comp;		/* last non-throttled comp */
	unsigned long min_lat_nsec;
	struct backing_dev_info *bdi;
	struct request_queue *q;
	wait_queue_head_t wait;
	atomic_t inflight;

	struct wb_stat_ops *stat_ops;
	void *ops_data;
};

struct backing_dev_info;

void __wbt_done(struct rq_wb *);
void wbt_done(struct rq_wb *, struct wb_issue_stat *);
bool wbt_wait(struct rq_wb *, unsigned int, spinlock_t *);
struct rq_wb *wbt_init(struct backing_dev_info *, struct wb_stat_ops *, void *);
void wbt_exit(struct rq_wb *);
void wbt_update_limits(struct rq_wb *);
void wbt_requeue(struct rq_wb *, struct wb_issue_stat *);
void wbt_issue(struct rq_wb *, struct wb_issue_stat *);
void wbt_disable(struct rq_wb *);

void wbt_set_queue_depth(struct rq_wb *, unsigned int);
void wbt_set_write_cache(struct rq_wb *, bool);

#endif

