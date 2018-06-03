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
#define CPU_DVFS_FREQ0_LL_FY		1989000		/* KHz */
#define CPU_DVFS_FREQ1_LL_FY		1924000		/* KHz */
#define CPU_DVFS_FREQ2_LL_FY		1846000		/* KHz */
#define CPU_DVFS_FREQ3_LL_FY		1781000		/* KHz */
#define CPU_DVFS_FREQ4_LL_FY		1716000		/* KHz */
#define CPU_DVFS_FREQ5_LL_FY		1677000		/* KHz */
#define CPU_DVFS_FREQ6_LL_FY		1625000		/* KHz */
#define CPU_DVFS_FREQ7_LL_FY		1586000		/* KHz */
#define CPU_DVFS_FREQ8_LL_FY		1508000		/* KHz */
#define CPU_DVFS_FREQ9_LL_FY		1417000		/* KHz */
#define CPU_DVFS_FREQ10_LL_FY		1326000		/* KHz */
#define CPU_DVFS_FREQ11_LL_FY		1248000		/* KHz */
#define CPU_DVFS_FREQ12_LL_FY		1131000		/* KHz */
#define CPU_DVFS_FREQ13_LL_FY		1014000		/* KHz */
#define CPU_DVFS_FREQ14_LL_FY		910000		/* KHz */
#define CPU_DVFS_FREQ15_LL_FY		793000		/* KHz */

/* for DVFS OPP table B */
#define CPU_DVFS_FREQ0_L_FY			1989000		/* KHz */
#define CPU_DVFS_FREQ1_L_FY			1924000		/* KHz */
#define CPU_DVFS_FREQ2_L_FY			1846000		/* KHz */
#define CPU_DVFS_FREQ3_L_FY			1781000		/* KHz */
#define CPU_DVFS_FREQ4_L_FY			1716000		/* KHz */
#define CPU_DVFS_FREQ5_L_FY			1677000		/* KHz */
#define CPU_DVFS_FREQ6_L_FY			1625000		/* KHz */
#define CPU_DVFS_FREQ7_L_FY			1586000		/* KHz */
#define CPU_DVFS_FREQ8_L_FY			1508000		/* KHz */
#define CPU_DVFS_FREQ9_L_FY			1417000		/* KHz */
#define CPU_DVFS_FREQ10_L_FY		1326000		/* KHz */
#define CPU_DVFS_FREQ11_L_FY		1248000		/* KHz */
#define CPU_DVFS_FREQ12_L_FY		1131000		/* KHz */
#define CPU_DVFS_FREQ13_L_FY		1014000		/* KHz */
#define CPU_DVFS_FREQ14_L_FY		910000		/* KHz */
#define CPU_DVFS_FREQ15_L_FY		793000		/* KHz */

/* for DVFS OPP table CCI */
#define CPU_DVFS_FREQ0_CCI_FY		1196000		/* KHz */
#define CPU_DVFS_FREQ1_CCI_FY		1144000		/* KHz */
#define CPU_DVFS_FREQ2_CCI_FY		1092000		/* KHz */
#define CPU_DVFS_FREQ3_CCI_FY		1027000		/* KHz */
#define CPU_DVFS_FREQ4_CCI_FY		962000		/* KHz */
#define CPU_DVFS_FREQ5_CCI_FY		923000		/* KHz */
#define CPU_DVFS_FREQ6_CCI_FY		871000		/* KHz */
#define CPU_DVFS_FREQ7_CCI_FY		845000		/* KHz */
#define CPU_DVFS_FREQ8_CCI_FY		767000		/* KHz */
#define CPU_DVFS_FREQ9_CCI_FY		689000		/* KHz */
#define CPU_DVFS_FREQ10_CCI_FY		624000		/* KHz */
#define CPU_DVFS_FREQ11_CCI_FY		546000		/* KHz */
#define CPU_DVFS_FREQ12_CCI_FY		481000		/* KHz */
#define CPU_DVFS_FREQ13_CCI_FY		403000		/* KHz */
#define CPU_DVFS_FREQ14_CCI_FY		338000		/* KHz */
#define CPU_DVFS_FREQ15_CCI_FY		273000		/* KHz */

/* for DVFS OPP table L|CCI */
#define CPU_DVFS_VOLT0_VPROC1_FY	 100000		/* 10uV */
#define CPU_DVFS_VOLT1_VPROC1_FY	 97500		/* 10uV */
#define CPU_DVFS_VOLT2_VPROC1_FY	 95000		/* 10uV */
#define CPU_DVFS_VOLT3_VPROC1_FY	 92500		/* 10uV */
#define CPU_DVFS_VOLT4_VPROC1_FY	 90000		/* 10uV */
#define CPU_DVFS_VOLT5_VPROC1_FY	 88125		/* 10uV */
#define CPU_DVFS_VOLT6_VPROC1_FY	 86250		/* 10uV */
#define CPU_DVFS_VOLT7_VPROC1_FY	 85000		/* 10uV */
#define CPU_DVFS_VOLT8_VPROC1_FY	 82500		/* 10uV */
#define CPU_DVFS_VOLT9_VPROC1_FY	 80000		/* 10uV */
#define CPU_DVFS_VOLT10_VPROC1_FY	 77500		/* 10uV */
#define CPU_DVFS_VOLT11_VPROC1_FY	 75000		/* 10uV */
#define CPU_DVFS_VOLT12_VPROC1_FY	 72500		/* 10uV */
#define CPU_DVFS_VOLT13_VPROC1_FY	 70000		/* 10uV */
#define CPU_DVFS_VOLT14_VPROC1_FY	 67500		/* 10uV */
#define CPU_DVFS_VOLT15_VPROC1_FY	 65000		/* 10uV */

/* for DVFS OPP table B */
#define CPU_DVFS_VOLT0_VPROC2_FY	 100000		/* 10uV */
#define CPU_DVFS_VOLT1_VPROC2_FY	 97500		/* 10uV */
#define CPU_DVFS_VOLT2_VPROC2_FY	 95000		/* 10uV */
#define CPU_DVFS_VOLT3_VPROC2_FY	 92500		/* 10uV */
#define CPU_DVFS_VOLT4_VPROC2_FY	 90000		/* 10uV */
#define CPU_DVFS_VOLT5_VPROC2_FY	 88125		/* 10uV */
#define CPU_DVFS_VOLT6_VPROC2_FY	 86250		/* 10uV */
#define CPU_DVFS_VOLT7_VPROC2_FY	 85000		/* 10uV */
#define CPU_DVFS_VOLT8_VPROC2_FY	 82500		/* 10uV */
#define CPU_DVFS_VOLT9_VPROC2_FY	 80000		/* 10uV */
#define CPU_DVFS_VOLT10_VPROC2_FY	 77500		/* 10uV */
#define CPU_DVFS_VOLT11_VPROC2_FY	 75000		/* 10uV */
#define CPU_DVFS_VOLT12_VPROC2_FY	 72500		/* 10uV */
#define CPU_DVFS_VOLT13_VPROC2_FY	 70000		/* 10uV */
#define CPU_DVFS_VOLT14_VPROC2_FY	 67500		/* 10uV */
#define CPU_DVFS_VOLT15_VPROC2_FY	 65000		/* 10uV */

/* FY2 */
/* for DVFS OPP table L */
#define CPU_DVFS_FREQ0_LL_FY2		1989000		/* KHz */
#define CPU_DVFS_FREQ1_LL_FY2		1924000		/* KHz */
#define CPU_DVFS_FREQ2_LL_FY2		1846000		/* KHz */
#define CPU_DVFS_FREQ3_LL_FY2		1781000		/* KHz */
#define CPU_DVFS_FREQ4_LL_FY2		1716000		/* KHz */
#define CPU_DVFS_FREQ5_LL_FY2		1677000		/* KHz */
#define CPU_DVFS_FREQ6_LL_FY2		1625000		/* KHz */
#define CPU_DVFS_FREQ7_LL_FY2		1586000		/* KHz */
#define CPU_DVFS_FREQ8_LL_FY2		1508000		/* KHz */
#define CPU_DVFS_FREQ9_LL_FY2		1417000		/* KHz */
#define CPU_DVFS_FREQ10_LL_FY2		1326000		/* KHz */
#define CPU_DVFS_FREQ11_LL_FY2		1248000		/* KHz */
#define CPU_DVFS_FREQ12_LL_FY2		1131000		/* KHz */
#define CPU_DVFS_FREQ13_LL_FY2		1014000		/* KHz */
#define CPU_DVFS_FREQ14_LL_FY2		910000		/* KHz */
#define CPU_DVFS_FREQ15_LL_FY2		793000		/* KHz */

/* for DVFS OPP table B */
#define CPU_DVFS_FREQ0_L_FY2		1989000		/* KHz */
#define CPU_DVFS_FREQ1_L_FY2		1924000		/* KHz */
#define CPU_DVFS_FREQ2_L_FY2		1846000		/* KHz */
#define CPU_DVFS_FREQ3_L_FY2		1781000		/* KHz */
#define CPU_DVFS_FREQ4_L_FY2		1716000		/* KHz */
#define CPU_DVFS_FREQ5_L_FY2		1677000		/* KHz */
#define CPU_DVFS_FREQ6_L_FY2		1625000		/* KHz */
#define CPU_DVFS_FREQ7_L_FY2		1586000		/* KHz */
#define CPU_DVFS_FREQ8_L_FY2		1508000		/* KHz */
#define CPU_DVFS_FREQ9_L_FY2		1417000		/* KHz */
#define CPU_DVFS_FREQ10_L_FY2		1326000		/* KHz */
#define CPU_DVFS_FREQ11_L_FY2		1248000		/* KHz */
#define CPU_DVFS_FREQ12_L_FY2		1131000		/* KHz */
#define CPU_DVFS_FREQ13_L_FY2		1014000		/* KHz */
#define CPU_DVFS_FREQ14_L_FY2		910000		/* KHz */
#define CPU_DVFS_FREQ15_L_FY2		793000		/* KHz */

/* for DVFS OPP table CCI */
#define CPU_DVFS_FREQ0_CCI_FY2		1196000		/* KHz */
#define CPU_DVFS_FREQ1_CCI_FY2		1144000		/* KHz */
#define CPU_DVFS_FREQ2_CCI_FY2		1092000		/* KHz */
#define CPU_DVFS_FREQ3_CCI_FY2		1027000		/* KHz */
#define CPU_DVFS_FREQ4_CCI_FY2		962000		/* KHz */
#define CPU_DVFS_FREQ5_CCI_FY2		923000		/* KHz */
#define CPU_DVFS_FREQ6_CCI_FY2		871000		/* KHz */
#define CPU_DVFS_FREQ7_CCI_FY2		845000		/* KHz */
#define CPU_DVFS_FREQ8_CCI_FY2		767000		/* KHz */
#define CPU_DVFS_FREQ9_CCI_FY2		689000		/* KHz */
#define CPU_DVFS_FREQ10_CCI_FY2		624000		/* KHz */
#define CPU_DVFS_FREQ11_CCI_FY2		546000		/* KHz */
#define CPU_DVFS_FREQ12_CCI_FY2		481000		/* KHz */
#define CPU_DVFS_FREQ13_CCI_FY2		403000		/* KHz */
#define CPU_DVFS_FREQ14_CCI_FY2		338000		/* KHz */
#define CPU_DVFS_FREQ15_CCI_FY2		273000		/* KHz */

/* for DVFS OPP table L|CCI */
#define CPU_DVFS_VOLT0_VPROC1_FY2	 100000		/* 10uV */
#define CPU_DVFS_VOLT1_VPROC1_FY2	 97500		/* 10uV */
#define CPU_DVFS_VOLT2_VPROC1_FY2	 95000		/* 10uV */
#define CPU_DVFS_VOLT3_VPROC1_FY2	 92500		/* 10uV */
#define CPU_DVFS_VOLT4_VPROC1_FY2	 90000		/* 10uV */
#define CPU_DVFS_VOLT5_VPROC1_FY2	 88125		/* 10uV */
#define CPU_DVFS_VOLT6_VPROC1_FY2	 86250		/* 10uV */
#define CPU_DVFS_VOLT7_VPROC1_FY2	 85000		/* 10uV */
#define CPU_DVFS_VOLT8_VPROC1_FY2	 82500		/* 10uV */
#define CPU_DVFS_VOLT9_VPROC1_FY2	 80000		/* 10uV */
#define CPU_DVFS_VOLT10_VPROC1_FY2	 77500		/* 10uV */
#define CPU_DVFS_VOLT11_VPROC1_FY2	 75000		/* 10uV */
#define CPU_DVFS_VOLT12_VPROC1_FY2	 72500		/* 10uV */
#define CPU_DVFS_VOLT13_VPROC1_FY2	 70000		/* 10uV */
#define CPU_DVFS_VOLT14_VPROC1_FY2	 67500		/* 10uV */
#define CPU_DVFS_VOLT15_VPROC1_FY2	 65000		/* 10uV */

/* for DVFS OPP table B */
#define CPU_DVFS_VOLT0_VPROC2_FY2	 100000		/* 10uV */
#define CPU_DVFS_VOLT1_VPROC2_FY2	 97500		/* 10uV */
#define CPU_DVFS_VOLT2_VPROC2_FY2	 95000		/* 10uV */
#define CPU_DVFS_VOLT3_VPROC2_FY2	 92500		/* 10uV */
#define CPU_DVFS_VOLT4_VPROC2_FY2	 90000		/* 10uV */
#define CPU_DVFS_VOLT5_VPROC2_FY2	 88125		/* 10uV */
#define CPU_DVFS_VOLT6_VPROC2_FY2	 86250		/* 10uV */
#define CPU_DVFS_VOLT7_VPROC2_FY2	 85000		/* 10uV */
#define CPU_DVFS_VOLT8_VPROC2_FY2	 82500		/* 10uV */
#define CPU_DVFS_VOLT9_VPROC2_FY2	 80000		/* 10uV */
#define CPU_DVFS_VOLT10_VPROC2_FY2	 77500		/* 10uV */
#define CPU_DVFS_VOLT11_VPROC2_FY2	 75000		/* 10uV */
#define CPU_DVFS_VOLT12_VPROC2_FY2	 72500		/* 10uV */
#define CPU_DVFS_VOLT13_VPROC2_FY2	 70000		/* 10uV */
#define CPU_DVFS_VOLT14_VPROC2_FY2	 67500		/* 10uV */
#define CPU_DVFS_VOLT15_VPROC2_FY2	 65000		/* 10uV */

/* SB */
/* for DVFS OPP table LL B FY*/
#define CPU_DVFS_FREQ0_LL_SB		2197000		/* KHz */
#define CPU_DVFS_FREQ1_LL_SB		2106000		/* KHz */
#define CPU_DVFS_FREQ2_LL_SB		1989000		/* KHz */
#define CPU_DVFS_FREQ3_LL_SB		1846000		/* KHz */
#define CPU_DVFS_FREQ4_LL_SB		1716000		/* KHz */
#define CPU_DVFS_FREQ5_LL_SB		1677000		/* KHz */
#define CPU_DVFS_FREQ6_LL_SB		1625000		/* KHz */
#define CPU_DVFS_FREQ7_LL_SB		1586000		/* KHz */
#define CPU_DVFS_FREQ8_LL_SB		1508000		/* KHz */
#define CPU_DVFS_FREQ9_LL_SB		1417000		/* KHz */
#define CPU_DVFS_FREQ10_LL_SB		1326000		/* KHz */
#define CPU_DVFS_FREQ11_LL_SB		1248000		/* KHz */
#define CPU_DVFS_FREQ12_LL_SB		1131000		/* KHz */
#define CPU_DVFS_FREQ13_LL_SB		1014000		/* KHz */
#define CPU_DVFS_FREQ14_LL_SB		910000		/* KHz */
#define CPU_DVFS_FREQ15_LL_SB		793000		/* KHz */

/* for DVFS OPP table L B FY */
#define CPU_DVFS_FREQ0_L_SB		2197000		/* KHz */
#define CPU_DVFS_FREQ1_L_SB		2106000		/* KHz */
#define CPU_DVFS_FREQ2_L_SB		1989000		/* KHz */
#define CPU_DVFS_FREQ3_L_SB		1846000		/* KHz */
#define CPU_DVFS_FREQ4_L_SB		1716000		/* KHz */
#define CPU_DVFS_FREQ5_L_SB		1677000		/* KHz */
#define CPU_DVFS_FREQ6_L_SB		1625000		/* KHz */
#define CPU_DVFS_FREQ7_L_SB		1586000		/* KHz */
#define CPU_DVFS_FREQ8_L_SB		1508000		/* KHz */
#define CPU_DVFS_FREQ9_L_SB		1417000		/* KHz */
#define CPU_DVFS_FREQ10_L_SB		1326000		/* KHz */
#define CPU_DVFS_FREQ11_L_SB		1248000		/* KHz */
#define CPU_DVFS_FREQ12_L_SB		1131000		/* KHz */
#define CPU_DVFS_FREQ13_L_SB		1014000		/* KHz */
#define CPU_DVFS_FREQ14_L_SB		910000		/* KHz */
#define CPU_DVFS_FREQ15_L_SB		793000		/* KHz */

/* for DVFS OPP table CCI */
#define CPU_DVFS_FREQ0_CCI_SB		1339000		/* KHz */
#define CPU_DVFS_FREQ1_CCI_SB		1274000		/* KHz */
#define CPU_DVFS_FREQ2_CCI_SB		1196000		/* KHz */
#define CPU_DVFS_FREQ3_CCI_SB		1092000		/* KHz */
#define CPU_DVFS_FREQ4_CCI_SB		962000		/* KHz */
#define CPU_DVFS_FREQ5_CCI_SB		923000		/* KHz */
#define CPU_DVFS_FREQ6_CCI_SB		871000		/* KHz */
#define CPU_DVFS_FREQ7_CCI_SB		845000		/* KHz */
#define CPU_DVFS_FREQ8_CCI_SB		767000		/* KHz */
#define CPU_DVFS_FREQ9_CCI_SB		689000		/* KHz */
#define CPU_DVFS_FREQ10_CCI_SB		624000		/* KHz */
#define CPU_DVFS_FREQ11_CCI_SB		546000		/* KHz */
#define CPU_DVFS_FREQ12_CCI_SB		481000		/* KHz */
#define CPU_DVFS_FREQ13_CCI_SB		403000		/* KHz */
#define CPU_DVFS_FREQ14_CCI_SB		338000		/* KHz */
#define CPU_DVFS_FREQ15_CCI_SB		273000		/* KHz */

/* for DVFS OPP table L|CCI */
#define CPU_DVFS_VOLT0_VPROC1_SB	 111875    /* 10uV */
#define CPU_DVFS_VOLT1_VPROC1_SB	 105000    /* 10uV */
#define CPU_DVFS_VOLT2_VPROC1_SB	 95000     /* 10uV */
#define CPU_DVFS_VOLT3_VPROC1_SB	 90000     /* 10uV */
#define CPU_DVFS_VOLT4_VPROC1_SB	 85000     /* 10uV */
#define CPU_DVFS_VOLT5_VPROC1_SB	 83125     /* 10uV */
#define CPU_DVFS_VOLT6_VPROC1_SB	 81250     /* 10uV */
#define CPU_DVFS_VOLT7_VPROC1_SB	 80000     /* 10uV */
#define CPU_DVFS_VOLT8_VPROC1_SB	 77500     /* 10uV */
#define CPU_DVFS_VOLT9_VPROC1_SB	 75000     /* 10uV */
#define CPU_DVFS_VOLT10_VPROC1_SB	 72500     /* 10uV */
#define CPU_DVFS_VOLT11_VPROC1_SB	 70000     /* 10uV */
#define CPU_DVFS_VOLT12_VPROC1_SB	 67500     /* 10uV */
#define CPU_DVFS_VOLT13_VPROC1_SB	 65000     /* 10uV */
#define CPU_DVFS_VOLT14_VPROC1_SB	 62500     /* 10uV */
#define CPU_DVFS_VOLT15_VPROC1_SB	 60000     /* 10uV */

/* for DVFS OPP table B */
#define CPU_DVFS_VOLT0_VPROC2_SB	 111875    /* 10uV */
#define CPU_DVFS_VOLT1_VPROC2_SB	 105000    /* 10uV */
#define CPU_DVFS_VOLT2_VPROC2_SB	 95000     /* 10uV */
#define CPU_DVFS_VOLT3_VPROC2_SB	 90000     /* 10uV */
#define CPU_DVFS_VOLT4_VPROC2_SB	 85000     /* 10uV */
#define CPU_DVFS_VOLT5_VPROC2_SB	 83125     /* 10uV */
#define CPU_DVFS_VOLT6_VPROC2_SB	 81250     /* 10uV */
#define CPU_DVFS_VOLT7_VPROC2_SB	 80000     /* 10uV */
#define CPU_DVFS_VOLT8_VPROC2_SB	 77500     /* 10uV */
#define CPU_DVFS_VOLT9_VPROC2_SB	 75000     /* 10uV */
#define CPU_DVFS_VOLT10_VPROC2_SB	 72500     /* 10uV */
#define CPU_DVFS_VOLT11_VPROC2_SB	 70000     /* 10uV */
#define CPU_DVFS_VOLT12_VPROC2_SB	 67500     /* 10uV */
#define CPU_DVFS_VOLT13_VPROC2_SB	 65000     /* 10uV */
#define CPU_DVFS_VOLT14_VPROC2_SB	 62500     /* 10uV */
#define CPU_DVFS_VOLT15_VPROC2_SB	 60000     /* 10uV */

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
OPP_TBL(L, FY, 0, 2);
OPP_TBL(CCI, FY, 0, 1);

OPP_TBL(LL, FY2, 1, 1);
OPP_TBL(L, FY2, 1, 2);
OPP_TBL(CCI, FY2, 1, 1);

OPP_TBL(LL, SB, 2, 1);
OPP_TBL(L, SB, 2, 2);
OPP_TBL(CCI, SB, 2, 1);

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
static struct mt_cpu_freq_method opp_tbl_method_LL_e0[] = {	/* FY */
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
	FP(2,	1),
	FP(2,	1),
};

static struct mt_cpu_freq_method opp_tbl_method_CCI_e0[] = {	/* FY */
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
	FP(4,	2),
	FP(4,	2),
};

/* 16 steps OPP table */
static struct mt_cpu_freq_method opp_tbl_method_LL_e1[] = {	/* FY2 */
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
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
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
	FP(4,	2),
	FP(4,	2),
};

static struct mt_cpu_freq_method opp_tbl_method_LL_e2[] = {	/* SB */
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

static struct mt_cpu_freq_method opp_tbl_method_L_e2[] = {	/* SB */
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

static struct mt_cpu_freq_method opp_tbl_method_CCI_e2[] = {	/* SB */
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
	FP(4,	2),
	FP(4,	2),
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
