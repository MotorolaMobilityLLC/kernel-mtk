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
#include <drm/drmP.h>
#include <drm/drm_crtc.h>
#include <drm/drm_crtc_helper.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/kernel.h>
#include <linux/component.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/clk.h>

#include "mediatek_hdmi_display_core.h"
#include "mediatek_hdmi_dpi_ctrl.h"

static struct mediatek_hdmi_display_ops hdmi_dpi_ops = {
	.init = mtk_hdmi_dpi_init,
	.set_format = mtk_hdmi_dpi_set_display_mode,
};

static int mtk_hdmi_dpi_bind(struct device *dev, struct device *master,
			     void *data)
{
	return 0;
}

static void mtk_hdmi_dpi_unbind(struct device *dev, struct device *master,
				void *data)
{
}

static const struct component_ops mediatek_hdmi_dpi_component_ops = {
	.bind = mtk_hdmi_dpi_bind,
	.unbind = mtk_hdmi_dpi_unbind,
};

static int mtk_hdmi_dpi_probe(struct platform_device *pdev)
{
	int ret;
	struct resource *mem;
	struct mediatek_hdmi_dpi_context *dpi = NULL;

	dpi =
	    devm_kzalloc(&pdev->dev, sizeof(struct mediatek_hdmi_dpi_context),
			 GFP_KERNEL);
	if (!dpi) {
		ret = -ENOMEM;
		mtk_hdmi_err("alloc memory failed!\n");
		goto errcode;
	}

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem) {
		mtk_hdmi_err("get mem resouce failed!\n");
		goto errcode;
	}

	dpi->regs = devm_ioremap_resource(&pdev->dev, mem);
	if (IS_ERR(dpi->regs)) {
		mtk_hdmi_err("mem resource ioremap failed! dpi->regs = %p\n",
			     dpi->regs);
		ret = PTR_ERR(dpi->regs);
		goto errcode;
	}

	dpi->tvd_clk = of_clk_get(pdev->dev.of_node, 0);
	if (IS_ERR(dpi->tvd_clk)) {
		mtk_hdmi_err("get clk failed! dpi->clk = %p\n", dpi->tvd_clk);
		ret = -EINVAL;
		goto errcode;
	}

	dpi->irq = of_irq_get(pdev->dev.of_node, 0);
	if (dpi->irq <= 0) {
		mtk_hdmi_err("get irq failed! dpi->irq= %d\n", dpi->irq);
		goto errcode;
	}

	ret = component_add(&pdev->dev, &mediatek_hdmi_dpi_component_ops);
	if (ret) {
		mtk_hdmi_err("component_add failed !\n");
		goto errcode;
	}

	spin_lock_init(&dpi->lock);

	dpi->display_node =
	    mtk_hdmi_display_create_node(&hdmi_dpi_ops, pdev->dev.of_node, dpi);
	if (IS_ERR(dpi->display_node)) {
		mtk_hdmi_err("create dpi failed!\n");
		return PTR_ERR(dpi->display_node);
	}

	mtk_hdmi_info("mem resource start = 0x%llx, end = 0x%llx\n", mem->start,
		      mem->end);
	mtk_hdmi_info("dpi->regs : %p\n", dpi->regs);
	mtk_hdmi_info("dpi->irq : %d\n", dpi->irq);
	mtk_hdmi_info("dpi->tvd_clk : %p\n", dpi->tvd_clk);
	mtk_hdmi_info("dpi->display_node : %p\n", dpi->display_node);

	return 0;

errcode:
	return ret;
}

static int mtk_hdmi_dpi_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id mediatek_hdmi_dpi_of_ids[] = {
	{.compatible = "mediatek,mt2701-hdmi-dpi",},
	{}
};

struct platform_driver mediate_hdmi_dpi_driver = {
	.probe = mtk_hdmi_dpi_probe,
	.remove = mtk_hdmi_dpi_remove,
	.driver = {
		   .owner = THIS_MODULE,
		   .name = "mediatek-hdmi-dpi",
		   .of_match_table = mediatek_hdmi_dpi_of_ids,
		   },
};

MODULE_AUTHOR("Jie Qiu <jie.qiu@mediatek.com>");
MODULE_DESCRIPTION("MediaTek HDMI Driver");
MODULE_LICENSE("GPL");
