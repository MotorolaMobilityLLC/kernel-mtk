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

#ifndef _MLOG_LOGGER_H
#define _MLOG_LOGGER_H

#define MLOG_TRIGGER_TIMER  0
#define MLOG_TRIGGER_LMK    1
#define MLOG_TRIGGER_LTK    2

#ifdef CONFIG_MTK_MLOG

extern void mlog_init_procfs(void);
extern void mlog(int type);

#else /* CONFIG_MLOG */

void mlog_init_procfs(void) { return; }
void mlog(int type) { return; }

#endif /* CONFIG_MLOG */

#endif
