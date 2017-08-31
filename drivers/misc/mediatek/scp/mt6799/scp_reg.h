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

#ifndef __SCP_REG_H
#define __SCP_REG_H

#define SCP_BASE						(scpreg.cfg)

#define SCP_A_TO_HOST_REG				(*(volatile unsigned int *)(scpreg.cfg + 0x001C))
	#define SCP_IRQ_SCP2HOST     (1 << 0)
	#define SCP_IRQ_WDT          (1 << 8)

#define SCP_TO_SPM_REG           (*(volatile unsigned int *)(scpreg.cfg + 0x0020))

#define HOST_TO_SCP_REG          (*(volatile unsigned int *)(scpreg.cfg + 0x0024))

#define GIPC_TO_SCP_REG					(*(volatile unsigned int *)(scpreg.cfg + 0x0028))
	#define HOST_TO_SCP_A       (1 << 0)
	#define HOST_TO_SCP_B       (1 << 1)

#define SCP_GIPC_REG                    (scpreg.cfg + 0x0028)
	/* scp awake lock definition*/
	#define SCP_A_IPI_AWAKE_NUM		(2)
	#define SCP_B_IPI_AWAKE_NUM		(3)

#define SCP_A_DEBUG_PC_REG       (*(volatile unsigned int *)(scpreg.cfg + 0x00B4))
#define SCP_A_DEBUG_PSP_REG      (*(volatile unsigned int *)(scpreg.cfg + 0x00B0))
#define SCP_A_DEBUG_LR_REG       (*(volatile unsigned int *)(scpreg.cfg + 0x00AC))
#define SCP_A_DEBUG_SP_REG       (*(volatile unsigned int *)(scpreg.cfg + 0x00A8))
#define SCP_A_WDT_REG            (scpreg.cfg + 0x0084)

#define SCP_A_GENERAL_REG0       (*(volatile unsigned int *)(scpreg.cfg + 0x0050))
#define SCP_A_GENERAL_REG1       (*(volatile unsigned int *)(scpreg.cfg + 0x0054))
#define SCP_A_GENERAL_REG2       (*(volatile unsigned int *)(scpreg.cfg + 0x0058))
#define SCP_A_GENERAL_REG3       (*(volatile unsigned int *)(scpreg.cfg + 0x005C))
#define EXPECTED_FREQ_REG        (SCP_BASE  + 0x5C)
#define SCP_A_GENERAL_REG4       (*(volatile unsigned int *)(scpreg.cfg + 0x0060))
#define CURRENT_FREQ_REG         (SCP_BASE  + 0x60)
#define SCP_A_GENERAL_REG5       (*(volatile unsigned int *)(scpreg.cfg + 0x0064))
#define SCP_GPR_CM4_A_REBOOT     (SCP_BASE + 0x64)
#define CM4_A_READY_TO_REBOOT  0x34
#define SCP_A_GENERAL_REG6       (*(volatile unsigned int *)(scpreg.cfg + 0x0068))
#define SCP_GPR_CM4_B_REBOOT     (SCP_BASE + 0x68)
#define CM4_B_READY_TO_REBOOT  0x35

#define SCP_A_GENERAL_REG7       (*(volatile unsigned int *)(scpreg.cfg + 0x006C))
#define SCP_SEMAPHORE            (scpreg.cfg  + 0x90)
#define SCP_SEMAPHORE_3WAY       (scpreg.cfg  + 0x94)

#define SCP_CPU_SLEEP_STATUS			(scpreg.cfg + 0x0114)
	#define SCP_A_DEEP_SLEEP_BIT	(1)
	#define SCP_B_DEEP_SLEEP_BIT	(3)

#define SCP_SLEEP_STATUS_REG     (*(volatile unsigned int *)(scpreg.cfg + 0x0114))
	#define SCP_A_IS_SLEEP          (1<<0)
	#define SCP_A_IS_DEEPSLEEP      (1<<1)
	#define SCP_B_IS_SLEEP          (1<<2)
	#define SCP_B_IS_DEEPSLEEP      (1<<3)

#define SCP_B_TO_HOST_REG        (*(volatile unsigned int *)(scpreg.cfg + 0x021C))
#define SCP_B_DEBUG_PC_REG       (*(volatile unsigned int *)(scpreg.cfg + 0x02B4))
#define SCP_B_DEBUG_PSP_REG      (*(volatile unsigned int *)(scpreg.cfg + 0x02B0))
#define SCP_B_DEBUG_LR_REG       (*(volatile unsigned int *)(scpreg.cfg + 0x02AC))
#define SCP_B_DEBUG_SP_REG       (*(volatile unsigned int *)(scpreg.cfg + 0x02A8))
#define SCP_B_WDT_REG            (scpreg.cfg + 0x0284)

#define SCP_B_GENERAL_REG0       (*(volatile unsigned int *)(scpreg.cfg + 0x0250))
#define SCP_B_GENERAL_REG1       (*(volatile unsigned int *)(scpreg.cfg + 0x0254))
#define SCP_B_GENERAL_REG2       (*(volatile unsigned int *)(scpreg.cfg + 0x0258))
#define SCP_B_GENERAL_REG3       (*(volatile unsigned int *)(scpreg.cfg + 0x025C))
#define SCP_B_GENERAL_REG4       (*(volatile unsigned int *)(scpreg.cfg + 0x0260))
#define SCP_B_GENERAL_REG5       (*(volatile unsigned int *)(scpreg.cfg + 0x0264))
#define SCP_B_GENERAL_REG6       (*(volatile unsigned int *)(scpreg.cfg + 0x0268))
#define SCP_B_GENERAL_REG7       (*(volatile unsigned int *)(scpreg.cfg + 0x026C))


#define SCP_BASE_DUAL					(scpreg.cfg + 0x200)


#define SCP_CLK_CTRL_BASE				(scpreg.clkctrl)
#define SCP_CLK_CTRL_DUAL_BASE			(scpreg.clkctrldual)
#define SCP_A_SLEEP_DEBUG_REG    (*(volatile unsigned int *)(scpreg.clkctrl + 0x0028))
#define SCP_B_SLEEP_DEBUG_REG    (*(volatile unsigned int *)(scpreg.clkctrldual + 0x0028))

#define EXPECTED_FREQ_REG (SCP_BASE  + 0x5C)
#define CURRENT_FREQ_REG  (SCP_BASE  + 0x60)

#endif
