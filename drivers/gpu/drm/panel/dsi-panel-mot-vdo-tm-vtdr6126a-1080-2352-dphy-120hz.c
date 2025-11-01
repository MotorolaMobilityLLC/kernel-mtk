// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2025 Motorola Mobility LLC.
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
#include "include/dsi-panel-mot-vdo-tm-vtdr6126a-1080-2352-dphy-120hz.h"
#endif

#define PANEL_LDO_VTP_EN

#ifdef PANEL_LDO_VTP_EN
#include <linux/gpio/consumer.h>
#include <linux/regulator/consumer.h>
#endif

/* option function to read data from some panel address */
/* #define PANEL_SUPPORT_READBACK */

unsigned int vtdr6126a_rc_buf_thresh[14] = {896, 1792, 2688, 3584, 4480, 5376,
        6272, 6720, 7168, 7616, 7744, 7872, 8000, 8064};
unsigned int vtdr6126a_range_min_qp[15] = {0, 4, 5, 5, 7, 7, 7, 7, 7, 7, 9, 9, 9, 13, 16};
unsigned int vtdr6126a_range_max_qp[15] = {8, 8, 9, 10, 11, 11, 11, 12, 13, 14, 14, 15,
        15, 16, 17};
int vtdr6126a_range_bpg_ofs[15] = {2, 0, 0, -2, -4, -6, -8, -8, -8, -10, -10, -12, -12,
        -12, -12};

//TM EVT panel v0 support. to be disabled before PVT
//#define TM_PANEL_EVT_V0_SUPPORT		1

//panel id, reg 0xF1, value 02 05 5a 51
#define TM_PANEL_VENDOR_ID    0x515a0502
//#define TM_PANEL_VENDOR_ID  	(TM_PANEL_VENDOR_ID | (0xF << 24))
#define FOD_CENTER_X 636
#define FOD_CENTER_Y 2525
#define PANEL_EVT 1
#define PANEL_DVT1 2
#define PANEL_DVT2 3
#define PANEL_PVT 4
static int current_bl = 0;
static int panel_version = 1;

struct tm_vtdr6126a {
	struct device *dev;
	struct drm_panel panel;
	struct backlight_device *backlight;
	struct gpio_desc *reset_gpio;
	struct gpio_desc *vddi_en_gpio;

	struct regulator *dvdd_supply;
	struct regulator *vci_supply;

	bool prepared;
	bool enabled;

	int error;
	//enum panel_version version;
	bool lhbm_en;
	atomic_t hbm_mode;
	atomic_t current_fps;
	atomic_t dc_mode;
	int version;
};

static struct mtk_panel_para_table panel_lhbm_on[] = {
      //set LHBM on
      {3, {0x51, 0x36, 0xE0}},
      {5, {0x63, 0x10, 0x00, 0x0d, 0xc0}},
      {2, {0x62, 0x03}},
};

//set enable lhbm code, off status
static struct mtk_panel_para_table panel_lhbm_off[] = {
	//set LHBM off
      {2, {0x62,0x00}},
};

static struct mtk_panel_para_table panel_dc_on[] = {
	{2, {0x5E,0x01}},
};

static struct mtk_panel_para_table panel_dc_off[] = {
	{2, {0x5E,0x00}},
};

#define tm_vtdr6126a_dcs_write_seq(ctx, seq...)                                     \
	({                                                                     \
		const u8 d[] = {seq};                                          \
		BUILD_BUG_ON_MSG(ARRAY_SIZE(d) > 64,                           \
				 "DCS sequence too big for stack");            \
		tm_vtdr6126a_dcs_write(ctx, d, ARRAY_SIZE(d));                      \
	})

#define tm_vtdr6126a_dcs_write_seq_static(ctx, seq...)                              \
	({                                                                     \
		static const u8 d[] = {seq};                                   \
		tm_vtdr6126a_dcs_write(ctx, d, ARRAY_SIZE(d));                      \
	})

static inline struct tm_vtdr6126a *panel_to_tm_vtdr6126a(struct drm_panel *panel)
{
	return container_of(panel, struct tm_vtdr6126a, panel);
}

#ifdef PANEL_SUPPORT_READBACK
static int tm_vtdr6126a_dcs_read(struct tm_vtdr6126a *ctx, u8 cmd, void *data, size_t len)
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

static void tm_vtdr6126a_panel_get_data(struct tm_vtdr6126a *ctx)
{
	u8 buffer[3] = {0};
	static int ret;

	if (ret == 0) {
		ret = tm_vtdr6126a_dcs_read(ctx, 0x0A, buffer, 1);
		pr_info("disp: %s 0x%08x\n", __func__, buffer[0] | (buffer[1] << 8));
		dev_info(ctx->dev, "return %d data(0x%08x) to dsi engine\n",
			 ret, buffer[0] | (buffer[1] << 8));
	}
}
#endif

static void tm_vtdr6126a_dcs_write(struct tm_vtdr6126a *ctx, const void *data, size_t len)
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


static void tm_vtdr6126a_panel_init(struct tm_vtdr6126a *ctx)
{
	pr_info("disp: %s+\n", __func__);

	ctx->reset_gpio = devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio)) {
		dev_err(ctx->dev, "%s: cannot get reset_gpio %ld\n",
			__func__, PTR_ERR(ctx->reset_gpio));
		//return;
	}
	else {
		gpiod_set_value(ctx->reset_gpio, 1);
		msleep(15);
		gpiod_set_value(ctx->reset_gpio, 0);
		msleep(15);
		gpiod_set_value(ctx->reset_gpio, 1);
		msleep(25);
		devm_gpiod_put(ctx->dev, ctx->reset_gpio);
		pr_info("disp: %s reset_gpio\n", __func__);
	}

	//Scaling_up
	//==============CM1===================//
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0x03, 0x01);

	tm_vtdr6126a_dcs_write_seq_static(ctx, 0x35, 0x00);
	//dimming on
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0x53, 0x28);
	//DBV DIMING
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xF0, 0xAA, 0x13);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xD0, 0x0F);
if(ctx->version >= PANEL_DVT2) {//dvt2 or later
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0x55, 0x10);
}
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0x59, 0x09);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0x5E, 0x00);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0x6B, 0x01);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0x6C, 0x01);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0x6D, 0x00);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0x6F, 0x01);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0x72, 0x00);
if(ctx->version <= PANEL_DVT1) {//evt and dvt1
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xA4, 0x01);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0x38, 0);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0x6F, 0x01);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xA4, 0x00);
}
	//Vesa1.2,SliceNumber:2,Slice,Height:12H,10bit
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0x70, 0x12, 0x00, 0x00, 0xAB, 0x30, 0x80, 0x09, 0x30, 0x04, 0x38, 0x00, 0x0C, 0x02, 0x1C, 0x02, 0x1C, 0x02, 0x00, 0x01, 0x17, 0x00, 0x20, 0x02, 0x1A, 0x00, 0x07, 0x00, 0x01, 0x00, 0xBB, 0x08, 0x7A, 0x18, 0x00, 0x10, 0xF0, 0x07, 0x10, 0x20, 0x00, 0x06, 0x0F, 0x0F, 0x33, 0x0E, 0x1C, 0x2A, 0x38, 0x46, 0x54, 0x62, 0x69, 0x70, 0x77, 0x79, 0x7B, 0x7D, 0x7E, 0x02, 0x02, 0x22, 0x00, 0x2A, 0x40, 0x2A, 0xBE, 0x3A, 0xFC, 0x3A, 0xFA, 0x3A, 0xF8, 0x3B, 0x38, 0x3B, 0x78, 0x3B, 0xB6, 0x4B, 0xB6, 0x4B, 0xF4, 0x4B, 0xF4, 0x6C, 0x34, 0x84, 0x74, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
        //SCL
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xF0, 0xAA, 0x10);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xB0, 0x05, 0x6A, 0x01, 0x3E, 0x00, 0x04, 0x38, 0x04, 0x98);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xBA, 0x07, 0x0E, 0x23, 0x0B, 0x82, 0x0D, 0x96, 0x0D, 0x94);
	//T1A
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xF0, 0xAA, 0x10);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xB1, 0x01, 0x6B, 0x00, 0x18, 0x00, 0x54, 0x00, 0x01, 0xB3, 0x00, 0x18, 0x00, 0x54, 0x00, 0x01, 0xB3, 0x00, 0x18, 0x04, 0x14, 0x00, 0x01, 0xB3, 0x00, 0x18, 0x0B, 0x94, 0x00, 0x00, 0x14, 0x00, 0x4C, 0x00, 0x14, 0x00, 0x4C, 0x00, 0x14, 0x03, 0x7C, 0x00, 0x14, 0x09, 0xDC);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xB2, 0x01, 0xB3, 0x00, 0x18, 0x00, 0x54, 0x03, 0x01, 0xB3, 0x00, 0x18, 0x00, 0x54, 0x03, 0x01, 0xB3, 0x00, 0x18, 0x00, 0x54, 0x03, 0x00, 0x14, 0x00, 0x4C, 0x00, 0x14, 0x00, 0x4C, 0x00, 0x14, 0x00, 0x4C);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xF0, 0xAA, 0x14);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xCC, 0x00, 0x11, 0x00, 0x04, 0x00, 0x11, 0x00, 0x10, 0x00, 0x11, 0x00, 0x10, 0x00, 0x11, 0x00, 0x10, 0x00, 0x11, 0x00, 0x10, 0x00, 0x11, 0x00, 0x10, 0x00, 0x11, 0x00, 0x10);
	//OTK4
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xF0, 0xAA, 0x14);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xC1, 0x04, 0x02, 0x21, 0x12);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xCA, 0x11, 0x01, 0x05, 0x05);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0x65, 0x04);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xCA, 0x03, 0xBA, 0x04, 0x62, 0x04, 0x62, 0x04, 0x62, 0x02, 0xF2, 0x03, 0x9A, 0x03, 0x9A, 0x03, 0x9A);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0x65, 0x34);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xCA, 0x04, 0x62, 0x04, 0x62, 0x04, 0x62, 0x03, 0x9A, 0x03, 0x9A, 0x03, 0x9A);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0x65, 0x66);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xCA, 0x03, 0xA6, 0x04, 0x4E, 0x04, 0x4E, 0x04, 0x4E, 0x03, 0x06, 0x03, 0xAE, 0x03, 0xAE, 0x03, 0xAE);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0x65, 0x76);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xCA, 0x04, 0x4E, 0x04, 0x4E, 0x04, 0x4E, 0x03, 0xAE, 0x03, 0xAE, 0x03, 0xAE);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0x65, 0x14);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xCA, 0x03, 0x5E, 0x04, 0x06, 0x04, 0x06, 0x04, 0x06, 0x03, 0x4E, 0x03, 0xF6, 0x03, 0xF6, 0x03, 0xF6);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0x65, 0x40);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xCA, 0x04, 0x06, 0x04, 0x06, 0x04, 0x06, 0x03, 0xF6, 0x03, 0xF6, 0x03, 0xF6);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0x65, 0x24);
if(ctx->version <= PANEL_DVT1) {//evt and dvt1
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xCA, 0x00, 0xD9, 0x01, 0x03, 0x01, 0x03, 0x01, 0x03, 0x00, 0xD1, 0x00, 0xFB, 0x00, 0xFB, 0x00, 0xFB);
}
else {
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xCA, 0x00, 0xD6, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0xD4, 0x00, 0xFE, 0x00, 0xFE, 0x00, 0xFE);
}
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0x65, 0x4C);
if(ctx->version <= PANEL_DVT1) {//evt and dvt1
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xCA, 0x01, 0x03, 0x01, 0x03, 0x01, 0x03, 0x00, 0xFB, 0x00, 0xFB, 0x00, 0xFB);
}
else {
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xCA, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0xFE, 0x00, 0xFE, 0x00, 0xFE);
}
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0x65, 0x58);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xCA, 0x01, 0xAB, 0x01, 0xFF, 0x01, 0xFF, 0x01, 0xFF, 0x01, 0xFF, 0x01, 0xFF, 0x01, 0xFF);

	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xF0, 0xAA, 0x15);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0x65, 0x0A);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xB1, 0x02);

	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xF0, 0xAA, 0x10);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xCE, 0x20);
if(ctx->version <= PANEL_DVT1) {//evt and dvt1
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xF0, 0xAA,0x14);
    //CLK01
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xBC, 0x90);
    //CLK02
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xBD, 0x90);
    //EM_CLK01
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xBE, 0x90);

	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xF0, 0xAA,0x15);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xB5, 0x33,0x27,0x41,0x33,0x77,0x11,0x30,0x70,0x10,0x24,0x30,0x00,0x44,0x00,0x00,0x40,0x00,0x00);
}
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xF0, 0xAA,0x16);
    //MTE_VRR_BASE=1
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xD1, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00);

	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xF0, 0xAA,0x18);
    //LHBM FOD settings (Naples)——0902
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xB0, 0x80);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xB6, 0x6A,0x00,0x1F,0x09,0xC1,0x01,0x00,0x4B,0x95,0xC7,0x04,0x00,0x00);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xB7, 0x01,0x02,0x12,0x09,0x73,0x00,0xD4,0xD4);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xB8, 0x01,0x9C,0x32,0x9C,0x06,0x00,0x00,0x9C,0x6A,0x9C,0x07,0x00,0x26,0x9C,0x97,0x9C,0x06,0x00,0xA9,0x1C,0x97,0x1C,0x02,0x00,0xC7,0x9F,0x69,0x9F,0x01,0x00,0x00,0x9F,0x5D,0x9F,0x05,0x00,0x0C,0x1F,0x97,0x1F,0x08,0x00,0x2A,0x2F,0x97,0x2F,0x04,0x00,0xAD);

	tm_vtdr6126a_dcs_write_seq_static(ctx, 0x65, 0x0C);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xB0, 0x04);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xC0, 0xF0,0x20,0x2D,0xB1,0x28,0xAB,0xE1,0xB4,0xDC,0x5F,0x20,0x14,0x28,0x24,0x18);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xC1, 0x42,0x21,0x4C,0xC8,0x64,0x3A,0x1C,0xD0,0x88,0x34,0x9A,0x51,0x26,0xA3,0x50);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xC2, 0x35,0x1A,0x8D,0x46,0xA2,0x59,0x2C,0x96,0x4B,0x35,0x1A,0x8D,0x46,0xA3,0x50);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xC3, 0x25,0x12,0x8D,0x26,0x92,0x49,0x24,0xD0,0x68,0x24,0x19,0xC8,0xE4,0x73,0x30);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xC4, 0x23,0x11,0x88,0xA4,0x52,0x21,0x10,0xC6,0x43,0x21,0x10,0x88,0x24,0x12,0x00);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xC5, 0x20,0x17,0xCB,0xC5,0xE2,0xE9,0xF0,0xB8,0x5B,0x3D,0x16,0x8B,0x25,0x82,0xC0);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xC6, 0x2B,0x95,0x8E,0xA5,0x52,0xA1,0x4C,0xA4,0x52,0x28,0x94,0x09,0xE4,0xE2,0x68);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xC7, 0x26,0x93,0x09,0x64,0xA2,0x49,0x20,0x8E,0x46,0x22,0x91,0x08,0x64,0x22,0x08);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xC8, 0x20,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xC9, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xCA, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xCB, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xE0, 0x52,0x52,0x83,0x94,0x61,0x5A,0x4C,0x36,0x24,0xA0,0x49,0x5A,0x81,0x2C,0xAC);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xE1, 0x33,0x8E,0x03,0xB8,0xCC,0x22,0xC4,0x87,0x15,0x40,0x32,0xC0,0x55,0x38,0x67);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xE2, 0x58,0x06,0x75,0x38,0x24,0x3A,0x58,0xE0,0x8C,0xA6,0x42,0x56,0xC6,0xB9,0xCF);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xE3, 0x7B,0xE1,0x5A,0xD2,0x93,0x8C,0x1D,0x8A,0xCE,0x19,0xB4,0x9D,0x69,0x6E,0xD0);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xE4, 0xC4,0xF5,0x4E,0x55,0xD5,0xE5,0x37,0x3C,0xC6,0xFD,0x9E,0x5F,0x5D,0x42,0x99);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xE5, 0xF4,0xEF,0xB7,0xCE,0xDA,0xEC,0x69,0x6C,0xF3,0xD0,0x9D,0x2D,0x8C,0xEF,0x9D);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xE6, 0xF7,0xBF,0xFF,0x80,0x00,0xFF,0xBB,0xCD,0x00,0x1F,0xE0,0x01,0xFD,0x83,0xE0);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xE7, 0x07,0x80,0x00,0x00,0x00,0xE0,0x00,0x00,0x00,0x00,0x00,0x00,0x0F,0x00,0x1F);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xE8, 0x00,0x00,0x00,0x03,0xE0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xE9, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xEA, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xEB, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xEC, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xED, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xEE, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00);

    //==============CM3===================//
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xFF, 0x5A,0x80);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0x65, 0x25);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xfd, 0x01);
	//increase MIPI clk skew tolerance and close MIPI HS timeout
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xf9, 0x10);//Phy clk option increase skew tolerance
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0x65, 0x0A);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xF9, 0x1E);//close timeout
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xFF, 0x5A,0x81);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0x65, 0x03);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xF3, 0x61);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0x65, 0x0B);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xF3, 0x78);

    //close NEQ + close BCRC
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xFF, 0x5A,0x83);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0x65, 0x09);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xF7, 0x10,0x10);
    //ByVClearON
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0x65, 0x0B);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xF7, 0x03);

    //PageLock
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xF0, 0xaa,0x00);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0xFF, 0x5A,0x00);

	tm_vtdr6126a_dcs_write_seq_static(ctx, 0x11, 0);
	msleep(130);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0x29, 0);
	msleep(10);


	pr_info("disp:init code %s, data_rate=%d end!\n", __func__, DATA_RATE);
}

static int tm_vtdr6126a_disable(struct drm_panel *panel)
{
	struct tm_vtdr6126a*ctx = panel_to_tm_vtdr6126a(panel);
	pr_info("%s\n", __func__);

	if (!ctx->enabled) {
		pr_info("%s ctx->enabled is 0\n", __func__);
		return 0;
	} else {
		pr_info("%s ctx->enabled not 0\n", __func__);
	}

	if (ctx->backlight) {
		pr_info("%s ctx->backlight is not 0\n", __func__);
		ctx->backlight->props.power = FB_BLANK_POWERDOWN;
		backlight_update_status(ctx->backlight);
	} else {
		pr_info("%s ctx->backlight is  0\n", __func__);
	}

	ctx->enabled = false;

	return 0;
}

static int tm_vtdr6126a_unprepare(struct drm_panel *panel)
{
	struct tm_vtdr6126a *ctx = panel_to_tm_vtdr6126a(panel);
	int ret = 0;

	if (!ctx->prepared) {
		pr_info("%s, already unprepared, return\n", __func__);
		return 0;
	}
	pr_info("%s enter\n", __func__);

	tm_vtdr6126a_dcs_write_seq_static(ctx, 0x28);
	msleep(50);
	tm_vtdr6126a_dcs_write_seq_static(ctx, 0x10);
	msleep(150);

	ctx->reset_gpio = devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
	gpiod_set_value(ctx->reset_gpio, 0);
	devm_gpiod_put(ctx->dev, ctx->reset_gpio);
	msleep(10);

	ret = regulator_disable(ctx->dvdd_supply);
	if (ret) {
		dev_err(ctx->dev, "dvdd_supply failed to disable supply (%d)\n", ret);
		return ret;
	}

	msleep(5);
	ret = regulator_disable(ctx->vci_supply);
	if (ret) {
		dev_err(ctx->dev, "vci_supply failed to disable supply (%d)\n", ret);
		return ret;
	}

	msleep(5);

	//vddi low
	ctx->vddi_en_gpio = devm_gpiod_get(ctx->dev, "vddi_en", GPIOD_OUT_HIGH);
	gpiod_set_value(ctx->vddi_en_gpio, 0);
	devm_gpiod_put(ctx->dev, ctx->vddi_en_gpio);

	ctx->error = 0;
	ctx->prepared = false;

	pr_info("%s-\n", __func__);
	return 0;
}

static int tm_vtdr6126a_prepare(struct drm_panel *panel)
{
	struct tm_vtdr6126a *ctx = panel_to_tm_vtdr6126a(panel);
	int ret;

	pr_info("%s\n", __func__);
	if (ctx->prepared) {
		pr_info("%s, already prepared, return\n", __func__);
		return 0;
	}
	ctx->vddi_en_gpio = devm_gpiod_get(ctx->dev, "vddi_en", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->vddi_en_gpio)) {
		dev_err(ctx->dev, "%s: cannot get vddi_gpio %ld\n",
			__func__, PTR_ERR(ctx->vddi_en_gpio));
		return PTR_ERR(ctx->vddi_en_gpio);
	}
	gpiod_set_value(ctx->vddi_en_gpio, 1);
	devm_gpiod_put(ctx->dev, ctx->vddi_en_gpio);
	msleep(20);

	ret = regulator_enable(ctx->vci_supply);
	if (ret) {
		dev_err(ctx->dev, "failed to enable supply (%d)\n", ret);
		return ret;
	}
	dev_info(ctx->dev, "%s get vci normal purple\n", __func__);

	msleep(20);
	ctx->dvdd_supply = devm_regulator_get_optional(ctx->dev, "dvdd");
	if (IS_ERR_OR_NULL(ctx->dvdd_supply)) {
		dev_info(ctx->dev, "%s get dvdd failed \n", __func__);
		return PTR_ERR(ctx->dvdd_supply);
	} else {
		regulator_set_voltage(ctx->dvdd_supply, 1200000, 1200000);
		ret = regulator_enable(ctx->dvdd_supply);
		if (ret) {
			dev_err(ctx->dev, "failed to enable supply (%d)\n", ret);
			return ret;
		}
		dev_info(ctx->dev, "%s get dvdd normal \n", __func__);
	}

	tm_vtdr6126a_panel_init(ctx);
	ret = ctx->error;
	if (ret < 0) {
		pr_info("disp: %s error ret=%d\n", __func__, ret);
		tm_vtdr6126a_unprepare(panel);
	}

	ctx->prepared = true;
/*#if defined(CONFIG_MTK_PANEL_EXT)
	mtk_panel_tch_rst(panel);
#endif

#ifdef PANEL_SUPPORT_READBACK
	tm_vtdr6126a_panel_get_data(ctx);
#endif*/
	pr_info("disp: %s-\n", __func__);
	return ret;
}

static int tm_vtdr6126a_enable(struct drm_panel *panel)
{
	struct tm_vtdr6126a *ctx = panel_to_tm_vtdr6126a(panel);

	pr_info("disp: %s+\n", __func__);
	if (ctx->enabled) {
		pr_info("%s ctx->enabled is not 0\n", __func__);
		return 0;
	} else{
		pr_info("%s ctx->enabled is  0\n", __func__);
	}

	if (ctx->backlight) {
		pr_info("%s ctx->backlight is not 0\n", __func__);
		ctx->backlight->props.power = FB_BLANK_UNBLANK;
		backlight_update_status(ctx->backlight);
	} else {
		pr_info("%s ctx->backlight is  0\n", __func__);
	}

	ctx->enabled = true;

	pr_info("%s-\n", __func__);
	return 0;
}


static const struct drm_display_mode performance_mode_120hz = {
	.clock = 374838,
	.hdisplay = 1080,
	.hsync_start = 1080 + HFP,
	.hsync_end = 1080 + HFP + HSA,
	.htotal = 1080 + HFP + HSA + HBP,
	.vdisplay = 2352,
	.vsync_start = 2352 + MODE_120_VFP,
	.vsync_end = 2352 + MODE_120_VFP + VSA,
	.vtotal = 2352 + MODE_120_VFP + VSA + VBP,

};

static const struct drm_display_mode performance_mode_90hz = {
	.clock = 374838,
	.hdisplay = 1080,
	.hsync_start = 1080 + HFP,
	.hsync_end = 1080 + HFP + HSA,
	.htotal = 1080 + HFP + HSA + HBP,
	.vdisplay = 2352,
	.vsync_start = 2352 + MODE_90_VFP,
	.vsync_end = 2352 + MODE_90_VFP + VSA,
	.vtotal = 2352 + MODE_90_VFP + VSA + VBP,

};

static const struct drm_display_mode performance_mode_60hz = {
	.clock = 374838,
	.hdisplay = 1080,
	.hsync_start = 1080 + HFP,
	.hsync_end = 1080 + HFP + HSA,
	.htotal = 1080 + HFP + HSA + HBP,
	.vdisplay = 2352,
	.vsync_start = 2352 + MODE_60_VFP,
	.vsync_end = 2352 + MODE_60_VFP + VSA,
	.vtotal = 2352 + MODE_60_VFP + VSA + VBP,

};

#if defined(CONFIG_MTK_PANEL_EXT)


static struct mtk_panel_params ext_params_mode_60 = {
	.change_fps_by_vfp_send_cmd = 1,
 	.data_rate = 1130,
	.cust_esd_check = 1,
	.esd_check_enable = 1,
	.lcm_esd_check_table[0] = {
		.cmd = 0x0a,
		.count = 1,
		.para_list[0] = 0x9c,
	},
	.lcm_esd_check_table[1] = {
		.cmd = 0x66,
		.count = 2,
		.para_list[0] = 0x00,
	},
        .lcm_cellid = {
		.panel_cellid_reg = 0x5A,
		.panel_cellid_len = 23,
		.panel_cellid_offset_reg = 0x65,
		.panel_cellid_offset = 0x00,
        },

	.panel_ver = 1,
	//.panel_id = 0x515a0502,
	.panel_name = "tm_vtdr6126a_vid_1080_2352",
	.panel_supplier = "tm",
	.lcm_index = 0,
	.hbm_type = HBM_MODE_NONE,
	//.max_bl_level = 2047,
	.ssc_enable = 0,
	.lane_swap_en = 0,
	.lp_perline_en = 0,
	.physical_width_um = PHYSICAL_WIDTH,
	.physical_height_um = PHYSICAL_HEIGHT,
	.output_mode = MTK_PANEL_DSC_SINGLE_PORT,
	.dsc_params = {
		.enable = DSC_ENABLE,
		.ver = DSC_VER,
		.slice_mode = DSC_SLICE_MODE,
		.rgb_swap = DSC_RGB_SWAP,
		.dsc_cfg = DSC_DSC_CFG,
		.rct_on = DSC_RCT_ON,
		.bit_per_channel = DSC_BIT_PER_CHANNEL,
		.dsc_line_buf_depth = DSC_DSC_LINE_BUF_DEPTH,
		.bp_enable = DSC_BP_ENABLE,
		.bit_per_pixel = DSC_BIT_PER_PIXEL,
		.pic_height = FRAME_HEIGHT,
		.pic_width = FRAME_WIDTH,
		.slice_height = DSC_SLICE_HEIGHT,
		.slice_width = DSC_SLICE_WIDTH,
		.chunk_size = DSC_CHUNK_SIZE,
		.xmit_delay = DSC_XMIT_DELAY,
		.dec_delay = DSC_DEC_DELAY,
		.scale_value = DSC_SCALE_VALUE,
		.increment_interval = DSC_INCREMENT_INTERVAL,
		.decrement_interval = DSC_DECREMENT_INTERVAL,
		.line_bpg_offset = DSC_LINE_BPG_OFFSET,
		.nfl_bpg_offset = DSC_NFL_BPG_OFFSET,
		.slice_bpg_offset = DSC_SLICE_BPG_OFFSET,
		.initial_offset = DSC_INITIAL_OFFSET,
		.final_offset = DSC_FINAL_OFFSET,
		.flatness_minqp = DSC_FLATNESS_MINQP,
		.flatness_maxqp = DSC_FLATNESS_MAXQP,
		.rc_model_size = DSC_RC_MODEL_SIZE,
		.rc_edge_factor = DSC_RC_EDGE_FACTOR,
		.rc_quant_incr_limit0 = DSC_RC_QUANT_INCR_LIMIT0,
		.rc_quant_incr_limit1 = DSC_RC_QUANT_INCR_LIMIT1,
		.rc_tgt_offset_hi = DSC_RC_TGT_OFFSET_HI,
		.rc_tgt_offset_lo = DSC_RC_TGT_OFFSET_LO,
		.ext_pps_cfg = {
			.enable = 1,
			.rc_buf_thresh = vtdr6126a_rc_buf_thresh,
			.range_min_qp = vtdr6126a_range_min_qp,
			.range_max_qp = vtdr6126a_range_max_qp,
			.range_bpg_ofs = vtdr6126a_range_bpg_ofs,
		},
	},
	.lfr_enable = LFR_EN,
	.lfr_minimum_fps = 60,
	.dyn_fps = {
		.switch_en = 1,
		.vact_timing_fps = 120,
		.dfps_cmd_table[0] = {0, 2, {0x6C, 0x03} },
	},
	/* following MIPI hopping parameter might cause screen mess */
	.dyn = {
		.switch_en = 0,
	},
};

static struct mtk_panel_params ext_params_mode_90 = {
	.change_fps_by_vfp_send_cmd = 1,
	.data_rate = 1130,
	.cust_esd_check = 1,
	.esd_check_enable = 1,
	.lcm_esd_check_table[0] = {
		.cmd = 0x0a,
		.count = 1,
		.para_list[0] = 0x9c,
	},
	.lcm_esd_check_table[1] = {
		.cmd = 0x66,
		.count = 2,
		.para_list[0] = 0x00,
	},
        .lcm_cellid = {
		.panel_cellid_reg = 0x5A,
		.panel_cellid_len = 23,
		.panel_cellid_offset_reg = 0x65,
		.panel_cellid_offset = 0x00,
        },

	.panel_ver = 1,
	//.panel_id = 0x515a0502,
	.panel_name = "tm_vtdr6126a_vid_1080_2352",
	.panel_supplier = "tm",
	.lcm_index = 0,
	.hbm_type = HBM_MODE_NONE,
	//.max_bl_level = 2047,
	.ssc_enable = 0,
	.lane_swap_en = 0,
	.lp_perline_en = 0,
	.physical_width_um = PHYSICAL_WIDTH,
	.physical_height_um = PHYSICAL_HEIGHT,
	.output_mode = MTK_PANEL_DSC_SINGLE_PORT,
	.dsc_params = {
		.enable = DSC_ENABLE,
		.ver = DSC_VER,
		.slice_mode = DSC_SLICE_MODE,
		.rgb_swap = DSC_RGB_SWAP,
		.dsc_cfg = DSC_DSC_CFG,
		.rct_on = DSC_RCT_ON,
		.bit_per_channel = DSC_BIT_PER_CHANNEL,
		.dsc_line_buf_depth = DSC_DSC_LINE_BUF_DEPTH,
		.bp_enable = DSC_BP_ENABLE,
		.bit_per_pixel = DSC_BIT_PER_PIXEL,
		.pic_height = FRAME_HEIGHT,
		.pic_width = FRAME_WIDTH,
		.slice_height = DSC_SLICE_HEIGHT,
		.slice_width = DSC_SLICE_WIDTH,
		.chunk_size = DSC_CHUNK_SIZE,
		.xmit_delay = DSC_XMIT_DELAY,
		.dec_delay = DSC_DEC_DELAY,
		.scale_value = DSC_SCALE_VALUE,
		.increment_interval = DSC_INCREMENT_INTERVAL,
		.decrement_interval = DSC_DECREMENT_INTERVAL,
		.line_bpg_offset = DSC_LINE_BPG_OFFSET,
		.nfl_bpg_offset = DSC_NFL_BPG_OFFSET,
		.slice_bpg_offset = DSC_SLICE_BPG_OFFSET,
		.initial_offset = DSC_INITIAL_OFFSET,
		.final_offset = DSC_FINAL_OFFSET,
		.flatness_minqp = DSC_FLATNESS_MINQP,
		.flatness_maxqp = DSC_FLATNESS_MAXQP,
		.rc_model_size = DSC_RC_MODEL_SIZE,
		.rc_edge_factor = DSC_RC_EDGE_FACTOR,
		.rc_quant_incr_limit0 = DSC_RC_QUANT_INCR_LIMIT0,
		.rc_quant_incr_limit1 = DSC_RC_QUANT_INCR_LIMIT1,
		.rc_tgt_offset_hi = DSC_RC_TGT_OFFSET_HI,
		.rc_tgt_offset_lo = DSC_RC_TGT_OFFSET_LO,
		.ext_pps_cfg = {
			.enable = 1,
			.rc_buf_thresh = vtdr6126a_rc_buf_thresh,
			.range_min_qp = vtdr6126a_range_min_qp,
			.range_max_qp = vtdr6126a_range_max_qp,
			.range_bpg_ofs = vtdr6126a_range_bpg_ofs,
		},
	},
	.lfr_enable = LFR_EN,
	.lfr_minimum_fps = 60,
	.dyn_fps = {
		.switch_en = 1,
		.vact_timing_fps = 120,
		.dfps_cmd_table[0] = {0, 2, {0x6C, 0x02} },

	},
	/* following MIPI hopping parameter might cause screen mess */
	.dyn = {
		.switch_en = 0,

	},
};

static struct mtk_panel_params ext_params_mode_120 = {
	.change_fps_by_vfp_send_cmd = 1,
	.data_rate = 1130,
	.cust_esd_check = 1,
	.esd_check_enable = 1,
	.lcm_esd_check_table[0] = {
		.cmd = 0x0a,
		.count = 1,
		.para_list[0] = 0x9c,
	},
	.lcm_esd_check_table[1] = {
		.cmd = 0x66,
		.count = 2,
		.para_list[0] = 0x00,
	},
        .lcm_cellid = {
		.panel_cellid_reg = 0x5A,
		.panel_cellid_len = 23,
		.panel_cellid_offset_reg = 0x65,
		.panel_cellid_offset = 0x00,
        },

	.panel_ver = 1,
	//.panel_id = 0x515a0502,
	.panel_name = "tm_vtdr6126a_vid_1080_2352",
	.panel_supplier = "tm",
	.lcm_index = 0,
	.hbm_type = HBM_MODE_NONE,
	//.max_bl_level = 2047,
	.ssc_enable = 0,
	.lane_swap_en = 0,
	.lp_perline_en = 0,
	.physical_width_um = PHYSICAL_WIDTH,
	.physical_height_um = PHYSICAL_HEIGHT,
	.output_mode = MTK_PANEL_DSC_SINGLE_PORT,
	.dsc_params = {
		.enable = DSC_ENABLE,
		.ver = DSC_VER,
		.slice_mode = DSC_SLICE_MODE,
		.rgb_swap = DSC_RGB_SWAP,
		.dsc_cfg = DSC_DSC_CFG,
		.rct_on = DSC_RCT_ON,
		.bit_per_channel = DSC_BIT_PER_CHANNEL,
		.dsc_line_buf_depth = DSC_DSC_LINE_BUF_DEPTH,
		.bp_enable = DSC_BP_ENABLE,
		.bit_per_pixel = DSC_BIT_PER_PIXEL,
		.pic_height = FRAME_HEIGHT,
		.pic_width = FRAME_WIDTH,
		.slice_height = DSC_SLICE_HEIGHT,
		.slice_width = DSC_SLICE_WIDTH,
		.chunk_size = DSC_CHUNK_SIZE,
		.xmit_delay = DSC_XMIT_DELAY,
		.dec_delay = DSC_DEC_DELAY,
		.scale_value = DSC_SCALE_VALUE,
		.increment_interval = DSC_INCREMENT_INTERVAL,
		.decrement_interval = DSC_DECREMENT_INTERVAL,
		.line_bpg_offset = DSC_LINE_BPG_OFFSET,
		.nfl_bpg_offset = DSC_NFL_BPG_OFFSET,
		.slice_bpg_offset = DSC_SLICE_BPG_OFFSET,
		.initial_offset = DSC_INITIAL_OFFSET,
		.final_offset = DSC_FINAL_OFFSET,
		.flatness_minqp = DSC_FLATNESS_MINQP,
		.flatness_maxqp = DSC_FLATNESS_MAXQP,
		.rc_model_size = DSC_RC_MODEL_SIZE,
		.rc_edge_factor = DSC_RC_EDGE_FACTOR,
		.rc_quant_incr_limit0 = DSC_RC_QUANT_INCR_LIMIT0,
		.rc_quant_incr_limit1 = DSC_RC_QUANT_INCR_LIMIT1,
		.rc_tgt_offset_hi = DSC_RC_TGT_OFFSET_HI,
		.rc_tgt_offset_lo = DSC_RC_TGT_OFFSET_LO,
		.ext_pps_cfg = {
			.enable = 1,
			.rc_buf_thresh = vtdr6126a_rc_buf_thresh,
			.range_min_qp = vtdr6126a_range_min_qp,
			.range_max_qp = vtdr6126a_range_max_qp,
			.range_bpg_ofs = vtdr6126a_range_bpg_ofs,
		},
	},
	.lfr_enable = LFR_EN,
	.lfr_minimum_fps = 60,
	.dyn_fps = {
		.switch_en = 1,
		.vact_timing_fps = 120,
		.dfps_cmd_table[0] = {0, 2, {0x6C, 0x01} },
	},
	/* following MIPI hopping parameter might cause screen mess */
	.dyn = {
		.switch_en = 0,

	},
};


static int tm_vtdr6126a_setbacklight_cmdq(void *dsi, dcs_write_gce cb,
	void *handle, unsigned int level)
{
	static char bl_tb0[] = { 0x51, 0x3f, 0xff };

	pr_info("%s backlight = %d\n", __func__, level);
	current_bl = level;
	bl_tb0[1] = (level >> 8) & 0x3F;
	bl_tb0[2] = level & 0xFF;

	if (!cb)
		return -1;

	cb(dsi, handle, bl_tb0, ARRAY_SIZE(bl_tb0));

	return 0;
}

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
	struct tm_vtdr6126a*ctx = panel_to_tm_vtdr6126a(panel);

	if (!m)
		return ret;

	pr_info("%s:disp: mode fps=%d", __func__, drm_mode_vrefresh(m));
	if (drm_mode_vrefresh(m) == 60)
		ext->params = &ext_params_mode_60;
	else if (drm_mode_vrefresh(m) == 90)
		ext->params = &ext_params_mode_90;
	else if (drm_mode_vrefresh(m) == 120)
		ext->params = &ext_params_mode_120;
	else
		ret = 1;

	if (!ret) {
		atomic_set(&ctx->current_fps, drm_mode_vrefresh(m));
	}
	return ret;
}

static void mode_switch_to_120(struct drm_panel *panel)
{
	pr_info("%s\n", __func__);
}

static void mode_switch_to_90(struct drm_panel *panel)
{
	pr_info("%s\n", __func__);
}

static void mode_switch_to_60(struct drm_panel *panel)
{
	pr_info("%s\n", __func__);
}

static int mode_switch(struct drm_panel *panel,
		struct drm_connector *connector, unsigned int cur_mode,
		unsigned int dst_mode, enum MTK_PANEL_MODE_SWITCH_STAGE stage)
{
	int ret = 0;
	int dst_fps = 0;
	struct drm_display_mode *m = get_mode_by_id(connector, dst_mode);

	pr_info("%s cur_mode = %d dst_mode %d\n", __func__, cur_mode, dst_mode);

	dst_fps = m ? drm_mode_vrefresh(m) : -EINVAL;

	if (dst_fps == 60) { /* 60 switch to 120 */
		mode_switch_to_60(panel);
	} else if (dst_fps == 90) { /* 1200 switch to 60 */
		mode_switch_to_90(panel);
	} else if (dst_fps == 120) { /* 1200 switch to 60 */
		mode_switch_to_120(panel);
	}  else {
		pr_err("%s, dst_fps %d\n", __func__, dst_fps);
		ret = -EINVAL;
	}

	return ret;
}

static int panel_ext_reset(struct drm_panel *panel, int on)
{
	struct tm_vtdr6126a *ctx = panel_to_tm_vtdr6126a(panel);

	ctx->reset_gpio =
		devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
		gpiod_set_value(ctx->reset_gpio, on);
		devm_gpiod_put(ctx->dev, ctx->reset_gpio);

	pr_info("%s reset on\n", __func__);
	return 0;
}

static void set_lhbm_alpha(unsigned int bl_level)
{
	struct mtk_panel_para_table *pAlphaTable;
	unsigned int alpha = 0;
	unsigned int lhbm_alpha_index = bl_level;

	pr_info("%s: panel_version:%d, lhbm_alpha_index:%d\n", __func__, panel_version, lhbm_alpha_index);

	pAlphaTable = &panel_lhbm_on[0];
	if (panel_version <= PANEL_DVT1) {  //dev1 or before
		if (lhbm_alpha_index >= ARRAY_SIZE(lhbm_alpha)){
			pAlphaTable[0].para_list[1] = (bl_level >> 8) & 0xFF;
			pAlphaTable[0].para_list[2] = bl_level & 0xFF;
			pAlphaTable[1].para_list[1] = 0x10;
			pAlphaTable[1].para_list[2] = 0x00;
			pAlphaTable[1].para_list[3] = (bl_level >> 8) & 0xFF;
			pAlphaTable[1].para_list[4] = bl_level & 0xFF;
			pr_info("%s: backlight %d alpha %d(0x%x, 0x%x)\n", __func__, bl_level, alpha, pAlphaTable->para_list[3], pAlphaTable->para_list[4]);
		} else {
			alpha = lhbm_alpha[lhbm_alpha_index];
			pAlphaTable[0].para_list[1] = (bl_level >> 8) & 0xFF;
			pAlphaTable[0].para_list[2] = bl_level & 0xFF;
			pAlphaTable[1].para_list[1] = (alpha >> 8) & 0xFF;
			pAlphaTable[1].para_list[2] = alpha & 0xFF;
			pAlphaTable[1].para_list[3] = 0x0d;
			pAlphaTable[1].para_list[4] = 0xc0;
			pr_info("%s: backlight %d alpha %d(0x%x, 0x%x)\n", __func__, bl_level, alpha, pAlphaTable->para_list[1], pAlphaTable->para_list[2]);
		}
	}else {  //dev 2 or more
		if (lhbm_alpha_index >= ARRAY_SIZE(lhbm_alpha_dvt2)){
			pAlphaTable[0].para_list[1] = (bl_level >> 8) & 0xFF;
			pAlphaTable[0].para_list[2] = bl_level & 0xFF;
			pAlphaTable[1].para_list[1] = 0x10;
			pAlphaTable[1].para_list[2] = 0x00;
			pAlphaTable[1].para_list[3] = (bl_level >> 8) & 0xFF;
			pAlphaTable[1].para_list[4] = bl_level & 0xFF;
			pr_info("%s: backlight %d alpha %d(0x%x, 0x%x)\n", __func__, bl_level, alpha, pAlphaTable->para_list[3], pAlphaTable->para_list[4]);
		} else {
			alpha = lhbm_alpha_dvt2[lhbm_alpha_index];
			pAlphaTable[0].para_list[1] = (bl_level >> 8) & 0xFF;
			pAlphaTable[0].para_list[2] = bl_level & 0xFF;
			pAlphaTable[1].para_list[1] = (alpha >> 8) & 0xFF;
			pAlphaTable[1].para_list[2] = alpha & 0xFF;
			pAlphaTable[1].para_list[3] = 0x0d;
			pAlphaTable[1].para_list[4] = 0xc0;
			pr_info("%s: backlight %d alpha %d(0x%x, 0x%x)\n", __func__, bl_level, alpha, pAlphaTable->para_list[1], pAlphaTable->para_list[2]);
		}
	}
}

static int panel_lhbm_set_cmdq(void *dsi, dcs_grp_write_gce cb, void *handle, uint32_t on, uint32_t bl_level, uint32_t fps)
{
	unsigned int para_count = 0;
	struct mtk_panel_para_table *pTable = NULL;

	pr_info("%s: bl_level:%d, fps:%d, on:%d\n", __func__, bl_level, fps, on);

	if (on) {
		set_lhbm_alpha(bl_level);
		para_count = sizeof(panel_lhbm_on) / sizeof(struct mtk_panel_para_table);
		pTable = panel_lhbm_on;
	} else {
		para_count = sizeof(panel_lhbm_off) / sizeof(struct mtk_panel_para_table);
		pTable = panel_lhbm_off;
	}
	cb(dsi, handle, pTable, para_count);
	return 0;
}

static int panel_hbm_set_cmdq(struct tm_vtdr6126a *ctx, void *dsi, dcs_grp_write_gce cb, void *handle, uint32_t hbm_state)
{
	struct mtk_panel_para_table hbm_on_table = {3, {0x51, 0x3E, 0x80}};//set hbm code, on
	unsigned int level = current_bl;
	unsigned int fps = atomic_read(&ctx->current_fps);

	pr_info("%s: level:%d, fps:%d, hbm_state:%d, lhbm_en=%d\n", __func__, level, fps, hbm_state, ctx->lhbm_en);

	if (hbm_state > 2) return -1;

	switch (hbm_state)
	{
		case 0:
			if (ctx->lhbm_en){
				panel_lhbm_set_cmdq(dsi, cb, handle, 0, level,  fps);
			}
			break;
		case 1:
			if (ctx->lhbm_en) {
				panel_lhbm_set_cmdq(dsi, cb, handle, 0, level,  fps);

			} else {
				cb(dsi, handle, &hbm_on_table, 1);
			}
			break;
		case 2:
			if (ctx->lhbm_en){
				panel_lhbm_set_cmdq(dsi, cb, handle, 1, level,  fps);
			}
			else
				cb(dsi, handle, &hbm_on_table, 1);
			break;
		default:
			break;
	}

	atomic_set(&ctx->hbm_mode, hbm_state);
	return 0;
}

static int panel_dc_set_cmdq(struct tm_vtdr6126a *ctx, void *dsi, dcs_grp_write_gce cb, void *handle, uint32_t dc_state)
{
	unsigned int para_count = 0;
	struct mtk_panel_para_table *pTable;

	if (dc_state) {
		para_count = sizeof(panel_dc_on) / sizeof(struct mtk_panel_para_table);
		pTable = panel_dc_on;
	} else {
		para_count = sizeof(panel_dc_off) / sizeof(struct mtk_panel_para_table);
		pTable = panel_dc_off;
	}
	cb(dsi, handle, pTable, para_count);
	pr_info("%s: current_fps %d, dc_state %d\n", __func__, atomic_read(&ctx->current_fps), dc_state);
	return 0;
}

static int panel_feature_get(struct drm_panel *panel, struct panel_param_info *param_info){

	struct tm_vtdr6126a *ctx = panel_to_tm_vtdr6126a(panel);
	int ret = 0;
	pr_info("%s enter\n", __func__);

	switch (param_info->param_idx) {
		case PARAM_CABC:
			break;
		case PARAM_ACL:
			//ret = -1;
			break;
		case PARAM_HBM:
			param_info->value = atomic_read(&ctx->hbm_mode);
			break;
		case PARAM_DC:
			param_info->value = atomic_read(&ctx->dc_mode);
			break;
		default:
			ret = -1;
			break;
	}
	return ret;

}

static int panel_feature_set(struct drm_panel *panel, void *dsi,
			      dcs_grp_write_gce cb, void *handle, struct panel_param_info param_info)
{
	struct tm_vtdr6126a *ctx = panel_to_tm_vtdr6126a(panel);
	int ret = -1;

	if (!cb)
		return -1;

	if (!ctx->enabled) {
		pr_info("%s: skip set feature %d to %d, panel not enabled\n", __func__, param_info.param_idx, param_info.value);
		return -1;
	}

	pr_info("%s: start set feature %d to %d\n", __func__, param_info.param_idx, param_info.value);

	switch (param_info.param_idx) {
		case PARAM_CABC:
			break;
		case PARAM_HBM:
			atomic_set(&ctx->hbm_mode, param_info.value);
			panel_hbm_set_cmdq(ctx, dsi, cb, handle, param_info.value);
			ret = 0;
			break;
		case PARAM_DC:
			panel_dc_set_cmdq(ctx, dsi, cb, handle, param_info.value);
			atomic_set(&ctx->dc_mode, param_info.value);
			ret = 0;
			break;
		default:
			pr_info("%s: skip unsupport feature %d to %d\n", __func__, param_info.param_idx, param_info.value);
			break;
	}

	return ret;
}

static struct mtk_panel_funcs ext_funcs = {
	.set_backlight_cmdq = tm_vtdr6126a_setbacklight_cmdq,
	.reset = panel_ext_reset,
	.ext_param_set = mtk_panel_ext_param_set,
	.mode_switch = mode_switch,
//	.get_lcm_version = panel_get_lcm_version,
//	.ata_check = panel_ata_check,
	.panel_feature_set = panel_feature_set,
	.panel_feature_get = panel_feature_get,
};
#endif

static int tm_vtdr6126a_get_modes(struct drm_panel *panel,
						struct drm_connector *connector)
{
	struct drm_display_mode *mode;
	struct drm_display_mode *mode_1;
	struct drm_display_mode *mode_2;

	mode = drm_mode_duplicate(connector->dev, &performance_mode_120hz);
	printk("[%d]disp: mode:\n",__LINE__, __FUNCTION__);
	if (!mode) {
		dev_err(connector->dev->dev, "failed to add mode %ux%ux@%u\n",
			performance_mode_120hz.hdisplay,
			performance_mode_120hz.vdisplay,
			drm_mode_vrefresh(&performance_mode_120hz));
		return -ENOMEM;
	}
	drm_mode_set_name(mode);
	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	drm_mode_probed_add(connector, mode);

	mode_1 = drm_mode_duplicate(connector->dev, &performance_mode_60hz);
	if (!mode_1) {
		dev_err(connector->dev->dev, "failed to add mode %ux%ux@%u\n",
			performance_mode_60hz.hdisplay,
			performance_mode_60hz.vdisplay,
			drm_mode_vrefresh(&performance_mode_60hz));
		return -ENOMEM;
	}
	drm_mode_set_name(mode_1);
	mode_1->type = DRM_MODE_TYPE_DRIVER;
	drm_mode_probed_add(connector, mode_1);

	mode_2 = drm_mode_duplicate(connector->dev, &performance_mode_90hz);
	if (!mode_2) {
		dev_err(connector->dev->dev, "failed to add mode %ux%ux@%u\n",
			performance_mode_90hz.hdisplay,
			performance_mode_90hz.vdisplay,
			drm_mode_vrefresh(&performance_mode_90hz));
		return -ENOMEM;
	}
	drm_mode_set_name(mode_2);
	mode_2->type = DRM_MODE_TYPE_DRIVER;
	drm_mode_probed_add(connector, mode_2);

	connector->display_info.width_mm = 70;
	connector->display_info.height_mm = 156;
	printk("[%d  %s]end\n",__LINE__, __FUNCTION__);

	return 1;
}

static const struct drm_panel_funcs tm_vtdr6126a_drm_funcs = {
	.disable = tm_vtdr6126a_disable,
	.unprepare = tm_vtdr6126a_unprepare,
	.prepare = tm_vtdr6126a_prepare,
	.enable = tm_vtdr6126a_enable,
	.get_modes = tm_vtdr6126a_get_modes,
};

static void lcm_parse_panel_version(struct tm_vtdr6126a *ctx)
{
	int rc;
	struct device_node *chosen = of_find_node_by_name(NULL, "chosen");

	ctx->version = PANEL_PVT;
	if(chosen) {
		u32 tmp = 0;

		rc = of_property_read_u32(chosen, "mmi,panel_ver", &tmp);
		if (!rc) {
			if (PANEL_EVT == tmp) {
				ctx->version = PANEL_EVT;
			} else if(PANEL_DVT1 == tmp) {
				ctx->version = PANEL_DVT1;
			} else if(PANEL_DVT2 == tmp) {
				ctx->version = PANEL_DVT2;
			} else if(PANEL_PVT == tmp) {
				ctx->version = PANEL_PVT;
			} else {
				ctx->version = PANEL_PVT;
			}
			pr_info("get panel_ver:%d\n", ctx->version);
		}
		else
			pr_info("mmi,panel_ver not get\n");
	}
	else
		pr_info("parse_panel chosen node null\n");

	panel_version = ctx->version;
	pr_info("parse_panel get panel_ver:%d\n", ctx->version);
	return;
}

static int tm_vtdr6126a_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct device_node *dsi_node, *remote_node = NULL, *endpoint = NULL;
	struct tm_vtdr6126a *ctx;
	struct device_node *backlight;
	int ret;

	pr_info("%s+ disp:tm_vtdr6126a_probe start!\n", __func__);

	dsi_node = of_get_parent(dev->of_node);
	if (dsi_node) {
		endpoint = of_graph_get_next_endpoint(dsi_node, NULL);
		if (endpoint) {
			remote_node = of_graph_get_remote_port_parent(endpoint);
			if (!remote_node) {
				pr_info("No panel connected,skip probe lcm\n");
				return -ENODEV;
			}
			pr_info("disp:device node name:%s\n", remote_node->name);
		}
	}
	if (remote_node != dev->of_node) {
		pr_info("%s+ skip probe due to not current lcm\n", __func__);
		return -ENODEV;
	}

	ctx = devm_kzalloc(dev, sizeof(struct tm_vtdr6126a), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	mipi_dsi_set_drvdata(dsi, ctx);

	ctx->dev = dev;
	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_SYNC_PULSE |
			 MIPI_DSI_MODE_NO_EOT_PACKET |
			MIPI_DSI_CLOCK_NON_CONTINUOUS;

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

	ctx->vddi_en_gpio = devm_gpiod_get(ctx->dev, "vddi_en", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->vddi_en_gpio)) {
		dev_info(dev, "cannot get vddi_en_gpio %ld\n",
			 PTR_ERR(ctx->vddi_en_gpio));
		return PTR_ERR(ctx->vddi_en_gpio);
	}
	devm_gpiod_put(ctx->dev, ctx->vddi_en_gpio);

	ctx->vci_supply = devm_regulator_get_optional(ctx->dev, "vci");
	if (IS_ERR_OR_NULL(ctx->vci_supply)) {
		dev_info(dev, "%s get vci failed \n", __func__);
		return -EPROBE_DEFER;
	} else {
		regulator_set_voltage(ctx->vci_supply, 3000000, 3000000);
		ret = regulator_enable(ctx->vci_supply);
		if (ret) {
			dev_err(ctx->dev, "failed to enable supply (%d)\n", ret);
			return ret;
		}
		dev_info(dev, "%s get vci normal purple\n", __func__);
	}

	ctx->dvdd_supply = devm_regulator_get_optional(ctx->dev, "dvdd");
	if (IS_ERR_OR_NULL(ctx->dvdd_supply)) {
		dev_info(dev, "%s get dvdd failed \n", __func__);
		return -EPROBE_DEFER;
	} else {
		regulator_set_voltage(ctx->dvdd_supply, 1200000, 1200000);
		ret = regulator_enable(ctx->dvdd_supply);
		if (ret) {
			dev_err(ctx->dev, "failed to enable supply (%d)\n", ret);
			return ret;
		}
		dev_info(dev, "%s get dvdd normal \n", __func__);
	}
	ctx->prepared = true;
	ctx->enabled = true;

	drm_panel_init(&ctx->panel, dev, &tm_vtdr6126a_drm_funcs, DRM_MODE_CONNECTOR_DSI);

	drm_panel_add(&ctx->panel);

	//parse panel version for evt/dvt/pvt
	lcm_parse_panel_version(ctx);

	ret = mipi_dsi_attach(dsi);
	if (ret < 0)
		drm_panel_remove(&ctx->panel);

#if defined(CONFIG_MTK_PANEL_EXT)
	mtk_panel_tch_handle_reg(&ctx->panel);
	ret = mtk_panel_ext_create(dev, &ext_params_mode_120, &ext_funcs, &ctx->panel);
	if (ret < 0)
		return ret;
#endif
	atomic_set(&ctx->hbm_mode, 0);
	ctx->lhbm_en = 1;
	atomic_set(&ctx->current_fps, 120);
	atomic_set(&ctx->dc_mode, 0);
	pr_info("[%d  %s]-tm,vtdr6126a,vdo,120hz ret:%d\n", __LINE__, __func__,ret);

	return ret;
}

static int tm_vtdr6126a_remove(struct mipi_dsi_device *dsi)
{
	struct tm_vtdr6126a *ctx = mipi_dsi_get_drvdata(dsi);
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

static void lcm_shutdown(struct mipi_dsi_device *dsi)
{

        pr_info("%s\n", __func__);

#ifdef BIAS_OCP2138
                pr_info("%s: ocp2138_BiasPower_disable\n", __func__);
                ocp2138_BiasPower_disable(5);
#endif

}

static const struct of_device_id tm_vtdr6126a_of_match[] = {
	{
		.compatible = "tm_vtdr6126a_vid_1080_2352",
	},
	{}
};

MODULE_DEVICE_TABLE(of, tm_vtdr6126a_of_match);

static struct mipi_dsi_driver tm_vtdr6126a_driver = {
	.probe = tm_vtdr6126a_probe,
	.remove = tm_vtdr6126a_remove,
	.shutdown = lcm_shutdown,
	.driver = {
		.name = "tm_vtdr6126a_vid_1080_2352",
		.owner = THIS_MODULE,
		.of_match_table = tm_vtdr6126a_of_match,
	},
};

module_mipi_dsi_driver(tm_vtdr6126a_driver);

MODULE_AUTHOR("mediatek");
MODULE_DESCRIPTION("tm vtdr6126a incell 120hz Panel Driver");
MODULE_LICENSE("GPL v2");

