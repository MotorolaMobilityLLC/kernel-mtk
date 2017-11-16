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

#ifndef __DISP_DEBUG_H__
#define __DISP_DEBUG_H__

#include <linux/kernel.h>
#include "ddp_mmp.h"
#include "ddp_dump.h"
#include "mmprofile.h"
#include "disp_log.h"

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

extern unsigned int gEnableMutexRisingEdge;
extern unsigned int gPrefetchControl;
extern unsigned int gOVLBackground;
extern unsigned int gEnableDSIStateCheck;
extern unsigned int gMutexFreeRun;
extern unsigned int disp_low_power_lfr;

void ddp_process_dbg_opt(const char *opt);
void mtkfb_process_dbg_opt(const char *opt);
unsigned int ddp_dump_reg_to_buf(unsigned int start_module, unsigned long *addr);
void sub_debug_init(void);
void sub_debug_deinit(void);

#endif /* __DISP_DEBUG_H__ */
