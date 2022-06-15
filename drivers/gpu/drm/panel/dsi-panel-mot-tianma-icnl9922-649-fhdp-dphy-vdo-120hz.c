// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/backlight.h>
#include <linux/delay.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_panel.h>
#include <drm/drm_modes.h>
#include <drm/drm_connector.h>
#include <drm/drm_device.h>

#include <video/mipi_display.h>
#include <video/of_videomode.h>
#include <video/videomode.h>

#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/of_graph.h>
#include <linux/platform_device.h>
#include <linux/gpio/consumer.h>

#define CONFIG_MTK_PANEL_EXT
#if defined(CONFIG_MTK_PANEL_EXT)
#include "../mediatek/mediatek_v2/mtk_panel_ext.h"
#include "../mediatek/mediatek_v2/mtk_drm_graphics_base.h"
#include "include/dsi-panel-mot-tianma-icnl9922-649-fhdp-dphy-vdo-120hz.h"
#endif

/* option function to read data from some panel address */
/* #define PANEL_SUPPORT_READBACK */

struct tianma {
	struct device *dev;
	struct drm_panel panel;
	struct backlight_device *backlight;
	struct gpio_desc *pm_enable_gpio;
	struct gpio_desc *reset_gpio;
	struct gpio_desc *bias_n_gpio;
	struct gpio_desc *bias_p_gpio;

	bool prepared;
	bool enabled;

	int error;
};

static char bl_tb0[] = { 0x51, 0xff };

#define tianma_dcs_write_seq(ctx, seq...)                                     \
	({                                                                     \
		const u8 d[] = {seq};                                          \
		BUILD_BUG_ON_MSG(ARRAY_SIZE(d) > 64,                           \
				 "DCS sequence too big for stack");            \
		tianma_dcs_write(ctx, d, ARRAY_SIZE(d));                      \
	})

#define tianma_dcs_write_seq_static(ctx, seq...)                              \
	({                                                                     \
		static const u8 d[] = {seq};                                   \
		tianma_dcs_write(ctx, d, ARRAY_SIZE(d));                      \
	})

static inline struct tianma *panel_to_tianma(struct drm_panel *panel)
{
	return container_of(panel, struct tianma, panel);
}

#ifdef PANEL_SUPPORT_READBACK
static int tianma_dcs_read(struct tianma *ctx, u8 cmd, void *data, size_t len)
{
	struct mipi_dsi_device *dsi = to_mipi_dsi_device(ctx->dev);
	ssize_t ret;

	if (ctx->error < 0)
		return 0;

	ret = mipi_dsi_dcs_read(dsi, cmd, data, len);
	if (ret < 0) {
		dev_err(ctx->dev, "error %d reading dcs seq:(%#x)\n", ret, cmd);
		ctx->error = ret;
	}

	return ret;
}

static void tianma_panel_get_data(struct tianma *ctx)
{
	u8 buffer[3] = {0};
	static int ret;

	if (ret == 0) {
		ret = tianma_dcs_read(ctx, 0x0A, buffer, 1);
		dev_info(ctx->dev, "return %d data(0x%08x) to dsi engine\n",
			 ret, buffer[0] | (buffer[1] << 8));
	}
}
#endif

static void tianma_dcs_write(struct tianma *ctx, const void *data, size_t len)
{
	struct mipi_dsi_device *dsi = to_mipi_dsi_device(ctx->dev);
	ssize_t ret;
	char *addr;

	if (ctx->error < 0)
		return;

	addr = (char *)data;
	if ((int)*addr < 0xB0)
		ret = mipi_dsi_dcs_write_buffer(dsi, data, len);
	else
		ret = mipi_dsi_generic_write(dsi, data, len);
	if (ret < 0) {
		dev_err(ctx->dev, "error %zd writing seq: %ph\n", ret, data);
		ctx->error = ret;
	}
}


static void tianma_panel_init(struct tianma *ctx)
{
	ctx->reset_gpio = devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio)) {
		dev_err(ctx->dev, "%s: cannot get reset_gpio %ld\n",
			__func__, PTR_ERR(ctx->reset_gpio));
		return;
	}
	
	ctx->bias_n_gpio = devm_gpiod_get(ctx->dev, "bias_n", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->bias_n_gpio)) {
		dev_err(ctx->dev, "%s: cannot get bias_n_gpio %ld\n",
			__func__, PTR_ERR(ctx->bias_n_gpio));
		return;
	}
	
	ctx->bias_p_gpio = devm_gpiod_get(ctx->dev, "bias_p", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->bias_p_gpio)) {
		dev_err(ctx->dev, "%s: cannot get bias_p_gpio %ld\n",
			__func__, PTR_ERR(ctx->bias_p_gpio));
		return;
	}
	gpiod_set_value(ctx->bias_n_gpio, 1);
	msleep(5);
	gpiod_set_value(ctx->bias_p_gpio, 1);
	msleep(5);

	gpiod_set_value(ctx->reset_gpio, 1);
	msleep(10);
	gpiod_set_value(ctx->reset_gpio, 0);
	msleep(5);
	gpiod_set_value(ctx->reset_gpio, 1);
	msleep(60);
	
	devm_gpiod_put(ctx->dev, ctx->reset_gpio);
	devm_gpiod_put(ctx->dev, ctx->bias_n_gpio);
	devm_gpiod_put(ctx->dev, ctx->bias_p_gpio);
	printk("[%d  %s]hxl_check_bias !!\n",__LINE__, __FUNCTION__);

	tianma_dcs_write_seq_static(ctx, 0xF0, 0x5A,0x59);
	tianma_dcs_write_seq_static(ctx, 0xF1, 0xA5,0xA6);
	//tianma_dcs_write_seq_static(ctx, 0xBD,0xA0);//not used
	tianma_dcs_write_seq_static(ctx, 0x70, 0x00);//dsc disble //0x41 enable
	//tianma_dcs_write_seq_static(ctx, 0xD0,0x4C,0x23,0x18,0xFF,0xFF,0x00,0x80,0x0C,0xFF,0x0F,0x42,0x00,0x08,0x10);
	tianma_dcs_write_seq_static(ctx, 0xD1,0x00,0x00,0x00,0x00,0x00,0x33,0x01,0x01);//mipi high speed   Ths-- settle time
	//tianma_dcs_write_seq_static(ctx, 0x51, 0x0F, 0xFF);
	tianma_dcs_write_seq_static(ctx, 0x51, 0x0F,0xFF);
	tianma_dcs_write_seq_static(ctx, 0x53, 0x2c);
	tianma_dcs_write_seq_static(ctx, 0x35, 0x00);
	tianma_dcs_write_seq_static(ctx, 0x11, 0x00);
	msleep(120);
	tianma_dcs_write_seq_static(ctx, 0x29, 0x00);
	msleep(20);

}

static int tianma_disable(struct drm_panel *panel)
{
	struct tianma *ctx = panel_to_tianma(panel);
	
	printk("[%d  %s]hxl_check_dsi_pcl_data_rate !!\n",__LINE__, __FUNCTION__);

	if (!ctx->enabled)
		return 0;

	if (ctx->backlight) {
		ctx->backlight->props.power = FB_BLANK_POWERDOWN;
		backlight_update_status(ctx->backlight);
	}

	ctx->enabled = false;

	return 0;
}

static int tianma_unprepare(struct drm_panel *panel)
{
	struct tianma *ctx = panel_to_tianma(panel);

	if (!ctx->prepared)
		return 0;
	pr_info("%s\n", __func__);
	printk("[%d  %s]hxl_check_dsi !!\n",__LINE__, __FUNCTION__);

	tianma_dcs_write_seq_static(ctx, 0x28);
	tianma_dcs_write_seq_static(ctx, 0x10);
	msleep(120);
	
	ctx->bias_n_gpio = devm_gpiod_get(ctx->dev, "bias_n", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->bias_n_gpio)) {
		dev_err(ctx->dev, "%s: cannot get bias_n_gpio %ld\n",
			__func__, PTR_ERR(ctx->bias_n_gpio));
		return -1;
	}
	
	ctx->bias_p_gpio = devm_gpiod_get(ctx->dev, "bias_p", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->bias_p_gpio)) {
		dev_err(ctx->dev, "%s: cannot get bias_p_gpio %ld\n",
			__func__, PTR_ERR(ctx->bias_p_gpio));
		return -1;
	}
	gpiod_set_value(ctx->bias_n_gpio, 0);
	udelay(10 * 1000);
	gpiod_set_value(ctx->bias_p_gpio, 0);
	udelay(10 * 1000);
	devm_gpiod_put(ctx->dev, ctx->bias_n_gpio);
	devm_gpiod_put(ctx->dev, ctx->bias_p_gpio);
	printk("[%d  %s]hxl_check_bias !!\n",__LINE__, __FUNCTION__);

	ctx->error = 0;
	ctx->prepared = false;
	return 0;
}

static int tianma_prepare(struct drm_panel *panel)
{
	struct tianma *ctx = panel_to_tianma(panel);
	int ret;

	pr_info("%s\n", __func__);
	printk("[%d  %s]hxl_check_dsi !!\n",__LINE__, __FUNCTION__);
	if (ctx->prepared)
		return 0;

	//lcm_power_enable();
	tianma_panel_init(ctx);

	ret = ctx->error;
	printk("[%d  %s]hxl_check_dsi1   ret:%d !!\n",__LINE__, __FUNCTION__,ret);
	if (ret < 0)
		tianma_unprepare(panel);

	ctx->prepared = true;
/*#if defined(CONFIG_MTK_PANEL_EXT)
	mtk_panel_tch_rst(panel);
#endif

#ifdef PANEL_SUPPORT_READBACK
	tianma_panel_get_data(ctx);
#endif*/
printk("[%d  %s]hxl_check_dsi  ret:%d !!\n",__LINE__, __FUNCTION__,ret);

	return ret;
}

static int tianma_enable(struct drm_panel *panel)
{
	struct tianma *ctx = panel_to_tianma(panel);
	
	printk("[%d  %s]hxl_check_dsi !!\n",__LINE__, __FUNCTION__);

	if (ctx->enabled)
		return 0;

	if (ctx->backlight) {
		ctx->backlight->props.power = FB_BLANK_UNBLANK;
		backlight_update_status(ctx->backlight);
	}

	ctx->enabled = true;

	return 0;
}

static const struct drm_display_mode default_mode = {
	.clock = 185697,
	.hdisplay = 1080,
	.hsync_start = 1080 + 76,
	.hsync_end = 1080 + 76 + 16,
	.htotal = 1080 + 76 + 16 + 80,
	.vdisplay = 2400,
	.vsync_start = 2400 + 38,
	.vsync_end = 2400 + 38 + 2,
	.vtotal = 2400 + 38 + 2 + 32,
};


#if defined(CONFIG_MTK_PANEL_EXT)
static struct mtk_panel_params ext_params = {
	//.pll_clk = 600,
	.data_rate = 1200,
	//.vfp_low_power = 840,
	.cust_esd_check = 0,
	.esd_check_enable = 0,
	.lcm_esd_check_table[0] = {
		.cmd = 0x0a,
		.count = 1,
		.para_list[0] = 0x9c,
	},
	.lane_swap_en = 0,
	.lp_perline_en = 0,
	.output_mode = MTK_PANEL_SINGLE_PORT,
	.physical_width_um = PHYSICAL_WIDTH,
	.physical_height_um = PHYSICAL_HEIGHT,
};

static int tianma_setbacklight_cmdq(void *dsi, dcs_write_gce cb,
	void *handle, unsigned int level)
{
	if (level > 255)
		level = 255;
	pr_info("%s backlight = -%d\n", __func__, level);
	bl_tb0[1] = (u8)level;

	if (!cb)
		return -1;

	cb(dsi, handle, bl_tb0, ARRAY_SIZE(bl_tb0));
	return 0;
}

static int panel_ext_reset(struct drm_panel *panel, int on)
{
	struct tianma *ctx = panel_to_tianma(panel);

	ctx->reset_gpio =
		devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
	gpiod_set_value(ctx->reset_gpio, on);
	devm_gpiod_put(ctx->dev, ctx->reset_gpio);

	return 0;
}

static enum mtk_lcm_version mtk_panel_get_lcm_version(void)
{
	return MTK_LEGACY_LCM_DRV_WITH_BACKLIGHTCLASS;
}

static struct mtk_panel_funcs ext_funcs = {
	.set_backlight_cmdq = tianma_setbacklight_cmdq,
	.reset = panel_ext_reset,
//	.ext_param_set = mtk_panel_ext_param_set,
//	.ata_check = panel_ata_check,
	.get_lcm_version = mtk_panel_get_lcm_version,
};
#endif

static int tianma_get_modes(struct drm_panel *panel,
						struct drm_connector *connector)
{
	struct drm_display_mode *mode;

	mode = drm_mode_duplicate(connector->dev, &default_mode);
	printk("[%d  %s]hxl_check_dsi_modes  mode:%d  !!\n",__LINE__, __FUNCTION__,mode);
	if (!mode) {
		dev_err(connector->dev->dev, "failed to add mode %ux%ux@%u\n",
			default_mode.hdisplay, default_mode.vdisplay,
			drm_mode_vrefresh(&default_mode));
		return -ENOMEM;
	}
	drm_mode_set_name(mode);
	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	drm_mode_probed_add(connector, mode);

	connector->display_info.width_mm = 68;
	connector->display_info.height_mm = 150;

	return 1;
}

static const struct drm_panel_funcs tianma_drm_funcs = {
	.disable = tianma_disable,
	.unprepare = tianma_unprepare,
	.prepare = tianma_prepare,
	.enable = tianma_enable,
	.get_modes = tianma_get_modes,
};

static int tianma_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct device_node *dsi_node, *remote_node = NULL, *endpoint = NULL;
	struct tianma *ctx;
	struct device_node *backlight;
	int ret;

	pr_info("%s+ tianma,ICNL9922,vdo,120hz\n", __func__);
	printk("[%s %d],hxl_check_lcd Enter!! \n", __func__, __LINE__);

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

	ctx = devm_kzalloc(dev, sizeof(struct tianma), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	mipi_dsi_set_drvdata(dsi, ctx);

	ctx->dev = dev;
	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST
			 | MIPI_DSI_MODE_LPM | MIPI_DSI_MODE_EOT_PACKET
			 | MIPI_DSI_CLOCK_NON_CONTINUOUS;

	backlight = of_parse_phandle(dev->of_node, "backlight", 0);
	if (backlight) {
		ctx->backlight = of_find_backlight_by_node(backlight);
		of_node_put(backlight);

		if (!ctx->backlight)
			return -EPROBE_DEFER;
	}

	ctx->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio)) {
		dev_err(dev, "cannot get reset-gpios %ld\n",
			PTR_ERR(ctx->reset_gpio));
		return PTR_ERR(ctx->reset_gpio);
	}
	devm_gpiod_put(dev, ctx->reset_gpio);

	ctx->prepared = true;
	ctx->enabled = true;

	drm_panel_init(&ctx->panel, dev, &tianma_drm_funcs, DRM_MODE_CONNECTOR_DSI);

	drm_panel_add(&ctx->panel);

	ret = mipi_dsi_attach(dsi);
	if (ret < 0)
		drm_panel_remove(&ctx->panel);

#if defined(CONFIG_MTK_PANEL_EXT)
	mtk_panel_tch_handle_reg(&ctx->panel);
	ret = mtk_panel_ext_create(dev, &ext_params, &ext_funcs, &ctx->panel);
	if (ret < 0)
		return ret;
#endif

	pr_info("[%d  %s]- tianma,ICNL9922,vdo,120hz  ret:%d  \n", __LINE__, __func__,ret);

	return ret;
}

static int tianma_remove(struct mipi_dsi_device *dsi)
{
	struct tianma *ctx = mipi_dsi_get_drvdata(dsi);
#if defined(CONFIG_MTK_PANEL_EXT)
	struct mtk_panel_ctx *ext_ctx = find_panel_ctx(&ctx->panel);
#endif

	mipi_dsi_detach(dsi);
	drm_panel_remove(&ctx->panel);
#if defined(CONFIG_MTK_PANEL_EXT)
	mtk_panel_detach(ext_ctx);
	mtk_panel_remove(ext_ctx);
#endif

	return 0;
}

static const struct of_device_id tianma_of_match[] = {
	{
		.compatible = "tianma,icnl9922,vdo,120hz",
	},
	{} };

MODULE_DEVICE_TABLE(of, tianma_of_match);

static struct mipi_dsi_driver tianma_driver = {
	.probe = tianma_probe,
	.remove = tianma_remove,
	.driver = {
		.name = "panel-tianma-icnl9922-vdo-120hz",
		.owner = THIS_MODULE,
		.of_match_table = tianma_of_match,
	},
};

module_mipi_dsi_driver(tianma_driver);

MODULE_AUTHOR("Jingjing Liu <jingjing.liu@mediatek.com>");
MODULE_DESCRIPTION("tianma icnl9922 incell 120hz Panel Driver");
MODULE_LICENSE("GPL v2");

