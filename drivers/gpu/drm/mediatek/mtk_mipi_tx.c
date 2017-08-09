/*
 * Copyright (c) 2015 MediaTek Inc.
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
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/phy/phy.h>

#include "mtk_mipi_tx.h"

#define MIPITX_DSI_CON		0x00
#define RG_DSI_LDOCORE_EN		BIT(0)
#define RG_DSI_CKG_LDOOUT_EN		BIT(1)
#define RG_DSI_BCLK_SEL			(3 << 2)
#define RG_DSI_LD_IDX_SEL		(7 << 4)
#define RG_DSI_PHYCLK_SEL		(2 << 8)
#define RG_DSI_DSICLK_FREQ_SEL		BIT(10)
#define RG_DSI_LPTX_CLMP_EN		BIT(11)

#define MIPITX_DSI_CLOCK_LANE	0x04
#define RG_DSI_LNTC_LDOOUT_EN		BIT(0)
#define RG_DSI_LNTC_CKLANE_EN		BIT(1)
#define RG_DSI_LNTC_LPTX_IPLUS1		BIT(2)
#define RG_DSI_LNTC_LPTX_IPLUS2		BIT(3)
#define RG_DSI_LNTC_LPTX_IMINUS		BIT(4)
#define RG_DSI_LNTC_LPCD_IPLUS		BIT(5)
#define RG_DSI_LNTC_LPCD_IMLUS		BIT(6)
#define RG_DSI_LNTC_RT_CODE		(0xf << 8)

#define MIPITX_DSI_DATA_LANE0	0x08
#define RG_DSI_LNT0_LDOOUT_EN		BIT(0)
#define RG_DSI_LNT0_CKLANE_EN		BIT(1)
#define RG_DSI_LNT0_LPTX_IPLUS1		BIT(2)
#define RG_DSI_LNT0_LPTX_IPLUS2		BIT(3)
#define RG_DSI_LNT0_LPTX_IMINUS		BIT(4)
#define RG_DSI_LNT0_LPCD_IPLUS		BIT(5)
#define RG_DSI_LNT0_LPCD_IMINUS		BIT(6)
#define RG_DSI_LNT0_RT_CODE		(0xf << 8)

#define MIPITX_DSI_DATA_LANE1	0x0c
#define RG_DSI_LNT1_LDOOUT_EN		BIT(0)
#define RG_DSI_LNT1_CKLANE_EN		BIT(1)
#define RG_DSI_LNT1_LPTX_IPLUS1		BIT(2)
#define RG_DSI_LNT1_LPTX_IPLUS2		BIT(3)
#define RG_DSI_LNT1_LPTX_IMINUS		BIT(4)
#define RG_DSI_LNT1_LPCD_IPLUS		BIT(5)
#define RG_DSI_LNT1_LPCD_IMINUS		BIT(6)
#define RG_DSI_LNT1_RT_CODE		(0xf << 8)

#define MIPITX_DSI_DATA_LANE2	0x10
#define RG_DSI_LNT2_LDOOUT_EN		BIT(0)
#define RG_DSI_LNT2_CKLANE_EN		BIT(1)
#define RG_DSI_LNT2_LPTX_IPLUS1		BIT(2)
#define RG_DSI_LNT2_LPTX_IPLUS2		BIT(3)
#define RG_DSI_LNT2_LPTX_IMINUS		BIT(4)
#define RG_DSI_LNT2_LPCD_IPLUS		BIT(5)
#define RG_DSI_LNT2_LPCD_IMINUS		BIT(6)
#define RG_DSI_LNT2_RT_CODE		(0xf << 8)

#define MIPITX_DSI_DATA_LANE3	0x14
#define RG_DSI_LNT3_LDOOUT_EN		BIT(0)
#define RG_DSI_LNT3_CKLANE_EN		BIT(1)
#define RG_DSI_LNT3_LPTX_IPLUS1		BIT(2)
#define RG_DSI_LNT3_LPTX_IPLUS2		BIT(3)
#define RG_DSI_LNT3_LPTX_IMINUS		BIT(4)
#define RG_DSI_LNT3_LPCD_IPLUS		BIT(5)
#define RG_DSI_LNT3_LPCD_IMINUS		BIT(6)
#define RG_DSI_LNT3_RT_CODE		(0xf << 8)

#define MIPITX_DSI_TOP_CON	0x40
#define RG_DSI_LNT_INTR_EN		BIT(0)
#define RG_DSI_LNT_HS_BIAS_EN		BIT(1)
#define RG_DSI_LNT_IMP_CAL_EN		BIT(2)
#define RG_DSI_LNT_TESTMODE_EN		BIT(3)
#define RG_DSI_LNT_IMP_CAL_CODE		(0xf << 4)
#define RG_DSI_LNT_AIO_SEL		(7 << 8)
#define RG_DSI_PAD_TIE_LOW_EN		BIT(11)
#define RG_DSI_DEBUG_INPUT_EN		BIT(12)
#define RG_DSI_PRESERVE			(7 << 13)

#define MIPITX_DSI_BG_CON	0x44
#define RG_DSI_BG_CORE_EN		BIT(0)
#define RG_DSI_BG_CKEN			BIT(1)
#define RG_DSI_BG_DIV			(0x3 << 2)
#define RG_DSI_BG_FAST_CHARGE		BIT(4)
#define RG_DSI_VOUT_MSK			(0x3ffff << 5)
#define RG_DSI_V12_SEL			(7 << 5)
#define RG_DSI_V10_SEL			(7 << 8)
#define RG_DSI_V072_SEL			(7 << 11)
#define RG_DSI_V04_SEL			(7 << 14)
#define RG_DSI_V032_SEL			(7 << 17)
#define RG_DSI_V02_SEL			(7 << 20)
#define RG_DSI_BG_R1_TRIM		(0xf << 24)
#define RG_DSI_BG_R2_TRIM		(0xf << 28)

#define MIPITX_DSI_PLL_CON0	0x50
#define RG_DSI_MPPLL_PLL_EN		BIT(0)
#define RG_DSI_MPPLL_DIV_MSK		(0x1ff << 1)
#define RG_DSI_MPPLL_PREDIV		(3 << 1)
#define RG_DSI_MPPLL_TXDIV0		(3 << 3)
#define RG_DSI_MPPLL_TXDIV1		(3 << 5)
#define RG_DSI_MPPLL_POSDIV		(7 << 7)
#define RG_DSI_MPPLL_MONVC_EN		BIT(10)
#define RG_DSI_MPPLL_MONREF_EN		BIT(11)
#define RG_DSI_MPPLL_VOD_EN		BIT(12)

#define MIPITX_DSI_PLL_CON1	0x54
#define RG_DSI_MPPLL_SDM_FRA_EN		BIT(0)
#define RG_DSI_MPPLL_SDM_SSC_PH_INIT	BIT(1)
#define RG_DSI_MPPLL_SDM_SSC_EN		BIT(2)
#define RG_DSI_MPPLL_SDM_SSC_PRD	(0xffff << 16)

#define MIPITX_DSI_PLL_CON2	0x58

#define MIPITX_DSI_PLL_TOP	0x64
#define RG_DSI_MPPLL_PRESERVE		(0xff << 8)

#define MIPITX_DSI_PLL_PWR	0x68
#define RG_DSI_MPPLL_SDM_PWR_ON		BIT(0)
#define RG_DSI_MPPLL_SDM_ISO_EN		BIT(1)
#define RG_DSI_MPPLL_SDM_PWR_ACK	BIT(8)

#define MIPITX_DSI_SW_CTRL	0x80
#define SW_CTRL_EN			BIT(0)

#define MIPITX_DSI_SW_CTRL_CON0	0x84
#define SW_LNTC_LPTX_PRE_OE		BIT(0)
#define SW_LNTC_LPTX_OE			BIT(1)
#define SW_LNTC_LPTX_P			BIT(2)
#define SW_LNTC_LPTX_N			BIT(3)
#define SW_LNTC_HSTX_PRE_OE		BIT(4)
#define SW_LNTC_HSTX_OE			BIT(5)
#define SW_LNTC_HSTX_ZEROCLK		BIT(6)
#define SW_LNT0_LPTX_PRE_OE		BIT(7)
#define SW_LNT0_LPTX_OE			BIT(8)
#define SW_LNT0_LPTX_P			BIT(9)
#define SW_LNT0_LPTX_N			BIT(10)
#define SW_LNT0_HSTX_PRE_OE		BIT(11)
#define SW_LNT0_HSTX_OE			BIT(12)
#define SW_LNT0_LPRX_EN			BIT(13)
#define SW_LNT1_LPTX_PRE_OE		BIT(14)
#define SW_LNT1_LPTX_OE			BIT(15)
#define SW_LNT1_LPTX_P			BIT(16)
#define SW_LNT1_LPTX_N			BIT(17)
#define SW_LNT1_HSTX_PRE_OE		BIT(18)
#define SW_LNT1_HSTX_OE			BIT(19)
#define SW_LNT2_LPTX_PRE_OE		BIT(20)
#define SW_LNT2_LPTX_OE			BIT(21)
#define SW_LNT2_LPTX_P			BIT(22)
#define SW_LNT2_LPTX_N			BIT(23)
#define SW_LNT2_HSTX_PRE_OE		BIT(24)
#define SW_LNT2_HSTX_OE			BIT(25)

enum reg_idx {
	TX_DSI_CON,
	TX_DSI_CLOCK_LANE,
	TX_DSI_DATA_LANE0,
	TX_DSI_DATA_LANE1,
	TX_DSI_DATA_LANE2,
	TX_DSI_DATA_LANE3,
	TX_DSI_TOP_CON,
	TX_DSI_BG_CON,
	TX_DSI_PLL_CON0,
	TX_DSI_PLL_CON1,
	TX_DSI_PLL_CON2,
	TX_DSI_PLL_TOP,
	TX_DSI_PLL_PWR,
	TX_DSI_SW_CTRL,

	NUM_REGS
};

static unsigned int mtk_regs_ofs[] = {
	[TX_DSI_CON]		=  MIPITX_DSI_CON,
	[TX_DSI_CLOCK_LANE]	=  MIPITX_DSI_CLOCK_LANE,
	[TX_DSI_DATA_LANE0]	=  MIPITX_DSI_DATA_LANE0,
	[TX_DSI_DATA_LANE1]	=  MIPITX_DSI_DATA_LANE1,
	[TX_DSI_DATA_LANE2]	=  MIPITX_DSI_DATA_LANE2,
	[TX_DSI_DATA_LANE3]	=  MIPITX_DSI_DATA_LANE3,
	[TX_DSI_TOP_CON]	=  MIPITX_DSI_TOP_CON,
	[TX_DSI_BG_CON]		=  MIPITX_DSI_BG_CON,
	[TX_DSI_PLL_CON0]	=  MIPITX_DSI_PLL_CON0,
	[TX_DSI_PLL_CON1]	=  MIPITX_DSI_PLL_CON1,
	[TX_DSI_PLL_CON2]	=  MIPITX_DSI_PLL_CON2,
	[TX_DSI_PLL_TOP]	=  MIPITX_DSI_PLL_TOP,
	[TX_DSI_PLL_PWR]	=  MIPITX_DSI_PLL_PWR,
	[TX_DSI_SW_CTRL]	=  MIPITX_DSI_SW_CTRL,
};

static unsigned int mt2701_regs_val[] = {
	[TX_DSI_PLL_TOP]	=  (3 << 8),
};

static unsigned int mt8173_regs_val[] = {
	[TX_DSI_PLL_TOP]	=  (0 << 8),
};

struct mtk_mipitx_driver_data {
	unsigned int *reg_ofs;
	unsigned int *reg_value;
};

static struct mtk_mipitx_driver_data mt2701_mipitx_driver_data = {
	.reg_ofs = mtk_regs_ofs,
	.reg_value = mt2701_regs_val,
};

static struct mtk_mipitx_driver_data mt8173_mipitx_driver_data = {
	.reg_ofs = mtk_regs_ofs,
	.reg_value = mt8173_regs_val,
};

static const struct of_device_id mtk_mipi_tx_match[] = {
	{ .compatible = "mediatek,mt2701-mipi-tx",
	  .data = &mt2701_mipitx_driver_data },
	{ .compatible = "mediatek,mt8173-mipi-tx",
	  .data = &mt8173_mipitx_driver_data },
	{},
};

struct mtk_mipi_tx {
	void __iomem *regs;
	unsigned int data_rate;
	struct mtk_mipitx_driver_data *driver_data;
};

#define MIPITX_REG_ADDR(mipi_tx, reg_idx) ((mipi_tx)->regs + \
				mipi_tx->driver_data->reg_ofs[(reg_idx)])
#define MIPITX_WRITE(mipi_tx, reg_idx, val)	writel((val), \
				MIPITX_REG_ADDR((mipi_tx), (reg_idx)))
#define MIPITX_READ(mipi_tx, reg_idx) \
				readl(MIPITX_REG_ADDR((mipi_tx), (reg_idx)))

static void mtk_mipi_tx_mask(struct mtk_mipi_tx *mipi_tx, u32 reg_idx, u32 mask,
			     u32 data)
{
	u32 temp = MIPITX_READ(mipi_tx, reg_idx);

	MIPITX_WRITE(mipi_tx, reg_idx, (temp & ~mask) | (data & mask));
}

int mtk_mipi_tx_set_data_rate(struct phy *phy, unsigned int data_rate)
{
	struct mtk_mipi_tx *mipi_tx = phy_get_drvdata(phy);

	if (data_rate < 50 || data_rate > 1250)
		return -EINVAL;

	mipi_tx->data_rate = data_rate;

	return 0;
}

static int mtk_mipi_tx_power_on(struct phy *phy)
{
	struct mtk_mipi_tx *mipi_tx = phy_get_drvdata(phy);
	unsigned int txdiv, txdiv0, txdiv1;
	u64 pcw;

	mtk_mipi_tx_mask(mipi_tx, TX_DSI_TOP_CON,
			 RG_DSI_LNT_IMP_CAL_CODE | RG_DSI_LNT_HS_BIAS_EN,
			 (8 << 4) | RG_DSI_LNT_HS_BIAS_EN);

	mtk_mipi_tx_mask(mipi_tx, TX_DSI_BG_CON,
			 RG_DSI_VOUT_MSK | RG_DSI_BG_CKEN | RG_DSI_BG_CORE_EN,
			 (4 << 20) | (4 << 17) | (4 << 14) |
			 (4 << 11) | (4 << 8) | (4 << 5) |
			 RG_DSI_BG_CKEN | RG_DSI_BG_CORE_EN);

	usleep_range(30, 100);

	mtk_mipi_tx_mask(mipi_tx, TX_DSI_CON,
			 RG_DSI_CKG_LDOOUT_EN | RG_DSI_LDOCORE_EN,
			 RG_DSI_CKG_LDOOUT_EN | RG_DSI_LDOCORE_EN);

	mtk_mipi_tx_mask(mipi_tx, TX_DSI_PLL_PWR,
			 RG_DSI_MPPLL_SDM_PWR_ON | RG_DSI_MPPLL_SDM_ISO_EN,
			 RG_DSI_MPPLL_SDM_PWR_ON);

	/**
	 * data_rate = (pixel_clock / 1000) * pixel_dipth * mipi_ratio;
	 * pixel_clock unit is Khz, data_rata unit is MHz, so need divide 1000.
	 * mipi_ratio is mipi clk coefficient for balance the pixel clk in mipi.
	 * we set mipi_ratio is 1.05.
	 */

	if (mipi_tx->data_rate > 1250) {
		return -EINVAL;
	} else if (mipi_tx->data_rate >= 500) {
		txdiv = 1;
		txdiv0 = 0;
		txdiv1 = 0;
	} else if (mipi_tx->data_rate >= 250) {
		txdiv = 2;
		txdiv0 = 1;
		txdiv1 = 0;
	} else if (mipi_tx->data_rate >= 125) {
		txdiv = 4;
		txdiv0 = 2;
		txdiv1 = 0;
	} else if (mipi_tx->data_rate > 62) {
		txdiv = 8;
		txdiv0 = 2;
		txdiv1 = 1;
	} else if (mipi_tx->data_rate >= 50) {
		txdiv = 16;
		txdiv0 = 2;
		txdiv1 = 2;
	} else {
		return -EINVAL;
	}

	mtk_mipi_tx_mask(mipi_tx, TX_DSI_PLL_CON0,
			 RG_DSI_MPPLL_PREDIV | RG_DSI_MPPLL_TXDIV0 |
			 RG_DSI_MPPLL_TXDIV1 | RG_DSI_MPPLL_POSDIV,
			 (0 << 1) | (txdiv0 << 3) | (txdiv1 << 5) | (0 << 7));

	/*
	 * PLL PCW config
	 * PCW bit 24~30 = integer part of pcw
	 * PCW bit 0~23 = fractional part of pcw
	 * pcw = data_Rate*4*txdiv/(Ref_clk*2);
	 * Post DIV =4, so need data_Rate*4
	 * Ref_clk is 26MHz
	 */
	pcw = ((u64) mipi_tx->data_rate * txdiv) << 24;
	do_div(pcw, 13);
	MIPITX_WRITE(mipi_tx, TX_DSI_PLL_CON2, pcw);

	mtk_mipi_tx_mask(mipi_tx, TX_DSI_PLL_CON1,
			 RG_DSI_MPPLL_SDM_FRA_EN, RG_DSI_MPPLL_SDM_FRA_EN);

	mtk_mipi_tx_mask(mipi_tx, TX_DSI_CLOCK_LANE,
			 RG_DSI_LNTC_LDOOUT_EN, RG_DSI_LNTC_LDOOUT_EN);
	mtk_mipi_tx_mask(mipi_tx, TX_DSI_DATA_LANE0,
			 RG_DSI_LNT0_LDOOUT_EN, RG_DSI_LNT0_LDOOUT_EN);
	mtk_mipi_tx_mask(mipi_tx, TX_DSI_DATA_LANE1,
			 RG_DSI_LNT1_LDOOUT_EN, RG_DSI_LNT1_LDOOUT_EN);
	mtk_mipi_tx_mask(mipi_tx, TX_DSI_DATA_LANE2,
			 RG_DSI_LNT2_LDOOUT_EN, RG_DSI_LNT2_LDOOUT_EN);
	mtk_mipi_tx_mask(mipi_tx, TX_DSI_DATA_LANE3,
			 RG_DSI_LNT3_LDOOUT_EN, RG_DSI_LNT3_LDOOUT_EN);

	mtk_mipi_tx_mask(mipi_tx, TX_DSI_PLL_CON0,
			 RG_DSI_MPPLL_PLL_EN, RG_DSI_MPPLL_PLL_EN);

	usleep_range(20, 100);

	mtk_mipi_tx_mask(mipi_tx, TX_DSI_PLL_CON1,
			 RG_DSI_MPPLL_SDM_SSC_EN, (0 << 2));
	mtk_mipi_tx_mask(mipi_tx, TX_DSI_PLL_TOP, RG_DSI_MPPLL_PRESERVE,
			 mipi_tx->driver_data->reg_value[TX_DSI_PLL_TOP]);
	mtk_mipi_tx_mask(mipi_tx, TX_DSI_TOP_CON, RG_DSI_PAD_TIE_LOW_EN, (0 << 11));

	return 0;
}

static int mtk_mipi_tx_power_off(struct phy *phy)
{
	struct mtk_mipi_tx *mipi_tx = phy_get_drvdata(phy);

	mtk_mipi_tx_mask(mipi_tx, TX_DSI_PLL_CON0, RG_DSI_MPPLL_PLL_EN, 0);

	mtk_mipi_tx_mask(mipi_tx, TX_DSI_PLL_TOP, RG_DSI_MPPLL_PRESERVE,
			 (0 << 8));

	mtk_mipi_tx_mask(mipi_tx, TX_DSI_TOP_CON, RG_DSI_PAD_TIE_LOW_EN,
			RG_DSI_PAD_TIE_LOW_EN);

	mtk_mipi_tx_mask(mipi_tx, TX_DSI_CLOCK_LANE,
			 RG_DSI_LNTC_LDOOUT_EN, 0);
	mtk_mipi_tx_mask(mipi_tx, TX_DSI_DATA_LANE0,
			 RG_DSI_LNT0_LDOOUT_EN, 0);
	mtk_mipi_tx_mask(mipi_tx, TX_DSI_DATA_LANE1,
			 RG_DSI_LNT1_LDOOUT_EN, 0);
	mtk_mipi_tx_mask(mipi_tx, TX_DSI_DATA_LANE2,
			 RG_DSI_LNT2_LDOOUT_EN, 0);
	mtk_mipi_tx_mask(mipi_tx, TX_DSI_DATA_LANE3,
			 RG_DSI_LNT3_LDOOUT_EN, 0);

	mtk_mipi_tx_mask(mipi_tx, TX_DSI_PLL_PWR,
			RG_DSI_MPPLL_SDM_ISO_EN | RG_DSI_MPPLL_SDM_PWR_ON,
			RG_DSI_MPPLL_SDM_ISO_EN);

	mtk_mipi_tx_mask(mipi_tx, TX_DSI_TOP_CON, RG_DSI_LNT_HS_BIAS_EN, 0);

	mtk_mipi_tx_mask(mipi_tx, TX_DSI_CON,
			RG_DSI_CKG_LDOOUT_EN | RG_DSI_LDOCORE_EN, 0);

	mtk_mipi_tx_mask(mipi_tx, TX_DSI_BG_CON,
			RG_DSI_BG_CKEN | RG_DSI_BG_CORE_EN, 0);

	mtk_mipi_tx_mask(mipi_tx, TX_DSI_PLL_CON0, RG_DSI_MPPLL_DIV_MSK, 0);

	return 0;
}

static struct phy_ops mtk_mipi_tx_ops = {
	.power_on = mtk_mipi_tx_power_on,
	.power_off = mtk_mipi_tx_power_off,
	.owner = THIS_MODULE,
};

static int mtk_mipi_tx_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_mipi_tx *mipi_tx;
	const struct of_device_id *of_id;
	struct resource *mem;
	struct phy *phy;
	struct phy_provider *phy_provider;
	int ret;

	mipi_tx = devm_kzalloc(dev, sizeof(*mipi_tx), GFP_KERNEL);
	if (!mipi_tx)
		return -ENOMEM;

	of_id = of_match_device(mtk_mipi_tx_match, &pdev->dev);
	mipi_tx->driver_data = (struct mtk_mipitx_driver_data *)of_id->data;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	mipi_tx->regs = devm_ioremap_resource(dev, mem);
	if (IS_ERR(mipi_tx->regs)) {
		ret = PTR_ERR(mipi_tx->regs);
		dev_err(dev, "Failed to get memory resource: %d\n", ret);
		return ret;
	}

	phy = devm_phy_create(dev, NULL, &mtk_mipi_tx_ops);
	if (IS_ERR(phy)) {
		dev_err(dev, "Failed to create MIPI D-PHY\n");
		return PTR_ERR(phy);
	}
	phy_set_drvdata(phy, mipi_tx);

	phy_provider = devm_of_phy_provider_register(dev, of_phy_simple_xlate);

	return PTR_ERR_OR_ZERO(phy_provider);
}

static int mtk_mipi_tx_remove(struct platform_device *pdev)
{
	return 0;
}

struct platform_driver mtk_mipi_tx_driver = {
	.probe = mtk_mipi_tx_probe,
	.remove = mtk_mipi_tx_remove,
	.driver = {
		.name = "mediatek-mipi-tx",
		.of_match_table = mtk_mipi_tx_match,
	},
};
