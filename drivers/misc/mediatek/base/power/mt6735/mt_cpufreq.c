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

/**
 * @file	mt_cpufreq.c
 * @brief   Driver for CPU DVFS
 *
 */

#define __MT_CPUFREQ_C__
#define DEBUG 1
/*=============================================================*/
/* Include files											   */
/*=============================================================*/

/* system includes */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/cpu.h>
#include <linux/cpufreq.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/notifier.h>
#include <linux/fb.h>
#include <linux/spinlock.h>
#include <linux/kthread.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/jiffies.h>
#include <linux/bitops.h>
#include <linux/uaccess.h>
#include <linux/seq_file.h>
#include <asm/io.h>

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_address.h>
#endif

#include "mt-plat/aee.h"

/* project includes */
#include "mach/mt_thermal.h"
#include <mt-plat/mt_hotplug_strategy.h>
/*#include "mach/mt_spm_idle.h"*/
#include "mach/mt_clkmgr.h"
#include "mach/mt_freqhopping.h"
#include "mt_ptp.h"
#include "mt_static_power.h"
#include "mach/mt_pbm.h"
#include "mt-plat/mt_devinfo.h"

#ifndef __KERNEL__
#include "freqhop_sw.h"
#include "mt_spm.h"
#include "pmic.h"
#include "mt_pmic_wrap.h"
#include "efuse.h"	/* for SLT efuse check */
#else
#include "mt_spm.h"
#include "mt-plat/upmu_common.h"
#include "mach/upmu_sw.h"
#include "mach/upmu_hw.h"
/* #include "pwrap_hal.h" */
#endif

/* local includes */
#include "mt_cpufreq.h"


/*=============================================================*/
/* Macro definition											*/
/*=============================================================*/
#if defined(CONFIG_ARCH_MT6735M)
#define PMIC_WRAP_DVFS_ADR0			 ((unsigned long)(PMIC_WRAP_BASE+0xE8))
#define PMIC_WRAP_DVFS_WDATA0		   ((unsigned long)(PMIC_WRAP_BASE+0xEC))
#define PMIC_WRAP_DVFS_ADR1			 ((unsigned long)(PMIC_WRAP_BASE+0xF0))
#define PMIC_WRAP_DVFS_WDATA1		   ((unsigned long)(PMIC_WRAP_BASE+0xF4))
#define PMIC_WRAP_DVFS_ADR2			 ((unsigned long)(PMIC_WRAP_BASE+0xF8))
#define PMIC_WRAP_DVFS_WDATA2		   ((unsigned long)(PMIC_WRAP_BASE+0xFC))
#define PMIC_WRAP_DVFS_ADR3			 ((unsigned long)(PMIC_WRAP_BASE+0x100))
#define PMIC_WRAP_DVFS_WDATA3		   ((unsigned long)(PMIC_WRAP_BASE+0x104))
#define PMIC_WRAP_DVFS_ADR4			 ((unsigned long)(PMIC_WRAP_BASE+0x108))
#define PMIC_WRAP_DVFS_WDATA4		   ((unsigned long)(PMIC_WRAP_BASE+0x10C))
#define PMIC_WRAP_DVFS_ADR5			 ((unsigned long)(PMIC_WRAP_BASE+0x110))
#define PMIC_WRAP_DVFS_WDATA5		   ((unsigned long)(PMIC_WRAP_BASE+0x114))
#define PMIC_WRAP_DVFS_ADR6			 ((unsigned long)(PMIC_WRAP_BASE+0x118))
#define PMIC_WRAP_DVFS_WDATA6		   ((unsigned long)(PMIC_WRAP_BASE+0x11C))
#define PMIC_WRAP_DVFS_ADR7			 ((unsigned long)(PMIC_WRAP_BASE+0x120))
#define PMIC_WRAP_DVFS_WDATA7		   ((unsigned long)(PMIC_WRAP_BASE+0x124))
#define PMIC_WRAP_DVFS_ADR8			 ((unsigned long)(PMIC_WRAP_BASE+0x128))
#define PMIC_WRAP_DVFS_WDATA8		   ((unsigned long)(PMIC_WRAP_BASE+0x12C))
#define PMIC_WRAP_DVFS_ADR9			 ((unsigned long)(PMIC_WRAP_BASE+0x130))
#define PMIC_WRAP_DVFS_WDATA9		   ((unsigned long)(PMIC_WRAP_BASE+0x134))
#define PMIC_WRAP_DVFS_ADR10			((unsigned long)(PMIC_WRAP_BASE+0x138))
#define PMIC_WRAP_DVFS_WDATA10		  ((unsigned long)(PMIC_WRAP_BASE+0x13C))
#define PMIC_WRAP_DVFS_ADR11			((unsigned long)(PMIC_WRAP_BASE+0x140))
#define PMIC_WRAP_DVFS_WDATA11		  ((unsigned long)(PMIC_WRAP_BASE+0x144))
#define PMIC_WRAP_DVFS_ADR12			((unsigned long)(PMIC_WRAP_BASE+0x148))
#define PMIC_WRAP_DVFS_WDATA12		  ((unsigned long)(PMIC_WRAP_BASE+0x14C))
#define PMIC_WRAP_DVFS_ADR13			((unsigned long)(PMIC_WRAP_BASE+0x150))
#define PMIC_WRAP_DVFS_WDATA13		  ((unsigned long)(PMIC_WRAP_BASE+0x154))
#define PMIC_WRAP_DVFS_ADR14			((unsigned long)(PMIC_WRAP_BASE+0x158))
#define PMIC_WRAP_DVFS_WDATA14		  ((unsigned long)(PMIC_WRAP_BASE+0x15C))
#define PMIC_WRAP_DVFS_ADR15			((unsigned long)(PMIC_WRAP_BASE+0x160))
#define PMIC_WRAP_DVFS_WDATA15		  ((unsigned long)(PMIC_WRAP_BASE+0x164))
#else
#define PMIC_WRAP_DVFS_ADR0			 ((unsigned long)(PMIC_WRAP_BASE+0xFC))
#define PMIC_WRAP_DVFS_WDATA0		   ((unsigned long)(PMIC_WRAP_BASE+0x100))
#define PMIC_WRAP_DVFS_ADR1			 ((unsigned long)(PMIC_WRAP_BASE+0x104))
#define PMIC_WRAP_DVFS_WDATA1		   ((unsigned long)(PMIC_WRAP_BASE+0x108))
#define PMIC_WRAP_DVFS_ADR2			 ((unsigned long)(PMIC_WRAP_BASE+0x10C))
#define PMIC_WRAP_DVFS_WDATA2		   ((unsigned long)(PMIC_WRAP_BASE+0x110))
#define PMIC_WRAP_DVFS_ADR3			 ((unsigned long)(PMIC_WRAP_BASE+0x114))
#define PMIC_WRAP_DVFS_WDATA3		   ((unsigned long)(PMIC_WRAP_BASE+0x118))
#define PMIC_WRAP_DVFS_ADR4			 ((unsigned long)(PMIC_WRAP_BASE+0x11C))
#define PMIC_WRAP_DVFS_WDATA4		   ((unsigned long)(PMIC_WRAP_BASE+0x120))
#define PMIC_WRAP_DVFS_ADR5			 ((unsigned long)(PMIC_WRAP_BASE+0x124))
#define PMIC_WRAP_DVFS_WDATA5		   ((unsigned long)(PMIC_WRAP_BASE+0x128))
#define PMIC_WRAP_DVFS_ADR6			 ((unsigned long)(PMIC_WRAP_BASE+0x12C))
#define PMIC_WRAP_DVFS_WDATA6		   ((unsigned long)(PMIC_WRAP_BASE+0x130))
#define PMIC_WRAP_DVFS_ADR7			 ((unsigned long)(PMIC_WRAP_BASE+0x134))
#define PMIC_WRAP_DVFS_WDATA7		   ((unsigned long)(PMIC_WRAP_BASE+0x138))
#define PMIC_WRAP_DVFS_ADR8			 ((unsigned long)(PMIC_WRAP_BASE+0x13C))
#define PMIC_WRAP_DVFS_WDATA8		   ((unsigned long)(PMIC_WRAP_BASE+0x140))
#define PMIC_WRAP_DVFS_ADR9			 ((unsigned long)(PMIC_WRAP_BASE+0x144))
#define PMIC_WRAP_DVFS_WDATA9		   ((unsigned long)(PMIC_WRAP_BASE+0x148))
#define PMIC_WRAP_DVFS_ADR10			((unsigned long)(PMIC_WRAP_BASE+0x14C))
#define PMIC_WRAP_DVFS_WDATA10		  ((unsigned long)(PMIC_WRAP_BASE+0x150))
#define PMIC_WRAP_DVFS_ADR11			((unsigned long)(PMIC_WRAP_BASE+0x154))
#define PMIC_WRAP_DVFS_WDATA11		  ((unsigned long)(PMIC_WRAP_BASE+0x158))
#define PMIC_WRAP_DVFS_ADR12			((unsigned long)(PMIC_WRAP_BASE+0x15C))
#define PMIC_WRAP_DVFS_WDATA12		  ((unsigned long)(PMIC_WRAP_BASE+0x160))
#define PMIC_WRAP_DVFS_ADR13			((unsigned long)(PMIC_WRAP_BASE+0x164))
#define PMIC_WRAP_DVFS_WDATA13		  ((unsigned long)(PMIC_WRAP_BASE+0x168))
#define PMIC_WRAP_DVFS_ADR14			((unsigned long)(PMIC_WRAP_BASE+0x16C))
#define PMIC_WRAP_DVFS_WDATA14		  ((unsigned long)(PMIC_WRAP_BASE+0x170))
#define PMIC_WRAP_DVFS_ADR15			((unsigned long)(PMIC_WRAP_BASE+0x174))
#define PMIC_WRAP_DVFS_WDATA15		  ((unsigned long)(PMIC_WRAP_BASE+0x178))
#endif

/* Operations and REG Access */
#define MAX(a, b)						   ((a) >= (b) ? (a) : (b))
#define MIN(a, b)						   ((a) >= (b) ? (b) : (a))
#define _BIT_(_bit_)						(unsigned)(1 << (_bit_))
#define _BITS_(_bits_, _val_)	\
	((((unsigned) -1 >> (31 - ((1) ? _bits_))) & ~((1U << ((0) ? _bits_)) - 1)) & ((_val_)<<((0) ? _bits_)))
#define _BITMASK_(_bits_)		\
	(((unsigned) -1 >> (31 - ((1) ? _bits_))) & ~((1U << ((0) ? _bits_)) - 1))
#define _GET_BITS_VAL_(_bits_, _val_)	   (((_val_) & (_BITMASK_(_bits_))) >> ((0) ? _bits_))

#define cpufreq_read(addr)				  __raw_readl(addr)
#define cpufreq_write(addr, val)			mt_reg_sync_writel(val, addr)
#define cpufreq_write_mask(addr, mask, val)	\
	cpufreq_write(addr, (cpufreq_read(addr) & ~(_BITMASK_(mask))) | _BITS_(mask, val))


#define for_each_cpu_dvfs(i, p)		for (i = 0, p = cpu_dvfs; i < NR_MT_CPU_DVFS; i++, p = &cpu_dvfs[i])
#define cpu_dvfs_is(p, id)			 (p == &cpu_dvfs[id])
#define cpu_dvfs_is_available(p)	  (p->opp_tbl)
#define cpu_dvfs_get_name(p)		   (p->name)

#define cpu_dvfs_get_cur_freq(p)				(p->opp_tbl[p->idx_opp_tbl].cpufreq_khz)
#define cpu_dvfs_get_freq_by_idx(p, idx)		(p->opp_tbl[idx].cpufreq_khz)
#define cpu_dvfs_get_max_freq(p)				(p->opp_tbl[0].cpufreq_khz)
#define cpu_dvfs_get_normal_max_freq(p)		 (p->opp_tbl[p->idx_normal_max_opp].cpufreq_khz)
#define cpu_dvfs_get_min_freq(p)				(p->opp_tbl[p->nr_opp_tbl - 1].cpufreq_khz)

#define cpu_dvfs_get_cur_volt(p)				(p->opp_tbl[p->idx_opp_tbl].cpufreq_volt)
#define cpu_dvfs_get_volt_by_idx(p, idx)		(p->opp_tbl[idx].cpufreq_volt)

#ifdef CONFIG_CPU_DVFS_HAS_EXTBUCK
#define cpu_dvfs_is_extbuck_valid()	 (is_ext_buck_exist() && is_ext_buck_sw_ready())

/* used @ _mt_cpufreq_set_cur_volt_extBuck() for SW tracking */
#define NORMAL_DIFF_VSRAM_VPROC	(10000)	/* 100mv * 100 */
#define MAX_DIFF_VSRAM_VPROC	   (20000)	/* 200mv * 100 */
#endif


#define MIN_VSRAM_VOLT			 (93125)	/* 931.25mv * 100 */
#define MAX_VSRAM_VOLT			 (131250)	/* 1312.5mv * 100 */
#define MAX_VPROC_VOLT			 (125000)	/* 1250mv * 100 */


 /* PMIC/PLL settle time (us), should not be changed */
#define PMIC_CMD_DELAY_TIME	 (5)
#define MIN_PMIC_SETTLE_TIME	(25)
#define PMIC_VOLT_UP_SETTLE_TIME(old_volt, new_volt)	\
	(((((new_volt) - (old_volt)) + 1250 - 1) / 1250) + PMIC_CMD_DELAY_TIME)
#define PMIC_VOLT_DOWN_SETTLE_TIME(old_volt, new_volt)	\
	(((((old_volt) - (new_volt)) * 2)  / 625) + PMIC_CMD_DELAY_TIME)
#define PLL_SETTLE_TIME			  (20)


/* PMIC wrapper */
#define VOLT_TO_PMIC_VAL(volt)  (((volt) - 60000 + 625 - 1) / 625)
#define PMIC_VAL_TO_VOLT(pmic)  (((pmic) * 625) + 60000)
#ifdef CONFIG_CPU_DVFS_HAS_EXTBUCK
#define VOLT_TO_EXTBUCK_VAL(volt)   VOLT_TO_PMIC_VAL(volt)	/* (((((volt) - 300) + 9) / 10) & 0x7F) */
#define EXTBUCK_VAL_TO_VOLT(val)	PMIC_VAL_TO_VOLT(val)	/* (300 + ((val) & 0x7F) * 10) */
#endif

#define NR_PMIC_WRAP_CMD 16	/* num of pmic wrap cmd (fixed value) */

/* DVFS OPP table */
#ifdef CONFIG_ARCH_MT6735M
#define CPU_DVFS_FREQ0_1 (1248000)	/* KHz */
#define CPU_DVFS_FREQ0   (1144000)	/* KHz */
#define CPU_DVFS_FREQ1_1 (1092000)	/* KHz */
#define CPU_DVFS_FREQ1   (1027000)	/* KHz */
#define CPU_DVFS_FREQ2   (988000)	/* KHz */
#define CPU_DVFS_FREQ3   (923000)	/* KHz */
#define CPU_DVFS_FREQ4   (910000)	/* KHz */
#define CPU_DVFS_FREQ5   (858000)	/* KHz */
#define CPU_DVFS_FREQ6   (793000)	/* KHz */
#define CPU_DVFS_FREQ7   (637000)	/* KHz */
#define CPU_DVFS_FREQ8   (494000)	/* KHz */
#define CPU_DVFS_FREQ9   (364000)	/* KHz */
#define CPU_DVFS_FREQ10  (221000)	/* KHz */

#define CPUFREQ_BOUNDARY_FOR_FHCTL (CPU_DVFS_FREQ1)
#define CPUFREQ_FIX_FREQ_FOR_ES	(CPU_DVFS_FREQ2)
#else
#define CPU_DVFS_FREQ0   (1495000)	/* KHz */
#define CPU_DVFS_FREQ0_1 (1443000)	/* KHz */
#define CPU_DVFS_FREQ1   (1300000)	/* KHz */
#define CPU_DVFS_FREQ2   (1235000)	/* KHz */
#define CPU_DVFS_FREQ3   (1170000)	/* KHz */
#define CPU_DVFS_FREQ3_1 (1144000)	/* KHz (for 6753 FY)*/
#define CPU_DVFS_FREQ3_2 (1092000)	/* KHz (for 37T to 35M+)*/
#define CPU_DVFS_FREQ4   (1040000)	/* KHz */
#define CPU_DVFS_FREQ4_1 (988000)		/* KHz (for 37T to 35M+)*/
#define CPU_DVFS_FREQ4_2 (858000)		/* KHz (for 37T to 35M+)*/
#define CPU_DVFS_FREQ5   (819000)		/* KHz */
#define CPU_DVFS_FREQ5_1 (793000)		/* KHz (for 37T to 35M+)*/
#define CPU_DVFS_FREQ5_2 (637000)		/* KHz (for 37T to 35M+)*/
#define CPU_DVFS_FREQ6   (598000)		/* KHz */
#define CPU_DVFS_FREQ6_1 (494000)		/* KHz (for 37T to 35M+)*/
#define CPU_DVFS_FREQ7   (442000)		/* KHz */
#define CPU_DVFS_FREQ7_1 (364000)		/* KHz (for 37T to 35M+)*/
#define CPU_DVFS_FREQ8   (299000)		/* KHz */
#define CPU_DVFS_FREQ8_1 (221000)		/* KHz (for 37T to 35M+)*/

#define CPUFREQ_BOUNDARY_FOR_FHCTL (CPU_DVFS_FREQ4)	/* if cross 1040MHz when DFS, don't used FHCTL */
#define CPUFREQ_FIX_FREQ_FOR_ES	(CPU_DVFS_FREQ4)
#endif

#define OP(khz, volt) {			\
	.cpufreq_khz = khz,			 \
	.cpufreq_volt = volt,		   \
	.cpufreq_volt_org = volt,	   \
}


/* DDS calculation */
#define PLL_FREQ_STEP		(13000)		/* KHz */

#define PLL_DIV1_FREQ		(1001000)	/* KHz */
#define PLL_DIV2_FREQ		(520000)	/* KHz */
#define PLL_DIV4_FREQ		(260000)	/* KHz */
#define PLL_DIV8_FREQ		(130000)	/* KHz */

#define DDS_DIV1_FREQ		(0x0009A000)	/* 1001MHz */
#define DDS_DIV2_FREQ		(0x010A0000)	/* 520MHz  */
#define DDS_DIV4_FREQ		(0x020A0000)	/* 260MHz  */
#define DDS_DIV8_FREQ		(0x030A0000)	/* 130MHz  */


/* Turbo mode */
#ifdef CONFIG_CPU_DVFS_TURBO_MODE
#define TURBO_MODE_BOUNDARY_CPU_NUM	2
#define TURBO_MODE_FREQ(mode, freq)	\
	(((freq * (100 + turbo_mode_cfg[mode].freq_delta)) / PLL_FREQ_STEP) / 100 * PLL_FREQ_STEP)
#define TURBO_MODE_VOLT(mode, volt) (volt + turbo_mode_cfg[mode].volt_delta)

/* idx sort by temp from low to high */
enum turbo_mode {
	TURBO_MODE_2,
	TURBO_MODE_1,
	TURBO_MODE_NONE,

	NR_TURBO_MODE,
};
#endif

/* Power table */
/* Notice: Each table MUST has 8 element to avoid ptpod error */
#define NR_MAX_OPP_TBL  8
#ifdef CONFIG_ARCH_MT6753
#define NR_MAX_CPU	  8
#else
#define NR_MAX_CPU	  4
#endif

/* Power throttling */
#ifdef CONFIG_CPU_DVFS_POWER_THROTTLING
#define PWR_THRO_MODE_LBAT_819MHZ	BIT(0)
#define PWR_THRO_MODE_BAT_PER_819MHZ	BIT(1)
#define PWR_THRO_MODE_BAT_OC_1040MHZ	BIT(2)
#endif


/* PMIC 5A throttle (only for MT6753) */
#ifdef CONFIG_ARCH_MT6753
#define PMIC_5A_THRO_MAX_CPU_CORE_NUM   6
#define PMIC_5A_THRO_MAX_CPU_FREQ	   1144000
#endif


/* Debugging */
#undef TAG
#define TAG	 "[Power/cpufreq] "

#define cpufreq_err(fmt, args...)	   \
	pr_err(TAG"[ERROR]"fmt, ##args)
#define cpufreq_warn(fmt, args...)	  \
	pr_warn(TAG"[WARNING]"fmt, ##args)
#define cpufreq_info(fmt, args...)	  \
	pr_warn(TAG""fmt, ##args)
#define cpufreq_dbg(fmt, args...)	   \
	pr_debug(TAG""fmt, ##args)
#define cpufreq_ver(fmt, args...)	   \
	do {								\
		if (func_lv_mask)		   \
			cpufreq_info(fmt, ##args);	\
	} while (0)

#define FUNC_LV_MODULE		BIT(0)	/* module, platform driver interface */
#define FUNC_LV_CPUFREQ		BIT(1)	/* cpufreq driver interface		  */
#define FUNC_LV_API		BIT(2)	/* mt_cpufreq driver global function */
#define FUNC_LV_LOCAL		BIT(3)	/* mt_cpufreq driver local function  */
#define FUNC_LV_HELP		BIT(4)	/* mt_cpufreq driver help function   */
#define FUNC_ENTER(lv)	\
	do { if ((lv) & func_lv_mask) cpufreq_dbg(">> %s()\n", __func__); } while (0)
#define FUNC_EXIT(lv)	\
	do { if ((lv) & func_lv_mask) cpufreq_dbg("<< %s():%d\n", __func__, __LINE__); } while (0)


/* Lock */
#define cpufreq_lock(flags) \
	do { \
		/* to fix compile warning */  \
		flags = (unsigned long)&flags; \
		mutex_lock(&cpufreq_mutex); \
		is_in_cpufreq = 1;\
		/* spm_mcdi_wakeup_all_cores(); */ \
	} while (0)

#define cpufreq_unlock(flags) \
	do { \
		/* to fix compile warning */  \
		flags = (unsigned long)&flags; \
		is_in_cpufreq = 0;\
		mutex_unlock(&cpufreq_mutex); \
	} while (0)

#if 1
static DEFINE_SPINLOCK(pmic_wrap_lock);
#define pmic_wrap_lock(flags) spin_lock_irqsave(&pmic_wrap_lock, flags)
#define pmic_wrap_unlock(flags) spin_unlock_irqrestore(&pmic_wrap_lock, flags)
#else
#define pmic_wrap_lock(flags) \
	do { \
		/* to fix compile warning */  \
		flags = (unsigned long)&flags; \
		mutex_lock(&pmic_wrap_mutex); \
	} while (0)

#define pmic_wrap_unlock(flags) \
	do { \
		/* to fix compile warning */  \
		flags = (unsigned long)&flags; \
		mutex_unlock(&pmic_wrap_mutex); \
	} while (0)
#endif


/* EFUSE */
#define CPUFREQ_EFUSE_INDEX	 (3)
/* #define FUNC_CODE_EFUSE_INDEX (28) */
#define CPU_LEVEL_0			 (0x0)
#define CPU_LEVEL_1			 (0x1)
#define CPU_LEVEL_2			 (0x2)
#define CPU_LEVEL_3			 (0x3)
#define CPU_LEVEL_4			 (0x4)

/* SRAM debugging */
#ifdef CONFIG_CPU_DVFS_AEE_RR_REC
enum cpu_dvfs_state {
	CPU_DVFS_LITTLE_IS_DOING_DVFS = 0,
	CPU_DVFS_LITTLE_IS_TURBO,
};
#endif


/*=============================================================*/
/* Local type definition									   */
/*=============================================================*/
struct pmic_wrap_cmd {
	unsigned long cmd_addr;
	unsigned long cmd_wdata;
};

struct pmic_wrap_setting {
	enum pmic_wrap_phase_id phase;
	struct pmic_wrap_cmd addr[NR_PMIC_WRAP_CMD];
	struct {
		struct {
			unsigned long cmd_addr;
			unsigned long cmd_wdata;
		} _[NR_PMIC_WRAP_CMD];
		const int nr_idx;
	} set[NR_PMIC_WRAP_PHASE];
};

struct mt_cpu_freq_info {
	const unsigned int cpufreq_khz;
	unsigned int cpufreq_volt;		/* mv * 100 */
	const unsigned int cpufreq_volt_org;	/* mv * 100 */
};

struct mt_cpu_power_info {
	unsigned int cpufreq_khz;
	unsigned int cpufreq_ncpu;
	unsigned int cpufreq_power;
};

struct mt_cpu_dvfs {
	const char *name;
	unsigned int cpu_id;			/* for cpufreq */
	unsigned int cpu_level;
	struct mt_cpu_dvfs_ops *ops;

	/* opp (freq) table */
	struct mt_cpu_freq_info *opp_tbl;	/* OPP table */
	int nr_opp_tbl;				/* size for OPP table */
	int idx_opp_tbl;			/* current OPP idx */
	int idx_normal_max_opp;			/* idx for normal max OPP */
	int idx_opp_tbl_for_late_resume;	/* keep the setting for late resume */

	struct cpufreq_frequency_table *freq_tbl_for_cpufreq;	/* freq table for cpufreq */

	/* power table */
	struct mt_cpu_power_info *power_tbl;
	unsigned int nr_power_tbl;

	/* enable/disable DVFS function */
	bool dvfs_disable_by_ptpod;
	bool dvfs_disable_by_suspend;
	bool dvfs_disable_by_early_suspend;
	bool dvfs_disable_by_procfs;

	/* limit for thermal/PBM */
	unsigned int limited_max_ncpu;
	unsigned int limited_max_freq;
	unsigned int limited_power_idx;
	unsigned int idx_opp_tbl_for_thermal_thro;
	unsigned int limited_power_by_thermal;
#ifndef DISABLE_PBM_FEATURE
	unsigned int limited_power_by_pbm;
#endif

	/* limit for HEVC (via. sysfs) */
	unsigned int limited_freq_by_hevc;

	/* limit max freq from user */
	unsigned int limited_max_freq_by_user;

	/* limit min/max freq from other kernel driver (for BSP package only) */
	unsigned int limited_min_freq_by_kdriver;
	unsigned int limited_max_freq_by_kdriver;

#ifdef CONFIG_CPU_DVFS_TURBO_MODE
	/* turbo mode */
	unsigned int turbo_mode;
#endif

	/* power throttling */
#ifdef CONFIG_CPU_DVFS_POWER_THROTTLING
	int idx_opp_tbl_for_pwr_thro;	/* keep the setting for power throttling */
	int idx_pwr_thro_max_opp;	/* idx for power throttle max OPP */
	unsigned int pwr_thro_mode;
#endif

};

struct mt_cpu_dvfs_ops {
	/* for thermal/PBM */
	int (*setup_power_table)(struct mt_cpu_dvfs *p);

	/* for freq change (PLL/MUX), return (physical) freq (KHz) */
	unsigned int (*get_cur_phy_freq)(struct mt_cpu_dvfs *p);
	/* set freq  */
	void (*set_cur_freq)(struct mt_cpu_dvfs *p, unsigned int cur_khz, unsigned int target_khz);

	/* for volt change (PMICWRAP/extBuck),  return volt (mV * 100)*/
	unsigned int (*get_cur_volt)(struct mt_cpu_dvfs *p);
	/* set volt (mv * 100), return 0 (success), -1 (fail) */
	int (*set_cur_volt)(struct mt_cpu_dvfs *p, unsigned int volt);
};

struct opp_tbl_info {
	struct mt_cpu_freq_info *const opp_tbl;
	const int size;
};

#ifdef CONFIG_CPU_DVFS_TURBO_MODE
struct turbo_mode_cfg {
	int temp;		/* degree x 1000 */
	int freq_delta;		/* percentage	*/
	int volt_delta;		/* mv * 100	   */
};
#endif

/*=============================================================*/
/* Local function definition								   */
/*=============================================================*/
/* for thermal */
static int _mt_cpufreq_setup_power_table(struct mt_cpu_dvfs *p);

/* for freq change (PLL/MUX) */
static unsigned int _mt_cpufreq_get_cur_phy_freq(struct mt_cpu_dvfs *p);
static void _mt_cpufreq_set_cur_freq(struct mt_cpu_dvfs *p, unsigned int cur_khz,
					 unsigned int target_khz);

/* for volt change (PMICWRAP/extBuck) */
static unsigned int _mt_cpufreq_get_cur_volt(struct mt_cpu_dvfs *p);
static int _mt_cpufreq_set_cur_volt(struct mt_cpu_dvfs *p, unsigned int volt);	/* volt: mv * 100 */
#ifdef CONFIG_CPU_DVFS_HAS_EXTBUCK
static unsigned int _mt_cpufreq_get_cur_volt_extbuck(struct mt_cpu_dvfs *p);
static int _mt_cpufreq_set_cur_volt_extbuck(struct mt_cpu_dvfs *p, unsigned int volt);	/* volt: mv * 100 */
#endif

/* CPU callback */
static int __cpuinit _mt_cpufreq_cpu_CB(struct notifier_block *nfb, unsigned long action,
					void *hcpu);

/* cpufreq driver */
#ifdef CONFIG_CPU_FREQ
static int _mt_cpufreq_verify(struct cpufreq_policy *policy);
static int _mt_cpufreq_target(struct cpufreq_policy *policy, unsigned int target_freq,
				  unsigned int relation);
static int _mt_cpufreq_init(struct cpufreq_policy *policy);
static unsigned int _mt_cpufreq_get(unsigned int cpu);
#endif

/* (early-)suspend */
static int _mt_cpufreq_fb_notifier_callback(struct notifier_block *self, unsigned long event, void *data);
static int _mt_cpufreq_suspend(struct device *dev);
static int _mt_cpufreq_resume(struct device *dev);
static int _mt_cpufreq_pm_restore_early(struct device *dev);	/* for IPO-H HW(freq) / SW(opp_tbl_idx) */

/*Platform driver */
static int _mt_cpufreq_pdrv_probe(struct platform_device *pdev);
static int _mt_cpufreq_pdrv_remove(struct platform_device *pdev);


/*=============================================================*/
/* Global Variables								   */
/*=============================================================*/
/* PMIC WRAP */
static struct pmic_wrap_setting pw = {
	.phase = NR_PMIC_WRAP_PHASE,	/* invalid setting for init */
	.addr = {{0, 0} },
#ifdef CONFIG_ARCH_MT6735M
	.set[PMIC_WRAP_PHASE_NORMAL] = {
		._[IDX_NM_VCORE_TRANS4] = {MT6328_PMIC_VCORE1_VOSEL_ON_ADDR, VOLT_TO_PMIC_VAL(121250),},
		._[IDX_NM_VCORE_TRANS3] = {MT6328_PMIC_VCORE1_VOSEL_ON_ADDR, VOLT_TO_PMIC_VAL(118125),},
		._[IDX_NM_VCORE_HPM] = {MT6328_PMIC_VCORE1_VOSEL_ON_ADDR, VOLT_TO_PMIC_VAL(115000),},
		._[IDX_NM_VCORE_TRANS2] = {MT6328_PMIC_VCORE1_VOSEL_ON_ADDR, VOLT_TO_PMIC_VAL(111250),},
		._[IDX_NM_VCORE_TRANS1] = {MT6328_PMIC_VCORE1_VOSEL_ON_ADDR, VOLT_TO_PMIC_VAL(108125),},
		._[IDX_NM_VCORE_LPM] = {MT6328_PMIC_VCORE1_VOSEL_ON_ADDR, VOLT_TO_PMIC_VAL(105000),},
		._[IDX_NM_VCORE_UHPM] = {MT6328_PMIC_VCORE1_VOSEL_ON_ADDR, VOLT_TO_PMIC_VAL(125000),},
		._[IDX_NM_VRF18_0_PWR_ON] = {MT6328_PMIC_RG_VRF18_0_EN_ADDR, _BITS_(1:1, 1),},
		._[IDX_NM_NOT_USED] = {0, 0,},
		._[IDX_NM_VRF18_0_SHUTDOWN] = {MT6328_PMIC_RG_VRF18_0_EN_ADDR, _BITS_(1:1, 0),},
		.nr_idx = NR_IDX_NM,
	},
#else
	.set[PMIC_WRAP_PHASE_NORMAL] = {
		._[IDX_NM_NOT_USED1] = {0, 0,},
		._[IDX_NM_NOT_USED2] = {0, 0,},
#ifdef CONFIG_ARCH_MT6753
		._[IDX_NM_VCORE_HPM] = {MT6328_PMIC_VCORE1_VOSEL_ON_ADDR, VOLT_TO_PMIC_VAL(125000),},
		._[IDX_NM_VCORE_TRANS2] = {MT6328_PMIC_VCORE1_VOSEL_ON_ADDR, VOLT_TO_PMIC_VAL(121250),},
		._[IDX_NM_VCORE_TRANS1] = {MT6328_PMIC_VCORE1_VOSEL_ON_ADDR, VOLT_TO_PMIC_VAL(118125),},
		._[IDX_NM_VCORE_LPM] = {MT6328_PMIC_VCORE1_VOSEL_ON_ADDR, VOLT_TO_PMIC_VAL(115000),},
#else	/* 6735 */
		._[IDX_NM_VCORE_HPM] = {MT6328_PMIC_VCORE1_VOSEL_ON_ADDR, VOLT_TO_PMIC_VAL(115000),},
		._[IDX_NM_VCORE_TRANS2] = {MT6328_PMIC_VCORE1_VOSEL_ON_ADDR, VOLT_TO_PMIC_VAL(111250),},
		._[IDX_NM_VCORE_TRANS1] = {MT6328_PMIC_VCORE1_VOSEL_ON_ADDR, VOLT_TO_PMIC_VAL(108125),},
		._[IDX_NM_VCORE_LPM] = {MT6328_PMIC_VCORE1_VOSEL_ON_ADDR, VOLT_TO_PMIC_VAL(105000),},
#endif
		._[IDX_NM_VRF18_0_SHUTDOWN] = {MT6328_PMIC_RG_VRF18_0_EN_ADDR, _BITS_(1:1, 0),},
		._[IDX_NM_VRF18_0_PWR_ON] = {MT6328_PMIC_RG_VRF18_0_EN_ADDR, _BITS_(1:1, 1),},
		.nr_idx = NR_IDX_NM,
	},
#endif
	.set[PMIC_WRAP_PHASE_SUSPEND] = {
		._[IDX_SP_VPROC_PWR_ON] = {MT6328_PMIC_VPROC_EN_ADDR, _BITS_(0:0, 1),},
		._[IDX_SP_VPROC_SHUTDOWN] = {MT6328_PMIC_VPROC_EN_ADDR, _BITS_(0:0, 0),},
#ifdef CONFIG_ARCH_MT6753
		._[IDX_SP_VCORE_HPM] = {MT6328_PMIC_VCORE1_VOSEL_ON_ADDR, VOLT_TO_PMIC_VAL(125000),},
		._[IDX_SP_VCORE_TRANS2] = {MT6328_PMIC_VCORE1_VOSEL_ON_ADDR, VOLT_TO_PMIC_VAL(121250),},
		._[IDX_SP_VCORE_TRANS1] = {MT6328_PMIC_VCORE1_VOSEL_ON_ADDR, VOLT_TO_PMIC_VAL(118125),},
		._[IDX_SP_VCORE_LPM] = {MT6328_PMIC_VCORE1_VOSEL_ON_ADDR, VOLT_TO_PMIC_VAL(115000),},
#else	/* 6735(M) */
		._[IDX_SP_VCORE_HPM] = {MT6328_PMIC_VCORE1_VOSEL_ON_ADDR, VOLT_TO_PMIC_VAL(115000),},
		._[IDX_SP_VCORE_TRANS2] = {MT6328_PMIC_VCORE1_VOSEL_ON_ADDR, VOLT_TO_PMIC_VAL(111250),},
		._[IDX_SP_VCORE_TRANS1] = {MT6328_PMIC_VCORE1_VOSEL_ON_ADDR, VOLT_TO_PMIC_VAL(108125),},
		._[IDX_SP_VCORE_LPM] = {MT6328_PMIC_VCORE1_VOSEL_ON_ADDR, VOLT_TO_PMIC_VAL(105000),},
#endif
		._[IDX_SP_VSRAM_SHUTDOWN] = {MT6328_PMIC_RG_VSRAM_EN_ADDR, _BITS_(1:1, 0),},
		._[IDX_SP_VRF18_0_PWR_ON] = {MT6328_PMIC_RG_VRF18_0_EN_ADDR, _BITS_(1:1, 1),},
		._[IDX_SP_VSRAM_PWR_ON] = {MT6328_PMIC_RG_VSRAM_EN_ADDR, _BITS_(1:1, 1),},
		._[IDX_SP_VRF18_0_SHUTDOWN] = {MT6328_PMIC_RG_VRF18_0_EN_ADDR, _BITS_(1:1, 0),},
		.nr_idx = NR_IDX_SP,
	},
	.set[PMIC_WRAP_PHASE_DEEPIDLE] = {
		._[IDX_DI_VPROC_NORMAL] = {MT6328_PMIC_VPROC_VOSEL_CTRL_ADDR, _BITS_(1:1, 1),},
		._[IDX_DI_VPROC_SLEEP] = {MT6328_PMIC_VPROC_VOSEL_CTRL_ADDR, _BITS_(1:1, 0),},
#ifdef CONFIG_ARCH_MT6753
		._[IDX_DI_VCORE_HPM] = {MT6328_PMIC_VCORE1_VOSEL_ON_ADDR, VOLT_TO_PMIC_VAL(125000),},
		._[IDX_DI_VCORE_TRANS2] = {MT6328_PMIC_VCORE1_VOSEL_ON_ADDR, VOLT_TO_PMIC_VAL(121250),},
		._[IDX_DI_VCORE_TRANS1] = {MT6328_PMIC_VCORE1_VOSEL_ON_ADDR, VOLT_TO_PMIC_VAL(118125),},
		._[IDX_DI_VCORE_LPM] = {MT6328_PMIC_VCORE1_VOSEL_ON_ADDR, VOLT_TO_PMIC_VAL(115000),},
		._[IDX_DI_VSRAM_SLEEP] = {MT6328_PMIC_VSRAM_VOSEL_OFFSET_ADDR, _BITS_(15:0, 0x10),},
		._[IDX_DI_VRF18_0_PWR_ON] = {MT6328_PMIC_RG_VRF18_0_EN_ADDR, _BITS_(1:1, 1),},
		/* We will get actual value at DVFS driver init procedure */
		._[IDX_DI_VSRAM_NORMAL] = {MT6328_PMIC_VSRAM_VOSEL_OFFSET_ADDR, _BITS_(15:0, 0x10),},
		._[IDX_DI_VRF18_0_SHUTDOWN] = {MT6328_PMIC_RG_VRF18_0_EN_ADDR, _BITS_(1:1, 0),},
		._[IDX_DI_VCORE_IDLE_LPM] = {MT6328_PMIC_VCORE1_VOSEL_ON_ADDR, VOLT_TO_PMIC_VAL(105000),},
		._[IDX_DI_VSRAM_SLEEP_FOR_TURBO] = {MT6328_PMIC_VSRAM_VOSEL_CTRL_ADDR, _BITS_(1:1, 1),},
		._[IDX_DI_VSRAM_NORMAL_FOR_TURBO] = {MT6328_PMIC_VSRAM_VOSEL_CTRL_ADDR, _BITS_(1:1, 0),},
#else	/* 6735(M) */
		._[IDX_DI_VCORE_HPM] = {MT6328_PMIC_VCORE1_VOSEL_ON_ADDR, VOLT_TO_PMIC_VAL(115000),},
		._[IDX_DI_VCORE_TRANS2] = {MT6328_PMIC_VCORE1_VOSEL_ON_ADDR, VOLT_TO_PMIC_VAL(111250),},
		._[IDX_DI_VCORE_TRANS1] = {MT6328_PMIC_VCORE1_VOSEL_ON_ADDR, VOLT_TO_PMIC_VAL(108125),},
		._[IDX_DI_VCORE_LPM] = {MT6328_PMIC_VCORE1_VOSEL_ON_ADDR, VOLT_TO_PMIC_VAL(105000),},
		._[IDX_DI_VSRAM_SLEEP] = {MT6328_PMIC_VSRAM_VOSEL_OFFSET_ADDR, _BITS_(15:0, 0x10),},
		._[IDX_DI_VRF18_0_PWR_ON] = {MT6328_PMIC_RG_VRF18_0_EN_ADDR, _BITS_(1:1, 1),},
		._[IDX_DI_VSRAM_NORMAL] = {MT6328_PMIC_VSRAM_VOSEL_CTRL_ADDR, _BITS_(1:1, 1),},
		._[IDX_DI_VRF18_0_SHUTDOWN] = {MT6328_PMIC_RG_VRF18_0_EN_ADDR, _BITS_(1:1, 0),},
#endif
		.nr_idx = NR_IDX_DI,
	},
};

/* DVFS operation and structure */
static struct mt_cpu_dvfs_ops dvfs_ops = {
	.setup_power_table = _mt_cpufreq_setup_power_table,
	.get_cur_phy_freq = _mt_cpufreq_get_cur_phy_freq,
	.set_cur_freq = _mt_cpufreq_set_cur_freq,
	.get_cur_volt = _mt_cpufreq_get_cur_volt,
	.set_cur_volt = _mt_cpufreq_set_cur_volt,
};

#ifdef CONFIG_CPU_DVFS_HAS_EXTBUCK
static struct mt_cpu_dvfs_ops dvfs_ops_extbuck = {
	.setup_power_table = _mt_cpufreq_setup_power_table,
	.get_cur_phy_freq = _mt_cpufreq_get_cur_phy_freq,
	.set_cur_freq = _mt_cpufreq_set_cur_freq,
	.get_cur_volt = _mt_cpufreq_get_cur_volt_extbuck,
	.set_cur_volt = _mt_cpufreq_set_cur_volt_extbuck,
};
#endif

static struct mt_cpu_dvfs cpu_dvfs[] = {
	[MT_CPU_DVFS_LITTLE] = {
		.name = __stringify(MT_CPU_DVFS_LITTLE),
		.cpu_id = MT_CPU_DVFS_LITTLE,	/* TODO: FIXME */
		.cpu_level = CPU_LEVEL_1,	/* FY segment */
		.ops = &dvfs_ops,
#ifdef CONFIG_CPU_DVFS_TURBO_MODE
		.turbo_mode = 0,
#endif
#ifdef CONFIG_CPU_DVFS_POWER_THROTTLING
		.idx_opp_tbl_for_pwr_thro = -1,
		.idx_pwr_thro_max_opp = 0,
#endif
	},
};

/* DVFS table */
#ifdef CONFIG_ARCH_MT6753
/* CPU LEVEL 0, 1.5GHz segment */
static struct mt_cpu_freq_info opp_tbl_e1_0[] = {
	OP(CPU_DVFS_FREQ0, 125000),
	OP(CPU_DVFS_FREQ1, 121250),
	OP(CPU_DVFS_FREQ3, 118125),
	OP(CPU_DVFS_FREQ4, 115000),
	OP(CPU_DVFS_FREQ5, 108750),
	OP(CPU_DVFS_FREQ6, 103125),
	OP(CPU_DVFS_FREQ7, 99375),
	OP(CPU_DVFS_FREQ8, 95000),
};

/* CPU LEVEL 1, 1.3GHz segment */
static struct mt_cpu_freq_info opp_tbl_e1_1[] = {
	OP(CPU_DVFS_FREQ1, 125000),
	OP(CPU_DVFS_FREQ2, 123125),
	OP(CPU_DVFS_FREQ3_1, 120000),
	OP(CPU_DVFS_FREQ4, 115000),
	OP(CPU_DVFS_FREQ5, 110000),
	OP(CPU_DVFS_FREQ6, 105000),
	OP(CPU_DVFS_FREQ7, 100000),
	OP(CPU_DVFS_FREQ8, 95000),
};

static struct opp_tbl_info opp_tbls[] = {
	[CPU_LEVEL_0] = {opp_tbl_e1_0, ARRAY_SIZE(opp_tbl_e1_0)},
	[CPU_LEVEL_1] = {opp_tbl_e1_1, ARRAY_SIZE(opp_tbl_e1_1)},
};
#elif defined(CONFIG_ARCH_MT6735M)
/* CPU LEVEL 0, 1.2GHz segment */
static struct mt_cpu_freq_info opp_tbl_e1_0[] = {
	OP(CPU_DVFS_FREQ0, 125000),
	OP(CPU_DVFS_FREQ1, 121875),
	OP(CPU_DVFS_FREQ4, 118750),
	OP(CPU_DVFS_FREQ6, 115000),
	OP(CPU_DVFS_FREQ7, 110000),
	OP(CPU_DVFS_FREQ8, 105000),
	OP(CPU_DVFS_FREQ9, 100000),
	OP(CPU_DVFS_FREQ10, 95000),
};

/* CPU LEVEL 1, 1GHz segment */
static struct mt_cpu_freq_info opp_tbl_e1_1[] = {
	OP(CPU_DVFS_FREQ2, 125000),
	OP(CPU_DVFS_FREQ3, 121875),
	OP(CPU_DVFS_FREQ5, 118750),
	OP(CPU_DVFS_FREQ6, 115000),
	OP(CPU_DVFS_FREQ7, 110000),
	OP(CPU_DVFS_FREQ8, 105000),
	OP(CPU_DVFS_FREQ9, 100000),
	OP(CPU_DVFS_FREQ10, 95000),
};

/* CPU LEVEL 2, 1.25GHz segment */
static struct mt_cpu_freq_info opp_tbl_e1_2[] = {
	OP(CPU_DVFS_FREQ0_1, 125000),
	OP(CPU_DVFS_FREQ1,  121875),
	OP(CPU_DVFS_FREQ5,  118750),
	OP(CPU_DVFS_FREQ6,  115000),
	OP(CPU_DVFS_FREQ7,  110000),
	OP(CPU_DVFS_FREQ8,  105000),
	OP(CPU_DVFS_FREQ9,  100000),
	OP(CPU_DVFS_FREQ10, 95000),
};

/* CPU LEVEL 3, 1.1GHz segment */
static struct mt_cpu_freq_info opp_tbl_e1_3[] = {
	OP(CPU_DVFS_FREQ1_1, 125000),
	OP(CPU_DVFS_FREQ2,  121875),
	OP(CPU_DVFS_FREQ5,  118750),
	OP(CPU_DVFS_FREQ6,  115000),
	OP(CPU_DVFS_FREQ7,  110000),
	OP(CPU_DVFS_FREQ8,  105000),
	OP(CPU_DVFS_FREQ9,  100000),
	OP(CPU_DVFS_FREQ10, 95000),
};

static struct opp_tbl_info opp_tbls[] = {
	[CPU_LEVEL_0] = {opp_tbl_e1_0, ARRAY_SIZE(opp_tbl_e1_0)},
	[CPU_LEVEL_1] = {opp_tbl_e1_1, ARRAY_SIZE(opp_tbl_e1_1)},
	[CPU_LEVEL_2] = {opp_tbl_e1_2, ARRAY_SIZE(opp_tbl_e1_2)},
	[CPU_LEVEL_3] = {opp_tbl_e1_3, ARRAY_SIZE(opp_tbl_e1_3)},
};
#else
/* CPU LEVEL 0, 1.5GHz segment */
static struct mt_cpu_freq_info opp_tbl_e1_0[] = {
	OP(CPU_DVFS_FREQ0, 125000),
	OP(CPU_DVFS_FREQ1, 121250),
	OP(CPU_DVFS_FREQ3, 118125),
	OP(CPU_DVFS_FREQ4, 115000),
	OP(CPU_DVFS_FREQ5, 108750),
	OP(CPU_DVFS_FREQ6, 103125),
	OP(CPU_DVFS_FREQ7, 99375),
	OP(CPU_DVFS_FREQ8, 95000),
};

/* CPU LEVEL 1, 1.3GHz segment */
static struct mt_cpu_freq_info opp_tbl_e1_1[] = {
	OP(CPU_DVFS_FREQ1, 125000),
	OP(CPU_DVFS_FREQ2, 123125),
	OP(CPU_DVFS_FREQ3, 120625),
	OP(CPU_DVFS_FREQ4, 115000),
	OP(CPU_DVFS_FREQ5, 110000),
	OP(CPU_DVFS_FREQ6, 105000),
	OP(CPU_DVFS_FREQ7, 100000),
	OP(CPU_DVFS_FREQ8, 95000),
};

/* CPU LEVEL 2, 1.1GHz segment */
static struct mt_cpu_freq_info opp_tbl_e1_2[] = {
	OP(CPU_DVFS_FREQ4, 115000),
	OP(CPU_DVFS_FREQ5, 110000),
	OP(CPU_DVFS_FREQ6, 105000),
	OP(CPU_DVFS_FREQ7, 100000),
	OP(CPU_DVFS_FREQ8, 95000),
	OP(CPU_DVFS_FREQ8, 95000),
	OP(CPU_DVFS_FREQ8, 95000),
	OP(CPU_DVFS_FREQ8, 95000),
};

/* CPU LEVEL 3, 1.45GHz segment */
static struct mt_cpu_freq_info opp_tbl_e1_3[] = {
	OP(CPU_DVFS_FREQ0_1, 125000),
	OP(CPU_DVFS_FREQ1,  123125),
	OP(CPU_DVFS_FREQ3,  120625),
	OP(CPU_DVFS_FREQ4,  115000),
	OP(CPU_DVFS_FREQ5,  110000),
	OP(CPU_DVFS_FREQ6,  105000),
	OP(CPU_DVFS_FREQ7,  100000),
	OP(CPU_DVFS_FREQ8,  95000),
};

/* CPU LEVEL 4, 1.1GHz segment (for 37T to 35M+)*/
static struct mt_cpu_freq_info opp_tbl_e1_4[] = {
	OP(CPU_DVFS_FREQ3_2,  125000),
	OP(CPU_DVFS_FREQ4_1,  123125),
	OP(CPU_DVFS_FREQ4_2,  120625),
	OP(CPU_DVFS_FREQ5_1,  115000),
	OP(CPU_DVFS_FREQ5_2,  110000),
	OP(CPU_DVFS_FREQ6_1,  105000),
	OP(CPU_DVFS_FREQ7_1,  100000),
	OP(CPU_DVFS_FREQ8_1,  95000),
};
static struct opp_tbl_info opp_tbls[] = {
	[CPU_LEVEL_0] = {opp_tbl_e1_0, ARRAY_SIZE(opp_tbl_e1_0)},
	[CPU_LEVEL_1] = {opp_tbl_e1_1, ARRAY_SIZE(opp_tbl_e1_1)},
	[CPU_LEVEL_2] = {opp_tbl_e1_2, ARRAY_SIZE(opp_tbl_e1_2)},
	[CPU_LEVEL_3] = {opp_tbl_e1_3, ARRAY_SIZE(opp_tbl_e1_3)},
	[CPU_LEVEL_4] = {opp_tbl_e1_4, ARRAY_SIZE(opp_tbl_e1_4)},
};
#endif

/* CPU on/off callback */
#if defined(CONFIG_ARCH_MT6753) || defined(CONFIG_CPU_DVFS_TURBO_MODE)
static unsigned int num_online_cpus_delta;
#endif
static struct notifier_block __refdata _mt_cpufreq_cpu_notifier = {
	.notifier_call = _mt_cpufreq_cpu_CB,
};

/* TODO: waiting for HPT to provide turbo settings */
#ifdef CONFIG_CPU_DVFS_TURBO_MODE
struct turbo_mode_cfg turbo_mode_cfg[] = {
	[TURBO_MODE_2] = {
		.temp = 65000,
		.freq_delta = 10,
		.volt_delta = 4000,
	},
	[TURBO_MODE_1] = {
		.temp = 85000,
		.freq_delta = 5,
		.volt_delta = 2000,
	},
	[TURBO_MODE_NONE] = {
		.temp = 125000,
		.freq_delta = 0,
		.volt_delta = 0,
	},
};

static bool is_in_turbo_mode;
#endif

#ifdef CONFIG_CPU_FREQ
static struct freq_attr *_mt_cpufreq_attr[] = {
	&cpufreq_freq_attr_scaling_available_freqs,
	NULL,
};

static struct cpufreq_driver _mt_cpufreq_driver = {
	.flags = CPUFREQ_ASYNC_NOTIFICATION,
	.verify = _mt_cpufreq_verify,
	.target = _mt_cpufreq_target,
	.init = _mt_cpufreq_init,
	.get = _mt_cpufreq_get,
	.name = "mt-cpufreq",
	.attr = _mt_cpufreq_attr,
};
#endif


/* (early-)suspend / (late-)resume */
static struct notifier_block _mt_cpufreq_fb_notifier = {
	.notifier_call = _mt_cpufreq_fb_notifier_callback,
};

static const struct dev_pm_ops _mt_cpufreq_pm_ops = {
	.suspend = _mt_cpufreq_suspend,
	.resume = _mt_cpufreq_resume,
	.restore_early = _mt_cpufreq_pm_restore_early,
	.freeze = _mt_cpufreq_suspend,
	.thaw = _mt_cpufreq_resume,
	.restore = _mt_cpufreq_resume,
};

static bool _allow_dpidle_ctrl_vproc;
static unsigned int is_fix_freq_in_ES = 1;


/* Platform driver */
static struct platform_device _mt_cpufreq_pdev = {
	.name = "mt-cpufreq",
	.id = -1,
};

static struct platform_driver _mt_cpufreq_pdrv = {
	.probe = _mt_cpufreq_pdrv_probe,
	.remove = _mt_cpufreq_pdrv_remove,
	.driver = {
		.name = "mt-cpufreq",
		.pm = &_mt_cpufreq_pm_ops,
		.owner = THIS_MODULE,
	},
};

/* Debugging */
/* static unsigned int func_lv_mask =
	(FUNC_LV_MODULE | FUNC_LV_CPUFREQ | FUNC_LV_API | FUNC_LV_LOCAL | FUNC_LV_HELP); */
static unsigned int func_lv_mask;
static unsigned int do_dvfs_stress_test;

/* Lock */
static DEFINE_MUTEX(cpufreq_mutex);
/* static DEFINE_MUTEX(pmic_wrap_mutex); */
bool is_in_cpufreq = 0;		/* used in MCDI */

/* cpu voltage sampler */
static cpuVoltsampler_func g_pCpuVoltSampler;

/* for PMIC 5A throttle */
#ifdef CONFIG_ARCH_MT6753
static bool pmic_5A_throttle_enable;
static bool pmic_5A_throttle_on;
#endif

/*=============================================================*/
/* Function Implementation									 */
/*=============================================================*/

/* weak functions */
void __attribute__ ((weak))
register_battery_oc_notify(void (*battery_oc_callback) (BATTERY_OC_LEVEL), BATTERY_OC_PRIO prio_val)
{
	cpufreq_err("%s doesn't exist\n", __func__);
}

void __attribute__ ((weak))
register_battery_percent_notify(void (*battery_percent_callback) (BATTERY_PERCENT_LEVEL),
		BATTERY_PERCENT_PRIO prio_val)
{
	cpufreq_err("%s doesn't exist\n", __func__);
}

#ifdef CONFIG_ARCH_MT6753
static bool is_need_5A_throttle(struct mt_cpu_dvfs *p, unsigned int cur_freq, unsigned int cur_core_num)
{
	if (pmic_5A_throttle_enable && pmic_5A_throttle_on
		&& (cur_core_num > PMIC_5A_THRO_MAX_CPU_CORE_NUM)
		&& (cur_freq > PMIC_5A_THRO_MAX_CPU_FREQ))
		return true;

	return false;
}
#endif

static struct mt_cpu_dvfs *id_to_cpu_dvfs(enum mt_cpu_dvfs_id id)
{
	return (id < NR_MT_CPU_DVFS) ? &cpu_dvfs[id] : NULL;
}

static enum mt_cpu_dvfs_id _get_cpu_dvfs_id(unsigned int cpu_id)
{
	return MT_CPU_DVFS_LITTLE;
}

#ifdef __KERNEL__
static unsigned int _mt_cpufreq_get_cpu_level(void)
{
	unsigned int lv = 0;
	unsigned int cpu_spd_bond = _GET_BITS_VAL_(2:0, get_devinfo_with_index(CPUFREQ_EFUSE_INDEX));
	unsigned int cpu_speed = 0;
#ifdef CONFIG_OF
	struct device_node *node = of_find_node_by_type(NULL, "cpu");

	if (!of_property_read_u32(node, "clock-frequency", &cpu_speed))
		cpu_speed = cpu_speed / 1000 / 1000;	/* MHz */
	else {
		cpufreq_err("@%s: missing clock-frequency property, use default CPU level\n", __func__);
		return CPU_LEVEL_1;
	}
	cpufreq_info("CPU clock-frequency from DT = %d MHz\n", cpu_speed);
#endif

	cpufreq_info("@%s: efuse cpu_spd_bond = 0x%x\n", __func__, cpu_spd_bond);

#ifdef CONFIG_ARCH_MT6753
	{
		unsigned int efuse_spare2 = _GET_BITS_VAL_(21:20, get_devinfo_with_index(5));

		cpufreq_info("@%s: efuse_spare2 = 0x%x\n", __func__, efuse_spare2);

		/* 6753T check, spare2[21:20] should be 0x3 */
		if (cpu_spd_bond == 0 && efuse_spare2 == 3)
			return CPU_LEVEL_0;
	}
#else
	{
		unsigned int segment_code = _GET_BITS_VAL_(31 : 25, get_devinfo_with_index(47));

		cpufreq_info("@%s: segment_code = 0x%x\n", __func__, segment_code);

#if defined(CONFIG_ARCH_MT6735) && defined(CONFIG_MTK_EFUSE_DOWNGRADE)
		return CPU_LEVEL_4;	/* SW config 37T to 35M+ */
#endif

		switch (segment_code) {
		case 0x41:
		case 0x42:
		case 0x43:
			return CPU_LEVEL_3;	/* 37T: 1.45G */
		case 0x49:
			return CPU_LEVEL_4;	/* 37M: 1.1G */
		case 0x4A:
		case 0x4B:
			return CPU_LEVEL_3;	/* 35M+: 1.1G */
		case 0x51:
			return CPU_LEVEL_1;	/* 37: 1.3G */
		case 0x52:
		case 0x53:
#ifdef CONFIG_MTK_EFUSE_DOWNGRADE
			return CPU_LEVEL_3;	/* SW config to 35M+ */
#else
			return CPU_LEVEL_2;	/* 35P+ 1.25G */
#endif
		default:
			break;
		}
	}
#endif

	/* No efuse,  use clock-frequency from device tree to determine CPU table type! */
	if (cpu_spd_bond == 0) {
#ifdef CONFIG_ARCH_MT6753
		if (cpu_speed >= 1500 && cpu_dvfs_is_extbuck_valid())
			lv = CPU_LEVEL_0;	/* 1.5G */
		else if (cpu_speed >= 1300)
			lv = CPU_LEVEL_1;	/* 1.3G */
		else {
			cpufreq_err("No suitable DVFS table, set to default CPU level! clock-frequency=%d\n",
					cpu_speed);
			lv = CPU_LEVEL_1;
		}
#elif defined(CONFIG_ARCH_MT6735M)
		if (cpu_speed >= 1150)
			lv = CPU_LEVEL_0;	/* 1.15G */
		else if (cpu_speed >= 1000)
			lv = CPU_LEVEL_1;	/* 1G */
		else {
			cpufreq_err("No suitable DVFS table, set to default CPU level! clock-frequency=%d\n",
					cpu_speed);
			lv = CPU_LEVEL_1;
		}
#else	/* CONFIG_ARCH_MT6735 */
		if (cpu_speed >= 1500)
			lv = CPU_LEVEL_0;	/* 1.5G */
		else if (cpu_speed >= 1300)
			lv = CPU_LEVEL_1;	/* 1.3G */
		else if (cpu_speed >= 1100)
			lv = CPU_LEVEL_2;	/* 1.1G */
		else {
			cpufreq_err("No suitable DVFS table, set to default CPU level! clock-frequency=%d\n",
					cpu_speed);
			lv = CPU_LEVEL_1;
		}
#endif
		return lv;
	}

	/* no DT, we should check efuse for CPU speed HW bounding */
	switch (cpu_spd_bond) {
#ifdef CONFIG_ARCH_MT6735M
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		lv = CPU_LEVEL_0;	/* 1.15G */
		break;
	case 6:
	case 7:
	default:
		lv = CPU_LEVEL_1;	/* 1G */
		break;
#else	/* !CONFIG_ARCH_MT6735M */
	case 1:
	case 2:
		lv = CPU_LEVEL_0;	/* 1.5G */
		break;
	case 3:
	case 4:
		lv = CPU_LEVEL_1;	/* 1.3G */
		break;
	case 5:
	case 6:
	case 7:
#ifdef CONFIG_ARCH_MT6735
		lv = CPU_LEVEL_2;	/* 1.1G */
		break;
#endif
	default:
		lv = CPU_LEVEL_1;	/* 1.3G */
		break;
#endif
	}

	return lv;
}
#else
static unsigned int _mt_cpufreq_get_cpu_level(void)
{
#ifdef __MTK_SLT_
	CHIP_TYPE chip_type = mt_get_chip_type_by_efuse();
	unsigned int lv = 0;

	switch (chip_type) {
	case CHIP_MT6735:
		lv = CPU_LEVEL_1;	/* D1 1.3G */
		break;
	case CHIP_MT6735M:
	case CHIP_MT6735P:
#ifdef CONFIG_ARCH_MT6735M
		lv = CPU_LEVEL_1;	/* D2 1.1G */
#else
		lv = CPU_LEVEL_2;	/* D1 1.1G */
#endif
		break;
	case CHIP_NONE:
	default:
		lv = CPU_LEVEL_1;	/* FY table */
		break;
	}

	cpufreq_info("@%s: chip_type = 0x%x, cpu_lv = %d\n", __func__, chip_type, lv);

	return lv;
#else
	return CPU_LEVEL_1;	/* always use FY table for DVT */
#endif
}
#endif


#ifdef CONFIG_CPU_DVFS_AEE_RR_REC
static void _mt_cpufreq_aee_init(void)
{
	aee_rr_rec_cpu_dvfs_vproc_big(0xFF);
	aee_rr_rec_cpu_dvfs_vproc_little(0xFF);
	aee_rr_rec_cpu_dvfs_oppidx(0xFF);
	aee_rr_rec_cpu_dvfs_status(0xFC);
}
#endif

static int _mt_cpufreq_set_spm_dvfs_ctrl_volt(u32 value)
{
#define MAX_RETRY_COUNT (100)

	u32 ap_dvfs_con;
	int retry = 0;

	FUNC_ENTER(FUNC_LV_HELP);

	spm_write(SPM_POWERON_CONFIG_SET, (SPM_PROJECT_CODE << 16) | (1U << 0));

	ap_dvfs_con = spm_read(SPM_AP_DVFS_CON_SET);
#ifdef CONFIG_ARCH_MT6753
	/* 6753 has 16 PWRAP commands */
	spm_write(SPM_AP_DVFS_CON_SET, (ap_dvfs_con & ~(0xF)) | value);
#else
	spm_write(SPM_AP_DVFS_CON_SET, (ap_dvfs_con & ~(0x7)) | value);
#endif
	udelay(5);

	while ((spm_read(SPM_AP_DVFS_CON_SET) & (0x1 << 31)) == 0) {
		if (retry >= MAX_RETRY_COUNT) {
			cpufreq_err("@%s:  SPM write fail!\n", __func__);
			return -1;
		}

		retry++;
		/* cpufreq_dbg("wait for ACK signal from PMIC wrapper, retry = %d\n", retry); */

		udelay(5);
	}

	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

static void _mt_cpufreq_pmic_table_init(void)
{
	struct pmic_wrap_cmd pwrap_cmd_default[NR_PMIC_WRAP_CMD] = {
		{(unsigned long)PMIC_WRAP_DVFS_ADR0, (unsigned long)PMIC_WRAP_DVFS_WDATA0,},
		{(unsigned long)PMIC_WRAP_DVFS_ADR1, (unsigned long)PMIC_WRAP_DVFS_WDATA1,},
		{(unsigned long)PMIC_WRAP_DVFS_ADR2, (unsigned long)PMIC_WRAP_DVFS_WDATA2,},
		{(unsigned long)PMIC_WRAP_DVFS_ADR3, (unsigned long)PMIC_WRAP_DVFS_WDATA3,},
		{(unsigned long)PMIC_WRAP_DVFS_ADR4, (unsigned long)PMIC_WRAP_DVFS_WDATA4,},
		{(unsigned long)PMIC_WRAP_DVFS_ADR5, (unsigned long)PMIC_WRAP_DVFS_WDATA5,},
		{(unsigned long)PMIC_WRAP_DVFS_ADR6, (unsigned long)PMIC_WRAP_DVFS_WDATA6,},
		{(unsigned long)PMIC_WRAP_DVFS_ADR7, (unsigned long)PMIC_WRAP_DVFS_WDATA7,},
		{(unsigned long)PMIC_WRAP_DVFS_ADR8, (unsigned long)PMIC_WRAP_DVFS_WDATA8,},
		{(unsigned long)PMIC_WRAP_DVFS_ADR9, (unsigned long)PMIC_WRAP_DVFS_WDATA9,},
		{(unsigned long)PMIC_WRAP_DVFS_ADR10, (unsigned long)PMIC_WRAP_DVFS_WDATA10,},
		{(unsigned long)PMIC_WRAP_DVFS_ADR11, (unsigned long)PMIC_WRAP_DVFS_WDATA11,},
		{(unsigned long)PMIC_WRAP_DVFS_ADR12, (unsigned long)PMIC_WRAP_DVFS_WDATA12,},
		{(unsigned long)PMIC_WRAP_DVFS_ADR13, (unsigned long)PMIC_WRAP_DVFS_WDATA13,},
		{(unsigned long)PMIC_WRAP_DVFS_ADR14, (unsigned long)PMIC_WRAP_DVFS_WDATA14,},
		{(unsigned long)PMIC_WRAP_DVFS_ADR15, (unsigned long)PMIC_WRAP_DVFS_WDATA15,},
	};

	FUNC_ENTER(FUNC_LV_HELP);

	memcpy(pw.addr, pwrap_cmd_default, sizeof(pwrap_cmd_default));

	FUNC_EXIT(FUNC_LV_HELP);
}

void mt_cpufreq_set_pmic_phase(enum pmic_wrap_phase_id phase)
{
	int i;
	unsigned long flags;

	FUNC_ENTER(FUNC_LV_API);

	BUG_ON(phase >= NR_PMIC_WRAP_PHASE);


	if (pw.addr[0].cmd_addr == 0) {
		cpufreq_warn("pmic table not initialized\n");
		_mt_cpufreq_pmic_table_init();
	}

	pmic_wrap_lock(flags);

	pw.phase = phase;

	for (i = 0; i < pw.set[phase].nr_idx; i++) {
		cpufreq_write(pw.addr[i].cmd_addr, pw.set[phase]._[i].cmd_addr);
		cpufreq_write(pw.addr[i].cmd_wdata, pw.set[phase]._[i].cmd_wdata);
	}

	pmic_wrap_unlock(flags);

	FUNC_EXIT(FUNC_LV_API);
}
EXPORT_SYMBOL(mt_cpufreq_set_pmic_phase);

/* just set wdata value */
void mt_cpufreq_set_pmic_cmd(enum pmic_wrap_phase_id phase, int idx, unsigned int cmd_wdata)
{
	unsigned long flags;

	FUNC_ENTER(FUNC_LV_API);

	BUG_ON(phase >= NR_PMIC_WRAP_PHASE);
	BUG_ON(idx >= pw.set[phase].nr_idx);

	/* cpufreq_dbg("@%s: phase = 0x%x, idx = %d, cmd_wdata = 0x%x\n", __func__, phase, idx, cmd_wdata); */

	pmic_wrap_lock(flags);

	pw.set[phase]._[idx].cmd_wdata = cmd_wdata;

	if (pw.phase == phase)
		cpufreq_write(pw.addr[idx].cmd_wdata, cmd_wdata);

	pmic_wrap_unlock(flags);

	FUNC_EXIT(FUNC_LV_API);
}
EXPORT_SYMBOL(mt_cpufreq_set_pmic_cmd);

/* kick spm */
void mt_cpufreq_apply_pmic_cmd(int idx)
{
	unsigned long flags;

	FUNC_ENTER(FUNC_LV_API);

	BUG_ON(idx >= pw.set[pw.phase].nr_idx);

	/* cpufreq_dbg("@%s: idx = %d\n", __func__, idx); */

	pmic_wrap_lock(flags);

	_mt_cpufreq_set_spm_dvfs_ctrl_volt(idx);

	pmic_wrap_unlock(flags);

	FUNC_EXIT(FUNC_LV_API);
}
EXPORT_SYMBOL(mt_cpufreq_apply_pmic_cmd);

void mt_cpufreq_setvolt_registerCB(cpuVoltsampler_func pCB)
{
	g_pCpuVoltSampler = pCB;
}
EXPORT_SYMBOL(mt_cpufreq_setvolt_registerCB);


#ifdef CONFIG_CPU_DVFS_TURBO_MODE
static enum turbo_mode _mt_cpufreq_get_turbo_mode(struct mt_cpu_dvfs *p, unsigned int target_khz)
{
	enum turbo_mode mode = TURBO_MODE_NONE;
#ifdef CONFIG_THERMAL
	int temp = tscpu_get_temp_by_bank(THERMAL_BANK0);	/* bank0 for CPU */
#else
	int temp = 40;
#endif
	unsigned int online_cpus = num_online_cpus() + num_online_cpus_delta;
	int i;

	if (p->turbo_mode && target_khz == cpu_dvfs_get_freq_by_idx(p, 0)
		&& online_cpus <= TURBO_MODE_BOUNDARY_CPU_NUM) {
		for (i = 0; i < NR_TURBO_MODE; i++) {
			if (temp < turbo_mode_cfg[i].temp) {
				mode = i;
				break;
			}
		}
	}
	/* enter turbo mode, SW workaround here */
	if (mode < TURBO_MODE_NONE && is_in_turbo_mode == false) {
		is_in_turbo_mode = true;
#ifdef CONFIG_CPU_DVFS_AEE_RR_REC
		aee_rr_rec_cpu_dvfs_status(aee_rr_curr_cpu_dvfs_status() | (1 << CPU_DVFS_LITTLE_IS_TURBO));
#endif
	}

	cpufreq_ver("%s(), mode = %d, temp = %d, target_khz = %d (%d), num_online_cpus = %d\n",
			__func__,
			mode,
			temp,
			target_khz,
			TURBO_MODE_FREQ(mode, target_khz),
			online_cpus
	);

	return mode;
}
#endif


/* for PTP-OD */
static int _mt_cpufreq_set_cur_volt_locked(struct mt_cpu_dvfs *p, unsigned int volt)	/* volt: mv * 100 */
{
	int ret = -1;

	FUNC_ENTER(FUNC_LV_HELP);

	BUG_ON(NULL == p);

	if (!cpu_dvfs_is_available(p)) {
		FUNC_EXIT(FUNC_LV_HELP);
		return 0;
	}

	/* set volt */
	ret = p->ops->set_cur_volt(p, volt);

	FUNC_EXIT(FUNC_LV_HELP);

	return ret;
}

static int _mt_cpufreq_restore_default_volt(struct mt_cpu_dvfs *p)
{
	unsigned long flags;
	int i;
	int ret = -1;

	FUNC_ENTER(FUNC_LV_HELP);

	BUG_ON(NULL == p);

	if (!cpu_dvfs_is_available(p)) {
		FUNC_EXIT(FUNC_LV_HELP);
		return 0;
	}

	cpufreq_lock(flags);

	/* restore to default volt */
	for (i = 0; i < p->nr_opp_tbl; i++)
		p->opp_tbl[i].cpufreq_volt = p->opp_tbl[i].cpufreq_volt_org;

	/* set volt */
#ifdef CONFIG_CPU_DVFS_TURBO_MODE
	ret = _mt_cpufreq_set_cur_volt_locked(
		p,
		TURBO_MODE_VOLT(
			_mt_cpufreq_get_turbo_mode
			(p, cpu_dvfs_get_cur_freq(p)),
			cpu_dvfs_get_cur_volt(p)
		)
		);
#else
	ret = _mt_cpufreq_set_cur_volt_locked(p, cpu_dvfs_get_cur_volt(p));
#endif

	cpufreq_unlock(flags);

	FUNC_EXIT(FUNC_LV_HELP);

	return ret;
}


static int _mt_cpufreq_set_limit_by_pwr_budget(unsigned int budget)
{
	struct mt_cpu_dvfs *p = id_to_cpu_dvfs(0);	/* TODO: FIXME */
	int possible_cpu = num_possible_cpus();
	int ncpu;
	int i;

	for (ncpu = possible_cpu; ncpu > 0; ncpu--) {
		for (i = 0; i < p->nr_opp_tbl * possible_cpu; i++) {
#ifdef CONFIG_ARCH_MT6753
			if (is_need_5A_throttle(p, p->power_tbl[i].cpufreq_khz,
						p->power_tbl[i].cpufreq_ncpu))
				continue;
#endif
			if (p->power_tbl[i].cpufreq_power <= budget) {
				p->limited_power_idx = i;
				p->limited_max_ncpu = p->power_tbl[i].cpufreq_ncpu;
				p->limited_max_freq = p->power_tbl[i].cpufreq_khz;
				return 1;
			}
		}
	}

	/* not found and use lowest power limit */
	p->limited_power_idx = p->nr_power_tbl - 1;
	p->limited_max_ncpu = p->power_tbl[p->limited_power_idx].cpufreq_ncpu;
	p->limited_max_freq = p->power_tbl[p->limited_power_idx].cpufreq_khz;

	return 0;
}

#ifndef DISABLE_PBM_FEATURE
static unsigned int _mt_cpufreq_get_limited_core_num(unsigned int budget)
{
	struct mt_cpu_dvfs *p = id_to_cpu_dvfs(0);	/* TODO: FIXME */
	int possible_cpu = num_possible_cpus();
	int ncpu;
	int i;

	for (ncpu = possible_cpu; ncpu > 0; ncpu--) {
		for (i = 0; i < p->nr_opp_tbl * possible_cpu; i++) {
#ifdef CONFIG_ARCH_MT6753
			if (is_need_5A_throttle(p, p->power_tbl[i].cpufreq_khz,
						p->power_tbl[i].cpufreq_ncpu))
				continue;
#endif
			if (p->power_tbl[i].cpufreq_power <= budget)
				return p->power_tbl[i].cpufreq_ncpu;
		}
	}

	/* not found */
	return p->power_tbl[p->nr_power_tbl - 1].cpufreq_ncpu;
}

/* for power budget calculate */
static int _mt_cpufreq_get_pwr_idx(struct mt_cpu_dvfs *p, unsigned int freq, unsigned int ncpu)
{
	int possible_cpu = num_possible_cpus();
	int i;

	if (freq > cpu_dvfs_get_max_freq(p))
		freq = cpu_dvfs_get_max_freq(p);	/* skip turbo freq HERE */

	for (i = 0; i < p->nr_opp_tbl * possible_cpu; i++) {
		if (p->power_tbl[i].cpufreq_ncpu == ncpu && p->power_tbl[i].cpufreq_khz == freq)
			return i;
	}

	return -1;	/* not found */
}

static void _kick_PBM_by_cpu(struct mt_cpu_dvfs *p)
{
	unsigned int cur_freq = p->ops->get_cur_phy_freq(p);
	unsigned int cur_volt = p->ops->get_cur_volt(p) / 100;	/* mV */
	unsigned int cur_ncpu = num_online_cpus();
	int idx = _mt_cpufreq_get_pwr_idx(p, cur_freq, cur_ncpu);
	unsigned int limited_power = 0;

	if (idx == -1) {	/* Not found in power table */
		limited_power = p->power_tbl[p->nr_power_tbl - 1].cpufreq_power;
		cpufreq_warn("@%s: Not found in power table! cur_freq = %d, cur_ncpu = %d\n",
				 __func__, cur_freq, cur_ncpu);
	} else {
		limited_power = p->power_tbl[idx].cpufreq_power;
		cpufreq_ver("@%s: cur_freq = %d, cur_volt = %d, cur_ncpu = %d, limited_power = %d\n",
				__func__, cur_freq, cur_volt, cur_ncpu, limited_power);

		kicker_pbm_by_cpu(limited_power, cur_ncpu, cur_volt);
	}
}
#endif

void mt_cpufreq_set_power_limit_by_pbm(unsigned int limited_power)
{
#ifndef DISABLE_PBM_FEATURE
	struct mt_cpu_dvfs *p = id_to_cpu_dvfs(0);	/* TODO: FIXME */
	unsigned long flags;

	FUNC_ENTER(FUNC_LV_API);

	BUG_ON(NULL == p);

	if (!cpu_dvfs_is_available(p)) {
		FUNC_EXIT(FUNC_LV_API);
		return;
	}

	if (p->limited_power_by_thermal != 0 && limited_power > p->limited_power_by_thermal) {
		cpufreq_ver("@%s: power is limited by thermal(%d), ignore budget from PBM(%d)...\n",
				__func__, p->limited_power_by_thermal, limited_power);
		/* we still need to update limited core */
		hps_set_cpu_num_limit(LIMIT_LOW_BATTERY, _mt_cpufreq_get_limited_core_num(limited_power), 0);
		return;
	}

	if (limited_power == p->limited_power_by_pbm) {
		cpufreq_ver("@%s: limited_power(%d mW) not changed, skip it!\n", __func__,
				limited_power);
		return;
	}

	cpufreq_lock(flags);

	p->limited_power_by_pbm = limited_power;

	cpufreq_ver("@%s: limited power from PBM = %d\n", __func__, p->limited_power_by_pbm);

	if (!p->limited_power_by_pbm && !p->limited_power_by_thermal) {
#ifdef CONFIG_ARCH_MT6753
		if (p->cpu_level == CPU_LEVEL_1) {
			/* give a large budget to find the first safe limit combination */
			_mt_cpufreq_set_limit_by_pwr_budget(99999);
		} else {
			p->limited_max_ncpu = num_possible_cpus();
			p->limited_max_freq = cpu_dvfs_get_max_freq(p);
			p->limited_power_idx = 0;
		}
#else
		p->limited_max_ncpu = num_possible_cpus();
		p->limited_max_freq = cpu_dvfs_get_max_freq(p);
		p->limited_power_idx = 0;
#endif
	} else
		_mt_cpufreq_set_limit_by_pwr_budget(p->limited_power_by_pbm);

	if (p->limited_max_ncpu < num_online_cpus())
		cpufreq_dbg("@%s: limited power = %d, limited_max_ncpu = %d, online CPU = %d\n",
				__func__, p->limited_power_by_pbm, p->limited_max_ncpu, num_online_cpus());

	cpufreq_ver("@%s: limited_power_idx = %d, limited_max_freq = %d, limited_max_ncpu = %d\n",
			__func__, p->limited_power_idx, p->limited_max_freq, p->limited_max_ncpu);

	cpufreq_unlock(flags);

	hps_set_cpu_num_limit(LIMIT_LOW_BATTERY, p->limited_max_ncpu, 0);

	/* TODO: Trigger DVFS here? */

	FUNC_EXIT(FUNC_LV_API);
#endif
}

unsigned int mt_cpufreq_get_leakage_mw(enum mt_cpu_dvfs_id id)
{
#ifndef DISABLE_PBM_FEATURE
	struct mt_cpu_dvfs *p = id_to_cpu_dvfs(id);

#ifdef CONFIG_THERMAL
	int temp = tscpu_get_temp_by_bank(THERMAL_BANK0) / 1000;	/* bank0 for CPU */
#else
	int temp = 40;
#endif

	return mt_spower_get_leakage(MT_SPOWER_CPU, p->ops->get_cur_volt(p) / 100, temp);
#else
	return 0;
#endif
}

void mt_cpufreq_set_min_freq(enum mt_cpu_dvfs_id id, unsigned int freq)
{
	struct mt_cpu_dvfs *p = id_to_cpu_dvfs(id);

	FUNC_ENTER(FUNC_LV_API);

	BUG_ON(NULL == p);

	if (!cpu_dvfs_is_available(p)) {
		FUNC_EXIT(FUNC_LV_API);
		return;
	}

	p->limited_min_freq_by_kdriver = freq;

	if ((p->limited_min_freq_by_kdriver != 0)
		&& (p->limited_min_freq_by_kdriver > cpu_dvfs_get_cur_freq(p))) {
		struct cpufreq_policy *policy = cpufreq_cpu_get(p->cpu_id);

		if (policy) {
			cpufreq_driver_target(
				policy, p->limited_min_freq_by_kdriver, CPUFREQ_RELATION_L);
			cpufreq_cpu_put(policy);
		}
	}

	FUNC_EXIT(FUNC_LV_API);
}

void mt_cpufreq_set_max_freq(enum mt_cpu_dvfs_id id, unsigned int freq)
{
	struct mt_cpu_dvfs *p = id_to_cpu_dvfs(id);

	FUNC_ENTER(FUNC_LV_API);

	BUG_ON(NULL == p);

	if (!cpu_dvfs_is_available(p)) {
		FUNC_EXIT(FUNC_LV_API);
		return;
	}

	p->limited_max_freq_by_kdriver = freq;

	if ((p->limited_max_freq_by_kdriver != 0)
		&& (p->limited_max_freq_by_kdriver < cpu_dvfs_get_cur_freq(p))) {
		struct cpufreq_policy *policy = cpufreq_cpu_get(p->cpu_id);

		if (policy) {
			cpufreq_driver_target(
				policy, p->limited_max_freq_by_kdriver, CPUFREQ_RELATION_H);
			cpufreq_cpu_put(policy);
		}
	}

	FUNC_EXIT(FUNC_LV_API);
}

unsigned int mt_cpufreq_get_cur_freq(enum mt_cpu_dvfs_id id)
{
	struct mt_cpu_dvfs *p = id_to_cpu_dvfs(id);

	FUNC_ENTER(FUNC_LV_API);

	BUG_ON(NULL == p);

	if (!cpu_dvfs_is_available(p)) {
		FUNC_EXIT(FUNC_LV_API);
		return 0;
	}

	FUNC_EXIT(FUNC_LV_API);

	return _mt_cpufreq_get_cur_phy_freq(p);
}

unsigned int mt_cpufreq_get_freq_by_idx(enum mt_cpu_dvfs_id id, int idx)
{
	struct mt_cpu_dvfs *p = id_to_cpu_dvfs(id);

	FUNC_ENTER(FUNC_LV_API);

	BUG_ON(NULL == p);

	if (!cpu_dvfs_is_available(p)) {
		FUNC_EXIT(FUNC_LV_API);
		return 0;
	}

	BUG_ON(idx >= p->nr_opp_tbl);

	FUNC_EXIT(FUNC_LV_API);

	return cpu_dvfs_get_freq_by_idx(p, idx);
}
EXPORT_SYMBOL(mt_cpufreq_get_freq_by_idx);

int mt_cpufreq_update_volt(enum mt_cpu_dvfs_id id, unsigned int *volt_tbl, int nr_volt_tbl)
{
	struct mt_cpu_dvfs *p = id_to_cpu_dvfs(id);
	unsigned long flags;
	int i;
	int ret = -1;

	FUNC_ENTER(FUNC_LV_API);

	BUG_ON(NULL == p);

	if (!cpu_dvfs_is_available(p)) {
		FUNC_EXIT(FUNC_LV_API);
		return 0;
	}

	BUG_ON(nr_volt_tbl > p->nr_opp_tbl);

	cpufreq_lock(flags);

	/* update volt table */
	for (i = 0; i < nr_volt_tbl; i++)
		p->opp_tbl[i].cpufreq_volt = PMIC_VAL_TO_VOLT(volt_tbl[i]);

	/* set volt */
#ifdef CONFIG_CPU_DVFS_TURBO_MODE
	ret = _mt_cpufreq_set_cur_volt_locked(
		p,
		TURBO_MODE_VOLT(
			_mt_cpufreq_get_turbo_mode
			(p, cpu_dvfs_get_cur_freq(p)),
			cpu_dvfs_get_cur_volt(p)
		)
		);
#else
	ret = _mt_cpufreq_set_cur_volt_locked(p, cpu_dvfs_get_cur_volt(p));
#endif

	cpufreq_unlock(flags);

	FUNC_EXIT(FUNC_LV_API);

	return ret;
}
EXPORT_SYMBOL(mt_cpufreq_update_volt);

void mt_cpufreq_restore_default_volt(enum mt_cpu_dvfs_id id)
{
	struct mt_cpu_dvfs *p = id_to_cpu_dvfs(id);

	FUNC_ENTER(FUNC_LV_API);

	BUG_ON(NULL == p);

	if (!cpu_dvfs_is_available(p)) {
		FUNC_EXIT(FUNC_LV_API);
		return;
	}

	_mt_cpufreq_restore_default_volt(p);

	FUNC_EXIT(FUNC_LV_API);
}
EXPORT_SYMBOL(mt_cpufreq_restore_default_volt);

static unsigned int _mt_cpufreq_cpu_freq_calc(unsigned int con1, unsigned int ckdiv1)
{
	unsigned int freq = 0;

con1 &= _BITMASK_(26:0);

	if (con1 >= DDS_DIV8_FREQ) {
		freq = DDS_DIV8_FREQ;
		freq = PLL_DIV8_FREQ + (((con1 - freq) / 0x2000) * PLL_FREQ_STEP / 8);
	} else if (con1 >= DDS_DIV4_FREQ) {
		freq = DDS_DIV4_FREQ;
		freq = PLL_DIV4_FREQ + (((con1 - freq) / 0x2000) * PLL_FREQ_STEP / 4);
	} else if (con1 >= DDS_DIV2_FREQ) {
		freq = DDS_DIV2_FREQ;
		freq = PLL_DIV2_FREQ + (((con1 - freq) / 0x2000) * PLL_FREQ_STEP / 2);
	} else if (con1 >= DDS_DIV1_FREQ) {
		freq = DDS_DIV1_FREQ;
		freq = PLL_DIV1_FREQ + (((con1 - freq) / 0x2000) * PLL_FREQ_STEP);
	} else {
		cpufreq_err("@%s: Invalid DDS value = 0x%x\n", __func__, con1);
		BUG();
	}

	FUNC_ENTER(FUNC_LV_HELP);

	switch (ckdiv1) {
	case 9:
		freq = freq * 3 / 4;
		break;

	case 10:
		freq = freq * 2 / 4;
		break;

	case 11:
		freq = freq * 1 / 4;
		break;

	case 17:
		freq = freq * 4 / 5;
		break;

	case 18:
		freq = freq * 3 / 5;
		break;

	case 19:
		freq = freq * 2 / 5;
		break;

	case 20:
		freq = freq * 1 / 5;
		break;

	case 25:
		freq = freq * 5 / 6;
		break;

	case 26:
		freq = freq * 4 / 6;
		break;

	case 27:
		freq = freq * 3 / 6;
		break;

	case 28:
		freq = freq * 2 / 6;
		break;

	case 29:
		freq = freq * 1 / 6;
		break;

	case 8:
	case 16:
	case 24:
	default:
		break;
	}

	FUNC_EXIT(FUNC_LV_HELP);

	return freq;	/* TODO: adjust by ptp level??? */
}

static unsigned int _mt_cpufreq_get_cur_phy_freq(struct mt_cpu_dvfs *p)
{
	unsigned int con1;
	unsigned int ckdiv1;
	unsigned int cur_khz;

	FUNC_ENTER(FUNC_LV_LOCAL);

	BUG_ON(NULL == p);

	con1 = cpufreq_read(ARMPLL_CON1);
	ckdiv1 = cpufreq_read(TOP_CKDIV1);
	ckdiv1 = _GET_BITS_VAL_(4:0, ckdiv1);

	cur_khz = _mt_cpufreq_cpu_freq_calc(con1, ckdiv1);

	cpufreq_ver("@%s: cur_khz = %d, con1 = 0x%x, ckdiv1_val = 0x%x\n",
				__func__, cur_khz, con1, ckdiv1);

	FUNC_EXIT(FUNC_LV_LOCAL);

	return cur_khz;
}

static unsigned int _mt_cpufreq_cpu_dds_calc(unsigned int khz)
{
	unsigned int dds = 0;

	FUNC_ENTER(FUNC_LV_HELP);

	if (khz >= PLL_DIV1_FREQ)
		dds = DDS_DIV1_FREQ + ((khz - PLL_DIV1_FREQ) / PLL_FREQ_STEP) * 0x2000;
	else if (khz >= PLL_DIV2_FREQ)
		dds = DDS_DIV2_FREQ + ((khz - PLL_DIV2_FREQ) * 2 / PLL_FREQ_STEP) * 0x2000;
	else if (khz >= PLL_DIV4_FREQ)
		dds = DDS_DIV4_FREQ + ((khz - PLL_DIV4_FREQ) * 4 / PLL_FREQ_STEP) * 0x2000;
	else if (khz >= PLL_DIV8_FREQ)
		dds = DDS_DIV8_FREQ + ((khz - PLL_DIV8_FREQ) * 8 / PLL_FREQ_STEP) * 0x2000;
	else
		BUG();

	FUNC_EXIT(FUNC_LV_HELP);

	return dds;
}

static void _mt_cpufreq_set_cpu_clk_src(struct mt_cpu_dvfs *p, enum top_ckmuxsel sel)
{
	FUNC_ENTER(FUNC_LV_HELP);

	switch (sel) {
	case TOP_CKMUXSEL_CLKSQ:
	case TOP_CKMUXSEL_ARMPLL:
		cpufreq_write_mask(TOP_CKMUXSEL, 3:2, sel);
		/* disable MAINPLL_CORE_CK_CG_EN */
		cpufreq_write_mask(AP_PLL_CON2, 5:5, 0x0);
		break;
	case TOP_CKMUXSEL_MAINPLL:
		/* enable MAINPLL_CORE_CK_CG_EN */
		cpufreq_write_mask(AP_PLL_CON2, 5:5, 0x1);
		udelay(3);
		cpufreq_write_mask(TOP_CKMUXSEL, 3:2, sel);
		break;
	default:
		BUG();
		break;
	}

	FUNC_EXIT(FUNC_LV_HELP);
}

static enum top_ckmuxsel _mt_cpufreq_get_cpu_clk_src(struct mt_cpu_dvfs *p)
{
	unsigned int val = cpufreq_read(TOP_CKMUXSEL);
	unsigned int mask = _BITMASK_(3:2);

	FUNC_ENTER(FUNC_LV_HELP);

	val = (val & mask) >> 2;

	FUNC_EXIT(FUNC_LV_HELP);

	return val;
}

int mt_cpufreq_set_cpu_clk_src(enum mt_cpu_dvfs_id id, enum top_ckmuxsel sel)
{
	struct mt_cpu_dvfs *p = id_to_cpu_dvfs(id);

	if (!p)
		return -1;

	_mt_cpufreq_set_cpu_clk_src(p, sel);

	return 0;
}

enum top_ckmuxsel mt_cpufreq_get_cpu_clk_src(enum mt_cpu_dvfs_id id)
{
	struct mt_cpu_dvfs *p = id_to_cpu_dvfs(id);

	if (!p)
		return -1;

	return _mt_cpufreq_get_cpu_clk_src(p);
}

static void _mt_cpufreq_dfs_by_set_armpll(struct mt_cpu_dvfs *p, unsigned int dds,
					  unsigned int ckdiv_sel)
{
	unsigned int ckdiv1_val = _GET_BITS_VAL_(4:0, cpufreq_read(TOP_CKDIV1));
	unsigned int ckdiv1_mask = _BITMASK_(4:0);

	/* set ARMPLL and CLKDIV */
#ifdef __KERNEL__
	freqhopping_config(FH_ARM_PLLID, 1599000, 0);	/* disable SSC */
#endif
	_mt_cpufreq_set_cpu_clk_src(p, TOP_CKMUXSEL_MAINPLL);
	cpufreq_write(ARMPLL_CON1, dds | _BIT_(31));	/* CHG */
	udelay(PLL_SETTLE_TIME);
	cpufreq_write(TOP_CKDIV1, (ckdiv1_val & ~ckdiv1_mask) | ckdiv_sel);
	_mt_cpufreq_set_cpu_clk_src(p, TOP_CKMUXSEL_ARMPLL);
#ifdef __KERNEL__
	freqhopping_config(FH_ARM_PLLID, 1599000, 1);	/* enable SSC */
#endif
}

#ifdef CONFIG_ARCH_MT6735M
/*
 * CPU freq scaling (only freqhopping)
 */
static void _mt_cpufreq_set_cur_freq(struct mt_cpu_dvfs *p, unsigned int cur_khz,
					 unsigned int target_khz)
{
	FUNC_ENTER(FUNC_LV_LOCAL);

	if (cur_khz == target_khz)
		return;

	if (p->cpu_level == CPU_LEVEL_1) {
		unsigned int dds = 0;
		unsigned int sel = 0;
		unsigned int ckdiv1_val = _GET_BITS_VAL_(4:0, cpufreq_read(TOP_CKDIV1));
		unsigned int ckdiv1_mask = _BITMASK_(4:0);

		switch (target_khz) {
		case CPU_DVFS_FREQ0:
		case CPU_DVFS_FREQ0_1:
		case CPU_DVFS_FREQ1:
		case CPU_DVFS_FREQ1_1:
		case CPU_DVFS_FREQ2:
		case CPU_DVFS_FREQ3:
		case CPU_DVFS_FREQ4:
		case CPU_DVFS_FREQ5:
		case CPU_DVFS_FREQ6:
		case CPU_DVFS_FREQ7:
			dds = _mt_cpufreq_cpu_dds_calc(target_khz);
			sel = 8;	/* 4/4 */
			break;
		case CPU_DVFS_FREQ8:
		case CPU_DVFS_FREQ9:
			dds = _mt_cpufreq_cpu_dds_calc(target_khz * 2);
			sel = 10;	/* 2/4 */
			break;
		case CPU_DVFS_FREQ10:
			dds = _mt_cpufreq_cpu_dds_calc(target_khz * 4);
			sel = 11;	/* 1/4 */
			break;
		default:
			cpufreq_err("@%s: invalid target freq = %dKHz\n", __func__, target_khz);
			BUG();
		}

		/* ignore postdiv */
		dds &= ~(_BITMASK_(26:24));

		cpufreq_ver("@%s():%d, cur_khz = %d, target_khz = %d, dds = 0x%x, sel = %d\n",
				__func__, __LINE__, cur_khz, target_khz, dds, sel);

		/* posdiv should be 2 */
		/* BUG_ON((dds & _BITMASK_(26 : 24)) != (1 << 24)); */

#if !defined(__KERNEL__)	/* for CTP */
		if (target_khz > cur_khz) {
#if defined(MTKDRV_FREQHOP)
			fhdrv_dvt_dvfs_enable(ARMPLL_ID, dds);
#endif
			cpufreq_write(TOP_CKDIV1, (ckdiv1_val & ~ckdiv1_mask) | sel);
		} else {
			cpufreq_write(TOP_CKDIV1, (ckdiv1_val & ~ckdiv1_mask) | sel);
#if defined(MTKDRV_FREQHOP)
			fhdrv_dvt_dvfs_enable(ARMPLL_ID, dds);
#endif
		}

#else	/* __KERNEL__ */
		if (target_khz > cur_khz) {
			mt_dfs_armpll(FH_ARM_PLLID, dds);
			cpufreq_write(TOP_CKDIV1, (ckdiv1_val & ~ckdiv1_mask) | sel);
		} else {
			cpufreq_write(TOP_CKDIV1, (ckdiv1_val & ~ckdiv1_mask) | sel);
			mt_dfs_armpll(FH_ARM_PLLID, dds);
		}
#endif
	} else {
		unsigned int dds = 0;
		unsigned int is_fhctl_used;
		unsigned int sel = 0;
		unsigned int cur_volt = 0;

		if (((cur_khz < CPUFREQ_BOUNDARY_FOR_FHCTL) && (target_khz > CPUFREQ_BOUNDARY_FOR_FHCTL))
			|| ((target_khz < CPUFREQ_BOUNDARY_FOR_FHCTL) && (cur_khz > CPUFREQ_BOUNDARY_FOR_FHCTL))) {
			_mt_cpufreq_set_cur_freq(p, cur_khz, CPUFREQ_BOUNDARY_FOR_FHCTL);
			cur_khz = CPUFREQ_BOUNDARY_FOR_FHCTL;
		}

		is_fhctl_used = ((target_khz >= CPUFREQ_BOUNDARY_FOR_FHCTL)
				&& (cur_khz >= CPUFREQ_BOUNDARY_FOR_FHCTL)) ? 1 : 0;

		cpufreq_ver("@%s():%d, cur_khz = %d, target_khz = %d, is_fhctl_used = %d\n",
					__func__,
					__LINE__,
					cur_khz,
					target_khz,
					is_fhctl_used
					);

		if (!is_fhctl_used) {
			switch (target_khz) {
			case CPU_DVFS_FREQ1:
			case CPU_DVFS_FREQ2:
			case CPU_DVFS_FREQ3:
			case CPU_DVFS_FREQ4:
			case CPU_DVFS_FREQ5:
			case CPU_DVFS_FREQ6:
			case CPU_DVFS_FREQ7:
				dds = _mt_cpufreq_cpu_dds_calc(target_khz);
				sel = 8;	/* 4/4 */
				break;
			case CPU_DVFS_FREQ8:
			case CPU_DVFS_FREQ9:
				dds = _mt_cpufreq_cpu_dds_calc(target_khz * 2);
				sel = 10;	/* 2/4 */
				break;
			case CPU_DVFS_FREQ10:
				dds = _mt_cpufreq_cpu_dds_calc(target_khz * 4);
				sel = 11;	/* 1/4 */
				break;
			default:
				cpufreq_err("@%s: invalid target freq = %dKHz\n", __func__, target_khz);
				BUG();
			}

			cur_volt = p->ops->get_cur_volt(p);
			if (cur_volt < 125000)
				p->ops->set_cur_volt(p, 125000);
			else
				cur_volt = 0;

			/* set ARMPLL and CLKDIV */
			_mt_cpufreq_dfs_by_set_armpll(p, dds, sel);

			/* restore Vproc */
			if (cur_volt)
				p->ops->set_cur_volt(p, cur_volt);
		} else {
			unsigned int ckdiv1_val = _GET_BITS_VAL_(4 : 0, cpufreq_read(TOP_CKDIV1));

#define IS_CLKDIV_USED(clkdiv)  (((clkdiv < 8) || ((clkdiv % 8) == 0)) ? 0 : 1)

			dds = _mt_cpufreq_cpu_dds_calc(target_khz);
			BUG_ON(dds & _BITMASK_(26 : 24));	 /* should not use posdiv */
			BUG_ON(IS_CLKDIV_USED(ckdiv1_val));   /* should not use clkdiv */

#if !defined(__KERNEL__)
#if defined(MTKDRV_FREQHOP)
			fhdrv_dvt_dvfs_enable(ARMPLL_ID, dds);
#else
			_mt_cpufreq_dfs_by_set_armpll(p, dds, 8);
#endif
	}

#else  /* __KERNEL__ */
#if 1
			mt_dfs_armpll(FH_ARM_PLLID, dds);
#else
			_mt_cpufreq_dfs_by_set_armpll(p, dds, 8);
#endif
#endif /* ! __KERNEL__ */
		}
	}

	FUNC_EXIT(FUNC_LV_LOCAL);
}
#else
/*
 * CPU freq scaling (set ARMPLL + freqhopping)
 *
 * above 1GHz: use freq hopping
 * below 1GHz: set ARMPLL and CLKDIV
 * if cross 1GHz, migrate to 1GHz first.
 *
 */
static void _mt_cpufreq_set_cur_freq(struct mt_cpu_dvfs *p, unsigned int cur_khz,
					 unsigned int target_khz)
{
	unsigned int dds = 0;
	unsigned int is_fhctl_used;
	unsigned int sel = 0;
	unsigned int cur_volt = 0;

	FUNC_ENTER(FUNC_LV_LOCAL);

	if (cur_khz == target_khz)
		return;

	if (((cur_khz < CPUFREQ_BOUNDARY_FOR_FHCTL) && (target_khz > CPUFREQ_BOUNDARY_FOR_FHCTL))
		|| ((target_khz < CPUFREQ_BOUNDARY_FOR_FHCTL)
		&& (cur_khz > CPUFREQ_BOUNDARY_FOR_FHCTL))) {
		_mt_cpufreq_set_cur_freq(p, cur_khz, CPUFREQ_BOUNDARY_FOR_FHCTL);
		cur_khz = CPUFREQ_BOUNDARY_FOR_FHCTL;
	}

	is_fhctl_used = ((target_khz >= CPUFREQ_BOUNDARY_FOR_FHCTL)
			 && (cur_khz >= CPUFREQ_BOUNDARY_FOR_FHCTL)) ? 1 : 0;

	cpufreq_ver("@%s():%d, cur_khz = %d, target_khz = %d, is_fhctl_used = %d\n",
			__func__, __LINE__, cur_khz, target_khz, is_fhctl_used);

	if (!is_fhctl_used) {
		switch (target_khz) {
		case CPU_DVFS_FREQ4:
		case CPU_DVFS_FREQ4_1:
		case CPU_DVFS_FREQ4_2:
		case CPU_DVFS_FREQ5:
		case CPU_DVFS_FREQ5_1:
		case CPU_DVFS_FREQ5_2:
		case CPU_DVFS_FREQ6:
			dds = _mt_cpufreq_cpu_dds_calc(target_khz);
			sel = 8;	/* 4/4 */
			break;
		case CPU_DVFS_FREQ6_1:
		case CPU_DVFS_FREQ7:
		case CPU_DVFS_FREQ7_1:
		case CPU_DVFS_FREQ8:
			dds = _mt_cpufreq_cpu_dds_calc(target_khz * 2);
			sel = 10;	/* 2/4 */
			break;
		case CPU_DVFS_FREQ8_1:
			dds = _mt_cpufreq_cpu_dds_calc(target_khz * 4);
			sel = 11;	/* 1/4 */
			break;
		default:
			cpufreq_err("@%s: invalid target freq = %dKHz\n", __func__, target_khz);
			BUG();
		}

		cur_volt = p->ops->get_cur_volt(p);
#ifdef CONFIG_ARCH_MT6735
		/* Raise to 1.25V directly to avoid armpll_divider timing violation issue (only for 6735) */
		if (cur_volt < 125000)
			p->ops->set_cur_volt(p, 125000);
#else
		/* Adjust Vproc since MAINPLL is 1092 MHz (~= CPU_DVFS_FREQ3 or FREQ3_1) */
		if (cur_volt < cpu_dvfs_get_volt_by_idx(p, 2))
			p->ops->set_cur_volt(p, cpu_dvfs_get_volt_by_idx(p, 2));
#endif
		else
			cur_volt = 0;

		/* set ARMPLL and CLKDIV */
		_mt_cpufreq_dfs_by_set_armpll(p, dds, sel);

		/* restore Vproc */
		if (cur_volt)
			p->ops->set_cur_volt(p, cur_volt);
	} else {
		unsigned int ckdiv1_val = _GET_BITS_VAL_(4:0, cpufreq_read(TOP_CKDIV1));

#define IS_CLKDIV_USED(clkdiv)  (((clkdiv < 8) || ((clkdiv % 8) == 0)) ? 0 : 1)

		dds = _mt_cpufreq_cpu_dds_calc(target_khz);
		BUG_ON(dds & _BITMASK_(26:24));		/* should not use posdiv */
		BUG_ON(IS_CLKDIV_USED(ckdiv1_val));	/* should not use clkdiv */

#if !defined(__KERNEL__)
#if defined(MTKDRV_FREQHOP)
		fhdrv_dvt_dvfs_enable(ARMPLL_ID, dds);
#else
		_mt_cpufreq_dfs_by_set_armpll(p, dds, 8);
#endif

#else	/* __KERNEL__ */
#if 1
		mt_dfs_armpll(FH_ARM_PLLID, dds);
#else
		_mt_cpufreq_dfs_by_set_armpll(p, dds, 8);
#endif
#endif	/* ! __KERNEL__ */
	}

	FUNC_EXIT(FUNC_LV_LOCAL);
}
#endif

/* for volt change (PMICWRAP/extBuck) */
static unsigned int _mt_cpufreq_get_cur_volt(struct mt_cpu_dvfs *p)
{
	unsigned int rdata = 0;
	unsigned int retry_cnt = 5;

	FUNC_ENTER(FUNC_LV_LOCAL);

	rdata = pmic_get_register_value(PMIC_VPROC_EN);

	if (rdata) {	/* enabled i.e. not 0mv */
		do {
			rdata = pmic_get_register_value(PMIC_VPROC_VOSEL_ON);
		} while ((rdata == 0 || rdata > 0x7F) && retry_cnt--);

		rdata = PMIC_VAL_TO_VOLT(rdata);
		/* cpufreq_ver("@%s: volt = %d\n", __func__, rdata); */
	} else
		cpufreq_err("@%s: read VPROC_EN failed, rdata = 0x%x\n", __func__, rdata);

	FUNC_EXIT(FUNC_LV_LOCAL);

	return rdata;	/* vproc: mv*100 */
}

unsigned int mt_cpufreq_get_cur_volt(enum mt_cpu_dvfs_id id)
{
	struct mt_cpu_dvfs *p = id_to_cpu_dvfs(id);

	FUNC_ENTER(FUNC_LV_API);

	BUG_ON(NULL == p);
	BUG_ON(NULL == p->ops);

	FUNC_EXIT(FUNC_LV_API);

	return p->ops->get_cur_volt(p);	/* mv * 100 */
}
EXPORT_SYMBOL(mt_cpufreq_get_cur_volt);

static unsigned int _mt_cpufreq_calc_pmic_settle_time(unsigned int old_vproc,
							  unsigned int old_vsram,
							  unsigned int new_vproc,
							  unsigned int new_vsram)
{
	unsigned delay = 100;

	if (new_vproc == old_vproc && new_vsram == old_vsram)
		return 0;

	/* VPROC is UP */
	if (new_vproc >= old_vproc) {
		/* VSRAM is UP too, choose larger one to calculate settle time */
		if (new_vsram >= old_vsram)
			delay = MAX(PMIC_VOLT_UP_SETTLE_TIME(old_vsram, new_vsram),
					PMIC_VOLT_UP_SETTLE_TIME(old_vproc, new_vproc)
				);
		/* VSRAM is DOWN, it may happen at bootup stage */
		else
			delay = MAX(PMIC_VOLT_DOWN_SETTLE_TIME(old_vsram, new_vsram),
					PMIC_VOLT_UP_SETTLE_TIME(old_vproc, new_vproc)
				);
	}
	/* VPROC is DOWN */
	else {
		/* VSRAM is DOWN too, choose larger one to calculate settle time */
		if (old_vsram >= new_vsram)
			delay = MAX(PMIC_VOLT_DOWN_SETTLE_TIME(old_vsram, new_vsram),
					PMIC_VOLT_DOWN_SETTLE_TIME(old_vproc, new_vproc)
				);
		/* VSRAM is UP, it may happen at bootup stage */
		else
			delay = MAX(PMIC_VOLT_UP_SETTLE_TIME(old_vsram, new_vsram),
					PMIC_VOLT_DOWN_SETTLE_TIME(old_vproc, new_vproc)
				);
	}

	if (delay < MIN_PMIC_SETTLE_TIME)
		delay = MIN_PMIC_SETTLE_TIME;

	return delay;
}

static int _mt_cpufreq_set_cur_volt(struct mt_cpu_dvfs *p, unsigned int volt)
{				/* volt: vproc (mv*100) */
	unsigned int cur_volt = _mt_cpufreq_get_cur_volt(p);
#ifdef CONFIG_CPU_DVFS_TURBO_MODE
	bool is_leaving_turbo_mode = false;
#endif

	FUNC_ENTER(FUNC_LV_LOCAL);

#ifdef CONFIG_CPU_DVFS_AEE_RR_REC
	aee_rr_rec_cpu_dvfs_vproc_little(VOLT_TO_PMIC_VAL(volt));
#endif

#ifdef CONFIG_CPU_DVFS_TURBO_MODE
	if (is_in_turbo_mode && cur_volt > cpu_dvfs_get_volt_by_idx(p, 0)
		&& volt <= cpu_dvfs_get_volt_by_idx(p, 0))
		is_leaving_turbo_mode = true;
#endif

	pmic_set_register_value(PMIC_VPROC_VOSEL_ON, VOLT_TO_PMIC_VAL(volt));

	/* delay for scaling up */
	if (volt > cur_volt)
		/* vsram is autotracking, bypass it */
		udelay(_mt_cpufreq_calc_pmic_settle_time(cur_volt, 0, volt, 0));

	if (NULL != g_pCpuVoltSampler)
		g_pCpuVoltSampler(MT_CPU_DVFS_LITTLE, volt / 100);	/* mv */

#ifdef CONFIG_CPU_DVFS_TURBO_MODE
	if (is_leaving_turbo_mode) {
		/* cpufreq_dbg("@%s: turbo mode end\n", __func__); */

		is_in_turbo_mode = false;
#ifdef CONFIG_CPU_DVFS_AEE_RR_REC
		aee_rr_rec_cpu_dvfs_status(
			aee_rr_curr_cpu_dvfs_status() & ~(1 << CPU_DVFS_LITTLE_IS_TURBO));
#endif
	}
#endif

	FUNC_EXIT(FUNC_LV_LOCAL);

	return 0;
}


#ifdef CONFIG_CPU_DVFS_HAS_EXTBUCK
/* for SW tracking error handling */
static void _mt_cpufreq_dump_opp_table(struct mt_cpu_dvfs *p)
{
	int i;

	cpufreq_err("[%s/%d]\n" "cpufreq_oppidx = %d\n", p->name, p->cpu_id, p->idx_opp_tbl);

	for (i = 0; i < p->nr_opp_tbl; i++) {
		cpufreq_err("\tOP(%d, %d),\n",
				cpu_dvfs_get_freq_by_idx(p, i), cpu_dvfs_get_volt_by_idx(p, i)
			);
	}
}

static unsigned int _mt_cpufreq_get_cur_vsram(struct mt_cpu_dvfs *p)
{
	unsigned int rdata = 0;
	unsigned int retry_cnt = 5;

	FUNC_ENTER(FUNC_LV_LOCAL);

	rdata = pmic_get_register_value(PMIC_RG_VSRAM_EN);

	if (rdata) {	/* enabled i.e. not 0mv */
		do {
			rdata = pmic_get_register_value(PMIC_RG_VSRAM_VOSEL);
		} while ((rdata == 0 || rdata > 0x7F) && retry_cnt--);

		rdata = PMIC_VAL_TO_VOLT(rdata);
		/* cpufreq_ver("@%s: volt = %d\n", __func__, rdata); */
	} else
		cpufreq_err("@%s: read VSRAM_EN failed, rdata = 0x%x\n", __func__, rdata);

	FUNC_EXIT(FUNC_LV_LOCAL);

	return rdata;	/* vproc: mv*100 */
}

static unsigned int _mt_cpufreq_get_cur_volt_extbuck(struct mt_cpu_dvfs *p)
{
	unsigned char ret_val = 0;
	unsigned int ret_volt = 0;	/* volt: mv * 100 */
	unsigned int retry_cnt = 5;

	FUNC_ENTER(FUNC_LV_LOCAL);

	if (cpu_dvfs_is_extbuck_valid()) {
		do {
			if (!mt6311_read_byte(MT6311_VDVFS11_CON13, &ret_val)) {
				cpufreq_err("%s(), fail to read ext buck volt\n", __func__);
				ret_volt = 0;
			} else {
				ret_volt = EXTBUCK_VAL_TO_VOLT(ret_val);
				cpufreq_ver("@%s: volt = %d\n", __func__, ret_volt);
			}
		/* XXX: EXTBUCK_VAL_TO_VOLT(0) is impossible setting and need to retry */
		} while (ret_volt == EXTBUCK_VAL_TO_VOLT(0) && retry_cnt--);
	} else
		cpufreq_err("%s(), can not use ext buck!\n", __func__);

	FUNC_EXIT(FUNC_LV_LOCAL);

	return ret_volt;
}

/* volt: vproc (mv*100) */
static int _mt_cpufreq_set_cur_volt_extbuck(struct mt_cpu_dvfs *p, unsigned int volt)
{
	unsigned int cur_vsram = _mt_cpufreq_get_cur_vsram(p);
	unsigned int cur_vproc = _mt_cpufreq_get_cur_volt_extbuck(p);
	unsigned int delay_us = 0;
#ifdef CONFIG_CPU_DVFS_TURBO_MODE
	bool is_leaving_turbo_mode = false;
#endif
	int ret = 0;

	FUNC_ENTER(FUNC_LV_LOCAL);

#ifdef CONFIG_CPU_DVFS_AEE_RR_REC
	aee_rr_rec_cpu_dvfs_vproc_little(VOLT_TO_EXTBUCK_VAL(volt));
#endif

	if (cur_vproc == 0 || !cpu_dvfs_is_extbuck_valid()) {
		cpufreq_err("@%s():%d, can not use ext buck!\n", __func__, __LINE__);
		return -1;
	}
#ifdef CONFIG_CPU_DVFS_TURBO_MODE
	if (is_in_turbo_mode && cur_vproc > cpu_dvfs_get_volt_by_idx(p, 0)
		&& volt <= cpu_dvfs_get_volt_by_idx(p, 0))
		is_leaving_turbo_mode = true;
#endif

	if (unlikely
		(!((cur_vsram > cur_vproc) && (MAX_DIFF_VSRAM_VPROC >= (cur_vsram - cur_vproc))))) {
		unsigned int i, val, extbuck_chip_id = mt6311_get_chip_id();

		_mt_cpufreq_dump_opp_table(p);
		cpufreq_err("@%s():%d, cur_vsram = %d, cur_vproc = %d, extbuck_chip_id = 0x%x\n",
				__func__, __LINE__, cur_vsram, cur_vproc, extbuck_chip_id);

		/* read extbuck chip id to verify I2C is still worked or not */
		for (i = 0; i < 10; i++) {
			val = ((mt6311_get_cid() << 8) | (mt6311_get_swcid()));
			cpufreq_err("read 6311 chip id from I2C, id = 0x%x\n", val);
		}

		/* read pmic wrap chip id */
		for (i = 0; i < 10; i++) {
			/* pwrap_read(0x200, &val); */
			val = pmic_get_register_value(PMIC_HWCID);
			cpufreq_err("pmic 6328 HW CID = %x\n", val);
		}

		aee_kernel_warning(TAG, "@%s():%d, cur_vsram = %d, cur_vproc = %d\n",
				   __func__, __LINE__, cur_vsram, cur_vproc);

		cur_vproc = cpu_dvfs_get_cur_volt(p);
		cur_vsram = cur_vproc + NORMAL_DIFF_VSRAM_VPROC;
	}

	/* UP */
	if (volt > cur_vproc) {
		unsigned int target_vsram = volt + NORMAL_DIFF_VSRAM_VPROC;
		unsigned int next_vsram;

		do {
			unsigned int old_vproc = cur_vproc;
			unsigned int old_vsram = cur_vsram;

			next_vsram = MIN(((MAX_DIFF_VSRAM_VPROC - 2500) + cur_vproc), target_vsram);

			/* update vsram */
			cur_vsram = MAX(next_vsram, MIN_VSRAM_VOLT);
			if (cur_vsram > MAX_VSRAM_VOLT) {
				cur_vsram = MAX_VSRAM_VOLT;
				target_vsram = MAX_VSRAM_VOLT;	/* to end the loop */
			}

			if (unlikely
				(!((cur_vsram > cur_vproc)
				   && (MAX_DIFF_VSRAM_VPROC >= (cur_vsram - cur_vproc))))) {
				_mt_cpufreq_dump_opp_table(p);
				cpufreq_err("@%s():%d, cur_vsram = %d, cur_vproc = %d\n",
						__func__, __LINE__, cur_vsram, cur_vproc);
				BUG();
			}

			pmic_set_register_value(PMIC_RG_VSRAM_VOSEL, VOLT_TO_PMIC_VAL(cur_vsram));

			/* update vproc */
			if (next_vsram > MAX_VSRAM_VOLT)
				cur_vproc = volt;	/* Vsram was limited, set to target vproc directly */
			else
				cur_vproc = next_vsram - NORMAL_DIFF_VSRAM_VPROC;

			if (unlikely
				(!((cur_vsram > cur_vproc)
				   && (MAX_DIFF_VSRAM_VPROC >= (cur_vsram - cur_vproc))))) {
				_mt_cpufreq_dump_opp_table(p);
				cpufreq_err("@%s():%d, cur_vsram = %d, cur_vproc = %d\n",
						__func__, __LINE__, cur_vsram, cur_vproc);
				BUG();
			}

			if (cpu_dvfs_is_extbuck_valid()) {
				mt6311_set_vdvfs11_vosel_on(VOLT_TO_EXTBUCK_VAL(cur_vproc));
			} else {
				cpufreq_err("%s(), fail to set ext buck volt\n", __func__);
				ret = -1;
				break;
			}

			delay_us =
				_mt_cpufreq_calc_pmic_settle_time(
					old_vproc, old_vsram, cur_vproc, cur_vsram);
			udelay(delay_us);

			cpufreq_ver("@%s(): UP --> old_vsram=%d, cur_vsram=%d, old_vproc=%d, cur_vproc=%d, delay=%d\n",
					__func__, old_vsram, cur_vsram, old_vproc, cur_vproc, delay_us);
		} while (target_vsram > cur_vsram);
	}
	/* DOWN */
	else if (volt < cur_vproc) {
		unsigned int next_vproc;
		unsigned int next_vsram = cur_vproc + NORMAL_DIFF_VSRAM_VPROC;

		do {
			unsigned int old_vproc = cur_vproc;
			unsigned int old_vsram = cur_vsram;

			next_vproc = MAX((next_vsram - (MAX_DIFF_VSRAM_VPROC - 2500)), volt);

			/* update vproc */
			cur_vproc = next_vproc;

			if (unlikely
				(!((cur_vsram > cur_vproc)
				   && (MAX_DIFF_VSRAM_VPROC >= (cur_vsram - cur_vproc))))) {
				_mt_cpufreq_dump_opp_table(p);
				cpufreq_err("@%s():%d, cur_vsram = %d, cur_vproc = %d\n",
						__func__, __LINE__, cur_vsram, cur_vproc);
				BUG();
			}

			if (cpu_dvfs_is_extbuck_valid()) {
				mt6311_set_vdvfs11_vosel_on(VOLT_TO_EXTBUCK_VAL(cur_vproc));
			} else {
				cpufreq_err("%s(), fail to set ext buck volt\n", __func__);
				ret = -1;
				break;
			}

			/* update vsram */
			next_vsram = cur_vproc + NORMAL_DIFF_VSRAM_VPROC;
			cur_vsram = MAX(next_vsram, MIN_VSRAM_VOLT);
			cur_vsram = MIN(cur_vsram, MAX_VSRAM_VOLT);

			if (unlikely
				(!((cur_vsram > cur_vproc)
				   && (MAX_DIFF_VSRAM_VPROC >= (cur_vsram - cur_vproc))))) {
				_mt_cpufreq_dump_opp_table(p);
				cpufreq_err("@%s():%d, cur_vsram = %d, cur_vproc = %d\n",
						__func__, __LINE__, cur_vsram, cur_vproc);
				BUG();
			}

			pmic_set_register_value(PMIC_RG_VSRAM_VOSEL, VOLT_TO_PMIC_VAL(cur_vsram));

			delay_us =
				_mt_cpufreq_calc_pmic_settle_time(
					old_vproc, old_vsram, cur_vproc, cur_vsram);
			udelay(delay_us);

			cpufreq_ver(
				"@%s(): DOWN --> old_vsram=%d, cur_vsram=%d, old_vproc=%d, cur_vproc=%d, delay=%d\n",
					__func__, old_vsram, cur_vsram, old_vproc, cur_vproc, delay_us);
		} while (cur_vproc > volt);
	}

	if (NULL != g_pCpuVoltSampler)
		g_pCpuVoltSampler(MT_CPU_DVFS_LITTLE, volt / 100);	/* mv */

	cpufreq_ver("@%s():%d, cur_vsram = %d, cur_vproc = %d\n",
			__func__, __LINE__, cur_vsram, cur_vproc);

#ifdef CONFIG_CPU_DVFS_TURBO_MODE
	if (is_leaving_turbo_mode) {
		/* cpufreq_dbg("@%s: turbo mode end\n", __func__); */

		is_in_turbo_mode = false;
#ifdef CONFIG_CPU_DVFS_AEE_RR_REC
		aee_rr_rec_cpu_dvfs_status(
			aee_rr_curr_cpu_dvfs_status() & ~(1 << CPU_DVFS_LITTLE_IS_TURBO));
#endif
	}
#endif

	FUNC_EXIT(FUNC_LV_LOCAL);

	return ret;
}
#endif


/* cpufreq set (freq & volt) */
static unsigned int _mt_cpufreq_search_available_volt(struct mt_cpu_dvfs *p,
							  unsigned int target_khz)
{
	int i;

	FUNC_ENTER(FUNC_LV_HELP);

	BUG_ON(NULL == p);

	/* search available voltage */
	for (i = p->nr_opp_tbl - 1; i >= 0; i--) {
		if (target_khz <= cpu_dvfs_get_freq_by_idx(p, i))
			break;
	}

	BUG_ON(i < 0);	/* i.e. target_khz > p->opp_tbl[0].cpufreq_khz */

	FUNC_EXIT(FUNC_LV_HELP);

	return cpu_dvfs_get_volt_by_idx(p, i);	/* mv * 100 */
}

static int _mt_cpufreq_set_locked(struct mt_cpu_dvfs *p, unsigned int cur_khz,
				  unsigned int target_khz, struct cpufreq_policy *policy)
{
	unsigned int volt;	/* mv * 100 */
	int ret = 0;
#ifdef CONFIG_CPU_FREQ
	struct cpufreq_freqs freqs;
	/* unsigned int cpu; */
	unsigned int target_khz_orig = target_khz;
#endif
#ifdef CONFIG_CPU_DVFS_TURBO_MODE
	enum turbo_mode mode = _mt_cpufreq_get_turbo_mode(p, target_khz);
#endif

	FUNC_ENTER(FUNC_LV_HELP);

	volt = _mt_cpufreq_search_available_volt(p, target_khz);

#ifdef CONFIG_CPU_DVFS_TURBO_MODE
	if (cur_khz != TURBO_MODE_FREQ(mode, target_khz))
		cpufreq_ver("@%s(), target_khz = %d (%d), volt = %d (%d), num_online_cpus = %d, cur_khz = %d\n",
				__func__, target_khz, TURBO_MODE_FREQ(mode, target_khz), volt,
				TURBO_MODE_VOLT(mode, volt), num_online_cpus(), cur_khz
		);

	volt = TURBO_MODE_VOLT(mode, volt);
	target_khz = TURBO_MODE_FREQ(mode, target_khz);
#else
	if (cur_khz != target_khz)
		cpufreq_ver("@%s(), target_khz = %d, volt = %d, cur_khz = %d\n",
				__func__, target_khz, volt, cur_khz);
#endif

	if (cur_khz == target_khz)
		goto out;

	/* set volt (UP) */
	if (cur_khz < target_khz) {
		ret = p->ops->set_cur_volt(p, volt);

		if (ret)	/* set volt fail */
			goto out;
	}
#ifdef CONFIG_CPU_FREQ
	freqs.old = cur_khz;
	/* new freq without turbo */
	freqs.new = target_khz_orig;
#if 0
	if (policy) {
		for_each_online_cpu(cpu) {
			freqs.cpu = cpu;
			cpufreq_freq_transition_begin(policy, &freqs);
		}
	}
#else
	/* fix notify transition hang issue for Linux-3.18 */
	if (policy) {
		freqs.cpu = policy->cpu;
		cpufreq_freq_transition_begin(policy, &freqs);
	}
#endif
#endif

/* set freq (UP/DOWN) */
	if (cur_khz != target_khz)
		p->ops->set_cur_freq(p, cur_khz, target_khz);

#ifdef CONFIG_CPU_FREQ
#if 0
	if (policy) {
		for_each_online_cpu(cpu) {
			freqs.cpu = cpu;
			cpufreq_freq_transition_end(policy, &freqs, 0);
		}
	}
#else
	/* fix notify transition hang issue for Linux-3.18 */
	if (policy)
		cpufreq_freq_transition_end(policy, &freqs, 0);
#endif
#endif

	/* set volt (DOWN) */
	if (cur_khz > target_khz) {
		ret = p->ops->set_cur_volt(p, volt);

		if (ret)	/* set volt fail */
			goto out;
	}

	cpufreq_dbg("@%s(): Vproc = %dmv, freq = %d KHz\n",
			__func__, (p->ops->get_cur_volt(p)) / 100, p->ops->get_cur_phy_freq(p)
		);
#ifdef CONFIG_CPU_DVFS_HAS_EXTBUCK
	/* SW tracking, print Vsram result */
	if (cpu_dvfs_is_extbuck_valid())
		cpufreq_dbg("@%s(): Vsram = %dmv\n", __func__, _mt_cpufreq_get_cur_vsram(p) / 100);
#endif

#ifndef DISABLE_PBM_FEATURE
	if (!p->dvfs_disable_by_suspend)
		_kick_PBM_by_cpu(p);
#endif

	/* trigger exception if freq/volt not correct during stress */
	if (do_dvfs_stress_test) {
		BUG_ON(p->ops->get_cur_volt(p) != volt);
		BUG_ON(p->ops->get_cur_phy_freq(p) != target_khz);
	}

	FUNC_EXIT(FUNC_LV_HELP);
out:
	return ret;
}

/* return -1 (not found) */
static int _mt_cpufreq_get_idx_by_freq(struct mt_cpu_dvfs *p, unsigned int target_khz,
					   unsigned int relation)
{
	int new_opp_idx = -1;
	int i;

	FUNC_ENTER(FUNC_LV_HELP);

	if (CPUFREQ_RELATION_L == relation) {
		for (i = (signed)(p->nr_opp_tbl - 1); i >= 0; i--) {
			if (cpu_dvfs_get_freq_by_idx(p, i) >= target_khz) {
				new_opp_idx = i;
				break;
			}
		}
	} else {	/* CPUFREQ_RELATION_H */
		for (i = 0; i < (signed)p->nr_opp_tbl; i++) {
			if (cpu_dvfs_get_freq_by_idx(p, i) <= target_khz) {
				new_opp_idx = i;
				break;
			}
		}
	}

	FUNC_EXIT(FUNC_LV_HELP);

	return new_opp_idx;
}

#ifdef CONFIG_ARCH_MT6753
static bool is_limit_modified_by_5A_throttle;
#endif

static int _mt_cpufreq_power_limited_verify(struct mt_cpu_dvfs *p, int new_opp_idx)
{
	unsigned int target_khz = cpu_dvfs_get_freq_by_idx(p, new_opp_idx);
	int possible_cpu = 0;
	unsigned int online_cpu = 0;
	int found = 0;
	int i;

	FUNC_ENTER(FUNC_LV_HELP);

	possible_cpu = num_possible_cpus();
	online_cpu = num_online_cpus();

	/* cpufreq_dbg("%s(): begin, idx = %d, online_cpu = %d\n", __func__, new_opp_idx, online_cpu); */

	/* no limited */
#ifndef DISABLE_PBM_FEATURE
	if (0 == p->limited_power_by_thermal && 0 == p->limited_power_by_pbm)
#else
	if (0 == p->limited_power_by_thermal)
#endif
		return new_opp_idx;

#ifdef CONFIG_ARCH_MT6753
	if (is_need_5A_throttle(p, p->limited_max_freq, p->limited_max_ncpu)) {
		cpufreq_ver("@%s: modify limited max freq and ncpu due to 5A limit enabled!\n", __func__);
		p->limited_max_freq = PMIC_5A_THRO_MAX_CPU_FREQ;
		p->limited_max_ncpu = possible_cpu;
		p->limited_power_idx = 3;
		is_limit_modified_by_5A_throttle = true;
	} else if (is_limit_modified_by_5A_throttle) {
		/* re-calculate limit */
#ifndef DISABLE_PBM_FEATURE
		if (p->limited_power_by_pbm && p->limited_power_by_thermal)
			_mt_cpufreq_set_limit_by_pwr_budget(MIN(p->limited_power_by_pbm, p->limited_power_by_thermal));
		else if (p->limited_power_by_pbm)
			_mt_cpufreq_set_limit_by_pwr_budget(p->limited_power_by_pbm);
		else if (p->limited_power_by_thermal)
			_mt_cpufreq_set_limit_by_pwr_budget(p->limited_power_by_thermal);
		else {
			/* unlimit */
			p->limited_max_freq = cpu_dvfs_get_max_freq(p);
			p->limited_max_ncpu = possible_cpu;
			p->limited_power_idx = 0;
		}
#else
		if (p->limited_power_by_thermal)
			_mt_cpufreq_set_limit_by_pwr_budget(p->limited_power_by_thermal);
		else {
			/* unlimit */
			p->limited_max_freq = cpu_dvfs_get_max_freq(p);
			p->limited_max_ncpu = possible_cpu;
			p->limited_power_idx = 0;
		}
#endif
		is_limit_modified_by_5A_throttle = false;
	}
#endif

	for (i = 0; i < p->nr_opp_tbl * possible_cpu; i++) {
		if (p->power_tbl[i].cpufreq_ncpu == p->limited_max_ncpu
			&& p->power_tbl[i].cpufreq_khz == p->limited_max_freq)
			break;
	}

	cpufreq_ver("%s(): idx = %d, limited_max_ncpu = %d, limited_max_freq = %d\n",
			__func__, i, p->limited_max_ncpu, p->limited_max_freq);

	for (; i < p->nr_opp_tbl * possible_cpu; i++) {
		if (p->power_tbl[i].cpufreq_ncpu == online_cpu) {
			if (target_khz >= p->power_tbl[i].cpufreq_khz) {
				found = 1;
				break;
			}
		}
	}

	if (found) {
		target_khz = p->power_tbl[i].cpufreq_khz;
		cpufreq_ver("%s(): freq found, idx = %d, target_khz = %d, online_cpu = %d\n",
				__func__, i, target_khz, online_cpu);
	} else {
		target_khz = p->limited_max_freq;
		cpufreq_dbg("%s(): freq not found, set to limited_max_freq = %d\n",
				__func__, target_khz);
	}

	/* TODO: refine this function for idx searching */
	i = _mt_cpufreq_get_idx_by_freq(p, target_khz, CPUFREQ_RELATION_H);

	FUNC_EXIT(FUNC_LV_HELP);

	return i;
}

static unsigned int _mt_cpufreq_calc_new_opp_idx(struct mt_cpu_dvfs *p, int new_opp_idx)
{
	int idx;

	FUNC_ENTER(FUNC_LV_HELP);

	BUG_ON(NULL == p);

	/* kdriver limit min freq */
	if (p->limited_min_freq_by_kdriver) {
		idx = _mt_cpufreq_get_idx_by_freq(p, p->limited_min_freq_by_kdriver, CPUFREQ_RELATION_L);

		if (idx != -1 && new_opp_idx > idx) {
			new_opp_idx = idx;
			cpufreq_ver("%s(): kdriver limited freq, idx = %d\n", __func__, new_opp_idx);
		}
	}

	/* HEVC */
	if (p->limited_freq_by_hevc) {
		idx = _mt_cpufreq_get_idx_by_freq(p, p->limited_freq_by_hevc, CPUFREQ_RELATION_L);

		if (idx != -1 && new_opp_idx > idx) {
			new_opp_idx = idx;
			cpufreq_ver("%s(): hevc limited freq, idx = %d\n", __func__, new_opp_idx);
		}
	}

	/* search power limited freq */
	idx = _mt_cpufreq_power_limited_verify(p, new_opp_idx);
	if (idx != -1 && idx != new_opp_idx) {
		new_opp_idx = idx;
		cpufreq_ver("%s(): thermal/DLPT limited freq, idx = %d\n", __func__, new_opp_idx);
	}

	/* for early suspend */
	if (p->dvfs_disable_by_early_suspend) {
		if (is_fix_freq_in_ES) {
			/* Fix at about 1GHz to fix OGG Vorbis Playback noise issue */
			idx =
				_mt_cpufreq_get_idx_by_freq(
					p, CPUFREQ_FIX_FREQ_FOR_ES, CPUFREQ_RELATION_L);
			if (idx != -1)
				new_opp_idx = idx;
			else
				new_opp_idx = p->idx_normal_max_opp;
		} else {
			if (new_opp_idx > p->idx_normal_max_opp)
				new_opp_idx = p->idx_normal_max_opp;
		}
		cpufreq_ver("%s(): for early suspend, idx = %d\n", __func__, new_opp_idx);
	}

	/* for suspend */
	if (p->dvfs_disable_by_suspend)
		new_opp_idx = p->idx_normal_max_opp;

	/* for power throttling */
#ifdef CONFIG_CPU_DVFS_POWER_THROTTLING
	if (p->pwr_thro_mode && new_opp_idx < p->idx_pwr_thro_max_opp) {
		new_opp_idx = p->idx_pwr_thro_max_opp;
		cpufreq_ver("%s(): for power throttling = %d\n", __func__, new_opp_idx);
	}
#endif

	/* limit max freq by user */
	if (p->limited_max_freq_by_user) {
		idx = _mt_cpufreq_get_idx_by_freq(
			p, p->limited_max_freq_by_user, CPUFREQ_RELATION_H);

		if (idx != -1 && new_opp_idx < idx) {
			new_opp_idx = idx;
			cpufreq_ver("%s(): limited max freq by user, idx = %d\n",
					__func__, new_opp_idx);
		}
	}

	/* kdriver limit max freq */
	if (p->limited_max_freq_by_kdriver) {
		idx = _mt_cpufreq_get_idx_by_freq(
			p, p->limited_max_freq_by_kdriver, CPUFREQ_RELATION_H);

		if (idx != -1 && new_opp_idx < idx) {
			new_opp_idx = idx;
			cpufreq_ver("%s(): limited max freq by kdriver, idx = %d\n",
					__func__, new_opp_idx);
		}
	}

	/* for ptpod init */
	if (p->dvfs_disable_by_ptpod) {
#ifdef CONFIG_ARCH_MT6753
		/* idx 2 for Vboot = 1.2V */
		new_opp_idx = 2;
#else
		/* idx 0 for Vboot = 1.25V */
		new_opp_idx = 0;
#endif
		cpufreq_info("%s(): for ptpod init, idx = %d\n", __func__, new_opp_idx);
	}
#ifdef CONFIG_ARCH_MT6753
	if (is_need_5A_throttle(p, cpu_dvfs_get_freq_by_idx(p, new_opp_idx),
				num_online_cpus() + num_online_cpus_delta)) {
		idx = _mt_cpufreq_get_idx_by_freq(p, PMIC_5A_THRO_MAX_CPU_FREQ, CPUFREQ_RELATION_H);

		if (idx != -1 && new_opp_idx < idx) {
			new_opp_idx = idx;
			cpufreq_ver("%s(): limited max freq by PMIC 5A throttle, idx = %d\n",
					__func__, new_opp_idx);
		}
	}
#endif

#ifdef CONFIG_CPU_DVFS_AEE_RR_REC
	aee_rr_rec_cpu_dvfs_oppidx((aee_rr_curr_cpu_dvfs_oppidx() & 0xF0) | new_opp_idx);
#endif

	FUNC_EXIT(FUNC_LV_HELP);

	return new_opp_idx;
}

static void _mt_cpufreq_set(enum mt_cpu_dvfs_id id, int new_opp_idx)
{
	unsigned long flags;
	struct mt_cpu_dvfs *p = id_to_cpu_dvfs(id);
	unsigned int cur_freq;
	unsigned int target_freq;
#ifdef CONFIG_CPU_FREQ
	struct cpufreq_policy *policy;
#endif

	FUNC_ENTER(FUNC_LV_LOCAL);

	BUG_ON(NULL == p);
	BUG_ON(new_opp_idx >= p->nr_opp_tbl);

#ifdef CONFIG_CPU_FREQ
	policy = cpufreq_cpu_get(p->cpu_id);
#endif

	cpufreq_lock(flags);	/* <-XXX */

	/* get current idx here to avoid idx synchronization issue */
	if (new_opp_idx == -1)
		new_opp_idx = p->idx_opp_tbl;

	if (do_dvfs_stress_test && !(p->dvfs_disable_by_suspend))
		new_opp_idx = jiffies & 0x7;	/* 0~7 */
	else
		new_opp_idx = _mt_cpufreq_calc_new_opp_idx(id_to_cpu_dvfs(id), new_opp_idx);

	cur_freq = p->ops->get_cur_phy_freq(p);
	target_freq = cpu_dvfs_get_freq_by_idx(p, new_opp_idx);
#ifdef CONFIG_CPU_FREQ
	_mt_cpufreq_set_locked(p, cur_freq, target_freq, policy);
#else
	_mt_cpufreq_set_locked(p, cur_freq, target_freq, NULL);
#endif
	p->idx_opp_tbl = new_opp_idx;

	cpufreq_unlock(flags);	/* <-XXX */

#ifdef CONFIG_CPU_FREQ
	if (policy)
		cpufreq_cpu_put(policy);
#endif

	FUNC_EXIT(FUNC_LV_LOCAL);
}

static int __cpuinit _mt_cpufreq_cpu_CB(struct notifier_block *nfb, unsigned long action,
					void *hcpu)
{
	unsigned int cpu = (unsigned long)hcpu;
	unsigned int online_cpus = num_online_cpus();
	struct device *dev;
	struct mt_cpu_dvfs *p = id_to_cpu_dvfs(0);

	cpufreq_ver("@%s():%d, cpu = %d, action = %lu, oppidx = %d, num_online_cpus = %d\n",
			__func__, __LINE__, cpu, action, p->idx_opp_tbl, online_cpus);	/* <-XXX */

	dev = get_cpu_device(cpu);

	if (dev) {
#ifdef CONFIG_ARCH_MT6753
		cpufreq_ver("@%s():%d, num_online_cpus_delta = %d\n",
				__func__, __LINE__, num_online_cpus_delta);

		if ((p->cpu_level == CPU_LEVEL_1 && !cpu_dvfs_is_extbuck_valid()
			 && PMIC_5A_THRO_MAX_CPU_CORE_NUM == online_cpus)
#ifdef CONFIG_CPU_DVFS_TURBO_MODE
			|| TURBO_MODE_BOUNDARY_CPU_NUM == online_cpus
#endif
			) {
			switch (action) {
			case CPU_UP_PREPARE:
			case CPU_UP_PREPARE_FROZEN:
				num_online_cpus_delta = 1;
			case CPU_DEAD:
			case CPU_DEAD_FROZEN:
				_mt_cpufreq_set(MT_CPU_DVFS_LITTLE, -1);
				break;
			}
		} else {
			switch (action) {
			case CPU_ONLINE:	/* CPU UP done */
			case CPU_ONLINE_FROZEN:
			case CPU_UP_CANCELED:	/* CPU UP failed */
			case CPU_UP_CANCELED_FROZEN:
				num_online_cpus_delta = 0;
				break;
			}
		}

		cpufreq_ver("@%s():%d, num_online_cpus_delta = %d\n",
				__func__, __LINE__, num_online_cpus_delta);
#endif

#ifndef DISABLE_PBM_FEATURE
		/* Notify PBM after CPU on/off */
		if (action == CPU_ONLINE || action == CPU_ONLINE_FROZEN
			|| action == CPU_DEAD || action == CPU_DEAD_FROZEN) {
			unsigned long flags;

			cpufreq_lock(flags);

			if (!p->dvfs_disable_by_suspend)
				_kick_PBM_by_cpu(p);

			cpufreq_unlock(flags);
		}
#endif

		cpufreq_ver("@%s():%d, cpu = %d, action = %lu, oppidx = %d, num_online_cpus = %d\n",
				__func__, __LINE__, cpu, action, p->idx_opp_tbl, online_cpus);	/* <-XXX */
	}

	return NOTIFY_OK;
}

static int _mt_cpufreq_sync_opp_tbl_idx(struct mt_cpu_dvfs *p)
{
	int ret = -1;
	unsigned int freq;
	int i;

	FUNC_ENTER(FUNC_LV_HELP);

	BUG_ON(NULL == p);
	BUG_ON(NULL == p->opp_tbl);
	BUG_ON(NULL == p->ops);

	freq = p->ops->get_cur_phy_freq(p);

	for (i = p->nr_opp_tbl - 1; i >= 0; i--) {
		if (freq <= cpu_dvfs_get_freq_by_idx(p, i)) {
			p->idx_opp_tbl = i;
			break;
		}

	}

	if (i >= 0) {
		cpufreq_info("%s freq = %d\n", cpu_dvfs_get_name(p), cpu_dvfs_get_cur_freq(p));

		/* TODO: apply correct voltage??? */

		ret = 0;
	} else
		cpufreq_warn("%s can't find freq = %d\n", cpu_dvfs_get_name(p), freq);

	FUNC_EXIT(FUNC_LV_HELP);

	return ret;
}

static void _mt_cpufreq_power_calculation(struct mt_cpu_dvfs *p, int oppidx, int ncpu)
{
#ifdef CONFIG_ARCH_MT6753
#define CA53_REF_POWER		2456	/* mW  */
#define CA53_REF_FREQ		  1300000	/* KHz */
#elif defined(CONFIG_ARCH_MT6735M)
#define CA53_REF_POWER		988	/* mW  */
#define CA53_REF_FREQ		  1000000	/* KHz */
#else
#define CA53_REF_POWER		1228	/* mW  */
#define CA53_REF_FREQ		  1300000	/* KHz */
#endif
#define CA53_REF_VOLT		  125000	/* mV * 100 */

	int p_dynamic = 0, p_leakage = 0, ref_freq, ref_volt;
	int possible_cpu = num_possible_cpus();

	FUNC_ENTER(FUNC_LV_HELP);

	ref_freq = CA53_REF_FREQ;
	ref_volt = CA53_REF_VOLT;
	p_dynamic = CA53_REF_POWER;

	/* TODO: Use temp=65 to calculate leakage? check this! */
	p_leakage = mt_spower_get_leakage(MT_SPOWER_CPU, p->opp_tbl[oppidx].cpufreq_volt / 100, 65);

	p_dynamic = p_dynamic *
		(p->opp_tbl[oppidx].cpufreq_khz / 1000) / (ref_freq / 1000) *
		p->opp_tbl[oppidx].cpufreq_volt / ref_volt *
		p->opp_tbl[oppidx].cpufreq_volt / ref_volt + p_leakage;

	p->power_tbl[NR_MAX_OPP_TBL * (possible_cpu - 1 - ncpu) + oppidx].cpufreq_ncpu = ncpu + 1;
	p->power_tbl[NR_MAX_OPP_TBL * (possible_cpu - 1 - ncpu) + oppidx].cpufreq_khz =
		p->opp_tbl[oppidx].cpufreq_khz;
	p->power_tbl[NR_MAX_OPP_TBL * (possible_cpu - 1 - ncpu) + oppidx].cpufreq_power =
		p_dynamic * (ncpu + 1) / possible_cpu;

	FUNC_EXIT(FUNC_LV_HELP);
}

static int _mt_cpufreq_setup_power_table(struct mt_cpu_dvfs *p)
{
#ifdef CONFIG_ARCH_MT6753
	static const unsigned int pwr_tbl_cgf[NR_MAX_CPU] = { 0, 0, 0, 0, 0, 0, 0, 0 };
#else
	static const unsigned int pwr_tbl_cgf[NR_MAX_CPU] = { 0, 0, 0, 0 };
#endif
	unsigned int pwr_eff_tbl[NR_MAX_OPP_TBL][NR_MAX_CPU];
	unsigned int pwr_eff_num;
	int possible_cpu = num_possible_cpus();
	int i, j;
	int ret = 0;

	FUNC_ENTER(FUNC_LV_LOCAL);

	BUG_ON(NULL == p);

	if (p->power_tbl)
		goto out;

	/* allocate power table */
	memset((void *)pwr_eff_tbl, 0, sizeof(pwr_eff_tbl));
	p->power_tbl =
		kzalloc(p->nr_opp_tbl * possible_cpu * sizeof(struct mt_cpu_power_info), GFP_KERNEL);

	if (NULL == p->power_tbl) {
		ret = -ENOMEM;
		goto out;
	}

	/* setup power efficiency array */
	for (i = 0, pwr_eff_num = 0; i < possible_cpu; i++) {
		if (1 == pwr_tbl_cgf[i])
			pwr_eff_num++;
	}

	for (i = 0; i < p->nr_opp_tbl; i++) {
		for (j = 0; j < possible_cpu; j++) {
			if (1 == pwr_tbl_cgf[j])
				pwr_eff_tbl[i][j] = 1;
		}
	}

	p->nr_power_tbl = p->nr_opp_tbl * (possible_cpu - pwr_eff_num);

	/* calc power and fill in power table */
	for (i = 0; i < p->nr_opp_tbl; i++) {
		for (j = 0; j < possible_cpu; j++) {
			if (0 == pwr_eff_tbl[i][j])
				_mt_cpufreq_power_calculation(p, i, j);
		}
	}

	/* sort power table */
	for (i = p->nr_opp_tbl * possible_cpu; i > 0; i--) {
		for (j = 1; j < i; j++) {
			if (p->power_tbl[j - 1].cpufreq_power < p->power_tbl[j].cpufreq_power) {
				struct mt_cpu_power_info tmp;

				tmp.cpufreq_khz = p->power_tbl[j - 1].cpufreq_khz;
				tmp.cpufreq_ncpu = p->power_tbl[j - 1].cpufreq_ncpu;
				tmp.cpufreq_power = p->power_tbl[j - 1].cpufreq_power;

				p->power_tbl[j - 1].cpufreq_khz = p->power_tbl[j].cpufreq_khz;
				p->power_tbl[j - 1].cpufreq_ncpu = p->power_tbl[j].cpufreq_ncpu;
				p->power_tbl[j - 1].cpufreq_power = p->power_tbl[j].cpufreq_power;

				p->power_tbl[j].cpufreq_khz = tmp.cpufreq_khz;
				p->power_tbl[j].cpufreq_ncpu = tmp.cpufreq_ncpu;
				p->power_tbl[j].cpufreq_power = tmp.cpufreq_power;
			}
		}
	}

	/* dump power table */
	for (i = 0; i < p->nr_opp_tbl * possible_cpu; i++) {
		cpufreq_info
			("[%d] = { .cpufreq_khz = %d,\t.cpufreq_ncpu = %d,\t.cpufreq_power = %d }\n", i,
			 p->power_tbl[i].cpufreq_khz, p->power_tbl[i].cpufreq_ncpu,
			 p->power_tbl[i].cpufreq_power);
	}

#if 0	/* unused */
#ifdef CONFIG_THERMAL
	mtk_cpufreq_register(p->power_tbl, p->nr_power_tbl);
#endif
#endif

out:
	FUNC_EXIT(FUNC_LV_LOCAL);

	return ret;
}

static int _mt_cpufreq_setup_freqs_table(struct cpufreq_policy *policy,
					 struct mt_cpu_freq_info *freqs, int num)
{
	struct mt_cpu_dvfs *p;
	struct cpufreq_frequency_table *table;
	int i, ret = 0;

	FUNC_ENTER(FUNC_LV_LOCAL);

	BUG_ON(NULL == policy);
	BUG_ON(NULL == freqs);

	p = id_to_cpu_dvfs(_get_cpu_dvfs_id(policy->cpu));

	if (NULL == p->freq_tbl_for_cpufreq) {
		table = kzalloc((num + 1) * sizeof(*table), GFP_KERNEL);

		if (NULL == table) {
			ret = -ENOMEM;
			goto out;
		}

		for (i = 0; i < num; i++) {
			table[i].driver_data = i;
			table[i].frequency = freqs[i].cpufreq_khz;
		}

		table[num].driver_data = i;	/* TODO: FIXME, why need this??? */
		table[num].frequency = CPUFREQ_TABLE_END;

		p->opp_tbl = freqs;
		p->nr_opp_tbl = num;
		p->freq_tbl_for_cpufreq = table;
	}
#ifdef CONFIG_CPU_FREQ
	ret = cpufreq_frequency_table_cpuinfo(policy, p->freq_tbl_for_cpufreq);

#if 0	/* not available for kernel 3.18 */
	if (!ret)
		cpufreq_frequency_table_get_attr(p->freq_tbl_for_cpufreq, policy->cpu);
#else
	if (!ret)
		policy->freq_table = p->freq_tbl_for_cpufreq;
#endif
#endif

	if (NULL == p->power_tbl)
		p->ops->setup_power_table(p);

out:
	FUNC_EXIT(FUNC_LV_LOCAL);

	return 0;
}

void mt_cpufreq_enable_by_ptpod(enum mt_cpu_dvfs_id id)
{
	struct mt_cpu_dvfs *p = id_to_cpu_dvfs(id);

	FUNC_ENTER(FUNC_LV_API);

	BUG_ON(NULL == p);

	p->dvfs_disable_by_ptpod = false;

	/* Turbo mode is enabled if efuse turbo bit is set */
#ifdef CONFIG_CPU_DVFS_TURBO_MODE
	/* TODO: add turbo bit check HERE! */
	if (0) {
		cpufreq_info("@%s: Turbo mode enabled!\n", __func__);
		p->turbo_mode = 1;
	}
#endif

	if (!cpu_dvfs_is_available(p)) {
		FUNC_EXIT(FUNC_LV_API);
		return;
	}

	_mt_cpufreq_set(id, p->idx_opp_tbl_for_late_resume);

	FUNC_EXIT(FUNC_LV_API);
}
EXPORT_SYMBOL(mt_cpufreq_enable_by_ptpod);

unsigned int mt_cpufreq_disable_by_ptpod(enum mt_cpu_dvfs_id id)
{
	struct mt_cpu_dvfs *p = id_to_cpu_dvfs(id);

	FUNC_ENTER(FUNC_LV_API);

	BUG_ON(NULL == p);

	p->dvfs_disable_by_ptpod = true;

	if (!cpu_dvfs_is_available(p)) {
		FUNC_EXIT(FUNC_LV_API);
		return 0;
	}

	p->idx_opp_tbl_for_late_resume = p->idx_opp_tbl;
	_mt_cpufreq_set(id, p->idx_normal_max_opp);	/* XXX: useless, decided @ _mt_cpufreq_calc_new_opp_idx() */

	FUNC_EXIT(FUNC_LV_API);

	return cpu_dvfs_get_cur_freq(p);
}
EXPORT_SYMBOL(mt_cpufreq_disable_by_ptpod);

int mt_cpufreq_set_lte_volt(int pmic_val)
{
	pmic_set_register_value(PMIC_VLTE_VOSEL_ON, pmic_val);
	return 0;
}
EXPORT_SYMBOL(mt_cpufreq_set_lte_volt);

void mt_cpufreq_thermal_protect(unsigned int limited_power)
{
	FUNC_ENTER(FUNC_LV_API);

	cpufreq_info("%s(): limited_power = %d\n", __func__, limited_power);

#ifdef CONFIG_CPU_FREQ
	{
		struct cpufreq_policy *policy;
		struct mt_cpu_dvfs *p;
		int possible_cpu;
		int found = 0;
		unsigned long flags;

		policy = cpufreq_cpu_get(0);	/* TODO: FIXME if it has more than one DVFS policy */
		if (NULL == policy) {
			cpufreq_warn("@%s: No DVFS policy, DVFS driver not init yet!\n", __func__);
			goto no_policy;
		}

		p = id_to_cpu_dvfs(_get_cpu_dvfs_id(policy->cpu));

		BUG_ON(NULL == p);

		cpufreq_lock(flags);	/* <- lock */

		/* save current oppidx */
		if (!p->limited_power_by_thermal)
			p->idx_opp_tbl_for_thermal_thro = p->idx_opp_tbl;

		p->limited_power_by_thermal = limited_power;
		possible_cpu = num_possible_cpus();

		/* no limited */
		if (0 == limited_power) {
#ifdef CONFIG_ARCH_MT6753
			if (p->cpu_level == CPU_LEVEL_1) {
				/* give a large budget to find the first safe limit combination */
				_mt_cpufreq_set_limit_by_pwr_budget(99999);
			} else {
				p->limited_max_ncpu = possible_cpu;
				p->limited_max_freq = cpu_dvfs_get_max_freq(p);
				p->limited_power_idx = 0;
			}
#else
			p->limited_max_ncpu = possible_cpu;
			p->limited_max_freq = cpu_dvfs_get_max_freq(p);
			p->limited_power_idx = 0;
#endif
			/* restore oppidx */
			p->idx_opp_tbl = p->idx_opp_tbl_for_thermal_thro;
		} else
			found = _mt_cpufreq_set_limit_by_pwr_budget(limited_power);

		cpufreq_dbg(
			"@%s: found = %d, limited_power_idx = %d, limited_max_freq = %d, limited_max_ncpu = %d\n",
			__func__, found, p->limited_power_idx, p->limited_max_freq, p->limited_max_ncpu);

		cpufreq_unlock(flags);	/* <- unlock */

		hps_set_cpu_num_limit(LIMIT_THERMAL, p->limited_max_ncpu, 0);

		/* correct opp idx will be calcualted in _mt_cpufreq_power_limited_verify() */
		_mt_cpufreq_set(MT_CPU_DVFS_LITTLE, -1);
		cpufreq_cpu_put(policy);	/* <- policy put */
	}
no_policy:
#endif

	FUNC_EXIT(FUNC_LV_API);
}
EXPORT_SYMBOL(mt_cpufreq_thermal_protect);

void mt_cpufreq_thermal_5A_limit(bool enable)
{
	FUNC_ENTER(FUNC_LV_API);

	cpufreq_info("%s(): PMIC 5A limit = %d\n", __func__, enable);

#ifdef CONFIG_ARCH_MT6753
	{
		struct mt_cpu_dvfs *p = id_to_cpu_dvfs(MT_CPU_DVFS_LITTLE);

		pmic_5A_throttle_on = enable;

		if (cpu_dvfs_is_available(p))
			_mt_cpufreq_set(MT_CPU_DVFS_LITTLE, -1);
	}
#endif

	FUNC_EXIT(FUNC_LV_API);
}
EXPORT_SYMBOL(mt_cpufreq_thermal_5A_limit);


#ifdef CONFIG_CPU_DVFS_POWER_THROTTLING
static void _mt_cpufreq_calc_power_throttle_idx(struct mt_cpu_dvfs *p)
{
	FUNC_ENTER(FUNC_LV_HELP);

	cpufreq_ver("%s(): original idx = %d\n", __func__, p->idx_pwr_thro_max_opp);

	if (!p->pwr_thro_mode)
		p->idx_pwr_thro_max_opp = 0;
	else if ((p->pwr_thro_mode & PWR_THRO_MODE_LBAT_819MHZ)
		 || (p->pwr_thro_mode & PWR_THRO_MODE_BAT_PER_819MHZ)) {
		switch (p->cpu_level) {
#ifdef CONFIG_ARCH_MT6735M
		case CPU_LEVEL_0:
		case CPU_LEVEL_1:
			p->idx_pwr_thro_max_opp = 3;
			break;
		case CPU_LEVEL_2:
		case CPU_LEVEL_3:
			p->idx_pwr_thro_max_opp = 3;
			break;
#else				/* 6735/6753 */
		case CPU_LEVEL_0:
		case CPU_LEVEL_1:
			p->idx_pwr_thro_max_opp = 4;
			break;
#ifdef CONFIG_ARCH_MT6735
		case CPU_LEVEL_2:
			p->idx_pwr_thro_max_opp = 1;
			break;
		case CPU_LEVEL_3:
			p->idx_pwr_thro_max_opp = 4;
			break;
		case CPU_LEVEL_4:
			p->idx_pwr_thro_max_opp = 3;
			break;
#endif
#endif
		default:
			break;
		}
	} else if (p->pwr_thro_mode & PWR_THRO_MODE_BAT_OC_1040MHZ) {
		switch (p->cpu_level) {
#ifdef CONFIG_ARCH_MT6735M
		case CPU_LEVEL_0:
		case CPU_LEVEL_2:
		case CPU_LEVEL_3:
			p->idx_pwr_thro_max_opp = 1;
			break;
		case CPU_LEVEL_1:
			p->idx_pwr_thro_max_opp = 0;
			break;
#else				/* 6735/6753 */
		case CPU_LEVEL_0:
		case CPU_LEVEL_1:
			p->idx_pwr_thro_max_opp = 3;
			break;
#ifdef CONFIG_ARCH_MT6735
		case CPU_LEVEL_2:
			p->idx_pwr_thro_max_opp = 0;
			break;
		case CPU_LEVEL_3:
			p->idx_pwr_thro_max_opp = 3;
			break;
		case CPU_LEVEL_4:
			p->idx_pwr_thro_max_opp = 1;
			break;
#endif
#endif
		default:
			break;
		}
	}

	cpufreq_ver("%s(): new idx = %d\n", __func__, p->idx_pwr_thro_max_opp);

	FUNC_EXIT(FUNC_LV_HELP);
}

static void _mt_cpufreq_power_throttle_bat_per_CB(BATTERY_PERCENT_LEVEL level)
{
	struct mt_cpu_dvfs *p;
	int i;
	unsigned long flags;

	cpufreq_dbg("@%s: level: %d\n", __func__, level);

	for_each_cpu_dvfs(i, p) {
		if (!cpu_dvfs_is_available(p))
			continue;

		cpufreq_lock(flags);

		if (!p->pwr_thro_mode)
			p->idx_opp_tbl_for_pwr_thro = p->idx_opp_tbl;

		switch (level) {
		case BATTERY_PERCENT_LEVEL_1:
			/* Trigger CPU Limit to under 819M */
			p->pwr_thro_mode |= PWR_THRO_MODE_BAT_PER_819MHZ;
			break;
		default:
			/* Unlimit CPU */
			p->pwr_thro_mode &= ~PWR_THRO_MODE_BAT_PER_819MHZ;
			break;
		}

		_mt_cpufreq_calc_power_throttle_idx(p);

		if (!p->pwr_thro_mode)
			p->idx_opp_tbl = p->idx_opp_tbl_for_pwr_thro;

		cpufreq_unlock(flags);

#ifdef CONFIG_ARCH_MT6753
		switch (level) {
		case BATTERY_PERCENT_LEVEL_1:
			/* Limit CPU core num to 4 */
			hps_set_cpu_num_limit(LIMIT_LOW_BATTERY, 4, 0);
			break;
		default:
			/* Unlimit CPU core num if no other limit */
			if (!p->pwr_thro_mode)
				hps_set_cpu_num_limit(LIMIT_LOW_BATTERY, num_possible_cpus(), 0);
			break;
		}
#endif

		_mt_cpufreq_set(MT_CPU_DVFS_LITTLE, -1);
	}
}

static void _mt_cpufreq_power_throttle_bat_oc_CB(BATTERY_OC_LEVEL level)
{
	struct mt_cpu_dvfs *p;
	int i;
	unsigned long flags;

	cpufreq_dbg("@%s: level: %d\n", __func__, level);

	for_each_cpu_dvfs(i, p) {
		if (!cpu_dvfs_is_available(p))
			continue;

		cpufreq_lock(flags);

		if (!p->pwr_thro_mode)
			p->idx_opp_tbl_for_pwr_thro = p->idx_opp_tbl;

		switch (level) {
		case BATTERY_OC_LEVEL_1:
			/* Trigger CPU Limit to under 1G */
			p->pwr_thro_mode |= PWR_THRO_MODE_BAT_OC_1040MHZ;
			break;
		default:
			/* Unlimit CPU */
			p->pwr_thro_mode &= ~PWR_THRO_MODE_BAT_OC_1040MHZ;
			break;
		}

		_mt_cpufreq_calc_power_throttle_idx(p);

		if (!p->pwr_thro_mode)
			p->idx_opp_tbl = p->idx_opp_tbl_for_pwr_thro;

		cpufreq_unlock(flags);

#ifdef CONFIG_ARCH_MT6753
		switch (level) {
		case BATTERY_OC_LEVEL_1:
			/* Limit CPU core num to 4 */
			hps_set_cpu_num_limit(LIMIT_LOW_BATTERY, 4, 0);
			break;
		default:
			/* Unlimit CPU core num if no other limit */
			if (!p->pwr_thro_mode)
				hps_set_cpu_num_limit(LIMIT_LOW_BATTERY, num_possible_cpus(), 0);
			break;
		}
#endif

		_mt_cpufreq_set(MT_CPU_DVFS_LITTLE, -1);
	}
}

void _mt_cpufreq_power_throttle_low_bat_CB(LOW_BATTERY_LEVEL level)
{
	struct mt_cpu_dvfs *p;
	int i;
	unsigned long flags;

	cpufreq_dbg("@%s: level: %d\n", __func__, level);

	for_each_cpu_dvfs(i, p) {
		if (!cpu_dvfs_is_available(p))
			continue;

		cpufreq_lock(flags);

		if (!p->pwr_thro_mode)
			p->idx_opp_tbl_for_pwr_thro = p->idx_opp_tbl;

		switch (level) {
		case LOW_BATTERY_LEVEL_1:
		case LOW_BATTERY_LEVEL_2:
			/* 1st and 2nd LV trigger CPU Limit to under 819M */
			p->pwr_thro_mode |= PWR_THRO_MODE_LBAT_819MHZ;
			break;
		default:
			/* Unlimit CPU */
			p->pwr_thro_mode &= ~PWR_THRO_MODE_LBAT_819MHZ;
			break;
		}

		_mt_cpufreq_calc_power_throttle_idx(p);

		if (!p->pwr_thro_mode)
			p->idx_opp_tbl = p->idx_opp_tbl_for_pwr_thro;

		cpufreq_unlock(flags);

#ifdef CONFIG_ARCH_MT6753
		switch (level) {
		case LOW_BATTERY_LEVEL_1:
		case LOW_BATTERY_LEVEL_2:
			/* Limit CPU core num to 4 */
			hps_set_cpu_num_limit(LIMIT_LOW_BATTERY, 4, 0);
			break;
		default:
			/* Unlimit CPU core num if no other limit */
			if (!p->pwr_thro_mode)
				hps_set_cpu_num_limit(LIMIT_LOW_BATTERY, num_possible_cpus(), 0);
			break;
		}
#endif

		_mt_cpufreq_set(MT_CPU_DVFS_LITTLE, -1);
	}
}
#endif

/*
 * cpufreq driver
 */
#ifdef CONFIG_CPU_FREQ
static int _mt_cpufreq_verify(struct cpufreq_policy *policy)
{
	struct mt_cpu_dvfs *p;
	int ret = 0;	/* cpufreq_frequency_table_verify() always return 0 */

	FUNC_ENTER(FUNC_LV_MODULE);

	p = id_to_cpu_dvfs(_get_cpu_dvfs_id(policy->cpu));

	BUG_ON(NULL == p);

#ifdef CONFIG_CPU_FREQ
	ret = cpufreq_frequency_table_verify(policy, p->freq_tbl_for_cpufreq);
#endif

	FUNC_EXIT(FUNC_LV_MODULE);

	return ret;
}

static int _mt_cpufreq_target(struct cpufreq_policy *policy, unsigned int target_freq,
				  unsigned int relation)
{
	unsigned int new_opp_idx;
	enum mt_cpu_dvfs_id id = _get_cpu_dvfs_id(policy->cpu);
	int ret = 0;	/* -EINVAL; */

	FUNC_ENTER(FUNC_LV_MODULE);

	if (policy->cpu >= num_possible_cpus()
		|| cpufreq_frequency_table_target(policy, id_to_cpu_dvfs(id)->freq_tbl_for_cpufreq,
						  target_freq, relation, &new_opp_idx)
		|| (id_to_cpu_dvfs(id) && id_to_cpu_dvfs(id)->dvfs_disable_by_procfs)
		)
		return -EINVAL;

#ifdef CONFIG_CPU_DVFS_AEE_RR_REC
	aee_rr_rec_cpu_dvfs_status(aee_rr_curr_cpu_dvfs_status() |
				   (1 << CPU_DVFS_LITTLE_IS_DOING_DVFS));
#endif

	_mt_cpufreq_set(id, new_opp_idx);

#ifdef CONFIG_CPU_DVFS_AEE_RR_REC
	aee_rr_rec_cpu_dvfs_status(
		aee_rr_curr_cpu_dvfs_status() & ~(1 << CPU_DVFS_LITTLE_IS_DOING_DVFS));
#endif

	FUNC_EXIT(FUNC_LV_MODULE);

	return ret;
}

static int _mt_cpufreq_init(struct cpufreq_policy *policy)
{
	int ret = -EINVAL;

	FUNC_ENTER(FUNC_LV_MODULE);

	if (policy->cpu >= num_possible_cpus())
		return -EINVAL;

	cpufreq_info("@%s: num_possible_cpus: %d\n", __func__, num_possible_cpus());

	policy->shared_type = CPUFREQ_SHARED_TYPE_ANY;
	cpumask_setall(policy->cpus);

	/*******************************************************
	 * 1 us, assumed, will be overwrited by min_sampling_rate
	 ********************************************************/
	policy->cpuinfo.transition_latency = 1000;

	/*********************************************
	 * set default policy and cpuinfo, unit : Khz
	 **********************************************/
	{
#define DORMANT_MODE_VOLT   85000

		enum mt_cpu_dvfs_id id = _get_cpu_dvfs_id(policy->cpu);
		struct mt_cpu_dvfs *p = id_to_cpu_dvfs(id);
		unsigned int lv = _mt_cpufreq_get_cpu_level();
		struct opp_tbl_info *opp_tbl_info = &opp_tbls[lv];

		BUG_ON(NULL == p);
		BUG_ON(!(lv == CPU_LEVEL_0 || lv == CPU_LEVEL_1 || lv == CPU_LEVEL_2
					|| lv == CPU_LEVEL_3 || lv == CPU_LEVEL_4));

		p->cpu_level = lv;

#ifdef CONFIG_ARCH_MT6753
		/* check 5A throttle */
		if (p->cpu_level == CPU_LEVEL_1 && !cpu_dvfs_is_extbuck_valid()) {
			pmic_5A_throttle_enable = true;
			cpufreq_info("@%s: PMIC 5A throttle enabled!\n", __func__);
		}
#endif

#ifndef CONFIG_CPU_DVFS_HAS_EXTBUCK
		/* make sure Vproc & Vsram in normal mode path */
		pmic_set_register_value(PMIC_VPROC_VOSEL_CTRL, 0x1);
		pmic_set_register_value(PMIC_VSRAM_VOSEL_CTRL, 0x1);
		/* set dpidle volt for Vproc & Vsram */
		pmic_set_register_value(PMIC_VPROC_VOSEL, VOLT_TO_PMIC_VAL(DORMANT_MODE_VOLT));
		pmic_set_register_value(PMIC_VSRAM_VOSEL_RSV, VOLT_TO_PMIC_VAL(DORMANT_MODE_VOLT));
#else
		if (cpu_dvfs_is_extbuck_valid()) {
			p->ops = &dvfs_ops_extbuck;

			cpufreq_info("@%s: Has ExtBuck!\n", __func__);

			/* set dpidle volt for Vproc */
			mt6311_set_vdvfs11_vosel_ctrl(0x1);
			mt6311_set_vdvfs11_vosel(VOLT_TO_EXTBUCK_VAL(DORMANT_MODE_VOLT));

			cpufreq_info("@%s: cur_vsram = %dmV, cur_vproc = %dmV\n",
					 __func__, _mt_cpufreq_get_cur_vsram(p) / 100,
					 (p->ops->get_cur_volt(p)) / 100);
		} else {
			cpufreq_info("@%s: No ExtBuck!\n", __func__);

			/* make sure Vproc & Vsram in normal mode path */
			pmic_set_register_value(PMIC_VPROC_VOSEL_CTRL, 0x1);
			pmic_set_register_value(PMIC_VSRAM_VOSEL_CTRL, 0x1);
			/* set dpidle volt for Vproc & Vsram */
			pmic_set_register_value(
				PMIC_VPROC_VOSEL, VOLT_TO_PMIC_VAL(DORMANT_MODE_VOLT));
			pmic_set_register_value(
				PMIC_VSRAM_VOSEL_RSV, VOLT_TO_PMIC_VAL(DORMANT_MODE_VOLT));
		}
#endif

		ret = _mt_cpufreq_setup_freqs_table(
				policy, opp_tbl_info->opp_tbl, opp_tbl_info->size);

		policy->cpuinfo.max_freq = cpu_dvfs_get_max_freq(id_to_cpu_dvfs(id));
		policy->cpuinfo.min_freq = cpu_dvfs_get_min_freq(id_to_cpu_dvfs(id));

		policy->cur = _mt_cpufreq_get_cur_phy_freq(p);	/* use cur phy freq is better */
		policy->max = cpu_dvfs_get_max_freq(id_to_cpu_dvfs(id));
		policy->min = cpu_dvfs_get_min_freq(id_to_cpu_dvfs(id));

		if (_mt_cpufreq_sync_opp_tbl_idx(p) >= 0)
			p->idx_normal_max_opp = p->idx_opp_tbl;

		/* restore default volt, sync opp idx, set default limit */
		_mt_cpufreq_restore_default_volt(p);

		/* reset thermal/PBM limit */
#ifndef DISABLE_PBM_FEATURE
		p->limited_power_by_pbm = 0;
#endif
		p->limited_power_by_thermal = 0;
#ifdef CONFIG_ARCH_MT6753
		if (p->cpu_level == CPU_LEVEL_1) {
			/* give a large budget to find the first safe limit combination */
			_mt_cpufreq_set_limit_by_pwr_budget(99999);
		} else {
			p->limited_max_freq = cpu_dvfs_get_max_freq(p);
			p->limited_max_ncpu = num_possible_cpus();
			p->limited_power_idx = 0;
		}
#else
		p->limited_max_freq = cpu_dvfs_get_max_freq(p);
		p->limited_max_ncpu = num_possible_cpus();
		p->limited_power_idx = 0;
#endif

		cpufreq_info("@%s: limited_power_idx = %d\n", __func__, p->limited_power_idx);

#ifdef CONFIG_CPU_DVFS_SYSTEM_BOOTUP_BOOST
		p->limited_min_freq_by_kdriver = cpu_dvfs_get_max_freq(p);
#endif

#ifdef CONFIG_CPU_DVFS_POWER_THROTTLING
		register_battery_percent_notify(
			&_mt_cpufreq_power_throttle_bat_per_CB,	BATTERY_PERCENT_PRIO_CPU_L);
		register_battery_oc_notify(
			&_mt_cpufreq_power_throttle_bat_oc_CB, BATTERY_OC_PRIO_CPU_L);
		register_low_battery_notify(
			&_mt_cpufreq_power_throttle_low_bat_CB, LOW_BATTERY_PRIO_CPU_L);
#endif
	}

#ifdef CONFIG_ARCH_MT6753
	/* save VSRAM_VOSEL_OFFSET and DELTA init value to deepidle CMD8 */
	mt_cpufreq_set_pmic_cmd(PMIC_WRAP_PHASE_DEEPIDLE,
				IDX_DI_VSRAM_NORMAL,
				(pmic_get_register_value(PMIC_VSRAM_VOSEL_OFFSET) << 8) |
				pmic_get_register_value(PMIC_VSRAM_VOSEL_DELTA)
		);
#endif

	if (ret)
		cpufreq_err("failed to setup frequency table\n");
	else
		cpufreq_info("CPU DVFS init done!\n");

	FUNC_EXIT(FUNC_LV_MODULE);

	return ret;
}

static unsigned int _mt_cpufreq_get(unsigned int cpu)
{
	struct mt_cpu_dvfs *p;

	FUNC_ENTER(FUNC_LV_MODULE);

	p = id_to_cpu_dvfs(_get_cpu_dvfs_id(cpu));

	BUG_ON(NULL == p);

	FUNC_EXIT(FUNC_LV_MODULE);

	return cpu_dvfs_get_cur_freq(p);
}
#endif


/*
 * Early suspend
 */
bool mt_cpufreq_earlysuspend_status_get(void)
{
	return _allow_dpidle_ctrl_vproc;
}
EXPORT_SYMBOL(mt_cpufreq_earlysuspend_status_get);

static void _mt_cpufreq_lcm_status_switch(int onoff)
{
	struct mt_cpu_dvfs *p;
	int i;

	cpufreq_info("@%s: LCM is %s\n", __func__, (onoff) ? "on" : "off");

	/* onoff = 0: LCM OFF */
	/* others: LCM ON */
	if (onoff) {
		_allow_dpidle_ctrl_vproc = false;

		for_each_cpu_dvfs(i, p) {
			if (!cpu_dvfs_is_available(p))
				continue;

			p->dvfs_disable_by_early_suspend = false;

			if (is_fix_freq_in_ES) {
#ifdef CONFIG_CPU_FREQ
				struct cpufreq_policy *policy = cpufreq_cpu_get(p->cpu_id);

				if (policy) {
					cpufreq_driver_target(
						policy,
						cpu_dvfs_get_freq_by_idx(
							p,
							p->idx_opp_tbl_for_late_resume
						),
						CPUFREQ_RELATION_L
					);
					cpufreq_cpu_put(policy);
				}
#endif
			}
		}
	} else {
		for_each_cpu_dvfs(i, p) {
			if (!cpu_dvfs_is_available(p))
				continue;

			p->dvfs_disable_by_early_suspend = true;

			p->idx_opp_tbl_for_late_resume = p->idx_opp_tbl;

			if (is_fix_freq_in_ES) {
#ifdef CONFIG_CPU_FREQ
				struct cpufreq_policy *policy = cpufreq_cpu_get(p->cpu_id);

				if (policy) {
					cpufreq_driver_target(
						policy,
						cpu_dvfs_get_normal_max_freq(p), CPUFREQ_RELATION_L);
					cpufreq_cpu_put(policy);
				}
#endif
			}
		}
		_allow_dpidle_ctrl_vproc = true;
	}
}

static int _mt_cpufreq_fb_notifier_callback(struct notifier_block *self, unsigned long event, void *data)
{
	struct fb_event *evdata = data;
	int blank;

	FUNC_ENTER(FUNC_LV_MODULE);

	blank = *(int *)evdata->data;
	cpufreq_ver("@%s: blank = %d, event = %lu\n", __func__, blank, event);

	/* skip if it's not a blank event */
	if (event != FB_EVENT_BLANK)
		return 0;

	switch (blank) {
	/* LCM ON */
	case FB_BLANK_UNBLANK:
		_mt_cpufreq_lcm_status_switch(1);
		break;
	/* LCM OFF */
	case FB_BLANK_POWERDOWN:
		_mt_cpufreq_lcm_status_switch(0);
		break;
	default:
		break;
	}

	FUNC_EXIT(FUNC_LV_MODULE);

	return 0;
}

static int _mt_cpufreq_suspend(struct device *dev)
{
	/* struct cpufreq_policy *policy; */
	struct mt_cpu_dvfs *p;
	int i;

	FUNC_ENTER(FUNC_LV_MODULE);

	for_each_cpu_dvfs(i, p) {
		if (!cpu_dvfs_is_available(p))
			continue;

		p->dvfs_disable_by_suspend = true;

		/* XXX: useless, decided @ _mt_cpufreq_calc_new_opp_idx() */
		_mt_cpufreq_set(MT_CPU_DVFS_LITTLE, p->idx_normal_max_opp);
	}

	FUNC_EXIT(FUNC_LV_MODULE);

	return 0;
}

static int _mt_cpufreq_resume(struct device *dev)
{
	struct mt_cpu_dvfs *p;
	int i;

	FUNC_ENTER(FUNC_LV_MODULE);

	for_each_cpu_dvfs(i, p) {
		if (!cpu_dvfs_is_available(p))
			continue;

		p->dvfs_disable_by_suspend = false;
	}

	FUNC_EXIT(FUNC_LV_MODULE);

	return 0;
}

static int _mt_cpufreq_pm_restore_early(struct device *dev)	/* for IPO-H HW(freq) / SW(opp_tbl_idx) */
{
	struct mt_cpu_dvfs *p;
	int i;

	FUNC_ENTER(FUNC_LV_MODULE);

	for_each_cpu_dvfs(i, p) {
		if (cpu_dvfs_is_available(p))
			_mt_cpufreq_sync_opp_tbl_idx(p);
	}

	FUNC_EXIT(FUNC_LV_MODULE);

	return 0;
}


/*
 * Platform driver
 */
static int _mt_cpufreq_pdrv_probe(struct platform_device *pdev)
{
	FUNC_ENTER(FUNC_LV_MODULE);

#ifdef CONFIG_CPU_DVFS_PERFORMANCE_TEST
	{
		struct mt_cpu_dvfs *p = id_to_cpu_dvfs(0);

		/* fix at max freq(FY) and do not enable DVFS */
		pmic_set_register_value(PMIC_VPROC_VOSEL_ON, VOLT_TO_PMIC_VAL(125000));
		udelay(300);
#ifdef CONFIG_ARCH_MT6735M
		_mt_cpufreq_dfs_by_set_armpll(p, _mt_cpufreq_cpu_dds_calc(CPU_DVFS_FREQ2), 8);
#else
		_mt_cpufreq_dfs_by_set_armpll(p, _mt_cpufreq_cpu_dds_calc(CPU_DVFS_FREQ1), 8);
#endif
		cpufreq_info("@%s: Disable DVFS and fix freq at %dKHz\n",
				__func__, _mt_cpufreq_get_cur_phy_freq(p));

		return 0;
	}
#endif

	if (pw.addr[0].cmd_addr == 0)
		_mt_cpufreq_pmic_table_init();

	/* init static power table */
	mt_spower_init();

#ifdef CONFIG_CPU_DVFS_AEE_RR_REC
	_mt_cpufreq_aee_init();
#endif

	/* register early suspend */
	if (fb_register_client(&_mt_cpufreq_fb_notifier)) {
		cpufreq_err("@%s: register FB client failed!\n", __func__);
		return 0;
	}

	/* init PMIC_WRAP & volt */
	mt_cpufreq_set_pmic_phase(PMIC_WRAP_PHASE_NORMAL);

#ifdef CONFIG_CPU_FREQ
	cpufreq_register_driver(&_mt_cpufreq_driver);
#endif

	register_hotcpu_notifier(&_mt_cpufreq_cpu_notifier);	/* <-XXX */

	cpufreq_info("CPU DVFS driver probe done\n");

	FUNC_EXIT(FUNC_LV_MODULE);

	return 0;
}

static int _mt_cpufreq_pdrv_remove(struct platform_device *pdev)
{
	FUNC_ENTER(FUNC_LV_MODULE);

	unregister_hotcpu_notifier(&_mt_cpufreq_cpu_notifier);	/* <-XXX */

#ifdef CONFIG_CPU_FREQ
	cpufreq_unregister_driver(&_mt_cpufreq_driver);
#endif

	FUNC_EXIT(FUNC_LV_MODULE);

	return 0;
}


#ifndef __KERNEL__
/*
 * For CTP
 */
static unsigned int _mt_get_cpu_freq(void);

int mt_cpufreq_pdrv_probe(void)
{
	static struct cpufreq_policy policy;

	_mt_cpufreq_pdrv_probe(NULL);

	policy.cpu = cpu_dvfs[MT_CPU_DVFS_LITTLE].cpu_id;
	_mt_cpufreq_init(&policy);

#ifdef __MTK_SLT_
	/* modify SLT DVFS table for HV/NV/LV */
#ifdef CONFIG_ARCH_MT6735M
#if defined(SLT_VMAX)
	cpu_dvfs[MT_CPU_DVFS_LITTLE].opp_tbl[0].cpufreq_volt = 131250;
#elif defined(SLT_VMIN)
	cpu_dvfs[MT_CPU_DVFS_LITTLE].opp_tbl[0].cpufreq_volt = 113750;
#else
	cpu_dvfs[MT_CPU_DVFS_LITTLE].opp_tbl[0].cpufreq_volt = 125000;
#endif
#else	/* !CONFIG_ARCH_MT6735M */
#if defined(SLT_VMAX)
	cpu_dvfs[MT_CPU_DVFS_LITTLE].opp_tbl[0].cpufreq_volt = 126875;
#elif defined(SLT_VMIN)
	cpu_dvfs[MT_CPU_DVFS_LITTLE].opp_tbl[0].cpufreq_volt = 109375;
#else
	cpu_dvfs[MT_CPU_DVFS_LITTLE].opp_tbl[0].cpufreq_volt = 115000;
#endif
#endif

	/* fix CPU at Max Freq */
	mt_cpufreq_set_freq(MT_CPU_DVFS_LITTLE, 0);
	cpufreq_info("set CPU freq to %d KHz\n", _mt_get_cpu_freq());
#endif

	return 0;
}

int mt_cpufreq_set_opp_volt(enum mt_cpu_dvfs_id id, int idx)
{
	int ret = 0;
	static struct opp_tbl_info *info;
	struct mt_cpu_dvfs *p = id_to_cpu_dvfs(id);

	info = &opp_tbls[(p->cpu_level)];
	if (idx >= info->size)
		return -1;

	return _mt_cpufreq_set_cur_volt_locked(p, info->opp_tbl[idx].cpufreq_volt);
}

int mt_cpufreq_set_freq(enum mt_cpu_dvfs_id id, int idx)
{
	unsigned int cur_freq;
	unsigned int target_freq;
	int ret;
	struct mt_cpu_dvfs *p = id_to_cpu_dvfs(id);

	cur_freq = p->ops->get_cur_phy_freq(p);
	target_freq = cpu_dvfs_get_freq_by_idx(p, idx);

	ret = _mt_cpufreq_set_locked(p, cur_freq, target_freq, NULL);

	if (ret < 0)
		return ret;

	p->idx_opp_tbl = idx;

	return target_freq;
}

#include "dvfs.h"

static unsigned int _mt_get_cpu_freq(void)
{
	int output = 0;
	int i = 0;
	unsigned int temp, clk26cali_0, clk_cfg_8, clk_misc_cfg_1;

	clk26cali_0 = DRV_Reg32(CLK26CALI_0);
	DRV_WriteReg32(CLK26CALI_0, clk26cali_0 | 0x80);	/* enable fmeter_en */

	clk_misc_cfg_1 = DRV_Reg32(CLK_MISC_CFG_1);
	DRV_WriteReg32(CLK_MISC_CFG_1, 0xFFFF0300);	/* select divider */

	clk_cfg_8 = DRV_Reg32(CLK_CFG_8);
	DRV_WriteReg32(CLK_CFG_8, (39 << 8));	/* select abist_cksw */

	temp = DRV_Reg32(CLK26CALI_0);
	DRV_WriteReg32(CLK26CALI_0, temp | 0x1);	/* start fmeter */

	/* wait frequency meter finish */
	while (DRV_Reg32(CLK26CALI_0) & 0x1) {
		mdelay(10);
		i++;
		if (i > 10)
			break;
	}

	temp = DRV_Reg32(CLK26CALI_1) & 0xFFFF;

	output = ((temp * 26000) / 1024) * 4;	/* Khz */

	DRV_WriteReg32(CLK_CFG_8, clk_cfg_8);
	DRV_WriteReg32(CLK_MISC_CFG_1, clk_misc_cfg_1);
	DRV_WriteReg32(CLK26CALI_0, clk26cali_0);

	if (i > 10)
		return 0;
	else
		return output;
}

unsigned int dvfs_get_cpu_freq(enum mt_cpu_dvfs_id id)
{
#ifdef __MTK_SLT_
	struct mt_cpu_dvfs *p = id_to_cpu_dvfs(id);

	return _mt_cpufreq_get_cur_phy_freq(p);
#else
	return _mt_get_cpu_freq();
#endif
}

void dvfs_set_cpu_freq_FH(enum mt_cpu_dvfs_id id, int freq)
{
	struct mt_cpu_dvfs *p = id_to_cpu_dvfs(id);
	int idx;

	if (!p) {
		cpufreq_err("%s(%d, %d), id is wrong\n", __func__, id, freq);
		return;
	}

	idx = _mt_cpufreq_get_idx_by_freq(p, freq, CPUFREQ_RELATION_H);

	if (-1 == idx) {
		cpufreq_err("%s(%d, %d), freq is wrong\n", __func__, id, freq);
		return;
	}

	mt_cpufreq_set_freq(id, idx);
}

unsigned int dvfs_get_cur_oppidx(enum mt_cpu_dvfs_id id)
{
	struct mt_cpu_dvfs *p = id_to_cpu_dvfs(id);

	return p->idx_opp_tbl;
}

unsigned int cpu_frequency_output_slt(enum mt_cpu_dvfs_id id)
{
	return (MT_CPU_DVFS_LITTLE == id) ? _mt_get_cpu_freq() : 0;
}

unsigned int mt_get_cur_volt_lte(void)	/* volt: mv * 100 */
{
	unsigned int volt = 0;

	volt = pmic_get_register_value(PMIC_VLTE_VOSEL_ON);
	volt = PMIC_VAL_TO_VOLT(volt);

	return volt;
}

unsigned int dvfs_get_cpu_volt(enum mt_cpu_dvfs_id id)	/* volt: mv * 100 */
{
	struct mt_cpu_dvfs *p = id_to_cpu_dvfs(id);
	unsigned int volt = 0;

	volt = p->ops->get_cur_volt(p);

	cpufreq_dbg("%s(%d) volt = %d\n", __func__, id, volt);

	return volt;
}

void dvfs_set_cpu_volt(enum mt_cpu_dvfs_id id, int volt)	/* volt: mv * 100 */
{
	struct mt_cpu_dvfs *p = id_to_cpu_dvfs(id);

	cpufreq_dbg("%s(%d, %d)\n", __func__, id, volt);

	if (!p) {
		cpufreq_err("%s(%d, %d), id is wrong\n", __func__, id, volt);
		return;
	}

	if (_mt_cpufreq_set_cur_volt_locked(p, volt))
		cpufreq_err("%s(%d, %d), set volt fail\n", __func__, id, volt);


	cpufreq_dbg("%s(%d, %d) Vproc = %d\n", __func__, id, volt, p->ops->get_cur_volt(p));
#ifdef CONFIG_CPU_DVFS_HAS_EXTBUCK
	cpufreq_dbg("%s, Vsram = %d\n", __func__, _mt_cpufreq_get_cur_vsram(p));
#endif
}

#if 0
void dvfs_set_gpu_volt(int pmic_val)
{
	cpufreq_dbg("%s(%d)\n", __func__, pmic_val);
	mt_cpufreq_set_pmic_cmd(PMIC_WRAP_PHASE_NORMAL, IDX_NM_VGPU, pmic_val);
	mt_cpufreq_apply_pmic_cmd(IDX_NM_VGPU);
}
#endif

/* NOTE: This is ONLY for PTPOD SLT. Should not adjust VCORE in other cases. */
void dvfs_set_vcore(int pmic_val)
{
	cpufreq_dbg("%s(%d)\n", __func__, pmic_val);
	mt_cpufreq_set_pmic_cmd(PMIC_WRAP_PHASE_NORMAL, IDX_NM_VCORE_HPM, pmic_val);
	mt_cpufreq_apply_pmic_cmd(IDX_NM_VCORE_HPM);
}

void dvfs_set_vlte(int pmic_val)
{
	pmic_set_register_value(PMIC_VLTE_VOSEL_ON, pmic_val);
}

#if 0
void dvfs_set_vcore_pdn_volt(int pmic_val)
{
	cpufreq_dbg("%s(%d)\n", __func__, pmic_val);
	mt_cpufreq_set_pmic_phase(PMIC_WRAP_PHASE_DEEPIDLE);
	mt_cpufreq_set_pmic_cmd(PMIC_WRAP_PHASE_DEEPIDLE, IDX_DI_VCORE_PDN_NORMAL, pmic_val);
	mt_cpufreq_apply_pmic_cmd(IDX_DI_VCORE_PDN_NORMAL);
	mt_cpufreq_set_pmic_phase(PMIC_WRAP_PHASE_NORMAL);
}
#endif


static unsigned int vcpu_backup;
static unsigned int vcore_backup;
static unsigned int vlte_backup;

enum {
	PTP_CTRL_CPU = 0,
	PTP_CTRL_SOC = 1,
	PTP_CTRL_LTE = 2,
	PTP_CTRL_ALL = 3,
	NR_PTP_CTRL,
};

void dvfs_disable_by_ptpod(int id)
{
	struct mt_cpu_dvfs *p = id_to_cpu_dvfs(MT_CPU_DVFS_LITTLE);

	cpufreq_dbg("%s()\n", __func__);	/* <-XXX */

	switch (id) {
	case PTP_CTRL_CPU:
		vcpu_backup = dvfs_get_cpu_volt(MT_CPU_DVFS_LITTLE);
#ifdef __MTK_SLT_
		/* force Vboot = 1.25V for SLT */
		dvfs_set_cpu_volt(MT_CPU_DVFS_LITTLE, 125000);
#else
		mt_cpufreq_disable_by_ptpod(MT_CPU_DVFS_LITTLE);	/* VBOOT = 1.25 */
#endif
		break;

	case PTP_CTRL_SOC:
		vcore_backup = pmic_get_register_value(PMIC_VCORE1_VOSEL_ON);
		dvfs_set_vcore(VOLT_TO_PMIC_VAL(115000));	/* VBOOT = 1.15 */
		break;

	case PTP_CTRL_LTE:
		vlte_backup = pmic_get_register_value(PMIC_VLTE_VOSEL_ON);
		pmic_set_register_value(PMIC_VLTE_VOSEL_ON, VOLT_TO_PMIC_VAL(105000));	/* VBOOT = 1.05 */
		break;

	case PTP_CTRL_ALL:
		vcpu_backup = dvfs_get_cpu_volt(MT_CPU_DVFS_LITTLE);
		vcore_backup = pmic_get_register_value(PMIC_VCORE1_VOSEL_ON);
		vlte_backup = pmic_get_register_value(PMIC_VLTE_VOSEL_ON);
#ifdef __MTK_SLT_
		/* force Vboot = 1.25V for SLT */
		dvfs_set_cpu_volt(MT_CPU_DVFS_LITTLE, 125000);
#else
		mt_cpufreq_disable_by_ptpod(MT_CPU_DVFS_LITTLE);
#endif
		/* dvfs_set_vcore(VOLT_TO_PMIC_VAL(115000)); //VBOOT = 1.15 */
		/* pmic_set_register_value(PMIC_VLTE_VOSEL_ON, VOLT_TO_PMIC_VAL(105000));//VBOOT = 1.05 */
		break;

	default:
		break;
	}
}

void dvfs_enable_by_ptpod(int id)
{
#ifndef __MTK_SLT_
	struct mt_cpu_dvfs *p = id_to_cpu_dvfs(MT_CPU_DVFS_LITTLE);

	cpufreq_dbg("%s()\n", __func__);	/* <-XXX */

	switch (id) {
	case PTP_CTRL_CPU:
		dvfs_set_cpu_volt(MT_CPU_DVFS_LITTLE, vcpu_backup);
		break;

	case PTP_CTRL_SOC:
		dvfs_set_vcore(vcore_backup);
		break;

	case PTP_CTRL_LTE:
		pmic_set_register_value(PMIC_VLTE_VOSEL_ON, vlte_backup);
		break;

	case PTP_CTRL_ALL:
		dvfs_set_cpu_volt(MT_CPU_DVFS_LITTLE, vcpu_backup);
		dvfs_set_vcore(vcore_backup);
		pmic_set_register_value(PMIC_VLTE_VOSEL_ON, vlte_backup);
		break;

	default:
		break;
	}
#endif
}
#endif	/* ! __KERNEL__ */


#ifdef CONFIG_PROC_FS
/*
 * PROC
 */
static char *_copy_from_user_for_proc(const char __user *buffer, size_t count)
{
	char *buf = (char *)__get_free_page(GFP_USER);

	if (!buf)
		return NULL;

	if (count >= PAGE_SIZE)
		goto out;

	if (copy_from_user(buf, buffer, count))
		goto out;

	buf[count] = '\0';

	return buf;

out:
	free_page((unsigned long)buf);

	return NULL;
}

/* cpufreq_debug */
static int cpufreq_debug_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "cpufreq debug (log level) = %d\n", func_lv_mask);

	return 0;
}

static ssize_t cpufreq_debug_proc_write(struct file *file, const char __user *buffer, size_t count,
					loff_t *pos)
{
	unsigned int dbg_lv;

	char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	if (!kstrtouint(buf, 10, &dbg_lv))
		func_lv_mask = dbg_lv;
	else
		cpufreq_err("echo dbg_lv (dec) > /proc/cpufreq/cpufreq_debug\n");

	free_page((unsigned long)buf);
	return count;
}

/* cpufreq_fftt_test */
#include <linux/sched_clock.h>

static unsigned long long _delay_us;
static unsigned long long _delay_us_buf;

static int cpufreq_fftt_test_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%llu\n", _delay_us);

	if (_delay_us < _delay_us_buf)
		cpufreq_err("@%s(), %llu < %llu, loops_per_jiffy = %lu\n",
				__func__, _delay_us, _delay_us_buf, loops_per_jiffy);

	return 0;
}

static ssize_t cpufreq_fftt_test_proc_write(struct file *file, const char __user *buffer,
						size_t count, loff_t *pos)
{
	char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	if (!kstrtoull(buf, 10, &_delay_us_buf)) {
		unsigned long start;

		start = (unsigned long)sched_clock();
		udelay(_delay_us_buf);
		_delay_us = ((unsigned long)sched_clock() - start) / 1000;

		cpufreq_ver("@%s(%llu), _delay_us = %llu, loops_per_jiffy = %lu\n",
				__func__, _delay_us_buf, _delay_us, loops_per_jiffy);
	}

	free_page((unsigned long)buf);

	return count;
}

static int cpufreq_stress_test_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", do_dvfs_stress_test);

	return 0;
}

static ssize_t cpufreq_stress_test_proc_write(struct file *file, const char __user *buffer,
						  size_t count, loff_t *pos)
{
	unsigned int do_stress;

	char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	if (!kstrtouint(buf, 10, &do_stress))
		do_dvfs_stress_test = do_stress;
	else
		cpufreq_err("echo 0/1 > /proc/cpufreq/cpufreq_stress_test\n");

	free_page((unsigned long)buf);
	return count;
}

static int cpufreq_fix_freq_in_es_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", is_fix_freq_in_ES);

	return 0;
}

static ssize_t cpufreq_fix_freq_in_es_proc_write(struct file *file, const char __user *buffer,
						 size_t count, loff_t *pos)
{
	unsigned int fix_freq_in_ES;

	char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	if (!kstrtouint(buf, 10, &fix_freq_in_ES))
		is_fix_freq_in_ES = fix_freq_in_ES;
	else
		cpufreq_err("echo 0/1 > /proc/cpufreq/cpufreq_fix_freq_in_es\n");

	free_page((unsigned long)buf);
	return count;
}

/* cpufreq_limited_by_hevc */
static int cpufreq_limited_by_hevc_proc_show(struct seq_file *m, void *v)
{
	struct mt_cpu_dvfs *p = (struct mt_cpu_dvfs *)m->private;

	seq_printf(m, "%d\n", p->limited_freq_by_hevc);

	return 0;
}

static ssize_t cpufreq_limited_by_hevc_proc_write(struct file *file, const char __user *buffer,
						  size_t count, loff_t *pos)
{
	struct mt_cpu_dvfs *p = (struct mt_cpu_dvfs *)PDE_DATA(file_inode(file));
	int limited_freq_by_hevc;

	char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	if (!kstrtoint(buf, 10, &limited_freq_by_hevc)) {
		p->limited_freq_by_hevc = limited_freq_by_hevc;
		if (cpu_dvfs_is_available(p)
			&& (p->limited_freq_by_hevc > cpu_dvfs_get_cur_freq(p))) {
#ifdef CONFIG_CPU_FREQ
			struct cpufreq_policy *policy = cpufreq_cpu_get(p->cpu_id);

			if (policy) {
				cpufreq_driver_target(
					policy, p->limited_freq_by_hevc, CPUFREQ_RELATION_L);
				cpufreq_cpu_put(policy);
			}
#endif
		}
	} else
		cpufreq_err("echo limited_freq_by_hevc (dec) > /proc/cpufreq/cpufreq_limited_by_hevc\n");

	free_page((unsigned long)buf);
	return count;
}

/* cpufreq_limited_by_pbm */
#ifndef DISABLE_PBM_FEATURE
static int cpufreq_limited_by_pbm_proc_show(struct seq_file *m, void *v)
{
	struct mt_cpu_dvfs *p;
	int i;

	for_each_cpu_dvfs(i, p) {
		seq_printf(m, "[%s/%d]\n"
			   "limited_power_idx = %d\n"
			   "limited_max_freq = %d\n"
			   "limited_max_ncpu = %d\n"
			   "limited_power_by_thermal = %d\n"
			   "limited_power_by_pbm = %d\n",
			   p->name, i,
			   p->limited_power_idx,
			   p->limited_max_freq,
			   p->limited_max_ncpu,
			   p->limited_power_by_thermal, p->limited_power_by_pbm);
	}

	return 0;
}

static ssize_t cpufreq_limited_by_pbm_proc_write(struct file *file, const char __user *buffer,
						 size_t count, loff_t *pos)
{
	int limited_power;

	char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	if (!kstrtoint(buf, 10, &limited_power))
		mt_cpufreq_set_power_limit_by_pbm(limited_power);	/* TODO: specify limited_power by id??? */
	else
		cpufreq_err("echo limited_power (dec) > /proc/cpufreq/cpufreq_limited_by_pbm\n");

	free_page((unsigned long)buf);

	return count;
}
#endif

/* cpufreq_limited_by_thermal */
static int cpufreq_limited_by_thermal_proc_show(struct seq_file *m, void *v)
{
	struct mt_cpu_dvfs *p;
	int i;

	for_each_cpu_dvfs(i, p) {
		seq_printf(m, "[%s/%d]\n"
			   "limited_power_idx = %d\n"
			   "limited_max_freq = %d\n"
			   "limited_max_ncpu = %d\n"
			   "limited_power_by_thermal = %d\n",
			   p->name, i,
			   p->limited_power_idx,
			   p->limited_max_freq, p->limited_max_ncpu, p->limited_power_by_thermal);
	}

	return 0;
}

static ssize_t cpufreq_limited_by_thermal_proc_write(struct file *file, const char __user *buffer,
							 size_t count, loff_t *pos)
{
	int limited_power;

	char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	if (!kstrtoint(buf, 10, &limited_power))
		mt_cpufreq_thermal_protect(limited_power);	/* TODO: specify limited_power by id??? */
	else
		cpufreq_err("echo limited_power (dec) > /proc/cpufreq/cpufreq_limited_by_thermal\n");

	free_page((unsigned long)buf);
	return count;
}

/* PMIC 5A limit */
#ifdef CONFIG_ARCH_MT6753
static int cpufreq_5A_throttle_enable_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "cpufreq PMIC 5A throttle enable = %d\n", pmic_5A_throttle_enable);
	seq_printf(m, "cpufreq PMIC 5A throttle on/off = %d\n", pmic_5A_throttle_on);

	return 0;
}

static ssize_t cpufreq_5A_throttle_enable_proc_write(struct file *file, const char __user *buffer,
							 size_t count, loff_t *pos)
{
	unsigned int enable;

	char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	if (!kstrtouint(buf, 10, &enable)) {
		pmic_5A_throttle_enable = enable;
		_mt_cpufreq_set(MT_CPU_DVFS_LITTLE, -1);
	} else
		cpufreq_err("echo 1/0 > /proc/cpufreq/cpufreq_5A_throttle_enable\n");

	free_page((unsigned long)buf);
	return count;
}
#endif

/* cpufreq_limited_max_freq_by_user */
static int cpufreq_limited_max_freq_by_user_proc_show(struct seq_file *m, void *v)
{
	struct mt_cpu_dvfs *p = (struct mt_cpu_dvfs *)m->private;

	seq_printf(m, "%d\n", p->limited_max_freq_by_user);

	return 0;
}

static ssize_t cpufreq_limited_max_freq_by_user_proc_write(struct file *file,
							   const char __user *buffer, size_t count,
							   loff_t *pos)
{
	struct mt_cpu_dvfs *p = (struct mt_cpu_dvfs *)PDE_DATA(file_inode(file));
	int limited_max_freq;

	char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	if (!kstrtoint(buf, 10, &limited_max_freq)) {

		p->limited_max_freq_by_user = limited_max_freq;

		if (cpu_dvfs_is_available(p) && (p->limited_max_freq_by_user != 0)
			&& (p->limited_max_freq_by_user < cpu_dvfs_get_cur_freq(p))) {
			struct cpufreq_policy *policy = cpufreq_cpu_get(p->cpu_id);

			if (policy) {
				cpufreq_driver_target(
					policy, p->limited_max_freq_by_user, CPUFREQ_RELATION_H);
				cpufreq_cpu_put(policy);
			}
		}
	} else
		cpufreq_err("echo limited_max_freq (dec) > /proc/cpufreq/%s/cpufreq_limited_max_freq_by_user\n",
				p->name);

	free_page((unsigned long)buf);
	return count;
}

/* cpufreq_power_dump */
static int cpufreq_power_dump_proc_show(struct seq_file *m, void *v)
{
	struct mt_cpu_dvfs *p;
	int i, j;

	for_each_cpu_dvfs(i, p) {
		seq_printf(m, "[%s/%d]\n", p->name, i);

		for (j = 0; j < p->nr_power_tbl; j++) {
			seq_printf(m,
				   "[%d] = { .cpufreq_khz = %d,\t.cpufreq_ncpu = %d,\t.cpufreq_power = %d, },\n",
				   j, p->power_tbl[j].cpufreq_khz, p->power_tbl[j].cpufreq_ncpu,
				   p->power_tbl[j].cpufreq_power);
		}
	}

	return 0;
}

/* cpufreq_ptpod_freq_volt */
static int cpufreq_ptpod_freq_volt_proc_show(struct seq_file *m, void *v)
{
	struct mt_cpu_dvfs *p = (struct mt_cpu_dvfs *)m->private;
	int j;

	for (j = 0; j < p->nr_opp_tbl; j++) {
		seq_printf(m,
			   "[%d] = { .cpufreq_khz = %d,\t.cpufreq_volt = %d,\t.cpufreq_volt_org = %d, },\n",
			   j, p->opp_tbl[j].cpufreq_khz, p->opp_tbl[j].cpufreq_volt,
			   p->opp_tbl[j].cpufreq_volt_org);
	}

	return 0;
}

/* cpufreq_state */
static int cpufreq_state_proc_show(struct seq_file *m, void *v)
{
	struct mt_cpu_dvfs *p;
	int i;

	for_each_cpu_dvfs(i, p) {
		seq_printf(m, "[%s/%d]\n"
			   "dvfs_disable_by_procfs = %d\n"
			   "limited_freq_by_hevc = %d KHz\n"
			   "limited_power_by_thermal = %d mW\n"
			   "dvfs_disable_by_early_suspend = %d\n" "dvfs_disable_by_suspend = %d\n"
#ifdef CONFIG_CPU_DVFS_POWER_THROTTLING
			   "pwr_thro_mode = %d\n"
#endif
			   "limited_max_freq_by_user = %d KHz\n"
			   "dvfs_disable_by_ptpod = %d\n",
			   p->name, i,
			   p->dvfs_disable_by_procfs,
			   p->limited_freq_by_hevc,
			   p->limited_power_by_thermal,
			   p->dvfs_disable_by_early_suspend, p->dvfs_disable_by_suspend,
#ifdef CONFIG_CPU_DVFS_POWER_THROTTLING
			   p->pwr_thro_mode,
#endif
			   p->limited_max_freq_by_user, p->dvfs_disable_by_ptpod);
	}

	return 0;
}

static ssize_t cpufreq_state_proc_write(struct file *file, const char __user *buffer, size_t count,
					loff_t *pos)
{
	struct mt_cpu_dvfs *p = (struct mt_cpu_dvfs *)PDE_DATA(file_inode(file));
	char *buf = _copy_from_user_for_proc(buffer, count);
	int enable;

	if (!buf)
		return -EINVAL;

	if (!kstrtoint(buf, 10, &enable)) {
		if (enable == 0)
			p->dvfs_disable_by_procfs = true;
		else
			p->dvfs_disable_by_procfs = false;
	} else
		cpufreq_err("echo 1/0 > /proc/cpufreq/cpufreq_state\n");

	free_page((unsigned long)buf);
	return count;
}

/* cpufreq_oppidx */
static int cpufreq_oppidx_proc_show(struct seq_file *m, void *v)	/* <-XXX */
{
	struct mt_cpu_dvfs *p = (struct mt_cpu_dvfs *)m->private;
	int j;

	seq_printf(m, "[%s/%d]\n" "cpufreq_oppidx = %d\n", p->name, p->cpu_id, p->idx_opp_tbl);

	for (j = 0; j < p->nr_opp_tbl; j++) {
		seq_printf(m, "\tOP(%d, %d),\n",
			   cpu_dvfs_get_freq_by_idx(p, j), cpu_dvfs_get_volt_by_idx(p, j)
			);
	}

	return 0;
}

static ssize_t cpufreq_oppidx_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos)
{
	struct mt_cpu_dvfs *p = (struct mt_cpu_dvfs *)PDE_DATA(file_inode(file));
	int oppidx;

	char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	BUG_ON(NULL == p);

	if (!kstrtoint(buf, 10, &oppidx) && 0 <= oppidx && oppidx < p->nr_opp_tbl) {
		p->dvfs_disable_by_procfs = true;
		_mt_cpufreq_set(MT_CPU_DVFS_LITTLE, oppidx);
	} else {
		p->dvfs_disable_by_procfs = false;
		cpufreq_err("echo oppidx > /proc/cpufreq/cpufreq_oppidx (0 <= %d < %d)\n",
				oppidx, p->nr_opp_tbl);
	}

	free_page((unsigned long)buf);

	return count;
}

/* cpufreq_freq */
static int cpufreq_freq_proc_show(struct seq_file *m, void *v)	/* <-XXX */
{
	struct mt_cpu_dvfs *p = (struct mt_cpu_dvfs *)m->private;

	seq_printf(m, "%d KHz\n", p->ops->get_cur_phy_freq(p));

	return 0;
}

static ssize_t cpufreq_freq_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos)
{
	unsigned long flags;
	struct mt_cpu_dvfs *p = (struct mt_cpu_dvfs *)PDE_DATA(file_inode(file));
	unsigned int cur_freq;
	int freq, i, found;

	char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	BUG_ON(NULL == p);

	if (!kstrtoint(buf, 10, &freq)) {
		if (freq < cpu_dvfs_get_min_freq(p)) {
			if (freq != 0)
				cpufreq_err("frequency should higher than %dKHz!\n",
						cpu_dvfs_get_min_freq(p));
			p->dvfs_disable_by_procfs = false;
			goto end;
		} else if (freq > cpu_dvfs_get_max_freq(p)) {
			cpufreq_err("frequency should lower than %dKHz!\n",
					cpu_dvfs_get_max_freq(p));
			p->dvfs_disable_by_procfs = false;
			goto end;
		} else {
			for (i = 0; i < p->nr_opp_tbl; i++) {
				if (freq == p->opp_tbl[i].cpufreq_khz) {
					found = 1;
					break;
				}
			}

			if (found == 1) {
				p->dvfs_disable_by_procfs = true;
				cpufreq_lock(flags);
				cur_freq = p->ops->get_cur_phy_freq(p);
				if (freq != cur_freq) {
#ifdef CONFIG_ARCH_MT6753
					if (is_need_5A_throttle(p, freq, num_online_cpus() + num_online_cpus_delta)) {
						cpufreq_warn("@%s: frequency %dKHz over 5A limit!\n", __func__, freq);
						p->ops->set_cur_freq(p, cur_freq, PMIC_5A_THRO_MAX_CPU_FREQ);
					} else
						p->ops->set_cur_freq(p, cur_freq, freq);
#else
					p->ops->set_cur_freq(p, cur_freq, freq);
#endif
#ifndef DISABLE_PBM_FEATURE
					if (!p->dvfs_disable_by_suspend)
						_kick_PBM_by_cpu(p);
#endif
				}
				cpufreq_unlock(flags);
			} else {
				p->dvfs_disable_by_procfs = false;
				cpufreq_err("frequency %dKHz! is not found in CPU opp table\n",
						freq);
			}
		}
	} else {
		p->dvfs_disable_by_procfs = false;
		cpufreq_err("echo khz > /proc/cpufreq/cpufreq_freq\n");
	}

end:
	free_page((unsigned long)buf);

	return count;
}

/* cpufreq_volt */
static int cpufreq_volt_proc_show(struct seq_file *m, void *v)	/* <-XXX */
{
	struct mt_cpu_dvfs *p = (struct mt_cpu_dvfs *)m->private;

#ifdef CONFIG_CPU_DVFS_HAS_EXTBUCK
	if (cpu_dvfs_is_extbuck_valid()) {
		seq_printf(m, "Vproc: %d mv\n", p->ops->get_cur_volt(p) / 100);	/* mv */
		seq_printf(m, "Vsram: %d mv\n", _mt_cpufreq_get_cur_vsram(p) / 100);	/* mv */
	} else
		seq_printf(m, "%d mv\n", p->ops->get_cur_volt(p) / 100);	/* mv */
#else
	seq_printf(m, "%d mv\n", p->ops->get_cur_volt(p) / 100);	/* mv */
#endif

	return 0;
}

static ssize_t cpufreq_volt_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos)
{
	unsigned long flags;
	struct mt_cpu_dvfs *p = (struct mt_cpu_dvfs *)PDE_DATA(file_inode(file));
	int mv;

	char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	if (!kstrtoint(buf, 10, &mv)) {
		p->dvfs_disable_by_procfs = true;
		cpufreq_lock(flags);
		_mt_cpufreq_set_cur_volt_locked(p, mv * 100);
		cpufreq_unlock(flags);
	} else {
		p->dvfs_disable_by_procfs = false;
		cpufreq_err("echo mv > /proc/cpufreq/cpufreq_volt\n");
	}

	free_page((unsigned long)buf);

	return count;
}

#ifdef CONFIG_CPU_DVFS_TURBO_MODE
/* cpufreq_turbo_mode */
static int cpufreq_turbo_mode_proc_show(struct seq_file *m, void *v)	/* <-XXX */
{
	struct mt_cpu_dvfs *p = (struct mt_cpu_dvfs *)m->private;
	int i;

	seq_printf(m, "turbo_mode = %d\n", p->turbo_mode);

	for (i = 0; i < NR_TURBO_MODE; i++) {
		seq_printf(m, "[%d] = { .temp = %d, .freq_delta = %d, .volt_delta = %d }\n",
			   i,
			   turbo_mode_cfg[i].temp,
			   turbo_mode_cfg[i].freq_delta, turbo_mode_cfg[i].volt_delta);
	}

	return 0;
}

static ssize_t cpufreq_turbo_mode_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos)
	struct mt_cpu_dvfs *p = (struct mt_cpu_dvfs *)PDE_DATA(file_inode(file));
	unsigned int turbo_mode;
	int temp;
	int freq_delta;
	int volt_delta;

	char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	if ((sscanf(buf, "%d %d %d %d", &turbo_mode, &temp, &freq_delta, &volt_delta) == 4)
		&& turbo_mode < NR_TURBO_MODE) {
		turbo_mode_cfg[turbo_mode].temp = temp;
		turbo_mode_cfg[turbo_mode].freq_delta = freq_delta;
		turbo_mode_cfg[turbo_mode].volt_delta = volt_delta;
	} else if (!kstrtouint(buf, 10, &turbo_mode))
		p->turbo_mode = turbo_mode;
	else {
		cpufreq_err("echo 0/1 > /proc/cpufreq/cpufreq_turbo_mode\n");
		cpufreq_err("echo idx temp freq_delta volt_delta > /proc/cpufreq/cpufreq_turbo_mode\n");
	}

	free_page((unsigned long)buf);

	return count;
}
#endif

#define PROC_FOPS_RW(name)							\
static int name ## _proc_open(struct inode *inode, struct file *file)	\
{									\
	return single_open(file, name ## _proc_show, PDE_DATA(inode));	\
}									\
static const struct file_operations name ## _proc_fops = {		\
	.owner	= THIS_MODULE,				\
	.open	= name ## _proc_open,		\
	.read	= seq_read,					\
	.llseek	= seq_lseek,					\
	.release	= single_release,				\
	.write	= name ## _proc_write,		\
}

#define PROC_FOPS_RO(name)							\
static int name ## _proc_open(struct inode *inode, struct file *file)	\
{									\
	return single_open(file, name ## _proc_show, PDE_DATA(inode));	\
}									\
static const struct file_operations name ## _proc_fops = {		\
	.owner	= THIS_MODULE,				\
	.open	= name ## _proc_open,		\
	.read	= seq_read,					\
	.llseek	= seq_lseek,					\
	.release	= single_release,				\
}

#define PROC_ENTRY(name)	{__stringify(name), &name ## _proc_fops}

PROC_FOPS_RW(cpufreq_debug);
PROC_FOPS_RW(cpufreq_fftt_test);
PROC_FOPS_RW(cpufreq_stress_test);
PROC_FOPS_RW(cpufreq_fix_freq_in_es);
PROC_FOPS_RW(cpufreq_limited_by_hevc);
#ifndef DISABLE_PBM_FEATURE
PROC_FOPS_RW(cpufreq_limited_by_pbm);
#endif
PROC_FOPS_RW(cpufreq_limited_by_thermal);
#ifdef CONFIG_ARCH_MT6753
PROC_FOPS_RW(cpufreq_5A_throttle_enable);
#endif
PROC_FOPS_RW(cpufreq_limited_max_freq_by_user);
PROC_FOPS_RO(cpufreq_power_dump);
PROC_FOPS_RO(cpufreq_ptpod_freq_volt);
PROC_FOPS_RW(cpufreq_state);
PROC_FOPS_RW(cpufreq_oppidx);	/* <-XXX */
PROC_FOPS_RW(cpufreq_freq);	/* <-XXX */
PROC_FOPS_RW(cpufreq_volt);	/* <-XXX */
#ifdef CONFIG_CPU_DVFS_TURBO_MODE
PROC_FOPS_RW(cpufreq_turbo_mode);	/* <-XXX */
#endif

static int _mt_cpufreq_create_procfs(void)
{
	struct proc_dir_entry *dir = NULL;
	/* struct proc_dir_entry *cpu_dir = NULL; */
	struct mt_cpu_dvfs *p = id_to_cpu_dvfs(0);
	int i;	/* , j; */

	struct pentry {
		const char *name;
		const struct file_operations *fops;
	};

	const struct pentry entries[] = {
		PROC_ENTRY(cpufreq_debug),
		PROC_ENTRY(cpufreq_fftt_test),
		PROC_ENTRY(cpufreq_stress_test),
		PROC_ENTRY(cpufreq_fix_freq_in_es),
		PROC_ENTRY(cpufreq_limited_by_thermal),
#ifdef CONFIG_ARCH_MT6753
		PROC_ENTRY(cpufreq_5A_throttle_enable),
#endif
#ifndef DISABLE_PBM_FEATURE
		PROC_ENTRY(cpufreq_limited_by_pbm),
#endif
		PROC_ENTRY(cpufreq_power_dump),
	};

	const struct pentry cpu_entries[] = {
		PROC_ENTRY(cpufreq_limited_by_hevc),
		PROC_ENTRY(cpufreq_limited_max_freq_by_user),
		PROC_ENTRY(cpufreq_ptpod_freq_volt),
		PROC_ENTRY(cpufreq_state),
		PROC_ENTRY(cpufreq_oppidx),	/* <-XXX */
		PROC_ENTRY(cpufreq_freq),	/* <-XXX */
		PROC_ENTRY(cpufreq_volt),	/* <-XXX */
#ifdef CONFIG_CPU_DVFS_TURBO_MODE
		PROC_ENTRY(cpufreq_turbo_mode),	/* <-XXX */
#endif
	};

	dir = proc_mkdir("cpufreq", NULL);

	if (!dir) {
		cpufreq_err("fail to create /proc/cpufreq @ %s()\n", __func__);
		return -ENOMEM;
	}

	for (i = 0; i < ARRAY_SIZE(entries); i++) {
		if (!proc_create
			(entries[i].name, S_IRUGO | S_IWUSR | S_IWGRP, dir, entries[i].fops))
			cpufreq_err("%s(), create /proc/cpufreq/%s failed\n",
					__func__, entries[i].name);
	}

	for (i = 0; i < ARRAY_SIZE(cpu_entries); i++) {
		if (!proc_create_data
			(cpu_entries[i].name, S_IRUGO | S_IWUSR | S_IWGRP, dir, cpu_entries[i].fops, p))
			cpufreq_err("%s(), create /proc/cpufreq/%s failed\n",
					__func__, cpu_entries[i].name);
	}

	return 0;
}
#endif	/* CONFIG_PROC_FS */

/*
 * Module driver
 */
static int __init _mt_cpufreq_pdrv_init(void)
{
	int ret = 0;

	FUNC_ENTER(FUNC_LV_MODULE);

#ifdef CONFIG_PROC_FS
	/* init proc */
	if (_mt_cpufreq_create_procfs())
		goto out;
#endif	/* CONFIG_PROC_FS */

	/* register platform device/driver */
	ret = platform_device_register(&_mt_cpufreq_pdev);
	if (ret) {
		cpufreq_err("fail to register cpufreq device @ %s()\n", __func__);
		goto out;
	}

	ret = platform_driver_register(&_mt_cpufreq_pdrv);
	if (ret) {
		cpufreq_err("fail to register cpufreq driver @ %s()\n", __func__);
		platform_device_unregister(&_mt_cpufreq_pdev);
	}

	cpufreq_info("CPU DVFS pdrv init done.\n");
out:
	FUNC_EXIT(FUNC_LV_MODULE);

	return ret;
}

static void __exit _mt_cpufreq_pdrv_exit(void)
{
	FUNC_ENTER(FUNC_LV_MODULE);

	platform_driver_unregister(&_mt_cpufreq_pdrv);
	platform_device_unregister(&_mt_cpufreq_pdev);

	FUNC_EXIT(FUNC_LV_MODULE);
}
late_initcall(_mt_cpufreq_pdrv_init);
module_exit(_mt_cpufreq_pdrv_exit);

MODULE_DESCRIPTION("MediaTek CPU DVFS Driver v0.3");
MODULE_LICENSE("GPL");
