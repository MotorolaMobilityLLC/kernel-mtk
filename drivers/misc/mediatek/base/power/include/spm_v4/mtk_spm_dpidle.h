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

#ifndef __MTK_SPM_DPIDLE_H__
#define __MTK_SPM_DPIDLE_H__

#include "mtk_spm.h"
#include "mtk_spm_internal.h"

#define __WAKE_SRC_FOR_DPIDLE__ \
	(WAKE_SRC_R12_PCM_TIMER | \
	WAKE_SRC_R12_SSPM_WDT_EVENT_B | \
	WAKE_SRC_R12_KP_IRQ_B | \
	WAKE_SRC_R12_APXGPT1_EVENT_B | \
	WAKE_SRC_R12_CONN2AP_SPM_WAKEUP_B | \
	WAKE_SRC_R12_EINT_EVENT_B | \
	WAKE_SRC_R12_CONN_WDT_IRQ_B | \
	WAKE_SRC_R12_CCIF0_EVENT_B | \
	WAKE_SRC_R12_SSPM_SPM_IRQ_B | \
	WAKE_SRC_R12_USB_CDSC_B | \
	WAKE_SRC_R12_USB_POWERDWN_B | \
	WAKE_SRC_R12_SYS_TIMER_EVENT_B | \
	WAKE_SRC_R12_EINT_EVENT_SECURE_B | \
	WAKE_SRC_R12_CCIF1_EVENT_B | \
	WAKE_SRC_R12_AFE_IRQ_MCU_B | \
	WAKE_SRC_R12_MD2AP_PEER_EVENT_B | \
	WAKE_SRC_R12_MD1_WDT_B | \
	WAKE_SRC_R12_CLDMA_EVENT_B)

#if defined(CONFIG_MICROTRUST_TEE_SUPPORT)
#define WAKE_SRC_FOR_DPIDLE \
	(__WAKE_SRC_FOR_DPIDLE__)
#else
#define WAKE_SRC_FOR_DPIDLE \
	(__WAKE_SRC_FOR_DPIDLE__ | \
	WAKE_SRC_R12_SEJ_WDT_GPT_B)
#endif /* #if defined(CONFIG_MICROTRUST_TEE_SUPPORT) */

extern void spm_dpidle_pre_process(unsigned int operation_cond, struct pwr_ctrl *pwrctrl);
extern void spm_dpidle_post_process(void);
extern void spm_deepidle_chip_init(void);
extern void spm_dpidle_notify_sspm_after_wfi_async_wait(void);

#endif /* __MTK_SPM_DPIDLE_H__ */

