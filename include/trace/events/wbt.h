#undef TRACE_SYSTEM
#define TRACE_SYSTEM wbt

#if !defined(_TRACE_WBT_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_WBT_H

#include <linux/tracepoint.h>
#include <linux/wbt.h>

/**
 * wbt_stat - trace stats for blk_wb
 * @stat: array of read/write stats
 */
TRACE_EVENT(wbt_stat,

	TP_PROTO(struct backing_dev_info *bdi, struct blk_rq_stat *stat),

	TP_ARGS(bdi, stat),

	TP_STRUCT__entry(
		__array(char, name, 32)
		__field(s64, rmean)
		__field(u64, rmin)
		__field(u64, rmax)
		__field(s64, rnr_samples)
		__field(s64, rtime)
		__field(s64, wmean)
		__field(u64, wmin)
		__field(u64, wmax)
		__field(s64, wnr_samples)
		__field(s64, wtime)
	),

	TP_fast_assign(
		strncpy(__entry->name, dev_name(bdi->dev), 32);
		__entry->rmean		= stat[0].mean;
		__entry->rmin		= stat[0].min;
		__entry->rmax		= stat[0].max;
		__entry->rnr_samples	= stat[0].nr_samples;
		__entry->wmean		= stat[1].mean;
		__entry->wmin		= stat[1].min;
		__entry->wmax		= stat[1].max;
		__entry->wnr_samples	= stat[1].nr_samples;
	),

	TP_printk("%s: rmean=%llu, rmin=%llu, rmax=%llu, rsamples=%llu, "
		  "wmean=%llu, wmin=%llu, wmax=%llu, wsamples=%llu\n",
		  __entry->name, __entry->rmean, __entry->rmin, __entry->rmax,
		  __entry->rnr_samples, __entry->wmean, __entry->wmin,
		  __entry->wmax, __entry->wnr_samples)
);

/**
 * wbt_lat - trace latency event
 * @lat: latency trigger
 */
TRACE_EVENT(wbt_lat,

	TP_PROTO(struct backing_dev_info *bdi, unsigned long lat),

	TP_ARGS(bdi, lat),

	TP_STRUCT__entry(
		__array(char, name, 32)
		__field(unsigned long, lat)
	),

	TP_fast_assign(
		strncpy(__entry->name, dev_name(bdi->dev), 32);
		__entry->lat = lat;
	),

	TP_printk("%s: latency %llu\n", __entry->name,
			(unsigned long long) __entry->lat)
);

/**
 * wbt_step - trace wb event step
 * @msg: context message
 * @step: the current scale step count
 * @window: the current monitoring window
 * @bg: the current background queue limit
 * @normal: the current normal writeback limit
 * @max: the current max throughput writeback limit
 */
TRACE_EVENT(wbt_step,

	TP_PROTO(struct backing_dev_info *bdi, const char *msg,
		 unsigned int step, unsigned long window, unsigned int bg,
		 unsigned int normal, unsigned int max),

	TP_ARGS(bdi, msg, step, window, bg, normal, max),

	TP_STRUCT__entry(
		__array(char, name, 32)
		__field(const char *, msg)
		__field(unsigned int, step)
		__field(unsigned long, window)
		__field(unsigned int, bg)
		__field(unsigned int, normal)
		__field(unsigned int, max)
	),

	TP_fast_assign(
		strncpy(__entry->name, dev_name(bdi->dev), 32);
		__entry->msg	= msg;
		__entry->step	= step;
		__entry->window	= window;
		__entry->bg	= bg;
		__entry->normal	= normal;
		__entry->max	= max;
	),

	TP_printk("%s: %s: step=%u, window=%lu, background=%u, normal=%u, max=%u\n",
		  __entry->name, __entry->msg, __entry->step, __entry->window,
		  __entry->bg, __entry->normal, __entry->max)
);

#endif /* _TRACE_WBT_H */

/* This part must be outside protection */
#include <trace/define_trace.h>

