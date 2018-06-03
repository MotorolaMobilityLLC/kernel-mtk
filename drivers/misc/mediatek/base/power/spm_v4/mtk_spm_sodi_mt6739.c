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

#if defined(CONFIG_MTK_PMIC) || defined(CONFIG_MTK_PMIC_NEW_ARCH)
#include <mt-plat/upmu_common.h>
#endif

#include <mtk_spm.h>
#include <mtk_spm_idle.h>
#include <mtk_spm_internal.h>
#include <mtk_spm_pmic_wrap.h>
#include <mtk_pmic_api_buck.h>
#include <mtk_spm_sodi_cmdq.h>


/* NOTE: Debug only */
#if 0
/*
 * GCE write spm SW2SPM_MAILBOX_3 (0x100065F8)
 *     [15:4] : Key, must write 9CE
 *     [1]: CG mode
 *     [0]: Need Bus clock
 * Then polling SPM2SW_MAILBOX_3 (0x100065DC) = 0
 */

void exit_pd_by_cmdq(struct cmdqRecStruct *handler)
{
	/* Switch to CG mode */
	cmdqRecWrite(handler, 0x100065F8, 0x9ce2, 0xffff);
	/* Polling ack */
	cmdqRecPoll(handler, 0x100065DC, 0x0, ~0);
}

void enter_pd_by_cmdq(struct cmdqRecStruct *handler)
{
	/* Switch to PD mode */
	cmdqRecWrite(handler, 0x100065F8, 0x9ce0, 0xffff);
}
#endif

void spm_sodi_pre_process(struct pwr_ctrl *pwrctrl, u32 operation_cond)
{
#ifndef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
	spm_pmic_power_mode(PMIC_PWR_SODI, 0, 0);
#endif
}

void spm_sodi_post_process(void)
{

}

void spm_sodi3_pre_process(struct pwr_ctrl *pwrctrl, u32 operation_cond)
{
#ifndef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
	spm_pmic_power_mode(PMIC_PWR_SODI3, 0, 0);
#endif
}

void spm_sodi3_post_process(void)
{

}

