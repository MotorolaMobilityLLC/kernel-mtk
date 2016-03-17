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
#include <drm/drm_crtc_helper.h>
#include <drm/drm_atomic_helper.h>

#include <linux/clk.h>
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
#include <linux/component.h>

#include <linux/regulator/consumer.h>

#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/wait.h>

#include <drm/drm_mipi_dsi.h>
#include <drm/drm_panel.h>

#include <video/mipi_display.h>
#include <video/videomode.h>

#include "mediatek_drm_drv.h"
#include "mediatek_drm_crtc.h"

#include "mediatek_drm_ddp.h"

#include "mediatek_drm_gem.h"
#include "mediatek_drm_dsi.h"

#define DSI_VIDEO_FIFO_DEPTH	(1920 / 4)
#define DSI_HOST_FIFO_DEPTH	64

#define DSI_START	0x00
#define DSI_INTEN	0x08
#define DSI_INTSTA	0x0c

#define DSI_CON_CTRL	0x10
#define DSI_RESET	BIT(0)

#define DSI_MODE_CTRL	0x14
#define MODE		(3)
#define CMD_MODE	0
#define SYNC_PULSE_MODE	1
#define SYNC_EVENT_MODE	2
#define BURST_MODE	3
#define FRM_MODE	BIT(16)
#define MIX_MODE	BIT(17)

#define DSI_TXRX_CTRL	0x18
#define VC_NUM		(2 << 0)
#define LANE_NUM	(0xf << 2)
#define DIS_EOT		BIT(6)
#define NULL_EN		BIT(7)
#define TE_FREERUN	BIT(8)
#define EXT_TE_EN	BIT(9)
#define EXT_TE_EDGE	BIT(10)
#define MAX_RTN_SIZE	(0xf << 12)
#define HSTX_CKLP_EN	BIT(16)

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

#define DSI_CMDQ_SIZE	0x60
#define CMDQ_SIZE		0x3f

#define DSI_HSTX_CKL_WC	0x64
#define DSI_RACK	0x84

#define DSI_PHY_LCCON	0x104
#define LC_HS_TX_EN	BIT(0)
#define LC_ULPM_EN	BIT(1)
#define LC_WAKEUP_EN	BIT(2)

#define DSI_PHY_LD0CON	0x108
#define LD0_HS_TX_EN	BIT(0)
#define LD0_ULPM_EN	BIT(1)
#define LD0_WAKEUP_EN	BIT(2)

#define DSI_PHY_TIMECON0	0x110
#define LPX		(0xff << 0)
#define HS_PRPR		(0xff << 8)
#define HS_ZERO		(0xff << 16)
#define HS_TRAIL	(0xff << 24)

#define DSI_PHY_TIMECON1	0x114
#define TA_GO		(0xff << 0)
#define TA_SURE		(0xff << 8)
#define TA_GET		(0xff << 16)
#define DA_HS_EXIT	(0xff << 24)

#define DSI_PHY_TIMECON2	0x118
#define CONT_DET	(0xff << 0)
#define CLK_ZERO	(0xff << 16)
#define CLK_TRAIL	(0xff << 24)

#define DSI_PHY_TIMECON3	0x11c
#define CLK_HS_PRPR	(0xff << 0)
#define CLK_HS_POST	(0xff << 8)
#define CLK_HS_EXIT	(0xff << 16)

#define DSI_CMDQ0	0x180

#define MIPITX_DSI0_CON		0x00
#define RG_DSI0_LDOCORE_EN	BIT(0)
#define RG_DSI0_CKG_LDOOUT_EN	BIT(1)
#define RG_DSI0_BCLK_SEL	(3 << 2)
#define RG_DSI0_LD_IDX_SEL	(7 << 4)
#define RG_DSI0_PHYCLK_SEL	(2 << 8)
#define RG_DSI0_DSICLK_FREQ_SEL	BIT(10)
#define RG_DSI0_LPTX_CLMP_EN	BIT(11)

#define MIPITX_DSI0_CLOCK_LANE	0x04
#define RG_DSI0_LNTC_LDOOUT_EN		BIT(0)
#define RG_DSI0_LNTC_CKLANE_EN		BIT(1)
#define RG_DSI0_LNTC_LPTX_IPLUS1	BIT(2)
#define RG_DSI0_LNTC_LPTX_IPLUS2	BIT(3)
#define RG_DSI0_LNTC_LPTX_IMINUS	BIT(4)
#define RG_DSI0_LNTC_LPCD_IPLUS		BIT(5)
#define RG_DSI0_LNTC_LPCD_IMLUS		BIT(6)
#define RG_DSI0_LNTC_RT_CODE		(0xf << 8)

#define MIPITX_DSI0_DATA_LANE0	0x08
#define RG_DSI0_LNT0_LDOOUT_EN		BIT(0)
#define RG_DSI0_LNT0_CKLANE_EN		BIT(1)
#define RG_DSI0_LNT0_LPTX_IPLUS1	BIT(2)
#define RG_DSI0_LNT0_LPTX_IPLUS2	BIT(3)
#define RG_DSI0_LNT0_LPTX_IMINUS	BIT(4)
#define RG_DSI0_LNT0_LPCD_IPLUS		BIT(5)
#define RG_DSI0_LNT0_LPCD_IMINUS	BIT(6)
#define RG_DSI0_LNT0_RT_CODE		(0xf << 8)

#define MIPITX_DSI0_DATA_LANE1	0x0c
#define RG_DSI0_LNT1_LDOOUT_EN		BIT(0)
#define RG_DSI0_LNT1_CKLANE_EN		BIT(1)
#define RG_DSI0_LNT1_LPTX_IPLUS1	BIT(2)
#define RG_DSI0_LNT1_LPTX_IPLUS2	BIT(3)
#define RG_DSI0_LNT1_LPTX_IMINUS	BIT(4)
#define RG_DSI0_LNT1_LPCD_IPLUS		BIT(5)
#define RG_DSI0_LNT1_LPCD_IMINUS	BIT(6)
#define RG_DSI0_LNT1_RT_CODE		(0xf << 8)

#define MIPITX_DSI0_DATA_LANE2	0x10
#define RG_DSI0_LNT2_LDOOUT_EN		BIT(0)
#define RG_DSI0_LNT2_CKLANE_EN		BIT(1)
#define RG_DSI0_LNT2_LPTX_IPLUS1	BIT(2)
#define RG_DSI0_LNT2_LPTX_IPLUS2	BIT(3)
#define RG_DSI0_LNT2_LPTX_IMINUS	BIT(4)
#define RG_DSI0_LNT2_LPCD_IPLUS		BIT(5)
#define RG_DSI0_LNT2_LPCD_IMINUS	BIT(6)
#define RG_DSI0_LNT2_RT_CODE		(0xf << 8)

#define MIPITX_DSI0_DATA_LANE3	0x14
#define RG_DSI0_LNT3_LDOOUT_EN		BIT(0)
#define RG_DSI0_LNT3_CKLANE_EN		BIT(1)
#define RG_DSI0_LNT3_LPTX_IPLUS1	BIT(2)
#define RG_DSI0_LNT3_LPTX_IPLUS2	BIT(3)
#define RG_DSI0_LNT3_LPTX_IMINUS	BIT(4)
#define RG_DSI0_LNT3_LPCD_IPLUS		BIT(5)
#define RG_DSI0_LNT3_LPCD_IMINUS	BIT(6)
#define RG_DSI0_LNT3_RT_CODE		(0xf << 8)

#define MIPITX_DSI_TOP_CON	0x40
#define RG_DSI_LNT_INTR_EN		BIT(0)
#define RG_DSI_LNT_HS_BIAS_EN		BIT(1)
#define RG_DSI_LNT_IMP_CAL_EN		BIT(2)
#define RG_DSI_LNT_TESTMODE_EN		BIT(3)
#define RG_DSI_LNT_IMP_CAL_CODE		(0xf << 4)
#define RG_DSI_LNT_AIO_SEL		(7 << 8)
#define RG_DSI_PAD_TIE_LOW_EN		BIT(11)
#define RG_DSI_DEBUG_INPUT_EN		BIT(12)
#define RG_DSI_PRESERVE			(7 << 13)

#define MIPITX_DSI_BG_CON	0x44
#define RG_DSI_BG_CORE_EN		BIT(0)
#define RG_DSI_BG_CKEN			BIT(1)
#define RG_DSI_BG_DIV			(0x3 << 2)
#define RG_DSI_BG_FAST_CHARGE		BIT(4)
#define RG_DSI_VOUT_MSK			(0x3ffff << 5)
#define RG_DSI_V12_SEL			(7 << 5)
#define RG_DSI_V10_SEL			(7 << 8)
#define RG_DSI_V072_SEL			(7 << 11)
#define RG_DSI_V04_SEL			(7 << 14)
#define RG_DSI_V032_SEL			(7 << 17)
#define RG_DSI_V02_SEL			(7 << 20)
#define RG_DSI_BG_R1_TRIM		(0xf << 24)
#define RG_DSI_BG_R2_TRIM		(0xf << 28)

#define MIPITX_DSI_PLL_CON0	0x50
#define RG_DSI0_MPPLL_PLL_EN		BIT(0)
#define RG_DSI0_MPPLL_DIV_MSK		(0x1ff << 1)
#define RG_DSI0_MPPLL_PREDIV		(3 << 1)
#define RG_DSI0_MPPLL_TXDIV0		(3 << 3)
#define RG_DSI0_MPPLL_TXDIV1		(3 << 5)
#define RG_DSI0_MPPLL_POSDIV		(7 << 7)
#define RG_DSI0_MPPLL_MONVC_EN		BIT(10)
#define RG_DSI0_MPPLL_MONREF_EN		BIT(11)
#define RG_DSI0_MPPLL_VOD_EN		BIT(12)

#define MIPITX_DSI_PLL_CON1	0x54
#define RG_DSI0_MPPLL_SDM_FRA_EN	BIT(0)
#define RG_DSI0_MPPLL_SDM_SSC_PH_INIT	BIT(1)
#define RG_DSI0_MPPLL_SDM_SSC_EN	BIT(2)
#define RG_DSI0_MPPLL_SDM_SSC_PRD	(0xffff << 16)

#define MIPITX_DSI_PLL_CON2	0x58

#define MIPITX_DSI_PLL_TOP 0x64
#define RG_DSI_MPPLL_PRESERVE		BIT(8)

#define MIPITX_DSI_PLL_PWR	0x68
#define RG_DSI_MPPLL_SDM_PWR_ON		BIT(0)
#define RG_DSI_MPPLL_SDM_ISO_EN		BIT(1)
#define RG_DSI_MPPLL_SDM_PWR_ACK	BIT(8)

#define MIPITX_DSI_SW_CTRL	0x80
#define SW_CTRL_EN			BIT(0)

#define MIPITX_DSI_SW_CTRL_CON0	0x84
#define SW_LNTC_LPTX_PRE_OE		BIT(0)
#define SW_LNTC_LPTX_OE			BIT(1)
#define SW_LNTC_LPTX_P			BIT(2)
#define SW_LNTC_LPTX_N			BIT(3)
#define SW_LNTC_HSTX_PRE_OE		BIT(4)
#define SW_LNTC_HSTX_OE			BIT(5)
#define SW_LNTC_HSTX_ZEROCLK		BIT(6)
#define SW_LNT0_LPTX_PRE_OE		BIT(7)
#define SW_LNT0_LPTX_OE			BIT(8)
#define SW_LNT0_LPTX_P			BIT(9)
#define SW_LNT0_LPTX_N			BIT(10)
#define SW_LNT0_HSTX_PRE_OE		BIT(11)
#define SW_LNT0_HSTX_OE			BIT(12)
#define SW_LNT0_LPRX_EN			BIT(13)
#define SW_LNT1_LPTX_PRE_OE		BIT(14)
#define SW_LNT1_LPTX_OE			BIT(15)
#define SW_LNT1_LPTX_P			BIT(16)
#define SW_LNT1_LPTX_N			BIT(17)
#define SW_LNT1_HSTX_PRE_OE		BIT(18)
#define SW_LNT1_HSTX_OE			BIT(19)
#define SW_LNT2_LPTX_PRE_OE		BIT(20)
#define SW_LNT2_LPTX_OE			BIT(21)
#define SW_LNT2_LPTX_P			BIT(22)
#define SW_LNT2_LPTX_N			BIT(23)
#define SW_LNT2_HSTX_PRE_OE		BIT(24)
#define SW_LNT2_HSTX_OE			BIT(25)

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

static void mtk_dsi_tx_mask(struct mtk_dsi *dsi, u32 offset, u32 mask, u32 data)
{
	u32 temp = readl(dsi->dsi_tx_reg_base + offset);

	writel((temp & ~mask) | (data & mask), dsi->dsi_tx_reg_base + offset);
}

static void mtk_dsi_mask(struct mtk_dsi *dsi, u32 offset, u32 mask, u32 data)
{
	u32 temp = readl(dsi->dsi_reg_base + offset);

	writel((temp & ~mask) | (data & mask), dsi->dsi_reg_base + offset);
}

static void dsi_phy_clk_switch_off(struct mtk_dsi *dsi)
{
	mtk_dsi_tx_mask(dsi, MIPITX_DSI_PLL_CON0, RG_DSI0_MPPLL_PLL_EN, 0);

	mtk_dsi_tx_mask(dsi, MIPITX_DSI_TOP_CON, RG_DSI_PAD_TIE_LOW_EN,
			RG_DSI_PAD_TIE_LOW_EN);

	mtk_dsi_tx_mask(dsi, MIPITX_DSI0_CLOCK_LANE, RG_DSI0_LNTC_LDOOUT_EN, 0);
	mtk_dsi_tx_mask(dsi, MIPITX_DSI0_DATA_LANE0, RG_DSI0_LNT0_LDOOUT_EN, 0);
	mtk_dsi_tx_mask(dsi, MIPITX_DSI0_DATA_LANE1, RG_DSI0_LNT1_LDOOUT_EN, 0);
	mtk_dsi_tx_mask(dsi, MIPITX_DSI0_DATA_LANE2, RG_DSI0_LNT2_LDOOUT_EN, 0);
	mtk_dsi_tx_mask(dsi, MIPITX_DSI0_DATA_LANE3, RG_DSI0_LNT3_LDOOUT_EN, 0);

	mtk_dsi_tx_mask(dsi, MIPITX_DSI_PLL_PWR,
			RG_DSI_MPPLL_SDM_ISO_EN | RG_DSI_MPPLL_SDM_PWR_ON,
			RG_DSI_MPPLL_SDM_ISO_EN);

	mtk_dsi_tx_mask(dsi, MIPITX_DSI_TOP_CON, RG_DSI_LNT_HS_BIAS_EN, 0);

	mtk_dsi_tx_mask(dsi, MIPITX_DSI0_CON,
			RG_DSI0_CKG_LDOOUT_EN |	RG_DSI0_LDOCORE_EN, 0);

	mtk_dsi_tx_mask(dsi, MIPITX_DSI_BG_CON,
			RG_DSI_BG_CKEN | RG_DSI_BG_CORE_EN, 0);

	mtk_dsi_tx_mask(dsi, MIPITX_DSI_PLL_CON0, RG_DSI0_MPPLL_DIV_MSK, 0);
}

static void dsi_phy_clk_setting(struct mtk_dsi *dsi)
{
	unsigned int txdiv;
	u64 pcw;
	u32 reg;

	mtk_dsi_tx_mask(dsi, MIPITX_DSI_TOP_CON,
			RG_DSI_LNT_IMP_CAL_CODE | RG_DSI_LNT_HS_BIAS_EN,
			(8 << 4) | RG_DSI_LNT_HS_BIAS_EN);

	mtk_dsi_tx_mask(dsi, MIPITX_DSI_BG_CON,
			RG_DSI_VOUT_MSK | RG_DSI_BG_CKEN | RG_DSI_BG_CORE_EN,
			(4 << 20) | (4 << 17) | (4 << 14) |
			(4 << 11) | (4 << 8) | (4 << 5) |
			RG_DSI_BG_CKEN | RG_DSI_BG_CORE_EN);
	usleep_range(10000, 11000);

	mtk_dsi_tx_mask(dsi, MIPITX_DSI0_CON,
			RG_DSI0_CKG_LDOOUT_EN | RG_DSI0_LDOCORE_EN,
			RG_DSI0_CKG_LDOOUT_EN | RG_DSI0_LDOCORE_EN);

	mtk_dsi_tx_mask(dsi, MIPITX_DSI_PLL_PWR,
			RG_DSI_MPPLL_SDM_PWR_ON | RG_DSI_MPPLL_SDM_ISO_EN,
			RG_DSI_MPPLL_SDM_PWR_ON);
	usleep_range(10000, 11000);

	mtk_dsi_tx_mask(dsi, MIPITX_DSI_PLL_CON0, RG_DSI0_MPPLL_PLL_EN, 0);

	/**
	 * data_rate = (pixel_clock / 1000) * pixel_dipth * mipi_ratio;
	 * pixel_clock unit is Khz, data_rata unit is MHz, so need divide 1000.
	 * mipi_ratio is mipi clk coefficient for balance the pixel clk in mipi.
	 * we set mipi_ratio is 1.05.
	 */
	dsi->data_rate = dsi->vm.pixelclock * 3 * 21 / (1 * 1000 * 10);
	if (dsi->data_rate > 1250) {
		DRM_ERROR("Wrong mipi dsi data rate, pls check it\n");
		return;
	} else if (dsi->data_rate >= 500) {
		txdiv = 1;
	} else if (dsi->data_rate >= 250) {
		txdiv = 2;
	} else if (dsi->data_rate >= 125) {
		txdiv = 4;
	} else if (dsi->data_rate > 62) {
		txdiv = 8;
	} else if (dsi->data_rate >= 50) {
		txdiv = 16;
	} else {
		DRM_ERROR("Wrong pixel clk, pls check pixel clk\n");
		return;
	}

	reg = readl(dsi->dsi_tx_reg_base + MIPITX_DSI_PLL_CON0);

	switch (txdiv) {
	case 1:
		reg &= ~RG_DSI0_MPPLL_TXDIV0;
		reg &= ~RG_DSI0_MPPLL_TXDIV1;
		break;
	case 2:
		reg = (reg & (~RG_DSI0_MPPLL_TXDIV0)) | (1 << 3);
		reg &= ~RG_DSI0_MPPLL_TXDIV1;
		break;
	case 4:
		reg = (reg & (~RG_DSI0_MPPLL_TXDIV0)) | (2 << 3);
		reg &= ~RG_DSI0_MPPLL_TXDIV1;
		break;
	case 8:
		reg = (reg & (~RG_DSI0_MPPLL_TXDIV0)) | (2 << 3);
		reg = (reg & (~RG_DSI0_MPPLL_TXDIV1)) | (1 << 5);
		break;
	case 16:
		reg = (reg & (~RG_DSI0_MPPLL_TXDIV0)) | (2 << 3);
		reg = (reg & (~RG_DSI0_MPPLL_TXDIV1)) | (2 << 5);
		break;
	default:
		break;
	}
	reg &= ~RG_DSI0_MPPLL_PREDIV;
	writel(reg, dsi->dsi_tx_reg_base + MIPITX_DSI_PLL_CON0);

	/**
	 * PLL PCW config
	 * PCW bit 24~30 = integer part of pcw
	 * PCW bit 0~23 = fractional part of pcw
	 * pcw = data_Rate*4*txdiv/(Ref_clk*2);
	 * Post DIV =4, so need data_Rate*4
	 * Ref_clk is 26MHz
	 */
	pcw = ((u64)dsi->data_rate * txdiv) << 24;
	do_div(pcw, 13);
	writel(pcw, dsi->dsi_tx_reg_base + MIPITX_DSI_PLL_CON2);

	mtk_dsi_tx_mask(dsi, MIPITX_DSI_PLL_CON1,
			RG_DSI0_MPPLL_SDM_FRA_EN, RG_DSI0_MPPLL_SDM_FRA_EN);

	mtk_dsi_tx_mask(dsi, MIPITX_DSI0_CLOCK_LANE,
			RG_DSI0_LNTC_LDOOUT_EN,	RG_DSI0_LNTC_LDOOUT_EN);
	mtk_dsi_tx_mask(dsi, MIPITX_DSI0_DATA_LANE0,
			RG_DSI0_LNT0_LDOOUT_EN,	RG_DSI0_LNT0_LDOOUT_EN);
	mtk_dsi_tx_mask(dsi, MIPITX_DSI0_DATA_LANE1,
			RG_DSI0_LNT1_LDOOUT_EN,	RG_DSI0_LNT1_LDOOUT_EN);
	mtk_dsi_tx_mask(dsi, MIPITX_DSI0_DATA_LANE2,
			RG_DSI0_LNT2_LDOOUT_EN,	RG_DSI0_LNT2_LDOOUT_EN);
	mtk_dsi_tx_mask(dsi, MIPITX_DSI0_DATA_LANE3,
			RG_DSI0_LNT3_LDOOUT_EN,	RG_DSI0_LNT3_LDOOUT_EN);

	mtk_dsi_tx_mask(dsi, MIPITX_DSI_PLL_CON0,
			RG_DSI0_MPPLL_PLL_EN, RG_DSI0_MPPLL_PLL_EN);
	usleep_range(1000, 1100);

	mtk_dsi_tx_mask(dsi, MIPITX_DSI_PLL_CON1, RG_DSI0_MPPLL_SDM_SSC_EN, 0);

	reg = readl(dsi->dsi_tx_reg_base + MIPITX_DSI_PLL_TOP);
	reg &= ~(0xffff00ff);
	reg |= (0x03 << 8);
	writel(reg, dsi->dsi_tx_reg_base + MIPITX_DSI_PLL_TOP);

	mtk_dsi_tx_mask(dsi, MIPITX_DSI_TOP_CON, RG_DSI_PAD_TIE_LOW_EN, 0);
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

	writel(timcon0, dsi->dsi_reg_base + DSI_PHY_TIMECON0);
	writel(timcon1, dsi->dsi_reg_base + DSI_PHY_TIMECON1);
	writel(timcon2, dsi->dsi_reg_base + DSI_PHY_TIMECON2);
	writel(timcon3, dsi->dsi_reg_base + DSI_PHY_TIMECON3);
}

static void mtk_dsi_reset(struct mtk_dsi *dsi)
{
	writel(3, dsi->dsi_reg_base + DSI_CON_CTRL);
	writel(2, dsi->dsi_reg_base + DSI_CON_CTRL);
}

static int mtk_dsi_poweron(struct mtk_dsi *dsi)
{
	int ret;
	struct drm_device *dev = dsi->drm_dev;

	dsi_phy_clk_setting(dsi);

	ret = clk_prepare_enable(dsi->dsi0_engine_clk_cg);
	if (ret < 0) {
		dev_err(dev->dev, "can't enable dsi0_engine_clk_cg %d\n", ret);
		goto err_dsi0_engine_clk_cg;
	}

	ret = clk_prepare_enable(dsi->dsi0_digital_clk_cg);
	if (ret < 0) {
		dev_err(dev->dev, "can't enable dsi0_digital_clk_cg %d\n", ret);
		goto err_dsi0_digital_clk_cg;
	}

	mtk_dsi_reset((dsi));
	dsi_phy_timconfig(dsi);

	return 0;

err_dsi0_digital_clk_cg:
	clk_disable_unprepare(dsi->dsi0_engine_clk_cg);

err_dsi0_engine_clk_cg:

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

static void dsi_lane0_ulp_mode(struct mtk_dsi *dsi, bool enter)
{
	if (enter) {
		mtk_dsi_mask(dsi, DSI_PHY_LD0CON, LD0_HS_TX_EN, 0);
		mtk_dsi_mask(dsi, DSI_PHY_LD0CON, LD0_ULPM_EN, 0);
	} else {
		mtk_dsi_mask(dsi, DSI_PHY_LD0CON, LD0_ULPM_EN, 0);
		mtk_dsi_mask(dsi, DSI_PHY_LD0CON, LD0_WAKEUP_EN, LD0_WAKEUP_EN);
		mtk_dsi_mask(dsi, DSI_PHY_LD0CON, LD0_WAKEUP_EN, 0);
	}
}

static bool dsi_clk_hs_state(struct mtk_dsi *dsi)
{
	u32 tmp_reg1;

	tmp_reg1 = readl(dsi->dsi_reg_base + DSI_PHY_LCCON);
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
	u32 tmp_reg1;

	tmp_reg1 = 0;

	if (dsi->mode_flags & MIPI_DSI_MODE_VIDEO) {
		tmp_reg1 = SYNC_PULSE_MODE;

		if (dsi->mode_flags & MIPI_DSI_MODE_VIDEO_BURST)
			tmp_reg1 = BURST_MODE;

		if (dsi->mode_flags & MIPI_DSI_MODE_VIDEO_SYNC_PULSE)
			tmp_reg1 = SYNC_PULSE_MODE;
	}

	writel(tmp_reg1, dsi->dsi_reg_base + DSI_MODE_CTRL);
}

static void dsi_set_cmd_mode(struct mtk_dsi *dsi)
{
	u32 tmp_reg1;

	tmp_reg1 = CMD_MODE;
	writel(tmp_reg1, dsi->dsi_reg_base + DSI_MODE_CTRL);
}

static void dsi_ps_control_vact(struct mtk_dsi *dsi)
{
	struct videomode *vm = &dsi->vm;
	u32 dsi_tmp_buf_bpp, ps_wc;
	u32 tmp_reg;
	u32 tmp_hstx_cklp_wc;

	if (dsi->format == MIPI_DSI_FMT_RGB565)
		dsi_tmp_buf_bpp = 2;
	else
		dsi_tmp_buf_bpp = 3;

	ps_wc = vm->vactive * dsi_tmp_buf_bpp;

	tmp_reg = ps_wc;

	switch (dsi->format) {
	case MIPI_DSI_FMT_RGB888:
		tmp_reg |= PACKED_PS_24BIT_RGB888;
		break;
	case MIPI_DSI_FMT_RGB666:
		tmp_reg |= PACKED_PS_18BIT_RGB666;
		break;
	case MIPI_DSI_FMT_RGB666_PACKED:
		tmp_reg |= LOOSELY_PS_18BIT_RGB666;
		break;
	case MIPI_DSI_FMT_RGB565:
		tmp_reg |= PACKED_PS_16BIT_RGB565;
		break;
	}

	tmp_hstx_cklp_wc = ps_wc;

	writel(vm->vactive, dsi->dsi_reg_base + DSI_VACT_NL);
	writel(tmp_reg, dsi->dsi_reg_base + DSI_PSCTRL);
	writel(tmp_hstx_cklp_wc, dsi->dsi_reg_base + DSI_HSTX_CKL_WC);
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

	writel(tmp_reg, dsi->dsi_reg_base + DSI_TXRX_CTRL);
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
	writel(tmp_reg, dsi->dsi_reg_base + DSI_PSCTRL);
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

	writel(vm->vsync_len, dsi->dsi_reg_base + DSI_VSA_NL);
	writel(vm->vback_porch, dsi->dsi_reg_base + DSI_VBP_NL);
	writel(vm->vfront_porch, dsi->dsi_reg_base + DSI_VFP_NL);
	writel(vm->vactive, dsi->dsi_reg_base + DSI_VACT_NL);

	horizontal_sync_active_byte = (vm->hsync_len * dsi_tmp_buf_bpp - 10);

	if (dsi->mode_flags & MIPI_DSI_MODE_VIDEO_SYNC_PULSE)
		horizontal_backporch_byte =
			(vm->hback_porch * dsi_tmp_buf_bpp - 10);
	else
		horizontal_backporch_byte = ((vm->hback_porch + vm->hsync_len) *
			dsi_tmp_buf_bpp - 10);

	horizontal_frontporch_byte = (vm->hfront_porch * dsi_tmp_buf_bpp - 12);

	writel(horizontal_sync_active_byte, dsi->dsi_reg_base + DSI_HSA_WC);
	writel(horizontal_backporch_byte, dsi->dsi_reg_base + DSI_HBP_WC);
	writel(horizontal_frontporch_byte, dsi->dsi_reg_base + DSI_HFP_WC);

	dsi_ps_control(dsi);
}

static void dsi_set_interrupt_enable(struct mtk_dsi *dsi)
{
	writel(0x0000003f, dsi->dsi_reg_base + DSI_INTEN);
}

static void mtk_dsi_start(struct mtk_dsi *dsi)
{
	writel(0, dsi->dsi_reg_base + DSI_START);
	writel(1, dsi->dsi_reg_base + DSI_START);
}

static void mtk_dsi_poweroff(struct mtk_dsi *dsi)
{
	clk_disable_unprepare(dsi->dsi0_engine_clk_cg);
	clk_disable_unprepare(dsi->dsi0_digital_clk_cg);

	dsi_phy_clk_switch_off(dsi);
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
	dsi_set_cmd_mode(dsi);
	dsi_rxtx_control(dsi);
	dsi_clk_ulp_mode_leave(dsi);
	dsi_lane0_ulp_mode(dsi, 0);
	dsi_clk_hs_mode(dsi, 0);
	dsi_ps_control_vact(dsi);
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

	dsi_lane0_ulp_mode(dsi, 1);
	dsi_clk_ulp_mode_enter(dsi);
	mtk_dsi_poweroff(dsi);
	dsi_phy_clk_switch_off(dsi);

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

static void mtk_dsi_encoder_commit(struct drm_encoder *encoder)
{
	struct mtk_dsi *dsi = encoder_to_dsi(encoder);

	mtk_output_dsi_enable(dsi);
}

static enum drm_connector_status
mtk_dsi_connector_detect(struct drm_connector *connector, bool force)
{
	struct mtk_dsi *dsi = connector_to_dsi(connector);

	DRM_INFO("dsi connector detect panel = 0x%p, panel node = 0x%p\n",
		dsi->panel, dsi->panel_node);

	if (!dsi->panel) {
		dsi->panel = of_drm_find_panel(dsi->panel_node);
		DRM_INFO("dsi connector detect panel 0x%p\n", dsi->panel);
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

static const struct drm_display_mode default_modes[] = {
	/* 1368x768@60Hz */
	{ DRM_MODE("1080x1920", DRM_MODE_TYPE_DRIVER, 148500,
	1080, 1080 + 58, 1080 + 58 + 58, 1080 + 58 + 58 + 58, 0,
	1920, 1920 + 4, 1920 + 4 + 4, 1920 + 4 + 4 + 4, 0, 0) },
};

static int mtk_dsi_connector_get_modes(struct drm_connector *connector)
{
	struct mtk_dsi *dsi = connector_to_dsi(connector);

	if (dsi->panel)
		return dsi->panel->funcs->get_modes(dsi->panel);

	return 0;
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
};

static const struct drm_connector_funcs mtk_dsi_connector_funcs = {
	.dpms = drm_helper_connector_dpms,
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

struct bridge_init {
	struct i2c_client *mipirx_client;
	struct i2c_client *dptx_client;
	struct device_node *node_mipirx;
	struct device_node *node_dptx;
};

static int mtk_dsi_create_conn_enc(struct mtk_dsi *dsi)
{
	int ret;

	ret = drm_encoder_init(
			dsi->drm_dev, &dsi->encoder,
			&mtk_dsi_encoder_funcs, DRM_MODE_ENCODER_DSI
			);

	if (ret) {
		DRM_ERROR("Failed to encoder init to drm\n");
		return ret;
	}

	drm_encoder_helper_add(&dsi->encoder, &mtk_dsi_encoder_helper_funcs);

	dsi->encoder.possible_crtcs = 1;

	ret = drm_connector_init(dsi->drm_dev, &dsi->conn,
				 &mtk_dsi_connector_funcs,
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
		mtk_dsi_reset(dsi);
	}
	#else
	while (timeout_ms--) {
		i = readl(dsi->dsi_reg_base + DSI_INTSTA);
		if (!(i & BIT(31)))
			break;

		usleep_range(1, 2);
	}
	#endif

	if (msg->tx_len > 1) {
		t2.config = 2;
		t2.type = msg->type;
		t2.wc16 = msg->tx_len;

		reg_val = (t2.wc16 << 16) | (t2.type << 8) | t2.config;

		writel(reg_val, dsi->dsi_reg_base + DSI_CMDQ0);

		for (i = 0; i < msg->tx_len; i++) {
			goto_addr = DSI_CMDQ0 + i;
			mask_para = (0xff << ((goto_addr & 0x3) * 8));
			set_para = (tx_buf[i] << ((goto_addr & 0x3) * 8));
			mtk_dsi_tx_mask(dsi, goto_addr & (~0x3), mask_para,
					set_para);
		}

		mtk_dsi_tx_mask(dsi, DSI_CMDQ_SIZE, CMDQ_SIZE,
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

		writel(reg_val, dsi->dsi_reg_base + DSI_CMDQ0);
		mtk_dsi_tx_mask(dsi, DSI_CMDQ_SIZE, CMDQ_SIZE, 1);
	}

	mtk_dsi_start(dsi);

	return 0;
}

static const struct mipi_dsi_host_ops mtk_dsi_ops = {
	.attach = mtk_dsi_host_attach,
	.detach = mtk_dsi_host_detach,
	.transfer = mtk_dsi_host_transfer,
};

static int mtk_dsi_bind(struct device *dev, struct device *master, void *data)
{
	int ret;
	struct mtk_dsi *dsi = NULL;

	dsi = platform_get_drvdata(to_platform_device(dev));

	if (!dsi) {
		ret = -EFAULT;
		DRM_ERROR("Failed to get driver data from dsi device\n");
		goto errcode;
	}

	dsi->drm_dev = data;

	ret = mtk_dsi_create_conn_enc(dsi);
	if (ret) {
		DRM_ERROR("Encoder create failed with %d\n", ret);
		return ret;
	}

	return 0;

errcode:
	return ret;
}

static void mtk_dsi_unbind(
		struct device *dev, struct device *master,
		void *data
		)
{
	struct mtk_dsi *dsi = NULL;

	dsi = platform_get_drvdata(to_platform_device(dev));
	mtk_dsi_destroy_conn_enc(dsi);
	mipi_dsi_host_unregister(&dsi->host);

	dsi->drm_dev = NULL;
}

static const struct component_ops mtk_dsi_component_ops = {
	.bind = mtk_dsi_bind,
	.unbind = mtk_dsi_unbind,
};

/* of_* functions will be removed after merge of of_graph patches */
static struct device_node *
of_get_child_by_name_reg(struct device_node *parent, const char *name, u32 reg)
{
	struct device_node *np;

	for_each_child_of_node(parent, np) {
		u32 r;

		if (!np->name || of_node_cmp(np->name, name))
			continue;

		if (of_property_read_u32(np, "reg", &r) < 0)
			r = 0;

		if (reg == r)
			break;
	}

	return np;
}

static struct device_node *of_graph_get_port_by_reg(struct device_node *parent,
						    u32 reg)
{
	struct device_node *ports, *port;

	ports = of_get_child_by_name(parent, "ports");
	if (ports)
		parent = ports;

	port = of_get_child_by_name_reg(parent, "port", reg);

	of_node_put(ports);

	return port;
}

static struct device_node *
of_graph_get_endpoint_by_reg(struct device_node *port, u32 reg)
{
	return of_get_child_by_name_reg(port, "endpoint", reg);
}

enum {
	DSI_PORT_IN,
	DSI_PORT_OUT
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

	status = readl(dsi->dsi_reg_base + DSI_INTSTA);

	if (status & BIT(0)) {
		/* write clear RD_RDY interrupt */
		/* write clear RD_RDY interrupt must be before DSI_RACK */
		/* because CMD_DONE will raise after DSI_RACK, */
		/* so write clear RD_RDY after that will clear CMD_DONE too */
		do {
			/* send read ACK */
			mtk_dsi_mask(dsi, DSI_RACK,	BIT(0), BIT(0));
			tmp = readl(dsi->dsi_reg_base + DSI_INTSTA);
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
	struct mtk_dsi *dsi = NULL;
	struct device *dev = &pdev->dev;
	struct device_node *port, *ep, *node = dev->of_node;
	struct resource *regs;
	int ret;

	dsi = devm_kzalloc(dev, sizeof(*dsi), GFP_KERNEL);
	dsi->mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_SYNC_PULSE;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->lanes = 4;

	port = of_graph_get_port_by_reg(node, DSI_PORT_OUT);
	if (!port) {
		dev_err(dev, "no output port specified\n");
		return -EINVAL;
	}

	ep = of_graph_get_endpoint_by_reg(port, 0);
	of_node_put(port);
	if (!ep) {
		dev_err(dev, "no endpoint specified in output port\n");
		return -EINVAL;
	}

	of_node_put(ep);

	dsi->dsi0_engine_clk_cg = devm_clk_get(dev, "dsi0_engine_disp_ck");
	if (IS_ERR(dsi->dsi0_engine_clk_cg)) {
		dev_err(dev, "cannot get dsi0_engine_clk_cg\n");
		return PTR_ERR(dsi->dsi0_engine_clk_cg);
	}

	dsi->dsi0_digital_clk_cg = devm_clk_get(dev, "dsi0_digital_disp_ck");
	if (IS_ERR(dsi->dsi0_digital_clk_cg)) {
		dev_err(dev, "cannot get dsi0_digital_disp_ck\n");
		return PTR_ERR(dsi->dsi0_digital_clk_cg);
	}

	regs = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	dsi->dsi_reg_base = devm_ioremap_resource(dev, regs);
	if (IS_ERR(dsi->dsi_reg_base)) {
		dev_err(dev, "cannot get dsi->dsi_reg_base\n");
		return PTR_ERR(dsi->dsi_reg_base);
	}

	regs = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	dsi->dsi_tx_reg_base = devm_ioremap_resource(dev, regs);
	if (IS_ERR(dsi->dsi_tx_reg_base)) {
		dev_err(dev, "cannot get dsi->dsi_tx_reg_base\n");
		return PTR_ERR(dsi->dsi_tx_reg_base);
	}

	dsi->host.ops = &mtk_dsi_ops;
	dsi->host.dev = dev;
	mipi_dsi_host_register(&dsi->host);

	#ifdef INTERRUPT_MODE
	dsi->irq = platform_get_irq(pdev, 0);
	if (dsi->irq < 0) {
		dev_err(&pdev->dev, "failed to request dsi irq resource\n");
		ret = dsi->irq;
		return -EPROBE_DEFER;
	}

	irq_set_status_flags(dsi->irq, IRQ_TYPE_LEVEL_LOW);
	ret = devm_request_irq(&pdev->dev, dsi->irq,
					mediatek_dsi_irq, IRQF_TRIGGER_LOW,
					dev_name(&pdev->dev), dsi);
	if (ret) {
		dev_err(&pdev->dev, "failed to request mediatek dsi irq\n");
		return -EPROBE_DEFER;
	} else {
		DRM_INFO("dsi irq num is 0x%x\n", dsi->irq);
	}

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
		dev_err(dev, "cannot get dsi->dsi_tx_reg_base\n");
		return -EPROBE_DEFER;
	}

	DRM_INFO("dsi_reg_base = 0x%x, dsi_tx_reg_base = 0x%x\n",
		(unsigned int)dsi->dsi_reg_base, (unsigned int)dsi->dsi_tx_reg_base);

	return 0;
}

static int mtk_dsi_remove(struct platform_device *pdev)
{
	struct mtk_dsi *dsi = platform_get_drvdata(pdev);

	mtk_output_dsi_disable(dsi);
	component_del(&pdev->dev, &mtk_dsi_component_ops);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int mtk_dsi_suspend(struct device *dev)
{
	struct mtk_dsi *dsi = NULL;
	int ret = 0;

	dsi = dev_get_drvdata(dev);
	if (!dsi) {
		DRM_ERROR("dsi is null\n");
		ret = -EINVAL;
		goto errcode;
	}

	mtk_output_dsi_disable(dsi);
	DRM_INFO("dsi suspend success!\n");

	return 0;

errcode:
	DRM_ERROR("dsi suspend failed!\n");
	return ret;
}

static int mtk_dsi_resume(struct device *dev)
{
	struct mtk_dsi *dsi = NULL;
	int ret = 0;

	dsi = dev_get_drvdata(dev);
	if (!dsi) {
		DRM_ERROR("dsi is null\n");
		ret = -EINVAL;
		goto errcode;
	}

	mtk_output_dsi_enable(dsi);
	DRM_INFO("dsi resume success!\n");

	return 0;

errcode:
	DRM_ERROR("dsi resume failed!\n");
	return ret;
}

static const struct dev_pm_ops mediatek_dsi_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(mtk_dsi_suspend, mtk_dsi_resume)
};

#endif

static const struct of_device_id mtk_dsi_of_match[] = {
	{ .compatible = "mediatek,mt2701-dsi" },
	{ },
};

MODULE_DEVICE_TABLE(of, mtk_dsi_of_match);

struct platform_driver mtk_dsi_driver = {
	.probe = mtk_dsi_probe,
	.remove = mtk_dsi_remove,
	.driver = {
		.name = "mtk-dsi",
		.of_match_table = mtk_dsi_of_match,
		.owner = THIS_MODULE,
#ifdef CONFIG_PM_SLEEP
		.pm = &mediatek_dsi_pm_ops,
#endif
	},
};
