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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/of_fdt.h>
#include <linux/lockdep.h>
#include <linux/irqchip/mt-gic.h>

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#endif

#include <mach/wd_api.h>
#if defined(CONFIG_ARCH_MT6753)
#include <mach/mt_secure_api.h>
#endif
#include <mt-plat/upmu_common.h>
#include <mt-plat/mt_cirq.h>
#include <mt-plat/mt_ccci_common.h>

#include "mt_spm_idle.h"
#include "mt_cpufreq.h"
#include "mt_cpuidle.h"

#include "mt_spm_internal.h"

/*
 * only for internal debug
 */
#define DPIDLE_TAG     "[DP] "
#define dpidle_dbg(fmt, args...)	pr_debug(DPIDLE_TAG fmt, ##args)

#define SPM_PWAKE_EN            0
#define SPM_BYPASS_SYSPWREQ     1

#if defined(CONFIG_ARCH_MT6735)
#if defined(CONFIG_MICROTRUST_TEE_SUPPORT)
#define WAKE_SRC_FOR_DPIDLE \
	(WAKE_SRC_KP | WAKE_SRC_GPT | WAKE_SRC_EINT | WAKE_SRC_CONN_WDT | \
	WAKE_SRC_CCIF0_MD | WAKE_SRC_CCIF1_MD | WAKE_SRC_CONN2AP | WAKE_SRC_USB_CD | \
	WAKE_SRC_USB_PDN | WAKE_SRC_AFE | WAKE_SRC_CIRQ | WAKE_SRC_MD1_VRF18_WAKE | \
	WAKE_SRC_SYSPWREQ | WAKE_SRC_MD_WDT | WAKE_SRC_C2K_WDT | WAKE_SRC_CLDMA_MD)
#else /* CONFIG_MICROTRUST_TEE_SUPPORT */
#define WAKE_SRC_FOR_DPIDLE \
	(WAKE_SRC_KP | WAKE_SRC_GPT | WAKE_SRC_EINT | WAKE_SRC_CONN_WDT | \
	WAKE_SRC_CCIF0_MD | WAKE_SRC_CCIF1_MD | WAKE_SRC_CONN2AP | WAKE_SRC_USB_CD | \
	WAKE_SRC_USB_PDN | WAKE_SRC_SEJ | WAKE_SRC_AFE | WAKE_SRC_CIRQ | WAKE_SRC_MD1_VRF18_WAKE | \
	WAKE_SRC_SYSPWREQ | WAKE_SRC_MD_WDT | WAKE_SRC_C2K_WDT | WAKE_SRC_CLDMA_MD)
#endif /* !CONFIG_MICROTRUST_TEE_SUPPORT */

#elif defined(CONFIG_ARCH_MT6735M)
#if defined(CONFIG_MICROTRUST_TEE_SUPPORT)
#define WAKE_SRC_FOR_DPIDLE \
	(WAKE_SRC_KP | WAKE_SRC_GPT | WAKE_SRC_EINT | WAKE_SRC_CONN_WDT | \
	WAKE_SRC_CCIF0_MD | WAKE_SRC_CCIF1_MD | WAKE_SRC_CONN2AP | WAKE_SRC_USB_CD | \
	WAKE_SRC_USB_PDN | WAKE_SRC_AFE | WAKE_SRC_CIRQ | WAKE_SRC_MD1_VRF18_WAKE | \
	WAKE_SRC_SYSPWREQ | WAKE_SRC_MD_WDT | WAKE_SRC_CLDMA_MD)
#else /* CONFIG_MICROTRUST_TEE_SUPPORT */
#define WAKE_SRC_FOR_DPIDLE \
	(WAKE_SRC_KP | WAKE_SRC_GPT | WAKE_SRC_EINT | WAKE_SRC_CONN_WDT | \
	WAKE_SRC_CCIF0_MD | WAKE_SRC_CCIF1_MD | WAKE_SRC_CONN2AP | WAKE_SRC_USB_CD | \
	WAKE_SRC_USB_PDN | WAKE_SRC_SEJ | WAKE_SRC_AFE | WAKE_SRC_CIRQ | WAKE_SRC_MD1_VRF18_WAKE | \
	WAKE_SRC_SYSPWREQ | WAKE_SRC_MD_WDT | WAKE_SRC_CLDMA_MD)
#endif /* !CONFIG_MICROTRUST_TEE_SUPPORT */

#elif defined(CONFIG_ARCH_MT6753)
#if defined(CONFIG_MICROTRUST_TEE_SUPPORT)
#define WAKE_SRC_FOR_DPIDLE \
	(WAKE_SRC_KP | WAKE_SRC_GPT | WAKE_SRC_EINT | WAKE_SRC_CONN_WDT | \
	WAKE_SRC_CCIF0_MD | WAKE_SRC_CCIF1_MD | WAKE_SRC_CONN2AP | WAKE_SRC_USB_CD | \
	WAKE_SRC_USB_PDN | WAKE_SRC_AFE | WAKE_SRC_CIRQ | WAKE_SRC_MD1_VRF18_WAKE | \
	WAKE_SRC_SYSPWREQ | WAKE_SRC_MD_WDT | WAKE_SRC_C2K_WDT | WAKE_SRC_CLDMA_MD)
#else /* CONFIG_MICROTRUST_TEE_SUPPORT */
#define WAKE_SRC_FOR_DPIDLE \
	(WAKE_SRC_KP | WAKE_SRC_GPT | WAKE_SRC_EINT | WAKE_SRC_CONN_WDT | \
	WAKE_SRC_CCIF0_MD | WAKE_SRC_CCIF1_MD | WAKE_SRC_CONN2AP | WAKE_SRC_USB_CD | \
	WAKE_SRC_USB_PDN | WAKE_SRC_SEJ | WAKE_SRC_AFE | WAKE_SRC_CIRQ | WAKE_SRC_MD1_VRF18_WAKE | \
	WAKE_SRC_SYSPWREQ | WAKE_SRC_MD_WDT | WAKE_SRC_C2K_WDT | WAKE_SRC_CLDMA_MD)
#endif /* !CONFIG_MICROTRUST_TEE_SUPPORT */

#elif defined(CONFIG_ARCH_MT6570) || defined(CONFIG_ARCH_MT6580)
#define WAKE_SRC_FOR_DPIDLE \
	(WAKE_SRC_KP | WAKE_SRC_GPT | WAKE_SRC_EINT | WAKE_SRC_CONN_WDT| \
	WAKE_SRC_CCIF0_MD | WAKE_SRC_CONN2AP | WAKE_SRC_USB_CD | WAKE_SRC_USB_PDN | \
	WAKE_SRC_AFE | WAKE_SRC_SYSPWREQ | WAKE_SRC_MD1_WDT | WAKE_SRC_SEJ)
#else
#error "Does not support!"
#endif

#define WAKE_SRC_FOR_MD32  0

#define I2C_CHANNEL 2

#define spm_is_wakesrc_invalid(wakesrc)     (!!((u32)(wakesrc) & 0xc0003803))

#if defined(CONFIG_ARCH_MT6753)
#define reg_read(addr)         __raw_readl(IOMEM(addr))
#define reg_write(addr, val)   mt_reg_sync_writel((val), ((void *)addr))

#if defined(CONFIG_OF)
#define MCUCFG_NODE "mediatek,MCUCFG"
static unsigned long mcucfg_base;
static unsigned long mcucfg_phys_base;
#undef MCUCFG_BASE
#define MCUCFG_BASE             (mcucfg_base)

#else				/* #if defined(CONFIG_OF) */
#undef MCUCFG_BASE
#define MCUCFG_BASE             0xF0200000
#endif				/* #if defined(CONFIG_OF) */

/* MCUCFG registers */
#define MP0_AXI_CONFIG       (MCUCFG_BASE + 0x2C)
#define MP0_AXI_CONFIG_PHYS  (mcucfg_phys_base + 0x2C)
#define MP1_AXI_CONFIG       (MCUCFG_BASE + 0x22C)
#define MP1_AXI_CONFIG_PHYS  (mcucfg_phys_base + 0x22C)
#define ACINACTM                (1 << 4)

#if defined(CONFIG_ARM_PSCI) || defined(CONFIG_MTK_PSCI)
#define MCUSYS_SMC_WRITE(addr, val)  mcusys_smc_write_phy(addr##_PHYS, val)
#else
#define MCUSYS_SMC_WRITE(addr, val)  mcusys_smc_write(addr, val)
#endif
#endif				/* #if defined(CONFIG_ARCH_MT6753) */

#ifdef CONFIG_MTK_RAM_CONSOLE
#define SPM_AEE_RR_REC 1
#else
#define SPM_AEE_RR_REC 0
#endif
#define SPM_USE_TWAM_DEBUG	0

#define	DPIDLE_LOG_PRINT_TIMEOUT_CRITERIA	20
#define	DPIDLE_LOG_DISCARD_CRITERIA			5000	/* ms */

#if SPM_AEE_RR_REC
enum spm_deepidle_step {
	SPM_DEEPIDLE_ENTER = 0,
	SPM_DEEPIDLE_ENTER_UART_SLEEP,
	SPM_DEEPIDLE_ENTER_WFI,
	SPM_DEEPIDLE_LEAVE_WFI,
	SPM_DEEPIDLE_ENTER_UART_AWAKE,
	SPM_DEEPIDLE_LEAVE
};
#endif

#if defined(CONFIG_ARCH_MT6735)
static struct pwr_ctrl dpidle_ctrl = {
	.wake_src = WAKE_SRC_FOR_DPIDLE,
	.wake_src_md32 = WAKE_SRC_FOR_MD32,
	.r0_ctrl_en = 1,
	.r7_ctrl_en = 1,
	.infra_dcm_lock = 1,
	.wfi_op = WFI_OP_AND,
	.ca15_wfi0_en = 1,
	.ca15_wfi1_en = 1,
	.ca15_wfi2_en = 1,
	.ca15_wfi3_en = 1,
	.ca7_wfi0_en = 1,
	.ca7_wfi1_en = 1,
	.ca7_wfi2_en = 1,
	.ca7_wfi3_en = 1,
	.disp_req_mask = 1,
	.mfg_req_mask = 1,
	.lte_mask = 1,
	.syspwreq_mask = 1,
};
#elif defined(CONFIG_ARCH_MT6735M)
static struct pwr_ctrl dpidle_ctrl = {
	.wake_src = WAKE_SRC_FOR_DPIDLE,
	.wake_src_md32 = WAKE_SRC_FOR_MD32,
	.r0_ctrl_en = 1,
	.r7_ctrl_en = 1,
	.infra_dcm_lock = 1,
	.wfi_op = WFI_OP_AND,
	.ca15_wfi0_en = 1,
	.ca15_wfi1_en = 1,
	.ca15_wfi2_en = 1,
	.ca15_wfi3_en = 1,
	.ca7_wfi0_en = 1,
	.ca7_wfi1_en = 1,
	.ca7_wfi2_en = 1,
	.ca7_wfi3_en = 1,
	.md2_req_mask = 1,
	.disp_req_mask = 1,
	.mfg_req_mask = 1,
	.lte_mask = 1,
	.syspwreq_mask = 1,
};
#elif defined(CONFIG_ARCH_MT6753)
static struct pwr_ctrl dpidle_ctrl = {
	.wake_src = WAKE_SRC_FOR_DPIDLE,
	.wake_src_md32 = WAKE_SRC_FOR_MD32,
	.r0_ctrl_en = 1,
	.r7_ctrl_en = 1,
	.infra_dcm_lock = 1,
	.wfi_op = WFI_OP_AND,
	.ca15_wfi0_en = 1,
	.ca15_wfi1_en = 1,
	.ca15_wfi2_en = 1,
	.ca15_wfi3_en = 1,
	.ca7_wfi0_en = 1,
	.ca7_wfi1_en = 1,
	.ca7_wfi2_en = 1,
	.ca7_wfi3_en = 1,
	.disp_req_mask = 1,
	.mfg_req_mask = 1,
	.lte_mask = 1,
	.syspwreq_mask = 1,

	.pcm_apsrc_req = 0,
	.md1_req_mask = 0,
	.md2_req_mask = 0,
};
#elif defined(CONFIG_ARCH_MT6570) || defined(CONFIG_ARCH_MT6580)
/**********************************************************
 * PCM code for deep idle
 **********************************************************/
static const u32 dpidle_binary[] = {
	0x80318400, 0x80328400, 0xc2802e80, 0x1290041f, 0x1b00001f, 0x7fffd7ff,
	0xf0000000, 0x17c07c1f, 0x1b00001f, 0x3fffc7ff, 0x1b80001f, 0x20000004,
	0xd80002cc, 0x17c07c1f, 0xa0128400, 0xa0118400, 0xc2802e80, 0x1290841f,
	0xc2802e80, 0x129f041f, 0x1b00001f, 0x3fffcfff, 0xf0000000, 0x17c07c1f,
	0x81449801, 0xd8000405, 0x17c07c1f, 0x1a00001f, 0x10006604, 0xe2200000,
	0xc0c02da0, 0x17c07c1f, 0x81491801, 0xd8000565, 0x17c07c1f, 0x18c0001f,
	0x102085cc, 0x1910001f, 0x102085cc, 0x813f8404, 0xe0c00004, 0x1910001f,
	0x102085cc, 0x81411801, 0xd8000685, 0x17c07c1f, 0x18c0001f, 0x10006240,
	0xe0e00016, 0xe0e0001e, 0xe0e0000e, 0xe0e0000f, 0x803e0400, 0x1b80001f,
	0x20000222, 0x80380400, 0x1b80001f, 0x20000280, 0x803b0400, 0x1b80001f,
	0x2000001a, 0x803d0400, 0x1b80001f, 0x20000208, 0x80340400, 0x80310400,
	0x1b80001f, 0x2000000a, 0x18c0001f, 0x10006240, 0xe0e0000d, 0xd8000ca5,
	0x17c07c1f, 0x1b80001f, 0x20000020, 0x18c0001f, 0x102080f0, 0x1910001f,
	0x102080f0, 0xa9000004, 0x10000000, 0xe0c00004, 0x1b80001f, 0x2000000a,
	0x89000004, 0xefffffff, 0xe0c00004, 0x18c0001f, 0x102070f4, 0x1910001f,
	0x102070f4, 0xa9000004, 0x02000000, 0xe0c00004, 0x1b80001f, 0x2000000a,
	0x89000004, 0xfdffffff, 0xe0c00004, 0x1910001f, 0x102070f4, 0x81fa0407,
	0x81f08407, 0xe8208000, 0x10006354, 0xfffffc23, 0xa1d80407, 0xa1df0407,
	0xc2802e80, 0x1291041f, 0x81491801, 0xd8000f25, 0x17c07c1f, 0x18c0001f,
	0x102085cc, 0x1910001f, 0x102085cc, 0xa11f8404, 0xe0c00004, 0x1910001f,
	0x102085cc, 0x1b00001f, 0xbfffc7ff, 0xf0000000, 0x17c07c1f, 0x1b80001f,
	0x20000fdf, 0x1a50001f, 0x10006608, 0x80c9a401, 0x810ba401, 0x10920c1f,
	0xa0979002, 0x8080080d, 0xd82012a2, 0x17c07c1f, 0x81f08407, 0xa1d80407,
	0xa1df0407, 0x1b00001f, 0x3fffc7ff, 0x1b80001f, 0x20000004, 0xd8001b4c,
	0x17c07c1f, 0x1b00001f, 0xbfffc7ff, 0xd0001b40, 0x17c07c1f, 0x81f80407,
	0x81ff0407, 0x1880001f, 0x10006320, 0xc0c02640, 0xe080000f, 0xd8001103,
	0x17c07c1f, 0xe080001f, 0xa1da0407, 0x18c0001f, 0x110040d8, 0x1910001f,
	0x110040d8, 0xa11f8404, 0xe0c00004, 0x1910001f, 0x110040d8, 0x81491801,
	0xd8001645, 0x17c07c1f, 0x18c0001f, 0x102085cc, 0x1910001f, 0x102085cc,
	0x813f8404, 0xe0c00004, 0x1910001f, 0x102085cc, 0xa0110400, 0xa0140400,
	0xa0180400, 0xa01b0400, 0xa01d0400, 0x17c07c1f, 0x17c07c1f, 0xa01e0400,
	0x17c07c1f, 0x17c07c1f, 0x81491801, 0xd80018e5, 0x17c07c1f, 0x18c0001f,
	0x102085cc, 0x1910001f, 0x102085cc, 0xa11f8404, 0xe0c00004, 0x1910001f,
	0x102085cc, 0x81411801, 0xd80019c5, 0x17c07c1f, 0x18c0001f, 0x10006240,
	0xc0c025a0, 0x17c07c1f, 0x81449801, 0xd8001ac5, 0x17c07c1f, 0x1a00001f,
	0x10006604, 0xe2200001, 0xc0c02da0, 0x17c07c1f, 0xc2802e80, 0x1291841f,
	0x1b00001f, 0x7fffd7ff, 0xf0000000, 0x17c07c1f, 0x18c0001f, 0x10006b6c,
	0xe0f03eef, 0x80318400, 0x80328400, 0xa1de0407, 0x80390400, 0x1b80001f,
	0x20000300, 0x803c0400, 0x1b80001f, 0x20000300, 0x80350400, 0x1b80001f,
	0x20000104, 0x81f40407, 0x18c0001f, 0x111403a8, 0xe0c00001, 0x18c0001f,
	0x10006810, 0x1910001f, 0x10006810, 0x81308404, 0xe0c00004, 0x1b00001f,
	0x7fffcfff, 0xf0000000, 0x17c07c1f, 0x81fe0407, 0x18c0001f, 0x10006810,
	0x1910001f, 0x10006810, 0xa1108404, 0xe0c00004, 0x18c0001f, 0x10006b6c,
	0xe0f05ead, 0x1b00001f, 0xffffcfff, 0x80dd040d, 0xd82020a3, 0x17c07c1f,
	0xc0c02ba0, 0x17c07c1f, 0xa0150400, 0xa01c0400, 0xa0190400, 0xf0000000,
	0x17c07c1f, 0xe0f07f16, 0x1380201f, 0xe0f07f1e, 0x1380201f, 0xe0f07f0e,
	0x1b80001f, 0x20000100, 0xe0f07f0c, 0xe0f07f0d, 0xe0f07e0d, 0xe0f07c0d,
	0xe0f0780d, 0xe0f0700d, 0xf0000000, 0x17c07c1f, 0xe0f07f0d, 0xe0f07f0f,
	0xe0f07f1e, 0xe0f07f12, 0xf0000000, 0x17c07c1f, 0xe0e00016, 0x1380201f,
	0xe0e0001e, 0x1380201f, 0xe0e0000e, 0xe0e0000c, 0xe0e0000d, 0xf0000000,
	0x17c07c1f, 0xe0e0000f, 0xe0e0001e, 0xe0e00012, 0xf0000000, 0x17c07c1f,
	0x1112841f, 0xa1d08407, 0xd8202704, 0x80eab401, 0xd8002683, 0x01200404,
	0x1a00001f, 0x10006814, 0xe2000003, 0xf0000000, 0x17c07c1f, 0xa1d00407,
	0x1b80001f, 0x20000100, 0x80ea3401, 0x1a00001f, 0x10006814, 0xe2000003,
	0xf0000000, 0x17c07c1f, 0xd800298a, 0x17c07c1f, 0xe2e00036, 0xe2e0003e,
	0x1380201f, 0xe2e0003c, 0xd8202aca, 0x17c07c1f, 0x1b80001f, 0x20000018,
	0xe2e0007c, 0x1b80001f, 0x20000003, 0xe2e0005c, 0xe2e0004c, 0xe2e0004d,
	0xf0000000, 0x17c07c1f, 0xa1d10407, 0x1b80001f, 0x20000020, 0xf0000000,
	0x17c07c1f, 0xa1d40407, 0x1391841f, 0xf0000000, 0x17c07c1f, 0xd8002cca,
	0x17c07c1f, 0xe2e0004f, 0xe2e0006f, 0xe2e0002f, 0xd8202d6a, 0x17c07c1f,
	0xe2e0002e, 0xe2e0003e, 0xe2e00032, 0xf0000000, 0x17c07c1f, 0x18d0001f,
	0x10006604, 0x10cf8c1f, 0xd8202da3, 0x17c07c1f, 0xf0000000, 0x17c07c1f,
	0x18c0001f, 0x10006b18, 0x1910001f, 0x10006b18, 0xa1002804, 0xe0c00004,
	0xf0000000, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
	0x17c07c1f, 0x17c07c1f, 0x1840001f, 0x00000001, 0xa1d48407, 0x1990001f,
	0x10006b08, 0xe8208000, 0x10006b18, 0x00000000, 0x1b00001f, 0x3fffc7ff,
	0x1b80001f, 0xd00f0000, 0x8880000c, 0x3fffc7ff, 0xd8005522, 0x17c07c1f,
	0xe8208000, 0x10006354, 0xfffffc23, 0xc0c02b00, 0x81401801, 0xd80046c5,
	0x17c07c1f, 0x81f60407, 0x18c0001f, 0x10006200, 0xc0c02c20, 0x12807c1f,
	0xe8208000, 0x1000625c, 0x00000001, 0x1b80001f, 0x20000080, 0xc0c02c20,
	0x1280041f, 0x18c0001f, 0x10006208, 0xc0c02c20, 0x12807c1f, 0xe8208000,
	0x10006248, 0x00000000, 0x1b80001f, 0x20000080, 0xc0c02c20, 0x1280041f,
	0x18c0001f, 0x10006290, 0xc0c02c20, 0x12807c1f, 0xc0c02c20, 0x1280041f,
	0xc2802e80, 0x1292041f, 0x18c0001f, 0x10006294, 0xe0f07fff, 0xe0e00fff,
	0xe0e000ff, 0xc0c02ba0, 0x17c07c1f, 0xa1d38407, 0xa1d98407, 0x18c0001f,
	0x11004078, 0x1910001f, 0x11004078, 0xa11f8404, 0xe0c00004, 0x1910001f,
	0x11004078, 0x18c0001f, 0x11004098, 0x1910001f, 0x11004098, 0xa11f8404,
	0xe0c00004, 0x1910001f, 0x11004098, 0xa0108400, 0xa0120400, 0xa0148400,
	0xa0150400, 0xa0158400, 0xa01b8400, 0xa01c0400, 0xa01c8400, 0xa0188400,
	0xa0190400, 0xa0198400, 0xe8208000, 0x10006310, 0x0b1600f8, 0x1b00001f,
	0xbfffc7ff, 0x1b80001f, 0x90100000, 0x80c18001, 0xc8c00003, 0x17c07c1f,
	0x80c10001, 0xc8c00303, 0x17c07c1f, 0x1b00001f, 0x3fffc7ff, 0x18c0001f,
	0x10006294, 0xe0e001fe, 0xe0e003fc, 0xe0e007f8, 0xe0e00ff0, 0x1b80001f,
	0x20000020, 0xe0f07ff0, 0xe0f07f00, 0x80388400, 0x80390400, 0x80398400,
	0x1b80001f, 0x20000300, 0x803b8400, 0x803c0400, 0x803c8400, 0x1b80001f,
	0x20000300, 0x80348400, 0x80350400, 0x80358400, 0x1b80001f, 0x20000104,
	0x80308400, 0x80320400, 0x81f38407, 0x81f98407, 0x81f40407, 0x81401801,
	0xd8005525, 0x17c07c1f, 0x18c0001f, 0x10006290, 0x1212841f, 0xc0c028c0,
	0x12807c1f, 0xc0c028c0, 0x1280041f, 0x18c0001f, 0x10006208, 0x1212841f,
	0xc0c028c0, 0x12807c1f, 0xe8208000, 0x10006248, 0x00000001, 0x1b80001f,
	0x20000080, 0xc0c028c0, 0x1280041f, 0x18c0001f, 0x10006200, 0x1212841f,
	0xc0c028c0, 0x12807c1f, 0xe8208000, 0x1000625c, 0x00000000, 0x1b80001f,
	0x20000080, 0xc0c028c0, 0x1280041f, 0x81f10407, 0x81f48407, 0xa1d60407,
	0x1ac0001f, 0x55aa55aa, 0x10007c1f, 0xf0000000
};

static struct pcm_desc dpidle_pcm = {
	.version = "pcm_deepidle_v14.0_new",
	.base = dpidle_binary,
	.size = 688,
	.sess = 2,
	.replace = 0,
	.vec0 = EVENT_VEC(11, 1, 0, 0),	/* FUNC_26M_WAKEUP */
	.vec1 = EVENT_VEC(12, 1, 0, 8),	/* FUNC_26M_SLEEP */
	.vec2 = EVENT_VEC(30, 1, 0, 24),	/* FUNC_APSRC_WAKEUP */
	.vec3 = EVENT_VEC(31, 1, 0, 125),	/* FUNC_APSRC_SLEEP */
	.vec4 = EVENT_VEC(20, 1, 0, 220),	/* FUNC_AUDIO_REQ_WAKEUP */
	.vec5 = EVENT_VEC(1, 1, 0, 249),	/* FUNC_AUDIO_REQ_SLEEP */
};

static struct pwr_ctrl dpidle_ctrl = {
	.wake_src = WAKE_SRC_FOR_DPIDLE,
	.wake_src_md32 = WAKE_SRC_FOR_MD32,
	.r0_ctrl_en = 1,
	.r7_ctrl_en = 1,
	.infra_dcm_lock = 1,
	.wfi_op = WFI_OP_AND,
	.ca15_wfi0_en = 1,
	.ca15_wfi1_en = 1,
	.ca15_wfi2_en = 1,
	.ca15_wfi3_en = 1,
	.ca7_wfi0_en = 1,
	.ca7_wfi1_en = 1,
	.ca7_wfi2_en = 1,
	.ca7_wfi3_en = 1,
	.disp_req_mask = 1,
	.mfg_req_mask = 1,
	.lte_mask = 1,
	.syspwreq_mask = 1,
};
#else
#error "Does not support!"
#endif

static unsigned int dpidle_log_discard_cnt;
static unsigned int dpidle_log_print_prev_time;

struct spm_lp_scen __spm_dpidle = {
#if defined(CONFIG_ARCH_MT6570) || defined(CONFIG_ARCH_MT6580)
	.pcmdesc = &dpidle_pcm,
#endif
	.pwrctrl = &dpidle_ctrl,
};

int __attribute__ ((weak)) request_uart_to_sleep(void)
{
	return 0;
}

int __attribute__ ((weak)) request_uart_to_wakeup(void)
{
	return 0;
}

#if SPM_AEE_RR_REC
void __attribute__ ((weak)) aee_rr_rec_deepidle_val(u32 val)
{
}

u32 __attribute__ ((weak)) aee_rr_curr_deepidle_val(void)
{
	return 0;
}
#endif

void __attribute__ ((weak)) mt_cirq_clone_gic(void)
{
}

void __attribute__ ((weak)) mt_cirq_enable(void)
{
}

void __attribute__ ((weak)) mt_cirq_flush(void)
{
}

void __attribute__ ((weak)) mt_cirq_disable(void)
{
}

int __attribute__ ((weak)) exec_ccci_kern_func_by_md_id(int md_id, unsigned int id, char *buf,
							unsigned int len)
{
	return 0;
}

u32 __attribute__ ((weak)) spm_get_sleep_wakesrc(void)
{
	return 0;
}

void __attribute__ ((weak)) mt_cpufreq_set_pmic_phase(enum pmic_wrap_phase_id phase)
{
}

static long int idle_get_current_time_ms(void)
{
	struct timeval t;

	do_gettimeofday(&t);
	return ((t.tv_sec & 0xFFF) * 1000000 + t.tv_usec) / 1000;
}

static void spm_trigger_wfi_for_dpidle(struct pwr_ctrl *pwrctrl)
{
#if defined(CONFIG_ARCH_MT6753)
	u32 v0, v1;
#endif

	if (is_cpu_pdn(pwrctrl->pcm_flags)) {
		mt_cpu_dormant(CPU_DEEPIDLE_MODE);
	} else {
		/*
		 * MT6735/MT6735M: Mp0_axi_config[4] is one by default. No need to program it before entering deepidle.
		 * MT6753:         Have to program it before entering deepidle.
		 */
#if defined(CONFIG_ARM_MT6735) || defined(CONFIG_ARM_MT6735M) || defined(CONFIG_ARCH_MT6570) || \
defined(CONFIG_ARCH_MT6580)
		wfi_with_sync();
#elif defined(CONFIG_ARCH_MT6753)
		/* backup MPx_AXI_CONFIG */
		v0 = reg_read(MP0_AXI_CONFIG);
		v1 = reg_read(MP1_AXI_CONFIG);

		/* disable snoop function */
		MCUSYS_SMC_WRITE(MP0_AXI_CONFIG, v0 | ACINACTM);
		MCUSYS_SMC_WRITE(MP1_AXI_CONFIG, v1 | ACINACTM);

		wfi_with_sync();

		/* restore MP0_AXI_CONFIG */
		MCUSYS_SMC_WRITE(MP0_AXI_CONFIG, v0);
		MCUSYS_SMC_WRITE(MP1_AXI_CONFIG, v1);
#endif
	}
}

/*
 * wakesrc: WAKE_SRC_XXX
 * enable : enable or disable @wakesrc
 * replace: if true, will replace the default setting
 */
int spm_set_dpidle_wakesrc(u32 wakesrc, bool enable, bool replace)
{
	unsigned long flags;

	if (spm_is_wakesrc_invalid(wakesrc))
		return -EINVAL;

	spin_lock_irqsave(&__spm_lock, flags);
	if (enable) {
		if (replace)
			__spm_dpidle.pwrctrl->wake_src = wakesrc;
		else
			__spm_dpidle.pwrctrl->wake_src |= wakesrc;
	} else {
		if (replace)
			__spm_dpidle.pwrctrl->wake_src = 0;
		else
			__spm_dpidle.pwrctrl->wake_src &= ~wakesrc;
	}
	spin_unlock_irqrestore(&__spm_lock, flags);

	return 0;
}

static wake_reason_t spm_output_wake_reason(struct wake_status *wakesta, struct pcm_desc *pcmdesc, u32 dump_log)
{
	wake_reason_t wr = WR_NONE;
	unsigned long int dpidle_log_print_curr_time = 0;
	bool log_print = false;
	static bool timer_out_too_short;

	if (dump_log == DEEPIDLE_LOG_FULL) {
		wr = __spm_output_wake_reason(wakesta, pcmdesc, false);
	} else if (dump_log == DEEPIDLE_LOG_REDUCED) {
		/* Determine print SPM log or not */
		dpidle_log_print_curr_time = idle_get_current_time_ms();

		if (wakesta->assert_pc != 0)
			log_print = true;
#if 0
		/* Not wakeup by GPT */
		else if ((wakesta->r12 & (0x1 << 4)) == 0)
			log_print = true;
		else if (wakesta->timer_out <= DPIDLE_LOG_PRINT_TIMEOUT_CRITERIA)
			log_print = true;
#endif
		else if ((dpidle_log_print_curr_time - dpidle_log_print_prev_time) >
			 DPIDLE_LOG_DISCARD_CRITERIA)
			log_print = true;

		if (wakesta->timer_out <= DPIDLE_LOG_PRINT_TIMEOUT_CRITERIA)
			timer_out_too_short = true;

		/* Print SPM log */
		if (log_print == true) {
			dpidle_dbg("dpidle_log_discard_cnt = %d, timer_out_too_short = %d\n",
						dpidle_log_discard_cnt,
						timer_out_too_short);
			wr = __spm_output_wake_reason(wakesta, pcmdesc, false);

			dpidle_log_print_prev_time = dpidle_log_print_curr_time;
			dpidle_log_discard_cnt = 0;
			timer_out_too_short = false;
		} else {
			dpidle_log_discard_cnt++;

			wr = WR_NONE;
		}
	}
#if !defined(CONFIG_ARCH_MT6570) && !defined(CONFIG_ARCH_MT6580)
	if (wakesta->r12 & WAKE_SRC_CLDMA_MD)
		exec_ccci_kern_func_by_md_id(0, ID_GET_MD_WAKEUP_SRC, NULL, 0);
#endif

	return wr;
}

#if defined(CONFIG_ARM_MT6735) || defined(CONFIG_ARM_MT6735M) || defined(CONFIG_ARCH_MT6753)
static u32 vsram_vosel_on_lb;
#endif
static void spm_dpidle_pre_process(void)
{
	/* set PMIC WRAP table for deepidle power control */
	mt_cpufreq_set_pmic_phase(PMIC_WRAP_PHASE_DEEPIDLE);
#if defined(CONFIG_ARM_MT6735) || defined(CONFIG_ARM_MT6735M) || defined(CONFIG_ARCH_MT6753)
	vsram_vosel_on_lb = __spm_dpidle_sodi_set_pmic_setting();
#endif
}

static void spm_dpidle_post_process(void)
{
#if defined(CONFIG_ARM_MT6735) || defined(CONFIG_ARM_MT6735M) || defined(CONFIG_ARCH_MT6753)
	__spm_dpidle_sodi_restore_pmic_setting(vsram_vosel_on_lb);
#endif
	/* set PMIC WRAP table for normal power control */
	mt_cpufreq_set_pmic_phase(PMIC_WRAP_PHASE_NORMAL);
}

wake_reason_t spm_go_to_dpidle(u32 spm_flags, u32 spm_data, u32 dump_log)
{
	struct wake_status wakesta;
	unsigned long flags;
	struct mtk_irq_mask mask;
	wake_reason_t wr = WR_NONE;
	struct pcm_desc *pcmdesc;
	struct pwr_ctrl *pwrctrl = __spm_dpidle.pwrctrl;

#if SPM_AEE_RR_REC
	aee_rr_rec_deepidle_val(1 << SPM_DEEPIDLE_ENTER);
#endif

#if defined(CONFIG_ARCH_MT6570) || defined(CONFIG_ARCH_MT6580)
	pcmdesc = __spm_dpidle.pcmdesc;
#else
	if (dyna_load_pcm[DYNA_LOAD_PCM_DEEPIDLE].ready)
		pcmdesc = &(dyna_load_pcm[DYNA_LOAD_PCM_DEEPIDLE].desc);
	else
		BUG();
#endif

	set_pwrctrl_pcm_flags(pwrctrl, spm_flags);

	spm_dpidle_before_wfi();

	lockdep_off();
	spin_lock_irqsave(&__spm_lock, flags);
	mt_irq_mask_all(&mask);
	mt_irq_unmask_for_sleep(SPM_IRQ0_ID);
	mt_cirq_clone_gic();
	mt_cirq_enable();

#if defined(CONFIG_ARCH_MT6753)
	__spm_enable_i2c4_clk();
#endif

#if SPM_AEE_RR_REC
	aee_rr_rec_deepidle_val(aee_rr_curr_deepidle_val() | (1 << SPM_DEEPIDLE_ENTER_UART_SLEEP));
#endif

#if !defined(CONFIG_ARCH_MT6570) && !defined(CONFIG_ARCH_MT6580)
	if (request_uart_to_sleep()) {
		wr = WR_UART_BUSY;
		goto RESTORE_IRQ;
	}
#endif

	__spm_reset_and_init_pcm(pcmdesc);

	__spm_kick_im_to_fetch(pcmdesc);

	__spm_init_pcm_register();

	__spm_init_event_vector(pcmdesc);

	__spm_set_power_control(pwrctrl);

	__spm_set_wakeup_event(pwrctrl);

	spm_dpidle_pre_process();

	__spm_kick_pcm_to_run(pwrctrl);

#if SPM_AEE_RR_REC
	aee_rr_rec_deepidle_val(aee_rr_curr_deepidle_val() | (1 << SPM_DEEPIDLE_ENTER_WFI));
#endif

#ifdef SPM_DEEPIDLE_PROFILE_TIME
	gpt_get_cnt(SPM_PROFILE_APXGPT, &dpidle_profile[1]);
#endif
#if defined(CONFIG_ARCH_MT6570) || defined(CONFIG_ARCH_MT6580)
	gic_set_primask();
#endif
	spm_trigger_wfi_for_dpidle(pwrctrl);

#if defined(CONFIG_ARCH_MT6570) || defined(CONFIG_ARCH_MT6580)
	gic_clear_primask();
#endif
#ifdef SPM_DEEPIDLE_PROFILE_TIME
	gpt_get_cnt(SPM_PROFILE_APXGPT, &dpidle_profile[2]);
#endif

#if SPM_AEE_RR_REC
	aee_rr_rec_deepidle_val(aee_rr_curr_deepidle_val() | (1 << SPM_DEEPIDLE_LEAVE_WFI));
#endif

	spm_dpidle_post_process();

	__spm_get_wakeup_status(&wakesta);

	__spm_clean_after_wakeup();

#if SPM_AEE_RR_REC
	aee_rr_rec_deepidle_val(aee_rr_curr_deepidle_val() | (1 << SPM_DEEPIDLE_ENTER_UART_AWAKE));
#endif

#if !defined(CONFIG_ARCH_MT6570) && !defined(CONFIG_ARCH_MT6580)
	request_uart_to_wakeup();
#endif

	wr = spm_output_wake_reason(&wakesta, pcmdesc, dump_log);

#if defined(CONFIG_ARCH_MT6753)
	__spm_disable_i2c4_clk();
#endif
#if !defined(CONFIG_ARCH_MT6570) && !defined(CONFIG_ARCH_MT6580)
RESTORE_IRQ:
#endif
	mt_cirq_flush();
	mt_cirq_disable();
	mt_irq_mask_restore(&mask);
	spin_unlock_irqrestore(&__spm_lock, flags);
	lockdep_on();
	spm_dpidle_after_wfi();

#if SPM_AEE_RR_REC
	aee_rr_rec_deepidle_val(0);
#endif
	return wr;
}

/*
 * cpu_pdn:
 *    true  = CPU dormant
 *    false = CPU standby
 * pwrlevel:
 *    0 = AXI is off
 *    1 = AXI is 26M
 * pwake_time:
 *    >= 0  = specific wakeup period
 */
wake_reason_t spm_go_to_sleep_dpidle(u32 spm_flags, u32 spm_data)
{
	u32 sec = 0;
	int wd_ret;
	struct wake_status wakesta;
	unsigned long flags;
	struct mtk_irq_mask mask;
	struct wd_api *wd_api;
	static wake_reason_t last_wr = WR_NONE;
	struct pcm_desc *pcmdesc;
	struct pwr_ctrl *pwrctrl = __spm_dpidle.pwrctrl;

#if defined(CONFIG_ARCH_MT6570) || defined(CONFIG_ARCH_MT6580)
	pcmdesc = __spm_dpidle.pcmdesc;
#else
	if (dyna_load_pcm[DYNA_LOAD_PCM_DEEPIDLE].ready)
		pcmdesc = &(dyna_load_pcm[DYNA_LOAD_PCM_DEEPIDLE].desc);
	else
		BUG();
#endif

	set_pwrctrl_pcm_flags(pwrctrl, spm_flags);

#if SPM_PWAKE_EN
	sec = spm_get_wake_period(-1 /* FIXME */ , last_wr);
#endif
	pwrctrl->timer_val = sec * 32768;

	pwrctrl->wake_src = spm_get_sleep_wakesrc();

	wd_ret = get_wd_api(&wd_api);
	if (!wd_ret)
		wd_api->wd_suspend_notify();

	spin_lock_irqsave(&__spm_lock, flags);

	mt_irq_mask_all(&mask);
	mt_irq_unmask_for_sleep(SPM_IRQ0_ID);

	mt_cirq_clone_gic();
	mt_cirq_enable();

	/* set PMIC WRAP table for deepidle power control */
	mt_cpufreq_set_pmic_phase(PMIC_WRAP_PHASE_DEEPIDLE);

	spm_crit2("sleep_deepidle, sec = %u, wakesrc = 0x%x [%u]\n",
		  sec, pwrctrl->wake_src, is_cpu_pdn(pwrctrl->pcm_flags));

	__spm_reset_and_init_pcm(pcmdesc);

	__spm_kick_im_to_fetch(pcmdesc);

	if (request_uart_to_sleep()) {
		last_wr = WR_UART_BUSY;
		goto RESTORE_IRQ;
	}

	__spm_init_pcm_register();

	__spm_init_event_vector(pcmdesc);

	__spm_set_power_control(pwrctrl);

	__spm_set_wakeup_event(pwrctrl);

	__spm_kick_pcm_to_run(pwrctrl);

	spm_dpidle_pre_process();

	spm_trigger_wfi_for_dpidle(pwrctrl);

	spm_dpidle_post_process();

	__spm_get_wakeup_status(&wakesta);

	__spm_clean_after_wakeup();

	request_uart_to_wakeup();

	last_wr = __spm_output_wake_reason(&wakesta, pcmdesc, true);

RESTORE_IRQ:

	/* set PMIC WRAP table for normal power control */
	mt_cpufreq_set_pmic_phase(PMIC_WRAP_PHASE_NORMAL);

	mt_cirq_flush();
	mt_cirq_disable();

	mt_irq_mask_restore(&mask);

	spin_unlock_irqrestore(&__spm_lock, flags);

	if (!wd_ret)
		wd_api->wd_resume_notify();

	return last_wr;
}

#if SPM_USE_TWAM_DEBUG
#define SPM_TWAM_MONITOR_TICK 333333


static void twam_handler(struct twam_sig *twamsig)
{
	spm_crit("sig_high = %u%%  %u%%  %u%%  %u%%, r13 = 0x%x\n",
		 get_percent(twamsig->sig0, SPM_TWAM_MONITOR_TICK),
		 get_percent(twamsig->sig1, SPM_TWAM_MONITOR_TICK),
		 get_percent(twamsig->sig2, SPM_TWAM_MONITOR_TICK),
		 get_percent(twamsig->sig3, SPM_TWAM_MONITOR_TICK), spm_read(SPM_PCM_REG13_DATA));
}
#endif

void __init spm_deepidle_init(void)
{
#if defined(CONFIG_ARCH_MT6753)
#if defined(CONFIG_OF)
	struct device_node *node;
	struct resource r;

	/* mcucfg */
	node = of_find_compatible_node(NULL, NULL, MCUCFG_NODE);
	if (!node) {
		spm_err("error: cannot find node " MCUCFG_NODE);
		BUG();
	}

	mcucfg_base = (unsigned long)of_iomap(node, 0);
	if (!mcucfg_base) {
		spm_err("error: cannot iomap " MCUCFG_NODE);
		BUG();
	}
	if (of_address_to_resource(node, 0, &r)) {
		spm_err("error: cannot get phys addr" MCUCFG_NODE);
		BUG();
	}
	mcucfg_phys_base = r.start;

	pr_debug("[%s] mcucfg_base = 0x%u\n", __func__, (unsigned int)mcucfg_base);
#endif
#endif

#if SPM_USE_TWAM_DEBUG
	unsigned long flags;
	struct twam_sig twamsig = {
		.sig0 = 26,	/* md1_srcclkena */
		.sig1 = 22,	/* md_apsrc_req_mux */
		.sig2 = 25,	/* md2_srcclkena */
		.sig3 = 21,	/* md2_apsrc_req_mux */
	};

	spm_twam_register_handler(twam_handler);
	spm_twam_enable_monitor(&twamsig, false, SPM_TWAM_MONITOR_TICK);
#endif
}

MODULE_DESCRIPTION("SPM-DPIdle Driver v0.1");
