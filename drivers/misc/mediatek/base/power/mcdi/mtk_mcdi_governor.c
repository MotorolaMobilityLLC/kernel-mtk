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

#include <linux/delay.h>
#include <linux/pm_qos.h>
#include <linux/spinlock.h>
#include <linux/timekeeping.h>

#include <mtk_idle_mcdi.h>

#include <mtk_mcdi.h>
#include <mtk_mcdi_governor.h>
#include <mtk_mcdi_mbox.h>
#include <mtk_mcdi_state.h>

#include <trace/events/mtk_events.h>

#define BOOT_TIME_LIMIT             10      /* sec */

#define CHECK_CLUSTER_RESIDENCY     0

#define ANY_CORE_DPIDLE_SODI

int last_core_token = -1;
static int boot_time_check;

struct mcdi_status {
	bool valid;
	int state;
	unsigned long long enter_time_us;
	unsigned long long predict_us;
};

struct mcdi_gov {
	int num_mcusys;
	int num_cluster[NF_CLUSTER];
	unsigned int avail_cpu_mask;
	unsigned int avail_cluster_mask;
	int avail_cnt_mcusys;
	int avail_cnt_cluster[NF_CLUSTER];
	struct mcdi_status status[NF_CPU];
};

static unsigned int all_cpu_in_cluster_pwr_off_mask[NF_CLUSTER] = {
	0x000F,     /* Cluster 0 */
	0x00F0      /* Cluster 1 */
};

static unsigned int cpu_cluster_pwr_stat_table[NF_CPU] = {
	0x200FE,     /* Only CPU 0 */
	0x200FD,     /* Only CPU 1 */
	0x200FB,
	0x200F7,
	0x100EF,
	0x100DF,
	0x100BF,
	0x1007F      /* Only CPU 7 */
};

static unsigned long any_core_cpu_cond_info[NF_ANY_CORE_CPU_COND_INFO];
static DEFINE_SPINLOCK(any_core_cpu_cond_spin_lock);

static struct mcdi_feature_status mcdi_feature_stat;
static DEFINE_SPINLOCK(mcdi_feature_stat_spin_lock);

static struct mcdi_gov mcdi_gov_data;

static DEFINE_SPINLOCK(mcdi_gov_spin_lock);

static int mcdi_residency_latency[NF_CPU];

void any_core_cpu_cond_inc(int idx)
{
	unsigned long flags;

	if (!(idx >= 0 && idx < NF_ANY_CORE_CPU_COND_INFO))
		return;

	spin_lock_irqsave(&any_core_cpu_cond_spin_lock, flags);

	any_core_cpu_cond_info[idx]++;

	spin_unlock_irqrestore(&any_core_cpu_cond_spin_lock, flags);
}

void any_core_cpu_cond_get(unsigned long buf[NF_ANY_CORE_CPU_COND_INFO])
{
	unsigned long flags;
	int i;

	spin_lock_irqsave(&any_core_cpu_cond_spin_lock, flags);

	for (i = 0; i < NF_ANY_CORE_CPU_COND_INFO; i++)
		buf[i] = any_core_cpu_cond_info[i];

	spin_unlock_irqrestore(&any_core_cpu_cond_spin_lock, flags);
}

static inline unsigned long long idle_get_current_time_us(void)
{
	struct timeval t;

	do_gettimeofday(&t);
	return ((t.tv_sec & 0xFFF) * 1000000 + t.tv_usec);
}

static bool is_anycore_dpidle_sodi_state(int state)
{
	return (state == MCDI_STATE_DPIDLE || state == MCDI_STATE_SODI3 || state == MCDI_STATE_SODI);
}

void update_residency_latency_result(int cpu)
{
	struct cpuidle_driver *tbl = mcdi_state_tbl_get();
	int i;
	unsigned int predict_us = get_menu_predict_us();
	int latency_req = pm_qos_request(PM_QOS_CPU_DMA_LATENCY);
	int state = 0;
	bool cluster_off = false;
	unsigned long flags;

	/*
	 * if cluster do NOT power OFF, keep residency result = CPU_OFF,
	 * to let ATF do NOT flush L2$ and notice MCDI controllert to power OFF clsuter
	 */
	spin_lock_irqsave(&mcdi_feature_stat_spin_lock, flags);
	cluster_off = mcdi_feature_stat.cluster_off;
	spin_unlock_irqrestore(&mcdi_feature_stat_spin_lock, flags);

	if (!cluster_off) {
		mcdi_residency_latency[cpu] = MCDI_STATE_CPU_OFF;
		return;
	}

	for (i = 0; i < tbl->state_count; i++) {
		struct cpuidle_state *s = &tbl->states[i];

		if (s->target_residency > predict_us)
			continue;
		if (s->exit_latency > latency_req)
			continue;

		state = i;
	}

	mcdi_residency_latency[cpu] = state;
}

int get_residency_latency_result(int cpu)
{
	return mcdi_residency_latency[cpu];
}

#if CHECK_CLUSTER_RESIDENCY
bool cluster_residency_check(int cpu)
{
	struct cpuidle_driver *mcdi_state_tbl = mcdi_state_tbl_get();
	int cluster_idx = cluster_idx_get(cpu);
	int cpu_start = cluster_idx * 4;
	int cpu_end   = cluster_idx * 4 + (4 - 1);
	int i;
	bool cluster_can_pwr_off = true;
	unsigned long long curr_time_us = idle_get_current_time_us();
	struct mcdi_status *mcdi_stat = NULL;
	unsigned int target_residency = mcdi_state_tbl->states[MCDI_STATE_CLUSTER_OFF].target_residency;
	unsigned int this_cpu_predict_us = get_menu_predict_us();

	for (i = cpu_start; i <= cpu_end; i++) {
		if ((mcdi_gov_data.avail_cpu_mask & (1 << i)) && (cpu != i)) {

			mcdi_stat = &mcdi_gov_data.status[i];

			/* Check predict_us only when C state period < predict_us */
			if ((curr_time_us - mcdi_stat->enter_time_us) > mcdi_stat->predict_us) {
				if ((mcdi_stat->enter_time_us + mcdi_stat->predict_us)
						< (curr_time_us + target_residency)) {

					cluster_can_pwr_off = false;
					break;
				}
			}
		}
	}

	if (!(this_cpu_predict_us > target_residency))
		cluster_can_pwr_off = false;

#if 0
	if (!cluster_can_pwr_off) {
		pr_err("this cpu = %d, predict_us = %u, curr_time_us = %llu, residency = %u\n",
					cpu,
					this_cpu_predict_us,
					curr_time_us,
					target_residency
		);

		for (i = cpu_start; i <= cpu_end; i++) {
			if ((mcdi_gov_data.avail_cpu_mask & (1 << i)) && (cpu != i)) {

				mcdi_stat = &mcdi_gov_data.status[i];

				pr_err("\tcpu = %d, state = %d, enter_time_us = %llu, predict_us = %llu, remain_predict_us = %llu\n",
					i,
					mcdi_stat->state,
					mcdi_stat->enter_time_us,
					mcdi_stat->predict_us,
					mcdi_stat->predict_us - (curr_time_us - mcdi_stat->enter_time_us)
				);
			}
		}
	}
#endif

	return cluster_can_pwr_off;
}
#else
bool cluster_residency_check(int cpu)
{
	return true;
}
#endif

bool mcdi_cpu_cluster_on_off_stat_check(int cpu)
{
#if 0
	int cluster_idx           = cluster_idx_get(cpu);
	unsigned int cluster_val  = (CLUSTER_PWR_STAT_MASK & ~(1 << (cluster_idx + 16)));
	unsigned int cpu_val      = (CPU_PWR_STAT_MASK     & ~(1 << (cpu         +  0)));
	unsigned int on_off_stat  = 0;
	bool         ret          = false;

	on_off_stat = mcdi_mbox_read(MCDI_MBOX_CPU_CLUSTER_PWR_STAT);

	if (((on_off_stat & CLUSTER_PWR_STAT_MASK) == cluster_val)
		&& ((on_off_stat & CPU_PWR_STAT_MASK) == cpu_val))
		ret = true;

	return ret;
#else
	bool ret = false;
	unsigned int on_off_stat  = 0;

	unsigned int check_mask =
		(cpu_cluster_pwr_stat_table[cpu]
			& ((mcdi_gov_data.avail_cluster_mask << 16) | mcdi_gov_data.avail_cpu_mask));

	on_off_stat = mcdi_mbox_read(MCDI_MBOX_CPU_CLUSTER_PWR_STAT);

	if (on_off_stat == check_mask)
		ret = true;

	return ret;
#endif
}

#ifdef ANY_CORE_DPIDLE_SODI
#if defined(CONFIG_MACH_MT6763)
static int mtk_idle_state_mapping[NR_TYPES] = {
	MCDI_STATE_DPIDLE,		/* IDLE_TYPE_DP */
	MCDI_STATE_SODI3,		/* IDLE_TYPE_SO3 */
	MCDI_STATE_SODI,		/* IDLE_TYPE_SO */
	MCDI_STATE_CLUSTER_OFF,	/* IDLE_TYPE_MC */
	MCDI_STATE_CLUSTER_OFF,	/* IDLE_TYPE_SL */
	MCDI_STATE_CLUSTER_OFF	/* IDLE_TYPE_RG */
};
#elif defined(CONFIG_MACH_MT6758)
static int mtk_idle_state_mapping[NR_TYPES] = {
	MCDI_STATE_DPIDLE,		/* IDLE_TYPE_DP */
	MCDI_STATE_SODI3,		/* IDLE_TYPE_SO3 */
	MCDI_STATE_SODI,		/* IDLE_TYPE_SO */
	MCDI_STATE_CLUSTER_OFF,		/* IDLE_TYPE_MCSO */
	MCDI_STATE_CLUSTER_OFF,		/* IDLE_TYPE_MC */
	MCDI_STATE_CLUSTER_OFF,		/* IDLE_TYPE_SL */
	MCDI_STATE_CLUSTER_OFF		/* IDLE_TYPE_RG */
};
#endif
#endif

#define NF_CHECK_MCDI_CONTROLLER_TOKEN          50
#define CHECK_MCDI_CONTROLLER_TOKEN_DELAY_US    100

bool mcdi_controller_token_get(int cpu)
{
	/* Try to get token from MCDI controller, which means only 1 CPU power ON */
	bool token_get = false;
	bool last_core_in_mcusys = false;
	int cnt = 0;
	unsigned long flags;

	mcdi_task_pause(true);

	for (cnt = 0; cnt < NF_CHECK_MCDI_CONTROLLER_TOKEN; cnt++) {

		if (mcdi_cpu_cluster_on_off_stat_check(cpu)) {
			token_get = true;
			break;
		}

		/* if other CPU(s) leave MCDI, means more than 1 CPU powered ON */
		spin_lock_irqsave(&mcdi_gov_spin_lock, flags);

		last_core_in_mcusys
			= (mcdi_gov_data.num_mcusys == mcdi_gov_data.avail_cnt_mcusys);

		spin_unlock_irqrestore(&mcdi_gov_spin_lock, flags);

		if (!last_core_in_mcusys) {
			token_get = false;
			break;
		}

		/* Resume MCDI task and wait for a while, then re-check */
		mcdi_task_pause(false);

		udelay(CHECK_MCDI_CONTROLLER_TOKEN_DELAY_US);

		mcdi_task_pause(true);
	}

	return token_get;
}

bool any_core_deepidle_sodi_residency_check(int cpu)
{
	bool ret = true;
	int  i   = 0;
	unsigned int cpu_mask = 0;
	unsigned long flags;

	spin_lock_irqsave(&mcdi_gov_spin_lock, flags);

	cpu_mask = mcdi_gov_data.avail_cpu_mask;

	spin_unlock_irqrestore(&mcdi_gov_spin_lock, flags);

	/* Check each CPU available controlled by MCDI */
	for (i = 0; i < NF_CPU; i++) {
		if (i == cpu)
			continue;

		if (!(cpu_mask & (1 << i)))
			continue;

		if (get_residency_latency_result(i) <= MCDI_STATE_CPU_OFF) {
			ret = false;
			break;
		}
	}

	return ret;
}

int any_core_deepidle_sodi_check(int cpu)
{
	int state = MCDI_STATE_CPU_OFF;
#ifdef ANY_CORE_DPIDLE_SODI
	int mtk_idle_state = IDLE_TYPE_RG;
#endif

	bool token_get = false;

	token_get = mcdi_controller_token_get(cpu);

	if (!token_get) {
		any_core_cpu_cond_inc(MULTI_CORE_CNT);
		goto end;
	}

	/* Check residency */
	if (!any_core_deepidle_sodi_residency_check(cpu)) {
		any_core_cpu_cond_inc(RESIDENCY_CNT);
		goto end;
	}

	any_core_cpu_cond_inc(LAST_CORE_CNT);

	state = MCDI_STATE_CLUSTER_OFF;

#ifdef ANY_CORE_DPIDLE_SODI
	/* Check other deepidle/SODI criteria */
	mtk_idle_state = mtk_idle_select(cpu);

	if (state >= 0 && state < NR_TYPES)
		state = mtk_idle_state_mapping[mtk_idle_state];
#endif
end:
	return state;
}

static bool is_mcdi_working(void)
{
	unsigned long flags;
	bool working = false;

	spin_lock_irqsave(&mcdi_feature_stat_spin_lock, flags);

	working = mcdi_feature_stat.enable && !mcdi_feature_stat.pause;

	spin_unlock_irqrestore(&mcdi_feature_stat_spin_lock, flags);

	return working;
}

/* Select deepidle/SODI/cluster OFF/CPU OFF/WFI */
int mcdi_governor_select(int cpu, int cluster_idx)
{
	unsigned long flags;
	int select_state = MCDI_STATE_WFI;
	bool last_core_in_mcusys = false;
	bool last_core_token_get = false;
	bool last_core_in_this_cluster = false;
	struct mcdi_status *mcdi_stat = NULL;

	if (!is_mcdi_working())
		return MCDI_STATE_WFI;

	if (boot_time_check != 1) {
		struct timespec uptime;
		unsigned long val;

		get_monotonic_boottime(&uptime);
		val = (unsigned long)uptime.tv_sec;

		if (val >= BOOT_TIME_LIMIT) {
			pr_warn("MCDI bootup check: PASS\n");
			boot_time_check = 1;
		} else {
			return MCDI_STATE_WFI;
		}
	}

	spin_lock_irqsave(&mcdi_gov_spin_lock, flags);

	/* increase MCDI num (MCUSYS/cluster) */
	mcdi_gov_data.num_mcusys++;
	mcdi_gov_data.num_cluster[cluster_idx]++;

	/* Check if the last CPU in MCUSYS/cluster entering MCDI */
	last_core_in_mcusys
		= (mcdi_gov_data.num_mcusys == mcdi_gov_data.avail_cnt_mcusys);
	last_core_in_this_cluster
		= (mcdi_gov_data.num_cluster[cluster_idx] == mcdi_gov_data.avail_cnt_cluster[cluster_idx]);

	/* update mcdi_status of this CPU */
	mcdi_stat = &mcdi_gov_data.status[cpu];

	mcdi_stat->valid         = true;
	mcdi_stat->enter_time_us = idle_get_current_time_us();
	mcdi_stat->predict_us    = get_menu_predict_us();

/*	trace_mcdi_enter(cpu, mcdi_gov_data.num_cluster[cluster_idx]); */

	if (last_core_in_mcusys && last_core_token == -1) {
		last_core_token      = cpu;
		last_core_token_get  = true;
	}

	spin_unlock_irqrestore(&mcdi_gov_spin_lock, flags);

	/* residency latency check of current CPU */
	update_residency_latency_result(cpu);

	if (!(cpu >= 0 && cpu <= 7))
		return MCDI_STATE_WFI;

	/* Check if any core deepidle/SODI can entered */
	if (mcdi_feature_stat.any_core && last_core_token_get) {

		any_core_cpu_cond_inc(PAUSE_CNT);

		select_state = any_core_deepidle_sodi_check(cpu);

		/* Resume MCDI task if anycore deepidle/SODI check failed */
		if (!is_anycore_dpidle_sodi_state(select_state)) {
			mcdi_task_pause(false);

			spin_lock_irqsave(&mcdi_gov_spin_lock, flags);

			WARN_ON(last_core_token != cpu);

			last_core_token = -1;

			spin_unlock_irqrestore(&mcdi_gov_spin_lock, flags);
		}
	}

	if (!is_anycore_dpidle_sodi_state(select_state)) {
		if (mcdi_feature_stat.cluster_off && last_core_in_this_cluster) {
			/* Check if Cluster can be powered OFF */

			if (cluster_residency_check(cpu)) {
				select_state = MCDI_STATE_CLUSTER_OFF;
/*				trace_mcdi_cluster_enter(cluster_idx); */
			} else
				select_state = MCDI_STATE_CPU_OFF;

		} else {

			/* Only CPU powered OFF */
			select_state = MCDI_STATE_CPU_OFF;
		}
	}

	return select_state;
}

void mcdi_governor_reflect(int cpu, int state)
{
	unsigned long flags;
	int cluster_idx = cluster_idx_get(cpu);
	struct mcdi_status *mcdi_stat = NULL;
	bool cluster_power_on = false;

	/* decrease MCDI num (MCUSYS/cluster) */
	spin_lock_irqsave(&mcdi_gov_spin_lock, flags);

	if (state == MCDI_STATE_CPU_OFF
			|| state == MCDI_STATE_CLUSTER_OFF
			|| state == MCDI_STATE_SODI
			|| state == MCDI_STATE_DPIDLE
			|| state == MCDI_STATE_SODI3) {

		mcdi_gov_data.num_mcusys--;
		mcdi_gov_data.num_cluster[cluster_idx]--;
	}

	mcdi_stat = &mcdi_gov_data.status[cpu];

	mcdi_stat->valid         = false;
	mcdi_stat->state         = 0;
	mcdi_stat->enter_time_us = 0;
	mcdi_stat->predict_us    = 0;

/*	trace_mcdi_leave(cpu, mcdi_gov_data.num_cluster[cluster_idx]); */
	cluster_power_on = (mcdi_gov_data.num_cluster[cluster_idx] == 3);

	if (last_core_token == cpu)
		last_core_token = -1;

	spin_unlock_irqrestore(&mcdi_gov_spin_lock, flags);

	/* Resume MCDI task after anycore deepidle/SODI return */
	if (is_anycore_dpidle_sodi_state(state))
		mcdi_task_pause(false);

#if 0
	if (cluster_power_on)
		trace_mcdi_cluster_leave(cluster_idx);
#endif
}

void mcdi_avail_cpu_cluster_update(void)
{
	unsigned long flags;
	int i, cpu, cluster_idx;
	unsigned int cpu_mask = 0;

	spin_lock_irqsave(&mcdi_gov_spin_lock, flags);

	mcdi_gov_data.avail_cnt_mcusys = num_online_cpus();
	mcdi_gov_data.avail_cpu_mask = 0;
	mcdi_gov_data.avail_cluster_mask = 0;

	for (i = 0; i < NF_CLUSTER; i++)
		mcdi_gov_data.avail_cnt_cluster[i] = 0;

	for (cpu = 0; cpu < NF_CPU; cpu++) {

		cluster_idx = cluster_idx_get(cpu);

		if (cpu_online(cpu)) {
			mcdi_gov_data.avail_cnt_cluster[cluster_idx]++;
			mcdi_gov_data.avail_cpu_mask |= (1 << cpu);
		}
	}

	for (cluster_idx = 0; cluster_idx < NF_CLUSTER; cluster_idx++) {
		if ((mcdi_gov_data.avail_cpu_mask & (all_cpu_in_cluster_pwr_off_mask[cluster_idx])))
			mcdi_gov_data.avail_cluster_mask |= (1 << cluster_idx);
	}


	cpu_mask = mcdi_gov_data.avail_cpu_mask;

	spin_unlock_irqrestore(&mcdi_gov_spin_lock, flags);

	update_avail_cpu_mask_to_mcdi_controller(cpu_mask);

#if 0
	pr_warn("online = %d, avail: mcusys = %d, cluster[0] = %d, [1] = %d, cpu_mask = %04x, cluster_mask = %04x\n",
		num_online_cpus(),
		mcdi_gov_data.avail_cnt_mcusys,
		mcdi_gov_data.avail_cnt_cluster[0],
		mcdi_gov_data.avail_cnt_cluster[1],
		mcdi_gov_data.avail_cpu_mask,
		mcdi_gov_data.avail_cluster_mask
	);
#endif
}

void set_mcdi_enable_status(bool enabled)
{
	unsigned long flags;

	spin_lock_irqsave(&mcdi_feature_stat_spin_lock, flags);

	mcdi_feature_stat.enable = enabled;

	spin_unlock_irqrestore(&mcdi_feature_stat_spin_lock, flags);
}

void set_mcdi_s_state(int state)
{
	unsigned long flags;
	bool cluster_off = false;
	bool any_core = false;

	if (!(state >= MCDI_STATE_CPU_OFF && state <= MCDI_STATE_SODI3))
		return;

	switch (state) {
	case MCDI_STATE_CPU_OFF:
		cluster_off = false;
		any_core    = false;
		break;
	case MCDI_STATE_CLUSTER_OFF:
		cluster_off = true;
		any_core    = false;
		break;
	case MCDI_STATE_SODI:
	case MCDI_STATE_DPIDLE:
	case MCDI_STATE_SODI3:
		cluster_off = true;
		any_core    = true;
		break;
	default:
		return;
	}

	pr_err("set_mcdi_s_state = %d\n", state);

	spin_lock_irqsave(&mcdi_feature_stat_spin_lock, flags);

	mcdi_feature_stat.s_state     = state;
	mcdi_feature_stat.cluster_off = cluster_off;
	mcdi_feature_stat.any_core    = any_core;

	spin_unlock_irqrestore(&mcdi_feature_stat_spin_lock, flags);
}

void mcdi_governor_init(void)
{
	unsigned long flags;
	int i;

	mcdi_avail_cpu_cluster_update();

	spin_lock_irqsave(&mcdi_gov_spin_lock, flags);

	mcdi_gov_data.num_mcusys           = 0;
	mcdi_gov_data.num_cluster[0]       = 0;
	mcdi_gov_data.num_cluster[1]       = 0;

	for (i = 0; i < NF_CPU; i++) {
		mcdi_gov_data.status[i].state         = 0;
		mcdi_gov_data.status[i].enter_time_us = 0;
		mcdi_gov_data.status[i].predict_us    = 0;
	}

	spin_unlock_irqrestore(&mcdi_gov_spin_lock, flags);

	set_mcdi_enable_status(true);
	set_mcdi_s_state(MCDI_STATE_SODI3);
}

void get_mcdi_feature_status(struct mcdi_feature_status *stat)
{
	unsigned long flags;

	spin_lock_irqsave(&mcdi_feature_stat_spin_lock, flags);

	stat->enable      = mcdi_feature_stat.enable;
	stat->pause       = mcdi_feature_stat.pause;
	stat->cluster_off = mcdi_feature_stat.cluster_off;
	stat->any_core    = mcdi_feature_stat.any_core;
	stat->s_state     = mcdi_feature_stat.s_state;

	spin_unlock_irqrestore(&mcdi_feature_stat_spin_lock, flags);
}

void get_mcdi_avail_mask(unsigned int *cpu_mask, unsigned int *cluster_mask)
{
	unsigned long flags;

	spin_lock_irqsave(&mcdi_gov_spin_lock, flags);

	*cpu_mask     = mcdi_gov_data.avail_cpu_mask;
	*cluster_mask = mcdi_gov_data.avail_cluster_mask;

	spin_unlock_irqrestore(&mcdi_gov_spin_lock, flags);
}

void mcdi_state_pause(bool pause)
{
	unsigned long flags;

	spin_lock_irqsave(&mcdi_feature_stat_spin_lock, flags);

	mcdi_feature_stat.pause = pause;

	spin_unlock_irqrestore(&mcdi_feature_stat_spin_lock, flags);
}

