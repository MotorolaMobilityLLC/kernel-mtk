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

#ifndef VENC_H264_VPU_H_
#define VENC_H264_VPU_H_

/**
 * enum venc_h264_vpu_work_buf - h264 encoder buffer index
 */
enum venc_h264_vpu_work_buf {
	VENC_H264_VPU_WORK_BUF_RC_INFO,
	VENC_H264_VPU_WORK_BUF_RC_CODE,
	VENC_H264_VPU_WORK_BUF_REC_LUMA,
	VENC_H264_VPU_WORK_BUF_REC_CHROMA,
	VENC_H264_VPU_WORK_BUF_REF_LUMA,
	VENC_H264_VPU_WORK_BUF_REF_CHROMA,
	VENC_H264_VPU_WORK_BUF_MV_INFO_1,
	VENC_H264_VPU_WORK_BUF_MV_INFO_2,
	VENC_H264_VPU_WORK_BUF_SKIP_FRAME,
	VENC_H264_VPU_WORK_BUF_MAX,
};

/**
 * enum venc_h264_bs_mode - for bs_mode argument in h264_enc_vpu_encode
 */
enum venc_h264_bs_mode {
	H264_BS_MODE_SPS,
	H264_BS_MODE_PPS,
	H264_BS_MODE_FRAME,
};

/*
 * struct venc_h264_vpu_config - Structure for h264 encoder configuration
 * @yuv_fmt: input yuv format
 * @bitrate: target bitrate (in bps)
 * @pic_w: picture width
 * @pic_h: picture height
 * @buf_w: buffer width
 * @buf_h: buffer height
 * @intra_period: intra frame period
 * @framerate: frame rate
 * @profile: as specified in standard
 * @level: as specified in standard
 * @wfd: WFD mode 1:on, 0:off
 */
struct venc_h264_vpu_config {
	unsigned int yuv_fmt;
	unsigned int bitrate;
	unsigned int pic_w;
	unsigned int pic_h;
	unsigned int buf_w;
	unsigned int buf_h;
	unsigned int intra_period;
	unsigned int framerate;
	unsigned int profile;
	unsigned int level;
	unsigned int wfd;
};

/*
 * struct venc_h264_vpu_buf - Structure for buffer information
 * @align: buffer alignment (in bytes)
 * @pa: physical address
 * @vpua: VPU side memory addr which is used by RC_CODE
 * @size: buffer size (in bytes)
 */
struct venc_h264_vpu_buf {
	unsigned int align;
	unsigned int pa;
	unsigned int vpua;
	unsigned int size;
};

/*
 * struct venc_h264_vpu_drv - Structure for VPU driver control and info share
 * @config: h264 encoder configuration
 * @wb: working buffer information
 */
struct venc_h264_vpu_drv {
	struct venc_h264_vpu_config config;
	struct venc_h264_vpu_buf wb[VENC_H264_VPU_WORK_BUF_MAX];
};

/*
 * struct venc_h264_vpu_inst - h264 encoder VPU driver instance
 * @wq_hd: wait queue used for vpu cmd trigger then wait vpu interrupt done
 * @signaled: flag used for checking vpu interrupt done
 * @failure: flag to show vpu cmd succ or not
 * @state: enum venc_ipi_msg_enc_state
 * @bs_size: bitstream size for skip frame case usage
 * @wait_int: flag to wait interrupt done (0: for skip frame case, 1: normal case)
 * @id: VPU instance id
 * @drv: driver structure allocated by VPU side for control and info share
 */
struct venc_h264_vpu_inst {
	wait_queue_head_t wq_hd;
	int signaled;
	int failure;
	int state;
	int bs_size;
	int wait_int;
	unsigned int id;
	struct venc_h264_vpu_drv *drv;
};

int h264_enc_vpu_init(void *handle);
int h264_enc_vpu_set_param(void *handle, unsigned int id, void *param);
int h264_enc_vpu_encode(void *handle, unsigned int bs_mode,
			struct venc_frm_buf *frm_buf,
			struct mtk_vcodec_mem *bs_buf,
			unsigned int *bs_size);
int h264_enc_vpu_deinit(void *handle);

#endif
