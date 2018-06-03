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

#include "ufs-mtk.h"

#define SPREAD_PRINTF(buff, size, evt, fmt, args...) \
do { \
	if (buff && size && *(size)) { \
		unsigned long var = snprintf(*(buff), *(size), fmt, ##args); \
		if (var > 0) { \
			*(size) -= var; \
			*(buff) += var; \
		} \
	} \
	if (evt) \
		seq_printf(evt, fmt, ##args); \
	if (!buff && !evt) { \
		pr_info(fmt, ##args); \
	} \
} while (0)

#define SPREAD_DEV_PRINTF(buff, size, evt, dev, fmt, args...) \
	do { \
		if (buff && size && *(size)) { \
			unsigned long var = snprintf(*(buff), *(size), fmt, ##args); \
			if (var > 0) { \
				*(size) -= var; \
				*(buff) += var; \
			} \
		} \
		if (evt) \
			seq_printf(evt, fmt, ##args); \
		if (!buff && !evt) { \
			dev_info(dev, fmt, ##args); \
		} \
	} while (0)

enum {
	UFS_CMDS_DUMP = 0,
	UFS_GET_PWR_MODE = 1,
	UFS_DUMP_HEALTH_DESCRIPTOR = 2,
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
void ufs_mtk_dbg_add_trace(enum ufs_trace_event event, u32 tag,
					u8 lun, u32 transfer_len, sector_t lba, u8 opcode);
void ufs_mtk_dbg_hang_detect_dump(void);
void ufs_mtk_dbg_proc_dump(struct seq_file *m);
#endif
