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

#ifndef __MODEM_REG_BASE_H__
#define __MODEM_REG_BASE_H__

/* ============================================================ */
/* Modem 1 part */
/* ============================================================ */
/* MD peripheral register: MD bank8; AP bank2 */
#define CLDMA_AP_BASE 0x200F0000
#define CLDMA_AP_LENGTH 0x3000
#define CLDMA_MD_BASE 0x200E0000
#define CLDMA_MD_LENGTH 0x3000

#define MD_BOOT_VECTOR_EN 0x20000024

#define MD_GLOBAL_CON0 0x20000450
#define MD_GLOBAL_CON0_CLDMA_BIT 12
/* #define MD_PEER_WAKEUP 0x20030B00 */
#define MD_BUS_STATUS_BASE 0x203B0000/* 0x20000000 */
#define MD_BUS_STATUS_LENGTH 0x214 /*0x468 */
#define MD_PC_MONITOR_BASE 0x10473000 /* 0x201F0000 */
#define MD_PC_MONITOR_LENGTH 0x500 /* 0x114 */
#define MD_PC_MONITORL1_BASE 0x1046F000 /* 0x201F0000 */
#define MD_PC_MONITORL1_LENGTH 0x500 /* 0x114 */
#define MD_TOPSM_STATUS_BASE 0x200D0000 /* 0x20030000 */
#define MD_TOPSM_STATUS_LENGTH  0xC30 /* 0xBFF */
#define MD_OST_STATUS_BASE  0x200E000 /* 0x20040000 */
#define MD_OST_STATUS_LENGTH  0x228 /* 0x1FF */
/* #define _MD_CCIF0_BASE (AP_CCIF0_BASE+0x1000) */
#define CCIF_SRAM_SIZE 512

#define BASE_ADDR_MDRSTCTL   0x200f0000  /* From md, no use by AP directly */
#define L1_BASE_ADDR_L1RGU   0x26010000  /* From md, no use by AP directly  */
#define MD_RGU_BASE          (BASE_ADDR_MDRSTCTL + 0x100)  /* AP use */
#define L1_RGU_BASE          L1_BASE_ADDR_L1RGU    /* AP use */

/* MD1 PLL */
#define MD_CLKSW_BASE				(0x20150000)
#define MD_CLKSW_LENGTH  0x118
#define MD_GLOBAL_CON_DCM_BASE	(0x20130000)
#define MD_GLOBAL_CON_DCM_LEN  0x1004
#define PSMCU_MISC_BASE			(0x20200000)
#define MD_PERI_MISC_BASE			(0x20060000)
#define MD_PERI_MISC_LEN   0xD0
#define MDL1A0_BASE				(0x260F0000)
#define MDL1A0_LEN  0x200
#define MDTOP_PLLMIXED_BASE		(0x20140000)
#define MDSYS_CLKCTL_BASE			(0x20120000)
#define MDSYS_CLKCTL_LEN   0xD0
/*#define L1_BASE_MADDR_MDL1_CONF	(0x260F0000)*/

/*MD bootup register*/
/* in modem view */
/* #define MD1_CFG_BASE (0x20000000) */
/* #define MD1_CFG_BOOT_STATS0 (MD1_CFG_BASE+0x70) */
/* #define MD1_CFG_BOOT_STATS1 (MD1_CFG_BASE+0x74) */

/* in AP infra view */
#define MD1_CFG_BOOT_STATS0 (0x10201300)
#define MD1_CFG_BOOT_STATS1 (0x10201304)
/* ============================================================ */
/* Modem 2 part */
/* ============================================================ */
#define MD2_BOOT_VECTOR 0x30190000
#define MD2_BOOT_VECTOR_KEY 0x3019379C
#define MD2_BOOT_VECTOR_EN 0x30195488

#define MD2_BOOT_VECTOR_VALUE 0x00000000
#define MD2_BOOT_VECTOR_KEY_VALUE 0x3567C766
#define MD2_BOOT_VECTOR_EN_VALUE 0xA3B66175
#define MD2_RGU_BASE 0x30050000
#define APCCIF1_SRAM_SIZE 512

/*
* ============================================================
*  Modem 3 part
* ============================================================
* need modify, haow
*/
#define MD3_BOOT_VECTOR 0x30190000
#define MD3_BOOT_VECTOR_KEY 0x3019379C
#define MD3_BOOT_VECTOR_EN 0x30195488

#define MD3_BOOT_VECTOR_VALUE		0x00000000
#define MD3_BOOT_VECTOR_KEY_VALUE	0x3567C766
#define MD3_BOOT_VECTOR_EN_VALUE	0xA3B66175

#define MD3_RGU_BASE			0x3A001080
#define APCCIF1_SRAM_SIZE		512

#endif				/* __MODEM_REG_BASE_H__ */
