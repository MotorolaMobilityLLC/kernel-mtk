#ifndef __MT_SPM_DPIDLE_MT6755__H__
#define __MT_SPM_DPIDLE_MT6755__H__

#include "mt_spm.h"

#define WAKE_SRC_FOR_DPIDLE             \
	(WAKE_SRC_R12_KP_IRQ_B |            \
	WAKE_SRC_R12_APXGPT1_EVENT_B |      \
	WAKE_SRC_R12_EINT_EVENT_B |         \
	WAKE_SRC_R12_CCIF0_EVENT_B |        \
	WAKE_SRC_R12_USB_CDSC_B |           \
	WAKE_SRC_R12_USB_POWERDWN_B |       \
	WAKE_SRC_R12_C2K_WDT_IRQ_B |        \
	WAKE_SRC_R12_EINT_EVENT_SECURE_B |  \
	WAKE_SRC_R12_CCIF1_EVENT_B |        \
	WAKE_SRC_R12_AFE_IRQ_MCU_B |        \
	WAKE_SRC_R12_SYS_CIRQ_IRQ_B |       \
	WAKE_SRC_R12_MD1_WDT_B |            \
	WAKE_SRC_R12_CLDMA_EVENT_B |        \
	WAKE_SRC_R12_SEJ_WDT_GPT_B)

#endif /* __MT_SPM_DPIDLE_MT6755__H__ */

