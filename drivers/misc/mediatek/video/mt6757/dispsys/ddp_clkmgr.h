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


enum DDP_CLK_ID {
	DISP0_SMI_COMMON = 0,
	DISP0_SMI_LARB0,
	DISP0_SMI_LARB4,
	DISP0_DISP_OVL0,
	DISP0_DISP_OVL1,
	DISP0_DISP_OVL0_2L,
	DISP0_DISP_OVL1_2L,
	DISP0_DISP_RDMA0,
	DISP0_DISP_RDMA1,
	DISP0_DISP_RDMA2,
	DISP0_DISP_WDMA0,
	DISP0_DISP_WDMA1,
	DISP0_DISP_COLOR,
	DISP0_DISP_COLOR1,
	DISP0_DISP_CCORR,
	DISP0_DISP_CCORR1,
	DISP0_DISP_AAL,
	DISP0_DISP_AAL1,
	DISP0_DISP_GAMMA,
	DISP0_DISP_GAMMA1,
	DISP0_DISP_OD,
	DISP0_DISP_DITHER,
	DISP0_DISP_DITHER1,
	DISP0_DISP_UFOE,
	DISP0_DISP_DSC,
	DISP0_DISP_SPLIT,
	DISP1_DSI0_MM_CLOCK,
	DISP1_DSI0_INTERFACE_CLOCK,
	DISP1_DSI1_MM_CLOCK,
	DISP1_DSI1_INTERFACE_CLOCK,
	DISP1_DPI_MM_CLOCK,
	DISP1_DPI_INTERFACE_CLOCK,
	DISP1_DISP_OVL0_MOUT,
	DISP_PWM,
	DISP_MTCMOS_CLK,
	MUX_DPI0,
	TVDPLL_D2,
	TVDPLL_D4,
	TVDPLL_D8,
	TVDPLL_D16,
	DPI_CK,
	MUX_PWM,
	CLK26M,
	UNIVPLL2_D4,
	ULPOSC_D4,
	ULPOSC_D8,
	MUX_MM,
	MM_VENCPLL,
	SYSPLL2_D2,
	MAX_DISP_CLK_CNT
};

#ifndef CONFIG_MTK_CLKMGR

int ddp_clk_prepare(enum DDP_CLK_ID id);
int ddp_clk_unprepare(enum DDP_CLK_ID id);
int ddp_clk_enable(enum DDP_CLK_ID id);
int ddp_clk_disable(enum DDP_CLK_ID id);
int ddp_clk_prepare_enable(enum DDP_CLK_ID id);
int ddp_clk_disable_unprepare(enum DDP_CLK_ID id);
int ddp_clk_set_parent(enum DDP_CLK_ID id, enum DDP_CLK_ID parent);
int ddp_set_clk_handle(struct clk *pclk, unsigned int n);
int ddp_parse_apmixed_base(void);
int ddp_set_mipi26m(enum DISP_MODULE_ENUM module, int en);

#endif				/* CONFIG_MTK_CLKMGR */

#endif
