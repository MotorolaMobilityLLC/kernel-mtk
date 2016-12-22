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

#ifndef __MT_SPM_DPIDLE_MT6797__H__
#define __MT_SPM_DPIDLE_MT6797__H__

#include "mt_spm.h"

#if defined(CONFIG_MICROTRUST_TEE_SUPPORT)
#define WAKE_SRC_FOR_DPIDLE             \
	(WAKE_SRC_R12_KP_IRQ_B |            \
	WAKE_SRC_R12_PCM_TIMER |            \
	WAKE_SRC_R12_APXGPT1_EVENT_B |      \
	WAKE_SRC_R12_CONN2AP_SPM_WAKEUP_B | \
	WAKE_SRC_R12_EINT_EVENT_B |         \
	WAKE_SRC_R12_CONN_WDT_IRQ_B |       \
	WAKE_SRC_R12_CCIF0_EVENT_B |        \
	WAKE_SRC_R12_MD32_SPM_IRQ_B |       \
	WAKE_SRC_R12_USB0_CDSC_B_AND_USB1_CSDC_B |           \
	WAKE_SRC_R12_USB0_POWERDWN_B_AND_USB1_POWERDWN_B |       \
	WAKE_SRC_R12_C2K_WDT_IRQ_B |        \
	WAKE_SRC_R12_EINT_EVENT_SECURE_B |  \
	WAKE_SRC_R12_CCIF1_EVENT_B |        \
	WAKE_SRC_R12_AFE_IRQ_MCU_B |        \
	WAKE_SRC_R12_SYS_CIRQ_IRQ_B |       \
	WAKE_SRC_R12_MD1_WDT_B |            \
	WAKE_SRC_R12_CLDMA_EVENT_B)
#else
#define WAKE_SRC_FOR_DPIDLE             \
	(WAKE_SRC_R12_KP_IRQ_B |            \
	WAKE_SRC_R12_PCM_TIMER |            \
	WAKE_SRC_R12_APXGPT1_EVENT_B |      \
	WAKE_SRC_R12_CONN2AP_SPM_WAKEUP_B | \
	WAKE_SRC_R12_EINT_EVENT_B |         \
	WAKE_SRC_R12_CONN_WDT_IRQ_B |       \
	WAKE_SRC_R12_CCIF0_EVENT_B |        \
	WAKE_SRC_R12_MD32_SPM_IRQ_B |       \
	WAKE_SRC_R12_USB0_CDSC_B_AND_USB1_CSDC_B |           \
	WAKE_SRC_R12_USB0_POWERDWN_B_AND_USB1_POWERDWN_B |       \
	WAKE_SRC_R12_C2K_WDT_IRQ_B |        \
	WAKE_SRC_R12_EINT_EVENT_SECURE_B |  \
	WAKE_SRC_R12_CCIF1_EVENT_B |        \
	WAKE_SRC_R12_AFE_IRQ_MCU_B |        \
	WAKE_SRC_R12_SYS_CIRQ_IRQ_B |       \
	WAKE_SRC_R12_MD1_WDT_B |            \
	WAKE_SRC_R12_CLDMA_EVENT_B |        \
	WAKE_SRC_R12_SEJ_WDT_B_AND_SEJ_GPT_B)
#endif /* #if defined(CONFIG_MICROTRUST_TEE_SUPPORT) */

#endif /* __MT_SPM_DPIDLE_MT6797__H__ */

