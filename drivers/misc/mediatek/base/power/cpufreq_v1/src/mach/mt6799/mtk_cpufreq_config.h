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

/*
* @file mt_cpufreq_config.h
* @brief CPU DVFS driver interface
*/

#ifndef __MT_CPUFREQ_CONFIG_H__
#define __MT_CPUFREQ_CONFIG_H__

#include <mt-plat/upmu_common.h>
#include <mach/upmu_sw.h>
#include <mach/upmu_hw.h>
/* #include <mt6313/mt6313.h> */

/* 750Mhz */
#define DEFAULT_B_FREQ_IDX 13

enum mt_cpu_dvfs_id {
	MT_CPU_DVFS_LL,
	MT_CPU_DVFS_L,
	MT_CPU_DVFS_B,
	MT_CPU_DVFS_CCI,

	NR_MT_CPU_DVFS,
};

/* PMIC Config */
enum mt_cpu_dvfs_buck_id {
	CPU_DVFS_VPROC1, /* LL */
	CPU_DVFS_VPROC2, /* L */
	CPU_DVFS_VSRAM1, /* LL */
	CPU_DVFS_VSRAM2, /* L */

	NR_MT_BUCK,
};

enum mt_cpu_dvfs_pmic_type {
	BUCK_ISL91302a_VPROC1,
	BUCK_ISL91302a_VPROC2,
	BUCK_MT6335_VSRAM1,
	BUCK_MT6335_VSRAM2,

	NR_MT_PMIC,
};

/* PLL Config */
enum mt_cpu_dvfs_pll_id {
	PLL_LL_CLUSTER,
	PLL_L_CLUSTER,
	PLL_CCI_CLUSTER,
	PLL_B_CLUSTER,

	NR_MT_PLL,
};

/* 3 => MAIN */
enum top_ckmuxsel {
	TOP_CKMUXSEL_CLKSQ = 0,
	TOP_CKMUXSEL_ARMPLL = 1,
	TOP_CKMUXSEL_MAINPLL = 2,
	TOP_CKMUXSEL_UNIVPLL = 3,

	NR_TOP_CKMUXSEL,
};
#endif
