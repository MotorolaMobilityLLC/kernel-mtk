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

/*
 * @file    mt_cpufreq.c
 * @brief   Driver for CPU DVFS
 *
 */

#define __MT_CPUFREQ_C__

/*=============================================================*/
/* Include files                                               */
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
#include "aee.h"
#include <linux/seq_file.h>

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_address.h>
#endif

/* project includes */
/* #include "mach/mt_typedefs.h" */
#include <asm/io.h>
#include "mach/mt_thermal.h"
#include <mt-plat/mt_hotplug_strategy.h>
/* #include "mach/mt_pmic_wrap.h" */
#include "mach/mt_clkmgr.h"
#include "mach/mt_freqhopping.h"
#include "mt_ptp.h"
/* #include "mach/upmu_sw.h" */

#ifndef __KERNEL__
#include "freqhop_sw.h"
#include "mt_spm.h"
#include "clock_manager.h"
#include "pmic.h"
#else
#include "mt_spm.h"
#endif

/* local includes */
#include "mt_cpufreq.h"

/*=============================================================*/
/* Macro definition                                            */
/*=============================================================*/

/*
 * CONFIG
 */
#define CONFIG_CPU_DVFS_SHOWLOG 1

#define MAX(a, b) ((a) >= (b) ? (a) : (b))
#define MIN(a, b) ((a) >= (b) ? (b) : (a))

/* used @ set_cur_volt() */
#define MAX_VPROC_VOLT             115000	/* 1150mv * 100 */

/* PMIC/PLL settle time (us), should not be changed */
#define PMIC_CMD_DELAY_TIME     5
#define MIN_PMIC_SETTLE_TIME    25
#define PMIC_VOLT_UP_SETTLE_TIME(old_volt, new_volt)\
	(((((new_volt) - (old_volt)) + 1250 - 1) / 1250) + PMIC_CMD_DELAY_TIME)
#define PMIC_VOLT_DOWN_SETTLE_TIME(old_volt, new_volt)\
	(((((old_volt) - (new_volt)) * 2)  / 625) + PMIC_CMD_DELAY_TIME)
#define PLL_SETTLE_TIME         (20)

#define DEFAULT_VOLT_VCORE      (115000)

/* for DVFS OPP table */
#define CPU_DVFS_FREQ0   (1495000)	/* KHz */
#define CPU_DVFS_FREQ1   (1300000)	/* KHz */
#define CPU_DVFS_FREQ2   (1209000)	/* KHz */
#define CPU_DVFS_FREQ3   (1105000)	/* KHz */
#define CPU_DVFS_FREQ4   (1001000)	/* KHz */
#define CPU_DVFS_FREQ5   (903500) /* KHz */	/* 1.807/2 */
#define CPU_DVFS_FREQ6   (754000) /* KHz */	/* 1.508/2 */
#define CPU_DVFS_FREQ7   (604500) /* KHz */	/* 1.209/2 */

#define CPUFREQ_LAST_FREQ_LEVEL    (CPU_DVFS_FREQ7)

/*
 * LOG and Test
 */
#undef TAG
#define TAG     "[Power/cpufreq] "

#define cpufreq_err(fmt, args...)	pr_err(TAG"[ERROR]"fmt, ##args)
#define cpufreq_warn(fmt, args...)	pr_warn(TAG"[WARNING]"fmt, ##args)
#define cpufreq_info(fmt, args...)	pr_warn(TAG""fmt, ##args)
#define cpufreq_dbg(fmt, args...)	pr_debug(TAG""fmt, ##args)
#define cpufreq_ver(fmt, args...) \
	do { \
		if (func_lv_mask) \
			pr_debug(TAG""fmt, ##args); \
	} while (0)

#define FUNC_LV_MODULE         BIT(0)
#define FUNC_LV_CPUFREQ        BIT(1)
#define FUNC_LV_API                BIT(2)
#define FUNC_LV_LOCAL            BIT(3)
#define FUNC_LV_HELP              BIT(4)

static unsigned int func_lv_mask;
static unsigned int do_dvfs_stress_test;

#ifdef CONFIG_CPU_DVFS_SHOWLOG
#define FUNC_ENTER(lv)\
	do { \
		if ((lv) & func_lv_mask) \
			cpufreq_dbg(">> %s()\n", __func__); \
	} while (0)
#define FUNC_EXIT(lv)\
	do { \
		if ((lv) & func_lv_mask) \
			cpufreq_dbg("<< %s():%d\n", __func__, __LINE__); \
	} while (0)
#else
#define FUNC_ENTER(lv)
#define FUNC_EXIT(lv)
#endif

/*
 * BIT Operation
 */
#define _BIT_(_bit_)                    (unsigned)(1 << (_bit_))
#define _BITS_(_bits_, _val_)\
	((((unsigned) -1 >> (31 - ((1) ? _bits_))) & ~((1U << ((0) ? _bits_)) - 1)) & ((_val_)<<((0) ? _bits_)))
#define _BITMASK_(_bits_)               (((unsigned) -1 >> (31 - ((1) ? _bits_))) & ~((1U << ((0) ? _bits_)) - 1))
#define _GET_BITS_VAL_(_bits_, _val_)   (((_val_) & (_BITMASK_(_bits_))) >> ((0) ? _bits_))

/*
 * REG ACCESS
 */
#define cpufreq_read(addr)                  __raw_readl(addr)
#define cpufreq_write(addr, val)            mt_reg_sync_writel(val, addr)
#define cpufreq_write_mask(addr, mask, val) \
cpufreq_write(addr, (cpufreq_read(addr) & ~(_BITMASK_(mask))) | _BITS_(mask, val))

/*=============================================================*/
/* Local type definition                                       */
/*=============================================================*/


/*=============================================================*/
/* Local variable definition                                   */
/*=============================================================*/


/*=============================================================*/
/* Local function definition                                   */
/*=============================================================*/


/*=============================================================*/
/* Gobal function definition                                   */
/*=============================================================*/
static int _mt_cpufreq_fb_notifier_callback(struct notifier_block *self, unsigned long event, void *data);

static struct notifier_block _mt_cpufreq_fb_notifier = {
	.notifier_call = _mt_cpufreq_fb_notifier_callback,
};
/*
 * LOCK
 */
static DEFINE_MUTEX(cpufreq_mutex);
bool is_in_cpufreq = 0;
#define cpufreq_lock(flags) \
	do { \
		flags = (unsigned long)&flags; \
		mutex_lock(&cpufreq_mutex);	\
		is_in_cpufreq = 1; \
	} while (0)
#define cpufreq_unlock(flags) \
	do { \
		flags = (unsigned long)&flags; \
		is_in_cpufreq = 0; \
		mutex_unlock(&cpufreq_mutex); \
	} while (0)

/*
 * EFUSE
 */
#define CPUFREQ_EFUSE_INDEX     (3)
#define FUNC_CODE_EFUSE_INDEX	(28)

#define CPU_LEVEL_0             (0x0)
#define CPU_LEVEL_1             (0x1)
#define CPU_LEVEL_2             (0x2)
#define CPU_LEVEL_3             (0x3)

#define CPU_LV_TO_OPP_IDX(lv)   ((lv))	/* cpu_level to opp_idx */

#ifdef __KERNEL__
static unsigned int _mt_cpufreq_get_cpu_level(void)
{
	unsigned int lv = 0;
	/* get CPU clock-frequency from DT */
#ifdef CONFIG_OF
	lv = CPU_LEVEL_0;
#else
	/* no DT, we should check efuse for CPU speed HW bounding */
	{
unsigned int cpu_speed_bounding = _GET_BITS_VAL_(3:0, get_devinfo_with_index(CPUFREQ_EFUSE_INDEX));

		cpufreq_info("No DT, get CPU frequency bounding from efuse = %x\n",
			     cpu_speed_bounding);

		switch (cpu_speed_bounding) {
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
			lv = CPU_LEVEL_1;	/* 1.7G */
			break;

		case 5:
		case 6:
			lv = CPU_LEVEL_2;	/* 1.5G */
			break;

		case 7:
		case 8:
			lv = CPU_LEVEL_3;	/* 1.3G */
			break;

		default:
			cpufreq_err
			    ("No suitable DVFS table, set to default CPU level! efuse=0x%x\n",
			     cpu_speed_bounding);
			lv = CPU_LEVEL_1;
			break;
		}
	}
#endif

	return lv;
}
#else
static unsigned int _mt_cpufreq_get_cpu_level(void)
{
	return CPU_LEVEL_0;
}
#endif

/*
 * PMIC_WRAP
 */
#define VOLT_TO_PMIC_VAL(volt)  (((volt) - 70000 + 625 - 1) / 625)
#define PMIC_VAL_TO_VOLT(pmic)  (((pmic) * 625) + 70000)

#define PMIC_WRAP_DVFS_ADR0     (PWRAP_BASE_ADDR + 0x0E4)
#define PMIC_WRAP_DVFS_WDATA0   (PWRAP_BASE_ADDR + 0x0E8)
#define PMIC_WRAP_DVFS_ADR1     (PWRAP_BASE_ADDR + 0x0EC)
#define PMIC_WRAP_DVFS_WDATA1   (PWRAP_BASE_ADDR + 0x0F0)
#define PMIC_WRAP_DVFS_ADR2     (PWRAP_BASE_ADDR + 0x0F4)
#define PMIC_WRAP_DVFS_WDATA2   (PWRAP_BASE_ADDR + 0x0F8)
#define PMIC_WRAP_DVFS_ADR3     (PWRAP_BASE_ADDR + 0x0FC)
#define PMIC_WRAP_DVFS_WDATA3   (PWRAP_BASE_ADDR + 0x100)
#define PMIC_WRAP_DVFS_ADR4     (PWRAP_BASE_ADDR + 0x104)
#define PMIC_WRAP_DVFS_WDATA4   (PWRAP_BASE_ADDR + 0x108)
#define PMIC_WRAP_DVFS_ADR5     (PWRAP_BASE_ADDR + 0x10C)
#define PMIC_WRAP_DVFS_WDATA5   (PWRAP_BASE_ADDR + 0x110)
#define PMIC_WRAP_DVFS_ADR6     (PWRAP_BASE_ADDR + 0x114)
#define PMIC_WRAP_DVFS_WDATA6   (PWRAP_BASE_ADDR + 0x118)
#define PMIC_WRAP_DVFS_ADR7     (PWRAP_BASE_ADDR + 0x11C)
#define PMIC_WRAP_DVFS_WDATA7   (PWRAP_BASE_ADDR + 0x120)

/* PMIC ADDR */
#define PMIC_ADDR_VCORE_VOSEL_ON     0x220	/* [6:0]                    */
#define  PMIC_VCORE_VOSEL_CTRL_ADDR 0x216	/* 1.05V for dpidle */
#define NR_PMIC_WRAP_CMD 8	/* num of pmic wrap cmd (fixed value) */

struct pmic_wrap_cmd {
	unsigned int cmd_addr;
	unsigned int cmd_wdata;
};

struct pmic_wrap_setting {
	enum pmic_wrap_phase_id phase;
	struct pmic_wrap_cmd addr[NR_PMIC_WRAP_CMD];
	struct {
		struct {
			unsigned int cmd_addr;
			unsigned int cmd_wdata;
		} _[NR_PMIC_WRAP_CMD];
		const int nr_idx;
	} set[NR_PMIC_WRAP_PHASE];
};

static struct pmic_wrap_setting pw = {
	.phase = NR_PMIC_WRAP_PHASE,
	.addr = {{0, 0} },
	.set[PMIC_WRAP_PHASE_NORMAL] = {
		._[IDX_NM_VCORE] = {
			PMIC_ADDR_VCORE_VOSEL_ON,
			VOLT_TO_PMIC_VAL(DEFAULT_VOLT_VCORE),
		},
		.nr_idx = NR_IDX_NM,
	},
	.set[PMIC_WRAP_PHASE_DEEPIDLE] = {
		._[IDX_DI_VCORE_NORMAL] = {PMIC_VCORE_VOSEL_CTRL_ADDR, _BITS_(1:1, 1),},
		._[IDX_DI_VCORE_SLEEP] = {PMIC_VCORE_VOSEL_CTRL_ADDR, _BITS_(1:1, 0),},
		.nr_idx = NR_IDX_DI,
	},
};
#if 1
static DEFINE_SPINLOCK(pmic_wrap_lock);
#define pmic_wrap_lock(flags) spin_lock_irqsave(&pmic_wrap_lock, flags)
#define pmic_wrap_unlock(flags) spin_unlock_irqrestore(&pmic_wrap_lock, flags)
#else
static DEFINE_MUTEX(pmic_wrap_mutex);

#define pmic_wrap_lock(flags) \
	do { \
		flags = (unsigned long)&flags; \
		mutex_lock(&pmic_wrap_mutex); \
	} while (0)
#define pmic_wrap_unlock(flags) \
	do { \
		flags = (unsigned long)&flags; \
		mutex_unlock(&pmic_wrap_mutex); \
	} while (0)
#endif

static int _spm_dvfs_ctrl_volt(u32 value)
{
#define MAX_RETRY_COUNT (100)
	u32 ap_dvfs_con;
	int retry = 0;

	FUNC_ENTER(FUNC_LV_HELP);

	spm_write(SPM_POWERON_CONFIG_SET, (SPM_PROJECT_CODE << 16) | (1U << 0));

	ap_dvfs_con = spm_read(SPM_AP_DVFS_CON_SET);	/* DVFS voltage control 0x604 */
	spm_write(SPM_AP_DVFS_CON_SET, (ap_dvfs_con & ~(0x7)) | value);
	udelay(5);

	while ((spm_read(SPM_AP_DVFS_CON_SET) & (0x1 << 31)) == 0) {
		if (retry >= MAX_RETRY_COUNT) {
			cpufreq_err("FAIL: no response from PMIC wrapper\n");
			return -1;
		}

		retry++;
		/* cpufreq_dbg("wait for ACK signal from PMIC wrapper, retry = %d\n", retry); */

		udelay(5);
	}

	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

void _mt_cpufreq_pmic_table_init(void)
{
	struct pmic_wrap_cmd pwrap_cmd_default[NR_PMIC_WRAP_CMD] = {
		{PMIC_WRAP_DVFS_ADR0, PMIC_WRAP_DVFS_WDATA0,},
		{PMIC_WRAP_DVFS_ADR1, PMIC_WRAP_DVFS_WDATA1,},
		{PMIC_WRAP_DVFS_ADR2, PMIC_WRAP_DVFS_WDATA2,},
		{PMIC_WRAP_DVFS_ADR3, PMIC_WRAP_DVFS_WDATA3,},
		{PMIC_WRAP_DVFS_ADR4, PMIC_WRAP_DVFS_WDATA4,},
		{PMIC_WRAP_DVFS_ADR5, PMIC_WRAP_DVFS_WDATA5,},
		{PMIC_WRAP_DVFS_ADR6, PMIC_WRAP_DVFS_WDATA6,},
		{PMIC_WRAP_DVFS_ADR7, PMIC_WRAP_DVFS_WDATA7,},
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

void mt_cpufreq_apply_pmic_cmd(int idx)
{				/* kick spm */
	unsigned long flags;

	FUNC_ENTER(FUNC_LV_API);

	BUG_ON(idx >= pw.set[pw.phase].nr_idx);

	/* cpufreq_dbg("@%s: idx = %d\n", __func__, idx); */

	pmic_wrap_lock(flags);

	_spm_dvfs_ctrl_volt(idx);

	pmic_wrap_unlock(flags);

	FUNC_EXIT(FUNC_LV_API);
}
EXPORT_SYMBOL(mt_cpufreq_apply_pmic_cmd);

/* cpu voltage sampler */
static cpuVoltsampler_func g_pCpuVoltSampler;

void mt_cpufreq_setvolt_registerCB(cpuVoltsampler_func pCB)
{
	g_pCpuVoltSampler = pCB;
}
EXPORT_SYMBOL(mt_cpufreq_setvolt_registerCB);

/*
 * mt_cpufreq driver
 */
#define OP(khz, volt) {.cpufreq_khz = khz, .cpufreq_volt = volt, .cpufreq_volt_org = volt,}

#define for_each_cpu_dvfs(i, p)        for (i = 0, p = cpu_dvfs; i < NR_MT_CPU_DVFS; i++, p = &cpu_dvfs[i])
#define cpu_dvfs_is(p, id)                 (p == &cpu_dvfs[id])
#define cpu_dvfs_is_available(p)      (p->opp_tbl)
#define cpu_dvfs_get_name(p)         (p->name)

#define cpu_dvfs_get_cur_freq(p)                (p->opp_tbl[p->idx_opp_tbl].cpufreq_khz)
#define cpu_dvfs_get_freq_by_idx(p, idx)        (p->opp_tbl[idx].cpufreq_khz)
#define cpu_dvfs_get_max_freq(p)                (p->opp_tbl[0].cpufreq_khz)
#define cpu_dvfs_get_normal_max_freq(p)         (p->opp_tbl[p->idx_normal_max_opp].cpufreq_khz)
#define cpu_dvfs_get_min_freq(p)                (p->opp_tbl[p->nr_opp_tbl - 1].cpufreq_khz)

#define cpu_dvfs_get_cur_volt(p)                (p->opp_tbl[p->idx_opp_tbl].cpufreq_volt)
#define cpu_dvfs_get_volt_by_idx(p, idx)        (p->opp_tbl[idx].cpufreq_volt)

struct mt_cpu_freq_info {
	const unsigned int cpufreq_khz;
	unsigned int cpufreq_volt;	/* mv * 100 */
	const unsigned int cpufreq_volt_org;	/* mv * 100 */
};

struct mt_cpu_power_info {
	unsigned int cpufreq_khz;
	unsigned int cpufreq_ncpu;
	unsigned int cpufreq_power;
};

struct mt_cpu_dvfs {
	const char *name;
	unsigned int cpu_id;	/* for cpufreq */
	unsigned int cpu_level;
	struct mt_cpu_dvfs_ops *ops;

	/* opp (freq) table */
	struct mt_cpu_freq_info *opp_tbl;	/* OPP table */
	int nr_opp_tbl;		/* size for OPP table */
	int idx_opp_tbl;	/* current OPP idx */
	int idx_normal_max_opp;	/* idx for normal max OPP */
	int idx_opp_tbl_for_late_resume;	/* keep the setting for late resume */

	struct cpufreq_frequency_table *freq_tbl_for_cpufreq;	/* freq table for cpufreq */

	/* power table */
	struct mt_cpu_power_info *power_tbl;
	unsigned int nr_power_tbl;

	/* enable/disable DVFS function */
	int dvfs_disable_count;
	bool dvfs_disable_by_ptpod;
	bool dvfs_disable_by_suspend;
	bool dvfs_disable_by_early_suspend;
	bool dvfs_disable_by_procfs;

	/* limit for thermal */
	unsigned int limited_max_ncpu;
	unsigned int limited_max_freq;
	unsigned int idx_opp_tbl_for_thermal_thro;
	unsigned int thermal_protect_limited_power;

	/* limit for HEVC (via. sysfs) */
	unsigned int limited_freq_by_hevc;

	/* limit max freq from user */
	unsigned int limited_max_freq_by_user;

	int pre_online_cpu;
	unsigned int pre_freq;
};

struct mt_cpu_dvfs_ops {
	/* for thermal */
	void (*protect)(struct mt_cpu_dvfs *p, unsigned int limited_power);
	unsigned int (*get_temp)(struct mt_cpu_dvfs *p);
	int (*setup_power_table)(struct mt_cpu_dvfs *p);

	/* for freq change (PLL/MUX) */
	unsigned int (*get_cur_phy_freq)(struct mt_cpu_dvfs *p);
	void (*set_cur_freq)(struct mt_cpu_dvfs *p, unsigned int cur_khz, unsigned int target_khz);

	/* for volt change (PMICWRAP) */
	unsigned int (*get_cur_volt)(struct mt_cpu_dvfs *p);
	int (*set_cur_volt)(struct mt_cpu_dvfs *p, unsigned int volt);
};


/* for thermal */
static int setup_power_table(struct mt_cpu_dvfs *p);

/* for freq change (PLL/MUX) */
static unsigned int get_cur_phy_freq(struct mt_cpu_dvfs *p);
static void set_cur_freq(struct mt_cpu_dvfs *p, unsigned int cur_khz, unsigned int target_khz);

/* for volt change (PMICWRAP) */
static unsigned int get_cur_volt_pmic_wrap(struct mt_cpu_dvfs *p);
static int set_cur_volt_pmic_wrap(struct mt_cpu_dvfs *p, unsigned int volt);	/* volt: mv * 100 */

static unsigned int max_cpu_num = 8;

static struct mt_cpu_dvfs_ops dvfs_ops_pmic_wrap = {
	.setup_power_table = setup_power_table,

	.get_cur_phy_freq = get_cur_phy_freq,
	.set_cur_freq = set_cur_freq,

	.get_cur_volt = get_cur_volt_pmic_wrap,
	.set_cur_volt = set_cur_volt_pmic_wrap,
};

static struct mt_cpu_dvfs cpu_dvfs[] = {
	[MT_CPU_DVFS_LITTLE] = {
				.name = __stringify(MT_CPU_DVFS_LITTLE),
				.cpu_id = MT_CPU_DVFS_LITTLE,
				.cpu_level = CPU_LEVEL_1,	/* 1.7GHz */
				.ops = &dvfs_ops_pmic_wrap,

				.pre_online_cpu = 0,
				.pre_freq = 0,
				},
};


static struct mt_cpu_dvfs *id_to_cpu_dvfs(enum mt_cpu_dvfs_id id)
{
	return (id < NR_MT_CPU_DVFS) ? &cpu_dvfs[id] : NULL;
}


/* DVFS OPP table */
/* Notice: Each table MUST has 8 element to avoid ptpod error */

#define NR_MAX_OPP_TBL  8
#define NR_MAX_CPU      8

/* CPU LEVEL 0, 1.3GHz segment */
static struct mt_cpu_freq_info opp_tbl_e1_0[] = {
	OP(CPU_DVFS_FREQ1, 130625),
	OP(CPU_DVFS_FREQ2, 122500),
	OP(CPU_DVFS_FREQ3, 119375),
	OP(CPU_DVFS_FREQ4, 115000),
	OP(CPU_DVFS_FREQ5, 115000),
	OP(CPU_DVFS_FREQ6, 115000),
	OP(CPU_DVFS_FREQ7, 115000),
	OP(CPU_DVFS_FREQ7, 115000),
};

/* CPU LEVEL 1, 1.5GHz segment */
static struct mt_cpu_freq_info opp_tbl_e1_1[] = {
	OP(CPU_DVFS_FREQ0, 131000),
	OP(CPU_DVFS_FREQ1, 122000),
	OP(CPU_DVFS_FREQ2, 119000),
	OP(CPU_DVFS_FREQ3, 115000),
	OP(CPU_DVFS_FREQ4, 112000),
	OP(CPU_DVFS_FREQ5, 105000),
	OP(CPU_DVFS_FREQ6, 105000),
	OP(CPU_DVFS_FREQ7, 105000),
};

struct opp_tbl_info {
	struct mt_cpu_freq_info *const opp_tbl;
	const int size;
};

static struct opp_tbl_info opp_tbls[] = {
	[CPU_LV_TO_OPP_IDX(CPU_LEVEL_0)] = {(opp_tbl_e1_0), ARRAY_SIZE(opp_tbl_e1_0),},
	[CPU_LV_TO_OPP_IDX(CPU_LEVEL_1)] = {(opp_tbl_e1_1), ARRAY_SIZE(opp_tbl_e1_1),},
};

/* for freq change (PLL/MUX) */
#define PLL_FREQ_STEP		(13000)	/* KHz */

#define PLL_DIV1_1807_FREQ		(1807000)	/* for 900MHz */
#define PLL_DIV1_1508_FREQ		(1508000)	/* for 750MHz */
#define PLL_DIV1_1495_FREQ		(1495000)	/* for 1.5G */
#define PLL_DIV1_1209_FREQ		(1209000)	/* for 1.2G & 600MHz */
#define PLL_DIV1_1001_FREQ		(1001000)	/* for 1G - low */
#define PLL_DIV1_1000_FREQ		(1000000)	/* for 1G - low */
#define PLL_DIV2_FREQ			(520000)	/* KHz */

#define DDS_DIV1_1807_FREQ		(0x00116000)	/* 1807MHz */
#define DDS_DIV1_1508_FREQ		(0x000E8000)	/* 1508MHz */
#define DDS_DIV1_1495_FREQ		(0x000E6000)	/* 1495MHz */
#define DDS_DIV1_1209_FREQ		(0x000BA000)	/* 1209MHz */
#define DDS_DIV1_1001_FREQ		(0x0009A000)	/* 1001MHz */
#define DDS_DIV1_FREQ			(0x0009A000)	/* 1001MHz */
#define DDS_DIV2_FREQ			(0x010A0000)	/* 520MHz  */

#define DDS_DIV1_1000_FREQ		(0x00099D8A)	/* 1000MHz */

/* for turbo mode */
#define TURBO_MODE_BOUNDARY_CPU_NUM	2

/* idx sort by temp from low to high */
enum turbo_mode {
	TURBO_MODE_2,
	TURBO_MODE_1,
	TURBO_MODE_NONE,

	NR_TURBO_MODE,
};

/* idx sort by temp from low to high */
struct turbo_mode_cfg {
	int temp;		/* degree x 1000 */
	int freq_delta;		/* percentage    */
	int volt_delta;		/* mv * 100       */
} turbo_mode_cfg[] = {
	[TURBO_MODE_2] = {
	.temp = 65000, .freq_delta = 10, .volt_delta = 4000,}, [TURBO_MODE_1] = {
	.temp = 85000, .freq_delta = 5, .volt_delta = 2000,}, [TURBO_MODE_NONE] = {
.temp = 125000, .freq_delta = 0, .volt_delta = 0,},};

#define TURBO_MODE_FREQ(mode, freq) \
(((freq * (100 + turbo_mode_cfg[mode].freq_delta)) / PLL_FREQ_STEP) / 100 * PLL_FREQ_STEP)
#define TURBO_MODE_VOLT(mode, volt) (volt + turbo_mode_cfg[mode].volt_delta)

static enum turbo_mode get_turbo_mode(struct mt_cpu_dvfs *p, unsigned int target_khz)
{
	return TURBO_MODE_NONE;
}

/* for PTP-OD */
static int _set_cur_volt_locked(struct mt_cpu_dvfs *p, unsigned int volt)
{				/* volt: mv * 100 */
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

static int _restore_default_volt(struct mt_cpu_dvfs *p)
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
	ret = _set_cur_volt_locked(p,
				   TURBO_MODE_VOLT(get_turbo_mode(p, cpu_dvfs_get_cur_freq(p)),
						   cpu_dvfs_get_cur_volt(p)
				   )
	    );

	cpufreq_unlock(flags);

	FUNC_EXIT(FUNC_LV_HELP);

	return ret;
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
	ret = _set_cur_volt_locked(p,
				   TURBO_MODE_VOLT(get_turbo_mode(p, cpu_dvfs_get_cur_freq(p)),
						   cpu_dvfs_get_cur_volt(p)
				   )
	    );

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

	_restore_default_volt(p);

	FUNC_EXIT(FUNC_LV_API);
}
EXPORT_SYMBOL(mt_cpufreq_restore_default_volt);

static unsigned int _cpu_freq_calc(unsigned int con1, unsigned int ckdiv1)
{
	unsigned int freq = 0;

	FUNC_ENTER(FUNC_LV_HELP);

con1 &= _BITMASK_(26:0);

	if (con1 >= DDS_DIV2_FREQ) {
		freq = DDS_DIV2_FREQ;
		freq = PLL_DIV2_FREQ + (((con1 - freq) / 0x2000) * PLL_FREQ_STEP / 2);
	} else if (con1 >= DDS_DIV1_FREQ) {
		freq = DDS_DIV1_FREQ;
		freq = PLL_DIV1_1001_FREQ + (((con1 - freq) / 0x2000) * PLL_FREQ_STEP);
	} else
		BUG();

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

	return freq;
}

/* #define TOP_CKMUXSEL    (INFRA_SYS_CFG_AO_BASE) */
/* #define TOP_CKMUXSEL   (0xF0001000) */
/* #define TOP_CKDIV1      (TOP_CKMUXSEL + 0x8) */
static unsigned int get_cur_phy_freq(struct mt_cpu_dvfs *p)
{
	unsigned int con1;
	unsigned int ckdiv1 = 0;
	unsigned int cur_khz;

	FUNC_ENTER(FUNC_LV_LOCAL);

	/* BUG_ON(NULL == p); */
	con1 = cpufreq_read(ARMPLL_CON1);
	ckdiv1 = cpufreq_read(TOP_CKDIV1);
	ckdiv1 = _GET_BITS_VAL_(4:0, ckdiv1);
	cur_khz = _cpu_freq_calc(con1, ckdiv1);

	cpufreq_ver("@%s: cur_khz = %d, con1 = 0x%x, ckdiv1_val = 0x%x\n", __func__, cur_khz, con1,
		    ckdiv1);

	FUNC_EXIT(FUNC_LV_LOCAL);

	return cur_khz;
}

static unsigned int _mt_cpufreq_get_cur_phy_freq(enum mt_cpu_dvfs_id id)
{
	struct mt_cpu_dvfs *p = id_to_cpu_dvfs(id);

	FUNC_ENTER(FUNC_LV_LOCAL);

	BUG_ON(NULL == p);

	FUNC_EXIT(FUNC_LV_LOCAL);

	return p->ops->get_cur_phy_freq(p);
}

static unsigned int _cpu_dds_calc(unsigned int khz)
{
	unsigned int dds;

	FUNC_ENTER(FUNC_LV_HELP);

	if (khz >= PLL_DIV1_1001_FREQ)
		dds = DDS_DIV1_1001_FREQ + ((khz - PLL_DIV1_1001_FREQ) / PLL_FREQ_STEP) * 0x2000;
	else if (khz >= PLL_DIV2_FREQ)
		dds = DDS_DIV2_FREQ + ((khz - PLL_DIV2_FREQ) * 2 / PLL_FREQ_STEP) * 0x2000;
	else
		BUG();


	FUNC_EXIT(FUNC_LV_HELP);

	return dds;
}

static void _cpu_clock_switch(struct mt_cpu_dvfs *p, enum top_ckmuxsel sel)
{
	FUNC_ENTER(FUNC_LV_HELP);

	switch (sel) {
	case TOP_CKMUXSEL_CLKSQ:
	case TOP_CKMUXSEL_ARMPLL:
	case TOP_CKMUXSEL_UNIVPLL:
	case TOP_CKMUXSEL_MAINPLL:
cpufreq_write_mask(TOP_CKMUXSEL, 3:2, sel);
		break;

	default:
		BUG();
		break;
	}

	FUNC_EXIT(FUNC_LV_HELP);
}

static enum top_ckmuxsel _get_cpu_clock_switch(struct mt_cpu_dvfs *p)
{
	unsigned int val = cpufreq_read(TOP_CKMUXSEL);
unsigned int mask = _BITMASK_(3:2);

	FUNC_ENTER(FUNC_LV_HELP);

	val &= mask;		/* _BITMASK_(1 : 0) */
	val >>= 2;
	FUNC_EXIT(FUNC_LV_HELP);

	return val;
}

int mt_cpufreq_clock_switch(enum mt_cpu_dvfs_id id, enum top_ckmuxsel sel)
{
	struct mt_cpu_dvfs *p = id_to_cpu_dvfs(id);

	if (!p)
		return -1;

	_cpu_clock_switch(p, sel);

	return 0;
}

enum top_ckmuxsel mt_cpufreq_get_clock_switch(enum mt_cpu_dvfs_id id)
{
	struct mt_cpu_dvfs *p = id_to_cpu_dvfs(id);

	if (!p)
		return -1;

	return _get_cpu_clock_switch(p);
}

/*
 * CPU freq scaling
 *
 * above 1209MHz: use freq hopping
 * below 1209MHz: set CLKDIV1
 * if cross 1209MHz, migrate to 1209MHz first.
 *
 */
#define CPUFREQ_BOUNDARY_FOR_FHCTL   (CPU_DVFS_FREQ4)

static void set_cur_freq(struct mt_cpu_dvfs *p, unsigned int cur_khz, unsigned int target_khz)
{
	unsigned int dds;
	unsigned int is_fhctl_used;
unsigned int ckdiv1_val = _GET_BITS_VAL_(4:0, cpufreq_read(TOP_CKDIV1));
unsigned int ckdiv1_mask = _BITMASK_(4:0);
	unsigned int sel = 0;
	unsigned int cur_volt = 0;
	unsigned int mainpll_volt_idx = 0;

#define IS_CLKDIV_USED(clkdiv)  (((clkdiv < 8) || ((clkdiv % 8) == 0)) ? 0 : 1)

	FUNC_ENTER(FUNC_LV_LOCAL);

	if (cur_khz == target_khz)
		return;

	/* cpufreq_ver("cur_khz = %d, ckdiv1_val = 0x%x\n", cur_khz, ckdiv1_val); */

	if (((cur_khz < CPUFREQ_BOUNDARY_FOR_FHCTL) && (target_khz > CPUFREQ_BOUNDARY_FOR_FHCTL))
	    || ((target_khz < CPUFREQ_BOUNDARY_FOR_FHCTL)
		&& (cur_khz > CPUFREQ_BOUNDARY_FOR_FHCTL))) {
		set_cur_freq(p, cur_khz, CPUFREQ_BOUNDARY_FOR_FHCTL);
		cur_khz = CPUFREQ_BOUNDARY_FOR_FHCTL;
	}

	is_fhctl_used = ((target_khz >= CPUFREQ_BOUNDARY_FOR_FHCTL)
			 && (cur_khz >= CPUFREQ_BOUNDARY_FOR_FHCTL)) ? 1 : 0;

	cpufreq_ver("@%s():%d, cur_khz = %d, target_khz = %d, is_fhctl_used = %d\n",
		    __func__, __LINE__, cur_khz, target_khz, is_fhctl_used);

	if (!is_fhctl_used) {
		/* set ca7_clkdiv1_sel */
		switch (target_khz) {
		case CPU_DVFS_FREQ4:
			dds = _cpu_dds_calc(CPU_DVFS_FREQ4);	/* 1001 */
			sel = 8;	/* 4/4 */
			break;

		case CPU_DVFS_FREQ5:
			dds = _cpu_dds_calc(CPU_DVFS_FREQ5);	/* 903= 1807 / 2 */
			sel = 8;	/* 4/4 */
			break;

		case CPU_DVFS_FREQ6:
			dds = _cpu_dds_calc(CPU_DVFS_FREQ6);	/* 754= 1508 / 2 */
			sel = 8;	/* 4/4 */
			break;

		case CPU_DVFS_FREQ7:
			dds = _cpu_dds_calc(1209000);	/* 604 = 1209 / 2 */
			sel = 10;	/* 2/4 */
			break;

		default:
			BUG();
		}

		cur_volt = p->ops->get_cur_volt(p);

		switch (p->cpu_level) {
		case CPU_LEVEL_0:
		case CPU_LEVEL_1:
			mainpll_volt_idx = 3;
			break;

		default:
			mainpll_volt_idx = 3;
			break;
		}

		if (cur_volt < cpu_dvfs_get_volt_by_idx(p, mainpll_volt_idx))
			p->ops->set_cur_volt(p, cpu_dvfs_get_volt_by_idx(p, mainpll_volt_idx));
		else
			cur_volt = 0;

		enable_clock(MT_CG_PLL2_CK, "CPU_DVFS");
		/* set ARMPLL and CLKDIV */
		_cpu_clock_switch(p, TOP_CKMUXSEL_MAINPLL);
		cpufreq_write(ARMPLL_CON1, dds | _BIT_(31));	/* 0x10018000       APMIXEDSYS + 0x0104 */

		udelay(PLL_SETTLE_TIME);
		cpufreq_write(TOP_CKDIV1, (ckdiv1_val & ~ckdiv1_mask) | sel);
		_cpu_clock_switch(p, TOP_CKMUXSEL_ARMPLL);
		disable_clock(MT_CG_PLL2_CK, "CPU_DVFS");

		/* restore Vproc */
		if (cur_volt)
			p->ops->set_cur_volt(p, cur_volt);
	} else {
		dds = _cpu_dds_calc(target_khz);
BUG_ON(dds & _BITMASK_(26:24));/* should not use posdiv */

#if !defined(__KERNEL__) && defined(MTKDRV_FREQHOP)
		fhdrv_dvt_dvfs_enable(ARMCA7PLL_ID, dds);
#else				/* __KERNEL__ */
#ifndef CPUDVFS_WORKAROUND_FOR_GIT
		mt_dfs_armpll(FH_ARMCA7_PLLID, dds);
#endif
#endif				/* ! __KERNEL__ */
	}

	FUNC_EXIT(FUNC_LV_LOCAL);
}

/* for volt change (PMICWRAP) */

static unsigned int get_cur_volt_pmic_wrap(struct mt_cpu_dvfs *p)
{
	unsigned int rdata = 0;
	unsigned int retry_cnt = 5;

	FUNC_ENTER(FUNC_LV_LOCAL);

	do {
		pwrap_read(PMIC_ADDR_VCORE_VOSEL_ON, &rdata);
	} while (rdata == PMIC_VAL_TO_VOLT(0) && retry_cnt--);

	rdata = PMIC_VAL_TO_VOLT(rdata);
	/* cpufreq_ver("@%s: volt = %d\n", __func__, rdata); */

	FUNC_EXIT(FUNC_LV_LOCAL);

	return rdata;		/* vproc: mv*100 */
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

#define PMIC_SETTLE_TIME (40)	/* us */
static int set_cur_volt_pmic_wrap(struct mt_cpu_dvfs *p, unsigned int volt)
{				/* volt: vproc (mv*100) */
	unsigned int cur_volt = get_cur_volt_pmic_wrap(p);


	FUNC_ENTER(FUNC_LV_LOCAL);

	mt_cpufreq_set_pmic_cmd(PMIC_WRAP_PHASE_NORMAL, IDX_NM_VCORE, VOLT_TO_PMIC_VAL(volt));
	mt_cpufreq_apply_pmic_cmd(IDX_NM_VCORE);

	if (volt > cur_volt)
		udelay(PMIC_VOLT_UP_SETTLE_TIME(cur_volt, volt));

	if (NULL != g_pCpuVoltSampler)
		g_pCpuVoltSampler(MT_CPU_DVFS_LITTLE, volt / 100);	/* mv */

	FUNC_EXIT(FUNC_LV_LOCAL);

	return 0;
}

/* cpufreq set (freq & volt) */

static unsigned int _search_available_volt(struct mt_cpu_dvfs *p, unsigned int target_khz)
{
	int i;

	FUNC_ENTER(FUNC_LV_HELP);

	BUG_ON(NULL == p);

	/* search available voltage */
	for (i = p->nr_opp_tbl - 1; i >= 0; i--) {
		if (target_khz <= cpu_dvfs_get_freq_by_idx(p, i))
			break;
	}

	BUG_ON(i < 0);		/* i.e. target_khz > p->opp_tbl[0].cpufreq_khz */

	FUNC_EXIT(FUNC_LV_HELP);

	return cpu_dvfs_get_volt_by_idx(p, i);	/* mv * 100 */
}

static int _cpufreq_set_locked(struct mt_cpu_dvfs *p, unsigned int cur_khz, unsigned int target_khz,
			       struct cpufreq_policy *policy)
{
	unsigned int volt;	/* mv * 100 */
	int ret = 0;
#ifdef CONFIG_CPU_FREQ
	struct cpufreq_freqs freqs;
#endif

	enum turbo_mode mode = get_turbo_mode(p, target_khz);

	FUNC_ENTER(FUNC_LV_HELP);

	volt = _search_available_volt(p, target_khz);

	if (cur_khz != TURBO_MODE_FREQ(mode, target_khz))
		cpufreq_ver
		    ("@%s(), target_khz = %d (%d), volt = %d (%d), num_online_cpus = %d, cur_khz = %d\n",
		     __func__, target_khz, TURBO_MODE_FREQ(mode, target_khz), volt,
		     TURBO_MODE_VOLT(mode, volt), num_online_cpus(), cur_khz);


	if (cur_khz == target_khz)
		goto out;

	/* set volt (UP) */
	if (cur_khz < target_khz) {
		ret = p->ops->set_cur_volt(p, volt);

		if (ret)
			goto out;
	}
#ifdef CONFIG_CPU_FREQ
	freqs.old = cur_khz;
	freqs.new = target_khz;
	if (policy)
		freqs.cpu = policy->cpu;

	if (policy)
		cpufreq_freq_transition_begin(policy, &freqs);

#endif

	/* set freq (UP/DOWN) */
	if (cur_khz != target_khz)
		p->ops->set_cur_freq(p, cur_khz, target_khz);

#ifdef CONFIG_CPU_FREQ
	if (policy)
		cpufreq_freq_transition_end(policy, &freqs, 0);
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

	/* trigger exception if freq/volt not correct during stress */
	if (do_dvfs_stress_test) {
		BUG_ON(p->ops->get_cur_volt(p) < volt);
		BUG_ON(p->ops->get_cur_phy_freq(p) < target_khz);
		/* BUG_ON(p->ops->get_cur_phy_freq(p) != target_khz); */
	}

	FUNC_EXIT(FUNC_LV_HELP);
out:
	return ret;
}

static unsigned int _calc_new_opp_idx(struct mt_cpu_dvfs *p, int new_opp_idx);

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

	cpufreq_lock(flags);

	if (new_opp_idx == -1)
		new_opp_idx = p->idx_opp_tbl;

	if (do_dvfs_stress_test)
		new_opp_idx = jiffies & 0x7;	/* 0~7 */
	else {
#if defined(CONFIG_CPU_DVFS_BRINGUP)
		new_opp_idx = id_to_cpu_dvfs(id)->idx_normal_max_opp;
#else
		new_opp_idx = _calc_new_opp_idx(id_to_cpu_dvfs(id), new_opp_idx);
#endif
	}

	cur_freq = p->ops->get_cur_phy_freq(p);
	target_freq = cpu_dvfs_get_freq_by_idx(p, new_opp_idx);
#ifdef CONFIG_CPU_FREQ
	_cpufreq_set_locked(p, cur_freq, target_freq, policy);
#else
	_cpufreq_set_locked(p, cur_freq, target_freq, NULL);
#endif
	p->idx_opp_tbl = new_opp_idx;

	cpufreq_unlock(flags);

#ifdef CONFIG_CPU_FREQ

	if (policy)
		cpufreq_cpu_put(policy);

#endif

	FUNC_EXIT(FUNC_LV_LOCAL);
}

static void _set_no_limited(struct mt_cpu_dvfs *p)
{
	FUNC_ENTER(FUNC_LV_HELP);

	BUG_ON(NULL == p);

	p->limited_max_freq = cpu_dvfs_get_max_freq(p);
	p->limited_max_ncpu = max_cpu_num;
	p->thermal_protect_limited_power = 0;

	FUNC_EXIT(FUNC_LV_HELP);
}

static int _sync_opp_tbl_idx(struct mt_cpu_dvfs *p)
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
		ret = 0;
	} else
		cpufreq_warn("%s can't find freq = %d\n", cpu_dvfs_get_name(p), freq);

	FUNC_EXIT(FUNC_LV_HELP);

	return ret;
}

static void _mt_cpufreq_sync_opp_tbl_idx(void)
{
	struct mt_cpu_dvfs *p;
	int i;

	FUNC_ENTER(FUNC_LV_LOCAL);

	for_each_cpu_dvfs(i, p) {
		if (cpu_dvfs_is_available(p))
			_sync_opp_tbl_idx(p);
	}

	FUNC_EXIT(FUNC_LV_LOCAL);
}

static enum mt_cpu_dvfs_id _get_cpu_dvfs_id(unsigned int cpu_id)
{
	return MT_CPU_DVFS_LITTLE;
}

/* Power Table */
static void _power_calculation(struct mt_cpu_dvfs *p, int oppidx, int ncpu)
{
#define CA7_REF_POWER	670	/* mW  */
#define CA7_REF_FREQ	1300000	/* KHz */
#define CA7_REF_VOLT	115000	/* mV  */
	int p_dynamic = 0, p_leakage = 0, ref_freq, ref_volt;
	int possible_cpu = max_cpu_num;

	FUNC_ENTER(FUNC_LV_HELP);

	p_dynamic = CA7_REF_POWER;
	ref_freq = CA7_REF_FREQ;
	ref_volt = CA7_REF_VOLT;

	p_leakage = 65;		/* MCL50_65C, 4core */

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

static int setup_power_table(struct mt_cpu_dvfs *p)
{
	static const unsigned int pwr_tbl_cgf[NR_MAX_CPU] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	unsigned int pwr_eff_tbl[NR_MAX_OPP_TBL][NR_MAX_CPU];
	unsigned int pwr_eff_num;
	int possible_cpu = max_cpu_num;
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
				_power_calculation(p, i, j);
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

		table[num].driver_data = i;
		table[num].frequency = CPUFREQ_TABLE_END;

		p->opp_tbl = freqs;
		p->nr_opp_tbl = num;
		p->freq_tbl_for_cpufreq = table;
	}
#ifdef CONFIG_CPU_FREQ
	ret = cpufreq_frequency_table_cpuinfo(policy, p->freq_tbl_for_cpufreq);

#if 0
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
	_mt_cpufreq_set(id, p->idx_normal_max_opp);

	FUNC_EXIT(FUNC_LV_API);

	return cpu_dvfs_get_cur_freq(p);
}
EXPORT_SYMBOL(mt_cpufreq_disable_by_ptpod);

void mt_cpufreq_thermal_protect(unsigned int limited_power)
{
	FUNC_ENTER(FUNC_LV_API);

	cpufreq_ver("%s(): limited_power = %d\n", __func__, limited_power);

#ifdef CONFIG_CPU_FREQ
	{
		struct cpufreq_policy *policy;
		struct mt_cpu_dvfs *p;
		int possible_cpu;
		int ncpu;
		int found = 0;
		unsigned long flag;
		int i;

		policy = cpufreq_cpu_get(0);

		if (NULL == policy)
			goto no_policy;

		p = id_to_cpu_dvfs(_get_cpu_dvfs_id(policy->cpu));

		BUG_ON(NULL == p);

		cpufreq_lock(flag);	/* <- lock */

		/* save current oppidx */
		if (!p->thermal_protect_limited_power)
			p->idx_opp_tbl_for_thermal_thro = p->idx_opp_tbl;

		p->thermal_protect_limited_power = limited_power;
		possible_cpu = max_cpu_num;

		/* no limited */
		if (0 == limited_power) {
			p->limited_max_ncpu = possible_cpu;
			p->limited_max_freq = cpu_dvfs_get_max_freq(p);
			/* restore oppidx */
			p->idx_opp_tbl = p->idx_opp_tbl_for_thermal_thro;
		} else {
			for (ncpu = possible_cpu; ncpu > 0; ncpu--) {
				for (i = 0; i < p->nr_opp_tbl * possible_cpu; i++) {
					if (p->power_tbl[i].cpufreq_power <= limited_power) {
						p->limited_max_ncpu = p->power_tbl[i].cpufreq_ncpu;
						p->limited_max_freq = p->power_tbl[i].cpufreq_khz;
						found = 1;
						ncpu = 0;	/* for break outer loop */
						break;
					}
				}
			}

			/* not found and use lowest power limit */
			if (!found) {
				p->limited_max_ncpu =
				    p->power_tbl[p->nr_power_tbl - 1].cpufreq_ncpu;
				p->limited_max_freq = p->power_tbl[p->nr_power_tbl - 1].cpufreq_khz;
			}
		}

		cpufreq_ver("found = %d, limited_max_freq = %d, limited_max_ncpu = %d\n", found,
			     p->limited_max_freq, p->limited_max_ncpu);

		cpufreq_unlock(flag);	/* <- unlock */
#ifndef CPUDVFS_WORKAROUND_FOR_GIT
		hps_set_cpu_num_limit(LIMIT_THERMAL, p->limited_max_ncpu, 0);
#endif
		/* correct opp idx will be calcualted in _thermal_limited_verify() */
		_mt_cpufreq_set(MT_CPU_DVFS_LITTLE, -1);
		cpufreq_cpu_put(policy);	/* <- policy put */
	}
no_policy:
#endif

	FUNC_EXIT(FUNC_LV_API);
}
EXPORT_SYMBOL(mt_cpufreq_thermal_protect);

static int _search_available_freq_idx(struct mt_cpu_dvfs *p, unsigned int target_khz,
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
	} else {		/* CPUFREQ_RELATION_H */
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

static int _thermal_limited_verify(struct mt_cpu_dvfs *p, int new_opp_idx)
{
	unsigned int target_khz = cpu_dvfs_get_freq_by_idx(p, new_opp_idx);
	int possible_cpu = 0;
	unsigned int online_cpu = 0;
	int found = 0;
	int i;

	FUNC_ENTER(FUNC_LV_HELP);

	possible_cpu = max_cpu_num;
	online_cpu = num_online_cpus();

	/* cpufreq_dbg("%s(): begin, idx = %d, online_cpu = %d\n", __func__, new_opp_idx, online_cpu); */

	/* no limited */
	if (0 == p->thermal_protect_limited_power)
		return new_opp_idx;

	for (i = 0; i < p->nr_opp_tbl * possible_cpu; i++) {
		if (p->power_tbl[i].cpufreq_ncpu == p->limited_max_ncpu
		    && p->power_tbl[i].cpufreq_khz == p->limited_max_freq)
			break;
	}

	cpufreq_ver("%s(): idx = %d, limited_max_ncpu = %d, limited_max_freq = %d\n", __func__, i,
		     p->limited_max_ncpu, p->limited_max_freq);

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
		cpufreq_ver("%s(): freq not found, set to limited_max_freq = %d\n", __func__,
			     target_khz);
	}

	i = _search_available_freq_idx(p, target_khz, CPUFREQ_RELATION_H);

	FUNC_EXIT(FUNC_LV_HELP);

	return i;
}

static unsigned int _calc_new_opp_idx(struct mt_cpu_dvfs *p, int new_opp_idx)
{
	int idx;

	FUNC_ENTER(FUNC_LV_HELP);

	BUG_ON(NULL == p);

	/* HEVC */
	if (p->limited_freq_by_hevc) {
		idx = _search_available_freq_idx(p, p->limited_freq_by_hevc, CPUFREQ_RELATION_L);

		if (idx != -1 && new_opp_idx > idx) {
			new_opp_idx = idx;
			cpufreq_ver("%s(): hevc limited freq, idx = %d\n", __func__, new_opp_idx);
		}
	}

	/* search thermal limited freq */
	idx = _thermal_limited_verify(p, new_opp_idx);

	if (idx != -1 && idx != new_opp_idx) {
		new_opp_idx = idx;
		cpufreq_ver("%s(): thermal limited freq, idx = %d\n", __func__, new_opp_idx);
	}

	/* for early suspend */
	if (p->dvfs_disable_by_early_suspend) {
		new_opp_idx = 3;
		cpufreq_ver("%s(): for early suspend, idx = %d\n", __func__, new_opp_idx);
	}

	/* for suspend */
	if (p->dvfs_disable_by_suspend)
		new_opp_idx = p->idx_normal_max_opp;

	/* limit max freq by user */
	if (p->limited_max_freq_by_user) {
		idx =
		    _search_available_freq_idx(p, p->limited_max_freq_by_user, CPUFREQ_RELATION_H);

		if (idx != -1 && new_opp_idx < idx) {
			new_opp_idx = idx;
			cpufreq_ver("%s(): limited max freq by user, idx = %d\n", __func__,
				    new_opp_idx);
		}
	}

	/* for ptpod init */
	if (p->dvfs_disable_by_ptpod) {
		/* at least CPU_DVFS_FREQ6 will make sure VBoot >= 1V */
		/* idx = _search_available_freq_idx(p, CPU_DVFS_FREQ6, CPUFREQ_RELATION_L); */
		idx = _search_available_freq_idx(p, CPU_DVFS_FREQ1, CPUFREQ_RELATION_L);

		if (idx != -1) {
			new_opp_idx = idx;
			cpufreq_info("%s(): for ptpod init, idx = %d\n", __func__, new_opp_idx);
		}
	}

	FUNC_EXIT(FUNC_LV_HELP);

	return new_opp_idx;
}

/*
 * cpufreq driver
 */
static int _mt_cpufreq_verify(struct cpufreq_policy *policy)
{
	struct mt_cpu_dvfs *p;
	int ret = 0;

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


	int ret = 0;

	FUNC_ENTER(FUNC_LV_MODULE);

	if (policy->cpu >= max_cpu_num
	    || cpufreq_frequency_table_target(policy, id_to_cpu_dvfs(id)->freq_tbl_for_cpufreq,
					      target_freq, relation, &new_opp_idx)
	    || (id_to_cpu_dvfs(id) && id_to_cpu_dvfs(id)->dvfs_disable_by_procfs)
	    )
		return -EINVAL;

	_mt_cpufreq_set(id, new_opp_idx);

	FUNC_EXIT(FUNC_LV_MODULE);

	return ret;
}

static int _mt_cpufreq_init(struct cpufreq_policy *policy)
{
	int ret = -EINVAL;

	FUNC_ENTER(FUNC_LV_MODULE);

	max_cpu_num = num_possible_cpus();

	if (policy->cpu >= max_cpu_num)
		return -EINVAL;

	cpufreq_info("@%s: max_cpu_num: %d\n", __func__, max_cpu_num);

	policy->shared_type = CPUFREQ_SHARED_TYPE_ANY;
	cpumask_setall(policy->cpus);

	policy->cpuinfo.transition_latency = 1000;

	{
#define DORMANT_MODE_VOLT   80000

		enum mt_cpu_dvfs_id id = _get_cpu_dvfs_id(policy->cpu);
		struct mt_cpu_dvfs *p = id_to_cpu_dvfs(id);
		unsigned int lv = _mt_cpufreq_get_cpu_level();
		struct opp_tbl_info *opp_tbl_info = &opp_tbls[CPU_LV_TO_OPP_IDX(lv)];

		BUG_ON(NULL == p);
		BUG_ON(!
		       (lv == CPU_LEVEL_0 || lv == CPU_LEVEL_1 || lv == CPU_LEVEL_2
			|| lv == CPU_LEVEL_3));

		p->cpu_level = lv;
		p->ops = &dvfs_ops_pmic_wrap;

		ret = _mt_cpufreq_setup_freqs_table(policy,
						    opp_tbl_info->opp_tbl, opp_tbl_info->size);

		policy->cpuinfo.max_freq = cpu_dvfs_get_max_freq(id_to_cpu_dvfs(id));
		policy->cpuinfo.min_freq = cpu_dvfs_get_min_freq(id_to_cpu_dvfs(id));

		policy->cur = _mt_cpufreq_get_cur_phy_freq(id);	/* use cur phy freq is better */
		policy->max = cpu_dvfs_get_max_freq(id_to_cpu_dvfs(id));
		policy->min = cpu_dvfs_get_min_freq(id_to_cpu_dvfs(id));

		if (_sync_opp_tbl_idx(p) >= 0)	/* sync p->idx_opp_tbl first before _restore_default_volt() */
			p->idx_normal_max_opp = p->idx_opp_tbl;

		/* restore default volt, sync opp idx, set default limit */
		_restore_default_volt(p);
		_set_no_limited(p);
	}

	if (ret)
		cpufreq_err("failed to setup frequency table\n");

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

static void _mt_cpufreq_lcm_status_switch(int onoff)
{
	struct mt_cpu_dvfs *p;
	int i;
#ifdef CONFIG_CPU_FREQ
	struct cpufreq_policy *policy;
#endif
	cpufreq_ver("@%s: LCM is %s\n", __func__, (onoff) ? "on" : "off");

	/* onoff = 0: LCM OFF */
	/* others: LCM ON */
	if (onoff) {
		for_each_cpu_dvfs(i, p) {
			if (!cpu_dvfs_is_available(p))
				continue;

			p->dvfs_disable_by_early_suspend = false;

#ifdef CONFIG_CPU_FREQ
			policy = cpufreq_cpu_get(p->cpu_id);
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
	} else {
		for_each_cpu_dvfs(i, p) {
			if (!cpu_dvfs_is_available(p))
				continue;

			p->dvfs_disable_by_early_suspend = true;

			p->idx_opp_tbl_for_late_resume = p->idx_opp_tbl;

#ifdef CONFIG_CPU_FREQ
			policy = cpufreq_cpu_get(p->cpu_id);
			if (policy) {
				cpufreq_driver_target(
					policy,
					cpu_dvfs_get_freq_by_idx(p, 3), CPUFREQ_RELATION_L);
				cpufreq_cpu_put(policy);
			}
#endif
		}
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

/*
 * Platform driver
 */
static int _mt_cpufreq_suspend(struct device *dev)
{
	struct mt_cpu_dvfs *p;
	int i;

	FUNC_ENTER(FUNC_LV_MODULE);

	for_each_cpu_dvfs(i, p) {
		if (!cpu_dvfs_is_available(p))
			continue;

		p->dvfs_disable_by_suspend = true;
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

static int _mt_cpufreq_pm_restore_early(struct device *dev)
{
	FUNC_ENTER(FUNC_LV_MODULE);

	_mt_cpufreq_sync_opp_tbl_idx();

	FUNC_EXIT(FUNC_LV_MODULE);

	return 0;
}

static int _mt_cpufreq_pdrv_probe(struct platform_device *pdev)
{
	FUNC_ENTER(FUNC_LV_MODULE);

	if (fb_register_client(&_mt_cpufreq_fb_notifier))
		cpufreq_err("@%s: register FB client failed!\n", __func__);

	if (pw.addr[0].cmd_addr == 0)
		_mt_cpufreq_pmic_table_init();

	/* init PMIC_WRAP & volt */
	mt_cpufreq_set_pmic_phase(PMIC_WRAP_PHASE_NORMAL);

#ifdef CONFIG_CPU_FREQ
	cpufreq_register_driver(&_mt_cpufreq_driver);
#endif

	FUNC_EXIT(FUNC_LV_MODULE);

	return 0;
}

static int _mt_cpufreq_pdrv_remove(struct platform_device *pdev)
{
	FUNC_ENTER(FUNC_LV_MODULE);

#ifdef CONFIG_CPU_FREQ
	cpufreq_unregister_driver(&_mt_cpufreq_driver);
#endif

	FUNC_EXIT(FUNC_LV_MODULE);

	return 0;
}

static const struct dev_pm_ops _mt_cpufreq_pm_ops = {
	.suspend = _mt_cpufreq_suspend,
	.resume = _mt_cpufreq_resume,
	.restore_early = _mt_cpufreq_pm_restore_early,
	.freeze = _mt_cpufreq_suspend,
	.thaw = _mt_cpufreq_resume,
	.restore = _mt_cpufreq_resume,
};

struct platform_device _mt_cpufreq_pdev = {
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
	int rc;
	char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	rc = kstrtoint(buf, 10, &dbg_lv);
	if (rc < 0)
		cpufreq_err("echo dbg_lv (dec) > /proc/cpufreq/cpufreq_debug\n");
	else
		func_lv_mask = dbg_lv;

	free_page((unsigned int)buf);
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
	int rc;
	char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	rc = kstrtoint(buf, 10, &do_stress);
	if (rc < 0)
		cpufreq_err("echo 0/1 > /proc/cpufreq/cpufreq_stress_test\n");
	else
		do_dvfs_stress_test = do_stress;

	free_page((unsigned int)buf);
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
	int rc;

	char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	rc = kstrtoint(buf, 10, &limited_freq_by_hevc);
	if (rc < 0)
		cpufreq_err(
		"echo limited_freq_by_hevc (dec) > /proc/cpufreq/cpufreq_limited_by_hevc\n");
	else {
		p->limited_freq_by_hevc = limited_freq_by_hevc;

		if (cpu_dvfs_is_available(p)
		    && (p->limited_freq_by_hevc > cpu_dvfs_get_cur_freq(p))) {
#ifdef CONFIG_CPU_FREQ
			struct cpufreq_policy *policy = cpufreq_cpu_get(p->cpu_id);

			if (policy) {
				cpufreq_driver_target(policy, p->limited_freq_by_hevc,
						      CPUFREQ_RELATION_L);
				cpufreq_cpu_put(policy);
			}
#endif
		}
	}
	free_page((unsigned int)buf);
	return count;
}

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
	int rc;

	char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	rc = kstrtoint(buf, 10, &limited_max_freq);
	if (rc < 0)
		cpufreq_err(
		"echo limited_max_freq (dec) > /proc/cpufreq/%s/cpufreq_limited_max_freq_by_user\n",
		     p->name);
	else {
		p->limited_max_freq_by_user = limited_max_freq;

		if (cpu_dvfs_is_available(p) && (p->limited_max_freq_by_user != 0)
		    && (p->limited_max_freq_by_user < cpu_dvfs_get_cur_freq(p))) {
			struct cpufreq_policy *policy = cpufreq_cpu_get(p->cpu_id);

			if (policy) {
				cpufreq_driver_target(policy, p->limited_max_freq_by_user,
						      CPUFREQ_RELATION_H);
				cpufreq_cpu_put(policy);
			}
		}
	}
	free_page((unsigned long)buf);
	return count;
}


/* cpufreq_limited_power */
static int cpufreq_limited_power_proc_show(struct seq_file *m, void *v)
{
	struct mt_cpu_dvfs *p;
	int i;

	for_each_cpu_dvfs(i, p) {
		seq_printf(m, "[%s/%d]\n"
			   "limited_max_freq = %d\n"
			   "limited_max_ncpu = %d\n",
			   p->name, i, p->limited_max_freq, p->limited_max_ncpu);
	}

	return 0;
}

static ssize_t cpufreq_limited_power_proc_write(struct file *file, const char __user *buffer,
						size_t count, loff_t *pos)
{
	int limited_power;
	int rc;
	char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	rc = kstrtoint(buf, 10, &limited_power);
	if (rc < 0)
		cpufreq_err("echo limited_power (dec) > /proc/cpufreq/cpufreq_limited_power\n");
	else
		mt_cpufreq_thermal_protect(limited_power);

	free_page((unsigned int)buf);
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

/* cpufreq_oppidx */
static int cpufreq_oppidx_proc_show(struct seq_file *m, void *v)
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

static ssize_t cpufreq_oppidx_proc_write(struct file *file, const char __user *buffer,
					 size_t count, loff_t *pos)
{
	struct mt_cpu_dvfs *p = (struct mt_cpu_dvfs *)PDE_DATA(file_inode(file));
	int oppidx;
	int rc;

	char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	BUG_ON(NULL == p);

	rc = kstrtoint(buf, 10, &oppidx);
	if (rc < 0) {
		p->dvfs_disable_by_procfs = false;
		cpufreq_err("echo oppidx > /proc/cpufreq/cpufreq_oppidx (0 <= %d < %d)\n", oppidx,
			    p->nr_opp_tbl);
	} else {
		if (0 <= oppidx && oppidx < p->nr_opp_tbl) {
			p->dvfs_disable_by_procfs = true;
			_mt_cpufreq_set(MT_CPU_DVFS_LITTLE, oppidx);
		} else {
			p->dvfs_disable_by_procfs = false;
			cpufreq_err("echo oppidx > /proc/cpufreq/cpufreq_oppidx (0 <= %d < %d)\n", oppidx,
					p->nr_opp_tbl);
		}
	}

	free_page((unsigned int)buf);

	return count;
}

/* cpufreq_freq */
static int cpufreq_freq_proc_show(struct seq_file *m, void *v)
{
	struct mt_cpu_dvfs *p = (struct mt_cpu_dvfs *)m->private;

	seq_printf(m, "%d KHz\n", p->ops->get_cur_phy_freq(p));

	return 0;
}

static ssize_t cpufreq_freq_proc_write(struct file *file, const char __user *buffer, size_t count,
				       loff_t *pos)
{
	unsigned long flags;
	struct mt_cpu_dvfs *p = (struct mt_cpu_dvfs *)PDE_DATA(file_inode(file));
	unsigned int cur_freq;
	int freq, i, found;
	int rc;

	char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	BUG_ON(NULL == p);

	rc = kstrtoint(buf, 10, &freq);
	if (rc < 0) {
		p->dvfs_disable_by_procfs = false;
		cpufreq_err("echo khz > /proc/cpufreq/cpufreq_freq\n");
	} else {
		if (freq < CPUFREQ_LAST_FREQ_LEVEL) {
			if (freq != 0)
				cpufreq_err("frequency should higher than %dKHz!\n",
					    CPUFREQ_LAST_FREQ_LEVEL);

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

				if (freq != cur_freq)
					p->ops->set_cur_freq(p, cur_freq, freq);

				cpufreq_unlock(flags);
			} else {
				p->dvfs_disable_by_procfs = false;
				cpufreq_err("frequency %dKHz! is not found in CPU opp table\n",
					    freq);
			}
		}
	}

end:
	free_page((unsigned int)buf);

	return count;
}

/* cpufreq_volt */
static int cpufreq_volt_proc_show(struct seq_file *m, void *v)
{
	struct mt_cpu_dvfs *p = (struct mt_cpu_dvfs *)m->private;

	seq_printf(m, "%d mv\n", p->ops->get_cur_volt(p) / 100);	/* mv */

	return 0;
}

static ssize_t cpufreq_volt_proc_write(struct file *file, const char __user *buffer, size_t count,
				       loff_t *pos)
{
	unsigned long flags;
	struct mt_cpu_dvfs *p = (struct mt_cpu_dvfs *)PDE_DATA(file_inode(file));
	int mv;
	int rc;

	char *buf = _copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	rc = kstrtoint(buf, 10, &mv);

	if (rc < 0) {
		p->dvfs_disable_by_procfs = false;
		cpufreq_err("echo mv > /proc/cpufreq/cpufreq_volt\n");
	} else {
		p->dvfs_disable_by_procfs = true;
		cpufreq_lock(flags);
		_set_cur_volt_locked(p, mv * 100);
		cpufreq_unlock(flags);
	}
	free_page((unsigned int)buf);

	return count;
}

#define PROC_FOPS_RW(name)							\
static int name ## _proc_open(struct inode *inode, struct file *file)	\
{									\
	return single_open(file, name ## _proc_show, PDE_DATA(inode));	\
}									\
static const struct file_operations name ## _proc_fops = {		\
	.owner          = THIS_MODULE,					\
	.open           = name ## _proc_open,				\
	.read           = seq_read,					\
	.llseek         = seq_lseek,					\
	.release        = single_release,				\
	.write          = name ## _proc_write,				\
}

#define PROC_FOPS_RO(name)							\
static int name ## _proc_open(struct inode *inode, struct file *file)	\
{									\
	return single_open(file, name ## _proc_show, PDE_DATA(inode));	\
}									\
static const struct file_operations name ## _proc_fops = {		\
	.owner          = THIS_MODULE,					\
	.open           = name ## _proc_open,				\
	.read           = seq_read,					\
	.llseek         = seq_lseek,					\
	.release        = single_release,				\
}

#define PROC_ENTRY(name)	{__stringify(name), &name ## _proc_fops}

PROC_FOPS_RW(cpufreq_debug);
PROC_FOPS_RW(cpufreq_stress_test);
PROC_FOPS_RW(cpufreq_limited_power);
PROC_FOPS_RO(cpufreq_power_dump);

PROC_FOPS_RW(cpufreq_limited_by_hevc);
PROC_FOPS_RW(cpufreq_limited_max_freq_by_user);
PROC_FOPS_RO(cpufreq_ptpod_freq_volt);
PROC_FOPS_RW(cpufreq_oppidx);
PROC_FOPS_RW(cpufreq_freq);
PROC_FOPS_RW(cpufreq_volt);

static int _create_procfs(void)
{
	struct proc_dir_entry *dir = NULL;
	/* struct proc_dir_entry *cpu_dir = NULL; */
	struct mt_cpu_dvfs *p = id_to_cpu_dvfs(0);
	int i;

	struct pentry {
		const char *name;
		const struct file_operations *fops;
	};

	const struct pentry entries[] = {
		PROC_ENTRY(cpufreq_debug),
		PROC_ENTRY(cpufreq_stress_test),
		PROC_ENTRY(cpufreq_limited_power),
		PROC_ENTRY(cpufreq_power_dump),
	};

	const struct pentry cpu_entries[] = {
		PROC_ENTRY(cpufreq_limited_by_hevc),
		PROC_ENTRY(cpufreq_limited_max_freq_by_user),
		PROC_ENTRY(cpufreq_ptpod_freq_volt),
		PROC_ENTRY(cpufreq_oppidx),
		PROC_ENTRY(cpufreq_freq),
		PROC_ENTRY(cpufreq_volt),
	};

	dir = proc_mkdir("cpufreq", NULL);

	if (!dir) {
		cpufreq_err("fail to create /proc/cpufreq @ %s()\n", __func__);
		return -ENOMEM;
	}

	for (i = 0; i < ARRAY_SIZE(entries); i++) {
		if (!proc_create
		    (entries[i].name, S_IRUGO | S_IWUSR | S_IWGRP, dir, entries[i].fops))
			cpufreq_err("%s(), create /proc/cpufreq/%s failed\n", __func__,
				    entries[i].name);
	}

	for (i = 0; i < ARRAY_SIZE(cpu_entries); i++) {
		if (!proc_create_data
		    (cpu_entries[i].name, S_IRUGO | S_IWUSR | S_IWGRP, dir, cpu_entries[i].fops, p))
			cpufreq_err("%s(), create /proc/cpufreq/%s failed\n", __func__,
				    cpu_entries[i].name);
	}
	return 0;
}
#endif				/* CONFIG_PROC_FS */

/*
 * Module driver
 */
static int __init _mt_cpufreq_pdrv_init(void)
{
	int ret = 0;

	FUNC_ENTER(FUNC_LV_MODULE);

#ifdef CONFIG_PROC_FS

	/* init proc */
	if (_create_procfs())
		goto out;

#endif				/* CONFIG_PROC_FS */

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
