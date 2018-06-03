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

#include "mtk_cpufreq_struct.h"
#include "mtk_cpufreq_config.h"

/* FY */
/* for DVFS OPP table L */
#define CPU_DVFS_FREQ0_L_FY    2351000    /* KHz */
#define CPU_DVFS_FREQ1_L_FY    2314000    /* KHz */
#define CPU_DVFS_FREQ2_L_FY    2275000    /* KHz */
#define CPU_DVFS_FREQ3_L_FY    2223000    /* KHz */
#define CPU_DVFS_FREQ4_L_FY    2145000    /* KHz */
#define CPU_DVFS_FREQ5_L_FY    2067000    /* KHz */
#define CPU_DVFS_FREQ6_L_FY    1989000    /* KHz */
#define CPU_DVFS_FREQ7_L_FY    1898000    /* KHz */
#define CPU_DVFS_FREQ8_L_FY    1820000    /* KHz */
#define CPU_DVFS_FREQ9_L_FY    1742000    /* KHz */
#define CPU_DVFS_FREQ10_L_FY   1599000    /* KHz */
#define CPU_DVFS_FREQ11_L_FY   1469000    /* KHz */
#define CPU_DVFS_FREQ12_L_FY   1339000    /* KHz */
#define CPU_DVFS_FREQ13_L_FY   1170000    /* KHz */
#define CPU_DVFS_FREQ14_L_FY   1040000    /* KHz */
#define CPU_DVFS_FREQ15_L_FY    897000    /* KHz */

/* for DVFS OPP table LL */
#define CPU_DVFS_FREQ0_LL_FY    1701000    /* KHz */
#define CPU_DVFS_FREQ1_LL_FY    1664000    /* KHz */
#define CPU_DVFS_FREQ2_LL_FY    1638000    /* KHz */
#define CPU_DVFS_FREQ3_LL_FY    1573000    /* KHz */
#define CPU_DVFS_FREQ4_LL_FY    1508000    /* KHz */
#define CPU_DVFS_FREQ5_LL_FY    1417000    /* KHz */
#define CPU_DVFS_FREQ6_LL_FY    1339000    /* KHz */
#define CPU_DVFS_FREQ7_LL_FY    1235000    /* KHz */
#define CPU_DVFS_FREQ8_LL_FY    1131000    /* KHz */
#define CPU_DVFS_FREQ9_LL_FY    1040000    /* KHz */
#define CPU_DVFS_FREQ10_LL_FY    936000    /* KHz */
#define CPU_DVFS_FREQ11_LL_FY    832000    /* KHz */
#define CPU_DVFS_FREQ12_LL_FY    741000    /* KHz */
#define CPU_DVFS_FREQ13_LL_FY    611000    /* KHz */
#define CPU_DVFS_FREQ14_LL_FY    507000    /* KHz */
#define CPU_DVFS_FREQ15_LL_FY    390000    /* KHz */

/* for DVFS OPP table CCI */
#define CPU_DVFS_FREQ0_CCI_FY    1051000    /* KHz */
#define CPU_DVFS_FREQ1_CCI_FY    1027000    /* KHz */
#define CPU_DVFS_FREQ2_CCI_FY    1001000    /* KHz */
#define CPU_DVFS_FREQ3_CCI_FY     975000    /* KHz */
#define CPU_DVFS_FREQ4_CCI_FY     936000    /* KHz */
#define CPU_DVFS_FREQ5_CCI_FY     897000    /* KHz */
#define CPU_DVFS_FREQ6_CCI_FY     871000    /* KHz */
#define CPU_DVFS_FREQ7_CCI_FY     806000    /* KHz */
#define CPU_DVFS_FREQ8_CCI_FY     767000    /* KHz */
#define CPU_DVFS_FREQ9_CCI_FY     715000    /* KHz */
#define CPU_DVFS_FREQ10_CCI_FY    637000    /* KHz */
#define CPU_DVFS_FREQ11_CCI_FY    585000    /* KHz */
#define CPU_DVFS_FREQ12_CCI_FY    520000    /* KHz */
#define CPU_DVFS_FREQ13_CCI_FY    429000    /* KHz */
#define CPU_DVFS_FREQ14_CCI_FY    364000    /* KHz */
#define CPU_DVFS_FREQ15_CCI_FY    299000    /* KHz */

/* for DVFS OPP table */
#define CPU_DVFS_VOLT0_VPROC1_FY    111875    /* 10uV */
#define CPU_DVFS_VOLT1_VPROC1_FY    107500    /* 10uV */
#define CPU_DVFS_VOLT2_VPROC1_FY    103750    /* 10uV */
#define CPU_DVFS_VOLT3_VPROC1_FY    100000    /* 10uV */
#define CPU_DVFS_VOLT4_VPROC1_FY     96875    /* 10uV */
#define CPU_DVFS_VOLT5_VPROC1_FY     93125    /* 10uV */
#define CPU_DVFS_VOLT6_VPROC1_FY     90000    /* 10uV */
#define CPU_DVFS_VOLT7_VPROC1_FY     86250    /* 10uV */
#define CPU_DVFS_VOLT8_VPROC1_FY     83125    /* 10uV */
#define CPU_DVFS_VOLT9_VPROC1_FY     80000    /* 10uV */
#define CPU_DVFS_VOLT10_VPROC1_FY    76250    /* 10uV */
#define CPU_DVFS_VOLT11_VPROC1_FY    73125    /* 10uV */
#define CPU_DVFS_VOLT12_VPROC1_FY    70000    /* 10uV */
#define CPU_DVFS_VOLT13_VPROC1_FY    66250    /* 10uV */
#define CPU_DVFS_VOLT14_VPROC1_FY    63125    /* 10uV */
#define CPU_DVFS_VOLT15_VPROC1_FY    60000    /* 10uV */

/* SB */
/* for DVFS OPP table L */
#define CPU_DVFS_FREQ0_L_SB    2501000    /* KHz */
#define CPU_DVFS_FREQ1_L_SB    2392000    /* KHz */
#define CPU_DVFS_FREQ2_L_SB    2301000    /* KHz */
#define CPU_DVFS_FREQ3_L_SB    2223000    /* KHz */
#define CPU_DVFS_FREQ4_L_SB    2145000    /* KHz */
#define CPU_DVFS_FREQ5_L_SB    2067000    /* KHz */
#define CPU_DVFS_FREQ6_L_SB    1989000    /* KHz */
#define CPU_DVFS_FREQ7_L_SB    1898000    /* KHz */
#define CPU_DVFS_FREQ8_L_SB    1820000    /* KHz */
#define CPU_DVFS_FREQ9_L_SB    1742000    /* KHz */
#define CPU_DVFS_FREQ10_L_SB   1599000    /* KHz */
#define CPU_DVFS_FREQ11_L_SB   1469000    /* KHz */
#define CPU_DVFS_FREQ12_L_SB   1339000    /* KHz */
#define CPU_DVFS_FREQ13_L_SB   1170000    /* KHz */
#define CPU_DVFS_FREQ14_L_SB   1040000    /* KHz */
#define CPU_DVFS_FREQ15_L_SB    897000    /* KHz */

/* for DVFS OPP table LL */
#define CPU_DVFS_FREQ0_LL_SB    1901000    /* KHz */
#define CPU_DVFS_FREQ1_LL_SB    1781000    /* KHz */
#define CPU_DVFS_FREQ2_LL_SB    1677000    /* KHz */
#define CPU_DVFS_FREQ3_LL_SB    1573000    /* KHz */
#define CPU_DVFS_FREQ4_LL_SB    1508000    /* KHz */
#define CPU_DVFS_FREQ5_LL_SB    1417000    /* KHz */
#define CPU_DVFS_FREQ6_LL_SB    1339000    /* KHz */
#define CPU_DVFS_FREQ7_LL_SB    1235000    /* KHz */
#define CPU_DVFS_FREQ8_LL_SB    1131000    /* KHz */
#define CPU_DVFS_FREQ9_LL_SB    1040000    /* KHz */
#define CPU_DVFS_FREQ10_LL_SB    936000    /* KHz */
#define CPU_DVFS_FREQ11_LL_SB    832000    /* KHz */
#define CPU_DVFS_FREQ12_LL_SB    741000    /* KHz */
#define CPU_DVFS_FREQ13_LL_SB    611000    /* KHz */
#define CPU_DVFS_FREQ14_LL_SB    507000    /* KHz */
#define CPU_DVFS_FREQ15_LL_SB    390000    /* KHz */

/* for DVFS OPP table CCI */
#define CPU_DVFS_FREQ0_CCI_SB    1101000    /* KHz */
#define CPU_DVFS_FREQ1_CCI_SB    1053000    /* KHz */
#define CPU_DVFS_FREQ2_CCI_SB    1014000    /* KHz */
#define CPU_DVFS_FREQ3_CCI_SB     975000    /* KHz */
#define CPU_DVFS_FREQ4_CCI_SB     936000    /* KHz */
#define CPU_DVFS_FREQ5_CCI_SB     897000    /* KHz */
#define CPU_DVFS_FREQ6_CCI_SB     871000    /* KHz */
#define CPU_DVFS_FREQ7_CCI_SB     806000    /* KHz */
#define CPU_DVFS_FREQ8_CCI_SB     767000    /* KHz */
#define CPU_DVFS_FREQ9_CCI_SB     715000    /* KHz */
#define CPU_DVFS_FREQ10_CCI_SB    637000    /* KHz */
#define CPU_DVFS_FREQ11_CCI_SB    585000    /* KHz */
#define CPU_DVFS_FREQ12_CCI_SB    520000    /* KHz */
#define CPU_DVFS_FREQ13_CCI_SB    429000    /* KHz */
#define CPU_DVFS_FREQ14_CCI_SB    364000    /* KHz */
#define CPU_DVFS_FREQ15_CCI_SB    299000    /* KHz */

/* for DVFS OPP table */
#define CPU_DVFS_VOLT0_VPROC1_SB    111875     /* 10uV */
#define CPU_DVFS_VOLT1_VPROC1_SB    107500     /* 10uV */
#define CPU_DVFS_VOLT2_VPROC1_SB    103750     /* 10uV */
#define CPU_DVFS_VOLT3_VPROC1_SB    100000     /* 10uV */
#define CPU_DVFS_VOLT4_VPROC1_SB     96875     /* 10uV */
#define CPU_DVFS_VOLT5_VPROC1_SB     93125     /* 10uV */
#define CPU_DVFS_VOLT6_VPROC1_SB     90000     /* 10uV */
#define CPU_DVFS_VOLT7_VPROC1_SB     86250     /* 10uV */
#define CPU_DVFS_VOLT8_VPROC1_SB     83125     /* 10uV */
#define CPU_DVFS_VOLT9_VPROC1_SB     80000     /* 10uV */
#define CPU_DVFS_VOLT10_VPROC1_SB    76250     /* 10uV */
#define CPU_DVFS_VOLT11_VPROC1_SB    73125     /* 10uV */
#define CPU_DVFS_VOLT12_VPROC1_SB    70000     /* 10uV */
#define CPU_DVFS_VOLT13_VPROC1_SB    66250     /* 10uV */
#define CPU_DVFS_VOLT14_VPROC1_SB    63125     /* 10uV */
#define CPU_DVFS_VOLT15_VPROC1_SB    60000     /* 10uV */

/* DVFS OPP table */
#define OPP_TBL(cluster, seg, lv, vol)	\
static struct mt_cpu_freq_info opp_tbl_##cluster##_e##lv##_0[] = {        \
	OP                                                                \
(CPU_DVFS_FREQ0_##cluster##_##seg, CPU_DVFS_VOLT0_VPROC##vol##_##seg),    \
	OP                                                                \
(CPU_DVFS_FREQ1_##cluster##_##seg, CPU_DVFS_VOLT1_VPROC##vol##_##seg),    \
	OP                                                                \
(CPU_DVFS_FREQ2_##cluster##_##seg, CPU_DVFS_VOLT2_VPROC##vol##_##seg),    \
	OP                                                                \
(CPU_DVFS_FREQ3_##cluster##_##seg, CPU_DVFS_VOLT3_VPROC##vol##_##seg),    \
	OP                                                                \
(CPU_DVFS_FREQ4_##cluster##_##seg, CPU_DVFS_VOLT4_VPROC##vol##_##seg),    \
	OP                                                                \
(CPU_DVFS_FREQ5_##cluster##_##seg, CPU_DVFS_VOLT5_VPROC##vol##_##seg),    \
	OP                                                                \
(CPU_DVFS_FREQ6_##cluster##_##seg, CPU_DVFS_VOLT6_VPROC##vol##_##seg),    \
	OP                                                                \
(CPU_DVFS_FREQ7_##cluster##_##seg, CPU_DVFS_VOLT7_VPROC##vol##_##seg),    \
	OP                                                                \
(CPU_DVFS_FREQ8_##cluster##_##seg, CPU_DVFS_VOLT8_VPROC##vol##_##seg),    \
	OP                                                                \
(CPU_DVFS_FREQ9_##cluster##_##seg, CPU_DVFS_VOLT9_VPROC##vol##_##seg),    \
	OP                                                                \
(CPU_DVFS_FREQ10_##cluster##_##seg, CPU_DVFS_VOLT10_VPROC##vol##_##seg),  \
	OP                                                                \
(CPU_DVFS_FREQ11_##cluster##_##seg, CPU_DVFS_VOLT11_VPROC##vol##_##seg),  \
	OP                                                                \
(CPU_DVFS_FREQ12_##cluster##_##seg, CPU_DVFS_VOLT12_VPROC##vol##_##seg),  \
	OP                                                                \
(CPU_DVFS_FREQ13_##cluster##_##seg, CPU_DVFS_VOLT13_VPROC##vol##_##seg),  \
	OP                                                                \
(CPU_DVFS_FREQ14_##cluster##_##seg, CPU_DVFS_VOLT14_VPROC##vol##_##seg),  \
	OP                                                                \
(CPU_DVFS_FREQ15_##cluster##_##seg, CPU_DVFS_VOLT15_VPROC##vol##_##seg),  \
}

OPP_TBL(L,   FY, 0, 1); /* opp_tbl_L_e0_0   */
OPP_TBL(LL,  FY, 0, 1); /* opp_tbl_LL_e0_0  */
OPP_TBL(CCI, FY, 0, 1); /* opp_tbl_CCI_e0_0 */

OPP_TBL(L,   SB, 1, 1); /* opp_tbl_L_e1_0   */
OPP_TBL(LL,  SB, 1, 1); /* opp_tbl_LL_e1_0  */
OPP_TBL(CCI, SB, 1, 1); /* opp_tbl_CCI_e1_0 */

/* v0.4 */
struct opp_tbl_info opp_tbls[NR_MT_CPU_DVFS][NUM_CPU_LEVEL] = {
	/* L */
	{
		[CPU_LEVEL_0] = { opp_tbl_L_e0_0,
				ARRAY_SIZE(opp_tbl_L_e0_0) },
		[CPU_LEVEL_1] = { opp_tbl_L_e1_0,
				ARRAY_SIZE(opp_tbl_L_e1_0) },
	},
	/* LL */
	{
		[CPU_LEVEL_0] = { opp_tbl_LL_e0_0,
				ARRAY_SIZE(opp_tbl_LL_e0_0) },
		[CPU_LEVEL_1] = { opp_tbl_LL_e1_0,
				ARRAY_SIZE(opp_tbl_LL_e1_0) },
	},
	/* CCI */
	{
		[CPU_LEVEL_0] = { opp_tbl_CCI_e0_0,
				ARRAY_SIZE(opp_tbl_CCI_e0_0) },
		[CPU_LEVEL_1] = { opp_tbl_CCI_e1_0,
				ARRAY_SIZE(opp_tbl_CCI_e1_0) },
	},
};

/* 16 steps OPP table */
static struct mt_cpu_freq_method opp_tbl_method_L_FY[] = {
	/* POS,	CLK */
	FP(1,	1),
	FP(1,	1),
	FP(1,	1),
	FP(1,	1),
	FP(1,	1),
	FP(1,	1),
	FP(1,	1),
	FP(1,	1),
	FP(1,	1),
	FP(1,	1),
	FP(1,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
};

static struct mt_cpu_freq_method opp_tbl_method_LL_FY[] = {
	/* POS,	CLK */
	FP(1,	1),
	FP(1,	1),
	FP(1,	1),
	FP(1,	1),
	FP(1,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(4,	1),
	FP(4,	1),
	FP(4,	1),
	FP(4,	2),
};

static struct mt_cpu_freq_method opp_tbl_method_CCI_FY[] = {
	/* POS,	CLK */
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(4,	1),
	FP(4,	1),
	FP(4,	1),
	FP(4,	1),
	FP(4,	2),
	FP(4,	2),
	FP(4,	2),
};

static struct mt_cpu_freq_method opp_tbl_method_L_SB[] = {
	/* POS,	CLK */
	FP(1,	1),
	FP(1,	1),
	FP(1,	1),
	FP(1,	1),
	FP(1,	1),
	FP(1,	1),
	FP(1,	1),
	FP(1,	1),
	FP(1,	1),
	FP(1,	1),
	FP(1,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
};

static struct mt_cpu_freq_method opp_tbl_method_LL_SB[] = {
	/* POS,	CLK */
	FP(1,	1),
	FP(1,	1),
	FP(1,	1),
	FP(1,	1),
	FP(1,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(4,	1),
	FP(4,	1),
	FP(4,	1),
	FP(4,	2),
};

static struct mt_cpu_freq_method opp_tbl_method_CCI_SB[] = {
	/* POS,	CLK */
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(4,	1),
	FP(4,	1),
	FP(4,	1),
	FP(4,	1),
	FP(4,	2),
	FP(4,	2),
	FP(4,	2),
};

/* v0.4 */
struct opp_tbl_m_info opp_tbls_m[NR_MT_CPU_DVFS][NUM_CPU_LEVEL] = {
	/* L */
	{
		[CPU_LEVEL_0] = { opp_tbl_method_L_FY },
		[CPU_LEVEL_1] = { opp_tbl_method_L_SB },
	},
	/* LL */
	{
		[CPU_LEVEL_0] = { opp_tbl_method_LL_FY },
		[CPU_LEVEL_1] = { opp_tbl_method_LL_SB },
	},
	/* CCI */
	{
		[CPU_LEVEL_0] = { opp_tbl_method_CCI_FY },
		[CPU_LEVEL_1] = { opp_tbl_method_CCI_SB },
	},
};
