/*
 * Copyright (c) 2015 MediaTek Inc.
 * Author: Jungchang Tsao <jungchang.tsao@mediatek.com>
 *	   PC Chen <pc.chen@mediatek.com>
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

#ifndef _VDEC_VP8_IF_H_
#define _VDEC_VP8_IF_H_

#include "vdec_drv_base.h"

#define VP8_MAX_FRM_BUFF_NUM				5

/**
 * struct vdec_vp8_dec_info - decode misc information
 * @vp_wrapper_dma    : wrapper buffer dma
 * @prev_y_dma        : previous decoded frame buffer Y plane address
 * @cur_y_fb_dma      : current plane Y frame buffer dma
 * @cur_c_fb_dma      : current plane C frame buffer dma
 * @bs_dma	      : bitstream dma
 * @bs_sz	      : current plane C frame buffer dma
 * @resolution_changed: resolution change flag
 * @show_frame	      : display this frame or not
 */
struct vdec_vp8_dec_info {
	uint64_t vp_wrapper_dma;
	uint64_t prev_y_dma;
#ifdef Y_C_SEPARATE
	uint64_t cur_y_fb_dma;
	uint64_t cur_c_fb_dma;
#else
	uint64_t cur_fb_dma;
#endif
	uint64_t bs_dma;
	uint32_t bs_sz;
	uint32_t resolution_changed;
	uint32_t show_frame;
	uint32_t reserved;
};

/**
 * struct vdec_vp8_vsi - VPU shared information
 * @dec    : decoding information
 * @pic    : picture information
 */
struct vdec_vp8_vsi {
	struct vdec_vp8_dec_info dec;
	struct vdec_pic_info pic;
};

/**
 * struct vdec_vp8_vpu_inst - VPU instance for VP8 decode
 * @wq_hd	: Wait queue to wait VPU message ack
 * @signaled	: 1 - Host has received ack message from VPU, 0 - not recevie
 * @failure	: VPU execution result status
 * @h_drv	: handle to VPU driver
 */
struct vdec_vp8_vpu_inst {
	wait_queue_head_t wq_hd;
	int signaled;
	int failure;
	unsigned int h_drv;
};

/* frame buffer (fb) list
 * [dec_fb_list]   - decode fb are initialized to 0 and populated in
 * [dec_use_list]  - fb is set after decode and is moved to this list
 * [dec_free_list] - fb is not needed for reference will be moved from
 *		     [dec_use_list] to [dec_free_list] and
 *		     once user remove fb from [dec_free_list],
 *		     it is circulated back to [dec_fb_list]
 * [disp_fb_list]  - display fb are initialized to 0 and populated in
 * [disp_rdy_list] - fb is set after decode and is moved to this list
 *                   once user remove fb from [disp_rdy_list] it is
 *                   circulated back to [disp_fb_list]
 */

/**
 * struct vdec_vp8_inst - h264 decoder instance
 * @cur_fb		: current frame buffer
 * @dec_fb		: decode frame buffer node
 * @disp_fb		: display frame buffer node
 * @dec_fb_list		: list to store decode frame buffer
 * @dec_use_list	: list to store frame buffer in use
 * @dec_free_list	: list to store free frame buffer
 * @disp_fb_list	: list to store display frame buffer
 * @disp_rdy_list	: list to store display ready frame buffer
 * @vp_wrapper_buf	: decoder working buffer
 * @frm_cnt		: decode frame count
 * @ctx			: V4L2 context
 * @dev			: platform device
 * @vsi			: VPU share information
 * @vpu			: VPU instance for decoder
 */
struct vdec_vp8_inst {
	struct vdec_fb *cur_fb;
	struct vdec_fb_node dec_fb[VP8_MAX_FRM_BUFF_NUM];
	struct vdec_fb_node disp_fb[VP8_MAX_FRM_BUFF_NUM];
	struct list_head dec_fb_list;
	struct list_head dec_use_list;
	struct list_head dec_free_list;
	struct list_head disp_fb_list;
	struct list_head disp_rdy_list;
	struct mtk_vcodec_mem vp_wrapper_buf;
	unsigned int frm_cnt;
	void *ctx;
	struct platform_device *dev;
	struct vdec_vp8_vsi *vsi;
	struct vdec_vp8_vpu_inst vpu;
#ifdef DBG_VDEC_VP8
	unsigned int disp_total;
#endif
};

struct vdec_common_if *get_vp8_dec_comm_if(void);

#endif
