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
#include "include/dsi-panel-mot-csot-ft8726-650-fhdp-dphy-vdo-120hz.h"
#endif

/* option function to read data from some panel address */
/* #define PANEL_SUPPORT_READBACK */

#ifdef BIAS_SM5109
extern int __attribute__ ((weak)) sm5109_BiasPower_disable(u32 pwrdown_delay);
extern int __attribute__ ((weak)) sm5109_BiasPower_enable(u32 avdd, u32 avee,u32 pwrup_delay);
#else
//To be removed when sm5109 bias config ready
#define BIAS_TMP_V0
#endif

struct csot {
	struct device *dev;
	struct drm_panel panel;
	struct backlight_device *backlight;
	struct gpio_desc *reset_gpio;
	struct gpio_desc *bias_n_gpio;
	struct gpio_desc *bias_p_gpio;

	bool prepared;
	bool enabled;

	int error;
};

//static char bl_tb0[] = { 0x51, 0xff };

#define csot_dcs_write_seq(ctx, seq...)                                     \
	({                                                                     \
		const u8 d[] = {seq};                                          \
		BUILD_BUG_ON_MSG(ARRAY_SIZE(d) > 64,                           \
				 "DCS sequence too big for stack");            \
		csot_dcs_write(ctx, d, ARRAY_SIZE(d));                      \
	})

#define csot_dcs_write_seq_static(ctx, seq...)                              \
	({                                                                     \
		static const u8 d[] = {seq};                                   \
		csot_dcs_write(ctx, d, ARRAY_SIZE(d));                      \
	})

static inline struct csot *panel_to_csot(struct drm_panel *panel)
{
	return container_of(panel, struct csot, panel);
}

#ifdef PANEL_SUPPORT_READBACK
static int csot_dcs_read(struct csot *ctx, u8 cmd, void *data, size_t len)
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

static void csot_panel_get_data(struct csot *ctx)
{
	u8 buffer[3] = {0};
	static int ret;

	if (ret == 0) {
		ret = csot_dcs_read(ctx, 0x0A, buffer, 1);
		dev_info(ctx->dev, "return %d data(0x%08x) to dsi engine\n",
			 ret, buffer[0] | (buffer[1] << 8));
	}
}
#endif

static void csot_dcs_write(struct csot *ctx, const void *data, size_t len)
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


static void csot_panel_init(struct csot *ctx)
{
	pr_info("disp: %s+\n", __func__);

#ifdef BIAS_SM5109
	sm5109_BiasPower_enable(15,15,5);
#endif

#ifdef BIAS_TMP_V0
	ctx->bias_n_gpio = devm_gpiod_get(ctx->dev, "bias_n", GPIOD_OUT_HIGH);
	ctx->bias_p_gpio = devm_gpiod_get(ctx->dev, "bias_p", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->bias_n_gpio)) {
		dev_err(ctx->dev, "%s: cannot get bias_n_gpio %ld\n",
			__func__, PTR_ERR(ctx->bias_n_gpio));
		//return;
	}
	else if (IS_ERR(ctx->bias_p_gpio)) {
		dev_err(ctx->dev, "%s: cannot get bias_p_gpio %ld\n",
			__func__, PTR_ERR(ctx->bias_p_gpio));
		//return;
	}
	else {
		gpiod_set_value(ctx->bias_p_gpio, 1);
		msleep(5);
		gpiod_set_value(ctx->bias_n_gpio, 1);
		msleep(5);

		devm_gpiod_put(ctx->dev, ctx->bias_n_gpio);
		devm_gpiod_put(ctx->dev, ctx->bias_p_gpio);
	}
#endif

	ctx->reset_gpio = devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio)) {
		dev_err(ctx->dev, "%s: cannot get reset_gpio %ld\n",
			__func__, PTR_ERR(ctx->reset_gpio));
		//return;
	}
	else {
		gpiod_set_value(ctx->reset_gpio, 1);
		usleep_range(5000,5001);
		gpiod_set_value(ctx->reset_gpio, 0);
		usleep_range(5000,5001);
		gpiod_set_value(ctx->reset_gpio, 1);
		usleep_range(20000,20001);

		devm_gpiod_put(ctx->dev, ctx->reset_gpio);
		pr_info("disp: %s reset_gpio\n", __func__);
	}

	csot_dcs_write_seq_static(ctx,0x00, 0x00);
	csot_dcs_write_seq_static(ctx,0xFF, 0x87,0x20,0x01);
	csot_dcs_write_seq_static(ctx,0x00, 0x80);
	csot_dcs_write_seq_static(ctx,0xFF, 0x87,0x20);
	csot_dcs_write_seq_static(ctx,0x00, 0xA3);
	csot_dcs_write_seq_static(ctx,0xB3, 0x09,0x60,0x00,0x18);
	csot_dcs_write_seq_static(ctx,0x00, 0x80);
	csot_dcs_write_seq_static(ctx,0xC0, 0x00 ,0x4D ,0x00 ,0x2D ,0x00 ,0x11);
	csot_dcs_write_seq_static(ctx,0x00, 0x90);
	csot_dcs_write_seq_static(ctx,0xC0, 0x00 ,0x4D ,0x00 ,0x2D ,0x00 ,0x11);
	csot_dcs_write_seq_static(ctx,0x00, 0xA0);
	csot_dcs_write_seq_static(ctx,0xC0, 0x00 ,0x66 ,0x00 ,0x2D ,0x00 ,0x11);
	csot_dcs_write_seq_static(ctx,0x00, 0xB0);
	csot_dcs_write_seq_static(ctx,0xC0, 0x00 ,0xD5 ,0x00 ,0x2D ,0x11);
	csot_dcs_write_seq_static(ctx,0x00, 0x60);
	csot_dcs_write_seq_static(ctx,0xC0, 0x00 ,0xB4 ,0x00 ,0x2D ,0x00 ,0x11);
	csot_dcs_write_seq_static(ctx,0x00, 0x70);
	csot_dcs_write_seq_static(ctx,0xC0, 0x00, 0xC0 ,0x00 ,0xC0 ,0x0D ,0x03 ,0x14 ,0x00 ,0x00 ,0x13 ,0x01 ,0x69);
	csot_dcs_write_seq_static(ctx,0x00, 0xA3);
	csot_dcs_write_seq_static(ctx,0xC1, 0x00, 0x6C, 0x00 ,0x3E ,0x00 ,0x02);
	csot_dcs_write_seq_static(ctx,0x00, 0x80);
	csot_dcs_write_seq_static(ctx,0xCE, 0x01, 0x81 ,0xFF ,0xFF ,0x00 ,0xC0 ,0x00 ,0xC8 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x70 ,0x00,0x98);
	csot_dcs_write_seq_static(ctx,0x00, 0x90);
	csot_dcs_write_seq_static(ctx,0xCE, 0x00 ,0xBD ,0x11 ,0x3A ,0x00 ,0xBD ,0x80 ,0xFF ,0xFF ,0x00 ,0x06 ,0x40 ,0x0E ,0x16 ,0x00);
	csot_dcs_write_seq_static(ctx,0x00, 0xA0);
	csot_dcs_write_seq_static(ctx,0xCE, 0x10 ,0x00 ,0x00);
	csot_dcs_write_seq_static(ctx,0x00, 0xB0);
	csot_dcs_write_seq_static(ctx,0xCE, 0x22 ,0x00 ,0x00);
	csot_dcs_write_seq_static(ctx,0x00, 0xD1);
	csot_dcs_write_seq_static(ctx,0xCE, 0x00 ,0x00 ,0x01 ,0x00 ,0x00 ,0x00 ,0x00);
	csot_dcs_write_seq_static(ctx,0x00, 0xE1);
	csot_dcs_write_seq_static(ctx,0xCE, 0x03 ,0x03 ,0x14 ,0x03 ,0x14 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00);
	csot_dcs_write_seq_static(ctx,0x00, 0xF1);
	csot_dcs_write_seq_static(ctx,0xCE, 0x27 ,0x1D ,0x00 ,0x00 ,0xFD ,0x01 ,0x20 ,0x00 ,0x00);
	csot_dcs_write_seq_static(ctx,0x00, 0xB0);
	csot_dcs_write_seq_static(ctx,0xCF, 0x00 ,0x00 ,0x97 ,0x9B);
	csot_dcs_write_seq_static(ctx,0x00, 0xB5);
	csot_dcs_write_seq_static(ctx,0xCF, 0x04 ,0x04 ,0xF9 ,0xFD);
	csot_dcs_write_seq_static(ctx,0x00, 0xC0);
	csot_dcs_write_seq_static(ctx,0xCF, 0x09 ,0x09 ,0x5B ,0x5F);
	csot_dcs_write_seq_static(ctx,0x00, 0xC5);
	csot_dcs_write_seq_static(ctx,0xCF, 0x09 ,0x09 ,0x61 ,0x65);
	csot_dcs_write_seq_static(ctx,0x00, 0xD1);
	csot_dcs_write_seq_static(ctx,0xC1, 0x0A ,0xDF ,0x0F ,0x1C ,0x19 ,0xCB ,0x0A ,0xDF ,0x0F ,0x2D ,0x19 ,0xE6);
	csot_dcs_write_seq_static(ctx,0x00, 0xE1);
	csot_dcs_write_seq_static(ctx,0xC1, 0x0F ,0x1C);
	csot_dcs_write_seq_static(ctx,0x00, 0xE4);
	csot_dcs_write_seq_static(ctx,0xCF, 0x09 ,0xF1 ,0x09 ,0xF0 ,0x09 ,0xF0 ,0x09 ,0xF0 ,0x09 ,0xF0 ,0x09 ,0xF0);
	csot_dcs_write_seq_static(ctx,0x00, 0x80);
	csot_dcs_write_seq_static(ctx,0xC1, 0x44 ,0x44);
	csot_dcs_write_seq_static(ctx,0x00, 0x90);
	csot_dcs_write_seq_static(ctx,0xC1, 0x03);
	csot_dcs_write_seq_static(ctx,0x00, 0xF5);
	csot_dcs_write_seq_static(ctx,0xCF, 0x02);
	csot_dcs_write_seq_static(ctx,0x00, 0xF6);
	csot_dcs_write_seq_static(ctx,0xCF, 0x78);
	csot_dcs_write_seq_static(ctx,0x00, 0xF1);
	csot_dcs_write_seq_static(ctx,0xCF, 0x78);
	csot_dcs_write_seq_static(ctx,0x00, 0xF0);
	csot_dcs_write_seq_static(ctx,0xC1, 0x00);
	csot_dcs_write_seq_static(ctx,0x00, 0xCC);
	csot_dcs_write_seq_static(ctx,0xC1, 0x18);
	csot_dcs_write_seq_static(ctx,0x00, 0x91);
	csot_dcs_write_seq_static(ctx,0xC4, 0x08);
	csot_dcs_write_seq_static(ctx,0x00,0x80);
	csot_dcs_write_seq_static(ctx,0xC5,0x88);
	csot_dcs_write_seq_static(ctx,0x00,0x88);
	csot_dcs_write_seq_static(ctx,0xC4,0x08);
	csot_dcs_write_seq_static(ctx,0x00,0x80);
	csot_dcs_write_seq_static(ctx,0xC2,0x84,0x00,0x05,0x85,0x83,0x00,0x05,0x85);
	csot_dcs_write_seq_static(ctx,0x00,0xA0);
	csot_dcs_write_seq_static(ctx,0xC2,0x82,0x04,0x00,0x05,0x85,0x81,0x04,0x00,0x05,0x85,0x80,0x04,0x00,0x05,0x85);
	csot_dcs_write_seq_static(ctx,0x00,0xB0);
	csot_dcs_write_seq_static(ctx,0xC2,0x01,0x04,0x00,0x05,0x85,0x02,0x04,0x00,0x05,0x85,0x03,0x04,0x00,0x05,0x85);
	csot_dcs_write_seq_static(ctx,0x00,0xC0);
	csot_dcs_write_seq_static(ctx,0xC2,0x04,0x04,0x00,0x05,0x85,0x05,0x04,0x00,0x05,0x85);
	csot_dcs_write_seq_static(ctx,0x00,0xE0);
	csot_dcs_write_seq_static(ctx,0xC2,0x77,0x77,0x77,0x77);
	csot_dcs_write_seq_static(ctx,0x00,0xE8);
	csot_dcs_write_seq_static(ctx,0xC2,0x18,0x99,0x6B,0x75,0x00,0x00,0x00,0x00);
	csot_dcs_write_seq_static(ctx,0x00,0xD0);
	csot_dcs_write_seq_static(ctx,0xC3,0x35,0x0A,0x00,0x00,0x35,0x0A,0x00,0x00,0x35,0x0A,0x00,0x00,0x00,0x00,0x00,0x00);
	csot_dcs_write_seq_static(ctx,0x00,0xE0);
	csot_dcs_write_seq_static(ctx,0xC3,0x35,0x0A,0x00,0x00,0x35,0x0A,0x00,0x00,0x35,0x0A,0x00,0x00,0x00,0x00,0x00,0x00);
	csot_dcs_write_seq_static(ctx,0x00,0x80);
	csot_dcs_write_seq_static(ctx,0xCB,0x00,0x05,0x00,0x00,0x05,0x05,0x00,0x05,0x0A,0xC0,0xC5,0x00,0x0F,0x00,0x00,0x00);
	csot_dcs_write_seq_static(ctx,0x00,0x90);
	csot_dcs_write_seq_static(ctx,0xCB,0x00,0x04,0x00,0x00,0x04,0x04,0x00,0x04,0x08,0x00,0x04,0x00,0x0C,0x00,0x00,0x00);
	csot_dcs_write_seq_static(ctx,0x00,0xA0);
	csot_dcs_write_seq_static(ctx,0xCB,0x00,0x00,0x00,0x00,0x00,0x00,0x0C,0x00);
	csot_dcs_write_seq_static(ctx,0x00,0xB0);
	csot_dcs_write_seq_static(ctx,0xCB,0x10,0x51,0x84,0xC0);
	csot_dcs_write_seq_static(ctx,0x00,0xC0);
	csot_dcs_write_seq_static(ctx,0xCB,0x10,0x51,0x84,0xC0);
	csot_dcs_write_seq_static(ctx,0x00,0xD5);
	csot_dcs_write_seq_static(ctx,0xCB,0x81,0x00,0x81,0x81,0x00,0x81,0x81,0x00,0x81,0x81,0x00);
	csot_dcs_write_seq_static(ctx,0x00,0xE0);
	csot_dcs_write_seq_static(ctx,0xCB,0x81,0x81,0x00,0x81,0x81,0x00,0x81,0x81,0x00,0x81,0x81,0x00,0x81);
	csot_dcs_write_seq_static(ctx,0x00,0x80);
	csot_dcs_write_seq_static(ctx,0xCC,0x03,0x07,0x09,0x0B,0x0D,0x1E,0x26,0x1C,0x26,0x1E,0x20,0x1F,0x26,0x18,0x17,0x16);
	csot_dcs_write_seq_static(ctx,0x00,0x90);
	csot_dcs_write_seq_static(ctx,0xCC,0x26,0x26,0x26,0x18,0x17,0x16,0x26,0x26);
	csot_dcs_write_seq_static(ctx,0x00,0x80);
	csot_dcs_write_seq_static(ctx,0xCD,0x02,0x06,0x08,0x0A,0x0C,0x1E,0x26,0x1C,0x26,0x1E,0x20,0x1F,0x26,0x18,0x17,0x16);
	csot_dcs_write_seq_static(ctx,0x00,0x90);
	csot_dcs_write_seq_static(ctx,0xCD,0x26,0x26,0x26,0x18,0x17,0x16,0x26,0x26);
	csot_dcs_write_seq_static(ctx,0x00,0xA0);
	csot_dcs_write_seq_static(ctx,0xCC,0x02,0x08,0x06,0x0C,0x0A,0x1C,0x26,0x1E,0x26,0x1E,0x20,0x1F,0x26,0x18,0x17,0x16);
	csot_dcs_write_seq_static(ctx,0x00,0xB0);
	csot_dcs_write_seq_static(ctx,0xCC,0x26,0x26,0x26,0x18,0x17,0x16,0x26,0x26);
	csot_dcs_write_seq_static(ctx,0x00,0xA0);
	csot_dcs_write_seq_static(ctx,0xCD,0x03,0x09,0x07,0x0D,0x0B,0x1C,0x26,0x1E,0x26,0x1E,0x20,0x1F,0x26,0x18,0x17,0x16);
	csot_dcs_write_seq_static(ctx,0x00,0xB0);
	csot_dcs_write_seq_static(ctx,0xCD,0x26,0x26,0x26,0x18,0x17,0x16,0x26,0x26);
	csot_dcs_write_seq_static(ctx,0x00,0x86);
	csot_dcs_write_seq_static(ctx,0xC0,0x01,0x01,0x01,0x01,0x11,0x04);
	csot_dcs_write_seq_static(ctx,0x00,0x96);
	csot_dcs_write_seq_static(ctx,0xC0,0x01,0x01,0x01,0x01,0x11,0x04);
	csot_dcs_write_seq_static(ctx,0x00,0xa6);
	csot_dcs_write_seq_static(ctx,0xC0,0x00,0x01,0x00,0x01,0x19,0x04);
	csot_dcs_write_seq_static(ctx,0x00,0xA3);
	csot_dcs_write_seq_static(ctx,0xCE,0x01,0x01,0x01,0x01,0x11,0x04);
	csot_dcs_write_seq_static(ctx,0x00,0xB3);
	csot_dcs_write_seq_static(ctx,0xCE,0x01,0x01,0x01,0x01,0x11,0x04);
	csot_dcs_write_seq_static(ctx,0x00,0x66);
	csot_dcs_write_seq_static(ctx,0xC0,0x01,0x08,0x01,0x01,0x2E,0x06);
	csot_dcs_write_seq_static(ctx,0x00,0x93);
	csot_dcs_write_seq_static(ctx,0xC5,0x37);
	csot_dcs_write_seq_static(ctx,0x00,0x97);
	csot_dcs_write_seq_static(ctx,0xC5,0x37);
	csot_dcs_write_seq_static(ctx,0x00,0x9A);
	csot_dcs_write_seq_static(ctx,0xC5,0x19);
	csot_dcs_write_seq_static(ctx,0x00,0x9C);
	csot_dcs_write_seq_static(ctx,0xC5,0x19);
	csot_dcs_write_seq_static(ctx,0x00,0xB6);
	csot_dcs_write_seq_static(ctx,0xC5,0x19,0x19,0x0A,0x0A,0x19,0x19,0x0A,0x0A);
	csot_dcs_write_seq_static(ctx,0x00,0x99);
	csot_dcs_write_seq_static(ctx,0xCF,0x50);//TP_SWB_GIP
	csot_dcs_write_seq_static(ctx,0x00,0x9C);
	csot_dcs_write_seq_static(ctx,0xF5,0x00);//VGHO2 disable
	csot_dcs_write_seq_static(ctx,0x00,0x9E);
	csot_dcs_write_seq_static(ctx,0xF5,0x00);//VGLO2 disable
	csot_dcs_write_seq_static(ctx,0x00,0xB0);
	csot_dcs_write_seq_static(ctx,0xC5,0xD0,0x4A,0x3D,0xD0,0x4A,0x0F);
	csot_dcs_write_seq_static(ctx,0x00,0x00);
	csot_dcs_write_seq_static(ctx,0xD8,0x2B,0x2B);
	csot_dcs_write_seq_static(ctx,0x00,0x00);
	csot_dcs_write_seq_static(ctx,0xD9,0x1E,0x1E,0x1E,0x1E);
	csot_dcs_write_seq_static(ctx,0x00,0x06);
	csot_dcs_write_seq_static(ctx,0xD9,0x1E,0x1E,0x1E);
	csot_dcs_write_seq_static(ctx,0x00,0x82);
	csot_dcs_write_seq_static(ctx,0xA7,0x22,0x02);
	csot_dcs_write_seq_static(ctx,0x00,0x8D);
	csot_dcs_write_seq_static(ctx,0xA7,0x02,0x00,0x02);
	csot_dcs_write_seq_static(ctx,0x00,0x9B);
	csot_dcs_write_seq_static(ctx,0xC4,0xFF);
	csot_dcs_write_seq_static(ctx,0x00,0x94);
	csot_dcs_write_seq_static(ctx,0xE9,0x00);
	csot_dcs_write_seq_static(ctx,0x00,0x9A);
	csot_dcs_write_seq_static(ctx,0xC4,0x11);
	csot_dcs_write_seq_static(ctx,0x00,0x95);
	csot_dcs_write_seq_static(ctx,0xE9,0x10);
	csot_dcs_write_seq_static(ctx,0x00,0xB1);
	csot_dcs_write_seq_static(ctx,0xF5,0x1F);
	csot_dcs_write_seq_static(ctx,0x00,0x80); //disable 21h
	csot_dcs_write_seq_static(ctx,0xB3,0x22);
	csot_dcs_write_seq_static(ctx,0x00,0xB0); //HS lock CMD1
	csot_dcs_write_seq_static(ctx,0xB3,0x00);
	csot_dcs_write_seq_static(ctx,0x00,0x83); //packet loss cover
	csot_dcs_write_seq_static(ctx,0xB0,0x63);
	csot_dcs_write_seq_static(ctx,0x00,0x84);
	csot_dcs_write_seq_static(ctx,0xA4,0x02);
	csot_dcs_write_seq_static(ctx,0x00,0x81);
	csot_dcs_write_seq_static(ctx,0xA6,0x04);
	csot_dcs_write_seq_static(ctx,0x00,0x00);
	csot_dcs_write_seq_static(ctx,0xE1,0x05,0x06,0x09,0x10,0x59,0x1A,0x22,0x28,0x32,0x4B,0x3A,0x41,0x46,0x4B,0x8A,0x50,0x58,0x5F,0x66,0x29,0x6D,0x74,0x7B,0x84,0x5D,0x8E,0x94,0x9A,0xA1,0x47,0xAA,0xB5,0xC3,0xCD,0xFD,0xD9,0xEA,0xF7,0xFF,0x07);
	csot_dcs_write_seq_static(ctx,0x00,0x30);
	csot_dcs_write_seq_static(ctx,0xE1,0x05,0x06,0x09,0x10,0x59,0x1A,0x22,0x28,0x32,0x4B,0x3A,0x41,0x46,0x4B,0x8A,0x50,0x58,0x5F,0x66,0x29,0x6D,0x74,0x7B,0x84,0x5D,0x8E,0x94,0x9A,0xA1,0x47,0xAA,0xB5,0xC3,0xCD,0xFD,0xD9,0xEA,0xF7,0xFF,0x07);
	csot_dcs_write_seq_static(ctx,0x00,0x60);
	csot_dcs_write_seq_static(ctx,0xE1,0x05,0x06,0x09,0x10,0x59,0x1A,0x22,0x28,0x32,0x4B,0x3A,0x41,0x46,0x4B,0x8A,0x50,0x58,0x5F,0x66,0x29,0x6D,0x74,0x7B,0x84,0x5D,0x8E,0x94,0x9A,0xA1,0x47,0xAA,0xB5,0xC3,0xCD,0xFD,0xD9,0xEA,0xF7,0xFF,0x07);
	csot_dcs_write_seq_static(ctx,0x00,0x90);
	csot_dcs_write_seq_static(ctx,0xE1,0x05,0x06,0x09,0x10,0x59,0x1A,0x22,0x28,0x32,0x4B,0x3A,0x41,0x46,0x4B,0x8B,0x50,0x58,0x5F,0x66,0x49,0x6D,0x74,0x7B,0x84,0x5D,0x8E,0x94,0x9A,0xA1,0x47,0xAA,0xB5,0xC3,0xCD,0xFD,0xD9,0xEA,0xF7,0xFF,0x07);
	csot_dcs_write_seq_static(ctx,0x00,0xC0);
	csot_dcs_write_seq_static(ctx,0xE1,0x05,0x06,0x09,0x10,0x59,0x1A,0x22,0x28,0x32,0x4B,0x3A,0x41,0x46,0x4B,0x8A,0x50,0x58,0x5F,0x66,0x29,0x6D,0x74,0x7B,0x84,0x5D,0x8E,0x94,0x9A,0xA1,0x47,0xAA,0xB5,0xC3,0xCD,0xFD,0xD9,0xEA,0xF7,0xFF,0x07);
	csot_dcs_write_seq_static(ctx,0x00,0xF0);
	csot_dcs_write_seq_static(ctx,0xE1,0x05,0x06,0x09,0x10,0x59,0x1A,0x22,0x28,0x32,0x4B,0x3A,0x41,0x46,0x4B,0x8A,0x50);
	csot_dcs_write_seq_static(ctx,0x00,0x00);
	csot_dcs_write_seq_static(ctx,0xE2,0x58,0x5F,0x66,0x29,0x6D,0x74,0x7B,0x84,0x5D,0x8E,0x94,0x9A,0xA1,0x47,0xAA,0xB5,0xC3,0xCD,0xFD,0xD9,0xEA,0xF7,0xFF,0x07);
	csot_dcs_write_seq_static(ctx,0x00,0x82);
	csot_dcs_write_seq_static(ctx,0xF5,0x01);
	csot_dcs_write_seq_static(ctx,0x00,0x93);
	csot_dcs_write_seq_static(ctx,0xF5,0x01);
	csot_dcs_write_seq_static(ctx,0x00,0x9B);
	csot_dcs_write_seq_static(ctx,0xF5,0x49);
	csot_dcs_write_seq_static(ctx,0x00,0x9D);
	csot_dcs_write_seq_static(ctx,0xF5,0x49);
	csot_dcs_write_seq_static(ctx,0x00,0xBE);
	csot_dcs_write_seq_static(ctx,0xC5,0xF0,0xF0);
	csot_dcs_write_seq_static(ctx,0x00,0xE8);
	csot_dcs_write_seq_static(ctx,0xC0,0x40);
	csot_dcs_write_seq_static(ctx,0x00,0x80);
	csot_dcs_write_seq_static(ctx,0xA7,0x03);
	csot_dcs_write_seq_static(ctx,0x00,0xCC);
	csot_dcs_write_seq_static(ctx,0xC0,0x13);
	csot_dcs_write_seq_static(ctx,0x00,0xE0);
	csot_dcs_write_seq_static(ctx,0xCF,0x34);
	csot_dcs_write_seq_static(ctx,0x00,0x92);//VGH clamp off
	csot_dcs_write_seq_static(ctx,0xC5,0x00);
	csot_dcs_write_seq_static(ctx,0x00,0xA0);//VGH pump stop with charge phase @TP term
	csot_dcs_write_seq_static(ctx,0xC5,0x40);
	csot_dcs_write_seq_static(ctx,0x00,0x98);//VGH pump stop with charge phase @TP term
	csot_dcs_write_seq_static(ctx,0xC5,0x24);
	csot_dcs_write_seq_static(ctx,0x00,0xA1);//VGL pump stop with pull phase @TP term
	csot_dcs_write_seq_static(ctx,0xC5,0x40);
	csot_dcs_write_seq_static(ctx,0x00,0x9D);//VGL pump line rate 1/4H
	csot_dcs_write_seq_static(ctx,0xC5,0x44);
	csot_dcs_write_seq_static(ctx,0x00,0x94);//VGH pump line rate 1/2H
	csot_dcs_write_seq_static(ctx,0xC5,0x04);
	csot_dcs_write_seq_static(ctx,0x00,0x86);
	csot_dcs_write_seq_static(ctx,0xA4,0xF1);
	csot_dcs_write_seq_static(ctx,0x00,0x82);
	csot_dcs_write_seq_static(ctx,0xCE,0x2F,0x2F);
	csot_dcs_write_seq_static(ctx,0x00,0x90);
	csot_dcs_write_seq_static(ctx,0xA7,0x1d);
	csot_dcs_write_seq_static(ctx,0x00,0xB0);
	csot_dcs_write_seq_static(ctx,0xB4,0x00,0x08,0x02,0x00,0x00,0xbb,0x00,0x07,0x0d,0xb7,0x0c,0xb7,0x10,0xf0);
	csot_dcs_write_seq_static(ctx,0x00,0x00);
	csot_dcs_write_seq_static(ctx,0xFF,0xFF,0xFF,0xFF);
	csot_dcs_write_seq_static(ctx,0x35,0x00);
	csot_dcs_write_seq_static(ctx, 0x51, 0xC3,0x09);
	csot_dcs_write_seq_static(ctx, 0x53, 0x2C);
	csot_dcs_write_seq_static(ctx, 0x55, 0x00);
	csot_dcs_write_seq_static(ctx, 0x11, 0x00);
	usleep_range(98000,98001);
	csot_dcs_write_seq_static(ctx, 0x29, 0x00);
	usleep_range(2000,2001);

}

static int csot_disable(struct drm_panel *panel)
{
	struct csot *ctx = panel_to_csot(panel);

	pr_info("%s\n", __func__);

	if (!ctx->enabled)
		return 0;

	if (ctx->backlight) {
		ctx->backlight->props.power = FB_BLANK_POWERDOWN;
		backlight_update_status(ctx->backlight);
	}

	ctx->enabled = false;

	return 0;
}

static int csot_unprepare(struct drm_panel *panel)
{
	struct csot *ctx = panel_to_csot(panel);
	int ret=0;

	if (!ctx->prepared) {
		pr_info("%s, already unprepared, return\n", __func__);
		return 0;
	}
	pr_info("%s\n", __func__);

	csot_dcs_write_seq_static(ctx, 0x28);
	msleep(20);
	csot_dcs_write_seq_static(ctx, 0x10);
	msleep(120);

	csot_dcs_write_seq_static(ctx, 0x00, 0x00);//cmd2 enable
	csot_dcs_write_seq_static(ctx, 0xFF, 0x87, 0x20, 0x01);
	csot_dcs_write_seq_static(ctx, 0x00, 0x80);
	csot_dcs_write_seq_static(ctx, 0xFF, 0x87, 0x20);

	csot_dcs_write_seq_static(ctx, 0x00, 0x00);
	csot_dcs_write_seq_static(ctx, 0xF7, 0x5A, 0xA5, 0x95, 0x27);

	csot_dcs_write_seq_static(ctx, 0x00, 0x00);//cmd2 disable
	csot_dcs_write_seq_static(ctx, 0xFF, 0x00, 0x00, 0x00);
	csot_dcs_write_seq_static(ctx, 0x00, 0x80);
	csot_dcs_write_seq_static(ctx, 0xFF, 0x00, 0x00);

	ctx->reset_gpio = devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio)) {
		dev_err(ctx->dev, "%s: cannot get reset_gpio %ld\n",
			__func__, PTR_ERR(ctx->reset_gpio));
		//return -1;
	}
	else {
		gpiod_set_value(ctx->reset_gpio, 1);
		msleep(5);
		devm_gpiod_put(ctx->dev, ctx->reset_gpio);
	}

	ctx->prepared = false;

#ifdef BIAS_SM5109
	sm5109_BiasPower_disable(5);
#endif

#ifdef BIAS_TMP_V0
	ctx->bias_n_gpio = devm_gpiod_get(ctx->dev, "bias_n", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->bias_n_gpio)) {
		dev_err(ctx->dev, "%s: cannot get bias_n_gpio %ld\n",
			__func__, PTR_ERR(ctx->bias_n_gpio));
		ret = -1;
	}

	ctx->bias_p_gpio = devm_gpiod_get(ctx->dev, "bias_p", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->bias_p_gpio)) {
		dev_err(ctx->dev, "%s: cannot get bias_p_gpio %ld\n",
			__func__, PTR_ERR(ctx->bias_p_gpio));
		ret = -1;
	}
	if (!ret) {
		gpiod_set_value(ctx->bias_n_gpio, 0);
		udelay(10 * 1000);
		gpiod_set_value(ctx->bias_p_gpio, 0);
		udelay(10 * 1000);
		devm_gpiod_put(ctx->dev, ctx->bias_n_gpio);
		devm_gpiod_put(ctx->dev, ctx->bias_p_gpio);
		printk("[%d  %s]hxl_check_bias !!\n",__LINE__, __FUNCTION__);
	}
#endif

	ctx->error = 0;
	return 0;
}

static int csot_prepare(struct drm_panel *panel)
{
	struct csot *ctx = panel_to_csot(panel);
	int ret;

	pr_info("%s\n", __func__);
	if (ctx->prepared) {
		pr_info("%s, already prepared, return\n", __func__);
		return 0;
	}

	gpiod_set_value(ctx->reset_gpio, 0);
	msleep(3);

	//lcm_power_enable();
	csot_panel_init(ctx);

	ret = ctx->error;
	if (ret < 0) {
		pr_info("disp: %s error ret=%d\n", __func__, ret);
		csot_unprepare(panel);
	}

	ctx->prepared = true;
/*#if defined(CONFIG_MTK_PANEL_EXT)
	mtk_panel_tch_rst(panel);
#endif

#ifdef PANEL_SUPPORT_READBACK
	csot_panel_get_data(ctx);
#endif*/

	return ret;
}

static int csot_enable(struct drm_panel *panel)
{
	struct csot *ctx = panel_to_csot(panel);


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
	.clock = 360586,
	.hdisplay = FRAME_WIDTH,
	.hsync_start = FRAME_WIDTH + MODE_0_HFP,
	.hsync_end = FRAME_WIDTH + MODE_0_HFP + HSA,
	.htotal = FRAME_WIDTH + MODE_0_HFP + HSA + HBP,
	.vdisplay = FRAME_HEIGHT,
	.vsync_start = FRAME_HEIGHT + MODE_0_VFP,
	.vsync_end = FRAME_HEIGHT + MODE_0_VFP + VSA,
	.vtotal = FRAME_HEIGHT + MODE_0_VFP + VSA + VBP,
};
static const struct drm_display_mode performance_mode_1 = {
	.clock = 360586,
	.hdisplay = FRAME_WIDTH,
	.hsync_start = FRAME_WIDTH + MODE_1_HFP,
	.hsync_end = FRAME_WIDTH + MODE_1_HFP + HSA,
	.htotal = FRAME_WIDTH + MODE_1_HFP + HSA + HBP,
	.vdisplay = FRAME_HEIGHT,
	.vsync_start = FRAME_HEIGHT + MODE_1_VFP,
	.vsync_end = FRAME_HEIGHT + MODE_1_VFP + VSA,
	.vtotal = FRAME_HEIGHT + MODE_1_VFP + VSA + VBP,
};

static const struct drm_display_mode performance_mode_2 = {
	.clock = 360586,
	.hdisplay = FRAME_WIDTH,
	.hsync_start = FRAME_WIDTH + MODE_2_HFP,
	.hsync_end = FRAME_WIDTH + MODE_2_HFP + HSA,
	.htotal = FRAME_WIDTH + MODE_2_HFP + HSA + HBP,
	.vdisplay = FRAME_HEIGHT,
	.vsync_start = FRAME_HEIGHT + MODE_2_VFP,
	.vsync_end = FRAME_HEIGHT + MODE_2_VFP + VSA,
	.vtotal = FRAME_HEIGHT + MODE_2_VFP + VSA + VBP,
};

#if defined(CONFIG_MTK_PANEL_EXT)
static struct mtk_panel_params ext_params = {
	//.pll_clk = 300,
	.data_rate = 1028,
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
	.physical_width_um = PHYSICAL_WIDTH,
	.physical_height_um = PHYSICAL_HEIGHT,
	.lcm_index = 2,

	.output_mode = MTK_PANEL_DSC_SINGLE_PORT,
	.dsc_params = {
		.enable                =  DSC_ENABLE,
		.ver                   =  DSC_VER,
		.slice_mode            =  DSC_SLICE_MODE,
		.rgb_swap              =  DSC_RGB_SWAP,
		.dsc_cfg               =  DSC_DSC_CFG,
		.rct_on                =  DSC_RCT_ON,
		.bit_per_channel       =  DSC_BIT_PER_CHANNEL,
		.dsc_line_buf_depth    =  DSC_DSC_LINE_BUF_DEPTH,
		.bp_enable             =  DSC_BP_ENABLE,
		.bit_per_pixel         =  DSC_BIT_PER_PIXEL,
		.pic_height            =  FRAME_HEIGHT,
		.pic_width             =  FRAME_WIDTH,
		.slice_height          =  DSC_SLICE_HEIGHT,
		.slice_width           =  DSC_SLICE_WIDTH,
		.chunk_size            =  DSC_CHUNK_SIZE,
		.xmit_delay            =  DSC_XMIT_DELAY,
		.dec_delay             =  DSC_DEC_DELAY,
		.scale_value           =  DSC_SCALE_VALUE,
		.increment_interval    =  DSC_INCREMENT_INTERVAL,
		.decrement_interval    =  DSC_DECREMENT_INTERVAL,
		.line_bpg_offset       =  DSC_LINE_BPG_OFFSET,
		.nfl_bpg_offset        =  DSC_NFL_BPG_OFFSET,
		.slice_bpg_offset      =  DSC_SLICE_BPG_OFFSET,
		.initial_offset        =  DSC_INITIAL_OFFSET,
		.final_offset          =  DSC_FINAL_OFFSET,
		.flatness_minqp        =  DSC_FLATNESS_MINQP,
		.flatness_maxqp        =  DSC_FLATNESS_MAXQP,
		.rc_model_size         =  DSC_RC_MODEL_SIZE,
		.rc_edge_factor        =  DSC_RC_EDGE_FACTOR,
		.rc_quant_incr_limit0  =  DSC_RC_QUANT_INCR_LIMIT0,
		.rc_quant_incr_limit1  =  DSC_RC_QUANT_INCR_LIMIT1,
		.rc_tgt_offset_hi      =  DSC_RC_TGT_OFFSET_HI,
		.rc_tgt_offset_lo      =  DSC_RC_TGT_OFFSET_LO,
	},
	.lfr_enable = 1,
	.lfr_minimum_fps = 60,
	.max_bl_level = 2047,
	.hbm_type = HBM_MODE_DCS_I2C,

};

static struct mtk_panel_params ext_params_mode_1 = {
//	.vfp_low_power = 1302,//60hz
	.data_rate = 1028,
	.cust_esd_check = 0,
	.esd_check_enable = 0,
	.lcm_esd_check_table[0] = {
		.cmd = 0x0a,
		.count = 1,
		.para_list[0] = 0x9c,
	},
	.lane_swap_en = 0,
	.lp_perline_en = 0,
	.physical_width_um = PHYSICAL_WIDTH,
	.physical_height_um = PHYSICAL_HEIGHT,
	.lcm_index = 2,

	.output_mode = MTK_PANEL_DSC_SINGLE_PORT,
	.dsc_params = {
		.enable                =  DSC_ENABLE,
		.ver                   =  DSC_VER,
		.slice_mode            =  DSC_SLICE_MODE,
		.rgb_swap              =  DSC_RGB_SWAP,
		.dsc_cfg               =  DSC_DSC_CFG,
		.rct_on                =  DSC_RCT_ON,
		.bit_per_channel       =  DSC_BIT_PER_CHANNEL,
		.dsc_line_buf_depth    =  DSC_DSC_LINE_BUF_DEPTH,
		.bp_enable             =  DSC_BP_ENABLE,
		.bit_per_pixel         =  DSC_BIT_PER_PIXEL,
		.pic_height            =  FRAME_HEIGHT,
		.pic_width             =  FRAME_WIDTH,
		.slice_height          =  DSC_SLICE_HEIGHT,
		.slice_width           =  DSC_SLICE_WIDTH,
		.chunk_size            =  DSC_CHUNK_SIZE,
		.xmit_delay            =  DSC_XMIT_DELAY,
		.dec_delay             =  DSC_DEC_DELAY,
		.scale_value           =  DSC_SCALE_VALUE,
		.increment_interval    =  DSC_INCREMENT_INTERVAL,
		.decrement_interval    =  DSC_DECREMENT_INTERVAL,
		.line_bpg_offset       =  DSC_LINE_BPG_OFFSET,
		.nfl_bpg_offset        =  DSC_NFL_BPG_OFFSET,
		.slice_bpg_offset      =  DSC_SLICE_BPG_OFFSET,
		.initial_offset        =  DSC_INITIAL_OFFSET,
		.final_offset          =  DSC_FINAL_OFFSET,
		.flatness_minqp        =  DSC_FLATNESS_MINQP,
		.flatness_maxqp        =  DSC_FLATNESS_MAXQP,
		.rc_model_size         =  DSC_RC_MODEL_SIZE,
		.rc_edge_factor        =  DSC_RC_EDGE_FACTOR,
		.rc_quant_incr_limit0  =  DSC_RC_QUANT_INCR_LIMIT0,
		.rc_quant_incr_limit1  =  DSC_RC_QUANT_INCR_LIMIT1,
		.rc_tgt_offset_hi      =  DSC_RC_TGT_OFFSET_HI,
		.rc_tgt_offset_lo      =  DSC_RC_TGT_OFFSET_LO,
	},
	.lfr_enable = 1,
	.lfr_minimum_fps = 60,
	.max_bl_level = 2047,
	.hbm_type = HBM_MODE_DCS_I2C,
};

static struct mtk_panel_params ext_params_mode_2 = {
//	.vfp_low_power = 2558,//60hz
	.data_rate = 1028,
	.cust_esd_check = 0,
	.esd_check_enable = 0,
	.lcm_esd_check_table[0] = {
		.cmd = 0x0a,
		.count = 1,
		.para_list[0] = 0x9c,
	},
	.lane_swap_en = 0,
	.lp_perline_en = 0,
	.physical_width_um = PHYSICAL_WIDTH,
	.physical_height_um = PHYSICAL_HEIGHT,
	.lcm_index = 2,

	.output_mode = MTK_PANEL_DSC_SINGLE_PORT,
	.dsc_params = {
		.enable                =  DSC_ENABLE,
		.ver                   =  DSC_VER,
		.slice_mode            =  DSC_SLICE_MODE,
		.rgb_swap              =  DSC_RGB_SWAP,
		.dsc_cfg               =  DSC_DSC_CFG,
		.rct_on                =  DSC_RCT_ON,
		.bit_per_channel       =  DSC_BIT_PER_CHANNEL,
		.dsc_line_buf_depth    =  DSC_DSC_LINE_BUF_DEPTH,
		.bp_enable             =  DSC_BP_ENABLE,
		.bit_per_pixel         =  DSC_BIT_PER_PIXEL,
		.pic_height            =  FRAME_HEIGHT,
		.pic_width             =  FRAME_WIDTH,
		.slice_height          =  DSC_SLICE_HEIGHT,
		.slice_width           =  DSC_SLICE_WIDTH,
		.chunk_size            =  DSC_CHUNK_SIZE,
		.xmit_delay            =  DSC_XMIT_DELAY,
		.dec_delay             =  DSC_DEC_DELAY,
		.scale_value           =  DSC_SCALE_VALUE,
		.increment_interval    =  DSC_INCREMENT_INTERVAL,
		.decrement_interval    =  DSC_DECREMENT_INTERVAL,
		.line_bpg_offset       =  DSC_LINE_BPG_OFFSET,
		.nfl_bpg_offset        =  DSC_NFL_BPG_OFFSET,
		.slice_bpg_offset      =  DSC_SLICE_BPG_OFFSET,
		.initial_offset        =  DSC_INITIAL_OFFSET,
		.final_offset          =  DSC_FINAL_OFFSET,
		.flatness_minqp        =  DSC_FLATNESS_MINQP,
		.flatness_maxqp        =  DSC_FLATNESS_MAXQP,
		.rc_model_size         =  DSC_RC_MODEL_SIZE,
		.rc_edge_factor        =  DSC_RC_EDGE_FACTOR,
		.rc_quant_incr_limit0  =  DSC_RC_QUANT_INCR_LIMIT0,
		.rc_quant_incr_limit1  =  DSC_RC_QUANT_INCR_LIMIT1,
		.rc_tgt_offset_hi      =  DSC_RC_TGT_OFFSET_HI,
		.rc_tgt_offset_lo      =  DSC_RC_TGT_OFFSET_LO,
	},
	.lfr_enable = 1,
	.lfr_minimum_fps = 60,
	.max_bl_level = 2047,
	.hbm_type = HBM_MODE_DCS_I2C,
};
/*
static int csot_setbacklight_cmdq(void *dsi, dcs_write_gce cb,
	void *handle, unsigned int level)
{
	pr_info("%s backlight = -%d\n", __func__, level);
	bl_tb0[1] = (u8)level;

	if (!cb)
		return -1;

	cb(dsi, handle, bl_tb0, ARRAY_SIZE(bl_tb0));
	return 0;
}
*/
struct drm_display_mode *get_mode_by_id(struct drm_connector *connector,
	unsigned int mode)
{
	struct drm_display_mode *m;
	unsigned int i = 0;

	list_for_each_entry(m, &connector->modes, head) {
		if (i == mode)
			return m;
		i++;
	}
	return NULL;
}

static int mtk_panel_ext_param_set(struct drm_panel *panel,
			 struct drm_connector *connector, unsigned int mode)
{
	struct mtk_panel_ext *ext = find_panel_ext(panel);
	int ret = 0;
	struct drm_display_mode *m = get_mode_by_id(connector, mode);
	int flag=0;

	flag=drm_mode_vrefresh(m);

	if (!m)
		return ret;

	if (drm_mode_vrefresh(m) == MODE_0_FPS)
		ext->params = &ext_params;
	else if (drm_mode_vrefresh(m) == MODE_1_FPS)
		ext->params = &ext_params_mode_1;
	else if (drm_mode_vrefresh(m) == MODE_2_FPS)
		ext->params = &ext_params_mode_2;
	else
		ret = 1;
	return ret;
}
static int panel_ext_reset(struct drm_panel *panel, int on)
{
	struct csot *ctx = panel_to_csot(panel);

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

/*
static int pane_cabc_set_cmdq(struct csot *ctx, void *dsi, dcs_grp_write_gce cb, void *handle, uint32_t hbm_state)
{
	struct mtk_panel_para_table cabc_ui_table = {2, {0x55, 0x01}};
	struct mtk_panel_para_table cabc_mv_table = {2, {0x55, 0x03}};
	struct mtk_panel_para_table cabc_off_table = {2, {0x55, 0x00}};

	if (hbm_state > 3) return -1;
	switch (hbm_state)
	{
		case 0:
			cb(dsi, handle, &cabc_ui_table, 1);
			break;
		case 1:
			cb(dsi, handle, &cabc_mv_table, 1);
			break;
		case 2:
			cb(dsi, handle, &cabc_off_table, 1);
			break;
		default:
			break;
	}

	return 0;
}

static int pane_hbm_set_cmdq(struct csot *ctx, void *dsi, dcs_grp_write_gce cb, void *handle, uint32_t hbm_state)
{
	struct mtk_panel_para_table hbm_on_table = {3, {0x51, 0xF9, 0x06}};
	struct mtk_panel_para_table hbm_off_table = {3, {0x51, 0xC3, 0x09}};

	if (hbm_state > 2) return -1;
	switch (hbm_state)
	{
		case 0:
			cb(dsi, handle, &hbm_off_table, 1);
			break;
		case 1:
			cb(dsi, handle, &hbm_on_table, 1);
			break;
		case 2:
			break;
		default:
			break;
	}

	return 0;
}
static int panel_feature_set(struct drm_panel *panel, void *dsi,
			      dcs_grp_write_gce cb, void *handle, struct panel_param_info param_info)
{

	struct csot *ctx = panel_to_csot(panel);
	int ret = 0;

	if ((!cb) || (!ctx->enabled)) {
		ret = -1;
	} else {
		pr_info("%s: set feature %d to %d\n", __func__, param_info.param_idx, param_info.value);

		switch (param_info.param_idx) {
			case PARAM_CABC:
				if (ctx->cabc_mode == param_info.value) return -1;
				ctx->cabc_mode = param_info.value;
				pane_cabc_set_cmdq(ctx, dsi, cb, handle, param_info.value);
				break;
			case PARAM_HBM:
				if (ctx->hbm_mode == param_info.value) return -1;
				ctx->hbm_mode = param_info.value;
				pane_hbm_set_cmdq(ctx, dsi, cb, handle, param_info.value);
				break;
			default:
				ret = -1;
				break;
		}
		pr_info("%s: set feature %d to %d, ret %d\n", __func__, param_info.param_idx, param_info.value, ret);
	}
	return ret;
}
*/

static struct mtk_panel_funcs ext_funcs = {
//	.set_backlight_cmdq = csot_setbacklight_cmdq,
	.reset = panel_ext_reset,
	.ext_param_set = mtk_panel_ext_param_set,
//	.ata_check = panel_ata_check,
	.get_lcm_version = mtk_panel_get_lcm_version,
	//.panel_feature_set = panel_feature_set,
};
#endif

static int csot_get_modes(struct drm_panel *panel,
						struct drm_connector *connector)
{
	struct drm_display_mode *mode;
	struct drm_display_mode *mode_1;
	struct drm_display_mode *mode_2;

	mode = drm_mode_duplicate(connector->dev, &default_mode);
	if (!mode) {
		dev_err(connector->dev->dev, "failed to add mode %ux%ux@%u\n",
			default_mode.hdisplay, default_mode.vdisplay,
			drm_mode_vrefresh(&default_mode));
		return -ENOMEM;
	}
	drm_mode_set_name(mode);
	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	drm_mode_probed_add(connector, mode);

	mode_1 = drm_mode_duplicate(connector->dev, &performance_mode_1);
	if (!mode_1) {
		dev_err(connector->dev->dev, "failed to add mode %ux%ux@%u\n",
			performance_mode_1.hdisplay,
			performance_mode_1.vdisplay,
			drm_mode_vrefresh(&performance_mode_1));
		return -ENOMEM;
	}
	drm_mode_set_name(mode_1);
	mode_1->type = DRM_MODE_TYPE_DRIVER;
	drm_mode_probed_add(connector, mode_1);


	mode_2 = drm_mode_duplicate(connector->dev, &performance_mode_2);
	if (!mode_2) {
		dev_err(connector->dev->dev, "failed to add mode %ux%ux@%u\n",
			performance_mode_2.hdisplay,
			performance_mode_2.vdisplay,
			drm_mode_vrefresh(&performance_mode_2));
		return -ENOMEM;
	}
	drm_mode_set_name(mode_2);
	mode_2->type = DRM_MODE_TYPE_DRIVER;
	drm_mode_probed_add(connector, mode_2);
	connector->display_info.width_mm = 68;
	connector->display_info.height_mm = 150;

	return 1;
}

static const struct drm_panel_funcs csot_drm_funcs = {
	.disable = csot_disable,
	.unprepare = csot_unprepare,
	.prepare = csot_prepare,
	.enable = csot_enable,
	.get_modes = csot_get_modes,
};

static int csot_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct device_node *dsi_node, *remote_node = NULL, *endpoint = NULL;
	struct csot *ctx;
	struct device_node *backlight;
	int ret;

	pr_info("%s+ csot_ft8726,vdo,120hz\n", __func__);

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

	ctx = devm_kzalloc(dev, sizeof(struct csot), GFP_KERNEL);
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

	drm_panel_init(&ctx->panel, dev, &csot_drm_funcs, DRM_MODE_CONNECTOR_DSI);

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

	pr_info("[%d  %s]- csot_ft8726,vdo,120hz ret:%d  \n", __LINE__, __func__,ret);

	return ret;
}

static int csot_remove(struct mipi_dsi_device *dsi)
{
	struct csot *ctx = mipi_dsi_get_drvdata(dsi);
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

static const struct of_device_id csot_of_match[] = {
	{
		.compatible = "csot_ft8726,vid,120hz",
	},
	{} };

MODULE_DEVICE_TABLE(of, csot_of_match);

static struct mipi_dsi_driver csot_driver = {
	.probe = csot_probe,
	.remove = csot_remove,
	.driver = {
		.name = "csot_ft8726_vid_649_1080_120hz",
		.owner = THIS_MODULE,
		.of_match_table = csot_of_match,
	},
};

module_mipi_dsi_driver(csot_driver);

MODULE_AUTHOR("mediatek");
MODULE_DESCRIPTION("csot ft8726 incell 120hz Panel Driver");
MODULE_LICENSE("GPL v2");

