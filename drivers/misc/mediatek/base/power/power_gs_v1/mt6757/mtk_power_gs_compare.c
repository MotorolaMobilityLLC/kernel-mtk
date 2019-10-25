/*
 * Copyright (C) 2017 MediaTek Inc.
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

void mt_power_gs_suspend_compare(unsigned int dump_flag)
{
#ifdef CONFIG_MTK_PMIC_CHIP_MT6355
	if (dump_flag & GS_PMIC)
		mt_power_gs_compare("Suspend ", "6355",
				    MT6355_PMIC_REG_gs_suspend,
				    MT6355_PMIC_REG_gs_suspend_len);
#else
	if (dump_flag & GS_PMIC)
		mt_power_gs_compare(
		    "Suspend ", "6351",
		    MT6351_PMIC_REG_gs_flightmode_suspend_mode,
		    MT6351_PMIC_REG_gs_flightmode_suspend_mode_len);
#endif

	if (dump_flag & GS_CG)
		mt_power_gs_compare("Suspend ", "CG  ",
				    CG_Golden_Setting_tcl_gs_suspend,
				    CG_Golden_Setting_tcl_gs_suspend_len);

	if (dump_flag & GS_DCM)
		mt_power_gs_compare("Suspend ", "DCM ",
				    AP_DCM_Golden_Setting_tcl_gs_suspend,
				    AP_DCM_Golden_Setting_tcl_gs_suspend_len);
}

void mt_power_gs_dpidle_compare(unsigned int dump_flag)
{
#ifdef CONFIG_MTK_PMIC_CHIP_MT6355
	if (dump_flag & GS_PMIC)
		mt_power_gs_compare("DPIdle  ", "6355",
				    MT6355_PMIC_REG_gs_deepidle_lp_mp3,
				    MT6355_PMIC_REG_gs_deepidle_lp_mp3_len);
#else
	if (dump_flag & GS_PMIC)
		mt_power_gs_compare(
		    "DPIdle  ", "6351",
		    MT6351_PMIC_REG_gs_early_suspend_deep_idle_mode,
		    MT6351_PMIC_REG_gs_early_suspend_deep_idle_mode_len);
#endif

	if (dump_flag & GS_CG)
		mt_power_gs_compare("DPIdle ", "CG  ",
				    CG_Golden_Setting_tcl_gs_dpidle,
				    CG_Golden_Setting_tcl_gs_dpidle_len);
	if (dump_flag & GS_DCM)
		mt_power_gs_compare("DPIdle ", "DCM ",
				    AP_DCM_Golden_Setting_tcl_gs_dpidle,
				    AP_DCM_Golden_Setting_tcl_gs_dpidle_len);
}
