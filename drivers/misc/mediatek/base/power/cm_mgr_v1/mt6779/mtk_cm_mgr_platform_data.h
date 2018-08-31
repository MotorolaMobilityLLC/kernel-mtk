/*
 * Copyright (C) 2015 MediaTek Inc.
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

#ifndef __MTK_CM_MGR_PLATFORM_DATA_H__
#define __MTK_CM_MGR_PLATFORM_DATA_H__

#ifndef PROC_FOPS_RW
#define PROC_FOPS_RW(name)					\
	static int name ## _proc_open(struct inode *inode,	\
		struct file *file)				\
	{							\
		return single_open(file, name ## _proc_show,	\
			PDE_DATA(inode));			\
	}							\
	static const struct file_operations name ## _proc_fops = {	\
		.owner		  = THIS_MODULE,			\
		.open		   = name ## _proc_open,		\
		.read		   = seq_read,				\
		.llseek		 = seq_lseek,				\
		.release		= single_release,		\
		.write		  = name ## _proc_write,		\
	}
#endif /* PROC_FOPS_RW */

#ifndef PROC_FOPS_RO
#define PROC_FOPS_RO(name)					\
	static int name ## _proc_open(struct inode *inode,	\
		struct file *file)				\
	{							\
		return single_open(file, name ## _proc_show,	\
			PDE_DATA(inode));			\
	}							\
	static const struct file_operations name ## _proc_fops = {	\
		.owner		  = THIS_MODULE,			\
		.open		   = name ## _proc_open,		\
		.read		   = seq_read,				\
		.llseek		 = seq_lseek,				\
		.release		= single_release,		\
	}
#endif /* PROC_FOPS_RO */

#ifndef PROC_ENTRY
#define PROC_ENTRY(name)	{__stringify(name), &name ## _proc_fops}
#endif /* PROC_ENTRY */

int light_load_cps = 1000;
static int cm_mgr_loop_count;
static int cm_mgr_dram_level;
static int cm_mgr_loop;
static int total_bw_value;
int cpu_power_ratio_up[CM_MGR_EMI_OPP] = {100, 100, 100, 100};
int cpu_power_ratio_down[CM_MGR_EMI_OPP] = {100, 100, 100, 100};
int vcore_power_ratio_up[CM_MGR_EMI_OPP] = {100, 100, 100, 100};
int vcore_power_ratio_down[CM_MGR_EMI_OPP] = {100, 100, 100, 100};
int debounce_times_up_adb[CM_MGR_EMI_OPP] = {3, 3, 3, 3};
int debounce_times_down_adb[CM_MGR_EMI_OPP] = {0, 0, 0, 0};
int debounce_times_reset_adb = 1;
int debounce_times_perf_down = 50;
int debounce_times_perf_force_down = 100;
static int update;
static int emi_latency = 1;
static int update_v2f_table = 1;
static int cm_mgr_opp_enable = 1;
int cm_mgr_enable = 1;
#if defined(CONFIG_MTK_TINYSYS_SSPM_SUPPORT) && defined(USE_CM_MGR_AT_SSPM)
int cm_mgr_sspm_enable = 1;
#endif /* CONFIG_MTK_TINYSYS_SSPM_SUPPORT */
#ifdef USE_TIMER_CHECK
int cm_mgr_timer_enable = 1;
#endif /* USE_TIMER_CHECK */
int cm_mgr_ratio_timer_enable;
int cm_mgr_disable_fb = 1;
int cm_mgr_blank_status;
int cm_mgr_perf_enable = 1;
int cm_mgr_perf_timer_enable;
int cm_mgr_perf_force_enable;
int cm_mgr_loading_level = 7000;
int cm_mgr_loading_enable = 1;

static int vcore_power_gain_0[][VCORE_ARRAY_SIZE] = {
	{150, 119, 311, 460},
	{130, 82, 298, 480},
	{90, 64, 286, 520},
	{70, 100, 300, 550},
	{60, 180, 320, 580},
	{60, 170, 430, 620},
	{60, 160, 460, 730},
	{60, 140, 480, 750},
	{60, 150, 490, 770},
	{60, 140, 520, 790},
	{60, 140, 550, 810},
	{60, 140, 570, 840},
};

#define VCORE_POWER_ARRAY_SIZE(name) \
	(sizeof(vcore_power_gain_##name) / \
	 sizeof(unsigned int) / \
	 VCORE_ARRAY_SIZE)

#define VCORE_POWER_GAIN_PTR(name) \
	(&vcore_power_gain_##name[0][0])

static int vcore_power_array_size(int idx)
{
	switch (idx) {
	case 0:
		return VCORE_POWER_ARRAY_SIZE(0);
	}

	pr_info("#@# %s(%d) warning value %d\n", __func__, __LINE__, idx);
	return 0;
};

static int *vcore_power_gain_ptr(int idx)
{
	switch (idx) {
	case 0:
		return VCORE_POWER_GAIN_PTR(0);
	}

	pr_info("#@# %s(%d) warning value %d\n", __func__, __LINE__, idx);
	return NULL;
};

static int *vcore_power_gain = VCORE_POWER_GAIN_PTR(0);
#define vcore_power_gain(p, i, j) (*(p + (i) * VCORE_ARRAY_SIZE + (j)))

static unsigned int _v2f_all[][CM_MGR_CPU_CLUSTER] = {
	{280, 275},
	{236, 232},
	{180, 180},
	{151, 151},
	{125, 125},
	{113, 113},
	{102, 102},
	{93, 93},
	{84, 84},
	{76, 76},
	{68, 68},
	{59, 59},
	{50, 50},
	{42, 42},
	{35, 35},
	{29, 29},
};

static unsigned int cpu_power_gain_UpLow0[][CM_MGR_CPU_ARRAY_SIZE] = {
	{4, 14, 2, 5, 2, 6, 1, 3},
	{9, 27, 3, 10, 4, 12, 2, 5},
	{82, 338, 5, 14, 6, 18, 3, 8},
	{82, 334, 6, 19, 8, 24, 3, 11},
	{82, 330, 8, 24, 10, 30, 4, 13},
	{83, 326, 9, 29, 68, 280, 5, 16},
	{83, 322, 11, 33, 66, 269, 6, 19},
	{84, 319, 61, 248, 64, 258, 7, 21},
	{84, 315, 58, 235, 62, 246, 8, 24},
	{84, 311, 56, 222, 60, 235, 9, 27},
	{85, 307, 53, 209, 58, 223, 9, 29},
	{85, 304, 51, 197, 56, 212, 10, 32},
	{85, 300, 48, 184, 53, 201, 39, 157},
	{86, 296, 46, 171, 51, 189, 36, 142},
	{86, 292, 43, 159, 49, 178, 33, 127},
	{87, 288, 41, 146, 47, 166, 30, 112},
	{87, 285, 38, 133, 45, 155, 27, 98},
	{87, 281, 36, 121, 43, 144, 24, 83},
	{88, 277, 33, 108, 41, 132, 20, 68},
	{88, 273, 31, 95, 39, 121, 17, 53},
};

static unsigned int cpu_power_gain_DownLow0[][CM_MGR_CPU_ARRAY_SIZE] = {
	{7, 21, 2, 5, 2, 7, 1, 3},
	{90, 383, 4, 11, 5, 14, 2, 6},
	{93, 385, 5, 16, 7, 21, 3, 9},
	{95, 387, 7, 22, 9, 29, 4, 11},
	{98, 389, 9, 27, 75, 276, 5, 14},
	{100, 391, 11, 33, 73, 267, 6, 17},
	{103, 393, 68, 284, 71, 258, 6, 20},
	{105, 396, 65, 271, 70, 250, 7, 23},
	{108, 398, 63, 257, 68, 241, 8, 26},
	{111, 400, 60, 244, 66, 232, 9, 29},
	{113, 402, 58, 230, 64, 223, 10, 31},
	{116, 404, 55, 217, 62, 214, 45, 163},
	{118, 406, 53, 203, 60, 205, 42, 149},
	{121, 408, 50, 190, 58, 196, 38, 136},
	{123, 410, 48, 176, 56, 187, 35, 123},
	{126, 413, 45, 163, 54, 178, 32, 110},
	{128, 415, 43, 149, 52, 170, 28, 97},
	{131, 417, 40, 136, 50, 161, 25, 83},
	{133, 419, 38, 122, 48, 152, 22, 70},
	{136, 421, 35, 109, 46, 143, 18, 57},
};

static unsigned int cpu_power_gain_UpHigh0[][CM_MGR_CPU_ARRAY_SIZE] = {
	{4, 14, 2, 5, 2, 6, 1, 3},
	{9, 27, 3, 10, 4, 12, 2, 5},
	{13, 41, 5, 14, 6, 18, 3, 8},
	{18, 55, 6, 19, 8, 24, 3, 11},
	{115, 425, 8, 24, 10, 30, 4, 13},
	{113, 415, 9, 29, 12, 36, 5, 16},
	{111, 405, 11, 33, 14, 42, 6, 19},
	{109, 395, 12, 38, 16, 48, 7, 21},
	{108, 385, 14, 43, 18, 54, 8, 24},
	{106, 374, 15, 48, 81, 298, 9, 27},
	{104, 364, 17, 52, 77, 280, 9, 29},
	{102, 354, 18, 57, 73, 263, 10, 32},
	{101, 344, 63, 228, 69, 245, 11, 35},
	{99, 334, 59, 209, 64, 227, 12, 37},
	{97, 324, 54, 190, 60, 209, 13, 40},
	{95, 314, 49, 171, 56, 192, 14, 43},
	{94, 304, 45, 152, 52, 174, 15, 45},
	{92, 293, 40, 133, 47, 156, 15, 48},
	{90, 283, 35, 114, 43, 138, 16, 51},
	{88, 273, 31, 95, 39, 121, 17, 53},
};

static unsigned int cpu_power_gain_DownHigh0[][CM_MGR_CPU_ARRAY_SIZE] = {
	{7, 21, 2, 5, 2, 7, 1, 3},
	{14, 42, 4, 11, 5, 14, 2, 6},
	{142, 529, 5, 16, 7, 21, 3, 9},
	{141, 522, 7, 22, 9, 29, 4, 11},
	{141, 516, 9, 27, 12, 36, 5, 14},
	{141, 510, 11, 33, 14, 43, 6, 17},
	{140, 503, 12, 38, 16, 50, 6, 20},
	{140, 497, 14, 43, 18, 57, 7, 23},
	{140, 491, 16, 49, 99, 335, 8, 26},
	{139, 484, 18, 54, 94, 318, 9, 29},
	{139, 478, 19, 60, 90, 300, 10, 31},
	{139, 472, 78, 284, 85, 283, 11, 34},
	{138, 465, 73, 262, 80, 265, 12, 37},
	{138, 459, 67, 240, 75, 248, 13, 40},
	{138, 453, 62, 218, 70, 230, 14, 43},
	{137, 446, 57, 196, 65, 213, 15, 46},
	{137, 440, 51, 175, 61, 195, 16, 49},
	{137, 434, 46, 153, 56, 178, 17, 51},
	{136, 427, 40, 131, 51, 160, 18, 54},
	{136, 421, 35, 109, 46, 143, 18, 57},
};

#define cpu_power_gain(p, i, j) (*(p + (i) * CM_MGR_CPU_ARRAY_SIZE + (j)))
#define CPU_POWER_GAIN(a, b, c) \
	(&cpu_power_gain_##a##b##c[0][0])

static unsigned int *_cpu_power_gain_ptr(int isUP, int isLow, int idx)
{
	switch (isUP) {
	case 0:
		switch (isLow) {
		case 0:
			switch (idx) {
			case 0:
				return CPU_POWER_GAIN(Down, High, 0);
			}
			break;
		case 1:
			switch (idx) {
			case 0:
				return CPU_POWER_GAIN(Down, Low, 0);
			}
			break;
		}
		break;
	case 1:
		switch (isLow) {
		case 0:
			switch (idx) {
			case 0:
				return CPU_POWER_GAIN(Up, High, 0);
			}
			break;
		case 1:
			switch (idx) {
			case 0:
				return CPU_POWER_GAIN(Up, Low, 0);
			}
			break;
		}
		break;
	}

	pr_info("#@# %s(%d) warning value %d\n", __func__, __LINE__, idx);
	return NULL;
};

static unsigned int *cpu_power_gain_up = CPU_POWER_GAIN(Up, High, 0);
static unsigned int *cpu_power_gain_down = CPU_POWER_GAIN(Down, High, 0);

static void cpu_power_gain_ptr(int opp, int tbl, int cluster)
{
	switch (cluster) {
	case 0:
		if (opp < CM_MGR_LOWER_OPP) {
			switch (tbl) {
			case 0:
				cpu_power_gain_up =
					CPU_POWER_GAIN(Up, Low, 0);
				cpu_power_gain_down =
					CPU_POWER_GAIN(Down, Low, 0);
				break;
			}
		} else {
			switch (tbl) {
			case 0:
				cpu_power_gain_up =
					CPU_POWER_GAIN(Up, High, 0);
				cpu_power_gain_down =
					CPU_POWER_GAIN(Down, High, 0);
				break;
			}
		}
		break;
	case 1:
		if (opp < CM_MGR_LOWER_OPP_1) {
			switch (tbl) {
			case 0:
				cpu_power_gain_up =
					CPU_POWER_GAIN(Up, Low, 0);
				cpu_power_gain_down =
					CPU_POWER_GAIN(Down, Low, 0);
				break;
			}
		} else {
			switch (tbl) {
			case 0:
				cpu_power_gain_up =
					CPU_POWER_GAIN(Up, High, 0);
				cpu_power_gain_down =
					CPU_POWER_GAIN(Down, High, 0);
				break;
			}
		}
		break;
	}
}

int cpu_power_gain_opp(int bw, int is_up, int opp, int ratio_idx, int idx)
{
	cpu_power_gain_ptr(opp, cm_mgr_get_idx(), idx % CM_MGR_CPU_CLUSTER);

	if (is_up)
		return cpu_power_gain(cpu_power_gain_up, ratio_idx, idx);
	else
		return cpu_power_gain(cpu_power_gain_down, ratio_idx, idx);
}

#endif	/* __MTK_CM_MGR_PLATFORM_DATA_H__ */
