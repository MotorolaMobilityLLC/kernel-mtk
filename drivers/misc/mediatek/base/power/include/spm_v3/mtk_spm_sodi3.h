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

#ifndef __MTK_SPM_SODI3_H__
#define __MTK_SPM_SODI3_H__

#include <mtk_cpuidle.h>
#include <mtk_spm_idle.h>
#include <mtk_spm_misc.h>
#include <mtk_spm_internal.h>
#include <mtk_spm_pmic_wrap.h>
#include "mtk_spm_sodi.h"

#if defined(CONFIG_MACH_MT6799)

#if defined(CONFIG_MICROTRUST_TEE_SUPPORT)
#define WAKE_SRC_FOR_SODI3 \
	(WAKE_SRC_R12_PCMTIMER | \
	WAKE_SRC_R12_SSPM_WDT_EVENT_B | \
	WAKE_SRC_R12_KP_IRQ_B | \
	WAKE_SRC_R12_APXGPT1_EVENT_B | \
	WAKE_SRC_R12_SYS_TIMER_EVENT_B | \
	WAKE_SRC_R12_EINT_EVENT_B | \
	WAKE_SRC_R12_C2K_WDT_IRQ_B | \
	WAKE_SRC_R12_CCIF0_EVENT_B | \
	WAKE_SRC_R12_SSPM_SPM_IRQ_B | \
	WAKE_SRC_R12_SCP_IPC_MD2SPM_B | \
	WAKE_SRC_R12_SCP_WDT_EVENT_B | \
	WAKE_SRC_R12_USBX_CDSC_B | \
	WAKE_SRC_R12_USBX_POWERDWN_B | \
	WAKE_SRC_R12_EINT_EVENT_SECURE_B | \
	WAKE_SRC_R12_CCIF1_EVENT_B | \
	WAKE_SRC_R12_AFE_IRQ_MCU_B | \
	WAKE_SRC_R12_SCP_CIRQ_IRQ_B | \
	WAKE_SRC_R12_MD1_WDT_B | \
	WAKE_SRC_R12_MD2AP_PEER_WAKEUP_EVENT)
#else
#define WAKE_SRC_FOR_SODI3 \
	(WAKE_SRC_R12_PCMTIMER | \
	WAKE_SRC_R12_SSPM_WDT_EVENT_B | \
	WAKE_SRC_R12_KP_IRQ_B | \
	WAKE_SRC_R12_APXGPT1_EVENT_B | \
	WAKE_SRC_R12_SYS_TIMER_EVENT_B | \
	WAKE_SRC_R12_EINT_EVENT_B | \
	WAKE_SRC_R12_C2K_WDT_IRQ_B | \
	WAKE_SRC_R12_CCIF0_EVENT_B | \
	WAKE_SRC_R12_SSPM_SPM_IRQ_B | \
	WAKE_SRC_R12_SCP_IPC_MD2SPM_B | \
	WAKE_SRC_R12_SCP_WDT_EVENT_B | \
	WAKE_SRC_R12_USBX_CDSC_B | \
	WAKE_SRC_R12_USBX_POWERDWN_B | \
	WAKE_SRC_R12_EINT_EVENT_SECURE_B | \
	WAKE_SRC_R12_CCIF1_EVENT_B | \
	WAKE_SRC_R12_AFE_IRQ_MCU_B | \
	WAKE_SRC_R12_SCP_CIRQ_IRQ_B | \
	WAKE_SRC_R12_MD1_WDT_B | \
	WAKE_SRC_R12_MD2AP_PEER_WAKEUP_EVENT | \
	WAKE_SRC_R12_SEJ_EVENT_B)
#endif /* CONFIG_MICROTRUST_TEE_SUPPORT */

#elif defined(CONFIG_MACH_MT6759) || defined(CONFIG_MACH_MT6758)

#if defined(CONFIG_MICROTRUST_TEE_SUPPORT)
#define WAKE_SRC_FOR_SODI3 WAKE_SRC_FOR_COMMON_SODI
#else
#define WAKE_SRC_FOR_SODI3 (WAKE_SRC_FOR_COMMON_SODI | WAKE_SRC_R12_SEJ_EVENT_B)
#endif /* CONFIG_MICROTRUST_TEE_SUPPORT */

#else
#error "Does not support!"
#endif


#ifdef SPM_SODI3_PROFILE_TIME
extern unsigned int	soidle3_profile[4];
#endif

enum spm_sodi3_step {
	SPM_SODI3_ENTER = 0,
	SPM_SODI3_ENTER_UART_SLEEP,
	SPM_SODI3_ENTER_SPM_FLOW,
	SPM_SODI3_ENTER_SSPM_ASYNC_IPI_BEFORE_WFI,
	SPM_SODI3_B4,
	SPM_SODI3_B5,
	SPM_SODI3_B6,
	SPM_SODI3_ENTER_WFI,
	SPM_SODI3_LEAVE_WFI,
	SPM_SODI3_LEAVE_SSPM_ASYNC_IPI_AFTER_WFI,
	SPM_SODI3_LEAVE_SPM_FLOW,
	SPM_SODI3_ENTER_UART_AWAKE,
	SPM_SODI3_LEAVE,
};

#if SPM_AEE_RR_REC
void __attribute__((weak)) aee_rr_rec_sodi3_val(u32 val)
{
}

u32 __attribute__((weak)) aee_rr_curr_sodi3_val(void)
{
	return 0;
}

#endif

static inline void spm_sodi3_footprint(enum spm_sodi3_step step)
{
#if SPM_AEE_RR_REC
	aee_rr_rec_sodi3_val(aee_rr_curr_sodi3_val() | (1 << step));
#endif
}

static inline void spm_sodi3_footprint_val(u32 val)
{
#if SPM_AEE_RR_REC
		aee_rr_rec_sodi3_val(aee_rr_curr_sodi3_val() | val);
#endif
}

static inline void spm_sodi3_aee_init(void)
{
#if SPM_AEE_RR_REC
	aee_rr_rec_sodi3_val(0);
#endif
}

#define spm_sodi3_reset_footprint() spm_sodi3_aee_init()

#endif /* __MTK_SPM_SODI3_H__ */

