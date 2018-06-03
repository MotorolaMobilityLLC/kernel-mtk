
/*
 * Copyright (C) 2016 MediaTek Inc.
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
#ifndef _MTK_EEM_CONFIG_H_
#define _MTK_EEM_CONFIG_H_

/* CONFIG (SW related) */
#define CONFIG_EEM_SHOWLOG	(0)
#define EN_ISR_LOG			(1)
#define DVT					(1)
#define EEM_BANK_SOC		(0) /* use voltage bin, so disable it */
#define EARLY_PORTING		(0) /* for detecting real vboot in eem_init01 */
#define DUMP_DATA_TO_DE		(1)
#define EEM_ENABLE			(0)
#define EEM_FAKE_EFUSE		(0)
#define UPDATE_TO_UPOWER	(0)
#define EEM_LOCKTIME_LIMIT	(3000)

/* These 4 configs are reserving for sspm EEM */
/* #define EEM_CUR_VOLT_PROC_SHOW */
#define EEM_OFFSET
/* #define EEM_LOG_EN */
/* #define EEM_VCORE_IN_SSPM */

/* for early porting */
#ifdef CONFIG_FPGA_EARLY_PORTING
#define EARLY_PORTING_GPU
#else
#define EARLY_PORTING_GPU
#endif

#define EARLY_PORTING_CPU
/* #define EARLY_PORTING_PPM */
/* #define EARLY_PORTING_PMIC */
/* #define EARLY_PORTING_VCORE */
/* because thermal still have bug */
/* #define EARLY_PORTING_THERMAL */

/* only used for ap ptp */
#ifdef CONFIG_MTK_TINYSYS_SSPM_SUPPORT
	#define EEM_ENABLE_TINYSYS_SSPM (0)
#else
	#define EEM_ENABLE_TINYSYS_SSPM (0)
#endif
#define SET_PMIC_VOLT (1)
#define SET_PMIC_VOLT_TO_DVFS (0)
#define LOG_INTERVAL	(2LL * NSEC_PER_SEC)
#define ITurbo (0)

/* fake devinfo for early verification */
/* big */
#define DEVINFO_0 0x10E0660C
#define DEVINFO_1 0x004E0063
#define DEVINFO_DVT_0 0x10BD3C1B
#define DEVINFO_DVT_1 0x00550000
#define DEVINFO_IDX_0 50
#define DEVINFO_IDX_1 51
/* cci */
#define DEVINFO_2 0x10E077F1
#define DEVINFO_3 0x004E0063
#define DEVINFO_DVT_2 0x10BD3C1B
#define DEVINFO_DVT_3 0x00550055
#define DEVINFO_IDX_2 52
#define DEVINFO_IDX_3 53
/* gpu */
#define DEVINFO_4 0x10E84E1D
#define DEVINFO_5 0x00440003
#define DEVINFO_DVT_4 0x10BD3C1B
#define DEVINFO_DVT_5 0x005500C0
#define DEVINFO_IDX_4 54
#define DEVINFO_IDX_5 55
/* LL */
#define DEVINFO_6 0x10E097C0
#define DEVINFO_7 0x004E0063
#define DEVINFO_DVT_6 0x10BD3C1B
#define DEVINFO_DVT_7 0x00550055
#define DEVINFO_IDX_6 56
#define DEVINFO_IDX_7 57
/* L */
#define DEVINFO_8 0x10E081E2
#define DEVINFO_9 0x004E0063
#define DEVINFO_DVT_8 0x10BD3C1B
#define DEVINFO_DVT_9 0x005500C0
#define DEVINFO_IDX_8 58
#define DEVINFO_IDX_9 59
/* soc */
#define DEVINFO_12 0x00000000
#define DEVINFO_IDX_12 62

/* atever (DEVINFO 18) */
#define DEVINFO_IDX_FTPGM 68

/* segment */
#define DEVINFO_IDX_SEG 30

/* voltage adjust */
#define DEVINFO_IDX_VOLT_ADJUST 3
/*****************************************
* eem sw setting
******************************************
*/
#define NR_HW_RES		(11) /* reserve for eem log */
#define NR_HW_RES_FOR_BANK	(11) /* real eem banks for efuse */
#define VCORE_NR_FREQ_EFUSE     (16)
#define VCORE_NR_FREQ		(4)
#define HW_RES_IDX_TURBO	(11) /* for providing turbo value to cpu dvfs */
#define EEM_INIT01_FLAG		(0x1F) /* should be 0x5F if vcore eem enable */

#if DVT
	#define NR_FREQ 8
	#define NR_FREQ_GPU 8
	#define NR_FREQ_CPU 8
#else
/* if max freq = 16 , NR_FREQ = 16, NR_FREQ_CPU = 16
 * if max freq = 4, NR_FREQ = 8, NR_FREQ_CPU = 4
*/
	#define NR_FREQ 16
	#define NR_FREQ_GPU 16
	#define NR_FREQ_CPU 16
#endif

/*
 * 100 us, This is the EEM Detector sampling time as represented in
 * cycles of bclk_ck during INIT. 52 MHz
 */
#define DETWINDOW_VAL		0x514

/*
 * mili Volt to config value. voltage = 600mV + val * 6.25mV
 * val = (voltage - 600) / 6.25
 * @mV:	mili volt
 */

/* 1mV=>10uV */
/* EEM */
#define EEM_V_BASE	  (50000)
#define EEM_STEP		(625)

/* SRAM (for vcore) */
#define SRAM_BASE	   (51875)
#define SRAM_STEP	   (625)
#define EEM_2_RMIC(value)	(((((value) * EEM_STEP) + EEM_V_BASE - SRAM_BASE + SRAM_STEP - 1) / SRAM_STEP) + 3)
#define PMIC_2_RMIC(det, value)	\
	(((((value) * det->pmic_step) + det->pmic_base - SRAM_BASE + SRAM_STEP - 1) / SRAM_STEP) + 3)
#define VOLT_2_RMIC(volt)	(((((volt) - SRAM_BASE) + SRAM_STEP - 1) / SRAM_STEP) + 3)

/* for voltage bin vcore dvfs */
#define VCORE_PMIC_BASE (50000)
#define VCORE_PMIC_STEP (625)
#if 0
#define VOLT_2_VCORE_PMIC(volt)   ((((volt) - VCORE_PMIC_BASE) + VCORE_PMIC_STEP - 1) / VCORE_PMIC_STEP)
#define VCORE_PMIC_2_VOLT(pmic) (((pmic) * VCORE_PMIC_STEP) + VCORE_PMIC_BASE)
#endif

/* CPU */
#define CPU_PMIC_BASE	   (40625)
#define CPU_PMIC_STEP	   (625) /* 1.231/1024=0.001202v=120(10uv)*/
#define MAX_ITURBO_OFFSET   (2000)

/* GPU */
#define GPU_PMIC_BASE	   (40625)
#define GPU_PMIC_STEP	   (625) /* 1.231/1024=0.001202v=120(10uv)*/

/* common part: for big, cci, LL, L, GPU, SOC*/
#define VBOOT_VAL		(0x37) /* volt domain: 0.75.v */
#define VMAX_VAL		(0x63) /* volt domain: 1.025v*/
#define VMIN_VAL		(0x1A)  /* volt domain: 0.568v*/
#define VCO_VAL		 (0x1A)
#define DVTFIXED_VAL	(0x6)

#define DTHI_VAL		(0x01)		/* positive */
#define DTLO_VAL		(0xfe)		/* negative (2's compliment) */
#define DETMAX_VAL		(0xffff)	/* This timeout value is in cycles of bclk_ck. */
#define AGECONFIG_VAL	(0x555555)
#define AGEM_VAL		(0x0)
#define DCCONFIG_VAL	(0x555555)

/* different for GPU */
#define VBOOT_VAL_GPU	   (0x2F) /* eem domain: 0x2F, volt domain: 0.7v */
#define VMAX_VAL_GPU		(0x4F) /* eem domain: 0x4F, volt domain: 0.9v */
#define DVTFIXED_VAL_GPU	(0x4)

/* different for SOC */
#define VMAX_VAL_SOC		(0x37) /* eem domain: 0x37, volt domain: 0.75v */

/* use in base_ops_mon_mode */
#define MTS_VAL	(0x1fb)
#define BTS_VAL	(0x6d1)

#define CORESEL_VAL (0x83ff0000)
#define CORESEL_INIT2_VAL (0x03ff0000)
#define INVERT_TEMP_VAL (33000)
/* After ATE program version 5 */
/* #define VCO_VAL_AFTER_5		0x04 */
/* #define VCO_VAL_AFTER_5_BIG	0x32 */

/* #define VMAX_SRAM			(115000) */
/* #define VMIN_SRAM			(80000) */

#endif
