/*
* Copyright (c) 2015 MediaTek Inc.
* Author: Tiffany Lin <tiffany.lin@mediatek.com>
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
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <soc/mediatek/smi.h>
#include <linux/pm_runtime.h>

#include "mtk_vcodec_pm.h"
#include "mtk_vcodec_util.h"
#include "mtk_vcodec_iommu.h"
#include "mtk_vpu_core.h"

static struct mtk_vcodec_pm *pm;

int mtk_vcodec_init_enc_pm(struct mtk_vcodec_dev *mtkdev)
{
	struct device_node *node;
	struct platform_device *pdev;
	struct device *dev;
	int ret = 0;

	pdev = mtkdev->plat_dev;
	pm = &mtkdev->pm;
	memset(pm, 0, sizeof(struct mtk_vcodec_pm));
	pm->mtkdev = mtkdev;
	dev = &pdev->dev;

	node = of_parse_phandle(dev->of_node, "larb", 0);
	if (!node)
		return -1;
	pdev = of_find_device_by_node(node);
	if (WARN_ON(!pdev)) {
		of_node_put(node);
		return -1;
	}
	pm->larbvenc = &pdev->dev;

	node = of_parse_phandle(dev->of_node, "larb", 1);
	if (!node)
		return -1;
	pdev = of_find_device_by_node(node);
	if (WARN_ON(!pdev)) {
		of_node_put(node);
		return -EINVAL;
	}
	pm->larbvenclt = &pdev->dev;

	node = of_parse_phandle(dev->of_node, "venc_pm", 0);
	if (!node)
		return -1;
	pdev = of_find_device_by_node(node);
	if (WARN_ON(!pdev)) {
		of_node_put(node);
		return -1;
	}
	pm->pmvenc = &pdev->dev;

	node = of_parse_phandle(dev->of_node, "venc_pm", 1);
	if (!node)
		return -1;
	pdev = of_find_device_by_node(node);
	if (WARN_ON(!pdev)) {
		of_node_put(node);
		return -EINVAL;
	}
	pm->pmvenclt = &pdev->dev;

	pdev = mtkdev->plat_dev;
	pm->dev = &pdev->dev;

	return ret;
}

void mtk_vcodec_release_enc_pm(struct mtk_vcodec_dev *dev)
{
	pm_runtime_disable(pm->dev);
}

void mtk_vcodec_enc_pw_on(void)
{
	int ret;

	ret = pm_runtime_get_sync(pm->pmvenc);
	if (ret)
		mtk_v4l2_err("pm_runtime_get_sync fail %s\n",
				dev_name(pm->pmvenc));

	ret = pm_runtime_get_sync(pm->pmvenclt);
	if (ret)
		mtk_v4l2_err("pm_runtime_get_sync fail %s\n",
				dev_name(pm->pmvenclt));

	mtk_vcodec_enc_clock_on();
}

void mtk_vcodec_enc_pw_off(void)
{
	int ret;

	mtk_vcodec_enc_clock_off();

	ret = pm_runtime_put_sync(pm->pmvenc);
	if (ret)
		mtk_v4l2_err("pm_runtime_put_sync fail %s\n",
				dev_name(pm->pmvenc));

	ret = pm_runtime_put_sync(pm->pmvenclt);
	if (ret)
		mtk_v4l2_err("pm_runtime_put_sync fail %s\n",
				dev_name(pm->pmvenclt));

}

void mtk_vcodec_enc_clock_on(void)
{
	int ret;

	ret = mtk_smi_larb_get(pm->larbvenc);
	if (ret)
		mtk_v4l2_err("mtk_smi_larb_get larb3 fail %d\n", ret);
	ret = mtk_smi_larb_get(pm->larbvenclt);
	if (ret)
		mtk_v4l2_err("mtk_smi_larb_get larb4 fail %d\n", ret);
	mtk_vcodec_iommu_init(pm->dev);
	ret = vpu_enable_clock(vpu_get_plat_device(pm->mtkdev->plat_dev));
	if (ret)
		mtk_v4l2_err("vpu clk fail %d for enc", ret);
}

void mtk_vcodec_enc_clock_off(void)
{

	vpu_disable_clock(vpu_get_plat_device(pm->mtkdev->plat_dev));
	mtk_smi_larb_put(pm->larbvenc);
	mtk_smi_larb_put(pm->larbvenclt);
}
