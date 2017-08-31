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

#include <linux/errno.h>
#include <linux/delay.h>
#include "mtk_charger_intf.h"

int mtk_chr_get_tchr(int *min_temp, int *max_temp)
{
	*min_temp = 25;
	*max_temp = 25;
	return 0;
}


int pe30_dc_enable_chip(struct charger_manager *info, unsigned char en)
{
	int ret = 0;

	if (info->dc_chg != NULL)
		charger_dev_enable_chip(info->dc_chg, en);

	return ret;
}

int pe30_dc_enable(struct charger_manager *info, unsigned char charging_enable)
{
	int ret = 0;

	if (info->dc_chg != NULL)
		charger_dev_enable_direct_charging(info->dc_chg, true);

	return ret;
}

int pe30_dc_kick_wdt(struct charger_manager *info)
{
	int ret = 0;

	if (info->dc_chg != NULL)
		charger_dev_kick_direct_charging_wdt(info->dc_chg);

	return ret;
}

int pe30_dc_set_vbus_ov(struct charger_manager *info, unsigned int vol)
{
	int ret = 0;

	if (info->dc_chg != NULL)
		charger_dev_set_direct_charging_vbusov(info->dc_chg, vol);

	return ret;
}

int pe30_dc_set_ibus_oc(struct charger_manager *info, unsigned int cur)
{
	int ret = 0;

	if (info->dc_chg != NULL)
		charger_dev_set_direct_charging_ibusoc(info->dc_chg, cur);

	return ret;
}

int pe30_dc_get_temperature(struct charger_manager *info, int *min_temp, int *max_temp)
{
	if (info->dc_chg != NULL)
		charger_dev_get_temperature(info->dc_chg, min_temp, max_temp);

	return 0;
}

int pe30_chr_enable_charge(struct charger_manager *info, bool en)
{
	if (info->chg1_dev != NULL)
		charger_dev_enable(info->chg1_dev, en);
	return 0;
}

int pe30_chr_enable_power_path(struct charger_manager *info, bool en)
{
	if (info->chg1_dev != NULL)
		charger_dev_enable_powerpath(info->chg1_dev, en);
	return 0;
}

int pe30_chr_get_tchr(struct charger_manager *info, int *min_temp, int *max_temp)
{
	*min_temp = 25;
	*max_temp = 25;
	return 25;
}

int pe30_chr_get_input_current(struct charger_manager *info, unsigned int *cur)
{
	int ret = 0;
	return ret;
}

int __attribute__ ((weak))
	mtk_cooler_is_abcct_unlimit(void)
{
	pr_err("does not support mtk_cooler_is_abcct_unlimit\n");
	return 1;
}

/* VDM cmd */


bool __attribute__ ((weak))
	mtk_is_pd_chg_ready(void)
{
	pr_err("does not support mtk_is_pd_chg_ready\n");
	return false;
}

int __attribute__ ((weak))
	tcpm_hard_reset(void *ptr)
{
	pr_err("does not support tcpm_hard_reset\n");
	return 0;
}

int __attribute__ ((weak))
	tcpm_set_direct_charge_en(void *tcpc_dev, bool en)
{
	pr_err("does not support tcpm_set_direct_charge_en\n");
	return 0;
}

int __attribute__ ((weak))
	tcpm_get_cable_capability(void *tcpc_dev,
					unsigned char *capability)
{
	pr_err("does not support tcpm_get_cable_capability\n");
	return 0;
}

int __attribute__ ((weak))
	mtk_clr_ta_pingcheck_fault(void *ptr)
{
	pr_err("does not support mtk_clr_ta_pingcheck_fault\n");
	return 0;
}

bool __attribute__ ((weak))
	mtk_is_pep30_en_unlock(void)
{
	pr_err("does not support mtk_is_pep30_en_unlock\n");
	return false;
}

/* VDM cmd end*/
