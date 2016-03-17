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
#ifndef _MEDDIATEK_HDMI_DISPLAY_CORE_H
#define _MEDDIATEK_HDMI_DISPLAY_CORE_H

#include <linux/types.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/hdmi.h>
#include <linux/kernel.h>
#include <linux/kern_levels.h>
#include <drm/drmP.h>
#include <drm/drm_crtc.h>
#include <drm/drm_mode.h>
#include <drm/drm_modes.h>

enum HDMI_DISPLAY_COLOR_DEPTH {
	HDMI_DEEP_COLOR_24BITS,
	HDMI_DEEP_COLOR_30BITS,
	HDMI_DEEP_COLOR_36BITS,
	HDMI_DEEP_COLOR_48BITS,
};

struct mediatek_hdmi_display_ops {
	int (*init)(void *private_data);
	int (*set_format)(struct drm_display_mode *display_mode,
			  void *private_data);
	int (*set_deepcolor)(enum HDMI_DISPLAY_COLOR_DEPTH depth,
			     void *private_data);
	int (*set_colorspace)(enum hdmi_colorspace csp, void *private_data);
};

struct mediatek_hdmi_display_node {
	struct list_head node;
	struct device_node *np;
	void *private_data;
	struct drm_display_mode mode;
	struct mediatek_hdmi_display_ops *display_ops;
	struct mediatek_hdmi_display_node *pre_node;
	struct mediatek_hdmi_display_node *next_node;
};

extern struct platform_driver mediate_hdmi_dpi_driver;
extern struct platform_driver mediatek_hdmi_ddc_driver;
bool mtk_hdmi_display_init(struct mediatek_hdmi_display_node *display_node);
struct mediatek_hdmi_display_node *
	mtk_hdmi_display_create_node(
	struct mediatek_hdmi_display_ops *display_ops,
		struct device_node *np, void *data);
void mtk_hdmi_display_del_node(struct mediatek_hdmi_display_node
			       *display_node);
int mtk_hdmi_display_add_pre_node(struct mediatek_hdmi_display_node *node,
				  struct mediatek_hdmi_display_node *pre_node);
bool mtk_hdmi_display_set_vid_format(struct mediatek_hdmi_display_node *node,
				     struct drm_display_mode *mode);
struct mediatek_hdmi_display_node *mtk_hdmi_display_find_node(
	struct device_node *np);

extern struct platform_driver mediate_hdmi_dpi_driver;
extern struct platform_driver mediatek_hdmi_ddc_driver;

#define mtk_hdmi_err(fmt, ...) \
	pr_err("[mediatek drm hdmi] ERROR!!! fun:%s, line:%d  " \
	fmt, __func__, __LINE__,  ##__VA_ARGS__)
#define mtk_hdmi_info(fmt, ...) \
	pr_info("[mediatek drm hdmi] INFO fun:%s, line:%d  " \
	fmt, __func__, __LINE__,  ##__VA_ARGS__)
#define mtk_hdmi_output(fmt, ...) \
		pr_info(fmt, ##__VA_ARGS__)

#endif /*  */
