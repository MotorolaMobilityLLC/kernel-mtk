/*
 * Copyright (c) 2015 MediaTek Inc.
 * Author: Ming Hsiu Tsai <minghsiu.tsai@mediatek.com>
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

#ifndef __MTK_VCODEC_VDEC_CONV_H__
#define __MTK_VCODEC_VDEC_CONV_H__

#include <linux/platform_device.h>

enum vdec_conv_fmt {
	VDEC_CONV_FMT_VIDEO,
	VDEC_CONV_FMT_JPEG
};

int mtk_vdec_conv_create(struct platform_device *pdev, unsigned long *h_conv,
						     enum vdec_conv_fmt fmt);
int mtk_vdec_conv_destroy(unsigned long h_conv);
int mtk_vdec_conv_init(unsigned long h_conv);
int mtk_vdec_conv_deinit(unsigned long h_conv);
int mtk_vdec_conv_nv12(unsigned long h_conv, uint64_t y_srcaddr,
			uint64_t uv_srcaddr, uint32_t width,
			uint32_t height, uint64_t dstaddr);
int mtk_vdec_conv_i420(unsigned long h_conv, uint64_t y_srcaddr,
			uint64_t u_srcaddr, uint64_t v_srcaddr,
			uint32_t src_width, uint32_t src_height,
			uint64_t y_dstaddr, uint64_t u_dstaddr,
			uint64_t v_dstaddr, uint32_t dst_width,
			uint32_t dst_height, bool src_is_422);

#endif
