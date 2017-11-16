/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef _CUST_BAT_H_
#define _CUST_BAT_H_

/* stop charging while in talking mode */
#define STOP_CHARGING_IN_TAKLING
#define TALKING_RECHARGE_VOLTAGE 3800
#define TALKING_SYNC_TIME		   60

/* Battery Temperature Protection */
#define MTK_TEMPERATURE_RECHARGE_SUPPORT
#define MAX_CHARGE_TEMPERATURE  50
#define MAX_CHARGE_TEMPERATURE_MINUS_X_DEGREE	47
#define MIN_CHARGE_TEMPERATURE  0
#define MIN_CHARGE_TEMPERATURE_PLUS_X_DEGREE	6
#define ERR_CHARGE_TEMPERATURE  0xFF

/* Linear Charging Threshold */
#define V_PRE2CC_THRES	3400	/*mV*/
#define V_CC2TOPOFF_THRES	4050
#define RECHARGING_VOLTAGE	4110
#define CHARGING_FULL_CURRENT	100	/*mA*/

/* Charging Current Setting */
/*#define CONFIG_USB_IF */
#define USB_CHARGER_CURRENT_SUSPEND			0		/* def CONFIG_USB_IF*/
#define USB_CHARGER_CURRENT_UNCONFIGURED	CHARGE_CURRENT_70_00_MA	/* 70mA*/
#define USB_CHARGER_CURRENT_CONFIGURED	CHARGE_CURRENT_500_00_MA	/* 500mA*/

#define USB_CHARGER_CURRENT	CHARGE_CURRENT_500_00_MA	/*500mA*/
/*#define AC_CHARGER_CURRENT					CHARGE_CURRENT_650_00_MA*/
#define AC_CHARGER_CURRENT	CHARGE_CURRENT_1000_00_MA
#define NON_STD_AC_CHARGER_CURRENT	CHARGE_CURRENT_500_00_MA
#define CHARGING_HOST_CHARGER_CURRENT	CHARGE_CURRENT_650_00_MA
#define APPLE_0_5A_CHARGER_CURRENT	CHARGE_CURRENT_500_00_MA
#define APPLE_1_0A_CHARGER_CURRENT	CHARGE_CURRENT_650_00_MA
#define APPLE_2_1A_CHARGER_CURRENT	CHARGE_CURRENT_800_00_MA


/* Precise Tunning */
#define BATTERY_AVERAGE_DATA_NUMBER	3
#define BATTERY_AVERAGE_SIZE	30

/* charger error check */
#define BAT_LOW_TEMP_PROTECT_ENABLE         /* stop charging if temp < MIN_CHARGE_TEMPERATURE */
#define V_CHARGER_ENABLE	0	/* 1:ON , 0:OFF	*/
#define V_CHARGER_MAX	6500	/* 6.5 V*/
#define V_CHARGER_MIN	4400	/* 4.4 V*/

/* Tracking TIME */
#define ONEHUNDRED_PERCENT_TRACKING_TIME	10	/* 10 second*/
#define NPERCENT_TRACKING_TIME	20	/* 20 second*/
#define SYNC_TO_REAL_TRACKING_TIME	60	/* 60 second*/
#define V_0PERCENT_TRACKING	3450	/*3450mV*/

/* Battery Notify */
#define BATTERY_NOTIFY_CASE_0001_VCHARGER
#define BATTERY_NOTIFY_CASE_0002_VBATTEMP
/*#define BATTERY_NOTIFY_CASE_0003_ICHARGING*/
/*#define BATTERY_NOTIFY_CASE_0004_VBAT*/
/*#define BATTERY_NOTIFY_CASE_0005_TOTAL_CHARGINGTIME*/

/* JEITA parameter */
/*#define MTK_JEITA_STANDARD_SUPPORT*/
#define CUST_SOC_JEITA_SYNC_TIME	30
#define JEITA_RECHARGE_VOLTAGE	4110	/* for linear charging*/
#define JEITA_TEMP_ABOVE_POS_60_CV_VOLTAGE	BATTERY_VOLT_04_100000_V
#define JEITA_TEMP_POS_45_TO_POS_60_CV_VOLTAGE	BATTERY_VOLT_04_100000_V
#define JEITA_TEMP_POS_10_TO_POS_45_CV_VOLTAGE	BATTERY_VOLT_04_200000_V
#define JEITA_TEMP_POS_0_TO_POS_10_CV_VOLTAGE	BATTERY_VOLT_04_100000_V
#define JEITA_TEMP_NEG_10_TO_POS_0_CV_VOLTAGE	BATTERY_VOLT_03_900000_V
#define JEITA_TEMP_BELOW_NEG_10_CV_VOLTAGE	BATTERY_VOLT_03_900000_V

/* For JEITA Linear Charging only */
#define JEITA_NEG_10_TO_POS_0_FULL_CURRENT	120	/*mA */
#define JEITA_TEMP_POS_45_TO_POS_60_RECHARGE_VOLTAGE	4000
#define JEITA_TEMP_POS_10_TO_POS_45_RECHARGE_VOLTAGE	4100
#define JEITA_TEMP_POS_0_TO_POS_10_RECHARGE_VOLTAGE	4000
#define JEITA_TEMP_NEG_10_TO_POS_0_RECHARGE_VOLTAGE	3800
#define JEITA_TEMP_POS_45_TO_POS_60_CC2TOPOFF_THRESHOLD	4050
#define JEITA_TEMP_POS_10_TO_POS_45_CC2TOPOFF_THRESHOLD	4050
#define JEITA_TEMP_POS_0_TO_POS_10_CC2TOPOFF_THRESHOLD	4050
#define JEITA_TEMP_NEG_10_TO_POS_0_CC2TOPOFF_THRESHOLD	3850


/* High battery support */
#define HIGH_BATTERY_VOLTAGE_SUPPORT

/* Disable Battery check for HQA */
#ifdef CONFIG_MTK_DISABLE_POWER_ON_OFF_VOLTAGE_LIMITATION
#define CONFIG_DIS_CHECK_BATTERY
#endif

#ifdef CONFIG_MTK_FAN5405_SUPPORT
#define FAN5405_BUSNUM 1
#endif

/*Other_platform_modify sunxuebin.wt ADD 20161029 handle ovp resume*/
#define WT_CHR_OVP_RESUME

/*Other_platform_modify sunxuebin.wt  ADD 20161029  over temperature notify resume*/
#define WT_BAT_TEMP_NOTIFY_RESUME

/*+Other_platform_modify  sunxuebin.wt  ADD 20161029  ui_soc sync full in charging*/
/*#define WT_UI_SOC_SYNC_FULL_IN_CHARGING*/   /*delete by sunxuebin.wt for UI SOC display error bug 20161102*/
#ifdef WT_UI_SOC_SYNC_FULL_IN_CHARGING
	#define WT_UI_SOC_SYNC_FULL_ITERM 400
#endif
/*-Other_platform_modify  sunxuebin.wt  ADD 20161029  ui_soc sync full in charging*/

/* sunxuebin.wt add for charging control when temperature in higher or lower range begin */
#define WT_LOW_TEMP_AND_HIGH_TEMP_CHARGING_CONTROL
#ifdef WT_LOW_TEMP_AND_HIGH_TEMP_CHARGING_CONTROL
#define WT_LOW_TEMP 15
#define WT_HIGH_TEMP 45
#define WT_LOW_TEMP_CHARGING_CURRENT 500  /* 500 limit is 450mA */
#define WT_HIGH_TEMP_CV_VOLTAGE 4100000 /* HC40 battery need 4.100V +- 0.041V*/
#define WT_HIGH_TEMP_TOPOFF_THRESHOLD 3950 /* HC40 battery need 4.100V +- 0.041V*/
#define WT_HIGH_TEMP_RECHARGING_VOLTAGE 4000 /* HC40 battery */
#endif
/* sunxuebin.wt add for charging control when temperature in higher or lower range end */
#endif
