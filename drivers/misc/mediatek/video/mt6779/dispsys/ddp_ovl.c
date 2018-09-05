/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#define LOG_TAG "OVL"
#include "ddp_log.h"
#include "ddp_clkmgr.h"
#include "ddp_m4u.h"
#include <linux/delay.h>
#include "ddp_info.h"
#include "ddp_hal.h"
#include "ddp_reg.h"
#include "ddp_ovl.h"
#include "primary_display.h"
#include "disp_rect.h"
#include "disp_assert_layer.h"
#include "ddp_mmp.h"
#include "debug.h"
#include "disp_drv_platform.h"

#define OVL_REG_BACK_MAX	(40)
#define OVL_LAYER_OFFSET	(0x20)
#define OVL_RDMA_DEBUG_OFFSET	(0x4)

enum OVL_COLOR_SPACE {
	OVL_COLOR_SPACE_RGB = 0,
	OVL_COLOR_SPACE_YUV,
};

struct OVL_REG {
	unsigned long address;
	unsigned int value;
};

static enum DISP_MODULE_ENUM ovl_index_module[OVL_NUM] = {
	DISP_MODULE_OVL0, DISP_MODULE_OVL0_2L, DISP_MODULE_OVL1_2L
};

static unsigned int gOVL_bg_color = 0xff000000;
static unsigned int gOVL_dim_color = 0xff000000;

static unsigned int ovl_bg_w[OVL_NUM];
static unsigned int ovl_bg_h[OVL_NUM];
static enum DISP_MODULE_ENUM next_rsz_module = DISP_MODULE_UNKNOWN;
static enum DISP_MODULE_ENUM prev_rsz_module = DISP_MODULE_UNKNOWN;

unsigned int ovl_set_bg_color(unsigned int bg_color)
{
	unsigned int old = gOVL_bg_color;

	gOVL_bg_color = bg_color;
	return old;
}

unsigned int ovl_set_dim_color(unsigned int dim_color)
{
	unsigned int old = gOVL_dim_color;

	gOVL_dim_color = dim_color;
	return old;
}

static inline int is_module_ovl(enum DISP_MODULE_ENUM module)
{
	if (module == DISP_MODULE_OVL0 || module == DISP_MODULE_OVL0_2L ||
	    module == DISP_MODULE_OVL1_2L)
		return 1;

	return 0;
}

static inline bool is_module_rsz(enum DISP_MODULE_ENUM module)
{
	if (module == DISP_MODULE_RSZ0)
		return true;

	return false;
}

unsigned long ovl_base_addr(enum DISP_MODULE_ENUM module)
{
	switch (module) {
	case DISP_MODULE_OVL0:
		return DISPSYS_OVL0_BASE;
	case DISP_MODULE_OVL0_2L:
		return DISPSYS_OVL0_2L_BASE;
	case DISP_MODULE_OVL1_2L:
		return DISPSYS_OVL1_2L_BASE;
	default:
		DDP_PR_ERR("invalid ovl module=%d\n", module);
		return -1;
	}
	return 0;
}

static inline unsigned long ovl_layer_num(enum DISP_MODULE_ENUM module)
{
	switch (module) {
	case DISP_MODULE_OVL0:
		return 4;
	case DISP_MODULE_OVL0_2L:
		return 2;
	case DISP_MODULE_OVL1_2L:
		return 2;
	default:
		DDP_PR_ERR("invalid ovl module=%d\n", module);
		return -1;
	}
	return 0;
}

enum CMDQ_EVENT_ENUM ovl_to_cmdq_event_nonsec_end(enum DISP_MODULE_ENUM module)
{
	switch (module) {
	case DISP_MODULE_OVL0:
		return CMDQ_SYNC_DISP_OVL0_2NONSEC_END;
	case DISP_MODULE_OVL0_2L:
		return CMDQ_SYNC_DISP_2LOVL0_2NONSEC_END;
	case DISP_MODULE_OVL1_2L:
		return CMDQ_SYNC_DISP_2LOVL1_2NONSEC_END;
	default:
		DDP_PR_ERR("invalid ovl module=%d, get cmdq event nonsecure fail\n",
			   module);
		ASSERT(0);
		return DISP_MODULE_UNKNOWN;
	}
	return 0;
}

static inline unsigned long ovl_to_cmdq_engine(enum DISP_MODULE_ENUM module)
{
	switch (module) {
	case DISP_MODULE_OVL0:
		return CMDQ_ENG_DISP_OVL0;
	case DISP_MODULE_OVL0_2L:
		return CMDQ_ENG_DISP_2L_OVL0;
	case DISP_MODULE_OVL1_2L:
		return CMDQ_ENG_DISP_2L_OVL1;
	default:
		DDP_PR_ERR("invalid ovl module=%d, get cmdq engine fail\n",
			   module);
		ASSERT(0);
		return DISP_MODULE_UNKNOWN;
	}
	return 0;
}

unsigned int ovl_to_index(enum DISP_MODULE_ENUM module)
{
	unsigned int i;

	for (i = 0; i < OVL_NUM; i++) {
		if (ovl_index_module[i] == module)
			return i;
	}
	DDP_PR_ERR("invalid ovl module=%d, get ovl index fail\n", module);
	ASSERT(0);
	return 0;
}

static inline enum DISP_MODULE_ENUM ovl_index_to_module(int index)
{
	if (index >= OVL_NUM) {
		DDP_PR_ERR("invalid ovl index=%d\n", index);
		ASSERT(0);
	}

	return ovl_index_module[index];
}

int ovl_start(enum DISP_MODULE_ENUM module, void *handle)
{
	unsigned long baddr = ovl_base_addr(module);

	DISP_REG_SET_FIELD(handle, EN_FLD_OVL_EN, baddr + DISP_REG_OVL_EN, 0x1);

	/* TODO: only use for mt3967 E1, remove on E2 */
	DISP_REG_SET_FIELD(handle, EN_FLD_HF_FOVL_CK_ON,
			   baddr + DISP_REG_OVL_EN, 0x1);

	DISP_REG_SET(handle, baddr + DISP_REG_OVL_INTEN,
		     0x1E0 | REG_FLD_VAL(INTEN_FLD_ABNORMAL_SOF, 1) |
		     REG_FLD_VAL(INTEN_FLD_START_INTEN, 1));

#if (defined(CONFIG_MTK_TEE_GP_SUPPORT) || \
	defined(CONFIG_TRUSTONIC_TEE_SUPPORT)) && \
	defined(CONFIG_MTK_SEC_VIDEO_PATH_SUPPORT)
	DISP_REG_SET_FIELD(handle, INTEN_FLD_FME_CPL_INTEN,
			   baddr + DISP_REG_OVL_INTEN, 1);
#endif
	DISP_REG_SET_FIELD(handle, DATAPATH_CON_FLD_LAYER_SMI_ID_EN,
			   baddr + DISP_REG_OVL_DATAPATH_CON, 0);
	DISP_REG_SET_FIELD(handle, DATAPATH_CON_FLD_OUTPUT_NO_RND,
			   baddr + DISP_REG_OVL_DATAPATH_CON, 0);
	DISP_REG_SET_FIELD(handle, DATAPATH_CON_FLD_GCLAST_EN,
			   baddr + DISP_REG_OVL_DATAPATH_CON, 1);
	DISP_REG_SET_FIELD(handle, DATAPATH_CON_FLD_OUTPUT_CLAMP,
			   baddr + DISP_REG_OVL_DATAPATH_CON, 1);
	return 0;
}

int ovl_stop(enum DISP_MODULE_ENUM module, void *handle)
{
	unsigned long baddr = ovl_base_addr(module);

	DISP_REG_SET(handle, baddr + DISP_REG_OVL_INTEN, 0);
	DISP_REG_SET_FIELD(handle, EN_FLD_OVL_EN, baddr + DISP_REG_OVL_EN, 0);
	DISP_REG_SET(handle, baddr + DISP_REG_OVL_INTSTA, 0);

	return 0;
}

int ovl_is_idle(enum DISP_MODULE_ENUM module)
{
	unsigned long baddr = ovl_base_addr(module);
	unsigned int ovl_flow_ctrl_dbg;

	ovl_flow_ctrl_dbg = DISP_REG_GET(baddr + DISP_REG_OVL_FLOW_CTRL_DBG);
	if ((ovl_flow_ctrl_dbg & 0x3ff) != 0x1 &&
	    (ovl_flow_ctrl_dbg & 0x3ff) != 0x2)
		return 0;

	return 1;
}

int ovl_reset(enum DISP_MODULE_ENUM module, void *handle)
{
#define OVL_IDLE (0x3)
	int ret = 0;
	unsigned int delay_cnt = 0;
	unsigned long baddr = ovl_base_addr(module);
	unsigned int ovl_flow_ctrl_dbg;

	DISP_CPU_REG_SET(baddr + DISP_REG_OVL_RST, 1);
	DISP_CPU_REG_SET(baddr + DISP_REG_OVL_RST, 0);
	/* only wait if not cmdq */
	if (handle)
		return ret;

	ovl_flow_ctrl_dbg = DISP_REG_GET(baddr + DISP_REG_OVL_FLOW_CTRL_DBG);
	while (!(ovl_flow_ctrl_dbg & OVL_IDLE)) {
		delay_cnt++;
		udelay(10);
		if (delay_cnt > 2000) {
			DDP_PR_ERR("%s reset timeout!\n",
				   ddp_get_module_name(module));
			ret = -1;
			break;
		}
	}
	return ret;
}

static void _store_roi(enum DISP_MODULE_ENUM module,
		       unsigned int bg_w, unsigned int bg_h)
{
	int idx = ovl_to_index(module);

	if (idx >= OVL_NUM)
		return;

	ovl_bg_w[idx] = bg_w;
	ovl_bg_h[idx] = bg_h;
}

static void _get_roi(enum DISP_MODULE_ENUM module,
		     unsigned int *bg_w, unsigned int *bg_h)
{
	int idx = ovl_to_index(module);

	if (idx >= OVL_NUM)
		return;

	*bg_w = ovl_bg_w[idx];
	*bg_h = ovl_bg_h[idx];
}

int ovl_roi(enum DISP_MODULE_ENUM module, unsigned int bg_w, unsigned int bg_h,
	    unsigned int bg_color, void *handle)
{
	unsigned long baddr = ovl_base_addr(module);

	if ((bg_w > OVL_MAX_WIDTH) || (bg_h > OVL_MAX_HEIGHT)) {
		DDP_PR_ERR("%s,exceed OVL max size, wh(%ux%u)\n",
			   __func__, bg_w, bg_h);
		ASSERT(0);
	}

	DISP_REG_SET(handle, baddr + DISP_REG_OVL_ROI_SIZE, bg_h << 16 | bg_w);
	DISP_REG_SET(handle, baddr + DISP_REG_OVL_ROI_BGCLR, bg_color);

	DISP_REG_SET_FIELD(handle, FLD_OVL_LC_SRC_W,
			   baddr + DISP_REG_OVL_LC_SRC_SIZE, bg_w);
	DISP_REG_SET_FIELD(handle, FLD_OVL_LC_SRC_H,
			   baddr + DISP_REG_OVL_LC_SRC_SIZE, bg_h);

	_store_roi(module, bg_w, bg_h);

	DDPMSG("%s_roi:(%ux%u)\n", ddp_get_module_name(module), bg_w, bg_h);
	return 0;
}

static int _ovl_get_rsz_layer_roi(const struct OVL_CONFIG_STRUCT *const c,
				u32 *dst_x, u32 *dst_y, u32 *dst_w, u32 *dst_h,
				struct disp_rect src_total_roi)
{
	if (c->src_w > c->dst_w || c->src_h > c->dst_h) {
		DDP_PR_ERR("%s:L%u:src(%ux%u)>dst(%ux%u)\n", __func__, c->layer,
			   c->src_w, c->src_h, c->dst_w, c->dst_h);
		return -EINVAL;
	}

	if (c->src_w < c->dst_w || c->src_h < c->dst_h) {
		*dst_x = ((c->dst_x * c->src_w) / c->dst_w) - src_total_roi.x;
		*dst_y = ((c->dst_y * c->src_h) / c->dst_h) - src_total_roi.y;
		*dst_w = c->src_w;
		*dst_h = c->src_h;
	}

	if (c->src_w != c->dst_w || c->src_h != c->dst_h) {
		DDPDBG("%s:L%u:(%u,%u,%ux%u)->(%u,%u,%ux%u)\n", __func__,
		       c->layer, *dst_x, *dst_y, *dst_w, *dst_h,
		       c->dst_x, c->dst_y, c->dst_w, c->dst_h);
	}

	return 0;
}

static int _ovl_get_rsz_roi(enum DISP_MODULE_ENUM module,
			    struct disp_ddp_path_config *pconfig,
			    u32 *bg_w, u32 *bg_h)
{
	struct disp_rect *rsz_src_roi = &pconfig->rsz_src_roi;

	if (pconfig->rsz_enable) {
		*bg_w = rsz_src_roi->width;
		*bg_h = rsz_src_roi->height;
	}

	return 0;
}

static int _ovl_set_rsz_roi(enum DISP_MODULE_ENUM module,
			    struct disp_ddp_path_config *pconfig, void *handle)
{
	u32 bg_w = pconfig->dst_w, bg_h = pconfig->dst_h;

	_ovl_get_rsz_roi(module, pconfig, &bg_w, &bg_h);
	ovl_roi(module, bg_w, bg_h, gOVL_bg_color, handle);

	return 0;
}

static int _ovl_lc_config(enum DISP_MODULE_ENUM module,
			  struct disp_ddp_path_config *pconfig, void *handle)
{
	unsigned long ovl_base = ovl_base_addr(module);
	struct disp_rect rsz_dst_roi = pconfig->rsz_dst_roi;
	u32 lc_x = 0, lc_y = 0, lc_w = pconfig->dst_w, lc_h = pconfig->dst_h;

	if (pconfig->rsz_enable) {
		lc_x = rsz_dst_roi.x;
		lc_y = rsz_dst_roi.y;
		lc_w = rsz_dst_roi.width;
		lc_h = rsz_dst_roi.height;
	}

	DISP_REG_SET_FIELD(handle, FLD_OVL_LC_XOFF,
			   ovl_base + DISP_REG_OVL_LC_OFFSET, lc_x);
	DISP_REG_SET_FIELD(handle, FLD_OVL_LC_YOFF,
			   ovl_base + DISP_REG_OVL_LC_OFFSET, lc_y);
	DISP_REG_SET_FIELD(handle, FLD_OVL_LC_SRC_W,
			   ovl_base + DISP_REG_OVL_LC_SRC_SIZE, lc_w);
	DISP_REG_SET_FIELD(handle, FLD_OVL_LC_SRC_H,
			   ovl_base + DISP_REG_OVL_LC_SRC_SIZE, lc_h);

	DISPMSG("[RPO] module=%s,lc(%d,%d,%dx%d)\n",
		ddp_get_module_name(module), lc_x, lc_y, lc_w, lc_h);

	return 0;
}

/* only disable L0 dim layer if RPO */
static void _rpo_disable_dim_L0(enum DISP_MODULE_ENUM module,
				struct disp_ddp_path_config *pconfig,
				int *en_layers)
{
	struct OVL_CONFIG_STRUCT *c = NULL;

	c = &pconfig->ovl_config[0];
	if (c->module != module)
		return;
	if (!(c->layer_en && c->source == OVL_LAYER_SOURCE_RESERVED))
		return;

	c = &pconfig->ovl_config[1];
	if (!(c->layer_en && ((c->src_w < c->dst_w) || (c->src_h < c->dst_h))))
		return;

	c = &pconfig->ovl_config[0];
	*en_layers &= ~(c->layer_en << c->phy_layer);
}

int disable_ovl_layers(enum DISP_MODULE_ENUM module, void *handle)
{
	unsigned long baddr = ovl_base_addr(module);

	/* physical layer control */
	DISP_REG_SET(handle, baddr + DISP_REG_OVL_SRC_CON, 0);
	/* ext layer control */
	DISP_REG_SET(handle, baddr + DISP_REG_OVL_DATAPATH_EXT_CON, 0);
	DDPSVPMSG("[SVP] switch %s to nonsec: disable all layers first!\n",
		  ddp_get_module_name(module));
	return 0;
}

int ovl_layer_switch(enum DISP_MODULE_ENUM module, unsigned int layer,
		     unsigned int en, void *handle)
{
	unsigned long baddr = ovl_base_addr(module);

	ASSERT(layer <= 3);
	switch (layer) {
	case 0:
		DISP_REG_SET_FIELD(handle, SRC_CON_FLD_L0_EN, baddr +
				   DISP_REG_OVL_SRC_CON, en);
		DISP_REG_SET(handle, baddr + DISP_REG_OVL_RDMA0_CTRL, en);
		break;
	case 1:
		DISP_REG_SET_FIELD(handle, SRC_CON_FLD_L1_EN, baddr +
				   DISP_REG_OVL_SRC_CON, en);
		DISP_REG_SET(handle, baddr + DISP_REG_OVL_RDMA1_CTRL, en);
		break;
	case 2:
		DISP_REG_SET_FIELD(handle, SRC_CON_FLD_L2_EN, baddr +
				   DISP_REG_OVL_SRC_CON, en);
		DISP_REG_SET(handle, baddr + DISP_REG_OVL_RDMA2_CTRL, en);
		break;
	case 3:
		DISP_REG_SET_FIELD(handle, SRC_CON_FLD_L3_EN, baddr +
				   DISP_REG_OVL_SRC_CON, en);
		DISP_REG_SET(handle, baddr + DISP_REG_OVL_RDMA3_CTRL, en);
		break;
	default:
		DDP_PR_ERR("invalid layer=%d\n", layer);
		ASSERT(0);
		break;
	}

	return 0;
}

static int
ovl_layer_config(enum DISP_MODULE_ENUM module, unsigned int phy_layer,
		 struct disp_rect src_total_roi, unsigned int is_engine_sec,
		 const struct OVL_CONFIG_STRUCT *const cfg,
		 const struct disp_rect *const ovl_partial_roi,
		 const struct disp_rect *const layer_partial_roi, void *handle)
{
	unsigned int value = 0;
	unsigned int Bpp, input_swap, input_fmt;
	unsigned int rgb_swap = 0;
	int is_rgb;
	int color_matrix = 0x4;
	int rotate = 0;
	int is_ext_layer = cfg->ext_layer != -1;
	unsigned long baddr = ovl_base_addr(module);
	unsigned long Lx_base = 0;
	unsigned long Lx_clr_base = 0;
	unsigned long Lx_addr_base = 0;
	unsigned int offset = 0;
	enum UNIFIED_COLOR_FMT format = cfg->fmt;
	unsigned int src_x = cfg->src_x;
	unsigned int src_y = cfg->src_y;
	unsigned int dst_x = cfg->dst_x;
	unsigned int dst_y = cfg->dst_y;
	unsigned int dst_w = cfg->dst_w;
	unsigned int dst_h = cfg->dst_h;
	unsigned int dim_color;

	if (is_ext_layer) {
		Lx_base = baddr + cfg->ext_layer * OVL_LAYER_OFFSET;
		Lx_base += (DISP_REG_OVL_EL0_CON - DISP_REG_OVL_L0_CON);

		Lx_clr_base = baddr + cfg->ext_layer * 0x4;
		Lx_clr_base += (DISP_REG_OVL_EL0_CLR - DISP_REG_OVL_L0_CLR);

		Lx_addr_base = baddr + cfg->ext_layer * 0x4;
		Lx_addr_base += (DISP_REG_OVL_EL0_ADDR - DISP_REG_OVL_L0_ADDR);
	} else {
		Lx_base = baddr + phy_layer * OVL_LAYER_OFFSET;
		Lx_clr_base = baddr + phy_layer * 0x4;
		Lx_addr_base = baddr + phy_layer * OVL_LAYER_OFFSET;
	}

	if (ovl_partial_roi) {
		dst_x = layer_partial_roi->x - ovl_partial_roi->x;
		dst_y = layer_partial_roi->y - ovl_partial_roi->y;
		dst_w = layer_partial_roi->width;
		dst_h = layer_partial_roi->height;
		src_x = cfg->src_x + layer_partial_roi->x - cfg->dst_x;
		src_y = cfg->src_y + layer_partial_roi->y - cfg->dst_y;

		DDPDBG("layer partial (%d,%d)(%d,%d,%dx%d) to (%d,%d)(%d,%d,%dx%d)\n",
		       cfg->src_x, cfg->src_y, cfg->dst_x, cfg->dst_y,
		       cfg->dst_w, cfg->dst_h, src_x, src_y, dst_x, dst_y,
		       dst_w, dst_h);
	}

	ASSERT(dst_w <= OVL_MAX_WIDTH);
	ASSERT(dst_h <= OVL_MAX_HEIGHT);

	if (!cfg->addr && cfg->source == OVL_LAYER_SOURCE_MEM) {
		DDP_PR_ERR("source from memory, but addr is 0!\n");
		ASSERT(0);
		return -1;
	}

	_ovl_get_rsz_layer_roi(cfg, &dst_x, &dst_y, &dst_w, &dst_h,
			       src_total_roi);

#ifdef CONFIG_MTK_LCM_PHYSICAL_ROTATION_HW
	if (module != DISP_MODULE_OVL1_2L)
		rotate = 1;
#endif

	/* check dim layer fmt */
	if (cfg->source == OVL_LAYER_SOURCE_RESERVED) {
		if (cfg->aen == 0)
			DDP_PR_ERR("dim layer%d alpha enable should be 1!\n",
				   phy_layer);
		format = UFMT_RGB888;
	}
	Bpp = ufmt_get_Bpp(format);
	input_swap = ufmt_get_byteswap(format);
	input_fmt = ufmt_get_format(format);
	is_rgb = ufmt_get_rgb(format);

	if (rotate) {
		unsigned int bg_w = 0, bg_h = 0;

		_get_roi(module, &bg_w, &bg_h);
		value = ((bg_h - dst_h - dst_y) << 16) | (bg_w - dst_w - dst_x);
	} else {
		value = (dst_y << 16) | dst_x;
	}
	DISP_REG_SET(handle, DISP_REG_OVL_L0_OFFSET + Lx_base, value);

	value = 0;
	if (format == UFMT_UYVY || format == UFMT_VYUY ||
	    format == UFMT_YUYV || format == UFMT_YVYU) {
		if (src_x % 2) {
			src_x -= 1; /* make src_x to even */
			dst_w += 1;
			value |= REG_FLD_VAL(OVL_L_CLIP_FLD_LEFT, 1);
		}

		if ((src_x + dst_w) % 2) {
			dst_w += 1; /* make right boundary even */
			value |= REG_FLD_VAL(OVL_L_CLIP_FLD_RIGHT, 1);
		}
	}
	DISP_REG_SET(handle, DISP_REG_OVL_L0_CLIP + Lx_base, value);

	switch (cfg->yuv_range) {
	case 0:
		color_matrix = 4;
		break; /* BT601_full */
	case 1:
		color_matrix = 6;
		break; /* BT601 */
	case 2:
		color_matrix = 7;
		break; /* BT709 */
	default:
		DDP_PR_ERR("un-recognized yuv_range=%d!\n", cfg->yuv_range);
		color_matrix = 4;
		break;
	}

	DISP_REG_SET(handle, DISP_REG_OVL_RDMA0_CTRL + Lx_base, 0x1);

	value = (REG_FLD_VAL((L_CON_FLD_LSRC), (cfg->source)) |
		 REG_FLD_VAL((L_CON_FLD_CFMT), (input_fmt)) |
		 REG_FLD_VAL((L_CON_FLD_AEN), (cfg->aen)) |
		 REG_FLD_VAL((L_CON_FLD_APHA), (cfg->alpha)) |
		 REG_FLD_VAL((L_CON_FLD_SKEN), (cfg->keyEn)) |
		 REG_FLD_VAL((L_CON_FLD_BTSW), (input_swap)));
	if (ufmt_is_old_fmt(format)) {
		if (format == UFMT_PARGB8888 || format == UFMT_PABGR8888 ||
		    format == UFMT_PRGBA8888 || format == UFMT_PBGRA8888) {
			rgb_swap = ufmt_get_rgbswap(format);
			value |= REG_FLD_VAL((L_CON_FLD_RGB_SWAP), (rgb_swap));
		}
		value |= REG_FLD_VAL((L_CON_FLD_CLRFMT_MAN), (1));
	}

	if (!is_rgb)
		value |= REG_FLD_VAL((L_CON_FLD_MTX), (color_matrix));

	if (rotate)
		value |= REG_FLD_VAL((L_CON_FLD_VIRTICAL_FLIP), (1)) |
			 REG_FLD_VAL((L_CON_FLD_HORI_FLIP), (1));

	DISP_REG_SET(handle, DISP_REG_OVL_L0_CON + Lx_base, value);

	dim_color = gOVL_dim_color == 0xff000000 ?
		    gOVL_dim_color : cfg->dim_color;
	DISP_REG_SET(handle, DISP_REG_OVL_L0_CLR + Lx_clr_base,
		     0xff000000 | dim_color);

	DISP_REG_SET(handle, DISP_REG_OVL_L0_SRC_SIZE + Lx_base,
		     dst_h << 16 | dst_w);

	if (rotate)
		offset = (src_x + dst_w) * Bpp + (src_y + dst_h - 1) *
						cfg->src_pitch - 1;
	else
		offset = src_x * Bpp + src_y * cfg->src_pitch;

	if (!is_engine_sec) {
		DISP_REG_SET(handle, DISP_REG_OVL_L0_ADDR + Lx_addr_base,
			     cfg->addr + offset);
	} else {
#ifdef MTKFB_M4U_SUPPORT
		unsigned int size;
		int m4u_port;

		size = (dst_h - 1) * cfg->src_pitch + dst_w * Bpp;
		m4u_port = module_to_m4u_port(module);
#ifdef MTKFB_CMDQ_SUPPORT
		if (cfg->security != DISP_SECURE_BUFFER) {
			/*
			 * OVL is sec but this layer is non-sec
			 * we need to tell cmdq to help map non-sec mva
			 * to sec mva
			 */
			cmdqRecWriteSecure(handle,
					disp_addr_convert(DISP_REG_OVL_L0_ADDR +
							  Lx_addr_base),
					CMDQ_SAM_NMVA_2_MVA, cfg->addr + offset,
					0, size, m4u_port);
		} else {
			/*
			 * for sec layer, addr variable stores sec handle
			 * we need to pass this handle and offset to cmdq driver
			 * cmdq sec driver will help to convert handle to
			 * correct address
			 */
			cmdqRecWriteSecure(handle,
					disp_addr_convert(DISP_REG_OVL_L0_ADDR +
							  Lx_addr_base),
					CMDQ_SAM_H_2_MVA, cfg->addr, offset,
					size, m4u_port);
		}
#endif
#endif /* MTKFB_M4U_SUPPORT */
	}
	DISP_REG_SET(handle, DISP_REG_OVL_L0_SRCKEY + Lx_base, cfg->key);

	value = REG_FLD_VAL(L_PITCH_FLD_SA_SEL, cfg->src_alpha & 0x3) |
		REG_FLD_VAL(L_PITCH_FLD_SRGB_SEL, cfg->src_alpha & 0x3) |
		REG_FLD_VAL(L_PITCH_FLD_DA_SEL, cfg->dst_alpha & 0x3) |
		REG_FLD_VAL(L_PITCH_FLD_DRGB_SEL, cfg->dst_alpha & 0x3) |
		REG_FLD_VAL(L_PITCH_FLD_SURFL_EN, cfg->src_alpha & 0x1) |
		REG_FLD_VAL(L_PITCH_FLD_SRC_PITCH, cfg->src_pitch);

	if (cfg->const_bld)
		value |= REG_FLD_VAL((L_PITCH_FLD_CONST_BLD), (1));
	DISP_REG_SET(handle, DISP_REG_OVL_L0_PITCH + Lx_base, value);

	return 0;
}

int ovl_clock_on(enum DISP_MODULE_ENUM module, void *handle)
{
	DDPDBG("%s clock_on\n", ddp_get_module_name(module));
	ddp_clk_enable_by_module(module);
	return 0;
}

int ovl_clock_off(enum DISP_MODULE_ENUM module, void *handle)
{
	DDPDBG("%s clock_off\n", ddp_get_module_name(module));
	ddp_clk_disable_by_module(module);
	return 0;
}

int ovl_init(enum DISP_MODULE_ENUM module, void *handle)
{
	return ovl_clock_on(module, handle);
}

int ovl_deinit(enum DISP_MODULE_ENUM module, void *handle)
{
	return ovl_clock_off(module, handle);
}

static int _ovl_UFOd_in(enum DISP_MODULE_ENUM module, int connect, void *handle)
{
	unsigned long ovl_base = ovl_base_addr(module);

	if (!connect) {
		DISP_REG_SET_FIELD(handle, SRC_CON_FLD_LC_EN,
				   ovl_base + DISP_REG_OVL_SRC_CON, 0);
		DISP_REG_SET_FIELD(handle, L_CON_FLD_LSRC,
				   ovl_base + DISP_REG_OVL_LC_CON, 0);
		return 0;
	}

	DISP_REG_SET_FIELD(handle, L_CON_FLD_LSRC,
			   ovl_base + DISP_REG_OVL_LC_CON, 2);
	DISP_REG_SET_FIELD(handle, LC_SRC_SEL_FLD_L_SEL,
			   ovl_base + DISP_REG_OVL_LC_SRC_SEL, 0);
	DISP_REG_SET_FIELD(handle, L_CON_FLD_AEN,
			   ovl_base + DISP_REG_OVL_LC_CON, 0);
	DISP_REG_SET_FIELD(handle, SRC_CON_FLD_LC_EN,
			   ovl_base + DISP_REG_OVL_SRC_CON, 1);

	return 0;
}

int ovl_connect(enum DISP_MODULE_ENUM module, enum DISP_MODULE_ENUM prev,
		enum DISP_MODULE_ENUM next, int connect, void *handle)
{
	unsigned long baddr = ovl_base_addr(module);

	if (connect && is_module_ovl(prev))
		DISP_REG_SET_FIELD(handle, DATAPATH_CON_FLD_BGCLR_IN_SEL,
				   baddr + DISP_REG_OVL_DATAPATH_CON, 1);
	else
		DISP_REG_SET_FIELD(handle, DATAPATH_CON_FLD_BGCLR_IN_SEL,
				   baddr + DISP_REG_OVL_DATAPATH_CON, 0);

	if (connect && is_module_rsz(prev)) {
		_ovl_UFOd_in(module, 1, handle);
		next_rsz_module = module;
	} else {
		_ovl_UFOd_in(module, 0, handle);
	}

	if (connect && is_module_rsz(next))
		prev_rsz_module = module;

	DISP_REG_SET_FIELD(handle, DATAPATH_CON_FLD_OUTPUT_CLAMP,
			   baddr + DISP_REG_OVL_DATAPATH_CON, 1);

	return 0;
}

unsigned int ddp_ovl_get_cur_addr(bool rdma_mode, int layerid)
{
	/* just workaround, should remove this func */
	unsigned long baddr = ovl_base_addr(DISP_MODULE_OVL0);
	unsigned long Lx_base = 0;

	if (rdma_mode)
		return DISP_REG_GET(DISP_REG_RDMA_MEM_START_ADDR);

	Lx_base = layerid * OVL_LAYER_OFFSET + baddr;
	if (DISP_REG_GET(DISP_REG_OVL_RDMA0_CTRL + Lx_base) & 0x1)
		return DISP_REG_GET(DISP_REG_OVL_L0_ADDR + Lx_base);

	return 0;
}

void ovl_get_address(enum DISP_MODULE_ENUM module, unsigned long *addr)
{
	int i = 0;
	unsigned long baddr = ovl_base_addr(module);
	unsigned long Lx_base = 0;
	unsigned int src_on = DISP_REG_GET(DISP_REG_OVL_SRC_CON + baddr);

	for (i = 0; i < 4; i++) {
		Lx_base = i * OVL_LAYER_OFFSET + baddr;
		if (src_on & (0x1 << i))
			addr[i] = DISP_REG_GET(Lx_base + DISP_REG_OVL_L0_ADDR);
		else
			addr[i] = 0;
	}
}

void ovl_get_info(enum DISP_MODULE_ENUM module, void *data)
{
	int i = 0;
	struct OVL_BASIC_STRUCT *pdata = data;
	unsigned long baddr = ovl_base_addr(module);
	unsigned long Lx_base = 0;
	unsigned int src_on = DISP_REG_GET(DISP_REG_OVL_SRC_CON + baddr);
	struct OVL_BASIC_STRUCT *p = NULL;

	for (i = 0; i < ovl_layer_num(module); i++) {
		unsigned int val = 0;

		Lx_base = i * OVL_LAYER_OFFSET + baddr;
		p = &pdata[i];
		p->layer = i;
		p->layer_en = src_on & (0x1 << i);
		if (!p->layer_en) {
			DDPMSG("%s:layer%d,en %d,w %d,h %d,bpp %d,addr %lu\n",
			       __func__, i, p->layer_en, p->src_w, p->src_h,
			       p->bpp, p->addr);
			continue;
		}

		val = DISP_REG_GET(DISP_REG_OVL_L0_CON + Lx_base);
		p->fmt = display_fmt_reg_to_unified_fmt(
				REG_FLD_VAL_GET(L_CON_FLD_CFMT, val),
				REG_FLD_VAL_GET(L_CON_FLD_BTSW, val),
				REG_FLD_VAL_GET(L_CON_FLD_RGB_SWAP, val));
		p->bpp = UFMT_GET_bpp(p->fmt) / 8;
		p->alpha = REG_FLD_VAL_GET(L_CON_FLD_AEN, val);

		p->addr = DISP_REG_GET(Lx_base + DISP_REG_OVL_L0_ADDR);

		val = DISP_REG_GET(DISP_REG_OVL_L0_SRC_SIZE + Lx_base);
		p->src_w = REG_FLD_VAL_GET(SRC_SIZE_FLD_SRC_W, val);
		p->src_h = REG_FLD_VAL_GET(SRC_SIZE_FLD_SRC_H, val);

		val = DISP_REG_GET(DISP_REG_OVL_L0_PITCH + Lx_base);
		p->src_pitch = REG_FLD_VAL_GET(L_PITCH_FLD_SRC_PITCH, val);

		val = DISP_REG_GET(DISP_REG_OVL_DATAPATH_CON + Lx_base);
		p->gpu_mode = val & (0x1 << (8 + i));
		p->adobe_mode = REG_FLD_VAL_GET(DATAPATH_CON_FLD_ADOBE_MODE,
						val);
		p->ovl_gamma_out = REG_FLD_VAL_GET(
					DATAPATH_CON_FLD_OVL_GAMMA_OUT, val);

		DDPMSG("%s:layer%d,en %d,w %d,h %d,bpp %d,addr %lu\n",
		       __func__, i, p->layer_en, p->src_w, p->src_h,
		       p->bpp, p->addr);
	}
}

static int ovl_check_input_param(struct OVL_CONFIG_STRUCT *cfg)
{
	if ((cfg->addr == 0 && cfg->source == 0) ||
	    cfg->dst_w == 0 || cfg->dst_h == 0) {
		DDP_PR_ERR("ovl parameter invalidate, addr=0x%08lx, w=%d, h=%d\n",
			   cfg->addr, cfg->dst_w, cfg->dst_h);
		ASSERT(0);
		return -1;
	}

	return 0;
}

/* use noinline to reduce stack size */
static noinline void
print_layer_config_args(int module, struct OVL_CONFIG_STRUCT *cfg,
			struct disp_rect *roi)
{
	DDPDBG("%s:L%d/e%d/%u,source=%s,(%u,%u,%ux%u)(%u,%u,%ux%u),pitch=%u\n",
	       ddp_get_module_name(module), cfg->phy_layer, cfg->ext_layer,
	       cfg->layer, (cfg->source == 0) ? "mem" : "dim",
	       cfg->src_x, cfg->src_y, cfg->src_w, cfg->src_h,
	       cfg->dst_x, cfg->dst_y, cfg->dst_w, cfg->dst_h, cfg->src_pitch);
	DDPDBG(" fmt=%s,addr=0x%08lx,keyEn=%d,key=%d,aen=%d,alpha=%d\n",
	       unified_color_fmt_name(cfg->fmt), cfg->addr,
	       cfg->keyEn, cfg->key, cfg->aen, cfg->alpha);
	DDPDBG(" sur_aen=%d,sur_alpha=0x%x,yuv_range=%d,sec=%d,const_bld=%d\n",
	       cfg->sur_aen, (cfg->dst_alpha << 2) | cfg->src_alpha,
	       cfg->yuv_range, cfg->security, cfg->const_bld);

	if (roi)
		DDPDBG("dirty(%d,%d,%dx%d)\n", roi->x, roi->y, roi->height,
		       roi->width);
}

static int ovl_is_sec[OVL_NUM];

static inline int ovl_switch_to_sec(enum DISP_MODULE_ENUM module, void *handle)
{
	unsigned int ovl_idx = ovl_to_index(module);
	enum CMDQ_ENG_ENUM cmdq_engine;

	cmdq_engine = ovl_to_cmdq_engine(module);
	cmdqRecSetSecure(handle, 1);
	/*
	 * set engine as sec port, it will to access
	 * the sec memory EMI_MPU protected
	 */
	cmdqRecSecureEnablePortSecurity(handle, (1LL << cmdq_engine));
	/* enable DAPC to protect the engine register */
	/* cmdqRecSecureEnableDAPC(handle, (1LL << cmdq_engine)); */
	if (ovl_is_sec[ovl_idx] == 0) {
		DDPSVPMSG("[SVP] switch ovl%d to sec\n", ovl_idx);
		mmprofile_log_ex(ddp_mmp_get_events()->svp_module[module],
				 MMPROFILE_FLAG_START, 0, 0);
	}
	ovl_is_sec[ovl_idx] = 1;

	return 0;
}

int ovl_switch_to_nonsec(enum DISP_MODULE_ENUM module, void *handle)
{
	unsigned int ovl_idx = ovl_to_index(module);
	enum CMDQ_ENG_ENUM cmdq_engine;
	enum CMDQ_EVENT_ENUM cmdq_event_nonsec_end;

	cmdq_engine = ovl_to_cmdq_engine(module);

	if (ovl_is_sec[ovl_idx] == 1) {
		/* OVL is in sec stat, we need to switch it to nonsec */
		struct cmdqRecStruct *nonsec_switch_handle = NULL;
		int ret;

		ret = cmdqRecCreate(
				CMDQ_SCENARIO_DISP_PRIMARY_DISABLE_SECURE_PATH,
				&(nonsec_switch_handle));
		if (ret)
			DDPAEE("[SVP]fail to create disable_handle %s ret=%d\n",
			       __func__, ret);

		cmdqRecReset(nonsec_switch_handle);

		if (module != DISP_MODULE_OVL1_2L) {
			/* Primary Mode */
			if (primary_display_is_decouple_mode())
				cmdqRecWaitNoClear(nonsec_switch_handle,
						   CMDQ_EVENT_DISP_WDMA0_EOF);
			else
				_cmdq_insert_wait_frame_done_token_mira(
							nonsec_switch_handle);
		} else {
			/* External Mode */
			cmdqRecWaitNoClear(nonsec_switch_handle,
					   CMDQ_SYNC_DISP_EXT_STREAM_EOF);
		}
		cmdqRecSetSecure(nonsec_switch_handle, 1);

		/*
		 * we should disable OVL before new (nonsec) setting takes
		 * effect, or translation fault may happen.
		 * if we switch OVL to nonsec BUT its setting is still sec
		 */
		disable_ovl_layers(module, nonsec_switch_handle);
		/* in fact, dapc/port_sec will be disabled by cmdq */
		cmdqRecSecureEnablePortSecurity(nonsec_switch_handle,
						(1LL << cmdq_engine));

		if (handle) {
			/* async flush method */
			cmdq_event_nonsec_end = ovl_to_cmdq_event_nonsec_end(
									module);
			cmdqRecSetEventToken(nonsec_switch_handle,
					     cmdq_event_nonsec_end);
			cmdqRecFlushAsync(nonsec_switch_handle);
			cmdqRecWait(handle, cmdq_event_nonsec_end);
		} else {
			/* sync flush method */
			cmdqRecFlush(nonsec_switch_handle);
		}

		cmdqRecDestroy(nonsec_switch_handle);
		DDPSVPMSG("[SVP] switch ovl%d to nonsec\n", ovl_idx);
		mmprofile_log_ex(ddp_mmp_get_events()->svp_module[module],
				 MMPROFILE_FLAG_END, 0, 0);
	}
	ovl_is_sec[ovl_idx] = 0;

	return 0;
}

static int setup_ovl_sec(enum DISP_MODULE_ENUM module,
			 struct disp_ddp_path_config *pConfig, void *handle)
{
	int ret;
	int layer_id;
	int has_sec_layer = 0;
	struct OVL_CONFIG_STRUCT *ovl_config;

	/* check if the OVL module has sec layer */
	for (layer_id = 0; layer_id < TOTAL_OVL_LAYER_NUM; layer_id++) {
		ovl_config = &pConfig->ovl_config[layer_id];
		if (ovl_config->module == module && ovl_config->layer_en &&
		    ovl_config->security == DISP_SECURE_BUFFER)
			has_sec_layer = 1;
	}

	if (!handle) {
		DDPDBG("[SVP] bypass ovl sec setting sec=%d,handle=NULL\n",
		       has_sec_layer);
		return 0;
	}

	if (has_sec_layer == 1)
		ret = ovl_switch_to_sec(module, handle);
	else
		ret = ovl_switch_to_nonsec(module, NULL);

	if (ret)
		DDPAEE("[SVP]fail to %s: ret=%d\n",
		       __func__, ret);

	return has_sec_layer;
}

/**
 * for enabled layers: layout continuously for each OVL HW engine
 * for disabled layers: ignored
 */
static int ovl_layer_layout(enum DISP_MODULE_ENUM module,
			    struct disp_ddp_path_config *pConfig)
{
	int local_layer, global_layer = 0;
	int phy_layer = -1, ext_layer = -1, ext_layer_idx = 0;
	struct OVL_CONFIG_STRUCT *cfg = NULL;

	/*
	 * 1. check if it has been prepared,
	 * just only prepare once for each frame
	 */
	for (global_layer = 0; global_layer < TOTAL_OVL_LAYER_NUM;
	     global_layer++) {
		if (!(pConfig->ovl_layer_scanned & (1 << global_layer)))
			break;
	}
	if (global_layer >= TOTAL_OVL_LAYER_NUM)
		return 0;

	/* reset which module belonged to */
	for (local_layer = 0; local_layer < TOTAL_OVL_LAYER_NUM;
	     local_layer++) {
		cfg = &pConfig->ovl_config[local_layer];
		cfg->module = -1;
	}

	/*
	 * layer layout for @module:
	 *   dispatch which module belonged to, phy layer, and ext layer
	 */
	for (local_layer = 0; local_layer <= ovl_layer_num(module);
	     global_layer++) {
		if (global_layer >= TOTAL_OVL_LAYER_NUM)
			break;

		cfg = &pConfig->ovl_config[global_layer];

		/* check if there is any extended layer on last phy layer */
		if (local_layer == ovl_layer_num(module)) {
			if (!disp_helper_get_option(DISP_OPT_OVL_EXT_LAYER) ||
			    cfg->ext_sel_layer == -1)
				break;
		}

		ext_layer = -1;
		cfg->phy_layer = 0;
		cfg->ext_layer = -1;

		pConfig->ovl_layer_scanned |= (1 << global_layer);

		if (cfg->layer_en == 0) {
			local_layer++;
			continue;
		}

		if (disp_helper_get_option(DISP_OPT_OVL_EXT_LAYER) &&
		    cfg->ext_sel_layer != -1) {
			if (phy_layer != cfg->ext_sel_layer) {
				DDP_PR_ERR("L%d layout not match: cur_phy=%d, ext_sel=%d\n",
					   global_layer, phy_layer,
					   cfg->ext_sel_layer);
				phy_layer++;
				ext_layer = -1;
			} else {
				ext_layer = ext_layer_idx++;
			}
		} else {
			phy_layer = local_layer++;
		}

		cfg->module = module;
		cfg->phy_layer = phy_layer;
		cfg->ext_layer = ext_layer;
		DDPDBG("layout:L%d->%s,phy:%d,ext:%d,ext_sel:%d\n",
		       global_layer, ddp_get_module_name(module),
		       phy_layer, ext_layer,
		       (ext_layer >= 0) ? phy_layer : cfg->ext_sel_layer);
	}

	/* for ASSERT_LAYER, do it specially */
	if (is_DAL_Enabled() && module == DISP_MODULE_OVL0_2L) {
		unsigned int dal = primary_display_get_option("ASSERT_LAYER");

		cfg = &pConfig->ovl_config[dal];
		cfg->module = module;
		cfg->phy_layer = ovl_layer_num(cfg->module) - 1;
		cfg->ext_sel_layer = -1;
		cfg->ext_layer = -1;
	}

	return 1;
}

static int ovl_config_l(enum DISP_MODULE_ENUM module,
			struct disp_ddp_path_config *pConfig, void *handle)
{
	int phy_layer_en = 0;
	int has_sec_layer = 0;
	int layer_id;
	int ovl_layer = 0;
	int ext_layer_en = 0, ext_sel_layers = 0;
	unsigned long baddr = ovl_base_addr(module);
	unsigned long Lx_base = 0;

	if (pConfig->dst_dirty)
		ovl_roi(module, pConfig->dst_w, pConfig->dst_h, gOVL_bg_color,
			handle);

	if (!pConfig->ovl_dirty)
		return 0;

	ovl_layer_layout(module, pConfig);

	/* be careful, turn off all layers */
	for (ovl_layer = 0; ovl_layer < ovl_layer_num(module); ovl_layer++) {
		Lx_base = baddr + ovl_layer * OVL_LAYER_OFFSET;
		DISP_REG_SET(handle, DISP_REG_OVL_RDMA0_CTRL + Lx_base, 0x0);
	}

	has_sec_layer = setup_ovl_sec(module, pConfig, handle);

	if (!pConfig->ovl_partial_dirty && module == prev_rsz_module)
		_ovl_set_rsz_roi(module, pConfig, handle);

	for (layer_id = 0; layer_id < TOTAL_OVL_LAYER_NUM; layer_id++) {
		struct OVL_CONFIG_STRUCT *cfg = &pConfig->ovl_config[layer_id];
		int enable = cfg->layer_en;

		if (enable == 0)
			continue;
		if (ovl_check_input_param(cfg)) {
			DDPAEE("invalid layer parameters!\n");
			continue;
		}
		if (cfg->module != module)
			continue;

		if (pConfig->ovl_partial_dirty) {
			struct disp_rect layer_roi = {0, 0, 0, 0};
			struct disp_rect layer_partial_roi = {0, 0, 0, 0};

			layer_roi.x = cfg->dst_x;
			layer_roi.y = cfg->dst_y;
			layer_roi.width = cfg->dst_w;
			layer_roi.height = cfg->dst_h;
			if (rect_intersect(&layer_roi,
					   &pConfig->ovl_partial_roi,
					   &layer_partial_roi)) {
				print_layer_config_args(module, cfg,
							&layer_partial_roi);
				ovl_layer_config(module, cfg->phy_layer,
						pConfig->rsz_src_roi,
						has_sec_layer, cfg,
						&pConfig->ovl_partial_roi,
						&layer_partial_roi, handle);
			} else {
				/* this layer will not be displayed */
				enable = 0;
			}
		} else {
			print_layer_config_args(module, cfg, NULL);
			ovl_layer_config(module, cfg->phy_layer,
					 pConfig->rsz_src_roi, has_sec_layer,
					 cfg, NULL, NULL, handle);
		}

		if (cfg->ext_layer != -1) {
			ext_layer_en |= enable << cfg->ext_layer;
			ext_sel_layers |= cfg->phy_layer <<
						(16 + 4 * cfg->ext_layer);
		} else {
			phy_layer_en |= enable << cfg->phy_layer;
		}
	}

	if (!pConfig->ovl_partial_dirty && module == next_rsz_module)
		_ovl_lc_config(module, pConfig, handle);

	_rpo_disable_dim_L0(module, pConfig, &phy_layer_en);

	DDPDBG("%s:layer_en=0x%01x,ext_layer_en=0x%01x,ext_sel_layers=0x%04x\n",
	       ddp_get_module_name(module), phy_layer_en, ext_layer_en,
	       ext_sel_layers >> 16);
	DISP_REG_SET_FIELD(handle, SRC_CON_FLD_L_EN,
			   ovl_base_addr(module) + DISP_REG_OVL_SRC_CON,
			   phy_layer_en);
	/* ext layer control */
	DISP_REG_SET(handle, baddr + DISP_REG_OVL_DATAPATH_EXT_CON,
		     ext_sel_layers | ext_layer_en);

	return 0;
}

int ovl_build_cmdq(enum DISP_MODULE_ENUM module, void *cmdq_trigger_handle,
		   enum CMDQ_STATE state)
{
	int ret = 0;
	/* int reg_pa = DISP_REG_OVL_FLOW_CTRL_DBG & 0x1fffffff; */

	if (!cmdq_trigger_handle) {
		DDP_PR_ERR("cmdq_trigger_handle is NULL\n");
		return -1;
	}

	if (state == CMDQ_CHECK_IDLE_AFTER_STREAM_EOF) {
		if (module == DISP_MODULE_OVL0) {
			ret = cmdqRecPoll(cmdq_trigger_handle, 0x14007240, 2,
					  0x3f);
		} else {
			DDP_PR_ERR("wrong module: %s\n",
				   ddp_get_module_name(module));
			return -1;
		}
	}

	return 0;
}

void ovl_dump_golden_setting(enum DISP_MODULE_ENUM module)
{
	unsigned long base = ovl_base_addr(module);
	unsigned long rg0 = 0, rg1 = 0, rg2 = 0, rg3 = 0, rg4 = 0;
	int i = 0;

	DDPDUMP("-- %s Golden Setting --\n", ddp_get_module_name(module));
	for (i = 0; i < ovl_layer_num(module); i++) {
		rg0 = DISP_REG_OVL_RDMA0_MEM_GMC_SETTING + i * OVL_LAYER_OFFSET;
		rg1 = DISP_REG_OVL_RDMA0_FIFO_CTRL + i * OVL_LAYER_OFFSET;
		rg2 = DISP_REG_OVL_RDMA0_MEM_GMC_S2 + i * 0x4;
		rg3 = DISP_REG_OVL_RDMAn_BUF_LOW(i);
		rg4 = DISP_REG_OVL_RDMAn_BUF_HIGH(i);
		DDPDUMP("0x%03lx:0x%08x 0x%03lx:0x%08x 0x%03lx:0x%08x 0x%03lx:0x%08x 0x%03lx:0x%08x\n",
			rg0, DISP_REG_GET(rg0 + base),
			rg1, DISP_REG_GET(rg1 + base),
			rg2, DISP_REG_GET(rg2 + base),
			rg3, DISP_REG_GET(rg3 + base),
			rg4, DISP_REG_GET(rg4 + base));
	}

	rg0 = DISP_REG_OVL_RDMA_GREQ_NUM;
	rg1 = DISP_REG_OVL_RDMA_GREQ_URG_NUM;
	rg2 = DISP_REG_OVL_RDMA_ULTRA_SRC;
	DDPDUMP("0x%03lx:0x%08x 0x%03lx:0x%08x 0x%03lx:0x%08x\n",
		rg0, DISP_REG_GET(rg0 + base),
		rg1, DISP_REG_GET(rg1 + base),
		rg2, DISP_REG_GET(rg2 + base));


	DDPDUMP("RDMA0_MEM_GMC_SETTING1\n");
	DDPDUMP("[9:0]:%x [25:16]:%x [28]:%x [31]:%x\n",
		DISP_REG_GET_FIELD(FLD_OVL_RDMA_MEM_GMC_ULTRA_THRESHOLD,
		base + DISP_REG_OVL_RDMA0_MEM_GMC_SETTING),
		DISP_REG_GET_FIELD(FLD_OVL_RDMA_MEM_GMC_PRE_ULTRA_THRESHOLD,
		base + DISP_REG_OVL_RDMA0_MEM_GMC_SETTING),
		DISP_REG_GET_FIELD(
		    FLD_OVL_RDMA_MEM_GMC_ULTRA_THRESHOLD_HIGH_OFS,
		base + DISP_REG_OVL_RDMA0_MEM_GMC_SETTING),
		DISP_REG_GET_FIELD(
		    FLD_OVL_RDMA_MEM_GMC_PRE_ULTRA_THRESHOLD_HIGH_OFS,
		base + DISP_REG_OVL_RDMA0_MEM_GMC_SETTING));


	DDPDUMP("RDMA0_FIFO_CTRL\n");
	DDPDUMP("[9:0]:%u [25:16]:%u\n",
		DISP_REG_GET_FIELD(FLD_OVL_RDMA_FIFO_THRD,
			base + DISP_REG_OVL_RDMA0_FIFO_CTRL),
		DISP_REG_GET_FIELD(FLD_OVL_RDMA_FIFO_SIZE,
			base + DISP_REG_OVL_RDMA0_FIFO_CTRL));

	DDPDUMP("RDMA0_MEM_GMC_SETTING2\n");
	DDPDUMP("[11:0]:%u [27:16]:%u [28]:%u [29]:%u [30]:%u\n",
		DISP_REG_GET_FIELD(FLD_OVL_RDMA_MEM_GMC2_ISSUE_REQ_THRES,
			base + DISP_REG_OVL_RDMA0_MEM_GMC_S2),
		DISP_REG_GET_FIELD(FLD_OVL_RDMA_MEM_GMC2_ISSUE_REQ_THRES_URG,
			base + DISP_REG_OVL_RDMA0_MEM_GMC_S2),
		DISP_REG_GET_FIELD(FLD_OVL_RDMA_MEM_GMC2_REQ_THRES_PREULTRA,
			base + DISP_REG_OVL_RDMA0_MEM_GMC_S2),
		DISP_REG_GET_FIELD(FLD_OVL_RDMA_MEM_GMC2_REQ_THRES_ULTRA,
			base + DISP_REG_OVL_RDMA0_MEM_GMC_S2),
		DISP_REG_GET_FIELD(FLD_OVL_RDMA_MEM_GMC2_FORCE_REQ_THRES,
			base + DISP_REG_OVL_RDMA0_MEM_GMC_S2));

	DDPDUMP("RDMA_GREQ_NUM\n");
	DDPDUMP("[3:0]%u [7:4]%u [11:8]%u [15:12]%u [23:16]%x [26:24]%u\n",
		DISP_REG_GET_FIELD(FLD_OVL_RDMA_GREQ_LAYER0_GREQ_NUM,
			base + DISP_REG_OVL_RDMA_GREQ_NUM),
		DISP_REG_GET_FIELD(FLD_OVL_RDMA_GREQ_LAYER1_GREQ_NUM,
			base + DISP_REG_OVL_RDMA_GREQ_NUM),
		DISP_REG_GET_FIELD(FLD_OVL_RDMA_GREQ_LAYER2_GREQ_NUM,
			base + DISP_REG_OVL_RDMA_GREQ_NUM),
		DISP_REG_GET_FIELD(FLD_OVL_RDMA_GREQ_LAYER3_GREQ_NUM,
			base + DISP_REG_OVL_RDMA_GREQ_NUM),
		DISP_REG_GET_FIELD(FLD_OVL_RDMA_GREQ_OSTD_GREQ_NUM,
			base + DISP_REG_OVL_RDMA_GREQ_NUM),
		DISP_REG_GET_FIELD(FLD_OVL_RDMA_GREQ_GREQ_DIS_CNT,
			base + DISP_REG_OVL_RDMA_GREQ_NUM));

	DDPDUMP("[27]%u [28]%u [29]%u [30]%u [31]%u\n",
		DISP_REG_GET_FIELD(FLD_OVL_RDMA_GREQ_STOP_EN,
			base + DISP_REG_OVL_RDMA_GREQ_NUM),
		DISP_REG_GET_FIELD(FLD_OVL_RDMA_GREQ_GRP_END_STOP,
			base + DISP_REG_OVL_RDMA_GREQ_NUM),
		DISP_REG_GET_FIELD(FLD_OVL_RDMA_GREQ_GRP_BRK_STOP,
			base + DISP_REG_OVL_RDMA_GREQ_NUM),
		DISP_REG_GET_FIELD(FLD_OVL_RDMA_GREQ_IOBUF_FLUSH_PREULTRA,
			base + DISP_REG_OVL_RDMA_GREQ_NUM),
		DISP_REG_GET_FIELD(FLD_OVL_RDMA_GREQ_IOBUF_FLUSH_ULTRA,
			base + DISP_REG_OVL_RDMA_GREQ_NUM));

	DDPDUMP("RDMA_GREQ_URG_NUM\n");
	DDPDUMP("[3:0]:%u [7:4]:%u [11:8]:%u [15:12]:%u [25:16]:%u [28]:%u\n",
		DISP_REG_GET_FIELD(FLD_OVL_RDMA_GREQ_LAYER0_GREQ_URG_NUM,
			base + DISP_REG_OVL_RDMA_GREQ_URG_NUM),
		DISP_REG_GET_FIELD(FLD_OVL_RDMA_GREQ_LAYER1_GREQ_URG_NUM,
			base + DISP_REG_OVL_RDMA_GREQ_URG_NUM),
		DISP_REG_GET_FIELD(FLD_OVL_RDMA_GREQ_LAYER2_GREQ_URG_NUM,
			base + DISP_REG_OVL_RDMA_GREQ_URG_NUM),
		DISP_REG_GET_FIELD(FLD_OVL_RDMA_GREQ_LAYER3_GREQ_URG_NUM,
			base + DISP_REG_OVL_RDMA_GREQ_URG_NUM),
		DISP_REG_GET_FIELD(FLD_OVL_RDMA_GREQ_ARG_GREQ_URG_TH,
			base + DISP_REG_OVL_RDMA_GREQ_URG_NUM),
		DISP_REG_GET_FIELD(FLD_OVL_RDMA_GREQ_ARG_URG_BIAS,
				base + DISP_REG_OVL_RDMA_GREQ_URG_NUM));

	DDPDUMP("RDMA_ULTRA_SRC\n");
	DDPDUMP("[1:0]%u [3:2]%u [5:4]%u [7:6]%u [9:8]%u\n",
		DISP_REG_GET_FIELD(FLD_OVL_RDMA_PREULTRA_BUF_SRC,
			base + DISP_REG_OVL_RDMA_ULTRA_SRC),
		DISP_REG_GET_FIELD(FLD_OVL_RDMA_PREULTRA_SMI_SRC,
			base + DISP_REG_OVL_RDMA_ULTRA_SRC),
		DISP_REG_GET_FIELD(FLD_OVL_RDMA_PREULTRA_ROI_END_SRC,
			base + DISP_REG_OVL_RDMA_ULTRA_SRC),
		DISP_REG_GET_FIELD(FLD_OVL_RDMA_PREULTRA_RDMA_SRC,
			base + DISP_REG_OVL_RDMA_ULTRA_SRC),
		DISP_REG_GET_FIELD(FLD_OVL_RDMA_ULTRA_BUF_SRC,
			base + DISP_REG_OVL_RDMA_ULTRA_SRC));
	DDPDUMP("[11:10]%u [13:12]%u [15:14]%u\n",
		DISP_REG_GET_FIELD(FLD_OVL_RDMA_ULTRA_SMI_SRC,
			base + DISP_REG_OVL_RDMA_ULTRA_SRC),
		DISP_REG_GET_FIELD(FLD_OVL_RDMA_ULTRA_ROI_END_SRC,
			base + DISP_REG_OVL_RDMA_ULTRA_SRC),
		DISP_REG_GET_FIELD(FLD_OVL_RDMA_ULTRA_RDMA_SRC,
			base + DISP_REG_OVL_RDMA_ULTRA_SRC));


	DDPDUMP("RDMA0_BUF_LOW\n");
	DDPDUMP("[11:0]:%x [23:12]:%x\n",
		DISP_REG_GET_FIELD(FLD_OVL_RDMA_BUF_LOW_ULTRA_TH,
			base + DISP_REG_OVL_RDMAn_BUF_LOW(0)),
		DISP_REG_GET_FIELD(FLD_OVL_RDMA_BUF_LOW_PREULTRA_TH,
			base + DISP_REG_OVL_RDMAn_BUF_LOW(0)));

	DDPDUMP("RDMA0_BUF_HIGH\n");
	DDPDUMP("[23:12]:%x [31]:%x\n",
		DISP_REG_GET_FIELD(FLD_OVL_RDMA_BUF_HIGH_PREULTRA_TH,
			base + DISP_REG_OVL_RDMAn_BUF_HIGH(0)),
		DISP_REG_GET_FIELD(FLD_OVL_RDMA_BUF_HIGH_PREULTRA_DIS,
			base + DISP_REG_OVL_RDMAn_BUF_HIGH(0)));

	DDPDUMP("DATAPATH_CON\n");
	DDPDUMP("[0]:%u, [3]:%u [24]:%u\n",
			DISP_REG_GET_FIELD(DATAPATH_CON_FLD_LAYER_SMI_ID_EN,
				base + DISP_REG_OVL_DATAPATH_CON),
			DISP_REG_GET_FIELD(DATAPATH_CON_FLD_OUTPUT_NO_RND,
				base + DISP_REG_OVL_DATAPATH_CON),
			DISP_REG_GET_FIELD(DATAPATH_CON_FLD_GCLAST_EN,
				base + DISP_REG_OVL_DATAPATH_CON));
}

void ovl_dump_reg(enum DISP_MODULE_ENUM module)
{
	if (disp_helper_get_option(DISP_OPT_REG_PARSER_RAW_DUMP)) {
		unsigned long module_base = ovl_base_addr(module);

		DDPDUMP("== START: DISP %s REGS ==\n",
			ddp_get_module_name(module));
		DDPDUMP("OVL0: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x0, INREG32(module_base + 0x0),
			0x4, INREG32(module_base + 0x4),
			0x8, INREG32(module_base + 0x8),
			0xC, INREG32(module_base + 0xC));
		DDPDUMP("OVL0: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x10, INREG32(module_base + 0x10),
			0x14, INREG32(module_base + 0x14),
			0x20, INREG32(module_base + 0x20),
			0x24, INREG32(module_base + 0x24));
		DDPDUMP("OVL0: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x28, INREG32(module_base + 0x28),
			0x2C, INREG32(module_base + 0x2C),
			0x30, INREG32(module_base + 0x30),
			0x34, INREG32(module_base + 0x34));
		DDPDUMP("OVL0: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x38, INREG32(module_base + 0x38),
			0x3C, INREG32(module_base + 0x3C),
			0xF40, INREG32(module_base + 0xF40),
			0x44, INREG32(module_base + 0x44));
		DDPDUMP("OVL0: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x48, INREG32(module_base + 0x48),
			0x4C, INREG32(module_base + 0x4C),
			0x50, INREG32(module_base + 0x50),
			0x54, INREG32(module_base + 0x54));
		DDPDUMP("OVL0: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x58, INREG32(module_base + 0x58),
			0x5C, INREG32(module_base + 0x5C),
			0xF60, INREG32(module_base + 0xF60),
			0x64, INREG32(module_base + 0x64));
		DDPDUMP("OVL0: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x68, INREG32(module_base + 0x68),
			0x6C, INREG32(module_base + 0x6C),
			0x70, INREG32(module_base + 0x70),
			0x74, INREG32(module_base + 0x74));
		DDPDUMP("OVL0: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x78, INREG32(module_base + 0x78),
			0x7C, INREG32(module_base + 0x7C),
			0xF80, INREG32(module_base + 0xF80),
			0x84, INREG32(module_base + 0x84));
		DDPDUMP("OVL0: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x88, INREG32(module_base + 0x88),
			0x8C, INREG32(module_base + 0x8C),
			0x90, INREG32(module_base + 0x90),
			0x94, INREG32(module_base + 0x94));
		DDPDUMP("OVL0: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x98, INREG32(module_base + 0x98),
			0x9C, INREG32(module_base + 0x9C),
			0xFa0, INREG32(module_base + 0xFa0),
			0xa4, INREG32(module_base + 0xa4));
		DDPDUMP("OVL0: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0xa8, INREG32(module_base + 0xa8),
			0xAC, INREG32(module_base + 0xAC),
			0xc0, INREG32(module_base + 0xc0),
			0xc8, INREG32(module_base + 0xc8));
		DDPDUMP("OVL0: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0xcc, INREG32(module_base + 0xcc),
			0xd0, INREG32(module_base + 0xd0),
			0xe0, INREG32(module_base + 0xe0),
			0xe8, INREG32(module_base + 0xe8));
		DDPDUMP("OVL0: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0xec, INREG32(module_base + 0xec),
			0xf0, INREG32(module_base + 0xf0),
			0x100, INREG32(module_base + 0x100),
			0x108, INREG32(module_base + 0x108));
		DDPDUMP("OVL0: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x10c, INREG32(module_base + 0x10c),
			0x110, INREG32(module_base + 0x110),
			0x120, INREG32(module_base + 0x120),
			0x128, INREG32(module_base + 0x128));
		DDPDUMP("OVL0: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x12c, INREG32(module_base + 0x12c),
			0x130, INREG32(module_base + 0x130),
			0x134, INREG32(module_base + 0x134),
			0x138, INREG32(module_base + 0x138));
		DDPDUMP("OVL0: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x13c, INREG32(module_base + 0x13c),
			0x140, INREG32(module_base + 0x140),
			0x144, INREG32(module_base + 0x144),
			0x148, INREG32(module_base + 0x148));
		DDPDUMP("OVL0: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x14c, INREG32(module_base + 0x14c),
			0x150, INREG32(module_base + 0x150),
			0x154, INREG32(module_base + 0x154),
			0x158, INREG32(module_base + 0x158));
		DDPDUMP("OVL0: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x15c, INREG32(module_base + 0x15c),
			0x160, INREG32(module_base + 0x160),
			0x164, INREG32(module_base + 0x164),
			0x168, INREG32(module_base + 0x168));
		DDPDUMP("OVL0: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x16c, INREG32(module_base + 0x16c),
			0x170, INREG32(module_base + 0x170),
			0x174, INREG32(module_base + 0x174),
			0x178, INREG32(module_base + 0x178));
		DDPDUMP("OVL0: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x17c, INREG32(module_base + 0x17c),
			0x180, INREG32(module_base + 0x180),
			0x184, INREG32(module_base + 0x184),
			0x188, INREG32(module_base + 0x188));
		DDPDUMP("OVL0: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x18c, INREG32(module_base + 0x18c),
			0x190, INREG32(module_base + 0x190),
			0x194, INREG32(module_base + 0x194),
			0x198, INREG32(module_base + 0x198));
		DDPDUMP("OVL0: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x19c, INREG32(module_base + 0x19c),
			0x1a0, INREG32(module_base + 0x1a0),
			0x1a4, INREG32(module_base + 0x1a4),
			0x1a8, INREG32(module_base + 0x1a8));
		DDPDUMP("OVL0: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x1ac, INREG32(module_base + 0x1ac),
			0x1b0, INREG32(module_base + 0x1b0),
			0x1b4, INREG32(module_base + 0x1b4),
			0x1b8, INREG32(module_base + 0x1b8));
		DDPDUMP("OVL0: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x1bc, INREG32(module_base + 0x1bc),
			0x1c0, INREG32(module_base + 0x1c0),
			0x1c4, INREG32(module_base + 0x1c4),
			0x1c8, INREG32(module_base + 0x1c8));
		DDPDUMP("OVL0: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x1cc, INREG32(module_base + 0x1cc),
			0x1d0, INREG32(module_base + 0x1d0),
			0x1d4, INREG32(module_base + 0x1d4),
			0x1e0, INREG32(module_base + 0x1e0));
		DDPDUMP("OVL0: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x1e4, INREG32(module_base + 0x1e4),
			0x1e8, INREG32(module_base + 0x1e8),
			0x1ec, INREG32(module_base + 0x1ec),
			0x1F0, INREG32(module_base + 0x1F0));
		DDPDUMP("OVL0: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x1F4, INREG32(module_base + 0x1F4),
			0x1F8, INREG32(module_base + 0x1F8),
			0x1FC, INREG32(module_base + 0x1FC),
			0x200, INREG32(module_base + 0x200));
		DDPDUMP("OVL0: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x208, INREG32(module_base + 0x208),
			0x20C, INREG32(module_base + 0x20C),
			0x210, INREG32(module_base + 0x210),
			0x214, INREG32(module_base + 0x214));
		DDPDUMP("OVL0: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x218, INREG32(module_base + 0x218),
			0x21C, INREG32(module_base + 0x21C),
			0x230, INREG32(module_base + 0x230),
			0x234, INREG32(module_base + 0x234));
		DDPDUMP("OVL0: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x238, INREG32(module_base + 0x238),
			0x240, INREG32(module_base + 0x240),
			0x244, INREG32(module_base + 0x244),
			0x24c, INREG32(module_base + 0x24c));
		DDPDUMP("OVL0: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x250, INREG32(module_base + 0x250),
			0x254, INREG32(module_base + 0x254),
			0x258, INREG32(module_base + 0x258),
			0x25c, INREG32(module_base + 0x25c));
		DDPDUMP("OVL0: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x260, INREG32(module_base + 0x260),
			0x264, INREG32(module_base + 0x264),
			0x268, INREG32(module_base + 0x268),
			0x26C, INREG32(module_base + 0x26C));
		DDPDUMP("OVL0: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x270, INREG32(module_base + 0x270),
			0x280, INREG32(module_base + 0x280),
			0x284, INREG32(module_base + 0x284),
			0x288, INREG32(module_base + 0x288));
		DDPDUMP("OVL0: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x28C, INREG32(module_base + 0x28C),
			0x290, INREG32(module_base + 0x290),
			0x29C, INREG32(module_base + 0x29C),
			0x2A0, INREG32(module_base + 0x2A0));
		DDPDUMP("OVL0: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x2A4, INREG32(module_base + 0x2A4),
			0x2B0, INREG32(module_base + 0x2B0),
			0x2B4, INREG32(module_base + 0x2B4),
			0x2B8, INREG32(module_base + 0x2B8));
		DDPDUMP("OVL0: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x2BC, INREG32(module_base + 0x2BC),
			0x2C0, INREG32(module_base + 0x2C0),
			0x2C4, INREG32(module_base + 0x2C4),
			0x2C8, INREG32(module_base + 0x2C8));
		DDPDUMP("OVL0: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x324, INREG32(module_base + 0x324),
			0x330, INREG32(module_base + 0x330),
			0x334, INREG32(module_base + 0x334),
			0x338, INREG32(module_base + 0x338));
		DDPDUMP("OVL0: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x33C, INREG32(module_base + 0x33C),
			0xFB0, INREG32(module_base + 0xFB0),
			0x344, INREG32(module_base + 0x344),
			0x348, INREG32(module_base + 0x348));
		DDPDUMP("OVL0: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x34C, INREG32(module_base + 0x34C),
			0x350, INREG32(module_base + 0x350),
			0x354, INREG32(module_base + 0x354),
			0x358, INREG32(module_base + 0x358));
		DDPDUMP("OVL0: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x35C, INREG32(module_base + 0x35C),
			0xFB4, INREG32(module_base + 0xFB4),
			0x364, INREG32(module_base + 0x364),
			0x368, INREG32(module_base + 0x368));
		DDPDUMP("OVL0: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x36C, INREG32(module_base + 0x36C),
			0x370, INREG32(module_base + 0x370),
			0x374, INREG32(module_base + 0x374),
			0x378, INREG32(module_base + 0x378));
		DDPDUMP("OVL0: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x37C, INREG32(module_base + 0x37C),
			0xFB8, INREG32(module_base + 0xFB8),
			0x384, INREG32(module_base + 0x384),
			0x388, INREG32(module_base + 0x388));
		DDPDUMP("OVL0: 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x, 0x%04x=0x%08x\n",
			0x38C, INREG32(module_base + 0x38C),
			0x390, INREG32(module_base + 0x390),
			0x394, INREG32(module_base + 0x394),
			0x398, INREG32(module_base + 0x398));
		DDPDUMP("OVL0: 0x%04x=0x%08x\n",
			0xFC0, INREG32(module_base + 0xFC0));
		DDPDUMP("-- END: DISP %s REGS --\n",
			ddp_get_module_name(module));
	} else {
		unsigned int i = 0;
		unsigned long base = ovl_base_addr(module);
		unsigned int src_on = DISP_REG_GET(DISP_REG_OVL_SRC_CON + base);

		DDPDUMP("== DISP %s REGS ==\n", ddp_get_module_name(module));
		DDPDUMP("0x%03lx:0x%08x 0x%08x 0x%08x 0x%08x\n",
			DISP_REG_OVL_STA,
			DISP_REG_GET(DISP_REG_OVL_STA + base),
			DISP_REG_GET(DISP_REG_OVL_INTEN + base),
			DISP_REG_GET(DISP_REG_OVL_INTSTA + base),
			DISP_REG_GET(DISP_REG_OVL_EN + base));
		DDPDUMP("0x%03lx:0x%08x 0x%08x\n",
			DISP_REG_OVL_TRIG,
			DISP_REG_GET(DISP_REG_OVL_TRIG + base),
			DISP_REG_GET(DISP_REG_OVL_RST + base));
		DDPDUMP("0x%03lx:0x%08x 0x%08x 0x%08x 0x%08x\n",
			DISP_REG_OVL_ROI_SIZE,
			DISP_REG_GET(DISP_REG_OVL_ROI_SIZE + base),
			DISP_REG_GET(DISP_REG_OVL_DATAPATH_CON + base),
			DISP_REG_GET(DISP_REG_OVL_ROI_BGCLR + base),
			DISP_REG_GET(DISP_REG_OVL_SRC_CON + base));

		for (i = 0; i < ovl_layer_num(module); i++) {
			unsigned long l_addr = 0;

			if (!(src_on & (0x1 << i)))
				continue;

			l_addr = i * OVL_LAYER_OFFSET + base;
			DDPDUMP("0x%03x:0x%08x 0x%08x 0x%08x 0x%08x\n",
				0x030 + i * 0x20,
				DISP_REG_GET(DISP_REG_OVL_L0_CON + l_addr),
				DISP_REG_GET(DISP_REG_OVL_L0_SRCKEY + l_addr),
				DISP_REG_GET(DISP_REG_OVL_L0_SRC_SIZE + l_addr),
				DISP_REG_GET(DISP_REG_OVL_L0_OFFSET + l_addr));

			DDPDUMP("0x%03x:0x%08x 0x%03x:0x%08x\n",
				0xf40 + i * 0x20,
				DISP_REG_GET(DISP_REG_OVL_L0_ADDR + l_addr),
				0x044 + i * 0x20,
				DISP_REG_GET(DISP_REG_OVL_L0_PITCH + l_addr));

			DDPDUMP("0x%03x:0x%08x 0x%03x:0x%08x\n",
				0x0c0 + i * 0x20,
				DISP_REG_GET(DISP_REG_OVL_RDMA0_CTRL + l_addr),
				0x24c + i * 0x4,
				DISP_REG_GET(DISP_REG_OVL_RDMA0_DBG +
					     base + i * 0x4));
		}

		DDPDUMP("0x1d4:0x%08x 0x200:0x%08x 0x208:0x%08x\n",
			DISP_REG_GET(DISP_REG_OVL_DEBUG_MON_SEL + base),
			DISP_REG_GET(DISP_REG_OVL_DUMMY_REG + base),
			DISP_REG_GET(DISP_REG_OVL_GDRDY_PRD + base));

		DDPDUMP("0x230:0x%08x 0x%08x 0x240:0x%08x 0x%08x 0x2a0:0x%08x 0x%08x\n",
			DISP_REG_GET(DISP_REG_OVL_SMI_DBG + base),
			DISP_REG_GET(DISP_REG_OVL_GREQ_LAYER_CNT + base),
			DISP_REG_GET(DISP_REG_OVL_FLOW_CTRL_DBG + base),
			DISP_REG_GET(DISP_REG_OVL_ADDCON_DBG + base),
			DISP_REG_GET(DISP_REG_OVL_FUNC_DCM0 + base),
			DISP_REG_GET(DISP_REG_OVL_FUNC_DCM1 + base));

		ovl_dump_golden_setting(module);
	}
}

static void ovl_printf_status(unsigned int status)
{
	DDPDUMP("- OVL_FLOW_CONTROL_DEBUG -\n");
	DDPDUMP("addcon_idle:%d,blend_idle:%d,out_valid:%d,out_ready:%d,out_idle:%d\n",
		(status >> 10) & (0x1), (status >> 11) & (0x1),
		(status >> 12) & (0x1), (status >> 13) & (0x1),
		(status >> 15) & (0x1));
	DDPDUMP("rdma_idle3-0:(%d,%d,%d,%d),rst:%d\n",
		(status >> 16) & (0x1), (status >> 17) & (0x1),
		(status >> 18) & (0x1), (status >> 19) & (0x1),
		(status >> 20) & (0x1));
	DDPDUMP("trig:%d,frame_hwrst_done:%d,frame_swrst_done:%d,frame_underrun:%d,frame_done:%d\n",
		(status >> 21) & (0x1), (status >> 23) & (0x1),
		(status >> 24) & (0x1), (status >> 25) & (0x1),
		(status >> 26) & (0x1));
	DDPDUMP("ovl_running:%d,ovl_start:%d,ovl_clr:%d,reg_update:%d,ovl_upd_reg:%d\n",
		(status >> 27) & (0x1), (status >> 28) & (0x1),
		(status >> 29) & (0x1), (status >> 30) & (0x1),
		(status >> 31) & (0x1));

	DDPDUMP("ovl_fms_state:\n");
	switch (status & 0x3ff) {
	case 0x1:
		DDPDUMP("idle\n");
		break;
	case 0x2:
		DDPDUMP("wait_SOF\n");
		break;
	case 0x4:
		DDPDUMP("prepare\n");
		break;
	case 0x8:
		DDPDUMP("reg_update\n");
		break;
	case 0x10:
		DDPDUMP("eng_clr(internal reset)\n");
		break;
	case 0x20:
		DDPDUMP("eng_act(processing)\n");
		break;
	case 0x40:
		DDPDUMP("h_wait_w_rst\n");
		break;
	case 0x80:
		DDPDUMP("s_wait_w_rst\n");
		break;
	case 0x100:
		DDPDUMP("h_w_rst\n");
		break;
	case 0x200:
		DDPDUMP("s_w_rst\n");
		break;
	default:
		DDPDUMP("ovl_fsm_unknown\n");
		break;
	}
}

static void ovl_print_ovl_rdma_status(unsigned int status)
{
	DDPDUMP("warm_rst_cs:%d,layer_greq:%d,out_data:0x%x\n",
		status & 0x7, (status >> 3) & 0x1, (status >> 4) & 0xffffff);
	DDPDUMP("out_ready:%d,out_valid:%d,smi_busy:%d,smi_greq:%d\n",
		(status >> 28) & 0x1, (status >> 29) & 0x1,
		(status >> 30) & 0x1, (status >> 31) & 0x1);
}

static void ovl_dump_layer_info(enum DISP_MODULE_ENUM module, int layer,
				bool is_ext_layer)
{
	unsigned int con, src_size, offset, pitch, addr;
	enum UNIFIED_COLOR_FMT fmt;
	unsigned long baddr = ovl_base_addr(module);
	unsigned long Lx_base = 0;
	unsigned long Lx_addr_base = 0;

	if (is_ext_layer) {
		Lx_base = baddr + layer * OVL_LAYER_OFFSET;
		Lx_base += (DISP_REG_OVL_EL0_CON - DISP_REG_OVL_L0_CON);

		Lx_addr_base = baddr + layer * 0x4;
		Lx_addr_base += (DISP_REG_OVL_EL0_ADDR - DISP_REG_OVL_L0_ADDR);
	} else {
		Lx_base = baddr + layer * OVL_LAYER_OFFSET;
		Lx_addr_base = baddr + layer * OVL_LAYER_OFFSET;
	}

	con = DISP_REG_GET(DISP_REG_OVL_L0_CON + Lx_base);
	offset = DISP_REG_GET(DISP_REG_OVL_L0_OFFSET + Lx_base);
	src_size = DISP_REG_GET(DISP_REG_OVL_L0_SRC_SIZE + Lx_base);
	pitch = DISP_REG_GET(DISP_REG_OVL_L0_PITCH + Lx_base);
	addr = DISP_REG_GET(DISP_REG_OVL_L0_ADDR + Lx_addr_base);

	fmt = display_fmt_reg_to_unified_fmt(
			REG_FLD_VAL_GET(L_CON_FLD_CFMT, con),
			REG_FLD_VAL_GET(L_CON_FLD_BTSW, con),
			REG_FLD_VAL_GET(L_CON_FLD_RGB_SWAP, con));

	DDPDUMP("%s_L%d:(%u,%u,%ux%u),pitch=%u,addr=0x%08x,fmt=%s,source=%s,aen=%u,alpha=%u\n",
		is_ext_layer ? "ext" : "phy", layer,
		offset & 0xfff, (offset >> 16) & 0xfff,
		src_size & 0xfff, (src_size >> 16) & 0xfff,
		pitch & 0xffff, addr, unified_color_fmt_name(fmt),
		(REG_FLD_VAL_GET(L_CON_FLD_LSRC, con) == 0) ?
			"mem" : "constant_color",
		REG_FLD_VAL_GET(L_CON_FLD_AEN, con),
		REG_FLD_VAL_GET(L_CON_FLD_APHA, con));
}

void ovl_dump_analysis(enum DISP_MODULE_ENUM module)
{
	int i = 0;
	unsigned long Lx_base = 0;
	unsigned long rdma_offset = 0;
	unsigned long baddr = ovl_base_addr(module);
	unsigned int src_con = DISP_REG_GET(DISP_REG_OVL_SRC_CON + baddr);
	unsigned int ext_con = DISP_REG_GET(DISP_REG_OVL_DATAPATH_EXT_CON +
					    baddr);
	unsigned int addcon = DISP_REG_GET(DISP_REG_OVL_ADDCON_DBG + baddr);

	DDPDUMP("== DISP %s ANALYSIS ==\n", ddp_get_module_name(module));
	DDPDUMP("ovl_en=%d,layer_en(%d,%d,%d,%d),bg(%dx%d)\n",
		DISP_REG_GET(DISP_REG_OVL_EN + baddr) & 0x1,
		src_con & 0x1, (src_con >> 1) & 0x1,
		(src_con >> 2) & 0x1, (src_con >> 3) & 0x1,
		DISP_REG_GET(DISP_REG_OVL_ROI_SIZE + baddr) & 0xfff,
		(DISP_REG_GET(DISP_REG_OVL_ROI_SIZE + baddr) >> 16) & 0xfff);
	DDPDUMP("ext_layer:layer_en(%d,%d,%d),attach_layer(%d,%d,%d)\n",
		ext_con & 0x1, (ext_con >> 1) & 0x1, (ext_con >> 2) & 0x1,
		(ext_con >> 16) & 0xf, (ext_con >> 20) & 0xf,
		(ext_con >> 24) & 0xf);
	DDPDUMP("cur_pos(%u,%u),layer_hit(%u,%u,%u,%u),bg_mode=%s,sta=0x%x\n",
		REG_FLD_VAL_GET(ADDCON_DBG_FLD_ROI_X, addcon),
		REG_FLD_VAL_GET(ADDCON_DBG_FLD_ROI_Y, addcon),
		REG_FLD_VAL_GET(ADDCON_DBG_FLD_L0_WIN_HIT, addcon),
		REG_FLD_VAL_GET(ADDCON_DBG_FLD_L1_WIN_HIT, addcon),
		REG_FLD_VAL_GET(ADDCON_DBG_FLD_L2_WIN_HIT, addcon),
		REG_FLD_VAL_GET(ADDCON_DBG_FLD_L3_WIN_HIT, addcon),
		DISP_REG_GET_FIELD(DATAPATH_CON_FLD_BGCLR_IN_SEL,
				   DISP_REG_OVL_DATAPATH_CON + baddr) ?
				   "DL" : "const",
		DISP_REG_GET(DISP_REG_OVL_STA + baddr));

	/* phy layer */
	for (i = 0; i < ovl_layer_num(module); i++) {
		unsigned int rdma_ctrl;

		if (src_con & (0x1 << i))
			ovl_dump_layer_info(module, i, false);
		else
			DDPDUMP("phy_L%d:disabled\n", i);

		Lx_base = i * OVL_LAYER_OFFSET + baddr;
		rdma_ctrl = DISP_REG_GET(Lx_base + DISP_REG_OVL_RDMA0_CTRL);
		DDPDUMP("ovl rdma%d status:(en=%d,fifo_used:%d,GMC=0x%x)\n",
			i, REG_FLD_VAL_GET(RDMA0_CTRL_FLD_RDMA_EN, rdma_ctrl),
			REG_FLD_VAL_GET(RDMA0_CTRL_FLD_RMDA_FIFO_USED_SZ,
					rdma_ctrl),
			DISP_REG_GET(Lx_base +
				     DISP_REG_OVL_RDMA0_MEM_GMC_SETTING));

		rdma_offset = i * OVL_RDMA_DEBUG_OFFSET + baddr;
		ovl_print_ovl_rdma_status(DISP_REG_GET(DISP_REG_OVL_RDMA0_DBG +
						       rdma_offset));
	}

	/* ext layer */
	for (i = 0; i < 3; i++) {
		if (ext_con & (0x1 << i))
			ovl_dump_layer_info(module, i, true);
		else
			DDPDUMP("ext_L%d:disabled\n", i);
	}
	ovl_printf_status(DISP_REG_GET(DISP_REG_OVL_FLOW_CTRL_DBG + baddr));
}

int ovl_dump(enum DISP_MODULE_ENUM module, int level)
{
	ovl_dump_analysis(module);
	ovl_dump_reg(module);

	return 0;
}

static int ovl_golden_setting(enum DISP_MODULE_ENUM module,
			      enum dst_module_type dst_mod_type, void *cmdq)
{
	unsigned long baddr = ovl_base_addr(module);
	unsigned int regval;
	int i, layer_num;
	int is_large_resolution = 0;
	unsigned int layer_greq_num;
	unsigned int dst_w, dst_h;

	layer_num = ovl_layer_num(module);

	dst_w = primary_display_get_width();
	dst_h = primary_display_get_height();

	if (dst_w > 1260 && dst_h > 2240) {
		/* WQHD */
		is_large_resolution = 1;
	} else {
		/* FHD */
		is_large_resolution = 0;
	}

	/* DISP_REG_OVL_RDMA0_MEM_GMC_SETTING */
	regval = REG_FLD_VAL(FLD_OVL_RDMA_MEM_GMC_ULTRA_THRESHOLD, 0x3ff);
	if (dst_mod_type == DST_MOD_REAL_TIME)
		regval |= REG_FLD_VAL(FLD_OVL_RDMA_MEM_GMC_PRE_ULTRA_THRESHOLD,
				      0x3ff);
	else
		regval |= REG_FLD_VAL(FLD_OVL_RDMA_MEM_GMC_PRE_ULTRA_THRESHOLD,
				      0xe0);

	for (i = 0; i < layer_num; i++) {
		unsigned long Lx_base = i * OVL_LAYER_OFFSET + baddr;

		DISP_REG_SET(cmdq, Lx_base +
			     DISP_REG_OVL_RDMA0_MEM_GMC_SETTING, regval);
	}

	/* DISP_REG_OVL_RDMA0_FIFO_CTRL */
	regval = REG_FLD_VAL(FLD_OVL_RDMA_FIFO_SIZE, 288);
	for (i = 0; i < layer_num; i++) {
		unsigned long Lx_base = i * OVL_LAYER_OFFSET + baddr;

		DISP_REG_SET(cmdq, Lx_base + DISP_REG_OVL_RDMA0_FIFO_CTRL,
			     regval);
	}

	/* DISP_REG_OVL_RDMA0_MEM_GMC_S2 */
	regval = 0;
	if (dst_mod_type == DST_MOD_REAL_TIME) {
		regval |= REG_FLD_VAL(FLD_OVL_RDMA_MEM_GMC2_ISSUE_REQ_THRES,
				      191);
		regval |= REG_FLD_VAL(FLD_OVL_RDMA_MEM_GMC2_ISSUE_REQ_THRES_URG,
				      95);
	} else {
		/* decouple */
		regval |= REG_FLD_VAL(FLD_OVL_RDMA_MEM_GMC2_ISSUE_REQ_THRES,
				      15);
		regval |= REG_FLD_VAL(FLD_OVL_RDMA_MEM_GMC2_ISSUE_REQ_THRES_URG,
				      15);
	}
	regval |= REG_FLD_VAL(FLD_OVL_RDMA_MEM_GMC2_REQ_THRES_PREULTRA, 0);
	regval |= REG_FLD_VAL(FLD_OVL_RDMA_MEM_GMC2_REQ_THRES_ULTRA, 1);
	regval |= REG_FLD_VAL(FLD_OVL_RDMA_MEM_GMC2_FORCE_REQ_THRES, 0);

	for (i = 0; i < layer_num; i++)
		DISP_REG_SET(cmdq, baddr + DISP_REG_OVL_RDMA0_MEM_GMC_S2 +
			     i * 4, regval);

	/* DISP_REG_OVL_RDMA_GREQ_NUM */
	if (dst_mod_type == DST_MOD_REAL_TIME)
		layer_greq_num = 11;
	else
		layer_greq_num = 1;

	regval = REG_FLD_VAL(FLD_OVL_RDMA_GREQ_LAYER0_GREQ_NUM, layer_greq_num);
	if (layer_num > 1)
		regval |= REG_FLD_VAL(FLD_OVL_RDMA_GREQ_LAYER1_GREQ_NUM,
				      layer_greq_num);
	if (layer_num > 2)
		regval |= REG_FLD_VAL(FLD_OVL_RDMA_GREQ_LAYER2_GREQ_NUM,
				      layer_greq_num);
	if (layer_num > 3)
		regval |= REG_FLD_VAL(FLD_OVL_RDMA_GREQ_LAYER3_GREQ_NUM,
				      layer_greq_num);

	regval |= REG_FLD_VAL(FLD_OVL_RDMA_GREQ_OSTD_GREQ_NUM, 0xff);
	regval |= REG_FLD_VAL(FLD_OVL_RDMA_GREQ_GREQ_DIS_CNT, 1);
	regval |= REG_FLD_VAL(FLD_OVL_RDMA_GREQ_STOP_EN, 0);
	regval |= REG_FLD_VAL(FLD_OVL_RDMA_GREQ_GRP_END_STOP, 1);
	regval |= REG_FLD_VAL(FLD_OVL_RDMA_GREQ_GRP_BRK_STOP, 1);
	regval |= REG_FLD_VAL(FLD_OVL_RDMA_GREQ_IOBUF_FLUSH_PREULTRA, 1);
	regval |= REG_FLD_VAL(FLD_OVL_RDMA_GREQ_IOBUF_FLUSH_ULTRA, 1);
	DISP_REG_SET(cmdq, baddr + DISP_REG_OVL_RDMA_GREQ_NUM, regval);

	/* DISP_REG_OVL_RDMA_GREQ_URG_NUM */
	if (dst_mod_type == DST_MOD_REAL_TIME)
		layer_greq_num = 11;
	else
		layer_greq_num = 1;

	regval = REG_FLD_VAL(FLD_OVL_RDMA_GREQ_LAYER0_GREQ_URG_NUM,
			     layer_greq_num);
	if (layer_num > 0)
		regval |= REG_FLD_VAL(FLD_OVL_RDMA_GREQ_LAYER1_GREQ_URG_NUM,
				      layer_greq_num);
	if (layer_num > 1)
		regval |= REG_FLD_VAL(FLD_OVL_RDMA_GREQ_LAYER2_GREQ_URG_NUM,
				      layer_greq_num);
	if (layer_num > 2)
		regval |= REG_FLD_VAL(FLD_OVL_RDMA_GREQ_LAYER3_GREQ_URG_NUM,
				      layer_greq_num);

	regval |= REG_FLD_VAL(FLD_OVL_RDMA_GREQ_ARG_GREQ_URG_TH, 0);
	regval |= REG_FLD_VAL(FLD_OVL_RDMA_GREQ_ARG_URG_BIAS, 0);
	DISP_REG_SET(cmdq, baddr + DISP_REG_OVL_RDMA_GREQ_URG_NUM, regval);

	/* DISP_REG_OVL_RDMA_ULTRA_SRC */
	regval = REG_FLD_VAL(FLD_OVL_RDMA_PREULTRA_BUF_SRC, 0);
	regval |= REG_FLD_VAL(FLD_OVL_RDMA_PREULTRA_SMI_SRC, 0);
	regval |= REG_FLD_VAL(FLD_OVL_RDMA_PREULTRA_ROI_END_SRC, 0);
	regval |= REG_FLD_VAL(FLD_OVL_RDMA_PREULTRA_RDMA_SRC, 1);
	regval |= REG_FLD_VAL(FLD_OVL_RDMA_ULTRA_BUF_SRC, 0);
	regval |= REG_FLD_VAL(FLD_OVL_RDMA_ULTRA_SMI_SRC, 0);
	if (dst_mod_type == DST_MOD_REAL_TIME)
		regval |= REG_FLD_VAL(FLD_OVL_RDMA_ULTRA_ROI_END_SRC, 0);
	else
		regval |= REG_FLD_VAL(FLD_OVL_RDMA_ULTRA_ROI_END_SRC, 2);
	regval |= REG_FLD_VAL(FLD_OVL_RDMA_ULTRA_RDMA_SRC, 2);
	DISP_REG_SET(cmdq, baddr + DISP_REG_OVL_RDMA_ULTRA_SRC, regval);

	/* DISP_REG_OVL_RDMAn_BUF_LOW */
	regval = REG_FLD_VAL(FLD_OVL_RDMA_BUF_LOW_ULTRA_TH, 0);
	if (dst_mod_type == DST_MOD_REAL_TIME)
		regval |= REG_FLD_VAL(FLD_OVL_RDMA_BUF_LOW_PREULTRA_TH, 0);
	else
		regval |= REG_FLD_VAL(FLD_OVL_RDMA_BUF_LOW_PREULTRA_TH, 36);

	for (i = 0; i < layer_num; i++)
		DISP_REG_SET(cmdq, baddr + DISP_REG_OVL_RDMAn_BUF_LOW(i),
			     regval);

	/* DISP_REG_OVL_RDMAn_BUF_HIGH */
	regval = REG_FLD_VAL(FLD_OVL_RDMA_BUF_HIGH_PREULTRA_DIS, 1);
	if (dst_mod_type == DST_MOD_REAL_TIME)
		regval |= REG_FLD_VAL(FLD_OVL_RDMA_BUF_HIGH_PREULTRA_TH, 0);
	else
		regval |= REG_FLD_VAL(FLD_OVL_RDMA_BUF_HIGH_PREULTRA_TH, 216);

	for (i = 0; i < layer_num; i++)
		DISP_REG_SET(cmdq, baddr + DISP_REG_OVL_RDMAn_BUF_HIGH(i),
			     regval);

	/* DISP_REG_OVL_FUNC_DCM0 */
	DISP_REG_SET(cmdq, baddr + DISP_REG_OVL_FUNC_DCM0, 0);

	/* DISP_REG_OVL_FUNC_DCM1 */
	DISP_REG_SET(cmdq, baddr + DISP_REG_OVL_FUNC_DCM1, 0);

	/* DISP_REG_OVL_DATAPATH_CON */
	/* GCLAST_EN is set @ ovl_start() */

	return 0;
}

int ovl_partial_update(enum DISP_MODULE_ENUM module, unsigned int bg_w,
		       unsigned int bg_h, void *handle)
{
	unsigned long baddr = ovl_base_addr(module);

	if ((bg_w > OVL_MAX_WIDTH) || (bg_h > OVL_MAX_HEIGHT)) {
		DDP_PR_ERR("ovl_roi,exceed OVL max size, w=%d, h=%d\n",
			   bg_w, bg_h);
		ASSERT(0);
	}
	DDPDBG("%s partial update\n", ddp_get_module_name(module));
	DISP_REG_SET(handle, baddr + DISP_REG_OVL_ROI_SIZE, bg_h << 16 | bg_w);

	return 0;
}

int ovl_addr_mva_replacement(enum DISP_MODULE_ENUM module,
			     struct ddp_fb_info *p_fb_info, void *handle)
{
	unsigned long base = ovl_base_addr(module);
	unsigned int src_on = DISP_REG_GET(DISP_REG_OVL_SRC_CON + base);
	unsigned int layer_addr, layer_mva;

	if (src_on & 0x1) {
		layer_addr = DISP_REG_GET(DISP_REG_OVL_L0_ADDR + base);
		layer_mva = layer_addr - p_fb_info->fb_pa + p_fb_info->fb_mva;
		DISP_REG_SET(handle, DISP_REG_OVL_L0_ADDR + base, layer_mva);
	}

	if (src_on & 0x2) {
		layer_addr = DISP_REG_GET(DISP_REG_OVL_L1_ADDR + base);
		layer_mva = layer_addr - p_fb_info->fb_pa + p_fb_info->fb_mva;
		DISP_REG_SET(handle, DISP_REG_OVL_L1_ADDR + base, layer_mva);
	}

	if (src_on & 0x4) {
		layer_addr = DISP_REG_GET(DISP_REG_OVL_L2_ADDR + base);
		layer_mva = layer_addr - p_fb_info->fb_pa + p_fb_info->fb_mva;
		DISP_REG_SET(handle, DISP_REG_OVL_L2_ADDR + base, layer_mva);
	}

	if (src_on & 0x8) {
		layer_addr = DISP_REG_GET(DISP_REG_OVL_L3_ADDR + base);
		layer_mva = layer_addr - p_fb_info->fb_pa + p_fb_info->fb_mva;
		DISP_REG_SET(handle, DISP_REG_OVL_L3_ADDR + base, layer_mva);
	}

	return 0;
}

static int ovl_ioctl(enum DISP_MODULE_ENUM module, void *handle,
		     enum DDP_IOCTL_NAME ioctl_cmd, void *params)
{
	int ret = 0;

	if (ioctl_cmd == DDP_OVL_GOLDEN_SETTING) {
		struct ddp_io_golden_setting_arg *gset_arg = params;

		ovl_golden_setting(module, gset_arg->dst_mod_type, handle);
	} else if (ioctl_cmd == DDP_PARTIAL_UPDATE) {
		struct disp_rect *roi = (struct disp_rect *)params;

		ovl_partial_update(module, roi->width, roi->height, handle);
	} else if (ioctl_cmd == DDP_OVL_MVA_REPLACEMENT) {
		struct ddp_fb_info *p_fb_info = (struct ddp_fb_info *)params;

		ovl_addr_mva_replacement(module, p_fb_info, handle);
	} else {
		ret = -1;
	}

	return ret;
}

/* ---------------- driver ---------------- */
struct DDP_MODULE_DRIVER ddp_driver_ovl = {
	.init = ovl_init,
	.deinit = ovl_deinit,
	.config = ovl_config_l,
	.start = ovl_start,
	.trigger = NULL,
	.stop = ovl_stop,
	.reset = ovl_reset,
	.power_on = ovl_clock_on,
	.power_off = ovl_clock_off,
	.is_idle = NULL,
	.is_busy = NULL,
	.dump_info = ovl_dump,
	.bypass = NULL,
	.build_cmdq = NULL,
	.set_lcm_utils = NULL,
	.ioctl = ovl_ioctl,
	.connect = ovl_connect,
	.switch_to_nonsec = ovl_switch_to_nonsec,
};
