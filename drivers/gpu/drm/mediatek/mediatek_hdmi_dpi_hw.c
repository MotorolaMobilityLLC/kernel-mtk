/*
 * Copyright (c) 2014 MediaTek Inc.
 * Author: Jie Qiu <jie.qiu@mediatek.com>
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
#include "mediatek_hdmi_dpi_regs.h"
#include "mediatek_hdmi_dpi_ctrl.h"
#include "mediatek_hdmi_dpi_hw.h"

#define hdmi_dpi_read(addr) readl(dpi->regs + addr)
#define hdmi_dpi_write(addr, val) writel(val, dpi->regs + addr)
#define hdmi_dpi_write_mask(addr, val, mask) \
	hdmi_dpi_write((addr), \
	(hdmi_dpi_read(addr) & (~(mask))) | ((val) & (mask)))

void hdmi_dpi_phy_sw_reset(struct mediatek_hdmi_dpi_context *dpi, bool reset)
{
	hdmi_dpi_write_mask(DPI_RET, reset << RST, RST_MASK);
}

void hdmi_dpi_phy_enable(struct mediatek_hdmi_dpi_context *dpi)
{
	hdmi_dpi_write_mask(DPI_EN, 1 << EN, EN_MASK);
}

void hdmi_dpi_phy_disable(struct mediatek_hdmi_dpi_context *dpi)
{
	hdmi_dpi_write_mask(DPI_EN, 0 << EN, EN_MASK);
}

void hdmi_dpi_phy_config_hsync(struct mediatek_hdmi_dpi_context *dpi,
			       struct mediatek_hdmi_dpi_sync_param *sync)
{
	hdmi_dpi_write_mask(DPI_TGEN_HWIDTH, sync->sync_width << HPW, HPW_MASK);
	hdmi_dpi_write_mask(DPI_TGEN_HPORCH, sync->back_porch << HBP, HBP_MASK);
	hdmi_dpi_write_mask(DPI_TGEN_HPORCH, sync->front_porch << HFP,
			    HFP_MASK);
}

void hdmi_dpi_phy_config_vsync_lodd(struct mediatek_hdmi_dpi_context *dpi,
				    struct mediatek_hdmi_dpi_sync_param *sync)
{
	hdmi_dpi_write_mask(DPI_TGEN_VWIDTH, sync->sync_width << VPW, VPW_MASK);
	hdmi_dpi_write_mask(DPI_TGEN_VWIDTH, sync->shift_half_line << VPW_HALF,
			    VPW_HALF_MASK);
	hdmi_dpi_write_mask(DPI_TGEN_VPORCH, sync->back_porch << VBP, VBP_MASK);
	hdmi_dpi_write_mask(DPI_TGEN_VPORCH, sync->front_porch << VFP,
			    VFP_MASK);
}

void hdmi_dpi_phy_config_vsync_leven(struct mediatek_hdmi_dpi_context *dpi,
				     struct mediatek_hdmi_dpi_sync_param *sync)
{
	hdmi_dpi_write_mask(DPI_TGEN_VWIDTH_LEVEN,
			    sync->sync_width << VPW_LEVEN, VPW_LEVEN_MASK);
	hdmi_dpi_write_mask(DPI_TGEN_VWIDTH_LEVEN,
			    sync->shift_half_line << VPW_HALF_LEVEN,
			    VPW_HALF_LEVEN_MASK);
	hdmi_dpi_write_mask(DPI_TGEN_VPORCH_LEVEN,
			    sync->back_porch << VBP_LEVEN, VBP_LEVEN_MASK);
	hdmi_dpi_write_mask(DPI_TGEN_VPORCH_LEVEN,
			    sync->front_porch << VFP_LEVEN, VFP_LEVEN_MASK);
}

void hdmi_dpi_phy_config_vsync_rodd(struct mediatek_hdmi_dpi_context *dpi,
				    struct mediatek_hdmi_dpi_sync_param *sync)
{
	hdmi_dpi_write_mask(DPI_TGEN_VWIDTH_RODD, sync->sync_width << VPW_RODD,
			    VPW_RODD_MASK);
	hdmi_dpi_write_mask(DPI_TGEN_VWIDTH_RODD,
			    sync->shift_half_line << VPW_HALF_RODD,
			    VPW_HALF_RODD_MASK);
	hdmi_dpi_write_mask(DPI_TGEN_VPORCH_RODD, sync->back_porch << VBP_RODD,
			    VBP_RODD_MASK);
	hdmi_dpi_write_mask(DPI_TGEN_VPORCH_RODD, sync->front_porch << VFP_RODD,
			    VFP_RODD_MASK);
}

void hdmi_dpi_phy_config_vsync_reven(struct mediatek_hdmi_dpi_context *dpi,
				     struct mediatek_hdmi_dpi_sync_param *sync)
{
	hdmi_dpi_write_mask(DPI_TGEN_VWIDTH_REVEN,
			    sync->sync_width << VPW_REVEN, VPW_REVEN_MASK);
	hdmi_dpi_write_mask(DPI_TGEN_VWIDTH_REVEN,
			    sync->shift_half_line << VPW_HALF_REVEN,
			    VPW_HALF_REVEN_MASK);
	hdmi_dpi_write_mask(DPI_TGEN_VPORCH_REVEN,
			    sync->back_porch << VBP_REVEN, VBP_REVEN_MASK);
	hdmi_dpi_write_mask(DPI_TGEN_VPORCH_REVEN,
			    sync->front_porch << VFP_REVEN, VFP_REVEN_MASK);
}

void hdmi_dpi_phy_config_pol(struct mediatek_hdmi_dpi_context *dpi,
			     struct mediatek_hdmi_dpi_polarity *dpi_pol)
{
	hdmi_dpi_write_mask(DPI_OUTPUT_SETTING, dpi_pol->ck_pol << CK_POL,
			    CK_POL_MASK);
	hdmi_dpi_write_mask(DPI_OUTPUT_SETTING, dpi_pol->de_pol << DE_POL,
			    DE_POL_MASK);
	hdmi_dpi_write_mask(DPI_OUTPUT_SETTING, dpi_pol->hsync_pol << HSYNC_POL,
			    HSYNC_POL_MASK);
	hdmi_dpi_write_mask(DPI_OUTPUT_SETTING, dpi_pol->vsync_pol << VSYNC_POL,
			    VSYNC_POL_MASK);
}

void hdmi_dpi_phy_config_3d(struct mediatek_hdmi_dpi_context *dpi, bool en_3d)
{
	hdmi_dpi_write_mask(DPI_CON, en_3d << TDFP_EN, TDFP_EN_MASK);
}

void hdmi_dpi_phy_config_interface(struct mediatek_hdmi_dpi_context *dpi,
				   bool inter)
{
	hdmi_dpi_write_mask(DPI_CON, inter << INTL_EN, INTL_EN_MASK);
}

void hdmi_dpi_phy_config_fb_size(struct mediatek_hdmi_dpi_context *dpi,
				 u32 width, u32 height)
{
	hdmi_dpi_write_mask(DPI_SIZE, width << HSIZE, HSIZE_MASK);
	hdmi_dpi_write_mask(DPI_SIZE, height << VSIZE, VSIZE_MASK);
}

void hdmi_dpi_phy_config_channel_limit(struct mediatek_hdmi_dpi_context *dpi,
				       struct mediatek_hdmi_dpi_yc_limit *limit)
{
	hdmi_dpi_write_mask(DPI_Y_LIMIT, limit->y_bottom << Y_LIMINT_BOT,
			    Y_LIMINT_BOT_MASK);
	hdmi_dpi_write_mask(DPI_Y_LIMIT, limit->y_top << Y_LIMINT_TOP,
			    Y_LIMINT_TOP_MASK);
	hdmi_dpi_write_mask(DPI_C_LIMIT, limit->c_bottom << C_LIMIT_BOT,
			    C_LIMIT_BOT_MASK);
	hdmi_dpi_write_mask(DPI_C_LIMIT, limit->c_top << C_LIMIT_TOP,
			    C_LIMIT_TOP_MASK);
}

void hdmi_dpi_phy_config_bit_swap(struct mediatek_hdmi_dpi_context *dpi,
				  bool swap)
{
	hdmi_dpi_write_mask(DPI_OUTPUT_SETTING, swap << BIT_SWAP,
			    BIT_SWAP_MASK);
}

void hdmi_dpi_phy_config_bit_num(struct mediatek_hdmi_dpi_context *dpi,
				 enum HDMI_DPI_OUT_BIT_NUM num)
{
	if (num == HDMI_DPI_OUT_BIT_NUM_8BITS)
		hdmi_dpi_write_mask(DPI_OUTPUT_SETTING, OUT_BIT_8 << OUT_BIT,
				    OUT_BIT_MASK);
	else if (num == HDMI_DPI_OUT_BIT_NUM_10BITS)
		hdmi_dpi_write_mask(DPI_OUTPUT_SETTING, OUT_BIT_10 << OUT_BIT,
				    OUT_BIT_MASK);
	else if (num == HDMI_DPI_OUT_BIT_NUM_12BITS)
		hdmi_dpi_write_mask(DPI_OUTPUT_SETTING, OUT_BIT_12 << OUT_BIT,
				    OUT_BIT_MASK);
	else if (num == HDMI_DPI_OUT_BIT_NUM_16BITS)
		hdmi_dpi_write_mask(DPI_OUTPUT_SETTING, OUT_BIT_16 << OUT_BIT,
				    OUT_BIT_MASK);
}

void hdmi_dpi_phy_config_yc_map(struct mediatek_hdmi_dpi_context *dpi,
				enum HDMI_DPI_OUT_YC_MAP map)
{
	if (map == HDMI_DPI_OUT_YC_MAP_RGB)
		hdmi_dpi_write_mask(DPI_OUTPUT_SETTING, YC_MAP_RGB << YC_MAP,
				    YC_MAP_MASK);
	if (map == HDMI_DPI_OUT_YC_MAP_CYCY)
		hdmi_dpi_write_mask(DPI_OUTPUT_SETTING, YC_MAP_CYCY << YC_MAP,
				    YC_MAP_MASK);
	if (map == HDMI_DPI_OUT_YC_MAP_YCYC)
		hdmi_dpi_write_mask(DPI_OUTPUT_SETTING, YC_MAP_YCYC << YC_MAP,
				    YC_MAP_MASK);
	if (map == HDMI_DPI_OUT_YC_MAP_CY)
		hdmi_dpi_write_mask(DPI_OUTPUT_SETTING, YC_MAP_CY << YC_MAP,
				    YC_MAP_MASK);
	if (map == HDMI_DPI_OUT_YC_MAP_YC)
		hdmi_dpi_write_mask(DPI_OUTPUT_SETTING, YC_MAP_YC << YC_MAP,
				    YC_MAP_MASK);
}

void hdmi_dpi_phy_config_channel_swap(struct mediatek_hdmi_dpi_context *dpi,
				      enum HDMI_DPI_OUT_CHANNEL_SWAP swap)
{
	if (swap == HDMI_DPI_OUT_CHANNEL_SWAP_RGB)
		hdmi_dpi_write_mask(DPI_OUTPUT_SETTING, SWAP_RGB << CH_SWAP,
				    CH_SWAP_MASK);
	if (swap == HDMI_DPI_OUT_CHANNEL_SWAP_GBR)
		hdmi_dpi_write_mask(DPI_OUTPUT_SETTING, SWAP_GBR << CH_SWAP,
				    CH_SWAP_MASK);
	if (swap == HDMI_DPI_OUT_CHANNEL_SWAP_BRG)
		hdmi_dpi_write_mask(DPI_OUTPUT_SETTING, SWAP_BRG << CH_SWAP,
				    CH_SWAP_MASK);
	if (swap == HDMI_DPI_OUT_CHANNEL_SWAP_RBG)
		hdmi_dpi_write_mask(DPI_OUTPUT_SETTING, SWAP_RBG << CH_SWAP,
				    CH_SWAP_MASK);
	if (swap == HDMI_DPI_OUT_CHANNEL_SWAP_GRB)
		hdmi_dpi_write_mask(DPI_OUTPUT_SETTING, SWAP_GRB << CH_SWAP,
				    CH_SWAP_MASK);
	if (swap == HDMI_DPI_OUT_CHANNEL_SWAP_BGR)
		hdmi_dpi_write_mask(DPI_OUTPUT_SETTING, SWAP_BGR << CH_SWAP,
				    CH_SWAP_MASK);
}

void hdmi_dpi_phy_config_yuv422_enable(struct mediatek_hdmi_dpi_context *dpi,
				       bool enable)
{
	hdmi_dpi_write_mask(DPI_CON, enable << YUV422_EN, YUV422_EN_MASK);
}

void hdmi_dpi_phy_config_csc_enable(struct mediatek_hdmi_dpi_context *dpi,
				    bool enable)
{
	hdmi_dpi_write_mask(DPI_CON, enable << CSC_ENABLE, CSC_ENABLE_MASK);
}

void hdmi_dpi_phy_config_swap_input(struct mediatek_hdmi_dpi_context *dpi,
				    bool enable)
{
	hdmi_dpi_write_mask(DPI_CON, enable << IN_RB_SWAP, IN_RB_SWAP_MASK);
}

void hdmi_dpi_phy_config_2n_h_fre(struct mediatek_hdmi_dpi_context *dpi)
{
	hdmi_dpi_write_mask(DPI_H_FRE_CON, H_FRE_2N, H_FRE_MASK);
}

void hdmi_dpi_phy_config_color_format(struct mediatek_hdmi_dpi_context *dpi,
				      enum HDMI_DPI_OUT_COLOR_FORMAT format)
{
	if ((HDMI_DPI_COLOR_FORMAT_YCBCR_444 == format) ||
	    (HDMI_DPI_COLOR_FORMAT_YCBCR_444_FULL == format)) {
		hdmi_dpi_phy_config_yuv422_enable(dpi, false);
		hdmi_dpi_phy_config_csc_enable(dpi, true);
		hdmi_dpi_phy_config_swap_input(dpi, false);
		hdmi_dpi_phy_config_channel_swap(dpi,
						 HDMI_DPI_OUT_CHANNEL_SWAP_BGR);
	} else if ((HDMI_DPI_COLOR_FORMAT_YCBCR_422 == format) ||
		   (HDMI_DPI_COLOR_FORMAT_YCBCR_422_FULL == format)) {
		hdmi_dpi_phy_config_yuv422_enable(dpi, true);
		hdmi_dpi_phy_config_csc_enable(dpi, true);
		hdmi_dpi_phy_config_swap_input(dpi, true);
		hdmi_dpi_phy_config_channel_swap(dpi,
						 HDMI_DPI_OUT_CHANNEL_SWAP_RGB);
	} else {
		hdmi_dpi_phy_config_yuv422_enable(dpi, false);
		hdmi_dpi_phy_config_csc_enable(dpi, false);
		hdmi_dpi_phy_config_swap_input(dpi, false);
		hdmi_dpi_phy_config_channel_swap(dpi,
						 HDMI_DPI_OUT_CHANNEL_SWAP_RGB);
	}
}
