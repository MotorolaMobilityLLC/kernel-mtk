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
#include "include/dsi-panel-mot-csot-ft8726-fhdp-dphy-vdo-120hz.h"
#endif

/* option function to read data from some panel address */
/* #define PANEL_SUPPORT_READBACK */

extern int __attribute__ ((weak)) ocp2138_BiasPower_disable(u32 pwrdown_delay);
extern int __attribute__ ((weak)) ocp2138_BiasPower_enable(u32 avdd, u32 avee,u32 pwrup_delay);
static int tp_gesture_flag=0;

struct csot {
	struct device *dev;
	struct drm_panel panel;
	struct backlight_device *backlight;
	struct gpio_desc *pm_enable_gpio;
	struct gpio_desc *reset_gpio;

	bool prepared;
	bool enabled;

	int error;
	unsigned int hbm_mode;
	unsigned int cabc_mode;
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
	ctx->reset_gpio = devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio)) {
		dev_err(ctx->dev, "%s: cannot get reset_gpio %ld\n",
			__func__, PTR_ERR(ctx->reset_gpio));
		return;
	}
	gpiod_set_value(ctx->reset_gpio, 1);
	usleep_range(5000,5001);
	gpiod_set_value(ctx->reset_gpio, 0);
	usleep_range(5000,5001);
	gpiod_set_value(ctx->reset_gpio, 1);
	usleep_range(20000,20001);

	devm_gpiod_put(ctx->dev, ctx->reset_gpio);

	csot_dcs_write_seq_static(ctx, 0x00, 0x00);
	csot_dcs_write_seq_static(ctx, 0xFF, 0x87,0x20,0x01);
	csot_dcs_write_seq_static(ctx, 0x00, 0x80);
	csot_dcs_write_seq_static(ctx, 0xFF, 0x87,0x20);
	csot_dcs_write_seq_static(ctx, 0x00, 0x91);
	csot_dcs_write_seq_static(ctx, 0xC4, 0x08);
	csot_dcs_write_seq_static(ctx, 0x00, 0xFC);
	csot_dcs_write_seq_static(ctx, 0xC0, 0x00);
	csot_dcs_write_seq_static(ctx, 0x00, 0xFD);
	csot_dcs_write_seq_static(ctx, 0xC0, 0x16);
	csot_dcs_write_seq_static(ctx, 0x00, 0xB0);
	csot_dcs_write_seq_static(ctx, 0xB4, 0x00,0x08,0x02,0x00,0x00,0xBB,0x00,0x07,0x0D,0xB7,0x0C,0xB7,0x10,0xF0);
	csot_dcs_write_seq_static(ctx, 0x00, 0x00);
	csot_dcs_write_seq_static(ctx, 0xFF, 0xFF,0xFF,0xFF);
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


	if (!ctx->enabled)
		return 0;

	if (ctx->backlight) {
		ctx->backlight->props.power = FB_BLANK_POWERDOWN;
		backlight_update_status(ctx->backlight);
	}

	ctx->enabled = false;

	return 0;
}

static int csot_set_gesture_flag(int state)
{
	if(state == 1)
	{
		tp_gesture_flag = 1;
	}
	else
	{
		tp_gesture_flag = 0;
	}
	return 0;
}

static int csot_unprepare(struct drm_panel *panel)
{
	struct csot *ctx = panel_to_csot(panel);
	int ret=0;

	if (!ctx->prepared)
		return 0;
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

	if(tp_gesture_flag == 0)
	{
	ctx->reset_gpio = devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio)) {
		dev_err(ctx->dev, "%s: cannot get reset_gpio %ld\n",
			__func__, PTR_ERR(ctx->reset_gpio));
		return -1;
	}
	gpiod_set_value(ctx->reset_gpio, 1);
	msleep(5);
	devm_gpiod_put(ctx->dev, ctx->reset_gpio);

	ret = ocp2138_BiasPower_disable(5);
	}


	ctx->error = 0;
	ctx->prepared = false;
	return 0;
}

static int csot_prepare(struct drm_panel *panel)
{
	struct csot *ctx = panel_to_csot(panel);
	int ret;

	pr_info("%s\n", __func__);
	if (ctx->prepared)
		return 0;

	gpiod_set_value(ctx->reset_gpio, 0);
	msleep(3);
	ret = ocp2138_BiasPower_enable(15,15,5);

	//lcm_power_enable();
	csot_panel_init(ctx);

	ret = ctx->error;
	if (ret < 0)
		csot_unprepare(panel);

	ctx->prepared = true;

	ctx->hbm_mode = 0;
	ctx->cabc_mode = 0;

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
	.clock = 343022,
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
	.clock = 343231,
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
	.clock = 343301,
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
	.data_rate = 900,
	//.vfp_low_power = 840,
	.cust_esd_check = 1,
	.esd_check_enable = 1,
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
#if 0
	.dyn = {
		.switch_en = 1,
		.pll_clk = 580,
		.vfp_lp_dyn = 888,
		.hfp = 800,
		.vfp = 54,
	},
	.dyn_fps = {
		.dfps_cmd_table[0] = {0, 2, {0xFF, 0x25} },
		.dfps_cmd_table[1] = {0, 2, {0xFB, 0x01} },
		.dfps_cmd_table[2] = {0, 2, {0x18, 0x22} },
		/*switch page for esd check*/
		.dfps_cmd_table[3] = {0, 2, {0xFF, 0x10} },
		.dfps_cmd_table[4] = {0, 2, {0xFB, 0x01} },
	},
	.data_rate = DATA_RATE,
	.lfr_enable = LFR_EN,
	.lfr_minimum_fps = MODE_0_FPS,
#endif
};

static struct mtk_panel_params ext_params_mode_1 = {
//	.vfp_low_power = 1302,//60hz
	.data_rate = 900,
	.cust_esd_check = 1,
	.esd_check_enable = 1,
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
#if 0
	.dyn = {
		.switch_en = 1,
		.pll_clk = 580,
		.vfp_lp_dyn = 1300,
		.hfp = 372,
		.vfp = 54,
	},
	.dyn_fps = {
		.dfps_cmd_table[0] = {0, 2, {0xFF, 0x25} },
		.dfps_cmd_table[1] = {0, 2, {0xFB, 0x01} },
		.dfps_cmd_table[2] = {0, 2, {0x18, 0x21} },
		/*switch page for esd check*/
		.dfps_cmd_table[3] = {0, 2, {0xFF, 0x10} },
		.dfps_cmd_table[4] = {0, 2, {0xFB, 0x01} },
	},
	.data_rate = DATA_RATE,
	.lfr_enable = LFR_EN,
	.lfr_minimum_fps = MODE_0_FPS,
#endif
};

static struct mtk_panel_params ext_params_mode_2 = {
//	.vfp_low_power = 2558,//60hz
	.data_rate = 900,
	.cust_esd_check = 1,
	.esd_check_enable = 1,
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
#if 0
	.dyn = {
		.switch_en = 1,
		.pll_clk = 580,
		.vfp_lp_dyn = 2540,
		.hfp = 157,
		.vfp = 54,
	},
	.dyn_fps = {
		.dfps_cmd_table[0] = {0, 2, {0xFF, 0x25} },
		.dfps_cmd_table[1] = {0, 2, {0xFB, 0x01} },
		.dfps_cmd_table[2] = {0, 2, {0x18, 0x20} },
		/*switch page for esd check*/
		.dfps_cmd_table[3] = {0, 2, {0xFF, 0x10} },
		.dfps_cmd_table[4] = {0, 2, {0xFB, 0x01} },
	},
	.data_rate = DATA_RATE,
	.lfr_enable = LFR_EN,
	.lfr_minimum_fps = MODE_0_FPS,
	#endif
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

static struct mtk_panel_funcs ext_funcs = {
//	.set_backlight_cmdq = csot_setbacklight_cmdq,
	.reset = panel_ext_reset,
	.ext_param_set = mtk_panel_ext_param_set,
//	.ata_check = panel_ata_check,
	.set_gesture_flag = csot_set_gesture_flag,
	.get_lcm_version = mtk_panel_get_lcm_version,
	.panel_feature_set = panel_feature_set,
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

	pr_info("%s+ csot,FT8726,vdo,120hz\n", __func__);

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

	ctx->hbm_mode = 0;
	ctx->cabc_mode = 0;

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

	pr_info("[%d  %s]- csot,FT8726,vdo,120hz  ret:%d  \n", __LINE__, __func__,ret);

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
		.compatible = "csot,FT8726,vdo,120hz",
	},
	{} };

MODULE_DEVICE_TABLE(of, csot_of_match);

static struct mipi_dsi_driver csot_driver = {
	.probe = csot_probe,
	.remove = csot_remove,
	.driver = {
		.name = "panel-csot-FT8726-vdo-120hz",
		.owner = THIS_MODULE,
		.of_match_table = csot_of_match,
	},
};

module_mipi_dsi_driver(csot_driver);

MODULE_AUTHOR("Jingjing Liu <jingjing.liu@mediatek.com>");
MODULE_DESCRIPTION("csot FT8726 incell 120hz Panel Driver");
MODULE_LICENSE("GPL v2");

