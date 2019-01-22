/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */


/*
  * Take VPU__D2D as an example:
  * Define 2 ftrace event:
  *            1. enter event
  *            2. leave event
  */

#undef TRACE_SYSTEM
#define TRACE_SYSTEM met_vpusys_events
#if !defined(_TRACE_MET_VPUSYS_EVENTS_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_MET_VPUSYS_EVENTS_H
#include <linux/tracepoint.h>

TRACE_EVENT(VPU__D2D_enter,
	TP_PROTO(int core, int algo_id, int vcore_opp, int dsp_freq, int ipu_if_freq, int dsp1_freq, int dsp2_freq),
	TP_ARGS(core, algo_id, vcore_opp, dsp_freq, ipu_if_freq, dsp1_freq, dsp2_freq),
	TP_STRUCT__entry(
		__field(int, core)
		__field(int, algo_id)
		__field(int, vcore_opp)
		__field(int, dsp_freq)
		__field(int, ipu_if_freq)
		__field(int, dsp1_freq)
		__field(int, dsp2_freq)
		),
	TP_fast_assign(
		__entry->core = core;
		__entry->algo_id = algo_id;
		__entry->vcore_opp = vcore_opp;
		__entry->dsp_freq = dsp_freq;
		__entry->ipu_if_freq = ipu_if_freq;
		__entry->dsp1_freq = dsp1_freq;
		__entry->dsp2_freq = dsp2_freq;
	),
	TP_printk("_id=c%da%d, vcore_opp=%d, dsp_freq=%d, ipu_if_freq=%d, dsp1_freq=%d, dsp2_freq=%d",
					__entry->core,
					__entry->algo_id,
					__entry->vcore_opp,
					__entry->dsp_freq,
					__entry->ipu_if_freq,
					__entry->dsp1_freq,
					__entry->dsp2_freq)
);

TRACE_EVENT(VPU__D2D_leave,
	TP_PROTO(int core, int algo_id, int dummy),
	TP_ARGS(core, algo_id, dummy),
	TP_STRUCT__entry(
		__field(int, core)
		__field(int, algo_id)
		__field(int, dummy)
	),
	TP_fast_assign(
		__entry->core = core;
		__entry->algo_id = algo_id;
		__entry->dummy = dummy;
	),
	TP_printk("_id=c%da%d", __entry->core, __entry->algo_id)
);

TRACE_EVENT(VPU__polling,
	TP_PROTO(int core, int value1, int value2, int value3, int value4),
	TP_ARGS(core, value1, value2, value3, value4),
	TP_STRUCT__entry(
		__field(int, core)
		__field(int, value1)
		__field(int, value2)
		__field(int, value3)
		__field(int, value4)
		),
	TP_fast_assign(
		__entry->core = core;
		__entry->value1 = value1;
		__entry->value2 = value2;
		__entry->value3 = value3;
		__entry->value4 = value4;
	),
	TP_printk("_id=c%d, value1=%d, value2=%d, value3=%d, value4=%d",
					__entry->core,
					__entry->value1,
					__entry->value2,
					__entry->value3,
					__entry->value4)
);


#endif /* _TRACE_MET_VPUSYS_EVENTS_H */

/* This part must be outside protection */
#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH .
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE met_vpusys_events
#include <trace/define_trace.h>

