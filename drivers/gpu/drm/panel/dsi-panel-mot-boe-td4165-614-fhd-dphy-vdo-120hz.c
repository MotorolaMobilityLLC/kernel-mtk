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
#include "include/dsi-panel-mot-boe-td4165-614-fhd-dphy-vdo-120hz.h"
#endif

/* option function to read data from some panel address */
/* #define PANEL_SUPPORT_READBACK */

unsigned int boe_td4165_rc_buf_thresh[14] = {896, 1792, 2688, 3584, 4480, 5376,
		6272, 6720, 7168, 7616, 7744, 7872, 8000, 8064};
unsigned int boe_td4165_range_min_qp[15] = {0, 0, 1, 1, 3, 3, 3, 3, 3, 3, 5, 5, 5, 7, 13};
unsigned int boe_td4165_range_max_qp[15] = {4, 4, 5, 6, 7, 7, 7, 8, 9, 10, 11, 12, 13, 13, 15};
int boe_td4165_range_bpg_ofs[15] = {2, 0, 0, -2, -4, -6, -8, -8, -8, -10, -10, -12, -12, -12, -12};

extern int __attribute__ ((weak)) ocp2138_BiasPower_disable(u32 pwrdown_delay);
extern int __attribute__ ((weak)) ocp2138_BiasPower_enable(u32 avdd, u32 avee,u32 pwrup_delay);
extern int mtkfb_esd_get_recovery_flag(void);
static BLOCKING_NOTIFIER_HEAD(panel_gesture_notifier_list);

static int tp_gesture_flag = 0;

struct boe_td4165 {
	struct device *dev;
	struct drm_panel panel;
	struct backlight_device *backlight;
	struct gpio_desc *reset_gpio;
	struct gpio_desc *tp_reset_gpio;
	struct gpio_desc *bias_n_gpio;
	struct gpio_desc *bias_p_gpio;

	bool prepared;
	bool enabled;

	int error;
//	unsigned int hbm_mode;
	unsigned int cabc_mode;
	s64 screen_on_timestamp;
};

static struct mtk_panel_para_table panel_cabc_ui[] = {
	{2, {0x55, 0x01}},
};

static struct mtk_panel_para_table panel_cabc_mv[] = {
	{2, {0x55, 0x03}},
};

static struct mtk_panel_para_table panel_cabc_disable[] = {
	{2, {0x55, 0x00}},
};

#if 0
static struct mtk_panel_para_table panel_hbm_on[] = {
	{2, {0xFF, 0x10}},
	{2, {0xFB, 0x01}},
	{3, {0x51, 0x07, 0xFF}},
};

static struct mtk_panel_para_table panel_hbm_off[] = {
	{2, {0xFF, 0x10}},
	{2, {0xFB, 0x01}},
	{3, {0x51, 0x06, 0x66}},
};
#endif

//static char bl_tb0[] = { 0x51, 0xff };

//struct boe_td4165 *g_ctx = NULL;

#define boe_td4165_dcs_write_seq(ctx, seq...)                                         \
	({                                                                     \
		const u8 d[] = { seq };                                        \
		BUILD_BUG_ON_MSG(ARRAY_SIZE(d) > 64,                           \
				 "DCS sequence too big for stack");            \
		boe_td4165_dcs_write(ctx, d, ARRAY_SIZE(d));                          \
	})

#define boe_td4165_dcs_write_seq_static(ctx, seq...)                                  \
	({                                                                     \
		static const u8 d[] = { seq };                                 \
		boe_td4165_dcs_write(ctx, d, ARRAY_SIZE(d));                          \
	})

static inline struct boe_td4165 *panel_to_boe_td4165(struct drm_panel *panel)
{
	return container_of(panel, struct boe_td4165, panel);
}

#ifdef PANEL_SUPPORT_READBACK
static int boe_td4165_dcs_read(struct boe_td4165 *ctx, u8 cmd, void *data, size_t len)
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

static void boe_td4165_panel_get_data(struct boe_td4165 *ctx)
{
	u8 buffer[3] = {0};
	static int ret;

	if (ret == 0) {
		ret = boe_td4165_dcs_read(ctx, 0x0A, buffer, 1);
		pr_info("disp: %s 0x%08x\n", __func__, buffer[0] | (buffer[1] << 8));
		dev_info(ctx->dev, "return %d data(0x%08x) to dsi engine\n",
			 ret, buffer[0] | (buffer[1] << 8));
	}
}
#endif

static void boe_td4165_dcs_write(struct boe_td4165 *ctx, const void *data, size_t len)
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

static void boe_panel_tp_reset(struct boe_td4165 *ctx)
{
	pr_info("%s:boe_td4165: +\n", __func__);

	ctx->tp_reset_gpio = devm_gpiod_get(ctx->dev, "tp_reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->tp_reset_gpio)) {
		dev_err(ctx->dev, "%s:boe_td4165: cannot get tp_reset_gpio %ld\n",
			__func__, PTR_ERR(ctx->tp_reset_gpio));
		//return;
	}
	else {
		gpiod_set_value(ctx->tp_reset_gpio, 1);
		udelay(3 * 1000);
		devm_gpiod_put(ctx->dev, ctx->tp_reset_gpio);
		pr_info("%s:boe_td4165: tp_reset_gpio 1\n", __func__);
	}
}

int panel_gesture_register_client(const char *source, struct notifier_block *nb)
{
	if (!source)
		return -EINVAL;

	return blocking_notifier_chain_register(&panel_gesture_notifier_list, nb);
}
EXPORT_SYMBOL(panel_gesture_register_client);

int panel_gesture_unregister_client(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&panel_gesture_notifier_list, nb);
}
EXPORT_SYMBOL(panel_gesture_unregister_client);

int panel_gesture_notifier_call_chain(unsigned long val, void *v)
{
	return blocking_notifier_call_chain(&panel_gesture_notifier_list, val, v);
}
EXPORT_SYMBOL(panel_gesture_notifier_call_chain);

static void boe_td4165_panel_init(struct boe_td4165 *ctx)
{
	ktime_t now;
	pr_info("disp: %s+\n", __func__);

	ocp2138_BiasPower_enable(20,20,5);

	boe_panel_tp_reset(ctx);

	ctx->reset_gpio = devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio)) {
		dev_err(ctx->dev, "%s: cannot get reset_gpio %ld\n",
			__func__, PTR_ERR(ctx->reset_gpio));
		//return;
	}
	else {
		gpiod_set_value(ctx->reset_gpio, 1);
		udelay(10 * 1000);
		gpiod_set_value(ctx->reset_gpio, 0);
		udelay(10 * 1000);
		gpiod_set_value(ctx->reset_gpio, 1);
		//The time between the release of LCD reset and the transmission of MIPI CMD shall be more than 20 ms.
		msleep(23);
		devm_gpiod_put(ctx->dev, ctx->reset_gpio);
		pr_info("disp: %s reset_gpio\n", __func__);
	}

	boe_td4165_dcs_write_seq_static(ctx, 0xB0,0x80);
	boe_td4165_dcs_write_seq_static(ctx, 0xD6,0x00);
	//report rate
	boe_td4165_dcs_write_seq_static(ctx, 0xC1,0x30,0x11,0x50,0xfa,0x00,0x00,0x00,0x22,0x00,0x00,0x00,0x00,0x40,0x0f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x43,0xFF,0xF5,0x84,0x7A,0x0A,0x00,0x20,0x00,0x00,0x00,0xbc,0x0a,0x4c,0x0e,0xb5,0x12,0xc0,0x15,0xff,0x01,0x00,0x01,0x10,0x00,0x00);
	//TSVD time
	boe_td4165_dcs_write_seq_static(ctx, 0xEB,0x0F,0x50,0xF5,0x00,0x01,0x00,0x01,0x01,0x00,0xCC,0xCC,0x0C,0x08,0x06,0xD8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00);
	boe_td4165_dcs_write_seq_static(ctx, 0xED,0x01,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0xF5,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0B,0xa0,0xF5,0x00,0x00,0x00,0x00,0x00,0x40,0x10,0x00);
	//CABC PWM setting
	boe_td4165_dcs_write_seq_static(ctx, 0xB8,0x06,0x90,0x00,0x00,0x00,0x38);
	boe_td4165_dcs_write_seq_static(ctx, 0xB9,0x06,0x90,0x00,0x00,0x00,0x9a);
	boe_td4165_dcs_write_seq_static(ctx, 0xBA,0x06,0x90,0x00,0x00,0x00,0x9a);
	boe_td4165_dcs_write_seq_static(ctx, 0xCE,0x5D,0x58,0x62,0x6C,0x76,0x80,0x8B,0x95,0x9F,0xA9,0xB3,0xCA,0xE2,0xFA,0xFC,0xFE,0xFF,0x01,0x4b,0x04,0x04,0x00,0x03,0x68);
	//DSC setting
	boe_td4165_dcs_write_seq_static(ctx, 0xE7,0x11,0x00,0x89,0x30,0x80,0x07,0xc0,0x03,0x84,0x00,0x08,0x01,0xc2,0x01,0xc2,0x02,0x00,0x01,0xe1,0x20,0x00,0xab,0x00,0x06,0x0c,0x0d,0xb7,0x0f,0x41,0x18,0x00,0x10,0xf0,0x03,0x0c,0x20,0x00,0x06,0x0b,0x0b,0x33,0x0e,0x1c,0x2a,0x38,0x46,0x54,0x62,0x69,0x70,0x77,0x79,0x7b,0x7d,0x7e,0x01,0x02,0x01,0x00,0x09,0x40,0x09,0xbe,0x19,0xfc,0x19,0xfa,0x19,0xf8,0x1a,0x38,0x1a,0x78,0x1a,0xb6,0x2a,0xf6,0x2b,0x34,0x2b,0x74,0x3b,0x74,0x6b,0xf4,0x00,0x00,0x00,0x00,0x00,0x00);
	boe_td4165_dcs_write_seq_static(ctx, 0x51,0x07,0xFF);
	boe_td4165_dcs_write_seq_static(ctx, 0x53,0x0C);
	boe_td4165_dcs_write_seq_static(ctx, 0x55,0x00);
	boe_td4165_dcs_write_seq_static(ctx, 0x35,0x00);
	boe_td4165_dcs_write_seq_static(ctx, 0xB0,0x83);
	//Sleep Out
	boe_td4165_dcs_write_seq_static(ctx, 0x11,0x00);
	//Display On
	boe_td4165_dcs_write_seq_static(ctx, 0x29,0x00);
	usleep_range(80*1000, 81*1000);

	now = ktime_get();
	ctx->screen_on_timestamp = ktime_to_ms(now);
	pr_info("disp:%s -screen on timestamp: %lld \n", __func__, ctx->screen_on_timestamp);

	msleep(10);

	pr_info("%s-\n", __func__);
}

static int boe_td4165_disable(struct drm_panel *panel)
{
	struct boe_td4165 *ctx = panel_to_boe_td4165(panel);

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

static int boe_td4165_set_gesture_flag(int state)
{
	if(state == 1)
		tp_gesture_flag = 1;
	else
		tp_gesture_flag = 0;

	pr_info("%s:disp:set tp_gesture_flag:%d\n", __func__, tp_gesture_flag);
	return 0;
}

static int boe_td4165_unprepare(struct drm_panel *panel)
{
	struct boe_td4165 *ctx = panel_to_boe_td4165(panel);
	ktime_t now;
	s64 timestamp = 0;
	s64 diff = 0;

	if (!ctx->prepared) {
		pr_info("%s, already unprepared, return\n", __func__);
		return 0;
	}

	now = ktime_get();
	timestamp = ktime_to_ms(now);
	diff = timestamp - ctx->screen_on_timestamp;
	pr_info("disp %s -screen on and off time diff: %lld \n", __func__, diff);
	if (diff < 500 && diff >= 0) {
		// Per vendor, enforce 500ms min between screen on/off to prevent IC issues
		msleep(500 - diff);
	}

	pr_info("%s\n", __func__);

	boe_td4165_dcs_write_seq_static(ctx, 0x28);
	udelay(10 * 1000);
	boe_td4165_dcs_write_seq_static(ctx, 0x10);
	msleep(120);

	if(tp_gesture_flag)
		panel_gesture_notifier_call_chain(0x01,NULL);

	if(!tp_gesture_flag){
		ctx->reset_gpio = devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
		if (IS_ERR(ctx->reset_gpio)) {
			dev_err(ctx->dev, "%s:boe_td4165: cannot get reset_gpio %ld\n",
			__func__, PTR_ERR(ctx->reset_gpio));
			return PTR_ERR(ctx->reset_gpio);
		}
		gpiod_set_value(ctx->reset_gpio, 0);
		devm_gpiod_put(ctx->dev, ctx->reset_gpio);
		usleep_range(5000,5001);
		pr_info("%s:boe_td4165: reset_gpio 0\n", __func__);
	}
	//modify td4165 unprepare sequence: lcd reset low -> VSP VSN low -> tp reset low
	pr_info("%s:disp: tp_gesture_flag:%d, esd_recovery_flg=%d \n",__func__, tp_gesture_flag, mtkfb_esd_get_recovery_flag());
	if(!tp_gesture_flag || mtkfb_esd_get_recovery_flag()) {
		ocp2138_BiasPower_disable(5);
	}
	if(!tp_gesture_flag){
		msleep(5);
		ctx->tp_reset_gpio = devm_gpiod_get(ctx->dev, "tp_reset", GPIOD_OUT_HIGH);
		if (IS_ERR(ctx->tp_reset_gpio)) {
			dev_err(ctx->dev, "%s:boe_td4165: cannot get tp_reset_gpio %ld\n",
				__func__, PTR_ERR(ctx->tp_reset_gpio));
			//return PTR_ERR(ctx->tp_reset_gpio);
		}
		else{
			gpiod_set_value(ctx->tp_reset_gpio, 0);
			devm_gpiod_put(ctx->dev, ctx->tp_reset_gpio);
			usleep_range(5000,5001);
			pr_info("%s:boe_td4165: tp_reset_gpio 0\n", __func__);
		}
	}

	ctx->error = 0;
	ctx->prepared = false;

	pr_info("%s -\n", __func__);

	return 0;
}

static int boe_td4165_prepare(struct drm_panel *panel)
{
	struct boe_td4165 *ctx = panel_to_boe_td4165(panel);
	int ret;

	pr_info("disp: %s+\n", __func__);
	if (ctx->prepared) {
		pr_info("%s, already prepared, return\n", __func__);
		return 0;
	}

	boe_td4165_panel_init(ctx);
//	ctx->hbm_mode = 0;
	ctx->cabc_mode = 0;

	ret = ctx->error;
	if (ret < 0) {
		pr_info("disp: %s error ret=%d\n", __func__, ret);
		boe_td4165_unprepare(panel);
	}

	ctx->prepared = true;
/*#if defined(CONFIG_MTK_PANEL_EXT)
	mtk_panel_tch_rst(panel);
#endif

#ifdef PANEL_SUPPORT_READBACK
	boe_td4165_panel_get_data(ctx);
#endif*/
	pr_info("disp: %s-\n", __func__);
	return ret;
}

static int boe_td4165_enable(struct drm_panel *panel)
{
	struct boe_td4165 *ctx = panel_to_boe_td4165(panel);

	pr_info("disp: %s+\n", __func__);
	if (ctx->enabled)
		return 0;

	if (ctx->backlight) {
		ctx->backlight->props.power = FB_BLANK_UNBLANK;
		backlight_update_status(ctx->backlight);
	}

	ctx->enabled = true;

	pr_info("disp: %s-\n", __func__);
	return 0;
}

static const struct drm_display_mode performance_mode_60hz = {
	.clock		=  (int)((FRAME_WIDTH + HFP + HSA + HBP) * (FRAME_HEIGHT + MODE_60_VFP + VSA + VBP) * MODE_60_FPS / 1000),
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
	.clock		= (int)((FRAME_WIDTH + HFP + HSA + HBP) * (FRAME_HEIGHT + MODE_90_VFP + VSA + VBP) * MODE_90_FPS / 1000),
	.hdisplay = FRAME_WIDTH,
	.hsync_start = FRAME_WIDTH + MODE_90_HFP,
	.hsync_end = FRAME_WIDTH + MODE_90_HFP + HSA,
	.htotal = FRAME_WIDTH + MODE_90_HFP + HSA + HBP,
	.vdisplay = FRAME_HEIGHT,
	.vsync_start = FRAME_HEIGHT + MODE_90_VFP,
	.vsync_end = FRAME_HEIGHT + MODE_90_VFP + VSA,
	.vtotal = FRAME_HEIGHT + MODE_90_VFP + VSA + VBP,
};

static const struct drm_display_mode performance_mode_120hz = {
	.clock		= (int)((FRAME_WIDTH + HFP + HSA + HBP) * (FRAME_HEIGHT + MODE_120_VFP + VSA + VBP) * MODE_120_FPS / 1000),
	.hdisplay = FRAME_WIDTH,
	.hsync_start = FRAME_WIDTH + MODE_120_HFP,
	.hsync_end = FRAME_WIDTH + MODE_120_HFP + HSA,
	.htotal = FRAME_WIDTH + MODE_120_HFP + HSA + HBP,
	.vdisplay = FRAME_HEIGHT,
	.vsync_start = FRAME_HEIGHT + MODE_120_VFP,
	.vsync_end = FRAME_HEIGHT + MODE_120_VFP + VSA,
	.vtotal = FRAME_HEIGHT + MODE_120_VFP + VSA + VBP,
};

#if defined(CONFIG_MTK_PANEL_EXT)
static struct mtk_panel_params ext_params_60hz = {
	.data_rate = DATA_RATE,
	//.vfp_low_power = 880,
	.cust_esd_check = 1,
	.esd_check_enable = 1,
	.lcm_esd_check_table[0] = {
		.cmd = 0x0A,
		.count = 1,
		.para_list[0] = 0x9C,
	},

	.lcm_cellid = {
		.panel_cellid_reg = 0xBF,
		//.panel_cellid_reg_seq = 1,
		.panel_cellid_len = 23,
		.panel_cellid_read_max = 8,
		.panel_cellid_offset_reg = 0xBF,
		.panel_cellid_offset = 0x05,
		.panel_cellid_generic_read = 1,
		.panel_cellid_esd_dis = 1,
		//.page_cmd_always = 1,
		.page_table = {
			{0x15,0x02,0xB0,0x80}
		},
		.page_post_table = {
			{0x15,0x02,0xB0,0x83}
		},
	},
	.panel_ver = 1,
	//.panel_id = 0x01012891,
	.panel_name = "boe_td4165_vid_614_900_120hz",
	.panel_supplier = "boe",
	.lcm_index = 0,
	.max_bl_level = 2047,
	.hbm_type = HBM_MODE_RAMPING,
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
			.rc_buf_thresh = boe_td4165_rc_buf_thresh,
			.range_min_qp = boe_td4165_range_min_qp,
			.range_max_qp = boe_td4165_range_max_qp,
			.range_bpg_ofs = boe_td4165_range_bpg_ofs,
		},
	},
	/*.dyn = {
		.switch_en = 1,
		.pll_clk = 620,
		.hfp = 30,
	},*/

/*

	//.ssc_enable = 0,
	.lane_swap_en = 0,
	.lp_perline_en = 0,
	.output_mode = MTK_PANEL_DSC_SINGLE_PORT,
	.lfr_enable = LFR_EN,
	.lfr_minimum_fps = MODE_60_FPS,
*/
};

static struct mtk_panel_params ext_params_90hz = {
//	.vfp_low_power = 7476,//30hz
	.data_rate = DATA_RATE,
	.cust_esd_check = 1,
	.esd_check_enable = 1,
	.lcm_esd_check_table[0] = {
		.cmd = 0x0a,
		.count = 1,
		.para_list[0] = 0x9c,
	},

	.lcm_cellid = {
		.panel_cellid_reg = 0xBF,
		//.panel_cellid_reg_seq = 1,
		.panel_cellid_len = 23,
		.panel_cellid_read_max = 8,
		.panel_cellid_offset_reg = 0xBF,
		.panel_cellid_offset = 0x05,
		.panel_cellid_generic_read = 1,
		.panel_cellid_esd_dis = 1,
		//.page_cmd_always = 1,
		.page_table = {
			{0x15,0x02,0xB0,0x80}
		},
		.page_post_table = {
			{0x15,0x02,0xB0,0x83}
		},
	},
	.panel_ver = 1,
	//.panel_id = 0x01012891,
	.panel_name = "boe_td4165_vid_614_900_120hz",
	.panel_supplier = "boe",
	.lcm_index = 0,
	.max_bl_level = 2047,
	.hbm_type = HBM_MODE_RAMPING,
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
			.rc_buf_thresh = boe_td4165_rc_buf_thresh,
			.range_min_qp = boe_td4165_range_min_qp,
			.range_max_qp = boe_td4165_range_max_qp,
			.range_bpg_ofs = boe_td4165_range_bpg_ofs,
		},
	},
	/*.dyn = {
		.switch_en = 1,
		.pll_clk = 620,
		.hfp = 30,
	},*/

/*
	.ssc_enable = 0,
	.lane_swap_en = 0,
	.lp_perline_en = 0,
	.output_mode = MTK_PANEL_DSC_SINGLE_PORT,
	.lfr_enable = LFR_EN,
	.lfr_minimum_fps = MODE_60_FPS,
*/
};

static struct mtk_panel_params ext_params_120hz = {
//	.vfp_low_power = 7476,//30hz
	.data_rate = DATA_RATE,
	.cust_esd_check = 1,
	.esd_check_enable = 1,
	.lcm_esd_check_table[0] = {
		.cmd = 0x0A,
		.count = 1,
		.para_list[0] = 0x9C,
	},

	.lcm_cellid = {
		.panel_cellid_reg = 0xBF,
		//.panel_cellid_reg_seq = 1,
		.panel_cellid_len = 23,
		.panel_cellid_read_max = 8,
		.panel_cellid_offset_reg = 0xBF,
		.panel_cellid_offset = 0x05,
		.panel_cellid_generic_read = 1,
		.panel_cellid_esd_dis = 1,
		//.page_cmd_always = 1,
		.page_table = {
			{0x15,0x02,0xB0,0x80}
		},
		.page_post_table = {
			{0x15,0x02,0xB0,0x83}
		},
	},
	.panel_ver = 1,
	//.panel_id = 0x01012891,
	.panel_name = "boe_td4165_vid_614_900_120hz",
	.panel_supplier = "boe",
	.lcm_index = 0,
	.max_bl_level = 2047,
	.hbm_type = HBM_MODE_RAMPING,
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
			.rc_buf_thresh = boe_td4165_rc_buf_thresh,
			.range_min_qp = boe_td4165_range_min_qp,
			.range_max_qp = boe_td4165_range_max_qp,
			.range_bpg_ofs = boe_td4165_range_bpg_ofs,
		},
	},
	/*.dyn = {
		.switch_en = 1,
		.pll_clk = 620,
		.hfp = 30,
	},*/

/*
	.ssc_enable = 0,
	.lane_swap_en = 0,
	.lp_perline_en = 0,
	.output_mode = MTK_PANEL_DSC_SINGLE_PORT,
	.lfr_enable = LFR_EN,
	.lfr_minimum_fps = MODE_60_FPS,
*/
};

static int boe_td4165_setbacklight_cmdq(void *dsi, dcs_write_gce cb, void *handle,
				 unsigned int level)
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

struct drm_display_mode *get_mode_by_id_hfp(struct drm_connector *connector,
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
	struct drm_display_mode *m = get_mode_by_id_hfp(connector, mode);

	if (!m) {
		pr_err("%s:%d invalid display_mode\n", __func__, __LINE__);
		return ret;
	}

	pr_info("%s:disp: mode fps=%d", __func__, drm_mode_vrefresh(m));
/*
	if (drm_mode_vrefresh(m) == 60)
		ext->params = &ext_params_60hz;
*/

	if (drm_mode_vrefresh(m) == MODE_120_FPS)
		ext->params = &ext_params_120hz;
	else if (drm_mode_vrefresh(m) == MODE_60_FPS)
		ext->params = &ext_params_60hz;
	else if (drm_mode_vrefresh(m) == MODE_90_FPS)
		ext->params = &ext_params_90hz;
	else
		ret = 1;

	return ret;
}

static int panel_ext_reset(struct drm_panel *panel, int on)
{
	struct boe_td4165 *ctx = panel_to_boe_td4165(panel);

	pr_info("%s+ \n", __func__);

	ctx->reset_gpio =
		devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
	gpiod_set_value(ctx->reset_gpio, on);
	devm_gpiod_put(ctx->dev, ctx->reset_gpio);

	return 0;
}

static enum mtk_lcm_version td4165_get_lcm_version(void)
{
	return MTK_LEGACY_LCM_DRV_WITH_BACKLIGHTCLASS;
}

static int panel_cabc_set_cmdq(struct boe_td4165 *ctx, void *dsi, dcs_grp_write_gce cb, void *handle, uint32_t cabc_mode)
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

#if 0
static int panel_hbm_set_cmdq(struct boe_td4165 *ctx, void *dsi, dcs_grp_write_gce cb, void *handle, uint32_t hbm_state)
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
	struct boe_td4165 *ctx = panel_to_boe_td4165(panel);
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
		/*	if (ctx->hbm_mode != param_info.value) {
				ctx->hbm_mode = param_info.value;
				panel_hbm_set_cmdq(ctx, dsi, cb, handle, param_info.value);
				pr_debug("%s: set HBM to %d end\n", __func__, param_info.value);
				ret = 0;
			}
			else
				pr_info("%s: skip same HBM mode:%d\n", __func__, ctx->hbm_mode);
		*/
				pr_info("%s: set HBM to %d end\n", __func__, param_info.value);
			break;
		default:
			pr_info("%s: skip unsupport feature %d to %d\n", __func__, param_info.param_idx, param_info.value);
			break;
	}

	return ret;
}

static struct mtk_panel_funcs ext_funcs = {
	.reset = panel_ext_reset,
	.set_backlight_cmdq = boe_td4165_setbacklight_cmdq,
	.ext_param_set = mtk_panel_ext_param_set,
	.get_lcm_version =  td4165_get_lcm_version,
//	.ata_check = panel_ata_check,
	.set_gesture_flag = boe_td4165_set_gesture_flag,
	.panel_feature_set = panel_feature_set,
};
#endif

static int boe_td4165_get_modes(struct drm_panel *panel,
					struct drm_connector *connector)
{
	//struct drm_display_mode *mode;
	struct drm_display_mode *mode_1;
	struct drm_display_mode *mode_2;
	struct drm_display_mode *mode_3;

#if 0
	mode = drm_mode_duplicate(connector->dev, &performance_mode_60hz);
	printk("[%d  %s]disp: mode:\n",__LINE__, __FUNCTION__,mode);
	if (!mode) {
		dev_info(connector->dev->dev, "failed to add mode %ux%ux@%u\n",
			 performance_mode_60hz.hdisplay, performance_mode_60hz.vdisplay,
			 drm_mode_vrefresh(&performance_mode_60hz));
		return -ENOMEM;
	}

	drm_mode_set_name(mode);
	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	drm_mode_probed_add(connector, mode);
#endif

//TBD
	mode_1 = drm_mode_duplicate(connector->dev, &performance_mode_60hz);
	printk("[%d  %s]disp mode:%d\n",__LINE__, __FUNCTION__,mode_1);
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

	mode_3 = drm_mode_duplicate(connector->dev, &performance_mode_120hz);
	if (!mode_3) {
		dev_err(connector->dev->dev, "failed to add mode %ux%ux@%u\n",
			performance_mode_120hz.hdisplay,
			performance_mode_120hz.vdisplay,
			drm_mode_vrefresh(&performance_mode_120hz));
		return -ENOMEM;
	}
	drm_mode_set_name(mode_3);
	mode_3->type = DRM_MODE_TYPE_DRIVER;
	drm_mode_probed_add(connector, mode_3);

	connector->display_info.width_mm = 70;
	connector->display_info.height_mm = 154;
	printk("[%d  %s]end\n",__LINE__, __FUNCTION__);

	return 1;
}

static const struct drm_panel_funcs boe_td4165_drm_funcs = {
	.disable = boe_td4165_disable,
	.unprepare = boe_td4165_unprepare,
	.prepare = boe_td4165_prepare,
	.enable = boe_td4165_enable,
	.get_modes = boe_td4165_get_modes,
};

static int boe_td4165_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct device_node *dsi_node, *remote_node = NULL, *endpoint = NULL;
	struct boe_td4165 *ctx;
	struct device_node *backlight;
	int ret;

	pr_info("%s+ disp:boe,td4165,vdo,120hz\n", __func__);

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

	ctx = devm_kzalloc(dev, sizeof(struct boe_td4165), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	mipi_dsi_set_drvdata(dsi, ctx);

	ctx->dev = dev;
	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST;
//			 | MIPI_DSI_MODE_LPM | MIPI_DSI_MODE_EOT_PACKET
//			 | MIPI_DSI_CLOCK_NON_CONTINUOUS;

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
	drm_panel_init(&ctx->panel, dev, &boe_td4165_drm_funcs, DRM_MODE_CONNECTOR_DSI);

	drm_panel_add(&ctx->panel);

	ret = mipi_dsi_attach(dsi);
	if (ret < 0)
		drm_panel_remove(&ctx->panel);

#if defined(CONFIG_MTK_PANEL_EXT)
	mtk_panel_tch_handle_reg(&ctx->panel);
	ret = mtk_panel_ext_create(dev, &ext_params_60hz, &ext_funcs, &ctx->panel);
	if (ret < 0)
		return ret;

#endif

	pr_info("[%d  %s]- boe,td4165,vdo,120hz ret:%d\n", __LINE__, __func__,ret);

	return ret;
}

static int boe_td4165_remove(struct mipi_dsi_device *dsi)
{
	struct boe_td4165 *ctx = mipi_dsi_get_drvdata(dsi);
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
	struct boe_td4165 *ctx = mipi_dsi_get_drvdata(dsi);

	pr_info("%s\n", __func__);
	ctx->reset_gpio = devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio)) {
	    dev_err(ctx->dev, "%s:boe_td4165: cannot get reset_gpio %ld\n",
	    __func__, PTR_ERR(ctx->reset_gpio));
	} else {
	    gpiod_set_value(ctx->reset_gpio, 0);
	    devm_gpiod_put(ctx->dev, ctx->reset_gpio);
	    pr_info("%s:boe_td4165: reset_gpio 0\n", __func__);
	    usleep_range(5000,5001);
	}

	pr_info("%s: ocp2138_BiasPower_disable\n", __func__);
	ocp2138_BiasPower_disable(5);
	//add TP reset low when device shutdown.
	msleep(5);
	ctx->tp_reset_gpio = devm_gpiod_get(ctx->dev, "tp_reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->tp_reset_gpio)) {
			dev_err(ctx->dev, "%s:boe_td4165: cannot get tp_reset_gpio %ld\n",
				__func__, PTR_ERR(ctx->tp_reset_gpio));
			//return PTR_ERR(ctx->tp_reset_gpio);
	} else {
			gpiod_set_value(ctx->tp_reset_gpio, 0);
			devm_gpiod_put(ctx->dev, ctx->tp_reset_gpio);
			usleep_range(5000,5001);
			pr_info("%s:boe_td4165: tp_reset_gpio 0\n", __func__);
	}
}

static const struct of_device_id boe_td4165_of_match[] = {
	{
#if defined(CONFIG_DRM_PANEL_NUM_NO_LIMIT)
		.compatible = "boe_td4165_vid_614_900_120hz",
#else
		.compatible = "boe,td4165,vdo,120hz",
#endif
	},
	{}
};

MODULE_DEVICE_TABLE(of, boe_td4165_of_match);

static struct mipi_dsi_driver boe_td4165_driver = {
	.probe = boe_td4165_probe,
	.remove = boe_td4165_remove,
	.shutdown = lcm_shutdown,
	.driver = {
		.name = "boe_td4165_vid_614_900_120hz",
		.owner = THIS_MODULE,
		.of_match_table = boe_td4165_of_match,
	},
};

module_mipi_dsi_driver(boe_td4165_driver);

MODULE_AUTHOR("mediatek");
MODULE_DESCRIPTION("boe td4165 incell 120hz Panel Driver");
MODULE_LICENSE("GPL v2");

