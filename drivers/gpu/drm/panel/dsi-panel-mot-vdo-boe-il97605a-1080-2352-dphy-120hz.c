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
#include "include/dsi-panel-mot-vdo-boe-il97605a-1080-2352-dphy-120hz.h"
#endif

#define PANEL_LDO_VTP_EN

#ifdef PANEL_LDO_VTP_EN
#include <linux/gpio/consumer.h>
#include <linux/regulator/consumer.h>
#endif

/* option function to read data from some panel address */
/* #define PANEL_SUPPORT_READBACK */

unsigned int il97605a_rc_buf_thresh[14] = {896, 1792, 2688, 3584, 4480, 5376,
        6272, 6720, 7168, 7616, 7744, 7872, 8000, 8064};
unsigned int il97605a_range_min_qp[15] = {0, 4, 5, 5, 7, 7, 7, 7, 7, 7, 9, 9, 9, 13, 16};
unsigned int il97605a_range_max_qp[15] = {8, 8, 9, 10, 11, 11, 11, 12, 13, 14, 14, 15,
        15, 16, 17};
int il97605a_range_bpg_ofs[15] = {2, 0, 0, -2, -4, -6, -8, -8, -8, -10, -10, -12, -12,
        -12, -12};

//TM EVT panel v0 support. to be disabled before PVT
//#define TM_PANEL_EVT_V0_SUPPORT		1

//panel id, reg 0xF1, value 02 05 5a 51
#define IL_BOE_PANEL_VENDOR_ID    0x515a0502
//#define IL_BOE_PANEL_VENDOR_ID  	(TM_ILI_PANEL_VENDOR_ID | (0xF << 24))
#define FOD_CENTER_X 636
#define FOD_CENTER_Y 2525

static int tp_gesture_flag = 0;

struct boe_il97605a {
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
};

//set enable lhbm code, on status
static struct mtk_panel_para_table panel_lhbm_on[] = {
      //set LHBM on
      {4, {0xFF, 0x5A, 0xA5, 0x00}},
      {2, {0xBA, 0x03}},
};

//set enable lhbm code, off status
static struct mtk_panel_para_table panel_lhbm_off[] = {
	//set LHBM off
      {4, {0xFF,0x5A,0xA5,0x00}},
      {2, {0xBA,0x00}},
};

#define boe_il97605a_dcs_write_seq(ctx, seq...)                                     \
	({                                                                     \
		const u8 d[] = {seq};                                          \
		BUILD_BUG_ON_MSG(ARRAY_SIZE(d) > 64,                           \
				 "DCS sequence too big for stack");            \
		boe_il97605a_dcs_write(ctx, d, ARRAY_SIZE(d));                      \
	})

#define boe_il97605a_dcs_write_seq_static(ctx, seq...)                              \
	({                                                                     \
		static const u8 d[] = {seq};                                   \
		boe_il97605a_dcs_write(ctx, d, ARRAY_SIZE(d));                      \
	})

static inline struct boe_il97605a *panel_to_boe_il97605a(struct drm_panel *panel)
{
	return container_of(panel, struct boe_il97605a, panel);
}

#ifdef PANEL_SUPPORT_READBACK
static int boe_il97605a_dcs_read(struct boe_il97605a *ctx, u8 cmd, void *data, size_t len)
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

static void boe_il97605a_panel_get_data(struct boe_il97605a *ctx)
{
	u8 buffer[3] = {0};
	static int ret;

	if (ret == 0) {
		ret = boe_il97605a_dcs_read(ctx, 0x0A, buffer, 1);
		pr_info("disp: %s 0x%08x\n", __func__, buffer[0] | (buffer[1] << 8));
		dev_info(ctx->dev, "return %d data(0x%08x) to dsi engine\n",
			 ret, buffer[0] | (buffer[1] << 8));
	}
}
#endif

static void boe_il97605a_dcs_write(struct boe_il97605a *ctx, const void *data, size_t len)
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


static void boe_il97605a_panel_init(struct boe_il97605a *ctx)
{
	//int current_fps = atomic_read(&ctx->current_fps);

	pr_info("disp: %s+\n", __func__);

	ctx->reset_gpio = devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio)) {
		dev_err(ctx->dev, "%s: cannot get reset_gpio %ld\n",
			__func__, PTR_ERR(ctx->reset_gpio));
		//return;
	}
	else {
		gpiod_set_value(ctx->reset_gpio, 0);
		usleep_range(1 * 1000, 2 * 1000);
		gpiod_set_value(ctx->reset_gpio, 1);
		usleep_range(1 * 1000, 2 * 1000);
		gpiod_set_value(ctx->reset_gpio, 0);
		usleep_range(10 * 1000, 12 * 1000);
		gpiod_set_value(ctx->reset_gpio, 1);
		usleep_range(10 * 1000, 12 * 1000);
		devm_gpiod_put(ctx->dev, ctx->reset_gpio);
		pr_info("disp: %s reset_gpio\n", __func__);
	}
	 //page 7
	 //scaling up Start
	 //Update scallingup data from vendor OP file
	 boe_il97605a_dcs_write_seq_static(ctx, 0xFF, 0x5A, 0xA5, 0x07);
	 boe_il97605a_dcs_write_seq_static(ctx, 0xC0, 0x14);
	 boe_il97605a_dcs_write_seq_static(ctx, 0xC2, 0x10);
	 boe_il97605a_dcs_write_seq_static(ctx, 0xC3, 0x40);
	 boe_il97605a_dcs_write_seq_static(ctx, 0xC4, 0x01);
	 boe_il97605a_dcs_write_seq_static(ctx, 0xCC, 0x81);
	 boe_il97605a_dcs_write_seq_static(ctx, 0xCD, 0x59);
	 boe_il97605a_dcs_write_seq_static(ctx, 0xCE, 0x80);
	 boe_il97605a_dcs_write_seq_static(ctx, 0xCF, 0xFB);
	 boe_il97605a_dcs_write_seq_static(ctx, 0xD0, 0x81);
	 boe_il97605a_dcs_write_seq_static(ctx, 0xD1, 0x1E);
	 boe_il97605a_dcs_write_seq_static(ctx, 0xD2, 0x85);
	 boe_il97605a_dcs_write_seq_static(ctx, 0xD3, 0x66);
	 boe_il97605a_dcs_write_seq_static(ctx, 0xFF, 0x5A,0xA5,0x00);
	 boe_il97605a_dcs_write_seq_static(ctx, 0xB1, 0x01);
	 boe_il97605a_dcs_write_seq_static(ctx, 0xFF, 0x5A,0xA5,0x04);
	 boe_il97605a_dcs_write_seq_static(ctx, 0xCF, 0x00);
	 boe_il97605a_dcs_write_seq_static(ctx, 0xD0, 0x1C);
	 boe_il97605a_dcs_write_seq_static(ctx, 0xD1, 0x00);
	 boe_il97605a_dcs_write_seq_static(ctx, 0xD2, 0x21);
	 boe_il97605a_dcs_write_seq_static(ctx, 0xD3, 0x04);
	 boe_il97605a_dcs_write_seq_static(ctx, 0xD4, 0x00);
	 boe_il97605a_dcs_write_seq_static(ctx, 0xD5, 0xD9);
	 boe_il97605a_dcs_write_seq_static(ctx, 0xD6, 0x36);
	 boe_il97605a_dcs_write_seq_static(ctx, 0xDD, 0x00);
	 boe_il97605a_dcs_write_seq_static(ctx, 0xE0, 0x04);
	 boe_il97605a_dcs_write_seq_static(ctx, 0xE1, 0x38);
	 boe_il97605a_dcs_write_seq_static(ctx, 0xE2, 0x09);
	 boe_il97605a_dcs_write_seq_static(ctx, 0xE3, 0x30);
	 boe_il97605a_dcs_write_seq_static(ctx, 0xE9, 0x10,0x00);//Hsync Gating Off
	 boe_il97605a_dcs_write_seq_static(ctx, 0xED, 0x01,0x00,0x21,0xD9,0x36,0x00,0x35,0xD9,0x5B,0x33,0x00,0x00,0x00,0x00,0x00,0x00,0x00);

	 boe_il97605a_dcs_write_seq_static(ctx, 0xB3, 0x08);//Scalling Maunal MODE
	 boe_il97605a_dcs_write_seq_static(ctx, 0xB4, 0x1D);
	 //update data for 0xB5 for scallingup
	 boe_il97605a_dcs_write_seq_static(ctx, 0xB5, 0x00,0x6B, 0x00, 0x6B, 0x00, 0x5B, 0x00, 0x5B, 0x00, 0x6B, 0x00, 0x6B, 0x00, 0x6B, 0x00, 0x6B, 0x00, 0x6B, 0x00, 0x6B);
	 boe_il97605a_dcs_write_seq_static(ctx, 0xCA, 0x80);
	 boe_il97605a_dcs_write_seq_static(ctx, 0xD7, 0x80);

	 boe_il97605a_dcs_write_seq_static(ctx, 0xFF, 0x5A,0xA5,0x02);
	 boe_il97605a_dcs_write_seq_static(ctx, 0x86, 0x56,0x1D,0x18);
	 boe_il97605a_dcs_write_seq_static(ctx, 0xFF, 0x5A,0xA5,0x1F);
	 boe_il97605a_dcs_write_seq_static(ctx, 0xA0, 0x18);
	 //scalling up end
	 //init code
	 //page 5
	 boe_il97605a_dcs_write_seq_static(ctx, 0xFF, 0x5A,0xA5,0x05);
	 boe_il97605a_dcs_write_seq_static(ctx, 0x83, 0x83);
	 //LVD? 03 on / 83 off
	 //----DSC 10bit 3.75 compress----//
	 boe_il97605a_dcs_write_seq_static(ctx, 0xFF, 0x5A, 0xA5, 0x07);
	 boe_il97605a_dcs_write_seq_static(ctx, 0x8A, 0x01);
	 boe_il97605a_dcs_write_seq_static(ctx, 0x8B, 0xA0);

	 boe_il97605a_dcs_write_seq_static(ctx, 0x80, 0x00,0x00,0x00,0x12,0x00,0x00,0xab,0x30,0x80,0x09,0x30,0x04,0x38,0x00,0x0c,0x02,0x1c,0x02,0x1c,0x02,0x00,0x02,0x0e,0x00,0x20,0x01,0x1f,0x00,0x07,0x00,0x0c,0x08,0xbb,0x08,0x7a,0x18,0x00,0x10,0xf0,0x07,0x10,0x20,0x00,0x06,0x0f,0x0f,0x33,0x0e,0x1c,0x2a,0x38,0x46,0x54,0x62,0x69,0x70,0x77,0x79,0x7b,0x7d,0x7e,0x02,0x02,0x22,0x00,0x2a,0x40,0x2a,0xbe,0x3a,0xfc,0x3a,0xfa,0x3a,0xf8,0x3b,0x38,0x3b,0x78,0x3b,0xb6,0x4b,0xb6,0x4b,0xf4,0x4b,0xf4,0x6c,0x34,0x84,0x74,0x00,0x00,0x00,0x00,0x00,0x00);
	 boe_il97605a_dcs_write_seq_static(ctx, 0x81, 0x00,0x00,0x00,0x12,0x00,0x00,0xab,0x30,0x80,0x09,0x30,0x04,0x38,0x00,0x0c,0x02,0x1c,0x02,0x1c,0x02,0x00,0x02,0x0e,0x00,0x20,0x01,0x1f,0x00,0x07,0x00,0x0c,0x08,0xbb,0x08,0x7a,0x18,0x00,0x10,0xf0,0x07,0x10,0x20,0x00,0x06,0x0f,0x0f,0x33,0x0e,0x1c,0x2a,0x38,0x46,0x54,0x62,0x69,0x70,0x77,0x79,0x7b,0x7d,0x7e,0x02,0x02,0x22,0x00,0x2a,0x40,0x2a,0xbe,0x3a,0xfc,0x3a,0xfa,0x3a,0xf8,0x3b,0x38,0x3b,0x78,0x3b,0xb6,0x4b,0xb6,0x4b,0xf4,0x4b,0xf4,0x6c,0x34,0x84,0x74,0x00,0x00,0x00,0x00,0x00,0x00);
	//Page CRC
	 boe_il97605a_dcs_write_seq_static(ctx, 0xFF, 0x5A, 0xA5, 0x07);
	 boe_il97605a_dcs_write_seq_static(ctx, 0xA0, 0x24);
	 //Page 3
	 boe_il97605a_dcs_write_seq_static(ctx, 0xFF, 0x5A, 0xA5, 0x03);
	 //PCD enable
	 boe_il97605a_dcs_write_seq_static(ctx, 0xBD, 0x11);
	 boe_il97605a_dcs_write_seq_static(ctx, 0xBB, 0x4C);

	 //--page8--
	 boe_il97605a_dcs_write_seq_static(ctx, 0xFF, 0x5A, 0xA5, 0x08);
	 //sleep out don't reload OTP
	 boe_il97605a_dcs_write_seq_static(ctx, 0xC8, 0x62);

	 //--page24--
	 boe_il97605a_dcs_write_seq_static(ctx, 0xFF, 0x5A, 0xA5, 0x24);
	 //set dimming
	 boe_il97605a_dcs_write_seq_static(ctx, 0x85, 0x61, 0x80);
	 boe_il97605a_dcs_write_seq_static(ctx, 0x86, 0x61, 0x80);

	 //--IRC EN--
	 boe_il97605a_dcs_write_seq_static(ctx, 0xFF, 0x5A, 0xA5, 0x00);
	 //--IRC ON--
	 boe_il97605a_dcs_write_seq_static(ctx, 0xB9, 0x01);
	 //mode control
	 //LTPS mode = 120Hz normal
	 boe_il97605a_dcs_write_seq_static(ctx, 0xD0, 0x00);
	 //AOD LTPS model = 120Hz AOD
	 boe_il97605a_dcs_write_seq_static(ctx, 0xD1, 0x00);
	 //DSI video enable
	 //dsi_video_en=1
	 boe_il97605a_dcs_write_seq_static(ctx, 0x71, 0x01);
	 //--------------------//
	 boe_il97605a_dcs_write_seq_static(ctx, 0xFF, 0x5A, 0xA5, 0x00);
	 //TE On
	 boe_il97605a_dcs_write_seq_static(ctx, 0x35, 0x00);
	 //BC On,ox Dimming Off
	 boe_il97605a_dcs_write_seq_static(ctx, 0x53, 0x24);

	//BEGIN Motorola, IKSWW-46934, Modify FOD spot position
	boe_il97605a_dcs_write_seq_static(ctx, 0xFF, 0x5A, 0xA5, 0x00);
	boe_il97605a_dcs_write_seq_static(ctx, 0x96, (FOD_CENTER_Y >> 8) & 0xFF);// POS-Y
	boe_il97605a_dcs_write_seq_static(ctx, 0x97, FOD_CENTER_Y & 0xFF);
	boe_il97605a_dcs_write_seq_static(ctx, 0x98, (FOD_CENTER_X >> 8) & 0xFF); // POS-X
	boe_il97605a_dcs_write_seq_static(ctx, 0x99, FOD_CENTER_X & 0xFF);
	//END Motorola, IKSWW-46934

	 //Sleep out
	 boe_il97605a_dcs_write_seq_static(ctx, 0x11, 0);
	 msleep(120);
 	 //Display on
	 boe_il97605a_dcs_write_seq_static(ctx, 0x29, 0);
	 msleep(10);

	pr_info("disp:init code %s, data_rate=%d end!\n", __func__, DATA_RATE);
}

static int boe_il97605a_disable(struct drm_panel *panel)
{
	struct boe_il97605a*ctx = panel_to_boe_il97605a(panel);
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

static int panel_set_gesture_flag(int state)
{
	if(state == 1)
		tp_gesture_flag = 1;
	else
		tp_gesture_flag = 0;

	pr_info("%s:disp:set tp_gesture_flag:%d\n", __func__, tp_gesture_flag);
	return 0;
}

static int boe_il97605a_unprepare(struct drm_panel *panel)
{
	struct boe_il97605a *ctx = panel_to_boe_il97605a(panel);
	int ret = 0;

	if (!ctx->prepared) {
		pr_info("%s, already unprepared, return\n", __func__);
		return 0;
	}
	pr_info("%s\n", __func__);
	printk("[%d  %s]_check_dsi !!\n",__LINE__, __FUNCTION__);

	boe_il97605a_dcs_write_seq_static(ctx, 0x28);
	msleep(20);
	boe_il97605a_dcs_write_seq_static(ctx, 0x10);
	msleep(120);

	ctx->prepared = false;

	pr_info("%s:disp: tp_gesture_flag:%d\n",__func__, tp_gesture_flag);
	if(!tp_gesture_flag) {
		ret = regulator_disable(ctx->vci_supply);
		if (ret) {
			dev_err(ctx->dev, "vci_supply failed to disable supply (%d)\n", ret);
			return ret;
		}

		msleep(5);

		ret = regulator_disable(ctx->dvdd_supply);
		if (ret) {
			dev_err(ctx->dev, "dvdd_supply failed to disable supply (%d)\n", ret);
			return ret;
		}

		msleep(5);

		//vddi low
		ctx->vddi_en_gpio = devm_gpiod_get(ctx->dev, "vddi_en", GPIOD_OUT_HIGH);
		gpiod_set_value(ctx->vddi_en_gpio, 0);
		devm_gpiod_put(ctx->dev, ctx->vddi_en_gpio);
	}

	ctx->error = 0;
	return 0;
}

static int boe_il97605a_prepare(struct drm_panel *panel)
{
	struct boe_il97605a *ctx = panel_to_boe_il97605a(panel);
	int ret;

	pr_info("%s\n", __func__);
	if (ctx->prepared) {
		pr_info("%s, already prepared, return\n", __func__);
		return 0;
	}

	if(!tp_gesture_flag) {
		ctx->reset_gpio = devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
		gpiod_set_value(ctx->reset_gpio, 0);
		devm_gpiod_put(ctx->dev, ctx->reset_gpio);
		// end

		msleep(5);

		ctx->vddi_en_gpio = devm_gpiod_get(ctx->dev, "vddi_en", GPIOD_OUT_HIGH);
		gpiod_set_value(ctx->vddi_en_gpio, 1);
		devm_gpiod_put(ctx->dev, ctx->vddi_en_gpio);

		msleep(5);

		ret = regulator_enable(ctx->dvdd_supply);
		if (ret) {
			dev_err(ctx->dev, "failed to enable supply (%d)\n", ret);
			return ret;
		}

		msleep(5);

		ret = regulator_enable(ctx->vci_supply);
		if (ret) {
			dev_err(ctx->dev, "failed to enable supply (%d)\n", ret);
			return ret;
		}
	}

	boe_il97605a_panel_init(ctx);
	ret = ctx->error;
	if (ret < 0) {
		pr_info("disp: %s error ret=%d\n", __func__, ret);
		boe_il97605a_unprepare(panel);
	}

	ctx->prepared = true;
/*#if defined(CONFIG_MTK_PANEL_EXT)
	mtk_panel_tch_rst(panel);
#endif

#ifdef PANEL_SUPPORT_READBACK
	boe_il97605a_panel_get_data(ctx);
#endif*/
	pr_info("disp: %s-\n", __func__);
	return ret;
}

static int boe_il97605a_enable(struct drm_panel *panel)
{
	struct boe_il97605a *ctx = panel_to_boe_il97605a(panel);

	pr_info("disp: %s+\n", __func__);
	if (ctx->enabled)
		return 0;

	if (ctx->backlight) {
		ctx->backlight->props.power = FB_BLANK_UNBLANK;
		backlight_update_status(ctx->backlight);
	}

	ctx->enabled = true;

	pr_info("%s-\n", __func__);
	return 0;
}


static const struct drm_display_mode performance_mode_120hz = {
	.clock		= 374837,
	.hdisplay = FRAME_WIDTH,
	.hsync_start = FRAME_WIDTH + MODE_120_HFP,
	.hsync_end = FRAME_WIDTH + MODE_120_HFP + HSA,
	.htotal = FRAME_WIDTH + MODE_120_HFP + HSA + HBP,
	.vdisplay = FRAME_HEIGHT,
	.vsync_start = FRAME_HEIGHT + MODE_120_VFP,
	.vsync_end = FRAME_HEIGHT + MODE_120_VFP + VSA,
	.vtotal = FRAME_HEIGHT + MODE_120_VFP + VSA + VBP,
};

static const struct drm_display_mode performance_mode_90hz = {
	.clock		= 374837,
	.hdisplay = FRAME_WIDTH,
	.hsync_start = FRAME_WIDTH + MODE_90_HFP,
	.hsync_end = FRAME_WIDTH + MODE_90_HFP + HSA,
	.htotal = FRAME_WIDTH + MODE_90_HFP + HSA + HBP,
	.vdisplay = FRAME_HEIGHT,
	.vsync_start = FRAME_HEIGHT + MODE_90_VFP,
	.vsync_end = FRAME_HEIGHT + MODE_90_VFP + VSA,
	.vtotal = FRAME_HEIGHT + MODE_90_VFP + VSA + VBP,
};

static const struct drm_display_mode performance_mode_60hz = {
	.clock		= 374685,
	.hdisplay = FRAME_WIDTH,
	.hsync_start = FRAME_WIDTH + MODE_60_HFP,
	.hsync_end = FRAME_WIDTH + MODE_60_HFP + HSA,
	.htotal = FRAME_WIDTH + MODE_60_HFP + HSA + HBP,
	.vdisplay = FRAME_HEIGHT,
	.vsync_start = FRAME_HEIGHT + MODE_60_VFP,
	.vsync_end = FRAME_HEIGHT + MODE_60_VFP + VSA,
	.vtotal = FRAME_HEIGHT + MODE_60_VFP + VSA + VBP,
};

#if defined(CONFIG_MTK_PANEL_EXT)


static struct mtk_panel_params ext_params_mode_60 = {
	.data_rate = DATA_RATE,
	.cust_esd_check = 1,
	.esd_check_enable = 1,
	.lcm_esd_check_table[0] = {
		.cmd = 0x0a,
		.count = 1,
		.para_list[0] = 0x9c,
	},
	.lcm_cellid = {
		.panel_cellid_reg = 0xF2,
		.panel_cellid_len = 23,
		.panel_cellid_esd_dis = 1,
		.panel_cellid_read_max = 1,
		.panel_cellid_regdata_increase = 1,
		.page_cmd_always = 1,
		.page_table = {
			{0x39, 0x04, 0xFF, 0x5A, 0xA5, 0x08},
			{0x15, 0x02, 0xB0, 0x55},
			{0x39, 0x04, 0xFF, 0x5A, 0xA5, 0x00},
			{0x39, 0x03, 0xFD, 0x01, 0x81},
		},
		.page_post_table = {
			{0x39, 0x04, 0xFD, 0x00, 0x00, 0x00},
			{0x39, 0x04, 0xFF, 0x5A, 0xA5, 0x08},
			{0x39, 0x03, 0xB0, 0x01, 0x00},
			{0x39, 0x04, 0xFF, 0x5A, 0xA5, 0x00},
		},
	},

	.panel_ver = 1,
	//.panel_id = 0x515a0502,
	.panel_name = "boe_il97605a_vid_1080_2352",
	.panel_supplier = "boe",
	.lcm_index = 0,
	//.hbm_type = HBM_MODE_RAMPING,
	//.max_bl_level = 2047,
	.ssc_enable = 0,
	.lane_swap_en = 0,
	.lp_perline_en = 0,
	.physical_width_um = PHYSICAL_WIDTH,
	.physical_height_um = PHYSICAL_HEIGHT,
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
		.ext_pps_cfg = {
			.enable = 1,
			.rc_buf_thresh = il97605a_rc_buf_thresh,
			.range_min_qp = il97605a_range_min_qp,
			.range_max_qp = il97605a_range_max_qp,
			.range_bpg_ofs = il97605a_range_bpg_ofs,
		},
	},
	.lfr_enable = LFR_EN,
	.lfr_minimum_fps = MODE_60_FPS,
	.dyn_fps = {
		.switch_en = 1,
		.vact_timing_fps = 120,
		.dfps_cmd_table[0] = {0, 4, {0xFF, 0x5A,0xA5,0x00} },
		.dfps_cmd_table[1] = {0, 2, {0x38, 0x00} },
		.dfps_cmd_table[2] = {0, 2, {0xD0, 0x20} },
	},
	/* following MIPI hopping parameter might cause screen mess */
	.dyn = {
		.switch_en = 0,
		.pll_clk = 595,
		.vfp_lp_dyn = 2454,
		.hfp = 100,
		.vfp = 41,
	},
};

static struct mtk_panel_params ext_params_mode_90 = {
	.data_rate = DATA_RATE,
	.cust_esd_check = 1,
	.esd_check_enable = 1,
	.lcm_esd_check_table[0] = {
		.cmd = 0x0a,
		.count = 1,
		.para_list[0] = 0x9c,
	},
	.lcm_cellid = {
		.panel_cellid_reg = 0xF2,
		.panel_cellid_len = 23,
		.panel_cellid_esd_dis = 1,
		.panel_cellid_read_max = 1,
		.panel_cellid_regdata_increase = 1,
		.page_cmd_always = 1,
		.page_table = {
			{0x39, 0x04, 0xFF, 0x5A, 0xA5, 0x08},
			{0x15, 0x02, 0xB0, 0x55},
			{0x39, 0x04, 0xFF, 0x5A, 0xA5, 0x00},
			{0x39, 0x03, 0xFD, 0x01, 0x81},
		},
		.page_post_table = {
			{0x39, 0x04, 0xFD, 0x00, 0x00, 0x00},
			{0x39, 0x04, 0xFF, 0x5A, 0xA5, 0x08},
			{0x39, 0x03, 0xB0, 0x01, 0x00},
			{0x39, 0x04, 0xFF, 0x5A, 0xA5, 0x00},
		},
	},

	.panel_ver = 1,
	//.panel_id = 0x515a0502,
	.panel_name = "boe_il97605a_vid_1080_2352",
	.panel_supplier = "boe",
	.lcm_index = 0,
	//.hbm_type = HBM_MODE_RAMPING,
	//.max_bl_level = 2047,
	.ssc_enable = 0,
	.lane_swap_en = 0,
	.lp_perline_en = 0,
	.physical_width_um = PHYSICAL_WIDTH,
	.physical_height_um = PHYSICAL_HEIGHT,
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
		.ext_pps_cfg = {
			.enable = 1,
			.rc_buf_thresh = il97605a_rc_buf_thresh,
			.range_min_qp = il97605a_range_min_qp,
			.range_max_qp = il97605a_range_max_qp,
			.range_bpg_ofs = il97605a_range_bpg_ofs,
		},
	},
	.lfr_enable = LFR_EN,
	.lfr_minimum_fps = MODE_60_FPS,
	.dyn_fps = {
		.switch_en = 1,
		.vact_timing_fps = 120,
		.dfps_cmd_table[0] = {0, 4, {0xFF, 0x5A,0xA5,0x00} },
		.dfps_cmd_table[1] = {0, 2, {0x38, 0x00} },
		.dfps_cmd_table[2] = {0, 2, {0xD0, 0x10} },

	},
	/* following MIPI hopping parameter might cause screen mess */
	.dyn = {
		.switch_en = 0,
		.pll_clk = 595,
		.vfp_lp_dyn = 2454,
		.hfp = 100,
		.vfp = 846,
	},
};

static struct mtk_panel_params ext_params_mode_120 = {
	.data_rate = DATA_RATE,
	.cust_esd_check = 1,
	.esd_check_enable = 1,
	.lcm_esd_check_table[0] = {
		.cmd = 0x0a,
		.count = 1,
		.para_list[0] = 0x9c,
	},
	.lcm_cellid = {
		.panel_cellid_reg = 0xF2,
		.panel_cellid_len = 23,
		.panel_cellid_esd_dis = 1,
		.panel_cellid_read_max = 1,
		.panel_cellid_regdata_increase = 1,
		.page_cmd_always = 1,
		.page_table = {
			{0x39, 0x04, 0xFF, 0x5A, 0xA5, 0x08},
			{0x15, 0x02, 0xB0, 0x55},
			{0x39, 0x04, 0xFF, 0x5A, 0xA5, 0x00},
			{0x39, 0x03, 0xFD, 0x01, 0x81},
		},
		.page_post_table = {
			{0x39, 0x04, 0xFD, 0x00, 0x00, 0x00},
			{0x39, 0x04, 0xFF, 0x5A, 0xA5, 0x08},
			{0x39, 0x03, 0xB0, 0x01, 0x00},
			{0x39, 0x04, 0xFF, 0x5A, 0xA5, 0x00},
		},
	},

	.panel_ver = 1,
	//.panel_id = 0x515a0502,
	.panel_name = "boe_il97605a_vid_1080_2352",
	.panel_supplier = "boe",
	.lcm_index = 0,
	//.hbm_type = HBM_MODE_RAMPING,
	//.max_bl_level = 2047,
	.ssc_enable = 0,
	.lane_swap_en = 0,
	.lp_perline_en = 0,
	.physical_width_um = PHYSICAL_WIDTH,
	.physical_height_um = PHYSICAL_HEIGHT,
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
		.ext_pps_cfg = {
			.enable = 1,
			.rc_buf_thresh = il97605a_rc_buf_thresh,
			.range_min_qp = il97605a_range_min_qp,
			.range_max_qp = il97605a_range_max_qp,
			.range_bpg_ofs = il97605a_range_bpg_ofs,
		},
	},
	.lfr_enable = LFR_EN,
	.lfr_minimum_fps = MODE_60_FPS,
	.dyn_fps = {
		.switch_en = 1,
		.vact_timing_fps = 120,
		.dfps_cmd_table[0] = {0, 4, {0xFF, 0x5A,0xA5,0x00} },
		.dfps_cmd_table[1] = {0, 2, {0x38, 0x00} },
		.dfps_cmd_table[2] = {0, 2, {0xD0, 0x00} },

	},
	/* following MIPI hopping parameter might cause screen mess */
	.dyn = {
		.switch_en = 0,
		.pll_clk = 595,
		.vfp_lp_dyn = 2454,
		.hfp = 100,
		.vfp = 41,
	},
};


static int boe_il97605a_setbacklight_cmdq(void *dsi, dcs_write_gce cb,
	void *handle, unsigned int level)
{
	static char bl_tb0[] = { 0x51, 0x3f, 0xff };

	pr_info("%s backlight = %d\n", __func__, level);

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
	struct boe_il97605a*ctx = panel_to_boe_il97605a(panel);

	if (!m)
		return ret;

	pr_info("%s:disp: mode fps=%d", __func__, drm_mode_vrefresh(m));
	if (drm_mode_vrefresh(m) == MODE_60_FPS)
		ext->params = &ext_params_mode_60;
	else if (drm_mode_vrefresh(m) == MODE_90_FPS)
		ext->params = &ext_params_mode_90;
	else if (drm_mode_vrefresh(m) == MODE_120_FPS)
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
	struct boe_il97605a *ctx = panel_to_boe_il97605a(panel);

	pr_info("%s\n", __func__);

	boe_il97605a_dcs_write_seq_static(ctx, 0xFF, 0x5A,0xA5,0x00);
	boe_il97605a_dcs_write_seq_static(ctx, 0x38, 0x00);
	boe_il97605a_dcs_write_seq_static(ctx, 0xD0, 0x00);//120hz
}

static void mode_switch_to_90(struct drm_panel *panel)
{
	struct boe_il97605a *ctx = panel_to_boe_il97605a(panel);

	pr_info("%s\n", __func__);

	boe_il97605a_dcs_write_seq_static(ctx, 0xFF, 0x5A,0xA5,0x00);
	boe_il97605a_dcs_write_seq_static(ctx, 0x38, 0x00);
	boe_il97605a_dcs_write_seq_static(ctx, 0xD0, 0x10);//90hz
}

static void mode_switch_to_60(struct drm_panel *panel)
{
	struct boe_il97605a *ctx = panel_to_boe_il97605a(panel);

	pr_info("%s\n", __func__);

	boe_il97605a_dcs_write_seq_static(ctx, 0xFF, 0x5A,0xA5,0x00);
	boe_il97605a_dcs_write_seq_static(ctx, 0x38, 0x00);
	boe_il97605a_dcs_write_seq_static(ctx, 0xD0, 0x20);//60hz
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
	struct boe_il97605a *ctx = panel_to_boe_il97605a(panel);

	ctx->reset_gpio =
		devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
		gpiod_set_value(ctx->reset_gpio, on);
		devm_gpiod_put(ctx->dev, ctx->reset_gpio);

	return 0;
}

/*
static enum mtk_lcm_version panel_get_lcm_version(void)
{
	return MTK_LEGACY_LCM_DRV_WITH_BACKLIGHTCLASS;
}
*/

static int panel_lhbm_set_cmdq(void *dsi, dcs_grp_write_gce cb, void *handle, uint32_t on, uint32_t bl_level, uint32_t fps)
{
	unsigned int para_count = 0;
	struct mtk_panel_para_table *pTable = NULL;

	pr_info("%s: bl_level:%d, fps:%d, on:%d\n", __func__, bl_level, fps, on);

	if (on) {
		para_count = sizeof(panel_lhbm_on) / sizeof(struct mtk_panel_para_table);
		pTable = panel_lhbm_on;
	} else {
		para_count = sizeof(panel_lhbm_off) / sizeof(struct mtk_panel_para_table);
		pTable = panel_lhbm_off;
	}
	cb(dsi, handle, pTable, para_count);
	return 0;
}

static int panel_hbm_set_cmdq(struct boe_il97605a *ctx, void *dsi, dcs_grp_write_gce cb, void *handle, uint32_t hbm_state)
{
	struct mtk_panel_para_table hbm_on_table = {3, {0x51, 0x3E, 0x80}};//set hbm code, on
	unsigned int level = 0;
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


static int panel_feature_get(struct drm_panel *panel, struct panel_param_info *param_info){

	struct boe_il97605a *ctx = panel_to_boe_il97605a(panel);
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
			//param_info->value = atomic_read(&ctx->dc_mode);
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
	struct boe_il97605a *ctx = panel_to_boe_il97605a(panel);
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
		default:
			pr_info("%s: skip unsupport feature %d to %d\n", __func__, param_info.param_idx, param_info.value);
			break;
	}

	return ret;
}

static struct mtk_panel_funcs ext_funcs = {
	.set_backlight_cmdq = boe_il97605a_setbacklight_cmdq,
	.reset = panel_ext_reset,
	.ext_param_set = mtk_panel_ext_param_set,
	.mode_switch = mode_switch,
//	.get_lcm_version = panel_get_lcm_version,
//	.ata_check = panel_ata_check,
	.set_gesture_flag = panel_set_gesture_flag,
	.panel_feature_set = panel_feature_set,
	.panel_feature_get = panel_feature_get,
};
#endif

static int boe_il97605a_get_modes(struct drm_panel *panel,
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

static const struct drm_panel_funcs boe_il97605a_drm_funcs = {
	.disable = boe_il97605a_disable,
	.unprepare = boe_il97605a_unprepare,
	.prepare = boe_il97605a_prepare,
	.enable = boe_il97605a_enable,
	.get_modes = boe_il97605a_get_modes,
};
#if 0
static void boe_il97605a_parse_panel_version(struct boe_il97605a*ctx)
{
#if TM_PANEL_EVT_V0_SUPPORT
	int rc;
	struct device_node *chosen = of_find_node_by_name(NULL, "chosen");

	ctx->version = PANEL_V1;
	if(chosen) {
		u32 txdp_id = 0;

		rc = of_property_read_u32(chosen, "mmi,panel_vendor_id", &txdp_id);
		if (!rc) {
			if (TM_ILI_PANEL_V0_VENDOR_ID == txdp_id) {
				ctx->version = PANEL_V0;
				pr_info("boe_il97605a panel version v0, ver=%d, vendor_id=0x%x\n", ctx->version, txdp_id);
			}
			else
				pr_info("boe_il97605a get vendor_id:0x%x\n", txdp_id);
		}
		else
			pr_info("boe_il97605a mmi,panel_vendor_id not get\n");
	}
	else
		pr_info("boe_il97605a_parse_panel_version: chosen node null\n");

	pr_info("parse boe_il97605a panel version:%d\n", ctx->version);
#endif

	return;
}
#endif

static int boe_il97605a_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct device_node *dsi_node, *remote_node = NULL, *endpoint = NULL;
	struct boe_il97605a *ctx;
	struct device_node *backlight;
	int ret;

	pr_info("%s+ disp:boe_il97605a_probe start!\n", __func__);

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

	ctx = devm_kzalloc(dev, sizeof(struct boe_il97605a), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	mipi_dsi_set_drvdata(dsi, ctx);

	ctx->dev = dev;
	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_SYNC_PULSE |
			MIPI_DSI_MODE_LPM | MIPI_DSI_MODE_NO_EOT_PACKET |
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

	ctx->dvdd_supply = devm_regulator_get_optional(dev, "dvdd");
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

	ctx->vci_supply = devm_regulator_get_optional(dev, "vci");
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

	ctx->prepared = true;
	ctx->enabled = true;

	drm_panel_init(&ctx->panel, dev, &boe_il97605a_drm_funcs, DRM_MODE_CONNECTOR_DSI);

	drm_panel_add(&ctx->panel);

	//parse panel version for evt/dvt
	//boe_il97605a_parse_panel_version(ctx);

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

	pr_info("[%d  %s]- txd,il97605a,vdo,120hz ret:%d\n", __LINE__, __func__,ret);

	return ret;
}

static int boe_il97605a_remove(struct mipi_dsi_device *dsi)
{
	struct boe_il97605a *ctx = mipi_dsi_get_drvdata(dsi);
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

static const struct of_device_id boe_il97605a_of_match[] = {
	{
		.compatible = "boe_il97605a_vid_1080_2352",
	},
	{}
};

MODULE_DEVICE_TABLE(of, boe_il97605a_of_match);

static struct mipi_dsi_driver boe_il97605a_driver = {
	.probe = boe_il97605a_probe,
	.remove = boe_il97605a_remove,
	.shutdown = lcm_shutdown,
	.driver = {
		.name = "boe_il97605a_vid_1080_2352",
		.owner = THIS_MODULE,
		.of_match_table = boe_il97605a_of_match,
	},
};

module_mipi_dsi_driver(boe_il97605a_driver);

MODULE_AUTHOR("mediatek");
MODULE_DESCRIPTION("boe il97605a incell 120hz Panel Driver");
MODULE_LICENSE("GPL v2");

