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
#define EEM_NOT_READY (1) /* #define EEM_NOT_READY	(1) */
#define CONFIG_EEM_SHOWLOG (0)
#define EN_ISR_LOG (0)
#define EEM_BANK_SOC (0) /* use voltage bin, so disable it */
#define EARLY_PORTING (0) /* for detecting real vboot in eem_init01 */
#define DUMP_DATA_TO_DE (1)
#define EEM_ENABLE (1) /* enable; after pass HPT mini-SQC */
#define EEM_FAKE_EFUSE (0)
/* FIX ME */
#define UPDATE_TO_UPOWER (1)
#define EEM_LOCKTIME_LIMIT (3000)
#define ENABLE_EEMCTL0 (1)
#define ENABLE_INIT1_STRESS (1)

#define EEM_OFFSET
#define SET_PMIC_VOLT (1)
#define SET_PMIC_VOLT_TO_DVFS (1)
#define LOG_INTERVAL	(2LL * NSEC_PER_SEC)

#define DEVINFO_IDX_0 50	/* 580 */
#define DEVINFO_IDX_1 51	/* 584 */
#define DEVINFO_IDX_2 52	/* 588 */
#define DEVINFO_IDX_3 53	/* 58C */
#define DEVINFO_IDX_4 54	/* 590 */
#define DEVINFO_IDX_5 55	/* 594 */
#define DEVINFO_IDX_6 56	/* 598 */
#define DEVINFO_IDX_7 57	/* 59C */
#define DEVINFO_IDX_8 58	/* 5A0 */

/* Fake EFUSE */
#define DEVINFO_0 0x0000FF00
/* L */
#define DEVINFO_1 0x10BD3C1B
#define DEVINFO_2 0x00550000
/* LL */
#define DEVINFO_3 0x10BD3C1B
#define DEVINFO_4 0x00550000
/* CCI */
#define DEVINFO_5 0x10BD3C1B
#define DEVINFO_6 0x00550000
/* GPU */
#define DEVINFO_7 0x10BD3C1B
#define DEVINFO_8 0x00550000

/*****************************************
 * eem sw setting
 ******************************************
 */
#define NR_HW_RES_FOR_BANK	(9) /* real eem banks for efuse */
#define EEM_INIT01_FLAG (0xF) /* [3]:GPU, [2]:CCI, [1]:LL, [0]:L */

#define NR_FREQ 16
#define NR_FREQ_GPU 16
#define NR_FREQ_CPU 16

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
#define EEM_V_BASE		(40000)
#define EEM_STEP		(625)

/* CPU */
#define CPU_PMIC_BASE_6357	(51875)
#define CPU_PMIC_STEP		(625) /* 1.231/1024=0.001202v=120(10uv)*/

/* GPU */
#define GPU_PMIC_BASE		(51875)
#define GPU_PMIC_STEP		(625) /* 1.231/1024=0.001202v=120(10uv)*/

/* common part: for cci, LL, L, GPU */
#define VBOOT_VAL		(0x2d) /* volt domain: 0.8v */
#define VMAX_VAL		(0x5d) /* volt domain: 1.12v*/
#define VMIN_VAL		(0xd) /* volt domain: 0.6v*/
#define VCO_VAL			(0xd)
#define DVTFIXED_VAL	(0x7)

#define DTHI_VAL		(0x01) /* positive */
#define DTLO_VAL		(0xfe) /* negative (2's compliment) */
#define DETMAX_VAL (0xffff) /*This timeout value is in cycles of bclk_ck.*/
#define AGECONFIG_VAL		(0x555555)
#define AGEM_VAL		(0x0)
#define DCCONFIG_VAL		(0x555555)

/* different for GPU */
#define VBOOT_VAL_GPU (0x2d) /* eem domain: 0x40, volt domain: 0.8v */
#define VMAX_VAL_GPU (0x4d) /* eem domain: 0x60, volt domain: 1.0v */
#define DVTFIXED_VAL_GPU	(0x6)

/* use in base_ops_mon_mode */
#define MTS_VAL			(0x1fb)
#define BTS_VAL			(0x6d1)

#define CORESEL_VAL			(0x8fff0000)
#define CORESEL_INIT2_VAL		(0x0fff0000)


#define INVERT_TEMP_VAL (25000)
#define OVER_INV_TEM_VAL (27000)

#define LOW_TEMP_OFF_DEFAULT (0)

#if ENABLE_EEMCTL0
#define EEM_CTL0_L (0x00000001)
#define EEM_CTL0_2L (0x00010001)
#define EEM_CTL0_CCI (0x00100003)
#define EEM_CTL0_GPU (0x00020001)
#endif

#if EEM_FAKE_EFUSE	/* select PTP secure mode based on efuse config. */
#define SEC_MOD_SEL			0xF0		/* non secure  mode */
#else
#define SEC_MOD_SEL			0x00		/* Secure Mode 0 */
/* #define SEC_MOD_SEL			0x10	*/	/* Secure Mode 1 */
/* #define SEC_MOD_SEL			0x20	*/	/* Secure Mode 2 */
/* #define SEC_MOD_SEL			0x30	*/	/* Secure Mode 3 */
/* #define SEC_MOD_SEL			0x40	*/	/* Secure Mode 4 */
#endif

#if SEC_MOD_SEL == 0x00
#define SEC_DCBDET 0xCC
#define SEC_DCMDET 0xE6
#define SEC_BDES 0xF5
#define SEC_MDES 0x97
#define SEC_MTDES 0xAC
#elif SEC_MOD_SEL == 0x10
#define SEC_DCBDET 0xE5
#define SEC_DCMDET 0xB
#define SEC_BDES 0x31
#define SEC_MDES 0x53
#define SEC_MTDES 0x68
#elif SEC_MOD_SEL == 0x20
#define SEC_DCBDET 0x39
#define SEC_DCMDET 0xFE
#define SEC_BDES 0x18
#define SEC_MDES 0x8F
#define SEC_MTDES 0xB4
#elif SEC_MOD_SEL == 0x30
#define SEC_DCBDET 0xDF
#define SEC_DCMDET 0x18
#define SEC_BDES 0x0B
#define SEC_MDES 0x7A
#define SEC_MTDES 0x52
#elif SEC_MOD_SEL == 0x40
#define SEC_DCBDET 0x36
#define SEC_DCMDET 0xF1
#define SEC_BDES 0xE2
#define SEC_MDES 0x80
#define SEC_MTDES 0x41
#endif

#endif
