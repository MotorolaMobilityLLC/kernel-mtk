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
#include <linux/workqueue.h>
#include <linux/unistd.h>
#include <linux/module.h>
#include <linux/sched.h>

#include "fpsgo_common.h"
#include "fpsgo_base.h"
#include "fpsgo_usedext.h"
#include "fbt_cpu.h"
#include "fstb.h"
#include "fps_composer.h"
#include "xgf.h"

#ifdef CONFIG_MTK_DYNAMIC_FPS_FRAMEWORK_SUPPORT
#include "dfrc.h"
#include "dfrc_drv.h"
#endif

#define TARGET_UNLIMITED_FPS 60

enum FPSGO_NOTIFIER_PUSH_TYPE {
	FPSGO_NOTIFIER_SWITCH_FPSGO			= 0x00,
	FPSGO_NOTIFIER_QUEUE_DEQUEUE		= 0x01,
	FPSGO_NOTIFIER_INTENDED_VSYNC		= 0x02,
	FPSGO_NOTIFIER_FRAME_COMPLETE		= 0x03,
	FPSGO_NOTIFIER_CONNECT				= 0x04,
	FPSGO_NOTIFIER_CPU_CAP				= 0x05,
	FPSGO_NOTIFIER_DFRC_FPS				= 0x06,
};

/* TODO: use union*/
struct FPSGO_NOTIFIER_PUSH_TAG {
	enum FPSGO_NOTIFIER_PUSH_TYPE ePushType;

	int pid;
	unsigned long long cur_ts;

	int enable;

	int qudeq_cmd;
	unsigned int queue_arg;

	unsigned long long bufID;
	int connectedAPI;

	int frame_type;
	unsigned long long Q2Q_time;
	unsigned long long Running_time;
	unsigned int Curr_cap;
	unsigned int Max_cap;
	unsigned int Target_fps;

	int ui_pid;
	unsigned long long frame_time;
	int render_method;
	int render;

	int dfrc_fps;

	struct work_struct sWork;
};

static struct mutex notify_lock;
struct workqueue_struct *g_psNotifyWorkQueue;
static int fpsgo_enable;

/* TODO: event register & dispatch */
static void fpsgo_notifier_wq_cb_dfrc_fps(int dfrc_fps)
{
	FPSGO_LOGI("[FPSGO_CB] dfrc_fps %d\n", dfrc_fps);

	fpsgo_ctrl2fbt_dfrc_fps(dfrc_fps);
}

static void fpsgo_notifier_wq_cb_intended_vsync(int cur_pid, unsigned long long cur_ts)
{
	FPSGO_LOGI("[FPSGO_CB] intended_vsync: pid %d, ts %llu\n", cur_pid, cur_ts);

	fpsgo_ctrl2comp_vysnc_aligned_frame_start(cur_pid, cur_ts);
}

static void fpsgo_notifier_wq_cb_connect(int pid, unsigned long long bufID, int connectedAPI)
{
	FPSGO_LOGI("[FPSGO_CB] connect: pid %d, bufID %llu, connectedAPI %d\n", pid, bufID, connectedAPI);

	if (connectedAPI == WINDOW_DISCONNECT)
		fpsgo_ctrl2comp_disconnect_api(pid, bufID, connectedAPI);
	else
		fpsgo_ctrl2comp_connect_api(pid, bufID, connectedAPI);
}

static void fpsgo_notifier_wq_cb_framecomplete(int cur_pid, int ui_pid, unsigned long long frame_time,
						int render_method, int render, unsigned long long cur_ts)
{
	FPSGO_LOGI("[FPSGO_CB] framecomplete: cur_pid %d, ui_pid %d, frame_time %llu, render %d, render_method %d\n",
				cur_pid, ui_pid, frame_time, render, render_method);

	fpsgo_ctrl2comp_vysnc_aligned_frame_done(cur_pid, ui_pid, frame_time, render, cur_ts, render_method);
}

static void fpsgo_notifier_wq_cb_qudeq(int qudeq, unsigned int startend, unsigned long long bufID,
					int cur_pid, unsigned long long curr_ts)
{
	FPSGO_LOGI("[FPSGO_CB] qudeq: %d-%d, buf %llu, pid %d, ts %llu\n",
		qudeq, startend, bufID, cur_pid, curr_ts);

	switch (qudeq) {
	case 1:
		if (startend) {
			FPSGO_LOGI("[FPSGO_CTRL] QUEUE Start: pid %d\n", cur_pid);
			fpsgo_ctrl2comp_enqueue_start(cur_pid, curr_ts, bufID);
		} else {
			FPSGO_LOGI("[FPSGO_CTRL] QUEUE End: pid %d\n", cur_pid);
			fpsgo_ctrl2comp_enqueue_end(cur_pid, curr_ts, bufID);
		}
		break;
	case 0:
		if (startend) {
			FPSGO_LOGI("[FPSGO_CTRL] DEQUEUE Start: pid %d\n", cur_pid);
			fpsgo_ctrl2comp_dequeue_start(cur_pid, curr_ts, bufID);
		} else {
			FPSGO_LOGI("[FPSGO_CTRL] DEQUEUE End: pid %d\n", cur_pid);
			fpsgo_ctrl2comp_dequeue_end(cur_pid, curr_ts, bufID);
		}
		break;
	default:
		break;
	}
}

static void fpsgo_notifier_wq_cb_enable(int enable)
{
	FPSGO_LOGI("[FPSGO_CB] enable %d, fpsgo_enable %d\n", enable, fpsgo_enable);

	mutex_lock(&notify_lock);
	if (enable == fpsgo_enable) {
		mutex_unlock(&notify_lock);
		return;
	}

	fpsgo_ctrl2fbt_switch_fbt(enable);
	fpsgo_ctrl2fstb_switch_fstb(enable);
	fpsgo_ctrl2xgf_switch_xgf(enable);

	if (enable == 1)
		fpsgo_ctrl2comp_resent_by_pass_info();

	fpsgo_enable = enable;
	FPSGO_LOGI("[FPSGO_CB] fpsgo_enable %d\n", fpsgo_enable);
	mutex_unlock(&notify_lock);
}

static void fpsgo_notifier_wq_cb(struct work_struct *psWork)
{
	struct FPSGO_NOTIFIER_PUSH_TAG *vpPush = FPSGO_CONTAINER_OF(psWork, struct FPSGO_NOTIFIER_PUSH_TAG, sWork);

	if (!vpPush) {
		FPSGO_LOGE("[FPSGO_CTRL] ERROR\n");
		return;
	}

	FPSGO_LOGI("[FPSGO_CTRL] push type = %d\n", vpPush->ePushType);

	switch (vpPush->ePushType) {
	case FPSGO_NOTIFIER_SWITCH_FPSGO:
		fpsgo_notifier_wq_cb_enable(vpPush->enable);
		break;
	case FPSGO_NOTIFIER_QUEUE_DEQUEUE:
		fpsgo_notifier_wq_cb_qudeq(vpPush->qudeq_cmd, vpPush->queue_arg, vpPush->bufID,
					vpPush->pid, vpPush->cur_ts);
		break;
	case FPSGO_NOTIFIER_INTENDED_VSYNC:
		fpsgo_notifier_wq_cb_intended_vsync(vpPush->pid, vpPush->cur_ts);
		break;
	case FPSGO_NOTIFIER_FRAME_COMPLETE:
		fpsgo_notifier_wq_cb_framecomplete(vpPush->pid, vpPush->ui_pid, vpPush->frame_time,
					vpPush->render_method, vpPush->render, vpPush->cur_ts);
		break;
	case FPSGO_NOTIFIER_CONNECT:
		fpsgo_notifier_wq_cb_connect(vpPush->pid, vpPush->bufID, vpPush->connectedAPI);
		break;
	case FPSGO_NOTIFIER_CPU_CAP:
		fpsgo_fbt2fstb_update_cpu_frame_info(
			vpPush->pid,
			vpPush->frame_type,
			vpPush->Q2Q_time,
			vpPush->Running_time,
			vpPush->Curr_cap,
			vpPush->Max_cap,
			vpPush->Target_fps);
		break;
	case FPSGO_NOTIFIER_DFRC_FPS:
		fpsgo_notifier_wq_cb_dfrc_fps(vpPush->dfrc_fps);
		break;
	default:
		FPSGO_LOGE("[FPSGO_CTRL] unhandled push type = %d\n", vpPush->ePushType);
		break;
	}

	fpsgo_free(vpPush, sizeof(struct FPSGO_NOTIFIER_PUSH_TAG));
}

int fpsgo_fbt2fstb_cpu_capability(
	int pid, int frame_type,
	unsigned int curr_cap,
	unsigned int max_cap,
	unsigned int target_fps,
	unsigned long long Q2Q_time,
	unsigned long long Running_time)
{
	struct FPSGO_NOTIFIER_PUSH_TAG *vpPush =
		(struct FPSGO_NOTIFIER_PUSH_TAG *) fpsgo_alloc_atomic(sizeof(struct FPSGO_NOTIFIER_PUSH_TAG));

	if (!vpPush) {
		FPSGO_LOGE("[FPSGO_CTRL] OOM\n");
		return FPSGO_ERROR_OOM;
	}

	if (!g_psNotifyWorkQueue) {
		FPSGO_LOGE("[FPSGO_CTRL] NULL WorkQueue\n");
		fpsgo_free(vpPush, sizeof(struct FPSGO_NOTIFIER_PUSH_TAG));
		return FPSGO_ERROR_OOM;
	}

	vpPush->ePushType = FPSGO_NOTIFIER_CPU_CAP;
	vpPush->pid = pid;
	vpPush->frame_type = frame_type;
	vpPush->Q2Q_time = Q2Q_time;
	vpPush->Running_time = Running_time;
	vpPush->Curr_cap = curr_cap;
	vpPush->Max_cap = max_cap;
	vpPush->Target_fps = target_fps;

	INIT_WORK(&vpPush->sWork, fpsgo_notifier_wq_cb);
	queue_work(g_psNotifyWorkQueue, &vpPush->sWork);

	return FPSGO_OK;
}

void fpsgo_notify_qudeq(int qudeq, unsigned int startend, unsigned long long bufID, int pid)
{
	unsigned long long cur_ts;
	struct FPSGO_NOTIFIER_PUSH_TAG *vpPush;

	FPSGO_LOGI("[FPSGO_CTRL] qudeq %d-%d, buf %llu pid %d\n", qudeq, startend, bufID, pid);

	if (!fpsgo_enable)
		return;

	vpPush =
		(struct FPSGO_NOTIFIER_PUSH_TAG *) fpsgo_alloc_atomic(sizeof(struct FPSGO_NOTIFIER_PUSH_TAG));
	if (!vpPush) {
		FPSGO_LOGE("[FPSGO_CTRL] OOM\n");
		return;
	}

	if (!g_psNotifyWorkQueue) {
		FPSGO_LOGE("[FPSGO_CTRL] NULL WorkQueue\n");
		fpsgo_free(vpPush, sizeof(struct FPSGO_NOTIFIER_PUSH_TAG));
		return;
	}

	cur_ts = fpsgo_get_time();

	vpPush->ePushType = FPSGO_NOTIFIER_QUEUE_DEQUEUE;
	vpPush->pid = pid;
	vpPush->cur_ts = cur_ts;
	vpPush->qudeq_cmd = qudeq;
	vpPush->queue_arg = startend;
	vpPush->bufID = bufID;

	INIT_WORK(&vpPush->sWork, fpsgo_notifier_wq_cb);
	queue_work(g_psNotifyWorkQueue, &vpPush->sWork);
}

void fpsgo_notify_intended_vsync(int pid)
{
	unsigned long long cur_ts;
	struct FPSGO_NOTIFIER_PUSH_TAG *vpPush;

	FPSGO_LOGI("[FPSGO_CTRL] intended_vsync pid %d\n", pid);

	if (!fpsgo_enable)
		return;

	cur_ts = fpsgo_get_time();

	vpPush =
		(struct FPSGO_NOTIFIER_PUSH_TAG *) fpsgo_alloc_atomic(sizeof(struct FPSGO_NOTIFIER_PUSH_TAG));

	if (!vpPush) {
		FPSGO_LOGE("[FPSGO_CTRL] OOM\n");
		return;
	}

	if (!g_psNotifyWorkQueue) {
		FPSGO_LOGE("[FPSGO_CTRL] NULL WorkQueue\n");
		fpsgo_free(vpPush, sizeof(struct FPSGO_NOTIFIER_PUSH_TAG));
		return;
	}

	vpPush->ePushType = FPSGO_NOTIFIER_INTENDED_VSYNC;
	vpPush->pid = pid;
	vpPush->cur_ts = cur_ts;

	INIT_WORK(&vpPush->sWork, fpsgo_notifier_wq_cb);
	queue_work(g_psNotifyWorkQueue, &vpPush->sWork);
}

void fpsgo_notify_framecomplete(int ui_pid, unsigned long long frame_time,
								int render_method, int render)
{
	unsigned long long cur_ts;
	struct FPSGO_NOTIFIER_PUSH_TAG *vpPush;
	int cur_pid = task_pid_nr(current);

	FPSGO_LOGI("[FPSGO_CTRL] framecomplete: cur_pid %d, ui_pid %d, frame_time %llu, render %d, render_method %d\n",
		cur_pid, ui_pid, frame_time, render, render_method);

	if (!fpsgo_enable)
		return;

	cur_ts = fpsgo_get_time();

	vpPush =
		(struct FPSGO_NOTIFIER_PUSH_TAG *) fpsgo_alloc_atomic(sizeof(struct FPSGO_NOTIFIER_PUSH_TAG));

	if (!vpPush) {
		FPSGO_LOGE("[FPSGO_CTRL] OOM\n");
		return;
	}

	if (!g_psNotifyWorkQueue) {
		FPSGO_LOGE("[FPSGO_CTRL] NULL WorkQueue\n");
		fpsgo_free(vpPush, sizeof(struct FPSGO_NOTIFIER_PUSH_TAG));
		return;
	}

	vpPush->ePushType = FPSGO_NOTIFIER_FRAME_COMPLETE;
	vpPush->pid = cur_pid;
	vpPush->cur_ts = cur_ts;
	vpPush->ui_pid = ui_pid;
	vpPush->frame_time = frame_time;
	vpPush->render = render;
	vpPush->render_method = render_method;

	INIT_WORK(&vpPush->sWork, fpsgo_notifier_wq_cb);
	queue_work(g_psNotifyWorkQueue, &vpPush->sWork);
}

void fpsgo_notify_connect(int pid, unsigned long long bufID, int connectedAPI)
{
	struct FPSGO_NOTIFIER_PUSH_TAG *vpPush;

	FPSGO_LOGI("[FPSGO_CTRL] connect pid %d, buf %llu, API %d\n", pid, bufID, connectedAPI);

	vpPush =
		(struct FPSGO_NOTIFIER_PUSH_TAG *) fpsgo_alloc_atomic(sizeof(struct FPSGO_NOTIFIER_PUSH_TAG));

	if (!vpPush) {
		FPSGO_LOGE("[FPSGO_CTRL] OOM\n");
		return;
	}

	if (!g_psNotifyWorkQueue) {
		FPSGO_LOGE("[FPSGO_CTRL] NULL WorkQueue\n");
		fpsgo_free(vpPush, sizeof(struct FPSGO_NOTIFIER_PUSH_TAG));
		return;
	}

	vpPush->ePushType = FPSGO_NOTIFIER_CONNECT;
	vpPush->pid = pid;
	vpPush->bufID = bufID;
	vpPush->connectedAPI = connectedAPI;

	INIT_WORK(&vpPush->sWork, fpsgo_notifier_wq_cb);
	queue_work(g_psNotifyWorkQueue, &vpPush->sWork);
}

void fpsgo_notify_vsync(void)
{
	FPSGO_LOGI("[FPSGO_CTRL] vsync\n");

	if (!fpsgo_enable)
		return;

	fpsgo_ctrl2fbt_vsync();
}

void fpsgo_notify_cpufreq(int cid, unsigned long freq)
{
	FPSGO_LOGI("[FPSGO_CTRL] cid %d, cpufreq %lu\n", cid, freq);

	if (!fpsgo_enable)
		return;

	fpsgo_ctrl2fbt_cpufreq_cb(cid, freq);
}

void fpsgo_notify_display_update(unsigned long long ts)
{
	FPSGO_LOGI("[FPSGO_CTRL] display_update\n");

	if (!fpsgo_enable)
		return;

	fpsgo_ctrl2fstb_display_time_update(ts);
}

void fpsgo_notify_gpu_display_update(long long t_gpu, unsigned int cur_freq, unsigned int cur_max_freq)
{
	FPSGO_LOGI("[FPSGO_CTRL] gpu_display_update\n");

	if (!fpsgo_enable)
		return;

	fpsgo_ctrl2fstb_gpu_time_update(t_gpu, cur_freq, cur_max_freq);
}

#ifdef CONFIG_MTK_DYNAMIC_FPS_FRAMEWORK_SUPPORT
void dfrc_fps_limit_cb(int fps_limit)
{
	unsigned int vTmp = TARGET_UNLIMITED_FPS;
	struct FPSGO_NOTIFIER_PUSH_TAG *vpPush;

	if (!fpsgo_enable)
		return;

	if (fps_limit != DFRC_DRV_FPS_NON_ASSIGN)
		vTmp = fps_limit;

	FPSGO_LOGI("[FPSGO_CTRL] dfrc_fps %d\n", vTmp);

	vpPush =
		(struct FPSGO_NOTIFIER_PUSH_TAG *) fpsgo_alloc_atomic(sizeof(struct FPSGO_NOTIFIER_PUSH_TAG));

	if (!vpPush) {
		FPSGO_LOGE("[FPSGO_CTRL] OOM\n");
		return;
	}

	if (!g_psNotifyWorkQueue) {
		FPSGO_LOGE("[FPSGO_CTRL] NULL WorkQueue\n");
		fpsgo_free(vpPush, sizeof(struct FPSGO_NOTIFIER_PUSH_TAG));
		return;
	}

	vpPush->ePushType = FPSGO_NOTIFIER_DFRC_FPS;
	vpPush->dfrc_fps = vTmp;

	INIT_WORK(&vpPush->sWork, fpsgo_notifier_wq_cb);
	queue_work(g_psNotifyWorkQueue, &vpPush->sWork);
}
EXPORT_SYMBOL(dfrc_fps_limit_cb);
#endif

/* FPSGO control */
int fpsgo_is_enable(void)
{
	FPSGO_LOGI("[FPSGO_CTRL] isenable %d\n", fpsgo_enable);
	return fpsgo_enable;
}

void fpsgo_switch_enable(int enable)
{
	struct FPSGO_NOTIFIER_PUSH_TAG *vpPush =
		(struct FPSGO_NOTIFIER_PUSH_TAG *) fpsgo_alloc_atomic(sizeof(struct FPSGO_NOTIFIER_PUSH_TAG));

	if (!vpPush) {
		FPSGO_LOGE("[FPSGO_CTRL] OOM\n");
		return;
	}

	if (!g_psNotifyWorkQueue) {
		FPSGO_LOGE("[FPSGO_CTRL] NULL WorkQueue\n");
		fpsgo_free(vpPush, sizeof(struct FPSGO_NOTIFIER_PUSH_TAG));
		return;
	}

	FPSGO_LOGI("[FPSGO_CTRL] switch enable %d\n", enable);

	vpPush->ePushType = FPSGO_NOTIFIER_SWITCH_FPSGO;
	vpPush->enable = enable;

	INIT_WORK(&vpPush->sWork, fpsgo_notifier_wq_cb);
	queue_work(g_psNotifyWorkQueue, &vpPush->sWork);
}

/* FSTB control */
int fpsgo_is_fstb_enable(void)
{
	return is_fstb_enable();
}

int fpsgo_switch_fstb(int enable)
{
	return fpsgo_ctrl2fstb_switch_fstb(enable);
}

int fpsgo_fstb_sample_window(long long time_usec)
{
	return switch_sample_window(time_usec);
}

int fpsgo_fstb_fps_range(int nr_level, struct fps_level *level)
{
	return switch_fps_range(nr_level, level);
}

int fpsgo_fstb_dfps_ceiling(int fps)
{
	return switch_dfps_ceiling(fps);
}

int fpsgo_fstb_fps_error_threhosld(int threshold)
{
	return switch_fps_error_threhosld(threshold);
}

int fpsgo_fstb_percentile_frametime(int ratio)
{
	return switch_percentile_frametime(ratio);
}

static void fpsgo_exit(void)
{
	fpsgo_notifier_wq_cb_enable(0);

	if (g_psNotifyWorkQueue) {
		flush_workqueue(g_psNotifyWorkQueue);
		destroy_workqueue(g_psNotifyWorkQueue);
		g_psNotifyWorkQueue = NULL;
	}

	fbt_cpu_exit();
	mtk_fstb_exit();
	fpsgo_composer_exit();
}

static int __init fpsgo_init(void)
{
	FPSGO_LOGI("[FPSGO_CTRL] init\n");

	g_psNotifyWorkQueue = create_singlethread_workqueue("fpsgo_notifier_wq");

	if (g_psNotifyWorkQueue == NULL)
		return -EFAULT;

	mutex_init(&notify_lock);

	init_fpsgo_common();
	fbt_cpu_init();
	mtk_fstb_init();
	fpsgo_composer_init();

	fpsgo_switch_enable(1);

	cpufreq_notifier_fp = fpsgo_notify_cpufreq;
	display_time_fps_stablizer = fpsgo_notify_display_update;
	ged_kpi_output_gfx_info_fp = fpsgo_notify_gpu_display_update;

	ged_vsync_notifier_fp = fpsgo_notify_vsync;

	fpsgo_fbt2fstb_cpu_capability_fp = fpsgo_fbt2fstb_cpu_capability;

	fpsgo_notify_qudeq_fp = fpsgo_notify_qudeq;
	fpsgo_notify_intended_vsync_fp = fpsgo_notify_intended_vsync;
	fpsgo_notify_framecomplete_fp = fpsgo_notify_framecomplete;
	fpsgo_notify_connect_fp = fpsgo_notify_connect;

	return 0;
}

module_init(fpsgo_init);
module_exit(fpsgo_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MediaTek FPSGO");
MODULE_AUTHOR("MediaTek Inc.");
