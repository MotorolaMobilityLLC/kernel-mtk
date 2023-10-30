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
#include "../mediatek/mediatek_v2/mtk_panel_ext.h"
#include "../mediatek/mediatek_v2/mtk_drm_graphics_base.h"
#endif

#include "dsi-panel-mot-csot-nt37701a-655-fhdp-dphy-cmd-144hz-lhbm-alpha-boe.h"

//#define SUPPORT_144HZ_REFRESH_RATE

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
	enum panel_version version;
};

struct lcm *g_ctx = NULL;

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
    lcm_dcs_write_seq_static(ctx, 0xF0, 0x55, 0xAA, 0x52, 0x08, 0x08);
    lcm_dcs_write_seq_static(ctx, 0xE1, 0x00, 0x73, 0x73, 0x73, 0x00);
    lcm_dcs_write_seq_static(ctx, 0xFF, 0xAA, 0x55, 0xA5, 0x80);
    lcm_dcs_write_seq_static(ctx, 0x6F, 0x1D);
    lcm_dcs_write_seq_static(ctx, 0xF2, 0x05);
    lcm_dcs_write_seq_static(ctx, 0xFF, 0xAA, 0x55, 0xA5, 0x81);
    lcm_dcs_write_seq_static(ctx, 0x35);
    lcm_dcs_write_seq_static(ctx, 0x53, 0x20);
    lcm_dcs_write_seq_static(ctx, 0x51, 0x00, 0x00);
    lcm_dcs_write_seq_static(ctx, 0x2A, 0x00, 0x00, 0x04, 0x37);
    lcm_dcs_write_seq_static(ctx, 0x2B, 0x00, 0x00, 0x09, 0x5F);
    lcm_dcs_write_seq_static(ctx, 0x82, 0xB0);
    lcm_dcs_write_seq_static(ctx, 0x88, 0x01, 0x02, 0x1C, 0x08, 0x74);
    lcm_dcs_write_seq_static(ctx, 0x03, 0x01);
    lcm_dcs_write_seq_static(ctx, 0x90, 0x01);
    lcm_dcs_write_seq_static(ctx, 0x91, 0xAB, 0x28, 0x00, 0x0C, 0xC2, 0x00, 0x03, 0x1C, 0x01, 0x7E, 0x00, 0x0F, 0x08, 0xBB, 0x04, 0x3D, 0x10, 0xF0);
    lcm_dcs_write_seq_static(ctx, 0x2C);
    lcm_dcs_write_seq_static(ctx, 0x8B, 0x80);

    //FrameRate 60Hz:0x01  120HZ:0x02
    lcm_dcs_write_seq_static(ctx, 0x2F, 0x01);

    /* Sleep Out */
    lcm_dcs_write_seq_static(ctx, 0x11);
    msleep(120);
    /* Display On */
    lcm_dcs_write_seq_static(ctx, 0x29);

	pr_info("%s-\n", __func__);
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
		for (i=0; i < 4; i++) {
			pm_en_pin = NULL;
			pm_en_pin = devm_gpiod_get_index(ctx->dev, "pm-enable", i, GPIOD_OUT_LOW);
			if (IS_ERR(pm_en_pin)) {
				pr_err("cannot get bias-gpios 0 %ld\n", PTR_ERR(pm_en_pin));
				return PTR_ERR(pm_en_pin);
			}
			gpiod_set_value(pm_en_pin, gpio_status);
			devm_gpiod_put(ctx->dev, pm_en_pin);
			usleep_range(1000, 1001);
		}
	} else {
		for (i=3; i >= 0; i--) {
			pm_en_pin = NULL;
			pm_en_pin = devm_gpiod_get_index(ctx->dev, "pm-enable", i, GPIOD_OUT_LOW);
			if (IS_ERR(pm_en_pin)) {
				pr_err("cannot get bias-gpios 0 %ld\n", PTR_ERR(pm_en_pin));
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


	pr_info("%s+\n", __func__);
	if (!ctx->prepared)
		return 0;

	lcm_dcs_write_seq_static(ctx, MIPI_DCS_SET_DISPLAY_OFF);
	lcm_dcs_write_seq_static(ctx, MIPI_DCS_ENTER_SLEEP_MODE);
	msleep(120);

	ctx->error = 0;
	ctx->prepared = false;

	return 0;
}

static int lcm_prepare(struct drm_panel *panel)
{
	struct lcm *ctx = panel_to_lcm(panel);
	int ret;

	pr_info("%s+\n", __func__);
	if (ctx->prepared)
		return 0;

	// lcd reset L->H -> L -> L
	ctx->reset_gpio = devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_LOW);
	gpiod_set_value(ctx->reset_gpio, 0);
	usleep_range(11000, 11001);
	gpiod_set_value(ctx->reset_gpio, 1);
	usleep_range(5000, 5001);
	gpiod_set_value(ctx->reset_gpio, 0);
	usleep_range(5000, 5001);
	gpiod_set_value(ctx->reset_gpio, 1);
	usleep_range(11000, 11001);
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

#define HFP (4)
#define HSA (4)
#define HBP (4)
#define HACT (1080)
#define VFP (20)
#define VSA (2)
#define VBP (8)
#define VACT (2400)
#define PLL_CLOCK (330)

static const struct drm_display_mode switch_mode_144hz = {
	.clock		= 382112,
	.hdisplay	= HACT,
	.hsync_start	= HACT + HFP,
	.hsync_end	= HACT + HFP + HSA,
	.htotal		= HACT + HFP + HSA + HBP,
	.vdisplay	= VACT,
	.vsync_start	= VACT + VFP,
	.vsync_end	= VACT + VFP + VSA,
	.vtotal		= VACT + VFP + VSA + VBP,
};

static const struct drm_display_mode switch_mode_120hz = {
	.clock		= 318428,
	.hdisplay	= HACT,
	.hsync_start	= HACT + HFP,
	.hsync_end	= HACT + HFP + HSA,
	.htotal		= HACT + HFP + HSA + HBP,
	.vdisplay	= VACT,
	.vsync_start	= VACT + VFP,
	.vsync_end	= VACT + VFP + VSA,
	.vtotal		= VACT + VFP + VSA + VBP,
};

static const struct drm_display_mode switch_mode_90hz = {
	.clock		= 238821,
	.hdisplay	= HACT,
	.hsync_start	= HACT + HFP,
	.hsync_end	= HACT + HFP + HSA,
	.htotal		= HACT + HFP + HSA + HBP,
	.vdisplay	= VACT,
	.vsync_start	= VACT + VFP,
	.vsync_end	= VACT + VFP + VSA,
	.vtotal		= VACT + VFP + VSA + VBP,
};

static const struct drm_display_mode switch_mode_60hz = {
	.clock = 159214,
	.hdisplay	= HACT,
	.hsync_start	= HACT + HFP,
	.hsync_end	= HACT + HFP + HSA,
	.htotal		= HACT + HFP + HSA + HBP,
	.vdisplay	= VACT,
	.vsync_start	= VACT + VFP,
	.vsync_end	= VACT + VFP + VSA,
	.vtotal		= VACT + VFP + VSA + VBP,
};


static const struct drm_display_mode switch_mode_48hz = {
	.clock = 127371,
	.hdisplay	= HACT,
	.hsync_start	= HACT + HFP,
	.hsync_end	= HACT + HFP + HSA,
	.htotal		= HACT + HFP + HSA + HBP,
	.vdisplay	= VACT,
	.vsync_start	= VACT + VFP,
	.vsync_end	= VACT + VFP + VSA,
	.vtotal		= VACT + VFP + VSA + VBP,
};

#if defined(CONFIG_MTK_PANEL_EXT)
static struct mtk_panel_params ext_params_48hz = {
	.dyn_fps = {
		.data_rate = 330,
	},
	.data_rate = 330,

	.cust_esd_check = 0,
	.esd_check_enable = 1,
	.lcm_esd_check_table[0] = {
		.cmd = 0x0a,
		.count = 1,
		.para_list[0] = 0x9c,
	},
	.physical_width_um = 68256,
	.physical_height_um = 151680,
	.lcm_index = 1,

	.output_mode = MTK_PANEL_DSC_SINGLE_PORT,
	.dsc_params = {
		.enable = 1,
		.ver = 17,
		.slice_mode = 0,
		.rgb_swap = 0,
		.dsc_cfg = 2088,
		.rct_on = 1,
		.bit_per_channel = 10,
		.dsc_line_buf_depth = 11,
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
		.flatness_minqp = 7,
		.flatness_maxqp = 16,
		.rc_model_size = 8192,
		.rc_edge_factor = 6,
		.rc_quant_incr_limit0 = 15,
		.rc_quant_incr_limit1 = 15,
		.rc_tgt_offset_hi = 3,
		.rc_tgt_offset_lo = 3,
		.pps_list = {
			.count = 3,
			.dsc_pps_params[0] = {
				.dsc_pps_idx = 17,
				.dsc_pps_para = 0xD209D9E9,
			},
			.dsc_pps_params[1] = {
				.dsc_pps_idx = 18,
				.dsc_pps_para = 0xD22BD229,
			},
			.dsc_pps_params[2] = {
				.dsc_pps_idx = 19,
				.dsc_pps_para = 0x0000D271,
			},
		},

	},
	.max_bl_level = 3514,
	.hbm_type = HBM_MODE_DCS_ONLY,
	.te_delay = 1,
};

static struct mtk_panel_params ext_params_60hz = {
	.dyn_fps = {
		.data_rate = 380,
	},
	.data_rate = 380,

	.cust_esd_check = 0,
	.esd_check_enable = 1,
	.lcm_esd_check_table[0] = {
		.cmd = 0x0a,
		.count = 1,
		.para_list[0] = 0x9c,
	},
	.physical_width_um = 68256,
	.physical_height_um = 151680,
	.lcm_index = 1,

	.output_mode = MTK_PANEL_DSC_SINGLE_PORT,
	.dsc_params = {
		.enable = 1,
		.ver = 17,
		.slice_mode = 0,
		.rgb_swap = 0,
		.dsc_cfg = 2088,
		.rct_on = 1,
		.bit_per_channel = 10,
		.dsc_line_buf_depth = 11,
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
		.flatness_minqp = 7,
		.flatness_maxqp = 16,
		.rc_model_size = 8192,
		.rc_edge_factor = 6,
		.rc_quant_incr_limit0 = 15,
		.rc_quant_incr_limit1 = 15,
		.rc_tgt_offset_hi = 3,
		.rc_tgt_offset_lo = 3,
		.pps_list = {
			.count = 3,
			.dsc_pps_params[0] = {
				.dsc_pps_idx = 17,
				.dsc_pps_para = 0xD209D9E9,
			},
			.dsc_pps_params[1] = {
				.dsc_pps_idx = 18,
				.dsc_pps_para = 0xD22BD229,
			},
			.dsc_pps_params[2] = {
				.dsc_pps_idx = 19,
				.dsc_pps_para = 0x0000D271,
			},
		},

	},
	.max_bl_level = 3514,
	.hbm_type = HBM_MODE_DCS_ONLY,
	.te_delay = 1,
};
static struct mtk_panel_params ext_params_90hz = {
	.dyn_fps = {
		.data_rate = 570,
	},
	.data_rate = 570,

	.cust_esd_check = 0,
	.esd_check_enable = 1,
	.lcm_esd_check_table[0] = {
		.cmd = 0x0a,
		.count = 1,
		.para_list[0] = 0x9c,
	},
	.physical_width_um = 68256,
	.physical_height_um = 151680,
	.lcm_index = 1,

	.output_mode = MTK_PANEL_DSC_SINGLE_PORT,
	.dsc_params = {
		.enable = 1,
		.ver = 17,
		.slice_mode = 0,
		.rgb_swap = 0,
		.dsc_cfg = 2088,
		.rct_on = 1,
		.bit_per_channel = 10,
		.dsc_line_buf_depth = 11,
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
		.flatness_minqp = 7,
		.flatness_maxqp = 16,
		.rc_model_size = 8192,
		.rc_edge_factor = 6,
		.rc_quant_incr_limit0 = 15,
		.rc_quant_incr_limit1 = 15,
		.rc_tgt_offset_hi = 3,
		.rc_tgt_offset_lo = 3,
		.pps_list = {
			.count = 3,
			.dsc_pps_params[0] = {
				.dsc_pps_idx = 17,
				.dsc_pps_para = 0xD209D9E9,
			},
			.dsc_pps_params[1] = {
				.dsc_pps_idx = 18,
				.dsc_pps_para = 0xD22BD229,
			},
			.dsc_pps_params[2] = {
				.dsc_pps_idx = 19,
				.dsc_pps_para = 0x0000D271,
			},
		},

	},
	.max_bl_level = 3514,
	.hbm_type = HBM_MODE_DCS_ONLY,
	.te_delay = 1,
};
static struct mtk_panel_params ext_params_120hz = {
	.dyn_fps = {
		.data_rate = 760,
	},
	.data_rate = 760,

	.cust_esd_check = 0,
	.esd_check_enable = 1,
	.lcm_esd_check_table[0] = {
		.cmd = 0x0a,
		.count = 1,
		.para_list[0] = 0x9c,
	},
	.physical_width_um = 68256,
	.physical_height_um = 151680,
	.lcm_index = 1,

	.output_mode = MTK_PANEL_DSC_SINGLE_PORT,
	.dsc_params = {
		.enable = 1,
		.ver = 17,
		.slice_mode = 0,
		.rgb_swap = 0,
		.dsc_cfg = 2088,
		.rct_on = 1,
		.bit_per_channel = 10,
		.dsc_line_buf_depth = 11,
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
		.flatness_minqp = 7,
		.flatness_maxqp = 16,
		.rc_model_size = 8192,
		.rc_edge_factor = 6,
		.rc_quant_incr_limit0 = 15,
		.rc_quant_incr_limit1 = 15,
		.rc_tgt_offset_hi = 3,
		.rc_tgt_offset_lo = 3,
		.pps_list = {
			.count = 3,
			.dsc_pps_params[0] = {
				.dsc_pps_idx = 17,
				.dsc_pps_para = 0xD209D9E9,
			},
			.dsc_pps_params[1] = {
				.dsc_pps_idx = 18,
				.dsc_pps_para = 0xD22BD229,
			},
			.dsc_pps_params[2] = {
				.dsc_pps_idx = 19,
				.dsc_pps_para = 0x0000D271,
			},
		},

	},
	.max_bl_level = 3514,
	.hbm_type = HBM_MODE_DCS_ONLY,
	.te_delay = 1,
};

static struct mtk_panel_params ext_params_144hz = {
	.dyn_fps = {
		.data_rate = 920,
	},
	.data_rate = 920,

	.cust_esd_check = 0,
	.esd_check_enable = 1,
	.lcm_esd_check_table[0] = {
		.cmd = 0x0a,
		.count = 1,
		.para_list[0] = 0x9c,
	},
	.physical_width_um = 68256,
	.physical_height_um = 151680,
	.lcm_index = 1,

	.output_mode = MTK_PANEL_DSC_SINGLE_PORT,
	.dsc_params = {
		.enable = 1,
		.ver = 17,
		.slice_mode = 0,
		.rgb_swap = 0,
		.dsc_cfg = 2088,
		.rct_on = 1,
		.bit_per_channel = 10,
		.dsc_line_buf_depth = 11,
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
		.flatness_minqp = 7,
		.flatness_maxqp = 16,
		.rc_model_size = 8192,
		.rc_edge_factor = 6,
		.rc_quant_incr_limit0 = 15,
		.rc_quant_incr_limit1 = 15,
		.rc_tgt_offset_hi = 3,
		.rc_tgt_offset_lo = 3,
		.pps_list = {
			.count = 3,
			.dsc_pps_params[0] = {
				.dsc_pps_idx = 17,
				.dsc_pps_para = 0xD209D9E9,
			},
			.dsc_pps_params[1] = {
				.dsc_pps_idx = 18,
				.dsc_pps_para = 0xD22BD229,
			},
			.dsc_pps_params[2] = {
				.dsc_pps_idx = 19,
				.dsc_pps_para = 0x0000D271,
			},
		},

	},
	.max_bl_level = 3514,
	.hbm_type = HBM_MODE_DCS_ONLY,
	.te_delay = 1,
};

static int panel_ata_check(struct drm_panel *panel)
{
	/* Customer test by own ATA tool */
	return 1;
}

static int lcm_setbacklight_cmdq(void *dsi, dcs_write_gce cb, void *handle,
				 unsigned int level)
{
	char bl_tb0[] = { 0x51, 0x0f, 0xff};
	struct lcm *ctx = g_ctx;

	if (ctx->hbm_mode) {
		pr_info("hbm_mode = %d, skip backlight(%d)\n", ctx->hbm_mode, level);
		return 0;
	}

	if (!(ctx->current_bl && level)) pr_info("backlight changed from %u to %u\n", ctx->current_bl, level);
	else pr_debug("backlight changed from %u to %u\n", ctx->current_bl, level);

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

	if (drm_mode_vrefresh(m) == 144)
		ext->params = &ext_params_144hz;
	else if (drm_mode_vrefresh(m) == 120)
		ext->params = &ext_params_120hz;
	else if (drm_mode_vrefresh(m) == 90)
		ext->params = &ext_params_90hz;
	else if (drm_mode_vrefresh(m) == 60)
		ext->params = &ext_params_60hz;
	else if (drm_mode_vrefresh(m) == 48)
		ext->params = &ext_params_48hz;
	else
		ret = 1;

	return ret;
}

static void mode_switch_to_144(struct drm_panel *panel,
	enum MTK_PANEL_MODE_SWITCH_STAGE stage)
{
	if (stage == BEFORE_DSI_POWERDOWN) {
		struct lcm *ctx = panel_to_lcm(panel);

		lcm_dcs_write_seq_static(ctx, 0x2F, 0x04);
	}
}

static void mode_switch_to_120(struct drm_panel *panel,
	enum MTK_PANEL_MODE_SWITCH_STAGE stage)
{
	if (stage == BEFORE_DSI_POWERDOWN) {
		struct lcm *ctx = panel_to_lcm(panel);

		lcm_dcs_write_seq_static(ctx, 0x2F, 0x02);
	}
}

static void mode_switch_to_90(struct drm_panel *panel,
	enum MTK_PANEL_MODE_SWITCH_STAGE stage)
{
	if (stage == BEFORE_DSI_POWERDOWN) {
		struct lcm *ctx = panel_to_lcm(panel);

		lcm_dcs_write_seq_static(ctx, 0x2F, 0x03);
	}
}

static void mode_switch_to_60(struct drm_panel *panel,
	enum MTK_PANEL_MODE_SWITCH_STAGE stage)
{
	if (stage == BEFORE_DSI_POWERDOWN) {
		struct lcm *ctx = panel_to_lcm(panel);

		lcm_dcs_write_seq_static(ctx, 0x2F, 0x01);
	}
}

static void mode_switch_to_48(struct drm_panel *panel,
	enum MTK_PANEL_MODE_SWITCH_STAGE stage)
{
	if (stage == BEFORE_DSI_POWERDOWN) {
		struct lcm *ctx = panel_to_lcm(panel);

		lcm_dcs_write_seq_static(ctx, 0x2F, 0x07);
	}
}

static int mode_switch(struct drm_panel *panel,
		struct drm_connector *connector, unsigned int cur_mode,
		unsigned int dst_mode, enum MTK_PANEL_MODE_SWITCH_STAGE stage)
{
	int ret = 0;
	struct drm_display_mode *m = get_mode_by_id(connector, dst_mode);

	if (cur_mode == dst_mode)
		return ret;

	if (drm_mode_vrefresh(m) == 48) { /*switch to 48 */
		mode_switch_to_48(panel, stage);
	} else if (drm_mode_vrefresh(m) == 60) { /*switch to 60 */
		mode_switch_to_60(panel, stage);
	} else if (drm_mode_vrefresh(m) == 90) { /*switch to 60 */
		mode_switch_to_90(panel, stage);
	} else if (drm_mode_vrefresh(m) == 120) { /*switch to 120 */
		mode_switch_to_120(panel, stage);
	} else if (drm_mode_vrefresh(m) == 144) { /*switch to 144 */
		mode_switch_to_144(panel, stage);
	} else
		ret = 1;

	return ret;
}

static struct mtk_panel_para_table panel_lhbm_on[] = {
	{6, {0xF0, 0x55, 0xAA, 0x52, 0x08, 0x07}},
	{2, {0xC0, 0x01}},
	{3, {0x51, 0x0F, 0xFF}},
	{3, {0x87, 0x1F, 0xFF}},
	{2, {0x88, 0x01}},
	{2, {0x6F, 0x01}},
	{5, {0x88, 0x02, 0x1C, 0x08, 0x74}},
	{2, {0x85, 0x01}},
};

static struct mtk_panel_para_table panel_lhbm_off[] = {
	{3, {0x51, 0x0F, 0xFF}},
	{3, {0x87, 0x0F, 0xFF}},
	{2, {0x88, 0x01}},
	{2, {0x86, 0x01}},
};

static void set_lhbm_alpha(unsigned int bl_level)
{
	struct mtk_panel_para_table *pTable = &panel_lhbm_on[3];
	unsigned int alpha = 0;
/*
	if((bl_level < 4095) && (bl_level > 3514))
		alpha = bl_level;
	else if((bl_level < 3515) && (bl_level > 1327))
		alpha = 9955 * bl_level / 10000 + 17;
	else
		alpha = (3*bl_level*bl_level + 4754*bl_level)/10000 + 267;
*/
	if (bl_level < ARRAY_SIZE(lhbm_alpha))
		alpha = lhbm_alpha[bl_level];
	else
		bl_level = ARRAY_SIZE(lhbm_alpha) -1;

	pTable->para_list[1] = (alpha >> 8) & 0xFF;
	pTable->para_list[2] = alpha & 0xFF;
	pr_info("%s: backlight %d alpha %d(0x%x, 0x%x)\n", __func__, bl_level, alpha, pTable->para_list[1], pTable->para_list[2]);
}

static int panel_lhbm_set_cmdq(void *dsi, dcs_grp_write_gce cb, void *handle, uint32_t on, uint32_t bl_level)
{
	unsigned int para_count = 0;
	struct mtk_panel_para_table *pTable;

	if (on) {
		set_lhbm_alpha(bl_level);
		para_count = sizeof(panel_lhbm_on) / sizeof(struct mtk_panel_para_table);
		pTable = panel_lhbm_on;
	} else {
		panel_lhbm_off[0].para_list[1] = (bl_level >> 8) & 0x0F;
		panel_lhbm_off[0].para_list[2] = bl_level & 0x0FF;
		para_count = sizeof(panel_lhbm_off) / sizeof(struct mtk_panel_para_table);
		pTable = panel_lhbm_off;
	}
	cb(dsi, handle, pTable, para_count);
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
				panel_lhbm_set_cmdq(dsi, cb, handle, 0, ctx->current_bl);
			break;
		case 1:
			if (ctx->lhbm_en)
				panel_lhbm_set_cmdq(dsi, cb, handle, 0, ctx->current_bl);
			cb(dsi, handle, &hbm_on_table, 1);
			break;
		case 2:
			if (ctx->lhbm_en)
				panel_lhbm_set_cmdq(dsi, cb, handle, 1, ctx->current_bl);
			else
				cb(dsi, handle, &hbm_on_table, 1);
			break;
		default:
			break;
	}

	return 0;
}

static struct mtk_panel_para_table panel_dc_off[] = {
   {6, {0xF0, 0x55, 0xAA, 0x52, 0x08, 0x00}},
   {37, {0xB3, 0x00,0x01,0x01,0x58,0x01,0x58,0x02,0x19,0x02,0x19,0x03,0x09,0x03,0x09,0x04,0x43,0x04,0x43,0x06,0x8D,0x06,0x8D,0x06,0x8E,0x06,0x8E,0x0B,0x31,0x0B,0x31,0x0D,0xBA,0x0D,0xBA,0x0F,0xFF}},
   {2, {0x6F, 0x24}},
   {9, {0xB3, 0x00, 0x02, 0x07, 0x5F, 0x07, 0x5F, 0x0F, 0xFE}},
   {2, {0x6F, 0x30}},
   {39, {0xB4,0x0C,0x0D,0x0C,0x0D,0x0B,0xB0,0x0B,0xB0,0x0B,0x12,0x0B,0x12,0x09,0xD9,0x09,0xD9,0x07,0x68,0x07,0x68,0x00,0x60,0x00,0x60,0x00,0x10,0x00,0x10,0x00,0x10,0x00,0x10,0x00,0x10,0x00,0x10,0x00,0x10}},
   {39, {0xB4,0x12,0x14,0x12,0x14,0x11,0x88,0x11,0x88,0x10,0x9C,0x10,0x9C,0x0E,0xC6,0x0E,0xC6,0x0B,0x1C,0x0B,0x1C,0x00,0x90,0x00,0x90,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x18}},
   {2, {0x6F, 0x26}},
   {11, {0xB4,0x04,0xA2,0x04,0xA2,0x04,0xA2,0x04,0xA2,0x04,0xA2}},
   {2, {0x6F, 0x2C}},
   {39, {0xB3,0x09,0x0A,0x09,0x0A,0x08,0xC4,0x08,0xC4,0x08,0x4E,0x08,0x4E,0x07,0x63,0x07,0x63,0x05,0x8E,0x05,0x8E,0x00,0x48,0x00,0x48,0x00,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x0C}},
   {2, {0x6F, 0x56}},
   {39, {0xB4,0x09,0x0A,0x09,0x0A,0x08,0xC4,0x08,0xC4,0x08,0x4E,0x08,0x4E,0x07,0x63,0x07,0x63,0x05,0x8E,0x05,0x8E,0x00,0x48,0x00,0x48,0x00,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x0C}},
   {2, {0xB2, 0x11}},
   {2, {0x6F, 0x0F}},
   {9, {0xB2, 0x60, 0x50, 0x36, 0x8D, 0x56, 0x8D, 0x2F, 0xFF}},
};

static struct mtk_panel_para_table panel_dc_on[] = {
   {6, {0xF0, 0x55, 0xAA, 0x52, 0x08, 0x00}},
   {2, {0x6F, 0xAC}},
   {21, {0xB2,0x00,0x00,0x02,0xA3,0x04,0x18,0x05,0x9D,0x07,0xB1,0x0A,0xAA,0x0F,0xFF,0x0F,0xFF,0x0F,0xFF,0x0F,0xFF}},
   {2, {0x6F, 0xC0}},
   {21, {0xB2,0x0F,0xFF,0x0F,0xFF,0x0F,0xFF,0x0F,0xFF,0x0F,0xFF,0x0F,0xFF,0x0F,0xFF,0x0F,0xFF,0x0F,0xFF,0x0F,0xFF}},
   {2, {0x6F, 0xD4}},
   {21, {0xB2,0x0F,0xFF,0x0F,0xFF,0x0F,0xFF,0x0F,0xFF,0x0F,0xFF,0x0F,0xFF,0x0F,0xFF,0x0F,0xFF,0x0F,0xFF,0x0F,0xFF}},
   {2, {0x6F, 0xE8}},
   {11, {0xB2,0x0F,0xFF,0x0F,0xFF,0x0F,0xFF,0x0F,0xFF,0x0F,0xFF}},
   {2, {0x6F, 0x52}},
   {21, {0xB3,0x00,0x01,0x01,0x58,0x02,0x19,0x03,0x09,0x04,0x43,0x06,0x8D,0x06,0x8E,0x0F,0xFF,0x0F,0xFF,0x0F,0xFF}},
   {2, {0x6F, 0x66}},
   {21, {0xB3,0x0F,0xFF,0x0F,0xFF,0x0F,0xFF,0x0F,0xFF,0x0F,0xFF,0x0F,0xFF,0x0F,0xFF,0x0F,0xFF,0x0F,0xFF,0x0F,0xFF}},
   {2, {0x6F, 0x7A}},
   {21, {0xB3,0x0F,0xFF,0x0F,0xFF,0x0F,0xFF,0x0F,0xFF,0x0F,0xFF,0x0F,0xFF,0x0F,0xFF,0x0F,0xFF,0x0F,0xFF,0x0F,0xFF}},
   {2, {0x6F, 0x8E}},
   {9, {0xB3,0x0F,0xFF,0x0F,0xFF,0x0F,0xFF,0x0F,0xFF}}, 
   {6, {0xF0, 0x55, 0xAA, 0x52, 0x08, 0x00}},
   {2, {0xB2, 0x91}},
   {2, {0x6F, 0x0F}},
   {9, {0xB2,0x60,0x50,0x30,0x00,0x50,0x00,0x2F,0xFF}}, 
   {37, {0xB3,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x06,0x8E,0x06,0x8E,0x0B,0x31,0x0B,0x31,0x0D,0xBA,0x0D,0xBA,0x0F,0xFF}}, 
   {2, {0x6F, 0x30}},
   {39, {0xB4,0x00,0x60,0x00,0x10,0x00,0x10,0x00,0x10,0x00,0x10,0x00,0x10,0x00,0x10,0x00,0x10,0x00,0x10,0x00,0x10,0x00,0x10,0x00,0x10,0x00,0x10,0x00,0x10,0x00,0x10,0x00,0x10,0x00,0x10,0x00,0x10,0x00,0x10}}, 
   {39, {0xB4,0x00,0x90,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x18,0x00,0x18}}, 
   {2, {0x6F, 0x2C}},
   {39, {0xB3,0x00,0x48,0x00,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x0C}}, 
   {2, {0x6F, 0x56}},
   {39, {0xB4,0x00,0x48,0x00,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x0C,0x00,0x0C}}, 
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

static int panel_ext_init_power(struct drm_panel *panel)
{
	int ret;
	struct lcm *ctx = panel_to_lcm(panel);

	ctx->reset_gpio = devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
	gpiod_set_value(ctx->reset_gpio, 0);
	devm_gpiod_put(ctx->dev, ctx->reset_gpio);
	msleep(10);

	ret = gate_ic_Power_on(panel, 1);
	return ret;
}

static int panel_ext_powerdown(struct drm_panel *panel)
{
	struct lcm *ctx = panel_to_lcm(panel);

	pr_info("%s+\n", __func__);
	if (ctx->prepared)
	    return 0;
#if 0
	ctx->reset_gpio = devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
	gpiod_set_value(ctx->reset_gpio, 0);
	devm_gpiod_put(ctx->dev, ctx->reset_gpio);
#endif
	gate_ic_Power_on(panel, 0);

	return 0;
}

static struct mtk_panel_funcs ext_funcs = {
	.reset = panel_ext_reset,
	.set_backlight_cmdq = lcm_setbacklight_cmdq,
	.init_power = panel_ext_init_power,
	.power_down = panel_ext_powerdown,
	.ata_check = panel_ata_check,
	.ext_param_set = mtk_panel_ext_param_set,
	.mode_switch = mode_switch,
	.panel_feature_set = panel_feature_set,
};
#endif

static int lcm_get_modes(struct drm_panel *panel,
					struct drm_connector *connector)
{
	struct drm_display_mode *mode;
	struct drm_display_mode *mode_1;
	struct drm_display_mode *mode_2;
	struct drm_display_mode *mode_3;
#ifdef SUPPORT_144HZ_REFRESH_RATE
	struct drm_display_mode *mode_4;
#endif

	mode = drm_mode_duplicate(connector->dev, &switch_mode_48hz);
	if (!mode) {
		dev_info(connector->dev->dev, "failed to add mode %ux%ux@%u\n",
			 switch_mode_48hz.hdisplay, switch_mode_48hz.vdisplay,
			 drm_mode_vrefresh(&switch_mode_48hz));
		return -ENOMEM;
	}
	drm_mode_set_name(mode);
	mode->type = DRM_MODE_TYPE_DRIVER;
	drm_mode_probed_add(connector, mode);

	mode_1 = drm_mode_duplicate(connector->dev, &switch_mode_60hz);
	if (!mode_1) {
		dev_info(connector->dev->dev, "failed to add mode %ux%ux@%u\n",
			 switch_mode_60hz.hdisplay, switch_mode_60hz.vdisplay,
			 drm_mode_vrefresh(&switch_mode_60hz));
		return -ENOMEM;
	}
	drm_mode_set_name(mode_1);
	mode_1->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	drm_mode_probed_add(connector, mode_1);

	mode_2 = drm_mode_duplicate(connector->dev, &switch_mode_90hz);
	if (!mode_2) {
		dev_info(connector->dev->dev, "failed to add mode %ux%ux@%u\n",
			switch_mode_90hz.hdisplay, switch_mode_90hz.vdisplay,
			drm_mode_vrefresh(&switch_mode_90hz));
		return -ENOMEM;
	}
	drm_mode_set_name(mode_2);
	mode_2->type = DRM_MODE_TYPE_DRIVER;
	drm_mode_probed_add(connector, mode_2);

	mode_3 = drm_mode_duplicate(connector->dev, &switch_mode_120hz);
	if (!mode_3) {
		dev_info(connector->dev->dev, "failed to add mode %ux%ux@%u\n",
			switch_mode_120hz.hdisplay, switch_mode_120hz.vdisplay,
			drm_mode_vrefresh(&switch_mode_120hz));
		return -ENOMEM;
	}
	drm_mode_set_name(mode_3);
	mode_3->type = DRM_MODE_TYPE_DRIVER;
	drm_mode_probed_add(connector, mode_3);
#ifdef SUPPORT_144HZ_REFRESH_RATE
	mode_4 = drm_mode_duplicate(connector->dev, &switch_mode_144hz);
	if (!mode_4) {
		dev_info(connector->dev->dev, "failed to add mode %ux%ux@%u\n",
			switch_mode_144hz.hdisplay, switch_mode_144hz.vdisplay,
			drm_mode_vrefresh(&switch_mode_144hz));
		return -ENOMEM;
	}
	drm_mode_set_name(mode_4);
	mode_4->type = DRM_MODE_TYPE_DRIVER;
	drm_mode_probed_add(connector, mode_4);
#endif
	connector->display_info.width_mm = 68;
	connector->display_info.height_mm = 152;

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
	g_ctx = ctx;
	ctx->dev = dev;
	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags = MIPI_DSI_MODE_LPM | MIPI_DSI_MODE_EOT_PACKET
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
	drm_panel_init(&ctx->panel, dev, &lcm_drm_funcs, DRM_MODE_CONNECTOR_DSI);

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

	ctx->lhbm_en = of_property_read_bool(dev->of_node, "lhbm-enable");

	pr_info("%s- lcm,csot, nt37701,cmd,120hz, lhbm_en = %d\n", __func__, ctx->lhbm_en);

	return ret;
}

static int lcm_remove(struct mipi_dsi_device *dsi)
{
	struct lcm *ctx = mipi_dsi_get_drvdata(dsi);
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

static const struct of_device_id lcm_of_match[] = {
	{
		.compatible = "boe,nt37701a,cmd,144hz",
	},
	{}
};

MODULE_DEVICE_TABLE(of, lcm_of_match);

static struct mipi_dsi_driver lcm_driver = {
	.probe = lcm_probe,
	.remove = lcm_remove,
	.driver = {
		.name = "boe_nt37701A_655_1080x2400",
		.owner = THIS_MODULE,
		.of_match_table = lcm_of_match,
	},
};

module_mipi_dsi_driver(lcm_driver);

MODULE_AUTHOR("MEDIATEK");
MODULE_DESCRIPTION("BOE nt37701 AMOLED CMD SPR LCD Panel Driver");
MODULE_LICENSE("GPL v2");
