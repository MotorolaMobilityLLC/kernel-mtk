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
#include <linux/mutex.h>
#include <linux/wakelock.h>
#include <linux/delay.h>
#include "mtk_charger_intf.h"

static bool mtk_is_pdc_ready(struct charger_manager *info)
{
	if (info->pdc.tcpc == NULL)
		return false;

	if (tcpm_inquire_pd_prev_connected(info->pdc.tcpc) == 0)
		return false;

	return true;
}

bool mtk_pdc_check_charger(struct charger_manager *info)
{

	if (mtk_is_pdc_ready(info) == false)
		return false;

	return true;
}

void mtk_pdc_plugout_reset(struct charger_manager *info)
{
	info->pdc.cap.nr = 0;
}

void mtk_pdc_set_max_watt(struct charger_manager *info, unsigned int watt)
{
	info->pdc.pdc_max_watt = watt;
}

unsigned int mtk_pdc_get_max_watt(struct charger_manager *info)
{
	int charging_current = info->data.pd_charger_current / 1000;
	int vbat = pmic_get_battery_voltage();

	if (info->pdc.tcpc == NULL)
		return 0;

	if (info->chg1_data.thermal_charging_current_limit != -1)
		charging_current = info->chg1_data.thermal_charging_current_limit / 1000;

	info->pdc.pdc_max_watt = vbat * charging_current;

	chr_err("[%s]watt:%d vbat:%d c:%d=>\n", __func__,
		info->pdc.pdc_max_watt, vbat, charging_current);

	return info->pdc.pdc_max_watt;
}

int mtk_pdc_setup(struct charger_manager *info, int idx)
{
	int ret;
	struct mtk_pdc *pd = &info->pdc;

	if (info->pdc.tcpc == NULL)
		return -1;

	ret = tcpm_set_remote_power_cap(pd->tcpc, pd->cap.max_mv[idx], pd->cap.ma[idx]);

	chr_err("[%s]idx:%d vbus:%d cur:%d ret:%d\n", __func__,
		idx, pd->cap.max_mv[idx], pd->cap.ma[idx], ret);


	return ret;
}

int mtk_pdc_get_setting(struct charger_manager *info, int *vbus, int *cur, int *idx)
{
	int i = 0;
	unsigned int max_watt;
	struct mtk_pdc *pd = &info->pdc;
	int min_vbus;

	if (info->pdc.tcpc == NULL)
		return -1;

	mtk_pdc_init_table(info);

	if (pd->cap.nr == 0)
		return -1;

	max_watt = mtk_pdc_get_max_watt(info);

	*vbus = pd->cap.max_mv[pd->cap.selected_cap_idx];
	*cur = pd->cap.ma[pd->cap.selected_cap_idx];
	*idx = pd->cap.selected_cap_idx;
	min_vbus = pd->cap.max_mv[pd->cap.selected_cap_idx];

	for (i = 0; i < pd->cap.nr; i++) {
		if (pd->cap.max_mv[i] != pd->cap.min_mv[i])
			continue;
		if (max_watt <= pd->cap.maxwatt[i] && max_watt >= pd->cap.minwatt[i]) {
			if (pd->cap.max_mv[i] < min_vbus) {
				*vbus = pd->cap.max_mv[i];
				*cur = pd->cap.ma[i];
				*idx = i;
			}
		}
	}

	chr_err("[%s]watt:%d vbus:%d current:%d idx:%d=>\n", __func__,
		info->pdc.pdc_max_watt, *vbus, *cur, *idx);

	return 0;
}

void mtk_pdc_init_table(struct charger_manager *info)
{
	struct tcpm_remote_power_cap cap;
	int i;
	struct mtk_pdc *pd = &info->pdc;

	if (info->pdc.tcpc == NULL)
		return;
	cap.nr = 0;
	if (mtk_is_pdc_ready(info)) {
		tcpm_get_remote_power_cap(info->pdc.tcpc, &cap);

		chr_err("[%s] nr=%d\n", __func__, cap.nr);
		if (cap.nr != 0) {
			pd->cap.nr = cap.nr;
			pd->cap.selected_cap_idx = cap.selected_cap_idx;
			for (i = 0; i < cap.nr; i++) {
				pd->cap.ma[i] = cap.ma[i];
				pd->cap.max_mv[i] = cap.max_mv[i];
				pd->cap.min_mv[i] = cap.min_mv[i];
				if (cap.max_mv[i] != cap.min_mv[i]) {
					pd->cap.maxwatt[i] = 0;
					pd->cap.minwatt[i] = 0;
				} else {
					pd->cap.maxwatt[i] = pd->cap.ma[i] * pd->cap.max_mv[i];
					pd->cap.minwatt[i] = 100 * pd->cap.max_mv[i];
				}
				chr_err("[%s]: %d mv:[%d,%d] %d max:%d min:%d\n",
					__func__, i, pd->cap.min_mv[i], pd->cap.max_mv[i],
					pd->cap.ma[i],
					pd->cap.maxwatt[i], pd->cap.minwatt[i]);
			}
		}
	} else
		chr_err("mtk_is_pdc_ready is fail\n");

#ifdef PDC_TEST
	for (i = 0; i < 35000000; i = i + 2000000) {
		int vbus, cur, idx;

		mtk_pdc_set_max_watt(i);
		mtk_pdc_get_setting(&vbus, &cur, &idx);
		chr_err("[%s]watt:%d,vbus:%d,cur:%d,idx:%d\n",
			__func__, pdc_max_watt, vbus, cur, idx);
	}
#endif

	chr_err("[%s]:=>%d\n", __func__, cap.nr);
}

bool mtk_pdc_init(struct charger_manager *info)
{
	info->pdc.tcpc = tcpc_dev_get_by_name("type_c_port0");
	return true;
}

