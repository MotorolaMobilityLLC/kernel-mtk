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
#include "disp_drv_log.h"

#ifndef CONFIG_MTK_CLKMGR

#define READ_REGISTER_UINT32(reg)       (*(uint32_t * const)(reg))
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


unsigned int parsed_apmixed;
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

	if (ddp_clk[id] == NULL) {
		DDPERR("DISPSYS CLK %d NULL\n", id);
		return -1;
	}
	ret = clk_prepare(ddp_clk[id]);
	if (ret)
		DDPERR("DISPSYS CLK prepare failed: errno %d id %d\n", ret, id);

	return ret;
}

int ddp_clk_unprepare(enum DDP_CLK_ID id)
{
	int ret = 0;

	if (ddp_clk[id] == NULL) {
		DDPERR("DISPSYS CLK %d NULL\n", id);
		return -1;
	}
	clk_unprepare(ddp_clk[id]);
	return ret;
}

int ddp_clk_enable(enum DDP_CLK_ID id)
{
	int ret = 0;

	if (ddp_clk[id] == NULL) {
		DDPERR("DISPSYS CLK %d NULL\n", id);
		return -1;
	}

	ret = clk_enable(ddp_clk[id]);
	if (ret)
		DDPERR("DISPSYS CLK enable failed: errno %d id=%d\n", ret, id);

	return ret;
}

int ddp_clk_disable(enum DDP_CLK_ID id)
{
	int ret = 0;

	if (ddp_clk[id] == NULL) {
		DDPERR("DISPSYS CLK %d NULL\n", id);
		return -1;
	}
	clk_disable(ddp_clk[id]);
	return ret;
}

int ddp_clk_prepare_enable(enum DDP_CLK_ID id)
{
	int ret = 0;

	if (ddp_clk[id] == NULL) {
		DDPERR("DISPSYS CLK %d NULL\n", id);
		return -1;
	}
	ret = clk_prepare_enable(ddp_clk[id]);
	if (ret)
		DDPERR("DISPSYS CLK prepare failed: errno %d\n", ret);

	return ret;
}

int ddp_clk_disable_unprepare(enum DDP_CLK_ID id)
{
	int ret = 0;

	if (ddp_clk[id] == NULL) {
		DDPERR("DISPSYS CLK %d NULL\n", id);
		return -1;
	}
	clk_disable_unprepare(ddp_clk[id]);
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
		pr_err("set mipi26m error:idx=%d\n", idx);
		return -1;
	}

	mask = 1 << (16 + idx);
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
		__ddp_set_mipi26m(0, en);
	if (module == DISP_MODULE_DSI1 || module == DISP_MODULE_DSIDUAL)
		__ddp_set_mipi26m(1, en);

	DISPINFO("%s en=%d, val=0x%x\n", __func__, en, clk_readl(AP_PLL_CON0));

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
