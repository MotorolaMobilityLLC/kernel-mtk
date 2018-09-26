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
	{2, 10, 0, 1, 1, 6, 1, 3},
	{4, 19, 0, 2, 3, 11, 2, 7},
	{7, 29, 1, 3, 4, 17, 2, 10},
	{86, 300, 1, 3, 5, 22, 3, 14},
	{83, 293, 1, 4, 6, 28, 4, 17},
	{81, 286, 1, 5, 8, 34, 5, 21},
	{78, 280, 1, 6, 72, 251, 5, 24},
	{75, 273, 2, 7, 68, 240, 6, 28},
	{73, 267, 2, 8, 64, 230, 7, 31},
	{70, 260, 2, 9, 61, 219, 8, 35},
	{67, 254, 2, 9, 57, 208, 52, 185},
	{65, 247, 2, 10, 54, 198, 48, 172},
	{62, 241, 2, 11, 50, 187, 44, 159},
	{59, 234, 3, 12, 47, 176, 40, 147},
	{57, 227, 3, 13, 43, 165, 36, 134},
	{54, 221, 3, 14, 39, 155, 32, 121},
	{51, 214, 3, 15, 36, 144, 28, 108},
	{49, 208, 3, 15, 32, 133, 24, 95},
	{46, 201, 4, 16, 29, 123, 20, 83},
	{44, 195, 4, 17, 25, 112, 16, 70},
};

static unsigned int cpu_power_gain_DownLow0[][CM_MGR_CPU_ARRAY_SIZE] = {
	{3, 12, 0, 1, 1, 6, 1, 4},
	{6, 25, 0, 2, 3, 13, 2, 8},
	{95, 309, 1, 3, 4, 19, 3, 11},
	{93, 306, 1, 4, 6, 26, 3, 15},
	{91, 302, 1, 4, 7, 32, 4, 19},
	{88, 298, 1, 5, 80, 270, 5, 23},
	{86, 295, 1, 6, 77, 260, 6, 26},
	{84, 291, 2, 7, 73, 249, 7, 30},
	{81, 287, 2, 8, 69, 239, 8, 34},
	{79, 284, 2, 9, 66, 229, 60, 203},
	{76, 280, 2, 10, 62, 219, 55, 190},
	{74, 276, 2, 11, 58, 209, 51, 178},
	{72, 273, 3, 11, 54, 199, 47, 165},
	{69, 269, 3, 12, 51, 189, 43, 152},
	{67, 265, 3, 13, 47, 178, 38, 139},
	{65, 262, 3, 14, 43, 168, 34, 127},
	{62, 258, 3, 15, 40, 158, 30, 114},
	{60, 254, 4, 16, 36, 148, 25, 101},
	{58, 251, 4, 17, 32, 138, 21, 88},
	{55, 247, 4, 18, 29, 128, 17, 76},
};

static unsigned int cpu_power_gain_UpHigh0[][CM_MGR_CPU_ARRAY_SIZE] = {
	{2, 10, 0, 1, 1, 6, 1, 3},
	{4, 19, 0, 2, 3, 11, 2, 7},
	{7, 29, 1, 3, 4, 17, 2, 10},
	{9, 39, 1, 3, 5, 22, 3, 14},
	{11, 49, 1, 4, 6, 28, 4, 17},
	{13, 58, 1, 5, 8, 34, 5, 21},
	{15, 68, 1, 6, 9, 39, 5, 24},
	{97, 354, 2, 7, 10, 45, 6, 28},
	{93, 341, 2, 8, 11, 50, 7, 31},
	{88, 328, 2, 9, 13, 56, 8, 35},
	{84, 315, 2, 9, 14, 62, 9, 38},
	{79, 301, 2, 10, 15, 67, 9, 42},
	{75, 288, 2, 11, 16, 73, 10, 45},
	{70, 275, 3, 12, 57, 217, 11, 49},
	{66, 261, 3, 13, 52, 199, 12, 52},
	{61, 248, 3, 14, 47, 182, 12, 56},
	{57, 235, 3, 15, 41, 164, 13, 59},
	{52, 221, 3, 15, 36, 147, 14, 63},
	{48, 208, 4, 16, 30, 130, 15, 66},
	{44, 195, 4, 17, 25, 112, 16, 70},
};

static unsigned int cpu_power_gain_DownHigh0[][CM_MGR_CPU_ARRAY_SIZE] = {
	{3, 12, 0, 1, 1, 6, 1, 4},
	{6, 25, 0, 2, 3, 13, 2, 8},
	{8, 37, 1, 3, 4, 19, 3, 11},
	{11, 49, 1, 4, 6, 26, 3, 15},
	{14, 62, 1, 4, 7, 32, 4, 19},
	{123, 468, 1, 5, 9, 38, 5, 23},
	{118, 452, 1, 6, 10, 45, 6, 26},
	{113, 436, 2, 7, 11, 51, 7, 30},
	{109, 420, 2, 8, 13, 57, 8, 34},
	{104, 405, 2, 9, 14, 64, 8, 38},
	{99, 389, 2, 10, 16, 70, 9, 42},
	{94, 373, 2, 11, 78, 258, 10, 45},
	{89, 357, 3, 11, 72, 242, 11, 49},
	{84, 342, 3, 12, 66, 226, 12, 53},
	{79, 326, 3, 13, 59, 209, 13, 57},
	{75, 310, 3, 14, 53, 193, 14, 60},
	{70, 294, 3, 15, 47, 177, 14, 64},
	{65, 279, 4, 16, 41, 160, 15, 68},
	{60, 263, 4, 17, 35, 144, 16, 72},
	{55, 247, 4, 18, 29, 128, 17, 76},
};

static unsigned int cpu_power_gain_UpLow1[][CM_MGR_CPU_ARRAY_SIZE] = {
	{2, 10, 0, 1, 1, 6, 1, 6},
	{4, 19, 0, 2, 3, 11, 3, 11},
	{7, 29, 1, 3, 4, 17, 4, 17},
	{86, 300, 1, 3, 5, 22, 5, 23},
	{83, 293, 1, 4, 6, 28, 6, 29},
	{81, 286, 1, 5, 8, 34, 8, 34},
	{78, 280, 1, 6, 72, 251, 72, 252},
	{75, 273, 2, 7, 68, 240, 68, 241},
	{73, 267, 2, 8, 64, 230, 65, 231},
	{70, 260, 2, 9, 61, 219, 61, 220},
	{67, 254, 2, 9, 57, 208, 58, 209},
	{65, 247, 2, 10, 54, 198, 54, 199},
	{62, 241, 2, 11, 50, 187, 50, 188},
	{59, 234, 3, 12, 47, 176, 47, 178},
	{57, 228, 3, 13, 43, 166, 43, 167},
	{54, 221, 3, 14, 39, 155, 40, 156},
	{51, 214, 3, 15, 36, 144, 36, 146},
	{49, 208, 3, 15, 32, 133, 33, 135},
	{46, 201, 4, 16, 29, 123, 29, 125},
	{44, 195, 4, 17, 25, 112, 25, 114},
};

static unsigned int cpu_power_gain_DownLow1[][CM_MGR_CPU_ARRAY_SIZE] = {
	{3, 12, 0, 1, 1, 6, 1, 7},
	{6, 25, 0, 2, 3, 13, 3, 13},
	{95, 309, 1, 3, 4, 19, 4, 20},
	{93, 306, 1, 4, 6, 26, 6, 26},
	{91, 302, 1, 4, 7, 32, 7, 33},
	{88, 298, 1, 5, 80, 270, 80, 270},
	{86, 295, 1, 6, 77, 260, 77, 260},
	{84, 291, 2, 7, 73, 249, 73, 250},
	{81, 287, 2, 8, 69, 239, 69, 240},
	{79, 284, 2, 9, 66, 229, 66, 230},
	{76, 280, 2, 10, 62, 219, 62, 220},
	{74, 276, 2, 11, 58, 209, 58, 210},
	{72, 273, 3, 11, 54, 199, 55, 200},
	{69, 269, 3, 12, 51, 189, 51, 190},
	{67, 265, 3, 13, 47, 178, 47, 180},
	{65, 262, 3, 14, 43, 168, 44, 170},
	{62, 258, 3, 15, 40, 158, 40, 160},
	{60, 254, 4, 16, 36, 148, 36, 150},
	{58, 251, 4, 17, 32, 138, 33, 140},
	{55, 247, 4, 18, 29, 128, 29, 130},
};

static unsigned int cpu_power_gain_UpHigh1[][CM_MGR_CPU_ARRAY_SIZE] = {
	{2, 10, 0, 1, 1, 6, 1, 6},
	{4, 19, 0, 2, 3, 11, 3, 11},
	{7, 29, 1, 3, 4, 17, 4, 17},
	{9, 39, 1, 3, 5, 22, 5, 23},
	{11, 49, 1, 4, 6, 28, 6, 29},
	{13, 58, 1, 5, 8, 34, 8, 34},
	{15, 68, 1, 6, 9, 39, 9, 40},
	{97, 355, 2, 7, 10, 45, 10, 46},
	{93, 341, 2, 8, 11, 50, 11, 51},
	{88, 328, 2, 9, 13, 56, 13, 57},
	{84, 315, 2, 9, 14, 62, 14, 63},
	{79, 301, 2, 10, 15, 67, 15, 68},
	{75, 288, 2, 11, 16, 73, 63, 236},
	{70, 275, 3, 12, 57, 217, 58, 218},
	{66, 261, 3, 13, 52, 199, 52, 201},
	{61, 248, 3, 14, 47, 182, 47, 184},
	{57, 235, 3, 15, 41, 164, 42, 166},
	{52, 221, 3, 15, 36, 147, 36, 149},
	{48, 208, 4, 16, 30, 130, 31, 131},
	{44, 195, 4, 17, 25, 112, 25, 114},
};

static unsigned int cpu_power_gain_DownHigh1[][CM_MGR_CPU_ARRAY_SIZE] = {
	{3, 12, 0, 1, 1, 6, 1, 7},
	{6, 25, 0, 2, 3, 13, 3, 13},
	{8, 37, 1, 3, 4, 19, 4, 20},
	{11, 49, 1, 4, 6, 26, 6, 26},
	{14, 62, 1, 4, 7, 32, 7, 33},
	{123, 468, 1, 5, 9, 38, 9, 39},
	{118, 452, 1, 6, 10, 45, 10, 46},
	{113, 436, 2, 7, 11, 51, 12, 52},
	{109, 420, 2, 8, 13, 57, 13, 59},
	{104, 405, 2, 9, 14, 64, 15, 65},
	{99, 389, 2, 10, 16, 70, 16, 72},
	{94, 373, 2, 11, 78, 258, 78, 260},
	{89, 357, 3, 11, 72, 242, 72, 244},
	{84, 342, 3, 12, 66, 226, 66, 227},
	{79, 326, 3, 13, 59, 209, 60, 211},
	{75, 310, 3, 14, 53, 193, 54, 195},
	{70, 294, 3, 15, 47, 177, 48, 179},
	{65, 279, 4, 16, 41, 160, 41, 163},
	{60, 263, 4, 17, 35, 144, 35, 146},
	{55, 247, 4, 18, 29, 128, 29, 130},
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
