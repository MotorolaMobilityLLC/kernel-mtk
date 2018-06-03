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

#include <linux/of.h>
#include <linux/of_address.h>

#if defined(CONFIG_MTK_PMIC) || defined(CONFIG_MTK_PMIC_NEW_ARCH)
#include <mt-plat/upmu_common.h>
#endif

#include <mtk_spm.h>
#include <mtk_spm_idle.h>
#include <mtk_spm_internal.h>
#include <mtk_spm_pmic_wrap.h>
#include <mtk_pmic_api_buck.h>
#include <mtk_spm_vcore_dvfs.h>

void spm_dpidle_pre_process(unsigned int operation_cond, struct pwr_ctrl *pwrctrl)
{
#ifndef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
	/* setup PMIC low power mode */
	spm_pmic_power_mode(PMIC_PWR_DEEPIDLE, 0, 0);
#endif

#if !defined(CONFIG_FPGA_EARLY_PORTING)
	/* vs1 normal voltage voter (AP and CONN) */
	if (spm_read(PWR_STATUS) & PWR_STATUS_CONN)
		pmic_set_register_value(PMIC_RG_BUCK_VS1_VOTER_EN_SET, 0x01 << 1);
	else
		pmic_set_register_value(PMIC_RG_BUCK_VS1_VOTER_EN_CLR, 0x01 << 1);

	pmic_set_register_value(PMIC_RG_BUCK_VS1_VOTER_EN_CLR, 0x01 << 0);
#endif

	if (__spm_get_md_srcclkena_setting())
		dvfsrc_mdsrclkena_control_nolock(0);

	__spm_sync_pcm_flags(pwrctrl);
}

void spm_dpidle_post_process(void)
{
#if !defined(CONFIG_FPGA_EARLY_PORTING)
	/* vs1 normal voltage voter (AP and CONN) */
	pmic_set_register_value(PMIC_RG_BUCK_VS1_VOTER_EN_SET, 0x01 << 0);
	pmic_set_register_value(PMIC_RG_BUCK_VS1_VOTER_EN_SET, 0x01 << 1);
#endif

	if (__spm_get_md_srcclkena_setting())
		dvfsrc_mdsrclkena_control_nolock(1);
}

void spm_deepidle_chip_init(void)
{
}

