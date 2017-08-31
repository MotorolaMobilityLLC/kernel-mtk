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
#ifdef MTK_GED_KPI
#include <primary_display.h>
#include <mt-plat/mtk_gpu_utility.h>
/* #ifdef GED_DVFS_ENABLE */
#include <mtk_gpufreq.h>
/* #endif */
#endif

#ifdef GED_KPI_MET_DEBUG
#include <mt-plat/met_drv.h>
#endif

#ifdef GED_KPI_DFRC
#include <dfrc.h>
#include <dfrc_drv.h>
#include <ged_frr.h>
#endif

#if (KERNEL_VERSION(3, 10, 0) > LINUX_VERSION_CODE)
#include <linux/sync.h>
#else
#include <../drivers/staging/android/sync.h>
#endif

#ifdef MTK_GED_KPI

#ifdef GED_KPI_DEBUG
#undef GED_LOGE
#define GED_LOGE printk
#endif

#define GED_KPI_MSEC_DIVIDER 1000000
#define GED_KPI_SEC_DIVIDER 1000000000
#define GED_KPI_MAX_FPS 60

typedef enum {
	GED_TIMESTAMP_TYPE_1		= 0x1,
	GED_TIMESTAMP_TYPE_2		= 0x2,
	GED_TIMESTAMP_TYPE_S		= 0x4,
	GED_TIMESTAMP_TYPE_H		= 0x8,
} GED_TIMESTAMP_TYPE;

#ifdef GED_KPI_DFRC
typedef enum {
	GED_KPI_FRC_DEFAULT_MODE		= DFRC_DRV_MODE_DEFAULT,	/* No frame control is applied */
	GED_KPI_FRC_FRR_MODE			= DFRC_DRV_MODE_FRR,
	GED_KPI_FRC_ARR_MODE			= DFRC_DRV_MODE_ARR,
	GED_KPI_FRC_SW_VSYNC_MODE		= DFRC_DRV_MODE_INTERNAL_SW,
} GED_KPI_FRC_MODE_TYPE;
#else
typedef enum {
	GED_KPI_FRC_DEFAULT_MODE		= 0,	/* No frame control is applied */
	GED_KPI_FRC_FRR_MODE			= 1,
	GED_KPI_FRC_ARR_MODE			= 2,
	GED_KPI_FRC_SW_VSYNC_MODE		= 3,
} GED_KPI_FRC_MODE_TYPE;
#endif

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
	long long t_cpu_latest;
	long long t_gpu_latest;
	struct list_head sList;
	int i32QedBuffer_length;
	int i32Gpu_uncompleted;
	int i32DebugQedBuffer_length;
	int isSF;
	int isFRR_enabled;
	int isARR_enabled;
	int target_fps;
	int t_cpu_target;
	int t_gpu_target;
	GED_KPI_FRC_MODE_TYPE frc_mode;
	int frc_client;
	unsigned long long last_QedBufferDelay;
} GED_KPI_HEAD;

typedef struct GED_KPI_TAG {
	int pid;
	unsigned long long ullWnd;
	unsigned long i32QueueID;
	unsigned long  i32AcquireID;
	unsigned long ulMask;
	unsigned long frame_attr;
	unsigned long long ullTimeStamp1;
	unsigned long long ullTimeStamp2;
	unsigned long long ullTimeStampS;
	unsigned long long ullTimeStampH;
	unsigned int gpu_freq; /* in MHz*/
	unsigned int gpu_loading;
	struct list_head sList;
	long long t_cpu_remained;
	long long t_gpu_remained;
	int i32QedBuffer_length;
	int i32Gpu_uncompleted;
	int i32DebugQedBuffer_length;
	int boost_linear_cpu;
	int boost_real_cpu;
	int boost_accum_cpu;
	int boost_accum_gpu;
	long long t_cpu_remained_pred;
	unsigned long long t_acquire_period;
	unsigned long long QedBufferDelay;
	unsigned long cpu_max_freq_LL;
	unsigned long cpu_max_freq_L;
	unsigned long cpu_max_freq_B;
	unsigned long cpu_cur_freq_LL;
	unsigned long cpu_cur_freq_L;
	unsigned long cpu_cur_freq_B;
	long long t_cpu_latest;
	int t_cpu_target;
	int t_gpu_target;
	int if_fallback_to_ft;
} GED_KPI;

typedef struct GED_TIMESTAMP_TAG {
	GED_TIMESTAMP_TYPE eTimeStampType;
	int pid;
	unsigned long long ullWnd;
	unsigned long i32FrameID;
	unsigned long long ullTimeStamp;
	int i32QedBuffer_length;
	int isSF;
	struct work_struct sWork;
	void *fence_addr;
} GED_TIMESTAMP;

typedef struct GED_KPI_GPU_TS_TAG {
	int pid;
	unsigned long long ullWdnd;
	unsigned long i32FrameID;
	struct sync_fence_waiter sSyncWaiter;
	struct sync_fence *psSyncFence;
} GED_KPI_GPU_TS;







#define GED_KPI_TOTAL_ITEMS 64
#define GED_KPI_UID(pid, wnd) (pid | ((unsigned long)wnd))
#define SCREEN_IDLE_PERIOD 500000000

/* static int display_fps = GED_KPI_MAX_FPS; */
static int is_game_control_frame_rate;
static int target_fps_4_main_head = 60;
static long long vsync_period = GED_KPI_SEC_DIVIDER / GED_KPI_MAX_FPS;
static GED_LOG_BUF_HANDLE ghLogBuf;
static struct workqueue_struct *g_psWorkQueue;
static GED_HASHTABLE_HANDLE gs_hashtable;
static GED_KPI g_asKPI[GED_KPI_TOTAL_ITEMS];
static int g_i32Pos;
static GED_THREAD_HANDLE ghThread;
static unsigned int gx_dfps; /* variable to fix FPS*/
static unsigned int gx_frc_mode; /* variable to fix FRC mode*/
#ifdef GED_KPI_CPU_BOOST
static unsigned int enable_cpu_boost = 1;
#endif
static unsigned int enable_gpu_boost = 1;
static unsigned int is_GED_KPI_enabled = 1;
static unsigned int ap_self_frc_detection_rate = 20;
module_param(gx_dfps, uint, S_IRUGO|S_IWUSR);
module_param(gx_frc_mode, uint, S_IRUGO|S_IWUSR);
#ifdef GED_KPI_CPU_BOOST
module_param(enable_cpu_boost, uint, S_IRUGO|S_IWUSR);
#endif
module_param(enable_gpu_boost, uint, S_IRUGO|S_IWUSR);
module_param(is_GED_KPI_enabled, uint, S_IRUGO|S_IWUSR);
module_param(ap_self_frc_detection_rate, uint, S_IRUGO|S_IWUSR);
/* for calculating remained time budgets of CPU and GPU:
 *		time budget: the buffering time that prevents fram drop
 */
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
static int gx_3D_benchmark_on;
#ifdef GED_KPI_CPU_BOOST
static int gx_force_cpu_boost;
static int enable_game_self_frc_detect = 1;
#endif
static unsigned int gx_fps;
static unsigned int gx_cpu_time_avg;
static unsigned int gx_gpu_time_avg;
static unsigned int gx_response_time_avg;
static unsigned int gx_gpu_remained_time_avg;
static unsigned int gx_cpu_remained_time_avg;
static unsigned int gx_gpu_freq_avg;

static int boost_accum_gpu = 100;
#ifdef GED_KPI_CPU_BOOST
static int boost_accum_cpu;
static long target_t_cpu_remained = 16000000; /* for non-GED_KPI_MAX_FPS-FPS cases */
/* static long target_t_cpu_remained_min = 8300000; */ /* default 0.5 vsync period */
static int cpu_boost_policy;
static int boost_extra;
static int boost_amp;
static int deboost_reduce;
static int boost_upper_bound = 100;
static void (*ged_kpi_cpu_boost_policy_fp)(GED_KPI_HEAD *psHead, GED_KPI *psKPI);
module_param(target_t_cpu_remained, long, S_IRUGO|S_IWUSR);
module_param(gx_force_cpu_boost, int, S_IRUGO|S_IWUSR);
module_param(cpu_boost_policy, int, S_IRUGO|S_IWUSR);
module_param(boost_extra, int, S_IRUGO|S_IWUSR);
module_param(boost_amp, int, S_IRUGO|S_IWUSR);
module_param(deboost_reduce, int, S_IRUGO|S_IWUSR);
module_param(boost_upper_bound, int, S_IRUGO|S_IWUSR);
module_param(enable_game_self_frc_detect, int, S_IRUGO|S_IWUSR);
#endif
module_param(gx_game_mode, int, S_IRUGO|S_IWUSR);
module_param(gx_3D_benchmark_on, int, S_IRUGO|S_IWUSR);

#ifdef GED_KPI_CPU_BOOST
/* ----------------------------------------------------------------------------- */
/* detect if app self frc to make frame time 16 ms */
#define GED_KPI_FALLBACK_VOTE_NUM 30
static int if_fallback_to_ft;
static int num_fallback_vote;
static int recorded_fallback_vote;
static void ged_kpi_check_fallback_main_head_reset(void)
{
	num_fallback_vote = 0;
	recorded_fallback_vote = 0;
	if_fallback_to_ft = 0;
}
static inline void ged_kpi_check_if_fallback_is_needed(int boost_value, int t_cpu_latest)
{
	if (if_fallback_to_ft == 0) {
		if (boost_value > 30 && t_cpu_latest < 19000000)
			num_fallback_vote++;
		recorded_fallback_vote++;
		if (recorded_fallback_vote == GED_KPI_FALLBACK_VOTE_NUM) {
			if (num_fallback_vote * 100 / recorded_fallback_vote > 85)
				if_fallback_to_ft = 1;
			else
				ged_kpi_check_fallback_main_head_reset();
		}
	}
}
/* ----------------------------------------------------------------------------- */
#define GED_KPI_GAME_SELF_FRC_DETECT_MONITOR_WINDOW_SIZE 3
static int fps_records[GED_KPI_GAME_SELF_FRC_DETECT_MONITOR_WINDOW_SIZE];
static int cur_fps_idx;
static int fps_recorded_num = 1;
static int reset = 1;
static int afrc_rst_cnt_down;
static int afrc_rst_over_target_cnt;
static void ged_kpi_frc_detection_main_head_reset(int fps)
{
	cur_fps_idx = 0;
	fps_recorded_num = 1;
	target_fps_4_main_head = fps;
	afrc_rst_cnt_down = 0;
	afrc_rst_over_target_cnt = 0;
	is_game_control_frame_rate = 0;
	reset = 1;
}
static void ged_kpi_push_cur_fps_and_detect_app_self_frc(int fps)
{
	int fps_grp[GED_KPI_GAME_SELF_FRC_DETECT_MONITOR_WINDOW_SIZE];
	int i;

	if (enable_game_self_frc_detect && fps > 18 && fps <= 61) {
		fps_records[cur_fps_idx] = fps;
		if (reset == 0) {
			if (fps > target_fps_4_main_head + 1 || afrc_rst_cnt_down == 120) {
				ged_kpi_frc_detection_main_head_reset(60);
				GED_LOGE("[AFRC] reset: %d, %d, afrc_rst: %d, %d\n",
					fps, target_fps_4_main_head + 1, afrc_rst_cnt_down, afrc_rst_over_target_cnt);
			} else if (afrc_rst_over_target_cnt >= 15) {
				int is_fps_grp_aligned = 1;

				for (i = 0; i < GED_KPI_GAME_SELF_FRC_DETECT_MONITOR_WINDOW_SIZE; i++) {
					if (fps_records[i] <= 25)
						fps_grp[i] = 24;
					else if (fps_records[i] <= 28)
						fps_grp[i] = 27;
					else if (fps_records[i] <= 31)
						fps_grp[i] = 30;
					else if (fps_records[i] <= 45)
						fps_grp[i] = 45;
					else if (fps_records[i] <= 49)
						fps_grp[i] = 48;
					else
						fps_grp[i] = 60;
				}

				for (i = 0; i < GED_KPI_GAME_SELF_FRC_DETECT_MONITOR_WINDOW_SIZE - 1; i++) {
					if (fps_grp[i] != fps_grp[i + 1]) {
						is_fps_grp_aligned = 0;
						break;
					}
				}

				if (is_fps_grp_aligned) {
					target_fps_4_main_head = fps_grp[0];
					is_game_control_frame_rate = 1;
					afrc_rst_over_target_cnt = 0;
					GED_LOGE("[AFRC] retarget to %d FPS due to over target detection\n",
						fps_grp[0]);
				}

			} else {
				if (fps <= 25)
					fps = 24;
				else if (fps <= 28)
					fps = 27;
				else if (fps <= 31)
					fps = 30;
				else if (fps <= 45)
					fps = 45;
				else if (fps <= 49)
					fps = 48;
				else
					fps = 60;

				if (fps < target_fps_4_main_head)
					afrc_rst_over_target_cnt++;
				else
					afrc_rst_over_target_cnt = 0;
				afrc_rst_cnt_down++;
			}
		} else {
			if (fps_recorded_num == GED_KPI_GAME_SELF_FRC_DETECT_MONITOR_WINDOW_SIZE) {
				for (i = 0; i < GED_KPI_GAME_SELF_FRC_DETECT_MONITOR_WINDOW_SIZE; i++) {
					if (fps_records[i] <= 25)
						fps_grp[i] = 24;
					else if (fps_records[i] <= 28)
						fps_grp[i] = 27;
					else if (fps_records[i] <= 31)
						fps_grp[i] = 30;
					else if (fps_records[i] <= 45)
						fps_grp[i] = 45;
					else if (fps_records[i] <= 49)
						fps_grp[i] = 48;
					else
						fps_grp[i] = 60;
				}

				reset = 0;
				for (i = 0; i < GED_KPI_GAME_SELF_FRC_DETECT_MONITOR_WINDOW_SIZE - 1; i++) {
					if (fps_grp[i] != fps_grp[i + 1]) {
						reset = 1;
						break;
					}
				}

				GED_LOGE("[AFRC] fps_grp: %d, %d, %d\n", fps_grp[0], fps_grp[1], fps_grp[2]);

				if (reset == 0 && fps_grp[0] < 60) {
					target_fps_4_main_head = fps_grp[0];
					is_game_control_frame_rate = 1;
				} else {
					reset = 1;
				}
			} else {
				fps_recorded_num++;
			}
		}
		cur_fps_idx++;
		cur_fps_idx %= GED_KPI_GAME_SELF_FRC_DETECT_MONITOR_WINDOW_SIZE;
	} else {
		if (target_fps_4_main_head == 60 || enable_game_self_frc_detect == 0)
			is_game_control_frame_rate = 0;
	}
	/* debug AFRC detection */
	GED_LOGE("[AFRC]: cur_fps: %d, fps_records: %d, %d, %d, target_fps: %d\n",
		fps, fps_records[0], fps_records[1], fps_records[2], target_fps_4_main_head);
	/* end of debug AFRC detection */
}
#endif /* GED_KPI_CPU_BOOST */
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
void ged_kpi_main_head_reset(void)
{
#ifdef GED_KPI_CPU_BOOST
	ged_kpi_frc_detection_main_head_reset(60);
	ged_kpi_check_fallback_main_head_reset();
#endif
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
		ged_kpi_main_head_reset();
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

		if (g_elapsed_time_per_sec >= GED_KPI_SEC_DIVIDER) {
			unsigned long long g_fps;

			g_fps = g_frame_count;
			g_fps *= GED_KPI_SEC_DIVIDER;
			do_div(g_fps, g_elapsed_time_per_sec);
			gx_fps = g_fps;

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
#ifdef GED_KPI_CPU_BOOST
			ged_kpi_push_cur_fps_and_detect_app_self_frc((int)gx_fps);
#endif
		}
	}
}
/* ----------------------------------------------------------------------------- */
#define GED_KPI_IS_SF_SHIFT 0
#define GED_KPI_IS_SF_MASK 0x1
#define GED_KPI_QBUFFER_LEN_SHIFT 1
#define GED_KPI_QBUFFER_LEN_MASK 0x7
#define GED_KPI_DEBUG_QBUFFER_LEN_SHIFT 4
#define GED_KPI_DEBUG_QBUFFER_LEN_MASK 0x7
#define GED_KPI_GPU_UNCOMPLETED_LEN_SHIFT 7
#define GED_KPI_GPU_UNCOMPLETED_LEN_MASK 0x7
#define GED_KPI_FRC_MODE_SHIFT 10
#define GED_KPI_FRC_MODE_MASK 0x7
#define GED_KPI_FRC_CLIENT_SHIFT 13
#define GED_KPI_FRC_CLIENT_MASK 0xF
#define GED_KPI_GPU_FREQ_INFO_SHIFT 7
#define GED_KPI_GPU_FREQ_INFO_MASK 0xFFF /* max @ 4096 MHz */
#define GED_KPI_GPU_LOADING_INFO_SHIFT 0
#define GED_KPI_GPU_LOADING_INFO_MASK 0x7F
static void ged_kpi_statistics_and_remove(GED_KPI_HEAD *psHead, GED_KPI *psKPI)
{
	unsigned long ulID = (unsigned long) psKPI->ullWnd;
	unsigned long frame_attr = 0;
	unsigned long gpu_info = 0;

	ged_kpi_calc_kpi_info(ulID, psKPI, psHead);
	frame_attr |= ((psHead->isSF & GED_KPI_IS_SF_MASK) << GED_KPI_IS_SF_SHIFT);
	frame_attr |= ((psKPI->i32QedBuffer_length & GED_KPI_QBUFFER_LEN_MASK) << GED_KPI_QBUFFER_LEN_SHIFT);
	frame_attr |= ((psKPI->i32DebugQedBuffer_length & GED_KPI_DEBUG_QBUFFER_LEN_MASK)
					<< GED_KPI_DEBUG_QBUFFER_LEN_SHIFT);
	frame_attr |= ((psKPI->i32Gpu_uncompleted & GED_KPI_GPU_UNCOMPLETED_LEN_MASK)
					<< GED_KPI_GPU_UNCOMPLETED_LEN_SHIFT);
	frame_attr |= ((psHead->frc_mode & GED_KPI_FRC_MODE_MASK) << GED_KPI_FRC_MODE_SHIFT);
	frame_attr |= ((psHead->frc_client & GED_KPI_FRC_CLIENT_MASK) << GED_KPI_FRC_CLIENT_SHIFT);
	gpu_info |= ((psKPI->gpu_freq & GED_KPI_GPU_FREQ_INFO_MASK) << GED_KPI_GPU_FREQ_INFO_SHIFT);
	gpu_info |= ((psKPI->gpu_loading & GED_KPI_GPU_LOADING_INFO_MASK) << GED_KPI_GPU_LOADING_INFO_SHIFT);
	psKPI->frame_attr = frame_attr;

	/* statistics */
	ged_log_buf_print(ghLogBuf,
		"%d,%llu,%lu,%lu,%lu,%llu,%llu,%llu,%llu,%lu,%d,%d,%lld,%d,%lld,%llu,%lu,%lu,%lu,%lu,%lu,%lu",
		psHead->pid,
		psHead->ullWnd,
		psKPI->i32QueueID,
		psKPI->i32AcquireID,
		psKPI->frame_attr,
		psKPI->ullTimeStamp1,
		psKPI->ullTimeStamp2,
		psKPI->ullTimeStampS,
		psKPI->ullTimeStampH,
		gpu_info,
		psKPI->boost_accum_cpu,
		psKPI->boost_accum_gpu,
		psKPI->t_cpu_remained_pred,
		psKPI->t_cpu_target,
		vsync_period,
		psKPI->QedBufferDelay,
		psKPI->cpu_max_freq_LL,
		psKPI->cpu_max_freq_L,
		psKPI->cpu_max_freq_B,
		psKPI->cpu_cur_freq_LL,
		psKPI->cpu_cur_freq_L,
		psKPI->cpu_cur_freq_B
		);
}
#ifdef GED_KPI_CPU_BOOST
/* ----------------------------------------------------------------------------- */
static inline void ged_kpi_cpu_boost_policy_0(GED_KPI_HEAD *psHead, GED_KPI *psKPI)
{
	long long cpu_target_loss, cpu_target_loss_4_rt;
	int boost_real_cpu, boost_linear_cpu;
	long long t_cpu_cur, t_gpu_cur, t_cpu_target;
	long long t_cpu_rem_cur = 0;
	int is_gpu_bound;
	int temp_boost_accum_cpu;
	static int num_over_boost, num_monitored;

	if (psHead == main_head) {
		boost_linear_cpu = 0;
		boost_real_cpu = 0;

		t_cpu_cur = psHead->t_cpu_latest;
		t_gpu_cur = psHead->t_gpu_latest;

		if ((long long)psHead->t_cpu_target > t_gpu_cur) {
			t_cpu_target = (long long)psKPI->t_cpu_target;
			is_gpu_bound = 0;
		} else {
			/* when GPU bound, chase GPU frame time as target */
			t_cpu_target = t_gpu_cur + 2000000; /* 2 ms buffer*/
			is_gpu_bound = 1;
		}

		cpu_target_loss = t_cpu_cur - t_cpu_target;

		cpu_target_loss_4_rt = cpu_target_loss;
			/* ARR mode and default mode with GED_KPI_MAX_FPS FPS */
		if (psHead->target_fps == GED_KPI_MAX_FPS) {
			t_cpu_rem_cur = vsync_period * psHead->i32DebugQedBuffer_length;
			t_cpu_rem_cur -= (psKPI->ullTimeStamp1 - psHead->last_TimeStampS);
			if (cpu_target_loss_4_rt < 0 && t_cpu_rem_cur < target_t_cpu_remained && !if_fallback_to_ft) {
				cpu_target_loss_4_rt = 0;
			}
		} else {  /* FRR mode or (default mode && FPS != GED_KPI_MAX_FPS) */
			t_cpu_rem_cur = psKPI->t_cpu_target;
			t_cpu_rem_cur -= (psKPI->ullTimeStamp1 - (long long)psHead->last_TimeStampS);
		}
		psKPI->t_cpu_remained_pred = (long long)t_cpu_rem_cur;

		if (!is_gpu_bound)
			cpu_target_loss = cpu_target_loss_4_rt;

		boost_linear_cpu = (int)cpu_target_loss * 100 / (int)t_cpu_target;

		if (boost_linear_cpu < 0)
			boost_real_cpu = (-1)*linear_real_boost((-1)*boost_linear_cpu);
		else
			boost_real_cpu = linear_real_boost(boost_linear_cpu);

		if (boost_real_cpu >= 0)
			boost_real_cpu = boost_real_cpu * (100 + boost_amp) / 100;
		else
			boost_real_cpu = boost_real_cpu * (100 - deboost_reduce) / 100;

		if (boost_real_cpu != 0) {
			if (boost_accum_cpu <= 0) {
				boost_accum_cpu += boost_linear_cpu;
				if (boost_real_cpu > 0 && boost_accum_cpu > boost_real_cpu)
					boost_accum_cpu = boost_real_cpu;
			} else {
				int temp;

				temp = (100 + boost_real_cpu) * (100 + boost_accum_cpu) / 100 - 100;
				if (temp <= 0 && boost_real_cpu < 0)
					boost_accum_cpu = boost_accum_cpu >> 1;
				else
					boost_accum_cpu = temp;
			}
		}

		if (boost_accum_cpu > boost_upper_bound)
			boost_accum_cpu = boost_upper_bound;
		else if (boost_accum_cpu < 0)
			boost_accum_cpu = 0;

		temp_boost_accum_cpu = boost_accum_cpu;
		boost_accum_cpu += boost_extra;

		if (boost_accum_cpu > 100)
			boost_accum_cpu = 100;
		else if (boost_accum_cpu < 0)
			boost_accum_cpu = 0;

		update_eas_boost_value(EAS_KIR_FBC, CGROUP_TA, boost_accum_cpu);
		psKPI->boost_linear_cpu = boost_linear_cpu;
		psKPI->boost_real_cpu = boost_real_cpu;
		psKPI->boost_accum_cpu = boost_accum_cpu;
		psKPI->if_fallback_to_ft = if_fallback_to_ft;
		boost_accum_cpu = temp_boost_accum_cpu;
		psKPI->cpu_max_freq_LL = arch_scale_get_max_freq(0);
		psKPI->cpu_max_freq_L = arch_scale_get_max_freq(4);
#ifdef GED_KPI_CPU_TRI_CLUSTER
		psKPI->cpu_max_freq_B = arch_scale_get_max_freq(8);
#endif
		psKPI->cpu_cur_freq_LL = psKPI->cpu_max_freq_LL * cpufreq_scale_freq_capacity(NULL, 0) / 1024;
		psKPI->cpu_cur_freq_L = psKPI->cpu_max_freq_L * cpufreq_scale_freq_capacity(NULL, 4) / 1024;
#ifdef GED_KPI_CPU_TRI_CLUSTER
		psKPI->cpu_cur_freq_B = psKPI->cpu_max_freq_B * cpufreq_scale_freq_capacity(NULL, 8) / 1024;
#endif

		ged_kpi_check_if_fallback_is_needed(boost_accum_cpu, psKPI->t_cpu_latest);

		if (t_cpu_cur < (int)psHead->t_cpu_target + 2000000 && boost_accum_cpu > 30)
			num_over_boost++;
		num_monitored++;

		if (num_monitored == GED_KPI_FALLBACK_VOTE_NUM) {
			if (num_over_boost * 100 / num_monitored > 85)
				boost_accum_cpu = 0;
			num_over_boost = 0;
			num_monitored = 0;
		}

#ifdef GED_KPI_MET_DEBUG
		met_tag_oneshot(0, "ged_pframeb_boost_accum_cpu", boost_accum_cpu);

		if (boost_real_cpu < 100)
			met_tag_oneshot(0, "ged_pframeb_boost_real_cpu", boost_real_cpu);
		else
			met_tag_oneshot(0, "ged_pframeb_boost_real_cpu", 100);

		met_tag_oneshot(0, "ged_pframebc_boost_linear_cpu", boost_linear_cpu);
		met_tag_oneshot(0, "ged_pframebg_boost_accum_gpu", boost_accum_gpu);

		if (psHead->t_cpu_latest < 100*1000*1000)
			met_tag_oneshot(0, "ged_pframe_t_cpu",
				(long long)((int)psHead->t_cpu_latest + 999999)/GED_KPI_MSEC_DIVIDER);
		else
			met_tag_oneshot(0, "ged_pframe_t_cpu", 100);

		if (psHead->t_gpu_latest < 100*1000*1000)
			met_tag_oneshot(0, "ged_pframe_t_gpu",
				(long long)((int)psHead->t_gpu_latest + 999999)/GED_KPI_MSEC_DIVIDER);
		else
			met_tag_oneshot(0, "ged_pframe_t_gpu", 100);

		if (psKPI->QedBufferDelay < 100*1000*1000)
			met_tag_oneshot(0, "ged_pframe_QedBufferDelay"
				, ((int)psKPI->QedBufferDelay + 999999)/GED_KPI_MSEC_DIVIDER);
		else
			met_tag_oneshot(0, "ged_pframe_QedBufferDelay", 100);

		if (t_cpu_rem_cur < 100*1000*1000)
			met_tag_oneshot(0, "ged_pframe_t_cpu_remained_pred",
				((int)t_cpu_rem_cur + 999999)/GED_KPI_MSEC_DIVIDER);
		else
			met_tag_oneshot(0, "ged_pframe_t_cpu_remianed_pred", 100);

		met_tag_oneshot(0, "ged_pframe_t_cpu_target", ((int)t_cpu_target + 999999)/GED_KPI_MSEC_DIVIDER);
#endif
	}
}
/* ----------------------------------------------------------------------------- */
static inline void ged_kpi_cpu_boost(GED_KPI_HEAD *psHead, GED_KPI *psKPI)
{
	void (*cpu_boost_policy_fp)(GED_KPI_HEAD *psHead, GED_KPI *psKPI);

	cpu_boost_policy_fp = NULL;

	switch (cpu_boost_policy) {
	case 0:
		cpu_boost_policy_fp = ged_kpi_cpu_boost_policy_0;
		break;
	default:
		break;
	}

	if (ged_kpi_cpu_boost_policy_fp != cpu_boost_policy_fp) {
		ged_kpi_cpu_boost_policy_fp = cpu_boost_policy_fp;
		GED_LOGE("[GED_KPI] use cpu_boost_policy %d\n", cpu_boost_policy);
	}

	if (ged_kpi_cpu_boost_policy_fp != NULL)
		ged_kpi_cpu_boost_policy_fp(psHead, psKPI);
	else
		GED_LOGE("[GED_KPI] no such cpu_boost_policy %d\n", cpu_boost_policy);
}
#endif /* GED_KPI_CPU_BOOST */
/* ----------------------------------------------------------------------------- */
static GED_BOOL ged_kpi_tag_type_s(unsigned long ulID, GED_KPI_HEAD *psHead, GED_TIMESTAMP *psTimeStamp)
{
	GED_KPI *psKPI = NULL;
	GED_KPI *psKPI_prev = NULL;
	GED_BOOL ret = GED_FALSE;

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
					psKPI_prev->t_cpu_remained = vsync_period;
					psKPI_prev->t_cpu_remained -=
						(psKPI_prev->ullTimeStamp1 - psHead->last_TimeStampS);
					psKPI_prev->t_acquire_period =
						psTimeStamp->ullTimeStamp - psHead->last_TimeStampS;
					psHead->last_TimeStampS =
						psTimeStamp->ullTimeStamp;
					psKPI_prev->i32AcquireID = psTimeStamp->i32FrameID;
#ifdef GED_KPI_DEBUG
					if (psKPI_prev->i32AcquireID != psKPI_prev->i32QueueID)
						GED_LOGE(
							"[GED_KPI][Exception]: i32FrameID: (%lu, %lu)\n",
							psKPI_prev->i32QueueID, psKPI_prev->i32AcquireID);
#endif
					ret = GED_TRUE;
				}
				break;
			}
			psKPI_prev = psKPI;
		}
		if (psKPI && psKPI == psKPI_prev) {
			/* (0 == (psKPI->ulMask & GED_TIMESTAMP_TYPE_S) */
			psKPI->ulMask |= GED_TIMESTAMP_TYPE_S;
			psKPI->ullTimeStampS = psTimeStamp->ullTimeStamp;
			psHead->t_cpu_remained = psTimeStamp->ullTimeStamp - psKPI->ullTimeStamp1;
			psKPI->t_cpu_remained = vsync_period; /* fixed value since vsync period unchange */
			psKPI->t_cpu_remained -= (psKPI->ullTimeStamp1 - psHead->last_TimeStampS);
			psKPI->t_acquire_period = psTimeStamp->ullTimeStamp - psHead->last_TimeStampS;
			psHead->last_TimeStampS = psTimeStamp->ullTimeStamp;
#ifdef GED_KPI_DEBUG
			if (psTimeStamp->i32FrameID != psKPI->i32QueueID)
				GED_LOGE("[GED_KPI][Exception]: i32FrameID: (%lu, %lu)\n",
				psTimeStamp->i32FrameID, psKPI->i32QueueID);
#endif
			ret = GED_TRUE;
		}
		return ret;
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
							vsync_period; /* fixed value since vsync period unchange */
						psKPI_prev->t_gpu_remained -=
							(psKPI_prev->ullTimeStamp2 - psHead->last_TimeStampH);

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
				psKPI->t_gpu_remained = vsync_period; /* fixed value since vsync period unchange */
				psKPI->t_gpu_remained -=
					(psKPI->ullTimeStamp2 - psHead->last_TimeStampH);

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
static GED_BOOL ged_kpi_update_target_time_and_target_fps(GED_KPI_HEAD *psHead
											, int target_fps
											, GED_KPI_FRC_MODE_TYPE mode
											, int client)
{
	GED_BOOL ret = GED_FALSE;

	if (psHead) {
		switch (mode) {
		case GED_KPI_FRC_DEFAULT_MODE:
			psHead->frc_mode = GED_KPI_FRC_DEFAULT_MODE;
			vsync_period = GED_KPI_SEC_DIVIDER / GED_KPI_MAX_FPS;
			break;
		case GED_KPI_FRC_FRR_MODE:
			psHead->frc_mode = GED_KPI_FRC_FRR_MODE;
			vsync_period = GED_KPI_SEC_DIVIDER / GED_KPI_MAX_FPS;
			break;
		case GED_KPI_FRC_ARR_MODE:
			psHead->frc_mode = GED_KPI_FRC_ARR_MODE;
			vsync_period = GED_KPI_SEC_DIVIDER / target_fps;
			break;
		case GED_KPI_FRC_SW_VSYNC_MODE:
			psHead->frc_mode = GED_KPI_FRC_SW_VSYNC_MODE;
			vsync_period = GED_KPI_SEC_DIVIDER / target_fps;
			break;
		default:
			psHead->frc_mode = GED_KPI_FRC_DEFAULT_MODE;
			vsync_period = GED_KPI_SEC_DIVIDER / GED_KPI_MAX_FPS;
			GED_LOGE("[GED_KPI][Exception]: no invalid FRC mode is specified, use default mode\n");
		}

		if (is_game_control_frame_rate && (psHead == main_head)
			&& (psHead->frc_mode == GED_KPI_FRC_DEFAULT_MODE)
			&& (gx_3D_benchmark_on == 0))
			target_fps = target_fps_4_main_head;

		psHead->target_fps = target_fps;
		psHead->t_cpu_target = GED_KPI_SEC_DIVIDER/target_fps;
		psHead->t_gpu_target = psHead->t_cpu_target;
		psHead->frc_client = client;
		ret = GED_TRUE;
	}
	return ret;
}
/* ----------------------------------------------------------------------------- */
typedef struct ged_kpi_miss_tag {
	unsigned long ulID;
	unsigned long i32FrameID;
	GED_TIMESTAMP_TYPE eTimeStampType;
	struct list_head sList;
} GED_KPI_MISS_TAG;

static GED_KPI_MISS_TAG *miss_tag_head;

static void ged_kpi_record_miss_tag(unsigned long ulID, int i32FrameID, GED_TIMESTAMP_TYPE eTimeStampType)
{
	GED_KPI_MISS_TAG *psMiss_tag;

	if (unlikely(miss_tag_head == NULL)) {
		miss_tag_head = (GED_KPI_MISS_TAG *)ged_alloc_atomic(sizeof(GED_KPI_MISS_TAG));
		if (miss_tag_head) {
			memset(miss_tag_head, 0, sizeof(GED_KPI_MISS_TAG));
			INIT_LIST_HEAD(&miss_tag_head->sList);
		} else {
			GED_LOGE("[GED_KPI][Exception] ged_alloc_atomic(sizeof(GED_KPI_MISS_TAG)) failed\n");
			return;
		}
	}

	psMiss_tag = (GED_KPI_MISS_TAG *)ged_alloc_atomic(sizeof(GED_KPI_MISS_TAG));

	if (unlikely(!psMiss_tag)) {
		GED_LOGE("[GED_KPI][Exception]: ged_alloc_atomic(sizeof(GED_KPI_MISS_TAG)) failed\n");
		return;
	}

	memset(psMiss_tag, 0, sizeof(GED_KPI_MISS_TAG));
	INIT_LIST_HEAD(&psMiss_tag->sList);
	psMiss_tag->i32FrameID = i32FrameID;
	psMiss_tag->eTimeStampType = eTimeStampType;
	psMiss_tag->ulID = ulID;
	list_add_tail(&psMiss_tag->sList, &miss_tag_head->sList);
}
static GED_BOOL ged_kpi_find_and_delete_miss_tag(unsigned long ulID, int i32FrameID, GED_TIMESTAMP_TYPE eTimeStampType)
{
	GED_BOOL ret = GED_FALSE;

	if (miss_tag_head) {
		GED_KPI_MISS_TAG *psMiss_tag;
		struct list_head *psListEntry, *psListEntryTemp;
		struct list_head *psList = &miss_tag_head->sList;

		list_for_each_prev_safe(psListEntry, psListEntryTemp, psList) {
			psMiss_tag = list_entry(psListEntry, GED_KPI_MISS_TAG, sList);
			if (psMiss_tag
				&& psMiss_tag->ulID == ulID
				&& psMiss_tag->i32FrameID == i32FrameID
				&& psMiss_tag->eTimeStampType == eTimeStampType) {
				list_del(&psMiss_tag->sList);
				if (psMiss_tag != miss_tag_head)
					ged_free(psMiss_tag, sizeof(GED_KPI_MISS_TAG));
				ret = GED_TRUE;
				break;
			}
		}
	}

	return ret;
}
/* ----------------------------------------------------------------------------- */
static void ged_kpi_work_cb(struct work_struct *psWork)
{
	GED_TIMESTAMP *psTimeStamp = GED_CONTAINER_OF(psWork, GED_TIMESTAMP, sWork);
	GED_KPI_HEAD *psHead;
	GED_KPI *psKPI;
	unsigned long ulID;

#ifdef GED_KPI_DEBUG
	GED_LOGE("[GED_KPI] ts type = %d, pid = %d, wnd = %llu, frame = %lu\n",
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
				if (psHead == main_head)
					main_head = NULL;
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
				psHead->i32DebugQedBuffer_length = 0;
				psHead->isSF = psTimeStamp->isSF;
				psHead->i32Gpu_uncompleted = 0;
				ged_kpi_update_target_time_and_target_fps(psHead,
					GED_KPI_MAX_FPS, GED_KPI_FRC_DEFAULT_MODE, -1);
				INIT_LIST_HEAD(&psHead->sList);
				ged_hashtable_set(gs_hashtable, ulID, (void *)psHead);
			} else {
				GED_LOGE("[GED_KPI][Exception] ged_alloc_atomic(sizeof(GED_KPI_HEAD)) failed\n");
			}
		}
		if (psHead) {
#ifdef GED_KPI_DFRC
			int d_target_fps, mode, client;
			unsigned long long ctx_id;
#endif

			/* new data */
			psKPI->pid = psTimeStamp->pid;
			psKPI->ullWnd = psTimeStamp->ullWnd;
			psKPI->i32QueueID = psTimeStamp->i32FrameID;
			psKPI->ulMask = GED_TIMESTAMP_TYPE_1;
			psKPI->ullTimeStamp1 = psTimeStamp->ullTimeStamp;
			psKPI->i32QedBuffer_length = psTimeStamp->i32QedBuffer_length;
			psHead->i32QedBuffer_length = psTimeStamp->i32QedBuffer_length;
			list_add_tail(&psKPI->sList, &psHead->sList);

			/* section to query fps from FPS policy */
#ifdef GED_KPI_DFRC
			if (ged_frr_fence2context_table_get_cid(psKPI->pid,
				psTimeStamp->fence_addr,
				(uint64_t *)&ctx_id) == GED_OK) {

				if (dfrc_get_frr_config(psKPI->pid, ctx_id,
					&d_target_fps, &mode, &client) == 0) {

					if (d_target_fps == 0)
						d_target_fps = GED_KPI_MAX_FPS;

					ged_kpi_update_target_time_and_target_fps(psHead,
							d_target_fps, mode, client);
#ifdef GED_KPI_DEBUG
					GED_LOGE("[GED_KPI] psHead: %p, fps: %d, mode: %d, client: %d\n",
							psHead, d_target_fps, mode, client);
#endif
				}
			} else {
#ifdef GED_KPI_DEBUG
				GED_LOGE("[GED_KPI][Exception] failed to get ctx_id by fence_addr: %p\n",
						psTimeStamp->fence_addr);
#endif
				ged_kpi_update_target_time_and_target_fps(psHead,
									GED_KPI_MAX_FPS, GED_KPI_FRC_DEFAULT_MODE, -1);
			}
#endif /* GED_KPI_DFRC */

			/**********************************/
			psKPI->t_cpu_target = psHead->t_cpu_target;
			psKPI->t_gpu_target = psHead->t_gpu_target;
			psHead->i32Count += 1;
			psHead->i32Gpu_uncompleted += 1;
			psKPI->i32Gpu_uncompleted = psHead->i32Gpu_uncompleted;
			psHead->i32DebugQedBuffer_length += 1;
			psKPI->i32DebugQedBuffer_length = psHead->i32DebugQedBuffer_length;
			/* recording cpu time per frame and boost CPU if needed */
			psHead->t_cpu_latest =
				psKPI->ullTimeStamp1 - psHead->last_TimeStamp1 - psHead->last_QedBufferDelay;
			psKPI->t_cpu_latest = psHead->t_cpu_latest;
			psKPI->QedBufferDelay = psHead->last_QedBufferDelay;
			psHead->last_QedBufferDelay = 0;
#ifdef GED_KPI_CPU_BOOST
			if ((gx_game_mode == 1 || gx_force_cpu_boost == 1) && enable_cpu_boost == 1) {
				/* is_EAS_boost_off = 0; */
				ged_kpi_cpu_boost(psHead, psKPI);
			}
#endif
			/* else if (is_EAS_boost_off == 0) {is_EAS_boost_off = 1;perfmgr_kick_fg_boost(KIR_GED, -1);} */
			psHead->last_TimeStamp1 = psKPI->ullTimeStamp1;

			if (ged_kpi_find_and_delete_miss_tag(ulID, psTimeStamp->i32FrameID
									, GED_TIMESTAMP_TYPE_S) == GED_TRUE) {

				if (!psHead) {
					GED_LOGE("[GED_KPI][Exception] TYPE_S: no hashtable head for ulID: %lu\n",
							ulID);
					return;
				}

				psTimeStamp->eTimeStampType = GED_TIMESTAMP_TYPE_S;
				ged_kpi_tag_type_s(ulID, psHead, psTimeStamp);
				psHead->i32DebugQedBuffer_length -= 1;
#ifdef GED_KPI_DEBUG
				GED_LOGE("[GED_KPI] timestamp matched, Type_S: psHead: %p\n", psHead);
#endif

				if ((main_head != NULL && gx_game_mode == 1) &&
					(main_head->t_cpu_remained > (-1)*SCREEN_IDLE_PERIOD)) {
#ifdef GED_KPI_DEBUG
					GED_LOGE("[GED_KPI] t_cpu_remained: %lld, main_head: %p\n",
							 main_head->t_cpu_remained, main_head);
#endif
					ged_kpi_set_cpu_remained_time(main_head->t_cpu_remained,
											main_head->i32QedBuffer_length);
				}
			}

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
		else
			GED_LOGE("[GED_KPI][Exception] no hashtable head for ulID: %lu\n", ulID);
		break;
	case GED_TIMESTAMP_TYPE_2:
		ulID = (unsigned long) psTimeStamp->ullWnd;
		psHead = (GED_KPI_HEAD *)ged_hashtable_find(gs_hashtable, ulID);


		if (psHead) {
			struct list_head *psListEntry, *psListEntryTemp;
			struct list_head *psList = &psHead->sList;
			int boost_linear_gpu;

			list_for_each_prev_safe(psListEntry, psListEntryTemp, psList) {
				psKPI = list_entry(psListEntry, GED_KPI, sList);
				if (psKPI && ((psKPI->ulMask & GED_TIMESTAMP_TYPE_2) == 0)
						&& (psKPI->i32QueueID == psTimeStamp->i32FrameID)) {
					psKPI->ulMask |= GED_TIMESTAMP_TYPE_2;
					psKPI->ullTimeStamp2 = psTimeStamp->ullTimeStamp;
					/* calculate gpu time */
					if (psKPI->ullTimeStamp1 > psHead->last_TimeStamp2)
						psHead->t_gpu_latest = psKPI->ullTimeStamp2 - psKPI->ullTimeStamp1;
					else
						psHead->t_gpu_latest = psKPI->ullTimeStamp2 - psHead->last_TimeStamp2;

					psKPI->gpu_freq = mt_gpufreq_get_cur_freq() / 1000;

					psHead->last_TimeStamp2 = psTimeStamp->ullTimeStamp;
					psHead->i32Gpu_uncompleted -= 1;
					mtk_get_gpu_loading(&psKPI->gpu_loading);

					if (psHead->last_TimeStamp1 != psKPI->ullTimeStamp1) {
						psHead->last_QedBufferDelay =
							psTimeStamp->ullTimeStamp - psHead->last_TimeStamp1;
					}
					/* notify gpu performance info to GPU DVFS module */
					boost_linear_gpu = (int)psHead->t_gpu_latest * 100 / psHead->t_gpu_target;
					boost_accum_gpu = (boost_accum_gpu * boost_linear_gpu) / 100;

					if (boost_accum_gpu < 100)
						boost_accum_gpu = 100;
					else if (boost_accum_gpu > 500)
						boost_accum_gpu = 500;

					/* for aligning boost_accum_cpu presentation */
					psKPI->boost_accum_gpu = boost_accum_gpu - 100;

					if (gx_game_mode == 1 && enable_gpu_boost == 1 && gx_3D_benchmark_on == 0) {
						ged_kpi_set_gpu_dvfs_hint((int)(psHead->t_gpu_target/1000)
												, (int)boost_accum_gpu);
					} else {
						ged_kpi_set_gpu_dvfs_hint(((int)vsync_period / 1000), 100);
					}

					break;
				}
#ifdef GED_KPI_DEBUG
				else {
					GED_LOGE(
						"[GED_KPI][Exception] TYPE_2: frame(%lu) fail to be taged to ulID: %lu\n",
						psTimeStamp->i32FrameID,
						ulID);
				}
#endif
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


		if (!psHead) {
			GED_LOGE("[GED_KPI][Exception] TYPE_S: no hashtable head for ulID: %lu\n", ulID);
			return;
		}

		if (ged_kpi_tag_type_s(ulID, psHead, psTimeStamp) != GED_TRUE) {
#ifdef GED_KPI_DEBUG
			GED_LOGE("[GED_KPI] TYPE_S timestamp miss, ulID: %lu\n", ulID);
#endif
			ged_kpi_record_miss_tag(ulID, psTimeStamp->i32FrameID, GED_TIMESTAMP_TYPE_S);
		} else {
			psHead->i32DebugQedBuffer_length -= 1;
#ifdef GED_KPI_DEBUG
			GED_LOGE("[GED_KPI] timestamp matched, Type_S: psHead: %p\n", psHead);
#endif
		}

		if ((main_head != NULL && gx_game_mode == 1) && (main_head->t_cpu_remained > (-1)*SCREEN_IDLE_PERIOD)) {
#ifdef GED_KPI_DEBUG
			GED_LOGE("[GED_KPI] t_cpu_remained: %lld, main_head: %p\n", main_head->t_cpu_remained, main_head);
#endif
			ged_kpi_set_cpu_remained_time(main_head->t_cpu_remained, main_head->i32QedBuffer_length);
		}
		break;
	case GED_TIMESTAMP_TYPE_H:
		ged_hashtable_iterator(gs_hashtable, ged_kpi_h_iterator_func, (void *)psTimeStamp);

#ifndef GED_KPI_DFRC
		ged_kpi_update_target_time_and_target_fps(main_head, GED_KPI_MAX_FPS, GED_KPI_FRC_DEFAULT_MODE, -1);
#endif
		if (gx_dfps <= GED_KPI_MAX_FPS && gx_dfps >= 10)
			ged_kpi_update_target_time_and_target_fps(main_head
									, gx_dfps
									, gx_frc_mode
									, -1);
		else
			gx_dfps = 0;

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
	int isSF,
	void *fence_addr)
{
	static int event_QedBuffer_cnt, event_3d_fence_cnt, event_hw_vsync;

	if (g_psWorkQueue && is_GED_KPI_enabled) {
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
		psTimeStamp->fence_addr = fence_addr;
		INIT_WORK(&psTimeStamp->sWork, ged_kpi_work_cb);
		queue_work(g_psWorkQueue, &psTimeStamp->sWork);
		switch (eTimeStampType) {
		case GED_TIMESTAMP_TYPE_1:
			event_QedBuffer_cnt++;
			ged_log_trace_counter("GED_KPI_QedBuffer_CNT", event_QedBuffer_cnt);
			event_3d_fence_cnt++;
			ged_log_trace_counter("GED_KPI_3D_fence_CNT", event_3d_fence_cnt);
			break;
		case GED_TIMESTAMP_TYPE_2:
			event_3d_fence_cnt--;
			ged_log_trace_counter("GED_KPI_3D_fence_CNT", event_3d_fence_cnt);
			break;
		case GED_TIMESTAMP_TYPE_S:
			event_QedBuffer_cnt--;
			ged_log_trace_counter("GED_KPI_QedBuffer_CNT", event_QedBuffer_cnt);
			break;
		case GED_TIMESTAMP_TYPE_H:
			ged_log_trace_counter("GED_KPI_HW_Vsync", event_hw_vsync);
			event_hw_vsync++;
			event_hw_vsync %= 2;
			break;
		}
	}
#ifdef GED_KPI_DEBUG
	else {
		GED_LOGE("[GED_KPI][Exception]: g_psWorkQueue: NULL or GED KPI is disabled\n");
		return GED_ERROR_FAIL;
	}
#endif
	return GED_OK;
}
/* ----------------------------------------------------------------------------- */
static GED_ERROR ged_kpi_time1(int pid, unsigned long long ullWdnd, int i32FrameID, int QedBuffer_length
							, int isSF, void *fence_addr)
{
	return ged_kpi_push_timestamp(GED_TIMESTAMP_TYPE_1, ged_get_time(), pid,
						ullWdnd, i32FrameID, QedBuffer_length, isSF, fence_addr);
}
/* ----------------------------------------------------------------------------- */
static GED_ERROR ged_kpi_time2(int pid, unsigned long long ullWdnd, int i32FrameID)
{
	return ged_kpi_push_timestamp(GED_TIMESTAMP_TYPE_2, ged_get_time(), pid,
								ullWdnd, i32FrameID, -1, -1, NULL);
}
/* ----------------------------------------------------------------------------- */
static GED_ERROR ged_kpi_timeS(int pid, unsigned long long ullWdnd, int i32FrameID)
{
	return ged_kpi_push_timestamp(GED_TIMESTAMP_TYPE_S, ged_get_time(), pid,
								ullWdnd, i32FrameID, -1, -1, NULL);
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
unsigned int ged_kpi_enabled(void)
{
#ifdef MTK_GED_KPI
	return is_GED_KPI_enabled;
#else
	return 0;
#endif
}
/* ----------------------------------------------------------------------------- */
GED_ERROR ged_kpi_queue_buffer_ts(int pid, unsigned long long ullWdnd, int i32FrameID, int fence_fd, int QedBuffer_length, int isSF)
{
#ifdef MTK_GED_KPI
	GED_ERROR ret;
	GED_KPI_GPU_TS *psMonitor;
	struct sync_fence *psSyncFence;

	psSyncFence = sync_fence_fdget(fence_fd);

	ret = ged_kpi_time1(pid, ullWdnd, i32FrameID, QedBuffer_length, isSF, (void *)psSyncFence);

	if (ret != GED_OK)
		return ret;

	psMonitor = (GED_KPI_GPU_TS *)ged_alloc(sizeof(GED_KPI_GPU_TS));

	if (!psMonitor)
		return GED_ERROR_OOM;

	sync_fence_waiter_init(&psMonitor->sSyncWaiter, ged_kpi_gpu_3d_fence_sync_cb);
	psMonitor->psSyncFence = psSyncFence;
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
	return ged_kpi_push_timestamp(GED_TIMESTAMP_TYPE_H, ged_get_time(), 0, 0, 0, 0, 0, NULL);
#else
	return GED_OK;
#endif
}
/* ----------------------------------------------------------------------------- */
GED_BOOL ged_kpi_set_target_fps(unsigned int target_fps, int mode)
{
#ifdef MTK_GED_KPI
	return ged_kpi_update_target_time_and_target_fps(main_head, target_fps, mode, -1);
#else
	return GED_FALSE;
#endif
}
/* ----------------------------------------------------------------------------- */
void ged_kpi_get_latest_perf_state(long long *t_cpu_remained,
									long long *t_gpu_remained,
									long *t_cpu_target,
									long *t_gpu_target)
{
#ifdef MTK_GED_KPI
	if (t_cpu_remained != NULL && main_head != NULL && !(main_head->t_cpu_remained < (-1)*SCREEN_IDLE_PERIOD))
		*t_cpu_remained = main_head->t_cpu_remained;

	if (t_gpu_remained != NULL && main_head != NULL && !(main_head->t_gpu_remained < (-1)*SCREEN_IDLE_PERIOD))
		*t_gpu_remained = main_head->t_gpu_remained;

	if (t_cpu_target != NULL && main_head != NULL)
		*t_cpu_target = main_head->t_cpu_target;
	if (t_gpu_target != NULL && main_head != NULL)
		*t_gpu_target = main_head->t_gpu_target;
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

	ghLogBuf = ged_log_buf_alloc(GED_KPI_MAX_FPS * 5,
		220 * GED_KPI_MAX_FPS * 5, GED_LOG_BUF_TYPE_RINGBUFFER, NULL, "KPI");

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
void (*ged_kpi_set_gpu_dvfs_hint_fp)(int t_gpu_target, int boost_accum_gpu);
EXPORT_SYMBOL(ged_kpi_set_gpu_dvfs_hint_fp);

bool ged_kpi_set_gpu_dvfs_hint(int t_gpu_target, int boost_accum_gpu)
{
	if (ged_kpi_set_gpu_dvfs_hint_fp != NULL) {
		ged_kpi_set_gpu_dvfs_hint_fp(t_gpu_target, boost_accum_gpu);
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
void (*ged_kpi_set_game_hint_value_fp_2)(int is_game_mode);
EXPORT_SYMBOL(ged_kpi_set_game_hint_value_fp_2);

bool ged_kpi_set_game_hint_value(int is_game_mode)
{
	bool ret = false;

	if (ged_kpi_set_game_hint_value_fp != NULL) {
		ged_kpi_set_game_hint_value_fp(is_game_mode);
		ret = true;
	}
	if (ged_kpi_set_game_hint_value_fp_2 != NULL) {
		ged_kpi_set_game_hint_value_fp_2(is_game_mode);
		ret = true;
	}
	return ret;
}
/* ----------------------------------------------------------------------------- */
void ged_kpi_set_game_hint(int mode)
{
#ifdef MTK_GED_KPI
	if (mode == 1 || mode == 0) {
		gx_game_mode = mode;
		ged_kpi_set_game_hint_value(mode);
	}
#endif
}
