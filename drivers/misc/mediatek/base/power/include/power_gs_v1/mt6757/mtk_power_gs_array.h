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

#ifndef MTK_POWER_GS_ARRAY_H
#define MTK_POWER_GS_ARRAY_H

extern void mt_power_gs_sp_dump(void);
extern unsigned int golden_read_reg(unsigned int addr);

#ifdef CONFIG_MTK_PMIC_CHIP_MT6355
extern const unsigned int *MT6355_PMIC_REG_gs_suspend_32kless;
extern unsigned int MT6355_PMIC_REG_gs_suspend_32kless_len;

extern const unsigned int *MT6355_PMIC_REG_gs_sodi3_0_32kless;
extern unsigned int MT6355_PMIC_REG_gs_sodi3_0_32kless_len;

extern const unsigned int *MT6355_PMIC_REG_gs_deepidle_lp_mp3;
extern unsigned int MT6355_PMIC_REG_gs_deepidle_lp_mp3_len;

extern const unsigned int *MT6355_PMIC_REG_gs_suspend;
extern unsigned int MT6355_PMIC_REG_gs_suspend_len;

extern const unsigned int *MT6355_PMIC_REG_gs_deepidle_lp_mp3_32kless;
extern unsigned int MT6355_PMIC_REG_gs_deepidle_lp_mp3_32kless_len;

extern const unsigned int *MT6355_PMIC_REG_gs_sodi3_0;
extern unsigned int MT6355_PMIC_REG_gs_sodi3_0_len;

extern const unsigned int *MT6355_PMIC_REG_gs_w_key;
extern unsigned int MT6355_PMIC_REG_gs_w_key_len;

#else
extern const unsigned int *MT6351_PMIC_REG_gs_flightmode_suspend_mode;
extern unsigned int MT6351_PMIC_REG_gs_flightmode_suspend_mode_len;

extern const unsigned int *MT6351_PMIC_REG_gs_suspend_mode;
extern unsigned int MT6351_PMIC_REG_gs_suspend_mode_len;

extern const unsigned int *MT6351_PMIC_REG_gs_early_suspend_deep_idle_mode;
extern unsigned int MT6351_PMIC_REG_gs_early_suspend_deep_idle_mode_len;

#endif

extern const unsigned int *CG_Golden_Setting_tcl_gs_dpidle;
extern unsigned int CG_Golden_Setting_tcl_gs_dpidle_len;

extern const unsigned int *CG_Golden_Setting_tcl_gs_suspend;
extern unsigned int CG_Golden_Setting_tcl_gs_suspend_len;

extern const unsigned int *CG_Golden_Setting_tcl_gs_vp;
extern unsigned int CG_Golden_Setting_tcl_gs_vp_len;

extern const unsigned int *CG_Golden_Setting_tcl_gs_topck_name;
extern unsigned int CG_Golden_Setting_tcl_gs_topck_name_len;

extern const unsigned int *CG_Golden_Setting_tcl_gs_paging;
extern unsigned int CG_Golden_Setting_tcl_gs_paging_len;

extern const unsigned int *CG_Golden_Setting_tcl_gs_mp3_play;
extern unsigned int CG_Golden_Setting_tcl_gs_mp3_play_len;

extern const unsigned int *CG_Golden_Setting_tcl_gs_pdn_ao;
extern unsigned int CG_Golden_Setting_tcl_gs_pdn_ao_len;

extern const unsigned int *CG_Golden_Setting_tcl_gs_idle;
extern unsigned int CG_Golden_Setting_tcl_gs_idle_len;

extern const unsigned int *CG_Golden_Setting_tcl_gs_clkon;
extern unsigned int CG_Golden_Setting_tcl_gs_clkon_len;

extern const unsigned int *CG_Golden_Setting_tcl_gs_talk;
extern unsigned int CG_Golden_Setting_tcl_gs_talk_len;

extern const unsigned int *CG_Golden_Setting_tcl_gs_connsys;
extern unsigned int CG_Golden_Setting_tcl_gs_connsys_len;

extern const unsigned int *CG_Golden_Setting_tcl_gs_datalink;
extern unsigned int CG_Golden_Setting_tcl_gs_datalink_len;

extern const unsigned int *CG_Golden_Setting_tcl_gs_vr;
extern unsigned int CG_Golden_Setting_tcl_gs_vr_len;

extern const unsigned int *CG_Golden_Setting_tcl_gs_flight;
extern unsigned int CG_Golden_Setting_tcl_gs_flight_len;

extern const unsigned int *AP_DCM_Golden_Setting_tcl_gs_dpidle;
extern unsigned int AP_DCM_Golden_Setting_tcl_gs_dpidle_len;

extern const unsigned int *AP_DCM_Golden_Setting_tcl_gs_suspend;
extern unsigned int AP_DCM_Golden_Setting_tcl_gs_suspend_len;

extern const unsigned int *AP_DCM_Golden_Setting_tcl_gs_vp;
extern unsigned int AP_DCM_Golden_Setting_tcl_gs_vp_len;

extern const unsigned int *AP_DCM_Golden_Setting_tcl_gs_topck_name;
extern unsigned int AP_DCM_Golden_Setting_tcl_gs_topck_name_len;

extern const unsigned int *AP_DCM_Golden_Setting_tcl_gs_paging;
extern unsigned int AP_DCM_Golden_Setting_tcl_gs_paging_len;

extern const unsigned int *AP_DCM_Golden_Setting_tcl_gs_mp3_play;
extern unsigned int AP_DCM_Golden_Setting_tcl_gs_mp3_play_len;

extern const unsigned int *AP_DCM_Golden_Setting_tcl_gs_pdn_ao;
extern unsigned int AP_DCM_Golden_Setting_tcl_gs_pdn_ao_len;

extern const unsigned int *AP_DCM_Golden_Setting_tcl_gs_idle;
extern unsigned int AP_DCM_Golden_Setting_tcl_gs_idle_len;

extern const unsigned int *AP_DCM_Golden_Setting_tcl_gs_talk;
extern unsigned int AP_DCM_Golden_Setting_tcl_gs_talk_len;

extern const unsigned int *AP_DCM_Golden_Setting_tcl_gs_connsys;
extern unsigned int AP_DCM_Golden_Setting_tcl_gs_connsys_len;

extern const unsigned int *AP_DCM_Golden_Setting_tcl_gs_dcm_off;
extern unsigned int AP_DCM_Golden_Setting_tcl_gs_dcm_off_len;

extern const unsigned int *AP_DCM_Golden_Setting_tcl_gs_datalink;
extern unsigned int AP_DCM_Golden_Setting_tcl_gs_datalink_len;

extern const unsigned int *AP_DCM_Golden_Setting_tcl_gs_vr;
extern unsigned int AP_DCM_Golden_Setting_tcl_gs_vr_len;

extern const unsigned int *AP_DCM_Golden_Setting_tcl_gs_flight;
extern unsigned int AP_DCM_Golden_Setting_tcl_gs_flight_len;
#endif
