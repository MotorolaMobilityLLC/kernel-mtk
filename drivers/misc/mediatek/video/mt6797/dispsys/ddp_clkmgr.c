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

#define READ_REGISTER_UINT32(reg)       (*(volatile uint32_t * const)(reg))
#define INREG32(x)          READ_REGISTER_UINT32((uint32_t *)((void *)(x)))
#define DRV_Reg32(addr) INREG32(addr)
#define clk_readl(addr) DRV_Reg32(addr)
#define clk_writel(addr, val) mt_reg_sync_writel(val, addr)
#define clk_setl(addr, val) mt_reg_sync_writel(clk_readl(addr) | (val), addr)
#define clk_clrl(addr, val) mt_reg_sync_writel(clk_readl(addr) & ~(val), addr)

static struct clk *ddp_clk[MAX_DISP_CLK_CNT];

static void __iomem *ddp_apmixed_base;
#ifndef AP_PLL_CON0
#define AP_PLL_CON0 (ddp_apmixed_base + 0x00)
#endif


unsigned int parsed_apmixed = 0;
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

int ddp_clk_prepare(eDDP_CLK_ID id)
{
	int ret = 0;

	if (NULL == ddp_clk[id]) {
		DDPERR("DISPSYS CLK %d NULL\n", id);
		return -1;
	}
	ret = clk_prepare(ddp_clk[id]);
	if (ret)
		DDPERR("DISPSYS CLK prepare failed: errno %d id %d\n", ret, id);

	return ret;
}

int ddp_clk_unprepare(eDDP_CLK_ID id)
{
	int ret = 0;

	if (NULL == ddp_clk[id]) {
		DDPERR("DISPSYS CLK %d NULL\n", id);
		return -1;
	}
	clk_unprepare(ddp_clk[id]);
	return ret;
}

int ddp_clk_enable(eDDP_CLK_ID id)
{
	int ret = 0;

	if (NULL == ddp_clk[id]) {
		DDPERR("DISPSYS CLK %d NULL\n", id);
		return -1;
	}

	ret = clk_enable(ddp_clk[id]);
	if (ret)
		DDPERR("DISPSYS CLK enable failed: errno %d id=%d\n", ret, id);

	return ret;
}

int ddp_clk_disable(eDDP_CLK_ID id)
{
	int ret = 0;

	if (NULL == ddp_clk[id]) {
		DDPERR("DISPSYS CLK %d NULL\n", id);
		return -1;
	}
	clk_disable(ddp_clk[id]);
	return ret;
}

int ddp_clk_prepare_enable(eDDP_CLK_ID id)
{
	int ret = 0;

	if (NULL == ddp_clk[id]) {
		DDPERR("DISPSYS CLK %d NULL\n", id);
		return -1;
	}
	ret = clk_prepare_enable(ddp_clk[id]);
	if (ret)
		DDPERR("DISPSYS CLK prepare failed: errno %d\n", ret);

	return ret;
}

int ddp_clk_disable_unprepare(eDDP_CLK_ID id)
{
	int ret = 0;

	if (NULL == ddp_clk[id]) {
		DDPERR("DISPSYS CLK %d NULL\n", id);
		return -1;
	}
	clk_disable_unprepare(ddp_clk[id]);
	return ret;
}

int ddp_clk_set_parent(eDDP_CLK_ID id, eDDP_CLK_ID parent)
{
	if ((NULL == ddp_clk[id]) || (NULL == ddp_clk[parent])) {
		DDPERR("DISPSYS CLK %d or parent %d NULL\n", id, parent);
		return -1;
	}
	return clk_set_parent(ddp_clk[id], ddp_clk[parent]);
}

int ddp_set_mipi26m(int en)
{
	int ret = 0;

	ret = ddp_parse_apmixed_base();
	if (ret)
		return -1;
	if (en)
		clk_setl(AP_PLL_CON0, 1 << 6);
	else
		clk_clrl(AP_PLL_CON0, 1 << 6);
	return ret;
}

int ddp_parse_apmixed_base(void)
{
	int ret = 0;

	struct device_node *node;

	if (parsed_apmixed)
		return ret;

	node = of_find_compatible_node(NULL, NULL, "mediatek,APMIXED");
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
