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

unsigned int reg_dump_addr_off[] = {
	0x0000,
	0x0004,
	0x0008,
	0x000C,
	0x0010,
	0x0014,
	0x0018,
	0x001c,
	0x0024,
	0x0028,
	0x002c,
	0x0030,
	0x0034,
	0x0038,
	0x003c,
	0x0040,
	0x0044,
	0x0048,
	0x004c,
	0x0050,
	0x0054,
	0x0058,
	0x005c,
	0x0060,
	0x0064,
	0x0068,
	0x006c,
	0x0070,
	0x0074,
	0x0078,
	0x007c,
	0x0080,
	0x0084,
	0x0088,
	0x008c,
	0x0090,
	0x0094,
	0x0098,
	0x00a0,
	0x00a4,
	0x00a8,
	0x00B0,
	0x00B4,
	0x00B8,
	0x00BC,
	0x00C0,
	0x00C4,
	0x00C8,
	0x00CC,
	0x00F0,
	0x00F4,
	0x00F8,
	0x00FC,
	0x0200,
	0x0204,
	0x0208,
	0x020C,
	0x0210,
	0x0214,
	0x0218,
	0x021C,
	0x0220,
	0x0224,
	0x0228,
	0x022C,
	0x0230,
	0x0234,
	0x0238,
	0x023C,
	0x0240,
	0x0244,
	0x0248,
	0x024C,
	0x0250,
	0x0254,
	0x0258,
	0x025C,
	0x0260,
	0x0264,
	0x0268,
	0x026C,
	0x0270,
	0x0274,
	0x0278,
	0x027C,
	0x0280,
	0x0284,
	0x0400,
	0x0404,
	0x0408,
	0x040C,
	0x0410,
	0x0414,
	0x0418,
	0x041C,
	0x0420,
	0x0424,
	0x0428,
	0x042C,
	0x0430,
};

unsigned int littleFreq_FY[8] = {1001000, 910000, 819000, 689000, 598000, 494000, 338000, 156000};

#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
	#if defined(CONFIG_MTK_MT6750TT)
		unsigned int bigFreq_FY[8] = {1807000, 1430000, 1352000, 1196000, 1027000, 871000, 663000, 286000};
	#else
		unsigned int bigFreq_FY[8] = {1508000, 1430000, 1352000, 1196000, 1027000, 871000, 663000, 286000};
	#endif
#else
unsigned int bigFreq_FY[8] = {1807000, 1651000, 1495000, 1196000, 1027000, 871000, 663000, 286000};
unsigned int bigFreq_FY_M[8] = {1807000, 1651000, 1495000, 1196000, 1098000, 871000, 663000, 286000};
#endif

unsigned int littleFreq_SB[8] = {1144000, 1014000, 871000, 689000, 598000, 494000, 338000, 156000};
unsigned int bigFreq_SB[8] = {1950000, 1755000, 1573000, 1196000, 1027000, 871000, 663000, 286000};

unsigned int littleFreq_P15[8] = {1248000, 1079000, 910000, 689000, 598000, 494000, 338000, 156000};
unsigned int bigFreq_P15[8] = {2145000, 1911000, 1664000, 1196000, 1027000, 871000, 663000, 286000};

unsigned int gpuFreq[8] = {728000, 650000, 598000, 520000, 468000, 429000, 390000, 351000};


/*
[31:27] CCI dcmdiv
[26:22] CCI div
[21:17] dcm div
[16:12] clkdiv; ARM PLL Divider
[11:9] post dif
[8:0] DDS[20:12]

[13:7] Vsram pmic value
[6:0] Vproc pmic value
*/
#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)

unsigned int fyTbl[][9] = {
/* CCI dcmdiv, CCI_div, dcm_div, clk_div, post_div, DDS, Vsram, Vproc */
	{0x09, 0xa, 0x13, 0x8, 0x1, 0x134, 0x0, 0x58, 0x58},/* 1001 (LL) */
	{0x08, 0xa, 0x11, 0x8, 0x1, 0x118, 0x0, 0x58, 0x52},/* 910 */
	{0x07, 0xa, 0x0f, 0x8, 0x1, 0x0fc, 0x0, 0x58, 0x4c},/* 819 */
	{0x06, 0xa, 0x0D, 0x8, 0x1, 0x0d4, 0x0, 0x50, 0x40},/* 689 */
	{0x05, 0xa, 0x0B, 0x8, 0x1, 0x0b8, 0x0, 0x4b, 0x3b},/* 598 */
	{0x04, 0xb, 0x09, 0xa, 0x1, 0x130, 0x0, 0x46, 0x36},/* 494 */
	{0x03, 0xb, 0x06, 0xa, 0x1, 0x0d0, 0x0, 0x40, 0x30},/* 338 */
	{0x01, 0x1d, 0x03, 0xb, 0x1, 0x0c0, 0x0, 0x35, 0x20},/* 156 */
	{0x0e, 0xa, 0x1d, 0x8, 0x0, 0x0e8, 0x0, 0x58, 0x58},/* 1508 (L) */
	{0x0d, 0xa, 0x1b, 0x8, 0x0, 0x0dc, 0x0, 0x58, 0x52},/* 1430 */
	{0x0d, 0xa, 0x1a, 0x8, 0x0, 0x0d0, 0x0, 0x58, 0x4c},/* 1352 */
	{0x0b, 0xa, 0x17, 0x8, 0x0, 0x0b8, 0x0, 0x50, 0x40},/* 1196 */
	{0x09, 0xa, 0x13, 0x8, 0x1, 0x13c, 0x0, 0x4b, 0x3b},/* 1027 */
	{0x08, 0xa, 0x10, 0x8, 0x1, 0x10c, 0x0, 0x46, 0x36},/* 871 */
	{0x06, 0xa, 0x0c, 0x8, 0x1, 0x0cc, 0x0, 0x40, 0x30},/* 663 */
	{0x02, 0xb, 0x05, 0xa, 0x1, 0x0B0, 0x0, 0x35, 0x20},/* 286 */
};

unsigned int sbTbl[][9] = {
	{0x0b, 0xa, 0x16, 0x8, 0x1, 0x160, 0x0, 0x58, 0x58},/* 1144 (LL) */
	{0x09, 0xa, 0x13, 0x8, 0x1, 0x138, 0x0, 0x58, 0x52},/* 1014 */
	{0x08, 0xa, 0x10, 0x8, 0x1, 0x10c, 0x0, 0x58, 0x4c},/* 871 */
	{0x06, 0xa, 0x0D, 0x8, 0x1, 0x0d4, 0x0, 0x50, 0x40},/* 689 */
	{0x05, 0xa, 0x0B, 0x8, 0x1, 0x0b8, 0x0, 0x4b, 0x3b},/* 598 */
	{0x04, 0xb, 0x09, 0xa, 0x1, 0x130, 0x0, 0x46, 0x36},/* 494 */
	{0x03, 0xb, 0x06, 0xa, 0x1, 0x0d0, 0x0, 0x40, 0x30},/* 338 */
	{0x01, 0x1d, 0x03, 0xb, 0x1, 0x0c0, 0x0, 0x35, 0x20},/* 156 */
	{0x12, 0xa, 0x1f, 0x8, 0x0, 0x12c, 0x0, 0x58, 0x58},/* 1950 (L) */
	{0x10, 0xa, 0x1f, 0x8, 0x0, 0x10e, 0x0, 0x58, 0x52},/* 1755 */
	{0x0f, 0xa, 0x1e, 0x8, 0x0, 0x0f2, 0x0, 0x58, 0x4c},/* 1573 */
	{0x0b, 0xa, 0x17, 0x8, 0x0, 0x0b8, 0x0, 0x50, 0x40},/* 1196 */
	{0x09, 0xa, 0x13, 0x8, 0x1, 0x13c, 0x0, 0x4b, 0x3b},/* 1027 */
	{0x08, 0xa, 0x10, 0x8, 0x1, 0x10c, 0x0, 0x46, 0x36},/* 871 */
	{0x06, 0xa, 0x0c, 0x8, 0x1, 0x0cc, 0x0, 0x40, 0x30},/* 663 */
	{0x02, 0xb, 0x05, 0xa, 0x1, 0x0B0, 0x0, 0x35, 0x20},/* 286 */
};
static unsigned int *tTbl;

#else
unsigned int fyTbl[][9] = {
/* CCI dcmdiv, CCI_div, dcm_div, clk_div, post_div, DDS, Vsram, Vproc */
	{0x09, 0xa, 0x13, 0x8, 0x1, 0x134, 0xc51, 0x58, 0x58},/* 1001 (LL) */
	{0x08, 0xa, 0x11, 0x8, 0x1, 0x118, 0xb33, 0x58, 0x52},/* 910 */
	{0x07, 0xa, 0x0f, 0x8, 0x1, 0x0fc, 0xa14, 0x58, 0x4c},/* 819 */
	{0x06, 0xa, 0x0D, 0x8, 0x1, 0x0d4, 0x87a, 0x50, 0x40},/* 689 */
	{0x05, 0xa, 0x0B, 0x8, 0x1, 0x0b8, 0x75c, 0x4b, 0x3b},/* 598 */
	{0x02, 0xb, 0x09, 0xa, 0x1, 0x130, 0xc28, 0x46, 0x36},/* 494 */
	{0x01, 0xb, 0x06, 0xa, 0x1, 0x0d0, 0x851, 0x40, 0x30},/* 338 */
	{0x00, 0x1d, 0x03, 0xb, 0x1, 0x0c0, 0x7ae, 0x35, 0x20},/* 156 */
	{0x11, 0xa, 0x1f, 0x8, 0x0, 0x116, 0xb1e, 0x58, 0x58},/* 1807 (L) */
	{0x0f, 0xa, 0x1f, 0x8, 0x0, 0x0fe, 0xA28, 0x58, 0x52},/* 1651 */
	{0x0e, 0xa, 0x1c, 0x8, 0x0, 0x0e6, 0x933, 0x58, 0x4c},/* 1495 */
	{0x0b, 0xa, 0x17, 0x8, 0x0, 0x0b8, 0x75c, 0x50, 0x40},/* 1196 */
	{0x09, 0xa, 0x13, 0x8, 0x1, 0x13c, 0xca3, 0x4b, 0x3b},/* 1027 */
	{0x08, 0xa, 0x10, 0x8, 0x1, 0x10c, 0xab8, 0x46, 0x36},/* 871 */
	{0x06, 0xa, 0x0c, 0x8, 0x1, 0x0cc, 0x828, 0x40, 0x30},/* 663 */
	{0x02, 0xb, 0x05, 0xa, 0x1, 0x0B0, 0x70a, 0x35, 0x20},/* 286 */
};

unsigned int fyTbl_M[][9] = {
/* CCI dcmdiv, CCI_div, dcm_div, clk_div, post_div, DDS, Vsram, Vproc */
	{0x09, 0xa, 0x13, 0x8, 0x1, 0x134, 0xc51, 0x58, 0x58},/* 1001 (LL) */
	{0x08, 0xa, 0x11, 0x8, 0x1, 0x118, 0xb33, 0x58, 0x52},/* 910 */
	{0x07, 0xa, 0x0f, 0x8, 0x1, 0x0fc, 0xa14, 0x58, 0x4c},/* 819 */
	{0x06, 0xa, 0x0D, 0x8, 0x1, 0x0d4, 0x87a, 0x50, 0x40},/* 689 */
	{0x05, 0xa, 0x0B, 0x8, 0x1, 0x0b8, 0x75c, 0x4b, 0x3b},/* 598 */
	{0x02, 0xb, 0x09, 0xa, 0x1, 0x130, 0xc28, 0x46, 0x36},/* 494 */
	{0x01, 0xb, 0x06, 0xa, 0x1, 0x0d0, 0x851, 0x40, 0x30},/* 338 */
	{0x00, 0x1d, 0x03, 0xb, 0x1, 0x0c0, 0x7ae, 0x35, 0x20},/* 156 */
	{0x11, 0xa, 0x1f, 0x8, 0x0, 0x116, 0xb1e, 0x58, 0x58},/* 1807 (L) */
	{0x0f, 0xa, 0x1f, 0x8, 0x0, 0x0fe, 0xA28, 0x58, 0x52},/* 1651 */
	{0x0e, 0xa, 0x1c, 0x8, 0x0, 0x0e6, 0x933, 0x58, 0x4c},/* 1495 */
	{0x0b, 0xa, 0x17, 0x8, 0x0, 0x0b8, 0x75c, 0x50, 0x40},/* 1196 */
	{0x09, 0xa, 0x13, 0x8, 0x0, 0x09e, 0xca3, 0x4b, 0x3b},/* 1027 */
	{0x08, 0xa, 0x10, 0x8, 0x1, 0x10c, 0xab8, 0x46, 0x36},/* 871 */
	{0x06, 0xa, 0x0c, 0x8, 0x1, 0x0cc, 0x828, 0x40, 0x30},/* 663 */
	{0x02, 0xb, 0x05, 0xa, 0x1, 0x0B0, 0x70a, 0x35, 0x20},/* 286 */
};

unsigned int sbTbl[][9] = {
	{0x0b, 0xa, 0x16, 0x8, 0x1, 0x160, 0xe14, 0x58, 0x58},/* 1144 (LL) */
	{0x09, 0xa, 0x13, 0x8, 0x1, 0x138, 0xc7a, 0x58, 0x52},/* 1014 */
	{0x08, 0xa, 0x10, 0x8, 0x1, 0x10c, 0xab8, 0x58, 0x4c},/* 871 */
	{0x06, 0xa, 0x0D, 0x8, 0x1, 0x0d4, 0x87a, 0x50, 0x40},/* 689 */
	{0x05, 0xa, 0x0B, 0x8, 0x1, 0x0b8, 0x75c, 0x4b, 0x3b},/* 598 */
	{0x02, 0xb, 0x09, 0xa, 0x1, 0x130, 0xc28, 0x46, 0x36},/* 494 */
	{0x01, 0xb, 0x06, 0xa, 0x1, 0x0d0, 0x851, 0x40, 0x30},/* 338 */
	{0x00, 0x1d, 0x03, 0xb, 0x1, 0x0c0, 0x7ae, 0x35, 0x20},/* 156 */
	{0x12, 0xa, 0x1f, 0x8, 0x0, 0x12c, 0xc00, 0x58, 0x58},/* 1950 (L) */
	{0x10, 0xa, 0x1f, 0x8, 0x0, 0x10e, 0xacc, 0x58, 0x52},/* 1755 */
	{0x0f, 0xa, 0x1e, 0x8, 0x0, 0x0f2, 0x9ae, 0x58, 0x4c},/* 1573 */
	{0x0b, 0xa, 0x17, 0x8, 0x0, 0x0b8, 0x75c, 0x50, 0x40},/* 1196 */
	{0x09, 0xa, 0x13, 0x8, 0x1, 0x13c, 0xca3, 0x4b, 0x3b},/* 1027 */
	{0x08, 0xa, 0x10, 0x8, 0x1, 0x10c, 0xab8, 0x46, 0x36},/* 871 */
	{0x06, 0xa, 0x0c, 0x8, 0x1, 0x0cc, 0x828, 0x40, 0x30},/* 663 */
	{0x02, 0xb, 0x05, 0xa, 0x1, 0x0B0, 0x70a, 0x35, 0x20},/* 286 */
};

unsigned int p15Tbl[][9] = {
	{0x0c, 0xa, 0x18, 0x8, 0x1, 0x180, 0xe14, 0x58, 0x58},/* 1248 (LL) */
	{0x0a, 0xa, 0x14, 0x8, 0x1, 0x14c, 0xc7a, 0x58, 0x52},/* 1079 */
	{0x08, 0xa, 0x11, 0x8, 0x1, 0x118, 0xab8, 0x58, 0x4c},/* 910 */
	{0x06, 0xa, 0x0D, 0x8, 0x1, 0x0d4, 0x87a, 0x50, 0x40},/* 689 */
	{0x05, 0xa, 0x0B, 0x8, 0x1, 0x0b8, 0x75c, 0x4b, 0x3b},/* 598 */
	{0x02, 0xb, 0x09, 0xa, 0x1, 0x130, 0xc28, 0x46, 0x36},/* 494 */
	{0x01, 0xb, 0x06, 0xa, 0x1, 0x0d0, 0x851, 0x40, 0x30},/* 338 */
	{0x00, 0x1d, 0x03, 0xb, 0x1, 0x0c0, 0x7ae, 0x35, 0x20},/* 156 */
	{0x14, 0xa, 0x1f, 0x8, 0x0, 0x14A, 0xc00, 0x58, 0x58},/* 2145 (L) */
	{0x12, 0xa, 0x1f, 0x8, 0x0, 0x126, 0xacc, 0x58, 0x52},/* 1911 */
	{0x10, 0xa, 0x1e, 0x8, 0x0, 0x100, 0x9ae, 0x58, 0x4c},/* 1664 */
	{0x0b, 0xa, 0x17, 0x8, 0x0, 0x0b8, 0x75c, 0x50, 0x40},/* 1196 */
	{0x09, 0xa, 0x13, 0x8, 0x1, 0x13c, 0xca3, 0x4b, 0x3b},/* 1027 */
	{0x08, 0xa, 0x10, 0x8, 0x1, 0x10c, 0xab8, 0x46, 0x36},/* 871 */
	{0x06, 0xa, 0x0c, 0x8, 0x1, 0x0cc, 0x828, 0x40, 0x30},/* 663 */
	{0x02, 0xb, 0x05, 0xa, 0x1, 0x0B0, 0x70a, 0x35, 0x20},/* 286 */
};
#endif

unsigned int gpuSb[8] = {0x54, 0x54, 0x54, 0x40, 0x40, 0x40, 0x40, 0x35};
unsigned int gpuFy[8] = {0x54, 0x40, 0x40, 0x40, 0x40, 0x35, 0x00, 0x00};
static unsigned int record_tbl_locked[8];
static unsigned int *recordTbl;
static unsigned int cpu_speed;

/**
 * @file    mt_ptp.c
 * @brief   Driver for EEM
 *
 */

#define __MT_EEM_C__

/* Early porting use that avoid to cause compiler error */
/* #define EARLY_PORTING */

/*=============================================================
 * Include files
 *=============================================================*/

/* system includes */
#include <linux/init.h>
#include <linux/cpu.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/spinlock.h>
#include <linux/kthread.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/interrupt.h>
#include <linux/syscore_ops.h>
#include <linux/platform_device.h>
#include <linux/completion.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/delay.h>

/* project includes */
/* #include "mach/mt_reg_base.h" */
/* #include "mach/mt_typedefs.h" */

#include "mach/irqs.h"
/* #include "mach/mt_irq.h" */
#include "mt_ptp.h"
#include "mt_cpufreq.h"
/* #include "mach/mt_spm_idle.h"
#include "mach/mt_pmic_wrap.h"
#include "mach/mt_clkmgr.h" */
#include "mach/mt_freqhopping.h"
#include "mach/mtk_rtc_hal.h"
#include "mach/mt_rtc_hw.h"

#ifdef CONFIG_OF
	#include <linux/of.h>
	#include <linux/of_irq.h>
	#include <linux/of_address.h>
	#include <linux/of_fdt.h>
	#include <mt-plat/aee.h>
	#if defined(CONFIG_MTK_CLKMGR)
		#include <mach/mt_clkmgr.h>
	#else
		#include <linux/clk.h>
	#endif
#endif

/* local includes */
#include "mt_defptp.h"
#ifdef __KERNEL__
	#include <mt-plat/mt_chip.h>
	#include <mt-plat/mt_gpio.h>
	#include "mt-plat/upmu_common.h"
	#include "mach/mt_thermal.h"
	#include "mach/mt_ppm_api.h"
	#include "mt_gpufreq.h"
	#include "../../../power/mt6755/mt6311.h"
	#if defined(CONFIG_MTK_PMIC_CHIP_MT6353) && defined(CONFIG_MTK_MT6750TT)
		#include "mt_eem_turbo.h"
	#endif
#else
	#include "mach/mt_ptpslt.h"
	#include "kernel2ctp.h"
	#include "ptp.h"
	#include "upmu_common.h"
	#include "mt6311.h"
	#ifndef EARLY_PORTING
		#include "gpu_dvfs.h"
		#include "thermal.h"
		/* #include "gpio_const.h" */
	#endif
#endif

#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
static unsigned int ctrl_ITurbo = 0, ITurboRun, fab, segment;
unsigned int cpuBinLevel, cpuBinLevel_eng;
#endif
static int eem_log_en;

/* Global variable for slow idle*/
volatile unsigned int ptp_data[3] = {0, 0, 0};


struct eem_det;
struct eem_ctrl;
u32 *recordRef;

static void eem_set_eem_volt(struct eem_det *det);
static void eem_restore_eem_volt(struct eem_det *det);

#if 0
static void eem_init01_prepare(struct eem_det *det);
static void eem_init01_finish(struct eem_det *det);
#endif

/*=============================================================
 * Macro definition
 *=============================================================*/

/*
 * CONFIG (SW related)
 */
/* #define CONFIG_EEM_SHOWLOG */

#define EN_ISR_LOG		(0)

#define EEM_GET_REAL_VAL	(1) /* get val from efuse */
#define SET_PMIC_VOLT		(1) /* apply PMIC voltage */

#define DUMP_DATA_TO_DE		(1)

#define LOG_INTERVAL		(2LL * NSEC_PER_SEC)
#define NR_FREQ			8


/*
 * 100 us, This is the EEM Detector sampling time as represented in
 * cycles of bclk_ck during INIT. 52 MHz
 */
#define DETWINDOW_VAL		0x514

/*
 * mili Volt to config value. voltage = 600mV + val * 6.25mV
 * val = (voltage - 600) / 6.25
 * @mV:	mili volt
 */
#if 1
/* from Brian Yang,1mV=>10uV */
#define EEM_VOLT_TO_PMIC_VAL(volt)  (((volt) - 70000 + 625 - 1) / 625) /* ((((volt) - 700 * 100 + 625 - 1) / 625) */

/* pmic value from EEM already add 16 steps(16*6.25=100mV) for real voltage transfer */
#define EEM_PMIC_VAL_TO_VOLT(pmic)  (((pmic) * 625) + 60000) /* (((pmic) * 625) / 100 + 600) */
#else
#define MV_TO_VAL(MV)		((((MV) - 600) * 100 + 625 - 1) / 625) /* TODO: FIXME, refer to VOLT_TO_PMIC_VAL() */
#define VAL_TO_MV(VAL)		(((VAL) * 625) / 100 + 600) /* TODO: FIXME, refer to PMIC_VAL_TO_VOLT() */
#endif
/* offset 0x10(16 steps) for CPU/GPU DVFS */
#define EEM_PMIC_OFFSET (0x10)

/* I-Chang */
#define VMAX_VAL		EEM_VOLT_TO_PMIC_VAL(115000)
#define VMIN_VAL		EEM_VOLT_TO_PMIC_VAL(80000)
#define VMIN_SRAM               EEM_VOLT_TO_PMIC_VAL(93000)
#define VMAX_VAL_GPU            EEM_VOLT_TO_PMIC_VAL(112500)
#define VMIN_VAL_GPU		EEM_VOLT_TO_PMIC_VAL(90000)
#if 0
#define VMAX_VAL_SOC		EEM_VOLT_TO_PMIC_VAL(125000)
#define VMIN_VAL_SOC		EEM_VOLT_TO_PMIC_VAL(115000)
#define VMAX_VAL_LTE		EEM_VOLT_TO_PMIC_VAL(105000)
#define VMIN_VAL_LTE		EEM_VOLT_TO_PMIC_VAL(93000)
#endif

#define DTHI_VAL		0x01		/* positive */
#define DTLO_VAL		0xfe		/* negative (2's compliment) */
#define DETMAX_VAL		0xffff		/* This timeout value is in cycles of bclk_ck. */
#define AGECONFIG_VAL		0x555555
#define AGEM_VAL		0x0
#define DVTFIXED_VAL		0x6
#define DVTFIXED_VAL_GPU	0x4
#define DVTFIXED_VAL_SOC	0x4
#define DVTFIXED_VAL_LTE	0x4
#define VCO_VAL			0x10
#define VCO_VAL_GPU		0x20
#define VCO_VAL_SOC		0x28
#define VCO_VAL_LTE		0x25
#define DCCONFIG_VAL		0x555555

#if defined(CONFIG_EEM_AEE_RR_REC) && !defined(EARLY_PORTING)
static void _mt_eem_aee_init(void)
{
	aee_rr_rec_ptp_cpu_little_volt(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_cpu_big_volt(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_gpu_volt(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_temp(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_status(0xFF);
}
#endif

/*
 * bit operation
 */
#undef  BIT
#define BIT(bit)	(1U << (bit))

#define MSB(range)	(1 ? range)
#define LSB(range)	(0 ? range)
/**
 * Genearte a mask wher MSB to LSB are all 0b1
 * @r:	Range in the form of MSB:LSB
 */
#define BITMASK(r)	\
	(((unsigned) -1 >> (31 - MSB(r))) & ~((1U << LSB(r)) - 1))

/**
 * Set value at MSB:LSB. For example, BITS(7:3, 0x5A)
 * will return a value where bit 3 to bit 7 is 0x5A
 * @r:	Range in the form of MSB:LSB
 */
/* BITS(MSB:LSB, value) => Set value at MSB:LSB  */
#define BITS(r, val)	((val << LSB(r)) & BITMASK(r))

#define GET_BITS_VAL(_bits_, _val_)   (((_val_) & (BITMASK(_bits_))) >> ((0) ? _bits_))
/*
 * LOG
 */
#ifdef __KERNEL__
#define EEM_TAG     "[xxxx1] "
#ifdef USING_XLOG
	#include <linux/xlog.h>
	#define eem_emerg(fmt, args...)     pr_err(ANDROID_LOG_ERROR, EEM_TAG, fmt, ##args)
	#define eem_alert(fmt, args...)     pr_err(ANDROID_LOG_ERROR, EEM_TAG, fmt, ##args)
	#define eem_crit(fmt, args...)      pr_err(ANDROID_LOG_ERROR, EEM_TAG, fmt, ##args)
	#define eem_error(fmt, args...)     pr_err(ANDROID_LOG_ERROR, EEM_TAG, fmt, ##args)
	#define eem_warning(fmt, args...)   pr_warn(ANDROID_LOG_WARN, EEM_TAG, fmt, ##args)
	#define eem_notice(fmt, args...)    pr_info(ANDROID_LOG_INFO, EEM_TAG, fmt, ##args)
	#define eem_info(fmt, args...)      pr_info(ANDROID_LOG_INFO, EEM_TAG, fmt, ##args)
	#define eem_debug(fmt, args...)     pr_debug(ANDROID_LOG_DEBUG, EEM_TAG, fmt, ##args)
#else
	#define eem_emerg(fmt, args...)     pr_emerg(EEM_TAG fmt, ##args)
	#define eem_alert(fmt, args...)     pr_alert(EEM_TAG fmt, ##args)
	#define eem_crit(fmt, args...)      pr_crit(EEM_TAG fmt, ##args)
	#define eem_error(fmt, args...)     pr_err(EEM_TAG fmt, ##args)
	#define eem_warning(fmt, args...)   pr_warn(EEM_TAG fmt, ##args)
	#define eem_notice(fmt, args...)    pr_notice(EEM_TAG fmt, ##args)
	#define eem_info(fmt, args...)      pr_info(EEM_TAG fmt, ##args)
	#define eem_debug(fmt, args...)     /* pr_debug(EEM_TAG fmt, ##args) */
#endif

	#if EN_ISR_LOG /* For Interrupt use */
		#define eem_isr_info(fmt, args...)  eem_debug(fmt, ##args)
	#else
		#define eem_isr_info(fmt, args...)
	#endif
#endif

#define FUNC_LV_MODULE          BIT(0)  /* module, platform driver interface */
#define FUNC_LV_CPUFREQ         BIT(1)  /* cpufreq driver interface          */
#define FUNC_LV_API             BIT(2)  /* mt_cpufreq driver global function */
#define FUNC_LV_LOCAL           BIT(3)  /* mt_cpufreq driver lcaol function  */
#define FUNC_LV_HELP            BIT(4)  /* mt_cpufreq driver help function   */


#if defined(CONFIG_EEM_SHOWLOG)
	static unsigned int func_lv_mask =
		(FUNC_LV_MODULE | FUNC_LV_CPUFREQ | FUNC_LV_API | FUNC_LV_LOCAL | FUNC_LV_HELP);
	#define FUNC_ENTER(lv)	\
		do { if ((lv) & func_lv_mask) eem_debug(">> %s()\n", __func__); } while (0)
	#define FUNC_EXIT(lv)	\
		do { if ((lv) & func_lv_mask) eem_debug("<< %s():%d\n", __func__, __LINE__); } while (0)
#else
	#define FUNC_ENTER(lv)
	#define FUNC_EXIT(lv)
#endif /* CONFIG_CPU_DVFS_SHOWLOG */

/*
 * REG ACCESS
 */
#ifdef __KERNEL__
	#define eem_read(addr)	DRV_Reg32(addr)
	#define eem_read_field(addr, range)	\
		((eem_read(addr) & BITMASK(range)) >> LSB(range))
	#define eem_write(addr, val)	mt_reg_sync_writel(val, addr)
#endif
/**
 * Write a field of a register.
 * @addr:	Address of the register
 * @range:	The field bit range in the form of MSB:LSB
 * @val:	The value to be written to the field
 */
#define eem_write_field(addr, range, val)	\
	eem_write(addr, (eem_read(addr) & ~BITMASK(range)) | BITS(range, val))

/**
 * Helper macros
 */

/* EEM detector is disabled by who */
enum {
	BY_PROCFS	= BIT(0),
	BY_INIT_ERROR	= BIT(1),
	BY_MON_ERROR	= BIT(2),
	BY_PROCFS_INIT2 = BIT(3),
	BY_VOLT_SET	= BIT(4),
	BY_FREQ_SET	= BIT(5)
};

#ifdef CONFIG_OF
void __iomem *eem_base;
static u32 eem_irq_number;
#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
	void __iomem *eem_apmixed_base;
	#define cpu_eem_is_extbuck_valid()     (is_ext_buck_exist() && is_ext_buck_sw_ready())
	static unsigned int eem_is_extbuck_valid;
	static unsigned int eemFeatureSts = 0x5;
#else
	static unsigned int eemFeatureSts = 0x7;
#endif
#endif

/**
 * iterate over list of detectors
 * @det:	the detector * to use as a loop cursor.
 */
#define for_each_det(det) for (det = eem_detectors; det < (eem_detectors + ARRAY_SIZE(eem_detectors)); det++)

/**
 * iterate over list of detectors and its controller
 * @det:	the detector * to use as a loop cursor.
 * @ctrl:	the eem_ctrl * to use as ctrl pointer of current det.
 */
#define for_each_det_ctrl(det, ctrl)				\
	for (det = eem_detectors,				\
	     ctrl = id_to_eem_ctrl(det->ctrl_id);		\
	     det < (eem_detectors + ARRAY_SIZE(eem_detectors)); \
	     det++,						\
	     ctrl = id_to_eem_ctrl(det->ctrl_id))

/**
 * iterate over list of controllers
 * @pos:	the eem_ctrl * to use as a loop cursor.
 */
#define for_each_ctrl(ctrl) for (ctrl = eem_ctrls; ctrl < (eem_ctrls + ARRAY_SIZE(eem_ctrls)); ctrl++)

/**
 * Given a eem_det * in eem_detectors. Return the id.
 * @det:	pointer to a eem_det in eem_detectors
 */
#define det_to_id(det)	((det) - &eem_detectors[0])

/**
 * Given a eem_ctrl * in eem_ctrls. Return the id.
 * @det:	pointer to a eem_ctrl in eem_ctrls
 */
#define ctrl_to_id(ctrl)	((ctrl) - &eem_ctrls[0])

/**
 * Check if a detector has a feature
 * @det:	pointer to a eem_det to be check
 * @feature:	enum eem_features to be checked
 */
#define HAS_FEATURE(det, feature)	((det)->features & feature)

#define PERCENT(numerator, denominator)	\
	(unsigned char)(((numerator) * 100 + (denominator) - 1) / (denominator))

/*=============================================================
 * Local type definition
 *=============================================================*/

/*
 * CONFIG (CHIP related)
 * EEMCORESEL.APBSEL
 */

enum eem_phase {
	EEM_PHASE_INIT01,
	EEM_PHASE_INIT02,
	EEM_PHASE_MON,

	NR_EEM_PHASE,
};

enum {
	EEM_VOLT_NONE    = 0,
	EEM_VOLT_UPDATE  = BIT(0),
	EEM_VOLT_RESTORE = BIT(1),
};

struct eem_ctrl {
	const char *name;
	enum eem_det_id det_id;
	/* struct completion init_done; */
	/* atomic_t in_init; */
#ifdef __KERNEL__
	/* for voltage setting thread */
	wait_queue_head_t wq;
#endif
	int volt_update;
	struct task_struct *thread;
};

struct eem_det_ops {
	/* interface to EEM */
	void (*enable)(struct eem_det *det, int reason);
	void (*disable)(struct eem_det *det, int reason);
	void (*disable_locked)(struct eem_det *det, int reason);
	void (*switch_bank)(struct eem_det *det);

	int (*init01)(struct eem_det *det);
	int (*init02)(struct eem_det *det);
	int (*mon_mode)(struct eem_det *det);

	int (*get_status)(struct eem_det *det);
	void (*dump_status)(struct eem_det *det);

	void (*set_phase)(struct eem_det *det, enum eem_phase phase);

	/* interface to thermal */
	int (*get_temp)(struct eem_det *det);

	/* interface to DVFS */
	int (*get_volt)(struct eem_det *det);
	int (*set_volt)(struct eem_det *det);
	void (*restore_default_volt)(struct eem_det *det);
	void (*get_freq_table)(struct eem_det *det);
};

enum eem_features {
	FEA_INIT01	= BIT(EEM_PHASE_INIT01),
	FEA_INIT02	= BIT(EEM_PHASE_INIT02),
	FEA_MON		= BIT(EEM_PHASE_MON),
};

struct eem_det {
	const char *name;
	struct eem_det_ops *ops;
	int status;			/* TODO: enable/disable */
	int features;		/* enum eem_features */
	enum eem_ctrl_id ctrl_id;

	/* devinfo */
	unsigned int EEMINITEN;
	unsigned int EEMMONEN;
	unsigned int MDES;
	unsigned int BDES;
	unsigned int DCMDET;
	unsigned int DCBDET;
	unsigned int AGEDELTA;
	unsigned int MTDES;

	/* constant */
	unsigned int DETWINDOW;
	unsigned int VMAX;
	unsigned int VMIN;
	unsigned int DTHI;
	unsigned int DTLO;
	unsigned int VBOOT;
	unsigned int DETMAX;
	unsigned int AGECONFIG;
	unsigned int AGEM;
	unsigned int DVTFIXED;
	unsigned int VCO;
	unsigned int DCCONFIG;

	/* Generated by EEM init01. Used in EEM init02 */
	unsigned int DCVOFFSETIN;
	unsigned int AGEVOFFSETIN;

	/* for debug */
	unsigned int dcvalues[NR_EEM_PHASE];

	unsigned int eem_freqpct30[NR_EEM_PHASE];
	unsigned int eem_26c[NR_EEM_PHASE];
	unsigned int eem_vop30[NR_EEM_PHASE];
	unsigned int eem_eemen[NR_EEM_PHASE];
	u32 *recordRef;
#if DUMP_DATA_TO_DE
	unsigned int reg_dump_data[ARRAY_SIZE(reg_dump_addr_off)][NR_EEM_PHASE];
#endif
	/* slope */
	unsigned int MTS;
	unsigned int BTS;
	unsigned int t250;
	/* dvfs */
	unsigned int num_freq_tbl; /* could be got @ the same time
				      with freq_tbl[] */
	unsigned int max_freq_khz; /* maximum frequency used to
				      calculate percentage */
	unsigned char freq_tbl[NR_FREQ]; /* percentage to maximum freq */

	unsigned int volt_tbl[NR_FREQ]; /* pmic value */
	unsigned int volt_tbl_init2[NR_FREQ]; /* pmic value */
	unsigned int volt_tbl_pmic[NR_FREQ]; /* pmic value */
	unsigned int volt_tbl_bin[NR_FREQ]; /* pmic value */
	int volt_offset;
	int pi_offset;

	unsigned int disabled; /* Disabled by error or sysfs */
};


struct eem_devinfo {
	/* EEM0 0x10206660 */
	unsigned int CPU0_BDES:8;
	unsigned int CPU0_MDES:8;
	unsigned int CPU0_DCBDET:8;
	unsigned int CPU0_DCMDET:8;

	/* EEM1 0x10206664 */
	unsigned int CPU1_MTDES:8;
	unsigned int CPU1_AGEDELTA:8;
	unsigned int CPU0_MTDES:8;
	unsigned int CPU0_AGEDELTA:8;

	/* EEM2 0x10206668 */
	unsigned int CPU1_BDES:8;
	unsigned int CPU1_MDES:8;
	unsigned int CPU1_DCBDET:8;
	unsigned int CPU1_DCMDET:8;

	/* EEM3 0x1020666C */
	unsigned int GPU_BDES:8;
	unsigned int GPU_MDES:8;
	unsigned int GPU_DCBDET:8;
	unsigned int GPU_DCMDET:8;

	/* EEM4 0x10206670 */
	unsigned int OD4_RESERVED:16;
	unsigned int GPU_MTDES:8;
	unsigned int GPU_AGEDELTA:8;

	/* M_SRM_RP5 0x10206274 */
	unsigned int RP5_RESERVED:8;
	unsigned int GPU_LEAKAGE:8;
	unsigned int CPU1_LEAKAGE:8;
	unsigned int CPU0_LEAKAGE:8;

	/* M_SRM_RP6 0x10206278 */
	unsigned int FT_PGM:4;
	unsigned int CPU0_SPEC:3;
	unsigned int CPU0_TURBO:1;
	unsigned int CPU0_DVFS:2;
	unsigned int CPU1_SPEC:3;
	unsigned int CPU1_DVFS:2;
	unsigned int CPU1_TURBO:1;
	unsigned int PR6_RESERVED:16;

	unsigned int EEMINITEN:8;
	unsigned int EEMMONEN:8;
	unsigned int RESERVED:16;
#if 0
	/* M_SRM_RP5 */
	unsigned int SOC_MTDES:8;
	unsigned int SOC_AGEDELTA:8;

	/* M_SRM_RP6 */
	unsigned int SOC_BDES:8;
	unsigned int SOC_MDES:8;
	unsigned int SOC_DCBDET:8;
	unsigned int SOC_DCMDET:8;
#endif
};

/*=============================================================
 *Local variable definition
 *=============================================================*/

#ifdef __KERNEL__
/*
 * lock
 */
static DEFINE_SPINLOCK(eem_spinlock);
static DEFINE_MUTEX(record_mutex);
	#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
		/* CPU callback */
		static int __cpuinit _mt_eem_cpu_CB(struct notifier_block *nfb,
			unsigned long action, void *hcpu);
		static struct notifier_block __refdata _mt_eem_cpu_notifier = {
			.notifier_call = _mt_eem_cpu_CB,
		};
	#endif
#endif
/**
 * EEM controllers
 */
struct eem_ctrl eem_ctrls[NR_EEM_CTRL] = {
	[EEM_CTRL_LITTLE] = {
		.name = __stringify(EEM_CTRL_LITTLE),
		.det_id = EEM_DET_LITTLE,
	},

	[EEM_CTRL_GPU] = {
		.name = __stringify(EEM_CTRL_GPU),
		.det_id = EEM_DET_GPU,
	},

	[EEM_CTRL_BIG] = {
		.name = __stringify(EEM_CTRL_BIG),
		.det_id = EEM_DET_BIG,
	},
	#if 0
	[EEM_CTRL_SOC] = {
		.name = __stringify(EEM_CTRL_SOC),
		.det_id = EEM_DET_SOC,
	},
	#endif
};

/*
 * EEM detectors
 */
static void base_ops_enable(struct eem_det *det, int reason);
static void base_ops_disable(struct eem_det *det, int reason);
static void base_ops_disable_locked(struct eem_det *det, int reason);
static void base_ops_switch_bank(struct eem_det *det);

static int base_ops_init01(struct eem_det *det);
static int base_ops_init02(struct eem_det *det);
static int base_ops_mon_mode(struct eem_det *det);

static int base_ops_get_status(struct eem_det *det);
static void base_ops_dump_status(struct eem_det *det);

static void base_ops_set_phase(struct eem_det *det, enum eem_phase phase);
static int base_ops_get_temp(struct eem_det *det);
static int base_ops_get_volt(struct eem_det *det);
static int base_ops_set_volt(struct eem_det *det);
static void base_ops_restore_default_volt(struct eem_det *det);
static void base_ops_get_freq_table(struct eem_det *det);

static int get_volt_cpu(struct eem_det *det);
static int set_volt_cpu(struct eem_det *det);
static void restore_default_volt_cpu(struct eem_det *det);
static void get_freq_table_cpu(struct eem_det *det);

static int get_volt_gpu(struct eem_det *det);
static int set_volt_gpu(struct eem_det *det);
static void restore_default_volt_gpu(struct eem_det *det);
static void get_freq_table_gpu(struct eem_det *det);

#if 0
static int get_volt_lte(struct eem_det *det);
static int set_volt_lte(struct eem_det *det);
static void restore_default_volt_lte(struct eem_det *det);

static int get_volt_vcore(struct eem_det *det);
#ifndef __KERNEL__
static int set_volt_vcore(struct eem_det *det);
#endif

static void switch_to_vcore_ao(struct eem_det *det);
static void switch_to_vcore_pdn(struct eem_det *det);
#endif

#define BASE_OP(fn)	.fn = base_ops_ ## fn
static struct eem_det_ops eem_det_base_ops = {
	BASE_OP(enable),
	BASE_OP(disable),
	BASE_OP(disable_locked),
	BASE_OP(switch_bank),

	BASE_OP(init01),
	BASE_OP(init02),
	BASE_OP(mon_mode),

	BASE_OP(get_status),
	BASE_OP(dump_status),

	BASE_OP(set_phase),

	BASE_OP(get_temp),

	BASE_OP(get_volt),
	BASE_OP(set_volt),
	BASE_OP(restore_default_volt),
	BASE_OP(get_freq_table),
};

static struct eem_det_ops little_det_ops = { /* I-Chang */
	.get_volt		= get_volt_cpu,
	.set_volt		= set_volt_cpu,
	.restore_default_volt	= restore_default_volt_cpu,
	.get_freq_table		= get_freq_table_cpu,
};

static struct eem_det_ops gpu_det_ops = {
	.get_volt		= get_volt_gpu,
	.set_volt		= set_volt_gpu, /* <-@@@ */
	.restore_default_volt	= restore_default_volt_gpu,
	.get_freq_table		= get_freq_table_gpu,
};

static struct eem_det_ops big_det_ops = { /* I-Chang */
	.get_volt		= get_volt_cpu,
	.set_volt		= set_volt_cpu, /* <-@@@ */
	.restore_default_volt	= restore_default_volt_cpu,
	.get_freq_table		= get_freq_table_cpu,
};
#if 0
static struct eem_det_ops lte_det_ops = {
	.get_volt		= get_volt_lte,
	.set_volt		= set_volt_lte,
	.restore_default_volt	= restore_default_volt_lte,
	.get_freq_table		= NULL,
};

static struct eem_det_ops soc_det_ops = {
	.get_volt		= get_volt_vcore, /* <-@@@@@@@@@@@@ */
#ifndef __KERNEL__
	.set_volt		= set_volt_vcore, /* <-@@@@@@@@@@@@ */
#else
	.set_volt		= NULL,
#endif
	.restore_default_volt	= NULL, /* <-@@@@@@@@@@@@ */
	.get_freq_table		= NULL,
};
#endif

static struct eem_det eem_detectors[NR_EEM_DET] = {
	[EEM_DET_LITTLE] = {
		.name		= __stringify(EEM_DET_LITTLE),
		.ops		= &little_det_ops,
		.ctrl_id	= EEM_CTRL_LITTLE,
		.features	= FEA_INIT01 | FEA_INIT02 | FEA_MON,
		.max_freq_khz	= 1144000,/* 1144 MHz */
		.VBOOT		= EEM_VOLT_TO_PMIC_VAL(100000), /* 1.0v: 0x30 */
		.volt_offset	= 0,
	},

	[EEM_DET_GPU] = {
		.name		= __stringify(EEM_DET_GPU),
		.ops		= &gpu_det_ops,
		.ctrl_id	= EEM_CTRL_GPU,
		#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
		.features	= 0,
		#else
		.features	= FEA_INIT01 | FEA_INIT02 | FEA_MON,
		#endif
		.max_freq_khz	= 728000,/* 728 MHz */
		.VBOOT		= EEM_VOLT_TO_PMIC_VAL(100000), /* 1.0v: 0x30 */
		.volt_offset	= 0,
	},

	[EEM_DET_BIG] = {
		.name		= __stringify(EEM_DET_BIG),
		.ops		= &big_det_ops,
		.ctrl_id	= EEM_CTRL_BIG,
		.features	= FEA_INIT01 | FEA_INIT02 | FEA_MON,
		.max_freq_khz	= 1950000,/* 1950Mhz */
		.VBOOT		= EEM_VOLT_TO_PMIC_VAL(100000), /* 1.0v: 0x30 */
		.volt_offset	= 0,
	},
	#if 0
	[EEM_DET_SOC] = {
		.name		= __stringify(EEM_DET_SOC),
		.ops		= &soc_det_ops,
		.ctrl_id	= EEM_CTRL_SOC,
		#if !defined(__MTK_SLT_)
			.features	= FEA_INIT01 | FEA_INIT02 | FEA_MON,
		#else
			.features	= FEA_INIT01 | FEA_INIT02,
		#endif
		.max_freq_khz	= 1866000,
		.VBOOT		= EEM_VOLT_TO_PMIC_VAL(100000), /* 1.0v: 0x30 */
		.volt_offset	= 0,
	},

	[EEM_DET_LTE] = {
		.name		= __stringify(EEM_DET_LTE),
		.ops		= &lte_det_ops,
		.ctrl_id	= EEM_CTRL_LTE,
		#if !defined(__MTK_SLT_)
			.features	= FEA_INIT01 | FEA_INIT02 | FEA_MON,
		#else
			.features	= FEA_INIT01 | FEA_INIT02,
		#endif
		.max_freq_khz	= 300000,
		.VBOOT		= EEM_VOLT_TO_PMIC_VAL(105000), /* 1.0v: 0x30 */
		.volt_offset	= 0,
	},
	#endif
};

static struct eem_devinfo eem_devinfo;
/* static unsigned int eem_level;  debug info */

#ifdef __KERNEL__
/**
 * timer for log
 */
static struct hrtimer eem_log_timer;
#endif

/*=============================================================
 * Local function definition
 *=============================================================*/

static struct eem_det *id_to_eem_det(enum eem_det_id id)
{
	if (likely(id < NR_EEM_DET))
		return &eem_detectors[id];
	else
		return NULL;
}

static struct eem_ctrl *id_to_eem_ctrl(enum eem_ctrl_id id)
{
	if (likely(id < NR_EEM_CTRL))
		return &eem_ctrls[id];
	else
		return NULL;
}

static void base_ops_enable(struct eem_det *det, int reason)
{
	/* FIXME: UNDER CONSTRUCTION */
	FUNC_ENTER(FUNC_LV_HELP);
	det->disabled &= ~reason;
	FUNC_EXIT(FUNC_LV_HELP);
}

static void base_ops_switch_bank(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
	eem_write_field(EEMCORESEL, 2:0, det->ctrl_id);
	FUNC_EXIT(FUNC_LV_HELP);
}

static void base_ops_disable_locked(struct eem_det *det, int reason)
{
	FUNC_ENTER(FUNC_LV_HELP);

	switch (reason) {
	case BY_MON_ERROR:
		/* disable EEM */
		eem_write(EEMEN, 0x0);

		/* Clear EEM interrupt EEMINTSTS */
		eem_write(EEMINTSTS, 0x00ffffff);
		/* fall through */

	case BY_PROCFS_INIT2:
		/* set init2 value to DVFS table (PMIC) */
		memcpy(det->volt_tbl, det->volt_tbl_init2, sizeof(det->volt_tbl_init2));
		eem_set_eem_volt(det);
		det->disabled |= reason;
		break;

	case BY_INIT_ERROR:
		/* disable EEM */
		eem_write(EEMEN, 0x0);

		/* Clear EEM interrupt EEMINTSTS */
		eem_write(EEMINTSTS, 0x00ffffff);
		/* fall through */

	case BY_PROCFS:
		det->disabled |= reason;
		/* restore default DVFS table (PMIC) */
		eem_restore_eem_volt(det);
		break;

	default:
		det->disabled &= ~BY_PROCFS;
		det->disabled &= ~BY_PROCFS_INIT2;
		eem_set_eem_volt(det);
		break;
	}

	eem_debug("Disable EEM[%s] done. reason=[%d]\n", det->name, det->disabled);

	FUNC_EXIT(FUNC_LV_HELP);
}

static void base_ops_disable(struct eem_det *det, int reason)
{
	unsigned long flags;

	FUNC_ENTER(FUNC_LV_HELP);

	mt_ptp_lock(&flags);
	det->ops->switch_bank(det);
	det->ops->disable_locked(det, reason);
	mt_ptp_unlock(&flags);

	FUNC_EXIT(FUNC_LV_HELP);
}

static int base_ops_init01(struct eem_det *det)
{
	/* struct eem_ctrl *ctrl = id_to_eem_ctrl(det->ctrl_id); */

	FUNC_ENTER(FUNC_LV_HELP);

	if (unlikely(!HAS_FEATURE(det, FEA_INIT01))) {
		eem_debug("det %s has no INIT01\n", det->name);
		FUNC_EXIT(FUNC_LV_HELP);
		return -1;
	}

	/*
	if (det->disabled & BY_PROCFS) {
		eem_debug("[%s] Disabled by PROCFS\n", __func__);
		FUNC_EXIT(FUNC_LV_HELP);
		return -2;
	}
	*/

	/* eem_debug("%s(%s) start (eem_level = 0x%08X).\n", __func__, det->name, eem_level); */
	/* atomic_inc(&ctrl->in_init); */
	/* eem_init01_prepare(det); */
	/* det->ops->dump_status(det); */
	det->ops->set_phase(det, EEM_PHASE_INIT01);

	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

static int base_ops_init02(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);

	if (unlikely(!HAS_FEATURE(det, FEA_INIT02))) {
		eem_debug("det %s has no INIT02\n", det->name);
		FUNC_EXIT(FUNC_LV_HELP);
		return -1;
	}

	/*
	if (det->disabled & BY_PROCFS) {
		eem_debug("[%s] Disabled by PROCFS\n", __func__);
		FUNC_EXIT(FUNC_LV_HELP);
		return -2;
	}
	eem_debug("%s(%s) start (eem_level = 0x%08X).\n", __func__, det->name, eem_level);
	eem_debug("DCV = 0x%08X, AGEV = 0x%08X\n", det->DCVOFFSETIN, det->AGEVOFFSETIN);
	*/

	/* det->ops->dump_status(det); */
	det->ops->set_phase(det, EEM_PHASE_INIT02);

	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

static int base_ops_mon_mode(struct eem_det *det)
{
	#ifndef EARLY_PORTING
	struct TS_PTPOD ts_info;
	thermal_bank_name ts_bank;
	#endif

	FUNC_ENTER(FUNC_LV_HELP);

	if (!HAS_FEATURE(det, FEA_MON)) {
		eem_debug("det %s has no MON mode\n", det->name);
		FUNC_EXIT(FUNC_LV_HELP);
		return -1;
	}

	/*
	if (det->disabled & BY_PROCFS) {
		eem_debug("[%s] Disabled by PROCFS\n", __func__);
		FUNC_EXIT(FUNC_LV_HELP);
		return -2;
	}
	eem_debug("%s(%s) start (eem_level = 0x%08X).\n", __func__, det->name, eem_level);
	*/

#if !defined(EARLY_PORTING)
	ts_bank = det->ctrl_id;
	get_thermal_slope_intercept(&ts_info, ts_bank);/* I-Chang */
	det->MTS = ts_info.ts_MTS;/* I-Chang */
	det->BTS = ts_info.ts_BTS;/* I-Chang */
#else
	det->MTS =  0x2cf; /* (2048 * TS_SLOPE) + 2048; */
	det->BTS =  0x80E; /* 4 * TS_INTERCEPT; */
#endif
	eem_debug("MTS = %d, BTS = %d\n", det->MTS, det->BTS);
	if ((det->EEMINITEN == 0x0) || (det->EEMMONEN == 0x0)) {
		eem_debug("EEMINITEN = 0x%08X, EEMMONEN = 0x%08X\n", det->EEMINITEN, det->EEMMONEN);
		FUNC_EXIT(FUNC_LV_HELP);
		return 1;
	}

	/* det->ops->dump_status(det); */
	det->ops->set_phase(det, EEM_PHASE_MON);

	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

static int base_ops_get_status(struct eem_det *det)
{
	int status;
	unsigned long flags;

	FUNC_ENTER(FUNC_LV_HELP);

	mt_ptp_lock(&flags);
	det->ops->switch_bank(det);
	status = (eem_read(EEMEN) != 0) ? 1 : 0;
	mt_ptp_unlock(&flags);

	FUNC_EXIT(FUNC_LV_HELP);

	return status;
}

static void base_ops_dump_status(struct eem_det *det)
{
	unsigned int i;

	FUNC_ENTER(FUNC_LV_HELP);

	eem_isr_info("[%s]\n",			det->name);

	eem_isr_info("EEMINITEN = 0x%08X\n",	det->EEMINITEN);
	eem_isr_info("EEMMONEN = 0x%08X\n",	det->EEMMONEN);
	eem_isr_info("MDES = 0x%08X\n",		det->MDES);
	eem_isr_info("BDES = 0x%08X\n",		det->BDES);
	eem_isr_info("DCMDET = 0x%08X\n",	det->DCMDET);

	eem_isr_info("DCCONFIG = 0x%08X\n",	det->DCCONFIG);
	eem_isr_info("DCBDET = 0x%08X\n",	det->DCBDET);

	eem_isr_info("AGECONFIG = 0x%08X\n",	det->AGECONFIG);
	eem_isr_info("AGEM = 0x%08X\n",		det->AGEM);

	eem_isr_info("AGEDELTA = 0x%08X\n",	det->AGEDELTA);
	eem_isr_info("DVTFIXED = 0x%08X\n",	det->DVTFIXED);
	eem_isr_info("MTDES = 0x%08X\n",	det->MTDES);
	eem_isr_info("VCO = 0x%08X\n",		det->VCO);

	eem_isr_info("DETWINDOW = 0x%08X\n",	det->DETWINDOW);
	eem_isr_info("VMAX = 0x%08X\n",		det->VMAX);
	eem_isr_info("VMIN = 0x%08X\n",		det->VMIN);
	eem_isr_info("DTHI = 0x%08X\n",		det->DTHI);
	eem_isr_info("DTLO = 0x%08X\n",		det->DTLO);
	eem_isr_info("VBOOT = 0x%08X\n",	det->VBOOT);
	eem_isr_info("DETMAX = 0x%08X\n",	det->DETMAX);

	eem_isr_info("DCVOFFSETIN = 0x%08X\n",	det->DCVOFFSETIN);
	eem_isr_info("AGEVOFFSETIN = 0x%08X\n",	det->AGEVOFFSETIN);

	eem_isr_info("MTS = 0x%08X\n",		det->MTS);
	eem_isr_info("BTS = 0x%08X\n",		det->BTS);

	eem_isr_info("num_freq_tbl = %d\n", det->num_freq_tbl);

	for (i = 0; i < det->num_freq_tbl; i++)
		eem_isr_info("freq_tbl[%d] = %d\n", i, det->freq_tbl[i]);

	for (i = 0; i < det->num_freq_tbl; i++)
		eem_isr_info("volt_tbl[%d] = %d\n", i, det->volt_tbl[i]);

	for (i = 0; i < det->num_freq_tbl; i++)
		eem_isr_info("volt_tbl_init2[%d] = %d\n", i, det->volt_tbl_init2[i]);

	for (i = 0; i < det->num_freq_tbl; i++)
		eem_isr_info("volt_tbl_pmic[%d] = %d\n", i, det->volt_tbl_pmic[i]);

	FUNC_EXIT(FUNC_LV_HELP);
}

static void base_ops_set_phase(struct eem_det *det, enum eem_phase phase)
{
	unsigned int i, filter, val;
	/* unsigned long flags; */

	FUNC_ENTER(FUNC_LV_HELP);

	/* mt_ptp_lock(&flags); */

	det->ops->switch_bank(det);
	/* config EEM register */
	eem_write(DESCHAR,
		  ((det->BDES << 8) & 0xff00) | (det->MDES & 0xff));
	eem_write(TEMPCHAR,
		  (((det->VCO << 16) & 0xff0000) |
		   ((det->MTDES << 8) & 0xff00) | (det->DVTFIXED & 0xff)));
	eem_write(DETCHAR,
		  ((det->DCBDET << 8) & 0xff00) | (det->DCMDET & 0xff));
	eem_write(AGECHAR,
		  ((det->AGEDELTA << 8) & 0xff00) | (det->AGEM & 0xff));
	eem_write(EEM_DCCONFIG, det->DCCONFIG);
	eem_write(EEM_AGECONFIG, det->AGECONFIG);

	if (EEM_PHASE_MON == phase)
		eem_write(TSCALCS,
			  ((det->BTS << 12) & 0xfff000) | (det->MTS & 0xfff));

	if (det->AGEM == 0x0)
		eem_write(RUNCONFIG, 0x80000000);
	else {
		val = 0x0;

		for (i = 0; i < 24; i += 2) {
			filter = 0x3 << i;

			if (((det->AGECONFIG) & filter) == 0x0)
				val |= (0x1 << i);
			else
				val |= ((det->AGECONFIG) & filter);
		}

		eem_write(RUNCONFIG, val);
	}

	eem_write(FREQPCT30,
		  ((det->freq_tbl[3] << 24) & 0xff000000)	|
		  ((det->freq_tbl[2] << 16) & 0xff0000)	|
		  ((det->freq_tbl[1] << 8) & 0xff00)	|
		  (det->freq_tbl[0] & 0xff));
	eem_write(FREQPCT74,
		  ((det->freq_tbl[7] << 24) & 0xff000000)	|
		  ((det->freq_tbl[6] << 16) & 0xff0000)	|
		  ((det->freq_tbl[5] << 8) & 0xff00)	|
		  ((det->freq_tbl[4]) & 0xff));
	eem_write(LIMITVALS,
		  ((det->VMAX << 24) & 0xff000000)	|
		  ((det->VMIN << 16) & 0xff0000)	|
		  ((det->DTHI << 8) & 0xff00)		|
		  (det->DTLO & 0xff));
	/* eem_write(LIMITVALS, 0xFF0001FE); */
	eem_write(EEM_VBOOT, (((det->VBOOT) & 0xff)));
	eem_write(EEM_DETWINDOW, (((det->DETWINDOW) & 0xffff)));
	eem_write(EEMCONFIG, (((det->DETMAX) & 0xffff)));

	/* clear all pending EEM interrupt & config EEMINTEN */
	eem_write(EEMINTSTS, 0xffffffff);

	eem_debug("%s phase = %d\n", ((char *)(det->name) + 8), phase);
	switch (phase) {
	case EEM_PHASE_INIT01:
		eem_write(EEMINTEN, 0x00005f01);
		/* enable EEM INIT measurement */
		eem_write(EEMEN, 0x00000001);
		break;

	case EEM_PHASE_INIT02:
		eem_write(EEMINTEN, 0x00005f01);
		eem_write(INIT2VALS,
			  ((det->AGEVOFFSETIN << 16) & 0xffff0000) |
			  (det->DCVOFFSETIN & 0xffff));
		/* enable EEM INIT measurement */
		eem_write(EEMEN, 0x00000005);
		break;

	case EEM_PHASE_MON:
		eem_write(EEMINTEN, 0x00FF0000);
		/* enable EEM monitor mode */
		eem_write(EEMEN, 0x00000002);
		break;

	default:
		BUG();
		break;
	}

	/* mt_ptp_unlock(&flags); */

	FUNC_EXIT(FUNC_LV_HELP);
}

static int base_ops_get_temp(struct eem_det *det)
{
	thermal_bank_name ts_bank;
#if 1 /* TODO: FIXME */
	FUNC_ENTER(FUNC_LV_HELP);
	/* THERMAL_BANK0 = 0, */ /* CPU0  */
	/* THERMAL_BANK2 = 1, */ /* GPU/SOC */
	/* THERMAL_BANK1 = 2, */ /* CPU1 */

	ts_bank = (det_to_id(det) == EEM_DET_LITTLE) ? THERMAL_BANK0  :
		      (det_to_id(det) == EEM_DET_BIG) ? THERMAL_BANK2  :
		       THERMAL_BANK1;

	FUNC_EXIT(FUNC_LV_HELP);
#endif

#if !defined(EARLY_PORTING)
	#ifdef __KERNEL__
		return tscpu_get_temp_by_bank(ts_bank);
	#else
		thermal_init();
		udelay(1000);
		return mtktscpu_get_hw_temp();
	#endif
#else
	return 0;
#endif
}

static int base_ops_get_volt(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
	eem_debug("[%s] default func\n", __func__);
	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

static int base_ops_set_volt(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
	eem_debug("[%s] default func\n", __func__);
	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

static void base_ops_restore_default_volt(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
	eem_debug("[%s] default func\n", __func__);
	FUNC_EXIT(FUNC_LV_HELP);
}

static void base_ops_get_freq_table(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);

	det->freq_tbl[0] = 100;
	det->num_freq_tbl = 1;

	FUNC_EXIT(FUNC_LV_HELP);
}

static void record(struct eem_det *det)
{
	int i;
	unsigned int vSram;

	det->recordRef[NR_FREQ * 2] = 0x00000000;
	mb(); /* SRAM writing */
	for (i = 0; i < NR_FREQ; i++) {
		vSram = clamp((unsigned int)(record_tbl_locked[i] + 0x10),
				(unsigned int)(VMIN_SRAM + EEM_PMIC_OFFSET),
				(unsigned int)(det->VMAX + EEM_PMIC_OFFSET));

		det->recordRef[i*2] = (det->recordRef[i*2] & (~0x3FFF)) |
			((((vSram & 0x7F) << 7) | (record_tbl_locked[i] & 0x7F)) & 0x3fff);
	}
	det->recordRef[NR_FREQ * 2] = 0xFFFFFFFF;
	mb(); /* SRAM writing */
}

static void restore_record(struct eem_det *det)
{
	int i;

	det->recordRef[NR_FREQ * 2] = 0x00000000;
	mb(); /* SRAM writing */
	for (i = 0; i < NR_FREQ; i++) {
		if (det_to_id(det) == EEM_DET_LITTLE)
			det->recordRef[i*2] = (det->recordRef[i*2] & (~0x3FFF)) |
				(((*(recordTbl + (i * 9) + 7) & 0x7F) << 7) |
				(*(recordTbl + (i * 9) + 8) & 0x7F));
		else if (det_to_id(det) == EEM_DET_BIG)
			det->recordRef[i*2] = (det->recordRef[i*2] & (~0x3FFF)) |
				(((*(recordTbl + (i + 8) * 9 + 7) & 0x7F) << 7) |
				(*(recordTbl + (i + 8) * 9 + 8) & 0x7F));
	}
	det->recordRef[NR_FREQ * 2] = 0xFFFFFFFF;
	mb(); /* SRAM writing */
}

static void mt_cpufreq_set_ptbl_funcEEM(enum mt_cpu_dvfs_id id, int restore)
{
	struct eem_det *det = id_to_eem_det((id == MT_CPU_DVFS_LITTLE) ? EEM_CTRL_LITTLE : EEM_CTRL_BIG);

	if (restore)
		restore_record(det);
	else
		record(det);
}

/* Will return 10uV */
static int get_volt_cpu(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
	FUNC_EXIT(FUNC_LV_HELP);
	#ifdef EARLY_PORTING
		return 0;
	#else
		/* unit mv * 100 = 10uv */  /* I-Chang */
		return mt_cpufreq_get_cur_volt(
			(det_to_id(det) == EEM_DET_LITTLE) ?
			MT_CPU_DVFS_LITTLE : MT_CPU_DVFS_BIG);
	#endif
}

#if defined(__MTK_SLT_) && (defined(SLT_VMAX) || defined(SLT_VMIN))
	#define EEM_PMIC_LV_HV_OFFSET (0x3)
	#define EEM_PMIC_NV_OFFSET (0xB)
#endif
/* volt_tbl_pmic is convert from 10uV */
static int set_volt_cpu(struct eem_det *det)
{
	int value = 0;

	FUNC_ENTER(FUNC_LV_HELP);
	FUNC_EXIT(FUNC_LV_HELP);
#if defined(__MTK_SLT_)
	#if defined(SLT_VMAX)
		det->volt_tbl_pmic[0] = det->volt_tbl_pmic[0] - EEM_PMIC_NV_OFFSET + EEM_PMIC_LV_HV_OFFSET;
		eem_debug("VPROC voltage EEM to 0x%X\n", det->volt_tbl_pmic[0]);
	#elif defined(SLT_VMIN)
		det->volt_tbl_pmic[0] = det->volt_tbl_pmic[0] - EEM_PMIC_NV_OFFSET - EEM_PMIC_LV_HV_OFFSET;
		eem_debug("VPROC voltage EEM to 0x%X\n", det->volt_tbl_pmic[0]);
	#else
		/* det->volt_tbl_pmic[0] = det->volt_tbl_pmic[0] - EEM_PMIC_NV_OFFSET; */
		eem_debug("NV VPROC voltage EEM to 0x%X\n", det->volt_tbl_pmic[0]);
	#endif
#endif
	eem_debug("init02_vop_30 = 0x%x\n", det->eem_vop30[EEM_PHASE_INIT02]);

	#ifdef EARLY_PORTING
		return value;
	#else
		#ifdef __KERNEL__
		/* record(det); */
		mutex_lock(&record_mutex);
		#endif

		for (value = 0; value < NR_FREQ; value++)
			record_tbl_locked[value] = det->volt_tbl_pmic[value];

		value = mt_cpufreq_update_volt(
			(det_to_id(det) == EEM_DET_LITTLE) ?
			MT_CPU_DVFS_LITTLE : MT_CPU_DVFS_BIG,
			record_tbl_locked, det->num_freq_tbl);

		#ifdef __KERNEL__
		mutex_unlock(&record_mutex);
		#endif

		return value;
	#endif
}

static void restore_default_volt_cpu(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);

	#ifndef EARLY_PORTING
	/* I-Chang */
	#ifdef __KERNEL__
	/* restore_record(det); */
	#endif
	mt_cpufreq_restore_default_volt(
		(det_to_id(det) == EEM_DET_LITTLE) ?
		MT_CPU_DVFS_LITTLE : MT_CPU_DVFS_BIG);
	#endif

	FUNC_EXIT(FUNC_LV_HELP);
}


static void get_freq_table_cpu(struct eem_det *det)
{
	int i;
	unsigned int binLevel;
	#if !defined(CONFIG_MTK_PMIC_CHIP_MT6353)
	unsigned int freq_bound, binLevel_eng;
	#endif
	#ifndef EARLY_PORTING
	enum mt_cpu_dvfs_id cpu;

	FUNC_ENTER(FUNC_LV_HELP);
	cpu = (det_to_id(det) == EEM_DET_LITTLE) ?
		MT_CPU_DVFS_LITTLE : MT_CPU_DVFS_BIG;
	#endif

	/* det->max_freq_khz = mt_cpufreq_get_freq_by_idx(cpu, 0); */
	#ifdef __KERNEL__
		binLevel = GET_BITS_VAL(7:0, get_devinfo_with_index(21));
		#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
		if (eem_is_extbuck_valid) {
			if ((binLevel == 0x82) || (binLevel == 0x86))
				binLevel = 2;
			else
				binLevel = 1;
		} else
			binLevel = 1;
		cpuBinLevel = binLevel;
		#else
		binLevel_eng = GET_BITS_VAL(15:0, get_devinfo_with_index(19));
		freq_bound = GET_BITS_VAL(25:23, get_devinfo_with_index(4));
		#endif
	#else
		binLevel_eng = GET_BITS_VAL(15:0, eem_read(0x10206278));
		#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
		/* binLevel = (eem_is_extbuck_valid) ? 2 : 1; */
		binLevel = 1;
		#else
		binLevel = GET_BITS_VAL(7:0, eem_read(0x1020627C));
		freq_bound = GET_BITS_VAL(25:23, eem_read(0x10206044));
		#endif
	#endif

	for (i = 0; i < NR_FREQ; i++) {
		#ifdef EARLY_PORTING
			det->freq_tbl[i] =
				PERCENT((det_to_id(det) == EEM_DET_LITTLE) ? littleFreq_SB[i] : bigFreq_SB[i],
					det->max_freq_khz);
		#else
			/* I-Chang */
			/* det->freq_tbl[i] = PERCENT(mt_cpufreq_get_freq_by_idx(cpu, i), det->max_freq_khz); */
			#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
			if (0) {
			#else
			if (1001 == cpu_speed) {
			#endif
				det->freq_tbl[i] =
				PERCENT((det_to_id(det) == EEM_DET_LITTLE) ? littleFreq_FY[i] :
					#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
					bigFreq_FY[i],
					#else
					((freq_bound == 5) ? bigFreq_FY_M[i] : bigFreq_FY[i]),
					#endif
				det->max_freq_khz);
				/* eem_error("2--->Get cpu speed from Device tree = %d\n", cpu_speed); */
			} else {
				#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
				if (1 == binLevel) {
				#else
				if ((1 == binLevel) || (3 == binLevel)) {
				#endif
					det->freq_tbl[i] =
					PERCENT((det_to_id(det) == EEM_DET_LITTLE) ? littleFreq_FY[i] :
					#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
						bigFreq_FY[i],
					#else
						((freq_bound == 5) ? bigFreq_FY_M[i] : bigFreq_FY[i]),
					#endif
					det->max_freq_khz);
				#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
				} else if (2 == binLevel) {
				#else
				} else if ((2 == binLevel) || (4 == binLevel)) {
				#endif
					det->freq_tbl[i] =
					PERCENT((det_to_id(det) == EEM_DET_LITTLE) ? littleFreq_SB[i] : bigFreq_SB[i],
					det->max_freq_khz);
				#ifdef CONFIG_ARCH_MT6755_TURBO
				} else if (0x20 == (binLevel & 0xF0)) {
					det->freq_tbl[i] =
					PERCENT((det_to_id(det) == EEM_DET_LITTLE) ? littleFreq_P15[i] : bigFreq_P15[i],
					det->max_freq_khz);
				#endif
				#if !defined(CONFIG_MTK_PMIC_CHIP_MT6353)
				} else {
					if ((2 == ((binLevel_eng >> 4) & 0x07)) ||
					    (2 == ((binLevel_eng >> 10) & 0x07))) {
						det->freq_tbl[i] =
							PERCENT(
							(det_to_id(det) == EEM_DET_LITTLE) ?
								littleFreq_FY[i] :
								((freq_bound == 5) ? bigFreq_FY_M[i] : bigFreq_FY[i]),
							det->max_freq_khz);
					} else {
						det->freq_tbl[i] =
							PERCENT(
							(det_to_id(det) == EEM_DET_LITTLE) ?
								littleFreq_SB[i] :
								bigFreq_SB[i],
							det->max_freq_khz);
					}
				}
				#else
				}
				#endif
			}
		#endif
		if (0 == det->freq_tbl[i])
			break;
	}

	det->num_freq_tbl = i;

	FUNC_EXIT(FUNC_LV_HELP);
}

static int get_volt_gpu(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
	FUNC_EXIT(FUNC_LV_HELP);

	/* eem_debug("get_volt_gpu=%d\n",mt_gpufreq_get_cur_volt()); */
	#ifdef EARLY_PORTING
	return 0; /* gpu_dvfs_get_cur_volt(); Ask gpu owner */
	#else
		#if defined(__MTK_SLT_)
			return gpu_dvfs_get_cur_volt();
		#else
			return mt_gpufreq_get_cur_volt(); /* unit  mv * 100 = 10uv */
		#endif
	#endif
}

static int set_volt_gpu(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
	FUNC_EXIT(FUNC_LV_HELP);
	/*
	eem_debug("set_volt_gpu=%x %x %x %x\n",
		det->volt_tbl_pmic[0],
		det->volt_tbl_pmic[1],
		det->volt_tbl_pmic[2],
		det->volt_tbl_pmic[3]);
	*/
	#ifdef EARLY_PORTING
		return 0;
	#else
		return mt_gpufreq_update_volt(det->volt_tbl_pmic, det->num_freq_tbl);
	#endif
}

static void restore_default_volt_gpu(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
	#if !defined(EARLY_PORTING)
	mt_gpufreq_restore_default_volt();
	#endif
	FUNC_EXIT(FUNC_LV_HELP);
}

static void get_freq_table_gpu(struct eem_det *det)
{
	int i;

	FUNC_ENTER(FUNC_LV_HELP);

	for (i = 0; i < NR_FREQ; i++) {
		#ifdef EARLY_PORTING
		det->freq_tbl[i] = PERCENT(gpuFreq[i], det->max_freq_khz);
		#else
		det->freq_tbl[i] = PERCENT(mt_gpufreq_get_freq_by_idx(i), det->max_freq_khz);
		#endif
		if (0 == det->freq_tbl[i])
			break;
	}

	FUNC_EXIT(FUNC_LV_HELP);

	det->num_freq_tbl = i;
}

#if 0
static int set_volt_lte(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
#ifdef __KERNEL__
	return mt_cpufreq_set_lte_volt(det->volt_tbl_init2[0]);
#else
	return 0;
	/* return dvfs_set_vlte(det->volt_tbl_bin[0]); */
	/* return dvfs_set_vlte(det->volt_tbl_init2[0]); */
#endif
	FUNC_EXIT(FUNC_LV_HELP);
}

static int get_volt_lte(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
	return mt_get_cur_volt_lte(); /* unit mv * 100 = 10uv */
	FUNC_EXIT(FUNC_LV_HELP);
}

static void restore_default_volt_lte(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
#ifdef __KERNEL__
	if (0x337 == mt_get_chip_hw_code())
		mt_cpufreq_set_lte_volt(EEM_VOLT_TO_PMIC_VAL(105000) +
			EEM_PMIC_OFFSET); /*-700+100 */
#else
	/* dvfs_set_vlte(EEM_VOLT_TO_PMIC_VAL(105000)+EEM_PMIC_OFFSET); */
#endif
	FUNC_EXIT(FUNC_LV_HELP);
}


static void switch_to_vcore_ao(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);

	eem_write_field(PERI_VCORE_EEM_CON0, VCORE_EEMODSEL, SEL_VCORE_AO);
	eem_write_field(EEMCORESEL, APBSEL, det->ctrl_id);
	eem_ctrls[EEM_CTRL_VCORE].det_id = det_to_id(det);

	FUNC_EXIT(FUNC_LV_HELP);
}

static void switch_to_vcore_pdn(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);

	eem_write_field(PERI_VCORE_EEM_CON0, VCORE_EEMODSEL, SEL_VCORE_PDN);
	eem_write_field(EEMCORESEL, APBSEL, det->ctrl_id);
	eem_ctrls[EEM_CTRL_SOC].det_id = det_to_id(det);

	FUNC_EXIT(FUNC_LV_HELP);
}

#ifndef __KERNEL__
static int set_volt_vcore(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
	FUNC_EXIT(FUNC_LV_HELP);
	#if defined(__MTK_SLT_)
		#if defined(SLT_VMAX)
			det->volt_tbl_init2[0] = EEM_VOLT_TO_PMIC_VAL(131000) + EEM_PMIC_OFFSET;
			eem_debug("HV VCORE voltage EEM to 0x%X\n", det->volt_tbl_init2[0]);
		#elif defined(SLT_VMIN)
			det->volt_tbl_init2[0] = clamp(det->volt_tbl_init2[0] - 0xB,
				det->VMIN + EEM_PMIC_OFFSET, det->VMAX + EEM_PMIC_OFFSET);
			eem_debug("LV VCORE voltage EEM to 0x%X\n", det->volt_tbl_init2[0]);
		#else
			eem_debug("NV VCORE voltage EEM to 0x%x\n", det->volt_tbl_init2[0]);
		#endif
	#else
		eem_debug("VCORE EEM voltage to 0x%x\n", det->volt_tbl_init2[0]);
	#endif

	/* return mt_set_cur_volt_vcore_pdn(det->volt_tbl_pmic[0]); */  /* unit = 10 uv */
	#ifdef EARLY_PORTING
		return 0;
	#else
		return dvfs_set_vcore(det->volt_tbl_init2[0]);
	#endif
}
#endif

static int get_volt_vcore(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
	FUNC_EXIT(FUNC_LV_HELP);

	/* eem_debug("get_volt_vcore=%d\n",mt_get_cur_volt_vcore_ao()); */
	#ifdef EARLY_PORTING
	return 0;
	#else
	return mt_get_cur_volt_vcore_ao(); /* unit = 10 uv */
	#endif
}
#endif

/*=============================================================
 * Global function definition
 *=============================================================*/
#if 0
unsigned int mt_eem_get_level(void)
{
	unsigned int spd_bin_resv = 0, ret = 0;

	FUNC_ENTER(FUNC_LV_HELP);

	spd_bin_resv = (get_devinfo_with_index(15) >> 28) & 0x7;

	switch (spd_bin_resv) {
	case 1:
		ret = 1; /* 2.0G */
		break;

	case 2:
		ret = 2; /* 1.3G */
		break;

	case 4:
		ret = 2; /* 1.3G */
		break;

	default:
		ret = 0; /* 1.7G */
		break;
	}

	FUNC_EXIT(FUNC_LV_HELP);

	return ret;
}
#endif

void mt_ptp_lock(unsigned long *flags)
{
	/* FUNC_ENTER(FUNC_LV_HELP); */
	/* FIXME: lock with MD32 */
	/* get_md32_semaphore(SEMAPHORE_PTP); */
#ifdef __KERNEL__
	spin_lock_irqsave(&eem_spinlock, *flags);
#endif
	/* FUNC_EXIT(FUNC_LV_HELP); */
}
#ifdef __KERNEL__
EXPORT_SYMBOL(mt_ptp_lock);
#endif

void mt_ptp_unlock(unsigned long *flags)
{
	/* FUNC_ENTER(FUNC_LV_HELP); */
#ifdef __KERNEL__
	spin_unlock_irqrestore(&eem_spinlock, *flags);
	/* FIXME: lock with MD32 */
	/* release_md32_semaphore(SEMAPHORE_PTP); */
#endif
	/* FUNC_EXIT(FUNC_LV_HELP); */
}
#ifdef __KERNEL__
EXPORT_SYMBOL(mt_ptp_unlock);
#endif

#if 0
int mt_eem_idle_can_enter(void)
{
	struct eem_ctrl *ctrl;

	FUNC_ENTER(FUNC_LV_HELP);

	for_each_ctrl(ctrl) {
		if (atomic_read(&ctrl->in_init)) {
			FUNC_EXIT(FUNC_LV_HELP);
			return 0;
		}
	}

	FUNC_EXIT(FUNC_LV_HELP);

	return 1;
}
EXPORT_SYMBOL(mt_eem_idle_can_enter);
#endif

#ifdef __KERNEL__
/*
 * timer for log
 */
static enum hrtimer_restart eem_log_timer_func(struct hrtimer *timer)
{
	struct eem_det *det;

	FUNC_ENTER(FUNC_LV_HELP);

	for_each_det(det) {
		eem_error("Timer Bank=%d - (%d) - (%d, %d, %d, %d, %d, %d, %d, %d) - (0x%x), sts(%d)\n"
			   /*
			   "(%d, %d, %d, %d, %d, %d, %d, %d) -"
			   "(%d, %d, %d, %d)\n"*/,
			   det->ctrl_id, det->ops->get_temp(det),
			   EEM_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[0]),
			   EEM_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[1]),
			   EEM_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[2]),
			   EEM_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[3]),
			   EEM_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[4]),
			   EEM_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[5]),
			   EEM_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[6]),
			   EEM_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[7]),
			   det->t250,
			   det->ops->get_status(det)
			   /*
			   det->freq_tbl[0],
			   det->freq_tbl[1],
			   det->freq_tbl[2],
			   det->freq_tbl[3],
			   det->freq_tbl[4],
			   det->freq_tbl[5],
			   det->freq_tbl[6],
			   det->freq_tbl[7],
			   det->dcvalues[3],
			   det->eem_freqpct30[3],
			   det->eem_26c[3],
			   det->eem_vop30[3]*/);
	}

	hrtimer_forward_now(timer, ns_to_ktime(LOG_INTERVAL));
	FUNC_EXIT(FUNC_LV_HELP);

	return HRTIMER_RESTART;
}
#endif
/*
 * Thread for voltage setting
 */
static int eem_volt_thread_handler(void *data)
{
	struct eem_ctrl *ctrl = (struct eem_ctrl *)data;
	struct eem_det *det = id_to_eem_det(ctrl->det_id);

	FUNC_ENTER(FUNC_LV_HELP);
	if (det == NULL)
		return 0;

#ifdef __KERNEL__

	do {
		wait_event_interruptible(ctrl->wq, ctrl->volt_update);

		if ((ctrl->volt_update & EEM_VOLT_UPDATE) && det->ops->set_volt) {
#ifdef CONFIG_EEM_AEE_RR_REC
			int temp = -1;
			#if !defined(EARLY_PORTING) /* I-Chang */
			switch (det->ctrl_id) {
			case EEM_CTRL_LITTLE:
				aee_rr_rec_ptp_status(aee_rr_curr_ptp_status() |
					(1 << EEM_CPU_LITTLE_IS_SET_VOLT));
				temp = EEM_CPU_LITTLE_IS_SET_VOLT;
				break;

			case EEM_CTRL_GPU:
				aee_rr_rec_ptp_status(aee_rr_curr_ptp_status() |
					(1 << EEM_GPU_IS_SET_VOLT));
				temp = EEM_GPU_IS_SET_VOLT;
				break;

			case EEM_CTRL_BIG:
				aee_rr_rec_ptp_status(aee_rr_curr_ptp_status() |
					(1 << EEM_CPU_BIG_IS_SET_VOLT));
				temp = EEM_CPU_BIG_IS_SET_VOLT;
				break;

			default:
				break;
			}
			#endif
#endif
			det->ops->set_volt(det);

#if (defined(CONFIG_EEM_AEE_RR_REC) && !defined(EARLY_PORTING))
			if (temp >= EEM_CPU_LITTLE_IS_SET_VOLT)
				aee_rr_rec_ptp_status(aee_rr_curr_ptp_status() & ~(1 << temp));
#endif

		}
		if ((ctrl->volt_update & EEM_VOLT_RESTORE) && det->ops->restore_default_volt)
			det->ops->restore_default_volt(det);

		ctrl->volt_update = EEM_VOLT_NONE;

	} while (!kthread_should_stop());

#endif
	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

static void inherit_base_det(struct eem_det *det)
{
	/*
	 * Inherit ops from eem_det_base_ops if ops in det is NULL
	 */
	FUNC_ENTER(FUNC_LV_HELP);

	#define INIT_OP(ops, func)					\
		do {							\
			if (ops->func == NULL)				\
				ops->func = eem_det_base_ops.func;	\
		} while (0)

	INIT_OP(det->ops, disable);
	INIT_OP(det->ops, disable_locked);
	INIT_OP(det->ops, switch_bank);
	INIT_OP(det->ops, init01);
	INIT_OP(det->ops, init02);
	INIT_OP(det->ops, mon_mode);
	INIT_OP(det->ops, get_status);
	INIT_OP(det->ops, dump_status);
	INIT_OP(det->ops, set_phase);
	INIT_OP(det->ops, get_temp);
	INIT_OP(det->ops, get_volt);
	INIT_OP(det->ops, set_volt);
	INIT_OP(det->ops, restore_default_volt);
	INIT_OP(det->ops, get_freq_table);

	FUNC_EXIT(FUNC_LV_HELP);
}

static void eem_init_ctrl(struct eem_ctrl *ctrl)
{
	FUNC_ENTER(FUNC_LV_HELP);

	/* init_completion(&ctrl->init_done); */
	/* atomic_set(&ctrl->in_init, 0); */
#ifdef __KERNEL__

	if (1) {
		/* HAS_FEATURE(id_to_eem_det(ctrl->det_id), FEA_MON)) {
		TODO: FIXME, why doesn't work <-XXX */
		init_waitqueue_head(&ctrl->wq);
		ctrl->thread = kthread_run(eem_volt_thread_handler, ctrl, ctrl->name);

		if (IS_ERR(ctrl->thread))
			eem_debug("Create %s thread failed: %ld\n", ctrl->name, PTR_ERR(ctrl->thread));
	}

#endif
	FUNC_EXIT(FUNC_LV_HELP);
}

static void eem_init_det(struct eem_det *det, struct eem_devinfo *devinfo)
{
	enum eem_det_id det_id = det_to_id(det);

	FUNC_ENTER(FUNC_LV_HELP);
	eem_debug("det=%s, id=%d\n", ((char *)(det->name) + 8), det_id);

	inherit_base_det(det);

	/* init with devinfo */
	det->EEMINITEN	= devinfo->EEMINITEN;
	det->EEMMONEN   = devinfo->EEMMONEN;

	/* init with constant */
	det->DETWINDOW = DETWINDOW_VAL;
	det->VMAX = VMAX_VAL;
	det->VMIN = VMIN_VAL;
	det->VBOOT = EEM_VOLT_TO_PMIC_VAL(100000);

	det->DTHI = DTHI_VAL;
	det->DTLO = DTLO_VAL;
	det->DETMAX = DETMAX_VAL;

	det->AGECONFIG = AGECONFIG_VAL;
	det->AGEM = AGEM_VAL;
	#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
	if ((get_devinfo_with_index(38) & 0x100) && (get_devinfo_with_index(28) & 0x40000000)) {
		/* for Jade- UMC */
		det->DVTFIXED = 0;
		eem_error("Apply HT settings FEA(%d)\n", det->features);
	} else
		det->DVTFIXED = DVTFIXED_VAL;
	#else
	det->DVTFIXED = DVTFIXED_VAL;
	#endif
	det->VCO = VCO_VAL;
	det->DCCONFIG = DCCONFIG_VAL;

	if (NULL != det->ops->get_volt) {
		det->VBOOT = EEM_VOLT_TO_PMIC_VAL(det->ops->get_volt(det));
		eem_debug("@%s, %s-VBOOT = %d(%d)\n",
			__func__, ((char *)(det->name) + 8), det->VBOOT, det->ops->get_volt(det));
	}

	switch (det_id) {
	case EEM_DET_LITTLE:
		det->MDES	= devinfo->CPU0_MDES;
		det->BDES	= devinfo->CPU0_BDES;
		det->DCMDET	= devinfo->CPU0_DCMDET;
		det->DCBDET	= devinfo->CPU0_DCBDET;
		det->recordRef  = recordRef;

		#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
		/* For Jade- UMC use */
		if (get_devinfo_with_index(28) & 0x40000000)
			det->max_freq_khz = 1001000;
		#endif
		#if 0
			int i;

			for (int i = 0; i < NR_FREQ; i++)
				eem_debug("@(Record)%s----->(%s), = 0x%08x\n",
						__func__,
						det->name,
						*(recordRef + i));
		#endif
		break;

	case EEM_DET_BIG:
		det->MDES	= devinfo->CPU1_MDES;
		det->BDES	= devinfo->CPU1_BDES;
		det->DCMDET	= devinfo->CPU1_DCMDET;
		det->DCBDET	= devinfo->CPU1_DCBDET;
		det->recordRef  = recordRef + 36;

		#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
		/* For Jade- UMC use */
		if (get_devinfo_with_index(28) & 0x40000000)
			det->max_freq_khz = 1508000;
		#endif

		#if 0
			int i;

			for (int i = 0; i < NR_FREQ; i++)
				eem_debug("@(SRAM)%s----->(%s), = 0x%08x\n",
						__func__,
						det->name,
						*(recordRef + i));
		#endif
		break;

	case EEM_DET_GPU:
		det->MDES	= devinfo->GPU_MDES;
		det->BDES	= devinfo->GPU_BDES;
		det->DCMDET	= devinfo->GPU_DCMDET;
		det->DCBDET	= devinfo->GPU_DCBDET;
		det->VMAX	= VMAX_VAL_GPU;
		det->VMIN	= VMIN_VAL_GPU; /* override default setting */
		det->DVTFIXED	= DVTFIXED_VAL_GPU;
		det->VCO	= VCO_VAL_GPU;
		break;

	#if 0
	case EEM_DET_SOC:
		det->MDES	= devinfo->SOC_MDES;
		det->BDES	= devinfo->SOC_BDES;
		det->DCMDET	= devinfo->SOC_DCMDET;
		det->DCBDET	= devinfo->SOC_DCBDET;
		det->VMAX	= VMAX_VAL_SOC; /* override default setting */
		det->VMIN	= VMIN_VAL_SOC; /* override default setting */
		det->DVTFIXED	= DVTFIXED_VAL_SOC;
		det->VBOOT	= EEM_VOLT_TO_PMIC_VAL(125000); /* TODO: FIXME, unit = 10us */
		det->num_freq_tbl = 2;
		det->freq_tbl[0] = 1600 * 100 / 1600; /* XXX: percentage, 800/800, dedicated for VCORE only */
		det->freq_tbl[1] = 1333 * 100 / 1600; /* XXX: percentage, 600/800, dedicated for VCORE only */
		break;

	case EEM_DET_LTE:
		det->MDES	= devinfo->LTE_MDES;
		det->BDES	= devinfo->LTE_BDES;
		det->DCMDET	= devinfo->LTE_DCMDET;
		det->DCBDET	= devinfo->LTE_DCBDET;
		det->DVTFIXED = DVTFIXED_VAL_LTE;
		/* det->VBOOT	= EEM_VOLT_TO_PMIC_VAL(105000); */ /* TODO: FIXME, unit = 10us */
		det->VMAX	= VMAX_VAL_LTE; /* override default setting */
		det->VMIN	= VMIN_VAL_LTE; /* override default setting */
		det->num_freq_tbl = 1;
		det->freq_tbl[0] = 300 * 100 / 300; /* XXX: percentage, 300/300, dedicated for LTE only */
		break;
	#endif

	default:
		eem_debug("[%s]: Unknown det_id %d\n", __func__, det_id);
		break;
	}

	switch (det->ctrl_id) {
	case EEM_CTRL_LITTLE:
		det->AGEDELTA	= devinfo->CPU0_AGEDELTA;
		det->MTDES		= devinfo->CPU0_MTDES;
		break;

	case EEM_CTRL_GPU:
		det->AGEDELTA	= devinfo->GPU_AGEDELTA;
		det->MTDES	= devinfo->GPU_MTDES;
		break;

	case EEM_CTRL_BIG:
		det->AGEDELTA	= devinfo->CPU1_AGEDELTA;
		det->MTDES		= devinfo->CPU1_MTDES;
		break;

	#if 0
	case EEM_CTRL_SOC:
		det->AGEDELTA	= devinfo->SOC_AGEDELTA;
		det->MTDES	= devinfo->SOC_MTDES;
		break;

	case EEM_CTRL_LTE:
		det->AGEDELTA	= devinfo->LTE_AGEDELTA;
		det->MTDES	= devinfo->LTE_MTDES;
		break;
	#endif

	default:
		eem_debug("[%s]: Unknown ctrl_id %d\n", __func__, det->ctrl_id);
		break;
	}

	/* get DVFS frequency table */
	det->ops->get_freq_table(det);

	FUNC_EXIT(FUNC_LV_HELP);
}

int __attribute__((weak)) tscpu_is_temp_valid(void)
{
	return 1;
}

static void eem_set_eem_volt(struct eem_det *det)
{
#if SET_PMIC_VOLT
	unsigned i;
	int cur_temp, low_temp_offset;
	struct eem_ctrl *ctrl = id_to_eem_ctrl(det->ctrl_id);

	if (ctrl == NULL)
		return;

	cur_temp = det->ops->get_temp(det);
	/* eem_debug("eem_set_eem_volt cur_temp = %d, valid = %d\n", cur_temp, tscpu_is_temp_valid()); */
	if ((cur_temp <= 33000) || !tscpu_is_temp_valid()) {
		low_temp_offset = 10;
		ctrl->volt_update |= EEM_VOLT_UPDATE;
	} else {
		low_temp_offset = 0;
		ctrl->volt_update |= EEM_VOLT_UPDATE;
		#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
		if ((get_devinfo_with_index(38) & 0x100) && (get_devinfo_with_index(28) & 0x40000000))
			memcpy(det->volt_tbl, det->volt_tbl_init2, sizeof(det->volt_tbl_init2));
		#endif
	}
	/* eem_debug("ctrl->volt_update |= EEM_VOLT_UPDATE\n"); */

	/* all scale of volt_tbl_pmic, volt_tbl, volt_offset are pmic value */
	/* scale of det->volt_offset must equal 10uV */
	for (i = 0; i < det->num_freq_tbl; i++) {
		det->volt_tbl_pmic[i] = clamp(det->volt_tbl[i] + det->volt_offset + low_temp_offset + det->pi_offset,
						det->VMIN + EEM_PMIC_OFFSET,
						det->VMAX + EEM_PMIC_OFFSET);
		if (det_to_id(det) == EEM_DET_LITTLE)
			det->volt_tbl_pmic[i] = min(det->volt_tbl_pmic[i], (*(recordTbl + (i * 9) + 8) & 0x7F));
		else if (det_to_id(det) == EEM_DET_BIG)
			det->volt_tbl_pmic[i] = min(det->volt_tbl_pmic[i], (*(recordTbl + (i + 8) * 9 + 8) & 0x7F));
		#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
		else {
			if (eem_is_extbuck_valid) {
				if (8 == det->num_freq_tbl)
					det->volt_tbl_pmic[i] = min(det->volt_tbl_pmic[i], gpuSb[i]);
				else {
					if (0 == gpuFy[i])
						break;
					det->volt_tbl_pmic[i] = min(det->volt_tbl_pmic[i], gpuFy[i]);
				}
			}
		}
		#else
		else {
			if (8 == det->num_freq_tbl)
				det->volt_tbl_pmic[i] = min(det->volt_tbl_pmic[i], gpuSb[i]);
			else {
				if (0 == gpuFy[i])
					break;
				det->volt_tbl_pmic[i] = min(det->volt_tbl_pmic[i], gpuFy[i]);
			}
		}
		#endif
	}

#ifdef __KERNEL__
	if ((0 == (det->disabled % 2)) && (0 == (det->disabled & BY_PROCFS_INIT2)))
		wake_up_interruptible(&ctrl->wq);
	else
		eem_error("Disabled by [%d]\n", det->disabled);
#else
	#if defined(__MTK_SLT_)
		if ((ctrl->volt_update & EEM_VOLT_UPDATE) && det->ops->set_volt)
			det->ops->set_volt(det);

		if ((ctrl->volt_update & EEM_VOLT_RESTORE) && det->ops->restore_default_volt)
			det->ops->restore_default_volt(det);

		ctrl->volt_update = EEM_VOLT_NONE;
	#endif
#endif
#endif

	FUNC_ENTER(FUNC_LV_HELP);
	FUNC_EXIT(FUNC_LV_HELP);
}

static void eem_restore_eem_volt(struct eem_det *det)
{
#if SET_PMIC_VOLT
	struct eem_ctrl *ctrl = id_to_eem_ctrl(det->ctrl_id);

	if (ctrl == NULL)
		return;

	ctrl->volt_update |= EEM_VOLT_RESTORE;
#ifdef __KERNEL__
	wake_up_interruptible(&ctrl->wq);
#else
#if defined(__MTK_SLT_)
	if ((ctrl->volt_update & EEM_VOLT_UPDATE) && det->ops->set_volt)
			det->ops->set_volt(det);

	if ((ctrl->volt_update & EEM_VOLT_RESTORE) && det->ops->restore_default_volt)
			det->ops->restore_default_volt(det);

	ctrl->volt_update = EEM_VOLT_NONE;
#endif
#endif
#endif

	FUNC_ENTER(FUNC_LV_HELP);
	FUNC_EXIT(FUNC_LV_HELP);
}

#if 0
static void mt_eem_reg_dump_locked(void)
{
#ifndef CONFIG_ARM64
	unsigned int addr;

	for (addr = (unsigned int)DESCHAR; addr <= (unsigned int)SMSTATE1; addr += 4)
		eem_isr_info("0x%08X = 0x%08X\n", addr, *(volatile unsigned int *)addr);

	addr = (unsigned int)EEMCORESEL;
	eem_isr_info("0x%08X = 0x%08X\n", addr, *(volatile unsigned int *)addr);
#else
	unsigned long addr;

	for (addr = (unsigned long)DESCHAR; addr <= (unsigned long)SMSTATE1; addr += 4)
		eem_isr_info("0x%lu = 0x%lu\n", addr, *(volatile unsigned long *)addr);

	addr = (unsigned long)EEMCORESEL;
	eem_isr_info("0x%lu = 0x%lu\n", addr, *(volatile unsigned long *)addr);
#endif
}
#endif

static inline void handle_init01_isr(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_LOCAL);

	eem_isr_info("mode = init1 %s-isr\n", ((char *)(det->name) + 8));

	det->dcvalues[EEM_PHASE_INIT01]		= eem_read(DCVALUES);
	det->eem_freqpct30[EEM_PHASE_INIT01]	= eem_read(FREQPCT30);
	det->eem_26c[EEM_PHASE_INIT01]		= eem_read(EEMINTEN + 0x10);
	det->eem_vop30[EEM_PHASE_INIT01]	= eem_read(VOP30);
	det->eem_eemen[EEM_PHASE_INIT01]	= eem_read(EEMEN);

	/*
	#if defined(__MTK_SLT_)
	eem_debug("CTP - dcvalues 240 = 0x%x\n", det->dcvalues[EEM_PHASE_INIT01]);
	eem_debug("CTP - eem_freqpct30 218 = 0x%x\n", det->eem_freqpct30[EEM_PHASE_INIT01]);
	eem_debug("CTP - eem_26c 26c = 0x%x\n", det->eem_26c[EEM_PHASE_INIT01]);
	eem_debug("CTP - eem_vop30 248 = 0x%x\n", det->eem_vop30[EEM_PHASE_INIT01]);
	eem_debug("CTP - eem_eemen 238 = 0x%x\n", det->eem_eemen[EEM_PHASE_INIT01]);i
	#endif
	*/
#if DUMP_DATA_TO_DE
	{
		unsigned int i;

		for (i = 0; i < ARRAY_SIZE(reg_dump_addr_off); i++) {
			det->reg_dump_data[i][EEM_PHASE_INIT01] = eem_read(EEM_BASEADDR + reg_dump_addr_off[i]);
			#ifdef __KERNEL__
			eem_isr_info("0x%lx = 0x%08x\n",
			#else
			eem_isr_info("0x%X = 0x%X\n",
			#endif
				(unsigned long)EEM_BASEADDR + reg_dump_addr_off[i],
				det->reg_dump_data[i][EEM_PHASE_INIT01]
				);
		}
	}
#endif
	/*
	 * Read & store 16 bit values DCVALUES.DCVOFFSET and
	 * AGEVALUES.AGEVOFFSET for later use in INIT2 procedure
	 */
	det->DCVOFFSETIN = ~(eem_read(DCVALUES) & 0xffff) + 1; /* hw bug, workaround */
	det->AGEVOFFSETIN = eem_read(AGEVALUES) & 0xffff;

	/*
	 * Set EEMEN.EEMINITEN/EEMEN.EEMINIT2EN = 0x0 &
	 * Clear EEM INIT interrupt EEMINTSTS = 0x00000001
	 */
	eem_write(EEMEN, 0x0);
	eem_write(EEMINTSTS, 0x1);
	/* eem_init01_finish(det); */
	/* det->ops->init02(det); */

	FUNC_EXIT(FUNC_LV_LOCAL);
}
#if defined(__MTK_SLT_)
int cpu_in_mon;
#endif
static inline void handle_init02_isr(struct eem_det *det)
{
	unsigned int temp, i;
	/* struct eem_ctrl *ctrl = id_to_eem_ctrl(det->ctrl_id); */

	FUNC_ENTER(FUNC_LV_LOCAL);

	eem_isr_info("mode = init2 %s-isr\n", ((char *)(det->name) + 8));

	det->dcvalues[EEM_PHASE_INIT02]		= eem_read(DCVALUES);
	det->eem_freqpct30[EEM_PHASE_INIT02]	= eem_read(FREQPCT30);
	det->eem_26c[EEM_PHASE_INIT02]		= eem_read(EEMINTEN + 0x10);
	det->eem_vop30[EEM_PHASE_INIT02]	= eem_read(VOP30);
	det->eem_eemen[EEM_PHASE_INIT02]	= eem_read(EEMEN);

	/*
	#if defined(__MTK_SLT_)
	eem_debug("CTP - dcvalues 240 = 0x%x\n", det->dcvalues[EEM_PHASE_INIT02]);
	eem_debug("CTP - eem_freqpct30 218 = 0x%x\n", det->eem_freqpct30[EEM_PHASE_INIT02]);
	eem_debug("CTP - eem_26c 26c = 0x%x\n", det->eem_26c[EEM_PHASE_INIT02]);
	eem_debug("CTP - eem_vop30 248 = 0x%x\n", det->eem_vop30[EEM_PHASE_INIT02]);
	eem_debug("CTP - eem_eemen 238 = 0x%x\n", det->eem_eemen[EEM_PHASE_INIT02]);
	#endif
	*/

#if DUMP_DATA_TO_DE
	for (i = 0; i < ARRAY_SIZE(reg_dump_addr_off); i++) {
		det->reg_dump_data[i][EEM_PHASE_INIT02] = eem_read(EEM_BASEADDR + reg_dump_addr_off[i]);
		#ifdef __KERNEL__
		eem_isr_info("0x%lx = 0x%08x\n",
		#else
		eem_isr_info("0x%X = 0x%X\n",
		#endif
			(unsigned long)EEM_BASEADDR + reg_dump_addr_off[i],
			det->reg_dump_data[i][EEM_PHASE_INIT02]
			);
	}
#endif
	temp = eem_read(VOP30);
	/* eem_isr_info("init02 read(VOP30) = 0x%08X\n", temp); */
	/* VOP30=>pmic value */
	det->volt_tbl[0] = (temp & 0xff)       + EEM_PMIC_OFFSET;
	det->volt_tbl[1] = ((temp >> 8)  & 0xff) + EEM_PMIC_OFFSET;
	det->volt_tbl[2] = ((temp >> 16) & 0xff) + EEM_PMIC_OFFSET;
	det->volt_tbl[3] = ((temp >> 24) & 0xff) + EEM_PMIC_OFFSET;

	temp = eem_read(VOP74);
	/* eem_isr_info("init02 read(VOP74) = 0x%08X\n", temp); */
	/* VOP74=>pmic value */
	det->volt_tbl[4] = (temp & 0xff)       + EEM_PMIC_OFFSET;
	det->volt_tbl[5] = ((temp >> 8)  & 0xff) + EEM_PMIC_OFFSET;
	det->volt_tbl[6] = ((temp >> 16) & 0xff) + EEM_PMIC_OFFSET;
	det->volt_tbl[7] = ((temp >> 24) & 0xff) + EEM_PMIC_OFFSET;

	/* backup to volt_tbl_init2 */
	memcpy(det->volt_tbl_init2, det->volt_tbl, sizeof(det->volt_tbl_init2));

	for (i = 0; i < NR_FREQ; i++) {
#if defined(CONFIG_EEM_AEE_RR_REC) && !defined(EARLY_PORTING) /* I-Chang */
		switch (det->ctrl_id) {
		case EEM_CTRL_LITTLE:
			aee_rr_rec_ptp_cpu_little_volt(((unsigned long long)(det->volt_tbl[i]) << (8 * i)) |
				(aee_rr_curr_ptp_cpu_little_volt() & ~((unsigned long long)(0xFF) << (8 * i))));
			break;
		case EEM_CTRL_GPU:
			aee_rr_rec_ptp_gpu_volt(((unsigned long long)(det->volt_tbl[i]) << (8 * i)) |
				(aee_rr_curr_ptp_gpu_volt() & ~((unsigned long long)(0xFF) << (8 * i))));
			break;
		case EEM_CTRL_BIG:
			aee_rr_rec_ptp_cpu_big_volt(((unsigned long long)(det->volt_tbl[i]) << (8 * i)) |
				(aee_rr_curr_ptp_cpu_big_volt() & ~((unsigned long long)(0xFF) << (8 * i))));
			break;
		default:
			break;
		}
#endif
		/*
		eem_isr_info("init02_[%s].volt_tbl[%d] = 0x%08X (%d)\n",
			     det->name, i, det->volt_tbl[i], EEM_PMIC_VAL_TO_VOLT(det->volt_tbl[i]));
		*/
	}
	#if defined(__MTK_SLT_)
	if (det->ctrl_id <= EEM_CTRL_BIG)
		cpu_in_mon = 0;
	#endif
	eem_set_eem_volt(det);

	/*
	 * Set EEMEN.EEMINITEN/EEMEN.EEMINIT2EN = 0x0 &
	 * Clear EEM INIT interrupt EEMINTSTS = 0x00000001
	 */
	eem_write(EEMEN, 0x0);
	eem_write(EEMINTSTS, 0x1);

	/* atomic_dec(&ctrl->in_init); */
	/* complete(&ctrl->init_done); */
	det->ops->mon_mode(det);

	FUNC_EXIT(FUNC_LV_LOCAL);
}

static inline void handle_init_err_isr(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_LOCAL);
	eem_isr_info("====================================================\n");
	eem_isr_info("EEM init err: EEMEN(%p) = 0x%08X, EEMINTSTS(%p) = 0x%08X\n",
		     EEMEN, eem_read(EEMEN),
		     EEMINTSTS, eem_read(EEMINTSTS));
	eem_isr_info("SMSTATE0 (%p) = 0x%08X\n",
		     SMSTATE0, eem_read(SMSTATE0));
	eem_isr_info("SMSTATE1 (%p) = 0x%08X\n",
		     SMSTATE1, eem_read(SMSTATE1));
	eem_isr_info("====================================================\n");

	/*
	TODO: FIXME
	{
		struct eem_ctrl *ctrl = id_to_eem_ctrl(det->ctrl_id);
		atomic_dec(&ctrl->in_init);
		complete(&ctrl->init_done);
	}
	TODO: FIXME
	*/

#ifdef __KERNEL__
	aee_kernel_warning("mt_eem", "@%s():%d, get_volt(%s) = 0x%08X\n",
		__func__,
		__LINE__,
		det->name,
		det->VBOOT);
#endif
	det->ops->disable_locked(det, BY_INIT_ERROR);

	FUNC_EXIT(FUNC_LV_LOCAL);
}

static inline void handle_mon_mode_isr(struct eem_det *det)
{
	unsigned int temp, i;

	FUNC_ENTER(FUNC_LV_LOCAL);

	eem_isr_info("mode = mon %s-isr\n", ((char *)(det->name) + 8));

#if defined(CONFIG_THERMAL) && !defined(EARLY_PORTING)
			eem_isr_info("C0_temp=%d, C1_temp=%d GPU_temp=%d\n",
			tscpu_get_temp_by_bank(THERMAL_BANK0),
			tscpu_get_temp_by_bank(THERMAL_BANK2),
			tscpu_get_temp_by_bank(THERMAL_BANK1));
#endif

#if defined(CONFIG_EEM_AEE_RR_REC) && !defined(EARLY_PORTING)
	switch (det->ctrl_id) {
	case EEM_CTRL_LITTLE:
		#ifdef CONFIG_THERMAL
		if (tscpu_get_temp_by_bank(THERMAL_BANK0) != 0) {
			aee_rr_rec_ptp_temp(
			(tscpu_get_temp_by_bank(THERMAL_BANK0) / 1000) << (8 * EEM_CPU_LITTLE_IS_SET_VOLT) |
			(aee_rr_curr_ptp_temp() & ~(0xFF << (8 * EEM_CPU_LITTLE_IS_SET_VOLT))));
		}
		#endif
		break;

	case EEM_CTRL_GPU:
		#ifdef CONFIG_THERMAL
		if (tscpu_get_temp_by_bank(THERMAL_BANK1) != 0) {
			aee_rr_rec_ptp_temp(
			(tscpu_get_temp_by_bank(THERMAL_BANK1) / 1000) << (8 * EEM_GPU_IS_SET_VOLT) |
			(aee_rr_curr_ptp_temp() & ~(0xFF << (8 * EEM_GPU_IS_SET_VOLT))));
		}
		#endif
		break;

	case EEM_CTRL_BIG:
		#ifdef CONFIG_THERMAL
		if (tscpu_get_temp_by_bank(THERMAL_BANK2) != 0) {
			aee_rr_rec_ptp_temp(
			(tscpu_get_temp_by_bank(THERMAL_BANK2) / 1000) << (8 * EEM_CPU_BIG_IS_SET_VOLT) |
			(aee_rr_curr_ptp_temp() & ~(0xFF << (8 * EEM_CPU_BIG_IS_SET_VOLT))));
		}
		#endif
		break;

	default:
		break;
	}
#endif

	det->dcvalues[EEM_PHASE_MON]		= eem_read(DCVALUES);
	det->eem_freqpct30[EEM_PHASE_MON]	= eem_read(FREQPCT30);
	det->eem_26c[EEM_PHASE_MON]		= eem_read(EEMINTEN + 0x10);
	det->eem_vop30[EEM_PHASE_MON]		= eem_read(VOP30);
	det->eem_eemen[EEM_PHASE_MON]		= eem_read(EEMEN);

	/*
	#if defined(__MTK_SLT_)
	eem_debug("CTP - dcvalues 240 = 0x%x\n", det->dcvalues[EEM_PHASE_MON]);
	eem_debug("CTP - eem_freqpct30 218 = 0x%x\n", det->eem_freqpct30[EEM_PHASE_MON]);
	eem_debug("CTP - eem_26c 26c = 0x%x\n", det->eem_26c[EEM_PHASE_MON]);
	eem_debug("CTP - eem_vop30 248 = 0x%x\n", det->eem_vop30[EEM_PHASE_MON]);
	eem_debug("CTP - eem_eemen 238 = 0x%x\n", det->eem_eemen[EEM_PHASE_MON]);
	#endif
	*/

#if DUMP_DATA_TO_DE
	for (i = 0; i < ARRAY_SIZE(reg_dump_addr_off); i++) {
		det->reg_dump_data[i][EEM_PHASE_MON] = eem_read(EEM_BASEADDR + reg_dump_addr_off[i]);

		#ifdef __KERNEL__
		eem_isr_info("0x%lx = 0x%08x\n",
		#else
		eem_isr_info("0x%X = 0x%X\n",
		#endif
			(unsigned long)EEM_BASEADDR + reg_dump_addr_off[i],
			det->reg_dump_data[i][EEM_PHASE_MON]
			);
	}
#endif
	/* check if thermal sensor init completed? */
	det->t250 = eem_read(TEMP);

	if (((det->t250 & 0xff)  > 0x4b) && ((det->t250  & 0xff) < 0xd3)) {
		eem_isr_info("thermal sensor init has not been completed.(temp = 0x%08X)\n", det->t250);
		goto out;
	}

	temp = eem_read(VOP30);
	/* eem_debug("mon eem_read(VOP30) = 0x%08X\n", temp); */
	/* VOP30=>pmic value */
	det->volt_tbl[0] =  (temp & 0xff)	 + EEM_PMIC_OFFSET;
	det->volt_tbl[1] = ((temp >> 8)  & 0xff) + EEM_PMIC_OFFSET;
	det->volt_tbl[2] = ((temp >> 16) & 0xff) + EEM_PMIC_OFFSET;
	det->volt_tbl[3] = ((temp >> 24) & 0xff) + EEM_PMIC_OFFSET;

	temp = eem_read(VOP74);
	/* eem_debug("mon eem_read(VOP74) = 0x%08X\n", temp); */
	/* VOP74=>pmic value */
	det->volt_tbl[4] =  (temp & 0xff)        + EEM_PMIC_OFFSET;
	det->volt_tbl[5] = ((temp >> 8)  & 0xff) + EEM_PMIC_OFFSET;
	det->volt_tbl[6] = ((temp >> 16) & 0xff) + EEM_PMIC_OFFSET;
	det->volt_tbl[7] = ((temp >> 24) & 0xff) + EEM_PMIC_OFFSET;

	for (i = 0; i < NR_FREQ; i++) {
	#if defined(CONFIG_EEM_AEE_RR_REC) && !defined(EARLY_PORTING) /* I-Chang */
		switch (det->ctrl_id) {
		case EEM_CTRL_LITTLE:
			aee_rr_rec_ptp_cpu_little_volt(((unsigned long long)(det->volt_tbl[i]) << (8 * i)) |
				(aee_rr_curr_ptp_cpu_little_volt() & ~((unsigned long long)(0xFF) << (8 * i))));
			break;

		case EEM_CTRL_GPU:
			aee_rr_rec_ptp_gpu_volt(((unsigned long long)(det->volt_tbl[i]) << (8 * i)) |
				(aee_rr_curr_ptp_gpu_volt() & ~((unsigned long long)(0xFF) << (8 * i))));
			break;

		case EEM_CTRL_BIG:
			aee_rr_rec_ptp_cpu_big_volt(((unsigned long long)(det->volt_tbl[i]) << (8 * i)) |
				(aee_rr_curr_ptp_cpu_big_volt() & ~((unsigned long long)(0xFF) << (8 * i))));
			break;

		default:
			break;
		}
	#endif
		eem_isr_info("mon_[%s].volt_tbl[%d] = 0x%08X (%d)\n",
			     det->name, i, det->volt_tbl[i], EEM_PMIC_VAL_TO_VOLT(det->volt_tbl[i]));
	}
	eem_isr_info("ISR : EEM_TEMPSPARE1 = 0x%08X\n", eem_read(EEM_TEMPSPARE1));

	#if defined(__MTK_SLT_)
	if ((cpu_in_mon == 1) && (det->ctrl_id <= EEM_CTRL_BIG))
		eem_isr_info("Won't do CPU eem_set_eem_volt\n");
	else{
		if (det->ctrl_id <= EEM_CTRL_BIG) {
			eem_isr_info("Do CPU eem_set_eem_volt\n");
			cpu_in_mon = 1;
		}
		eem_set_eem_volt(det);
	}
	#else
	eem_set_eem_volt(det);
		#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
			if (EEM_CTRL_BIG == det->ctrl_id) {
				ctrl_ITurbo = (0 == ctrl_ITurbo) ? 0 : 2;
				eem_isr_info("Finished BIG monitor mode!!\n");
			}
		#endif
	#endif

out:
	/* Clear EEM INIT interrupt EEMINTSTS = 0x00ff0000 */
	eem_write(EEMINTSTS, 0x00ff0000);

	FUNC_EXIT(FUNC_LV_LOCAL);
}

static inline void handle_mon_err_isr(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_LOCAL);

	/* EEM Monitor mode error handler */
	eem_isr_info("====================================================\n");
	eem_isr_info("EEM mon err: EEMEN(%p) = 0x%08X, EEMINTSTS(%p) = 0x%08X\n",
		     EEMEN, eem_read(EEMEN),
		     EEMINTSTS, eem_read(EEMINTSTS));
	eem_isr_info("SMSTATE0 (%p) = 0x%08X\n",
		     SMSTATE0, eem_read(SMSTATE0));
	eem_isr_info("SMSTATE1 (%p) = 0x%08X\n",
		     SMSTATE1, eem_read(SMSTATE1));
	eem_isr_info("TEMP (%p) = 0x%08X\n",
		     TEMP, eem_read(TEMP));
	eem_isr_info("EEM_TEMPMSR0 (%p) = 0x%08X\n",
		     EEM_TEMPMSR0, eem_read(EEM_TEMPMSR0));
	eem_isr_info("EEM_TEMPMSR1 (%p) = 0x%08X\n",
		     EEM_TEMPMSR1, eem_read(EEM_TEMPMSR1));
	eem_isr_info("EEM_TEMPMSR2 (%p) = 0x%08X\n",
		     EEM_TEMPMSR2, eem_read(EEM_TEMPMSR2));
	eem_isr_info("EEM_TEMPMONCTL0 (%p) = 0x%08X\n",
		     EEM_TEMPMONCTL0, eem_read(EEM_TEMPMONCTL0));
	eem_isr_info("EEM_TEMPMSRCTL1 (%p) = 0x%08X\n",
		     EEM_TEMPMSRCTL1, eem_read(EEM_TEMPMSRCTL1));
	eem_isr_info("====================================================\n");

#ifdef __KERNEL__
	aee_kernel_warning("mt_eem", "@%s():%d, get_volt(%s) = 0x%08X\n", __func__, __LINE__, det->name, det->VBOOT);
#endif
	det->ops->disable_locked(det, BY_MON_ERROR);

	FUNC_EXIT(FUNC_LV_LOCAL);
}

static inline void eem_isr_handler(struct eem_det *det)
{
	unsigned int eemintsts, eemen;

	FUNC_ENTER(FUNC_LV_LOCAL);

	eemintsts = eem_read(EEMINTSTS);
	eemen = eem_read(EEMEN);

	eem_isr_info("Bank_number = %d %s-isr, 0x%X, 0x%X\n",
		det->ctrl_id, ((char *)(det->name) + 8), eemintsts, eemen);

	if (eemintsts == 0x1) { /* EEM init1 or init2 */
		if ((eemen & 0x7) == 0x1)   /* EEM init1 */
			handle_init01_isr(det);
		else if ((eemen & 0x7) == 0x5)   /* EEM init2 */
			handle_init02_isr(det);
		else {
			/* error : init1 or init2 */
			handle_init_err_isr(det);
		}
	} else if ((eemintsts & 0x00ff0000) != 0x0)
		handle_mon_mode_isr(det);
	else { /* EEM error handler */
		/* init 1  || init 2 error handler */
		if (((eemen & 0x7) == 0x1) || ((eemen & 0x7) == 0x5))
			handle_init_err_isr(det);
		else /* EEM Monitor mode error handler */
			handle_mon_err_isr(det);
	}

	FUNC_EXIT(FUNC_LV_LOCAL);
}
#ifdef __KERNEL__
static irqreturn_t eem_isr(int irq, void *dev_id)
#else
int ptp_isr(void)
#endif
{
	unsigned long flags;
	struct eem_det *det = NULL;
	int i;

	FUNC_ENTER(FUNC_LV_MODULE);

	mt_ptp_lock(&flags);

#if 0
	if (!(BIT(EEM_CTRL_VCORE) & eem_read(EEMODINTST))) {
		switch (eem_read_field(PERI_VCORE_EEM_CON0, VCORE_EEMODSEL)) {
		case SEL_VCORE_AO:
			det = &eem_detectors[PTP_DET_VCORE_AO];
			break;

		case SEL_VCORE_PDN:
			det = &eem_detectors[PTP_DET_VCORE_PDN];
			break;
		}

		if (likely(det)) {
			det->ops->switch_bank(det);
			eem_isr_handler(det);
		}
	}
#endif

	for (i = 0; i < NR_EEM_CTRL; i++) {
		/*
		if (i == EEM_CTRL_VCORE)
			continue;
		*/
		/* TODO: FIXME, it is better to link i @ struct eem_det */
		if ((BIT(i) & eem_read(EEMODINTST)))
			continue;

		det = &eem_detectors[i];

		det->ops->switch_bank(det);

		/*mt_eem_reg_dump_locked(); */

		eem_isr_handler(det);
	}

	mt_ptp_unlock(&flags);

	FUNC_EXIT(FUNC_LV_MODULE);
#ifdef __KERNEL__
	return IRQ_HANDLED;
#else
	return 0;
#endif
}

#if 0
static atomic_t eem_init01_cnt;
static void eem_init01_prepare(struct eem_det *det)
{
	FUNC_ENTER(FUNC_LV_LOCAL);

	atomic_inc(&eem_init01_cnt);

	if (atomic_read(&eem_init01_cnt) == 1) {
		enum mt_cpu_dvfs_id cpu;

		switch (det_to_id(det)) {
		case EEM_DET_LITTLE:
			cpu = MT_CPU_DVFS_LITTLE;
			break;

		case EEM_DET_BIG:
			cpu = MT_CPU_DVFS_BIG;
			break;

		default:
			return;
		}
	}

	FUNC_EXIT(FUNC_LV_LOCAL);
}

static void eem_init01_finish(struct eem_det *det)
{
	atomic_dec(&eem_init01_cnt);

	FUNC_ENTER(FUNC_LV_LOCAL);

	if (atomic_read(&eem_init01_cnt) < 0)
		BUG();

	if (atomic_read(&eem_init01_cnt) == 0) {
#if 0
		enum mt_cpu_dvfs_id cpu;

		switch (det_to_id(det)) {
		case EEM_DET_LITTLE:
			cpu = MT_CPU_DVFS_LITTLE;
			break;

		case EEM_DET_BIG:
			cpu = MT_CPU_DVFS_BIG;
			break;

		default:
			return;
		}
#endif
	}

	FUNC_EXIT(FUNC_LV_LOCAL);
}
#endif

#if defined(__KERNEL__) && defined(CONFIG_MTK_PMIC_CHIP_MT6353)
#define ITURBO_CPU_NUM 2
static int __cpuinit _mt_eem_cpu_CB(struct notifier_block *nfb,	unsigned long action, void *hcpu)
{
#if defined(CONFIG_MTK_MT6750TT)
	unsigned long flags;
	unsigned int cpu = (unsigned long)hcpu;
	unsigned int online_cpus = num_online_cpus();
	struct device *dev;
	struct eem_det *det;
	enum mt_eem_cpu_id cluster_id;

	/* CPU mask - Get on-line cpus per-cluster */
	struct cpumask eem_cpumask;
	struct cpumask cpu_online_cpumask;
	unsigned int cpus, big_cpus, timeout;

	if (ctrl_ITurbo < 2) {
		eem_debug("Default Turbo off (%d) !!", ctrl_ITurbo);
		return NOTIFY_OK;
	}
	/* eem_debug("Turbo start to run (%d) !!", ctrl_ITurbo); */
	tTbl = get_turbo(cpuBinLevel, cpuBinLevel_eng);

	/* Current active CPU is belong which cluster */
	cluster_id = arch_get_cluster_id(cpu);

	/* How many active CPU in this cluster, present by bit mask
		ex:	BIG	LITTLE
			1111	0000 */
	arch_get_cluster_cpus(&eem_cpumask, cluster_id);

	/* How many active CPU online in this cluster, present by number */
	cpumask_and(&cpu_online_cpumask, &eem_cpumask, cpu_online_mask);
	cpus = cpumask_weight(&cpu_online_cpumask);

	if (eem_log_en)
		eem_error("@%s():%d, cpu = %d, act = %lu, on_cpus = %d, clst = %d, clst_cpu = %d\n"
		, __func__, __LINE__, cpu, action, online_cpus, cluster_id, cpus);

	dev = get_cpu_device(cpu);
	if (dev) {
		det = id_to_eem_det(EEM_DET_BIG);
		arch_get_cluster_cpus(&eem_cpumask, MT_EEM_CPU_BIG);
		cpumask_and(&cpu_online_cpumask, &eem_cpumask, cpu_online_mask);
		big_cpus = cpumask_weight(&cpu_online_cpumask);

		switch (action) {
		case CPU_POST_DEAD:
			if ((0 == ITurboRun) &&
				(0 < big_cpus) &&
				(ITURBO_CPU_NUM >= big_cpus) &&
				(MT_EEM_CPU_BIG == cluster_id)) {
				if (eem_log_en)
					eem_error("Turbo(1) POST_DEAD (%d) BIG_cc(%d)\n", online_cpus, big_cpus);
				mt_ptp_lock(&flags);
				ITurboRun = 1;
				/* Revise BIG private table */
				/* CCI dcmdiv, CCI_div, dcm_div, clk_div, post_div, DDS */
				det->recordRef[1] =
					((tTbl[0] & 0x1F) << 27) |
					((tTbl[1] & 0x1F) << 22) |
					((tTbl[2] & 0x1F) << 17) |
					((tTbl[3] & 0x1F) << 12) |
					((tTbl[4] & 0x07) << 9) |
					(tTbl[5] & 0x1FF);
				mb(); /* SRAM writing */
				mt_ptp_unlock(&flags);
			} else {
				if (eem_log_en)
					eem_error("Turbo(%d)ed !! POST_DEAD (%d), BIG_cc(%d)\n",
						ITurboRun, online_cpus, big_cpus);
			}
		break;

		case CPU_DOWN_PREPARE:
			if ((1 == ITurboRun) && (1 == big_cpus) && (MT_EEM_CPU_BIG == cluster_id)) {
				if (eem_log_en)
					eem_error("Turbo(0) DP (%d) BIG_cc(%d)\n", online_cpus, big_cpus);
				mt_ptp_lock(&flags);
				ITurboRun = 0;
				/* Restore BIG private table */
				/* CCI dcmdiv, CCI_div, dcm_div, clk_div, post_div, DDS */
				det->recordRef[1] =
					((*(recordTbl + (8 * 9) + 0) & 0x1F) << 27) |
					((*(recordTbl + (8 * 9) + 1) & 0x1F) << 22) |
					((*(recordTbl + (8 * 9) + 2) & 0x1F) << 17) |
					((*(recordTbl + (8 * 9) + 3) & 0x1F) << 12) |
					((*(recordTbl + (8 * 9) + 4) & 0x07) << 9) |
					(*(recordTbl + (8 * 9) + 5) & 0x1FF);
				mb(); /* SRAM writing */
				mt_ptp_unlock(&flags);
			} else {
				if (eem_log_en)
					eem_error("Turbo(%d)ed !! DP (%d), BIG_cc(%d)\n",
						ITurboRun, online_cpus, big_cpus);
			}
		break;

		case CPU_UP_PREPARE:
			if ((0 == ITurboRun) && (0 == big_cpus) && (MT_EEM_CPU_BIG == cluster_id)) {
				if (eem_log_en)
					eem_error("Turbo(1) UP (%d), BIG_cc(%d)\n", online_cpus, big_cpus);
				mt_ptp_lock(&flags);
				ITurboRun = 1;
				/* Revise BIG private table */
				/* CCI dcmdiv, CCI_div, dcm_div, clk_div, post_div, DDS */
				det->recordRef[1] =
					((tTbl[0] & 0x1F) << 27) |
					((tTbl[1] & 0x1F) << 22) |
					((tTbl[2] & 0x1F) << 17) |
					((tTbl[3] & 0x1F) << 12) |
					((tTbl[4] & 0x07) << 9) |
					(tTbl[5] & 0x1FF);
				mb(); /* SRAM writing */
				mt_ptp_unlock(&flags);
			} else if ((1 == ITurboRun) &&
					((ITURBO_CPU_NUM == big_cpus) /* || (5 < cpu) */) &&
					(MT_EEM_CPU_BIG == cluster_id)) {
				if (eem_log_en)
					eem_error("Turbo(0) UP c(%d), on_c(%d), BIG_cc(%d)\n",
						cpu, online_cpus, big_cpus);
				mt_ptp_lock(&flags);
				ITurboRun = 0;
				/* Restore BIG private table */
				det->recordRef[1] =
					((*(recordTbl + (8 * 9) + 0) & 0x1F) << 27) |
					((*(recordTbl + (8 * 9) + 1) & 0x1F) << 22) |
					((*(recordTbl + (8 * 9) + 2) & 0x1F) << 17) |
					((*(recordTbl + (8 * 9) + 3) & 0x1F) << 12) |
					((*(recordTbl + (8 * 9) + 4) & 0x07) << 9) |
					(*(recordTbl + (8 * 9) + 5) & 0x1FF);
				mb(); /* SRAM writing */
				mt_ptp_unlock(&flags);
				timeout = 0;
				while (((eem_read(eem_apmixed_base + 0x204) >> 12) & 0x1FF) >
					*(recordTbl + (8 * 9) + 5)) {
					udelay(120);
					if (timeout == 100) {
						eem_error("DDS = %x, %x\n",
							(eem_read(eem_apmixed_base + 0x204) >> 12) & 0x1FF,
							*(recordTbl + (8 * 9) + 5));
						break;
					}
					timeout++;
				}
				udelay(120);
			} else {
				if (eem_log_en)
					eem_error("Turbo(%d)ed !! UP (%d), BIG_cc(%d)\n",
						ITurboRun, online_cpus, big_cpus);
			}
		break;
		}
	}
	return NOTIFY_OK;
#else
	tTbl = NULL;
	return NOTIFY_OK;
#endif
}

unsigned int get_turbo_status(void)
{
	return ITurboRun;
}
#endif

void eem_init02(const char *str)
{
	struct eem_det *det;
	struct eem_ctrl *ctrl;

	FUNC_ENTER(FUNC_LV_LOCAL);
	eem_error("eem_init02 called by [%s]\n", str);
	for_each_det_ctrl(det, ctrl) {
		if (HAS_FEATURE(det, FEA_INIT02)) {
			unsigned long flag;

			mt_ptp_lock(&flag);
			det->ops->init02(det);
			#if defined(__MTK_SLT_)
				mdelay(5);
			#endif
			mt_ptp_unlock(&flag);
		}
	}

	FUNC_EXIT(FUNC_LV_LOCAL);
}

void eem_init01(void)
{
	struct eem_det *det;
	struct eem_ctrl *ctrl;
	unsigned int out = 0, timeout = 0;

	FUNC_ENTER(FUNC_LV_LOCAL);

	for_each_det_ctrl(det, ctrl) {
		{
			unsigned long flag;
			unsigned int vboot = 0;

			if (NULL != det->ops->get_volt)
				vboot = EEM_VOLT_TO_PMIC_VAL(det->ops->get_volt(det));

			eem_debug("%s, vboot = %d, VBOOT = %d\n",
				((char *)(det->name) + 8), vboot, det->VBOOT);
#ifdef __KERNEL__

			if (vboot != det->VBOOT) {
				eem_error("@%s():%d, get_volt(%s) = 0x%08X, VBOOT = 0x%08X\n",
					__func__, __LINE__, det->name, vboot, det->VBOOT);
				aee_kernel_warning("mt_eem", "@%s():%d, get_volt(%s) = 0x%08X, VBOOT = 0x%08X\n",
					__func__, __LINE__, det->name, vboot, det->VBOOT);
			}

			BUG_ON(EEM_VOLT_TO_PMIC_VAL(det->ops->get_volt(det)) != det->VBOOT);
#endif
			mt_ptp_lock(&flag); /* <-XXX */
			det->ops->init01(det);
			mt_ptp_unlock(&flag); /* <-XXX */
		}

		/*
		 * VCORE_AO and VCORE_PDN use the same controller.
		 * Wait until VCORE_AO init01 and init02 done
		 */

		/*
		if (atomic_read(&ctrl->in_init)) {
			TODO: Use workqueue to avoid blocking
			wait_for_completion(&ctrl->init_done);
		}
		*/
	}

	/* This patch is waiting for whole bank finish the init01 then go
	 * next. Due to LL/L use same bulk PMIC, LL voltage table change
	 * will impact L to process init01 stage, because L require a
	 * stable 1V for init01.
	*/
	while (1) {
		for_each_det_ctrl(det, ctrl) {
			if ((EEM_CTRL_LITTLE == det->ctrl_id) && (1 == det->eem_eemen[EEM_PHASE_INIT01]))
				out |= BIT(EEM_CTRL_LITTLE);
			else if ((EEM_CTRL_GPU == det->ctrl_id) && (1 == det->eem_eemen[EEM_PHASE_INIT01]))
				out |= BIT(EEM_CTRL_GPU);
			else if ((EEM_CTRL_BIG == det->ctrl_id) && (1 == det->eem_eemen[EEM_PHASE_INIT01]))
				out |= BIT(EEM_CTRL_BIG);
		}
		if ((eemFeatureSts == out) || (30 == timeout)) {
			eem_debug("init01 finish time is %d\n", timeout);
			break;
		}
		udelay(100);
		timeout++;
	}
	eem_init02(__func__);
	FUNC_EXIT(FUNC_LV_LOCAL);
}

#if EN_EEM_OD
#if 0
static char *readline(struct file *fp)
{
#define BUFSIZE 1024
	static char buf[BUFSIZE]; /* TODO: FIXME, dynamic alloc */
	static int buf_end;
	static int line_start;
	static int line_end;
	char *ret;

	FUNC_ENTER(FUNC_LV_HELP);
empty:

	if (line_start >= buf_end) {
		line_start = 0;
		buf_end = fp->f_op->read(fp, &buf[line_end], sizeof(buf) - line_end, &fp->f_pos);

		if (0 == buf_end) {
			line_end = 0;
			return NULL;
		}

		buf_end += line_end;
	}

	while (buf[line_end] != '\n') {
		line_end++;

		if (line_end >= buf_end) {
			memcpy(&buf[0], &buf[line_start], buf_end - line_start);
			line_end = buf_end - line_start;
			line_start = buf_end;
			goto empty;
		}
	}

	buf[line_end] = '\0';
	ret = &buf[line_start];
	line_start = line_end + 1;

	FUNC_EXIT(FUNC_LV_HELP);

	return ret;
}
#endif
/* leakage */
unsigned int leakage_core;
unsigned int leakage_gpu;
unsigned int leakage_sram2;
unsigned int leakage_sram1;


void get_devinfo(struct eem_devinfo *p)
{
	int *val = (int *)p;

	FUNC_ENTER(FUNC_LV_HELP);

#ifndef EARLY_PORTING
	#if defined(__KERNEL__)
		#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
		if ((get_devinfo_with_index(38) & 0x200) && (get_devinfo_with_index(28) & 0x40000000)) {
			/* for Jade- UMC */
			val[0] = get_devinfo_with_index(37);
			val[1] = get_devinfo_with_index(38);
			val[2] = get_devinfo_with_index(39);
		} else {
			val[0] = get_devinfo_with_index(34);
			val[1] = get_devinfo_with_index(35);
			val[2] = get_devinfo_with_index(36);
		}
		#else
		val[0] = get_devinfo_with_index(34);
		val[1] = get_devinfo_with_index(35);
		val[2] = get_devinfo_with_index(36);
		#endif
		val[3] = get_devinfo_with_index(37);
		val[4] = get_devinfo_with_index(38);
		val[5] = get_devinfo_with_index(18);
		val[6] = get_devinfo_with_index(19);
	#else
		if ((eem_read(0x10206670) & 0x200) && (eem_read(0x10206540) & 0x40000000)) {
			/* for Jade- UMC */
			val[0] = eem_read(0x1020666C);
			val[1] = eem_read(0x10206670);
			val[2] = eem_read(0x10206674);
		} else {
			val[0] = eem_read(0x10206660); /* EEM0 */
			val[1] = eem_read(0x10206664); /* EEM1 */
			val[2] = eem_read(0x10206668); /* EEM2 */
		}
		val[3] = eem_read(0x1020666C); /* EEM3 */
		val[4] = eem_read(0x10206670); /* EEM4 */
		val[5] = eem_read(0x10206274); /* EEM_SRM_RP5 */
		val[6] = eem_read(0x10206278); /* EEM_SRM_RP6 */
	#endif
#else
	/* test pattern */
	val[0] = 0x071769F7;
	val[1] = 0x00260026;
	val[2] = 0x071769F7;
	val[3] = 0x071769F7;
	val[4] = 0x00260000;
	val[5] = 0x00000000;
	val[6] = 0x00000000;
#endif

	#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
	if ((p->CPU0_MDES != 0) && (p->CPU1_MDES != 0)) {
			val[7] = 0x01;
			val[7] = val[7] | (0x01<<8);
	}

	#else
	if ((p->CPU0_MDES != 0) && (p->CPU1_MDES != 0) && (p->GPU_MDES != 0)) {
		val[7] = 0x01;
		val[7] = val[7] | (0x01<<8);
	}

	#endif

	eem_debug("M_HW_RES0 = 0x%X\n", val[0]);
	eem_debug("M_HW_RES1 = 0x%X\n", val[1]);
	eem_debug("M_HW_RES2 = 0x%X\n", val[2]);
	eem_debug("M_HW_RES3 = 0x%X\n", val[3]);
	eem_debug("M_HW_RES4 = 0x%X\n", val[4]);
	eem_debug("M_HW_RES5 = 0x%X\n", val[5]);
	eem_debug("M_HW_RES6 = 0x%X\n", val[6]);
	eem_debug("M_HW_RES7 = 0x%X\n", val[7]);
	eem_debug("p->EEMINITEN=0x%x\n", p->EEMINITEN);
	eem_debug("p->EEMMONEN=0x%x\n", p->EEMMONEN);
	/* p->EEMINITEN = 0; */ /* TODO: FIXME */
	/* p->EEMMONEN  = 0; */ /* TODO: FIXME */
	#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
	fab = get_devinfo_with_index(28) & 0x40000000;
	segment = get_devinfo_with_index(21) & 0xFF;
	if (((segment == 0x41) ||
	     (segment == 0x45) ||
	     (segment == 0x40) ||
	     (segment == 0xC1) ||
	     (segment == 0xC5)) && !(fab))
		#if defined(CONFIG_MTK_MT6750TT)
		ctrl_ITurbo = 1; /* t */
		#else
		ctrl_ITurbo = 0; /* u */
		#endif
	#endif

	FUNC_EXIT(FUNC_LV_HELP);
}

static int eem_probe(struct platform_device *pdev)
{
	int ret;
	struct eem_det *det;
	struct eem_ctrl *ctrl;
	#if (defined(__KERNEL__) && !defined(CONFIG_MTK_CLKMGR))
		struct clk *clk_thermal;
		struct clk *clk_mfg, *clk_mfg_scp; /* for gpu clock use */
	#endif
	/* unsigned int code = mt_get_chip_hw_code(); */

	FUNC_ENTER(FUNC_LV_MODULE);

	#ifdef __KERNEL__
		#if !defined(CONFIG_MTK_CLKMGR)
			/* enable thermal CG */
			clk_thermal = devm_clk_get(&pdev->dev, "therm-eem");
			if (IS_ERR(clk_thermal)) {
				eem_error("cannot get thermal clock\n");
				return PTR_ERR(clk_thermal);
			}

			/* get GPU clock */
			clk_mfg = devm_clk_get(&pdev->dev, "mfg-main");
			if (IS_ERR(clk_mfg)) {
				eem_error("cannot get mfg main clock\n");
				return PTR_ERR(clk_mfg);
			}

			/* get GPU mtcomose */
			clk_mfg_scp = devm_clk_get(&pdev->dev, "mtcmos-mfg");
			if (IS_ERR(clk_mfg_scp)) {
				eem_error("cannot get mtcmos mfg\n");
				return PTR_ERR(clk_mfg_scp);
			}
			eem_debug("thmal=%p, gpu_clk=%p, gpu_mtcmos=%p",
				clk_thermal,
				clk_mfg,
				clk_mfg_scp);
		#else
			enable_clock(MT_CG_INFRA_THERM, "PTPOD");
			enable_clock(MT_CG_MFG_BG3D, "PTPOD");
		#endif

		/* set EEM IRQ */
		ret = request_irq(eem_irq_number, eem_isr, IRQF_TRIGGER_LOW, "ptp", NULL);

		if (ret) {
			eem_debug("EEM IRQ register failed (%d)\n", ret);
			WARN_ON(1);
		}
		eem_debug("Set EEM IRQ OK.\n");
	#endif

	/* eem_level = mt_eem_get_level(); */
	eem_debug("In eem_probe\n");
	/* atomic_set(&eem_init01_cnt, 0); */

	#if (defined(CONFIG_EEM_AEE_RR_REC) && !defined(EARLY_PORTING))
		_mt_eem_aee_init();
	#endif

	for_each_ctrl(ctrl) {
		eem_init_ctrl(ctrl);
	}
	#ifdef __KERNEL__
		#ifndef EARLY_PORTING
			/* disable frequency hopping (main PLL) */
			mt_fh_popod_save();/* I-Chang */

			/* disable DVFS and set vproc = 1v (LITTLE = 689 MHz)(BIG = 1196 MHz) */
			mt_ppm_ptpod_policy_activate();
			mt_gpufreq_disable_by_ptpod(); /* GPU bulk enable*/

			#if !defined(CONFIG_MTK_CLKMGR)
			ret = clk_prepare_enable(clk_thermal); /* Thermal clock enable */
			if (ret)
				eem_error("clk_prepare_enable failed when enabling THERMAL\n");

			ret = clk_prepare_enable(clk_mfg_scp); /* GPU MTCMOS enable*/
			if (ret)
				eem_error("clk_prepare_enable failed when enabling mfg MTCMOS\n");

			ret = clk_prepare_enable(clk_mfg); /* GPU CLOCK */
			if (ret)
				eem_error("clk_prepare_enable failed when enabling mfg clock\n");
			#endif
		#endif
	#else
		dvfs_disable_by_ptpod();
		gpu_dvfs_disable_by_ptpod();
	#endif

	#if (defined(__KERNEL__) && !defined(EARLY_PORTING))
	{
		/*
		extern unsigned int ckgen_meter(int val);
		eem_debug("@%s(), hf_faxi_ck = %d, hd_faxi_ck = %d\n",
			__func__, ckgen_meter(1), ckgen_meter(2));
		*/
	}
	#endif

	#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
		/* for Jade-(MT6353) */
		pmic_set_register_value(PMIC_RG_VPROC_MODESET, 1);
		pmic_set_register_value(PMIC_RG_VCORE_MODESET, 1);
		pmic_set_register_value(PMIC_RG_VCORE2_MODESET, 1);
		if (cpu_eem_is_extbuck_valid()) {
			eem_is_extbuck_valid = 1;
			mt6311_config_interface(0x7C, 0x1, 0x1, 6); /* set PWM mode for MT6311 */
		}
	#else
		/* for Jade/Everest/Olympus(MT6351) */
		pmic_force_vcore_pwm(true); /* set PWM mode for MT6351 */
		mt6311_config_interface(0x7C, 0x1, 0x1, 6); /* set PWM mode for MT6311 */
	#endif

	/* for slow idle */
	ptp_data[0] = 0xffffffff;

	for_each_det(det)
		eem_init_det(det, &eem_devinfo);

	#ifdef __KERNEL__
		mt_cpufreq_set_ptbl_registerCB(mt_cpufreq_set_ptbl_funcEEM);
		eem_init01();
	#endif
	ptp_data[0] = 0;

	#if (defined(__KERNEL__) && !defined(EARLY_PORTING))
		/*
		unsigned int ckgen_meter(int val);
		eem_debug("@%s(), hf_faxi_ck = %d, hd_faxi_ck = %d\n",
			__func__,
			ckgen_meter(1),
			ckgen_meter(2));
		*/
	#endif

	#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
		/* for Jade-(MT6353) */
		pmic_set_register_value(PMIC_RG_VPROC_MODESET, 0);
		pmic_set_register_value(PMIC_RG_VCORE_MODESET, 0);
		pmic_set_register_value(PMIC_RG_VCORE2_MODESET, 0);
		if (cpu_eem_is_extbuck_valid())
			mt6311_config_interface(0x7C, 0x0, 0x1, 6); /* set non-PWM mode for MT6311 */
	#else
		/* for Jade/Everest/Olympus(MT6351) */
		mt6311_config_interface(0x7C, 0x0, 0x1, 6); /* set non-PWM mode for MT6311 */
		pmic_force_vcore_pwm(false); /* set non-PWM mode for MT6351 */
	#endif

	#ifdef __KERNEL__
		#ifndef EARLY_PORTING
			#if !defined(CONFIG_MTK_CLKMGR)
				clk_disable_unprepare(clk_mfg); /* Disable GPU clock */
				clk_disable_unprepare(clk_mfg_scp); /* Disable GPU MTCMOSE */
				clk_disable_unprepare(clk_thermal); /* Disable Thermal clock */
			#endif
		mt_gpufreq_enable_by_ptpod();/* Disable GPU bulk */
		mt_ppm_ptpod_policy_deactivate();
		/* enable frequency hopping (main PLL) */
		mt_fh_popod_restore();/* I-Chang */
		#endif
	#else
		gpu_dvfs_enable_by_ptpod();
		dvfs_enable_by_ptpod();
	#endif

	#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
	register_hotcpu_notifier(&_mt_eem_cpu_notifier);
	#endif

	eem_debug("eem_probe ok\n");
	FUNC_EXIT(FUNC_LV_MODULE);

	return 0;
}

static int eem_suspend(struct platform_device *pdev, pm_message_t state)
{
	/*
	kthread_stop(eem_volt_thread);
	*/
	FUNC_ENTER(FUNC_LV_MODULE);
	FUNC_EXIT(FUNC_LV_MODULE);

	return 0;
}

static int eem_resume(struct platform_device *pdev)
{
	/*
	eem_volt_thread = kthread_run(eem_volt_thread_handler, 0, "eem volt");
	if (IS_ERR(eem_volt_thread))
	{
	    eem_debug("[%s]: failed to create eem volt thread\n", __func__);
	}
	*/
	FUNC_ENTER(FUNC_LV_MODULE);
	eem_init02(__func__);
	FUNC_EXIT(FUNC_LV_MODULE);

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id mt_eem_of_match[] = {
	{ .compatible = "mediatek,mt6755-ptp_fsm", },
	{},
};
#endif

static struct platform_driver eem_driver = {
	.remove     = NULL,
	.shutdown   = NULL,
	.probe      = eem_probe,
	.suspend    = eem_suspend,
	.resume     = eem_resume,
	.driver     = {
		.name   = "mt-eem",
#ifdef CONFIG_OF
		.of_match_table = mt_eem_of_match,
#endif
	},
};

#ifndef __KERNEL__
/*
 * For CTP
 */
int no_efuse;

int can_not_read_efuse(void)
{
	return no_efuse;
}

int mt_ptp_probe(void)
{
	eem_debug("CTP - In mt_ptp_probe\n");
	no_efuse = eem_init();

#if defined(__MTK_SLT_)
	if (no_efuse == -1)
		return 0;
#endif
	eem_probe(NULL);
	return 0;
}

void eem_init01_ctp(unsigned int id)
{
	struct eem_det *det;
	struct eem_ctrl *ctrl;
	unsigned int out = 0, timeout = 0;

	FUNC_ENTER(FUNC_LV_LOCAL);

	for_each_det_ctrl(det, ctrl) {
		unsigned long flag; /* <-XXX */

		eem_debug("CTP - (%d:%d)\n",
			EEM_VOLT_TO_PMIC_VAL(det->ops->get_volt(det)),
			det->VBOOT);

		mt_ptp_lock(&flag); /* <-XXX */
		det->ops->init01(det);
		mt_ptp_unlock(&flag); /* <-XXX */
	}

	while (1) {
		for_each_det_ctrl(det, ctrl) {
			if ((EEM_CTRL_LITTLE == det->ctrl_id) && (1 == det->eem_eemen[EEM_PHASE_INIT01]))
				out |= BIT(EEM_CTRL_LITTLE);
			else if ((EEM_CTRL_GPU == det->ctrl_id) && (1 == det->eem_eemen[EEM_PHASE_INIT01]))
				out |= BIT(EEM_CTRL_GPU);
			else if ((EEM_CTRL_BIG == det->ctrl_id) && (1 == det->eem_eemen[EEM_PHASE_INIT01]))
				out |= BIT(EEM_CTRL_BIG);
		}
		if ((0x07 == out) || (300 == timeout)) {
			eem_error("init01 finish time is %d\n", timeout);
			break;
		}
		udelay(100);
		timeout++;
	}

	thermal_init();
	udelay(500);
	eem_init02(__func__);

	FUNC_EXIT(FUNC_LV_LOCAL);
}

void ptp_init01_ptp(int id)
{
	eem_init01_ctp(id);
}

#endif

#ifdef CONFIG_PROC_FS
int mt_eem_opp_num(enum eem_det_id id)
{
	struct eem_det *det = id_to_eem_det(id);

	FUNC_ENTER(FUNC_LV_API);

	if (det == NULL)
		return 0;

	FUNC_EXIT(FUNC_LV_API);

	return det->num_freq_tbl;
}
EXPORT_SYMBOL(mt_eem_opp_num);

void mt_eem_opp_freq(enum eem_det_id id, unsigned int *freq)
{
	struct eem_det *det = id_to_eem_det(id);
	int i = 0;

	FUNC_ENTER(FUNC_LV_API);

	if (det == NULL)
		return;

	for (i = 0; i < det->num_freq_tbl; i++)
		freq[i] = det->freq_tbl[i];

	FUNC_EXIT(FUNC_LV_API);
}
EXPORT_SYMBOL(mt_eem_opp_freq);

void mt_eem_opp_status(enum eem_det_id id, unsigned int *temp, unsigned int *volt)
{
	struct eem_det *det = id_to_eem_det(id);
	int i = 0;

	FUNC_ENTER(FUNC_LV_API);

	if (det == NULL)
		return;

#if defined(__KERNEL__) && defined(CONFIG_THERMAL) && !defined(EARLY_PORTING)
	*temp = tscpu_get_temp_by_bank(
		(id == EEM_DET_LITTLE) ? THERMAL_BANK0 :
		(id == EEM_DET_BIG) ? THERMAL_BANK2 : THERMAL_BANK1); /* I-Chang */
#else
	*temp = 0;
#endif

	for (i = 0; i < det->num_freq_tbl; i++)
		volt[i] = EEM_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[i]);

	FUNC_EXIT(FUNC_LV_API);
}
EXPORT_SYMBOL(mt_eem_opp_status);

/***************************
* return current EEM stauts
****************************/
int mt_eem_status(enum eem_det_id id)
{
	struct eem_det *det = id_to_eem_det(id);

	FUNC_ENTER(FUNC_LV_API);

	BUG_ON(!det);
	BUG_ON(!det->ops);
	BUG_ON(!det->ops->get_status);

	FUNC_EXIT(FUNC_LV_API);

	return det->ops->get_status(det);
}

/**
 * ===============================================
 * PROCFS interface for debugging
 * ===============================================
 */

/*
 * show current EEM stauts
 */
static int eem_debug_proc_show(struct seq_file *m, void *v)
{
	struct eem_det *det = (struct eem_det *)m->private;

	FUNC_ENTER(FUNC_LV_HELP);

	/* FIXME: EEMEN sometimes is disabled temp */
	seq_printf(m, "[%s] (%s, %d) (%d)\n",
		   ((char *)(det->name) + 8),
		   det->disabled ? "disabled" : "enable",
		   det->disabled,
		   det->ops->get_status(det)
		   );

	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

/*
 * set EEM status by procfs interface
 */
static ssize_t eem_debug_proc_write(struct file *file,
				    const char __user *buffer, size_t count, loff_t *pos)
{
	int ret;
	int enabled = 0;
	char *buf = (char *) __get_free_page(GFP_USER);
	struct eem_det *det = (struct eem_det *)PDE_DATA(file_inode(file));

	FUNC_ENTER(FUNC_LV_HELP);

	if (!buf) {
		FUNC_EXIT(FUNC_LV_HELP);
		return -ENOMEM;
	}

	ret = -EINVAL;

	if (count >= PAGE_SIZE)
		goto out;

	ret = -EFAULT;

	if (copy_from_user(buf, buffer, count))
		goto out;

	buf[count] = '\0';

	if (!kstrtoint(buf, 10, &enabled)) {
		ret = 0;

		if (0 == enabled)
			/* det->ops->enable(det, BY_PROCFS); */
			det->ops->disable(det, 0);
		else if (1 == enabled)
			det->ops->disable(det, BY_PROCFS);
		else if (2 == enabled)
			det->ops->disable(det, BY_PROCFS_INIT2);
	} else
		ret = -EINVAL;

out:
	free_page((unsigned long)buf);
	FUNC_EXIT(FUNC_LV_HELP);

	return (ret < 0) ? ret : count;
}

/*
 * show current EEM data
 */
static int eem_dump_proc_show(struct seq_file *m, void *v)
{
	struct eem_det *det;
	int *val = (int *)&eem_devinfo;
	int i;
	#if DUMP_DATA_TO_DE
		int j;
	#endif


	FUNC_ENTER(FUNC_LV_HELP);
	/*
	eem_detectors[EEM_DET_BIG].ops->dump_status(&eem_detectors[EEM_DET_BIG]);
	eem_detectors[EEM_DET_LITTLE].ops->dump_status(&eem_detectors[EEM_DET_LITTLE]);
	seq_printf(m, "det->EEMMONEN= 0x%08X,det->EEMINITEN= 0x%08X\n", det->EEMMONEN, det->EEMINITEN);
	seq_printf(m, "leakage_core\t= %d\n"
			"leakage_gpu\t= %d\n"
			"leakage_little\t= %d\n"
			"leakage_big\t= %d\n",
			leakage_core,
			leakage_gpu,
			leakage_sram2,
			leakage_sram1
			);
	*/


	for (i = 0; i < sizeof(struct eem_devinfo)/sizeof(unsigned int); i++)
		seq_printf(m, "M_HW_RES%d\t= 0x%08X\n", i, val[i]);

	for_each_det(det) {
		for (i = EEM_PHASE_INIT01; i < NR_EEM_PHASE; i++) {
			seq_printf(m, "Bank_number = %d\n", det->ctrl_id);
			if (i < EEM_PHASE_MON)
				seq_printf(m, "mode = init%d\n", i+1);
			else
				seq_puts(m, "mode = mon\n");
			if (eem_log_en) {
				seq_printf(m, "0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X\n",
					det->dcvalues[i],
					det->eem_freqpct30[i],
					det->eem_26c[i],
					det->eem_vop30[i],
					det->eem_eemen[i]
				);

				/* if (det->eem_eemen[i] == 0x5) */
				{
					seq_printf(m, "EEM_LOG: Bank_number = [%d] (%d)\n",
					det->ctrl_id, det->ops->get_temp(det));

					seq_printf(m, "p(%d, %d, %d, %d, %d, %d, %d, %d)\n",
					EEM_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[0]),
					EEM_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[1]),
					EEM_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[2]),
					EEM_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[3]),
					EEM_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[4]),
					EEM_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[5]),
					EEM_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[6]),
					EEM_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[7]));

					seq_printf(m, "2(%d, %d, %d, %d, %d, %d, %d, %d)\n",
					EEM_PMIC_VAL_TO_VOLT(det->volt_tbl_init2[0]),
					EEM_PMIC_VAL_TO_VOLT(det->volt_tbl_init2[1]),
					EEM_PMIC_VAL_TO_VOLT(det->volt_tbl_init2[2]),
					EEM_PMIC_VAL_TO_VOLT(det->volt_tbl_init2[3]),
					EEM_PMIC_VAL_TO_VOLT(det->volt_tbl_init2[4]),
					EEM_PMIC_VAL_TO_VOLT(det->volt_tbl_init2[5]),
					EEM_PMIC_VAL_TO_VOLT(det->volt_tbl_init2[6]),
					EEM_PMIC_VAL_TO_VOLT(det->volt_tbl_init2[7]));

					seq_printf(m, "f(%d, %d, %d, %d, %d, %d, %d, %d)\n",
					det->freq_tbl[0],
					det->freq_tbl[1],
					det->freq_tbl[2],
					det->freq_tbl[3],
					det->freq_tbl[4],
					det->freq_tbl[5],
					det->freq_tbl[6],
					det->freq_tbl[7]);
				}
			}
			#if DUMP_DATA_TO_DE
			for (j = 0; j < ARRAY_SIZE(reg_dump_addr_off); j++)
				seq_printf(m, "0x%08lx = 0x%08x\n",
					(unsigned long)EEM_BASEADDR + reg_dump_addr_off[j],
					det->reg_dump_data[j][i]
					);
			#endif
		}
	}
	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

/*
 * show current voltage
 */
static int eem_cur_volt_proc_show(struct seq_file *m, void *v)
{
	struct eem_det *det = (struct eem_det *)m->private;
	u32 rdata = 0, i;

	FUNC_ENTER(FUNC_LV_HELP);

	rdata = det->ops->get_volt(det);

	if (rdata != 0)
		seq_printf(m, "%d\n", rdata);
	else
		seq_printf(m, "EEM[%s] read current voltage fail\n", det->name);

	if (EEM_CTRL_GPU != det->ctrl_id) {
		for (i = 0; i < NR_FREQ; i++) {
			seq_printf(m, "(SSC, 0x%x) (Vs, 0x%x) (Vp, 0x%x)\n",
				det->recordRef[i*2] >> 16 & 0xFFF,
				det->recordRef[i*2] >> 7 & 0x7F,
				det->recordRef[i*2] & 0x7F
				);

			seq_printf(m, "(F_Setting)(%x, %x, %x, %x, %x, %x)\n",
				(det->recordRef[i*2+1] >> 27) & 0x1F,
				(det->recordRef[i*2+1] >> 22) & 0x1F,
				(det->recordRef[i*2+1] >> 17) & 0x1F,
				(det->recordRef[i*2+1] >> 12) & 0x1F,
				(det->recordRef[i*2+1] >> 9) & 0x07,
				det->recordRef[i*2+1] & 0x1ff
				);
		}
	}
	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

/*
 * update record message
 */
static ssize_t eem_cur_volt_proc_write(struct file *file,
				     const char __user *buffer, size_t count, loff_t *pos)
{
	int ret;
	char *buf = (char *) __get_free_page(GFP_USER);
	unsigned int voltValue = 0, voltProc = 0, voltSram = 0, voltPmic = 0, index = 0;
	struct eem_det *det = (struct eem_det *)PDE_DATA(file_inode(file));

	FUNC_ENTER(FUNC_LV_HELP);

	if (!buf) {
		FUNC_EXIT(FUNC_LV_HELP);
		return -ENOMEM;
	}

	ret = -EINVAL;

	if (count >= PAGE_SIZE)
		goto out;

	ret = -EFAULT;

	if (copy_from_user(buf, buffer, count))
		goto out;

	buf[count] = '\0';

	if (EEM_CTRL_GPU != det->ctrl_id) {
		/* if (sscanf(buf, "%d", &voltValue) == 1) { */
		if (2 == sscanf(buf, "%u %u", &voltValue, &index)) {
			ret = 0;
			det->recordRef[NR_FREQ * 2] = 0x00000000;
			mb(); /* SRAM writing */
			voltPmic = EEM_VOLT_TO_PMIC_VAL(voltValue) + EEM_PMIC_OFFSET;
			voltProc = clamp(voltPmic,
				det->VMIN + EEM_PMIC_OFFSET,
				det->VMAX + EEM_PMIC_OFFSET);

			voltSram = clamp((unsigned int)(voltProc + 0x10),
				(unsigned int)(VMIN_SRAM + EEM_PMIC_OFFSET),
				(unsigned int)(det->VMAX + EEM_PMIC_OFFSET));

			/* for (i = 0; i < NR_FREQ; i++) */
			if ((index >= 0) && (index < 8))
				det->recordRef[index*2] = (det->recordRef[index*2] & (~0x3FFF)) |
					(((voltSram & 0x7F) << 7) | (voltProc & 0x7F));

			det->recordRef[NR_FREQ * 2] = 0xFFFFFFFF;
			mb(); /* SRAM writing */
		} else {
			ret = -EINVAL;
			eem_debug("bad argument_1!! argument should be 80000 ~ 115500, index = (0 ~ 8)\n");
		}
	}
out:
	free_page((unsigned long)buf);
	FUNC_EXIT(FUNC_LV_HELP);

	return (ret < 0) ? ret : count;
}

#ifdef __KERNEL__
#define VPROC_MAX 125000
#define CPU_TURBO_VOLT_DIFF_1 4375
#define CPU_TURBO_VOLT_DIFF_2 2500
#define NUM_OF_CPU_FREQ_TABLE 8
#define NUM_OF_GPU_FREQ_TABLE 8

static int volt_to_buck_volt(u32 cur_volt, int need)
{
	if (need) {
		if (cur_volt >= VPROC_MAX)
			return VPROC_MAX;
		else {
			if (cur_volt % 625 == 0)
				return cur_volt;
			else
				return (cur_volt + 625) / 1000 * 1000;
		}
	}
	return cur_volt;
}

static int eem_not_work(u32 cur_volt, unsigned int *volt_tbl_pmic, int num)
{
	int i;
	int need = 0;

	if (num == NUM_OF_CPU_FREQ_TABLE) {
		need = 1;
		eem_isr_info("CPU: In eem_not_work, cur_volt = %d -> %d\n",
			cur_volt,
			EEM_VOLT_TO_PMIC_VAL(cur_volt));
	} else
		eem_isr_info("GPU: In eem_not_work, cur_volt = %d -> %d\n",
			cur_volt,
			EEM_VOLT_TO_PMIC_VAL(cur_volt));

	for (i = 0; i < num; i++) {
		eem_isr_info("volt_tbl_pmic[%d] = %d => %d\n",
				i,
				volt_tbl_pmic[i],
				volt_to_buck_volt(EEM_PMIC_VAL_TO_VOLT(volt_tbl_pmic[i]), need));

		if (cur_volt == volt_to_buck_volt(EEM_PMIC_VAL_TO_VOLT(volt_tbl_pmic[i]), need))
			return 0;
	}

	if ((cur_volt == (EEM_PMIC_VAL_TO_VOLT(volt_tbl_pmic[0]) + CPU_TURBO_VOLT_DIFF_1)) ||
		(cur_volt == (EEM_PMIC_VAL_TO_VOLT(volt_tbl_pmic[0]) + CPU_TURBO_VOLT_DIFF_2)))	{
		eem_isr_info("%d, %d, %d\n",
			cur_volt,
			EEM_PMIC_VAL_TO_VOLT(volt_tbl_pmic[0]) + CPU_TURBO_VOLT_DIFF_1,
			EEM_PMIC_VAL_TO_VOLT(volt_tbl_pmic[0]) + CPU_TURBO_VOLT_DIFF_2);
		return 0;
	}
	return 1;
}

static int eem_stress_result_proc_show(struct seq_file *m, void *v)
{
	unsigned int result = 0;
	struct eem_det *det;
	u32 rdata = 0;

	FUNC_ENTER(FUNC_LV_HELP);

	for_each_det(det) {
		rdata = det->ops->get_volt(det);
		switch (det->ctrl_id) {
		case EEM_CTRL_LITTLE: /* I-Chang */
			result |= (eem_not_work(rdata, det->volt_tbl_pmic,
				NUM_OF_CPU_FREQ_TABLE) << EEM_CTRL_LITTLE);
			if ((result & (1 << EEM_CTRL_LITTLE)) != 0)
				eem_isr_info("BANK0 EEM fail\n");
			break;

		case EEM_CTRL_BIG: /* I-Chang */
			result |= (eem_not_work(rdata, det->volt_tbl_pmic,
				NUM_OF_CPU_FREQ_TABLE) << EEM_CTRL_BIG);
			if ((result & (1 << EEM_CTRL_BIG)) != 0)
				eem_isr_info("BANK2 EEM fail\n");
			break;

		case EEM_CTRL_GPU: /* I-Chang */
			result |= (eem_not_work(rdata, det->volt_tbl_pmic,
				NUM_OF_GPU_FREQ_TABLE) << EEM_CTRL_GPU);
			if ((result & (1 << EEM_CTRL_GPU)) != 0)
				eem_isr_info("GPU EEM fail\n");
			break;

		default:
			break;
		}
	}
	seq_printf(m, "0x%X\n", result);
	return 0;
}
#endif
/*
 * show current EEM status
 */
static int eem_status_proc_show(struct seq_file *m, void *v)
{
	struct eem_det *det = (struct eem_det *)m->private;

	FUNC_ENTER(FUNC_LV_HELP);

	seq_printf(m, "bank = %d, (%d) - p(%d, %d, %d, %d, %d, %d, %d, %d) - (%d, %d, %d, %d, %d, %d, %d, %d)\n",
		   det->ctrl_id, det->ops->get_temp(det),
		   EEM_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[0]),
		   EEM_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[1]),
		   EEM_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[2]),
		   EEM_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[3]),
		   EEM_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[4]),
		   EEM_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[5]),
		   EEM_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[6]),
		   EEM_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[7]),

		   det->freq_tbl[0],
		   det->freq_tbl[1],
		   det->freq_tbl[2],
		   det->freq_tbl[3],
		   det->freq_tbl[4],
		   det->freq_tbl[5],
		   det->freq_tbl[6],
		   det->freq_tbl[7]);

	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

/*
 * set EEM log enable by procfs interface
 */

static int eem_log_en_proc_show(struct seq_file *m, void *v)
{
	FUNC_ENTER(FUNC_LV_HELP);
	seq_printf(m, "%d\n", eem_log_en);
	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

static ssize_t eem_log_en_proc_write(struct file *file,
				     const char __user *buffer, size_t count, loff_t *pos)
{
	int ret;
	char *buf = (char *) __get_free_page(GFP_USER);

	FUNC_ENTER(FUNC_LV_HELP);

	if (!buf) {
		FUNC_EXIT(FUNC_LV_HELP);
		return -ENOMEM;
	}

	ret = -EINVAL;

	if (count >= PAGE_SIZE)
		goto out;

	ret = -EFAULT;

	if (copy_from_user(buf, buffer, count))
		goto out;

	buf[count] = '\0';

	ret = -EINVAL;

	if (kstrtoint(buf, 10, &eem_log_en)) {
		eem_debug("bad argument!! Should be \"0\" or \"1\"\n");
		goto out;
	}

	ret = 0;

	switch (eem_log_en) {
	case 0:
		eem_debug("EEM log disabled.\n");
		hrtimer_cancel(&eem_log_timer);
		break;

	case 1:
		eem_debug("EEM log enabled.\n");
		hrtimer_start(&eem_log_timer, ns_to_ktime(LOG_INTERVAL), HRTIMER_MODE_REL);
		break;

	default:
		eem_debug("bad argument!! Should be \"0\" or \"1\"\n");
		ret = -EINVAL;
	}

out:
	free_page((unsigned long)buf);
	FUNC_EXIT(FUNC_LV_HELP);

	return (ret < 0) ? ret : count;
}


/*
 * show EEM offset
 */
static int eem_offset_proc_show(struct seq_file *m, void *v)
{
	struct eem_det *det = (struct eem_det *)m->private;

	FUNC_ENTER(FUNC_LV_HELP);

	seq_printf(m, "%d\n", det->volt_offset);

	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

/*
 * set EEM offset by procfs
 */
static ssize_t eem_offset_proc_write(struct file *file,
				     const char __user *buffer, size_t count, loff_t *pos)
{
	int ret;
	char *buf = (char *) __get_free_page(GFP_USER);
	int offset = 0;
	struct eem_det *det = (struct eem_det *)PDE_DATA(file_inode(file));

	FUNC_ENTER(FUNC_LV_HELP);

	if (!buf) {
		FUNC_EXIT(FUNC_LV_HELP);
		return -ENOMEM;
	}

	ret = -EINVAL;

	if (count >= PAGE_SIZE)
		goto out;

	ret = -EFAULT;

	if (copy_from_user(buf, buffer, count))
		goto out;

	buf[count] = '\0';

	if (!kstrtoint(buf, 10, &offset)) {
		ret = 0;
		det->volt_offset = offset;
		eem_set_eem_volt(det);
	} else {
		ret = -EINVAL;
		eem_debug("bad argument_1!! argument should be \"0\"\n");
	}

out:
	free_page((unsigned long)buf);
	FUNC_EXIT(FUNC_LV_HELP);

	return (ret < 0) ? ret : count;
}

#define PROC_FOPS_RW(name)					\
	static int name ## _proc_open(struct inode *inode,	\
		struct file *file)				\
	{							\
		return single_open(file, name ## _proc_show,	\
			PDE_DATA(inode));			\
	}							\
	static const struct file_operations name ## _proc_fops = {	\
		.owner          = THIS_MODULE,				\
		.open           = name ## _proc_open,			\
		.read           = seq_read,				\
		.llseek         = seq_lseek,				\
		.release        = single_release,			\
		.write          = name ## _proc_write,			\
	}

#define PROC_FOPS_RO(name)					\
	static int name ## _proc_open(struct inode *inode,	\
		struct file *file)				\
	{							\
		return single_open(file, name ## _proc_show,	\
			PDE_DATA(inode));			\
	}							\
	static const struct file_operations name ## _proc_fops = {	\
		.owner          = THIS_MODULE,				\
		.open           = name ## _proc_open,			\
		.read           = seq_read,				\
		.llseek         = seq_lseek,				\
		.release        = single_release,			\
	}

#define PROC_ENTRY(name)	{__stringify(name), &name ## _proc_fops}

PROC_FOPS_RW(eem_debug);
PROC_FOPS_RO(eem_dump);
PROC_FOPS_RO(eem_stress_result);
PROC_FOPS_RW(eem_log_en);
PROC_FOPS_RO(eem_status);
PROC_FOPS_RW(eem_cur_volt);
PROC_FOPS_RW(eem_offset);

static int create_procfs(void)
{
	struct proc_dir_entry *eem_dir = NULL;
	struct proc_dir_entry *det_dir = NULL;
	int i;
	struct eem_det *det;

	struct pentry {
		const char *name;
		const struct file_operations *fops;
	};

	struct pentry det_entries[] = {
		PROC_ENTRY(eem_debug),
		PROC_ENTRY(eem_status),
		PROC_ENTRY(eem_cur_volt),
		PROC_ENTRY(eem_offset),
	};

	struct pentry eem_entries[] = {
		PROC_ENTRY(eem_dump),
		PROC_ENTRY(eem_log_en),
		PROC_ENTRY(eem_stress_result),
	};

	FUNC_ENTER(FUNC_LV_HELP);

	eem_dir = proc_mkdir("eem", NULL);

	if (!eem_dir) {
		eem_debug("[%s]: mkdir /proc/eem failed\n", __func__);
		FUNC_EXIT(FUNC_LV_HELP);
		return -1;
	}

	for (i = 0; i < ARRAY_SIZE(eem_entries); i++) {
		if (!proc_create(eem_entries[i].name, S_IRUGO | S_IWUSR | S_IWGRP, eem_dir, eem_entries[i].fops)) {
			eem_debug("[%s]: create /proc/eem/%s failed\n", __func__, eem_entries[i].name);
			FUNC_EXIT(FUNC_LV_HELP);
			return -3;
		}
	}

	for_each_det(det) {
		det_dir = proc_mkdir(det->name, eem_dir);

		if (!det_dir) {
			eem_debug("[%s]: mkdir /proc/eem/%s failed\n", __func__, det->name);
			FUNC_EXIT(FUNC_LV_HELP);
			return -2;
		}

		for (i = 0; i < ARRAY_SIZE(det_entries); i++) {
			if (!proc_create_data(det_entries[i].name,
				S_IRUGO | S_IWUSR | S_IWGRP,
				det_dir, det_entries[i].fops, det)) {
				eem_debug("[%s]: create /proc/eem/%s/%s failed\n", __func__,
				det->name, det_entries[i].name);
				FUNC_EXIT(FUNC_LV_HELP);
				return -3;
			}
		}
	}

	FUNC_EXIT(FUNC_LV_HELP);
	return 0;
}

int get_ptpod_status(void)
{
	get_devinfo(&eem_devinfo);

	return eem_devinfo.EEMINITEN;

}
EXPORT_SYMBOL(get_ptpod_status);
#endif

#if 0
#define VLTE_BIN0_VMAX (121000)
#define VLTE_BIN0_VMIN (104500)
#define VLTE_BIN0_VNRL (110625)

#define VLTE_BIN1_VMAX (121000)
#define VLTE_BIN1_VMIN (102500)
#define VLTE_BIN1_VNRL (105000)

#define VLTE_BIN2_VMAX (121000)
#define VLTE_BIN2_VMIN (97500)
#define VLTE_BIN2_VNRL (100000)

#define VSOC_BIN0_VMAX (126500)
#define VSOC_BIN0_VMIN (109000)
#define VSOC_BIN0_VNRL (115000)

#define VSOC_BIN1_VMAX (126500)
#define VSOC_BIN1_VMIN (107500)
#define VSOC_BIN1_VNRL (110000)

#define VSOC_BIN2_VMAX (126500)
#define VSOC_BIN2_VMIN (102500)
#define VSOC_BIN2_VNRL (105000)

#if defined(SLT_VMAX)
#define VLTE_BIN(x) VLTE_BIN##x##_VMAX
#define VSOC_BIN(x) VSOC_BIN##x##_VMAX
#elif defined(SLT_VMIN)
#define VLTE_BIN(x) VLTE_BIN##x##_VMIN
#define VSOC_BIN(x) VSOC_BIN##x##_VMIN
#else
#define VLTE_BIN(x) VLTE_BIN##x##_VNRL
#define VSOC_BIN(x) VSOC_BIN##x##_VNRL
#endif

void process_voltage_bin(struct eem_devinfo *devinfo)
{
	switch (devinfo->LTE_VOLTBIN) {
	case 0:
		#ifndef EARLY_PORTING
			#ifdef __KERNEL__
				/* det->volt_tbl_bin[0] = EEM_VOLT_TO_PMIC_VAL(VLTE_BIN(0)) + EEM_PMIC_OFFSET; */
				mt_cpufreq_set_lte_volt(EEM_VOLT_TO_PMIC_VAL(VLTE_BIN(0)) + EEM_PMIC_OFFSET);
			#else
				dvfs_set_vlte(EEM_VOLT_TO_PMIC_VAL(VLTE_BIN(0)) + EEM_PMIC_OFFSET);
			#endif
		#endif
		eem_debug("VLTE voltage bin to %dmV\n", (VLTE_BIN(0)/100));
		break;
	case 1:
		#ifndef EARLY_PORTING
			#ifdef __KERNEL__
				/* det->volt_tbl_bin[0] = EEM_VOLT_TO_PMIC_VAL(105000) + EEM_PMIC_OFFSET; */
				mt_cpufreq_set_lte_volt(EEM_VOLT_TO_PMIC_VAL(105000) + EEM_PMIC_OFFSET);
			#else
				dvfs_set_vlte(EEM_VOLT_TO_PMIC_VAL(VLTE_BIN(1)) + EEM_PMIC_OFFSET);
			#endif
		#endif
		eem_debug("VLTE voltage bin to %dmV\n", (VLTE_BIN(1)/100));
		break;
	case 2:
		#ifndef EARLY_PORTING
			#ifdef __KERNEL__
				/* det->volt_tbl_bin[0] = EEM_VOLT_TO_PMIC_VAL(100000) + EEM_PMIC_OFFSET; */
				mt_cpufreq_set_lte_volt(EEM_VOLT_TO_PMIC_VAL(100000) + EEM_PMIC_OFFSET);
			#else
				dvfs_set_vlte(EEM_VOLT_TO_PMIC_VAL(VLTE_BIN(2)) + EEM_PMIC_OFFSET);
			#endif
		#endif
		eem_debug("VLTE voltage bin to %dmV\n", (VLTE_BIN(2)/100));
		break;
	default:
		#ifndef EARLY_PORTING
			#ifdef __KERNEL__
				mt_cpufreq_set_lte_volt(EEM_VOLT_TO_PMIC_VAL(110625) + EEM_PMIC_OFFSET);
			#else
				dvfs_set_vlte(EEM_VOLT_TO_PMIC_VAL(110625) + EEM_PMIC_OFFSET);
			#endif
		#endif
		eem_debug("VLTE voltage bin to 1.10625V\n");
		break;
	};
	/* mt_cpufreq_set_lte_volt(det->volt_tbl_bin[0]); */

	switch (devinfo->SOC_VOLTBIN) {
	case 0:
#ifdef __MTK_SLT_
		if (!slt_is_low_vcore()) {
			dvfs_set_vcore(EEM_VOLT_TO_PMIC_VAL(VSOC_BIN(0)) + EEM_PMIC_OFFSET);
			eem_debug("VCORE voltage bin to %dmV\n", (VSOC_BIN(0)/100));
		} else {
			dvfs_set_vcore(EEM_VOLT_TO_PMIC_VAL(105000) + EEM_PMIC_OFFSET);
			eem_debug("VCORE voltage bin to %dmV\n", (105000/100));
		}
#else
		dvfs_set_vcore(EEM_VOLT_TO_PMIC_VAL(VSOC_BIN(0)) + EEM_PMIC_OFFSET);
		eem_debug("VCORE voltage bin to %dmV\n", (VSOC_BIN(0)/100));
#endif
		break;
	case 1:
		dvfs_set_vcore(EEM_VOLT_TO_PMIC_VAL(VSOC_BIN(1)) + EEM_PMIC_OFFSET);
		eem_debug("VCORE voltage bin to %dmV\n", (VSOC_BIN(1)/100));
		break;
	case 2:
		dvfs_set_vcore(EEM_VOLT_TO_PMIC_VAL(VSOC_BIN(2)) + EEM_PMIC_OFFSET);
		eem_debug("VCORE voltage bin to %dmV\n", (VSOC_BIN(2)/100));
		break;
	default:
		dvfs_set_vcore(EEM_VOLT_TO_PMIC_VAL(105000) + EEM_PMIC_OFFSET);
		eem_debug("VCORE voltage bin to 1.05V\n");
		break;
	};
}
#endif

#define VCORE_VOLT_0 1000000
#define VCORE_VOLT_1 900000
#define VCORE_VOLT_2 900000

unsigned int vcore0;
unsigned int vcore1;
unsigned int vcore2;
unsigned int have_550;

int is_have_550(void)
{
	return have_550;
}

unsigned int get_vcore_ptp_volt(int uv)
{
	unsigned int ret;

	switch (uv) {
	case VCORE_VOLT_0:
		/* ret = vcore0; */
		ret = EEM_VOLT_TO_PMIC_VAL(VCORE_VOLT_0 / 10) + EEM_PMIC_OFFSET;
		break;

	case VCORE_VOLT_1:
		/* ret = vcore1; */
		ret = EEM_VOLT_TO_PMIC_VAL(VCORE_VOLT_1 / 10) + EEM_PMIC_OFFSET;
		break;

	/* Jade only use 2 level voltage
	case VCORE_VOLT_2:
		ret = vcore2;
		break;
	*/
	default:
		ret = EEM_VOLT_TO_PMIC_VAL(uv / 10) + EEM_PMIC_OFFSET;
		break;
	}

	if (ret == 0)
		ret = EEM_VOLT_TO_PMIC_VAL(uv / 10) + EEM_PMIC_OFFSET;

	return ret;
}

void eem_set_pi_offset(enum eem_ctrl_id id, int step)
{
	struct eem_det *det = id_to_eem_det(id);

	if (det == NULL)
		return;

	det->pi_offset = step;

#ifdef CONFIG_EEM_AEE_RR_REC
	aee_rr_rec_eem_pi_offset(step);
#endif
}

#ifdef __KERNEL__
static int __init dt_get_ptp_devinfo(unsigned long node, const char *uname, int depth, void *data)
{
	struct devinfo_ptp_tag *tags;
	unsigned int size = 0;

	if (depth != 1 || (strcmp(uname, "chosen") != 0 && strcmp(uname, "chosen@0") != 0))
		return 0;

	tags = (struct devinfo_ptp_tag *) of_get_flat_dt_prop(node, "atag,ptp", &size);

	if (tags) {
		vcore0 = tags->volt0;
		vcore1 = tags->volt1;
		vcore2 = tags->volt2;
		have_550 = tags->have_550;
		eem_debug("[PTP][VCORE] - Kernel Got from DT (0x%0X, 0x%0X, 0x%0X, 0x%0X)\n",
			vcore0, vcore1, vcore2, have_550);
	}
	return 1;
}

static int __init vcore_ptp_init(void)
{
	of_scan_flat_dt(dt_get_ptp_devinfo, NULL);

	return 0;
}

/*
 * Module driver
 */

static int __init eem_conf(void)
{
	int i;
	unsigned int binLevel;
	#if !defined(CONFIG_MTK_PMIC_CHIP_MT6353)
	unsigned int freq_bound, binLevel_eng;
	#endif
	struct device_node *cpuSpeedNode = NULL;

	recordRef = ioremap_nocache(EEMCONF_S, EEMCONF_SIZE);
	eem_debug("@(Record)%s----->(%p)\n", __func__, recordRef);
	memset_io((u8 *)recordRef, 0x00, EEMCONF_SIZE);
	if (!recordRef)
		return -ENOMEM;

	cpuSpeedNode = of_find_node_by_type(NULL, "cpu");
	cpu_speed = 0;

	if (!of_property_read_u32(cpuSpeedNode, "clock-frequency", &cpu_speed))
		cpu_speed = cpu_speed / 1000 / 1000; /* MHz */
	else {
		eem_error("missing clock-frequency property, use EFUSE to get FY/SB\n");
		cpu_speed = 0;
	}
	eem_error("0--->The cpu_speed = %d\n", cpu_speed);

	/* read E-fuse for segment selection */
	binLevel = GET_BITS_VAL(7:0, get_devinfo_with_index(21));
	#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
	if (cpu_eem_is_extbuck_valid()) {
		if ((binLevel == 0x82) || (binLevel == 0x86))
			binLevel = 2;
		else
			binLevel = 1;
	} else
		binLevel = 1;
	if (0) {
	#else
	binLevel_eng = GET_BITS_VAL(15:0, get_devinfo_with_index(19));
	freq_bound = GET_BITS_VAL(25:23, get_devinfo_with_index(4));
	if (1001 == cpu_speed) {
	#endif
		#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
		if (0) {
		#else
		if (freq_bound == 5) {
			recordTbl = &fyTbl_M[0][0];
			eem_debug("1--->The table ----->(fyTbl_M), cpu_speed = %d\n", cpu_speed);
		#endif
		} else {
			recordTbl = &fyTbl[0][0];
			eem_debug("1--->The table ----->(fyTbl), cpu_speed = %d\n", cpu_speed);
		}
	} else {
		#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
		if (1 == binLevel) {
		#else
		if ((1 == binLevel) || (3 == binLevel)) {
		#endif
			#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
			if (0) {
			#else
			if (freq_bound == 5) {
				recordTbl = &fyTbl_M[0][0];
				eem_error("@The table ----->(fyTbl_M)\n");
			#endif
			} else {
				recordTbl = &fyTbl[0][0];
				eem_error("@The table ----->(fyTbl)\n");
			}
		#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
		} else if (2 == binLevel) {
		#else
		} else if ((2 == binLevel) || (4 == binLevel)) {
		#endif
			recordTbl = &sbTbl[0][0];
			eem_error("@The table ----->(sbTbl)\n");
		#ifdef CONFIG_ARCH_MT6755_TURBO
		} else if (0x20 == (binLevel & 0xF0)) {
			recordTbl = &p15Tbl[0][0];
			eem_error("@The table ----->(p15Tbl)\n");
		#endif
		#if !defined(CONFIG_MTK_PMIC_CHIP_MT6353)
		} else {
			if ((2 == ((binLevel_eng >> 4) & 0x07)) || (2 == ((binLevel_eng >> 10) & 0x07))) {
				if (freq_bound == 5) {
					recordTbl = &fyTbl_M[0][0];
					eem_error("@The table ----->(ENG fyTbl_M)\n");
				} else {
					recordTbl = &fyTbl[0][0];
					eem_error("@The table ----->(ENG fyTbl)\n");
				}
			} else {
				recordTbl = &sbTbl[0][0];
				eem_error("@The table ----->(ENG sbTbl)\n");
			}
		}
		#else
		}
		#endif
	}

	/* [13:7] = Vsram pmic value, [6:0] = Vproc pmic value */
	for (i = 0; i < NR_FREQ; i++) {
		/* LL */
		recordRef[i * 2] =
			((*(recordTbl + (i * 9) + 6) & 0xFFF)<<16) |
			((*(recordTbl + (i * 9) + 7) & 0x7F) << 7) |
			(*(recordTbl + (i * 9) + 8) & 0x7F);
		recordRef[i * 2 + 1] =
					((*(recordTbl + (i * 9) + 0) & 0x1F) << 27) |
					((*(recordTbl + (i * 9) + 1) & 0x1F) << 22) |
					((*(recordTbl + (i * 9) + 2) & 0x1F) << 17) |
					((*(recordTbl + (i * 9) + 3) & 0x1F) << 12) |
					((*(recordTbl + (i * 9) + 4) & 0x07) << 9) |
					(*(recordTbl + (i * 9) + 5) & 0x1FF);

		/* L */
		recordRef[i * 2 + 36] =
					((*(recordTbl + ((i + 8) * 9) + 6) & 0xFFF)<<16) |
					((*(recordTbl + ((i + 8) * 9) + 7) & 0x7F) << 7) |
					(*(recordTbl + ((i + 8) * 9) + 8) & 0x7F);
		recordRef[i * 2 + 36 + 1] =
					((*(recordTbl + ((i + 8) * 9) + 0) & 0x1F) << 27) |
					((*(recordTbl + ((i + 8) * 9) + 1) & 0x1F) << 22) |
					((*(recordTbl + ((i + 8) * 9) + 2) & 0x1F) << 17) |
					((*(recordTbl + ((i + 8) * 9) + 3) & 0x1F) << 12) |
					((*(recordTbl + ((i + 8) * 9) + 4) & 0x07) << 9) |
					(*(recordTbl + ((i + 8) * 9) + 5) & 0x1FF);
	}
	recordRef[i*2] = 0xffffffff;
	recordRef[i*2+36] = 0xffffffff;
	mb(); /* SRAM writing */
	for (i = 0; i < NR_FREQ; i++) {
		eem_debug("LL (SSC, 0x%x) (Vs, 0x%x) (Vp, 0x%x)\n",
			((*(recordRef + (i * 2))) >> 16) & 0xFFF,
			((*(recordRef + (i * 2))) >> 7) & 0x7F,
			(*(recordRef + (i * 2))) & 0x7F
			);

		eem_debug("LL(F_Setting)(%x, %x, %x, %x, %x, %x)\n",
			((*(recordRef + (i * 2) + 1)) >> 27) & 0x1F,
			((*(recordRef + (i * 2) + 1)) >> 22) & 0x1F,
			((*(recordRef + (i * 2) + 1)) >> 17) & 0x1F,
			((*(recordRef + (i * 2) + 1)) >> 12) & 0x1F,
			((*(recordRef + (i * 2) + 1)) >> 9) & 0x07,
			((*(recordRef + (i * 2) + 1)) & 0x1ff)
			);

		eem_debug("L (SSC, 0x%x) (Vs, 0x%x) (Vp, 0x%x),\n",
			((*(recordRef + 36 + (i * 2))) >> 16) & 0xFFF,
			((*(recordRef + 36 + (i * 2))) >> 7) & 0x7F,
			(*(recordRef + 36 + (i * 2))) & 0x7F
			);

		eem_debug("L(F_Setting)(%x, %x, %x, %x, %x, %x)\n",
			((*(recordRef + 36 + (i * 2) + 1)) >> 27) & 0x1F,
			((*(recordRef + 36 + (i * 2) + 1)) >> 22) & 0x1F,
			((*(recordRef + 36 + (i * 2) + 1)) >> 17) & 0x1F,
			((*(recordRef + 36 + (i * 2) + 1)) >> 12) & 0x1F,
			((*(recordRef + 36 + (i * 2) + 1)) >> 9) & 0x07,
			((*(recordRef + 36 + (i * 2) + 1)) & 0x1ff)
			);
	}
	return 0;
}

static int new_eem_val = 1; /* default no change */
static int  __init fn_change_eem_status(char *str)
{
	int new_set_val;

	eem_debug("fn_change_eem_status\n");
	if (get_option(&str, &new_set_val)) {
		new_eem_val = new_set_val;
		eem_debug("new_eem_val = %d\n", new_eem_val);
		return 0;
	}
	return -EINVAL;
}
early_param("eemen", fn_change_eem_status);

static int __init eem_init(void)
#else
int __init eem_init(void)
#endif
{
	int err = 0;
#ifdef __KERNEL__
	struct device_node *node = NULL;
#endif
	FUNC_ENTER(FUNC_LV_MODULE);

#ifdef __KERNEL__
	node = of_find_compatible_node(NULL, NULL, "mediatek,mt6755-ptp_fsm");

	if (node) {
		/* Setup IO addresses */
		eem_base = of_iomap(node, 0);
		eem_debug("[EEM] eem_base = 0x%p\n", eem_base);
	}

	/*get eem irq num*/
	eem_irq_number = irq_of_parse_and_map(node, 0);
	eem_debug("[THERM_CTRL] eem_irq_number=%d\n", eem_irq_number);
	if (!eem_irq_number) {
		eem_debug("[EEM] get irqnr failed=0x%x\n", eem_irq_number);
		return 0;
	}

	#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
	/* apmixed */
	node = of_find_compatible_node(NULL, NULL, "mediatek,apmixed");
	if (node) {
		eem_apmixed_base = of_iomap(node, 0);
		eem_debug("[EEM] eem_apmixed_base = 0x%p\n", eem_apmixed_base);
	}
	#endif

#endif
	get_devinfo(&eem_devinfo);
#ifdef __KERNEL__
	if (new_eem_val == 0) {
		eem_devinfo.EEMINITEN = 0;
		eem_debug("Disable EEM by kernel config\n");
	}
#endif
	/* process_voltage_bin(&eem_devinfo); */ /* LTE voltage bin use I-Chang */
	/* eem_devinfo.EEMINITEN = 0;    DISABLE_EEM!!! */
	if (0 == eem_devinfo.EEMINITEN) {
		eem_error("EEMINITEN = 0x%08X\n", eem_devinfo.EEMINITEN);
		FUNC_EXIT(FUNC_LV_MODULE);
		return 0;
	}

#ifdef __KERNEL__
	/*
	 * init timer for log / volt
	 */
	hrtimer_init(&eem_log_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	eem_log_timer.function = eem_log_timer_func;

	create_procfs();
#endif

	/*
	 * reg platform device driver
	 */
	err = platform_driver_register(&eem_driver);

	if (err) {
		eem_debug("EEM driver callback register failed..\n");
		FUNC_EXIT(FUNC_LV_MODULE);
		return err;
	}

	FUNC_EXIT(FUNC_LV_MODULE);

	return 0;
}

static void __exit eem_exit(void)
{
	FUNC_ENTER(FUNC_LV_MODULE);
	eem_debug("EEM de-initialization\n");
	FUNC_EXIT(FUNC_LV_MODULE);
}

#ifdef __KERNEL__
device_initcall_sync(eem_conf);
arch_initcall(vcore_ptp_init); /* I-Chang */
late_initcall(eem_init);
#endif
#endif

MODULE_DESCRIPTION("MediaTek EEM Driver v0.3");
MODULE_LICENSE("GPL");
#ifdef EARLY_PORTING
	#undef EARLY_PORTING
#endif
#undef __MT_EEM_C__

