
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
#define EN_ISR_LOG		(1)
#define EEM_BANK_SOC		(0) /* use voltage bin, so disable it */
#define EARLY_PORTING		(0) /* for detecting real vboot in eem_init01 */
#define DUMP_DATA_TO_DE		(1)
#define EEM_ENABLE		(1) /* enable; after pass HPT mini-SQC */
#define EEM_FAKE_EFUSE		(0)
#define UPDATE_TO_UPOWER	(1)
#define EEM_LOCKTIME_LIMIT	(3000)
#define ENABLE_EEMCTL0		(1)

#define EEM_OFFSET
#define EEM_ENABLE_TINYSYS_SSPM (0)
#define SET_PMIC_VOLT (1)
#define SET_PMIC_VOLT_TO_DVFS (1)
#define LOG_INTERVAL	(2LL * NSEC_PER_SEC)

/* FIX ME */
#define DEVINFO_IDX_0 50
#define DEVINFO_IDX_1 51
#define DEVINFO_IDX_2 52
#define DEVINFO_IDX_3 53
#define DEVINFO_IDX_4 54
#define DEVINFO_IDX_5 55
#define DEVINFO_IDX_6 56
#define DEVINFO_IDX_7 57
#define DEVINFO_IDX_8 58
#define DEVINFO_IDX_9 131

/* Fake EFUSE */
#define DEVINFO_0 0xFF00
/* LL_LOW */
#define DEVINFO_1 0x10bd3c1b
/* L_LOW + LL_LOW */
#define DEVINFO_2 0x550055
/* L_LOW */
#define DEVINFO_3 0x10bd3c1b
/* CCI */
#define DEVINFO_4 0x10bd3c1b
/* GPU + CCI */
#define DEVINFO_5 0x550055
/* GPU */
#define DEVINFO_6 0x10bd3c1b
/* LL_HIGH */
#define DEVINFO_7 0x10bd3c1b
/* L_HIGH + LL_HIGH */
#define DEVINFO_8 0x550055
/* L_HIGH */
#define DEVINFO_9 0x10bd3c1b

/*****************************************
* eem sw setting
******************************************
*/
#define NR_HW_RES_FOR_BANK	(10) /* real eem banks for efuse */
#define EEM_INIT01_FLAG		(0x0F) /* should be 0x0F */

#define NR_FREQ 16
#define NR_FREQ_GPU 16
#define NR_FREQ_CPU 16

/*
 * 100 us, This is the EEM Detector sampling time as represented in
 * cycles of bclk_ck during INIT. 52 MHz
 */
#define DETWINDOW_VAL		0xA28
#define DETWINDOW_VAL_FAKE	0xA28

/*
 * mili Volt to config value. voltage = 600mV + val * 6.25mV
 * val = (voltage - 600) / 6.25
 * @mV:	mili volt
 */

/* 1mV=>10uV */
/* EEM */
#define EEM_V_BASE		(50000)
#define EEM_STEP		(625)

/* CPU */
#define CPU_PMIC_BASE_6358	(50000) /* (50000) */
#define CPU_PMIC_STEP		(625) /* 1.231/1024=0.001202v=120(10uv)*/

/* GPU */
#define GPU_PMIC_BASE		(50000)
#define GPU_PMIC_STEP		(625) /* 1.231/1024=0.001202v=120(10uv)*/

/* common part: for big, cci, LL, L, GPU, SOC*/
#define VBOOT_VAL		(0x40) /* volt domain: 0.8v */
#define VMAX_VAL		(0x74) /* volt domain: 1.12v*/
#define VMIN_VAL		(0x20) /* volt domain: 0.6v*/
#define VCO_VAL			(0x20)
#define DVTFIXED_VAL		(0x9)

#define DTHI_VAL		(0x01) /* positive */
#define DTLO_VAL		(0xfe) /* negative (2's compliment) */
#define DETMAX_VAL		(0xffff) /* This timeout value is in cycles of bclk_ck. */
#define AGECONFIG_VAL		(0x555555)
#define AGEM_VAL		(0x0)
#define DCCONFIG_VAL		(0x555555)

/* different for GPU */
#define VBOOT_VAL_GPU		(0x40) /* eem domain: 0x40, volt domain: 0.8v */
#define VMAX_VAL_GPU		(0x50) /* eem domain: 0x60, volt domain: 1.0v */
#define DVTFIXED_VAL_GPU	(0x3)

/* use in base_ops_mon_mode */
#define MTS_VAL			(0x1fb)
#define BTS_VAL			(0x6d1)

#define CORESEL_VAL (0x800f0000)
#define CORESEL_INIT2_VAL (0x000f0000)

#define INVERT_TEMP_VAL (25000)
#define OVER_INV_TEM_VAL (27000)

#define LOW_TEMP_OFF_DEFAULT (0)

#if ENABLE_EEMCTL0
#define EEM_CTL0_2L (0x00010001)
#define EEM_CTL0_L (0x00000001)
#define EEM_CTL0_CCI (0x02100003)
#define EEM_CTL0_GPU (0x00020001)
#endif
#endif
