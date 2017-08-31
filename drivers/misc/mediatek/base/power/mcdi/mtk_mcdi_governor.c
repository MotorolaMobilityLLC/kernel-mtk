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

#include <linux/pm_qos.h>
#include <linux/spinlock.h>

#include <mtk_mcdi.h>
#include <mtk_mcdi_state.h>

#include <trace/events/mtk_events.h>

#define CHECK_CLUSTER_RESIDENCY     0

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
	int avail_cnt_mcusys;
	int avail_cnt_cluster[NF_CLUSTER];
	struct mcdi_status status[NF_CPU];
};

static struct mcdi_gov mcdi_gov_data;

static DEFINE_SPINLOCK(mcdi_gov_spin_lock);

static int mcdi_residency_vote[NF_CPU];

static inline unsigned long long idle_get_current_time_us(void)
{
	struct timeval t;

	do_gettimeofday(&t);
	return ((t.tv_sec & 0xFFF) * 1000000 + t.tv_usec);
}

void update_residency_vote_result(int cpu)
{
	struct cpuidle_driver *tbl = mcdi_state_tbl_get();
	int i;
	unsigned int predict_us = get_menu_predict_us();
	int latency_req = pm_qos_request(PM_QOS_CPU_DMA_LATENCY);
	int state = 0;

	for (i = 0; i < tbl->state_count; i++) {
		struct cpuidle_state *s = &tbl->states[i];

		if (s->target_residency > predict_us)
			continue;
		if (s->exit_latency > latency_req)
			continue;

		state = i;
	}

	mcdi_residency_vote[cpu] = state;
}

int get_residency_vote_result(int cpu)
{
	return mcdi_residency_vote[cpu];
}

#if CHECK_CLUSTER_RESIDENCY
bool cluster_residency_check(int cpu)
{
	struct cpuidle_driver *mcdi_state_tbl = mcdi_state_tbl_get();
	int cluster_idx = cpu / 4;
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

/* Select deepidle/SODI/cluster OFF/CPU OFF/WFI */
int mcdi_governor_select(int cpu)
{
	unsigned long flags;
	int select_state = MCDI_STATE_WFI;
	int cluster_idx = cpu / 4;
	bool last_core_in_mcusys = false;
	bool last_core_in_this_cluster = false;
	struct mcdi_status *mcdi_stat = NULL;

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

	trace_mcdi_enter(cpu, mcdi_gov_data.num_cluster[cluster_idx]);

	spin_unlock_irqrestore(&mcdi_gov_spin_lock, flags);

	/* residency voting of current CPU */
	update_residency_vote_result(cpu);

	if (last_core_in_mcusys) {

		/* Check any core deepidle/SODI */

		/* 1. Pause MCDI */

		/* 2. Check power ON CPU status from SSPM */

		/* 3. Check other deepidle/SODI criteria */
	} else if (last_core_in_this_cluster) {
		/* Check if Cluster can be powered OFF */

		if (cluster_residency_check(cpu)) {
			select_state = MCDI_STATE_CLUSTER_OFF;
			trace_mcdi_cluster_enter(cluster_idx);
		} else
			select_state = MCDI_STATE_CPU_OFF;

	} else {

		/* Only CPU powered OFF */
		select_state = MCDI_STATE_CPU_OFF;
	}

	return select_state;
}

void mcdi_governor_reflect(int cpu, int state)
{
	unsigned long flags;
	int cluster_idx = cpu / 4;
	struct mcdi_status *mcdi_stat = NULL;
	bool cluster_power_on = false;

	/* decrease MCDI num (MCUSYS/cluster) */
	spin_lock_irqsave(&mcdi_gov_spin_lock, flags);

	mcdi_gov_data.num_mcusys--;
	mcdi_gov_data.num_cluster[cluster_idx]--;

	mcdi_stat = &mcdi_gov_data.status[cpu];

	mcdi_stat->valid         = false;
	mcdi_stat->state         = 0;
	mcdi_stat->enter_time_us = 0;
	mcdi_stat->predict_us    = 0;

	trace_mcdi_leave(cpu, mcdi_gov_data.num_cluster[cluster_idx]);
	cluster_power_on = (mcdi_gov_data.num_cluster[cluster_idx] == 3);

	spin_unlock_irqrestore(&mcdi_gov_spin_lock, flags);

	if (cluster_power_on)
		trace_mcdi_cluster_leave(cluster_idx);
}

void mcdi_avail_cpu_cluster_update(void)
{
	unsigned long flags;
	int i, cpu, cluster_idx;

	/* TODO */
	spin_lock_irqsave(&mcdi_gov_spin_lock, flags);

	mcdi_gov_data.avail_cnt_mcusys = num_online_cpus();
	mcdi_gov_data.avail_cpu_mask = 0;

	for (i = 0; i < NF_CLUSTER; i++)
		mcdi_gov_data.avail_cnt_cluster[i] = 0;

	for (cpu = 0; cpu < NF_CPU; cpu++) {

		cluster_idx = cpu / 4;

		if (cpu_online(cpu)) {
			mcdi_gov_data.avail_cnt_cluster[cluster_idx]++;
			mcdi_gov_data.avail_cpu_mask |= (1 << cpu);
		}
	}

	spin_unlock_irqrestore(&mcdi_gov_spin_lock, flags);

	pr_info("online = %d, avail_cnt_mcusys = %d, avail_cnt_cluster[0] = %d, [1] = %d, avail_cpu_mask = %04x\n",
		num_online_cpus(),
		mcdi_gov_data.avail_cnt_mcusys,
		mcdi_gov_data.avail_cnt_cluster[0],
		mcdi_gov_data.avail_cnt_cluster[1],
		mcdi_gov_data.avail_cpu_mask
	);
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
}

