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

/*
 * @file    mt_ptp.c
 * @brief   Driver for PTP
 *
 */

#define __MT_PTP_C__

/*
 * Include files
 */

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
#include <linux/uaccess.h>
#include <linux/seq_file.h>
#include <asm/io.h>
#include "mt_ptp.h"
#include "mt_cpufreq.h"
#include "mach/mt_thermal.h"
#include "mach/mt_clkmgr.h"
#include "mach/mt_freqhopping.h"
#include "mt-plat/upmu_common.h"

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_fdt.h>
#endif
/* local includes */
#include "mt_spm.h"
#include "aee.h"
#include <linux/gpio.h>

/* Global variable for slow idle*/
unsigned int ptp_data[3] = { 0, 0, 0 };

struct ptp_det;
struct ptp_ctrl;

static void ptp_set_ptp_volt(struct ptp_det *det);
static void ptp_restore_ptp_volt(struct ptp_det *det);

#define CONFIG_PTP_SHOWLOG 1

#define EN_ISR_LOG			 (0)

#define PTP_GET_REAL_VAL	 (1)	/* get val from efuse */
#define SET_PMIC_VOLT		 (1)	/* apply PMIC voltage */

#define DUMP_DATA_TO_DE      (0)

#define LOG_INTERVAL		 (2LL * NSEC_PER_SEC)
#define NR_FREQ				 8

/*
 * 100 us, This is the PTP Detector sampling time as represented in
 * cycles of bclk_ck during INIT. 52 MHz
 */
/* #define DETWINDOW_VAL		0xa28 */
#define DETWINDOW_VAL		0x514

#define PTP_VOLT_TO_PMIC_VAL(volt)  (((volt) - 70000 + 625 - 1) / 625)
#define PTP_PMIC_VAL_TO_VOLT(pmic)  (((pmic) * 625) + 60000)

/* offset 0x10(16 steps) for CPU/GPU DVFS */
#define PTPOD_PMIC_OFFSET (0x10)


#define VMAX_VAL			PTP_VOLT_TO_PMIC_VAL(125000)
#define VMIN_VAL			PTP_VOLT_TO_PMIC_VAL(95000)

#define DTHI_VAL		0x01
#define DTLO_VAL		0xfe
#define DETMAX_VAL		0xffff
#define AGECONFIG_VAL	0x555555
#define AGEM_VAL		0x0
#define DVTFIXED_VAL	0x6
#define VCO_VAL			0x28
#define DCCONFIG_VAL	0x555555

/*
 * bit operation
 */
#undef  BIT
#define BIT(bit)	(1U << (bit))

#define MSB(range)	(1 ? range)
#define LSB(range)	(0 ? range)
/*
 * Genearte a mask wher MSB to LSB are all 0b1
 * @r:	Range in the form of MSB:LSB
 */
#define BITMASK(r)	\
	(((unsigned) -1 >> (31 - MSB(r))) & ~((1U << LSB(r)) - 1))

/*
 * Set value at MSB:LSB. For example, BITS(7:3, 0x5A)
 * will return a value where bit 3 to bit 7 is 0x5A
 * @r:	Range in the form of MSB:LSB
 */
/* BITS(MSB:LSB, value) => Set value at MSB:LSB  */
#define BITS(r, val)	((val << LSB(r)) & BITMASK(r))

/*
 * LOG
 */
#define ptp_emerg(fmt, args...)     pr_err("[PTP] " fmt, ##args)
#define ptp_alert(fmt, args...)     pr_err("[PTP] " fmt, ##args)
#define ptp_crit(fmt, args...)      pr_err("[PTP] " fmt, ##args)
#define ptp_error(fmt, args...)     pr_err("[PTP] " fmt, ##args)
#define ptp_warning(fmt, args...)   pr_warn("[PTP] " fmt, ##args)
#define ptp_notice(fmt, args...)    pr_warn("[PTP] " fmt, ##args)
#define ptp_info(fmt, args...)      pr_warn("[PTP] " fmt, ##args)
#define ptp_debug(fmt, args...)     pr_warn("[PTP] " fmt, ##args)

#if EN_ISR_LOG
#define ptp_isr_info(fmt, args...)  ptp_notice(fmt, ##args)
#else
#define ptp_isr_info(fmt, args...)
#endif

#define FUNC_LV_MODULE          BIT(0)	/* module, platform driver interface */
#define FUNC_LV_CPUFREQ         BIT(1)	/* cpufreq driver interface          */
#define FUNC_LV_API             BIT(2)	/* mt_cpufreq driver global function */
#define FUNC_LV_LOCAL           BIT(3)	/* mt_cpufreq driver lcaol function  */
#define FUNC_LV_HELP            BIT(4)	/* mt_cpufreq driver help function   */

static unsigned int func_lv_mask;

#if defined(CONFIG_PTP_SHOWLOG)
#define FUNC_ENTER(lv)          do { if ((lv) & func_lv_mask) ptp_debug(">> %s()\n", __func__); } while (0)
#define FUNC_EXIT(lv)           do { if ((lv) & func_lv_mask) ptp_debug("<< %s():%d\n", __func__, __LINE__); } while (0)
#else
#define FUNC_ENTER(lv)
#define FUNC_EXIT(lv)
#endif				/* CONFIG_CPU_DVFS_SHOWLOG */

/*
 * REG ACCESS
 */

#define ptp_read(addr)	__raw_readl(addr)
#define ptp_read_field(addr, range)	\
	((ptp_read(addr) & BITMASK(range)) >> LSB(range))

#define ptp_write(addr, val)	mt_reg_sync_writel(val, addr)
/*
 * Write a field of a register.
 * @addr:	Address of the register
 * @range:	The field bit range in the form of MSB:LSB
 * @val:	The value to be written to the field
 */
#define ptp_write_field(addr, range, val)	\
	ptp_write(addr, (ptp_read(addr) & ~BITMASK(range)) | BITS(range, val))

/*
 * Helper macros
 */

/* PTP detector is disabled by who */
enum {
	BY_PROCFS = BIT(0),
	BY_INIT_ERROR = BIT(1),
	BY_MON_ERROR = BIT(2),
};

#ifdef CONFIG_OF

void __iomem *ptpod_base;
static u32 ptpod_irq_number;
int ptpod_phy_base;
#endif


/*
 * iterate over list of detectors
 * @det:	the detector * to use as a loop cursor.
 */
#define for_each_det(det) for (det = ptp_detectors; det < (ptp_detectors + ARRAY_SIZE(ptp_detectors)); det++)

/*
 * iterate over list of detectors and its controller
 * @det:	the detector * to use as a loop cursor.
 * @ctrl:	the ptp_ctrl * to use as ctrl pointer of current det.
 */
#define for_each_det_ctrl(det, ctrl)				\
	for (det = ptp_detectors,				\
	     ctrl = id_to_ptp_ctrl(det->ctrl_id);		\
	     det < (ptp_detectors + ARRAY_SIZE(ptp_detectors)); \
	     det++,						\
	     ctrl = id_to_ptp_ctrl(det->ctrl_id))

/*
 * iterate over list of controllers
 * @pos:	the ptp_ctrl * to use as a loop cursor.
 */
#define for_each_ctrl(ctrl) for (ctrl = ptp_ctrls; ctrl < (ptp_ctrls + ARRAY_SIZE(ptp_ctrls)); ctrl++)

/*
 * Given a ptp_det * in ptp_detectors. Return the id.
 * @det:	pointer to a ptp_det in ptp_detectors
 */
#define det_to_id(det)	((det) - &ptp_detectors[0])

/*
 * Given a ptp_ctrl * in ptp_ctrls. Return the id.
 * @det:	pointer to a ptp_ctrl in ptp_ctrls
 */
#define ctrl_to_id(ctrl)	((ctrl) - &ptp_ctrls[0])

/*
 * Check if a detector has a feature
 * @det:	pointer to a ptp_det to be check
 * @feature:	enum ptp_features to be checked
 */
#define HAS_FEATURE(det, feature)	((det)->features & feature)

#define PERCENT(numerator, denominator)	\
	(unsigned char)(((numerator) * 100 + (denominator) - 1) / (denominator))

typedef enum {
	PTP_PHASE_INIT01 = 0,
	PTP_PHASE_INIT02,
	PTP_PHASE_MON,

	NR_PTP_PHASE,
} ptp_phase;

enum {
	PTP_VOLT_NONE = 0,
	PTP_VOLT_UPDATE = BIT(0),
	PTP_VOLT_RESTORE = BIT(1),
};

struct ptp_ctrl {
	const char *name;
	ptp_det_id det_id;
	/* struct completion init_done; */
	/* atomic_t in_init; */
	/* for voltage setting thread */
	wait_queue_head_t wq;
	int volt_update;
	struct task_struct *thread;
};

struct ptp_det_ops {
	/* interface to PTP-OD */
	void (*enable)(struct ptp_det *det, int reason);
	void (*disable)(struct ptp_det *det, int reason);
	void (*disable_locked)(struct ptp_det *det, int reason);
	void (*switch_bank)(struct ptp_det *det);

	int (*init01)(struct ptp_det *det);
	int (*init02)(struct ptp_det *det);
	int (*mon_mode)(struct ptp_det *det);

	int (*get_status)(struct ptp_det *det);
	void (*dump_status)(struct ptp_det *det);

	void (*set_phase)(struct ptp_det *det, ptp_phase phase);

	/* interface to thermal */
	int (*get_temp)(struct ptp_det *det);

	/* interface to DVFS */
	int (*get_volt)(struct ptp_det *det);
	int (*set_volt)(struct ptp_det *det);
	void (*restore_default_volt)(struct ptp_det *det);
	void (*get_freq_table)(struct ptp_det *det);
};

enum ptp_features {
	FEA_INIT01 = BIT(PTP_PHASE_INIT01),
	FEA_INIT02 = BIT(PTP_PHASE_INIT02),
	FEA_MON = BIT(PTP_PHASE_MON),
};

struct ptp_det {
	const char *name;
	struct ptp_det_ops *ops;
	int status;
	int features;
	ptp_ctrl_id ctrl_id;

	/* devinfo */
	unsigned int PTPINITEN;
	unsigned int PTPMONEN;
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

	unsigned int DCVOFFSETIN;
	unsigned int AGEVOFFSETIN;

	/* for debug */
	unsigned int dcvalues[NR_PTP_PHASE];

	unsigned int ptp_freqpct30[NR_PTP_PHASE];
	unsigned int ptp_26c[NR_PTP_PHASE];
	unsigned int ptp_vop30[NR_PTP_PHASE];
	unsigned int ptp_ptpen[NR_PTP_PHASE];
#if DUMP_DATA_TO_DE
	unsigned int reg_dump_data[ARRAY_SIZE(reg_dump_addr_off)][NR_PTP_PHASE];
#endif
	/* slope */
	unsigned int MTS;
	unsigned int BTS;

	/* dvfs */
	unsigned int num_freq_tbl;
	unsigned int max_freq_khz;
	unsigned char freq_tbl[NR_FREQ];

	unsigned int volt_tbl[NR_FREQ];
	unsigned int volt_tbl_init2[NR_FREQ];
	unsigned int volt_tbl_pmic[NR_FREQ];
	unsigned int volt_tbl_bin[NR_FREQ];
	int volt_offset;

	int disabled;
};


struct ptp_devinfo {
	/* M_HW_RES0 10206180 */
	unsigned int CPU_BDES:8;
	unsigned int CPU_MDES:8;
	unsigned int CPU_DCBDET:8;
	unsigned int CPU_DCMDET:8;
	/* M_HW_RES1 10206184 */
	unsigned int GPU_MTDES:8;
	unsigned int GPU_AGEDELTA:8;
	unsigned int CPU_MTDES:8;
	unsigned int CPU_AGEDELTA:8;
	/* M_HW_RES2 10206188 */
#ifdef CONFIG_ARCH_MT6735
	unsigned int SOC_VOLTBIN:2;
	unsigned int LTE_VOLTBIN:2;
	unsigned int GPU_BDES:4;
#else
#ifdef CONFIG_ARCH_MT6753
	unsigned int GPU_BDES:8;
#else
	unsigned int SOC_VOLTBIN:2;
	unsigned int LTE_VOLTBIN:2;
	unsigned int SOC_VOLTBIN_550:2;
	unsigned int GPU_BDES:2;
#endif
#endif
	unsigned int GPU_MDES:8;
	unsigned int GPU_DCBDET:8;
	unsigned int GPU_DCMDET:8;
	/* M_HW_RES3 1020618C */
	unsigned int M_HW_RES3:32;
	/* M_HW_RES4 10206190 */
#ifdef CONFIG_ARCH_MT6753
	unsigned int LTE_VOLTBIN:2;
	unsigned int LTE_BDES:6;
#else
	unsigned int LTE_BDES:8;
#endif
	unsigned int LTE_MDES:8;
	unsigned int LTE_DCBDET:8;
	unsigned int LTE_DCMDET:8;
	/* M_HW_RES5 10206194 */
	unsigned int PTPINITEN:1;
	unsigned int PTPMONEN:1;
	unsigned int Bodybias:1;
	unsigned int PTPOD_T:1;
	unsigned int EPS:1;
	unsigned int M_HW_RES5_OTHERS:11;
	unsigned int LTE_MTDES:8;
	unsigned int LTE_AGEDELTA:8;
	/* M_HW_RES6 10206270 */
	unsigned int LotID:32;
	/* M_HW_RES7 102061B0 */
	unsigned int WaferID:32;
};

/*
 *Local variable definition
*/
static int ptp_probe(struct platform_device *pdev);
static int ptp_suspend(struct platform_device *pdev, pm_message_t state);
static int ptp_resume(struct platform_device *pdev);

/*
 * lock
 */
static DEFINE_SPINLOCK(ptp_spinlock);

/*
 * PTP controllers
 */
struct ptp_ctrl ptp_ctrls[NR_PTP_CTRL] = {
	[PTP_CTRL_CPU] = {
			  .name = __stringify(PTP_CTRL_CPU),
			  .det_id = PTP_DET_CPU,
			  },
};

/*
 * PTP detectors
 */
static void base_ops_enable(struct ptp_det *det, int reason);
static void base_ops_disable(struct ptp_det *det, int reason);
static void base_ops_disable_locked(struct ptp_det *det, int reason);
static void base_ops_switch_bank(struct ptp_det *det);

static int base_ops_init01(struct ptp_det *det);
static int base_ops_init02(struct ptp_det *det);
static int base_ops_mon_mode(struct ptp_det *det);

static int base_ops_get_status(struct ptp_det *det);
static void base_ops_dump_status(struct ptp_det *det);

static void base_ops_set_phase(struct ptp_det *det, ptp_phase phase);
static int base_ops_get_temp(struct ptp_det *det);
static int base_ops_get_volt(struct ptp_det *det);
static int base_ops_set_volt(struct ptp_det *det);
static void base_ops_restore_default_volt(struct ptp_det *det);
static void base_ops_get_freq_table(struct ptp_det *det);

static int get_volt_cpu(struct ptp_det *det);
static int set_volt_cpu(struct ptp_det *det);
static void restore_default_volt_cpu(struct ptp_det *det);
static void get_freq_table_cpu(struct ptp_det *det);

#define BASE_OP(fn)	.fn = base_ops_ ## fn
static struct ptp_det_ops ptp_det_base_ops = {
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

static struct ptp_det_ops cpu_det_ops = {
	.get_volt = get_volt_cpu,
	.set_volt = set_volt_cpu,
	.restore_default_volt = restore_default_volt_cpu,
	.get_freq_table = get_freq_table_cpu,
};

static struct ptp_det ptp_detectors[NR_PTP_DET] = {
	[PTP_DET_CPU] = {
			 .name = __stringify(PTP_DET_CPU),
			 .ops = &cpu_det_ops,
			 .ctrl_id = PTP_CTRL_CPU,
			 .features = FEA_INIT01 | FEA_INIT02 | FEA_MON,
#ifdef CONFIG_ARCH_MT6735
			 .max_freq_khz = 1300000,
#else
#ifdef CONFIG_ARCH_MT6753
			 .max_freq_khz = 1495000,
#else
			 .max_freq_khz = 1000000,
#endif
#endif
			 .VBOOT = PTP_VOLT_TO_PMIC_VAL(112500),
			 },
};

static struct ptp_devinfo ptp_devinfo;

static unsigned int ptp_level;	/* debug info */
unsigned int stress_result = 1;	/* ATE stress */

/*
 * timer for log
 */
static struct hrtimer ptp_log_timer;


static struct ptp_det *id_to_ptp_det(ptp_det_id id)
{
	if (likely(id < NR_PTP_DET))
		return &ptp_detectors[id];
	else
		return NULL;
}

static struct ptp_ctrl *id_to_ptp_ctrl(ptp_ctrl_id id)
{
	if (likely(id < NR_PTP_CTRL))
		return &ptp_ctrls[id];
	else
		return NULL;
}

static void base_ops_enable(struct ptp_det *det, int reason)
{
	FUNC_ENTER(FUNC_LV_HELP);
	det->disabled &= ~reason;
	FUNC_EXIT(FUNC_LV_HELP);
}

static void base_ops_switch_bank(struct ptp_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
	ptp_write_field(PTP_PTPCORESEL, 2:0, det->ctrl_id);
	FUNC_EXIT(FUNC_LV_HELP);
}

static void base_ops_disable_locked(struct ptp_det *det, int reason)
{
	FUNC_ENTER(FUNC_LV_HELP);

	/* disable PTP */
	ptp_write(PTP_PTPEN, 0x0);

	/* Clear PTP interrupt PTPINTSTS */
	ptp_write(PTP_PTPINTSTS, 0x00ffffff);

	switch (reason) {
	case BY_MON_ERROR:
		/* set init2 value to DVFS table (PMIC) */
		memcpy(det->volt_tbl, det->volt_tbl_init2, sizeof(det->volt_tbl_init2));
		ptp_set_ptp_volt(det);
		break;

	case BY_INIT_ERROR:
	case BY_PROCFS:
	default:
		/* restore default DVFS table (PMIC) */
		ptp_restore_ptp_volt(det);
		break;
	}

	ptp_notice("Disable PTP-OD[%s] done.\n", det->name);
	det->disabled |= reason;

	FUNC_EXIT(FUNC_LV_HELP);
}

static void base_ops_disable(struct ptp_det *det, int reason)
{
	unsigned long flags;

	FUNC_ENTER(FUNC_LV_HELP);

	mt_ptp_lock(&flags);
	det->ops->switch_bank(det);
	det->ops->disable_locked(det, reason);
	mt_ptp_unlock(&flags);

	FUNC_EXIT(FUNC_LV_HELP);
}

static int base_ops_init01(struct ptp_det *det)
{
	/* struct ptp_ctrl *ctrl = id_to_ptp_ctrl(det->ctrl_id); */

	FUNC_ENTER(FUNC_LV_HELP);

	if (unlikely(!HAS_FEATURE(det, FEA_INIT01))) {
		ptp_notice("det %s has no INIT01\n", det->name);
		FUNC_EXIT(FUNC_LV_HELP);
		return -1;
	}

	if (det->disabled & BY_PROCFS) {
		ptp_notice("[%s] Disabled by PROCFS\n", __func__);
		FUNC_EXIT(FUNC_LV_HELP);
		return -2;
	}

	ptp_notice("%s(%s) start (ptp_level = 0x%08X).\n", __func__, det->name, ptp_level);
	/* atomic_inc(&ctrl->in_init); */
	/* ptp_init01_prepare(det); */
	/* det->ops->dump_status(det); */
	det->ops->set_phase(det, PTP_PHASE_INIT01);

	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

static int base_ops_init02(struct ptp_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);

	if (unlikely(!HAS_FEATURE(det, FEA_INIT02))) {
		ptp_notice("det %s has no INIT02\n", det->name);
		FUNC_EXIT(FUNC_LV_HELP);
		return -1;
	}

	if (det->disabled & BY_PROCFS) {
		ptp_notice("[%s] Disabled by PROCFS\n", __func__);
		FUNC_EXIT(FUNC_LV_HELP);
		return -2;
	}

	/* ptp_notice("%s(%s) start (ptp_level = 0x%08X).\n", __func__, det->name, ptp_level);
	ptp_notice("DCVOFFSETIN = 0x%08X\n", det->DCVOFFSETIN);
	ptp_notice("AGEVOFFSETIN = 0x%08X\n", det->AGEVOFFSETIN); */
	/* det->ops->dump_status(det); */
	det->ops->set_phase(det, PTP_PHASE_INIT02);

	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

static int base_ops_mon_mode(struct ptp_det *det)
{
	struct TS_PTPOD ts_info;
	thermal_bank_name ts_bank;

	FUNC_ENTER(FUNC_LV_HELP);

	if (!HAS_FEATURE(det, FEA_MON)) {
		ptp_notice("det %s has no MON mode\n", det->name);
		FUNC_EXIT(FUNC_LV_HELP);
		return -1;
	}

	if (det->disabled & BY_PROCFS) {
		ptp_notice("[%s] Disabled by PROCFS\n", __func__);
		FUNC_EXIT(FUNC_LV_HELP);
		return -2;
	}

	/* ptp_notice("%s(%s) start (ptp_level = 0x%08X).\n", __func__, det->name, ptp_level); */

	ts_bank = det->ctrl_id;
	get_thermal_slope_intercept(&ts_info, ts_bank);
	det->MTS = ts_info.ts_MTS;
	det->BTS = ts_info.ts_BTS;


	if ((det->PTPINITEN == 0x0) || (det->PTPMONEN == 0x0)) {
		ptp_notice("PTPINITEN = 0x%08X, PTPMONEN = 0x%08X\n", det->PTPINITEN,
			   det->PTPMONEN);
		FUNC_EXIT(FUNC_LV_HELP);
		return 1;
	}

	det->ops->set_phase(det, PTP_PHASE_MON);

	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

static int base_ops_get_status(struct ptp_det *det)
{
	int status;
	unsigned long flags;

	FUNC_ENTER(FUNC_LV_HELP);

	mt_ptp_lock(&flags);
	det->ops->switch_bank(det);
	status = (ptp_read(PTP_PTPEN) != 0) ? 1 : 0;
	mt_ptp_unlock(&flags);

	FUNC_EXIT(FUNC_LV_HELP);

	return status;
}

static void base_ops_dump_status(struct ptp_det *det)
{
	int i;

	FUNC_ENTER(FUNC_LV_HELP);

	ptp_isr_info("[%s]\n", det->name);

	ptp_isr_info("PTPINITEN = 0x%08X\n", det->PTPINITEN);
	ptp_isr_info("PTPMONEN = 0x%08X\n", det->PTPMONEN);
	ptp_isr_info("MDES = 0x%08X\n", det->MDES);
	ptp_isr_info("BDES = 0x%08X\n", det->BDES);
	ptp_isr_info("DCMDET = 0x%08X\n", det->DCMDET);

	ptp_isr_info("DCCONFIG = 0x%08X\n", det->DCCONFIG);
	ptp_isr_info("DCBDET = 0x%08X\n", det->DCBDET);

	ptp_isr_info("AGECONFIG = 0x%08X\n", det->AGECONFIG);
	ptp_isr_info("AGEM = 0x%08X\n", det->AGEM);

	ptp_isr_info("AGEDELTA = 0x%08X\n", det->AGEDELTA);
	ptp_isr_info("DVTFIXED = 0x%08X\n", det->DVTFIXED);
	ptp_isr_info("MTDES = 0x%08X\n", det->MTDES);
	ptp_isr_info("VCO = 0x%08X\n", det->VCO);

	ptp_isr_info("DETWINDOW = 0x%08X\n", det->DETWINDOW);
	ptp_isr_info("VMAX = 0x%08X\n", det->VMAX);
	ptp_isr_info("VMIN = 0x%08X\n", det->VMIN);
	ptp_isr_info("DTHI = 0x%08X\n", det->DTHI);
	ptp_isr_info("DTLO = 0x%08X\n", det->DTLO);
	ptp_isr_info("VBOOT = 0x%08X\n", det->VBOOT);
	ptp_isr_info("DETMAX = 0x%08X\n", det->DETMAX);

	ptp_isr_info("DCVOFFSETIN = 0x%08X\n", det->DCVOFFSETIN);
	ptp_isr_info("AGEVOFFSETIN = 0x%08X\n", det->AGEVOFFSETIN);

	ptp_isr_info("MTS = 0x%08X\n", det->MTS);
	ptp_isr_info("BTS = 0x%08X\n", det->BTS);

	ptp_isr_info("num_freq_tbl = %d\n", det->num_freq_tbl);

	for (i = 0; i < det->num_freq_tbl; i++)
		ptp_isr_info("freq_tbl[%d] = %d\n", i, det->freq_tbl[i]);

	for (i = 0; i < det->num_freq_tbl; i++)
		ptp_isr_info("volt_tbl[%d] = %d\n", i, det->volt_tbl[i]);

	for (i = 0; i < det->num_freq_tbl; i++)
		ptp_isr_info("volt_tbl_init2[%d] = %d\n", i, det->volt_tbl_init2[i]);

	for (i = 0; i < det->num_freq_tbl; i++)
		ptp_isr_info("volt_tbl_pmic[%d] = %d\n", i, det->volt_tbl_pmic[i]);

	FUNC_EXIT(FUNC_LV_HELP);
}

static void base_ops_set_phase(struct ptp_det *det, ptp_phase phase)
{
	unsigned int i, filter, val;

	FUNC_ENTER(FUNC_LV_HELP);

	det->ops->switch_bank(det);
	/* config PTP register */
	ptp_write(PTP_DESCHAR, ((det->BDES << 8) & 0xff00) | (det->MDES & 0xff));
	ptp_write(PTP_TEMPCHAR,
		  (((det->VCO << 16) & 0xff0000) |
		   ((det->MTDES << 8) & 0xff00) | (det->DVTFIXED & 0xff)));
	ptp_write(PTP_DETCHAR, ((det->DCBDET << 8) & 0xff00) | (det->DCMDET & 0xff));
	ptp_write(PTP_AGECHAR, ((det->AGEDELTA << 8) & 0xff00) | (det->AGEM & 0xff));
	ptp_write(PTP_DCCONFIG, det->DCCONFIG);
	ptp_write(PTP_AGECONFIG, det->AGECONFIG);

	if (PTP_PHASE_MON == phase)
		ptp_write(PTP_TSCALCS, ((det->BTS << 12) & 0xfff000) | (det->MTS & 0xfff));

	if (det->AGEM == 0x0)
		ptp_write(PTP_RUNCONFIG, 0x80000000);
	else {
		val = 0x0;

		for (i = 0; i < 24; i += 2) {
			filter = 0x3 << i;

			if (((det->AGECONFIG) & filter) == 0x0)
				val |= (0x1 << i);
			else
				val |= ((det->AGECONFIG) & filter);
		}

		ptp_write(PTP_RUNCONFIG, val);
	}

	ptp_write(PTP_FREQPCT30,
		  ((det->freq_tbl[3] << 24) & 0xff000000) |
		  ((det->freq_tbl[2] << 16) & 0xff0000) |
		  ((det->freq_tbl[1] << 8) & 0xff00) | (det->freq_tbl[0] & 0xff));
	ptp_write(PTP_FREQPCT74,
		  ((det->freq_tbl[7] << 24) & 0xff000000) |
		  ((det->freq_tbl[6] << 16) & 0xff0000) |
		  ((det->freq_tbl[5] << 8) & 0xff00) | ((det->freq_tbl[4]) & 0xff));
	ptp_write(PTP_LIMITVALS,
		  ((det->VMAX << 24) & 0xff000000) |
		  ((det->VMIN << 16) & 0xff0000) |
		  ((det->DTHI << 8) & 0xff00) | (det->DTLO & 0xff));
	ptp_write(PTP_VBOOT, (((det->VBOOT) & 0xff)));
	ptp_write(PTP_DETWINDOW, (((det->DETWINDOW) & 0xffff)));
	ptp_write(PTP_PTPCONFIG, (((det->DETMAX) & 0xffff)));

	/* clear all pending PTP interrupt & config PTPINTEN */
	ptp_write(PTP_PTPINTSTS, 0xffffffff);

	switch (phase) {
	case PTP_PHASE_INIT01:
		ptp_write(PTP_PTPINTEN, 0x00005f01);
		/* enable PTP INIT measurement */
		ptp_write(PTP_PTPEN, 0x00000001);
		break;

	case PTP_PHASE_INIT02:
		ptp_write(PTP_PTPINTEN, 0x00005f01);
		ptp_write(PTP_INIT2VALS,
			  ((det->AGEVOFFSETIN << 16) & 0xffff0000) | (det->DCVOFFSETIN & 0xffff));
		/* enable PTP INIT measurement */
		ptp_write(PTP_PTPEN, 0x00000005);
		break;

	case PTP_PHASE_MON:
		ptp_write(PTP_PTPINTEN, 0x00FF0000);
		/* enable PTP monitor mode */
		ptp_write(PTP_PTPEN, 0x00000002);
		break;

	default:
		BUG();
		break;
	}

	FUNC_EXIT(FUNC_LV_HELP);
}

static int base_ops_get_temp(struct ptp_det *det)
{
	thermal_bank_name ts_bank;

	FUNC_ENTER(FUNC_LV_HELP);

	ts_bank = THERMAL_BANK0;

	FUNC_EXIT(FUNC_LV_HELP);

	return tscpu_get_temp_by_bank(ts_bank);
}

static int base_ops_get_volt(struct ptp_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
	ptp_warning("[%s] default func\n", __func__);
	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

static int base_ops_set_volt(struct ptp_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
	ptp_warning("[%s] default func\n", __func__);
	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

static void base_ops_restore_default_volt(struct ptp_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
	ptp_warning("[%s] default func\n", __func__);
	FUNC_EXIT(FUNC_LV_HELP);
}

static void base_ops_get_freq_table(struct ptp_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);

	det->freq_tbl[0] = 100;
	det->num_freq_tbl = 1;

	FUNC_EXIT(FUNC_LV_HELP);
}

/* Will return 10uV */
static int get_volt_cpu(struct ptp_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
	return mt_cpufreq_get_cur_volt(MT_CPU_DVFS_LITTLE);	/* unit mv * 100 = 10uv */
	FUNC_EXIT(FUNC_LV_HELP);
}

/* volt_tbl_pmic is convert from 10uV */
static int set_volt_cpu(struct ptp_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
	FUNC_EXIT(FUNC_LV_HELP);
	return mt_cpufreq_update_volt(MT_CPU_DVFS_LITTLE, det->volt_tbl_pmic, det->num_freq_tbl);
}

static void restore_default_volt_cpu(struct ptp_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);


	mt_cpufreq_restore_default_volt(MT_CPU_DVFS_LITTLE);

	FUNC_EXIT(FUNC_LV_HELP);
}

static void get_freq_table_cpu(struct ptp_det *det)
{
	int i;
	enum mt_cpu_dvfs_id cpu;

	FUNC_ENTER(FUNC_LV_HELP);
	cpu = MT_CPU_DVFS_LITTLE;

	/* det->max_freq_khz = mt_cpufreq_get_freq_by_idx(cpu, 0); */
	for (i = 0; i < NR_FREQ; i++) {
		det->freq_tbl[i] = PERCENT(mt_cpufreq_get_freq_by_idx(cpu, i), det->max_freq_khz);
		if (0 == det->freq_tbl[i])
			break;
	}
	det->num_freq_tbl = i;

	FUNC_EXIT(FUNC_LV_HELP);
}

void mt_ptp_lock(unsigned long *flags)
{
	spin_lock_irqsave(&ptp_spinlock, *flags);
}
EXPORT_SYMBOL(mt_ptp_lock);

void mt_ptp_unlock(unsigned long *flags)
{
	spin_unlock_irqrestore(&ptp_spinlock, *flags);
}
EXPORT_SYMBOL(mt_ptp_unlock);

/*
 * timer for log
 */
static enum hrtimer_restart ptp_log_timer_func(struct hrtimer *timer)
{
	struct ptp_det *det;

	 FUNC_ENTER(FUNC_LV_HELP);

	 for_each_det(det) {
		ptp_notice(
		"PTP_LOG: PTPOD [%s](%d) -(%d, %d, %d, %d, %d, %d, %d, %d)-(%d, %d, %d, %d, %d, %d, %d, %d)\n",
			   det->name, det->ops->get_temp(det),
			   PTP_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[0]),
			   PTP_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[1]),
			   PTP_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[2]),
			   PTP_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[3]),
			   PTP_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[4]),
			   PTP_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[5]),
			   PTP_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[6]),
			   PTP_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[7]),
			   det->freq_tbl[0],
			   det->freq_tbl[1],
			   det->freq_tbl[2],
			   det->freq_tbl[3],
			   det->freq_tbl[4], det->freq_tbl[5], det->freq_tbl[6], det->freq_tbl[7]);
	}

	hrtimer_forward_now(timer, ns_to_ktime(LOG_INTERVAL));
	FUNC_EXIT(FUNC_LV_HELP);

	return HRTIMER_RESTART;
}

/*
 * Thread for voltage setting
 */
static int ptp_volt_thread_handler(void *data)
{
	struct ptp_ctrl *ctrl = (struct ptp_ctrl *)data;
	struct ptp_det *det = id_to_ptp_det(ctrl->det_id);

	 FUNC_ENTER(FUNC_LV_HELP);

	do {
		wait_event_interruptible(ctrl->wq, ctrl->volt_update);

		if ((ctrl->volt_update & PTP_VOLT_UPDATE) && det->ops->set_volt)
			det->ops->set_volt(det);

		if ((ctrl->volt_update & PTP_VOLT_RESTORE) && det->ops->restore_default_volt)
			det->ops->restore_default_volt(det);

		ctrl->volt_update = PTP_VOLT_NONE;

	} while (!kthread_should_stop());

	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

static void inherit_base_det(struct ptp_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);

#define INIT_OP(ops, func)					\
		do {							\
			if (ops->func == NULL)				\
				ops->func = ptp_det_base_ops.func;	\
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
static void ptp_init_ctrl(struct ptp_ctrl *ctrl)
{
	FUNC_ENTER(FUNC_LV_HELP);

		init_waitqueue_head(&ctrl->wq);
		ctrl->thread = kthread_run(ptp_volt_thread_handler, ctrl, ctrl->name);

		if (IS_ERR(ctrl->thread))
			ptp_error("Create %s thread failed: %ld\n", ctrl->name,
				  PTR_ERR(ctrl->thread));

	FUNC_EXIT(FUNC_LV_HELP);
}

#define _BIT_(_bit_)                        (unsigned)(1 << (_bit_))
#define _BITMASK_(_bits_)                   (((unsigned) -1 >> (31 - ((1) ? _bits_))) & ~((1U << ((0) ? _bits_)) - 1))
#define _GET_BITS_VAL_(_bits_, _val_)       (((_val_) & (_BITMASK_(_bits_))) >> ((0) ? _bits_))

static void ptp_init_det(struct ptp_det *det, struct ptp_devinfo *devinfo)
{
	unsigned int segment_code = _GET_BITS_VAL_(31 : 25, get_devinfo_with_index(47));
	unsigned int down_grade_bit = _GET_BITS_VAL_(20 : 20, get_devinfo_with_index(24));

	ptp_det_id det_id = det_to_id(det);

	 FUNC_ENTER(FUNC_LV_HELP);
	 ptp_notice("det name=%s,det_id=%d\n", det->name, det_id);

	 inherit_base_det(det);

	/* init with devinfo */
	 det->PTPINITEN = devinfo->PTPINITEN;
	 det->PTPMONEN = devinfo->PTPMONEN;

	/* init with constant */
	 det->DETWINDOW = DETWINDOW_VAL;
	 det->VMAX = VMAX_VAL;
	 det->VMIN = VMIN_VAL;

	 det->DTHI = DTHI_VAL;
	 det->DTLO = DTLO_VAL;
	 det->DETMAX = DETMAX_VAL;

	 det->AGECONFIG = AGECONFIG_VAL;
	 det->AGEM = AGEM_VAL;
	 det->DVTFIXED = DVTFIXED_VAL;
	 det->VCO = VCO_VAL;
	 det->DCCONFIG = DCCONFIG_VAL;

	if (NULL != det->ops->get_volt) {
		det->VBOOT = PTP_VOLT_TO_PMIC_VAL(det->ops->get_volt(det));
		ptp_alert("@%s(), det->VBOOT = %d\n", __func__, det->VBOOT);
	}

	switch (det_id) {
	case PTP_DET_CPU:
		det->MDES = devinfo->CPU_MDES;
		det->BDES = devinfo->CPU_BDES;
		det->DCMDET = devinfo->CPU_DCMDET;
		det->DCBDET = devinfo->CPU_DCBDET;
		switch (segment_code) {
		case 0x4A:
		case 0x4B:
		case 0x52:
		case 0x53:
			if (down_grade_bit)
				det->volt_offset = 0x2;
			det->DVTFIXED = 0x8;
			break;
		default:
			det->DVTFIXED = 0x6;
			break;
		}
		break;

	default:
		ptp_error("[%s]: Unknown det_id %d\n", __func__, det_id);
		break;
	}

	switch (det->ctrl_id) {
	case PTP_CTRL_CPU:
		det->AGEDELTA = devinfo->CPU_AGEDELTA;
		det->MTDES = devinfo->CPU_MTDES;
		break;

	default:
		ptp_error("[%s]: Unknown ctrl_id %d\n", __func__, det->ctrl_id);
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

void __attribute__((weak)) pmic_force_vproc_pwm(unsigned int en)
{
}

static void ptp_set_ptp_volt(struct ptp_det *det)
{
#if SET_PMIC_VOLT
	int i, cur_temp, low_temp_offset;
	struct ptp_ctrl *ctrl = id_to_ptp_ctrl(det->ctrl_id);
	int tscpu_bank0_temp_is_valid = tscpu_is_temp_valid();

	 cur_temp = det->ops->get_temp(det);
	ptp_debug("ptp_set_ptp_volt(): cur_temp = %d, valid = %d\n", cur_temp, tscpu_bank0_temp_is_valid);
	if (!tscpu_bank0_temp_is_valid || cur_temp <= 33000) {
		low_temp_offset = 10;
		ctrl->volt_update |= PTP_VOLT_UPDATE;
	} else {
		low_temp_offset = 0;
		ctrl->volt_update |= PTP_VOLT_UPDATE;
	}

	for (i = 0; i < det->num_freq_tbl; i++)
		det->volt_tbl_pmic[i] =
		    clamp(det->volt_tbl[i] + det->volt_offset + low_temp_offset,
			  det->VMIN + PTPOD_PMIC_OFFSET, det->VMAX + PTPOD_PMIC_OFFSET);

	wake_up_interruptible(&ctrl->wq);
#endif

	FUNC_ENTER(FUNC_LV_HELP);
	FUNC_EXIT(FUNC_LV_HELP);
}

static void ptp_restore_ptp_volt(struct ptp_det *det)
{
#if SET_PMIC_VOLT
	struct ptp_ctrl *ctrl = id_to_ptp_ctrl(det->ctrl_id);

	ctrl->volt_update |= PTP_VOLT_RESTORE;
	wake_up_interruptible(&ctrl->wq);
#endif

	 FUNC_ENTER(FUNC_LV_HELP);
	 FUNC_EXIT(FUNC_LV_HELP);
}

static void mt_ptp_reg_dump(void)
{
	struct ptp_det *det;
	unsigned long flags;

	 FUNC_ENTER(FUNC_LV_HELP);

	 ptp_isr_info("PTP_REVISIONID	= 0x%08X\n", ptp_read(PTP_REVISIONID));
	 ptp_isr_info("PTP_TEMPMONCTL0	= 0x%08X\n", ptp_read(PTP_TEMPMONCTL0));
	 ptp_isr_info("PTP_TEMPMONCTL1	= 0x%08X\n", ptp_read(PTP_TEMPMONCTL1));
	 ptp_isr_info("PTP_TEMPMONCTL2	= 0x%08X\n", ptp_read(PTP_TEMPMONCTL2));
	 ptp_isr_info("PTP_TEMPMONINT	= 0x%08X\n", ptp_read(PTP_TEMPMONINT));
	 ptp_isr_info("PTP_TEMPMONINTSTS	= 0x%08X\n", ptp_read(PTP_TEMPMONINTSTS));
	 ptp_isr_info("PTP_TEMPMONIDET0	= 0x%08X\n", ptp_read(PTP_TEMPMONIDET0));
	 ptp_isr_info("PTP_TEMPMONIDET1	= 0x%08X\n", ptp_read(PTP_TEMPMONIDET1));
	 ptp_isr_info("PTP_TEMPMONIDET2	= 0x%08X\n", ptp_read(PTP_TEMPMONIDET2));
	 ptp_isr_info("PTP_TEMPH2NTHRE	= 0x%08X\n", ptp_read(PTP_TEMPH2NTHRE));
	 ptp_isr_info("PTP_TEMPHTHRE	= 0x%08X\n", ptp_read(PTP_TEMPHTHRE));
	 ptp_isr_info("PTP_TEMPCTHRE	= 0x%08X\n", ptp_read(PTP_TEMPCTHRE));
	 ptp_isr_info("PTP_TEMPOFFSETH	= 0x%08X\n", ptp_read(PTP_TEMPOFFSETH));
	 ptp_isr_info("PTP_TEMPOFFSETL	= 0x%08X\n", ptp_read(PTP_TEMPOFFSETL));
	 ptp_isr_info("PTP_TEMPMSRCTL0	= 0x%08X\n", ptp_read(PTP_TEMPMSRCTL0));
	 ptp_isr_info("PTP_TEMPMSRCTL1	= 0x%08X\n", ptp_read(PTP_TEMPMSRCTL1));
	 ptp_isr_info("PTP_TEMPAHBPOLL	= 0x%08X\n", ptp_read(PTP_TEMPAHBPOLL));
	 ptp_isr_info("PTP_TEMPAHBTO	= 0x%08X\n", ptp_read(PTP_TEMPAHBTO));
	 ptp_isr_info("PTP_TEMPADCPNP0	= 0x%08X\n", ptp_read(PTP_TEMPADCPNP0));
	 ptp_isr_info("PTP_TEMPADCPNP1	= 0x%08X\n", ptp_read(PTP_TEMPADCPNP1));
	 ptp_isr_info("PTP_TEMPADCPNP2	= 0x%08X\n", ptp_read(PTP_TEMPADCPNP2));
	 ptp_isr_info("PTP_TEMPADCMUX	= 0x%08X\n", ptp_read(PTP_TEMPADCMUX));
	 ptp_isr_info("PTP_TEMPADCEXT	= 0x%08X\n", ptp_read(PTP_TEMPADCEXT));
	 ptp_isr_info("PTP_TEMPADCEXT1	= 0x%08X\n", ptp_read(PTP_TEMPADCEXT1));
	 ptp_isr_info("PTP_TEMPADCEN	= 0x%08X\n", ptp_read(PTP_TEMPADCEN));
	 ptp_isr_info("PTP_TEMPPNPMUXADDR	= 0x%08X\n", ptp_read(PTP_TEMPPNPMUXADDR));
	 ptp_isr_info("PTP_TEMPADCMUXADDR	= 0x%08X\n", ptp_read(PTP_TEMPADCMUXADDR));
	 ptp_isr_info("PTP_TEMPADCEXTADDR	= 0x%08X\n", ptp_read(PTP_TEMPADCEXTADDR));
	 ptp_isr_info("PTP_TEMPADCEXT1ADDR	= 0x%08X\n", ptp_read(PTP_TEMPADCEXT1ADDR));
	 ptp_isr_info("PTP_TEMPADCENADDR	= 0x%08X\n", ptp_read(PTP_TEMPADCENADDR));
	 ptp_isr_info("PTP_TEMPADCVALIDADDR	= 0x%08X\n", ptp_read(PTP_TEMPADCVALIDADDR));
	 ptp_isr_info("PTP_TEMPADCVOLTADDR	= 0x%08X\n", ptp_read(PTP_TEMPADCVOLTADDR));
	 ptp_isr_info("PTP_TEMPRDCTRL	= 0x%08X\n", ptp_read(PTP_TEMPRDCTRL));
	 ptp_isr_info("PTP_TEMPADCVALIDMASK	= 0x%08X\n", ptp_read(PTP_TEMPADCVALIDMASK));
	 ptp_isr_info("PTP_TEMPADCVOLTAGESHIFT	= 0x%08X\n", ptp_read(PTP_TEMPADCVOLTAGESHIFT));
	 ptp_isr_info("PTP_TEMPADCWRITECTRL	= 0x%08X\n", ptp_read(PTP_TEMPADCWRITECTRL));
	 ptp_isr_info("PTP_TEMPMSR0	= 0x%08X\n", ptp_read(PTP_TEMPMSR0));
	 ptp_isr_info("PTP_TEMPMSR1	= 0x%08X\n", ptp_read(PTP_TEMPMSR1));
	 ptp_isr_info("PTP_TEMPMSR2	= 0x%08X\n", ptp_read(PTP_TEMPMSR2));
	 ptp_isr_info("PTP_TEMPIMMD0	= 0x%08X\n", ptp_read(PTP_TEMPIMMD0));
	 ptp_isr_info("PTP_TEMPIMMD1	= 0x%08X\n", ptp_read(PTP_TEMPIMMD1));
	 ptp_isr_info("PTP_TEMPIMMD2	= 0x%08X\n", ptp_read(PTP_TEMPIMMD2));
	 ptp_isr_info("PTP_TEMPMONIDET3	= 0x%08X\n", ptp_read(PTP_TEMPMONIDET3));
	 ptp_isr_info("PTP_TEMPADCPNP3	= 0x%08X\n", ptp_read(PTP_TEMPADCPNP3));
	 ptp_isr_info("PTP_TEMPMSR3	= 0x%08X\n", ptp_read(PTP_TEMPMSR3));
	 ptp_isr_info("PTP_TEMPIMMD3	= 0x%08X\n", ptp_read(PTP_TEMPIMMD3));
	 ptp_isr_info("PTP_TEMPPROTCTL	= 0x%08X\n", ptp_read(PTP_TEMPPROTCTL));
	 ptp_isr_info("PTP_TEMPPROTTA	= 0x%08X\n", ptp_read(PTP_TEMPPROTTA));
	 ptp_isr_info("PTP_TEMPPROTTB	= 0x%08X\n", ptp_read(PTP_TEMPPROTTB));
	 ptp_isr_info("PTP_TEMPPROTTC	= 0x%08X\n", ptp_read(PTP_TEMPPROTTC));
	 ptp_isr_info("PTP_TEMPSPARE0	= 0x%08X\n", ptp_read(PTP_TEMPSPARE0));
	 ptp_isr_info("PTP_TEMPSPARE1	= 0x%08X\n", ptp_read(PTP_TEMPSPARE1));
	 ptp_isr_info("PTP_TEMPSPARE2	= 0x%08X\n", ptp_read(PTP_TEMPSPARE2));
	 ptp_isr_info("PTP_TEMPSPARE3	= 0x%08X\n", ptp_read(PTP_TEMPSPARE3));

	 for_each_det(det) {
		mt_ptp_lock(&flags);
		det->ops->switch_bank(det);

		ptp_isr_info("PTP_DESCHAR[%s]	= 0x%08X\n", det->name, ptp_read(PTP_DESCHAR));
		ptp_isr_info("PTP_TEMPCHAR[%s]	= 0x%08X\n", det->name, ptp_read(PTP_TEMPCHAR));
		ptp_isr_info("PTP_DETCHAR[%s]	= 0x%08X\n", det->name, ptp_read(PTP_DETCHAR));
		ptp_isr_info("PTP_AGECHAR[%s]	= 0x%08X\n", det->name, ptp_read(PTP_AGECHAR));
		ptp_isr_info("PTP_DCCONFIG[%s]	= 0x%08X\n", det->name, ptp_read(PTP_DCCONFIG));
		ptp_isr_info("PTP_AGECONFIG[%s]	= 0x%08X\n", det->name, ptp_read(PTP_AGECONFIG));
		ptp_isr_info("PTP_FREQPCT30[%s]	= 0x%08X\n", det->name, ptp_read(PTP_FREQPCT30));
		ptp_isr_info("PTP_FREQPCT74[%s]	= 0x%08X\n", det->name, ptp_read(PTP_FREQPCT74));
		ptp_isr_info("PTP_LIMITVALS[%s]	= 0x%08X\n", det->name, ptp_read(PTP_LIMITVALS));
		ptp_isr_info("PTP_VBOOT[%s]	= 0x%08X\n", det->name, ptp_read(PTP_VBOOT));
		ptp_isr_info("PTP_DETWINDOW[%s]	= 0x%08X\n", det->name, ptp_read(PTP_DETWINDOW));
		ptp_isr_info("PTP_PTPCONFIG[%s]	= 0x%08X\n", det->name, ptp_read(PTP_PTPCONFIG));
		ptp_isr_info("PTP_TSCALCS[%s]	= 0x%08X\n", det->name, ptp_read(PTP_TSCALCS));
		ptp_isr_info("PTP_RUNCONFIG[%s]	= 0x%08X\n", det->name, ptp_read(PTP_RUNCONFIG));
		ptp_isr_info("PTP_PTPEN[%s]	= 0x%08X\n", det->name, ptp_read(PTP_PTPEN));
		ptp_isr_info("PTP_INIT2VALS[%s]	= 0x%08X\n", det->name, ptp_read(PTP_INIT2VALS));
		ptp_isr_info("PTP_DCVALUES[%s]	= 0x%08X\n", det->name, ptp_read(PTP_DCVALUES));
		ptp_isr_info("PTP_AGEVALUES[%s]	= 0x%08X\n", det->name, ptp_read(PTP_AGEVALUES));
		ptp_isr_info("PTP_VOP30[%s]	= 0x%08X\n", det->name, ptp_read(PTP_VOP30));
		ptp_isr_info("PTP_VOP74[%s]	= 0x%08X\n", det->name, ptp_read(PTP_VOP74));
		ptp_isr_info("PTP_TEMP[%s]	= 0x%08X\n", det->name, ptp_read(PTP_TEMP));
		ptp_isr_info("PTP_PTPINTSTS[%s]	= 0x%08X\n", det->name, ptp_read(PTP_PTPINTSTS));
		ptp_isr_info("PTP_PTPINTSTSRAW[%s]	= 0x%08X\n", det->name,
			     ptp_read(PTP_PTPINTSTSRAW));
		ptp_isr_info("PTP_PTPINTEN[%s]	= 0x%08X\n", det->name, ptp_read(PTP_PTPINTEN));
		ptp_isr_info("PTP_SMSTATE0[%s]	= 0x%08X\n", det->name, ptp_read(PTP_SMSTATE0));
		ptp_isr_info("PTP_SMSTATE1[%s]	= 0x%08X\n", det->name, ptp_read(PTP_SMSTATE1));

		mt_ptp_unlock(&flags);
	}

	ptp_isr_info("PTP_PTPCORESEL	= 0x%08X\n", ptp_read(PTP_PTPCORESEL));
	ptp_isr_info("PTP_THERMINTST	= 0x%08X\n", ptp_read(PTP_THERMINTST));
	ptp_isr_info("PTP_PTPODINTST	= 0x%08X\n", ptp_read(PTP_PTPODINTST));
	ptp_isr_info("PTP_THSTAGE0ST	= 0x%08X\n", ptp_read(PTP_THSTAGE0ST));
	ptp_isr_info("PTP_THSTAGE1ST	= 0x%08X\n", ptp_read(PTP_THSTAGE1ST));
	ptp_isr_info("PTP_THSTAGE2ST	= 0x%08X\n", ptp_read(PTP_THSTAGE2ST));
	ptp_isr_info("PTP_THAHBST0	= 0x%08X\n", ptp_read(PTP_THAHBST0));
	ptp_isr_info("PTP_THAHBST1	= 0x%08X\n", ptp_read(PTP_THAHBST1));
	ptp_isr_info("PTP_PTPSPARE0	= 0x%08X\n", ptp_read(PTP_PTPSPARE0));
	ptp_isr_info("PTP_PTPSPARE1	= 0x%08X\n", ptp_read(PTP_PTPSPARE1));
	ptp_isr_info("PTP_PTPSPARE2	= 0x%08X\n", ptp_read(PTP_PTPSPARE2));
	ptp_isr_info("PTP_PTPSPARE3	= 0x%08X\n", ptp_read(PTP_PTPSPARE3));
	ptp_isr_info("PTP_THSLPEVEB	= 0x%08X\n", ptp_read(PTP_THSLPEVEB));

	FUNC_EXIT(FUNC_LV_HELP);
}

static inline void handle_init01_isr(struct ptp_det *det)
{
	FUNC_ENTER(FUNC_LV_LOCAL);

	ptp_isr_info("@ %s(%s)\n", __func__, det->name);

	det->dcvalues[PTP_PHASE_INIT01] = ptp_read(PTP_DCVALUES);
	det->ptp_freqpct30[PTP_PHASE_INIT01] = ptp_read(PTP_FREQPCT30);
	det->ptp_26c[PTP_PHASE_INIT01] = ptp_read(PTP_PTPINTEN + 0x10);
	det->ptp_vop30[PTP_PHASE_INIT01] = ptp_read(PTP_VOP30);
	det->ptp_ptpen[PTP_PHASE_INIT01] = ptp_read(PTP_PTPEN);
#if DUMP_DATA_TO_DE
	{
		int i;

		for (i = 0; i < ARRAY_SIZE(reg_dump_addr_off); i++) {
			det->reg_dump_data[i][PTP_PHASE_INIT01] =
			    ptp_read(PTP_BASEADDR + reg_dump_addr_off[i]);
			ptp_isr_info("0x%lx === 0x%08x\n",
				     (unsigned long)PTP_BASEADDR + reg_dump_addr_off[i],
				     det->reg_dump_data[i][PTP_PHASE_INIT01]
			    );
	} }
#endif
	det->DCVOFFSETIN = ~(ptp_read(PTP_DCVALUES) & 0xffff) + 1;
	det->AGEVOFFSETIN = ptp_read(PTP_AGEVALUES) & 0xffff;

	/*
	 * Set PTPEN.PTPINITEN/PTPEN.PTPINIT2EN = 0x0 &
	 * Clear PTP INIT interrupt PTPINTSTS = 0x00000001
	 */
	ptp_write(PTP_PTPEN, 0x0);
	ptp_write(PTP_PTPINTSTS, 0x1);
	/* ptp_init01_finish(det); */
	det->ops->init02(det);

	FUNC_EXIT(FUNC_LV_LOCAL);
}

static inline void handle_init02_isr(struct ptp_det *det)
{
	unsigned int temp;
	int i;
	/* struct ptp_ctrl *ctrl = id_to_ptp_ctrl(det->ctrl_id); */

	 FUNC_ENTER(FUNC_LV_LOCAL);

	 ptp_isr_info("@ %s(%s)\n", __func__, det->name);
	 det->dcvalues[PTP_PHASE_INIT02] = ptp_read(PTP_DCVALUES);
	 det->ptp_freqpct30[PTP_PHASE_INIT02] = ptp_read(PTP_FREQPCT30);
	 det->ptp_26c[PTP_PHASE_INIT02] = ptp_read(PTP_PTPINTEN + 0x10);
	 det->ptp_vop30[PTP_PHASE_INIT02] = ptp_read(PTP_VOP30);
	 det->ptp_ptpen[PTP_PHASE_INIT02] = ptp_read(PTP_PTPEN);
#if DUMP_DATA_TO_DE
	{
		int i;

		for (i = 0; i < ARRAY_SIZE(reg_dump_addr_off); i++) {
			det->reg_dump_data[i][PTP_PHASE_INIT02] =
			    ptp_read(PTP_BASEADDR + reg_dump_addr_off[i]);
			ptp_isr_info("0x%lx === 0x%08x\n",
				     (unsigned long)PTP_BASEADDR + reg_dump_addr_off[i],
				     det->reg_dump_data[i][PTP_PHASE_INIT02]
			    );
	} }
#endif
	temp = ptp_read(PTP_VOP30);
	/* PTP_VOP30=>pmic value */
	det->volt_tbl[0] = (temp & 0xff) + PTPOD_PMIC_OFFSET;
	det->volt_tbl[1] = ((temp >> 8) & 0xff) + PTPOD_PMIC_OFFSET;
	det->volt_tbl[2] = ((temp >> 16) & 0xff) + PTPOD_PMIC_OFFSET;
	det->volt_tbl[3] = ((temp >> 24) & 0xff) + PTPOD_PMIC_OFFSET;

	temp = ptp_read(PTP_VOP74);
	/* PTP_VOP74=>pmic value */
	det->volt_tbl[4] = (temp & 0xff) + PTPOD_PMIC_OFFSET;
	det->volt_tbl[5] = ((temp >> 8) & 0xff) + PTPOD_PMIC_OFFSET;
	det->volt_tbl[6] = ((temp >> 16) & 0xff) + PTPOD_PMIC_OFFSET;
	det->volt_tbl[7] = ((temp >> 24) & 0xff) + PTPOD_PMIC_OFFSET;

	/* backup to volt_tbl_init2 */
	memcpy(det->volt_tbl_init2, det->volt_tbl, sizeof(det->volt_tbl_init2));

	for (i = 0; i < NR_FREQ; i++) {
		ptp_isr_info("ptp_detectors[%s].volt_tbl[%d] = 0x%08X (%d)\n",
			     det->name, i, det->volt_tbl[i],
			     PTP_PMIC_VAL_TO_VOLT(det->volt_tbl[i]));
	}
	ptp_isr_info("ptp_level = 0x%08X\n", ptp_level);

	ptp_set_ptp_volt(det);

	if (stress_result == 1)
		stress_result = 0;

	/*
	 * Set PTPEN.PTPINITEN/PTPEN.PTPINIT2EN = 0x0 &
	 * Clear PTP INIT interrupt PTPINTSTS = 0x00000001
	 */
	ptp_write(PTP_PTPEN, 0x0);
	ptp_write(PTP_PTPINTSTS, 0x1);

	/* atomic_dec(&ctrl->in_init); */
	/* complete(&ctrl->init_done); */
	det->ops->mon_mode(det);

	FUNC_EXIT(FUNC_LV_LOCAL);
}

static inline void handle_init_err_isr(struct ptp_det *det)
{
	FUNC_ENTER(FUNC_LV_LOCAL);
	ptp_isr_info("====================================================\n");
	ptp_isr_info("PTP init err: PTPEN(%p) = 0x%08X, PTPINTSTS(%p) = 0x%08X\n",
		     PTP_PTPEN, ptp_read(PTP_PTPEN), PTP_PTPINTSTS, ptp_read(PTP_PTPINTSTS));
	ptp_isr_info("PTP_SMSTATE0 (%p) = 0x%08X\n", PTP_SMSTATE0, ptp_read(PTP_SMSTATE0));
	ptp_isr_info("PTP_SMSTATE1 (%p) = 0x%08X\n", PTP_SMSTATE1, ptp_read(PTP_SMSTATE1));
	ptp_isr_info("====================================================\n");

	det->ops->disable_locked(det, BY_INIT_ERROR);

	FUNC_EXIT(FUNC_LV_LOCAL);
}

static inline void handle_mon_mode_isr(struct ptp_det *det)
{
	unsigned int temp;
	int i;

	 FUNC_ENTER(FUNC_LV_LOCAL);

	 ptp_isr_info("@ %s(%s)\n", __func__, det->name);
	 ptp_isr_info("cpu_temp=%d\n", tscpu_get_temp_by_bank(THERMAL_BANK0));

	det->dcvalues[PTP_PHASE_MON] = ptp_read(PTP_DCVALUES);
	det->ptp_freqpct30[PTP_PHASE_MON] = ptp_read(PTP_FREQPCT30);
	det->ptp_26c[PTP_PHASE_MON] = ptp_read(PTP_PTPINTEN + 0x10);
	det->ptp_vop30[PTP_PHASE_MON] = ptp_read(PTP_VOP30);
	det->ptp_ptpen[PTP_PHASE_MON] = ptp_read(PTP_PTPEN);
#if DUMP_DATA_TO_DE
	{
		int i;

		for (i = 0; i < ARRAY_SIZE(reg_dump_addr_off); i++) {
			det->reg_dump_data[i][PTP_PHASE_MON] =
			    ptp_read(PTP_BASEADDR + reg_dump_addr_off[i]);
			ptp_isr_info("0x%lx === 0x%08x\n",
				     (unsigned long)PTP_BASEADDR + reg_dump_addr_off[i],
				     det->reg_dump_data[i][PTP_PHASE_MON]
			    );
		}
	}
#endif
	/* check if thermal sensor init completed? */
	temp = (ptp_read(PTP_TEMP) & 0xff);

	if ((temp > 0x4b) && (temp < 0xd3)) {
		ptp_isr_info("thermal sensor init has not been completed.(temp = 0x%08X)\n", temp);
		goto out;
	}

	temp = ptp_read(PTP_VOP30);
	det->volt_tbl[0] = (temp & 0xff) + PTPOD_PMIC_OFFSET;
	det->volt_tbl[1] = ((temp >> 8) & 0xff) + PTPOD_PMIC_OFFSET;
	det->volt_tbl[2] = ((temp >> 16) & 0xff) + PTPOD_PMIC_OFFSET;
	det->volt_tbl[3] = ((temp >> 24) & 0xff) + PTPOD_PMIC_OFFSET;

	temp = ptp_read(PTP_VOP74);
	det->volt_tbl[4] = (temp & 0xff) + PTPOD_PMIC_OFFSET;
	det->volt_tbl[5] = ((temp >> 8) & 0xff) + PTPOD_PMIC_OFFSET;
	det->volt_tbl[6] = ((temp >> 16) & 0xff) + PTPOD_PMIC_OFFSET;
	det->volt_tbl[7] = ((temp >> 24) & 0xff) + PTPOD_PMIC_OFFSET;

	for (i = 0; i < NR_FREQ; i++)
		ptp_isr_info("ptp_detectors[%s].volt_tbl[%d] = 0x%08X (%d)\n",
			     det->name, i, det->volt_tbl[i],
			     PTP_PMIC_VAL_TO_VOLT(det->volt_tbl[i]));

	/* ptp_isr_info("ptp_level = 0x%08X\n", ptp_level); */

	ptp_set_ptp_volt(det);

out:
	/* Clear PTP INIT interrupt PTPINTSTS = 0x00ff0000 */
	ptp_write(PTP_PTPINTSTS, 0x00ff0000);

	FUNC_EXIT(FUNC_LV_LOCAL);
}

static inline void handle_mon_err_isr(struct ptp_det *det)
{
	FUNC_ENTER(FUNC_LV_LOCAL);

	/* PTP Monitor mode error handler */
	ptp_isr_info("====================================================\n");
	ptp_isr_info("PTP mon err: PTPEN(%p) = 0x%08X, PTPINTSTS(%p) = 0x%08X\n",
		     PTP_PTPEN, ptp_read(PTP_PTPEN), PTP_PTPINTSTS, ptp_read(PTP_PTPINTSTS));
	ptp_isr_info("PTP_SMSTATE0 (%p) = 0x%08X\n", PTP_SMSTATE0, ptp_read(PTP_SMSTATE0));
	ptp_isr_info("PTP_SMSTATE1 (%p) = 0x%08X\n", PTP_SMSTATE1, ptp_read(PTP_SMSTATE1));
	ptp_isr_info("PTP_TEMP (%p) = 0x%08X\n", PTP_TEMP, ptp_read(PTP_TEMP));
	ptp_isr_info("PTP_TEMPMSR0 (%p) = 0x%08X\n", PTP_TEMPMSR0, ptp_read(PTP_TEMPMSR0));
	ptp_isr_info("PTP_TEMPMSR1 (%p) = 0x%08X\n", PTP_TEMPMSR1, ptp_read(PTP_TEMPMSR1));
	ptp_isr_info("PTP_TEMPMSR2 (%p) = 0x%08X\n", PTP_TEMPMSR2, ptp_read(PTP_TEMPMSR2));
	ptp_isr_info("PTP_TEMPMONCTL0 (%p) = 0x%08X\n", PTP_TEMPMONCTL0, ptp_read(PTP_TEMPMONCTL0));
	ptp_isr_info("PTP_TEMPMSRCTL1 (%p) = 0x%08X\n", PTP_TEMPMSRCTL1, ptp_read(PTP_TEMPMSRCTL1));
	ptp_isr_info("====================================================\n");

	det->ops->disable_locked(det, BY_MON_ERROR);

	FUNC_EXIT(FUNC_LV_LOCAL);
}
static inline void ptp_isr_handler(struct ptp_det *det)
{
	unsigned int PTPINTSTS, PTPEN;

	FUNC_ENTER(FUNC_LV_LOCAL);

	PTPINTSTS = ptp_read(PTP_PTPINTSTS);
	PTPEN = ptp_read(PTP_PTPEN);

	ptp_isr_info("[%s]\n", det->name);
	ptp_isr_info("PTPINTSTS = 0x%08X\n", PTPINTSTS);
	ptp_isr_info("PTP_PTPEN = 0x%08X\n", PTPEN);
	ptp_isr_info("*(%p) = 0x%08X\n", PTP_DCVALUES, ptp_read(PTP_DCVALUES));
	ptp_isr_info("*(%p) = 0x%08X\n", PTP_AGECOUNT, ptp_read(PTP_AGECOUNT));

	if (PTPINTSTS == 0x1) {	/* PTP init1 or init2 */
		if ((PTPEN & 0x7) == 0x1)	/* PTP init1 */
			handle_init01_isr(det);
		else if ((PTPEN & 0x7) == 0x5)	/* PTP init2 */
			handle_init02_isr(det);
		else {
			handle_init_err_isr(det);
		}
	} else if ((PTPINTSTS & 0x00ff0000) != 0x0)
		handle_mon_mode_isr(det);
	else {
		if (((PTPEN & 0x7) == 0x1) || ((PTPEN & 0x7) == 0x5))
			handle_init_err_isr(det);
		else
			handle_mon_err_isr(det);
	}

	FUNC_EXIT(FUNC_LV_LOCAL);
}

static irqreturn_t ptp_isr(int irq, void *dev_id)
{
	unsigned long flags;
	struct ptp_det *det = NULL;
	int i;

	FUNC_ENTER(FUNC_LV_MODULE);

	/* mt_ptp_reg_dump(); */

	mt_ptp_lock(&flags);

	for (i = 0; i < NR_PTP_CTRL; i++) {
		/* if (i == PTP_CTRL_VCORE) */
		/* continue; */

		if ((BIT(i) & ptp_read(PTP_PTPODINTST)))
			continue;

		det = &ptp_detectors[i];

		det->ops->switch_bank(det);
		/* mt_ptp_reg_dump_locked(); */
		ptp_isr_handler(det);
	}

	mt_ptp_unlock(&flags);

	FUNC_EXIT(FUNC_LV_MODULE);

	return IRQ_HANDLED;
}

void ptp_init01(void)
{
	struct ptp_det *det;
	struct ptp_ctrl *ctrl;

	FUNC_ENTER(FUNC_LV_LOCAL);

	for_each_det_ctrl(det, ctrl) {
		{
			unsigned long flag;
			unsigned int vboot;

				vboot = PTP_VOLT_TO_PMIC_VAL(det->ops->get_volt(det));
			ptp_alert("@%s(),vboot = %d\n", __func__, vboot);

			if (vboot != det->VBOOT) {
				ptp_error("@%s():%d, get_volt(%s) = 0x%08X, VBOOT = 0x%08X\n",
					  __func__, __LINE__, det->name, vboot, det->VBOOT);
				aee_kernel_warning("mt_ptp",
						   "@%s():%d, get_volt(%s) = 0x%08X, VBOOT = 0x%08X\n",
						   __func__, __LINE__, det->name, vboot,
						   det->VBOOT);
			}

			mt_ptp_lock(&flag);
			det->ops->init01(det);
			mt_ptp_unlock(&flag);
		}
	}

	FUNC_EXIT(FUNC_LV_LOCAL);
}

void ptp_init02(void)
{
	struct ptp_det *det;
	struct ptp_ctrl *ctrl;

	 FUNC_ENTER(FUNC_LV_LOCAL);

	 for_each_det_ctrl(det, ctrl) {
		if (HAS_FEATURE(det, FEA_MON)) {
			unsigned long flag;

			 mt_ptp_lock(&flag);
			 det->ops->init02(det);
			 mt_ptp_unlock(&flag);
		}
	}

	FUNC_EXIT(FUNC_LV_LOCAL);
}

#if EN_PTP_OD
/* leakage */
unsigned int leakage_core;
unsigned int leakage_gpu;
unsigned int leakage_sram2;
unsigned int leakage_sram1;

void get_devinfo(struct ptp_devinfo *p)
{
	int *val = (int *)p;

	 FUNC_ENTER(FUNC_LV_HELP);

			val[0] = get_devinfo_with_index(7);	/* ptp_read(0x10206180); */
			val[1] = get_devinfo_with_index(8);	/* ptp_read(0x10206184); */
			val[2] = get_devinfo_with_index(9);	/* ptp_read(0x10206188); */
			val[3] = get_devinfo_with_index(14);	/* ptp_read(0x1020618C); */
			val[4] = get_devinfo_with_index(15);	/* ptp_read(0x10206190); */
	val[5] = get_devinfo_with_index(16);	/* ptp_read(0x10206194); */
			val[6] = get_devinfo_with_index(17);	/* ptp_read(0xF0206270); */
			val[7] = get_devinfo_with_index(47);	/* ptp_read(0xF02061B0); */

		ptp_crit("val[0]=0x%x\n", val[0]);
		ptp_crit("val[1]=0x%x\n", val[1]);
		ptp_crit("val[2]=0x%x\n", val[2]);
		ptp_crit("val[3]=0x%x\n", val[3]);
		ptp_crit("val[4]=0x%x\n", val[4]);
		ptp_crit("val[5]=0x%x\n", val[5]);
		ptp_crit("val[6]=0x%x\n", val[6]);
		ptp_crit("val[7]=0x%x\n", val[7]);

		ptp_crit("p->PTPINITEN=0x%x\n", p->PTPINITEN);
		ptp_crit("p->PTPMONEN=0x%x\n", p->PTPMONEN);

	FUNC_EXIT(FUNC_LV_HELP);
}

static int ptp_probe(struct platform_device *pdev)
{
	int ret;
	struct ptp_det *det;
	struct ptp_ctrl *ctrl;
	enum mt_cpu_dvfs_id cpu;

	FUNC_ENTER(FUNC_LV_MODULE);

	cpu = MT_CPU_DVFS_LITTLE;

	/* set PTP IRQ */
	ret = request_irq(ptpod_irq_number, ptp_isr, IRQF_TRIGGER_LOW, "ptp", NULL);

	if (ret) {
		ptp_notice("PTP IRQ register failed (%d)\n", ret);
		WARN_ON(1);
	}

	ptp_notice("Set PTP IRQ OK.\n");

	/* ptp_level = mt_ptp_get_level(); */
	/* atomic_set(&ptp_init01_cnt, 0); */
	for_each_ctrl(ctrl) {
		ptp_init_ctrl(ctrl);
	}

	mt_fh_popod_save();
	/* disable DVFS and set vproc = 1.15v (1 GHz) */
	mt_cpufreq_disable_by_ptpod(MT_CPU_DVFS_LITTLE);

	/* Enable PWM mode here */
	if (mt_cpufreq_get_freq_by_idx(cpu, 0) <= 1300000)
		pmic_force_vproc_pwm(1);

	/*for slow idle */
	ptp_data[0] = 0xffffffff;

	for_each_det(det) {
		ptp_init_det(det, &ptp_devinfo);
	}
	ptp_init01();

	/* Disable PWM mode here */
	if (mt_cpufreq_get_freq_by_idx(cpu, 0) <= 1300000)
		pmic_force_vproc_pwm(0);

	ptp_data[0] = 0;
	/* enable DVFS */
	mt_cpufreq_enable_by_ptpod(MT_CPU_DVFS_LITTLE);
	mt_fh_popod_restore();

	FUNC_EXIT(FUNC_LV_MODULE);

	return 0;
}

static int ptp_suspend(struct platform_device *pdev, pm_message_t state)
{
	/*
	   kthread_stop(ptp_volt_thread);
	 */
	FUNC_ENTER(FUNC_LV_MODULE);
	FUNC_EXIT(FUNC_LV_MODULE);

	return 0;
}
static int ptp_resume(struct platform_device *pdev)
{
	/*
	   ptp_volt_thread = kthread_run(ptp_volt_thread_handler, 0, "ptp volt");
	   if (IS_ERR(ptp_volt_thread))
	   {
	   printk("[%s]: failed to create ptp volt thread\n", __func__);
	   }
	 */
	FUNC_ENTER(FUNC_LV_MODULE);
	ptp_init02();
	FUNC_EXIT(FUNC_LV_MODULE);

	return 0;
}
#ifdef CONFIG_OF
static const struct of_device_id mt_ptpod_of_match[] = {
	{
	.compatible = "mediatek,ptp_fsm_v1",}, {
},};
#endif

static struct platform_driver ptp_driver = {
	.remove = NULL,
	.shutdown = NULL,
	.probe = ptp_probe,
	.suspend = ptp_suspend,
	.resume = ptp_resume,
	.driver = {
		.name = "mt-ptp",
#ifdef CONFIG_OF
		    .of_match_table = mt_ptpod_of_match,
#endif
		   },
};


int mt_ptp_opp_num(ptp_det_id id)
{
	struct ptp_det *det = id_to_ptp_det(id);

	FUNC_ENTER(FUNC_LV_API);
	FUNC_EXIT(FUNC_LV_API);

	return det->num_freq_tbl;
}
EXPORT_SYMBOL(mt_ptp_opp_num);

void mt_ptp_opp_freq(ptp_det_id id, unsigned int *freq)
{
	struct ptp_det *det = id_to_ptp_det(id);
	int i = 0;

	FUNC_ENTER(FUNC_LV_API);

	for (i = 0; i < det->num_freq_tbl; i++)
		freq[i] = det->freq_tbl[i];

	FUNC_EXIT(FUNC_LV_API);
}
EXPORT_SYMBOL(mt_ptp_opp_freq);

void mt_ptp_opp_status(ptp_det_id id, unsigned int *temp, unsigned int *volt)
{
	struct ptp_det *det = id_to_ptp_det(id);
	int i = 0;

	FUNC_ENTER(FUNC_LV_API);

	*temp = 0;

	for (i = 0; i < det->num_freq_tbl; i++)
		volt[i] = PTP_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[i]);

	FUNC_EXIT(FUNC_LV_API);
}
EXPORT_SYMBOL(mt_ptp_opp_status);

/*
* return current PTP stauts
*/
int mt_ptp_status(ptp_det_id id)
{
	struct ptp_det *det = id_to_ptp_det(id);

	FUNC_ENTER(FUNC_LV_API);

	BUG_ON(!det);
	BUG_ON(!det->ops);
	BUG_ON(!det->ops->get_status);

	FUNC_EXIT(FUNC_LV_API);

	return det->ops->get_status(det);
}

#ifdef CONFIG_PROC_FS
/*
 *
 * PROCFS interface for debugging
 *
 */

/*
 * show current PTP stauts
 */
static int ptp_debug_proc_show(struct seq_file *m, void *v)
{
	struct ptp_det *det = (struct ptp_det *)m->private;

	FUNC_ENTER(FUNC_LV_HELP);

	seq_printf(m, "PTPOD[%s] %s (ptp_level = 0x%08X)\n",
		    det->name, det->ops->get_status(det) ? "enabled" : "disable", ptp_level);

	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}
/*
 * set PTP status by procfs interface
 */
static ssize_t ptp_debug_proc_write(struct file *file,
					const char __user *buffer, size_t count, loff_t *pos)
{
	int ret;
	int enabled = 0;
	char *buf = (char *)__get_free_page(GFP_USER);
	struct ptp_det *det = (struct ptp_det *)PDE_DATA(file_inode(file));
	int rc;

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

	rc = kstrtoint(buf, 10, &enabled);
	if (rc < 0)
		ret = -EINVAL;
	else {
		ret = 0;
		if (0 == enabled)
			det->ops->disable(det, BY_PROCFS);
	}

out:
	free_page((unsigned long)buf);
	FUNC_EXIT(FUNC_LV_HELP);

	return (ret < 0) ? ret : count;
}

/*
 * show current PTP data
 */
static int ptp_dump_proc_show(struct seq_file *m, void *v)
{
	struct ptp_det *det;
	int *val = (int *)&ptp_devinfo;
	int i;

	FUNC_ENTER(FUNC_LV_HELP);

	mt_ptp_reg_dump();

	for (i = 0; i < sizeof(struct ptp_devinfo) / sizeof(unsigned int); i++)
		seq_printf(m, "PTP_OD%d\t= 0x%08X\n", i, val[i]);

	for_each_det(det) {

		seq_printf(m, "PTP_DCVALUES[%s]\t= 0x%08X\n", det->name, det->VBOOT);

		for (i = PTP_PHASE_INIT01; i < NR_PTP_PHASE; i++) {

			seq_printf(m,
				   "dcvalues=0x%08X, ptp_freqpct30=0x%08X, ptp_26c=0x%08X, ptp_vop30=0x%08X,ptp_ptpen= 0x%08X\n",
				   det->dcvalues[i], det->ptp_freqpct30[i], det->ptp_26c[i],
				   det->ptp_vop30[i], det->ptp_ptpen[i]
			    );
#if DUMP_DATA_TO_DE
			{
				int j;

				for (j = 0; j < ARRAY_SIZE(reg_dump_addr_off); j++)
					seq_printf(m, "0x%lx === 0x%08x\n",
						   (unsigned long)PTP_BASEADDR +
						   reg_dump_addr_off[j], det->reg_dump_data[j][i]
					    );
			}
#endif
		}
	}

	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

/*
 * show current voltage
 */
static int ptp_cur_volt_proc_show(struct seq_file *m, void *v)
{
	struct ptp_det *det = (struct ptp_det *)m->private;
	u32 rdata = 0;

	FUNC_ENTER(FUNC_LV_HELP);

	rdata = det->ops->get_volt(det);

	if (rdata != 0)
		seq_printf(m, "%d\n", rdata);
	else
		seq_printf(m, "PTPOD[%s] read current voltage fail\n", det->name);

	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

static int ptp_stress_result_proc_show(struct seq_file *m, void *v)
{
	if (stress_result != 0)
		ptp_isr_info("PTP fail to trigger irq\n");

	seq_printf(m, "0x%X\n", stress_result);

	return 0;
}

/*
 * show current PTP status
 */
static int ptp_status_proc_show(struct seq_file *m, void *v)
{
	struct ptp_det *det = (struct ptp_det *)m->private;

	 FUNC_ENTER(FUNC_LV_HELP);

	 seq_printf(m,
	 "PTP_LOG: PTPOD [%s] (%d) - (%d, %d, %d, %d, %d, %d, %d, %d) - (%d, %d, %d, %d, %d, %d, %d, %d)\n",
		    det->name, det->ops->get_temp(det),
		    PTP_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[0]),
		    PTP_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[1]),
		    PTP_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[2]),
		    PTP_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[3]),
		    PTP_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[4]),
		    PTP_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[5]),
		    PTP_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[6]),
		    PTP_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[7]),
		    det->freq_tbl[0],
		    det->freq_tbl[1],
		    det->freq_tbl[2],
		    det->freq_tbl[3],
		    det->freq_tbl[4], det->freq_tbl[5], det->freq_tbl[6], det->freq_tbl[7]);

	 FUNC_EXIT(FUNC_LV_HELP);

	 return 0;
}
/*
 * set PTP log enable by procfs interface
 */
static int ptp_log_en;

static int ptp_log_en_proc_show(struct seq_file *m, void *v)
{
	FUNC_ENTER(FUNC_LV_HELP);
	seq_printf(m, "%d\n", ptp_log_en);
	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}
static ssize_t ptp_log_en_proc_write(struct file *file,
				     const char __user *buffer, size_t count, loff_t *pos)
{
	int ret;
	char *buf = (char *)__get_free_page(GFP_USER);
	int rc;

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

	rc = kstrtoint(buf, 10, &ptp_log_en);
	if (rc < 0) {
		ptp_notice("bad argument!! Should be \"0\" or \"1\"\n");
		goto out;
	}

	ret = 0;

	switch (ptp_log_en) {
	case 0:
		ptp_notice("ptp log disabled.\n");
		hrtimer_cancel(&ptp_log_timer);
		break;

	case 1:
		ptp_notice("ptp log enabled.\n");
		hrtimer_start(&ptp_log_timer, ns_to_ktime(LOG_INTERVAL), HRTIMER_MODE_REL);
		break;

	default:
		ptp_error("bad argument!! Should be \"0\" or \"1\"\n");
		ret = -EINVAL;
	}

out:
	free_page((unsigned long)buf);
	FUNC_EXIT(FUNC_LV_HELP);

	return (ret < 0) ? ret : count;
}


/*
 * show PTP offset
 */
static int ptp_offset_proc_show(struct seq_file *m, void *v)
{
	struct ptp_det *det = (struct ptp_det *)m->private;

	 FUNC_ENTER(FUNC_LV_HELP);

	 seq_printf(m, "%d\n", det->volt_offset);

	 FUNC_EXIT(FUNC_LV_HELP);

	 return 0;
}
/*
 * set PTP offset by procfs
 */
static ssize_t ptp_offset_proc_write(struct file *file,
				     const char __user *buffer, size_t count, loff_t *pos)
{
	int ret;
	char *buf = (char *)__get_free_page(GFP_USER);
	int offset = 0;
	struct ptp_det *det = (struct ptp_det *)PDE_DATA(file_inode(file));
	int rc;

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

	rc = kstrtoint(buf, 10, &offset);
	if (rc < 0) {
		ret = -EINVAL;
		ptp_notice("bad argument_1!! argument should be \"0\"\n");
	} else {
		ret = 0;
		det->volt_offset = offset;
		ptp_set_ptp_volt(det);
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

PROC_FOPS_RW(ptp_debug);
PROC_FOPS_RO(ptp_dump);
PROC_FOPS_RO(ptp_stress_result);
PROC_FOPS_RW(ptp_log_en);
PROC_FOPS_RO(ptp_status);
PROC_FOPS_RO(ptp_cur_volt);
PROC_FOPS_RW(ptp_offset);

static int create_procfs(void)
{
	struct proc_dir_entry *ptp_dir = NULL;
	struct proc_dir_entry *det_dir = NULL;
	int i;
	struct ptp_det *det;

	struct pentry {
		const char *name;
		const struct file_operations *fops;
	};

	struct pentry det_entries[] = {
		PROC_ENTRY(ptp_debug),
		PROC_ENTRY(ptp_status),
		PROC_ENTRY(ptp_cur_volt),
		PROC_ENTRY(ptp_offset),
	};

	struct pentry ptp_entries[] = {
		PROC_ENTRY(ptp_dump),
		PROC_ENTRY(ptp_log_en),
		PROC_ENTRY(ptp_stress_result),
	};

	FUNC_ENTER(FUNC_LV_HELP);

	ptp_dir = proc_mkdir("ptp", NULL);

	if (!ptp_dir) {
		ptp_error("[%s]: mkdir /proc/ptp failed\n", __func__);
		FUNC_EXIT(FUNC_LV_HELP);
		return -1;
	}

	for (i = 0; i < ARRAY_SIZE(ptp_entries); i++) {
		if (!proc_create
		    (ptp_entries[i].name, S_IRUGO | S_IWUSR | S_IWGRP, ptp_dir,
		     ptp_entries[i].fops)) {
			ptp_error("[%s]: create /proc/ptp/%s failed\n", __func__,
				  ptp_entries[i].name);
			FUNC_EXIT(FUNC_LV_HELP);
			return -3;
		}
	}

	for_each_det(det) {
		det_dir = proc_mkdir(det->name, ptp_dir);

		if (!det_dir) {
			ptp_error("[%s]: mkdir /proc/ptp/%s failed\n", __func__, det->name);
			FUNC_EXIT(FUNC_LV_HELP);
			return -2;
		}

		for (i = 0; i < ARRAY_SIZE(det_entries); i++) {
			if (!proc_create_data
			    (det_entries[i].name, S_IRUGO | S_IWUSR | S_IWGRP, det_dir,
			     det_entries[i].fops, det)) {
				ptp_error("[%s]: create /proc/ptp/%s/%s failed\n", __func__,
					  det->name, det_entries[i].name);
				FUNC_EXIT(FUNC_LV_HELP);
				return -3;
			}
		}
	}

	FUNC_EXIT(FUNC_LV_HELP);
	return 0;
}
#endif

int get_ptpod_status(void)
{
	get_devinfo(&ptp_devinfo);

	return ptp_devinfo.PTPINITEN;

}
EXPORT_SYMBOL(get_ptpod_status);

#define VCORE_VOLT_0 1250000
#define VCORE_VOLT_1 1150000
#define VCORE_VOLT_2 1050000

unsigned int vcore0;
unsigned int vcore1;
unsigned int vcore2;
unsigned int have_550;

static int __init dt_get_ptp_devinfo(unsigned long node, const char *uname, int depth, void *data)
{
	struct devinfo_ptp_tag *tags;
	unsigned int size = 0;

	if (depth != 1 || (strcmp(uname, "chosen") != 0 && strcmp(uname, "chosen@0") != 0))
		return 0;

	tags = (struct devinfo_ptp_tag *)of_get_flat_dt_prop(node, "atag,ptp", &size);

	if (tags) {
		vcore0 = tags->volt0;
		vcore1 = tags->volt1;
		vcore2 = tags->volt2;
		have_550 = tags->have_550;
		ptp_notice("[PTP][VCORE] - Kernel Got from DT (0x%0X, 0x%0X, 0x%0X, 0x%0X)\n", vcore0,
		       vcore1, vcore2, have_550);
	}
	return 1;
}

unsigned int is_have_550(void)
{
	return have_550;
}
unsigned int get_vcore_ptp_volt(int uv)
{
	unsigned int ret;

	switch (uv) {
	case VCORE_VOLT_0:
		ret = vcore0;
		break;

	case VCORE_VOLT_1:
		ret = vcore1;
		break;

	case VCORE_VOLT_2:
		ret = vcore2;
		break;

	default:
		ret = PTP_VOLT_TO_PMIC_VAL(uv / 10) + PTPOD_PMIC_OFFSET;
		break;
	}
	if (ret == 0)
		ret = PTP_VOLT_TO_PMIC_VAL(uv / 10) + PTPOD_PMIC_OFFSET;

	return ret;
}

static int __init vcore_ptp_init(void)
{
	of_scan_flat_dt(dt_get_ptp_devinfo, NULL);

	return 0;
}
void process_voltage_bin(struct ptp_devinfo *devinfo)
{
	if (gpio_get_value(130)) {
		switch (devinfo->LTE_VOLTBIN) {
		case 0:
			mt_cpufreq_set_lte_volt(PTP_VOLT_TO_PMIC_VAL(110625) + PTPOD_PMIC_OFFSET);
			ptp_notice("VLTE voltage bin to 1.10625V\n");
			break;
		case 1:
			mt_cpufreq_set_lte_volt(PTP_VOLT_TO_PMIC_VAL(105000) + PTPOD_PMIC_OFFSET);
			ptp_notice("VLTE voltage bin to 1.05V\n");
			break;
		case 2:
			mt_cpufreq_set_lte_volt(PTP_VOLT_TO_PMIC_VAL(100000) + PTPOD_PMIC_OFFSET);
			ptp_notice("VLTE voltage bin to 1.0V\n");
			break;
		default:
			mt_cpufreq_set_lte_volt(PTP_VOLT_TO_PMIC_VAL(110625) + PTPOD_PMIC_OFFSET);
			ptp_notice("VLTE voltage bin to 1.10625V\n");
			break;
		};
	}
	/* mt_cpufreq_set_lte_volt(det->volt_tbl_bin[0]); */
}

/*
 * Module driver
 */
static int __init ptp_init(void)
{
	int err = 0;
	struct device_node *node = NULL;

	 node = of_find_compatible_node(NULL, NULL, "mediatek,ptp_fsm_v1");

	if (node) {
		/* Setup IO addresses */
		ptpod_base = of_iomap(node, 0);
	}

	/*get ptpod irq num */
	ptpod_irq_number = irq_of_parse_and_map(node, 0);
	if (!ptpod_irq_number)
		return 0;
	get_devinfo(&ptp_devinfo);

	process_voltage_bin(&ptp_devinfo);

	if (0 == ptp_devinfo.PTPINITEN) {
		ptp_notice("PTPINITEN = 0x%08X\n", ptp_devinfo.PTPINITEN);
		FUNC_EXIT(FUNC_LV_MODULE);
		return 0;
	}

	/*
	 * init timer for log / volt
	 */
	hrtimer_init(&ptp_log_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	ptp_log_timer.function = ptp_log_timer_func;

	create_procfs();

	err = platform_driver_register(&ptp_driver);

	if (err) {
		ptp_notice("PTP driver callback register failed..\n");
		FUNC_EXIT(FUNC_LV_MODULE);
		return err;
	}

	FUNC_EXIT(FUNC_LV_MODULE);

	return 0;
}

static void __exit ptp_exit(void)
{
	FUNC_ENTER(FUNC_LV_MODULE);
	ptp_notice("PTP de-initialization\n");
	FUNC_EXIT(FUNC_LV_MODULE);
} arch_initcall(vcore_ptp_init);
#ifndef CONFIG_MTK_FPGA
late_initcall(ptp_init);
#endif
#endif

MODULE_DESCRIPTION("MediaTek PTPOD Driver v0.3");
MODULE_LICENSE("GPL");

#undef __MT_PTP_C__
