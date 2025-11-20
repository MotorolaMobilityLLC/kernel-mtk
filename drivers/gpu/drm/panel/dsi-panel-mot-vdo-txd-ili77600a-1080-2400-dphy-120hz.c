/*
 * Copyright (C) 2025 Motorola Mobility LLC
 * All Rights Reserved.
 * Motorola Mobility Confidential Restricted.
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
#include "include/dsi-panel-mot-vdo-txd-ili77600a-1080-2400-dphy-120hz.h"
#endif

#define PANEL_LDO_VTP_EN

#ifdef PANEL_LDO_VTP_EN
#include <linux/gpio/consumer.h>
#include <linux/regulator/consumer.h>
#endif

/* option function to read data from some panel address */
/* #define PANEL_SUPPORT_READBACK */

#define BIAS_OCP2138
#ifdef BIAS_OCP2138
//extern int __attribute__ ((weak)) sm5109_BiasPower_disable(u32 pwrdown_delay);
//extern int __attribute__ ((weak)) sm5109_BiasPower_enable(u32 avdd, u32 avee,u32 pwrup_delay);
extern int __attribute__ ((weak)) ocp2138_BiasPower_disable(u32 pwrdown_delay);
extern int __attribute__ ((weak)) ocp2138_BiasPower_enable(u32 avdd, u32 avee,u32 pwrup_delay);
#endif

//txd EVT panel v0 support. to be disabled before PVT
//#define txd_PANEL_EVT_V0_SUPPORT		1

#define TXD_ILI_PANEL_VENDOR_ID    0x91070501
//#define txd_ILI_PANEL_V0_VENDOR_ID  	(txd_ILI_PANEL_VENDOR_ID | (0xF << 24))
#if 0
enum panel_version {
        PANEL_V1,  //DVT, PVT
        PANEL_V0,  //EVT
};
#endif
static int tp_gesture_flag = 0;

struct txd_ili77600a {
	struct device *dev;
	struct drm_panel panel;
	struct backlight_device *backlight;
	struct gpio_desc *reset_gpio;
	struct gpio_desc *bias_n_gpio;
	struct gpio_desc *bias_p_gpio;

	bool prepared;
	bool enabled;

	int error;
	//unsigned int hbm_mode;
	unsigned int cabc_mode;
	//enum panel_version version;
};

static struct mtk_panel_para_table panel_cabc_ui[] = {
	{4, {0xFF, 0x5A, 0xA5, 0x00}},
	{2, {0x55, 0x02}},
};

static struct mtk_panel_para_table panel_cabc_mv[] = {
	{4, {0xFF, 0x5A, 0xA5, 0x00}},
	{2, {0x55, 0x02}},
};

static struct mtk_panel_para_table panel_cabc_disable[] = {
	{4, {0xFF, 0x5A, 0xA5, 0x00}},
	{2, {0x55, 0x00}},
};

#if 0
static struct mtk_panel_para_table panel_hbm_on[] = {
	{4, {0xFF, 0x5A, 0xA5, 0x00}},
	{3, {0x51, 0x07, 0xAC}},
};

static struct mtk_panel_para_table panel_hbm_off[] = {
	{4, {0xFF, 0x5A, 0xA5, 0x00}},
	{3, {0x51, 0x06, 0x23}},
};
#endif

#define txd_ili77600a_dcs_write_seq(ctx, seq...)                                     \
	({                                                                     \
		const u8 d[] = {seq};                                          \
		BUILD_BUG_ON_MSG(ARRAY_SIZE(d) > 64,                           \
				 "DCS sequence too big for stack");            \
		txd_ili77600a_dcs_write(ctx, d, ARRAY_SIZE(d));                      \
	})

#define txd_ili77600a_dcs_write_seq_static(ctx, seq...)                              \
	({                                                                     \
		static const u8 d[] = {seq};                                   \
		txd_ili77600a_dcs_write(ctx, d, ARRAY_SIZE(d));                      \
	})

static inline struct txd_ili77600a *panel_to_txd_ili77600a(struct drm_panel *panel)
{
	return container_of(panel, struct txd_ili77600a, panel);
}

#ifdef PANEL_SUPPORT_READBACK
static int txd_ili77600a_dcs_read(struct txd_ili77600a *ctx, u8 cmd, void *data, size_t len)
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

static void txd_ili77600a_panel_get_data(struct txd_ili77600a *ctx)
{
	u8 buffer[3] = {0};
	static int ret;

	if (ret == 0) {
		ret = txd_ili77600a_dcs_read(ctx, 0x0A, buffer, 1);
		pr_info("disp: %s 0x%08x\n", __func__, buffer[0] | (buffer[1] << 8));
		dev_info(ctx->dev, "return %d data(0x%08x) to dsi engine\n",
			 ret, buffer[0] | (buffer[1] << 8));
	}
}
#endif

static void txd_ili77600a_dcs_write(struct txd_ili77600a *ctx, const void *data, size_t len)
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


static void txd_ili77600a_panel_init(struct txd_ili77600a *ctx)
{
	pr_info("disp: %s+\n", __func__);

	ctx->reset_gpio = devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio)) {
		dev_err(ctx->dev, "%s: cannot get reset_gpio %ld\n",
			__func__, PTR_ERR(ctx->reset_gpio));
		//return;
	}
	else {
		//based on another project like il99506
		gpiod_set_value(ctx->reset_gpio, 1);
		usleep_range(10 * 1000, 12 * 1000);
		gpiod_set_value(ctx->reset_gpio, 0);
		usleep_range(10 * 1000, 12 * 1000);
		gpiod_set_value(ctx->reset_gpio, 1);
		usleep_range(20000, 22000);
		devm_gpiod_put(ctx->dev, ctx->reset_gpio);
		pr_info("disp: %s reset_gpio\n", __func__);
	}

	pr_info("txd ili77600a init start!\n");
	txd_ili77600a_dcs_write_seq_static(ctx,0xFF, 0x5A,0xA5,0x06);
	txd_ili77600a_dcs_write_seq_static(ctx,0x3E, 0x62);
	txd_ili77600a_dcs_write_seq_static(ctx,0x79, 0x00);
	txd_ili77600a_dcs_write_seq_static(ctx,0xC6, 0x40);
	txd_ili77600a_dcs_write_seq_static(ctx,0xFF, 0x5A,0xA5,0x03);
	txd_ili77600a_dcs_write_seq_static(ctx,0x85, 0x30);
	txd_ili77600a_dcs_write_seq_static(ctx,0x88, 0xE6);
	txd_ili77600a_dcs_write_seq_static(ctx,0x89, 0xF0);
	txd_ili77600a_dcs_write_seq_static(ctx,0x8A, 0xF6);
	txd_ili77600a_dcs_write_seq_static(ctx,0x8B, 0xFF);
	txd_ili77600a_dcs_write_seq_static(ctx,0x87, 0x4D);
	txd_ili77600a_dcs_write_seq_static(ctx,0x8C, 0xD2);
	txd_ili77600a_dcs_write_seq_static(ctx,0x8D, 0xD6);
	txd_ili77600a_dcs_write_seq_static(ctx,0x8E, 0xDA);
	txd_ili77600a_dcs_write_seq_static(ctx,0x8F, 0xDE);
	txd_ili77600a_dcs_write_seq_static(ctx,0x90, 0xDF);
	txd_ili77600a_dcs_write_seq_static(ctx,0x91, 0xE6);
	txd_ili77600a_dcs_write_seq_static(ctx,0x92, 0xE9);
	txd_ili77600a_dcs_write_seq_static(ctx,0x93, 0xED);
	txd_ili77600a_dcs_write_seq_static(ctx,0x94, 0xF0);
	txd_ili77600a_dcs_write_seq_static(ctx,0x95, 0xFF);
	txd_ili77600a_dcs_write_seq_static(ctx,0x96, 0xB1);
	txd_ili77600a_dcs_write_seq_static(ctx,0x97, 0xB4);
	txd_ili77600a_dcs_write_seq_static(ctx,0x98, 0xB9);
	txd_ili77600a_dcs_write_seq_static(ctx,0x99, 0xBE);
	txd_ili77600a_dcs_write_seq_static(ctx,0x9A, 0xC9);
	txd_ili77600a_dcs_write_seq_static(ctx,0x9B, 0xCD);
	txd_ili77600a_dcs_write_seq_static(ctx,0x9C, 0xD3);
	txd_ili77600a_dcs_write_seq_static(ctx,0x9D, 0xE6);
	txd_ili77600a_dcs_write_seq_static(ctx,0x9E, 0xF2);
	txd_ili77600a_dcs_write_seq_static(ctx,0x9F, 0xF3);
	txd_ili77600a_dcs_write_seq_static(ctx,0xAF, 0x18);
	txd_ili77600a_dcs_write_seq_static(ctx,0xB7, 0x01);
	txd_ili77600a_dcs_write_seq_static(ctx,0xB8, 0x74);
	txd_ili77600a_dcs_write_seq_static(ctx,0xFF, 0x5A,0xA5,0x00);
	txd_ili77600a_dcs_write_seq_static(ctx,0x51, 0x07,0xFF);
	txd_ili77600a_dcs_write_seq_static(ctx,0x53, 0x2C);
	txd_ili77600a_dcs_write_seq_static(ctx,0x55, 0x02);
	txd_ili77600a_dcs_write_seq_static(ctx,0x35, 0x00);
	txd_ili77600a_dcs_write_seq_static(ctx,0x11, 0x00);
	msleep(120);
	txd_ili77600a_dcs_write_seq_static(ctx,0x29, 0x00);
	msleep(20);

	pr_info("disp:init code %s, data_rate=%d end!\n", __func__, DATA_RATE);
}

static int txd_ili77600a_disable(struct drm_panel *panel)
{
	struct txd_ili77600a *ctx = panel_to_txd_ili77600a(panel);
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

static int txd_ili77600a_unprepare(struct drm_panel *panel)
{
	struct txd_ili77600a *ctx = panel_to_txd_ili77600a(panel);

	if (!ctx->prepared) {
		pr_info("%s, already unprepared, return\n", __func__);
		return 0;
	}
	pr_info("%s\n", __func__);
	msleep(1);
	txd_ili77600a_dcs_write_seq_static(ctx, 0x28);
	msleep(20);
	txd_ili77600a_dcs_write_seq_static(ctx, 0x10);
	msleep(120);

	ctx->prepared = false;

	pr_info("%s:disp: tp_gesture_flag:%d\n",__func__, tp_gesture_flag);
	if(!tp_gesture_flag) {
#ifdef BIAS_OCP2138
		pr_info("%s: ocp2138_BiasPower_disable\n", __func__);
		ocp2138_BiasPower_disable(5);
#endif
	}

	ctx->error = 0;
	return 0;
}

static int txd_ili77600a_prepare(struct drm_panel *panel)
{
	struct txd_ili77600a *ctx = panel_to_txd_ili77600a(panel);
	int ret;

	pr_info("%s\n", __func__);
	if (ctx->prepared) {
		pr_info("%s, already prepared, return\n", __func__);
		return 0;
	}


#ifdef BIAS_OCP2138
	pr_info("%s: start ocp2138_BiasPower_enable\n", __func__);
	ocp2138_BiasPower_enable(15,15,5);
	msleep(1);
#endif

	txd_ili77600a_panel_init(ctx);
	//ctx->hbm_mode = 0;
	ctx->cabc_mode = 0;

	ret = ctx->error;
	if (ret < 0) {
		pr_info("disp: %s error ret=%d\n", __func__, ret);
		txd_ili77600a_unprepare(panel);
	}

	ctx->prepared = true;
/*#if defined(CONFIG_MTK_PANEL_EXT)
	mtk_panel_tch_rst(panel);
#endif

#ifdef PANEL_SUPPORT_READBACK
	txd_ili77600a_panel_get_data(ctx);
#endif*/
	pr_info("disp: %s-\n", __func__);
	return ret;
}

static int txd_ili77600a_enable(struct drm_panel *panel)
{
	struct txd_ili77600a *ctx = panel_to_txd_ili77600a(panel);

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
	.clock	= ((FRAME_WIDTH + MODE_120_HFP + HSA + HBP)*(FRAME_HEIGHT + MODE_120_VFP + VSA + VBP)*MODE_120_FPS)/1000,
	.hdisplay = FRAME_WIDTH,
	.hsync_start = FRAME_WIDTH + MODE_120_HFP,
	.hsync_end = FRAME_WIDTH + MODE_120_HFP + HSA,
	.htotal = FRAME_WIDTH + MODE_120_HFP + HSA + HBP,
	.vdisplay = FRAME_HEIGHT,
	.vsync_start = FRAME_HEIGHT + MODE_120_VFP,
	.vsync_end = FRAME_HEIGHT + MODE_120_VFP + VSA,
	.vtotal = FRAME_HEIGHT + MODE_120_VFP + VSA + VBP,
};

static const struct drm_display_mode performance_mode_30hz = {
	.clock	= ((FRAME_WIDTH + MODE_30_HFP + HSA + HBP)*(FRAME_HEIGHT + MODE_30_VFP + VSA + VBP)*MODE_30_FPS)/1000,
	.hdisplay = FRAME_WIDTH,
	.hsync_start = FRAME_WIDTH + MODE_30_HFP,
	.hsync_end = FRAME_WIDTH + MODE_30_HFP + HSA,
	.htotal = FRAME_WIDTH + MODE_30_HFP + HSA + HBP,
	.vdisplay = FRAME_HEIGHT,
	.vsync_start = FRAME_HEIGHT + MODE_30_VFP,
	.vsync_end = FRAME_HEIGHT + MODE_30_VFP + VSA,
	.vtotal = FRAME_HEIGHT + MODE_30_VFP + VSA + VBP,
};

static const struct drm_display_mode performance_mode_60hz = {
	.clock	= ((FRAME_WIDTH + MODE_60_HFP + HSA + HBP)*(FRAME_HEIGHT + MODE_60_VFP + VSA + VBP)*MODE_60_FPS)/1000,
	.hdisplay = FRAME_WIDTH,
	.hsync_start = FRAME_WIDTH + MODE_60_HFP,
	.hsync_end = FRAME_WIDTH + MODE_60_HFP + HSA,
	.htotal = FRAME_WIDTH + MODE_60_HFP + HSA + HBP,
	.vdisplay = FRAME_HEIGHT,
	.vsync_start = FRAME_HEIGHT + MODE_60_VFP,
	.vsync_end = FRAME_HEIGHT + MODE_60_VFP + VSA,
	.vtotal = FRAME_HEIGHT + MODE_60_VFP + VSA + VBP,
};

static const struct drm_display_mode performance_mode_90hz = {
	.clock	= ((FRAME_WIDTH + MODE_90_HFP + HSA + HBP)*(FRAME_HEIGHT + MODE_90_VFP + VSA + VBP)*MODE_90_FPS)/1000,
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
static struct mtk_panel_params ext_params_mode_30 = {
	//.change_fps_by_vfp_send_cmd = 0,
	//.vfp_low_power = 20,
	.data_rate = DATA_RATE,
	.cust_esd_check = 1,
	.esd_check_enable = 1,
	.lcm_esd_check_table[0] = {
		.cmd = 0x0a,
		.count = 1,
		.para_list[0] = 0x9c,
	},
	.lcm_cellid = {
		.panel_cellid_reg = 0x10,
		.panel_cellid_reg_seq = 1,
		.panel_cellid_len = 23,
		.panel_cellid_read_max = 1,
		.panel_cellid_esd_dis = 1,
		.page_table = {
			{
                        },
		},
		.page_post_table = {
			{0x39, 0x04, 0xFF, 0x5A, 0xA5, 0x00},
		},
	},
	.panel_ver = 1,
	//.panel_id = 0x01050791,
	.panel_name = "txd_il77600a_672",
	.panel_supplier = "txd",
	.lcm_index = 0,
	.hbm_type = HBM_MODE_RAMPING,
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
	.lfr_minimum_fps = MODE_30_FPS,

};

static struct mtk_panel_params ext_params_mode_60 = {
	//.change_fps_by_vfp_send_cmd = 0,
	//.vfp_low_power = 20,
	.data_rate = DATA_RATE,
	.cust_esd_check = 1,
	.esd_check_enable = 1,
	.lcm_esd_check_table[0] = {
		.cmd = 0x0a,
		.count = 1,
		.para_list[0] = 0x9c,
	},
	.lcm_cellid = {
		.panel_cellid_reg = 0x10,
		.panel_cellid_reg_seq = 1,
		.panel_cellid_len = 23,
		.panel_cellid_read_max = 1,
		.panel_cellid_esd_dis = 1,
		.page_table = {
			{0x39, 0x04, 0xFF, 0x5A, 0xA5, 0x0B},
		},
		.page_post_table = {
			{0x39, 0x04, 0xFF, 0x5A, 0xA5, 0x00},
		},
	},
	.panel_ver = 1,
	//.panel_id = 0x01050791,
	.panel_name = "txd_il77600a_672",
	.panel_supplier = "txd",
	.lcm_index = 0,
	.hbm_type = HBM_MODE_RAMPING,
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

};

static struct mtk_panel_params ext_params_mode_90 = {
//	.vfp_low_power = 7476,
	.data_rate = DATA_RATE,
	.cust_esd_check = 1,
	.esd_check_enable = 1,
	.lcm_esd_check_table[0] = {
		.cmd = 0x0a,
		.count = 1,
		.para_list[0] = 0x9c,
	},
	.lcm_cellid = {
		.panel_cellid_reg = 0x10,
		.panel_cellid_reg_seq = 1,
		.panel_cellid_len = 23,
		.panel_cellid_read_max = 1,
		.panel_cellid_esd_dis = 1,
		.page_table = {
			{0x39, 0x04, 0xFF, 0x5A, 0xA5, 0x0B},
		},
		.page_post_table = {
			{0x39, 0x04, 0xFF, 0x5A, 0xA5, 0x00},
		},
	},
	.panel_ver = 1,
	//.panel_id = 0x10050a91,
	.panel_name = "txd_il77600a_672",
	.panel_supplier = "txd",
	.lcm_index = 0,
	.hbm_type = HBM_MODE_RAMPING,
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
};

static struct mtk_panel_params ext_params_mode_120 = {
//	.vfp_low_power = 7476,
	.data_rate = DATA_RATE,
	.cust_esd_check = 1,
	.esd_check_enable = 1,
	.lcm_esd_check_table[0] = {
		.cmd = 0x0a,
		.count = 1,
		.para_list[0] = 0x9c,
	},
	.lcm_cellid = {
		.panel_cellid_reg = 0x10,
		.panel_cellid_reg_seq = 1,
		.panel_cellid_len = 23,
		.panel_cellid_read_max = 1,
		.panel_cellid_esd_dis = 1,
		.page_table = {
			{0x39, 0x04, 0xFF, 0x5A, 0xA5, 0x0B},
		},
		.page_post_table = {
			{0x39, 0x04, 0xFF, 0x5A, 0xA5, 0x00},
		},
	},
	.panel_ver = 1,
	//.panel_id = 0x10050a91,
	.panel_name = "txd_il77600a_672",
	.panel_supplier = "txd",
	.lcm_index = 0,
	.hbm_type = HBM_MODE_RAMPING,
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

};

static int txd_ili77600a_setbacklight_cmdq(void *dsi, dcs_write_gce cb,
	void *handle, unsigned int level)
{
	static char bl_tb0[] = { 0x51, 0x7f, 0xff };

	pr_info("%s backlight = %d\n", __func__, level);

	bl_tb0[1] = (level >> 8) & 0x7;
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

	if (!m)
		return ret;

	pr_info("%s:disp: mode fps=%d", __func__, drm_mode_vrefresh(m));
	if (drm_mode_vrefresh(m) == MODE_30_FPS)
		ext->params = &ext_params_mode_30;
	else if (drm_mode_vrefresh(m) == MODE_60_FPS)
		ext->params = &ext_params_mode_60;
	else if (drm_mode_vrefresh(m) == MODE_90_FPS)
		ext->params = &ext_params_mode_90;
	else if (drm_mode_vrefresh(m) == MODE_120_FPS)
		ext->params = &ext_params_mode_120;
	else
		ret = 1;
	return ret;
}

static int panel_ext_reset(struct drm_panel *panel, int on)
{
	struct txd_ili77600a *ctx = panel_to_txd_ili77600a(panel);

	ctx->reset_gpio =
		devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
		gpiod_set_value(ctx->reset_gpio, on);
		devm_gpiod_put(ctx->dev, ctx->reset_gpio);

	return 0;
}

static int panel_cabc_set_cmdq(struct txd_ili77600a *ctx, void *dsi, dcs_grp_write_gce cb, void *handle, uint32_t cabc_mode)
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

#if 0 // HBM RAMPING
static int panel_hbm_set_cmdq(struct txd_ili77600a *ctx, void *dsi, dcs_grp_write_gce cb, void *handle, uint32_t hbm_state)
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
#endif

static int panel_feature_set(struct drm_panel *panel, void *dsi,
			      dcs_grp_write_gce cb, void *handle, struct panel_param_info param_info)
{
	struct txd_ili77600a *ctx = panel_to_txd_ili77600a(panel);
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
				pr_info("%s: HBM ramping, skip HBM mode:%d\n", __func__, param_info.value);
			break;
		default:
			pr_info("%s: skip unsupport feature %d to %d\n", __func__, param_info.param_idx, param_info.value);
			break;
	}

	return ret;
}

static struct mtk_panel_funcs ext_funcs = {
	.set_backlight_cmdq = txd_ili77600a_setbacklight_cmdq,
	.reset = panel_ext_reset,
	.ext_param_set = mtk_panel_ext_param_set,
//	.get_lcm_version = panel_get_lcm_version,
//	.ata_check = panel_ata_check,
	.set_gesture_flag = panel_set_gesture_flag,
	.panel_feature_set = panel_feature_set,
};
#endif

static int txd_ili77600a_get_modes(struct drm_panel *panel,
						struct drm_connector *connector)
{
	struct drm_display_mode *mode;
	struct drm_display_mode *mode_1;
	struct drm_display_mode *mode_2;
	struct drm_display_mode *mode_3;

	mode = drm_mode_duplicate(connector->dev, &performance_mode_120hz);
	pr_info("disp: added mode with vrefresh %d\n", drm_mode_vrefresh(mode));
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

	mode_1 = drm_mode_duplicate(connector->dev, &performance_mode_30hz);
	pr_info("disp: added mode with vrefresh %d\n", drm_mode_vrefresh(mode_1));
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

	mode_2 = drm_mode_duplicate(connector->dev, &performance_mode_60hz);
	pr_info("disp: added mode with vrefresh %d\n", drm_mode_vrefresh(mode_2));
	if (!mode_2) {
		dev_err(connector->dev->dev, "failed to add mode %ux%ux@%u\n",
			performance_mode_60hz.hdisplay,
			performance_mode_60hz.vdisplay,
			drm_mode_vrefresh(&performance_mode_60hz));
		return -ENOMEM;
	}
	drm_mode_set_name(mode_2);
	mode_2->type = DRM_MODE_TYPE_DRIVER;
	drm_mode_probed_add(connector, mode_2);

	mode_3 = drm_mode_duplicate(connector->dev, &performance_mode_90hz);
	if (!mode_3) {
		dev_err(connector->dev->dev, "failed to add mode %ux%ux@%u\n",
			performance_mode_90hz.hdisplay,
			performance_mode_90hz.vdisplay,
			drm_mode_vrefresh(&performance_mode_90hz));
		return -ENOMEM;
	}
	drm_mode_set_name(mode_3);
	mode_3->type = DRM_MODE_TYPE_DRIVER;
	drm_mode_probed_add(connector, mode_3);
	connector->display_info.width_mm = 70;
	connector->display_info.height_mm = 156;
	pr_info("end\n");

	return 1;
}

static const struct drm_panel_funcs txd_ili77600a_drm_funcs = {
	.disable = txd_ili77600a_disable,
	.unprepare = txd_ili77600a_unprepare,
	.prepare = txd_ili77600a_prepare,
	.enable = txd_ili77600a_enable,
	.get_modes = txd_ili77600a_get_modes,
};

static int txd_ili77600a_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct device_node *dsi_node, *remote_node = NULL, *endpoint = NULL;
	struct txd_ili77600a *ctx;
	struct device_node *backlight;
	int ret;

	pr_info("%s+ disp:zkd txd_ili77600a_probe start!\n", __func__);

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

	ctx = devm_kzalloc(dev, sizeof(struct txd_ili77600a), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	mipi_dsi_set_drvdata(dsi, ctx);

	ctx->dev = dev;
	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST
			 | MIPI_DSI_MODE_LPM | MIPI_DSI_CLOCK_NON_CONTINUOUS;

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

	drm_panel_init(&ctx->panel, dev, &txd_ili77600a_drm_funcs, DRM_MODE_CONNECTOR_DSI);

	drm_panel_add(&ctx->panel);

	ret = mipi_dsi_attach(dsi);
	if (ret < 0)
		drm_panel_remove(&ctx->panel);

#if defined(CONFIG_MTK_PANEL_EXT)
	mtk_panel_tch_handle_reg(&ctx->panel);
	ret = mtk_panel_ext_create(dev, &ext_params_mode_120, &ext_funcs, &ctx->panel);
	if (ret < 0)
		return ret;
#endif

	pr_info("[%d  %s]- txd,ili77600a,vdo,120hz ret:%d\n", __LINE__, __func__,ret);

	return ret;
}

static int txd_ili77600a_remove(struct mipi_dsi_device *dsi)
{
	struct txd_ili77600a *ctx = mipi_dsi_get_drvdata(dsi);
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

static const struct of_device_id txd_ili77600a_of_match[] = {
	{
#if defined(CONFIG_DRM_PANEL_NUM_NO_LIMIT)
		.compatible = "txd_il77600a_672",
#else
		.compatible = "txd,il77600a,672",
#endif
	},
	{}
};

MODULE_DEVICE_TABLE(of, txd_ili77600a_of_match);

static struct mipi_dsi_driver txd_ili77600a_driver = {
	.probe = txd_ili77600a_probe,
	.remove = txd_ili77600a_remove,
	.shutdown = lcm_shutdown,
	.driver = {
		.name = "txd_il77600a_672",
		.owner = THIS_MODULE,
		.of_match_table = txd_ili77600a_of_match,
	},
};

module_mipi_dsi_driver(txd_ili77600a_driver);

MODULE_AUTHOR("Motorola Mobility");
MODULE_DESCRIPTION("txd ili77600a incell 120hz Panel Driver");
MODULE_LICENSE("GPL v2");

