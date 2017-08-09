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
#define MD_BOOT_VECTOR 0x20190000
#define MD_BOOT_VECTOR_KEY 0x2019379C
#define MD_BOOT_VECTOR_EN 0x20195488
#define MD_RGU_BASE 0x20050000
#define MD_GLOBAL_CON0 0x20000450
#define MD_GLOBAL_CON0_CLDMA_BIT 12
/* #define MD_PEER_WAKEUP 0x20030B00 */
#define MD_BUS_STATUS_BASE 0x20000000
#define MD_BUS_STATUS_LENGTH 0x468
#define MD_PC_MONITOR_BASE 0x201F0000
#define MD_PC_MONITOR_LENGTH 0x114
#define MD_TOPSM_STATUS_BASE 0x20030000
#define MD_TOPSM_STATUS_LENGTH 0xBFF
#define MD_PLL_BASE 0x20120000
#define MD_PLL_LENGTH 0x800
#define MD_OST_STATUS_BASE 0x20040000
#define MD_OST_STATUS_LENGTH 0x1FF
/* #define _MD_CCIF0_BASE (AP_CCIF0_BASE+0x1000) */
#define CCIF_SRAM_SIZE 512

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

#define MD3_BOOT_VECTOR_VALUE 0x00000000
#define MD3_BOOT_VECTOR_KEY_VALUE 0x3567C766
#define MD3_BOOT_VECTOR_EN_VALUE 0xA3B66175
#define MD3_RGU_BASE 0x30050000
#define APCCIF1_SRAM_SIZE 512

#endif				/* __MODEM_REG_BASE_H__ */
