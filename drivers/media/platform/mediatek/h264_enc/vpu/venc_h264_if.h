/*
 * Copyright (c) 2015 MediaTek Inc.
 * Author: Jungchang Tsao <jungchang.tsao@mediatek.com>
 *         Daniel Hsiao <daniel.hsiao@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _VENC_H264_IF_H_
#define _VENC_H264_IF_H_

#include "venc_drv_base.h"
#include "venc_h264_vpu.h"

/*
 * struct venc_h264_handle - h264 encoder AP driver handle
 * @hw_base: h264 encoder hardware register base
 * @work_buf: working buffer
 * @pps_buf: buffer to store the pps bitstream
 * @work_buf_alloc: working buffer allocated flag
 * @frm_cnt: encoded frame count
 * @prepend_hdr: when the v4l2 layer send VENC_SET_PARAM_PREPEND_HEADER cmd
 *  through h264_enc_set_param interface, it will set this flag and prepend the
 *  sps/pps in h264_enc_encode function.
 * @is_key_frm: key frame flag
 * @vpu_inst: VPU instance to exchange information between AP and VPU
 * @ctx: context for v4l2 layer integration
 * @dev: device for v4l2 layer integration
 */
struct venc_h264_handle {
	void __iomem *hw_base;
	struct mtk_vcodec_mem work_buf[VENC_H264_VPU_WORK_BUF_MAX];
	struct mtk_vcodec_mem pps_buf;
	unsigned int work_buf_alloc;
	unsigned int frm_cnt;
	unsigned int prepend_hdr;
	unsigned int is_key_frm;
	struct venc_h264_vpu_inst vpu_inst;
	void *ctx;
	struct platform_device *dev;
};

struct venc_common_if *get_h264_enc_comm_if(void);

#endif
