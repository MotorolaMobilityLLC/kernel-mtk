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

#ifndef MT_MSDC_DEUBG
#define MT_MSDC_DEUBG
#include "mt_sd.h"

#define MTK_MSDC_ERROR_TUNE_DEBUG

/* Debug message event */
#define DBG_EVT_NONE        (0)         /* No event */
#define DBG_EVT_DMA         (1 << 0)    /* DMA related event */
#define DBG_EVT_CMD         (1 << 1)    /* MSDC CMD related event */
#define DBG_EVT_RSP         (1 << 2)    /* MSDC CMD RSP related event */
#define DBG_EVT_INT         (1 << 3)    /* MSDC INT event */
#define DBG_EVT_CFG         (1 << 4)    /* MSDC CFG event */
#define DBG_EVT_FUC         (1 << 5)    /* Function event */
#define DBG_EVT_OPS         (1 << 6)    /* Read/Write operation event */
#define DBG_EVT_FIO         (1 << 7)    /* FIFO operation event */
#define DBG_EVT_WRN         (1 << 8)    /* Warning event */
#define DBG_EVT_PWR         (1 << 9)    /* Power event */
#define DBG_EVT_CLK         (1 << 10)   /* Trace clock gate/ungate operation */
#define DBG_EVT_CHE         (1 << 11)   /* eMMC cache feature operation */
/* ==================================================== */
#define DBG_EVT_RW          (1 << 12)   /* Trace the Read/Write Command */
#define DBG_EVT_NRW         (1 << 13)   /* Trace other Command */
#define DBG_EVT_ALL         (0xffffffff)

#define DBG_EVT_MASK        (DBG_EVT_ALL)

#define TAG "msdc"
#define N_MSG(evt, fmt, args...) \
do {    \
	if ((DBG_EVT_##evt) & sd_debug_zone[host->id]) { \
		pr_err(TAG"%d -> "fmt" <- %s() : L<%d> PID<%s><0x%x>\n", \
			host->id,  ##args, __func__, __LINE__, \
			current->comm, current->pid); \
	}   \
} while (0)

#if 1
#define ERR_MSG(fmt, args...) \
	pr_err(TAG"%d -> "fmt" <- %s() : L<%d> PID<%s><0x%x>\n", \
		host->id,  ##args, __func__, __LINE__, current->comm, \
		current->pid)

#else
#define MAX_PRINT_PERIOD            (500000000)  /* 500ms */
#define MAX_PRINT_NUMS_OVER_PERIOD  (50)
#define ERR_MSG(fmt, args...) \
do { \
	if (print_nums == 0) { \
		print_nums++; \
		msdc_print_start_time = sched_clock(); \
		pr_err(TAG"MSDC", TAG"%d -> "fmt" <- %s() : L<%d> " \
			"PID<%s><0x%x>\n", \
			host->id,  ##args, __func__, __LINE__, \
			current->comm, current->pid); \
	} else { \
		msdc_print_end_time = sched_clock();    \
		if ((msdc_print_end_time - msdc_print_start_time) >= \
			MAX_PRINT_PERIOD) { \
			pr_err(TAG"MSDC", TAG"%d -> "fmt" <- %s() : L<%d> " \
				"PID<%s><0x%x>\n", \
				host->id,  ##args, __func__, __LINE__, \
				current->comm, current->pid); \
			print_nums = 0; \
		} \
		if (print_nums <= MAX_PRINT_NUMS_OVER_PERIOD) { \
			pr_err(TAG"MSDC", TAG"%d -> "fmt" <- %s() : " \
				"L<%d> PID<%s><0x%x>\n", \
				host->id,  ##args, __func__, \
				__LINE__, current->comm, current->pid); \
			print_nums++;   \
		} \
	} \
} while (0)
#endif

#define INIT_MSG(fmt, args...) \
	pr_err(TAG"%d -> "fmt" <- %s() : L<%d> PID<%s><0x%x>\n", \
		host->id,  ##args, __func__, __LINE__, current->comm, \
		current->pid)

#define SIMPLE_INIT_MSG(fmt, args...) \
	pr_err("%d:"fmt"\n", id,  ##args)

#define INFO_MSG(fmt, args...) \
	pr_debug(TAG"%d -> "fmt" <- %s() : L<%d> PID<%s><0x%x>\n", \
		host->id,  ##args, __func__, __LINE__, current->comm, \
		current->pid)

#if 0
/* PID in ISR in not corrent */
#define IRQ_MSG(fmt, args...) \
	pr_err(TAG"%d -> "fmt" <- %s() : L<%d>\n", \
		host->id,  ##args, __func__, __LINE__)
#else
#define IRQ_MSG(fmt, args...)
#endif

#endif
