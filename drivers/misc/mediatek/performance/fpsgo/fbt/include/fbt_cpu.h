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

#ifndef __FBT_CPU_H__
#define __FBT_CPU_H__
#include <mach/mtk_ppm_api.h>
#include <linux/hrtimer.h>

#define MAX_FREQ_BOUND_NUM 2

#ifdef NR_FREQ_CPU
struct fbt_cpu_dvfs_info {
	unsigned int power[NR_FREQ_CPU];
};
#endif

struct fbt_jerk {
	int id;
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
	unsigned long long frame_time;
	unsigned long long q2q_time;
	int count;
};

#define SEQ_printf(m, x...)\
do {\
	if (m)\
		seq_printf(m, x);\
	else\
		pr_debug(x);\
} while (0)

void fbt_cpu_exit(void);
int fbt_cpu_init(void);

extern unsigned long int min_boost_freq[3];
extern int fbt_reset_asfc(int level);
extern void fstb_game_mode_change(int is_game);

extern int set_cpuset(int cluster);
extern int prefer_idle_for_perf_idx(int idx, int prefer_idle);
extern unsigned int mt_cpufreq_get_freq_by_idx(int id, int idx);
extern unsigned int mt_ppm_userlimit_freq_limit_by_others(
		unsigned int cluster);
extern void fbc_notify_game(int game);
extern void fstb_queue_time_update(unsigned long long ts);

void fbt_cpu_set_game_hint_cb(int is_game_mode);

#ifdef CONFIG_MTK_FPSGO_FBT_GAME
int switch_fbt_game(int);
int switch_fbt_cpuset(int);
#else
static inline int switch_fbt_game(int en) { return 0; }
static inline int switch_fbt_cpuset(int en) { return 0; }
#endif

#endif
