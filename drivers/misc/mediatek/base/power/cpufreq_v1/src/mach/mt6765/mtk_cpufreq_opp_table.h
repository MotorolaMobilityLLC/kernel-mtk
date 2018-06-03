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
#define CPU_DVFS_FREQ1_L_FY    2316000    /* KHz */
#define CPU_DVFS_FREQ2_L_FY    2286000    /* KHz */
#define CPU_DVFS_FREQ3_L_FY    2223000    /* KHz */
#define CPU_DVFS_FREQ4_L_FY    2153000    /* KHz */
#define CPU_DVFS_FREQ5_L_FY    2069000    /* KHz */
#define CPU_DVFS_FREQ6_L_FY    2000000    /* KHz */
#define CPU_DVFS_FREQ7_L_FY    1906000    /* KHz */
#define CPU_DVFS_FREQ8_L_FY    1828000    /* KHz */
#define CPU_DVFS_FREQ9_L_FY    1750000    /* KHz */
#define CPU_DVFS_FREQ10_L_FY   1600000    /* KHz */
#define CPU_DVFS_FREQ11_L_FY   1475000    /* KHz */
#define CPU_DVFS_FREQ12_L_FY   1350000    /* KHz */
#define CPU_DVFS_FREQ13_L_FY   1181000    /* KHz */
#define CPU_DVFS_FREQ14_L_FY   1040000    /* KHz */
#define CPU_DVFS_FREQ15_L_FY    900000    /* KHz */

/* for DVFS OPP table LL */
#define CPU_DVFS_FREQ0_LL_FY    1701000    /* KHz */
#define CPU_DVFS_FREQ1_LL_FY    1673000    /* KHz */
#define CPU_DVFS_FREQ2_LL_FY    1649000    /* KHz */
#define CPU_DVFS_FREQ3_LL_FY    1585000    /* KHz */
#define CPU_DVFS_FREQ4_LL_FY    1512000    /* KHz */
#define CPU_DVFS_FREQ5_LL_FY    1423000    /* KHz */
#define CPU_DVFS_FREQ6_LL_FY    1350000    /* KHz */
#define CPU_DVFS_FREQ7_LL_FY    1237000    /* KHz */
#define CPU_DVFS_FREQ8_LL_FY    1143000    /* KHz */
#define CPU_DVFS_FREQ9_LL_FY    1050000    /* KHz */
#define CPU_DVFS_FREQ10_LL_FY    937000    /* KHz */
#define CPU_DVFS_FREQ11_LL_FY    843000    /* KHz */
#define CPU_DVFS_FREQ12_LL_FY    749000    /* KHz */
#define CPU_DVFS_FREQ13_LL_FY    618000    /* KHz */
#define CPU_DVFS_FREQ14_LL_FY    509000    /* KHz */
#define CPU_DVFS_FREQ15_LL_FY    400000    /* KHz */

/* for DVFS OPP table CCI */
#define CPU_DVFS_FREQ0_CCI_FY    1051000    /* KHz */
#define CPU_DVFS_FREQ1_CCI_FY    1027000    /* KHz */
#define CPU_DVFS_FREQ2_CCI_FY    1007000    /* KHz */
#define CPU_DVFS_FREQ3_CCI_FY     976000    /* KHz */
#define CPU_DVFS_FREQ4_CCI_FY     944000    /* KHz */
#define CPU_DVFS_FREQ5_CCI_FY     906000    /* KHz */
#define CPU_DVFS_FREQ6_CCI_FY     874000    /* KHz */
#define CPU_DVFS_FREQ7_CCI_FY     818000    /* KHz */
#define CPU_DVFS_FREQ8_CCI_FY     771000    /* KHz */
#define CPU_DVFS_FREQ9_CCI_FY     724000    /* KHz */
#define CPU_DVFS_FREQ10_CCI_FY    649000    /* KHz */
#define CPU_DVFS_FREQ11_CCI_FY    587000    /* KHz */
#define CPU_DVFS_FREQ12_CCI_FY    524000    /* KHz */
#define CPU_DVFS_FREQ13_CCI_FY    440000    /* KHz */
#define CPU_DVFS_FREQ14_CCI_FY    370000    /* KHz */
#define CPU_DVFS_FREQ15_CCI_FY    300000    /* KHz */

/* for DVFS OPP table */
#define CPU_DVFS_VOLT0_VPROC_L_FY    111875      /* 10uV */
#define CPU_DVFS_VOLT1_VPROC_L_FY    107500      /* 10uV */
#define CPU_DVFS_VOLT2_VPROC_L_FY    103750      /* 10uV */
#define CPU_DVFS_VOLT3_VPROC_L_FY    100000      /* 10uV */
#define CPU_DVFS_VOLT4_VPROC_L_FY     96875      /* 10uV */
#define CPU_DVFS_VOLT5_VPROC_L_FY     93125      /* 10uV */
#define CPU_DVFS_VOLT6_VPROC_L_FY     90000      /* 10uV */
#define CPU_DVFS_VOLT7_VPROC_L_FY     86250      /* 10uV */
#define CPU_DVFS_VOLT8_VPROC_L_FY     83125      /* 10uV */
#define CPU_DVFS_VOLT9_VPROC_L_FY     80000      /* 10uV */
#define CPU_DVFS_VOLT10_VPROC_L_FY    76250      /* 10uV */
#define CPU_DVFS_VOLT11_VPROC_L_FY    73125      /* 10uV */
#define CPU_DVFS_VOLT12_VPROC_L_FY    70000      /* 10uV */
#define CPU_DVFS_VOLT13_VPROC_L_FY    66250      /* 10uV */
#define CPU_DVFS_VOLT14_VPROC_L_FY    63125      /* 10uV */
#define CPU_DVFS_VOLT15_VPROC_L_FY    60000      /* 10uV */

#define CPU_DVFS_VOLT0_VPROC_LL_FY    111875      /* 10uV */
#define CPU_DVFS_VOLT1_VPROC_LL_FY    107500      /* 10uV */
#define CPU_DVFS_VOLT2_VPROC_LL_FY    103750      /* 10uV */
#define CPU_DVFS_VOLT3_VPROC_LL_FY    100000      /* 10uV */
#define CPU_DVFS_VOLT4_VPROC_LL_FY     96875      /* 10uV */
#define CPU_DVFS_VOLT5_VPROC_LL_FY     93125      /* 10uV */
#define CPU_DVFS_VOLT6_VPROC_LL_FY     90000      /* 10uV */
#define CPU_DVFS_VOLT7_VPROC_LL_FY     86250      /* 10uV */
#define CPU_DVFS_VOLT8_VPROC_LL_FY     83125      /* 10uV */
#define CPU_DVFS_VOLT9_VPROC_LL_FY     80000      /* 10uV */
#define CPU_DVFS_VOLT10_VPROC_LL_FY    76250      /* 10uV */
#define CPU_DVFS_VOLT11_VPROC_LL_FY    73125      /* 10uV */
#define CPU_DVFS_VOLT12_VPROC_LL_FY    70000      /* 10uV */
#define CPU_DVFS_VOLT13_VPROC_LL_FY    66250      /* 10uV */
#define CPU_DVFS_VOLT14_VPROC_LL_FY    63125      /* 10uV */
#define CPU_DVFS_VOLT15_VPROC_LL_FY    60000      /* 10uV */

#define CPU_DVFS_VOLT0_VPROC_CCI_FY    111875      /* 10uV */
#define CPU_DVFS_VOLT1_VPROC_CCI_FY    107500      /* 10uV */
#define CPU_DVFS_VOLT2_VPROC_CCI_FY    103750      /* 10uV */
#define CPU_DVFS_VOLT3_VPROC_CCI_FY    100000      /* 10uV */
#define CPU_DVFS_VOLT4_VPROC_CCI_FY     96875      /* 10uV */
#define CPU_DVFS_VOLT5_VPROC_CCI_FY     93125      /* 10uV */
#define CPU_DVFS_VOLT6_VPROC_CCI_FY     90000      /* 10uV */
#define CPU_DVFS_VOLT7_VPROC_CCI_FY     86250      /* 10uV */
#define CPU_DVFS_VOLT8_VPROC_CCI_FY     83125      /* 10uV */
#define CPU_DVFS_VOLT9_VPROC_CCI_FY     80000      /* 10uV */
#define CPU_DVFS_VOLT10_VPROC_CCI_FY    76250      /* 10uV */
#define CPU_DVFS_VOLT11_VPROC_CCI_FY    73125      /* 10uV */
#define CPU_DVFS_VOLT12_VPROC_CCI_FY    70000      /* 10uV */
#define CPU_DVFS_VOLT13_VPROC_CCI_FY    66250      /* 10uV */
#define CPU_DVFS_VOLT14_VPROC_CCI_FY    63125      /* 10uV */
#define CPU_DVFS_VOLT15_VPROC_CCI_FY    60000      /* 10uV */

/* SB */
/* for DVFS OPP table L */
#define CPU_DVFS_FREQ0_L_SB    2501000    /* KHz */
#define CPU_DVFS_FREQ1_L_SB    2397000    /* KHz */
#define CPU_DVFS_FREQ2_L_SB    2309000    /* KHz */
#define CPU_DVFS_FREQ3_L_SB    2223000    /* KHz */
#define CPU_DVFS_FREQ4_L_SB    2153000    /* KHz */
#define CPU_DVFS_FREQ5_L_SB    2069000    /* KHz */
#define CPU_DVFS_FREQ6_L_SB    2000000    /* KHz */
#define CPU_DVFS_FREQ7_L_SB    1906000    /* KHz */
#define CPU_DVFS_FREQ8_L_SB    1828000    /* KHz */
#define CPU_DVFS_FREQ9_L_SB    1750000    /* KHz */
#define CPU_DVFS_FREQ10_L_SB   1600000    /* KHz */
#define CPU_DVFS_FREQ11_L_SB   1475000    /* KHz */
#define CPU_DVFS_FREQ12_L_SB   1350000    /* KHz */
#define CPU_DVFS_FREQ13_L_SB   1181000    /* KHz */
#define CPU_DVFS_FREQ14_L_SB   1040000    /* KHz */
#define CPU_DVFS_FREQ15_L_SB    900000    /* KHz */

/* for DVFS OPP table LL */
#define CPU_DVFS_FREQ0_LL_SB    1901000    /* KHz */
#define CPU_DVFS_FREQ1_LL_SB    1782000    /* KHz */
#define CPU_DVFS_FREQ2_LL_SB    1679000    /* KHz */
#define CPU_DVFS_FREQ3_LL_SB    1585000    /* KHz */
#define CPU_DVFS_FREQ4_LL_SB    1512000    /* KHz */
#define CPU_DVFS_FREQ5_LL_SB    1423000    /* KHz */
#define CPU_DVFS_FREQ6_LL_SB    1350000    /* KHz */
#define CPU_DVFS_FREQ7_LL_SB    1237000    /* KHz */
#define CPU_DVFS_FREQ8_LL_SB    1143000    /* KHz */
#define CPU_DVFS_FREQ9_LL_SB    1050000    /* KHz */
#define CPU_DVFS_FREQ10_LL_SB    937000    /* KHz */
#define CPU_DVFS_FREQ11_LL_SB    843000    /* KHz */
#define CPU_DVFS_FREQ12_LL_SB    749000    /* KHz */
#define CPU_DVFS_FREQ13_LL_SB    618000    /* KHz */
#define CPU_DVFS_FREQ14_LL_SB    509000    /* KHz */
#define CPU_DVFS_FREQ15_LL_SB    400000    /* KHz */

/* for DVFS OPP table CCI */
#define CPU_DVFS_FREQ0_CCI_SB    1101000    /* KHz */
#define CPU_DVFS_FREQ1_CCI_SB    1055000    /* KHz */
#define CPU_DVFS_FREQ2_CCI_SB    1015000    /* KHz */
#define CPU_DVFS_FREQ3_CCI_SB     976000    /* KHz */
#define CPU_DVFS_FREQ4_CCI_SB     944000    /* KHz */
#define CPU_DVFS_FREQ5_CCI_SB     906000    /* KHz */
#define CPU_DVFS_FREQ6_CCI_SB     874000    /* KHz */
#define CPU_DVFS_FREQ7_CCI_SB     818000    /* KHz */
#define CPU_DVFS_FREQ8_CCI_SB     771000    /* KHz */
#define CPU_DVFS_FREQ9_CCI_SB     724000    /* KHz */
#define CPU_DVFS_FREQ10_CCI_SB    649000    /* KHz */
#define CPU_DVFS_FREQ11_CCI_SB    587000    /* KHz */
#define CPU_DVFS_FREQ12_CCI_SB    524000    /* KHz */
#define CPU_DVFS_FREQ13_CCI_SB    440000    /* KHz */
#define CPU_DVFS_FREQ14_CCI_SB    370000    /* KHz */
#define CPU_DVFS_FREQ15_CCI_SB    300000    /* KHz */

/* for DVFS OPP table */
#define CPU_DVFS_VOLT0_VPROC_L_SB    111875       /* 10uV */
#define CPU_DVFS_VOLT1_VPROC_L_SB    107500       /* 10uV */
#define CPU_DVFS_VOLT2_VPROC_L_SB    103750       /* 10uV */
#define CPU_DVFS_VOLT3_VPROC_L_SB    100000       /* 10uV */
#define CPU_DVFS_VOLT4_VPROC_L_SB     96875       /* 10uV */
#define CPU_DVFS_VOLT5_VPROC_L_SB     93125       /* 10uV */
#define CPU_DVFS_VOLT6_VPROC_L_SB     90000       /* 10uV */
#define CPU_DVFS_VOLT7_VPROC_L_SB     86250       /* 10uV */
#define CPU_DVFS_VOLT8_VPROC_L_SB     83125       /* 10uV */
#define CPU_DVFS_VOLT9_VPROC_L_SB     80000       /* 10uV */
#define CPU_DVFS_VOLT10_VPROC_L_SB    76250       /* 10uV */
#define CPU_DVFS_VOLT11_VPROC_L_SB    73125       /* 10uV */
#define CPU_DVFS_VOLT12_VPROC_L_SB    70000       /* 10uV */
#define CPU_DVFS_VOLT13_VPROC_L_SB    66250       /* 10uV */
#define CPU_DVFS_VOLT14_VPROC_L_SB    63125       /* 10uV */
#define CPU_DVFS_VOLT15_VPROC_L_SB    60000       /* 10uV */

#define CPU_DVFS_VOLT0_VPROC_LL_SB    111875       /* 10uV */
#define CPU_DVFS_VOLT1_VPROC_LL_SB    107500       /* 10uV */
#define CPU_DVFS_VOLT2_VPROC_LL_SB    103750       /* 10uV */
#define CPU_DVFS_VOLT3_VPROC_LL_SB    100000       /* 10uV */
#define CPU_DVFS_VOLT4_VPROC_LL_SB     96875       /* 10uV */
#define CPU_DVFS_VOLT5_VPROC_LL_SB     93125       /* 10uV */
#define CPU_DVFS_VOLT6_VPROC_LL_SB     90000       /* 10uV */
#define CPU_DVFS_VOLT7_VPROC_LL_SB     86250       /* 10uV */
#define CPU_DVFS_VOLT8_VPROC_LL_SB     83125       /* 10uV */
#define CPU_DVFS_VOLT9_VPROC_LL_SB     80000       /* 10uV */
#define CPU_DVFS_VOLT10_VPROC_LL_SB    76250       /* 10uV */
#define CPU_DVFS_VOLT11_VPROC_LL_SB    73125       /* 10uV */
#define CPU_DVFS_VOLT12_VPROC_LL_SB    70000       /* 10uV */
#define CPU_DVFS_VOLT13_VPROC_LL_SB    66250       /* 10uV */
#define CPU_DVFS_VOLT14_VPROC_LL_SB    63125       /* 10uV */
#define CPU_DVFS_VOLT15_VPROC_LL_SB    60000       /* 10uV */

#define CPU_DVFS_VOLT0_VPROC_CCI_SB    111875       /* 10uV */
#define CPU_DVFS_VOLT1_VPROC_CCI_SB    107500       /* 10uV */
#define CPU_DVFS_VOLT2_VPROC_CCI_SB    103750       /* 10uV */
#define CPU_DVFS_VOLT3_VPROC_CCI_SB    100000       /* 10uV */
#define CPU_DVFS_VOLT4_VPROC_CCI_SB     96875       /* 10uV */
#define CPU_DVFS_VOLT5_VPROC_CCI_SB     93125       /* 10uV */
#define CPU_DVFS_VOLT6_VPROC_CCI_SB     90000       /* 10uV */
#define CPU_DVFS_VOLT7_VPROC_CCI_SB     86250       /* 10uV */
#define CPU_DVFS_VOLT8_VPROC_CCI_SB     83125       /* 10uV */
#define CPU_DVFS_VOLT9_VPROC_CCI_SB     80000       /* 10uV */
#define CPU_DVFS_VOLT10_VPROC_CCI_SB    76250       /* 10uV */
#define CPU_DVFS_VOLT11_VPROC_CCI_SB    73125       /* 10uV */
#define CPU_DVFS_VOLT12_VPROC_CCI_SB    70000       /* 10uV */
#define CPU_DVFS_VOLT13_VPROC_CCI_SB    66250       /* 10uV */
#define CPU_DVFS_VOLT14_VPROC_CCI_SB    63125       /* 10uV */
#define CPU_DVFS_VOLT15_VPROC_CCI_SB    60000       /* 10uV */

/* C65T */
/* for DVFS OPP table L */
#define CPU_DVFS_FREQ0_L_C65T    2501000    /* KHz */
#define CPU_DVFS_FREQ1_L_C65T    2412000    /* KHz */
#define CPU_DVFS_FREQ2_L_C65T    2323000    /* KHz */
#define CPU_DVFS_FREQ3_L_C65T    2237000    /* KHz */
#define CPU_DVFS_FREQ4_L_C65T    2153000    /* KHz */
#define CPU_DVFS_FREQ5_L_C65T    2069000    /* KHz */
#define CPU_DVFS_FREQ6_L_C65T    1984000    /* KHz */
#define CPU_DVFS_FREQ7_L_C65T    1890000    /* KHz */
#define CPU_DVFS_FREQ8_L_C65T    1796000    /* KHz */
#define CPU_DVFS_FREQ9_L_C65T    1700000    /* KHz */
#define CPU_DVFS_FREQ10_L_C65T   1575000    /* KHz */
#define CPU_DVFS_FREQ11_L_C65T   1450000    /* KHz */
#define CPU_DVFS_FREQ12_L_C65T   1321000    /* KHz */
#define CPU_DVFS_FREQ13_L_C65T   1181000    /* KHz */
#define CPU_DVFS_FREQ14_L_C65T   1040000    /* KHz */
#define CPU_DVFS_FREQ15_L_C65T    900000    /* KHz */

/* for DVFS OPP table LL */
#define CPU_DVFS_FREQ0_LL_C65T    1800000    /* KHz */
#define CPU_DVFS_FREQ1_LL_C65T    1707000    /* KHz */
#define CPU_DVFS_FREQ2_LL_C65T    1615000    /* KHz */
#define CPU_DVFS_FREQ3_LL_C65T    1526000    /* KHz */
#define CPU_DVFS_FREQ4_LL_C65T    1453000    /* KHz */
#define CPU_DVFS_FREQ5_LL_C65T    1379000    /* KHz */
#define CPU_DVFS_FREQ6_LL_C65T    1293000    /* KHz */
#define CPU_DVFS_FREQ7_LL_C65T    1200000    /* KHz */
#define CPU_DVFS_FREQ8_LL_C65T    1106000    /* KHz */
#define CPU_DVFS_FREQ9_LL_C65T    1012000    /* KHz */
#define CPU_DVFS_FREQ10_LL_C65T    918000    /* KHz */
#define CPU_DVFS_FREQ11_LL_C65T    824000    /* KHz */
#define CPU_DVFS_FREQ12_LL_C65T    728000    /* KHz */
#define CPU_DVFS_FREQ13_LL_C65T    618000    /* KHz */
#define CPU_DVFS_FREQ14_LL_C65T    509000    /* KHz */
#define CPU_DVFS_FREQ15_LL_C65T    400000    /* KHz */

/* for DVFS OPP table CCI */
#define CPU_DVFS_FREQ0_CCI_C65T    1101000    /* KHz */
#define CPU_DVFS_FREQ1_CCI_C65T    1061000    /* KHz */
#define CPU_DVFS_FREQ2_CCI_C65T    1022000    /* KHz */
#define CPU_DVFS_FREQ3_CCI_C65T     982000    /* KHz */
#define CPU_DVFS_FREQ4_CCI_C65T     944000    /* KHz */
#define CPU_DVFS_FREQ5_CCI_C65T     906000    /* KHz */
#define CPU_DVFS_FREQ6_CCI_C65T     865000    /* KHz */
#define CPU_DVFS_FREQ7_CCI_C65T     809000    /* KHz */
#define CPU_DVFS_FREQ8_CCI_C65T     753000    /* KHz */
#define CPU_DVFS_FREQ9_CCI_C65T     699000    /* KHz */
#define CPU_DVFS_FREQ10_CCI_C65T    637000    /* KHz */
#define CPU_DVFS_FREQ11_CCI_C65T    574000    /* KHz */
#define CPU_DVFS_FREQ12_CCI_C65T    510000    /* KHz */
#define CPU_DVFS_FREQ13_CCI_C65T    440000    /* KHz */
#define CPU_DVFS_FREQ14_CCI_C65T    370000    /* KHz */
#define CPU_DVFS_FREQ15_CCI_C65T    300000    /* KHz */

/* for DVFS OPP table */
#define CPU_DVFS_VOLT0_VPROC_L_C65T    111875        /* 10uV */
#define CPU_DVFS_VOLT1_VPROC_L_C65T    108125        /* 10uV */
#define CPU_DVFS_VOLT2_VPROC_L_C65T    104375        /* 10uV */
#define CPU_DVFS_VOLT3_VPROC_L_C65T    100625        /* 10uV */
#define CPU_DVFS_VOLT4_VPROC_L_C65T     96875        /* 10uV */
#define CPU_DVFS_VOLT5_VPROC_L_C65T     93125        /* 10uV */
#define CPU_DVFS_VOLT6_VPROC_L_C65T     89375        /* 10uV */
#define CPU_DVFS_VOLT7_VPROC_L_C65T     85625        /* 10uV */
#define CPU_DVFS_VOLT8_VPROC_L_C65T     81875        /* 10uV */
#define CPU_DVFS_VOLT9_VPROC_L_C65T     78750        /* 10uV */
#define CPU_DVFS_VOLT10_VPROC_L_C65T    75625        /* 10uV */
#define CPU_DVFS_VOLT11_VPROC_L_C65T    72500        /* 10uV */
#define CPU_DVFS_VOLT12_VPROC_L_C65T    69375        /* 10uV */
#define CPU_DVFS_VOLT13_VPROC_L_C65T    66250        /* 10uV */
#define CPU_DVFS_VOLT14_VPROC_L_C65T    63125        /* 10uV */
#define CPU_DVFS_VOLT15_VPROC_L_C65T    60000        /* 10uV */

#define CPU_DVFS_VOLT0_VPROC_LL_C65T    108750        /* 10uV */
#define CPU_DVFS_VOLT1_VPROC_LL_C65T    105000        /* 10uV */
#define CPU_DVFS_VOLT2_VPROC_LL_C65T    101250        /* 10uV */
#define CPU_DVFS_VOLT3_VPROC_LL_C65T     97500        /* 10uV */
#define CPU_DVFS_VOLT4_VPROC_LL_C65T     94375        /* 10uV */
#define CPU_DVFS_VOLT5_VPROC_LL_C65T     91250        /* 10uV */
#define CPU_DVFS_VOLT6_VPROC_LL_C65T     88125        /* 10uV */
#define CPU_DVFS_VOLT7_VPROC_LL_C65T     85000        /* 10uV */
#define CPU_DVFS_VOLT8_VPROC_LL_C65T     81875        /* 10uV */
#define CPU_DVFS_VOLT9_VPROC_LL_C65T     78750        /* 10uV */
#define CPU_DVFS_VOLT10_VPROC_LL_C65T    75625        /* 10uV */
#define CPU_DVFS_VOLT11_VPROC_LL_C65T    72500        /* 10uV */
#define CPU_DVFS_VOLT12_VPROC_LL_C65T    69375        /* 10uV */
#define CPU_DVFS_VOLT13_VPROC_LL_C65T    66250        /* 10uV */
#define CPU_DVFS_VOLT14_VPROC_LL_C65T    63125        /* 10uV */
#define CPU_DVFS_VOLT15_VPROC_LL_C65T    60000        /* 10uV */

#define CPU_DVFS_VOLT0_VPROC_CCI_C65T    111875        /* 10uV */
#define CPU_DVFS_VOLT1_VPROC_CCI_C65T    108125        /* 10uV */
#define CPU_DVFS_VOLT2_VPROC_CCI_C65T    104375        /* 10uV */
#define CPU_DVFS_VOLT3_VPROC_CCI_C65T    100625        /* 10uV */
#define CPU_DVFS_VOLT4_VPROC_CCI_C65T     96875        /* 10uV */
#define CPU_DVFS_VOLT5_VPROC_CCI_C65T     93125        /* 10uV */
#define CPU_DVFS_VOLT6_VPROC_CCI_C65T     89375        /* 10uV */
#define CPU_DVFS_VOLT7_VPROC_CCI_C65T     85625        /* 10uV */
#define CPU_DVFS_VOLT8_VPROC_CCI_C65T     81875        /* 10uV */
#define CPU_DVFS_VOLT9_VPROC_CCI_C65T     78750        /* 10uV */
#define CPU_DVFS_VOLT10_VPROC_CCI_C65T    75625        /* 10uV */
#define CPU_DVFS_VOLT11_VPROC_CCI_C65T    72500        /* 10uV */
#define CPU_DVFS_VOLT12_VPROC_CCI_C65T    69375        /* 10uV */
#define CPU_DVFS_VOLT13_VPROC_CCI_C65T    66250        /* 10uV */
#define CPU_DVFS_VOLT14_VPROC_CCI_C65T    63125        /* 10uV */
#define CPU_DVFS_VOLT15_VPROC_CCI_C65T    60000        /* 10uV */

/* C65 */
/* for DVFS OPP table L */
#define CPU_DVFS_FREQ0_L_C65    2301000    /* KHz */
#define CPU_DVFS_FREQ1_L_C65    2276000    /* KHz */
#define CPU_DVFS_FREQ2_L_C65    2209000    /* KHz */
#define CPU_DVFS_FREQ3_L_C65    2139000    /* KHz */
#define CPU_DVFS_FREQ4_L_C65    2069000    /* KHz */
#define CPU_DVFS_FREQ5_L_C65    2000000    /* KHz */
#define CPU_DVFS_FREQ6_L_C65    1921000    /* KHz */
#define CPU_DVFS_FREQ7_L_C65    1843000    /* KHz */
#define CPU_DVFS_FREQ8_L_C65    1765000    /* KHz */
#define CPU_DVFS_FREQ9_L_C65    1650000    /* KHz */
#define CPU_DVFS_FREQ10_L_C65   1525000    /* KHz */
#define CPU_DVFS_FREQ11_L_C65   1400000    /* KHz */
#define CPU_DVFS_FREQ12_L_C65   1265000    /* KHz */
#define CPU_DVFS_FREQ13_L_C65   1125000    /* KHz */
#define CPU_DVFS_FREQ14_L_C65   1012000    /* KHz */
#define CPU_DVFS_FREQ15_L_C65    900000    /* KHz */

/* for DVFS OPP table LL */
#define CPU_DVFS_FREQ0_LL_C65    1800000    /* KHz */
#define CPU_DVFS_FREQ1_LL_C65    1707000    /* KHz */
#define CPU_DVFS_FREQ2_LL_C65    1615000    /* KHz */
#define CPU_DVFS_FREQ3_LL_C65    1526000    /* KHz */
#define CPU_DVFS_FREQ4_LL_C65    1453000    /* KHz */
#define CPU_DVFS_FREQ5_LL_C65    1379000    /* KHz */
#define CPU_DVFS_FREQ6_LL_C65    1293000    /* KHz */
#define CPU_DVFS_FREQ7_LL_C65    1200000    /* KHz */
#define CPU_DVFS_FREQ8_LL_C65    1106000    /* KHz */
#define CPU_DVFS_FREQ9_LL_C65    1012000    /* KHz */
#define CPU_DVFS_FREQ10_LL_C65    918000    /* KHz */
#define CPU_DVFS_FREQ11_LL_C65    824000    /* KHz */
#define CPU_DVFS_FREQ12_LL_C65    728000    /* KHz */
#define CPU_DVFS_FREQ13_LL_C65    618000    /* KHz */
#define CPU_DVFS_FREQ14_LL_C65    509000    /* KHz */
#define CPU_DVFS_FREQ15_LL_C65    400000    /* KHz */

/* for DVFS OPP table CCI */
#define CPU_DVFS_FREQ0_CCI_C65    1027000    /* KHz */
#define CPU_DVFS_FREQ1_CCI_C65    1001000    /* KHz */
#define CPU_DVFS_FREQ2_CCI_C65     988000    /* KHz */
#define CPU_DVFS_FREQ3_CCI_C65     949000    /* KHz */
#define CPU_DVFS_FREQ4_CCI_C65     910000    /* KHz */
#define CPU_DVFS_FREQ5_CCI_C65     884000    /* KHz */
#define CPU_DVFS_FREQ6_CCI_C65     845000    /* KHz */
#define CPU_DVFS_FREQ7_CCI_C65     793000    /* KHz */
#define CPU_DVFS_FREQ8_CCI_C65     741000    /* KHz */
#define CPU_DVFS_FREQ9_CCI_C65     689000    /* KHz */
#define CPU_DVFS_FREQ10_CCI_C65    637000    /* KHz */
#define CPU_DVFS_FREQ11_CCI_C65    572000    /* KHz */
#define CPU_DVFS_FREQ12_CCI_C65    507000    /* KHz */
#define CPU_DVFS_FREQ13_CCI_C65    429000    /* KHz */
#define CPU_DVFS_FREQ14_CCI_C65    364000    /* KHz */
#define CPU_DVFS_FREQ15_CCI_C65    299000    /* KHz */

/* for DVFS OPP table */
#define CPU_DVFS_VOLT0_VPROC_L_C65    105625        /* 10uV */
#define CPU_DVFS_VOLT1_VPROC_L_C65    102500        /* 10uV */
#define CPU_DVFS_VOLT2_VPROC_L_C65     99375        /* 10uV */
#define CPU_DVFS_VOLT3_VPROC_L_C65     96250        /* 10uV */
#define CPU_DVFS_VOLT4_VPROC_L_C65     93125        /* 10uV */
#define CPU_DVFS_VOLT5_VPROC_L_C65     90000        /* 10uV */
#define CPU_DVFS_VOLT6_VPROC_L_C65     86875        /* 10uV */
#define CPU_DVFS_VOLT7_VPROC_L_C65     83750        /* 10uV */
#define CPU_DVFS_VOLT8_VPROC_L_C65     80625        /* 10uV */
#define CPU_DVFS_VOLT9_VPROC_L_C65     77500        /* 10uV */
#define CPU_DVFS_VOLT10_VPROC_L_C65    74375        /* 10uV */
#define CPU_DVFS_VOLT11_VPROC_L_C65    71250        /* 10uV */
#define CPU_DVFS_VOLT12_VPROC_L_C65    68125        /* 10uV */
#define CPU_DVFS_VOLT13_VPROC_L_C65    65000        /* 10uV */
#define CPU_DVFS_VOLT14_VPROC_L_C65    62500        /* 10uV */
#define CPU_DVFS_VOLT15_VPROC_L_C65    60000        /* 10uV */

#define CPU_DVFS_VOLT0_VPROC_LL_C65    108750        /* 10uV */
#define CPU_DVFS_VOLT1_VPROC_LL_C65    105000        /* 10uV */
#define CPU_DVFS_VOLT2_VPROC_LL_C65    101250        /* 10uV */
#define CPU_DVFS_VOLT3_VPROC_LL_C65     97500        /* 10uV */
#define CPU_DVFS_VOLT4_VPROC_LL_C65     94375        /* 10uV */
#define CPU_DVFS_VOLT5_VPROC_LL_C65     91250        /* 10uV */
#define CPU_DVFS_VOLT6_VPROC_LL_C65     88125        /* 10uV */
#define CPU_DVFS_VOLT7_VPROC_LL_C65     85000        /* 10uV */
#define CPU_DVFS_VOLT8_VPROC_LL_C65     81875        /* 10uV */
#define CPU_DVFS_VOLT9_VPROC_LL_C65     78750        /* 10uV */
#define CPU_DVFS_VOLT10_VPROC_LL_C65    75625        /* 10uV */
#define CPU_DVFS_VOLT11_VPROC_LL_C65    72500        /* 10uV */
#define CPU_DVFS_VOLT12_VPROC_LL_C65    69375        /* 10uV */
#define CPU_DVFS_VOLT13_VPROC_LL_C65    66250        /* 10uV */
#define CPU_DVFS_VOLT14_VPROC_LL_C65    63125        /* 10uV */
#define CPU_DVFS_VOLT15_VPROC_LL_C65    60000        /* 10uV */

#define CPU_DVFS_VOLT0_VPROC_CCI_C65    108750        /* 10uV */
#define CPU_DVFS_VOLT1_VPROC_CCI_C65    105000        /* 10uV */
#define CPU_DVFS_VOLT2_VPROC_CCI_C65    101250        /* 10uV */
#define CPU_DVFS_VOLT3_VPROC_CCI_C65     97500        /* 10uV */
#define CPU_DVFS_VOLT4_VPROC_CCI_C65     94375        /* 10uV */
#define CPU_DVFS_VOLT5_VPROC_CCI_C65     91250        /* 10uV */
#define CPU_DVFS_VOLT6_VPROC_CCI_C65     88125        /* 10uV */
#define CPU_DVFS_VOLT7_VPROC_CCI_C65     85000        /* 10uV */
#define CPU_DVFS_VOLT8_VPROC_CCI_C65     81875        /* 10uV */
#define CPU_DVFS_VOLT9_VPROC_CCI_C65     78750        /* 10uV */
#define CPU_DVFS_VOLT10_VPROC_CCI_C65    75625        /* 10uV */
#define CPU_DVFS_VOLT11_VPROC_CCI_C65    72500        /* 10uV */
#define CPU_DVFS_VOLT12_VPROC_CCI_C65    69375        /* 10uV */
#define CPU_DVFS_VOLT13_VPROC_CCI_C65    66250        /* 10uV */
#define CPU_DVFS_VOLT14_VPROC_CCI_C65    63125        /* 10uV */
#define CPU_DVFS_VOLT15_VPROC_CCI_C65    60000        /* 10uV */

/* C62 */
/* for DVFS OPP table L */
#define CPU_DVFS_FREQ0_L_C62    2001000    /* KHz */
#define CPU_DVFS_FREQ1_L_C62    1938000    /* KHz */
#define CPU_DVFS_FREQ2_L_C62    1875000    /* KHz */
#define CPU_DVFS_FREQ3_L_C62    1812000    /* KHz */
#define CPU_DVFS_FREQ4_L_C62    1765000    /* KHz */
#define CPU_DVFS_FREQ5_L_C62    1700000    /* KHz */
#define CPU_DVFS_FREQ6_L_C62    1625000    /* KHz */
#define CPU_DVFS_FREQ7_L_C62    1550000    /* KHz */
#define CPU_DVFS_FREQ8_L_C62    1475000    /* KHz */
#define CPU_DVFS_FREQ9_L_C62    1400000    /* KHz */
#define CPU_DVFS_FREQ10_L_C62   1321000    /* KHz */
#define CPU_DVFS_FREQ11_L_C62   1237000    /* KHz */
#define CPU_DVFS_FREQ12_L_C62   1153000    /* KHz */
#define CPU_DVFS_FREQ13_L_C62   1068000    /* KHz */
#define CPU_DVFS_FREQ14_L_C62    984000    /* KHz */
#define CPU_DVFS_FREQ15_L_C62    900000    /* KHz */

/* for DVFS OPP table LL */
#define CPU_DVFS_FREQ0_LL_C62    1500000    /* KHz */
#define CPU_DVFS_FREQ1_LL_C62    1446000    /* KHz */
#define CPU_DVFS_FREQ2_LL_C62    1391000    /* KHz */
#define CPU_DVFS_FREQ3_LL_C62    1331000    /* KHz */
#define CPU_DVFS_FREQ4_LL_C62    1256000    /* KHz */
#define CPU_DVFS_FREQ5_LL_C62    1181000    /* KHz */
#define CPU_DVFS_FREQ6_LL_C62    1106000    /* KHz */
#define CPU_DVFS_FREQ7_LL_C62    1031000    /* KHz */
#define CPU_DVFS_FREQ8_LL_C62     956000    /* KHz */
#define CPU_DVFS_FREQ9_LL_C62     881000    /* KHz */
#define CPU_DVFS_FREQ10_LL_C62    806000    /* KHz */
#define CPU_DVFS_FREQ11_LL_C62    728000    /* KHz */
#define CPU_DVFS_FREQ12_LL_C62    640000    /* KHz */
#define CPU_DVFS_FREQ13_LL_C62    553000    /* KHz */
#define CPU_DVFS_FREQ14_LL_C62    465000    /* KHz */
#define CPU_DVFS_FREQ15_LL_C62    400000    /* KHz */

/* for DVFS OPP table CCI */
#define CPU_DVFS_FREQ0_CCI_C62    944000    /* KHz */
#define CPU_DVFS_FREQ1_CCI_C62    919000    /* KHz */
#define CPU_DVFS_FREQ2_CCI_C62    894000    /* KHz */
#define CPU_DVFS_FREQ3_CCI_C62    865000    /* KHz */
#define CPU_DVFS_FREQ4_CCI_C62    828000    /* KHz */
#define CPU_DVFS_FREQ5_CCI_C62    790000    /* KHz */
#define CPU_DVFS_FREQ6_CCI_C62    753000    /* KHz */
#define CPU_DVFS_FREQ7_CCI_C62    712000    /* KHz */
#define CPU_DVFS_FREQ8_CCI_C62    662000    /* KHz */
#define CPU_DVFS_FREQ9_CCI_C62    612000    /* KHz */
#define CPU_DVFS_FREQ10_CCI_C62   562000    /* KHz */
#define CPU_DVFS_FREQ11_CCI_C62   510000    /* KHz */
#define CPU_DVFS_FREQ12_CCI_C62   454000    /* KHz */
#define CPU_DVFS_FREQ13_CCI_C62   398000    /* KHz */
#define CPU_DVFS_FREQ14_CCI_C62   342000    /* KHz */
#define CPU_DVFS_FREQ15_CCI_C62   300000    /* KHz */

/* for DVFS OPP table */
#define CPU_DVFS_VOLT0_VPROC_L_C62    90000        /* 10uV */
#define CPU_DVFS_VOLT1_VPROC_L_C62    87500        /* 10uV */
#define CPU_DVFS_VOLT2_VPROC_L_C62    85000        /* 10uV */
#define CPU_DVFS_VOLT3_VPROC_L_C62    82500        /* 10uV */
#define CPU_DVFS_VOLT4_VPROC_L_C62    80625        /* 10uV */
#define CPU_DVFS_VOLT5_VPROC_L_C62    78750        /* 10uV */
#define CPU_DVFS_VOLT6_VPROC_L_C62    76875        /* 10uV */
#define CPU_DVFS_VOLT7_VPROC_L_C62    75000        /* 10uV */
#define CPU_DVFS_VOLT8_VPROC_L_C62    73125        /* 10uV */
#define CPU_DVFS_VOLT9_VPROC_L_C62    71250        /* 10uV */
#define CPU_DVFS_VOLT10_VPROC_L_C62   69375        /* 10uV */
#define CPU_DVFS_VOLT11_VPROC_L_C62   67500        /* 10uV */
#define CPU_DVFS_VOLT12_VPROC_L_C62   65625        /* 10uV */
#define CPU_DVFS_VOLT13_VPROC_L_C62   63750        /* 10uV */
#define CPU_DVFS_VOLT14_VPROC_L_C62   61875        /* 10uV */
#define CPU_DVFS_VOLT15_VPROC_L_C62   60000        /* 10uV */

#define CPU_DVFS_VOLT0_VPROC_LL_C62    96875        /* 10uV */
#define CPU_DVFS_VOLT1_VPROC_LL_C62    94375        /* 10uV */
#define CPU_DVFS_VOLT2_VPROC_LL_C62    91875        /* 10uV */
#define CPU_DVFS_VOLT3_VPROC_LL_C62    89375        /* 10uV */
#define CPU_DVFS_VOLT4_VPROC_LL_C62    86875        /* 10uV */
#define CPU_DVFS_VOLT5_VPROC_LL_C62    84375        /* 10uV */
#define CPU_DVFS_VOLT6_VPROC_LL_C62    81875        /* 10uV */
#define CPU_DVFS_VOLT7_VPROC_LL_C62    79375        /* 10uV */
#define CPU_DVFS_VOLT8_VPROC_LL_C62    76875        /* 10uV */
#define CPU_DVFS_VOLT9_VPROC_LL_C62    74375        /* 10uV */
#define CPU_DVFS_VOLT10_VPROC_LL_C62   71875        /* 10uV */
#define CPU_DVFS_VOLT11_VPROC_LL_C62   69375        /* 10uV */
#define CPU_DVFS_VOLT12_VPROC_LL_C62   66875        /* 10uV */
#define CPU_DVFS_VOLT13_VPROC_LL_C62   64375        /* 10uV */
#define CPU_DVFS_VOLT14_VPROC_LL_C62   61875        /* 10uV */
#define CPU_DVFS_VOLT15_VPROC_LL_C62   60000        /* 10uV */

#define CPU_DVFS_VOLT0_VPROC_CCI_C62    96875        /* 10uV */
#define CPU_DVFS_VOLT1_VPROC_CCI_C62    94375        /* 10uV */
#define CPU_DVFS_VOLT2_VPROC_CCI_C62    91875        /* 10uV */
#define CPU_DVFS_VOLT3_VPROC_CCI_C62    89375        /* 10uV */
#define CPU_DVFS_VOLT4_VPROC_CCI_C62    86875        /* 10uV */
#define CPU_DVFS_VOLT5_VPROC_CCI_C62    84375        /* 10uV */
#define CPU_DVFS_VOLT6_VPROC_CCI_C62    81875        /* 10uV */
#define CPU_DVFS_VOLT7_VPROC_CCI_C62    79375        /* 10uV */
#define CPU_DVFS_VOLT8_VPROC_CCI_C62    76875        /* 10uV */
#define CPU_DVFS_VOLT9_VPROC_CCI_C62    74375        /* 10uV */
#define CPU_DVFS_VOLT10_VPROC_CCI_C62   71875        /* 10uV */
#define CPU_DVFS_VOLT11_VPROC_CCI_C62   69375        /* 10uV */
#define CPU_DVFS_VOLT12_VPROC_CCI_C62   66875        /* 10uV */
#define CPU_DVFS_VOLT13_VPROC_CCI_C62   64375        /* 10uV */
#define CPU_DVFS_VOLT14_VPROC_CCI_C62   61875        /* 10uV */
#define CPU_DVFS_VOLT15_VPROC_CCI_C62   60000        /* 10uV */

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

OPP_TBL(L,   FY, 0, L); /* opp_tbl_L_e0_0   */
OPP_TBL(LL,  FY, 0, LL); /* opp_tbl_LL_e0_0  */
OPP_TBL(CCI, FY, 0, CCI); /* opp_tbl_CCI_e0_0 */

OPP_TBL(L,   SB, 1, L); /* opp_tbl_L_e1_0   */
OPP_TBL(LL,  SB, 1, LL); /* opp_tbl_LL_e1_0  */
OPP_TBL(CCI, SB, 1, CCI); /* opp_tbl_CCI_e1_0 */

OPP_TBL(L,   C65T, 2, L); /* opp_tbl_L_e2_0   */
OPP_TBL(LL,  C65T, 2, LL); /* opp_tbl_LL_e2_0  */
OPP_TBL(CCI, C65T, 2, CCI); /* opp_tbl_CCI_e2_0 */

OPP_TBL(L,   C65, 3, L); /* opp_tbl_L_e3_0   */
OPP_TBL(LL,  C65, 3, LL); /* opp_tbl_LL_e3_0  */
OPP_TBL(CCI, C65, 3, CCI); /* opp_tbl_CCI_e3_0 */

OPP_TBL(L,   C62, 4, L); /* opp_tbl_L_e4_0   */
OPP_TBL(LL,  C62, 4, LL); /* opp_tbl_LL_e4_0  */
OPP_TBL(CCI, C62, 4, CCI); /* opp_tbl_CCI_e4_0 */

/* v0.7 */
struct opp_tbl_info opp_tbls[NR_MT_CPU_DVFS][NUM_CPU_LEVEL] = {
	/* L */
	{
		[CPU_LEVEL_0] = { opp_tbl_L_e0_0,
				ARRAY_SIZE(opp_tbl_L_e0_0) },
		[CPU_LEVEL_1] = { opp_tbl_L_e1_0,
				ARRAY_SIZE(opp_tbl_L_e1_0) },
		[CPU_LEVEL_2] = { opp_tbl_L_e2_0,
				ARRAY_SIZE(opp_tbl_L_e2_0) },
		[CPU_LEVEL_3] = { opp_tbl_L_e3_0,
				ARRAY_SIZE(opp_tbl_L_e3_0) },
		[CPU_LEVEL_4] = { opp_tbl_L_e4_0,
				ARRAY_SIZE(opp_tbl_L_e4_0) },
	},
	/* LL */
	{
		[CPU_LEVEL_0] = { opp_tbl_LL_e0_0,
				ARRAY_SIZE(opp_tbl_LL_e0_0) },
		[CPU_LEVEL_1] = { opp_tbl_LL_e1_0,
				ARRAY_SIZE(opp_tbl_LL_e1_0) },
		[CPU_LEVEL_2] = { opp_tbl_LL_e2_0,
				ARRAY_SIZE(opp_tbl_LL_e2_0) },
		[CPU_LEVEL_3] = { opp_tbl_LL_e3_0,
				ARRAY_SIZE(opp_tbl_LL_e3_0) },
		[CPU_LEVEL_4] = { opp_tbl_LL_e4_0,
				ARRAY_SIZE(opp_tbl_LL_e4_0) },
	},
	/* CCI */
	{
		[CPU_LEVEL_0] = { opp_tbl_CCI_e0_0,
				ARRAY_SIZE(opp_tbl_CCI_e0_0) },
		[CPU_LEVEL_1] = { opp_tbl_CCI_e1_0,
				ARRAY_SIZE(opp_tbl_CCI_e1_0) },
		[CPU_LEVEL_2] = { opp_tbl_CCI_e2_0,
				ARRAY_SIZE(opp_tbl_CCI_e2_0) },
		[CPU_LEVEL_3] = { opp_tbl_CCI_e3_0,
				ARRAY_SIZE(opp_tbl_CCI_e3_0) },
		[CPU_LEVEL_4] = { opp_tbl_CCI_e4_0,
				ARRAY_SIZE(opp_tbl_CCI_e4_0) },
	},
};

/* 16 steps OPP table */
/* FY */
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

/* SB */
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

/* C65T */
static struct mt_cpu_freq_method opp_tbl_method_L_C65T[] = {
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

static struct mt_cpu_freq_method opp_tbl_method_LL_C65T[] = {
	/* POS,	CLK */
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
	FP(4,	1),
	FP(4,	1),
	FP(4,	1),
	FP(4,	2),
};

static struct mt_cpu_freq_method opp_tbl_method_CCI_C65T[] = {
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

/* C65 */
static struct mt_cpu_freq_method opp_tbl_method_L_C65[] = {
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

static struct mt_cpu_freq_method opp_tbl_method_LL_C65[] = {
	/* POS,	CLK */
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
	FP(4,	1),
	FP(4,	1),
	FP(4,	1),
	FP(4,	2),
};

static struct mt_cpu_freq_method opp_tbl_method_CCI_C65[] = {
	/* POS,	CLK */
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
	FP(4,	1),
	FP(4,	2),
	FP(4,	2),
	FP(4,	2),
};

/* C62 */
static struct mt_cpu_freq_method opp_tbl_method_L_C62[] = {
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

static struct mt_cpu_freq_method opp_tbl_method_LL_C62[] = {
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
	FP(4,	1),
	FP(4,	1),
	FP(4,	1),
	FP(4,	2),
	FP(4,	2),
};

static struct mt_cpu_freq_method opp_tbl_method_CCI_C62[] = {
	/* POS,	CLK */
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
	FP(4,	1),
	FP(4,	2),
	FP(4,	2),
	FP(4,	2),
	FP(4,	2),
};

/* v0.7 */
struct opp_tbl_m_info opp_tbls_m[NR_MT_CPU_DVFS][NUM_CPU_LEVEL] = {
	/* L */
	{
		[CPU_LEVEL_0] = { opp_tbl_method_L_FY },
		[CPU_LEVEL_1] = { opp_tbl_method_L_SB },
		[CPU_LEVEL_2] = { opp_tbl_method_L_C65T },
		[CPU_LEVEL_3] = { opp_tbl_method_L_C65 },
		[CPU_LEVEL_4] = { opp_tbl_method_L_C62 },
	},
	/* LL */
	{
		[CPU_LEVEL_0] = { opp_tbl_method_LL_FY },
		[CPU_LEVEL_1] = { opp_tbl_method_LL_SB },
		[CPU_LEVEL_2] = { opp_tbl_method_LL_C65T },
		[CPU_LEVEL_3] = { opp_tbl_method_LL_C65 },
		[CPU_LEVEL_4] = { opp_tbl_method_LL_C62 },
	},
	/* CCI */
	{
		[CPU_LEVEL_0] = { opp_tbl_method_CCI_FY },
		[CPU_LEVEL_1] = { opp_tbl_method_CCI_SB },
		[CPU_LEVEL_2] = { opp_tbl_method_CCI_C65T },
		[CPU_LEVEL_3] = { opp_tbl_method_CCI_C65 },
		[CPU_LEVEL_4] = { opp_tbl_method_CCI_C62 },
	},
};
