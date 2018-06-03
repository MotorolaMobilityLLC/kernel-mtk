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

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_pci.h>
#include <linux/of_platform.h>
#include <mt-plat/mtk_devinfo.h>

#define PHY_REG_GPIO_CTLD       (0x30C)

#define REG_SSUSB_IP_SW_RST     (1 << 31)

#define PHYA_BASE               (0xB00)
#define PHYD_BASE               (0x900)
#define PHYA_DA_BASE            (0xC00)

#define PHYD_IMPCAL0                    (PHYD_BASE + 0x10)
#define PHYD_IMPCAL1                    (PHYD_BASE + 0x14)
#define DA_SSUSB_PLL_SSC_DELTA1         (PHYA_DA_BASE + 0x38)
#define DA_SSUSB_PLL_SSC_DELTA          (PHYA_DA_BASE + 0x44)

#define RG_SSUSB_TX_IMPSEL_OFST         (24)
#define RG_SSUSB_RX_IMPSEL_OFST         (24)
#define RG_SSUSB_IEXT_INTR_CTRL_OFST    (10)

#define RG_SSUSB_RX_IMPSEL_MASK         (0x1f << RG_SSUSB_RX_IMPSEL_OFST)       /* 28:24 */
#define RG_SSUSB_TX_IMPSEL_MASK         (0x1f << RG_SSUSB_TX_IMPSEL_OFST)       /* 28:24 */
#define RG_SSUSB_IEXT_INTR_CTRL_MASK    (0x3f << RG_SSUSB_IEXT_INTR_CTRL_OFST)  /* 15:10 */


struct mtk_pcie_phy;

struct mtk_pcie_phy_data {
	unsigned int setting0;
	int (*init)(struct mtk_pcie_phy *mtk_pcie);
	int (*power_on)(struct mtk_pcie_phy *mtk_pcie);
	int (*power_off)(struct mtk_pcie_phy *mtk_pcie);
};

struct mtk_pcie_phy {
	struct device *dev;
	void __iomem *phy_base;

	struct phy *phy;
	const struct mtk_pcie_phy_data *phy_data;

	struct clk *clk;
};

static inline u32 phy_read(struct mtk_pcie_phy *mtk_pcie, u32 reg)
{
	return readl(mtk_pcie->phy_base + reg);
}

static inline void phy_write(struct mtk_pcie_phy *mtk_pcie, u32 val, u32 reg)
{
	writel(val, mtk_pcie->phy_base + reg);
}

int a60931_pcie_phy_init(struct mtk_pcie_phy *mtk_pcie)
{
	struct device *dev = mtk_pcie->dev;
	u32 evalue;

	dev_info(dev, "%s\n", __func__);

	/* de-assert ip sw reset. */
	phy_write(mtk_pcie,
		phy_read(mtk_pcie, PHY_REG_GPIO_CTLD) & (~REG_SSUSB_IP_SW_RST),
		PHY_REG_GPIO_CTLD);

	phy_write(mtk_pcie,
		(phy_read(mtk_pcie, DA_SSUSB_PLL_SSC_DELTA1) & 0x0000FFFF) | 0x00660000,
		DA_SSUSB_PLL_SSC_DELTA1
		);

	phy_write(mtk_pcie,
		0x00660066,
		DA_SSUSB_PLL_SSC_DELTA1 + 4
		);

	phy_write(mtk_pcie,
		(phy_read(mtk_pcie, DA_SSUSB_PLL_SSC_DELTA1 + 8) & 0xFFFF0000) | 0x0066,
		DA_SSUSB_PLL_SSC_DELTA1 + 8
		);

	phy_write(mtk_pcie,
		0x00620062,
		DA_SSUSB_PLL_SSC_DELTA
		);

	phy_write(mtk_pcie,
		0x00620062,
		DA_SSUSB_PLL_SSC_DELTA + 4
		);

	/* TODO. read phy setting by chips from efuse and apply to phy register. */

	evalue = get_devinfo_with_index(114);
	if (evalue == 0) {
		dev_warn(dev, "ignore efuse update\n");
		return 0;
	}

	/* [21:16] (BGR_code) => RG_SSUSB_IEXT_INTR_CTRL[5:0] */
	evalue = (get_devinfo_with_index(114) & (0x3f << 16)) >> 16;
	dev_warn(dev, "apply efuse setting, rg_ssusb_iext_intr_ctrl=0x%x\n", evalue);
	phy_write(mtk_pcie,
		(phy_read(mtk_pcie, PHYA_BASE) & ~RG_SSUSB_IEXT_INTR_CTRL_MASK) |
								(evalue << RG_SSUSB_IEXT_INTR_CTRL_OFST),
		PHYA_BASE
		);

	/* [12:8] (RX_50_code) => RG_SSUSB_IEXT_RX_IMPSEL[4:0] */
	evalue = (get_devinfo_with_index(114) & (0x1f << 8)) >> 8;
	dev_warn(dev, "apply efuse setting, rg_ssusb_rx_impsel=0x%x\n", evalue);
	phy_write(mtk_pcie,
		(phy_read(mtk_pcie, PHYD_IMPCAL1) & ~RG_SSUSB_RX_IMPSEL_MASK) |
								(evalue << RG_SSUSB_RX_IMPSEL_OFST),
		PHYD_IMPCAL1
		);

	/* [4:0] (TX_50_code) => RG_SSUSB_IEXT_TX_IMPSEL[4:0], don't care : 0-bit */
	evalue = (get_devinfo_with_index(114) & (0x1f << 0)) >> 0;
	dev_warn(dev, "apply efuse setting, rg_ssusb_tx_impsel=0x%x\n", evalue);
	phy_write(mtk_pcie,
		(phy_read(mtk_pcie, PHYD_IMPCAL0) & ~RG_SSUSB_TX_IMPSEL_MASK) |
								(evalue << RG_SSUSB_TX_IMPSEL_OFST),
		PHYD_IMPCAL0
		);

	return 0;
}

int a60931_pcie_phy_power_on(struct mtk_pcie_phy *mtk_pcie)
{
	struct device *dev = mtk_pcie->dev;

	dev_info(dev, "%s\n", __func__);

/*
 *      phy_write(mtk_pcie,
 *              phy_read(mtk_pcie, PHY_REG_GPIO_CTLD) & (~REG_SSUSB_IP_SW_RST),
 *              PHY_REG_GPIO_CTLD);
 */
	return 0;
}

int a60931_pcie_phy_power_off(struct mtk_pcie_phy *mtk_pcie)
{
	struct device *dev = mtk_pcie->dev;

	dev_info(dev, "%s\n", __func__);
/*
 *      phy_write(mtk_pcie,
 *              phy_read(mtk_pcie, PHY_REG_GPIO_CTLD) | REG_SSUSB_IP_SW_RST,
 *              PHY_REG_GPIO_CTLD);
 */
	return 0;
}

struct mtk_pcie_phy_data a60931_pcie_phy_data = {
	.setting0       = 0,
	.init           = a60931_pcie_phy_init,
	.power_on       = a60931_pcie_phy_power_on,
	.power_off      = a60931_pcie_phy_power_off
};

static struct phy *mtk_pcie_phy_xlate(struct device *dev,
					struct of_phandle_args *args)
{
	struct mtk_pcie_phy *mtk_pcie = dev_get_drvdata(dev);


	return mtk_pcie->phy;
}

static int mtk_pcie_phy_init(struct phy *phy)
{
	struct mtk_pcie_phy *mtk_pcie = phy_get_drvdata(phy);
	unsigned int ret;

	ret = mtk_pcie->phy_data->init(mtk_pcie);

	return ret;
}

static int mtk_pcie_phy_power_on(struct phy *phy)
{
	struct mtk_pcie_phy *mtk_pcie = phy_get_drvdata(phy);
	unsigned int ret;

	ret = mtk_pcie->phy_data->power_on(mtk_pcie);

	return ret;
}

static int mtk_pcie_phy_power_off(struct phy *phy)
{
	struct mtk_pcie_phy *mtk_pcie = phy_get_drvdata(phy);
	unsigned int ret;

	ret = mtk_pcie->phy_data->power_off(mtk_pcie);

	return 0;
}

static struct phy_ops mtk_pcie_phy_ops = {
	.init           = mtk_pcie_phy_init,
	.power_on       = mtk_pcie_phy_power_on,
	.power_off      = mtk_pcie_phy_power_off,
	.owner          = THIS_MODULE,
};

static const struct of_device_id mtk_pcie_phy_id_table[] = {
	{ .compatible = "mediatek,a60931", .data = &a60931_pcie_phy_data },
	/*
	 * { .compatible = "mediatek,a60xxx", .data = &a60xxx_pcie_phy_data },
	 */
	{ },
};
MODULE_DEVICE_TABLE(of, mtk_pcie_phy_id_table);

static int mtk_pcie_phy_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct phy_provider *phy_provider;
	struct resource *phy_res;
	struct mtk_pcie_phy *mtk_pcie;
	const struct of_device_id *match;

	match = of_match_device(mtk_pcie_phy_id_table, dev);
	if (!match || !match->data)
		return -EINVAL;

	mtk_pcie = devm_kzalloc(dev, sizeof(*mtk_pcie), GFP_KERNEL);
	if (!mtk_pcie)
		return -ENOMEM;

	mtk_pcie->dev = dev;
	mtk_pcie->phy_data = match->data;

	phy_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	mtk_pcie->phy_base = devm_ioremap_resource(dev, phy_res);

	if (IS_ERR(mtk_pcie->phy_base)) {
		dev_err(dev, "failed to remap phy register\n");
		return PTR_ERR(mtk_pcie->phy_base);
	}

	mtk_pcie->phy = devm_phy_create(mtk_pcie->dev, NULL, &mtk_pcie_phy_ops);
	if (IS_ERR(mtk_pcie->phy)) {
		dev_err(&pdev->dev, "Failed to create PHY\n");
		return PTR_ERR(mtk_pcie->phy);
	}
	phy_set_drvdata(mtk_pcie->phy, mtk_pcie);

	/* Probably we don't need this. multiple phy support (pcie/usb/sata)? */
	phy_provider = devm_of_phy_provider_register(mtk_pcie->dev,
						     mtk_pcie_phy_xlate);
	if (IS_ERR(phy_provider)) {
		dev_err(dev, "Failed to register phy provider\n");
		return PTR_ERR(phy_provider);
	}

	platform_set_drvdata(pdev, mtk_pcie);

	return 0;
}

static int mtk_pcie_phy_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver mtk_pcie_phy_driver = {
	.probe  = mtk_pcie_phy_probe,
	.remove = mtk_pcie_phy_remove,
	.driver = {
		.name    = "mtk-pcie-phy",
		.of_match_table = mtk_pcie_phy_id_table,
	},
};

module_platform_driver(mtk_pcie_phy_driver);

MODULE_DESCRIPTION("Mediatek PCIe PHY driver");
MODULE_LICENSE("GPL v2");

