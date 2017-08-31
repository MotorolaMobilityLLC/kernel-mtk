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

/* pe20 */
#define PE20_ICHG_LEAVE_THRESHOLD 1000 /* mA */
#define TA_START_BATTERY_SOC	1
#define TA_STOP_BATTERY_SOC	85

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
