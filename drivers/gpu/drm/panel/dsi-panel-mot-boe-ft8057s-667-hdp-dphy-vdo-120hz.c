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
#include "include/dsi-panel-mot-boe-ft8057s-667-hdp-dphy-vdo-120hz.h"
#endif

/* option function to read data from some panel address */
/* #define PANEL_SUPPORT_READBACK */

extern int __attribute__ ((weak)) ocp2138_BiasPower_disable(u32 pwrdown_delay);
extern int __attribute__ ((weak)) ocp2138_BiasPower_enable(u32 avdd, u32 avee,u32 pwrup_delay);

static int tp_gesture_flag = 0;

struct boe_ft8057s {
	struct device *dev;
	struct drm_panel panel;
	struct backlight_device *backlight;
	struct gpio_desc *reset_gpio;
	struct gpio_desc *bias_n_gpio;
	struct gpio_desc *bias_p_gpio;

	bool prepared;
	bool enabled;

	int error;
	unsigned int cabc_mode;
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

//static char bl_tb0[] = { 0x51, 0xff };

//struct boe_ft8057s *g_ctx = NULL;

#define boe_ft8057s_dcs_write_seq(ctx, seq...)                                         \
	({                                                                     \
		const u8 d[] = { seq };                                        \
		BUILD_BUG_ON_MSG(ARRAY_SIZE(d) > 64,                           \
				 "DCS sequence too big for stack");            \
		boe_ft8057s_dcs_write(ctx, d, ARRAY_SIZE(d));                          \
	})

#define boe_ft8057s_dcs_write_seq_static(ctx, seq...)                                  \
	({                                                                     \
		static const u8 d[] = { seq };                                 \
		boe_ft8057s_dcs_write(ctx, d, ARRAY_SIZE(d));                          \
	})

static inline struct boe_ft8057s *panel_to_boe_ft8057s(struct drm_panel *panel)
{
	return container_of(panel, struct boe_ft8057s, panel);
}

#ifdef PANEL_SUPPORT_READBACK
static int boe_ft8057s_dcs_read(struct boe_ft8057s *ctx, u8 cmd, void *data, size_t len)
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

static void boe_ft8057s_panel_get_data(struct boe_ft8057s *ctx)
{
	u8 buffer[3] = {0};
	static int ret;

	if (ret == 0) {
		ret = boe_ft8057s_dcs_read(ctx, 0x0A, buffer, 1);
		pr_info("disp: %s 0x%08x\n", __func__, buffer[0] | (buffer[1] << 8));
		dev_info(ctx->dev, "return %d data(0x%08x) to dsi engine\n",
			 ret, buffer[0] | (buffer[1] << 8));
	}
}
#endif

static void boe_ft8057s_dcs_write(struct boe_ft8057s *ctx, const void *data, size_t len)
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

static void boe_ft8057s_panel_init(struct boe_ft8057s *ctx)
{
	pr_info("disp: %s+\n", __func__);

	ocp2138_BiasPower_enable(15,15,5);
	ctx->reset_gpio = devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio)) {
		dev_err(ctx->dev, "%s: cannot get reset_gpio %ld\n",
			__func__, PTR_ERR(ctx->reset_gpio));
		//return;
	}
	else {
		gpiod_set_value(ctx->reset_gpio, 0);
		udelay(10 * 1000);
		gpiod_set_value(ctx->reset_gpio, 1);
		udelay(10 * 1000);
		gpiod_set_value(ctx->reset_gpio, 0);
		udelay(10 * 1000);
		gpiod_set_value(ctx->reset_gpio, 1);
		udelay(12 * 1000);
		devm_gpiod_put(ctx->dev, ctx->reset_gpio);
		pr_info("disp: %s reset_gpio\n", __func__);
	}

	boe_ft8057s_dcs_write_seq_static(ctx, 0x11);
	msleep(90);
	boe_ft8057s_dcs_write_seq_static(ctx, 0x29);
	boe_ft8057s_dcs_write_seq_static(ctx, 0x35,0x00);
	boe_ft8057s_dcs_write_seq_static(ctx, 0x51,0xFF,0x0F);
	boe_ft8057s_dcs_write_seq_static(ctx, 0x53,0x2C);
	boe_ft8057s_dcs_write_seq_static(ctx, 0x55,0x01);
	msleep(10);

	pr_info("%s-\n", __func__);
}

static int boe_ft8057s_disable(struct drm_panel *panel)
{
	struct boe_ft8057s *ctx = panel_to_boe_ft8057s(panel);

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

/*
static int boe_ft8057s_set_gesture_flag(int state)
{
	if(state == 1)
		tp_gesture_flag = 1;
	else
		tp_gesture_flag = 0;

	pr_info("%s:disp:set tp_gesture_flag:%d\n", __func__, tp_gesture_flag);
	return 0;
}
*/

static int boe_ft8057s_unprepare(struct drm_panel *panel)
{
	struct boe_ft8057s *ctx = panel_to_boe_ft8057s(panel);

	if (!ctx->prepared) {
		pr_info("%s, already unprepared, return\n", __func__);
		return 0;
	}

	pr_info("%s\n", __func__);

	boe_ft8057s_dcs_write_seq_static(ctx, 0x28);
	udelay(10 * 1000);
	boe_ft8057s_dcs_write_seq_static(ctx, 0x10);
	udelay(10 * 1000);


	pr_info("%s:disp: tp_gesture_flag:%d\n",__func__, tp_gesture_flag);
	if(!tp_gesture_flag) {
		ocp2138_BiasPower_disable(5);
	}

	ctx->error = 0;
	ctx->prepared = false;

	pr_info("%s -\n", __func__);

	return 0;
}

static int boe_ft8057s_prepare(struct drm_panel *panel)
{
	struct boe_ft8057s *ctx = panel_to_boe_ft8057s(panel);
	int ret;

	pr_info("disp: %s+\n", __func__);
	if (ctx->prepared) {
		pr_info("%s, already prepared, return\n", __func__);
		return 0;
	}

	boe_ft8057s_panel_init(ctx);
	ctx->cabc_mode = 0;

	ret = ctx->error;
	if (ret < 0) {
		pr_info("disp: %s error ret=%d\n", __func__, ret);
		boe_ft8057s_unprepare(panel);
	}

	ctx->prepared = true;
/*#if defined(CONFIG_MTK_PANEL_EXT)
	mtk_panel_tch_rst(panel);
#endif

#ifdef PANEL_SUPPORT_READBACK
	boe_ft8057s_panel_get_data(ctx);
#endif*/
	pr_info("disp: %s-\n", __func__);
	return ret;
}

static int boe_ft8057s_enable(struct drm_panel *panel)
{
	struct boe_ft8057s *ctx = panel_to_boe_ft8057s(panel);

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
	.clock		= 186451,
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
	.clock		= 186662,
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
	.clock		= 186826,
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
	.pll_clk = 627,
	//.data_rate = DATA_RATE,
	//.vfp_low_power = 880,
	.cust_esd_check = 1,
	.esd_check_enable = 1,
	.lcm_esd_check_table[0] = {
		.cmd = 0x0A,
		.count = 1,
		.para_list[0] = 0x9C,
	},

	.panel_ver = 1,
	//.panel_id = 0x15025892,
	.panel_name = "boe_ft8057s_vid_667_720",
	.panel_supplier = "boe",
	.lcm_index = 0,
	.max_bl_level = 2047,
	.hbm_type = HBM_MODE_RAMPING,
/*

	//.ssc_enable = 0,
	.lane_swap_en = 0,
	.lp_perline_en = 0,
	.physical_width_um = PHYSICAL_WIDTH,
	.physical_height_um = PHYSICAL_HEIGHT,
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
	.panel_ver = 1,
	//.panel_id = 0x15025892,
	.panel_name = "boe_ft8057s_vid_667_720",
	.panel_supplier = "boe",
	.lcm_index = 0,
	.max_bl_level = 2047,
	.hbm_type = HBM_MODE_RAMPING,
/*
	.ssc_enable = 0,
	.lane_swap_en = 0,
	.lp_perline_en = 0,
	.physical_width_um = PHYSICAL_WIDTH,
	.physical_height_um = PHYSICAL_HEIGHT,
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
	.panel_ver = 1,
	//.panel_id = 0x15025892,
	.panel_name = "boe_ft8057s_vid_667_720",
	.panel_supplier = "boe",
	.lcm_index = 0,
	.max_bl_level = 2047,
	.hbm_type = HBM_MODE_RAMPING,
/*
	.ssc_enable = 0,
	.lane_swap_en = 0,
	.lp_perline_en = 0,
	.physical_width_um = PHYSICAL_WIDTH,
	.physical_height_um = PHYSICAL_HEIGHT,
	.output_mode = MTK_PANEL_DSC_SINGLE_PORT,
	.lfr_enable = LFR_EN,
	.lfr_minimum_fps = MODE_60_FPS,
*/

};

static int boe_ft8057s_setbacklight_cmdq(void *dsi, dcs_write_gce cb, void *handle,
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
	struct boe_ft8057s *ctx = panel_to_boe_ft8057s(panel);

	pr_info("%s+ \n", __func__);

	ctx->reset_gpio =
		devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
	gpiod_set_value(ctx->reset_gpio, on);
	devm_gpiod_put(ctx->dev, ctx->reset_gpio);

	return 0;
}

static enum mtk_lcm_version ft8057s_get_lcm_version(void)
{
	return MTK_LEGACY_LCM_DRV_WITH_BACKLIGHTCLASS;
}

static int panel_cabc_set_cmdq(struct boe_ft8057s *ctx, void *dsi, dcs_grp_write_gce cb, void *handle, uint32_t cabc_mode)
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
	struct boe_ft8057s *ctx = panel_to_boe_ft8057s(panel);
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
	.reset = panel_ext_reset,
	.set_backlight_cmdq = boe_ft8057s_setbacklight_cmdq,
	.ext_param_set = mtk_panel_ext_param_set,
	.get_lcm_version = ft8057s_get_lcm_version,
//	.ata_check = panel_ata_check,
//	.set_gesture_flag = boe_ft8057s_set_gesture_flag,
	.panel_feature_set = panel_feature_set,
};
#endif

static int boe_ft8057s_get_modes(struct drm_panel *panel,
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

static const struct drm_panel_funcs boe_ft8057s_drm_funcs = {
	.disable = boe_ft8057s_disable,
	.unprepare = boe_ft8057s_unprepare,
	.prepare = boe_ft8057s_prepare,
	.enable = boe_ft8057s_enable,
	.get_modes = boe_ft8057s_get_modes,
};

static int boe_ft8057s_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct device_node *dsi_node, *remote_node = NULL, *endpoint = NULL;
	struct boe_ft8057s *ctx;
	struct device_node *backlight;
	int ret;

	pr_info("%s+ disp:boe,ft8057s,vdo,120hz\n", __func__);

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

	ctx = devm_kzalloc(dev, sizeof(struct boe_ft8057s), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	mipi_dsi_set_drvdata(dsi, ctx);

	ctx->dev = dev;
	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_SYNC_PULSE;
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
	drm_panel_init(&ctx->panel, dev, &boe_ft8057s_drm_funcs, DRM_MODE_CONNECTOR_DSI);

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

	pr_info("[%d  %s]- boe,ft8057s,vdo,120hz ret:%d\n", __LINE__, __func__,ret);

	return ret;
}

static int boe_ft8057s_remove(struct mipi_dsi_device *dsi)
{
	struct boe_ft8057s *ctx = mipi_dsi_get_drvdata(dsi);
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

static const struct of_device_id boe_ft8057s_of_match[] = {
	{
		.compatible = "boe,ft8057s,vdo,120hz",
	},
	{}
};

MODULE_DEVICE_TABLE(of, boe_ft8057s_of_match);

static struct mipi_dsi_driver boe_ft8057s_driver = {
	.probe = boe_ft8057s_probe,
	.remove = boe_ft8057s_remove,
	.driver = {
		.name = "boe_ft8057s_vid_667_720",
		.owner = THIS_MODULE,
		.of_match_table = boe_ft8057s_of_match,
	},
};

module_mipi_dsi_driver(boe_ft8057s_driver);

MODULE_AUTHOR("mediatek");
MODULE_DESCRIPTION("boe ft8057s incell 120hz Panel Driver");
MODULE_LICENSE("GPL v2");

