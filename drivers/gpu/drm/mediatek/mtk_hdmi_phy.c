/*
 * Copyright (c) 2014 MediaTek Inc.
 * Author: Jie Qiu <jie.qiu@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/types.h>
#include "mtk_hdmi.h"
#include "mtk_hdmi_phy_regs.h"

struct mtk_hdmi_phy {
	void __iomem *regs;
	struct clk *pll_ref_clk;
	u8 drv_imp_clk;
	u8 drv_imp_d2;
	u8 drv_imp_d1;
	u8 drv_imp_d0;
	u32 ibias;
	u32 ibias_up;
};

static const u8 PREDIV[3][4] = {
	{0x0, 0x0, 0x0, 0x0},	/* 27Mhz */
	{0x1, 0x1, 0x1, 0x1},	/* 74Mhz */
	{0x1, 0x1, 0x1, 0x1}	/* 148Mhz */
};

static const u8 TXDIV[3][4] = {
	{0x3, 0x3, 0x3, 0x2},	/* 27Mhz */
	{0x2, 0x1, 0x1, 0x1},	/* 74Mhz */
	{0x1, 0x0, 0x0, 0x0}	/* 148Mhz */
};

static const u8 FBKSEL[3][4] = {
	{0x1, 0x1, 0x1, 0x1},	/* 27Mhz */
	{0x1, 0x0, 0x1, 0x1},	/* 74Mhz */
	{0x1, 0x0, 0x1, 0x1}	/* 148Mhz */
};

static const u8 FBKDIV[3][4] = {
	{19, 24, 29, 19},	/* 27Mhz */
	{19, 24, 14, 19},	/* 74Mhz */
	{19, 24, 14, 19}	/* 148Mhz */
};

static const u8 DIVEN[3][4] = {
	{0x2, 0x1, 0x1, 0x2},	/* 27Mhz */
	{0x2, 0x2, 0x2, 0x2},	/* 74Mhz */
	{0x2, 0x2, 0x2, 0x2}	/* 148Mhz */
};

static const u8 HTPLLBP[3][4] = {
	{0xc, 0xc, 0x8, 0xc},	/* 27Mhz */
	{0xc, 0xf, 0xf, 0xc},	/* 74Mhz */
	{0xc, 0xf, 0xf, 0xc}	/* 148Mhz */
};

static const u8 HTPLLBC[3][4] = {
	{0x2, 0x3, 0x3, 0x2},	/* 27Mhz */
	{0x2, 0x3, 0x3, 0x2},	/* 74Mhz */
	{0x2, 0x3, 0x3, 0x2}	/* 148Mhz */
};

static const u8 HTPLLBR[3][4] = {
	{0x1, 0x1, 0x0, 0x1},	/* 27Mhz */
	{0x1, 0x2, 0x2, 0x1},	/* 74Mhz */
	{0x1, 0x2, 0x2, 0x1}	/* 148Mhz */
};

static void mtk_hdmi_phy_mask(struct mtk_hdmi_phy *hdmi_phy, u32 offset,
			      u32 val, u32 mask)
{
	u32 tmp = readl(hdmi_phy->regs  + offset) & ~mask;

	tmp |= (val & mask);
	writel(tmp, hdmi_phy->regs + offset);
}

static void mtk_hdmi_phy_enable_tmds(struct mtk_hdmi_phy *hdmi_phy)
{
	clk_prepare_enable(hdmi_phy->pll_ref_clk);
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_CON1, RG_HDMITX_PLL_AUTOK_EN,
			  RG_HDMITX_PLL_AUTOK_EN);
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_CON0, RG_HDMITX_PLL_POSDIV,
			  RG_HDMITX_PLL_POSDIV);
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_CON3, 0, RG_HDMITX_MHLCK_EN);
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_CON1, RG_HDMITX_PLL_BIAS_EN,
			  RG_HDMITX_PLL_BIAS_EN);
	usleep_range(100, 150);
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_CON0, RG_HDMITX_PLL_EN,
			  RG_HDMITX_PLL_EN);
	usleep_range(100, 150);
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_CON1, RG_HDMITX_PLL_BIAS_LPF_EN,
			  RG_HDMITX_PLL_BIAS_LPF_EN);
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_CON1, RG_HDMITX_PLL_TXDIV_EN,
			  RG_HDMITX_PLL_TXDIV_EN);
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_CON3, RG_HDMITX_SER_EN,
			  RG_HDMITX_SER_EN);
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_CON3, RG_HDMITX_PRD_EN,
			  RG_HDMITX_PRD_EN);
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_CON3, RG_HDMITX_DRV_EN,
			  RG_HDMITX_DRV_EN);
	usleep_range(100, 150);
}

static void mtk_hdmi_phy_disable_tmds(struct mtk_hdmi_phy *hdmi_phy)
{
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_CON3, 0, RG_HDMITX_DRV_EN);
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_CON3, 0, RG_HDMITX_PRD_EN);
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_CON3, 0, RG_HDMITX_SER_EN);
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_CON1, 0, RG_HDMITX_PLL_TXDIV_EN);
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_CON1, 0, RG_HDMITX_PLL_BIAS_LPF_EN);
	usleep_range(100, 150);
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_CON0, 0, RG_HDMITX_PLL_EN);
	usleep_range(100, 150);
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_CON1, 0, RG_HDMITX_PLL_BIAS_EN);
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_CON0, 0, RG_HDMITX_PLL_POSDIV);
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_CON1, 0, RG_HDMITX_PLL_AUTOK_EN);
	usleep_range(100, 150);
	clk_disable_unprepare(hdmi_phy->pll_ref_clk);
}

void mtk_hdmi_phy_set_pll(struct phy *phy, u32 clock,
			  enum hdmi_display_color_depth depth)
{
	struct mtk_hdmi_phy *hdmi_phy = phy_get_drvdata(phy);
	unsigned int ibias;
	unsigned int pix;

	if (clock <= 74000000)
		clk_set_rate(hdmi_phy->pll_ref_clk, clock);
	else
		clk_set_rate(hdmi_phy->pll_ref_clk, clock / 2);

	dev_dbg(&phy->dev, "Want REF %u Hz, pixel clock %lu Hz\n",
		clock, clk_get_rate(hdmi_phy->pll_ref_clk));

	if (clock <= 27000000)
		pix = 0;
	else if (clock <= 74000000)
		pix = 1;
	else
		pix = 2;

	mtk_hdmi_phy_mask(hdmi_phy, HDMI_CON0,
			  ((PREDIV[pix][depth]) << PREDIV_SHIFT),
			  RG_HDMITX_PLL_PREDIV);
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_CON0, RG_HDMITX_PLL_POSDIV,
			  RG_HDMITX_PLL_POSDIV);
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_CON0,
			  (0x1 << PLL_IC_SHIFT) | (0x1 << PLL_IR_SHIFT),
			  RG_HDMITX_PLL_IC | RG_HDMITX_PLL_IR);
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_CON1,
			  ((TXDIV[pix][depth]) << PLL_TXDIV_SHIFT),
			  RG_HDMITX_PLL_TXDIV);
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_CON0,
			  ((FBKSEL[pix][depth]) << PLL_FBKSEL_SHIFT) |
			  ((FBKDIV[pix][depth]) << PLL_FBKDIV_SHIFT),
			  RG_HDMITX_PLL_FBKSEL | RG_HDMITX_PLL_FBKDIV);
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_CON1,
			  ((DIVEN[pix][depth]) << PLL_DIVEN_SHIFT),
			  RG_HDMITX_PLL_DIVEN);
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_CON0,
			  ((HTPLLBP[pix][depth]) << PLL_BP_SHIFT) |
			  ((HTPLLBC[pix][depth]) << PLL_BC_SHIFT) |
			  ((HTPLLBR[pix][depth]) << PLL_BR_SHIFT),
			  RG_HDMITX_PLL_BP | RG_HDMITX_PLL_BC |
			  RG_HDMITX_PLL_BR);

	if ((pix == 2) && (depth != HDMI_DEEP_COLOR_24BITS)) {
		mtk_hdmi_phy_mask(hdmi_phy, HDMI_CON3, RG_HDMITX_PRD_IMP_EN,
				  RG_HDMITX_PRD_IMP_EN);
		mtk_hdmi_phy_mask(hdmi_phy, HDMI_CON4,
				  (0x6 << PRD_IBIAS_CLK_SHIFT) |
				  (0x6 << PRD_IBIAS_D2_SHIFT) |
				  (0x6 << PRD_IBIAS_D1_SHIFT) |
				  (0x6 << PRD_IBIAS_D0_SHIFT),
				  RG_HDMITX_PRD_IBIAS_CLK |
				  RG_HDMITX_PRD_IBIAS_D2 |
				  RG_HDMITX_PRD_IBIAS_D1 |
				  RG_HDMITX_PRD_IBIAS_D0);
		mtk_hdmi_phy_mask(hdmi_phy, HDMI_CON3,
				  0xf << DRV_IMP_EN_SHIFT,
				  RG_HDMITX_DRV_IMP_EN);
		ibias = hdmi_phy->ibias_up;
	} else {
		mtk_hdmi_phy_mask(hdmi_phy, HDMI_CON3, 0, RG_HDMITX_PRD_IMP_EN);
		mtk_hdmi_phy_mask(hdmi_phy, HDMI_CON4,
				  (0x3 << PRD_IBIAS_CLK_SHIFT) |
				  (0x3 << PRD_IBIAS_D2_SHIFT) |
				  (0x3 << PRD_IBIAS_D1_SHIFT) |
				  (0x3 << PRD_IBIAS_D0_SHIFT),
				  RG_HDMITX_PRD_IBIAS_CLK |
				  RG_HDMITX_PRD_IBIAS_D2 |
				  RG_HDMITX_PRD_IBIAS_D1 |
				  RG_HDMITX_PRD_IBIAS_D0);
		mtk_hdmi_phy_mask(hdmi_phy, HDMI_CON3,
				  (0x0 << DRV_IMP_EN_SHIFT),
				  RG_HDMITX_DRV_IMP_EN);
		ibias = hdmi_phy->ibias;
	}

	mtk_hdmi_phy_mask(hdmi_phy, HDMI_CON6,
			  (hdmi_phy->drv_imp_clk << DRV_IMP_CLK_SHIFT) |
			  (hdmi_phy->drv_imp_d2 << DRV_IMP_D2_SHIFT) |
			  (hdmi_phy->drv_imp_d1 << DRV_IMP_D1_SHIFT) |
			  (hdmi_phy->drv_imp_d0 << DRV_IMP_D0_SHIFT),
			  RG_HDMITX_DRV_IMP_CLK | RG_HDMITX_DRV_IMP_D2 |
			  RG_HDMITX_DRV_IMP_D1 | RG_HDMITX_DRV_IMP_D0);
	mtk_hdmi_phy_mask(hdmi_phy, HDMI_CON5,
			  (ibias << DRV_IBIAS_CLK_SHIFT) |
			  (ibias << DRV_IBIAS_D2_SHIFT) |
			  (ibias << DRV_IBIAS_D1_SHIFT) |
			  (ibias << DRV_IBIAS_D0_SHIFT),
			  RG_HDMITX_DRV_IBIAS_CLK | RG_HDMITX_DRV_IBIAS_D2 |
			  RG_HDMITX_DRV_IBIAS_D1 | RG_HDMITX_DRV_IBIAS_D0);
}

static int mtk_hdmi_phy_power_on(struct phy *phy)
{
	struct mtk_hdmi_phy *hdmi_phy = phy_get_drvdata(phy);

	mtk_hdmi_phy_enable_tmds(hdmi_phy);

	return 0;
}

static int mtk_hdmi_phy_power_off(struct phy *phy)
{
	struct mtk_hdmi_phy *hdmi_phy = phy_get_drvdata(phy);

	mtk_hdmi_phy_disable_tmds(hdmi_phy);

	return 0;
}

static struct phy_ops mtk_hdmi_phy_ops = {
	.power_on = mtk_hdmi_phy_power_on,
	.power_off = mtk_hdmi_phy_power_off,
	.owner = THIS_MODULE,
};

static inline int drv_imp_div(u8 drv_imp)
{
	int sum = drv_imp & 0x1f;

	if (drv_imp & 0x20)
		sum += 20;
	return sum;
}

static int mtk_hdmi_phy_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_hdmi_phy *hdmi_phy;
	struct resource *mem;
	struct regmap *efusec;
	struct phy *phy;
	struct phy_provider *phy_provider;
	int ret;
	u32 tmp;

	hdmi_phy = devm_kzalloc(dev, sizeof(*hdmi_phy), GFP_KERNEL);
	if (!hdmi_phy)
		return -ENOMEM;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	hdmi_phy->regs = devm_ioremap_resource(dev, mem);
	if (IS_ERR(hdmi_phy->regs)) {
		ret = PTR_ERR(hdmi_phy->regs);
		dev_err(dev, "Failed to get memory resource: %d\n", ret);
		return ret;
	}

	hdmi_phy->pll_ref_clk = devm_clk_get(dev, "pll_ref");
	if (IS_ERR(hdmi_phy->pll_ref_clk)) {
		ret = PTR_ERR(hdmi_phy->pll_ref_clk);
		dev_err(&pdev->dev, "Failed to get PLL reference clock: %d\n",
			ret);
		return ret;
	}

	ret = of_property_read_u32(dev->of_node, "mediatek,ibias",
				   &hdmi_phy->ibias);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to get ibias: %d\n", ret);
		return ret;
	}

	ret = of_property_read_u32(dev->of_node, "mediatek,ibias_up",
				   &hdmi_phy->ibias_up);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to get ibias up: %d\n", ret);
		return ret;
	}

	/* TODO: Fuses should be handled via the NVMEM framework */
	efusec = syscon_regmap_lookup_by_phandle(dev->of_node, "efusec");
	if (IS_ERR(efusec)) {
		ret = PTR_ERR(efusec);
		dev_warn(dev, "Failed to read fuses: %d\n", ret);
	} else {
		ret = regmap_read(efusec, 0x4c8, &tmp);
		dev_info(dev, "regmap_read(4c8) = %x\n", tmp);
		if (!ret) {
			hdmi_phy->drv_imp_clk = tmp >> 24;
			hdmi_phy->drv_imp_d2 = (tmp & 0x00ff0000) >> 16;
		}

		ret = regmap_read(efusec, 0x530, &tmp);
		dev_info(dev, "regmap_read(530) = %x (50a90bad)\n", tmp);
		if (!ret) {
			hdmi_phy->drv_imp_d1 = (tmp & 0x00000fc0) >> 6;
			hdmi_phy->drv_imp_d0 = tmp & 0x3f;
		}
	}

	if (hdmi_phy->drv_imp_clk == 0 || hdmi_phy->drv_imp_d2 == 0 ||
	    hdmi_phy->drv_imp_d1 == 0 || hdmi_phy->drv_imp_d0 == 0) {
		dev_info(dev, "Using default TX DRV impedance: 4.2k/36\n");
		hdmi_phy->drv_imp_clk = 0x30;
		hdmi_phy->drv_imp_d2 = 0x30;
		hdmi_phy->drv_imp_d1 = 0x30;
		hdmi_phy->drv_imp_d0 = 0x30;
	} else {
		dev_info(dev,
			 "Using TX DRV impedance: CLK:4.2k/%d D2:4.2k/%d D1:4.2k/%d D0:4.2k/%d\n",
			 drv_imp_div(hdmi_phy->drv_imp_clk),
			 drv_imp_div(hdmi_phy->drv_imp_d2),
			 drv_imp_div(hdmi_phy->drv_imp_d1),
			 drv_imp_div(hdmi_phy->drv_imp_d0));
	}

	phy = devm_phy_create(dev, NULL, &mtk_hdmi_phy_ops);
	if (IS_ERR(phy)) {
		dev_err(dev, "Failed to create HDMI PHY\n");
		return PTR_ERR(phy);
	}
	phy_set_drvdata(phy, hdmi_phy);

	phy_provider = devm_of_phy_provider_register(dev, of_phy_simple_xlate);

	return PTR_ERR_OR_ZERO(phy_provider);
}

static int mtk_hdmi_phy_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id mtk_hdmi_phy_match[] = {
	{ .compatible = "mediatek,mt8173-hdmi-phy", },
	{},
};

struct platform_driver mtk_hdmi_phy_driver = {
	.probe = mtk_hdmi_phy_probe,
	.remove = mtk_hdmi_phy_remove,
	.driver = {
		.name = "mediatek-hdmi-phy",
		.of_match_table = mtk_hdmi_phy_match,
	},
};

MODULE_AUTHOR("Jie Qiu <jie.qiu@mediatek.com>");
MODULE_DESCRIPTION("MediaTek MT8173 HDMI PHY Driver");
MODULE_LICENSE("GPL v2");
