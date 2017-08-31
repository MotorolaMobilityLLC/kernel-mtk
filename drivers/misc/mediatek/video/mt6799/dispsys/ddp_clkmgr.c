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


#include "ddp_clkmgr.h"
#include "ddp_log.h"
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/types.h>
#include "mt-plat/sync_write.h"

#ifndef CONFIG_MTK_CLKMGR

#define READ_REGISTER_UINT32(reg)       (*(uint32_t * const)(reg))
#define INREG32(x)          READ_REGISTER_UINT32((uint32_t *)((void *)(x)))
#define DRV_Reg32(addr) INREG32(addr)
#define clk_readl(addr) DRV_Reg32(addr)
#define clk_writel(addr, val) mt_reg_sync_writel(val, addr)
#define clk_setl(addr, val) mt_reg_sync_writel(clk_readl(addr) | (val), addr)
#define clk_clrl(addr, val) mt_reg_sync_writel(clk_readl(addr) & ~(val), addr)

static struct clk *ddp_clk[MAX_DISP_CLK_CNT];
static int _ddp_clk_cnt[MAX_DISP_CLK_CNT];

static void __iomem *ddp_apmixed_base;
#ifndef AP_PLL_CON0
#define AP_PLL_CON0 (ddp_apmixed_base + 0x00)
#endif


unsigned int parsed_apmixed;

/* module can be connect if 1 */
static int ddp_clk_exist[DISP_MODULE_NUM] = {
	1, /* DISP_MODULE_OVL0 = 0, */
	1, /* DISP_MODULE_OVL1, */
	1, /* DISP_MODULE_OVL0_2L, */
	1, /* DISP_MODULE_OVL1_2L, */
	0, /* DISP_MODULE_OVL0_VIRTUAL, */
	0, /* DISP_MODULE_OVL0_2L_VIRTUAL, */
	0, /* DISP_MODULE_OVL1_2L_VIRTUAL, */
	1, /* DISP_MODULE_RDMA0, */
	1, /* DISP_MODULE_RDMA1, */
	1, /* DISP_MODULE_RDMA2, */
	1, /* DISP_MODULE_WDMA0, */
	1, /* DISP_MODULE_WDMA1, */
	1, /* DISP_MODULE_COLOR0, */
	1, /* DISP_MODULE_COLOR1, */
	1, /* DISP_MODULE_CCORR0, */
	1, /* DISP_MODULE_CCORR1, */
	1, /* DISP_MODULE_AAL0, */
	1, /* DISP_MODULE_AAL1, */
	1, /* DISP_MODULE_GAMMA0, */
	1, /* DISP_MODULE_GAMMA1, */
	1, /* DISP_MODULE_OD, */
	1, /* DISP_MODULE_DITHER0, */
	1, /* DISP_MODULE_DITHER1, */
	0, /* DISP_PATH0, */
	0, /* DISP_PATH1, */
	1, /* DISP_MODULE_UFOE, */
	1, /* DISP_MODULE_DSC, */
	1, /* DISP_MODULE_DSC_2ND, */
	1, /* DISP_MODULE_SPLIT0, */
	1, /* DISP_MODULE_DPI, */
	1, /* DISP_MODULE_DSI0, */
	1, /* DISP_MODULE_DSI1, */
	1, /* DISP_MODULE_DSIDUAL, */
	1, /* DISP_MODULE_PWM0, */
	1, /* DISP_MODULE_PWM1, */
	0, /* DISP_MODULE_CONFIG, */
	0, /* DISP_MODULE_MUTEX, */
	1, /* DISP_MODULE_SMI_COMMON, */
	1, /* DISP_MODULE_SMI_LARB0, */
	1, /* DISP_MODULE_SMI_LARB1, */
	0, /* DISP_MODULE_MIPI0, */
	0, /* DISP_MODULE_MIPI1, */
	1, /* DISP_MODULE_RSZ0, */
	1, /* DISP_MODULE_RSZ1, */
	1, /* DISP_MODULE_MTCMOS, */
	1, /* DISP_MODULE_FAKE_ENG, */
	0, /* DISP_MODULE_CLOCK_MUX, */
	0, /* DISP_MODULE_UNKNOWN, */
};

static int ddp_clk_smi_map[] = {
	DISP0_SMI_COMMON,
	DISP0_SMI_COMMON_2X,
	GALS_M0_2X,
	GALS_M1_2X,
	UPSZ0,
	UPSZ1,
	FIFO0,
	FIFO1,
};

int ddp_clk_is_exist(enum DISP_MODULE_ENUM module)
{
	return ddp_clk_exist[module];
}

int ddp_clk_cnt(enum DISP_MODULE_ENUM module)
{
	int i;
	int err = 0;

	switch (module) {
	case DISP_MODULE_OVL0:
		if (_ddp_clk_cnt[DISP0_DISP_OVL0] != _ddp_clk_cnt[DISP1_DISP_OVL0_MOUT]) {
			DDPERR("%d clk mismatch 0x%x, 0x%x\n", module,
			       _ddp_clk_cnt[DISP0_DISP_OVL0], _ddp_clk_cnt[DISP1_DISP_OVL0_MOUT]);
			return -1;
		}
		return _ddp_clk_cnt[DISP0_DISP_OVL0];
	case DISP_MODULE_OVL1:
		return _ddp_clk_cnt[DISP0_DISP_OVL1];
	case DISP_MODULE_OVL0_2L:
		return _ddp_clk_cnt[DISP0_DISP_OVL0_2L];
	case DISP_MODULE_OVL1_2L:
		return _ddp_clk_cnt[DISP0_DISP_OVL1_2L];
	case DISP_MODULE_RDMA0:
		return _ddp_clk_cnt[DISP0_DISP_RDMA0];
	case DISP_MODULE_RDMA1:
		return _ddp_clk_cnt[DISP0_DISP_RDMA1];
	case DISP_MODULE_RDMA2:
		return _ddp_clk_cnt[DISP0_DISP_RDMA2];
	case DISP_MODULE_WDMA0:
		return _ddp_clk_cnt[DISP0_DISP_WDMA0];
	case DISP_MODULE_WDMA1:
		return _ddp_clk_cnt[DISP0_DISP_WDMA1];
	case DISP_MODULE_COLOR0:
		return _ddp_clk_cnt[DISP0_DISP_COLOR0];
	case DISP_MODULE_COLOR1:
		return _ddp_clk_cnt[DISP0_DISP_COLOR1];
	case DISP_MODULE_CCORR0:
		return _ddp_clk_cnt[DISP0_DISP_CCORR0];
	case DISP_MODULE_CCORR1:
		return _ddp_clk_cnt[DISP0_DISP_CCORR1];
	case DISP_MODULE_AAL0:
		return _ddp_clk_cnt[DISP0_DISP_AAL0];
	case DISP_MODULE_AAL1:
		return _ddp_clk_cnt[DISP0_DISP_AAL1];
	case DISP_MODULE_GAMMA0:
		return _ddp_clk_cnt[DISP0_DISP_GAMMA0];
	case DISP_MODULE_GAMMA1:
		return _ddp_clk_cnt[DISP0_DISP_GAMMA1];
	case DISP_MODULE_OD:
		return _ddp_clk_cnt[DISP0_DISP_OD];
	case DISP_MODULE_DITHER0:
		return _ddp_clk_cnt[DISP0_DISP_DITHER0];
	case DISP_MODULE_DITHER1:
		return _ddp_clk_cnt[DISP0_DISP_DITHER1];
	case DISP_PATH0:
		return -1;
	case DISP_PATH1:
		return -1;
	case DISP_MODULE_UFOE:
		return _ddp_clk_cnt[DISP0_DISP_UFOE];
	case DISP_MODULE_DSC:
		return _ddp_clk_cnt[DISP0_DISP_DSC];
	case DISP_MODULE_DSC_2ND:
		return _ddp_clk_cnt[DISP0_DISP_DSC];
	case DISP_MODULE_SPLIT0:
		return _ddp_clk_cnt[DISP0_DISP_SPLIT];
	case DISP_MODULE_DPI:
		if (_ddp_clk_cnt[DISP1_DPI_MM_CLOCK] != _ddp_clk_cnt[DISP1_DPI_INTERFACE_CLOCK]) {
			DDPERR("%d clk mismatch 0x%x, 0x%x\n", module,
			       _ddp_clk_cnt[DISP1_DPI_MM_CLOCK],
			       _ddp_clk_cnt[DISP1_DPI_INTERFACE_CLOCK]);
			return -1;
		}
		return _ddp_clk_cnt[DISP1_DPI_MM_CLOCK];
	case DISP_MODULE_DSI0:
		if (_ddp_clk_cnt[DISP1_DSI0_MM_CLOCK] != _ddp_clk_cnt[DISP1_DSI0_INTERFACE_CLOCK]) {
			DDPERR("%d clk mismatch 0x%x, 0x%x\n", module,
			       _ddp_clk_cnt[DISP1_DSI0_MM_CLOCK],
			       _ddp_clk_cnt[DISP1_DSI0_INTERFACE_CLOCK]);
			return -1;
		}
		return _ddp_clk_cnt[DISP1_DSI0_MM_CLOCK];
	case DISP_MODULE_DSI1:
		if (_ddp_clk_cnt[DISP1_DSI1_MM_CLOCK] != _ddp_clk_cnt[DISP1_DSI1_INTERFACE_CLOCK]) {
			DDPERR("%d clk mismatch 0x%x, 0x%x\n", module,
			       _ddp_clk_cnt[DISP1_DSI1_MM_CLOCK],
			       _ddp_clk_cnt[DISP1_DSI1_INTERFACE_CLOCK]);
			return -1;
		}
		return _ddp_clk_cnt[DISP1_DSI1_MM_CLOCK];
	case DISP_MODULE_DSIDUAL:
		if (_ddp_clk_cnt[DISP1_DSI0_MM_CLOCK] != _ddp_clk_cnt[DISP1_DSI0_INTERFACE_CLOCK]) {
			DDPERR("%d clk mismatch 0x%x, 0x%x\n", module,
			       _ddp_clk_cnt[DISP1_DSI0_MM_CLOCK],
			       _ddp_clk_cnt[DISP1_DSI0_INTERFACE_CLOCK]);
			err++;
		}
		if (_ddp_clk_cnt[DISP1_DSI1_MM_CLOCK] != _ddp_clk_cnt[DISP1_DSI1_INTERFACE_CLOCK]) {
			DDPERR("%d clk mismatch 0x%x, 0x%x\n", module,
			       _ddp_clk_cnt[DISP1_DSI1_MM_CLOCK],
			       _ddp_clk_cnt[DISP1_DSI1_INTERFACE_CLOCK]);
			err++;
		}
		if (err) {
			DDPERR("%d %d times clk mismatch!\n", module, err);
			return -1;
		}
		return _ddp_clk_cnt[DISP1_DSI0_MM_CLOCK];
	case DISP_MODULE_PWM0:
		return _ddp_clk_cnt[DISP_CLK_PWM0];
	case DISP_MODULE_PWM1:
		return _ddp_clk_cnt[DISP_CLK_PWM1];
	case DISP_MODULE_CONFIG:
		return -1;
	case DISP_MODULE_MUTEX:
		return -1;
	case DISP_MODULE_SMI_COMMON:
		for (i = 0; i < (DDP_CLK_SMI_NUM - 1); i++) {
			if (_ddp_clk_cnt[ddp_clk_smi_map[i]] !=
			    _ddp_clk_cnt[ddp_clk_smi_map[i + 1]]) {
				DDPERR("%d clk mismatch 0x%x, 0x%x\n", module,
				       _ddp_clk_cnt[ddp_clk_smi_map[i]],
				       _ddp_clk_cnt[ddp_clk_smi_map[i + 1]]);
				err++;
			}
		}
		if (err) {
			DDPERR("%d %d times clk mismatch!\n", module, err);
			return -1;
		}
		return _ddp_clk_cnt[ddp_clk_smi_map[0]];
	case DISP_MODULE_SMI_LARB0:
		return _ddp_clk_cnt[DISP0_SMI_LARB0];
	case DISP_MODULE_SMI_LARB1:
		return _ddp_clk_cnt[DISP0_SMI_LARB1];
	case DISP_MODULE_MIPI0:
		return -1;
	case DISP_MODULE_MIPI1:
		return -1;
	case DISP_MODULE_RSZ0:
		return _ddp_clk_cnt[DISP0_DISP_RSZ0];
	case DISP_MODULE_RSZ1:
		return _ddp_clk_cnt[DISP0_DISP_RSZ1];
	case DISP_MODULE_OVL0_VIRTUAL:
		return -1;
	case DISP_MODULE_OVL0_2L_VIRTUAL:
		return -1;
	case DISP_MODULE_OVL1_2L_VIRTUAL:
		return -1;
	case DISP_MODULE_MTCMOS:
		return _ddp_clk_cnt[DISP_MTCMOS_CLK];
	case DISP_MODULE_FAKE_ENG:
		if (_ddp_clk_cnt[DISP1_FAKE_ENG] != _ddp_clk_cnt[DISP1_FAKE_ENG2]) {
			DDPERR("%d clk mismatch 0x%x, 0x%x\n", module,
			       _ddp_clk_cnt[DISP1_FAKE_ENG],
			       _ddp_clk_cnt[DISP1_FAKE_ENG2]);
			return -1;
		}
		return _ddp_clk_cnt[DISP1_FAKE_ENG];
	case DISP_MODULE_CLOCK_MUX:
		return -1;
	default:
		DDPERR("invalid module id=%d\n", module);
		return -1;
	}
}

int ddp_set_clk_handle(struct clk *pclk, unsigned int n)
{
	int ret = 0;

	if (n >= MAX_DISP_CLK_CNT) {
		DDPERR("DISPSYS CLK id=%d is more than MAX_DISP_CLK_CNT\n", n);
		return -1;
	}
	ddp_clk[n] = pclk;
	DDPMSG("ddp_clk[%d] %p\n", n, ddp_clk[n]);
	return ret;
}

int ddp_clk_prepare(enum DDP_CLK_ID id)
{
	int ret = 0;
	int before, after;

	if (ddp_clk[id] == NULL) {
		DDPERR("DISPSYS CLK %d NULL\n", id);
		return -1;
	}

	before = _ddp_clk_cnt[id];
	_ddp_clk_cnt[id] = _ddp_clk_cnt[id] + (0x1 << 16);
	ret = clk_prepare(ddp_clk[id]);
	after = _ddp_clk_cnt[id];

	DDPMSG("%s %d:%d,%d\n", __func__, id, before, after);
	if (ret)
		DDPERR("DISPSYS CLK prepare failed: errno %d id %d\n", ret, id);

	return ret;
}

int ddp_clk_unprepare(enum DDP_CLK_ID id)
{
	int ret = 0;
	int before, after;

	if (ddp_clk[id] == NULL) {
		DDPERR("DISPSYS CLK %d NULL\n", id);
		return -1;
	}

	before = _ddp_clk_cnt[id];
	_ddp_clk_cnt[id] = _ddp_clk_cnt[id] - (0x1 << 16);
	clk_unprepare(ddp_clk[id]);
	after = _ddp_clk_cnt[id];

	DDPMSG("%s %d:%d,%d\n", __func__, id, before, after);

	return ret;
}

int ddp_clk_enable(enum DDP_CLK_ID id)
{
	int ret = 0;
	int before, after;

	if (ddp_clk[id] == NULL) {
		DDPERR("DISPSYS CLK %d NULL\n", id);
		return -1;
	}

	before = _ddp_clk_cnt[id];
	_ddp_clk_cnt[id] = _ddp_clk_cnt[id] + 0x1;
	ret = clk_enable(ddp_clk[id]);
	after = _ddp_clk_cnt[id];

	DDPMSG("%s %d:%d,%d\n", __func__, id, before, after);
	if (ret)
		DDPERR("DISPSYS CLK enable failed: errno %d id=%d\n", ret, id);

	return ret;
}

int ddp_clk_disable(enum DDP_CLK_ID id)
{
	int ret = 0;
	int before, after;

	if (ddp_clk[id] == NULL) {
		DDPERR("DISPSYS CLK %d NULL\n", id);
		return -1;
	}

	before = _ddp_clk_cnt[id];
	_ddp_clk_cnt[id] = _ddp_clk_cnt[id] - 0x1;
	clk_disable(ddp_clk[id]);
	after = _ddp_clk_cnt[id];

	DDPMSG("%s %d:%d,%d\n", __func__, id, before, after);

	return ret;
}

int ddp_clk_prepare_enable(enum DDP_CLK_ID id)
{
	int ret = 0;
	int before, after;

	if (ddp_clk[id] == NULL) {
		DDPERR("DISPSYS CLK %d NULL\n", id);
		return -1;
	}

	before = _ddp_clk_cnt[id];
	_ddp_clk_cnt[id] = _ddp_clk_cnt[id] + (0x1 << 16) + 0x1;
	ret = clk_prepare_enable(ddp_clk[id]);
	after = _ddp_clk_cnt[id];

	DDPMSG("%s %d:%d,%d\n", __func__, id, before, after);
	if (ret)
		DDPERR("DISPSYS CLK prepare failed: errno %d\n", ret);

	return ret;
}

int ddp_clk_disable_unprepare(enum DDP_CLK_ID id)
{
	int ret = 0;
	int before, after;

	if (ddp_clk[id] == NULL) {
		DDPERR("DISPSYS CLK %d NULL\n", id);
		return -1;
	}

	before = _ddp_clk_cnt[id];
	_ddp_clk_cnt[id] = _ddp_clk_cnt[id] - (0x1 << 16) - 0x1;
	clk_disable_unprepare(ddp_clk[id]);
	after = _ddp_clk_cnt[id];

	DDPMSG("%s %d:%d,%d\n", __func__, id, before, after);

	return ret;
}

int ddp_clk_enable_by_module(enum DISP_MODULE_ENUM module)
{
	int i;
	int ret = 0;

	switch (module) {
	case DISP_MODULE_OVL0:
		ddp_clk_enable(DISP0_DISP_OVL0);
		ddp_clk_enable(DISP1_DISP_OVL0_MOUT);
		break;
	case DISP_MODULE_OVL1:
		ddp_clk_enable(DISP0_DISP_OVL1);
		break;
	case DISP_MODULE_OVL0_2L:
		ddp_clk_enable(DISP0_DISP_OVL0_2L);
		break;
	case DISP_MODULE_OVL1_2L:
		ddp_clk_enable(DISP0_DISP_OVL1_2L);
		break;
	case DISP_MODULE_RDMA0:
		ddp_clk_enable(DISP0_DISP_RDMA0);
		break;
	case DISP_MODULE_RDMA1:
		ddp_clk_enable(DISP0_DISP_RDMA1);
		break;
	case DISP_MODULE_RDMA2:
		ddp_clk_enable(DISP0_DISP_RDMA2);
		break;
	case DISP_MODULE_WDMA0:
		ddp_clk_enable(DISP0_DISP_WDMA0);
		break;
	case DISP_MODULE_WDMA1:
		ddp_clk_enable(DISP0_DISP_WDMA1);
		break;
	case DISP_MODULE_COLOR0:
		ddp_clk_enable(DISP0_DISP_COLOR0);
		break;
	case DISP_MODULE_COLOR1:
		ddp_clk_enable(DISP0_DISP_COLOR1);
		break;
	case DISP_MODULE_CCORR0:
		ddp_clk_enable(DISP0_DISP_CCORR0);
		break;
	case DISP_MODULE_CCORR1:
		ddp_clk_enable(DISP0_DISP_CCORR1);
		break;
	case DISP_MODULE_AAL0:
		ddp_clk_enable(DISP0_DISP_AAL0);
		break;
	case DISP_MODULE_AAL1:
		ddp_clk_enable(DISP0_DISP_AAL1);
		break;
	case DISP_MODULE_GAMMA0:
		ddp_clk_enable(DISP0_DISP_GAMMA0);
		break;
	case DISP_MODULE_GAMMA1:
		ddp_clk_enable(DISP0_DISP_GAMMA1);
		break;
	case DISP_MODULE_OD:
		ddp_clk_enable(DISP0_DISP_OD);
		break;
	case DISP_MODULE_DITHER0:
		ddp_clk_enable(DISP0_DISP_DITHER0);
		break;
	case DISP_MODULE_DITHER1:
		ddp_clk_enable(DISP0_DISP_DITHER1);
		break;
	case DISP_PATH0:
		/* no need */
		break;
	case DISP_PATH1:
		/* no need */
		break;
	case DISP_MODULE_UFOE:
		ddp_clk_enable(DISP0_DISP_UFOE);
		break;
	case DISP_MODULE_DSC:
		ddp_clk_enable(DISP0_DISP_DSC);
		break;
	case DISP_MODULE_DSC_2ND:
		ddp_clk_enable(DISP0_DISP_DSC);
		break;
	case DISP_MODULE_SPLIT0:
		ddp_clk_enable(DISP0_DISP_SPLIT);
		break;
	case DISP_MODULE_DPI:
		ddp_clk_enable(DISP1_DPI_MM_CLOCK);
		ddp_clk_enable(DISP1_DPI_INTERFACE_CLOCK);
		break;
	case DISP_MODULE_DSI0:
		ddp_clk_enable(DISP1_DSI0_MM_CLOCK);
		ddp_clk_enable(DISP1_DSI0_INTERFACE_CLOCK);
		break;
	case DISP_MODULE_DSI1:
		ddp_clk_enable(DISP1_DSI1_MM_CLOCK);
		ddp_clk_enable(DISP1_DSI1_INTERFACE_CLOCK);
		break;
	case DISP_MODULE_DSIDUAL:
		ddp_clk_enable(DISP1_DSI0_MM_CLOCK);
		ddp_clk_enable(DISP1_DSI0_INTERFACE_CLOCK);
		ddp_clk_enable(DISP1_DSI1_MM_CLOCK);
		ddp_clk_enable(DISP1_DSI1_INTERFACE_CLOCK);
		break;
	case DISP_MODULE_PWM0:
		ddp_clk_enable(DISP_CLK_PWM0);
		break;
	case DISP_MODULE_PWM1:
		ddp_clk_enable(DISP_CLK_PWM1);
		break;
	case DISP_MODULE_CONFIG:
		/* no need */
		break;
	case DISP_MODULE_MUTEX:
		/* no need */
		break;
	case DISP_MODULE_SMI_COMMON:
		for (i = 0; i < DDP_CLK_SMI_NUM; i++)
			ddp_clk_enable(ddp_clk_smi_map[i]);
		break;
	case DISP_MODULE_SMI_LARB0:
		ddp_clk_enable(DISP0_SMI_LARB0);
		break;
	case DISP_MODULE_SMI_LARB1:
		ddp_clk_enable(DISP0_SMI_LARB1);
		break;
	case DISP_MODULE_MIPI0:
		/* no need */
		break;
	case DISP_MODULE_MIPI1:
		/* no need */
		break;
	case DISP_MODULE_RSZ0:
		ddp_clk_enable(DISP0_DISP_RSZ0);
		break;
	case DISP_MODULE_RSZ1:
		ddp_clk_enable(DISP0_DISP_RSZ1);
		break;
	case DISP_MODULE_OVL0_VIRTUAL:
		/* no need */
		break;
	case DISP_MODULE_OVL0_2L_VIRTUAL:
		/* no need */
		break;
	case DISP_MODULE_OVL1_2L_VIRTUAL:
		/* no need */
		break;
	case DISP_MODULE_MTCMOS:
		ddp_clk_enable(DISP_MTCMOS_CLK);
		break;
	case DISP_MODULE_FAKE_ENG:
		ddp_clk_enable(DISP1_FAKE_ENG);
		ddp_clk_enable(DISP1_FAKE_ENG2);
		break;
	case DISP_MODULE_CLOCK_MUX:
		/* no need */
		break;
	default:
		DDPERR("invalid module id=%d\n", module);
		ret = -1;
	}
	return ret;
}

int ddp_clk_disable_by_module(enum DISP_MODULE_ENUM module)
{
	int i;
	int ret = 0;

	switch (module) {
	case DISP_MODULE_OVL0:
		ddp_clk_disable(DISP1_DISP_OVL0_MOUT);
		ddp_clk_disable(DISP0_DISP_OVL0);
		break;
	case DISP_MODULE_OVL1:
		ddp_clk_disable(DISP0_DISP_OVL1);
		break;
	case DISP_MODULE_OVL0_2L:
		ddp_clk_disable(DISP0_DISP_OVL0_2L);
		break;
	case DISP_MODULE_OVL1_2L:
		ddp_clk_disable(DISP0_DISP_OVL1_2L);
		break;
	case DISP_MODULE_RDMA0:
		ddp_clk_disable(DISP0_DISP_RDMA0);
		break;
	case DISP_MODULE_RDMA1:
		ddp_clk_disable(DISP0_DISP_RDMA1);
		break;
	case DISP_MODULE_RDMA2:
		ddp_clk_disable(DISP0_DISP_RDMA2);
		break;
	case DISP_MODULE_WDMA0:
		ddp_clk_disable(DISP0_DISP_WDMA0);
		break;
	case DISP_MODULE_WDMA1:
		ddp_clk_disable(DISP0_DISP_WDMA1);
		break;
	case DISP_MODULE_COLOR0:
		ddp_clk_disable(DISP0_DISP_COLOR0);
		break;
	case DISP_MODULE_COLOR1:
		ddp_clk_disable(DISP0_DISP_COLOR1);
		break;
	case DISP_MODULE_CCORR0:
		ddp_clk_disable(DISP0_DISP_CCORR0);
		break;
	case DISP_MODULE_CCORR1:
		ddp_clk_disable(DISP0_DISP_CCORR1);
		break;
	case DISP_MODULE_AAL0:
		ddp_clk_disable(DISP0_DISP_AAL0);
		break;
	case DISP_MODULE_AAL1:
		ddp_clk_disable(DISP0_DISP_AAL1);
		break;
	case DISP_MODULE_GAMMA0:
		ddp_clk_disable(DISP0_DISP_GAMMA0);
		break;
	case DISP_MODULE_GAMMA1:
		ddp_clk_disable(DISP0_DISP_GAMMA1);
		break;
	case DISP_MODULE_OD:
		ddp_clk_disable(DISP0_DISP_OD);
		break;
	case DISP_MODULE_DITHER0:
		ddp_clk_disable(DISP0_DISP_DITHER0);
		break;
	case DISP_MODULE_DITHER1:
		ddp_clk_disable(DISP0_DISP_DITHER1);
		break;
	case DISP_PATH0:
		/* no need */
		break;
	case DISP_PATH1:
		/* no need */
		break;
	case DISP_MODULE_UFOE:
		ddp_clk_disable(DISP0_DISP_UFOE);
		break;
	case DISP_MODULE_DSC:
		ddp_clk_disable(DISP0_DISP_DSC);
		break;
	case DISP_MODULE_DSC_2ND:
		ddp_clk_disable(DISP0_DISP_DSC);
		break;
	case DISP_MODULE_SPLIT0:
		ddp_clk_disable(DISP0_DISP_SPLIT);
		break;
	case DISP_MODULE_DPI:
		ddp_clk_disable(DISP1_DPI_INTERFACE_CLOCK);
		ddp_clk_disable(DISP1_DPI_MM_CLOCK);
		break;
	case DISP_MODULE_DSI0:
		ddp_clk_disable(DISP1_DSI0_INTERFACE_CLOCK);
		ddp_clk_disable(DISP1_DSI0_MM_CLOCK);
		break;
	case DISP_MODULE_DSI1:
		ddp_clk_disable(DISP1_DSI1_INTERFACE_CLOCK);
		ddp_clk_disable(DISP1_DSI1_MM_CLOCK);
		break;
	case DISP_MODULE_DSIDUAL:
		ddp_clk_disable(DISP1_DSI1_INTERFACE_CLOCK);
		ddp_clk_disable(DISP1_DSI1_MM_CLOCK);
		ddp_clk_disable(DISP1_DSI0_INTERFACE_CLOCK);
		ddp_clk_disable(DISP1_DSI0_MM_CLOCK);
		break;
	case DISP_MODULE_PWM0:
		ddp_clk_disable(DISP_CLK_PWM0);
		break;
	case DISP_MODULE_PWM1:
		ddp_clk_disable(DISP_CLK_PWM1);
		break;
	case DISP_MODULE_CONFIG:
		/* no need */
		break;
	case DISP_MODULE_MUTEX:
		/* no need */
		break;
	case DISP_MODULE_SMI_COMMON:
		for (i = (DDP_CLK_SMI_NUM - 1); i >= 0; i--)
			ddp_clk_disable(ddp_clk_smi_map[i]);
		break;
	case DISP_MODULE_SMI_LARB0:
		ddp_clk_disable(DISP0_SMI_LARB0);
		break;
	case DISP_MODULE_SMI_LARB1:
		ddp_clk_disable(DISP0_SMI_LARB1);
		break;
	case DISP_MODULE_MIPI0:
		/* no need */
		break;
	case DISP_MODULE_MIPI1:
		/* no need */
		break;
	case DISP_MODULE_RSZ0:
		ddp_clk_disable(DISP0_DISP_RSZ0);
		break;
	case DISP_MODULE_RSZ1:
		ddp_clk_disable(DISP0_DISP_RSZ1);
		break;
	case DISP_MODULE_OVL0_VIRTUAL:
		/* no need */
		break;
	case DISP_MODULE_OVL0_2L_VIRTUAL:
		/* no need */
		break;
	case DISP_MODULE_OVL1_2L_VIRTUAL:
		/* no need */
		break;
	case DISP_MODULE_MTCMOS:
		ddp_clk_disable(DISP_MTCMOS_CLK);
		break;
	case DISP_MODULE_FAKE_ENG:
		ddp_clk_disable(DISP1_FAKE_ENG2);
		ddp_clk_disable(DISP1_FAKE_ENG);
		break;
	case DISP_MODULE_CLOCK_MUX:
		/* no need */
		break;
	default:
		DDPERR("invalid module id=%d\n", module);
		ret = -1;
	}
	return ret;
}

int ddp_clk_set_parent(enum DDP_CLK_ID id, enum DDP_CLK_ID parent)
{
	if ((ddp_clk[id] == NULL) || (ddp_clk[parent] == NULL)) {
		DDPERR("DISPSYS CLK %d or parent %d NULL\n", id, parent);
		return -1;
	}
	return clk_set_parent(ddp_clk[id], ddp_clk[parent]);
}

static int __ddp_set_mipi26m(int idx, int en)
{
	unsigned int mask = 0;
	/* ref cnt for this clock control */
	static struct spinlock mipi_lock = __SPIN_LOCK_UNLOCKED(mipi_lock);
	static int refcnt[2];
	int old_cnt;

	if (idx < 0 || idx > 1) {
		DDPERR("set mipi26m error:idx=%d\n", idx);
		return -1;
	}

	mask = 1 << (13 + idx);
	spin_lock(&mipi_lock);

	old_cnt = refcnt[idx];
	if (en)
		refcnt[idx]++;
	else
		refcnt[idx]--;

	if (refcnt[idx] < 0)
		ASSERT(0);

	/* refcnt 0-->1 enable clock */
	if (old_cnt == 0)
		clk_setl(AP_PLL_CON0, mask);

	/* refcnt 1-->0 disable clock */
	if (refcnt[idx] == 0)
		clk_clrl(AP_PLL_CON0, mask);

	spin_unlock(&mipi_lock);
	return 0;
}


int ddp_set_mipi26m(enum DISP_MODULE_ENUM module, int en)
{
	int ret = 0;

	ret = ddp_parse_apmixed_base();
	if (ret)
		return -1;

	if (module == DISP_MODULE_DSI0 || module == DISP_MODULE_DSIDUAL)
		__ddp_set_mipi26m(1, en);
	if (module == DISP_MODULE_DSI1 || module == DISP_MODULE_DSIDUAL)
		__ddp_set_mipi26m(0, en);

	DDPMSG("%s en=%d, val=0x%x\n", __func__, en, clk_readl(AP_PLL_CON0));

	return ret;
}


int ddp_get_mipi26m(void)
{
	int ret = 0;

	ret = ddp_parse_apmixed_base();
	if (ret)
		return -1;

	DDPMSG("%s val=0x%x\n", __func__, clk_readl(AP_PLL_CON0));

	return ret;
}

int ddp_parse_apmixed_base(void)
{
	int ret = 0;

	struct device_node *node;

	if (parsed_apmixed)
		return ret;

	node = of_find_compatible_node(NULL, NULL, "mediatek,mt6799-apmixedsys");
	if (!node) {
		DDPERR("[DDP_APMIXED] DISP find apmixed node failed\n");
		return -1;
	}
	ddp_apmixed_base = of_iomap(node, 0);
	if (!ddp_apmixed_base) {
		DDPERR("[DDP_APMIXED] DISP apmixed base failed\n");
		return -1;
	}
	parsed_apmixed = 1;
	return ret;
}

#endif	/* CONFIG_MTK_CLKMGR */
