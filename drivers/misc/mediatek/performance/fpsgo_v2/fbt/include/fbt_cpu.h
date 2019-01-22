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

#define MAX_FREQ_BOUND_NUM 2

#ifdef CONFIG_MTK_FPSGO_FBT_GAME
void fpsgo_ctrl2fbt_dfrc_fps(int fps_limit);
void fpsgo_ctrl2fbt_cpufreq_cb(int cid, unsigned long freq);
void fpsgo_ctrl2fbt_vsync(void);
void fpsgo_comp2fbt_frame_start(int pid, unsigned long long q2q_time,
							unsigned long long self_time, int type, unsigned long long ts);
void fpsgo_comp2fbt_frame_complete(int pid, int type, int render, unsigned long long ts);
void fpsgo_comp2fbt_bypass_connect(int pid);
void fpsgo_comp2fbt_bypass_disconnect(int pid);
void fpsgo_comp2fbt_enq_start(int pid, unsigned long long ts);
void fpsgo_comp2fbt_enq_end(int pid, unsigned long long ts);
void fpsgo_comp2fbt_deq_start(int pid, unsigned long long ts);
void fpsgo_comp2fbt_deq_end(int pid, unsigned long long ts, unsigned long long deq_time);
void fpsgo_fstb2fbt_target_fps(int pid, int target_fps, int queue_fps);

int fbt_cpu_init(void);
void fbt_cpu_exit(void);

int fpsgo_ctrl2fbt_switch_fbt(int enable);

void fbt_check_thread_status(void);

#else
static inline void fpsgo_ctrl2fbt_dfrc_fps(int fps_limit) { }
static inline void fpsgo_ctrl2fbt_cpufreq_cb(int cid, unsigned long freq) { }
static inline void fpsgo_ctrl2fbt_vsync(void) { }
static inline void fpsgo_comp2fbt_frame_start(int pid, unsigned long long q2q_time,
						unsigned long long self_time, int type, unsigned long long ts) { }
static inline void fpsgo_comp2fbt_frame_complete(int pid, int type, int render, unsigned long long ts) { }
static inline void fpsgo_comp2fbt_bypass_connect(int pid) { }
static inline void fpsgo_comp2fbt_bypass_disconnect(int pid) { }
static inline void fpsgo_comp2fbt_enq_start(int pid, unsigned long long ts) { }
static inline void fpsgo_comp2fbt_enq_end(int pid, unsigned long long ts) { }
static inline void fpsgo_comp2fbt_deq_start(int pid, unsigned long long ts) { }
static inline void fpsgo_comp2fbt_deq_end(int pid, unsigned long long ts, unsigned long long deq_time) { }
static inline void fpsgo_fstb2fbt_target_fps(int pid, int target_fps, int queue_fps) { }

static inline int fbt_cpu_init(void) { }
static inline void fbt_cpu_exit(void) { }

static inline int fpsgo_ctrl2fbt_switch_fbt(int enable) { return 0; }

static inline void fbt_check_thread_status(void) { }

#endif

#endif
