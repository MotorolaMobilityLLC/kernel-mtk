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

#ifndef __MT_UFS_DEUBG__
#define __MT_UFS_DEUBG__


enum {
	UFS_CMDS_DUMP = 0,
	UFS_GET_PWR_MODE = 1,
};

#define UFS_PRINFO_PROC_MSG(evt, fmt, args...) \
do { \
	if (evt == NULL) { \
		pr_info(fmt, ##args); \
	} \
	else { \
		seq_printf(evt, fmt, ##args); \
	} \
} while (0)

#define UFS_DEVINFO_PROC_MSG(evt, dev, fmt, args...) \
do {	\
	if (evt == NULL) { \
		dev_info(dev, fmt, ##args); \
	} \
	else { \
		seq_printf(evt, fmt, ##args); \
	} \
} while (0)


#define UFS_PRDBG_PROC_MSG(evt, fmt, args...) \
do {	\
	if (evt == NULL) { \
		pr_dbg(fmt, ##args); \
	} \
	else { \
		seq_printf(evt, fmt, ##args); \
	} \
} while (0)

#define UFS_DEVDBG_PROC_MSG(evt, dev, fmt, args...) \
do {	\
	if (evt == NULL) { \
		dev_dbg(dev, fmt, ##args); \
	} \
	else { \
		seq_printf(evt, fmt, ##args); \
	} \
} while (0)


int ufs_mtk_debug_proc_init(void);

#endif
