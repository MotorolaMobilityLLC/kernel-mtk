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

#ifndef __MTK_SWPM_PLATFORM_H__
#define __MTK_SWPM_PLATFORM_H__

#define MAX_RECORD_CNT                  (64)
#define USE_PMU                         (1)
#define USE_MDLA_PMU                    (1)

#ifdef USE_PMU
#define PMU_EVENT_INST_SPEC             (0x1B)
#define PMU_EVENT_L3D_CACHE_RD          (0xA0)
#define PMU_CNTENSET_EVT_CNT_L          (0x1) /* 1 event counter enabled */
#define PMU_CNTENSET_EVT_CNT_B          (0x3) /* 2 event counter enabled */
#define PMU_CNTENSET_C                  (0x1 << 31)
#define PMU_EVTYPER_INC                 (0x1 << 27)
#define PMU_PMCR_E                      (0x1)
#define OFF_PMEVCNTR_0                  (0x000)
#define OFF_PMEVCNTR_1                  (0x008)
#define OFF_PMEVCNTR_2                  (0x010)
#define OFF_PMEVCNTR_3                  (0x018)
#define OFF_PMEVCNTR_4                  (0x020)
#define OFF_PMEVCNTR_5                  (0x028)
#define OFF_PMCCNTR_L                   (0x0F8)
#define OFF_PMCCNTR_H                   (0x0FC)
#define OFF_PMEVTYPER0                  (0x400)
#define OFF_PMEVTYPER1                  (0x404)
#define OFF_PMEVTYPER2                  (0x408)
#define OFF_PMEVTYPER3                  (0x40C)
#define OFF_PMEVTYPER4                  (0x410)
#define OFF_PMEVTYPER5                  (0x414)
#define OFF_PMCNTENSET                  (0xC00)
#define OFF_PMCNTENCLR                  (0xC20)
#define OFF_PMCFGR                      (0xE00)
#define OFF_PMCR                        (0xE04)
#define OFF_PMLAR                       (0xFB0)
#define OFF_PMLSR                       (0xFB4)
#endif
#ifdef USE_MDLA_PMU
#define MDLA_PMU_BASE			(mdla_pmu_base)
#define MDLA_PMU_CFG			(MDLA_PMU_BASE + 0x0E00)
#define MDLA_PMU_PMU1_SEL		(MDLA_PMU_BASE + 0x0E10)
#define MDLA_PMU_PMU1_CNT		(MDLA_PMU_BASE + 0x0E14)
#define MDLA_PMU_PMU2_SEL		(MDLA_PMU_BASE + 0x0E20)
#define MDLA_PMU_PMU2_CNT		(MDLA_PMU_BASE + 0x0E24)
#define MDLA_PMU_PMU3_SEL		(MDLA_PMU_BASE + 0x0E30)
#define MDLA_PMU_PMU3_CNT		(MDLA_PMU_BASE + 0x0E34)
#define MDLA_PMU_PMU4_SEL		(MDLA_PMU_BASE + 0x0E40)
#define MDLA_PMU_PMU4_CNT		(MDLA_PMU_BASE + 0x0E44)
#define MDLA_PMU_PMU5_SEL		(MDLA_PMU_BASE + 0x0E50)
#define MDLA_PMU_PMU5_CNT		(MDLA_PMU_BASE + 0x0E54)
#define MDLA_PMU_PMU6_SEL		(MDLA_PMU_BASE + 0x0E60)
#define MDLA_PMU_PMU6_CNT		(MDLA_PMU_BASE + 0x0E64)
#define MDLA_PMU_PMU7_SEL		(MDLA_PMU_BASE + 0x0E70)
#define MDLA_PMU_PMU7_CNT		(MDLA_PMU_BASE + 0x0E74)
#define MDLA_PMU_PMU8_SEL		(MDLA_PMU_BASE + 0x0E80)
#define MDLA_PMU_PMU8_CNT		(MDLA_PMU_BASE + 0x0E84)
#endif

/* data shared w/ SSPM */
enum profile_point {
	MON_TIME,
	CALC_TIME,
	REC_TIME,
	TOTAL_TIME,

	NR_PROFILE_POINT
};

enum power_meter_type {
	CPU_POWER_METER,
	GPU_POWER_METER,
	CORE_POWER_METER,
	MEM_POWER_METER,
	VPU_POWER_METER,
	MDLA_POWER_METER,

	NR_POWER_METER
};

enum power_rail {
	VPROC12,
	VPROC11,
	VGPU,
	VCORE,
	VDRAM1,
	VIO18_DDR,
	VIO18_DRAM,
	VVPU,
	VMDLA,

	NR_POWER_RAIL
};

enum ddr_freq {
	DDR_400,
	DDR_600,
	DDR_800,
	DDR_1200,
	DDR_1600,
	DDR_1866,

	NR_DDR_FREQ
};

enum aphy_pwr_type {
	APHY_VCORE_0P7V,
	APHY_VDDQ_0P6V,
	APHY_VM_1P1V,
	APHY_VIO_1P8V,

	NR_APHY_PWR_TYPE
};

enum dram_pwr_type {
	DRAM_VDD1_1P8V,
	DRAM_VDD2_1P1V,
	DRAM_VDDQ_0P6V,

	NR_DRAM_PWR_TYPE
};

struct aphy_pwr {
	unsigned short bw[5];
	unsigned short coef[5];
};

/* unit: uW / V^2 */
struct aphy_pwr_data {
	struct aphy_pwr read_pwr[NR_DDR_FREQ];
	struct aphy_pwr write_pwr[NR_DDR_FREQ];
	unsigned short coef_idle[NR_DDR_FREQ];
};

/* unit: uA */
struct dram_pwr_conf {
	unsigned int i_dd0;
	unsigned int i_dd2p;
	unsigned int i_dd2n;
	unsigned int i_dd4r;
	unsigned int i_dd4w;
	unsigned int i_dd5;
	unsigned int i_dd6;
};

struct swpm_rec_data {
	/* 8 bytes */
	unsigned int cur_idx;
	unsigned int profile_enable;

	/* 4(int) * 64(rec_cnt) * 9 = 2304 bytes */
	unsigned int pwr[NR_POWER_RAIL][MAX_RECORD_CNT];

	/* 8(long) * 5(prof_pt) * 3 = 120 bytes */
	unsigned long long avg_latency[NR_PROFILE_POINT];
	unsigned long long max_latency[NR_PROFILE_POINT];
	unsigned long long prof_cnt[NR_PROFILE_POINT];

	/* 2(short) * 4(pwr_type) * 126 = 1008 bytes */
	struct aphy_pwr_data aphy_pwr_tbl[NR_APHY_PWR_TYPE];

	/* 4(int) * 3(pwr_type) * 7 = 84 bytes */
	struct dram_pwr_conf dram_conf[NR_DRAM_PWR_TYPE];

	/* remaining size = 572 bytes */
};

extern struct swpm_rec_data *swpm_info_ref;

#endif

