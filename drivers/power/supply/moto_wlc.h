/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __MOTO_WLC_H
#define __MOTO_WLC_H

#include "mtk_charger_algorithm_class.h"


#define WLC_V_CHARGER_MIN 4600000 /* 4.6 V */
/*wireless input current and charging current*/
#define WIRELESS_CHARGER_MAX_CURRENT			3000000
#define WIRELESS_CHARGER_MAX_INPUT_CURRENT		1150000
#define DISABLE_VBAT_THRESHOLD -1
#define ECABLEOUT	1	/* cable out */
#define EHAL		2	/* hal operation error */


#define WLC_ERROR_LEVEL	1
#define WLC_INFO_LEVEL	2
#define WLC_DEBUG_LEVEL	3

extern int wlc_get_debug_level(void);
#define wlc_err(fmt, args...)					\
do {								\
	if (wlc_get_debug_level() >= WLC_ERROR_LEVEL) {	\
		pr_info(fmt, ##args);				\
	}							\
} while (0)

#define wlc_info(fmt, args...)					\
do {								\
	if (wlc_get_debug_level() >= WLC_ERROR_LEVEL) { \
		pr_info(fmt, ##args);			\
	}							\
} while (0)

#define wlc_dbg(fmt, args...)					\
do {								\
	if (wlc_get_debug_level() >= WLC_ERROR_LEVEL) {	\
		pr_info(fmt, ##args);				\
	}							\
} while (0)


enum wlc_state_enum {
	WLC_HW_UNINIT = 0,
	WLC_HW_FAIL,
	WLC_HW_READY,
	WLC_TA_NOT_SUPPORT,
	WLC_RUN,
	WLC_DONE,
};

struct wlc_profile {
	unsigned int vbat;
	unsigned int vchr;
};

struct mtk_wlc {
	struct platform_device *pdev;
	struct chg_alg_device *alg;

	struct mutex access_lock;
	struct wakeup_source *suspend_lock;
	struct mutex cable_out_lock;
	struct mutex data_lock;
	bool is_cable_out_occur; /* Plug out happened while detect PE+20 */
	struct power_supply *bat_psy;
	struct power_supply *wlc_psy;
	int idx;
	int vbus;

	int min_charger_voltage;
	int ref_vbat; /* Vbat with cable in */
	/* module parameters*/
	int cv;
	int old_cv;
	int wlc_6pin_en;
	int stop_6pin_re_en;
	int input_current_limit1;
	int input_current_limit2;
	int charging_current_limit1;
	int charging_current_limit2;

	/* current IC setting */
	int input_current1;
	int charging_current1;
	int input_current2;
	int charging_current2;


	enum wlc_state_enum state;
	int wls_control_en;
	int mmi_fcc;
	/*wireless charger*/
	int wireless_charger_max_current;
	int wireless_charger_max_input_current;
	int input_current_limit;
	bool cable_ready;

	/*wireless thermal*/
	struct thermal_cooling_device *tcd;
	int max_state;
	int cur_state;
};
struct moto_wls_chg_ops {
	void *data;
	void (*wls_current_select)(int  *icl, int *vbus, bool *cable_ready);
	void (*wls_set_battery_soc)(int uisoc);
	void (*wls_stop_epp)(void);
	void (*wls_notify_thermal_icl)(int thermal_icl);
	void (*wls_notify_cur_state)(int cur_state, int wls_ccl);
};

extern int wlc_hal_init_hardware(struct chg_alg_device *alg);
extern int wlc_hal_get_uisoc(struct chg_alg_device *alg);
extern int wlc_hal_get_charger_type(struct chg_alg_device *alg);
extern int wlc_hal_set_mivr(struct chg_alg_device *alg,
	enum chg_idx chgidx, int uV);

extern bool wlc_hal_is_chip_enable(struct chg_alg_device *alg,
	enum chg_idx chgidx);
extern int wlc_hal_enable_charger(struct chg_alg_device *alg,
	enum chg_idx chgidx, bool en);
extern int wlc_hal_reset_ta(struct chg_alg_device *alg, enum chg_idx chgidx);
extern int wlc_hal_get_vbus(struct chg_alg_device *alg);
extern int wlc_hal_get_vbat(struct chg_alg_device *alg);
extern int wlc_hal_get_ibat(struct chg_alg_device *alg);

extern int wlc_hal_set_charging_current(struct chg_alg_device *alg,
	enum chg_idx chgidx, u32 ua);
extern int wlc_hal_get_charging_current(struct chg_alg_device *alg,
	enum chg_idx chgidx, u32 *ua);
extern int wlc_hal_set_input_current(struct chg_alg_device *alg,
	enum chg_idx chgidx, u32 ua);
extern int wlc_hal_get_mivr_state(struct chg_alg_device *alg,
	enum chg_idx chgidx, bool *in_loop);

extern int wlc_hal_enable_vbus_ovp(struct chg_alg_device *alg, bool enable);
extern int wlc_hal_set_cv(struct chg_alg_device *alg,
	enum chg_idx chgidx, u32 uv);

extern int wlc_hal_get_min_charging_current(struct chg_alg_device *alg,
	enum chg_idx chgidx, u32 *uA);
extern int wlc_hal_get_min_input_current(struct chg_alg_device *alg,
	enum chg_idx chgidx, u32 *uA);

extern int wlc_hal_vbat_mon_en(struct chg_alg_device *alg,
	enum chg_idx chgidx, bool en);
extern int wlc_hal_is_charger_enable(struct chg_alg_device *alg,
	enum chg_idx chgidx, bool *en);
extern int wlc_hal_get_log_level(struct chg_alg_device *alg);

extern int wlc_hal_get_batt_temp(struct chg_alg_device *alg);
extern int wlc_hal_get_batt_cv(struct chg_alg_device *alg);
extern int moto_wireless_chg_ops_register(struct moto_wls_chg_ops *ops);
#endif /* __MTK_wlc_H */
