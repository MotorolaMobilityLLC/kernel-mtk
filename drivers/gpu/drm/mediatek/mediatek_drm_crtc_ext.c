/*
 * Copyright (c) 2015 MediaTek Inc.
 * Authors:
 *	YT Shen <yt.shen@mediatek.com>
 *	CK Hu <ck.hu@mediatek.com>
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
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/of_device.h>
#include <linux/component.h>
#include <linux/pm_runtime.h>
#include "mediatek_drm_drv.h"
#include "mediatek_drm_crtc.h"
#include "mediatek_drm_gem.h"
#include "mediatek_drm_ddp.h"
#include "mediatek_drm_ddp_comp.h"
#include "mediatek_drm_plane.h"


struct crtc_ext_context {
	struct device		*dev;
	struct drm_device	*drm_dev;
	struct mtk_drm_crtc	*crtc;
	struct mediatek_drm_plane		planes[OVL_LAYER_NR];
	int	pipe;

	struct device		*ddp_dev;
	struct clk		*ovl1_disp_clk;
	struct clk		*rdma1_disp_clk;
	struct clk		*color1_disp_clk;
	struct clk		*gamma_disp_clk;

	void __iomem		*ovl1_regs;
	void __iomem		*rdma1_regs;
	void __iomem		*color1_regs;
	void __iomem		*gamma_regs;

	bool			pending_config;
	unsigned int		pending_width;
	unsigned int		pending_height;

	bool			pending_ovl_config;
	bool			pending_ovl_enable;
	unsigned int		pending_ovl_addr;
	unsigned int		pending_ovl_width;
	unsigned int		pending_ovl_height;
	unsigned int		pending_ovl_pitch;
	unsigned int		pending_ovl_format;

	bool			pending_ovl_cursor_config;
	bool			pending_ovl_cursor_enable;
	unsigned int		pending_ovl_cursor_addr;
	int			pending_ovl_cursor_x;
	int			pending_ovl_cursor_y;
};


static int crtc_ext_ctx_initialize(struct crtc_ext_context *ctx,
	struct drm_device *drm_dev)
{
	struct mtk_drm_private *priv;

	priv = drm_dev->dev_private;
	ctx->drm_dev = drm_dev;
	ctx->pipe = priv->pipe++;

	return 0;
}

static void crtc_ext_ctx_remove(struct crtc_ext_context *ctx)
{
}

static void crtc_ext_power_on(struct crtc_ext_context *ctx)
{
	int ret;

	DRM_INFO("crtc_ext_power_on\n");

	ret = clk_prepare_enable(ctx->ovl1_disp_clk);
	if (ret != 0)
		DRM_ERROR("clk_prepare_enable(ctx->ovl1_disp_clk) error!\n");

	ret = clk_prepare_enable(ctx->rdma1_disp_clk);
	if (ret != 0)
		DRM_ERROR("clk_prepare_enable(ctx->rdma1_disp_clk) error!\n");

	ret = clk_prepare_enable(ctx->color1_disp_clk);
	if (ret != 0)
		DRM_ERROR("clk_prepare_enable(ctx->color1_disp_clk) error!\n");

	ret = clk_prepare_enable(ctx->gamma_disp_clk);
	if (ret != 0)
		DRM_ERROR("clk_prepare_enable(ctx->gamma_disp_clk) error!\n");
}

static void crtc_ext_power_off(struct crtc_ext_context *ctx)
{
	DRM_INFO("crtc_ext_power_off\n");
	clk_disable(ctx->ovl1_disp_clk);
	clk_unprepare(ctx->ovl1_disp_clk);

	clk_disable(ctx->rdma1_disp_clk);
	clk_unprepare(ctx->rdma1_disp_clk);

	clk_disable(ctx->color1_disp_clk);
	clk_unprepare(ctx->color1_disp_clk);

	clk_disable(ctx->gamma_disp_clk);
	clk_unprepare(ctx->gamma_disp_clk);
}

static void crtc_ext_hw_init(struct crtc_ext_context *ctx)
{
	unsigned int width, height;

	width = ctx->crtc->base.mode.hdisplay;
	height = ctx->crtc->base.mode.vdisplay;

	DRM_INFO("crtc_ext_hw_init\n");
	crtc_ext_power_on(ctx);

	DRM_INFO("mediatek_ddp_main_path_setup\n");
	mediatek_ddp_ext_path_setup(ctx->ddp_dev);

	DRM_INFO("ext_disp_path_power_on %dx%d\n", width, height);
	ext_disp_path_power_on(width, height, ctx->ovl1_regs, ctx->rdma1_regs,
			ctx->color1_regs);
}

static void crtc_ext_hw_fini(struct crtc_ext_context *ctx)
{
	DRM_INFO("crtc_ext_hw_fini\n");
	crtc_ext_power_off(ctx);
}

static void crtc_ext_dpms(struct mtk_drm_crtc *crtc, int mode)
{
	struct crtc_ext_context *ctx = (struct crtc_ext_context *)crtc->ctx;

	switch (mode) {
	case DRM_MODE_DPMS_ON:
		mediatek_ddp_clock_on(ctx->ddp_dev);
		crtc_ext_hw_init(crtc->ctx);
		break;
	case DRM_MODE_DPMS_STANDBY:
	case DRM_MODE_DPMS_SUSPEND:
	case DRM_MODE_DPMS_OFF:
		crtc_ext_hw_fini(crtc->ctx);
		mediatek_ddp_clock_off(ctx->ddp_dev);
		break;
	default:
		DRM_DEBUG_KMS("unspecified mode %d\n", mode);
		break;
	}
}

static int crtc_ext_enable_vblank(struct mtk_drm_crtc *crtc)
{
	struct crtc_ext_context *ctx = (struct crtc_ext_context *)crtc->ctx;

	mediatek_ovl_enable_vblank(ctx->ovl1_regs);

	return 0;
}

static void crtc_ext_disable_vblank(struct mtk_drm_crtc *crtc)
{
	struct crtc_ext_context *ctx = (struct crtc_ext_context *)crtc->ctx;

	mediatek_ovl_disable_vblank(ctx->ovl1_regs);
}

static void crtc_ext_commit(struct mtk_drm_crtc *crtc)
{
	struct crtc_ext_context *ctx = (struct crtc_ext_context *)crtc->ctx;

	ctx->pending_ovl_config = true;
}

static void crtc_ext_config(struct mtk_drm_crtc *crtc,
	unsigned int width, unsigned int height)
{
	struct crtc_ext_context *ctx = (struct crtc_ext_context *)crtc->ctx;

	ctx->pending_width = width;
	ctx->pending_height = height;
	ctx->pending_config = true;
}

static void crtc_ext_plane_config(struct mtk_drm_crtc *crtc,
	bool enable, dma_addr_t addr)
{
	struct crtc_ext_context *ctx = (struct crtc_ext_context *)crtc->ctx;
	unsigned int pitch = 0;
	unsigned int width = 0;
	unsigned int height = 0;

	if (crtc->base.primary->fb && crtc->base.primary->fb->pitches[0]) {
		width = crtc->base.primary->fb->width;
		height = crtc->base.primary->fb->height;
		pitch = crtc->base.primary->fb->pitches[0];
	}

	if (width == 0 || height == 0)
		enable = false;

	if (width > ctx->pending_width || height > ctx->pending_height)
		enable = false;

	ctx->pending_ovl_enable = enable;
	ctx->pending_ovl_addr = addr;
	ctx->pending_ovl_width = width;
	ctx->pending_ovl_height = height;
	ctx->pending_ovl_pitch = pitch;
	if (crtc->base.primary && crtc->base.primary->fb)
		ctx->pending_ovl_format = crtc->base.primary->fb->pixel_format;
}

static void crtc_ext_ovl_layer_config_cursor(struct mtk_drm_crtc *crtc,
	bool enable, dma_addr_t addr)
{
	struct crtc_ext_context *ctx = (struct crtc_ext_context *)crtc->ctx;

	ctx->pending_ovl_cursor_enable = enable;
	ctx->pending_ovl_cursor_addr = addr;
	ctx->pending_ovl_cursor_x = crtc->cursor_x;
	ctx->pending_ovl_cursor_y = crtc->cursor_y;
	ctx->pending_ovl_cursor_config = true;
}

static struct mediatek_drm_crtc_ops crtc_ext_crtc_ops = {
	.dpms = crtc_ext_dpms,
	.enable_vblank = crtc_ext_enable_vblank,
	.disable_vblank = crtc_ext_disable_vblank,
	.crtc_config = crtc_ext_config,
	.plane_config = crtc_ext_plane_config,
	.ovl_layer_config_cursor = crtc_ext_ovl_layer_config_cursor,
	.crtc_commit = crtc_ext_commit,
};

static void crtc_ext_irq(struct crtc_ext_context *ctx)
{
	struct drm_device *dev = ctx->drm_dev;
	struct mtk_drm_crtc *mtk_crtc = ctx->crtc;
	unsigned long flags;

	if (ctx->pending_ovl_config) {
		ctx->pending_ovl_config = false;

		if (!ctx->pending_ovl_enable)
			mediatek_ovl_layer_off(ctx->ovl1_regs);

		mediatek_ovl_layer_config(ctx->ovl1_regs,
			ctx->pending_ovl_addr,
			ctx->pending_ovl_width,
			ctx->pending_ovl_height,
			ctx->pending_ovl_pitch,
			ctx->pending_ovl_format);

		if (ctx->pending_ovl_enable)
			mediatek_ovl_layer_on(ctx->ovl1_regs);
	}

	if (ctx->pending_ovl_cursor_config) {
		ctx->pending_ovl_cursor_config = false;

		mediatek_ovl_layer_config_cursor(ctx->ovl1_regs,
			ctx->pending_ovl_cursor_enable,
			ctx->pending_ovl_cursor_addr,
			ctx->pending_ovl_cursor_x,
			ctx->pending_ovl_cursor_y,
			ctx->pending_ovl_width,
			ctx->pending_ovl_height);
	}

	if (ctx->pending_config) {
		ctx->pending_config = false;

		mediatek_ovl_layer_off(ctx->ovl1_regs);
		mediatek_ovl_layer_config_cursor(ctx->ovl1_regs,
			false, 0, 0, 0, 0, 0);

		mediatek_ovl_config(ctx->ovl1_regs,
			ctx->pending_width,
			ctx->pending_height);
	}

	drm_handle_vblank(ctx->drm_dev, ctx->pipe);
	spin_lock_irqsave(&dev->event_lock, flags);
	if (mtk_crtc->pending_needs_vblank) {
		mediatek_plane_finish_page_flip(mtk_crtc);
		mtk_crtc->pending_needs_vblank = false;
	}
	spin_unlock_irqrestore(&dev->event_lock, flags);
}

irqreturn_t crtc_ext_irq_handler(int irq, void *dev_id)
{
	struct crtc_ext_context *ctx = (struct crtc_ext_context *)dev_id;

	mediatek_ovl_clear_vblank(ctx->ovl1_regs);

	if (ctx->pipe < 0 || !ctx->drm_dev)
		goto out;

	crtc_ext_irq(ctx);
out:
	return IRQ_HANDLED;
}

static int crtc_ext_bind(struct device *dev, struct device *master, void *data)
{
	struct crtc_ext_context *ctx = dev_get_drvdata(dev);
	struct drm_device *drm_dev = data;
	enum drm_plane_type type;
	unsigned int zpos;
	int ret;

	ret = crtc_ext_ctx_initialize(ctx, drm_dev);
	if (ret) {
		DRM_ERROR("crtc_ext_ctx_initialize failed.\n");
		return ret;
	}

	for (zpos = 0; zpos < OVL_LAYER_NR; zpos++) {
		type = (zpos == 0) ? DRM_PLANE_TYPE_PRIMARY :
						DRM_PLANE_TYPE_OVERLAY;
		ret = mediatek_plane_init(drm_dev, &ctx->planes[zpos],
			1 << ctx->pipe, type, zpos, OVL_LAYER_NR);
		if (ret)
			return ret;
	}

	ctx->crtc = mtk_drm_crtc_create(drm_dev, &(ctx->planes[0].base),
		ctx->pipe, &crtc_ext_crtc_ops, ctx);

	if (IS_ERR(ctx->crtc)) {
		crtc_ext_ctx_remove(ctx);
		return PTR_ERR(ctx->crtc);
	}

	mediatek_ddp_clock_on(ctx->ddp_dev);
	crtc_ext_hw_init(ctx);

	return 0;

}

static void crtc_ext_unbind(struct device *dev, struct device *master,
			void *data)
{
	struct crtc_ext_context *ctx = dev_get_drvdata(dev);

	crtc_ext_ctx_remove(ctx);
}

static const struct component_ops crtc_ext_component_ops = {
	.bind	= crtc_ext_bind,
	.unbind = crtc_ext_unbind,
};

static int crtc_ext_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct crtc_ext_context *ctx;
	struct device_node *node;
	struct platform_device *ddp_pdev;
	struct resource *regs;
	int irq;
	int ret;

	if (!dev->of_node)
		return -ENODEV;

	ctx = devm_kzalloc(dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	node = of_parse_phandle(dev->of_node, "ddp", 0);
	if (!node) {
		dev_err(dev, "crtc_ext_probe: Get ddp node fail.\n");
		ret = -EINVAL;
		goto err;
	}

	ddp_pdev = of_find_device_by_node(node);
	if (WARN_ON(!ddp_pdev)) {
		dev_err(dev, "crtc_ext_probe: Find ddp device fail.\n");
		ret = -EINVAL;
		goto err;
	}
	ctx->ddp_dev = &ddp_pdev->dev;

	ctx->ovl1_disp_clk = devm_clk_get(dev, "ovl1_disp");
	if (IS_ERR(ctx->ovl1_disp_clk)) {
		dev_err(dev, "crtc_ext_probe: Get ovl1_disp_clk fail.\n");
		ret = PTR_ERR(ctx->ovl1_disp_clk);
		goto err;
	}

	ctx->rdma1_disp_clk = devm_clk_get(dev, "rdma1_disp");
	if (IS_ERR(ctx->rdma1_disp_clk)) {
		dev_err(dev, "crtc_ext_probe: Get rdma1_disp_clk.\n");
		ret = PTR_ERR(ctx->rdma1_disp_clk);
		goto err;
	}

	ctx->color1_disp_clk = devm_clk_get(dev, "color1_disp");
	if (IS_ERR(ctx->color1_disp_clk)) {
		dev_err(dev, "crtc_ext_probe: Get color1_disp_clk fail.\n");
		ret = PTR_ERR(ctx->color1_disp_clk);
		goto err;
	}

	ctx->gamma_disp_clk = devm_clk_get(dev, "gamma_disp");
	if (IS_ERR(ctx->gamma_disp_clk)) {
		dev_err(dev, "crtc_ext_probe: Get gamma_disp_clk fail.\n");
		ret = PTR_ERR(ctx->gamma_disp_clk);
		goto err;
	}

	regs = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	ctx->ovl1_regs = devm_ioremap_resource(dev, regs);
	if (IS_ERR(ctx->ovl1_regs)) {
		ret = PTR_ERR(ctx->ovl1_regs);
		goto err;
	}

	regs = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	ctx->rdma1_regs = devm_ioremap_resource(dev, regs);
	if (IS_ERR(ctx->rdma1_regs)) {
		ret = PTR_ERR(ctx->rdma1_regs);
		goto err;
	}

	regs = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	ctx->color1_regs = devm_ioremap_resource(dev, regs);
	if (IS_ERR(ctx->color1_regs)) {
		ret = PTR_ERR(ctx->color1_regs);
		goto err;
	}

	regs = platform_get_resource(pdev, IORESOURCE_MEM, 3);
	ctx->gamma_regs = devm_ioremap_resource(dev, regs);
	if (IS_ERR(ctx->gamma_regs)) {
		ret = PTR_ERR(ctx->gamma_regs);
		goto err;
	}

	irq = platform_get_irq(pdev, 0);
	ret = devm_request_irq(dev, irq, crtc_ext_irq_handler,
		IRQF_TRIGGER_NONE, dev_name(dev), ctx);
	if (ret < 0) {
		dev_err(dev, "devm_request_irq %d fail %d\n", irq, ret);
		ret = -ENXIO;
		goto err;
	}

	platform_set_drvdata(pdev, ctx);

	ret = component_add(&pdev->dev, &crtc_ext_component_ops);
	if (ret) {
		dev_err(dev, "component_add fail\n");
		goto err;
	}

	return 0;

err:
	if (node)
		of_node_put(node);

	return ret;
}

static int crtc_ext_remove(struct platform_device *pdev)
{
	component_del(&pdev->dev, &crtc_ext_component_ops);

	return 0;
}

static const struct of_device_id crtc_ext_driver_dt_match[] = {
	{ .compatible = "mediatek,mt2701-crtc-ext" },
	{},
};
MODULE_DEVICE_TABLE(of, crtc_ext_driver_dt_match);

struct platform_driver mediatek_crtc_ext_driver = {
	.probe		= crtc_ext_probe,
	.remove		= crtc_ext_remove,
	.driver		= {
		.name	= "mediatek-crtc-ext",
		.owner	= THIS_MODULE,
		.of_match_table = crtc_ext_driver_dt_match,
	},
};
