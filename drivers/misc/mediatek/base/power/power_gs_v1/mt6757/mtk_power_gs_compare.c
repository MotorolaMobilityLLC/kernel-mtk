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

void mt_power_gs_suspend_compare(void)
{
	mt_power_gs_compare("Suspend ", "6351",
			    MT6351_PMIC_REG_gs_flightmode_suspend_mode,
			    MT6351_PMIC_REG_gs_flightmode_suspend_mode_len);
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
	mt_power_gs_compare("DPIdle  ", "6351",
			    MT6351_PMIC_REG_gs_early_suspend_deep_idle_mode,
			    MT6351_PMIC_REG_gs_early_suspend_deep_idle_mode_len);
	mt_power_gs_compare("DPIdle ", "CG  ",
			    CG_Golden_Setting_tcl_gs_dpidle,
			    CG_Golden_Setting_tcl_gs_dpidle_len);
	mt_power_gs_compare("DPIdle ", "DCM ",
			    AP_DCM_Golden_Setting_tcl_gs_dpidle,
			    AP_DCM_Golden_Setting_tcl_gs_dpidle_len);
	mt_power_gs_sp_dump();
}
