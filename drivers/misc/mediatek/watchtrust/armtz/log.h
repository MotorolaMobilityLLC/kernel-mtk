/*
 * Copyright (c) 2016-2018, Watchdata Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef __LOG_H__
#define __LOG_H__

int log_kthread(void *pshmaddr);
int read_log_char(char *buf);
void wdtee_log_thread_read(void);
void wd_log_read(void);

extern struct task_struct *log_thread;
#endif
