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
#include <linux/clk.h>
#include <linux/component.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <soc/mediatek/smi.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/fs.h>

#define MT8173_SMI_LARB_NR	6
#define MT8167_SMI_LARB_NR	3
#define MTK_SMI_LARB_NR_MAX	8
#define MT8173_MMU_EN	0xf00
#define MT8167_MMU_EN	0xfc0
#define MT8167_LARB0_OFF	0
#define MT8167_LARB1_OFF	8
#define MT8167_LARB2_OFF	21

struct mtk_smi {
	struct device	*dev;
	struct clk	*clk_apb, *clk_smi;
};

struct mtk_larb_plat {
	int larb_nr;
	u32 mmu_offset;
	int port_in_larb[MTK_LARB_NR_MAX + 1];
};

struct mtk_smi_larb { /* larb: local arbiter */
	struct mtk_smi  smi;
	void __iomem	*base;
	struct device	*smi_common_dev;
	u32		*mmu;
	const struct mtk_larb_plat *mt_plat;
	int		larbid;
};

struct mtk_larb_dev {
	struct device *dev;
	int larbid;
};

static struct mtk_larb_dev gmtk_larb_dev[MTK_SMI_LARB_NR_MAX];

static int mtk_smi_enable(const struct mtk_smi *smi)
{
	int ret;

	ret = pm_runtime_get_sync(smi->dev);
	if (ret < 0)
		return ret;

	ret = clk_prepare_enable(smi->clk_apb);
	if (ret)
		goto err_put_pm;

	ret = clk_prepare_enable(smi->clk_smi);
	if (ret)
		goto err_disable_apb;

	return 0;

err_disable_apb:
	clk_disable_unprepare(smi->clk_apb);
err_put_pm:
	pm_runtime_put_sync(smi->dev);
	return ret;
}

static void mtk_smi_disable(const struct mtk_smi *smi)
{
	clk_disable_unprepare(smi->clk_smi);
	clk_disable_unprepare(smi->clk_apb);
	pm_runtime_put_sync(smi->dev);
}

int mtk_smi_larb_get(struct device *larbdev)
{
	struct mtk_smi_larb *larb = dev_get_drvdata(larbdev);
	struct mtk_smi *common = dev_get_drvdata(larb->smi_common_dev);
	int ret;

	/* Enable the smi-common's power and clocks */
	ret = mtk_smi_enable(common);
	if (ret)
		return ret;

	/* Enable the larb's power and clocks */
	ret = mtk_smi_enable(&larb->smi);
	if (ret) {
		mtk_smi_disable(common);
		return ret;
	}

	/* larb is not bind yet, we only enable clock and power for now. */
	if (!larb->mmu)
		return 0;

#ifdef M4U_TEE_SERVICE_ENABLE
		{
			int i;

			for (i = 0; i < 32; i++)
				if (BIT(i) & (*larb->mmu)) {
#ifdef CONFIG_MACH_MT8167
					/*
					 * for mt8167, we need to get the global larbid
					 * for trustzone to config the port.
					 */
					pseudo_config_port_tee(i + larb->mt_plat->port_in_larb[larb->larbid]);
#else
					pseudo_config_port_tee(i + (larb->larbid << 5));
#endif
				}
		}
#else
	/* Configure the iommu info for this larb */
	writel(*larb->mmu, larb->base + larb->mt_plat->mmu_offset);
#endif
	return 0;
}

void mtk_smi_larb_put(struct device *larbdev)
{
	struct mtk_smi_larb *larb = dev_get_drvdata(larbdev);
	struct mtk_smi *common = dev_get_drvdata(larb->smi_common_dev);

	/*
	 * Don't de-configure the iommu info for this larb since there may be
	 * several modules in this larb.
	 * The iommu info will be reset after power off.
	 */

	mtk_smi_disable(&larb->smi);
	mtk_smi_disable(common);
}

int mtk_smi_larb_clock_on(int larbid, bool pm)
{
	struct device *dev = gmtk_larb_dev[larbid].dev;

	if (!dev)
		return -1;

	return mtk_smi_larb_get(dev);
}

int mtk_smi_larb_ready(int larbid)
{
	struct device *dev = gmtk_larb_dev[larbid].dev;

	if (dev)
		return 1;

	return 0;
}

void mtk_smi_larb_clock_off(int larbid, bool pm)
{
	struct device *dev = gmtk_larb_dev[larbid].dev;

	if (!dev)
		return;

	mtk_smi_larb_put(dev);
}

static int
mtk_smi_larb_bind(struct device *dev, struct device *master, void *data)
{
	struct mtk_smi_larb *larb = dev_get_drvdata(dev);
	struct mtk_smi_iommu *smi_iommu = data;
	unsigned int         i;

	for (i = 0; i < smi_iommu->larb_nr; i++) {
		if (dev == smi_iommu->larb_imu[i].dev) {
			/* The 'mmu' may be updated in iommu-attach/detach. */
			larb->mmu = &smi_iommu->larb_imu[i].mmu;
			return 0;
		}
	}
	return -ENODEV;
}

static void
mtk_smi_larb_unbind(struct device *dev, struct device *master, void *data)
{
	/* Do nothing as the iommu is always enabled. */
}

static const struct component_ops mtk_smi_larb_component_ops = {
	.bind = mtk_smi_larb_bind,
	.unbind = mtk_smi_larb_unbind,
};

static const struct mtk_larb_plat mt8173_larb_plat = {
	.larb_nr = MT8173_SMI_LARB_NR,
	.mmu_offset = MT8173_MMU_EN,
};

static const struct mtk_larb_plat mt8167_larb_plat = {
	.larb_nr = MT8167_SMI_LARB_NR,
	.mmu_offset = MT8167_MMU_EN,
	.port_in_larb = {
		MT8167_LARB0_OFF, MT8167_LARB1_OFF, MT8167_LARB2_OFF
	},
};

static const struct of_device_id mtk_smi_larb_of_ids[] = {
	{ .compatible = "mediatek,mt8173-smi-larb", .data = &mt8173_larb_plat },
	{ .compatible = "mediatek,mt8167-smi-larb", .data = &mt8167_larb_plat },
	{}
};

static int mtk_smi_larb_probe(struct platform_device *pdev)
{
	struct mtk_smi_larb *larb;
	struct resource *res;
	struct device *dev = &pdev->dev;
	struct device_node *smi_node;
	struct platform_device *smi_pdev;
	const struct of_device_id *of_id;
	int ret, larbid;

	if (!dev->pm_domain)
		return -EPROBE_DEFER;

	of_id = of_match_node(mtk_smi_larb_of_ids, pdev->dev.of_node);
	if (!of_id)
		return -EINVAL;

	larb = devm_kzalloc(dev, sizeof(*larb), GFP_KERNEL);
	if (!larb)
		return -ENOMEM;

	larb->mt_plat = of_id->data;
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	larb->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(larb->base))
		return PTR_ERR(larb->base);

	larb->smi.clk_apb = devm_clk_get(dev, "apb");
	if (IS_ERR(larb->smi.clk_apb))
		return PTR_ERR(larb->smi.clk_apb);

	larb->smi.clk_smi = devm_clk_get(dev, "smi");
	if (IS_ERR(larb->smi.clk_smi))
		return PTR_ERR(larb->smi.clk_smi);
	larb->smi.dev = dev;

	ret = of_property_read_u32(dev->of_node, "mediatek,larbid", &larbid);
	if (ret)
		return ret;

	smi_node = of_parse_phandle(dev->of_node, "mediatek,smi", 0);
	if (!smi_node)
		return -EINVAL;

	smi_pdev = of_find_device_by_node(smi_node);
	of_node_put(smi_node);
	if (smi_pdev) {
		/* wait for smi probe done to get the smi common dev. */
		if (!(&smi_pdev->dev))
			return -EPROBE_DEFER;

		larb->smi_common_dev = &smi_pdev->dev;
	} else {
		dev_err(dev, "Failed to get the smi_common device\n");
		return -EINVAL;
	}

	pm_runtime_enable(dev);
	platform_set_drvdata(pdev, larb);

	gmtk_larb_dev[larbid].dev = &pdev->dev;
	gmtk_larb_dev[larbid].larbid = larbid;
	larb->larbid = larbid;

	return component_add(dev, &mtk_smi_larb_component_ops);
}

static int mtk_smi_larb_remove(struct platform_device *pdev)
{
	pm_runtime_disable(&pdev->dev);
	component_del(&pdev->dev, &mtk_smi_larb_component_ops);
	return 0;
}

static struct platform_driver mtk_smi_larb_driver = {
	.probe	= mtk_smi_larb_probe,
	.remove = mtk_smi_larb_remove,
	.driver	= {
		.name = "mtk-smi-larb",
		.of_match_table = mtk_smi_larb_of_ids,
	}
};

static int mtk_smi_common_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_smi *common;

	if (!dev->pm_domain)
		return -EPROBE_DEFER;

	common = devm_kzalloc(dev, sizeof(*common), GFP_KERNEL);
	if (!common)
		return -ENOMEM;
	common->dev = dev;

	common->clk_apb = devm_clk_get(dev, "apb");
	if (IS_ERR(common->clk_apb))
		return PTR_ERR(common->clk_apb);

	common->clk_smi = devm_clk_get(dev, "smi");
	if (IS_ERR(common->clk_smi))
		return PTR_ERR(common->clk_smi);

	pm_runtime_enable(dev);
	/*
	 * Without pm_runtime_get_sync(dev), the disp power domain
	 * would be turn off after pm_runtime_enable, meanwhile disp
	 * hw are still access register, this would cause system
	 * abnormal.
	 *
	 * If we do not call pm_runtime_get_sync, then system would hang
	 * in larb0's power domain attach, power domain SA and DE are
	 * still checking that. We would like to bypass this first and
	 * don't block the software flow.
	 */
	pm_runtime_get_sync(dev);
	platform_set_drvdata(pdev, common);
	return 0;
}

static int mtk_smi_common_remove(struct platform_device *pdev)
{
	pm_runtime_disable(&pdev->dev);
	return 0;
}

static const struct of_device_id mtk_smi_common_of_ids[] = {
	{ .compatible = "mediatek,mt8173-smi-common", },
	{ .compatible = "mediatek,mt8167-smi-common", },
	{}
};

static struct platform_driver mtk_smi_common_driver = {
	.probe	= mtk_smi_common_probe,
	.remove = mtk_smi_common_remove,
	.driver	= {
		.name = "mtk-smi-common",
		.of_match_table = mtk_smi_common_of_ids,
	}
};

static int __init mtk_smi_init(void)
{
	int ret;

	ret = platform_driver_register(&mtk_smi_common_driver);
	if (ret != 0) {
		pr_err("Failed to register SMI driver\n");
		return ret;
	}

	ret = platform_driver_register(&mtk_smi_larb_driver);
	if (ret != 0) {
		pr_err("Failed to register SMI-LARB driver\n");
		goto err_unreg_smi;
	}
	return ret;

err_unreg_smi:
	platform_driver_unregister(&mtk_smi_common_driver);
	return ret;
}

#define MTK_SMI_MAJOR_NUMBER 190

static dev_t smidev_num = MKDEV(MTK_SMI_MAJOR_NUMBER, 0);
static struct cdev *psmi_dev;

static int smi_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int smi_release(struct inode *inode, struct file *file)
{
	return 0;
}

static long smi_ioctl(struct file *pfile, unsigned int cmd, unsigned long param)
{
	return 0;
}

#if IS_ENABLED(CONFIG_COMPAT)
static long MTK_SMI_COMPAT_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	return 0;
}

#else
#define MTK_SMI_COMPAT_ioctl  NULL
#endif

static const struct file_operations smi_fops = {
	.owner = THIS_MODULE,
	.open = smi_open,
	.release = smi_release,
	.unlocked_ioctl = smi_ioctl,
	.compat_ioctl = MTK_SMI_COMPAT_ioctl
};

static inline int smi_register(void)
{
	if (alloc_chrdev_region(&smidev_num, 0, 1, "MTK_SMI")) {
		pr_err("Allocate device No. failed");
		return -EAGAIN;
	}
	/* Allocate driver */
	psmi_dev = cdev_alloc();

	if (psmi_dev == NULL) {
		unregister_chrdev_region(smidev_num, 1);
		pr_err("Allocate mem for kobject failed");
		return -ENOMEM;
	}
	/* Attatch file operation. */
	cdev_init(psmi_dev, &smi_fops);
	psmi_dev->owner = THIS_MODULE;

	/* Add to system */
	if (cdev_add(psmi_dev, smidev_num, 1)) {
		pr_err("Attatch file operation failed");
		unregister_chrdev_region(smidev_num, 1);
		return -EAGAIN;
	}

	return 0;
}

static struct class *psmi_class;
static int smi_dev_register(void)
{
	int ret;
	struct device *smi_device = NULL;

	if (smi_register()) {
		pr_err("register SMI failed\n");
		return -EAGAIN;
	}

	psmi_class = class_create(THIS_MODULE, "MTK_SMI");
	if (IS_ERR(psmi_class)) {
		ret = PTR_ERR(psmi_class);
		pr_err("Unable to create class, err = %d", ret);
		return ret;
	}

	smi_device = device_create(psmi_class, NULL, smidev_num, NULL, "MTK_SMI");

	return 0;
}

/* put the disp power domain that we got in smi probe */
static int __init mtk_smi_init_late(void)
{
	struct device *dev = NULL;
	struct mtk_smi_larb *larb = dev_get_drvdata(gmtk_larb_dev[0].dev);
	struct mtk_smi *common = dev_get_drvdata(larb->smi_common_dev);

	if (!larb) {
		dev_err(dev, "%s, %d\n", __func__, __LINE__);
		return -1;
	}

	/*
	 * We get_sync the disp power domain in smi common probe,
	 * need to put_sync it to avoid dis-pairing of get/put_sync
	 * for the disp power domain.
	 */
	dev = common->dev;
	pm_runtime_put_sync(dev);

	return smi_dev_register();
}

subsys_initcall(mtk_smi_init);
late_initcall(mtk_smi_init_late);
