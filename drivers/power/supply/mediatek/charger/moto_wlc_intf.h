/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 Motorola Inc.
*/

#ifndef __MTK_WLC_INTF_H__
#define __MTK_WLC_INTF_H__

#define WLC_V_CHARGER_MIN 4600000 /* 4.6 V */
/*wireless input current and charging current*/
#define WIRELESS_CHARGER_MAX_CURRENT			3000000
#define WIRELESS_CHARGER_MAX_INPUT_CURRENT		1150000
#define DISABLE_VBAT_THRESHOLD -1
#define ECABLEOUT	1	/* cable out */
#define EHAL		2	/* hal operation error */

struct moto_wls_chg_ops {
	void *data;
	void (*wls_current_select)(int  *icl, int *vbus, bool *cable_ready);
	void (*wls_set_battery_soc)(int uisoc);
};

/* pe 2.0*/
struct wlc_profile {
	unsigned int vbat;
	unsigned int vchr;
};

struct mtk_wlc {
	struct mutex access_lock;
	struct mutex pmic_sync_lock;
	struct wakeup_source *suspend_lock;
	int ta_vchr_org;
	int idx;
	int vbus;
	bool to_check_chr_type;
	bool is_cable_out_occur; /* Plug out happened while detect PE+20 */
	bool is_connect;
	bool is_enabled;
	struct wlc_profile profile[10];

	int vbat_orig; /* Measured VBAT before cable impedance measurement */
	int aicr_cable_imp; /* AICR to set after cable impedance measurement */
	int wls_control_en;
	int min_charger_voltage;
	int wireless_charger_max_current;
	int wireless_charger_max_input_current;
	int input_current_limit;
	int input_current_limit1;
	int input_current_limit2;
	int charging_current_limit1;
	int charging_current_limit2;
	/* current IC setting */
	int input_current1;
	int charging_current1;
	int input_current2;
	int charging_current2;

	int mmi_fcc;

	/*wireless thermal*/
	struct thermal_cooling_device *tcd;
	int max_state;
	int cur_state;
	bool cable_ready;
};

#ifdef CONFIG_MOTO_WLC_ALG_SUPPORT

extern  int mtk_wlc_init(struct charger_manager *pinfo,struct device *dev);
extern int mtk_wlc_reset_ta_vchr(struct charger_manager *pinfo);
extern int mtk_wlc_check_charger(struct charger_manager *pinfo);
extern int mtk_wlc_start_algorithm(struct charger_manager *pinfo);
extern int mtk_wlc_set_charging_current(struct charger_manager *pinfo,
					 unsigned int *ichg,
					 unsigned int *aicr);

extern void mtk_wlc_set_to_check_chr_type(struct charger_manager *pinfo,
					bool check);
extern void mtk_wlc_set_is_enable(struct charger_manager *pinfo, bool enable);
extern void mtk_wlc_set_is_cable_out_occur(struct charger_manager *pinfo,
					bool out);

extern bool mtk_wlc_get_to_check_chr_type(struct charger_manager *pinfo);
extern bool mtk_wlc_get_is_connect(struct charger_manager *pinfo);
extern bool mtk_wlc_get_is_enable(struct charger_manager *pinfo);
extern int moto_wireless_chg_ops_register(struct moto_wls_chg_ops *ops);
extern int mtk_wlc_remove(struct charger_manager *pinfo);
extern bool mtk_wlc_check_charger_avail(void);
extern void moto_wlc_control_gpio(struct charger_manager *pinfo, bool enable);
#else /* NOT CONFIG_MTK_PUMP_EXPRESS_PLUS_20_SUPPORT */

static inline int mtk_wlc_init(struct charger_manager *pinfo,struct device *dev)

{
	return -ENOTSUPP;
}

static inline int mtk_wlc_reset_ta_vchr(struct charger_manager *pinfo)
{
	return -ENOTSUPP;
}

static inline int mtk_wlc_check_charger(struct charger_manager *pinfo)
{
	return -ENOTSUPP;
}

static inline int mtk_wlc_start_algorithm(struct charger_manager *pinfo)
{
	return -ENOTSUPP;
}

static inline int mtk_wlc_set_charging_current(struct charger_manager *pinfo,
						unsigned int *ichg,
						unsigned int *aicr)
{
	return -ENOTSUPP;
}

static inline void mtk_wlc_set_to_check_chr_type(struct charger_manager *pinfo,
						  bool check)
{
}

static inline void mtk_wlc_set_is_enable(struct charger_manager *pinfo,
					   bool enable)
{
}

static inline
void mtk_wlc_set_is_cable_out_occur(struct charger_manager *pinfo, bool out)
{
}

static inline bool mtk_wlc_get_to_check_chr_type(struct charger_manager *pinfo)
{
	return false;
}

static inline bool mtk_wlc_get_is_connect(struct charger_manager *pinfo)
{
	return false;
}

static inline bool mtk_wlc_get_is_enable(struct charger_manager *pinfo)
{
	return false;
}
static inline int moto_wireless_chg_ops_register(struct moto_wls_chg_ops *ops){
	return false;
}
static inline int mtk_wlc_remove(struct charger_manager *pinfo){
	return false;
}
static inline bool mtk_wlc_check_charger_avail(void){
	return false;
}
static inline void moto_wlc_control_gpio(struct charger_manager *pinfo, bool enable){
	return ;
}
#endif /* CONFIG_MOTO_WLC_ALG_SUPPORT */

#endif /* __MTK_WLC_INTF_H__ */
