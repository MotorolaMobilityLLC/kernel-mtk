#ifndef _BATTERY_METER_H
#define _BATTERY_METER_H

#include <linux/platform_device.h>
#include <mt-plat/mt_typedefs.h>
#include <mach/mt_battery_meter.h>
/* ============================================================ */
/* define */
/* ============================================================ */
#define FG_CURRENT_AVERAGE_SIZE 30

/* ============================================================ */
/* ENUM */
/* ============================================================ */

/* ============================================================ */
/* structure */
/* ============================================================ */

#define FGD_NL_MSG_T_HDR_LEN 12
#define FGD_NL_MSG_MAX_LEN 9200

struct fgd_nl_msg_t {
	unsigned int fgd_cmd;
	unsigned int fgd_data_len;
	unsigned int fgd_ret_data_len;
	char fgd_data[FGD_NL_MSG_MAX_LEN];
};

enum {
	FG_MAIN = 1,
	FG_SUSPEND = 2,
	FG_RESUME = 4,
	FG_CHARGER = 8
};

enum {
	HW_FG,
	SW_FG,
	AUXADC
};


/* ============================================================ */
/* typedef */
/* ============================================================ */
typedef struct {
	INT32 BatteryTemp;
	INT32 TemperatureR;
} BATT_TEMPERATURE;

#if !defined(CONFIG_MTK_HAFG_20)
struct battery_meter_custom_data {

	/* mt_battery_meter.h */

	/* ADC resister */
	int r_bat_sense;
	int r_i_sense;
	int r_charger_1;
	int r_charger_2;

	int temperature_t0;
	int temperature_t1;
	int temperature_t2;
	int temperature_t3;
	int temperature_t;

	int fg_meter_resistance;

	/* Qmax for battery  */
	int q_max_pos_50;
	int q_max_pos_25;
	int q_max_pos_0;
	int q_max_neg_10;
	int q_max_pos_50_h_current;
	int q_max_pos_25_h_current;
	int q_max_pos_0_h_current;
	int q_max_neg_10_h_current;

	int oam_d5;		/* 1 : D5,   0: D2 */

	int change_tracking_point;
	int cust_tracking_point;
	int cust_r_sense;
	int cust_hw_cc;
	int aging_tuning_value;
	int cust_r_fg_offset;
	int ocv_board_compesate;
	int r_fg_board_base;
	int r_fg_board_slope;
	int car_tune_value;

	/* HW Fuel gague  */
	int current_detect_r_fg;
	int minerroroffset;
	int fg_vbat_average_size;
	int r_fg_value;
	int cust_poweron_delta_capacity_tolrance;
	int cust_poweron_low_capacity_tolrance;
	int cust_poweron_max_vbat_tolrance;
	int cust_poweron_delta_vbat_tolrance;
	int cust_poweron_delta_hw_sw_ocv_capacity_tolrance;

	int fixed_tbat_25;

	/* Dynamic change wake up period of battery thread when suspend */
	int vbat_normal_wakeup;
	int vbat_low_power_wakeup;
	int normal_wakeup_period;
	int low_power_wakeup_period;
	int close_poweroff_wakeup_period;


	/* mt_battery_meter.h */
	int bat_ntc;
	int rbat_pull_up_r;
	int rbat_pull_up_volt;

};
#endif

struct battery_custom_data {
	/* mt_charging.h */
	/* stop charging while in talking mode */
	int stop_charging_in_takling;
	int talking_recharge_voltage;
	int talking_sync_time;

	/* Battery Temperature Protection */
	int mtk_temperature_recharge_support;
	int max_charge_temperature;
	int max_charge_temperature_minus_x_degree;
	int min_charge_temperature;
	int min_charge_temperature_plus_x_degree;
	int err_charge_temperature;

	/* Linear Charging Threshold */
	int v_pre2cc_thres;
	int v_cc2topoff_thres;
	int recharging_voltage;
	int charging_full_current;


	/* Charging Current Setting */
	int config_usb_if;
	int usb_charger_current_suspend;
	int usb_charger_current_unconfigured;
	int usb_charger_current_configured;
	int usb_charger_current;
	int ac_charger_current;
	int non_std_ac_charger_current;
	int charging_host_charger_current;
	int apple_0_5a_charger_current;
	int apple_1_0a_charger_current;
	int apple_2_1a_charger_current;

	/* Precise Tunning
	   int battery_average_data_number;
	   int battery_average_size;
	 */

	/* charger error check */
	int bat_low_temp_protect_enable;
	int v_charger_enable;
	int v_charger_max;
	int v_charger_min;

	/* Tracking TIME */
	int onehundred_percent_tracking_time;
	int npercent_tracking_time;
	int sync_to_real_tracking_time;
	int v_0percent_tracking;

	/* Battery Notify
	   int battery_notify_case_0001_vcharger;
	   int battery_notify_case_0002_vbattemp;
	   int battery_notify_case_0003_icharging;
	   int battery_notify_case_0004_vbat;
	   int battery_notify_case_0005_total_chargingtime;
	 */

	/* High battery support */
	int high_battery_voltage_support;

	/* JEITA parameter
	   int mtk_jeita_standard_support;
	   int cust_soc_jeita_sync_time;
	   int jeita_recharge_voltage;
	   int jeita_temp_above_pos_60_cv_voltage;
	   int jeita_temp_pos_45_to_pos_60_cv_voltage;
	   int jeita_temp_pos_10_to_pos_45_cv_voltage;
	   int jeita_temp_pos_0_to_pos_10_cv_voltage;
	   int jeita_temp_neg_10_to_pos_0_cv_voltage;
	   int jeita_temp_below_neg_10_cv_voltage;
	 */

	/* For JEITA Linear Charging only
	   int jeita_neg_10_to_pos_0_full_current;
	   int jeita_temp_pos_45_to_pos_60_recharge_voltage;
	   int jeita_temp_pos_10_to_pos_45_recharge_voltage;
	   int jeita_temp_pos_0_to_pos_10_recharge_voltage;
	   int jeita_temp_neg_10_to_pos_0_recharge_voltage;
	   int jeita_temp_pos_45_to_pos_60_cc2topoff_threshold;
	   int jeita_temp_pos_10_to_pos_45_cc2topoff_threshold;
	   int jeita_temp_pos_0_to_pos_10_cc2topoff_threshold;
	   int jeita_temp_neg_10_to_pos_0_cc2topoff_threshold;
	 */

	/* cust_pe.h
	   int mtk_pump_express_plus_support;
	   int ta_start_battery_soc;
	   int ta_stop_battery_soc;
	   int ta_ac_9v_input_current;
	   int ta_ac_7v_input_current;
	   int ta_ac_charging_current;
	   int ta_9v_support;
	 */
};



typedef enum {
	FG_DAEMON_CMD_GET_INIT_FLAG,
	FG_DAEMON_CMD_GET_SOC,
	FG_DAEMON_CMD_GET_DOD0,
	FG_DAEMON_CMD_GET_DOD1,
	FG_DAEMON_CMD_GET_HW_OCV,
	FG_DAEMON_CMD_GET_HW_FG_INIT_CURRENT,
	FG_DAEMON_CMD_GET_HW_FG_CURRENT,
	FG_DAEMON_CMD_GET_HW_FG_INIT_CURRENT_SIGN,
	FG_DAEMON_CMD_GET_HW_FG_CURRENT_SIGN,
	FG_DAEMON_CMD_GET_HW_FG_CAR_ACT,
	FG_DAEMON_CMD_GET_TEMPERTURE,
	FG_DAEMON_CMD_DUMP_REGISTER,
	FG_DAEMON_CMD_CHARGING_ENABLE,
	FG_DAEMON_CMD_GET_BATTERY_INIT_VOLTAGE,
	FG_DAEMON_CMD_GET_BATTERY_VOLTAGE,
	FG_DAEMON_CMD_FGADC_RESET,
	FG_DAEMON_CMD_GET_BATTERY_PLUG_STATUS,
	FG_DAEMON_CMD_GET_RTC_SPARE_FG_VALUE,
	FG_DAEMON_CMD_IS_CHARGER_EXIST,
	FG_DAEMON_CMD_IS_BATTERY_FULL,	/* bat_is_battery_full, */
	FG_DAEMON_CMD_SET_BATTERY_FULL,	/* bat_set_battery_full, */
	FG_DAEMON_CMD_SET_RTC,	/* set RTC, */
	FG_DAEMON_CMD_SET_POWEROFF,	/* set Poweroff, */
	FG_DAEMON_CMD_IS_KPOC,	/* is KPOC, */
	FG_DAEMON_CMD_GET_BOOT_REASON,	/* g_boot_reason, */
	FG_DAEMON_CMD_GET_CHARGING_CURRENT,
	FG_DAEMON_CMD_GET_CHARGER_VOLTAGE,
	FG_DAEMON_CMD_GET_SHUTDOWN_COND,
	FG_DAEMON_CMD_GET_CUSTOM_SETTING,
	FG_DAEMON_CMD_GET_UI_SOC,
	FG_DAEMON_CMD_GET_CV_VALUE,
	FG_DAEMON_CMD_GET_DURATION_TIME,
	FG_DAEMON_CMD_GET_TRACKING_TIME,
	FG_DAEMON_CMD_GET_CURRENT_TH,
	FG_DAEMON_CMD_GET_CHECK_TIME,
	FG_DAEMON_CMD_GET_DIFFERENCE_VOLTAGE_UPDATE,
	FG_DAEMON_CMD_GET_AGING1_LOAD_SOC,
	FG_DAEMON_CMD_GET_AGING1_UPDATE_SOC,
	FG_DAEMON_CMD_GET_SHUTDOWN_SYSTEM_VOLTAGE,
	FG_DAEMON_CMD_GET_CHARGE_TRACKING_TIME,
	FG_DAEMON_CMD_GET_DISCHARGE_TRACKING_TIME,
	FG_DAEMON_CMD_GET_SHUTDOWN_GAUGE0,
	FG_DAEMON_CMD_GET_SHUTDOWN_GAUGE1_XMINS,
	FG_DAEMON_CMD_GET_SHUTDOWN_GAUGE1_MINS,
	FG_DAEMON_CMD_SET_SUSPEND_TIME,
	FG_DAEMON_CMD_SET_WAKEUP_SMOOTH_TIME,
	FG_DAEMON_CMD_SET_IS_CHARGING,
	FG_DAEMON_CMD_SET_RBAT,
	FG_DAEMON_CMD_SET_SWOCV,
	FG_DAEMON_CMD_SET_DOD0,
	FG_DAEMON_CMD_SET_DOD1,
	FG_DAEMON_CMD_SET_QMAX,
	FG_DAEMON_CMD_SET_SOC,
	FG_DAEMON_CMD_SET_UI_SOC,
	FG_DAEMON_CMD_SET_UI_SOC2,
	FG_DAEMON_CMD_SET_INIT_FLAG,
	FG_DAEMON_CMD_SET_DAEMON_PID,
	FG_DAEMON_CMD_NOTIFY_DAEMON,

	FG_DAEMON_CMD_FROM_USER_NUMBER
} FG_DAEMON_CTRL_CMD_FROM_USER;


/* ============================================================ */
/* External Variables */
/* ============================================================ */

#if !defined(CONFIG_MTK_HAFG_20)
extern struct battery_custom_data batt_cust_data;
extern struct battery_meter_custom_data batt_meter_cust_data;
#else
extern struct battery_custom_data batt_cust_data;
extern struct battery_meter_custom_data batt_meter_cust_data;
extern struct battery_meter_table_custom_data batt_meter_table_cust_data;
#endif

#ifdef MTK_ENABLE_AGING_ALGORITHM
extern U32 suspend_time;
#endif
extern BOOL bat_spm_timeout;
extern U32 _g_bat_sleep_total_time;


/* ============================================================ */
/* External function */
/* ============================================================ */
extern kal_int32 battery_meter_get_battery_voltage(kal_bool update);
extern kal_int32 battery_meter_get_charging_current_imm(void);
extern kal_int32 battery_meter_get_charging_current(void);
extern kal_int32 battery_meter_get_battery_current(void);
extern kal_bool battery_meter_get_battery_current_sign(void);
extern kal_int32 battery_meter_get_car(void);
extern kal_int32 battery_meter_get_battery_temperature(void);
extern kal_int32 battery_meter_get_charger_voltage(void);
extern kal_int32 battery_meter_get_battery_percentage(void);
extern kal_int32 battery_meter_initial(void);
extern kal_int32 battery_meter_reset(void);
extern kal_int32 battery_meter_sync(kal_int32 bat_i_sense_offset);

extern kal_int32 battery_meter_get_battery_zcv(void);
extern kal_int32 battery_meter_get_battery_nPercent_zcv(void);	/* 15% zcv,  15% can be customized */
extern kal_int32 battery_meter_get_battery_nPercent_UI_SOC(void);	/* tracking point */

extern kal_int32 battery_meter_get_tempR(kal_int32 dwVolt);
extern kal_int32 battery_meter_get_tempV(void);
extern kal_int32 battery_meter_get_VSense(void);/* isense voltage */
extern int wakeup_fg_algo(int flow_state);

extern int batt_meter_init_cust_data(struct platform_device *dev);
#if defined(CUST_CAPACITY_OCV2CV_TRANSFORM)
extern void battery_meter_set_reset_soc(kal_bool bUSE_UI_SOC);
extern kal_int32 battery_meter_get_battery_soc(void);
#endif

#ifdef MTK_MULTI_BAT_PROFILE_SUPPORT
extern int IMM_GetOneChannelValue_Cali(int Channel, int *voltage);
#ifdef MTK_GET_BATTERY_ID_BY_AUXADC
extern kal_uint32 upmu_get_reg_value(kal_uint32 reg);
extern int IMM_GetOneChannelValue(int dwChannel, int data[4], int *rawdata);
extern int IMM_IsAdcInitReady(void);
extern U32 pmic_config_interface(U32 RegNum, U32 val, U32 MASK, U32 SHIFT);
extern U32 pmic_read_interface(U32 RegNum, U32 *val, U32 MASK, U32 SHIFT);
extern unsigned int get_pmic_mt6325_cid(void);
#endif
#endif

#endif /* #ifndef _BATTERY_METER_H */
