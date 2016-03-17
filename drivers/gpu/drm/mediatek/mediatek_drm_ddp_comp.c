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

#include <drm/drmP.h>
#include <linux/clk.h>


#define DISP_REG_OVL_INTEN			0x0004
#define DISP_REG_OVL_INTSTA			0x0008
#define DISP_REG_OVL_EN				0x000c
#define DISP_REG_OVL_RST			0x0014
#define DISP_REG_OVL_ROI_SIZE			0x0020
#define DISP_REG_OVL_ROI_BGCLR			0x0028
#define DISP_REG_OVL_SRC_CON			0x002c
#define DISP_REG_OVL_L0_CON			0x0030
#define DISP_REG_OVL_L0_SRCKEY			0x0034
#define DISP_REG_OVL_L0_SRC_SIZE		0x0038
#define DISP_REG_OVL_L0_OFFSET			0x003c
#define DISP_REG_OVL_L0_PITCH			0x0044
#define DISP_REG_OVL_L1_CON			0x0050
#define DISP_REG_OVL_L1_SRCKEY			0x0054
#define DISP_REG_OVL_L1_SRC_SIZE		0x0058
#define DISP_REG_OVL_L1_OFFSET			0x005c
#define DISP_REG_OVL_L1_PITCH			0x0064
#define DISP_REG_OVL_RDMA0_CTRL			0x00c0
#define DISP_REG_OVL_RDMA0_MEM_GMC_SETTING	0x00c8
#define DISP_REG_OVL_RDMA0_FIFO_CTRL            0x00D0
#define DISP_REG_OVL_RDMA1_CTRL			0x00e0
#define DISP_REG_OVL_RDMA1_MEM_GMC_SETTING	0x00e8
#define DISP_REG_OVL_RDMA1_FIFO_CTRL            0x00F0
#define DISP_REG_OVL_L0_ADDR			0x0040
#define DISP_REG_OVL_L1_ADDR			0x0060
#define DISP_REG_OVL_RDMA2_CTRL                 0x0100
#define DISP_REG_OVL_RDMA2_MEM_GMC_SETTING      0x0108
#define DISP_REG_OVL_RDMA2_FIFO_CTRL            0x0110
#define DISP_REG_OVL_RDMA3_CTRL                 0x0120
#define DISP_REG_OVL_RDMA3_MEM_GMC_SETTING      0x0128
#define DISP_REG_OVL_RDMA3_FIFO_CTRL            0x0130

#define DISP_REG_RDMA_INT_ENABLE		0x0000
#define DISP_REG_RDMA_INT_STATUS		0x0004
#define DISP_REG_RDMA_GLOBAL_CON		0x0010
#define DISP_REG_RDMA_SIZE_CON_0		0x0014
#define DISP_REG_RDMA_SIZE_CON_1		0x0018
#define DISP_REG_RDMA_MEM_START_ADDR		0x0028
#define DISP_REG_RDMA_MEM_SRC_PITCH		0x002c
#define DISP_REG_RDMA_FIFO_CON			0x0040

#define DISP_OD_EN				0x0000
#define DISP_OD_INTEN				0x0008
#define DISP_OD_INTS				0x000c
#define DISP_OD_CFG				0x0020
#define DISP_OD_SIZE				0x0030

#define DISP_REG_UFO_START			0x0000

#define DISP_COLOR_CFG_MAIN			0x0400
#define DISP_COLOR_DBG_CFG_MAIN			0x0420
#define DISP_COLOR_START			0x0f00
#define DISP_COLOR_INTER			0x0f04
#define DISP_COLOR_OUT_SEL			0x0f0c


#define DISP_REG_BLS_EN                         0x0000
#define DISP_REG_BLS_SRC_SIZE	                0x0018
#define DISP_REG_BLS_PWM_DUTY	                0x00A0
#define DISP_REG_BLS_PWM_CON                    0x00A8
#define DISP_REG_BLS_DEBUG                      0x00B0

enum DISPLAY_PATH {
	PRIMARY_PATH = 0,
	EXTERNAL_PATH = 1,
};

enum RDMA_MODE {
	RDMA_MODE_DIRECT_LINK = 0,
	RDMA_MODE_MEMORY = 1,
};

enum RDMA_OUTPUT_FORMAT {
	RDMA_OUTPUT_FORMAT_ARGB = 0,
	RDMA_OUTPUT_FORMAT_YUV444 = 1,
};

#define OVL_COLOR_BASE 30
enum OVL_INPUT_FORMAT {
	OVL_INFMT_RGB888 = 0,
	OVL_INFMT_RGB565 = 1,
	OVL_INFMT_ARGB888 = 2,
	OVL_INFMT_ARGB8888 = 3,
	OVL_INFMT_YUYV = 8,
	OVL_INFMT_UYVY = 9,
	OVL_INFMT_UNKNOWN = 16,

	OVL_INFMT_BGR565 = OVL_INFMT_RGB565 + OVL_COLOR_BASE,
	OVL_INFMT_BGR888 = OVL_INFMT_RGB888 + OVL_COLOR_BASE,
	OVL_INFMT_ABGR8888 = OVL_INFMT_ARGB8888 + OVL_COLOR_BASE,
};

enum {
	OD_RELAY_MODE           = 0x1,
};

enum {
	UFO_BYPASS              = 0x4,
};

enum {
	COLOR_BYPASS_ALL        = (1UL<<7),
	COLOR_SEQ_SEL           = (1UL<<13),
	COLOR_DBUF_EN		= (1UL<<29),
};

enum {
	OVL_LAYER_SRC_DRAM      = 0,
};


void mediatek_ovl_enable_vblank(void __iomem *disp_base)
{
	writel(0x2, disp_base + DISP_REG_OVL_INTEN);
}

void mediatek_ovl_disable_vblank(void __iomem *disp_base)
{
	writel(0x0, disp_base + DISP_REG_OVL_INTEN);
}

void mediatek_ovl_clear_vblank(void __iomem *disp_base)
{
	writel(0x0, disp_base + DISP_REG_OVL_INTSTA);
}

static void mediatek_ovl_start(void __iomem *ovl_base)
{
	writel(0x0f, ovl_base + DISP_REG_OVL_INTEN);
	writel(0x01, ovl_base + DISP_REG_OVL_EN);
	writel(0x400000, ovl_base + DISP_REG_OVL_RDMA0_FIFO_CTRL);
}

void mediatek_ovl_config(void __iomem *ovl_base,
		unsigned int w, unsigned int h)
{
	writel(h << 16 | w, ovl_base + DISP_REG_OVL_ROI_SIZE);
	writel(0x00000000, ovl_base + DISP_REG_OVL_ROI_BGCLR);

	writel(0x1, ovl_base + DISP_REG_OVL_RST);
	writel(0x0, ovl_base + DISP_REG_OVL_RST);
}

void mediatek_ovl_layer_config_cursor(void __iomem *ovl_base,
	bool enable, unsigned int addr, int x, int y, int w, int h)
{
	unsigned int reg;
/*	unsigned int layer = 1; */
	unsigned int width = 64;
	unsigned int height = 64;
	unsigned int src_pitch = 64 * 4;
	bool keyEn = 0;
	bool aen = 1;			/* alpha enable */
	unsigned char alpha = 0xFF;
	unsigned int fmt = OVL_INFMT_ARGB8888;
	unsigned int src_con, new_set;

#if 1
	if (width + x > w)
		width = w - min(x, w);

	if (height + y > h)
		height = h - min(y, h);
#endif

	src_con = readl(ovl_base + DISP_REG_OVL_SRC_CON);
	if (enable)
		new_set = src_con | 0x2;
	else
		new_set = src_con & ~(0x2);

	if (new_set != src_con) {
		writel(new_set, ovl_base + DISP_REG_OVL_SRC_CON);
		writel(0x00000001, ovl_base + DISP_REG_OVL_RDMA1_CTRL);
		writel(0x40402020,
			ovl_base + DISP_REG_OVL_RDMA1_MEM_GMC_SETTING);

		reg = keyEn << 30 | fmt << 12 | aen << 8 | alpha;
		writel(reg, ovl_base + DISP_REG_OVL_L1_CON);
		writel(src_pitch & 0xFFFF, ovl_base + DISP_REG_OVL_L1_PITCH);
	}
	writel(height << 16 | width, ovl_base + DISP_REG_OVL_L1_SRC_SIZE);
	writel(y << 16 | x, ovl_base + DISP_REG_OVL_L1_OFFSET);
	writel(addr, ovl_base + DISP_REG_OVL_L1_ADDR);
}

static unsigned int ovl_fmt_convert(unsigned int fmt)
{
	switch (fmt) {
	case DRM_FORMAT_RGB888:
		return OVL_INFMT_RGB888;
	case DRM_FORMAT_RGB565:
		return OVL_INFMT_RGB565;
	case DRM_FORMAT_ARGB8888:
		return OVL_INFMT_ARGB8888;
	case DRM_FORMAT_BGR888:
		return OVL_INFMT_BGR888;
	case DRM_FORMAT_BGR565:
		return OVL_INFMT_BGR565;
	case DRM_FORMAT_ABGR8888:
		return OVL_INFMT_ABGR8888;
	case DRM_FORMAT_XRGB8888:
		return OVL_INFMT_ARGB888;
	case DRM_FORMAT_YUYV:
		return OVL_INFMT_YUYV;
	case DRM_FORMAT_UYVY:
		return OVL_INFMT_UYVY;
	default:
		return OVL_INFMT_UNKNOWN;
	}
}

void mediatek_ovl_layer_on(void __iomem *ovl_base)
{
	writel(0x1, ovl_base + DISP_REG_OVL_SRC_CON);

	/* frame buffer set in  mediatek_ovl_layer_config and disable in this*/
	writel(0x00000001, ovl_base + DISP_REG_OVL_RDMA0_CTRL);
	writel(0x40402020, ovl_base + DISP_REG_OVL_RDMA0_MEM_GMC_SETTING);
}

void mediatek_ovl_layer_off(void __iomem *ovl_base)
{
	unsigned int reg;

	reg = readl(ovl_base + DISP_REG_OVL_SRC_CON);
	reg = reg & ~0x1;
	writel(reg, ovl_base + DISP_REG_OVL_SRC_CON);
}

void mediatek_ovl_layer_config(void __iomem *ovl_base,
		unsigned int addr, unsigned int width, unsigned int height,
		unsigned int pitch, unsigned int format)
{
	unsigned int reg;
	unsigned int dst_x = 0;
	unsigned int dst_y = 0;
	bool color_key_en = 1;
	unsigned int color_key = 0xFF000000;
	bool alpha_en = 0;
	unsigned char alpha = 0x0;
	unsigned int rgb_swap, bpp;
	unsigned int fmt = ovl_fmt_convert(format);

	if (fmt == OVL_INFMT_BGR888 || fmt == OVL_INFMT_BGR565 ||
		fmt == OVL_INFMT_ABGR8888) {
		fmt -= OVL_COLOR_BASE;
		rgb_swap = 1;
	} else {
		rgb_swap = 0;
	}

	switch (fmt) {
	case OVL_INFMT_ARGB8888:
	case OVL_INFMT_ARGB888:
		bpp = 4;
		break;
	case OVL_INFMT_RGB888:
		bpp = 3;
		break;
	case OVL_INFMT_RGB565:
	case OVL_INFMT_YUYV:
	case OVL_INFMT_UYVY:
		bpp = 2;
		break;
	default:
		bpp = 1;
	}

	if (pitch == 0)
		pitch = width * bpp;

	reg = color_key_en << 30 | OVL_LAYER_SRC_DRAM << 28 |
		rgb_swap << 25 | fmt << 12 | alpha_en << 8 | alpha;
	writel(reg, ovl_base + DISP_REG_OVL_L0_CON);
	writel(color_key, ovl_base + DISP_REG_OVL_L0_SRCKEY);
	writel(height << 16 | width, ovl_base + DISP_REG_OVL_L0_SRC_SIZE);
	writel(dst_y << 16 | dst_x, ovl_base + DISP_REG_OVL_L0_OFFSET);

	writel(addr, ovl_base + DISP_REG_OVL_L0_ADDR);
	reg = readl(ovl_base + DISP_REG_OVL_L0_PITCH);
	reg &= ~(0xFFFFU);
	reg |= (pitch&0xFFFFU);
	writel(reg, ovl_base + DISP_REG_OVL_L0_PITCH);
}

static void mediatek_rdma_start(void __iomem *rdma_base)
{
	unsigned int reg;

	writel(0x3FU, rdma_base + DISP_REG_RDMA_INT_ENABLE);
	reg = readl(rdma_base + DISP_REG_RDMA_GLOBAL_CON);
	reg |= 1;
	writel(reg, rdma_base + DISP_REG_RDMA_GLOBAL_CON);
}

static void mediatek_rdma_config_direct_link(void __iomem *rdma_base,
		unsigned width, unsigned height)
{
	unsigned int reg;
	enum RDMA_MODE mode = RDMA_MODE_DIRECT_LINK;
	enum RDMA_OUTPUT_FORMAT output_format = RDMA_OUTPUT_FORMAT_ARGB;
	unsigned int fifo_pseudo_length = 256;
	unsigned int fifo_threashold;
	unsigned int bpp = 4;	/* xRGB from gpu */

	reg = readl(rdma_base + DISP_REG_RDMA_GLOBAL_CON);
	if (mode == RDMA_MODE_DIRECT_LINK)
		reg &= ~(0x2U);
	writel(reg, rdma_base + DISP_REG_RDMA_GLOBAL_CON);

	reg = readl(rdma_base + DISP_REG_RDMA_SIZE_CON_0);
	if (output_format == RDMA_OUTPUT_FORMAT_ARGB)
		reg &= ~(0x20000000U);
	else
		reg |= 0x20000000U;
	writel(reg, rdma_base + DISP_REG_RDMA_SIZE_CON_0);

	reg = readl(rdma_base + DISP_REG_RDMA_SIZE_CON_0);
	reg = (reg & ~(0xFFFU)) | (width & 0xFFFU);
	writel(reg, rdma_base + DISP_REG_RDMA_SIZE_CON_0);

	/* no rgb swap but need to check if it swaps or not*/
	reg = readl(rdma_base + DISP_REG_RDMA_SIZE_CON_0);
	reg = reg & ~(0x80000000U) & ~(0x40000000U);
	writel(reg, rdma_base + DISP_REG_RDMA_SIZE_CON_0);

	reg = readl(rdma_base + DISP_REG_RDMA_SIZE_CON_1);
	reg = (reg & ~(0xFFFFFU)) | (height & 0xFFFFFU);
	writel(reg, rdma_base + DISP_REG_RDMA_SIZE_CON_1);

	fifo_threashold = (width + 120) * bpp / 16;
	fifo_threashold =
	    fifo_threashold > fifo_pseudo_length ? fifo_pseudo_length : fifo_threashold;

	writel((1 << 31) | (fifo_pseudo_length << 16) | fifo_threashold,
		rdma_base + DISP_REG_RDMA_FIFO_CON);

	writel(0, rdma_base + DISP_REG_RDMA_MEM_START_ADDR);
	writel(0x3F, rdma_base + DISP_REG_RDMA_INT_ENABLE);
}

static void mediatek_color_start(void __iomem *color_base)
{
	writel(COLOR_BYPASS_ALL | COLOR_DBUF_EN,
		color_base + DISP_COLOR_CFG_MAIN);	/* color enable */
	writel(0, color_base + DISP_COLOR_DBG_CFG_MAIN);
	writel(0x1, color_base + DISP_COLOR_START);	/* color start */
	/* enable interrupt */
	writel(0x00000007, color_base + DISP_COLOR_INTER);
	/* Set 10bit->8bit Rounding  */
	writel(0x333, color_base + DISP_COLOR_OUT_SEL);
}

static void __iomem *g_bls_base;

static void mediatek_bls_init(void __iomem *bls_base, unsigned int width, unsigned int height)
{
	g_bls_base = bls_base;
	writel((height << 16) | width, bls_base + DISP_REG_BLS_SRC_SIZE);
	writel(0, bls_base + DISP_REG_BLS_PWM_DUTY);
	writel(0x0 | (1 << 16), bls_base + DISP_REG_BLS_PWM_CON);
	writel(0x00010000, bls_base + DISP_REG_BLS_EN);
}

/* add bls api for led driver to set backlight */
/* ------------------------------------------------------------------------------------ */
static DEFINE_MUTEX(backlight_mutex);

static unsigned int brightness_mapping(unsigned int level)
{
	unsigned int mapped_level;
	static int gMaxLevel = 1023;

	mapped_level = level;

	if (mapped_level > gMaxLevel)
		mapped_level = gMaxLevel;

	DRM_INFO("after mapping, mapped_level: %d\n", mapped_level);

	return mapped_level;
}

int disp_bls_set_backlight(unsigned int level)
{
	unsigned int regVal;
	unsigned int mapped_level;
	void __iomem *bls_base = g_bls_base;

	DRM_INFO("disp_bls_set_backlight: %d\n", level);
	mutex_lock(&backlight_mutex);

	writel(0x3, bls_base + DISP_REG_BLS_DEBUG);
	mapped_level = brightness_mapping(level);
	writel(mapped_level, bls_base + DISP_REG_BLS_PWM_DUTY);
	if (level != 0) {
		regVal = readl(bls_base + DISP_REG_BLS_EN);
		if (!(regVal & 0x10000))
			writel(regVal | 0x10000, bls_base + DISP_REG_BLS_EN);
	} else {
		regVal = readl(bls_base + DISP_REG_BLS_EN);
		if (regVal & 0x10000)
			writel(regVal & 0xFFFEFFFF, bls_base + DISP_REG_BLS_EN);
	}
	DRM_INFO("after SET, PWM_DUTY: %d\n", readl(bls_base + DISP_REG_BLS_PWM_DUTY));
	writel(0x0, bls_base + DISP_REG_BLS_DEBUG);

	mutex_unlock(&backlight_mutex);

	return 0;
}

void dump_bls_regs(void)
{
	void __iomem *bls_base = g_bls_base;

	DRM_INFO("dump_bls_regs\n");
	DRM_INFO("-------------------------------------------------------------\n");
	DRM_INFO("bls_en       = 0x%x\n", readl(bls_base + DISP_REG_BLS_EN));
	DRM_INFO("bls_src_size = 0x%x\n", readl(bls_base + DISP_REG_BLS_SRC_SIZE));
	DRM_INFO("bls_pwm_duty = 0x%x\n", readl(bls_base + DISP_REG_BLS_PWM_DUTY));
	DRM_INFO("bls_pwm_con  = 0x%x\n", readl(bls_base + DISP_REG_BLS_PWM_CON));
	DRM_INFO("bls_debug    = 0x%x\n", readl(bls_base + DISP_REG_BLS_DEBUG));
	DRM_INFO("-------------------------------------------------------------\n");
}
/* ------------------------------------------------------------------------------------ */

void main_disp_path_power_on(unsigned int width, unsigned int height,
		void __iomem *ovl_base,
		void __iomem *rdma_base,
		void __iomem *color_base,
		void __iomem *ufoe_base,
		void __iomem *bls_base)
{
	mediatek_ovl_config(ovl_base, width, height);

	mediatek_ovl_start(ovl_base);
	mediatek_bls_init(bls_base, width, height);
	mediatek_color_start(color_base);
	mediatek_rdma_config_direct_link(rdma_base, width, height);
	mediatek_rdma_start(rdma_base);
}

void ext_disp_path_power_on(unsigned int width, unsigned int height,
		void __iomem *ovl_base,
		void __iomem *rdma_base,
		void __iomem *color_base)
{
	width = 1920;
	height = 1080;

	mediatek_ovl_config(ovl_base, width, height);
	mediatek_rdma_config_direct_link(rdma_base, width, height);

	mediatek_ovl_start(ovl_base);
	mediatek_rdma_start(rdma_base);
	mediatek_color_start(color_base);
}

