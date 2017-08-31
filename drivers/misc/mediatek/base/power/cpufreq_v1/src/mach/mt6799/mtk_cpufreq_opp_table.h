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

#include "mtk_cpufreq_internal.h"

/* for DVFS OPP table LL/FY */
#define CPU_DVFS_FREQ0_LL_FY    (1430000)
#define CPU_DVFS_FREQ1_LL_FY    (1365000)
#define CPU_DVFS_FREQ2_LL_FY    (1326000)
#define CPU_DVFS_FREQ3_LL_FY    (1261000)
#define CPU_DVFS_FREQ4_LL_FY    (1196000)
#define CPU_DVFS_FREQ5_LL_FY    (1131000)
#define CPU_DVFS_FREQ6_LL_FY    (1066000)
#define CPU_DVFS_FREQ7_LL_FY    (1001000)
#define CPU_DVFS_FREQ8_LL_FY    (923000)
#define CPU_DVFS_FREQ9_LL_FY    (858000)
#define CPU_DVFS_FREQ10_LL_FY    (780000)
#define CPU_DVFS_FREQ11_LL_FY    (689000)
#define CPU_DVFS_FREQ12_LL_FY    (611000)
#define CPU_DVFS_FREQ13_LL_FY    (520000)
#define CPU_DVFS_FREQ14_LL_FY    (429000)
#define CPU_DVFS_FREQ15_LL_FY    (325000)

/* for DVFS OPP table L/FY */
#define CPU_DVFS_FREQ0_L_FY    (1625000)	/* KHz */
#define CPU_DVFS_FREQ1_L_FY    (1534000)	/* KHz */
#define CPU_DVFS_FREQ2_L_FY    (1482000)	/* KHz */
#define CPU_DVFS_FREQ3_L_FY    (1404000)	/* KHz */
#define CPU_DVFS_FREQ4_L_FY    (1326000)	/* KHz */
#define CPU_DVFS_FREQ5_L_FY    (1248000)	/* KHz */
#define CPU_DVFS_FREQ6_L_FY    (1157000)	/* KHz */
#define CPU_DVFS_FREQ7_L_FY    (1079000)	/* KHz */
#define CPU_DVFS_FREQ8_L_FY    (988000)	/* KHz */
#define CPU_DVFS_FREQ9_L_FY     (897000)	/* KHz */
#define CPU_DVFS_FREQ10_L_FY    (806000)	/* KHz */
#define CPU_DVFS_FREQ11_L_FY    (715000)	/* KHz */
#define CPU_DVFS_FREQ12_L_FY    (624000)	/* KHz */
#define CPU_DVFS_FREQ13_L_FY    (533000)	/* KHz */
#define CPU_DVFS_FREQ14_L_FY    (442000)	/* KHz */
#define CPU_DVFS_FREQ15_L_FY    (325000)	/* KHz */

/* for DVFS OPP table CCI/FY */
#define CPU_DVFS_FREQ0_CCI_FY    (910000)	/* KHz */
#define CPU_DVFS_FREQ1_CCI_FY    (858000)	/* KHz */
#define CPU_DVFS_FREQ2_CCI_FY    (819000)	/* KHz */
#define CPU_DVFS_FREQ3_CCI_FY    (780000)	/* KHz */
#define CPU_DVFS_FREQ4_CCI_FY    (728000)	/* KHz */
#define CPU_DVFS_FREQ5_CCI_FY    (689000)	/* KHz */
#define CPU_DVFS_FREQ6_CCI_FY    (637000)	/* KHz */
#define CPU_DVFS_FREQ7_CCI_FY    (598000)	/* KHz */
#define CPU_DVFS_FREQ8_CCI_FY    (533000)	/* KHz */
#define CPU_DVFS_FREQ9_CCI_FY    (494000)	/* KHz */
#define CPU_DVFS_FREQ10_CCI_FY    (429000)	/* KHz */
#define CPU_DVFS_FREQ11_CCI_FY    (377000)	/* KHz */
#define CPU_DVFS_FREQ12_CCI_FY    (312000)	/* KHz */
#define CPU_DVFS_FREQ13_CCI_FY    (260000)	/* KHz */
#define CPU_DVFS_FREQ14_CCI_FY    (195000)	/* KHz */
#define CPU_DVFS_FREQ15_CCI_FY    (117000)	/* KHz */

/* for DVFS OPP table B/FY */
#define CPU_DVFS_FREQ0_B_FY    (1885000)	/* KHz */
#define CPU_DVFS_FREQ1_B_FY    (1768000)	/* KHz */
#define CPU_DVFS_FREQ2_B_FY    (1690000)	/* KHz */
#define CPU_DVFS_FREQ3_B_FY    (1586000)	/* KHz */
#define CPU_DVFS_FREQ4_B_FY    (1482000)	/* KHz */
#define CPU_DVFS_FREQ5_B_FY    (1378000)	/* KHz */
#define CPU_DVFS_FREQ6_B_FY    (1261000)	/* KHz */
#define CPU_DVFS_FREQ7_B_FY    (1157000)	/* KHz */
#define CPU_DVFS_FREQ8_B_FY    (1027000)	/* KHz */
#define CPU_DVFS_FREQ9_B_FY     (923000)	/* KHz */
#define CPU_DVFS_FREQ10_B_FY    (819000)	/* KHz */
#define CPU_DVFS_FREQ11_B_FY    (728000)	/* KHz */
#define CPU_DVFS_FREQ12_B_FY    (637000)	/* KHz */
#define CPU_DVFS_FREQ13_B_FY    (533000)	/* KHz */
#define CPU_DVFS_FREQ14_B_FY    (442000)	/* KHz */
#define CPU_DVFS_FREQ15_B_FY    (325000)	/* KHz */

/* for DVFS OPP table LL|B|CCI and same as L */
#define CPU_DVFS_VOLT0_VPROC1_FY    (97374)	/* 10MV */
#define CPU_DVFS_VOLT1_VPROC1_FY    (94969)	/* 10MV */
#define CPU_DVFS_VOLT2_VPROC1_FY    (93166)	/* 10MV */
#define CPU_DVFS_VOLT3_VPROC1_FY    (90762)	/* 10MV */
#define CPU_DVFS_VOLT4_VPROC1_FY    (88357)	/* 10MV */
#define CPU_DVFS_VOLT5_VPROC1_FY    (85953)	/* 10MV */
#define CPU_DVFS_VOLT6_VPROC1_FY    (83549)	/* 10MV */
#define CPU_DVFS_VOLT7_VPROC1_FY    (81145)	/* 10MV */
#define CPU_DVFS_VOLT8_VPROC1_FY    (78139)		/* 10MV */
#define CPU_DVFS_VOLT9_VPROC1_FY    (75735)		/* 10MV */
#define CPU_DVFS_VOLT10_VPROC1_FY    (75014)	/* 10MV */
#define CPU_DVFS_VOLT11_VPROC1_FY    (75014)	/* 10MV */
#define CPU_DVFS_VOLT12_VPROC1_FY    (75014)	/* 10MV */
#define CPU_DVFS_VOLT13_VPROC1_FY    (75014)	/* 10MV */
#define CPU_DVFS_VOLT14_VPROC1_FY    (75014)	/* 10MV */
#define CPU_DVFS_VOLT15_VPROC1_FY    (75014)	/* 10MV */

/* for DVFS OPP table LL/SB */
#define CPU_DVFS_FREQ0_LL_SB    (1573000)	/* KHz */
#define CPU_DVFS_FREQ1_LL_SB    (1482000)	/* KHz */
#define CPU_DVFS_FREQ2_LL_SB    (1417000)	/* KHz */
#define CPU_DVFS_FREQ3_LL_SB    (1326000)	/* KHz */
#define CPU_DVFS_FREQ4_LL_SB    (1235000)	/* KHz */
#define CPU_DVFS_FREQ5_LL_SB    (1144000)	/* KHz */
#define CPU_DVFS_FREQ6_LL_SB    (1066000)	/* KHz */
#define CPU_DVFS_FREQ7_LL_SB    (1001000)	/* KHz */
#define CPU_DVFS_FREQ8_LL_SB    (923000)	/* KHz */
#define CPU_DVFS_FREQ9_LL_SB    (858000)	/* KHz */
#define CPU_DVFS_FREQ10_LL_SB    (780000)	/* KHz */
#define CPU_DVFS_FREQ11_LL_SB    (689000)	/* KHz */
#define CPU_DVFS_FREQ12_LL_SB    (611000)	/* KHz */
#define CPU_DVFS_FREQ13_LL_SB    (520000)	/* KHz */
#define CPU_DVFS_FREQ14_LL_SB    (429000)	/* KHz */
#define CPU_DVFS_FREQ15_LL_SB    (325000)	/* KHz */

/* for DVFS OPP table L/SB */
#define CPU_DVFS_FREQ0_L_SB    (1742000)	/* KHz */
#define CPU_DVFS_FREQ1_L_SB    (1638000)	/* KHz */
#define CPU_DVFS_FREQ2_L_SB    (1560000)	/* KHz */
#define CPU_DVFS_FREQ3_L_SB    (1456000)	/* KHz */
#define CPU_DVFS_FREQ4_L_SB    (1352000)	/* KHz */
#define CPU_DVFS_FREQ5_L_SB    (1248000)	/* KHz */
#define CPU_DVFS_FREQ6_L_SB    (1157000)	/* KHz */
#define CPU_DVFS_FREQ7_L_SB    (1079000)	/* KHz */
#define CPU_DVFS_FREQ8_L_SB    (988000)	/* KHz */
#define CPU_DVFS_FREQ9_L_SB    (897000)	/* KHz */
#define CPU_DVFS_FREQ10_L_SB    (806000)	/* KHz */
#define CPU_DVFS_FREQ11_L_SB    (715000)	/* KHz */
#define CPU_DVFS_FREQ12_L_SB    (624000)	/* KHz */
#define CPU_DVFS_FREQ13_L_SB    (533000)	/* KHz */
#define CPU_DVFS_FREQ14_L_SB    (442000)	/* KHz */
#define CPU_DVFS_FREQ15_L_SB    (325000)	/* KHz */

/* for DVFS OPP table CCI/SB */
#define CPU_DVFS_FREQ0_CCI_SB    (1001000)	/* KHz */
#define CPU_DVFS_FREQ1_CCI_SB    (936000)	/* KHz */
#define CPU_DVFS_FREQ2_CCI_SB    (884000)	/* KHz */
#define CPU_DVFS_FREQ3_CCI_SB    (819000)	/* KHz */
#define CPU_DVFS_FREQ4_CCI_SB    (754000)	/* KHz */
#define CPU_DVFS_FREQ5_CCI_SB    (689000)	/* KHz */
#define CPU_DVFS_FREQ6_CCI_SB    (637000)	/* KHz */
#define CPU_DVFS_FREQ7_CCI_SB    (598000)	/* KHz */
#define CPU_DVFS_FREQ8_CCI_SB    (533000)	/* KHz */
#define CPU_DVFS_FREQ9_CCI_SB    (494000)	/* KHz */
#define CPU_DVFS_FREQ10_CCI_SB    (429000)	/* KHz */
#define CPU_DVFS_FREQ11_CCI_SB    (377000)	/* KHz */
#define CPU_DVFS_FREQ12_CCI_SB    (312000)	/* KHz */
#define CPU_DVFS_FREQ13_CCI_SB    (260000)	/* KHz */
#define CPU_DVFS_FREQ14_CCI_SB    (195000)	/* KHz */
#define CPU_DVFS_FREQ15_CCI_SB    (117000)	/* KHz */

/* for DVFS OPP table B/SB */
#define CPU_DVFS_FREQ0_B_SB    (2054000)	/* KHz */
#define CPU_DVFS_FREQ1_B_SB    (1911000)	/* KHz */
#define CPU_DVFS_FREQ2_B_SB    (1807000)	/* KHz */
#define CPU_DVFS_FREQ3_B_SB    (1664000)	/* KHz */
#define CPU_DVFS_FREQ4_B_SB    (1534000)	/* KHz */
#define CPU_DVFS_FREQ5_B_SB    (1391000)	/* KHz */
#define CPU_DVFS_FREQ6_B_SB    (1261000)	/* KHz */
#define CPU_DVFS_FREQ7_B_SB    (1157000)	/* KHz */
#define CPU_DVFS_FREQ8_B_SB    (1027000)	/* KHz */
#define CPU_DVFS_FREQ9_B_SB    (923000)	/* KHz */
#define CPU_DVFS_FREQ10_B_SB    (819000)	/* KHz */
#define CPU_DVFS_FREQ11_B_SB    (728000)	/* KHz */
#define CPU_DVFS_FREQ12_B_SB    (637000)	/* KHz */
#define CPU_DVFS_FREQ13_B_SB    (533000)	/* KHz */
#define CPU_DVFS_FREQ14_B_SB    (442000)	/* KHz */
#define CPU_DVFS_FREQ15_B_SB    (325000)	/* KHz */

/* for DVFS OPP table LL|B|CCI and same as L */
#define CPU_DVFS_VOLT0_VPROC1_SB    (97374)	/* 10MV */
#define CPU_DVFS_VOLT1_VPROC1_SB    (94969)	/* 10MV */
#define CPU_DVFS_VOLT2_VPROC1_SB    (93166)	/* 10MV */
#define CPU_DVFS_VOLT3_VPROC1_SB    (90762)	/* 10MV */
#define CPU_DVFS_VOLT4_VPROC1_SB    (88357)	/* 10MV */
#define CPU_DVFS_VOLT5_VPROC1_SB    (85953)	/* 10MV */
#define CPU_DVFS_VOLT6_VPROC1_SB    (83549)	/* 10MV */
#define CPU_DVFS_VOLT7_VPROC1_SB    (81145)	/* 10MV */
#define CPU_DVFS_VOLT8_VPROC1_SB    (78139)		/* 10MV */
#define CPU_DVFS_VOLT9_VPROC1_SB    (75735)		/* 10MV */
#define CPU_DVFS_VOLT10_VPROC1_SB    (75014)	/* 10MV */
#define CPU_DVFS_VOLT11_VPROC1_SB    (75014)	/* 10MV */
#define CPU_DVFS_VOLT12_VPROC1_SB    (75014)	/* 10MV */
#define CPU_DVFS_VOLT13_VPROC1_SB    (75014)	/* 10MV */
#define CPU_DVFS_VOLT14_VPROC1_SB    (75014)	/* 10MV */
#define CPU_DVFS_VOLT15_VPROC1_SB    (75014)	/* 10MV */

/* DVFS OPP table */
/* LL, FY, 1, 1*/
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

OPP_TBL(LL, FY, 1, 1);
OPP_TBL(L, FY, 1, 1);
OPP_TBL(CCI, FY, 1, 1);
OPP_TBL(B, FY, 1, 1);

OPP_TBL(LL, SB, 2, 1);
OPP_TBL(L, SB, 2, 1);
OPP_TBL(CCI, SB, 2, 1);
OPP_TBL(B, SB, 2, 1);

struct opp_tbl_info opp_tbls[NR_MT_CPU_DVFS][2] = {
	/* LL */
	{
		[CPU_LV_TO_OPP_IDX(CPU_LEVEL_0)] = {opp_tbl_LL_e1_0, ARRAY_SIZE(opp_tbl_LL_e1_0),},
		[CPU_LV_TO_OPP_IDX(CPU_LEVEL_1)] = {opp_tbl_LL_e2_0, ARRAY_SIZE(opp_tbl_LL_e2_0),},
	},
	/* L */
	{
		[CPU_LV_TO_OPP_IDX(CPU_LEVEL_0)] = {opp_tbl_L_e1_0, ARRAY_SIZE(opp_tbl_L_e1_0),},
		[CPU_LV_TO_OPP_IDX(CPU_LEVEL_1)] = {opp_tbl_L_e2_0, ARRAY_SIZE(opp_tbl_L_e2_0),},
	},
	/* B */
	{
		[CPU_LV_TO_OPP_IDX(CPU_LEVEL_0)] = {opp_tbl_B_e1_0, ARRAY_SIZE(opp_tbl_B_e1_0),},
		[CPU_LV_TO_OPP_IDX(CPU_LEVEL_1)] = {opp_tbl_B_e2_0, ARRAY_SIZE(opp_tbl_B_e2_0),},
	},
	/* CCI */
	{
		[CPU_LV_TO_OPP_IDX(CPU_LEVEL_0)] = {opp_tbl_CCI_e1_0, ARRAY_SIZE(opp_tbl_CCI_e1_0),},
		[CPU_LV_TO_OPP_IDX(CPU_LEVEL_1)] = {opp_tbl_CCI_e2_0, ARRAY_SIZE(opp_tbl_CCI_e2_0),},
	},
};

/* 16 steps OPP table*/
static struct mt_cpu_freq_method opp_tbl_method_LL_e1[] = {
	/* Target Frequency, POS, CLK */
	FP(2, 1),
	FP(2, 1),
	FP(2, 1),
	FP(2, 1),
	FP(2, 1),
	FP(2, 1),
	FP(2, 1),
	FP(2, 1),
	FP(4, 1),
	FP(4, 1),
	FP(4, 1),
	FP(4, 1),
	FP(4, 1),
	FP(4, 1),
	FP(4, 2),
	FP(4, 2),
};

static struct mt_cpu_freq_method opp_tbl_method_L_e1[] = {
	/* Target Frequency, POS, CLK */
	FP(2, 1),
	FP(2, 1),
	FP(2, 1),
	FP(2, 1),
	FP(2, 1),
	FP(2, 1),
	FP(2, 1),
	FP(2, 1),
	FP(4, 1),
	FP(4, 1),
	FP(4, 1),
	FP(4, 1),
	FP(4, 1),
	FP(4, 1),
	FP(4, 2),
	FP(4, 2),
};

static struct mt_cpu_freq_method opp_tbl_method_CCI_e1[] = {
	/* Target Frequency,		POS, CLK */
	FP(4, 1),
	FP(4, 1),
	FP(4, 1),
	FP(4, 1),
	FP(4, 1),
	FP(4, 1),
	FP(4, 1),
	FP(4, 1),
	FP(4, 1),
	FP(4, 2),
	FP(4, 2),
	FP(4, 2),
	FP(4, 2),
	FP(4, 2),
	FP(4, 4),
	FP(8, 4),
};

static struct mt_cpu_freq_method opp_tbl_method_B_e1[] = {
	/* Target Frequency,	POS, CLK */
	FP(2, 1),
	FP(2, 1),
	FP(2, 1),
	FP(2, 1),
	FP(2, 1),
	FP(2, 1),
	FP(2, 1),
	FP(2, 1),
	FP(2, 1),
	FP(4, 1),
	FP(4, 1),
	FP(4, 1),
	FP(4, 1),
	FP(4, 1),
	FP(4, 2),
	FP(4, 2),
};

static struct mt_cpu_freq_method opp_tbl_method_LL_e2[] = {
	/* Target Frequency, POS, CLK */
	FP(2, 1),
	FP(2, 1),
	FP(2, 1),
	FP(2, 1),
	FP(2, 1),
	FP(2, 1),
	FP(2, 1),
	FP(2, 1),
	FP(4, 1),
	FP(4, 1),
	FP(4, 1),
	FP(4, 1),
	FP(4, 1),
	FP(4, 1),
	FP(4, 2),
	FP(4, 2),
};

static struct mt_cpu_freq_method opp_tbl_method_L_e2[] = {
	/* Target Frequency, POS, CLK */
	FP(2, 1),
	FP(2, 1),
	FP(2, 1),
	FP(2, 1),
	FP(2, 1),
	FP(2, 1),
	FP(2, 1),
	FP(2, 1),
	FP(4, 1),
	FP(4, 1),
	FP(4, 1),
	FP(4, 1),
	FP(4, 1),
	FP(4, 1),
	FP(4, 2),
	FP(4, 2),
};

static struct mt_cpu_freq_method opp_tbl_method_CCI_e2[] = {
	/* Target Frequency,		POS, CLK */
	FP(2, 1),
	FP(4, 1),
	FP(4, 1),
	FP(4, 1),
	FP(4, 1),
	FP(4, 1),
	FP(4, 1),
	FP(4, 1),
	FP(4, 1),
	FP(4, 2),
	FP(4, 2),
	FP(4, 2),
	FP(4, 2),
	FP(4, 2),
	FP(4, 4),
	FP(8, 4),
};

static struct mt_cpu_freq_method opp_tbl_method_B_e2[] = {
	/* Target Frequency,	POS, CLK */
	FP(1, 1),
	FP(2, 1),
	FP(2, 1),
	FP(2, 1),
	FP(2, 1),
	FP(2, 1),
	FP(2, 1),
	FP(2, 1),
	FP(2, 1),
	FP(4, 1),
	FP(4, 1),
	FP(4, 1),
	FP(4, 1),
	FP(4, 1),
	FP(4, 2),
	FP(4, 2),
};

struct opp_tbl_m_info opp_tbls_m[NR_MT_CPU_DVFS][2] = {
	/* LL */
	{
		[CPU_LV_TO_OPP_IDX(CPU_LEVEL_0)] = {opp_tbl_method_LL_e1,},
		[CPU_LV_TO_OPP_IDX(CPU_LEVEL_1)] = {opp_tbl_method_LL_e2,},
	},
	/* L */
	{
		[CPU_LV_TO_OPP_IDX(CPU_LEVEL_0)] = {opp_tbl_method_L_e1,},
		[CPU_LV_TO_OPP_IDX(CPU_LEVEL_1)] = {opp_tbl_method_L_e2,},
	},
	/* B */
	{
		[CPU_LV_TO_OPP_IDX(CPU_LEVEL_0)] = {opp_tbl_method_B_e1,},
		[CPU_LV_TO_OPP_IDX(CPU_LEVEL_1)] = {opp_tbl_method_B_e2,},
	},
	/* CCI */
	{
		[CPU_LV_TO_OPP_IDX(CPU_LEVEL_0)] = {opp_tbl_method_CCI_e1,},
		[CPU_LV_TO_OPP_IDX(CPU_LEVEL_1)] = {opp_tbl_method_CCI_e2,},
	},
};
