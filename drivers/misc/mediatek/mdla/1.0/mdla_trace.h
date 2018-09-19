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

#ifdef CONFIG_MTK_MDLA_SUPPORT
#define MDLA_MET_READY 1

#include <linux/sched.h>
#include <mt-plat/met_drv.h>

int mdla_profile_start(void);
int mdla_profile_stop(int type);
int mdla_init_procfs(void);
int mdla_exit_procfs(void);

void mdla_trace_begin(const int cmd_num);
void mdla_trace_end(const int sw_cmd_id);
void mdla_trace_tag_begin(const char *format, ...);
void mdla_trace_tag_end(void);

#endif

#endif

