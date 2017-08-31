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

#ifndef __MTK_CHARGER_INIT_H__
#define __MTK_CHARGER_INIT_H__

#define BATTERY_CV 4350000

#define USB_CHARGER_CURRENT_SUSPEND			0		/* def CONFIG_USB_IF */
#define USB_CHARGER_CURRENT_UNCONFIGURED	70000	/* 70mA */
#define USB_CHARGER_CURRENT_CONFIGURED		500000	/* 500mA */
#define USB_CHARGER_CURRENT					500000	/* 500mA */
#define AC_CHARGER_CURRENT					2050000
#define AC_CHARGER_INPUT_CURRENT			3200000
#define NON_STD_AC_CHARGER_CURRENT			500000
#define CHARGING_HOST_CHARGER_CURRENT       650000
#define TA_AC_CHARGING_CURRENT	3000000

#define V_CHARGER_MAX 6500000				/* 6.5 V */

/* pe2.0 */
#define PE20_ICHG_LEAVE_THRESHOLD 1000 /* mA */
#define TA_START_BATTERY_SOC	1
#define TA_STOP_BATTERY_SOC	85

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

#define CC_NORMAL 4000	/*normal charging ibus OC threshold (mA)*/
#define CC_MAX 6500	/*PE3.0 ibus OC threshold (mA)*/
#define CC_END 2000	/*PE3.0 min charging current (mA)*/
#define CC_STEP 200	/*CC state charging current step (mA)*/
#define CC_SS_STEP 500	/*soft start state charging current step (mA)*/
#define CC_SS_STEP2 200	/*soft start state charging current step (mA),when battery voltage > CC_SS_STEP2*/
#define CC_SS_STEP3 100	/*soft start state charging current step (mA),when battery voltage > CC_SS_STEP3*/
#define CV_SS_STEP2 4000	/*battery voltage threshold for CC_SS_STEP2*/
#define CV_SS_STEP3 4200	/*battery voltage threshold for CC_SS_STEP3*/
#define CC_SS_BLANKING 100	/*polling duraction for init/soft start state(ms)*/
#define CC_BLANKING 1000	/*polling duraction for CC state*/
#define CHARGER_TEMP_MAX 70	/*max charger IC temperature*/
#define TA_TEMP_MAX 80	/*max adapter temperature*/
#define VBUS_OV_GAP 100
#define FOD_CURRENT 200 /*FOD current threshold*/
#define R_VBAT_MIN 40	/*min R_VBAT*/
#define R_SW_MIN 20	/*min R_SW*/
#define PE30_MAX_CHARGING_TIME 5400	/*PE3.0 max chargint time (sec)*/

#if defined(CONFIG_MTK_JEITA_STANDARD_SUPPORT)
#define BATTERY_TEMP_MIN TEMP_POS_10_THRESHOLD
#define BATTERY_TEMP_MAX TEMP_POS_45_THRESHOLD
#else
#define BATTERY_TEMP_MIN 0	/*PE3.0 min battery temperature*/
#define BATTERY_TEMP_MAX 50	/*PE3.0 man battery temperature*/
#endif

/* cable measurement impedance */
#define CABLE_IMP_THRESHOLD 699
#define VBAT_CABLE_IMP_THRESHOLD 3900

/* bif */
#define bif_threshold1 4250	/* mV */
#define bif_threshold2 4300	/* mV */
#define bif_cv_under_threshold2 4450	/* mV */
#define bif_cv 4352

/* sw jeita */
#define JEITA_TEMP_ABOVE_T4_CV_VOLTAGE	4240000
#define JEITA_TEMP_T3_TO_T4_CV_VOLTAGE	4240000
#define JEITA_TEMP_T2_TO_T3_CV_VOLTAGE	4340000
#define JEITA_TEMP_T1_TO_T2_CV_VOLTAGE	4240000
#define JEITA_TEMP_T0_TO_T1_CV_VOLTAGE	4040000
#define JEITA_TEMP_BELOW_T0_CV_VOLTAGE	4040000
#define TEMP_T4_THRESHOLD  50
#define TEMP_T4_THRES_MINUS_X_DEGREE 47
#define TEMP_T3_THRESHOLD  45
#define TEMP_T3_THRES_MINUS_X_DEGREE 39
#define TEMP_T2_THRESHOLD  10
#define TEMP_T2_THRES_PLUS_X_DEGREE 16
#define TEMP_T1_THRESHOLD  0
#define TEMP_T1_THRES_PLUS_X_DEGREE 6
#define TEMP_T0_THRESHOLD  0
#define TEMP_T0_THRES_PLUS_X_DEGREE  0

/* Battery Temperature Protection */
#define MTK_TEMPERATURE_RECHARGE_SUPPORT
#define MAX_CHARGE_TEMPERATURE  50
#define MAX_CHARGE_TEMPERATURE_MINUS_X_DEGREE	47
#define MIN_CHARGE_TEMPERATURE  0
#define MIN_CHARGE_TEMPERATURE_PLUS_X_DEGREE	6

#define TEMP_NEG_10_THRESHOLD 0

/* battery warning */
#define BATTERY_NOTIFY_CASE_0001_VCHARGER
#define BATTERY_NOTIFY_CASE_0002_VBATTEMP

#endif /*__MTK_CHARGER_INIT_H__*/
