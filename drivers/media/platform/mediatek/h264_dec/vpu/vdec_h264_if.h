/*
 * Copyright (c) 2015 MediaTek Inc.
 * Author: PC Chen <pc.chen@mediatek.com>
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

#ifndef _VDEC_H264_IF_H_
#define _VDEC_H264_IF_H_

#include "mtk_vcodec_util.h"

#include "vdec_drv_base.h"
#include "vdec_h264_vpu.h"

#ifdef DBG_VDEC_H264
#include "vdec_h264_debug.h"
#endif

#define H264_MAX_FB_NUM				17
#define NAL_TYPE(value)				((value) & 0x1F)

/**
 * struct h264_fb - h264 decode frame buffer information
 * @vdec_fb_va	: virtual address of struct vdec_fb
 * @fb_dma	: dma address of frame buffer
 * @poc		: picture order count of frame buffer
 * @reserved	: for 8 bytes alignment
 */
#ifdef Y_C_SEPARATE
struct h264_fb {
	uint64_t vdec_fb_va;
	uint64_t y_fb_dma;
	uint64_t c_fb_dma;
	int32_t poc;
	uint32_t reserved;
};
#else
struct h264_fb {
	uint64_t vdec_fb_va;
	uint64_t fb_dma;
	int32_t poc;
	uint32_t reserved;
};
#endif

/**
 * struct h264_ring_fb_list - ring frame buffer list
 * @fb_list   : frame buffer arrary
 * @read_idx  : read index
 * @write_idx : write index
 * @count     : buffer count in list
 */
struct h264_ring_fb_list {
	struct h264_fb fb_list[H264_MAX_FB_NUM];
	unsigned int read_idx;
	unsigned int write_idx;
	unsigned int count;
	unsigned int reserved;
};

/**
 * struct vdec_vh264_dec_info - decode information
 * @dpb_sz		: decoding picture buffer size
 * @resolution_changed  : resoltion change happen
 * @realloc_mv_buf	: flag to notify driver to re-allocate mv buffer
 * @reserved		: for 8 bytes alignment
 */
struct vdec_vh264_dec_info {
	uint32_t dpb_sz;
	uint32_t resolution_changed;
	uint32_t realloc_mv_buf;
	uint32_t reserved;
};

/**
 * struct vdec_h264_vsi - shared memory for decode information exchange
 *                        between VPU and Host.
 *                        The memory is allocated by VPU and mapping to Host
 *                        in vpu_dec_init()
 * @ppl_buf_dma : HW working buffer ppl dma address
 * @mv_buf_dma  : HW working buffer mv dma address
 * @list_free   : free frame buffer ring list
 * @list_disp   : display frame buffer ring list
 * @dec		: decode information
 * @pic		: picture information
 * @crop        : crop information
 */
struct vdec_h264_vsi {
	uint64_t ppl_buf_dma;
	uint64_t mv_buf_dma[H264_MAX_FB_NUM];
	struct h264_ring_fb_list list_free;
	struct h264_ring_fb_list list_disp;
	struct vdec_vh264_dec_info dec;
	struct vdec_pic_info pic;
	struct v4l2_rect crop;
};

/**
 * struct vdec_h264_inst - h264 decoder instance
 * @ctx_id   : mtk_vcodec_ctx context id
 * @ctx      : point to mtk_vcodec_ctx
 * @num_nalu : how many nalus be decoded
 * @ppl_buf  : HW working buffer for ppl
 * @mv_buf   : HW working buffer for mv
 * @vsi      : VPU shared information
 * @vpu      : VPU instance
 */
struct vdec_h264_inst {
	int ctx_id;
	void *ctx;
	unsigned int num_nalu;
	struct platform_device *dev;
	struct mtk_vcodec_mem ppl_buf;
	struct mtk_vcodec_mem mv_buf[H264_MAX_FB_NUM];
	struct vdec_h264_vsi *vsi;
	struct vdec_h264_vpu_inst vpu;
};

struct vdec_common_if *get_h264_dec_comm_if(void);

#endif
