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
#include <mt-plat/upmu_common.h>
#include <mt-plat/mtk_battery.h>
#include <mt-plat/mtk_auxadc_intf.h>



int pmic_get_battery_voltage(void)
{
	int bat = 0;
#if defined(CONFIG_POWER_EXT)
	bat = 4201;
#else
	bat = pmic_get_auxadc_value(AUXADC_LIST_BATADC);
#endif
	return bat;
}

int pmic_get_vbus(void)
{
	int vchr = 0;
#if defined(CONFIG_POWER_EXT)
	vchr = 5001;
#else
	vchr = pmic_get_auxadc_value(AUXADC_LIST_VCDT);
	vchr =
		(((fg_cust_data.r_charger_1 +
		fg_cust_data.r_charger_2) * 100 * vchr) /
		fg_cust_data.r_charger_2) / 100;

#endif
	return vchr;
}

int pmic_get_ibat(void)
{
	return 0;
}

int pmic_is_bif_exist(void)
{
	int vbat;

	if (pmic_get_bif_battery_voltage(&vbat) == -1)
		return 0;
	return 1;
}

int pmic_get_bif_battery_voltage(int *vbat)
{
	return -1;
}

int pmic_get_bif_battery_temperature(int *tmp)
{
	return -1;
}

