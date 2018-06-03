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

#ifndef _MT_PBM_
#define _MT_PBM_

/* #include <cust_pmic.h> */
#include <mach/mtk_pmic.h>

#ifdef DISABLE_DLPT_FEATURE
#define DISABLE_PBM_FEATURE
#endif

#define MD1_MAX_PW	3000	/* mW */
#define MD3_MAX_PW	2500	/* mW */
#define POWER_FLASH	3500	/* mW */
#define GUARDING_PATTERN	0

struct pbm {
	u8 feature_en;
	u8 pbm_drv_done;
	u32 hpf_en;
};

struct hpf {
	bool switch_md1;
	bool switch_md3;
	bool switch_gpu;
	bool switch_flash;

	bool md1_ccci_ready;
	bool md3_ccci_ready;

	int cpu_volt;
	int gpu_volt;
	int cpu_num;

	unsigned long loading_leakage;
	unsigned long loading_dlpt;
	unsigned long loading_md1;
	unsigned long loading_md3;
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
	KR_DLPT,		/* 0 */
	KR_MD1,			/* 1 */
	KR_MD3,			/* 2 */
	KR_CPU,			/* 3 */
	KR_GPU,			/* 4 */
	KR_FLASH		/* 5 */
};

#define MD_POWER_METER_ENABLE 1
/* #define TEST_MD_POWER */

#define SECTION_LEN	0xFFFFFFFF	/* total 4 byte, 6 section =  11 11111 11111 11111 11111 11111 11111 */
#define SECTION_VALUE	0x1F		/* each section is 0x1F = bit(11111) */

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
	CAT6_CA_DATALINK,	/* 0 */
	NON_CA_DATALINK,	/* 1 */
	PAGING,			/* 2 */
	POSITION,		/* 3 */
	CELL_SEARCH,		/* 4 */
	CELL_MANAGEMENT,	/* 5 */
	TALKING_2G,		/* 6 */
	DATALINK_2G,		/* 7 */
	TALKING_3G,		/* 8 */
	DATALINK_3G,		/* 9 */
	SCENARIO_NUM		/* 10 */
};

enum share_mem_mapping {	/* each of 4 byte */
	DBM_2G_TABLE = 0,
	DBM_3G_TABLE = 1,
	DBM_4G_TABLE = 2,
	DBM_TDD_TABLE = 3,
	DBM_C2K_TABLE = 4,
	SECTION_LEVLE_2G = 5,
	SECTION_LEVLE_3G = 6,
	SECTION_LEVLE_4G = 7,
	SECTION_LEVLE_TDD = 8,
	SECTION_LEVLE_C2K = 9,
	SHARE_MEM_BLOCK_NUM
};

/*
 * MD1/MD3 Section level (can't more than SECTION_VALUE)
 */
enum md1_section_level_tbl_2g {
	VAL_MD1_2G_SECTION_1 = 31,
	VAL_MD1_2G_SECTION_2 = 29,
	VAL_MD1_2G_SECTION_3 = 27,
	VAL_MD1_2G_SECTION_4 = 23,
	VAL_MD1_2G_SECTION_5 = 17,
	VAL_MD1_2G_SECTION_6 = 0
};

enum md1_section_level_tbl_3g {
	VAL_MD1_3G_SECTION_1 = 21,
	VAL_MD1_3G_SECTION_2 = 19,
	VAL_MD1_3G_SECTION_3 = 18,
	VAL_MD1_3G_SECTION_4 = 16,
	VAL_MD1_3G_SECTION_5 = 13,
	VAL_MD1_3G_SECTION_6 = 0
};

enum md1_section_level_tbl_4g {
	VAL_MD1_4G_SECTION_1 = 21,
	VAL_MD1_4G_SECTION_2 = 19,
	VAL_MD1_4G_SECTION_3 = 18,
	VAL_MD1_4G_SECTION_4 = 16,
	VAL_MD1_4G_SECTION_5 = 13,
	VAL_MD1_4G_SECTION_6 = 0
};

enum md1_section_level_tbl_tdd {
	VAL_MD1_TDD_SECTION_1 = 21,
	VAL_MD1_TDD_SECTION_2 = 19,
	VAL_MD1_TDD_SECTION_3 = 18,
	VAL_MD1_TDD_SECTION_4 = 16,
	VAL_MD1_TDD_SECTION_5 = 13,
	VAL_MD1_TDD_SECTION_6 = 0
};

enum md3_section_level_tbl {
	VAL_MD3_SECTION_1 = 24,
	VAL_MD3_SECTION_2 = 23,
	VAL_MD3_SECTION_3 = 21,
	VAL_MD3_SECTION_4 = 19,
	VAL_MD3_SECTION_5 = 14,
	VAL_MD3_SECTION_6 = 0
};

/*
 * MD1/MD3 Scenario power
 */
enum md1_scenario_pwr_tbl {
	PW_CAT6_CA_DATALINK = 870,
	PW_NON_CA_DATALINK = 650,
	PW_PAGING = 30,
	PW_POSITION = 870,		/* same as PW_CAT6_CA_DATALINK */
	PW_CELL_SEARCH = 0,		/* no use */
	PW_CELL_MANAGEMENT = 0,		/* no use */
	PW_TALKING_2G = 200,
	PW_DATALINK_2G = 200,
	PW_TALKING_3G = 320,
	PW_DATALINK_3G = 450
};

enum md3_scenario_pwr_tbl {
	PW_MD3 = 500
};

/*
 * MD1/MD3 PA power
 */
enum md1_pa_pwr_tbl_2g {
	PW_MD1_PA_2G_SECTION_1 = 688,
	PW_MD1_PA_2G_SECTION_2 = 440,
	PW_MD1_PA_2G_SECTION_3 = 353,
	PW_MD1_PA_2G_SECTION_4 = 284,
	PW_MD1_PA_2G_SECTION_5 = 184,
	PW_MD1_PA_2G_SECTION_6 = 99
};

enum md1_pa_pwr_tbl_3g {
	PW_MD1_PA_3G_SECTION_1 = 1965,
	PW_MD1_PA_3G_SECTION_2 = 1557,
	PW_MD1_PA_3G_SECTION_3 = 1022,
	PW_MD1_PA_3G_SECTION_4 = 914,
	PW_MD1_PA_3G_SECTION_5 = 553,
	PW_MD1_PA_3G_SECTION_6 = 294
};

enum md1_pa_pwr_tbl_4g {
	PW_MD1_PA_4G_SECTION_1 = 1965,
	PW_MD1_PA_4G_SECTION_2 = 1557,
	PW_MD1_PA_4G_SECTION_3 = 1022,
	PW_MD1_PA_4G_SECTION_4 = 914,
	PW_MD1_PA_4G_SECTION_5 = 553,
	PW_MD1_PA_4G_SECTION_6 = 294
};

enum md3_pa_pwr_tbl {
	PW_MD3_PA_SECTION_1 = 1956,
	PW_MD3_PA_SECTION_2 = 1759,
	PW_MD3_PA_SECTION_3 = 1257,
	PW_MD3_PA_SECTION_4 = 907,
	PW_MD3_PA_SECTION_5 = 554,
	PW_MD3_PA_SECTION_6 = 226
};

/*
 * MD1/MD3 RF power
 */
enum md1_rf_power {
	PW_MD1_RF_SECTION_1 = 512,
	PW_MD1_RF_SECTION_2 = 256
};

enum md3_rf_power {
	PW_MD3_RF_SECTION_1 = 280,
	PW_MD3_RF_SECTION_2 = 140
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
