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
#include "disp_log.h"

#ifdef CONFIG_MTK_CLKMGR
#include <mach/mt_clkmgr.h>
#endif
#include <linux/delay.h>
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

	DISP_REG_SET(handle, idx * DISP_WDMA_INDEX_OFFSET + DISP_REG_WDMA_INTEN, 0x03);
	DISP_REG_SET(handle, idx * DISP_WDMA_INDEX_OFFSET + DISP_REG_WDMA_EN, 0x01);

	return 0;
}

static int wdma_config_yuv420(DISP_MODULE_ENUM module,
			      DpColorFormat fmt,
			      unsigned int dstPitch,
			      unsigned int Height,
			      unsigned long dstAddress, DISP_BUFFER_TYPE sec, void *handle)
{
	unsigned int idx = wdma_index(module);
	unsigned int idx_offst = idx * DISP_WDMA_INDEX_OFFSET;
	unsigned int u_off = 0;
	unsigned int v_off = 0;
	unsigned int u_stride = 0;
	unsigned int y_size = 0;
	unsigned int u_size = 0;
	unsigned int stride = dstPitch;
	int has_v = 1;

	if (fmt == eYV12) {
		y_size = stride * Height;
		u_stride = ALIGN_TO(stride / 2, 16);
		u_size = u_stride * Height / 2;
		u_off = y_size;
		v_off = y_size + u_size;
	} else if (fmt == eYV21) {
		y_size = stride * Height;
		u_stride = ALIGN_TO(stride / 2, 16);
		u_size = u_stride * Height / 2;
		u_off = y_size;
		v_off = y_size + u_size;
	} else if (fmt == eNV12 || fmt == eNV21) {
		y_size = stride * Height;
		u_stride = stride / 2;
		u_size = u_stride * Height / 2;
		u_off = y_size;
		has_v = 0;
	}

	if (sec != DISP_SECURE_BUFFER) {
		DISP_REG_SET(handle, idx_offst + DISP_REG_WDMA_DST_ADDR1, dstAddress + u_off);
		if (has_v)
			DISP_REG_SET(handle, idx_offst + DISP_REG_WDMA_DST_ADDR2,
				     dstAddress + v_off);
	} else {
		int m4u_port;

		m4u_port = M4U_PORT_DISP_WDMA0;

		cmdqRecWriteSecure(handle, disp_addr_convert(idx_offst + DISP_REG_WDMA_DST_ADDR1),
				   CMDQ_SAM_H_2_MVA, dstAddress, u_off, u_size, m4u_port);
		if (has_v)
			cmdqRecWriteSecure(handle,
					   disp_addr_convert(idx_offst + DISP_REG_WDMA_DST_ADDR2),
					   CMDQ_SAM_H_2_MVA, dstAddress, v_off, u_size, m4u_port);
	}
	DISP_REG_SET_FIELD(handle, DST_W_IN_BYTE_FLD_DST_W_IN_BYTE,
			   idx_offst + DISP_REG_WDMA_DST_UV_PITCH, u_stride);
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
	unsigned int pre_ultra_low_time;	/* in unit of 0.5 us */
	unsigned int ultra_low_time;	/* in unit of 0.5 us */
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
	}
	/* consume_levels_per_sec = ((long long unsigned int)width * height * frame_rate * blank_overhead * bpp) /
						rdma_fifo_width / 100;
	 */
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
	/* write ultra_high_ofs, pre_ultra_high_ofs, ultra_low_ofs,
	 * pre_ultra_low_ofs=pre_ultra_low_level into register DISP_WDMA_BUF_CON2
	 */
	/* DISP_REG_SET(handle,idx*DISP_WDMA_INDEX_OFFSET+DISP_REG_WDMA_BUF_CON2, */
	/* ultra_low_level|(ultra_low_ofs<<8)|(pre_ultra_high_ofs<<16)|(ultra_high_ofs<<24)); */

	/* TODO: set ultra/pre-ultra by resolution and format */
	/*
	   YV12: FHD=0x19010ea2, HD=0x0b0106d6, qHD=0x060103e9
	   UYVY: FHD=0x22011283, HD=0x0f0108c8, qHD=0x080104e1
	   RGB:  FHD=0x35011b44, HD=0x17010bad, qHD=0x0c0107d1
	 */
	if (idx == 0 && primary_display_is_decouple_mode() == 0) {
		DISP_REG_SET(handle, idx * DISP_WDMA_INDEX_OFFSET + DISP_REG_WDMA_BUF_CON2,
			     0x1B010E22);
		DISP_REG_SET(handle, idx * DISP_WDMA_INDEX_OFFSET + DISP_REG_WDMA_BUF_CON1,
			     0xD0100080);
	} else {
		DISP_REG_SET(handle, idx * DISP_WDMA_INDEX_OFFSET + DISP_REG_WDMA_BUF_CON2,
			     0x1B010E22);
		DISP_REG_SET(handle, idx * DISP_WDMA_INDEX_OFFSET + DISP_REG_WDMA_BUF_CON1,
			     0x50100080);
	}


	DISPDBG("pre_ultra_low_level  = 0x%03x = %d\n", pre_ultra_low_level, pre_ultra_low_level);
	DISPDBG("ultra_low_level      = 0x%03x = %d\n", ultra_low_level, ultra_low_level);
	DISPDBG("ultra_high_level     = 0x%03x = %d\n", ultra_high_level, ultra_high_level);
	DISPDBG("ultra_low_ofs        = 0x%03x = %d\n", ultra_low_ofs, ultra_low_ofs);
	DISPDBG("pre_ultra_high_ofs   = 0x%03x = %d\n", pre_ultra_high_ofs, pre_ultra_high_ofs);
	DISPDBG("ultra_high_ofs       = 0x%03x = %d\n", ultra_high_ofs, ultra_high_ofs);
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
		       unsigned int useSpecifiedAlpha,
		       unsigned char alpha, DISP_BUFFER_TYPE sec, void *handle)
{
	unsigned int idx = wdma_index(module);
	unsigned int output_swap = fmt_swap(out_format);
	unsigned int output_color_space = fmt_color_space(out_format);
	unsigned int out_fmt_reg = fmt_hw_value(out_format);
	unsigned int yuv444_to_yuv422 = 0;
	int color_matrix = 0x2;	/* 0010 RGB_TO_BT601 */
	unsigned int idx_offst = idx * DISP_WDMA_INDEX_OFFSET;
	size_t size = dstPitch * clipHeight;

#if defined(CONFIG_TRUSTONIC_TEE_SUPPORT) && defined(CONFIG_MTK_SEC_VIDEO_PATH_SUPPORT)
	DISPMSG("module %s, src(w=%d,h=%d), clip(x=%d,y=%d,w=%d,h=%d),out_fmt=%s,dst_address=0x%lx,dst_p=%d,\n",
		ddp_get_module_name(module), srcWidth, srcHeight, clipX, clipY, clipWidth, clipHeight,
		fmt_string(out_format), dstAddress, dstPitch);
	DISPMSG("spific_alfa=%d,alpa=%d,handle=0x%p,sec%d\n", useSpecifiedAlpha, alpha, handle, sec);
#endif
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
					   sec, handle);
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
	if (sec != DISP_SECURE_BUFFER) {
		DISP_REG_SET(handle, idx_offst + DISP_REG_WDMA_DST_ADDR0, dstAddress);
	} else {
		int m4u_port;

		m4u_port = M4U_PORT_DISP_WDMA0;

		/* for sec layer, addr variable stores sec handle */
		/* we need to pass this handle and offset to cmdq driver */
		/* cmdq sec driver will help to convert handle to correct address */
		cmdqRecWriteSecure(handle, disp_addr_convert(idx_offst + DISP_REG_WDMA_DST_ADDR0),
				   CMDQ_SAM_H_2_MVA, dstAddress, 0, size, m4u_port);
	}
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
	/* DISPMSG("wmda%d_clock_on\n",idx); */
#ifdef ENABLE_CLK_MGR
	if (idx == 0) {
#ifdef CONFIG_MTK_CLKMGR
		enable_clock(MT_CG_DISP0_DISP_WDMA0, "WDMA0");
#else
		ddp_clk_enable(DISP0_DISP_WDMA0);
#endif
	}
#endif
	return 0;
}

static int wdma_clock_off(DISP_MODULE_ENUM module, void *handle)
{
	unsigned int idx = wdma_index(module);
	/* DISPMSG("wdma%d_clock_off\n",idx); */
#ifdef ENABLE_CLK_MGR
	if (idx == 0) {
#ifdef CONFIG_MTK_CLKMGR
		disable_clock(MT_CG_DISP0_DISP_WDMA0, "WDMA0");
#else
		ddp_clk_disable(DISP0_DISP_WDMA0);
#endif
	}
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

static int wdma_is_sec[2];

static int wdma_config_l(DISP_MODULE_ENUM module, disp_ddp_path_config *pConfig, void *handle)
{

	WDMA_CONFIG_STRUCT *config = &pConfig->wdma_config;
	int wdma_idx = wdma_index(module);
	CMDQ_ENG_ENUM cmdq_engine;

	if (!pConfig->wdma_dirty)
		return 0;

	/* warm reset wdma every time we use it */
	if (handle) {
		if (/*(primary_display_is_video_mode()==0 && primary_display_is_decouple_mode()==0) || */
			   primary_display_is_decouple_mode() == 1) {
			unsigned int idx_offst = wdma_idx * DISP_WDMA_INDEX_OFFSET;

			DISP_REG_SET(handle, idx_offst + DISP_REG_WDMA_RST, 0x01);	/* trigger soft reset */
			cmdqRecPoll(handle,
				    disp_addr_convert(idx_offst + DISP_REG_WDMA_FLOW_CTRL_DBG), 1,
				    0x1);
			DISP_REG_SET(handle, idx_offst + DISP_REG_WDMA_RST, 0x0);	/* trigger soft reset */
		}
	}

	cmdq_engine = wdma_idx == 0 ? CMDQ_ENG_DISP_WDMA0 : CMDQ_ENG_DISP_WDMA1;

	if (config->security == DISP_SECURE_BUFFER) {
		cmdqRecSetSecure(handle, 1);

		/* set engine as sec */
		cmdqRecSecureEnablePortSecurity(handle, (1LL << cmdq_engine));
		cmdqRecSecureEnableDAPC(handle, (1LL << cmdq_engine));
		if (wdma_is_sec[wdma_idx] == 0)
			DISPMSG("[SVP] switch wdma%d to sec\n", wdma_idx);
		wdma_is_sec[wdma_idx] = 1;
	} else {
		if (wdma_is_sec[wdma_idx] && !primary_display_is_ovl1to2_handle(handle)) {
			/* wdma is in sec stat, we need to switch it to nonsec */
			cmdqRecHandle nonsec_switch_handle;
			int ret;

			ret =
			    cmdqRecCreate(CMDQ_SCENARIO_DISP_PRIMARY_DISABLE_SECURE_PATH,
					  &(nonsec_switch_handle));
			if (ret)
				DISPAEE("[SVP]fail to create disable handle %s ret=%d\n",
				       __func__, ret);

			cmdqRecReset(nonsec_switch_handle);
			_cmdq_insert_wait_frame_done_token_mira(nonsec_switch_handle);
			cmdqRecSetSecure(nonsec_switch_handle, 1);

			/*in fact, dapc/port_sec will be disabled by cmdq */
			cmdqRecSecureEnablePortSecurity(nonsec_switch_handle, (1LL << cmdq_engine));
			cmdqRecSecureEnableDAPC(nonsec_switch_handle, (1LL << cmdq_engine));
			cmdqRecFlush(nonsec_switch_handle);
			cmdqRecDestroy(nonsec_switch_handle);
			DISPMSG("[SVP] switch wdma%d to nonsec\n", wdma_idx);
		}
		wdma_is_sec[wdma_idx] = 0;
	}

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
			    config->dstPitch,
			    config->useSpecifiedAlpha, config->alpha, config->security, handle);
	return 0;
}

int wdma_build_cmdq(DISP_MODULE_ENUM module, void *cmdq_trigger_handle, CMDQ_STATE state)
{

	if (cmdq_trigger_handle == NULL) {
		DISPERR("cmdq_trigger_handle is NULL\n");
		return -1;
	}

	return 0;
}

/* wdma */

DDP_MODULE_DRIVER ddp_driver_wdma = {
	.module = DISP_MODULE_WDMA0,
#ifdef WDMA_PATH_CLOCK_DYNAMIC_SWITCH
	.init = NULL,
	.deinit = NULL,
#else
	.init = wdma_clock_on,
	.deinit = wdma_clock_off,
#endif
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
