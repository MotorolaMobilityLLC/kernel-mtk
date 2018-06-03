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

#ifndef __MTK_BATTERY_INTF_H__
#define __MTK_BATTERY_INTF_H__

/* ============================================================ */
/* Define Macro Value */
/* ============================================================ */
#define FGD_NL_MSG_T_HDR_LEN 20
#define FGD_NL_MSG_MAX_LEN 9200

#define UNIT_TRANS_10	10

#define UNIT_TRANS_100	100
#define UNIT_TRANS_1000	1000
#define UNIT_TRANS_60 60

/* ============================================================ */
/* power misc related */
/* ============================================================ */
#define BAT_VOLTAGE_LOW_BOUND 3400
#define BAT_VOLTAGE_HIGH_BOUND 3450
#define SHUTDOWN_TIME 40
#define AVGVBAT_ARRAY_SIZE 30
#define INIT_VOLTAGE 3450
#define BATTERY_SHUTDOWN_TEMPERATURE 60

/* ============================================================ */
/* typedef and Struct*/
/* ============================================================ */
#define BMLOG_ERROR_LEVEL   3
#define BMLOG_WARNING_LEVEL 4
#define BMLOG_NOTICE_LEVEL  5
#define BMLOG_INFO_LEVEL    6
#define BMLOG_DEBUG_LEVEL   7
#define BMLOG_TRACE_LEVEL   8

#define bm_err(fmt, args...)   \
		do {									\
			if (bat_get_debug_level() >= BMLOG_ERROR_LEVEL) {			\
				pr_notice(fmt, ##args); \
			}								   \
		} while (0)

#define bm_warn(fmt, args...)   \
		do {									\
			if (bat_get_debug_level() >= BMLOG_WARNING_LEVEL) {	\
				pr_notice(fmt, ##args); \
			}								   \
		} while (0)

#define bm_notice(fmt, args...)   \
		do {									\
			if (bat_get_debug_level() >= BMLOG_NOTICE_LEVEL) {			\
				pr_notice(fmt, ##args); \
			}								   \
		} while (0)

#define bm_info(fmt, args...)   \
		do {									\
			if (bat_get_debug_level() >= BMLOG_INFO_LEVEL) {		\
				pr_notice(fmt, ##args); \
			}								   \
		} while (0)

#define bm_debug(fmt, args...)   \
		do {									\
			if (bat_get_debug_level() >= BMLOG_DEBUG_LEVEL) {		\
				pr_notice(fmt, ##args); \
			}								   \
		} while (0)

#define bm_trace(fmt, args...)\
		do {									\
			if (bat_get_debug_level() >= BMLOG_TRACE_LEVEL) {			\
				pr_notice(fmt, ##args);\
	}						\
} while (0)

#define BM_DAEMON_DEFAULT_LOG_LEVEL 3



typedef enum {
	FG_DAEMON_CMD_PRINT_LOG,
	FG_DAEMON_CMD_GET_CUSTOM_SETTING,
	FG_DAEMON_CMD_IS_BAT_EXIST,
	FG_DAEMON_CMD_GET_INIT_FLAG,
	FG_DAEMON_CMD_SET_INIT_FLAG,
	FG_DAEMON_CMD_SET_DAEMON_PID,
	FG_DAEMON_CMD_NOTIFY_DAEMON,
	FG_DAEMON_CMD_CHECK_FG_DAEMON_VERSION,
	FG_DAEMON_CMD_FGADC_RESET,
	FG_DAEMON_CMD_GET_TEMPERTURE,
	FG_DAEMON_CMD_GET_RAC,
	FG_DAEMON_CMD_GET_PTIM_VBAT,
	FG_DAEMON_CMD_GET_PTIM_I,
	FG_DAEMON_CMD_IS_CHARGER_EXIST,
	FG_DAEMON_CMD_GET_HW_OCV,
	FG_DAEMON_CMD_GET_FG_HW_CAR,
	FG_DAEMON_CMD_SET_FG_BAT_INT1_GAP,
	FG_DAEMON_CMD_SET_FG_BAT_TMP_GAP,
	FG_DAEMON_CMD_SET_FG_BAT_INT2_HT_GAP,
	FG_DAEMON_CMD_ENABLE_FG_BAT_INT2_HT,
	FG_DAEMON_CMD_SET_FG_BAT_INT2_LT_GAP,
	FG_DAEMON_CMD_ENABLE_FG_BAT_INT2_LT,
	FG_DAEMON_CMD_IS_BAT_PLUGOUT,
	FG_DAEMON_CMD_IS_BAT_CHARGING,
	FG_DAEMON_CMD_GET_CHARGER_STATUS,
	FG_DAEMON_CMD_SET_SW_OCV,
	FG_DAEMON_CMD_GET_SHUTDOWN_DURATION_TIME,
	FG_DAEMON_CMD_GET_BAT_PLUG_OUT_TIME,
	FG_DAEMON_CMD_GET_IS_FG_INITIALIZED,
	FG_DAEMON_CMD_SET_IS_FG_INITIALIZED,
	FG_DAEMON_CMD_SET_FG_RESET_RTC_STATUS,
	FG_DAEMON_CMD_IS_HWOCV_UNRELIABLE,
	FG_DAEMON_CMD_GET_FG_CURRENT_AVG,
	FG_DAEMON_CMD_SET_FG_TIME,
	FG_DAEMON_CMD_GET_FG_TIME,
	FG_DAEMON_CMD_GET_ZCV,
	FG_DAEMON_CMD_GET_FG_SW_CAR_NAFG_CNT,
	FG_DAEMON_CMD_GET_FG_SW_CAR_NAFG_DLTV,
	FG_DAEMON_CMD_GET_FG_SW_CAR_NAFG_C_DLTV,
	FG_DAEMON_CMD_SET_NAG_ZCV,
	FG_DAEMON_CMD_SET_NAG_ZCV_EN,
	FG_DAEMON_CMD_SET_NAG_C_DLTV,
	FG_DAEMON_CMD_SET_ZCV_INTR,
	FG_DAEMON_CMD_SET_FG_QUSE,/*remove*/
	FG_DAEMON_CMD_SET_FG_RESISTANCE,/*remove*/
	FG_DAEMON_CMD_SET_FG_DC_RATIO,
	FG_DAEMON_CMD_SET_BATTERY_CYCLE_THRESHOLD,
	FG_DAEMON_CMD_SOFF_RESET,
	FG_DAEMON_CMD_NCAR_RESET,
	FG_DAEMON_CMD_GET_IMIX,
	FG_DAEMON_CMD_GET_AGING_FACTOR_CUST,
	FG_DAEMON_CMD_GET_D0_C_SOC_CUST,
	FG_DAEMON_CMD_GET_D0_V_SOC_CUST,
	FG_DAEMON_CMD_GET_UISOC_CUST,
	FG_DAEMON_CMD_IS_KPOC,
	FG_DAEMON_CMD_GET_NAFG_VBAT,
	FG_DAEMON_CMD_GET_HW_INFO,
	FG_DAEMON_CMD_SET_KERNEL_SOC,
	FG_DAEMON_CMD_SET_KERNEL_UISOC,
	FG_DAEMON_CMD_SET_KERNEL_INIT_VBAT,
	FG_DAEMON_CMD_SET_BAT_PLUGOUT_INTR,
	FG_DAEMON_CMD_SET_IAVG_INTR,
	FG_DAEMON_CMD_SET_FG_SHUTDOWN_COND,
	FG_DAEMON_CMD_GET_FG_SHUTDOWN_COND,
	FG_DAEMON_CMD_ENABLE_FG_VBAT_L_INT,
	FG_DAEMON_CMD_ENABLE_FG_VBAT_H_INT,
	FG_DAEMON_CMD_SET_FG_VBAT_L_TH,
	FG_DAEMON_CMD_SET_FG_VBAT_H_TH,
	FG_DAEMON_CMD_SET_CAR_TUNE_VALUE,
	FG_DAEMON_CMD_GET_FG_CURRENT_IAVG_VALID,
	FG_DAEMON_CMD_GET_RTC_UI_SOC,
	FG_DAEMON_CMD_SET_RTC_UI_SOC,
	FG_DAEMON_CMD_SET_FG_BAT_TMP_C_GAP,
	FG_DAEMON_CMD_IS_BATTERY_CYCLE_RESET,
	FG_DAEMON_CMD_GET_RTC_TWO_SEC_REBOOT,
	FG_DAEMON_CMD_GET_RTC_INVALID,
	FG_DAEMON_CMD_GET_VBAT,
	FG_DAEMON_CMD_GET_DISABLE_NAFG,
	FG_DAEMON_CMD_DUMP_LOG,


	FG_DAEMON_CMD_FROM_USER_NUMBER
} FG_DAEMON_CTRL_CMD_FROM_USER;

/* kernel cmd */
typedef enum {
	FG_KERNEL_CMD_NO_ACTION,
	FG_KERNEL_CMD_DUMP_REGULAR_LOG,
	FG_KERNEL_CMD_DISABLE_NAFG,
	FG_KERNEL_CMD_DUMP_LOG,

	FG_KERNEL_CMD_FROM_USER_NUMBER

} FG_KERNEL_CTRL_CMD;

typedef enum {
	FG_INTR_0 = 0,
	FG_INTR_TIMER_UPDATE  = 1,
	FG_INTR_BAT_CYCLE = 2,
	FG_INTR_CHARGER_OUT = 4,
	FG_INTR_CHARGER_IN = 8,
	FG_INTR_FG_TIME =		16,
	FG_INTR_BAT_INT1_HT =	32,
	FG_INTR_BAT_INT1_LT =	64,
	FG_INTR_BAT_INT2_HT =	128,
	FG_INTR_BAT_INT2_LT =	256,
	FG_INTR_BAT_TMP_HT =	512,
	FG_INTR_BAT_TMP_LT =	1024,
	FG_INTR_BAT_TIME_INT =	2048,
	FG_INTR_NAG_C_DLTV =	4096,
	FG_INTR_FG_ZCV = 8192,
	FG_INTR_SHUTDOWN = 16384,
	FG_INTR_RESET_NVRAM = 32768,
	FG_INTR_BAT_PLUGOUT = 65536,
	FG_INTR_IAVG = 0x20000,
	FG_INTR_VBAT2_L = 0x40000,
	FG_INTR_VBAT2_H = 0x80000,
	FG_INTR_CHR_FULL = 0x100000,
	FG_INTR_DLPT_SD = 0x200000,
	FG_INTR_BAT_TMP_C_HT = 0x400000,
	FG_INTR_BAT_TMP_C_LT = 0x800000,
	FG_INTR_BAT_INT1_CHECK = 0x1000000,
	FG_INTR_KERNEL_CMD = 0x2000000,

} FG_INTERRUPT_FLAG;

struct fgd_nl_msg_t {
	unsigned int fgd_cmd;
	unsigned int fgd_subcmd;
	unsigned int fgd_subcmd_para1;
	unsigned int fgd_data_len;
	unsigned int fgd_ret_data_len;
	char fgd_data[FGD_NL_MSG_MAX_LEN];
};

struct fuel_gauge_custom_data {

	int versionID1;
	int versionID2;
	int versionID3;

	/* Qmax for battery  */
	int q_max_L_current;
	int q_max_H_current;
	int q_max_t0;
	int q_max_t1;
	int q_max_t2;
	int q_max_t3;
	int q_max_t4;
	int q_max_t0_h_current;
	int q_max_t1_h_current;
	int q_max_t2_h_current;
	int q_max_t3_h_current;
	int q_max_t4_h_current;
	int q_max_sys_voltage;

	int temperature_t0;
	int temperature_t1;
	int temperature_t2;
	int temperature_t3;
	int temperature_t4;
	int temperature_tb0;
	int temperature_tb1;

	int pseudo1_t0;
	int pseudo1_t1;
	int pseudo1_t2;
	int pseudo1_t3;
	int pseudo1_t4;
	int pseudo100_t0;
	int pseudo100_t1;
	int pseudo100_t2;
	int pseudo100_t3;
	int pseudo100_t4;
	int pseudo1_en;
	int pseudo100_en;
	int pseudo100_en_dis;
	int pseudo1_iq_offset;

	/* vboot related */
	int qmax_sel;
	int iboot_sel;
	int poweron_system_iboot;
	int shutdown_system_iboot;
	int pmic_min_vol;

	/* hw related */
	int car_tune_value;
	int fg_meter_resistance;
	int r_fg_value;
	int mtk_chr_exist;

	/* Aging Compensation 1*/
	int aging_one_en;
	int aging1_update_soc;
	int aging1_load_soc;
	int aging_temp_diff;
	int aging_100_en;
	int difference_voltage_update;

	/* Aging Compensation 2*/
	int aging_two_en;

	/* Aging Compensation 3*/
	int aging_third_en;


	/* ui_soc */
	int diff_soc_setting;
	int keep_100_percent;
	int difference_full_cv;
	int diff_bat_temp_setting;
	int diff_bat_temp_setting_c;
	int discharge_tracking_time;
	int charge_tracking_time;
	int difference_fullocv_vth;
	int difference_fullocv_ith;
	int charge_pseudo_full_level;


	/* threshold */
	int hwocv_swocv_diff;	/* 0.1 mv */
	int hwocv_swocv_diff_lt;	/* 0.1 mv */
	int hwocv_swocv_diff_lt_temp;	/* degree */
	int hwocv_oldocv_diff;	/* 0.1 mv */
	int swocv_oldocv_diff;	/* 0.1 mv */
	int vbat_oldocv_diff;	/* 0.1 mv */
	int tnew_told_pon_diff;	/* degree */
	int tnew_told_pon_diff2;/* degree */
	int pmic_shutdown_time;	/* sec */
	int bat_plug_out_time;	/* min */
	int swocv_oldocv_diff_emb;	/* 0.1 mv */

	/* fgc & fgv threshold */
	int difference_fgc_fgv_th1;
	int difference_fgc_fgv_th2;
	int difference_fgc_fgv_th3;
	int difference_fgc_fgv_th_soc1;
	int difference_fgc_fgv_th_soc2;
	int nafg_time_setting;
	int nafg_ratio;
	int nafg_ratio_en;
	int nafg_resistance;

	/* mode select */
	int pmic_shutdown_current;
	int pmic_shutdown_sw_en;
	int force_vc_mode;
	int embedded_sel;
	int loading_1_en;
	int loading_2_en;
	int diff_iavg_th;

	/* ADC resister */
	int r_bat_sense;	/*is it used?*/
	int r_i_sense;		/*is it used?*/
	int r_charger_1;
	int r_charger_2;

	/* pre_tracking */
	int fg_pre_tracking_en;
	int vbat2_det_time;
	int vbat2_det_counter;
	int vbat2_det_voltage1;
	int vbat2_det_voltage2;
	int vbat2_det_voltage3;

	int shutdown_1_time;
	int shutdown_gauge0;
	int shutdown_gauge1_xmins;
	int shutdown_gauge1_mins;
	int shutdown_gauge0_voltage;

	/* ZCV update */
	int zcv_suspend_time;
	int sleep_current_avg;

	int dc_ratio_sel;
	int dc_r_cnt;

	/* shutdown_hl_zcv */
	int shutdown_hl_zcv_t0;
	int shutdown_hl_zcv_t1;
	int shutdown_hl_zcv_t2;
	int shutdown_hl_zcv_t3;
	int shutdown_hl_zcv_t4;

	int pseudo1_sel;

	/* Additional battery table */
	int additional_battery_table_en;

	int d0_sel;
	int aging_sel;
	int fg_tracking_current;
	int fg_tracking_current_iboot_en;
	int ui_fast_tracking_en;
	int ui_fast_tracking_gap;
	int bat_par_i;

	int aging_factor_min;
	int aging_factor_diff;
	int keep_100_percent_minsoc;
	int battery_tmp_to_disable_gm30;
	int battery_tmp_to_disable_nafg;
	int battery_tmp_to_enable_nafg;
	int disable_nafg;

	int zcv_car_gap_percentage;

#if 0
/*======old setting ======*/
	/* cust_battery_meter.h */
	int soc_flow;

	int hw_fg_force_use_sw_ocv;

	int oam_d5; /* 1 : D5,   0: D2 */

	int change_tracking_point;
	int cust_tracking_point;
	int cust_r_sense;
	int cust_hw_cc;
	int aging_tuning_value;
	int cust_r_fg_offset;
	int ocv_board_compesate;
	int r_fg_board_base;
	int r_fg_board_slope;

	/* HW Fuel gague  */
	int current_detect_r_fg;
	int minerroroffset;
	int fg_vbat_average_size;

	int difference_hwocv_rtc;
	int difference_hwocv_swocv;
	int difference_swocv_rtc;
	int max_swocv;

	int max_hwocv;
	int max_vbat;
	int difference_hwocv_vbat;

	int suspend_current_threshold;
	int ocv_check_time;
	int shutdown_system_voltage;
	int recharge_tolerance;
	int fixed_tbat_25;

	int batterypseudo100;
	int batterypseudo1;

	/* Dynamic change wake up period of battery thread when suspend*/
	int vbat_normal_wakeup;
	int vbat_low_power_wakeup;
	int normal_wakeup_period;
	int low_power_wakeup_period;
	int close_poweroff_wakeup_period;

	int init_soc_by_sw_soc;
	int sync_ui_soc_imm;                  /*3. ui soc sync to fg soc immediately*/
	int mtk_enable_aging_algorithm; /*6. q_max aging algorithm*/
	int md_sleep_current_check;     /*5. gauge adjust by ocv 9. md sleep current check*/
	int q_max_by_current;           /*7. qmax variant by current loading.*/
	/*int q_max_sys_voltage;*/		/*8. qmax variant by sys voltage.*/

	int min_charging_smooth_time;

	/* SW Fuel gauge */
	int apsleep_battery_voltage_compensate;
#endif
};

struct hw_info_data {
	int current_1;
	int current_2;
	int current_avg;
	int current_avg_sign;
	int car;
	int ncar;
	int time;
};

struct fuel_gauge_table_custom_data {
	/* cust_battery_meter_table.h */
	int fg_profile_t0_size;
	FUELGAUGE_PROFILE_STRUCT fg_profile_t0[100];
	int fg_profile_t1_size;
	FUELGAUGE_PROFILE_STRUCT fg_profile_t1[100];
	int fg_profile_t2_size;
	FUELGAUGE_PROFILE_STRUCT fg_profile_t2[100];
	int fg_profile_t3_size;
	FUELGAUGE_PROFILE_STRUCT fg_profile_t3[100];
	int fg_profile_t4_size;
	FUELGAUGE_PROFILE_STRUCT fg_profile_t4[100];
	int fg_profile_temperature_0_size;
	FUELGAUGE_PROFILE_STRUCT fg_profile_temperature_0[100];
	int fg_profile_temperature_1_size;
	FUELGAUGE_PROFILE_STRUCT fg_profile_temperature_1[100];
};

typedef struct {
	bool bat_exist;
	bool bat_full;
	signed int bat_charging_state;
	unsigned int bat_vol;
	bool bat_in_recharging_state;
	unsigned int Vsense;
	bool charger_exist;
	unsigned int charger_vol;
	signed int charger_protect_status;
	signed int ICharging;
	signed int IBattery;
	signed int temperature;
	signed int temperatureR;
	signed int temperatureV;
	unsigned int total_charging_time;
	unsigned int PRE_charging_time;
	unsigned int CC_charging_time;
	unsigned int TOPOFF_charging_time;
	unsigned int POSTFULL_charging_time;
	unsigned int charger_type;
	signed int SOC;
	signed int UI_SOC;
	signed int UI_SOC2;
	unsigned int nPercent_ZCV;
	unsigned int nPrecent_UI_SOC_check_point;
	unsigned int ZCV;
} PMU_ChargerStruct;		/* BMT_status */

struct battery_data {
	struct power_supply_desc psd;
	struct power_supply *psy;
	int BAT_STATUS;
	int BAT_HEALTH;
	int BAT_PRESENT;
	int BAT_TECHNOLOGY;
	int BAT_CAPACITY;
	/* Add for Battery Service */
	int BAT_batt_vol;
	int BAT_batt_temp;
	/* Add for EM */
	int BAT_TemperatureR;
	int BAT_TempBattVoltage;
	int BAT_InstatVolt;
	int BAT_BatteryAverageCurrent;
	int BAT_BatterySenseVoltage;
	int BAT_ISenseVoltage;
	int BAT_ChargerVoltage;
	/* Dual battery */
	int status_smb;
	int capacity_smb;
	int present_smb;
	int adjust_power;
};

struct BAT_EC_Struct {
	int fixed_temp_en;
	int fixed_temp_value;
	int debug_rac_en;
	int debug_rac_value;
	int debug_ptim_v_en;
	int debug_ptim_v_value;
	int debug_ptim_r_en;
	int debug_ptim_r_value;
	int debug_ptim_r_value_sign;
	int debug_fg_curr_en;
	int debug_fg_curr_value;
	int debug_bat_id_en;
	int debug_bat_id_value;
	int debug_d0_c_en;
	int debug_d0_c_value;
	int debug_d0_v_en;
	int debug_d0_v_value;
	int debug_uisoc_en;
	int debug_uisoc_value;
};

/* mtk_power_misc */
enum {
	NORMAL = 0,
	OVERHEAT,
	SOC_ZERO_PERCENT,
	UISOC_ONE_PERCENT,
	LOW_BAT_VOLT,
	DLPT_SHUTDOWN,
	SHUTDOWN_FACTOR_MAX
};

extern struct fuel_gauge_custom_data fg_cust_data;
extern PMU_ChargerStruct BMT_status;
extern struct BAT_EC_Struct Bat_EC_ctrl;
extern struct gauge_hw_status FG_status;


extern int wakeup_fg_algo_cmd(unsigned int flow_state, int cmd, int para1);



/* DLPT */
extern int do_ptim_gauge(bool isSuspend, unsigned int *bat, signed int *cur, bool *is_charging);

#endif /* __MTK_BATTERY_INTF_H__ */
