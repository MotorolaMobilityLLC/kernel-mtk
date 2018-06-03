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

#ifndef _MTK_BATTERY_PROPERTY_H
#define _MTK_BATTERY_PROPERTY_H

/* customize */
#define DIFFERENCE_FULLOCV_ITH	150	/* mA */
#define MTK_CHR_EXIST 1
#define SHUTDOWN_1_TIME	60
#define KEEP_100_PERCENT 2
#define R_FG_VALUE	10				/* mOhm */
#define EMBEDDED_SEL 0
#define PMIC_SHUTDOWN_CURRENT 20	/* 0.01 mA */
#define FG_METER_RESISTANCE	50
#define CAR_TUNE_VALUE	100 /*1.00 */

#define SHUTDOWN_GAUGE0 1
#define SHUTDOWN_GAUGE1_XMINS 1
#define SHUTDOWN_GAUGE0_VOLTAGE 35000

#define POWERON_SYSTEM_IBOOT 1000	/* mA */

#define D0_SEL 0	/* not implement */
#define AGING_SEL 0	/* not implement */

/* ADC resistor  */
#define R_BAT_SENSE	4
#define R_I_SENSE	4
#define R_CHARGER_1	330
#define R_CHARGER_2	39


#define QMAX_SEL 1
#define IBOOT_SEL 0
#define SHUTDOWN_SYSTEM_IBOOT 15000	/* 0.1mA */
#define PMIC_MIN_VOL 32000

/*ui_soc related */
#define DIFFERENCE_FULL_CV 1000 /*0.01%*/
#define PSEUDO1_EN 1
#define PSEUDO100_EN 1
#define PSEUDO100_EN_DIS 1

#define DIFF_SOC_SETTING 50	/* 0.01% */
#define DIFF_BAT_TEMP_SETTING 1
#define DIFF_BAT_TEMP_SETTING_C 10
#define DISCHARGE_TRACKING_TIME 10
#define CHARGE_TRACKING_TIME 60
#define DIFFERENCE_FULLOCV_VTH	1000	/* 0.1mV */
#define CHARGE_PSEUDO_FULL_LEVEL 9000

/* pre tracking */
#define FG_PRE_TRACKING_EN 0
#define VBAT2_DET_TIME 5
#define VBAT2_DET_COUNTER 6
#define VBAT2_DET_VOLTAGE1	34500
#define VBAT2_DET_VOLTAGE2	32000
#define VBAT2_DET_VOLTAGE3	35000

/* PCB setting */
#define CALIBRATE_CAR_TUNE_VALUE_BY_META_TOOL
#define CALI_CAR_TUNE_AVG_NUM	60

/* Aging Compensation 1*/
#define AGING_FACTOR_MIN 75
#define AGING_FACTOR_DIFF 10
#define DIFFERENCE_VOLTAGE_UPDATE 50
#define AGING_ONE_EN 1
#define AGING1_UPDATE_SOC 30
#define AGING1_LOAD_SOC 70
#define AGING_TEMP_DIFF 10
#define AGING_100_EN 1

/* Aging Compensation 2*/
#define AGING_TWO_EN 1

/* Aging Compensation 3*/
#define AGING_THIRD_EN 1

/* threshold */
#define HWOCV_SWOCV_DIFF	300
#define HWOCV_OLDOCV_DIFF	300
#define SWOCV_OLDOCV_DIFF	300
#define VBAT_OLDOCV_DIFF	1000
#define SWOCV_OLDOCV_DIFF_EMB	1000

#define TNEW_TOLD_PON_DIFF	5
#define TNEW_TOLD_PON_DIFF2	15
#define PMIC_SHUTDOWN_TIME	30
#define BAT_PLUG_OUT_TIME	32
#define EXT_HWOCV_SWOCV		300

/* fgc & fgv threshold */
#define DIFFERENCE_FGC_FGV_TH1 500
#define DIFFERENCE_FGC_FGV_TH2 1500
#define DIFFERENCE_FGC_FGV_TH3 500
#define DIFFERENCE_FGC_FGV_TH_SOC1 7000
#define DIFFERENCE_FGC_FGV_TH_SOC2 3000
#define NAFG_TIME_SETTING 10
#define NAFG_RATIO 80
#define NAFG_RATIO_EN 0
#define NAFG_RESISTANCE 1500

#define PMIC_SHUTDOWN_SW_EN 1
#define FORCE_VC_MODE 0	/* 0: mix, 1:Coulomb, 2:voltage */

#define LOADING_1_EN 0
#define LOADING_2_EN 2
#define DIFF_IAVG_TH 3000

/* ZCV INTR */
#define ZCV_SUSPEND_TIME 6
#define SLEEP_CURRENT_AVG 100 /*0.1mA*/

/* Additional battery table */
#define ADDITIONAL_BATTERY_TABLE_EN 0

#define DC_RATIO_SEL	5
#define DC_R_CNT	10	/* if set 0, dcr_start will not be 1*/

#define BAT_PAR_I 4000	/* not implement */

#define PSEUDO1_SEL	2

#define FG_TRACKING_CURRENT	10000	/* not implement */
#define FG_TRACKING_CURRENT_IBOOT_EN	1	/* not implement */
#define UI_FAST_TRACKING_EN 0
#define UI_FAST_TRACKING_GAP 300
#define KEEP_100_PERCENT_MINSOC 9000

#define UNIT_FGCURRENT     (381470)		/* mt6335 381.470 uA */
#define UNIT_FGCAR         (108506)		/* unit 2^11 LSB*/
#define R_VAL_TEMP_2         (2)			/* MT6335 use 3, old chip use 4 */
#define R_VAL_TEMP_3         (3)			/* MT6335 use 3, old chip use 4 */
#define UNIT_TIME          (50)
#define UNIT_FGCAR_ZCV     (190735)     /* unit 2^0 LSB */
#define UNIT_FG_IAVG		(190735)
#define CAR_TO_REG_FACTOR  (0x49BA)

/*#define SHUTDOWN_CONDITION_LOW_BAT_VOLT*/

/* extern function */
extern int get_rac(void);
extern int get_imix(void);
extern void get_ptim(unsigned int*, signed int*);
extern int do_ptim(bool);


#endif
