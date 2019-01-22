/*
 * Copyright (C) 2017 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/hardirq.h>
#include <linux/init.h>
#include <linux/kallsyms.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/workqueue.h>
#include <linux/kthread.h>
#include <mt-plat/aee.h>

#include <mach/mtk_ppm_api.h>
#include <fpsgo_common.h>

#include "fbt_error.h"
#include "fbt_base.h"
#include "fbt_usedext.h"
#include "fbt_cpu.h"
#include "xgf.h"

/*#define FBT_NOTIFIER_DEBUG 1*/

static struct workqueue_struct *g_psNotifyWorkQueue;
static struct mutex notify_lock;
static int game_mode, benchmark_mode;

enum FBT_NOTIFIER_PUSH_TYPE {
	FBT_NOTIFIER_PUSH_TYPE_FRAMETIME	= 0x01,
	FBT_NOTIFIER_PUSH_DFRC_FPS_LIMIT	= 0x02,
	FBT_NOTIFIER_PUSH_GAME_MODE		= 0x03,
};

struct FBT_NOTIFIER_PUSH_TAG {
	enum FBT_NOTIFIER_PUSH_TYPE ePushType;

	unsigned long long Q2Q_time;
	unsigned long long Running_time;
	unsigned int Curr_cap;
	unsigned int Max_cap;
	unsigned int Target_fps;
	unsigned int DFRC_fps_limit;
	int game;

	struct work_struct sWork;

};

static unsigned int gLast_Q2Q;
static unsigned int gLast_Running;

int (*fbt_notifier_cpu_frame_time_fps_stabilizer)(
	unsigned long long Q2Q_time,
	unsigned long long Runnging_time,
	unsigned int Curr_cap,
	unsigned int Max_cap,
	unsigned int Target_fps);
EXPORT_SYMBOL(fbt_notifier_cpu_frame_time_fps_stabilizer);

int (*fbt_notifier_dfrc_fp_fbt_cpu)(unsigned int DFRC_fpt_limit);
EXPORT_SYMBOL(fbt_notifier_dfrc_fp_fbt_cpu);

static void fbt_notifier_wq_cb_FBT_NOTIFIER_PUSH_TYPE_FRAMETIME(struct FBT_NOTIFIER_PUSH_TAG *pPush)
{
#ifdef FBT_NOTIFIER_DEBUG
	FBT_LOGE("[FBT_NTF_PTF] Q2Q %llu, Running %llu, Curr_cap %u,Max_cap %u, Target_fps %d\n",
			pPush->Q2Q_time,
			pPush->Running_time,
			pPush->Curr_cap,
			pPush->Max_cap,
			pPush->Target_fps);
#endif

	if (fbt_notifier_cpu_frame_time_fps_stabilizer)
		fbt_notifier_cpu_frame_time_fps_stabilizer(
			pPush->Q2Q_time,
			pPush->Running_time,
			pPush->Curr_cap,
			pPush->Max_cap,
			pPush->Target_fps);

}

static void fbt_notifier_wq_cb_FBT_NOTIFIER_PUSH_DFRC_FPS_LIMIT(struct FBT_NOTIFIER_PUSH_TAG *pPush)
{
	if (fbt_notifier_dfrc_fp_fbt_cpu)
		fbt_notifier_dfrc_fp_fbt_cpu(pPush->DFRC_fps_limit);
}

void fbt_notifier_push_benchmark_hint(int is_benchmark)
{
	mutex_lock(&notify_lock);
	benchmark_mode = is_benchmark;

#ifdef FBT_NOTIFIER_DEBUG
	FBT_LOGE("[FBT_NOTIFIER] fbt_notifier_push_benchmark_hint game = %d benchmark_mode = %d\n",
			game_mode, benchmark_mode);
#endif

	if (game_mode && !benchmark_mode) {
		fbt_cpu_set_game_hint_cb(1);
		fstb_game_mode_change(1);
		fbc_notify_game(1);
		xgf_game_mode_exit(0);
	} else {
		fbt_cpu_set_game_hint_cb(0);
		fstb_game_mode_change(0);
		fbc_notify_game(0);
		xgf_game_mode_exit(1);
	}
	mutex_unlock(&notify_lock);
}

static void fbt_notifier_wq_cb_FBT_NOTIFIER_PUSH_GAME_MODE(struct FBT_NOTIFIER_PUSH_TAG *pPush)
{
	mutex_lock(&notify_lock);
	game_mode = pPush->game;

#ifdef FBT_NOTIFIER_DEBUG
	FBT_LOGE("[FBT_NOTIFIER] fbt_notifier_wq_cb_FBT_NOTIFIER_PUSH_GAME_MODE game = %d, benchmark_mode = %d\n",
			game_mode, benchmark_mode);
#endif

	if (game_mode && !benchmark_mode) {
		fbt_cpu_set_game_hint_cb(1);
		fstb_game_mode_change(1);
		fbc_notify_game(1);
		xgf_game_mode_exit(0);
	} else {
		fbt_cpu_set_game_hint_cb(0);
		fstb_game_mode_change(0);
		fbc_notify_game(0);
		xgf_game_mode_exit(1);
	}
	mutex_unlock(&notify_lock);
}

static void fbt_notifier_wq_cb(struct work_struct *psWork)
{
	struct FBT_NOTIFIER_PUSH_TAG *vpPush = FBT_CONTAINER_OF(psWork, struct FBT_NOTIFIER_PUSH_TAG, sWork);

#ifdef FBT_NOTIFIER_DEBUG
	FBT_LOGE("[FBT_NOTIFIER] push type = %d\n", vpPush->ePushType);
#endif

	switch (vpPush->ePushType) {
	case FBT_NOTIFIER_PUSH_TYPE_FRAMETIME:
		fbt_notifier_wq_cb_FBT_NOTIFIER_PUSH_TYPE_FRAMETIME(vpPush);
		break;
	case FBT_NOTIFIER_PUSH_DFRC_FPS_LIMIT:
		fbt_notifier_wq_cb_FBT_NOTIFIER_PUSH_DFRC_FPS_LIMIT(vpPush);
		break;
	case FBT_NOTIFIER_PUSH_GAME_MODE:
		fbt_notifier_wq_cb_FBT_NOTIFIER_PUSH_GAME_MODE(vpPush);
		break;

	default:
		FBT_LOGE("[FBT_NOTIFIER] unhandled push type = %d\n", vpPush->ePushType);
		break;
	}

	fbt_free(vpPush, sizeof(struct FBT_NOTIFIER_PUSH_TAG));
}

int fbt_notifier_push_cpu_frame_time(
	unsigned long long Q2Q_time,
	unsigned long long Running_time)
{
	fpsgo_systrace_c_ntfr(-100, Q2Q_time, "Q2Q_time");
	fpsgo_systrace_c_ntfr(-100, Running_time, "Running_time");

	gLast_Q2Q = Q2Q_time;
	gLast_Running = Running_time;

	return FBT_OK;
}

int fbt_notifier_push_cpu_capability(
	unsigned int curr_cap,
	unsigned int max_cap,
	unsigned int target_fps)
{
	fpsgo_systrace_c_ntfr(-100, curr_cap, "curr_cap");
	fpsgo_systrace_c_ntfr(-100, max_cap, "max_cap");

	if (g_psNotifyWorkQueue) {
		struct FBT_NOTIFIER_PUSH_TAG *vpPush =
			(struct FBT_NOTIFIER_PUSH_TAG *) fbt_alloc_atomic(sizeof(struct FBT_NOTIFIER_PUSH_TAG));

		if (!vpPush)
			return FBT_ERROR_OOM;

		vpPush->ePushType = FBT_NOTIFIER_PUSH_TYPE_FRAMETIME;
		vpPush->Q2Q_time = gLast_Q2Q;
		vpPush->Running_time = gLast_Running;
		vpPush->Curr_cap = curr_cap;
		vpPush->Max_cap = max_cap;
		vpPush->Target_fps = target_fps;

		INIT_WORK(&vpPush->sWork, fbt_notifier_wq_cb);
		queue_work(g_psNotifyWorkQueue, &vpPush->sWork);
	}

	return FBT_OK;
}

int fbt_notifier_push_game_mode(int is_game_mode)
{
	if (g_psNotifyWorkQueue) {
		struct FBT_NOTIFIER_PUSH_TAG *vpPush =
			(struct FBT_NOTIFIER_PUSH_TAG *) fbt_alloc_atomic(sizeof(struct FBT_NOTIFIER_PUSH_TAG));

		if (!vpPush)
			return FBT_ERROR_OOM;

		vpPush->ePushType = FBT_NOTIFIER_PUSH_GAME_MODE;
		vpPush->game = is_game_mode;

		INIT_WORK(&vpPush->sWork, fbt_notifier_wq_cb);
		queue_work(g_psNotifyWorkQueue, &vpPush->sWork);
	}

	return FBT_OK;
}

int fbt_notifier_push_dfrc_fps_limit(unsigned int fps_limit)
{
	fpsgo_systrace_c_ntfr(-200, fps_limit, "dfrc_fps_limit");

	if (g_psNotifyWorkQueue) {
		struct FBT_NOTIFIER_PUSH_TAG *vpPush =
			(struct FBT_NOTIFIER_PUSH_TAG *) fbt_alloc_atomic(sizeof(struct FBT_NOTIFIER_PUSH_TAG));

		if (!vpPush)
			return FBT_ERROR_OOM;

		vpPush->ePushType = FBT_NOTIFIER_PUSH_DFRC_FPS_LIMIT;
		vpPush->DFRC_fps_limit = fps_limit;

		INIT_WORK(&vpPush->sWork, fbt_notifier_wq_cb);
		queue_work(g_psNotifyWorkQueue, &vpPush->sWork);
	}

	return FBT_OK;
}

void fbt_notifier_exit(void)
{
	if (!g_psNotifyWorkQueue)
		return;

	flush_workqueue(g_psNotifyWorkQueue);

	destroy_workqueue(g_psNotifyWorkQueue);

	g_psNotifyWorkQueue = NULL;
}

int fbt_notifier_init(void)
{
	mutex_init(&notify_lock);
	g_psNotifyWorkQueue = create_workqueue("fbt_notifier_wq");

	if (g_psNotifyWorkQueue == NULL)
		goto ERROR;

	return 0;

ERROR:
	fbt_notifier_exit();

	return -EFAULT;
}
