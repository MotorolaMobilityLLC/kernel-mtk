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
#include "mt-plat/mtk_rtc.h"

void mt_power_gs_table_init(void)
{
	mt_power_gs_base_remap_init("Suspend ", "CG  ",
			    CG_Golden_Setting_tcl_gs_suspend,
			    CG_Golden_Setting_tcl_gs_suspend_len);
	mt_power_gs_base_remap_init("Suspend ", "DCM ",
			    AP_DCM_Golden_Setting_tcl_gs_suspend,
			    AP_DCM_Golden_Setting_tcl_gs_suspend_len);
	mt_power_gs_base_remap_init("DPIdle ", "CG  ",
			    CG_Golden_Setting_tcl_gs_dpidle,
			    CG_Golden_Setting_tcl_gs_dpidle_len);
	mt_power_gs_base_remap_init("DPIdle ", "DCM ",
			    AP_DCM_Golden_Setting_tcl_gs_dpidle,
			    AP_DCM_Golden_Setting_tcl_gs_dpidle_len);
	mt_power_gs_base_remap_init("SODI ", "CG  ",
			    CG_Golden_Setting_tcl_gs_sodi,
			    CG_Golden_Setting_tcl_gs_sodi_len);
	mt_power_gs_base_remap_init("SODI ", "DCM ",
			    AP_DCM_Golden_Setting_tcl_gs_sodi,
			    AP_DCM_Golden_Setting_tcl_gs_sodi_len);
}

void mt_power_gs_suspend_compare(unsigned int dump_flag)
{
	if (dump_flag & GS_PMIC) {
		if (crystal_exist_status() == true) {
			/* 32k */
			mt_power_gs_compare("Suspend ", "6335",
				    MT6335_PMIC_REG_gs_flightmode_suspend_mode,
				    MT6335_PMIC_REG_gs_flightmode_suspend_mode_len);
		} else {
			/* 32k-less */
			pr_warn("Power_gs: %s in 32k-less\n", __func__);
			mt_power_gs_compare("Suspend ", "6335",
				    MT6335_PMIC_REG_gs_flightmode_suspend_mode_32kless,
				    MT6335_PMIC_REG_gs_flightmode_suspend_mode_32kless_len);
		}
	}

	if (dump_flag & GS_CG) {
		mt_power_gs_compare("Suspend ", "CG  ",
				    CG_Golden_Setting_tcl_gs_suspend,
				    CG_Golden_Setting_tcl_gs_suspend_len);
	}

	if (dump_flag & GS_DCM) {
		mt_power_gs_compare("Suspend ", "DCM ",
				    AP_DCM_Golden_Setting_tcl_gs_suspend,
				    AP_DCM_Golden_Setting_tcl_gs_suspend_len);
	}

	mt_power_gs_sp_dump();
}

void mt_power_gs_dpidle_compare(unsigned int dump_flag)
{
	if (dump_flag & GS_PMIC) {
		if (crystal_exist_status() == true) {
			/* 32k */
			mt_power_gs_compare("DPIdle  ", "6335",
				    MT6335_PMIC_REG_gs_early_suspend_deep_idle_mode,
				    MT6335_PMIC_REG_gs_early_suspend_deep_idle_mode_len);
		} else {
			/* 32k-less */
			pr_warn("Power_gs: %s in 32k-less\n", __func__);
			mt_power_gs_compare("DPIdle  ", "6335",
				    MT6335_PMIC_REG_gs_early_suspend_deep_idle_mode_32kless,
				    MT6335_PMIC_REG_gs_early_suspend_deep_idle_mode_32kless_len);
		}
	}

	if (dump_flag & GS_CG) {
		mt_power_gs_compare("DPIdle ", "CG  ",
				    CG_Golden_Setting_tcl_gs_dpidle,
				    CG_Golden_Setting_tcl_gs_dpidle_len);
	}

	if (dump_flag & GS_DCM) {
		mt_power_gs_compare("DPIdle ", "DCM ",
				    AP_DCM_Golden_Setting_tcl_gs_dpidle,
				    AP_DCM_Golden_Setting_tcl_gs_dpidle_len);
	}

	mt_power_gs_sp_dump();
}

void mt_power_gs_sodi_compare(unsigned int dump_flag)
{
	if (dump_flag & GS_PMIC) {
		if (crystal_exist_status() == true) {
			/* 32k */
			mt_power_gs_compare("SODI  ", "6335",
				    MT6335_PMIC_REG_gs_sodi3P0,
				    MT6335_PMIC_REG_gs_sodi3P0_len);
		} else {
			/* 32k-less */
			pr_warn("Power_gs: %s in 32k-less\n", __func__);
			mt_power_gs_compare("SODI  ", "6335",
				    MT6335_PMIC_REG_gs_sodi3P0_32kless,
				    MT6335_PMIC_REG_gs_sodi3P0_32kless_len);
		}
	}

	if (dump_flag & GS_CG) {
		mt_power_gs_compare("SODI ", "CG  ",
				    CG_Golden_Setting_tcl_gs_sodi,
				    CG_Golden_Setting_tcl_gs_sodi_len);
	}

	if (dump_flag & GS_DCM) {
		mt_power_gs_compare("SODI ", "DCM ",
				    AP_DCM_Golden_Setting_tcl_gs_sodi,
				    AP_DCM_Golden_Setting_tcl_gs_sodi_len);
	}

	mt_power_gs_sp_dump();
}
