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

#ifndef _MT_PBM_
#define _MT_PBM_

/* #include <cust_pmic.h> */
#include <mach/mtk_pmic.h>

#ifdef DISABLE_DLPT_FEATURE
#define DISABLE_PBM_FEATURE
#endif

#define MD1_MAX_PW 4000  /* mW */
#define POWER_FLASH 3500 /* mW */
#define GUARDING_PATTERN 0

#define _BIT_(_bit_)		(unsigned int)(1 << (_bit_))
#define _BITMASK_(_bits_)	(((unsigned int) -1 >> (31 - ((1) ?	\
				_bits_))) & ~((1U << ((0) ? _bits_)) - 1))

struct pbm {
	u8 feature_en;
	u8 pbm_drv_done;
	u32 hpf_en;
};

struct hpf {
	bool switch_md1;
	bool switch_gpu;
	bool switch_flash;

	bool md1_ccci_ready;

	int cpu_volt;
	int gpu_volt;
	int cpu_num;

	unsigned long loading_leakage;
	unsigned long loading_dlpt;
	unsigned long loading_md1;
	unsigned long loading_cpu;
	unsigned long loading_gpu;
	unsigned long loading_flash;
};

struct mrp {
	bool switch_md;
	bool switch_gpu;
	bool switch_flash;

	int cpu_volt;
	int gpu_volt;
	int cpu_num;

	unsigned long loading_dlpt;
	unsigned long loading_cpu;
	unsigned long loading_gpu;
};

enum pbm_kicker {
	KR_DLPT, /* 0 */
	KR_MD1,  /* 1 */
	KR_MD3,  /* 2 */
	KR_CPU,  /* 3 */
	KR_GPU,  /* 4 */
	KR_FLASH /* 5 */
};

#define MD_POWER_METER_ENABLE 1
/*#define TEST_MD_POWER*/

/* total 4 byte, 6 section =  11 11111 11111 11111 11111 11111 11111 */
#define SECTION_LEN 0xFFFFFFFF
#define SECTION_VALUE 0x1F /* each section is 0x1F = bit(11111) */

enum section_level_tbl {
	BIT_SECTION_1 = 0,
	BIT_SECTION_2 = 5,
	BIT_SECTION_3 = 10,
	BIT_SECTION_4 = 15,
	BIT_SECTION_5 = 20,
	BIT_SECTION_6 = 25,
	SECTION_NUM = 6
};

enum md1_scenario {
	S_STANDBY = 0,
	S_2G_TALKING_OR_DATALINK,
	S_3G_TALKING,
	S_3G_DATALINK,
	S_4G_DL_1CC,
	S_4G_DL_2CC,
	SCENARIO_NUM
};

enum share_mem_mapping { /* each of 4 byte */
			 DBM_2G_TABLE = 0,
			 DBM_3G_TABLE,
			 DBM_4G_TABLE,
			 DBM_4G_1_TABLE,
			 DBM_4G_2_TABLE,
			 DBM_4G_3_TABLE,
			 DBM_4G_4_TABLE,
			 DBM_4G_5_TABLE,
			 DBM_4G_6_TABLE,
			 DBM_4G_7_TABLE,
			 DBM_4G_8_TABLE,
			 DBM_4G_9_TABLE,
			 DBM_4G_10_TABLE,
			 DBM_4G_11_TABLE,
			 DBM_4G_12_TABLE,
			 DBM_TDD_TABLE,
			 DBM_C2K_1_TABLE,
			 DBM_C2K_2_TABLE,
			 DBM_C2K_3_TABLE,
			 DBM_C2K_4_TABLE,
			 SECTION_LEVLE_2G,
			 SECTION_LEVLE_3G,
			 SECTION_LEVLE_4G,
			 SECTION_1_LEVLE_4G,
			 SECTION_2_LEVLE_4G,
			 SECTION_3_LEVLE_4G,
			 SECTION_4_LEVLE_4G,
			 SECTION_5_LEVLE_4G,
			 SECTION_6_LEVLE_4G,
			 SECTION_7_LEVLE_4G,
			 SECTION_8_LEVLE_4G,
			 SECTION_9_LEVLE_4G,
			 SECTION_10_LEVLE_4G,
			 SECTION_11_LEVLE_4G,
			 SECTION_12_LEVLE_4G,
			 SECTION_LEVLE_TDD,
			 SECTION_1_LEVLE_C2K,
			 SECTION_2_LEVLE_C2K,
			 SECTION_3_LEVLE_C2K,
			 SECTION_4_LEVLE_C2K,
			 SHARE_MEM_BLOCK_NUM
};

/*
 * MD1 Section level (can't more than SECTION_VALUE)
 */
/* Each section has only 5 bits. The range is from 0 to 31 */
enum md1_section_level_tbl_2g {
	VAL_MD1_2G_SECTION_1 = 31,
	VAL_MD1_2G_SECTION_2 = 29,
	VAL_MD1_2G_SECTION_3 = 25,
	VAL_MD1_2G_SECTION_4 = 19,
	VAL_MD1_2G_SECTION_5 = 11,
	VAL_MD1_2G_SECTION_6 = 0
};

enum md1_section_level_tbl_3g {
	VAL_MD1_3G_SECTION_1 = 22,
	VAL_MD1_3G_SECTION_2 = 20,
	VAL_MD1_3G_SECTION_3 = 17,
	VAL_MD1_3G_SECTION_4 = 16,
	VAL_MD1_3G_SECTION_5 = 11,
	VAL_MD1_3G_SECTION_6 = 0
};

enum md1_section_level_tbl_4g_upL1 {
	VAL_MD1_4G_upL1_SECTION_1 = 21,
	VAL_MD1_4G_upL1_SECTION_2 = 19,
	VAL_MD1_4G_upL1_SECTION_3 = 17,
	VAL_MD1_4G_upL1_SECTION_4 = 16,
	VAL_MD1_4G_upL1_SECTION_5 = 11,
	VAL_MD1_4G_upL1_SECTION_6 = 0
};

enum md1_section_level_tbl_4g_upL2 {
	VAL_MD1_4G_upL2_SECTION_1 = 21,
	VAL_MD1_4G_upL2_SECTION_2 = 19,
	VAL_MD1_4G_upL2_SECTION_3 = 17,
	VAL_MD1_4G_upL2_SECTION_4 = 16,
	VAL_MD1_4G_upL2_SECTION_5 = 11,
	VAL_MD1_4G_upL2_SECTION_6 = 0
};

enum md1_section_level_tbl_tdd {
	VAL_MD1_TDD_SECTION_1 = 21,
	VAL_MD1_TDD_SECTION_2 = 19,
	VAL_MD1_TDD_SECTION_3 = 18,
	VAL_MD1_TDD_SECTION_4 = 16,
	VAL_MD1_TDD_SECTION_5 = 13,
	VAL_MD1_TDD_SECTION_6 = 0
};

enum md1_section_level_tbl_c2k {
	VAL_MD1_C2K_SECTION_1 = 23,
	VAL_MD1_C2K_SECTION_2 = 22,
	VAL_MD1_C2K_SECTION_3 = 20,
	VAL_MD1_C2K_SECTION_4 = 17,
	VAL_MD1_C2K_SECTION_5 = 16,
	VAL_MD1_C2K_SECTION_6 = 0
};

/*
 * MD1 Scenario power
 */
enum md1_scenario_pwr_tbl {
	PW_STANDBY = 2,
	PW_2G_TALKING_OR_DATALINK = 30,
	PW_3G_TALKING = 72,
	PW_3G_DATALINK = 180,
	PW_4G_DL_1CC = 171,
	PW_4G_DL_2CC = 320
};

/*
 * MD1 PA power
 */
enum md1_pa_pwr_tbl_2g {
	PW_MD1_PA_2G_SECTION_1 = 688,
	PW_MD1_PA_2G_SECTION_2 = 549,
	PW_MD1_PA_2G_SECTION_3 = 353,
	PW_MD1_PA_2G_SECTION_4 = 228,
	PW_MD1_PA_2G_SECTION_5 = 121,
	PW_MD1_PA_2G_SECTION_6 = 56
};

enum md1_pa_pwr_tbl_3g {
	PW_MD1_PA_3G_SECTION_1 = 1980,
	PW_MD1_PA_3G_SECTION_2 = 1391,
	PW_MD1_PA_3G_SECTION_3 = 888,
	PW_MD1_PA_3G_SECTION_4 = 633,
	PW_MD1_PA_3G_SECTION_5 = 394,
	PW_MD1_PA_3G_SECTION_6 = 192
};

enum md1_pa_pwr_tbl_4g_upL1 {
	PW_MD1_PA_4G_upL1_SECTION_1 = 1851,
	PW_MD1_PA_4G_upL1_SECTION_2 = 1397,
	PW_MD1_PA_4G_upL1_SECTION_3 = 931,
	PW_MD1_PA_4G_upL1_SECTION_4 = 745,
	PW_MD1_PA_4G_upL1_SECTION_5 = 423,
	PW_MD1_PA_4G_upL1_SECTION_6 = 202
};

enum md1_pa_pwr_tbl_4g_upL2 {
	PW_MD1_PA_4G_upL2_SECTION_1 = 1851,
	PW_MD1_PA_4G_upL2_SECTION_2 = 1397,
	PW_MD1_PA_4G_upL2_SECTION_3 = 931,
	PW_MD1_PA_4G_upL2_SECTION_4 = 745,
	PW_MD1_PA_4G_upL2_SECTION_5 = 423,
	PW_MD1_PA_4G_upL2_SECTION_6 = 202
};

enum md1_pa_pwr_tbl_c2k {
	PW_MD1_PA_C2K_SECTION_1 = 2084,
	PW_MD1_PA_C2K_SECTION_2 = 1548,
	PW_MD1_PA_C2K_SECTION_3 = 1146,
	PW_MD1_PA_C2K_SECTION_4 = 767,
	PW_MD1_PA_C2K_SECTION_5 = 535,
	PW_MD1_PA_C2K_SECTION_6 = 329
};

/*
 * MD1 RF power
 */
enum md1_rf_pwr_tbl_2g {
	PW_MD1_RF_2G_SECTION_1 = 32,
	PW_MD1_RF_2G_SECTION_2 = 32,
	PW_MD1_RF_2G_SECTION_3 = 32,
	PW_MD1_RF_2G_SECTION_4 = 32,
	PW_MD1_RF_2G_SECTION_5 = 32,
	PW_MD1_RF_2G_SECTION_6 = 32
};

enum md1_rf_pwr_tbl_3g {
	PW_MD1_RF_3G_SECTION_1 = 99,
	PW_MD1_RF_3G_SECTION_2 = 99,
	PW_MD1_RF_3G_SECTION_3 = 99,
	PW_MD1_RF_3G_SECTION_4 = 99,
	PW_MD1_RF_3G_SECTION_5 = 99,
	PW_MD1_RF_3G_SECTION_6 = 99
};

enum md1_rf_pwr_tbl_4g_upL1 {
	PW_MD1_RF_4G_upL1_SECTION_1 = 201,
	PW_MD1_RF_4G_upL1_SECTION_2 = 201,
	PW_MD1_RF_4G_upL1_SECTION_3 = 201,
	PW_MD1_RF_4G_upL1_SECTION_4 = 201,
	PW_MD1_RF_4G_upL1_SECTION_5 = 201,
	PW_MD1_RF_4G_upL1_SECTION_6 = 201
};

enum md1_rf_pwr_tbl_4g_upL2 {
	PW_MD1_RF_4G_upL2_SECTION_1 = 201,
	PW_MD1_RF_4G_upL2_SECTION_2 = 201,
	PW_MD1_RF_4G_upL2_SECTION_3 = 201,
	PW_MD1_RF_4G_upL2_SECTION_4 = 201,
	PW_MD1_RF_4G_upL2_SECTION_5 = 201,
	PW_MD1_RF_4G_upL2_SECTION_6 = 201
};

enum md1_rf_pwr_tbl_c2k {
	PW_MD1_RF_C2K_SECTION_1 = 310,
	PW_MD1_RF_C2K_SECTION_2 = 310,
	PW_MD1_RF_C2K_SECTION_3 = 310,
	PW_MD1_RF_C2K_SECTION_4 = 310,
	PW_MD1_RF_C2K_SECTION_5 = 310,
	PW_MD1_RF_C2K_SECTION_6 = 310
};

extern void kicker_pbm_by_dlpt(unsigned int i_max);
extern void kicker_pbm_by_md(enum pbm_kicker kicker, bool status);
extern void kicker_pbm_by_cpu(unsigned int loading, int core, int voltage);
extern void kicker_pbm_by_gpu(bool status, unsigned int loading, int voltage);
extern void kicker_pbm_by_flash(bool status);

extern void init_md_section_level(enum pbm_kicker);

#ifndef DISABLE_PBM_FEATURE
extern int g_dlpt_stop;
#endif

#endif
