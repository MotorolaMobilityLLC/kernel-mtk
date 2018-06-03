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

#include <linux/of.h>
#include <linux/of_address.h>

#include <mt-plat/upmu_common.h>

#include <mtk_spm.h>
#include <mtk_spm_idle.h>
#include <mtk_spm_internal.h>
#include <mtk_spm_pmic_wrap.h>
#include <mtk_pmic_api_buck.h>

void spm_dpidle_pre_process(unsigned int operation_cond, struct pwr_ctrl *pwrctrl)
{
#if 0
#ifndef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
	unsigned int value = 0;
	unsigned int vcore_lp_mode = 0;

	/* set PMIC wrap table for Vproc/Vsram voltage decreased */
	/* VSRAM_DVFS1 */
	pmic_read_interface_nolock(PMIC_RG_VSRAM_DVFS1_VOSEL_ADDR,
								&value,
								PMIC_RG_VSRAM_DVFS1_VOSEL_MASK,
								PMIC_RG_VSRAM_DVFS1_VOSEL_SHIFT);

	mt_spm_pmic_wrap_set_cmd(PMIC_WRAP_PHASE_ALLINONE,
								IDX_ALL_1_VSRAM_NORMAL,
								value);

	/* VSRAM_DVFS2 */
	pmic_read_interface_nolock(PMIC_RG_VSRAM_DVFS2_VOSEL_ADDR,
								&value,
								PMIC_RG_VSRAM_DVFS2_VOSEL_MASK,
								PMIC_RG_VSRAM_DVFS2_VOSEL_SHIFT);

	mt_spm_pmic_wrap_set_cmd(PMIC_WRAP_PHASE_ALLINONE,
								IDX_ALL_2_VSRAM_NORMAL,
								value);

	mt_spm_pmic_wrap_set_cmd(PMIC_WRAP_PHASE_ALLINONE,
								IDX_ALL_VCORE_SUSPEND,
								pwrctrl->vcore_volt_pmic_val);

	/* TODO: setup PMIC low power mode */
	/* spm_pmic_power_mode(PMIC_PWR_DEEPIDLE, 0, 0); */

	/* set PMIC WRAP table for deepidle power control */
	mt_spm_pmic_wrap_set_phase(PMIC_WRAP_PHASE_ALLINONE);

	vcore_lp_mode = !!(operation_cond & DEEPIDLE_OPT_VCORE_LP_MODE);
	pmic_config_interface_nolock(PMIC_RG_BUCK_VCORE_HW2_OP_EN_ADDR,
									vcore_lp_mode,
									PMIC_RG_BUCK_VCORE_HW2_OP_EN_MASK,
									PMIC_RG_BUCK_VCORE_HW2_OP_EN_SHIFT);

	wk_auxadc_bgd_ctrl(0);
#endif

	__spm_sync_pcm_flags(pwrctrl);
#endif
}

void spm_dpidle_post_process(void)
{
#if 0
#ifndef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
	/* set PMIC WRAP table for normal power control */
	mt_spm_pmic_wrap_set_phase(PMIC_WRAP_PHASE_ALLINONE);

	wk_auxadc_bgd_ctrl(1);
#endif
#endif
}

#if 0
static int __init get_base_from_node(
	const char *cmp, void __iomem **pbase, int idx)
{
	struct device_node *node;

	node = of_find_compatible_node(NULL, NULL, cmp);

	if (!node) {
		pr_err("node '%s' not found!\n", cmp);
		/* FIXME: */
		/* BUG(); */
	}

	*pbase = of_iomap(node, idx);
	if (!(*pbase)) {
		pr_err("node '%s' cannot iomap!\n", cmp);
		/* FIXME: */
		/* BUG(); */
	}

	return 0;
}
#endif

void spm_deepidle_chip_init(void)
{
}

