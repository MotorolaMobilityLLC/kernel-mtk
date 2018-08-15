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

#include <linux/module.h>
#include <linux/kernel.h>

#include <mtk_spm.h>	/* SPM TWAM API */
#include <mtk_idle_internal.h>
#include <mtk_spm_reg.h>
/* SPM TAWM */
#define TRIGGER_TYPE                (2) /* b'10: high */
#define TWAM_PERIOD_MS              (1000)
#define WINDOW_LEN_SPEED            (TWAM_PERIOD_MS * 0x65B8)
#define WINDOW_LEN_NORMAL           (TWAM_PERIOD_MS * 0xD)
#define GET_EVENT_RATIO_SPEED(x)    ((x)/(WINDOW_LEN_SPEED/1000))
#define GET_EVENT_RATIO_NORMAL(x)   ((x)/(WINDOW_LEN_NORMAL/1000))


/********************************************************************
 * Weak functions for chip dependent flow.
 *******************************************************************/

/* [ByChip] Internal weak functions: implemented in mtk_spm_irq.c */
void __attribute__((weak)) spm_twam_register_handler(twam_handler_t handler) {}

void __attribute__((weak)) spm_twam_enable_monitor(bool en_monitor,
bool debug_signal, twam_handler_t cb_handler);
void __attribute__((weak)) spm_twam_disable_monitor(void) {}
void __attribute__((weak)) spm_twam_set_idle_select(unsigned int sel) {}
void __attribute__((weak)) spm_twam_set_window_length(unsigned int len) {}

struct twam_cfg twamsig = {};

static struct mtk_idle_twam idle_twam = {
	.running = false,
	.speed_mode = true,
	.sel = 0,
	.event = 29,
};

struct mtk_idle_twam *mtk_idle_get_twam(void)
{
	return &idle_twam;
}

static void mtk_idle_twam_callback(struct twam_cfg *ts, struct twam_select *sel)
{

	pr_notice("Power/swap spm twam (sel%d: %d) ratio: %5u/1000, %d, %d\n",
		sel->signal0, sel->id0,
		(idle_twam.speed_mode) ? GET_EVENT_RATIO_SPEED(ts->byte0.id) :
			GET_EVENT_RATIO_NORMAL(ts->byte0.id),
			idle_twam.speed_mode, ts->byte0.id);
	pr_notice("Power/swap spm twam (sel%d: %d) ratio: %5u/1000, %d, %d\n",
		sel->signal1, sel->id1,
		(idle_twam.speed_mode) ? GET_EVENT_RATIO_SPEED(ts->byte1.id) :
			GET_EVENT_RATIO_NORMAL(ts->byte1.id),
			idle_twam.speed_mode, ts->byte1.id);
	pr_notice("Power/swap spm twam (sel%d: %d) ratio: %5u/1000, %d, %d\n",
		sel->signal2, sel->id2,
		(idle_twam.speed_mode) ? GET_EVENT_RATIO_SPEED(ts->byte2.id) :
			GET_EVENT_RATIO_NORMAL(ts->byte2.id),
			idle_twam.speed_mode, ts->byte2.id);
	pr_notice("Power/swap spm twam (sel%d: %d) ratio: %5u/1000, %d, %d\n",
		sel->signal3, sel->id3,
		(idle_twam.speed_mode) ? GET_EVENT_RATIO_SPEED(ts->byte3.id) :
			GET_EVENT_RATIO_NORMAL(ts->byte3.id),
			idle_twam.speed_mode, ts->byte3.id);
}

void mtk_idle_twam_disable(void)
{
	if (idle_twam.running == false)
		return;
	spm_twam_enable_monitor(false, false, NULL);
	idle_twam.running = false;
}

void mtk_idle_twam_enable(unsigned int event)
{
	if (idle_twam.event != event)
		mtk_idle_twam_disable();

	if (idle_twam.running == true)
		return;

	idle_twam.event = (event < 32) ? event : 29;

	if (idle_twam.sel == 0) {
		twamsig.byte0.signal = idle_twam.sel;
		twamsig.byte0.id = idle_twam.event;
	} else if (idle_twam.sel == 1) {
		twamsig.byte1.signal = idle_twam.sel;
		twamsig.byte1.id = idle_twam.event;
	} else if (idle_twam.sel == 2) {
		twamsig.byte2.signal = idle_twam.sel;
		twamsig.byte2.id = idle_twam.event;
	} else if (idle_twam.sel == 3) {
		twamsig.byte3.signal = idle_twam.sel;
		twamsig.byte3.id = idle_twam.event;
	}

	twamsig.byte0.monitor_type = TRIGGER_TYPE;
	twamsig.byte1.monitor_type = TRIGGER_TYPE;
	twamsig.byte2.monitor_type = TRIGGER_TYPE;
	twamsig.byte3.monitor_type = TRIGGER_TYPE;

	spm_twam_config_channel(&twamsig,
		idle_twam.speed_mode,
		(idle_twam.speed_mode) ? WINDOW_LEN_SPEED : WINDOW_LEN_NORMAL);
	spm_twam_enable_monitor(true, false, mtk_idle_twam_callback);
	idle_twam.running = true;
}
