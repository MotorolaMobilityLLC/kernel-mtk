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
#include <asm/div64.h>
#include <mt-plat/fpsgo_common.h>
#include "fstb.h"
#include "fstb_usedext.h"
/* fps update from display */
#ifdef CONFIG_MTK_FB
#include "disp_session.h"
#endif
#ifdef CONFIG_MTK_GPU_SUPPORT
#include "ged_dvfs.h"
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
static int min_fps_limit = CFG_MIN_FPS_LIMIT;

#define MAX_NR_FPS_LEVELS	8
static struct fps_level fps_levels[MAX_NR_FPS_LEVELS];
static int nr_fps_levels = MAX_NR_FPS_LEVELS;

/* in percentage */
static int fps_error_threshold = 10;
/* in round */
/* FPS is active when over stable tpcb or always */
static int in_game_mode;
static int fstb_reset;
static int fstb_check_30_fps;

#define FPS_STABILIZER

#define FRAME_TIME_BUFFER_SIZE 1200
static int QUANTILE = 75;
static long long FRAME_TIME_WINDOW_SIZE_US = 1000000;
static long long ADJUST_INTERVAL_US = 1000000;
int fstb_enable = 1;
int asfc_flag;

static void reset_fps_level(void);
static int set_fps_level(int nr_level, struct fps_level *level);

static long long q2q_time[FRAME_TIME_BUFFER_SIZE];
static long long weighted_cpu_time[FRAME_TIME_BUFFER_SIZE];
static long long weighted_cpu_time_ts[FRAME_TIME_BUFFER_SIZE];
static int weighted_cpu_time_begin,  weighted_cpu_time_end;
static long long sorted_weighted_cpu_time[FRAME_TIME_BUFFER_SIZE];
static struct mutex fstb_lock;

static unsigned long long queue_time_ts[FRAME_TIME_BUFFER_SIZE]; /*timestamp*/
static int queue_time_begin, queue_time_end;

static int lppfps;
static int lpp_mode;
static unsigned int lpp_freq = 1200000;
static int last_lpp_mode;
static unsigned long long cpu_loading[FRAME_TIME_BUFFER_SIZE];
static unsigned long long sorted_cpu_loading[FRAME_TIME_BUFFER_SIZE];
static int lpp_fps_budget;
static unsigned int max_freq;
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
static unsigned int cur_capacity[FRAME_TIME_BUFFER_SIZE];
int second_chance_flag;

static int vag_flag;
static int force_vag;
static int vag_fps = 30;

int is_fstb_enable(void)
{
	return fstb_enable;
}
int switch_fstb(int enable)
{
	mutex_lock(&fstb_lock);
	if (fstb_enable == enable) {
		mutex_unlock(&fstb_lock);
		return 0;
	}

	fstb_enable = enable;
	fstb_reset = 1;
	fpsgo_systrace_c_log(fstb_enable, "fstb_enable");
	fpsgo_systrace_c_log(fstb_reset, "fstb_reset");

	if (wq) {
		struct work_struct *psWork = kmalloc(sizeof(struct work_struct), GFP_ATOMIC);

		if (psWork) {
			INIT_WORK(psWork, fstb_fps_stats);
			queue_work(wq, psWork);
		}
	}

	mutex_unlock(&fstb_lock);

	return 0;
}

int switch_sample_window(long long time_usec)
{
	if (time_usec < 0 || time_usec > 1000000 * FRAME_TIME_BUFFER_SIZE / 120)
		return -EINVAL;

	FRAME_TIME_WINDOW_SIZE_US = ADJUST_INTERVAL_US = time_usec;

	return 0;
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

int switch_force_vag(int arg)
{
	if (arg == 1) {
		force_vag = 1;
		dfrc_set_kernel_policy(DFRC_DRV_API_LOADING, -1, DFRC_DRV_MODE_INTERNAL_SW, 0, 0);
	} else if (arg == 0) {
		force_vag = 0;
	} else
		return -EINVAL;

	return 0;
}

int switch_vag_fps(int arg)
{
	if (arg <= 60 && arg >= 20)
		vag_fps = arg;
	else
		vag_fps = 30;

	return 0;
}

int switch_lpp_fps(int arg)
{
		if (arg == -1) {
			last_lpp_mode = lpp_mode;
			lpp_mode = 0;
			lppfps = CFG_MAX_FPS_LIMIT;
			lpp_fps_budget = 0;
		} else if (arg > CFG_MIN_FPS_LIMIT && arg < CFG_MAX_FPS_LIMIT) {
			last_lpp_mode = lpp_mode;
			lpp_mode = 1;
			lppfps = arg;
			lpp_fps_budget = 0;
			dfrc_set_kernel_policy(DFRC_DRV_API_LOADING, -1, DFRC_DRV_MODE_INTERNAL_SW, 0, 0);
		} else
			return -EINVAL;

		return 0;
}

int switch_lpp_freq(unsigned int arg)
{
	if (arg > max_freq)
		return -EINVAL;

	lpp_freq = arg;

	return 0;
}

int fbt_reset_asfc(int level)
{
	mutex_lock(&fstb_lock);
	if (asfc_flag) {
		asfc_flag = 0;
		fstb_reset = 1;
		fpsgo_systrace_c_fstb(-200, asfc_flag, "asfc_flag");
	} else {
		mutex_unlock(&fstb_lock);
		return 0;
	}

	if (in_game_mode && fstb_enable) {
		fpsgo_systrace_c_fstb(-200, 1, "reset_asfc");

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

#define FPS30_CHECK_NR_FRAME 5
#define FPS30_CHECK_FRAME_TIME_LOW 31000000LL
#define FPS30_CHECK_FRAME_TIME_HIGH 35000000LL

static int is_30_fps(void)
{
	int i;
	int retval = 1;

	if (weighted_cpu_time_end - weighted_cpu_time_begin > 0 &&
			weighted_cpu_time_end - weighted_cpu_time_begin < FRAME_TIME_BUFFER_SIZE &&
			weighted_cpu_time_end - FPS30_CHECK_NR_FRAME >= 0) {
		for (i = weighted_cpu_time_end - FPS30_CHECK_NR_FRAME; i < weighted_cpu_time_end; i++)
			if (q2q_time[i] < FPS30_CHECK_FRAME_TIME_LOW || q2q_time[i] > FPS30_CHECK_FRAME_TIME_HIGH) {
				retval = 0;
				break;
			}
	}

	return retval;
}

void fstb_game_mode_change(int is_game)
{
	mutex_lock(&fstb_lock);

	if (in_game_mode == is_game) {
		mutex_unlock(&fstb_lock);
		return;
	}

	in_game_mode = is_game;
	fstb_reset = 1;
	fpsgo_systrace_c_fstb(-200, fstb_reset, "fstb_reset");

	if (wq) {
		struct work_struct *psWork = kmalloc(sizeof(struct work_struct), GFP_ATOMIC);

		if (psWork) {
			INIT_WORK(psWork, fstb_fps_stats);
			queue_work(wq, psWork);
		}
	}

	mutex_unlock(&fstb_lock);
}

static int cmplonglong(const void *a, const void *b)
{
	return *(long long *)a - *(long long *)b;
}

int update_cpu_frame_info(
	unsigned long long Q2Q_time,
	unsigned long long Runnging_time,
	unsigned int Curr_cap,
	unsigned int Max_cap,
	unsigned int Target_fps)
{
#ifdef FPS_STABILIZER
	/* for porting*/
	long long frame_time_ns = (long long)Runnging_time;
	unsigned int max_current_cap = Curr_cap;
	unsigned int max_cpu_cap = Max_cap;

	ktime_t cur_time;
	long long cur_time_us;

	mutex_lock(&fstb_lock);

	if (!in_game_mode || !fstb_enable) {
		mutex_unlock(&fstb_lock);
		return 0;
	}

	mtk_fstb_dprintk("Q2Q_time %lld Runnging_time %lld Curr_cap %u Max_cap %u\n",
			Q2Q_time, Runnging_time, Curr_cap, Max_cap);

	if (weighted_cpu_time_begin < 0 ||
		weighted_cpu_time_end < 0 ||
		weighted_cpu_time_begin > weighted_cpu_time_end ||
		weighted_cpu_time_end >= FRAME_TIME_BUFFER_SIZE) {

		/* purge all data */
		weighted_cpu_time_begin = weighted_cpu_time_end = 0;
	}

	/*get current time*/
	cur_time = ktime_get();
	cur_time_us = ktime_to_us(cur_time);

	/*remove old entries*/
	while (weighted_cpu_time_begin < weighted_cpu_time_end) {
		if (weighted_cpu_time_ts[weighted_cpu_time_begin] < cur_time_us - FRAME_TIME_WINDOW_SIZE_US)
			weighted_cpu_time_begin++;
		else
			break;
	}

	if (weighted_cpu_time_begin == weighted_cpu_time_end &&
		weighted_cpu_time_end == FRAME_TIME_BUFFER_SIZE - 1)
		weighted_cpu_time_begin = weighted_cpu_time_end = 0;

	/*insert entries to weighted_cpu_time*/
	/*if buffer full --> move array align first*/
	if (weighted_cpu_time_begin < weighted_cpu_time_end &&
		weighted_cpu_time_end == FRAME_TIME_BUFFER_SIZE - 1) {

		memmove(weighted_cpu_time, &weighted_cpu_time[weighted_cpu_time_begin],
				sizeof(long long) * (weighted_cpu_time_end - weighted_cpu_time_begin));
		memmove(weighted_cpu_time_ts, &weighted_cpu_time_ts[weighted_cpu_time_begin],
				sizeof(long long) * (weighted_cpu_time_end - weighted_cpu_time_begin));
		memmove(q2q_time, &q2q_time[weighted_cpu_time_begin],
				sizeof(long long) * (weighted_cpu_time_end - weighted_cpu_time_begin));
		memmove(cur_capacity, &cur_capacity[weighted_cpu_time_begin],
				sizeof(unsigned int) * (weighted_cpu_time_end - weighted_cpu_time_begin));
		memmove(cpu_loading, &cpu_loading[weighted_cpu_time_begin],
				sizeof(long long) * (weighted_cpu_time_end - weighted_cpu_time_begin));

		/*reset index*/
		weighted_cpu_time_end = weighted_cpu_time_end - weighted_cpu_time_begin;
		weighted_cpu_time_begin = 0;
	}

	if (max_cpu_cap > 0 && Max_cap > Curr_cap) {
		weighted_cpu_time[weighted_cpu_time_end] = frame_time_ns * max_current_cap;
		do_div(weighted_cpu_time[weighted_cpu_time_end], max_cpu_cap);
	} else
		weighted_cpu_time[weighted_cpu_time_end] = frame_time_ns;

	q2q_time[weighted_cpu_time_end] = Q2Q_time;
	weighted_cpu_time_ts[weighted_cpu_time_end] = cur_time_us;
	cur_capacity[weighted_cpu_time_end] = max_current_cap;
	cpu_loading[weighted_cpu_time_end] = frame_time_ns * max_current_cap;
	weighted_cpu_time_end++;

	fstb_check_30_fps--;
	if (fstb_check_30_fps < 0)
		fstb_check_30_fps = 0;
	fpsgo_systrace_c_fstb(-200, fstb_check_30_fps, "fstb_check_30_fps");
	/* check if self-control 30 -> if yes, jump to 30 directly*/
	if (!lpp_mode && !force_vag &&
		fstb_fps_cur_limit > 30 && vag_flag < 2 && asfc_flag == 0 && fstb_check_30_fps == 0 && is_30_fps()) {
		asfc_flag = 1;
		fpsgo_systrace_c_fstb(-200, asfc_flag, "asfc_flag");
		if (wq) {
			struct work_struct *psWork = kmalloc(sizeof(struct work_struct), GFP_ATOMIC);

			if (psWork) {
				INIT_WORK(psWork, fstb_fps_stats);
				queue_work(wq, psWork);
			}
		}
	}

	mutex_unlock(&fstb_lock);

	mtk_fstb_dprintk(
			"fstb: time %lld %lld frame_time_ns %lld max_current_cap %u max_cpu_cap %u\n",
			cur_time_us, ktime_to_us(ktime_get())-cur_time_us, frame_time_ns, max_current_cap, max_cpu_cap);

	fpsgo_systrace_c_fstb(-200, (int)frame_time_ns, "t_cpu");
	fpsgo_systrace_c_fstb(-200, (int)max_current_cap, "cur_cpu_cap");
	fpsgo_systrace_c_fstb(-200, (int)max_cpu_cap, "max_cpu_cap");
#endif
	return -1;
}

void (*update_lppcap)(unsigned int cap);
void (*set_fps)(unsigned int fps);
#define MAX_CAP 100

static int get_lpp_cpu_max_capacity(unsigned int *ptr_lppcap)
{
	int ret = 0;
	unsigned int lppcap = MAX_CAP;
	int target_fps = lppfps;
	int cpu_loading_len = weighted_cpu_time_end - weighted_cpu_time_begin;
	int target_loading_len = 0;
	unsigned long long compare_loading = 0;
	unsigned long long temp_loading_len = 0;
	unsigned long long temp_loading = 0;
	unsigned long mod = 0;

	/*copy entries to temp array*/
	/*sort this array*/
	if (cpu_loading_len > 0 &&
			cpu_loading_len < FRAME_TIME_BUFFER_SIZE) {
		memcpy(sorted_cpu_loading, &cpu_loading[weighted_cpu_time_begin],
				sizeof(long long) * cpu_loading_len);
		sort(sorted_cpu_loading, cpu_loading_len,
				sizeof(long long), cmplonglong, NULL);
	}

	/*update fps budget*/
	if (lppfps && tm_input_fps) {
		if (tm_input_fps > lppfps)
			lpp_fps_budget += (tm_input_fps - lppfps);
		else if (tm_input_fps < lppfps)
			lpp_fps_budget -= (lppfps - tm_input_fps);
	}

	/*get lppcap*/
	if (cpu_loading_len) {
		if (max_freq && target_fps) {
			compare_loading = 1000000ULL * (1000 / target_fps) * 100 * lpp_freq;
			do_div(compare_loading, max_freq);
		}

		if (sorted_cpu_loading[QUANTILE * cpu_loading_len / 100 - 1] <= compare_loading) {
			/*light loading*/
			lppcap = MAX_CAP;
			ret = 1;
		} else {
			if (lpp_fps_budget >= lppfps * fps_error_threshold / 100)
				target_fps = lppfps * (100 - fps_error_threshold) / 100;
			else if (lpp_fps_budget > 0)
				target_fps = lppfps - lpp_fps_budget;
			target_fps = calculate_fps_limit(target_fps);

			temp_loading_len = target_fps * FRAME_TIME_WINDOW_SIZE_US;
			do_div(temp_loading_len, 1000000);
			target_loading_len = (int)temp_loading_len;
			if (cpu_loading_len > target_loading_len) {
			/*decrease*/
				temp_loading = sorted_cpu_loading[target_loading_len - 1];
				ret = 1;
			} else if (cpu_loading_len < target_loading_len) {
			/*increase*/
				temp_loading = sorted_cpu_loading[QUANTILE * cpu_loading_len / 100 - 1];
				ret = 1;
			}

			if (ret == 1 && target_fps) {
				do_div(temp_loading, 1000000);
				mod = do_div(temp_loading, (1000 / target_fps));
				lppcap = (unsigned int)temp_loading;
				if (mod)
					lppcap++;
			}
		}
		if (lppcap > MAX_CAP)
			lppcap = MAX_CAP;
	}

	if (ptr_lppcap)
		*ptr_lppcap = lppcap;
	else
		ret = 0;

	fpsgo_systrace_c_fstb(-200, cpu_loading_len, "cpu_loading_len");
	fpsgo_systrace_c_fstb(-200, target_fps, "target_fps");
	fpsgo_systrace_c_fstb(-200, sorted_cpu_loading[QUANTILE * cpu_loading_len / 100 - 1], "cpu_loading[Q]");
	fpsgo_systrace_c_fstb(-200,
		sorted_cpu_loading[target_loading_len <= FRAME_TIME_BUFFER_SIZE ?
		target_loading_len - 1 : FRAME_TIME_BUFFER_SIZE - 1], "cpu_loading[target]");
	fpsgo_systrace_c_fstb(-200, lpp_fps_budget, "lpp_fps_budget");
	fpsgo_systrace_c_fstb(-200, lppcap, "lppcap");
	fpsgo_systrace_c_fstb(-200, ret, "need_to_update_cap");
	return ret;
}

static long long get_cpu_frame_time(void)
{
	long long ret = INT_MAX;
	/*copy entries to temp array*/
	/*sort this array*/
	if (weighted_cpu_time_end - weighted_cpu_time_begin > 0 &&
			weighted_cpu_time_end - weighted_cpu_time_begin < FRAME_TIME_BUFFER_SIZE) {
		memcpy(sorted_weighted_cpu_time, &weighted_cpu_time[weighted_cpu_time_begin],
				sizeof(long long) * (weighted_cpu_time_end - weighted_cpu_time_begin));
		sort(sorted_weighted_cpu_time, weighted_cpu_time_end - weighted_cpu_time_begin,
				sizeof(long long), cmplonglong, NULL);
	}

	/*update nth value*/
	if (weighted_cpu_time_end - weighted_cpu_time_begin) {
		if (sorted_weighted_cpu_time[QUANTILE*(weighted_cpu_time_end-weighted_cpu_time_begin)/100] > INT_MAX)
			ret = INT_MAX;
		else
			ret = sorted_weighted_cpu_time[QUANTILE*(weighted_cpu_time_end-weighted_cpu_time_begin)/100];
	}

	fpsgo_systrace_c_log(ret, "quantile_weighted_cpu_time");
	return ret;

}

static unsigned int get_cpu_cur_capacity(void)
{
	unsigned int ret = 100;
	int i = 0;
	int arr_len = weighted_cpu_time_end - weighted_cpu_time_begin;
	unsigned int temp_capacity_sum = 0;

	if (arr_len > 0 && arr_len < FRAME_TIME_BUFFER_SIZE) {
		for (i = weighted_cpu_time_begin; i < weighted_cpu_time_end; i++)
			temp_capacity_sum += cur_capacity[i];
		ret = temp_capacity_sum / arr_len;

		if (ret > 100)
			ret = 100;
	}
	fpsgo_systrace_c_fstb(-200, ret, "avg_capacity");
	return ret;

}

static long long fps_limit[FRAME_TIME_BUFFER_SIZE];
static long long fps_limit_ts[FRAME_TIME_BUFFER_SIZE];	/*timestamp*/
static int fps_limit_begin,  fps_limit_end;
static int avg_fps_limit;

static int update_fps_limit(int cur_fps_limit)
{
#ifdef FPS_STABILIZER
	int i;

	ktime_t cur_time;
	long long cur_time_us;
	long long tot_fps_limit = 0;


	/*get current time*/
	cur_time = ktime_get();
	cur_time_us = ktime_to_us(cur_time);

	/*remove old entries*/
	while (fps_limit_begin < fps_limit_end) {
		if (fps_limit_ts[fps_limit_begin] < cur_time_us - FRAME_TIME_WINDOW_SIZE_US)
			fps_limit_begin++;
		else
			break;
	}

	if (fps_limit_begin == fps_limit_end &&
		fps_limit_end == FRAME_TIME_BUFFER_SIZE - 1)
		fps_limit_begin = fps_limit_end = 0;

	/*insert entries to weighted_fps_limit*/
	/*if buffer full --> move array align first*/
	if (fps_limit_end == FRAME_TIME_BUFFER_SIZE - 1) {
		for (i = 0; i < fps_limit_end - fps_limit_begin; i++) {
			fps_limit[i] = fps_limit[fps_limit_begin+i];
			fps_limit_ts[i] = fps_limit_ts[fps_limit_begin+i];
		}
		/*reset index*/
		fps_limit_end = fps_limit_end - fps_limit_begin;
		fps_limit_begin = 0;
	}

	fps_limit_ts[fps_limit_end] = cur_time_us;
	fps_limit[fps_limit_end] = cur_fps_limit;
	fps_limit_end++;

	/*update nth&avg value*/
	for (i = fps_limit_begin; i < fps_limit_end; i++)
		tot_fps_limit += fps_limit[i];

	if (fps_limit_end - fps_limit_begin) {
		avg_fps_limit = tot_fps_limit;
		do_div(avg_fps_limit, (fps_limit_end - fps_limit_begin));
	}

	if (FRAME_TIME_WINDOW_SIZE_US == ADJUST_INTERVAL_US)
		avg_fps_limit = cur_fps_limit;

	mtk_fstb_dprintk("fstb: time %lld %lld fps_limit %d avg_fps_limit %d\n",
			cur_time_us, ktime_to_us(ktime_get())-cur_time_us, cur_fps_limit, avg_fps_limit);
	fpsgo_systrace_c_fstb(-200, (int)avg_fps_limit, "avg_fps_limit");
#endif
	return -1;
}

static unsigned long long display_time_ts[FRAME_TIME_BUFFER_SIZE]; /*timestamp*/
static int display_time_begin, display_time_end;

void display_time_update(unsigned long long ts)
{
	ktime_t cur_time;
	long long cur_time_us;

	mutex_lock(&fstb_lock);

	if (!in_game_mode || !fstb_enable) {
		mutex_unlock(&fstb_lock);
		return;
	}

	/*get current time*/
	cur_time = ktime_get();
	cur_time_us = ktime_to_us(cur_time);

	if (display_time_begin < 0 ||
		display_time_end < 0 ||
		display_time_begin > display_time_end ||
		display_time_end >= FRAME_TIME_BUFFER_SIZE) {

		/* purge all data */
		display_time_begin = display_time_end = 0;
	}

	/*remove old entries*/
	while (display_time_begin < display_time_end) {
		if (display_time_ts[display_time_begin] < ts - (long long)FRAME_TIME_WINDOW_SIZE_US * 1000)
			display_time_begin++;
		else
			break;
	}

	if (display_time_begin == display_time_end &&
		display_time_end == FRAME_TIME_BUFFER_SIZE - 1)
		display_time_begin = display_time_end = 0;

	/*insert entries to weighted_display_time*/
	/*if buffer full --> move array align first*/
	if (display_time_begin < display_time_end &&
		display_time_end == FRAME_TIME_BUFFER_SIZE - 1) {
		memmove(display_time_ts, &display_time_ts[display_time_begin],
				sizeof(unsigned long long) * (display_time_end - display_time_begin));
		/*reset index*/
		display_time_end = display_time_end - display_time_begin;
		display_time_begin = 0;
	}

	display_time_ts[display_time_end] = ts;
	display_time_end++;

	mutex_unlock(&fstb_lock);

	mtk_fstb_dprintk("fstb: time %lld %lld\n", cur_time_us, ktime_to_us(ktime_get())-cur_time_us);
}

#define DISPLAY_FPS_FILTER_NS 100000000ULL

static int get_display_fps(long long interval)
{
	int i = display_time_begin, j;
	unsigned long long frame_interval_count = 0;
	unsigned long long avg_frame_interval = 0;
	unsigned long long retval = 0;
	long long tm_input_fps_no_filter;

	/*remove old entries*/
	while (i < display_time_end) {
		if (display_time_ts[i] < sched_clock() - interval * 1000)
			i++;
		else
			break;
	}

	for (j = i + 1; j < display_time_end; j++) {
		if ((display_time_ts[j] - display_time_ts[j - 1]) < DISPLAY_FPS_FILTER_NS) {
			avg_frame_interval += (display_time_ts[j] - display_time_ts[j - 1]);
			frame_interval_count++;
		}
	}

	tm_input_fps_no_filter = (long long)(display_time_end - i) * 1000000LL;
	do_div(tm_input_fps_no_filter, interval);
	fpsgo_systrace_c_fstb(-200, (int)tm_input_fps_no_filter, "tm_input_fps_no_filter");

	if (avg_frame_interval != 0) {
		retval = 1000000000ULL * frame_interval_count;
		do_div(retval, avg_frame_interval);
		return retval;
	} else
		return tm_input_fps;

}

void fstb_queue_time_update(unsigned long long ts)
{
	if (queue_time_begin < 0 ||
		queue_time_end < 0 ||
		queue_time_begin > queue_time_end ||
		queue_time_end >= FRAME_TIME_BUFFER_SIZE) {
		/* purge all data */
		queue_time_begin = queue_time_end = 0;
	}

	/*remove old entries*/
	while (queue_time_begin < queue_time_end) {
		if (queue_time_ts[queue_time_begin] < ts - (long long)FRAME_TIME_WINDOW_SIZE_US * 1000)
			queue_time_begin++;
		else
			break;
	}

	if (queue_time_begin == queue_time_end &&
		queue_time_end == FRAME_TIME_BUFFER_SIZE - 1)
		queue_time_begin = queue_time_end = 0;

	/*insert entries to weighted_display_time*/
	/*if buffer full --> move array align first*/
	if (queue_time_begin < queue_time_end &&
		queue_time_end == FRAME_TIME_BUFFER_SIZE - 1) {
		memmove(queue_time_ts, &queue_time_ts[queue_time_begin],
				sizeof(unsigned long long) * (queue_time_end - queue_time_begin));
		/*reset index*/
		queue_time_end = queue_time_end - queue_time_begin;
		queue_time_begin = 0;
	}

	queue_time_ts[queue_time_end] = ts;
	queue_time_end++;
}

static int fstb_get_queue_fps(long long interval)
{
	int i = queue_time_begin, j;
	long long queue_fps;
	unsigned long long frame_interval_count = 0;
	unsigned long long avg_frame_interval = 0;
	unsigned long long retval = 0;

	/* remove old entries */
	while (i < queue_time_end) {
		if (queue_time_ts[i] < sched_clock() - interval * 1000)
			i++;
		else
			break;
	}

	/* filter */
	for (j = i + 1; j < queue_time_end; j++) {
		if ((queue_time_ts[j] - queue_time_ts[j - 1]) < DISPLAY_FPS_FILTER_NS) {
			avg_frame_interval += (queue_time_ts[j] - queue_time_ts[j - 1]);
			frame_interval_count++;
		}
	}

	queue_fps = (long long)(queue_time_end - i) * 1000000LL;
	do_div(queue_fps, interval);
	fpsgo_systrace_c_fstb(-200, (int)queue_fps, "queue_fps_nofilter");

	if (avg_frame_interval != 0) {
		retval = 1000000000ULL * frame_interval_count;
		do_div(retval, avg_frame_interval);
		return retval;
	} else
		return queue_fps;
}

static long long weighted_gpu_time[FRAME_TIME_BUFFER_SIZE];
static long long weighted_gpu_time_ts[FRAME_TIME_BUFFER_SIZE];	/*timestamp*/
static int weighted_gpu_time_begin,  weighted_gpu_time_end;
static long long sorted_weighted_gpu_time[FRAME_TIME_BUFFER_SIZE];

void gpu_time_update(long long t_gpu, unsigned int cur_freq, unsigned int cur_max_freq)
{
#ifdef FPS_STABILIZER

	ktime_t cur_time;
	long long cur_time_us;

	mutex_lock(&fstb_lock);

	if (!in_game_mode || !fstb_enable) {
		mutex_unlock(&fstb_lock);
		return;
	}

	mtk_fstb_dprintk("t_gpu %lld cur_freq %u cur_max_freq %u\n", t_gpu, cur_freq, cur_max_freq);
	fpsgo_systrace_c_fstb(-200, (int)t_gpu, "t_gpu");
	fpsgo_systrace_c_fstb(-200, (int)cur_freq, "cur_gpu_freq");
	fpsgo_systrace_c_fstb(-200, (int)cur_max_freq, "max_gpu_freq");

	/*get current time*/
	cur_time = ktime_get();
	cur_time_us = ktime_to_us(cur_time);

	if (weighted_gpu_time_begin < 0 ||
		weighted_gpu_time_end < 0 ||
		weighted_gpu_time_begin > weighted_gpu_time_end ||
		weighted_gpu_time_end >= FRAME_TIME_BUFFER_SIZE) {

		/* purge all data */
		weighted_gpu_time_begin = weighted_gpu_time_end = 0;
	}

	/*remove old entries*/
	while (weighted_gpu_time_begin < weighted_gpu_time_end) {
		if (weighted_gpu_time_ts[weighted_gpu_time_begin] < cur_time_us - FRAME_TIME_WINDOW_SIZE_US)
			weighted_gpu_time_begin++;
		else
			break;
	}

	if (weighted_gpu_time_begin == weighted_gpu_time_end &&
		weighted_gpu_time_end == FRAME_TIME_BUFFER_SIZE - 1) {
		weighted_gpu_time_begin = weighted_gpu_time_end = 0;
	}

	/*insert entries to weighted_gpu_time*/
	/*if buffer full --> move array align first*/
	if (weighted_gpu_time_begin < weighted_gpu_time_end &&
		weighted_gpu_time_end == FRAME_TIME_BUFFER_SIZE - 1) {

		memmove(weighted_gpu_time, &weighted_gpu_time[weighted_gpu_time_begin],
				sizeof(long long) * (weighted_gpu_time_end - weighted_gpu_time_begin));
		memmove(weighted_gpu_time_ts, &weighted_gpu_time_ts[weighted_gpu_time_begin],
				sizeof(long long) * (weighted_gpu_time_end - weighted_gpu_time_begin));

		/*reset index*/
		weighted_gpu_time_end = weighted_gpu_time_end - weighted_gpu_time_begin;
		weighted_gpu_time_begin = 0;
	}

	if (cur_max_freq > 0 && cur_max_freq > cur_freq) {
		weighted_gpu_time[weighted_gpu_time_end] = t_gpu * cur_freq;
		do_div(weighted_gpu_time[weighted_gpu_time_end], cur_max_freq);
	}	else
		weighted_gpu_time[weighted_gpu_time_end] = t_gpu;
	weighted_gpu_time_ts[weighted_gpu_time_end] = cur_time_us;
	weighted_gpu_time_end++;

	mutex_unlock(&fstb_lock);

	/*print debug message*/
	mtk_fstb_dprintk("fstb: time %lld %lld t_gpu %lld cur_freq %u cur_max_freq %u\n",
			cur_time_us, ktime_to_us(ktime_get())-cur_time_us, t_gpu, cur_freq, cur_max_freq);

#endif
}

static int get_gpu_frame_time(void)
{
	int ret = INT_MAX;
	/*copy entries to temp array*/
	/*sort this array*/
	if (weighted_gpu_time_end - weighted_gpu_time_begin > 0 &&
			weighted_gpu_time_end - weighted_gpu_time_begin < FRAME_TIME_BUFFER_SIZE) {
		memcpy(sorted_weighted_gpu_time, &weighted_gpu_time[weighted_gpu_time_begin],
				sizeof(long long) * (weighted_gpu_time_end - weighted_gpu_time_begin));
		sort(sorted_weighted_gpu_time, weighted_gpu_time_end - weighted_gpu_time_begin,
				sizeof(long long), cmplonglong, NULL);
	}

	/*update nth value*/
	if (weighted_gpu_time_end - weighted_gpu_time_begin) {
		if (sorted_weighted_gpu_time[QUANTILE * (weighted_gpu_time_end - weighted_gpu_time_begin) / 100]
				> INT_MAX)
			ret = INT_MAX;
		else
			ret =
					sorted_weighted_gpu_time[QUANTILE *
					(weighted_gpu_time_end - weighted_gpu_time_begin) / 100];
	}

	mtk_fstb_dprintk("fstb: quantile_weighted_gpu_time %d\n", ret);
	fpsgo_systrace_c_log(ret, "quantile_weighted_gpu_time");
	return ret;
}

static int fps_update(void)
{
	tm_queue_fps = fstb_get_queue_fps(FRAME_TIME_WINDOW_SIZE_US);
	fpsgo_systrace_c_log(tm_queue_fps, "tm_queue_fps");

	tm_input_fps = get_display_fps(FRAME_TIME_WINDOW_SIZE_US);
	return 0;
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
	int i, fps_limit = max_fps_limit;


	if (target_fps >= max_fps_limit)
		return max_fps_limit;

	for (i = nr_fps_levels - 1; i >= 0; i--) {
		if (fps_levels[i].start >= target_fps) {
			if (target_fps >= fps_levels[i].end)
				fps_limit = target_fps;
			else {
				if (i == nr_fps_levels - 1)
					fps_limit = fps_levels[i].end;
				else
					fps_limit = fps_levels[i+1].start;
			}
			break;
		}
	}

	if (i < 0)
		fps_limit = max_fps_limit;

	return fps_limit;
}

static long long check_vag(long long target_limit)
{
	if (vag_flag == 2)
		target_limit = max_fps_limit;
	else if (tm_queue_fps <= (fstb_fps_cur_limit / 2 + 2)) {
		vag_flag++;
		if (fstb_fps_cur_limit == 30)
			asfc_flag = 0;
		if (vag_flag == 2)
			target_limit = max_fps_limit;
	} else
		vag_flag = 0;

	fpsgo_systrace_c_log(vag_flag, "vag_flag");
	return target_limit;
}

static int cur_cpu_time;
static int cur_gpu_time;

static void fstb_fps_stats(struct work_struct *work)
{

	int ret = -1;
	long long target_limit = 60;
	long long tmp_target_limit = 60;
	ktime_t cur_time;
	long long cur_time_us;
	unsigned int cur_cap;
	unsigned int lppcap = MAX_CAP;

	if (work != &fps_stats_work)
		kfree(work);

	mutex_lock(&fstb_lock);

	/*get current time*/
	cur_time = ktime_get();
	cur_time_us = ktime_to_us(cur_time);

	/* check the fps update from display */
	fps_update();

	cur_cpu_time = get_cpu_frame_time();
	cur_gpu_time = get_gpu_frame_time();
	cur_cap = get_cpu_cur_capacity();

	fpsgo_systrace_c_fstb(-200, lpp_mode, "lpp_mode");
	fpsgo_systrace_c_fstb(-200, last_lpp_mode, "last_lpp_mode");
	fpsgo_systrace_c_fstb(-200, lppfps, "lppfps");
	if (lpp_mode == 1) {
		if (update_lppcap) {
			if (fstb_reset)
				update_lppcap(MAX_CAP);
			else if (get_lpp_cpu_max_capacity(&lppcap))
				update_lppcap(lppcap);
		}
	} else {
		if (last_lpp_mode == 1) {
			/*reset*/
			last_lpp_mode = 0;
			fpsgo_systrace_c_fstb(-200, last_lpp_mode, "last_lpp_mode");
			if (update_lppcap)
				update_lppcap(MAX_CAP);
		}
	}

	if (fstb_reset) {
		fstb_reset = 0;
		fstb_check_30_fps = FPS30_CHECK_NR_FRAME;
		fpsgo_systrace_c_fstb(-200, fstb_check_30_fps, "fstb_check_30_fps");
		fpsgo_systrace_c_log(fstb_reset, "fstb_reset");
		target_limit = max_fps_limit;
		asfc_flag = 0;
		fpsgo_systrace_c_fstb(-200, asfc_flag, "asfc_flag");
		second_chance_flag = 0;
		fpsgo_systrace_c_fstb(-200, second_chance_flag, "second_chance_flag");
		vag_flag = 0;
		fpsgo_systrace_c_log(vag_flag, "vag_flag");
	} else if (lpp_mode == 1) {
		target_limit = max_fps_limit;
		asfc_flag = 0;
		second_chance_flag = 0;
		vag_flag = 0;
		/*decrease*/
	} else if (avg_fps_limit - tm_input_fps > fstb_fps_cur_limit * fps_error_threshold / 100) {
		if (cur_cap > SECOND_CHANCE_CAPACITY || second_chance_flag == 1) {
			if (tm_input_fps < fstb_fps_cur_limit)
				target_limit = tm_input_fps;
			else
				target_limit = fstb_fps_cur_limit;
			second_chance_flag = 0;
		} else {
			second_chance_flag = 1;
			target_limit = fstb_fps_cur_limit;
		}
		fpsgo_systrace_c_fstb(-200, (int)target_limit, "tmp_target_limit");
		fpsgo_systrace_c_fstb(-200, second_chance_flag, "second_chance_flag");
		target_limit = check_vag(target_limit);
		/*increase*/
	} else if (avg_fps_limit - tm_input_fps <= 1) {

		second_chance_flag = 0;
		fpsgo_systrace_c_fstb(-200, second_chance_flag, "second_chance_flag");
		tmp_target_limit = 1000000000LL;
		do_div(tmp_target_limit, (long long)max(cur_cpu_time, cur_gpu_time));
		fpsgo_systrace_c_fstb(-200, (int)tmp_target_limit, "tmp_target_limit");

		if ((int)tmp_target_limit - fstb_fps_cur_limit > fstb_fps_cur_limit * fps_error_threshold / 100) {
			if (fstb_fps_cur_limit >= 48)
				target_limit = tmp_target_limit;
			else if (fstb_fps_cur_limit >= 45)
				target_limit = 48;
			else if (fstb_fps_cur_limit >= 36)
				target_limit = 45;
			else if (fstb_fps_cur_limit > 30)
				target_limit = 36;
			else if (fstb_fps_cur_limit == 30)
				target_limit = 60;
			else if (fstb_fps_cur_limit >= 27)
				target_limit = 30;
			else if (fstb_fps_cur_limit >= 24)
				target_limit = 27;
			else
				target_limit = tmp_target_limit;
			if (target_limit < fstb_fps_cur_limit)
				target_limit = fstb_fps_cur_limit;
		} else
			target_limit = fstb_fps_cur_limit;
		target_limit = check_vag(target_limit);
		/*stable state*/
	} else {
		second_chance_flag = 0;
		fpsgo_systrace_c_fstb(-200, second_chance_flag, "second_chance_flag");
		target_limit = fstb_fps_cur_limit;
		target_limit = check_vag(target_limit);
	}

	if (vag_flag < 2 && asfc_flag && target_limit > 30)
		target_limit = 30;

	fstb_fps_cur_limit = calculate_fps_limit(target_limit);

	if (fstb_fps_cur_limit == 30 && vag_flag == 0) {
		asfc_flag = 1;
		fpsgo_systrace_c_fstb(-200, asfc_flag, "asfc_flag");
	}

	if (vag_flag == 2 || force_vag)
		fbt_cpu_vag_set_fps(vag_fps);
	else
		fbt_cpu_vag_set_fps(0);

#ifdef CONFIG_MTK_DYNAMIC_FPS_FRAMEWORK_SUPPORT
	if (!lpp_mode && !force_vag)
		ret = dfrc_set_kernel_policy(DFRC_DRV_API_LOADING,
				((fstb_fps_cur_limit != 60 && in_game_mode && fstb_enable) ? fstb_fps_cur_limit : -1),
				DFRC_DRV_MODE_INTERNAL_SW, 0, 0);
#endif

	update_fps_limit(fstb_fps_cur_limit);

	if (in_game_mode && fstb_enable)
		enable_fstb_timer();
	else
		disable_fstb_timer();

	mutex_unlock(&fstb_lock);

	mtk_fstb_dprintk_always("[DFPS] fps:%d, ret = %d\n", fstb_fps_cur_limit, ret);
	mtk_fstb_dprintk(
		"fstb: time %lld %lld quantile_weighted_cpu_time %d target_limit %lld fstb_fps_cur_limit %d ret %d tm_input_fps %d\n",
		cur_time_us, ktime_to_us(ktime_get())-cur_time_us, cur_cpu_time, target_limit, fstb_fps_cur_limit,
		ret, tm_input_fps);

	fpsgo_systrace_c_fstb(-200, (int)target_limit, "target_limit");
	fpsgo_systrace_c_log((int)fstb_fps_cur_limit, "fstb_fps_cur_limit");
	fpsgo_systrace_c_log((int)tm_input_fps, "tm_input_fps");
}

static int set_fps_level(int nr_level, struct fps_level *level)
{
	int i;

	if (nr_level > MAX_NR_FPS_LEVELS)
		return -EINVAL;

	for (i = 0; i < nr_level; i++) {
		/* check if they are interleaving */
		if (level[i].end > level[i].start ||
				(i > 0 && level[i].start > level[i - 1].end))
			return -EINVAL;
	}

	memcpy(fps_levels, level, nr_level * sizeof(struct fps_level));

	nr_fps_levels = nr_level;
	max_fps_limit = fps_levels[0].start;
	min_fps_limit = fps_levels[nr_fps_levels - 1].end;

	return 0;
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

static int fstb_tune_lpp_freq_read(struct seq_file *m, void *v)
{
	seq_printf(m, "%d ", lpp_freq);
	return 0;
}

static ssize_t fstb_tune_lpp_freq_write(struct file *file, const char __user *buffer,
		size_t count, loff_t *data)
{
	int ret;
	unsigned int arg;

	if (!kstrtouint_from_user(buffer, count, 0, &arg))
		ret = switch_lpp_freq(arg);
	else
		ret = -EINVAL;

	return (ret < 0) ? ret : count;
}

static int fstb_tune_lpp_freq_open(struct inode *inode, struct file *file)
{
	return single_open(file, fstb_tune_lpp_freq_read, NULL);
}

static const struct file_operations fstb_tune_lpp_freq_fops = {
	.owner = THIS_MODULE,
	.open = fstb_tune_lpp_freq_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = fstb_tune_lpp_freq_write,
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

static int fstb_tune_force_vag_read(struct seq_file *m, void *v)
{
	seq_printf(m, "%d ", force_vag);
	return 0;
}

static ssize_t fstb_tune_force_vag_write(struct file *file, const char __user *buffer,
		size_t count, loff_t *data)
{
	int ret;
	int arg;

	if (!kstrtoint_from_user(buffer, count, 0, &arg))
		ret = switch_force_vag(arg);
	else
		ret = -EINVAL;

	return (ret < 0) ? ret : count;
}

static int fstb_tune_force_vag_open(struct inode *inode, struct file *file)
{
	return single_open(file, fstb_tune_force_vag_read, NULL);
}

static const struct file_operations fstb_tune_force_vag_fops = {
	.owner = THIS_MODULE,
	.open = fstb_tune_force_vag_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = fstb_tune_force_vag_write,
	.release = single_release,
};

static int fstb_tune_vag_fps_read(struct seq_file *m, void *v)
{
	seq_printf(m, "%d ", vag_fps);
	return 0;
}

static ssize_t fstb_tune_vag_fps_write(struct file *file, const char __user *buffer,
		size_t count, loff_t *data)
{
	int ret;
	int arg;

	if (!kstrtoint_from_user(buffer, count, 0, &arg))
		ret = switch_vag_fps(arg);
	else
		ret = -EINVAL;

	return (ret < 0) ? ret : count;
}

static int fstb_tune_vag_fps_open(struct inode *inode, struct file *file)
{
	return single_open(file, fstb_tune_vag_fps_read, NULL);
}

static const struct file_operations fstb_tune_vag_fps_fops = {
	.owner = THIS_MODULE,
	.open = fstb_tune_vag_fps_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = fstb_tune_vag_fps_write,
	.release = single_release,
};

static int fstb_tune_lpp_fps_read(struct seq_file *m, void *v)
{
	seq_printf(m, "%d ", lppfps);
	return 0;
}

static ssize_t fstb_tune_lpp_fps_write(struct file *file, const char __user *buffer,
		size_t count, loff_t *data)
{
	int ret;
	int arg;

	if (!kstrtoint_from_user(buffer, count, 0, &arg))
		ret = switch_lpp_fps(arg);
	else
		ret = -EINVAL;

	return (ret < 0) ? ret : count;
}

static int fstb_tune_lpp_fps_open(struct inode *inode, struct file *file)
{
	return single_open(file, fstb_tune_lpp_fps_read, NULL);
}

static const struct file_operations fstb_tune_lpp_fps_fops = {
	.owner = THIS_MODULE,
	.open = fstb_tune_lpp_fps_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = fstb_tune_lpp_fps_write,
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
			switch_fstb(k_enable);

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

	seq_printf(m, "%d,%d,%d,%d,%d,%d,%d\n",
			info.updateFPS / 100, tm_input_fps, fstb_fps_cur_limit,
			avg_fps_limit, in_game_mode, cur_cpu_time, cur_gpu_time);

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

static int __init mtk_fstb_init(void)
{
	int num_cluster = 0;

	mtk_fstb_dprintk("init\n");

	fbt_notifier_cpu_frame_time_fps_stabilizer = update_cpu_frame_info;
	display_time_fps_stablizer = display_time_update;
	ged_kpi_output_gfx_info_fp = gpu_time_update;
	num_cluster = arch_get_nr_clusters();
	max_freq = mt_cpufreq_get_freq_by_idx((num_cluster > 0) ? num_cluster - 1 : 0, 0);

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

	debugfs_create_file("fstb_tune_lpp_freq",
			    S_IRUGO | S_IWUSR | S_IWGRP,
			    fstb_debugfs_dir,
			    NULL,
			    &fstb_tune_lpp_freq_fops);

	debugfs_create_file("fstb_tune_lpp_fps",
			    S_IRUGO | S_IWUSR | S_IWGRP,
			    fstb_debugfs_dir,
			    NULL,
			    &fstb_tune_lpp_fps_fops);

	debugfs_create_file("fstb_tune_force_vag",
			    S_IRUGO | S_IWUSR | S_IWGRP,
			    fstb_debugfs_dir,
			    NULL,
			    &fstb_tune_force_vag_fops);

	debugfs_create_file("fstb_tune_vag_fps",
			    S_IRUGO | S_IWUSR | S_IWGRP,
			    fstb_debugfs_dir,
			    NULL,
			    &fstb_tune_vag_fps_fops);

	reset_fps_level();

	wq = create_singlethread_workqueue("mt_fstb");
	if (!wq)
		goto err;

	hrtimer_init(&hrt, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hrt.function = &mt_fstb;

	return 0;

err:
	return -1;
}

static void __exit mtk_fstb_exit(void)
{
	mtk_fstb_dprintk("exit\n");

	disable_fstb_timer();

	/* remove the debugfs file */
	debugfs_remove_recursive(fstb_debugfs_dir);
}

module_init(mtk_fstb_init);
module_exit(mtk_fstb_exit);
