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

#include "mtk_power_gs.h"
#include "mtk_power_gs_array.h"
#include <mt-plat/mtk_rtc.h>
#include <mt-plat/mtk_chip.h>

static unsigned int chip_sw_ver;

static unsigned int *cg_suspend;
static unsigned int cg_suspend_len;
static unsigned int *dcm_suspend;
static unsigned int dcm_suspend_len;

static unsigned int *cg_sodi;
static unsigned int cg_sodi_len;
static unsigned int *dcm_sodi;
static unsigned int dcm_sodi_len;

static unsigned int *cg_dpidle;
static unsigned int cg_dpidle_len;
static unsigned int *dcm_dpidle;
static unsigned int dcm_dpidle_len;

static unsigned int *pmic_suspend;
static unsigned int pmic_suspend_len;
static unsigned int *pmic_suspend_32kless;
static unsigned int pmic_suspend_32kless_len;

static unsigned int *pmic_sodi;
static unsigned int pmic_sodi_len;
static unsigned int *pmic_sodi_32kless;
static unsigned int pmic_sodi_32kless_len;

static unsigned int *pmic_dpidle;
static unsigned int pmic_dpidle_len;
static unsigned int *pmic_dpidle_32kless;
static unsigned int pmic_dpidle_32kless_len;

void mt_power_gs_internal_init(void)
{
	chip_sw_ver = mt_get_chip_sw_ver();

	if (chip_sw_ver >= CHIP_SW_VER_02) {
		cg_suspend = CG_Golden_Setting_tcl_gs_suspend_v2;
		cg_suspend_len = CG_Golden_Setting_tcl_gs_suspend_len_v2;
		dcm_suspend = AP_DCM_Golden_Setting_tcl_gs_suspend_v2;
		dcm_suspend_len = AP_DCM_Golden_Setting_tcl_gs_suspend_len_v2;
		pmic_suspend = MT6335_PMIC_REG_gs_flightmode_suspend_mode_v2;
		pmic_suspend_len = MT6335_PMIC_REG_gs_flightmode_suspend_mode_len_v2;
		pmic_suspend_32kless = MT6335_PMIC_REG_gs_flightmode_suspend_mode_32kless_v2;
		pmic_suspend_32kless_len = MT6335_PMIC_REG_gs_flightmode_suspend_mode_32kless_len_v2;
		cg_sodi = CG_Golden_Setting_tcl_gs_sodi_v2;
		cg_sodi_len = CG_Golden_Setting_tcl_gs_sodi_len_v2;
		dcm_sodi = AP_DCM_Golden_Setting_tcl_gs_sodi_v2;
		dcm_sodi_len = AP_DCM_Golden_Setting_tcl_gs_sodi_len_v2;
		pmic_sodi = MT6335_PMIC_REG_gs_sodi3P0_v2;
		pmic_sodi_len = MT6335_PMIC_REG_gs_sodi3P0_len_v2;
		pmic_sodi_32kless = MT6335_PMIC_REG_gs_sodi3P0_32kless_v2;
		pmic_sodi_32kless_len = MT6335_PMIC_REG_gs_sodi3P0_32kless_len_v2;
		cg_dpidle = CG_Golden_Setting_tcl_gs_dpidle_v2;
		cg_dpidle_len = CG_Golden_Setting_tcl_gs_dpidle_len_v2;
		dcm_dpidle = AP_DCM_Golden_Setting_tcl_gs_dpidle_v2;
		dcm_dpidle_len = AP_DCM_Golden_Setting_tcl_gs_dpidle_len_v2;
		pmic_dpidle = MT6335_PMIC_REG_gs_early_suspend_deep_idle_mode_v2;
		pmic_dpidle_len = MT6335_PMIC_REG_gs_early_suspend_deep_idle_mode_len_v2;
		pmic_dpidle_32kless = MT6335_PMIC_REG_gs_early_suspend_deep_idle_mode_32kless_v2;
		pmic_dpidle_32kless_len = MT6335_PMIC_REG_gs_early_suspend_deep_idle_mode_32kless_len_v2;
	} else {
		cg_suspend = CG_Golden_Setting_tcl_gs_suspend;
		cg_suspend_len = CG_Golden_Setting_tcl_gs_suspend_len;
		dcm_suspend = AP_DCM_Golden_Setting_tcl_gs_suspend;
		dcm_suspend_len = AP_DCM_Golden_Setting_tcl_gs_suspend_len;
		pmic_suspend = MT6335_PMIC_REG_gs_flightmode_suspend_mode;
		pmic_suspend_len = MT6335_PMIC_REG_gs_flightmode_suspend_mode_len;
		pmic_suspend_32kless = MT6335_PMIC_REG_gs_flightmode_suspend_mode_32kless;
		pmic_suspend_32kless_len = MT6335_PMIC_REG_gs_flightmode_suspend_mode_32kless_len;
		cg_sodi = CG_Golden_Setting_tcl_gs_sodi;
		cg_sodi_len = CG_Golden_Setting_tcl_gs_sodi_len;
		dcm_sodi = AP_DCM_Golden_Setting_tcl_gs_sodi;
		dcm_sodi_len = AP_DCM_Golden_Setting_tcl_gs_sodi_len;
		pmic_sodi = MT6335_PMIC_REG_gs_sodi3P0;
		pmic_sodi_len = MT6335_PMIC_REG_gs_sodi3P0_len;
		pmic_sodi_32kless = MT6335_PMIC_REG_gs_sodi3P0_32kless;
		pmic_sodi_32kless_len = MT6335_PMIC_REG_gs_sodi3P0_32kless_len;
		cg_dpidle = CG_Golden_Setting_tcl_gs_dpidle;
		cg_dpidle_len = CG_Golden_Setting_tcl_gs_dpidle_len;
		dcm_dpidle = AP_DCM_Golden_Setting_tcl_gs_dpidle;
		dcm_dpidle_len = AP_DCM_Golden_Setting_tcl_gs_dpidle_len;
		pmic_dpidle = MT6335_PMIC_REG_gs_early_suspend_deep_idle_mode;
		pmic_dpidle_len = MT6335_PMIC_REG_gs_early_suspend_deep_idle_mode_len;
		pmic_dpidle_32kless = MT6335_PMIC_REG_gs_early_suspend_deep_idle_mode_32kless;
		pmic_dpidle_32kless_len = MT6335_PMIC_REG_gs_early_suspend_deep_idle_mode_32kless_len;
	}
}

void mt_power_gs_table_init(void)
{
	mt_power_gs_base_remap_init("Suspend ", "CG  ", cg_suspend, cg_suspend_len);
	mt_power_gs_base_remap_init("Suspend ", "DCM ", dcm_suspend, dcm_suspend_len);
	mt_power_gs_base_remap_init("DPIdle ", "CG  ", cg_dpidle, cg_dpidle_len);
	mt_power_gs_base_remap_init("DPIdle ", "DCM ", dcm_dpidle, dcm_dpidle_len);
	mt_power_gs_base_remap_init("SODI ", "CG  ", cg_sodi, cg_sodi_len);
	mt_power_gs_base_remap_init("SODI ", "DCM ", dcm_sodi, dcm_sodi_len);
}

void mt_power_gs_suspend_compare(unsigned int dump_flag)
{
	pr_warn("Power_gs: ver_%d\n", chip_sw_ver);

	if (dump_flag & GS_PMIC) {
		if (crystal_exist_status() == true) {
			/* 32k */
			mt_power_gs_compare("Suspend ", "6335", pmic_suspend, pmic_suspend_len);
		} else {
			/* 32k-less */
			pr_warn("Power_gs: %s in 32k-less\n", __func__);
			mt_power_gs_compare("Suspend ", "6335",
				    pmic_suspend_32kless,
				    pmic_suspend_32kless_len);
		}
	}

	if (dump_flag & GS_CG) {
		mt_power_gs_compare("Suspend ", "CG  ", cg_suspend, cg_suspend_len);
	}

	if (dump_flag & GS_DCM) {
		mt_power_gs_compare("Suspend ", "DCM ", dcm_suspend, dcm_suspend_len);
	}

	mt_power_gs_sp_dump();
}

void mt_power_gs_dpidle_compare(unsigned int dump_flag)
{
	pr_warn("Power_gs: ver_%d\n", chip_sw_ver);

	if (dump_flag & GS_PMIC) {
		if (crystal_exist_status() == true) {
			/* 32k */
			mt_power_gs_compare("DPIdle  ", "6335", pmic_dpidle, pmic_dpidle_len);
		} else {
			/* 32k-less */
			pr_warn("Power_gs: %s in 32k-less\n", __func__);
			mt_power_gs_compare("DPIdle  ", "6335",
				    pmic_dpidle_32kless,
				    pmic_dpidle_32kless_len);
		}
	}

	if (dump_flag & GS_CG) {
		mt_power_gs_compare("DPIdle ", "CG  ", cg_dpidle, cg_dpidle_len);
	}

	if (dump_flag & GS_DCM) {
		mt_power_gs_compare("DPIdle ", "DCM ", dcm_dpidle, dcm_dpidle_len);
	}

	mt_power_gs_sp_dump();
}

void mt_power_gs_sodi_compare(unsigned int dump_flag)
{
	pr_warn("Power_gs: ver_%d\n", chip_sw_ver);

	if (dump_flag & GS_PMIC) {
		if (crystal_exist_status() == true) {
			/* 32k */
			mt_power_gs_compare("SODI  ", "6335", pmic_sodi, pmic_sodi_len);
		} else {
			/* 32k-less */
			pr_warn("Power_gs: %s in 32k-less\n", __func__);
			mt_power_gs_compare("SODI  ", "6335",
				    pmic_sodi_32kless,
				    pmic_sodi_32kless_len);
		}
	}

	if (dump_flag & GS_CG) {
		mt_power_gs_compare("SODI ", "CG  ", cg_sodi, cg_sodi_len);
	}

	if (dump_flag & GS_DCM) {
		mt_power_gs_compare("SODI ", "DCM ", dcm_sodi, dcm_sodi_len);
	}

	mt_power_gs_sp_dump();
}
