/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/of.h>
#include <linux/of_address.h>

#include <mt-plat/upmu_common.h>

#include "mt_spm.h"
#include "mt_spm_internal.h"
#include "mt_spm_pmic_wrap.h"

static u32 ap_pll_con0_val;
static void __iomem *apmixedsys_base_in_dpidle;

#define APMIXED_REG(ofs)	(apmixedsys_base_in_dpidle + ofs)

#define AP_PLL_CON0			APMIXED_REG(0x0)

/* Early Porting */
void __attribute__((weak))  mt_spm_pmic_wrap_set_phase(enum pmic_wrap_phase_id phase)
{

}

/* #define PMIC_CLK_SRC_BY_SRCCLKEN_IN1 */

void spm_dpidle_pre_process(void)
{
	u32 value = 0;

	__spm_pmic_pg_force_on();

	spm_pmic_power_mode(PMIC_PWR_DEEPIDLE, 0, 0);

	spm_bypass_boost_gpio_set();

	value = 0;
	pmic_read_interface_nolock(MT6351_TOP_CON, &value, 0x037F, 0);
	mt_spm_pmic_wrap_set_cmd(PMIC_WRAP_PHASE_DEEPIDLE,
								IDX_DI_SRCCLKEN_IN2_NORMAL,
								value | (1 << MT6351_PMIC_RG_SRCLKEN_IN2_EN_SHIFT));
	mt_spm_pmic_wrap_set_cmd(PMIC_WRAP_PHASE_DEEPIDLE,
								IDX_DI_SRCCLKEN_IN2_SLEEP,
								value & ~(1 << MT6351_PMIC_RG_SRCLKEN_IN2_EN_SHIFT));

	value = 0;
	pmic_read_interface_nolock(MT6351_PMIC_RG_VCORE_VDIFF_ENLOWIQ_ADDR, &value, 0xFFFF, 0);
	mt_spm_pmic_wrap_set_cmd(PMIC_WRAP_PHASE_DEEPIDLE,
			IDX_DI_VCORE_LQ_EN,
			value | (1 << MT6351_PMIC_RG_VCORE_VDIFF_ENLOWIQ_SHIFT));
	mt_spm_pmic_wrap_set_cmd(PMIC_WRAP_PHASE_DEEPIDLE,
			IDX_DI_VCORE_LQ_DIS,
			value & ~(1 << MT6351_PMIC_RG_VCORE_VDIFF_ENLOWIQ_SHIFT));

	/* set PMIC WRAP table for deepidle power control */
	mt_spm_pmic_wrap_set_phase(PMIC_WRAP_PHASE_DEEPIDLE);

	/* Do more low power setting when MD1/C2K/CONN off */
	if (is_md_c2k_conn_power_off()) {
		__spm_bsi_top_init_setting();

		__spm_backup_pmic_ck_pdn();

		/* disable 26M clks: MIPID, MIPIC1, MIPIC0, MDPLLGP, SSUSB */
		ap_pll_con0_val = spm_read(AP_PLL_CON0);
		spm_write(AP_PLL_CON0, ap_pll_con0_val & (~0x18D0));
#if defined(PMIC_CLK_SRC_BY_SRCCLKEN_IN1)
		pmic_config_interface_nolock(MT6351_BUCK_ALL_CON2, 0x111, 0x3FF, 0);
#endif
	}

	__spm_pmic_low_iq_mode(1);
}

void spm_dpidle_post_process(void)
{
	__spm_pmic_low_iq_mode(0);

	/* Do more low power setting when MD1/C2K/CONN off */
	if (is_md_c2k_conn_power_off()) {
#if defined(PMIC_CLK_SRC_BY_SRCCLKEN_IN1)
		pmic_config_interface_nolock(MT6351_BUCK_ALL_CON2, 0x0, 0x3FF, 0);
#endif
		/* Enable 26M clks: MIPID, MIPIC1, MIPIC0, MDPLLGP, SSUSB */
		spm_write(AP_PLL_CON0, ap_pll_con0_val);

		__spm_restore_pmic_ck_pdn();
	}

	/* set PMIC WRAP table for normal power control */
	mt_spm_pmic_wrap_set_phase(PMIC_WRAP_PHASE_NORMAL);

	__spm_pmic_pg_force_off();
}

static int __init get_base_from_node(
	const char *cmp, void __iomem **pbase, int idx)
{
	struct device_node *node;

	node = of_find_compatible_node(NULL, NULL, cmp);

	if (!node) {
		pr_err("node '%s' not found!\n", cmp);
		/* TODO: BUG() */
	}

	*pbase = of_iomap(node, idx);
	if (!(*pbase)) {
		pr_err("node '%s' cannot iomap!\n", cmp);
		/* TODO: BUG() */
	}

	return 0;
}

void spm_deepidle_chip_init(void)
{
	get_base_from_node("mediatek,apmixed", &apmixedsys_base_in_dpidle, 0);
}

