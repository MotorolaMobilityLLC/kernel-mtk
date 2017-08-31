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
}

void mt_power_gs_suspend_compare(void)
{
	if (crystal_exist_status() == true) {
		/* 32k */
		mt_power_gs_compare("Suspend ", "6335",
			    MT6335_PMIC_REG_gs_flightmode_suspend_mode,
			    MT6335_PMIC_REG_gs_flightmode_suspend_mode_len);
	} else {
		/* 32k-less */
		mt_power_gs_compare("Suspend_32kless ", "6335",
			    MT6335_PMIC_REG_gs_flightmode_suspend_mode_32kless,
			    MT6335_PMIC_REG_gs_flightmode_suspend_mode_32kless_len);
	}
	mt_power_gs_compare("Suspend ", "CG  ",
			    CG_Golden_Setting_tcl_gs_suspend,
			    CG_Golden_Setting_tcl_gs_suspend_len);
	mt_power_gs_compare("Suspend ", "DCM ",
			    AP_DCM_Golden_Setting_tcl_gs_suspend,
			    AP_DCM_Golden_Setting_tcl_gs_suspend_len);
	mt_power_gs_sp_dump();
}

void mt_power_gs_dpidle_compare(void)
{
	if (crystal_exist_status() == true) {
		/* 32k */
		mt_power_gs_compare("DPIdle  ", "6335",
			    MT6335_PMIC_REG_gs_early_suspend_deep_idle_mode,
			    MT6335_PMIC_REG_gs_early_suspend_deep_idle_mode_len);
	} else {
		/* 32k-less */
		mt_power_gs_compare("DPIdle_32kless  ", "6335",
			    MT6335_PMIC_REG_gs_early_suspend_deep_idle_mode_32kless,
			    MT6335_PMIC_REG_gs_early_suspend_deep_idle_mode_32kless_len);
	}
	mt_power_gs_compare("DPIdle ", "CG  ",
			    CG_Golden_Setting_tcl_gs_dpidle,
			    CG_Golden_Setting_tcl_gs_dpidle_len);
	mt_power_gs_compare("DPIdle ", "DCM ",
			    AP_DCM_Golden_Setting_tcl_gs_dpidle,
			    AP_DCM_Golden_Setting_tcl_gs_dpidle_len);
	mt_power_gs_sp_dump();
}
