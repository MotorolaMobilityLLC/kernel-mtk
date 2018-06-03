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

#if defined(CONFIG_MACH_MT6739)
#include "mtk_mcdi_mt6739.h"
#endif

#define CPU_PWR_STAT_MASK       0x000000FF
#define CLUSTER_PWR_STAT_MASK   0x00030000

extern u32 aee_rr_rec_mcdi_val(int id, u32 val);

int cluster_idx_get(int cpu);
unsigned int get_menu_predict_us(void);
bool mcdi_task_pause(bool paused);
unsigned int mcdi_mbox_read(void __iomem *id);
void mcdi_mbox_write(void __iomem *id, unsigned int val);
void update_avail_cpu_mask_to_mcdi_controller(unsigned int cpu_mask);
bool is_cpu_pwr_on_event_pending(void);
int mcdi_get_mcdi_idle_state(int idx);
void mcdi_status_init(void);
void mcdi_of_init(void);
unsigned int get_pwr_stat_check_map(int type, int idx);

#endif /* __MTK_MCDI_H__ */
