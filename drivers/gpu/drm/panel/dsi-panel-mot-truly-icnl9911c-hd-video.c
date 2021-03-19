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

#include <drm/drmP.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_panel.h>
#include <linux/backlight.h>

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
#include "../mediatek/mtk_drm_graphics_base.h"
#include "../mediatek/mtk_log.h"
#include "../mediatek/mtk_panel_ext.h"
#endif

#ifdef CONFIG_MTK_ROUND_CORNER_SUPPORT
#include "../mediatek/mtk_corner_pattern/mtk_data_hw_roundedpattern.h"
#endif

struct lcm {
	struct device *dev;
	struct drm_panel panel;
	struct backlight_device *backlight;
	struct gpio_desc *reset_gpio;
	struct gpio_desc *bias_pos, *bias_neg;
	struct gpio_desc *bl_iset_en_gpio;

	bool prepared;
	bool enabled;

	int error;
	bool hbm_en;
	unsigned int cabc_mode;
};

#define lcm_dcs_write_seq(ctx, seq...)						\
	({									\
		const u8 d[] = {seq};						\
		BUILD_BUG_ON_MSG(ARRAY_SIZE(d) > 64,				\
				 "DCS sequence too big for stack");		\
		lcm_dcs_write(ctx, d, ARRAY_SIZE(d));				\
	})

#define lcm_dcs_write_seq_static(ctx, seq...)					\
	({									\
		static const u8 d[] = {seq};					\
		lcm_dcs_write(ctx, d, ARRAY_SIZE(d));				\
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
		dev_err(ctx->dev, "error %zd writing seq: %ph\n", ret, data);
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
		dev_err(ctx->dev, "error %d reading dcs seq:(%#x)\n", ret, cmd);
		ctx->error = ret;
	}

	return ret;
}

static void lcm_panel_get_data(struct lcm *ctx)
{
	u8 buffer[3] = {0};
	static int ret;

	if (ret == 0) {
		ret = lcm_dcs_read(ctx, 0x0A, buffer, 1);
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
		pr_err("get dsv_pos fail, error: %d\n", ret);
		return ret;
	}

	disp_bias_neg = regulator_get(NULL, "dsv_neg");
	if (IS_ERR(disp_bias_neg)) { /* handle return value */
		ret = PTR_ERR(disp_bias_neg);
		pr_err("get dsv_neg fail, error: %d\n", ret);
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
		pr_err("set voltage disp_bias_pos fail, ret = %d\n", ret);
	retval |= ret;

	ret = regulator_set_voltage(disp_bias_neg, 5400000, 5400000);
	if (ret < 0)
		pr_err("set voltage disp_bias_neg fail, ret = %d\n", ret);
	retval |= ret;

	/* enable regulator */
	ret = regulator_enable(disp_bias_pos);
	if (ret < 0)
		pr_err("enable regulator disp_bias_pos fail, ret = %d\n", ret);
	retval |= ret;

	ret = regulator_enable(disp_bias_neg);
	if (ret < 0)
		pr_err("enable regulator disp_bias_neg fail, ret = %d\n", ret);
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
		pr_err("disable regulator disp_bias_neg fail, ret = %d\n", ret);
	retval |= ret;

	ret = regulator_disable(disp_bias_pos);
	if (ret < 0)
		pr_err("disable regulator disp_bias_pos fail, ret = %d\n", ret);
	retval |= ret;

	return retval;
}
#endif

static void lcm_panel_init(struct lcm *ctx)
{
	ctx->reset_gpio =
		devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio)) {
		dev_err(ctx->dev, "%s: cannot get reset_gpio %ld\n",
			__func__, PTR_ERR(ctx->reset_gpio));
		return;
	}
	udelay(5 * 1000);
	gpiod_set_value(ctx->reset_gpio, 0);
	udelay(5 * 1000);
	gpiod_set_value(ctx->reset_gpio, 1);
	udelay(15 * 1000);
	devm_gpiod_put(ctx->dev, ctx->reset_gpio);

	lcm_dcs_write_seq_static(ctx,0xF0,0x5A,0x59);
	lcm_dcs_write_seq_static(ctx,0xF1,0xA5,0xA6);
	lcm_dcs_write_seq_static(ctx,0xB0,0x87,0x86,0x85,0x84,0x88,0x89,0x00,0x00,0x33,0x33,0x33,0x33,0x00,0x00,0x00,0x5E,0x00,0x00,0x0F,0x05,0x04,0x03,0x02,0x01,0x02,0x03,0x04,0x00,0x00,0x00);
	lcm_dcs_write_seq_static(ctx,0xB1,0x53,0x43,0x85,0x00,0x00,0x00,0x00,0x5E,0x00,0x00,0x04,0x08,0x54,0x00,0x00,0x00,0x44,0x40,0x02,0x01,0x40,0x02,0x01,0x40,0x02,0x01,0x40,0x02,0x01);//GCK=4H
	lcm_dcs_write_seq_static(ctx,0xB5,0x08,0x00,0x00,0xc0,0x00,0x04,0x06,0xc1,0xc1,0x0C,0x0C,0x0E,0x0E,0x10,0x10,0x12,0x12,0x03,0x03,0x03,0x03,0x03,0xFF,0xFF,0xFC,0x00,0x00,0x00);
	lcm_dcs_write_seq_static(ctx,0xB4,0x09,0x00,0x00,0xc0,0x00,0x05,0x07,0xc1,0xc1,0x0D,0x0D,0x0F,0x0F,0x11,0x11,0x13,0x13,0x03,0x03,0x03,0x03,0x03,0xFF,0xFF,0xFC,0x00,0x00,0x00);
	lcm_dcs_write_seq_static(ctx,0xB8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00);
	lcm_dcs_write_seq_static(ctx,0xBB,0x01,0x05,0x09,0x11,0x0D,0x19,0x1D,0x55,0x25,0x69,0x00,0x21,0x25);
	lcm_dcs_write_seq_static(ctx,0xBC,0x00,0x00,0x00,0x00,0x02,0x20,0xFF,0x00,0x03,0x13,0x01,0x73,0x33,0x00);
	lcm_dcs_write_seq_static(ctx,0xBD,0xE9,0x02,0x4F,0xCF,0x72,0xA4,0x08,0x44,0xAE,0x15);
	lcm_dcs_write_seq_static(ctx,0xBE,0x73,0x73,0x50,0x32,0x0C,0x77,0x43,0x07,0x0E,0x0E);  //VOP=5.3V; VGH=15V; VGL=-10V
	lcm_dcs_write_seq_static(ctx,0xBF,0x07,0x25,0x07,0x25,0x7F,0x00,0x11,0x04);
	lcm_dcs_write_seq_static(ctx,0xC0,0x10,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0xFF,0x00);
	lcm_dcs_write_seq_static(ctx,0xC1,0xC0,0x0C,0x20,0x96,0x04,0x32,0x32,0x04,0x2A,0x40,0x36,0x00,0x07,0xCF,0xFF,0xFF,0xC0,0x00,0xC0);
	lcm_dcs_write_seq_static(ctx,0xC2,0x00);
	lcm_dcs_write_seq_static(ctx,0xC3,0x06,0x00,0xFF,0x00,0xFF,0x00,0x00,0x81,0x01,0x00);
	lcm_dcs_write_seq_static(ctx,0xC4,0x84,0x01,0x2B,0x41,0x00,0x3C,0x00,0x03,0x03,0x2E);
	lcm_dcs_write_seq_static(ctx,0xC5,0x03,0x1C,0xC0,0xC0,0x40,0x10,0x42,0x44,0x0a,0x09,0x14);
	lcm_dcs_write_seq_static(ctx,0xC6,0x88,0x16,0x20,0x28,0x29,0x30,0x7F,0x34,0x08,0x04);
	lcm_dcs_write_seq_static(ctx,0xCB,0x00);
	lcm_dcs_write_seq_static(ctx,0xD0,0x80,0x0D,0xFF,0x0F,0x63);
	lcm_dcs_write_seq_static(ctx,0xD2,0x42);
	lcm_dcs_write_seq_static(ctx,0xE0,0x30,0x00,0x80,0x88,0x11,0x3F,0x22,0x62,0xDF,0xA0,0x04,0xCC,0x01,0xFF,0xF6,0xFF,0xF0,0xFD,0xFF,0xFD,0xF8,0xF5,0xFC,0xFC,0xFD,0xFF);
	lcm_dcs_write_seq_static(ctx,0xE1,0xEF,0xFE,0xFE,0xFE,0xFE,0xEE,0xF0,0x20,0x33,0xFF,0x00,0x00,0x6A,0x90,0xC0,0x0D,0x6A,0xF0,0x3E,0xFF,0x00,0x07,0xD0);
	lcm_dcs_write_seq_static(ctx,0xF1,0x5A,0x59);
	lcm_dcs_write_seq_static(ctx,0xF0,0xA5,0xA6);
	lcm_dcs_write_seq_static(ctx,0x35,0x00);
	lcm_dcs_write_seq_static(ctx,0x51,0x00,0x00);
	lcm_dcs_write_seq_static(ctx,0x53,0x2C);
	lcm_dcs_write_seq_static(ctx,0x55,0x01);
	lcm_dcs_write_seq_static(ctx,0x11);
	msleep(120);
	lcm_dcs_write_seq_static(ctx,0x29);
	msleep(20);
	lcm_dcs_write_seq_static(ctx,0x26,0x01);
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

	pr_info("%s\n", __func__);
	if (!ctx->prepared)
		return 0;
	lcm_dcs_write_seq_static(ctx, 0x26, 0x08);
	lcm_dcs_write_seq_static(ctx, 0x28);
	msleep(20);
	lcm_dcs_write_seq_static(ctx, 0x10);
	msleep(65);

	ctx->error = 0;
	ctx->prepared = false;
#if defined(CONFIG_RT5081_PMU_DSV) || defined(CONFIG_MT6370_PMU_DSV)
	lcm_panel_bias_disable();
#else
	ctx->reset_gpio =
		devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio)) {
		dev_err(ctx->dev, "%s: cannot get reset_gpio %ld\n",
			__func__, PTR_ERR(ctx->reset_gpio));
		return PTR_ERR(ctx->reset_gpio);
	}
	gpiod_set_value(ctx->reset_gpio, 0);
	devm_gpiod_put(ctx->dev, ctx->reset_gpio);


	ctx->bias_neg = devm_gpiod_get_index(ctx->dev,
		"bias", 1, GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->bias_neg)) {
		dev_err(ctx->dev, "%s: cannot get bias_neg %ld\n",
			__func__, PTR_ERR(ctx->bias_neg));
		return PTR_ERR(ctx->bias_neg);
	}
	gpiod_set_value(ctx->bias_neg, 0);
	devm_gpiod_put(ctx->dev, ctx->bias_neg);

	udelay(1000);

	ctx->bias_pos = devm_gpiod_get_index(ctx->dev,
		"bias", 0, GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->bias_pos)) {
		dev_err(ctx->dev, "%s: cannot get bias_pos %ld\n",
			__func__, PTR_ERR(ctx->bias_pos));
		return PTR_ERR(ctx->bias_pos);
	}
	gpiod_set_value(ctx->bias_pos, 0);
	devm_gpiod_put(ctx->dev, ctx->bias_pos);
#endif

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
#else
	ctx->bias_pos = devm_gpiod_get_index(ctx->dev,
		"bias", 0, GPIOD_OUT_HIGH);

	if (IS_ERR(ctx->bias_pos)) {
		dev_err(ctx->dev, "%s: cannot get bias_pos %ld\n",
			__func__, PTR_ERR(ctx->bias_pos));
		return PTR_ERR(ctx->bias_pos);
	}
	gpiod_set_value(ctx->bias_pos, 1);
	devm_gpiod_put(ctx->dev, ctx->bias_pos);

	udelay(2000);

	ctx->bias_neg = devm_gpiod_get_index(ctx->dev,
		"bias", 1, GPIOD_OUT_HIGH);

	if (IS_ERR(ctx->bias_neg)) {
		dev_err(ctx->dev, "%s: cannot get bias_neg %ld\n",
			__func__, PTR_ERR(ctx->bias_neg));
		return PTR_ERR(ctx->bias_neg);
	}
	gpiod_set_value(ctx->bias_neg, 1);
	devm_gpiod_put(ctx->dev, ctx->bias_neg);
#endif
	lcm_panel_init(ctx);
	ret = ctx->error;
	if (ret < 0)
		lcm_unprepare(panel);

	ctx->prepared = true;

	ctx->cabc_mode = 0; //UI mode
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

#define HFP (50)
#define HSA (4)
#define HBP (50)
#define VFP_45HZ (1900)
#define VFP_60HZ (1040)
#define VFP_90HZ (150)
#define VSA (4)
#define VBP (12)
#define VAC (1600)
#define HAC (720)

static struct drm_display_mode default_mode = {
	.clock = 131312,
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
	.clock = 130966,
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
static int panel_ext_reset(struct drm_panel *panel, int on)
{
	struct lcm *ctx = panel_to_lcm(panel);

	ctx->reset_gpio =
		devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio)) {
		dev_err(ctx->dev, "%s: cannot get reset_gpio %ld\n",
			__func__, PTR_ERR(ctx->reset_gpio));
		return PTR_ERR(ctx->reset_gpio);
	}
	gpiod_set_value(ctx->reset_gpio, on);
	devm_gpiod_put(ctx->dev, ctx->reset_gpio);

	return 0;
}

static int panel_ata_check(struct drm_panel *panel)
{
	return 0;
}

static int lcm_setbacklight_cmdq(void *dsi, dcs_write_gce cb, void *handle,
				 unsigned int level)
{
	char bl_tb0[] = {0x51, 0xf, 0xff};

	pr_info("%s backlight = %d\n", __func__, level);

	if (level > 255)
		level = 255;

	level = level * 4095 / 255;
	bl_tb0[1] = ((level >> 8) & 0xf);
	bl_tb0[2] = (level & 0xff);

	if (!cb)
		return -1;

	cb(dsi, handle, bl_tb0, ARRAY_SIZE(bl_tb0));
	return 0;
}

static int lcm_get_virtual_heigh(void)
{
	return VAC;
}

static int lcm_get_virtual_width(void)
{
	return HAC;
}

static struct mtk_panel_params ext_params = {
	.pll_clk = 415,
	.vfp_low_power = VFP_45HZ,
	.cust_esd_check = 1,
	.esd_check_enable = 1,
	.lcm_esd_check_table[0] = {
		.cmd = 0x0a,
		.count = 1,
		.para_list[0] = 0x9c,
	},
	.dyn_fps = {
		.switch_en = 1,
		.vact_timing_fps = 90,
	},
	.panel_hbm_mode = HBM_MODE_GPIO,
};

static struct mtk_panel_params ext_params_90hz = {
	.pll_clk = 415,
	.vfp_low_power = VFP_60HZ,
	.cust_esd_check = 1,
	.esd_check_enable = 1,
	.lcm_esd_check_table[0] = {
		.cmd = 0x0a,
		.count = 1,
		.para_list[0] = 0x9c,
	},
	.dyn_fps = {
		.switch_en = 1,
		.vact_timing_fps = 90,
	},
	.panel_hbm_mode = HBM_MODE_GPIO,
};

static int mtk_panel_ext_param_set(struct drm_panel *panel, unsigned int mode)
{
	struct mtk_panel_ext *ext = find_panel_ext(panel);
	struct lcm *ctx = panel_to_lcm(panel);
	int ret = 0;

	if (mode == 0) {
		ext->params = &ext_params;
		lcm_dcs_write_seq_static(ctx, 0x26, 0x01);
	}
	else if (mode == 1) {
		ext->params = &ext_params_90hz;
		lcm_dcs_write_seq_static(ctx, 0x26, 0x02);
	}
	else
		ret = 1;

	return ret;
}

static int mtk_panel_ext_param_get(struct mtk_panel_params *ext_para,
			 unsigned int mode)
{
	int ret = 0;

	if (mode == 0)
		ext_para = &ext_params;
	else if (mode == 1)
		ext_para = &ext_params_90hz;
	else
		ret = 1;

	return ret;

}

static int panel_cabc_set_cmdq(struct drm_panel *panel, void *dsi,
			      dcs_write_gce cb, void *handle, unsigned int cabc_mode)
{
	const unsigned int cabc_value_map[3] = {1, 3, 0};
	int cabc_value = 1;
	char cabc_tb[2] = {0x55, 0x01};
	u8 cabc_tb1[] = {0xF0, 0x5A, 0x59};//Password open
	u8 cabc_tb2[] = {0xF1, 0xA5, 0xA6};//Password open
	u8 cabc_ui_tb3[] = {0xE0, 0x30, 0x00, 0x80, 0x88, 0x11, 0x3F, 0x22, 0x62, 0xDF, 0xA0, 0x04, 0xCC, 0x01, 0xFF, 0xF6, 0xFF, 0xF0, 0xFD, 0xFF, 0xFD, 0xF8, 0xF5, 0xFC, 0xFC, 0xFD, 0xFF};
	u8 cabc_ui_tb4[] = {0xE1, 0xEF, 0xFE, 0xFE, 0xFE, 0xFE, 0xEE, 0xF0, 0x20, 0x33, 0xFF, 0x00, 0x00, 0x6A, 0x90, 0xC0, 0x0D, 0x6A, 0xF0, 0x3E, 0xFF, 0x00, 0x07, 0xD0};
	u8 cabc_movie_tb3[] = {0xE0, 0x30, 0x00, 0x80, 0x88, 0x11, 0x3F, 0x22, 0x62, 0xDF, 0xA0, 0x04, 0xCC, 0x01, 0xFF, 0xFA, 0xFF, 0xF0, 0xFD, 0xFF, 0xFB, 0xF8, 0xF5, 0xFC, 0xFC, 0xFB, 0xFF};
	u8 cabc_movie_tb4[] = {0xE1, 0xBC, 0xF8, 0xCC, 0xFA, 0xDB, 0x9B, 0xF0, 0xE7, 0xF0, 0x85, 0xF0, 0x70, 0x00, 0x50, 0x00, 0x9A, 0xFD, 0xF0, 0xE0, 0xFF, 0x00, 0x07, 0xD0};
	u8 cabc_tb5[] = {0xF1, 0x5A, 0x59};//Password off
	u8 cabc_tb6[] = {0xF0, 0xA5, 0xA6};//Password off

	struct lcm *ctx = panel_to_lcm(panel);

	if (ctx->cabc_mode == cabc_mode)
		goto done;

	if (!cb)
		return -1;

	if (cabc_mode > 2) return -1;

	cabc_value = cabc_value_map[cabc_mode];
	cabc_tb[1] = cabc_value;

	cb(dsi, handle, cabc_tb, ARRAY_SIZE(cabc_tb));
	cb(dsi, handle, cabc_tb1, ARRAY_SIZE(cabc_tb1));
	cb(dsi, handle, cabc_tb2, ARRAY_SIZE(cabc_tb2));

	if (cabc_value == 3) {
		cb(dsi, handle, cabc_movie_tb3, ARRAY_SIZE(cabc_movie_tb3));
		cb(dsi, handle, cabc_movie_tb4, ARRAY_SIZE(cabc_movie_tb4));
	}else {
		cb(dsi, handle, cabc_ui_tb3, ARRAY_SIZE(cabc_ui_tb3));
		cb(dsi, handle, cabc_ui_tb4, ARRAY_SIZE(cabc_ui_tb4));
	}

	cb(dsi, handle, cabc_tb1, ARRAY_SIZE(cabc_tb5));
	cb(dsi, handle, cabc_tb2, ARRAY_SIZE(cabc_tb6));
	pr_info(" set cabc to %d\n", cabc_value);

done:
	ctx->cabc_mode = cabc_mode;
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
	struct lcm *ctx = panel_to_lcm(panel);

	if (hbm_en) {
		ctx->bl_iset_en_gpio =
		devm_gpiod_get(ctx->dev, "bl-iset-en", GPIOD_OUT_LOW);
		if (IS_ERR(ctx->bl_iset_en_gpio)) {
			dev_err(ctx->dev, "%s: cannot get bl_iset_en_gpio %ld\n",
				__func__, PTR_ERR(ctx->bl_iset_en_gpio));
			return -1;
		}
		devm_gpiod_put(ctx->dev, ctx->bl_iset_en_gpio);
	} else {
		ctx->bl_iset_en_gpio =
		devm_gpiod_get(ctx->dev, "bl-iset-en", GPIOD_IN);
		if (IS_ERR(ctx->bl_iset_en_gpio)) {
			dev_err(ctx->dev, "%s: cannot get bl_iset_en_gpio %ld\n",
				__func__, PTR_ERR(ctx->bl_iset_en_gpio));
			return -1;
		}
		devm_gpiod_put(ctx->dev, ctx->bl_iset_en_gpio);
	}
	ctx->hbm_en = hbm_en;
	pr_info("%s set HBM to %d\n", __func__, hbm_en);
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
	.ext_param_set = mtk_panel_ext_param_set,
	.ext_param_get = mtk_panel_ext_param_get,
	.ata_check = panel_ata_check,
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

static int lcm_get_modes(struct drm_panel *panel)
{
	struct drm_display_mode *mode;
	struct drm_display_mode *mode2;

	mode = drm_mode_duplicate(panel->drm, &default_mode);
	if (!mode) {
		dev_err(panel->drm->dev, "failed to add mode %ux%ux@%u\n",
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
			 performance_mode.hdisplay, performance_mode.vdisplay,
			 performance_mode.vrefresh);
		return -ENOMEM;
	}

	drm_mode_set_name(mode2);
	mode2->type = DRM_MODE_TYPE_DRIVER;
	drm_mode_probed_add(panel->connector, mode2);

	panel->connector->display_info.width_mm = 67;
	panel->connector->display_info.height_mm = 150;

	panel->connector->display_info.panel_ver = 0x01;
	panel->connector->display_info.panel_id = 0xFF0A1C91;
	strcpy(panel->connector->display_info.panel_name, "mipi_mot_vid_truly_hdp_681");
	strcpy(panel->connector->display_info.panel_supplier, "truly_chipone");

	return 1;
}

static const struct drm_panel_funcs lcm_drm_funcs = {
	.disable = lcm_disable,
	.unprepare = lcm_unprepare,
	.prepare = lcm_prepare,
	.enable = lcm_enable,
	.get_modes = lcm_get_modes,
};

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
	dsi->mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_SYNC_PULSE
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
		dev_err(dev, "%s: cannot get reset-gpios %ld\n",
			__func__, PTR_ERR(ctx->reset_gpio));
		return PTR_ERR(ctx->reset_gpio);
	}
	devm_gpiod_put(dev, ctx->reset_gpio);

	ctx->bias_pos = devm_gpiod_get_index(dev, "bias", 0, GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->bias_pos)) {
		dev_err(dev, "%s: cannot get bias-pos 0 %ld\n",
			__func__, PTR_ERR(ctx->bias_pos));
		return PTR_ERR(ctx->bias_pos);
	}
	devm_gpiod_put(dev, ctx->bias_pos);

	ctx->bias_neg = devm_gpiod_get_index(dev, "bias", 1, GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->bias_neg)) {
		dev_err(dev, "%s: cannot get bias-neg 1 %ld\n",
			__func__, PTR_ERR(ctx->bias_neg));
		return PTR_ERR(ctx->bias_neg);
	}
	devm_gpiod_put(dev, ctx->bias_neg);

	ctx->bl_iset_en_gpio = devm_gpiod_get(dev, "bl-iset-en", GPIOD_IN);
	if (IS_ERR(ctx->bl_iset_en_gpio)) {
		dev_err(dev, "%s: cannot get bl_iset_en_gpio %ld\n",
			__func__, PTR_ERR(ctx->bl_iset_en_gpio));
		return PTR_ERR(ctx->bl_iset_en_gpio);
	}
	devm_gpiod_put(dev, ctx->bl_iset_en_gpio);

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
	{ .compatible = "icnl9911c,truly,vdo", },
	{ }
};

MODULE_DEVICE_TABLE(of, lcm_of_match);

static struct mipi_dsi_driver lcm_driver = {
	.probe = lcm_probe,
	.remove = lcm_remove,
	.driver = {
		.name = "panel-truly-icnl9911c-vdo",
		.owner = THIS_MODULE,
		.of_match_table = lcm_of_match,
	},
};

module_mipi_dsi_driver(lcm_driver);

MODULE_AUTHOR("Cui Zhang <cui.zhang@mediatek.com>");
MODULE_DESCRIPTION("truly icnl9911c VDO LCD Panel Driver");
MODULE_LICENSE("GPL v2");
