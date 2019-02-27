/*
 * Copyright (C) 2018 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include <linux/clk.h>
#include <linux/device.h>       /* needed by device_* */

#include "adsp_clk.h"

struct clk *clk_adsp_infra;
struct clk *clk_top_adsp_sel;
struct clk *clk_adsp_clk26m;
struct clk *clk_top_mmpll_d4;
struct clk *clk_top_adsppll_d4;
struct clk *clk_top_adsppll_d6;

int adsp_set_top_mux(enum adsp_clk clk)
{
	int ret = 0;
	struct clk *parent;

	pr_debug("%s(%x)\n", __func__, clk);

	switch (clk) {
	case CLK_ADSP_CLK26M:
		parent = clk_adsp_clk26m;
		break;
	case CLK_TOP_MMPLL_D4:
		parent = clk_top_mmpll_d4;
		break;
	case CLK_TOP_ADSPPLL_D4:
		parent = clk_top_adsppll_d4;
		break;
	case CLK_TOP_ADSPPLL_D6:
		parent = clk_top_adsppll_d6;
		break;
	default:
		parent = clk_adsp_clk26m;
		break;
	}
	ret = clk_set_parent(clk_top_adsp_sel, parent);
	if (IS_ERR(&ret)) {
		pr_err("[ADSP] %s clk_set_parent(clk_top_adsp_sel-%x) fail %d\n",
		      __func__, clk, ret);
		return ret;
	} else {
		return 0;
	}
}

/* clock init */
int adsp_clk_device_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	clk_adsp_infra = devm_clk_get(&pdev->dev, "clk_adsp_infra");
	if (IS_ERR(clk_adsp_infra)) {
		dev_err(dev, "clk_get(\"clk_adsp_infra\") failed\n");
		return PTR_ERR(clk_adsp_infra);
	}

	clk_top_adsp_sel = devm_clk_get(&pdev->dev, "clk_top_adsp_sel");
	if (IS_ERR(clk_top_adsp_sel)) {
		dev_err(dev, "clk_get(\"clk_top_adsp_sel\") failed\n");
		return PTR_ERR(clk_top_adsp_sel);
	}

	clk_adsp_clk26m = devm_clk_get(&pdev->dev, "clk_adsp_clk26m");
	if (IS_ERR(clk_adsp_clk26m)) {
		dev_err(dev, "clk_get(\"clk_adsp_clk26m\") failed\n");
		return PTR_ERR(clk_adsp_clk26m);
	}

	clk_top_mmpll_d4 = devm_clk_get(&pdev->dev, "clk_top_mmpll_d4");
	if (IS_ERR(clk_top_mmpll_d4)) {
		dev_err(dev, "clk_get(\"clk_top_mmpll_d4\") failed\n");
		return PTR_ERR(clk_top_mmpll_d4);
	}

	clk_top_adsppll_d4 = devm_clk_get(&pdev->dev, "clk_top_adsppll_d4");
	if (IS_ERR(clk_top_adsppll_d4)) {
		dev_err(dev, "clk_get(\"clk_top_adsppll_d4\") failed\n");
		return PTR_ERR(clk_top_adsppll_d4);
	}

	clk_top_adsppll_d6 = devm_clk_get(&pdev->dev, "clk_top_adsppll_d6");
	if (IS_ERR(clk_top_adsppll_d6)) {
		dev_err(dev, "clk_get(\"clk_top_adsppll_d6\") failed\n");
		return PTR_ERR(clk_top_adsppll_d6);
	}
	return 0;
}

int adsp_enable_clock(void)
{
	int ret = 0;

	ret = clk_prepare_enable(clk_adsp_infra);
	if (IS_ERR(&ret)) {
		pr_err("[ADSP] %s clk_prepare_enable(clk_adsp_infra) fail %d\n",
		       __func__, ret);
		return PTR_ERR(&ret);
	}
	return ret;
}

void adsp_disable_clock(void)
{
	clk_disable_unprepare(clk_adsp_infra);
}
