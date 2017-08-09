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
#ifndef _MEDIATEK_HDMI_DPI_CTRL_H
#define _MEDIATEK_HDMI_DPI_CTRL_H

#include <linux/types.h>
#include <linux/clk.h>
#include <linux/spinlock.h>
#include <linux/irq.h>
#include "mediatek_hdmi_display_core.h"

enum HDMI_DPI_OUT_BIT_NUM {
	HDMI_DPI_OUT_BIT_NUM_8BITS,
	HDMI_DPI_OUT_BIT_NUM_10BITS,
	HDMI_DPI_OUT_BIT_NUM_12BITS,
	HDMI_DPI_OUT_BIT_NUM_16BITS
};

enum HDMI_DPI_OUT_YC_MAP {
	HDMI_DPI_OUT_YC_MAP_RGB,
	HDMI_DPI_OUT_YC_MAP_CYCY,
	HDMI_DPI_OUT_YC_MAP_YCYC,
	HDMI_DPI_OUT_YC_MAP_CY,
	HDMI_DPI_OUT_YC_MAP_YC
};

enum HDMI_DPI_OUT_CHANNEL_SWAP {
	HDMI_DPI_OUT_CHANNEL_SWAP_RGB,
	HDMI_DPI_OUT_CHANNEL_SWAP_GBR,
	HDMI_DPI_OUT_CHANNEL_SWAP_BRG,
	HDMI_DPI_OUT_CHANNEL_SWAP_RBG,
	HDMI_DPI_OUT_CHANNEL_SWAP_GRB,
	HDMI_DPI_OUT_CHANNEL_SWAP_BGR
};

enum HDMI_DPI_OUT_COLOR_FORMAT {
	HDMI_DPI_COLOR_FORMAT_RGB,
	HDMI_DPI_COLOR_FORMAT_RGB_FULL,
	HDMI_DPI_COLOR_FORMAT_YCBCR_444,
	HDMI_DPI_COLOR_FORMAT_YCBCR_422,
	HDMI_DPI_COLOR_FORMAT_XV_YCC,
	HDMI_DPI_COLOR_FORMAT_YCBCR_444_FULL,
	HDMI_DPI_COLOR_FORMAT_YCBCR_422_FULL
};

struct mediatek_hdmi_dpi_context {
	void __iomem *regs;
	struct clk *tvd_clk;
	int irq;
	spinlock_t	lock;/*spin lock for multi cpus*/
	bool power_on;
	struct mediatek_hdmi_display_node *display_node;
	struct drm_display_mode display_mode;
	enum HDMI_DPI_OUT_COLOR_FORMAT color_format;
	enum HDMI_DPI_OUT_YC_MAP yc_map;
	enum HDMI_DPI_OUT_BIT_NUM bit_num;
	enum HDMI_DPI_OUT_CHANNEL_SWAP channel_swap;
};

int mtk_hdmi_dpi_init(void *private_data);
int mtk_hdmi_dpi_set_display_mode(struct drm_display_mode *display_mode,
				  void *private_data);
#endif /*  */
