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
#ifdef CONFIG_MTK_CLKMGR
#include <mach/mt_clkmgr.h>
#else
#include "ddp_clkmgr.h"
#endif
#include "m4u.h"
#include <linux/delay.h>
#include "ddp_info.h"
#include "ddp_hal.h"
#include "ddp_reg.h"
#include "ddp_ovl.h"
#include "primary_display.h"

#define OVL_REG_BACK_MAX          (40)
#define OVL_LAYER_OFFSET        (0x20)
#define OVL_RDMA_DEBUG_OFFSET   (0x4)

enum OVL_COLOR_SPACE {
	OVL_COLOR_SPACE_RGB = 0,
	OVL_COLOR_SPACE_YUV,
};

struct OVL_REG {
	unsigned long address;
	unsigned int value;
};

static DISP_MODULE_ENUM ovl_index_module[OVL_NUM] = {
	DISP_MODULE_OVL0, DISP_MODULE_OVL1, DISP_MODULE_OVL0_2L, DISP_MODULE_OVL1_2L
};

static unsigned int reg_back_cnt[OVL_NUM];
static struct OVL_REG reg_back[OVL_NUM][OVL_REG_BACK_MAX];
static unsigned int gOVLBackground = 0xFF000000;

static inline int is_module_ovl(DISP_MODULE_ENUM module)
{
	if (module == DISP_MODULE_OVL0 ||
	    module == DISP_MODULE_OVL1 ||
	    module == DISP_MODULE_OVL0_2L || module == DISP_MODULE_OVL1_2L)
		return 1;
	else
		return 0;
}

unsigned long ovl_base_addr(DISP_MODULE_ENUM module)
{
	switch (module) {
	case DISP_MODULE_OVL0:
		return DDP_REG_BASE_DISP_OVL0;
	case DISP_MODULE_OVL1:
		return DDP_REG_BASE_DISP_OVL1;
	case DISP_MODULE_OVL0_2L:
		return DDP_REG_BASE_DISP_OVL0_2L;
	case DISP_MODULE_OVL1_2L:
		return DDP_REG_BASE_DISP_OVL1_2L;
	default:
		DDPERR("invalid ovl module=%d\n", module);
		BUG();
	}
	return 0;
}

static inline unsigned long ovl_layer_num(DISP_MODULE_ENUM module)
{
	switch (module) {
	case DISP_MODULE_OVL0:
		return 4;
	case DISP_MODULE_OVL1:
		return 4;
	case DISP_MODULE_OVL0_2L:
		return 2;
	case DISP_MODULE_OVL1_2L:
		return 2;
	default:
		DDPERR("invalid ovl module=%d\n", module);
		BUG();
	}
	return 0;
}

static inline unsigned long ovl_to_m4u_port(DISP_MODULE_ENUM module)
{
	switch (module) {
	case DISP_MODULE_OVL0:
		return M4U_PORT_DISP_OVL0;
	case DISP_MODULE_OVL1:
		return M4U_PORT_DISP_OVL1;
	case DISP_MODULE_OVL0_2L:
		return M4U_PORT_DISP_2L_OVL0;
	case DISP_MODULE_OVL1_2L:
		return M4U_PORT_DISP_2L_OVL1;
	default:
		DDPERR("invalid ovl module=%d\n", module);
		BUG();
	}
	return 0;
}

CMDQ_EVENT_ENUM ovl_to_cmdq_event_nonsec_end(DISP_MODULE_ENUM module)
{
	switch (module) {
	case DISP_MODULE_OVL0:
		return CMDQ_SYNC_DISP_OVL0_2NONSEC_END;
	case DISP_MODULE_OVL1:
		return CMDQ_SYNC_DISP_OVL1_2NONSEC_END;
	case DISP_MODULE_OVL0_2L:
		return CMDQ_SYNC_DISP_2LOVL0_2NONSEC_END;
	case DISP_MODULE_OVL1_2L:
		return CMDQ_SYNC_DISP_2LOVL1_2NONSEC_END;
	default:
		DDPERR("invalid ovl module=%d\n", module);
		BUG();
	}
	return 0;

}

static inline unsigned long ovl_to_cmdq_engine(DISP_MODULE_ENUM module)
{
	switch (module) {
	case DISP_MODULE_OVL0:
		return CMDQ_ENG_DISP_OVL0;
	case DISP_MODULE_OVL1:
		return CMDQ_ENG_DISP_OVL1;
	case DISP_MODULE_OVL0_2L:
		return CMDQ_ENG_DISP_2L_OVL0;
	case DISP_MODULE_OVL1_2L:
		return CMDQ_ENG_DISP_2L_OVL1;
	default:
		DDPERR("invalid ovl module=%d\n", module);
		BUG();
	}
	return 0;
}

unsigned long ovl_to_index(DISP_MODULE_ENUM module)
{
	int i;
	for (i = 0; i < OVL_NUM; i++) {
		if (ovl_index_module[i] == module)
			return i;
	}
	DDPERR("invalid ovl module=%d\n", module);
	BUG();
	return 0;
}

static inline DISP_MODULE_ENUM ovl_index_to_module(int index)
{
	if (index >= OVL_NUM) {
		DDPERR("invalid ovl index=%d\n", index);
		BUG();
	}
	return ovl_index_module[index];
}

int ovl_start(DISP_MODULE_ENUM module, void *handle)
{
	unsigned long ovl_base = ovl_base_addr(module);
	DISP_REG_SET(handle, ovl_base + DISP_REG_OVL_EN, 0x01);
	DISP_REG_SET(handle, ovl_base + DISP_REG_OVL_INTEN,
		     0x1E | REG_FLD_VAL(INTEN_FLD_ABNORMAL_SOF, 1));
	DISP_REG_SET_FIELD(handle, DATAPATH_CON_FLD_LAYER_SMI_ID_EN,
			   ovl_base + DISP_REG_OVL_DATAPATH_CON, 0x1);
	return 0;
}

int ovl_stop(DISP_MODULE_ENUM module, void *handle)
{
	unsigned long ovl_base = ovl_base_addr(module);
	DISP_REG_SET(handle, ovl_base + DISP_REG_OVL_INTEN, 0x00);
	DISP_REG_SET(handle, ovl_base + DISP_REG_OVL_EN, 0x00);
	DISP_REG_SET(handle, ovl_base + DISP_REG_OVL_INTSTA, 0x00);

	return 0;
}

int ovl_is_idle(DISP_MODULE_ENUM module)
{
	unsigned long ovl_base = ovl_base_addr(module);
	if (((DISP_REG_GET(ovl_base + DISP_REG_OVL_FLOW_CTRL_DBG) & 0x3ff) != 0x1) &&
	    ((DISP_REG_GET(ovl_base + DISP_REG_OVL_FLOW_CTRL_DBG) & 0x3ff) != 0x2))
		return 0;
	else
		return 1;

}

int ovl_reset(DISP_MODULE_ENUM module, void *handle)
{
#define OVL_IDLE (0x3)
	int ret = 0;
	unsigned int delay_cnt = 0;
	unsigned long ovl_base = ovl_base_addr(module);
	DISP_CPU_REG_SET(ovl_base + DISP_REG_OVL_RST, 0x1);
	DISP_CPU_REG_SET(ovl_base + DISP_REG_OVL_RST, 0x0);
	/*only wait if not cmdq */
	if (handle == NULL) {
		while (!(DISP_REG_GET(ovl_base + DISP_REG_OVL_FLOW_CTRL_DBG) & OVL_IDLE)) {
			delay_cnt++;
			udelay(10);
			if (delay_cnt > 2000) {
				DDPERR("%s reset timeout!\n", ddp_get_module_name(module));
				ret = -1;
				break;
			}
		}
	}
	return ret;
}

int ovl_roi(DISP_MODULE_ENUM module,
	    unsigned int bg_w, unsigned int bg_h, unsigned int bg_color, void *handle)
{
	unsigned long ovl_base = ovl_base_addr(module);

	if ((bg_w > OVL_MAX_WIDTH) || (bg_h > OVL_MAX_HEIGHT)) {
		DDPERR("ovl_roi,exceed OVL max size, w=%d, h=%d\n", bg_w, bg_h);
		ASSERT(0);
	}

	DISP_REG_SET(handle, ovl_base + DISP_REG_OVL_ROI_SIZE, bg_h << 16 | bg_w);

	DISP_REG_SET(handle, ovl_base + DISP_REG_OVL_ROI_BGCLR, bg_color);

	return 0;
}

int ovl_layer_switch(DISP_MODULE_ENUM module, unsigned layer, unsigned int en, void *handle)
{
	unsigned long ovl_base = ovl_base_addr(module);
	ASSERT(layer <= 3);
	switch (layer) {
	case 0:
		DISP_REG_SET_FIELD(handle, SRC_CON_FLD_L0_EN, ovl_base + DISP_REG_OVL_SRC_CON, en);
		DISP_REG_SET(handle, ovl_base + DISP_REG_OVL_RDMA0_CTRL, en);
		break;
	case 1:
		DISP_REG_SET_FIELD(handle, SRC_CON_FLD_L1_EN, ovl_base + DISP_REG_OVL_SRC_CON, en);
		DISP_REG_SET(handle, ovl_base + DISP_REG_OVL_RDMA1_CTRL, en);
		break;
	case 2:
		DISP_REG_SET_FIELD(handle, SRC_CON_FLD_L2_EN, ovl_base + DISP_REG_OVL_SRC_CON, en);
		DISP_REG_SET(handle, ovl_base + DISP_REG_OVL_RDMA2_CTRL, en);
		break;
	case 3:
		DISP_REG_SET_FIELD(handle, SRC_CON_FLD_L3_EN, ovl_base + DISP_REG_OVL_SRC_CON, en);
		DISP_REG_SET(handle, ovl_base + DISP_REG_OVL_RDMA3_CTRL, en);
		break;
	default:
		DDPERR("invalid layer=%d\n", layer);
		ASSERT(0);
	}

	return 0;
}

static int ovl_layer_config(DISP_MODULE_ENUM module,
		unsigned int layer,
		unsigned int is_engine_sec,
		const OVL_CONFIG_STRUCT * const cfg,
		void *handle)
{
	unsigned int value = 0;
	unsigned int Bpp, input_swap, input_fmt;
	unsigned int rgb_swap = 0;
	int is_rgb;
	int color_matrix = 0x4;
	int rotate = 0;
	unsigned long ovl_base = ovl_base_addr(module);
	unsigned long layer_offset = ovl_base + layer * OVL_LAYER_OFFSET;
	unsigned int offset = 0;
	enum UNIFIED_COLOR_FMT format = cfg->fmt;
	unsigned int src_x = cfg->src_x;
	unsigned int dst_w = cfg->dst_w;

	if (cfg->dst_w > OVL_MAX_WIDTH)
		BUG();
	if (cfg->dst_h > OVL_MAX_HEIGHT)
		BUG();
	if (layer > 3)
		BUG();

	if (!cfg->addr && cfg->source == OVL_LAYER_SOURCE_MEM) {
		DDPERR("source from memory, but addr is 0!\n");
		BUG();
	}

#ifdef CONFIG_MTK_LCM_PHYSICAL_ROTATION_HW
	if (module != DISP_MODULE_OVL1)
		rotate = 1;
#endif

	/* check dim layer fmt */
	if (cfg->source == OVL_LAYER_SOURCE_RESERVED) {
		if (cfg->aen == 0)
			DDPERR("dim layer%d ahpha enable should be 1!\n", layer);
		format = UFMT_RGB888;
	}
	Bpp = ufmt_get_Bpp(format);
	input_swap = ufmt_get_byteswap(format);
	input_fmt = ufmt_get_format(format);
	is_rgb = ufmt_get_rgb(format);

	if (format == UFMT_UYVY || format == UFMT_VYUY ||
	    format == UFMT_YUYV || format == UFMT_YVYU) {
		unsigned int regval = 0;

		if (src_x % 2) {
			src_x -= 1;	/* make src_x to even */
			dst_w += 1;
			regval |= REG_FLD_VAL(OVL_L_CLIP_FLD_LEFT, 1);
		}

		if ((src_x + dst_w) % 2) {
			dst_w += 1;	/* make right boundary even */
			regval |= REG_FLD_VAL(OVL_L_CLIP_FLD_RIGHT, 1);
		}
		DISP_REG_SET(handle, DISP_REG_OVL_L0_CLIP + layer_offset, regval);
	}

	switch (cfg->yuv_range) {
	case 0:
		color_matrix = 4;
		break;		/* BT601_full */
	case 1:
		color_matrix = 6;
		break;		/* BT601 */
	case 2:
		color_matrix = 7;
		break;		/* BT709 */
	default:
		DDPERR("un-recognized yuv_range=%d!\n", cfg->yuv_range);
		color_matrix = 4;
	}


	DISP_REG_SET(handle, DISP_REG_OVL_RDMA0_CTRL + layer_offset, 0x1);

	value = (REG_FLD_VAL((L_CON_FLD_LARC), (cfg->source)) |
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
		value = value | REG_FLD_VAL((L_CON_FLD_MTX), (color_matrix));

	if (rotate)
		value |= REG_FLD_VAL((L_CON_FLD_VIRTICAL_FLIP), (1)) | REG_FLD_VAL((L_CON_FLD_HORI_FLIP), (1));

	DISP_REG_SET(handle, DISP_REG_OVL_L0_CON + layer_offset, value);

	DISP_REG_SET(handle, DISP_REG_OVL_L0_CLR + ovl_base + layer * 4, 0xff000000);

	DISP_REG_SET(handle, DISP_REG_OVL_L0_SRC_SIZE + layer_offset, cfg->dst_h << 16 | dst_w);

	if (rotate) {
		unsigned int bg_h, bg_w;

		bg_h = DISP_REG_GET(ovl_base + DISP_REG_OVL_ROI_SIZE);
		bg_w = bg_h & 0xFFFF;
		bg_h = bg_h >> 16;
		DISP_REG_SET(handle, DISP_REG_OVL_L0_OFFSET + layer_offset,
			     ((bg_h - cfg->dst_h - cfg->dst_y) << 16) | (bg_w - dst_w - cfg->dst_x));
		DISP_REG_SET(handle, DISP_REG_OVL_L0_ADDR + layer_offset,
			     cfg->addr + cfg->src_pitch * (cfg->dst_h + cfg->src_y - 1) + (src_x + dst_w) * Bpp - 1);
		offset = (src_x + dst_w) * Bpp + (cfg->src_y + cfg->dst_h - 1) * cfg->src_pitch - 1;
	} else {
		DISP_REG_SET(handle, DISP_REG_OVL_L0_OFFSET + layer_offset, (cfg->dst_y << 16) | cfg->dst_x);
		DISP_REG_SET(handle, DISP_REG_OVL_L0_ADDR + layer_offset,
			cfg->addr + src_x * Bpp + cfg->src_y * cfg->src_pitch);
		offset = src_x * Bpp + cfg->src_y * cfg->src_pitch;
	}

	if (!is_engine_sec) {
		DISP_REG_SET(handle, DISP_REG_OVL_L0_ADDR + layer_offset, cfg->addr + offset);
	} else {
		unsigned int size;
		int m4u_port;
		size = (cfg->dst_h - 1) * cfg->src_pitch + dst_w * Bpp;
		m4u_port = ovl_to_m4u_port(module);
		if (cfg->security != DISP_SECURE_BUFFER) {
			/* ovl is sec but this layer is non-sec */
			/* we need to tell cmdq to help map non-sec mva to sec mva */
			cmdqRecWriteSecure(handle,
					   disp_addr_convert(DISP_REG_OVL_L0_ADDR + layer_offset),
					   CMDQ_SAM_NMVA_2_MVA, cfg->addr + offset, 0, size, m4u_port);

		} else {
			/* for sec layer, addr variable stores sec handle */
			/* we need to pass this handle and offset to cmdq driver */
			/* cmdq sec driver will help to convert handle to correct address */
			cmdqRecWriteSecure(handle,
					   disp_addr_convert(DISP_REG_OVL_L0_ADDR + layer_offset),
					   CMDQ_SAM_H_2_MVA, cfg->addr, offset, size, m4u_port);
		}
	}
	DISP_REG_SET(handle, DISP_REG_OVL_L0_SRCKEY + layer_offset, cfg->key);

	value = (((cfg->sur_aen & 0x1) << 15) |
		 ((cfg->dst_alpha & 0x3) << 6) | ((cfg->dst_alpha & 0x3) << 4) |
		 ((cfg->src_alpha & 0x3) << 2) | (cfg->src_alpha & 0x3));

	value = (REG_FLD_VAL((L_PITCH_FLD_SUR_ALFA), (value)) |
		 REG_FLD_VAL((L_PITCH_FLD_LSP), (cfg->src_pitch)));

	if (cfg->const_bld)
		value = value | REG_FLD_VAL((L_PITCH_FLD_CONST_BLD), (1));
	DISP_REG_SET(handle, DISP_REG_OVL_L0_PITCH + layer_offset, value);

	return 0;
}

static void ovl_store_regs(DISP_MODULE_ENUM module)
{
	int i = 0;
	unsigned long ovl_base = ovl_base_addr(module);
	int idx = ovl_to_index(module);

	static unsigned long regs[3];
	regs[0] = DISP_REG_OVL_ROI_SIZE + ovl_base;
	regs[1] = DISP_REG_OVL_ROI_BGCLR + ovl_base;
	regs[2] = DISP_REG_OVL_DATAPATH_CON + ovl_base;


	reg_back_cnt[idx] = sizeof(regs) / sizeof(unsigned long);
	ASSERT(reg_back_cnt[idx] <= OVL_REG_BACK_MAX);


	for (i = 0; i < reg_back_cnt[idx]; i++) {
		reg_back[idx][i].address = regs[i];
		reg_back[idx][i].value = DISP_REG_GET(regs[i]);
	}
	DDPMSG("store %d cnt registers on ovl %d\n", reg_back_cnt[idx], idx);

}

static void ovl_restore_regs(DISP_MODULE_ENUM module, void *handle)
{
	int idx = ovl_to_index(module);
	int i = reg_back_cnt[idx];
	while (i > 0) {
		i--;
		DISP_REG_SET(handle, reg_back[idx][i].address, reg_back[idx][i].value);
	}
	DDPMSG("restore %d cnt registers on ovl %d\n", reg_back_cnt[idx], idx);
	reg_back_cnt[idx] = 0;
}

int ovl_clock_on(DISP_MODULE_ENUM module, void *handle)
{
	unsigned long ovl_base = ovl_base_addr(module);
	DDPDBG("%s clock_on\n", ddp_get_module_name(module));
	/* do not set CG */
/*
#ifdef ENABLE_CLK_MGR

	switch (module) {
	case DISP_MODULE_OVL0:
#ifdef CONFIG_MTK_CLKMGR
		enable_clock(MT_CG_DISP0_DISP_OVL0, ddp_get_module_name(module));
#else
		ddp_clk_enable(DISP0_DISP_OVL0);
#endif
		break;
	case DISP_MODULE_OVL1:
#ifdef CONFIG_MTK_CLKMGR
		enable_clock(MT_CG_DISP0_DISP_OVL1, ddp_get_module_name(module));
#else
		ddp_clk_enable(DISP0_DISP_OVL1);
#endif
		break;
	case DISP_MODULE_OVL0_2L:
#ifdef CONFIG_MTK_CLKMGR
		enable_clock(MT_CG_DISP0_DISP_OVL0_2L, ddp_get_module_name(module));
#else
		ddp_clk_enable(DISP0_DISP_2L_OVL0);
#endif
		break;
	case DISP_MODULE_OVL1_2L:
#ifdef CONFIG_MTK_CLKMGR
		enable_clock(MT_CG_DISP0_DISP_OVL1_2L, ddp_get_module_name(module));
#else
		ddp_clk_enable(DISP0_DISP_2L_OVL1);
#endif
		break;
	default:
		DDPERR("invalid ovl module=%d\n", module);
		BUG();
	}

#endif
*/
	/* DCM Setting -- Enable DCM */
	/* DISP_REG_SET(NULL, ovl_base + DISP_REG_OVL_FUNC_DCM0, 0x10); */
	DISP_REG_SET(NULL, ovl_base + DISP_REG_OVL_FUNC_DCM1, 0x10);

	return 0;
}

int ovl_clock_off(DISP_MODULE_ENUM module, void *handle)
{
	DDPDBG("%s clock_off\n", ddp_get_module_name(module));
	/* do not set CG */
/*
#ifdef ENABLE_CLK_MGR
	switch (module) {
	case DISP_MODULE_OVL0:
#ifdef CONFIG_MTK_CLKMGR
		disable_clock(MT_CG_DISP0_DISP_OVL0, ddp_get_module_name(module));
#else
		ddp_clk_disable(DISP0_DISP_OVL0);
#endif
		break;
	case DISP_MODULE_OVL1:
#ifdef CONFIG_MTK_CLKMGR
		disable_clock(MT_CG_DISP0_DISP_OVL1, ddp_get_module_name(module));
#else
		ddp_clk_disable(DISP0_DISP_OVL1);
#endif
		break;
	case DISP_MODULE_OVL0_2L:
#ifdef CONFIG_MTK_CLKMGR
		disable_clock(MT_CG_DISP0_DISP_OVL0_2L, ddp_get_module_name(module));
#else
		ddp_clk_disable(DISP0_DISP_2L_OVL0);
#endif
		break;
	case DISP_MODULE_OVL1_2L:
#ifdef CONFIG_MTK_CLKMGR
		disable_clock(MT_CG_DISP0_DISP_OVL1_2L, ddp_get_module_name(module));
#else
		ddp_clk_disable(DISP0_DISP_2L_OVL1);
#endif
		break;
	default:
		DDPERR("invalid ovl module=%d\n", module);
		BUG();
	}
#endif
*/
	return 0;
}

int ovl_resume(DISP_MODULE_ENUM module, void *handle)
{
	ovl_clock_on(module, handle);
	ovl_restore_regs(module, handle);
	return 0;
}

int ovl_suspend(DISP_MODULE_ENUM module, void *handle)
{
	DDPMSG("%s suspend\n", ddp_get_module_name(module));
	ovl_store_regs(module);
	ovl_clock_off(module, handle);
	return 0;
}

int ovl_init(DISP_MODULE_ENUM module, void *handle)
{
	return ovl_clock_on(module, handle);
}

int ovl_deinit(DISP_MODULE_ENUM module, void *handle)
{
	return ovl_clock_off(module, handle);
}

int ovl_connect(DISP_MODULE_ENUM module, DISP_MODULE_ENUM prev,
		DISP_MODULE_ENUM next, int connect, void *handle)
{
	unsigned long ovl_base = ovl_base_addr(module);

	if (connect && is_module_ovl(prev))
		DISP_REG_SET_FIELD(handle, DATAPATH_CON_FLD_BGCLR_IN_SEL,
				   ovl_base + DISP_REG_OVL_DATAPATH_CON, 1);
	else
		DISP_REG_SET_FIELD(handle, DATAPATH_CON_FLD_BGCLR_IN_SEL,
				   ovl_base + DISP_REG_OVL_DATAPATH_CON, 0);
	return 0;
}

unsigned int ddp_ovl_get_cur_addr(bool rdma_mode, int layerid)
{
	/*just workaround, should remove this func */
	unsigned long ovl_base = ovl_base_addr(DISP_MODULE_OVL0_2L);

	if (rdma_mode)
		return DISP_REG_GET(DISP_REG_RDMA_MEM_START_ADDR);
	if (DISP_REG_GET(DISP_REG_OVL_RDMA0_CTRL + layerid * 0x20 + ovl_base) & 0x1)
		return DISP_REG_GET(DISP_REG_OVL_L0_ADDR + layerid * 0x20 + ovl_base);
	return 0;
}

void ovl_get_address(DISP_MODULE_ENUM module, unsigned long *add)
{
	int i = 0;
	unsigned long ovl_base = ovl_base_addr(module);
	unsigned long layer_off = 0;
	unsigned int src_on = DISP_REG_GET(DISP_REG_OVL_SRC_CON + ovl_base);
	for (i = 0; i < 4; i++) {
		layer_off = i * OVL_LAYER_OFFSET + ovl_base;
		if (src_on & (0x1 << i))
			add[i] = DISP_REG_GET(layer_off + DISP_REG_OVL_L0_ADDR);
		else
			add[i] = 0;
	}
}

void ovl_get_info(DISP_MODULE_ENUM module, void *data)
{
	int i = 0;
	OVL_BASIC_STRUCT *pdata = data;
	unsigned long ovl_base = ovl_base_addr(module);
	unsigned long layer_off = 0;
	unsigned int src_on = DISP_REG_GET(DISP_REG_OVL_SRC_CON + ovl_base);
	OVL_BASIC_STRUCT *p = NULL;

	for (i = 0; i < ovl_layer_num(module); i++) {
		layer_off = i * OVL_LAYER_OFFSET + ovl_base;
		p = &pdata[i];
		p->layer = i;
		p->layer_en = src_on & (0x1 << i);
		if (p->layer_en) {
			p->fmt = display_fmt_reg_to_unified_fmt(DISP_REG_GET_FIELD(L_CON_FLD_CFMT,
										   layer_off +
										   DISP_REG_OVL_L0_CON),
								DISP_REG_GET_FIELD(L_CON_FLD_BTSW,
										   layer_off +
										   DISP_REG_OVL_L0_CON),
								DISP_REG_GET_FIELD(L_CON_FLD_RGB_SWAP,
										   layer_off +
										   DISP_REG_OVL_L0_CON));
			p->addr = DISP_REG_GET(layer_off + DISP_REG_OVL_L0_ADDR);
			p->src_w = DISP_REG_GET(layer_off + DISP_REG_OVL_L0_SRC_SIZE) & 0xfff;
			p->src_h =
			    (DISP_REG_GET(layer_off + DISP_REG_OVL_L0_SRC_SIZE) >> 16) & 0xfff;
			p->src_pitch = DISP_REG_GET(layer_off + DISP_REG_OVL_L0_PITCH) & 0xffff;
			p->bpp = UFMT_GET_bpp(p->fmt) / 8;
			p->gpu_mode = (DISP_REG_GET(layer_off + DISP_REG_OVL_DATAPATH_CON) & (0x1<<(8+i)))?1:0;
			p->adobe_mode = (DISP_REG_GET(layer_off + DISP_REG_OVL_DATAPATH_CON) & (0x1<<12))?1:0;
			p->ovl_gamma_out = (DISP_REG_GET(layer_off + DISP_REG_OVL_DATAPATH_CON) & (0x1<<15))?1:0;
			p->alpha = (DISP_REG_GET(layer_off + DISP_REG_OVL_L0_CON + (i*0x20)) & (0x1<<8))?1:0;
		}
		DDPDBG("ovl_get_info:layer%d,en %d,w %d,h %d,bpp %d,addr %lu\n",
		       i, p->layer_en, p->src_w, p->src_h, p->bpp, p->addr);
	}
}

static int ovl_check_input_param(OVL_CONFIG_STRUCT *config)
{
	if ((config->addr == 0 && config->source == 0) || config->dst_w == 0 || config->dst_h == 0) {
		DDPERR("ovl parameter invalidate, addr=%lu, w=%d, h=%d\n",
		       config->addr, config->dst_w, config->dst_h);
		ASSERT(0);
		return -1;
	}

	return 0;
}

/* use noinline to reduce stack size */
static noinline void print_layer_config_args(int module, int local_layer, OVL_CONFIG_STRUCT *ovl_cfg)
{
	DDPDBG("%s, layer=%d(%d), source=%s, off(x=%d, y=%d), dst(%d, %d, %d, %d),pitch=%d,",
		ddp_get_module_name(module), local_layer, ovl_cfg->layer,
		(ovl_cfg->source == 0) ? "memory" : "dim", ovl_cfg->src_x, ovl_cfg->src_y,
		ovl_cfg->dst_x, ovl_cfg->dst_y, ovl_cfg->dst_w, ovl_cfg->dst_h, ovl_cfg->src_pitch);
	DDPDBG("fmt=%s, addr=%lx, keyEn=%d, key=%d, aen=%d, alpha=%d,",
		unified_color_fmt_name(ovl_cfg->fmt), ovl_cfg->addr,
		ovl_cfg->keyEn, ovl_cfg->key, ovl_cfg->aen, ovl_cfg->alpha);
	DDPDBG("sur_aen=%d,sur_alpha=0x%x,yuv_range=%d,sec=%d,const_bld=%d\n",
		ovl_cfg->sur_aen, (ovl_cfg->dst_alpha << 2) | ovl_cfg->src_alpha,
		ovl_cfg->yuv_range, ovl_cfg->security, ovl_cfg->const_bld);
}

static int ovl_is_sec[OVL_NUM];
static int setup_ovl_sec(DISP_MODULE_ENUM module, void *handle, int is_engine_sec)
{
	int i = 0;
	int ovl_idx = ovl_to_index(module);
	CMDQ_ENG_ENUM cmdq_engine;
	/*CMDQ_EVENT_ENUM cmdq_event_nonsec_end;*/
	cmdq_engine = ovl_to_cmdq_engine(module);
	/*cmdq_event_nonsec_end = ovl_to_cmdq_event_nonsec_end(module);*/

	if (is_engine_sec) {

		cmdqRecSetSecure(handle, 1);
		/* set engine as sec */
		cmdqRecSecureEnablePortSecurity(handle, (1LL << cmdq_engine));
		/* cmdqRecSecureEnableDAPC(handle, (1LL << cmdq_engine)); */
		if (ovl_is_sec[ovl_idx] == 0)
			DDPMSG("[SVP] switch ovl%d to sec\n", ovl_idx);
		ovl_is_sec[ovl_idx] = 1;
	} else {
		if (ovl_is_sec[ovl_idx] == 1) {
			/* ovl is in sec stat, we need to switch it to nonsec */
			cmdqRecHandle nonsec_switch_handle;
			int ret;
			ret =
			    cmdqRecCreate(CMDQ_SCENARIO_DISP_PRIMARY_DISABLE_SECURE_PATH,
					  &(nonsec_switch_handle));
			if (ret)
				DDPAEE("[SVP]fail to create disable handle %s ret=%d\n",
				       __func__, ret);

			cmdqRecReset(nonsec_switch_handle);
			/*_cmdq_insert_wait_frame_done_token_mira(nonsec_switch_handle);*/

			if (module != DISP_MODULE_OVL1) {
				/*Primary Mode*/
				if (primary_display_is_decouple_mode())
					cmdqRecWaitNoClear(nonsec_switch_handle, CMDQ_EVENT_DISP_WDMA0_EOF);
				else
					_cmdq_insert_wait_frame_done_token_mira(nonsec_switch_handle);
			} else {
				/*External Mode*/
				/*_cmdq_insert_wait_frame_done_token_mira(nonsec_switch_handle);*/
				cmdqRecWaitNoClear(nonsec_switch_handle, CMDQ_SYNC_DISP_EXT_STREAM_EOF);
			}
			cmdqRecSetSecure(nonsec_switch_handle, 1);

			/* we should disable ovl before new (nonsec) setting take effect
			 * or translation fault may happen,
			 * if we switch ovl to nonsec BUT its setting is still sec */
			for (i = 0; i < ovl_layer_num(module); i++)
				ovl_layer_switch(module, i, 0, nonsec_switch_handle);
			/*in fact, dapc/port_sec will be disabled by cmdq */
			cmdqRecSecureEnablePortSecurity(nonsec_switch_handle, (1LL << cmdq_engine));
			/* cmdqRecSecureEnableDAPC(handle, (1LL << cmdq_engine)); */
			/*cmdqRecSetEventToken(nonsec_switch_handle, cmdq_event_nonsec_end);*/
			/*cmdqRecFlushAsync(nonsec_switch_handle);*/
			cmdqRecFlush(nonsec_switch_handle);
			cmdqRecDestroy(nonsec_switch_handle);
			/*cmdqRecWait(handle, cmdq_event_nonsec_end);*/
			DDPMSG("[SVP] switch ovl%d to nonsec\n", ovl_idx);
		}
		ovl_is_sec[ovl_idx] = 0;
	}

	return 0;
}

static int ovl_config_l(DISP_MODULE_ENUM module, disp_ddp_path_config *pConfig, void *handle)
{
	int enabled_layers = 0;
	int has_sec_layer = 0;
	int local_layer, global_layer, layer_id;

	if (pConfig->dst_dirty)
		ovl_roi(module, pConfig->dst_w, pConfig->dst_h, gOVLBackground, handle);

	if (!pConfig->ovl_dirty)
		return 0;

	for (global_layer = 0; global_layer < TOTAL_OVL_LAYER_NUM; global_layer++) {
		if (!(pConfig->ovl_layer_scanned & (1 << global_layer)))
			break;
	}
	if (global_layer > TOTAL_OVL_LAYER_NUM - ovl_layer_num(module)) {
		DDPERR("%s: %s scan error, layer_scanned=%u\n", __func__,
		       ddp_get_module_name(module), pConfig->ovl_layer_scanned);
		BUG();
	}

	/* check if the ovl module has sec layer */
	for (layer_id = global_layer; layer_id < (global_layer + ovl_layer_num(module)); layer_id++) {
		if (pConfig->ovl_config[layer_id].layer_en &&
		    (pConfig->ovl_config[layer_id].security == DISP_SECURE_BUFFER))
			has_sec_layer = 1;
	}

	setup_ovl_sec(module, handle, has_sec_layer);

	for (local_layer = 0; local_layer < ovl_layer_num(module); local_layer++, global_layer++) {

		OVL_CONFIG_STRUCT *ovl_cfg = &pConfig->ovl_config[global_layer];
		pConfig->ovl_layer_scanned |= (1 << global_layer);

		if (ovl_cfg->layer_en == 0)
			continue;
		if (ovl_check_input_param(ovl_cfg))
			continue;
		print_layer_config_args(module, local_layer, ovl_cfg);
		ovl_layer_config(module, local_layer, has_sec_layer, ovl_cfg, handle);

		enabled_layers |= 1 << local_layer;

	}

	DISP_REG_SET(handle, ovl_base_addr(module) + DISP_REG_OVL_SRC_CON, enabled_layers);

	return 0;
}

int ovl_build_cmdq(DISP_MODULE_ENUM module, void *cmdq_trigger_handle, CMDQ_STATE state)
{
	int ret = 0;
	/*int reg_pa = DISP_REG_OVL_FLOW_CTRL_DBG & 0x1fffffff;*/
	if (cmdq_trigger_handle == NULL) {
		DDPERR("cmdq_trigger_handle is NULL\n");
		return -1;
	}

	if (state == CMDQ_CHECK_IDLE_AFTER_STREAM_EOF) {
		if (module == DISP_MODULE_OVL0) {
			ret = cmdqRecPoll(cmdq_trigger_handle, 0x14007240, 2, 0x3f);
		} else {
			DDPERR("wrong module: %s\n", ddp_get_module_name(module));
			return -1;
		}
	}

	return 0;
}


/***************** ovl debug info ************/

void ovl_dump_reg(DISP_MODULE_ENUM module)
{
	unsigned long offset = ovl_base_addr(module);
	unsigned int src_on = DISP_REG_GET(DISP_REG_OVL_SRC_CON + offset);
	DDPDUMP("== START: DISP %s REGS ==\n", ddp_get_module_name(module));
	DDPDUMP("OVL:0x000=0x%08x,0x004=0x%08x,0x008=0x%08x,0x00c=0x%08x\n",
		DISP_REG_GET(DISP_REG_OVL_STA + offset),
		DISP_REG_GET(DISP_REG_OVL_INTEN + offset),
		DISP_REG_GET(DISP_REG_OVL_INTSTA + offset), DISP_REG_GET(DISP_REG_OVL_EN + offset));
	DDPDUMP("OVL:0x010=0x%08x,0x014=0x%08x,0x020=0x%08x,0x024=0x%08x\n",
		DISP_REG_GET(DISP_REG_OVL_TRIG + offset),
		DISP_REG_GET(DISP_REG_OVL_RST + offset),
		DISP_REG_GET(DISP_REG_OVL_ROI_SIZE + offset),
		DISP_REG_GET(DISP_REG_OVL_DATAPATH_CON + offset));
	DDPDUMP("OVL:0x028=0x%08x,0x02c=0x%08x\n",
		DISP_REG_GET(DISP_REG_OVL_ROI_BGCLR + offset),
		DISP_REG_GET(DISP_REG_OVL_SRC_CON + offset));

	if (src_on & 0x1) {
		DDPDUMP("OVL:0x030=0x%08x,0x034=0x%08x,0x038=0x%08x,0x03c=0x%08x\n",
			DISP_REG_GET(DISP_REG_OVL_L0_CON + offset),
			DISP_REG_GET(DISP_REG_OVL_L0_SRCKEY + offset),
			DISP_REG_GET(DISP_REG_OVL_L0_SRC_SIZE + offset),
			DISP_REG_GET(DISP_REG_OVL_L0_OFFSET + offset));

		DDPDUMP("OVL:0xf40=0x%08x,0x044=0x%08x,0x0c0=0x%08x,0x0c8=0x%08x\n",
			DISP_REG_GET(DISP_REG_OVL_L0_ADDR + offset),
			DISP_REG_GET(DISP_REG_OVL_L0_PITCH + offset),
			DISP_REG_GET(DISP_REG_OVL_RDMA0_CTRL + offset),
			DISP_REG_GET(DISP_REG_OVL_RDMA0_MEM_GMC_SETTING + offset));

		DDPDUMP("OVL:0x0d0=0x%08x,0x1e0=0x%08x,0x24c=0x%08x\n",
			DISP_REG_GET(DISP_REG_OVL_RDMA0_FIFO_CTRL + offset),
			DISP_REG_GET(DISP_REG_OVL_RDMA0_MEM_GMC_S2 + offset),
			DISP_REG_GET(DISP_REG_OVL_RDMA0_DBG + offset));
	}
	if (src_on & 0x2) {
		DDPDUMP("OVL:0x050=0x%08x,0x054=0x%08x,0x058=0x%08x,0x05c=0x%08x\n",
			DISP_REG_GET(DISP_REG_OVL_L1_CON + offset),
			DISP_REG_GET(DISP_REG_OVL_L1_SRCKEY + offset),
			DISP_REG_GET(DISP_REG_OVL_L1_SRC_SIZE + offset),
			DISP_REG_GET(DISP_REG_OVL_L1_OFFSET + offset));

		DDPDUMP("OVL:0xf60=0x%08x,0x064=0x%08x,0x0e0=0x%08x,0x0e8=0x%08x,0x0f0=0x%08x\n",
			DISP_REG_GET(DISP_REG_OVL_L1_ADDR + offset),
			DISP_REG_GET(DISP_REG_OVL_L1_PITCH + offset),
			DISP_REG_GET(DISP_REG_OVL_RDMA1_CTRL + offset),
			DISP_REG_GET(DISP_REG_OVL_RDMA1_MEM_GMC_SETTING + offset),
			DISP_REG_GET(DISP_REG_OVL_RDMA1_FIFO_CTRL + offset));

		DDPDUMP("OVL:0x1e4=0x%08x,0x250=0x%08x\n",
			DISP_REG_GET(DISP_REG_OVL_RDMA1_MEM_GMC_S2 + offset),
			DISP_REG_GET(DISP_REG_OVL_RDMA1_DBG + offset));
	}
	if (src_on & 0x4) {
		DDPDUMP("OVL:0x070=0x%08x,0x074=0x%08x,0x078=0x%08x,0x07c=0x%08x\n",
			DISP_REG_GET(DISP_REG_OVL_L2_CON + offset),
			DISP_REG_GET(DISP_REG_OVL_L2_SRCKEY + offset),
			DISP_REG_GET(DISP_REG_OVL_L2_SRC_SIZE + offset),
			DISP_REG_GET(DISP_REG_OVL_L2_OFFSET + offset));

		DDPDUMP("OVL:0xf80=0x%08x,0x084=0x%08x,0x100=0x%08x,0x110=0x%08x\n",
			DISP_REG_GET(DISP_REG_OVL_L2_ADDR + offset),
			DISP_REG_GET(DISP_REG_OVL_L2_PITCH + offset),
			DISP_REG_GET(DISP_REG_OVL_RDMA2_CTRL + offset),
			DISP_REG_GET(DISP_REG_OVL_RDMA2_FIFO_CTRL + offset));

		DDPDUMP("OVL:0x1e8=0x%08x,0x254=0x%08x\n",
			DISP_REG_GET(DISP_REG_OVL_RDMA2_MEM_GMC_S2 + offset),
			DISP_REG_GET(DISP_REG_OVL_RDMA2_DBG + offset));
	}
	if (src_on & 0x8) {
		DDPDUMP("OVL:0x090=0x%08x,0x094=0x%08x,0x098=0x%08x,0x09c=0x%08x\n",
			DISP_REG_GET(DISP_REG_OVL_L3_CON + offset),
			DISP_REG_GET(DISP_REG_OVL_L3_SRCKEY + offset),
			DISP_REG_GET(DISP_REG_OVL_L3_SRC_SIZE + offset),
			DISP_REG_GET(DISP_REG_OVL_L3_OFFSET + offset));

		DDPDUMP("OVL:0xfa0=0x%08x,0x0a4=0x%08x,0x120=0x%08x,0x130=0x%08x\n",
			DISP_REG_GET(DISP_REG_OVL_L3_ADDR + offset),
			DISP_REG_GET(DISP_REG_OVL_L3_PITCH + offset),
			DISP_REG_GET(DISP_REG_OVL_RDMA3_CTRL + offset),
			DISP_REG_GET(DISP_REG_OVL_RDMA3_FIFO_CTRL + offset));

		DDPDUMP("OVL:0x1ec=0x%08x,0x258=0x%08x\n",
			DISP_REG_GET(DISP_REG_OVL_RDMA3_MEM_GMC_S2 + offset),
			DISP_REG_GET(DISP_REG_OVL_RDMA3_DBG + offset));
	}
	DDPDUMP("OVL:0x1d4=0x%08x,0x1f8=0x%08x,0x1fc=0x%08x,0x200=0x%08x,0x20c=0x%08x\n"
		"0x210=0x%08x,0x214=0x%08x,0x218=0x%08x,0x21c=0x%08x\n"
		"0x230=0x%08x,0x234=0x%08x,0x240=0x%08x,0x244=0x%08x,0x2A0=0x%08x,0x2A4=0x%08x\n",
		DISP_REG_GET(DISP_REG_OVL_DEBUG_MON_SEL + offset),
		DISP_REG_GET(DISP_REG_OVL_RDMA_GREQ_NUM + offset),
		DISP_REG_GET(DISP_REG_OVL_RDMA_GREQ_URG_NUM + offset),
		DISP_REG_GET(DISP_REG_OVL_DUMMY_REG + offset),
		DISP_REG_GET(DISP_REG_OVL_RDMA_ULTRA_SRC + offset),

		DISP_REG_GET(DISP_REG_OVL_RDMAn_BUF_LOW(0) + offset),
		DISP_REG_GET(DISP_REG_OVL_RDMAn_BUF_LOW(1) + offset),
		DISP_REG_GET(DISP_REG_OVL_RDMAn_BUF_LOW(2) + offset),
		DISP_REG_GET(DISP_REG_OVL_RDMAn_BUF_LOW(3) + offset),

		DISP_REG_GET(DISP_REG_OVL_SMI_DBG + offset),
		DISP_REG_GET(DISP_REG_OVL_GREQ_LAYER_CNT + offset),
		DISP_REG_GET(DISP_REG_OVL_FLOW_CTRL_DBG + offset),
		DISP_REG_GET(DISP_REG_OVL_ADDCON_DBG + offset),
		DISP_REG_GET(DISP_REG_OVL_FUNC_DCM0 + offset),
		DISP_REG_GET(DISP_REG_OVL_FUNC_DCM1 + offset));
	DDPDUMP("-- END: DISP %s REGS --\n", ddp_get_module_name(module));
}

static void ovl_printf_status(unsigned int status)
{
	DDPDUMP("=OVL_FLOW_CONTROL_DEBUG=:\n");
	DDPDUMP("addcon_idle:%d,blend_idle:%d,out_valid:%d,out_ready:%d,out_idle:%d\n",
		(status >> 10) & (0x1),
		(status >> 11) & (0x1),
		(status >> 12) & (0x1), (status >> 13) & (0x1), (status >> 15) & (0x1)
	    );
	DDPDUMP("rdma3_idle:%d,rdma2_idle:%d,rdma1_idle:%d, rdma0_idle:%d,rst:%d\n",
		(status >> 16) & (0x1),
		(status >> 17) & (0x1),
		(status >> 18) & (0x1), (status >> 19) & (0x1), (status >> 20) & (0x1)
	    );
	DDPDUMP("trig:%d,frame_hwrst_done:%d,frame_swrst_done:%d,frame_underrun:%d,frame_done:%d\n",
		(status >> 21) & (0x1),
		(status >> 23) & (0x1),
		(status >> 24) & (0x1), (status >> 25) & (0x1), (status >> 26) & (0x1)
	    );
	DDPDUMP("ovl_running:%d,ovl_start:%d,ovl_clr:%d,reg_update:%d,ovl_upd_reg:%d\n",
		(status >> 27) & (0x1),
		(status >> 28) & (0x1),
		(status >> 29) & (0x1), (status >> 30) & (0x1), (status >> 31) & (0x1)
	    );

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
	DDPDUMP("wram_rst_cs:0x%x,layer_greq:0x%x,out_data:0x%x,",
		status & 0x7, (status >> 3) & 0x1, (status >> 4) & 0xffffff);
	DDPDUMP("out_ready:0x%x,out_valid:0x%x,smi_busy:0x%x,smi_greq:0x%x\n",
		(status >> 28) & 0x1, (status >> 29) & 0x1, (status >> 30) & 0x1,
		(status >> 31) & 0x1);
}

static void ovl_dump_layer_info(int layer, unsigned long layer_offset)
{
	enum UNIFIED_COLOR_FMT fmt;
	fmt =
	    display_fmt_reg_to_unified_fmt(DISP_REG_GET_FIELD
					   (L_CON_FLD_CFMT, DISP_REG_OVL_L0_CON + layer_offset),
					   DISP_REG_GET_FIELD(L_CON_FLD_BTSW,
							      DISP_REG_OVL_L0_CON + layer_offset),
						DISP_REG_GET_FIELD(L_CON_FLD_RGB_SWAP,
							      DISP_REG_OVL_L0_CON + layer_offset));

	DDPDUMP("layer%d: w=%d,h=%d,off(x=%d,y=%d),pitch=%d,addr=0x%x,fmt=%s,source=%s,aen=%d,alpha=%d\n",
	     layer, DISP_REG_GET(layer_offset + DISP_REG_OVL_L0_SRC_SIZE) & 0xfff,
	     (DISP_REG_GET(layer_offset + DISP_REG_OVL_L0_SRC_SIZE) >> 16) & 0xfff,
	     DISP_REG_GET(layer_offset + DISP_REG_OVL_L0_OFFSET) & 0xfff,
	     (DISP_REG_GET(layer_offset + DISP_REG_OVL_L0_OFFSET) >> 16) & 0xfff,
	     DISP_REG_GET(layer_offset + DISP_REG_OVL_L0_PITCH) & 0xffff,
	     DISP_REG_GET(layer_offset + DISP_REG_OVL_L0_ADDR), unified_color_fmt_name(fmt),
	     (DISP_REG_GET_FIELD(L_CON_FLD_LARC, DISP_REG_OVL_L0_CON + layer_offset) ==
	      0) ? "memory" : "constant_color", DISP_REG_GET_FIELD(L_CON_FLD_AEN,
								   DISP_REG_OVL_L0_CON +
								   layer_offset),
	     DISP_REG_GET_FIELD(L_CON_FLD_APHA, DISP_REG_OVL_L0_CON + layer_offset)
	    );
}

void ovl_dump_analysis(DISP_MODULE_ENUM module)
{
	int i = 0;
	unsigned long layer_offset = 0;
	unsigned long rdma_offset = 0;
	unsigned long offset = ovl_base_addr(module);
	unsigned int src_on = DISP_REG_GET(DISP_REG_OVL_SRC_CON + offset);
	unsigned int rdma_ctrl;

	DDPDUMP("==DISP %s ANALYSIS==\n", ddp_get_module_name(module));
	DDPDUMP("ovl_en=%d,layer_enable(%d,%d,%d,%d),bg(w=%d, h=%d),",
		DISP_REG_GET(DISP_REG_OVL_EN + offset),
		DISP_REG_GET(DISP_REG_OVL_SRC_CON + offset) & 0x1,
		(DISP_REG_GET(DISP_REG_OVL_SRC_CON + offset) >> 1) & 0x1,
		(DISP_REG_GET(DISP_REG_OVL_SRC_CON + offset) >> 2) & 0x1,
		(DISP_REG_GET(DISP_REG_OVL_SRC_CON + offset) >> 3) & 0x1,
		DISP_REG_GET(DISP_REG_OVL_ROI_SIZE + offset) & 0xfff,
		(DISP_REG_GET(DISP_REG_OVL_ROI_SIZE + offset) >> 16) & 0xfff);
	DDPDUMP("cur_pos(x=%d,y=%d),layer_hit(%d,%d,%d,%d),bg_mode=%s,sta=0x%x\n",
		DISP_REG_GET_FIELD(ADDCON_DBG_FLD_ROI_X, DISP_REG_OVL_ADDCON_DBG + offset),
		DISP_REG_GET_FIELD(ADDCON_DBG_FLD_ROI_Y, DISP_REG_OVL_ADDCON_DBG + offset),
		DISP_REG_GET_FIELD(ADDCON_DBG_FLD_L0_WIN_HIT, DISP_REG_OVL_ADDCON_DBG + offset),
		DISP_REG_GET_FIELD(ADDCON_DBG_FLD_L1_WIN_HIT, DISP_REG_OVL_ADDCON_DBG + offset),
		DISP_REG_GET_FIELD(ADDCON_DBG_FLD_L2_WIN_HIT, DISP_REG_OVL_ADDCON_DBG + offset),
		DISP_REG_GET_FIELD(ADDCON_DBG_FLD_L3_WIN_HIT, DISP_REG_OVL_ADDCON_DBG + offset),
		DISP_REG_GET_FIELD(DATAPATH_CON_FLD_BGCLR_IN_SEL,
				   DISP_REG_OVL_DATAPATH_CON + offset) ? "directlink" : "const",
		DISP_REG_GET(DISP_REG_OVL_STA + offset)
	    );
	for (i = 0; i < 4; i++) {
		layer_offset = i * OVL_LAYER_OFFSET + offset;
		rdma_offset = i * OVL_RDMA_DEBUG_OFFSET + offset;
		if (src_on & (0x1 << i))
			ovl_dump_layer_info(i, layer_offset);
		else
			DDPDUMP("layer%d: disabled\n", i);

		rdma_ctrl = DISP_REG_GET(layer_offset + DISP_REG_OVL_RDMA0_CTRL);

		DDPDUMP("ovl rdma%d status:(en=%d, fifo_used %d, GMC=0x%x)\n", i,
			REG_FLD_VAL_GET(RDMA0_CTRL_FLD_RDMA_EN, rdma_ctrl),
			REG_FLD_VAL_GET(RDMA0_CTRL_FLD_RMDA_FIFO_USED_SZ, rdma_ctrl),
			DISP_REG_GET(layer_offset + DISP_REG_OVL_RDMA0_MEM_GMC_SETTING));
		ovl_print_ovl_rdma_status(DISP_REG_GET(DISP_REG_OVL_RDMA0_DBG + rdma_offset));
	}
	ovl_printf_status(DISP_REG_GET(DISP_REG_OVL_FLOW_CTRL_DBG + offset));
}

int ovl_dump(DISP_MODULE_ENUM module, int level)
{
	ovl_dump_analysis(module);
	ovl_dump_reg(module);

	return 0;
}

static int ovl_golden_setting(DISP_MODULE_ENUM module, enum dst_module_type dst_mod_type, void *cmdq)
{
	unsigned long ovl_base = ovl_base_addr(module);
	unsigned int regval;
	int i, layer_num;

	layer_num = ovl_layer_num(module);

	/* DISP_REG_OVL_RDMA0_MEM_GMC_SETTING */
	regval = REG_FLD_VAL(FLD_OVL_RDMA_MEM_GMC_ULTRA_THRESHOLD, 0xff);
	if (dst_mod_type == DST_MOD_REAL_TIME)
		regval |= REG_FLD_VAL(FLD_OVL_RDMA_MEM_GMC_PRE_ULTRA_THRESHOLD, 0xff);
	else
		regval |= REG_FLD_VAL(FLD_OVL_RDMA_MEM_GMC_PRE_ULTRA_THRESHOLD, /*0x78*/ 0xff);

	for (i = 0; i < layer_num; i++) {
		unsigned long layer_offset = i * OVL_LAYER_OFFSET + ovl_base;
		DISP_REG_SET(cmdq, layer_offset + DISP_REG_OVL_RDMA0_MEM_GMC_SETTING, regval);
	}

	/* DISP_REG_OVL_RDMA0_FIFO_CTRL */
	regval = REG_FLD_VAL(FLD_OVL_RDMA_FIFO_SIZE, 144);
	for (i = 0; i < layer_num; i++) {
		unsigned long layer_offset = i * OVL_LAYER_OFFSET + ovl_base;
		DISP_REG_SET(cmdq, layer_offset + DISP_REG_OVL_RDMA0_FIFO_CTRL, regval);
	}

	/* DISP_REG_OVL_RDMA_GREQ_NUM */
	regval = REG_FLD_VAL(FLD_OVL_RDMA_GREQ_LAYER0_GREQ_NUM, 5);
	if (layer_num > 1)
		regval |= REG_FLD_VAL(FLD_OVL_RDMA_GREQ_LAYER1_GREQ_NUM, 5);
	if (layer_num > 2)
		regval |= REG_FLD_VAL(FLD_OVL_RDMA_GREQ_LAYER2_GREQ_NUM, 5);
	if (layer_num > 3)
		regval |= REG_FLD_VAL(FLD_OVL_RDMA_GREQ_LAYER3_GREQ_NUM, 5);

	regval |= REG_FLD_VAL(FLD_OVL_RDMA_GREQ_OSTD_GREQ_NUM, 0xff);
	regval |= REG_FLD_VAL(FLD_OVL_RDMA_GREQ_GREQ_DIS_CNT, 0);
	regval |= REG_FLD_VAL(FLD_OVL_RDMA_GREQ_IOBUF_FLUSH_PREULTRA, 1);
	regval |= REG_FLD_VAL(FLD_OVL_RDMA_GREQ_IOBUF_FLUSH_ULTRA, 0);
	DISP_REG_SET(cmdq, ovl_base + DISP_REG_OVL_RDMA_GREQ_NUM, regval);

	/* DISP_REG_OVL_RDMA_GREQ_URG_NUM */
	regval = REG_FLD_VAL(FLD_OVL_RDMA_GREQ_LAYER0_GREQ_URG_NUM, 5);
	if (layer_num > 0)
		regval |= REG_FLD_VAL(FLD_OVL_RDMA_GREQ_LAYER1_GREQ_URG_NUM, 5);
	if (layer_num > 1)
		regval |= REG_FLD_VAL(FLD_OVL_RDMA_GREQ_LAYER2_GREQ_URG_NUM, 5);
	if (layer_num > 2)
		regval |= REG_FLD_VAL(FLD_OVL_RDMA_GREQ_LAYER3_GREQ_URG_NUM, 5);

	regval |= REG_FLD_VAL(FLD_OVL_RDMA_GREQ_ARG_GREQ_URG_TH, 0);
	regval |= REG_FLD_VAL(FLD_OVL_RDMA_GREQ_ARG_URG_BIAS, 0);
	DISP_REG_SET(cmdq, ovl_base + DISP_REG_OVL_RDMA_GREQ_URG_NUM, regval);

	/* DISP_REG_OVL_RDMA_ULTRA_SRC */
	regval = REG_FLD_VAL(FLD_OVL_RDMA_PREULTRA_BUF_SRC, 0);
	regval |= REG_FLD_VAL(FLD_OVL_RDMA_PREULTRA_SMI_SRC, 0);
	regval |= REG_FLD_VAL(FLD_OVL_RDMA_PREULTRA_RDMA_SRC, 1);
	regval |= REG_FLD_VAL(FLD_OVL_RDMA_ULTRA_BUF_SRC, 0);
	regval |= REG_FLD_VAL(FLD_OVL_RDMA_ULTRA_SMI_SRC, 0);
	if (dst_mod_type == DST_MOD_REAL_TIME) {
		regval |= REG_FLD_VAL(FLD_OVL_RDMA_ULTRA_ROI_END_SRC, 0);
		regval |= REG_FLD_VAL(FLD_OVL_RDMA_PREULTRA_ROI_END_SRC, 0);

	} else {
		regval |= REG_FLD_VAL(FLD_OVL_RDMA_ULTRA_ROI_END_SRC, 2);
		regval |= REG_FLD_VAL(FLD_OVL_RDMA_PREULTRA_ROI_END_SRC, 1);

	}
	regval |= REG_FLD_VAL(FLD_OVL_RDMA_ULTRA_RDMA_SRC, 2);

	DISP_REG_SET(cmdq, ovl_base + DISP_REG_OVL_RDMA_ULTRA_SRC, regval);

	/* DISP_REG_OVL_RDMAn_BUF_LOW */
	regval = REG_FLD_VAL(FLD_OVL_RDMA_BUF_LOW_ULTRA_TH, 0);
	if (dst_mod_type == DST_MOD_REAL_TIME)
		regval |= REG_FLD_VAL(FLD_OVL_RDMA_BUF_LOW_PREULTRA_TH, 0);
	else
		regval |= REG_FLD_VAL(FLD_OVL_RDMA_BUF_LOW_PREULTRA_TH, /*0x30*/ 0x0);

	for (i = 0; i < layer_num; i++)
		DISP_REG_SET(cmdq, ovl_base + DISP_REG_OVL_RDMAn_BUF_LOW(i), regval);

	/* DISP_REG_OVL_FUNC_DCM1 -- no need anymore, because we set it @ovl_clock_on()*/
	/* DISP_REG_SET(cmdq, ovl_base + DISP_REG_OVL_FUNC_DCM1, 0x10); */

	return 0;
}

static int ovl_ioctl(DISP_MODULE_ENUM module, void *handle, DDP_IOCTL_NAME ioctl_cmd, void *params)
{
	int ret = 0;

	if (ioctl_cmd == DDP_OVL_GOLDEN_SETTING) {
		struct ddp_io_golden_setting_arg *gset_arg = params;
		ovl_golden_setting(module, gset_arg->dst_mod_type, handle);
	}

	return ret;
}

/***************** driver************/
DDP_MODULE_DRIVER ddp_driver_ovl = {
	.init = ovl_init,
	.deinit = ovl_deinit,
	.config = ovl_config_l,
	.start = ovl_start,
	.trigger = NULL,
	.stop = ovl_stop,
	.reset = ovl_reset,
	.power_on = ovl_clock_on,
	.power_off = ovl_clock_off,
	.suspend = ovl_suspend,
	.resume = ovl_resume,
	.is_idle = NULL,
	.is_busy = NULL,
	.dump_info = ovl_dump,
	.bypass = NULL,
	.build_cmdq = NULL,
	.set_lcm_utils = NULL,
	.ioctl = ovl_ioctl,
	.connect = ovl_connect,
};
