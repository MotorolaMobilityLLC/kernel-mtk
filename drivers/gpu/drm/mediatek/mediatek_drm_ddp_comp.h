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

#ifndef MEDIATEK_DRM_DDP_COMP_H
#define MEDIATEK_DRM_DDP_COMP_H

#define OVL_LAYER_NR	4

void mediatek_ovl_enable_vblank(void __iomem *disp_base);
void mediatek_ovl_disable_vblank(void __iomem *disp_base);
void mediatek_ovl_clear_vblank(void __iomem *disp_base);
void mediatek_ovl_config(void __iomem *ovl_base,
		unsigned int w, unsigned int h);
void mediatek_ovl_layer_on(void __iomem *ovl_base);
void mediatek_ovl_layer_off(void __iomem *ovl_base);
void mediatek_ovl_layer_config(void __iomem *ovl_base,
		unsigned int addr, unsigned int width, unsigned int height,
		unsigned int pitch,	unsigned int format);
void mediatek_ovl_layer_config_cursor(void __iomem *ovl_base,
		bool enable, unsigned int addr, int x, int y, int w, int h);

void main_disp_path_power_on(unsigned int width, unsigned int height,
		void __iomem *ovl_base,	void __iomem *rdma_base,
		void __iomem *color_base, void __iomem *ufoe_base,
		void __iomem *bls_base);
void ext_disp_path_power_on(unsigned int width, unsigned int height,
		void __iomem *ovl_base, void __iomem *rdma_base,
		void __iomem *color_base);
int disp_bls_set_backlight(unsigned int level);
void dump_bls_regs(void);

#endif /* MEDIATEK_DRM_DDP_COMP_H */

