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

#ifndef __MTK_MCDI_H__
#define __MTK_MCDI_H__

#define NF_CPU                  8
#define NF_CLUSTER              2
#define CPU_PWR_STAT_MASK       0x000000FF
#define CLUSTER_PWR_STAT_MASK   0x00030000

enum {
	ALL_CPU_IN_CLUSTER = 0,
	CPU_CLUSTER,
	CPU_IN_OTHER_CLUSTER,
	OTHER_CLUSTER_IDX,
	NF_PWR_STAT_MAP_TYPE
};

enum {
	MCDI_SMC_EVENT_ASYNC_WAKEUP_EN = 0,

	NF_MCDI_SMC_EVENT
};

extern void aee_rr_rec_mcdi_val(int id, u32 val);

int cluster_idx_get(int cpu);
unsigned int get_menu_predict_us(void);
bool mcdi_task_pause(bool paused);
unsigned int mcdi_mbox_read(int id);
void mcdi_mbox_write(int id, unsigned int val);
void update_avail_cpu_mask_to_mcdi_controller(unsigned int cpu_mask);
bool is_cpu_pwr_on_event_pending(void);
unsigned int get_pwr_stat_check_map(int type, int idx);
int mcdi_get_state_tbl(int idx);
int mcdi_get_mcdi_idle_state(int idx);
unsigned int mcdi_get_buck_ctrl_mask(void);
void mcdi_status_init(void);
void mcdi_of_init(void);
void update_cpu_isolation_mask_to_mcdi_controller(unsigned int iso_mask);
void mcdi_wakeup_all_cpu(void);

#endif /* __MTK_MCDI_H__ */
