/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */
#ifndef _SCHED_COMMON_H
#define _SCHED_COMMON_H

#include "sched_mtk.h"

#define MTK_VENDOR_DATA_SIZE_TEST(mstruct, kstruct)		\
	BUILD_BUG_ON(sizeof(mstruct) > (sizeof(u64) *		\
		ARRAY_SIZE(((kstruct *)0)->android_vendor_data1)))
#define MTK_TASK_FLAG 9
#define MTK_TG_FLAG 1

#if IS_ENABLED(CONFIG_NONLINEAR_FREQ_CTL)
extern void mtk_map_util_freq(void *data, unsigned long util, unsigned long freq,
			struct cpumask *cpumask, unsigned long *next_freq);
#else
#define mtk_map_util_freq(data, util, freq, cap, next_freq)
#endif /* CONFIG_NONLINEAR_FREQ_CTL */

#if IS_ENABLED(CONFIG_MTK_CPUFREQ_SUGOV_EXT)
unsigned long mtk_cpu_util(int cpu, unsigned long util_cfs,
				unsigned long max, enum schedutil_type type,
				struct task_struct *p);
#endif
__always_inline
unsigned long mtk_uclamp_rq_util_with(struct rq *rq, unsigned long util,
				  struct task_struct *p);

#define EAS_NODE_NAME "eas_info"
#define EAS_PROP_CSRAM "csram-base"
#define EAS_PROP_OFFS_CAP "offs-cap"
#define EAS_PROP_OFFS_THERMAL_S "offs-thermal-limit"

struct eas_info {
	unsigned int csram_base;
	unsigned int offs_cap;
	unsigned int offs_thermal_limit_s;
	bool available;
};

struct vip_task_struct {
	struct list_head		vip_list;
	u64				sum_exec_snapshot;
	u64				total_exec;
	int				vip_prio;
	bool				basic_vip;
	u64				throttle_time;
	bool				vvip;
};

struct mtk_task {
	u64 reserved0[MTK_TASK_FLAG];
	struct vip_task_struct	vip_task;
};

struct vip_task_group {
	unsigned int threshold;
};

struct mtk_tg {
	u64 reserved0[MTK_TG_FLAG];
	struct vip_task_group vtg;
};

void parse_eas_data(struct eas_info *info);

#endif /* _SCHED_COMMON_H */
