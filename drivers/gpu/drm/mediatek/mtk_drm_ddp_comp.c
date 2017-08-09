/*
 * Copyright (c) 2015 MediaTek Inc.
 * Authors:
 *	YT Shen <yt.shen@mediatek.com>
 *	CK Hu <ck.hu@mediatek.com>
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

#include <linux/clk.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <drm/drmP.h>
#include "mtk_drm_drv.h"
#include "mtk_drm_plane.h"
#include "mtk_drm_ddp_comp.h"

#define DISP_REG_OVL_INTEN			0x0004
#define OVL_FME_CPL_INT					BIT(1)
#define DISP_REG_OVL_INTSTA			0x0008
#define DISP_REG_OVL_EN				0x000c
#define DISP_REG_OVL_RST			0x0014
#define DISP_REG_OVL_ROI_SIZE			0x0020
#define DISP_REG_OVL_ROI_BGCLR			0x0028
#define DISP_REG_OVL_SRC_CON			0x002c
#define DISP_REG_OVL_CON(n)			(0x0030 + 0x20 * n)
#define DISP_REG_OVL_SRC_SIZE(n)		(0x0038 + 0x20 * n)
#define DISP_REG_OVL_OFFSET(n)			(0x003c + 0x20 * n)
#define DISP_REG_OVL_PITCH(n)			(0x0044 + 0x20 * n)
#define DISP_REG_OVL_RDMA_CTRL(n)		(0x00c0 + 0x20 * n)
#define DISP_REG_OVL_RDMA_GMC(n)		(0x00c8 + 0x20 * n)
#define DISP_REG_OVL_ADDR(n)			(0x0f40 + 0x20 * n)

#define DISP_REG_RDMA_INT_ENABLE		0x0000
#define DISP_REG_RDMA_INT_STATUS		0x0004
#define DISP_REG_RDMA_GLOBAL_CON		0x0010
#define DISP_REG_RDMA_SIZE_CON_0		0x0014
#define DISP_REG_RDMA_SIZE_CON_1		0x0018
#define DISP_REG_RDMA_FIFO_CON			0x0040
#define RDMA_FIFO_UNDERFLOW_EN				BIT(31)
#define RDMA_FIFO_PSEUDO_SIZE(bytes)			(((bytes) / 16) << 16)
#define RDMA_OUTPUT_VALID_FIFO_THRESHOLD(bytes)		((bytes) / 16)

#define DISP_REG_BLS_EN				0x0000
#define DISP_REG_BLS_SRC_SIZE			0x0038
#define DISP_REG_BLS_PWM_DUTY			0x00a0
#define DISP_REG_BLS_PWM_CON			0x0038


#define DISP_OD_EN				0x0000
#define DISP_OD_INTEN				0x0008
#define DISP_OD_INTSTA				0x000c
#define DISP_OD_CFG				0x0020
#define DISP_OD_SIZE				0x0030

#define DISP_REG_UFO_START			0x0000

#define DISP_COLOR_CFG_MAIN			0x0400
#define DISP_COLOR_START			0x0c00
#define DISP_COLOR_WIDTH			0x0c50
#define DISP_COLOR_HEIGHT			0x0c54

enum OVL_INPUT_FORMAT {
	OVL_INFMT_RGB565 = 0,
	OVL_INFMT_RGB888 = 1,
	OVL_INFMT_RGBA8888 = 2,
	OVL_INFMT_ARGB8888 = 3,
};

#define	OVL_RDMA_MEM_GMC	0x40402020
#define	OVL_AEN			BIT(8)
#define	OVL_ALPHA		0xff

#define	OD_RELAY_MODE		BIT(0)

#define	UFO_BYPASS		BIT(2)

#define	COLOR_BYPASS_ALL	BIT(7)
#define	COLOR_SEQ_SEL		BIT(13)

#define BLS_PWM_CLKDIV		BIT(16)
#define BLS_PWM_EN		BIT(16)

static void mtk_bls_config(void __iomem *bls_base, unsigned int w,
		unsigned int h, unsigned int vrefresh,
		unsigned int fifo_pseudo_size)
{
	writel(h << 16 | w, bls_base + DISP_REG_BLS_SRC_SIZE);
	writel(0, bls_base + DISP_REG_BLS_PWM_DUTY);
	writel(BLS_PWM_CLKDIV, bls_base + DISP_REG_BLS_PWM_CON);
	writel(BLS_PWM_EN, bls_base + DISP_REG_BLS_EN);
}

static void mtk_color_config(void __iomem *color_base, unsigned int w,
		unsigned int h, unsigned int vrefresh,
		unsigned int fifo_pseudo_size)
{
	writel(w, color_base + DISP_COLOR_WIDTH);
	writel(h, color_base + DISP_COLOR_HEIGHT);
}

static void mtk_color_start(void __iomem *color_base)
{
	writel(COLOR_BYPASS_ALL | COLOR_SEQ_SEL,
		color_base + DISP_COLOR_CFG_MAIN);
	writel(0x1, color_base + DISP_COLOR_START);
}

static void mtk_od_config(void __iomem *od_base, unsigned int w, unsigned int h,
		unsigned int vrefresh, unsigned int fifo_pseudo_size)
{
	writel(w << 16 | h, od_base + DISP_OD_SIZE);
}

static void mtk_od_start(void __iomem *od_base)
{
	writel(OD_RELAY_MODE, od_base + DISP_OD_CFG);
	writel(1, od_base + DISP_OD_EN);
}

static void mtk_ovl_enable_vblank(void __iomem *disp_base)
{
	writel(OVL_FME_CPL_INT, disp_base + DISP_REG_OVL_INTEN);
}

static void mtk_ovl_disable_vblank(void __iomem *disp_base)
{
	writel(0x0, disp_base + DISP_REG_OVL_INTEN);
}

static void mtk_ovl_clear_vblank(void __iomem *disp_base)
{
	writel(0x0, disp_base + DISP_REG_OVL_INTSTA);
}

static void mtk_ovl_start(void __iomem *ovl_base)
{
	writel(0x1, ovl_base + DISP_REG_OVL_EN);
}

static void mtk_ovl_config(void __iomem *ovl_base,
		unsigned int w, unsigned int h, unsigned int vrefresh,
		unsigned int fifo_pseudo_size)
{
	if (w != 0 && h != 0)
		writel(h << 16 | w, ovl_base + DISP_REG_OVL_ROI_SIZE);
	writel(0x0, ovl_base + DISP_REG_OVL_ROI_BGCLR);

	writel(0x1, ovl_base + DISP_REG_OVL_RST);
	writel(0x0, ovl_base + DISP_REG_OVL_RST);
}

static bool has_rb_swapped(unsigned int fmt)
{
	switch (fmt) {
	case DRM_FORMAT_BGR888:
	case DRM_FORMAT_BGR565:
	case DRM_FORMAT_ABGR8888:
	case DRM_FORMAT_XBGR8888:
	case DRM_FORMAT_BGRA8888:
	case DRM_FORMAT_BGRX8888:
		return true;
	default:
		return false;
	}
}

static unsigned int ovl_fmt_convert(unsigned int fmt,
					unsigned int rgb888,
					unsigned int rgb565)
{
	switch (fmt) {
	case DRM_FORMAT_RGB888:
	case DRM_FORMAT_BGR888:
		return rgb888;
	case DRM_FORMAT_RGB565:
	case DRM_FORMAT_BGR565:
		return rgb565;
	case DRM_FORMAT_RGBX8888:
	case DRM_FORMAT_RGBA8888:
	case DRM_FORMAT_BGRX8888:
	case DRM_FORMAT_BGRA8888:
		return OVL_INFMT_ARGB8888;
	case DRM_FORMAT_XRGB8888:
	case DRM_FORMAT_ARGB8888:
	case DRM_FORMAT_XBGR8888:
	case DRM_FORMAT_ABGR8888:
		return OVL_INFMT_RGBA8888;
	default:
		DRM_ERROR("unsupported format[%08x]\n", fmt);
		return OVL_INFMT_RGB888;
	}
}

static void mtk_ovl_layer_on(void __iomem *ovl_base, unsigned int idx)
{
	unsigned int reg;

	writel(0x1, ovl_base + DISP_REG_OVL_RDMA_CTRL(idx));
	writel(OVL_RDMA_MEM_GMC, ovl_base + DISP_REG_OVL_RDMA_GMC(idx));

	reg = readl(ovl_base + DISP_REG_OVL_SRC_CON);
	reg = reg | (1 << idx);
	writel(reg, ovl_base + DISP_REG_OVL_SRC_CON);
}

static void mtk_ovl_layer_off(void __iomem *ovl_base, unsigned int idx)
{
	unsigned int reg;

	reg = readl(ovl_base + DISP_REG_OVL_SRC_CON);
	reg = reg & ~(1 << idx);
	writel(reg, ovl_base + DISP_REG_OVL_SRC_CON);

	writel(0x0, ovl_base + DISP_REG_OVL_RDMA_CTRL(idx));
}

static void mtk_ovl_layer_config(void __iomem *ovl_base,
				unsigned int ovl_addr,
				unsigned int idx,
				struct mtk_plane_state *state,
				unsigned int rgb888,
				unsigned int rgb565)
{
	unsigned int addr = state->pending_addr;
	unsigned int pitch = state->pending_pitch & 0xffff;
	unsigned int fmt = state->pending_format;
	unsigned int offset = (state->pending_y << 16) | state->pending_x;
	unsigned int src_size = (state->pending_height << 16) |
				state->pending_width;
	unsigned int con;

	con = has_rb_swapped(fmt) << 24 |
		ovl_fmt_convert(fmt, rgb888, rgb565) << 12;
	if (idx != 0)
		con |= OVL_AEN | OVL_ALPHA;

	writel(con, ovl_base + DISP_REG_OVL_CON(idx));
	writel(pitch, ovl_base + DISP_REG_OVL_PITCH(idx));
	writel(src_size, ovl_base + DISP_REG_OVL_SRC_SIZE(idx));
	writel(offset, ovl_base + DISP_REG_OVL_OFFSET(idx));
	writel(addr, ovl_base + (ovl_addr+idx*0x20));
}

static void mtk_rdma_start(void __iomem *rdma_base)
{
	unsigned int reg;

	writel(0x4, rdma_base + DISP_REG_RDMA_INT_ENABLE);
	reg = readl(rdma_base + DISP_REG_RDMA_GLOBAL_CON);
	reg |= 1;
	writel(reg, rdma_base + DISP_REG_RDMA_GLOBAL_CON);
}

static void mtk_rdma_config(void __iomem *rdma_base,
		unsigned width, unsigned height, unsigned int vrefresh,
		unsigned int fifo_pseudo_size)
{
	unsigned int threshold;
	unsigned int reg;

	reg = readl(rdma_base + DISP_REG_RDMA_SIZE_CON_0);
	reg = (reg & ~(0xfff)) | (width & 0xfff);
	writel(reg, rdma_base + DISP_REG_RDMA_SIZE_CON_0);

	reg = readl(rdma_base + DISP_REG_RDMA_SIZE_CON_1);
	reg = (reg & ~(0xfffff)) | (height & 0xfffff);
	writel(reg, rdma_base + DISP_REG_RDMA_SIZE_CON_1);

	/*
	 * Enable FIFO underflow since DSI and DPI can't be blocked.
	 * Keep the FIFO pseudo size reset default of 8 KiB. Set the
	 * output threshold to 6 microseconds with 7/6 overhead to
	 * account for blanking, and with a pixel depth of 4 bytes:
	 */
	threshold = width * height * vrefresh * 4 * 7 / 1000000;
	reg = RDMA_FIFO_UNDERFLOW_EN |
	      RDMA_FIFO_PSEUDO_SIZE(fifo_pseudo_size) |
	      RDMA_OUTPUT_VALID_FIFO_THRESHOLD(threshold);
	writel(reg, rdma_base + DISP_REG_RDMA_FIFO_CON);
}

static void mtk_ufoe_start(void __iomem *ufoe_base)
{
	writel(UFO_BYPASS, ufoe_base + DISP_REG_UFO_START);
}

static const struct mtk_ddp_comp_funcs ddp_nop = {
};

static const struct mtk_ddp_comp_funcs ddp_bls = {
	.config = mtk_bls_config,
};

static const struct mtk_ddp_comp_funcs ddp_color = {
	.config = mtk_color_config,
	.power_on = mtk_color_start,
};

static const struct mtk_ddp_comp_funcs ddp_od = {
	.config = mtk_od_config,
	.power_on = mtk_od_start,
};

static const struct mtk_ddp_comp_funcs ddp_ovl = {
	.config = mtk_ovl_config,
	.power_on = mtk_ovl_start,
	.enable_vblank = mtk_ovl_enable_vblank,
	.disable_vblank = mtk_ovl_disable_vblank,
	.clear_vblank = mtk_ovl_clear_vblank,
	.layer_on = mtk_ovl_layer_on,
	.layer_off = mtk_ovl_layer_off,
	.layer_config = mtk_ovl_layer_config,
};

static const struct mtk_ddp_comp_funcs ddp_rdma = {
	.config = mtk_rdma_config,
	.power_on = mtk_rdma_start,
};

static const struct mtk_ddp_comp_funcs ddp_ufoe = {
	.power_on = mtk_ufoe_start,
};

static const char * const mtk_ddp_comp_stem[MTK_DDP_COMP_TYPE_MAX] = {
	[MTK_DISP_OVL] = "ovl",
	[MTK_DISP_RDMA] = "rdma",
	[MTK_DISP_WDMA] = "wdma",
	[MTK_DISP_COLOR] = "color",
	[MTK_DISP_AAL] = "aal",
	[MTK_DISP_GAMMA] = "gamma",
	[MTK_DISP_UFOE] = "ufoe",
	[MTK_DSI] = "dsi",
	[MTK_DPI] = "dpi",
	[MTK_DISP_PWM] = "pwm",
	[MTK_DISP_MUTEX] = "mutex",
	[MTK_DISP_OD] = "od",
	[MTK_DISP_BLS] = "bls",
};

struct mtk_ddp_comp_match {
	enum mtk_ddp_comp_type type;
	int alias_id;
	const struct mtk_ddp_comp_funcs *funcs;
};

static struct mtk_ddp_comp_match mtk_ddp_matches[DDP_COMPONENT_ID_MAX] = {
	[DDP_COMPONENT_AAL]	= { MTK_DISP_AAL,	0, &ddp_nop },
	[DDP_COMPONENT_BLS]	= { MTK_DISP_BLS,	0, &ddp_bls },
	[DDP_COMPONENT_COLOR0]	= { MTK_DISP_COLOR,	0, &ddp_color },
	[DDP_COMPONENT_COLOR1]	= { MTK_DISP_COLOR,	1, &ddp_color },
	[DDP_COMPONENT_DPI0]	= { MTK_DPI,		0, &ddp_nop },
	[DDP_COMPONENT_DSI0]	= { MTK_DSI,		0, &ddp_nop },
	[DDP_COMPONENT_DSI1]	= { MTK_DSI,		1, &ddp_nop },
	[DDP_COMPONENT_GAMMA]	= { MTK_DISP_GAMMA,	0, &ddp_nop },
	[DDP_COMPONENT_OD]	= { MTK_DISP_OD,	0, &ddp_nop },
	[DDP_COMPONENT_OVL0]	= { MTK_DISP_OVL,	0, &ddp_ovl },
	[DDP_COMPONENT_OVL1]	= { MTK_DISP_OVL,	1, &ddp_ovl },
	[DDP_COMPONENT_PWM0]	= { MTK_DISP_PWM,	0, &ddp_nop },
	[DDP_COMPONENT_RDMA0]	= { MTK_DISP_RDMA,	0, &ddp_rdma },
	[DDP_COMPONENT_RDMA1]	= { MTK_DISP_RDMA,	1, &ddp_rdma },
	[DDP_COMPONENT_RDMA2]	= { MTK_DISP_RDMA,	2, &ddp_rdma },
	[DDP_COMPONENT_UFOE]	= { MTK_DISP_UFOE,	0, &ddp_ufoe },
	[DDP_COMPONENT_WDMA0]	= { MTK_DISP_WDMA,	0, &ddp_nop },
	[DDP_COMPONENT_WDMA1]	= { MTK_DISP_WDMA,	1, &ddp_nop },
};

int mtk_ddp_comp_get_id(struct device_node *node,
			enum mtk_ddp_comp_type comp_type)
{
	int id = of_alias_get_id(node, mtk_ddp_comp_stem[comp_type]);
	int i;

	for (i = 0; i < ARRAY_SIZE(mtk_ddp_matches); i++) {
		if (comp_type == mtk_ddp_matches[i].type &&
		    (id < 0 || id == mtk_ddp_matches[i].alias_id))
			return i;
	}

	return -EINVAL;
}

int mtk_ddp_comp_init(struct device *dev, struct device_node *node,
		      struct mtk_ddp_comp *comp, enum mtk_ddp_comp_id comp_id)
{
	enum mtk_ddp_comp_type type;
	struct device_node *larb_node;
	struct platform_device *larb_pdev;

	if (comp_id < 0 || comp_id >= DDP_COMPONENT_ID_MAX)
		return -EINVAL;

	comp->id = comp_id;
	comp->funcs = mtk_ddp_matches[comp_id].funcs;

	if (comp_id == DDP_COMPONENT_DPI0 ||
	    comp_id == DDP_COMPONENT_DSI0 ||
	    comp_id == DDP_COMPONENT_PWM0) {
		comp->regs = NULL;
		comp->clk = NULL;
		comp->irq = 0;
		return 0;
	}

	comp->regs = of_iomap(node, 0);
	comp->irq = of_irq_get(node, 0);
	comp->clk = of_clk_get(node, 0);
	if (IS_ERR(comp->clk))
		comp->clk = NULL;

	type = mtk_ddp_matches[comp_id].type;

	/* Only DMA capable components need the LARB property */
	comp->larb_dev = NULL;
	if (type != MTK_DISP_OVL &&
	    type != MTK_DISP_RDMA &&
	    type != MTK_DISP_WDMA)
		return 0;

	larb_node = of_parse_phandle(node, "mediatek,larb", 0);
	if (!larb_node) {
		dev_err(dev,
			"Missing mediadek,larb phandle in %s node\n",
			node->full_name);
		return -EINVAL;
	}

	larb_pdev = of_find_device_by_node(larb_node);
	if (!larb_pdev) {
		dev_warn(dev, "Waiting for larb device %s\n",
			 larb_node->full_name);
		return -EPROBE_DEFER;
	}

	comp->larb_dev = &larb_pdev->dev;

	return 0;
}

int mtk_ddp_comp_register(struct drm_device *drm, struct mtk_ddp_comp *comp)
{
	struct mtk_drm_private *private = drm->dev_private;

	if (private->ddp_comp[comp->id])
		return -EBUSY;

	private->ddp_comp[comp->id] = comp;
	return 0;
}

void mtk_ddp_comp_unregister(struct drm_device *drm, struct mtk_ddp_comp *comp)
{
	struct mtk_drm_private *private = drm->dev_private;

	private->ddp_comp[comp->id] = NULL;
}
