/*
 * Copyright (C) 2017 MediaTek Inc.
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
#include <linux/of_address.h>
#include <mt-plat/aee.h>	/* aee_xxx */

#ifdef CONFIG_MTK_DRAMC
#include <mtk_dramc.h>
#endif /* CONFIG_MTK_DRAMC */

#include <mtk_spm_internal.h>

int spmfw_idx = -1;

#ifdef CONFIG_MTK_DRAMC
/***********************************************************
 * SPM Golden Seting API(MEMPLL Control, DRAMC)
 ***********************************************************/
static struct ddrphy_golden_cfg ddrphy_setting_lp4_2ch[] = {
	{DRAMC_AO_CHA, 0x038, 0xc0000027, 0xc0000007},
	{DRAMC_AO_CHB, 0x038, 0xc0000027, 0xc0000007},
	{PHY_AO_CHA, 0x0284, 0x001bff00, 0x00000100},
	{PHY_AO_CHB, 0x0284, 0x001bff00, 0x00000100},
	{PHY_AO_CHA, 0x0c20, 0xfff80000, 0x00200000},
	{PHY_AO_CHB, 0x0c20, 0xfff80000, 0x00200000},
	{PHY_AO_CHA, 0x1120, 0xfff80000, 0x00200000},
	{PHY_AO_CHB, 0x1120, 0xfff80000, 0x00200000},
	{PHY_AO_CHA, 0x1620, 0xfff80000, 0x00200000},
	{PHY_AO_CHB, 0x1620, 0xfff80000, 0x00200000},
	{PHY_AO_CHA, 0x0ca0, 0xfff80000, 0x00200000},
	{PHY_AO_CHB, 0x0ca0, 0xfff80000, 0x00200000},
	{PHY_AO_CHA, 0x11a0, 0xfff80000, 0x00200000},
	{PHY_AO_CHB, 0x11a0, 0xfff80000, 0x00200000},
	{PHY_AO_CHA, 0x16a0, 0xfff80000, 0x00200000},
	{PHY_AO_CHB, 0x16a0, 0xfff80000, 0x00200000},
	{PHY_AO_CHA, 0x0d20, 0xfff80000, 0x00000000},
	{PHY_AO_CHB, 0x0d20, 0xfff80000, 0x00000000},
	{PHY_AO_CHA, 0x1220, 0xfff80000, 0x00000000},
	{PHY_AO_CHB, 0x1220, 0xfff80000, 0x00000000},
	{PHY_AO_CHA, 0x1720, 0xfff80000, 0x00000000},
	{PHY_AO_CHB, 0x1720, 0xfff80000, 0x00000000},
	{PHY_AO_CHA, 0x0298, 0x00770000, 0x00770000},
	{PHY_AO_CHB, 0x0298, 0x00770000, 0x00770000},
	{PHY_AO_CHA, 0x02a8, 0x0c000000, 0x00000000},
	{PHY_AO_CHB, 0x02a8, 0x0c000000, 0x00000000},
	{PHY_AO_CHA, 0x028c, 0xffffffff, 0x806003be},
	{PHY_AO_CHB, 0x028c, 0xffffffff, 0x806003be},
	{PHY_AO_CHA, 0x0084, 0x00100000, 0x00000000},
	{PHY_AO_CHB, 0x0084, 0x00100000, 0x00000000},
	{PHY_AO_CHA, 0x0104, 0x00100000, 0x00000000},
	{PHY_AO_CHB, 0x0104, 0x00100000, 0x00000000},
	{PHY_AO_CHA, 0x0184, 0x00100000, 0x00000000},
	{PHY_AO_CHB, 0x0184, 0x00100000, 0x00000000},
	{PHY_AO_CHA, 0x0c34, 0x00000001, 0x00000001},
	{PHY_AO_CHB, 0x0c34, 0x00000001, 0x00000001},
	{PHY_AO_CHA, 0x0cb4, 0x00000001, 0x00000001},
	{PHY_AO_CHB, 0x0cb4, 0x00000001, 0x00000001},
	{PHY_AO_CHA, 0x0d34, 0x00000001, 0x00000001},
	{PHY_AO_CHB, 0x0d34, 0x00000001, 0x00000001},
	{PHY_AO_CHA, 0x1134, 0x00000001, 0x00000001},
	{PHY_AO_CHB, 0x1134, 0x00000001, 0x00000001},
	{PHY_AO_CHA, 0x11b4, 0x00000001, 0x00000001},
	{PHY_AO_CHB, 0x11b4, 0x00000001, 0x00000001},
	{PHY_AO_CHA, 0x1234, 0x00000001, 0x00000001},
	{PHY_AO_CHB, 0x1234, 0x00000001, 0x00000001},
	{PHY_AO_CHA, 0x1634, 0x00000001, 0x00000001},
	{PHY_AO_CHB, 0x1634, 0x00000001, 0x00000001},
	{PHY_AO_CHA, 0x16b4, 0x00000001, 0x00000001},
	{PHY_AO_CHB, 0x16b4, 0x00000001, 0x00000001},
	{PHY_AO_CHA, 0x1734, 0x00000001, 0x00000001},
	{PHY_AO_CHB, 0x1734, 0x00000001, 0x00000001},
	{PHY_AO_CHA, 0x0c1c, 0x000e0000, 0x000e0000},
	{PHY_AO_CHB, 0x0c1c, 0x000e0000, 0x000e0000},
	{PHY_AO_CHA, 0x0c9c, 0x000e0000, 0x000e0000},
	{PHY_AO_CHB, 0x0c9c, 0x000e0000, 0x000e0000},
	{PHY_AO_CHA, 0x0d1c, 0x000e0000, 0x000e0000},
	{PHY_AO_CHB, 0x0d1c, 0x000e0000, 0x000e0000},
	{PHY_AO_CHA, 0x111c, 0x000e0000, 0x000e0000},
	{PHY_AO_CHB, 0x111c, 0x000e0000, 0x000e0000},
	{PHY_AO_CHA, 0x119c, 0x000e0000, 0x000e0000},
	{PHY_AO_CHB, 0x119c, 0x000e0000, 0x000e0000},
	{PHY_AO_CHA, 0x121c, 0x000e0000, 0x000e0000},
	{PHY_AO_CHB, 0x121c, 0x000e0000, 0x000e0000},
	{PHY_AO_CHA, 0x161c, 0x000e0000, 0x000e0000},
	{PHY_AO_CHB, 0x161c, 0x000e0000, 0x000e0000},
	{PHY_AO_CHA, 0x169c, 0x000e0000, 0x000e0000},
	{PHY_AO_CHB, 0x169c, 0x000e0000, 0x000e0000},
	{PHY_AO_CHA, 0x171c, 0x000e0000, 0x000e0000},
	{PHY_AO_CHB, 0x171c, 0x000e0000, 0x000e0000},
};

static struct ddrphy_golden_cfg ddrphy_setting_lp3_1ch[] = {
	{DRAMC_AO_CHA, 0x038, 0xc0000027, 0xc0000007},
	{DRAMC_AO_CHB, 0x038, 0xc0000027, 0xc0000007},
	{PHY_AO_CHA, 0x0284, 0x001bff00, 0x00000100},
	{PHY_AO_CHB, 0x0284, 0x001bff00, 0x00100000},
	{PHY_AO_CHA, 0x0c20, 0xfff80000, 0x00000000},
	{PHY_AO_CHB, 0x0c20, 0xfff80000, 0x00200000},
	{PHY_AO_CHA, 0x1120, 0xfff80000, 0x00000000},
	{PHY_AO_CHB, 0x1120, 0xfff80000, 0x00200000},
	{PHY_AO_CHA, 0x1160, 0xfff80000, 0x00000000},
	{PHY_AO_CHB, 0x1160, 0xfff80000, 0x00200000},
	{PHY_AO_CHA, 0x0ca0, 0xfff80000, 0x00200000},
	{PHY_AO_CHB, 0x0ca0, 0xfff80000, 0x00200000},
	{PHY_AO_CHA, 0x11a0, 0xfff80000, 0x00200000},
	{PHY_AO_CHB, 0x11a0, 0xfff80000, 0x00200000},
	{PHY_AO_CHA, 0x16a0, 0xfff80000, 0x00200000},
	{PHY_AO_CHB, 0x16a0, 0xfff80000, 0x00200000},
	{PHY_AO_CHA, 0x0d20, 0xfff80000, 0x00000000},
	{PHY_AO_CHB, 0x0d20, 0xfff80000, 0x00200000},
	{PHY_AO_CHA, 0x1220, 0xfff80000, 0x00000000},
	{PHY_AO_CHB, 0x1220, 0xfff80000, 0x00200000},
	{PHY_AO_CHA, 0x1720, 0xfff80000, 0x00000000},
	{PHY_AO_CHB, 0x1720, 0xfff80000, 0x00200000},
	{PHY_AO_CHA, 0x0298, 0x00770000, 0x00570000},
	{PHY_AO_CHB, 0x0298, 0x00770000, 0x00070000},
	{PHY_AO_CHA, 0x02a8, 0x0c000000, 0x00000000},
	{PHY_AO_CHB, 0x02a8, 0x0c000000, 0x00000000},
	{PHY_AO_CHA, 0x028c, 0xffffffff, 0x806003be},
	{PHY_AO_CHB, 0x028c, 0xffffffff, 0x806003be},
	{PHY_AO_CHA, 0x0084, 0x00100000, 0x00000000},
	{PHY_AO_CHB, 0x0084, 0x00100000, 0x00000000},
	{PHY_AO_CHA, 0x0104, 0x00100000, 0x00000000},
	{PHY_AO_CHB, 0x0104, 0x00100000, 0x00000000},
	{PHY_AO_CHA, 0x0184, 0x00100000, 0x00000000},
	{PHY_AO_CHB, 0x0184, 0x00100000, 0x00000000},
	{PHY_AO_CHA, 0x0c34, 0x00000001, 0x00000001},
	{PHY_AO_CHB, 0x0c34, 0x00000001, 0x00000001},
	{PHY_AO_CHA, 0x0cb4, 0x00000001, 0x00000001},
	{PHY_AO_CHB, 0x0cb4, 0x00000001, 0x00000001},
	{PHY_AO_CHA, 0x0d34, 0x00000001, 0x00000001},
	{PHY_AO_CHB, 0x0d34, 0x00000001, 0x00000001},
	{PHY_AO_CHA, 0x1134, 0x00000001, 0x00000001},
	{PHY_AO_CHB, 0x1134, 0x00000001, 0x00000001},
	{PHY_AO_CHA, 0x11b4, 0x00000001, 0x00000001},
	{PHY_AO_CHB, 0x11b4, 0x00000001, 0x00000001},
	{PHY_AO_CHA, 0x1234, 0x00000001, 0x00000001},
	{PHY_AO_CHB, 0x1234, 0x00000001, 0x00000001},
	{PHY_AO_CHA, 0x1634, 0x00000001, 0x00000001},
	{PHY_AO_CHB, 0x1634, 0x00000001, 0x00000001},
	{PHY_AO_CHA, 0x16b4, 0x00000001, 0x00000001},
	{PHY_AO_CHB, 0x16b4, 0x00000001, 0x00000001},
	{PHY_AO_CHA, 0x1734, 0x00000001, 0x00000001},
	{PHY_AO_CHB, 0x1734, 0x00000001, 0x00000001},
	{PHY_AO_CHA, 0x0c1c, 0x000e0000, 0x000e0000},
	{PHY_AO_CHB, 0x0c1c, 0x000e0000, 0x000e0000},
	{PHY_AO_CHA, 0x0c9c, 0x000e0000, 0x000e0000},
	{PHY_AO_CHB, 0x0c9c, 0x000e0000, 0x000e0000},
	{PHY_AO_CHA, 0x0d1c, 0x000e0000, 0x000e0000},
	{PHY_AO_CHB, 0x0d1c, 0x000e0000, 0x000e0000},
	{PHY_AO_CHA, 0x111c, 0x000e0000, 0x000e0000},
	{PHY_AO_CHB, 0x111c, 0x000e0000, 0x000e0000},
	{PHY_AO_CHA, 0x119c, 0x000e0000, 0x000e0000},
	{PHY_AO_CHB, 0x119c, 0x000e0000, 0x000e0000},
	{PHY_AO_CHA, 0x121c, 0x000e0000, 0x000e0000},
	{PHY_AO_CHB, 0x121c, 0x000e0000, 0x000e0000},
	{PHY_AO_CHA, 0x161c, 0x000e0000, 0x000e0000},
	{PHY_AO_CHB, 0x161c, 0x000e0000, 0x000e0000},
	{PHY_AO_CHA, 0x169c, 0x000e0000, 0x000e0000},
	{PHY_AO_CHB, 0x169c, 0x000e0000, 0x000e0000},
	{PHY_AO_CHA, 0x171c, 0x000e0000, 0x000e0000},
	{PHY_AO_CHB, 0x171c, 0x000e0000, 0x000e0000},
};


static int spm_dram_golden_setting_cmp(bool en)
{
	int i, ddrphy_num, r = 0;
	struct ddrphy_golden_cfg *ddrphy_setting;

	if (!en)
		return r;

	switch (spm_get_spmfw_idx()) {
	switch (__spm_get_dram_type()) {
	case SPMFW_LP4X_2CH_3733:
		ddrphy_setting = ddrphy_setting_lp4_2ch;
		ddrphy_num = ARRAY_SIZE(ddrphy_setting_lp4_2ch);
		break;
	case SPMFW_LP4X_2CH_3200:
		ddrphy_setting = ddrphy_setting_lp4_2ch;
		ddrphy_num = ARRAY_SIZE(ddrphy_setting_lp4_2ch);
		break;
	case SPMFW_LP3_1CH_1866:
		ddrphy_setting = ddrphy_setting_lp3_1ch;
		ddrphy_num = ARRAY_SIZE(ddrphy_setting_lp3_1ch);
		break;
	default:
		return r;
	}

	/*Compare Dramc Goldeing Setting */
	for (i = 0; i < ddrphy_num; i++) {
		u32 value;

		value = lpDram_Register_Read(ddrphy_setting[i].base,
					ddrphy_setting[i].offset);
		if ((value & ddrphy_setting[i].mask) !=
				ddrphy_setting[i].value) {
			spm_crit2(
				"dramc mismatch addr: 0x%.2x, offset: 0x%.3x, ",
				ddrphy_setting[i].base,
				ddrphy_setting[i].offset);
			spm_crit2(
				"mask: 0x%.8x, val: 0x%x, read: 0x%x\n",
				ddrphy_setting[i].mask,
				ddrphy_setting[i].value,
				value);

			r = -EPERM;
		}
	}

	return r;

}

static void spm_dram_type_check(void)
{
	int ddr_type = get_ddr_type();
	int emi_ch_num = get_emi_ch_num();

	if (ddr_type == TYPE_LPDDR4X && emi_ch_num == 2)
		spmfw_idx = SPMFW_LP4X_2CH;
	else if (ddr_type == TYPE_LPDDR4X && emi_ch_num == 1)
		spmfw_idx = SPMFW_LP4X_1CH;
	else if (ddr_type == TYPE_LPDDR3 && emi_ch_num == 1)
		spmfw_idx = SPMFW_LP3_1CH;
	pr_info("#@# %s(%d) __spmfw_idx 0x%x\n", __func__, __LINE__, spmfw_idx);
}
#endif /* CONFIG_MTK_DRAMC */

static void spm_phypll_mode_check(void)
{
	unsigned int val = spm_read(SPM_POWER_ON_VAL0);

	if ((val & (R0_SC_PHYPLL_MODE_SW_PCM | R0_SC_PHYPLL2_MODE_SW_PCM))
			!= R0_SC_PHYPLL_MODE_SW_PCM) {

		aee_kernel_warning(
			"SPM Warning",
			"Invalid SPM_POWER_ON_VAL0: 0x%08x\n",
			val);
	}
}

int spm_get_spmfw_idx(void)
{
	if (spmfw_idx == -1) {
		spmfw_idx++;
#ifdef CONFIG_MTK_DRAMC
		spm_dram_type_check();
#endif /* CONFIG_MTK_DRAMC */
	}

	return spmfw_idx;
}
EXPORT_SYMBOL(spm_get_spmfw_idx);

void spm_do_dram_config_check(void)
{
#if !defined(CONFIG_FPGA_EARLY_PORTING) && defined(CONFIG_MTK_DRAMC)
	if (spm_dram_golden_setting_cmp(1) != 0)
		aee_kernel_warning("SPM Warning",
			"dram golden setting mismach");
#endif /* CONFIG_MTK_DRAMC && !CONFIG_FPGA_EARLY_PORTING */

	spm_phypll_mode_check();
}


