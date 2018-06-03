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

/* for DVFS OPP table LL B- */
#define CPU_DVFS_FREQ0_LL_FY		1495000		/* KHz */
#define CPU_DVFS_FREQ1_LL_FY		1443000		/* KHz */
#define CPU_DVFS_FREQ2_LL_FY		1404000		/* KHz */
#define CPU_DVFS_FREQ3_LL_FY		1378000		/* KHz */
#define CPU_DVFS_FREQ4_LL_FY		1326000		/* KHz */
#define CPU_DVFS_FREQ5_LL_FY		1261000		/* KHz */
#define CPU_DVFS_FREQ6_LL_FY		1196000		/* KHz */
#define CPU_DVFS_FREQ7_LL_FY		1144000		/* KHz */
#define CPU_DVFS_FREQ8_LL_FY		1053000		/* KHz */
#define CPU_DVFS_FREQ9_LL_FY		1001000		/* KHz */
#define CPU_DVFS_FREQ10_LL_FY		 910000		/* KHz */
#define CPU_DVFS_FREQ11_LL_FY		 819000		/* KHz */
#define CPU_DVFS_FREQ12_LL_FY		 715000	/* KHz */
#define CPU_DVFS_FREQ13_LL_FY		 611000	/* KHz */
#define CPU_DVFS_FREQ14_LL_FY		 481000	/* KHz */
#define CPU_DVFS_FREQ15_LL_FY		 338000	/* KHz */

/* for DVFS OPP table L */
#define CPU_DVFS_FREQ0_L_FY			2002000		/* KHz */
#define CPU_DVFS_FREQ1_L_FY			1976000		/* KHz */
#define CPU_DVFS_FREQ2_L_FY			1950000		/* KHz */
#define CPU_DVFS_FREQ3_L_FY			1937000		/* KHz */
#define CPU_DVFS_FREQ4_L_FY			1885000		/* KHz */
#define CPU_DVFS_FREQ5_L_FY			1807000		/* KHz */
#define CPU_DVFS_FREQ6_L_FY			1716000		/* KHz */
#define CPU_DVFS_FREQ7_L_FY			1638000		/* KHz */
#define CPU_DVFS_FREQ8_L_FY			1521000		/* KHz */
#define CPU_DVFS_FREQ9_L_FY			1443000		/* KHz */
#define CPU_DVFS_FREQ10_L_FY		1313000		/* KHz */
#define CPU_DVFS_FREQ11_L_FY		1183000		/* KHz */
#define CPU_DVFS_FREQ12_L_FY		1027000		/* KHz */
#define CPU_DVFS_FREQ13_L_FY		 884000		/* KHz */
#define CPU_DVFS_FREQ14_L_FY		 715000		/* KHz */
#define CPU_DVFS_FREQ15_L_FY		 520000		/* KHz */

/* for DVFS OPP table CCI */
#define CPU_DVFS_FREQ0_CCI_FY		 897000		/* KHz */
#define CPU_DVFS_FREQ1_CCI_FY		 871000		/* KHz */
#define CPU_DVFS_FREQ2_CCI_FY		 832000		/* KHz */
#define CPU_DVFS_FREQ3_CCI_FY		 819000		/* KHz */
#define CPU_DVFS_FREQ4_CCI_FY		 780000		/* KHz */
#define CPU_DVFS_FREQ5_CCI_FY		 754000		/* KHz */
#define CPU_DVFS_FREQ6_CCI_FY		 715000		/* KHz */
#define CPU_DVFS_FREQ7_CCI_FY		 689000		/* KHz */
#define CPU_DVFS_FREQ8_CCI_FY		 637000		/* KHz */
#define CPU_DVFS_FREQ9_CCI_FY		 611000		/* KHz */
#define CPU_DVFS_FREQ10_CCI_FY		 559000		/* KHz */
#define CPU_DVFS_FREQ11_CCI_FY		 507000		/* KHz */
#define CPU_DVFS_FREQ12_CCI_FY		 442000		/* KHz */
#define CPU_DVFS_FREQ13_CCI_FY		 377000		/* KHz */
#define CPU_DVFS_FREQ14_CCI_FY		 286000		/* KHz */
#define CPU_DVFS_FREQ15_CCI_FY		 195000		/* KHz */

/* for DVFS OPP table LL|L|CCI */
#define CPU_DVFS_VOLT0_VPROC1_FY	 96250		/* 10uV */
#define CPU_DVFS_VOLT1_VPROC1_FY	 94375		/* 10uV */
#define CPU_DVFS_VOLT2_VPROC1_FY	 92500		/* 10uV */
#define CPU_DVFS_VOLT3_VPROC1_FY	 91250		/* 10uV */
#define CPU_DVFS_VOLT4_VPROC1_FY	 89375		/* 10uV */
#define CPU_DVFS_VOLT5_VPROC1_FY	 87500		/* 10uV */
#define CPU_DVFS_VOLT6_VPROC1_FY	 85625		/* 10uV */
#define CPU_DVFS_VOLT7_VPROC1_FY	 83750		/* 10uV */
#define CPU_DVFS_VOLT8_VPROC1_FY	 81250		/* 10uV */
#define CPU_DVFS_VOLT9_VPROC1_FY	 79375		/* 10uV */
#define CPU_DVFS_VOLT10_VPROC1_FY	 76875		/* 10uV */
#define CPU_DVFS_VOLT11_VPROC1_FY	 74375		/* 10uV */
#define CPU_DVFS_VOLT12_VPROC1_FY	 71250		/* 10uV */
#define CPU_DVFS_VOLT13_VPROC1_FY	 68125		/* 10uV */
#define CPU_DVFS_VOLT14_VPROC1_FY	 64375		/* 10uV */
#define CPU_DVFS_VOLT15_VPROC1_FY	 60000		/* 10uV */

/* for DVFS OPP table LL B/B+ FY*/
#define CPU_DVFS_FREQ0_LL_FY2		1690000		/* KHz */
#define CPU_DVFS_FREQ1_LL_FY2		1677000		/* KHz */
#define CPU_DVFS_FREQ2_LL_FY2		1651000		/* KHz */
#define CPU_DVFS_FREQ3_LL_FY2		1638000		/* KHz */
#define CPU_DVFS_FREQ4_LL_FY2		1573000		/* KHz */
#define CPU_DVFS_FREQ5_LL_FY2		1521000		/* KHz */
#define CPU_DVFS_FREQ6_LL_FY2		1456000		/* KHz */
#define CPU_DVFS_FREQ7_LL_FY2		1404000		/* KHz */
#define CPU_DVFS_FREQ8_LL_FY2		1326000		/* KHz */
#define CPU_DVFS_FREQ9_LL_FY2		1222000		/* KHz */
#define CPU_DVFS_FREQ10_LL_FY2		1144000		/* KHz */
#define CPU_DVFS_FREQ11_LL_FY2		1014000		/* KHz */
#define CPU_DVFS_FREQ12_LL_FY2		 884000		/* KHz */
#define CPU_DVFS_FREQ13_LL_FY2		 754000		/* KHz */
#define CPU_DVFS_FREQ14_LL_FY2		 572000		/* KHz */
#define CPU_DVFS_FREQ15_LL_FY2		 338000		/* KHz */

/* for DVFS OPP table L B+ FY 2457 */
#define CPU_DVFS_FREQ0_L_FY2		2340000		/* KHz */
#define CPU_DVFS_FREQ1_L_FY2		2288000		/* KHz */
#define CPU_DVFS_FREQ2_L_FY2		2249000		/* KHz */
#define CPU_DVFS_FREQ3_L_FY2		2197000		/* KHz */
#define CPU_DVFS_FREQ4_L_FY2		2145000		/* KHz */
#define CPU_DVFS_FREQ5_L_FY2		2080000		/* KHz */
#define CPU_DVFS_FREQ6_L_FY2		2028000		/* KHz */
#define CPU_DVFS_FREQ7_L_FY2		1976000		/* KHz */
#define CPU_DVFS_FREQ8_L_FY2		1885000		/* KHz */
#define CPU_DVFS_FREQ9_L_FY2		1755000		/* KHz */
#define CPU_DVFS_FREQ10_L_FY2		1638000		/* KHz */
#define CPU_DVFS_FREQ11_L_FY2		1469000		/* KHz */
#define CPU_DVFS_FREQ12_L_FY2		1287000		/* KHz */
#define CPU_DVFS_FREQ13_L_FY2		1092000		/* KHz */
#define CPU_DVFS_FREQ14_L_FY2		 832000		/* KHz */
#define CPU_DVFS_FREQ15_L_FY2		 520000		/* KHz */

/* for DVFS OPP table CCI */
#define CPU_DVFS_FREQ0_CCI_FY2		1040000		/* KHz */
#define CPU_DVFS_FREQ1_CCI_FY2		1027000		/* KHz */
#define CPU_DVFS_FREQ2_CCI_FY2		1014000		/* KHz */
#define CPU_DVFS_FREQ3_CCI_FY2		1001000		/* KHz */
#define CPU_DVFS_FREQ4_CCI_FY2		 962000		/* KHz */
#define CPU_DVFS_FREQ5_CCI_FY2		 910000		/* KHz */
#define CPU_DVFS_FREQ6_CCI_FY2		 871000		/* KHz */
#define CPU_DVFS_FREQ7_CCI_FY2		 832000		/* KHz */
#define CPU_DVFS_FREQ8_CCI_FY2		 780000		/* KHz */
#define CPU_DVFS_FREQ9_CCI_FY2		 728000		/* KHz */
#define CPU_DVFS_FREQ10_CCI_FY2		 689000		/* KHz */
#define CPU_DVFS_FREQ11_CCI_FY2		 624000		/* KHz */
#define CPU_DVFS_FREQ12_CCI_FY2		 546000		/* KHz */
#define CPU_DVFS_FREQ13_CCI_FY2		 468000		/* KHz */
#define CPU_DVFS_FREQ14_CCI_FY2		 351000		/* KHz */
#define CPU_DVFS_FREQ15_CCI_FY2		 195000		/* KHz */

/* for DVFS OPP table LL|L|CCI */
#define CPU_DVFS_VOLT0_VPROC1_FY2	 111875		/* 10uV */
#define CPU_DVFS_VOLT1_VPROC1_FY2	 108750		/* 10uV */
#define CPU_DVFS_VOLT2_VPROC1_FY2	 105625		/* 10uV */
#define CPU_DVFS_VOLT3_VPROC1_FY2	 102500		/* 10uV */
#define CPU_DVFS_VOLT4_VPROC1_FY2	 100000		/* 10uV */
#define CPU_DVFS_VOLT5_VPROC1_FY2	 97500		/* 10uV */
#define CPU_DVFS_VOLT6_VPROC1_FY2	 95000		/* 10uV */
#define CPU_DVFS_VOLT7_VPROC1_FY2	 92500		/* 10uV */
#define CPU_DVFS_VOLT8_VPROC1_FY2	 89375		/* 10uV */
#define CPU_DVFS_VOLT9_VPROC1_FY2	 86250		/* 10uV */
#define CPU_DVFS_VOLT10_VPROC1_FY	 83750		/* 10uV */
#define CPU_DVFS_VOLT11_VPROC1_FY2	 80000		/* 10uV */
#define CPU_DVFS_VOLT12_VPROC1_FY2	 76250		/* 10uV */
#define CPU_DVFS_VOLT13_VPROC1_FY2	 72500		/* 10uV */
#define CPU_DVFS_VOLT14_VPROC1_FY2	 66875		/* 10uV */
#define CPU_DVFS_VOLT15_VPROC1_FY2	 60000		/* 10uV */

/* for DVFS OPP table LL B+ SB */
#define CPU_DVFS_FREQ0_LL_FY3		1885000		/* KHz */
#define CPU_DVFS_FREQ1_LL_FY3		1846000		/* KHz */
#define CPU_DVFS_FREQ2_LL_FY3		1768000		/* KHz */
#define CPU_DVFS_FREQ3_LL_FY3		1729000		/* KHz */
#define CPU_DVFS_FREQ4_LL_FY3		1651000		/* KHz */
#define CPU_DVFS_FREQ5_LL_FY3		1599000		/* KHz */
#define CPU_DVFS_FREQ6_LL_FY3		1534000		/* KHz */
#define CPU_DVFS_FREQ7_LL_FY3		1456000		/* KHz */
#define CPU_DVFS_FREQ8_LL_FY3		1391000		/* KHz */
#define CPU_DVFS_FREQ9_LL_FY3		1300000		/* KHz */
#define CPU_DVFS_FREQ10_LL_FY3		1196000		/* KHz */
#define CPU_DVFS_FREQ11_LL_FY3		1079000		/* KHz */
#define CPU_DVFS_FREQ12_LL_FY3		 949000		/* KHz */
#define CPU_DVFS_FREQ13_LL_FY3		 780000		/* KHz */
#define CPU_DVFS_FREQ14_LL_FY3		 585000		/* KHz */
#define CPU_DVFS_FREQ15_LL_FY3		 338000		/* KHz */

/* for DVFS OPP table L B+ SB 2626*/
#define CPU_DVFS_FREQ0_L_FY3		2496000		/* KHz */
#define CPU_DVFS_FREQ1_L_FY3		2431000		/* KHz */
#define CPU_DVFS_FREQ2_L_FY3		2353000		/* KHz */
#define CPU_DVFS_FREQ3_L_FY3		2301000		/* KHz */
#define CPU_DVFS_FREQ4_L_FY3		2223000		/* KHz */
#define CPU_DVFS_FREQ5_L_FY3		2158000		/* KHz */
#define CPU_DVFS_FREQ6_L_FY3		2106000		/* KHz */
#define CPU_DVFS_FREQ7_L_FY3		2028000		/* KHz */
#define CPU_DVFS_FREQ8_L_FY3		1963000		/* KHz */
#define CPU_DVFS_FREQ9_L_FY3		1859000		/* KHz */
#define CPU_DVFS_FREQ10_L_FY3		1716000		/* KHz */
#define CPU_DVFS_FREQ11_L_FY3		1547000		/* KHz */
#define CPU_DVFS_FREQ12_L_FY3		1378000		/* KHz */
#define CPU_DVFS_FREQ13_L_FY3		1131000		/* KHz */
#define CPU_DVFS_FREQ14_L_FY3		 858000		/* KHz */
#define CPU_DVFS_FREQ15_L_FY3		 520000		/* KHz */

/* for DVFS OPP table CCI */
#define CPU_DVFS_FREQ0_CCI_FY3		1092000		/* KHz */
#define CPU_DVFS_FREQ1_CCI_FY3		1079000		/* KHz */
#define CPU_DVFS_FREQ2_CCI_FY3		1053000		/* KHz */
#define CPU_DVFS_FREQ3_CCI_FY3		1027000		/* KHz */
#define CPU_DVFS_FREQ4_CCI_FY3		1001000		/* KHz */
#define CPU_DVFS_FREQ5_CCI_FY3		 962000		/* KHz */
#define CPU_DVFS_FREQ6_CCI_FY3		 923000		/* KHz */
#define CPU_DVFS_FREQ7_CCI_FY3		 871000		/* KHz */
#define CPU_DVFS_FREQ8_CCI_FY3		 819000		/* KHz */
#define CPU_DVFS_FREQ9_CCI_FY3		 767000		/* KHz */
#define CPU_DVFS_FREQ10_CCI_FY3		 715000		/* KHz */
#define CPU_DVFS_FREQ11_CCI_FY3		 650000		/* KHz */
#define CPU_DVFS_FREQ12_CCI_FY3		 585000		/* KHz */
#define CPU_DVFS_FREQ13_CCI_FY3		 481000		/* KHz */
#define CPU_DVFS_FREQ14_CCI_FY3		 364000		/* KHz */
#define CPU_DVFS_FREQ15_CCI_FY3		 195000		/* KHz */

/* for DVFS OPP table LL|L|CCI */
#define CPU_DVFS_VOLT0_VPROC1_FY3	 111875		/* 10uV */
#define CPU_DVFS_VOLT1_VPROC1_FY3	 110000		/* 10uV */
#define CPU_DVFS_VOLT2_VPROC1_FY3	 107500		/* 10uV */
#define CPU_DVFS_VOLT3_VPROC1_FY3	 105625		/* 10uV */
#define CPU_DVFS_VOLT4_VPROC1_FY3	 103125		/* 10uV */
#define CPU_DVFS_VOLT5_VPROC1_FY3	 100625		/* 10uV */
#define CPU_DVFS_VOLT6_VPROC1_FY3	 98125		/* 10uV */
#define CPU_DVFS_VOLT7_VPROC1_FY3	 95000		/* 10uV */
#define CPU_DVFS_VOLT8_VPROC1_FY3	 91875		/* 10uV */
#define CPU_DVFS_VOLT9_VPROC1_FY3	 88750		/* 10uV */
#define CPU_DVFS_VOLT10_VPROC1_FY3	 85625		/* 10uV */
#define CPU_DVFS_VOLT11_VPROC1_FY3	 81875		/* 10uV */
#define CPU_DVFS_VOLT12_VPROC1_FY3	 78125		/* 10uV */
#define CPU_DVFS_VOLT13_VPROC1_FY3	 73125		/* 10uV */
#define CPU_DVFS_VOLT14_VPROC1_FY3	 67500		/* 10uV */
#define CPU_DVFS_VOLT15_VPROC1_FY3	 60000		/* 10uV */

/* DVFS OPP table */
#define OPP_TBL(cluster, seg, lv, vol)	\
static struct mt_cpu_freq_info opp_tbl_##cluster##_e##lv##_0[] = {	\
	OP(CPU_DVFS_FREQ0_##cluster##_##seg, CPU_DVFS_VOLT0_VPROC##vol##_##seg),	\
	OP(CPU_DVFS_FREQ1_##cluster##_##seg, CPU_DVFS_VOLT1_VPROC##vol##_##seg),	\
	OP(CPU_DVFS_FREQ2_##cluster##_##seg, CPU_DVFS_VOLT2_VPROC##vol##_##seg),	\
	OP(CPU_DVFS_FREQ3_##cluster##_##seg, CPU_DVFS_VOLT3_VPROC##vol##_##seg),	\
	OP(CPU_DVFS_FREQ4_##cluster##_##seg, CPU_DVFS_VOLT4_VPROC##vol##_##seg),	\
	OP(CPU_DVFS_FREQ5_##cluster##_##seg, CPU_DVFS_VOLT5_VPROC##vol##_##seg),	\
	OP(CPU_DVFS_FREQ6_##cluster##_##seg, CPU_DVFS_VOLT6_VPROC##vol##_##seg),	\
	OP(CPU_DVFS_FREQ7_##cluster##_##seg, CPU_DVFS_VOLT7_VPROC##vol##_##seg),	\
	OP(CPU_DVFS_FREQ8_##cluster##_##seg, CPU_DVFS_VOLT8_VPROC##vol##_##seg),	\
	OP(CPU_DVFS_FREQ9_##cluster##_##seg, CPU_DVFS_VOLT9_VPROC##vol##_##seg),	\
	OP(CPU_DVFS_FREQ10_##cluster##_##seg, CPU_DVFS_VOLT10_VPROC##vol##_##seg),	\
	OP(CPU_DVFS_FREQ11_##cluster##_##seg, CPU_DVFS_VOLT11_VPROC##vol##_##seg),	\
	OP(CPU_DVFS_FREQ12_##cluster##_##seg, CPU_DVFS_VOLT12_VPROC##vol##_##seg),	\
	OP(CPU_DVFS_FREQ13_##cluster##_##seg, CPU_DVFS_VOLT13_VPROC##vol##_##seg),	\
	OP(CPU_DVFS_FREQ14_##cluster##_##seg, CPU_DVFS_VOLT14_VPROC##vol##_##seg),	\
	OP(CPU_DVFS_FREQ15_##cluster##_##seg, CPU_DVFS_VOLT15_VPROC##vol##_##seg),	\
}

OPP_TBL(LL, FY, 0, 1);
OPP_TBL(L, FY, 0, 1);
OPP_TBL(CCI, FY, 0, 1);

OPP_TBL(LL, FY2, 1, 1);
OPP_TBL(L, FY2, 1, 1);
OPP_TBL(CCI, FY2, 1, 1);

OPP_TBL(LL, FY3, 2, 1);
OPP_TBL(L, FY3, 2, 1);
OPP_TBL(CCI, FY3, 2, 1);

struct opp_tbl_info opp_tbls[NR_MT_CPU_DVFS][NUM_CPU_LEVEL] = {		/* v1.1 */
	/* LL */
	{
		[CPU_LEVEL_0] = { opp_tbl_LL_e0_0, ARRAY_SIZE(opp_tbl_LL_e0_0) },
		[CPU_LEVEL_1] = { opp_tbl_LL_e1_0, ARRAY_SIZE(opp_tbl_LL_e1_0) },
		[CPU_LEVEL_2] = { opp_tbl_LL_e2_0, ARRAY_SIZE(opp_tbl_LL_e2_0) },
	},
	/* L */
	{
		[CPU_LEVEL_0] = { opp_tbl_L_e0_0, ARRAY_SIZE(opp_tbl_L_e0_0) },
		[CPU_LEVEL_1] = { opp_tbl_L_e1_0, ARRAY_SIZE(opp_tbl_L_e1_0) },
		[CPU_LEVEL_2] = { opp_tbl_L_e2_0, ARRAY_SIZE(opp_tbl_L_e2_0) },
	},
	/* CCI */
	{
		[CPU_LEVEL_0] = { opp_tbl_CCI_e0_0, ARRAY_SIZE(opp_tbl_CCI_e0_0) },
		[CPU_LEVEL_1] = { opp_tbl_CCI_e1_0, ARRAY_SIZE(opp_tbl_CCI_e1_0) },
		[CPU_LEVEL_2] = { opp_tbl_CCI_e2_0, ARRAY_SIZE(opp_tbl_CCI_e2_0) },
	},
};

/* 16 steps OPP table */
static struct mt_cpu_freq_method opp_tbl_method_LL_e0[] = {	/* B- */
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
	FP(2,	1),
	FP(2,	1),
	FP(2,	2),
	FP(2,	2),
	FP(2,	2),
	FP(2,	4),
};

static struct mt_cpu_freq_method opp_tbl_method_L_e0[] = {	/* FY */
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
	FP(2,	2),
	FP(2,	2),
};

static struct mt_cpu_freq_method opp_tbl_method_CCI_e0[] = {	/* FY */
	/* POS,	CLK */
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
	FP(2,	2),
	FP(2,	2),
	FP(2,	4),
	FP(2,	4),
};

static struct mt_cpu_freq_method opp_tbl_method_LL_e1[] = {	/* B */
	/* POS,	CLK */
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
	FP(2,	2),
	FP(2,	4),
};

static struct mt_cpu_freq_method opp_tbl_method_L_e1[] = {	/* FY2 */
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
	FP(2,	2),
};

static struct mt_cpu_freq_method opp_tbl_method_CCI_e1[] = {	/* FY2 */
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
	FP(2,	2),
	FP(2,	2),
	FP(2,	2),
	FP(2,	2),
	FP(2,	2),
	FP(2,	4),
	FP(2,	4),
};

static struct mt_cpu_freq_method opp_tbl_method_LL_e2[] = {	/* B+ */
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
	FP(2,	2),
	FP(2,	4),
};

static struct mt_cpu_freq_method opp_tbl_method_L_e2[] = {	/* FY3 */
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
	FP(1,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	2),
};

static struct mt_cpu_freq_method opp_tbl_method_CCI_e2[] = {	/* FY3 */
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
	FP(2,	4),
	FP(2,	4),
};

struct opp_tbl_m_info opp_tbls_m[NR_MT_CPU_DVFS][NUM_CPU_LEVEL] = {	/* v1.1 */
	/* LL */
	{
		[CPU_LEVEL_0] = { opp_tbl_method_LL_e0 },
		[CPU_LEVEL_1] = { opp_tbl_method_LL_e1 },
		[CPU_LEVEL_2] = { opp_tbl_method_LL_e2 },
	},
	/* L */
	{
		[CPU_LEVEL_0] = { opp_tbl_method_L_e0 },
		[CPU_LEVEL_1] = { opp_tbl_method_L_e1 },
		[CPU_LEVEL_2] = { opp_tbl_method_L_e2 },
	},
	/* CCI */
	{
		[CPU_LEVEL_0] = { opp_tbl_method_CCI_e0 },
		[CPU_LEVEL_1] = { opp_tbl_method_CCI_e1 },
		[CPU_LEVEL_2] = { opp_tbl_method_CCI_e2 },
	},
};
