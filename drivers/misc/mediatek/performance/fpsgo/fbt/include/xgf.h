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

#ifndef _XGF_H_
#define _XGF_H_

#include <linux/rbtree.h>

struct xgf_tick {
	unsigned long long ts; /* timestamp */
	unsigned long long runtime;
};

struct xgf_intvl {
	struct xgf_tick *start, *end;
};

struct xgf_sect {
	struct hlist_node hlist;
	struct xgf_intvl un;
};

struct xgf_proc {
	struct hlist_node hlist;
	pid_t parent;
	pid_t render;
	struct hlist_head timer_head;
	struct rb_root timer_rec;

	struct xgf_tick queue;
	struct xgf_tick deque;

	unsigned long long slptime;
	unsigned long long slptime_ged;
	unsigned long long quetime;
	unsigned long long deqtime;
};

struct xgf_timer {
	struct hlist_node hlist;
	struct rb_node rb_node;
	const void *hrtimer;
	struct xgf_tick fire;
	struct xgf_tick expire;
	union {
		int expired;
		int blacked;
	};
};

extern int (*xgf_est_slptime_fp)(struct xgf_proc *proc,
		unsigned long long *slptime, struct xgf_tick *ref,
		struct xgf_tick *now, pid_t r_pid);
extern void xgf_dequeuebuffer(unsigned long arg);
extern void xgf_game_mode_exit(int val);
extern void xgf_get_deqend_time(void);

void xgf_lockprove(const char *tag);
int xgf_est_slptime(struct xgf_proc *proc, unsigned long long *slptime,
		    struct xgf_tick *ref, struct xgf_tick *now, pid_t r_pid);
void xgf_trace(const char *fmt, ...);
void xgf_reset_render(struct xgf_proc *proc);

void *xgf_kzalloc(size_t size);
void xgf_kfree(const void *block);

int xgf_query_blank_time(pid_t, unsigned long long*,
			 unsigned long long*);
int xgf_self_ctrl_enable(int, int);

#endif
