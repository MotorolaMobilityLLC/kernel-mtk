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

#define MD_PCORE_PCCIF_BASE 0x20510000

#define MD_GLOBAL_CON0 0x20000450
#define MD_GLOBAL_CON0_CLDMA_BIT 12
#define CCIF_SRAM_SIZE 512

#define BASE_ADDR_MDRSTCTL   0x200f0000  /* From md, no use by AP directly */
#define L1_BASE_ADDR_L1RGU   0x26010000  /* From md, no use by AP directly  */
#define MD_RGU_BASE          (BASE_ADDR_MDRSTCTL + 0x100)  /* AP use */
#define L1_RGU_BASE          L1_BASE_ADDR_L1RGU    /* AP use */

/* MD1 PLL */
#define MDTOP_PLLMIXED_BASE		(0x20140000)
#define MDTOP_PLLMIXED_LENGTH	(0x1000)
#define MDTOP_CLKSW_BASE			(0x20150000)
#define MDTOP_CLKSW_LENGTH	(0x1000)

#define MD_PERI_MISC_BASE			(0x20060000)
#define MD_PERI_MISC_LEN   0xD0
#define MDL1A0_BASE				(0x260F0000)
#define MDL1A0_LEN  0x200

#define MDSYS_CLKCTL_BASE			(0x20120000)
#define MDSYS_CLKCTL_LEN   0xD0
/*#define L1_BASE_MADDR_MDL1_CONF	(0x260F0000)*/

/* MD Exception dump register list start[ */
#define MD1_OPEN_DEBUG_APB_CLK		(0x10006000)
/* PC Monitor */
#define MD1_PC_MONITOR_BASE0		(0x0D0D9800)
#define MD1_PC_MONITOR_LEN0		(0x800)
#define MD1_PC_MONITOR_BASE1		(0x0D0D9000)
#define MD1_PC_MONITOR_LEN1		(0x800)

#define MD1_PCMNO_CTRL_OFFSET		(0)
#define MD1_PCMON_CTRL_STOP_ALL		0x22
#define MD1_PCMON_RESTART_ALL_VAL	0x11
#define MD1_PCMON_CORE1_SWITCH		0x100
#define MD1_PCMON_CORE2_SWITCH		0x200
#define MD1_PCMON_CORE3_SWITCH		0x300

/* PLL reg (clock control) */
/** MD CLKSW **/
#define MD_CLKSW_BASE			(0x0D0D6000)
#define MD_CLKSW_LENGTH  0xD4

#define MD1_CLKSW_BASE0			(0x0D0D6000)
#define MD1_CLKSW_LEN0			0xD4
#define MD1_CLKSW_BASE1			(0x0D0D6100)
#define MD1_CLKSW_LEN1			0x18
#define MD1_CLKSW_BASE2			(0x0D0D6200)
#define MD1_CLKSW_LEN2			0x4
#define MD1_CLKSW_BASE3			(0x0D0D6F00)
#define MD1_CLKSW_LEN3			0x8
/** MD PLLMIXED **/
#define MD1_PLLMIXED_BASE0		(0x0D0D4000)
#define MD1_PLLMIXED_LEN0		0x68
#define MD1_PLLMIXED_BASE1		(0x0D0D4100)
#define MD1_PLLMIXED_LEN1		0x18
#define MD1_PLLMIXED_BASE2		(0x0D0D4200)
#define MD1_PLLMIXED_LEN2		0x8
#define MD1_PLLMIXED_BASE3		(0x0D0D4300)
#define MD1_PLLMIXED_LEN3		0x1C
#define MD1_PLLMIXED_BASE4		(0x0D0D4400)
#define MD1_PLLMIXED_LEN4		0x5C
#define MD1_PLLMIXED_BASE5		(0x0D0D4500)
#define MD1_PLLMIXED_LEN5		0xD0
#define MD1_PLLMIXED_BASE6		(0x0D0D4600)
#define MD1_PLLMIXED_LEN6		0x10
#define MD1_PLLMIXED_BASE7		(0x0D0D4C00)
#define MD1_PLLMIXED_LEN7		0x48
#define MD1_PLLMIXED_BASE8		(0x0D0D4D00)
#define MD1_PLLMIXED_LEN8		0x8
#define MD1_PLLMIXED_BASE9		(0x0D0D4F00)
#define MD1_PLLMIXED_LEN9		0x14

/** MD CLKCTL **/
#define MD1_CLKCTL_BASE0			(0x0D0C3800)
#define MD1_CLKCTL_LEN0			0x1C
#define MD1_CLKCTL_BASE1			(0x0D0C3910)
#define MD1_CLKCTL_LEN1			0x20

/** MD GLOBALCON **/
#define MD1_GLOBALCON_BASE0		(0x0D0D5000)
#define MD1_GLOBALCON_LEN0		0xA0
#define MD1_GLOBALCON_BASE1		(0x0D0D5100)
#define MD1_GLOBALCON_LEN1		0x10
#define MD1_GLOBALCON_BASE2		(0x0D0D5200)
#define MD1_GLOBALCON_LEN2		0x98
#define MD1_GLOBALCON_BASE3		(0x0D0D5300)
#define MD1_GLOBALCON_LEN3		0x24
#define MD1_GLOBALCON_BASE4		(0x0D0D5800)
#define MD1_GLOBALCON_LEN4		0x8
#define MD1_GLOBALCON_BASE5		(0x0D0D5900)
#define MD1_GLOBALCON_LEN5		0x700		/*incude more than 1 region*/
/* BUS reg */
#define MD1_BUS_REG_BASE0		(0x0D0C2000)/* mdmcu_misc_reg */
#define MD1_BUS_REG_LEN0			0x100
#define MD1_BUS_REG_BASE1		(0x0D0C7000)/* mdinfra_misc_reg */
#define MD1_BUS_REG_LEN1			0xAC
#define MD1_BUS_REG_BASE2		(0x0D0C9000)/* cm2_misc */
#define MD1_BUS_REG_LEN2			0xAC
#define MD1_BUS_REG_BASE3		(0x0D0E0000)/* modeml1_ao_config */
#define MD1_BUS_REG_LEN3			0x6C
/* BUSREC */
#define MD1_MCU_MO_BUSREC_STOP_VAL	0x0
#define MD1_MCU_MO_BUSREC_BASE0		(0x0D0C6000)
#define MD1_MCU_MO_BUSREC_LEN0		0x1000
#define MD1_MCU_MO_BUSREC_RESTART_VAL	0x1

#define MD1_INFRA_BUSREC_STOP_VAL	0x0
#define MD1_INFRA_BUSREC_BASE0		(0x0D0C8000)
#define MD1_INFRA_BUSREC_LEN0		0x1000
#define MD1_INFRA_BUSREC_RESTART_VAL	0x1

#define MD1_BUSREC_LAY_BASE0		(0x0D0C2500)
#define MD1_BUSREC_LAY_LEN0		0x8

/* ECT */
#define MD1_ECT_REG_BASE0		(0x0D0CC130)/* MD ECT triggerIn/Out status */
#define MD1_ECT_REG_LEN0			0x8
#define MD1_ECT_REG_BASE1		(0x0D0CD130)/* ModemSys ECT triggerIn/Out status */
#define MD1_ECT_REG_LEN1			0x8
#define MD1_ECT_REG_BASE2		(0x0D0CE014)/* MD32 ECT status */
#define MD1_ECT_REG_LEN2			0x4
#define MD1_ECT_REG_BASE3		(0x0D0CE00C)/* MD32 ECT status */
#define MD1_ECT_REG_LEN3			0x4
/* TOPSM reg */
#define MD1_TOPSM_REG_BASE0		(0x0200D0000)
#define MD1_TOPSM_REG_LEN0		0x8E4
/* MD RGU reg */
#define MD1_RGU_REG_BASE0		(0x0200F0100)
#define MD1_RGU_REG_LEN0			0xCC
#define MD1_RGU_REG_BASE1		(0x0200F0300)
#define MD1_RGU_REG_LEN1			0x5C
/* OST status */
#define MD_OST_STATUS_BASE		0x200E0000
#define MD_OST_STATUS_LENGTH		0x300
/* CSC reg */
#define MD_CSC_REG_BASE			0x20100000
#define MD_CSC_REG_LENGTH		0x214

/*MD bootup register*/
#define MD1_CFG_BASE (0x1020E300)
#define MD1_CFG_BOOT_STATS0 (MD1_CFG_BASE+0x00)
#define MD1_CFG_BOOT_STATS1 (MD1_CFG_BASE+0x04)

/* MD Exception dump register list end] */

#define MD_SRAM_PD_PSMCUSYS_SRAM_BASE	(0x200D0000)
#define MD_SRAM_PD_PSMCUSYS_SRAM_LEN	(0xB00)

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
