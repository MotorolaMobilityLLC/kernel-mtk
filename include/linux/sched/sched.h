/*
 * Copyright (C) 2017 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */
#ifdef CONFIG_MTK_UNIFY_POWER
#include "../../drivers/misc/mediatek/base/power/include/mtk_upower.h"
#endif

#ifdef CONFIG_MTK_SCHED_TRACE
#ifdef CONFIG_MTK_SCHED_DEBUG
#define mt_sched_printf(event, x...) \
do { \
	char strings[128] = "";  \
	snprintf(strings, sizeof(strings)-1, x); \
	pr_debug(x); \
	trace_##event(strings); \
} while (0)
#else
#define mt_sched_printf(event, x...) \
do { \
	char strings[128] = "";  \
	snprintf(strings, sizeof(strings)-1, x); \
	trace_##event(strings); \
} while (0)

#endif
#else
#define mt_sched_printf(event, x...) do {} while (0)
#endif

struct energy_env {
	struct sched_group	*sg_top;
	struct sched_group	*sg_cap;
	int			cap_idx;
	int			util_delta;
	int			src_cpu;
	int			dst_cpu;
	int			energy;
	int			payoff;
	struct task_struct	*task;
	struct {
		int before;
		int after;
		int delta;
		int diff;
	} nrg;
	struct {
		int before;
		int after;
		int delta;
	} cap;
#ifdef CONFIG_MTK_SCHED_EAS_POWER_SUPPORT
	int	opp_idx[3];	/* [FIXME] cluster may > 3 */
#endif
};

/* cpu_core_energy & cpu_cluster_energy both implmented in topology.c */
extern
const struct sched_group_energy * const cpu_core_energy(int cpu);

extern
const struct sched_group_energy * const cpu_cluster_energy(int cpu);

#ifdef CONFIG_MTK_SCHED_EAS_POWER_SUPPORT
extern inline
int mtk_idle_power(int idle_state, int cpu, void *argu, int sd_level);

extern inline
int mtk_busy_power(int cpu, void *argu, int sd_level);

extern int mtk_cluster_capacity_idx(int cid, struct energy_env *eenv);
#endif


