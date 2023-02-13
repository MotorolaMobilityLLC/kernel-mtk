// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/backlight.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_panel.h>
#include <drm/drm_modes.h>
#include <linux/delay.h>
#include <drm/drm_connector.h>
#include <drm/drm_device.h>

#include <linux/gpio/consumer.h>
#include <linux/regulator/consumer.h>

#include <video/mipi_display.h>
#include <video/of_videomode.h>
#include <video/videomode.h>

#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/of_graph.h>
#include <linux/platform_device.h>

#define CONFIG_MTK_PANEL_EXT
#if defined(CONFIG_MTK_PANEL_EXT)
#include "../mediatek/mtk_panel_ext.h"
#include "../mediatek/mtk_drm_graphics_base.h"
#endif

struct panel_ctx {
	struct device *dev;
	struct drm_panel panel;
	struct backlight_device *backlight;
	bool prepared;
	bool enabled;

	int error;
};

static int dummy_panel_disable(struct drm_panel *panel)
{
	return 0;
}

static int dummy_panel_unprepare(struct drm_panel *panel)
{
	return 0;
}

static int dummy_panel_prepare(struct drm_panel *panel)
{
	return 0;
}

static int dummy_panel_enable(struct drm_panel *panel)
{
	return 0;
}

static const struct drm_display_mode default_mode = {
	.clock = 182284,
	.hdisplay = 1080,
	.hsync_start = 1080 + 76,//HFP
	.hsync_end = 1080 + 76 + 12,//HSA
	.htotal = 1080 + 76 + 12 + 60,//HBP
	.vdisplay = 2400,
	.vsync_start = 2400 + 54,//VFP
	.vsync_end = 2400 + 54 + 10,//VSA
	.vtotal = 2400 + 54 + 10 + 10,//VBP
};

#if defined(CONFIG_MTK_PANEL_EXT)
static struct mtk_panel_params ext_params = {
	.pll_clk = 595,
	.vfp_low_power = 840,
	.data_rate = 1190,
};

static int panel_ata_check(struct drm_panel *panel)
{
	/* Customer test by own ATA tool */
	return 1;
}

static int dummy_panel_setbacklight_cmdq(void *dsi, dcs_write_gce cb, void *handle,
				 unsigned int level)
{
	return 0;
}

static int panel_ext_reset(struct drm_panel *panel, int on)
{
	return 0;
}

static struct mtk_panel_funcs ext_funcs = {
	.reset = panel_ext_reset,
	.set_backlight_cmdq = dummy_panel_setbacklight_cmdq,
	.ata_check = panel_ata_check,
};
#endif

static int dummy_panel_get_modes(struct drm_panel *panel)
{
	struct drm_display_mode *mode;

	mode = drm_mode_duplicate(panel->drm, &default_mode);
	if (!mode) {
		dev_info(panel->drm->dev, "failed to add mode %ux%ux@%u\n",
			 default_mode.hdisplay, default_mode.vdisplay,
			 drm_mode_vrefresh(&default_mode));
		return -ENOMEM;
	}

	drm_mode_set_name(mode);
	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	drm_mode_probed_add(panel->connector, mode);

	panel->connector->display_info.width_mm = 70;
	panel->connector->display_info.height_mm = 152;
	strcpy(panel->connector->display_info.panel_name, "mipi_mot_vdo_dummy");
	strcpy(panel->connector->display_info.panel_supplier, "dummy");

	return 1;
}

static const struct drm_panel_funcs dummy_panel_drm_funcs = {
	.disable = dummy_panel_disable,
	.unprepare = dummy_panel_unprepare,
	.prepare = dummy_panel_prepare,
	.enable = dummy_panel_enable,
	.get_modes = dummy_panel_get_modes,
};

static int dummy_panel_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct device_node *dsi_node, *remote_node = NULL, *endpoint = NULL;
	struct panel_ctx *ctx;
	struct device_node *backlight;
	int ret;

	pr_info("%s+\n", __func__);

	dsi_node = of_get_parent(dev->of_node);
	if (dsi_node) {
		endpoint = of_graph_get_next_endpoint(dsi_node, NULL);
		if (endpoint) {
			remote_node = of_graph_get_remote_port_parent(endpoint);
			if (!remote_node) {
				pr_info("No panel connected,skip probe lcm\n");
				return -ENODEV;
			}
			pr_info("device node name:%s\n", remote_node->name);
		}
	}
	if (remote_node != dev->of_node) {
		pr_info("%s+ skip probe due to not current lcm\n", __func__);
		return -ENODEV;
	}

	ctx = devm_kzalloc(dev, sizeof(struct panel_ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	mipi_dsi_set_drvdata(dsi, ctx);

	ctx->dev = dev;
	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_SYNC_PULSE |
			MIPI_DSI_MODE_LPM | MIPI_DSI_MODE_EOT_PACKET |
			MIPI_DSI_CLOCK_NON_CONTINUOUS;

	backlight = of_parse_phandle(dev->of_node, "backlight", 0);
	if (backlight) {
		ctx->backlight = of_find_backlight_by_node(backlight);
		of_node_put(backlight);

		if (!ctx->backlight)
			return -EPROBE_DEFER;
	}

	ctx->prepared = true;
	ctx->enabled = true;
	drm_panel_init(&ctx->panel);
	ctx->panel.dev = dev;
	ctx->panel.funcs = &dummy_panel_drm_funcs;
	drm_panel_add(&ctx->panel);

	ret = mipi_dsi_attach(dsi);
	if (ret < 0)
		drm_panel_remove(&ctx->panel);

#if defined(CONFIG_MTK_PANEL_EXT)
	ret = mtk_panel_ext_create(dev, &ext_params, &ext_funcs, &ctx->panel);
	if (ret < 0)
		return ret;

#endif

	pr_info("%s- moto,dummy,vdo,60hz\n", __func__);

	return ret;
}

static int dummy_panel_remove(struct mipi_dsi_device *dsi)
{
	struct panel_ctx *ctx = mipi_dsi_get_drvdata(dsi);

	mipi_dsi_detach(dsi);
	drm_panel_remove(&ctx->panel);

	return 0;
}

static const struct of_device_id dummy_panel_of_match[] = {
	{
	    .compatible = "mot,dummy,vdo,60hz",
	},
	{}
};

MODULE_DEVICE_TABLE(of, dummy_panel_of_match);

static struct mipi_dsi_driver dummy_panel_driver = {
	.probe = dummy_panel_probe,
	.remove = dummy_panel_remove,
	.driver = {
		.name = "panel_mot_vdo_dummy",
		.owner = THIS_MODULE,
		.of_match_table = dummy_panel_of_match,
	},
};

module_mipi_dsi_driver(dummy_panel_driver);

MODULE_AUTHOR("MEDIATEK");
MODULE_DESCRIPTION("tianma nt37701 AMOLED CMD LCD Panel Driver");
MODULE_LICENSE("GPL v2");
