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

#include <linux/mutex.h>
#include <linux/semaphore.h>
#include <linux/time.h>
#include <linux/jiffies.h>
#include <linux/module.h>

#include "ged_frr.h"
#include "ged_base.h"
#include "ged_type.h"
#include "primary_display.h"
#include "dfrc.h"
#include "dfrc_drv.h"

#ifdef GED_LOGE
#undef GED_LOGE
#define GED_LOGE pr_warn
#endif

/* These module params are for developers to set specific fps and debug. */
static int ged_frr_debug_pid = GED_FRR_TABLE_ZERO;
static unsigned long ged_frr_debug_cid = GED_FRR_TABLE_ZERO;
static int ged_frr_debug_fps = GED_FRR_TABLE_ZERO;
static char *ged_frr_debug_dump_table;

module_param(ged_frr_debug_pid, int, S_IRUGO|S_IWUSR);
module_param(ged_frr_debug_cid, ulong, S_IRUGO|S_IWUSR);
module_param(ged_frr_debug_fps, int, S_IRUGO|S_IWUSR);
module_param(ged_frr_debug_dump_table, charp, S_IRUGO|S_IWUSR);

/*  the "FRR fps Table" */
typedef struct GED_FRR_FRR_TABLE_TYPE {
	int			pid;
	uint64_t	cid;
	int			fps;
	uint64_t	createTime;
} GED_FRR_FRR_TABLE;

static GED_FRR_FRR_TABLE *gpsFrrTable;
static int giFrrTableSize;

static void ged_frr_table_dump(void)
{
	int i;
	static char buf[1024];
	char temp[128];

	memset(buf, '\0', 1024);

	for (i = 0; i < giFrrTableSize; i++) {
		snprintf(temp, sizeof(temp),
				"%d,%llu,%d,%llu\n",
				gpsFrrTable[i].pid,
				gpsFrrTable[i].cid,
				gpsFrrTable[i].fps,
				gpsFrrTable[i].createTime);

		strcat(buf, temp);
	}

	ged_frr_debug_dump_table = buf;
}

static GED_ERROR ged_frr_table_create(void)
{
	int i;

	gpsFrrTable = NULL;
	giFrrTableSize = 0;

	giFrrTableSize = GED_FRR_TABLE_SIZE;
	gpsFrrTable = (GED_FRR_FRR_TABLE *)ged_alloc(giFrrTableSize * sizeof(GED_FRR_FRR_TABLE));

	if (!gpsFrrTable) {
		GED_LOGE("[FRR] ged_frr_table_create, gpsFrrTable(NULL)\n");
		return GED_ERROR_OOM;
	}

	for (i = 0; i < giFrrTableSize; i++) {
		gpsFrrTable[i].pid = GED_FRR_TABLE_ZERO;
		gpsFrrTable[i].cid = GED_FRR_TABLE_ZERO;
		gpsFrrTable[i].fps = GED_FRR_TABLE_ZERO;
		gpsFrrTable[i].createTime = GED_FRR_TABLE_ZERO;
	}

	gpsFrrTable[0].pid = GED_FRR_TABLE_FOR_ALL_PID;
	gpsFrrTable[0].cid = GED_FRR_TABLE_FOR_ALL_CID;

	ged_frr_table_dump();

	return GED_OK;
}

static GED_ERROR ged_frr_table_release(void)
{
	if (gpsFrrTable)
		ged_free(gpsFrrTable, giFrrTableSize * sizeof(GED_FRR_FRR_TABLE));

	return GED_OK;
}

static GED_ERROR ged_frr_table_add_item(int targetPid, uint64_t targetCid, int targetFps)
{
	int i;
	int targetIndex;
	unsigned long long leastCreateTime;

	GED_LOGE("[FRR] [+] ged_frr_table_add_item, pid(%d), cid(%llu), fps(%d)\n", targetPid, targetCid, targetFps);

	if (!gpsFrrTable) {
		GED_LOGE("[FRR] [-] ged_frr_table_add_item, gpsFrrTable(NULL)\n");
		return GED_ERROR_FAIL;
	}

	targetIndex = 1;
	leastCreateTime = gpsFrrTable[1].createTime;

	for (i = 1; i < giFrrTableSize; i++) {
		if (gpsFrrTable[i].pid == GED_FRR_TABLE_ZERO && gpsFrrTable[i].cid == GED_FRR_TABLE_ZERO) {
			targetIndex = i;
			break;
		}
		if (gpsFrrTable[i].createTime < leastCreateTime) {
			leastCreateTime = gpsFrrTable[i].createTime;
			targetIndex = i;
		}
	}

	gpsFrrTable[targetIndex].pid = targetPid;
	gpsFrrTable[targetIndex].cid = targetCid;
	gpsFrrTable[targetIndex].fps = targetFps;
	gpsFrrTable[targetIndex].createTime = ged_get_time();

	GED_LOGE("[FRR] [-] ged_frr_table_add_item, targetIndex(%d)\n", targetIndex);

	return GED_OK;
}

GED_ERROR ged_frr_system_init(void)
{
	return ged_frr_table_create();
}

GED_ERROR ged_frr_system_exit(void)
{
	return ged_frr_table_release();
}

GED_ERROR ged_frr_table_set_fps(int targetPid, uint64_t targetCid, int targetFps)
{
	int i;

	if (!gpsFrrTable) {
		GED_LOGE("[FRR] ged_frr_table_set_fps, gpsFrrTable(NULL)\n");
		return GED_ERROR_FAIL;
	}

	if (targetFps < GED_FRR_TABLE_LOWER_BOUND_FPS && targetFps != GED_FRR_TABLE_ZERO) {
		GED_LOGE("[FRR] ged_frr_table_set_fps, targetFps < 20\n");
		return GED_ERROR_INVALID_PARAMS;
	}

	if (targetPid == GED_FRR_TABLE_FOR_ALL_PID && targetCid == GED_FRR_TABLE_FOR_ALL_CID) {
		gpsFrrTable[0].fps = targetFps;
		ged_frr_table_dump();
		return GED_OK;
	}

	for (i = 1; i < giFrrTableSize; i++) {
		if (gpsFrrTable[i].pid == targetPid && gpsFrrTable[i].cid == targetCid) {
			gpsFrrTable[i].fps = targetFps;
			ged_frr_table_dump();
			return GED_OK;
		}
	}

	if (ged_frr_table_add_item(targetPid, targetCid, targetFps)) {
		ged_frr_table_dump();
		return GED_OK;
	}

	return GED_ERROR_FAIL;
}

int ged_frr_table_get_fps(int targetPid, uint64_t targetCid)
{
#if 0
	int i;
	int fps, forAllFps;

	if (!gpsFrrTable) {
		GED_LOGE("[FRR] ged_frr_table_get_fps, gpsFrrTable(NULL)\n");
		return GED_FRR_TABLE_ZERO;
	}

	if (ged_frr_debug_pid > GED_FRR_TABLE_ZERO && ged_frr_debug_cid > GED_FRR_TABLE_ZERO &&
		ged_frr_debug_fps > GED_FRR_TABLE_ZERO) {

		ged_frr_table_set_fps(ged_frr_debug_pid, ged_frr_debug_cid, ged_frr_debug_fps);
		ged_frr_debug_pid = GED_FRR_TABLE_ZERO;
		ged_frr_debug_cid = GED_FRR_TABLE_ZERO;
		ged_frr_debug_fps = GED_FRR_TABLE_ZERO;
	}

	fps = GED_FRR_TABLE_ZERO;
	forAllFps = gpsFrrTable[0].fps;

	for (i = 0; i < giFrrTableSize; i++) {
		if (gpsFrrTable[i].pid == targetPid && gpsFrrTable[i].cid == targetCid) {
			/*GED_LOGE("[FRR]get_fps: idx(%d), fps(%d)\n",i,gpsFrrTable[i].fps);*/
			fps = gpsFrrTable[i].fps;
			break;
		}
	}

	if (forAllFps > GED_FRR_TABLE_ZERO && fps > GED_FRR_TABLE_ZERO)
		return (forAllFps > fps) ? fps : forAllFps;

	if (forAllFps > GED_FRR_TABLE_ZERO)
		return forAllFps;

	return fps;
#endif
	int fps, mode;

	dfrc_get_frr_setting(targetPid, targetCid, &fps, &mode);

	if (mode != DFRC_DRV_MODE_FRR)
		fps = 60;

	return fps;
}

GED_ERROR ged_frr_wait_hw_vsync(void)
{
	int ret = 0;
	struct disp_session_vsync_config vsync_config;

	ret = primary_display_wait_for_vsync(&vsync_config);
	if (ret < 0) {
		GED_LOGE("[FRR] ged_frr_wait_hw_vsync, ret(%d)\n", ret);
		return GED_ERROR_FAIL;
	}
	return GED_OK;
}

