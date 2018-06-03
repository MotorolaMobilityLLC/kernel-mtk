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

/* pe3.0 */
#define CV_LIMIT 6000	/* vbus upper bound */
#define BAT_UPPER_BOUND 4350	/* battery upper bound */
#define BAT_LOWER_BOUND 3000	/* battery low bound */

#define CC_SS_INIT 1500	/*init charging current (mA)*/
#define CC_INIT 5000	/*max charging current (mA)*/
#define CC_INIT_BAD_CABLE1 4000	/*charging current(mA) for bad Cable1*/
#define CC_INIT_BAD_CABLE2 3000	/*charging current(mA) for bad Cable2*/
#define CC_INIT_R 230	/*good cable max impedance*/
#define CC_INIT_BAD_CABLE1_R 280	/*bad cable1 max impedance*/
#define CC_INIT_BAD_CABLE2_R 360	/*bad cable2 max impedance*/

#define CC_NORMAL 4000000	/*normal charging ibus OC threshold (uA)*/
#define CC_MAX 6500000	/*PE3.0 ibus OC threshold (uA)*/
#define CC_END 2000	/*PE3.0 min charging current (mA)*/
#define CC_STEP 200	/*CC state charging current step (mA)*/
#define CC_SS_STEP 500	/*soft start state charging current step (mA)*/
#define CC_SS_STEP2 200	/*soft start state charging current step (mA),when battery voltage > CC_SS_STEP2*/
#define CC_SS_STEP3 100	/*soft start state charging current step (mA),when battery voltage > CC_SS_STEP3*/
#define CV_SS_STEP2 4000	/*battery voltage threshold for CC_SS_STEP2*/
#define CV_SS_STEP3 4200	/*battery voltage threshold for CC_SS_STEP3*/
#define CC_SS_BLANKING 100	/*polling duraction for init/soft start state(ms)*/
#define CC_BLANKING 1000	/*polling duraction for CC state(ms)*/
#define CHARGER_TEMP_MAX 120	/*max charger IC temperature*/
#define TA_TEMP_MAX 80	/*max adapter temperature*/
#define VBUS_OV_GAP 100
#define FOD_CURRENT 200 /*FOD current threshold*/
#define R_VBAT_MIN 40	/*min R_VBAT*/
#define R_SW_MIN 20	/*min R_SW*/
#define PE30_MAX_CHARGING_TIME 5400	/*PE3.0 max chargint time (sec)*/
#if defined(CONFIG_MTK_JEITA_STANDARD_SUPPORT)
#define BATTERY_TEMP_MIN TEMP_T2_THRESHOLD
#define BATTERY_TEMP_MAX TEMP_T3_THRESHOLD
#else
#define BATTERY_TEMP_MIN 0	/*PE3.0 min battery temperature*/
#define BATTERY_TEMP_MAX 50	/*PE3.0 man battery temperature*/
#endif

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

