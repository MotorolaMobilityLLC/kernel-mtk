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

#include <linux/version.h>
#include <linux/module.h>
#include <linux/ftrace_event.h>
#include "ged_base.h"
#include "ged_trace.h"

static unsigned int ged_trace_enable = 0;
static unsigned long __read_mostly tracing_mark_write_addr = 0;

static inline void __mt_update_tracing_mark_write_addr(void)
{
	if(unlikely(0 == tracing_mark_write_addr))
	{
		tracing_mark_write_addr = kallsyms_lookup_name("tracing_mark_write");
	}
}

void ged_trace_begin(int tid, char *name)
{
	if(ged_trace_enable)
	{
    	__mt_update_tracing_mark_write_addr();
        event_trace_printk(tracing_mark_write_addr, "B|%d|%s\n", tid, name);
	}
}
EXPORT_SYMBOL(ged_trace_begin);

void ged_trace_end(void)
{
	if(ged_trace_enable)
	{
   		__mt_update_tracing_mark_write_addr();
       	event_trace_printk(tracing_mark_write_addr, "E\n");
	}
}
EXPORT_SYMBOL(ged_trace_end);

void ged_trace_counter(int tid, char *name, int count)
{
	if(ged_trace_enable)
	{
    	__mt_update_tracing_mark_write_addr();
        event_trace_printk(tracing_mark_write_addr, "C|%d|%s|%d\n", tid, name, count);
	}
}
EXPORT_SYMBOL(ged_trace_counter);

module_param(ged_trace_enable, uint, 0644);

