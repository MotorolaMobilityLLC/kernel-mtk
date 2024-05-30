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
#include <linux/fb.h>

#define CONFIG_MTK_PANEL_EXT
#if defined(CONFIG_MTK_PANEL_EXT)
#include "../mediatek/mediatek_v2/mtk_panel_ext.h"
#include "../mediatek/mediatek_v2/mtk_drm_graphics_base.h"
#include "include/dsi_panel_mot_mipi_vid_cost_nt36672c_653_hdp.h"
#endif

/* option function to read data from some panel address */
/* #define PANEL_SUPPORT_READBACK */
static int tp_gesture_flag=0;
struct csot {
	struct device *dev;
	struct drm_panel panel;
	struct backlight_device *backlight;
	struct gpio_desc *pm_enable_gpio;
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
	{2, {0xFF, 0x10}},
	{2, {0xFB, 0x01}},
	{2, {0x55, 0x01}},
};

static struct mtk_panel_para_table panel_cabc_mv[] = {
	{2, {0xFF, 0x10}},
	{2, {0xFB, 0x01}},
	{2, {0x55, 0x03}},
};

static struct mtk_panel_para_table panel_cabc_disable[] = {
	{2, {0xFF, 0x10}},
	{2, {0xFB, 0x01}},
	{2, {0x55, 0x00}},
};

static struct mtk_panel_para_table panel_hbm_on[] = {
	{2, {0xFF, 0x10}},
	{2, {0xFB, 0x01}},
	{3, {0x51, 0x07, 0xFF}},
};

//DVT PVT
static struct mtk_panel_para_table panel_hbm_off[] = {
	{2, {0xFF, 0x10}},
	{2, {0xFB, 0x01}},
	{3, {0x51, 0x05, 0xB0}},
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
	
	ctx->bias_n_gpio = devm_gpiod_get(ctx->dev, "bias_n", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->bias_n_gpio)) {
		dev_err(ctx->dev, "%s: cannot get bias_n_gpio %ld\n",
			__func__, PTR_ERR(ctx->bias_n_gpio));
		return;
	}

	ctx->bias_p_gpio = devm_gpiod_get(ctx->dev, "bias_p", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->bias_p_gpio)) {
		dev_err(ctx->dev, "%s: cannot get bias_p_gpio %ld\n",
			__func__, PTR_ERR(ctx->bias_p_gpio));
		return;
	}
//+EKCEBU-680,pengzhenhua.wt,modify,20220618, modify to lcd 120 fps
	gpiod_set_value(ctx->bias_p_gpio, 1);
	msleep(5);
	gpiod_set_value(ctx->bias_n_gpio, 1);
	msleep(5);
//+EKCEBU-680,pengzhenhua.wt,modify,20220618, modify to lcd 120 fps
	gpiod_set_value(ctx->reset_gpio, 1);
	msleep(10);
	gpiod_set_value(ctx->reset_gpio, 0);
	msleep(5);
	gpiod_set_value(ctx->reset_gpio, 1);
	msleep(60);

	devm_gpiod_put(ctx->dev, ctx->reset_gpio);
	devm_gpiod_put(ctx->dev, ctx->bias_n_gpio);
	devm_gpiod_put(ctx->dev, ctx->bias_p_gpio);
	printk("[%d  %s]hxl_check_bias !!\n",__LINE__, __FUNCTION__);

	csot_dcs_write_seq_static(ctx, 0XFF, 0XE0),
	csot_dcs_write_seq_static(ctx, 0XFB, 0X01),
	csot_dcs_write_seq_static(ctx, 0X35, 0X82),
	csot_dcs_write_seq_static(ctx, 0XFF, 0XF0),
	csot_dcs_write_seq_static(ctx, 0XFB, 0X01),
	csot_dcs_write_seq_static(ctx, 0X1C, 0X01),
	csot_dcs_write_seq_static(ctx, 0X33, 0X01),
	csot_dcs_write_seq_static(ctx, 0X5A, 0X00),
	csot_dcs_write_seq_static(ctx, 0XFF, 0XD0),
	csot_dcs_write_seq_static(ctx, 0XFB, 0X01),
	csot_dcs_write_seq_static(ctx, 0X53, 0X22),
	csot_dcs_write_seq_static(ctx, 0X54, 0X02),
	csot_dcs_write_seq_static(ctx, 0XFF, 0XC0),
	csot_dcs_write_seq_static(ctx, 0XFB, 0X01),
	csot_dcs_write_seq_static(ctx, 0X9C, 0X11),
	csot_dcs_write_seq_static(ctx, 0X9D, 0X11),
	csot_dcs_write_seq_static(ctx, 0XFF, 0X2B),
	csot_dcs_write_seq_static(ctx, 0XFB, 0X01),
	csot_dcs_write_seq_static(ctx, 0XB7, 0X2A),
	csot_dcs_write_seq_static(ctx, 0XB8, 0X0D),
	csot_dcs_write_seq_static(ctx, 0XC0, 0X01),
	csot_dcs_write_seq_static(ctx, 0xFF, 0xF0),
	csot_dcs_write_seq_static(ctx, 0xFB, 0x01),
	csot_dcs_write_seq_static(ctx, 0xD2, 0x50),
	csot_dcs_write_seq_static(ctx, 0XFF, 0X10),
	csot_dcs_write_seq_static(ctx, 0XFB, 0X01),
	csot_dcs_write_seq_static(ctx, 0X35, 0X00),
	csot_dcs_write_seq_static(ctx, 0x51, 0x07,0xFF);
	csot_dcs_write_seq_static(ctx, 0x53, 0x2C),
	csot_dcs_write_seq_static(ctx, 0x55, 0x01),
	csot_dcs_write_seq_static(ctx, 0x68, 0x00,0x01),
	csot_dcs_write_seq_static(ctx, 0XB0, 0X00),
	csot_dcs_write_seq_static(ctx, 0XC0, 0X00),
	csot_dcs_write_seq_static(ctx, 0XC2, 0X1B,0XA0),
	csot_dcs_write_seq_static(ctx, 0x11, 0x00);
	msleep(120);
	csot_dcs_write_seq_static(ctx, 0x29, 0x00);
	msleep(20);

}

static int csot_disable(struct drm_panel *panel)
{
	struct csot *ctx = panel_to_csot(panel);
	
	printk("[%d  %s]hxl_check_dsi_pcl_data_rate !!\n",__LINE__, __FUNCTION__);

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
	pr_info("%s, tp_gesture_flag = %d\n", __func__, tp_gesture_flag);

	return 0;
}

static int csot_unprepare(struct drm_panel *panel)
{
	struct csot *ctx = panel_to_csot(panel);

	if (!ctx->prepared)
		return 0;
	pr_info("%s\n", __func__);
	printk("[%d  %s]hxl_check_dsi !!\n",__LINE__, __FUNCTION__);

	csot_dcs_write_seq_static(ctx, 0x28);
	csot_dcs_write_seq_static(ctx, 0x10);
	msleep(120);

	if (tp_gesture_flag == 0)
	{
		ctx->bias_n_gpio = devm_gpiod_get(ctx->dev, "bias_n", GPIOD_OUT_HIGH);
		if (IS_ERR(ctx->bias_n_gpio)) {
			dev_err(ctx->dev, "%s: cannot get bias_n_gpio %ld\n",
				__func__, PTR_ERR(ctx->bias_n_gpio));
			return -1;
		}

		ctx->bias_p_gpio = devm_gpiod_get(ctx->dev, "bias_p", GPIOD_OUT_HIGH);
		if (IS_ERR(ctx->bias_p_gpio)) {
			dev_err(ctx->dev, "%s: cannot get bias_p_gpio %ld\n",
				__func__, PTR_ERR(ctx->bias_p_gpio));
			return -1;
		}
		gpiod_set_value(ctx->bias_n_gpio, 0);
		udelay(10 * 1000);
		gpiod_set_value(ctx->bias_p_gpio, 0);
		udelay(10 * 1000);
		devm_gpiod_put(ctx->dev, ctx->bias_n_gpio);
		devm_gpiod_put(ctx->dev, ctx->bias_p_gpio);
		printk("[%d  %s]hxl_check_bias !!\n",__LINE__, __FUNCTION__);
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
	printk("[%d  %s]hxl_check_dsi !!\n",__LINE__, __FUNCTION__);
	if (ctx->prepared)
		return 0;

	//lcm_power_enable();
	csot_panel_init(ctx);

	ret = ctx->error;
	printk("[%d  %s]hxl_check_dsi   ret:%d !!\n",__LINE__, __FUNCTION__,ret);
	if (ret < 0)
		csot_unprepare(panel);

	ctx->prepared = true;
/*#if defined(CONFIG_MTK_PANEL_EXT)
	mtk_panel_tch_rst(panel);
#endif

#ifdef PANEL_SUPPORT_READBACK
	csot_panel_get_data(ctx);
#endif*/
printk("[%d  %s]hxl_check_dsi  ret:%d !!\n",__LINE__, __FUNCTION__,ret);

	return ret;
}

static int csot_enable(struct drm_panel *panel)
{
	struct csot *ctx = panel_to_csot(panel);
	
	printk("[%d  %s]hxl_check_dsi !!\n",__LINE__, __FUNCTION__);

	if (ctx->enabled)
		return 0;

	if (ctx->backlight) {
		ctx->backlight->props.power = FB_BLANK_UNBLANK;
		backlight_update_status(ctx->backlight);
	}

	ctx->enabled = true;

	return 0;
}
//+EKCEBU-680,pengzhenhua.wt,modify,20220618, modify to lcd 120 fps
static const struct drm_display_mode default_mode = {
	.clock		= 126498,
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
	.clock		= 126498,
	.hdisplay = FRAME_WIDTH,
	.hsync_start = FRAME_WIDTH + MODE_1_HFP,
	.hsync_end = FRAME_WIDTH + MODE_1_HFP + HSA,
	.htotal = FRAME_WIDTH + MODE_1_HFP + HSA + HBP,
	.vdisplay = FRAME_HEIGHT,
	.vsync_start = FRAME_HEIGHT + MODE_1_VFP,
	.vsync_end = FRAME_HEIGHT + MODE_1_VFP + VSA,
	.vtotal = FRAME_HEIGHT + MODE_1_VFP + VSA + VBP,
};

#if defined(CONFIG_MTK_PANEL_EXT)

//60HZ
static struct mtk_panel_params ext_params = {
	//.pll_clk = 600,
	.data_rate = 818,
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
};

//90HZ
static struct mtk_panel_params ext_params_mode_1 = {
	.data_rate = 818,
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
};

static int csot_setbacklight_cmdq(void *dsi, dcs_write_gce cb,
	void *handle, unsigned int level)
{
	char bl_tb0[] = {0x51, 0x7, 0xff};
	//pr_info("%s: skip for using bl ic, level=%d\n", __func__, level);

	if (!cb) {
		pr_info("%s cb NULL!\n", __func__);
		return -1;
	}
	//for 11bit
	//level = (2047 * level)/255;
	bl_tb0[1] = ((level & 0x700) >> 8);
	bl_tb0[2]  = (level & 0xFF);

	pr_info("%s set level:%d, bl_tb:0x%02x%02x\n", __func__, level, bl_tb0[1], bl_tb0[2]);
	cb(dsi, handle, bl_tb0, ARRAY_SIZE(bl_tb0));

	return 0;
}
//+EKCEBU-680,pengzhenhua.wt,modify,20220618, modify to lcd 120 fps
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

	if (drm_mode_vrefresh(m) == MODE_0_FPS)
		ext->params = &ext_params;
	else if (drm_mode_vrefresh(m) == MODE_1_FPS)
		ext->params = &ext_params_mode_1;
	else
		ret = 1;
	return ret;
}
//-EKCEBU-680,pengzhenhua.wt,modify,20220618, modify to lcd 120 fps
static int panel_ext_reset(struct drm_panel *panel, int on)
{
	struct csot *ctx = panel_to_csot(panel);

	ctx->reset_gpio =
		devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
	gpiod_set_value(ctx->reset_gpio, on);
	devm_gpiod_put(ctx->dev, ctx->reset_gpio);

	return 0;
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
	.set_gesture_flag = csot_set_gesture_flag,
	.ext_param_set = mtk_panel_ext_param_set,	//EKCEBU-680,pengzhenhua.wt,modify,20220618, modify to lcd 120 fps
//	.ata_check = panel_ata_check,
	.panel_feature_set = panel_feature_set,
};
#endif

static int csot_get_modes(struct drm_panel *panel,
						struct drm_connector *connector)
{
	struct drm_display_mode *mode;
	struct drm_display_mode *mode_1;	//EKCEBU-680,pengzhenhua.wt,modify,20220618, modify to lcd 120 fps

	mode = drm_mode_duplicate(connector->dev, &default_mode);
	printk("[%d  %s]hxl_check_dsi_modes  mode:%d  !!\n",__LINE__, __FUNCTION__,mode);
	if (!mode) {
		dev_err(connector->dev->dev, "failed to add mode %ux%ux@%u\n",
			default_mode.hdisplay, default_mode.vdisplay,
			drm_mode_vrefresh(&default_mode));
		return -ENOMEM;
	}
	drm_mode_set_name(mode);
	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	drm_mode_probed_add(connector, mode);
//+EKCEBU-680,pengzhenhua.wt,modify,20220618, modify to lcd 120 fps
	mode_1 = drm_mode_duplicate(connector->dev, &performance_mode_1);
	printk("[%d  %s]ww_check_dsi_modes  mode:%d  !!\n",__LINE__, __FUNCTION__,mode_1);
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

	connector->display_info.width_mm = 68;
	connector->display_info.height_mm = 151;
	printk("[%d  %s]ww_check_dsi_modes !!\n",__LINE__, __FUNCTION__);

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

	pr_info("%s+ csot,nt36672,incell,90hz,hdplus1600\n", __func__);
	printk("[%s %d],hxl_check_lcd Enter!! \n", __func__, __LINE__);

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

	pr_info("[%d  %s]- csot,nt36672c,vdo,120hz  ret:%d  \n", __LINE__, __func__,ret);

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
		.compatible = "csot,nt36672,incell,90hz,hdplus1600",
	},
	{} };

MODULE_DEVICE_TABLE(of, csot_of_match);

static struct mipi_dsi_driver csot_driver = {
	.probe = csot_probe,
	.remove = csot_remove,
	.driver = {
		.name = "mipi_mot_vid_csot_nt36672c_hdp_653",
		.owner = THIS_MODULE,
		.of_match_table = csot_of_match,
	},
};

module_mipi_dsi_driver(csot_driver);

MODULE_AUTHOR("Jingjing Liu <jingjing.liu@mediatek.com>");
MODULE_DESCRIPTION("csot nt36672c incell 120hz Panel Driver");
MODULE_LICENSE("GPL v2");
