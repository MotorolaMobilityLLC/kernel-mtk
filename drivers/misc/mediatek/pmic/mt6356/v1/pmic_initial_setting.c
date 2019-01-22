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

#include <mt-plat/upmu_common.h>
#include <mt-plat/mtk_chip.h>
#include <mt-plat/mtk_rtc.h>

#include <linux/io.h>
#include <linux/io.h>
#include <linux/of_address.h>

#include "include/pmic.h"
#include "include/pmic_api.h"
#include "include/pmic_api_buck.h"

int PMIC_MD_INIT_SETTING_V1(void)
{
	/* No need for PMIC MT6356 */
	return 0;
}

int PMIC_check_wdt_status(void)
{
	unsigned int ret = 0;

	is_wdt_reboot_pmic = pmic_get_register_value(PMIC_WDTRSTB_STATUS);
	ret = pmic_set_register_value(PMIC_TOP_RST_MISC_SET, 0x8);
	is_wdt_reboot_pmic_chk = pmic_get_register_value(PMIC_WDTRSTB_STATUS);
	ret = pmic_set_register_value(PMIC_TOP_RST_MISC_CLR, 0x8);
	ret = pmic_set_register_value(PMIC_TOP_RST_MISC_SET, 0x1);
	ret = pmic_get_register_value(PMIC_RG_WDTRSTB_EN);
	return ret;
}

int PMIC_check_pwrhold_status(void)
{
	unsigned int val = 0;

	pmic_read_interface(PMIC_RG_PWRHOLD_ADDR, &val, PMIC_RG_PWRHOLD_MASK,
			      PMIC_RG_PWRHOLD_SHIFT);
	return val;
}

int PMIC_check_battery(void)
{
	unsigned int val = 0;

	/* ask shin-shyu programming guide */
	mt6356_upmu_set_rg_baton_en(1);
	/*PMIC_upmu_set_baton_tdet_en(1);*/
	val = mt6356_upmu_get_rgs_baton_undet();
	if (val == 0) {
		pr_debug("bat is exist.\n");
		is_battery_remove = 0;
		return 1;
	}
	pr_debug("bat NOT exist.\n");
	is_battery_remove = 1;
	return 0;
}

int PMIC_POWER_HOLD(unsigned int hold)
{
	if (hold > 1) {
		pr_err("[PMIC_KERNEL] PMIC_POWER_HOLD hold = %d only 0 or 1\n", hold);
		return -1;
	}

	if (hold)
		PMICLOG("[PMIC_KERNEL] PMIC_POWER_HOLD ON\n");
	else
		PMICLOG("[PMIC_KERNEL] PMIC_POWER_HOLD OFF\n");

	/* MT6355 must keep power hold */
	pmic_config_interface_nolock(PMIC_RG_PWRHOLD_ADDR, hold, PMIC_RG_PWRHOLD_MASK,
			      PMIC_RG_PWRHOLD_SHIFT);
	PMICLOG("[PMIC_KERNEL] MT6356 PowerHold = 0x%x\n", upmu_get_reg_value(MT6356_PPCCTL0));

	return 0;
}

void PMIC_LP_INIT_SETTING(void)
{
	/*--suspend--*/

	/*--deepidle--*/

}
