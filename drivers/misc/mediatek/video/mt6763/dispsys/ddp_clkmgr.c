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

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/types.h>
#include "mt-plat/sync_write.h"

#include "ddp_reg.h"
#include "ddp_info.h"
#include "ddp_log.h"
#include "primary_display.h"
#include "ddp_clkmgr.h"

/* #define READ_REGISTER_UINT32(reg)       (*(volatile uint32_t * const)(reg)) */
/* #define INREG32(x)          READ_REGISTER_UINT32((uint32_t *)((void *)(x))) */
#define DRV_Reg32(addr) INREG32(addr)
#define clk_readl(addr) DRV_Reg32(addr)
#define clk_writel(addr, val) mt_reg_sync_writel(val, addr)
#define clk_setl(addr, val) mt_reg_sync_writel(clk_readl(addr) | (val), addr)
#define clk_clrl(addr, val) mt_reg_sync_writel(clk_readl(addr) & ~(val), addr)


/* display clk table
 * -- by chip
 *	struct clk *pclk;
 *	const char *clk_name;
 *	unsigned int belong_to; 1: main display 2 externel display
 *	enum DISP_MODULE_ENUM module_id;
 */
static ddp_clk ddp_clks[MAX_DISP_CLK_CNT] = {
	{NULL, "MMSYS_MTCMOS", (0), DISP_MODULE_UNKNOWN}, /* set 0, top clk */
	{NULL, "MMSYS_SMI_COMMON", (0), DISP_MODULE_UNKNOWN}, /* set 0, top clk */
	{NULL, "MMSYS_SMI_LARB0", (0), DISP_MODULE_UNKNOWN}, /* set 0, top clk */
	{NULL, "MMSYS_SMI_LARB1", (0), DISP_MODULE_UNKNOWN}, /* set 0, top clk */
	{NULL, "MMSYS_GALS_COMM0", (0), DISP_MODULE_UNKNOWN}, /* set 0, top clk */
	{NULL, "MMSYS_GALS_COMM1", (0), DISP_MODULE_UNKNOWN}, /* set 0, top clk */
	{NULL, "MMSYS_GALS_VENC2MM", (0), DISP_MODULE_UNKNOWN}, /* set 0, top clk */
	{NULL, "MMSYS_GALS_VDEC2MM", (0), DISP_MODULE_UNKNOWN}, /* set 0, top clk */
	{NULL, "MMSYS_GALS_IMG2MM", (0), DISP_MODULE_UNKNOWN}, /* set 0, top clk */
	{NULL, "MMSYS_GALS_CAM2MM", (0), DISP_MODULE_UNKNOWN}, /* set 0, top clk */
	{NULL, "MMSYS_GALS_IPU2MM", (0), DISP_MODULE_UNKNOWN}, /* set 0, top clk */
	{NULL, "MMSYS_DISP_OVL0", (1), DISP_MODULE_OVL0},
	{NULL, "MMSYS_DISP_OVL0_2L", (1), DISP_MODULE_OVL0_2L},
	{NULL, "MMSYS_DISP_OVL1_2L", (1<<1), DISP_MODULE_OVL1_2L},
	{NULL, "MMSYS_DISP_RDMA0", (1), DISP_MODULE_RDMA0},
	{NULL, "MMSYS_DISP_RDMA1", (1<<1), DISP_MODULE_RDMA1},
	{NULL, "MMSYS_DISP_WDMA0", (1|1<<1), DISP_MODULE_WDMA0},
	{NULL, "MMSYS_DISP_COLOR0", (1), DISP_MODULE_COLOR0},
	{NULL, "MMSYS_DISP_CCORR0", (1), DISP_MODULE_CCORR0},
	{NULL, "MMSYS_DISP_AAL0", (1), DISP_MODULE_AAL0},
	{NULL, "MMSYS_DISP_GAMMA0", (1), DISP_MODULE_GAMMA0},
	{NULL, "MMSYS_DISP_DITHER0", (1), DISP_MODULE_DITHER0},
	{NULL, "MMSYS_DISP_SPLIT", (1), DISP_MODULE_SPLIT0},
	{NULL, "MMSYS_DSI0_MM_CK", (0), DISP_MODULE_UNKNOWN}, /* set 0, particular case */
	{NULL, "MMSYS_DSI0_IF_CK", (0), DISP_MODULE_UNKNOWN}, /* set 0, particular case */
	{NULL, "MMSYS_DSI1_MM_CK", (0), DISP_MODULE_UNKNOWN}, /* set 0, particular case */
	{NULL, "MMSYS_DSI1_IF_CK", (0), DISP_MODULE_UNKNOWN}, /* set 0, particular case */
	{NULL, "MMSYS_26M", (1), DISP_MODULE_UNKNOWN}, /* cg */
	{NULL, "MMSYS_DISP_RSZ", (1), DISP_MODULE_RSZ0},
	{NULL, "TOP_MUX_MM", (1), DISP_MODULE_UNKNOWN},
	{NULL, "TOP_MUX_DISP_PWM", (0), DISP_MODULE_UNKNOWN},
	{NULL, "DISP_PWM", (1), DISP_MODULE_PWM0},
	{NULL, "TOP_26M", (0), DISP_MODULE_UNKNOWN},
	{NULL, "TOP_UNIVPLL2_D4", (0), DISP_MODULE_UNKNOWN},
	{NULL, "TOP_OSC_D4", (0), DISP_MODULE_UNKNOWN},
	{NULL, "TOP_OSC_D16", (0), DISP_MODULE_UNKNOWN},
};

static void __iomem *ddp_apmixed_base;
#ifndef AP_PLL_CON0
#define AP_PLL_CON0 (ddp_apmixed_base + 0x00)
#endif


unsigned int parsed_apmixed;

const char *ddp_get_clk_name(unsigned int n)
{
	if (n >= MAX_DISP_CLK_CNT) {
		DDPERR("DISPSYS CLK id=%d is more than MAX_DISP_CLK_CNT\n", n);
		return NULL;
	}

	return ddp_clks[n].clk_name;
}

int ddp_set_clk_handle(struct clk *pclk, unsigned int n)
{
	int ret = 0;

	if (n >= MAX_DISP_CLK_CNT) {
		DDPERR("DISPSYS CLK id=%d is more than MAX_DISP_CLK_CNT\n", n);
		return -1;
	}
	ddp_clks[n].pclk = pclk;
	DDPMSG("ddp_clk[%d] %p\n", n, ddp_clks[n].pclk);
	return ret;
}

#if 0
int ddp_clk_prepare(enum DDP_CLK_ID id)
{
	int ret = 0;

	if (id >= MAX_DISP_CLK_CNT) {
		DDPERR("DISPSYS CLK id=%d is more than MAX_DISP_CLK_CNT\n", id);
		return -1;
	}

	if (ddp_clks[id].pclk == NULL) {
		DDPERR("DISPSYS CLK %d NULL\n", id);
		return -1;
	}
	ret = clk_prepare(ddp_clks[id].pclk);
	if (ret)
		DDPERR("DISPSYS CLK prepare failed: errno %d id %d\n", ret, id);

	return ret;
}

int ddp_clk_unprepare(enum DDP_CLK_ID id)
{
	int ret = 0;

	if (id >= MAX_DISP_CLK_CNT) {
		DDPERR("DISPSYS CLK id=%d is more than MAX_DISP_CLK_CNT\n", id);
		return -1;
	}

	if (ddp_clks[id].pclk == NULL) {
		DDPERR("DISPSYS CLK %d NULL\n", id);
		return -1;
	}
	clk_unprepare(ddp_clks[id].pclk);
	return ret;
}

int ddp_clk_enable(enum DDP_CLK_ID id)
{
	int ret = 0;

	if (id >= MAX_DISP_CLK_CNT) {
		DDPERR("DISPSYS CLK id=%d is more than MAX_DISP_CLK_CNT\n", id);
		return -1;
	}

	if (ddp_clks[id].pclk == NULL) {
		DDPERR("DISPSYS CLK %d NULL\n", id);
		return -1;
	}

	ret = clk_enable(ddp_clks[id].pclk);
	if (ret)
		DDPERR("DISPSYS CLK enable failed: errno %d id=%d\n", ret, id);

	return ret;
}

int ddp_clk_disable(enum DDP_CLK_ID id)
{
	int ret = 0;

	if (id >= MAX_DISP_CLK_CNT) {
		DDPERR("DISPSYS CLK id=%d is more than MAX_DISP_CLK_CNT\n", id);
		return -1;
	}

	if (ddp_clks[id].pclk == NULL) {
		DDPERR("DISPSYS CLK %d NULL\n", id);
		return -1;
	}
	clk_disable(ddp_clks[id].pclk);
	return ret;
}
#endif

int ddp_clk_prepare_enable(enum DDP_CLK_ID id)
{
	int ret = 0;

	pr_warn("ddp_clk_prepare_enable, clkid = %d\n", id);

	if (disp_helper_get_stage() != DISP_HELPER_STAGE_NORMAL)
		return ret;

	if (id >= MAX_DISP_CLK_CNT) {
		DDPERR("DISPSYS CLK id=%d is more than MAX_DISP_CLK_CNT\n", id);
		return -1;
	}

	if (ddp_clks[id].pclk == NULL) {
		DDPERR("DISPSYS CLK %d NULL\n", id);
		return -1;
	}
	ret = clk_prepare_enable(ddp_clks[id].pclk);
	if (ret)
		DDPERR("DISPSYS CLK prepare failed: errno %d\n", ret);

	return ret;
}

int ddp_clk_disable_unprepare(enum DDP_CLK_ID id)
{
	int ret = 0;

	pr_warn("ddp_clk_disable_unprepare, clkid = %d\n", id);

	if (disp_helper_get_stage() != DISP_HELPER_STAGE_NORMAL)
		return ret;

	if (id >= MAX_DISP_CLK_CNT) {
		DDPERR("DISPSYS CLK id=%d is more than MAX_DISP_CLK_CNT\n", id);
		return -1;
	}

	if (ddp_clks[id].pclk == NULL) {
		DDPERR("DISPSYS CLK %d NULL\n", id);
		return -1;
	}
	clk_disable_unprepare(ddp_clks[id].pclk);

	return ret;
}

int ddp_clk_set_parent(enum DDP_CLK_ID id, enum DDP_CLK_ID parent)
{
	if (id >= MAX_DISP_CLK_CNT) {
		DDPERR("DISPSYS CLK id=%d is more than MAX_DISP_CLK_CNT\n", id);
		return -1;
	}

	if (parent >= MAX_DISP_CLK_CNT) {
		DDPERR("DISPSYS CLK parent=%d is more than MAX_DISP_CLK_CNT\n", parent);
		return -1;
	}

	if ((ddp_clks[id].pclk == NULL) || (ddp_clks[parent].pclk == NULL)) {
		DDPERR("DISPSYS CLK %d or parent %d NULL\n", id, parent);
		return -1;
	}
	return clk_set_parent(ddp_clks[id].pclk, ddp_clks[parent].pclk);
}

static int __ddp_set_mipi26m(int idx, int en)
{
	unsigned int mask = 0;

	if (idx == 0) {
		mask = 1 << 14; /* 14: from apmix code */
	} else if (idx == 1) {
		mask = 1 << 13; /* 13: from apmix code */
	} else {
		pr_err("set mipi26m error:idx=%d\n", idx);
		return -1;
	}

	if (en)
		clk_setl(AP_PLL_CON0, mask);
	else
		clk_clrl(AP_PLL_CON0, mask);

	return 0;
}


int ddp_set_mipi26m(enum DISP_MODULE_ENUM module, int en)
{
	int ret = 0;

	if (disp_helper_get_stage() != DISP_HELPER_STAGE_NORMAL)
		return ret;

	ret = ddp_parse_apmixed_base();
	if (ret)
		return -1;

	if (module == DISP_MODULE_DSI0 || module == DISP_MODULE_DSIDUAL)
		__ddp_set_mipi26m(0, en);
	if (module == DISP_MODULE_DSI1 || module == DISP_MODULE_DSIDUAL)
		__ddp_set_mipi26m(1, en);

	DDPMSG("%s en=%d, val=0x%x\n", __func__, en, clk_readl(AP_PLL_CON0));

	return ret;
}

int ddp_parse_apmixed_base(void)
{
	int ret = 0;

	struct device_node *node;

	if (parsed_apmixed)
		return ret;

	node = of_find_compatible_node(NULL, NULL, "mediatek,apmixed");
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

static unsigned int _is_main_module(ddp_clk *pclk)
{
	return (pclk->belong_to & 0x1);
}

/* ddp_main_modules_clk_on
 *
 * success: ret = 0
 * error: ret = -1
 */
int ddp_main_modules_clk_on(void)
{
	unsigned int i = 0;
	int ret = 0;
	enum DISP_MODULE_ENUM module;

	DISPFUNC();
	/* --TOP CLK-- */
	ddp_clk_prepare_enable(DISP_MTCMOS_CLK);
	ddp_clk_prepare_enable(DISP0_SMI_COMMON);
	ddp_clk_prepare_enable(DISP0_SMI_LARB0);
	ddp_clk_prepare_enable(DISP0_SMI_LARB1);

	ddp_clk_prepare_enable(CLK_MM_GALS_COMM0);
	ddp_clk_prepare_enable(CLK_MM_GALS_COMM1);
	ddp_clk_prepare_enable(CLK_MM_GALS_VENC2MM);
	ddp_clk_prepare_enable(CLK_MM_GALS_VDEC2MM);
	ddp_clk_prepare_enable(CLK_MM_GALS_IMG2MM);
	ddp_clk_prepare_enable(CLK_MM_GALS_CAM2MM);
	ddp_clk_prepare_enable(CLK_MM_GALS_IPU2MM);

	ddp_clk_prepare_enable(DISP0_DISP_26M);

	/* --MODULE CLK-- */
	for (i = 0; i < MAX_DISP_CLK_CNT; i++) {
		if (!_is_main_module(&ddp_clks[i]))
			continue;

		module = ddp_clks[i].module_id;
		if (module != DISP_MODULE_UNKNOWN
			&& ddp_get_module_driver(module) != 0) {
			/* module driver power on */
			if (ddp_get_module_driver(module)->power_on != 0
				&& ddp_get_module_driver(module)->power_off != 0) {
				pr_warn("%s power_on\n", ddp_get_module_name(module));
				ddp_get_module_driver(module)->power_on(module, NULL);
			} else {
				DDPERR("[modules_clk_on] %s no power on(off) function\n", ddp_get_module_name(module));
				ret = -1;
			}
		}
	}

	/* DISP_DSI */
	module = _get_dst_module_by_lcm(primary_get_lcm());
	if (module == DISP_MODULE_UNKNOWN)
		ret = -1;
	else
		ddp_get_module_driver(module)->power_on(module, NULL);

	pr_warn("CG0 0x%x, CG1 0x%x\n", clk_readl(DISP_REG_CONFIG_MMSYS_CG_CON0),
									clk_readl(DISP_REG_CONFIG_MMSYS_CG_CON1));
	return ret;
}

/* ddp_main_modules_clk_on
 *
 * success: ret = 0
 * error: ret = -1
 */
int ddp_main_modules_clk_off(void)
{
	unsigned int i = 0;
	int ret = 0;
	enum DISP_MODULE_ENUM module;

	DISPFUNC();
	/* --MODULE CLK-- */
	for (i = 0; i < MAX_DISP_CLK_CNT; i++) {
		if (!_is_main_module(&ddp_clks[i]))
			continue;

		module = ddp_clks[i].module_id;
		if (module != DISP_MODULE_UNKNOWN
			&& ddp_get_module_driver(module) != 0) {
			/* module driver power off */
			if (ddp_get_module_driver(module)->power_on != 0
				&& ddp_get_module_driver(module)->power_off != 0) {
				pr_warn("%s power_off\n", ddp_get_module_name(module));
				ddp_get_module_driver(module)->power_off(module, NULL);
			} else {
				DDPERR("[modules_clk_on] %s no power on(off) function\n", ddp_get_module_name(module));
				ret = -1;
			}
		}
	}

	/* DISP_DSI */
	module = _get_dst_module_by_lcm(primary_get_lcm());
	if (module == DISP_MODULE_UNKNOWN)
		ret = -1;
	else
		ddp_get_module_driver(module)->power_off(module, NULL);


	/* --TOP CLK-- */
	ddp_clk_disable_unprepare(DISP0_DISP_26M);

	ddp_clk_disable_unprepare(CLK_MM_GALS_IPU2MM);
	ddp_clk_disable_unprepare(CLK_MM_GALS_CAM2MM);
	ddp_clk_disable_unprepare(CLK_MM_GALS_IMG2MM);
	ddp_clk_disable_unprepare(CLK_MM_GALS_VDEC2MM);
	ddp_clk_disable_unprepare(CLK_MM_GALS_VENC2MM);
	ddp_clk_disable_unprepare(CLK_MM_GALS_COMM1);
	ddp_clk_disable_unprepare(CLK_MM_GALS_COMM0);

	ddp_clk_disable_unprepare(DISP0_SMI_LARB0);
	ddp_clk_disable_unprepare(DISP0_SMI_LARB1);
	ddp_clk_disable_unprepare(DISP0_SMI_COMMON);
	ddp_clk_disable_unprepare(DISP_MTCMOS_CLK);

	pr_warn("CG0 0x%x, CG1 0x%x\n", clk_readl(DISP_REG_CONFIG_MMSYS_CG_CON0),
									clk_readl(DISP_REG_CONFIG_MMSYS_CG_CON1));
	return ret;
}


int ddp_module_clk_enable(enum DISP_MODULE_TYPE_ENUM module_t)
{
	int ret = 0;
	unsigned int i = 0;
	unsigned int j = 0;
	unsigned int number = 0;
	enum DISP_MODULE_ENUM module_id = DISP_MODULE_UNKNOWN;

	number = ddp_get_module_num_by_t(module_t);
	pr_warn("[ddp_module_clk_enable] module type = %d, module num on this type = %d\n", module_t, number);
	for (i = 0; i < number; i++) {
		module_id = ddp_get_module_id_by_idx(module_t, i);
		pr_warn("[ddp_module_clk_enable] module id = %d\n", module_id);
		for (j = 0; j < MAX_DISP_CLK_CNT; j++) {
			if (ddp_clks[j].module_id == module_id)
				ddp_clk_prepare_enable(j);
		}
	}

	return ret;
}

int ddp_module_clk_disable(enum DISP_MODULE_TYPE_ENUM module_t)
{
	int ret = 0;
	unsigned int i = 0;
	unsigned int j = 0;
	unsigned int number = 0;
	enum DISP_MODULE_ENUM module_id = DISP_MODULE_UNKNOWN;

	number = ddp_get_module_num_by_t(module_t);
	pr_warn("[ddp_module_clk_disable] module type = %d, module num on this type = %d\n", module_t, number);
	for (i = 0; i < number; i++) {
		module_id = ddp_get_module_id_by_idx(module_t, i);
		pr_warn("[ddp_module_clk_disable] module id = %d\n", module_id);
		for (j = 0; j < MAX_DISP_CLK_CNT; j++) {
			if (ddp_clks[j].module_id == module_id)
				ddp_clk_disable_unprepare(j);
		}
	}

	return ret;

}

enum DDP_CLK_ID ddp_get_module_clk_id(enum DISP_MODULE_ENUM module_id)
{
	unsigned int i = 0;

	for (i = 0; i < MAX_DISP_CLK_CNT; i++) {
		if (ddp_clks[i].module_id == module_id) {
			pr_warn("%s's clk id = %d\n", ddp_get_module_name(module_id), i);
			return i;
		}
	}

	return MAX_DISP_CLK_CNT;
}

void ddp_clk_force_on(unsigned int on)
{
	if (on) {
		ddp_clk_prepare_enable(DISP_MTCMOS_CLK);
		ddp_clk_prepare_enable(DISP0_SMI_COMMON);
		ddp_clk_prepare_enable(DISP0_SMI_LARB0);
		ddp_clk_prepare_enable(DISP0_SMI_LARB1);

		ddp_clk_prepare_enable(CLK_MM_GALS_COMM0);
		ddp_clk_prepare_enable(CLK_MM_GALS_COMM1);
	} else {
		ddp_clk_disable_unprepare(CLK_MM_GALS_COMM1);
		ddp_clk_disable_unprepare(CLK_MM_GALS_COMM0);

		ddp_clk_disable_unprepare(DISP0_SMI_LARB1);
		ddp_clk_disable_unprepare(DISP0_SMI_LARB0);
		ddp_clk_disable_unprepare(DISP0_SMI_COMMON);
		ddp_clk_disable_unprepare(DISP_MTCMOS_CLK);
	}
}
