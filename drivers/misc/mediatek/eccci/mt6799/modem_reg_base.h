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

#define MD_USB_INTERRUPT_MUX_BASE (0x10000280)
#define MD_USB_INTERRUPT_MUX_LENGTH 0x4
/* MD1 PLL */
#define MDTOP_PLLMIXED_BASE		(0x20140000)
#define MDTOP_PLLMIXED_LENGTH	(0xC10)
#define MDTOP_CLKSW_BASE			(0x20150000)
#define MDTOP_CLKSW_LENGTH	(0x1000)

#define MD_PERI_MISC_BASE			(0x20060000)
#define MD_PERI_MISC_LEN   0xD0
#define MDL1A0_BASE				(0x260F0000)
#define MDL1A0_LEN  0x200

#define MDSYS_CLKCTL_BASE			(0x20120000)
#define MDSYS_CLKCTL_LEN   0x300
/*#define L1_BASE_MADDR_MDL1_CONF	(0x260F0000)*/

/* MD Exception dump register list start[ */
#define MD1_OPEN_DEBUG_APB_CLK		(0x10000510)
/* PC Monitor V01 */
#define MD1_PC_MONITOR_BASE0		(0x0D0C5000)
#define MD1_PC_MONITOR_LEN0		(0xB4)
#define MD1_PC_MONITOR_BASE1		(0x0D0C50FC)
#define MD1_PC_MONITOR_LEN1		(0x404)

#define MD1_PCMNO_CTRL_OFFSET		(0)
#define MD1_PCMON_CTRL_STOP_ALL		0xAA
#define MD1_PCMON_RESTART_ALL_VAL	0x55
#define MD1_PCMON_CORE1_SWITCH		0x100
#define MD1_PCMON_CORE2_SWITCH		0x200
#define MD1_PCMON_CORE3_SWITCH		0x300
/* PC Monitor V02 */
#define MD1_PC_MONITOR_BASEC0		(0x0D0C7800)
#define MD1_PC_MONITOR_C_LEN0       (0xD0)
#define MD1_PC_MONITOR_BASEC1		(0x0D0C7910)
#define MD1_PC_MONITOR_C_LEN1       (0x50)
#define MD1_PC_MONITOR_BASEC2		(0x0D0C7A10)
#define MD1_PC_MONITOR_C_LEN2       (0x50)
#define MD1_PC_MONITOR_BASEC3		(0x0D0C7B10)
#define MD1_PC_MONITOR_C_LEN3       (0x50)
#define MD1_PC_MONITOR_BASEC4		(0x0D0C7C10)
#define MD1_PC_MONITOR_C_LEN4		(0x50)
#define MD1_PC_MONITOR_BASE0_1	(0x0D0C5000)
#define MD1_PC_MONITOR_LEN0_1		(0x400)
#define MD1_PC_MONITOR_BASE1_1	(0x0D0C5400)
#define MD1_PC_MONITOR_LEN1_1		(0x400)
#define MD1_PC_MONITOR_BASE2_1	(0x0D0C5800)
#define MD1_PC_MONITOR_LEN2_1		(0x400)
#define MD1_PC_MONITOR_BASE3_1	(0x0D0C5C00)
#define MD1_PC_MONITOR_LEN3_1		(0x400)

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
#define MD1_PLLMIXED_LEN0		0xA8
#define MD1_PLLMIXED_BASE1		(0x0D0D4200)
#define MD1_PLLMIXED_LEN1		0x8
#define MD1_PLLMIXED_BASE2		(0x0D0D4300)
#define MD1_PLLMIXED_LEN2		0x1C
#define MD1_PLLMIXED_BASE3		(0x0D0D4400)
#define MD1_PLLMIXED_LEN3		0xCC
#define MD1_PLLMIXED_BASE4		(0x0D0D4500)
#define MD1_PLLMIXED_LEN4		0xD0
#define MD1_PLLMIXED_BASE5		(0x0D0D4600)
#define MD1_PLLMIXED_LEN5		0xD0
#define MD1_PLLMIXED_BASE6		(0x0D0D4700)
#define MD1_PLLMIXED_LEN6		0x10
#define MD1_PLLMIXED_BASE7		(0x0D0D4840)
#define MD1_PLLMIXED_LEN7		0x90
#define MD1_PLLMIXED_BASE8		(0x0D0D4900)
#define MD1_PLLMIXED_LEN8		0x10
#define MD1_PLLMIXED_BASE9		(0x0D0D4C00)
#define MD1_PLLMIXED_LEN9		0x44
#define MD1_PLLMIXED_BASEA		(0x0D0D4F00)
#define MD1_PLLMIXED_LENA		0x8
/** MD CLKCTL **/
#define MD1_CLKCTL_BASE0			(0x0D0CE000)
#define MD1_CLKCTL_LEN0			0xB4
#define MD1_CLKCTL_BASE1			(0x0D0CE100)
#define MD1_CLKCTL_LEN1			0x3C
#define MD1_CLKCTL_BASE2			(0x0D0CE200)
#define MD1_CLKCTL_LEN2			0x44
#define MD1_CLKCTL_BASE3			(0x0D0CEF00)
#define MD1_CLKCTL_LEN3			0x8
/** MD GLOBALCON **/
#define MD1_GLOBALCON_BASE0		(0x0D0D5000)
#define MD1_GLOBALCON_LEN0		0xA0
#define MD1_GLOBALCON_BASE1		(0x0D0D5100)
#define MD1_GLOBALCON_LEN1		0x10
#define MD1_GLOBALCON_BASE2		(0x0D0D5400)
#define MD1_GLOBALCON_LEN2		0x4
#define MD1_GLOBALCON_BASE3		(0x0D0D5800)
#define MD1_GLOBALCON_LEN3		0x8
#define MD1_GLOBALCON_BASE4		(0x0D0D5C00)
#define MD1_GLOBALCON_LEN4		0x1C
#define MD1_GLOBALCON_BASE5		(0x0D0D5F00)
#define MD1_GLOBALCON_LEN5		0x30
/* BUS reg */
#define MD1_BUS_REG_BASE0		(0x0D0C6000)/* mdmcu_misc_reg */
#define MD1_BUS_REG_LEN0			0x310
#define MD1_BUS_REG_BASE1		(0x0D0CA000)/* mdinfra_misc_reg */
#define MD1_BUS_REG_LEN1			0x21C
#define MD1_BUS_REG_BASE2		(0x0D0C7000)/* cm2_misc */
#define MD1_BUS_REG_LEN2			0x120
#define MD1_BUS_REG_BASE3		(0x0D0EA200)/* modeml1_ao_config */
#define MD1_BUS_REG_LEN3			0xAC
#define MD1_BUS_REG_BASE4		(0x0D0EE200)/* l1_infra_config */
#define MD1_BUS_REG_LEN4			0x60
/* BUSREC */
#define MD1_MCU_MM_BUSREC_BASE0		(0x0D0C4000)
#define MD1_MCU_MM_BUSREC_LEN0		0x418

#define MD1_MCU_MO_BUSREC_STOP_SET_ADDR	(0x0D0C6804)
#define MD1_MCU_MO_BUSREC_STOP_VAL	0x1
#define MD1_MCU_MO_BUSREC_BASE0		(0x0D0C6800)
#define MD1_MCU_MO_BUSREC_LEN0		0x418
#define MD1_MCU_MO_BUSREC_RESTART_VAL	0x3

#define MD1_INFRA_BUSREC_STOP_SET_ADDR	(0x0D0C9004)
#define MD1_INFRA_BUSREC_STOP_VAL	0x1
#define MD1_INFRA_BUSREC_BASE0		(0x0D0C9000)
#define MD1_INFRA_BUSREC_LEN0		0x418
#define MD1_INFRA_BUSREC_RESTART_VAL	0x3

/* ECT */
#define MD1_ECT_REG_BASE0		(0x0D0CC130)/* MD ECT triggerIn/Out status */
#define MD1_ECT_REG_LEN0			0x8
#define MD1_ECT_REG_BASE1		(0x0D0EC130)/* ModemSys ECT triggerIn/Out status */
#define MD1_ECT_REG_LEN1			0x8
#define MD1_ECT_REG_BASE2		(0x0D0EA00C)/* MD32 ECT status */
#define MD1_ECT_REG_LEN2			0x4
#define MD1_ECT_REG_BASE3		(0x0D0EA014)/* MD32 ECT status */
#define MD1_ECT_REG_LEN3			0x4
/* TOPSM reg */
#define MD1_TOPSM_REG_BASE0		(0x0200D0000)
#define MD1_TOPSM_REG_LEN0		0xC2C
/* MD RGU reg */
#define MD1_RGU_REG_BASE0		(0x0200F0100)
#define MD1_RGU_REG_LEN0			0xC4
#define MD1_RGU_REG_BASE1		(0x0200F0300)
#define MD1_RGU_REG_LEN1			0x5C
/* OST status */
#define MD_OST_STATUS_BASE		0x200E0000
#define MD_OST_STATUS_LENGTH		0x228

/*MD bootup register*/
#define MD1_CFG_BASE (0x10260300)
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
