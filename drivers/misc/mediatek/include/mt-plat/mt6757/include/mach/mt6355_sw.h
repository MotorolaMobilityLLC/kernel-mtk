/*
 * Copyright (C) 2016 MediaTek Inc.

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#ifndef _MT_PMIC_UPMU_SW_MT6355_H_
#define _MT_PMIC_UPMU_SW_MT6355_H_
#include <mach/upmu_hw.h>

#define AUXADC_SUPPORT_IMM_CURRENT_MODE
/* #define BATTERY_DTS_SUPPORT */
#define BATTERY_SW_INIT
#define RBAT_PULL_UP_VOLT_BY_BIF
/* #define INIT_BAT_CUR_FROM_PTIM */

#define FG_RG_INT_EN_CHRDET 5

#define FG_RG_INT_EN_BAT2_H 20
#define FG_RG_INT_EN_BAT2_L 21

#define FG_RG_INT_EN_BAT_TEMP_H 22
#define FG_RG_INT_EN_BAT_TEMP_L 23

#define FG_RG_INT_EN_NAG_C_DLTV 25

#define FG_BAT0_INT_H_NO 80
#define FG_BAT0_INT_L_NO 81
#define FG_BAT_INT_H_NO 80
#define FG_BAT_INT_L_NO 81

#define FG_CUR_H_NO 82
#define FG_CUR_L_NO 83
#define FG_ZCV_NO 84
#define FG_BAT1_INT_H_NO 85
#define FG_BAT1_INT_L_NO 86
#define FG_N_CHARGE_L_NO 87
#define FG_IAVG_H_NO 88
#define FG_IAVG_L_NO 89
#define FG_TIME_NO 90
#define FG_BAT_PLUGOUT_NO 10

/* ==============================================================================
 * Low battery level define
 * ==============================================================================
 */
#define LOW_BATTERY_LEVEL enum LOW_BATTERY_LEVEL_TAG
#define LOW_BATTERY_PRIO enum LOW_BATTERY_PRIO_TAG
enum LOW_BATTERY_LEVEL_TAG {
	LOW_BATTERY_LEVEL_0 = 0,
	LOW_BATTERY_LEVEL_1 = 1,
	LOW_BATTERY_LEVEL_2 = 2
};

enum LOW_BATTERY_PRIO_TAG {
	LOW_BATTERY_PRIO_CPU_B = 0,
	LOW_BATTERY_PRIO_CPU_L = 1,
	LOW_BATTERY_PRIO_GPU = 2,
	LOW_BATTERY_PRIO_MD = 3,
	LOW_BATTERY_PRIO_MD5 = 4,
	LOW_BATTERY_PRIO_FLASHLIGHT = 5,
	LOW_BATTERY_PRIO_VIDEO = 6,
	LOW_BATTERY_PRIO_WIFI = 7,
	LOW_BATTERY_PRIO_BACKLIGHT = 8
};

extern void (*low_battery_callback)(LOW_BATTERY_LEVEL);
extern void
register_low_battery_notify(void (*low_battery_callback)(LOW_BATTERY_LEVEL),
			    LOW_BATTERY_PRIO prio_val);

/* ==============================================================================
 * Battery OC level define
 * ==============================================================================
 */
#define BATTERY_OC_LEVEL enum BATTERY_OC_LEVEL_TAG
#define BATTERY_OC_PRIO enum BATTERY_OC_PRIO_TAG
enum BATTERY_OC_LEVEL_TAG { BATTERY_OC_LEVEL_0 = 0, BATTERY_OC_LEVEL_1 = 1 };

enum BATTERY_OC_PRIO_TAG {
	BATTERY_OC_PRIO_CPU_B = 0,
	BATTERY_OC_PRIO_CPU_L = 1,
	BATTERY_OC_PRIO_GPU = 2,
	BATTERY_OC_PRIO_MD = 3,
	BATTERY_OC_PRIO_MD5 = 4,
	BATTERY_OC_PRIO_FLASHLIGHT = 5
};

extern void (*battery_oc_callback)(BATTERY_OC_LEVEL);
extern void
register_battery_oc_notify(void (*battery_oc_callback)(BATTERY_OC_LEVEL),
			   BATTERY_OC_PRIO prio_val);

/* ==============================================================================
 * Battery percent define
 * ==============================================================================
 */
#define BATTERY_PERCENT_LEVEL enum BATTERY_PERCENT_LEVEL_TAG
#define BATTERY_PERCENT_PRIO enum BATTERY_PERCENT_PRIO_TAG

enum BATTERY_PERCENT_LEVEL_TAG {
	BATTERY_PERCENT_LEVEL_0 = 0,
	BATTERY_PERCENT_LEVEL_1 = 1
};

enum BATTERY_PERCENT_PRIO_TAG {
	BATTERY_PERCENT_PRIO_CPU_B = 0,
	BATTERY_PERCENT_PRIO_CPU_L = 1,
	BATTERY_PERCENT_PRIO_GPU = 2,
	BATTERY_PERCENT_PRIO_MD = 3,
	BATTERY_PERCENT_PRIO_MD5 = 4,
	BATTERY_PERCENT_PRIO_FLASHLIGHT = 5,
	BATTERY_PERCENT_PRIO_VIDEO = 6,
	BATTERY_PERCENT_PRIO_WIFI = 7,
	BATTERY_PERCENT_PRIO_BACKLIGHT = 8
};

extern void (*battery_percent_callback)(BATTERY_PERCENT_LEVEL);
extern void register_battery_percent_notify(
    void (*battery_percent_callback)(BATTERY_PERCENT_LEVEL),
    BATTERY_PERCENT_PRIO prio_val);

/* ADC Channel Number */
#define pmic_adc_ch_list_enum enum pmic_adc_ch_list_tag
enum pmic_adc_ch_list_tag {
	PMIC_AUX_BATSNS_AP = 0x000,
	PMIC_AUX_ISENSE_AP,
	PMIC_AUX_VCDT_AP,
	PMIC_AUX_BATON_AP, /* BATON/BIF */
	PMIC_AUX_CH4,
	PMIC_AUX_VACCDET_AP,
	PMIC_AUX_CH6,
	PMIC_AUX_TSX,
	PMIC_AUX_CH8,
	PMIC_AUX_CH9,
	PMIC_AUX_CH10,
	PMIC_AUX_CH11,
	PMIC_AUX_CH12,
	PMIC_AUX_CH13,
	PMIC_AUX_CH14,
	PMIC_AUX_CH15,
	PMIC_AUX_CH16,
	PMIC_AUX_CH4_DCXO = 0x10000004,
};

#define MT65XX_POWER enum MT_POWER_TAG
enum MT_POWER_TAG {
	MT65XX_POWER_COUNT_END,
	MT65XX_POWER_LDO_DEFAULT,
	MT65XX_POWER_NONE = -1
};

/*==============================================================================
 * DLPT define
 *==============================================================================
 */
#define DLPT_PRIO enum DLPT_PRIO_TAG
enum DLPT_PRIO_TAG {
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
};

extern void (*dlpt_callback)(unsigned int);
extern void register_dlpt_notify(void (*dlpt_callback)(unsigned int),
				 DLPT_PRIO prio_val);
extern const PMU_FLAG_TABLE_ENTRY pmu_flags_table[];

extern unsigned short is_battery_remove;
extern unsigned short is_wdt_reboot_pmic;
extern unsigned short is_wdt_reboot_pmic_chk;

/*==============================================================================
 * PMIC LDO define
 *==============================================================================
 */

/*==============================================================================
 * PMIC IRQ ENUM define
 *==============================================================================
 */
enum PMIC_IRQ_ENUM {
	INT_PWRKEY,
	INT_HOMEKEY,
	INT_PWRKEY_R,
	INT_HOMEKEY_R,
	INT_NI_LBAT_INT,
	INT_CHRDET,
	INT_CHRDET_EDGE,
	INT_BATON_LV,
	INT_BATON_HV,
	INT_BATON_BAT_IN,
	INT_BATON_BAT_OUT,
	INT_RTC,
	INT_RTC_NSEC,
	INT_BIF,
	INT_VCDT_HV_DET,
	NO_USE_0_15,
	INT_THR_H,
	INT_THR_L,
	INT_BAT_H,
	INT_BAT_L,
	INT_BAT2_H,
	INT_BAT2_L,
	INT_BAT_TEMP_H,
	INT_BAT_TEMP_L,
	INT_AUXADC_IMP,
	INT_NAG_C_DLTV,
	INT_JEITA_HOT,
	INT_JEITA_WARM,
	INT_JEITA_COOL,
	INT_JEITA_COLD,
	NO_USE_1_14,
	NO_USE_1_15,
	INT_VPROC11_OC,
	INT_VPROC12_OC,
	INT_VCORE_OC,
	INT_VGPU_OC,
	INT_VDRAM1_OC,
	INT_VDRAM2_OC,
	INT_VMODEM_OC,
	INT_VS1_OC,
	INT_VS2_OC,
	INT_VPA_OC,
	INT_VCORE_PREOC,
	INT_VA10_OC,
	INT_VA12_OC,
	INT_VA18_OC,
	INT_VBIF28_OC,
	INT_VCAMA1_OC,
	INT_VCAMA2_OC,
	INT_VXO18_OC,
	INT_VCAMD1_OC,
	INT_VCAMD2_OC,
	INT_VCAMIO_OC,
	INT_VCN18_OC,
	INT_VCN28_OC,
	INT_VCN33_OC,
	INT_VTCXO24_OC,
	INT_VEMC_OC,
	INT_VFE28_OC,
	INT_VGP_OC,
	INT_VLDO28_OC,
	INT_VIO18_OC,
	INT_VIO28_OC,
	INT_VMC_OC,
	INT_VMCH_OC,
	INT_VMIPI_OC,
	INT_VRF12_OC,
	INT_VRF18_1_OC,
	INT_VRF18_2_OC,
	INT_VSIM1_OC,
	INT_VSIM2_OC,
	INT_VGP2_OC,
	INT_VSRAM_CORE_OC,
	INT_VSRAM_PROC_OC,
	INT_VSRAM_GPU_OC,
	INT_VSRAM_MD_OC,
	INT_VUFS18_OC,
	INT_VUSB33_OC,
	INT_VXO22_OC,
	NO_USE_4_15,
	INT_FG_BAT0_H,
	INT_FG_BAT0_L,
	INT_FG_CUR_H,
	INT_FG_CUR_L,
	INT_FG_ZCV,
	INT_FG_BAT1_H,
	INT_FG_BAT1_L,
	INT_FG_N_CHARGE_L,
	INT_FG_IAVG_H,
	INT_FG_IAVG_L,
	INT_FG_TIME_H,
	INT_FG_DISCHARGE,
	INT_FG_CHARGE,
	NO_USE_5_13,
	NO_USE_5_14,
	NO_USE_5_15,
	INT_AUDIO,
	INT_MAD,
	INT_EINT_RTC32K_1V8_1,
	INT_EINT_AUD_CLK,
	INT_EINT_AUD_DAT_MOSI,
	INT_EINT_AUD_DAT_MISO,
	INT_EINT_VOW_CLK_MISO,
	INT_ACCDET,
	INT_ACCDET_EINT,
	INT_SPI_CMD_ALERT,
	NO_USE_6_10,
	NO_USE_6_11,
	NO_USE_6_12,
	NO_USE_6_13,
	NO_USE_6_14,
	NO_USE_6_15,
};

/*==============================================================================
 * PMIC auxadc define
 *==============================================================================
 */
extern signed int g_I_SENSE_offset;
extern void pmic_auxadc_init(void);
extern void pmic_auxadc_lock(void);
extern void pmic_auxadc_unlock(void);
extern void mt_power_off(void);
/*==============================================================================
 * PMIC fg define
 *==============================================================================
 */
extern unsigned int bat_get_ui_percentage(void);
extern signed int fgauge_read_v_by_d(int d_val);
extern signed int fgauge_read_r_bat_by_v(signed int voltage);
extern signed int fgauge_read_IM_current(void *data);
extern void kpd_pwrkey_pmic_handler(unsigned long pressed);
extern void kpd_pmic_rstkey_handler(unsigned long pressed);
extern int is_mt6311_sw_ready(void);
extern int is_mt6311_exist(void);
extern int get_mt6311_i2c_ch_num(void);
/*extern bool crystal_exist_status(void);*/
#if defined CONFIG_MTK_LEGACY
extern void pmu_drv_tool_customization_init(void);
#endif
extern int batt_init_cust_data(void);
extern void PMIC_INIT_SETTING_V1(void);

extern int do_ptim_ex(bool isSuspend, unsigned int *bat, signed int *cur);

#endif /* _MT_PMIC_UPMU_SW_MT6355_H_ */
