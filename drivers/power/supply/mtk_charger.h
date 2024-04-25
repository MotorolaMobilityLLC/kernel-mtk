/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __MTK_CHARGER_H
#define __MTK_CHARGER_H

#include <linux/alarmtimer.h>
#include "charger_class.h"
#include "adapter_class.h"
#include "mtk_charger_algorithm_class.h"
#include <linux/power_supply.h>
#include "mtk_smartcharging.h"
#include <linux/power/moto_chg_tcmd.h>

#define CHARGING_INTERVAL 10
#define CHARGING_FULL_INTERVAL 20

#define CHRLOG_ERROR_LEVEL	1
#define CHRLOG_INFO_LEVEL	2
#define CHRLOG_DEBUG_LEVEL	3

#ifdef CONFIG_MOTO_CHANNEL_SWITCH
#define VBUS_VAC1_ONLINE_VOLTAGE	4400
#endif

#define SC_TAG "smartcharging"

extern int chr_get_debug_level(void);

#define chr_err(fmt, args...)					\
do {								\
	if (chr_get_debug_level() >= CHRLOG_ERROR_LEVEL) {	\
		pr_notice(fmt, ##args);				\
	}							\
} while (0)

#define chr_info(fmt, args...)					\
do {								\
	if (chr_get_debug_level() >= CHRLOG_INFO_LEVEL) {	\
		pr_notice_ratelimited(fmt, ##args);		\
	}							\
} while (0)

#define chr_debug(fmt, args...)					\
do {								\
	if (chr_get_debug_level() >= CHRLOG_DEBUG_LEVEL) {	\
		pr_notice(fmt, ##args);				\
	}							\
} while (0)

struct mtk_charger;
struct charger_data;
#define BATTERY_CV 4350000
#define V_CHARGER_MAX 6500000 /* 6.5 V */
#define V_CHARGER_MIN 4600000 /* 4.6 V */

#define USB_CHARGER_CURRENT_SUSPEND		0 /* def CONFIG_USB_IF */
#define USB_CHARGER_CURRENT_UNCONFIGURED	70000 /* 70mA */
#define USB_CHARGER_CURRENT_CONFIGURED		500000 /* 500mA */
#define USB_CHARGER_CURRENT			500000 /* 500mA */
#define AC_CHARGER_CURRENT			2050000
#define AC_CHARGER_INPUT_CURRENT		3200000
#define NON_STD_AC_CHARGER_CURRENT		500000
#define CHARGING_HOST_CHARGER_CURRENT		650000
/*wireless input current and charging current*/
#define WIRELESS_FACTORY_MAX_CURRENT			3000000
#define WIRELESS_FACTORY_MAX_INPUT_CURRENT		600000

/*wireless charging power*/
#define WLS_RX_CAP_15W 15
#define WLS_RX_CAP_10W 10
#define WLS_RX_CAP_8W 8
#define WLS_RX_CAP_5W 5

/* dynamic mivr */
#define V_CHARGER_MIN_1 4400000 /* 4.4 V */
#define V_CHARGER_MIN_2 4200000 /* 4.2 V */
#define MAX_DMIVR_CHARGER_CURRENT 1800000 /* 1.8 A */

/* battery warning */
#define BATTERY_NOTIFY_CASE_0001_VCHARGER
#define BATTERY_NOTIFY_CASE_0002_VBATTEMP

/* charging abnormal status */
#define CHG_VBUS_OV_STATUS	(1 << 0)
#define CHG_BAT_OT_STATUS	(1 << 1)
#define CHG_OC_STATUS		(1 << 2)
#define CHG_BAT_OV_STATUS	(1 << 3)
#define CHG_ST_TMO_STATUS	(1 << 4)
#define CHG_BAT_LT_STATUS	(1 << 5)
#define CHG_TYPEC_WD_STATUS	(1 << 6)

/* Battery Temperature Protection */
#define MIN_CHARGE_TEMP  0
#define MIN_CHARGE_TEMP_PLUS_X_DEGREE	6
#define MAX_CHARGE_TEMP  50
#define MAX_CHARGE_TEMP_MINUS_X_DEGREE	47

#define MAX_ALG_NO 10
#define DEFAULT_ALG 0

enum mmi_mux_channel {
	MMI_MUX_CHANNEL_NONE = 0,
	MMI_MUX_CHANNEL_TYPEC_CHG,
	MMI_MUX_CHANNEL_TYPEC_OTG,
	MMI_MUX_CHANNEL_WLC_CHG,
	MMI_MUX_CHANNEL_WLC_OTG,
	MMI_MUX_CHANNEL_TYPEC_CHG_WLC_OTG,
	MMI_MUX_CHANNEL_TYPEC_CHG_WLC_CHG,
	MMI_MUX_CHANNEL_TYPEC_OTG_WLC_CHG,
	MMI_MUX_CHANNEL_TYPEC_OTG_WLC_OTG,
	MMI_MUX_CHANNEL_WLC_FW_UPDATE,
	MMI_MUX_CHANNEL_WLC_FACTORY_TEST,
#ifdef CONFIG_MOTO_CHANNEL_SWITCH
	MMI_MUX_CHANNEL_WLC_CHG_OTG,
	MMI_MUX_CHANNEL_WLC_CHG_OTG_WLC_OTG,
	MMI_MUX_CHANNEL_WLC_CHG_OTG_WLC_CHG,
#endif
	MMI_MUX_CHANNEL_MAX
};

struct mmi_mux_chan {
	enum mmi_mux_channel chan;
	bool on;
};

struct mmi_mux_configure {
	u32 typec_mos;
	u32 wls_mos;
	bool wls_boost_en;
	bool wls_loadswtich_en;
	bool wls_chip_en;
};

enum bat_temp_state_enum {
	BAT_TEMP_LOW = 0,
	BAT_TEMP_NORMAL,
	BAT_TEMP_HIGH
};

enum chg_dev_notifier_events {
	EVENT_FULL,
	EVENT_RECHARGE,
	EVENT_DISCHARGE,
};

struct battery_thermal_protection_data {
	int sm;
	bool enable_min_charge_temp;
	int min_charge_temp;
	int min_charge_temp_plus_x_degree;
	int max_charge_temp;
	int max_charge_temp_minus_x_degree;
};

/* sw jeita */
#define JEITA_TEMP_ABOVE_T4_CV	4240000
#define JEITA_TEMP_T3_TO_T4_CV	4240000
#define JEITA_TEMP_T2_TO_T3_CV	4340000
#define JEITA_TEMP_T1_TO_T2_CV	4240000
#define JEITA_TEMP_T0_TO_T1_CV	4040000
#define JEITA_TEMP_BELOW_T0_CV	4040000
#define TEMP_T4_THRES  50
#define TEMP_T4_THRES_MINUS_X_DEGREE 47
#define TEMP_T3_THRES  45
#define TEMP_T3_THRES_MINUS_X_DEGREE 39
#define TEMP_T2_THRES  10
#define TEMP_T2_THRES_PLUS_X_DEGREE 16
#define TEMP_T1_THRES  0
#define TEMP_T1_THRES_PLUS_X_DEGREE 6
#define TEMP_T0_THRES  0
#define TEMP_T0_THRES_PLUS_X_DEGREE  0
#define TEMP_NEG_10_THRES 0

/*
 * Software JEITA
 * T0: -10 degree Celsius
 * T1: 0 degree Celsius
 * T2: 10 degree Celsius
 * T3: 45 degree Celsius
 * T4: 50 degree Celsius
 */
enum sw_jeita_state_enum {
	TEMP_BELOW_T0 = 0,
	TEMP_T0_TO_T1,
	TEMP_T1_TO_T2,
	TEMP_T2_TO_T3,
	TEMP_T3_TO_T4,
	TEMP_ABOVE_T4
};

struct sw_jeita_data {
	int sm;
	int pre_sm;
	int cv;
	bool charging;
	bool error_recovery_flag;
};

struct mtk_charger_algorithm {
	int (*do_mux)(struct mtk_charger *info, enum mmi_mux_channel channel, bool on);
	int (*do_algorithm)(struct mtk_charger *info);
	int (*enable_charging)(struct mtk_charger *info, bool en);
	int (*do_event)(struct notifier_block *nb, unsigned long ev, void *v);
	int (*do_dvchg1_event)(struct notifier_block *nb, unsigned long ev,
			       void *v);
	int (*do_dvchg2_event)(struct notifier_block *nb, unsigned long ev,
			       void *v);
	int (*change_current_setting)(struct mtk_charger *info);
	void *algo_data;
};

struct charger_custom_data {
	int battery_cv;	/* uv */
	int max_charger_voltage;
	int max_charger_voltage_setting;
	int min_charger_voltage;

	int usb_charger_current;
	int ac_charger_current;
	int ac_charger_input_current;
	int charging_host_charger_current;

	/* sw jeita */
	int jeita_temp_above_t4_cv;
	int jeita_temp_t3_to_t4_cv;
	int jeita_temp_t2_to_t3_cv;
	int jeita_temp_t1_to_t2_cv;
	int jeita_temp_t0_to_t1_cv;
	int jeita_temp_below_t0_cv;
	int temp_t4_thres;
	int temp_t4_thres_minus_x_degree;
	int temp_t3_thres;
	int temp_t3_thres_minus_x_degree;
	int temp_t2_thres;
	int temp_t2_thres_plus_x_degree;
	int temp_t1_thres;
	int temp_t1_thres_plus_x_degree;
	int temp_t0_thres;
	int temp_t0_thres_plus_x_degree;
	int temp_neg_10_thres;

	/* battery temperature protection */
	int mtk_temperature_recharge_support;
	int max_charge_temp;
	int max_charge_temp_minus_x_degree;
	int min_charge_temp;
	int min_charge_temp_plus_x_degree;

	/* dynamic mivr */
	int min_charger_voltage_1;
	int min_charger_voltage_2;
	int max_dmivr_charger_current;

	/*wireless charger*/
	int wireless_factory_max_current;
	int wireless_factory_max_input_current;
};

struct charger_data {
	int input_current_limit;
	int charging_current_limit;

	int force_charging_current;
	int thermal_input_current_limit;
	int thermal_charging_current_limit;
	bool thermal_throttle_record;
	int disable_charging_count;
	int input_current_limit_by_aicl;
	int junction_temp_min;
	int junction_temp_max;
	int moto_chg_tcmd_ichg;
	int moto_chg_tcmd_ibat;
#if defined(CONFIG_MOTO_DISCRETE_CHARGE_PUMP_SUPPORT) || defined(CONFIG_MOTO_CHARGER_SGM415XX)
	int cp_ichg_limit;
#endif
};

/*moto mmi Functionality start*/
struct mmi_ffc_zone  {
	int		temp;
	int		ffc_max_mv;
	int		ffc_chg_iterm;
};

struct mmi_cycle_cv_steps {
	int		cycle;
	int		delta_cv_mv;
};

struct mmi_temp_zone {
	int		temp_c;
	int		norm_mv;
	int		fcc_max_ma;
	int		fcc_norm_ma;
};

#define MAX_NUM_STEPS 10
enum mmi_temp_zones {
	ZONE_FIRST = 0,
	/* states 0-9 are reserved for zones */
	ZONE_LAST = MAX_NUM_STEPS + ZONE_FIRST - 1,
	ZONE_HOT,
	ZONE_COLD,
	ZONE_NONE = 0xFF,
};

enum mmi_chrg_step {
	STEP_MAX,
	STEP_NORM,
	STEP_FULL,
	STEP_FLOAT,
	STEP_DEMO,
	STEP_STOP,
	STEP_NONE = 0xFF,
};

enum charging_limit_modes {
	CHARGING_LIMIT_OFF,
	CHARGING_LIMIT_RUN,
	CHARGING_LIMIT_UNKNOWN,
};

#ifdef CONFIG_MOTO_CHG_FFC_5V10W_SUPPORT
typedef enum {
	CHARGER_FFC_STATE_INITIAL,
	CHARGER_FFC_STATE_PROBING,
	CHARGER_FFC_STATE_STANDBY,
	CHARGER_FFC_STATE_GOREADY,
	CHARGER_FFC_STATE_RUNNING,
	CHARGER_FFC_STATE_AVGEXIT,
	CHARGER_FFC_STATE_FFCDONE,
	CHARGER_FFC_STATE_INVALID,
} CHARGER_FFC_STATE_T;

enum {
	CHARGER_FLAG_INVALID,
	CHARGER_FLAG_FFC,
	CHARGER_FLAG_NON_FFC,
};
#endif

struct ntc_temp {
	signed int Temp;
	signed int TemperatureR;
};

struct mmi_params {
	bool			init_done;
	bool			factory_mode;
	int			demo_mode;
	bool			demo_discharging;

	bool			factory_kill_armed;

	/*adaptive charging*/
	bool adaptive_charging_disable_ichg;
	bool adaptive_charging_disable_ibat;
	bool charging_enable_hz;
	bool battery_charging_disable;

	/* Charge Profile */
	int			num_temp_zones;
	struct mmi_temp_zone	*temp_zones;
	enum mmi_temp_zones	pres_temp_zone;
	enum mmi_chrg_step	pres_chrg_step;
	int			chrg_taper_cnt;
	int			temp_state;
	int			chrg_iterm;
	int			back_chrg_iterm;

	int			num_ffc_zones;
	struct mmi_ffc_zone	*ffc_zones;
	int			num_cycle_cv_steps;
	struct mmi_cycle_cv_steps	*cycle_cv_steps;

#ifdef CONFIG_MOTO_CHG_FFC_5V10W_SUPPORT
	CHARGER_FFC_STATE_T	ffc_state;
	int			ffc_entry_threshold;
	int			ffc_exit_threshold;
	int			ffc_uisoc_threshold;
	long		ffc_ibat_windowsum;
	long		ffc_ibat_count;
	int			ffc_ibat_windowsize;
	int			ffc_iavg;
	unsigned long ffc_iavg_update_timestamp;
#endif

	bool			enable_charging_limit;
	bool			is_factory_image;
	enum charging_limit_modes	charging_limit_modes;
	int			upper_limit_capacity;
	int			lower_limit_capacity;
	int			base_fv_mv;
	int			vfloat_comp_mv;
	int			batt_health;
	int			batt_statues;
	int			max_chrg_temp;

	/*target parameter*/
	int			target_fv;
	bool			chg_disable;
	int			target_fcc;
	int			target_usb;
	struct notifier_block	chg_reboot;
	int			min_therm_current_limit;
	bool			enable_mux;
	struct			mmi_mux_chan mux_channel;
	int			wls_switch_en;
	int			wls_boost_en;
	int			charge_rate;
	unsigned int	active_fast_alg;
	int			typec_rp_max_current;

	int			pd_pmax_mw;
	struct adapter_auth_data	apdo_cap;
	int			pd_cap_max_watt;
	int			vbus_h;
	int			vbus_l;
	int			charger_watt;

	int			typec_ntc_pull_up_r;
	struct ntc_temp 		*typec_ntc_table;
	int			num_typec_ntc_table;
};
/*moto mmi Functionality end*/

enum chg_data_idx_enum {
	CHG1_SETTING,
	CHG2_SETTING,
	DVCHG1_SETTING,
	DVCHG2_SETTING,
	CHGS_SETTING_MAX,
};

struct mtk_charger {
	struct platform_device *pdev;
	struct charger_device *chg1_dev;
	struct notifier_block chg1_nb;
	struct charger_device *chg2_dev;
	struct charger_device *dvchg1_dev;
	struct notifier_block dvchg1_nb;
	struct charger_device *dvchg2_dev;
	struct notifier_block dvchg2_nb;

	struct charger_data chg_data[CHGS_SETTING_MAX];
	struct chg_limit_setting setting;
	enum charger_configuration config;

	struct power_supply_desc psy_desc1;
	struct power_supply_config psy_cfg1;
	struct power_supply *psy1;

	struct power_supply_desc psy_desc2;
	struct power_supply_config psy_cfg2;
	struct power_supply *psy2;

	struct power_supply_desc psy_dvchg_desc1;
	struct power_supply_config psy_dvchg_cfg1;
	struct power_supply *psy_dvchg1;

	struct power_supply_desc psy_dvchg_desc2;
	struct power_supply_config psy_dvchg_cfg2;
	struct power_supply *psy_dvchg2;

	struct power_supply  *chg_psy;
	struct power_supply  *wl_psy;
	struct power_supply  *bat_psy;
	struct adapter_device *pd_adapter;
	struct notifier_block pd_nb;
	struct mutex pd_lock;
	int pd_type;
	bool pd_reset;

	u32 bootmode;
	u32 boottype;

	int chr_type;
	int usb_type;
	int usb_state;

	struct mutex cable_out_lock;
	int cable_out_cnt;

	/* system lock */
	spinlock_t slock;
	struct wakeup_source *charger_wakelock;
	struct mutex charger_lock;

	/* thread related */
	wait_queue_head_t  wait_que;
	bool charger_thread_timeout;
	unsigned int polling_interval;
	bool charger_thread_polling;

	/* alarm timer */
	struct alarm charger_timer;
	struct timespec64 endtime;
	bool is_suspend;
	struct notifier_block pm_notifier;
	ktime_t timer_cb_duration[8];

	/* notify charger user */
	struct srcu_notifier_head evt_nh;

	/* common info */
	int log_level;
	bool usb_unlimited;
	bool charger_unlimited;
	bool disable_charger;
	bool disable_aicl;
	int battery_temp;
	bool can_charging;
	bool cmd_discharging;
	bool safety_timeout;
	int safety_timer_cmd;
	bool vbusov_stat;
	bool is_chg_done;
	/* ATM */
	bool atm_enabled;

	const char *algorithm_name;
	struct mtk_charger_algorithm algo;

	/* dtsi custom data */
	struct charger_custom_data data;

	/* battery warning */
	unsigned int notify_code;
	unsigned int notify_test_mode;

	/* sw safety timer */
	bool enable_sw_safety_timer;
	bool sw_safety_timer_setting;
	struct timespec64 charging_begin_time;

	/* vbat monitor, 6pin bat */
	bool batpro_done;
	bool enable_vbat_mon;
	bool enable_vbat_mon_bak;
	int old_cv;
	bool stop_6pin_re_en;
	int vbat0_flag;

	/* sw jeita */
	bool enable_sw_jeita;
	struct sw_jeita_data sw_jeita;

	/* battery thermal protection */
	struct battery_thermal_protection_data thermal;

	struct chg_alg_device *alg[MAX_ALG_NO];
	struct notifier_block chg_alg_nb;
	bool enable_hv_charging;

	/* water detection */
	bool water_detected;
	bool record_water_detected;

	bool enable_dynamic_mivr;

	/* fast charging algo support indicator */
	bool enable_fast_charging_indicator;
	unsigned int fast_charging_indicator;

	/* diasable meta current limit for testing */
	unsigned int enable_meta_current_limit;

	struct smartcharging sc;

#ifdef CONFIG_MOTO_CHG_FFC_5V10W_SUPPORT
	/* 10w ffc */
	bool ffc_discharging;
	int ffc_input_current_backup;
	int ffc_max_fv_mv_backup;
	struct wakeup_source *ffc_charger_wakelock;
	struct delayed_work ffc_enable_charge_work;
#endif

	/*daemon related*/
	struct sock *daemo_nl_sk;
	u_int g_scd_pid;
	struct scd_cmd_param_t_1 sc_data;

	/*charger IC charging status*/
	bool is_charging;

	ktime_t uevent_time_check;

	bool force_disable_pp[CHG2_SETTING + 1];
	bool enable_pp[CHG2_SETTING + 1];
	struct mutex pp_lock[CHG2_SETTING + 1];
	struct moto_chg_tcmd_client chg_tcmd_client;
	struct mmi_params	mmi;
	int wireless_online;
	struct mutex mmi_mux_lock;

	struct tcpc_device *tcpc_dev;
	bool typecotp_charger;
	bool typec_otp_sts;
	bool dcp_otp_sts;
	bool pdc_otp_sts;
	/*typec connecter ntc thermal*/
	bool typecotp_use_thermal_cooling;
	struct thermal_cooling_device *tcd;
	int typec_otp_max_state;
	int typec_otp_cur_state;

	/*Battery info*/
	unsigned long		manufacturing_date;
	unsigned long		first_usage_date;

};

static inline int mtk_chg_alg_notify_call(struct mtk_charger *info,
					  enum chg_alg_notifier_events evt,
					  int value)
{
	int i;
	struct chg_alg_notify notify = {
		.evt = evt,
		.value = value,
	};

	for (i = 0; i < MAX_ALG_NO; i++) {
		if (info->alg[i])
			chg_alg_notifier_call(info->alg[i], &notify);
	}
	return 0;
}

/* functions which framework needs*/
extern int mtk_basic_charger_init(struct mtk_charger *info);
extern int mtk_pulse_charger_init(struct mtk_charger *info);
extern int get_uisoc(struct mtk_charger *info);
extern int get_battery_voltage(struct mtk_charger *info);
extern int get_battery_temperature(struct mtk_charger *info);
extern int get_battery_current(struct mtk_charger *info);
extern int get_vbus(struct mtk_charger *info);
extern int get_ibat(struct mtk_charger *info);
extern int get_ibus(struct mtk_charger *info);
extern bool is_battery_exist(struct mtk_charger *info);
extern int get_charger_type(struct mtk_charger *info);
extern int get_usb_type(struct mtk_charger *info);
extern int disable_hw_ovp(struct mtk_charger *info, int en);
extern bool is_charger_exist(struct mtk_charger *info);
extern int get_charger_temperature(struct mtk_charger *info,
	struct charger_device *chg);
extern int get_charger_charging_current(struct mtk_charger *info,
	struct charger_device *chg);
extern int get_charger_input_current(struct mtk_charger *info,
	struct charger_device *chg);
extern int get_charger_zcv(struct mtk_charger *info,
	struct charger_device *chg);
extern void _wake_up_charger(struct mtk_charger *info);

/* functions for other */
extern int mtk_chg_enable_vbus_ovp(bool enable);
extern void aee_kernel_RT_Monitor_api_factory(void);

#endif /* __MTK_CHARGER_H */
