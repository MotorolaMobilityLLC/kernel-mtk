/*
 * Copyright (c) 2015-2016 MediaTek Inc.
 * Author: Yong Wu <yong.wu@mediatek.com>
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
#include <linux/of.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/atomic.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>

#include <soc/mediatek/smi.h>

struct mtk_smi_dev *common;
struct mtk_smi_dev **larbs;

int mtk_smi_clk_ref_cnts_read(struct mtk_smi_dev *smi)
{
	/* check parameter */
	if (!smi || !smi->dev) {
		pr_info("%s %d no such device or address\n",
			smi->index == common->index ? "common" : "larb",
			smi->index);
		return -ENXIO;
	}
	/* read reference counts */
	return (int)atomic_read(&(smi->clk_ref_cnts));
}
EXPORT_SYMBOL_GPL(mtk_smi_clk_ref_cnts_read);

int mtk_smi_dev_enable(struct mtk_smi_dev *smi)
{
	int	i, j, ret;
	/* check parameter */
	if (!smi || !smi->dev || !smi->clks) {
		pr_info("%s %d no such device or address\n",
			smi->index == common->index ? "common" : "larb",
			smi->index);
		return -ENXIO;
	}
	/* enable clocks without mtcmos */
	for (i = 1; i < smi->nr_clks; i++) {
		ret = clk_prepare_enable(smi->clks[i]);
		if (ret) {
			dev_notice(smi->dev, "%s %d clk %d enable failed %d\n",
				smi->index == common->index ? "common" : "larb",
				smi->index, i, ret);
			break;
		}
	}
	/* rollback */
	if (ret)
		for (j = i - 1; j >= 0; j--)
			clk_disable_unprepare(smi->clks[j]);
	/* add one for reference counts */
	atomic_inc(&(smi->clk_ref_cnts));
	return ret;
}
EXPORT_SYMBOL_GPL(mtk_smi_dev_enable);

int mtk_smi_dev_disable(struct mtk_smi_dev *smi)
{
	int	i;
	/* check parameter */
	if (!smi || !smi->dev || !smi->clks) {
		pr_info("%s %d no such device or address\n",
			smi->index == common->index ? "common" : "larb",
			smi->index);
		return -ENXIO;
	}
	/* disable clocks without mtcmos */
	for (i = smi->nr_clks - 1; i >= 1; i--)
		clk_disable_unprepare(smi->clks[i]);
	/* sub one for reference counts */
	atomic_dec(&(smi->clk_ref_cnts));
	return 0;
}
EXPORT_SYMBOL_GPL(mtk_smi_dev_disable);

static int mtk_smi_clks_get(struct mtk_smi_dev *smi)
{
	struct property	*prop;
	const char	*name, *clk_names = "clock-names";
	int		i = 0, ret = 0;
	/* check parameter */
	if (!smi || !smi->dev) {
		pr_info("%s %d no such device or address\n",
			smi->index == common->index ? "common" : "larb",
			smi->index);
		return -ENXIO;
	}
	/* count number of clocks */
	smi->nr_clks = of_property_count_strings(smi->dev->of_node, clk_names);
	if (!smi->nr_clks)
		return ret;
#if IS_ENABLED(CONFIG_MACH_MT6758)
	/* workaround for mmdvfs at mt6758 */
	if (smi->index == common->index)
		smi->nr_clks = 4;
#endif
	/* allocate and get clks */
	smi->clks = devm_kcalloc(smi->dev, smi->nr_clks, sizeof(struct clk *),
		GFP_KERNEL);
	if (!smi->clks)
		return -ENOMEM;

	of_property_for_each_string(smi->dev->of_node, clk_names, prop, name) {
		smi->clks[i] = devm_clk_get(smi->dev, name);
		if (IS_ERR(smi->clks[i])) {
			dev_notice(smi->dev, "%s %d clks[%d]=%s get failed\n",
				smi->index == common->index ? "common" : "larb",
				smi->index, i, name);
			ret += 1;
		} else
			dev_dbg(smi->dev, "%s %d clks[%d]=%s\n",
				smi->index == common->index ? "common" : "larb",
				smi->index, i, name);
		i += 1;
		if (i == smi->nr_clks)
			break;
	}
	if (ret)
		return PTR_ERR(smi->clks);
	/* init zero for reference counts */
	atomic_set(&(smi->clk_ref_cnts), 0);
	return ret;
}

static int mtk_smi_config_get(struct mtk_smi_dev *smi)
{
	unsigned int	val, col;
	const __be32	*cur;
	struct property	*prop;
	const char	*name[2] = {"nr_config_pairs", "config_pairs"};
	int		i = 0, ret;
	/* check parameter */
	if (!smi || !smi->dev) {
		pr_info("%s %d no such device or address\n",
			smi->index == common->index ? "common" : "larb",
			smi->index);
		return -ENXIO;
	}
	/* read nr_config_pairs */
	ret = of_property_read_u32(smi->dev->of_node, name[0], &val);
	if (ret) {
		dev_notice(smi->dev, "%s %d %s %d read failed %d\n",
			smi->index == common->index ? "common" : "larb",
			smi->index, name[0], val, ret);
		return ret;
	}
	smi->nr_config_pairs = val;
	if (!smi->nr_config_pairs)
		return ret;
	/* allocate and get config_pairs */
	smi->config_pairs = devm_kcalloc(smi->dev, smi->nr_config_pairs,
		sizeof(struct mtk_smi_pair *), GFP_KERNEL);
	if (!smi->config_pairs)
		return -ENOMEM;

	of_property_for_each_u32(smi->dev->of_node, name[1], prop, cur, val) {
		col = i % smi->nr_config_pairs;
		if (!(i / smi->nr_config_pairs))
			smi->config_pairs[col].offset = val;
		else
			smi->config_pairs[col].value = val;
		i += 1;
	}
	return ret;
}

int mtk_smi_config_set(struct mtk_smi_dev *smi, const unsigned int scen_indx,
	const bool mtcmos)
{
	static int		mmu;
	struct mtk_smi_pair	*pairs;
	unsigned int		nr_pairs;
	int			i, ret = 0;
	/* check parameter */
	if (!smi || !smi->dev || !smi->config_pairs) {
		pr_info("%s %d no such device or address\n",
			smi->index == common->index ? "common" : "larb",
			smi->index);
		return -ENXIO;
	} else if (scen_indx > smi->nr_scens) {
		dev_info(smi->dev, "%s %d invalid scen_indx %d > nr_scens %d\n",
			smi->index == common->index ? "common" : "larb",
			smi->index, scen_indx, smi->nr_scens);
		return -EINVAL;
	}
	/* nr_pairs and pairs */
	nr_pairs = (scen_indx == smi->nr_scens) ?
		smi->nr_config_pairs : smi->nr_scen_pairs;
	if (scen_indx == smi->nr_scens && smi->config_pairs)
		pairs = smi->config_pairs;
	else if (smi->scen_pairs && smi->scen_pairs[scen_indx])
		pairs = smi->scen_pairs[scen_indx];
	else
		pairs = NULL;
	if (!nr_pairs || !pairs)
		return ret;
	/* write configs */
	ret = smi_bus_prepare_enable(smi->index, "MTK_SMI", mtcmos);
	if (ret)
		return ret;

	for (i = 0; i < nr_pairs; i++) {
		unsigned int prev, curr;

		prev = readl(smi->base + pairs[i].offset);
		if (mmu > common->index ||
			pairs[i].offset < 0x380 || pairs[i].offset >= 0x400)
			writel(pairs[i].value, smi->base + pairs[i].offset);
		else
			continue;
		curr = readl(smi->base + pairs[i].offset);
		dev_dbg(smi->dev, "%s %d pairs[%d] %#x=%#x->%#x->%#x\n",
			smi->index == common->index ? "common" : "larb",
			smi->index, i, pairs[i].offset,
			prev, pairs[i].value, curr);
	}
	if (mmu <= common->index)
		mmu += 1;

	ret = smi_bus_disable_unprepare(smi->index, "MTK_SMI", mtcmos);
	if (ret)
		return ret;
	return ret;
}
EXPORT_SYMBOL_GPL(mtk_smi_config_set);

static int mtk_smi_larb_probe(struct platform_device *pdev)
{
	struct resource	*res;
	unsigned int	index;
	int		ret;
	/* check parameter */
	if (!pdev) {
		pr_notice("platform_device larb missed\n");
		return -ENODEV;
	}
	/* index */
	ret = of_property_read_u32(pdev->dev.of_node, "cell-index", &index);
	if (ret) {
		dev_notice(&pdev->dev, "larb index %d read failed %d\n",
			index, ret);
		return ret;
	}
	/* dev */
	larbs[index] = devm_kzalloc(&pdev->dev, sizeof(struct mtk_smi_dev *),
		GFP_KERNEL);
	if (!larbs[index])
		return -ENOMEM;
	larbs[index]->dev = &pdev->dev;
	larbs[index]->index = index;
	/* base */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	larbs[index]->base = devm_ioremap_resource(larbs[index]->dev, res);
	if (IS_ERR(larbs[index]->base)) {
		dev_notice(&pdev->dev, "larb %d base 0x%p read failed %d\n",
			larbs[index]->index, larbs[index]->base, ret);
		return PTR_ERR(larbs[index]->base);
	}
	ret = of_address_to_resource(larbs[index]->dev->of_node, 0, res);
	dev_dbg(&pdev->dev, "larb %d base va=0x%p, pa=%pa\n",
		larbs[index]->index, larbs[index]->base, &res->start);
	/* clks */
	ret = mtk_smi_clks_get(larbs[index]);
	if (ret)
		return ret;
	/* config */
	ret = mtk_smi_config_get(larbs[index]);
	if (ret)
		return ret;
	/* nr_scens and set config */
	ret = of_property_read_u32(larbs[index]->dev->of_node, "nr_scens",
		&larbs[index]->nr_scens);
	if (ret) {
		dev_notice(&pdev->dev, "larb %d nr_scens %d read failed %d\n",
			larbs[index]->index, larbs[index]->nr_scens, ret);
		return ret;
	}
	ret = mtk_smi_config_set(larbs[index], larbs[index]->nr_scens, true);
	if (ret)
		return ret;
	/* device set driver data */
	platform_set_drvdata(pdev, larbs[index]);
	return ret;
}

static const struct of_device_id mtk_smi_larb_of_ids[] = {
	{
		.compatible = "mediatek,smi_larb",
	},
	{}
};

static struct platform_driver mtk_smi_larb_driver = {
	.probe	= mtk_smi_larb_probe,
	.driver	= {
		.name = "mtk-smi-larb",
		.of_match_table = mtk_smi_larb_of_ids,
	}
};

static int mtk_smi_common_probe(struct platform_device *pdev)
{
	struct resource	*res;
	unsigned int	nr_larbs;
	int		ret;
	/* check parameter */
	if (!pdev) {
		pr_notice("platform_device common missed\n");
		return -ENODEV;
	}
	/* dev */
	common = devm_kzalloc(&pdev->dev, sizeof(struct mtk_smi_dev *),
		GFP_KERNEL);
	if (!common)
		return -ENOMEM;
	common->dev = &pdev->dev;
	/* index */
	ret = of_property_read_u32(common->dev->of_node, "nr_larbs", &nr_larbs);
	if (ret) {
		dev_notice(&pdev->dev, "common nr_larbs %d read failed %d\n",
			nr_larbs, ret);
		return ret;
	}
	common->index = nr_larbs;
	/* base */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	common->base = devm_ioremap_resource(common->dev, res);
	if (IS_ERR(common->base)) {
		dev_notice(&pdev->dev, "common %d base 0x%p read failed %d\n",
			common->index, common->base, ret);
		return PTR_ERR(common->base);
	}
	ret = of_address_to_resource(common->dev->of_node, 0, res);
	dev_dbg(&pdev->dev, "common %d base va=0x%p, pa=%pa\n",
		common->index, common->base, &res->start);
	/* clks */
	ret = mtk_smi_clks_get(common);
	if (ret)
		return ret;
	/* config */
	ret = mtk_smi_config_get(common);
	if (ret)
		return ret;
	/* nr_scens and set config */
	ret = of_property_read_u32(common->dev->of_node, "nr_scens",
		&common->nr_scens);
	if (ret) {
		dev_notice(&pdev->dev, "common %d nr_scens %d read failed %d\n",
			common->index, common->nr_scens, ret);
		return ret;
	}
	ret = mtk_smi_config_set(common, common->nr_scens, true);
	if (ret)
		return ret;
	/* larbs */
	larbs = devm_kcalloc(common->dev, common->index,
		sizeof(struct mtk_smi_dev *), GFP_KERNEL);
	if (!larbs)
		return -ENOMEM;
	/* device set driver data */
	platform_set_drvdata(pdev, common);
	return ret;
}

static const struct of_device_id mtk_smi_common_of_ids[] = {
	{
		.compatible = "mediatek,smi_common",
	},
	{}
};

static struct platform_driver mtk_smi_common_driver = {
	.probe	= mtk_smi_common_probe,
	.driver	= {
		.name = "mtk-smi-common",
		.of_match_table = mtk_smi_common_of_ids,
	}
};

static int __init mtk_smi_init(void)
{
	int ret;

	ret = platform_driver_register(&mtk_smi_common_driver);
	if (ret) {
		pr_notice("Failed to register SMI driver\n");
		return ret;
	}

	ret = platform_driver_register(&mtk_smi_larb_driver);
	if (ret) {
		pr_notice("Failed to register SMI-LARB driver\n");
		platform_driver_unregister(&mtk_smi_common_driver);
		return ret;
	}
#if IS_ENABLED(CONFIG_MTK_SMI_EXT)
	ret = smi_register(&mtk_smi_common_driver);
	if (ret) {
		pr_notice("Failed to register SMI-EXT driver\n");
		platform_driver_unregister(&mtk_smi_larb_driver);
		platform_driver_unregister(&mtk_smi_common_driver);
		return ret;
	}
#endif
	return ret;
}

static void __exit mtk_smi_exit(void)
{
	platform_driver_unregister(&mtk_smi_larb_driver);
	platform_driver_unregister(&mtk_smi_common_driver);
	smi_unregister(&mtk_smi_common_driver);
}
arch_initcall_sync(mtk_smi_init);
module_exit(mtk_smi_exit);
MODULE_DESCRIPTION("SMI");
MODULE_LICENSE("GPL");
