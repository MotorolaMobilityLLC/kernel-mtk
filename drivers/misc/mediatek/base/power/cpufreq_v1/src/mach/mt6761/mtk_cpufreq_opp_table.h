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
/* for DVFS OPP table LL */
#define CPU_DVFS_FREQ0_LL_FY    2001000    /* KHz */
#define CPU_DVFS_FREQ1_LL_FY    1934000    /* KHz */
#define CPU_DVFS_FREQ2_LL_FY    1867000    /* KHz */
#define CPU_DVFS_FREQ3_LL_FY    1800000    /* KHz */
#define CPU_DVFS_FREQ4_LL_FY    1734000    /* KHz */
#define CPU_DVFS_FREQ5_LL_FY    1667000    /* KHz */
#define CPU_DVFS_FREQ6_LL_FY    1600000    /* KHz */
#define CPU_DVFS_FREQ7_LL_FY    1533000    /* KHz */
#define CPU_DVFS_FREQ8_LL_FY    1466000    /* KHz */
#define CPU_DVFS_FREQ9_LL_FY    1400000    /* KHz */
#define CPU_DVFS_FREQ10_LL_FY   1308000    /* KHz */
#define CPU_DVFS_FREQ11_LL_FY   1216000    /* KHz */
#define CPU_DVFS_FREQ12_LL_FY   1125000    /* KHz */
#define CPU_DVFS_FREQ13_LL_FY   1033000    /* KHz */
#define CPU_DVFS_FREQ14_LL_FY    941000    /* KHz */
#define CPU_DVFS_FREQ15_LL_FY    850000    /* KHz */

#define CPU_DVFS_VOLT0_VPROC_LL_FY    102500         /* 10uV */
#define CPU_DVFS_VOLT1_VPROC_LL_FY    100000         /* 10uV */
#define CPU_DVFS_VOLT2_VPROC_LL_FY     97500         /* 10uV */
#define CPU_DVFS_VOLT3_VPROC_LL_FY     95000         /* 10uV */
#define CPU_DVFS_VOLT4_VPROC_LL_FY     92500         /* 10uV */
#define CPU_DVFS_VOLT5_VPROC_LL_FY     90000         /* 10uV */
#define CPU_DVFS_VOLT6_VPROC_LL_FY     87500         /* 10uV */
#define CPU_DVFS_VOLT7_VPROC_LL_FY     85000         /* 10uV */
#define CPU_DVFS_VOLT8_VPROC_LL_FY     82500         /* 10uV */
#define CPU_DVFS_VOLT9_VPROC_LL_FY     80000         /* 10uV */
#define CPU_DVFS_VOLT10_VPROC_LL_FY    77500         /* 10uV */
#define CPU_DVFS_VOLT11_VPROC_LL_FY    75000         /* 10uV */
#define CPU_DVFS_VOLT12_VPROC_LL_FY    72500         /* 10uV */
#define CPU_DVFS_VOLT13_VPROC_LL_FY    70000         /* 10uV */
#define CPU_DVFS_VOLT14_VPROC_LL_FY    67500         /* 10uV */
#define CPU_DVFS_VOLT15_VPROC_LL_FY    65000         /* 10uV */

/* SB */
/* for DVFS OPP table LL */
#define CPU_DVFS_FREQ0_LL_SB    2201000    /* KHz */
#define CPU_DVFS_FREQ1_LL_SB    2112000    /* KHz */
#define CPU_DVFS_FREQ2_LL_SB    2023000    /* KHz */
#define CPU_DVFS_FREQ3_LL_SB    1934000    /* KHz */
#define CPU_DVFS_FREQ4_LL_SB    1845000    /* KHz */
#define CPU_DVFS_FREQ5_LL_SB    1756000    /* KHz */
#define CPU_DVFS_FREQ6_LL_SB    1667000    /* KHz */
#define CPU_DVFS_FREQ7_LL_SB    1578000    /* KHz */
#define CPU_DVFS_FREQ8_LL_SB    1489000    /* KHz */
#define CPU_DVFS_FREQ9_LL_SB    1400000    /* KHz */
#define CPU_DVFS_FREQ10_LL_SB   1308000    /* KHz */
#define CPU_DVFS_FREQ11_LL_SB   1216000    /* KHz */
#define CPU_DVFS_FREQ12_LL_SB   1125000    /* KHz */
#define CPU_DVFS_FREQ13_LL_SB   1033000    /* KHz */
#define CPU_DVFS_FREQ14_LL_SB    941000    /* KHz */
#define CPU_DVFS_FREQ15_LL_SB    850000    /* KHz */

#define CPU_DVFS_VOLT0_VPROC_LL_SB    102500          /* 10uV */
#define CPU_DVFS_VOLT1_VPROC_LL_SB    100000          /* 10uV */
#define CPU_DVFS_VOLT2_VPROC_LL_SB     97500          /* 10uV */
#define CPU_DVFS_VOLT3_VPROC_LL_SB     95000          /* 10uV */
#define CPU_DVFS_VOLT4_VPROC_LL_SB     92500          /* 10uV */
#define CPU_DVFS_VOLT5_VPROC_LL_SB     90000          /* 10uV */
#define CPU_DVFS_VOLT6_VPROC_LL_SB     87500          /* 10uV */
#define CPU_DVFS_VOLT7_VPROC_LL_SB     85000          /* 10uV */
#define CPU_DVFS_VOLT8_VPROC_LL_SB     82500          /* 10uV */
#define CPU_DVFS_VOLT9_VPROC_LL_SB     80000          /* 10uV */
#define CPU_DVFS_VOLT10_VPROC_LL_SB    77500          /* 10uV */
#define CPU_DVFS_VOLT11_VPROC_LL_SB    75000          /* 10uV */
#define CPU_DVFS_VOLT12_VPROC_LL_SB    72500          /* 10uV */
#define CPU_DVFS_VOLT13_VPROC_LL_SB    70000          /* 10uV */
#define CPU_DVFS_VOLT14_VPROC_LL_SB    67500          /* 10uV */
#define CPU_DVFS_VOLT15_VPROC_LL_SB    65000          /* 10uV */

/* FY2 */
/* for DVFS OPP table LL */
#define CPU_DVFS_FREQ0_LL_FY2    2001000    /* KHz */
#define CPU_DVFS_FREQ1_LL_FY2    1934000    /* KHz */
#define CPU_DVFS_FREQ2_LL_FY2    1867000    /* KHz */
#define CPU_DVFS_FREQ3_LL_FY2    1800000    /* KHz */
#define CPU_DVFS_FREQ4_LL_FY2    1734000    /* KHz */
#define CPU_DVFS_FREQ5_LL_FY2    1667000    /* KHz */
#define CPU_DVFS_FREQ6_LL_FY2    1600000    /* KHz */
#define CPU_DVFS_FREQ7_LL_FY2    1533000    /* KHz */
#define CPU_DVFS_FREQ8_LL_FY2    1466000    /* KHz */
#define CPU_DVFS_FREQ9_LL_FY2    1400000    /* KHz */
#define CPU_DVFS_FREQ10_LL_FY2   1308000    /* KHz */
#define CPU_DVFS_FREQ11_LL_FY2   1216000    /* KHz */
#define CPU_DVFS_FREQ12_LL_FY2   1125000    /* KHz */
#define CPU_DVFS_FREQ13_LL_FY2   1033000    /* KHz */
#define CPU_DVFS_FREQ14_LL_FY2    941000    /* KHz */
#define CPU_DVFS_FREQ15_LL_FY2    850000    /* KHz */

#define CPU_DVFS_VOLT0_VPROC_LL_FY2    102500         /* 10uV */
#define CPU_DVFS_VOLT1_VPROC_LL_FY2    100000         /* 10uV */
#define CPU_DVFS_VOLT2_VPROC_LL_FY2     97500         /* 10uV */
#define CPU_DVFS_VOLT3_VPROC_LL_FY2     95000         /* 10uV */
#define CPU_DVFS_VOLT4_VPROC_LL_FY2     92500         /* 10uV */
#define CPU_DVFS_VOLT5_VPROC_LL_FY2     90000         /* 10uV */
#define CPU_DVFS_VOLT6_VPROC_LL_FY2     87500         /* 10uV */
#define CPU_DVFS_VOLT7_VPROC_LL_FY2     85000         /* 10uV */
#define CPU_DVFS_VOLT8_VPROC_LL_FY2     82500         /* 10uV */
#define CPU_DVFS_VOLT9_VPROC_LL_FY2     80000         /* 10uV */
#define CPU_DVFS_VOLT10_VPROC_LL_FY2    80000         /* 10uV */
#define CPU_DVFS_VOLT11_VPROC_LL_FY2    80000         /* 10uV */
#define CPU_DVFS_VOLT12_VPROC_LL_FY2    80000         /* 10uV */
#define CPU_DVFS_VOLT13_VPROC_LL_FY2    80000         /* 10uV */
#define CPU_DVFS_VOLT14_VPROC_LL_FY2    80000         /* 10uV */
#define CPU_DVFS_VOLT15_VPROC_LL_FY2    80000         /* 10uV */

/* DVFS OPP table */
#define OPP_TBL(cluster, seg, lv, vol)	\
static struct mt_cpu_freq_info opp_tbl_##cluster##_e##lv##_0[] = {        \
	OP                                                                \
(CPU_DVFS_FREQ0_##cluster##_##seg, CPU_DVFS_VOLT0_VPROC_##vol##_##seg),   \
	OP                                                                \
(CPU_DVFS_FREQ1_##cluster##_##seg, CPU_DVFS_VOLT1_VPROC_##vol##_##seg),   \
	OP                                                                \
(CPU_DVFS_FREQ2_##cluster##_##seg, CPU_DVFS_VOLT2_VPROC_##vol##_##seg),   \
	OP                                                                \
(CPU_DVFS_FREQ3_##cluster##_##seg, CPU_DVFS_VOLT3_VPROC_##vol##_##seg),   \
	OP                                                                \
(CPU_DVFS_FREQ4_##cluster##_##seg, CPU_DVFS_VOLT4_VPROC_##vol##_##seg),   \
	OP                                                                \
(CPU_DVFS_FREQ5_##cluster##_##seg, CPU_DVFS_VOLT5_VPROC_##vol##_##seg),   \
	OP                                                                \
(CPU_DVFS_FREQ6_##cluster##_##seg, CPU_DVFS_VOLT6_VPROC_##vol##_##seg),   \
	OP                                                                \
(CPU_DVFS_FREQ7_##cluster##_##seg, CPU_DVFS_VOLT7_VPROC_##vol##_##seg),   \
	OP                                                                \
(CPU_DVFS_FREQ8_##cluster##_##seg, CPU_DVFS_VOLT8_VPROC_##vol##_##seg),   \
	OP                                                                \
(CPU_DVFS_FREQ9_##cluster##_##seg, CPU_DVFS_VOLT9_VPROC_##vol##_##seg),   \
	OP                                                                \
(CPU_DVFS_FREQ10_##cluster##_##seg, CPU_DVFS_VOLT10_VPROC_##vol##_##seg), \
	OP                                                                \
(CPU_DVFS_FREQ11_##cluster##_##seg, CPU_DVFS_VOLT11_VPROC_##vol##_##seg), \
	OP                                                                \
(CPU_DVFS_FREQ12_##cluster##_##seg, CPU_DVFS_VOLT12_VPROC_##vol##_##seg), \
	OP                                                                \
(CPU_DVFS_FREQ13_##cluster##_##seg, CPU_DVFS_VOLT13_VPROC_##vol##_##seg), \
	OP                                                                \
(CPU_DVFS_FREQ14_##cluster##_##seg, CPU_DVFS_VOLT14_VPROC_##vol##_##seg), \
	OP                                                                \
(CPU_DVFS_FREQ15_##cluster##_##seg, CPU_DVFS_VOLT15_VPROC_##vol##_##seg), \
}

OPP_TBL(LL,  FY, 0, LL); /* opp_tbl_LL_e0_0  */
OPP_TBL(LL,  SB, 1, LL); /* opp_tbl_LL_e1_0  */
OPP_TBL(LL,  FY2, 2, LL); /* opp_tbl_LL_e2_0  */


/* v0.2 */
struct opp_tbl_info opp_tbls[NR_MT_CPU_DVFS][NUM_CPU_LEVEL] = {
	/* LL */
	{
		[CPU_LEVEL_0] = { opp_tbl_LL_e0_0,
				ARRAY_SIZE(opp_tbl_LL_e0_0) },
		[CPU_LEVEL_1] = { opp_tbl_LL_e1_0,
				ARRAY_SIZE(opp_tbl_LL_e1_0) },
		[CPU_LEVEL_2] = { opp_tbl_LL_e2_0,
				ARRAY_SIZE(opp_tbl_LL_e2_0) },
	},
};

/* 16 steps OPP table */
/* FY */
static struct mt_cpu_freq_method opp_tbl_method_LL_FY[] = {
	/* POS,	CLK */
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
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
};

/* SB */
static struct mt_cpu_freq_method opp_tbl_method_LL_SB[] = {
	/* POS,	CLK */
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
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
};

/* FY2 */
static struct mt_cpu_freq_method opp_tbl_method_LL_FY2[] = {
	/* POS,	CLK */
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
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
};

/* v0.2 */
struct opp_tbl_m_info opp_tbls_m[NR_MT_CPU_DVFS][NUM_CPU_LEVEL] = {
	/* LL */
	{
		[CPU_LEVEL_0] = { opp_tbl_method_LL_FY },
		[CPU_LEVEL_1] = { opp_tbl_method_LL_SB },
		[CPU_LEVEL_2] = { opp_tbl_method_LL_FY2 },
	},
};
