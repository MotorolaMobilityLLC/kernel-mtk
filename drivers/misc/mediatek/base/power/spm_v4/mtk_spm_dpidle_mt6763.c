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
#include <mt-plat/mtk_secure_api.h>

#if defined(CONFIG_MTK_PMIC) || defined(CONFIG_MTK_PMIC_NEW_ARCH)
#include <mt-plat/upmu_common.h>
#endif

#include <mtk_spm.h>
#include <mtk_spm_idle.h>
#include <mtk_spm_internal.h>
#include <mtk_spm_pmic_wrap.h>
#include <mtk_pmic_api_buck.h>
#include <mtk_spm_vcore_dvfs.h>
#include <mtk_spm_resource_req_internal.h>

void spm_dpidle_pre_process(unsigned int operation_cond, struct pwr_ctrl *pwrctrl)
{
#ifndef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
	/* set PMIC WRAP table for deepidle power control */
	mt_spm_pmic_wrap_set_phase(PMIC_WRAP_PHASE_ALLINONE);

	/* setup PMIC low power mode */
	spm_pmic_power_mode(PMIC_PWR_DEEPIDLE, 0, 0);
#endif

	__spm_sync_pcm_flags(pwrctrl);

	dvfsrc_md_scenario_update(1);
}

void spm_dpidle_post_process(void)
{
	dvfsrc_md_scenario_update(0);

#ifndef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
	/* set PMIC WRAP table for normal power control */
	mt_spm_pmic_wrap_set_phase(PMIC_WRAP_PHASE_ALLINONE);
#endif
}

void spm_dpidle_pcm_setup_before_wfi(bool sleep_dpidle, u32 cpu, struct pcm_desc *pcmdesc,
		struct pwr_ctrl *pwrctrl, u32 operation_cond)
{
	unsigned int resource_usage = 0;

	spm_dpidle_pre_process(operation_cond, pwrctrl);

	/* Get SPM resource request and update reg_spm_xxx_req */
	resource_usage = (!sleep_dpidle) ? spm_get_resource_usage() : 0;

	mt_secure_call(MTK_SIP_KERNEL_SPM_DPIDLE_ARGS, pwrctrl->pcm_flags, resource_usage, 0);

	if (sleep_dpidle)
		mt_secure_call(MTK_SIP_KERNEL_SPM_SLEEP_DPIDLE_ARGS, pwrctrl->timer_val, pwrctrl->wake_src, 0);
}

void spm_deepidle_chip_init(void)
{
}

