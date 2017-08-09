/*
 * Copyright (c) 2015 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <drm/drmP.h>
#include <drm/drm_gem.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_panel.h>
#include <linux/clk.h>
#include <linux/component.h>
#include <linux/debugfs.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/reset.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/of_graph.h>
#include <linux/regulator/consumer.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <video/mipi_display.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <video/videomode.h>

#include "mtk_drm_drv.h"
#include "mtk_drm_crtc.h"
#include "mtk_drm_ddp.h"
#include "mtk_drm_gem.h"
#include "mtk_dsi.h"
#include "mtk_mipi_tx.h"

#define DSI_VIDEO_FIFO_DEPTH	(1920 / 4)
#define DSI_HOST_FIFO_DEPTH	64

#define DSI_START		0x00
#define SLEEPOUT_START		BIT(2)
#define VM_CMD_START		BIT(16)

#define DSI_INTEN		0x08
#define DSI_INTSTA		0x0c

#define DSI_CON_CTRL		0x10
#define DSI_RESET		BIT(0)
#define DSI_EN			BIT(1)

#define DSI_MODE_CTRL		0x14
#define MODE			(3)
#define CMD_MODE		0
#define SYNC_PULSE_MODE		1
#define SYNC_EVENT_MODE		2
#define BURST_MODE		3
#define FRM_MODE		BIT(16)
#define MIX_MODE		BIT(17)
#define V2C_SWITCH_ON		BIT(18)
#define C2V_SWITCH_ON		BIT(19)
#define SLEEP_MODE		BIT(20)

#define DSI_TXRX_CTRL		0x18
#define VC_NUM			(2 << 0)
#define LANE_NUM		(0xf << 2)
#define DIS_EOT			BIT(6)
#define NULL_EN			BIT(7)
#define TE_FREERUN		BIT(8)
#define EXT_TE_EN		BIT(9)
#define EXT_TE_EDGE		BIT(10)
#define MAX_RTN_SIZE		(0xf << 12)
#define HSTX_CKLP_EN		BIT(16)

#define DSI_PSCTRL		0x1c
#define DSI_PS_WC		0x3fff
#define DSI_PS_SEL		(3 << 16)
#define PACKED_PS_16BIT_RGB565	(0 << 16)
#define LOOSELY_PS_18BIT_RGB666	(1 << 16)
#define PACKED_PS_18BIT_RGB666	(2 << 16)
#define PACKED_PS_24BIT_RGB888	(3 << 16)

#define DSI_VSA_NL		0x20
#define DSI_VBP_NL		0x24
#define DSI_VFP_NL		0x28
#define DSI_VACT_NL		0x2C
#define DSI_HSA_WC		0x50
#define DSI_HBP_WC		0x54
#define DSI_HFP_WC		0x58

#define DSI_CMDQ_SIZE		0x60
#define CMDQ_SIZE		0x3f

#define DSI_HSTX_CKL_WC		0x64
#define DSI_RACK		0x84

#define DSI_PHY_LCCON		0x104
#define LC_HS_TX_EN		BIT(0)
#define LC_ULPM_EN		BIT(1)
#define LC_WAKEUP_EN		BIT(2)

#define DSI_PHY_LD0CON		0x108
#define LD0_HS_TX_EN		BIT(0)
#define LD0_ULPM_EN		BIT(1)
#define LD0_WAKEUP_EN		BIT(2)

#define DSI_PHY_TIMECON0	0x110
#define LPX			(0xff << 0)
#define HS_PRPR			(0xff << 8)
#define HS_ZERO			(0xff << 16)
#define HS_TRAIL		(0xff << 24)

#define DSI_PHY_TIMECON1	0x114
#define TA_GO			(0xff << 0)
#define TA_SURE			(0xff << 8)
#define TA_GET			(0xff << 16)
#define DA_HS_EXIT		(0xff << 24)

#define DSI_PHY_TIMECON2	0x118
#define CONT_DET		(0xff << 0)
#define CLK_ZERO		(0xff << 16)
#define CLK_TRAIL		(0xff << 24)

#define DSI_PHY_TIMECON3	0x11c
#define CLK_HS_PRPR		(0xff << 0)
#define CLK_HS_POST		(0xff << 8)
#define CLK_HS_EXIT		(0xff << 16)

#define DSI_PHY_TIMECON4	0x120
#define ULPS_WAKEUP		(0x1fffff << 0)

#define DSI_CMDQ0		0x180

#define NS_TO_CYCLE(n, c)    ((n) / c + (((n) % c) ? 1 : 0))

struct dsi_cmd_t0 {
	u8 config;
	u8 type;
	u8 data0;
	u8 data1;
};

struct dsi_cmd_t1 {
	unsigned config;
	unsigned type;
	unsigned mem_start0;
	unsigned mem_start1;
};

struct dsi_cmd_t2 {
	u8 config;
	u8 type;
	u16 wc16;
	u8 *pdata;
};

struct dsi_cmd_t3 {
	unsigned config;
	unsigned type;
	unsigned mem_start0;
	unsigned mem_start1;
};

static void mtk_dsi_mask(struct mtk_dsi *dsi, u32 offset, u32 mask, u32 data)
{
	u32 temp = readl(dsi->regs + offset);

	writel((temp & ~mask) | (data & mask), dsi->regs + offset);
}

static void dsi_phy_timconfig(struct mtk_dsi *dsi)
{
	u32 timcon0, timcon1, timcon2, timcon3;
	unsigned int ui, cycle_time;
	unsigned int lpx;

	ui = 1000 / dsi->data_rate + 0x01;
	cycle_time = 8000 / dsi->data_rate + 0x01;
	lpx = 5;

	timcon0 = (8 << 24) | (0xa << 16) | (0x6 << 8) | lpx;
	timcon1 = (7 << 24) | (5 * lpx << 16) | ((3 * lpx) / 2) << 8 |
		  (4 * lpx);
	timcon2 = ((NS_TO_CYCLE(0x64, cycle_time) + 0xa) << 24) |
		  (NS_TO_CYCLE(0x150, cycle_time) << 16);
	timcon3 = (2 * lpx) << 16 | NS_TO_CYCLE(80 + 52 * ui, cycle_time) << 8 |
		   NS_TO_CYCLE(0x40, cycle_time);

	writel(timcon0, dsi->regs + DSI_PHY_TIMECON0);
	writel(timcon1, dsi->regs + DSI_PHY_TIMECON1);
	writel(timcon2, dsi->regs + DSI_PHY_TIMECON2);
	writel(timcon3, dsi->regs + DSI_PHY_TIMECON3);
}

static void mtk_dsi_enable(struct mtk_dsi *dsi)
{
	mtk_dsi_mask(dsi, DSI_CON_CTRL, DSI_EN, DSI_EN);
}

static void mtk_dsi_disable(struct mtk_dsi *dsi)
{
	mtk_dsi_mask(dsi, DSI_CON_CTRL, DSI_EN, 0);
}

static void mtk_dsi_reset(struct mtk_dsi *dsi)
{
	mtk_dsi_mask(dsi, DSI_CON_CTRL, DSI_RESET, DSI_RESET);
	mtk_dsi_mask(dsi, DSI_CON_CTRL, DSI_RESET, 0);
}

static int mtk_dsi_poweron(struct mtk_dsi *dsi)
{
	struct device *dev = dsi->dev;
	int ret;

	if (++dsi->refcount != 1)
		return 0;

	ret = clk_prepare_enable(dsi->engine_clk);
	if (ret < 0) {
		dev_err(dev, "can't enable engine %d\n", ret);
		goto err_engine_clk;
	}

	ret = clk_prepare_enable(dsi->digital_clk);
	if (ret < 0) {
		dev_err(dev, "can't enable digital %d\n", ret);
		goto err_digital_clk;
	}

	/**
	 * data_rate = (pixel_clock / 1000) * pixel_dipth * mipi_ratio;
	 * pixel_clock unit is Khz, data_rata unit is MHz, so need divide 1000.
	 * mipi_ratio is mipi clk coefficient for balance the pixel clk in mipi.
	 * we set mipi_ratio is 1.05.
	 */

	dsi->data_rate = dsi->vm.pixelclock * 3 * 21 / (1 * 1000 * 10);

	mtk_mipi_tx_set_data_rate(dsi->phy, dsi->data_rate);
	phy_power_on(dsi->phy);

	return 0;

err_digital_clk:
	clk_disable_unprepare(dsi->engine_clk);
err_engine_clk:
	dsi->refcount--;
	return ret;
}

static void dsi_clk_ulp_mode_enter(struct mtk_dsi *dsi)
{
	mtk_dsi_mask(dsi, DSI_PHY_LCCON, LC_HS_TX_EN, 0);
	mtk_dsi_mask(dsi, DSI_PHY_LCCON, LC_ULPM_EN, 0);
}

static void dsi_clk_ulp_mode_leave(struct mtk_dsi *dsi)
{
	mtk_dsi_mask(dsi, DSI_PHY_LCCON, LC_ULPM_EN, 0);
	mtk_dsi_mask(dsi, DSI_PHY_LCCON, LC_WAKEUP_EN, LC_WAKEUP_EN);
	mtk_dsi_mask(dsi, DSI_PHY_LCCON, LC_WAKEUP_EN, 0);
}

static void dsi_lane0_ulp_mode_enter(struct mtk_dsi *dsi)
{
	mtk_dsi_mask(dsi, DSI_PHY_LD0CON, LD0_HS_TX_EN, 0);
	mtk_dsi_mask(dsi, DSI_PHY_LD0CON, LD0_ULPM_EN, 0);
}

static void dsi_lane0_ulp_mode_leave(struct mtk_dsi *dsi)
{
	mtk_dsi_mask(dsi, DSI_PHY_LD0CON, LD0_ULPM_EN, 0);
	mtk_dsi_mask(dsi, DSI_PHY_LD0CON, LD0_WAKEUP_EN, LD0_WAKEUP_EN);
	mtk_dsi_mask(dsi, DSI_PHY_LD0CON, LD0_WAKEUP_EN, 0);
}

static bool dsi_clk_hs_state(struct mtk_dsi *dsi)
{
	u32 tmp_reg1;

	tmp_reg1 = readl(dsi->regs + DSI_PHY_LCCON);
	return ((tmp_reg1 & LC_HS_TX_EN) == 1) ? true : false;
}

static void dsi_clk_hs_mode(struct mtk_dsi *dsi, bool enter)
{
	if (enter && !dsi_clk_hs_state(dsi))
		mtk_dsi_mask(dsi, DSI_PHY_LCCON, LC_HS_TX_EN, LC_HS_TX_EN);
	else if (!enter && dsi_clk_hs_state(dsi))
		mtk_dsi_mask(dsi, DSI_PHY_LCCON, LC_HS_TX_EN, 0);
}

static void dsi_set_mode(struct mtk_dsi *dsi)
{
	u32 vid_mode = CMD_MODE;

	if (dsi->mode_flags & MIPI_DSI_MODE_VIDEO) {
		vid_mode = SYNC_PULSE_MODE;

		if ((dsi->mode_flags & MIPI_DSI_MODE_VIDEO_BURST) &&
		    !(dsi->mode_flags & MIPI_DSI_MODE_VIDEO_SYNC_PULSE))
			vid_mode = BURST_MODE;
	}

	writel(vid_mode, dsi->regs + DSI_MODE_CTRL);
}

static void dsi_ps_control_vact(struct mtk_dsi *dsi)
{
	struct videomode *vm = &dsi->vm;
	u32 dsi_buf_bpp, ps_wc;
	u32 ps_bpp_mode;

	if (dsi->format == MIPI_DSI_FMT_RGB565)
		dsi_buf_bpp = 2;
	else
		dsi_buf_bpp = 3;

	ps_wc = vm->hactive * dsi_buf_bpp;
	ps_bpp_mode = ps_wc;

	switch (dsi->format) {
	case MIPI_DSI_FMT_RGB888:
		ps_bpp_mode |= PACKED_PS_24BIT_RGB888;
		break;
	case MIPI_DSI_FMT_RGB666:
		ps_bpp_mode |= PACKED_PS_18BIT_RGB666;
		break;
	case MIPI_DSI_FMT_RGB666_PACKED:
		ps_bpp_mode |= LOOSELY_PS_18BIT_RGB666;
		break;
	case MIPI_DSI_FMT_RGB565:
		ps_bpp_mode |= PACKED_PS_16BIT_RGB565;
		break;
	}

	writel(vm->vactive, dsi->regs + DSI_VACT_NL);
	writel(ps_bpp_mode, dsi->regs + DSI_PSCTRL);
	writel(ps_wc, dsi->regs + DSI_HSTX_CKL_WC);
}

static void dsi_rxtx_control(struct mtk_dsi *dsi)
{
	u32 tmp_reg;

	switch (dsi->lanes) {
	case 1:
		tmp_reg = 1 << 2;
		break;
	case 2:
		tmp_reg = 3 << 2;
		break;
	case 3:
		tmp_reg = 7 << 2;
		break;
	case 4:
		tmp_reg = 0xf << 2;
		break;
	default:
		tmp_reg = 0xf << 2;
		break;
	}

	writel(tmp_reg, dsi->regs + DSI_TXRX_CTRL);
}

static void dsi_ps_control(struct mtk_dsi *dsi)
{
	unsigned int dsi_tmp_buf_bpp;
	u32 tmp_reg;

	switch (dsi->format) {
	case MIPI_DSI_FMT_RGB888:
		tmp_reg = PACKED_PS_24BIT_RGB888;
		dsi_tmp_buf_bpp = 3;
		break;
	case MIPI_DSI_FMT_RGB666:
		tmp_reg = LOOSELY_PS_18BIT_RGB666;
		dsi_tmp_buf_bpp = 3;
		break;
	case MIPI_DSI_FMT_RGB666_PACKED:
		tmp_reg = PACKED_PS_18BIT_RGB666;
		dsi_tmp_buf_bpp = 3;
		break;
	case MIPI_DSI_FMT_RGB565:
		tmp_reg = PACKED_PS_16BIT_RGB565;
		dsi_tmp_buf_bpp = 2;
		break;
	default:
		tmp_reg = PACKED_PS_24BIT_RGB888;
		dsi_tmp_buf_bpp = 3;
		break;
	}

	tmp_reg += dsi->vm.hactive * dsi_tmp_buf_bpp & DSI_PS_WC;
	writel(tmp_reg, dsi->regs + DSI_PSCTRL);
}

static void dsi_config_vdo_timing(struct mtk_dsi *dsi)
{
	unsigned int horizontal_sync_active_byte;
	unsigned int horizontal_backporch_byte;
	unsigned int horizontal_frontporch_byte;
	unsigned int dsi_tmp_buf_bpp;

	struct videomode *vm = &dsi->vm;

	if (dsi->format == MIPI_DSI_FMT_RGB565)
		dsi_tmp_buf_bpp = 2;
	else
		dsi_tmp_buf_bpp = 3;

	writel(vm->vsync_len, dsi->regs + DSI_VSA_NL);
	writel(vm->vback_porch, dsi->regs + DSI_VBP_NL);
	writel(vm->vfront_porch, dsi->regs + DSI_VFP_NL);
	writel(vm->vactive, dsi->regs + DSI_VACT_NL);

	horizontal_sync_active_byte = (vm->hsync_len * dsi_tmp_buf_bpp - 10);

	if (dsi->mode_flags & MIPI_DSI_MODE_VIDEO_SYNC_PULSE)
		horizontal_backporch_byte =
			(vm->hback_porch * dsi_tmp_buf_bpp - 10);
	else
		horizontal_backporch_byte = ((vm->hback_porch + vm->hsync_len) *
			dsi_tmp_buf_bpp - 10);

	horizontal_frontporch_byte = (vm->hfront_porch * dsi_tmp_buf_bpp - 12);

	writel(horizontal_sync_active_byte, dsi->regs + DSI_HSA_WC);
	writel(horizontal_backporch_byte, dsi->regs + DSI_HBP_WC);
	writel(horizontal_frontporch_byte, dsi->regs + DSI_HFP_WC);

	dsi_ps_control(dsi);
}

static void dsi_set_interrupt_enable(struct mtk_dsi *dsi)
{
	writel(0x0000003f, dsi->regs + DSI_INTEN);
}

static void mtk_dsi_start(struct mtk_dsi *dsi)
{
	writel(0, dsi->regs + DSI_START);
	writel(1, dsi->regs + DSI_START);
}

static void mtk_dsi_poweroff(struct mtk_dsi *dsi)
{
	if (WARN_ON(dsi->refcount == 0))
		return;

	if (--dsi->refcount != 0)
		return;

	dsi_lane0_ulp_mode_enter(dsi);
	dsi_clk_ulp_mode_enter(dsi);

	mtk_dsi_disable(dsi);

	clk_disable_unprepare(dsi->engine_clk);
	clk_disable_unprepare(dsi->digital_clk);

	phy_power_off(dsi->phy);
}

static void mtk_output_dsi_enable(struct mtk_dsi *dsi)
{
	int ret;

	if (dsi->enabled)
		return;

	ret = mtk_dsi_poweron(dsi);
	if (ret < 0) {
		DRM_ERROR("failed to power on dsi\n");
		return;
	}

	/* need set to cmd mode for send panel init code */
	dsi_set_interrupt_enable(dsi);
	dsi_rxtx_control(dsi);
	mtk_dsi_enable(dsi);
	dsi_clk_ulp_mode_leave(dsi);
	dsi_lane0_ulp_mode_leave(dsi);
	dsi_clk_hs_mode(dsi, 0);
	dsi_ps_control_vact(dsi);
	dsi_phy_timconfig(dsi);
	dsi_config_vdo_timing(dsi);

	if (dsi->panel) {
		if (drm_panel_prepare(dsi->panel)) {
			DRM_ERROR("failed to setup the panel\n");
			return;
		}
	}

	dsi_set_mode(dsi);
	dsi_clk_hs_mode(dsi, 1);
	mtk_dsi_start(dsi);

	dsi->enabled = true;
}

static void mtk_output_dsi_disable(struct mtk_dsi *dsi)
{
	if (!dsi->enabled)
		return;

	if (dsi->panel) {
		if (drm_panel_disable(dsi->panel)) {
			DRM_ERROR("failed to disable the panel\n");
			return;
		}
	}

	mtk_dsi_poweroff(dsi);

	dsi->enabled = false;
}

static void mtk_dsi_encoder_destroy(struct drm_encoder *encoder)
{
	drm_encoder_cleanup(encoder);
}

static const struct drm_encoder_funcs mtk_dsi_encoder_funcs = {
	.destroy = mtk_dsi_encoder_destroy,
};

static void mtk_dsi_encoder_dpms(struct drm_encoder *encoder, int mode)
{
	struct mtk_dsi *dsi = encoder_to_dsi(encoder);

	DRM_INFO("mtk_dsi_encoder_dpms 0x%x\n", mode);

	switch (mode) {
	case DRM_MODE_DPMS_ON:
		mtk_output_dsi_enable(dsi);
		break;
	case DRM_MODE_DPMS_STANDBY:
	case DRM_MODE_DPMS_SUSPEND:
	case DRM_MODE_DPMS_OFF:
		mtk_output_dsi_disable(dsi);
		break;
	default:
		break;
	}
}

static bool mtk_dsi_encoder_mode_fixup(
	struct drm_encoder *encoder,
	const struct drm_display_mode *mode,
	struct drm_display_mode *adjusted_mode
	)
{
	return true;
}

static void mtk_dsi_encoder_prepare(struct drm_encoder *encoder)
{
	struct mtk_dsi *dsi = encoder_to_dsi(encoder);

	mtk_output_dsi_disable(dsi);
}

static void mtk_dsi_encoder_mode_set(
		struct drm_encoder *encoder,
		struct drm_display_mode *mode,
		struct drm_display_mode *adjusted
		)
{
	struct mtk_dsi *dsi = encoder_to_dsi(encoder);

	dsi->vm.pixelclock = adjusted->clock;
	dsi->vm.hactive = adjusted->hdisplay;
	dsi->vm.hback_porch = adjusted->htotal - adjusted->hsync_end;
	dsi->vm.hfront_porch = adjusted->hsync_start - adjusted->hdisplay;
	dsi->vm.hsync_len = adjusted->hsync_end - adjusted->hsync_start;

	dsi->vm.vactive = adjusted->vdisplay;
	dsi->vm.vback_porch = adjusted->vtotal - adjusted->vsync_end;
	dsi->vm.vfront_porch = adjusted->vsync_start - adjusted->vdisplay;
	dsi->vm.vsync_len = adjusted->vsync_end - adjusted->vsync_start;
}

static void mtk_dsi_encoder_disable(struct drm_encoder *encoder)
{
	struct mtk_dsi *dsi = encoder_to_dsi(encoder);

	mtk_output_dsi_disable(dsi);
}

static void mtk_dsi_encoder_enable(struct drm_encoder *encoder)
{
	struct mtk_dsi *dsi = encoder_to_dsi(encoder);

	mtk_output_dsi_enable(dsi);
}

static void mtk_dsi_encoder_commit(struct drm_encoder *encoder)
{
	struct mtk_dsi *dsi = encoder_to_dsi(encoder);

	mtk_output_dsi_enable(dsi);
}

static enum drm_connector_status
mtk_dsi_connector_detect(struct drm_connector *connector, bool force)
{
	struct mtk_dsi *dsi = connector_to_dsi(connector);

	if (!dsi->panel) {
		dsi->panel = of_drm_find_panel(dsi->panel_node);
		if (dsi->panel)
			drm_panel_attach(dsi->panel, &dsi->conn);
	} else if (!dsi->panel_node) {
		drm_panel_detach(dsi->panel);
		dsi->panel = NULL;
	}

	if (dsi->panel)
		return connector_status_connected;

	return connector_status_disconnected;
}

static void mtk_dsi_connector_destroy(struct drm_connector *connector)
{
	drm_connector_unregister(connector);
	drm_connector_cleanup(connector);
}

static int mtk_dsi_connector_get_modes(struct drm_connector *connector)
{
	struct mtk_dsi *dsi = connector_to_dsi(connector);

	return drm_panel_get_modes(dsi->panel);
}

static struct drm_encoder
*mtk_dsi_connector_best_encoder(struct drm_connector *connector)
{
	struct mtk_dsi *dsi = connector_to_dsi(connector);

	return &dsi->encoder;
}

static const struct drm_encoder_helper_funcs mtk_dsi_encoder_helper_funcs = {
	.dpms = mtk_dsi_encoder_dpms,
	.mode_fixup = mtk_dsi_encoder_mode_fixup,
	.prepare = mtk_dsi_encoder_prepare,
	.mode_set = mtk_dsi_encoder_mode_set,
	.commit = mtk_dsi_encoder_commit,
	.disable = mtk_dsi_encoder_disable,
	.enable = mtk_dsi_encoder_enable,
};

static const struct drm_connector_funcs mtk_dsi_connector_funcs = {
	.dpms = drm_atomic_helper_connector_dpms,
	.detect = mtk_dsi_connector_detect,
	.fill_modes = drm_helper_probe_single_connector_modes,
	.destroy = mtk_dsi_connector_destroy,
	.reset = drm_atomic_helper_connector_reset,
	.atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_connector_destroy_state,
};

static const struct drm_connector_helper_funcs
	mtk_dsi_connector_helper_funcs = {
	.get_modes = mtk_dsi_connector_get_modes,
	.best_encoder = mtk_dsi_connector_best_encoder,
};

static int mtk_drm_attach_lcm_bridge(struct drm_bridge *bridge,
				     struct drm_encoder *encoder)
{
	int ret;

	encoder->bridge = bridge;
	bridge->encoder = encoder;
	ret = drm_bridge_attach(encoder->dev, bridge);
	if (ret) {
		DRM_ERROR("Failed to attach bridge to drm\n");
		return ret;
	}

	return 0;
}

static int mtk_dsi_create_conn_enc(struct drm_device *drm, struct mtk_dsi *dsi)
{
	int ret;

	ret = drm_encoder_init(drm, &dsi->encoder, &mtk_dsi_encoder_funcs,
			       DRM_MODE_ENCODER_DSI);
	if (ret) {
		DRM_ERROR("Failed to encoder init to drm\n");
		return ret;
	}

	drm_encoder_helper_add(&dsi->encoder, &mtk_dsi_encoder_helper_funcs);

	dsi->encoder.possible_crtcs = 1;

	/* Pre-empt DP connector creation if there's a bridge */
	if (dsi->bridge) {
		ret = mtk_drm_attach_lcm_bridge(dsi->bridge, &dsi->encoder);
		if (!ret)
			return 0;
	}

	ret = drm_connector_init(drm, &dsi->conn, &mtk_dsi_connector_funcs,
				 DRM_MODE_CONNECTOR_DSI);
	if (ret) {
		DRM_ERROR("Failed to connector init to drm\n");
		goto errconnector;
	}

	drm_connector_helper_add(&dsi->conn, &mtk_dsi_connector_helper_funcs);

	ret = drm_connector_register(&dsi->conn);
	if (ret) {
		DRM_ERROR("Failed to connector register to drm\n");
		goto errconnectorreg;
	}

	dsi->conn.dpms = DRM_MODE_DPMS_OFF;
	drm_mode_connector_attach_encoder(&dsi->conn, &dsi->encoder);

	if (dsi->panel) {
		ret = drm_panel_attach(dsi->panel, &dsi->conn);
		if (ret) {
			DRM_ERROR("Failed to attact panel to drm\n");
			return ret;
		}
	}

	return 0;

errconnector:
	drm_encoder_cleanup(&dsi->encoder);
errconnectorreg:
	drm_connector_cleanup(&dsi->conn);

	return ret;
}

static void mtk_dsi_destroy_conn_enc(struct mtk_dsi *dsi)
{
	drm_encoder_cleanup(&dsi->encoder);
	drm_connector_unregister(&dsi->conn);
	drm_connector_cleanup(&dsi->conn);
}

static int mtk_dsi_host_attach(struct mipi_dsi_host *host,
			       struct mipi_dsi_device *device)
{
	struct mtk_dsi *dsi = host_to_dsi(host);

	dsi->lanes = device->lanes;
	dsi->format = device->format;
	dsi->mode_flags = device->mode_flags;
	dsi->panel_node = device->dev.of_node;

	if (dsi->conn.dev)
		drm_helper_hpd_irq_event(dsi->conn.dev);

	return 0;
}

static int mtk_dsi_host_detach(struct mipi_dsi_host *host,
			       struct mipi_dsi_device *device)
{
	struct mtk_dsi *dsi = host_to_dsi(host);

	dsi->panel_node = NULL;

	if (dsi->conn.dev)
		drm_helper_hpd_irq_event(dsi->conn.dev);

	return 0;
}

static ssize_t mtk_dsi_host_transfer(struct mipi_dsi_host *host,
				     const struct mipi_dsi_msg *msg)
{
	struct mtk_dsi *dsi = host_to_dsi(host);
	const char *tx_buf = msg->tx_buf;

	u32 i, timeout_ms = 200;
	u32 goto_addr, mask_para, set_para, reg_val;
	struct dsi_cmd_t0 t0;
	struct dsi_cmd_t2 t2;

	#ifdef INTERRUPT_MODE
	u32 ret;

	i = readl(dsi->dsi_reg_base + DSI_INTSTA);
	ret = wait_event_interruptible_timeout(
			_dsi_cmd_done_wait_queue,
			!(i & BIT(31)), msecs_to_jiffies(timeout_ms)
			);
	if (0 == ret) {
		DRM_INFO("dsi wait not busy timeout!\n");
		mtk_dsi_clk_enable(dsi);
		mtk_dsi_reset(dsi);
	}
	#else
	while (timeout_ms--) {
		i = readl(dsi->regs + DSI_INTSTA);
		if (!(i & BIT(31)))
			break;

		usleep_range(1, 2);
	}

	if (0 == timeout_ms) {
		mtk_dsi_enable(dsi);
		mtk_dsi_reset(dsi);
	}
	#endif

	if (msg->tx_len > 1) {
		t2.config = 2;
		t2.type = msg->type;
		t2.wc16 = msg->tx_len;

		reg_val = (t2.wc16 << 16) | (t2.type << 8) | t2.config;

		writel(reg_val, dsi->regs + DSI_CMDQ0);

		for (i = 0; i < msg->tx_len; i++) {
			goto_addr = DSI_CMDQ0 + i;
			mask_para = (0xff << ((goto_addr & 0x3) * 8));
			set_para = (tx_buf[i] << ((goto_addr & 0x3) * 8));
			mtk_dsi_mask(dsi, goto_addr & (~0x3), mask_para,
				     set_para);
		}

		mtk_dsi_mask(dsi, DSI_CMDQ_SIZE, CMDQ_SIZE,
			     1 + (msg->tx_len - 1) / 4);
	} else {
		t0.config = 0;
		t0.type = msg->type;
		t0.data0 = tx_buf[0];
		if (msg->tx_len == 2)
			t0.data1 = tx_buf[1];
		else
			t0.data1 = 0;

		reg_val = (t0.data1 << 24) | (t0.data0 << 16) | (t0.type << 8) |
			   t0.config;

		writel(reg_val, dsi->regs + DSI_CMDQ0);
		mtk_dsi_mask(dsi, DSI_CMDQ_SIZE, CMDQ_SIZE, 1);
	}

	mtk_dsi_start(dsi);

	return 0;
}

static const struct mipi_dsi_host_ops mtk_dsi_ops = {
	.attach = mtk_dsi_host_attach,
	.detach = mtk_dsi_host_detach,
	.transfer = mtk_dsi_host_transfer,
};

static void mtk_dsi_ddp_start(struct mtk_ddp_comp *comp)
{
	struct mtk_dsi *dsi = container_of(comp, struct mtk_dsi, ddp_comp);

	mtk_dsi_poweron(dsi);
}

static void mtk_dsi_ddp_stop(struct mtk_ddp_comp *comp)
{
	struct mtk_dsi *dsi = container_of(comp, struct mtk_dsi, ddp_comp);

	mtk_dsi_poweroff(dsi);
}

static const struct mtk_ddp_comp_funcs mtk_dsi_funcs = {
	.start = mtk_dsi_ddp_start,
	.stop = mtk_dsi_ddp_stop,
};

static int mtk_dsi_bind(struct device *dev, struct device *master, void *data)
{
	int ret;
	struct drm_device *drm = data;
	struct mtk_dsi *dsi = dev_get_drvdata(dev);

	ret = mtk_ddp_comp_register(drm, &dsi->ddp_comp);
	if (ret < 0) {
		dev_err(dev, "Failed to register component %s: %d\n",
			dev->of_node->full_name, ret);
		return ret;
	}

	ret = mtk_dsi_create_conn_enc(drm, dsi);
	if (ret) {
		DRM_ERROR("Encoder create failed with %d\n", ret);
		goto err_unregister;
	}

	return 0;

err_unregister:
	mtk_ddp_comp_unregister(drm, &dsi->ddp_comp);
	return ret;
}

static void mtk_dsi_unbind(struct device *dev, struct device *master,
			   void *data)
{
	struct drm_device *drm = data;
	struct mtk_dsi *dsi;

	dsi = platform_get_drvdata(to_platform_device(dev));
	mtk_dsi_destroy_conn_enc(dsi);
	mipi_dsi_host_unregister(&dsi->host);
	mtk_ddp_comp_unregister(drm, &dsi->ddp_comp);
}

static const struct component_ops mtk_dsi_component_ops = {
	.bind = mtk_dsi_bind,
	.unbind = mtk_dsi_unbind,
};

#define INTERRUPT_MODE

#ifdef INTERRUPT_MODE
static wait_queue_head_t _dsi_cmd_done_wait_queue;
static wait_queue_head_t _dsi_dcs_read_wait_queue;
static wait_queue_head_t _dsi_wait_bta_te;
static wait_queue_head_t _dsi_wait_ext_te;
static wait_queue_head_t _dsi_wait_vm_done_queue;
static wait_queue_head_t _dsi_wait_vm_cmd_done_queue;

static irqreturn_t mediatek_dsi_irq(int irq, void *dev_id)
{
	struct mtk_dsi *dsi = dev_id;
	u32 status, tmp;

	status = readl(dsi->regs + DSI_INTSTA);

	if (status & BIT(0)) {
		/* write clear RD_RDY interrupt */
		/* write clear RD_RDY interrupt must be before DSI_RACK */
		/* because CMD_DONE will raise after DSI_RACK, */
		/* so write clear RD_RDY after that will clear CMD_DONE too */
		do {
			/* send read ACK */
			mtk_dsi_mask(dsi, DSI_RACK,	BIT(0), BIT(0));
			tmp = readl(dsi->regs + DSI_INTSTA);
		} while (tmp & BIT(31));

		mtk_dsi_mask(dsi, DSI_INTSTA, BIT(0), ~(BIT(0)));
		wake_up_interruptible(&_dsi_dcs_read_wait_queue);
	}

	if (status & BIT(1)) {
		mtk_dsi_mask(dsi, DSI_INTSTA, BIT(1), ~(BIT(1)));
		wake_up_interruptible(&_dsi_cmd_done_wait_queue);
	}

	if (status & BIT(2)) {
		mtk_dsi_mask(dsi, DSI_INTSTA, BIT(2), ~(BIT(2)));
		wake_up_interruptible(&_dsi_wait_bta_te);
	}

	if (status & BIT(4)) {
		mtk_dsi_mask(dsi, DSI_INTSTA, BIT(4), ~(BIT(4)));
		wake_up_interruptible(&_dsi_wait_ext_te);
	}

	if (status & BIT(3)) {
		mtk_dsi_mask(dsi, DSI_INTSTA, BIT(3), ~(BIT(3)));
		wake_up_interruptible(&_dsi_wait_vm_done_queue);
	}

	if (status & BIT(5)) {
		mtk_dsi_mask(dsi, DSI_INTSTA, BIT(5), ~(BIT(5)));
		wake_up_interruptible(&_dsi_wait_vm_cmd_done_queue);
	}

	return IRQ_HANDLED;
}
#endif

static int mtk_dsi_probe(struct platform_device *pdev)
{
	struct mtk_dsi *dsi;
	struct device *dev = &pdev->dev;
	struct device_node *remote_node, *endpoint;
	struct resource *regs;
	int comp_id;
	int ret;

	dsi = devm_kzalloc(dev, sizeof(*dsi), GFP_KERNEL);
	dsi->mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_SYNC_PULSE;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->lanes = 4;

	endpoint = of_graph_get_next_endpoint(dev->of_node, NULL);
	if (endpoint) {
		remote_node = of_graph_get_remote_port_parent(endpoint);
		of_node_put(endpoint);

		if (!remote_node) {
			dev_err(dev, "No panel connected\n");
			return -ENODEV;
		}

		dsi->device_node = remote_node;
		of_node_put(remote_node);
	}

	dsi->engine_clk = devm_clk_get(dev, "engine");
	if (IS_ERR(dsi->engine_clk)) {
		ret = PTR_ERR(dsi->engine_clk);
		dev_err(dev, "Failed to get engine clock: %d\n", ret);
		return ret;
	}

	dsi->digital_clk = devm_clk_get(dev, "digital");
	if (IS_ERR(dsi->digital_clk)) {
		ret = PTR_ERR(dsi->digital_clk);
		dev_err(dev, "Failed to get digital clock: %d\n", ret);
		return ret;
	}

	regs = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	dsi->regs = devm_ioremap_resource(dev, regs);
	if (IS_ERR(dsi->regs)) {
		ret = PTR_ERR(dsi->regs);
		dev_err(dev, "Failed to ioremap memory: %d\n", ret);
		return ret;
	}

	dsi->phy = devm_phy_get(dev, "dphy");
	if (IS_ERR(dsi->phy)) {
		ret = PTR_ERR(dsi->phy);
		dev_err(dev, "Failed to get MIPI-DPHY: %d\n", ret);
		return ret;
	}

	dsi->host.ops = &mtk_dsi_ops;
	dsi->host.dev = dev;
	mipi_dsi_host_register(&dsi->host);

	dsi->bridge = of_drm_find_bridge(dsi->device_node);
	dsi->panel = of_drm_find_panel(dsi->device_node);

	DRM_INFO("dsi connector detect panel = 0x%p, bridge = 0x%p\n",
		 dsi->panel, dsi->bridge);

	if (!dsi->bridge && !dsi->panel) {
		dev_info(dev, "Waiting for bridge or panel driver\n");
		return -EPROBE_DEFER;
	}

	comp_id = mtk_ddp_comp_get_id(dev->of_node, MTK_DSI);
	if (comp_id < 0) {
		dev_err(dev, "Failed to identify by alias: %d\n", comp_id);
		return comp_id;
	}

	ret = mtk_ddp_comp_init(dev, dev->of_node, &dsi->ddp_comp, comp_id,
				&mtk_dsi_funcs);
	if (ret) {
		dev_err(dev, "Failed to initialize component: %d\n", ret);
		return ret;
	}

	#ifdef INTERRUPT_MODE
	dsi->irq = platform_get_irq(pdev, 0);
	if (dsi->irq < 0) {
		dev_err(&pdev->dev, "failed to request dsi irq resource\n");
		ret = dsi->irq;
		return -EPROBE_DEFER;
	}

	irq_set_status_flags(dsi->irq, IRQ_TYPE_LEVEL_LOW);
	ret = devm_request_irq(&pdev->dev, dsi->irq, mediatek_dsi_irq,
			       IRQF_TRIGGER_LOW, dev_name(&pdev->dev), dsi);
	if (ret) {
		dev_err(&pdev->dev, "failed to request mediatek dsi irq\n");
		return -EPROBE_DEFER;
	}

	DRM_INFO("dsi irq num is 0x%x\n", dsi->irq);

	init_waitqueue_head(&_dsi_cmd_done_wait_queue);
	init_waitqueue_head(&_dsi_dcs_read_wait_queue);
	init_waitqueue_head(&_dsi_wait_bta_te);
	init_waitqueue_head(&_dsi_wait_ext_te);
	init_waitqueue_head(&_dsi_wait_vm_done_queue);
	init_waitqueue_head(&_dsi_wait_vm_cmd_done_queue);
	#endif

	platform_set_drvdata(pdev, dsi);

	ret = component_add(&pdev->dev, &mtk_dsi_component_ops);
	if (ret) {
		dev_err(dev, "Failed to add DSI component\n");
		return -EPROBE_DEFER;
	}

	return 0;
}

static int mtk_dsi_remove(struct platform_device *pdev)
{
	struct mtk_dsi *dsi = platform_get_drvdata(pdev);

	mtk_output_dsi_disable(dsi);
	component_del(&pdev->dev, &mtk_dsi_component_ops);

	return 0;
}

#ifdef CONFIG_PM
static int mtk_dsi_suspend(struct device *dev)
{
	struct mtk_dsi *dsi;

	dsi = dev_get_drvdata(dev);

	mtk_output_dsi_disable(dsi);
	DRM_INFO("dsi suspend success!\n");

	return 0;
}

static int mtk_dsi_resume(struct device *dev)
{
	struct mtk_dsi *dsi;

	dsi = dev_get_drvdata(dev);

	mtk_output_dsi_enable(dsi);
	DRM_INFO("dsi resume success!\n");

	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(mtk_dsi_pm_ops, mtk_dsi_suspend, mtk_dsi_resume);

static const struct of_device_id mtk_dsi_of_match[] = {
	{ .compatible = "mediatek,mt2701-dsi" },
	{ .compatible = "mediatek,mt8173-dsi" },
	{ },
};

struct platform_driver mtk_dsi_driver = {
	.probe = mtk_dsi_probe,
	.remove = mtk_dsi_remove,
	.driver = {
		.name = "mtk-dsi",
		.of_match_table = mtk_dsi_of_match,
		.pm = &mtk_dsi_pm_ops,
	},
};
