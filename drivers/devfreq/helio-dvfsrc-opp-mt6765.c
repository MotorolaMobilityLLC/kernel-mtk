/*
 * Copyright (C) 2018 MediaTek Inc.
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

#include <mt-plat/mtk_devinfo.h>
#include <mtk_spm_internal.h>

#include <mtk_dramc.h>

#include "helio-dvfsrc-opp.h"

int get_soc_efuse(void)
{
	return get_devinfo_with_index(60);
}

/* ToDo: Use efuse to adjust Vcore Voltage */
void vcore_volt_init(void)
{
	int i;
	int vcore_uv[VCORE_OPP_NUM];
	int vcore_opp, ddr_opp;

	switch (spm_get_spmfw_idx()) {
	case SPMFW_LP4X_2CH_3200:
		vcore_uv[0] = 800000;
		vcore_uv[1] = 700000;
		vcore_uv[2] = 700000;
		vcore_uv[3] = 650000;

		set_vcore_opp(VCORE_DVFS_OPP_0, VCORE_OPP_0);
		set_vcore_opp(VCORE_DVFS_OPP_1, VCORE_OPP_0);
		set_vcore_opp(VCORE_DVFS_OPP_2, VCORE_OPP_0);
		set_vcore_opp(VCORE_DVFS_OPP_3, VCORE_OPP_0);
		set_vcore_opp(VCORE_DVFS_OPP_4, VCORE_OPP_1);
		set_vcore_opp(VCORE_DVFS_OPP_5, VCORE_OPP_1);
		set_vcore_opp(VCORE_DVFS_OPP_6, VCORE_OPP_1);
		set_vcore_opp(VCORE_DVFS_OPP_7, VCORE_OPP_1);
		set_vcore_opp(VCORE_DVFS_OPP_8, VCORE_OPP_1);
		set_vcore_opp(VCORE_DVFS_OPP_9, VCORE_OPP_1);
		set_vcore_opp(VCORE_DVFS_OPP_10, VCORE_OPP_1);
		set_vcore_opp(VCORE_DVFS_OPP_11, VCORE_OPP_1);
		set_vcore_opp(VCORE_DVFS_OPP_12, VCORE_OPP_2);
		set_vcore_opp(VCORE_DVFS_OPP_13, VCORE_OPP_2);
		set_vcore_opp(VCORE_DVFS_OPP_14, VCORE_OPP_2);
		set_vcore_opp(VCORE_DVFS_OPP_15, VCORE_OPP_3);

		set_ddr_opp(VCORE_DVFS_OPP_0, DDR_OPP_0);
		set_ddr_opp(VCORE_DVFS_OPP_1, DDR_OPP_0);
		set_ddr_opp(VCORE_DVFS_OPP_2, DDR_OPP_0);
		set_ddr_opp(VCORE_DVFS_OPP_3, DDR_OPP_0);
		set_ddr_opp(VCORE_DVFS_OPP_4, DDR_OPP_0);
		set_ddr_opp(VCORE_DVFS_OPP_5, DDR_OPP_1);
		set_ddr_opp(VCORE_DVFS_OPP_6, DDR_OPP_0);
		set_ddr_opp(VCORE_DVFS_OPP_7, DDR_OPP_1);
		set_ddr_opp(VCORE_DVFS_OPP_8, DDR_OPP_2);
		set_ddr_opp(VCORE_DVFS_OPP_9, DDR_OPP_0);
		set_ddr_opp(VCORE_DVFS_OPP_10, DDR_OPP_1);
		set_ddr_opp(VCORE_DVFS_OPP_11, DDR_OPP_2);
		set_ddr_opp(VCORE_DVFS_OPP_12, DDR_OPP_2);
		set_ddr_opp(VCORE_DVFS_OPP_13, DDR_OPP_2);
		set_ddr_opp(VCORE_DVFS_OPP_14, DDR_OPP_2);
		set_ddr_opp(VCORE_DVFS_OPP_15, DDR_OPP_2);
		break;
	case SPMFW_LP3_1CH_1866:
		vcore_uv[0] = 800000;
		vcore_uv[1] = 700000;
		vcore_uv[2] = 700000;
		vcore_uv[3] = 650000;

		set_vcore_opp(VCORE_DVFS_OPP_0, VCORE_OPP_UNREQ);
		set_vcore_opp(VCORE_DVFS_OPP_1, VCORE_OPP_0);
		set_vcore_opp(VCORE_DVFS_OPP_2, VCORE_OPP_0);
		set_vcore_opp(VCORE_DVFS_OPP_3, VCORE_OPP_0);
		set_vcore_opp(VCORE_DVFS_OPP_4, VCORE_OPP_0);
		set_vcore_opp(VCORE_DVFS_OPP_5, VCORE_OPP_0);
		set_vcore_opp(VCORE_DVFS_OPP_6, VCORE_OPP_0);
		set_vcore_opp(VCORE_DVFS_OPP_7, VCORE_OPP_0);
		set_vcore_opp(VCORE_DVFS_OPP_8, VCORE_OPP_1);
		set_vcore_opp(VCORE_DVFS_OPP_9, VCORE_OPP_1);
		set_vcore_opp(VCORE_DVFS_OPP_10, VCORE_OPP_1);
		set_vcore_opp(VCORE_DVFS_OPP_11, VCORE_OPP_1);
		set_vcore_opp(VCORE_DVFS_OPP_12, VCORE_OPP_1);
		set_vcore_opp(VCORE_DVFS_OPP_13, VCORE_OPP_2);
		set_vcore_opp(VCORE_DVFS_OPP_14, VCORE_OPP_2);
		set_vcore_opp(VCORE_DVFS_OPP_15, VCORE_OPP_3);

		set_ddr_opp(VCORE_DVFS_OPP_0, DDR_OPP_UNREQ);
		set_ddr_opp(VCORE_DVFS_OPP_1, DDR_OPP_0);
		set_ddr_opp(VCORE_DVFS_OPP_2, DDR_OPP_0);
		set_ddr_opp(VCORE_DVFS_OPP_3, DDR_OPP_1);
		set_ddr_opp(VCORE_DVFS_OPP_4, DDR_OPP_0);
		set_ddr_opp(VCORE_DVFS_OPP_5, DDR_OPP_1);
		set_ddr_opp(VCORE_DVFS_OPP_6, DDR_OPP_0);
		set_ddr_opp(VCORE_DVFS_OPP_7, DDR_OPP_1);
		set_ddr_opp(VCORE_DVFS_OPP_8, DDR_OPP_1);
		set_ddr_opp(VCORE_DVFS_OPP_9, DDR_OPP_1);
		set_ddr_opp(VCORE_DVFS_OPP_10, DDR_OPP_2);
		set_ddr_opp(VCORE_DVFS_OPP_11, DDR_OPP_1);
		set_ddr_opp(VCORE_DVFS_OPP_12, DDR_OPP_2);
		set_ddr_opp(VCORE_DVFS_OPP_13, DDR_OPP_2);
		set_ddr_opp(VCORE_DVFS_OPP_14, DDR_OPP_2);
		set_ddr_opp(VCORE_DVFS_OPP_15, DDR_OPP_2);
		break;
	default:
		set_vcore_opp(VCORE_DVFS_OPP_0, VCORE_OPP_UNREQ);
		set_vcore_opp(VCORE_DVFS_OPP_1, VCORE_OPP_UNREQ);
		set_vcore_opp(VCORE_DVFS_OPP_2, VCORE_OPP_UNREQ);
		set_vcore_opp(VCORE_DVFS_OPP_3, VCORE_OPP_UNREQ);
		set_vcore_opp(VCORE_DVFS_OPP_4, VCORE_OPP_UNREQ);
		set_vcore_opp(VCORE_DVFS_OPP_5, VCORE_OPP_UNREQ);
		set_vcore_opp(VCORE_DVFS_OPP_6, VCORE_OPP_UNREQ);
		set_vcore_opp(VCORE_DVFS_OPP_7, VCORE_OPP_UNREQ);
		set_vcore_opp(VCORE_DVFS_OPP_8, VCORE_OPP_UNREQ);
		set_vcore_opp(VCORE_DVFS_OPP_9, VCORE_OPP_UNREQ);
		set_vcore_opp(VCORE_DVFS_OPP_10, VCORE_OPP_UNREQ);
		set_vcore_opp(VCORE_DVFS_OPP_11, VCORE_OPP_UNREQ);
		set_vcore_opp(VCORE_DVFS_OPP_12, VCORE_OPP_UNREQ);
		set_vcore_opp(VCORE_DVFS_OPP_13, VCORE_OPP_UNREQ);
		set_vcore_opp(VCORE_DVFS_OPP_14, VCORE_OPP_UNREQ);
		set_vcore_opp(VCORE_DVFS_OPP_15, VCORE_OPP_UNREQ);

		set_ddr_opp(VCORE_DVFS_OPP_0, DDR_OPP_UNREQ);
		set_ddr_opp(VCORE_DVFS_OPP_1, DDR_OPP_UNREQ);
		set_ddr_opp(VCORE_DVFS_OPP_2, DDR_OPP_UNREQ);
		set_ddr_opp(VCORE_DVFS_OPP_3, DDR_OPP_UNREQ);
		set_ddr_opp(VCORE_DVFS_OPP_4, DDR_OPP_UNREQ);
		set_ddr_opp(VCORE_DVFS_OPP_5, DDR_OPP_UNREQ);
		set_ddr_opp(VCORE_DVFS_OPP_6, DDR_OPP_UNREQ);
		set_ddr_opp(VCORE_DVFS_OPP_7, DDR_OPP_UNREQ);
		set_ddr_opp(VCORE_DVFS_OPP_8, DDR_OPP_UNREQ);
		set_ddr_opp(VCORE_DVFS_OPP_9, DDR_OPP_UNREQ);
		set_ddr_opp(VCORE_DVFS_OPP_10, DDR_OPP_UNREQ);
		set_ddr_opp(VCORE_DVFS_OPP_11, DDR_OPP_UNREQ);
		set_ddr_opp(VCORE_DVFS_OPP_12, DDR_OPP_UNREQ);
		set_ddr_opp(VCORE_DVFS_OPP_13, DDR_OPP_UNREQ);
		set_ddr_opp(VCORE_DVFS_OPP_14, DDR_OPP_UNREQ);
		set_ddr_opp(VCORE_DVFS_OPP_15, DDR_OPP_UNREQ);
		break;
	}

	for (i = 0; i < VCORE_DVFS_OPP_NUM; i++) {
		vcore_opp = get_vcore_opp(i);
		ddr_opp = get_ddr_opp(i);

		if (vcore_opp == VCORE_OPP_UNREQ || ddr_opp == DDR_OPP_UNREQ) {
			set_opp_table(i, 0, 0);
			continue;
		}
		set_opp_table(i, vcore_uv[vcore_opp],
				dram_steps_freq(ddr_opp) * 1000);
	}
}

