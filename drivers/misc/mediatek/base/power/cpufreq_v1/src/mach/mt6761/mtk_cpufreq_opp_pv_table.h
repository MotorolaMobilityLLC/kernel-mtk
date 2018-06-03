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

#include "mtk_cpufreq_config.h"

#define NR_FREQ		16
#define ARRAY_COL_SIZE	4

static unsigned int fyTbl[NR_FREQ * NR_MT_CPU_DVFS][ARRAY_COL_SIZE] = {
	/* Freq, Vproc, post_div, clk_div */
	{ 2001, 81, 1, 1 },	/* LL */
	{ 1934, 77, 1, 1 },
	{ 1867, 73, 1, 1 },
	{ 1800, 69, 1, 1 },
	{ 1734, 65, 1, 1 },
	{ 1667, 61, 1, 1 },
	{ 1600, 57, 1, 1 },
	{ 1533, 53, 1, 1 },
	{ 1466, 49, 2, 1 },
	{ 1400, 45, 2, 1 },
	{ 1308, 41, 2, 1 },
	{ 1216, 37, 2, 1 },
	{ 1125, 33, 2, 1 },
	{ 1033, 29, 2, 1 },
	{  941, 25, 2, 1 },
	{  850, 21, 2, 1 },
};

static unsigned int sbTbl[NR_FREQ * NR_MT_CPU_DVFS][ARRAY_COL_SIZE] = {
	/* Freq, Vproc, post_div, clk_div */
	{ 2201, 81, 1, 1 },	/* LL */
	{ 2112, 77, 1, 1 },
	{ 2023, 73, 1, 1 },
	{ 1934, 69, 1, 1 },
	{ 1845, 65, 1, 1 },
	{ 1756, 61, 1, 1 },
	{ 1667, 57, 1, 1 },
	{ 1578, 53, 1, 1 },
	{ 1489, 49, 2, 1 },
	{ 1400, 45, 2, 1 },
	{ 1308, 41, 2, 1 },
	{ 1216, 37, 2, 1 },
	{ 1125, 33, 2, 1 },
	{ 1033, 29, 2, 1 },
	{  941, 25, 2, 1 },
	{  850, 21, 2, 1 },
};

static unsigned int fy2Tbl[NR_FREQ * NR_MT_CPU_DVFS][ARRAY_COL_SIZE] = {
	/* Freq, Vproc, post_div, clk_div */
	{ 2001, 81, 1, 1 },	/* LL */
	{ 1934, 77, 1, 1 },
	{ 1867, 73, 1, 1 },
	{ 1800, 69, 1, 1 },
	{ 1734, 65, 1, 1 },
	{ 1667, 61, 1, 1 },
	{ 1600, 57, 1, 1 },
	{ 1533, 53, 1, 1 },
	{ 1466, 49, 2, 1 },
	{ 1400, 45, 2, 1 },
	{ 1308, 45, 2, 1 },
	{ 1216, 45, 2, 1 },
	{ 1125, 45, 2, 1 },
	{ 1033, 45, 2, 1 },
	{  941, 45, 2, 1 },
	{  850, 45, 2, 1 },
};

unsigned int *xrecordTbl[NUM_CPU_LEVEL] = {	/* v0.2 */
	[CPU_LEVEL_0] = &fyTbl[0][0],
	[CPU_LEVEL_1] = &sbTbl[0][0],
	[CPU_LEVEL_2] = &fy2Tbl[0][0],
};
