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

/* L */
unsigned int llFreq_FY[16] = {
	1638000, 1560000, 1508000, 1443000, 1378000, 1300000, 1235000, 1183000,
	1066000, 949000, 806000, 676000, 559000, 455000, 377000, 221000};
unsigned int littleFreq_FY[16] = {
	2340000, 2262000, 2197000, 2132000, 2067000, 1963000, 1872000, 1794000,
	1638000, 1456000, 1248000, 1040000, 897000, 754000, 533000, 442000};
unsigned int cciFreq_FY[16] = {
	988000, 949000, 910000, 858000, 819000, 767000, 741000, 702000,
	650000, 572000, 494000, 416000, 351000, 286000, 234000, 195000};
#if 0
unsigned int llFreq_SB[16] = {
	1833000, 1716000, 1612000, 1508000, 1404000, 1300000, 1235000, 1183000,
	1066000, 949000, 806000, 676000, 559000, 455000, 377000, 221000};

unsigned int littleFreq_SB[16] = {
	2574000, 2444000, 2327000, 2210000, 2093000, 1963000, 1872000, 1794000,
	1638000, 1456000, 1248000, 1040000, 897000, 754000, 533000, 442000};

unsigned int cciFreq_SB[16] = {
	1092000, 1027000, 962000, 897000, 832000, 767000, 741000, 702000,
	650000, 572000, 494000, 416000, 351000, 286000, 234000, 195000};
#endif
#if 0
unsigned int gpuFreq[16] = {
	850000, 850000, 785000, 785000, 720000, 720000, 600000, 600000,
	485000, 485000, 390000, 390000, 310000, 310000, 230000, 230000};
#endif

#define NUM_ELM_SRAM 8
/*
[26:22] dcmdiv
[21:12] DDS
[11:7] clkdiv; ARM PLL Divider
[6:4] postdiv
[3:0] CF index

[31:14] Reserve %(/2)
[13:7] Vsram pmic value
[6:0] Vproc pmic value
*/

unsigned int fyTbl[][NUM_ELM_SRAM] = {
	/* dcmdiv, DDS, clk_div, post_div, CF_index, Reserve, Vsram, Vproc */
	{0x19, 0x0FC, 0x8, 0x0, 0xF, 0x0, 0x48, 0x44},/* 1638 (LL) */
	{0x18, 0x0F0, 0x8, 0x0, 0xE, 0x0, 0x48, 0x3F},/* 1560 */
	{0x17, 0x0E8, 0x8, 0x0, 0xD, 0x0, 0x48, 0x3B},/* 1508 */
	{0x16, 0x1BC, 0x8, 0x1, 0xC, 0x0, 0x47, 0x37},/* 1443 */
	{0x15, 0x1A8, 0x8, 0x1, 0xB, 0x0, 0x43, 0x33},/* 1378 */
	{0x14, 0x190, 0x8, 0x1, 0xA, 0x0, 0x3E, 0x2E},/* 1300 */
	{0x13, 0x17C, 0x8, 0x1, 0x9, 0x0, 0x3B, 0x2B},/* 1235 */
	{0x12, 0x16C, 0x8, 0x1, 0x8, 0x0, 0x38, 0x28},/* 1183 */
	{0x10, 0x148, 0x8, 0x1, 0x7, 0x0, 0x32, 0x22},/* 1066 */
	{0x0E, 0x124, 0x8, 0x1, 0x6, 0x0, 0x2C, 0x1C},/* 949  */
	{0x0C, 0x0F8, 0x8, 0x1, 0x5, 0x0, 0x28, 0x16},/* 806  */
	{0x0A, 0x1A0, 0xA, 0x1, 0x4, 0x0, 0x28, 0x10},/* 676  */
	{0x08, 0x158, 0xA, 0x1, 0x3, 0x0, 0x28, 0x0C},/* 559  */
	{0x06, 0x110, 0xA, 0x1, 0x2, 0x0, 0x28, 0x08},/* 442  */
	{0x05, 0x0E8, 0xA, 0x1, 0x1, 0x0, 0x28, 0x04},/* 377  */
	{0x03, 0x110, 0xB, 0x1, 0x0, 0x0, 0x28, 0x00},/* 221  */

	{0x1F, 0x168, 0x8, 0x0, 0xF, 0x0, 0x48, 0x44},/* 2340 (L) */
	{0x1F, 0x15C, 0x8, 0x0, 0xF, 0x0, 0x48, 0x3F},/* 2262 */
	{0x1F, 0x152, 0x8, 0x0, 0xE, 0x0, 0x48, 0x3B},/* 2197 */
	{0x1F, 0x148, 0x8, 0x0, 0xD, 0x0, 0x47, 0x37},/* 2132 */
	{0x1F, 0x13E, 0x8, 0x0, 0xC, 0x0, 0x43, 0x33},/* 2067 */
	{0x1E, 0x12E, 0x8, 0x0, 0xB, 0x0, 0x3E, 0x2E},/* 1963 */
	{0x1C, 0x120, 0x8, 0x0, 0xA, 0x0, 0x3B, 0x2B},/* 1872 */
	{0x1B, 0x114, 0x8, 0x0, 0x9, 0x0, 0x38, 0x28},/* 1794 */
	{0x19, 0x0FC, 0x8, 0x0, 0x8, 0x0, 0x32, 0x22},/* 1638 */
	{0x16, 0x1C0, 0x8, 0x1, 0x7, 0x0, 0x2C, 0x1C},/* 1456 */
	{0x13, 0x180, 0x8, 0x1, 0x6, 0x0, 0x28, 0x16},/* 1248 */
	{0x10, 0x140, 0x8, 0x1, 0x5, 0x0, 0x28, 0x10},/* 1040 */
	{0x0D, 0x114, 0x8, 0x1, 0x4, 0x0, 0x28, 0x0C},/* 897  */
	{0x0B, 0x0E8, 0x8, 0x1, 0x3, 0x0, 0x28, 0x08},/* 754  */
	{0x08, 0x148, 0xa, 0x1, 0x2, 0x0, 0x28, 0x04},/* 533  */
	{0x06, 0x110, 0xa, 0x1, 0x1, 0x0, 0x28, 0x00},/* 442  */

	{0x0F, 0x130, 0x8, 0x1, 0x0, 0x0, 0x48, 0x44},/* 988 (CCI) */
	{0x0E, 0x124, 0x8, 0x1, 0x0, 0x0, 0x48, 0x3F},/* 949 */
	{0x0E, 0x118, 0x8, 0x1, 0x0, 0x0, 0x48, 0x3B},/* 910 */
	{0x0D, 0x108, 0x8, 0x1, 0x0, 0x0, 0x47, 0x37},/* 858 */
	{0x0C, 0x0FC, 0x8, 0x1, 0x0, 0x0, 0x43, 0x33},/* 819 */
	{0x0B, 0x0EC, 0x8, 0x1, 0x0, 0x0, 0x3E, 0x2E},/* 767 */
	{0x0B, 0x1C8, 0xA, 0x1, 0x0, 0x0, 0x3B, 0x2B},/* 741 */
	{0x0A, 0x1B0, 0xA, 0x1, 0x0, 0x0, 0x38, 0x28},/* 702 */
	{0x0A, 0x190, 0xA, 0x1, 0x0, 0x0, 0x32, 0x22},/* 650 */
	{0x08, 0x160, 0xA, 0x1, 0x0, 0x0, 0x2C, 0x1C},/* 572 */
	{0x07, 0x130, 0xA, 0x1, 0x0, 0x0, 0x28, 0x16},/* 494 */
	{0x06, 0x100, 0xA, 0x1, 0x0, 0x0, 0x28, 0x10},/* 416 */
	{0x05, 0x1B0, 0xB, 0x1, 0x0, 0x0, 0x28, 0x0C},/* 351 */
	{0x04, 0x160, 0xB, 0x1, 0x0, 0x0, 0x28, 0x08},/* 286 */
	{0x03, 0x120, 0xB, 0x1, 0x0, 0x0, 0x28, 0x04},/* 234 */
	{0x03, 0x0F0, 0xB, 0x1, 0x0, 0x0, 0x28, 0x00},/* 195 */
};
#if 0
unsigned int sbTbl[][NUM_ELM_SRAM] = {
	{0x1C, 0x11A, 0x8, 0x0, 0xF, 0x0, 0x48, 0x44},/* 1833 (LL)*/
	{0x1A, 0x108, 0x8, 0x0, 0xE, 0x0, 0x48, 0x3F},/* 1716 */
	{0x18, 0x0F8, 0x8, 0x0, 0xD, 0x0, 0x48, 0x3B},/* 1612 */
	{0x17, 0x0E8, 0x8, 0x0, 0xC, 0x0, 0x47, 0x37},/* 1508 */
	{0x15, 0x1B0, 0x8, 0x1, 0xB, 0x0, 0x43, 0x33},/* 1404 */
	{0x14, 0x190, 0x8, 0x1, 0xA, 0x0, 0x3E, 0x2E},/* 1300 */
	{0x13, 0x17C, 0x8, 0x1, 0x9, 0x0, 0x3B, 0x2B},/* 1235 */
	{0x12, 0x16C, 0x8, 0x1, 0x8, 0x0, 0x38, 0x28},/* 1183 */
	{0x10, 0x148, 0x8, 0x1, 0x7, 0x0, 0x32, 0x22},/* 1066 */
	{0x0E, 0x124, 0x8, 0x1, 0x6, 0x0, 0x2C, 0x1C},/* 949  */
	{0x0C, 0x0F8, 0x8, 0x1, 0x5, 0x0, 0x26, 0x16},/* 806  */
	{0x0A, 0x1A0, 0xA, 0x1, 0x4, 0x0, 0x20, 0x10},/* 676  */
	{0x08, 0x158, 0xA, 0x1, 0x3, 0x0, 0x20, 0x0C},/* 559  */
	{0x06, 0x110, 0xA, 0x1, 0x2, 0x0, 0x20, 0x08},/* 442  */
	{0x05, 0x0E8, 0xA, 0x1, 0x1, 0x0, 0x20, 0x04},/* 377  */
	{0x03, 0x110, 0xB, 0x1, 0x0, 0x0, 0x20, 0x00},/* 221  */

	{0x1F, 0x18C, 0x8, 0x0, 0xF, 0x0, 0x48, 0x44},/* 2574 (L) */
	{0x1F, 0x178, 0x8, 0x0, 0xF, 0x0, 0x48, 0x3F},/* 2444 */
	{0x1F, 0x166, 0x8, 0x0, 0xE, 0x0, 0x48, 0x3B},/* 2327 */
	{0x1F, 0x154, 0x8, 0x0, 0xD, 0x0, 0x47, 0x37},/* 2210 */
	{0x1F, 0x142, 0x8, 0x0, 0xC, 0x0, 0x43, 0x33},/* 2093 */
	{0x1E, 0x12E, 0x8, 0x0, 0xB, 0x0, 0x3E, 0x2E},/* 1963 */
	{0x1C, 0x120, 0x8, 0x0, 0xA, 0x0, 0x3B, 0x2B},/* 1872 */
	{0x1B, 0x114, 0x8, 0x0, 0x9, 0x0, 0x38, 0x28},/* 1794 */
	{0x19, 0x0FC, 0x8, 0x0, 0x8, 0x0, 0x32, 0x22},/* 1638 */
	{0x16, 0x1C0, 0x8, 0x1, 0x7, 0x0, 0x2C, 0x1C},/* 1456 */
	{0x13, 0x180, 0x8, 0x1, 0x6, 0x0, 0x26, 0x16},/* 1248 */
	{0x10, 0x140, 0x8, 0x1, 0x5, 0x0, 0x20, 0x10},/* 1040 */
	{0x0D, 0x114, 0x8, 0x1, 0x4, 0x0, 0x20, 0x0C},/* 897  */
	{0x0B, 0x0E8, 0x8, 0x1, 0x3, 0x0, 0x20, 0x08},/* 754  */
	{0x08, 0x148, 0xA, 0x1, 0x2, 0x0, 0x20, 0x04},/* 533  */
	{0x06, 0x110, 0xA, 0x1, 0x1, 0x0, 0x20, 0x00},/* 442  */

	{0x10, 0x150, 0x8, 0x1, 0x0, 0x0, 0x48,	0x44},/* 1092 (CCI) */
	{0x0F, 0x13C, 0x8, 0x1, 0x0, 0x0, 0x48,	0x3F},/* 1027 */
	{0x0E, 0x128, 0x8, 0x1, 0x0, 0x0, 0x48,	0x3B},/* 962  */
	{0x0D, 0x114, 0x8, 0x1, 0x0, 0x0, 0x47,	0x37},/* 897  */
	{0x0C, 0x100, 0x8, 0x1, 0x0, 0x0, 0x43, 0x33},/* 832  */
	{0x0B, 0x0EC, 0x8, 0x1, 0x0, 0x0, 0x3E, 0x2E},/* 767  */
	{0x0B, 0x1C8, 0xA, 0x1, 0x0, 0x0, 0x3B, 0x2B},/* 741  */
	{0x0A, 0x1B0, 0xA, 0x1, 0x0, 0x0, 0x38, 0x28},/* 702  */
	{0x0A, 0x190, 0xA, 0x1, 0x0, 0x0, 0x32, 0x22},/* 650  */
	{0x08, 0x160, 0xA, 0x1, 0x0, 0x0, 0x2C, 0x1C},/* 572  */
	{0x07, 0x130, 0xA, 0x1, 0x0, 0x0, 0x26, 0x16},/* 494  */
	{0x06, 0x100, 0xA, 0x1, 0x0, 0x0, 0x20, 0x10},/* 416  */
	{0x05, 0x1B0, 0xB, 0x1, 0x0, 0x0, 0x20, 0x0C},/* 351  */
	{0x04, 0x160, 0xB, 0x1, 0x0, 0x0, 0x20,	0x08},/* 286  */
	{0x03, 0x120, 0xB, 0x1, 0x0, 0x0, 0x20,	0x04},/* 234  */
	{0x03, 0x0F0, 0xB, 0x1, 0x0, 0x0, 0x20,	0x00},/* 195  */
};
#endif

unsigned int gpuOutput[8];


/* Early porting use that avoid to cause compiler error */
/* #define DISABLE_EEM */
/* #define EARLY_PORTING */
/* #define EARLY_PORTING_NO_PRI_TBL */



#define PTP_SLT_EARLY_PORTING
/* #define EARLY_PORTING_FAKE_FUNC */

#if defined(__MTK_SLT_) && defined(PTP_SLT_EARLY_PORTING)
unsigned int gpuFreq[16] = {
	900000, 900000, 785000, 785000, 720000, 720000, 600000, 600000,
	485000, 485000, 390000, 390000, 310000, 310000, 230000, 230000};
#endif


/* #ifndef EARLY_PORTING_NO_PRI_TBL */
static unsigned int record_tbl_locked[16];
static unsigned int *recordTbl;
/* #endif */
static unsigned int cpu_speed;
static int eem_log_en;

/**
 * @file    mt_ptp.c
 * @brief   Driver for EEM
 *
 */

#define __MT_EEM_C__


/*=============================================================
 * Include files
 *=============================================================*/

/* system includes */
#include <linux/init.h>
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
#include <linux/types.h>
#ifdef __KERNEL__
#include <linux/topology.h>
#endif

/* project includes */
/* #include "mach/mt_reg_base.h" */


#include "mach/mt_freqhopping.h"
#ifndef PTP_SLT_EARLY_PORTING
/* #include "x_define_irq.h" */
/* #include "mach/mtk_rtc_hal.h" */
/* #include "mach/mt_rtc_hw.h" */
#endif

/* #include "mach/mt_irq.h" */
/* #include "mach/mt_spm_idle.h" */
/* #include "mach/mt_pmic_wrap.h" */
/* #include "mach/mt_clkmgr.h" */

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
/* #include "mach/mt_defptp.h" */

#ifdef __KERNEL__
	#include <mt-plat/mt_chip.h>
	#include <mt-plat/mt_gpio.h>
	#include "mt-plat/upmu_common.h"
	#include "../../../include/mt-plat/mt6757/include/mach/mt_thermal.h"
	#include "mach/mt_ppm_api.h"
	#include "mt_gpufreq.h"
	#include "mt_cpufreq.h"
	#include "mt_eem.h"
	#include "mt_defptp.h"
#ifndef EARLY_PORTING
	#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
	#else
		#include "../../../power/mt6757/mt6311.h"
	#endif
#endif
#include "mt_eem_turbo.h"

#else
#include <cache_api.h>
#include "api.h"
#include "sizes.h"
#include "memmap.h"
#include "mmu.h"

#include "upmu_common.h"
#include "mt6311.h"
#include "mt_cpufreq.h"
#include "common.h"
#include "mach/mt_ptp.h"
#include "mach/mt_ptpslt.h"
#include "mach/mt_defptp.h"
#include "mach/mt_typedefs.h"
/* #include "kernel2ctp.h" */
#include "ptp.h"
/* #include "mt6311.h" */
#ifndef EARLY_PORTING
	/* #include "gpu_dvfs.h" */
	#include "thermal.h"
	/* #include "gpio_const.h" */
#endif
#endif
#ifdef EEM_ENABLE_VTURBO
#include <linux/cpu.h>
#endif

static unsigned int ctrl_VTurbo;

#if defined(EEM_ENABLE_VTURBO)
#define TURBO_CPU_ID 7
/* #define OD_IVER_TURBO_V -1 */ /* 2700 */
/* static unsigned int turbo_offset; */
static unsigned int *tTbl;
static unsigned int VTurboRun;
unsigned int cpuBinLevel, cpuBinLevel_eng;

#if defined(EEM_ENABLE_TTURBO)
static unsigned int thermalTimerStart, TTurboRuno, ctrl_TTurbo;
static struct hrtimer eem_thermal_turbo_timer;
static int cur_temp, pre_temp;
#endif
#if defined(EEM_ENABLE_ITURBO)
static unsigned int ctrl_ITurb, ITurboRun;
#endif
#endif

static struct eem_devinfo eem_devinfo;
/* static unsigned int eem_level;  debug info */

#ifdef __KERNEL__
/**
 * timer for log
 */
static struct hrtimer eem_log_timer;
#endif

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
#define TML_INTERVAL		(LOG_INTERVAL / 10)
#define NR_FREQ			16


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
#define EEM_VOLT_TO_PMIC_VAL(volt)  (((volt) - 60000 + 625 - 1) / 625) /* ((((volt) - 700 * 100 + 625 - 1) / 625) */

/* pmic value from EEM already add 16 steps(16*6.25=100mV) for real voltage transfer */
#define EEM_PMIC_VAL_TO_VOLT(pmic)  (((pmic) * 625) + 60000) /* (((pmic) * 625) / 100 + 600) */
#else
#define MV_TO_VAL(MV)		((((MV) - 600) * 100 + 625 - 1) / 625) /* TODO: FIXME, refer to VOLT_TO_PMIC_VAL() */
#define VAL_TO_MV(VAL)		(((VAL) * 625) / 100 + 600) /* TODO: FIXME, refer to PMIC_VAL_TO_VOLT() */
#endif
/* No offset for CPU/GPU DVFS, due to eem and pmic has the same base */
#define EEM_PMIC_OFFSET (0x0)


/* I-Chang */
#define VBOOT_CPU		85000 /* 100000[0x40], 92500[0x52], 85000[0x28] */
#define VBOOT_GPU		85000
#define VMAX_VAL		EEM_VOLT_TO_PMIC_VAL(102500)
/* #define VMAX_TURBO_VAL		EEM_VOLT_TO_PMIC_VAL(105000) */ /* This is also turbo voltage */
#define VMIN_VAL		EEM_VOLT_TO_PMIC_VAL(60000)
#define VMAX_SRAM               EEM_VOLT_TO_PMIC_VAL(105000)
#define VMIN_SRAM               EEM_VOLT_TO_PMIC_VAL(85000)
#define VMAX_VAL_GPU            EEM_VOLT_TO_PMIC_VAL(100000)
#define VMIN_VAL_GPU		EEM_VOLT_TO_PMIC_VAL(60000)


#define DTHI_VAL		0x01		/* positive */
#define DTLO_VAL		0xfe		/* negative (2's compliment) */
#define DETMAX_VAL		0xffff		/* This timeout value is in cycles of bclk_ck. */
#define AGECONFIG_VAL		0x555555
#define AGEM_VAL		0x0
#define DVTFIXED_VAL		0x7
#define DVTFIXED_VAL_GPU	0x3
#define DVTFIXED_VAL_SOC	0x4
#define DVTFIXED_VAL_LTE	0x4
#define VCO_VAL			0x0
#define VCO_VAL_GPU		0x0
#define DCCONFIG_VAL	0x555555

#define OVER_INV_TEM	27000
#define DEF_INV_TEM		25000
#define LOW_TEMP_OFT	4


#if defined(CONFIG_EEM_AEE_RR_REC) && !defined(EARLY_PORTING)
static void _mt_eem_aee_init(void)
{
	aee_rr_rec_ptp_vboot(0xFFFFFFFFFFFFFFFF);
	/* aee_rr_rec_ptp_cpu_big_volt(0xFFFFFFFFFFFFFFFF); */
	/* aee_rr_rec_ptp_cpu_big_volt_1(0xFFFFFFFFFFFFFFFF); */
	/* aee_rr_rec_ptp_cpu_big_volt_2(0xFFFFFFFFFFFFFFFF); */
	/* aee_rr_rec_ptp_cpu_big_volt_3(0xFFFFFFFFFFFFFFFF); */
	aee_rr_rec_ptp_gpu_volt(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_gpu_volt_1(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_gpu_volt_2(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_gpu_volt_3(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_cpu_little_volt(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_cpu_little_volt_1(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_cpu_little_volt_2(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_cpu_little_volt_3(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_cpu_2_little_volt(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_cpu_2_little_volt_1(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_cpu_2_little_volt_2(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_cpu_2_little_volt_3(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_cpu_cci_volt(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_cpu_cci_volt_1(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_cpu_cci_volt_2(0xFFFFFFFFFFFFFFFF);
	aee_rr_rec_ptp_cpu_cci_volt_3(0xFFFFFFFFFFFFFFFF);
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
#define EEM_TAG     "[Power/EEM] " /* "[xxxx1] " */
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
#ifdef EARLY_PORTING
	#define eem_debug(fmt, args...)     pr_err(EEM_TAG fmt, ##args)
#else
	#define eem_debug(fmt, args...)     /* pr_debug(EEM_TAG fmt, ##args) */
#endif
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
#if defined(EEM_ENABLE_VTURBO)
void __iomem *eem_apmixed_base;
#define REG_ARMPLL_L_CON1		(eem_apmixed_base + 0x214)
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
	void (*switch_bank)(struct eem_det *det, enum eem_phase phase);

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
	unsigned char isTempInv;
};


struct eem_devinfo {
	/* OD0 0x10206584 */
	unsigned int OD0_DUMMY1:4;
	unsigned int FT_PGM_VER:4;
	unsigned int OD0_PGM_DUMMY2:24;

	/* OD1 0x10206584 */
	unsigned int CPU0_BDES:8;
	unsigned int CPU0_MDES:8;
	unsigned int CPU0_DCBDET:8;
	unsigned int CPU0_DCMDET:8;

	/* OD2 0x10206588 */
	unsigned int CPU0_OTHER:8;
	unsigned int CPU0_LEAKAGE:8;
	unsigned int CPU0_MTDES:8;
	unsigned int CPU0_AGEDELTA:8;

	/* OD3 0x1020658C */
	unsigned int CPU1_BDES:8;
	unsigned int CPU1_MDES:8;
	unsigned int CPU1_DCBDET:8;
	unsigned int CPU1_DCMDET:8;

	/* OD4 0x10206590 */
	unsigned int CPU1_OTHER:8;
	unsigned int CPU1_LEAKAGE:8;
	unsigned int CPU1_MTDES:8;
	unsigned int CPU1_AGEDELTA:8;

	/* OD5 0x10206594 */
	unsigned int CCI_BDES:8;
	unsigned int CCI_MDES:8;
	unsigned int CCI_DCBDET:8;
	unsigned int CCI_DCMDET:8;

	/* OD6 0x10206598 */
	unsigned int CCI_OTHER:8;
	unsigned int CCI_LEAKAGE:8;
	unsigned int CCI_MTDES:8;
	unsigned int CCI_AGEDELTA:8;

	/* OD7 0x1020659C */
	unsigned int GPU_BDES:8;
	unsigned int GPU_MDES:8;
	unsigned int GPU_DCBDET:8;
	unsigned int GPU_DCMDET:8;

	/* OD8 0x102065A0 */
	unsigned int GPU_OTHER:8;
	unsigned int GPU_LEAKAGE:8;
	unsigned int GPU_MTDES:8;
	unsigned int GPU_AGEDELTA:8;


	unsigned int EEMINITEN:8;
	unsigned int EEMMONEN:8;
	unsigned int RESERVED:16;

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
	#if defined(EEM_ENABLE_VTURBO)
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
	[EEM_CTRL_2L] = {
		.name = __stringify(EEM_CTRL_2L),
		.det_id = EEM_DET_2L,
	},

	[EEM_CTRL_L] = {
		.name = __stringify(EEM_CTRL_L),
		.det_id = EEM_DET_L,
	},

	[EEM_CTRL_CCI] = {
		.name = __stringify(EEM_CTRL_CCI),
		.det_id = EEM_DET_CCI,
	},

	[EEM_CTRL_GPU] = {
		.name = __stringify(EEM_CTRL_GPU),
		.det_id = EEM_DET_GPU,
	},

};

unsigned int gpuFy[16] = {
	EEM_VOLT_TO_PMIC_VAL(90000),
	EEM_VOLT_TO_PMIC_VAL(90000),
	EEM_VOLT_TO_PMIC_VAL(85000),
	EEM_VOLT_TO_PMIC_VAL(85000),
	EEM_VOLT_TO_PMIC_VAL(80000),
	EEM_VOLT_TO_PMIC_VAL(80000),
	EEM_VOLT_TO_PMIC_VAL(75000),
	EEM_VOLT_TO_PMIC_VAL(75000),
	EEM_VOLT_TO_PMIC_VAL(70000),
	EEM_VOLT_TO_PMIC_VAL(70000),
	EEM_VOLT_TO_PMIC_VAL(65000),
	EEM_VOLT_TO_PMIC_VAL(65000),
	EEM_VOLT_TO_PMIC_VAL(65000),
	EEM_VOLT_TO_PMIC_VAL(65000),
	EEM_VOLT_TO_PMIC_VAL(60000),
	EEM_VOLT_TO_PMIC_VAL(60000)
};

/*
 * EEM detectors
 */
static void base_ops_enable(struct eem_det *det, int reason);
static void base_ops_disable(struct eem_det *det, int reason);
static void base_ops_disable_locked(struct eem_det *det, int reason);
static void base_ops_switch_bank(struct eem_det *det, enum eem_phase phase);

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

static struct eem_det_ops ll_det_ops = { /* I-Chang */
	.get_volt		= get_volt_cpu,
	.set_volt		= set_volt_cpu,
	.restore_default_volt	= restore_default_volt_cpu,
	.get_freq_table		= get_freq_table_cpu,
};

static struct eem_det_ops little_det_ops = { /* I-Chang */
	.get_volt		= get_volt_cpu,
	.set_volt		= set_volt_cpu, /* <-@@@ */
	.restore_default_volt	= restore_default_volt_cpu,
	.get_freq_table		= get_freq_table_cpu,
};

static struct eem_det_ops cci_det_ops = {
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
	[EEM_DET_2L] = {
		.name		= __stringify(EEM_DET_2L),
		.ops		= &ll_det_ops,
		.ctrl_id	= EEM_CTRL_2L,
		.features	= FEA_INIT01 | FEA_INIT02 | FEA_MON,
		.max_freq_khz	= 1833000,/* 1638 MHz */
		.VBOOT		= EEM_VOLT_TO_PMIC_VAL(VBOOT_CPU), /* 1.0v: 0x30 */
		.volt_offset	= 0,
	},

	[EEM_DET_L] = {
		.name		= __stringify(EEM_DET_L),
		.ops		= &little_det_ops,
		.ctrl_id	= EEM_CTRL_L,
		.features	= FEA_INIT01 | FEA_INIT02 | FEA_MON,
		.max_freq_khz	= 2574000,/* 2340Mhz */
		.VBOOT		= EEM_VOLT_TO_PMIC_VAL(VBOOT_CPU), /* 1.0v: 0x30 */
		.volt_offset	= 0,
	},

	[EEM_DET_CCI] = {
		.name		= __stringify(EEM_DET_CCI),
		.ops		= &cci_det_ops,
		.ctrl_id	= EEM_CTRL_CCI,
		.features	= FEA_INIT01 | FEA_INIT02 | FEA_MON,
		.max_freq_khz	= 1092000,/* 988 MHz */
		.VBOOT		= EEM_VOLT_TO_PMIC_VAL(VBOOT_CPU), /* 1.0v: 0x30 */
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
		.max_freq_khz	= 1000000,/* 900 MHz */
		.VBOOT		= EEM_VOLT_TO_PMIC_VAL(VBOOT_GPU), /* 1.0v: 0x30 */
		.volt_offset	= 0,
	},
};



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

static void base_ops_switch_bank(struct eem_det *det, enum eem_phase phase)
{
	FUNC_ENTER(FUNC_LV_HELP);
	/* Enable/disable system clk SW CG, only enable clk in INIT01 */
	if (phase == EEM_PHASE_INIT01)
		eem_write(EEMCORESEL, ((eem_read(EEMCORESEL) & ~BITMASK(2:0)) | BITS(2:0, det->ctrl_id)) | 0x800f0000);
	else
		eem_write(EEMCORESEL, ((eem_read(EEMCORESEL) & ~BITMASK(2:0)) | BITS(2:0, det->ctrl_id)) & 0x0fffffff);

	/* eem_write_field(EEMCORESEL, 2:0, det->ctrl_id); */
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
	det->ops->switch_bank(det, NR_EEM_PHASE);
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
	#if (!defined(EARLY_PORTING)) && defined(CONFIG_THERMAL)
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

#if (!defined(EARLY_PORTING)) && defined(CONFIG_THERMAL)
	ts_bank = det->ctrl_id;
	get_thermal_slope_intercept(&ts_info, ts_bank);
	det->MTS = ts_info.ts_MTS;
	det->BTS = ts_info.ts_BTS;
	eem_debug("MTS = 0x%x, BTS = 0x%x\n", det->MTS, det->BTS);
#else
	det->MTS =  0x1FB; /* (2048 * TS_SLOPE) + 2048; */
	det->BTS =  0x6D1; /* 4 * TS_INTERCEPT; */
#endif

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
	det->ops->switch_bank(det, NR_EEM_PHASE);
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

	det->ops->switch_bank(det, phase);
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
		  ((det->freq_tbl[3 * (NR_FREQ / 8)] << 24) & 0xff000000)	|
		  ((det->freq_tbl[2 * (NR_FREQ / 8)] << 16) & 0xff0000)	|
		  ((det->freq_tbl[1 * (NR_FREQ / 8)] << 8) & 0xff00)	|
		  (det->freq_tbl[0] & 0xff));
	eem_write(FREQPCT74,
		  ((det->freq_tbl[7 * (NR_FREQ / 8)] << 24) & 0xff000000)	|
		  ((det->freq_tbl[6 * (NR_FREQ / 8)] << 16) & 0xff0000)	|
		  ((det->freq_tbl[5 * (NR_FREQ / 8)] << 8) & 0xff00)	|
		  ((det->freq_tbl[4 * (NR_FREQ / 8)]) & 0xff));
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
			  ((eem_devinfo.FT_PGM_VER == 1) ? 0 : det->DCVOFFSETIN & 0xffff));
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
	udelay(100); /* add by Angus, due to all bank's phase can't be set without delay */

	/* mt_ptp_unlock(&flags); */

	FUNC_EXIT(FUNC_LV_HELP);
}

static int base_ops_get_temp(struct eem_det *det)
{
	thermal_bank_name ts_bank;

#if 1 /* TODO: FIXME */
	FUNC_ENTER(FUNC_LV_HELP);
	/* THERMAL_BANK0 = 0, */ /* CPU0  */
	/* THERMAL_BANK1 = 1, */ /* CPU1 */
	/* THERMAL_BANK2 = 2, */ /* GPU/SOC */

	switch (det_to_id(det)) {
	case EEM_DET_2L:
		ts_bank = THERMAL_BANK0;
		break;
	case EEM_DET_L:
		ts_bank = THERMAL_BANK1;
		break;
	case EEM_DET_CCI:
		ts_bank = THERMAL_BANK2;
		break;
	case EEM_DET_GPU:
	default:
		ts_bank = THERMAL_BANK3;
		break;
	}

	FUNC_EXIT(FUNC_LV_HELP);
#endif

#if !defined(EARLY_PORTING)
	#ifdef __KERNEL__
		#ifdef CONFIG_THERMAL
			return tscpu_get_temp_by_bank(ts_bank);
		#else
			return 0;
		#endif
	#else
		/* thermal_init(); */
		/* udelay(1000); */
		/* return mtktscpu_get_hw_temp(); */
		return 35000;
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

#ifndef EARLY_PORTING_NO_PRI_TBL
static void record(struct eem_det *det)
{
	int i;
	unsigned int vSram;

	det->recordRef[NR_FREQ * 2] = 0x00000000;
	mb(); /* SRAM writing */
	for (i = 0; i < NR_FREQ; i++) {
		vSram = clamp((unsigned int)(record_tbl_locked[i] + 0x10),
				(unsigned int)(VMIN_SRAM + EEM_PMIC_OFFSET),
				(unsigned int)(VMAX_SRAM + EEM_PMIC_OFFSET));
#ifdef EEM_ENABLE_VTURBO
		if ((0 == i) && (1 == VTurboRun)) {
			tTbl = get_turbo(2 , cpuBinLevel_eng);
			det->recordRef[0] =
				((tTbl[5] & 0x3FFFF) << 14) |
				((tTbl[6] & 0x7F) << 7) |
				(tTbl[7] & 0x7F);
		} else {
#endif
			det->recordRef[i*2] = (det->recordRef[i*2] & (~0x3FFF)) |
				((((vSram & 0x7F) << 7) | (record_tbl_locked[i] & 0x7F)) & 0x3fff);
#ifdef EEM_ENABLE_VTURBO
		}
#endif

	}
	det->recordRef[NR_FREQ * 2] = 0xFFFFFFFF;
	mb(); /* SRAM writing */

}

static void restore_record(struct eem_det *det)
	{
		int i;
		unsigned int vWrite;

		eem_debug("restore_record() called !!\n");
		det->recordRef[NR_FREQ * 2] = 0x00000000;
		mb(); /* SRAM writing */
		for (i = 0; i < NR_FREQ; i++) {
			vWrite = det->recordRef[i*2];
			if (det_to_id(det) == EEM_DET_2L)
				vWrite = (vWrite & (~0x3FFF)) |
					(((*(recordTbl + (i * NUM_ELM_SRAM) + 6) & 0x7F) << 7) |
					(*(recordTbl + (i * NUM_ELM_SRAM) + 7) & 0x7F));
			else if (det_to_id(det) == EEM_DET_L)
				vWrite = (vWrite & (~0x3FFF)) |
					(((*(recordTbl + (i + 16) * NUM_ELM_SRAM + 6) & 0x7F) << 7) |
					(*(recordTbl + (i + 16) * NUM_ELM_SRAM + 7) & 0x7F));
			else if (det_to_id(det) == EEM_DET_CCI)
				vWrite = (vWrite & (~0x3FFF)) |
					(((*(recordTbl + (i + 32) * NUM_ELM_SRAM + 6) & 0x7F) << 7) |
					(*(recordTbl + (i + 32) * NUM_ELM_SRAM + 7) & 0x7F));

			det->recordRef[i*2] = vWrite;
		}
		det->recordRef[NR_FREQ * 2] = 0xFFFFFFFF;
		mb(); /* SRAM writing */
	}

static void mt_cpufreq_set_ptbl_funcEEM(enum mt_cpu_dvfs_id id, int restore)
{
	struct eem_det *det = NULL;

	switch (id) {
	case MT_CPU_DVFS_LL:
		det = id_to_eem_det(EEM_DET_2L);
		break;

	case MT_CPU_DVFS_L:
		det = id_to_eem_det(EEM_DET_L);
		break;

	case MT_CPU_DVFS_CCI:
		det = id_to_eem_det(EEM_DET_CCI);
		break;

	default:
		break;
	}

	if (det) {
		if (restore)
			restore_record(det);
		else
			record(det);
	}

}
#endif

/* Will return 10uV */
static int get_volt_cpu(struct eem_det *det)
{
	int value = 0;

	FUNC_ENTER(FUNC_LV_HELP);
	FUNC_EXIT(FUNC_LV_HELP);

	/* unit mv * 100 = 10uv */  /* I-Chang */
	#ifdef EARLY_PORTING
		return value;
	#else
		switch (det_to_id(det)) {
		case EEM_DET_2L:
			value = mt_cpufreq_get_cur_volt(MT_CPU_DVFS_LL);
			break;

		case EEM_DET_L:
			value = mt_cpufreq_get_cur_volt(MT_CPU_DVFS_L);
			break;

		case EEM_DET_CCI:
			value = mt_cpufreq_get_cur_volt(MT_CPU_DVFS_CCI);
			break;

		default:
			value = 0;
			break;
		}

		return value;
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

		switch (det_to_id(det)) {
		case EEM_DET_2L:
			value = mt_cpufreq_update_volt(MT_CPU_DVFS_LL, record_tbl_locked, det->num_freq_tbl);
			break;

		case EEM_DET_L:
			value = mt_cpufreq_update_volt(MT_CPU_DVFS_L, record_tbl_locked, det->num_freq_tbl);
			break;

		case EEM_DET_CCI:
			value = mt_cpufreq_update_volt(MT_CPU_DVFS_CCI, record_tbl_locked, det->num_freq_tbl);
			break;

		default:
			value = 0;
			break;
		}
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
	switch (det_to_id(det)) {
	case EEM_DET_2L:
		mt_cpufreq_restore_default_volt(MT_CPU_DVFS_LL);
		break;

	case EEM_DET_L:
		mt_cpufreq_restore_default_volt(MT_CPU_DVFS_L);
		break;

	case EEM_DET_CCI:
		mt_cpufreq_restore_default_volt(MT_CPU_DVFS_CCI);
		break;

	}
	#endif


	FUNC_EXIT(FUNC_LV_HELP);
}


static void get_freq_table_cpu(struct eem_det *det)
{
	int i;
#if 0
	unsigned int binLevel, binLevel_eng;
	#ifndef EARLY_PORTING
	enum mt_cpu_dvfs_id cpu;

	FUNC_ENTER(FUNC_LV_HELP);
	cpu = (det_to_id(det) == EEM_DET_2L) ?
		MT_CPU_DVFS_LL : MT_CPU_DVFS_L;
	#endif

	#if 0
		/* det->max_freq_khz = mt_cpufreq_get_freq_by_idx(cpu, 0); */
		#ifdef __KERNEL__
			#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
			binLevel = 1;
			#else
			binLevel = GET_BITS_VAL(7:0, get_devinfo_with_index(21));
			binLevel_eng = GET_BITS_VAL(15:0, get_devinfo_with_index(19));
			#endif
		#else
			#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
			binLevel = 1;
			#else
			binLevel = GET_BITS_VAL(7:0, eem_read(0x1020627C));
			binLevel_eng = GET_BITS_VAL(15:0, eem_read(0x10206278));
			#endif
		#endif
	#else
	binLevel = 1;
	binLevel_eng = 1;
	#endif
	for (i = 0; i < NR_FREQ; i++) {
		#if 0 /* EARLY_PORTING , we can use below code for default setting*/
			det->freq_tbl[i] =
				PERCENT((det_to_id(det) == EEM_DET_2L) ? llFreq_SB[i] : littleFreq_SB[i],
					det->max_freq_khz);
		#else
			if ((1 == binLevel) || (3 == binLevel)) {
				det->freq_tbl[i] =
				PERCENT((det_to_id(det) == EEM_DET_L) ? littleFreq_FY[i] :
				(det_to_id(det) == EEM_DET_2L) ? llFreq_FY[i] :
				cciFreq_FY[i]
				,
				det->max_freq_khz);
			} else if ((2 == binLevel) || (4 == binLevel)) {
				det->freq_tbl[i] =
				PERCENT((det_to_id(det) == EEM_DET_L) ? littleFreq_SB[i] :
				(det_to_id(det) == EEM_DET_2L) ? llFreq_SB[i] :
				cciFreq_SB[i]
				,
				det->max_freq_khz);
			} else {
				if ((2 == ((binLevel_eng >> 4) & 0x07)) ||
					(2 == ((binLevel_eng >> 10) & 0x07))) {
					det->freq_tbl[i] =
					PERCENT((det_to_id(det) == EEM_DET_L) ? littleFreq_FY[i] :
					(det_to_id(det) == EEM_DET_2L) ? llFreq_FY[i] :
					cciFreq_FY[i]
					,
					det->max_freq_khz);
				} else {
					det->freq_tbl[i] =
					PERCENT((det_to_id(det) == EEM_DET_L) ? littleFreq_SB[i] :
					(det_to_id(det) == EEM_DET_2L) ? llFreq_SB[i] :
					cciFreq_SB[i]
					,
					det->max_freq_khz);
				}
			}
		#endif
		if (0 == det->freq_tbl[i])
			break;
	}
#endif

	for (i = 0; i < NR_FREQ; i++) {
		det->freq_tbl[i] =
		PERCENT((det_to_id(det) == EEM_DET_L) ? littleFreq_FY[i] :
		(det_to_id(det) == EEM_DET_2L) ? llFreq_FY[i] :
		cciFreq_FY[i],
		det->max_freq_khz);

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
	int i, status;
	unsigned int table_num;

	FUNC_ENTER(FUNC_LV_HELP);
#if defined(__MTK_SLT_) && defined(PTP_SLT_EARLY_PORTING)
	table_num = 8;
#else
	table_num = mt_gpufreq_get_dvfs_table_num();
#endif

	/* eem_debug("set_volt_gpu=%x %x %x %x %x %x %x\n",
		det->volt_tbl_pmic[0], det->volt_tbl_pmic[2],
		det->volt_tbl_pmic[4], det->volt_tbl_pmic[6],
		det->volt_tbl_pmic[8], det->volt_tbl_pmic[10],
		det->volt_tbl_pmic[14]); */

	FUNC_EXIT(FUNC_LV_HELP);

	if (NR_FREQ == table_num) {
		status = mt_gpufreq_update_volt(det->volt_tbl_pmic, table_num);
	} else {
		for (i = 0; i < 8; i++)
			gpuOutput[i] = det->volt_tbl_pmic[i * 2];

		status = mt_gpufreq_update_volt(gpuOutput, table_num);
	}

	return status;
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
	unsigned int table_num, gpuTblidx;

	FUNC_ENTER(FUNC_LV_HELP);
	det->num_freq_tbl = NR_FREQ;
#if defined(__MTK_SLT_) && defined(PTP_SLT_EARLY_PORTING)
	table_num = 8;
	for (i = 0; i < NR_FREQ; i++) {
		det->freq_tbl[i] = PERCENT(gpuFreq[i], det->max_freq_khz);
		if (0 == det->freq_tbl[i])
			break;
	}
#else
	table_num = mt_gpufreq_get_dvfs_table_num();
	for (i = 0; i < NR_FREQ; i++) {
		gpuTblidx = (table_num == NR_FREQ) ? i : ((i/2) * (table_num/8));
		det->freq_tbl[i] = PERCENT(mt_gpufreq_get_freq_by_idx(gpuTblidx), det->max_freq_khz);
		/* eem_error("[get_GPU_freq] i:%d, gpuTblidx:%d, freq:%d, freq_tbl:0x%x\n",
			i, gpuTblidx, mt_gpufreq_get_freq_by_idx(gpuTblidx), det->freq_tbl[i]); */

		if (0 == det->freq_tbl[i])
			BUG_ON(0 == det->freq_tbl[i]);
	}
#endif

	FUNC_EXIT(FUNC_LV_HELP);
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
#if 0
static enum hrtimer_restart eem_log_timer_func(struct hrtimer *timer)
{
	struct eem_det *det;

	FUNC_ENTER(FUNC_LV_HELP);

	for_each_det(det) {
		eem_error("Timer Bank=%d (%d)(%d, %d, %d, %d, %d, %d, %d, %d)(0x%x)\n",
			   det->ctrl_id, det->ops->get_temp(det),
			   EEM_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[0]),
			   EEM_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[1]),
			   EEM_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[2]),
			   EEM_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[3]),
			   EEM_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[4]),
			   EEM_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[5]),
			   EEM_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[6]),
			   EEM_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[7]),
			   det->t250);
		eem_error("sts(%d)\n",
			det->ops->get_status(det),
			);
	}

	hrtimer_forward_now(timer, ns_to_ktime(LOG_INTERVAL));
	FUNC_EXIT(FUNC_LV_HELP);

	return HRTIMER_RESTART;
}
#else
static enum hrtimer_restart eem_log_timer_func(struct hrtimer *timer)
{
	struct eem_det *det;

	FUNC_ENTER(FUNC_LV_HELP);

	for_each_det(det) {
		eem_error("Timer Bank=%d - (%d) (%d, %d, %d, %d, %d, %d, %d, %d)(0x%x)\n",
			   det->ctrl_id, det->ops->get_temp(det),
			   EEM_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[0]),
			   EEM_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[1]),
			   EEM_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[2]),
			   EEM_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[3]),
			   EEM_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[4]),
			   EEM_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[5]),
			   EEM_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[6]),
			   EEM_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[7]),
			   det->t250
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
		eem_error("sts(%d)\n",
			det->ops->get_status(det)
			);
	}

	hrtimer_forward_now(timer, ns_to_ktime(LOG_INTERVAL));
	FUNC_EXIT(FUNC_LV_HELP);

	return HRTIMER_RESTART;
}
#endif
#endif

#ifdef EEM_ENABLE_TTURBO

/*
 * Thread for temperature reading
 * static int eem_thermal_turbo_handler(void *data)
 */
static enum hrtimer_restart eem_thermal_turbo_timer_func(struct hrtimer *timer)
{
/* #if defined(EEM_ENABLE_VTURBO) */
	unsigned long flags;
	struct eem_det *det = id_to_eem_det(EEM_DET_L);

	if (ctrl_TTurbo < 2) {
		eem_debug("Default thermal turbo off (%d) !!\n", ctrl_TTurbo);
		return HRTIMER_RESTART;
	}
	/* eem_debug("Thermal turbo start to run (%d) !!", ctrl_TTurbo); */

	cur_temp = det->ops->get_temp(det);
	/* eem_error("The BIG CPU temperature = %d !!\n", cur_temp); */
	if (cur_temp >= pre_temp) {
		if (cur_temp >= 45000) {
			if (TTurboRun == 1) {
				eem_error("- >=65 off T-TBO (%d)(%d)!!\n", cur_temp, pre_temp);
				tTbl = get_turbo(1, cpuBinLevel_eng); /* 1.8G for Jade, 2470Mhz for Olympus */

				mt_ptp_lock(&flags);
				TTurboRun = 0;
				/* Restore BIG private table */
				/* CCI dcmdiv, CCI_div, dcm_div, clk_div, post_div, DDS */
				/* det->recordRef[1] =
					((tTbl[0] & 0x1F) << 27) |
					((tTbl[1] & 0x1F) << 22) |
					((tTbl[2] & 0x1F) << 17) |
					((tTbl[3] & 0x1F) << 12) |
					((tTbl[4] & 0x07) << 9) |
					(tTbl[5] & 0x1FF); */

				det->recordRef[0] =
					((tTbl[5] & 0x3FFFF) << 14) |
					((tTbl[6] & 0x7F) << 7) |
					(tTbl[7] & 0x7F);
				det->recordRef[1] =
					((tTbl[0] & 0x1F) << 22) |
					((tTbl[1] & 0x3FF) << 12) |
					((tTbl[2] & 0x1F) << 7) |
					((tTbl[3] & 0x7) << 4) |
					(tTbl[4] & 0xF);

				mb(); /* SRAM writing */
				/* turbo_offset =  OD_VTURBO_V; */
				eem_set_eem_volt(det);
				mt_ptp_unlock(&flags);
			} else
				eem_error("- >=65 off(ed) T-TBO (%d)(%d)!!\n", cur_temp, pre_temp);
		}
		#if 0
		else { /* cur_temp < 65000 */
			tTbl = get_turbo(2, cpuBinLevel_eng); /* 1.95G for Jade, 2500Mhz for Olympus*/
			mt_ptp_lock(&flags);
			if ((TTurboRun == 0) && (VTurboRun == 1)) {
				eem_error("UT (<65) Thermal turbo = 1 (%d)(%d)!!", cur_temp, pre_temp);

				TTurboRun = 1;
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
				turbo_offset =  OD_VTURBO_V;
				eem_set_eem_volt(det);
			} else
				eem_error("UT (<65) TTurboRun = %d, VTurboRun = %d, (%d)(%d)",
					TTurboRun, VTurboRun, cur_temp, pre_temp);
			mt_ptp_unlock(&flags);
		}
		#endif
	} else { /* cur_temp < pre_temp */
		if (cur_temp <= 40000) {
			tTbl = get_turbo(2, cpuBinLevel_eng); /* 1.95G for Jade, 2500Mhz for Olympus*/
			mt_ptp_lock(&flags);
			if ((TTurboRun == 0) && (VTurboRun == 1)) {
				eem_error("_ (<=60) on T_TBO (%d)(%d)!!\n", cur_temp, pre_temp);

				TTurboRun = 1;
				/* turbo_offset =  OD_VTURBO_V; */
				eem_set_eem_volt(det);
				/* Revise BIG private table */
				/* CCI dcmdiv, CCI_div, dcm_div, clk_div, post_div, DDS */
				/* det->recordRef[1] =
					((tTbl[0] & 0x1F) << 27) |
					((tTbl[1] & 0x1F) << 22) |
					((tTbl[2] & 0x1F) << 17) |
					((tTbl[3] & 0x1F) << 12) |
					((tTbl[4] & 0x07) << 9) |
					(tTbl[5] & 0x1FF); */
				det->recordRef[0] =
					((tTbl[5] & 0x3FFFF) << 14) |
					((tTbl[6] & 0x7F) << 7) |
					(tTbl[7] & 0x7F);

				det->recordRef[1] =
					((tTbl[0] & 0x1F) << 22) |
					((tTbl[1] & 0x3FF) << 12) |
					((tTbl[2] & 0x1F) << 7) |
					((tTbl[3] & 0x7) << 4) |
					(tTbl[4] & 0xF);
				mb(); /* SRAM writing */
			} else
				eem_error("_ <=60 T_TR = %d, I_TR = %d, (%d)(%d)\n",
					TTurboRun, VTurboRun, cur_temp, pre_temp);
			mt_ptp_unlock(&flags);
		}
	}

	pre_temp = cur_temp;
	hrtimer_forward_now(timer, ns_to_ktime(TML_INTERVAL));
/* #else
	TTurboRun = 0;
	cur_temp = 0;
	pre_temp = 0;
#endif */
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
#ifdef __KERNEL__

	do {
		wait_event_interruptible(ctrl->wq, ctrl->volt_update);

		if ((ctrl->volt_update & EEM_VOLT_UPDATE) && det->ops->set_volt) {
#ifndef EARLY_PORTING
#ifdef CONFIG_EEM_AEE_RR_REC
			int temp = -1;
			#if !defined(EARLY_PORTING) /* I-Chang */
			switch (det->ctrl_id) {
			case EEM_CTRL_2L:
				aee_rr_rec_ptp_status(aee_rr_curr_ptp_status() |
					(1 << EEM_CPU_2_LITTLE_IS_SET_VOLT));
				temp = EEM_CPU_2_LITTLE_IS_SET_VOLT;
				break;

			case EEM_CTRL_L:
				aee_rr_rec_ptp_status(aee_rr_curr_ptp_status() |
					(1 << EEM_CPU_LITTLE_IS_SET_VOLT));
				temp = EEM_CPU_LITTLE_IS_SET_VOLT;
				break;

			case EEM_CTRL_CCI:
				aee_rr_rec_ptp_status(aee_rr_curr_ptp_status() |
					(1 << EEM_CCI_IS_SET_VOLT));
				temp = EEM_CCI_IS_SET_VOLT;
				break;

			case EEM_CTRL_GPU:
				aee_rr_rec_ptp_status(aee_rr_curr_ptp_status() |
					(1 << EEM_GPU_IS_SET_VOLT));
				temp = EEM_GPU_IS_SET_VOLT;
				break;

			default:
				break;
			}
			#endif
#endif
#endif

			det->ops->set_volt(det);

#if (defined(CONFIG_EEM_AEE_RR_REC) && !defined(EARLY_PORTING))
			if (temp >= EEM_CPU_2_LITTLE_IS_SET_VOLT)
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
	/* det->VBOOT = EEM_VOLT_TO_PMIC_VAL(VBOOT_CPU); */

	det->DTHI = DTHI_VAL;
	det->DTLO = DTLO_VAL;
	det->DETMAX = DETMAX_VAL;

	det->AGECONFIG = AGECONFIG_VAL;
	det->AGEM = AGEM_VAL;
	det->DVTFIXED = DVTFIXED_VAL;
	det->VCO = VCO_VAL;
	det->DCCONFIG = DCCONFIG_VAL;

	/*
	if (NULL != det->ops->get_volt) {
		det->VBOOT = EEM_VOLT_TO_PMIC_VAL(det->ops->get_volt(det));
		eem_debug("@%s, %s-VBOOT = %d(%d)\n",
			__func__, ((char *)(det->name) + 8), det->VBOOT, det->ops->get_volt(det));
	} */

	switch (det_id) {
	case EEM_DET_2L:
		det->VBOOT = EEM_VOLT_TO_PMIC_VAL(VBOOT_CPU);
		det->MDES	= devinfo->CPU0_MDES;
		det->BDES	= devinfo->CPU0_BDES;
		det->DCMDET	= devinfo->CPU0_DCMDET;
		det->DCBDET	= devinfo->CPU0_DCBDET;
		det->recordRef  = recordRef;
		#if 0
			int i;

			for (int i = 0; i < NR_FREQ; i++)
				eem_debug("@(Record)%s----->(%s), = 0x%08x\n",
						__func__,
						det->name,
						*(recordRef + i));
		#endif
		break;

	case EEM_DET_L:
		det->VBOOT = EEM_VOLT_TO_PMIC_VAL(VBOOT_CPU);
		det->MDES	= devinfo->CPU1_MDES;
		det->BDES	= devinfo->CPU1_BDES;
		det->DCMDET	= devinfo->CPU1_DCMDET;
		det->DCBDET	= devinfo->CPU1_DCBDET;
		det->recordRef  = recordRef + 36;
		#if 0
			int i;

			for (int i = 0; i < NR_FREQ; i++)
				eem_debug("@(SRAM)%s----->(%s), = 0x%08x\n",
						__func__,
						det->name,
						*(recordRef + i));
		#endif
		break;

	case EEM_DET_CCI:
		det->VBOOT = EEM_VOLT_TO_PMIC_VAL(VBOOT_CPU);
		det->MDES	= devinfo->CCI_MDES;
		det->BDES	= devinfo->CCI_BDES;
		det->DCMDET	= devinfo->CCI_DCMDET;
		det->DCBDET	= devinfo->CCI_DCBDET;
		det->recordRef  = recordRef + 72; /* FIXME: need to check CCI offset */
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
		det->VBOOT = EEM_VOLT_TO_PMIC_VAL(VBOOT_GPU);
		det->MDES	= devinfo->GPU_MDES;
		det->BDES	= devinfo->GPU_BDES;
		det->DCMDET	= devinfo->GPU_DCMDET;
		det->DCBDET	= devinfo->GPU_DCBDET;
		det->VMAX	= VMAX_VAL_GPU;
		det->VMIN	= VMIN_VAL_GPU; /* override default setting */
		det->DVTFIXED   = DVTFIXED_VAL_GPU;
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
	case EEM_CTRL_2L:
		det->AGEDELTA	= devinfo->CPU0_AGEDELTA;
		det->MTDES		= devinfo->CPU0_MTDES;
		break;

	case EEM_CTRL_L:
		det->AGEDELTA	= devinfo->CPU1_AGEDELTA;
		det->MTDES		= devinfo->CPU1_MTDES;
		break;

	case EEM_CTRL_CCI:
		det->AGEDELTA	= devinfo->CCI_AGEDELTA;
		det->MTDES		= devinfo->CCI_MTDES;
		break;

	case EEM_CTRL_GPU:
		det->AGEDELTA	= devinfo->GPU_AGEDELTA;
		det->MTDES	= devinfo->GPU_MTDES;
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


static void eem_set_eem_volt(struct eem_det *det)
{
#if SET_PMIC_VOLT
	unsigned i;
	int cur_temp, low_temp_offset;
	unsigned int eemintsts;
	struct eem_ctrl *ctrl = id_to_eem_ctrl(det->ctrl_id);
#ifdef EEM_ENABLE_VTURBO
	/* int turboVolt; */ /* remove this */
#endif
	cur_temp = det->ops->get_temp(det);
#if defined(CONFIG_EEM_SHOWLOG)
	eem_debug("eem_set_eem_volt cur_temp = %d\n", cur_temp);
#endif

#ifdef __KERNEL__
#ifdef EEM_ENABLE_TTURBO
	if ((cur_temp >= 35000) && (cur_temp <= 75000)) {
		if (thermalTimerStart == 0) {
			thermalTimerStart = 1;
			hrtimer_start(&eem_thermal_turbo_timer, ns_to_ktime(TML_INTERVAL), HRTIMER_MODE_REL);
			eem_error("OO\n");
		}
	} else {
		if (thermalTimerStart == 1) {
			thermalTimerStart = 0;
			hrtimer_cancel(&eem_thermal_turbo_timer);
			eem_error("XX\n");
		}
	}
#endif
#endif


	eemintsts = eem_read(EEMINTSTS);
	low_temp_offset = 0;
	if ((eemintsts & 0x00ff0000) != 0x0) {
		if ((det->isTempInv) && (cur_temp > OVER_INV_TEM))
			det->isTempInv = 0;
		else if (cur_temp <= DEF_INV_TEM)
			det->isTempInv = 1;

		if (det->isTempInv)
			memcpy(det->volt_tbl, det->volt_tbl_init2, NR_FREQ * sizeof(unsigned int));
	}

	if (det->isTempInv) {
		/* Add low temp offset for gpu */
		if (det->ctrl_id == EEM_CTRL_GPU)
			low_temp_offset = LOW_TEMP_OFT;
	}

	ctrl->volt_update |= EEM_VOLT_UPDATE;


#if defined(CONFIG_EEM_SHOWLOG)
	eem_debug("ctrl->volt_update |= EEM_VOLT_UPDATE\n");
#endif


	/* all scale of volt_tbl_pmic, volt_tbl, volt_offset are pmic value */
	/* scale of det->volt_offset must equal 10uV */
	for (i = 0; i < det->num_freq_tbl; i++) {
		det->volt_tbl_pmic[i] = clamp(
						det->volt_tbl[i] + det->volt_offset + low_temp_offset + det->pi_offset,
						det->VMIN + EEM_PMIC_OFFSET,
						det->VMAX + EEM_PMIC_OFFSET);
#ifndef EARLY_PORTING
		switch (det->ctrl_id) {
		case EEM_CTRL_2L:
			det->volt_tbl_pmic[i] = min(det->volt_tbl_pmic[i],
			(*(recordTbl + (i * NUM_ELM_SRAM) + 7) & 0x7F));
			break;
		case EEM_CTRL_L:
			det->volt_tbl_pmic[i] = min(det->volt_tbl_pmic[i],
			(*(recordTbl + (i + 16) * NUM_ELM_SRAM + 7) & 0x7F));
		#if 0
		#if defined(EEM_ENABLE_VTURBO)
			if ((0 == i) && (1 == VTurboRun)) {
				/* turboVolt = turbo_offset; */ /* remove this */
				eem_error("Set Turbo volt !!\n");
				/* todo modify fix voltage */
				det->volt_tbl_pmic[0] =  VMAX_TURBO_VAL; /* set Vturbo voltage*/
			}
		#endif
		#endif

		#if defined(EEM_ENABLE_ITURBO)
			if ((0 == i) && (1 == ITurboRun)) {
				/* turboVolt = turbo_offset; */ /* remove this */
				eem_error("Set Turbo volt !!\n");
				/* todo modify fix voltage */
				/* det->volt_tbl_pmic[0] =  VMAX_VAL; */ /* set Iturbo voltage*/
			}
		#endif
			break;
		case EEM_CTRL_CCI:
			det->volt_tbl_pmic[i] = min(det->volt_tbl_pmic[i],
			(*(recordTbl + (i + 32) * NUM_ELM_SRAM + 7) & 0x7F));
			break;
		case EEM_CTRL_GPU:
			det->volt_tbl_pmic[i] = min(det->volt_tbl_pmic[i], gpuFy[i]);
			break;
		default:
			BUG_ON(det->ctrl_id > EEM_CTRL_GPU);
			break;
		}
#else
		det->volt_tbl_pmic[i] = min(det->volt_tbl_pmic[i], gpuFy[i]);
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

static unsigned int interpolate(unsigned int y1, unsigned int y0,
	unsigned int x1, unsigned int x0, unsigned int ym)
{
	unsigned int ratio, result;

	ratio = (((y1 - y0) * 100) + (x1 - x0 - 1)) / (x1 - x0);
	result =  (x1 - ((((y1 - ym) * 10000) + ratio - 1) / ratio) / 100);
	/*
	eem_debug("y1(%d), y0(%d), x1(%d), x0(%d), ym(%d), ratio(%d), rtn(%d)\n",
		y1, y0, x1, x0, ym, ratio, result);
	*/

	return result;
}

#ifdef EEM_ENABLE_VTURBO
static void eem_turnon_vturbo(struct eem_det *det)
{
	unsigned long flags;
	enum eem_det_id idx;

	mt_ptp_lock(&flags);
	VTurboRun = 1;
	/* Revise BIG private table */
	/* CCI dcmdiv, CCI_div, dcm_div, clk_div, post_div, DDS */
#ifdef EEM_ENABLE_TTURBO
	if (TTurboRun == 0) {
		if ((2 == ctrl_TTurbo) && (det->ops->get_temp(det) >= 45000)) {
			TTurboRun = 1;
			tTbl = get_turbo(1 , cpuBinLevel_eng);
		}
#else
		tTbl = get_turbo(2 , cpuBinLevel_eng);
#endif
		/* eem_error("Set Turbo volt: tTbl[1]:0x%x, tTbl[7]:0x%x !!\n", tTbl[1], tTbl[7]); */

#if 0
		det->recordRef[0] =
			((tTbl[5] & 0x3FFFF) << 14) |
			((tTbl[6] & 0x7F) << 7) |
			(tTbl[7] & 0x7F);
#endif
		/* Due to LL L CCI use common power bulk, update all bank's voltage*/
		for (idx = EEM_DET_2L; idx < EEM_DET_GPU; idx++) {
			recordRef[idx * 36] =
				((tTbl[5] & 0x3FFFF) << 14) |
				((tTbl[6] & 0x7F) << 7) |
				(tTbl[7] & 0x7F);
		}

		det->recordRef[1] =
			((tTbl[0] & 0x1F) << 22) |
			((tTbl[1] & 0x3FF) << 12) |
			((tTbl[2] & 0x1F) << 7) |
			((tTbl[3] & 0x7) << 4) |
			(tTbl[4] & 0xF);

		mb(); /* SRAM writing */
#ifdef EEM_ENABLE_TTURBO
	}
#endif
	/* eem_set_eem_volt(det); */
	mt_ptp_unlock(&flags);

}

static void eem_turnoff_vturbo(struct eem_det *det)
{
	unsigned long flags;
	enum eem_det_id idx;

	mt_ptp_lock(&flags);
	VTurboRun = 0;
#ifdef EEM_ENABLE_TTURBO
	TTurboRun = 0;
#endif
	/* Restore L private table */
#if 0
	det->recordRef[0] =
		((*(recordTbl + (0 * NUM_ELM_SRAM) + 5) & 0x3FFFF) << 14) |
		((*(recordTbl + (0 * NUM_ELM_SRAM) + 6) & 0x7F) << 7) |
		(*(recordTbl + (0 * NUM_ELM_SRAM) + 7) & 0x7F);
#endif

	/* Due to LL L CCI use common power bulk, update all bank's voltage*/
	for (idx = EEM_DET_2L; idx < EEM_DET_GPU; idx++) {
		recordRef[0 * 2 + idx * 36] =
			((*(recordTbl + (idx * NUM_ELM_SRAM) + 5) & 0x3FFFF) << 14) |
			((*(recordTbl + (idx * NUM_ELM_SRAM) + 6) & 0x7F) << 7) |
			(*(recordTbl + (idx * NUM_ELM_SRAM) + 7) & 0x7F);
	}

	/* [26:22] = dcmdiv, [21:12] = DDS, [11:7] = clkdiv, [6:4] = postdiv, [3:0] = CFindex */
	det->recordRef[1] =
		((*(recordTbl + ((0 + 16) * NUM_ELM_SRAM) + 0) & 0x1F) << 22) |
		((*(recordTbl + ((0 + 16) * NUM_ELM_SRAM) + 1) & 0x3FF) << 12) |
		((*(recordTbl + ((0 + 16) * NUM_ELM_SRAM) + 2) & 0x1F) << 7) |
		((*(recordTbl + ((0 + 16) * NUM_ELM_SRAM) + 3) & 0x7) << 4) |
		(*(recordTbl + ((0 + 16) * NUM_ELM_SRAM) + 4) & 0xF);

	/* eem_error("Set Turbo volt: det->recordRef[0]:0x%x, det->recordRef[1]:0x%x !!\n",
		det->recordRef[0], det->recordRef[1]); */

	mb(); /* SRAM writing */
	/* turbo_offset =  0; */
	/* eem_set_eem_volt(det); */
	mt_ptp_unlock(&flags);
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
	eem_debug("CTP - [INIT1:%d]dcvalues 240 = 0x%x\n", det->ctrl_id, det->dcvalues[EEM_PHASE_INIT01]);
	eem_debug("CTP - eem_freqpct30 218 = 0x%x\n", det->eem_freqpct30[EEM_PHASE_INIT01]);
	eem_debug("CTP - eem_26c 26c = 0x%x\n", det->eem_26c[EEM_PHASE_INIT01]);
	eem_debug("CTP - eem_vop30 248 = 0x%x\n", det->eem_vop30[EEM_PHASE_INIT01]);
	eem_debug("CTP - eem_eemen 238 = 0x%x\n", det->eem_eemen[EEM_PHASE_INIT01]);
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
	/* check if DCVALUES is minus and set DCVOFFSETIN to zero */
	if (det->DCVOFFSETIN & 0x8000)
		det->DCVOFFSETIN = 0;

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
	unsigned int temp, i, j;
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
	eem_debug("CTP - [INIT2:%d]dcvalues 240 = 0x%x\n", det->ctrl_id, det->dcvalues[EEM_PHASE_INIT02]);
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

	/* To nodify MD_SRAM & GPU_SRAM use the same bulk, MD owner will retrieve this */
	if (EEM_CTRL_GPU == det->ctrl_id)
		eem_write(EEMSPARE3, temp);

	/* eem_isr_info("init02 read(VOP30) = 0x%08X\n", temp); */
	/* VOP30=>pmic value */
	det->volt_tbl[0] = (temp & 0xff)       + EEM_PMIC_OFFSET;
	det->volt_tbl[1 * (NR_FREQ / 8)] = ((temp >> 8)  & 0xff) + EEM_PMIC_OFFSET;
	det->volt_tbl[2 * (NR_FREQ / 8)] = ((temp >> 16) & 0xff) + EEM_PMIC_OFFSET;
	det->volt_tbl[3 * (NR_FREQ / 8)] = ((temp >> 24) & 0xff) + EEM_PMIC_OFFSET;

	temp = eem_read(VOP74);

	/* eem_isr_info("init02 read(VOP74) = 0x%08X\n", temp); */
	/* VOP74=>pmic value */
	det->volt_tbl[4 * (NR_FREQ / 8)] = (temp & 0xff)       + EEM_PMIC_OFFSET;
	det->volt_tbl[5 * (NR_FREQ / 8)] = ((temp >> 8)  & 0xff) + EEM_PMIC_OFFSET;
	det->volt_tbl[6 * (NR_FREQ / 8)] = ((temp >> 16) & 0xff) + EEM_PMIC_OFFSET;
	det->volt_tbl[7 * (NR_FREQ / 8)] = ((temp >> 24) & 0xff) + EEM_PMIC_OFFSET;

	if (NR_FREQ > 8) {
		for (i = 0; i < 8; i++) {
			for (j = 1; j < (NR_FREQ / 8); j++) {
				if (i < 7) {
					det->volt_tbl[(i * (NR_FREQ / 8)) + j] =
						interpolate(
							det->freq_tbl[(i * (NR_FREQ / 8))],
							det->freq_tbl[((1 + i) * (NR_FREQ / 8))],
							det->volt_tbl[(i * (NR_FREQ / 8))],
							det->volt_tbl[((1 + i) * (NR_FREQ / 8))],
							det->freq_tbl[(i * (NR_FREQ / 8)) + j]
						);
				} else {
					det->volt_tbl[(i * (NR_FREQ / 8)) + j] =
					clamp(
						interpolate(
							det->freq_tbl[((i - 1) * (NR_FREQ / 8))],
							det->freq_tbl[((i) * (NR_FREQ / 8))],
							det->volt_tbl[((i - 1) * (NR_FREQ / 8))],
							det->volt_tbl[((i) * (NR_FREQ / 8))],
							det->freq_tbl[(i * (NR_FREQ / 8)) + j]
						),
						det->VMIN,
						det->VMAX
					);
				}
			}
		}
	}

	/* backup to volt_tbl_init2 */
	memcpy(det->volt_tbl_init2, det->volt_tbl, sizeof(det->volt_tbl_init2));

	for (i = 0; i < NR_FREQ; i++) {
#if defined(CONFIG_EEM_AEE_RR_REC) && !defined(EARLY_PORTING) /* FIXME for olympus */
		switch (det->ctrl_id) {
#if 0
		case EEM_CTRL_2L:
			aee_rr_rec_ptp_cpu_2_little_volt(((unsigned long long)(det->volt_tbl[i]) << (8 * i)) |
				(aee_rr_curr_ptp_cpu_little_volt() & ~((unsigned long long)(0xFF) << (8 * i))));
			break;
		case EEM_CTRL_GPU:
			aee_rr_rec_ptp_gpu_volt(((unsigned long long)(det->volt_tbl[i]) << (8 * i)) |
				(aee_rr_curr_ptp_gpu_volt() & ~((unsigned long long)(0xFF) << (8 * i))));
			break;
		case EEM_CTRL_L:
			aee_rr_rec_ptp_cpu_little_volt(((unsigned long long)(det->volt_tbl[i]) << (8 * i)) |
				(aee_rr_curr_ptp_cpu_little_volt() & ~((unsigned long long)(0xFF) << (8 * i))));
			break;
#endif
		case EEM_CTRL_GPU:
			if (i < 8) {
				aee_rr_rec_ptp_gpu_volt(
					((unsigned long long)(det->volt_tbl[i]) << (8 * i)) |
					(aee_rr_curr_ptp_gpu_volt() & ~
						((unsigned long long)(0xFF) << (8 * i))
					)
				);
			} else {
				aee_rr_rec_ptp_gpu_volt_1(
					((unsigned long long)(det->volt_tbl[i]) << (8 * (i - 8))) |
					(aee_rr_curr_ptp_gpu_volt_1() & ~
						((unsigned long long)(0xFF) << (8 * (i - 8)))
					)
				);
			}
			break;

		case EEM_CTRL_L:
			if (i < 8) {
				aee_rr_rec_ptp_cpu_little_volt(
					((unsigned long long)(det->volt_tbl[i]) << (8 * i)) |
					(aee_rr_curr_ptp_cpu_little_volt() & ~
						((unsigned long long)(0xFF) << (8 * i))
					)
				);
			} else {
				aee_rr_rec_ptp_cpu_little_volt_1(
					((unsigned long long)(det->volt_tbl[i]) << (8 * (i - 8))) |
					(aee_rr_curr_ptp_cpu_little_volt_1() & ~
						((unsigned long long)(0xFF) << (8 * (i - 8)))
					)
				);
			}
			break;

		case EEM_CTRL_2L:
			if (i < 8) {
				aee_rr_rec_ptp_cpu_2_little_volt(
					((unsigned long long)(det->volt_tbl[i]) << (8 * i)) |
					(aee_rr_curr_ptp_cpu_2_little_volt() & ~
						((unsigned long long)(0xFF) << (8 * i))
					)
				);
			} else {
				aee_rr_rec_ptp_cpu_2_little_volt_1(
					((unsigned long long)(det->volt_tbl[i]) << (8 * (i - 8))) |
					(aee_rr_curr_ptp_cpu_2_little_volt_1() & ~
						((unsigned long long)(0xFF) << (8 * (i - 8)))
					)
				);
			}
			break;

		case EEM_CTRL_CCI:
			if (i < 8) {
				aee_rr_rec_ptp_cpu_cci_volt(
					((unsigned long long)(det->volt_tbl[i]) << (8 * i)) |
					(aee_rr_curr_ptp_cpu_cci_volt() & ~
						((unsigned long long)(0xFF) << (8 * i))
					)
				);
			} else {
				aee_rr_rec_ptp_cpu_cci_volt_1(
					((unsigned long long)(det->volt_tbl[i]) << (8 * (i - 8))) |
					(aee_rr_curr_ptp_cpu_cci_volt_1() & ~
						((unsigned long long)(0xFF) << (8 * (i - 8)))
					)
				);
			}
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
	if (det->ctrl_id <= EEM_CTRL_CCI)
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
	unsigned int temp, i, j;

	FUNC_ENTER(FUNC_LV_LOCAL);

	eem_isr_info("mode = mon %s-isr\n", ((char *)(det->name) + 8));

#if defined(CONFIG_THERMAL) && !defined(EARLY_PORTING)
	eem_isr_info("LL_temp=%d, L_temp=%d, CCI_temp=%d, G_temp=%dn",
		tscpu_get_temp_by_bank(THERMAL_BANK0),
		tscpu_get_temp_by_bank(THERMAL_BANK1),
		tscpu_get_temp_by_bank(THERMAL_BANK2),
		tscpu_get_temp_by_bank(THERMAL_BANK3)
		);
#endif

#if defined(CONFIG_EEM_AEE_RR_REC) && !defined(EARLY_PORTING)
	switch (det->ctrl_id) {
	case EEM_CTRL_2L:
		#ifdef CONFIG_THERMAL
		if (tscpu_get_temp_by_bank(THERMAL_BANK0) != 0) {
			aee_rr_rec_ptp_temp(
			(tscpu_get_temp_by_bank(THERMAL_BANK0) / 1000) << (8 * EEM_CPU_2_LITTLE_IS_SET_VOLT) |
			(aee_rr_curr_ptp_temp() & ~(0xFF << (8 * EEM_CPU_2_LITTLE_IS_SET_VOLT))));
		}
		#endif
		break;

	case EEM_CTRL_L:
	#ifdef CONFIG_THERMAL
		if (tscpu_get_temp_by_bank(THERMAL_BANK1) != 0) {
			aee_rr_rec_ptp_temp(
			(tscpu_get_temp_by_bank(THERMAL_BANK1) / 1000) << (8 * EEM_CPU_LITTLE_IS_SET_VOLT) |
			(aee_rr_curr_ptp_temp() & ~(0xFF << (8 * EEM_CPU_LITTLE_IS_SET_VOLT))));
		}
	#endif
		break;

	case EEM_CTRL_CCI:
	#ifdef CONFIG_THERMAL
		if (tscpu_get_temp_by_bank(THERMAL_BANK2) != 0) {
			aee_rr_rec_ptp_temp(
			(tscpu_get_temp_by_bank(THERMAL_BANK2) / 1000) << (8 * EEM_CCI_IS_SET_VOLT) |
			(aee_rr_curr_ptp_temp() & ~(0xFF << (8 * EEM_CCI_IS_SET_VOLT))));
		}
	#endif
		break;

	case EEM_CTRL_GPU:
		#ifdef CONFIG_THERMAL
		if (tscpu_get_temp_by_bank(THERMAL_BANK3) != 0) {
			aee_rr_rec_ptp_temp(
			(tscpu_get_temp_by_bank(THERMAL_BANK3) / 1000) << (8 * EEM_GPU_IS_SET_VOLT) |
			(aee_rr_curr_ptp_temp() & ~(0xFF << (8 * EEM_GPU_IS_SET_VOLT))));
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
	eem_debug("CTP - [MON:%d]dcvalues 240 = 0x%x\n", det->ctrl_id, det->dcvalues[EEM_PHASE_MON]);
	eem_debug("CTP - [MON]eem_freqpct30 218 = 0x%x\n", det->eem_freqpct30[EEM_PHASE_MON]);
	eem_debug("CTP - [MON]eem_26c 26c = 0x%x\n", det->eem_26c[EEM_PHASE_MON]);
	eem_debug("CTP - [MON]eem_vop30 248 = 0x%x\n", det->eem_vop30[EEM_PHASE_MON]);
	eem_debug("CTP - [MON]eem_eemen 238 = 0x%x\n", det->eem_eemen[EEM_PHASE_MON]);
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

	/* 0x64 mappint to 100 + 25 = 125C,
	   0xB2 mapping to 178 - 128 = 50, -50 + 25 = -25C */
	if (((det->t250 & 0xff)  > 0x4b) && ((det->t250  & 0xff) < 0xd3)) {
		eem_error("Temperature to high ----- (%d) degree !!\n", det->t250 + 25);
		goto out;
	}

	temp = eem_read(VOP30);

	/* eem_debug("mon eem_read(VOP30) = 0x%08X\n", temp); */
	/* VOP30=>pmic value */
	det->volt_tbl[0] =  (temp & 0xff)	 + EEM_PMIC_OFFSET;
	det->volt_tbl[1 * (NR_FREQ / 8)] = ((temp >> 8)  & 0xff) + EEM_PMIC_OFFSET;
	det->volt_tbl[2 * (NR_FREQ / 8)] = ((temp >> 16) & 0xff) + EEM_PMIC_OFFSET;
	det->volt_tbl[3 * (NR_FREQ / 8)] = ((temp >> 24) & 0xff) + EEM_PMIC_OFFSET;

	temp = eem_read(VOP74);

	/* eem_debug("mon eem_read(VOP74) = 0x%08X\n", temp); */
	/* VOP74=>pmic value */
	det->volt_tbl[4 * (NR_FREQ / 8)] =  (temp & 0xff)        + EEM_PMIC_OFFSET;
	det->volt_tbl[5 * (NR_FREQ / 8)] = ((temp >> 8)  & 0xff) + EEM_PMIC_OFFSET;
	det->volt_tbl[6 * (NR_FREQ / 8)] = ((temp >> 16) & 0xff) + EEM_PMIC_OFFSET;
	det->volt_tbl[7 * (NR_FREQ / 8)] = ((temp >> 24) & 0xff) + EEM_PMIC_OFFSET;


	if (NR_FREQ > 8) {
		for (i = 0; i < 8; i++) {
			for (j = 1; j < (NR_FREQ / 8); j++) {
				if (i < 7) {
					det->volt_tbl[(i * (NR_FREQ / 8)) + j] =
						interpolate(
							det->freq_tbl[(i * (NR_FREQ / 8))],
							det->freq_tbl[((1 + i) * (NR_FREQ / 8))],
							det->volt_tbl[(i * (NR_FREQ / 8))],
							det->volt_tbl[((1 + i) * (NR_FREQ / 8))],
							det->freq_tbl[(i * (NR_FREQ / 8)) + j]
						);
				} else {
					det->volt_tbl[(i * (NR_FREQ / 8)) + j] =
					clamp(
						interpolate(
							det->freq_tbl[((i - 1) * (NR_FREQ / 8))],
							det->freq_tbl[((i) * (NR_FREQ / 8))],
							det->volt_tbl[((i - 1) * (NR_FREQ / 8))],
							det->volt_tbl[((i) * (NR_FREQ / 8))],
							det->freq_tbl[(i * (NR_FREQ / 8)) + j]
						),
						det->VMIN,
						det->VMAX
					);
				}
			}
		}
	}

	for (i = 0; i < NR_FREQ; i++) {
	#if defined(CONFIG_EEM_AEE_RR_REC) && !defined(EARLY_PORTING) /* FIXME for olympus */
		switch (det->ctrl_id) {
		case EEM_CTRL_GPU:
			if (i < 8) {
				aee_rr_rec_ptp_gpu_volt(
					((unsigned long long)(det->volt_tbl[i]) << (8 * i)) |
					(aee_rr_curr_ptp_gpu_volt() & ~
						((unsigned long long)(0xFF) << (8 * i))
					)
				);
			} else {
				aee_rr_rec_ptp_gpu_volt_1(
					((unsigned long long)(det->volt_tbl[i]) << (8 * (i - 8))) |
					(aee_rr_curr_ptp_gpu_volt_1() & ~
						((unsigned long long)(0xFF) << (8 * (i - 8)))
					)
				);
			}
			break;

		case EEM_CTRL_L:
			if (i < 8) {
				aee_rr_rec_ptp_cpu_little_volt(
					((unsigned long long)(det->volt_tbl[i]) << (8 * i)) |
					(aee_rr_curr_ptp_cpu_little_volt() & ~
						((unsigned long long)(0xFF) << (8 * i))
					)
				);
			} else {
				aee_rr_rec_ptp_cpu_little_volt_1(
					((unsigned long long)(det->volt_tbl[i]) << (8 * (i - 8))) |
					(aee_rr_curr_ptp_cpu_little_volt_1() & ~
						((unsigned long long)(0xFF) << (8 * (i - 8)))
					)
				);
			}
			break;

		case EEM_CTRL_2L:
			if (i < 8) {
				aee_rr_rec_ptp_cpu_2_little_volt(
					((unsigned long long)(det->volt_tbl[i]) << (8 * i)) |
					(aee_rr_curr_ptp_cpu_2_little_volt() & ~
						((unsigned long long)(0xFF) << (8 * i))
					)
				);
			} else {
				aee_rr_rec_ptp_cpu_2_little_volt_1(
					((unsigned long long)(det->volt_tbl[i]) << (8 * (i - 8))) |
					(aee_rr_curr_ptp_cpu_2_little_volt_1() & ~
						((unsigned long long)(0xFF) << (8 * (i - 8)))
					)
				);
			}
			break;

		case EEM_CTRL_CCI:
			if (i < 8) {
				aee_rr_rec_ptp_cpu_cci_volt(
					((unsigned long long)(det->volt_tbl[i]) << (8 * i)) |
					(aee_rr_curr_ptp_cpu_cci_volt() & ~
						((unsigned long long)(0xFF) << (8 * i))
					)
				);
			} else {
				aee_rr_rec_ptp_cpu_cci_volt_1(
					((unsigned long long)(det->volt_tbl[i]) << (8 * (i - 8))) |
					(aee_rr_curr_ptp_cpu_cci_volt_1() & ~
						((unsigned long long)(0xFF) << (8 * (i - 8)))
					)
				);
			}
			break;

		default:
			break;
		}
	#endif
		eem_isr_info("mon_[%s].volt_tbl[%d] = 0x%08X (%d)\n",
			     det->name, i, det->volt_tbl[i], EEM_PMIC_VAL_TO_VOLT(det->volt_tbl[i]));
	}
	eem_debug("M,B=(%d),T(%d),DC(0x%x),V74(0x%x),V30(0x%x),F74(0x%x),F30(0x%x),sts(0x%x),250(0x%x)\n",
		det->ctrl_id,
		det->ops->get_temp(det),
		eem_read(DCVALUES),
		eem_read(VOP74),
		eem_read(VOP30),
		eem_read(FREQPCT74),
		eem_read(FREQPCT30),
		eem_read(EEMEN),
		det->t250);
	eem_isr_info("ISR : TEMPSPARE1 = 0x%08X\n", eem_read(TEMPSPARE1));

	#if defined(__MTK_SLT_)
		if ((cpu_in_mon == 1) && (det->ctrl_id <= EEM_CTRL_CCI))
			eem_isr_info("Won't do CPU eem_set_eem_volt\n");
		else{
			if (det->ctrl_id <= EEM_CTRL_CCI) {
				eem_isr_info("Do CPU eem_set_eem_volt\n");
				cpu_in_mon = 1;
			}
			eem_set_eem_volt(det);
		}
	#else
		eem_set_eem_volt(det);

		#if defined(EEM_ENABLE_VTURBO)
			if (EEM_CTRL_L == det->ctrl_id) {
				ctrl_VTurbo = (0 == ctrl_VTurbo) ? 0 : 2;
				#if defined(EEM_ENABLE_TTURBO)
					ctrl_TTurbo = (0 == ctrl_TTurbo) ? 0 : 2;
				#endif
				#if defined(EEM_ENABLE_ITURBO)
					ctrl_ITurbo = (0 == ctrl_ITurbo) ? 0 : 2;
				#endif
				eem_debug("Finished L monitor mode!!\n");
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
	eem_isr_info("TEMPMSR0 (%p) = 0x%08X\n",
		     TEMPMSR0, eem_read(TEMPMSR0));
	eem_isr_info("TEMPMSR1 (%p) = 0x%08X\n",
		     TEMPMSR1, eem_read(TEMPMSR1));
	eem_isr_info("TEMPMSR2 (%p) = 0x%08X\n",
		     TEMPMSR2, eem_read(TEMPMSR2));
	eem_isr_info("TEMPMONCTL0 (%p) = 0x%08X\n",
		     TEMPMONCTL0, eem_read(TEMPMONCTL0));
	eem_isr_info("TEMPMSRCTL1 (%p) = 0x%08X\n",
		     TEMPMSRCTL1, eem_read(TEMPMSRCTL1));
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

	eem_debug("Bank_number = %d %s-isr, 0x%X, 0x%X\n",
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

		det->ops->switch_bank(det, NR_EEM_PHASE);

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
		case EEM_DET_2L:
			cpu = MT_CPU_DVFS_LITTLE;
			break;

		case EEM_DET_L:
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
		case EEM_DET_2L:
			cpu = MT_CPU_DVFS_LITTLE;
			break;

		case EEM_DET_L:
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

#if defined(__KERNEL__) && defined(EEM_ENABLE_VTURBO)
#define VTURBO_CPU_NUM 1
static int __cpuinit _mt_eem_cpu_CB(struct notifier_block *nfb,	unsigned long action, void *hcpu)
{
	unsigned int cpu = (unsigned long)hcpu;
	unsigned int online_cpus = num_online_cpus();
	struct device *dev;
	struct eem_det *det;
	enum mt_eem_cpu_id cluster_id;

	/* CPU mask - Get on-line cpus per-cluster */
	struct cpumask eem_cpumask;
	struct cpumask cpu_online_cpumask;
	unsigned int cpus, big_cpus, timeout;

	if (ctrl_VTurbo < 2) {
		eem_debug("Default Turbo off (%d) !!", ctrl_VTurbo);
		return NOTIFY_OK;
	}
	/* eem_debug("Turbo start to run (%d) !!", ctrl_VTurbo); */
	/* tTbl = get_turbo(2, cpuBinLevel_eng); */ /* 1.95G for Jade, 2500Mhz for Olympus */


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
		eem_debug("@%s():%d, cpu = %d, act = %lu, on_cpus = %d, clst = %d, clst_cpu = %d\n"
		, __func__, __LINE__, cpu, action, online_cpus, cluster_id, cpus);

	dev = get_cpu_device(cpu);
	if (dev) {
		if (MT_EEM_CPU_L != cluster_id) {
			det = id_to_eem_det(EEM_DET_L);
			arch_get_cluster_cpus(&eem_cpumask, MT_EEM_CPU_L);
			cpumask_and(&cpu_online_cpumask, &eem_cpumask, cpu_online_mask);
			big_cpus = cpumask_weight(&cpu_online_cpumask);
		} else {
			/* This is 2L cluster cpu, we already check in previous code, just save variable */
			big_cpus = cpus;
		}

		switch (action) {
		case CPU_POST_DEAD:
			if ((0 == VTurboRun) &&
				(cpu_online(TURBO_CPU_ID)) &&
				(VTURBO_CPU_NUM >= big_cpus) &&
				(cpu != TURBO_CPU_ID) && (online_cpus <= 2) &&
				(cpu_online(1) == 0) && (cpu_online(2) == 0) && (cpu_online(3) == 0)) {
				if (eem_log_en)
					eem_error("VTurbo(1) POST_DEAD c(%d), on_c(%d), L_clust(%d)\n",
						cpu, online_cpus, big_cpus);
				/* aee_rr_rec_ptp_cpu_big_volt(((unsigned long long)big_cpus << 32) |
					(online_cpus << 24) | (cpu << 16) | (action << 8) | VTurboRun); */
				eem_turnon_vturbo(det);
#ifdef EEM_ENABLE_ITURBO
			} else if ((0 == big_cpus) && (0 == ITurboRun) && (2 == ctrl_ITurbo)) {
				/* Enable Iturbo to decrease the LL voltage */
				if (eem_log_en)
					eem_error("ITurbo(1) POST_DEAD_1 c(%d), on_c(%d), L_clust(%d)\n",
						cpu, online_cpus, big_cpus);
				mt_ptp_lock(&flags);
				ITurboRun = 1;

				/* turbo_offset =  OD_IVER_TURBO_V; */
				eem_set_eem_volt(det);
				mt_ptp_unlock(&flags);
#endif
			} else {
				if (eem_log_en)
					eem_error("VTurbo(%d)ed !! POST_DEAD_2 c(%d), on_c(%d), L_clust(%d)\n",
						VTurboRun, cpu, online_cpus, big_cpus);
			}
		break;

		case CPU_DOWN_PREPARE:
			if ((1 == VTurboRun) && (cpu == TURBO_CPU_ID)) {
				if (eem_log_en)
					eem_error("VTurbo(0) DOWN_PRE c(%d), on_c(%d), L_clust(%d)\n",
					cpu, online_cpus, big_cpus);
				eem_turnoff_vturbo(det);
			} else {
				if (eem_log_en)
					eem_error("VTurbo(%d)ed !! DOWN_PRE_1 c(%d), on_c(%d), L_clust(%d)\n",
						VTurboRun, cpu, online_cpus, big_cpus);
			}
		break;

		case CPU_UP_PREPARE:
			if ((0 == VTurboRun) &&
				(0 == big_cpus) && (cpu_online(0)) &&
				(cpu == TURBO_CPU_ID) &&
				(MT_EEM_CPU_L == cluster_id)) {
				/* Turn on VTurbo */
				if (eem_log_en)
					eem_error("Turbo(1) UP c(%d), on_c(%d), L_num(%d)\n",
						cpu, online_cpus, big_cpus);
				eem_turnon_vturbo(det);
			} else if ((1 == VTurboRun) && (2 >= online_cpus) && (0 != cpu)) {
				if (eem_log_en)
					eem_error("Turbo(0) UP_1 c(%d), on_c(%d), L_num(%d)\n",
						cpu, online_cpus, big_cpus);

				eem_turnoff_vturbo(det);
				timeout = 0;
				/* To make sure dvfs already change L freq */
				while (((eem_read(REG_ARMPLL_L_CON1) >> 12) & 0x3FF) >
					*(recordTbl + (16 * NUM_ELM_SRAM) + 1)) {
					udelay(120);
					if (timeout == 100) {
						BUG_ON(((eem_read(REG_ARMPLL_L_CON1) >> 12) & 0x3FF) >
							*(recordTbl + (16 * NUM_ELM_SRAM) + 1));
					}
					timeout++;
				}
				udelay(120);
			} else {
				if (eem_log_en)
					eem_error("VTurbo(%d), UP_2 c(%d), on_c(%d), L_clust(%d)\n",
						VTurboRun, cpu, online_cpus, big_cpus);
			}
		break;
#ifdef EEM_ENABLE_ITURBO
		case CPU_ONLINE:
			if (2 == ctrl_ITurbo) {
				if ((0 == ITurboRun) &&
					(0 == big_cpus)) {
					/* Enable Iturbo to decrease the LL voltage */
					if (eem_log_en)
						eem_error("ITurbo(1) POST_DEAD c(%d), on_c(%d), L_clust(%d)\n",
							cpu, online_cpus, big_cpus);
					mt_ptp_lock(&flags);
					ITurboRun = 1;

					/* turbo_offset =  OD_IVER_TURBO_V; */
					eem_set_eem_volt(det);
					mt_ptp_unlock(&flags);
				}
				if (eem_log_en)
					eem_error("ITurbo(%d), UP c(%d), on_c(%d), L_clust(%d)\n",
						ITurboRun, cpu, online_cpus, big_cpus);
			}
		break;
#endif
		}
	}
	return NOTIFY_OK;
}

unsigned int get_turbo_status(void)
{
	return VTurboRun;
}
#endif

void eem_init02(const char *str)
{
	struct eem_det *det;
	struct eem_ctrl *ctrl;

	FUNC_ENTER(FUNC_LV_LOCAL);
	eem_debug("eem_init02 called by [%s]\n", str);

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

#ifndef EARLY_PORTING
			/* To check is there are only 4 cpus enabled */
			if (setup_max_cpus != 4)
				BUG_ON(EEM_VOLT_TO_PMIC_VAL(det->ops->get_volt(det)) != det->VBOOT);
#endif
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
			if ((EEM_CTRL_2L == det->ctrl_id) && (1 == det->eem_eemen[EEM_PHASE_INIT01]))
				out |= BIT(EEM_CTRL_2L);
			else if ((EEM_CTRL_L == det->ctrl_id) && (1 == det->eem_eemen[EEM_PHASE_INIT01]))
				out |= BIT(EEM_CTRL_L);
			else if ((EEM_CTRL_CCI == det->ctrl_id) && (1 == det->eem_eemen[EEM_PHASE_INIT01]))
				out |= BIT(EEM_CTRL_CCI);
			else if ((EEM_CTRL_GPU == det->ctrl_id) && (1 == det->eem_eemen[EEM_PHASE_INIT01]))
				out |= BIT(EEM_CTRL_GPU);
		}
		if ((0x0f == out) || (30 == timeout)) {
			eem_error("init01 finish time is %d, bankmask:0x%x\n", timeout, out);
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
		val[0] = get_devinfo_with_index(50);
		val[1] = get_devinfo_with_index(51);
		val[2] = get_devinfo_with_index(52);
		val[3] = get_devinfo_with_index(53);
		val[4] = get_devinfo_with_index(54);
		val[5] = get_devinfo_with_index(55);
		val[6] = get_devinfo_with_index(56);
		val[7] = get_devinfo_with_index(57);
		val[8] = get_devinfo_with_index(58);
#if defined(CONFIG_EEM_AEE_RR_REC)
		aee_rr_rec_ptp_60((unsigned int)val[0]);
		aee_rr_rec_ptp_64((unsigned int)val[1]);
		aee_rr_rec_ptp_68((unsigned int)val[2]);
		aee_rr_rec_ptp_6C((unsigned int)val[3]);
		aee_rr_rec_ptp_78((unsigned int)val[4]);
		aee_rr_rec_ptp_7C((unsigned int)val[5]);
		aee_rr_rec_ptp_80((unsigned int)val[6]);
		aee_rr_rec_ptp_84((unsigned int)val[7]);
		aee_rr_rec_ptp_88((unsigned int)val[8]);
#endif

	#else
		val[0] = eem_read(0x10206580); /* EEM0 */
		val[1] = eem_read(0x10206584); /* EEM1 */
		val[2] = eem_read(0x10206588); /* EEM2 */
		val[3] = eem_read(0x1020658C); /* EEM3 */
		val[4] = eem_read(0x10206590); /* EEM4 */
		val[5] = eem_read(0x10206594); /* EEM5 */
		val[6] = eem_read(0x10206598); /* EEM6 */
		val[7] = eem_read(0x1020659C); /* EEM7 */
		val[8] = eem_read(0x102065A0); /* EEM8 */
	#endif
#else
	/* test pattern */
	val[0] = 0x00000010; /* EEM0 */
	val[1] = 0x10C85BEF; /* EEM1 */
	val[2] = 0x00460060; /* EEM2 */
	val[3] = 0x10C85FE6; /* EEM3 */
	val[4] = 0x00460060; /* EEM4 */
	val[5] = 0x10C861E5; /* EEM5 */
	val[6] = 0x00460060; /* EEM6 */
	val[7] = 0x10C84FE4; /* EEM7 */
	val[8] = 0x004F0060; /* EEM8 */


#endif
	/* test pattern */
#if 0
	val[0] = 0x00000010; /* EEM0 */
	val[1] = 0x10BD3C1B; /* EEM1 */
	val[2] = 0x00550000; /* EEM2 */
	val[3] = 0x10BD3C1B; /* EEM3 */
	val[4] = 0x00550000; /* EEM4 */
	val[5] = 0x10BD3C1B; /* EEM5 */
	val[6] = 0x00550000; /* EEM6 */
	val[7] = 0x10BD3C1B; /* EEM7 */
	val[8] = 0x00550000; /* EEM8 */

	val[0] = 0x00000010; /* EEM0 */
	val[1] = 0x10C85BEF; /* EEM1 */
	val[2] = 0x00460060; /* EEM2 */
	val[3] = 0x10C85FE6; /* EEM3 */
	val[4] = 0x00460060; /* EEM4 */
	val[5] = 0x10C861E5; /* EEM5 */
	val[6] = 0x00460060; /* EEM6 */
	val[7] = 0x10C84FE4; /* EEM7 */
	val[8] = 0x004F0060; /* EEM8 */
#endif



	/* Check BDES... then set EEMINITEN to 1*/
	#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
	if ((p->CPU0_BDES != 0) && (p->CPU1_BDES != 0))
		val[9] = 0x01;
	if ((p->CPU0_MTDES != 0) && (p->CPU1_MTDES != 0))
		val[9] = val[8] | (0x01<<8);
	#else
	if ((p->CPU0_BDES != 0) && (p->CPU1_BDES != 0) && (p->GPU_BDES != 0))
		val[9] = 0x01;
	if ((p->CPU0_MTDES != 0) && (p->CPU1_MTDES != 0) && (p->GPU_MTDES != 0))
		val[9] = val[9] | (0x01<<8);
	#endif

	eem_debug("M_HW_RES0 = 0x%X\n", val[0]);
	eem_debug("M_HW_RES1 = 0x%X\n", val[1]);
	eem_debug("M_HW_RES2 = 0x%X\n", val[2]);
	eem_debug("M_HW_RES3 = 0x%X\n", val[3]);
	eem_debug("M_HW_RES4 = 0x%X\n", val[4]);
	eem_debug("M_HW_RES5 = 0x%X\n", val[5]);
	eem_debug("M_HW_RES6 = 0x%X\n", val[6]);
	eem_debug("M_HW_RES7 = 0x%X\n", val[7]);
	eem_debug("M_HW_RES8 = 0x%X\n", val[8]);
	eem_debug("p->EEMINITEN=0x%x\n", p->EEMINITEN);
	eem_debug("p->EEMMONEN=0x%x\n", p->EEMMONEN);


	/* To check is there are only 4 cpus enabled */
	if (setup_max_cpus == 4) {
		p->EEMINITEN = 0;
		p->EEMMONEN  = 0;
	}

#ifdef DISABLE_EEM
	p->EEMINITEN = 0;
	p->EEMMONEN  = 0;
#endif

	#ifdef EEM_ENABLE_VTURBO
		ctrl_VTurbo = 1; /* t */

		#ifdef EEM_ENABLE_TTURBO
			ctrl_TTurbo = 0;
		#endif
		#ifdef EEM_ENABLE_ITURBO
			ctrl_ITurbo = 0;
		#endif
	#else
		ctrl_VTurbo = 0; /* u */
		#ifdef EEM_ENABLE_TTURBO
		ctrl_TTurbo = 0;
		#endif
		#ifdef EEM_ENABLE_ITURBO
		ctrl_ITurbo = 0;
		#endif
	#endif

	FUNC_EXIT(FUNC_LV_HELP);
}

#ifdef EARLY_PORTING_FAKE_FUNC
/* add by Angus for 1st stage early porting */
typedef void (*mt_cpufreq_set_ptbl_funcPTP)(enum mt_cpu_dvfs_id id, int restore);
void mt_cpufreq_set_ptbl_registerCB(mt_cpufreq_set_ptbl_funcPTP pCB)
{
}
#endif

static int eem_probe(struct platform_device *pdev)
{
	int ret;
	struct eem_det *det;
	struct eem_ctrl *ctrl;
	#if (defined(__KERNEL__) && !defined(CONFIG_MTK_CLKMGR) && !defined(EARLY_PORTING))
		struct clk *clk_thermal;
		struct clk *clk_mfg, *clk_mfg_scp; /* for gpu clock use */
	#endif
	/* unsigned int code = mt_get_chip_hw_code(); */

	FUNC_ENTER(FUNC_LV_MODULE);

	#ifdef __KERNEL__
		#ifndef EARLY_PORTING
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

			/* disable DVFS and set vproc = 850mV (LITTLE = 689 MHz)(BIG = 1196 MHz) */
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
		#ifndef PTP_SLT_EARLY_PORTING
		dvfs_disable_by_ptpod();
		gpu_dvfs_disable_by_ptpod();
		#endif
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

#ifndef EARLY_PORTING
	#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
		/* for Jade-(MT6353) */
		pmic_set_register_value(PMIC_RG_VPROC_MODESET, 1);
		pmic_set_register_value(PMIC_RG_VCORE_MODESET, 1);
		pmic_set_register_value(PMIC_RG_VCORE2_MODESET, 1);
	#else

		/* for Jade/Everest/Olympus(MT6351) */
		/* pmic_config_interface(0x462, 0x1, 0x1, 1); */ /* set PWM mode for MT6351 */
		pmic_force_vgpu_pwm(true);

		/* mt6311_config_interface(0x7C, 0x1, 0x1, 6); */ /* set PWM mode for MT6311 */
		mt6311_set_rg_vdvfs11_modeset(true); /* set PWM mode for MT6311 */
	#endif
#endif
#ifdef EEM_DVT_TEST
	otp_fake_temp_test();
#endif

	/* for slow idle */
	ptp_data[0] = 0xffffffff;

	for_each_det(det)
		eem_init_det(det, &eem_devinfo);


	#ifdef __KERNEL__
	#ifndef EARLY_PORTING_NO_PRI_TBL
		mt_cpufreq_set_ptbl_registerCB(mt_cpufreq_set_ptbl_funcEEM);
	#endif
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

#ifndef EARLY_PORTING
	#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
		/* for Jade-(MT6353) */
		pmic_set_register_value(PMIC_RG_VPROC_MODESET, 0);
		pmic_set_register_value(PMIC_RG_VCORE_MODESET, 0);
		pmic_set_register_value(PMIC_RG_VCORE2_MODESET, 0);
	#else
		/* for Jade/Everest/Olympus(MT6351) */
		/* mt6311_config_interface(0x7C, 0x0, 0x1, 6); */ /* set non-PWM mode for MT6311 */
		mt6311_set_rg_vdvfs11_modeset(false);

		/* pmic_config_interface(0x462, 0x0, 0x1, 1); */ /* set non-PWM mode for MT6351 */
		pmic_force_vgpu_pwm(false);
	#endif
#endif

	#ifdef __KERNEL__
		#ifndef EARLY_PORTING
			#if !defined(CONFIG_MTK_CLKMGR)
				clk_disable_unprepare(clk_mfg); /* Disable GPU clock */
				clk_disable_unprepare(clk_mfg_scp); /* Disable GPU MTCMOSE */
				#ifndef EEM_DVT_TEST
				clk_disable_unprepare(clk_thermal); /* Disable Thermal clock */
				#endif
			#endif
		mt_gpufreq_enable_by_ptpod();/* Disable GPU bulk */
		mt_ppm_ptpod_policy_deactivate();

		/* enable frequency hopping (main PLL) */
		mt_fh_popod_restore();/* I-Chang */
		#endif
	#else
		#ifndef PTP_SLT_EARLY_PORTING
			gpu_dvfs_enable_by_ptpod();
			dvfs_enable_by_ptpod();
		#endif
	#endif

#if defined(EEM_ENABLE_VTURBO)
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
#ifdef __KERNEL__
#ifndef EARLY_PORTING
	mt_cpufreq_eem_resume();
#endif
#endif
	eem_init02(__func__);

	FUNC_EXIT(FUNC_LV_MODULE);

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id mt_eem_of_match[] = {
	{ .compatible = "mediatek,mt6757-eem_fsm", },
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
	thermal_init();
	udelay(500);

	for_each_det_ctrl(det, ctrl) {
		unsigned long flag; /* <-XXX */

		eem_debug("CTP - get_volt:0x%x, VBOOT:0x%x\n",
			EEM_VOLT_TO_PMIC_VAL(det->ops->get_volt(det)),
			det->VBOOT);

		mt_ptp_lock(&flag); /* <-XXX */
		det->ops->init01(det);
		mt_ptp_unlock(&flag); /* <-XXX */
	}

	while (1) {
		for_each_det_ctrl(det, ctrl) {
			if ((EEM_CTRL_2L == det->ctrl_id) && (1 == det->eem_eemen[EEM_PHASE_INIT01]))
				out |= BIT(EEM_CTRL_2L);
			else if ((EEM_CTRL_L == det->ctrl_id) && (1 == det->eem_eemen[EEM_PHASE_INIT01]))
				out |= BIT(EEM_CTRL_L);
			else if ((EEM_CTRL_CCI == det->ctrl_id) && (1 == det->eem_eemen[EEM_PHASE_INIT01]))
				out |= BIT(EEM_CTRL_CCI);
			else if ((EEM_CTRL_GPU == det->ctrl_id) && (1 == det->eem_eemen[EEM_PHASE_INIT01])) {
				eem_error("eem_init01_ctp det->eem_eemen[EEM_PHASE_INIT01]: 0x%x, bankmask:0x%x\n",
					det->eem_eemen[EEM_PHASE_INIT01], out);
				out |= BIT(EEM_CTRL_GPU);
			}
		}
		if ((0x0f == out) || (300 == timeout)) {
			eem_error("init01 finish time is %d, bankmask:0x%x\n", timeout, out);
			break;
		}
		udelay(100);
		timeout++;
	}

/* #ifndef PTP_SLT_EARLY_PORTING */
/* #endif */

	eem_init02(__func__);
	/* Add wait time to receive mornitor mode isr */
	udelay(500);

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
	FUNC_EXIT(FUNC_LV_API);

	return det->num_freq_tbl;
}
EXPORT_SYMBOL(mt_eem_opp_num);

void mt_eem_opp_freq(enum eem_det_id id, unsigned int *freq)
{
	struct eem_det *det = id_to_eem_det(id);
	int i = 0;

	FUNC_ENTER(FUNC_LV_API);

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

#if defined(__KERNEL__) && defined(CONFIG_THERMAL) && !defined(EARLY_PORTING)
	switch (id) {
	case EEM_DET_2L:
		*temp = tscpu_get_temp_by_bank(THERMAL_BANK0);
		break;
	case EEM_DET_L:
		*temp = tscpu_get_temp_by_bank(THERMAL_BANK1);
		break;
	case EEM_DET_CCI:
		*temp = tscpu_get_temp_by_bank(THERMAL_BANK2);
		break;
	case EEM_DET_GPU:
		*temp = tscpu_get_temp_by_bank(THERMAL_BANK3);
		break;
	default:
		*temp = tscpu_get_temp_by_bank(THERMAL_BANK0);
		break;
	}
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
	int i, k;
	#if DUMP_DATA_TO_DE
		int j;
	#endif


	FUNC_ENTER(FUNC_LV_HELP);
	/*
	eem_detectors[EEM_DET_L].ops->dump_status(&eem_detectors[EEM_DET_L]);
	eem_detectors[EEM_DET_2L].ops->dump_status(&eem_detectors[EEM_DET_2L]);
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
					seq_printf(m, "EEM_LOG: Bank_number = [%d] (%d) - (",
					det->ctrl_id, det->ops->get_temp(det));

					for (k = 0; k < det->num_freq_tbl - 1; k++)
						seq_printf(m, "%d, ",
						EEM_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[k]));
					seq_printf(m, "%d) - (", EEM_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[k]));

					for (k = 0; k < det->num_freq_tbl - 1; k++)
						seq_printf(m, "%d, ", det->freq_tbl[k]);
					seq_printf(m, "%d)\n", det->freq_tbl[k]);
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
		for (i = 0; i < det->num_freq_tbl; i++)
			seq_printf(m, "EEM_HW, det->volt_tbl[%d] = [%x], det->volt_tbl_pmic[%d] = [%x]\n",
			i, det->volt_tbl[i], i, det->volt_tbl_pmic[i]);

		for (i = 0; i < NR_FREQ; i++) {
			seq_printf(m, "(Reserve, 0x%x)(Vs, 0x%x) (Vp, 0x%x, %d) (F_Setting)(%x, %x, %x, %x, %x)\n",
				(det->recordRef[i*2] >> 14) & 0x3FFFF,
				(det->recordRef[i*2] >> 7) & 0x7F,
				det->recordRef[i*2] & 0x7F,
				(EEM_PMIC_VAL_TO_VOLT(det->recordRef[i*2] & 0x7F)),
				(det->recordRef[i*2+1] >> 22) & 0x1F,
				(det->recordRef[i*2+1] >> 12) & 0x3FF,
				(det->recordRef[i*2+1] >> 7) & 0x1F,
				(det->recordRef[i*2+1] >> 4) & 0x7,
				det->recordRef[i*2+1] & 0xF
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
	int ret, index;
	char *buf = (char *) __get_free_page(GFP_USER);
	unsigned int voltValue = 0, voltProc = 0, voltSram = 0, voltPmic = 0, i;
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
		if (2 == sscanf(buf, "%u %d", &voltValue, &index)) {
			ret = 0;
			det->recordRef[NR_FREQ * 2] = 0x00000000;
			mb(); /* SRAM writing */
			voltPmic = EEM_VOLT_TO_PMIC_VAL(voltValue) + EEM_PMIC_OFFSET;
			voltProc = clamp(voltPmic,
				det->VMIN + EEM_PMIC_OFFSET,
				det->VMAX + EEM_PMIC_OFFSET);

			voltSram = clamp((unsigned int)(voltProc + 0x10),
				(unsigned int)(VMIN_SRAM + EEM_PMIC_OFFSET),
				(unsigned int)(VMAX_SRAM + EEM_PMIC_OFFSET));

			if (-1 == index) {
				for (i = 0; i < NR_FREQ; i++) {
					det->recordRef[i*2] = (det->recordRef[i*2] & (~0x3FFF)) |
						(((voltSram & 0x7F) << 7) | (voltProc & 0x7F));
				}
			} else if (index < NR_FREQ) {
				det->recordRef[index*2] = (det->recordRef[index*2] & (~0x3FFF)) |
					(((voltSram & 0x7F) << 7) | (voltProc & 0x7F));
			}


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
#define VPROC_MAX 115000
#define CPU_TURBO_VOLT_DIFF_1 4375
#define CPU_TURBO_VOLT_DIFF_2 2500
#define NUM_OF_CPU_FREQ_TABLE 16
#define NUM_OF_GPU_FREQ_TABLE 16

static int volt_to_buck_volt(u32 cur_volt, int need)
{
	u32 ret_volt = VPROC_MAX;

	if (need) {
		if (cur_volt >= VPROC_MAX)
			ret_volt = VPROC_MAX;
		else {
			if (cur_volt % 625 == 0)
				ret_volt = cur_volt;
			else
				ret_volt = (cur_volt + 625) / 625 * 1000;
		}
	}
	return ret_volt;
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
		case EEM_CTRL_2L: /* I-Chang */
			result |= (eem_not_work(rdata, det->volt_tbl_pmic,
				NUM_OF_CPU_FREQ_TABLE) << EEM_CTRL_2L);
			if ((result & (1 << EEM_CTRL_2L)) != 0)
				eem_isr_info("BANK0 EEM fail\n");
			break;

		case EEM_CTRL_L: /* I-Chang */
			result |= (eem_not_work(rdata, det->volt_tbl_pmic,
				NUM_OF_CPU_FREQ_TABLE) << EEM_CTRL_L);
			if ((result & (1 << EEM_CTRL_L)) != 0)
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
	int i;
	struct eem_det *det = (struct eem_det *)m->private;

	FUNC_ENTER(FUNC_LV_HELP);

	seq_printf(m, "bank = %d, (%d) - (",
		   det->ctrl_id, det->ops->get_temp(det));
	for (i = 0; i < det->num_freq_tbl - 1; i++)
		seq_printf(m, "%d, ", EEM_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[i]));
	seq_printf(m, "%d) - (", EEM_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[i]));

	for (i = 0; i < det->num_freq_tbl - 1; i++)
		seq_printf(m, "%d, ", det->freq_tbl[i]);
	seq_printf(m, "%d)\n", det->freq_tbl[i]);

	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

/*
 * set EEM log enable by procfs interface
 */

static int eem_log_en_proc_show(struct seq_file *m, void *v)
{
	FUNC_ENTER(FUNC_LV_HELP);
	seq_printf(m, "0x%x\n", eem_log_en);
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

	if (kstrtoint(buf, 16, &eem_log_en)) {
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
		if ((eem_log_en & 0xF00) != 0) {
			eem_debug("EEMSPARE1 = 0x%08x\n", eem_log_en);
			eem_write(EEMSPARE1, eem_log_en);
			}
		else
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

/*
 * set EEM ITurbo enable by procfs interface
 */

static int eem_iturbo_en_proc_show(struct seq_file *m, void *v)
{

	FUNC_ENTER(FUNC_LV_HELP);
	seq_printf(m, "%s\n", ((ctrl_VTurbo) ? "Enable" : "Disable"));
	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

static ssize_t eem_iturbo_en_proc_write(struct file *file,
				     const char __user *buffer, size_t count, loff_t *pos)
{
	int ret;
	unsigned int value;
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

	if (kstrtoint(buf, 10, &value)) {
		eem_debug("bad argument!! Should be \"0\" or \"1\"\n");
		goto out;
	}

	ret = 0;

	switch (value) {
	case 0:
		eem_debug("eem Turbo disabled.\n");
		ctrl_VTurbo = 0;
		break;

	case 1:
		eem_debug("eem Turbo enabled.\n");
		ctrl_VTurbo = 2;
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
PROC_FOPS_RW(eem_iturbo_en);

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
		PROC_ENTRY(eem_iturbo_en),
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

unsigned int get_vcore_ptp_volt(int index)
{
	unsigned int ret;

	switch (index) {
	case VCORE_VOLT_0:
		ret = EEM_VOLT_TO_PMIC_VAL(80000);
		break;

	case VCORE_VOLT_1:
		ret = EEM_VOLT_TO_PMIC_VAL(70000);
		break;

	case VCORE_VOLT_2:
		ret = EEM_VOLT_TO_PMIC_VAL(70000);
		break;

	default:
		ret = EEM_VOLT_TO_PMIC_VAL(80000);
		break;
	}

	return ret;
}

void eem_set_pi_offset(enum eem_ctrl_id id, int step)
{
	struct eem_det *det = id_to_eem_det(id);

	det->pi_offset = step;

#ifdef CONFIG_PTP_AEE_RR_REC
	aee_rr_rec_eem_pi_offset(step);
#endif
}

#if 0
int mt_eem_get_ptp_volt(unsigned int id, unsigned int *pmic_volt, unsigned int *array_size)
{
	struct eem_det *det = NULL;
	int status = -1;

	switch (id) {
	case MT_CPU_DVFS_LL:
		det = id_to_eem_det(EEM_DET_2L);
		break;

	case MT_CPU_DVFS_L:
		det = id_to_eem_det(EEM_DET_L);
		break;

	case MT_CPU_DVFS_CCI:
		det = id_to_eem_det(EEM_DET_CCI);
		break;

	default:
		break;
	}

	if (det) {
		pmic_volt = det->volt_tbl_pmic;
		array_size = det->num_freq_tbl;

		status = ((pmic_volt[0]) && (array_size == NR_FREQ)) ? 0 : -1;
	}


	return status;
}
EXPORT_SYMBOL(mt_eem_get_ptp_volt);
#endif

/*
 * Module driver
 */

#ifdef __KERNEL__
#ifndef EARLY_PORTING
static int __init eem_conf(void)
{
	int i;
	/* unsigned int binLevel, binLevel_eng; */
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

#if 0
	/* read E-fuse for segment selection */
	#if defined(CONFIG_MTK_PMIC_CHIP_MT6353)
	binLevel = 1;
	if (0) {
	#else
	binLevel = GET_BITS_VAL(7:0, get_devinfo_with_index(21));
	binLevel_eng = GET_BITS_VAL(15:0, get_devinfo_with_index(19));
	if (1001 == cpu_speed) {
	#endif

		recordTbl = &fyTbl[0][0];
		/* eem_error("1--->The table ----->(fyTbl), cpu_speed = %d\n", cpu_speed); */
	} else {
		if ((1 == binLevel) || (3 == binLevel)) {
			recordTbl = &fyTbl[0][0];
			eem_error("@The table ----->(fyTbl)\n");
		} else if ((2 == binLevel) || (4 == binLevel)) {
			recordTbl = &sbTbl[0][0];
			eem_error("@The table ----->(sbTbl)\n");
		} else {
			if ((2 == ((binLevel_eng >> 4) & 0x07)) || (2 == ((binLevel_eng >> 10) & 0x07)))
				recordTbl = &fyTbl[0][0];
			else
				recordTbl = &sbTbl[0][0];
		}
	}
#else
	recordTbl = &fyTbl[0][0];
#endif

	for (i = 0; i < NR_FREQ; i++) {
		/* LL
		[31:14] = Reserve % (/2), [13:7] = Vsram pmic value, [6:0] = Vproc pmic value
		*/
		recordRef[i * 2] =
			((*(recordTbl + (i * NUM_ELM_SRAM) + 5) & 0x3FFFF) << 14) |
			((*(recordTbl + (i * NUM_ELM_SRAM) + 6) & 0x7F) << 7) |
			(*(recordTbl + (i * NUM_ELM_SRAM) + 7) & 0x7F);

		/* [26:22] = dcmdiv, [21:12] = DDS, [11:7] = clkdiv, [6:4] = postdiv, [3:0] = CFindex */
		recordRef[i * 2 + 1] =
			((*(recordTbl + (i * NUM_ELM_SRAM) + 0) & 0x1F) << 22) |
			((*(recordTbl + (i * NUM_ELM_SRAM) + 1) & 0x3FF) << 12) |
			((*(recordTbl + (i * NUM_ELM_SRAM) + 2) & 0x1F) << 7) |
			((*(recordTbl + (i * NUM_ELM_SRAM) + 3) & 0x7) << 4) |
			(*(recordTbl + (i * NUM_ELM_SRAM) + 4) & 0xF);

		/* L
		[31:14] = Reserve % (/2), [13:7] = Vsram pmic value, [6:0] = Vproc pmic value
		*/
		recordRef[i * 2 + 36] =
			((*(recordTbl + ((i + 16) * NUM_ELM_SRAM) + 5) & 0x3FFFF) << 14) |
			((*(recordTbl + ((i + 16) * NUM_ELM_SRAM) + 6) & 0x7F) << 7) |
			(*(recordTbl + ((i + 16) * NUM_ELM_SRAM) + 7) & 0x7F);

		/* [26:22] = dcmdiv, [21:12] = DDS, [11:7] = clkdiv, [6:4] = postdiv, [3:0] = CFindex */
		recordRef[i * 2 + 36 + 1] =
			((*(recordTbl + ((i + 16) * NUM_ELM_SRAM) + 0) & 0x1F) << 22) |
			((*(recordTbl + ((i + 16) * NUM_ELM_SRAM) + 1) & 0x3FF) << 12) |
			((*(recordTbl + ((i + 16) * NUM_ELM_SRAM) + 2) & 0x1F) << 7) |
			((*(recordTbl + ((i + 16) * NUM_ELM_SRAM) + 3) & 0x7) << 4) |
			(*(recordTbl + ((i + 16) * NUM_ELM_SRAM) + 4) & 0xF);

		/* CCI
		[31:14] = Reserve % (/2), [13:7] = Vsram pmic value, [6:0] = Vproc pmic value
		*/
		recordRef[i * 2 + 72] =
			((*(recordTbl + ((i + 32) * NUM_ELM_SRAM) + 5) & 0x3FFFF) << 14) |
			((*(recordTbl + ((i + 32) * NUM_ELM_SRAM) + 6) & 0x7F) << 7) |
			(*(recordTbl + ((i + 32) * NUM_ELM_SRAM) + 7) & 0x7F);

		/* [26:22] = dcmdiv, [21:12] = DDS, [11:7] = clkdiv, [6:4] = postdiv, [3:0] = CFindex */
		recordRef[i * 2 + 72 + 1] =
			((*(recordTbl + ((i + 32) * NUM_ELM_SRAM) + 0) & 0x1F) << 22) |
			((*(recordTbl + ((i + 32) * NUM_ELM_SRAM) + 1) & 0x3FF) << 12) |
			((*(recordTbl + ((i + 32) * NUM_ELM_SRAM) + 2) & 0x1F) << 7) |
			((*(recordTbl + ((i + 32) * NUM_ELM_SRAM) + 3) & 0x7) << 4) |
			(*(recordTbl + ((i + 32) * NUM_ELM_SRAM) + 4) & 0xF);

	}
	recordRef[i*2] = 0xffffffff;
	recordRef[i*2+36] = 0xffffffff;
	recordRef[i*2+72] = 0xffffffff;
	mb(); /* SRAM writing */
	for (i = 0; i < NR_FREQ; i++)
		eem_debug("LL (Reserve, 0x%x), (Vs, 0x%x), (Vp, 0x%x), (F_Setting)(%x, %x, %x, %x, %x)\n",
			((*(recordRef + (i * 2))) >> 14) & 0x3FFFF,
			((*(recordRef + (i * 2))) >> 7) & 0x7F,
			(*(recordRef + (i * 2))) & 0x7F,
			((*(recordRef + (i * 2) + 1)) >> 22) & 0x1F,
			((*(recordRef + (i * 2) + 1)) >> 12) & 0x3FF,
			((*(recordRef + (i * 2) + 1)) >> 7) & 0x1F,
			((*(recordRef + (i * 2) + 1)) >> 4) & 0x7,
			((*(recordRef + (i * 2) + 1)) & 0xF)
			);

	for (i = 0; i < NR_FREQ; i++)
		eem_debug("L (Reserve, 0x%x), (Vs, 0x%x), (Vp, 0x%x), (F_Setting)(%x, %x, %x, %x, %x)\n",
			((*(recordRef + 36 + (i * 2))) >> 14) & 0x3FFFF,
			((*(recordRef + 36 + (i * 2))) >> 7) & 0x7F,
			(*(recordRef + 36 + (i * 2))) & 0x7F,
			((*(recordRef + 36 + (i * 2) + 1)) >> 22) & 0x1F,
			((*(recordRef + 36 + (i * 2) + 1)) >> 12) & 0x3FF,
			((*(recordRef + 36 + (i * 2) + 1)) >> 7) & 0x1F,
			((*(recordRef + 36 + (i * 2) + 1)) >> 4) & 0x7,
			((*(recordRef + 36 + (i * 2) + 1)) & 0xF)
			);

	for (i = 0; i < NR_FREQ; i++)
		eem_debug("CCI (Reserve, 0x%x), (Vs, 0x%x), (Vp, 0x%x), (F_Setting)(%x, %x, %x, %x, %x)\n",
			((*(recordRef + 72 + (i * 2))) >> 14) & 0x3FFFF,
			((*(recordRef + 72 + (i * 2))) >> 7) & 0x7F,
			(*(recordRef + 72 + (i * 2))) & 0x7F,
			((*(recordRef + 72 + (i * 2) + 1)) >> 22) & 0x1F,
			((*(recordRef + 72 + (i * 2) + 1)) >> 12) & 0x3FF,
			((*(recordRef + 72 + (i * 2) + 1)) >> 7) & 0x1F,
			((*(recordRef + 72 + (i * 2) + 1)) >> 4) & 0x7,
			((*(recordRef + 72 + (i * 2) + 1)) & 0xF)
			);

	return 0;

}
#endif

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
	node = of_find_compatible_node(NULL, NULL, "mediatek,mt6757-eem_fsm");

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

	#if defined(EEM_ENABLE_VTURBO)
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

#ifdef EEM_ENABLE_TTURBO
	hrtimer_init(&eem_thermal_turbo_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	eem_thermal_turbo_timer.function = eem_thermal_turbo_timer_func;
#endif
	create_procfs();
	/*
	 * reg platform device driver
	 */
	err = platform_driver_register(&eem_driver);

	if (err) {
		eem_debug("EEM driver callback register failed..\n");
		FUNC_EXIT(FUNC_LV_MODULE);
		return err;
	}
#endif


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
#ifndef EARLY_PORTING
module_init(eem_conf);
#endif
late_initcall(eem_init);
#endif
#endif

MODULE_DESCRIPTION("MediaTek EEM Driver v0.3");
MODULE_LICENSE("GPL");
#ifdef EARLY_PORTING
	#undef EARLY_PORTING
#endif
#undef __MT_EEM_C__

