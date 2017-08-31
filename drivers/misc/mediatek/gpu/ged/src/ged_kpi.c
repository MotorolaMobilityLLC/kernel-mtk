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
#include <linux/atomic.h>
#include <linux/module.h>
#include <linux/version.h>
#include <ged_kpi.h>
#include <ged_base.h>
#include <ged_hashtable.h>
#include <ged_dvfs.h>
#include <ged_log.h>
#include <ged.h>
#include "ged_thread.h"
/* #include <ged_vsync.h> */
#include <primary_display.h>
#include <mt-plat/mtk_gpu_utility.h>
#ifdef GED_DVFS_ENABLE
#include <mtk_gpufreq.h>
#endif

/* #include <mt-plat/met_drv.h> */

#if (KERNEL_VERSION(3, 10, 0) > LINUX_VERSION_CODE)
#include <linux/sync.h>
#else
#include <../drivers/staging/android/sync.h>
#endif

#ifdef GED_KPI_DEBUG
#undef GED_LOGE
#define GED_LOGE printk
#endif

typedef enum {
	GED_TIMESTAMP_TYPE_1		= 0x1,
	GED_TIMESTAMP_TYPE_2		= 0x2,
	GED_TIMESTAMP_TYPE_S		= 0x4,
	GED_TIMESTAMP_TYPE_H		= 0x8,
} GED_TIMESTAMP_TYPE;

typedef struct GED_KPI_HEAD_TAG {
	int pid;
	int i32Count;
	unsigned long long ullWnd;
	unsigned long long last_TimeStamp1;
	unsigned long long last_TimeStamp2;
	unsigned long long last_TimeStampS;
	unsigned long long last_TimeStampH;
	long long t_cpu_remained;
	long long t_gpu_remained;
	unsigned long long t_cpu_latest;
	unsigned long long t_gpu_latest;
	struct list_head sList;
	int i32QedBuffer_length;
	int i32Gpu_uncompleted;
	int isSF;
} GED_KPI_HEAD;

typedef struct GED_KPI_TAG {
	int pid;
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
	long long t_cpu_remained;
	long long t_gpu_remained;
} GED_KPI;

typedef struct GED_TIMESTAMP_TAG {
	GED_TIMESTAMP_TYPE eTimeStampType;
	int pid;
	unsigned long long ullWnd;
	int i32FrameID;
	unsigned long long ullTimeStamp;
	int i32QedBuffer_length;
	int isSF;
	struct work_struct sWork;
} GED_TIMESTAMP;

typedef struct GED_KPI_GPU_TS_TAG {
	int pid;
	unsigned long long ullWdnd;
	int i32FrameID;
	struct sync_fence_waiter sSyncWaiter;
	struct sync_fence *psSyncFence;
} GED_KPI_GPU_TS;







#define GED_KPI_TOTAL_ITEMS 64
#define GED_KPI_UID(pid, wnd) (pid | ((unsigned long)wnd))
#define SCREEN_IDLE_PERIOD 500000000

#ifdef MTK_GED_KPI
static GED_LOG_BUF_HANDLE ghLogBuf;
static struct workqueue_struct *g_psWorkQueue;
static GED_HASHTABLE_HANDLE gs_hashtable;
static GED_KPI g_asKPI[GED_KPI_TOTAL_ITEMS];
static int g_i32Pos;
static GED_THREAD_HANDLE ghThread;
static unsigned int gx_dfps = 60; /* dynamic FPS*/
module_param(gx_dfps, uint, S_IRUGO|S_IWUSR);


/* for calculating remained time budgets of CPU and GPU:
 *		time budget: the buffering time that prevents fram drop
 */
typedef struct GED_KPI_PERF_STATE_TAG {
	long t_cpu_target;
	long t_gpu_target;
} GED_KPI_PERF_STATE;

static GED_KPI_PERF_STATE gx_perf_state = {.t_cpu_target = 16666666, .t_gpu_target = 16666666};
GED_KPI_HEAD *main_head;
GED_KPI_HEAD *prev_main_head;
/* end */

/* for calculating KPI info per second */
static unsigned long long g_pre_TimeStamp1;
static unsigned long long g_pre_TimeStamp2;
static unsigned long long g_pre_TimeStampH;
static unsigned long long g_elapsed_time_per_sec;
static unsigned long long g_cpu_time_accum;
static unsigned long long g_gpu_time_accum;
static unsigned long long g_response_time_accum;
static unsigned long long g_gpu_remained_time_accum;
static unsigned long long g_cpu_remained_time_accum;
static unsigned int g_gpu_freq_accum;
static unsigned int g_frame_count;

static int gx_game_mode;
static unsigned int gx_fps;
static unsigned int gx_cpu_time_avg;
static unsigned int gx_gpu_time_avg;
static unsigned int gx_response_time_avg;
static unsigned int gx_gpu_remained_time_avg;
static unsigned int gx_cpu_remained_time_avg;
static unsigned int gx_gpu_freq_avg;

module_param(gx_game_mode, int, S_IRUGO|S_IWUSR);

/* ----------------------------------------------------------------------------- */
static inline void ged_kpi_clean_kpi_info(void)
{
	g_frame_count = 0;
	g_elapsed_time_per_sec = 0;
	g_cpu_time_accum = 0;
	g_gpu_time_accum = 0;
	g_response_time_accum = 0;
	g_gpu_remained_time_accum = 0;
	g_cpu_remained_time_accum = 0;
	g_gpu_freq_accum = 0;
}
/* ----------------------------------------------------------------------------- */
static GED_BOOL ged_kpi_find_main_head_func(unsigned long ulID, void *pvoid, void *pvParam)
{
	GED_KPI_HEAD *psHead = (GED_KPI_HEAD *)pvoid;

	if (psHead) {
#ifdef GED_KPI_DEBUG
		GED_LOGE("[GED_KPI] psHead->i32Count: %d, isSF: %d\n", psHead->i32Count, psHead->isSF);
#endif
		if (psHead->isSF == 0) {
			if (main_head == NULL || psHead->i32Count > main_head->i32Count)
				main_head = psHead;
		}
	}
	return GED_TRUE;
}
/* ----------------------------------------------------------------------------- */
/* for calculating average per-second performance info */
/* ----------------------------------------------------------------------------- */
static inline void ged_kpi_calc_kpi_info(unsigned long ulID, GED_KPI *psKPI, GED_KPI_HEAD *psHead)
{
	ged_hashtable_iterator(gs_hashtable, ged_kpi_find_main_head_func, (void *)NULL);
#ifdef GED_KPI_DEBUG
	/* check if there is a main rendering thread */
	/* only surfaceflinger is excluded from the group of considered candidates */
	if (main_head)
		GED_LOGE("[GED_KPI] main_head = %p, i32Count= %d, i32QedBuffer_length=%d\n", main_head, main_head->i32Count, main_head->i32QedBuffer_length);
	else
		GED_LOGE("[GED_KPI] main_head = NULL\n");
#endif

	if (main_head != prev_main_head && main_head == psHead) {
		ged_kpi_clean_kpi_info();
		g_pre_TimeStamp1 = psKPI->ullTimeStamp1;
		g_pre_TimeStamp2 = psKPI->ullTimeStamp2;
		g_pre_TimeStampH = psKPI->ullTimeStampH;
		prev_main_head = main_head;
		return;
	}

	if (psHead == main_head) {
		g_elapsed_time_per_sec += psKPI->ullTimeStampH - g_pre_TimeStampH;
		g_response_time_accum += psKPI->ullTimeStampH - g_pre_TimeStamp1;

		if (psKPI->ullTimeStamp1 > g_pre_TimeStamp2)
			g_gpu_time_accum += psKPI->ullTimeStamp2 - psKPI->ullTimeStamp1;
		else
			g_gpu_time_accum += psKPI->ullTimeStamp2 - g_pre_TimeStamp2;

		g_gpu_remained_time_accum += psKPI->ullTimeStampH - psKPI->ullTimeStamp2;
		g_cpu_remained_time_accum += psKPI->ullTimeStampS - psKPI->ullTimeStamp1;
		g_gpu_freq_accum += psKPI->gpu_freq;
		g_cpu_time_accum += psKPI->ullTimeStamp1 - g_pre_TimeStamp1;
		g_frame_count++;

		g_pre_TimeStamp1 = psKPI->ullTimeStamp1;
		g_pre_TimeStamp2 = psKPI->ullTimeStamp2;
		g_pre_TimeStampH = psKPI->ullTimeStampH;

		if (g_elapsed_time_per_sec >= 1000000000) {
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

			do_div(g_gpu_remained_time_accum, g_frame_count);
			gx_gpu_remained_time_avg = (unsigned int)g_gpu_remained_time_accum;

			do_div(g_cpu_remained_time_accum, g_frame_count);
			gx_cpu_remained_time_avg = (unsigned int)g_cpu_remained_time_accum;

			do_div(g_gpu_freq_accum, g_frame_count);
			gx_gpu_freq_avg = g_gpu_freq_accum;

			ged_kpi_clean_kpi_info();
		}
	}
}
/* ----------------------------------------------------------------------------- */
static void ged_kpi_statistics_and_remove(GED_KPI_HEAD *psHead, GED_KPI *psKPI)
{
	unsigned long ulID = (unsigned long) psKPI->ullWnd;

	ged_kpi_calc_kpi_info(ulID, psKPI, psHead);

	/* statistics */
	ged_log_buf_print(ghLogBuf, "%d,%llu,%llu,%llu,%llu,%llu,%u,%u,%lld,%lld",
		psHead->pid,
		psHead->ullWnd,
		psKPI->ullTimeStamp1,
		psKPI->ullTimeStamp2,
		psKPI->ullTimeStampS,
		psKPI->ullTimeStampH,
		psKPI->gpu_freq,
		psKPI->gpu_loading,
		psKPI->t_cpu_remained,
		psKPI->t_gpu_remained
		);
}
/* ----------------------------------------------------------------------------- */
static GED_BOOL ged_kpi_tag_type_s(unsigned long ulID, GED_KPI_HEAD *psHead, GED_TIMESTAMP *psTimeStamp)
{
	GED_KPI *psKPI = NULL;
	GED_KPI *psKPI_prev = NULL;

	if (psHead) {
		struct list_head *psListEntry, *psListEntryTemp;
		struct list_head *psList = &psHead->sList;

		list_for_each_prev_safe(psListEntry, psListEntryTemp, psList) {
			psKPI = list_entry(psListEntry, GED_KPI, sList);
			if (psKPI && psKPI->ulMask & GED_TIMESTAMP_TYPE_S) {
				if (psKPI_prev) {
					psKPI_prev->ulMask |= GED_TIMESTAMP_TYPE_S;
					psKPI_prev->ullTimeStampS =
						psTimeStamp->ullTimeStamp;
					psHead->t_cpu_remained =
						psTimeStamp->ullTimeStamp - psKPI_prev->ullTimeStamp1;
					psKPI_prev->t_cpu_remained =
						16666666; /* fixed value since vsync period unchange */
					psKPI_prev->t_cpu_remained -=
						(long long)(psKPI_prev->ullTimeStamp1 - psHead->last_TimeStampS);
					psHead->last_TimeStampS =
						psKPI_prev->ullTimeStampS;
#ifdef GED_KPI_DEBUG
					if (psTimeStamp->i32FrameID != psKPI_prev->i32FrameID)
						GED_LOGE(
							"[GED_KPI][Exception]: i32FrameID: (%d, %d)\n",
							psTimeStamp->i32FrameID, psKPI_prev->i32FrameID);
#endif
					break;
				}
			}
			psKPI_prev = psKPI;
		}
		if (psKPI && psKPI == psKPI_prev) {
			/* (0 == (psKPI->ulMask & GED_TIMESTAMP_TYPE_S) */
			psKPI->ulMask |= GED_TIMESTAMP_TYPE_S;
			psKPI->ullTimeStampS = psTimeStamp->ullTimeStamp;
			psHead->t_cpu_remained = psTimeStamp->ullTimeStamp - psKPI->ullTimeStamp1;
			psKPI->t_cpu_remained = 16666666; /* fixed value since vsync period unchange */
			psKPI->t_cpu_remained -= (long long)(psKPI->ullTimeStamp1 - psHead->last_TimeStampS);
			psHead->last_TimeStampS = psTimeStamp->ullTimeStamp;
#ifdef GED_KPI_DEBUG
			if (psTimeStamp->i32FrameID != psKPI->i32FrameID)
				GED_LOGE("[GED_KPI][Exception]: i32FrameID: (%d, %d)\n",
				psTimeStamp->i32FrameID, psKPI->i32FrameID);
#endif
		}
		if (!psKPI)
			return GED_FALSE;
		else
			return GED_TRUE;
	} else
		return GED_FALSE;
}
/* ----------------------------------------------------------------------------- */
static GED_BOOL ged_kpi_h_iterator_func(unsigned long ulID, void *pvoid, void *pvParam)
{
	GED_KPI_HEAD *psHead = (GED_KPI_HEAD *)pvoid;
	GED_TIMESTAMP *psTimeStamp = (GED_TIMESTAMP *)pvParam;
	GED_KPI *psKPI = NULL;
	GED_KPI *psKPI_prev = NULL;

	if (psHead) {
		struct list_head *psListEntry, *psListEntryTemp;
		struct list_head *psList = &psHead->sList;

		list_for_each_prev_safe(psListEntry, psListEntryTemp, psList) {
			psKPI = list_entry(psListEntry, GED_KPI, sList);
			if (psKPI) {
				if (psKPI->ulMask & GED_TIMESTAMP_TYPE_H) {
					if (psKPI_prev && (psKPI_prev->ulMask & GED_TIMESTAMP_TYPE_S)
						&& (psKPI_prev->ulMask & GED_TIMESTAMP_TYPE_2)) {
						psKPI_prev->ulMask |= GED_TIMESTAMP_TYPE_H;
						psKPI_prev->ullTimeStampH = psTimeStamp->ullTimeStamp;

						/* Not yet precise due uncertain type_H ts*/
						psHead->t_gpu_remained =
							psTimeStamp->ullTimeStamp - psKPI_prev->ullTimeStamp2;
						psKPI_prev->t_gpu_remained =
							16666666; /* fixed value since vsync period unchange */
						psKPI_prev->t_gpu_remained -=
						(long long)(psKPI_prev->ullTimeStamp2 - psHead->last_TimeStampH);

						psHead->last_TimeStampH = psTimeStamp->ullTimeStamp;
						ged_kpi_statistics_and_remove(psHead, psKPI_prev);
					}
					break;
				}
			}
			psKPI_prev = psKPI;
		}
		if (psKPI && psKPI == psKPI_prev) {
			/* (0 == (psKPI->ulMask & GED_TIMESTAMP_TYPE_H) */
			if ((psKPI->ulMask & GED_TIMESTAMP_TYPE_S)
				&& (psKPI->ulMask & GED_TIMESTAMP_TYPE_2)) {
				psKPI->ulMask |= GED_TIMESTAMP_TYPE_H;
				psKPI->ullTimeStampH = psTimeStamp->ullTimeStamp;

				/* Not yet precise due uncertain type_H ts*/
				psHead->t_gpu_remained = psTimeStamp->ullTimeStamp - psKPI->ullTimeStamp2;
				psKPI->t_gpu_remained = 16666666; /* fixed value since vsync period unchange */
				psKPI->t_gpu_remained -=
					(long long)(psKPI->ullTimeStamp2 - psHead->last_TimeStampH);

				psHead->last_TimeStampH = psTimeStamp->ullTimeStamp;
				ged_kpi_statistics_and_remove(psHead, psKPI);
			}
		}
	}
	return GED_TRUE;
}
/* ----------------------------------------------------------------------------- */
static GED_BOOL ged_kpi_iterator_delete_func(unsigned long ulID, void *pvoid, void *pvParam, GED_BOOL *pbDeleted)
{
	GED_KPI_HEAD *psHead = (GED_KPI_HEAD *)pvoid;

	if (psHead) {
		ged_free(psHead, sizeof(GED_KPI_HEAD));
		*pbDeleted = GED_TRUE;
	}

	return GED_TRUE;
}
static void ged_kpi_update_target_time_and_target_fps(void)
{
	gx_perf_state.t_cpu_target = 1000000000/gx_dfps;
	gx_perf_state.t_gpu_target = gx_perf_state.t_cpu_target;
}
/* ----------------------------------------------------------------------------- */
void ged_kpi_get_latest_perf_state(long long *t_cpu_remained,
									long long *t_gpu_remained,
									long *t_cpu_target,
									long *t_gpu_target)
{
	if (t_cpu_remained != NULL && main_head != NULL && !(main_head->t_cpu_remained < (-1)*SCREEN_IDLE_PERIOD))
		*t_cpu_remained = main_head->t_cpu_remained;
	else
		*t_cpu_remained = 0;

	if (t_gpu_remained != NULL && main_head != NULL && !(main_head->t_gpu_remained < (-1)*SCREEN_IDLE_PERIOD))
		*t_gpu_remained = main_head->t_gpu_remained;
	else
		*t_gpu_remained = 0;

	if (t_cpu_target != NULL)
		*t_cpu_target = gx_perf_state.t_cpu_target;
	if (t_gpu_target != NULL)
		*t_gpu_target = gx_perf_state.t_gpu_target;
}
/* ----------------------------------------------------------------------------- */
void ged_kpi_set_target_fps(unsigned int target_fps)
{
	gx_dfps = target_fps;
	ged_kpi_update_target_time_and_target_fps();
}
/* ----------------------------------------------------------------------------- */
static void ged_kpi_work_cb(struct work_struct *psWork)
{
	GED_TIMESTAMP *psTimeStamp = GED_CONTAINER_OF(psWork, GED_TIMESTAMP, sWork);
	GED_KPI_HEAD *psHead;
	GED_KPI *psKPI;
	unsigned long ulID;

#ifdef GED_KPI_DEBUG
	GED_LOGE("[GED_KPI] ts type = %d, pid = %d, wnd = %llu, frame = %d\n",
		psTimeStamp->eTimeStampType, psTimeStamp->pid, psTimeStamp->ullWnd, psTimeStamp->i32FrameID);
#endif

	switch (psTimeStamp->eTimeStampType) {
	case GED_TIMESTAMP_TYPE_1:
		psKPI = &g_asKPI[g_i32Pos++];
		if (g_i32Pos >= GED_KPI_TOTAL_ITEMS)
			g_i32Pos = 0;

		/* remove */
		ulID = (unsigned long) psKPI->ullWnd;
		psHead = (GED_KPI_HEAD *)ged_hashtable_find(gs_hashtable, ulID);
		if (psHead) {
			psHead->i32Count -= 1;
			list_del(&psKPI->sList);

			if (psHead->i32Count < 1) {
				ged_hashtable_remove(gs_hashtable, ulID);
				ged_free(psHead, sizeof(GED_KPI_HEAD));
			}
		}
#ifdef GED_KPI_DEBUG
		else
			GED_LOGE("[GED_KPI][Exception] no hashtable head for ulID: %lu\n", ulID);
#endif

		/* reset data */
		memset(psKPI, 0, sizeof(GED_KPI));
		INIT_LIST_HEAD(&psKPI->sList);

		/* add */
		ulID = (unsigned long) psTimeStamp->ullWnd;
		psHead = (GED_KPI_HEAD *)ged_hashtable_find(gs_hashtable, ulID);
		if (!psHead) {
			psHead = (GED_KPI_HEAD *)ged_alloc_atomic(sizeof(GED_KPI_HEAD));
			if (psHead) {
				psHead->pid = psTimeStamp->pid;
				psHead->ullWnd = psTimeStamp->ullWnd;
				psHead->i32Count = 0;
				psHead->isSF = psTimeStamp->isSF;
				psHead->i32Gpu_uncompleted = 0;
				INIT_LIST_HEAD(&psHead->sList);
				ged_hashtable_set(gs_hashtable, ulID, (void *)psHead);
			}
		}
		if (psHead) {
			/* new data */
			psKPI->pid = psTimeStamp->pid;
			psKPI->ullWnd = psTimeStamp->ullWnd;
			psKPI->i32FrameID = psTimeStamp->i32FrameID;
			psKPI->ulMask = GED_TIMESTAMP_TYPE_1;
			psKPI->ullTimeStamp1 = psTimeStamp->ullTimeStamp;
			psHead->i32QedBuffer_length = psTimeStamp->i32QedBuffer_length;
			list_add_tail(&psKPI->sList, &psHead->sList);
			psHead->i32Count += 1;
			psHead->i32Gpu_uncompleted += 1;
			/* recording cpu time per frame and boost CPU if needed */
			psHead->t_cpu_latest = psKPI->ullTimeStamp1 - psHead->last_TimeStamp1;
			psHead->last_TimeStamp1 = psKPI->ullTimeStamp1;

#ifdef GED_KPI_DEBUG
			if (psHead->isSF != psTimeStamp->isSF)
				GED_LOGE("[GED_KPI][Exception] psHead->isSF != psTimeStamp->isSF\n");
			if (psHead->pid != psTimeStamp->pid) {
				GED_LOGE("[GED_KPI][Exception] psHead->pid != psTimeStamp->pid: (%d, %d)\n",
					psHead->pid, psTimeStamp->pid);
			}
			GED_LOGE("[GED_KPI] TimeStamp1, i32QedBuffer_length:%d, ts: %llu, psHead: %p\n",
				psTimeStamp->i32QedBuffer_length, psTimeStamp->ullTimeStamp, psHead);
#endif
		}
#ifdef GED_KPI_DEBUG
		else
			GED_LOGE("[GED_KPI][Exception] no hashtable head for ulID: %lu\n", ulID);
#endif
		break;
	case GED_TIMESTAMP_TYPE_2:
		ulID = (unsigned long) psTimeStamp->ullWnd;
		psHead = (GED_KPI_HEAD *)ged_hashtable_find(gs_hashtable, ulID);
		if (psHead) {
			struct list_head *psListEntry, *psListEntryTemp;
			struct list_head *psList = &psHead->sList;

			list_for_each_prev_safe(psListEntry, psListEntryTemp, psList) {
				psKPI = list_entry(psListEntry, GED_KPI, sList);
				if (psKPI) {
					if (psKPI->i32FrameID == psTimeStamp->i32FrameID) {
						if ((psKPI->ulMask & GED_TIMESTAMP_TYPE_2) == 0) {
							psKPI->ulMask |= GED_TIMESTAMP_TYPE_2;
							psKPI->ullTimeStamp2 = psTimeStamp->ullTimeStamp;

							/* calculate gpu time */
							if (psKPI->ullTimeStamp1 > psHead->last_TimeStamp2)
								psHead->t_gpu_latest = psKPI->ullTimeStamp2 - psKPI->ullTimeStamp1;
							else
								psHead->t_gpu_latest = psKPI->ullTimeStamp2 - psHead->last_TimeStamp2;
#ifdef GED_DVFS_ENABLE
							psKPI->gpu_freq = mt_gpufreq_get_cur_freq();
#else
							psKPI->gpu_freq = 0;
#endif
							psHead->last_TimeStamp2 = psTimeStamp->ullTimeStamp;
							psHead->i32Gpu_uncompleted -= 1;
							mtk_get_gpu_loading(&psKPI->gpu_loading);

							/* notify gpu performance info to GPU DVFS module */
							ged_kpi_set_gpu_dvfs_hint((int)(gx_perf_state.t_gpu_target/1000000), (int)(psHead->t_gpu_latest)/1000000);
						}
#ifdef GED_KPI_DEBUG
						else {
							GED_LOGE(
								"[GED_KPI][Exception] frame(%d) taged again to ulID: %lu\n",
								psTimeStamp->i32FrameID,
								ulID);
						}
						GED_LOGE(
							"[GED_KPI] timestamp matched, Type_2: psHead: %p, frame: %d\n",
							psHead, psTimeStamp->i32FrameID);
#endif
						break;
					}
				}
			}
		}
#ifdef GED_KPI_DEBUG
		else
			GED_LOGE("[GED_KPI][Exception] no hashtable head for ulID: %lu\n", ulID);
#endif
		break;
	case GED_TIMESTAMP_TYPE_S:
		ulID = (unsigned long) psTimeStamp->ullWnd;
		psHead = (GED_KPI_HEAD *)ged_hashtable_find(gs_hashtable, ulID);

#ifdef GED_KPI_DEBUG
		if (!psHead)
			GED_LOGE("[GED_KPI][Exception] TYPE_S: no hashtable head for ulID: %lu\n", ulID);
#endif

#ifdef GED_KPI_DEBUG
		if (ged_kpi_tag_type_s(ulID, psHead, psTimeStamp) != GED_TRUE)
			GED_LOGE("[GED_KPI][Exception] TYPE_S timestamp miss, ulID: %lu\n", ulID);
		else
			GED_LOGE("[GED_KPI] timestamp matched, Type_S: psHead: %p\n", psHead);
#else
		ged_kpi_tag_type_s(ulID, psHead, psTimeStamp);
#endif

		if ((main_head != NULL && gx_game_mode == 1) && (main_head->t_cpu_remained > (-1)*SCREEN_IDLE_PERIOD)) {
#ifdef GED_KPI_DEBUG
			GED_LOGE("[GED_KPI] t_cpu_remained: %lld, main_head: %p\n", main_head->t_cpu_remained, main_head);
#endif
			ged_kpi_set_cpu_remained_time(main_head->t_cpu_remained, main_head->i32QedBuffer_length);
		}
		break;
	case GED_TIMESTAMP_TYPE_H:
		ged_hashtable_iterator(gs_hashtable, ged_kpi_h_iterator_func, (void *)psTimeStamp);
		ged_kpi_update_target_time_and_target_fps();
#ifdef GED_KPI_DEBUG
		{
			long long t_cpu_remained, t_gpu_remained;
			long t_cpu_target, t_gpu_target;

			ged_kpi_get_latest_perf_state(&t_cpu_remained, &t_gpu_remained, &t_cpu_target, &t_gpu_target);
			GED_LOGE("[GED_KPI] t_cpu: %ld, %lld, t_gpu: %ld, %lld\n",
						t_cpu_target,
						t_cpu_remained,
						t_gpu_target,
						t_gpu_remained);
		}
#endif
		break;
	default:
		break;
	}

	ged_free(psTimeStamp, sizeof(GED_TIMESTAMP));
}
/* ----------------------------------------------------------------------------- */
static GED_ERROR ged_kpi_push_timestamp(
	GED_TIMESTAMP_TYPE eTimeStampType,
	unsigned long long ullTimeStamp,
	int pid,
	unsigned long long ullWnd,
	int i32FrameID,
	int QedBuffer_length,
	int isSF)
{
	if (g_psWorkQueue) {
		GED_TIMESTAMP *psTimeStamp = (GED_TIMESTAMP *)ged_alloc_atomic(sizeof(GED_TIMESTAMP));

		if (!psTimeStamp)
			return GED_ERROR_OOM;

		psTimeStamp->eTimeStampType = eTimeStampType;
		psTimeStamp->ullTimeStamp = ullTimeStamp;
		psTimeStamp->pid = pid;
		psTimeStamp->ullWnd = ullWnd;
		psTimeStamp->i32FrameID = i32FrameID;
		psTimeStamp->i32QedBuffer_length = QedBuffer_length;
		psTimeStamp->isSF = isSF;
		INIT_WORK(&psTimeStamp->sWork, ged_kpi_work_cb);

		queue_work(g_psWorkQueue, &psTimeStamp->sWork);
	}
#ifdef GED_KPI_DEBUG
	else {
		GED_LOGE("[GED_KPI][Exception]: g_psWorkQueue: NULL\n");
		return GED_ERROR_FAIL;
	}
#endif
	return GED_OK;
}
/* ----------------------------------------------------------------------------- */
static GED_ERROR ged_kpi_time1(int pid, unsigned long long ullWdnd, int i32FrameID, int QedBuffer_length, int isSF)
{
	return ged_kpi_push_timestamp(GED_TIMESTAMP_TYPE_1, ged_get_time(), pid,
								ullWdnd, i32FrameID, QedBuffer_length, isSF);
}
/* ----------------------------------------------------------------------------- */
static GED_ERROR ged_kpi_time2(int pid, unsigned long long ullWdnd, int i32FrameID)
{
	return ged_kpi_push_timestamp(GED_TIMESTAMP_TYPE_2, ged_get_time(), pid,
								ullWdnd, i32FrameID, -1, -1);
}
/* ----------------------------------------------------------------------------- */
static GED_ERROR ged_kpi_timeS(int pid, unsigned long long ullWdnd, int i32FrameID)
{
	return ged_kpi_push_timestamp(GED_TIMESTAMP_TYPE_S, ged_get_time(), pid, ullWdnd, i32FrameID, -1, -1);
}
/* ----------------------------------------------------------------------------- */
static void ged_kpi_gpu_3d_fence_sync_cb(struct sync_fence *fence, struct sync_fence_waiter *waiter)
{
	GED_KPI_GPU_TS *psMonitor;

	psMonitor = GED_CONTAINER_OF(waiter, GED_KPI_GPU_TS, sSyncWaiter);

	ged_kpi_time2(psMonitor->pid, psMonitor->ullWdnd, psMonitor->i32FrameID);

	sync_fence_put(psMonitor->psSyncFence);
	ged_free(psMonitor, sizeof(GED_KPI_GPU_TS));
}
/* ----------------------------------------------------------------------------- */
static void ged_kpi_wait_for_hw_vsync_cb(void *data)
{
	while (1) {
		dpmgr_wait_event(primary_get_dpmgr_handle(), DISP_PATH_EVENT_IF_VSYNC);
		ged_notification(GED_NOTIFICATION_TYPE_HW_VSYNC_PRIMARY_DISPLAY);
	}
}
#endif
/* ----------------------------------------------------------------------------- */
GED_ERROR ged_kpi_acquire_buffer_ts(int pid, unsigned long long ullWdnd, int i32FrameID)
{
#ifdef MTK_GED_KPI
	GED_ERROR ret;

	ret = ged_kpi_timeS(pid, ullWdnd, i32FrameID);
	return ret;
#else
	return GED_OK;
#endif
}
/* ----------------------------------------------------------------------------- */
GED_ERROR ged_kpi_queue_buffer_ts(int pid, unsigned long long ullWdnd, int i32FrameID, int fence_fd, int QedBuffer_length, int isSF)
{
#ifdef MTK_GED_KPI
	GED_ERROR ret;
	GED_KPI_GPU_TS *psMonitor;


	ret = ged_kpi_time1(pid, ullWdnd, i32FrameID, QedBuffer_length, isSF);
	if (ret != GED_OK)
		return ret;

	psMonitor = (GED_KPI_GPU_TS *)ged_alloc(sizeof(GED_KPI_GPU_TS));

	if (!psMonitor)
		return GED_ERROR_OOM;

	sync_fence_waiter_init(&psMonitor->sSyncWaiter, ged_kpi_gpu_3d_fence_sync_cb);
	psMonitor->psSyncFence = sync_fence_fdget(fence_fd);
	psMonitor->pid = pid;
	psMonitor->ullWdnd = ullWdnd;
	psMonitor->i32FrameID = i32FrameID;

	if (psMonitor->psSyncFence == NULL) {
		ged_free(psMonitor, sizeof(GED_KPI_GPU_TS));
		return GED_ERROR_INVALID_PARAMS;
	}

	ret = sync_fence_wait_async(psMonitor->psSyncFence, &psMonitor->sSyncWaiter);

	if ((ret == 1) || (ret < 0)) {
		sync_fence_put(psMonitor->psSyncFence);
		ged_free(psMonitor, sizeof(GED_KPI_GPU_TS));
		ret = ged_kpi_time2(pid, ullWdnd, i32FrameID);
	}
	return ret;
#else
	return GED_OK;
#endif
}
/* ----------------------------------------------------------------------------- */
GED_ERROR ged_kpi_sw_vsync(void)
{
#ifdef MTK_GED_KPI
	return GED_OK; /* disabled function*/
#else
	return GED_OK;
#endif
}
/* ----------------------------------------------------------------------------- */
GED_ERROR ged_kpi_hw_vsync(void)
{
#ifdef MTK_GED_KPI
	return ged_kpi_push_timestamp(GED_TIMESTAMP_TYPE_H, ged_get_time(), 0, 0, 0, 0, 0);
#else
	return GED_OK;
#endif
}
/* ----------------------------------------------------------------------------- */
int ged_kpi_get_uncompleted_count(void)
{
#ifdef MTK_GED_KPI
	/* return gx_i32UncompletedCount; */
	return 0; /* disabled function */
#else
	return 0;
#endif
}
/* ----------------------------------------------------------------------------- */
unsigned int ged_kpi_get_cur_fps(void)
{
#ifdef MTK_GED_KPI
	return gx_fps;
#else
	return 0;
#endif
}
/* ----------------------------------------------------------------------------- */
unsigned int ged_kpi_get_cur_avg_cpu_time(void)
{
#ifdef MTK_GED_KPI
	return gx_cpu_time_avg;
#else
	return 0;
#endif
}
/* ----------------------------------------------------------------------------- */
unsigned int ged_kpi_get_cur_avg_gpu_time(void)
{
#ifdef MTK_GED_KPI
	return gx_gpu_time_avg;
#else
	return 0;
#endif
}
/* ----------------------------------------------------------------------------- */
unsigned int ged_kpi_get_cur_avg_response_time(void)
{
#ifdef MTK_GED_KPI
	return gx_response_time_avg;
#else
	return 0;
#endif
}
/* ----------------------------------------------------------------------------- */
unsigned int ged_kpi_get_cur_avg_gpu_remained_time(void)
{
#ifdef MTK_GED_KPI
	return gx_gpu_remained_time_avg;
#else
	return 0;
#endif
}
/* ----------------------------------------------------------------------------- */
unsigned int ged_kpi_get_cur_avg_cpu_remained_time(void)
{
#ifdef MTK_GED_KPI
	return gx_cpu_remained_time_avg;
#else
	return 0;
#endif
}
/* ----------------------------------------------------------------------------- */
unsigned int ged_kpi_get_cur_avg_gpu_freq(void)
{
#ifdef MTK_GED_KPI
	return gx_gpu_freq_avg;
#else
	return 0;
#endif
}
/* ----------------------------------------------------------------------------- */
GED_ERROR ged_kpi_system_init(void)
{
#ifdef MTK_GED_KPI
	GED_ERROR ret;

	ghLogBuf = ged_log_buf_alloc(60 * 10, 100 * 60 * 10, GED_LOG_BUF_TYPE_RINGBUFFER, NULL, "KPI");

	ret = ged_thread_create(&ghThread, "kpi_wait_4_hw_vsync", ged_kpi_wait_for_hw_vsync_cb, (void *)&ghLogBuf);
	if (ret != GED_OK)
		return ret;

	g_psWorkQueue = alloc_ordered_workqueue("ged_kpi", WQ_FREEZABLE | WQ_MEM_RECLAIM);
	if (g_psWorkQueue) {
		memset(g_asKPI, 0, sizeof(g_asKPI));
		gs_hashtable = ged_hashtable_create(10);
		return gs_hashtable ? GED_OK : GED_ERROR_FAIL;
	}
	return GED_ERROR_FAIL;
#else
	return GED_OK;
#endif
}
/* ----------------------------------------------------------------------------- */
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
/* ----------------------------------------------------------------------------- */
void (*ged_kpi_set_gpu_dvfs_hint_fp)(int t_gpu_target, int t_gpu_cur);
EXPORT_SYMBOL(ged_kpi_set_gpu_dvfs_hint_fp);

bool ged_kpi_set_gpu_dvfs_hint(int t_gpu_target, int t_gpu_cur)
{
	if (ged_kpi_set_gpu_dvfs_hint_fp != NULL) {
		ged_kpi_set_gpu_dvfs_hint_fp(t_gpu_target, t_gpu_cur);
		return true;
	}
	return false;
}
/* ----------------------------------------------------------------------------- */
void (*ged_kpi_set_cpu_remained_time_fp)(long long t_cpu_remained, int QedBuffer_length);
EXPORT_SYMBOL(ged_kpi_set_cpu_remained_time_fp);

bool ged_kpi_set_cpu_remained_time(long long t_cpu_remained, int QedBuffer_length)
{
	if (ged_kpi_set_cpu_remained_time_fp != NULL) {
		ged_kpi_set_cpu_remained_time_fp(t_cpu_remained, QedBuffer_length);
		return true;
	}
	return false;
}
/* ----------------------------------------------------------------------------- */
void (*ged_kpi_set_game_hint_value_fp)(int is_game_mode);
EXPORT_SYMBOL(ged_kpi_set_game_hint_value_fp);

bool ged_kpi_set_game_hint_value(int is_game_mode)
{
	if (ged_kpi_set_game_hint_value_fp != NULL) {
		ged_kpi_set_game_hint_value_fp(is_game_mode);
		return true;
	}
	return false;
}
/* ----------------------------------------------------------------------------- */
void ged_kpi_set_game_hint(int mode)
{
	if (mode == 1 || mode == 0) {
		gx_game_mode = mode;
		ged_kpi_set_game_hint_value(mode);
	}
}
