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
#include <drm/mediatek_drm.h>

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

#include "dsi-panel-mot-tm-vtdr6115-655-fhdp-video-144hz-lhbm-alpha.h"

#define SUPPORT_144HZ_REFRESH_RATE
//#define DSC_10BIT
#ifdef DSC_10BIT
#define DSC_BITS	10
#define DSC_CFG		40//2088//40
#else
#define DSC_BITS	8
#define DSC_CFG		34
#endif

enum panel_version{
	PANEL_V1 = 1,
	PANEL_V2,
};

struct lcm {
	struct device *dev;
	struct drm_panel panel;
	struct backlight_device *backlight;
	struct gpio_desc *reset_gpio;
	bool prepared;
	bool enabled;
	bool lhbm_en;

	int error;
	unsigned int hbm_mode;
	unsigned int dc_mode;
	unsigned int current_bl;
	unsigned int current_fps;
	enum panel_version version;
};

static struct lcm *tm_ctx = NULL;

#define lcm_dcs_write_seq(ctx, seq...)                                         \
	({                                                                     \
		const u8 d[] = { seq };                                        \
		BUILD_BUG_ON_MSG(ARRAY_SIZE(d) > 64,                           \
				 "DCS sequence too big for stack");            \
		lcm_dcs_write(ctx, d, ARRAY_SIZE(d));                          \
	})

#define lcm_dcs_write_seq_static(ctx, seq...)  \
({\
	static const u8 d[] = { seq };\
	lcm_dcs_write(ctx, d, ARRAY_SIZE(d));\
})

static int panel_ext_init_power(struct drm_panel *panel);
static int panel_ext_powerdown(struct drm_panel *panel);

static inline struct lcm *panel_to_lcm(struct drm_panel *panel)
{
	return container_of(panel, struct lcm, panel);
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
		dev_info(ctx->dev, "error %d reading dcs seq:(%#x)\n", ret,
			 cmd);
		ctx->error = ret;
	}

	return ret;
}

static void lcm_panel_get_data(struct lcm *ctx)
{
	u8 buffer[3] = { 0 };
	static int ret;

	pr_info("%s+\n", __func__);

	if (ret == 0) {
		ret = lcm_dcs_read(ctx, 0x0A, buffer, 1);
		pr_info("%s  0x%08x\n", __func__, buffer[0] | (buffer[1] << 8));
		dev_info(ctx->dev, "return %d data(0x%08x) to dsi engine\n",
			ret, buffer[0] | (buffer[1] << 8));
	}
}
#endif

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

static void lcm_panel_init(struct lcm *ctx)
{
	pr_info("%s+\n", __func__);
	lcm_dcs_write_seq_static(ctx, 0x03, 0x01);
	lcm_dcs_write_seq_static(ctx, 0x35, 0x00);
	lcm_dcs_write_seq_static(ctx, 0x53, 0x20);
	lcm_dcs_write_seq_static(ctx, 0x51, 0x00, 0x00);
	lcm_dcs_write_seq_static(ctx, 0x59, 0x09);
	lcm_dcs_write_seq_static(ctx, 0x6f, 0x01);
	lcm_dcs_write_seq_static(ctx, 0x6c, 0x00);
	lcm_dcs_write_seq_static(ctx, 0x62, 0x00);
	lcm_dcs_write_seq_static(ctx, 0x6d, 0x00);
#ifdef DSC_10BIT
	lcm_dcs_write_seq_static(ctx, 0x70, 0x11,0x00,0x00,0xab,0x30,0x80,0x09,0x60,0x04,0x38,0x00,0x0C,0x02,0x1C,0x02,0x1C,
										0x02,0x00,0x02,0x0E,0x00,0x20,0x01,0x1F,0x00,0x07,0x00,0x0C,0x08,0xBB,0x08,0x7A,
										0x18,0x00,0x10,0xF0,0x07,0x10,0x20,0x00,0x06,0x0f,0x0f,0x33,0x0E,0x1C,0x2A,0x38,
										0x46,0x54,0x62,0x69,0x70,0x77,0x79,0x7B,0x7D,0x7E,0x02,0x02,0x22,0x00,0x2a,0x40,
										0x2a,0xBE,0x3a,0xFC,0x3a,0xFA,0x3a,0xF8,0x3b,0x38,0x3b,0x78,0x3b,0xB6,0x4b,0xB6,
										0x4b,0xF4,0x4b,0xF4,0x6c,0x34,0x84,0x74,0x00,0x00,0x00,0x00,0x00,0x00);
#else
	lcm_dcs_write_seq_static(ctx, 0x70, 0x11,0x00,0x00,0x89,0x30,0x80,0x09,0x60,0x04,0x38,0x00,0x0C,0x02,0x1C,0x02,0x1C,
										0x02,0x00,0x02,0x0E,0x00,0x20,0x01,0x1F,0x00,0x07,0x00,0x0C,0x08,0xBB,0x08,0x7A,
										0x18,0x00,0x10,0xF0,0x03,0x0C,0x20,0x00,0x06,0x0B,0x0B,0x33,0x0E,0x1C,0x2A,0x38,
										0x46,0x54,0x62,0x69,0x70,0x77,0x79,0x7B,0x7D,0x7E,0x01,0x02,0x01,0x00,0x09,0x40,
										0x09,0xBE,0x19,0xFC,0x19,0xFA,0x19,0xF8,0x1A,0x38,0x1A,0x78,0x1A,0xB6,0x2A,0xB6,
										0x2A,0xF4,0x2A,0xF4,0x4B,0x34,0x63,0x74,0x00,0x00,0x00,0x00,0x00,0x00);
#endif
	lcm_dcs_write_seq_static(ctx, 0xf0, 0xaa, 0x10);
	lcm_dcs_write_seq_static(ctx, 0xd0, 0x84,0x35,0x50,0x14,0x20,0x00,0x29,0x07,0x1d,0x19,0x00,0x00,0x07,0x1d,0x1a,0x00,
										0x00,0x0b,0x05,0x05,0x1d,0x1d);

	lcm_dcs_write_seq_static(ctx, 0xff, 0x5a, 0x80);
	lcm_dcs_write_seq_static(ctx, 0x65, 0x0e);
	lcm_dcs_write_seq_static(ctx, 0xf9, 0xb9);

	lcm_dcs_write_seq_static(ctx, 0xff, 0x5a, 0x81);
	lcm_dcs_write_seq_static(ctx, 0x65, 0x08);
	lcm_dcs_write_seq_static(ctx, 0xf6, 0x51, 0x44);

	lcm_dcs_write_seq_static(ctx, 0xff, 0x5a, 0x81);
	lcm_dcs_write_seq_static(ctx, 0x65, 0x03);
	lcm_dcs_write_seq_static(ctx, 0xf4, 0x03);

    /* Sleep Out */
    lcm_dcs_write_seq_static(ctx, 0x11);
    msleep(75);

	lcm_dcs_write_seq_static(ctx, 0xff, 0x5a, 0x81);
	lcm_dcs_write_seq_static(ctx, 0xf3, 0x08);

	lcm_dcs_write_seq_static(ctx, 0xff, 0x5a, 0x00);
	lcm_dcs_write_seq_static(ctx, 0xf0, 0xaa, 0x00);

    /* Display On */
    lcm_dcs_write_seq_static(ctx, 0x29);

	pr_info("%s tm\n", __func__);
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

static int gate_ic_Power_on(struct drm_panel *panel, int enabled)
{
	struct lcm *ctx = panel_to_lcm(panel);
	bool gpio_status;
	struct gpio_desc *pm_en_pin;
	int i;

	gpio_status = enabled ? 1:0;
	if (gpio_status) {
		for (i=0; i < 3; i++) {
			pm_en_pin = NULL;
			pm_en_pin = devm_gpiod_get_index(ctx->dev, "pm-enable", i, GPIOD_OUT_LOW);
			if (IS_ERR(pm_en_pin)) {
				pr_err("cannot get bias-gpios[%d] %ld\n", i, PTR_ERR(pm_en_pin));
				return PTR_ERR(pm_en_pin);
			}
			gpiod_set_value(pm_en_pin, gpio_status);
			devm_gpiod_put(ctx->dev, pm_en_pin);
			usleep_range(1000, 1001);
		}
	} else {
		for (i=2; i >= 0; i--) {
			pm_en_pin = NULL;
			pm_en_pin = devm_gpiod_get_index(ctx->dev, "pm-enable", i, GPIOD_OUT_LOW);
			if (IS_ERR(pm_en_pin)) {
				pr_err("cannot get bias-gpios[%d] %ld\n", i, PTR_ERR(pm_en_pin));
				return PTR_ERR(pm_en_pin);
			}
			gpiod_set_value(pm_en_pin, gpio_status);
			devm_gpiod_put(ctx->dev, pm_en_pin);
			usleep_range(1000, 1001);
		}
	}
	return 0;
}


static int lcm_unprepare(struct drm_panel *panel)
{
	struct lcm *ctx = panel_to_lcm(panel);
	int ret;


	pr_info("%s+\n", __func__);
	if (!ctx->prepared)
		return 0;

	lcm_dcs_write_seq_static(ctx, MIPI_DCS_SET_DISPLAY_OFF);
	lcm_dcs_write_seq_static(ctx, MIPI_DCS_ENTER_SLEEP_MODE);
	msleep(120);

	ctx->error = 0;
	ctx->prepared = false;

	ret = panel_ext_powerdown(panel);

	return ret;
}

static int lcm_prepare(struct drm_panel *panel)
{
	struct lcm *ctx = panel_to_lcm(panel);
	int ret;

	pr_info("%s+\n", __func__);
	if (ctx->prepared)
		return 0;

	ctx->hbm_mode = 0;
	ctx->dc_mode = 0;

	ret = panel_ext_init_power(panel);
	if (ret < 0) goto error;

	// lcd reset L->H -> L -> L
	ctx->reset_gpio = devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_LOW);
	gpiod_set_value(ctx->reset_gpio, 0);
	usleep_range(2000, 2001);
	gpiod_set_value(ctx->reset_gpio, 1);
	usleep_range(20000, 20001);
	devm_gpiod_put(ctx->dev, ctx->reset_gpio);
	// end

	lcm_panel_init(ctx);

	ret = ctx->error;
	if (ret < 0) goto error;

	ctx->prepared = true;
#ifdef PANEL_SUPPORT_READBACK
	lcm_panel_get_data(ctx);
#endif

	pr_info("%s-\n", __func__);
	return ret;
error:
	lcm_unprepare(panel);
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


#define HFP (32)
#define HSA (32)
#define HBP (32)
#define VFP (3480)
#define VSA (4)
#define VBP (20)
#define HACT (1080)
#define VACT (2400)
#define PLL_CLOCK (416586)	//1176*2460*144

static const struct drm_display_mode switch_mode_144hz = {
	.clock		= PLL_CLOCK,
	.hdisplay	= HACT,
	.hsync_start	= HACT + HFP,
	.hsync_end	= HACT + HFP + HSA,
	.htotal		= HACT + HFP + HSA + HBP,
	.vdisplay	= VACT,
	.vsync_start	= VACT + 36,
	.vsync_end	= VACT + 36 + VSA,
	.vtotal		= VACT + 36 + VSA + VBP,
};

static const struct drm_display_mode switch_mode_120hz = {
	.clock		= PLL_CLOCK,
	.hdisplay	= HACT,
	.hsync_start	= HACT + HFP,
	.hsync_end	= HACT + HFP + HSA,
	.htotal		= HACT + HFP + HSA + HBP,
	.vdisplay	= VACT,
	.vsync_start	= VACT + 528,
	.vsync_end	= VACT + 528 + VSA,
	.vtotal		= VACT + 528 + VSA + VBP,
};

static const struct drm_display_mode switch_mode_90hz = {
	.clock		= PLL_CLOCK,
	.hdisplay	= HACT,
	.hsync_start	= HACT + HFP,
	.hsync_end	= HACT + HFP + HSA,
	.htotal		= HACT + HFP + HSA + HBP,
	.vdisplay	= VACT,
	.vsync_start	= VACT + 1512,
	.vsync_end	= VACT + 1512 + VSA,
	.vtotal		= VACT + 1512 + VSA + VBP,
};

static const struct drm_display_mode switch_mode_60hz = {
	.clock = PLL_CLOCK,
	.hdisplay	= HACT,
	.hsync_start	= HACT + HFP,
	.hsync_end	= HACT + HFP + HSA,
	.htotal		= HACT + HFP + HSA + HBP,
	.vdisplay	= VACT,
	.vsync_start	= VACT + 3480,
	.vsync_end	= VACT + 3480 + VSA,
	.vtotal		= VACT + 3480 + VSA + VBP,
};

#if defined(CONFIG_MTK_PANEL_EXT)
static struct mtk_panel_params ext_params_60hz = {
	.cust_esd_check = 0,
	.esd_check_enable = 1,
	.lcm_esd_check_table[0] = {
		.cmd = 0x0a,
		.count = 1,
		.para_list[0] = 0x9c,
	},
	.lcm_color_mode = MTK_DRM_COLOR_MODE_DISPLAY_P3,
	.physical_width_um = 68256,
	.physical_height_um = 151680,
	.lcm_index = 1,

	.output_mode = MTK_PANEL_DSC_SINGLE_PORT,
	.dsc_params = {
		.enable = 1,
		.ver = 17,
		.slice_mode = 1,
		.rgb_swap = 0,
		.dsc_cfg = DSC_CFG,
		.rct_on = 1,
		.bit_per_channel = DSC_BITS,
		.dsc_line_buf_depth = 9,
		.bp_enable = 1,
		.bit_per_pixel = 128,
		.pic_height = 2400,
		.pic_width = 1080,
		.slice_height = 12,
		.slice_width = 540,
		.chunk_size = 540,
		.xmit_delay = 512,
		.dec_delay = 526,
		.scale_value = 32,
		.increment_interval = 287,
		.decrement_interval = 7,
		.line_bpg_offset = 12,
		.nfl_bpg_offset = 2235,
		.slice_bpg_offset = 2170,
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

		.rc_buf_thresh[ 0] = 14,
		.rc_buf_thresh[ 1] = 28,
		.rc_buf_thresh[ 2] = 42,
		.rc_buf_thresh[ 3] = 56,
		.rc_buf_thresh[ 4] = 70,
		.rc_buf_thresh[ 5] = 84,
		.rc_buf_thresh[ 6] = 98,
		.rc_buf_thresh[ 7] = 105,
		.rc_buf_thresh[ 8] = 112,
		.rc_buf_thresh[ 9] = 119,
		.rc_buf_thresh[10] = 121,
		.rc_buf_thresh[11] = 123,
		.rc_buf_thresh[12] = 125,
		.rc_buf_thresh[13] = 126,
		.rc_range_parameters[ 0].range_min_qp = 0,
		.rc_range_parameters[ 0].range_max_qp = 4,
		.rc_range_parameters[ 0].range_bpg_offset = 2,
		.rc_range_parameters[ 1].range_min_qp = 0,
		.rc_range_parameters[ 1].range_max_qp = 4,
		.rc_range_parameters[ 1].range_bpg_offset = 0,
		.rc_range_parameters[ 2].range_min_qp = 1,
		.rc_range_parameters[ 2].range_max_qp = 5,
		.rc_range_parameters[ 2].range_bpg_offset = 0,
		.rc_range_parameters[ 3].range_min_qp = 1,
		.rc_range_parameters[ 3].range_max_qp = 6,
		.rc_range_parameters[ 3].range_bpg_offset = -2,
		.rc_range_parameters[ 4].range_min_qp = 3,
		.rc_range_parameters[ 4].range_max_qp = 7,
		.rc_range_parameters[ 4].range_bpg_offset = -4,
		.rc_range_parameters[ 5].range_min_qp = 3,
		.rc_range_parameters[ 5].range_max_qp = 7,
		.rc_range_parameters[ 5].range_bpg_offset = -6,
		.rc_range_parameters[ 6].range_min_qp = 3,
		.rc_range_parameters[ 6].range_max_qp = 7,
		.rc_range_parameters[ 6].range_bpg_offset = -8,
		.rc_range_parameters[ 7].range_min_qp = 3,
		.rc_range_parameters[ 7].range_max_qp = 8,
		.rc_range_parameters[ 7].range_bpg_offset = -8,
		.rc_range_parameters[ 8].range_min_qp = 3,
		.rc_range_parameters[ 8].range_max_qp = 9,
		.rc_range_parameters[ 8].range_bpg_offset = -8,
		.rc_range_parameters[ 9].range_min_qp = 3,
		.rc_range_parameters[ 9].range_max_qp = 10,
		.rc_range_parameters[ 9].range_bpg_offset = -10,
		.rc_range_parameters[10].range_min_qp = 5,
		.rc_range_parameters[10].range_max_qp = 10,
		.rc_range_parameters[10].range_bpg_offset = -10,
		.rc_range_parameters[11].range_min_qp = 5,
		.rc_range_parameters[11].range_max_qp = 11,
		.rc_range_parameters[11].range_bpg_offset = -12,
		.rc_range_parameters[12].range_min_qp = 5,
		.rc_range_parameters[12].range_max_qp = 11,
		.rc_range_parameters[12].range_bpg_offset = -12,
		.rc_range_parameters[13].range_min_qp = 9,
		.rc_range_parameters[13].range_max_qp = 12,
		.rc_range_parameters[13].range_bpg_offset = -12,
		.rc_range_parameters[14].range_min_qp = 12,
		.rc_range_parameters[14].range_max_qp = 13,
		.rc_range_parameters[14].range_bpg_offset = -12,
	},

	.max_bl_level = 3514,
	.hbm_type = HBM_MODE_DCS_ONLY,
	//.te_delay = 1,

	.dyn = {
		.switch_en = 0,
		//.pll_clk = 600,
		.vfp_lp_dyn = 3480,
		.hfp = 60,
		.vfp = 3480,
	},
	.dyn_fps = {
		.switch_en = 1,
		.dfps_cmd_grp_table[0] = {2, {0x6c, 0x00} },
		.dfps_cmd_grp_table[1] = {2, {0x62, 0x00} },
		.dfps_cmd_grp_size = 2,
	},
	.data_rate = 1156,
	//.lfr_enable = 1,
	//.lfr_minimum_fps = 60,
	.change_fps_by_vfp_send_cmd = 1,
	.use_ext_panel_feature = 1,
};

static struct mtk_panel_params ext_params_90hz = {

	.cust_esd_check = 0,
	.esd_check_enable = 1,
	.lcm_esd_check_table[0] = {
		.cmd = 0x0a,
		.count = 1,
		.para_list[0] = 0x9c,
	},
	.lcm_color_mode = MTK_DRM_COLOR_MODE_DISPLAY_P3,
	.physical_width_um = 68256,
	.physical_height_um = 151680,
	.lcm_index = 1,

	.output_mode = MTK_PANEL_DSC_SINGLE_PORT,
	.dsc_params = {
		.enable = 1,
		.ver = 17,
		.slice_mode = 1,
		.rgb_swap = 0,
		.dsc_cfg = DSC_CFG,
		.rct_on = 1,
		.bit_per_channel = DSC_BITS,
		.dsc_line_buf_depth = 9,
		.bp_enable = 1,
		.bit_per_pixel = 128,
		.pic_height = 2400,
		.pic_width = 1080,
		.slice_height = 12,
		.slice_width = 540,
		.chunk_size = 540,
		.xmit_delay = 512,
		.dec_delay = 526,
		.scale_value = 32,
		.increment_interval = 287,
		.decrement_interval = 7,
		.line_bpg_offset = 12,
		.nfl_bpg_offset = 2235,
		.slice_bpg_offset = 2170,
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

		.rc_buf_thresh[ 0] = 14,
		.rc_buf_thresh[ 1] = 28,
		.rc_buf_thresh[ 2] = 42,
		.rc_buf_thresh[ 3] = 56,
		.rc_buf_thresh[ 4] = 70,
		.rc_buf_thresh[ 5] = 84,
		.rc_buf_thresh[ 6] = 98,
		.rc_buf_thresh[ 7] = 105,
		.rc_buf_thresh[ 8] = 112,
		.rc_buf_thresh[ 9] = 119,
		.rc_buf_thresh[10] = 121,
		.rc_buf_thresh[11] = 123,
		.rc_buf_thresh[12] = 125,
		.rc_buf_thresh[13] = 126,
		.rc_range_parameters[ 0].range_min_qp = 0,
		.rc_range_parameters[ 0].range_max_qp = 4,
		.rc_range_parameters[ 0].range_bpg_offset = 2,
		.rc_range_parameters[ 1].range_min_qp = 0,
		.rc_range_parameters[ 1].range_max_qp = 4,
		.rc_range_parameters[ 1].range_bpg_offset = 0,
		.rc_range_parameters[ 2].range_min_qp = 1,
		.rc_range_parameters[ 2].range_max_qp = 5,
		.rc_range_parameters[ 2].range_bpg_offset = 0,
		.rc_range_parameters[ 3].range_min_qp = 1,
		.rc_range_parameters[ 3].range_max_qp = 6,
		.rc_range_parameters[ 3].range_bpg_offset = -2,
		.rc_range_parameters[ 4].range_min_qp = 3,
		.rc_range_parameters[ 4].range_max_qp = 7,
		.rc_range_parameters[ 4].range_bpg_offset = -4,
		.rc_range_parameters[ 5].range_min_qp = 3,
		.rc_range_parameters[ 5].range_max_qp = 7,
		.rc_range_parameters[ 5].range_bpg_offset = -6,
		.rc_range_parameters[ 6].range_min_qp = 3,
		.rc_range_parameters[ 6].range_max_qp = 7,
		.rc_range_parameters[ 6].range_bpg_offset = -8,
		.rc_range_parameters[ 7].range_min_qp = 3,
		.rc_range_parameters[ 7].range_max_qp = 8,
		.rc_range_parameters[ 7].range_bpg_offset = -8,
		.rc_range_parameters[ 8].range_min_qp = 3,
		.rc_range_parameters[ 8].range_max_qp = 9,
		.rc_range_parameters[ 8].range_bpg_offset = -8,
		.rc_range_parameters[ 9].range_min_qp = 3,
		.rc_range_parameters[ 9].range_max_qp = 10,
		.rc_range_parameters[ 9].range_bpg_offset = -10,
		.rc_range_parameters[10].range_min_qp = 5,
		.rc_range_parameters[10].range_max_qp = 10,
		.rc_range_parameters[10].range_bpg_offset = -10,
		.rc_range_parameters[11].range_min_qp = 5,
		.rc_range_parameters[11].range_max_qp = 11,
		.rc_range_parameters[11].range_bpg_offset = -12,
		.rc_range_parameters[12].range_min_qp = 5,
		.rc_range_parameters[12].range_max_qp = 11,
		.rc_range_parameters[12].range_bpg_offset = -12,
		.rc_range_parameters[13].range_min_qp = 9,
		.rc_range_parameters[13].range_max_qp = 12,
		.rc_range_parameters[13].range_bpg_offset = -12,
		.rc_range_parameters[14].range_min_qp = 12,
		.rc_range_parameters[14].range_max_qp = 13,
		.rc_range_parameters[14].range_bpg_offset = -12,
	},
	.max_bl_level = 3514,
	.hbm_type = HBM_MODE_DCS_ONLY,
	//.te_delay = 1,

	.dyn = {
		.switch_en = 0,
		//.pll_clk = 600,
		.vfp_lp_dyn = 1512,
		.hfp = 60,
		.vfp = 1512,
	},
	.dyn_fps = {
		.switch_en = 1,
		.dfps_cmd_grp_table[0] = {2, {0x6c, 0x01} },
		.dfps_cmd_grp_table[1] = {2, {0x62, 0x01} },
		.dfps_cmd_grp_table[2] = {3, {0xf0, 0xaa, 0x10} },
		.dfps_cmd_grp_table[3] = {14, {0xb1, 0x01, 0x55, 0x00, 0x18, 0x0d, 0x98, 0x00, 0x01, 0x55, 0x00, 0x18, 0x05, 0xe8} },
		.dfps_cmd_grp_size = 4,
	},
	.data_rate = 1156,
	//.lfr_enable = 1,
	//.lfr_minimum_fps = 60,
	.change_fps_by_vfp_send_cmd = 1,
	.use_ext_panel_feature = 1,
};
static struct mtk_panel_params ext_params_120hz = {

	.cust_esd_check = 0,
	.esd_check_enable = 1,
	.lcm_esd_check_table[0] = {
		.cmd = 0x0a,
		.count = 1,
		.para_list[0] = 0x9c,
	},
	.lcm_color_mode = MTK_DRM_COLOR_MODE_DISPLAY_P3,
	.physical_width_um = 68256,
	.physical_height_um = 151680,
	.lcm_index = 1,

	.output_mode = MTK_PANEL_DSC_SINGLE_PORT,
	.dsc_params = {
		.enable = 1,
		.ver = 17,
		.slice_mode = 1,
		.rgb_swap = 0,
		.dsc_cfg = DSC_CFG,
		.rct_on = 1,
		.bit_per_channel = DSC_BITS,
		.dsc_line_buf_depth = 9,
		.bp_enable = 1,
		.bit_per_pixel = 128,
		.pic_height = 2400,
		.pic_width = 1080,
		.slice_height = 12,
		.slice_width = 540,
		.chunk_size = 540,
		.xmit_delay = 512,
		.dec_delay = 526,
		.scale_value = 32,
		.increment_interval = 287,
		.decrement_interval = 7,
		.line_bpg_offset = 12,
		.nfl_bpg_offset = 2235,
		.slice_bpg_offset = 2170,
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

		.rc_buf_thresh[ 0] = 14,
		.rc_buf_thresh[ 1] = 28,
		.rc_buf_thresh[ 2] = 42,
		.rc_buf_thresh[ 3] = 56,
		.rc_buf_thresh[ 4] = 70,
		.rc_buf_thresh[ 5] = 84,
		.rc_buf_thresh[ 6] = 98,
		.rc_buf_thresh[ 7] = 105,
		.rc_buf_thresh[ 8] = 112,
		.rc_buf_thresh[ 9] = 119,
		.rc_buf_thresh[10] = 121,
		.rc_buf_thresh[11] = 123,
		.rc_buf_thresh[12] = 125,
		.rc_buf_thresh[13] = 126,
		.rc_range_parameters[ 0].range_min_qp = 0,
		.rc_range_parameters[ 0].range_max_qp = 4,
		.rc_range_parameters[ 0].range_bpg_offset = 2,
		.rc_range_parameters[ 1].range_min_qp = 0,
		.rc_range_parameters[ 1].range_max_qp = 4,
		.rc_range_parameters[ 1].range_bpg_offset = 0,
		.rc_range_parameters[ 2].range_min_qp = 1,
		.rc_range_parameters[ 2].range_max_qp = 5,
		.rc_range_parameters[ 2].range_bpg_offset = 0,
		.rc_range_parameters[ 3].range_min_qp = 1,
		.rc_range_parameters[ 3].range_max_qp = 6,
		.rc_range_parameters[ 3].range_bpg_offset = -2,
		.rc_range_parameters[ 4].range_min_qp = 3,
		.rc_range_parameters[ 4].range_max_qp = 7,
		.rc_range_parameters[ 4].range_bpg_offset = -4,
		.rc_range_parameters[ 5].range_min_qp = 3,
		.rc_range_parameters[ 5].range_max_qp = 7,
		.rc_range_parameters[ 5].range_bpg_offset = -6,
		.rc_range_parameters[ 6].range_min_qp = 3,
		.rc_range_parameters[ 6].range_max_qp = 7,
		.rc_range_parameters[ 6].range_bpg_offset = -8,
		.rc_range_parameters[ 7].range_min_qp = 3,
		.rc_range_parameters[ 7].range_max_qp = 8,
		.rc_range_parameters[ 7].range_bpg_offset = -8,
		.rc_range_parameters[ 8].range_min_qp = 3,
		.rc_range_parameters[ 8].range_max_qp = 9,
		.rc_range_parameters[ 8].range_bpg_offset = -8,
		.rc_range_parameters[ 9].range_min_qp = 3,
		.rc_range_parameters[ 9].range_max_qp = 10,
		.rc_range_parameters[ 9].range_bpg_offset = -10,
		.rc_range_parameters[10].range_min_qp = 5,
		.rc_range_parameters[10].range_max_qp = 10,
		.rc_range_parameters[10].range_bpg_offset = -10,
		.rc_range_parameters[11].range_min_qp = 5,
		.rc_range_parameters[11].range_max_qp = 11,
		.rc_range_parameters[11].range_bpg_offset = -12,
		.rc_range_parameters[12].range_min_qp = 5,
		.rc_range_parameters[12].range_max_qp = 11,
		.rc_range_parameters[12].range_bpg_offset = -12,
		.rc_range_parameters[13].range_min_qp = 9,
		.rc_range_parameters[13].range_max_qp = 12,
		.rc_range_parameters[13].range_bpg_offset = -12,
		.rc_range_parameters[14].range_min_qp = 12,
		.rc_range_parameters[14].range_max_qp = 13,
		.rc_range_parameters[14].range_bpg_offset = -12,
	},
	.max_bl_level = 3514,
	.hbm_type = HBM_MODE_DCS_ONLY,
	//.te_delay = 1,

	.dyn = {
		.switch_en = 0,
		//.pll_clk = 600,
		.vfp_lp_dyn = 528,
		.hfp = 60,
		.vfp = 528,
	},
	.dyn_fps = {
		.switch_en = 1,
		.dfps_cmd_grp_table[0] = {2, {0x6c, 0x01} },
		.dfps_cmd_grp_table[1] = {2, {0x62, 0x00} },
		.dfps_cmd_grp_table[2] = {3, {0xf0, 0xaa, 0x10} },
		.dfps_cmd_grp_table[3] = {14, {0xb1, 0x01, 0x55, 0x00, 0x18, 0x0d, 0x98, 0x00, 0x01, 0x55, 0x00, 0x18, 0x02, 0x10} },
		.dfps_cmd_grp_size = 4,
	},
	.data_rate = 1156,
	//.lfr_enable = 1,
	//.lfr_minimum_fps = 60,
	.change_fps_by_vfp_send_cmd = 1,
	.use_ext_panel_feature = 1,
};

static struct mtk_panel_params ext_params_144hz = {

	.cust_esd_check = 0,
	.esd_check_enable = 1,
	.lcm_esd_check_table[0] = {
		.cmd = 0x0a,
		.count = 1,
		.para_list[0] = 0x9c,
	},
	.lcm_color_mode = MTK_DRM_COLOR_MODE_DISPLAY_P3,
	.physical_width_um = 68256,
	.physical_height_um = 151680,
	.lcm_index = 1,

	.output_mode = MTK_PANEL_DSC_SINGLE_PORT,
	.dsc_params = {
		.enable = 1,
		.ver = 17,
		.slice_mode = 1,
		.rgb_swap = 0,
		.dsc_cfg = DSC_CFG,
		.rct_on = 1,
		.bit_per_channel = DSC_BITS,
		.dsc_line_buf_depth = 9,
		.bp_enable = 1,
		.bit_per_pixel = 128,
		.pic_height = 2400,
		.pic_width = 1080,
		.slice_height = 12,
		.slice_width = 540,
		.chunk_size = 540,
		.xmit_delay = 512,
		.dec_delay = 526,
		.scale_value = 32,
		.increment_interval = 287,
		.decrement_interval = 7,
		.line_bpg_offset = 12,
		.nfl_bpg_offset = 2235,
		.slice_bpg_offset = 2170,
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

		.rc_buf_thresh[ 0] = 14,
		.rc_buf_thresh[ 1] = 28,
		.rc_buf_thresh[ 2] = 42,
		.rc_buf_thresh[ 3] = 56,
		.rc_buf_thresh[ 4] = 70,
		.rc_buf_thresh[ 5] = 84,
		.rc_buf_thresh[ 6] = 98,
		.rc_buf_thresh[ 7] = 105,
		.rc_buf_thresh[ 8] = 112,
		.rc_buf_thresh[ 9] = 119,
		.rc_buf_thresh[10] = 121,
		.rc_buf_thresh[11] = 123,
		.rc_buf_thresh[12] = 125,
		.rc_buf_thresh[13] = 126,
		.rc_range_parameters[ 0].range_min_qp = 0,
		.rc_range_parameters[ 0].range_max_qp = 4,
		.rc_range_parameters[ 0].range_bpg_offset = 2,
		.rc_range_parameters[ 1].range_min_qp = 0,
		.rc_range_parameters[ 1].range_max_qp = 4,
		.rc_range_parameters[ 1].range_bpg_offset = 0,
		.rc_range_parameters[ 2].range_min_qp = 1,
		.rc_range_parameters[ 2].range_max_qp = 5,
		.rc_range_parameters[ 2].range_bpg_offset = 0,
		.rc_range_parameters[ 3].range_min_qp = 1,
		.rc_range_parameters[ 3].range_max_qp = 6,
		.rc_range_parameters[ 3].range_bpg_offset = -2,
		.rc_range_parameters[ 4].range_min_qp = 3,
		.rc_range_parameters[ 4].range_max_qp = 7,
		.rc_range_parameters[ 4].range_bpg_offset = -4,
		.rc_range_parameters[ 5].range_min_qp = 3,
		.rc_range_parameters[ 5].range_max_qp = 7,
		.rc_range_parameters[ 5].range_bpg_offset = -6,
		.rc_range_parameters[ 6].range_min_qp = 3,
		.rc_range_parameters[ 6].range_max_qp = 7,
		.rc_range_parameters[ 6].range_bpg_offset = -8,
		.rc_range_parameters[ 7].range_min_qp = 3,
		.rc_range_parameters[ 7].range_max_qp = 8,
		.rc_range_parameters[ 7].range_bpg_offset = -8,
		.rc_range_parameters[ 8].range_min_qp = 3,
		.rc_range_parameters[ 8].range_max_qp = 9,
		.rc_range_parameters[ 8].range_bpg_offset = -8,
		.rc_range_parameters[ 9].range_min_qp = 3,
		.rc_range_parameters[ 9].range_max_qp = 10,
		.rc_range_parameters[ 9].range_bpg_offset = -10,
		.rc_range_parameters[10].range_min_qp = 5,
		.rc_range_parameters[10].range_max_qp = 10,
		.rc_range_parameters[10].range_bpg_offset = -10,
		.rc_range_parameters[11].range_min_qp = 5,
		.rc_range_parameters[11].range_max_qp = 11,
		.rc_range_parameters[11].range_bpg_offset = -12,
		.rc_range_parameters[12].range_min_qp = 5,
		.rc_range_parameters[12].range_max_qp = 11,
		.rc_range_parameters[12].range_bpg_offset = -12,
		.rc_range_parameters[13].range_min_qp = 9,
		.rc_range_parameters[13].range_max_qp = 12,
		.rc_range_parameters[13].range_bpg_offset = -12,
		.rc_range_parameters[14].range_min_qp = 12,
		.rc_range_parameters[14].range_max_qp = 13,
		.rc_range_parameters[14].range_bpg_offset = -12,
	},
	.max_bl_level = 3514,
	.hbm_type = HBM_MODE_DCS_ONLY,
	//.te_delay = 1,

	.dyn = {
		.switch_en = 0,
		//.pll_clk = 600,
		.vfp_lp_dyn = 36,
		.hfp = 60,
		.vfp = 36,
	},
	.dyn_fps = {
		.switch_en = 1,
		.dfps_cmd_grp_table[0] = {2, {0x6c, 0x02} },
		.dfps_cmd_grp_table[1] = {2, {0x62, 0x00} },
		.dfps_cmd_grp_size = 2,
	},
	.data_rate = 1156,
	//.lfr_enable = 1,
	//.lfr_minimum_fps = 60,
	.change_fps_by_vfp_send_cmd = 1,
	.use_ext_panel_feature = 1,
};
#endif

static int panel_ata_check(struct drm_panel *panel)
{
	/* Customer test by own ATA tool */
	return 1;
}

static int lcm_setbacklight_cmdq(void *dsi, dcs_write_gce cb, void *handle,
				 unsigned int level)
{
	char bl_tb0[] = { 0x51, 0x0f, 0xff};
	struct lcm *ctx = tm_ctx;

	if (ctx->hbm_mode) {
		pr_info("hbm_mode = %d, skip backlight(%d)\n", ctx->hbm_mode, level);
		ctx->current_bl = level;
		return 0;
	}

	if (!(ctx->current_bl && level)) pr_info("tm_v1 backlight changed from %u to %u\n", ctx->current_bl, level);
	else pr_info("tm_v1 backlight changed from %u to %u\n", ctx->current_bl, level);

	bl_tb0[1] = (u8)((level>>8)&0xF);
	bl_tb0[2] = (u8)(level&0xFF);

	if (!cb)
		return -1;

	cb(dsi, handle, bl_tb0, ARRAY_SIZE(bl_tb0));
	ctx->current_bl = level;
	return 0;
}

static int panel_ext_reset(struct drm_panel *panel, int on)
{
	struct lcm *ctx = panel_to_lcm(panel);

	ctx->reset_gpio =
		devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
	gpiod_set_value(ctx->reset_gpio, on);
	devm_gpiod_put(ctx->dev, ctx->reset_gpio);

	return 0;
}

static int mtk_panel_ext_param_set(struct drm_panel *panel, unsigned int mode)
{
	struct mtk_panel_ext *ext = find_panel_ext(panel);
	struct lcm *ctx = panel_to_lcm(panel);
	int ret = 0;

	if (mode == 0) {
		ext->params = &ext_params_60hz;
		ctx->current_fps = 60;
	}
	else if (mode == 1) {
		ext->params = &ext_params_144hz;
		ctx->current_fps = 144;
	}
	else if (mode == 2) {
		ext->params = &ext_params_120hz;
		ctx->current_fps = 120;
	}
	else if (mode == 3) {
		ext->params = &ext_params_90hz;
		ctx->current_fps = 90;
	}
	else
		ret = 1;

	return ret;
}

#if 0
static void mode_switch_to_144(struct drm_panel *panel,
	enum MTK_PANEL_MODE_SWITCH_STAGE stage)
{
	if (stage == BEFORE_DSI_POWERDOWN) {
		struct lcm *ctx = panel_to_lcm(panel);

		lcm_dcs_write_seq_static(ctx, 0x2F, 0x04);
		lcm_dcs_write_seq_static(ctx, 0x3B, 0x00, 0x07, 0x00, 0x1c);
	}
}

static void mode_switch_to_120(struct drm_panel *panel,
	enum MTK_PANEL_MODE_SWITCH_STAGE stage)
{
	if (stage == BEFORE_DSI_POWERDOWN) {
		struct lcm *ctx = panel_to_lcm(panel);

		lcm_dcs_write_seq_static(ctx, 0x2F, 0x02);
		lcm_dcs_write_seq_static(ctx, 0x3B, 0x00, 0x07, 0x01, 0xee);
	}
}

static void mode_switch_to_90(struct drm_panel *panel,
	enum MTK_PANEL_MODE_SWITCH_STAGE stage)
{
	if (stage == BEFORE_DSI_POWERDOWN) {
		struct lcm *ctx = panel_to_lcm(panel);

		lcm_dcs_write_seq_static(ctx, 0x2F, 0x03);
		lcm_dcs_write_seq_static(ctx, 0x3B, 0x00, 0x07, 0x05, 0xbc);
	}
}
static void mode_switch_to_60(struct drm_panel *panel,
	enum MTK_PANEL_MODE_SWITCH_STAGE stage)
{
	if (stage == BEFORE_DSI_POWERDOWN) {
		struct lcm *ctx = panel_to_lcm(panel);

		lcm_dcs_write_seq_static(ctx, 0x2F, 0x01);
		lcm_dcs_write_seq_static(ctx, 0x3B, 0x00, 0x07, 0x0d, 0x58);
	}
}

static int mode_switch(struct drm_panel *panel, unsigned int cur_mode,
		unsigned int dst_mode, enum MTK_PANEL_MODE_SWITCH_STAGE stage)
{
	int ret = 0;

	if (cur_mode == dst_mode)
		return ret;

	pr_info("##### %s: mode switch to %d\n", __func__, dst_mode);

    if (dst_mode == 60) { /*switch to 60 */
		mode_switch_to_60(panel, stage);
	} else if (dst_mode == 90) { /*switch to 60 */
		mode_switch_to_90(panel, stage);
	} else if (dst_mode == 120) { /*switch to 120 */
		mode_switch_to_120(panel, stage);
	} else if (dst_mode == 144) { /*switch to 144 */
		mode_switch_to_144(panel, stage);
	} else
		ret = 1;

	return ret;
}
#endif

static struct mtk_panel_para_table panel_lhbm_on[] = {
   {3, {0xf0, 0xaa, 0x13}},
   {2, {0xc6, 0x01}},
   {3, {0x63, 0x03, 0xff}},
   {3, {0x51, 0x06, 0x8f}},
   {2, {0x62, 0x03}},
};

static struct mtk_panel_para_table panel_lhbm_off[] = {
   {2, {0x62, 0x00}},
   {3, {0x51, 0x06, 0x8f}},
   {3, {0xf0, 0xaa, 0x13}},
   {2, {0xc6, 0x00}},
};

static void set_lhbm_alpha(unsigned int bl_level, unsigned int on)
{
	struct mtk_panel_para_table *pAlphaTable;
	struct mtk_panel_para_table *pDbvTable;
	unsigned int alpha = 0;
	unsigned int dbv = 0;

	if(on) {
		pAlphaTable = &panel_lhbm_on[2];
		pDbvTable = &panel_lhbm_on[3];
		if(bl_level < 1679)	//0x68f, bl threshold
			dbv = 1679;
		else
			dbv = bl_level;

		if (bl_level >= ARRAY_SIZE(lhbm_alpha))
			bl_level = ARRAY_SIZE(lhbm_alpha) -1;

		alpha = lhbm_alpha[bl_level];

		pAlphaTable->para_list[1] = (alpha >> 8) & 0xFF;
		pAlphaTable->para_list[2] = alpha & 0xFF;
		pDbvTable->para_list[1] = (dbv >> 8) & 0xFF;
		pDbvTable->para_list[2] = dbv & 0xFF;
		pr_info("%s: backlight %d alpha %d(0x%x, 0x%x), dbv(0x%x, 0x%x)\n", __func__, bl_level, alpha,
			pAlphaTable->para_list[1], pAlphaTable->para_list[2], pDbvTable->para_list[1], pDbvTable->para_list[2]);
	} else {
		pDbvTable = &panel_lhbm_off[1];
		pDbvTable->para_list[1] = (bl_level >> 8) & 0xFF;
		pDbvTable->para_list[2] = bl_level & 0xFF;
		pr_info("%s: backlight restore %d dbv(0x%x, 0x%x)\n", __func__, bl_level,
			pDbvTable->para_list[1], pDbvTable->para_list[2]);
	}
}

static int panel_lhbm_set_cmdq(void *dsi, dcs_grp_write_gce cb, void *handle, uint32_t on, uint32_t bl_level, uint32_t fps)
{
	unsigned int para_count = 0;
	struct mtk_panel_para_table *pTable;

	set_lhbm_alpha(bl_level, on);
	if (on) {
		para_count = sizeof(panel_lhbm_on) / sizeof(struct mtk_panel_para_table);
		pTable = panel_lhbm_on;
	} else {
		para_count = sizeof(panel_lhbm_off) / sizeof(struct mtk_panel_para_table);
		if(fps == 90)
			panel_lhbm_off[0].para_list[1] = 0x01;
		else
			panel_lhbm_off[0].para_list[1] = 0x00;
		pr_info("%s: panel_lhbm_off(0x%x, 0x%x)\n", __func__,
			panel_lhbm_off[0].para_list[0], panel_lhbm_off[0].para_list[1]);
		pTable = panel_lhbm_off;
	}
	cb(dsi, handle, pTable, para_count);
	pr_info("%s: para_count %d\n", __func__, para_count);
	return 0;

}

static int pane_hbm_set_cmdq(struct lcm *ctx, void *dsi, dcs_grp_write_gce cb, void *handle, uint32_t hbm_state)
{
	struct mtk_panel_para_table hbm_on_table = {3, {0x51, 0x0F, 0xFF}};

	if (hbm_state > 2) return -1;
	switch (hbm_state)
	{
		case 0:
			if (ctx->lhbm_en)
				panel_lhbm_set_cmdq(dsi, cb, handle, 0, ctx->current_bl, ctx->current_fps);
			break;
		case 1:
			if (ctx->lhbm_en)
				panel_lhbm_set_cmdq(dsi, cb, handle, 0, ctx->current_bl, ctx->current_fps);
			cb(dsi, handle, &hbm_on_table, 1);
			break;
		case 2:
			if (ctx->lhbm_en)
				panel_lhbm_set_cmdq(dsi, cb, handle, 1, ctx->current_bl, ctx->current_fps);
			else
				cb(dsi, handle, &hbm_on_table, 1);
			break;
		default:
			break;
	}

	return 0;
}

static struct mtk_panel_para_table panel_dc_off[] = {
   {2, {0x5e, 0x00}},
};

static struct mtk_panel_para_table panel_dc_on[] = {
   {2, {0x5e, 0x01}},
};

static int pane_dc_set_cmdq(void *dsi, dcs_grp_write_gce cb, void *handle, uint32_t dc_state)
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

	return 0;
}

static int panel_feature_set(struct drm_panel *panel, void *dsi,
			      dcs_grp_write_gce cb, void *handle, struct panel_param_info param_info)
{

	struct lcm *ctx = panel_to_lcm(panel);

	if (!cb)
		return -1;

	pr_info("%s: set feature %d to %d\n", __func__, param_info.param_idx, param_info.value);

	switch (param_info.param_idx) {
		case PARAM_CABC:
		case PARAM_ACL:
			break;
		case PARAM_HBM:
			ctx->hbm_mode = param_info.value;
			pane_hbm_set_cmdq(ctx, dsi, cb, handle, param_info.value);
			break;
		case PARAM_DC:
			pane_dc_set_cmdq(dsi, cb, handle, param_info.value);
			ctx->dc_mode = param_info.value;
			break;
		default:
			break;
	}

	pr_info("%s: set feature %d to %d success\n", __func__, param_info.param_idx, param_info.value);
	return 0;
}

static int panel_hbm_waitfor_fps_valid(struct drm_panel *panel, unsigned int timeout_ms)
{
	struct lcm *ctx = panel_to_lcm(panel);
	unsigned int count = 60;
	unsigned int poll_interval = 1;

	if (count == 0) return 0;
	pr_info("%s+, fps = %d \n", __func__, ctx->current_fps);
	while((ctx->current_fps != 120)) {
		if (!count) {
			pr_warn("%s: it is timeout, and current_fps = %d\n", __func__, ctx->current_fps);
			break;
		} else if (count > poll_interval) {
			usleep_range(poll_interval * 1000, poll_interval *1000);
			count -= poll_interval;
		} else {
			usleep_range(count * 1000, count *1000);
			count = 0;
		}
	}
	pr_info("%s-, fps = %d \n", __func__, ctx->current_fps);
	return 0;
}

static int panel_ext_init_power(struct drm_panel *panel)
{
	int ret;
	struct lcm *ctx = panel_to_lcm(panel);

	ctx->reset_gpio = devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
	gpiod_set_value(ctx->reset_gpio, 0);
	devm_gpiod_put(ctx->dev, ctx->reset_gpio);
	msleep(1);

	ret = gate_ic_Power_on(panel, 1);
	return ret;
}

static int panel_ext_powerdown(struct drm_panel *panel)
{
	struct lcm *ctx = panel_to_lcm(panel);

	pr_info("%s+\n", __func__);
	if (ctx->prepared)
	    return 0;

	ctx->reset_gpio = devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
	gpiod_set_value(ctx->reset_gpio, 0);
	devm_gpiod_put(ctx->dev, ctx->reset_gpio);

	gate_ic_Power_on(panel, 0);

	return 0;
}

static struct mtk_panel_funcs ext_funcs = {
	.reset = panel_ext_reset,
	.set_backlight_cmdq = lcm_setbacklight_cmdq,
	//.init_power = panel_ext_init_power,
	//.power_down = panel_ext_powerdown,
	.ata_check = panel_ata_check,
	.ext_param_set = mtk_panel_ext_param_set,
	//.mode_switch = mode_switch,
	.panel_feature_set = panel_feature_set,
	.panel_hbm_waitfor_fps_valid = panel_hbm_waitfor_fps_valid,
};

static int lcm_get_modes(struct drm_panel *panel)
{
	struct drm_display_mode *mode_1;
	struct drm_display_mode *mode_2;
	struct drm_display_mode *mode_3;
#ifdef SUPPORT_144HZ_REFRESH_RATE
	struct drm_display_mode *mode_4;
#endif

	mode_1 = drm_mode_duplicate(panel->drm, &switch_mode_60hz);
	if (!mode_1) {
		dev_info(panel->drm->dev, "failed to add mode %ux%ux@%u\n",
			 switch_mode_60hz.hdisplay, switch_mode_60hz.vdisplay,
			 drm_mode_vrefresh(&switch_mode_60hz));
		return -ENOMEM;
	}
	drm_mode_set_name(mode_1);
	mode_1->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	drm_mode_probed_add(panel->connector, mode_1);

	mode_2 = drm_mode_duplicate(panel->drm, &switch_mode_90hz);
	if (!mode_2) {
		dev_info(panel->drm->dev, "failed to add mode %ux%ux@%u\n",
			switch_mode_90hz.hdisplay, switch_mode_90hz.vdisplay,
			drm_mode_vrefresh(&switch_mode_90hz));
		return -ENOMEM;
	}
	drm_mode_set_name(mode_2);
	mode_2->type = DRM_MODE_TYPE_DRIVER;
	drm_mode_probed_add(panel->connector, mode_2);

	mode_3 = drm_mode_duplicate(panel->drm, &switch_mode_120hz);
	if (!mode_3) {
		dev_info(panel->drm->dev, "failed to add mode %ux%ux@%u\n",
			switch_mode_120hz.hdisplay, switch_mode_120hz.vdisplay,
			drm_mode_vrefresh(&switch_mode_120hz));
		return -ENOMEM;
	}
	drm_mode_set_name(mode_3);
	mode_3->type = DRM_MODE_TYPE_DRIVER;
	drm_mode_probed_add(panel->connector, mode_3);

#ifdef SUPPORT_144HZ_REFRESH_RATE
	mode_4 = drm_mode_duplicate(panel->drm, &switch_mode_144hz);
	if (!mode_4) {
		dev_info(panel->drm->dev, "failed to add mode %ux%ux@%u\n",
			switch_mode_144hz.hdisplay, switch_mode_144hz.vdisplay,
			drm_mode_vrefresh(&switch_mode_144hz));
		return -ENOMEM;
	}
	drm_mode_set_name(mode_4);
	mode_4->type = DRM_MODE_TYPE_DRIVER;
	drm_mode_probed_add(panel->connector, mode_4);
#endif
	panel->connector->display_info.width_mm = 68;
	panel->connector->display_info.height_mm = 152;

	panel->connector->display_info.panel_ver = 0x03;
	panel->connector->display_info.panel_id = 0x010b1591;
	strcpy(panel->connector->display_info.panel_name, "mipi_mot_vid_tianma_vtdr6115_fhd_655_v1");
	strcpy(panel->connector->display_info.panel_supplier, "tianma-vtdr6115");

	panel->connector->display_info.panel_cellid_reg = 0x5a;
	panel->connector->display_info.panel_cellid_offset = 0x65;
	panel->connector->display_info.panel_cellid_len = 23;

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
	const u32 *val;

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
		pr_info("%s+ skip probe due to not current lcm(node: %s)\n", __func__, dev->of_node->name);
		return -ENODEV;
	}

	ctx = devm_kzalloc(dev, sizeof(struct lcm), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	mipi_dsi_set_drvdata(dsi, ctx);
	tm_ctx = ctx;
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
		dev_info(dev, "cannot get reset-gpios %ld\n",
			 PTR_ERR(ctx->reset_gpio));
		return PTR_ERR(ctx->reset_gpio);
	}
	devm_gpiod_put(dev, ctx->reset_gpio);
	ctx->prepared = true;
	ctx->enabled = true;
	drm_panel_init(&ctx->panel);
	ctx->panel.dev = dev;
	ctx->panel.funcs = &lcm_drm_funcs;

	drm_panel_add(&ctx->panel);

	val = of_get_property(dev->of_node, "reg", NULL);
	ctx->version = val ? be32_to_cpup(val) : 1;

	pr_info("%s: panel version 0x%x\n", __func__, ctx->version);

	ret = mipi_dsi_attach(dsi);
	if (ret < 0)
		drm_panel_remove(&ctx->panel);

#if defined(CONFIG_MTK_PANEL_EXT)
	mtk_panel_tch_handle_reg(&ctx->panel);
	ret = mtk_panel_ext_create(dev, &ext_params_60hz, &ext_funcs, &ctx->panel);
	if (ret < 0)
		return ret;

#endif
	ctx->hbm_mode = 0;
	ctx->dc_mode = 0;
	ctx->current_bl = 1750;

	ctx->lhbm_en = of_property_read_bool(dev->of_node, "lhbm-enable");

	pr_info("%s- lcm,tm_v1, lhbm_en = %d\n", __func__, ctx->lhbm_en);

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
	{
		.compatible = "tm,vtdr6115,vdo,144hz,v1",
	},
	{}
};

MODULE_DEVICE_TABLE(of, lcm_of_match);

static struct mipi_dsi_driver lcm_driver = {
	.probe = lcm_probe,
	.remove = lcm_remove,
	.driver = {
		.name = "tm_vtdr6115_655_1080x2400_v1",
		.owner = THIS_MODULE,
		.of_match_table = lcm_of_match,
	},
};

module_mipi_dsi_driver(lcm_driver);

MODULE_AUTHOR("MEDIATEK");
MODULE_DESCRIPTION("TM VTDR6115 AMOLED CMD SPR LCD Panel Driver");
MODULE_LICENSE("GPL v2");
