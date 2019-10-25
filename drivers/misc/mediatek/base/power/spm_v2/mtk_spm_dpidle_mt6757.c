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

#include "mtk_spm_dpidle_mt6757.h"
#include "mtk_spm.h"
#include "mtk_spm_internal.h"
#include "mtk_spm_pmic_wrap.h"
#include <linux/of.h>
#include <linux/of_address.h>
#include <mt-plat/upmu_common.h>
#include <mtk_clkbuf_ctl.h>

#if defined(CONFIG_MTK_PMIC_CHIP_MT6355)
#include <mtk_pmic_api_buck.h>
#endif

static u32 ap_pll_con0_val;
static void __iomem *apmixedsys_base_in_dpidle;

#define APMIXED_REG(ofs) (apmixedsys_base_in_dpidle + ofs)

#define AP_PLL_CON0 APMIXED_REG(0x0)

/* #define PMIC_CLK_SRC_BY_SRCCLKEN_IN1 */
#if defined(CONFIG_FPGA_EARLY_PORTING)
static void spm_dpidle_pmic_before_wfi(void) {}

static void spm_dpidle_pmic_after_wfi(void) {}

#elif defined(CONFIG_MTK_PMIC_CHIP_MT6355)
static void spm_dpidle_pmic_before_wfi(void)
{
	u32 value = 0;

#ifdef POWER_DOWN_VPROC_VSRAM
	/* set PMIC wrap table for Vproc/Vsram voltage power down */
	pmic_read_interface_nolock(PMIC_RG_LDO_VSRAM_PROC_EN_ADDR, &value,
				   0xFFFF, 0);
	mt_spm_pmic_wrap_set_cmd_full(PMIC_WRAP_PHASE_DEEPIDLE,
				      IDX_DI_VSRAM_NORMAL,
				      PMIC_RG_LDO_VSRAM_PROC_EN_ADDR, 0x1);
	mt_spm_pmic_wrap_set_cmd_full(PMIC_WRAP_PHASE_DEEPIDLE,
				      IDX_DI_VSRAM_SLEEP,
				      PMIC_RG_LDO_VSRAM_PROC_EN_ADDR, 0x3);
	value = 0;
	pmic_read_interface_nolock(PMIC_RG_BUCK_VPROC11_EN_ADDR, &value, 0xFFFF,
				   0);
	mt_spm_pmic_wrap_set_cmd_full(PMIC_WRAP_PHASE_DEEPIDLE,
				      IDX_DI_VPROC_NORMAL,
				      PMIC_RG_BUCK_VPROC11_EN_ADDR, 0x1);
	mt_spm_pmic_wrap_set_cmd_full(PMIC_WRAP_PHASE_DEEPIDLE,
				      IDX_DI_VPROC_SLEEP,
				      PMIC_RG_BUCK_VPROC11_EN_ADDR, 0x3);
#else
	/* set PMIC wrap table for Vproc/Vsram voltage decreased */
	value = 0;
	pmic_read_interface_nolock(PMIC_RG_LDO_VSRAM_PROC_VOSEL_ADDR, &value,
				   PMIC_RG_LDO_VSRAM_PROC_VOSEL_MASK,
				   PMIC_RG_LDO_VSRAM_PROC_VOSEL_SHIFT);
	mt_spm_pmic_wrap_set_cmd_full(PMIC_WRAP_PHASE_DEEPIDLE,
				      IDX_DI_VSRAM_NORMAL,
				      PMIC_RG_LDO_VSRAM_PROC_VOSEL_ADDR, value);
	mt_spm_pmic_wrap_set_cmd_full(
	    PMIC_WRAP_PHASE_DEEPIDLE, IDX_DI_VSRAM_SLEEP,
	    PMIC_RG_LDO_VSRAM_PROC_VOSEL_ADDR, VOLT_TO_PMIC_VAL(60000));
	value = 0;
	pmic_read_interface_nolock(PMIC_RG_BUCK_VPROC11_VOSEL_ADDR, &value,
				   0xFFFF, 0);
	mt_spm_pmic_wrap_set_cmd_full(PMIC_WRAP_PHASE_DEEPIDLE,
				      IDX_DI_VPROC_NORMAL,
				      PMIC_RG_BUCK_VPROC11_VOSEL_ADDR, value);
	mt_spm_pmic_wrap_set_cmd_full(
	    PMIC_WRAP_PHASE_DEEPIDLE, IDX_DI_VPROC_SLEEP,
	    PMIC_RG_BUCK_VPROC11_VOSEL_ADDR, VOLT_TO_PMIC_VAL(60000));
#endif
	value = 0;
	pmic_read_interface_nolock(MT6355_TOP_SPI_CON0, &value, 0x037F, 0);
	mt_spm_pmic_wrap_set_cmd(PMIC_WRAP_PHASE_DEEPIDLE,
				 IDX_DI_SRCCLKEN_IN2_NORMAL,
				 value | (1 << PMIC_RG_SRCLKEN_IN2_EN_SHIFT));
	mt_spm_pmic_wrap_set_cmd(PMIC_WRAP_PHASE_DEEPIDLE,
				 IDX_DI_SRCCLKEN_IN2_SLEEP,
				 value & ~(1 << PMIC_RG_SRCLKEN_IN2_EN_SHIFT));

	__spm_pmic_low_iq_mode(1);
	__spm_pmic_pg_force_on();
	wk_auxadc_bgd_ctrl(0);

	if (is_md_c2k_conn_power_off())
		__spm_backup_pmic_ck_pdn();
}

static void spm_dpidle_pmic_after_wfi(void)
{
	if (is_md_c2k_conn_power_off())
		__spm_restore_pmic_ck_pdn();

	wk_auxadc_bgd_ctrl(1);
	__spm_pmic_pg_force_off();
	__spm_pmic_low_iq_mode(0);
}

#else
static void spm_dpidle_pmic_before_wfi(void)
{
	u32 value = 0;

	__spm_pmic_low_iq_mode(1);
	__spm_pmic_pg_force_on();
#ifdef POWER_DOWN_VPROC_VSRAM
	/* set PMIC wrap table for Vproc/Vsram voltage power down */
	pmic_read_interface_nolock(MT6351_PMIC_RG_VSRAM_PROC_EN_ADDR, &value,
				   0xFFFF, 0);
	mt_spm_pmic_wrap_set_cmd_full(
	    PMIC_WRAP_PHASE_DEEPIDLE, IDX_DI_VSRAM_NORMAL,
	    MT6351_PMIC_RG_VSRAM_PROC_EN_ADDR,
	    value | (1 << MT6351_PMIC_RG_VSRAM_PROC_EN_SHIFT));
	mt_spm_pmic_wrap_set_cmd_full(
	    PMIC_WRAP_PHASE_DEEPIDLE, IDX_DI_VSRAM_SLEEP,
	    MT6351_PMIC_RG_VSRAM_PROC_EN_ADDR,
	    value & ~(1 << MT6351_PMIC_RG_VSRAM_PROC_EN_SHIFT));
#else
	/* set PMIC wrap table for Vproc/Vsram voltage decreased */
	value = 0;
	pmic_read_interface_nolock(MT6351_PMIC_BUCK_VSRAM_PROC_VOSEL_ON_ADDR,
				   &value,
				   MT6351_PMIC_BUCK_VSRAM_PROC_VOSEL_ON_MASK,
				   MT6351_PMIC_BUCK_VSRAM_PROC_VOSEL_ON_SHIFT);
	mt_spm_pmic_wrap_set_cmd_full(
	    PMIC_WRAP_PHASE_DEEPIDLE, IDX_DI_VSRAM_NORMAL,
	    MT6351_PMIC_BUCK_VSRAM_PROC_VOSEL_ON_ADDR, value);
	mt_spm_pmic_wrap_set_cmd_full(
	    PMIC_WRAP_PHASE_DEEPIDLE, IDX_DI_VSRAM_SLEEP,
	    MT6351_PMIC_BUCK_VSRAM_PROC_VOSEL_ON_ADDR, VOLT_TO_PMIC_VAL(60000));
#endif

	value = 0;
	pmic_read_interface_nolock(MT6351_TOP_CON, &value, 0x037F, 0);
	mt_spm_pmic_wrap_set_cmd(
	    PMIC_WRAP_PHASE_DEEPIDLE, IDX_DI_SRCCLKEN_IN2_NORMAL,
	    value | (1 << MT6351_PMIC_RG_SRCLKEN_IN2_EN_SHIFT));
	mt_spm_pmic_wrap_set_cmd(
	    PMIC_WRAP_PHASE_DEEPIDLE, IDX_DI_SRCCLKEN_IN2_SLEEP,
	    value & ~(1 << MT6351_PMIC_RG_SRCLKEN_IN2_EN_SHIFT));

	/* set PMIC WRAP table for deepidle power control */
	mt_spm_pmic_wrap_set_phase(PMIC_WRAP_PHASE_DEEPIDLE);

	pmic_config_interface_nolock(
	    MT6351_PMIC_BUCK_VSRAM_MD_VOSEL_CTRL_ADDR, 1,
	    MT6351_PMIC_BUCK_VSRAM_MD_VOSEL_CTRL_MASK,
	    MT6351_PMIC_BUCK_VSRAM_MD_VOSEL_CTRL_SHIFT);
	if (is_md_c2k_conn_power_off()) {
		__spm_backup_pmic_ck_pdn();
#if defined(PMIC_CLK_SRC_BY_SRCCLKEN_IN1)
		pmic_config_interface_nolock(MT6351_BUCK_ALL_CON2, 0x111, 0x3FF,
					     0);
#endif
	}
}

static void spm_dpidle_pmic_after_wfi(void)
{
	if (is_md_c2k_conn_power_off()) {
#if defined(PMIC_CLK_SRC_BY_SRCCLKEN_IN1)
		pmic_config_interface_nolock(MT6351_BUCK_ALL_CON2, 0x0, 0x3FF,
					     0);
#endif
		__spm_restore_pmic_ck_pdn();
	}
	__spm_pmic_pg_force_off();
	__spm_pmic_low_iq_mode(0);
}
#endif

void spm_dpidle_pre_process(void)
{
	spm_pmic_power_mode(PMIC_PWR_DEEPIDLE, 0, 0);

	spm_bypass_boost_gpio_set();
	spm_dpidle_pmic_before_wfi();

	/* Do more low power setting when MD1/C2K/CONN off */
	if (is_md_c2k_conn_power_off()) {
		__spm_bsi_top_init_setting();

		/* disable 26M clks: MIPID, MIPIC1, MIPIC0, MDPLLGP, SSUSB */
		ap_pll_con0_val = spm_read(AP_PLL_CON0);
		spm_write(AP_PLL_CON0, ap_pll_con0_val & (~0x18D0));
#if !defined(CONFIG_FPGA_EARLY_PORTING)
		clk_buf_control_bblpm(true);
#endif
	}
}

void spm_dpidle_post_process(void)
{
	/* Do more low power setting when MD1/C2K/CONN off */
	if (is_md_c2k_conn_power_off()) {
#if !defined(CONFIG_FPGA_EARLY_PORTING)
		clk_buf_control_bblpm(false);
#endif
		/* Enable 26M clks: MIPID, MIPIC1, MIPIC0, MDPLLGP, SSUSB */
		spm_write(AP_PLL_CON0, ap_pll_con0_val);
	}

	spm_dpidle_pmic_after_wfi();

	/* set PMIC WRAP table for normal power control */
	mt_spm_pmic_wrap_set_phase(PMIC_WRAP_PHASE_NORMAL);
}

static int __init get_base_from_node(const char *cmp, void __iomem **pbase,
				     int idx)
{
	struct device_node *node;

	node = of_find_compatible_node(NULL, NULL, cmp);

	if (!node) {
		pr_debug("node '%s' not found!\n", cmp);
		/* TODO: BUG() */
	}

	*pbase = of_iomap(node, idx);
	if (!(*pbase)) {
		pr_debug("node '%s' cannot iomap!\n", cmp);
		/* TODO: BUG() */
	}

	return 0;
}

void __init spm_deepidle_chip_init(void)
{
	get_base_from_node("mediatek,apmixed", &apmixedsys_base_in_dpidle, 0);
}
