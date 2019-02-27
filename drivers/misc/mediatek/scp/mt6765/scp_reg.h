/*
 * Copyright (C) 2017 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#ifndef __SCP_REG_H
#define __SCP_REG_H

/*#define SCP_BASE						(scpreg.cfg)*/
#define SCP_AP_RESOURCE		(scpreg.cfg + 0x0004)
#define SCP_BUS_RESOURCE	(scpreg.cfg + 0x0008)

#define SCP_A_TO_HOST_REG			(scpreg.cfg + 0x001C)
	#define SCP_IRQ_SCP2HOST     (1 << 0)
	#define SCP_IRQ_WDT          (1 << 8)

#define SCP_TO_SPM_REG           (scpreg.cfg + 0x0020)

#define SCP_GIPC_IN_REG					(scpreg.cfg + 0x0028)
	#define HOST_TO_SCP_A       (1 << 0)
	#define HOST_TO_SCP_B       (1 << 1)
	/* scp awake lock definition*/
	#define SCP_A_IPI_AWAKE_NUM		(2)
	#define SCP_B_IPI_AWAKE_NUM		(3)


#define SCP_A_DEBUG_PC_REG       (scpreg.cfg + 0x00B4)
#define SCP_A_DEBUG_PSP_REG      (scpreg.cfg + 0x00B0)
#define SCP_A_DEBUG_LR_REG       (scpreg.cfg + 0x00AC)
#define SCP_A_DEBUG_SP_REG       (scpreg.cfg + 0x00A8)
#define SCP_A_WDT_REG            (scpreg.cfg + 0x0084)

#define SCP_A_GENERAL_REG0       (scpreg.cfg + 0x0050)
#define SCP_A_GENERAL_REG1       (scpreg.cfg + 0x0054)
#define SCP_A_GENERAL_REG2       (scpreg.cfg + 0x0058)
#define SCP_A_GENERAL_REG3       (scpreg.cfg + 0x005C)
#define EXPECTED_FREQ_REG        (scpreg.cfg  + 0x5C)
#define SCP_A_GENERAL_REG4       (scpreg.cfg + 0x0060)
#define CURRENT_FREQ_REG         (scpreg.cfg  + 0x60)
#define SCP_A_GENERAL_REG5       (scpreg.cfg + 0x0064)
#define SCP_GPR_CM4_A_REBOOT     (scpreg.cfg + 0x64)
	#define CM4_A_READY_TO_REBOOT  0x34
	#define CM4_A_REBOOT_OK        0x1

#define SCP_SEMAPHORE	         (scpreg.cfg  + 0x90)

#define SCP_SLP_PROTECT_CFG			(scpreg.cfg + 0x00C8)

#define SCP_CPU_SLEEP_STATUS			(scpreg.cfg + 0x0114)
	#define SCP_A_DEEP_SLEEP_BIT	(1)
	#define SCP_B_DEEP_SLEEP_BIT	(3)

#define SCP_SLEEP_STATUS_REG     (scpreg.cfg + 0x0114)
	#define SCP_A_IS_SLEEP          (1<<0)
	#define SCP_A_IS_DEEPSLEEP      (1<<1)
	#define SCP_B_IS_SLEEP          (1<<2)
	#define SCP_B_IS_DEEPSLEEP      (1<<3)

/* clk reg*/
#define SCP_CLK_CTRL_BASE				(scpreg.clkctrl)
#define SCP_CLK_SW_SEL				(scpreg.clkctrl)
#define SCP_CLK_ENABLE				(scpreg.clkctrl + 0x0004)
#define SCP_A_SLEEP_DEBUG_REG			(scpreg.clkctrl + 0x0028)
#define SCP_CLK_HIGH_CORE_CG		(scpreg.clkctrl + 0x005C)

/* INFRA_IRQ */
#define INFRA_IRQ_SET			(scpreg.scpsys + 0x0A20)
	#define AP_AWAKE_LOCK		(0)
	#define AP_AWAKE_UNLOCK		(1)
	#define CONNSYS_AWAKE_LOCK	(2)
	#define CONNSYS_AWAKE_UNLOCK	(3)
#define INFRA_IRQ_CLEAR			(scpreg.scpsys + 0x0A24)

#define SCP_SCP2SPM_VOL_LV		(scpreg.cfg + 0x0094)

#endif
