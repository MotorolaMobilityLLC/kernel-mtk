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

enum {
	MCDI_SMC_EVENT_ASYNC_WAKEUP_EN = 0,

	NF_MCDI_SMC_EVENT
};

extern void aee_rr_rec_mcdi_val(int id, unsigned int val);

/* mtk_menu */
unsigned int get_menu_predict_us(void);

/* main */
int wfi_enter(int cpu);
int mcdi_enter(int cpu);
bool mcdi_task_pause(bool paused);
void update_avail_cpu_mask_to_mcdi_controller(unsigned int cpu_mask);
bool is_cpu_pwr_on_event_pending(void);
void update_cpu_isolation_mask_to_mcdi_controller(unsigned int iso_mask);
void mcdi_wakeup_all_cpu(void);

#endif /* __MTK_MCDI_H__ */
