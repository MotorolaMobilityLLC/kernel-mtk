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

#ifndef _CUST_PMIC_H_
#define _CUST_PMIC_H_

/*#define PMIC_VDVFS_CUST_ENABLE*/

#define LOW_POWER_LIMIT_LEVEL_1 15

/*
//Define for disable low battery protect feature, default no define for enable low battery protect.
//#define DISABLE_LOW_BATTERY_PROTECT

//Define for disable battery OC protect
//#define DISABLE_BATTERY_OC_PROTECT

//Define for disable battery 15% protect
//#define DISABLE_BATTERY_PERCENT_PROTECT
*/
/*Define for DLPT*/
/*#define DISABLE_DLPT_FEATURE*/
#define POWER_UVLO_VOLT_LEVEL 2600
#define IMAX_MAX_VALUE 7000

#define POWER_INT0_VOLT 3400
#define POWER_INT1_VOLT 3250
#define POWER_INT2_VOLT 3100

#define POWER_BAT_OC_CURRENT_H    5950
#define POWER_BAT_OC_CURRENT_L    7000
#define POWER_BAT_OC_CURRENT_H_RE 5950
#define POWER_BAT_OC_CURRENT_L_RE 7000

#define DLPT_POWER_OFF_EN
#define POWEROFF_BAT_CURRENT 3000
#define DLPT_POWER_OFF_THD 100

#define BATTERY_MODULE_INIT

/* ADC Channel Number */
typedef enum {
	AUX_BATSNS_AP =	0x000,
	AUX_ISENSE_AP,
	AUX_VCDT_AP,
	AUX_BATON_AP,
	AUX_TSENSE_AP,
	AUX_TSENSE_MD =	0x005,
	AUX_VACCDET_AP = 0x007,
	AUX_VISMPS_AP =	0x00B,
	AUX_ICLASSAB_AP =	0x016,
	AUX_HP_AP =	0x017,
	AUX_CH10_AP =	0x018,
	AUX_VBIF_AP =	0x019,
	AUX_CH0_6311 = 0x020,
	AUX_CH1_6311 = 0x021,
	AUX_ADCVIN0_MD = 0x10F,
	AUX_ADCVIN0_GPS = 0x20C,
	AUX_CH12 = 0x1011,
	AUX_CH13 = 0x2011,
	AUX_CH14 = 0x3011,
	AUX_CH15 = 0x4011,
} upmu_adc_chl_list_enum;

#endif /* _CUST_PMIC_H_ */
