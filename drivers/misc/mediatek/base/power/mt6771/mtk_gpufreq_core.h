/*
 * Copyright (C) 2017 MediaTek Inc.
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

#ifndef _MT_GPUFREQ_CORE_H_
#define _MT_GPUFREQ_CORE_H_

/**************************************************
 * MT6763TT : GPU DVFS OPP table Setting
 **************************************************/
#define GPU_DVFS_FREQ0					(800000)	/* KHz */
#define GPU_DVFS_FREQ1					(764000)	/* KHz */
#define GPU_DVFS_FREQ2					(728000)	/* KHz */
#define GPU_DVFS_FREQ3					(692000)	/* KHz */
#define GPU_DVFS_FREQ4					(656000)	/* KHz */
#define GPU_DVFS_FREQ5					(620000)	/* KHz */
#define GPU_DVFS_FREQ6					(589000)	/* KHz */
#define GPU_DVFS_FREQ7					(558000)	/* KHz */
#define GPU_DVFS_FREQ8					(527000)	/* KHz */
#define GPU_DVFS_FREQ9					(496000)	/* KHz */
#define GPU_DVFS_FREQ10				(465000)	/* KHz */
#define GPU_DVFS_FREQ11				(434000)	/* KHz */
#define GPU_DVFS_FREQ12				(403000)	/* KHz */
#define GPU_DVFS_FREQ13				(372000)	/* KHz */
#define GPU_DVFS_FREQ14				(341000)	/* KHz */
#define GPU_DVFS_FREQ15				(310000)	/* KHz */

#define GPU_DVFS_VOLT0					(80000)	/* mV x 100 */
#define GPU_DVFS_VOLT1					(78125)	/* mV x 100 */
#define GPU_DVFS_VOLT2					(76875)	/* mV x 100 */
#define GPU_DVFS_VOLT3					(74375)	/* mV x 100 */
#define GPU_DVFS_VOLT4					(72500)	/* mV x 100 */
#define GPU_DVFS_VOLT5					(70000)	/* mV x 100 */
#define GPU_DVFS_VOLT6					(69375)	/* mV x 100 */
#define GPU_DVFS_VOLT7					(68125)	/* mV x 100 */
#define GPU_DVFS_VOLT8					(67500)	/* mV x 100 */
#define GPU_DVFS_VOLT9					(66250)	/* mV x 100 */
#define GPU_DVFS_VOLT10				(65000)	/* mV x 100 */
#define GPU_DVFS_VOLT11				(64375)	/* mV x 100 */
#define GPU_DVFS_VOLT12				(63125)	/* mV x 100 */
#define GPU_DVFS_VOLT13				(62500)	/* mV x 100 */
#define GPU_DVFS_VOLT14				(61250)	/* mV x 100 */
#define GPU_DVFS_VOLT15				(60000)	/* mV x 100 */

#define GPUFREQ_LAST_FREQ_LEVEL_6775	(GPU_DVFS_FREQ15)

/**************************************************
 * PMIC Setting
 **************************************************/
#define PMIC_MAX_VGPU					(GPU_DVFS_VOLT0)
#define PMIC_MIN_VGPU					(GPU_DVFS_VOLT15)

#define DELAY_FACTOR					(1408)
#define VGPU_ENABLE_TIME_US			(240)	/* spec is 220(us) */

/**************************************************
 * efuse Setting
 **************************************************/
#define GPUFREQ_EFUSE_INDEX			(8)
#define EFUSE_MFG_SPD_BOND_SHIFT		(8)
#define EFUSE_MFG_SPD_BOND_MASK		(0xF)
#define FUNC_CODE_EFUSE_INDEX			(22)

/**************************************************
 * Clock Setting
 **************************************************/
#define POST_DIV_2_MAX_FREQ			(1900000)
#define POST_DIV_2_MIN_FREQ			(750000)
#define POST_DIV_4_MAX_FREQ			(950000)
#define POST_DIV_4_MIN_FREQ			(375000)
#define POST_DIV_8_MAX_FREQ			(475000)
#define POST_DIV_8_MIN_FREQ			(187500)
#define POST_DIV_16_MAX_FREQ			(237500)
#define POST_DIV_16_MIN_FREQ			(93750)
#define POST_DIV_MASK					(0x70000000)
#define POST_DIV_SHIFT					(28)
#define TO_MHz_HEAD					(100)
#define TO_MHz_TAIL					(10)
#define ROUNDING_VALUE					(5)
#define DDS_SHIFT						(14)
#define GPUPLL_FIN						(26)
#define GPUPLL_CON0					(REG_FH_PLL6_CON0)
#define GPUPLL_CON1					(REG_FH_PLL6_CON1)

/**************************************************
 * Buck Setting
 **************************************************/
#define VGPU							(1)
#define BUCK_ON						(1)
#define BUCK_OFF						(0)
#define BUCK_ENFORCE_OFF				(4)

/**************************************************
 * Reference Power Setting
 **************************************************/
#define GPU_ACT_REF_POWER				(1018)	/* mW  */
#define GPU_ACT_REF_FREQ				(950000)	/* KHz */
#define GPU_ACT_REF_VOLT				(90000)	/* mV x 100 */
#define GPU_DVFS_PTPOD_DISABLE_VOLT	(80000)	/* mV x 100 */

/**************************************************
 * Log Setting
 **************************************************/
#define GPUFERQ_TAG							"[GPU/DVFS] "
#define gpufreq_pr_err(fmt, args...)		pr_err(GPUFERQ_TAG"[ERROR]"fmt, ##args)
#define gpufreq_pr_warn(fmt, args...)		pr_warn(GPUFERQ_TAG"[WARNING]"fmt, ##args)
#define gpufreq_pr_debug(fmt, args...)		pr_debug(GPUFERQ_TAG"[DEBUG]"fmt, ##args)

/**************************************************
 * Condition Setting
 **************************************************/
/* #define MT_GPUFREQ_OPP_STRESS_TEST */
#define MT_GPUFREQ_STATIC_PWR_READY2USE
#define MT_GPUFREQ_LOW_BATT_VOLT_PROTECT
#define MT_GPUFREQ_BATT_PERCENT_PROTECT
#define MT_GPUFREQ_BATT_OC_PROTECT
#define MT_GPUFREQ_DYNAMIC_POWER_TABLE_UPDATE
#ifdef CONFIG_MTK_RAM_CONSOLE
/* #define MT_GPUFREQ_AEE_RR_REC */
/* #define MT_GPUFERQ_PBM_SUPPORT */
#endif

/**************************************************
 * Battery Over Current Protect
 **************************************************/
#ifdef MT_GPUFREQ_BATT_OC_PROTECT
#define MT_GPUFREQ_BATT_OC_LIMIT_FREQ				GPU_DVFS_FREQ14	/* 442 MHz */
#endif

/**************************************************
 * Battery Percentage Protect
 **************************************************/
#ifdef MT_GPUFREQ_BATT_PERCENT_PROTECT
#define MT_GPUFREQ_BATT_PERCENT_LIMIT_FREQ			GPU_DVFS_FREQ0
#endif

/**************************************************
 * Low Battery Volume Protect
 **************************************************/
#ifdef MT_GPUFREQ_LOW_BATT_VOLT_PROTECT
#define MT_GPUFREQ_LOW_BATT_VOLT_LIMIT_FREQ_1		GPU_DVFS_FREQ0	/* no need to throttle when LV1 */
#define MT_GPUFREQ_LOW_BATT_VOLT_LIMIT_FREQ_2		GPU_DVFS_FREQ14	/* 485 MHz */
#endif

/**************************************************
 * Proc Node Definition
 **************************************************/
#ifdef CONFIG_PROC_FS
#define PROC_FOPS_RW(name)									\
static int mt_ ## name ## _proc_open(struct inode *inode, struct file *file)	\
{	\
	return single_open(file, mt_ ## name ## _proc_show, PDE_DATA(inode));	\
} \
static const struct file_operations mt_ ## name ## _proc_fops = {	\
	.owner		= THIS_MODULE,	\
	.open		= mt_ ## name ## _proc_open,	\
	.read		= seq_read,	\
	.llseek	= seq_lseek,	\
	.release	= single_release,	\
	.write		= mt_ ## name ## _proc_write,	\
}
#define PROC_FOPS_RO(name)	\
static int mt_ ## name ## _proc_open(struct inode *inode, struct file *file)	\
{	\
	return single_open(file, mt_ ## name ## _proc_show, PDE_DATA(inode));	\
}	\
static const struct file_operations mt_ ## name ## _proc_fops =	\
{	\
	.owner		= THIS_MODULE,	\
	.open		= mt_ ## name ## _proc_open,	\
	.read		= seq_read,	\
	.llseek	= seq_lseek,	\
	.release	= single_release,	\
}
#define PROC_ENTRY(name) \
{__stringify(name), &mt_ ## name ## _proc_fops}
#endif

/**************************************************
 * Operation Definition
 **************************************************/
#define VOLT_NORMALISATION(volt) ((volt % 625) ? (volt - (volt % 625) + 625) : volt)
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define GPUOP(khz, volt, idx)	\
{	\
	.gpufreq_khz = khz,	\
	.gpufreq_volt = volt,	\
	.gpufreq_idx = idx,	\
}

/**************************************************
 * Enumerations
 **************************************************/
enum g_device_id_enum {
	DEVICE_MT6771 = 0,
};
enum g_post_div_order_enum {
	POST_DIV2 = 1,
	POST_DIV4,
	POST_DIV8,
	POST_DIV16,
};
enum g_limited_idx_enum {
	IDX_THERMAL_PROTECT_LIMITED = 0,
	IDX_LOW_BATT_LIMITED,
	IDX_BATT_PERCENT_LIMITED,
	IDX_BATT_OC_LIMITED,
	IDX_PBM_LIMITED,
	NUMBER_OF_LIMITED_IDX,
};
#ifdef MT_GPUFREQ_AEE_RR_REC
enum gpu_dvfs_state_enum {
	GPU_DVFS_IS_DOING_DVFS = 0,
	GPU_DVFS_IS_VGPU_ENABLED,
};
#endif

/**************************************************
 * Structures
 **************************************************/
struct g_opp_table_info {
	unsigned int gpufreq_khz;
	unsigned int gpufreq_volt;
	unsigned int gpufreq_idx;
};
struct g_clk_info {
	struct clk *clk_mux;	/* main clock for mfg setting */
	struct clk *clk_main_parent;	/* substitution clock for mfg transient mux setting */
	struct clk *clk_sub_parent;	/* substitution clock for mfg transient parent setting */
	struct clk *mtcmos_mfg_async;	/* 0 */
	struct clk *mtcmos_mfg;	/* 1 */
	struct clk *mtcmos_mfg_core0;	/* 2 */
	struct clk *mtcmos_mfg_core1;	/* 3 */
	struct clk *mtcmos_mfg_core2;	/* 4 */
	struct clk *mtcmos_mfg_core3;	/* 5 */
	struct clk *subsys_mfg_cg;	/* Clock Gating */

};
struct g_pmic_info {
	struct regulator *reg_vgpu;
};
/**
 * ===============================================
 * SECTION : External functions declaration
 * ===============================================
 */

typedef unsigned int u32;
extern unsigned int mt_get_ckgen_freq(unsigned int);
extern u32 get_devinfo_with_index(u32 index);
#ifdef MT_GPUFREQ_AEE_RR_REC
extern void aee_rr_rec_gpu_dvfs_vgpu(u8 val);
extern void aee_rr_rec_gpu_dvfs_oppidx(u8 val);
extern void aee_rr_rec_gpu_dvfs_status(u8 val);
extern u8 aee_rr_curr_gpu_dvfs_status(void);
#endif				/* ifdef MT_GPUFREQ_AEE_RR_REC */

#endif				/* _MT_GPUFREQ_CORE_H_ */
