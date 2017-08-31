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

#define LOG_TAG "WDMA"
#include "ddp_log.h"
#ifdef CONFIG_MTK_CLKMGR
#include <mach/mt_clkmgr.h>
#else
#include "ddp_clkmgr.h"
#endif
#include <linux/delay.h>
#include "ddp_reg.h"
#include "ddp_matrix_para.h"
#include "ddp_info.h"
#include "ddp_wdma.h"
#include "ddp_wdma_ex.h"
#include "primary_display.h"
#include "m4u_port.h"
#include "ddp_mmp.h"
#include "disp_cmdq.h"

#define ALIGN_TO(x, n)  \
	(((x) + ((n) - 1)) & ~((n) - 1))


static char *wdma_get_status(unsigned int status)
{
	switch (status) {
	case 0x1:
		return "idle";
	case 0x2:
		return "clear";
	case 0x4:
		return "prepare";
	case 0x8:
		return "prepare";
	case 0x10:
		return "data_running";
	case 0x20:
		return "eof_wait";
	case 0x40:
		return "soft_reset_wait";
	case 0x80:
		return "eof_done";
	case 0x100:
		return "soft_reset_done";
	case 0x200:
		return "frame_complete";
	}
	return "unknown";

}

int wdma_start(enum DISP_MODULE_ENUM module, void *handle)
{
	unsigned long base_addr = wdma_base_addr(module);

	DISP_REG_SET(handle, base_addr + DISP_REG_WDMA_INTEN, 0x03);

	if (disp_helper_get_option(DISP_OPT_SHADOW_REGISTER)) {
		if (disp_helper_get_option(DISP_OPT_SHADOW_MODE) == 0) {
			/* full shadow mode */
		} else if (disp_helper_get_option(DISP_OPT_SHADOW_MODE) == 1) {
			/* force commit */
			DISP_REG_SET_FIELD(handle, WDMA_FORCE_COMMIT,
				base_addr + DISP_REG_WDMA_EN, 0x1);
		} else if (disp_helper_get_option(DISP_OPT_SHADOW_MODE) == 2) {
			/* bypass shadow */
			DISP_REG_SET_FIELD(handle, WDMA_BYPASS_SHADOW,
				base_addr + DISP_REG_WDMA_EN, 0x1);
		}
	}
	DISP_REG_SET_FIELD(handle, WDMA_ENABLE,
				   base_addr + DISP_REG_WDMA_EN, 0x1);

	return 0;
}

static int wdma_config_yuv420(enum DISP_MODULE_ENUM module,
			      enum UNIFIED_COLOR_FMT fmt,
			      unsigned int dstPitch,
			      unsigned int Height,
			      unsigned long dstAddress, enum DISP_BUFFER_TYPE sec, void *handle)
{
	unsigned long base_addr = wdma_base_addr(module);
	/* size_t size; */
	unsigned int u_off = 0;
	unsigned int v_off = 0;
	unsigned int u_stride = 0;
	unsigned int y_size = 0;
	unsigned int u_size = 0;
	/* unsigned int v_size = 0; */
	unsigned int stride = dstPitch;
	int has_v = 1;

	if (fmt != UFMT_YV12 && fmt != UFMT_I420 && fmt != UFMT_NV12 && fmt != UFMT_NV21)
		return 0;

	if (fmt == UFMT_YV12) {
		y_size = stride * Height;
		u_stride = ALIGN_TO(stride / 2, 16);
		u_size = u_stride * Height / 2;
		u_off = y_size;
		v_off = y_size + u_size;
	} else if (fmt == UFMT_I420) {
		y_size = stride * Height;
		u_stride = ALIGN_TO(stride / 2, 16);
		u_size = u_stride * Height / 2;
		v_off = y_size;
		u_off = y_size + u_size;
	} else if (fmt == UFMT_NV12 || fmt == UFMT_NV21) {
		y_size = stride * Height;
		u_stride = stride / 2;
		u_size = u_stride * Height / 2;
		u_off = y_size;
		has_v = 0;
	}

	if (sec != DISP_SECURE_BUFFER) {
		DISP_REG_SET(handle, base_addr + DISP_REG_WDMA_DST_ADDR1, dstAddress + u_off);
		if (has_v)
			DISP_REG_SET(handle, base_addr + DISP_REG_WDMA_DST_ADDR2,
				     dstAddress + v_off);
	} else {
		int m4u_port;

		m4u_port = module == DISP_MODULE_WDMA0 ? M4U_PORT_DISP_WDMA0 : M4U_PORT_DISP_WDMA1;

		disp_cmdq_write_reg_secure(handle, disp_addr_convert(base_addr + DISP_REG_WDMA_DST_ADDR1),
				   CMDQ_SAM_H_2_MVA, dstAddress, u_off, u_size, m4u_port);
		if (has_v)
			disp_cmdq_write_reg_secure(handle,
					   disp_addr_convert(base_addr + DISP_REG_WDMA_DST_ADDR2),
					   CMDQ_SAM_H_2_MVA, dstAddress, v_off, u_size, m4u_port);
	}
	DISP_REG_SET_FIELD(handle, DST_W_IN_BYTE_FLD_DST_W_IN_BYTE,
			   base_addr + DISP_REG_WDMA_DST_UV_PITCH, u_stride);
	return 0;
}

static int wdma_config(enum DISP_MODULE_ENUM module,
		       unsigned srcWidth,
		       unsigned srcHeight,
		       unsigned clipX,
		       unsigned clipY,
		       unsigned clipWidth,
		       unsigned clipHeight,
		       enum UNIFIED_COLOR_FMT out_format,
		       unsigned long dstAddress,
		       unsigned dstPitch,
		       unsigned int useSpecifiedAlpha,
		       unsigned char alpha, enum DISP_BUFFER_TYPE sec, void *handle)
{
	unsigned long base_addr = wdma_base_addr(module);
	unsigned int output_swap = ufmt_get_byteswap(out_format);
	unsigned int is_rgb = ufmt_get_rgb(out_format);
	unsigned int out_fmt_reg = ufmt_get_format(out_format);
	int color_matrix = 0x2;	/* 0010 RGB_TO_BT601 */
	size_t size = dstPitch * clipHeight;

	DDPDBG("%s,src(w%d,h%d),clip(x%d,y%d,w%d,h%d),fmt=%s,addr=0x%lx,pitch=%d,s_alfa=%d,alpa=%d,hnd=0x%p,sec%d\n",
	     ddp_get_module_name(module), srcWidth, srcHeight, clipX, clipY, clipWidth, clipHeight,
	     unified_color_fmt_name(out_format), dstAddress, dstPitch, useSpecifiedAlpha, alpha,
	     handle, sec);

	/* should use OVL alpha instead of sw config */
	DISP_REG_SET(handle, base_addr + DISP_REG_WDMA_SRC_SIZE, srcHeight << 16 | srcWidth);
	DISP_REG_SET(handle, base_addr + DISP_REG_WDMA_CLIP_COORD, clipY << 16 | clipX);
	DISP_REG_SET(handle, base_addr + DISP_REG_WDMA_CLIP_SIZE, clipHeight << 16 | clipWidth);
	DISP_REG_SET_FIELD(handle, CFG_FLD_OUT_FORMAT, base_addr + DISP_REG_WDMA_CFG, out_fmt_reg);

	if (!is_rgb) {
		/* set DNSP for UYVY and YUV_3P format for better quality */
		wdma_config_yuv420(module, out_format, dstPitch, clipHeight, dstAddress, sec,
				   handle);
		/*user internal matrix */
		DISP_REG_SET_FIELD(handle, CFG_FLD_EXT_MTX_EN, base_addr + DISP_REG_WDMA_CFG, 0);
		DISP_REG_SET_FIELD(handle, CFG_FLD_CT_EN, base_addr + DISP_REG_WDMA_CFG, 1);
		DISP_REG_SET_FIELD(handle, CFG_FLD_INT_MTX_SEL, base_addr + DISP_REG_WDMA_CFG,
				   color_matrix);
	} else {
		DISP_REG_SET_FIELD(handle, CFG_FLD_EXT_MTX_EN, base_addr + DISP_REG_WDMA_CFG, 0);
		DISP_REG_SET_FIELD(handle, CFG_FLD_CT_EN, base_addr + DISP_REG_WDMA_CFG, 0);
	}
	DISP_REG_SET_FIELD(handle, CFG_FLD_SWAP, base_addr + DISP_REG_WDMA_CFG, output_swap);
	if (sec != DISP_SECURE_BUFFER) {
		DISP_REG_SET(handle, base_addr + DISP_REG_WDMA_DST_ADDR0, dstAddress);
	} else {
		int m4u_port;

		m4u_port = module == DISP_MODULE_WDMA0 ? M4U_PORT_DISP_WDMA0 : M4U_PORT_DISP_WDMA1;

		/* for sec layer, addr variable stores sec handle */
		/* we need to pass this handle and offset to cmdq driver */
		/* cmdq sec driver will help to convert handle to correct address */
		disp_cmdq_write_reg_secure(handle, disp_addr_convert(base_addr + DISP_REG_WDMA_DST_ADDR0),
				   CMDQ_SAM_H_2_MVA, dstAddress, 0, size, m4u_port);
	}
	DISP_REG_SET(handle, base_addr + DISP_REG_WDMA_DST_W_IN_BYTE, dstPitch);
	DISP_REG_SET_FIELD(handle, ALPHA_FLD_A_SEL, base_addr + DISP_REG_WDMA_ALPHA,
			   useSpecifiedAlpha);
	DISP_REG_SET_FIELD(handle, ALPHA_FLD_A_VALUE, base_addr + DISP_REG_WDMA_ALPHA, alpha);

	return 0;
}

static int wdma_clock_on(enum DISP_MODULE_ENUM module, void *handle)
{
#ifdef ENABLE_CLK_MGR
	unsigned int idx = wdma_index(module);
	/* DDPMSG("wmda%d_clock_on\n",idx); */
#ifdef CONFIG_MTK_CLKMGR
	if (idx == 0)
		enable_clock(MT_CG_DISP0_DISP_WDMA0, "WDMA0");
	else
		enable_clock(MT_CG_DISP0_DISP_WDMA1, "WDMA1");
#else
	if (idx == 0)
		ddp_clk_enable(DISP0_DISP_WDMA0);
	else
		ddp_clk_enable(DISP0_DISP_WDMA1);
#endif
#endif
	return 0;
}

static int wdma_clock_off(enum DISP_MODULE_ENUM module, void *handle)
{
#ifdef ENABLE_CLK_MGR
	unsigned int idx = wdma_index(module);
	/* DDPMSG("wdma%d_clock_off\n",idx); */
#ifdef CONFIG_MTK_CLKMGR
	if (idx == 0)
		disable_clock(MT_CG_DISP0_DISP_WDMA0, "WDMA0");
	else
		disable_clock(MT_CG_DISP0_DISP_WDMA1, "WDMA1");
#else
	if (idx == 0)
		ddp_clk_disable(DISP0_DISP_WDMA0);
	else
		ddp_clk_disable(DISP0_DISP_WDMA1);
#endif

#endif
	return 0;
}


void wdma_dump_golden_setting(enum DISP_MODULE_ENUM module)
{
	unsigned long base_addr = wdma_base_addr(module);
	unsigned int read_wrk_state;

	read_wrk_state = DISP_REG_GET_FIELD(WDMA_READ_WORK_REG,
			base_addr + DISP_REG_WDMA_EN);
	DISP_REG_SET_FIELD(NULL, WDMA_READ_WORK_REG, base_addr + DISP_REG_OVL_EN, 1);


	DDPDUMP("dump WDMA golden setting\n");

	DDPDUMP("WDMA_SMI_CON:\n[3:0]:%x [4:4]:%x [7:5]:%x [15:8]:%x [19:16]:%u [23:20]:%u [27:24]:%u\n",
			DISP_REG_GET_FIELD(SMI_CON_FLD_THRESHOLD, base_addr + DISP_REG_WDMA_SMI_CON),
			DISP_REG_GET_FIELD(SMI_CON_FLD_SLOW_ENABLE, base_addr + DISP_REG_WDMA_SMI_CON),
			DISP_REG_GET_FIELD(SMI_CON_FLD_SLOW_LEVEL, base_addr + DISP_REG_WDMA_SMI_CON),
			DISP_REG_GET_FIELD(SMI_CON_FLD_SLOW_COUNT, base_addr + DISP_REG_WDMA_SMI_CON),
			DISP_REG_GET_FIELD(SMI_CON_FLD_SMI_Y_REPEAT_NUM, base_addr + DISP_REG_WDMA_SMI_CON),
			DISP_REG_GET_FIELD(SMI_CON_FLD_SMI_U_REPEAT_NUM, base_addr + DISP_REG_WDMA_SMI_CON),
			DISP_REG_GET_FIELD(SMI_CON_FLD_SMI_V_REPEAT_NUM, base_addr + DISP_REG_WDMA_SMI_CON));
	DDPDUMP("WDMA_BUF_CON1:\n[31]:%x [30]:%x [28]:%x [8:0]%d\n",
			DISP_REG_GET_FIELD(BUF_CON1_FLD_ULTRA_ENABLE, base_addr + DISP_REG_WDMA_BUF_CON1),
			DISP_REG_GET_FIELD(BUF_CON1_FLD_PRE_ULTRA_ENABLE, base_addr + DISP_REG_WDMA_BUF_CON1),
			DISP_REG_GET_FIELD(BUF_CON1_FLD_FRAME_END_ULTRA, base_addr + DISP_REG_WDMA_BUF_CON1),
			DISP_REG_GET_FIELD(BUF_CON1_FLD_FIFO_PSEUDO_SIZE, base_addr + DISP_REG_WDMA_BUF_CON1));
	DDPDUMP("WDMA_BUF_CON5:\n[9:0]:%d [25:16]:%d\n",
			DISP_REG_GET_FIELD(BUF_CON_FLD_PRE_ULTRA_LOW, base_addr + DISP_REG_WDMA_BUF_CON5),
			DISP_REG_GET_FIELD(BUF_CON_FLD_ULTRA_LOW, base_addr + DISP_REG_WDMA_BUF_CON5));
	DDPDUMP("WDMA_BUF_CON6:\n[9:0]:%d [25:16]:%d\n",
			DISP_REG_GET_FIELD(BUF_CON_FLD_PRE_ULTRA_HIGH, base_addr + DISP_REG_WDMA_BUF_CON6),
			DISP_REG_GET_FIELD(BUF_CON_FLD_ULTRA_HIGH, base_addr + DISP_REG_WDMA_BUF_CON6));
	DDPDUMP("WDMA_BUF_CON7:\n[9:0]:%d [25:16]:%d\n",
			DISP_REG_GET_FIELD(BUF_CON_FLD_PRE_ULTRA_LOW, base_addr + DISP_REG_WDMA_BUF_CON7),
			DISP_REG_GET_FIELD(BUF_CON_FLD_ULTRA_LOW, base_addr + DISP_REG_WDMA_BUF_CON7));
	DDPDUMP("WDMA_BUF_CON8:\n[9:0]:%d [25:16]:%d\n",
			DISP_REG_GET_FIELD(BUF_CON_FLD_PRE_ULTRA_HIGH, base_addr + DISP_REG_WDMA_BUF_CON8),
			DISP_REG_GET_FIELD(BUF_CON_FLD_ULTRA_HIGH, base_addr + DISP_REG_WDMA_BUF_CON8));
	DDPDUMP("WDMA_BUF_CON9:\n[9:0]:%d [25:16]:%d\n",
			DISP_REG_GET_FIELD(BUF_CON_FLD_PRE_ULTRA_LOW, base_addr + DISP_REG_WDMA_BUF_CON9),
			DISP_REG_GET_FIELD(BUF_CON_FLD_ULTRA_LOW, base_addr + DISP_REG_WDMA_BUF_CON9));
	DDPDUMP("WDMA_BUF_CON10:\n[9:0]:%d [25:16]:%d\n",
			DISP_REG_GET_FIELD(BUF_CON_FLD_PRE_ULTRA_HIGH, base_addr + DISP_REG_WDMA_BUF_CON10),
			DISP_REG_GET_FIELD(BUF_CON_FLD_ULTRA_HIGH, base_addr + DISP_REG_WDMA_BUF_CON10));
	DDPDUMP("WDMA_BUF_CON11:\n[9:0]:%d [25:16]:%d\n",
			DISP_REG_GET_FIELD(BUF_CON_FLD_PRE_ULTRA_LOW, base_addr + DISP_REG_WDMA_BUF_CON11),
			DISP_REG_GET_FIELD(BUF_CON_FLD_ULTRA_LOW, base_addr + DISP_REG_WDMA_BUF_CON11));
	DDPDUMP("WDMA_BUF_CON12:\n[9:0]:%d [25:16]:%d\n",
			DISP_REG_GET_FIELD(BUF_CON_FLD_PRE_ULTRA_HIGH, base_addr + DISP_REG_WDMA_BUF_CON12),
			DISP_REG_GET_FIELD(BUF_CON_FLD_ULTRA_HIGH, base_addr + DISP_REG_WDMA_BUF_CON12));
	DDPDUMP("WDMA_BUF_CON13:\n[9:0]:%d [25:16]:%d\n",
			DISP_REG_GET_FIELD(BUF_CON_FLD_PRE_ULTRA_LOW, base_addr + DISP_REG_WDMA_BUF_CON13),
			DISP_REG_GET_FIELD(BUF_CON_FLD_ULTRA_LOW, base_addr + DISP_REG_WDMA_BUF_CON13));
	DDPDUMP("WDMA_BUF_CON14:\n[9:0]:%d [25:16]:%d\n",
			DISP_REG_GET_FIELD(BUF_CON_FLD_PRE_ULTRA_HIGH, base_addr + DISP_REG_WDMA_BUF_CON14),
			DISP_REG_GET_FIELD(BUF_CON_FLD_ULTRA_HIGH, base_addr + DISP_REG_WDMA_BUF_CON14));
	DDPDUMP("WDMA_BUF_CON15:\n[9:0]:%d [25:16]:%d\n",
			DISP_REG_GET_FIELD(BUF_CON_FLD_PRE_ULTRA_LOW, base_addr + DISP_REG_WDMA_BUF_CON15),
			DISP_REG_GET_FIELD(BUF_CON_FLD_ULTRA_LOW, base_addr + DISP_REG_WDMA_BUF_CON15));
	DDPDUMP("WDMA_BUF_CON16:\n[9:0]:%d [25:16]:%d\n",
			DISP_REG_GET_FIELD(BUF_CON_FLD_PRE_ULTRA_HIGH, base_addr + DISP_REG_WDMA_BUF_CON16),
			DISP_REG_GET_FIELD(BUF_CON_FLD_ULTRA_HIGH, base_addr + DISP_REG_WDMA_BUF_CON16));
	DDPDUMP("WDMA_BUF_CON17:\n[0]:%d [25:16]:%d\n",
			DISP_REG_GET_FIELD(BUF_CON17_FLD_DVFS_EN, base_addr + DISP_REG_WDMA_BUF_CON17),
			DISP_REG_GET_FIELD(BUF_CON17_FLD_DVFS_TH_Y, base_addr + DISP_REG_WDMA_BUF_CON17));
	DDPDUMP("WDMA_BUF_CON18:\n[9:0]:%d [25:16]:%d\n",
			DISP_REG_GET_FIELD(BUF_CON18_FLD_DVFS_TH_U, base_addr + DISP_REG_WDMA_BUF_CON18),
			DISP_REG_GET_FIELD(BUF_CON18_FLD_DVFS_TH_V, base_addr + DISP_REG_WDMA_BUF_CON18));
	DDPDUMP("WDMA_DRS_CON0:\n[0]:%d [25:16]:%d\n",
			DISP_REG_GET_FIELD(BUF_DRS_FLD_DRS_EN, base_addr + DISP_REG_DRS_CON0),
			DISP_REG_GET_FIELD(BUF_DRS_FLD_ENTER_DRS_TH_Y, base_addr + DISP_REG_DRS_CON0));
	DDPDUMP("WDMA_DRS_CON1:\n[9:0]:%d [25:16]:%d\n",
			DISP_REG_GET_FIELD(BUF_DRS_FLD_ENTER_DRS_TH_U, base_addr + DISP_REG_DRS_CON1),
			DISP_REG_GET_FIELD(BUF_DRS_FLD_ENTER_DRS_TH_V, base_addr + DISP_REG_DRS_CON1));
	DDPDUMP("WDMA_DRS_CON2:\n[25:16]:%d\n",
			DISP_REG_GET_FIELD(BUF_DRS_FLD_LEAVE_DRS_TH_Y, base_addr + DISP_REG_DRS_CON2));
	DDPDUMP("WDMA_DRS_CON3:\n[9:0]:%d [25:16]:%d\n",
			DISP_REG_GET_FIELD(BUF_DRS_FLD_LEAVE_DRS_TH_U, base_addr + DISP_REG_DRS_CON3),
			DISP_REG_GET_FIELD(BUF_DRS_FLD_LEAVE_DRS_TH_V, base_addr + DISP_REG_DRS_CON3));
	DDPDUMP("WDMA_BUF_CON3:\n[8:0]:%d [25:16]:%d\n",
			DISP_REG_GET_FIELD(BUF_CON3_FLD_ISSUE_REQ_TH_Y, base_addr + DISP_REG_WDMA_BUF_CON3),
			DISP_REG_GET_FIELD(BUF_CON3_FLD_ISSUE_REQ_TH_U, base_addr + DISP_REG_WDMA_BUF_CON3));
	DDPDUMP("WDMA_BUF_CON4:\n[8:0]:%d\n",
			DISP_REG_GET_FIELD(BUF_CON4_FLD_ISSUE_REQ_TH_V, base_addr + DISP_REG_WDMA_BUF_CON4));

	DISP_REG_SET_FIELD(NULL, WDMA_READ_WORK_REG, base_addr + DISP_REG_OVL_EN, read_wrk_state);
}

void wdma_dump_analysis(enum DISP_MODULE_ENUM module)
{
	unsigned int index = wdma_index(module);
	unsigned long base_addr = wdma_base_addr(module);

	DDPDUMP("== DISP WDMA%d ANALYSIS ==\n", index);
	DDPDUMP("wdma%d:en=%d,w=%d,h=%d,clip=(%d,%d,%d,%d),pitch=(W=%d,UV=%d),addr=(0x%x,0x%x,0x%x),fmt=%s\n",
	     index, DISP_REG_GET(DISP_REG_WDMA_EN + base_addr),
	     DISP_REG_GET(DISP_REG_WDMA_SRC_SIZE + base_addr) & 0x3fff,
	     (DISP_REG_GET(DISP_REG_WDMA_SRC_SIZE + base_addr) >> 16) & 0x3fff,
	     DISP_REG_GET(DISP_REG_WDMA_CLIP_COORD + base_addr) & 0x3fff,
	     (DISP_REG_GET(DISP_REG_WDMA_CLIP_COORD + base_addr) >> 16) & 0x3fff,
	     DISP_REG_GET(DISP_REG_WDMA_CLIP_SIZE + base_addr) & 0x3fff,
	     (DISP_REG_GET(DISP_REG_WDMA_CLIP_SIZE + base_addr) >> 16) & 0x3fff,
	     DISP_REG_GET(DISP_REG_WDMA_DST_W_IN_BYTE + base_addr),
	     DISP_REG_GET(DISP_REG_WDMA_DST_UV_PITCH + base_addr),
	     DISP_REG_GET(DISP_REG_WDMA_DST_ADDR0 + base_addr),
	     DISP_REG_GET(DISP_REG_WDMA_DST_ADDR1 + base_addr),
	     DISP_REG_GET(DISP_REG_WDMA_DST_ADDR2 + base_addr),
	     unified_color_fmt_name(display_fmt_reg_to_unified_fmt
				    ((DISP_REG_GET(DISP_REG_WDMA_CFG + base_addr) >> 4) & 0xf,
				     (DISP_REG_GET(DISP_REG_WDMA_CFG + base_addr) >> 11) & 0x1, 0))
	    );
	DDPDUMP("wdma%d:status=%s,in_req=%d(prev sent data),in_ack=%d(ask data to prev),exec=%d,in_pix=(L:%d,P:%d)\n",
		index,
		wdma_get_status(DISP_REG_GET_FIELD
				(FLOW_CTRL_DBG_FLD_WDMA_STA_FLOW_CTRL,
				 DISP_REG_WDMA_FLOW_CTRL_DBG + base_addr)),
		DISP_REG_GET_FIELD(EXEC_DBG_FLD_WDMA_IN_REQ,
				   DISP_REG_WDMA_FLOW_CTRL_DBG + base_addr),
		DISP_REG_GET_FIELD(EXEC_DBG_FLD_WDMA_IN_ACK,
				   DISP_REG_WDMA_FLOW_CTRL_DBG + base_addr),
		DISP_REG_GET(DISP_REG_WDMA_EXEC_DBG + base_addr) & 0x1f,
		(DISP_REG_GET(DISP_REG_WDMA_CT_DBG + base_addr) >> 16) & 0xffff,
		DISP_REG_GET(DISP_REG_WDMA_CT_DBG + base_addr) & 0xffff);

	wdma_dump_golden_setting(module);
}

void wdma_dump_reg(enum DISP_MODULE_ENUM module)
{
	unsigned int idx = wdma_index(module);
	unsigned long base_addr = wdma_base_addr(module);

	DDPDUMP("== DISP WDMA%d REGS ==\n", idx);

	DDPDUMP("0x000: 0x%08x 0x%08x 0x%08x 0x%08x\n",
		DISP_REG_GET(DISP_REG_WDMA_INTEN + base_addr),
		DISP_REG_GET(DISP_REG_WDMA_INTSTA + base_addr),
		DISP_REG_GET(DISP_REG_WDMA_EN + base_addr),
		DISP_REG_GET(DISP_REG_WDMA_RST + base_addr));

	DDPDUMP("0x010: 0x%08x 0x%08x 0x%08x 0x%08x\n",
		DISP_REG_GET(DISP_REG_WDMA_SMI_CON + base_addr),
		DISP_REG_GET(DISP_REG_WDMA_CFG + base_addr),
		DISP_REG_GET(DISP_REG_WDMA_SRC_SIZE + base_addr),
		DISP_REG_GET(DISP_REG_WDMA_CLIP_SIZE + base_addr));

	DDPDUMP("0x020=0x%08x,0x028=0x%08x,0x02c=0x%08x,0x038=0x%08x\n",
		DISP_REG_GET(DISP_REG_WDMA_CLIP_COORD + base_addr),
		DISP_REG_GET(DISP_REG_WDMA_DST_W_IN_BYTE + base_addr),
		DISP_REG_GET(DISP_REG_WDMA_ALPHA + base_addr),
		DISP_REG_GET(DISP_REG_WDMA_BUF_CON1 + base_addr));

	DDPDUMP("0x058=0x%08x,0x05c=0x%08x,0x060=0x%08x\n",
		DISP_REG_GET(DISP_REG_WDMA_PRE_ADD0 + base_addr),
		DISP_REG_GET(DISP_REG_WDMA_PRE_ADD2 + base_addr),
		DISP_REG_GET(DISP_REG_WDMA_POST_ADD0 + base_addr));

	DDPDUMP("0x064=0x%08x,0x078=0x%08x,0x080=0x%08x,0x084=0x%08x\n",
		DISP_REG_GET(DISP_REG_WDMA_POST_ADD2 + base_addr),
		DISP_REG_GET(DISP_REG_WDMA_DST_UV_PITCH + base_addr),
		DISP_REG_GET(DISP_REG_WDMA_DST_ADDR_OFFSET0 + base_addr),
		DISP_REG_GET(DISP_REG_WDMA_DST_ADDR_OFFSET1 + base_addr));

	DDPDUMP("0x088=0x%08x,0x0a0=0x%08x,0x0a4=0x%08x,0x0a8=0x%08x\n",
		DISP_REG_GET(DISP_REG_WDMA_DST_ADDR_OFFSET2 + base_addr),
		DISP_REG_GET(DISP_REG_WDMA_FLOW_CTRL_DBG + base_addr),
		DISP_REG_GET(DISP_REG_WDMA_EXEC_DBG + base_addr),
		DISP_REG_GET(DISP_REG_WDMA_CT_DBG + base_addr));

	DDPDUMP("0x0ac=0x%08x,0xf00=0x%08x,0xf04=0x%08x,0xf08=0x%08x,\n",
		DISP_REG_GET(DISP_REG_WDMA_DEBUG + base_addr),
		DISP_REG_GET(DISP_REG_WDMA_DST_ADDR0 + base_addr),
		DISP_REG_GET(DISP_REG_WDMA_DST_ADDR1 + base_addr),
		DISP_REG_GET(DISP_REG_WDMA_DST_ADDR2 + base_addr));
}

static int wdma_dump(enum DISP_MODULE_ENUM module, int level)
{
	wdma_dump_analysis(module);
	wdma_dump_golden_setting(module);
	wdma_dump_reg(module);

	return 0;
}

static int wdma_golden_setting(enum DISP_MODULE_ENUM module,
	struct golden_setting_context *p_golden_setting, void *cmdq)
{
	unsigned int regval;
	unsigned long base_addr = wdma_base_addr(module);
	unsigned long res;
	unsigned int ultra_low_us = 4;
	unsigned int ultra_high_us = 6;
	unsigned int preultra_low_us = ultra_high_us;
	unsigned int preultra_high_us = 7;
	unsigned int mmsysclk = 273;
	unsigned long long fill_rate = 0;
	unsigned long long consume_rate = 0;
	unsigned int fifo_pseudo_size = 500;
	unsigned int frame_rate = 60;
	unsigned int bytes_per_sec = 3;
	long long temp;

	unsigned int ultra_low;
	unsigned int preultra_low;
	unsigned int preultra_high;
	unsigned int ultra_high;

	if (!p_golden_setting) {
		DDPERR("golden setting is null, %s,%d\n", __FILE__, __LINE__);
		ASSERT(0);
		return 0;
	}


	if (module == DISP_MODULE_WDMA0)
		res = p_golden_setting->dst_width * p_golden_setting->dst_height;
	else if (p_golden_setting->is_dual_pipe)
		res = p_golden_setting->dst_width * p_golden_setting->dst_height;
	else {
		res = p_golden_setting->ext_dst_width *
				p_golden_setting->ext_dst_height;
	}

	switch (p_golden_setting->mmsys_clk) {
	case MMSYS_CLK_LOW:
		mmsysclk = 364;
		break;
	case MMSYS_CLK_HIGH:
		mmsysclk = 364;
		break;
	default:
		mmsysclk = 364; /* worse case */
		break;
	}

	frame_rate = p_golden_setting->fps;

	/* DISP_REG_WDMA_SMI_CON */
	regval = 0;
	regval |= REG_FLD_VAL(SMI_CON_FLD_THRESHOLD, 7);
	regval |= REG_FLD_VAL(SMI_CON_FLD_SLOW_ENABLE, 0);
	regval |= REG_FLD_VAL(SMI_CON_FLD_SLOW_LEVEL, 0);
	regval |= REG_FLD_VAL(SMI_CON_FLD_SLOW_COUNT, 0);
	regval |= REG_FLD_VAL(SMI_CON_FLD_SMI_Y_REPEAT_NUM, 4);
	regval |= REG_FLD_VAL(SMI_CON_FLD_SMI_U_REPEAT_NUM, 2);
	regval |= REG_FLD_VAL(SMI_CON_FLD_SMI_V_REPEAT_NUM, 2);
	DISP_REG_SET(cmdq, base_addr + DISP_REG_WDMA_SMI_CON, regval);

	/* DISP_REG_WDMA_BUF_CON1 */
	regval = 0;
	if (p_golden_setting->is_dc)
		regval |= REG_FLD_VAL(BUF_CON1_FLD_ULTRA_ENABLE, 0);
	else
		regval |= REG_FLD_VAL(BUF_CON1_FLD_ULTRA_ENABLE, 1);

	regval |= REG_FLD_VAL(BUF_CON1_FLD_PRE_ULTRA_ENABLE, 1);

	if (p_golden_setting->is_dc)
		regval |= REG_FLD_VAL(BUF_CON1_FLD_FRAME_END_ULTRA, 0);
	else
		regval |= REG_FLD_VAL(BUF_CON1_FLD_FRAME_END_ULTRA, 1);

	regval |= REG_FLD_VAL(BUF_CON1_FLD_FIFO_PSEUDO_SIZE, fifo_pseudo_size);

	DISP_REG_SET(cmdq, base_addr + DISP_REG_WDMA_BUF_CON1, regval);

	/* DISP_REG_WDMA_BUF_CON3 */
	regval = 0;
	regval |= REG_FLD_VAL(BUF_CON3_FLD_ISSUE_REQ_TH_Y, 16);
	regval |= REG_FLD_VAL(BUF_CON3_FLD_ISSUE_REQ_TH_U, 16);

	DISP_REG_SET(cmdq, base_addr + DISP_REG_WDMA_BUF_CON3, regval);

	/* DISP_REG_WDMA_BUF_CON4 */
	regval = 0;
	regval |= REG_FLD_VAL(BUF_CON4_FLD_ISSUE_REQ_TH_V, 16);

	DISP_REG_SET(cmdq, base_addr + DISP_REG_WDMA_BUF_CON4, regval);


	if (p_golden_setting->is_dc)
		fill_rate = 960 * mmsysclk;
	else
		fill_rate = 960 * mmsysclk * 3 / 16;

	consume_rate = res * frame_rate * bytes_per_sec;
	do_div(consume_rate, 1000);
	consume_rate *= 1250;
	do_div(consume_rate, 16 * 1000);

	if (p_golden_setting->is_dual_pipe)
		do_div(consume_rate, 2);

	preultra_low = preultra_low_us * consume_rate;
	do_div(preultra_low, 1000);

	preultra_high = preultra_high_us * consume_rate;
	do_div(preultra_high, 1000);

	ultra_low = ultra_low_us * consume_rate;
	do_div(ultra_low, 1000);

	ultra_high = preultra_low;

	/* DISP_REG_WDMA_BUF_CON5 */
	regval = 0;
	temp = fifo_pseudo_size - preultra_high;
	regval |= REG_FLD_VAL(BUF_CON_FLD_PRE_ULTRA_LOW, temp);
	temp = fifo_pseudo_size - ultra_high;
	regval |= REG_FLD_VAL(BUF_CON_FLD_ULTRA_LOW, temp);

	DISP_REG_SET(cmdq, base_addr + DISP_REG_WDMA_BUF_CON5, regval);

	/* DISP_REG_WDMA_BUF_CON6 */
	regval = 0;
	temp = fifo_pseudo_size - preultra_low;
	regval |= REG_FLD_VAL(BUF_CON_FLD_PRE_ULTRA_HIGH, temp);
	temp = fifo_pseudo_size - ultra_low;
	regval |= REG_FLD_VAL(BUF_CON_FLD_ULTRA_HIGH, temp);

	DISP_REG_SET(cmdq, base_addr + DISP_REG_WDMA_BUF_CON6, regval);

	/* DISP_REG_WDMA_BUF_CON7 */
	regval = 0;
	temp = fifo_pseudo_size - preultra_high;
	temp = DIV_ROUND_UP(temp, 2);
	regval |= REG_FLD_VAL(BUF_CON_FLD_PRE_ULTRA_LOW, temp);
	temp = fifo_pseudo_size - ultra_high;
	temp = DIV_ROUND_UP(temp, 2);
	regval |= REG_FLD_VAL(BUF_CON_FLD_ULTRA_LOW, temp);

	DISP_REG_SET(cmdq, base_addr + DISP_REG_WDMA_BUF_CON7, regval);

	/* DISP_REG_WDMA_BUF_CON8 */
	regval = 0;
	temp = fifo_pseudo_size - preultra_low;
	temp = DIV_ROUND_UP(temp, 2);
	regval |= REG_FLD_VAL(BUF_CON_FLD_PRE_ULTRA_HIGH, temp);
	temp = fifo_pseudo_size - ultra_low;
	temp = DIV_ROUND_UP(temp, 2);
	regval |= REG_FLD_VAL(BUF_CON_FLD_ULTRA_HIGH, temp);

	DISP_REG_SET(cmdq, base_addr + DISP_REG_WDMA_BUF_CON8, regval);

	/* DISP_REG_WDMA_BUF_CON9 */
	regval = 0;
	temp = fifo_pseudo_size - preultra_high;
	temp = DIV_ROUND_UP(temp, 4);
	regval |= REG_FLD_VAL(BUF_CON_FLD_PRE_ULTRA_LOW, temp);
	temp = fifo_pseudo_size - ultra_high;
	temp = DIV_ROUND_UP(temp, 4);
	regval |= REG_FLD_VAL(BUF_CON_FLD_ULTRA_LOW, temp);

	DISP_REG_SET(cmdq, base_addr + DISP_REG_WDMA_BUF_CON9, regval);

	/* DISP_REG_WDMA_BUF_CON10 */
	regval = 0;
	temp = fifo_pseudo_size - preultra_low;
	temp = DIV_ROUND_UP(temp, 4);
	regval |= REG_FLD_VAL(BUF_CON_FLD_PRE_ULTRA_HIGH, temp);
	temp = fifo_pseudo_size - ultra_low;
	temp = DIV_ROUND_UP(temp, 4);
	regval |= REG_FLD_VAL(BUF_CON_FLD_ULTRA_HIGH, temp);

	DISP_REG_SET(cmdq, base_addr + DISP_REG_WDMA_BUF_CON10, regval);

	preultra_low = (preultra_low_us + 2) * consume_rate;
	do_div(preultra_low, 1000);

	preultra_high = (preultra_high_us + 2) * consume_rate;
	do_div(preultra_high, 1000);

	ultra_low = (ultra_low_us + 2) * consume_rate;
	do_div(ultra_low, 1000);

	ultra_high = preultra_low;

	/* DISP_REG_WDMA_BUF_CON11 */
	regval = 0;
	temp = fifo_pseudo_size - preultra_high;
	regval |= REG_FLD_VAL(BUF_CON_FLD_PRE_ULTRA_LOW, temp);
	temp = fifo_pseudo_size - ultra_high;
	regval |= REG_FLD_VAL(BUF_CON_FLD_ULTRA_LOW, temp);

	DISP_REG_SET(cmdq, base_addr + DISP_REG_WDMA_BUF_CON11, regval);

	/* DISP_REG_WDMA_BUF_CON12 */
	regval = 0;
	temp = fifo_pseudo_size - preultra_low;
	regval |= REG_FLD_VAL(BUF_CON_FLD_PRE_ULTRA_HIGH, temp);
	temp = fifo_pseudo_size - ultra_low;
	regval |= REG_FLD_VAL(BUF_CON_FLD_ULTRA_HIGH, temp);


	DISP_REG_SET(cmdq, base_addr + DISP_REG_WDMA_BUF_CON12, regval);

	/* DISP_REG_WDMA_BUF_CON13 */
	regval = 0;
	temp = fifo_pseudo_size - preultra_high;
	temp = DIV_ROUND_UP(temp, 2);
	temp = (temp > 0) ? temp : 0;
	regval |= REG_FLD_VAL(BUF_CON_FLD_PRE_ULTRA_LOW, temp);
	temp = fifo_pseudo_size - ultra_high;
	temp = DIV_ROUND_UP(temp, 2);
	temp = (temp > 0) ? temp : 0;
	regval |= REG_FLD_VAL(BUF_CON_FLD_ULTRA_LOW, temp);


	DISP_REG_SET(cmdq, base_addr + DISP_REG_WDMA_BUF_CON13, regval);

	/* DISP_REG_WDMA_BUF_CON14 */
	regval = 0;
	temp = fifo_pseudo_size - preultra_low;
	temp = DIV_ROUND_UP(temp, 2);
	regval |= REG_FLD_VAL(BUF_CON_FLD_PRE_ULTRA_HIGH, temp);
	temp = fifo_pseudo_size - ultra_low;
	temp = DIV_ROUND_UP(temp, 2);
	regval |= REG_FLD_VAL(BUF_CON_FLD_ULTRA_HIGH, temp);


	DISP_REG_SET(cmdq, base_addr + DISP_REG_WDMA_BUF_CON14, regval);

	/* DISP_REG_WDMA_BUF_CON15 */
	regval = 0;
	temp = fifo_pseudo_size - preultra_high;
	temp = DIV_ROUND_UP(temp, 4);
	regval |= REG_FLD_VAL(BUF_CON_FLD_PRE_ULTRA_LOW, temp);
	temp = fifo_pseudo_size - ultra_high;
	temp = DIV_ROUND_UP(temp, 4);
	regval |= REG_FLD_VAL(BUF_CON_FLD_ULTRA_LOW, temp);


	DISP_REG_SET(cmdq, base_addr + DISP_REG_WDMA_BUF_CON15, regval);

	/* DISP_REG_WDMA_BUF_CON16 */
	regval = 0;
	temp = fifo_pseudo_size - preultra_low;
	temp = DIV_ROUND_UP(temp, 4);
	regval |= REG_FLD_VAL(BUF_CON_FLD_PRE_ULTRA_HIGH, temp);
	temp = fifo_pseudo_size - ultra_low;
	temp = DIV_ROUND_UP(temp, 4);
	regval |= REG_FLD_VAL(BUF_CON_FLD_ULTRA_HIGH, temp);


	DISP_REG_SET(cmdq, base_addr + DISP_REG_WDMA_BUF_CON16, regval);

	/* DISP_REG_WDMA_BUF_CON17 */
	regval = 0;
	/* TODO: SET DVFS_EN */
	temp = fifo_pseudo_size - ultra_low;
	regval |= REG_FLD_VAL(BUF_CON17_FLD_DVFS_TH_Y, temp);

	DISP_REG_SET(cmdq, base_addr + DISP_REG_WDMA_BUF_CON17, regval);

	/* DISP_REG_WDMA_BUF_CON18 */
	regval = 0;
	temp = fifo_pseudo_size - ultra_low;
	do_div(temp, 2);
	regval |= REG_FLD_VAL(BUF_CON18_FLD_DVFS_TH_U, temp);
	temp = fifo_pseudo_size - ultra_low;
	do_div(temp, 4);
	regval |= REG_FLD_VAL(BUF_CON18_FLD_DVFS_TH_V, temp);

	DISP_REG_SET(cmdq, base_addr + DISP_REG_WDMA_BUF_CON18, regval);

	/* DISP_REG_WDMA_DRS_CON0 */
	regval = 0;
	/* TODO: SET DRS_EN */
	temp = fifo_pseudo_size - ultra_low;
	regval |= REG_FLD_VAL(BUF_DRS_FLD_ENTER_DRS_TH_Y, temp);

	DISP_REG_SET(cmdq, base_addr + DISP_REG_DRS_CON0, regval);

	/* DISP_REG_WDMA_DRS_CON1 */
	regval = 0;
	temp = fifo_pseudo_size - ultra_low;
	do_div(temp, 2);
	regval |= REG_FLD_VAL(BUF_DRS_FLD_ENTER_DRS_TH_U, temp);
	temp = fifo_pseudo_size - ultra_low;
	do_div(temp, 4);
	regval |= REG_FLD_VAL(BUF_DRS_FLD_ENTER_DRS_TH_V, temp);

	DISP_REG_SET(cmdq, base_addr + DISP_REG_DRS_CON1, regval);

	ultra_low = (ultra_low_us + 4) * consume_rate;
	do_div(ultra_low, 1000);

	regval = 0;
	temp = fifo_pseudo_size - ultra_low;
	regval |= REG_FLD_VAL(BUF_DRS_FLD_LEAVE_DRS_TH_Y, temp);

	DISP_REG_SET(cmdq, base_addr + DISP_REG_DRS_CON2, regval);

	regval = 0;
	temp = fifo_pseudo_size - ultra_low;
	do_div(temp, 2);
	regval |= REG_FLD_VAL(BUF_DRS_FLD_LEAVE_DRS_TH_U, temp);
	temp = fifo_pseudo_size - ultra_low;
	do_div(temp, 4);
	regval |= REG_FLD_VAL(BUF_DRS_FLD_LEAVE_DRS_TH_V, temp);

	DISP_REG_SET(cmdq, base_addr + DISP_REG_DRS_CON3, regval);


	return 0;
}


static int wdma_check_input_param(struct WDMA_CONFIG_STRUCT *config)
{
	if (!is_unified_color_fmt_supported(config->outputFormat)) {
		DDPERR("wdma parameter invalidate outfmt %s:0x%x\n",
		       unified_color_fmt_name(config->outputFormat), config->outputFormat);
		return -1;
	}

	if (config->dstAddress == 0 || config->srcWidth == 0 || config->srcHeight == 0) {
		DDPERR("wdma parameter invalidate, addr=0x%lx, w=%d, h=%d\n",
		       config->dstAddress, config->srcWidth, config->srcHeight);
		return -1;
	}
	return 0;
}

static int wdma_is_sec[2];
static inline int wdma_switch_to_sec(enum DISP_MODULE_ENUM module, void *handle)
{
	unsigned int wdma_idx = wdma_index(module);
	/*int *wdma_is_sec = svp_pgc->module_sec.wdma_sec;*/
	enum CMDQ_ENG_ENUM cmdq_engine;
	enum CMDQ_EVENT_ENUM cmdq_event;

	/*cmdq_engine = module_to_cmdq_engine(module);*/
	cmdq_engine = wdma_idx == 0 ? CMDQ_ENG_DISP_WDMA0 : CMDQ_ENG_DISP_WDMA1;
	cmdq_event  = wdma_idx == 0 ? CMDQ_EVENT_DISP_WDMA0_EOF : CMDQ_EVENT_DISP_WDMA1_EOF;

	disp_cmdq_set_secure(handle, 1);
	/* set engine as sec */
	disp_cmdq_enable_port_security(handle, (1LL << cmdq_engine));
	disp_cmdq_enable_dapc(handle, (1LL << cmdq_engine));
	if (wdma_is_sec[wdma_idx] == 0) {
		DDPSVPMSG("[SVP] switch wdma%d to sec\n", wdma_idx);
		mmprofile_log_ex(ddp_mmp_get_events()->svp_module[module],
			MMPROFILE_FLAG_START, 0, 0);
		/*mmprofile_log_ex(ddp_mmp_get_events()->svp_module[module],
		 *	MMPROFILE_FLAG_PULSE, wdma_idx, 1);
		 */
	}
	wdma_is_sec[wdma_idx] = 1;

	return 0;
}

int wdma_switch_to_nonsec(enum DISP_MODULE_ENUM module, void *handle)
{
	unsigned int wdma_idx = wdma_index(module);

	enum CMDQ_ENG_ENUM cmdq_engine;
	enum CMDQ_EVENT_ENUM cmdq_event;

	cmdq_engine = wdma_idx == 0 ? CMDQ_ENG_DISP_WDMA0 : CMDQ_ENG_DISP_WDMA1;
	cmdq_event  = wdma_idx == 0 ? CMDQ_EVENT_DISP_WDMA0_EOF : CMDQ_EVENT_DISP_WDMA1_EOF;

	if (wdma_is_sec[wdma_idx] == 1) {
		/* wdma is in sec stat, we need to switch it to nonsec */
		struct cmdqRecStruct *nonsec_switch_handle;
		int ret;

		ret = disp_cmdq_create(CMDQ_SCENARIO_DISP_PRIMARY_DISABLE_SECURE_PATH,
			&(nonsec_switch_handle));
		if (ret) {
			DDPERR("[SVP]%s:%d, create cmdq handle fail!ret=%d\n", __func__, __LINE__, ret);
			return -1;
		}

		disp_cmdq_reset(nonsec_switch_handle);

		if (wdma_idx == 0) {
			/*Primary Mode*/
			if (primary_display_is_decouple_mode())
				disp_cmdq_wait_event_no_clear(nonsec_switch_handle, cmdq_event);
			else
				_cmdq_insert_wait_frame_done_token_mira(nonsec_switch_handle);
		} else {
			/*External Mode*/
			/*ovl1->wdma1*/
			/*_cmdq_insert_wait_frame_done_token_mira(nonsec_switch_handle);*/
			disp_cmdq_wait_event_no_clear(nonsec_switch_handle, CMDQ_SYNC_DISP_EXT_STREAM_EOF);
		}

		/*_cmdq_insert_wait_frame_done_token_mira(nonsec_switch_handle);*/
		disp_cmdq_set_secure(nonsec_switch_handle, 1);

		/*in fact, dapc/port_sec will be disabled by cmdq */
		disp_cmdq_enable_port_security(nonsec_switch_handle, (1LL << cmdq_engine));
		disp_cmdq_enable_dapc(nonsec_switch_handle, (1LL << cmdq_engine));
		if (handle != NULL) {
			/*Async Flush method*/
			enum CMDQ_EVENT_ENUM cmdq_event_nonsec_end;
			/*cmdq_event_nonsec_end  = module_to_cmdq_event_nonsec_end(module);*/
			cmdq_event_nonsec_end  = wdma_idx == 0 ? CMDQ_SYNC_DISP_WDMA0_2NONSEC_END
						: CMDQ_SYNC_DISP_WDMA1_2NONSEC_END;
			disp_cmdq_set_event(nonsec_switch_handle, cmdq_event_nonsec_end);
			disp_cmdq_flush_async(nonsec_switch_handle, __func__, __LINE__);
			disp_cmdq_wait_event(handle, cmdq_event_nonsec_end);
		} else {
			/*Sync Flush method*/
			disp_cmdq_flush(nonsec_switch_handle, __func__, __LINE__);
		}
		disp_cmdq_destroy(nonsec_switch_handle, __func__, __LINE__);
		nonsec_switch_handle = NULL;
		DDPSVPMSG("[SVP] switch wdma%d to nonsec\n", wdma_idx);
		mmprofile_log_ex(ddp_mmp_get_events()->svp_module[module],
			MMPROFILE_FLAG_END, 0, 0);
		/*mmprofile_log_ex(ddp_mmp_get_events()->svp_module[module],
		 *MMPROFILE_FLAG_PULSE, wdma_idx, 0);
		 */
	}
	wdma_is_sec[wdma_idx] = 0;

	return 0;
}

int setup_wdma_sec(enum DISP_MODULE_ENUM module, struct disp_ddp_path_config *pConfig, void *handle)
{
	int ret;
	int is_engine_sec = 0;

	if (pConfig->wdma_config.security == DISP_SECURE_BUFFER)
		is_engine_sec = 1;

	if (!handle) {
		DDPDBG("[SVP] bypass wdma sec setting sec=%d,handle=NULL\n", is_engine_sec);
		return 0;
	}

	if (is_engine_sec == 1)
		ret = wdma_switch_to_sec(module, handle);
	else
		ret = wdma_switch_to_nonsec(module, NULL);/*hadle = NULL, use the sync flush method*/
	if (ret)
		DDPAEE("[SVP]fail to setup_ovl_sec: %s ret=%d\n",
			__func__, ret);

	return is_engine_sec;
}

static int wdma_config_l(enum DISP_MODULE_ENUM module, struct disp_ddp_path_config *pConfig, void *handle)
{

	struct WDMA_CONFIG_STRUCT *config = &pConfig->wdma_config;

	if (!pConfig->wdma_dirty)
		return 0;

#if 0
	int wdma_idx = wdma_index(module);
	enum CMDQ_ENG_ENUM cmdq_engine;
	enum CMDQ_EVENT_ENUM cmdq_event;
	enum CMDQ_EVENT_ENUM cmdq_event_nonsec_end;

	cmdq_engine = wdma_idx == 0 ? CMDQ_ENG_DISP_WDMA0 : CMDQ_ENG_DISP_WDMA1;
	cmdq_event  = wdma_idx == 0 ? CMDQ_EVENT_DISP_WDMA0_EOF : CMDQ_EVENT_DISP_WDMA1_EOF;
	cmdq_event_nonsec_end  = wdma_idx == 0 ? CMDQ_SYNC_DISP_WDMA0_2NONSEC_END : CMDQ_SYNC_DISP_WDMA1_2NONSEC_END;

	if (config->security == DISP_SECURE_BUFFER) {
		disp_cmdq_set_secure(handle, 1);

		/* set engine as sec */
		disp_cmdq_enable_port_security(handle, (1LL << cmdq_engine));
		disp_cmdq_enable_dapc(handle, (1LL << cmdq_engine));
		if (wdma_is_sec[wdma_idx] == 0)
			DDPMSG("[SVP] switch wdma%d to sec\n", wdma_idx);
		wdma_is_sec[wdma_idx] = 1;
	} else {
		if (wdma_is_sec[wdma_idx]) {
			/* wdma is in sec stat, we need to switch it to nonsec */
			struct cmdqRecStruct *nonsec_switch_handle;
			int ret;

			ret = disp_cmdq_create(CMDQ_SCENARIO_DISP_PRIMARY_DISABLE_SECURE_PATH,
					    &(nonsec_switch_handle));
			if (ret) {
				DDPERR("[SVP]%s:%d, create cmdq handle fail!ret=%d\n", __func__, __LINE__, ret);
				return -1;
			}

			disp_cmdq_reset(nonsec_switch_handle);

			if (wdma_idx == 0) {
				/*Primary Mode*/
				if (primary_display_is_decouple_mode())
					disp_cmdq_wait_event_no_clear(nonsec_switch_handle, cmdq_event);
				else
					_cmdq_insert_wait_frame_done_token_mira(nonsec_switch_handle);
			} else {
				/*External Mode*/
				/*ovl1->wdma1*/
				disp_cmdq_wait_event_no_clear(nonsec_switch_handle, CMDQ_SYNC_DISP_EXT_STREAM_EOF);
			}

			/*_cmdq_insert_wait_frame_done_token_mira(nonsec_switch_handle);*/
			disp_cmdq_set_secure(nonsec_switch_handle, 1);

			/*in fact, dapc/port_sec will be disabled by cmdq */
			disp_cmdq_enable_port_security(nonsec_switch_handle, (1LL << cmdq_engine));
			disp_cmdq_enable_dapc(nonsec_switch_handle, (1LL << cmdq_engine));
			disp_cmdq_set_event(nonsec_switch_handle, cmdq_event_nonsec_end);
			disp_cmdq_flush_async(nonsec_switch_handle, __func__, __LINE__);
			disp_cmdq_destroy(nonsec_switch_handle, __func__, __LINE__);
			nonsec_switch_handle = NULL;
			disp_cmdq_wait_event(handle, cmdq_event_nonsec_end);
			DDPMSG("[SVP] switch wdma%d to nonsec\n", wdma_idx);
		}
		wdma_is_sec[wdma_idx] = 0;
	}
#else
	setup_wdma_sec(module, pConfig, handle);
#endif
	if (wdma_check_input_param(config) == 0) {
		enum dst_module_type dst_mod_type;
		struct golden_setting_context *p_golden_setting;

		wdma_config(module,
			    config->srcWidth,
			    config->srcHeight,
			    config->clipX,
			    config->clipY,
			    config->clipWidth,
			    config->clipHeight,
			    config->outputFormat,
			    config->dstAddress,
			    config->dstPitch,
			    config->useSpecifiedAlpha, config->alpha, config->security, handle);

		dst_mod_type = dpmgr_path_get_dst_module_type(pConfig->path_handle);
		p_golden_setting = pConfig->p_golden_setting_context;
		wdma_golden_setting(module, p_golden_setting, handle);
	}
	return 0;
}

struct DDP_MODULE_DRIVER ddp_driver_wdma = {
	.module = DISP_MODULE_WDMA0,
	.init = wdma_clock_on,
	.deinit = wdma_clock_off,
	.config = wdma_config_l,
	.start = wdma_start,
	.trigger = NULL,
	.stop = wdma_stop,
	.reset = wdma_reset,
	.power_on = wdma_clock_on,
	.power_off = wdma_clock_off,
	.is_idle = NULL,
	.is_busy = NULL,
	.dump_info = wdma_dump,
	.bypass = NULL,
	.build_cmdq = NULL,
	.set_lcm_utils = NULL,
	.switch_to_nonsec = wdma_switch_to_nonsec,
};
