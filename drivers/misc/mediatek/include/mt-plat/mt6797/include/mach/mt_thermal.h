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

#ifndef _MT6797_THERMAL_H
#define _MT6797_THERMAL_H

#include <linux/module.h>
#include <linux/types.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>

#include <asm/io.h>
#include <asm/uaccess.h>

#include "mt-plat/sync_write.h"
#include "mtk_thermal_typedefs.h"
#include "mt_gpufreq.h"

/*
Bank0 : BIG     (TS_MCU1)
Bank1 : GPU     (TS_MCU4)
Bank2 : SOC     (TS_MCU2)
Bank3 : CPU-L   (TS_MCU2)
Bank4 : CPU-LL  (TS_MCU2)
Bank5 : MCUCCI  (TS_MCU2)
*/
typedef enum {
	THERMAL_SENSOR1     = 0,/*TS_MCU1*/
	THERMAL_SENSOR2     = 1,/*TS_MCU2*/
	THERMAL_SENSOR3     = 2,/*TS_MCU3*/
	THERMAL_SENSOR4     = 3,/*TS_MCU4*/
	THERMAL_SENSOR_ABB  = 4,/*TS_ABB*/
	THERMAL_SENSOR_NUM
} thermal_sensor_name;

typedef enum {
	THERMAL_BANK0     = 0,
	THERMAL_BANK1     = 1,
	THERMAL_BANK2     = 2,
	THERMAL_BANK3     = 3,
	THERMAL_BANK4     = 4,
	THERMAL_BANK5     = 5,
	THERMAL_BANK_NUM
} thermal_bank_name;


struct TS_PTPOD {
	unsigned int ts_MTS;
	unsigned int ts_BTS;
};

extern int mtktscpu_limited_dmips;

extern int tscpu_is_temp_valid(void); /* Valid if it returns 1, invalid if it returns 0*/
extern void get_thermal_slope_intercept(struct TS_PTPOD *ts_info, thermal_bank_name ts_bank);
extern void set_taklking_flag(bool flag);
extern int tscpu_get_cpu_temp(void);
extern int tscpu_get_temp_by_bank(thermal_bank_name ts_bank);

#define THERMAL_WRAP_WR32(val, addr)        mt_reg_sync_writel((val), ((void *)addr))


extern int get_immediate_big_wrap(void);
extern int get_immediate_gpu_wrap(void);
extern int get_immediate_soc_wrap(void);
extern int get_immediate_cpuL_wrap(void);
extern int get_immediate_cpuLL_wrap(void);
extern int get_immediate_mcucci_wrap(void);

/*add for DLPT*/
extern int tscpu_get_min_cpu_pwr(void);
extern int tscpu_get_min_gpu_pwr(void);

/*4 thermal sensors*/
typedef enum {
	MTK_THERMAL_SENSOR_TS1 = 0,
	MTK_THERMAL_SENSOR_TS2,
	MTK_THERMAL_SENSOR_TS3,
	MTK_THERMAL_SENSOR_TS4,
	MTK_THERMAL_SENSOR_TSABB,

	ATM_CPU_LIMIT,
	ATM_GPU_LIMIT,

	MTK_THERMAL_SENSOR_CPU_COUNT
} MTK_THERMAL_SENSOR_CPU_ID_MET;



extern int tscpu_get_cpu_temp_met(MTK_THERMAL_SENSOR_CPU_ID_MET id);


typedef void (*met_thermalsampler_funcMET)(void);
extern void mt_thermalsampler_registerCB(met_thermalsampler_funcMET pCB);

extern void tscpu_start_thermal(void);
extern void tscpu_stop_thermal(void);
extern void tscpu_cancel_thermal_timer(void);
extern void tscpu_start_thermal_timer(void);
extern void mtkts_btsmdpa_cancel_thermal_timer(void);
extern void mtkts_btsmdpa_start_thermal_timer(void);
extern void mtkts_bts_cancel_thermal_timer(void);
extern void mtkts_bts_start_thermal_timer(void);
extern void mtkts_pmic_cancel_thermal_timer(void);
extern void mtkts_pmic_start_thermal_timer(void);
extern void mtkts_battery_cancel_thermal_timer(void);
extern void mtkts_battery_start_thermal_timer(void);
extern void mtkts_pa_cancel_thermal_timer(void);
extern void mtkts_pa_start_thermal_timer(void);
extern void mtkts_wmt_cancel_thermal_timer(void);
extern void mtkts_wmt_start_thermal_timer(void);

extern void mtkts_allts_cancel_ts1_timer(void);
extern void mtkts_allts_start_ts1_timer(void);
extern void mtkts_allts_cancel_ts2_timer(void);
extern void mtkts_allts_start_ts2_timer(void);
extern void mtkts_allts_cancel_ts3_timer(void);
extern void mtkts_allts_start_ts3_timer(void);
extern void mtkts_allts_cancel_ts4_timer(void);
extern void mtkts_allts_start_ts4_timer(void);
extern void mtkts_allts_cancel_ts5_timer(void);
extern void mtkts_allts_start_ts5_timer(void);

extern int mtkts_bts_get_hw_temp(void);

extern int get_immediate_ts1_wrap(void);
extern int get_immediate_ts2_wrap(void);
extern int get_immediate_ts3_wrap(void);
extern int get_immediate_ts4_wrap(void);
extern int get_immediate_tsabb_wrap(void);

/* Get TS_ temperatures from its thermal_zone instead of raw data,
 * temperature here would be processed according to the policy setting */
extern int mtkts_get_ts1_temp(void);
extern int mtkts_get_ts2_temp(void);
extern int mtkts_get_ts3_temp(void);
extern int mtkts_get_ts4_temp(void);
extern int mtkts_get_tsabb_temp(void);

extern int is_cpu_power_unlimit(void);	/* in mtk_ts_cpu.c */
extern int is_cpu_power_min(void);	/* in mtk_ts_cpu.c */
extern int get_cpu_target_tj(void);
extern int get_cpu_target_offset(void);
extern int mtk_gpufreq_register(struct mt_gpufreq_power_table_info *freqs, int num);

extern int get_target_tj(void);
extern int mtk_thermal_get_tpcb_target(void);
extern void thermal_set_big_core_speed(U32 tempMonCtl1, U32 tempMonCtl2, U32 tempAhbPoll);
#endif
