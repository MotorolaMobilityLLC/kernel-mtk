/*
 * Copyright (C) 2018 MediaTek Inc.
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

#ifndef __MDLA_PMU_H__
#define __MDLA_PMU_H__

#include <linux/types.h>

#if 0
// TODO: Userspace should not use enumerations defined by hardware
enum MDLA_PMU_INTERFACE {
	MDLA_PMU_IF_WDEC0 = 0xe,
	MDLA_PMU_IF_WDEC1 = 0xf,
	MDLA_PMU_IF_CBLD0 = 0x10,
	MDLA_PMU_IF_CBLD1 = 0x11,
	MDLA_PMU_IF_SBLD0 = 0x12,
	MDLA_PMU_IF_SBLD1 = 0x13,
	MDLA_PMU_IF_STE0 = 0x14,
	MDLA_PMU_IF_STE1 = 0x15,
	MDLA_PMU_IF_CMDE = 0x16,
	MDLA_PMU_IF_DDE = 0x17,
	MDLA_PMU_IF_CONV = 0x18,
	MDLA_PMU_IF_RQU = 0x19,
	MDLA_PMU_IF_POOLING = 0x1a,
	MDLA_PMU_IF_EWE = 0x1b,
	MDLA_PMU_IF_CFLD = 0x1c
};

enum MDLA_PMU_DDE_EVENT {
	MDLA_PMU_DDE_WORK_CYC = 0x0,
	MDLA_PMU_DDE_TILE_DONE_CNT,
	MDLA_PMU_DDE_EFF_WORK_CYC,
	MDLA_PMU_DDE_BLOCK_CNT,
	MDLA_PMU_DDE_READ_CB_WT_CNT,
	MDLA_PMU_DDE_READ_CB_ACT_CNT,
	MDLA_PMU_DDE_WAIT_CB_TOKEN_CNT,
	MDLA_PMU_DDE_WAIT_CONV_RDY_CNT,
	MDLA_PMU_DDE_WAIT_CB_FCWT_CNT,
};

enum MDLA_PMU_MODE {
	MDLA_PMU_ACC_MODE = 0x0,
	MDLA_PMU_INTERVAL_MODE = 0x1,
};
#endif

#define MDLA_PMU_COUNTERS 15

unsigned int pmu_reg_read(u32 offset);

int pmu_counter_alloc(u32 interface, u32 event);
int pmu_counter_free(int handle);
int pmu_counter_event_save(u32 handle, u32 val);
int pmu_counter_event_get(int handle);
int pmu_counter_event_get_all(u32 out[MDLA_PMU_COUNTERS]);
u32 pmu_counter_get(int handle);
void pmu_counter_get_all(u32 out[MDLA_PMU_COUNTERS]);
void pmu_counter_read_all(u32 out[MDLA_PMU_COUNTERS]);

void pmu_reg_save(void);
void pmu_clr_mode_save(u32 mode);

u32 pmu_get_perf_start(void);
u32 pmu_get_perf_end(void);
u32 pmu_get_perf_cycle(void);

void pmu_reset_saved_counter(void);
void pmu_reset_saved_cycle(void);

void pmu_init(void);
void pmu_reset(void);

#endif

