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
#include <linux/debugfs.h>
#include <linux/sort.h>
#include <linux/string.h>
#include <asm/div64.h>
#include <linux/topology.h>
#include <linux/slab.h>


#include <fpsgo_common.h>
#include <trace/events/fpsgo.h>
#include <mach/mtk_ppm_api.h>

#include <mtk_vcorefs_governor.h>
#include <mtk_vcorefs_manager.h>

#include "eas_controller.h"
#include "legacy_controller.h"

#include "fbt_error.h"
#include "fbt_base.h"
#include "fbt_usedext.h"
#include "fbt_notifier.h"
#include "fbt_cpu.h"
#include "../fstb/fstb.h"
#include "xgf.h"
#include "fbt_cpu_platform.h"


#define Target_fps_30_NS 33333333
#define Target_fps_60_NS 16666666
#define GED_VSYNC_MISS_QUANTUM_NS 16666666
#define TIME_8MS  8000000
#define TIME_5MS  5000000
#define TIME_3MS  3000000
#define TIME_1MS  1000000
#define WINDOW 20


#define FBTCPU_SEC_DIVIDER 1000000000
#define TARGET_UNLIMITED_FPS 60
#define RESET_TOLERENCE 2

static int bhr;
static int bhr_opp;
static int target_cluster;
static int migrate_bound;
static int vsync_percent;
static int rescue_opp_f;
static int rescue_opp_c;
static int rescue_percent;
static int vsync_period;
static int deqtime_bound;
static int variance;
static int floor_bound;
static int kmin;
static int floor_opp;

module_param(bhr, int, S_IRUGO|S_IWUSR);
module_param(bhr_opp, int, S_IRUGO|S_IWUSR);
module_param(target_cluster, int, S_IRUGO|S_IWUSR);
module_param(migrate_bound, int, S_IRUGO|S_IWUSR);
module_param(vsync_percent, int, S_IRUGO|S_IWUSR);
module_param(rescue_opp_f, int, S_IRUGO|S_IWUSR);
module_param(rescue_opp_c, int, S_IRUGO|S_IWUSR);
module_param(rescue_percent, int, S_IRUGO|S_IWUSR);
module_param(vsync_period, int, S_IRUGO|S_IWUSR);
module_param(deqtime_bound, int, S_IRUGO|S_IWUSR);
module_param(variance, int, S_IRUGO|S_IWUSR);
module_param(floor_bound, int, S_IRUGO|S_IWUSR);
module_param(kmin, int, S_IRUGO|S_IWUSR);
module_param(floor_opp, int, S_IRUGO|S_IWUSR);

static unsigned int base_blc;

static unsigned int base_freq;

static unsigned long long vsync_distance;
static unsigned long long rescue_distance;


static atomic_t loading_cur;

static unsigned int last_obv;
static unsigned long long last_cb_ts;

static int dequeuebuffer;
static unsigned long long vsync_time;
static unsigned long long deqend_time;
static unsigned long long asfc_time;
static unsigned long long asfc_last_fps;

static int history_blc_count;

static DEFINE_SPINLOCK(xgf_slock);
static DEFINE_MUTEX(xgf_mlock);

static unsigned int _gdfrc_fps_limit;
static unsigned int _gdfrc_fps_limit_ex;
static int _gdfrc_cpu_target;

static struct fbt_proc proc;

static unsigned long long q2q_time;
static unsigned long long deq_time;
static int sf_check;
static int sf_bound;
static int sf_bound_max;
static int sf_bound_min;
static int last_fps;
static int middle_enable;

static int fbt_enable;
static int game_mode;
static int game_mode_hint;

static struct fbt_frame_info frame_info[WINDOW];
static unsigned long long floor;
static int floor_count;
static int reset_floor_bound;
static int f_iter;

static int vag_fps;

int cluster_freq_bound[MAX_FREQ_BOUND_NUM] = {0};
int cluster_rescue_bound[MAX_FREQ_BOUND_NUM] = {0};

int cluster_num;
static unsigned int cpu_max_freq;
static unsigned int *clus_obv;

static struct fbt_cpu_dvfs_info *cpu_dvfs;

static unsigned int lpp_max_cap;
static unsigned int lpp_fps;
static unsigned int *lpp_clus_max_freq;

static int *clus_max_freq;
static int max_freq_cluster;

unsigned long long nsec_to_usec(unsigned long long nsec)
{
	unsigned long long usec;

	usec = div64_u64(nsec, (unsigned long long)NSEC_PER_USEC);

	return usec;
}

static int fbt_cpuset_enable;

void fbt_cpu_vag_set_fps(unsigned int fps)
{
	unsigned long flags;

	spin_lock_irqsave(&xgf_slock, flags);
	vag_fps = fps;
	spin_unlock_irqrestore(&xgf_slock, flags);
	fpsgo_systrace_c_fbt_gm(-100, vag_fps, "vag_fps");
}

void fbt_cpu_lpp_set_fps(unsigned int fps)
{
	unsigned long flags;

	spin_lock_irqsave(&xgf_slock, flags);
	lpp_fps = fps;
	spin_unlock_irqrestore(&xgf_slock, flags);
	fpsgo_systrace_c_fbt_gm(-100, lpp_fps, "lpp_fps");
}

void fbt_cpu_lppcap_update(unsigned int llpcap)
{
	unsigned long flags;
	int cluster;

	spin_lock_irqsave(&xgf_slock, flags);

	if (llpcap)
		lpp_max_cap = llpcap;
	else
		lpp_max_cap = 100;

	if (lpp_max_cap < 100) {
		unsigned int lpp_max_freq;
		int i;

		lpp_max_freq = (min(lpp_max_cap, 100U) * cpu_max_freq);
		do_div(lpp_max_freq, 100U);

		for (cluster = 0 ; cluster < cluster_num; cluster++) {
			for (i = (NR_FREQ_CPU - 1); i > 0; i--) {
				if (cpu_dvfs[cluster].power[i] >= lpp_max_freq)
					break;
			}
			lpp_clus_max_freq[cluster] =
				cpu_dvfs[cluster].power[min((i+1), (NR_FREQ_CPU - 1))];
		}

	} else {
		for (cluster = 0 ; cluster < cluster_num; cluster++)
			lpp_clus_max_freq[cluster] = cpu_dvfs[cluster].power[0];
	}
	spin_unlock_irqrestore(&xgf_slock, flags);
}

int fbt_cpu_set_bhr(int new_bhr)
{
	unsigned long flags;

	if (new_bhr < 0 || new_bhr > 100)
		return -EINVAL;

	spin_lock_irqsave(&xgf_slock, flags);
	bhr = new_bhr;
	spin_unlock_irqrestore(&xgf_slock, flags);

	return 0;
}

int fbt_cpu_set_bhr_opp(int new_opp)
{
	unsigned long flags;

	if (new_opp < 0)
		return -EINVAL;

	spin_lock_irqsave(&xgf_slock, flags);
	bhr_opp = new_opp;
	spin_unlock_irqrestore(&xgf_slock, flags);

	return 0;
}

int fbt_cpu_set_rescue_opp_c(int new_opp)
{
	unsigned long flags;

	if (new_opp < 0)
		return -EINVAL;

	spin_lock_irqsave(&xgf_slock, flags);
	rescue_opp_c = new_opp;
	spin_unlock_irqrestore(&xgf_slock, flags);

	return 0;
}

int fbt_cpu_set_rescue_opp_f(int new_opp)
{
	unsigned long flags;

	if (new_opp < 0)
		return -EINVAL;

	spin_lock_irqsave(&xgf_slock, flags);
	rescue_opp_f = new_opp;
	spin_unlock_irqrestore(&xgf_slock, flags);

	return 0;
}

int fbt_cpu_set_rescue_percent(int percent)
{
	unsigned long flags;

	if (percent < 0 || percent > 100)
		return -EINVAL;

	spin_lock_irqsave(&xgf_slock, flags);
	rescue_percent = percent;
	spin_unlock_irqrestore(&xgf_slock, flags);

	return 0;
}

int fbt_cpu_set_variance(int var)
{
	unsigned long flags;

	if (var < 0 || var > 100)
		return -EINVAL;

	spin_lock_irqsave(&xgf_slock, flags);
	variance = var;
	spin_unlock_irqrestore(&xgf_slock, flags);

	return 0;
}

int fbt_cpu_set_floor_bound(int bound)
{
	unsigned long flags;

	if (bound < 0 || bound > WINDOW)
		return -EINVAL;

	spin_lock_irqsave(&xgf_slock, flags);
	floor_bound = bound;
	spin_unlock_irqrestore(&xgf_slock, flags);

	return 0;
}

int fbt_cpu_set_floor_kmin(int k)
{
	unsigned long flags;

	if (k < 0 || k > WINDOW)
		return -EINVAL;

	spin_lock_irqsave(&xgf_slock, flags);
	kmin = k;
	spin_unlock_irqrestore(&xgf_slock, flags);

	return 0;
}

int fbt_cpu_set_floor_opp(int new_opp)
{
	unsigned long flags;

	if (new_opp < 0)
		return -EINVAL;

	spin_lock_irqsave(&xgf_slock, flags);
	floor_opp = new_opp;
	spin_unlock_irqrestore(&xgf_slock, flags);

	return 0;
}

void xgf_setting_exit(void)
{
	struct ppm_limit_data *pld;
	unsigned long flags;
	int i;

	pld =
		kzalloc(cluster_num * sizeof(struct ppm_limit_data), GFP_KERNEL);

	/* clear boost data & loading*/
	spin_lock_irqsave(&xgf_slock, flags);
	atomic_set(&loading_cur, 0);
	last_cb_ts = ged_get_time();
	target_cluster = cluster_num;
	history_blc_count = 0;
	middle_enable = 1;
	floor = 0;
	floor_count = 0;
	reset_floor_bound = 0;
	f_iter = 0;
	q2q_time = 0;
	deq_time = 0;
	sf_check = 0;
	last_fps = 0;
	sf_bound = sf_bound_min;
	memset(frame_info, 0, sizeof(frame_info));
	spin_unlock_irqrestore(&xgf_slock, flags);
	fpsgo_systrace_c_fbt_gm(-100, target_cluster, "target_cluster");
	fpsgo_systrace_c_fbt_gm(-100, history_blc_count, "history_blc_count");
	fpsgo_systrace_c_fbt_gm(-100, atomic_read(&loading_cur), "loading");

	update_eas_boost_value(EAS_KIR_FBC, CGROUP_TA, 0);
	if (fbt_cpuset_enable) {
		prefer_idle_for_perf_idx(CGROUP_TA, 0); /*todo sync?*/
		if (cluster_num > 1)
			set_cpuset(-1); /*todo sync?*/
		/*vcorefs_request_dvfs_opp(KIR_FBT, -1);*/
	}

	for (i = 0; i < cluster_num; i++) {
		pld[i].max = -1;
		pld[i].min = -1;
	}

	update_userlimit_cpu_freq(PPM_KIR_FBC, cluster_num, pld);
	kfree(pld);
}

int switch_fbt_game(int enable)
{
	mutex_lock(&xgf_mlock);
	fbt_enable = enable;
	game_mode = game_mode_hint && fbt_enable;

	if (!fbt_enable && game_mode_hint) {
		xgf_setting_exit();
		ppm_game_mode_change_cb(0);
		xgf_game_mode_exit(!fbt_enable);
	}

	fpsgo_systrace_c_fbt_gm(-100, fbt_enable, "fbt_enable");
	mutex_unlock(&xgf_mlock);
	return 0;
}

void fbt_cpu_set_game_hint_cb(int is_game_mode)
{
	FBT_LOGE("%s game mode", is_game_mode ? "enter" : "exit");

	mutex_lock(&xgf_mlock);
	game_mode_hint = is_game_mode;
	game_mode = game_mode_hint && fbt_enable;

	ppm_game_mode_change_cb(game_mode);
	if (game_mode) {
		if (fbt_cpuset_enable)
			prefer_idle_for_perf_idx(CGROUP_TA, 1);
	} else
		xgf_setting_exit();
	mutex_unlock(&xgf_mlock);

}
EXPORT_SYMBOL(fbt_cpu_set_game_hint_cb);

int fbt_check_cluster_by_user_limit(int tgt_freq, int move)
{
	int new_cluster;
	int move_cluster = 0;
	int i;

	for (i = target_cluster; i < cluster_num; i++) {
		if (clus_max_freq[i] > tgt_freq)
			break;
	}
	if (i == cluster_num)
		new_cluster = max_freq_cluster;
	else
		new_cluster = i;

	fpsgo_systrace_c_log(target_cluster, "target_cluster");
	fpsgo_systrace_c_fbt_gm(-100, new_cluster, "new_cluster");

	if (target_cluster == new_cluster)
		move_cluster = move;
	else {
		move_cluster = !move;
		target_cluster = new_cluster;
	}
	xgf_trace("fbt_cpu max_freq_cluster=%d tgt_freq=%d move_cluster=%d",
		 max_freq_cluster, tgt_freq, move_cluster);

	return move_cluster;
}

int fbt_check_target_cluster(int freq, int type)
{
	int new_cluster = 0;

	switch (type) {
	case FBT_CPU_FREQ_BOUND:
		switch (cluster_num) {
		case 2:
			new_cluster = freq > cluster_freq_bound[0] ? 1 : 0;
			return new_cluster;
		case 3:
			if (freq > cluster_freq_bound[1])
				new_cluster = 2;
			else if (freq > cluster_freq_bound[0])
				new_cluster = 1;
			else
				new_cluster = 0;
			return new_cluster;
		default:
			return new_cluster;
		}
	case FBT_CPU_RESCUE_BOUND:
		switch (cluster_num) {
		case 2:
			new_cluster = freq > cluster_rescue_bound[0] ? 1 : 0;
			return new_cluster;
		case 3:
			if (freq > cluster_rescue_bound[1])
				new_cluster = 2;
			else if (freq > cluster_rescue_bound[0])
				new_cluster = 1;
			else
				new_cluster = 0;
			return new_cluster;
		default:
			return new_cluster;
		}
	default:
		return new_cluster;
	}
}

unsigned int fbt_get_new_base_blc(void)
{
	struct ppm_limit_data *pld;
	unsigned int blc_wt, blc_freq;
	int i, cluster;
	int *clus_opp;
	unsigned long flags;
	int move_cluster = 0;
	int new_cluster;

	clus_opp =
		kzalloc(cluster_num * sizeof(int), GFP_KERNEL);

	pld =
		kzalloc(cluster_num * sizeof(struct ppm_limit_data), GFP_KERNEL);

	spin_lock_irqsave(&xgf_slock, flags);

	if (target_cluster == cluster_num) {
		spin_unlock_irqrestore(&xgf_slock, flags);
		kfree(pld);
		kfree(clus_opp);
		return 0;
	}

	blc_wt = base_blc;
	blc_freq = base_freq;

	for (cluster = 0 ; cluster < cluster_num; cluster++) {
		for (i = (NR_FREQ_CPU - 1); i > 0; i--) {
			if (cpu_dvfs[cluster].power[i] >= blc_freq)
				break;
		}
		clus_opp[cluster] = i;
		xgf_trace("blc_freq=%d cpu_dvfs[%d].power[%d]=%d clus_opp=%d",
			 blc_freq, cluster, i, cpu_dvfs[cluster].power[i], clus_opp[cluster]);
	}

	blc_freq =
		cpu_dvfs[target_cluster].power[max((clus_opp[target_cluster] - rescue_opp_f), 0)];
	blc_freq = min_t(unsigned int, blc_freq, lpp_clus_max_freq[target_cluster]);

	new_cluster = fbt_check_target_cluster((int)blc_freq, FBT_CPU_RESCUE_BOUND);

	if (new_cluster != target_cluster) {
		target_cluster = new_cluster;
		history_blc_count = 0;
		move_cluster = 1;
	}

	move_cluster = fbt_check_cluster_by_user_limit((int)blc_freq, move_cluster);

	for (cluster = 0 ; cluster < cluster_num; cluster++) {
		if (cluster == target_cluster) {
			pld[cluster].max =
				cpu_dvfs[target_cluster].power[max((clus_opp[cluster] - rescue_opp_c), 0)];
		} else
			pld[cluster].max = cpu_dvfs[cluster].power[clus_opp[cluster]];
		pld[cluster].min = -1;
		pld[cluster].max = min_t(unsigned int, pld[cluster].max, lpp_clus_max_freq[cluster]);
	}

	blc_wt = blc_freq * 100;
	do_div(blc_wt, cpu_max_freq);
	fpsgo_systrace_c_log(blc_wt, "perf idx");
	fpsgo_systrace_c_log(target_cluster, "target_cluster");
	spin_unlock_irqrestore(&xgf_slock, flags);

	if (move_cluster && cluster_num > 1 && fbt_cpuset_enable)
		set_cpuset(target_cluster);
	update_userlimit_cpu_freq(PPM_KIR_FBC, cluster_num, pld);

	for (cluster = 0 ; cluster < cluster_num; cluster++) {
		fpsgo_systrace_c_fbt_gm(-100, pld[cluster].max, "cluster%d limit freq", cluster);
		fpsgo_systrace_c_fbt_gm(-100, lpp_clus_max_freq[cluster], "lpp_clus_max_freq %d", cluster);
	}

	kfree(pld);
	kfree(clus_opp);
	return blc_wt;
}


static void fbt_do_jerk(struct work_struct *work)
{
	struct fbt_jerk *jerk;

	if (target_cluster == cluster_num)
		return;

	jerk = container_of(work, struct fbt_jerk, work);

	if (jerk->id == proc.active_jerk_id) {
		unsigned int blc_wt;

		blc_wt = fbt_get_new_base_blc();
		if (blc_wt != 0)
			fbt_set_boost_value(target_cluster, blc_wt);
		xgf_trace("boost jerk %d proc.id %d", jerk->id, proc.active_jerk_id);
	} else
		xgf_trace("skip jerk %d proc.id %d", jerk->id, proc.active_jerk_id);
}

static enum hrtimer_restart fbt_jerk_tfn(struct hrtimer *timer)
{
	struct fbt_jerk *jerk;

	jerk = container_of(timer, struct fbt_jerk, timer);
	schedule_work(&jerk->work);
	return HRTIMER_NORESTART;
}

static inline void fbt_init_jerk(struct fbt_jerk *jerk, int id)
{
	jerk->id = id;

	hrtimer_init(&jerk->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	jerk->timer.function = &fbt_jerk_tfn;
	INIT_WORK(&jerk->work, fbt_do_jerk);
}

void xgf_ged_vsync(void)
{
	if (game_mode) {
		unsigned long flags;

		spin_lock_irqsave(&xgf_slock, flags);
		vsync_time = ged_get_time();
		xgf_trace("vsync_time=%llu", nsec_to_usec(vsync_time));
		spin_unlock_irqrestore(&xgf_slock, flags);
	}
}

void xgf_get_deqend_time(void)
{
	unsigned long flags;

	spin_lock_irqsave(&xgf_slock, flags);
	deqend_time = ged_get_time();
	xgf_trace("deqend_time=%llu", nsec_to_usec(deqend_time));
	spin_unlock_irqrestore(&xgf_slock, flags);
}

void __xgf_cpufreq_cb_normal(int cid, unsigned long freq, int ob)
{
	unsigned int curr_obv = 0U;
	unsigned long long curr_cb_ts;

	curr_cb_ts = ged_get_time();

	curr_obv = (unsigned int)(freq * 100);
	do_div(curr_obv, cpu_max_freq);

	if (clus_obv[cid] != curr_obv)
		fpsgo_systrace_c_log(curr_obv, "curr_obv[%d]", cid);

	clus_obv[cid] = curr_obv;

	if (last_obv == base_blc) {
		last_obv = clus_obv[target_cluster];
		last_cb_ts = curr_cb_ts;
		return;
	}

	if (curr_cb_ts > last_cb_ts) {
		unsigned long long dur = (curr_cb_ts - last_cb_ts);
		int diff = last_obv - base_blc;
		int old_loading;

		dur = nsec_to_usec(dur);
		old_loading = (int)atomic_read(&loading_cur);

		xgf_trace("base=%d obv=%d->%d time=%llu->%llu load=%d->%d", base_blc,
			  last_obv, clus_obv[target_cluster],
			  nsec_to_usec(last_cb_ts), nsec_to_usec(curr_cb_ts),
			  old_loading, atomic_add_return((int)(dur * diff), &loading_cur));
		fpsgo_systrace_c_fbt_gm(-100, atomic_read(&loading_cur), "loading");
	}

	last_obv = clus_obv[target_cluster];
	xgf_trace("keep last_obv=%d ", last_obv);
	last_cb_ts = curr_cb_ts;
}


void __xgf_cpufreq_cb_flush(void)
{
	unsigned long long curr_cb_ts;

	curr_cb_ts = ged_get_time();

	if (last_obv == base_blc) {
		last_obv = 0UL;
		last_cb_ts = curr_cb_ts;
		return;
	}

	if (curr_cb_ts > last_cb_ts) {
		unsigned long long dur  = (curr_cb_ts - last_cb_ts);
		int diff = last_obv - base_blc;
		int old_loading;

		dur = nsec_to_usec(dur);
		old_loading = (int)atomic_read(&loading_cur);

		xgf_trace("base=%d obv=%d->%d time=%llu->%llu load=%d->%d", base_blc,
			  last_obv, clus_obv[target_cluster],
			  nsec_to_usec(last_cb_ts), nsec_to_usec(curr_cb_ts),
			  old_loading, atomic_add_return((int)(dur * diff), &loading_cur));
		fpsgo_systrace_c_fbt_gm(-100, atomic_read(&loading_cur), "loading");
	}

	last_obv = 0UL;
	last_cb_ts = curr_cb_ts;
}

void __xgf_cpufreq_cb_skip(int cid, unsigned long freq, int ob)
{
	unsigned int curr_obv = 0U;
	unsigned long long curr_cb_ts;

	curr_cb_ts = ged_get_time();

	curr_obv = (unsigned int)(freq * 100);
	do_div(curr_obv, cpu_max_freq);

	if (clus_obv[cid] != curr_obv)
		fpsgo_systrace_c_log(curr_obv, "curr_obv[%d]", cid);

	clus_obv[cid] = curr_obv;
	xgf_trace("skip last_obv=%d ", last_obv);

	last_obv = 0U;
	last_cb_ts = curr_cb_ts;
}

void xgf_cpufreq_cb(int cid, unsigned long freq)
{
	unsigned long flags;
	signed int boost_min;
	int over_boost;

	if (!game_mode)
		return;

	spin_lock_irqsave(&xgf_slock, flags);

	if (target_cluster == cluster_num) {
		last_cb_ts = ged_get_time();
		clus_obv[cid] = (unsigned int)(freq * 100);
		do_div(clus_obv[cid], cpu_max_freq);
		spin_unlock_irqrestore(&xgf_slock, flags);
		return;
	}

	if (cid != target_cluster) {
		spin_unlock_irqrestore(&xgf_slock, flags);
		return;
	}

	boost_min = min_boost_freq[cid];
	over_boost = (freq > base_freq) ? 1 : 0;
	xgf_trace("over_boost=%d min_boost_freq[%d]=%d freq=%d base_freq=%d",
		over_boost, cid, min_boost_freq[cid], freq, base_freq);

	if (dequeuebuffer == 0)
		__xgf_cpufreq_cb_normal(cid, freq, over_boost);
	else
		__xgf_cpufreq_cb_skip(cid, freq, over_boost);
	spin_unlock_irqrestore(&xgf_slock, flags);
}

void xgf_dequeuebuffer(unsigned long arg)
{
	unsigned long flags;

	if (!game_mode)
		return;

	spin_lock_irqsave(&xgf_slock, flags);

	if (target_cluster == cluster_num) {
		dequeuebuffer = arg;
		spin_unlock_irqrestore(&xgf_slock, flags);
		return;
	}
	if (arg)
		__xgf_cpufreq_cb_flush();
	else {
		xgf_trace("dequeue obv=%d->%d time=%llu->%llu",
			  last_obv, clus_obv[target_cluster],
			  nsec_to_usec(last_cb_ts), nsec_to_usec(ged_get_time()));
		last_obv = clus_obv[target_cluster];
		last_cb_ts = ged_get_time();
	}
	dequeuebuffer = arg;

	xgf_trace("dequeuebuffer=%d", dequeuebuffer);
	fpsgo_systrace_c_fbt_gm(-100, last_obv, "deq_obv");
	spin_unlock_irqrestore(&xgf_slock, flags);

	fpsgo_systrace_c_fbt_gm(-100, arg, "dequeueBuffer");
}
EXPORT_SYMBOL(xgf_dequeuebuffer);

static void update_pwd_tbl(void)
{
	int cluster, i;

	for (cluster = 0; cluster < cluster_num ; cluster++) {
		for (i = 0; i < NR_FREQ_CPU; i++)
			cpu_dvfs[cluster].power[i] = mt_cpufreq_get_freq_by_idx(cluster, i);
	}
	cpu_max_freq = cpu_dvfs[cluster_num - 1].power[0];
}

static inline long long llabs(long long val)
{
	return (val < 0) ? -val : val;
}

static inline int fbt_is_self_control(unsigned long long t_cpu_target,
		unsigned long long deqtime_thr)
{
	long long diff;

	diff = (long long)t_cpu_target - (long long)q2q_time;
	if (llabs(diff) <= (long long)TIME_1MS)
		return 0;

	if (deq_time > deqtime_thr)
		return 0;

	return 1;
}

void fbt_check_self_control(unsigned long long t_cpu_target)
{
	unsigned long long thr;

	thr = t_cpu_target >> 4;

	if (middle_enable) {
		if (fbt_is_self_control(t_cpu_target, thr))
			sf_check = min(sf_bound_min, ++sf_check);
		else
			sf_check = 0;
		if (sf_check == sf_bound_min)
			middle_enable ^= 1;
		fpsgo_systrace_c_fbt_gm(-100, sf_bound_min, "sf_bound");

	} else {
		if (!fbt_is_self_control(t_cpu_target, thr))
			sf_check = min(sf_bound, ++sf_check);
		else
			sf_check = 0;
		if (sf_check == sf_bound)
			middle_enable ^= 1;
		fpsgo_systrace_c_fbt_gm(-100, sf_bound, "sf_bound");
	}
	fpsgo_systrace_c_fbt_gm(-100, sf_check, "sf_check");
	fpsgo_systrace_c_fbt_gm(-100, middle_enable, "middle_enable");

}

long long xgf_middle_vsync_check(long long t_cpu_target, unsigned long long t_cpu_slptime)
{
	unsigned long long next_vsync;
	unsigned long long queue_end;
	unsigned long long diff;
	int i;

	queue_end = ged_get_time();
	next_vsync = vsync_time;
	for (i = 1; i < 5; i++) {
		if (next_vsync > queue_end)
			break;
		next_vsync = vsync_time + (unsigned long long)(vsync_period * i);
	}
	diff = next_vsync - queue_end;
	xgf_trace("next_vsync=%llu vsync_time=%llu queue_end=%llu diff=%llu t_cpu_target=%llu",
	nsec_to_usec(next_vsync), nsec_to_usec(vsync_time),
	nsec_to_usec(queue_end), nsec_to_usec(diff), t_cpu_target);
	if (diff < vsync_distance)
		t_cpu_target = t_cpu_target - TIME_1MS;

	return t_cpu_target;
}

static int cmpint(const void *a, const void *b)
{
	return *(int *)a - *(int *)b;
}

void fbt_cpu_floor_check(long long t_cpu_cur, long long loading,
		unsigned int target_fps, long long t_cpu_target)
{
	int pre_iter = 0;
	int next_iter = 0;

	pre_iter = (f_iter - 1 + WINDOW) % WINDOW;
	next_iter = (f_iter + 1 + WINDOW) % WINDOW;

	if (target_fps > 50)
		frame_info[f_iter].target_fps = 60;
	else if (target_fps > 40)
		frame_info[f_iter].target_fps = 45;
	else
		frame_info[f_iter].target_fps = 30;

	if (!(frame_info[pre_iter].target_fps)) {
		frame_info[f_iter].mips = base_blc * t_cpu_cur + loading;
		xgf_trace("fbt_cpu floor first frame frame_info[%d].mips=%d q2q_time=%llu",
				f_iter, frame_info[f_iter].mips, frame_info[f_iter].q2q_time);
		f_iter++;
		f_iter = f_iter % WINDOW;
		return;
	}

	if (frame_info[f_iter].target_fps == frame_info[pre_iter].target_fps) {
		long long mips_diff;
		unsigned long long frame_time;
		unsigned long long frame_bound;

		frame_info[f_iter].mips = (long long)base_blc * t_cpu_cur + loading;
		mips_diff = (abs(frame_info[pre_iter].mips - frame_info[f_iter].mips) * 100);
		mips_diff = div64_s64(mips_diff, frame_info[f_iter].mips);
		mips_diff = min(mips_diff, 100LL);
		mips_diff = max(mips_diff, 1LL);
		xgf_trace("fbt_cpu floor frame_info[%d].mips=%lld frame_info[%d].mips=%lld mips_diff=%lld",
				f_iter, frame_info[f_iter].mips, pre_iter,
				frame_info[pre_iter].mips, mips_diff);
		frame_time = frame_info[pre_iter].q2q_time + frame_info[f_iter].q2q_time;
		frame_time = div64_u64(frame_time, (unsigned long long)NSEC_PER_USEC);
		xgf_trace("fbt_cpu floor frame_info[%d].q2q_time=%llu frame_info[%d].q2q_time=%llu",
			f_iter, frame_info[f_iter].q2q_time, pre_iter,
			frame_info[pre_iter].q2q_time);

		frame_info[f_iter].mips_diff = mips_diff;
		frame_info[f_iter].frame_time = frame_time;

		frame_bound = 21ULL * (unsigned long long)t_cpu_target;
		frame_bound = div64_u64(frame_bound, 10ULL);

		fpsgo_systrace_c_fbt_gm(-100, frame_bound, "frame_bound");

		if (mips_diff > variance && frame_time > frame_bound)
			frame_info[f_iter].count = 1;
		else
			frame_info[f_iter].count = 0;

		floor_count = floor_count + frame_info[f_iter].count -	frame_info[next_iter].count;

		xgf_trace("fbt_cpu floor frame_info[%d].count=%d frame_info[%d].count=%d",
			f_iter, frame_info[f_iter].count, next_iter,
			frame_info[next_iter].count);

		fpsgo_systrace_c_fbt_gm(-100, mips_diff, "mips_diff");
		fpsgo_systrace_c_fbt_gm(-100, frame_time, "frame_time");
		fpsgo_systrace_c_fbt_gm(-100, frame_info[f_iter].count, "count");
		fpsgo_systrace_c_fbt_gm(-100, floor_count, "floor_count");

		if (floor_count >= floor_bound) {
			int i;
			int array[WINDOW];

			for (i = 0; i < WINDOW; i++)
				array[i] = (int)frame_info[i].mips_diff;
			sort(array, WINDOW, sizeof(int), cmpint, NULL);
			floor = array[kmin - 1];
		}

		/*reset floor check*/

		if (floor > 0) {
			if (floor_count == 0) {
				int reset_bound;

				reset_bound = 5 * frame_info[f_iter].target_fps;
				reset_floor_bound++;
				reset_floor_bound = min(reset_floor_bound, reset_bound);

				if (reset_floor_bound == reset_bound) {
					floor = 0;
					reset_floor_bound = 0;
				}
			} else if (floor_count > 2) {
				reset_floor_bound = 0;
			}
		}
		fpsgo_systrace_c_fbt_gm(-100, reset_floor_bound, "reset_floor_bound");

		f_iter++;
		f_iter = f_iter % WINDOW;
	} else {
		/*reset frame time info*/
		memset(frame_info, 0, sizeof(frame_info));
		floor_count = 0;
		f_iter = 0;
	}

}

void xgf_set_cpuset_and_limit(unsigned int blc_wt)
{
	struct ppm_limit_data *pld;
	int *clus_opp;
	unsigned int *clus_floor_freq;
	unsigned long flags;
	unsigned int tgt_freq, floor_freq = 0;
	unsigned int mbhr;
	int mbhr_opp;
	int cluster, i = 0;
	int new_cluster;
	int move_cluster = 0;
	unsigned int max_freq = 0;
	unsigned int new_blc;

	clus_opp =
		kzalloc(cluster_num * sizeof(int), GFP_KERNEL);
	clus_floor_freq =
		kzalloc(cluster_num * sizeof(unsigned int), GFP_KERNEL);
	pld =
		kzalloc(cluster_num * sizeof(struct ppm_limit_data), GFP_KERNEL);

	fpsgo_systrace_c_log(floor, "floor");

	new_blc = (blc_wt * ((unsigned int)floor + 100));
	do_div(new_blc, 100U);
	new_blc = min(new_blc, 100U);
	new_blc = max(new_blc, 1U);

	tgt_freq = (min(blc_wt, 100U) * cpu_max_freq);
	do_div(tgt_freq, 100U);
	floor_freq = (min(new_blc, 100U) * cpu_max_freq);
	do_div(floor_freq, 100U);

	for (cluster = 0 ; cluster < cluster_num; cluster++) {
		for (i = (NR_FREQ_CPU - 1); i > 0; i--) {
			if (cpu_dvfs[cluster].power[i] >= tgt_freq)
				break;
		}
		clus_opp[cluster] = i;
		clus_floor_freq[cluster] = cpu_dvfs[cluster].power[i];
		clus_floor_freq[cluster] = min(clus_floor_freq[cluster], lpp_clus_max_freq[cluster]);
		xgf_trace("tgt_freq=%d cpu_dvfs[%d].power[%d]=%d clus_opp=%d clus_floor_freq=%d",
			 tgt_freq, cluster, i, cpu_dvfs[cluster].power[i],
			 clus_opp[cluster], clus_floor_freq[cluster]);
	}

	if (floor > 1) {
		int k, opp;

		for (k = (NR_FREQ_CPU - 1); k > 0; k--) {
			if (cpu_dvfs[target_cluster].power[k] >= floor_freq)
				break;
		}
		if (cpu_dvfs[target_cluster].power[k] == clus_floor_freq[target_cluster])
			opp = max((int)(k - floor_opp), 0);
		else
			opp = k;

		xgf_trace("fbt_cpu clus_floor_freq=%d cpu_dvfs[%d]->power[%d]=%d floor_freq=%d new_blc=%d",
			 clus_floor_freq[target_cluster], target_cluster, opp,
			 cpu_dvfs[target_cluster].power[opp], floor_freq, new_blc);
		clus_floor_freq[target_cluster] = cpu_dvfs[target_cluster].power[opp];
		clus_opp[target_cluster] = opp;
		bhr_opp = floor_opp;
	}

	for (cluster = 0 ; cluster < cluster_num; cluster++) {
		mbhr_opp = max((clus_opp[cluster] - bhr_opp), 0);
		mbhr = clus_floor_freq[cluster] * 100U;
		do_div(mbhr, cpu_max_freq);
		mbhr = mbhr + (unsigned int)bhr;
		mbhr = (min(mbhr, 100U) * cpu_max_freq);
		do_div(mbhr, 100U);
		for (i = (NR_FREQ_CPU - 1); i > 0; i--) {
			if (cpu_dvfs[cluster].power[i] > mbhr)
				break;
		}
		mbhr = cpu_dvfs[cluster].power[i];

		pld[cluster].max =
			max(mbhr, cpu_dvfs[cluster].power[mbhr_opp]);
		pld[cluster].max =
			min_t(unsigned int, pld[cluster].max, lpp_clus_max_freq[cluster]);

		if (pld[cluster].max > max_freq)
			max_freq = pld[cluster].max;

		xgf_trace("max=%d tar=[%d]=%d opp=[%d]=%d bhr=%d",
			 pld[cluster].max, clus_opp[cluster], tgt_freq,
			 mbhr_opp, cpu_dvfs[cluster].power[mbhr_opp], mbhr);
	}

	new_cluster = fbt_check_target_cluster(max_freq, FBT_CPU_FREQ_BOUND);

	spin_lock_irqsave(&xgf_slock, flags);
	if (target_cluster == cluster_num) {
		target_cluster = new_cluster;
		move_cluster = 1;
	}
	if (new_cluster == target_cluster) {
		history_blc_count--;
		history_blc_count = max(history_blc_count, 0);
	} else {
		history_blc_count++;
		history_blc_count = min(history_blc_count, migrate_bound);
		if (history_blc_count == migrate_bound) {
			target_cluster = new_cluster;
			move_cluster = 1;
			history_blc_count = 0;
		}
	}

	move_cluster = fbt_check_cluster_by_user_limit(max_freq, move_cluster);

	tgt_freq = clus_floor_freq[target_cluster];
	base_blc = tgt_freq * 100;
	do_div(base_blc, cpu_max_freq);
	base_freq = tgt_freq;

	for (cluster = 0 ; cluster < cluster_num; cluster++) {
		if (cluster != target_cluster)
			pld[cluster].max = clus_floor_freq[target_cluster];
		pld[cluster].min = -1;
		xgf_trace("pld[%d].max=%d min=%d",
			cluster, pld[cluster].max, pld[cluster].min);
	}

	fpsgo_systrace_c_log(target_cluster, "target_cluster");
	fpsgo_systrace_c_log(base_blc, "perf idx");
	spin_unlock_irqrestore(&xgf_slock, flags);

	fpsgo_systrace_c_fbt_gm(-100, history_blc_count, "history_blc_count");
	fpsgo_systrace_c_fbt_gm(-100, tgt_freq, "tgt_freq");

	if (move_cluster && cluster_num > 1 && fbt_cpuset_enable) {
		set_cpuset(target_cluster);
#if 0
		if (target_cluster)
			vcorefs_request_dvfs_opp(KIR_FBT, 2);
		else
			vcorefs_request_dvfs_opp(KIR_FBT, -1);
#endif
	}

	fbt_set_boost_value(target_cluster, base_blc);

	update_userlimit_cpu_freq(PPM_KIR_FBC, cluster_num, pld);
	for (cluster = 0 ; cluster < cluster_num; cluster++) {
		fpsgo_systrace_c_fbt_gm(-100, pld[cluster].max, "cluster%d limit freq", cluster);
		fpsgo_systrace_c_fbt_gm(-100, lpp_clus_max_freq[cluster], "lpp_clus_max_freq %d", cluster);
	}

	kfree(clus_opp);
	kfree(clus_floor_freq);
	kfree(pld);
}

void xgf_update_userlimit_freq(void)
{
	int i;
	int *clus_max_idx;
	unsigned int max_freq = 0U;
	unsigned long flags;

	clus_max_idx =
		kzalloc(cluster_num * sizeof(int), GFP_KERNEL);

	for (i = 0; i < cluster_num; i++)
		clus_max_idx[i] = mt_ppm_userlimit_freq_limit_by_others(cluster_num);

	spin_lock_irqsave(&xgf_slock, flags);

	for (i = 0; i < cluster_num; i++) {
		clus_max_freq[i] = cpu_dvfs[i].power[clus_max_idx[i]];
		if (clus_max_freq[i] > max_freq) {
			max_freq = clus_max_freq[i];
			max_freq_cluster = i;
		}
	}

	spin_unlock_irqrestore(&xgf_slock, flags);

	for (i = 0 ; i < cluster_num; i++)
		fpsgo_systrace_c_fbt_gm(-100, clus_max_freq[i], "cluster%d max freq", i);

	fpsgo_systrace_c_fbt_gm(-100, max_freq_cluster, "max_freq_cluster");
	kfree(clus_max_idx);
}

unsigned int xgf_get_max_userlimit_freq(void)
{
	unsigned int max_freq = 0U;
	unsigned int limited_cap;
	unsigned long flags;

	spin_lock_irqsave(&xgf_slock, flags);
	max_freq = clus_max_freq[max_freq_cluster];
	spin_unlock_irqrestore(&xgf_slock, flags);

	limited_cap = max_freq * 100U;
	do_div(limited_cap, cpu_max_freq);
	xgf_trace("max_freq=%d limited_cap=%d", max_freq, limited_cap);
	return limited_cap;
}

unsigned long long fbt_get_t2wnt(long long t_cpu_target)
{
	unsigned long long next_vsync, queue_end, queue_start;
	unsigned long long t2wnt;
	int i;
	unsigned long flags;

	spin_lock_irqsave(&xgf_slock, flags);

	queue_start = ged_get_time();
	queue_end = queue_start + t_cpu_target;
	next_vsync = vsync_time;
	for (i = 1; i < 6; i++) {
		if (next_vsync > queue_end)
			break;
		next_vsync = vsync_time + vsync_period * i;
	}
	t2wnt = next_vsync - rescue_distance - queue_start;
	xgf_trace("t2wnt=%llu next_vsync=%llu queue_end=%llu",
		nsec_to_usec(t2wnt), nsec_to_usec(next_vsync),
		nsec_to_usec(queue_end));

	spin_unlock_irqrestore(&xgf_slock, flags);

	return t2wnt;
}

void fbt_cpu_boost_policy(
	long long t_cpu_cur,
	long long t_cpu_target,
	unsigned long long t_cpu_slptime,
	unsigned int target_fps)
{
	unsigned int blc_wt = 0U;
	unsigned long long target_time = 0ULL;
	long long aa;
	unsigned long flags;
	unsigned int limited_cap;
	unsigned long long t1, t2, t_sleep;
	u64 t2wnt;
	struct hrtimer *timer;
	char reset_asfc = 0;

	if (!game_mode)
		return;

	xgf_trace("fbt_cpu_boost_policy");

	fpsgo_systrace_c_fbt_gm(-100, target_fps, "fps before using dfrc");
	target_fps = _gdfrc_fps_limit;
	t_cpu_target = _gdfrc_cpu_target;

	spin_lock_irqsave(&xgf_slock, flags);

	target_time = t_cpu_target;
	if (target_fps != TARGET_UNLIMITED_FPS) {
		target_time = (unsigned long long)FBTCPU_SEC_DIVIDER;
		target_time = div64_u64(target_time, (unsigned long long)(target_fps + RESET_TOLERENCE));
	}

	xgf_trace("update target_time=%llu t_cpu_target=%lld target_fps=%d",
		nsec_to_usec(target_time),
		nsec_to_usec((unsigned long long)t_cpu_target),
		target_fps);

	vsync_period = t_cpu_target;
	rescue_distance = t_cpu_target * (unsigned long long)rescue_percent;
	rescue_distance = div64_u64(rescue_distance, 100ULL);
	vsync_distance = t_cpu_target * (unsigned long long)vsync_percent;
	vsync_distance = div64_u64(vsync_distance, 100ULL);
	xgf_trace("update vsync_period=%d rescue_distance=%llu vsync_distance=%llu",
		vsync_period, nsec_to_usec(rescue_distance), nsec_to_usec(vsync_distance));

	if (lpp_fps || vag_fps) {
		if (lpp_fps)
			target_fps = lpp_fps;
		else
			target_fps = vag_fps;
		target_time = (unsigned long long)(FBTCPU_SEC_DIVIDER / target_fps);
		middle_enable = 0;
	} else
		fbt_check_self_control(target_time);

	if (asfc_last_fps == target_fps && asfc_time
		&& last_fps == TARGET_UNLIMITED_FPS) {
		unsigned long long asfc_tmp;

		asfc_tmp = ged_get_time();
		if (asfc_tmp - asfc_time < 300 * TIME_1MS)
			sf_bound = min(sf_bound_max, (sf_bound + sf_bound_min));
		xgf_trace("asfc_tmp=%llu asfc_time=%llu diff=%llu",
			nsec_to_usec(asfc_tmp), nsec_to_usec(asfc_time),
			nsec_to_usec((asfc_tmp - asfc_time)));
		xgf_trace("last_fps=%d", last_fps);
		fpsgo_systrace_c_fbt_gm(-100, sf_bound, "sf_bound");
		fpsgo_systrace_c_fbt_gm(-100, asfc_last_fps, "asfc_last_fps");
	}

	last_fps = target_fps;

	if (middle_enable) {
		target_time = xgf_middle_vsync_check(target_time, t_cpu_slptime);
		if (target_fps == 30) {
			unsigned long long deqstar_time;

			deqstar_time = deqend_time - deq_time;
			xgf_trace("deqend_time=%llu vsync_time=%llu deq_time=%d deqstar_time=%llu",
					nsec_to_usec(deqend_time), nsec_to_usec(vsync_time),
					nsec_to_usec(deq_time), nsec_to_usec(deqstar_time));
			if (deqend_time > vsync_time && deqstar_time < vsync_time
				&& deq_time > deqtime_bound) {
				xgf_trace("fbt_reset_asfc");
				asfc_time = ged_get_time();
				asfc_last_fps = target_fps;
				reset_asfc = 1;
			}
		}
	}

	aa = (long long)atomic_read(&loading_cur);
	t1 = (unsigned long long)t_cpu_cur;
	t1 = nsec_to_usec(t1);
	t2 = (unsigned long long)target_time;
	t2 = nsec_to_usec(t2);
	t_sleep = (t_cpu_slptime + deq_time);
	t_sleep = nsec_to_usec(t_sleep);
	if (t1 > (2 * t2) || (base_blc * t1 + aa) < 0) {
		blc_wt = base_blc;
		aa = 0;
	} else if (t_sleep) {
		long long new_aa;

		new_aa = aa * t1;
		new_aa = div64_s64(new_aa, (t1 + (long long)t_sleep));
		xgf_trace("new_aa = %lld aa = %lld", new_aa, aa);
		aa = new_aa;
		blc_wt = (unsigned int)(base_blc * t1 + new_aa);
		do_div(blc_wt, (unsigned int)t2);
	} else {
		blc_wt = (unsigned int)(base_blc * t1 + aa);
		do_div(blc_wt, (unsigned int)t2);
	}

	xgf_trace("perf_index=%d base=%d aa=%lld t_cpu_cur=%llu target=%llu sleep=%llu",
		blc_wt, base_blc, aa, t1, t2, t_sleep);

	if (!lpp_fps)
		fbt_cpu_floor_check(t1, aa, target_fps, t2);

	atomic_set(&loading_cur, 0);
	fpsgo_systrace_c_fbt_gm(-100, atomic_read(&loading_cur), "loading");
	spin_unlock_irqrestore(&xgf_slock, flags);

	if (middle_enable && reset_asfc)
		fbt_reset_asfc(0);

	blc_wt = min(blc_wt, 100U);
	blc_wt = max(blc_wt, 1U);
	blc_wt = min(blc_wt, lpp_max_cap);

	fpsgo_systrace_c_fbt_gm(-100, t_cpu_cur, "t_cpu_cur");
	fpsgo_systrace_c_fbt_gm(-100, target_time, "target_time");
	fpsgo_systrace_c_log(blc_wt, "perf idx");

	xgf_update_userlimit_freq();

	xgf_set_cpuset_and_limit(blc_wt);

	/*disable rescue frame for vag*/
	if (!lpp_fps || !vag_fps) {
		t2wnt = (u64) fbt_get_t2wnt(target_time);
		proc.active_jerk_id ^= 1;
		timer = &proc.jerks[proc.active_jerk_id].timer;
		hrtimer_start(timer, ns_to_ktime(t2wnt), HRTIMER_MODE_REL);
	}

	limited_cap = xgf_get_max_userlimit_freq();
	fpsgo_systrace_c_log(limited_cap, "limited_cap");
	fbt_notifier_push_cpu_capability(base_blc, limited_cap, target_fps);
}

void fbt_save_deq_q2q_info(unsigned long long deqtime, unsigned long long q2qtime)
{
	deq_time = deqtime;
	/* update and than refer to these data in one thread, lockless */
	q2q_time = q2qtime;

	frame_info[f_iter].q2q_time = q2qtime;
	xgf_trace("update deq_time=%llu q2qtime=%llu",
		nsec_to_usec(deq_time), nsec_to_usec(q2q_time));
}

int fbt_cpu_push_frame_time(pid_t pid, unsigned long long last_ts,
		unsigned long long curr_ts,
		unsigned long long *p_frmtime,
		unsigned long long *p_slptime)
{
	unsigned long long q2qtime;
	unsigned long long deqtime, slptime, blank_time;

	if (!game_mode)
		return 0;

	if (curr_ts < last_ts)
		return -EINVAL;

	q2qtime = curr_ts - last_ts;

	xgf_query_blank_time(pid, &deqtime, &slptime);
	blank_time = deqtime + slptime;

	if (q2qtime < blank_time || *p_frmtime < blank_time)
		return -EINVAL;

	*p_frmtime -= blank_time;
	*p_slptime = slptime;

	fpsgo_systrace_c_log(q2qtime, "get frame q2q time");
	fpsgo_systrace_c_log(slptime, "get frame sleep time");

	fbt_save_deq_q2q_info(deqtime, q2qtime);
	fstb_queue_time_update(curr_ts);
	fbt_notifier_push_cpu_frame_time(q2qtime, *p_frmtime);
	return 0;
}

void fbt_cpu_cpu_boost_check_01(
	int gx_game_mode,
	int gx_force_cpu_boost,
	int enable_cpu_boost,
	int ismainhead)
{
	fpsgo_systrace_c_log(gx_game_mode, "gx_game_mode");
	fpsgo_systrace_c_fbt_gm(-100, gx_force_cpu_boost, "gx_force_cpu_boost");
	fpsgo_systrace_c_fbt_gm(-100, enable_cpu_boost, "enable_cpu_boost");
	fpsgo_systrace_c_fbt_gm(-100, ismainhead, "psHead == main_head");
}

static int fbt_cpu_dfrc_ntf_cb(unsigned int fps_limit)
{
	unsigned long flags;

	_gdfrc_fps_limit_ex = _gdfrc_fps_limit;
	_gdfrc_fps_limit = fps_limit;

	/* clear middle_enable once fps changes */
	spin_lock_irqsave(&xgf_slock, flags);
	if (_gdfrc_fps_limit_ex != _gdfrc_fps_limit) {
		middle_enable = 0;
		fpsgo_systrace_c_fbt_gm(-100, 0, "middle_enable");
	}
	spin_unlock_irqrestore(&xgf_slock, flags);

	_gdfrc_cpu_target = FBTCPU_SEC_DIVIDER/fps_limit;

	return 0;
}

inline void fpsgo_sched_nominate(pid_t *tid, int *util) { }

int switch_fbt_cpuset(int enable)
{
	unsigned long flags;
	int last_enable;

	spin_lock_irqsave(&xgf_slock, flags);

	last_enable = fbt_cpuset_enable;
	fbt_cpuset_enable = enable;

	spin_unlock_irqrestore(&xgf_slock, flags);

	if (last_enable && !fbt_cpuset_enable) {
		prefer_idle_for_perf_idx(CGROUP_TA, 0);
		set_cpuset(-1);
	} else if (game_mode && !last_enable && fbt_cpuset_enable)
		prefer_idle_for_perf_idx(CGROUP_TA, 1);

	return 0;
}

#define FBT_DEBUGFS_ENTRY(name) \
static int fbt_##name##_open(struct inode *i, struct file *file) \
{ \
	return single_open(file, fbt_##name##_show, i->i_private); \
} \
\
static const struct file_operations fbt_##name##_fops = { \
	.owner = THIS_MODULE, \
	.open = fbt_##name##_open, \
	.read = seq_read, \
	.write = fbt_##name##_write, \
	.llseek = seq_lseek, \
	.release = single_release, \
}

static int fbt_switch_fbt_show(struct seq_file *m, void *unused)
{
	SEQ_printf(m, "fbt_enable:%d\n", fbt_enable);
	return 0;
}

static ssize_t fbt_switch_fbt_write(struct file *flip,
			const char *ubuf, size_t cnt, loff_t *data)
{
	int val;
	int ret;

	ret = kstrtoint_from_user(ubuf, cnt, 0, &val);
	if (ret)
		return ret;

	switch_fbt_game(val);

	return cnt;
}

FBT_DEBUGFS_ENTRY(switch_fbt);

static int fbt_set_vag_fps_show(struct seq_file *m, void *unused)
{
	SEQ_printf(m, "vag_fps:%d\n", vag_fps);
	return 0;
}

static ssize_t fbt_set_vag_fps_write(struct file *flip,
			const char *ubuf, size_t cnt, loff_t *data)
{
	int val;
	int ret;

	ret = kstrtoint_from_user(ubuf, cnt, 0, &val);
	if (ret)
		return ret;

	fbt_cpu_vag_set_fps(val);

	return cnt;
}

FBT_DEBUGFS_ENTRY(set_vag_fps);

static int fbt_lpp_max_cap_show(struct seq_file *m, void *unused)
{
	SEQ_printf(m, "lpp_max_cap:%d\n", lpp_max_cap);
	return 0;
}

static ssize_t fbt_lpp_max_cap_write(struct file *flip,
			const char *ubuf, size_t cnt, loff_t *data)
{
	int val;
	int ret;

	ret = kstrtoint_from_user(ubuf, cnt, 0, &val);
	if (ret)
		return ret;

	fbt_cpu_lppcap_update(val);

	return cnt;
}

FBT_DEBUGFS_ENTRY(lpp_max_cap);

static int fbt_lpp_fps_show(struct seq_file *m, void *unused)
{
	SEQ_printf(m, "lpp_fps:%d\n", lpp_fps);
	return 0;
}

static ssize_t fbt_lpp_fps_write(struct file *flip,
			const char *ubuf, size_t cnt, loff_t *data)
{
	int val;
	int ret;

	ret = kstrtoint_from_user(ubuf, cnt, 0, &val);
	if (ret)
		return ret;

	fbt_cpu_lpp_set_fps(val);

	return cnt;
}

FBT_DEBUGFS_ENTRY(lpp_fps);

static int fbt_switch_cpuset_show(struct seq_file *m, void *unused)
{
	SEQ_printf(m, "fbt_cpuset_enable:%d\n", fbt_cpuset_enable);
	return 0;
}

static ssize_t fbt_switch_cpuset_write(struct file *flip,
			const char *ubuf, size_t cnt, loff_t *data)
{
	int val;
	int ret;

	ret = kstrtoint_from_user(ubuf, cnt, 0, &val);
	if (ret)
		return ret;

	switch_fbt_cpuset(val);

	return cnt;
}

FBT_DEBUGFS_ENTRY(switch_cpuset);

void fbt_cpu_exit(void)
{
	kfree(clus_obv);
	kfree(cpu_dvfs);
	kfree(lpp_clus_max_freq);
}

int __init fbt_cpu_init(void)
{
	struct proc_dir_entry *pe;
	int cluster;

	bhr = 5;
	bhr_opp = 1;
	migrate_bound = 10;
	vsync_percent = 50;
	rescue_opp_c = (NR_FREQ_CPU - 1);
	rescue_opp_f = 5;
	rescue_percent = 33;
	vsync_period = GED_VSYNC_MISS_QUANTUM_NS;
	sf_bound_max = 10;
	sf_bound_min = 3;
	sf_bound = sf_bound_min;
	deqtime_bound = TIME_3MS;
	fbt_enable = 1;
	variance = 40;
	floor_bound = 3;
	floor = 0;
	kmin = 1;
	floor_opp = 2;
	_gdfrc_fps_limit    = TARGET_UNLIMITED_FPS;
	_gdfrc_fps_limit_ex = TARGET_UNLIMITED_FPS;
	fbt_cpuset_enable = 1;

	cluster_num = arch_get_nr_clusters();
	target_cluster = cluster_num;
	max_freq_cluster = min((cluster_num - 1), 0);

	clus_obv =
		kzalloc(cluster_num * sizeof(unsigned int), GFP_KERNEL);

	cpu_dvfs =
		kzalloc(cluster_num * sizeof(struct fbt_cpu_dvfs_info), GFP_KERNEL);

	lpp_clus_max_freq =
		kzalloc(cluster_num * sizeof(int), GFP_KERNEL);

	clus_max_freq =
		kzalloc(cluster_num * sizeof(int), GFP_KERNEL);

	/* GED */
	ged_kpi_set_game_hint_value_fp_fbt = fbt_notifier_push_game_mode;
	ged_kpi_cpu_boost_fp_fbt = fbt_cpu_boost_policy;

	ged_kpi_push_game_frame_time_fp_fbt = fbt_cpu_push_frame_time;
	ged_kpi_push_app_self_fc_fp_fbt = xgf_self_ctrl_enable;

	ged_kpi_cpu_boost_check_01 = fbt_cpu_cpu_boost_check_01;

	fbt_notifier_dfrc_fp_fbt_cpu = fbt_cpu_dfrc_ntf_cb;

	/* */
	cpufreq_notifier_fp = xgf_cpufreq_cb;
	ged_vsync_notifier_fp = xgf_ged_vsync;
	update_pwd_tbl();
	fbt_init_cpuset_freq_bound_table();

	lpp_max_cap = 100;
	for (cluster = 0 ; cluster < cluster_num; cluster++)
		lpp_clus_max_freq[cluster] = cpu_dvfs[cluster].power[0];

	/*llp*/
	update_lppcap = fbt_cpu_lppcap_update;
	set_fps = fbt_cpu_lpp_set_fps;

	fbt_init_jerk(&proc.jerks[0], 0);
	fbt_init_jerk(&proc.jerks[1], 1);

	if (!proc_mkdir("fbt_cpu", NULL))
		return -1;

	pe = proc_create("fbt_cpu/switch_fbt", 0660, NULL, &fbt_switch_fbt_fops);
	if (!pe)
		return -ENOMEM;

	pe = proc_create("fbt_cpu/set_vag_fps", 0660, NULL, &fbt_set_vag_fps_fops);
	if (!pe)
		return -ENOMEM;

	pe = proc_create("fbt_cpu/lpp_max_cap", 0660, NULL, &fbt_lpp_max_cap_fops);
	if (!pe)
		return -ENOMEM;

	pe = proc_create("fbt_cpu/lpp_fps", 0660, NULL, &fbt_lpp_fps_fops);
	if (!pe)
		return -ENOMEM;

	pe = proc_create("fbt_cpu/switch_cpuset", 0660, NULL, &fbt_switch_cpuset_fops);
	if (!pe)
		return -ENOMEM;

	return 0;
}
