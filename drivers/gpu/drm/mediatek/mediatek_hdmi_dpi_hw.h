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
#ifndef _MEDIATEK_HDMI_DPI_HW_H
#define _MEDIATEK_HDMI_DPI_HW_H

#include <linux/types.h>

enum HDMI_DPI_POLARITY {
	HDMI_DPI_POLARITY_RISING,
	HDMI_DPI_POLARITY_FALLING,
};

struct mediatek_hdmi_dpi_polarity {
	enum HDMI_DPI_POLARITY de_pol;
	enum HDMI_DPI_POLARITY ck_pol;
	enum HDMI_DPI_POLARITY hsync_pol;
	enum HDMI_DPI_POLARITY vsync_pol;
};

struct mediatek_hdmi_dpi_sync_param {
	u32 sync_width;
	u32 front_porch;
	u32 back_porch;
	bool shift_half_line;
};

struct mediatek_hdmi_dpi_yc_limit {
	u16 y_top;
	u16 y_bottom;
	u16 c_top;
	u16 c_bottom;
};

void hdmi_dpi_phy_sw_reset(struct mediatek_hdmi_dpi_context *dpi,
			   bool reset);
void hdmi_dpi_phy_enable(struct mediatek_hdmi_dpi_context *dpi);
void hdmi_dpi_phy_disable(struct mediatek_hdmi_dpi_context *dpi);
void hdmi_dpi_phy_config_hsync(struct mediatek_hdmi_dpi_context *dpi,
			       struct mediatek_hdmi_dpi_sync_param *sync);
void hdmi_dpi_phy_config_vsync_lodd(struct mediatek_hdmi_dpi_context *dpi,
				    struct mediatek_hdmi_dpi_sync_param *sync);
void hdmi_dpi_phy_config_vsync_leven(struct mediatek_hdmi_dpi_context *dpi,
				     struct mediatek_hdmi_dpi_sync_param
				     *sync);
void hdmi_dpi_phy_config_vsync_rodd(struct mediatek_hdmi_dpi_context *dpi,
				    struct mediatek_hdmi_dpi_sync_param *sync);
void hdmi_dpi_phy_config_vsync_reven(struct mediatek_hdmi_dpi_context *dpi,
				     struct mediatek_hdmi_dpi_sync_param
				     *sync);
void hdmi_dpi_phy_config_pol(struct mediatek_hdmi_dpi_context *dpi,
			     struct mediatek_hdmi_dpi_polarity *dpi_pol);
void hdmi_dpi_phy_config_3d(struct mediatek_hdmi_dpi_context *dpi, bool en_3d);
void hdmi_dpi_phy_config_interface(struct mediatek_hdmi_dpi_context *dpi,
				   bool inter);
void hdmi_dpi_phy_config_fb_size(struct mediatek_hdmi_dpi_context *dpi,
				 u32 width, u32 height);
void hdmi_dpi_phy_config_channel_limit(struct mediatek_hdmi_dpi_context *dpi,
				       struct mediatek_hdmi_dpi_yc_limit
				       *limit);
void hdmi_dpi_phy_config_bit_swap(struct mediatek_hdmi_dpi_context *dpi,
				  bool swap);
void hdmi_dpi_phy_config_bit_num(struct mediatek_hdmi_dpi_context *dpi,
				 enum HDMI_DPI_OUT_BIT_NUM num);
void hdmi_dpi_phy_config_yc_map(struct mediatek_hdmi_dpi_context *dpi,
				enum HDMI_DPI_OUT_YC_MAP map);
void hdmi_dpi_phy_config_channel_swap(struct mediatek_hdmi_dpi_context *dpi,
				      enum HDMI_DPI_OUT_CHANNEL_SWAP swap);
void hdmi_dpi_phy_config_yuv422_enable(struct mediatek_hdmi_dpi_context *dpi,
				       bool enable);
void hdmi_dpi_phy_config_csc_enable(struct mediatek_hdmi_dpi_context *dpi,
				    bool enable);
void hdmi_dpi_phy_config_swap_input(struct mediatek_hdmi_dpi_context *dpi,
				    bool enable);
void hdmi_dpi_phy_config_color_format(struct mediatek_hdmi_dpi_context *dpi,
				      enum HDMI_DPI_OUT_COLOR_FORMAT format);
void hdmi_dpi_phy_config_2n_h_fre(struct mediatek_hdmi_dpi_context *dpi);
#endif /*  */
