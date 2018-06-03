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

#ifndef __MTK_CM_MGR_MT6771_DATA_H__
#define __MTK_CM_MGR_MT6771_DATA_H__

#define PROC_FOPS_RW(name)					\
	static int name ## _proc_open(struct inode *inode,	\
		struct file *file)				\
	{							\
		return single_open(file, name ## _proc_show,	\
			PDE_DATA(inode));			\
	}							\
	static const struct file_operations name ## _proc_fops = {	\
		.owner		  = THIS_MODULE,				\
		.open		   = name ## _proc_open,			\
		.read		   = seq_read,				\
		.llseek		 = seq_lseek,				\
		.release		= single_release,			\
		.write		  = name ## _proc_write,			\
	}

#define PROC_FOPS_RO(name)					\
	static int name ## _proc_open(struct inode *inode,	\
		struct file *file)				\
	{							\
		return single_open(file, name ## _proc_show,	\
			PDE_DATA(inode));			\
	}							\
	static const struct file_operations name ## _proc_fops = {	\
		.owner		  = THIS_MODULE,				\
		.open		   = name ## _proc_open,			\
		.read		   = seq_read,				\
		.llseek		 = seq_lseek,				\
		.release		= single_release,			\
	}

#define PROC_ENTRY(name)	{__stringify(name), &name ## _proc_fops}

struct cm_mgr_met_data {
	unsigned int cm_mgr_power[14];
	unsigned int cm_mgr_count[4];
	unsigned int cm_mgr_opp[6];
	unsigned int cm_mgr_loading[2];
	unsigned int cm_mgr_ratio[10];
	unsigned int cm_mgr_bw;
	unsigned int cm_mgr_valid;
};

static int light_load_cps = 1000;
static int cm_mgr_loop_count;
static int cm_mgr_loop;
static int total_bw_value;
static int cpu_power_ratio_up[CM_MGR_EMI_OPP] = {100, 100};
static int cpu_power_ratio_down[CM_MGR_EMI_OPP] = {100, 100};
static int vcore_power_ratio_up[CM_MGR_EMI_OPP] = {80, 100};
static int vcore_power_ratio_down[CM_MGR_EMI_OPP] = {80, 100};
static int debounce_times_up_adb[CM_MGR_EMI_OPP] = {0, 3};
static int debounce_times_down_adb[CM_MGR_EMI_OPP] = {0, 3};
static int debounce_times_reset_adb;
static int update;
static int update_v2f_table = 1;
static int cm_mgr_opp_enable = 1;
static int cm_mgr_enable;
static int is_lp3;

static unsigned int vcore_power_gain_lp4[][VCORE_ARRAY_SIZE] = {
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

static unsigned int vcore_power_gain_lp3[][VCORE_ARRAY_SIZE] = {
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

#define VCORE_POWER_ARRAY_SIZE \
	(sizeof(vcore_power_gain_lp4) / sizeof(unsigned int) / VCORE_ARRAY_SIZE)

static unsigned int *vcore_power_gain = &vcore_power_gain_lp4[0][0];
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
static unsigned int cpu_power_gain_up_low_1[][CM_MGR_CPU_ARRAY_SIZE] = {
	{2, 17, 2, 13},
	{4, 35, 3, 26},
	{7, 52, 5, 39},
	{9, 69, 7, 52},
	{68, 247, 8, 65},
	{66, 254, 63, 228},
	{65, 260, 61, 230},
	{63, 267, 59, 232},
	{62, 274, 57, 235},
	{60, 280, 55, 237},
	{59, 287, 53, 239},
	{57, 293, 50, 241},
	{56, 300, 48, 244},
	{54, 306, 46, 246},
	{53, 313, 44, 248},
	{51, 319, 42, 250},
	{50, 326, 40, 253},
	{48, 333, 38, 255},
	{46, 339, 36, 257},
	{45, 346, 34, 259},
};

static unsigned int cpu_power_gain_down_low_1[][CM_MGR_CPU_ARRAY_SIZE] = {
	{3, 26, 2, 17},
	{7, 52, 4, 35},
	{80, 276, 7, 52},
	{79, 290, 9, 69},
	{79, 305, 73, 261},
	{78, 319, 71, 267},
	{77, 333, 69, 273},
	{76, 347, 67, 278},
	{76, 362, 65, 284},
	{75, 376, 64, 289},
	{74, 390, 62, 295},
	{73, 404, 60, 301},
	{73, 419, 58, 306},
	{72, 433, 56, 312},
	{71, 447, 54, 318},
	{70, 461, 52, 323},
	{70, 476, 50, 329},
	{69, 490, 49, 334},
	{68, 504, 47, 340},
	{67, 519, 45, 346},
};

static unsigned int cpu_power_gain_up_high_1[][CM_MGR_CPU_ARRAY_SIZE] = {
	{2, 17, 2, 13},
	{4, 35, 3, 26},
	{7, 52, 5, 39},
	{9, 69, 7, 52},
	{11, 86, 8, 65},
	{13, 104, 10, 78},
	{16, 121, 12, 91},
	{95, 358, 13, 104},
	{91, 357, 15, 117},
	{87, 356, 81, 312},
	{83, 355, 77, 307},
	{79, 354, 72, 302},
	{74, 353, 67, 296},
	{70, 352, 62, 291},
	{66, 351, 57, 286},
	{62, 350, 53, 281},
	{57, 349, 48, 275},
	{53, 348, 43, 270},
	{49, 347, 38, 265},
	{45, 346, 34, 259},
};

static unsigned int cpu_power_gain_down_high_1[][CM_MGR_CPU_ARRAY_SIZE] = {
	{3, 26, 2, 17},
	{7, 52, 4, 35},
	{10, 78, 7, 52},
	{13, 104, 9, 69},
	{118, 418, 11, 86},
	{115, 424, 13, 104},
	{112, 431, 16, 121},
	{108, 438, 99, 369},
	{105, 445, 95, 367},
	{101, 451, 90, 365},
	{98, 458, 86, 363},
	{95, 465, 81, 361},
	{91, 471, 77, 359},
	{88, 478, 72, 357},
	{84, 485, 68, 355},
	{81, 492, 63, 353},
	{78, 498, 58, 351},
	{74, 505, 54, 350},
	{71, 512, 49, 348},
	{67, 519, 45, 346},
};

static unsigned int cpu_power_gain_up_low_2[][CM_MGR_CPU_ARRAY_SIZE] = {
	{2, 17, 2, 13},
	{4, 35, 3, 26},
	{7, 52, 5, 39},
	{9, 69, 7, 52},
	{68, 247, 8, 65},
	{66, 254, 63, 228},
	{65, 260, 61, 230},
	{63, 267, 59, 232},
	{62, 274, 57, 235},
	{60, 280, 55, 237},
	{59, 287, 53, 239},
	{57, 293, 50, 241},
	{56, 300, 48, 244},
	{54, 306, 46, 246},
	{53, 313, 44, 248},
	{51, 319, 42, 250},
	{50, 326, 40, 253},
	{48, 333, 38, 255},
	{46, 339, 36, 257},
	{45, 346, 34, 259},
};

static unsigned int cpu_power_gain_down_low_2[][CM_MGR_CPU_ARRAY_SIZE] = {
	{3, 26, 2, 17},
	{7, 52, 4, 35},
	{80, 276, 7, 52},
	{79, 290, 9, 69},
	{79, 305, 73, 261},
	{78, 319, 71, 267},
	{77, 333, 69, 273},
	{76, 347, 67, 278},
	{76, 362, 65, 284},
	{75, 376, 64, 289},
	{74, 390, 62, 295},
	{73, 404, 60, 301},
	{73, 419, 58, 306},
	{72, 433, 56, 312},
	{71, 447, 54, 318},
	{70, 461, 52, 323},
	{70, 476, 50, 329},
	{69, 490, 49, 334},
	{68, 504, 47, 340},
	{67, 519, 45, 346},
};

static unsigned int cpu_power_gain_up_high_2[][CM_MGR_CPU_ARRAY_SIZE] = {
	{2, 17, 2, 13},
	{4, 35, 3, 26},
	{7, 52, 5, 39},
	{9, 69, 7, 52},
	{11, 86, 8, 65},
	{13, 104, 10, 78},
	{16, 121, 12, 91},
	{95, 358, 13, 104},
	{91, 357, 15, 117},
	{87, 356, 81, 312},
	{83, 355, 77, 307},
	{79, 354, 72, 302},
	{74, 353, 67, 296},
	{70, 352, 62, 291},
	{66, 351, 57, 286},
	{62, 350, 53, 281},
	{57, 349, 48, 275},
	{53, 348, 43, 270},
	{49, 347, 38, 265},
	{45, 346, 34, 259},
};

static unsigned int cpu_power_gain_down_high_2[][CM_MGR_CPU_ARRAY_SIZE] = {
	{3, 26, 2, 17},
	{7, 52, 4, 35},
	{10, 78, 7, 52},
	{13, 104, 9, 69},
	{118, 418, 11, 86},
	{115, 424, 13, 104},
	{112, 431, 16, 121},
	{108, 438, 99, 369},
	{105, 445, 95, 367},
	{101, 451, 90, 365},
	{98, 458, 86, 363},
	{95, 465, 81, 361},
	{91, 471, 77, 359},
	{88, 478, 72, 357},
	{84, 485, 68, 355},
	{81, 492, 63, 353},
	{78, 498, 58, 351},
	{74, 505, 54, 350},
	{71, 512, 49, 348},
	{67, 519, 45, 346},
};

static unsigned int *cpu_power_gain_up = &cpu_power_gain_up_high_1[0][0];
static unsigned int *cpu_power_gain_down = &cpu_power_gain_down_high_1[0][0];
#define cpu_power_gain(p, i, j) (*(p + (i) * CM_MGR_CPU_ARRAY_SIZE + (j)))

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
#endif

#endif	/* __MTK_CM_MGR_MT6771_DATA_H__ */
