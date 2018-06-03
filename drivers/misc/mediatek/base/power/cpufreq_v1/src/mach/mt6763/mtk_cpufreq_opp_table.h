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

/* for DVFS OPP table LL B */
#define CPU_DVFS_FREQ0_LL_FY		1508000		/* KHz */
#define CPU_DVFS_FREQ1_LL_FY		1482000		/* KHz */
#define CPU_DVFS_FREQ2_LL_FY		1430000		/* KHz */
#define CPU_DVFS_FREQ3_LL_FY		1391000		/* KHz */
#define CPU_DVFS_FREQ4_LL_FY		1352000		/* KHz */
#define CPU_DVFS_FREQ5_LL_FY		1300000		/* KHz */
#define CPU_DVFS_FREQ6_LL_FY		1248000		/* KHz */
#define CPU_DVFS_FREQ7_LL_FY		1196000		/* KHz */
#define CPU_DVFS_FREQ8_LL_FY		1144000		/* KHz */
#define CPU_DVFS_FREQ9_LL_FY		1079000		/* KHz */
#define CPU_DVFS_FREQ10_LL_FY		 1014000		/* KHz */
#define CPU_DVFS_FREQ11_LL_FY		 910000		/* KHz */
#define CPU_DVFS_FREQ12_LL_FY		 780000	/* KHz */
#define CPU_DVFS_FREQ13_LL_FY		 663000	/* KHz */
#define CPU_DVFS_FREQ14_LL_FY		 520000	/* KHz */
#define CPU_DVFS_FREQ15_LL_FY		 338000	/* KHz */

/* for DVFS OPP table L */
#define CPU_DVFS_FREQ0_L_FY			2002000		/* KHz */
#define CPU_DVFS_FREQ1_L_FY			1989000		/* KHz */
#define CPU_DVFS_FREQ2_L_FY			1963000		/* KHz */
#define CPU_DVFS_FREQ3_L_FY			1937000		/* KHz */
#define CPU_DVFS_FREQ4_L_FY			1924000		/* KHz */
#define CPU_DVFS_FREQ5_L_FY			1859000		/* KHz */
#define CPU_DVFS_FREQ6_L_FY			1781000		/* KHz */
#define CPU_DVFS_FREQ7_L_FY			1716000		/* KHz */
#define CPU_DVFS_FREQ8_L_FY			1638000		/* KHz */
#define CPU_DVFS_FREQ9_L_FY			1547000		/* KHz */
#define CPU_DVFS_FREQ10_L_FY		1469000		/* KHz */
#define CPU_DVFS_FREQ11_L_FY		1313000		/* KHz */
#define CPU_DVFS_FREQ12_L_FY		1131000		/* KHz */
#define CPU_DVFS_FREQ13_L_FY		 962000		/* KHz */
#define CPU_DVFS_FREQ14_L_FY		 767000		/* KHz */
#define CPU_DVFS_FREQ15_L_FY		 520000		/* KHz */

/* for DVFS OPP table CCI */
#define CPU_DVFS_FREQ0_CCI_FY		 910000		/* KHz */
#define CPU_DVFS_FREQ1_CCI_FY		 884000		/* KHz */
#define CPU_DVFS_FREQ2_CCI_FY		 858000		/* KHz */
#define CPU_DVFS_FREQ3_CCI_FY		 819000		/* KHz */
#define CPU_DVFS_FREQ4_CCI_FY		 806000		/* KHz */
#define CPU_DVFS_FREQ5_CCI_FY		 767000		/* KHz */
#define CPU_DVFS_FREQ6_CCI_FY		 741000		/* KHz */
#define CPU_DVFS_FREQ7_CCI_FY		 715000		/* KHz */
#define CPU_DVFS_FREQ8_CCI_FY		 689000		/* KHz */
#define CPU_DVFS_FREQ9_CCI_FY		 650000		/* KHz */
#define CPU_DVFS_FREQ10_CCI_FY		 624000		/* KHz */
#define CPU_DVFS_FREQ11_CCI_FY		 559000		/* KHz */
#define CPU_DVFS_FREQ12_CCI_FY		 481000		/* KHz */
#define CPU_DVFS_FREQ13_CCI_FY		 416000		/* KHz */
#define CPU_DVFS_FREQ14_CCI_FY		 325000		/* KHz */
#define CPU_DVFS_FREQ15_CCI_FY		 195000		/* KHz */

/* for DVFS OPP table LL|L|CCI */
#define CPU_DVFS_VOLT0_VPROC1_FY	 96875		/* 10uV */
#define CPU_DVFS_VOLT1_VPROC1_FY	 95625		/* 10uV */
#define CPU_DVFS_VOLT2_VPROC1_FY	 93750		/* 10uV */
#define CPU_DVFS_VOLT3_VPROC1_FY	 91875		/* 10uV */
#define CPU_DVFS_VOLT4_VPROC1_FY	 90625		/* 10uV */
#define CPU_DVFS_VOLT5_VPROC1_FY	 88750		/* 10uV */
#define CPU_DVFS_VOLT6_VPROC1_FY	 86875		/* 10uV */
#define CPU_DVFS_VOLT7_VPROC1_FY	 85625		/* 10uV */
#define CPU_DVFS_VOLT8_VPROC1_FY	 83750		/* 10uV */
#define CPU_DVFS_VOLT9_VPROC1_FY	 81875		/* 10uV */
#define CPU_DVFS_VOLT10_VPROC1_FY	 80000		/* 10uV */
#define CPU_DVFS_VOLT11_VPROC1_FY	 76875		/* 10uV */
#define CPU_DVFS_VOLT12_VPROC1_FY	 73125		/* 10uV */
#define CPU_DVFS_VOLT13_VPROC1_FY	 70000		/* 10uV */
#define CPU_DVFS_VOLT14_VPROC1_FY	 65625		/* 10uV */
#define CPU_DVFS_VOLT15_VPROC1_FY	 60000		/* 10uV */

/* for DVFS OPP table LL B FY*/
#define CPU_DVFS_FREQ0_LL_FY2		1690000		/* KHz */
#define CPU_DVFS_FREQ1_LL_FY2		1677000		/* KHz */
#define CPU_DVFS_FREQ2_LL_FY2		1651000		/* KHz */
#define CPU_DVFS_FREQ3_LL_FY2		1625000		/* KHz */
#define CPU_DVFS_FREQ4_LL_FY2		1547000		/* KHz */
#define CPU_DVFS_FREQ5_LL_FY2		1495000		/* KHz */
#define CPU_DVFS_FREQ6_LL_FY2		1417000		/* KHz */
#define CPU_DVFS_FREQ7_LL_FY2		1339000		/* KHz */
#define CPU_DVFS_FREQ8_LL_FY2		1248000		/* KHz */
#define CPU_DVFS_FREQ9_LL_FY2		1144000		/* KHz */
#define CPU_DVFS_FREQ10_LL_FY2		1014000		/* KHz */
#define CPU_DVFS_FREQ11_LL_FY2		884000		/* KHz */
#define CPU_DVFS_FREQ12_LL_FY2		 780000		/* KHz */
#define CPU_DVFS_FREQ13_LL_FY2		 663000		/* KHz */
#define CPU_DVFS_FREQ14_LL_FY2		 520000		/* KHz */
#define CPU_DVFS_FREQ15_LL_FY2		 338000		/* KHz */

/* for DVFS OPP table L B FY 2457 */
#define CPU_DVFS_FREQ0_L_FY2		2340000		/* KHz */
#define CPU_DVFS_FREQ1_L_FY2		2288000		/* KHz */
#define CPU_DVFS_FREQ2_L_FY2		2236000		/* KHz */
#define CPU_DVFS_FREQ3_L_FY2		2184000		/* KHz */
#define CPU_DVFS_FREQ4_L_FY2		2119000		/* KHz */
#define CPU_DVFS_FREQ5_L_FY2		2054000		/* KHz */
#define CPU_DVFS_FREQ6_L_FY2		1989000		/* KHz */
#define CPU_DVFS_FREQ7_L_FY2		1924000		/* KHz */
#define CPU_DVFS_FREQ8_L_FY2		1781000		/* KHz */
#define CPU_DVFS_FREQ9_L_FY2		1638000		/* KHz */
#define CPU_DVFS_FREQ10_L_FY2		1469000		/* KHz */
#define CPU_DVFS_FREQ11_L_FY2		1287000		/* KHz */
#define CPU_DVFS_FREQ12_L_FY2		1131000		/* KHz */
#define CPU_DVFS_FREQ13_L_FY2		962000		/* KHz */
#define CPU_DVFS_FREQ14_L_FY2		 767000		/* KHz */
#define CPU_DVFS_FREQ15_L_FY2		 520000		/* KHz */

/* for DVFS OPP table CCI */
#define CPU_DVFS_FREQ0_CCI_FY2		1040000		/* KHz */
#define CPU_DVFS_FREQ1_CCI_FY2		1027000		/* KHz */
#define CPU_DVFS_FREQ2_CCI_FY2		1001000		/* KHz */
#define CPU_DVFS_FREQ3_CCI_FY2		988000		/* KHz */
#define CPU_DVFS_FREQ4_CCI_FY2		 936000		/* KHz */
#define CPU_DVFS_FREQ5_CCI_FY2		 897000		/* KHz */
#define CPU_DVFS_FREQ6_CCI_FY2		 845000		/* KHz */
#define CPU_DVFS_FREQ7_CCI_FY2		 793000		/* KHz */
#define CPU_DVFS_FREQ8_CCI_FY2		 741000		/* KHz */
#define CPU_DVFS_FREQ9_CCI_FY2		 689000		/* KHz */
#define CPU_DVFS_FREQ10_CCI_FY2		 624000		/* KHz */
#define CPU_DVFS_FREQ11_CCI_FY2		 546000		/* KHz */
#define CPU_DVFS_FREQ12_CCI_FY2		 481000		/* KHz */
#define CPU_DVFS_FREQ13_CCI_FY2		 416000		/* KHz */
#define CPU_DVFS_FREQ14_CCI_FY2		 325000		/* KHz */
#define CPU_DVFS_FREQ15_CCI_FY2		 195000		/* KHz */

/* for DVFS OPP table LL|L|CCI */
#define CPU_DVFS_VOLT0_VPROC1_FY2	 111875		/* 10uV */
#define CPU_DVFS_VOLT1_VPROC1_FY2	 108750		/* 10uV */
#define CPU_DVFS_VOLT2_VPROC1_FY2	 105000		/* 10uV */
#define CPU_DVFS_VOLT3_VPROC1_FY2	 101875		/* 10uV */
#define CPU_DVFS_VOLT4_VPROC1_FY2	 98750		/* 10uV */
#define CPU_DVFS_VOLT5_VPROC1_FY2	 96250		/* 10uV */
#define CPU_DVFS_VOLT6_VPROC1_FY2	 93125		/* 10uV */
#define CPU_DVFS_VOLT7_VPROC1_FY2	 90000		/* 10uV */
#define CPU_DVFS_VOLT8_VPROC1_FY2	 86875		/* 10uV */
#define CPU_DVFS_VOLT9_VPROC1_FY2	 83750		/* 10uV */
#define CPU_DVFS_VOLT10_VPROC1_FY2	 80000		/* 10uV */
#define CPU_DVFS_VOLT11_VPROC1_FY2	 76250		/* 10uV */
#define CPU_DVFS_VOLT12_VPROC1_FY2	 73125		/* 10uV */
#define CPU_DVFS_VOLT13_VPROC1_FY2	 70000		/* 10uV */
#define CPU_DVFS_VOLT14_VPROC1_FY2	 65625		/* 10uV */
#define CPU_DVFS_VOLT15_VPROC1_FY2	 60000		/* 10uV */

/* for DVFS OPP table LL B SB*/
#define CPU_DVFS_FREQ0_LL_SB2		1794000		/* KHz */
#define CPU_DVFS_FREQ1_LL_SB2		1755000		/* KHz */
#define CPU_DVFS_FREQ2_LL_SB2		1703000		/* KHz */
#define CPU_DVFS_FREQ3_LL_SB2		1664000		/* KHz */
#define CPU_DVFS_FREQ4_LL_SB2		1612000		/* KHz */
#define CPU_DVFS_FREQ5_LL_SB2		1534000		/* KHz */
#define CPU_DVFS_FREQ6_LL_SB2		1456000		/* KHz */
#define CPU_DVFS_FREQ7_LL_SB2		1391000		/* KHz */
#define CPU_DVFS_FREQ8_LL_SB2		1287000		/* KHz */
#define CPU_DVFS_FREQ9_LL_SB2		1157000		/* KHz */
#define CPU_DVFS_FREQ10_LL_SB2		1014000		/* KHz */
#define CPU_DVFS_FREQ11_LL_SB2		910000		/* KHz */
#define CPU_DVFS_FREQ12_LL_SB2		 806000		/* KHz */
#define CPU_DVFS_FREQ13_LL_SB2		 689000		/* KHz */
#define CPU_DVFS_FREQ14_LL_SB2		 520000		/* KHz */
#define CPU_DVFS_FREQ15_LL_SB2		 338000		/* KHz */

/* for DVFS OPP table L B SB 2457 */
#define CPU_DVFS_FREQ0_L_SB2		2340000		/* KHz */
#define CPU_DVFS_FREQ1_L_SB2		2301000		/* KHz */
#define CPU_DVFS_FREQ2_L_SB2		2262000		/* KHz */
#define CPU_DVFS_FREQ3_L_SB2		2223000		/* KHz */
#define CPU_DVFS_FREQ4_L_SB2		2171000		/* KHz */
#define CPU_DVFS_FREQ5_L_SB2		2106000		/* KHz */
#define CPU_DVFS_FREQ6_L_SB2		2028000		/* KHz */
#define CPU_DVFS_FREQ7_L_SB2		1963000		/* KHz */
#define CPU_DVFS_FREQ8_L_SB2		1833000		/* KHz */
#define CPU_DVFS_FREQ9_L_SB2		1664000		/* KHz */
#define CPU_DVFS_FREQ10_L_SB2		1469000		/* KHz */
#define CPU_DVFS_FREQ11_L_SB2		1313000		/* KHz */
#define CPU_DVFS_FREQ12_L_SB2		1157000		/* KHz */
#define CPU_DVFS_FREQ13_L_SB2		1001000		/* KHz */
#define CPU_DVFS_FREQ14_L_SB2		 767000		/* KHz */
#define CPU_DVFS_FREQ15_L_SB2		 520000		/* KHz */

/* for DVFS OPP table CCI */
#define CPU_DVFS_FREQ0_CCI_SB2		1040000		/* KHz */
#define CPU_DVFS_FREQ1_CCI_SB2		1027000		/* KHz */
#define CPU_DVFS_FREQ2_CCI_SB2		1014000		/* KHz */
#define CPU_DVFS_FREQ3_CCI_SB2		1001000		/* KHz */
#define CPU_DVFS_FREQ4_CCI_SB2		975000		/* KHz */
#define CPU_DVFS_FREQ5_CCI_SB2		 923000		/* KHz */
#define CPU_DVFS_FREQ6_CCI_SB2		 871000		/* KHz */
#define CPU_DVFS_FREQ7_CCI_SB2		 819000		/* KHz */
#define CPU_DVFS_FREQ8_CCI_SB2		 767000		/* KHz */
#define CPU_DVFS_FREQ9_CCI_SB2		 689000		/* KHz */
#define CPU_DVFS_FREQ10_CCI_SB2		 624000		/* KHz */
#define CPU_DVFS_FREQ11_CCI_SB2		 559000		/* KHz */
#define CPU_DVFS_FREQ12_CCI_SB2		 494000		/* KHz */
#define CPU_DVFS_FREQ13_CCI_SB2		 429000		/* KHz */
#define CPU_DVFS_FREQ14_CCI_SB2		 325000		/* KHz */
#define CPU_DVFS_FREQ15_CCI_SB2		 195000		/* KHz */

/* for DVFS OPP table LL|L|CCI */
#define CPU_DVFS_VOLT0_VPROC1_SB2	 111875		/* 10uV */
#define CPU_DVFS_VOLT1_VPROC1_SB2	 109375		/* 10uV */
#define CPU_DVFS_VOLT2_VPROC1_SB2	 106875		/* 10uV */
#define CPU_DVFS_VOLT3_VPROC1_SB2	 104375		/* 10uV */
#define CPU_DVFS_VOLT4_VPROC1_SB2	 101250		/* 10uV */
#define CPU_DVFS_VOLT5_VPROC1_SB2	 98125		/* 10uV */
#define CPU_DVFS_VOLT6_VPROC1_SB2	 95000		/* 10uV */
#define CPU_DVFS_VOLT7_VPROC1_SB2	 91875		/* 10uV */
#define CPU_DVFS_VOLT8_VPROC1_SB2	 88125		/* 10uV */
#define CPU_DVFS_VOLT9_VPROC1_SB2	 84375		/* 10uV */
#define CPU_DVFS_VOLT10_VPROC1_SB2	 80000		/* 10uV */
#define CPU_DVFS_VOLT11_VPROC1_SB2	 76875		/* 10uV */
#define CPU_DVFS_VOLT12_VPROC1_SB2	 73750		/* 10uV */
#define CPU_DVFS_VOLT13_VPROC1_SB2	 70625		/* 10uV */
#define CPU_DVFS_VOLT14_VPROC1_SB2	 65625		/* 10uV */
#define CPU_DVFS_VOLT15_VPROC1_SB2	 60000		/* 10uV */

/* for DVFS OPP table LL B+  */
#define CPU_DVFS_FREQ0_LL_FY3		1690000		/* KHz */
#define CPU_DVFS_FREQ1_LL_FY3		1677000		/* KHz */
#define CPU_DVFS_FREQ2_LL_FY3		1664000		/* KHz */
#define CPU_DVFS_FREQ3_LL_FY3		1651000		/* KHz */
#define CPU_DVFS_FREQ4_LL_FY3		1612000		/* KHz */
#define CPU_DVFS_FREQ5_LL_FY3		1534000		/* KHz */
#define CPU_DVFS_FREQ6_LL_FY3		1456000		/* KHz */
#define CPU_DVFS_FREQ7_LL_FY3		1391000		/* KHz */
#define CPU_DVFS_FREQ8_LL_FY3		1287000		/* KHz */
#define CPU_DVFS_FREQ9_LL_FY3		1157000		/* KHz */
#define CPU_DVFS_FREQ10_LL_FY3		1014000		/* KHz */
#define CPU_DVFS_FREQ11_LL_FY3		910000		/* KHz */
#define CPU_DVFS_FREQ12_LL_FY3		 806000		/* KHz */
#define CPU_DVFS_FREQ13_LL_FY3		 689000		/* KHz */
#define CPU_DVFS_FREQ14_LL_FY3		 520000		/* KHz */
#define CPU_DVFS_FREQ15_LL_FY3		 338000		/* KHz */

/* for DVFS OPP table L B+ 2626*/
#define CPU_DVFS_FREQ0_L_FY3		2496000		/* KHz */
#define CPU_DVFS_FREQ1_L_FY3		2418000		/* KHz */
#define CPU_DVFS_FREQ2_L_FY3		2340000		/* KHz */
#define CPU_DVFS_FREQ3_L_FY3		2262000		/* KHz */
#define CPU_DVFS_FREQ4_L_FY3		2171000		/* KHz */
#define CPU_DVFS_FREQ5_L_FY3		2106000		/* KHz */
#define CPU_DVFS_FREQ6_L_FY3		2028000		/* KHz */
#define CPU_DVFS_FREQ7_L_FY3		1963000		/* KHz */
#define CPU_DVFS_FREQ8_L_FY3		1833000		/* KHz */
#define CPU_DVFS_FREQ9_L_FY3		1664000		/* KHz */
#define CPU_DVFS_FREQ10_L_FY3		1469000		/* KHz */
#define CPU_DVFS_FREQ11_L_FY3		1313000		/* KHz */
#define CPU_DVFS_FREQ12_L_FY3		1157000		/* KHz */
#define CPU_DVFS_FREQ13_L_FY3		1001000		/* KHz */
#define CPU_DVFS_FREQ14_L_FY3		 767000		/* KHz */
#define CPU_DVFS_FREQ15_L_FY3		 520000		/* KHz */

/* for DVFS OPP table CCI */
#define CPU_DVFS_FREQ0_CCI_FY3		1092000		/* KHz */
#define CPU_DVFS_FREQ1_CCI_FY3		1066000		/* KHz */
#define CPU_DVFS_FREQ2_CCI_FY3		1040000		/* KHz */
#define CPU_DVFS_FREQ3_CCI_FY3		1014000		/* KHz */
#define CPU_DVFS_FREQ4_CCI_FY3		975000		/* KHz */
#define CPU_DVFS_FREQ5_CCI_FY3		 923000		/* KHz */
#define CPU_DVFS_FREQ6_CCI_FY3		 871000		/* KHz */
#define CPU_DVFS_FREQ7_CCI_FY3		 819000		/* KHz */
#define CPU_DVFS_FREQ8_CCI_FY3		 767000		/* KHz */
#define CPU_DVFS_FREQ9_CCI_FY3		 689000		/* KHz */
#define CPU_DVFS_FREQ10_CCI_FY3		 624000		/* KHz */
#define CPU_DVFS_FREQ11_CCI_FY3		 559000		/* KHz */
#define CPU_DVFS_FREQ12_CCI_FY3		 494000		/* KHz */
#define CPU_DVFS_FREQ13_CCI_FY3		 429000		/* KHz */
#define CPU_DVFS_FREQ14_CCI_FY3		 325000		/* KHz */
#define CPU_DVFS_FREQ15_CCI_FY3		 195000		/* KHz */

/* for DVFS OPP table LL|L|CCI */
#define CPU_DVFS_VOLT0_VPROC1_FY3	 111875		/* 10uV */
#define CPU_DVFS_VOLT1_VPROC1_FY3	 109375		/* 10uV */
#define CPU_DVFS_VOLT2_VPROC1_FY3	 106875		/* 10uV */
#define CPU_DVFS_VOLT3_VPROC1_FY3	 104375		/* 10uV */
#define CPU_DVFS_VOLT4_VPROC1_FY3	 101250		/* 10uV */
#define CPU_DVFS_VOLT5_VPROC1_FY3	 98125		/* 10uV */
#define CPU_DVFS_VOLT6_VPROC1_FY3	 95000		/* 10uV */
#define CPU_DVFS_VOLT7_VPROC1_FY3	 91875		/* 10uV */
#define CPU_DVFS_VOLT8_VPROC1_FY3	 88125		/* 10uV */
#define CPU_DVFS_VOLT9_VPROC1_FY3	 84375		/* 10uV */
#define CPU_DVFS_VOLT10_VPROC1_FY3	 80000		/* 10uV */
#define CPU_DVFS_VOLT11_VPROC1_FY3	 76875		/* 10uV */
#define CPU_DVFS_VOLT12_VPROC1_FY3	 73750		/* 10uV */
#define CPU_DVFS_VOLT13_VPROC1_FY3	 70625		/* 10uV */
#define CPU_DVFS_VOLT14_VPROC1_FY3	 65625		/* 10uV */
#define CPU_DVFS_VOLT15_VPROC1_FY3	 60000		/* 10uV */

/* for DVFS OPP table LL B+  */
#define CPU_DVFS_FREQ0_LL_SB3		1885000		/* KHz */
#define CPU_DVFS_FREQ1_LL_SB3		1820000		/* KHz */
#define CPU_DVFS_FREQ2_LL_SB3		1755000		/* KHz */
#define CPU_DVFS_FREQ3_LL_SB3		1690000		/* KHz */
#define CPU_DVFS_FREQ4_LL_SB3		1612000		/* KHz */
#define CPU_DVFS_FREQ5_LL_SB3		1534000		/* KHz */
#define CPU_DVFS_FREQ6_LL_SB3		1456000		/* KHz */
#define CPU_DVFS_FREQ7_LL_SB3		1391000		/* KHz */
#define CPU_DVFS_FREQ8_LL_SB3		1287000		/* KHz */
#define CPU_DVFS_FREQ9_LL_SB3		1157000		/* KHz */
#define CPU_DVFS_FREQ10_LL_SB3		1014000		/* KHz */
#define CPU_DVFS_FREQ11_LL_SB3		910000		/* KHz */
#define CPU_DVFS_FREQ12_LL_SB3		 806000		/* KHz */
#define CPU_DVFS_FREQ13_LL_SB3		 689000		/* KHz */
#define CPU_DVFS_FREQ14_LL_SB3		 520000		/* KHz */
#define CPU_DVFS_FREQ15_LL_SB3		 338000		/* KHz */

/* for DVFS OPP table L B+ 2626*/
#define CPU_DVFS_FREQ0_L_SB3		2496000		/* KHz */
#define CPU_DVFS_FREQ1_L_SB3		2418000		/* KHz */
#define CPU_DVFS_FREQ2_L_SB3		2340000		/* KHz */
#define CPU_DVFS_FREQ3_L_SB3		2262000		/* KHz */
#define CPU_DVFS_FREQ4_L_SB3		2171000		/* KHz */
#define CPU_DVFS_FREQ5_L_SB3		2106000		/* KHz */
#define CPU_DVFS_FREQ6_L_SB3		2028000		/* KHz */
#define CPU_DVFS_FREQ7_L_SB3		1963000		/* KHz */
#define CPU_DVFS_FREQ8_L_SB3		1833000		/* KHz */
#define CPU_DVFS_FREQ9_L_SB3		1664000		/* KHz */
#define CPU_DVFS_FREQ10_L_SB3		1469000		/* KHz */
#define CPU_DVFS_FREQ11_L_SB3		1313000		/* KHz */
#define CPU_DVFS_FREQ12_L_SB3		1157000		/* KHz */
#define CPU_DVFS_FREQ13_L_SB3		1001000		/* KHz */
#define CPU_DVFS_FREQ14_L_SB3		 767000		/* KHz */
#define CPU_DVFS_FREQ15_L_SB3		 520000		/* KHz */

/* for DVFS OPP table CCI */
#define CPU_DVFS_FREQ0_CCI_SB3		1092000		/* KHz */
#define CPU_DVFS_FREQ1_CCI_SB3		1066000		/* KHz */
#define CPU_DVFS_FREQ2_CCI_SB3		1040000		/* KHz */
#define CPU_DVFS_FREQ3_CCI_SB3		1014000		/* KHz */
#define CPU_DVFS_FREQ4_CCI_SB3		975000		/* KHz */
#define CPU_DVFS_FREQ5_CCI_SB3		 923000		/* KHz */
#define CPU_DVFS_FREQ6_CCI_SB3		 871000		/* KHz */
#define CPU_DVFS_FREQ7_CCI_SB3		 819000		/* KHz */
#define CPU_DVFS_FREQ8_CCI_SB3		 767000		/* KHz */
#define CPU_DVFS_FREQ9_CCI_SB3		 689000		/* KHz */
#define CPU_DVFS_FREQ10_CCI_SB3		 624000		/* KHz */
#define CPU_DVFS_FREQ11_CCI_SB3		 559000		/* KHz */
#define CPU_DVFS_FREQ12_CCI_SB3		 494000		/* KHz */
#define CPU_DVFS_FREQ13_CCI_SB3		 429000		/* KHz */
#define CPU_DVFS_FREQ14_CCI_SB3		 325000		/* KHz */
#define CPU_DVFS_FREQ15_CCI_SB3		 195000		/* KHz */

/* for DVFS OPP table LL|L|CCI */
#define CPU_DVFS_VOLT0_VPROC1_SB3	 111875		/* 10uV */
#define CPU_DVFS_VOLT1_VPROC1_SB3	 109375		/* 10uV */
#define CPU_DVFS_VOLT2_VPROC1_SB3	 106875		/* 10uV */
#define CPU_DVFS_VOLT3_VPROC1_SB3	 104375		/* 10uV */
#define CPU_DVFS_VOLT4_VPROC1_SB3	 101250		/* 10uV */
#define CPU_DVFS_VOLT5_VPROC1_SB3	 98125		/* 10uV */
#define CPU_DVFS_VOLT6_VPROC1_SB3	 95000		/* 10uV */
#define CPU_DVFS_VOLT7_VPROC1_SB3	 91875		/* 10uV */
#define CPU_DVFS_VOLT8_VPROC1_SB3	 88125		/* 10uV */
#define CPU_DVFS_VOLT9_VPROC1_SB3	 84375		/* 10uV */
#define CPU_DVFS_VOLT10_VPROC1_SB3	 80000		/* 10uV */
#define CPU_DVFS_VOLT11_VPROC1_SB3	 76875		/* 10uV */
#define CPU_DVFS_VOLT12_VPROC1_SB3	 73750		/* 10uV */
#define CPU_DVFS_VOLT13_VPROC1_SB3	 70625		/* 10uV */
#define CPU_DVFS_VOLT14_VPROC1_SB3	 65625		/* 10uV */
#define CPU_DVFS_VOLT15_VPROC1_SB3	 60000		/* 10uV */

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

OPP_TBL(LL, SB2, 2, 1);
OPP_TBL(L, SB2, 2, 1);
OPP_TBL(CCI, SB2, 2, 1);

OPP_TBL(LL, FY3, 3, 1);
OPP_TBL(L, FY3, 3, 1);
OPP_TBL(CCI, FY3, 3, 1);

OPP_TBL(LL, SB3, 4, 1);
OPP_TBL(L, SB3, 4, 1);
OPP_TBL(CCI, SB3, 4, 1);

struct opp_tbl_info opp_tbls[NR_MT_CPU_DVFS][NUM_CPU_LEVEL] = {		/* v1.1 */
	/* LL */
	{
		[CPU_LEVEL_0] = { opp_tbl_LL_e0_0, ARRAY_SIZE(opp_tbl_LL_e0_0) },
		[CPU_LEVEL_1] = { opp_tbl_LL_e1_0, ARRAY_SIZE(opp_tbl_LL_e1_0) },
		[CPU_LEVEL_2] = { opp_tbl_LL_e2_0, ARRAY_SIZE(opp_tbl_LL_e2_0) },
		[CPU_LEVEL_3] = { opp_tbl_LL_e3_0, ARRAY_SIZE(opp_tbl_LL_e3_0) },
		[CPU_LEVEL_4] = { opp_tbl_LL_e4_0, ARRAY_SIZE(opp_tbl_LL_e4_0) },
	},
	/* L */
	{
		[CPU_LEVEL_0] = { opp_tbl_L_e0_0, ARRAY_SIZE(opp_tbl_L_e0_0) },
		[CPU_LEVEL_1] = { opp_tbl_L_e1_0, ARRAY_SIZE(opp_tbl_L_e1_0) },
		[CPU_LEVEL_2] = { opp_tbl_L_e2_0, ARRAY_SIZE(opp_tbl_L_e2_0) },
		[CPU_LEVEL_3] = { opp_tbl_L_e3_0, ARRAY_SIZE(opp_tbl_L_e3_0) },
		[CPU_LEVEL_4] = { opp_tbl_L_e4_0, ARRAY_SIZE(opp_tbl_L_e4_0) },
	},
	/* CCI */
	{
		[CPU_LEVEL_0] = { opp_tbl_CCI_e0_0, ARRAY_SIZE(opp_tbl_CCI_e0_0) },
		[CPU_LEVEL_1] = { opp_tbl_CCI_e1_0, ARRAY_SIZE(opp_tbl_CCI_e1_0) },
		[CPU_LEVEL_2] = { opp_tbl_CCI_e2_0, ARRAY_SIZE(opp_tbl_CCI_e2_0) },
		[CPU_LEVEL_3] = { opp_tbl_CCI_e3_0, ARRAY_SIZE(opp_tbl_CCI_e3_0) },
		[CPU_LEVEL_4] = { opp_tbl_CCI_e4_0, ARRAY_SIZE(opp_tbl_CCI_e4_0) },
	},
};

/* 16 steps OPP table */
static struct mt_cpu_freq_method opp_tbl_method_LL_e0[] = {	/* FY */
	/* POS,	CLK */
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
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
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
	FP(1,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
	FP(2,	1),
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

static struct mt_cpu_freq_method opp_tbl_method_LL_e1[] = {	/* FY2 */
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
	FP(2,	1),
	FP(2,	2),
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
	FP(2,	1),
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
	FP(2,	2),
	FP(2,	2),
	FP(2,	2),
	FP(2,	2),
	FP(2,	2),
	FP(2,	2),
	FP(2,	4),
	FP(2,	4),
};

static struct mt_cpu_freq_method opp_tbl_method_LL_e2[] = {	/* SB2 */
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
	FP(2,	2),
	FP(2,	2),
	FP(2,	4),
};

static struct mt_cpu_freq_method opp_tbl_method_L_e2[] = {	/* SB2 */
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
	FP(2,	2),
};

static struct mt_cpu_freq_method opp_tbl_method_CCI_e2[] = {	/* SB2 */
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

static struct mt_cpu_freq_method opp_tbl_method_LL_e3[] = {	/* FY3 */
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
	FP(2,	2),
	FP(2,	2),
	FP(2,	4),
};

static struct mt_cpu_freq_method opp_tbl_method_L_e3[] = {	/* FY3 */
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
	FP(2,	2),
};

static struct mt_cpu_freq_method opp_tbl_method_CCI_e3[] = {	/* FY3 */
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

static struct mt_cpu_freq_method opp_tbl_method_LL_e4[] = {	/* SB3 */
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
	FP(2,	2),
	FP(2,	2),
	FP(2,	4),
};

static struct mt_cpu_freq_method opp_tbl_method_L_e4[] = {	/* SB3 */
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
	FP(2,	2),
};

static struct mt_cpu_freq_method opp_tbl_method_CCI_e4[] = {	/* SB3 */
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

struct opp_tbl_m_info opp_tbls_m[NR_MT_CPU_DVFS][NUM_CPU_LEVEL] = {	/* v1.1 */
	/* LL */
	{
		[CPU_LEVEL_0] = { opp_tbl_method_LL_e0 },
		[CPU_LEVEL_1] = { opp_tbl_method_LL_e1 },
		[CPU_LEVEL_2] = { opp_tbl_method_LL_e2 },
		[CPU_LEVEL_3] = { opp_tbl_method_LL_e3 },
		[CPU_LEVEL_4] = { opp_tbl_method_LL_e4 },
	},
	/* L */
	{
		[CPU_LEVEL_0] = { opp_tbl_method_L_e0 },
		[CPU_LEVEL_1] = { opp_tbl_method_L_e1 },
		[CPU_LEVEL_2] = { opp_tbl_method_L_e2 },
		[CPU_LEVEL_3] = { opp_tbl_method_L_e3 },
		[CPU_LEVEL_4] = { opp_tbl_method_L_e4 },
	},
	/* CCI */
	{
		[CPU_LEVEL_0] = { opp_tbl_method_CCI_e0 },
		[CPU_LEVEL_1] = { opp_tbl_method_CCI_e1 },
		[CPU_LEVEL_2] = { opp_tbl_method_CCI_e2 },
		[CPU_LEVEL_3] = { opp_tbl_method_CCI_e3 },
		[CPU_LEVEL_4] = { opp_tbl_method_CCI_e4 },
	},
};
