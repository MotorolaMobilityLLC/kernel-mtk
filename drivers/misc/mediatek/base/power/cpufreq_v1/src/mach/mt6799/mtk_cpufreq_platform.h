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
#ifndef __MT_CPUFREQ_PLATFORM_H__
#define __MT_CPUFREQ_PLATFORM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "mtk_cpufreq_config.h"
#include "mtk_cpufreq_internal.h"
/* 0x11F1_07C0 */
#define FUNC_CODE_EFUSE_INDEX	(120)

/* buck ctrl configs */
/* #define MIN_DIFF_VSRAM_PROC        1000  */
#define NORMAL_DIFF_VRSAM_VPROC    10000
#define MAX_DIFF_VSRAM_VPROC       25000
#define MIN_VSRAM_VOLT             80000  /* 85625 */
#define MAX_VSRAM_VOLT             103125 /* 97500 */
#define MIN_VPROC_VOLT             56875  /* 75014 */
#define MAX_VPROC_VOLT             103000 /* 97374 */
#define MIN_PMIC_SETTLE_TIME  25
#define PMIC_CMD_DELAY_TIME     5

int set_cur_volt_proc_cpu1(struct buck_ctrl_t *buck_p, unsigned int volt);
int set_cur_volt_proc_cpu2(struct buck_ctrl_t *buck_p, unsigned int volt);
unsigned int get_cur_volt_proc_cpu1(struct buck_ctrl_t *buck_p);
unsigned int get_cur_volt_proc_cpu2(struct buck_ctrl_t *buck_p);
unsigned int ISL191302_transfer2pmicval(unsigned int volt);
unsigned int ISL191302_transfer2volt(unsigned int val);
unsigned int ISL191302_settletime(unsigned int old_mv, unsigned int new_mv);

int set_cur_volt_sram_cpu1(struct buck_ctrl_t *buck_p, unsigned int volt); /* volt: mv * 100 */
int set_cur_volt_sram_cpu2(struct buck_ctrl_t *buck_p, unsigned int volt); /* volt: mv * 100 */
unsigned int get_cur_volt_sram_cpu1(struct buck_ctrl_t *buck_p);
unsigned int get_cur_volt_sram_cpu2(struct buck_ctrl_t *buck_p);
unsigned int MT6335_transfer2pmicval(unsigned int volt);
unsigned int MT6335_transfer2volt(unsigned int val);
unsigned int MT6335_settletime(unsigned int old_volt, unsigned int new_volt);

void prepare_pll_addr(enum mt_cpu_dvfs_pll_id pll_id);
void adjust_freq_hopping(struct pll_ctrl_t *pll_p, unsigned int dds);
unsigned int get_cur_phy_freq(struct pll_ctrl_t *pll_p);
void adjust_clkdiv(struct pll_ctrl_t *pll_p, unsigned int clk_div);
void adjust_posdiv(struct pll_ctrl_t *pll_p, unsigned int pos_div);
unsigned int _cpu_dds_calc(unsigned int khz);
void adjust_armpll_dds(struct pll_ctrl_t *pll_p, unsigned int vco, unsigned int pos_div);
void _cpu_clock_switch(struct pll_ctrl_t *pll_p, enum top_ckmuxsel sel);
enum top_ckmuxsel _get_cpu_clock_switch(struct pll_ctrl_t *pll_p);

int mt_cpufreq_dts_map(void);
int mt_cpufreq_regulator_map(struct platform_device *pdev);
#ifdef __cplusplus
}
#endif

#endif

