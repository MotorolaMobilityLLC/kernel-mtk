/*
 * Copyright (C) 2018 MediaTek Inc.
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
#define TRACE_SYSTEM met_mdlasys_events
#if !defined(_TRACE_MET_MDLASYS_EVENTS_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_MET_MDLASYS_EVENTS_H
#include <linux/tracepoint.h>

TRACE_EVENT(mdla_polling,
	TP_PROTO(int core, unsigned int id, int counter, unsigned int cycle),
	TP_ARGS(core, id, counter, cycle),
	TP_STRUCT__entry(
		__field(int, core)
		__field(unsigned int, id)
		__field(int, counter)
		__field(unsigned int, cycle)
		),
	TP_fast_assign(
		__entry->core = core;
		__entry->id = id;
		__entry->counter = counter;
		__entry->cycle = cycle;
	),
	TP_printk("_core=c%d, cmd_id=%d, counter=%d, cycle=%d",
					__entry->core,
					__entry->id,
					__entry->counter,
					__entry->cycle)
);

#endif /* _TRACE_MET_MDLASYS_EVENTS_H */

/* This part must be outside protection */
#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH .
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE met_mdlasys_events
#include <trace/define_trace.h>


