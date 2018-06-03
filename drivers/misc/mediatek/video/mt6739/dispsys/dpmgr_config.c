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

#if 0
unsigned int module_list_scenario[DDP_SCENARIO_MAX][DDP_ENING_NUM] = {
	{DISP_MODULE_OVL0, DISP_MODULE_COLOR0, DISP_MODULE_CCORR, DISP_MODULE_AAL,
		DISP_MODULE_GAMMA, DISP_MODULE_DITHER, DISP_MODULE_RDMA0, DISP_MODULE_UFOE, DISP_MODULE_DSI0, -1},
	{DISP_MODULE_OVL0, DISP_MODULE_WDMA0, -1, -1, -1, -1, -1, -1, -1, -1},
	{DISP_MODULE_OVL0, DISP_MODULE_WDMA0, DISP_MODULE_COLOR0, DISP_MODULE_CCORR,
		DISP_MODULE_AAL, DISP_MODULE_GAMMA, DISP_MODULE_DITHER,
		DISP_MODULE_RDMA0, DISP_MODULE_UFOE, DISP_MODULE_DSI0},
	{DISP_MODULE_RDMA0, DISP_MODULE_COLOR0, DISP_MODULE_CCORR, DISP_MODULE_AAL,
		DISP_MODULE_GAMMA, DISP_MODULE_DITHER, DISP_MODULE_UFOE, DISP_MODULE_DSI0, -1, -1},
	{DISP_MODULE_RDMA0, DISP_MODULE_DSI0, -1, -1, -1, -1, -1, -1, -1, -1},
	{DISP_MODULE_OVL1, DISP_MODULE_RDMA1, DISP_MODULE_DPI, -1, -1, -1, -1, -1, -1, -1},
	{DISP_MODULE_OVL1, DISP_MODULE_WDMA1, -1, -1, -1, -1, -1, -1, -1, -1},
	{DISP_MODULE_OVL1, DISP_MODULE_WDMA1, DISP_MODULE_RDMA1, DISP_MODULE_DPI, -1, -1, -1, -1, -1, -1},
	{DISP_MODULE_RDMA1, DISP_MODULE_DPI, -1, -1, -1, -1, -1, -1, -1, -1},
	{DISP_MODULE_OVL1, DISP_MODULE_OVL0, DISP_MODULE_WDMA0, -1, -1, -1, -1, -1, -1, -1},
};
#endif

#define	DSI_STATUS_REG_ADDR				0xF401B01C
#define	DSI_STATUS_IDLE_BIT			0x80000000

#define DSI_IRQ_BIT_RD_RDY			(0x1<<0)
#define	DSI_IRQ_BIT_CMD_DONE					(0x1<<1)
#define	DSI_IRQ_BIT_TE_RDY						(0x1<<2)
#define	DSI_IRQ_BIT_VM_DONE						(0x1<<3)
#define	DSI_IRQ_BIT_EXT_TE						(0x1<<4)
#define	DSI_IRQ_BIT_VM_CMD_DONE				(0x1<<5)
#define	DSI_IRQ_BIT_SLEEPOUT_DONE				(0x1<<6)

#define	RDMA0_IRQ_BIT_REG_UPDATE				(0x1<<0)
#define	RDMA0_IRQ_BIT_START						(0x1<<1)
#define	RDMA0_IRQ_BIT_DONE						(0x1<<2)
#define	RDMA0_IRQ_BIT_ABNORMAL					(0x1<<3)
#define	RDMA0_IRQ_BIT_UNDERFLOW				(0x1<<4)
#define	RDMA0_IRQ_BIT_TARGET_LINE				(0x1<<5)

#define	RDMA1_IRQ_BIT_REG_UPDATE				(0x1<<0)
#define	RDMA1_IRQ_BIT_START						(0x1<<1)
#define	RDMA1_IRQ_BIT_DONE						(0x1<<2)
#define	RDMA1_IRQ_BIT_ABNORMAL					(0x1<<3)
#define	RDMA1_IRQ_BIT_UNDERFLOW				(0x1<<4)
#define	RDMA1_IRQ_BIT_TARGET_LINE				(0x1<<5)

#define	RDMA2_IRQ_BIT_REG_UPDATE				(0x1<<0)
#define	RDMA2_IRQ_BIT_START						(0x1<<1)
#define	RDMA2_IRQ_BIT_DONE						(0x1<<2)
#define	RDMA2_IRQ_BIT_ABNORMAL					(0x1<<3)
#define	RDMA2_IRQ_BIT_UNDERFLOW				(0x1<<4)
#define	RDMA2_IRQ_BIT_TARGET_LINE				(0x1<<5)

#define		OVL0_IRQ_BIT_REG_UPDATE					(0x1<<0)
#define		OVL0_IRQ_BIT_REG_FRAME_DONE			(0x1<<1)
#define		OVL0_IRQ_BIT_REG_FRAME_UNDERFLOW		(0x1<<2)

#define		OVL1_IRQ_BIT_REG_UPDATE				(0x1<<0)
#define		OVL1_IRQ_BIT_REG_FRAME_DONE			(0x1<<1)
#define		OVL1_IRQ_BIT_REG_FRAME_UNDERFLOW		(0x1<<2)


