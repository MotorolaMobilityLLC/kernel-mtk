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
#include "include/dsi-panel-mot-csot-icnl9922c-672-fhdp-dphy-vdo-120hz.h"
#endif

#define PANEL_LDO_VTP_EN

#ifdef PANEL_LDO_VTP_EN
#include <linux/gpio/consumer.h>
#include <linux/regulator/consumer.h>
#endif

/* option function to read data from some panel address */
/* #define PANEL_SUPPORT_READBACK */

#define BIAS_SM5109
#ifdef BIAS_SM5109
extern int __attribute__ ((weak)) sm5109_BiasPower_disable(u32 pwrdown_delay);
extern int __attribute__ ((weak)) sm5109_BiasPower_enable(u32 avdd, u32 avee,u32 pwrup_delay);
#endif

static int tp_gesture_flag = 0;

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
	unsigned int hbm_mode;
	unsigned int cabc_mode;
};

static struct mtk_panel_para_table panel_cabc_ui[] = {
	{4, {0xF0, 0x99, 0x22, 0x0C}},
	{32,{0xE1, 0x0F, 0x1F, 0x2F, 0x3F, 0x4F, 0x5F, 0x6F, 0x7F, 0x90, 0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xE8, 0xEC, 0xED, 0xEF, 0xF1, 0xF3, 0xF5, 0xF7, 0xF9, 0xFB, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF, 0xFF, 0xFF}},
	{33,{0xE2, 0x36, 0x24, 0x37, 0x4B, 0x38, 0x72, 0x39, 0x99, 0x3A, 0x1C, 0x3A, 0x9F, 0x3B, 0x22, 0x3B, 0xA5, 0x3C, 0x28, 0x3C, 0xED, 0x3D, 0xB1, 0x3E, 0x76, 0x3F, 0x3A, 0x3F, 0xFF, 0x3F, 0xFF, 0x3F, 0xFF}},
	{4, {0xF0, 0x00, 0x00, 0x00}},
	{2, {0x55, 0x03}},
};

static struct mtk_panel_para_table panel_cabc_mv[] = {
	{4, {0xF0, 0x99, 0x22, 0x0C}},
	{32,{0xE1, 0x0F, 0x1F, 0x2F, 0x3F, 0x4F, 0x5F, 0x6F, 0x7F, 0x90, 0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xE8, 0xE7, 0xE7, 0xE7, 0xE7, 0xE9, 0xEB, 0xEC, 0xED, 0xF0, 0xF0, 0xF3, 0xF6, 0xF9, 0xFD, 0xFF, 0xFF}},
	{33,{0xE2, 0x2F, 0xFF, 0x2F, 0xFF, 0x2F, 0xFF, 0x2F, 0xFF, 0x31, 0x47, 0x32, 0x8F, 0x33, 0xD6, 0x35, 0x1E, 0x36, 0x66, 0x37, 0xEF, 0x39, 0x78, 0x3B, 0x01, 0x3C, 0x8A, 0x3E, 0x14, 0x3F, 0xFF, 0x3F, 0xFF}},
	{4, {0xF0, 0x00, 0x00, 0x00}},
	{2, {0x55, 0x03}},
};

static struct mtk_panel_para_table panel_cabc_disable[] = {
	{2, {0x55, 0x00}},
};

static struct mtk_panel_para_table panel_hbm_on[] = {
	{3, {0x51, 0x07, 0xFF}},
};

static struct mtk_panel_para_table panel_hbm_off[] = {
	{3, {0x51, 0x06, 0x66}},
};

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

#ifdef PANEL_LDO_VTP_EN
static struct regulator *reg_vtp_1p8;
static unsigned int lcm_get_reg_vtp_1p8(void)
{
	unsigned int volt = 0;

	if (regulator_is_enabled(reg_vtp_1p8))
		volt = regulator_get_voltage(reg_vtp_1p8);

	return volt;
}

static unsigned int lcm_enable_reg_vtp_1p8(int en)
{
	unsigned int ret = 0, volt = 0;
	static bool vddio_enable_flg = false;

	pr_info("[lcd_info]%s +\n", __func__);
	if (en) {
		if (!vddio_enable_flg) {
			if (!IS_ERR_OR_NULL(reg_vtp_1p8)) {
				ret = regulator_set_voltage(reg_vtp_1p8, 1800000, 1800000);
				if (ret < 0)
					pr_info("set voltage reg_vtp_1p8 fail, ret = %d\n", ret);

				ret = regulator_enable(reg_vtp_1p8);
				pr_info("[lh]Enable the Regulator vufs1p8 ret=%d.\n", ret);
				volt = lcm_get_reg_vtp_1p8();
				pr_info("[lh]get the Regulator vufs1p8 =%d.\n", volt);
				vddio_enable_flg = true;
			}
		}
	} else {
		if (vddio_enable_flg) {
			if (!IS_ERR_OR_NULL(reg_vtp_1p8)) {
				ret = regulator_disable(reg_vtp_1p8);
				pr_info("[lh]disable the Regulator vufs1p8 ret=%d.\n", ret);
				vddio_enable_flg = false;
			}
		}
	}

	pr_info("[lcd_info]%s -\n", __func__);

	return ret;
}
#endif

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
		pr_info("disp: %s 0x%08x\n", __func__, buffer[0] | (buffer[1] << 8));
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

	ctx->reset_gpio = devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio)) {
		dev_err(ctx->dev, "%s: cannot get reset_gpio %ld\n",
			__func__, PTR_ERR(ctx->reset_gpio));
		//return;
	}
	else {
		gpiod_set_value(ctx->reset_gpio, 1);
		msleep(2);
		gpiod_set_value(ctx->reset_gpio, 0);
		msleep(5);
		gpiod_set_value(ctx->reset_gpio, 1);
		msleep(15);
		devm_gpiod_put(ctx->dev, ctx->reset_gpio);
		pr_info("disp: %s reset_gpio\n", __func__);
	}

	csot_dcs_write_seq_static(ctx, 0xF0, 0x99, 0x22, 0x0C);
	csot_dcs_write_seq_static(ctx, 0xB6, 0x82, 0x82, 0x82, 0x1C, 0x1C, 0x1D, 0x1D, 0x1E, 0x1E, 0x82, 0x01, 0x82, 0xC0, 0x82, 0x00, 0x29, 0x0D, 0x0F, 0x11, 0x13, 0x82, 0xC1, 0x82, 0xC0, 0xFF, 0xD6, 0x0A, 0x00, 0x00, 0x00, 0x3C, 0x00);
	csot_dcs_write_seq_static(ctx, 0xB7, 0x82, 0x82, 0x82, 0x1C, 0x1C, 0x1D, 0x1D, 0x1E, 0x1E, 0x82, 0x01, 0x82, 0xC0, 0x82, 0x00, 0x28, 0x0C, 0x0E, 0x10, 0x12, 0x82, 0xC1, 0x82, 0xC0, 0xFF, 0xD6, 0x0A, 0x00, 0x00, 0x00, 0x3C, 0x00);
	csot_dcs_write_seq_static(ctx, 0xD0, 0x0C, 0x23, 0x18, 0xFF, 0xFF, 0x00, 0x80, 0x0C, 0xFF, 0x0F, 0x40, 0x00, 0x08, 0x10);
	csot_dcs_write_seq_static(ctx, 0xE0, 0x0C, 0x00, 0xB0, 0x0C, 0x00, 0x15, 0x7C, 0x29, 0x04, 0x01, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x24, 0x12, 0x08, 0x91);
	csot_dcs_write_seq_static(ctx, 0xE1, 0x0F, 0x1F, 0x2F, 0x3F, 0x4F, 0x5F, 0x6F, 0x7F, 0x90, 0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xE8, 0xEC, 0xED, 0xEF, 0xF1, 0xF3, 0xF5, 0xF7, 0xF9, 0xFB, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF, 0xFF, 0xFF);
	csot_dcs_write_seq_static(ctx, 0xE2, 0x36, 0x24, 0x37, 0x4B, 0x38, 0x72, 0x39, 0x99, 0x3A, 0x1C, 0x3A, 0x9F, 0x3B, 0x22, 0x3B, 0xA5, 0x3C, 0x28, 0x3C, 0xED, 0x3D, 0xB1, 0x3E, 0x76, 0x3F, 0x3A, 0x3F, 0xFF, 0x3F, 0xFF, 0x3F, 0xFF);
	csot_dcs_write_seq_static(ctx, 0xF0, 0x00, 0x00, 0x00);

	csot_dcs_write_seq_static(ctx, 0x35, 0x00, 0x00);

	csot_dcs_write_seq_static(ctx, 0x51, 0x06, 0x66);
	csot_dcs_write_seq_static(ctx, 0x53, 0x2C, 0x00);
	csot_dcs_write_seq_static(ctx, 0x55, 0x03, 0x00);
	csot_dcs_write_seq_static(ctx, 0x11, 0x00, 0x00);
	msleep(90);
	csot_dcs_write_seq_static(ctx, 0x29, 0x00, 0x00);
 	msleep(10);
	csot_dcs_write_seq_static(ctx, 0xAC, 0x05, 0x00);
	//csot_dcs_write_seq(ctx, bl_tb0[0], bl_tb0[1]);

	pr_info("%s-\n", __func__);
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

static int panel_set_gesture_flag(int state)
{
	if(state == 1)
		tp_gesture_flag = 1;
	else
		tp_gesture_flag = 0;

	pr_info("%s:disp:set tp_gesture_flag:%d\n", __func__, tp_gesture_flag);
	return 0;
}

static int csot_unprepare(struct drm_panel *panel)
{
	struct csot *ctx = panel_to_csot(panel);

	if (!ctx->prepared) {
		pr_info("%s, already unprepared, return\n", __func__);
		return 0;
	}
	pr_info("%s\n", __func__);
	csot_dcs_write_seq_static(ctx, 0xF0, 0x99, 0x22, 0x0C);
	csot_dcs_write_seq_static(ctx, 0xBB, 0x01, 0x02, 0x03, 0x0A, 0x04, 0x13, 0x14, 0x12, 0x16, 0x5C, 0x00, 0x15, 0x16, 0x00);
	csot_dcs_write_seq_static(ctx, 0xF0, 0x00, 0x00, 0x00);
	csot_dcs_write_seq_static(ctx, 0xAC, 0x0A, 0x00);

	csot_dcs_write_seq_static(ctx, 0x28, 0x00, 0x00);
	msleep(10);
	csot_dcs_write_seq_static(ctx, 0x10, 0x00, 0x00);
	msleep(120);
	usleep_range(3 * 1000, 8 * 1000);

	pr_info("%s:disp: tp_gesture_flag:%d\n",__func__, tp_gesture_flag);
	if(!tp_gesture_flag) {
#ifdef BIAS_SM5109
		sm5109_BiasPower_disable(5);
#endif

#ifdef PANEL_LDO_VTP_EN
		//lcm_enable_reg_vtp_1p8(0);
#endif
	}

	ctx->error = 0;
	ctx->prepared = false;

	return 0;
}

static int csot_prepare(struct drm_panel *panel)
{
	struct csot *ctx = panel_to_csot(panel);
	int ret;

	pr_info("%s+\n", __func__);
	if (ctx->prepared) {
		pr_info("%s, already prepared, return\n", __func__);
		return 0;
	}

#ifdef PANEL_LDO_VTP_EN
	ret = lcm_enable_reg_vtp_1p8(1);
	usleep_range(4 * 1000, 8 * 1000);
#endif
#ifdef BIAS_SM5109
	sm5109_BiasPower_enable(15,15,5);
	msleep(1);
#endif
	csot_panel_init(ctx);
	ctx->hbm_mode = 0;
	ctx->cabc_mode = 0;

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
	pr_info("disp: %s-\n", __func__);
	return ret;
}

static int csot_enable(struct drm_panel *panel)
{
	struct csot *ctx = panel_to_csot(panel);

	pr_info("disp: %s+\n", __func__);
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
	.clock = 332506,
	.hdisplay = FRAME_WIDTH,
	.hsync_start = FRAME_WIDTH + MODE_120_HFP,
	.hsync_end = FRAME_WIDTH + MODE_120_HFP + HSA,
	.htotal = FRAME_WIDTH + MODE_120_HFP + HSA + HBP,
	.vdisplay = FRAME_HEIGHT,
	.vsync_start = FRAME_HEIGHT + MODE_120_VFP,
	.vsync_end = FRAME_HEIGHT + MODE_120_VFP + VSA,
	.vtotal = FRAME_HEIGHT + MODE_120_VFP + VSA + VBP,
#if 0
	.hsync_start = 1080 + 16,//HFP
	.hsync_end = 1080 + 16 + 4,//HSA
	.htotal = 1080 + 16 + 4 + 16,//HBP
	.vdisplay = 2400,
	.vsync_start = 2400 + 38,//VFP
	.vsync_end = 2400 + 38 + 4,//VSA
	.vtotal = 2400 + 38 + 4 + 32,//VBP
#endif
};

#if 1
static const struct drm_display_mode performance_mode_30hz = {
	.clock		= 332774,
	.hdisplay = FRAME_WIDTH,
	.hsync_start = FRAME_WIDTH + MODE_30_HFP,
	.hsync_end = FRAME_WIDTH + MODE_30_HFP + HSA,
	.htotal = FRAME_WIDTH + MODE_30_HFP + HSA + HBP,
	.vdisplay = FRAME_HEIGHT,
	.vsync_start = FRAME_HEIGHT + MODE_30_VFP,
	.vsync_end = FRAME_HEIGHT + MODE_30_VFP + VSA,
	.vtotal = FRAME_HEIGHT + MODE_30_VFP + VSA + VBP,
};

static const struct drm_display_mode performance_mode_90hz = {
	.clock		= 332741,
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
	.clock		= 332774,
	.hdisplay = FRAME_WIDTH,
	.hsync_start = FRAME_WIDTH + MODE_60_HFP,
	.hsync_end = FRAME_WIDTH + MODE_60_HFP + HSA,
	.htotal = FRAME_WIDTH + MODE_60_HFP + HSA + HBP,
	.vdisplay = FRAME_HEIGHT,
	.vsync_start = FRAME_HEIGHT + MODE_60_VFP,
	.vsync_end = FRAME_HEIGHT + MODE_60_VFP + VSA,
	.vtotal = FRAME_HEIGHT + MODE_60_VFP + VSA + VBP,
};
#endif

#if defined(CONFIG_MTK_PANEL_EXT)
static struct mtk_panel_params ext_params = {
	.change_fps_by_vfp_send_cmd = 0,
	//.vfp_low_power = 20,
	.phy_timcon.lpx = 8,
	.cust_esd_check = 1,
	.esd_check_enable = 1,
	.lcm_esd_check_table[0] = {
		.cmd = 0x0a,
		.count = 1,
		.para_list[0] = 0x9c,
	},
	.panel_ver = 1,
	.panel_id = 0x080a2c93,
	.panel_name = "csot_icnl9922c_vid_672_1080",
	.panel_supplier = "csot",
	.lcm_index = 2,
	.hbm_type = HBM_MODE_DCS_I2C,
	.max_bl_level = 2047,
	.ssc_enable = 0,
	.lane_swap_en = 0,
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
	//.data_rate_khz = 808000,
	.data_rate = 832,
	.lfr_enable = 0,
	.lfr_minimum_fps = MODE_120_FPS,
	.dyn_fps = {
		.switch_en = 1,
		.vact_timing_fps = 120,
		//.dfps_cmd_table[0] = {0, 4, {0xB9, 0x83, 0x10, 0x21} },
		//.dfps_cmd_table[1] = {0, 2, {0xE2, 0x00} },
		//.dfps_cmd_table[2] = {0, 2, {0xB9, 0x00} },
	},
	/* following MIPI hopping parameter might cause screen mess */
	.dyn = {
		.switch_en = 1,
		.hfp = 169,
		.data_rate = 1124,
	},
};

#if 1
static struct mtk_panel_params ext_params_mode_30 = {
//	.vfp_low_power = 7476,//30hz
	.data_rate = 832,
	.phy_timcon.lpx = 8,
	.cust_esd_check = 1,
	.esd_check_enable = 1,
	.lcm_esd_check_table[0] = {
		.cmd = 0x0a,
		.count = 1,
		.para_list[0] = 0x9c,
	},
	.panel_ver = 1,
	.panel_id = 0x10050753,
	.panel_name = "csot_icnl9922c_vid_672_1080",
	.panel_supplier = "csot",
	.lcm_index = 2,
	.hbm_type = HBM_MODE_DCS_I2C,
	.max_bl_level = 2047,
	.ssc_enable = 1,
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
	},
	.dyn_fps = {
		.switch_en = 1,
		.vact_timing_fps = 30,
	},
	/* following MIPI hopping parameter might cause screen mess */
	.dyn = {
		.switch_en = 1,
		.hfp = 169,
		.data_rate = 1124,
	},
};

static struct mtk_panel_params ext_params_mode_90 = {
//	.vfp_low_power = 7476,//30hz
	.data_rate = 832,
	.phy_timcon.lpx = 8,
	.cust_esd_check = 1,
	.esd_check_enable = 1,
	.lcm_esd_check_table[0] = {
		.cmd = 0x0a,
		.count = 1,
		.para_list[0] = 0x9c,
	},
	.panel_ver = 1,
	.panel_id = 0x10050753,
	.panel_name = "csot_icnl9922c_vid_672_1080",
	.panel_supplier = "csot",
	.lcm_index = 2,
	.hbm_type = HBM_MODE_DCS_I2C,
	.max_bl_level = 2047,
	.ssc_enable = 1,
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
	},
	.lfr_enable = LFR_EN,
	.lfr_minimum_fps = MODE_90_FPS,
	.dyn_fps = {
		.switch_en = 1,
		.vact_timing_fps = 90,
	},
	/* following MIPI hopping parameter might cause screen mess */
	.dyn = {
		.switch_en = 1,
		.hfp = 169,
		.data_rate = 1124,
	},
};

static struct mtk_panel_params ext_params_mode_60 = {
//	.vfp_low_power = 7476,//30hz
	.data_rate = 832,
	.phy_timcon.lpx = 8,
	.cust_esd_check = 1,
	.esd_check_enable = 1,
	.lcm_esd_check_table[0] = {
		.cmd = 0x0a,
		.count = 1,
		.para_list[0] = 0x9c,
	},
	.panel_ver = 1,
	.panel_id = 0x10050753,
	.panel_name = "csot_icnl9922c_vid_672_1080",
	.panel_supplier = "csot",
	.lcm_index = 2,
	.hbm_type = HBM_MODE_DCS_I2C,
	.max_bl_level = 2047,
	.ssc_enable = 1,
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
	},
	.lfr_enable = LFR_EN,
	.lfr_minimum_fps = MODE_60_FPS,
	.dyn_fps = {
		.switch_en = 1,
		.vact_timing_fps = 60,
	},
	/* following MIPI hopping parameter might cause screen mess */
	.dyn = {
		.switch_en = 1,
		.hfp = 169,
		.data_rate = 1124,
	},
};
#endif

static int csot_setbacklight_cmdq(void *dsi, dcs_write_gce cb,
	void *handle, unsigned int level)
{
	pr_info("%s: skip for using bl ic, level=%d\n", __func__, level);

#if 0

	if (!cb) {
		pr_info("%s cb NULL!\n", __func__);
		return -1;
	}

	bl_tb0[1] = (u8)(level&0xFF);
	bl_tb0[2] = (u8)((level>>8)&0x7);

	pr_info("%s set level:%d, bl_tb:0x%02x%02x\n", __func__, level, bl_tb0[1], bl_tb0[2]);
	cb(dsi, handle, bl_tb0, ARRAY_SIZE(bl_tb0));
#endif

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

	if (!m)
		return ret;
	printk("[lcd_info][%s] switch fps to %d\n",__func__,drm_mode_vrefresh(m));
	pr_info("%s:disp: mode fps=%d", __func__, drm_mode_vrefresh(m));
	if (drm_mode_vrefresh(m) == MODE_120_FPS)
		ext->params = &ext_params;
#if 1
	else if (drm_mode_vrefresh(m) == MODE_30_FPS)
		ext->params = &ext_params_mode_30;
	else if (drm_mode_vrefresh(m) == MODE_90_FPS)
		ext->params = &ext_params_mode_90;
	else if (drm_mode_vrefresh(m) == MODE_60_FPS)
		ext->params = &ext_params_mode_60;
#endif
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

static enum mtk_lcm_version panel_get_lcm_version(void)
{
	return MTK_LEGACY_LCM_DRV_WITH_BACKLIGHTCLASS;
}

static int panel_cabc_set_cmdq(struct csot *ctx, void *dsi, dcs_grp_write_gce cb, void *handle, uint32_t cabc_mode)
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

static int panel_hbm_set_cmdq(struct csot *ctx, void *dsi, dcs_grp_write_gce cb, void *handle, uint32_t hbm_state)
{
	unsigned int para_count = 0;
	struct mtk_panel_para_table *pTable = NULL;

	if (hbm_state > 1) {
		pr_info("%s: invalid hbm_state:%d, return\n", __func__, hbm_state);
		return -1;
	}

	switch (hbm_state) {
		case 1:
			para_count = sizeof(panel_hbm_on) / sizeof(struct mtk_panel_para_table);
			pTable = panel_hbm_on;
			pr_info("%s: set HBM on", __func__);
			break;
		case 0:
			para_count = sizeof(panel_hbm_off) / sizeof(struct mtk_panel_para_table);
			pTable = panel_hbm_off;
			pr_info("%s: set HBM off", __func__);
			break;
		default:
			break;
	}

	if (pTable) {
		cb(dsi, handle, pTable, para_count);
	}
	else
		pr_info("%s: HBM pTable null, hbm_state:%s", __func__, hbm_state);

	return 0;
}

static int panel_feature_set(struct drm_panel *panel, void *dsi,
			      dcs_grp_write_gce cb, void *handle, struct panel_param_info param_info)
{
	struct csot *ctx = panel_to_csot(panel);
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
			if (ctx->cabc_mode != param_info.value) {
				ctx->cabc_mode = param_info.value;
				panel_cabc_set_cmdq(ctx, dsi, cb, handle, param_info.value);
				pr_debug("%s: set CABC to %d end\n", __func__, param_info.value);
				ret = 0;
			}
			else
				pr_info("%s: skip same CABC mode:%d\n", __func__, ctx->cabc_mode);
			break;
		case PARAM_HBM:
			if (ctx->hbm_mode != param_info.value) {
				ctx->hbm_mode = param_info.value;
				panel_hbm_set_cmdq(ctx, dsi, cb, handle, param_info.value);
				pr_debug("%s: set HBM to %d end\n", __func__, param_info.value);
				ret = 0;
			}
			else
				pr_info("%s: skip same HBM mode:%d\n", __func__, ctx->hbm_mode);
			break;
		default:
			pr_info("%s: skip unsupport feature %d to %d\n", __func__, param_info.param_idx, param_info.value);
			break;
	}

	return ret;
}

static struct mtk_panel_funcs ext_funcs = {
	.set_backlight_cmdq = csot_setbacklight_cmdq,
	.reset = panel_ext_reset,
	.ext_param_set = mtk_panel_ext_param_set,
	.get_lcm_version = panel_get_lcm_version,
//	.ata_check = panel_ata_check,
	.set_gesture_flag = panel_set_gesture_flag,
	.panel_feature_set = panel_feature_set,
};
#endif

static int csot_get_modes(struct drm_panel *panel,
						struct drm_connector *connector)
{
	struct drm_display_mode *mode;
	struct drm_display_mode *mode_1;
	struct drm_display_mode *mode_2;
	struct drm_display_mode *mode_3;

	mode = drm_mode_duplicate(connector->dev, &default_mode);
	printk("[%d  %s]disp: mode:\n",__LINE__, __FUNCTION__,mode);
	if (!mode) {
		dev_err(connector->dev->dev, "failed to add mode %ux%ux@%u\n",
			default_mode.hdisplay, default_mode.vdisplay,
			drm_mode_vrefresh(&default_mode));
		return -ENOMEM;
	}

	drm_mode_set_name(mode);
	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	drm_mode_probed_add(connector, mode);

#if 1
	mode_1 = drm_mode_duplicate(connector->dev, &performance_mode_30hz);
	printk("[%d  %s]disp mode:%d\n",__LINE__, __FUNCTION__,mode_1);
	if (!mode_1) {
		dev_err(connector->dev->dev, "failed to add mode %ux%ux@%u\n",
			performance_mode_30hz.hdisplay,
			performance_mode_30hz.vdisplay,
			drm_mode_vrefresh(&performance_mode_30hz));
		return -ENOMEM;
	}
	drm_mode_set_name(mode_1);
	mode_1->type = DRM_MODE_TYPE_DRIVER;
	drm_mode_probed_add(connector, mode_1);


	mode_2 = drm_mode_duplicate(connector->dev, &performance_mode_90hz);
	printk("[%d  %s]disp mode:%d\n",__LINE__, __FUNCTION__,mode_2);
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

	mode_3 = drm_mode_duplicate(connector->dev, &performance_mode_60hz);
	if (!mode_3) {
		dev_err(connector->dev->dev, "failed to add mode %ux%ux@%u\n",
			performance_mode_60hz.hdisplay,
			performance_mode_60hz.vdisplay,
			drm_mode_vrefresh(&performance_mode_60hz));
		return -ENOMEM;
	}

	drm_mode_set_name(mode_3);
	mode_3->type = DRM_MODE_TYPE_DRIVER;
	drm_mode_probed_add(connector, mode_3);
	connector->display_info.width_mm = 68;
	connector->display_info.height_mm = 150;
	printk("[%d  %s]end\n",__LINE__, __FUNCTION__);
#endif

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

	pr_info("%s+ disp:csot,icnl9922c,672,vdo,v0\n", __func__);

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

#ifdef PANEL_LDO_VTP_EN
	reg_vtp_1p8 = regulator_get(dev, "1p8");
	if (IS_ERR(reg_vtp_1p8)) {
		dev_info(dev, "%s[lh]: cannot get reg_vufs18 %ld\n",
			__func__, PTR_ERR(reg_vtp_1p8));
	}
	lcm_enable_reg_vtp_1p8(1);
#endif

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

	pr_info("[%d  %s]- csot,icnl9922c,672,vdo,120hz ret:%d  \n", __LINE__, __func__,ret);

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
		.compatible = "csot,icnl9922c,672,vdo,120hz",
	},
	{}
};

MODULE_DEVICE_TABLE(of, csot_of_match);

static struct mipi_dsi_driver csot_driver = {
	.probe = csot_probe,
	.remove = csot_remove,
	.driver = {
		.name = "csot_icnl9922c_vid_672_1080",
		.owner = THIS_MODULE,
		.of_match_table = csot_of_match,
	},
};

module_mipi_dsi_driver(csot_driver);

MODULE_AUTHOR("mediatek");
MODULE_DESCRIPTION("csot icnl9922c incell 120hz Panel Driver");
MODULE_LICENSE("GPL v2");

