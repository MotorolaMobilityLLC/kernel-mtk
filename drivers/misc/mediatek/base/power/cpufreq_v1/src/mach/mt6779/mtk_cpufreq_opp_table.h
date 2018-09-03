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
#define CPU_DVFS_FREQ0_LL_FY		1970000		/* KHz */
#define CPU_DVFS_FREQ1_LL_FY		1908000		/* KHz */
#define CPU_DVFS_FREQ2_LL_FY		1846000		/* KHz */
#define CPU_DVFS_FREQ3_LL_FY		1785000		/* KHz */
#define CPU_DVFS_FREQ4_LL_FY		1723000		/* KHz */
#define CPU_DVFS_FREQ5_LL_FY		1661000		/* KHz */
#define CPU_DVFS_FREQ6_LL_FY		1548000		/* KHz */
#define CPU_DVFS_FREQ7_LL_FY		1475000		/* KHz */
#define CPU_DVFS_FREQ8_LL_FY		1375000		/* KHz */
#define CPU_DVFS_FREQ9_LL_FY		1275000		/* KHz */
#define CPU_DVFS_FREQ10_LL_FY		1175000		/* KHz */
#define CPU_DVFS_FREQ11_LL_FY		1075000		/* KHz */
#define CPU_DVFS_FREQ12_LL_FY		999000		/* KHz */
#define CPU_DVFS_FREQ13_LL_FY		925000		/* KHz */
#define CPU_DVFS_FREQ14_LL_FY		850000		/* KHz */
#define CPU_DVFS_FREQ15_LL_FY		774000		/* KHz */

/* for DVFS OPP table B */
#define CPU_DVFS_FREQ0_L_FY			2184000		/* KHz */
#define CPU_DVFS_FREQ1_L_FY			2120000		/* KHz */
#define CPU_DVFS_FREQ2_L_FY			2056000		/* KHz */
#define CPU_DVFS_FREQ3_L_FY			1992000		/* KHz */
#define CPU_DVFS_FREQ4_L_FY			1928000		/* KHz */
#define CPU_DVFS_FREQ5_L_FY			1864000		/* KHz */
#define CPU_DVFS_FREQ6_L_FY			1800000		/* KHz */
#define CPU_DVFS_FREQ7_L_FY			1651000		/* KHz */
#define CPU_DVFS_FREQ8_L_FY			1532000		/* KHz */
#define CPU_DVFS_FREQ9_L_FY			1414000		/* KHz */
#define CPU_DVFS_FREQ10_L_FY		1295000		/* KHz */
#define CPU_DVFS_FREQ11_L_FY		1176000		/* KHz */
#define CPU_DVFS_FREQ12_L_FY		1087000		/* KHz */
#define CPU_DVFS_FREQ13_L_FY		998000		/* KHz */
#define CPU_DVFS_FREQ14_L_FY		909000		/* KHz */
#define CPU_DVFS_FREQ15_L_FY		850000		/* KHz */

/* for DVFS OPP table CCI */
#define CPU_DVFS_FREQ0_CCI_FY		1400000		/* KHz */
#define CPU_DVFS_FREQ1_CCI_FY		1353000		/* KHz */
#define CPU_DVFS_FREQ2_CCI_FY		1306000		/* KHz */
#define CPU_DVFS_FREQ3_CCI_FY		1260000		/* KHz */
#define CPU_DVFS_FREQ4_CCI_FY		1190000		/* KHz */
#define CPU_DVFS_FREQ5_CCI_FY		1120000		/* KHz */
#define CPU_DVFS_FREQ6_CCI_FY		1030000		/* KHz */
#define CPU_DVFS_FREQ7_CCI_FY		962000		/* KHz */
#define CPU_DVFS_FREQ8_CCI_FY		894000		/* KHz */
#define CPU_DVFS_FREQ9_CCI_FY		827000		/* KHz */
#define CPU_DVFS_FREQ10_CCI_FY		737000		/* KHz */
#define CPU_DVFS_FREQ11_CCI_FY		669000		/* KHz */
#define CPU_DVFS_FREQ12_CCI_FY		602000		/* KHz */
#define CPU_DVFS_FREQ13_CCI_FY		534000		/* KHz */
#define CPU_DVFS_FREQ14_CCI_FY		467000		/* KHz */
#define CPU_DVFS_FREQ15_CCI_FY		400000		/* KHz */

/* for DVFS OPP table L */
#define CPU_DVFS_VOLT0_VPROC1_FY	 95000		/* 10uV */
#define CPU_DVFS_VOLT1_VPROC1_FY	 92500		/* 10uV */
#define CPU_DVFS_VOLT2_VPROC1_FY	 90000		/* 10uV */
#define CPU_DVFS_VOLT3_VPROC1_FY	 87500		/* 10uV */
#define CPU_DVFS_VOLT4_VPROC1_FY	 85000		/* 10uV */
#define CPU_DVFS_VOLT5_VPROC1_FY	 82500		/* 10uV */
#define CPU_DVFS_VOLT6_VPROC1_FY	 80000		/* 10uV */
#define CPU_DVFS_VOLT7_VPROC1_FY	 76875		/* 10uV */
#define CPU_DVFS_VOLT8_VPROC1_FY	 74375		/* 10uV */
#define CPU_DVFS_VOLT9_VPROC1_FY	 71875		/* 10uV */
#define CPU_DVFS_VOLT10_VPROC1_FY	 69375		/* 10uV */
#define CPU_DVFS_VOLT11_VPROC1_FY	 66875		/* 10uV */
#define CPU_DVFS_VOLT12_VPROC1_FY	 65000		/* 10uV */
#define CPU_DVFS_VOLT13_VPROC1_FY	 63125		/* 10uV */
#define CPU_DVFS_VOLT14_VPROC1_FY	 61250		/* 10uV */
#define CPU_DVFS_VOLT15_VPROC1_FY	 60000		/* 10uV */

/* for DVFS OPP table B */
#define CPU_DVFS_VOLT0_VPROC2_FY	 95000		/* 10uV */
#define CPU_DVFS_VOLT1_VPROC2_FY	 92500		/* 10uV */
#define CPU_DVFS_VOLT2_VPROC2_FY	 90000		/* 10uV */
#define CPU_DVFS_VOLT3_VPROC2_FY	 87500		/* 10uV */
#define CPU_DVFS_VOLT4_VPROC2_FY	 85000		/* 10uV */
#define CPU_DVFS_VOLT5_VPROC2_FY	 82500		/* 10uV */
#define CPU_DVFS_VOLT6_VPROC2_FY	 80000		/* 10uV */
#define CPU_DVFS_VOLT7_VPROC2_FY	 76875		/* 10uV */
#define CPU_DVFS_VOLT8_VPROC2_FY	 74375		/* 10uV */
#define CPU_DVFS_VOLT9_VPROC2_FY	 71875		/* 10uV */
#define CPU_DVFS_VOLT10_VPROC2_FY	 69375		/* 10uV */
#define CPU_DVFS_VOLT11_VPROC2_FY	 66875		/* 10uV */
#define CPU_DVFS_VOLT12_VPROC2_FY	 65000		/* 10uV */
#define CPU_DVFS_VOLT13_VPROC2_FY	 63125		/* 10uV */
#define CPU_DVFS_VOLT14_VPROC2_FY	 61250		/* 10uV */
#define CPU_DVFS_VOLT15_VPROC2_FY	 60000		/* 10uV */

/* for DVFS OPP table CCI */
#define CPU_DVFS_VOLT0_VPROC3_FY	 95000		/* 10uV */
#define CPU_DVFS_VOLT1_VPROC3_FY	 92500		/* 10uV */
#define CPU_DVFS_VOLT2_VPROC3_FY	 90000		/* 10uV */
#define CPU_DVFS_VOLT3_VPROC3_FY	 87500		/* 10uV */
#define CPU_DVFS_VOLT4_VPROC3_FY	 83750		/* 10uV */
#define CPU_DVFS_VOLT5_VPROC3_FY	 80000		/* 10uV */
#define CPU_DVFS_VOLT6_VPROC3_FY	 77500		/* 10uV */
#define CPU_DVFS_VOLT7_VPROC3_FY	 75625		/* 10uV */
#define CPU_DVFS_VOLT8_VPROC3_FY	 73750		/* 10uV */
#define CPU_DVFS_VOLT9_VPROC3_FY	 71875		/* 10uV */
#define CPU_DVFS_VOLT10_VPROC3_FY	 69375		/* 10uV */
#define CPU_DVFS_VOLT11_VPROC3_FY	 67500		/* 10uV */
#define CPU_DVFS_VOLT12_VPROC3_FY	 65625		/* 10uV */
#define CPU_DVFS_VOLT13_VPROC3_FY	 63750		/* 10uV */
#define CPU_DVFS_VOLT14_VPROC3_FY	 61875		/* 10uV */
#define CPU_DVFS_VOLT15_VPROC3_FY	 60000		/* 10uV */

/* SBa */
/* for DVFS OPP table L */
#define CPU_DVFS_FREQ0_LL_SBa		1970000		/* KHz */
#define CPU_DVFS_FREQ1_LL_SBa		1908000		/* KHz */
#define CPU_DVFS_FREQ2_LL_SBa		1846000		/* KHz */
#define CPU_DVFS_FREQ3_LL_SBa		1785000		/* KHz */
#define CPU_DVFS_FREQ4_LL_SBa		1723000		/* KHz */
#define CPU_DVFS_FREQ5_LL_SBa		1661000		/* KHz */
#define CPU_DVFS_FREQ6_LL_SBa		1548000		/* KHz */
#define CPU_DVFS_FREQ7_LL_SBa		1475000		/* KHz */
#define CPU_DVFS_FREQ8_LL_SBa		1375000		/* KHz */
#define CPU_DVFS_FREQ9_LL_SBa		1275000		/* KHz */
#define CPU_DVFS_FREQ10_LL_SBa		1175000		/* KHz */
#define CPU_DVFS_FREQ11_LL_SBa		1075000		/* KHz */
#define CPU_DVFS_FREQ12_LL_SBa		999000		/* KHz */
#define CPU_DVFS_FREQ13_LL_SBa		925000		/* KHz */
#define CPU_DVFS_FREQ14_LL_SBa		850000		/* KHz */
#define CPU_DVFS_FREQ15_LL_SBa		774000		/* KHz */

/* for DVFS OPP table B */
#define CPU_DVFS_FREQ0_L_SBa		2391000		/* KHz */
#define CPU_DVFS_FREQ1_L_SBa		2310000		/* KHz */
#define CPU_DVFS_FREQ2_L_SBa		2220000		/* KHz */
#define CPU_DVFS_FREQ3_L_SBa		2136000		/* KHz */
#define CPU_DVFS_FREQ4_L_SBa		2056000		/* KHz */
#define CPU_DVFS_FREQ5_L_SBa		1992000		/* KHz */
#define CPU_DVFS_FREQ6_L_SBa		1928000		/* KHz */
#define CPU_DVFS_FREQ7_L_SBa		1864000		/* KHz */
#define CPU_DVFS_FREQ8_L_SBa		1800000		/* KHz */
#define CPU_DVFS_FREQ9_L_SBa		1621000		/* KHz */
#define CPU_DVFS_FREQ10_L_SBa		1473000		/* KHz */
#define CPU_DVFS_FREQ11_L_SBa		1325000		/* KHz */
#define CPU_DVFS_FREQ12_L_SBa		1176000		/* KHz */
#define CPU_DVFS_FREQ13_L_SBa		1057000		/* KHz */
#define CPU_DVFS_FREQ14_L_SBa		939000		/* KHz */
#define CPU_DVFS_FREQ15_L_SBa		850000		/* KHz */

/* for DVFS OPP table CCI */
#define CPU_DVFS_FREQ0_CCI_SBa		1400000		/* KHz */
#define CPU_DVFS_FREQ1_CCI_SBa		1353000		/* KHz */
#define CPU_DVFS_FREQ2_CCI_SBa		1306000		/* KHz */
#define CPU_DVFS_FREQ3_CCI_SBa		1260000		/* KHz */
#define CPU_DVFS_FREQ4_CCI_SBa		1190000		/* KHz */
#define CPU_DVFS_FREQ5_CCI_SBa		1120000		/* KHz */
#define CPU_DVFS_FREQ6_CCI_SBa		1030000		/* KHz */
#define CPU_DVFS_FREQ7_CCI_SBa		962000		/* KHz */
#define CPU_DVFS_FREQ8_CCI_SBa		894000		/* KHz */
#define CPU_DVFS_FREQ9_CCI_SBa		827000		/* KHz */
#define CPU_DVFS_FREQ10_CCI_SBa		737000		/* KHz */
#define CPU_DVFS_FREQ11_CCI_SBa		669000		/* KHz */
#define CPU_DVFS_FREQ12_CCI_SBa		602000		/* KHz */
#define CPU_DVFS_FREQ13_CCI_SBa		534000		/* KHz */
#define CPU_DVFS_FREQ14_CCI_SBa		467000		/* KHz */
#define CPU_DVFS_FREQ15_CCI_SBa		400000		/* KHz */

/* for DVFS OPP table L */
#define CPU_DVFS_VOLT0_VPROC1_SBa	 95000		/* 10uV */
#define CPU_DVFS_VOLT1_VPROC1_SBa	 92500		/* 10uV */
#define CPU_DVFS_VOLT2_VPROC1_SBa	 90000		/* 10uV */
#define CPU_DVFS_VOLT3_VPROC1_SBa	 87500		/* 10uV */
#define CPU_DVFS_VOLT4_VPROC1_SBa	 85000		/* 10uV */
#define CPU_DVFS_VOLT5_VPROC1_SBa	 82500		/* 10uV */
#define CPU_DVFS_VOLT6_VPROC1_SBa	 80000		/* 10uV */
#define CPU_DVFS_VOLT7_VPROC1_SBa	 76875		/* 10uV */
#define CPU_DVFS_VOLT8_VPROC1_SBa	 74375		/* 10uV */
#define CPU_DVFS_VOLT9_VPROC1_SBa	 71875		/* 10uV */
#define CPU_DVFS_VOLT10_VPROC1_SBa	 69375		/* 10uV */
#define CPU_DVFS_VOLT11_VPROC1_SBa	 66875		/* 10uV */
#define CPU_DVFS_VOLT12_VPROC1_SBa	 65000		/* 10uV */
#define CPU_DVFS_VOLT13_VPROC1_SBa	 63125		/* 10uV */
#define CPU_DVFS_VOLT14_VPROC1_SBa	 61250		/* 10uV */
#define CPU_DVFS_VOLT15_VPROC1_SBa	 60000		/* 10uV */

/* for DVFS OPP table B */
#define CPU_DVFS_VOLT0_VPROC2_SBa	 102500		/* 10uV */
#define CPU_DVFS_VOLT1_VPROC2_SBa	 99375		/* 10uV */
#define CPU_DVFS_VOLT2_VPROC2_SBa	 96250		/* 10uV */
#define CPU_DVFS_VOLT3_VPROC2_SBa	 93125		/* 10uV */
#define CPU_DVFS_VOLT4_VPROC2_SBa	 90000		/* 10uV */
#define CPU_DVFS_VOLT5_VPROC2_SBa	 87500		/* 10uV */
#define CPU_DVFS_VOLT6_VPROC2_SBa	 85000		/* 10uV */
#define CPU_DVFS_VOLT7_VPROC2_SBa	 82500		/* 10uV */
#define CPU_DVFS_VOLT8_VPROC2_SBa	 80000		/* 10uV */
#define CPU_DVFS_VOLT9_VPROC2_SBa	 76250		/* 10uV */
#define CPU_DVFS_VOLT10_VPROC2_SBa	 73125		/* 10uV */
#define CPU_DVFS_VOLT11_VPROC2_SBa	 70000		/* 10uV */
#define CPU_DVFS_VOLT12_VPROC2_SBa	 66875		/* 10uV */
#define CPU_DVFS_VOLT13_VPROC2_SBa	 64375		/* 10uV */
#define CPU_DVFS_VOLT14_VPROC2_SBa	 61875		/* 10uV */
#define CPU_DVFS_VOLT15_VPROC2_SBa	 60000		/* 10uV */

/* for DVFS OPP table CCI */
#define CPU_DVFS_VOLT0_VPROC3_SBa	 95000		/* 10uV */
#define CPU_DVFS_VOLT1_VPROC3_SBa	 92500		/* 10uV */
#define CPU_DVFS_VOLT2_VPROC3_SBa	 90000		/* 10uV */
#define CPU_DVFS_VOLT3_VPROC3_SBa	 87500		/* 10uV */
#define CPU_DVFS_VOLT4_VPROC3_SBa	 83750		/* 10uV */
#define CPU_DVFS_VOLT5_VPROC3_SBa	 80000		/* 10uV */
#define CPU_DVFS_VOLT6_VPROC3_SBa	 77500		/* 10uV */
#define CPU_DVFS_VOLT7_VPROC3_SBa	 75625		/* 10uV */
#define CPU_DVFS_VOLT8_VPROC3_SBa	 73750		/* 10uV */
#define CPU_DVFS_VOLT9_VPROC3_SBa	 71875		/* 10uV */
#define CPU_DVFS_VOLT10_VPROC3_SBa	 69375		/* 10uV */
#define CPU_DVFS_VOLT11_VPROC3_SBa	 67500		/* 10uV */
#define CPU_DVFS_VOLT12_VPROC3_SBa	 65625		/* 10uV */
#define CPU_DVFS_VOLT13_VPROC3_SBa	 63750		/* 10uV */
#define CPU_DVFS_VOLT14_VPROC3_SBa	 61875		/* 10uV */
#define CPU_DVFS_VOLT15_VPROC3_SBa	 60000		/* 10uV */

/* DVFS OPP table */
#define OPP_TBL(cluster, seg, lv, vol)	\
static struct mt_cpu_freq_info opp_tbl_##cluster##_e##lv##_0[] = {        \
	OP                                                                \
(CPU_DVFS_FREQ0_##cluster##_##seg, CPU_DVFS_VOLT0_VPROC##vol##_##seg),   \
	OP                                                               \
(CPU_DVFS_FREQ1_##cluster##_##seg, CPU_DVFS_VOLT1_VPROC##vol##_##seg),   \
	OP                                                               \
(CPU_DVFS_FREQ2_##cluster##_##seg, CPU_DVFS_VOLT2_VPROC##vol##_##seg),   \
	OP                                                               \
(CPU_DVFS_FREQ3_##cluster##_##seg, CPU_DVFS_VOLT3_VPROC##vol##_##seg),   \
	OP                                                               \
(CPU_DVFS_FREQ4_##cluster##_##seg, CPU_DVFS_VOLT4_VPROC##vol##_##seg),   \
	OP                                                               \
(CPU_DVFS_FREQ5_##cluster##_##seg, CPU_DVFS_VOLT5_VPROC##vol##_##seg),   \
	OP                                                               \
(CPU_DVFS_FREQ6_##cluster##_##seg, CPU_DVFS_VOLT6_VPROC##vol##_##seg),   \
	OP                                                               \
(CPU_DVFS_FREQ7_##cluster##_##seg, CPU_DVFS_VOLT7_VPROC##vol##_##seg),   \
	OP                                                               \
(CPU_DVFS_FREQ8_##cluster##_##seg, CPU_DVFS_VOLT8_VPROC##vol##_##seg),   \
	OP                                                               \
(CPU_DVFS_FREQ9_##cluster##_##seg, CPU_DVFS_VOLT9_VPROC##vol##_##seg),   \
	OP                                                                \
(CPU_DVFS_FREQ10_##cluster##_##seg, CPU_DVFS_VOLT10_VPROC##vol##_##seg), \
	OP                                                               \
(CPU_DVFS_FREQ11_##cluster##_##seg, CPU_DVFS_VOLT11_VPROC##vol##_##seg), \
	OP                                                               \
(CPU_DVFS_FREQ12_##cluster##_##seg, CPU_DVFS_VOLT12_VPROC##vol##_##seg), \
	OP                                                               \
(CPU_DVFS_FREQ13_##cluster##_##seg, CPU_DVFS_VOLT13_VPROC##vol##_##seg), \
	OP                                                               \
(CPU_DVFS_FREQ14_##cluster##_##seg, CPU_DVFS_VOLT14_VPROC##vol##_##seg), \
	OP                                                               \
(CPU_DVFS_FREQ15_##cluster##_##seg, CPU_DVFS_VOLT15_VPROC##vol##_##seg), \
}

OPP_TBL(LL,   FY, 0, 1); /* opp_tbl_LL_e0_0   */
OPP_TBL(L,  FY, 0, 2); /* opp_tbl_L_e0_0  */
OPP_TBL(CCI, FY, 0, 3); /* opp_tbl_CCI_e0_0 */

OPP_TBL(LL,  SBa, 1, 1); /* opp_tbl_LL_e1_0   */
OPP_TBL(L,  SBa, 1, 2); /* opp_tbl_L_e1_0  */
OPP_TBL(CCI, SBa, 1, 3); /* opp_tbl_CCI_e1_0 */

struct opp_tbl_info opp_tbls[NR_MT_CPU_DVFS][NUM_CPU_LEVEL] = {
	/* LL */
	{
		[CPU_LEVEL_0] = { opp_tbl_LL_e0_0,
				ARRAY_SIZE(opp_tbl_LL_e0_0) },
		[CPU_LEVEL_1] = { opp_tbl_LL_e1_0,
				ARRAY_SIZE(opp_tbl_LL_e1_0) },
	},
	/* L */
	{
		[CPU_LEVEL_0] = { opp_tbl_L_e0_0,
				ARRAY_SIZE(opp_tbl_L_e0_0) },
		[CPU_LEVEL_1] = { opp_tbl_L_e1_0,
				ARRAY_SIZE(opp_tbl_L_e1_0) },
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
static struct mt_cpu_freq_method opp_tbl_method_LL_FY[] = {	/* FY */
	/* POS,	CLK */
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
	FP(2,	1),
};

static struct mt_cpu_freq_method opp_tbl_method_L_FY[] = {	/* FY */
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
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
};

static struct mt_cpu_freq_method opp_tbl_method_CCI_FY[] = {	/* FY */
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
	FP(2,	1),
	FP(2,	2),
	FP(2,	2),
	FP(2,	2),
	FP(2,	2),
	FP(2,	2),
	FP(2,	2),
};

static struct mt_cpu_freq_method opp_tbl_method_LL_SBa[] = {	/* SBa */
	/* POS,	CLK */
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
	FP(2,	1),
};

static struct mt_cpu_freq_method opp_tbl_method_L_SBa[] = {	/* SBa */
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
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
};

static struct mt_cpu_freq_method opp_tbl_method_CCI_SBa[] = {	/* SBa */
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
	FP(2,	1),
	FP(2,	2),
	FP(2,	2),
	FP(2,	2),
	FP(2,	2),
	FP(2,	2),
	FP(2,	2),
};


struct opp_tbl_m_info opp_tbls_m[NR_MT_CPU_DVFS][NUM_CPU_LEVEL] = {
	/* LL */
	{
		[CPU_LEVEL_0] = { opp_tbl_method_LL_FY },
		[CPU_LEVEL_1] = { opp_tbl_method_LL_SBa },
	},
	/* L */
	{
		[CPU_LEVEL_0] = { opp_tbl_method_L_FY },
		[CPU_LEVEL_1] = { opp_tbl_method_L_SBa },
	},
	/* CCI */
	{
		[CPU_LEVEL_0] = { opp_tbl_method_CCI_FY },
		[CPU_LEVEL_1] = { opp_tbl_method_CCI_SBa },
	},
};
