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
#define CPU_DVFS_FREQ0_LL_FY    (1248000)
#define CPU_DVFS_FREQ1_LL_FY    (1209000)
#define CPU_DVFS_FREQ2_LL_FY    (1170000)
#define CPU_DVFS_FREQ3_LL_FY    (1118000)
#define CPU_DVFS_FREQ4_LL_FY    (1066000)
#define CPU_DVFS_FREQ5_LL_FY    (1027000)
#define CPU_DVFS_FREQ6_LL_FY    (975000)
#define CPU_DVFS_FREQ7_LL_FY    (923000)
#define CPU_DVFS_FREQ8_LL_FY    (858000)
#define CPU_DVFS_FREQ9_LL_FY    (806000)
#define CPU_DVFS_FREQ10_LL_FY    (741000)
#define CPU_DVFS_FREQ11_LL_FY    (663000)
#define CPU_DVFS_FREQ12_LL_FY    (585000)
#define CPU_DVFS_FREQ13_LL_FY    (520000)
#define CPU_DVFS_FREQ14_LL_FY    (442000)
#define CPU_DVFS_FREQ15_LL_FY    (351000)

/* for DVFS OPP table L/FY */
#define CPU_DVFS_FREQ0_L_FY    (1378000)	/* KHz */
#define CPU_DVFS_FREQ1_L_FY    (1313000)	/* KHz */
#define CPU_DVFS_FREQ2_L_FY    (1274000)	/* KHz */
#define CPU_DVFS_FREQ3_L_FY    (1209000)	/* KHz */
#define CPU_DVFS_FREQ4_L_FY    (1144000)	/* KHz */
#define CPU_DVFS_FREQ5_L_FY    (1079000)	/* KHz */
#define CPU_DVFS_FREQ6_L_FY    (1014000)	/* KHz */
#define CPU_DVFS_FREQ7_L_FY    (962000)	/* KHz */
#define CPU_DVFS_FREQ8_L_FY    (884000)	/* KHz */
#define CPU_DVFS_FREQ9_L_FY     (819000)	/* KHz */
#define CPU_DVFS_FREQ10_L_FY    (741000)	/* KHz */
#define CPU_DVFS_FREQ11_L_FY    (663000)	/* KHz */
#define CPU_DVFS_FREQ12_L_FY    (585000)	/* KHz */
#define CPU_DVFS_FREQ13_L_FY    (520000)	/* KHz */
#define CPU_DVFS_FREQ14_L_FY    (442000)	/* KHz */
#define CPU_DVFS_FREQ15_L_FY    (338000)	/* KHz */

/* for DVFS OPP table CCI/FY */
#define CPU_DVFS_FREQ0_CCI_FY    (754000)	/* KHz */
#define CPU_DVFS_FREQ1_CCI_FY    (715000)	/* KHz */
#define CPU_DVFS_FREQ2_CCI_FY    (689000)	/* KHz */
#define CPU_DVFS_FREQ3_CCI_FY    (650000)	/* KHz */
#define CPU_DVFS_FREQ4_CCI_FY    (611000)	/* KHz */
#define CPU_DVFS_FREQ5_CCI_FY    (585000)	/* KHz */
#define CPU_DVFS_FREQ6_CCI_FY    (546000)	/* KHz */
#define CPU_DVFS_FREQ7_CCI_FY    (507000)	/* KHz */
#define CPU_DVFS_FREQ8_CCI_FY    (468000)	/* KHz */
#define CPU_DVFS_FREQ9_CCI_FY    (429000)	/* KHz */
#define CPU_DVFS_FREQ10_CCI_FY    (390000)	/* KHz */
#define CPU_DVFS_FREQ11_CCI_FY    (338000)	/* KHz */
#define CPU_DVFS_FREQ12_CCI_FY    (299000)	/* KHz */
#define CPU_DVFS_FREQ13_CCI_FY    (247000)	/* KHz */
#define CPU_DVFS_FREQ14_CCI_FY    (208000)	/* KHz */
#define CPU_DVFS_FREQ15_CCI_FY    (156000)	/* KHz */

/* for DVFS OPP table B/FY */
#define CPU_DVFS_FREQ0_B_FY    (1638000)	/* KHz */
#define CPU_DVFS_FREQ1_B_FY    (1534000)	/* KHz */
#define CPU_DVFS_FREQ2_B_FY    (1469000)	/* KHz */
#define CPU_DVFS_FREQ3_B_FY    (1378000)	/* KHz */
#define CPU_DVFS_FREQ4_B_FY    (1287000)	/* KHz */
#define CPU_DVFS_FREQ5_B_FY    (1196000)	/* KHz */
#define CPU_DVFS_FREQ6_B_FY    (1105000)	/* KHz */
#define CPU_DVFS_FREQ7_B_FY    (1014000)	/* KHz */
#define CPU_DVFS_FREQ8_B_FY    (897000)	/* KHz */
#define CPU_DVFS_FREQ9_B_FY     (806000)	/* KHz */
#define CPU_DVFS_FREQ10_B_FY    (715000)	/* KHz */
#define CPU_DVFS_FREQ11_B_FY    (650000)	/* KHz */
#define CPU_DVFS_FREQ12_B_FY    (572000)	/* KHz */
#define CPU_DVFS_FREQ13_B_FY    (507000)	/* KHz */
#define CPU_DVFS_FREQ14_B_FY    (429000)	/* KHz */
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

/* for DVFS OPP table LL/FYA */
#define CPU_DVFS_FREQ0_LL_FYA    (1586000)
#define CPU_DVFS_FREQ1_LL_FYA    (1495000)
#define CPU_DVFS_FREQ2_LL_FYA    (1430000)
#define CPU_DVFS_FREQ3_LL_FYA    (1352000)
#define CPU_DVFS_FREQ4_LL_FYA    (1261000)
#define CPU_DVFS_FREQ5_LL_FYA    (1183000)
#define CPU_DVFS_FREQ6_LL_FYA    (1092000)
#define CPU_DVFS_FREQ7_LL_FYA    (1014000)
#define CPU_DVFS_FREQ8_LL_FYA    (910000)
#define CPU_DVFS_FREQ9_LL_FYA    (819000)
#define CPU_DVFS_FREQ10_LL_FYA    (741000)
#define CPU_DVFS_FREQ11_LL_FYA    (663000)
#define CPU_DVFS_FREQ12_LL_FYA    (585000)
#define CPU_DVFS_FREQ13_LL_FYA    (520000)
#define CPU_DVFS_FREQ14_LL_FYA    (442000)
#define CPU_DVFS_FREQ15_LL_FYA    (351000)

/* for DVFS OPP table L/FYA */
#define CPU_DVFS_FREQ0_L_FYA    (1690000)	/* KHz */
#define CPU_DVFS_FREQ1_L_FYA    (1586000)	/* KHz */
#define CPU_DVFS_FREQ2_L_FYA    (1521000)	/* KHz */
#define CPU_DVFS_FREQ3_L_FYA    (1417000)	/* KHz */
#define CPU_DVFS_FREQ4_L_FYA    (1326000)	/* KHz */
#define CPU_DVFS_FREQ5_L_FYA    (1235000)	/* KHz */
#define CPU_DVFS_FREQ6_L_FYA    (1131000)	/* KHz */
#define CPU_DVFS_FREQ7_L_FYA    (1040000)	/* KHz */
#define CPU_DVFS_FREQ8_L_FYA    (923000)	/* KHz */
#define CPU_DVFS_FREQ9_L_FYA     (832000)	/* KHz */
#define CPU_DVFS_FREQ10_L_FYA    (741000)	/* KHz */
#define CPU_DVFS_FREQ11_L_FYA    (663000)	/* KHz */
#define CPU_DVFS_FREQ12_L_FYA    (585000)	/* KHz */
#define CPU_DVFS_FREQ13_L_FYA    (520000)	/* KHz */
#define CPU_DVFS_FREQ14_L_FYA    (442000)	/* KHz */
#define CPU_DVFS_FREQ15_L_FYA    (338000)	/* KHz */

/* for DVFS OPP table CCI/FYA */
#define CPU_DVFS_FREQ0_CCI_FYA    (949000)	/* KHz */
#define CPU_DVFS_FREQ1_CCI_FYA    (884000)	/* KHz */
#define CPU_DVFS_FREQ2_CCI_FYA    (845000)	/* KHz */
#define CPU_DVFS_FREQ3_CCI_FYA    (793000)	/* KHz */
#define CPU_DVFS_FREQ4_CCI_FYA    (728000)	/* KHz */
#define CPU_DVFS_FREQ5_CCI_FYA    (676000)	/* KHz */
#define CPU_DVFS_FREQ6_CCI_FYA    (624000)	/* KHz */
#define CPU_DVFS_FREQ7_CCI_FYA    (559000)	/* KHz */
#define CPU_DVFS_FREQ8_CCI_FYA    (494000)	/* KHz */
#define CPU_DVFS_FREQ9_CCI_FYA    (442000)	/* KHz */
#define CPU_DVFS_FREQ10_CCI_FYA    (390000)	/* KHz */
#define CPU_DVFS_FREQ11_CCI_FYA    (338000)	/* KHz */
#define CPU_DVFS_FREQ12_CCI_FYA    (299000)	/* KHz */
#define CPU_DVFS_FREQ13_CCI_FYA    (247000)	/* KHz */
#define CPU_DVFS_FREQ14_CCI_FYA    (208000)	/* KHz */
#define CPU_DVFS_FREQ15_CCI_FYA    (156000)	/* KHz */

/* for DVFS OPP table B/FYA */
#define CPU_DVFS_FREQ0_B_FYA    (1976000)	/* KHz */
#define CPU_DVFS_FREQ1_B_FYA    (1846000)	/* KHz */
#define CPU_DVFS_FREQ2_B_FYA    (1742000)	/* KHz */
#define CPU_DVFS_FREQ3_B_FYA    (1612000)	/* KHz */
#define CPU_DVFS_FREQ4_B_FYA    (1495000)	/* KHz */
#define CPU_DVFS_FREQ5_B_FYA    (1365000)	/* KHz */
#define CPU_DVFS_FREQ6_B_FYA    (1235000)	/* KHz */
#define CPU_DVFS_FREQ7_B_FYA    (1105000)	/* KHz */
#define CPU_DVFS_FREQ8_B_FYA    (949000)	/* KHz */
#define CPU_DVFS_FREQ9_B_FYA     (819000)	/* KHz */
#define CPU_DVFS_FREQ10_B_FYA    (715000)	/* KHz */
#define CPU_DVFS_FREQ11_B_FYA    (650000)	/* KHz */
#define CPU_DVFS_FREQ12_B_FYA    (572000)	/* KHz */
#define CPU_DVFS_FREQ13_B_FYA    (507000)	/* KHz */
#define CPU_DVFS_FREQ14_B_FYA    (429000)	/* KHz */
#define CPU_DVFS_FREQ15_B_FYA    (338000)	/* KHz */

#define CPU_DVFS_VOLT0_VPROC1_FYA    (97374)	/* 10MV */
#define CPU_DVFS_VOLT1_VPROC1_FYA    (94969)	/* 10MV */
#define CPU_DVFS_VOLT2_VPROC1_FYA    (93166)	/* 10MV */
#define CPU_DVFS_VOLT3_VPROC1_FYA    (90762)	/* 10MV */
#define CPU_DVFS_VOLT4_VPROC1_FYA    (88357)	/* 10MV */
#define CPU_DVFS_VOLT5_VPROC1_FYA    (85953)	/* 10MV */
#define CPU_DVFS_VOLT6_VPROC1_FYA    (83549)	/* 10MV */
#define CPU_DVFS_VOLT7_VPROC1_FYA    (81145)	/* 10MV */
#define CPU_DVFS_VOLT8_VPROC1_FYA    (78139)		/* 10MV */
#define CPU_DVFS_VOLT9_VPROC1_FYA    (75735)		/* 10MV */
#define CPU_DVFS_VOLT10_VPROC1_FYA    (75014)	/* 10MV */
#define CPU_DVFS_VOLT11_VPROC1_FYA    (75014)	/* 10MV */
#define CPU_DVFS_VOLT12_VPROC1_FYA    (75014)	/* 10MV */
#define CPU_DVFS_VOLT13_VPROC1_FYA    (75014)	/* 10MV */
#define CPU_DVFS_VOLT14_VPROC1_FYA    (75014)	/* 10MV */
#define CPU_DVFS_VOLT15_VPROC1_FYA    (75014)	/* 10MV */

/* for DVFS OPP table LL/FYB */
#define CPU_DVFS_FREQ0_LL_FYB    (1755000)
#define CPU_DVFS_FREQ1_LL_FYB    (1651000)
#define CPU_DVFS_FREQ2_LL_FYB    (1573000)
#define CPU_DVFS_FREQ3_LL_FYB    (1469000)
#define CPU_DVFS_FREQ4_LL_FYB    (1365000)
#define CPU_DVFS_FREQ5_LL_FYB    (1261000)
#define CPU_DVFS_FREQ6_LL_FYB    (1157000)
#define CPU_DVFS_FREQ7_LL_FYB    (1053000)
#define CPU_DVFS_FREQ8_LL_FYB    (936000)
#define CPU_DVFS_FREQ9_LL_FYB    (832000)
#define CPU_DVFS_FREQ10_LL_FYB    (741000)
#define CPU_DVFS_FREQ11_LL_FYB    (663000)
#define CPU_DVFS_FREQ12_LL_FYB    (585000)
#define CPU_DVFS_FREQ13_LL_FYB    (520000)
#define CPU_DVFS_FREQ14_LL_FYB    (442000)
#define CPU_DVFS_FREQ15_LL_FYB    (351000)

/* for DVFS OPP table L/FYB */
#define CPU_DVFS_FREQ0_L_FYB    (1872000)	/* KHz */
#define CPU_DVFS_FREQ1_L_FYB    (1755000)	/* KHz */
#define CPU_DVFS_FREQ2_L_FYB    (1664000)	/* KHz */
#define CPU_DVFS_FREQ3_L_FYB    (1547000)	/* KHz */
#define CPU_DVFS_FREQ4_L_FYB    (1430000)	/* KHz */
#define CPU_DVFS_FREQ5_L_FYB    (1326000)	/* KHz */
#define CPU_DVFS_FREQ6_L_FYB    (1209000)	/* KHz */
#define CPU_DVFS_FREQ7_L_FYB    (1092000)	/* KHz */
#define CPU_DVFS_FREQ8_L_FYB    (949000)	/* KHz */
#define CPU_DVFS_FREQ9_L_FYB     (832000)	/* KHz */
#define CPU_DVFS_FREQ10_L_FYB    (741000)	/* KHz */
#define CPU_DVFS_FREQ11_L_FYB    (663000)	/* KHz */
#define CPU_DVFS_FREQ12_L_FYB    (585000)	/* KHz */
#define CPU_DVFS_FREQ13_L_FYB    (520000)	/* KHz */
#define CPU_DVFS_FREQ14_L_FYB    (442000)	/* KHz */
#define CPU_DVFS_FREQ15_L_FYB    (338000)	/* KHz */

/* for DVFS OPP table CCI/FYB */
#define CPU_DVFS_FREQ0_CCI_FYB    (1053000)	/* KHz */
#define CPU_DVFS_FREQ1_CCI_FYB    (975000)	/* KHz */
#define CPU_DVFS_FREQ2_CCI_FYB    (923000)	/* KHz */
#define CPU_DVFS_FREQ3_CCI_FYB    (858000)	/* KHz */
#define CPU_DVFS_FREQ4_CCI_FYB    (793000)	/* KHz */
#define CPU_DVFS_FREQ5_CCI_FYB    (728000)	/* KHz */
#define CPU_DVFS_FREQ6_CCI_FYB    (663000)	/* KHz */
#define CPU_DVFS_FREQ7_CCI_FYB    (598000)	/* KHz */
#define CPU_DVFS_FREQ8_CCI_FYB    (507000)	/* KHz */
#define CPU_DVFS_FREQ9_CCI_FYB    (442000)	/* KHz */
#define CPU_DVFS_FREQ10_CCI_FYB    (390000)	/* KHz */
#define CPU_DVFS_FREQ11_CCI_FYB    (338000)	/* KHz */
#define CPU_DVFS_FREQ12_CCI_FYB    (299000)	/* KHz */
#define CPU_DVFS_FREQ13_CCI_FYB    (247000)	/* KHz */
#define CPU_DVFS_FREQ14_CCI_FYB    (208000)	/* KHz */
#define CPU_DVFS_FREQ15_CCI_FYB    (156000)	/* KHz */

/* for DVFS OPP table B/FYB */
#define CPU_DVFS_FREQ0_B_FYB    (2197000)	/* KHz */
#define CPU_DVFS_FREQ1_B_FYB    (2041000)	/* KHz */
#define CPU_DVFS_FREQ2_B_FYB    (1924000)	/* KHz */
#define CPU_DVFS_FREQ3_B_FYB    (1768000)	/* KHz */
#define CPU_DVFS_FREQ4_B_FYB    (1625000)	/* KHz */
#define CPU_DVFS_FREQ5_B_FYB    (1469000)	/* KHz */
#define CPU_DVFS_FREQ6_B_FYB    (1313000)	/* KHz */
#define CPU_DVFS_FREQ7_B_FYB    (1170000)	/* KHz */
#define CPU_DVFS_FREQ8_B_FYB    (975000)	/* KHz */
#define CPU_DVFS_FREQ9_B_FYB     (819000)	/* KHz */
#define CPU_DVFS_FREQ10_B_FYB    (715000)	/* KHz */
#define CPU_DVFS_FREQ11_B_FYB    (650000)	/* KHz */
#define CPU_DVFS_FREQ12_B_FYB    (572000)	/* KHz */
#define CPU_DVFS_FREQ13_B_FYB    (507000)	/* KHz */
#define CPU_DVFS_FREQ14_B_FYB    (429000)	/* KHz */
#define CPU_DVFS_FREQ15_B_FYB    (338000)	/* KHz */

#define CPU_DVFS_VOLT0_VPROC1_FYB    (97374)	/* 10MV */
#define CPU_DVFS_VOLT1_VPROC1_FYB    (94969)	/* 10MV */
#define CPU_DVFS_VOLT2_VPROC1_FYB    (93166)	/* 10MV */
#define CPU_DVFS_VOLT3_VPROC1_FYB    (90762)	/* 10MV */
#define CPU_DVFS_VOLT4_VPROC1_FYB    (88357)	/* 10MV */
#define CPU_DVFS_VOLT5_VPROC1_FYB    (85953)	/* 10MV */
#define CPU_DVFS_VOLT6_VPROC1_FYB    (83549)	/* 10MV */
#define CPU_DVFS_VOLT7_VPROC1_FYB    (81145)	/* 10MV */
#define CPU_DVFS_VOLT8_VPROC1_FYB    (78139)		/* 10MV */
#define CPU_DVFS_VOLT9_VPROC1_FYB    (75735)		/* 10MV */
#define CPU_DVFS_VOLT10_VPROC1_FYB   (75014)	/* 10MV */
#define CPU_DVFS_VOLT11_VPROC1_FYB   (75014)	/* 10MV */
#define CPU_DVFS_VOLT12_VPROC1_FYB   (75014)	/* 10MV */
#define CPU_DVFS_VOLT13_VPROC1_FYB   (75014)	/* 10MV */
#define CPU_DVFS_VOLT14_VPROC1_FYB   (75014)	/* 10MV */
#define CPU_DVFS_VOLT15_VPROC1_FYB   (75014)	/* 10MV */

/* for DVFS OPP table LL/SB */
#define CPU_DVFS_FREQ0_LL_SB    (1378000)	/* KHz */
#define CPU_DVFS_FREQ1_LL_SB    (1300000)	/* KHz */
#define CPU_DVFS_FREQ2_LL_SB    (1248000)	/* KHz */
#define CPU_DVFS_FREQ3_LL_SB    (1183000)	/* KHz */
#define CPU_DVFS_FREQ4_LL_SB    (1105000)	/* KHz */
#define CPU_DVFS_FREQ5_LL_SB    (1027000)	/* KHz */
#define CPU_DVFS_FREQ6_LL_SB    (975000)	/* KHz */
#define CPU_DVFS_FREQ7_LL_SB    (923000)	/* KHz */
#define CPU_DVFS_FREQ8_LL_SB    (858000)	/* KHz */
#define CPU_DVFS_FREQ9_LL_SB    (806000)	/* KHz */
#define CPU_DVFS_FREQ10_LL_SB    (741000)	/* KHz */
#define CPU_DVFS_FREQ11_LL_SB    (663000)	/* KHz */
#define CPU_DVFS_FREQ12_LL_SB    (585000)	/* KHz */
#define CPU_DVFS_FREQ13_LL_SB    (520000)	/* KHz */
#define CPU_DVFS_FREQ14_LL_SB    (442000)	/* KHz */
#define CPU_DVFS_FREQ15_LL_SB    (351000)	/* KHz */

/* for DVFS OPP table L/SB */
#define CPU_DVFS_FREQ0_L_SB    (1521000)	/* KHz */
#define CPU_DVFS_FREQ1_L_SB    (1430000)	/* KHz */
#define CPU_DVFS_FREQ2_L_SB    (1365000)	/* KHz */
#define CPU_DVFS_FREQ3_L_SB    (1274000)	/* KHz */
#define CPU_DVFS_FREQ4_L_SB    (1183000)	/* KHz */
#define CPU_DVFS_FREQ5_L_SB    (1092000)	/* KHz */
#define CPU_DVFS_FREQ6_L_SB    (1014000)	/* KHz */
#define CPU_DVFS_FREQ7_L_SB    (962000)	/* KHz */
#define CPU_DVFS_FREQ8_L_SB    (884000)	/* KHz */
#define CPU_DVFS_FREQ9_L_SB    (819000)	/* KHz */
#define CPU_DVFS_FREQ10_L_SB    (741000)	/* KHz */
#define CPU_DVFS_FREQ11_L_SB    (663000)	/* KHz */
#define CPU_DVFS_FREQ12_L_SB    (585000)	/* KHz */
#define CPU_DVFS_FREQ13_L_SB    (520000)	/* KHz */
#define CPU_DVFS_FREQ14_L_SB    (442000)	/* KHz */
#define CPU_DVFS_FREQ15_L_SB    (338000)	/* KHz */

/* for DVFS OPP table CCI/SB */
#define CPU_DVFS_FREQ0_CCI_SB    (832000)	/* KHz */
#define CPU_DVFS_FREQ1_CCI_SB    (780000)	/* KHz */
#define CPU_DVFS_FREQ2_CCI_SB    (741000)	/* KHz */
#define CPU_DVFS_FREQ3_CCI_SB    (689000)	/* KHz */
#define CPU_DVFS_FREQ4_CCI_SB    (637000)	/* KHz */
#define CPU_DVFS_FREQ5_CCI_SB    (585000)	/* KHz */
#define CPU_DVFS_FREQ6_CCI_SB    (546000)	/* KHz */
#define CPU_DVFS_FREQ7_CCI_SB    (507000)	/* KHz */
#define CPU_DVFS_FREQ8_CCI_SB    (468000)	/* KHz */
#define CPU_DVFS_FREQ9_CCI_SB    (429000)	/* KHz */
#define CPU_DVFS_FREQ10_CCI_SB    (390000)	/* KHz */
#define CPU_DVFS_FREQ11_CCI_SB    (338000)	/* KHz */
#define CPU_DVFS_FREQ12_CCI_SB    (299000)	/* KHz */
#define CPU_DVFS_FREQ13_CCI_SB    (247000)	/* KHz */
#define CPU_DVFS_FREQ14_CCI_SB    (208000)	/* KHz */
#define CPU_DVFS_FREQ15_CCI_SB    (156000)	/* KHz */

/* for DVFS OPP table B/SB */
#define CPU_DVFS_FREQ0_B_SB    (1716000)	/* KHz */
#define CPU_DVFS_FREQ1_B_SB    (1612000)	/* KHz */
#define CPU_DVFS_FREQ2_B_SB    (1521000)	/* KHz */
#define CPU_DVFS_FREQ3_B_SB    (1417000)	/* KHz */
#define CPU_DVFS_FREQ4_B_SB    (1313000)	/* KHz */
#define CPU_DVFS_FREQ5_B_SB    (1196000)	/* KHz */
#define CPU_DVFS_FREQ6_B_SB    (1105000)	/* KHz */
#define CPU_DVFS_FREQ7_B_SB    (1014000)	/* KHz */
#define CPU_DVFS_FREQ8_B_SB    (897000)	/* KHz */
#define CPU_DVFS_FREQ9_B_SB    (806000)	/* KHz */
#define CPU_DVFS_FREQ10_B_SB    (715000)	/* KHz */
#define CPU_DVFS_FREQ11_B_SB    (650000)	/* KHz */
#define CPU_DVFS_FREQ12_B_SB    (572000)	/* KHz */
#define CPU_DVFS_FREQ13_B_SB    (507000)	/* KHz */
#define CPU_DVFS_FREQ14_B_SB    (429000)	/* KHz */
#define CPU_DVFS_FREQ15_B_SB    (338000)	/* KHz */

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

OPP_TBL(LL, FYA, 3, 1);
OPP_TBL(L, FYA, 3, 1);
OPP_TBL(CCI, FYA, 3, 1);
OPP_TBL(B, FYA, 3, 1);

OPP_TBL(LL, FYB, 4, 1);
OPP_TBL(L, FYB, 4, 1);
OPP_TBL(CCI, FYB, 4, 1);
OPP_TBL(B, FYB, 4, 1);

struct opp_tbl_info opp_tbls[NR_MT_CPU_DVFS][4] = {
	/* LL */
	{
		[CPU_LV_TO_OPP_IDX(CPU_LEVEL_0)] = {opp_tbl_LL_e1_0, ARRAY_SIZE(opp_tbl_LL_e1_0),},
		[CPU_LV_TO_OPP_IDX(CPU_LEVEL_1)] = {opp_tbl_LL_e2_0, ARRAY_SIZE(opp_tbl_LL_e2_0),},
		[CPU_LV_TO_OPP_IDX(CPU_LEVEL_2)] = {opp_tbl_LL_e3_0, ARRAY_SIZE(opp_tbl_LL_e3_0),},
		[CPU_LV_TO_OPP_IDX(CPU_LEVEL_3)] = {opp_tbl_LL_e4_0, ARRAY_SIZE(opp_tbl_LL_e4_0),},
	},
	/* L */
	{
		[CPU_LV_TO_OPP_IDX(CPU_LEVEL_0)] = {opp_tbl_L_e1_0, ARRAY_SIZE(opp_tbl_L_e1_0),},
		[CPU_LV_TO_OPP_IDX(CPU_LEVEL_1)] = {opp_tbl_L_e2_0, ARRAY_SIZE(opp_tbl_L_e2_0),},
		[CPU_LV_TO_OPP_IDX(CPU_LEVEL_2)] = {opp_tbl_L_e3_0, ARRAY_SIZE(opp_tbl_L_e3_0),},
		[CPU_LV_TO_OPP_IDX(CPU_LEVEL_3)] = {opp_tbl_L_e4_0, ARRAY_SIZE(opp_tbl_L_e4_0),},
	},
	/* B */
	{
		[CPU_LV_TO_OPP_IDX(CPU_LEVEL_0)] = {opp_tbl_B_e1_0, ARRAY_SIZE(opp_tbl_B_e1_0),},
		[CPU_LV_TO_OPP_IDX(CPU_LEVEL_1)] = {opp_tbl_B_e2_0, ARRAY_SIZE(opp_tbl_B_e2_0),},
		[CPU_LV_TO_OPP_IDX(CPU_LEVEL_2)] = {opp_tbl_B_e3_0, ARRAY_SIZE(opp_tbl_B_e3_0),},
		[CPU_LV_TO_OPP_IDX(CPU_LEVEL_3)] = {opp_tbl_B_e4_0, ARRAY_SIZE(opp_tbl_B_e4_0),},
	},
	/* CCI */
	{
		[CPU_LV_TO_OPP_IDX(CPU_LEVEL_0)] = {opp_tbl_CCI_e1_0, ARRAY_SIZE(opp_tbl_CCI_e1_0),},
		[CPU_LV_TO_OPP_IDX(CPU_LEVEL_1)] = {opp_tbl_CCI_e2_0, ARRAY_SIZE(opp_tbl_CCI_e2_0),},
		[CPU_LV_TO_OPP_IDX(CPU_LEVEL_2)] = {opp_tbl_CCI_e3_0, ARRAY_SIZE(opp_tbl_CCI_e3_0),},
		[CPU_LV_TO_OPP_IDX(CPU_LEVEL_3)] = {opp_tbl_CCI_e4_0, ARRAY_SIZE(opp_tbl_CCI_e4_0),},
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
	FP(4, 1),
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
	FP(4, 2),
	FP(4, 2),
	FP(4, 2),
	FP(4, 2),
	FP(4, 2),
	FP(4, 4),
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
	FP(4, 1),
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
	FP(4, 1),
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
	FP(4, 4),
	FP(4, 4),
};

static struct mt_cpu_freq_method opp_tbl_method_B_e2[] = {
	/* Target Frequency,	POS, CLK */
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

static struct mt_cpu_freq_method opp_tbl_method_LL_e3[] = {
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

static struct mt_cpu_freq_method opp_tbl_method_L_e3[] = {
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

static struct mt_cpu_freq_method opp_tbl_method_CCI_e3[] = {
	/* Target Frequency,		POS, CLK */
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
	FP(4, 4),
	FP(4, 4),
};

static struct mt_cpu_freq_method opp_tbl_method_B_e3[] = {
	/* Target Frequency,	POS, CLK */
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

static struct mt_cpu_freq_method opp_tbl_method_LL_e4[] = {
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

static struct mt_cpu_freq_method opp_tbl_method_L_e4[] = {
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

static struct mt_cpu_freq_method opp_tbl_method_CCI_e4[] = {
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
	FP(4, 4),
	FP(4, 4),
	FP(4, 4),
};

static struct mt_cpu_freq_method opp_tbl_method_B_e4[] = {
	/* Target Frequency,	POS, CLK */
	FP(1, 1),
	FP(1, 1),
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

struct opp_tbl_m_info opp_tbls_m[NR_MT_CPU_DVFS][4] = {
	/* LL */
	{
		[CPU_LV_TO_OPP_IDX(CPU_LEVEL_0)] = {opp_tbl_method_LL_e1,},
		[CPU_LV_TO_OPP_IDX(CPU_LEVEL_1)] = {opp_tbl_method_LL_e2,},
		[CPU_LV_TO_OPP_IDX(CPU_LEVEL_2)] = {opp_tbl_method_LL_e3,},
		[CPU_LV_TO_OPP_IDX(CPU_LEVEL_3)] = {opp_tbl_method_LL_e4,},
	},
	/* L */
	{
		[CPU_LV_TO_OPP_IDX(CPU_LEVEL_0)] = {opp_tbl_method_L_e1,},
		[CPU_LV_TO_OPP_IDX(CPU_LEVEL_1)] = {opp_tbl_method_L_e2,},
		[CPU_LV_TO_OPP_IDX(CPU_LEVEL_2)] = {opp_tbl_method_L_e3,},
		[CPU_LV_TO_OPP_IDX(CPU_LEVEL_3)] = {opp_tbl_method_L_e4,},
	},
	/* B */
	{
		[CPU_LV_TO_OPP_IDX(CPU_LEVEL_0)] = {opp_tbl_method_B_e1,},
		[CPU_LV_TO_OPP_IDX(CPU_LEVEL_1)] = {opp_tbl_method_B_e2,},
		[CPU_LV_TO_OPP_IDX(CPU_LEVEL_2)] = {opp_tbl_method_B_e3,},
		[CPU_LV_TO_OPP_IDX(CPU_LEVEL_3)] = {opp_tbl_method_B_e4,},
	},
	/* CCI */
	{
		[CPU_LV_TO_OPP_IDX(CPU_LEVEL_0)] = {opp_tbl_method_CCI_e1,},
		[CPU_LV_TO_OPP_IDX(CPU_LEVEL_1)] = {opp_tbl_method_CCI_e2,},
		[CPU_LV_TO_OPP_IDX(CPU_LEVEL_2)] = {opp_tbl_method_CCI_e3,},
		[CPU_LV_TO_OPP_IDX(CPU_LEVEL_3)] = {opp_tbl_method_CCI_e4,},
	},
};
