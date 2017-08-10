/*
 * Copyright (C) 2016 MediaTek Inc.
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

#ifndef __MTK_CHARGER_INTF
#define __MTK_CHARGER_INTF

#include <linux/types.h>

enum rt_charging_status {
	RT_CHG_STATUS_READY = 0,
	RT_CHG_STATUS_PROGRESS,
	RT_CHG_STATUS_DONE,
	RT_CHG_STATUS_FAULT,
	RT_CHG_STATUS_MAX,
};

/* You must implement all of the following interfaces */
extern int rt_charger_hw_init(void);
extern int rt_charger_sw_init(void);
extern int rt_charger_dump_register(void);
extern int rt_charger_enable_charging(const u8 enable);
extern int rt_charger_enable_hz(const u8 enable);
extern int rt_charger_enable_te(const u8 enable);
extern int rt_charger_enable_te_shutdown(const u8 enable);
extern int rt_charger_enable_timer(const u8 enable);
extern int rt_charger_enable_otg(const u8 enable);
extern int rt_charger_enable_pumpX(const u8 enable);
extern int rt_charger_enable_aicr(const u8 enable);
extern int rt_charger_enable_mivr(const u8 enable);
extern int rt_charger_enable_power_path(const u8 enable);

extern int rt_charger_set_ichg(const u32 ichg);
extern int rt_charger_set_ieoc(const u32 ieoc);
extern int rt_charger_set_aicr(const u32 aicr);
extern int rt_charger_set_mivr(const u32 mivr);
extern int rt_charger_set_battery_voreg(const u32 voreg);
extern int rt_charger_set_boost_voreg(const u32 voreg);
extern int rt_charger_set_boost_current_limit(const u32 ilimit);
extern int rt_charger_set_ta_current_pattern(const u8 is_pump_up);

extern int rt_charger_get_charging_status(enum rt_charging_status *chg_stat);
extern int rt_charger_get_ichg(u32 *ichg);
extern int rt_charger_get_ieoc(u32 *ieoc);
extern int rt_charger_get_aicr(u32 *aicr);
extern int rt_charger_get_mivr(u32 *mivr);
extern int rt_charger_get_battery_voreg(u32 *voreg);
extern int rt_charger_get_boost_voreg(u32 *voreg);
extern int rt_charger_get_temperature(int *min_temp, int *max_temp);

extern int rt_charger_is_charging_enable(u8 *enable);
extern int rt_charger_reset_wchdog_timer(void);

#endif
