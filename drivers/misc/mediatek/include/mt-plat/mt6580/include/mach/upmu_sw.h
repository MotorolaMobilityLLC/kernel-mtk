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

#ifndef _MT_PMIC_UPMU_SW_H_
#define _MT_PMIC_UPMU_SW_H_

#define BATTERY_DTS_SUPPORT
/* ============================================================================== */
/* Low battery level define */
/* ============================================================================== */
typedef enum LOW_BATTERY_LEVEL_TAG {
	LOW_BATTERY_LEVEL_0 = 0,
	LOW_BATTERY_LEVEL_1 = 1,
	LOW_BATTERY_LEVEL_2 = 2
} LOW_BATTERY_LEVEL;

typedef enum LOW_BATTERY_PRIO_TAG {
	LOW_BATTERY_PRIO_CPU_B = 0,
	LOW_BATTERY_PRIO_CPU_L = 1,
	LOW_BATTERY_PRIO_GPU = 2,
	LOW_BATTERY_PRIO_MD = 3,
	LOW_BATTERY_PRIO_MD5 = 4,
	LOW_BATTERY_PRIO_FLASHLIGHT = 5,
	LOW_BATTERY_PRIO_VIDEO = 6,
	LOW_BATTERY_PRIO_WIFI = 7,
	LOW_BATTERY_PRIO_BACKLIGHT = 8
} LOW_BATTERY_PRIO;

extern void (*low_battery_callback)(LOW_BATTERY_LEVEL);
extern void register_low_battery_notify(void (*low_battery_callback) (LOW_BATTERY_LEVEL),
					LOW_BATTERY_PRIO prio_val);


/* ============================================================================== */
/* Battery OC level define */
/* ============================================================================== */
typedef enum BATTERY_OC_LEVEL_TAG {
	BATTERY_OC_LEVEL_0 = 0,
	BATTERY_OC_LEVEL_1 = 1
} BATTERY_OC_LEVEL;

typedef enum BATTERY_OC_PRIO_TAG {
	BATTERY_OC_PRIO_CPU_B = 0,
	BATTERY_OC_PRIO_CPU_L = 1,
	BATTERY_OC_PRIO_GPU = 2
} BATTERY_OC_PRIO;

extern void (*battery_oc_callback)(BATTERY_OC_LEVEL);
extern void register_battery_oc_notify(void (*battery_oc_callback) (BATTERY_OC_LEVEL),
				       BATTERY_OC_PRIO prio_val);

/* ============================================================================== */
/* Battery percent define */
/* ============================================================================== */
typedef enum BATTERY_PERCENT_LEVEL_TAG {
	BATTERY_PERCENT_LEVEL_0 = 0,
	BATTERY_PERCENT_LEVEL_1 = 1
} BATTERY_PERCENT_LEVEL;

typedef enum BATTERY_PERCENT_PRIO_TAG {
	BATTERY_PERCENT_PRIO_CPU_B = 0,
	BATTERY_PERCENT_PRIO_CPU_L = 1,
	BATTERY_PERCENT_PRIO_GPU = 2,
	BATTERY_PERCENT_PRIO_MD = 3,
	BATTERY_PERCENT_PRIO_MD5 = 4,
	BATTERY_PERCENT_PRIO_FLASHLIGHT = 5,
	BATTERY_PERCENT_PRIO_VIDEO = 6,
	BATTERY_PERCENT_PRIO_WIFI = 7,
	BATTERY_PERCENT_PRIO_BACKLIGHT = 8
} BATTERY_PERCENT_PRIO;

extern void (*battery_percent_callback)(BATTERY_PERCENT_LEVEL);
extern void
register_battery_percent_notify(void (*battery_percent_callback) (BATTERY_PERCENT_LEVEL),
				BATTERY_PERCENT_PRIO prio_val);
/*
0 : BATON2 **
1 : CH6
2 : THR SENSE2 **
3 : THR SENSE1
4 : VCDT
5 : BATON1
6 : ISENSE
7 : BATSNS
8 : ACCDET
9-16 : audio
*/

/* ADC Channel Number */
typedef enum {
	/* MT6350 */

	MT6350_AUX_BATON2 = 0x000,
	MT6350_AUX_CH6,
	MT6350_AUX_THR_SENSE2,
	MT6350_AUX_THR_SENSE1,
	MT6350_AUX_VCDT,
	MT6350_AUX_BATON1,
	MT6350_AUX_ISENSE,
	MT6350_AUX_BATSNS,
	MT6350_AUX_ACCDET,
	MT6350_AUX_CH9,
	MT6350_AUX_CH10,
	MT6350_AUX_CH11,
	MT6350_AUX_CH12,
	MT6350_AUX_CH13,
	MT6350_AUX_CH14,
	MT6350_AUX_CH15,
	MT6350_AUX_CH16,
} pmic_adc_ch_list_enum;

typedef enum MT_POWER_TAG {
	MT6350_POWER_LDO_VTCXO0,
	MT6350_POWER_LDO_VTCXO1,
	MT6350_POWER_LDO_VAUD28,
	MT6350_POWER_LDO_VAUXA28,
	MT6350_POWER_LDO_VBIF28,
	MT6350_POWER_LDO_VCAMA,
	MT6350_POWER_LDO_VCN28,
	MT6350_POWER_LDO_VCN33,
	MT6350_POWER_LDO_VRF18_1,
	MT6350_POWER_LDO_VUSB33,
	MT6350_POWER_LDO_VMCH,
	MT6350_POWER_LDO_VMC,
	MT6350_POWER_LDO_VEMC33,
	MT6350_POWER_LDO_VIO28,
	MT6350_POWER_LDO_VCAM_AF,
	MT6350_POWER_LDO_VGP1,
	MT6350_POWER_LDO_VEFUSE,
	MT6350_POWER_LDO_VSIM1,
	MT6350_POWER_LDO_VSIM2,
	MT6350_POWER_LDO_VMIPI,
	MT6350_POWER_LDO_VCN18,
	MT6350_POWER_LDO_VGP2,
	MT6350_POWER_LDO_VCAMD,
	MT6350_POWER_LDO_VCAM_IO,
	MT6350_POWER_LDO_VSRAM_DVFS1,
	MT6350_POWER_LDO_VGP3,
	MT6350_POWER_LDO_VBIASN,
	MT6350_POWER_LDO_VRTC,
	MT65XX_POWER_LDO_DEFAULT,
	MT65XX_POWER_COUNT_END,
	MT65XX_POWER_NONE = -1
} MT65XX_POWER;

/* ============================================================================== */
/* DLPT define */
/* ============================================================================== */
typedef enum DLPT_PRIO_TAG {
	DLPT_PRIO_PBM = 0,
	DLPT_PRIO_CPU_B = 1,
	DLPT_PRIO_CPU_L = 2,
	DLPT_PRIO_GPU = 3,
	DLPT_PRIO_MD = 4,
	DLPT_PRIO_MD5 = 5,
	DLPT_PRIO_FLASHLIGHT = 6,
	DLPT_PRIO_VIDEO = 7,
	DLPT_PRIO_WIFI = 8,
	DLPT_PRIO_BACKLIGHT = 9
} DLPT_PRIO;

extern void (*dlpt_callback)(unsigned int);
extern void register_dlpt_notify(void (*dlpt_callback) (unsigned int), DLPT_PRIO prio_val);

/* ============================================================================== */
/* PMIC LDO define */
/* ============================================================================== */

#endif				/* _MT_PMIC_UPMU_SW_H_ */
