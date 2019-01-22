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

#ifndef __MTK_SPM_SODI_H__
#define __MTK_SPM_SODI_H__

#include <mtk_cpuidle.h>
#include "mtk_spm_idle.h"
#include "mtk_spm_misc.h"
#include "mtk_spm_internal.h"
#include "mtk_spm_pmic_wrap.h"
#include "mtk_spm_misc.h"
#include "mtk_spm_internal.h"


#ifdef SPM_SODI_PROFILE_TIME
extern unsigned int	soidle_profile[4];
#endif

enum spm_sodi_step {
	SPM_SODI_ENTER = 0,
	SPM_SODI_ENTER_SSPM_ASYNC_IPI_BEFORE_WFI,
	SPM_SODI_ENTER_SPM_FLOW,
	SPM_SODI_ENTER_UART_SLEEP,
	SPM_SODI_B4,
	SPM_SODI_B5,
	SPM_SODI_B6,
	SPM_SODI_ENTER_WFI,
	SPM_SODI_LEAVE_WFI,
	SPM_SODI_ENTER_UART_AWAKE,
	SPM_SODI_LEAVE_SSPM_ASYNC_IPI_AFTER_WFI,
	SPM_SODI_LEAVE_SPM_FLOW,
	SPM_SODI_LEAVE,
};

#if SPM_AEE_RR_REC
void __attribute__((weak)) aee_rr_rec_sodi_val(u32 val)
{
}

u32 __attribute__((weak)) aee_rr_curr_sodi_val(void)
{
	return 0;
}
#endif

static inline void spm_sodi_footprint(enum spm_sodi_step step)
{
#if SPM_AEE_RR_REC
	aee_rr_rec_sodi_val(aee_rr_curr_sodi_val() | (1 << step));
#endif
}

static inline void spm_sodi_footprint_val(u32 val)
{
#if SPM_AEE_RR_REC
		aee_rr_rec_sodi_val(aee_rr_curr_sodi_val() | val);
#endif
}

static inline void spm_sodi_aee_init(void)
{
#if SPM_AEE_RR_REC
	aee_rr_rec_sodi_val(0);
#endif
}

#define spm_sodi_reset_footprint() spm_sodi_aee_init()



#endif /* __MTK_SPM_SODI_H__ */

