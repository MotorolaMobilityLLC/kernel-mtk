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
#define CPU_DVFS_FREQ0_LL_FY    (1508000)
#define CPU_DVFS_FREQ1_LL_FY    (1443000)
#define CPU_DVFS_FREQ2_LL_FY    (1391000)
#define CPU_DVFS_FREQ3_LL_FY    (1326000)
#define CPU_DVFS_FREQ4_LL_FY    (1261000)
#define CPU_DVFS_FREQ5_LL_FY    (1196000)
#define CPU_DVFS_FREQ6_LL_FY    (1118000)
#define CPU_DVFS_FREQ7_LL_FY    (1053000)
#define CPU_DVFS_FREQ8_LL_FY    (975000)
#define CPU_DVFS_FREQ9_LL_FY    (910000)
#define CPU_DVFS_FREQ10_LL_FY    (819000)
#define CPU_DVFS_FREQ11_LL_FY    (728000)
#define CPU_DVFS_FREQ12_LL_FY    (637000)
#define CPU_DVFS_FREQ13_LL_FY    (546000)
#define CPU_DVFS_FREQ14_LL_FY    (455000)
#define CPU_DVFS_FREQ15_LL_FY    (338000)

/* for DVFS OPP table L/FY */
#define CPU_DVFS_FREQ0_L_FY    (1703000)	/* KHz */
#define CPU_DVFS_FREQ1_L_FY    (1625000)	/* KHz */
#define CPU_DVFS_FREQ2_L_FY    (1560000)	/* KHz */
#define CPU_DVFS_FREQ3_L_FY    (1469000)	/* KHz */
#define CPU_DVFS_FREQ4_L_FY    (1391000)	/* KHz */
#define CPU_DVFS_FREQ5_L_FY    (1313000)	/* KHz */
#define CPU_DVFS_FREQ6_L_FY    (1222000)	/* KHz */
#define CPU_DVFS_FREQ7_L_FY    (1144000)	/* KHz */
#define CPU_DVFS_FREQ8_L_FY    (1040000)	/* KHz */
#define CPU_DVFS_FREQ9_L_FY     (949000)	/* KHz */
#define CPU_DVFS_FREQ10_L_FY    (858000)	/* KHz */
#define CPU_DVFS_FREQ11_L_FY    (754000)	/* KHz */
#define CPU_DVFS_FREQ12_L_FY    (663000)	/* KHz */
#define CPU_DVFS_FREQ13_L_FY    (559000)	/* KHz */
#define CPU_DVFS_FREQ14_L_FY    (468000)	/* KHz */
#define CPU_DVFS_FREQ15_L_FY    (338000)	/* KHz */

/* for DVFS OPP table CCI/FY */
#define CPU_DVFS_FREQ0_CCI_FY    (949000)	/* KHz */
#define CPU_DVFS_FREQ1_CCI_FY    (910000)	/* KHz */
#define CPU_DVFS_FREQ2_CCI_FY    (871000)	/* KHz */
#define CPU_DVFS_FREQ3_CCI_FY    (819000)	/* KHz */
#define CPU_DVFS_FREQ4_CCI_FY    (767000)	/* KHz */
#define CPU_DVFS_FREQ5_CCI_FY    (728000)	/* KHz */
#define CPU_DVFS_FREQ6_CCI_FY    (676000)	/* KHz */
#define CPU_DVFS_FREQ7_CCI_FY    (624000)	/* KHz */
#define CPU_DVFS_FREQ8_CCI_FY    (559000)	/* KHz */
#define CPU_DVFS_FREQ9_CCI_FY    (520000)	/* KHz */
#define CPU_DVFS_FREQ10_CCI_FY    (455000)	/* KHz */
#define CPU_DVFS_FREQ11_CCI_FY    (390000)	/* KHz */
#define CPU_DVFS_FREQ12_CCI_FY    (325000)	/* KHz */
#define CPU_DVFS_FREQ13_CCI_FY    (273000)	/* KHz */
#define CPU_DVFS_FREQ14_CCI_FY    (208000)	/* KHz */
#define CPU_DVFS_FREQ15_CCI_FY    (130000)	/* KHz */

/* for DVFS OPP table B/FY */
#define CPU_DVFS_FREQ0_B_FY    (1976000)	/* KHz */
#define CPU_DVFS_FREQ1_B_FY    (1872000)	/* KHz */
#define CPU_DVFS_FREQ2_B_FY    (1781000)	/* KHz */
#define CPU_DVFS_FREQ3_B_FY    (1677000)	/* KHz */
#define CPU_DVFS_FREQ4_B_FY    (1560000)	/* KHz */
#define CPU_DVFS_FREQ5_B_FY    (1443000)	/* KHz */
#define CPU_DVFS_FREQ6_B_FY    (1339000)	/* KHz */
#define CPU_DVFS_FREQ7_B_FY    (1222000)	/* KHz */
#define CPU_DVFS_FREQ8_B_FY    (1079000)	/* KHz */
#define CPU_DVFS_FREQ9_B_FY     (962000)	/* KHz */
#define CPU_DVFS_FREQ10_B_FY    (858000)	/* KHz */
#define CPU_DVFS_FREQ11_B_FY    (767000)	/* KHz */
#define CPU_DVFS_FREQ12_B_FY    (663000)	/* KHz */
#define CPU_DVFS_FREQ13_B_FY    (572000)	/* KHz */
#define CPU_DVFS_FREQ14_B_FY    (468000)	/* KHz */
#define CPU_DVFS_FREQ15_B_FY    (338000)	/* KHz */

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
#define CPU_DVFS_FREQ0_LL_SB    (1664000)	/* KHz */
#define CPU_DVFS_FREQ1_LL_SB    (1560000)	/* KHz */
#define CPU_DVFS_FREQ2_LL_SB    (1495000)	/* KHz */
#define CPU_DVFS_FREQ3_LL_SB    (1391000)	/* KHz */
#define CPU_DVFS_FREQ4_LL_SB    (1300000)	/* KHz */
#define CPU_DVFS_FREQ5_LL_SB    (1196000)	/* KHz */
#define CPU_DVFS_FREQ6_LL_SB    (1118000)	/* KHz */
#define CPU_DVFS_FREQ7_LL_SB    (1053000)	/* KHz */
#define CPU_DVFS_FREQ8_LL_SB    (975000)	/* KHz */
#define CPU_DVFS_FREQ9_LL_SB    (910000)	/* KHz */
#define CPU_DVFS_FREQ10_LL_SB    (819000)	/* KHz */
#define CPU_DVFS_FREQ11_LL_SB    (728000)	/* KHz */
#define CPU_DVFS_FREQ12_LL_SB    (637000)	/* KHz */
#define CPU_DVFS_FREQ13_LL_SB    (546000)	/* KHz */
#define CPU_DVFS_FREQ14_LL_SB    (455000)	/* KHz */
#define CPU_DVFS_FREQ15_LL_SB    (338000)	/* KHz */

/* for DVFS OPP table L/SB */
#define CPU_DVFS_FREQ0_L_SB    (1833000)	/* KHz */
#define CPU_DVFS_FREQ1_L_SB    (1729000)	/* KHz */
#define CPU_DVFS_FREQ2_L_SB    (1651000)	/* KHz */
#define CPU_DVFS_FREQ3_L_SB    (1534000)	/* KHz */
#define CPU_DVFS_FREQ4_L_SB    (1430000)	/* KHz */
#define CPU_DVFS_FREQ5_L_SB    (1313000)	/* KHz */
#define CPU_DVFS_FREQ6_L_SB    (1222000)	/* KHz */
#define CPU_DVFS_FREQ7_L_SB    (1144000)	/* KHz */
#define CPU_DVFS_FREQ8_L_SB    (1040000)	/* KHz */
#define CPU_DVFS_FREQ9_L_SB    (949000)	/* KHz */
#define CPU_DVFS_FREQ10_L_SB    (858000)	/* KHz */
#define CPU_DVFS_FREQ11_L_SB    (754000)	/* KHz */
#define CPU_DVFS_FREQ12_L_SB    (663000)	/* KHz */
#define CPU_DVFS_FREQ13_L_SB    (559000)	/* KHz */
#define CPU_DVFS_FREQ14_L_SB    (468000)	/* KHz */
#define CPU_DVFS_FREQ15_L_SB    (338000)	/* KHz */

/* for DVFS OPP table B/SB */
#define CPU_DVFS_FREQ0_B_SB    (2158000)	/* KHz */
#define CPU_DVFS_FREQ1_B_SB    (2015000)	/* KHz */
#define CPU_DVFS_FREQ2_B_SB    (1911000)	/* KHz */
#define CPU_DVFS_FREQ3_B_SB    (1755000)	/* KHz */
#define CPU_DVFS_FREQ4_B_SB    (1612000)	/* KHz */
#define CPU_DVFS_FREQ5_B_SB    (1456000)	/* KHz */
#define CPU_DVFS_FREQ6_B_SB    (1339000)	/* KHz */
#define CPU_DVFS_FREQ7_B_SB    (1222000)	/* KHz */
#define CPU_DVFS_FREQ8_B_SB    (1079000)	/* KHz */
#define CPU_DVFS_FREQ9_B_SB    (962000)	/* KHz */
#define CPU_DVFS_FREQ10_B_SB    (858000)	/* KHz */
#define CPU_DVFS_FREQ11_B_SB    (767000)	/* KHz */
#define CPU_DVFS_FREQ12_B_SB    (663000)	/* KHz */
#define CPU_DVFS_FREQ13_B_SB    (572000)	/* KHz */
#define CPU_DVFS_FREQ14_B_SB    (468000)	/* KHz */
#define CPU_DVFS_FREQ15_B_SB    (338000)	/* KHz */

/* for DVFS OPP table CCI/SB */
#define CPU_DVFS_FREQ0_CCI_SB    (1053000)	/* KHz */
#define CPU_DVFS_FREQ1_CCI_SB    (988000)	/* KHz */
#define CPU_DVFS_FREQ2_CCI_SB    (936000)	/* KHz */
#define CPU_DVFS_FREQ3_CCI_SB    (871000)	/* KHz */
#define CPU_DVFS_FREQ4_CCI_SB    (793000)	/* KHz */
#define CPU_DVFS_FREQ5_CCI_SB    (728000)	/* KHz */
#define CPU_DVFS_FREQ6_CCI_SB    (676000)	/* KHz */
#define CPU_DVFS_FREQ7_CCI_SB    (624000)	/* KHz */
#define CPU_DVFS_FREQ8_CCI_SB    (559000)	/* KHz */
#define CPU_DVFS_FREQ9_CCI_SB    (520000)	/* KHz */
#define CPU_DVFS_FREQ10_CCI_SB    (455000)	/* KHz */
#define CPU_DVFS_FREQ11_CCI_SB    (390000)	/* KHz */
#define CPU_DVFS_FREQ12_CCI_SB    (325000)	/* KHz */
#define CPU_DVFS_FREQ13_CCI_SB    (273000)	/* KHz */
#define CPU_DVFS_FREQ14_CCI_SB    (208000)	/* KHz */
#define CPU_DVFS_FREQ15_CCI_SB    (130000)	/* KHz */

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
	FP(2, 1),
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
	FP(4, 1),
	FP(4, 2),
	FP(4, 2),
	FP(4, 2),
	FP(4, 2),
	FP(4, 4),
	FP(4, 4),
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
	FP(2, 1),
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
	FP(4, 1),
	FP(4, 2),
	FP(4, 2),
	FP(4, 2),
	FP(4, 2),
	FP(4, 4),
	FP(4, 4),
};

static struct mt_cpu_freq_method opp_tbl_method_B_e2[] = {
	/* Target Frequency,	POS, CLK */
	FP(1, 1),
	FP(1, 1),
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
