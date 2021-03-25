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

#include <linux/backlight.h>
#include <drm/drmP.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_panel.h>

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
#include "../mediatek/mtk_log.h"
#include "../mediatek/mtk_drm_graphics_base.h"
#endif

#ifdef CONFIG_MTK_ROUND_CORNER_SUPPORT
#include "../mediatek/mtk_corner_pattern/mtk_data_hw_roundedpattern.h"
#endif

struct lcm {
	struct device *dev;
	struct drm_panel panel;
	struct backlight_device *backlight;
	struct gpio_desc *reset_gpio;
	struct gpio_desc *vci_gpio;
	struct gpio_desc *dvdd_gpio;

	bool prepared;
	bool enabled;

	int error;
	bool hbm_en;
	unsigned int cabc_mode;
};

#define lcm_dcs_write_seq(ctx, seq...) \
({\
	const u8 d[] = { seq };\
	BUILD_BUG_ON_MSG(ARRAY_SIZE(d) > 64, "DCS sequence too big for stack");\
	lcm_dcs_write(ctx, d, ARRAY_SIZE(d));\
})

#define lcm_dcs_write_seq_static(ctx, seq...) \
({\
	static const u8 d[] = { seq };\
	lcm_dcs_write(ctx, d, ARRAY_SIZE(d));\
})

static inline struct lcm *panel_to_lcm(struct drm_panel *panel)
{
	return container_of(panel, struct lcm, panel);
}

static void lcm_dcs_write(struct lcm *ctx, const void *data, size_t len)
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
		dev_info(ctx->dev, "error %zd writing seq: %ph\n", ret, data);
		ctx->error = ret;
	}
}

#ifdef PANEL_SUPPORT_READBACK
static int lcm_dcs_read(struct lcm *ctx, u8 cmd, void *data, size_t len)
{
	struct mipi_dsi_device *dsi = to_mipi_dsi_device(ctx->dev);
	ssize_t ret;

	if (ctx->error < 0)
		return 0;

	ret = mipi_dsi_dcs_read(dsi, cmd, data, len);
	if (ret < 0) {
		dev_info(ctx->dev, "error %d reading dcs seq:(%#x)\n",
		ret, cmd);
		ctx->error = ret;
	}

	return ret;
}

static void lcm_panel_get_data(struct lcm *ctx)
{
	u8 buffer[3] = {0};
	static int ret;

	if (ret == 0) {
		ret = lcm_dcs_read(ctx,  0x0A, buffer, 1);
		dev_info(ctx->dev, "return %d data(0x%08x) to dsi engine\n",
			ret, buffer[0] | (buffer[1] << 8));
	}
}
#endif

#if defined(CONFIG_RT5081_PMU_DSV) || defined(CONFIG_MT6370_PMU_DSV)
static struct regulator *disp_bias_pos;
static struct regulator *disp_bias_neg;


static int lcm_panel_bias_regulator_init(void)
{
	static int regulator_inited;
	int ret = 0;

	if (regulator_inited)
		return ret;

	/* please only get regulator once in a driver */
	disp_bias_pos = regulator_get(NULL, "dsv_pos");
	if (IS_ERR(disp_bias_pos)) { /* handle return value */
		ret = PTR_ERR(disp_bias_pos);
		dev_info("get dsv_pos fail, error: %d\n", ret);
		return ret;
	}

	disp_bias_neg = regulator_get(NULL, "dsv_neg");
	if (IS_ERR(disp_bias_neg)) { /* handle return value */
		ret = PTR_ERR(disp_bias_neg);
		dev_info("get dsv_neg fail, error: %d\n", ret);
		return ret;
	}

	regulator_inited = 1;
	return ret; /* must be 0 */

}

static int lcm_panel_bias_enable(void)
{
	int ret = 0;
	int retval = 0;

	lcm_panel_bias_regulator_init();

	/* set voltage with min & max*/
	ret = regulator_set_voltage(disp_bias_pos, 5400000, 5400000);
	if (ret < 0)
		dev_info("set voltage disp_bias_pos fail, ret = %d\n", ret);
	retval |= ret;

	ret = regulator_set_voltage(disp_bias_neg, 5400000, 5400000);
	if (ret < 0)
		dev_info("set voltage disp_bias_neg fail, ret = %d\n", ret);
	retval |= ret;

	/* enable regulator */
	ret = regulator_enable(disp_bias_pos);
	if (ret < 0)
		dev_info("enable regulator disp_bias_pos fail, ret = %d\n",
			ret);
	retval |= ret;

	ret = regulator_enable(disp_bias_neg);
	if (ret < 0)
		dev_info("enable regulator disp_bias_neg fail, ret = %d\n",
			ret);
	retval |= ret;

	return retval;
}

static int lcm_panel_bias_disable(void)
{
	int ret = 0;
	int retval = 0;

	lcm_panel_bias_regulator_init();

	ret = regulator_disable(disp_bias_neg);
	if (ret < 0)
		dev_info("disable regulator disp_bias_neg fail, ret = %d\n",
			ret);
	retval |= ret;

	ret = regulator_disable(disp_bias_pos);
	if (ret < 0)
		dev_info("disable regulator disp_bias_pos fail, ret = %d\n",
			ret);
	retval |= ret;

	return retval;
}
#endif

static void lcm_panel_init(struct lcm *ctx)
{
	ctx->reset_gpio =
		devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio)) {
		dev_info(ctx->dev, "%s: cannot get reset-gpios %ld\n",
			__func__, PTR_ERR(ctx->reset_gpio));
		return;
	}
	gpiod_set_value(ctx->reset_gpio, 0);
	udelay(15 * 1000);
	gpiod_set_value(ctx->reset_gpio, 1);
	udelay(10 * 1000);
	gpiod_set_value(ctx->reset_gpio, 0);
	udelay(10 * 1000);
	gpiod_set_value(ctx->reset_gpio, 1);
	udelay(10 * 1000);
	devm_gpiod_put(ctx->dev, ctx->reset_gpio);

	lcm_dcs_write_seq_static(ctx, 0xFE, 0x16),
	lcm_dcs_write_seq_static(ctx, 0x88, 0x00),
	lcm_dcs_write_seq_static(ctx, 0xFE, 0x16),
	lcm_dcs_write_seq_static(ctx, 0x8D, 0x00),
	lcm_dcs_write_seq_static(ctx, 0x8C, 0x03),
	lcm_dcs_write_seq_static(ctx, 0x00, 0x00),
	lcm_dcs_write_seq_static(ctx, 0x66, 0x00),
	lcm_dcs_write_seq_static(ctx, 0x67, 0x00),
	lcm_dcs_write_seq_static(ctx, 0x68, 0x00),
	lcm_dcs_write_seq_static(ctx, 0x6A, 0x00),
	lcm_dcs_write_seq_static(ctx, 0x69, 0x70),
	lcm_dcs_write_seq_static(ctx, 0x6B, 0xF8),
	lcm_dcs_write_seq_static(ctx, 0x6C, 0x77),
	lcm_dcs_write_seq_static(ctx, 0x6D, 0xF8),
	lcm_dcs_write_seq_static(ctx, 0x6E, 0xF8),
	lcm_dcs_write_seq_static(ctx, 0x75, 0x00),
	lcm_dcs_write_seq_static(ctx, 0x76, 0x00),
	lcm_dcs_write_seq_static(ctx, 0x77, 0x00),
	lcm_dcs_write_seq_static(ctx, 0x78, 0x00),
	lcm_dcs_write_seq_static(ctx, 0x79, 0x00),
	lcm_dcs_write_seq_static(ctx, 0x7A, 0x00),
	lcm_dcs_write_seq_static(ctx, 0x7B, 0x00),
	lcm_dcs_write_seq_static(ctx, 0x7C, 0xCC),
	lcm_dcs_write_seq_static(ctx, 0x7D, 0x02),
	lcm_dcs_write_seq_static(ctx, 0x7E, 0x66),
	lcm_dcs_write_seq_static(ctx, 0x7F, 0x04),
	lcm_dcs_write_seq_static(ctx, 0x80, 0x00),
	lcm_dcs_write_seq_static(ctx, 0x81, 0x00),
	lcm_dcs_write_seq_static(ctx, 0x82, 0x00),
	lcm_dcs_write_seq_static(ctx, 0x83, 0x00),
	lcm_dcs_write_seq_static(ctx, 0x84, 0x00),
	lcm_dcs_write_seq_static(ctx, 0x85, 0x00),
	lcm_dcs_write_seq_static(ctx, 0x86, 0x00),

	lcm_dcs_write_seq_static(ctx, 0xFE, 0x60);
	lcm_dcs_write_seq_static(ctx, 0x1B, 0xCC);
	lcm_dcs_write_seq_static(ctx, 0x24, 0xCC);
	lcm_dcs_write_seq_static(ctx, 0xFE, 0xD0);
	lcm_dcs_write_seq_static(ctx, 0x42, 0x81);
	lcm_dcs_write_seq_static(ctx, 0x49, 0x01);
	lcm_dcs_write_seq_static(ctx, 0xFE, 0x84);
	lcm_dcs_write_seq_static(ctx, 0xE0, 0x00); //60Hz:40
	lcm_dcs_write_seq_static(ctx, 0xDA, 0x00); //60Hz:02
	lcm_dcs_write_seq_static(ctx, 0xFE, 0x22);//demura page
	lcm_dcs_write_seq_static(ctx, 0x77, 0x00);//demure off:02 on:00


	/* PPS */
        lcm_dcs_write_seq_static(ctx, 0xFE, 0xD2);
        lcm_dcs_write_seq_static(ctx, 0x00, 0x11);
        lcm_dcs_write_seq_static(ctx, 0x01, 0x89);
        lcm_dcs_write_seq_static(ctx, 0x02, 0x30);
        lcm_dcs_write_seq_static(ctx, 0x03, 0x09);
        lcm_dcs_write_seq_static(ctx, 0x04, 0x60);
        lcm_dcs_write_seq_static(ctx, 0x05, 0x04);
        lcm_dcs_write_seq_static(ctx, 0x06, 0x38);
        lcm_dcs_write_seq_static(ctx, 0x07, 0x00);
        lcm_dcs_write_seq_static(ctx, 0x08, 0x0c);
        lcm_dcs_write_seq_static(ctx, 0x09, 0x04);
        lcm_dcs_write_seq_static(ctx, 0x0a, 0x38);
        lcm_dcs_write_seq_static(ctx, 0x0b, 0x02);
        lcm_dcs_write_seq_static(ctx, 0x0c, 0x00);
        lcm_dcs_write_seq_static(ctx, 0x0d, 0x20);
        lcm_dcs_write_seq_static(ctx, 0x0e, 0x01);
        lcm_dcs_write_seq_static(ctx, 0x0f, 0x7e);
        lcm_dcs_write_seq_static(ctx, 0x10, 0x00);
        lcm_dcs_write_seq_static(ctx, 0x11, 0x0f);
        lcm_dcs_write_seq_static(ctx, 0x12, 0x0c);
        lcm_dcs_write_seq_static(ctx, 0x13, 0x08);
        lcm_dcs_write_seq_static(ctx, 0x14, 0xbb);
        lcm_dcs_write_seq_static(ctx, 0x15, 0x04);
        lcm_dcs_write_seq_static(ctx, 0x16, 0x3d);
        lcm_dcs_write_seq_static(ctx, 0x17, 0x18);
        lcm_dcs_write_seq_static(ctx, 0x18, 0x00);
        lcm_dcs_write_seq_static(ctx, 0x19, 0x10);
        lcm_dcs_write_seq_static(ctx, 0x1a, 0xf0);
        lcm_dcs_write_seq_static(ctx, 0x1b, 0x03);
        lcm_dcs_write_seq_static(ctx, 0x1c, 0x0c);
        lcm_dcs_write_seq_static(ctx, 0x1d, 0x20);
        lcm_dcs_write_seq_static(ctx, 0x1e, 0x00);
        lcm_dcs_write_seq_static(ctx, 0x1f, 0x06);
        lcm_dcs_write_seq_static(ctx, 0x20, 0x0b);
        lcm_dcs_write_seq_static(ctx, 0x21, 0x0b);
        lcm_dcs_write_seq_static(ctx, 0x22, 0x33);
        lcm_dcs_write_seq_static(ctx, 0x23, 0x0e);
        lcm_dcs_write_seq_static(ctx, 0x24, 0x1c);
        lcm_dcs_write_seq_static(ctx, 0x25, 0x2a);
        lcm_dcs_write_seq_static(ctx, 0x26, 0x38);
        lcm_dcs_write_seq_static(ctx, 0x27, 0x46);
        lcm_dcs_write_seq_static(ctx, 0x28, 0x54);
        lcm_dcs_write_seq_static(ctx, 0x29, 0x62);
        lcm_dcs_write_seq_static(ctx, 0x2a, 0x69);
        lcm_dcs_write_seq_static(ctx, 0x2b, 0x70);
        lcm_dcs_write_seq_static(ctx, 0x2d, 0x77);
        lcm_dcs_write_seq_static(ctx, 0x2f, 0x79);
        lcm_dcs_write_seq_static(ctx, 0x30, 0x7b);
        lcm_dcs_write_seq_static(ctx, 0x31, 0x7d);
        lcm_dcs_write_seq_static(ctx, 0x32, 0x7e);
        lcm_dcs_write_seq_static(ctx, 0x33, 0x01);
        lcm_dcs_write_seq_static(ctx, 0x34, 0x02);
        lcm_dcs_write_seq_static(ctx, 0x35, 0x01);
        lcm_dcs_write_seq_static(ctx, 0x36, 0x00);
        lcm_dcs_write_seq_static(ctx, 0x37, 0x09);
        lcm_dcs_write_seq_static(ctx, 0x38, 0x40);
        lcm_dcs_write_seq_static(ctx, 0x39, 0x09);
        lcm_dcs_write_seq_static(ctx, 0x3a, 0xbe);
        lcm_dcs_write_seq_static(ctx, 0x3b, 0x19);
        lcm_dcs_write_seq_static(ctx, 0x3d, 0xfc);
        lcm_dcs_write_seq_static(ctx, 0x3f, 0x19);
        lcm_dcs_write_seq_static(ctx, 0x40, 0xfa);
        lcm_dcs_write_seq_static(ctx, 0x41, 0x19);
        lcm_dcs_write_seq_static(ctx, 0x42, 0xf8);
        lcm_dcs_write_seq_static(ctx, 0x43, 0x1a);
        lcm_dcs_write_seq_static(ctx, 0x44, 0x38);
        lcm_dcs_write_seq_static(ctx, 0x45, 0x1a);
        lcm_dcs_write_seq_static(ctx, 0x46, 0x78);
        lcm_dcs_write_seq_static(ctx, 0x47, 0x1a);
        lcm_dcs_write_seq_static(ctx, 0x48, 0xb6);
        lcm_dcs_write_seq_static(ctx, 0x49, 0x2a);
        lcm_dcs_write_seq_static(ctx, 0x4a, 0xf6);
        lcm_dcs_write_seq_static(ctx, 0x4b, 0x2b);
        lcm_dcs_write_seq_static(ctx, 0x4c, 0x34);
        lcm_dcs_write_seq_static(ctx, 0x4d, 0x2b);
        lcm_dcs_write_seq_static(ctx, 0x4e, 0x74);
        lcm_dcs_write_seq_static(ctx, 0x4f, 0x3b);
        lcm_dcs_write_seq_static(ctx, 0x50, 0x74);
        lcm_dcs_write_seq_static(ctx, 0x51, 0x6b);
        lcm_dcs_write_seq_static(ctx, 0x52, 0xf4);
        lcm_dcs_write_seq_static(ctx, 0x86, 0x04);
        lcm_dcs_write_seq_static(ctx, 0x87, 0x38);
        lcm_dcs_write_seq_static(ctx, 0x88, 0x00);
        lcm_dcs_write_seq_static(ctx, 0x89, 0x00);
        lcm_dcs_write_seq_static(ctx, 0x8a, 0x00);
        lcm_dcs_write_seq_static(ctx, 0x8b, 0x00);
        lcm_dcs_write_seq_static(ctx, 0x8c, 0x00);
        lcm_dcs_write_seq_static(ctx, 0x8d, 0x00);
        lcm_dcs_write_seq_static(ctx, 0x8e, 0x80);



	lcm_dcs_write_seq_static(ctx, 0xFE, 0x40);
	lcm_dcs_write_seq_static(ctx, 0x82, 0x10);
	lcm_dcs_write_seq_static(ctx, 0xFE, 0xD2);
	lcm_dcs_write_seq_static(ctx, 0x53, 0x08);
	lcm_dcs_write_seq_static(ctx, 0x72, 0x07);
	lcm_dcs_write_seq_static(ctx, 0xFE, 0x00);
	lcm_dcs_write_seq_static(ctx, 0xFA, 0x01); // 07 VESAo ff, 01 VESA on
	lcm_dcs_write_seq_static(ctx, 0xC2, 0x08);//08 cmd, 03 video
	lcm_dcs_write_seq_static(ctx, 0x35, 0x00);
	lcm_dcs_write_seq_static(ctx,0x53,0x28);
	lcm_dcs_write_seq_static(ctx,0x55,0x01);
	lcm_dcs_write_seq_static(ctx, 0x51, 0x00, 0x00);
	lcm_dcs_write_seq_static(ctx, 0x11);
	msleep(120);
	lcm_dcs_write_seq_static(ctx, 0x29);
}

static int lcm_disable(struct drm_panel *panel)
{
	struct lcm *ctx = panel_to_lcm(panel);

	if (!ctx->enabled)
		return 0;

	if (ctx->backlight) {
		ctx->backlight->props.power = FB_BLANK_POWERDOWN;
		backlight_update_status(ctx->backlight);
	}

	ctx->enabled = false;

	return 0;
}

static int lcm_unprepare(struct drm_panel *panel)
{
	struct lcm *ctx = panel_to_lcm(panel);

	if (!ctx->prepared)
		return 0;

	lcm_dcs_write_seq_static(ctx, 0x28);
	lcm_dcs_write_seq_static(ctx, 0x10);
	msleep(120);

	ctx->error = 0;
	ctx->prepared = false;
#if defined(CONFIG_RT5081_PMU_DSV) || defined(CONFIG_MT6370_PMU_DSV)
	lcm_panel_bias_disable();
#endif
	ctx->reset_gpio = devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio)) {
		dev_info(ctx->dev, "%s: cannot get reset-gpios %ld\n",
			__func__, PTR_ERR(ctx->reset_gpio));
		return PTR_ERR(ctx->reset_gpio);
	}
	gpiod_set_value(ctx->reset_gpio, 0);
	devm_gpiod_put(ctx->dev, ctx->reset_gpio);

	ctx->vci_gpio = devm_gpiod_get(ctx->dev, "vci", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->vci_gpio)) {
		dev_info(ctx->dev, "%s: cannot get vci-gpios %ld\n",
			__func__, PTR_ERR(ctx->vci_gpio));
		return PTR_ERR(ctx->vci_gpio);;
	}
	gpiod_set_value(ctx->vci_gpio, 0);
	devm_gpiod_put(ctx->dev, ctx->vci_gpio);

	ctx->dvdd_gpio = devm_gpiod_get(ctx->dev, "dvdd", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->dvdd_gpio)) {
		dev_info(ctx->dev, "%s: cannot get dvdd-gpios %ld\n",
			__func__, PTR_ERR(ctx->dvdd_gpio));
		return PTR_ERR(ctx->dvdd_gpio);;
	}
	gpiod_set_value(ctx->dvdd_gpio, 0);
	devm_gpiod_put(ctx->dev, ctx->dvdd_gpio);

	return 0;
}

static int lcm_prepare(struct drm_panel *panel)
{
	struct lcm *ctx = panel_to_lcm(panel);
	int ret;

	pr_info("%s\n", __func__);
	if (ctx->prepared)
		return 0;

#if defined(CONFIG_RT5081_PMU_DSV) || defined(CONFIG_MT6370_PMU_DSV)
	lcm_panel_bias_enable();
#endif
	ctx->vci_gpio = devm_gpiod_get(ctx->dev, "vci", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->vci_gpio)) {
		dev_info(ctx->dev, "%s: cannot get vci-gpios %ld\n",
			__func__, PTR_ERR(ctx->vci_gpio));
		return PTR_ERR(ctx->vci_gpio);;
	}
	gpiod_set_value(ctx->vci_gpio, 1);
	devm_gpiod_put(ctx->dev, ctx->vci_gpio);

	ctx->dvdd_gpio = devm_gpiod_get(ctx->dev, "dvdd", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->dvdd_gpio)) {
		dev_info(ctx->dev, "%s: cannot get dvdd-gpios %ld\n",
			__func__, PTR_ERR(ctx->dvdd_gpio));
		return PTR_ERR(ctx->dvdd_gpio);;
	}
	gpiod_set_value(ctx->dvdd_gpio, 1);
	devm_gpiod_put(ctx->dev, ctx->dvdd_gpio);

	lcm_panel_init(ctx);

	ret = ctx->error;
	if (ret < 0)
		lcm_unprepare(panel);

	ctx->prepared = true;

	ctx->cabc_mode = 0; /*UI mode*/
	ctx->hbm_en = 0;

#if defined(CONFIG_MTK_PANEL_EXT)
	mtk_panel_tch_rst(panel);
#endif
#ifdef PANEL_SUPPORT_READBACK
	lcm_panel_get_data(ctx);
#endif

	return ret;
}

static int lcm_enable(struct drm_panel *panel)
{
	struct lcm *ctx = panel_to_lcm(panel);

	if (ctx->enabled)
		return 0;

	if (ctx->backlight) {
		ctx->backlight->props.power = FB_BLANK_UNBLANK;
		backlight_update_status(ctx->backlight);
	}

	ctx->enabled = true;

	return 0;
}

#define HFP (26)
#define HSA (2)
#define HBP (36)
#define VFP_45HZ (12)
#define VFP_60HZ (12)
#define VFP_90HZ (12)
#define VSA (8)
#define VBP (12)
#define VAC (2400)
#define HAC (1080)

static u32 fake_heigh = 2400;
static u32 fake_width = 1080;
static bool need_fake_resolution;

static struct drm_display_mode default_mode = {
	.clock = 166932,//htotal*vtotal*vrefresh/1000
	.hdisplay = HAC,
	.hsync_start = HAC + HFP,
	.hsync_end = HAC + HFP + HSA,
	.htotal = HAC + HFP + HSA + HBP,
	.vdisplay = VAC,
	.vsync_start = VAC + VFP_60HZ,
	.vsync_end = VAC + VFP_60HZ + VSA,
	.vtotal = VAC + VFP_60HZ + VSA + VBP,
	.vrefresh = 60,
};

static struct drm_display_mode performance_mode = {
	.clock = 250398,//htotal*vtotal*vrefresh/1000
	.hdisplay = HAC,
	.hsync_start = HAC + HFP,
	.hsync_end = HAC + HFP + HSA,
	.htotal = HAC + HFP + HSA + HBP,
	.vdisplay = VAC,
	.vsync_start = VAC + VFP_90HZ,
	.vsync_end = VAC + VFP_90HZ + VSA,
	.vtotal = VAC + VFP_90HZ + VSA + VBP,
	.vrefresh = 90,
};

#if defined(CONFIG_MTK_PANEL_EXT)
static struct mtk_panel_params ext_params = {
	.pll_clk = 280,
	.vfp_low_power = VFP_45HZ,
	.cust_esd_check = 1,
	.esd_check_enable = 1,
	.lcm_esd_check_table[0] = {
		.cmd = 0x0A, .count = 1, .para_list[0] = 0x9C,
	},
	.output_mode = MTK_PANEL_DSC_SINGLE_PORT,
	.dsc_params = {
                .enable = 1,
                .ver = 17,
                .slice_mode = 0,
                .rgb_swap = 0,
                .dsc_cfg = 34,
                .rct_on = 1,
                .bit_per_channel = 8,
                .dsc_line_buf_depth = 9,
                .bp_enable = 1,
                .bit_per_pixel = 128,
                .pic_height = 2400,
                .pic_width = 1080,
                .slice_height = 12,
                .slice_width = 1080,
                .chunk_size = 1080,
                .xmit_delay = 512,
                .dec_delay = 796,
                .scale_value = 32,
                .increment_interval = 382,
                .decrement_interval = 15,
                .line_bpg_offset = 12,
                .nfl_bpg_offset = 2235,
                .slice_bpg_offset = 1085,
                .initial_offset = 6144,
                .final_offset = 4336,
                .flatness_minqp = 3,
                .flatness_maxqp = 12,
                .rc_model_size = 8192,
                .rc_edge_factor = 6,
                .rc_quant_incr_limit0 = 11,
                .rc_quant_incr_limit1 = 11,
                .rc_tgt_offset_hi = 3,
                .rc_tgt_offset_lo = 3,
		},
	.dyn_fps = {
		.switch_en = 1, .vact_timing_fps = 90,
	},
	.panel_hbm_mode = HBM_MODE_DCS_ONLY,
};

static struct mtk_panel_params ext_params_90hz = {
	.pll_clk = 280,
	.vfp_low_power = VFP_60HZ,
	.cust_esd_check = 1,
	.esd_check_enable = 1,
	.lcm_esd_check_table[0] = {

		.cmd = 0x0A, .count = 1, .para_list[0] = 0x9C,
	},
	.output_mode = MTK_PANEL_DSC_SINGLE_PORT,
	.dsc_params = {
                .enable = 1,
                .ver = 17,
                .slice_mode = 0,
                .rgb_swap = 0,
                .dsc_cfg = 34,
                .rct_on = 1,
                .bit_per_channel = 8,
                .dsc_line_buf_depth = 9,
                .bp_enable = 1,
                .bit_per_pixel = 128,
                .pic_height = 2400,
                .pic_width = 1080,
                .slice_height = 12,
                .slice_width = 1080,
                .chunk_size = 1080,
                .xmit_delay = 512,
                .dec_delay = 796,
                .scale_value = 32,
                .increment_interval = 382,
                .decrement_interval = 15,
                .line_bpg_offset = 12,
                .nfl_bpg_offset = 2235,
                .slice_bpg_offset = 1085,
                .initial_offset = 6144,
                .final_offset = 4336,
                .flatness_minqp = 3,
                .flatness_maxqp = 12,
                .rc_model_size = 8192,
                .rc_edge_factor = 6,
                .rc_quant_incr_limit0 = 11,
                .rc_quant_incr_limit1 = 11,
                .rc_tgt_offset_hi = 3,
                .rc_tgt_offset_lo = 3,
		},
	.dyn_fps = {
		.switch_en = 1, .vact_timing_fps = 90,
	},
	.panel_hbm_mode = HBM_MODE_DCS_ONLY,
};

static int panel_ext_reset(struct drm_panel *panel, int on)
{
	struct lcm *ctx = panel_to_lcm(panel);

	ctx->reset_gpio =
		devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio)) {
		dev_info(ctx->dev, "%s: cannot get reset-gpios %ld\n",
			__func__, PTR_ERR(ctx->reset_gpio));
		return PTR_ERR(ctx->reset_gpio);
	}
	gpiod_set_value(ctx->reset_gpio, on);
	devm_gpiod_put(ctx->dev, ctx->reset_gpio);

	return 0;
}

static int panel_ata_check(struct drm_panel *panel)
{
#if 0
	struct lcm *ctx = panel_to_lcm(panel);
	struct mipi_dsi_device *dsi = to_mipi_dsi_device(ctx->dev);
	unsigned char data[3];
	unsigned char id[3] = {0x00, 0x00, 0x00};
	ssize_t ret;

	pr_info("%s success\n", __func__);

	ret = mipi_dsi_dcs_read(dsi, 0x4, data, 3);
	if (ret < 0)
		dev_info("%s error\n", __func__);

	DDPINFO("ATA read data %x %x %x\n", data[0], data[1], data[2]);

	if (data[0] == id[0] &&
			data[1] == id[1] &&
			data[2] == id[2])
		return 1;

	DDPINFO("ATA expect read data is %x %x %x\n",
			id[0], id[1], id[2]);
#endif
	return 1;
}

static int lcm_setbacklight_cmdq(void *dsi, dcs_write_gce cb,
	void *handle, unsigned int level)
{
	char bl_tb0[] = {0x51, 0x07, 0xFF};

	pr_info("%s backlight = %d\n", __func__, level);

	level = level * 2047  / 255;
	bl_tb0[1] = ((level >> 8) & 0x7);
	bl_tb0[2] = (level & 0xFF);

	if (!cb)
		return -1;

	cb(dsi, handle, bl_tb0, ARRAY_SIZE(bl_tb0));

	return 0;
}

static struct drm_display_mode *get_mode_by_id(struct drm_panel *panel,
	unsigned int mode)
{
	struct drm_display_mode *m;
	unsigned int i = 0;

	list_for_each_entry(m, &panel->connector->modes, head) {
		if (i == mode)
			return m;
		i++;
	}
	return NULL;
}
static int mtk_panel_ext_param_set(struct drm_panel *panel, unsigned int mode)
{
	struct mtk_panel_ext *ext = find_panel_ext(panel);
	int ret = 0;
	struct drm_display_mode *m = get_mode_by_id(panel, mode);

	if (m->vrefresh == 60)
		ext->params = &ext_params;
	else if (m->vrefresh == 90)
		ext->params = &ext_params_90hz;
	else
		ret = 1;

	return ret;
}


static void mode_switch_60_to_90(struct drm_panel *panel,
        enum MTK_PANEL_MODE_SWITCH_STAGE stage)
{
	struct lcm *ctx = panel_to_lcm(panel);

        if (stage == BEFORE_DSI_POWERDOWN) {
	} else if (stage == AFTER_DSI_POWERON) {
		/* 60 to 90 */
		lcm_dcs_write_seq_static(ctx, 0xFE, 0x84);
		lcm_dcs_write_seq_static(ctx, 0xE0, 0x00);
		lcm_dcs_write_seq_static(ctx, 0xDA, 0x00);
		lcm_dcs_write_seq_static(ctx, 0xFE, 0x00);
	}
}


static void mode_switch_90_to_60(struct drm_panel *panel,
        enum MTK_PANEL_MODE_SWITCH_STAGE stage)
{
	struct lcm *ctx = panel_to_lcm(panel);

        if (stage == BEFORE_DSI_POWERDOWN) {
	} else if (stage == AFTER_DSI_POWERON) {
                /* switch to 60hz */
		lcm_dcs_write_seq_static(ctx, 0xFE, 0x84);
		lcm_dcs_write_seq_static(ctx, 0xE0, 0x40);
		lcm_dcs_write_seq_static(ctx, 0xDA, 0x02);
		lcm_dcs_write_seq_static(ctx, 0xFE, 0x00);
        }
}




static int lcm_get_virtual_heigh(void)
{
	return VAC;
}

static int lcm_get_virtual_width(void)
{
	return HAC;
}

static int mode_switch(struct drm_panel *panel, unsigned int cur_mode,
                unsigned int dst_mode, enum MTK_PANEL_MODE_SWITCH_STAGE stage)
{
        int ret = 0;

        if (cur_mode == 0 && dst_mode == 1) { /* 60 switch to 90 */
                mode_switch_60_to_90(panel, stage);
        } else if (cur_mode == 1 && dst_mode == 0) { /* 90 switch to 60 */
                mode_switch_90_to_60(panel, stage);
        } else
                ret = 1;

        return ret;
}

static int panel_cabc_set_cmdq(struct drm_panel *panel, void *dsi,
			      dcs_write_gce cb, void *handle, unsigned int cabc_mode)
{
	char cabc_tb[2] = {0x55, 0x01};
	const unsigned int cabc_value_map[3] = {1, 3, 0};
	struct lcm *ctx = panel_to_lcm(panel);

	if (ctx->cabc_mode == cabc_mode)
		goto done;

	cabc_tb[1] = cabc_value_map[cabc_mode];

	if (!cb)
		return -1;

	if (cabc_mode > 2)
		return -1;

	cb(dsi, handle, cabc_tb, ARRAY_SIZE(cabc_tb));

	ctx->cabc_mode = cabc_mode;
	pr_info("%s set cabc to %d\n", __func__, cabc_mode);
done:
	return 0;
}

static void panel_cabc_get_state(struct drm_panel *panel, unsigned int *cabc_mode)
{
	struct lcm *ctx = panel_to_lcm(panel);

	*cabc_mode = ctx->cabc_mode;
}

static int panel_hbm_set(struct drm_panel *panel, void *dsi,
			      dcs_write_gce cb, void *handle, bool hbm_en)
{
	char hbm_tb[3] = {0x51, 0x07, 0xFF};
	const unsigned int hbm_value_map[2] = {0x07, 0x0F};
	struct lcm *ctx = panel_to_lcm(panel);

	if (ctx->hbm_en == hbm_en)
		goto done;

	hbm_tb[1] = hbm_value_map[hbm_en];

	if (!cb)
		return -1;

	cb(dsi, handle, hbm_tb, ARRAY_SIZE(hbm_tb));

	ctx->hbm_en = hbm_en;
	pr_info("%s set HBM to %d\n", __func__, hbm_en);
done:
	return 0;
}

static void panel_hbm_get_state(struct drm_panel *panel, bool *state)
{
	struct lcm *ctx = panel_to_lcm(panel);

	*state = ctx->hbm_en;
}

static struct mtk_panel_funcs ext_funcs = {
	.reset = panel_ext_reset,
	.set_backlight_cmdq = lcm_setbacklight_cmdq,
	.ata_check = panel_ata_check,
	.ext_param_set = mtk_panel_ext_param_set,
	.mode_switch = mode_switch,
	.get_virtual_heigh = lcm_get_virtual_heigh,
	.get_virtual_width = lcm_get_virtual_width,
	.hbm_set_cmdq = panel_hbm_set,
	.hbm_get_state = panel_hbm_get_state,
	.cabc_set_cmdq = panel_cabc_set_cmdq,
	.cabc_get_state = panel_cabc_get_state,
};
#endif

struct panel_desc {
	const struct drm_display_mode *modes;
	unsigned int num_modes;

	unsigned int bpc;

	struct {
		unsigned int width;
		unsigned int height;
	} size;

	struct {
		unsigned int prepare;
		unsigned int enable;
		unsigned int disable;
		unsigned int unprepare;
	} delay;
};

static void change_drm_disp_mode_params(struct drm_display_mode *mode)
{
	if (fake_heigh > 0 && fake_heigh < VAC) {
		mode->vsync_start = mode->vsync_start - mode->vdisplay
					+ fake_heigh;
		mode->vsync_end = mode->vsync_end - mode->vdisplay + fake_heigh;
		mode->vtotal = mode->vtotal - mode->vdisplay + fake_heigh;
		mode->vdisplay = fake_heigh;
	}
	if (fake_width > 0 && fake_width < HAC) {
		mode->hsync_start = mode->hsync_start - mode->hdisplay
					+ fake_width;
		mode->hsync_end = mode->hsync_end - mode->hdisplay + fake_width;
		mode->htotal = mode->htotal - mode->hdisplay + fake_width;
		mode->hdisplay = fake_width;
	}
}

static int lcm_get_modes(struct drm_panel *panel)
{
	struct drm_display_mode *mode;
	struct drm_display_mode *mode2;

	if (need_fake_resolution) {
		change_drm_disp_mode_params(&default_mode);
		change_drm_disp_mode_params(&performance_mode);
	}
	mode = drm_mode_duplicate(panel->drm, &default_mode);
	if (!mode) {
		dev_info(panel->drm->dev, "failed to add mode %ux%ux@%u\n",
			default_mode.hdisplay, default_mode.vdisplay,
			default_mode.vrefresh);
		return -ENOMEM;
	}

	drm_mode_set_name(mode);
	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	drm_mode_probed_add(panel->connector, mode);

	mode2 = drm_mode_duplicate(panel->drm, &performance_mode);
	if (!mode2) {
		dev_info(panel->drm->dev, "failed to add mode %ux%ux@%u\n",
			performance_mode.hdisplay,
			performance_mode.vdisplay,
			performance_mode.vrefresh);
		return -ENOMEM;
	}

	drm_mode_set_name(mode2);
	mode2->type = DRM_MODE_TYPE_DRIVER;
	drm_mode_probed_add(panel->connector, mode2);

	panel->connector->display_info.width_mm = 69;
	panel->connector->display_info.height_mm = 154;

	panel->connector->display_info.panel_ver = 0x01;
	panel->connector->display_info.panel_id = 0x12082A93;
	strcpy(panel->connector->display_info.panel_name, "mipi_mot_cmd_visionox_fhdp_667");
	strcpy(panel->connector->display_info.panel_supplier, "visionox");

	return 1;
}

static const struct drm_panel_funcs lcm_drm_funcs = {
	.disable = lcm_disable,
	.unprepare = lcm_unprepare,
	.prepare = lcm_prepare,
	.enable = lcm_enable,
	.get_modes = lcm_get_modes,
};

static void check_is_need_fake_resolution(struct device *dev)
{
	unsigned int ret = 0;

	ret = of_property_read_u32(dev->of_node, "fake_heigh", &fake_heigh);
	if (ret)
		need_fake_resolution = false;
	ret = of_property_read_u32(dev->of_node, "fake_width", &fake_width);
	if (ret)
		need_fake_resolution = false;
	if (fake_heigh > 0 && fake_heigh < VAC)
		need_fake_resolution = true;
	if (fake_width > 0 && fake_width < HAC)
		need_fake_resolution = true;
}

static int lcm_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct lcm *ctx;
	struct device_node *backlight;
	int ret;
	struct device_node *dsi_node, *remote_node = NULL, *endpoint = NULL;

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

	ctx = devm_kzalloc(dev, sizeof(struct lcm), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	mipi_dsi_set_drvdata(dsi, ctx);

	ctx->dev = dev;
	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags = MIPI_DSI_MODE_LPM
					| MIPI_DSI_MODE_EOT_PACKET
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
		dev_info(dev, "cannot get reset-gpios %ld\n",
			PTR_ERR(ctx->reset_gpio));
		return PTR_ERR(ctx->reset_gpio);
	}
	devm_gpiod_put(dev, ctx->reset_gpio);

	ctx->vci_gpio = devm_gpiod_get(dev, "vci", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->vci_gpio)) {
		dev_info(dev, "cannot get vci-gpios %ld\n",
			PTR_ERR(ctx->vci_gpio));
		return PTR_ERR(ctx->vci_gpio);
	}
	devm_gpiod_put(dev, ctx->vci_gpio);

	ctx->dvdd_gpio = devm_gpiod_get(dev, "dvdd", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->dvdd_gpio)) {
		dev_info(dev, "cannot get dvdd-gpios %ld\n",
			PTR_ERR(ctx->dvdd_gpio));
		return PTR_ERR(ctx->dvdd_gpio);
	}
	devm_gpiod_put(dev, ctx->dvdd_gpio);

	ctx->prepared = true;
	ctx->enabled = true;

	drm_panel_init(&ctx->panel);
	ctx->panel.dev = dev;
	ctx->panel.funcs = &lcm_drm_funcs;

	ret = drm_panel_add(&ctx->panel);
	if (ret < 0)
		return ret;

	ret = mipi_dsi_attach(dsi);
	if (ret < 0)
		drm_panel_remove(&ctx->panel);

#if defined(CONFIG_MTK_PANEL_EXT)
	mtk_panel_tch_handle_reg(&ctx->panel);
	ret = mtk_panel_ext_create(dev, &ext_params, &ext_funcs, &ctx->panel);
	if (ret < 0)
		return ret;
#endif
	check_is_need_fake_resolution(dev);
	pr_info("%s-\n", __func__);

	return ret;
}

static int lcm_remove(struct mipi_dsi_device *dsi)
{
	struct lcm *ctx = mipi_dsi_get_drvdata(dsi);

	mipi_dsi_detach(dsi);
	drm_panel_remove(&ctx->panel);

	return 0;
}

static const struct of_device_id lcm_of_match[] = {
	{ .compatible = "rm692dx,visionox,vdo", },
	{ }
};

MODULE_DEVICE_TABLE(of, lcm_of_match);

static struct mipi_dsi_driver lcm_driver = {
	.probe = lcm_probe,
	.remove = lcm_remove,
	.driver = {
		.name = "panel-visionox-rm692dx-vdo",
		.owner = THIS_MODULE,
		.of_match_table = lcm_of_match,
	},
};

module_mipi_dsi_driver(lcm_driver);

MODULE_AUTHOR("MEDIATEK");
MODULE_DESCRIPTION("Visionox RM692DX VDO LCD Panel Driver");
MODULE_LICENSE("GPL v2");
