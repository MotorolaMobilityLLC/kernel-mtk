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

#ifndef __MDLA_TRACE_H__
#define __MDLA_TRACE_H__

enum {
	MDLA_TRACE_MODE_CMD = 0,
	MDLA_TRACE_MODE_INT = 1
};

#ifdef CONFIG_MTK_MDLA_DEBUG

#define MDLA_MET_READY 1

#include <linux/sched.h>
#include <mt-plat/met_drv.h>

int mdla_profile_init(void);
int mdla_profile_exit(void);
int mdla_profile_reset(void);
int mdla_profile(const char *str);
int mdla_profile_start(void);
int mdla_profile_stop(int type);
void mdla_dump_prof(struct seq_file *s);
void mdla_trace_begin(const int cmd_num, void *cmd);
void mdla_trace_end(u32 cmd_id, u64 end, int mode);
void mdla_trace_tag_begin(const char *format, ...);
void mdla_trace_tag_end(void);
#else
static inline int mdla_profile_init(void)
{
	return 0;
}
static inline int mdla_profile_exit(void)
{
	return 0;
}
static inline int mdla_profile_reset(void)
{
	return 0;
}
static inline int mdla_profile(const char *str)
{
	return 0;
}
static inline int mdla_profile_start(void)
{
	return 0;
}
static inline int mdla_profile_stop(int type)
{
	return 0;
}
static inline void mdla_dump_prof(struct seq_file *s)
{
}
static inline void mdla_trace_begin(const int cmd_num, void *cmd)
{
}
static inline void mdla_trace_end(u32 cmd_id, u64 end, int mode)
{
}
static inline void mdla_trace_tag_begin(const char *format, ...)
{
}
static inline void mdla_trace_tag_end(void)
{
}
#endif

#endif

