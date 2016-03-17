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

#ifndef _MEDIATEK_DRM_DSI_H_
#define _MEDIATEK_DRM_DSI_H_

struct mtk_dsi {
	struct drm_device *drm_dev;
	struct drm_encoder encoder;
	struct drm_connector conn;
	struct device_node *panel_node;
	struct drm_panel *panel;
	struct drm_bridge *bridge;
	struct mipi_dsi_host host;
	struct regulator *disp_supplies;

	void __iomem *dsi_reg_base;
	void __iomem *dsi_tx_reg_base;

	struct clk *dsi_disp_clk_cg;
	struct clk *dsi_dsi_clk_cg;
	struct clk *dsi_div2_clk_cg;

	struct clk *dsi0_engine_clk_cg;
	struct clk *dsi0_digital_clk_cg;

	u32 data_rate;

	unsigned long mode_flags;
	enum mipi_dsi_pixel_format format;
	unsigned int lanes;
	struct videomode vm;
	bool enabled;
	int irq;
};

static inline struct mtk_dsi *host_to_dsi(struct mipi_dsi_host *h)
{
	return container_of(h, struct mtk_dsi, host);
}

static inline struct mtk_dsi *encoder_to_dsi(struct drm_encoder *e)
{
	return container_of(e, struct mtk_dsi, encoder);
}

#define connector_to_dsi(c) container_of(c, struct mtk_dsi, conn)

#endif

