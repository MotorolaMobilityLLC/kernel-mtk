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

#ifndef __MTK_MCDI_GOVERNOR_H__
#define __MTK_MCDI_GOVERNOR_H__

enum {
	PAUSE_CNT	= 0,
	CPU_ONOFF_STAT_FAIL_CNT,
	RESIDENCY_FAIL_CNT,
	CPU_COND_PASS_CNT,
	NF_ANY_CORE_CPU_COND_INFO
};

int mcdi_governor_select(int cpu, int cluster_idx);
void mcdi_governor_reflect(int cpu, int state);
void mcdi_avail_cpu_cluster_update(void);
void mcdi_governor_init(void);
void set_mcdi_enable_status(bool enabled);
void get_mcdi_enable_status(bool *enabled, bool *paused);
int get_residency_latency_result(int cpu);
void mcdi_state_pause(bool pause);
void any_core_cpu_cond_get(unsigned long buf[NF_ANY_CORE_CPU_COND_INFO]);

#endif /* __MTK_MCDI_GOVERNOR_H__ */
