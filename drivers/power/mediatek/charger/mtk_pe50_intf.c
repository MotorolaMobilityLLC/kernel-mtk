/*
 * Copyright (C) 2019 MediaTek Inc.
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

#include <mt-plat/prop_chgalgo_class.h>
#include "mtk_charger_intf.h"
#include "mtk_switch_charging.h"

int mtk_pe50_init(struct charger_manager *chgmgr)
{
	int ret;
	struct mtk_pe50 *pe50 = &chgmgr->pe5;

	if (!chgmgr->enable_pe_5)
		return -ENOTSUPP;

	pe50->pca_algo = prop_chgalgo_dev_get_by_name("pca_algo_dv2");
	if (!pe50->pca_algo) {
		chr_err("[PE50] Get pca_algo fail\n");
		ret = -EINVAL;
	} else {
		ret = prop_chgalgo_init_algo(pe50->pca_algo);
		if (ret < 0) {
			chr_err("[PE50] Init algo fail\n");
			pe50->pca_algo = NULL;
		}
	}
	return ret;
}

bool mtk_pe50_is_ready(struct charger_manager *chgmgr)
{
	struct mtk_pe50 *pe50 = &chgmgr->pe5;

	if (!chgmgr->enable_pe_5)
		return false;
	if (chgmgr->chr_type != STANDARD_CHARGER)
		return false;
	if (!prop_chgalgo_is_algo_ready(pe50->pca_algo))
		return false;
	return true;
}

int mtk_pe50_start(struct charger_manager *chgmgr)
{
	int ret;
	struct mtk_pe50 *pe50 = &chgmgr->pe5;

	charger_enable_vbus_ovp(chgmgr, false);
	ret = prop_chgalgo_start_algo(pe50->pca_algo);
	if (ret < 0)
		charger_enable_vbus_ovp(chgmgr, true);
	return ret;
}

bool mtk_pe50_is_running(struct charger_manager *chgmgr)
{
	bool running;
	struct mtk_pe50 *pe50 = &chgmgr->pe5;

	running = prop_chgalgo_is_algo_running(pe50->pca_algo);
	if (!running)
		charger_enable_vbus_ovp(chgmgr, true);
	return running;
}

int mtk_pe50_plugout_reset(struct charger_manager *chgmgr)
{
	int ret;
	struct mtk_pe50 *pe50 = &chgmgr->pe5;
	struct switch_charging_alg_data *swchgalg = chgmgr->algorithm_data;

	ret = prop_chgalgo_plugout_reset(pe50->pca_algo);
	swchgalg->state = CHR_CC;
	charger_enable_vbus_ovp(chgmgr, true);
	return ret;
}
