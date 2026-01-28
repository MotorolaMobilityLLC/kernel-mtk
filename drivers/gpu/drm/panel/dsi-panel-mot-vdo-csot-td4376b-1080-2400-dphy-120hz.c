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
#include "include/dsi-panel-mot-vdo-csot-td4376b-1080-2400-dphy-120hz.h"
#endif

#define PANEL_EVT 1
#define PANEL_DVT1 2
#define PANEL_DVT2 3
#define PANEL_PVT 4
/* option function to read data from some panel address */
/* #define PANEL_SUPPORT_READBACK */

extern int __attribute__ ((weak)) ocp2138_BiasPower_disable(u32 pwrdown_delay);
extern int __attribute__ ((weak)) ocp2138_BiasPower_enable(u32 avdd, u32 avee,u32 pwrup_delay);
extern int mtkfb_esd_get_recovery_flag(void);
static BLOCKING_NOTIFIER_HEAD(panel_gesture_notifier_list);

static int tp_gesture_flag = 0;

struct csot_td4376b {
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
	int version;
};

static struct mtk_panel_para_table panel_cabc_ui[] = {
	{2, {0x55, 0x02}},
};

static struct mtk_panel_para_table panel_cabc_mv[] = {
	{2, {0x55, 0x03}},
};

static struct mtk_panel_para_table panel_cabc_disable[] = {
	{2, {0x55, 0x00}},
};

static const DbvHalMap dbv_hal_map[] = {
    {0, 57, 0, 5, CALC_RANGES(0, 57, 0, 5)},
    {58, 81, 6, 9, CALC_RANGES(58, 81, 6, 9)},
    {82, 128, 10, 18, CALC_RANGES(82, 128, 10, 18)},
    {129, 297, 19, 67, CALC_RANGES(129, 297, 19, 67)},
    {298, 378, 68, 96, CALC_RANGES(298, 378, 68, 96)},
    {379, 517, 97, 155, CALC_RANGES(379, 517, 97, 155)},
    {518, 639, 156, 213, CALC_RANGES(518, 639, 156, 213)},
    {640, 678, 214, 233, CALC_RANGES(640, 678, 214, 233)},
    {679, 715, 234, 252, CALC_RANGES(679, 715, 234, 252)},
    {716, 786, 253, 291, CALC_RANGES(716, 786, 253, 291)},
    {787, 1015, 292, 428, CALC_RANGES(787, 1015, 292, 428)},
    {1016, 1249, 429, 584, CALC_RANGES(1016, 1249, 429, 584)},
    {1250, 1304, 585, 623, CALC_RANGES(1250, 1304, 585, 623)},
    {1305, 1384, 624, 681, CALC_RANGES(1305, 1384, 624, 681)},
    {1385, 1637, 682, 876, CALC_RANGES(1385, 1637, 682, 876)},
    {1638, 1756, 877, 974, CALC_RANGES(1638, 1756, 877, 974)},
    {1757, 1862, 975, 1364, CALC_RANGES(1757, 1862, 975, 1364)},
    {1863, 1915, 1365, 1559, CALC_RANGES(1863, 1915, 1365, 1559)},
    {1916, 2047, 1560, 2047, CALC_RANGES(1916, 2047, 1560, 2047)}
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

//struct csot_td4376b *g_ctx = NULL;

#define csot_td4376b_dcs_write_seq(ctx, seq...)                                         \
	({                                                                     \
		const u8 d[] = { seq };                                        \
		BUILD_BUG_ON_MSG(ARRAY_SIZE(d) > 64,                           \
				 "DCS sequence too big for stack");            \
		csot_td4376b_dcs_write(ctx, d, ARRAY_SIZE(d));                          \
	})

#define csot_td4376b_dcs_write_seq_static(ctx, seq...)                                  \
	({                                                                     \
		static const u8 d[] = { seq };                                 \
		csot_td4376b_dcs_write(ctx, d, ARRAY_SIZE(d));                          \
	})

static inline struct csot_td4376b *panel_to_csot_td4376b(struct drm_panel *panel)
{
	return container_of(panel, struct csot_td4376b, panel);
}

#ifdef PANEL_SUPPORT_READBACK
static int csot_td4376b_dcs_read(struct csot_td4376b *ctx, u8 cmd, void *data, size_t len)
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

static void csot_td4376b_panel_get_data(struct csot_td4376b *ctx)
{
	u8 buffer[3] = {0};
	static int ret;

	if (ret == 0) {
		ret = csot_td4376b_dcs_read(ctx, 0x0A, buffer, 1);
		pr_info("disp: %s 0x%08x\n", __func__, buffer[0] | (buffer[1] << 8));
		dev_info(ctx->dev, "return %d data(0x%08x) to dsi engine\n",
			 ret, buffer[0] | (buffer[1] << 8));
	}
}
#endif

static void csot_td4376b_dcs_write(struct csot_td4376b *ctx, const void *data, size_t len)
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

static void csot_panel_tp_reset(struct csot_td4376b *ctx)
{
	pr_info("%s:csot_td4376b: +\n", __func__);

	ctx->tp_reset_gpio = devm_gpiod_get(ctx->dev, "tp_reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->tp_reset_gpio)) {
		dev_err(ctx->dev, "%s:csot_td4376b: cannot get tp_reset_gpio %ld\n",
			__func__, PTR_ERR(ctx->tp_reset_gpio));
		//return;
	}
	else {
		gpiod_set_value(ctx->tp_reset_gpio, 1);
		udelay(3 * 1000);
		devm_gpiod_put(ctx->dev, ctx->tp_reset_gpio);
		pr_info("%s:csot_td4376b: tp_reset_gpio 1\n", __func__);
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

static void lcm_parse_panel_version(struct csot_td4376b *ctx)
{
	int rc;
	struct device_node *chosen = of_find_node_by_name(NULL, "chosen");

	ctx->version = PANEL_PVT;
	if(chosen) {
		u32 tmp = 0;

		rc = of_property_read_u32(chosen, "mmi,panel_ver", &tmp);
		if (!rc) {
			switch (tmp) {
				case PANEL_EVT:
					ctx->version = PANEL_EVT;
					break;
				case PANEL_DVT1:
					ctx->version = PANEL_DVT1;
					break;
				case PANEL_DVT2:
					ctx->version = PANEL_DVT2;
					break;
				case PANEL_PVT:
				default:
					ctx->version = PANEL_PVT;
					break;
			}
			pr_info("get panel_ver:%d\n", ctx->version);
		}
		else
			pr_info("mmi,panel_ver not get\n");
	}
	else
		pr_info("parse_panel chosen node null\n");
}

static void csot_td4376b_panel_init(struct csot_td4376b *ctx)
{
	ktime_t now;
	pr_info("disp: %s+\n", __func__);

	ocp2138_BiasPower_enable(15,15,5);

	csot_panel_tp_reset(ctx);

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
		msleep(20);
		devm_gpiod_put(ctx->dev, ctx->reset_gpio);
		pr_info("disp: %s reset_gpio\n", __func__);
	}

	csot_td4376b_dcs_write_seq_static(ctx, 0xB0, 0x00);
	csot_td4376b_dcs_write_seq_static(ctx, 0xD6, 0x00);
	csot_td4376b_dcs_write_seq_static(ctx, 0xE7, 0x11, 0x00, 0x89, 0x30, 0x80, 0x09, 0x60, 0x04, 0x38, 0x00, 0x08, 0x02, 0x1c, 0x02, 0x1c, 0x02, 0x00, 0x02, 0x0e, 0x20, 0x00, 0xBB, 0x00, 0x07, 0x0c, 0x0D, 0xB7, 0x0C, 0xB7);

	csot_td4376b_dcs_write_seq_static(ctx, 0xB8, 0x31, 0x55, 0x00, 0x00, 0x00, 0x00, 0x00);
	csot_td4376b_dcs_write_seq_static(ctx, 0xB9, 0x7D, 0x5B, 0x00, 0x00, 0x00, 0x00, 0x00);
	csot_td4376b_dcs_write_seq_static(ctx, 0xBA, 0x7D, 0x5B, 0x00, 0x00, 0x00, 0x00, 0x00);
	csot_td4376b_dcs_write_seq_static(ctx, 0xCE, 0x5D, 0x60, 0x62, 0x65, 0x69, 0x6C, 0x76, 0x81, 0x8D, 0x99, 0xA4, 0xB0, 0xC2, 0xD4, 0xE8, 0xF3, 0xF7, 0xFF, 0x01, 0x7A, 0x0f, 0x0f, 0x00, 0x43, 0x69, 0x5a, 0x40, 0x43, 0x00, 0x00, 0x00, 0x64, 0xfa, 0x00, 0x00);
	// setting brightness (DCS 0x51) to 0x0000 is intended to prevent a screen flash during boot-up.
	csot_td4376b_dcs_write_seq_static(ctx, 0x51, 0x00,0x00);
	csot_td4376b_dcs_write_seq_static(ctx, 0x53, 0x2C);
	csot_td4376b_dcs_write_seq_static(ctx, 0x55, 0x00);
	csot_td4376b_dcs_write_seq_static(ctx, 0x31, 0x00);
	csot_td4376b_dcs_write_seq_static(ctx, 0xB0, 0x03);

	//Sleep Out
	csot_td4376b_dcs_write_seq_static(ctx, 0x11);
	//Display On
	csot_td4376b_dcs_write_seq_static(ctx, 0x29);
	// Adjust display on delay per new vendor timing sequence`
	usleep_range(80*1000, 81*1000);

	now = ktime_get();
	ctx->screen_on_timestamp = ktime_to_ms(now);
	pr_info("disp:%s -screen on timestamp: %lld \n", __func__, ctx->screen_on_timestamp);

	msleep(10);

	pr_info("%s-\n", __func__);
}

static int csot_td4376b_disable(struct drm_panel *panel)
{
	struct csot_td4376b *ctx = panel_to_csot_td4376b(panel);

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

static int csot_td4376b_set_gesture_flag(int state)
{
	if(state == 1)
		tp_gesture_flag = 1;
	else
		tp_gesture_flag = 0;

	pr_info("%s:disp:set tp_gesture_flag:%d\n", __func__, tp_gesture_flag);
	return 0;
}

static int csot_td4376b_unprepare(struct drm_panel *panel)
{
	struct csot_td4376b *ctx = panel_to_csot_td4376b(panel);
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

	csot_td4376b_dcs_write_seq_static(ctx, 0x28);
	udelay(10 * 1000);
	csot_td4376b_dcs_write_seq_static(ctx, 0x10);
	// Increase delay to 150ms to meet panel vendor spec for clean power-down
	msleep(150);

	if(tp_gesture_flag)
		panel_gesture_notifier_call_chain(0x01,NULL);

	if(!tp_gesture_flag){
		ctx->reset_gpio = devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
		if (IS_ERR(ctx->reset_gpio)) {
			dev_err(ctx->dev, "%s:csot_td4376b: cannot get reset_gpio %ld\n",
			__func__, PTR_ERR(ctx->reset_gpio));
			return PTR_ERR(ctx->reset_gpio);
		}
		gpiod_set_value(ctx->reset_gpio, 0);
		devm_gpiod_put(ctx->dev, ctx->reset_gpio);
		usleep_range(5000,5001);
		pr_info("%s:csot_td4376b: reset_gpio 0\n", __func__);
	}
	// Disable bias power before resetting GPIOs per new power-off sequence
	pr_info("%s:disp: tp_gesture_flag:%d, esd_recovery_flg=%d \n",__func__, tp_gesture_flag, mtkfb_esd_get_recovery_flag());
	if(!tp_gesture_flag || mtkfb_esd_get_recovery_flag()) {
		ocp2138_BiasPower_disable(5);
	}
	if(!tp_gesture_flag){
		msleep(5);
		ctx->tp_reset_gpio = devm_gpiod_get(ctx->dev, "tp_reset", GPIOD_OUT_HIGH);
		if (IS_ERR(ctx->tp_reset_gpio)) {
			dev_err(ctx->dev, "%s:csot_td4376b: cannot get tp_reset_gpio %ld\n",
				__func__, PTR_ERR(ctx->tp_reset_gpio));
			//return PTR_ERR(ctx->tp_reset_gpio);
		}
		else{
			gpiod_set_value(ctx->tp_reset_gpio, 0);
			devm_gpiod_put(ctx->dev, ctx->tp_reset_gpio);
			usleep_range(5000,5001);
			pr_info("%s:csot_td4376b: tp_reset_gpio 0\n", __func__);
		}
	}

	ctx->error = 0;
	ctx->prepared = false;

	pr_info("%s -\n", __func__);

	return 0;
}

static int csot_td4376b_prepare(struct drm_panel *panel)
{
	struct csot_td4376b *ctx = panel_to_csot_td4376b(panel);
	int ret;

	pr_info("disp: %s+\n", __func__);
	if (ctx->prepared) {
		pr_info("%s, already prepared, return\n", __func__);
		return 0;
	}

	csot_td4376b_panel_init(ctx);
//	ctx->hbm_mode = 0;
	ctx->cabc_mode = 0;

	ret = ctx->error;
	if (ret < 0) {
		pr_info("disp: %s error ret=%d\n", __func__, ret);
		csot_td4376b_unprepare(panel);
	}

	ctx->prepared = true;
/*#if defined(CONFIG_MTK_PANEL_EXT)
	mtk_panel_tch_rst(panel);
#endif

#ifdef PANEL_SUPPORT_READBACK
	boe_ili77600a_panel_get_data(ctx);
#endif*/
	pr_info("disp: %s-\n", __func__);
	return ret;
}

static int csot_td4376b_enable(struct drm_panel *panel)
{
	struct csot_td4376b *ctx = panel_to_csot_td4376b(panel);

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

static const struct drm_display_mode performance_mode_30hz = {
	.clock		=  (int)((FRAME_WIDTH + HFP + HSA + HBP) * (FRAME_HEIGHT + MODE_30_VFP + VSA + VBP) * MODE_30_FPS / 1000),
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
static struct mtk_panel_params ext_params_30hz = {
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
		.panel_cellid_reg = 0x00,
		.panel_cellid_reg_seq = 1,
		.panel_cellid_len = 23,
		.panel_cellid_read_max = 1,
	},
	.panel_ver = 1,
	//.panel_id = 0x01012891,
	.panel_name = "csot_td4376b_672",
	.panel_supplier = "csot",
	.lcm_index = 1,
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

};

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
		.panel_cellid_reg = 0x00,
		.panel_cellid_reg_seq = 1,
		.panel_cellid_len = 23,
		.panel_cellid_read_max = 1,
	},
	.panel_ver = 1,
	//.panel_id = 0x01012891,
	.panel_name = "csot_td4376b_672",
	.panel_supplier = "csot",
	.lcm_index = 1,
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
		.panel_cellid_reg = 0x00,
		.panel_cellid_reg_seq = 1,
		.panel_cellid_len = 23,
		.panel_cellid_read_max = 1,
		//.panel_cellid_esd_dis = 1,
	},
	.panel_ver = 1,
	//.panel_id = 0x01012891,
	.panel_name = "csot_td4376b_672",
	.panel_supplier = "csot",
	.lcm_index = 1,
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
		.panel_cellid_reg = 0x00,
		.panel_cellid_reg_seq = 1,
		.panel_cellid_len = 23,
		.panel_cellid_read_max = 1,
		//.panel_cellid_esd_dis = 1,
	},
	.panel_ver = 1,
	//.panel_id = 0x01012891,
	.panel_name = "csot_td4376b_672",
	.panel_supplier = "csot",
	.lcm_index = 1,
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

};

/**
* @brief
*
* @param dbv
* @param hal_value
* @return bool
*/
bool dbv_to_hal(uint16_t dbv, uint16_t *hal_value) {
    int left = 0;
    int right = MAP_SIZE - 1;
    int mid;

    if (hal_value == NULL) {
        return false;
    }

    if (dbv > 2047) {
        *hal_value = 0;
        return false;
    }

    while (left <= right) {
        mid = (left + right) / 2;

        if (dbv >= dbv_hal_map[mid].start && dbv <= dbv_hal_map[mid].end) {
            uint32_t dbv_offset = dbv - dbv_hal_map[mid].start;

            uint32_t scaled_offset = (dbv_offset * dbv_hal_map[mid].hal_range) << 12;

            uint32_t hal_offset = (scaled_offset / dbv_hal_map[mid].dbv_range) >> 12;

            *hal_value = dbv_hal_map[mid].hal_start + hal_offset;

            if (*hal_value < dbv_hal_map[mid].hal_start) {
                *hal_value = dbv_hal_map[mid].hal_start;
            } else if (*hal_value > dbv_hal_map[mid].hal_end) {
                *hal_value = dbv_hal_map[mid].hal_end;
            }
            *hal_value = *hal_value * 1961 / 2047;

            return true;
        }
        else if (dbv < dbv_hal_map[mid].start) {
            right = mid - 1;
        }
        else {
            left = mid + 1;
        }
    }

    *hal_value = 0;
    return false;
}

static int csot_td4376b_setbacklight_cmdq(void *dsi, dcs_write_gce cb, void *handle,
				 unsigned int level)
{
	static char bl_tb0[] = { 0x51, 0x7f, 0xff };
	uint16_t hal_value;

	if (dbv_to_hal(level, &hal_value)) {
		pr_info("%s backlight (dbv) = %d, hal = %d\n", __func__, level, hal_value);
	} else {
		pr_err("%s dbv_to_hal failed for level %d\n", __func__, level);
		return -1;
	}

	bl_tb0[1] = (hal_value >> 8) & 0x7F;
	bl_tb0[2] = hal_value & 0xFF;

	if (!cb)
		return -1;

	cb(dsi, handle, bl_tb0, ARRAY_SIZE(bl_tb0));

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

	if (drm_mode_vrefresh(m) == MODE_120_FPS)
		ext->params = &ext_params_120hz;
	else if (drm_mode_vrefresh(m) == MODE_30_FPS)
		ext->params = &ext_params_30hz;
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
	struct csot_td4376b *ctx = panel_to_csot_td4376b(panel);

	pr_info("%s+ \n", __func__);

	ctx->reset_gpio =
		devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
	gpiod_set_value(ctx->reset_gpio, on);
	devm_gpiod_put(ctx->dev, ctx->reset_gpio);

	return 0;
}

static int panel_cabc_set_cmdq(struct csot_td4376b *ctx, void *dsi, dcs_grp_write_gce cb, void *handle, uint32_t cabc_mode)
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
static int panel_hbm_set_cmdq(struct csot_td4376b *ctx, void *dsi, dcs_grp_write_gce cb, void *handle, uint32_t hbm_state)
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
	struct csot_td4376b *ctx = panel_to_csot_td4376b(panel);
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
	.set_backlight_cmdq = csot_td4376b_setbacklight_cmdq,
	.ext_param_set = mtk_panel_ext_param_set,
//	.get_lcm_version =  td4376b_get_lcm_version,
//	.ata_check = panel_ata_check,
	.set_gesture_flag = csot_td4376b_set_gesture_flag,
	.panel_feature_set = panel_feature_set,
};
#endif

static int csot_td4376b_get_modes(struct drm_panel *panel,
					struct drm_connector *connector)
{
	struct drm_display_mode *mode;
	struct drm_display_mode *mode_1;
	struct drm_display_mode *mode_2;
	struct drm_display_mode *mode_3;

	mode = drm_mode_duplicate(connector->dev, &performance_mode_120hz);
	printk("[%d  %s]disp: mode:\n",__LINE__, __FUNCTION__,mode);
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

	mode_2 = drm_mode_duplicate(connector->dev, &performance_mode_60hz);
	printk("[%d  %s]disp mode:%d\n",__LINE__, __FUNCTION__,mode_2);
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
	printk("[%d  %s]end\n",__LINE__, __FUNCTION__);

	return 1;
}

static const struct drm_panel_funcs csot_td4376b_drm_funcs = {
	.disable = csot_td4376b_disable,
	.unprepare = csot_td4376b_unprepare,
	.prepare = csot_td4376b_prepare,
	.enable = csot_td4376b_enable,
	.get_modes = csot_td4376b_get_modes,
};

static int csot_td4376b_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct device_node *dsi_node, *remote_node = NULL, *endpoint = NULL;
	struct csot_td4376b *ctx;
	struct device_node *backlight;
	int ret;

	pr_info("%s+ disp:csot,td4376b,vdo,120hz\n", __func__);

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

	ctx = devm_kzalloc(dev, sizeof(struct csot_td4376b), GFP_KERNEL);
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
	drm_panel_init(&ctx->panel, dev, &csot_td4376b_drm_funcs, DRM_MODE_CONNECTOR_DSI);

	drm_panel_add(&ctx->panel);

	//parse panel version for evt/dvt/pvt
	lcm_parse_panel_version(ctx);

	ret = mipi_dsi_attach(dsi);
	if (ret < 0)
		drm_panel_remove(&ctx->panel);

#if defined(CONFIG_MTK_PANEL_EXT)
	mtk_panel_tch_handle_reg(&ctx->panel);
	ret = mtk_panel_ext_create(dev, &ext_params_60hz, &ext_funcs, &ctx->panel);
	if (ret < 0)
		return ret;

#endif

	pr_info("[%d  %s]- csot,td4376b,vdo,120hz ret:%d\n", __LINE__, __func__,ret);

	return ret;
}

static int csot_td4376b_remove(struct mipi_dsi_device *dsi)
{
	struct csot_td4376b *ctx = mipi_dsi_get_drvdata(dsi);
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

static const struct of_device_id csot_td4376b_of_match[] = {
	{
#if defined(CONFIG_DRM_PANEL_NUM_NO_LIMIT)
		.compatible = "csot_td4376b_672",
#else
		.compatible = "csot,td4376b,672",
#endif
	},
	{}
};

MODULE_DEVICE_TABLE(of, csot_td4376b_of_match);

static struct mipi_dsi_driver csot_td4376b_driver = {
	.probe = csot_td4376b_probe,
	.remove = csot_td4376b_remove,
	.shutdown = lcm_shutdown,
	.driver = {
		.name = "csot_td4376b_672",
		.owner = THIS_MODULE,
		.of_match_table = csot_td4376b_of_match,
	},
};

module_mipi_dsi_driver(csot_td4376b_driver);

MODULE_AUTHOR("Motorola Mobility LLC");
MODULE_DESCRIPTION("csot td4376b incell 120hz Panel Driver");
MODULE_LICENSE("GPL v2");

