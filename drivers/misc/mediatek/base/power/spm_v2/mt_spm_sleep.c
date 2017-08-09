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
#include <linux/string.h>
#include <linux/of_fdt.h>
#include <asm/setup.h>

#ifndef CONFIG_ARM64
#include <mach/irqs.h>
#else
#include <linux/irqchip/mt-gic.h>
#endif
#if defined(CONFIG_MTK_SYS_CIRQ)
#include <mt-plat/mt_cirq.h>
#endif
#include <mach/mt_clkmgr.h>
#include "mt_cpuidle.h"
#ifdef CONFIG_MTK_WD_KICKER
#include <mach/wd_api.h>
#endif
#include "mt_cpufreq.h"
#include <mt-plat/upmu_common.h>
#include "mt_spm_misc.h"
#include <mt_clkbuf_ctl.h>

#if 1
#include <mt_dramc.h>
#endif

#include "mt_spm_internal.h"
#if !defined(CONFIG_FPGA_EARLY_PORTING)
#include "mt_spm_pmic_wrap.h"
#endif

#include <mt-plat/mt_ccci_common.h>

#ifdef CONFIG_MTK_USB2JTAG_SUPPORT
#include <mt-plat/mt_usb2jtag.h>
#endif

#if defined(CONFIG_ARCH_MT6797)
#include "mt_vcorefs_governor.h"
#endif

/**************************************
 * only for internal debug
 **************************************/
#ifdef CONFIG_MTK_LDVT
#define SPM_PWAKE_EN            0
#define SPM_PCMWDT_EN           0
#define SPM_BYPASS_SYSPWREQ     1
#else
#define SPM_PWAKE_EN            1
#define SPM_PCMWDT_EN           1
#define SPM_BYPASS_SYSPWREQ     0
#endif

#ifdef CONFIG_OF
#define MCUCFG_BASE          spm_mcucfg
#else
#define MCUCFG_BASE          (0xF0200000)	/* 0x1020_0000 */
#endif
#define MP0_AXI_CONFIG          (MCUCFG_BASE + 0x2C)
#define MP1_AXI_CONFIG          (MCUCFG_BASE + 0x22C)
#if defined(CONFIG_ARCH_MT6797)
#define MP2_AXI_CONFIG          (MCUCFG_BASE + 0x220C)
#endif
#define ACINACTM                (1<<4)

int spm_dormant_sta = MT_CPU_DORMANT_RESET;
int spm_ap_mdsrc_req_cnt = 0;

struct wake_status suspend_info[20];
u32 log_wakesta_cnt = 0;
u32 log_wakesta_index = 0;
u8 spm_snapshot_golden_setting = 0;

struct wake_status spm_wakesta; /* record last wakesta */

#if defined(CONFIG_FPGA_EARLY_PORTING)
static const u32 suspend_binary[] = {
	0xa1d58407, 0xa1d70407, 0x81f68407, 0x803a0400, 0x1b80001f, 0x20000000,
	0x80300400, 0x80328400, 0xa1d28407, 0x81f20407, 0x81f88407, 0xe8208000,
	0x10006610, 0x20000000, 0x81431801, 0xd8000245, 0x17c07c1f, 0x80318400,
	0x81f00407, 0xa1dd0407, 0x1b80001f, 0x20000424, 0x81fd0407, 0x18c0001f,
	0x10006604, 0x1910001f, 0x10006604, 0x813f8404, 0xe0c00004, 0xc2805c00,
	0x1290041f, 0xa19d8406, 0xa1dc0407, 0x1a40001f, 0x00180006, 0xc0c06540,
	0x17c07c1f, 0xa2400c09, 0x1300241f, 0xf0000000, 0x17c07c1f, 0x81fc0407,
	0x1b00001f, 0x2fffe7ff, 0x81fc8407, 0x1b80001f, 0x20000004, 0x88c0000c,
	0x2fffe7ff, 0xd8200703, 0x17c07c1f, 0xa1dc0407, 0x1b00001f, 0x00000000,
	0xd0000ba0, 0x17c07c1f, 0xe8208000, 0x10006610, 0x10000000, 0x1a40001f,
	0x10006028, 0xc0c05ae0, 0xe240000f, 0xd82008e3, 0x17c07c1f, 0x81f00407,
	0xa1dc0407, 0x1b00001f, 0x00180006, 0xd0000ba0, 0x17c07c1f, 0xe240001f,
	0xc2805c00, 0x129f841f, 0xc2805c00, 0x1290841f, 0x81431801, 0xd8000a05,
	0x17c07c1f, 0xa0118400, 0x81bd8406, 0xa0128400, 0xa1dc0407, 0x1b00001f,
	0x00180001, 0xa1d88407, 0xa1d20407, 0x81f28407, 0xa1d68407, 0xa0100400,
	0xa01a0400, 0x81f70407, 0x81f58407, 0xf0000000, 0x17c07c1f, 0x81409801,
	0xd8000e85, 0x17c07c1f, 0x18c0001f, 0x10006318, 0xc0c042e0, 0x1200041f,
	0x1b80001f, 0x2000000e, 0x81439801, 0xd8000e85, 0x17c07c1f, 0x18c0001f,
	0x10006360, 0xe0e00016, 0x1380041f, 0xe0e0001e, 0x1380041f, 0xe0e0000e,
	0xe0e0000c, 0xe0e0000d, 0xe8208000, 0x10006610, 0x40000000, 0xe8208000,
	0x10006408, 0xff7f9fa7, 0xe8208000, 0x1000640c, 0xfe00ff00, 0xe8208000,
	0x10006610, 0x00000000, 0xc2805c00, 0x1291041f, 0xa19e0406, 0xa1dc0407,
	0x1a40001f, 0x00180058, 0xc0c06540, 0x17c07c1f, 0xa2400c09, 0x1300241f,
	0xf0000000, 0x17c07c1f, 0xe8208000, 0x10006408, 0xffffffff, 0xe8208000,
	0x1000640c, 0xffffffff, 0x81409801, 0xd8001425, 0x17c07c1f, 0x81439801,
	0xd80013a5, 0x17c07c1f, 0x18c0001f, 0x10006360, 0xe0e0000f, 0xe0e0001e,
	0xe0e00012, 0x18c0001f, 0x10006318, 0xc0c04680, 0x17c07c1f, 0xc2805c00,
	0x1291841f, 0x81be0406, 0xa1dc0407, 0x1a40001f, 0x00180006, 0xc0c06540,
	0x17c07c1f, 0xa2400c09, 0x1300241f, 0xf0000000, 0x17c07c1f, 0xc0c04c20,
	0x17c07c1f, 0xc0c054e0, 0x17c07c1f, 0x80310400, 0xe8208000, 0x10006408,
	0xff7f9fa7, 0xe8208000, 0x1000640c, 0xfe00ff00, 0xa1d80407, 0xc2805c00,
	0x1292041f, 0xa19e8406, 0x80cf1801, 0xa1dc0407, 0xd8001883, 0x17c07c1f,
	0x1a40001f, 0x00182060, 0xd00018c0, 0x17c07c1f, 0x1a40001f, 0x001820a0,
	0xc0c06540, 0x17c07c1f, 0xa2400c09, 0x1300241f, 0xf0000000, 0x17c07c1f,
	0xe8208000, 0x10006408, 0xffff9fbf, 0xe8208000, 0x1000640c, 0xfe3fff00,
	0x1b80001f, 0x2000001a, 0x81f80407, 0x1a40001f, 0x10006028, 0xa0140400,
	0xa0160400, 0xa0168400, 0xa0170400, 0xa0178400, 0x18c0001f, 0x10006460,
	0x1910001f, 0x10006460, 0x1900001f, 0xffffffff, 0xe0c00004, 0x18c0001f,
	0x10006470, 0x1910001f, 0x10006470, 0x1900001f, 0xffffffff, 0xe0c00004,
	0x1910001f, 0x10006170, 0xd8201d43, 0x80c41001, 0xa0120400, 0xc0c058c0,
	0x17c07c1f, 0xe240000f, 0xd8201ee3, 0x17c07c1f, 0xa1d80407, 0xd0002200,
	0x17c07c1f, 0xe240001f, 0xc0c052c0, 0x17c07c1f, 0xc0c047a0, 0x17c07c1f,
	0xc2805c00, 0x1292841f, 0x81be8406, 0xa19f8406, 0x80cf1801, 0xa1dc0407,
	0xd8002103, 0x17c07c1f, 0x1a40001f, 0x00180058, 0xd0002180, 0x17c07c1f,
	0x1b00001f, 0x00180090, 0x1a40001f, 0x00180090, 0xc0c06540, 0x17c07c1f,
	0xa2400c09, 0x1300241f, 0xf0000000, 0x17c07c1f, 0x814e9801, 0xd82022e5,
	0x17c07c1f, 0xc0c03d40, 0x17c07c1f, 0x80390400, 0x1b80001f, 0x2000001a,
	0x803c0400, 0x1b80001f, 0x20000208, 0x80350400, 0x1b80001f, 0x2000001a,
	0x81f98407, 0xc2805c00, 0x1293041f, 0xa19f0406, 0x80ce9801, 0xa1dc0407,
	0xd8002583, 0x17c07c1f, 0x1a40001f, 0x00180090, 0xd00026a0, 0x17c07c1f,
	0x80cf9801, 0xd8002663, 0x17c07c1f, 0x1a40001f, 0x001820a0, 0xd00026a0,
	0x17c07c1f, 0x1a40001f, 0x001810a0, 0xc0c06540, 0x17c07c1f, 0xa2400c09,
	0x1300241f, 0xc0c03ec0, 0x17c07c1f, 0xf0000000, 0x17c07c1f, 0x814e9801,
	0xd8202845, 0x17c07c1f, 0xc0c03d40, 0x17c07c1f, 0xa1d98407, 0xa0150400,
	0xa01c0400, 0xa0190400, 0x1a50001f, 0x1000644c, 0xaa400009, 0x00000020,
	0x18c0001f, 0x1000644c, 0xe0c00009, 0x1a50001f, 0x1000644c, 0xaa400009,
	0x00000200, 0x18c0001f, 0x1000644c, 0xe0c00009, 0x1a50001f, 0x1000644c,
	0xaa400009, 0x00000002, 0x18c0001f, 0x1000644c, 0xe0c00009, 0xc2805c00,
	0x1293841f, 0x81bf0406, 0x80ce9801, 0xa1dc0407, 0xd8002cc3, 0x17c07c1f,
	0x1a40001f, 0x00180058, 0xd0002de0, 0x17c07c1f, 0x80cf9801, 0xd8202da3,
	0x17c07c1f, 0x1a40001f, 0x00182060, 0xd0002de0, 0x17c07c1f, 0x1a40001f,
	0x00181060, 0xc0c06540, 0x17c07c1f, 0xa2400c09, 0x1300241f, 0xc0c03ec0,
	0x17c07c1f, 0xf0000000, 0x17c07c1f, 0xc0c054e0, 0x17c07c1f, 0xc2805c00,
	0x1294041f, 0xa19f8406, 0x80cf1801, 0xa1dc0407, 0xd8003083, 0x17c07c1f,
	0x1a40001f, 0x00182060, 0xd00030c0, 0x17c07c1f, 0x1a40001f, 0x001820a0,
	0xc0c06540, 0x17c07c1f, 0xa2400c09, 0x1300241f, 0xf0000000, 0x17c07c1f,
	0x1a40001f, 0x10006028, 0xc0c058c0, 0xe240000f, 0xd8203303, 0x17c07c1f,
	0xa1dc0407, 0x1b00001f, 0x00000000, 0x81fc0407, 0xd0003580, 0x17c07c1f,
	0xe240001f, 0xc08052c0, 0x17c07c1f, 0xc2805c00, 0x1294841f, 0x81bf8406,
	0x80cf1801, 0xa1dc0407, 0xd80034c3, 0x17c07c1f, 0x1a40001f, 0x00181060,
	0xd0003500, 0x17c07c1f, 0x1a40001f, 0x001810a0, 0xc0c06540, 0x17c07c1f,
	0xa2400c09, 0x1300241f, 0xf0000000, 0x17c07c1f, 0x814e9801, 0xd8203665,
	0x17c07c1f, 0xc0c03d40, 0x17c07c1f, 0xc0c06240, 0x17c07c1f, 0x18c0001f,
	0x10006404, 0x1910001f, 0x10006404, 0xa1108404, 0xe0c00004, 0x1b80001f,
	0x20000104, 0xc0c06420, 0x17c07c1f, 0xc2805c00, 0x1295041f, 0xa19d0406,
	0xa1dc0407, 0x1a50001f, 0x10006148, 0xa2578409, 0x82770409, 0x1300241f,
	0xc0c03ec0, 0x17c07c1f, 0xf0000000, 0x17c07c1f, 0x814e9801, 0xd8203a25,
	0x17c07c1f, 0xc0c03d40, 0x17c07c1f, 0xc0c06240, 0x17c07c1f, 0x18c0001f,
	0x10006404, 0x1910001f, 0x10006404, 0x81308404, 0xe0c00004, 0x1b80001f,
	0x20000104, 0xc0c06420, 0x17c07c1f, 0xc2805c00, 0x1295841f, 0x81bd0406,
	0xa1dc0407, 0x1a50001f, 0x10006148, 0x82778409, 0xa2570409, 0x1300241f,
	0xc0c03ec0, 0x17c07c1f, 0xf0000000, 0x17c07c1f, 0x814c1801, 0xd8203de5,
	0x17c07c1f, 0x1b80001f, 0x2000001a, 0x80310400, 0x17c07c1f, 0x17c07c1f,
	0x81fa0407, 0x81f08407, 0xf0000000, 0x17c07c1f, 0x816f9801, 0x814e9805,
	0xd82042a5, 0x17c07c1f, 0x1a40001f, 0x10006028, 0xe240000f, 0x1111841f,
	0xa1d08407, 0xd8204064, 0x80eab401, 0xd8003fe3, 0x01200404, 0x81f08c07,
	0x1a00001f, 0x100060b0, 0xe2000003, 0xd82041e3, 0x17c07c1f, 0xa1dc0407,
	0x1b00001f, 0x00000000, 0x81fc0407, 0xd00042a0, 0x17c07c1f, 0xe240001f,
	0xa1da0407, 0xa0110400, 0x814c1801, 0xd82042a5, 0x17c07c1f, 0xf0000000,
	0x17c07c1f, 0xe0f07f16, 0x1380201f, 0xe0f07f1e, 0x1380201f, 0xe0f07f0e,
	0xe0f07f0c, 0x1900001f, 0x10006354, 0xe120003e, 0xe120003c, 0xe1200038,
	0xe1200030, 0xe1200020, 0xe1200000, 0x1b80001f, 0x20000104, 0xe0f07f0d,
	0xe0f07e0d, 0x1b80001f, 0x2000001a, 0xe0f07c0d, 0x1b80001f, 0x2000001a,
	0xe0f0780d, 0x1b80001f, 0x2000001a, 0xe0f0700d, 0xf0000000, 0x17c07c1f,
	0x1900001f, 0x10006354, 0xe120003f, 0xe0f07f0d, 0xe0f07f0f, 0xe0f07f1e,
	0xe0f07f12, 0xf0000000, 0x17c07c1f, 0x80378400, 0x1a50001f, 0x1000644c,
	0xaa400009, 0x00000010, 0x18c0001f, 0x1000644c, 0xe0c00009, 0x1a50001f,
	0x1000644c, 0xaa400009, 0x00000100, 0x18c0001f, 0x1000644c, 0xe0c00009,
	0x1a50001f, 0x1000644c, 0xaa400009, 0x00000001, 0x18c0001f, 0x1000644c,
	0xe0c00009, 0x18c0001f, 0x1000631c, 0xe0e0010d, 0xe0e0030d, 0xe0e0070d,
	0xe0e00f0d, 0x1b80001f, 0x2000001a, 0xe0e00f0f, 0xe0e00f1f, 0xe0e00f1e,
	0xe0e00f12, 0xf0000000, 0x17c07c1f, 0x18c0001f, 0x1000631c, 0xe0e00f16,
	0xe0e00f1e, 0xe0e00f0e, 0x1b80001f, 0x2000001a, 0xe0e00f0c, 0xe0e00f0d,
	0xe0e0070d, 0xe0e0030d, 0xe0e0010d, 0xe0e0000d, 0x1a50001f, 0x1000644c,
	0x82720409, 0x18c0001f, 0x1000644c, 0xe0c00009, 0x1a50001f, 0x1000644c,
	0x82740409, 0x18c0001f, 0x1000644c, 0xe0c00009, 0x1b80001f, 0x00000201,
	0x1a50001f, 0x1000644c, 0x82700409, 0x18c0001f, 0x1000644c, 0xe0c00009,
	0xa0120400, 0x18c0001f, 0x10006654, 0xe0e00003, 0x1b80001f, 0x20000003,
	0xe0e0000f, 0x1b80001f, 0x20000003, 0xe0e00003, 0x1b80001f, 0x20000006,
	0xe0e00000, 0xa0178400, 0x1b80001f, 0x00000201, 0x1b80001f, 0x00000201,
	0xf0000000, 0x17c07c1f, 0xa1da0407, 0x1a50001f, 0x100062bc, 0xa25b8409,
	0x18c0001f, 0x100062bc, 0xe0c00009, 0xa0130400, 0xa0138400, 0xa0110400,
	0x80340400, 0x80360400, 0x80368400, 0x80370400, 0x80320400, 0xf0000000,
	0x17c07c1f, 0xa0170400, 0xa0168400, 0xa0180400, 0xa0160400, 0x1b80001f,
	0x0000000a, 0xa0140400, 0x1b80001f, 0x0000000a, 0x80330400, 0x80380400,
	0x80310400, 0x1b80001f, 0x00000004, 0x80338400, 0x81fa0407, 0x1a50001f,
	0x100062bc, 0x827b8409, 0x18c0001f, 0x100062bc, 0xe0c00009, 0x1b80001f,
	0x00000004, 0x81f08407, 0x824ab401, 0xd8005809, 0x17c07c1f, 0x80320400,
	0xf0000000, 0x17c07c1f, 0x1111841f, 0xa1d08407, 0xd8205984, 0x80eab401,
	0xd8005903, 0x01200404, 0x81f08c07, 0x1900001f, 0x100060b0, 0xe1000003,
	0xf0000000, 0x17c07c1f, 0xa1d10407, 0x1b80001f, 0x20000020, 0xf0000000,
	0x17c07c1f, 0xa1d00407, 0x1b80001f, 0x20000208, 0x10c07c1f, 0x1900001f,
	0x100060b0, 0xe1000003, 0xf0000000, 0x17c07c1f, 0x18c0001f, 0x10006604,
	0x1910001f, 0x10006604, 0xa1002804, 0xe0c00004, 0xf0000000, 0x17c07c1f,
	0x18c0001f, 0x100040f4, 0x19300003, 0x17c07c1f, 0x813a0404, 0xe0c00004,
	0x1b80001f, 0x20000003, 0x18c0001f, 0x10004110, 0x19300003, 0x17c07c1f,
	0xa11e8404, 0xe0c00004, 0x1b80001f, 0x20000004, 0x18c0001f, 0x100041ec,
	0x19300003, 0x17c07c1f, 0x81380404, 0xe0c00004, 0x18c0001f, 0x100040f4,
	0x19300003, 0x17c07c1f, 0xa11a8404, 0xe0c00004, 0x18c0001f, 0x100041dc,
	0x19300003, 0x17c07c1f, 0x813d0404, 0xe0c00004, 0x18c0001f, 0x10004110,
	0x19300003, 0x17c07c1f, 0x813e8404, 0xe0c00004, 0xf0000000, 0x17c07c1f,
	0x814f1801, 0xd80063e5, 0x17c07c1f, 0x80390400, 0x1b80001f, 0x2000001a,
	0x803c0400, 0x1b80001f, 0x20000208, 0x80350400, 0x1b80001f, 0x2000001a,
	0x81f98407, 0xf0000000, 0x17c07c1f, 0x814f1801, 0xd8006505, 0x17c07c1f,
	0xa1d98407, 0xa0150400, 0xa01c0400, 0xa0190400, 0xf0000000, 0x17c07c1f,
	0x10c07c1f, 0x810d1801, 0x810a1804, 0x816d1801, 0x814a1805, 0xa0d79003,
	0xa0d71403, 0xf0000000, 0x17c07c1f, 0x1b80001f, 0x20000300, 0xf0000000,
	0x17c07c1f, 0xe8208000, 0x1100f014, 0x00000002, 0xe8208000, 0x1100f020,
	0x00000001, 0xe8208000, 0x1100f004, 0x000000d6, 0x1a00001f, 0x1100f000,
	0x1a40001f, 0x1100f024, 0x18c0001f, 0x20000152, 0xd8206a4a, 0x17c07c1f,
	0xe220000a, 0xe22000f6, 0xe2400001, 0x13800c1f, 0xe220008a, 0xe2200001,
	0xe2400001, 0x13800c1f, 0xd0006b40, 0x17c07c1f, 0xe220008a, 0xe2200000,
	0xe2400001, 0x13800c1f, 0xe220000a, 0xe22000f4, 0xe2400001, 0x13800c1f,
	0xf0000000, 0x17c07c1f, 0x1880001f, 0x0000104d, 0xa0908402, 0xe2c00002,
	0x80b30402, 0xe2c00002, 0xa0928402, 0xe2c00002, 0x80b00402, 0xe2c00002,
	0xa0920402, 0xe2c00002, 0x80b10402, 0xe2c00002, 0x80b18402, 0xe2c00002,
	0x80b60402, 0xe2c00002, 0xa0940402, 0xe2c00002, 0x1950001f, 0x10006208,
	0x810c1401, 0xd8206e04, 0x17c07c1f, 0xa0938402, 0xe2c00002, 0xf0000000,
	0x17c07c1f, 0x1880001f, 0x0000104d, 0xa0908402, 0xe2c00002, 0xa0928402,
	0xe2c00002, 0x80b30402, 0xe2c00002, 0xa0940402, 0xe2c00002, 0x1950001f,
	0x10006204, 0x810c1401, 0xd8207064, 0x17c07c1f, 0x80b00402, 0xe2c00002,
	0xa0920402, 0xe2c00002, 0x80b10402, 0xe2c00002, 0x80b18402, 0xe2c00002,
	0xf0000000, 0x17c07c1f, 0x1880001f, 0x0000104d, 0xa0928402, 0xe2c00002,
	0xa0940402, 0xe2c00002, 0x1950001f, 0x10006200, 0x810c1401, 0xd8207304,
	0x17c07c1f, 0xa0938402, 0xe2c00002, 0xa0908402, 0xe2c00002, 0x80b00402,
	0xe2c00002, 0xa0920402, 0xe2c00002, 0x80b10402, 0xe2c00002, 0x80b18402,
	0xe2c00002, 0x1b80001f, 0x2000001a, 0xf0000000, 0x17c07c1f, 0x1880001f,
	0x00001332, 0x80b60402, 0xe2c00002, 0x80b00402, 0xe2c00002, 0xa0920402,
	0xe2c00002, 0xa0940402, 0xe2c00002, 0x1950001f, 0x10006208, 0x810c1401,
	0xd82076e4, 0x17c07c1f, 0x80b38402, 0xe2c00002, 0x80b40402, 0xe2c00002,
	0x1950001f, 0x10006208, 0x810c1401, 0xd8007804, 0x17c07c1f, 0xa0910402,
	0xe2c00002, 0x1b80001f, 0x20000006, 0x1950001f, 0x10006180, 0x81049401,
	0xd8207924, 0x17c07c1f, 0xa0918402, 0xe2c00002, 0x1950001f, 0x10006184,
	0x81049401, 0xd8207a04, 0x17c07c1f, 0xa0960402, 0xe2c00002, 0xa0930402,
	0xe2c00002, 0x80b08402, 0xe2c00002, 0x1b80001f, 0x2000001a, 0x80b28402,
	0xe2c00002, 0x80b20402, 0xe2c00002, 0xa0900402, 0xe2c00002, 0xf0000000,
	0x17c07c1f, 0x1880001f, 0x00001332, 0xa0910402, 0xe2c00002, 0x1b80001f,
	0x20000006, 0xa0918402, 0xe2c00002, 0x1950001f, 0x10006180, 0x81041401,
	0x1950001f, 0x10006184, 0x81441401, 0x81001404, 0xd8207da4, 0x17c07c1f,
	0x80b08402, 0xe2c00002, 0x80b40402, 0xe2c00002, 0x1950001f, 0x10006204,
	0x810c1401, 0xd8007f44, 0x17c07c1f, 0x1b80001f, 0x2000001a, 0xa0930402,
	0xe2c00002, 0x1b80001f, 0x20000003, 0x80b28402, 0xe2c00002, 0x80b20402,
	0xe2c00002, 0xa0900402, 0xe2c00002, 0xf0000000, 0x17c07c1f, 0x1880001f,
	0x000011f2, 0xa0910402, 0xe2c00002, 0xa0918402, 0xe2c00002, 0x1b80001f,
	0x2000001a, 0x1950001f, 0x10006180, 0x81071401, 0x1950001f, 0x10006184,
	0x81471401, 0x81001404, 0xd82082a4, 0x17c07c1f, 0x80b08402, 0xe2c00002,
	0x80b38402, 0xe2c00002, 0x80b40402, 0xe2c00002, 0x1950001f, 0x10006200,
	0x810c1401, 0xd8008484, 0x17c07c1f, 0x80b28402, 0xe2c00002, 0x1b80001f,
	0x2000001a, 0x80b20402, 0xe2c00002, 0xa0900402, 0xe2c00002, 0xf0000000,
	0x17c07c1f, 0xa1d40407, 0x1391841f, 0xa1d90407, 0x1392841f, 0x18c0001f,
	0x100062bc, 0x1910001f, 0x100062bc, 0x1900001f, 0x0000000f, 0xe0c00004,
	0xa1d38407, 0x18c0001f, 0x1000cf38, 0x1910001f, 0x1000cf38, 0xa1120404,
	0xe0c00004, 0xa0108400, 0xa0158400, 0xa0148400, 0x18c0001f, 0x100062bc,
	0x1910001f, 0x100062bc, 0x1900001f, 0x00ff0f0f, 0xe0c00004, 0xa01c8400,
	0xa01b8400, 0x18c0001f, 0x100062bc, 0x1910001f, 0x100062bc, 0x1900001f,
	0x0fff0f0f, 0xe0c00004, 0xa0198400, 0xa0188400, 0x18c0001f, 0x100062bc,
	0x1910001f, 0x100062bc, 0x1900001f, 0x0fffff0f, 0xe0c00004, 0xf0000000,
	0x17c07c1f, 0x80398400, 0x80388400, 0x18c0001f, 0x100062bc, 0x1910001f,
	0x100062bc, 0x1900001f, 0x0fff0f0f, 0xe0c00004, 0x1b80001f, 0x20000300,
	0x803c8400, 0x803b8400, 0x18c0001f, 0x100062bc, 0x1910001f, 0x100062bc,
	0x1900001f, 0x00ff0f0f, 0xe0c00004, 0x1b80001f, 0x20000300, 0x80358400,
	0x80348400, 0x18c0001f, 0x100062bc, 0x1910001f, 0x100062bc, 0x1900001f,
	0x00f00f0f, 0xe0c00004, 0x1b80001f, 0x20000104, 0x80308400, 0x81f38407,
	0x18c0001f, 0x1000cf38, 0x1910001f, 0x1000cf38, 0x81320404, 0xe0c00004,
	0x81f90407, 0x81f40407, 0xf0000000, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
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
	0x10006600, 0xa9800006, 0xfe000000, 0x1a50001f, 0x1000661c, 0x82400409,
	0xa19c2406, 0xe8208000, 0x10006604, 0x00000000, 0x81550406, 0xd8012005,
	0x17c07c1f, 0x81fc0407, 0x1b00001f, 0x2fffe7ff, 0xa1dc0407, 0x1b00001f,
	0x00000000, 0x81401801, 0xd80103c5, 0x17c07c1f, 0x1b80001f, 0x5000aaaa,
	0xd00104a0, 0x17c07c1f, 0x1b80001f, 0xd00f0000, 0x81fc8407, 0x8a40000c,
	0x2fffe7ff, 0xd8011ce9, 0x17c07c1f, 0xe8208000, 0x10006408, 0xff7f9fa7,
	0xe8208000, 0x1000640c, 0xfe00ff00, 0xc0c05a40, 0x81401801, 0xd8010985,
	0x17c07c1f, 0x80c41801, 0xd8010663, 0x17c07c1f, 0x81f60407, 0x18c0001f,
	0x10006208, 0x1910001f, 0x10006208, 0x81310404, 0xe0c00004, 0x1b80001f,
	0x2000001a, 0x18c0001f, 0x10006204, 0x1910001f, 0x10006204, 0x81310404,
	0xe0c00004, 0x1b80001f, 0x2000001a, 0x18c0001f, 0x10006200, 0xc0c07240,
	0x17c07c1f, 0xe8208000, 0x10006290, 0x00000003, 0xc2805c00, 0x1296041f,
	0x81401801, 0xd8010b25, 0x17c07c1f, 0xa1dc0407, 0x1b00001f, 0x00000000,
	0x1b80001f, 0x30000004, 0x81fc8407, 0x8a40000c, 0x2fffe7ff, 0xd80119a9,
	0x17c07c1f, 0x81481801, 0xd8010d25, 0x17c07c1f, 0x1a40001f, 0x10006350,
	0x1910001f, 0x10006350, 0x1900001f, 0xffffff00, 0xe2400004, 0x1900001f,
	0x00ffff00, 0xe2400004, 0x1900001f, 0x0000ff00, 0xe2400004, 0x81449801,
	0xd8010e65, 0x17c07c1f, 0x1a00001f, 0x10006400, 0xe2200001, 0xc0c066e0,
	0x12807c1f, 0xc0c06660, 0x17c07c1f, 0xc0c08660, 0x17c07c1f, 0xe8208000,
	0x10006080, 0x0000f3f3, 0x81fc0407, 0x1b00001f, 0x2fffe7ff, 0xa1dc0407,
	0x18c0001f, 0x001820a0, 0x814a1801, 0xa0d79403, 0x13000c1f, 0x1b80001f,
	0x90100000, 0x80cd9801, 0xc8e00003, 0x17c07c1f, 0x80ce1801, 0xc8e00be3,
	0x17c07c1f, 0x80cf1801, 0xc8e02243, 0x17c07c1f, 0x80cf9801, 0xc8e02ee3,
	0x17c07c1f, 0x80ce9801, 0xc8e015a3, 0x17c07c1f, 0x80cd1801, 0xc8e035c3,
	0x17c07c1f, 0x18c0001f, 0x10006608, 0xe0c0000c, 0x81481801, 0xd80117e5,
	0x17c07c1f, 0x1a40001f, 0x10006350, 0x1910001f, 0x10006350, 0x1900001f,
	0x0001ff00, 0xe2400004, 0x1900001f, 0x0003ff00, 0xe2400004, 0x1900001f,
	0x0007ff00, 0xe2400004, 0x1900001f, 0x000fff00, 0xe2400004, 0x1900001f,
	0x001fff00, 0xe2400004, 0x1900001f, 0x003fff00, 0xe2400004, 0x1900001f,
	0x007fff00, 0xe2400004, 0x1900001f, 0x00ffff00, 0xe2400004, 0x1b80001f,
	0x20000020, 0x1900001f, 0xffffff00, 0xe2400004, 0x1900001f, 0xffff0000,
	0xe2400004, 0xc0c08c60, 0x17c07c1f, 0x81449801, 0xd80119a5, 0x17c07c1f,
	0x1a00001f, 0x10006400, 0xe2200000, 0xc0c066e0, 0x1280041f, 0xc0c06660,
	0x17c07c1f, 0x1b80001f, 0x200016a8, 0x81401801, 0xd8011ce5, 0x17c07c1f,
	0xe8208000, 0x10006290, 0x00000000, 0x18c0001f, 0x10006200, 0xc0c081a0,
	0x17c07c1f, 0x18c0001f, 0x10006204, 0x1910001f, 0x10006204, 0xa1110404,
	0xe0c00004, 0x1b80001f, 0x2000001a, 0x18c0001f, 0x10006208, 0x1910001f,
	0x10006208, 0xa1110404, 0xe0c00004, 0x1b80001f, 0x2000001a, 0x81f48407,
	0xa1d60407, 0x81f10407, 0x18c0001f, 0x1000641c, 0x1910001f, 0x10006164,
	0xe0c00004, 0x18c0001f, 0x10006420, 0x1910001f, 0x10006150, 0xe0c00004,
	0x18c0001f, 0x10006614, 0x1910001f, 0x10006614, 0x09000004, 0x00000001,
	0xe0c00004, 0x1ac0001f, 0x55aa55aa, 0xe8208000, 0x10006090, 0x00000001,
	0xf0000000, 0x17c07c1f
};
static struct pcm_desc suspend_pcm = {
	.version	= "pcm_suspend_v1",
	.base		= suspend_binary,
	.size		= 2306,
	.sess		= 2,
	.replace	= 0,
	.addr_2nd	= 0,
	.vec0		= EVENT_VEC(32, 1, 0, 0),	/* FUNC_26M_WAKEUP */
	.vec1		= EVENT_VEC(33, 1, 0, 41),	/* FUNC_26M_SLEEP */
	.vec4		= EVENT_VEC(34, 1, 0, 95),	/* FUNC_INFRA_WAKEUP */
	.vec5		= EVENT_VEC(35, 1, 0, 140),	/* FUNC_INFRA_SLEEP */
	.vec6		= EVENT_VEC(46, 1, 0, 430),	/* FUNC_NFC_CLK_BUF_ON */
	.vec7		= EVENT_VEC(47, 1, 0, 460),	/* FUNC_NFC_CLK_BUF_OFF */
	.vec8		= EVENT_VEC(36, 1, 0, 173),	/* FUNC_APSRC_WAKEUP */
	.vec9		= EVENT_VEC(37, 1, 0, 204),	/* FUNC_APSRC_SLEEP */
	.vec12		= EVENT_VEC(38, 1, 0, 274),	/* FUNC_VRF18_WAKEUP */
	.vec13		= EVENT_VEC(39, 1, 0, 317),	/* FUNC_VRF18_SLEEP */
	.vec14		= EVENT_VEC(44, 1, 0, 375),	/* FUNC_DDREN_WAKEUP */
	.vec15		= EVENT_VEC(45, 1, 0, 396),	/* FUNC_DDREN_SLEEP */
};
#endif

/**************************************

 * SW code for suspend
 **************************************/
#define SPM_SYSCLK_SETTLE       99	/* 3ms */

#define WAIT_UART_ACK_TIMES     10	/* 10 * 10us */

#if defined(CONFIG_ARCH_MT6755) || defined(CONFIG_ARCH_MT6757)

#if defined(CONFIG_MICROTRUST_TEE_SUPPORT)
#define WAKE_SRC_FOR_SUSPEND \
	(WAKE_SRC_R12_MD32_WDT_EVENT_B | \
	WAKE_SRC_R12_KP_IRQ_B | \
	WAKE_SRC_R12_PCM_TIMER |            \
	WAKE_SRC_R12_CONN2AP_SPM_WAKEUP_B | \
	WAKE_SRC_R12_EINT_EVENT_B | \
	WAKE_SRC_R12_CONN_WDT_IRQ_B | \
	WAKE_SRC_R12_CCIF0_EVENT_B | \
	WAKE_SRC_R12_CCIF1_EVENT_B | \
	WAKE_SRC_R12_MD32_SPM_IRQ_B | \
	WAKE_SRC_R12_USB_CDSC_B | \
	WAKE_SRC_R12_USB_POWERDWN_B | \
	WAKE_SRC_R12_EINT_EVENT_SECURE_B | \
	WAKE_SRC_R12_MD1_WDT_B | \
	WAKE_SRC_R12_MD2_WDT_B | \
	WAKE_SRC_R12_CLDMA_EVENT_B | \
	WAKE_SRC_R12_ALL_MD32_WAKEUP_B)
#else
#define WAKE_SRC_FOR_SUSPEND \
	(WAKE_SRC_R12_MD32_WDT_EVENT_B | \
	WAKE_SRC_R12_KP_IRQ_B | \
	WAKE_SRC_R12_PCM_TIMER |            \
	WAKE_SRC_R12_CONN2AP_SPM_WAKEUP_B | \
	WAKE_SRC_R12_EINT_EVENT_B | \
	WAKE_SRC_R12_CONN_WDT_IRQ_B | \
	WAKE_SRC_R12_CCIF0_EVENT_B | \
	WAKE_SRC_R12_CCIF1_EVENT_B | \
	WAKE_SRC_R12_MD32_SPM_IRQ_B | \
	WAKE_SRC_R12_USB_CDSC_B | \
	WAKE_SRC_R12_USB_POWERDWN_B | \
	WAKE_SRC_R12_EINT_EVENT_SECURE_B | \
	WAKE_SRC_R12_MD1_WDT_B | \
	WAKE_SRC_R12_MD2_WDT_B | \
	WAKE_SRC_R12_CLDMA_EVENT_B | \
	WAKE_SRC_R12_SEJ_WDT_GPT_B | \
	WAKE_SRC_R12_ALL_MD32_WAKEUP_B)
#endif /* #if defined(CONFIG_MICROTRUST_TEE_SUPPORT) */

#else

#if defined(CONFIG_MICROTRUST_TEE_SUPPORT)
#define WAKE_SRC_FOR_SUSPEND \
	(WAKE_SRC_R12_PCM_TIMER | \
	WAKE_SRC_R12_KP_IRQ_B | \
	WAKE_SRC_R12_CONN2AP_SPM_WAKEUP_B | \
	WAKE_SRC_R12_EINT_EVENT_B | \
	WAKE_SRC_R12_CONN_WDT_IRQ_B | \
	WAKE_SRC_R12_CCIF0_EVENT_B | \
	WAKE_SRC_R12_MD32_SPM_IRQ_B | \
	WAKE_SRC_R12_USB_CDSC_B | \
	WAKE_SRC_R12_USB_POWERDWN_B | \
	WAKE_SRC_R12_C2K_WDT_IRQ_B | \
	WAKE_SRC_R12_EINT_EVENT_SECURE_B | \
	WAKE_SRC_R12_CCIF1_EVENT_B | \
	WAKE_SRC_R12_CSYSPWREQ_B | \
	WAKE_SRC_R12_MD1_WDT_B | \
	WAKE_SRC_R12_CLDMA_EVENT_B)
#else
#define WAKE_SRC_FOR_SUSPEND \
	(WAKE_SRC_R12_PCM_TIMER | \
	WAKE_SRC_R12_KP_IRQ_B | \
	WAKE_SRC_R12_CONN2AP_SPM_WAKEUP_B | \
	WAKE_SRC_R12_EINT_EVENT_B | \
	WAKE_SRC_R12_CONN_WDT_IRQ_B | \
	WAKE_SRC_R12_CCIF0_EVENT_B | \
	WAKE_SRC_R12_MD32_SPM_IRQ_B | \
	WAKE_SRC_R12_USB_CDSC_B | \
	WAKE_SRC_R12_USB_POWERDWN_B | \
	WAKE_SRC_R12_C2K_WDT_IRQ_B | \
	WAKE_SRC_R12_EINT_EVENT_SECURE_B | \
	WAKE_SRC_R12_CCIF1_EVENT_B | \
	WAKE_SRC_R12_CSYSPWREQ_B | \
	WAKE_SRC_R12_MD1_WDT_B | \
	WAKE_SRC_R12_CLDMA_EVENT_B | \
	WAKE_SRC_R12_SEJ_WDT_GPT_B)
#endif /* #if defined(CONFIG_MICROTRUST_TEE_SUPPORT) */

#endif

#define WAKE_SRC_FOR_MD32  0 \
				/* (WAKE_SRC_AUD_MD32) */

#define spm_is_wakesrc_invalid(wakesrc)     (!!((u32)(wakesrc) & 0xc0003803))

/* FIXME: check mt_cpu_dormant */
int __attribute__ ((weak)) mt_cpu_dormant(unsigned long flags)
{
	return 0;
}

#if defined(CONFIG_ARCH_MT6797)
void __attribute__((weak)) mt_cirq_clone_gic(void)
{
}

void __attribute__((weak)) mt_cirq_enable(void)
{
}

void __attribute__((weak)) mt_cirq_flush(void)
{
}

void __attribute__((weak)) mt_cirq_disable(void)
{
}
#endif

#if SPM_AEE_RR_REC
enum spm_suspend_step {
	SPM_SUSPEND_ENTER = 0,
	SPM_SUSPEND_ENTER_WFI = 0xff,
	SPM_SUSPEND_LEAVE_WFI = 0x1ff,
	SPM_SUSPEND_LEAVE
};
#endif

#if defined(CONFIG_ARCH_MT6757)
static struct pwr_ctrl suspend_ctrl = {
	.wake_src = WAKE_SRC_FOR_SUSPEND,
	.wake_src_md32 = WAKE_SRC_FOR_MD32,
	.r0_ctrl_en = 1,
	.r7_ctrl_en = 1,
	.infra_dcm_lock = 1,

	/* SPM_AP_STANDBY_CON */
	.wfi_op = WFI_OP_AND,
	.mp0_cputop_idle_mask = 0,
	.mp1_cputop_idle_mask = 0,
	.mcusys_idle_mask = 0,
	.mm_mask_b = 0,
	.md_ddr_en_dbc_en = 0,
	.md_mask_b = 1,
	.scp_mask_b = 0,
	.lte_mask_b = 0,
	.srcclkeni_mask_b = 0,
	.md_apsrc_1_sel = 0,
	.md_apsrc_0_sel = 0,
	.conn_mask_b = 1,
	.conn_apsrc_sel = 0,

	/* SPM_SRC_REQ */
	.spm_apsrc_req = 1,
	.spm_f26m_req = 1,
	.spm_lte_req = 1,
	.spm_infra_req = 1,
	.spm_vrf18_req = 1,
	.spm_dvfs_req = 0,
	.spm_dvfs_force_down = 1,
	.spm_ddren_req = 1,
	.spm_rsv_src_req = 0,
	.cpu_md_dvfs_sop_force_on = 0,

#if !defined(CONFIG_FPGA_EARLY_PORTING)
	/* for DVFS */
	.sw_crtl_event_on = 1,
#endif

	/* SPM_SRC_MASK */
#if SPM_BYPASS_SYSPWREQ
	.csyspwreq_mask = 1,
#endif
	.ccif0_md_event_mask_b = 1,
	.ccif0_ap_event_mask_b = 1,
	.ccif1_md_event_mask_b = 1,
	.ccif1_ap_event_mask_b = 1,
	.ccifmd_md1_event_mask_b = 1,
	.ccifmd_md2_event_mask_b = 1,
	.dsi0_vsync_mask_b = 0,
	.dsi1_vsync_mask_b = 0,
	.dpi_vsync_mask_b = 0,
	.isp0_vsync_mask_b = 0,
	.isp1_vsync_mask_b = 0,
	.md_srcclkena_0_infra_mask_b = 0,
	.md_srcclkena_1_infra_mask_b = 0,
	.conn_srcclkena_infra_mask_b = 0,
	.md32_srcclkena_infra_mask_b = 0,
#if defined(CONFIG_ARCH_MT6755)
	.srcclkeni_infra_mask_b = 1,
#else
	.srcclkeni_infra_mask_b = 0,
#endif
	.md_apsrc_req_0_infra_mask_b = 1,
	.md_apsrc_req_1_infra_mask_b = 0,
	.conn_apsrcreq_infra_mask_b = 1,
	.md32_apsrcreq_infra_mask_b = 0,
	.md_ddr_en_0_mask_b = 1,
	.md_ddr_en_1_mask_b = 0,
	.md_vrf18_req_0_mask_b = 1,
	.md_vrf18_req_1_mask_b = 0,
	.md1_dvfs_req_mask = 0,
	.cpu_dvfs_req_mask = 0,
	.emi_bw_dvfs_req_mask = 1,
	.md_srcclkena_0_dvfs_req_mask_b = 0,
	.md_srcclkena_1_dvfs_req_mask_b = 0,
	.conn_srcclkena_dvfs_req_mask_b = 0,

	/* SPM_SRC2_MASK */
	.dvfs_halt_mask_b = 0x1f,
	.vdec_req_mask_b = 0,
	.gce_req_mask_b = 0,
	.cpu_md_dvfs_req_merge_mask_b = 0,
	.md_ddr_en_dvfs_halt_mask_b = 0,
	.dsi0_vsync_dvfs_halt_mask_b = 0,
	.dsi1_vsync_dvfs_halt_mask_b = 0,
	.dpi_vsync_dvfs_halt_mask_b = 0,
	.isp0_vsync_dvfs_halt_mask_b = 0,
	.isp1_vsync_dvfs_halt_mask_b = 0,
	.conn_ddr_en_mask_b = 1,
	.disp_req_mask_b = 0,
	.disp1_req_mask_b = 0,
	.mfg_req_mask_b = 0,
	.c2k_ps_rccif_wake_mask_b = 1,
	.c2k_l1_rccif_wake_mask_b = 1,
	.ps_c2k_rccif_wake_mask_b = 1,
	.l1_c2k_rccif_wake_mask_b = 1,
	.sdio_on_dvfs_req_mask_b = 0,
	.emi_boost_dvfs_req_mask_b = 0,
	.cpu_md_emi_dvfs_req_prot_dis = 0,
	.dramc_spcmd_apsrc_req_mask_b = 0,

#if 0
	/* SPM_WAKEUP_EVENT_MASK */
	.spm_wakeup_event_mask = 0,

	/* SPM_WAKEUP_EVENT_EXT_MASK */
	.spm_wakeup_event_ext_mask = 0,
#endif

	/* SPM_CLK_CON */
	.srclkenai_mask = 1,

	.mp0_cpu0_wfi_en = 1,
	.mp0_cpu1_wfi_en = 1,
	.mp0_cpu2_wfi_en = 1,
	.mp0_cpu3_wfi_en = 1,
	.mp1_cpu0_wfi_en = 1,
	.mp1_cpu1_wfi_en = 1,
	.mp1_cpu2_wfi_en = 1,
	.mp1_cpu3_wfi_en = 1,
};
#else
static struct pwr_ctrl suspend_ctrl = {

	.wake_src = WAKE_SRC_FOR_SUSPEND,
	.wake_src_md32 = WAKE_SRC_FOR_MD32,
	.r0_ctrl_en = 1,
	.r7_ctrl_en = 1,
	.infra_dcm_lock = 1,
	.wfi_op = WFI_OP_AND,

	.mp0top_idle_mask = 0,
	.mp1top_idle_mask = 0,
#if defined(CONFIG_ARCH_MT6797)
	.mp2top_idle_mask = 0,
	.mp3top_idle_mask = 1,
	.mptop_idle_mask = 0,
#endif
	.mcusys_idle_mask = 0,
	.md_ddr_dbc_en = 0,
	.md1_req_mask_b = 1,
	.md2_req_mask_b = 0,
#if defined(CONFIG_ARCH_MT6755)
	.scp_req_mask_b = 0,
#elif defined(CONFIG_ARCH_MT6797)
	.scp_req_mask_b = 1,
#endif
	.lte_mask_b = 0,
	.md_apsrc1_sel = 0,
	.md_apsrc0_sel = 0,
	.conn_mask_b = 1,
	.conn_apsrc_sel = 0,

	.ccif0_to_ap_mask_b = 1,
	.ccif0_to_md_mask_b = 1,
	.ccif1_to_ap_mask_b = 1,
	.ccif1_to_md_mask_b = 1,
	.ccifmd_md1_event_mask_b = 1,
	.ccifmd_md2_event_mask_b = 1,
	.vsync_mask_b = 0,	/* 5bit */
	.md_srcclkena_0_infra_mask_b = 0,
	.md_srcclkena_1_infra_mask_b = 0,
	.conn_srcclkena_infra_mask_b = 0,
	.md32_srcclkena_infra_mask_b = 0,
#if defined(CONFIG_ARCH_MT6755)
	.srcclkeni_infra_mask_b = 1,
#elif defined(CONFIG_ARCH_MT6797)
	.srcclkeni_infra_mask_b = 0,
#endif

	.md_apsrcreq_0_infra_mask_b = 1,
	.md_apsrcreq_1_infra_mask_b = 0,
	.conn_apsrcreq_infra_mask_b = 1,
	.md32_apsrcreq_infra_mask_b = 0,
	.md_ddr_en_0_mask_b = 1,
	.md_ddr_en_1_mask_b = 0,
	.md_vrf18_req_0_mask_b = 1,
	.md_vrf18_req_1_mask_b = 0,
#if defined(CONFIG_ARCH_MT6797)
	.md1_dvfs_req_mask = 0,
	.cpu_dvfs_req_mask = 0,
#endif
	.emi_bw_dvfs_req_mask = 1,
	.md_srcclkena_0_dvfs_req_mask_b = 0,
	.md_srcclkena_1_dvfs_req_mask_b = 0,
	.conn_srcclkena_dvfs_req_mask_b = 0,

	.dvfs_halt_mask_b = 0x1f,	/* 5bit */
	.vdec_req_mask_b = 0,
	.gce_req_mask_b = 0,
	.cpu_md_dvfs_erq_merge_mask_b = 0,
	.md1_ddr_en_dvfs_halt_mask_b = 0,
	.md2_ddr_en_dvfs_halt_mask_b = 0,
	.vsync_dvfs_halt_mask_b = 0,	/* 5bit */
	.conn_ddr_en_mask_b = 1,
	.disp_req_mask_b = 0,
	.disp1_req_mask_b = 0,
	.mfg_req_mask_b = 0,
	.c2k_ps_rccif_wake_mask_b = 1,
	.c2k_l1_rccif_wake_mask_b = 1,
	.ps_c2k_rccif_wake_mask_b = 1,
	.l1_c2k_rccif_wake_mask_b = 1,
	.sdio_on_dvfs_req_mask_b = 0,
	.emi_boost_dvfs_req_mask_b = 0,
	.cpu_md_emi_dvfs_req_prot_dis = 0,
#if defined(CONFIG_ARCH_MT6797)
	.disp_od_req_mask_b = 0,
#endif
	.spm_apsrc_req = 0,
	.spm_f26m_req = 0,
	.spm_lte_req = 0,
	.spm_infra_req = 0,
	.spm_vrf18_req = 0,
	.spm_dvfs_req = 0,
	.spm_dvfs_force_down = 1,
	.spm_ddren_req = 0,
	.cpu_md_dvfs_sop_force_on = 0,

	/* SPM_CLK_CON */
	.srclkenai_mask = 1,

	.mp0_cpu0_wfi_en = 1,
	.mp0_cpu1_wfi_en = 1,
	.mp0_cpu2_wfi_en = 1,
	.mp0_cpu3_wfi_en = 1,
	.mp1_cpu0_wfi_en = 1,
	.mp1_cpu1_wfi_en = 1,
	.mp1_cpu2_wfi_en = 1,
	.mp1_cpu3_wfi_en = 1,
#if defined(CONFIG_ARCH_MT6797)
	.mp2_cpu0_wfi_en = 1,
	.mp2_cpu1_wfi_en = 1,
#endif
#if SPM_BYPASS_SYSPWREQ
	.syspwreq_mask = 1,
#endif
};
#endif

/* please put firmware to vendor/mediatek/proprietary/hardware/spm/mtxxxx/ */
struct spm_lp_scen __spm_suspend = {
#if defined(CONFIG_FPGA_EARLY_PORTING)
	.pcmdesc = &suspend_pcm,
#endif
	.pwrctrl = &suspend_ctrl,
	.wakestatus = &suspend_info[0],
};

static void spm_suspend_pre_process(struct pwr_ctrl *pwrctrl)
{
#if !defined(CONFIG_FPGA_EARLY_PORTING)
	unsigned int temp;
#endif

	__spm_pmic_low_iq_mode(1);

	__spm_pmic_pg_force_on();

	spm_pmic_power_mode(PMIC_PWR_SUSPEND, 0, 0);

#if defined(CONFIG_ARCH_MT6755) || defined(CONFIG_ARCH_MT6757)
#if !defined(CONFIG_FPGA_EARLY_PORTING)
	/* set PMIC WRAP table for suspend power control */
	pmic_read_interface_nolock(MT6351_PMIC_RG_VSRAM_PROC_EN_ADDR, &temp, 0xFFFF, 0);
	mt_spm_pmic_wrap_set_cmd(PMIC_WRAP_PHASE_SUSPEND,
			IDX_SP_VSRAM_PWR_ON,
			temp | (1 << MT6351_PMIC_RG_VSRAM_PROC_EN_SHIFT));
	mt_spm_pmic_wrap_set_cmd(PMIC_WRAP_PHASE_SUSPEND,
			IDX_SP_VSRAM_SHUTDOWN,
			temp & ~(1 << MT6351_PMIC_RG_VSRAM_PROC_EN_SHIFT));
#endif
	/* fpr dpd */
	if (!(pwrctrl->pcm_flags & SPM_FLAG_DIS_DPD))
		spm_dpd_init();

#elif defined(CONFIG_ARCH_MT6797)
	/* set PMIC WRAP table for suspend power control */
	pmic_read_interface_nolock(MT6351_PMIC_RG_VCORE_VDIFF_ENLOWIQ_ADDR, &temp, 0xFFFF, 0);
	mt_spm_pmic_wrap_set_cmd(PMIC_WRAP_PHASE_SUSPEND,
			IDX_SP_VCORE_LQ_EN,
			temp | (1 << MT6351_PMIC_RG_VCORE_VDIFF_ENLOWIQ_SHIFT));
	mt_spm_pmic_wrap_set_cmd(PMIC_WRAP_PHASE_SUSPEND,
			IDX_SP_VCORE_LQ_DIS,
			temp & ~(1 << MT6351_PMIC_RG_VCORE_VDIFF_ENLOWIQ_SHIFT));

	pmic_read_interface_nolock(MT6351_PMIC_RG_VCORE_VSLEEP_SEL_ADDR, &temp, 0xFFFF, 0);
	mt_spm_pmic_wrap_set_cmd(PMIC_WRAP_PHASE_SUSPEND,
			IDX_SP_VCORE_VSLEEP_SEL_0P6V,
			temp | (3 << MT6351_PMIC_RG_VCORE_VSLEEP_SEL_SHIFT));
	mt_spm_pmic_wrap_set_cmd(PMIC_WRAP_PHASE_SUSPEND,
			IDX_SP_VCORE_VSLEEP_SEL_0P7V,
			temp & ~(3 << MT6351_PMIC_RG_VCORE_VSLEEP_SEL_SHIFT));
#endif

#if !defined(CONFIG_FPGA_EARLY_PORTING)
	mt_spm_pmic_wrap_set_phase(PMIC_WRAP_PHASE_SUSPEND);
#endif

#if !defined(CONFIG_FPGA_EARLY_PORTING)
	/* for afcdac setting */
	clk_buf_write_afcdac();
#endif

	/* Do more low power setting when MD1/C2K/CONN off */
	if (is_md_c2k_conn_power_off()) {
		__spm_bsi_top_init_setting();

		__spm_backup_pmic_ck_pdn();
	}
}

static void spm_suspend_post_process(struct pwr_ctrl *pwrctrl)
{
	/* Do more low power setting when MD1/C2K/CONN off */
	if (is_md_c2k_conn_power_off())
		__spm_restore_pmic_ck_pdn();

#if defined(CONFIG_ARCH_MT6755) || defined(CONFIG_ARCH_MT6757)
	/* fpr dpd */
	if (!(pwrctrl->pcm_flags & SPM_FLAG_DIS_DPD))
		spm_dpd_dram_init();
#endif

	/* set PMIC WRAP table for normal power control */
#if !defined(CONFIG_FPGA_EARLY_PORTING)
	mt_spm_pmic_wrap_set_phase(PMIC_WRAP_PHASE_NORMAL);
#endif

	__spm_pmic_low_iq_mode(0);

	__spm_pmic_pg_force_off();
}

static void spm_set_sysclk_settle(void)
{
	u32 md_settle, settle;

	/* get MD SYSCLK settle */
	spm_write(SPM_CLK_CON, spm_read(SPM_CLK_CON) | SYS_SETTLE_SEL_LSB);
	spm_write(SPM_CLK_SETTLE, 0);
	md_settle = spm_read(SPM_CLK_SETTLE);

	/* SYSCLK settle = MD SYSCLK settle but set it again for MD PDN */
	spm_write(SPM_CLK_SETTLE, SPM_SYSCLK_SETTLE - md_settle);
	settle = spm_read(SPM_CLK_SETTLE);

	spm_crit2("md_settle = %u, settle = %u\n", md_settle, settle);
}

static void spm_kick_pcm_to_run(struct pwr_ctrl *pwrctrl)
{
	/* enable PCM WDT (normal mode) to start count if needed */
#if SPM_PCMWDT_EN
	if (!pwrctrl->wdt_disable)
		__spm_set_pcm_wdt(1);
#endif

#if defined(CONFIG_ARCH_MT6797)
	if (spm_save_thermal_adc())
		pwrctrl->pcm_flags = pwrctrl->pcm_flags | 0x80000;
#endif
	__spm_kick_pcm_to_run(pwrctrl);
}

static void spm_trigger_wfi_for_sleep(struct pwr_ctrl *pwrctrl)
{
#if 0
	sync_hw_gating_value();	/* for Vcore DVFS */
#endif

	if (is_cpu_pdn(pwrctrl->pcm_flags)) {
		spm_dormant_sta = mt_cpu_dormant(CPU_SHUTDOWN_MODE /* | DORMANT_SKIP_WFI */);
		switch (spm_dormant_sta) {
		case MT_CPU_DORMANT_RESET:
			break;
		case MT_CPU_DORMANT_ABORT:
			break;
		case MT_CPU_DORMANT_BREAK:
			break;
		case MT_CPU_DORMANT_BYPASS:
			break;
		}
	} else {
		spm_dormant_sta = -1;
		spm_write(MP0_AXI_CONFIG, spm_read(MP0_AXI_CONFIG) | ACINACTM);
		spm_write(MP1_AXI_CONFIG, spm_read(MP1_AXI_CONFIG) | ACINACTM);
#if defined(CONFIG_ARCH_MT6797)
		spm_write(MP2_AXI_CONFIG, spm_read(MP2_AXI_CONFIG) | (ACINACTM + 1));
#endif
		wfi_with_sync();
		spm_write(MP0_AXI_CONFIG, spm_read(MP0_AXI_CONFIG) & ~ACINACTM);
		spm_write(MP1_AXI_CONFIG, spm_read(MP1_AXI_CONFIG) & ~ACINACTM);
#if defined(CONFIG_ARCH_MT6797)
		spm_write(MP2_AXI_CONFIG, spm_read(MP2_AXI_CONFIG) & ~(ACINACTM + 1));
#endif
	}

	if (is_infra_pdn(pwrctrl->pcm_flags))
		mtk_uart_restore();
}

static void spm_clean_after_wakeup(void)
{
	/* disable PCM WDT to stop count if needed */
#if SPM_PCMWDT_EN
		__spm_set_pcm_wdt(0);
#endif

	__spm_clean_after_wakeup();
}

static wake_reason_t spm_output_wake_reason(struct wake_status *wakesta, struct pcm_desc *pcmdesc, int vcore_status)
{
	wake_reason_t wr;
	u32 md32_flag = 0;
	u32 md32_flag2 = 0;

	wr = __spm_output_wake_reason(wakesta, pcmdesc, true);

#if 1
	memcpy(&suspend_info[log_wakesta_cnt], wakesta, sizeof(struct wake_status));
	suspend_info[log_wakesta_cnt].log_index = log_wakesta_index;

	if (10 <= log_wakesta_cnt) {
		log_wakesta_cnt = 0;
		spm_snapshot_golden_setting = 0;
	} else {
		log_wakesta_cnt++;
		log_wakesta_index++;
	}

	if (0xFFFFFFF0 <= log_wakesta_index)
		log_wakesta_index = 0;
#endif

	spm_crit2("suspend dormant state = %d, md32_flag = 0x%x, md32_flag2 = %d, vcore_status = %d\n",
		  spm_dormant_sta, md32_flag, md32_flag2, vcore_status);
	if (0 != spm_ap_mdsrc_req_cnt)
		spm_crit2("warning: spm_ap_mdsrc_req_cnt = %d, r7[ap_mdsrc_req] = 0x%x\n",
			  spm_ap_mdsrc_req_cnt, spm_read(SPM_POWER_ON_VAL1) & (1 << 17));

#if !defined(CONFIG_FPGA_EARLY_PORTING)
	if (wakesta->r12 & WAKE_SRC_R12_EINT_EVENT_B)
		mt_eint_print_status();
#endif

#if 0
	if (wakesta->debug_flag & (1 << 18)) {
		spm_crit2("MD32 suspned pmic wrapper error");
		BUG();
	}

	if (wakesta->debug_flag & (1 << 19)) {
		spm_crit2("MD32 resume pmic wrapper error");
		BUG();
	}
#endif

#ifdef CONFIG_MTK_CCCI_DEVICES
	/* if (wakesta->r13 & 0x18) { */
		spm_crit2("dump ID_DUMP_MD_SLEEP_MODE");
		exec_ccci_kern_func_by_md_id(0, ID_DUMP_MD_SLEEP_MODE, NULL, 0);
	/* } */
#endif

#if !defined(CONFIG_FPGA_EARLY_PORTING)
#ifdef CONFIG_MTK_ECCCI_DRIVER
	if (wakesta->r12 & WAKE_SRC_R12_CLDMA_EVENT_B)
		exec_ccci_kern_func_by_md_id(0, ID_GET_MD_WAKEUP_SRC, NULL, 0);
	if (wakesta->r12 & WAKE_SRC_R12_CCIF1_EVENT_B)
		exec_ccci_kern_func_by_md_id(2, ID_GET_MD_WAKEUP_SRC, NULL, 0);
#endif
#endif
	return wr;
}

/*
 * wakesrc: WAKE_SRC_XXX
 * enable : enable or disable @wakesrc
 * replace: if true, will replace the default setting
 */
int spm_set_sleep_wakesrc(u32 wakesrc, bool enable, bool replace)
{
	unsigned long flags;

	if (spm_is_wakesrc_invalid(wakesrc))
		return -EINVAL;

	spin_lock_irqsave(&__spm_lock, flags);
	if (enable) {
		if (replace)
			__spm_suspend.pwrctrl->wake_src = wakesrc;
		else
			__spm_suspend.pwrctrl->wake_src |= wakesrc;
	} else {
		if (replace)
			__spm_suspend.pwrctrl->wake_src = 0;
		else
			__spm_suspend.pwrctrl->wake_src &= ~wakesrc;
	}
	spin_unlock_irqrestore(&__spm_lock, flags);

	return 0;
}

/*
 * wakesrc: WAKE_SRC_XXX
 */
u32 spm_get_sleep_wakesrc(void)
{
	return __spm_suspend.pwrctrl->wake_src;
}

#if SPM_AEE_RR_REC
void spm_suspend_aee_init(void)
{
	aee_rr_rec_spm_suspend_val(0);
}
#endif

#if !defined(CONFIG_FPGA_EARLY_PORTING)
#ifdef CONFIG_MTK_PMIC
/* #include <cust_pmic.h> */
#ifndef DISABLE_DLPT_FEATURE
/* extern int get_dlpt_imix_spm(void); */
int __attribute__((weak)) get_dlpt_imix_spm(void)
{
	return 0;
}
#endif
#endif
#endif

wake_reason_t spm_go_to_sleep(u32 spm_flags, u32 spm_data)
{
	u32 sec = 2;
	/* struct wake_status wakesta; */
	unsigned long flags;
	struct mtk_irq_mask mask;
#ifdef CONFIG_MTK_WD_KICKER
	struct wd_api *wd_api;
	int wd_ret;
#endif
	static wake_reason_t last_wr = WR_NONE;
#if !defined(CONFIG_FPGA_EARLY_PORTING)
	struct pcm_desc *pcmdesc = NULL;
#else
	struct pcm_desc *pcmdesc;
#endif
	struct pwr_ctrl *pwrctrl;
#if !defined(CONFIG_FPGA_EARLY_PORTING)
	int vcore_status;
#endif
	u32 cpu = smp_processor_id();

#if !defined(CONFIG_FPGA_EARLY_PORTING)
#ifdef CONFIG_MTK_PMIC
#ifndef DISABLE_DLPT_FEATURE
	get_dlpt_imix_spm();
#endif
#endif
#endif

#if SPM_AEE_RR_REC
	spm_suspend_aee_init();
	aee_rr_rec_spm_suspend_val(SPM_SUSPEND_ENTER);
#endif

	if (dyna_load_pcm[DYNA_LOAD_PCM_SUSPEND + cpu / 4].ready)
		pcmdesc = &(dyna_load_pcm[DYNA_LOAD_PCM_SUSPEND + cpu / 4].desc);
	else
		BUG();
	spm_crit2("Online CPU is %d, suspend FW ver. is %s\n", cpu, pcmdesc->version);

#if defined(CONFIG_FPGA_EARLY_PORTING)
	pcmdesc = __spm_suspend.pcmdesc;
#endif
	pwrctrl = __spm_suspend.pwrctrl;

	update_pwrctrl_pcm_flags(&spm_flags);
	set_pwrctrl_pcm_flags(pwrctrl, spm_flags);
	set_pwrctrl_pcm_data(pwrctrl, spm_data);

#if SPM_PWAKE_EN
	sec = _spm_get_wake_period(-1 /* FIXME */, last_wr);
#endif
	pwrctrl->timer_val = sec * 32768;

#ifdef CONFIG_MTK_WD_KICKER
	wd_ret = get_wd_api(&wd_api);
	if (!wd_ret) {
		wd_api->wd_spmwdt_mode_config(WD_REQ_EN, WD_REQ_RST_MODE);
		wd_api->wd_suspend_notify();
	} else
		spm_crit2("FAILED TO GET WD API\n");
#endif

	spm_suspend_pre_process(pwrctrl);

#if 0
	/* snapshot golden setting */
	{
		if (!is_already_snap_shot)
			snapshot_golden_setting(__func__, 0);
	}
#endif

	spin_lock_irqsave(&__spm_lock, flags);

	mt_irq_mask_all(&mask);
	mt_irq_unmask_for_sleep(SPM_IRQ0_ID);

#if defined(CONFIG_MTK_SYS_CIRQ)
	mt_cirq_clone_gic();
	mt_cirq_enable();
#endif

	spm_set_sysclk_settle();

	spm_crit2("sec = %u, wakesrc = 0x%x (%u)(%u)\n",
		  sec, pwrctrl->wake_src, is_cpu_pdn(pwrctrl->pcm_flags),
		  is_infra_pdn(pwrctrl->pcm_flags));
	if (request_uart_to_sleep()) {
		last_wr = WR_UART_BUSY;
		goto RESTORE_IRQ;
	}
	__spm_reset_and_init_pcm(pcmdesc);

	__spm_kick_im_to_fetch(pcmdesc);

	__spm_init_pcm_register();

	__spm_init_event_vector(pcmdesc);

	__spm_check_md_pdn_power_control(pwrctrl);

#if !defined(CONFIG_FPGA_EARLY_PORTING)
#if defined(CONFIG_ARCH_MT6755) || defined(CONFIG_ARCH_MT6757)
	__spm_sync_vcore_dvfs_power_control(pwrctrl, __spm_vcore_dvfs.pwrctrl);
#endif

	vcore_status = vcorefs_get_curr_ddr();
#endif

#if defined(CONFIG_ARCH_MT6797)
	pwrctrl->pcm_flags &= ~SPM_FLAG_RUN_COMMON_SCENARIO;
	pwrctrl->pcm_flags &= ~SPM_FLAG_DIS_VCORE_DVS;
	pwrctrl->pcm_flags |= SPM_FLAG_DIS_VCORE_DFS;
#endif

	__spm_set_power_control(pwrctrl);

	__spm_set_wakeup_event(pwrctrl);

	spm_kick_pcm_to_run(pwrctrl);

#if SPM_AEE_RR_REC
	aee_rr_rec_spm_suspend_val(SPM_SUSPEND_ENTER_WFI);
#endif
	spm_trigger_wfi_for_sleep(pwrctrl);
#if SPM_AEE_RR_REC
	aee_rr_rec_spm_suspend_val(SPM_SUSPEND_LEAVE_WFI);
#endif

	/* record last wakesta */
	/* __spm_get_wakeup_status(&wakesta); */
	__spm_get_wakeup_status(&spm_wakesta);

#if defined(CONFIG_ARCH_MT6797)
	spm_wakesta.timer_out += (spm_read(SPM_SCP_MAILBOX) - 1) *
		(1 << SPM_THERMAL_TIMER);
#endif

	spm_clean_after_wakeup();

	request_uart_to_wakeup();

	/* record last wakesta */
	/* last_wr = spm_output_wake_reason(&wakesta, pcmdesc); */
#if !defined(CONFIG_FPGA_EARLY_PORTING)
	last_wr = spm_output_wake_reason(&spm_wakesta, pcmdesc, vcore_status);
#else
	last_wr = spm_output_wake_reason(&spm_wakesta, pcmdesc, 0);
#endif
RESTORE_IRQ:
#if defined(CONFIG_MTK_SYS_CIRQ)
	mt_cirq_flush();
	mt_cirq_disable();
#endif

	mt_irq_mask_restore(&mask);

	spin_unlock_irqrestore(&__spm_lock, flags);

	spm_suspend_post_process(pwrctrl);

#ifdef CONFIG_MTK_WD_KICKER
	if (!wd_ret) {
		if (!pwrctrl->wdt_disable)
			wd_api->wd_resume_notify();
		else
			spm_crit2("pwrctrl->wdt_disable %d\n", pwrctrl->wdt_disable);
		wd_api->wd_spmwdt_mode_config(WD_REQ_DIS, WD_REQ_RST_MODE);
	}
#endif

#if SPM_AEE_RR_REC
	aee_rr_rec_spm_suspend_val(0);
#endif

#if defined(CONFIG_ARCH_MT6797)
	/* Re-kick VCORE DVFS */
	if (last_wr != WR_UART_BUSY)
		if (is_vcorefs_feature_enable()) {
			__spm_backup_vcore_dvfs_dram_shuffle();
			__spm_kick_im_to_fetch(pcmdesc);
			__spm_init_pcm_register();
			__spm_init_event_vector(pcmdesc);
			__spm_check_md_pdn_power_control(pwrctrl);
			__spm_sync_vcore_dvfs_power_control(pwrctrl, __spm_vcore_dvfs.pwrctrl);
			pwrctrl->pcm_flags |= SPM_FLAG_RUN_COMMON_SCENARIO;
			pwrctrl->pcm_flags &= ~SPM_FLAG_DIS_VCORE_DVS;
			pwrctrl->pcm_flags |= SPM_FLAG_DIS_VCORE_DFS;
			__spm_set_power_control(pwrctrl);
			__spm_set_wakeup_event(pwrctrl);
			__spm_set_vcorefs_wakeup_event(__spm_vcore_dvfs.pwrctrl);
			spm_write(PCM_CON1, SPM_REGWR_CFG_KEY | (spm_read(PCM_CON1) & ~PCM_TIMER_EN_LSB));
			__spm_kick_pcm_to_run(pwrctrl);
			spm_crit2("R15: 0x%x\n", spm_read(PCM_REG15_DATA));

#if SPM_AEE_RR_REC
			aee_rr_rec_spm_common_scenario_val(SPM_COMMON_SCENARIO_SUSPEND);
#endif
		}
#endif

#ifdef CONFIG_MTK_USB2JTAG_SUPPORT
	if (usb2jtag_mode())
		mt_usb2jtag_resume();
#endif


	return last_wr;
}

bool spm_is_md_sleep(void)
{
	return !((spm_read(PCM_REG13_DATA) & R13_MD1_SRCCLKENA) |
		 (spm_read(PCM_REG13_DATA) & R13_MD2_SRCCLKENA));
}

bool spm_is_md1_sleep(void)
{
	return !(spm_read(PCM_REG13_DATA) & R13_MD1_SRCCLKENA);
}

bool spm_is_md2_sleep(void)
{
	return !(spm_read(PCM_REG13_DATA) & R13_MD2_SRCCLKENA);
}

bool spm_is_conn_sleep(void)
{
	return !(spm_read(PCM_REG13_DATA) & R13_CONN_SRCCLKENA);
}

void spm_set_wakeup_src_check(void)
{
	/* clean wakeup event raw status */
	spm_write(SPM_WAKEUP_EVENT_MASK, 0xFFFFFFFF);

	/* set wakeup event */
	spm_write(SPM_WAKEUP_EVENT_MASK, ~WAKE_SRC_FOR_SUSPEND);
}

bool spm_check_wakeup_src(void)
{
	u32 wakeup_src;

	/* check wanek event raw status */
	wakeup_src = spm_read(SPM_WAKEUP_STA);

	if (wakeup_src) {
		spm_crit2("WARNING: spm_check_wakeup_src = 0x%x", wakeup_src);
		return 1;
	} else
		return 0;
}

void spm_poweron_config_set(void)
{
	unsigned long flags;

	spin_lock_irqsave(&__spm_lock, flags);
	/* enable register control */
	spm_write(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | BCLK_CG_EN_LSB);
	spin_unlock_irqrestore(&__spm_lock, flags);
}

void spm_md32_sram_con(u32 value)
{
	unsigned long flags;

	spin_lock_irqsave(&__spm_lock, flags);
	/* enable register control */
	spm_write(SCP_SRAM_CON, value);
	spin_unlock_irqrestore(&__spm_lock, flags);
}

#if 0
#define hw_spin_lock_for_ddrdfs()           \
do {                                        \
	spm_write(0xF0050090, 0x8000);          \
} while (!(spm_read(0xF0050090) & 0x8000))

#define hw_spin_unlock_for_ddrdfs()         \
	spm_write(0xF0050090, 0x8000)
#else
#define hw_spin_lock_for_ddrdfs()
#define hw_spin_unlock_for_ddrdfs()
#endif

void spm_ap_mdsrc_req(u8 set)
{
	unsigned long flags;
	u32 i = 0;
	u32 md_sleep = 0;

	if (set) {
		spin_lock_irqsave(&__spm_lock, flags);

		if (spm_ap_mdsrc_req_cnt < 0) {
			spm_crit2("warning: set = %d, spm_ap_mdsrc_req_cnt = %d\n", set,
				  spm_ap_mdsrc_req_cnt);
			/* goto AP_MDSRC_REC_CNT_ERR; */
			spin_unlock_irqrestore(&__spm_lock, flags);
		} else {
			spm_ap_mdsrc_req_cnt++;

			hw_spin_lock_for_ddrdfs();
			spm_write(AP_MDSRC_REQ, spm_read(AP_MDSRC_REQ) | AP_MD1SRC_REQ_LSB);
			hw_spin_unlock_for_ddrdfs();

			spin_unlock_irqrestore(&__spm_lock, flags);

			/* if md_apsrc_req = 1'b0, wait 26M settling time (3ms) */
			if (0 == (spm_read(PCM_REG13_DATA) & R13_MD1_APSRC_REQ)) {
				md_sleep = 1;
				mdelay(3);
			}

			/* Check ap_mdsrc_ack = 1'b1 */
			while (0 == (spm_read(AP_MDSRC_REQ) & AP_MD1SRC_ACK_LSB)) {
				if (10 > i++) {
					mdelay(1);
				} else {
					spm_crit2
					    ("WARNING: MD SLEEP = %d, spm_ap_mdsrc_req CAN NOT polling AP_MD1SRC_ACK\n",
					     md_sleep);
					/* goto AP_MDSRC_REC_CNT_ERR; */
					break;
				}
			}
		}
	} else {
		spin_lock_irqsave(&__spm_lock, flags);

		spm_ap_mdsrc_req_cnt--;

		if (spm_ap_mdsrc_req_cnt < 0) {
			spm_crit2("warning: set = %d, spm_ap_mdsrc_req_cnt = %d\n", set,
				  spm_ap_mdsrc_req_cnt);
			/* goto AP_MDSRC_REC_CNT_ERR; */
		} else {
			if (0 == spm_ap_mdsrc_req_cnt) {
				hw_spin_lock_for_ddrdfs();
				spm_write(AP_MDSRC_REQ, spm_read(AP_MDSRC_REQ) & ~AP_MD1SRC_REQ_LSB);
				hw_spin_unlock_for_ddrdfs();
			}
		}

		spin_unlock_irqrestore(&__spm_lock, flags);
	}

/* AP_MDSRC_REC_CNT_ERR: */
/* spin_unlock_irqrestore(&__spm_lock, flags); */
}

bool spm_set_suspned_pcm_init_flag(u32 *suspend_flags)
{
	return true;
}

void spm_output_sleep_option(void)
{
	spm_notice("PWAKE_EN:%d, PCMWDT_EN:%d, BYPASS_SYSPWREQ:%d\n",
		   SPM_PWAKE_EN, SPM_PCMWDT_EN, SPM_BYPASS_SYSPWREQ);
}

uint32_t get_suspend_debug_regs(uint32_t index)
{
	uint32_t value = 0;

	switch (index) {
	case 0:
#if defined(CONFIG_ARCH_MT6755)
		value = 5;
#elif defined(CONFIG_ARCH_MT6797)
		value = 4;
#elif defined(CONFIG_ARCH_MT6797)
		value = 6;
#endif
		spm_crit("SPM Suspend debug regs count = 0x%.8x\n",  value);
	break;
	case 1:
		value = spm_read(PCM_WDT_LATCH_0);
		spm_crit("SPM Suspend debug regs(0x%x) = 0x%.8x\n", index, value);
	break;
	case 2:
		value = spm_read(PCM_WDT_LATCH_1);
		spm_crit("SPM Suspend debug regs(0x%x) = 0x%.8x\n", index, value);
	break;
	case 3:
		value = spm_read(PCM_WDT_LATCH_2);
		spm_crit("SPM Suspend debug regs(0x%x) = 0x%.8x\n", index, value);
	break;
	case 4:
		value = spm_read(PCM_WDT_LATCH_3);
		spm_crit("SPM Suspend debug regs(0x%x) = 0x%.8x\n", index, value);
	break;
	case 5:
		value = spm_read(DRAMC_DBG_LATCH);
		spm_crit("SPM Suspend debug regs(0x%x) = 0x%.8x\n", index, value);
	break;
#if defined(CONFIG_ARCH_MT6797)
	case 6:
		value = spm_read(PCM_WDT_LATCH_4);
		spm_crit("SPM Suspend debug regs(0x%x) = 0x%.8x\n", index, value);
	break;
#endif
	}

	return value;
}
EXPORT_SYMBOL(get_suspend_debug_regs);

/* record last wakesta */
u32 spm_get_last_wakeup_src(void)
{
	return spm_wakesta.r12;
}

u32 spm_get_last_wakeup_misc(void)
{
	return spm_wakesta.wake_misc;
}
MODULE_DESCRIPTION("SPM-Sleep Driver v0.1");
