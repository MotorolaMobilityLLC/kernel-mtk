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
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <asm/atomic.h>
#include <linux/module.h>
#include <linux/version.h>
#include "ged_kpi.h"
#include "ged_base.h"
#include "ged_hashtable.h"
#include "ged_dvfs.h"
#include "ged_trace.h"
#include "ged_log.h"
#include "ged.h"
#include "ged_thread.h"
#include "ged_vsync.h"
#include <mt-plat/mtk_gpu_utility.h>

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0))
#include <linux/sync.h>
#else
#include <../drivers/staging/android/sync.h>
#endif


typedef enum
{
	GED_TIMESTAMP_TYPE_1 		= 0x1,
	GED_TIMESTAMP_TYPE_2 		= 0x2,
	GED_TIMESTAMP_TYPE_S		= 0x4,
	GED_TIMESTAMP_TYPE_H		= 0x8, 
} GED_TIMESTAMP_TYPE;

typedef struct GED_KPI_HEAD_TAG
{
	int tid;
	int i32Count;
	unsigned long long ullWnd;
	struct list_head sList;
} GED_KPI_HEAD;

typedef struct GED_KPI_TAG
{
	int tid;
	unsigned long long ullWnd;
	int i32FrameID;
	unsigned long ulMask;
	unsigned long long ullTimeStamp1;	
	unsigned long long ullTimeStamp2;	
	unsigned long long ullTimeStampS;
	unsigned long long ullTimeStampH;
	unsigned int gpu_freq;
	unsigned int gpu_loading;
	struct list_head sList;
} GED_KPI;

typedef struct GED_TIMESTAMP_TAG
{
	GED_TIMESTAMP_TYPE eTimeStampType;
	int tid;
	unsigned long long ullWnd;
	int i32FrameID;
	unsigned long long ullTimeStamp;
	struct work_struct sWork;	
} GED_TIMESTAMP;

typedef struct GED_KPI_GPU_TS_TAG
{
	int tid;
	unsigned long long ullWdnd;
	int i32FrameID;
	struct sync_fence_waiter    sSyncWaiter;
	struct sync_fence*          psSyncFence;
}GED_KPI_GPU_TS;

#define KPI_DEBUG 0
#define GED_KPI_TOTAL_ITEMS 64
#define GED_KPI_UID(tid, wnd) (tid | ((unsigned long)wnd))
#define FRAME_COUNT_TO_START_GED_KPI_INFO_CALC 10
#define FRAME_COUNT_TO_STOP_GED_KPI_INFO_CALC 5

#ifdef MTK_GED_KPI
static GED_LOG_BUF_HANDLE ghLogBuf = 0;
static struct workqueue_struct *g_psWorkQueue = NULL;
static GED_HASHTABLE_HANDLE gs_hashtable = NULL;
static GED_KPI g_asKPI[GED_KPI_TOTAL_ITEMS];
static int g_i32Pos = 0;
static int g_i32VSyncOffset = 0;
static int g_i32UncompletedCount = 0;
static int gx_i32UncompletedCount = 0;
static GED_THREAD_HANDLE ghThread = NULL;

static unsigned long long g_pre_TimeStamp1 = 0;
static unsigned long long g_pre_TimeStamp2 = 0;
static unsigned long long g_pre_TimeStampH = 0;
static unsigned long long g_elapsed_time_per_sec = 0;
static unsigned long long g_cpu_time_accum = 0;
static unsigned long long g_gpu_time_accum = 0;
static unsigned long long g_response_time_accum = 0;
static unsigned long long g_wait_4_hw_vsync_time_accum = 0;
static unsigned int g_gpu_freq_accum = 0;
static unsigned int g_frame_count = 0;

static unsigned int gx_fps = 0;
static unsigned int gx_cpu_time_avg = 0;
static unsigned int gx_gpu_time_avg = 0;
static unsigned int gx_response_time_avg = 0;
static unsigned int gx_wait_4_hw_vsync_time_avg = 0;
static unsigned int gx_gpu_freq_avg = 0;

static int g_active_count = 0;
static long g_active_ulID = -1;
static int g_kpi_info_start = 0;
//static struct mutex gsMutex;

extern unsigned int mt_gpufreq_get_cur_freq(void);

//-----------------------------------------------------------------------------
static inline void ged_kpi_clean_kpi_info(void){
	g_frame_count = 0;
	g_elapsed_time_per_sec = 0;
	g_cpu_time_accum = 0;
	g_gpu_time_accum = 0;
	g_response_time_accum = 0;
	g_wait_4_hw_vsync_time_accum = 0;
	g_gpu_freq_accum = 0;
}
//-----------------------------------------------------------------------------
static inline void ged_kpi_calc_kpi_info(unsigned long ulID, GED_KPI* psKPI)
{
	if(g_active_count==0){
		g_active_ulID = ulID;
	}

	if(g_active_ulID != ulID){
		g_active_count--;
		if(g_active_count<FRAME_COUNT_TO_STOP_GED_KPI_INFO_CALC)
		{
			g_kpi_info_start = 0;
			ged_kpi_clean_kpi_info();
		}
		return;
	}
	else if(g_active_count<FRAME_COUNT_TO_START_GED_KPI_INFO_CALC){
		g_active_count++;
		if(g_active_count==FRAME_COUNT_TO_START_GED_KPI_INFO_CALC)
		{
			g_kpi_info_start = 1;
			g_pre_TimeStamp1 = psKPI->ullTimeStamp1;
			g_pre_TimeStamp2 = psKPI->ullTimeStamp2;
			g_pre_TimeStampH = psKPI->ullTimeStampH;
		}
		return;
	}
	
//	printk("[Luise]: start KPI info calc, frame_count: %u\n", g_frame_count);

	if(g_kpi_info_start==1)
	{
		g_elapsed_time_per_sec += psKPI->ullTimeStampH - g_pre_TimeStampH;
		g_response_time_accum += psKPI->ullTimeStampH - psKPI->ullTimeStamp1;

		if(psKPI->ullTimeStamp1 > g_pre_TimeStamp2)
			g_gpu_time_accum += psKPI->ullTimeStamp2 - psKPI->ullTimeStamp1;
		else
			g_gpu_time_accum += psKPI->ullTimeStamp2 - g_pre_TimeStamp2;

		g_wait_4_hw_vsync_time_accum += psKPI->ullTimeStampH - psKPI->ullTimeStamp2;
		g_gpu_freq_accum += psKPI->gpu_freq;
		g_cpu_time_accum += psKPI->ullTimeStamp1 - g_pre_TimeStamp1;
		g_frame_count++;

		g_pre_TimeStamp1 = psKPI->ullTimeStamp1;
		g_pre_TimeStamp2 = psKPI->ullTimeStamp2;
		g_pre_TimeStampH = psKPI->ullTimeStampH;
		
		if(g_elapsed_time_per_sec >= 1000000000)
		{
			unsigned long long g_fps;
			g_fps = (unsigned long long)g_frame_count;
			g_fps *= 1000000000;
			do_div(g_fps, g_elapsed_time_per_sec);

			gx_fps = (unsigned int)g_fps;

			do_div(g_cpu_time_accum, g_frame_count);
			gx_cpu_time_avg = (unsigned int)g_cpu_time_accum;

			do_div(g_gpu_time_accum, g_frame_count);
			gx_gpu_time_avg = (unsigned int)g_gpu_time_accum;

			do_div(g_response_time_accum, g_frame_count);
			gx_response_time_avg = (unsigned int)g_response_time_accum;

			do_div(g_wait_4_hw_vsync_time_accum, g_frame_count);
			gx_wait_4_hw_vsync_time_avg = (unsigned int)g_wait_4_hw_vsync_time_accum;

			do_div(g_gpu_freq_accum, g_frame_count);
			gx_gpu_freq_avg = g_gpu_freq_accum;

			ged_kpi_clean_kpi_info();
		}
	}
}
//-----------------------------------------------------------------------------
static void ged_kpi_statistics_and_remove(GED_KPI_HEAD* psHead, GED_KPI* psKPI)
{
	unsigned long ulID = GED_KPI_UID(psKPI->tid, psKPI->ullWnd);

	ged_kpi_calc_kpi_info(ulID, psKPI);

	/* remove */
	psHead->i32Count -= 1;	
	list_del(&psKPI->sList);		
	ged_trace_counter(ulID, "QueuedFrame", psHead->i32Count);

	/* statistics */
#if 1
	ged_log_buf_print(ghLogBuf, "%d,%llu,%llu,%llu,%llu,%llu,%u,%u",
		psHead->tid,
		psHead->ullWnd,
		psKPI->ullTimeStamp1,
		psKPI->ullTimeStamp2,
		psKPI->ullTimeStampS,
		psKPI->ullTimeStampH,
		psKPI->gpu_freq,
		psKPI->gpu_loading);
#else
	printk("GED KPI (%d,%llu,%d): %llu,%llu,%llu,%llu\n",
		psHead->tid,
		psHead->ullWnd,
		psHead->i32Count,
		psKPI->ullTimeStamp1,
		psKPI->ullTimeStamp2,
		psKPI->ullTimeStampS,
		psKPI->ullTimeStampH);
#endif
}
//-----------------------------------------------------------------------------
static GED_BOOL ged_kpi_s_iterator_func(unsigned long ulID, void* pvoid, void* pvParam)
{
	GED_KPI_HEAD* psHead = (GED_KPI_HEAD*)pvoid;
	GED_TIMESTAMP* psTimeStamp = (GED_TIMESTAMP*)pvParam;
	GED_KPI* psKPI = NULL;

	if (psHead)
	{
		struct list_head *psListEntry, *psListEntryTemp;
		struct list_head *psList = &psHead->sList;

		if (psHead->i32Count < g_i32UncompletedCount)
		{
			g_i32UncompletedCount = psHead->i32Count;
		}
		
		list_for_each_safe(psListEntry, psListEntryTemp, psList)
		{
			psKPI = list_entry(psListEntry, GED_KPI, sList);
			if (psKPI)
			{		
				if ((psKPI->ulMask & GED_TIMESTAMP_TYPE_1) && (0 == (psKPI->ulMask & GED_TIMESTAMP_TYPE_S)))
				{
					psKPI->ulMask |= GED_TIMESTAMP_TYPE_S;
					psKPI->ullTimeStampS = psTimeStamp->ullTimeStamp;					
					break;
				}
			}
		}
	}
	return GED_TRUE;
}
//-----------------------------------------------------------------------------
static GED_BOOL ged_kpi_h_iterator_func(unsigned long ulID, void* pvoid, void* pvParam)
{
	GED_KPI_HEAD* psHead = (GED_KPI_HEAD*)pvoid;
	GED_TIMESTAMP* psTimeStamp = (GED_TIMESTAMP*)pvParam;
	GED_KPI* psKPI = NULL;

	if (psHead)
	{
		struct list_head *psListEntry, *psListEntryTemp;
		struct list_head *psList = &psHead->sList;
		list_for_each_safe(psListEntry, psListEntryTemp, psList)
		{
			psKPI = list_entry(psListEntry, GED_KPI, sList);		
			if (psKPI)
			{
				if ((psKPI->ulMask & GED_TIMESTAMP_TYPE_S) && (psKPI->ulMask & GED_TIMESTAMP_TYPE_2))
				{
					if (0 == (psKPI->ulMask & GED_TIMESTAMP_TYPE_H))
					{
						psKPI->ulMask |= GED_TIMESTAMP_TYPE_H;
						psKPI->ullTimeStampH = psTimeStamp->ullTimeStamp;					
						ged_kpi_statistics_and_remove(psHead, psKPI);
						break;
					}
				}
			}
		}
	}
	return GED_TRUE;
}
//-----------------------------------------------------------------------------
static GED_BOOL ged_kpi_iterator_delete_func(unsigned long ulID, void* pvoid, void* pvParam, GED_BOOL* pbDeleted)
{
	GED_KPI_HEAD* psHead = (GED_KPI_HEAD*)pvoid;

	if (psHead)
	{
		ged_free(psHead, sizeof(GED_KPI_HEAD));
		*pbDeleted = GED_TRUE;
	}
	
	return GED_TRUE;
}
//-----------------------------------------------------------------------------
static void ged_kpi_work_cb(struct work_struct *psWork)
{
	GED_TIMESTAMP* psTimeStamp = GED_CONTAINER_OF(psWork, GED_TIMESTAMP, sWork);	
	GED_KPI_HEAD* psHead;
	GED_KPI* psKPI;		
	unsigned long ulID;

#if 0
	printk("GED type = %d, tid = %d, wnd = %llu, frame = %d\n", 
		psTimeStamp->eTimeStampType, psTimeStamp->tid, psTimeStamp->ullWnd, psTimeStamp->i32FrameID);
#endif

	//mutex_lock(&gsMutex);

	switch(psTimeStamp->eTimeStampType)
	{
	case GED_TIMESTAMP_TYPE_1:		
		psKPI = &g_asKPI[g_i32Pos++];
		if (g_i32Pos >= GED_KPI_TOTAL_ITEMS)
		{
			g_i32Pos = 0;
		}

		/* remove */
		ulID = GED_KPI_UID(psKPI->tid, psKPI->ullWnd);
		psHead = (GED_KPI_HEAD*)ged_hashtable_find(gs_hashtable, ulID);
		if (psHead)
		{
			if ((0 == (psKPI->ulMask & GED_TIMESTAMP_TYPE_2)) || (0 == (psKPI->ulMask & GED_TIMESTAMP_TYPE_H)))
			{
				psHead->i32Count -= 1;
				list_del(&psKPI->sList);
				ged_trace_counter(ulID, "QueuedFrame", psHead->i32Count);				
			}
			if (1 > psHead->i32Count)
			{			
				ged_hashtable_remove(gs_hashtable, ulID);
				ged_free(psHead, sizeof(GED_KPI_HEAD));				
			}
		}

		/* reset data */
		memset(psKPI, 0, sizeof(GED_KPI));
		INIT_LIST_HEAD(&psKPI->sList);

		/* add */
		ulID = GED_KPI_UID(psTimeStamp->tid, psTimeStamp->ullWnd);
		psHead = (GED_KPI_HEAD*)ged_hashtable_find(gs_hashtable, ulID);
		if (!psHead)
		{				
			psHead = (GED_KPI_HEAD*)ged_alloc_atomic(sizeof(GED_KPI_HEAD));
			if (psHead)
			{
				psHead->tid = psTimeStamp->tid;
				psHead->ullWnd = psTimeStamp->ullWnd;		
				psHead->i32Count = 0;
				INIT_LIST_HEAD(&psHead->sList);
				ged_hashtable_set(gs_hashtable, ulID, (void*)psHead);
			}
		}
		if (psHead)
		{
			/* new data */
			psKPI->tid = psTimeStamp->tid;
			psKPI->ullWnd = psTimeStamp->ullWnd;
			psKPI->i32FrameID = psTimeStamp->i32FrameID;
			psKPI->ulMask = GED_TIMESTAMP_TYPE_1;
			psKPI->ullTimeStamp1 = psTimeStamp->ullTimeStamp;		
			list_add_tail(&psKPI->sList, &psHead->sList);
			psHead->i32Count += 1;
			ged_trace_counter(ulID, "QueuedFrame", psHead->i32Count);
#if 0			
			{
				
				int i32Count = ged_hashtable_get_count(gs_hashtable);
				printk("GED KPI (%d, %llu, %d): total(%d)\n", psHead->tid, psHead->ullWnd, psHead->i32Count, i32Count);
			}
#endif
		}
		break;		
	case GED_TIMESTAMP_TYPE_2:
		ulID = GED_KPI_UID(psTimeStamp->tid, psTimeStamp->ullWnd);
		psHead = (GED_KPI_HEAD*)ged_hashtable_find(gs_hashtable, ulID);
		if (psHead)
		{
			struct list_head *psListEntry, *psListEntryTemp;
			struct list_head *psList = &psHead->sList;
			list_for_each_safe(psListEntry, psListEntryTemp, psList)
			{
				psKPI = list_entry(psListEntry, GED_KPI, sList);
				if (psKPI)
				{
					if (psKPI->i32FrameID == psTimeStamp->i32FrameID)
					{					
						if (0 == (psKPI->ulMask & GED_TIMESTAMP_TYPE_2))
						{
							psKPI->ulMask |= GED_TIMESTAMP_TYPE_2;
							psKPI->ullTimeStamp2 = psTimeStamp->ullTimeStamp;
							psKPI->gpu_freq = mt_gpufreq_get_cur_freq();
							mtk_get_gpu_loading(&psKPI->gpu_loading);
						}
						else
						{
							printk("GED KPI Exception!\n");
						}
						break;						
					}
				}
			}
		}		
		break;
	case GED_TIMESTAMP_TYPE_S:
		g_i32UncompletedCount = GED_KPI_TOTAL_ITEMS;
		ged_hashtable_iterator(gs_hashtable, ged_kpi_s_iterator_func, (void*)psTimeStamp);
		gx_i32UncompletedCount = g_i32UncompletedCount;
		break;
	case GED_TIMESTAMP_TYPE_H:		
		g_i32VSyncOffset = ged_dvfs_vsync_offset_level_get();
		if (0 == g_i32VSyncOffset)
		{
			g_i32VSyncOffset = 16666666;
		}
		ged_hashtable_iterator(gs_hashtable, ged_kpi_h_iterator_func, (void*)psTimeStamp);
		break;
	default:
		break;
	}


	//mutex_unlock(&gsMutex);

	ged_free(psTimeStamp, sizeof(GED_TIMESTAMP));
}
//-----------------------------------------------------------------------------
static GED_ERROR ged_kpi_push_timestamp(
	GED_TIMESTAMP_TYPE eTimeStampType,
	unsigned long long ullTimeStamp,
	int tid,
	unsigned long long ullWnd,
	int i32FrameID)
{
	if (g_psWorkQueue)
	{
		GED_TIMESTAMP* psTimeStamp = (GED_TIMESTAMP*)ged_alloc_atomic(sizeof(GED_TIMESTAMP));
		if (!psTimeStamp)
		{
			return GED_ERROR_OOM;
		}

		psTimeStamp->eTimeStampType = eTimeStampType;
		psTimeStamp->ullTimeStamp = ullTimeStamp;
		psTimeStamp->tid = tid;
		psTimeStamp->ullWnd = ullWnd;
		psTimeStamp->i32FrameID = i32FrameID;
		INIT_WORK(&psTimeStamp->sWork, ged_kpi_work_cb);

		queue_work(g_psWorkQueue, &psTimeStamp->sWork);
	}
	return GED_OK;
}
//-----------------------------------------------------------------------------
static GED_ERROR ged_kpi_time1(int tid, unsigned long long ullWdnd, int i32FrameID)
{
	return ged_kpi_push_timestamp(GED_TIMESTAMP_TYPE_1, ged_get_time(), tid, ullWdnd, i32FrameID);
}
//-----------------------------------------------------------------------------
static GED_ERROR ged_kpi_time2(int tid, unsigned long long ullWdnd, int i32FrameID)
{
	return ged_kpi_push_timestamp(GED_TIMESTAMP_TYPE_2, ged_get_time(), tid, ullWdnd, i32FrameID);
}
//-----------------------------------------------------------------------------
static void ged_kpi_gpu_ts_sync_cb(struct sync_fence *fence, struct sync_fence_waiter *waiter)
{
	GED_KPI_GPU_TS *psMonitor;
	psMonitor = GED_CONTAINER_OF(waiter, GED_KPI_GPU_TS, sSyncWaiter);
//	printk("[Luise]: ged_kpi_gpu_ts_sync_cb(), fence: %p, %d, %llu, %d ", fence, psMonitor->tid, psMonitor->ullWdnd, psMonitor->i32FrameID);

	ged_kpi_time2(psMonitor->tid, psMonitor->ullWdnd, psMonitor->i32FrameID);

	sync_fence_put(psMonitor->psSyncFence);
	ged_free(psMonitor, sizeof(GED_KPI_GPU_TS));
}
//-----------------------------------------------------------------------------
static void ged_kpi_wait_for_hw_vsync_cb(void* data)
{
	while(1){
		printk("[Luise]: ged_kpi_wait_for_hw_vsync_cb()\n");
		ged_vsync_wait();
		ged_notification(GED_NOTIFICATION_TYPE_HW_VSYNC_PRIMARY_DISPLAY);
	}
}
#endif
//-----------------------------------------------------------------------------
GED_ERROR ged_kpi_gpu_ts(int tid, unsigned long long ullWdnd, int i32FrameID, int fence_fd)
{
#ifdef MTK_GED_KPI
	GED_ERROR ret;
	GED_KPI_GPU_TS *psMonitor;

	ret = ged_kpi_time1(tid, ullWdnd, i32FrameID);
	if(ret != GED_OK) return ret;

	psMonitor = (GED_KPI_GPU_TS*)ged_alloc(sizeof(GED_KPI_GPU_TS));

	if (!psMonitor) return GED_ERROR_OOM;
	
	sync_fence_waiter_init(&psMonitor->sSyncWaiter, ged_kpi_gpu_ts_sync_cb);
	psMonitor->psSyncFence = sync_fence_fdget(fence_fd);
	psMonitor->tid = tid;
	psMonitor->ullWdnd = ullWdnd;
	psMonitor->i32FrameID = i32FrameID;
	// return ret;
//	printk("[Luise]: ged_kpi_gpu_ts(), fence: %p, %d, %llu, %d\n", psMonitor->psSyncFence, psMonitor->tid, psMonitor->ullWdnd, psMonitor->i32FrameID);

	if (NULL == psMonitor->psSyncFence)
	{
		ged_free(psMonitor, sizeof(GED_KPI_GPU_TS));
		return GED_ERROR_INVALID_PARAMS;
	}

	ret = sync_fence_wait_async(psMonitor->psSyncFence, &psMonitor->sSyncWaiter);

	if ((1 == ret) || (0 > ret))
	{
		sync_fence_put(psMonitor->psSyncFence);
		ged_free(psMonitor, sizeof(GED_KPI_GPU_TS));
		ret = ged_kpi_time2(tid, ullWdnd, i32FrameID);
	}
	return ret;
#else
	return GED_OK;
#endif
}
//-----------------------------------------------------------------------------
GED_ERROR ged_kpi_sw_vsync(void)
{
#ifdef MTK_GED_KPI
	return ged_kpi_push_timestamp(GED_TIMESTAMP_TYPE_S, ged_get_time(), 0, 0, 0);
#else
	return GED_OK;
#endif
}
//-----------------------------------------------------------------------------
GED_ERROR ged_kpi_hw_vsync(void)
{
#ifdef MTK_GED_KPI
	return ged_kpi_push_timestamp(GED_TIMESTAMP_TYPE_H, ged_get_time(), 0, 0, 0);
#else
	return GED_OK;
#endif
}
//-----------------------------------------------------------------------------
int ged_kpi_get_uncompleted_count(void)
{
#ifdef MTK_GED_KPI
	return gx_i32UncompletedCount;
#else
	return 0;
#endif
}
//-----------------------------------------------------------------------------
unsigned int ged_kpi_get_fps(void)
{
#ifdef MTK_GED_KPI
	return gx_fps;
#else
	return 0;
#endif
}
//-----------------------------------------------------------------------------
unsigned int ged_kpi_get_avg_cpu_time(void)
{
#ifdef MTK_GED_KPI
	return gx_cpu_time_avg;
#else
	return 0;
#endif
}
//-----------------------------------------------------------------------------
unsigned int ged_kpi_get_avg_gpu_time(void)
{
#ifdef MTK_GED_KPI
	return gx_gpu_time_avg;
#else
	return 0;
#endif
}
//-----------------------------------------------------------------------------
unsigned int ged_kpi_get_avg_response_time(void)
{
#ifdef MTK_GED_KPI
	return gx_response_time_avg;
#else
	return 0;
#endif
}
//-----------------------------------------------------------------------------
unsigned int ged_kpi_get_avg_wait_4_hw_vsync_time(void)
{
#ifdef MTK_GED_KPI
	return gx_wait_4_hw_vsync_time_avg;
#else
	return 0;
#endif
}
//-----------------------------------------------------------------------------
unsigned int ged_kpi_get_avg_gpu_freq(void)
{
#ifdef MTK_GED_KPI
	return gx_gpu_freq_avg;
#else
	return 0;
#endif
}
//-----------------------------------------------------------------------------
GED_ERROR ged_kpi_system_init(void)
{
#ifdef MTK_GED_KPI
	GED_ERROR ret;

	//mutex_init(&gsMutex);
	ghLogBuf = ged_log_buf_alloc(60 *10, 100 * 60 * 10, GED_LOG_BUF_TYPE_RINGBUFFER, NULL, "KPI");

	printk("[Luise] ged_thread_create() start\n");
	ret = ged_thread_create(&ghThread, "kpi_wait_4_hw_vsync", ged_kpi_wait_for_hw_vsync_cb, (void*)&ghLogBuf);
	if(ret != GED_OK)
	{
		printk("[Luise] ged_thread_create() failed\n");
		return ret;
	}

	g_psWorkQueue = alloc_ordered_workqueue("ged_kpi", WQ_FREEZABLE | WQ_MEM_RECLAIM);
	if (g_psWorkQueue)
	{
		memset(g_asKPI, 0, sizeof(g_asKPI));
		gs_hashtable = ged_hashtable_create(10);
		return gs_hashtable ? GED_OK : GED_ERROR_FAIL;
	}
	return GED_ERROR_FAIL;
#else
	return GED_OK;
#endif
}
//-----------------------------------------------------------------------------
void ged_kpi_system_exit(void)
{
#ifdef MTK_GED_KPI
	ged_hashtable_iterator_delete(gs_hashtable, ged_kpi_iterator_delete_func, NULL);
	destroy_workqueue(g_psWorkQueue);
	ged_thread_destroy(ghThread);
	ged_log_buf_free(ghLogBuf);
    ghLogBuf = 0;
#endif
}
