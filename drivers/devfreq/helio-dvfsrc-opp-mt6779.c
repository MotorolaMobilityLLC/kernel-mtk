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

#include <linux/kernel.h>

#include <mt-plat/mtk_devinfo.h>
//#include <mtk_spm_internal.h>

#include "helio-dvfsrc-opp.h"
#ifdef CONFIG_MTK_DRAMC
#include <mtk_dramc.h>
#endif

int __weak dram_steps_freq(unsigned int step)
{
	pr_info("get dram steps_freq fail\n");
	return -1;
}

static int get_opp_min_ct(void)
{
	int opp_min_ct;

	opp_min_ct = 5;

	return opp_min_ct;
}

static int get_vb_volt(int vcore_opp)
{
	int ret = 0;
	int idx, idx2;
	int opp_min_ct = get_opp_min_ct();
	int ptpod = get_devinfo_with_index(63);

	pr_info("%s: PTPOD: 0x%x\n", __func__, ptpod);

	switch (vcore_opp) {
	case VCORE_OPP_0:
		idx = (ptpod >> 6) & 0x7;
		if (idx >= opp_min_ct)
			ret = 1;
		break;
	case VCORE_OPP_1:
		idx = (ptpod >> 6) & 0x7;
		idx2 = (ptpod >> 3) & 0x7;
		if ((idx >= opp_min_ct) && (idx2 >= opp_min_ct))
			ret = 1;
		break;
	case VCORE_OPP_2:
		idx = ptpod & 0x7;
		if (idx >= opp_min_ct)
			ret = 1;
		break;
	default:
		break;
	}
	return ret * 25000;

}

int ddr_level_to_step(int opp)
{
	unsigned int step[] = {0, 8, 6, 4, 2, 7};
	return step[opp];
}

void dvfsrc_opp_level_mapping(void)
{
	set_vcore_opp(VCORE_DVFS_OPP_0, VCORE_OPP_0);
	set_vcore_opp(VCORE_DVFS_OPP_1, VCORE_OPP_0);
	set_vcore_opp(VCORE_DVFS_OPP_2, VCORE_OPP_0);
	set_vcore_opp(VCORE_DVFS_OPP_3, VCORE_OPP_0);
	set_vcore_opp(VCORE_DVFS_OPP_4, VCORE_OPP_0);
	set_vcore_opp(VCORE_DVFS_OPP_5, VCORE_OPP_1);
	set_vcore_opp(VCORE_DVFS_OPP_6, VCORE_OPP_1);
	set_vcore_opp(VCORE_DVFS_OPP_7, VCORE_OPP_1);
	set_vcore_opp(VCORE_DVFS_OPP_8, VCORE_OPP_1);
	set_vcore_opp(VCORE_DVFS_OPP_9, VCORE_OPP_2);
	set_vcore_opp(VCORE_DVFS_OPP_10, VCORE_OPP_2);
	set_vcore_opp(VCORE_DVFS_OPP_11, VCORE_OPP_2);
	set_vcore_opp(VCORE_DVFS_OPP_12, VCORE_OPP_2);

	set_ddr_opp(VCORE_DVFS_OPP_0, DDR_OPP_0);
	set_ddr_opp(VCORE_DVFS_OPP_1, DDR_OPP_1);
	set_ddr_opp(VCORE_DVFS_OPP_2, DDR_OPP_2);
	set_ddr_opp(VCORE_DVFS_OPP_3, DDR_OPP_3);
	set_ddr_opp(VCORE_DVFS_OPP_4, DDR_OPP_4);
	set_ddr_opp(VCORE_DVFS_OPP_5, DDR_OPP_1);
	set_ddr_opp(VCORE_DVFS_OPP_6, DDR_OPP_2);
	set_ddr_opp(VCORE_DVFS_OPP_7, DDR_OPP_3);
	set_ddr_opp(VCORE_DVFS_OPP_8, DDR_OPP_4);
	set_ddr_opp(VCORE_DVFS_OPP_9, DDR_OPP_2);
	set_ddr_opp(VCORE_DVFS_OPP_10, DDR_OPP_3);
	set_ddr_opp(VCORE_DVFS_OPP_11, DDR_OPP_4);
	set_ddr_opp(VCORE_DVFS_OPP_12, DDR_OPP_5);
}

void dvfsrc_opp_table_init(void)
{
	int i;
	int vcore_opp, ddr_opp;

	for (i = 0; i < VCORE_DVFS_OPP_NUM; i++) {
		vcore_opp = get_vcore_opp(i);
		ddr_opp = get_ddr_opp(i);

		if (vcore_opp == VCORE_OPP_UNREQ || ddr_opp == DDR_OPP_UNREQ) {
			set_opp_table(i, 0, 0);
			continue;
		}
		set_opp_table(i, get_vcore_uv_table(vcore_opp),
		dram_steps_freq(ddr_level_to_step(ddr_opp)) * 1000);
	}
}

static int __init dvfsrc_opp_init(void)
{
	int vcore_opp_0_uv, vcore_opp_1_uv, vcore_opp_2_uv;
	int is_vcore_ct = 1;
	int i;

	set_pwrap_cmd(VCORE_OPP_0, 3);
	set_pwrap_cmd(VCORE_OPP_1, 1);
	set_pwrap_cmd(VCORE_OPP_2, 0);

	if (is_vcore_ct) {
		vcore_opp_0_uv = 825000 - get_vb_volt(VCORE_OPP_0);
		vcore_opp_1_uv = 725000 - get_vb_volt(VCORE_OPP_1);
		vcore_opp_2_uv = 650000 - get_vb_volt(VCORE_OPP_2);
	} else {
		vcore_opp_0_uv = 825000;
		vcore_opp_1_uv = 725000;
		vcore_opp_2_uv = 650000;
	}

	pr_info("%s: CT=%d, FINAL vcore_opp_uv: %d, %d, %d\n",
			__func__,
			is_vcore_ct,
			vcore_opp_0_uv,
			vcore_opp_1_uv,
			vcore_opp_2_uv);

	set_vcore_uv_table(VCORE_OPP_0, vcore_opp_0_uv);
	set_vcore_uv_table(VCORE_OPP_1, vcore_opp_1_uv);
	set_vcore_uv_table(VCORE_OPP_2, vcore_opp_2_uv);


	for (i = 0; i < DDR_OPP_NUM; i++) {
		set_opp_ddr_freq(i,
			dram_steps_freq(ddr_level_to_step(i)) * 1000);
	}

	return 0;
}

fs_initcall_sync(dvfsrc_opp_init)

