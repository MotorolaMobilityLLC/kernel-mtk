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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>

#include <pwrap_hal.h>
#include <mtk_spm_pmic_wrap.h>

#include <mtk_spm.h>
#include <mt-plat/upmu_common.h>

/*
 * Macro and Definition
 */
#undef TAG
#define TAG "[SPM_PWRAP]"
#define spm_pwrap_crit(fmt, args...)	\
	pr_err(TAG"[CRTT]"fmt, ##args)
#define spm_pwrap_err(fmt, args...)	\
	pr_err(TAG"[ERR]"fmt, ##args)
#define spm_pwrap_warn(fmt, args...)	\
	pr_warn(TAG"[WARN]"fmt, ##args)
#define spm_pwrap_info(fmt, args...)	\
	pr_warn(TAG""fmt, ##args)	/* pr_info(TAG""fmt, ##args) */
#define spm_pwrap_debug(fmt, args...)	\
	pr_debug(TAG""fmt, ##args)


/*
 * BIT Operation
 */
#define _BIT_(_bit_)                    (unsigned)(1 << (_bit_))
#define _BITS_(_bits_, _val_)         \
	  ((((unsigned) -1 >> (31 - ((1) ? _bits_))) & ~((1U << ((0) ? _bits_)) - 1)) & ((_val_)<<((0) ? _bits_)))
#define _BITMASK_(_bits_)               (((unsigned) -1 >> (31 - ((1) ? _bits_))) & ~((1U << ((0) ? _bits_)) - 1))
#define _GET_BITS_VAL_(_bits_, _val_)   (((_val_) & (_BITMASK_(_bits_))) >> ((0) ? _bits_))

#define DEFAULT_VOLT_VSRAM      (100000)
#define DEFAULT_VOLT_VCORE      (100000)
#define NR_PMIC_WRAP_CMD (16)	/* 16 pmic wrap cmd */
#define MAX_RETRY_COUNT (100)

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

static struct pmic_wrap_setting pw = {
	.phase = NR_PMIC_WRAP_PHASE,	/* invalid setting for init */
	.addr = {{0, 0} },

	.set[PMIC_WRAP_PHASE_ALLINONE] = {
#if defined(CONFIG_MACH_MT6759) || defined(CONFIG_MACH_MT6758)
		._[IDX_ALL_VSRAM_PWR_ON]      = {PMIC_RG_LDO_VSRAM_PROC_EN_ADDR, _BITS_(0:0, 1),},
		._[IDX_ALL_VSRAM_SHUTDOWN]    = {PMIC_RG_LDO_VSRAM_PROC_EN_ADDR, _BITS_(0:0, 0),},
		._[IDX_ALL_VSRAM_NORMAL]      = {PMIC_RG_LDO_VSRAM_PROC_EN_ADDR, _BITS_(1:0, 1),},
		._[IDX_ALL_VSRAM_SLEEP]       = {PMIC_RG_LDO_VSRAM_PROC_EN_ADDR, _BITS_(1:0, 3),},
		._[IDX_ALL_DPIDLE_LEAVE]      = {PMIC_RG_SRCLKEN_IN2_EN_ADDR, _BITS_(0:0, 1),},
		._[IDX_ALL_DPIDLE_ENTER]      = {PMIC_RG_SRCLKEN_IN2_EN_ADDR, _BITS_(0:0, 0),},
		._[IDX_ALL_RESERVE_6]         = {0, 0,},
		._[IDX_ALL_VCORE_SUSPEND]     = {PMIC_RG_BUCK_VCORE_VOSEL_ADDR, VOLT_TO_PMIC_VAL(56800),},
		._[IDX_ALL_VPROCL2_PWR_ON]    = {PMIC_RG_BUCK_VPROC12_EN_ADDR, _BITS_(0:0, 1),},
		._[IDX_ALL_VPROCL2_SHUTDOWN]  = {PMIC_RG_BUCK_VPROC12_EN_ADDR, _BITS_(0:0, 0),},
		._[IDX_ALL_VCORE_LEVEL2]      = {PMIC_RG_BUCK_VCORE_VOSEL_ADDR, VOLT_TO_PMIC_VAL(62500),},
		._[IDX_ALL_VCORE_LEVEL3]      = {PMIC_RG_BUCK_VCORE_VOSEL_ADDR, VOLT_TO_PMIC_VAL(75000),},
		._[IDX_ALL_VPROC_PWR_ON]      = {PMIC_RG_BUCK_VPROC11_EN_ADDR, _BITS_(0:0, 1),},
		._[IDX_ALL_VPROC_SHUTDOWN]    = {PMIC_RG_BUCK_VPROC11_EN_ADDR, _BITS_(0:0, 0),},
		._[IDX_ALL_VPROC_NORMAL]      = {PMIC_RG_BUCK_VPROC11_EN_ADDR, _BITS_(1:0, 1),},
		._[IDX_ALL_VPROC_SLEEP]       = {PMIC_RG_BUCK_VPROC11_EN_ADDR, _BITS_(1:0, 3),},
#else
		._[IDX_ALL_1_VSRAM_PWR_ON]      = {MT6335_LDO_VSRAM_DVFS1_CON0, _BITS_(0:0, 1),},
		._[IDX_ALL_1_VSRAM_SHUTDOWN]    = {MT6335_LDO_VSRAM_DVFS1_CON0, _BITS_(0:0, 0),},
		._[IDX_ALL_1_VSRAM_NORMAL]      = {MT6335_LDO_VSRAM_DVFS1_CON1, VOLT_TO_PMIC_VAL(80000),},
		._[IDX_ALL_1_VSRAM_SLEEP]       = {MT6335_LDO_VSRAM_DVFS1_CON1, VOLT_TO_PMIC_VAL(55000),},
		._[IDX_ALL_DPIDLE_LEAVE]        = {MT6335_TOP_SPI_CON0, _BITS_(0:0, 1),},
		._[IDX_ALL_DPIDLE_ENTER]        = {MT6335_TOP_SPI_CON0, _BITS_(0:0, 0),},
		._[IDX_ALL_RESERVE_6]           = {0, 0,},
		._[IDX_ALL_VCORE_SUSPEND]       = {MT6335_BUCK_VCORE_CON1, VOLT_TO_PMIC_VAL(56800),},
		._[IDX_ALL_VCORE_LEVEL0]        = {MT6335_BUCK_VCORE_CON1, VOLT_TO_PMIC_VAL(65000),},
		._[IDX_ALL_VCORE_LEVEL1]        = {MT6335_BUCK_VCORE_CON1, VOLT_TO_PMIC_VAL(70000),},
		._[IDX_ALL_VCORE_LEVEL2]        = {MT6335_BUCK_VCORE_CON1, VOLT_TO_PMIC_VAL(75000),},
		._[IDX_ALL_VCORE_LEVEL3]        = {MT6335_BUCK_VCORE_CON1, VOLT_TO_PMIC_VAL(80000),},
		._[IDX_ALL_2_VSRAM_PWR_ON]      = {MT6335_LDO_VSRAM_DVFS2_CON0, _BITS_(0:0, 1),},
		._[IDX_ALL_2_VSRAM_SHUTDOWN]    = {MT6335_LDO_VSRAM_DVFS2_CON0, _BITS_(0:0, 0),},
		._[IDX_ALL_2_VSRAM_NORMAL]      = {MT6335_LDO_VSRAM_DVFS2_CON1, VOLT_TO_PMIC_VAL(80000),},
		._[IDX_ALL_2_VSRAM_SLEEP]       = {MT6335_LDO_VSRAM_DVFS2_CON1, VOLT_TO_PMIC_VAL(55000),},
#endif
		.nr_idx = NR_IDX_ALL,
	},
};

static DEFINE_SPINLOCK(pmic_wrap_lock);
#define pmic_wrap_lock(flags) spin_lock_irqsave(&pmic_wrap_lock, flags)
#define pmic_wrap_unlock(flags) spin_unlock_irqrestore(&pmic_wrap_lock, flags)

void _mt_spm_pmic_table_init(void)
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

	memcpy(pw.addr, pwrap_cmd_default, sizeof(pwrap_cmd_default));
}

void mt_spm_pmic_wrap_set_phase(enum pmic_wrap_phase_id phase)
{
	int i;
	unsigned long flags;

	WARN_ON(phase >= NR_PMIC_WRAP_PHASE);

	if (pw.phase == phase)
		return;

	if (pw.addr[0].cmd_addr == 0)
		_mt_spm_pmic_table_init();

	pmic_wrap_lock(flags);

	pw.phase = phase;

	for (i = 0; i < pw.set[phase].nr_idx; i++) {
		spm_write(pw.addr[i].cmd_addr, pw.set[phase]._[i].cmd_addr);
		spm_write(pw.addr[i].cmd_wdata, pw.set[phase]._[i].cmd_wdata);
	}

	spm_pwrap_warn("pmic table init: done\n");

	pmic_wrap_unlock(flags);
}
EXPORT_SYMBOL(mt_spm_pmic_wrap_set_phase);

void mt_spm_pmic_wrap_set_cmd(enum pmic_wrap_phase_id phase, int idx, unsigned int cmd_wdata)
{				/* just set wdata value */
	unsigned long flags;

	WARN_ON(phase >= NR_PMIC_WRAP_PHASE);
	WARN_ON(idx >= pw.set[phase].nr_idx);

	pmic_wrap_lock(flags);

	pw.set[phase]._[idx].cmd_wdata = cmd_wdata;

	if (pw.phase == phase)
		spm_write(pw.addr[idx].cmd_wdata, cmd_wdata);

	pmic_wrap_unlock(flags);
}
EXPORT_SYMBOL(mt_spm_pmic_wrap_set_cmd);

#if 1
void mt_spm_pmic_wrap_set_cmd_full(enum pmic_wrap_phase_id phase, int idx, unsigned int cmd_addr,
				   unsigned int cmd_wdata)
{
	unsigned long flags;

	WARN_ON(phase >= NR_PMIC_WRAP_PHASE);
	WARN_ON(idx >= pw.set[phase].nr_idx);

	pmic_wrap_lock(flags);

	pw.set[phase]._[idx].cmd_addr = cmd_addr;
	pw.set[phase]._[idx].cmd_wdata = cmd_wdata;

	if (pw.phase == phase) {
		spm_write(pw.addr[idx].cmd_addr, cmd_addr);
		spm_write(pw.addr[idx].cmd_wdata, cmd_wdata);
	}

	pmic_wrap_unlock(flags);
}
EXPORT_SYMBOL(mt_spm_pmic_wrap_set_cmd_full);

void mt_spm_pmic_wrap_get_cmd_full(enum pmic_wrap_phase_id phase, int idx, unsigned int *p_cmd_addr,
				   unsigned int *p_cmd_wdata)
{
	unsigned long flags;

	WARN_ON(phase >= NR_PMIC_WRAP_PHASE);
	WARN_ON(idx >= pw.set[phase].nr_idx);

	pmic_wrap_lock(flags);

	*p_cmd_addr = pw.set[phase]._[idx].cmd_addr;
	*p_cmd_wdata = pw.set[phase]._[idx].cmd_wdata;

	pmic_wrap_unlock(flags);
}
EXPORT_SYMBOL(mt_cpufreq_get_pmic_cmd_full);
#endif

int mt_spm_pmic_wrap_init(void)
{
	if (pw.addr[0].cmd_addr == 0)
		_mt_spm_pmic_table_init();

	/* init PMIC_WRAP & volt */
	mt_spm_pmic_wrap_set_phase(PMIC_WRAP_PHASE_ALLINONE);

	return 0;
}

MODULE_DESCRIPTION("SPM-PMIC_WRAP Driver v0.1");
