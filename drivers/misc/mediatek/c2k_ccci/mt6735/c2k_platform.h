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

#include <mt_ccci_common.h>

extern void ccci_mem_dump(int md_id, void *start_addr, int len);
extern void mtk_wdt_set_c2k_sysrst(unsigned int flag);

extern int c2k_plat_log_level;

#define LOG_ERR			0
#define LOG_INFO		1
#define LOG_NOTICE		2
#define LOG_NOTICE2		3
#define LOG_DEBUG		4

#define C2K_BOOTUP_LOG(fmt, args...) \
do { \
	ccci_dump_write(MD_SYS3, CCCI_DUMP_BOOTUP, CCCI_DUMP_CURR_FLAG|CCCI_DUMP_TIME_FLAG, fmt, ##args); \
	if (c2k_plat_log_level >= LOG_INFO) \
		pr_err(fmt, ##args); \
} while (0)

#define C2K_MEM_LOG_TAG(fmt, args...) \
do { \
	ccci_dump_write(MD_SYS3, CCCI_DUMP_MEM_DUMP, CCCI_DUMP_TIME_FLAG|CCCI_DUMP_CURR_FLAG, fmt, ##args); \
	if (c2k_plat_log_level >= LOG_DEBUG) \
		pr_err(fmt, ##args); \
} while (0)

#define C2K_MEM_LOG(fmt, args...) \
do { \
	ccci_dump_write(MD_SYS3, CCCI_DUMP_MEM_DUMP, 0, fmt, ##args); \
	if (c2k_plat_log_level >= LOG_DEBUG) \
		pr_err(fmt, ##args); \
} while (0)

#define C2K_NORMAL_LOG(fmt, args...) \
do { \
	ccci_dump_write(MD_SYS3, CCCI_DUMP_NORMAL, CCCI_DUMP_CURR_FLAG|CCCI_DUMP_TIME_FLAG, fmt, ##args); \
	if (c2k_plat_log_level >= LOG_INFO) \
		pr_err(fmt, ##args); \
} while (0)

#define C2K_ERROR_LOG(fmt, args...) \
do { \
	ccci_dump_write(MD_SYS3, CCCI_DUMP_NORMAL, CCCI_DUMP_CURR_FLAG|CCCI_DUMP_TIME_FLAG, fmt, ##args); \
	if (c2k_plat_log_level >= LOG_ERR) \
		pr_err(fmt, ##args); \
} while (0)

#define C2K_REPEAT_LOG(fmt, args...) \
do { \
	ccci_dump_write(MD_SYS3, CCCI_DUMP_REPEAT, CCCI_DUMP_CURR_FLAG|CCCI_DUMP_TIME_FLAG, fmt, ##args); \
	if (c2k_plat_log_level >= LOG_DEBUG) \
		pr_err(fmt, ##args); \
} while (0)
