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


int mtk_chr_enable_direct_charge(struct charger_manager *info, unsigned char charging_enable)
{
	int ret = 0;

	return ret;
}

int mtk_chr_kick_direct_charge_wdt(struct charger_manager *info)
{
	int ret = 0;

	return ret;

}

int mtk_chr_set_direct_charge_ibus_oc(struct charger_manager *info, unsigned int cur)
{
	int ret = 0;

	return ret;
}

int mtk_chr_enable_charge(struct charger_manager *info, unsigned char charging_enable)
{
	int ret = 0;


	return ret;
}

int mtk_chr_get_input_current(struct charger_manager *info, unsigned int *cur)
{
	int ret = 0;
	return ret;
}

int mtk_chr_enable_power_path(struct charger_manager *info, unsigned char en)
{
	return 0;
}

int mtk_chr_get_tchr(struct charger_manager *info, int *min_temp, int *max_temp)
{
	return 25;
}

void wake_up_bat3(void)
{}


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


