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
#include <mt-plat/mtk_sched.h>
#include <linux/debugfs.h>
#include <linux/sort.h>
#include <linux/string.h>
#include <asm/div64.h>
#include <linux/topology.h>
#include <linux/slab.h>
#include <linux/hrtimer.h>
#include <linux/list.h>
#include <linux/kernel.h>

#include <fpsgo_common.h>
#include <fpsgo_v2_common.h>

#include <trace/events/fpsgo.h>
#include <mach/mtk_ppm_api.h>

#include "eas_controller.h"
#include "legacy_controller.h"

#include "fpsgo_base.h"
#include "fbt_usedext.h"
#include "fbt_cpu.h"
#include "fbt_cpu_platform.h"
#include "../fstb/fstb.h"
#include "xgf.h"
#include "fps_composer.h"

#define GED_VSYNC_MISS_QUANTUM_NS 16666666
#define TIME_3MS  3000000
#define TIME_1MS  1000000
#define TIME_100MS  100000000ULL
#define TIME_1S  1000000000ULL
#define TARGET_UNLIMITED_FPS 60
#define WINDOW 20
#define FBTCPU_SEC_DIVIDER 1000000000
#define RESET_TOLERENCE 2
#define NSEC_PER_HUSEC 100000

#define SEQ_printf(m, x...)\
do {\
	if (m)\
		seq_printf(m, x);\
	else\
		pr_debug(x);\
} while (0)

#ifdef NR_FREQ_CPU
struct fbt_cpu_dvfs_info {
	unsigned int power[NR_FREQ_CPU];
	unsigned long long capacity[NR_FREQ_CPU];
};
#endif

struct fbt_jerk {
	int id;
	int jerking;
	struct hrtimer timer;
	struct work_struct work;
};
struct fbt_proc {
	int active_jerk_id;
	struct fbt_jerk jerks[2];
};

struct fbt_frame_info {
	int target_fps;
	long long mips_diff;
	long long mips;
	unsigned long long q2q_time;
	int count;
};

struct fbt_boost_info {
	unsigned long long q2q_time;
	unsigned long long deq_time;
	int fstb_target_fps;
	int last_target_fps;
	unsigned int last_blc;

	/* rescue*/
	struct fbt_proc proc;
	unsigned long long rescue_distance;

	/* variance control */
	struct fbt_frame_info frame_info[WINDOW];
	unsigned long long floor;
	int floor_count;
	int reset_floor_bound;
	int f_iter;

	/* middle_enable */
	int middle_enable;
	unsigned long long vsync_distance;

	/* APP self-control*/
	unsigned long long deqend_time;
	unsigned long long asfc_time;
	unsigned long long asfc_last_fps;
	int sf_check;
	int sf_bound;
};

struct fbt_thread_loading {
	int pid;
	atomic_t loading;
	atomic_t last_cb_ts;
	atomic_t skip_loading;
	struct list_head entry;
};

struct fbt_thread_info {
	struct list_head entry;
	int pid;
	int frame_type;
	unsigned long long last_qustr_time;
	int boosting;
	int linger;
	struct mutex thr_mlock;
	struct fbt_boost_info boost_info;
	struct fbt_thread_loading *pLoading;
};

struct fbt_thread_bypass {
	int pid;
	struct list_head entry;
};

static struct dentry *fbt_debugfs_dir;

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

static int sf_bound_max;
static int sf_bound_min;

static DEFINE_SPINLOCK(xgf_slock);
static DEFINE_SPINLOCK(freq_slock);
static DEFINE_MUTEX(fbt_mlock);
static DEFINE_SPINLOCK(loading_slock);
static DEFINE_MUTEX(fbt_actlist_lock);
static DEFINE_MUTEX(bypasslist_lock);

int cluster_freq_bound[MAX_FREQ_BOUND_NUM] = {0};
int cluster_rescue_bound[MAX_FREQ_BOUND_NUM] = {0};
int cluster_num;

static struct list_head active_list;
static struct list_head loading_list;
static struct list_head bypass_list;

static int fbt_enable;
static int fbt_idleprefer_enable;
static int bypass_flag;
static int vag_fps;
static int walt_enable;
static int set_idleprefer;
static int suppress_ceiling;

static unsigned int cpu_max_freq;
static struct fbt_cpu_dvfs_info *cpu_dvfs;
static int *cpu_capacity_ratio;

static int *clus_max_freq;
static int max_freq_cluster;
static int max_cap_cluster;

static unsigned int *base_freq;

static unsigned int max_blc;
static int max_blc_pid;

static unsigned int *clus_obv;
static int last_cid;
static unsigned int last_obv;

static unsigned long long vsync_time;

static int _gdfrc_fps_limit;
static int _gdfrc_cpu_target;

/* lpp - should move to per-thread struct*/
static unsigned int lpp_max_cap;
static unsigned int lpp_fps;
static unsigned int *lpp_clus_max_freq;

static int nsec_to_100usec(unsigned long long nsec)
{
	unsigned long long husec;

	husec = div64_u64(nsec, (unsigned long long)NSEC_PER_HUSEC);

	return (int)husec;
}

static unsigned long long nsec_to_usec(unsigned long long nsec)
{
	unsigned long long usec;

	usec = div64_u64(nsec, (unsigned long long)NSEC_PER_USEC);

	return usec;
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

static void fbt_list_bypass_add(int pid)
{
	struct fbt_thread_bypass *obj;
	struct fbt_thread_bypass *pos, *next;
	int exist = 0;

	mutex_lock(&bypasslist_lock);

	list_for_each_entry_safe(pos, next, &bypass_list, entry) {
		if (pid == pos->pid) {
			exist = 1;
			break;
		}
	}

	if (!exist) {
		obj = kzalloc(sizeof(struct fbt_thread_bypass), GFP_KERNEL);
		if (!obj) {
			FPSGO_LOGE("ERROR OOM\n");
			mutex_unlock(&bypasslist_lock);
			return;
		}

		INIT_LIST_HEAD(&obj->entry);
		obj->pid = pid;
		list_add_tail(&obj->entry, &bypass_list);
	}

	mutex_unlock(&bypasslist_lock);
}

static void fbt_list_bypass_del(int pid)
{
	struct fbt_thread_bypass *pos, *next;

	list_for_each_entry_safe(pos, next, &bypass_list, entry) {
		if (pid == pos->pid) {
			list_del(&pos->entry);
			kfree(pos);
		}
	}
}

static int fbt_list_bypass_is_empty(void)
{
	return list_empty(&bypass_list);
}

static struct fbt_thread_loading *fbt_list_loading_add(int pid)
{
	struct fbt_thread_loading *obj;
	unsigned long flags;

	obj = kzalloc(sizeof(struct fbt_thread_loading), GFP_KERNEL);
	if (!obj) {
		FPSGO_LOGE("ERROR OOM\n");
		return NULL;
	}

	INIT_LIST_HEAD(&obj->entry);
	obj->pid = pid;

	spin_lock_irqsave(&loading_slock, flags);
	list_add_tail(&obj->entry, &loading_list);
	spin_unlock_irqrestore(&loading_slock, flags);

	return obj;
}

static void fbt_list_loading_del(struct fbt_thread_loading *obj)
{
	unsigned long flags;

	if (!obj)
		return;

	spin_lock_irqsave(&loading_slock, flags);
	list_del(&obj->entry);
	spin_unlock_irqrestore(&loading_slock, flags);

	kfree(obj);
}

static inline void fbt_init_jerk(struct fbt_jerk *jerk, int id);
static inline void fbt_list_main_lock(void)
{
	mutex_lock(&fbt_actlist_lock);
}

static inline void fbt_list_main_unlock(void)
{
	mutex_unlock(&fbt_actlist_lock);
}

static inline void fbt_list_thread_lock(struct mutex *mlock)
{
	mutex_lock(mlock);
}

static inline void fbt_list_thread_unlock(struct mutex *mlock)
{
	mutex_unlock(mlock);
}

static struct fbt_thread_info *fbt_list_add(int pid)
{
	struct fbt_thread_info *obj;
	struct fbt_boost_info *boost;
	struct fbt_thread_loading *link;

	obj = kzalloc(sizeof(struct fbt_thread_info), GFP_KERNEL);
	if (!obj)
		return NULL;

	INIT_LIST_HEAD(&obj->entry);

	obj->pid = pid;
	mutex_init(&obj->thr_mlock);

	boost = &(obj->boost_info);
	boost->sf_bound = sf_bound_min;
	boost->fstb_target_fps = TARGET_UNLIMITED_FPS;
	boost->last_target_fps = TARGET_UNLIMITED_FPS;
	fbt_init_jerk(&(boost->proc.jerks[0]), 0);
	fbt_init_jerk(&(boost->proc.jerks[1]), 1);

	link = fbt_list_loading_add(pid);
	obj->pLoading = link;

	list_add_tail(&obj->entry, &active_list);

	return obj;
}

static struct fbt_thread_info *fbt_list_find_add(int pid, int add)
{
	struct fbt_thread_info *pos, *next;
	struct fbt_thread_info *obj;

	list_for_each_entry_safe(pos, next, &active_list, entry) {
		if (pid == pos->pid)
			return pos;
	}

	if (add) {
		obj = fbt_list_add(pid);
		return obj;
	}

	return NULL;
}

static int fbt_list_find_noboost(void)
{
	struct fbt_thread_info *pos, *next;

	fbt_list_main_lock();

	list_for_each_entry_safe(pos, next, &active_list, entry) {
		if (pos->boosting == 1) {
			fbt_list_main_unlock();
			return 0;
		}
	}
	fbt_list_main_unlock();
	return 1;
}

static void fbt_list_find_next_max_blc(unsigned int *temp_max_blc,
									unsigned int *temp_max_blc_pid)
{
	struct fbt_thread_info *pos, *next;
	*temp_max_blc = 0;
	*temp_max_blc_pid = 0;

	fbt_list_main_lock();
	list_for_each_entry_safe(pos, next, &active_list, entry) {
		if (pos->boost_info.last_blc > *temp_max_blc && pos->boosting) {
			*temp_max_blc = pos->boost_info.last_blc;
			*temp_max_blc_pid = pos->pid;
		}
	}
	fbt_list_main_unlock();
}

static void fbt_list_clear(void)
{
	struct fbt_thread_info *pos, *next;
	struct fbt_thread_bypass *pos2, *next2;

	struct fbt_thread_bypass *pos3, *next3;
	struct list_head comp_delete_list;
	struct fbt_thread_bypass *obj;

	unsigned long flags;
	int delete = 0;

	INIT_LIST_HEAD(&comp_delete_list);

	fbt_list_main_lock();
	list_for_each_entry_safe(pos, next, &active_list, entry) {
		if (pos) {
			fbt_list_thread_lock(&pos->thr_mlock);
			list_del(&pos->entry);
			fbt_list_loading_del(pos->pLoading);

			if (pos->boost_info.proc.jerks[0].jerking == 0 &&
				pos->boost_info.proc.jerks[1].jerking == 0)
				delete = 1;
			else {
				delete = 0;
				pos->linger = 1;
			}
			fbt_list_thread_unlock(&pos->thr_mlock);

			obj = kzalloc(sizeof(struct fbt_thread_bypass), GFP_KERNEL);
			if (obj) {
				INIT_LIST_HEAD(&obj->entry);
				obj->pid = pos->pid;
				list_add_tail(&obj->entry, &comp_delete_list);
			}

			if (delete)
				kfree(pos);
		}
	}
	INIT_LIST_HEAD(&active_list);
	fbt_list_main_unlock();

	spin_lock_irqsave(&loading_slock, flags);
	INIT_LIST_HEAD(&loading_list);
	spin_unlock_irqrestore(&loading_slock, flags);

	mutex_lock(&bypasslist_lock);
	list_for_each_entry_safe(pos2, next2, &bypass_list, entry) {
		if (pos2) {
			list_del(&pos2->entry);
			kfree(pos2);
		}
	}
	INIT_LIST_HEAD(&bypass_list);
	mutex_unlock(&bypasslist_lock);

	if (!list_empty(&comp_delete_list)) {
		list_for_each_entry_safe(pos3, next3, &comp_delete_list, entry) {
			list_del(&pos3->entry);
			fpsgo_fbt2comp_destroy_frame_info(pos3->pid);
			kfree(pos3);
		}
	}
}

void fbt_check_thread_status(void)
{
	struct fbt_thread_info *pos, *next;
	unsigned long long ts = fpsgo_get_time();
	unsigned long long expire_ts;
	int delete_pid = 0;
	unsigned long flags;
	int delete = 0;
	int check_max_blc = 0;
	int temp_max_blc = 0;
	int temp_max_blc_pid = 0;

	struct list_head comp_delete_list;
	struct fbt_thread_bypass *obj;
	struct fbt_thread_bypass *pos2, *next2;

	if (ts < TIME_1S)
		return;

	expire_ts = ts - TIME_1S;

	INIT_LIST_HEAD(&comp_delete_list);

	fbt_list_main_lock();

	list_for_each_entry_safe(pos, next, &active_list, entry) {
		fbt_list_thread_lock(&pos->thr_mlock);
		if (pos->last_qustr_time < expire_ts) {
			delete_pid = pos->pid;

			if (delete_pid == max_blc_pid)
				check_max_blc = 1;

			list_del(&pos->entry);
			fbt_list_loading_del(pos->pLoading);

			if (pos->boost_info.proc.jerks[0].jerking == 0
				&& pos->boost_info.proc.jerks[1].jerking == 0)
				delete = 1;
			else {
				delete = 0;
				pos->linger = 1;
			}

			fbt_list_thread_unlock(&pos->thr_mlock);

			obj = kzalloc(sizeof(struct fbt_thread_bypass), GFP_KERNEL);
			if (obj) {
				INIT_LIST_HEAD(&obj->entry);
				obj->pid = delete_pid;
				list_add_tail(&obj->entry, &comp_delete_list);
			}

			if (delete == 1)
				kfree(pos);
		} else
			fbt_list_thread_unlock(&pos->thr_mlock);
	}

	if (check_max_blc) {
		list_for_each_entry_safe(pos, next, &active_list, entry) {
			fbt_list_thread_lock(&pos->thr_mlock);
			if (pos->boost_info.last_blc > temp_max_blc && pos->boosting) {
				temp_max_blc = pos->boost_info.last_blc;
				temp_max_blc_pid = pos->pid;
			}
			fbt_list_thread_unlock(&pos->thr_mlock);
		}

		spin_lock_irqsave(&xgf_slock, flags);
		max_blc = temp_max_blc;
		max_blc_pid = temp_max_blc_pid;
		xgf_trace("max_blc %d, max_blc_pid %d", max_blc, max_blc_pid);
		spin_unlock_irqrestore(&xgf_slock, flags);
	}

	fbt_list_main_unlock();

	if (!list_empty(&comp_delete_list)) {
		list_for_each_entry_safe(pos2, next2, &comp_delete_list, entry) {
			list_del(&pos2->entry);
			fpsgo_fbt2comp_destroy_frame_info(pos2->pid);
			kfree(pos2);
		}
	}
}

static void fbt_free_bhr(void)
{
	struct ppm_limit_data *pld;
	int i;

	pld =
		kzalloc(cluster_num * sizeof(struct ppm_limit_data), GFP_KERNEL);
	if (!pld) {
		FPSGO_LOGE("ERROR OOM %d\n", __LINE__);
		return;
	}

	for (i = 0; i < cluster_num; i++) {
		pld[i].max = -1;
		pld[i].min = -1;
	}

	xgf_trace("fpsgo free bhr");

	update_userlimit_cpu_freq(PPM_KIR_FBC, cluster_num, pld);
	kfree(pld);
}

static void fbt_get_new_base_blc(unsigned int *blc_wt, struct ppm_limit_data *pld)
{
	unsigned int *blc_freq;
	int i, cluster;
	int *clus_opp;
	unsigned long flags;

	if (!blc_wt || !pld) {
		FPSGO_LOGE("ERROR %d\n", __LINE__);
		return;
	}

	clus_opp =
		kzalloc(cluster_num * sizeof(int), GFP_KERNEL);
	if (!clus_opp) {
		FPSGO_LOGE("ERROR OOM %d\n", __LINE__);
		return;
	}

	blc_freq =
		kzalloc(cluster_num * sizeof(unsigned int), GFP_KERNEL);
	if (!blc_freq) {
		kfree(clus_opp);
		FPSGO_LOGE("ERROR OOM %d\n", __LINE__);
		return;
	}

	spin_lock_irqsave(&xgf_slock, flags);

	for (cluster = 0 ; cluster < cluster_num; cluster++) {
		for (i = (NR_FREQ_CPU - 1); i > 0; i--) {
			if (cpu_dvfs[cluster].power[i] >= base_freq[cluster])
				break;
		}
		clus_opp[cluster] = i;

		blc_freq[cluster] =
			cpu_dvfs[cluster].power[max((clus_opp[cluster] - rescue_opp_f), 0)];
		blc_freq[cluster] = min_t(unsigned int, blc_freq[cluster], lpp_clus_max_freq[cluster]);

		blc_wt[cluster] = blc_freq[cluster] * 100;
		do_div(blc_wt[cluster], cpu_max_freq);
		blc_wt[cluster] = clamp(blc_wt[cluster], 1U, 100U);

		pld[cluster].min = -1;

		if (bypass_flag == 0 && suppress_ceiling) {
			pld[cluster].max =
				cpu_dvfs[cluster].power[max((clus_opp[cluster] - rescue_opp_c), 0)];
			pld[cluster].max = min_t(unsigned int, pld[cluster].max, lpp_clus_max_freq[cluster]);
		} else
			pld[cluster].max = -1;
	}

	spin_unlock_irqrestore(&xgf_slock, flags);

	kfree(clus_opp);
	kfree(blc_freq);
}

static void fbt_do_jerk(struct work_struct *work)
{
	struct fbt_jerk *jerk;
	struct fbt_proc *proc;
	struct fbt_boost_info *boost;
	struct fbt_thread_info *thr;
	int cluster;
	int tofree = 0;

	jerk = container_of(work, struct fbt_jerk, work);
	if (!jerk || (jerk->id != 0 && jerk->id != 1)) {
		FPSGO_LOGE("ERROR %d\n", __LINE__);
		return;
	}

	proc = container_of(jerk, struct fbt_proc, jerks[jerk->id]);
	if (!proc || (proc->active_jerk_id != 0 && proc->active_jerk_id != 1)) {
		FPSGO_LOGE("ERROR %d\n", __LINE__);
		return;
	}

	boost = container_of(proc, struct fbt_boost_info, proc);
	if (!boost) {
		FPSGO_LOGE("ERROR %d\n", __LINE__);
		return;
	}

	thr = container_of(boost, struct fbt_thread_info, boost_info);
	if (!thr) {
		FPSGO_LOGE("ERROR %d\n", __LINE__);
		return;
	}

	fbt_list_main_lock();
	fbt_list_thread_lock(&(thr->thr_mlock));

	if (jerk->id == proc->active_jerk_id && thr->boosting && thr->linger == 0) {
		unsigned int *blc_wt;
		struct ppm_limit_data *pld;

		blc_wt = kcalloc(cluster_num, sizeof(unsigned int), GFP_KERNEL);
		pld = kcalloc(cluster_num, sizeof(struct ppm_limit_data), GFP_KERNEL);
		if (blc_wt && pld) {
			fbt_get_new_base_blc(blc_wt, pld);
			for (cluster = 0 ; cluster < cluster_num; cluster++) {
				fpsgo_systrace_c_fbt(thr->pid, blc_wt[cluster], "cluster%d perf_index", cluster);
				fpsgo_systrace_c_fbt(thr->pid, pld[cluster].max, "cluster%d ceiling_freq", cluster);
				fbt_set_boost_value(cluster, blc_wt[cluster]);
			}
			update_userlimit_cpu_freq(PPM_KIR_FBC, cluster_num, pld);
		}
		kfree(blc_wt);
		kfree(pld);
	}

	jerk->jerking = 0;

	if (thr->boost_info.proc.jerks[0].jerking == 0 &&
		thr->boost_info.proc.jerks[1].jerking == 0 &&
		thr->linger > 0)
		tofree = 1;

	fbt_list_thread_unlock(&(thr->thr_mlock));

	if (tofree)
		kfree(thr);

	fbt_list_main_unlock();
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

static inline long long llabs(long long val)
{
	return (val < 0) ? -val : val;
}

static inline int fbt_is_self_control(unsigned long long t_cpu_target,
		unsigned long long deqtime_thr, unsigned long long q2q_time,
		unsigned long long deq_time)
{
	long long diff;

	diff = (long long)t_cpu_target - (long long)q2q_time;
	if (llabs(diff) <= (long long)TIME_1MS)
		return 0;

	if (deq_time > deqtime_thr)
		return 0;

	return 1;
}

static int fbt_check_self_control(unsigned long long t_cpu_target, int *middle_enable,
											int *sf_check, int sf_bound,
											unsigned long long q2q_time,
											unsigned long long deq_time)
{
	unsigned long long thr;
	int ret = 0;

	if (!middle_enable || !sf_check) {
		FPSGO_LOGE("ERROR %d\n", __LINE__);
		return 0;
	}

	thr = t_cpu_target >> 4;

	if (*middle_enable) {
		if (fbt_is_self_control(t_cpu_target, thr, q2q_time, deq_time))
			*sf_check = min(sf_bound_min, ++(*sf_check));
		else
			*sf_check = 0;
		if (*sf_check == sf_bound_min) {
			(*middle_enable) ^= 1;
			ret = 1;
		}
	} else {
		if (!fbt_is_self_control(t_cpu_target, thr, q2q_time, deq_time))
			*sf_check = min(sf_bound, ++(*sf_check));
		else
			*sf_check = 0;
		if (*sf_check == sf_bound)
			(*middle_enable) ^= 1;
	}

	return ret;
}

static long long fbt_middle_vsync_check(long long t_cpu_target,
		unsigned long long t_cpu_slptime, unsigned long long vsync_distance,
		unsigned long long queue_end)
{
	unsigned long long next_vsync;
	unsigned long long diff;
	int i;

	if (vsync_time == 0)
		return t_cpu_target;

	next_vsync = vsync_time;
	for (i = 1; i < 5; i++) {
		if (next_vsync > queue_end)
			break;
		next_vsync = vsync_time + (unsigned long long)(vsync_period * i);
	}

	if (next_vsync < queue_end)
		return t_cpu_target;

	diff = next_vsync - queue_end;

	if (diff < vsync_distance)
		t_cpu_target = t_cpu_target - TIME_1MS;

	return t_cpu_target;
}

static int cmpint(const void *a, const void *b)
{
	return *(int *)a - *(int *)b;
}

static void fbt_check_var(long long loading,
		unsigned int target_fps, long long t_cpu_target,
		int *f_iter, struct fbt_frame_info *frame_info,
		unsigned long long *floor, int *floor_count, int *reset_floor_bound)
{
	int pre_iter = 0;
	int next_iter = 0;

	if (!f_iter || !frame_info || !floor || !floor_count || !reset_floor_bound) {
		FPSGO_LOGE("ERROR %d\n", __LINE__);
		return;
	}
	if (*f_iter >= WINDOW) {
		FPSGO_LOGE("ERROR %d\n", __LINE__);
		*f_iter = 0;
		return;
	}

	pre_iter = (*f_iter - 1 + WINDOW) % WINDOW;
	next_iter = (*f_iter + 1 + WINDOW) % WINDOW;

	if (target_fps > 50)
		frame_info[*f_iter].target_fps = 60;
	else if (target_fps > 40)
		frame_info[*f_iter].target_fps = 45;
	else
		frame_info[*f_iter].target_fps = 30;

	if (!(frame_info[pre_iter].target_fps)) {
		frame_info[*f_iter].mips = loading;
		xgf_trace("first frame frame_info[%d].mips=%d q2q_time=%llu",
				*f_iter, frame_info[*f_iter].mips, frame_info[*f_iter].q2q_time);
		(*f_iter)++;
		*f_iter = *f_iter % WINDOW;
		return;
	}

	if (frame_info[*f_iter].target_fps == frame_info[pre_iter].target_fps) {
		long long mips_diff;
		unsigned long long frame_time;
		unsigned long long frame_bound;

		frame_info[*f_iter].mips = loading;
		mips_diff = (abs(frame_info[pre_iter].mips - frame_info[*f_iter].mips) * 100);
		if (frame_info[*f_iter].mips != 0)
			mips_diff = div64_s64(mips_diff, frame_info[*f_iter].mips);
		else
			mips_diff = frame_info[pre_iter].mips;
		mips_diff = clamp(mips_diff, 1LL, 100LL);
		frame_time = frame_info[pre_iter].q2q_time + frame_info[*f_iter].q2q_time;
		frame_time = div64_u64(frame_time, (unsigned long long)NSEC_PER_USEC);

		frame_info[*f_iter].mips_diff = mips_diff;

		frame_bound = 21ULL * (unsigned long long)t_cpu_target;
		frame_bound = div64_u64(frame_bound, 10ULL);

		if (mips_diff > variance && frame_time > frame_bound)
			frame_info[*f_iter].count = 1;
		else
			frame_info[*f_iter].count = 0;

		*floor_count = *floor_count + frame_info[*f_iter].count - frame_info[next_iter].count;

		xgf_trace("frame_info[%d].mips=%lld diff=%lld q2q=%llu count=%d floor_count=%d",
				*f_iter, frame_info[*f_iter].mips, mips_diff, frame_info[*f_iter].q2q_time,
				frame_info[*f_iter].count, *floor_count);

		if (*floor_count >= floor_bound) {
			int i;
			int array[WINDOW];

			for (i = 0; i < WINDOW; i++)
				array[i] = (int)frame_info[i].mips_diff;
			sort(array, WINDOW, sizeof(int), cmpint, NULL);
			*floor = array[kmin - 1];
		}

		/*reset floor check*/
		if (*floor > 0) {
			if (*floor_count == 0) {
				int reset_bound;

				reset_bound = 5 * frame_info[*f_iter].target_fps;
				(*reset_floor_bound)++;
				*reset_floor_bound = min(*reset_floor_bound, reset_bound);

				if (*reset_floor_bound == reset_bound) {
					*floor = 0;
					*reset_floor_bound = 0;
				}
			} else if (*floor_count > 2) {
				*reset_floor_bound = 0;
			}
		}

		(*f_iter)++;
		*f_iter = *f_iter % WINDOW;
	}	else {
		/*reset frame time info*/
		memset(frame_info, 0, WINDOW * sizeof(struct fbt_frame_info));
		*floor_count = 0;
		*f_iter = 0;
	}
}

static void fbt_do_boost(int *clus_opp, unsigned int *clus_floor_freq, int pid)
{
	struct ppm_limit_data *pld;
	unsigned int tgt_freq;
	unsigned int mbhr;
	int mbhr_opp;
	unsigned int final_blc;
	int cluster, i = 0;

	if (!clus_opp || !clus_floor_freq) {
		FPSGO_LOGE("ERROR %d\n", __LINE__);
		return;
	}

	pld =
		kzalloc(cluster_num * sizeof(struct ppm_limit_data), GFP_KERNEL);
	if (!pld) {
		FPSGO_LOGE("ERROR OOM %d\n", __LINE__);
		return;
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

		pld[cluster].min = -1;

		if (bypass_flag == 0 && suppress_ceiling) {
			pld[cluster].max =
				max(mbhr, cpu_dvfs[cluster].power[mbhr_opp]);
			pld[cluster].max =
				min_t(unsigned int, pld[cluster].max, lpp_clus_max_freq[cluster]);
		} else
			pld[cluster].max = -1;

		tgt_freq = clus_floor_freq[cluster];
		base_freq[cluster] = tgt_freq;

		final_blc = tgt_freq * 100;
		do_div(final_blc, cpu_max_freq);
		final_blc = clamp(final_blc, 1U, 100U);

		fbt_set_boost_value(cluster, final_blc);

		fpsgo_systrace_c_fbt(pid, final_blc, "cluster%d perf_index", cluster);
		fpsgo_systrace_c_fbt(pid, base_freq[cluster], "cluster%d floor_freq", cluster);
		fpsgo_systrace_c_fbt(pid, pld[cluster].max, "cluster%d ceiling_freq", cluster);
	}

	if (bypass_flag == 0)
		update_userlimit_cpu_freq(PPM_KIR_FBC, cluster_num, pld);

	kfree(pld);
}

static int fbt_set_limit(unsigned int blc_wt, unsigned long long floor, int pid)
{
	int *clus_opp;
	unsigned int *clus_floor_freq;

	unsigned int tgt_freq;
	int tgt_opp = 0;
	unsigned int orig_freq = 0;
	int orig_opp = 0;

	int cluster;
	int doboost = 0;
	unsigned int temp_freq;

	clus_opp =
		kzalloc(cluster_num * sizeof(int), GFP_KERNEL);
	if (!clus_opp) {
		FPSGO_LOGE("ERROR OOM %d\n", __LINE__);
		return blc_wt;
	}

	clus_floor_freq =
		kzalloc(cluster_num * sizeof(unsigned int), GFP_KERNEL);
	if (!clus_floor_freq) {
		kfree(clus_opp);
		FPSGO_LOGE("ERROR OOM %d\n", __LINE__);
		return blc_wt;
	}

	if (cpu_max_freq <= 1)
		fpsgo_systrace_c_fbt(pid, cpu_max_freq, "cpu_max_freq");

	if (floor > 1) {
		orig_freq = blc_wt * cpu_max_freq;
		do_div(orig_freq, 100U);

		blc_wt = (blc_wt * ((unsigned int)floor + 100));
		do_div(blc_wt, 100U);
		blc_wt = clamp(blc_wt, 1U, 100U);
	}

	tgt_freq = blc_wt * cpu_max_freq;
	do_div(tgt_freq, 100U);

	for (cluster = 0; cluster < cluster_num; cluster++) {
		if (fbt_is_mips_different() && (cluster != max_cap_cluster))
			temp_freq = (tgt_freq * cpu_capacity_ratio[cluster]) >> 7;
		else
			temp_freq = tgt_freq;

		for (tgt_opp = (NR_FREQ_CPU - 1); tgt_opp > 0; tgt_opp--) {
			if (cpu_dvfs[cluster].power[tgt_opp] >= temp_freq)
				break;
		}

		if (floor > 1) {
			if (fbt_is_mips_different() && (cluster != max_cap_cluster))
				temp_freq = (orig_freq * cpu_capacity_ratio[cluster]) >> 7;
			else
				temp_freq = orig_freq;

			for (orig_opp = (NR_FREQ_CPU - 1); orig_opp > 0; orig_opp--) {
				if (cpu_dvfs[cluster].power[orig_opp] >= temp_freq)
					break;
			}

			if (orig_freq != tgt_freq && tgt_opp == orig_opp)
				tgt_opp = max((int)(orig_opp - floor_opp), 0);
		}

		clus_floor_freq[cluster] = cpu_dvfs[cluster].power[tgt_opp];
		clus_opp[cluster] = tgt_opp;

		xgf_trace("cluster %d, opp %d, floor_freq %d",
			cluster, clus_opp[cluster], clus_floor_freq[cluster]);
	}

	blc_wt = clus_floor_freq[max_freq_cluster] * 100;
	do_div(blc_wt, cpu_max_freq);
	blc_wt = clamp(blc_wt, 1U, 100U);

	if (blc_wt > max_blc || pid == max_blc_pid) {
		max_blc = blc_wt;
		max_blc_pid = pid;
		doboost = 1;
	}
	xgf_trace("max_blc %d, max_blc_pid %d", max_blc, max_blc_pid);

	if (doboost == 1)
		fbt_do_boost(clus_opp, clus_floor_freq, pid);

	kfree(clus_opp);
	kfree(clus_floor_freq);

	return blc_wt;
}

static void fbt_update_userlimit_freq(void)
{
	int i;
	int *clus_max_idx;
	unsigned int max_freq = 0U;
	unsigned long flags;

	clus_max_idx =
		kzalloc(cluster_num * sizeof(int), GFP_KERNEL);

	if (!clus_max_idx)
		return;

	for (i = 0; i < cluster_num; i++)
		clus_max_idx[i] = mt_ppm_userlimit_freq_limit_by_others(cluster_num);

	spin_lock_irqsave(&xgf_slock, flags);

	for (i = 0; i < cluster_num; i++) {
		clus_max_freq[i] = cpu_dvfs[i].power[clus_max_idx[i]];
		if (clus_max_freq[i] > max_freq) {
			max_freq = clus_max_freq[i];
			max_freq_cluster = i;
		} else if (clus_max_freq[i] == max_freq
			&& cpu_capacity_ratio[i] < cpu_capacity_ratio[max_freq_cluster]) {
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

static unsigned int fbt_get_max_userlimit_freq(void)
{
	unsigned int max_freq = 0U;
	unsigned int limited_cap;
	unsigned long flags;

	spin_lock_irqsave(&xgf_slock, flags);
	max_freq = clus_max_freq[max_freq_cluster];
	spin_unlock_irqrestore(&xgf_slock, flags);

	limited_cap = max_freq * 100U;
	do_div(limited_cap, cpu_max_freq);
	return limited_cap;
}

static unsigned long long fbt_get_t2wnt(long long t_cpu_target, unsigned long long rescue_distance,
										unsigned long long queue_start)
{
	unsigned long long next_vsync, queue_end;
	unsigned long long t2wnt = 0ULL;
	int i;
	unsigned long flags;

	spin_lock_irqsave(&xgf_slock, flags);

	if (vsync_time == 0) {
		spin_unlock_irqrestore(&xgf_slock, flags);
		return 0;
	}

	queue_end = queue_start + t_cpu_target;
	next_vsync = vsync_time;
	for (i = 1; i < 6; i++) {
		if (next_vsync > queue_end)
			break;
		next_vsync = vsync_time + vsync_period * i;
	}

	if (next_vsync < queue_end) {
		spin_unlock_irqrestore(&xgf_slock, flags);
		return 0;
	}

	if (next_vsync - queue_start < rescue_distance) {
		FPSGO_LOGE("ERROR %d\n", __LINE__);
		spin_unlock_irqrestore(&xgf_slock, flags);
		return 0;
	}

	t2wnt = next_vsync - rescue_distance - queue_start;

	if (t2wnt > TIME_100MS) {
		FPSGO_LOGE("ERROR %d\n", __LINE__);
		spin_unlock_irqrestore(&xgf_slock, flags);
		return 0;
	}

	spin_unlock_irqrestore(&xgf_slock, flags);

	return t2wnt;
}

int (*fpsgo_fbt2fstb_cpu_capability_fp)(
	int pid, int frame_type,
	unsigned int curr_cap,
	unsigned int max_cap,
	unsigned int target_fps,
	unsigned long long Q2Q_time,
	unsigned long long Running_time);

static int fbt_boost_policy(
	long long t_cpu_cur,
	long long t_cpu_target,
	unsigned long long t_cpu_slptime,
	unsigned int target_fps,
	struct fbt_thread_info *thread_info,
	unsigned long long ts)
{
	unsigned int blc_wt = 0U;
	unsigned long long target_time = 0ULL;
	long aa;
	unsigned long long temp_blc;
	unsigned long flags, flags2;
	unsigned long long t1, t2, t_sleep;
	char reset_asfc = 0;
	struct fbt_boost_info *boost_info;
	int pid;
	int frame_type;
	struct hrtimer *timer;
	u64 t2wnt = 0ULL;
	int active_jerk_id = 0;

	if (!thread_info) {
		FPSGO_LOGE("ERROR %d\n", __LINE__);
		return 0;
	}

	spin_lock_irqsave(&xgf_slock, flags);

	pid = thread_info->pid;
	frame_type = thread_info->frame_type;
	boost_info = &(thread_info->boost_info);
	target_time = t_cpu_target;

	if (target_fps != TARGET_UNLIMITED_FPS) {
		target_time = (unsigned long long)FBTCPU_SEC_DIVIDER;
		target_time = div64_u64(target_time, (unsigned long long)(target_fps + RESET_TOLERENCE));
	}

	vsync_period = _gdfrc_cpu_target;
	boost_info->rescue_distance = _gdfrc_cpu_target * (unsigned long long)rescue_percent;
	boost_info->rescue_distance = div64_u64(boost_info->rescue_distance, 100ULL);
	boost_info->vsync_distance = _gdfrc_cpu_target * (unsigned long long)vsync_percent;
	boost_info->vsync_distance = div64_u64(boost_info->vsync_distance, 100ULL);

	if (lpp_fps || vag_fps) {
		if (lpp_fps)
			target_fps = lpp_fps;
		else
			target_fps = vag_fps;
		target_time = (unsigned long long)(FBTCPU_SEC_DIVIDER / target_fps);
		boost_info->middle_enable = 0;
	} else if (frame_type == NON_VSYNC_ALIGNED_TYPE) {
		fbt_check_self_control(target_time, &boost_info->middle_enable,
				&boost_info->sf_check, boost_info->sf_bound,
				boost_info->q2q_time, boost_info->deq_time);

		if (target_fps != _gdfrc_fps_limit)
			boost_info->middle_enable = 0;
	}

	if (boost_info->asfc_last_fps == target_fps && boost_info->asfc_time
		&& boost_info->last_target_fps == TARGET_UNLIMITED_FPS
		&& frame_type == NON_VSYNC_ALIGNED_TYPE) {
		unsigned long long asfc_tmp;

		asfc_tmp = ts;
		if (asfc_tmp - boost_info->asfc_time < 300 * TIME_1MS)
			boost_info->sf_bound = min(sf_bound_max, (boost_info->sf_bound + sf_bound_min));
		fpsgo_systrace_c_fbt_gm(pid, boost_info->sf_bound, "sf_bound");
	}

	boost_info->last_target_fps = target_fps;
	fpsgo_systrace_c_fbt(pid, boost_info->middle_enable, "middle_enable");
	fpsgo_systrace_c_fbt_gm(pid, boost_info->sf_check, "sf_check");

	if (boost_info->middle_enable) {
		target_time = fbt_middle_vsync_check(target_time, t_cpu_slptime, boost_info->vsync_distance, ts);
		if (target_fps == 30) {
			unsigned long long deqstar_time;

			deqstar_time = boost_info->deqend_time - boost_info->deq_time;

			if (boost_info->deqend_time > vsync_time && deqstar_time < vsync_time
				&& boost_info->deq_time > deqtime_bound) {
				boost_info->asfc_time = ts;
				boost_info->asfc_last_fps = target_fps;
				reset_asfc = 1;
			}
		}
	}
	fpsgo_systrace_c_fbt_gm(pid, target_time, "target_time");

	spin_lock_irqsave(&loading_slock, flags2);
	aa = atomic_read(&thread_info->pLoading->loading);
	spin_unlock_irqrestore(&loading_slock, flags2);
	t1 = (unsigned long long)t_cpu_cur;
	t1 = nsec_to_100usec(t1);
	t2 = target_time;
	t2 = nsec_to_100usec(t2);
	t_sleep = (t_cpu_slptime + boost_info->deq_time);
	t_sleep = nsec_to_100usec(t_sleep);
	if (frame_type == NON_VSYNC_ALIGNED_TYPE && (t1 > (2 * t2) || aa < 0)) {
		blc_wt = boost_info->last_blc;
		aa = 0;
	} else if (t_sleep) {
		long long new_aa;

		new_aa = aa * t1;
		new_aa = div64_s64(new_aa, (t1 + (long long)t_sleep));
		aa = new_aa;
		temp_blc = new_aa;
		do_div(temp_blc, (unsigned int)t2);
		blc_wt = (unsigned int)temp_blc;
	} else {
		temp_blc = aa;
		do_div(temp_blc, (unsigned int)t2);
		blc_wt = (unsigned int)temp_blc;
	}

	xgf_trace("perf_index=%d aa=%lld t_cpu_cur=%llu target=%llu sleep=%llu",
		blc_wt, aa, t1, t2, t_sleep);

	if (!lpp_fps)
		fbt_check_var(aa, target_fps, nsec_to_usec(target_time),
		&(boost_info->f_iter), &(boost_info->frame_info[0]), &(boost_info->floor),
		&(boost_info->floor_count), &(boost_info->reset_floor_bound));
	fpsgo_systrace_c_fbt(pid, boost_info->floor, "variance");

	spin_unlock_irqrestore(&xgf_slock, flags);

	spin_lock_irqsave(&loading_slock, flags);
	atomic_set(&thread_info->pLoading->loading, 0);
	fpsgo_systrace_c_fbt_gm(pid, atomic_read(&thread_info->pLoading->loading), "loading");
	spin_unlock_irqrestore(&loading_slock, flags);

	if (boost_info->middle_enable && reset_asfc)
		fpsgo_fbt2fstb_reset_asfc(pid, 0);

	fbt_update_userlimit_freq();

	blc_wt = min(blc_wt, lpp_max_cap);
	blc_wt = clamp(blc_wt, 1U, 100U);

	blc_wt = fbt_set_limit(blc_wt, boost_info->floor, pid);

	boost_info->last_blc = blc_wt;
	fpsgo_systrace_c_fbt(pid, blc_wt, "perf idx");

	/*disable rescue frame for vag*/
	if (!lpp_fps || !vag_fps) {
		t2wnt = (u64) fbt_get_t2wnt(target_time,
									boost_info->rescue_distance, ts);
		if (t2wnt) {
			boost_info->proc.active_jerk_id ^= 1;
			active_jerk_id = boost_info->proc.active_jerk_id;

			timer = &(boost_info->proc.jerks[active_jerk_id].timer);
			if (timer) {
				if (boost_info->proc.jerks[active_jerk_id].jerking == 0) {
					boost_info->proc.jerks[active_jerk_id].jerking = 1;
					hrtimer_start(timer, ns_to_ktime(t2wnt), HRTIMER_MODE_REL);
				}
			} else
				FPSGO_LOGE("ERROR timer\n");
		}
	}

	return blc_wt;
}

static unsigned long long fbt_est_loading(int cur_ts, atomic_t last_ts, unsigned int obv)
{
	if (obv == 0)
		return 0;

	if (atomic_read(&last_ts) == 0)
		return 0;

	if (cur_ts > atomic_read(&last_ts)) {
		unsigned long long dur = cur_ts - atomic_read(&last_ts);

		return dur * obv;
	}

	return 0;
/*
*	Since qu/deq notification may be delayed because of wq,
*	loading could be over estimated if qu/deq start comes later.
*	Keep the current design, and be awared of low power.
*/
}

void fpsgo_ctrl2fbt_cpufreq_cb(int cid, unsigned long freq)
{
	unsigned long flags, flags2;
	unsigned int curr_obv = 0U;
	unsigned long long curr_cb_ts;
	int new_ts;
	int i;
	int opp;
	int use_cid = cid;
	unsigned long long loading_result = 0U;
	struct fbt_thread_loading *pos, *next;

	if (!fbt_enable)
		return;

	spin_lock_irqsave(&freq_slock, flags);

	curr_cb_ts = fpsgo_get_time();
	new_ts = nsec_to_100usec(curr_cb_ts);

	if (fbt_is_mips_different()) {
		for (opp = (NR_FREQ_CPU - 1); opp > 0; opp--) {
			if (cpu_dvfs[cid].power[opp] >= freq)
				break;
		}
		curr_obv = cpu_dvfs[cid].capacity[NR_FREQ_CPU - 1 - opp] * 100 / 1024;
	} else {
		curr_obv = (unsigned int)(freq * 100);
		do_div(curr_obv, cpu_max_freq);
	}

	if (clus_obv[cid] != curr_obv) {
		xgf_trace("ts %llu, curr_obv[%d] %d", new_ts, cid, curr_obv);
		clus_obv[cid] = curr_obv;
	}

	if ((cid != last_cid && curr_obv <= last_obv) ||
		(cid == last_cid && curr_obv == last_obv)) {
		spin_unlock_irqrestore(&freq_slock, flags);
		return;
	}

	if (curr_obv < last_obv) {
		for (i = 0; i < cluster_num; i++) {
			if (curr_obv < clus_obv[i]) {
				curr_obv = clus_obv[i];
				use_cid = i;
			}
		}
	}

	spin_lock_irqsave(&loading_slock, flags2);
	list_for_each_entry_safe(pos, next, &loading_list, entry) {
		if (atomic_read(&pos->last_cb_ts) != 0 && atomic_read(&pos->skip_loading) == 0) {
			loading_result = fbt_est_loading(new_ts, pos->last_cb_ts, last_obv);
			atomic_add_return(loading_result, &(pos->loading));
		}
		atomic_set(&pos->last_cb_ts, new_ts);
		fpsgo_systrace_c_fbt_gm(pos->pid, atomic_read(&pos->loading), "loading");
	}
	spin_unlock_irqrestore(&loading_slock, flags2);

	last_cid = use_cid;
	last_obv = curr_obv;

	fpsgo_systrace_c_fbt_gm(-100, last_cid, "last_cid");
	fpsgo_systrace_c_fbt_gm(-100, last_obv, "last_obv");

	spin_unlock_irqrestore(&freq_slock, flags);
}

void fpsgo_ctrl2fbt_vsync(void)
{
	unsigned long flags;

	if (!fbt_enable)
		return;

	spin_lock_irqsave(&xgf_slock, flags);
	vsync_time = fpsgo_get_time();
	xgf_trace("vsync_time=%llu", nsec_to_usec(vsync_time));
	spin_unlock_irqrestore(&xgf_slock, flags);
}

void fpsgo_comp2fbt_frame_start(int pid, unsigned long long q2q_time,
					unsigned long long self_time, int type, unsigned long long ts)
{
	struct fbt_thread_info *thr;
	struct fbt_boost_info *boost;
	unsigned long long slptime = 0U, runtime;
	int targettime, targetfps;
	unsigned int limited_cap;
	unsigned long flags;
	int new_ts;
	int blc_wt = 0;

	if (!fbt_enable)
		return;

	fbt_list_main_lock();
	thr = fbt_list_find_add(pid, 1);
	if (!thr) {
		FPSGO_LOGE("ERROR %d\n", __LINE__);
		fbt_list_main_unlock();
		return;
	}

	fbt_list_thread_lock(&(thr->thr_mlock));
	fbt_list_main_unlock();

	thr->frame_type = type;
	fpsgo_systrace_c_fbt_gm(pid, type, "frame_type");

	if (type == BY_PASS_TYPE) {
		fbt_list_thread_unlock(&(thr->thr_mlock));
		return;
	}

	if (type == NON_VSYNC_ALIGNED_TYPE) {
		fpsgo_fbt2xgf_query_sleep_time(pid, &slptime);
		if (q2q_time < slptime) {
			fbt_list_thread_unlock(&(thr->thr_mlock));
			return;
		}
	}

	boost = &(thr->boost_info);
	boost->q2q_time = q2q_time;
	boost->frame_info[boost->f_iter].q2q_time = q2q_time;
	thr->boosting = 1;

	runtime = self_time - slptime;

	targetfps = boost->fstb_target_fps;
	if (!targetfps)
		targetfps = TARGET_UNLIMITED_FPS;
	targettime = FBTCPU_SEC_DIVIDER / targetfps;

	fpsgo_systrace_c_fbt_gm(pid, thr->boosting, "boosting");
	fpsgo_systrace_c_fbt(pid, runtime, "running time");
	fpsgo_systrace_c_fbt(pid, boost->q2q_time, "q2q time");

	new_ts = nsec_to_100usec(ts);
	spin_lock_irqsave(&loading_slock, flags);
	atomic_set(&thr->pLoading->last_cb_ts, new_ts);
	atomic_set(&thr->pLoading->skip_loading, 0);
	spin_unlock_irqrestore(&loading_slock, flags);
	fpsgo_systrace_c_fbt_gm(thr->pid, atomic_read(&thr->pLoading->last_cb_ts), "last_cb_ts");

	blc_wt = fbt_boost_policy(runtime, targettime, slptime, targetfps, thr, ts);

	fbt_list_thread_unlock(&(thr->thr_mlock));

	limited_cap = fbt_get_max_userlimit_freq();
	fpsgo_systrace_c_fbt_gm(pid, limited_cap, "limited_cap");

	if (fpsgo_fbt2fstb_cpu_capability_fp)
		fpsgo_fbt2fstb_cpu_capability_fp(pid, type, blc_wt,
							limited_cap, boost->last_target_fps,
							q2q_time, runtime);

}

void fpsgo_comp2fbt_frame_complete(int pid, int type, int render, unsigned long long ts)
{
	struct fbt_thread_info *thr;
	struct fbt_boost_info *boost;
	unsigned long long loading_result = 0U;
	unsigned long long old_loading = 0U;
	int iter = 0;
	unsigned long flags;
	unsigned int temp_obv;
	int new_ts;
	int cancel = 0;

	if (!fbt_enable)
		return;

	spin_lock_irqsave(&freq_slock, flags);
	temp_obv = last_obv;
	spin_unlock_irqrestore(&freq_slock, flags);

	fbt_list_main_lock();
	thr = fbt_list_find_add(pid, 0);
	if (!thr) {
		fbt_list_main_unlock();
		return;
	}

	new_ts = nsec_to_100usec(ts);

	fbt_list_thread_lock(&(thr->thr_mlock));
	fbt_list_main_unlock();
	boost = &(thr->boost_info);
	thr->boosting = 0;

	if (thr->frame_type != type) {
		thr->frame_type = type;
		fpsgo_systrace_c_fbt_gm(pid, type, "frame_type");
	}

	if (render) {
		spin_lock_irqsave(&loading_slock, flags);
		loading_result = fbt_est_loading(new_ts, thr->pLoading->last_cb_ts, temp_obv);
		atomic_add_return(loading_result, &thr->pLoading->loading);
		atomic_set(&thr->pLoading->last_cb_ts, new_ts);
		atomic_set(&thr->pLoading->skip_loading, 1);
		spin_unlock_irqrestore(&loading_slock, flags);
	} else {
		if (boost->f_iter >= WINDOW)
			iter = 0;
		else if (boost->f_iter >= 2)
			iter = boost->f_iter - 2;
		else if (boost->f_iter == 1)
			iter = WINDOW - 1;
		else if (boost->f_iter == 0)
			iter = WINDOW - 2;
		old_loading = boost->frame_info[iter].mips;
		spin_lock_irqsave(&loading_slock, flags);
		atomic_set(&thr->pLoading->loading, old_loading);
		atomic_set(&thr->pLoading->last_cb_ts, new_ts);
		atomic_set(&thr->pLoading->skip_loading, 1);
		spin_unlock_irqrestore(&loading_slock, flags);
	}

	if (max_blc_pid == thr->pid)
		cancel = 1;

	fbt_list_thread_unlock(&(thr->thr_mlock));

	fpsgo_systrace_c_fbt_gm(thr->pid, atomic_read(&thr->pLoading->loading), "loading");
	fpsgo_systrace_c_fbt_gm(thr->pid, atomic_read(&thr->pLoading->last_cb_ts), "last_cb_ts");
	fpsgo_systrace_c_fbt_gm(pid, thr->boosting, "boosting");

	if (fbt_list_find_noboost()) {
		spin_lock_irqsave(&xgf_slock, flags);
		max_blc_pid = 0;
		max_blc = 0;
		memset(base_freq, 0, cluster_num * sizeof(unsigned int));
		spin_unlock_irqrestore(&xgf_slock, flags);
		update_eas_boost_value(EAS_KIR_FBC, CGROUP_TA, 0);
		fbt_free_bhr();
		if (fbt_idleprefer_enable && set_idleprefer) {
			prefer_idle_for_perf_idx(CGROUP_TA, 0);
			set_idleprefer = 0;
		}
	} else if (cancel == 1) {
		unsigned int temp_max_blc = 0;
		int temp_max_blc_pid = 0;

		fbt_list_find_next_max_blc(&temp_max_blc, &temp_max_blc_pid);

		spin_lock_irqsave(&xgf_slock, flags);
		max_blc_pid = temp_max_blc_pid;
		max_blc = temp_max_blc;
		spin_unlock_irqrestore(&xgf_slock, flags);
	}
	xgf_trace("max_blc %d, max_blc_pid %d", max_blc, max_blc_pid);
}

void fpsgo_comp2fbt_bypass_connect(int pid)
{
	unsigned long flags;
	int free_bhr = 0;

	if (!fbt_enable)
		return;

	fbt_list_bypass_add(pid);

	spin_lock_irqsave(&xgf_slock, flags);
	if (!bypass_flag) {
		bypass_flag = 1;
		free_bhr = 1;
		walt_enable = 1;
		xgf_trace("bypass_flag %d", bypass_flag);
	}
	spin_unlock_irqrestore(&xgf_slock, flags);

	if (free_bhr) {
		fbt_free_bhr();
		sched_walt_enable(LT_WALT_FPSGO, 1);
		xgf_trace("fpsgo enable walt");
	}
}

void fpsgo_comp2fbt_bypass_disconnect(int pid)
{
	unsigned long flags;
	int clear = 0;
	int disable_walt = 0;

	if (!fbt_enable)
		return;

	mutex_lock(&bypasslist_lock);
	fbt_list_bypass_del(pid);
	if (fbt_list_bypass_is_empty())
		clear = 1;
	mutex_unlock(&bypasslist_lock);

	spin_lock_irqsave(&xgf_slock, flags);
	if (!bypass_flag) {
		FPSGO_LOGE("ERROR %d\n", __LINE__);
	} else {
		if (clear) {
			bypass_flag = 0;
			if (walt_enable) {
				walt_enable = 0;
				disable_walt = 1;
			}
		}
		xgf_trace("bypass_flag %d", bypass_flag);
	}
	spin_unlock_irqrestore(&xgf_slock, flags);

	if (disable_walt) {
		sched_walt_enable(LT_WALT_FPSGO, 0);
		xgf_trace("fpsgo disable walt");
	}
}

void fpsgo_comp2fbt_enq_start(int pid, unsigned long long ts)
{
	struct fbt_thread_info *thr;
	unsigned long long loading_result = 0U;
	unsigned long flags;
	unsigned int temp_obv;
	int new_ts;

	if (!fbt_enable)
		return;

	spin_lock_irqsave(&freq_slock, flags);
	temp_obv = last_obv;
	spin_unlock_irqrestore(&freq_slock, flags);

	fbt_list_main_lock();
	thr = fbt_list_find_add(pid, 1);
	if (!thr) {
		FPSGO_LOGE("ERROR %d\n", __LINE__);
		fbt_list_main_unlock();
		return;
	}

	new_ts = nsec_to_100usec(ts);

	fbt_list_thread_lock(&(thr->thr_mlock));
	fbt_list_main_unlock();
	thr->last_qustr_time = ts;
	spin_lock_irqsave(&loading_slock, flags);
	loading_result = fbt_est_loading(new_ts, thr->pLoading->last_cb_ts, temp_obv);
	atomic_add_return(loading_result, &thr->pLoading->loading);
	atomic_set(&thr->pLoading->last_cb_ts, new_ts);
	atomic_set(&thr->pLoading->skip_loading, 1);
	spin_unlock_irqrestore(&loading_slock, flags);
	fbt_list_thread_unlock(&(thr->thr_mlock));

	fpsgo_systrace_c_fbt_gm(pid, atomic_read(&thr->pLoading->loading), "loading");
	fpsgo_systrace_c_fbt_gm(pid, atomic_read(&thr->pLoading->last_cb_ts), "last_cb_ts");
}

void fpsgo_comp2fbt_enq_end(int pid, unsigned long long ts)
{
	struct fbt_thread_info *thr;
	unsigned long flags;
	int new_ts;

	if (!fbt_enable)
		return;

	fbt_list_main_lock();
	thr = fbt_list_find_add(pid, 0);
	if (!thr) {
		fbt_list_main_unlock();
		return;
	}

	new_ts = nsec_to_100usec(ts);

	fbt_list_thread_lock(&(thr->thr_mlock));
	fbt_list_main_unlock();
	spin_lock_irqsave(&loading_slock, flags);
	atomic_set(&thr->pLoading->last_cb_ts, new_ts);
	atomic_set(&thr->pLoading->skip_loading, 0);
	spin_unlock_irqrestore(&loading_slock, flags);
	fbt_list_thread_unlock(&(thr->thr_mlock));

	fpsgo_systrace_c_fbt_gm(pid, atomic_read(&thr->pLoading->last_cb_ts), "last_cb_ts");
}

void fpsgo_comp2fbt_deq_start(int pid, unsigned long long ts)
{
	struct fbt_thread_info *thr;
	unsigned long long loading_result = 0U;
	unsigned long flags;
	unsigned int temp_obv;
	int new_ts;

	if (!fbt_enable)
		return;

	spin_lock_irqsave(&freq_slock, flags);
	temp_obv = last_obv;
	spin_unlock_irqrestore(&freq_slock, flags);

	fbt_list_main_lock();
	thr = fbt_list_find_add(pid, 0);
	if (!thr) {
		fbt_list_main_unlock();
		return;
	}

	new_ts = nsec_to_100usec(ts);

	fbt_list_thread_lock(&(thr->thr_mlock));
	fbt_list_main_unlock();
	spin_lock_irqsave(&loading_slock, flags);
	loading_result = fbt_est_loading(new_ts, thr->pLoading->last_cb_ts, temp_obv);
	atomic_add_return(loading_result, &thr->pLoading->loading);
	atomic_set(&thr->pLoading->last_cb_ts, new_ts);
	atomic_set(&thr->pLoading->skip_loading, 1);
	spin_unlock_irqrestore(&loading_slock, flags);
	fbt_list_thread_unlock(&(thr->thr_mlock));

	fpsgo_systrace_c_fbt_gm(pid, atomic_read(&thr->pLoading->loading), "loading");
	fpsgo_systrace_c_fbt_gm(pid, atomic_read(&thr->pLoading->last_cb_ts), "last_cb_ts");
}

void fpsgo_comp2fbt_deq_end(int pid, unsigned long long ts, unsigned long long deq_time)
{
	struct fbt_thread_info *thr;
	struct fbt_boost_info *boost;
	unsigned long flags;
	int new_ts;

	if (!fbt_enable)
		return;

	fbt_list_main_lock();
	thr = fbt_list_find_add(pid, 0);
	if (!thr) {
		fbt_list_main_unlock();
		return;
	}

	new_ts = nsec_to_100usec(ts);

	fbt_list_thread_lock(&(thr->thr_mlock));
	fbt_list_main_unlock();
	spin_lock_irqsave(&loading_slock, flags);
	atomic_set(&thr->pLoading->last_cb_ts, new_ts);
	atomic_set(&thr->pLoading->skip_loading, 0);
	spin_unlock_irqrestore(&loading_slock, flags);
	boost = &(thr->boost_info);
	boost->deqend_time = ts;
	boost->deq_time = deq_time;
	fbt_list_thread_unlock(&(thr->thr_mlock));

	fpsgo_systrace_c_fbt_gm(pid, atomic_read(&thr->pLoading->last_cb_ts), "last_cb_ts");
	fpsgo_systrace_c_fbt_gm(pid, boost->deq_time, "deq_time");
}

void fpsgo_fstb2fbt_target_fps(int pid, int target_fps, int queue_fps)
{
	struct fbt_thread_info *thr;
	struct fbt_boost_info *boost;

	if (!fbt_enable)
		return;

	fbt_list_main_lock();
	thr = fbt_list_find_add(pid, 0);
	if (thr) {
		boost = &thr->boost_info;

		fbt_list_thread_lock(&(thr->thr_mlock));
		fbt_list_main_unlock();

		if (boost->fstb_target_fps != target_fps) {
			boost->middle_enable = 0;
			fpsgo_systrace_c_fbt(pid, boost->middle_enable, "middle_enable");
		}

		boost->fstb_target_fps = target_fps;
		fpsgo_systrace_c_fbt(pid, target_fps, "target_fps");

		fbt_list_thread_unlock(&(thr->thr_mlock));
	} else
		fbt_list_main_unlock();
}

void fpsgo_ctrl2fbt_dfrc_fps(int fps_limit)
{
	unsigned long flags;

	if (!fps_limit)
		return;

	spin_lock_irqsave(&xgf_slock, flags);

	_gdfrc_fps_limit = fps_limit;
	_gdfrc_cpu_target = FBTCPU_SEC_DIVIDER / fps_limit;

	spin_unlock_irqrestore(&xgf_slock, flags);

	xgf_trace("_gdfrc_fps_limit %d", _gdfrc_fps_limit);
	xgf_trace("_gdfrc_cpu_target %d", _gdfrc_cpu_target);
}

static void fbt_update_pwd_tbl(void)
{
	int cluster, opp;
	struct cpumask cluster_cpus;
	int cpu;
	const struct sched_group_energy *core_energy;
	unsigned long long max_cap = 0ULL;

	for (cluster = 0; cluster < cluster_num ; cluster++) {
		for (opp = 0; opp < NR_FREQ_CPU; opp++) {
			cpu_dvfs[cluster].power[opp] = mt_cpufreq_get_freq_by_idx(cluster, opp);

			if (fbt_is_mips_different()) {
				arch_get_cluster_cpus(&cluster_cpus, cluster);

				for_each_cpu(cpu, &cluster_cpus) {
					core_energy = cpu_core_energy(cpu);
					cpu_dvfs[cluster].capacity[opp] = core_energy->cap_states[opp].cap;
				}
			}
		}
	}

	if (fbt_is_mips_different()) {
		for (cluster = 0; cluster < cluster_num ; cluster++) {
			if (cpu_dvfs[cluster].capacity[0] > max_cap) {
				max_cap = cpu_dvfs[cluster].capacity[0];
				max_cap_cluster = cluster;
			}
		}
	}

	for (cluster = 0; cluster < cluster_num ; cluster++) {
		if (fbt_is_mips_different() && cpu_dvfs[cluster].capacity[0])
			cpu_capacity_ratio[cluster] =
				div64_u64((max_cap << 7), cpu_dvfs[cluster].capacity[0]);
		else
			cpu_capacity_ratio[cluster] = 128;
	}

	cpu_max_freq = cpu_dvfs[cluster_num - 1].power[0];
	if (!cpu_max_freq) {
		FPSGO_LOGE("NULL power table\n");
		cpu_max_freq = 1;
	}
}

static void fbt_setting_exit(void)
{
	unsigned long flags;
	int disable_walt = 0;

	spin_lock_irqsave(&xgf_slock, flags);
	last_obv = 0;
	bypass_flag = 0;
	vag_fps = 0;
	vsync_time = 0;
	_gdfrc_fps_limit    = TARGET_UNLIMITED_FPS;
	_gdfrc_cpu_target = GED_VSYNC_MISS_QUANTUM_NS;
	memset(base_freq, 0, cluster_num * sizeof(unsigned int));
	max_blc = 0;
	max_blc_pid = 0;
	if (walt_enable) {
		walt_enable = 0;
		disable_walt = 1;
	}
	spin_unlock_irqrestore(&xgf_slock, flags);

	fbt_list_clear();

	update_eas_boost_value(EAS_KIR_FBC, CGROUP_TA, 0);
	if (fbt_idleprefer_enable && set_idleprefer) {
		prefer_idle_for_perf_idx(CGROUP_TA, 0);
		set_idleprefer = 0;
	}

	fbt_free_bhr();

	if (disable_walt) {
		xgf_trace("fpsgo disable walt");
		sched_walt_enable(LT_WALT_FPSGO, 0);
	}
}

int fpsgo_ctrl2fbt_switch_fbt(int enable)
{
	mutex_lock(&fbt_mlock);

	if (fbt_enable == enable) {
		mutex_unlock(&fbt_mlock);
		return 0;
	}

	fbt_enable = enable;
	fpsgo_systrace_c_fbt_gm(-100, fbt_enable, "fbt_enable");

	ppm_game_mode_change_cb(enable);
	if (!enable)
		fbt_setting_exit();
	else {
		if (fbt_idleprefer_enable) {
			prefer_idle_for_perf_idx(CGROUP_TA, 1);
			set_idleprefer = 1;
		}
	}

	mutex_unlock(&fbt_mlock);
	return 0;
}

int fbt_switch_idleprefer(int enable)
{
	unsigned long flags;
	int last_enable;
	int do_set = 0;

	mutex_lock(&fbt_mlock);
	if (!fbt_enable) {
		mutex_unlock(&fbt_mlock);
		return 0;
	}
	mutex_unlock(&fbt_mlock);

	spin_lock_irqsave(&xgf_slock, flags);

	last_enable = fbt_idleprefer_enable;
	fbt_idleprefer_enable = enable;

	if (last_enable && !fbt_idleprefer_enable) {
		do_set = 1;
		set_idleprefer = 0;
	} else if (!last_enable && fbt_idleprefer_enable) {
		do_set = 1;
		set_idleprefer = 1;
	}

	spin_unlock_irqrestore(&xgf_slock, flags);

	if (do_set)
		prefer_idle_for_perf_idx(CGROUP_TA, enable);

	return 0;
}

int fbt_switch_ceiling(int enable)
{
	unsigned long flags;
	int last_enable;
	int do_free_bhr = 0;

	mutex_lock(&fbt_mlock);
	if (!fbt_enable) {
		mutex_unlock(&fbt_mlock);
		return 0;
	}
	mutex_unlock(&fbt_mlock);

	spin_lock_irqsave(&xgf_slock, flags);

	last_enable = suppress_ceiling;
	suppress_ceiling = enable;

	if (last_enable && !suppress_ceiling)
		do_free_bhr = 1;

	spin_unlock_irqrestore(&xgf_slock, flags);

	if (do_free_bhr)
		fbt_free_bhr();

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

	fpsgo_ctrl2fbt_switch_fbt(val);

	return cnt;
}

FBT_DEBUGFS_ENTRY(switch_fbt);

static int fbt_switch_idleprefer_show(struct seq_file *m, void *unused)
{
	SEQ_printf(m, "fbt_idleprefer_enable:%d\n", fbt_idleprefer_enable);
	return 0;
}

static ssize_t fbt_switch_idleprefer_write(struct file *flip,
			const char *ubuf, size_t cnt, loff_t *data)
{
	int val;
	int ret;

	ret = kstrtoint_from_user(ubuf, cnt, 0, &val);
	if (ret)
		return ret;

	fbt_switch_idleprefer(val);

	return cnt;
}

FBT_DEBUGFS_ENTRY(switch_idleprefer);

static int fbt_switch_ceiling_show(struct seq_file *m, void *unused)
{
	SEQ_printf(m, "suppress_ceiling:%d\n", suppress_ceiling);
	return 0;
}

static ssize_t fbt_switch_ceiling_write(struct file *flip,
		const char *ubuf, size_t cnt, loff_t *data)
{
	int val;
	int ret;

	ret = kstrtoint_from_user(ubuf, cnt, 0, &val);
	if (ret)
		return ret;

	fbt_switch_ceiling(val);

	return cnt;
}

FBT_DEBUGFS_ENTRY(switch_ceiling);

static int fbt_thread_info_show(struct seq_file *m, void *unused)
{
	struct fbt_thread_info *pos, *next;
	struct fbt_thread_bypass *pos2, *next2;

	SEQ_printf(m, "enable\tbypass\twalt\tmax_blc\tmax_pid\tdfrc_fps\tvsync\n");
	SEQ_printf(m, "%d\t%d\t%d\t%d\t%d\t%d\t\t%llu\n\n",
		fbt_enable, bypass_flag, walt_enable, max_blc, max_blc_pid, _gdfrc_fps_limit, vsync_time);

	mutex_lock(&fbt_mlock);
	if (!fbt_enable) {
		mutex_unlock(&fbt_mlock);
		return 0;
	}
	mutex_unlock(&fbt_mlock);

	SEQ_printf(m, "pid\ttype\tboost\ttfps\t");
	SEQ_printf(m, "q2q_t\t\tmiddle\tblc\tvar\tloading\t\n");

	fbt_list_main_lock();

	list_for_each_entry_safe(pos, next, &active_list, entry) {
		SEQ_printf(m, "%d\t", pos->pid);
		SEQ_printf(m, "%d\t", pos->frame_type);
		SEQ_printf(m, "%d\t", pos->boosting);
		SEQ_printf(m, "%d\t", pos->boost_info.fstb_target_fps);
		SEQ_printf(m, "%09llu\t", pos->boost_info.q2q_time);
		SEQ_printf(m, "%d\t", pos->boost_info.middle_enable);
		SEQ_printf(m, "%02d\t", pos->boost_info.last_blc);
		SEQ_printf(m, "%02llu\t", pos->boost_info.floor);
		SEQ_printf(m, "%010d\n", atomic_read(&pos->pLoading->loading));
	}

	fbt_list_main_unlock();

	SEQ_printf(m, "\nbypass list\n");
	mutex_lock(&bypasslist_lock);
	list_for_each_entry_safe(pos2, next2, &bypass_list, entry)
		SEQ_printf(m, "%03d\n", pos2->pid);
	mutex_unlock(&bypasslist_lock);


	return 0;
}

static ssize_t fbt_thread_info_write(struct file *flip,
			const char *ubuf, size_t cnt, loff_t *data)
{
	return 0;
}

FBT_DEBUGFS_ENTRY(thread_info);

void fbt_cpu_exit(void)
{
	kfree(base_freq);
	kfree(clus_obv);
	kfree(cpu_dvfs);
	kfree(clus_max_freq);
	kfree(cpu_capacity_ratio);

	kfree(lpp_clus_max_freq);

	fbt_list_clear();
}

int fbt_cpu_init(void)
{
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
	deqtime_bound = TIME_3MS;
	variance = 40;
	floor_bound = 3;
	kmin = 1;
	floor_opp = 2;
	_gdfrc_fps_limit = TARGET_UNLIMITED_FPS;
	_gdfrc_cpu_target = GED_VSYNC_MISS_QUANTUM_NS;
	fbt_idleprefer_enable = 1;
	suppress_ceiling = 1;

	cluster_num = arch_get_nr_clusters();
	max_freq_cluster = min((cluster_num - 1), 0);
	max_cap_cluster = min((cluster_num - 1), 0);

	base_freq =
		kzalloc(cluster_num * sizeof(unsigned int), GFP_KERNEL);

	clus_obv =
		kzalloc(cluster_num * sizeof(unsigned int), GFP_KERNEL);

	cpu_dvfs =
		kzalloc(cluster_num * sizeof(struct fbt_cpu_dvfs_info), GFP_KERNEL);

	clus_max_freq =
		kzalloc(cluster_num * sizeof(int), GFP_KERNEL);

	cpu_capacity_ratio =
		kzalloc(cluster_num * sizeof(int), GFP_KERNEL);

	fbt_update_pwd_tbl();
	fbt_init_cpuset_freq_bound_table();

	if (fpsgo_debugfs_dir) {
		fbt_debugfs_dir = debugfs_create_dir("fbt", fpsgo_debugfs_dir);
		if (fbt_debugfs_dir) {
			debugfs_create_file("thread_info",
					S_IRUGO | S_IWUSR | S_IWGRP,
					fbt_debugfs_dir,
					NULL,
					&fbt_thread_info_fops);
			debugfs_create_file("switch_fbt",
					S_IRUGO | S_IWUSR | S_IWGRP,
					fbt_debugfs_dir,
					NULL,
					&fbt_switch_fbt_fops);
			debugfs_create_file("switch_idleprefer",
					S_IRUGO | S_IWUSR | S_IWGRP,
					fbt_debugfs_dir,
					NULL,
					&fbt_switch_idleprefer_fops);
			debugfs_create_file("suppress_ceiling",
					S_IRUGO | S_IWUSR | S_IWGRP,
					fbt_debugfs_dir,
					NULL,
					&fbt_switch_ceiling_fops);
		}
	}

	INIT_LIST_HEAD(&active_list);
	INIT_LIST_HEAD(&loading_list);
	INIT_LIST_HEAD(&bypass_list);

	update_lppcap = fbt_cpu_lppcap_update;

	lpp_max_cap = 100;
	lpp_clus_max_freq =
		kzalloc(cluster_num * sizeof(int), GFP_KERNEL);
	for (cluster = 0 ; cluster < cluster_num; cluster++)
		lpp_clus_max_freq[cluster] = cpu_dvfs[cluster].power[0];

	return 0;
}
