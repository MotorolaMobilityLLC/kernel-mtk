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
#ifndef __MTK_IDLE_PROFILE_H__
#define __MTK_IDLE_PROFILE_H__

#include <linux/ktime.h>
#include "mtk_spm_internal.h"

#define SPM_MET_TAGGING  0
#define IDLE_LOG_BUF_LEN 1024

#define APXGPT_SYS_TICKS_PER_US ((u32)(13))
#define APXGPT_RTC_TICKS_PER_MS ((u32)(32))

typedef struct mtk_idle_twam {
	u32 event;
	u32 sel;
	bool running;
	bool speed_mode;
} idle_twam_t, *p_idle_twam_t;

struct mtk_idle_buf {
	char buf[IDLE_LOG_BUF_LEN];
	char *p_idx;
};

struct mtk_idle_recent_ratio {
	unsigned long long value;
	unsigned long long value_dp;
	unsigned long long value_so3;
	unsigned long long value_so;
	unsigned long long last_end_ts;
	unsigned long long start_ts;
	unsigned long long end_ts;
};

#define reset_idle_buf(idle) ((idle).p_idx = (idle).buf)
#define get_idle_buf(idle)   ((idle).buf)
#define idle_buf_append(idle, fmt, args...) \
	((idle).p_idx += snprintf((idle).p_idx, IDLE_LOG_BUF_LEN - strlen((idle).buf), fmt, ##args))

void mtk_idle_twam_callback(struct twam_sig *ts);
void mtk_idle_twam_disable(void);
void mtk_idle_twam_enable(u32 event);
p_idle_twam_t mtk_idle_get_twam(void);

bool mtk_idle_get_ratio_status(void);
void mtk_idle_ratio_calc_start(int type, int cpu);
void mtk_idle_ratio_calc_stop(int type, int cpu);
void mtk_idle_disable_ratio_calc(void);
void mtk_idle_enable_ratio_calc(void);

void mtk_idle_dump_cnt_in_interval(void);

bool mtk_idle_select_state(int type, int reason);
void mtk_idle_block_setting(int type, unsigned long *cnt, unsigned long *block_cnt, unsigned int *block_mask);
void mtk_idle_twam_init(void);

static inline long int idle_get_current_time_ms(void)
{
	struct timeval t;

	do_gettimeofday(&t);
	return ((t.tv_sec & 0xFFF) * 1000000 + t.tv_usec) / 1000;
}

void mtk_idle_recent_ratio_get(int *window_length_ms, struct mtk_idle_recent_ratio *ratio);

#endif /* __MTK_IDLE_PROFILE_H__ */

