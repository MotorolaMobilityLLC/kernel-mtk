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

#include <mtk_direct_charge_vdm.h>

#ifndef __MTK_PE_30_INTF_H
#define __MTK_PE_30_INTF_H

struct mtk_pe30_cap {
	int cur;
	int vol;
	int temp;
};

struct mtk_pe30 {

	struct mutex pe30_mutex;
	struct wake_lock pe30_wakelock;
	struct hrtimer mtk_charger_pe30_timer;
	struct task_struct *mtk_charger_pe30_thread;
	bool mtk_charger_pe30_thread_flag;
	wait_queue_head_t  mtk_charger_pe30_thread_waiter;
	bool is_pe30_done; /* one plug in only do pe30 once */
	int pe30_charging_state;
	void *tcpc;
	ktime_t ktime;
	int r_total, r_cable, r_sw, r_vbat;
	int batteryTemperature;
	int r_cable_1, r_cable_2;
	bool is_vdm_rdy;
	struct mtk_vdm_ta_cap current_cap;
	int charging_current_limit;
	int charging_current_limit_by_r;
	struct timespec startTime, endTime;
	int pmic_vbus, TA_vbus;
	int vbus_cali;

};

#ifdef CONFIG_MTK_PUMP_EXPRESS_PLUS_30_SUPPORT

extern bool mtk_pe30_init(struct charger_manager *info);
extern bool mtk_is_TA_support_pe30(struct charger_manager *info);
extern bool mtk_is_pe30_running(struct charger_manager *info);
extern void mtk_pe30_plugout_reset(struct charger_manager *info);
extern bool mtk_pe30_check_charger(struct charger_manager *info);
extern void mtk_pe30_set_pd_rdy(struct charger_manager *info, bool rdy);
extern void mtk_pe30_set_charging_current_limit(struct charger_manager *info, int cur);
extern int mtk_pe30_get_charging_current_limit(struct charger_manager *info);
extern void mtk_pe30_end(struct charger_manager *info);
extern bool mtk_pe30_get_is_enable(struct charger_manager *pinfo);
extern void mtk_pe30_set_is_enable(struct charger_manager *pinfo, bool enable);
extern void chrdet_int_handler(void);

extern int pe30_dc_enable(struct charger_manager *info, unsigned char charging_enable);
extern int pe30_dc_kick_wdt(struct charger_manager *info);
extern int pe30_dc_set_ibus_oc(struct charger_manager *info, unsigned int cur);
extern int pe30_dc_get_temperature(struct charger_manager *info, int *min_temp, int *max_temp);
extern int pe30_dc_enable_chip(struct charger_manager *info, unsigned char en);
extern int pe30_dc_set_vbus_ov(struct charger_manager *info, unsigned int vol);


extern int pe30_chr_enable_charge(struct charger_manager *info, bool charging_enable);
extern int pe30_chr_get_input_current(struct charger_manager *info, unsigned int *cur);
extern int pe30_chr_enable_power_path(struct charger_manager *info, bool en);
extern int pe30_chr_get_tchr(struct charger_manager *info, int *min_temp, int *max_temp);

extern int mtk_cooler_is_abcct_unlimit(void);


extern bool mtk_is_pd_chg_ready(void);
extern int tcpm_hard_reset(void *ptr);
extern int tcpm_set_direct_charge_en(void *tcpc_dev, bool en);
extern int tcpm_get_cable_capability(void *tcpc_dev, unsigned char *capability);
extern int mtk_clr_ta_pingcheck_fault(void *ptr);
extern bool mtk_is_pe30_en_unlock(void);
#else
static inline bool mtk_pe30_init(struct charger_manager *info)
{
	return false;
}

static inline bool mtk_is_TA_support_pe30(struct charger_manager *info)
{
	return false;
}

static inline bool mtk_is_pe30_running(struct charger_manager *info)
{
	return false;
}

static inline void mtk_pe30_set_charging_current_limit(struct charger_manager *info, int cur)
{
}

static inline int mtk_pe30_get_charging_current_limit(struct charger_manager *info)
{
	return true;
}

static inline void mtk_pe30_end(struct charger_manager *info)
{
}

static inline void mtk_pe30_plugout_reset(struct charger_manager *info)
{
}

static inline bool mtk_pe30_check_charger(struct charger_manager *info)
{
	return false;
}

static inline bool mtk_pe30_get_is_enable(struct charger_manager *pinfo)
{
	return false;
}

static inline void mtk_pe30_set_is_enable(struct charger_manager *pinfo, bool enable)
{
}

#endif

#endif /* __MTK_PE_30_INTF_H */

