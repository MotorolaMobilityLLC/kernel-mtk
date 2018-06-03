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

#include <linux/version.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <asm/atomic.h>
#include <linux/module.h>

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0))
#include <linux/sync.h>
#else
#include <../drivers/staging/android/sync.h>
#endif

#include <mt-plat/mtk_gpu_utility.h>
#include <trace/events/gpu.h>
#include <mtk_gpufreq.h>

#include "ged_monitor_3D_fence.h"

#include "ged_log.h"
#include "ged_base.h"
#include "ged_type.h"
#include "ged_dvfs.h"

#include <asm/div64.h>

static atomic_t g_i32Count = ATOMIC_INIT(0);
static unsigned int ged_monitor_3D_fence_debug;
static unsigned int ged_monitor_3D_fence_disable;
static unsigned int ged_monitor_3D_fence_switch = 1;
static unsigned int ged_monitor_3D_fence_systrace;
static unsigned long g_ul3DFenceDoneTime;

extern bool mtk_get_bottom_gpu_freq(unsigned int *pui32FreqLevel);

#ifdef GED_DEBUG_MONITOR_3D_FENCE
extern GED_LOG_BUF_HANDLE ghLogBuf_GED;
#endif
extern GED_LOG_BUF_HANDLE ghLogBuf_DVFS;

typedef struct GED_MONITOR_3D_FENCE_TAG
{
	struct sync_fence_waiter    sSyncWaiter;
	struct work_struct          sWork;
	struct sync_fence*          psSyncFence;
} GED_MONITOR_3D_FENCE;

static void ged_sync_cb(struct sync_fence *fence, struct sync_fence_waiter *waiter)
{
	GED_MONITOR_3D_FENCE *psMonitor;
	unsigned long long t;

	t = ged_get_time();

	do_div(t,1000);

	ged_monitor_3D_fence_notify();

#if defined(CONFIG_MACH_MT6799) || defined(CONFIG_MACH_MT8167) || defined(CONFIG_MACH_MT8173)
	/* FIX-ME: IMG's loading API requires mutext lock which is not suitable here */;
#else
#ifndef GED_ENABLE_FB_DVFS
	ged_dvfs_cal_gpu_utilization_force();
#endif
#endif

	psMonitor = GED_CONTAINER_OF(waiter, GED_MONITOR_3D_FENCE, sSyncWaiter);

	ged_log_buf_print(ghLogBuf_DVFS, "[-] ged_monitor_3D_fence_done (ts=%llu) %p", t, psMonitor->psSyncFence);

	schedule_work(&psMonitor->sWork);
}

static void ged_monitor_3D_fence_work_cb(struct work_struct *psWork)
{
	GED_MONITOR_3D_FENCE *psMonitor;

#ifdef GED_DEBUG_MONITOR_3D_FENCE
	ged_log_buf_print(ghLogBuf_GED, "ged_monitor_3D_fence_work_cb");
#endif

#ifndef GED_ENABLE_FB_DVFS
	ged_dvfs_cal_gpu_utilization_force();
#endif

	atomic_sub_return(1, &g_i32Count);

	if (ged_monitor_3D_fence_debug > 0)
		GED_LOGI("[-]3D fences count = %d\n", atomic_read(&g_i32Count));

	psMonitor = GED_CONTAINER_OF(psWork, GED_MONITOR_3D_FENCE, sWork);
	sync_fence_put(psMonitor->psSyncFence);
	ged_free(psMonitor, sizeof(GED_MONITOR_3D_FENCE));
}

unsigned long ged_monitor_3D_fence_done_time()
{
	return g_ul3DFenceDoneTime;
}

GED_ERROR ged_monitor_3D_fence_add(int fence_fd)
{
	int err;
	unsigned long long t;
	GED_MONITOR_3D_FENCE* psMonitor;

	if (ged_monitor_3D_fence_disable)
		return GED_OK;

	t = ged_get_time();

	do_div(t,1000);

	psMonitor = (GED_MONITOR_3D_FENCE*)ged_alloc(sizeof(GED_MONITOR_3D_FENCE));

#ifdef GED_DEBUG_MONITOR_3D_FENCE
	ged_log_buf_print(ghLogBuf_GED, "[+]ged_monitor_3D_fence_add");
#endif

	if (!psMonitor)
		return GED_ERROR_OOM;

	sync_fence_waiter_init(&psMonitor->sSyncWaiter, ged_sync_cb);
	INIT_WORK(&psMonitor->sWork, ged_monitor_3D_fence_work_cb);
	psMonitor->psSyncFence = sync_fence_fdget(fence_fd);
	if (psMonitor->psSyncFence == NULL) {
		ged_free(psMonitor, sizeof(GED_MONITOR_3D_FENCE));
		return GED_ERROR_INVALID_PARAMS;
	}

	ged_log_buf_print(ghLogBuf_DVFS, "[+] ged_monitor_3D_fence_add (ts=%llu) %p", t, psMonitor->psSyncFence);

#ifdef GED_DEBUG_MONITOR_3D_FENCE
	ged_log_buf_print(ghLogBuf_GED, "[+]sync_fence_wait_async");
#endif

	err = sync_fence_wait_async(psMonitor->psSyncFence, &psMonitor->sSyncWaiter);

#ifdef GED_DEBUG_MONITOR_3D_FENCE
	ged_log_buf_print(ghLogBuf_GED, "[-]sync_fence_wait_async, err = %d", err);
#endif

	if ((err == 1) || (err < 0)) {
		sync_fence_put(psMonitor->psSyncFence);
		ged_free(psMonitor, sizeof(GED_MONITOR_3D_FENCE));
	} else if (err == 0) {
		atomic_add_return(1, &g_i32Count);
	}

	if (ged_monitor_3D_fence_debug > 0)
		GED_LOGI("[+]3D fences count = %d\n", atomic_read(&g_i32Count));

#ifdef GED_DEBUG_MONITOR_3D_FENCE
	ged_log_buf_print(ghLogBuf_GED, "[-]ged_monitor_3D_fence_add, count = %d", atomic_read(&g_i32Count));
#endif

	return GED_OK;
}

void ged_monitor_3D_fence_set_enable(GED_BOOL bEnable)
{
	if (bEnable != ged_monitor_3D_fence_switch)
		ged_monitor_3D_fence_switch = bEnable;
}

void ged_monitor_3D_fence_notify(void)
{
	unsigned long long t;

	t = ged_get_time();

	do_div(t,1000);

	g_ul3DFenceDoneTime = (unsigned long)t;
}

int ged_monitor_3D_fence_get_count(void)
{
	return atomic_read(&g_i32Count);
}

module_param(ged_monitor_3D_fence_debug, uint, 0644);
module_param(ged_monitor_3D_fence_disable, uint, 0644);
module_param(ged_monitor_3D_fence_systrace, uint, 0644);
