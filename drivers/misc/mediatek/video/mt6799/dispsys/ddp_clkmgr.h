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

#ifndef __DDP_CLK_MGR_H__
#define __DDP_CLK_MGR_H__

#include "ddp_hal.h"

#ifndef CONFIG_MTK_CLKMGR
#include <linux/clk.h>
#endif


#define DDP_CLK_SMI_NUM (8)

enum DDP_CLK_ID {
	DISP_MTCMOS_CLK = 0,
	DISP0_SMI_COMMON,
	DISP0_SMI_COMMON_2X,
	GALS_M0_2X,
	GALS_M1_2X,
	UPSZ0,
	UPSZ1,
	FIFO0,
	FIFO1,
	DISP0_SMI_LARB0,
	DISP0_SMI_LARB1,
	DISP0_DISP_OVL0,
	DISP0_DISP_OVL1,
	DISP0_DISP_OVL0_2L,
	DISP0_DISP_OVL1_2L,
	DISP0_DISP_RDMA0,
	DISP0_DISP_RDMA1,
	DISP0_DISP_RDMA2,
	DISP0_DISP_WDMA0,
	DISP0_DISP_WDMA1,
	DISP0_DISP_COLOR0,
	DISP0_DISP_COLOR1,
	DISP0_DISP_CCORR0,
	DISP0_DISP_CCORR1,
	DISP0_DISP_AAL0,
	DISP0_DISP_AAL1,
	DISP0_DISP_GAMMA0,
	DISP0_DISP_GAMMA1,
	DISP0_DISP_OD,
	DISP0_DISP_DITHER0,
	DISP0_DISP_DITHER1,
	DISP0_DISP_UFOE,
	DISP1_DPI_INTERFACE_CLOCK,
	DISP0_DISP_DSC,
	DISP0_DISP_SPLIT,
	DISP1_DISP_OVL0_MOUT,
	DISP0_DISP_RSZ0,
	DISP0_DISP_RSZ1,
	DISP1_DSI0_MM_CLOCK,
	DISP1_DSI0_INTERFACE_CLOCK,
	DISP1_DSI1_MM_CLOCK,
	DISP1_DSI1_INTERFACE_CLOCK,
	DISP1_DPI_MM_CLOCK,
	DISP1_FAKE_ENG2,
	DISP1_FAKE_ENG,
	MDP_WROT0,
	MDP_WROT1,
	MUX_DPI0,
	TVDPLL_D2,
	TVDPLL_D4,
	TVDPLL_D8,
	TVDPLL_D16,
	DPI_CK,
	DISP_CLK_PWM0,
	DISP_CLK_PWM1,
	MUX_PWM,
	CLK26M,
	ULPOSC_D2,
	ULPOSC_D8,
	UNIVPLL2_D4,
	MAX_DISP_CLK_CNT
};

#ifndef CONFIG_MTK_CLKMGR

int ddp_clk_is_exist(enum DISP_MODULE_ENUM module);
int ddp_clk_cnt(enum DISP_MODULE_ENUM module);
int ddp_clk_prepare(enum DDP_CLK_ID id);
int ddp_clk_unprepare(enum DDP_CLK_ID id);
int ddp_clk_enable(enum DDP_CLK_ID id);
int ddp_clk_disable(enum DDP_CLK_ID id);
int ddp_clk_prepare_enable(enum DDP_CLK_ID id);
int ddp_clk_disable_unprepare(enum DDP_CLK_ID id);
int ddp_clk_set_parent(enum DDP_CLK_ID id, enum DDP_CLK_ID parent);
int ddp_clk_enable_by_module(enum DISP_MODULE_ENUM module);
int ddp_clk_disable_by_module(enum DISP_MODULE_ENUM module);
int ddp_set_clk_handle(struct clk *pclk, unsigned int n);
int ddp_parse_apmixed_base(void);
int ddp_set_mipi26m(enum DISP_MODULE_ENUM module, int en);
int ddp_get_mipi26m(void);

#endif				/* CONFIG_MTK_CLKMGR */

#endif
