/*
 * Copyright (C) 2017 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef __MTK_CM_MGR_DATA_H__
#define __MTK_CM_MGR_DATA_H__

#include <mtk_dramc.h>
#include <mtk_vcorefs_governor.h>

#define ATF_SECURE_SMC
/* #define ATF_SECURE_SMC_PER_CPU_STALL_RATIO */
#define LIGHT_LOAD
/* #define USE_AVG_PMU */

#define CM_MGR_EMI_OPP	2
#define CM_MGR_LOWER_OPP 10
#define CM_MGR_CPU_CLUSTER 2
#define CM_MGR_CPU_COUNT 8

#define VCORE_ARRAY_SIZE CM_MGR_EMI_OPP
#define CM_MGR_CPU_ARRAY_SIZE (CM_MGR_CPU_CLUSTER * CM_MGR_EMI_OPP)
#define RATIO_COUNT (100 / 5 - 1)
#define IS_UP 1
#define IS_DOWN 0

extern void spm_request_dvfs_opp(int id, enum dvfs_opp opp);
extern void stall_val_dbg_enable(int enable);
extern void stall_pmu_enable(int enable);

struct cm_mgr_met_data {
	unsigned int cm_mgr_power[14];
	unsigned int cm_mgr_count[4];
	unsigned int cm_mgr_opp[6];
	unsigned int cm_mgr_loading[12];
	unsigned int cm_mgr_ratio[4];
	unsigned int cm_mgr_bw;
	unsigned int cm_mgr_valid[2];
};

static int light_load_cps = 3000;
static int cm_mgr_loop_count = 10;
static int cm_mgr_loop;
static int total_bw_table_limit = 6;
static int total_bw_value;
static int cpu_power_ratio_up[CM_MGR_EMI_OPP] = {100, 100};
static int cpu_power_ratio_down[CM_MGR_EMI_OPP] = {100, 100};
static int vcore_power_ratio_up[CM_MGR_EMI_OPP] = {80, 100};
static int vcore_power_ratio_down[CM_MGR_EMI_OPP] = {80, 100};
static int debounce_times_up_adb[CM_MGR_EMI_OPP] = {0, 3};
static int debounce_times_down_adb[CM_MGR_EMI_OPP] = {0, 3};
static int debounce_times_reset_adb;
static int update;
static int update_v2f_table;
static int cm_mgr_opp_enable = 1;
static int cm_mgr_enable = 1;
static int is_lp3;
static int cm_mgr_disable_fb = 1;
static int cm_mgr_blank_status;

/* Last data update: 20171103 */
static unsigned int vcore_power_gain_lp4[][VCORE_ARRAY_SIZE] = {
	{118, 451},
	{146, 525},
	{199, 605},
	{188, 636},
	{208, 646},
	{238, 643},
	{242, 677},
	{240, 722},
	{238, 750},
	{240, 763},
	{251, 767},
	{251, 771},
	{251, 775},
	{251, 780},
	{251, 784},
};

static unsigned int vcore_power_gain_lp3[][VCORE_ARRAY_SIZE] = {
	{87, 454},
	{109, 508},
	{157, 568},
	{140, 580},
	{154, 570},
	{179, 547},
	{177, 562},
	{169, 588},
	{161, 596},
	{157, 590},
	{162, 574},
	{162, 559},
	{162, 544},
	{162, 528},
	{162, 513},
};

#define VCORE_POWER_ARRAY_SIZE \
	(sizeof(vcore_power_gain_lp4) / sizeof(unsigned int) / VCORE_ARRAY_SIZE)

static unsigned int _v2f_all[][CM_MGR_CPU_CLUSTER] = {
	{212, 293},
	{198, 271},
	{184, 251},
	{172, 231},
	{157, 214},
	{145, 198},
	{131, 183},
	{120, 169},
	{106, 151},
	{91, 131},
	{80, 115},
	{65, 94},
	{51, 75},
	{40, 57},
	{26, 37},
	{12, 19},
};

#ifndef ATF_SECURE_SMC_PER_CPU_STALL_RATIO
/* LP4 Table */
static unsigned int cpu_power_gain_up_low_1[][CM_MGR_CPU_ARRAY_SIZE] = {
	{2, 3, 1, 2},
	{3, 6, 2, 5},
	{5, 10, 4, 7},
	{6, 13, 5, 10},
	{81, 104, 6, 12},
	{77, 101, 7, 14},
	{74, 99, 71, 93},
	{71, 96, 68, 90},
	{67, 93, 64, 86},
	{64, 91, 60, 83},
	{61, 88, 57, 79},
	{58, 85, 53, 76},
	{54, 83, 49, 72},
	{51, 80, 45, 69},
	{48, 78, 42, 65},
	{44, 75, 38, 62},
	{41, 72, 34, 59},
	{38, 70, 31, 55},
	{34, 67, 27, 52},
	{31, 64, 23, 48},
};

static unsigned int cpu_power_gain_down_low_1[][CM_MGR_CPU_ARRAY_SIZE] = {
	{2, 5, 2, 3},
	{5, 10, 3, 6},
	{7, 14, 5, 10},
	{93, 121, 6, 13},
	{91, 119, 87, 111},
	{88, 118, 83, 108},
	{85, 116, 79, 105},
	{82, 115, 76, 102},
	{79, 113, 72, 99},
	{76, 112, 68, 96},
	{73, 110, 64, 92},
	{70, 109, 61, 89},
	{67, 107, 57, 86},
	{64, 106, 53, 83},
	{61, 104, 50, 80},
	{58, 103, 46, 77},
	{56, 101, 42, 74},
	{53, 100, 39, 71},
	{50, 98, 35, 68},
	{47, 97, 31, 64},
};

static unsigned int cpu_power_gain_up_high_1[][CM_MGR_CPU_ARRAY_SIZE] = {
	{2, 3, 1, 2},
	{3, 6, 2, 5},
	{5, 10, 4, 7},
	{6, 13, 5, 10},
	{8, 16, 6, 12},
	{9, 19, 7, 14},
	{11, 23, 8, 17},
	{114, 149, 9, 19},
	{107, 142, 11, 22},
	{101, 135, 12, 24},
	{94, 128, 89, 119},
	{87, 121, 82, 111},
	{80, 114, 75, 103},
	{73, 107, 67, 95},
	{66, 99, 60, 87},
	{59, 92, 53, 80},
	{52, 85, 45, 72},
	{45, 78, 38, 64},
	{38, 71, 31, 56},
	{31, 64, 23, 48},
};

static unsigned int cpu_power_gain_down_high_1[][CM_MGR_CPU_ARRAY_SIZE] = {
	{2, 5, 2, 3},
	{5, 10, 3, 6},
	{7, 14, 5, 10},
	{9, 19, 6, 13},
	{12, 24, 8, 16},
	{133, 172, 9, 19},
	{127, 167, 11, 23},
	{121, 161, 114, 149},
	{114, 156, 107, 142},
	{108, 151, 101, 135},
	{102, 145, 94, 128},
	{96, 140, 87, 121},
	{90, 134, 80, 114},
	{84, 129, 73, 107},
	{78, 124, 66, 99},
	{71, 118, 59, 92},
	{65, 113, 52, 85},
	{59, 107, 45, 78},
	{53, 102, 38, 71},
	{47, 97, 31, 64},
};

/* LP3 Table */
static unsigned int cpu_power_gain_up_low_2[][CM_MGR_CPU_ARRAY_SIZE] = {
	{1, 2, 1, 1},
	{2, 5, 1, 3},
	{4, 7, 2, 4},
	{5, 10, 3, 6},
	{6, 12, 3, 7},
	{7, 14, 4, 8},
	{71, 93, 5, 10},
	{68, 90, 5, 11},
	{64, 86, 6, 12},
	{60, 83, 7, 14},
	{57, 79, 7, 15},
	{53, 76, 47, 63},
	{49, 72, 43, 59},
	{45, 69, 38, 54},
	{42, 65, 34, 50},
	{38, 62, 30, 45},
	{34, 59, 26, 41},
	{31, 55, 22, 36},
	{27, 52, 18, 32},
	{23, 48, 13, 28},
};

static unsigned int cpu_power_gain_down_low_2[][CM_MGR_CPU_ARRAY_SIZE] = {
	{2, 3, 1, 2},
	{3, 6, 2, 3},
	{5, 10, 2, 5},
	{6, 13, 3, 6},
	{87, 111, 4, 8},
	{83, 108, 5, 10},
	{79, 105, 5, 11},
	{76, 102, 6, 13},
	{72, 99, 7, 14},
	{68, 96, 60, 79},
	{64, 92, 56, 75},
	{61, 89, 51, 70},
	{57, 86, 47, 65},
	{53, 83, 42, 61},
	{50, 80, 38, 56},
	{46, 77, 33, 51},
	{42, 74, 29, 46},
	{39, 71, 25, 42},
	{35, 68, 20, 37},
	{31, 64, 16, 32},
};

static unsigned int cpu_power_gain_up_high_2[][CM_MGR_CPU_ARRAY_SIZE] = {
	{1, 2, 1, 1},
	{2, 5, 1, 3},
	{4, 7, 2, 4},
	{5, 10, 3, 6},
	{6, 12, 3, 7},
	{7, 14, 4, 8},
	{8, 17, 5, 10},
	{9, 19, 5, 11},
	{11, 22, 6, 12},
	{12, 24, 7, 14},
	{89, 119, 7, 15},
	{82, 111, 8, 17},
	{75, 103, 9, 18},
	{67, 95, 9, 19},
	{60, 87, 10, 21},
	{53, 80, 11, 22},
	{45, 72, 11, 23},
	{38, 64, 12, 25},
	{31, 56, 21, 36},
	{23, 48, 13, 28},
};

static unsigned int cpu_power_gain_down_high_2[][CM_MGR_CPU_ARRAY_SIZE] = {
	{2, 3, 1, 2},
	{3, 6, 2, 3},
	{5, 10, 2, 5},
	{6, 13, 3, 6},
	{8, 16, 4, 8},
	{9, 19, 5, 10},
	{11, 23, 5, 11},
	{114, 149, 6, 13},
	{107, 142, 7, 14},
	{101, 135, 8, 16},
	{94, 128, 9, 18},
	{87, 121, 9, 19},
	{80, 114, 10, 21},
	{73, 107, 11, 22},
	{66, 99, 12, 24},
	{59, 92, 46, 67},
	{52, 85, 39, 58},
	{45, 78, 31, 49},
	{38, 71, 23, 41},
	{31, 64, 16, 32},
};

static unsigned int *cpu_power_gain_up = &cpu_power_gain_up_high_1[0][0];
static unsigned int *cpu_power_gain_down = &cpu_power_gain_down_high_1[0][0];
static unsigned int *vcore_power_gain = &vcore_power_gain_lp4[0][0];
#define cpu_power_gain(p, i, j) (*(p + (i) * CM_MGR_CPU_ARRAY_SIZE + (j)))
#define vcore_power_gain(p, i, j) (*(p + (i) * VCORE_ARRAY_SIZE + (j)))

static int cpu_power_gain_opp(int bw, int is_up, int opp, int ratio_idx, int idx)
{
	if (opp < CM_MGR_LOWER_OPP) {
		if (is_lp3) {
			cpu_power_gain_up = &cpu_power_gain_up_low_2[0][0];
			cpu_power_gain_down = &cpu_power_gain_down_low_2[0][0];
		} else {
			cpu_power_gain_up = &cpu_power_gain_up_low_1[0][0];
			cpu_power_gain_down = &cpu_power_gain_down_low_1[0][0];
		}
	} else {
		if (is_lp3) {
			cpu_power_gain_up = &cpu_power_gain_up_high_2[0][0];
			cpu_power_gain_down = &cpu_power_gain_down_high_2[0][0];
		} else {
			cpu_power_gain_up = &cpu_power_gain_up_high_1[0][0];
			cpu_power_gain_down = &cpu_power_gain_down_high_1[0][0];
		}
	}

	if (is_up)
		return cpu_power_gain(cpu_power_gain_up, ratio_idx, idx);
	else
		return cpu_power_gain(cpu_power_gain_down, ratio_idx, idx);
}

static int get_dram_opp(void)
{
#ifdef CONFIG_MTK_DRAMC
	unsigned int datarate = get_dram_data_rate();
	int opp = 0;

	if (is_lp3) {
		switch (datarate) {
		case 1200:
			opp = 2;
			break;
		case 1600:
			opp = 1;
			break;
		case 1866:
		default:
			opp = 0;
			break;
		}
	} else {
		switch (datarate) {
		case 1600:
			opp = 2;
			break;
		case 2400:
			opp = 1;
			break;
		case 3000:
		case 3200:
		default:
			opp = 0;
			break;
		}
	}

	return opp;
#else
	return 0;
#endif
}

static void set_dram_opp(int opp)
{
	switch (opp) {
	case 0:
		spm_request_dvfs_opp(1, OPP_0);
		break;
	case 1:
		spm_request_dvfs_opp(1, OPP_2);
		break;
	case 2:
		spm_request_dvfs_opp(1, OPP_3);
		break;
	default:
		break;
	}
}

#endif

#endif	/* __MTK_CM_MGR_DATA_H__ */
