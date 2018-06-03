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
#ifndef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
	/* set PMIC WRAP table for deepidle power control */
	mt_spm_pmic_wrap_set_phase(PMIC_WRAP_PHASE_ALLINONE);

	/* setup PMIC low power mode */
	spm_pmic_power_mode(PMIC_PWR_DEEPIDLE, 0, 0);
#endif

	__spm_sync_pcm_flags(pwrctrl);

}

void spm_dpidle_post_process(void)
{
#ifndef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
	/* set PMIC WRAP table for normal power control */
	mt_spm_pmic_wrap_set_phase(PMIC_WRAP_PHASE_ALLINONE);
#endif
}

void spm_deepidle_chip_init(void)
{
}

