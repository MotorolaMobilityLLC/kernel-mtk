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
#include "include/dsi-panel-mot-tm-icnl9911c-652-hdp-dphy-video-90hz.h"
#endif

/* option function to read data from some panel address */
/* #define PANEL_SUPPORT_READBACK */

extern int __attribute__ ((weak)) sm5109_bias_power_disable(u32 pwrdown_delay);
extern int __attribute__ ((weak)) sm5109_bias_power_enable(u32 avdd, u32 avee,u32 pwrup_delay);
static int tp_gesture_flag=0;

struct tianma {
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
	unsigned char vref_reg_buf;
};

static struct mtk_panel_para_table panel_cabc_ui[] = {
	{2,  {0x55, 0x01}},
	{3,  {0xF0, 0x5A, 0x59}},
	{3,  {0xF1, 0xA5, 0xA6}},
	{16, {0xE0, 0x30, 0x00, 0x80, 0x88, 0x11, 0x3F, 0x22, 0x62, 0xDF, 0xA0, 0x04, 0xCC, 0x01, 0xFF, 0xF6}},
	{2,  {0xF2, 0x10}},
	{2,  {0xE0, 0xFF}},
	{2,  {0xF2, 0x11}},
	{2,  {0xE0, 0xF0}},
	{2,  {0xF2, 0x12}},
	{2,  {0xE0, 0xFD}},
	{2,  {0xF2, 0x13}},
	{2,  {0xE0, 0xFF}},
	{2,  {0xF2, 0x14}},
	{2,  {0xE0, 0xFD}},
	{2,  {0xF2, 0x15}},
	{2,  {0xE0, 0xF8}},
	{2,  {0xF2, 0x16}},
	{2,  {0xE0, 0xF5}},
	{2,  {0xF2, 0x17}},
	{2,  {0xE0, 0xFC}},
	{2,  {0xF2, 0x18}},
	{2,  {0xE0, 0xFC}},
	{2,  {0xF2, 0x19}},
	{2,  {0xE0, 0xFD}},
	{2,  {0xF2, 0x1A}},
	{2,  {0xE0, 0xFF}},
	{16, {0xE1, 0xEF, 0xFE, 0xFE, 0xFE, 0xFE, 0xEE, 0xF0, 0x20, 0x33, 0xFF, 0x00, 0x00, 0x6A, 0x90, 0xC0}},
	{2,  {0xF2, 0x10}},
	{2,  {0xE1, 0x0D}},
	{2,  {0xF2, 0x11}},
	{2,  {0xE1, 0x6A}},
	{2,  {0xF2, 0x12}},
	{2,  {0xE1, 0xF0}},
	{2,  {0xF2, 0x13}},
	{2,  {0xE1, 0x3E}},
	{2,  {0xF2, 0x14}},
	{2,  {0xE1, 0xFF}},
	{2,  {0xF2, 0x15}},
	{2,  {0xE1, 0x00}},
	{2,  {0xF2, 0x16}},
	{2,  {0xE1, 0x07}},
	{2,  {0xF2, 0x17}},
	{2,  {0xE1, 0xD0}},
	{3,  {0xF1, 0x5A, 0x59}},
	{3,  {0xF0, 0xA5, 0xA6}},
};

static struct mtk_panel_para_table panel_cabc_mv[] = {
	{2,  {0x55, 0x03}},
	{3,  {0xF0, 0x5A, 0x59}},
	{3,  {0xF1, 0xA5, 0xA6}},
	{16, {0xE0, 0x30, 0x00, 0x80, 0x88, 0x11, 0x3F, 0x22, 0x62, 0xDF, 0xA0, 0x04, 0xCC, 0x01, 0xFF, 0xFA}},
	{2,  {0xF2, 0x10}},
	{2,  {0xE0, 0xFF}},
	{2,  {0xF2, 0x11}},
	{2,  {0xE0, 0xF0}},
	{2,  {0xF2, 0x12}},
	{2,  {0xE0, 0xFD}},
	{2,  {0xF2, 0x13}},
	{2,  {0xE0, 0xFF}},
	{2,  {0xF2, 0x14}},
	{2,  {0xE0, 0xFB}},
	{2,  {0xF2, 0x15}},
	{2,  {0xE0, 0xF8}},
	{2,  {0xF2, 0x16}},
	{2,  {0xE0, 0xF5}},
	{2,  {0xF2, 0x17}},
	{2,  {0xE0, 0xFC}},
	{2,  {0xF2, 0x18}},
	{2,  {0xE0, 0xFC}},
	{2,  {0xF2, 0x19}},
	{2,  {0xE0, 0xFB}},
	{2,  {0xF2, 0x1A}},
	{2,  {0xE0, 0xFF}},
	{16, {0xE1, 0xBC, 0xF8, 0xCC, 0xFA, 0xDB, 0x9B, 0xF0, 0xE7, 0xF0, 0x85, 0xF0, 0x70, 0x00, 0x50, 0x00}},
	{2,  {0xF2, 0x10}},
	{2,  {0xE1, 0x9A}},
	{2,  {0xF2, 0x11}},
	{2,  {0xE1, 0xFD}},
	{2,  {0xF2, 0x12}},
	{2,  {0xE1, 0xF0}},
	{2,  {0xF2, 0x13}},
	{2,  {0xE1, 0xE0}},
	{2,  {0xF2, 0x14}},
	{2,  {0xE1, 0xFF}},
	{2,  {0xF2, 0x15}},
	{2,  {0xE1, 0x00}},
	{2,  {0xF2, 0x16}},
	{2,  {0xE1, 0x07}},
	{2,  {0xF2, 0x17}},
	{2,  {0xE1, 0xD0}},
	{3,  {0xF1, 0x5A, 0x59}},
	{3,  {0xF0, 0xA5, 0xA6}},
};

static struct mtk_panel_para_table panel_cabc_disable[] = {
       {2, {0x55, 0x00}},
};

static struct mtk_panel_para_table panel_hbm_on[] = {
       {3, {0x51, 0x0F, 0xFF}}, //100% PWM
};

static struct mtk_panel_para_table panel_hbm_off[] = {
       {3, {0x51, 0x06, 0x66}},
};

#define tianma_dcs_write_seq(ctx, seq...)                                     \
	({                                                                     \
		const u8 d[] = {seq};                                          \
		BUILD_BUG_ON_MSG(ARRAY_SIZE(d) > 64,                           \
				 "DCS sequence too big for stack");            \
		tianma_dcs_write(ctx, d, ARRAY_SIZE(d));                      \
	})

#define tianma_dcs_write_seq_static(ctx, seq...)                              \
	({                                                                     \
		static const u8 d[] = {seq};                                   \
		tianma_dcs_write(ctx, d, ARRAY_SIZE(d));                      \
	})

static inline struct tianma *panel_to_tianma(struct drm_panel *panel)
{
	return container_of(panel, struct tianma, panel);
}

#ifdef PANEL_SUPPORT_READBACK
static int tianma_dcs_read(struct tianma *ctx, u8 cmd, void *data, size_t len)
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

static void tianma_panel_get_data(struct tianma *ctx)
{
	u8 buffer[3] = {0};
	static int ret;

	if (ret == 0) {
		ret = tianma_dcs_read(ctx, 0x0A, buffer, 1);
		dev_info(ctx->dev, "return %d data(0x%08x) to dsi engine\n",
			 ret, buffer[0] | (buffer[1] << 8));
	}
}
#endif

static void tianma_dcs_write(struct tianma *ctx, const void *data, size_t len)
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

static void kernel_vref_reg_update(struct tianma *ctx)
{
	int ret = 0;
	char *vref_f6_reg_start=NULL;
	char *temp;
	char vref_f6_reg[8]={'\0'};

	if(!ctx->vref_reg_buf) {
		vref_f6_reg_start = strstr(saved_command_line, "vref_reg=");
		if(vref_f6_reg_start == NULL){
			pr_err("[LCM]command_line have no vref_reg info.\n");
			return;
		}
		temp = vref_f6_reg_start + strlen("vref_reg=");
		memcpy(vref_f6_reg, temp, 4);

		ret =  kstrtou8(vref_f6_reg, 0, &ctx->vref_reg_buf);
		if (ret != 0){
			pr_err("[LCM]Convert vref_f6_reg string to unsigned int error.\n");
		}
	}

	tianma_dcs_write_seq(ctx, 0xF6, (const u8)ctx->vref_reg_buf);
	pr_info("[LCM kernel] write vref_reg_buf_F6 = 0x%02x.\n", ctx->vref_reg_buf);
}

static void tianma_panel_init(struct tianma *ctx)
{
	ctx->reset_gpio = devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio)) {
		dev_err(ctx->dev, "%s: cannot get reset_gpio %ld\n",
			__func__, PTR_ERR(ctx->reset_gpio));
		return;
	}
	gpiod_set_value(ctx->reset_gpio, 1);
	usleep_range(10000,10001);
	gpiod_set_value(ctx->reset_gpio, 0);
	usleep_range(5000,5001);
	gpiod_set_value(ctx->reset_gpio, 1);
	usleep_range(10000,10001);

	devm_gpiod_put(ctx->dev, ctx->reset_gpio);

	// password_open_and_offset_code
	tianma_dcs_write_seq_static(ctx, 0xF0, 0x5A, 0x59); //password open
	tianma_dcs_write_seq_static(ctx, 0xF1, 0xA5, 0xA6); //password open
	tianma_dcs_write_seq_static(ctx, 0xBD, 0xE9, 0x02, 0x4E);
	tianma_dcs_write_seq_static(ctx, 0xBE, 0x63, 0x6D);
	tianma_dcs_write_seq_static(ctx, 0xC1, 0xC0, 0x20, 0x20, 0x96, 0x04, 0x30, 0x30, 0x04, 0x2A, 0x40, 0x36, 0x00, 0x07, 0xCF, 0xFF, 0xFF, 0x7C, 0x01, 0xC0);
	tianma_dcs_write_seq_static(ctx, 0xCB, 0x05, 0x40, 0x55, 0x40, 0x04, 0x40, 0x35, 0x43, 0x43, 0x50, 0x1E, 0x40, 0x40, 0x43, 0x43, 0x64, 0x23, 0x40, 0x40, 0x22, 0x04);
	tianma_dcs_write_seq_static(ctx, 0xFA, 0x45, 0x93, 0x01);

	// kernel_vref_reg_update
	kernel_vref_reg_update(ctx);

	// password_close_code
	tianma_dcs_write_seq_static(ctx, 0xF1, 0x5A, 0x59); //password close
	tianma_dcs_write_seq_static(ctx, 0xF0, 0xA5, 0xA6); //password close

	// init_setting
	tianma_dcs_write_seq_static(ctx, 0x35, 0x00);           // MIPI_DCS_SET_TEAR_ON
	tianma_dcs_write_seq_static(ctx, 0x55, 0x01);           // MIPI_DCS_WRITE_POWER_SAVE
	tianma_dcs_write_seq_static(ctx, 0x53, 0x2C);           // MIPI_DCS_WRITE_CONTROL_DISPLAY

	tianma_dcs_write_seq_static(ctx, 0xF0, 0x5A, 0x59);
	tianma_dcs_write_seq_static(ctx, 0xF1, 0xA5, 0xA6);
	tianma_dcs_write_seq_static(ctx, 0xE0, 0x30, 0x00, 0x80, 0x88, 0x11, 0x3F, 0x22, 0x62, 0xDF, 0xA0, 0x04, 0xCC, 0x01, 0xFF, 0xF6, 0xFF, 0xF0, 0xFD, 0xFF, 0xFD, 0xF8, 0xF5, 0xFC, 0xFC, 0xFD, 0xFF);
	tianma_dcs_write_seq_static(ctx, 0xE1, 0xEF, 0xFE, 0xFE, 0xFE, 0xFE, 0xEE, 0xF0, 0x20, 0x33, 0xFF, 0x00, 0x00, 0x6A, 0x90, 0xC0, 0x0D, 0x6A, 0xF0, 0x3E, 0xFF, 0x00, 0x07, 0xD0);
	tianma_dcs_write_seq_static(ctx, 0xD0, 0x80, 0x0D, 0xFF, 0x0F, 0x61, 0x0B, 0x08, 0x04);

	tianma_dcs_write_seq_static(ctx, 0x11, 0x00);           // MIPI_DCS_EXIT_SLEEP_MODE
	usleep_range(20000,20001);
	tianma_dcs_write_seq_static(ctx, 0x29, 0x00);           // MIPI_DCS_SET_DISPLAY_ON
	usleep_range(80000,80001);
	tianma_dcs_write_seq_static(ctx, 0x26, 0x02);           // MIPI_DCS_SET_GAMMA_CURVE
}

static int tianma_disable(struct drm_panel *panel)
{
	struct tianma *ctx = panel_to_tianma(panel);
	

	if (!ctx->enabled)
		return 0;

	if (ctx->backlight) {
		ctx->backlight->props.power = FB_BLANK_POWERDOWN;
		backlight_update_status(ctx->backlight);
	}

	ctx->enabled = false;

	return 0;
}

static int tianma_set_gesture_flag(int state)
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

static int tianma_unprepare(struct drm_panel *panel)
{
	struct tianma *ctx = panel_to_tianma(panel);
	int ret=0;

	if (!ctx->prepared)
		return 0;
	pr_info("%s\n", __func__);

	tianma_dcs_write_seq_static(ctx, 0x26, 0x08);     // MIPI_DCS_SET_GAMMA_CURVE
	tianma_dcs_write_seq_static(ctx, 0x28);           // MIPI_DCS_SET_DISPLAY_OFF
	msleep(20);
	tianma_dcs_write_seq_static(ctx, 0x10);           // MIPI_DCS_ENTER_SLEEP_MODE
	msleep(100);

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

	ret = sm5109_bias_power_disable(5);
	}


	ctx->error = 0;
	ctx->prepared = false;
	return 0;
}

static int tianma_prepare(struct drm_panel *panel)
{
	struct tianma *ctx = panel_to_tianma(panel);
	int ret;

	pr_info("%s\n", __func__);
	if (ctx->prepared)
		return 0;

	gpiod_set_value(ctx->reset_gpio, 0);
	msleep(3);
	ret = sm5109_bias_power_enable(14,14,3);

	//lcm_power_enable();
	tianma_panel_init(ctx);

	ret = ctx->error;
	if (ret < 0)
		tianma_unprepare(panel);

	ctx->prepared = true;

	ctx->hbm_mode = 0;
	ctx->cabc_mode = 0;

/*#if defined(CONFIG_MTK_PANEL_EXT)
	mtk_panel_tch_rst(panel);
#endif

#ifdef PANEL_SUPPORT_READBACK
	tianma_panel_get_data(ctx);
#endif*/

	return ret;
}

static int tianma_enable(struct drm_panel *panel)
{
	struct tianma *ctx = panel_to_tianma(panel);
	

	if (ctx->enabled)
		return 0;

	if (ctx->backlight) {
		ctx->backlight->props.power = FB_BLANK_UNBLANK;
		backlight_update_status(ctx->backlight);
	}

	ctx->enabled = true;

	return 0;
}

static const struct drm_display_mode display_mode_60hz = {
	.clock = 123631,
	.hdisplay = FRAME_WIDTH,
	.hsync_start = FRAME_WIDTH + MODE_60_HFP,
	.hsync_end = FRAME_WIDTH + MODE_60_HFP + HSA,
	.htotal = FRAME_WIDTH + MODE_60_HFP + HSA + HBP,
	.vdisplay = FRAME_HEIGHT,
	.vsync_start = FRAME_HEIGHT + MODE_60_VFP,
	.vsync_end = FRAME_HEIGHT + MODE_60_VFP + VSA,
	.vtotal = FRAME_HEIGHT + MODE_60_VFP + VSA + VBP,
};

static const struct drm_display_mode display_mode_90hz = {
	.clock = 123631,
	.hdisplay = FRAME_WIDTH,
	.hsync_start = FRAME_WIDTH + MODE_90_HFP,
	.hsync_end = FRAME_WIDTH + MODE_90_HFP + HSA,
	.htotal = FRAME_WIDTH + MODE_90_HFP + HSA + HBP,
	.vdisplay = FRAME_HEIGHT,
	.vsync_start = FRAME_HEIGHT + MODE_90_VFP,
	.vsync_end = FRAME_HEIGHT + MODE_90_VFP + VSA,
	.vtotal = FRAME_HEIGHT + MODE_90_VFP + VSA + VBP,
};

#if defined(CONFIG_MTK_PANEL_EXT)
static struct mtk_panel_params ext_params_60 = {
	//.pll_clk = 300,
	.data_rate = MODE_60_DATA_RATE,
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
	.lcm_index = 1,

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
	.lfr_minimum_fps = MODE_60_FPS,
	.max_bl_level = 2047,
	.hbm_type = HBM_MODE_DCS_I2C,
};

static struct mtk_panel_params ext_params_90 = {
	//.pll_clk = 300,
	.data_rate = MODE_90_DATA_RATE,
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
	.lcm_index = 1,

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
	.lfr_minimum_fps = MODE_90_FPS,
	.max_bl_level = 2047,
	.hbm_type = HBM_MODE_DCS_I2C,
};

static int tianma_setbacklight_cmdq(void *dsi, dcs_write_gce cb,
	void *handle, unsigned int level)
{
	char bl_tb0[] = {0x51, 0x0f, 0xff};

	//for 11bit
	bl_tb0[1] = (u8)((level & 0x07F8) >> 3);
	bl_tb0[2] = (u8)((level & 0x07) << 1);

	if (!cb)
		return -1;

	cb(dsi, handle, bl_tb0, ARRAY_SIZE(bl_tb0));
	return 0;
}

static struct drm_display_mode *get_mode_by_id(struct drm_connector *connector,
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

	if (drm_mode_vrefresh(m) == MODE_60_FPS)
		ext->params = &ext_params_60;
	else if (drm_mode_vrefresh(m) == MODE_90_FPS)
		ext->params = &ext_params_90;
	else
		ret = 1;
	return ret;
}

static int panel_ext_reset(struct drm_panel *panel, int on)
{
	struct tianma *ctx = panel_to_tianma(panel);

	ctx->reset_gpio =
		devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
	gpiod_set_value(ctx->reset_gpio, on);
	devm_gpiod_put(ctx->dev, ctx->reset_gpio);

	return 0;
}

static int panel_cabc_set_cmdq(struct tianma *ctx, void *dsi, dcs_grp_write_gce cb, void *handle, uint32_t cabc_mode)
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

static int panel_hbm_set_cmdq(struct tianma *ctx, void *dsi, dcs_grp_write_gce cb, void *handle, uint32_t hbm_state)
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
	struct tianma *ctx = panel_to_tianma(panel);
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
	.reset = panel_ext_reset,
	.ext_param_set = mtk_panel_ext_param_set,
	.set_gesture_flag = tianma_set_gesture_flag,
	.set_backlight_cmdq = tianma_setbacklight_cmdq,
	.panel_feature_set = panel_feature_set,
};
#endif

static int tianma_get_modes(struct drm_panel *panel,
						struct drm_connector *connector)
{
	struct drm_display_mode *mode_60hz;
	struct drm_display_mode *mode_90hz;

	mode_60hz = drm_mode_duplicate(connector->dev, &display_mode_60hz);
	if (!mode_60hz) {
		dev_err(connector->dev->dev, "failed to add mode %ux%ux@%u\n",
			display_mode_60hz.hdisplay, display_mode_60hz.vdisplay,
			drm_mode_vrefresh(&display_mode_60hz));
		return -ENOMEM;
	}
	drm_mode_set_name(mode_60hz);
	mode_60hz->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	drm_mode_probed_add(connector, mode_60hz);


	mode_90hz = drm_mode_duplicate(connector->dev, &display_mode_90hz);
	printk("[%d  %s]disp mode:%d\n",__LINE__, __FUNCTION__,mode_90hz);
	if (!mode_90hz) {
		dev_err(connector->dev->dev, "failed to add mode %ux%ux@%u\n",
			display_mode_90hz.hdisplay, display_mode_90hz.vdisplay,
			drm_mode_vrefresh(&display_mode_90hz));
		return -ENOMEM;
	}
	drm_mode_set_name(mode_90hz);
	mode_90hz->type = DRM_MODE_TYPE_DRIVER;
	drm_mode_probed_add(connector, mode_90hz);

	connector->display_info.width_mm = 68;
	connector->display_info.height_mm = 150;

	return 1;
}

static const struct drm_panel_funcs tianma_drm_funcs = {
	.disable = tianma_disable,
	.unprepare = tianma_unprepare,
	.prepare = tianma_prepare,
	.enable = tianma_enable,
	.get_modes = tianma_get_modes,
};

static int tianma_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct device_node *dsi_node, *remote_node = NULL, *endpoint = NULL;
	struct tianma *ctx;
	struct device_node *backlight;
	int ret;

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
		pr_info("%s+ skip probe due to not current lcm\n", __func__);
		return -ENODEV;
	}

	ctx = devm_kzalloc(dev, sizeof(struct tianma), GFP_KERNEL);
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

	ctx->vref_reg_buf = 0x00;
	ctx->prepared = true;
	ctx->enabled = true;
	drm_panel_init(&ctx->panel, dev, &tianma_drm_funcs, DRM_MODE_CONNECTOR_DSI);

	drm_panel_add(&ctx->panel);

	ret = mipi_dsi_attach(dsi);
	if (ret < 0)
		drm_panel_remove(&ctx->panel);

#if defined(CONFIG_MTK_PANEL_EXT)
	mtk_panel_tch_handle_reg(&ctx->panel);
	ret = mtk_panel_ext_create(dev, &ext_params_60, &ext_funcs, &ctx->panel);
	if (ret < 0)
		return ret;

#endif

	pr_info("%s- moto,dummy,vdo,60hz\n", __func__);

	return ret;
}

static int tianma_remove(struct mipi_dsi_device *dsi)
{
	struct tianma *ctx = mipi_dsi_get_drvdata(dsi);
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

static const struct of_device_id tianma_panel_of_match[] = {
	{
	    .compatible = "tm,icnl9911c,vdo,90hz",
	},
	{}
};

MODULE_DEVICE_TABLE(of, tianma_panel_of_match);

static struct mipi_dsi_driver tianma_driver = {
	.probe = tianma_probe,
	.remove = tianma_remove,
	.driver = {
		.name = "mipi_mot_vid_tm_icnl9911c_652_hdp",
		.owner = THIS_MODULE,
		.of_match_table = tianma_panel_of_match,
	},
};


static int __init __tianma_panel_driver_init(void)
{
	int ret = 0;

	pr_notice("%s+\n", __func__);
	mtk_panel_lock();
	ret = mipi_dsi_driver_register(&tianma_driver);
	if (ret < 0)
		pr_notice("%s, Failed to register jdi driver: %d\n",
			__func__, ret);

	mtk_panel_unlock();
	pr_notice("%s- ret:%d\n", __func__, ret);
	return 0;
}

static void __exit __tianma_panel_driver_exit(void)
{
	pr_notice("%s+\n", __func__);
	mtk_panel_lock();
	mipi_dsi_driver_unregister(&tianma_driver);
	mtk_panel_unlock();
	pr_notice("%s-\n", __func__);
}
module_init(__tianma_panel_driver_init);
module_exit(__tianma_panel_driver_exit);

MODULE_AUTHOR("MEDIATEK");
MODULE_DESCRIPTION("tianma nt37701 AMOLED CMD LCD Panel Driver");
MODULE_LICENSE("GPL v2");
