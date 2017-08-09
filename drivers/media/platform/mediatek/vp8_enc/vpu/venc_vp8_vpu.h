/*
 * Copyright (c) 2015 MediaTek Inc.
 * Author: Daniel Hsiao <daniel.hsiao@mediatek.com>
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

#ifndef VENC_VP8_VPU_H_
#define VENC_VP8_VPU_H_

/**
 * enum venc_vp8_vpu_work_buf - vp8 encoder buffer index
 */
enum venc_vp8_vpu_work_buf {
	VENC_VP8_VPU_WORK_BUF_LUMA,
	VENC_VP8_VPU_WORK_BUF_LUMA2,
	VENC_VP8_VPU_WORK_BUF_LUMA3,
	VENC_VP8_VPU_WORK_BUF_CHROMA,
	VENC_VP8_VPU_WORK_BUF_CHROMA2,
	VENC_VP8_VPU_WORK_BUF_CHROMA3,
	VENC_VP8_VPU_WORK_BUF_MV_INFO,
	VENC_VP8_VPU_WORK_BUF_BS_HD,
	VENC_VP8_VPU_WORK_BUF_PROB_BUF,
	VENC_VP8_VPU_WORK_BUF_RC_INFO,
	VENC_VP8_VPU_WORK_BUF_RC_CODE,
	VENC_VP8_VPU_WORK_BUF_RC_CODE2,
	VENC_VP8_VPU_WORK_BUF_RC_CODE3,
	VENC_VP8_VPU_WORK_BUF_MAX,
};

/*
 * struct venc_vp8_vpu_config - Structure for vp8 encoder configuration
 * @yuv_fmt: input yuv format
 * @bitrate: target bitrate (in bps)
 * @pic_w: picture width
 * @pic_h: picture height
 * @buf_w: buffer width (with 16 alignment)
 * @buf_h: buffer height (with 16 alignment)
 * @intra_period: intra frame period
 * @framerate: frame rate
 * @ts_mode: temporal scalability mode (0: disable, 1: enable)
 *	     support three temporal layers - 0: 7.5fps 1: 7.5fps 2: 15fps.
 */
struct venc_vp8_vpu_config {
	unsigned int yuv_fmt;
	unsigned int bitrate;
	unsigned int pic_w;
	unsigned int pic_h;
	unsigned int buf_w;
	unsigned int buf_h;
	unsigned int intra_period;
	unsigned int framerate;
	unsigned int ts_mode;
};

/*
 * struct venc_vp8_vpu_buf -Structure for buffer information
 * @align: buffer alignment (in bytes)
 * @pa: physical address
 * @vpua: VPU side memory addr which is used by RC_CODE
 * @size: buffer size (in bytes)
 */
struct venc_vp8_vpu_buf {
	unsigned int align;
	unsigned int pa;
	unsigned int vpua;
	unsigned int size;
};

/*
 * struct venc_vp8_vpu_drv - Structure for VPU driver control and info share
 * @config: vp8 encoder configuration
 * @wb: working buffer information
 */
struct venc_vp8_vpu_drv {
	struct venc_vp8_vpu_config config;
	struct venc_vp8_vpu_buf wb[VENC_VP8_VPU_WORK_BUF_MAX];
};

/*
 * struct venc_vp8_vpu_inst - vp8 encoder VPU driver instance
 * @wq_hd: wait queue used for vpu cmd trigger then wait vpu interrupt done
 * @signaled: flag used for checking vpu interrupt done
 * @failure: flag to show vpu cmd succ or not
 * @id: VPU instance id
 * @drv: driver structure allocated by VPU side for control and info share
 */
struct venc_vp8_vpu_inst {
	wait_queue_head_t wq_hd;
	int signaled;
	int failure;
	unsigned int id;
	struct venc_vp8_vpu_drv *drv;
};

int vp8_enc_vpu_init(void *handle);
int vp8_enc_vpu_set_param(void *handle, unsigned int id, void *param);
int vp8_enc_vpu_encode(void *handle,
		       struct venc_frm_buf *frm_buf,
		       struct mtk_vcodec_mem *bs_buf);
int vp8_enc_vpu_deinit(void *handle);

#endif
