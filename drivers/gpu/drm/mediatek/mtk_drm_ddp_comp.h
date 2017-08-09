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

#ifndef MTK_DRM_DDP_COMP_H
#define MTK_DRM_DDP_COMP_H

#include <linux/io.h>

struct device;
struct device_node;
struct drm_device;
struct mtk_plane_state;

enum mtk_ddp_comp_type {
	MTK_DISP_OVL,
	MTK_DISP_RDMA,
	MTK_DISP_WDMA,
	MTK_DISP_COLOR,
	MTK_DISP_AAL,
	MTK_DISP_GAMMA,
	MTK_DISP_UFOE,
	MTK_DSI,
	MTK_DPI,
	MTK_DISP_PWM,
	MTK_DISP_MUTEX,
	MTK_DISP_OD,
	MTK_DISP_BLS,
	MTK_DDP_COMP_TYPE_MAX,
};

enum mtk_ddp_comp_id {
	DDP_COMPONENT_AAL,
	DDP_COMPONENT_COLOR0,
	DDP_COMPONENT_COLOR1,
	DDP_COMPONENT_DPI0,
	DDP_COMPONENT_DSI0,
	DDP_COMPONENT_DSI1,
	DDP_COMPONENT_GAMMA,
	DDP_COMPONENT_OD,
	DDP_COMPONENT_OVL0,
	DDP_COMPONENT_OVL1,
	DDP_COMPONENT_PWM0,
	DDP_COMPONENT_PWM1,
	DDP_COMPONENT_RDMA0,
	DDP_COMPONENT_RDMA1,
	DDP_COMPONENT_RDMA2,
	DDP_COMPONENT_UFOE,
	DDP_COMPONENT_WDMA0,
	DDP_COMPONENT_WDMA1,
	DDP_COMPONENT_BLS,
	DDP_COMPONENT_ID_MAX,
};

struct mtk_ddp_comp_funcs {
	void (*config)(void __iomem *base, unsigned int w, unsigned int h,
		       unsigned int vrefresh, unsigned int fifo_pseudo_size);
	void (*power_on)(void __iomem *base);
	void (*power_off)(void __iomem *base);
	void (*enable_vblank)(void __iomem *base);
	void (*disable_vblank)(void __iomem *base);
	void (*clear_vblank)(void __iomem *base);
	void (*layer_on)(void __iomem *base, unsigned int idx);
	void (*layer_off)(void __iomem *base, unsigned int idx);
	void (*layer_config)(void __iomem *base, unsigned int ovl_addr,
			    unsigned int idx, struct mtk_plane_state *state,
			    unsigned int rgb888, unsigned int rgb565);
};

struct mtk_ddp_comp_driver_data {
	enum mtk_ddp_comp_type comp_type;
	unsigned int reg_ovl_addr;
	unsigned int rdma_fifo_pseudo_size;
	unsigned int ovl_infmt_rgb888;
	unsigned int ovl_infmt_rgb565;
};

struct mtk_ddp_comp {
	struct clk *clk;
	void __iomem *regs;
	int irq;
	struct device *larb_dev;
	enum mtk_ddp_comp_id id;
	const struct mtk_ddp_comp_funcs *funcs;
	struct mtk_ddp_comp_driver_data *ddp_comp_driver_data;
};

static inline void mtk_ddp_comp_config(struct mtk_ddp_comp *comp,
				       unsigned int w, unsigned int h,
				       unsigned int vrefresh)
{
	if (comp->funcs->config)
		comp->funcs->config(comp->regs, w, h, vrefresh,
		comp->ddp_comp_driver_data->rdma_fifo_pseudo_size);
}

static inline void mtk_ddp_comp_power_on(struct mtk_ddp_comp *comp)
{
	if (comp->funcs->power_on)
		comp->funcs->power_on(comp->regs);
}

static inline void mtk_ddp_comp_power_off(struct mtk_ddp_comp *comp)
{
	if (comp->funcs->power_off)
		comp->funcs->power_off(comp->regs);
}

static inline void mtk_ddp_comp_enable_vblank(struct mtk_ddp_comp *comp)
{
	if (comp->funcs->enable_vblank)
		comp->funcs->enable_vblank(comp->regs);
}

static inline void mtk_ddp_comp_disable_vblank(struct mtk_ddp_comp *comp)
{
	if (comp->funcs->disable_vblank)
		comp->funcs->disable_vblank(comp->regs);
}

static inline void mtk_ddp_comp_clear_vblank(struct mtk_ddp_comp *comp)
{
	if (comp->funcs->clear_vblank)
		comp->funcs->clear_vblank(comp->regs);
}

static inline void mtk_ddp_comp_layer_on(struct mtk_ddp_comp *comp,
					 unsigned int idx)
{
	if (comp->funcs->layer_on)
		comp->funcs->layer_on(comp->regs, idx);
}

static inline void mtk_ddp_comp_layer_off(struct mtk_ddp_comp *comp,
					  unsigned int idx)
{
	if (comp->funcs->layer_off)
		comp->funcs->layer_off(comp->regs, idx);
}

static inline void mtk_ddp_comp_layer_config(struct mtk_ddp_comp *comp,
					     unsigned int idx,
					     struct mtk_plane_state *state)
{
	if (comp->funcs->layer_config)
		comp->funcs->layer_config(comp->regs,
			comp->ddp_comp_driver_data->reg_ovl_addr, idx, state,
			comp->ddp_comp_driver_data->ovl_infmt_rgb888,
			comp->ddp_comp_driver_data->ovl_infmt_rgb565);
}

int mtk_ddp_comp_get_id(struct device_node *node,
			enum mtk_ddp_comp_type comp_type);
int mtk_ddp_comp_init(struct device *dev, struct device_node *comp_node,
		      struct mtk_ddp_comp *comp, enum mtk_ddp_comp_id comp_id);
int mtk_ddp_comp_register(struct drm_device *drm, struct mtk_ddp_comp *comp);
void mtk_ddp_comp_unregister(struct drm_device *drm, struct mtk_ddp_comp *comp);

#endif /* MTK_DRM_DDP_COMP_H */
