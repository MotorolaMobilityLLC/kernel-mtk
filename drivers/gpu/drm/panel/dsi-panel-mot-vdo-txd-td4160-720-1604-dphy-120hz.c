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
#include "include/dsi-panel-mot-vdo-txd-td4160-720-1604-dphy-120hz.h"
#endif

#define PANEL_EVT 1
#define PANEL_DVT1 2
#define PANEL_DVT2 3
#define PANEL_PVT 4
/* option function to read data from some panel address */
/* #define PANEL_SUPPORT_READBACK */

#define BIAS_OCP2138
#ifdef BIAS_OCP2138
extern int __attribute__ ((weak)) ocp2138_BiasPower_disable(u32 pwrdown_delay);
extern int __attribute__ ((weak)) ocp2138_BiasPower_enable(u32 avdd, u32 avee,u32 pwrup_delay);
#endif

extern int mtkfb_esd_get_recovery_flag(void);
static BLOCKING_NOTIFIER_HEAD(panel_gesture_notifier_list);

static int tp_gesture_flag = 0;

struct txd_td4160 {
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
	{7, {0xB9, 0x35, 0x39, 0x00, 0xbe, 0x00, 0x00}},
	{31, {0xCE, 0x77, 0x52, 0x66, 0x74, 0x7D, 0x82, 0x87, 0x8E, 0x98, 0x9F, 0xB7, 0xD2, 0xEA, 0xF0, 0xF4, 0xF9, 0xFC, 0xFF, 0x00, 0x4B, 0x04, 0x04, 0x00, 0x04, 0x04, 0x62, 0x43, 0x69, 0x5A, 0x73}},
	{2, {0x55, 0x02}},
};

static struct mtk_panel_para_table panel_cabc_mv[] = {
	{7, {0xBA, 0x85, 0x55, 0x00, 0xbe, 0x00, 0x00}},
	{31, {0xCE, 0x77, 0x52, 0x66, 0x74, 0x7D, 0x82, 0x87, 0x8E, 0x98, 0x9F, 0xB7, 0xD2, 0xEA, 0xF0, 0xF4, 0xF9, 0xFC, 0xFF, 0x00, 0x4B, 0x04, 0x04, 0x00, 0x04, 0x04, 0x62, 0x43, 0x69, 0x5A, 0x73}},
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

#define txd_td4160_dcs_write_seq(ctx, seq...)                                         \
	({                                                                     \
		const u8 d[] = { seq };                                        \
		BUILD_BUG_ON_MSG(ARRAY_SIZE(d) > 64,                           \
				 "DCS sequence too big for stack");            \
		txd_td4160_dcs_write(ctx, d, ARRAY_SIZE(d));                          \
	})

#define txd_td4160_dcs_write_seq_static(ctx, seq...)                                  \
	({                                                                     \
		static const u8 d[] = { seq };                                 \
		txd_td4160_dcs_write(ctx, d, ARRAY_SIZE(d));                          \
	})

static inline struct txd_td4160 *panel_to_txd_td4160(struct drm_panel *panel)
{
	return container_of(panel, struct txd_td4160, panel);
}

#ifdef PANEL_SUPPORT_READBACK
static int txd_td4160_dcs_read(struct txd_td4160 *ctx, u8 cmd, void *data, size_t len)
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

static void txd_td4160_panel_get_data(struct txd_td4160 *ctx)
{
	u8 buffer[3] = {0};
	static int ret;

	if (ret == 0) {
		ret = txd_td4160_dcs_read(ctx, 0x0A, buffer, 1);
		pr_info("disp: %s 0x%08x\n", __func__, buffer[0] | (buffer[1] << 8));
		dev_info(ctx->dev, "return %d data(0x%08x) to dsi engine\n",
			 ret, buffer[0] | (buffer[1] << 8));
	}
}
#endif

static void txd_td4160_dcs_write(struct txd_td4160 *ctx, const void *data, size_t len)
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

static void txd_panel_tp_reset(struct txd_td4160 *ctx)
{
	pr_info("%s:txd_td4160: +\n", __func__);

	ctx->tp_reset_gpio = devm_gpiod_get(ctx->dev, "tp_reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->tp_reset_gpio)) {
		dev_err(ctx->dev, "%s:txd_td4160: cannot get tp_reset_gpio %ld\n",
			__func__, PTR_ERR(ctx->tp_reset_gpio));
		//return;
	}
	else {
		gpiod_set_value(ctx->tp_reset_gpio, 1);
		udelay(3 * 1000);
		devm_gpiod_put(ctx->dev, ctx->tp_reset_gpio);
		pr_info("%s:txd_td4160: tp_reset_gpio 1\n", __func__);
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

static void lcm_parse_panel_version(struct txd_td4160 *ctx)
{
	int rc;
	struct device_node *chosen = of_find_node_by_name(NULL, "chosen");

	ctx->version = PANEL_PVT;
	if(chosen) {
		u32 tmp = 0;

		rc = of_property_read_u32(chosen, "mmi,panel_ver", &tmp);
		if (!rc) {
			if (PANEL_EVT == tmp) {
				ctx->version = PANEL_EVT;
			} else if(PANEL_DVT1 == tmp) {
				ctx->version = PANEL_DVT1;
			} else if(PANEL_DVT2 == tmp) {
				ctx->version = PANEL_DVT2;
			} else if(PANEL_PVT == tmp) {
				ctx->version = PANEL_PVT;
			} else {
				ctx->version = PANEL_PVT;
			}
			pr_info("get panel_ver:%d\n", ctx->version);
		}
		else
			pr_info("mmi,panel_ver not get\n");
	}
	else
		pr_info("parse_panel chosen node null\n");

	return;
}

static void txd_td4160_panel_init(struct txd_td4160 *ctx)
{
	ktime_t now;
	pr_info("disp: %s+\n", __func__);
#ifdef BIAS_OCP2138
		ocp2138_BiasPower_enable(20,20,5);
#endif
	txd_panel_tp_reset(ctx);

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

	//Adjust GIP timing, STV3/v relative position
	txd_td4160_dcs_write_seq_static(ctx, 0xB0, 0x84);
	txd_td4160_dcs_write_seq_static(ctx, 0xD6, 0x00);
	txd_td4160_dcs_write_seq_static(ctx, 0xF0, 0xC1, 0x01, 0x31);
	if(ctx->version == PANEL_EVT) {
		//BIST mode 120HZ settings(C2H,DEH)
		txd_td4160_dcs_write_seq_static(ctx, 0xF0, 0xC2, 0x0F, 0x10);
		txd_td4160_dcs_write_seq_static(ctx, 0xF0, 0xDE, 0x0F, 0x3C);
	}
	// setting brightness (DCS 0x51) to 0x0000 is intended to prevent a screen flash during boot-up.
	txd_td4160_dcs_write_seq_static(ctx, 0x51, 0x00,0x00);
	txd_td4160_dcs_write_seq_static(ctx, 0x53, 0x2C);
	txd_td4160_dcs_write_seq_static(ctx, 0x55, 0x00);
	txd_td4160_dcs_write_seq_static(ctx, 0x35, 0x00);

	//Sleep Out
	txd_td4160_dcs_write_seq_static(ctx, 0x11);
	//Display On
	txd_td4160_dcs_write_seq_static(ctx, 0x29);
	usleep_range(80*1000, 81*1000);

	now = ktime_get();
	ctx->screen_on_timestamp = ktime_to_ms(now);
	pr_info("disp:%s -screen on timestamp: %lld \n", __func__, ctx->screen_on_timestamp);

	msleep(10);

	pr_info("%s-\n", __func__);
}

static int txd_td4160_disable(struct drm_panel *panel)
{
	struct txd_td4160 *ctx = panel_to_txd_td4160(panel);

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

static int txd_td4160_set_gesture_flag(int state)
{
	if(state == 1)
		tp_gesture_flag = 1;
	else
		tp_gesture_flag = 0;

	pr_info("%s:disp:set tp_gesture_flag:%d\n", __func__, tp_gesture_flag);
	return 0;
}

static int txd_td4160_unprepare(struct drm_panel *panel)
{
	struct txd_td4160 *ctx = panel_to_txd_td4160(panel);
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

	txd_td4160_dcs_write_seq_static(ctx, 0x28);
	udelay(10 * 1000);
	txd_td4160_dcs_write_seq_static(ctx, 0x10);
	// Increase delay to 150ms to meet panel vendor spec for clean power-down
	msleep(150);

	if(tp_gesture_flag)
		panel_gesture_notifier_call_chain(0x01,NULL);

	if(!tp_gesture_flag){
		ctx->reset_gpio = devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
		if (IS_ERR(ctx->reset_gpio)) {
			dev_err(ctx->dev, "%s:txd_td4160: cannot get reset_gpio %ld\n",
			__func__, PTR_ERR(ctx->reset_gpio));
			return PTR_ERR(ctx->reset_gpio);
		}
		gpiod_set_value(ctx->reset_gpio, 0);
		devm_gpiod_put(ctx->dev, ctx->reset_gpio);
		usleep_range(5000,5001);
		pr_info("%s:txd_td4160: reset_gpio 0\n", __func__);
	}
	pr_info("%s:disp: tp_gesture_flag:%d, esd_recovery_flg=%d \n",__func__, tp_gesture_flag, mtkfb_esd_get_recovery_flag());
	if(!tp_gesture_flag || mtkfb_esd_get_recovery_flag()) {
#ifdef BIAS_OCP2138
		ocp2138_BiasPower_disable(5);
#endif
	}
	if(!tp_gesture_flag){
		msleep(5);
		ctx->tp_reset_gpio = devm_gpiod_get(ctx->dev, "tp_reset", GPIOD_OUT_HIGH);
		if (IS_ERR(ctx->tp_reset_gpio)) {
			dev_err(ctx->dev, "%s:txd_td4160: cannot get tp_reset_gpio %ld\n",
				__func__, PTR_ERR(ctx->tp_reset_gpio));
			//return PTR_ERR(ctx->tp_reset_gpio);
		}
		else{
			gpiod_set_value(ctx->tp_reset_gpio, 0);
			devm_gpiod_put(ctx->dev, ctx->tp_reset_gpio);
			usleep_range(5000,5001);
			pr_info("%s:txd_td4160: tp_reset_gpio 0\n", __func__);
		}
	}

	ctx->error = 0;
	ctx->prepared = false;

	pr_info("%s -\n", __func__);

	return 0;
}

static int txd_td4160_prepare(struct drm_panel *panel)
{
	struct txd_td4160 *ctx = panel_to_txd_td4160(panel);
	int ret;

	pr_info("disp: %s+\n", __func__);
	if (ctx->prepared) {
		pr_info("%s, already prepared, return\n", __func__);
		return 0;
	}

	txd_td4160_panel_init(ctx);
//	ctx->hbm_mode = 0;
	ctx->cabc_mode = 0;

	ret = ctx->error;
	if (ret < 0) {
		pr_info("disp: %s error ret=%d\n", __func__, ret);
		txd_td4160_unprepare(panel);
	}

	ctx->prepared = true;

#ifdef PANEL_SUPPORT_READBACK
	txd_td4160_panel_get_data(ctx);
#endif
	pr_info("disp: %s-\n", __func__);
	return ret;
}

static int txd_td4160_enable(struct drm_panel *panel)
{
	struct txd_td4160 *ctx = panel_to_txd_td4160(panel);

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
		.panel_cellid_reg = 0x00,
		.panel_cellid_reg_seq = 1,
		.panel_cellid_len = 23,
		.panel_cellid_read_max = 4,

	},
	.dsc_params = {
		.enable                =  DSC_ENABLE,
	},
	.panel_ver = 1,
	//.panel_id = 0x01012891,
	.panel_name = "txd_td4160_vid_667_720_120hz",
	.panel_supplier = "txd",
	.lcm_index = 1,
	.max_bl_level = 2047,
	.hbm_type = HBM_MODE_RAMPING,
	.physical_width_um = PHYSICAL_WIDTH,
	.physical_height_um = PHYSICAL_HEIGHT,
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
		.panel_cellid_reg = 0xA1,
		.panel_cellid_reg_seq = 1,
		.panel_cellid_len = 23,
		.panel_cellid_read_max = 4,
	},
	.dsc_params = {
		.enable                =  DSC_ENABLE,
	},
	.panel_ver = 1,
	//.panel_id = 0x01012891,
	.panel_name = "txd_td4160_vid_667_720_120hz",
	.panel_supplier = "txd",
	.lcm_index = 1,
	.max_bl_level = 2047,
	.hbm_type = HBM_MODE_RAMPING,
	.physical_width_um = PHYSICAL_WIDTH,
	.physical_height_um = PHYSICAL_HEIGHT,
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
		.panel_cellid_reg = 0xA1,
		.panel_cellid_reg_seq = 1,
		.panel_cellid_len = 23,
		.panel_cellid_read_max = 4,
	},
	.dsc_params = {
		.enable                =  DSC_ENABLE,
	},
	.panel_ver = 1,
	//.panel_id = 0x01012891,
	.panel_name = "txd_td4160_vid_667_720_120hz",
	.panel_supplier = "txd",
	.lcm_index = 1,
	.max_bl_level = 2047,
	.hbm_type = HBM_MODE_RAMPING,
	.physical_width_um = PHYSICAL_WIDTH,
	.physical_height_um = PHYSICAL_HEIGHT,
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

static int txd_td4160_setbacklight_cmdq(void *dsi, dcs_write_gce cb, void *handle,
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
	struct txd_td4160 *ctx = panel_to_txd_td4160(panel);

	pr_info("%s+ \n", __func__);

	ctx->reset_gpio =
		devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
	gpiod_set_value(ctx->reset_gpio, on);
	devm_gpiod_put(ctx->dev, ctx->reset_gpio);

	return 0;
}

static int panel_cabc_set_cmdq(struct txd_td4160 *ctx, void *dsi, dcs_grp_write_gce cb, void *handle, uint32_t cabc_mode)
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
	struct txd_td4160 *ctx = panel_to_txd_td4160(panel);
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
#if 0
			if (ctx->hbm_mode != param_info.value) {
				ctx->hbm_mode = param_info.value;
				panel_hbm_set_cmdq(ctx, dsi, cb, handle, param_info.value);
				pr_debug("%s: set HBM to %d end\n", __func__, param_info.value);
				ret = 0;
			}
			else
				pr_info("%s: skip same HBM mode:%d\n", __func__, ctx->hbm_mode);
#endif
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
	.set_backlight_cmdq = txd_td4160_setbacklight_cmdq,
	.ext_param_set = mtk_panel_ext_param_set,
	.set_gesture_flag = txd_td4160_set_gesture_flag,
	.panel_feature_set = panel_feature_set,
};
#endif

static int txd_td4160_get_modes(struct drm_panel *panel,
					struct drm_connector *connector)
{
	struct drm_display_mode *mode_1;

	struct drm_display_mode *mode_2;
	struct drm_display_mode *mode_3;


	mode_1 = drm_mode_duplicate(connector->dev, &performance_mode_60hz);
	pr_info("[%d  %s]disp mode:%d\n",__LINE__, __FUNCTION__,mode_1);
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
	pr_info("[%d  %s]end\n",__LINE__, __FUNCTION__);

	return 1;
}

static const struct drm_panel_funcs txd_td4160_drm_funcs = {
	.disable = txd_td4160_disable,
	.unprepare = txd_td4160_unprepare,
	.prepare = txd_td4160_prepare,
	.enable = txd_td4160_enable,
	.get_modes = txd_td4160_get_modes,
};

static int txd_td4160_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct device_node *dsi_node, *remote_node = NULL, *endpoint = NULL;
	struct txd_td4160 *ctx;
	struct device_node *backlight;
	int ret;

	pr_info("%s+ disp:txd,td4160,vdo,120hz\n", __func__);

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

	ctx = devm_kzalloc(dev, sizeof(struct txd_td4160), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	mipi_dsi_set_drvdata(dsi, ctx);

	ctx->dev = dev;
	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_SYNC_PULSE |  MIPI_DSI_MODE_VIDEO_BURST;

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
	drm_panel_init(&ctx->panel, dev, &txd_td4160_drm_funcs, DRM_MODE_CONNECTOR_DSI);

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

	pr_info("[%d  %s]- txd,td4160,vdo,120hz ret:%d\n", __LINE__, __func__,ret);

	return ret;
}

static int txd_td4160_remove(struct mipi_dsi_device *dsi)
{
	struct txd_td4160 *ctx = mipi_dsi_get_drvdata(dsi);
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

static const struct of_device_id txd_td4160_of_match[] = {
	{
#if defined(CONFIG_DRM_PANEL_NUM_NO_LIMIT)
		.compatible = "txd_td4160_667",
#else
		.compatible = "txd,td4160,667",
#endif
	},
	{}
};

MODULE_DEVICE_TABLE(of, txd_td4160_of_match);

static struct mipi_dsi_driver txd_td4160_driver = {
	.probe = txd_td4160_probe,
	.remove = txd_td4160_remove,
	.shutdown = lcm_shutdown,
	.driver = {
		.name = "txd_td4160_667",
		.owner = THIS_MODULE,
		.of_match_table = txd_td4160_of_match,
	},
};

module_mipi_dsi_driver(txd_td4160_driver);

MODULE_AUTHOR("Motorola Mobility LLC");
MODULE_DESCRIPTION("txd td4160 incell 120hz Panel Driver");
MODULE_LICENSE("GPL v2");

