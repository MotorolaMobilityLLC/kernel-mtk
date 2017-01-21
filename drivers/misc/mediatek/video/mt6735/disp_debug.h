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

#ifndef __DISP_DEBUG_H
#define __DISP_DEBUG_H

#include "ddp_mmp.h"
#include "ddp_dump.h"

#define dprec_string_max_length         512
#define dprec_dump_max_length           (1024 * 16 * 4)
#define LOGGER_BUFFER_SIZE              (16 * 1024)
#define ERROR_BUFFER_COUNT              2
#define FENCE_BUFFER_COUNT              22
#define DEBUG_BUFFER_COUNT              4
#define DUMP_BUFFER_COUNT               2
#define STATUS_BUFFER_COUNT             1
#define DPREC_ERROR_LOG_BUFFER_LENGTH   \
	(1024 * 16 + LOGGER_BUFFER_SIZE * \
	(ERROR_BUFFER_COUNT + FENCE_BUFFER_COUNT + DEBUG_BUFFER_COUNT + DUMP_BUFFER_COUNT + STATUS_BUFFER_COUNT))

extern unsigned int dvfs_test;
extern int primary_display_switch_mmsys_clk(int mmsys_clk_old, int mmsys_clk_new);

void DBG_Init(void);
void DBG_Deinit(void);

void DBG_OnTriggerLcd(void);
void DBG_OnTeDelayDone(void);
void DBG_OnLcdDone(void);

#include "mmprofile.h"
extern struct MTKFB_MMP_Events_t {
	MMP_Event MTKFB;
	MMP_Event CreateSyncTimeline;
	MMP_Event PanDisplay;
	MMP_Event SetOverlayLayer;
	MMP_Event SetOverlayLayers;
	MMP_Event SetMultipleLayers;
	MMP_Event CreateSyncFence;
	MMP_Event IncSyncTimeline;
	MMP_Event SignalSyncFence;
	MMP_Event TrigOverlayOut;
	MMP_Event UpdateScreenImpl;
	MMP_Event VSync;
	MMP_Event UpdateConfig;
	MMP_Event ConfigOVL;
	MMP_Event ConfigAAL;
	MMP_Event ConfigMemOut;
	MMP_Event ScreenUpdate;
	MMP_Event CaptureFramebuffer;
	MMP_Event RegUpdate;
	MMP_Event EarlySuspend;
	MMP_Event DispDone;
	MMP_Event DSICmd;
	MMP_Event DSIIRQ;
	MMP_Event EsdCheck;
	MMP_Event WaitVSync;
	MMP_Event LayerDump;
	MMP_Event Layer[4];
	MMP_Event OvlDump;
	MMP_Event FBDump;
	MMP_Event DSIRead;
	MMP_Event GetLayerInfo;
	MMP_Event LayerInfo[4];
	MMP_Event IOCtrl;
	MMP_Event Debug;
} MTKFB_MMP_Events;

extern char DDP_STR_HELP[];
extern char MTKFB_STR_HELP[];

extern unsigned int gResetRDMAEnable;
extern unsigned int gOVLBackground;
extern unsigned int gEnableIRQ;
extern unsigned int gUltraEnable;
extern unsigned long int gRDMAUltraSetting;
extern unsigned long int gRDMAFIFOLen;

extern unsigned int disp_low_power_enlarge_blanking;
extern unsigned int disp_low_power_disable_ddp_clock;
extern unsigned int disp_low_power_disable_fence_thread;
extern unsigned int disp_low_power_remove_ovl;
extern unsigned int gDumpClockStatus;

extern unsigned int gSkipIdleDetect;
extern unsigned int gEnableSODIControl;
extern unsigned int gPrefetchControl;

extern unsigned int gEnableSWTrigger;
extern unsigned int gEnableMutexRisingEdge;
extern unsigned int gDisableSODIForTriggerLoop;

extern unsigned int gDumpConfigCMD;
extern unsigned int gEnableOVLStatusCheck;

extern unsigned int gESDEnableSODI;
extern unsigned int gDumpESDCMD;

extern unsigned int gResetOVLInAALTrigger;

extern unsigned int gDisableOVLTF;

extern unsigned int gDumpMemoutCmdq;

#ifdef MTKFB_DBG
#include "disp_drv_log.h"

#define DBG_BUF_SIZE		    2048
#define MAX_DBG_INDENT_LEVEL	5
#define DBG_INDENT_SIZE		    3
#define MAX_DBG_MESSAGES	    0

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

			pr_info("DISP/DBG " "%*s", ind * DBG_INDENT_SIZE, "");
			va_start(args, fmt);
			vsnprintf(dbg_buf, sizeof(dbg_buf), fmt, args);
			pr_info("DISP/DBG " dbg_buf);
			va_end(args);
			spin_unlock_irqrestore(&dbg_spinlock, flags);
		}
	}
}

#define DBGPRINT	dbg_print

#define DBGENTER(level)	do {					\
		dbg_print(level, "%s: Enter\n", __func__);	\
		dbg_indent++;					\
	} while (0)

#define DBGLEAVE(level)	do {					\
		dbg_indent--;					\
		dbg_print(level, "%s: Leave\n", __func__);	\
	} while (0)

/* Debug Macros */

#define MTKFB_DBG_EVT_NONE    0x00000000
#define MTKFB_DBG_EVT_FUNC    0x00000001	/* Function Entry     */
#define MTKFB_DBG_EVT_ARGU    0x00000002	/* Function Arguments */
#define MTKFB_DBG_EVT_INFO    0x00000003	/* Information        */

#define MTKFB_DBG_EVT_MASK    (MTKFB_DBG_EVT_NONE)

#define MSG(evt, fmt, args...)					\
	do {							\
		if ((MTKFB_DBG_EVT_##evt) & MTKFB_DBG_EVT_MASK)	\
			pr_info("DISP/DBG " fmt, ##args);	\
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
void _debug_pattern(unsigned long mva, unsigned long va, unsigned int w, unsigned int h,
		    unsigned int linepitch, unsigned int color, unsigned int layerid,
		    unsigned int bufidx);
void _debug_fps_meter(unsigned long mva, unsigned long va, unsigned int w, unsigned int h,
		      unsigned int linepitch, unsigned int color, unsigned int layerid,
		      unsigned int bufidx);

int get_ovl1_to_mem_on(void);

#endif				/* MTKFB_DBG */

void ddp_process_dbg_opt(const char *opt);
void mtkfb_process_dbg_opt(const char *opt);
unsigned int ddp_dump_reg_to_buf(unsigned int start_module, unsigned long *addr);
void sub_debug_init(void);
void sub_debug_deinit(void);
unsigned int ddp_dump_lcm_param_to_buf(unsigned int start_module, unsigned long *addr);
char *disp_get_fmt_name(DP_COLOR_ENUM color);

#endif				/* __MTKFB_DEBUG_H */

