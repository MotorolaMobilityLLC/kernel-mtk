/*
 * Copyright (C) 2017 MediaTek Inc.
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
#define TRACE_SYSTEM mtk_idle_event

#if !defined(_TRACE_MTK_IDLE_EVENT_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_MTK_IDLE_EVENT_H

#include <linux/tracepoint.h>

TRACE_EVENT(mcdi,

	TP_PROTO(
		int cpu,
		int enter
	),

	TP_ARGS(cpu, enter),

	TP_STRUCT__entry(
		__field(int, cpu)
		__field(int, enter)
	),

	TP_fast_assign(
		__entry->cpu = cpu;
		__entry->enter = enter;
	),

	TP_printk("cpu = %d %d", (int)__entry->cpu, (int)__entry->enter)
);

TRACE_EVENT(sodi,

	TP_PROTO(
		int cpu,
		int enter
	),

	TP_ARGS(cpu, enter),

	TP_STRUCT__entry(
		__field(int, cpu)
		__field(int, enter)
	),

	TP_fast_assign(
		__entry->cpu = cpu;
		__entry->enter = enter;
	),

	TP_printk("cpu = %d %d", (int)__entry->cpu, (int)__entry->enter)
);

TRACE_EVENT(sodi3,

	TP_PROTO(
		int cpu,
		int enter
	),

	TP_ARGS(cpu, enter),

	TP_STRUCT__entry(
		__field(int, cpu)
		__field(int, enter)
	),

	TP_fast_assign(
		__entry->cpu = cpu;
		__entry->enter = enter;
	),

	TP_printk("cpu = %d %d", (int)__entry->cpu, (int)__entry->enter)
);

TRACE_EVENT(dpidle,

	TP_PROTO(
		int cpu,
		int enter
	),

	TP_ARGS(cpu, enter),

	TP_STRUCT__entry(
		__field(int, cpu)
		__field(int, enter)
	),

	TP_fast_assign(
		__entry->cpu = cpu;
		__entry->enter = enter;
	),

	TP_printk("cpu = %d %d", (int)__entry->cpu, (int)__entry->enter)
);

#endif /* _TRACE_MTK_IDLE_EVENT_H */

/* This part must be outside protection */
#include <trace/define_trace.h>
