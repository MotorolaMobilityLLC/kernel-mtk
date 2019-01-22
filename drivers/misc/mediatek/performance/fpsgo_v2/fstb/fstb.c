
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

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/err.h>
#include <linux/syscalls.h>
#include <linux/slab.h>
#include <linux/debugfs.h>
#include <linux/sort.h>
#include <linux/string.h>
#include <linux/average.h>
#include <linux/topology.h>
#include <linux/vmalloc.h>
#include <asm/div64.h>
#include <mt-plat/fpsgo_common.h>
#include "../fbt/include/fbt_cpu.h"
#include "../fbt/include/xgf.h"
#include "../../perf_ioctl/perf_ioctl.h"
#include "fstb.h"
#include "fstb_usedext.h"
/* fps update from display */
#ifdef CONFIG_MTK_FB
#include "disp_session.h"
#endif


#ifdef CONFIG_MTK_DYNAMIC_FPS_FRAMEWORK_SUPPORT
#include "dfrc.h"
#include "dfrc_drv.h"
#endif

#define mtk_fstb_dprintk_always(fmt, args...) \
	pr_debug("[FSTB]" fmt, ##args)

#define mtk_fstb_dprintk(fmt, args...) \
	do { \
		if (fstb_fps_klog_on == 1) \
		pr_debug("[FSTB]" fmt, ##args); \
	} while (0)

static void fstb_fps_stats(struct work_struct *work);
static DECLARE_WORK(fps_stats_work, (void *) fstb_fps_stats);
static HLIST_HEAD(fstb_frame_infos);

static struct hrtimer hrt;
static struct workqueue_struct *wq;

static int fstb_fps_klog_on;

static unsigned int fstb_fps_cur_limit;
static unsigned int tm_input_fps;
static unsigned int tm_queue_fps;
static struct dentry *fstb_debugfs_dir;

#define CFG_MAX_FPS_LIMIT	60
#define CFG_MIN_FPS_LIMIT	26

static int max_fps_limit = CFG_MAX_FPS_LIMIT;
static int dfps_ceiling = CFG_MAX_FPS_LIMIT;
static int min_fps_limit = CFG_MIN_FPS_LIMIT;

#define MAX_NR_FPS_LEVELS	1
static struct fps_level fps_levels[MAX_NR_FPS_LEVELS];
static int nr_fps_levels = MAX_NR_FPS_LEVELS;
#define DISPLAY_FPS_FILTER_NS 100000000ULL

/* in percentage */
static int fps_error_threshold = 10;
/* in round */
/* FPS is active when over stable tpcb or always */


static int QUANTILE = 75;
static long long FRAME_TIME_WINDOW_SIZE_US = 1000000;
static long long ADJUST_INTERVAL_US = 1000000;
int fstb_enable, fstb_active, fstb_idle_cnt;

static void reset_fps_level(void);
static int set_fps_level(int nr_level, struct fps_level *level);

static struct mutex fstb_lock;


static int calculate_fps_limit(int target_fps);

static void enable_fstb_timer(void)
{
	ktime_t ktime;

	ktime = ktime_set(0, ADJUST_INTERVAL_US * 1000);
	hrtimer_start(&hrt, ktime, HRTIMER_MODE_REL);
}

static void disable_fstb_timer(void)
{
	hrtimer_cancel(&hrt);
}

static enum hrtimer_restart mt_fstb(struct hrtimer *timer)
{
	if (wq)
		queue_work(wq, &fps_stats_work);

	return HRTIMER_NORESTART;
}


#define SECOND_CHANCE_CAPACITY 80
int second_chance_flag;

int is_fstb_enable(void)
{
	return fstb_enable;
}
int fpsgo_ctrl2fstb_switch_fstb(int enable)
{
	struct FSTB_FRAME_INFO *iter;
	struct hlist_node *t;

	mutex_lock(&fstb_lock);
	if (fstb_enable == enable) {
		mutex_unlock(&fstb_lock);
		return 0;
	}

	fstb_enable = enable;
	fpsgo_systrace_c_fstb(-200, fstb_enable, "fstb_enable");

	mtk_fstb_dprintk_always("%s %d\n", __func__, fstb_enable);
	if (!fstb_enable) {
		hlist_for_each_entry_safe(iter, t, &fstb_frame_infos, hlist) {
			hlist_del(&iter->hlist);
			vfree(iter);
		}
	} else {

		if (wq) {
			struct work_struct *psWork = kmalloc(sizeof(struct work_struct), GFP_ATOMIC);

			if (psWork) {
				INIT_WORK(psWork, fstb_fps_stats);
				queue_work(wq, psWork);
			}
		}
	}

	mutex_unlock(&fstb_lock);

	return 0;
}

static void switch_fstb_active(void)
{
	fpsgo_systrace_c_fstb(-200, fstb_active, "fstb_active");

	mtk_fstb_dprintk_always("%s %d\n", __func__, fstb_active);

		if (wq) {
			struct work_struct *psWork = kmalloc(sizeof(struct work_struct), GFP_ATOMIC);

			if (psWork) {
				INIT_WORK(psWork, fstb_fps_stats);
				queue_work(wq, psWork);
			}
		}
}

int switch_sample_window(long long time_usec)
{
	if (time_usec < 0 || time_usec > 1000000 * FRAME_TIME_BUFFER_SIZE / 120)
		return -EINVAL;

	FRAME_TIME_WINDOW_SIZE_US = ADJUST_INTERVAL_US = time_usec;

	return 0;
}

int switch_dfps_ceiling(int fps)
{
#ifdef CONFIG_MTK_DYNAMIC_FPS_FRAMEWORK_SUPPORT
	int ret = 0;

	mutex_lock(&fstb_lock);

	if (fps <= CFG_MAX_FPS_LIMIT && fps >= CFG_MIN_FPS_LIMIT) {
		ret = dfrc_set_kernel_policy(DFRC_DRV_API_LOADING,
				((fps != 60 && fstb_enable) ? fps : -1),
				DFRC_DRV_MODE_INTERNAL_SW, 0, 0);

		dfps_ceiling = fps;
		max_fps_limit = min(dfps_ceiling, fps_levels[0].start);
		min_fps_limit =	min(dfps_ceiling, fps_levels[nr_fps_levels - 1].end);

		mutex_unlock(&fstb_lock);
	} else {
		mutex_unlock(&fstb_lock);
		return -1;
	}

	return ret;
#endif
	return -1;

}

int switch_fps_range(int nr_level, struct fps_level *level)
{
	return set_fps_level(nr_level, level);
}

int switch_fps_error_threhosld(int threshold)
{
	if (threshold < 0 || threshold > 100)
		return -EINVAL;

	fps_error_threshold = threshold;

	return 0;
}

int switch_percentile_frametime(int ratio)
{
	if (ratio < 0 ||  ratio > 100)
		return -EINVAL;

	QUANTILE = ratio;

	return 0;
}

void (*update_lppcap)(unsigned int cap);
int fpsgo_comp2fstb_bypass(int pid)
{
	struct FSTB_FRAME_INFO *iter;

	mutex_lock(&fstb_lock);

	hlist_for_each_entry(iter, &fstb_frame_infos, hlist) {
		if (iter->pid == pid)
			break;
	}

	if (iter == NULL) {
		mutex_unlock(&fstb_lock);
		return 0;
	}

	if (iter->frame_type != BY_PASS_TYPE)
		iter->frame_type = BY_PASS_TYPE;

	mutex_unlock(&fstb_lock);
	return 0;
}

int fpsgo_fbt2fstb_reset_asfc(int pid, int level)
{
	struct FSTB_FRAME_INFO *iter;

	mutex_lock(&fstb_lock);

	hlist_for_each_entry(iter, &fstb_frame_infos, hlist) {
		if (iter->pid == pid)
			break;
	}

	if (iter == NULL) {
		mutex_unlock(&fstb_lock);
		return 0;
	}

	if (iter->asfc_flag) {
		iter->asfc_flag = 0;
		fpsgo_systrace_c_fstb(pid, iter->asfc_flag, "asfc_flag");
	} else {
		mutex_unlock(&fstb_lock);
		return 0;
	}

	mutex_unlock(&fstb_lock);

	return 0;
}

static int cmplonglong(const void *a, const void *b)
{
	return *(long long *)a - *(long long *)b;
}

int fpsgo_fbt2fstb_update_cpu_frame_info(
		int pid,
		int frame_type,
		unsigned long long Q2Q_time,
		unsigned long long Runnging_time,
		unsigned int Curr_cap,
		unsigned int Max_cap,
		unsigned int Target_fps)
{
	/* for porting*/
	long long frame_time_ns = (long long)Runnging_time;
	unsigned int max_current_cap = Curr_cap;
	unsigned int max_cpu_cap = Max_cap;

	struct FSTB_FRAME_INFO *iter;

	ktime_t cur_time;
	long long cur_time_us;

	mutex_lock(&fstb_lock);

	if (!fstb_enable) {
		mutex_unlock(&fstb_lock);
		return 0;
	}

	if (!fstb_active) {
		fstb_active = 1;
		switch_fstb_active();
	}

	hlist_for_each_entry(iter, &fstb_frame_infos, hlist) {
		if (iter->pid == pid)
			break;
	}

	if (iter == NULL) {
		struct FSTB_FRAME_INFO *new_frame_info;

		new_frame_info = vmalloc(sizeof(*new_frame_info));
		if (new_frame_info == NULL) {
			mutex_unlock(&fstb_lock);
			return 0;
		}

		new_frame_info->pid = pid;
		new_frame_info->target_fps = max_fps_limit;
		new_frame_info->queue_fps = CFG_MAX_FPS_LIMIT;
		new_frame_info->frame_type = -1;
		new_frame_info->render_method = -1;
		new_frame_info->asfc_flag = 0;
		new_frame_info->queue_time_begin = 0;
		new_frame_info->queue_time_end = 0;
		new_frame_info->weighted_cpu_time_begin = 0;
		new_frame_info->weighted_cpu_time_end = 0;
		new_frame_info->new_info = 1;
		iter = new_frame_info;
		hlist_add_head(&iter->hlist, &fstb_frame_infos);
	}

	mtk_fstb_dprintk("pid %d Q2Q_time %lld Runnging_time %lld Curr_cap %u Max_cap %u\n",
			pid, Q2Q_time, Runnging_time, Curr_cap, Max_cap);

	if (iter->weighted_cpu_time_begin < 0 ||
			iter->weighted_cpu_time_end < 0 ||
			iter->weighted_cpu_time_begin > iter->weighted_cpu_time_end ||
			iter->weighted_cpu_time_end >= FRAME_TIME_BUFFER_SIZE) {

		/* purge all data */
		iter->weighted_cpu_time_begin = iter->weighted_cpu_time_end = 0;
	}

	/*get current time*/
	cur_time = ktime_get();
	cur_time_us = ktime_to_us(cur_time);

	/*remove old entries*/
	while (iter->weighted_cpu_time_begin < iter->weighted_cpu_time_end) {
		if (iter->weighted_cpu_time_ts[iter->weighted_cpu_time_begin] < cur_time_us - FRAME_TIME_WINDOW_SIZE_US)
			iter->weighted_cpu_time_begin++;
		else
			break;
	}

	if (iter->weighted_cpu_time_begin == iter->weighted_cpu_time_end &&
			iter->weighted_cpu_time_end == FRAME_TIME_BUFFER_SIZE - 1)
		iter->weighted_cpu_time_begin = iter->weighted_cpu_time_end = 0;

	/*insert entries to weighted_cpu_time*/
	/*if buffer full --> move array align first*/
	if (iter->weighted_cpu_time_begin < iter->weighted_cpu_time_end &&
			iter->weighted_cpu_time_end == FRAME_TIME_BUFFER_SIZE - 1) {

		memmove(iter->weighted_cpu_time, &(iter->weighted_cpu_time[iter->weighted_cpu_time_begin]),
				sizeof(long long) * (iter->weighted_cpu_time_end - iter->weighted_cpu_time_begin));
		memmove(iter->weighted_cpu_time_ts, &(iter->weighted_cpu_time_ts[iter->weighted_cpu_time_begin]),
				sizeof(long long) * (iter->weighted_cpu_time_end - iter->weighted_cpu_time_begin));
		memmove(iter->q2q_time, &(iter->q2q_time[iter->weighted_cpu_time_begin]),
				sizeof(long long) * (iter->weighted_cpu_time_end - iter->weighted_cpu_time_begin));
		memmove(iter->cur_capacity, &(iter->cur_capacity[iter->weighted_cpu_time_begin]),
				sizeof(unsigned int) * (iter->weighted_cpu_time_end - iter->weighted_cpu_time_begin));

		/*reset index*/
		iter->weighted_cpu_time_end = iter->weighted_cpu_time_end - iter->weighted_cpu_time_begin;
		iter->weighted_cpu_time_begin = 0;
	}

	if (max_cpu_cap > 0 && Max_cap > Curr_cap) {
		iter->weighted_cpu_time[iter->weighted_cpu_time_end] = frame_time_ns * max_current_cap;
		do_div(iter->weighted_cpu_time[iter->weighted_cpu_time_end], max_cpu_cap);
	} else
		iter->weighted_cpu_time[iter->weighted_cpu_time_end] = frame_time_ns;

	iter->q2q_time[iter->weighted_cpu_time_end] = Q2Q_time;
	iter->weighted_cpu_time_ts[iter->weighted_cpu_time_end] = cur_time_us;
	iter->cur_capacity[iter->weighted_cpu_time_end] = max_current_cap;
	iter->weighted_cpu_time_end++;

	mutex_unlock(&fstb_lock);

	mtk_fstb_dprintk(
			"pid %d fstb: time %lld %lld frame_time_ns %lld max_current_cap %u max_cpu_cap %u\n",
			pid, cur_time_us, ktime_to_us(ktime_get())-cur_time_us,
			frame_time_ns, max_current_cap, max_cpu_cap);

	fpsgo_systrace_c_fstb(pid, (int)frame_time_ns, "t_cpu");
	fpsgo_systrace_c_fstb(pid, (int)max_current_cap, "cur_cpu_cap");
	fpsgo_systrace_c_fstb(pid, (int)max_cpu_cap, "max_cpu_cap");
	return -1;
}

static long long get_cpu_frame_time(struct FSTB_FRAME_INFO *iter)
{
	long long ret = INT_MAX;
	/*copy entries to temp array*/
	/*sort this array*/
	if (iter->weighted_cpu_time_end - iter->weighted_cpu_time_begin > 0 &&
			iter->weighted_cpu_time_end - iter->weighted_cpu_time_begin < FRAME_TIME_BUFFER_SIZE) {
		memcpy(iter->sorted_weighted_cpu_time, &(iter->weighted_cpu_time[iter->weighted_cpu_time_begin]),
				sizeof(long long) * (iter->weighted_cpu_time_end - iter->weighted_cpu_time_begin));
		sort(iter->sorted_weighted_cpu_time, iter->weighted_cpu_time_end - iter->weighted_cpu_time_begin,
				sizeof(long long), cmplonglong, NULL);
	}

	/*update nth value*/
	if (iter->weighted_cpu_time_end - iter->weighted_cpu_time_begin) {
		if (
			iter->sorted_weighted_cpu_time[
				QUANTILE*(iter->weighted_cpu_time_end-iter->weighted_cpu_time_begin)/100]
			> INT_MAX)
			ret = INT_MAX;
		else
			ret =
				iter->sorted_weighted_cpu_time[
					QUANTILE*(iter->weighted_cpu_time_end-iter->weighted_cpu_time_begin)/100];
	} else
		ret = -1;

	fpsgo_systrace_c_fstb(iter->pid, ret, "quantile_weighted_cpu_time");
	return ret;

}

static unsigned int get_cpu_cur_capacity(struct FSTB_FRAME_INFO *iter)
{
	int ret = 100;
	int i = 0;
	int arr_len = iter->weighted_cpu_time_end - iter->weighted_cpu_time_begin;
	unsigned int temp_capacity_sum = 0;

	if (arr_len > 0 && arr_len < FRAME_TIME_BUFFER_SIZE) {
		for (i = iter->weighted_cpu_time_begin; i < iter->weighted_cpu_time_end; i++)
			temp_capacity_sum += iter->cur_capacity[i];
		ret = temp_capacity_sum / arr_len;

		if (ret > 100)
			ret = 100;
	} else
		ret = 0;
	fpsgo_systrace_c_fstb(iter->pid, ret, "avg_capacity");
	return ret;

}

void fpsgo_comp2fstb_queue_time_update(int pid, int frame_type, int render_method, unsigned long long ts)
{
	struct FSTB_FRAME_INFO *iter;

	mutex_lock(&fstb_lock);

	if (!fstb_enable) {
		mutex_unlock(&fstb_lock);
		return;
	}

	if (!fstb_active) {
		fstb_active = 1;
		switch_fstb_active();
	}

	hlist_for_each_entry(iter, &fstb_frame_infos, hlist) {
		if (iter->pid == pid)
			break;
	}


	if (iter == NULL) {
		struct FSTB_FRAME_INFO *new_frame_info;

		new_frame_info = vmalloc(sizeof(*new_frame_info));
		if (new_frame_info == NULL) {
			mutex_unlock(&fstb_lock);
			return;
		}
		new_frame_info->pid = pid;
		new_frame_info->target_fps = max_fps_limit;
		new_frame_info->queue_fps = CFG_MAX_FPS_LIMIT;
		new_frame_info->frame_type = frame_type;
		new_frame_info->render_method = render_method;
		new_frame_info->asfc_flag = 0;
		new_frame_info->queue_time_begin = 0;
		new_frame_info->queue_time_end = 0;
		new_frame_info->weighted_cpu_time_begin = 0;
		new_frame_info->weighted_cpu_time_end = 0;
		new_frame_info->new_info = 1;
		iter = new_frame_info;
		hlist_add_head(&iter->hlist, &fstb_frame_infos);
	} else {
		iter->render_method = render_method;
		iter->frame_type = frame_type;
	}

	if (iter->queue_time_begin < 0 ||
			iter->queue_time_end < 0 ||
			iter->queue_time_begin > iter->queue_time_end ||
			iter->queue_time_end >= FRAME_TIME_BUFFER_SIZE) {
		/* purge all data */
		iter->queue_time_begin = iter->queue_time_end = 0;
	}

	/*remove old entries*/
	while (iter->queue_time_begin < iter->queue_time_end) {
		if (iter->queue_time_ts[iter->queue_time_begin] < ts - (long long)FRAME_TIME_WINDOW_SIZE_US * 1000)
			iter->queue_time_begin++;
		else
			break;
	}

	if (iter->queue_time_begin == iter->queue_time_end &&
			iter->queue_time_end == FRAME_TIME_BUFFER_SIZE - 1)
		iter->queue_time_begin = iter->queue_time_end = 0;

	/*insert entries to weighted_display_time*/
	/*if buffer full --> move array align first*/
	if (iter->queue_time_begin < iter->queue_time_end &&
			iter->queue_time_end == FRAME_TIME_BUFFER_SIZE - 1) {
		memmove(iter->queue_time_ts, &(iter->queue_time_ts[iter->queue_time_begin]),
				sizeof(unsigned long long) * (iter->queue_time_end - iter->queue_time_begin));
		/*reset index*/
		iter->queue_time_end = iter->queue_time_end - iter->queue_time_begin;
		iter->queue_time_begin = 0;
	}

	iter->queue_time_ts[iter->queue_time_end] = ts;
	iter->queue_time_end++;

	mutex_unlock(&fstb_lock);
}

static int fstb_get_queue_fps(struct FSTB_FRAME_INFO *iter, long long interval)
{
	int i = iter->queue_time_begin, j;
	long long queue_fps;
	unsigned long long frame_interval_count = 0;
	unsigned long long avg_frame_interval = 0;
	unsigned long long retval = 0;

	/* remove old entries */
	while (i < iter->queue_time_end) {
		if (iter->queue_time_ts[i] < sched_clock() - interval * 1000)
			i++;
		else
			break;
	}

	/* filter */
	for (j = i + 1; j < iter->queue_time_end; j++) {
		if ((iter->queue_time_ts[j] - iter->queue_time_ts[j - 1]) < DISPLAY_FPS_FILTER_NS) {
			avg_frame_interval += (iter->queue_time_ts[j] - iter->queue_time_ts[j - 1]);
			frame_interval_count++;
		}
	}

	queue_fps = (long long)(iter->queue_time_end - i) * 1000000LL;
	do_div(queue_fps, interval);

	if (avg_frame_interval != 0) {
		retval = 1000000000ULL * frame_interval_count;
		do_div(retval, avg_frame_interval);
		mtk_fstb_dprintk_always("%s  %d %llu\n", __func__, iter->pid, retval);
		fpsgo_systrace_c_fstb(iter->pid, (int)retval, "queue_fps");
		return retval;
	}
	mtk_fstb_dprintk_always("%s  %d %d\n", __func__, iter->pid, 0);
	fpsgo_systrace_c_fstb(iter->pid, 0, "queue_fps");

	return 0;
}

static int fps_update(struct FSTB_FRAME_INFO *iter)
{
	iter->queue_fps = fstb_get_queue_fps(iter, FRAME_TIME_WINDOW_SIZE_US);
	fpsgo_systrace_c_fstb(iter->pid, tm_queue_fps, "tm_queue_fps");

	/*tm_input_fps = get_display_fps(FRAME_TIME_WINDOW_SIZE_US);*/
	tm_input_fps = max_fps_limit;
	return iter->queue_fps;
}

/* Calculate FPS limit:
 * @retval new fps limit
 *
 * search in ascending order
 * For discrete range: same as before, we select the upper one level that
 *                     is just larger than current target.
 * For contiguous range: if the new limit is between [start,end], use new limit
 */
static int calculate_fps_limit(int target_fps)
{
	if (target_fps >= max_fps_limit)
		return max_fps_limit;

	if (target_fps <= min_fps_limit)
		return min_fps_limit;

	return target_fps;
}

static int cur_cpu_time;

static int cal_target_fps(struct FSTB_FRAME_INFO *iter)
{
	long long target_limit = 60;
	long long tmp_target_limit = 60;
	int cur_cap;

	cur_cpu_time = get_cpu_frame_time(iter);
	cur_cap = get_cpu_cur_capacity(iter);

	if (iter->new_info == 1) {
		iter->new_info = 0;
		target_limit = max_fps_limit;
		iter->asfc_flag = 0;
		fpsgo_systrace_c_fstb(iter->pid, iter->asfc_flag, "asfc_flag");
		second_chance_flag = 0;
		fpsgo_systrace_c_fstb(iter->pid, second_chance_flag, "second_chance_flag");
		/*decrease*/
	} else if (iter->target_fps - iter->queue_fps > iter->target_fps * fps_error_threshold / 100) {
		if (cur_cap > SECOND_CHANCE_CAPACITY || second_chance_flag == 1) {
			if (iter->queue_fps < iter->target_fps)
				target_limit = iter->queue_fps;
			else
				target_limit = iter->target_fps;
			second_chance_flag = 0;
		} else {
			second_chance_flag = 1;
			target_limit = iter->target_fps;
		}
		fpsgo_systrace_c_fstb(iter->pid, (int)target_limit, "tmp_target_limit");
		fpsgo_systrace_c_fstb(iter->pid, second_chance_flag, "second_chance_flag");
		/*increase*/
	} else if (iter->target_fps - iter->queue_fps <= 1) {

		second_chance_flag = 0;
		fpsgo_systrace_c_fstb(iter->pid, second_chance_flag, "second_chance_flag");
		tmp_target_limit = 1000000000LL;
		do_div(tmp_target_limit, (long long)cur_cpu_time);
		fpsgo_systrace_c_fstb(iter->pid, (int)tmp_target_limit, "tmp_target_limit");

		if ((int)tmp_target_limit - iter->target_fps > iter->target_fps * fps_error_threshold / 100) {
			if (iter->target_fps >= 48)
				target_limit = tmp_target_limit;
			else if (iter->target_fps >= 45)
				target_limit = 48;
			else if (iter->target_fps >= 36)
				target_limit = 45;
			else if (iter->target_fps > 30)
				target_limit = 36;
			else if (iter->target_fps == 30)
				target_limit = 60;
			else if (iter->target_fps >= 27)
				target_limit = 30;
			else if (iter->target_fps >= 24)
				target_limit = 27;
			else
				target_limit = tmp_target_limit;

			if (iter->asfc_flag == 1 && iter->queue_fps >= 32)
				iter->asfc_flag = 0;
		} else
			target_limit = iter->target_fps;
		/*stable state*/
	} else {
		second_chance_flag = 0;
		fpsgo_systrace_c_fstb(iter->pid, second_chance_flag, "second_chance_flag");
		target_limit = iter->target_fps;
	}

	if (iter->asfc_flag && target_limit > 30)
		target_limit = 30;


	if (iter->target_fps == 30) {
		iter->asfc_flag = 1;
		fpsgo_systrace_c_fstb(iter->pid, iter->asfc_flag, "asfc_flag");
	}

	return target_limit;

}


static void fstb_fps_stats(struct work_struct *work)
{
	struct FSTB_FRAME_INFO *iter;
	struct hlist_node *n;
	int target_fps = max_fps_limit;
	int idle = 1;

	if (work != &fps_stats_work)
		kfree(work);

	mutex_lock(&fstb_lock);

	hlist_for_each_entry_safe(iter, n, &fstb_frame_infos, hlist) {
		/* if this process did queue buffer while last polling window */
		if (fps_update(iter)) {

			idle = 0;

			switch (iter->frame_type) {
			case BY_PASS_TYPE:
				target_fps = max_fps_limit;
				break;
			case VSYNC_ALIGNED_TYPE:
				if (iter->render_method == GLSURFACE)
					target_fps = cal_target_fps(iter);
				else
					target_fps = max_fps_limit;
				break;
			case NON_VSYNC_ALIGNED_TYPE:
				target_fps = cal_target_fps(iter);
				break;
			default:
				break;
			}

			mtk_fstb_dprintk_always("%s pid:%d target_fps:%d frame_type %d\n",
					__func__, iter->pid, iter->target_fps, iter->frame_type);
			iter->target_fps = calculate_fps_limit(target_fps);
			fpsgo_fstb2fbt_target_fps(iter->pid, iter->target_fps, iter->queue_fps);
			/* if queue fps == 0, we delete that frame_info */
		} else {
			hlist_del(&iter->hlist);
			vfree(iter);
		}
	}

	/* check idle twice to avoid fstb_active ping-pong */
	if (idle)
		fstb_idle_cnt++;
	else
		fstb_idle_cnt = 0;

	if (fstb_idle_cnt >= 2) {
		fstb_active = 0;
		fstb_idle_cnt = 0;
	}

	if (!fstb_active)
		fpsgo_fstb2xgf_set_idle();

	if (fstb_enable && fstb_active)
		enable_fstb_timer();
	else
		disable_fstb_timer();

	mutex_unlock(&fstb_lock);

	fbt_check_thread_status();
}

static int set_fps_level(int nr_level, struct fps_level *level)
{
	int i;

	mutex_lock(&fstb_lock);

	if (nr_level > MAX_NR_FPS_LEVELS)
		goto set_fps_level_err;

	for (i = 0; i < nr_level; i++) {
		/* check if they are interleaving */
		if (level[i].end > level[i].start ||
				(i > 0 && level[i].start > level[i - 1].end))
			goto set_fps_level_err;
	}

	memcpy(fps_levels, level, nr_level * sizeof(struct fps_level));

	nr_fps_levels = nr_level;
	max_fps_limit = min(dfps_ceiling, fps_levels[0].start);
	min_fps_limit = min(dfps_ceiling, fps_levels[nr_fps_levels - 1].end);

	mutex_unlock(&fstb_lock);

	return 0;
set_fps_level_err:
	mutex_unlock(&fstb_lock);
	return -EINVAL;
}

static void reset_fps_level(void)
{
	struct fps_level level[1];

	level[0].start = CFG_MAX_FPS_LIMIT;
	level[0].end = CFG_MIN_FPS_LIMIT;

	set_fps_level(1, level);
}

static int fstb_level_read(struct seq_file *m, void *v)
{
	int i;

	seq_printf(m, "%d ", nr_fps_levels);
	for (i = 0; i < nr_fps_levels; i++)
		seq_printf(m, "%d-%d ", fps_levels[i].start, fps_levels[i].end);
	seq_puts(m, "\n");

	return 0;
}

/* format example: 4 60-45 30-30 20-20 15-15
 * compatible: 4 60 30 20 15
 * mixed: 4 60-45 30 20 15
 */
static ssize_t fstb_level_write(struct file *file, const char __user *buffer,
		size_t count, loff_t *data)
{
	char *buf, *sepstr, *substr;
	int ret = -EINVAL, new_nr_fps_levels, i, start_fps, end_fps;
	struct fps_level *new_levels;

	/* we do not allow change fps_level during fps throttling,
	 * because fps_levels would be changed.
	 */

	buf = kmalloc(count + 1, GFP_KERNEL);
	if (buf == NULL)
		return -ENOMEM;

	new_levels = kmalloc(sizeof(fps_levels), GFP_KERNEL);
	if (new_levels == NULL) {
		ret = -ENOMEM;
		goto err_freebuf;
	}

	if (copy_from_user(buf, buffer, count)) {
		ret = -EFAULT;
		goto err;
	}
	buf[count] = '\0';
	sepstr = buf;

	substr = strsep(&sepstr, " ");
	if (kstrtoint(substr, 10, &new_nr_fps_levels) != 0 || new_nr_fps_levels > MAX_NR_FPS_LEVELS) {
		ret = -EINVAL;
		goto err;
	}

	for (i = 0; i < new_nr_fps_levels; i++) {
		substr = strsep(&sepstr, " ");
		if (!substr) {
			ret = -EINVAL;
			goto err;
		}
		if (strchr(substr, '-')) { /* maybe contiguous */
			if (sscanf(substr, "%d-%d", &start_fps, &end_fps) != 2) {
				ret = -EINVAL;
				goto err;
			}
			new_levels[i].start = start_fps;
			new_levels[i].end = end_fps;
		} else { /* discrete */
			if (kstrtoint(substr, 10, &start_fps) != 0) {
				ret = -EINVAL;
				goto err;
			}
			new_levels[i].start = start_fps;
			new_levels[i].end = start_fps;
		}
	}

	if (set_fps_level(new_nr_fps_levels, new_levels) != 0)
		goto err;

	ret = count;
err:
	kfree(new_levels);
err_freebuf:
	kfree(buf);

	return ret;
}

static int fstb_level_open(struct inode *inode, struct file *file)
{
	return single_open(file, fstb_level_read, NULL);
}

static const struct file_operations fstb_level_fops = {
	.owner = THIS_MODULE,
	.open = fstb_level_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = fstb_level_write,
	.release = single_release,
};

static int fstb_tune_window_size_read(struct seq_file *m, void *v)
{
	seq_printf(m, "%lld ", FRAME_TIME_WINDOW_SIZE_US);
	return 0;
}

static ssize_t fstb_tune_window_size_write(struct file *file, const char __user *buffer,
		size_t count, loff_t *data)
{
	int ret;
	long long arg;

	if (!kstrtoll_from_user(buffer, count, 0, &arg))
		ret = switch_sample_window(arg);
	else
		ret = -EINVAL;

	return (ret < 0) ? ret : count;
}

static int fstb_tune_window_size_open(struct inode *inode, struct file *file)
{
	return single_open(file, fstb_tune_window_size_read, NULL);
}

static const struct file_operations fstb_tune_window_size_fops = {
	.owner = THIS_MODULE,
	.open = fstb_tune_window_size_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = fstb_tune_window_size_write,
	.release = single_release,
};

static int fstb_tune_dfps_ceiling_read(struct seq_file *m, void *v)
{
	seq_printf(m, "%d ", dfps_ceiling);
	return 0;
}

static ssize_t fstb_tune_dfps_ceiling_write(struct file *file, const char __user *buffer,
		size_t count, loff_t *data)
{
	int ret;
	int arg;

	if (!kstrtoint_from_user(buffer, count, 0, &arg))
		ret = switch_dfps_ceiling(arg);
	else
		ret = -EINVAL;

	return (ret < 0) ? ret : count;
}

static int fstb_tune_dfps_ceiling_open(struct inode *inode, struct file *file)
{
	return single_open(file, fstb_tune_dfps_ceiling_read, NULL);
}

static const struct file_operations fstb_tune_dfps_ceiling_fops = {
	.owner = THIS_MODULE,
	.open = fstb_tune_dfps_ceiling_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = fstb_tune_dfps_ceiling_write,
	.release = single_release,
};

static int fstb_tune_quantile_read(struct seq_file *m, void *v)
{
	seq_printf(m, "%d ", QUANTILE);
	return 0;
}

static ssize_t fstb_tune_quantile_write(struct file *file, const char __user *buffer,
		size_t count, loff_t *data)
{
	int ret;
	int arg;

	if (!kstrtoint_from_user(buffer, count, 0, &arg))
		ret = switch_percentile_frametime(arg);
	else
		ret = -EINVAL;

	return (ret < 0) ? ret : count;
}

static int fstb_tune_quantile_open(struct inode *inode, struct file *file)
{
	return single_open(file, fstb_tune_quantile_read, NULL);
}

static const struct file_operations fstb_tune_quantile_fops = {
	.owner = THIS_MODULE,
	.open = fstb_tune_quantile_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = fstb_tune_quantile_write,
	.release = single_release,
};

static int fstb_tune_error_threshold_read(struct seq_file *m, void *v)
{
	seq_printf(m, "%d ", fps_error_threshold);
	return 0;
}

static ssize_t fstb_tune_error_threshold_write(struct file *file, const char __user *buffer,
		size_t count, loff_t *data)
{
	int ret;
	int arg;

	if (!kstrtoint_from_user(buffer, count, 0, &arg))
		ret = switch_fps_error_threhosld(arg);
	else
		ret = -EINVAL;

	return (ret < 0) ? ret : count;
}

static int fstb_tune_error_threshold_open(struct inode *inode, struct file *file)
{
	return single_open(file, fstb_tune_error_threshold_read, NULL);
}

static const struct file_operations fstb_tune_error_threshold_fops = {
	.owner = THIS_MODULE,
	.open = fstb_tune_error_threshold_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = fstb_tune_error_threshold_write,
	.release = single_release,
};

static int mtk_fstb_fps_proc_read(struct seq_file *m, void *v)
{
	/**
	 * The format to print out:
	 *  fstb_enable <0 or 1>
	 *  kernel_log <0 or 1>
	 */
	seq_printf(m, "fstb_enable %d\n", fstb_enable);
	seq_printf(m, "fstb_active %d\n", fstb_active);
	seq_printf(m, "fstb_idle_cnt %d\n", fstb_idle_cnt);
	seq_printf(m, "klog %d\n", fstb_fps_klog_on);

	return 0;
}

static ssize_t mtk_fstb_fps_proc_write(struct file *filp, const char __user *buffer, size_t count, loff_t *data)
{
	int len = 0;
	char desc[128];
	int k_enable, klog_on;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	/**
	 * sscanf format <fstb_enable> <klog_on>
	 * <klog_on> can only be 0 or 1
	 */

	if (data == NULL) {
		mtk_fstb_dprintk("[%s] null data\n", __func__);
		return -EINVAL;
	}

	if (sscanf(desc, "%d %d", &k_enable, &klog_on) >= 1) {
		if (k_enable == 0 || k_enable == 1)
			fpsgo_ctrl2fstb_switch_fstb(k_enable);

		if (klog_on == 0 || klog_on == 1)
			fstb_fps_klog_on = klog_on;

		return count;
	}

	mtk_fstb_dprintk("[%s] bad arg\n", __func__);
	return -EINVAL;
}

static int mtk_fstb_fps_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, mtk_fstb_fps_proc_read, NULL);
}

static const struct file_operations fstb_fps_fops = {
	.owner = THIS_MODULE,
	.open = mtk_fstb_fps_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = mtk_fstb_fps_proc_write,
	.release = single_release,
};

static ssize_t fstb_count_write(struct file *filp, const char __user *buf, size_t len, loff_t *data)
{
	char tmp[32] = {0};

	len = (len < (sizeof(tmp) - 1)) ? len : (sizeof(tmp) - 1);
	return len;
}

static int fstb_count_read(struct seq_file *m, void *v)
{
	struct disp_session_info info;


	memset(&info, 0, sizeof(info));
	info.session_id = MAKE_DISP_SESSION(DISP_SESSION_PRIMARY, 0);

	disp_mgr_get_session_info(&info);

	seq_printf(m, "%d,%d,%d,%d\n",
			info.updateFPS / 100, tm_input_fps, fstb_fps_cur_limit,
			cur_cpu_time);

	mtk_fstb_dprintk("[%s] %d\n", __func__, tm_input_fps);

	return 0;
}

static int fstb_count_open(struct inode *inode, struct file *file)
{
	return single_open(file, fstb_count_read, NULL);
}

static const struct file_operations tm_fps_fops = {
	.owner = THIS_MODULE,
	.open = fstb_count_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = fstb_count_write,
	.release = single_release,
};

static ssize_t fpsgo_status_write(struct file *filp, const char __user *buf, size_t len, loff_t *data)
{
	char tmp[32] = {0};

	len = (len < (sizeof(tmp) - 1)) ? len : (sizeof(tmp) - 1);
	return len;
}

static int fpsgo_status_read(struct seq_file *m, void *v)
{
	struct FSTB_FRAME_INFO *iter;
	struct task_struct *tsk, *gtsk;

	mutex_lock(&fstb_lock);

	if (!fstb_enable) {
		mutex_unlock(&fstb_lock);
		return 0;
	}

	seq_puts(m, "tid\tname\t\tframeType\trenderMethod\tcurrentFPS\ttargetFPS\n");

	hlist_for_each_entry(iter, &fstb_frame_infos, hlist) {
		rcu_read_lock();
		tsk = find_task_by_vpid(iter->pid);
		if (tsk) {
			get_task_struct(tsk);
			gtsk = find_task_by_vpid(tsk->tgid);
			put_task_struct(tsk);
			if (gtsk)
				get_task_struct(gtsk);
			else {
				rcu_read_unlock();
				continue;
			}
		} else {
			rcu_read_unlock();
			continue;
		}
		rcu_read_unlock();

		seq_printf(m, "%d\t", iter->pid);
		seq_printf(m, "%s\t", gtsk->comm);
		put_task_struct(gtsk);
		seq_printf(m, "%d\t\t", iter->frame_type);
		seq_printf(m, "%d\t\t", iter->frame_type != BY_PASS_TYPE ? iter->render_method : -1);
		seq_printf(m, "%d\t\t", iter->queue_fps > CFG_MAX_FPS_LIMIT ? CFG_MAX_FPS_LIMIT : iter->queue_fps);
		seq_printf(m, "%d\t", iter->target_fps);
		seq_puts(m, "\n");

	}

	mutex_unlock(&fstb_lock);

	return 0;
}

static int fpsgo_status_open(struct inode *inode, struct file *file)
{
	return single_open(file, fpsgo_status_read, NULL);
}

static const struct file_operations fpsgo_status_fops = {
	.owner = THIS_MODULE,
	.open = fpsgo_status_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = fpsgo_status_write,
	.release = single_release,
};

int mtk_fstb_init(void)
{
	int num_cluster = 0;

	mtk_fstb_dprintk_always("init\n");

	num_cluster = arch_get_nr_clusters();

	mutex_init(&fstb_lock);


	/* create debugfs file */
	if (!fpsgo_debugfs_dir)
		goto err;

	fstb_debugfs_dir = debugfs_create_dir("fstb",
			fpsgo_debugfs_dir);
	if (!fstb_debugfs_dir) {
		mtk_fstb_dprintk_always(
				"[%s]: mkdir /sys/kernel/debug/fpsgo/fstb failed\n",
				__func__);
		goto err;
	}

	debugfs_create_file("fpsgo_status",
			S_IRUGO | S_IWUSR | S_IWGRP,
			fstb_debugfs_dir,
			NULL,
			&fpsgo_status_fops);

	debugfs_create_file("fps_count",
			S_IRUGO | S_IWUSR | S_IWGRP,
			fstb_debugfs_dir,
			NULL,
			&tm_fps_fops);

	debugfs_create_file("fstb_debug",
			S_IRUGO | S_IWUSR | S_IWGRP,
			fstb_debugfs_dir,
			NULL,
			&fstb_fps_fops);

	debugfs_create_file("fstb_level",
			S_IRUGO | S_IWUSR | S_IWGRP,
			fstb_debugfs_dir,
			NULL,
			&fstb_level_fops);

	debugfs_create_file("fstb_tune_error_threshold",
			S_IRUGO | S_IWUSR | S_IWGRP,
			fstb_debugfs_dir,
			NULL,
			&fstb_tune_error_threshold_fops);

	debugfs_create_file("fstb_tune_quantile",
			S_IRUGO | S_IWUSR | S_IWGRP,
			fstb_debugfs_dir,
			NULL,
			&fstb_tune_quantile_fops);

	debugfs_create_file("fstb_tune_window_size",
			S_IRUGO | S_IWUSR | S_IWGRP,
			fstb_debugfs_dir,
			NULL,
			&fstb_tune_window_size_fops);

	debugfs_create_file("fstb_tune_dfps_ceiling",
			S_IRUGO | S_IWUSR | S_IWGRP,
			fstb_debugfs_dir,
			NULL,
			&fstb_tune_dfps_ceiling_fops);

	reset_fps_level();

	wq = create_singlethread_workqueue("mt_fstb");
	if (!wq)
		goto err;

	hrtimer_init(&hrt, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hrt.function = &mt_fstb;

	mtk_fstb_dprintk_always("init done\n");

	return 0;

err:
	return -1;
}

int mtk_fstb_exit(void)
{
	mtk_fstb_dprintk("exit\n");

	disable_fstb_timer();

	/* remove the debugfs file */
	debugfs_remove_recursive(fstb_debugfs_dir);
	return 0;
}
