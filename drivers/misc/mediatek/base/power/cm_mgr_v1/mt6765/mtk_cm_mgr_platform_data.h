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
#endif

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
#endif

#ifndef PROC_ENTRY
#define PROC_ENTRY(name)	{__stringify(name), &name ## _proc_fops}
#endif

struct cm_mgr_met_data {
	unsigned int cm_mgr_power[14];
	unsigned int cm_mgr_count[4];
	unsigned int cm_mgr_opp[6];
	unsigned int cm_mgr_loading[12];
	unsigned int cm_mgr_ratio[12];
	unsigned int cm_mgr_bw;
	unsigned int cm_mgr_valid;
};

static int light_load_cps = 1000;
static int cm_mgr_loop_count;
static int cm_mgr_loop;
static int total_bw_value;
static int cpu_power_ratio_up[CM_MGR_EMI_OPP] = {100, 100};
static int cpu_power_ratio_down[CM_MGR_EMI_OPP] = {100, 100};
int vcore_power_ratio_up[CM_MGR_EMI_OPP] = {100, 100};
int vcore_power_ratio_down[CM_MGR_EMI_OPP] = {100, 100};
static int debounce_times_up_adb[CM_MGR_EMI_OPP] = {0, 3};
static int debounce_times_down_adb[CM_MGR_EMI_OPP] = {0, 3};
static int debounce_times_reset_adb;
static int update;
static int update_v2f_table = 1;
static int cm_mgr_opp_enable = 1;
int cm_mgr_enable = 1;
#ifdef USE_TIMER_CHECK
int cm_mgr_timer_enable = 1;
#endif /* USE_TIMER_CHECK */
int cm_mgr_disable_fb = 1;
int cm_mgr_blank_status;

static unsigned int vcore_power_gain_0[][VCORE_ARRAY_SIZE] = {
	{64, 165},
	{518, 545},
	{488, 653},
	{540, 727},
	{653, 795},
	{770, 856},
	{833, 930},
	{852, 1005},
	{882, 1043},
	{912, 1081},
	{943, 1118},
	{973, 1156},
	{1003, 1194},
	{1003, 1214},
	{1003, 1234},
	{1003, 1253},
	{1003, 1273},
	{1003, 1293},
	{1003, 1313},
	{1003, 1333},
	{1003, 1353},
};

static unsigned int vcore_power_gain_1[][VCORE_ARRAY_SIZE] = {
	{19, 45},
	{122, 396},
	{192, 296},
	{238, 302},
	{334, 319},
	{375, 395},
	{379, 454},
	{372, 480},
	{380, 502},
	{388, 525},
	{396, 547},
	{404, 569},
	{412, 591},
	{412, 380},
	{412, 384},
	{412, 389},
	{412, 393},
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
	case 1:
		return VCORE_POWER_ARRAY_SIZE(1);
	}

	pr_info("#@# %s(%d) warning value %d\n", __func__, __LINE__, idx);
	return 0;
};

static unsigned int *vcore_power_gain_ptr(int idx)
{
	switch (idx) {
	case 0:
		return VCORE_POWER_GAIN_PTR(0);
	case 1:
		return VCORE_POWER_GAIN_PTR(1);
	}

	pr_info("#@# %s(%d) warning value %d\n", __func__, __LINE__, idx);
	return NULL;
};

static unsigned int *vcore_power_gain = VCORE_POWER_GAIN_PTR(0);
#define vcore_power_gain(p, i, j) (*(p + (i) * VCORE_ARRAY_SIZE + (j)))

static unsigned int _v2f_all[][CM_MGR_CPU_CLUSTER] = {
	/* SB */
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

#ifndef ATF_SECURE_SMC
static unsigned int cpu_power_gain_UpLow0[][CM_MGR_CPU_ARRAY_SIZE] = {
	{2, 2, 2, 1},
	{5, 4, 4, 3},
	{91, 5, 6, 4},
	{89, 7, 86, 5},
	{86, 84, 83, 7},
	{84, 81, 80, 78},
	{82, 78, 77, 74},
	{79, 74, 74, 71},
	{77, 71, 71, 67},
	{74, 68, 68, 64},
	{72, 65, 65, 60},
	{69, 62, 62, 56},
	{67, 59, 59, 53},
	{64, 55, 56, 49},
	{62, 52, 53, 45},
	{60, 49, 50, 42},
	{57, 46, 46, 38},
	{55, 43, 43, 35},
	{52, 40, 40, 31},
	{50, 37, 37, 27},
};

static unsigned int cpu_power_gain_DownLow0[][CM_MGR_CPU_ARRAY_SIZE] = {
	{4, 3, 5, 4},
	{112, 5, 107, 5},
	{110, 105, 103, 7},
	{108, 102, 100, 95},
	{106, 99, 97, 91},
	{104, 96, 93, 87},
	{102, 93, 90, 83},
	{100, 90, 86, 79},
	{98, 87, 83, 75},
	{96, 84, 80, 71},
	{93, 81, 76, 67},
	{91, 78, 73, 64},
	{89, 76, 70, 60},
	{87, 73, 66, 56},
	{85, 70, 63, 52},
	{83, 67, 60, 48},
	{81, 64, 56, 44},
	{79, 61, 53, 40},
	{77, 58, 50, 37},
	{75, 55, 0, 0},
};

static unsigned int cpu_power_gain_UpHigh0[][CM_MGR_CPU_ARRAY_SIZE] = {
	{2, 2, 2, 1},
	{5, 4, 4, 3},
	{7, 5, 6, 4},
	{10, 7, 7, 5},
	{12, 9, 9, 7},
	{15, 11, 11, 8},
	{122, 13, 13, 10},
	{117, 15, 15, 11},
	{111, 16, 106, 12},
	{106, 18, 99, 14},
	{100, 103, 93, 15},
	{94, 96, 87, 16},
	{89, 89, 81, 18},
	{83, 81, 75, 19},
	{78, 74, 68, 67},
	{72, 66, 62, 59},
	{66, 59, 56, 51},
	{61, 51, 50, 43},
	{55, 44, 43, 35},
	{50, 37, 37, 27},
};

static unsigned int cpu_power_gain_DownHigh0[][CM_MGR_CPU_ARRAY_SIZE] = {
	{4, 3, 5, 4},
	{7, 5, 7, 5},
	{11, 8, 10, 7},
	{15, 11, 12, 9},
	{153, 14, 15, 11},
	{148, 16, 134, 13},
	{143, 153, 128, 15},
	{137, 146, 121, 16},
	{132, 138, 115, 18},
	{127, 131, 108, 113},
	{122, 123, 102, 105},
	{116, 115, 95, 96},
	{111, 108, 89, 88},
	{106, 100, 82, 79},
	{101, 93, 76, 71},
	{96, 85, 69, 62},
	{90, 78, 63, 54},
	{85, 70, 56, 45},
	{80, 62, 50, 37},
	{75, 55, 0, 0},
};

static unsigned int cpu_power_gain_UpLow1[][CM_MGR_CPU_ARRAY_SIZE] = {
	{2, 1, 1, 1},
	{4, 3, 2, 1},
	{6, 4, 2, 2},
	{86, 5, 3, 2},
	{83, 7, 4, 3},
	{80, 78, 5, 4},
	{77, 74, 6, 4},
	{74, 71, 66, 5},
	{71, 67, 62, 5},
	{68, 64, 58, 6},
	{65, 60, 54, 7},
	{62, 56, 49, 7},
	{59, 53, 45, 43},
	{56, 49, 41, 38},
	{53, 45, 37, 34},
	{50, 42, 33, 30},
	{46, 38, 29, 25},
	{43, 35, 25, 21},
	{40, 31, 21, 17},
	{37, 27, 17, 12},
};

static unsigned int cpu_power_gain_DownLow1[][CM_MGR_CPU_ARRAY_SIZE] = {
	{2, 2, 2, 1},
	{5, 4, 3, 2},
	{107, 5, 4, 3},
	{103, 7, 5, 3},
	{100, 95, 6, 4},
	{97, 91, 82, 5},
	{93, 87, 77, 5},
	{90, 83, 73, 6},
	{86, 79, 68, 7},
	{83, 75, 63, 8},
	{80, 71, 58, 54},
	{76, 67, 53, 49},
	{73, 64, 48, 44},
	{70, 60, 43, 39},
	{66, 56, 38, 34},
	{63, 52, 33, 29},
	{60, 48, 28, 24},
	{56, 44, 24, 19},
	{53, 40, 19, 14},
	{50, 37, 0, 0},
};

static unsigned int cpu_power_gain_UpHigh1[][CM_MGR_CPU_ARRAY_SIZE] = {
	{2, 1, 1, 1},
	{4, 3, 2, 1},
	{6, 4, 2, 2},
	{7, 5, 3, 2},
	{9, 7, 4, 3},
	{11, 8, 5, 4},
	{13, 10, 6, 4},
	{15, 11, 7, 5},
	{106, 12, 7, 5},
	{99, 14, 8, 6},
	{93, 15, 9, 7},
	{87, 16, 10, 7},
	{81, 18, 11, 8},
	{75, 19, 12, 9},
	{68, 67, 12, 9},
	{62, 59, 13, 10},
	{56, 51, 14, 10},
	{50, 43, 15, 11},
	{43, 35, 24, 12},
	{37, 27, 17, 12},
};

static unsigned int cpu_power_gain_DownHigh1[][CM_MGR_CPU_ARRAY_SIZE] = {
	{2, 2, 2, 1},
	{5, 4, 3, 2},
	{7, 5, 4, 3},
	{10, 7, 5, 3},
	{12, 9, 6, 4},
	{15, 11, 7, 5},
	{134, 13, 7, 5},
	{128, 15, 8, 6},
	{121, 16, 9, 7},
	{115, 18, 10, 8},
	{108, 113, 11, 8},
	{102, 105, 12, 9},
	{95, 96, 13, 10},
	{89, 88, 14, 10},
	{82, 79, 15, 11},
	{76, 71, 43, 12},
	{69, 62, 35, 12},
	{63, 54, 27, 13},
	{56, 45, 19, 14},
	{50, 37, 0, 0},
};

#define cpu_power_gain(p, i, j) (*(p + (i) * CM_MGR_CPU_ARRAY_SIZE + (j)))
#define CPU_POWER_GAIN(a, b, c) \
	(&cpu_power_gain_##a##b##c[0][0])

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
			case 1:
				cpu_power_gain_up =
					CPU_POWER_GAIN(Up, Low, 1);
				cpu_power_gain_down =
					CPU_POWER_GAIN(Down, Low, 1);
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
			case 1:
				cpu_power_gain_up =
					CPU_POWER_GAIN(Up, High, 1);
				cpu_power_gain_down =
					CPU_POWER_GAIN(Down, High, 1);
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
			case 1:
				cpu_power_gain_up =
					CPU_POWER_GAIN(Up, Low, 1);
				cpu_power_gain_down =
					CPU_POWER_GAIN(Down, Low, 1);
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
			case 1:
				cpu_power_gain_up =
					CPU_POWER_GAIN(Up, High, 1);
				cpu_power_gain_down =
					CPU_POWER_GAIN(Down, High, 1);
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
#endif

#endif	/* __MTK_CM_MGR_PLATFORM_DATA_H__ */
