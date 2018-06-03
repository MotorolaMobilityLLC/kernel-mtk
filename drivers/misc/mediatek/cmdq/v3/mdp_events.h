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

#undef TRACE_SYSTEM
#define TRACE_SYSTEM mdp_events

#if !defined(_TRACE_MDP_EVENTS_H) || defined(TRACE_HEADER_MULTI_READ)

#define _TRACE_MDP_EVENTS_H

#include <linux/tracepoint.h>
TRACE_EVENT(MDP__PURE_MDP_enter,
	TP_PROTO(unsigned long long task, char *data_msg),
	TP_ARGS(task, data_msg),
	TP_STRUCT__entry(
		__field(unsigned long long, task)
		__string(message, data_msg)
	),

	TP_fast_assign(
		__entry->task = task;
		__assign_str(message, data_msg);
	),


	TP_printk("_id=%llu,%s", __entry->task, __get_str(message))
);

TRACE_EVENT(MDP__PURE_MDP_leave,
	TP_PROTO(unsigned long long task),
	TP_ARGS(task),
	TP_STRUCT__entry(
		__field(unsigned long long, task)
	),
	TP_fast_assign(
		__entry->task = task;
	),


	TP_printk("_id=%llu", __entry->task)
);
TRACE_EVENT(MDP__ISPDL_MDP_enter,
	TP_PROTO(unsigned long long task, char *data_msg),
	TP_ARGS(task, data_msg),
	TP_STRUCT__entry(
		__field(unsigned long long, task)
		__string(message, data_msg)
	),

	TP_fast_assign(
		__entry->task = task;
		__assign_str(message, data_msg);
	),


	TP_printk("_id=%llu,%s", __entry->task, __get_str(message))
);

TRACE_EVENT(MDP__ISPDL_MDP_leave,
	TP_PROTO(unsigned long long task),
	TP_ARGS(task),
	TP_STRUCT__entry(
		__field(unsigned long long, task)
	),
	TP_fast_assign(
		__entry->task = task;
	),


	TP_printk("_id=%llu", __entry->task)
);

TRACE_EVENT(MDP__ISPDL_ISP_enter,
	TP_PROTO(unsigned long long task, char *data_msg),
	TP_ARGS(task, data_msg),
	TP_STRUCT__entry(
		__field(unsigned long long, task)
		__string(message, data_msg)
	),

	TP_fast_assign(
		__entry->task = task;
		__assign_str(message, data_msg);
	),


	TP_printk("_id=%llu,%s", __entry->task, __get_str(message))
);

TRACE_EVENT(MDP__ISPDL_ISP_leave,
	TP_PROTO(unsigned long long task),
	TP_ARGS(task),
	TP_STRUCT__entry(
		__field(unsigned long long, task)
	),
	TP_fast_assign(
		__entry->task = task;
	),


	TP_printk("_id=%llu", __entry->task)
);
TRACE_EVENT(ISP__ISP_ONLY_enter,
	TP_PROTO(unsigned long long task, char *data_msg),
	TP_ARGS(task, data_msg),
	TP_STRUCT__entry(
		__field(unsigned long long, task)
		__string(message, data_msg)
	),

	TP_fast_assign(
		__entry->task = task;
		__assign_str(message, data_msg);
	),


	TP_printk("_id=%llu,%s", __entry->task, __get_str(message))
);

TRACE_EVENT(ISP__ISP_ONLY_leave,
	TP_PROTO(unsigned long long task),
	TP_ARGS(task),
	TP_STRUCT__entry(
		__field(unsigned long long, task)
	),
	TP_fast_assign(
		__entry->task = task;
	),

	TP_printk("_id=%llu", __entry->task)
);
#endif /* _TRACE_MDP_EVENTS_H */

/* This part must be outside protection */
#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH .
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE mdp_events
#include <trace/define_trace.h>
