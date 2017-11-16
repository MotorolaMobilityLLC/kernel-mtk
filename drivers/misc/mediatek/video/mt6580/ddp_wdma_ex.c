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
#include <mach/mt_clkmgr.h>
#include <linux/delay.h>

#include "disp_log.h"
#include "ddp_reg.h"
#include "ddp_matrix_para.h"
#include "ddp_info.h"
#include "ddp_wdma.h"
#include "ddp_wdma_ex.h"
#include "ddp_color_format.h"
#include "primary_display.h"
#include "m4u.h"
#include "ddp_drv.h"

#define WDMA_COLOR_SPACE_RGB (0)
#define WDMA_COLOR_SPACE_YUV (1)

enum WDMA_OUTPUT_FORMAT {
	WDMA_OUTPUT_FORMAT_BGR565 = 0x00,	/* basic format */
	WDMA_OUTPUT_FORMAT_RGB888 = 0x01,
	WDMA_OUTPUT_FORMAT_RGBA8888 = 0x02,
	WDMA_OUTPUT_FORMAT_ARGB8888 = 0x03,
	WDMA_OUTPUT_FORMAT_VYUY = 0x04,
	WDMA_OUTPUT_FORMAT_YVYU = 0x05,
	WDMA_OUTPUT_FORMAT_YONLY = 0x07,
	WDMA_OUTPUT_FORMAT_YV12 = 0x08,
	WDMA_OUTPUT_FORMAT_NV21 = 0x0c,

	WDMA_OUTPUT_FORMAT_UNKNOWN = 0x100,
};


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

int wdma_start(DISP_MODULE_ENUM module, void *handle)
{
	unsigned int idx = wdma_index(module);

	DISP_REG_SET(handle, idx * DISP_WDMA_INDEX_OFFSET + DISP_REG_WDMA_INTEN, 0x07);
	DISP_REG_SET(handle, idx * DISP_WDMA_INDEX_OFFSET + DISP_REG_WDMA_EN, 0x01);

	return 0;
}

static int wdma_config_uv(DISP_MODULE_ENUM module, unsigned long uAddr, unsigned long vAddr,
			  unsigned int uvpitch, void *handle)
{
	unsigned int idx = wdma_index(module);
	unsigned int idx_offst = idx * DISP_WDMA_INDEX_OFFSET;

	DISP_REG_SET(handle, idx_offst + DISP_REG_WDMA_DST_ADDR1, uAddr);
	DISP_REG_SET(handle, idx_offst + DISP_REG_WDMA_DST_ADDR2, vAddr);
	DISP_REG_SET_FIELD(handle, DST_W_IN_BYTE_FLD_DST_W_IN_BYTE,
			   idx_offst + DISP_REG_WDMA_DST_UV_PITCH, uvpitch);
	return 0;
}

static int wdma_config_yuv420(DISP_MODULE_ENUM module,
			      DpColorFormat fmt,
			      unsigned int dstPitch,
			      unsigned int Height, unsigned dstAddress, void *handle)
{
	unsigned int u_add = 0;
	unsigned int v_add = 0;
	unsigned int uv_add = 0;
	unsigned int c_stride = 0;
	unsigned int y_size = 0;
	unsigned int c_size = 0;
	unsigned int stride = dstPitch;

	if (fmt == eYV12) {
		y_size = stride * Height;
		c_stride = ALIGN_TO(stride / 2, 16);
		c_size = c_stride * Height / 2;
		u_add = dstAddress + y_size;
		v_add = dstAddress + y_size + c_size;
		wdma_config_uv(module, u_add, v_add, c_stride, handle);
	} else if (fmt == eYV21) {
		y_size = stride * Height;
		c_stride = ALIGN_TO(stride / 2, 16);
		c_size = c_stride * Height / 2;
		u_add = dstAddress + y_size;
		v_add = dstAddress + y_size + c_size;
		wdma_config_uv(module, u_add, v_add, c_stride, handle);
	} else if (fmt == eNV12 || fmt == eNV21) {
		y_size = stride * Height;
		c_stride = stride / 2;
		uv_add = dstAddress + y_size;
		wdma_config_uv(module, uv_add, 0, c_stride, handle);
	}
	return 0;
}

/* -------------------------------------------------------- */
/* calculate disp_wdma ultra/pre-ultra setting */
/* to start to issue ultra from fifo having 4us data */
/* to stop  to issue ultra until fifo having 6us data */
/* to start to issue pre-ultra from fifo having 6us data */
/* to stop  to issue pre-ultra until fifo having 7us data */
/* the sequence is pre_ultra_low < ultra_low < pre_ultra_high < ultra_high */
/* 4us             6us         6us+1level       7us */
/* -------------------------------------------------------- */
void wdma_calc_ultra(unsigned int idx, unsigned int width, unsigned int height, unsigned int bpp,
		     unsigned int frame_rate, void *handle)
{
	/* constant */
	unsigned int blank_overhead = 115;	/* it is 1.15, need to divide 100 later */
	unsigned int rdma_fifo_width = 16;	/* in unit of byte */
	/* bpp is defined by disp_wdma's output format */
	unsigned int pre_ultra_low_time = 0;	/* in unit of 0.5 us */
	unsigned int ultra_low_time = 0;	/* in unit of 0.5 us */
	unsigned int ultra_high_time = 0;	/* in unit of 0.5 us */

	/* working variables */
	unsigned int consume_levels_per_sec;
	unsigned int ultra_low_level;
	unsigned int pre_ultra_low_level;
	unsigned int ultra_high_level;
	unsigned int ultra_high_ofs;
	unsigned int ultra_low_ofs;
	unsigned int pre_ultra_high_ofs;

	if (bpp == 3) {		/* YV12 */
		pre_ultra_low_time = 2;	/* in unit of 0.5 us */
		ultra_low_time = 4;	/* in unit of 0.5 us */
		ultra_high_time = 8;	/* in unit of 0.5 us */
	} else if (bpp == 6) {	/* RGB888 */
		pre_ultra_low_time = 2;	/* in unit of 0.5 us */
		ultra_low_time = 4;	/* in unit of 0.5 us */
		ultra_high_time = 5;	/* in unit of 0.5 us */
	} else {
		DISPERR("Disp wdma bpp is wrong value %d\n", bpp);
	}
	/* consume_levels_per_sec =
		((long long unsigned int)width*height*frame_rate*blank_overhead*bpp)/rdma_fifo_width/100; */
	/* change calculation order to prevent overflow of unsigned int */
	/* bpp/2 for bpp in unit of 0.5 bytes/pixel */
	consume_levels_per_sec =
	    (width * height * frame_rate * bpp / 2 / rdma_fifo_width / 100) * blank_overhead;

	/* /1000000 /2 for ultra_low_time in unit of 0.5 us */
	ultra_low_level = (unsigned int)(ultra_low_time * consume_levels_per_sec / 1000000 / 2);
	pre_ultra_low_level =
	    (unsigned int)(pre_ultra_low_time * consume_levels_per_sec / 1000000 / 2);
	ultra_high_level = (unsigned int)(ultra_high_time * consume_levels_per_sec / 1000000 / 2);

	ultra_low_ofs = ultra_low_level - pre_ultra_low_level;
	pre_ultra_high_ofs = 1;
	ultra_high_ofs = ultra_high_level - ultra_low_level;

	ultra_low_level = 0x86;
	ultra_low_ofs = 0x35;
	pre_ultra_high_ofs = 1;
	ultra_high_ofs = 0x1b;
	/* write ultra_high_ofs, pre_ultra_high_ofs, ultra_low_ofs, pre_ultra_low_ofs=pre_ultra_low_level
		into register DISP_WDMA_BUF_CON2 */
	/* DISP_REG_SET(handle,idx*DISP_WDMA_INDEX_OFFSET+DISP_REG_WDMA_BUF_CON2, */
	/* ultra_low_level|(ultra_low_ofs<<8)|(pre_ultra_high_ofs<<16)|(ultra_high_ofs<<24)); */

	/* TODO: set ultra/pre-ultra by resolution and format */
	/*
	   YV12: FHD=0x19010ea2, HD=0x0b0106d6, qHD=0x060103e9
	   UYVY: FHD=0x22011283, HD=0x0f0108c8, qHD=0x080104e1
	   RGB:  FHD=0x35011b44, HD=0x17010bad, qHD=0x0c0107d1
	 */

	DISP_REG_SET(handle, idx * DISP_WDMA_INDEX_OFFSET + DISP_REG_WDMA_BUF_CON2, 0x1B010E22);
	DISP_REG_SET(handle, idx * DISP_WDMA_INDEX_OFFSET + DISP_REG_WDMA_BUF_CON1, 0xD0100080);

	DISPDBG("WDMA ultra_low_ofs      = 0x%03x = %d\n", ultra_low_ofs        , ultra_low_ofs);
	DISPDBG("WDMA ultra_low_ofs      = 0x%03x = %d\n", ultra_low_ofs        , ultra_low_ofs);
	DISPDBG("WDMA pre_ultra_high_ofs = 0x%03x = %d\n", pre_ultra_high_ofs   , pre_ultra_high_ofs);
	DISPDBG("WDMA pre_ultra_high_ofs = 0x%03x = %d\n", pre_ultra_high_ofs   , pre_ultra_high_ofs);
	DISPDBG("WDMA ultra_high_ofs     = 0x%03x = %d\n", ultra_high_ofs       , ultra_high_ofs);
	DISPDBG("WDMA ultra_high_ofs     = 0x%03x = %d\n", ultra_high_ofs       , ultra_high_ofs);
}

static int wdma_config(DISP_MODULE_ENUM module,
		       unsigned srcWidth,
		       unsigned srcHeight,
		       unsigned clipX,
		       unsigned clipY,
		       unsigned clipWidth,
		       unsigned clipHeight,
		       DpColorFormat out_format,
		       unsigned long dstAddress,
		       unsigned dstPitch,
		       unsigned int useSpecifiedAlpha, unsigned char alpha, void *handle)
{
	unsigned int idx = wdma_index(module);
	unsigned int output_swap = fmt_swap(out_format);
	unsigned int output_color_space = fmt_color_space(out_format);
	/* unsigned int bpp = fmt_bpp(out_format); */
	unsigned int out_fmt_reg = fmt_hw_value(out_format);
	unsigned int yuv444_to_yuv422 = 0;
	int color_matrix = 0x2;	/* 0010 RGB_TO_BT601 */
	unsigned int idx_offst = idx * DISP_WDMA_INDEX_OFFSET;

	DISPDBG(
		"module %s, src(w=%d,h=%d), clip(x=%d,y=%d,w=%d,h=%d),out_fmt=%s,dst_address=0x%lx,dst_p=%d,spific_alfa= %d,alpa=%d,handle=%p\n",
		ddp_get_module_name(module),
		srcWidth,
		srcHeight,
		clipX,
		clipY,
		clipWidth,
		clipHeight,
		fmt_string(out_format),
		dstAddress,
		dstPitch,
		useSpecifiedAlpha,
		alpha,
		handle);

	/* should use OVL alpha instead of sw config */
	DISP_REG_SET(handle, idx_offst + DISP_REG_WDMA_SRC_SIZE, srcHeight << 16 | srcWidth);
	DISP_REG_SET(handle, idx_offst + DISP_REG_WDMA_CLIP_COORD, clipY << 16 | clipX);
	DISP_REG_SET(handle, idx_offst + DISP_REG_WDMA_CLIP_SIZE, clipHeight << 16 | clipWidth);
	DISP_REG_SET_FIELD(handle, CFG_FLD_OUT_FORMAT, idx_offst + DISP_REG_WDMA_CFG, out_fmt_reg);

	if (output_color_space == WDMA_COLOR_SPACE_YUV) {
		yuv444_to_yuv422 = fmt_is_yuv422(out_format);
		/* set DNSP for UYVY and YUV_3P format for better quality */
		DISP_REG_SET_FIELD(handle, CFG_FLD_DNSP_SEL, idx_offst + DISP_REG_WDMA_CFG,
				   yuv444_to_yuv422);
		if (fmt_is_yuv420(out_format)) {
			wdma_config_yuv420(module, out_format, dstPitch, clipHeight, dstAddress,
					   handle);
		}
		/*user internal matrix */
		DISP_REG_SET_FIELD(handle, CFG_FLD_EXT_MTX_EN, idx_offst + DISP_REG_WDMA_CFG, 0);
		DISP_REG_SET_FIELD(handle, CFG_FLD_CT_EN, idx_offst + DISP_REG_WDMA_CFG, 1);
		DISP_REG_SET_FIELD(handle, CFG_FLD_INT_MTX_SEL, idx_offst + DISP_REG_WDMA_CFG,
				   color_matrix);
	} else {
		DISP_REG_SET_FIELD(handle, CFG_FLD_EXT_MTX_EN, idx_offst + DISP_REG_WDMA_CFG, 0);
		DISP_REG_SET_FIELD(handle, CFG_FLD_CT_EN, idx_offst + DISP_REG_WDMA_CFG, 0);
	}
	DISP_REG_SET_FIELD(handle, CFG_FLD_SWAP, idx_offst + DISP_REG_WDMA_CFG, output_swap);
	DISP_REG_SET(handle, idx_offst + DISP_REG_WDMA_DST_ADDR0, dstAddress);
	DISP_REG_SET(handle, idx_offst + DISP_REG_WDMA_DST_W_IN_BYTE, dstPitch);
	DISP_REG_SET_FIELD(handle, ALPHA_FLD_A_SEL, idx_offst + DISP_REG_WDMA_ALPHA,
			   useSpecifiedAlpha);
	DISP_REG_SET_FIELD(handle, ALPHA_FLD_A_VALUE, idx_offst + DISP_REG_WDMA_ALPHA, alpha);

	wdma_calc_ultra(idx, srcWidth, srcHeight, 6, 60, handle);

	return 0;
}

static int wdma_clock_on(DISP_MODULE_ENUM module, void *handle)
{
	unsigned int idx = wdma_index(module);

	DISPDBG("wmda%d_clock_on\n", idx);
#ifdef ENABLE_CLK_MGR
	if (idx == 0)
		enable_clock(MT_CG_DISP0_DISP_WDMA0, "WDMA0");
#endif
	return 0;
}

static int wdma_clock_off(DISP_MODULE_ENUM module, void *handle)
{
	unsigned int idx = wdma_index(module);

	DISPDBG("wdma%d_clock_off\n", idx);
#ifdef ENABLE_CLK_MGR
	if (idx == 0)
		disable_clock(MT_CG_DISP0_DISP_WDMA0, "WDMA0");
#endif
	return 0;
}

void wdma_dump_analysis(DISP_MODULE_ENUM module)
{
	unsigned int index = wdma_index(module);
	unsigned int idx_offst = index * DISP_WDMA_INDEX_OFFSET;

	DISPDMP("==DISP WDMA%d ANALYSIS==\n", index);
	DISPDMP
	    ("wdma%d:en=%d,w=%d,h=%d,clip=(%d,%d,%d,%d),pitch=(W=%d,UV=%d),addr=(0x%x,0x%x,0x%x),fmt=%s\n",
	     index, DISP_REG_GET(DISP_REG_WDMA_EN + idx_offst),
	     DISP_REG_GET(DISP_REG_WDMA_SRC_SIZE + idx_offst) & 0x3fff,
	     (DISP_REG_GET(DISP_REG_WDMA_SRC_SIZE + idx_offst) >> 16) & 0x3fff,
	     DISP_REG_GET(DISP_REG_WDMA_CLIP_COORD + idx_offst) & 0x3fff,
	     (DISP_REG_GET(DISP_REG_WDMA_CLIP_COORD + idx_offst) >> 16) & 0x3fff,
	     DISP_REG_GET(DISP_REG_WDMA_CLIP_SIZE + idx_offst) & 0x3fff,
	     (DISP_REG_GET(DISP_REG_WDMA_CLIP_SIZE + idx_offst) >> 16) & 0x3fff,
	     DISP_REG_GET(DISP_REG_WDMA_DST_W_IN_BYTE + idx_offst),
	     DISP_REG_GET(DISP_REG_WDMA_DST_UV_PITCH + idx_offst),
	     DISP_REG_GET(DISP_REG_WDMA_DST_ADDR0 + idx_offst),
	     DISP_REG_GET(DISP_REG_WDMA_DST_ADDR1 + idx_offst),
	     DISP_REG_GET(DISP_REG_WDMA_DST_ADDR2 + idx_offst),
	     fmt_string(fmt_type
			((DISP_REG_GET(DISP_REG_WDMA_CFG + idx_offst) >> 4) & 0xf,
			 (DISP_REG_GET(DISP_REG_WDMA_CFG + idx_offst) >> 11) & 0x1))
	    );

	DISPDMP("wdma%d:status=%s,in_req=%d,in_ack=%d, exec=%d, input_pixel=(L:%d,P:%d)\n",
		index,
		wdma_get_status(DISP_REG_GET_FIELD
				(FLOW_CTRL_DBG_FLD_WDMA_STA_FLOW_CTRL,
				 DISP_REG_WDMA_FLOW_CTRL_DBG + idx_offst)),
		DISP_REG_GET_FIELD(EXEC_DBG_FLD_WDMA_IN_REQ,
				   DISP_REG_WDMA_FLOW_CTRL_DBG + idx_offst),
		DISP_REG_GET_FIELD(EXEC_DBG_FLD_WDMA_IN_ACK,
				   DISP_REG_WDMA_FLOW_CTRL_DBG + idx_offst),
		DISP_REG_GET(DISP_REG_WDMA_EXEC_DBG + idx_offst) & 0x1f,
		(DISP_REG_GET(DISP_REG_WDMA_CT_DBG + idx_offst) >> 16) & 0xffff,
		DISP_REG_GET(DISP_REG_WDMA_CT_DBG + idx_offst) & 0xffff);
}

void wdma_dump_reg(DISP_MODULE_ENUM module)
{
	unsigned int idx = wdma_index(module);
	unsigned int off_sft = idx * DISP_WDMA_INDEX_OFFSET;

	DISPDMP("==DISP WDMA%d REGS==\n", idx);
	DISPDMP("WDMA:0x000=0x%08x,0x004=0x%08x,0x008=0x%08x,0x00c=0x%08x\n",
		DISP_REG_GET(DISP_REG_WDMA_INTEN + off_sft),
		DISP_REG_GET(DISP_REG_WDMA_INTSTA + off_sft),
		DISP_REG_GET(DISP_REG_WDMA_EN + off_sft),
		DISP_REG_GET(DISP_REG_WDMA_RST + off_sft));

	DISPDMP("WDMA:0x010=0x%08x,0x014=0x%08x,0x018=0x%08x,0x01c=0x%08x\n",
		DISP_REG_GET(DISP_REG_WDMA_SMI_CON + off_sft),
		DISP_REG_GET(DISP_REG_WDMA_CFG + off_sft),
		DISP_REG_GET(DISP_REG_WDMA_SRC_SIZE + off_sft),
		DISP_REG_GET(DISP_REG_WDMA_CLIP_SIZE + off_sft));

	DISPDMP("WDMA:0x020=0x%08x,0x028=0x%08x,0x02c=0x%08x,0x038=0x%08x\n",
		DISP_REG_GET(DISP_REG_WDMA_CLIP_COORD + off_sft),
		DISP_REG_GET(DISP_REG_WDMA_DST_W_IN_BYTE + off_sft),
		DISP_REG_GET(DISP_REG_WDMA_ALPHA + off_sft),
		DISP_REG_GET(DISP_REG_WDMA_BUF_CON1 + off_sft));

	DISPDMP("WDMA:0x03c=0x%08x,0x058=0x%08x,0x05c=0x%08x,0x060=0x%08x\n",
		DISP_REG_GET(DISP_REG_WDMA_BUF_CON2 + off_sft),
		DISP_REG_GET(DISP_REG_WDMA_PRE_ADD0 + off_sft),
		DISP_REG_GET(DISP_REG_WDMA_PRE_ADD2 + off_sft),
		DISP_REG_GET(DISP_REG_WDMA_POST_ADD0 + off_sft));

	DISPDMP("WDMA:0x064=0x%08x,0x078=0x%08x,0x080=0x%08x,0x084=0x%08x\n",
		DISP_REG_GET(DISP_REG_WDMA_POST_ADD2 + off_sft),
		DISP_REG_GET(DISP_REG_WDMA_DST_UV_PITCH + off_sft),
		DISP_REG_GET(DISP_REG_WDMA_DST_ADDR_OFFSET0 + off_sft),
		DISP_REG_GET(DISP_REG_WDMA_DST_ADDR_OFFSET1 + off_sft));

	DISPDMP("WDMA:0x088=0x%08x,0x0a0=0x%08x,0x0a4=0x%08x,0x0a8=0x%08x\n",
		DISP_REG_GET(DISP_REG_WDMA_DST_ADDR_OFFSET2 + off_sft),
		DISP_REG_GET(DISP_REG_WDMA_FLOW_CTRL_DBG + off_sft),
		DISP_REG_GET(DISP_REG_WDMA_EXEC_DBG + off_sft),
		DISP_REG_GET(DISP_REG_WDMA_CT_DBG + off_sft));

	DISPDMP("WDMA:0x0ac=0x%08x,0xf00=0x%08x,0xf04=0x%08x,0xf08=0x%08x,\n",
		DISP_REG_GET(DISP_REG_WDMA_DEBUG + off_sft),
		DISP_REG_GET(DISP_REG_WDMA_DST_ADDR0 + off_sft),
		DISP_REG_GET(DISP_REG_WDMA_DST_ADDR1 + off_sft),
		DISP_REG_GET(DISP_REG_WDMA_DST_ADDR2 + off_sft));
}

static int wdma_dump(DISP_MODULE_ENUM module, int level)
{
	wdma_dump_analysis(module);
	wdma_dump_reg(module);

	return 0;
}

static int wdma_check_input_param(WDMA_CONFIG_STRUCT *config)
{
	int unique = fmt_hw_value(config->outputFormat);

	if (unique > WDMA_OUTPUT_FORMAT_YV12 && unique != WDMA_OUTPUT_FORMAT_NV21) {
		DISPERR("wdma parameter invalidate outfmt 0x%x\n", config->outputFormat);
		return -1;
	}

	if (config->dstAddress == 0 || config->srcWidth == 0 || config->srcHeight == 0) {
		DISPERR("wdma parameter invalidate, addr=0x%lx, w=%d, h=%d\n",
		       config->dstAddress, config->srcWidth, config->srcHeight);
		return -1;
	}
	return 0;
}

static int wdma_config_l(DISP_MODULE_ENUM module, disp_ddp_path_config *pConfig, void *handle)
{

	WDMA_CONFIG_STRUCT *config = &pConfig->wdma_config;

	if (!pConfig->wdma_dirty)
		return 0;

	if (wdma_check_input_param(config) == 0)
		wdma_config(module,
			    config->srcWidth,
			    config->srcHeight,
			    config->clipX,
			    config->clipY,
			    config->clipWidth,
			    config->clipHeight,
			    config->outputFormat,
			    config->dstAddress,
			    config->dstPitch, config->useSpecifiedAlpha, config->alpha, handle);
	return 0;
}

DDP_MODULE_DRIVER ddp_driver_wdma = {
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
};
