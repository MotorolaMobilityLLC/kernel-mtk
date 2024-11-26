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
#include "include/dsi-panel-mot-boe-icnl9922c-672-fhd-dphy-vdo-120hz.h"
#endif

#if IS_ENABLED(CONFIG_OEM_DEVINFO)
#include "../../../../oem/devinfo/dev_info.h"
#endif

/* option function to read data from some panel address */
/* #define PANEL_SUPPORT_READBACK */

#define BIAS_OCP2138

#ifdef BIAS_OCP2138
extern int __attribute__ ((weak)) ocp2138_BiasPower_disable(u32 pwrdown_delay);
extern int __attribute__ ((weak)) ocp2138_BiasPower_enable(u32 avdd, u32 avee,u32 pwrup_delay);
#endif

static int tp_gesture_flag = 0;

struct lcm {
	struct device *dev;
	struct drm_panel panel;
	struct backlight_device *backlight;
	struct gpio_desc *reset_gpio;
#ifndef BIAS_OCP2138
	struct gpio_desc *avdd_en_gpio;
	struct gpio_desc *avee_en_gpio;
#endif
	bool prepared;
	bool enabled;
	int error;
	unsigned int hbm_mode;
	unsigned int cabc_mode;
};

#if 1
static struct mtk_panel_para_table panel_cabc_ui[] = {
	{4, {0xF0, 0x99, 0x22, 0x0C}},
	{32,{0xE1, 0x0F, 0x1F, 0x2F, 0x3F, 0x4F, 0x5F, 0x6F, 0x7F, 0x90, 0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xE8, 0xEC, 0xED, 0xEF, 0xF1, 0xF8, 0xF8, 0xF8, 0xF9, 0xFB, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF, 0xFF, 0xFF}},
	{33,{0xE2, 0x36, 0x24, 0x37, 0x4B, 0x38, 0x72, 0x39, 0x99, 0x3D, 0x00, 0x3C, 0x0F, 0x3B, 0xC2, 0x3B, 0xA5, 0x3C, 0x28, 0x3C, 0xED, 0x3D, 0xB1, 0x3E, 0x76, 0x3F, 0x3A, 0x3F, 0xFF, 0x3F, 0xFF, 0x3F, 0xFF}},
	{4, {0xF0, 0x00, 0x00, 0x00}},
};

static struct mtk_panel_para_table panel_cabc_mv[] = {
	{4, {0xF0, 0x99, 0x22, 0x0C}},
	{32,{0xE1, 0x0F, 0x1F, 0x2F, 0x3F, 0x4F, 0x5F, 0x6F, 0x7F, 0x90, 0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xE8, 0xE7, 0xE7, 0xE7, 0xE7, 0xE9, 0xEB, 0xEC, 0xED, 0xF0, 0xF0, 0xF3, 0xF6, 0xF9, 0xFD, 0xFF, 0xFF}},
	{33,{0xE2, 0x2F, 0xFF, 0x2F, 0xFF, 0x2F, 0xFF, 0x2F, 0xFF, 0x31, 0x47, 0x32, 0x8F, 0x33, 0xD6, 0x35, 0x1E, 0x36, 0x66, 0x37, 0xEF, 0x39, 0x78, 0x3B, 0x01, 0x3C, 0x8A, 0x3E, 0x14, 0x3F, 0xFF, 0x3F, 0xFF}},
	{4, {0xF0, 0x00, 0x00, 0x00}},
};

static struct mtk_panel_para_table panel_cabc_disable[] = {
	{4, {0xF0, 0x99, 0x22, 0x0C}},
	{32,{0xE1, 0x0F, 0x1F, 0x2F, 0x3F, 0x4F, 0x5F, 0x6F, 0x7F, 0x90, 0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xE8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}},
	{33,{0xE2, 0x3F, 0xFF, 0x3F, 0xFF, 0x3F, 0xFF, 0x3F, 0xFF, 0x3F, 0xFF, 0x3F, 0xFF, 0x3F, 0xFF, 0x3F, 0xFF, 0x3F, 0xFF, 0x3F, 0xFF, 0x3F, 0xFF, 0x3F, 0xFF, 0x3F, 0xFF, 0x3F, 0xFF, 0x3F, 0xFF, 0x3F, 0xFF}},
	{4, {0xF0, 0x00, 0x00, 0x00}},
};
#endif

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
        pr_info("error %zd writing seq: %ph\n", ret, data);
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
		dev_info(ctx->dev, "error %d reading dcs seq:(%#x)\n", ret, cmd);
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

static void lcm_panel_init(struct lcm *ctx)
{
	pr_info("disp: %s+, boe_icnl9922c\n", __func__);

	lcm_dcs_write_seq_static(ctx, 0xF0, 0x99, 0x22, 0x0C);

	lcm_dcs_write_seq_static(ctx, 0x35, 0x00, 0x00);
	lcm_dcs_write_seq_static(ctx, 0x53, 0x2C);
	lcm_dcs_write_seq_static(ctx, 0x55, 0x03);
	msleep(5);

	lcm_dcs_write_seq_static(ctx, 0xE0, 0x0C, 0x00, 0xB0, 0x0C, 0x00, 0x15, 0x7C, 0x29, 0x04, 0x21, 0x01, 0x00, 0x00, 0x00, 0x00, 0x2E, 0x17, 0x0C, 0x98);

	lcm_dcs_write_seq_static(ctx, 0xE1, 0x0F, 0x1F, 0x2F, 0x3F, 0x4F, 0x5F, 0x6F, 0x7F, 0x90, 0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xE8, 0xEC, 0xED, 0xEF, 0xF1, 0xF8, 0xF8, 0xF8, 0xF9, 0xFB, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF, 0xFF, 0xFF);
	lcm_dcs_write_seq_static(ctx, 0xE2, 0x36, 0x24, 0x37, 0x4B, 0x38, 0x72, 0x39, 0x99, 0x3D, 0x00, 0x3C, 0x0F, 0x3B, 0xC2, 0x3B, 0xA5, 0x3C, 0x28, 0x3C, 0xED, 0x3D, 0xB1, 0x3E, 0x76, 0x3F, 0x3A, 0x3F, 0xFF, 0x3F, 0xFF, 0x3F, 0xFF);
	lcm_dcs_write_seq_static(ctx, 0xF0, 0x00, 0x00, 0x00);

	lcm_dcs_write_seq_static(ctx, 0x11);
	msleep(100);
	lcm_dcs_write_seq_static(ctx, 0x29);
	msleep(10);
	lcm_dcs_write_seq_static(ctx, 0x51, 0x07, 0xCF);
	lcm_dcs_write_seq_static(ctx, 0xAC, 0x05);

	pr_info("%s-\n", __func__);
}

static int lcm_disable(struct drm_panel *panel)
{
	struct lcm *ctx = panel_to_lcm(panel);
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

#if 1
static int panel_set_gesture_flag(int state)
{
	if(state == 1)
		tp_gesture_flag = 1;
	else
		tp_gesture_flag = 0;

	pr_info("%s:disp:set tp_gesture_flag:%d\n", __func__, tp_gesture_flag);
	return 0;
}
#endif

static int lcm_unprepare(struct drm_panel *panel)
{
	struct lcm *ctx = panel_to_lcm(panel);

	if (!ctx->prepared) {
		pr_info("%s, already unprepared, return\n", __func__);
		return 0;
	}
	pr_info("%s+, boe_icnl9922c\n", __func__);

	lcm_dcs_write_seq_static(ctx, 0x28);
	msleep(10);
	lcm_dcs_write_seq_static(ctx, 0x10);
	msleep(100);

	pr_info("%s:disp: tp_gesture_flag:%d\n",__func__, tp_gesture_flag);
	if(!tp_gesture_flag) {
#ifdef BIAS_OCP2138
		ocp2138_BiasPower_disable(5);
#else
		ctx->avee_en_gpio = devm_gpiod_get_index(ctx->dev, "avee", 0, GPIOD_OUT_HIGH);
		if (IS_ERR(ctx->avee_en_gpio)) {
			dev_info(ctx->dev, "[error]%s: cannot get avee_en_gpio 0 %ld\n", __func__, PTR_ERR(ctx->avee_en_gpio));
			return PTR_ERR(ctx->avee_en_gpio);
		}
		gpiod_set_value(ctx->avee_en_gpio, 0);
		devm_gpiod_put(ctx->dev, ctx->avee_en_gpio);
		msleep(5);

		ctx->avdd_en_gpio = devm_gpiod_get_index(ctx->dev, "avdd", 0, GPIOD_OUT_HIGH);
		if (IS_ERR(ctx->avdd_en_gpio)) {
			dev_info(ctx->dev, "[error]%s: cannot get avdd_en_gpio 1 %ld\n", __func__, PTR_ERR(ctx->avdd_en_gpio));
			return PTR_ERR(ctx->avdd_en_gpio);
		}
		gpiod_set_value(ctx->avdd_en_gpio, 0);
		devm_gpiod_put(ctx->dev, ctx->avdd_en_gpio);
#endif
	}

	ctx->error = 0;
	ctx->prepared = false;
	pr_info("%s-\n", __func__);
	return 0;
}

static int lcm_prepare(struct drm_panel *panel)
{
	struct lcm *ctx = panel_to_lcm(panel);
	int ret;

	pr_info("%s+, boe_icnl9922c\n", __func__);
	if (ctx->prepared) {
		pr_info("%s, already prepared, return\n", __func__);
		return 0;
	}

	ctx->reset_gpio = devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio)) {
		dev_info(ctx->dev, "[error]%s: cannot get reset_gpio %ld\n", __func__, PTR_ERR(ctx->reset_gpio));
		return PTR_ERR(ctx->reset_gpio);
	}
	gpiod_set_value(ctx->reset_gpio, 0);
	devm_gpiod_put(ctx->dev, ctx->reset_gpio);
	msleep(5);

#ifdef BIAS_OCP2138
	ocp2138_BiasPower_enable(15,15,5);
	msleep(1);
#else
	ctx->avdd_en_gpio = devm_gpiod_get_index(ctx->dev, "avdd", 0, GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->avdd_en_gpio)) {
		dev_info(ctx->dev, "[error]%s: cannot get avdd_en_gpio 1 %ld\n", __func__, PTR_ERR(ctx->avdd_en_gpio));
		return PTR_ERR(ctx->avdd_en_gpio);
	}
	gpiod_set_value(ctx->avdd_en_gpio, 1);
	devm_gpiod_put(ctx->dev, ctx->avdd_en_gpio);
	msleep(5);

	ctx->avee_en_gpio = devm_gpiod_get_index(ctx->dev, "avee", 0, GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->avee_en_gpio)) {
		dev_info(ctx->dev, "[error]%s: cannot get avee_en_gpio 0 %ld\n", __func__, PTR_ERR(ctx->avee_en_gpio));
		return PTR_ERR(ctx->avee_en_gpio);
	}
	gpiod_set_value(ctx->avee_en_gpio, 1);
	devm_gpiod_put(ctx->dev, ctx->avee_en_gpio);
	msleep(5);
#endif

	gpiod_set_value(ctx->reset_gpio, 1);
	msleep(5);
	gpiod_set_value(ctx->reset_gpio, 0);
	msleep(5);
	gpiod_set_value(ctx->reset_gpio, 1);
	msleep(15);

	lcm_panel_init(ctx);
	ctx->hbm_mode = 0;
	ctx->cabc_mode = 0;

	ret = ctx->error;
	if (ret < 0) {
		pr_info("disp: %s error ret=%d\n", __func__, ret);
		lcm_unprepare(panel);
	}

	ctx->prepared = true;

#ifdef PANEL_SUPPORT_READBACK
	lcm_panel_get_data(ctx);
#endif
	pr_info("disp: %s-\n", __func__);
	return ret;
}

static int lcm_enable(struct drm_panel *panel)
{
	struct lcm *ctx = panel_to_lcm(panel);

	pr_info("disp: %s+, boe_icnl9922c\n", __func__);
	if (ctx->enabled)
		return 0;

	if (ctx->backlight) {
		ctx->backlight->props.power = FB_BLANK_UNBLANK;
		backlight_update_status(ctx->backlight);
	}

	ctx->enabled = true;

	return 0;
}

static const struct drm_display_mode switch_mode_60hz = {
	.clock = (int)((FRAME_WIDTH + HFP + HSA + HBP) * (FRAME_HEIGHT + MODE_60_VFP + VSA + VBP) * MODE_60_FPS / 1000),
	.hdisplay = FRAME_WIDTH,
	.hsync_start = FRAME_WIDTH + HFP,
	.hsync_end = FRAME_WIDTH + HFP + HSA,
	.htotal = FRAME_WIDTH + HFP + HSA + HBP,
	.vdisplay = FRAME_HEIGHT,
	.vsync_start = FRAME_HEIGHT + MODE_60_VFP,
	.vsync_end = FRAME_HEIGHT + MODE_60_VFP + VSA,
	.vtotal = FRAME_HEIGHT + MODE_60_VFP + VSA + VBP,
};

static const struct drm_display_mode switch_mode_90hz = {
	.clock = (int)((FRAME_WIDTH + HFP + HSA + HBP) * (FRAME_HEIGHT + MODE_90_VFP + VSA + VBP) * MODE_90_FPS / 1000),
	.hdisplay = FRAME_WIDTH,
	.hsync_start = FRAME_WIDTH + HFP,
	.hsync_end = FRAME_WIDTH + HFP + HSA,
	.htotal = FRAME_WIDTH + HFP + HSA + HBP,
	.vdisplay = FRAME_HEIGHT,
	.vsync_start = FRAME_HEIGHT + MODE_90_VFP,
	.vsync_end = FRAME_HEIGHT + MODE_90_VFP + VSA,
	.vtotal = FRAME_HEIGHT + MODE_90_VFP + VSA + VBP,
};

static const struct drm_display_mode switch_mode_120hz = {
	.clock = (int)((FRAME_WIDTH + HFP + HSA + HBP) * (FRAME_HEIGHT + MODE_120_VFP + VSA + VBP) * MODE_120_FPS / 1000),
	.hdisplay = FRAME_WIDTH,
	.hsync_start = FRAME_WIDTH + HFP,
	.hsync_end = FRAME_WIDTH + HFP + HSA,
	.htotal = FRAME_WIDTH + HFP + HSA + HBP,
	.vdisplay = FRAME_HEIGHT,
	.vsync_start = FRAME_HEIGHT + MODE_120_VFP,
	.vsync_end = FRAME_HEIGHT + MODE_120_VFP + VSA,
	.vtotal = FRAME_HEIGHT + MODE_120_VFP + VSA + VBP,
};

#if defined(CONFIG_MTK_PANEL_EXT)
/*
static int panel_ata_check(struct drm_panel *panel)
{
	struct lcm *ctx = panel_to_lcm(panel);
	struct mipi_dsi_device *dsi = to_mipi_dsi_device(ctx->dev);
	unsigned char data[3] = {0x00, 0x00, 0x00};
	unsigned char id[3] = {0x00, 0x80, 0x00};
	ssize_t ret;

	ret = mipi_dsi_dcs_read(dsi, 0x4, data, 3);
	if (ret < 0) {
		pr_info("%s error\n", __func__);
		return 0;
	}

	pr_info("ATA read data %x %x %x\n", data[0], data[1], data[2]);

	if (data[0] == id[0] &&
			data[1] == id[1] &&
			data[2] == id[2])
		return 1;

	pr_info("ATA expect read data is %x %x %x\n",
			id[0], id[1], id[2]);

	return 0;
}
*/

static int lcm_setbacklight_cmdq(void *dsi, dcs_write_gce cb,
	void *handle, unsigned int level)
{
	pr_info("%s: skip for using bl ic, level=%d\n", __func__, level);
	return 0;
}

static struct mtk_panel_params ext_params_60hz = {
	.data_rate = DATA_RATE,
	.ssc_enable = 0,
	.cust_esd_check = 1,
	.esd_check_enable = 1,
	.lcm_esd_check_table[0] = {
		.cmd = 0x0A, .count = 1, .para_list[0] = 0x9C,
	},
	.panel_ver = 1,
	.panel_id = 0x10050753,
	.panel_name = "boe_icnl9922c_vid_672_1080",
	.panel_supplier = "boe",
	.lcm_index = 0,
	.hbm_type = HBM_MODE_DCS_I2C,
	.max_bl_level = 2047,
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
	},
};

static struct mtk_panel_params ext_params_90hz = {
	.data_rate = DATA_RATE,
	.ssc_enable = 0,
	.cust_esd_check = 1,
	.esd_check_enable = 1,
	.lcm_esd_check_table[0] = {
		.cmd = 0x0A, .count = 1, .para_list[0] = 0x9C,
	},
	.panel_ver = 1,
	.panel_id = 0x10050753,
	.panel_name = "boe_icnl9922c_vid_672_1080",
	.panel_supplier = "boe",
	.lcm_index = 0,
	.hbm_type = HBM_MODE_DCS_I2C,
	.max_bl_level = 2047,
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
	},
};

static struct mtk_panel_params ext_params_120hz = {
	.data_rate = DATA_RATE,
	.ssc_enable = 0,
	.cust_esd_check = 1,
	.esd_check_enable = 1,
	.lcm_esd_check_table[0] = {
		.cmd = 0x0A, .count = 1, .para_list[0] = 0x9C,
	},
	.panel_ver = 1,
	.panel_id = 0x10050753,
	.panel_name = "boe_icnl9922c_vid_672_1080",
	.panel_supplier = "boe",
	.lcm_index = 0,
	.hbm_type = HBM_MODE_DCS_I2C,
	.max_bl_level = 2047,
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
	},
};

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

	if (!m)
		return ret;

	pr_info("%s:disp: mode fps=%d", __func__, drm_mode_vrefresh(m));
	if (drm_mode_vrefresh(m) == MODE_120_FPS)
		ext->params = &ext_params_120hz;
	else if (drm_mode_vrefresh(m) == MODE_90_FPS)
		ext->params = &ext_params_90hz;
	else if (drm_mode_vrefresh(m) == MODE_60_FPS)
		ext->params = &ext_params_60hz;
	else
		ret = 1;

	return ret;
}

static int panel_ext_reset(struct drm_panel *panel, int on)
{
	struct lcm *ctx = panel_to_lcm(panel);
	pr_info("[LCM] %s begin\n", __func__);

	ctx->reset_gpio = devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
	gpiod_set_value(ctx->reset_gpio, on);
	devm_gpiod_put(ctx->dev, ctx->reset_gpio);

	return 0;
}

static enum mtk_lcm_version panel_get_lcm_version(void)
{
	return MTK_LEGACY_LCM_DRV_WITH_BACKLIGHTCLASS;
}

#if 1
static int panel_cabc_set_cmdq(struct lcm *ctx, void *dsi, dcs_grp_write_gce cb, void *handle, uint32_t cabc_mode)
{
	unsigned int para_count = 0;
	struct mtk_panel_para_table *pTable = NULL;

	if (cabc_mode > 3) {
		pr_info("%s: invalid CABC mode:%d, return\n", __func__, cabc_mode);
		return -1;
	}

	switch (cabc_mode) {
		case 0:
			para_count = sizeof(panel_cabc_ui) / sizeof(struct mtk_panel_para_table);
			pTable = panel_cabc_ui;
			break;
		case 1:
			para_count = sizeof(panel_cabc_mv) / sizeof(struct mtk_panel_para_table);
			pTable = panel_cabc_mv;
			break;
		case 2:
			para_count = sizeof(panel_cabc_disable) / sizeof(struct mtk_panel_para_table);
			pTable = panel_cabc_disable;
			break;
		default:
			break;
	}

	if (pTable) {
		pr_info("%s: set CABC mode :%d", __func__, cabc_mode);
		cb(dsi, handle, pTable, para_count);
	}
	else
		pr_info("%s: CABC mode:%d not support", __func__, cabc_mode);

	return 0;
}

static int panel_feature_set(struct drm_panel *panel, void *dsi,
			      dcs_grp_write_gce cb, void *handle, struct panel_param_info param_info)
{
	struct lcm *ctx = panel_to_lcm(panel);
	int ret = -1;

	if (!cb) {
		pr_info("%s: cb NULL\n", __func__);
		return -1;
	}

	if (!ctx->enabled) {
		pr_info("%s: skip set feature %d to %d, panel not enabled\n", __func__, param_info.param_idx, param_info.value);
		return -1;
	}

	pr_info("%s: start set feature %d to %d\n", __func__, param_info.param_idx, param_info.value);
	switch (param_info.param_idx) {
		case PARAM_CABC:
			if (ctx->cabc_mode != param_info.value) {
				ctx->cabc_mode = param_info.value;
				panel_cabc_set_cmdq(ctx, dsi, cb, handle, param_info.value);
				pr_info("%s: set CABC to %d end\n", __func__, param_info.value);
				ret = 0;
			}
			else
				pr_info("%s: skip same CABC mode:%d\n", __func__, ctx->cabc_mode);
			break;
		case PARAM_HBM:
			/*if (ctx->hbm_mode != param_info.value) {
				ctx->hbm_mode = param_info.value;
				panel_hbm_set_cmdq(ctx, dsi, cb, handle, param_info.value);
				pr_debug("%s: set HBM to %d end\n", __func__, param_info.value);
				ret = 0;
			}
			else*/
				pr_info("%s: skip same HBM mode:%d\n", __func__, param_info.value);
			break;
		default:
			pr_info("%s: skip unsupport feature %d to %d\n", __func__, param_info.param_idx, param_info.value);
			break;
	}

	pr_debug("%s: set feature %d to %d, ret %d\n", __func__, param_info.param_idx, param_info.value, ret);
	return ret;
}
#endif

static struct mtk_panel_funcs ext_funcs = {
	.reset = panel_ext_reset,
	.set_backlight_cmdq = lcm_setbacklight_cmdq,
	.ext_param_set = mtk_panel_ext_param_set,
	.get_lcm_version = panel_get_lcm_version,
//	.ata_check = panel_ata_check,
	.set_gesture_flag = panel_set_gesture_flag,
	.panel_feature_set = panel_feature_set,
};
#endif

static int lcm_get_modes(struct drm_panel *panel, struct drm_connector *connector)
{
	struct drm_display_mode *mode_60hz;
	struct drm_display_mode *mode_90hz;
	struct drm_display_mode *mode_120hz;
	pr_info("[LCM] %s begin\n", __func__);

	mode_60hz = drm_mode_duplicate(connector->dev, &switch_mode_60hz);
	if (!mode_60hz) {
		dev_info(connector->dev->dev, "failed to add mode %ux%ux@%u\n",
			switch_mode_60hz.hdisplay, switch_mode_60hz.vdisplay,
			drm_mode_vrefresh(&switch_mode_60hz));
		return -ENOMEM;
	}
	drm_mode_set_name(mode_60hz);
	mode_60hz->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	drm_mode_probed_add(connector, mode_60hz);

	mode_90hz = drm_mode_duplicate(connector->dev, &switch_mode_90hz);
	if (!mode_90hz) {
		dev_info(connector->dev->dev, "failed to add mode %ux%ux@%u\n",
			switch_mode_90hz.hdisplay, switch_mode_90hz.vdisplay,
			drm_mode_vrefresh(&switch_mode_90hz));
		return -ENOMEM;
	}
	drm_mode_set_name(mode_90hz);
	mode_90hz->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	drm_mode_probed_add(connector, mode_90hz);

	mode_120hz = drm_mode_duplicate(connector->dev, &switch_mode_120hz);
	if (!mode_120hz) {
		dev_info(connector->dev->dev, "failed to add mode %ux%ux@%u\n",
			switch_mode_120hz.hdisplay, switch_mode_120hz.vdisplay,
			drm_mode_vrefresh(&switch_mode_120hz));
		return -ENOMEM;
	}
	drm_mode_set_name(mode_120hz);
	mode_120hz->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	drm_mode_probed_add(connector, mode_120hz);

	connector->display_info.width_mm = 68;
	connector->display_info.height_mm = 152;
	printk("[%d  %s]end\n",__LINE__, __FUNCTION__);

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
	struct device_node *dsi_node, *remote_node = NULL, *endpoint = NULL;
	struct lcm *ctx;
	struct device_node *backlight;
	int ret;

	pr_info("%s+ disp:boe,icnl9922c,672,vdo\n", __func__);

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

	ctx = devm_kzalloc(dev, sizeof(struct lcm), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	mipi_dsi_set_drvdata(dsi, ctx);

	ctx->dev = dev;
	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_SYNC_PULSE;

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

#ifndef BIAS_OCP2138
	ctx->avdd_en_gpio = devm_gpiod_get_index(dev, "avdd", 0, GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->avdd_en_gpio)) {
		dev_info(ctx->dev, "[error]%s: cannot get avdd_en_gpio 0 %ld\n", __func__, PTR_ERR(ctx->avdd_en_gpio));
		return PTR_ERR(ctx->avdd_en_gpio);
	}
	devm_gpiod_put(ctx->dev, ctx->avdd_en_gpio);

	ctx->avee_en_gpio = devm_gpiod_get_index(dev, "avee", 0, GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->avee_en_gpio)) {
		dev_info(ctx->dev, "[error]%s: cannot get avee_en_gpio 0 %ld\n", __func__, PTR_ERR(ctx->avee_en_gpio));
		return PTR_ERR(ctx->avee_en_gpio);
	}
	devm_gpiod_put(ctx->dev, ctx->avee_en_gpio);
#endif

	ctx->prepared = true;
	ctx->enabled = true;

	drm_panel_init(&ctx->panel, dev, &lcm_drm_funcs, DRM_MODE_CONNECTOR_DSI);
	ctx->panel.dev = dev;
	ctx->panel.funcs = &lcm_drm_funcs;

	drm_panel_add(&ctx->panel);

	ret = mipi_dsi_attach(dsi);
	if (ret < 0)
		drm_panel_remove(&ctx->panel);

#if defined(CONFIG_MTK_PANEL_EXT)
	ret = mtk_panel_ext_create(dev, &ext_params_120hz, &ext_funcs, &ctx->panel);
	if (ret < 0)
		return ret;
#endif

#if IS_ENABLED(CONFIG_OEM_DEVINFO)
	FULL_PRODUCT_DEVICE_INFO(ID_LCD, "ICNL9922C_FHDPLUS_DSI_VDO_BOE");
#endif
	pr_info("[%d  %s]- boe,icnl9922c,672,vdo,120hz ret:%d  \n", __LINE__, __func__,ret);

	return ret;
}

static int lcm_remove(struct mipi_dsi_device *dsi)
{
	struct lcm *ctx = mipi_dsi_get_drvdata(dsi);

#if defined(CONFIG_MTK_PANEL_EXT)
	struct mtk_panel_ctx *ext_ctx = find_panel_ctx(&ctx->panel);
#endif
	pr_info("[LCM] %s begin, boe_icnl9922c\n", __func__);
	mipi_dsi_detach(dsi);
	drm_panel_remove(&ctx->panel);
#if defined(CONFIG_MTK_PANEL_EXT)
	mtk_panel_detach(ext_ctx);
	mtk_panel_remove(ext_ctx);
#endif
	pr_info("[LCM] %s end\n", __func__);

	return 0;
}

static const struct of_device_id boe_of_match[] = {
	{
		.compatible = "boe,icnl9922c,672,vdo,120hz",
	},
	{}
};

MODULE_DEVICE_TABLE(of, boe_of_match);

static struct mipi_dsi_driver boe_driver = {
	.probe = lcm_probe,
	.remove = lcm_remove,
	.driver = {
		.name = "boe_icnl9922c_vid_672_1080",
		.owner = THIS_MODULE,
		.of_match_table = boe_of_match,
	},
};

module_mipi_dsi_driver(boe_driver);

MODULE_AUTHOR("mediatek");
MODULE_DESCRIPTION("boe icnl9922c incell 120hz Panel Driver");
MODULE_LICENSE("GPL v2");

