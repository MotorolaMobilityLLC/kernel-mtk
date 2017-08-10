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

#ifndef __MTKFB_DEBUG_H
#define __MTKFB_DEBUG_H


#include <linux/kernel.h>
#include "ddp_mmp.h"
#include "ddp_dump.h"
#include "disp_log.h"
#include "mmprofile.h"

#define LP_CUST_DISABLE (0)
#define LOW_POWER_MODE (1)
#define JUST_MAKE_MODE (2)
#define PERFORMANC_MODE (3)

#define DBG_BUF_SIZE		    2048
#define MAX_DBG_INDENT_LEVEL	    5
#define DBG_INDENT_SIZE		    3
#define MAX_DBG_MESSAGES	    0

int get_lp_cust_mode(void);
void backup_vfp_for_lp_cust(unsigned int vfp);
unsigned int get_backup_vfp(void);
void ddp_debug_init(void);
void ddp_debug_exit(void);

unsigned int  ddp_debug_analysis_to_buffer(void);
unsigned int  ddp_debug_dbg_log_level(void);
unsigned int  ddp_debug_irq_log_level(void);

int ddp_mem_test(void);
int ddp_lcd_test(void);
/*******************************************************/

void DBG_Init(void);
void DBG_Deinit(void);


extern int lcm_mode_status;
extern int bypass_blank;

/* For Log print Switch */
extern unsigned int gEnableUartLog;
extern unsigned int gMobilelog;
extern unsigned int gLoglevel;
extern unsigned int gRcdlevel;

#ifdef MTKFB_DBG

static int dbg_indent;
static int dbg_cnt;
static char dbg_buf[DBG_BUF_SIZE];
static spinlock_t dbg_spinlock = SPIN_LOCK_UNLOCKED;

static inline void dbg_print(int level, const char *fmt, ...)
{
	if (level <= MTKFB_DBG) {
		if (!MAX_DBG_MESSAGES || dbg_cnt < MAX_DBG_MESSAGES) {
			va_list args;
			int ind = dbg_indent;
			unsigned long flags;

			spin_lock_irqsave(&dbg_spinlock, flags);
			dbg_cnt++;
			if (ind > MAX_DBG_INDENT_LEVEL)
				ind = MAX_DBG_INDENT_LEVEL;

			DISPMSG("%*s", ind * DBG_INDENT_SIZE, "");
			va_start(args, fmt);
			vsnprintf(dbg_buf, sizeof(dbg_buf), fmt, args);
			DISPMSG(dbg_buf);
			va_end(args);
			spin_unlock_irqrestore(&dbg_spinlock, flags);
		}
	}
}

#define DBGPRINT	dbg_print

#define DBGENTER(level)	do { \
		dbg_print(level, "%s: Enter\n", __func__); \
		dbg_indent++; \
	} while (0)

#define DBGLEAVE(level)	do { \
		dbg_indent--; \
		dbg_print(level, "%s: Leave\n", __func__); \
	} while (0)

/* Debug Macros */

#define MTKFB_DBG_EVT_NONE    0x00000000
#define MTKFB_DBG_EVT_FUNC    0x00000001	/* Function Entry     */
#define MTKFB_DBG_EVT_ARGU    0x00000002	/* Function Arguments */
#define MTKFB_DBG_EVT_INFO    0x00000003	/* Information        */

#define MTKFB_DBG_EVT_MASK    (MTKFB_DBG_EVT_NONE)

#define MSG(evt, fmt, args...)                              \
	do {                                                    \
		if ((MTKFB_DBG_EVT_##evt) & MTKFB_DBG_EVT_MASK) {   \
			DISPMSG(fmt, ##args);                            \
		}                                                   \
	} while (0)

#define MSG_FUNC_ENTER(f)   MSG(FUNC, "<FB_ENTER>: %s\n", __func__)
#define MSG_FUNC_LEAVE(f)   MSG(FUNC, "<FB_LEAVE>: %s\n", __func__)


#else				/* MTKFB_DBG */

#define DBGPRINT(level, format, ...)
#define DBGENTER(level)
#define DBGLEAVE(level)

/* Debug Macros */

#define MSG(evt, fmt, args...)
#define MSG_FUNC_ENTER()
#define MSG_FUNC_LEAVE()
void _debug_pattern(unsigned int mva, unsigned int va, unsigned int w, unsigned int h,
		    unsigned int linepitch, unsigned int color, unsigned int layerid,
		    unsigned int bufidx);

/* defined in mtkfb.c */
extern unsigned int mtkfb_fm_auto_test(void);
extern int pan_display_test(int frame_num, int bpp);
extern int mtkfb_get_debug_state(char *stringbuf, int buf_len);


#endif				/* MTKFB_DBG */

#endif				/* __MTKFB_DEBUG_H */
