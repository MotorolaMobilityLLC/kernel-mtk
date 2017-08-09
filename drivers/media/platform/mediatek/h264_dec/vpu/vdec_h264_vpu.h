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

#ifndef _VDEC_H264_VPU_H_
#define _VDEC_H264_VPU_H_

/**
 * struct vdec_h264_vpu_inst - VPU instance for H264 decode
 * @hdr_bs_buf  : Header bit-stream buffer
 * @h_drv	: handle to VPU driver
 * @signaled    : 1 - Host has received ack message from VPU, 0 - not recevie
 * @failure     : VPU execution result status
 * @wq          : Wait queue to wait VPU message ack
 */
struct vdec_h264_vpu_inst {
	unsigned char *hdr_bs_buf;
	unsigned int h_drv;
	unsigned int signaled;
	int failure;
	wait_queue_head_t wq;
};

/*
 * Note these functions are not thread-safe for the same decoder instance.
 * the reason is |signaled|. In h264_dec_vpu_wait_ack,
 * wait_event_interruptible_timeout waits |signaled| to be 1.
 * Suppose wait_event_interruptible_timeout returns and the execution has not
 * reached line 127. If another thread calls vdec_h264_vpu_dec_end,
 * |signaled| will be 1 and wait_event_interruptible_timeout will return
 *  immediately. We enusure thread-safe to add mtk_vdec_lock()/unlock() in
 * vdec_drv_if.c
 *
 */
int vdec_h264_vpu_init(void *vdec_inst, uint64_t bs_dma, uint32_t bs_sz);
#ifdef Y_C_SEPARATE
int vdec_h264_vpu_dec_start(void *vdec_inst, uint32_t bs_sz,
			    uint32_t nal_start, uint64_t bs_dma,
			    uint64_t y_fb_dma, uint64_t c_fb_dma,
			    uint64_t vdec_fb_va);
#else
int vdec_h264_vpu_dec_start(void *vdec_inst, uint32_t bs_sz,
			    uint32_t nal_start, uint64_t bs_dma,
			    uint64_t fb_dma, uint64_t vdec_fb_va);
#endif
int vdec_h264_vpu_dec_end(void *vdec_inst);
int vdec_h264_vpu_deinit(void *vdec_inst);
int vdec_h264_vpu_reset(void *vdec_inst);

#endif
