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

#include "mtk_vpu_core.h"

#include "venc_h264_if.h"
#include "venc_h264_vpu.h"
#include "venc_ipi_msg.h"

static unsigned int h264_get_profile(unsigned int profile)
{
	/* (Baseline=66, Main=77, High=100) */
	switch (profile) {
	case V4L2_MPEG_VIDEO_H264_PROFILE_BASELINE:
		return 66;
	case V4L2_MPEG_VIDEO_H264_PROFILE_MAIN:
		return 77;
	case V4L2_MPEG_VIDEO_H264_PROFILE_HIGH:
		return 100;
	default:
		return 100;
	}
}

static unsigned int h264_get_level(unsigned int level)
{
	/* (UpTo4.1(HighProfile)) */
	switch (level) {
	case V4L2_MPEG_VIDEO_H264_LEVEL_1_0:
		return 10;
	case V4L2_MPEG_VIDEO_H264_LEVEL_1_1:
		return 11;
	case V4L2_MPEG_VIDEO_H264_LEVEL_1_2:
		return 12;
	case V4L2_MPEG_VIDEO_H264_LEVEL_1_3:
		return 13;
	case V4L2_MPEG_VIDEO_H264_LEVEL_2_0:
		return 20;
	case V4L2_MPEG_VIDEO_H264_LEVEL_2_1:
		return 21;
	case V4L2_MPEG_VIDEO_H264_LEVEL_2_2:
		return 22;
	case V4L2_MPEG_VIDEO_H264_LEVEL_3_0:
		return 30;
	case V4L2_MPEG_VIDEO_H264_LEVEL_3_1:
		return 31;
	case V4L2_MPEG_VIDEO_H264_LEVEL_3_2:
		return 32;
	case V4L2_MPEG_VIDEO_H264_LEVEL_4_0:
		return 40;
	case V4L2_MPEG_VIDEO_H264_LEVEL_4_1:
		return 41;
	default:
		return 31;
	}
}

static void handle_h264_enc_init_msg(struct venc_h264_handle *hndl, void *data)
{
	struct venc_vpu_ipi_msg_init *msg = data;

	hndl->vpu_inst.id = msg->inst_id;
	hndl->vpu_inst.drv = (struct venc_h264_vpu_drv *)vpu_mapping_dm_addr(
		hndl->dev, (uintptr_t *)(unsigned long)msg->inst_id);
}

static void handle_h264_enc_encode_msg(struct venc_h264_handle *hndl,
				       void *data)
{
	struct venc_vpu_ipi_msg_enc *msg = data;

	hndl->vpu_inst.state = msg->state;
	hndl->vpu_inst.bs_size = msg->bs_size;
	hndl->is_key_frm = msg->key_frame;
}

static void h264_enc_vpu_ipi_handler(void *data, unsigned int len, void *priv)
{
	struct venc_vpu_ipi_msg_common *msg = data;
	struct venc_h264_handle *hndl = (struct venc_h264_handle *)(uintptr_t)msg->venc_inst;

	mtk_vcodec_debug_enter(hndl);

	mtk_vcodec_debug(hndl, "msg_id %x hndl %p status %d",
			 msg->msg_id, hndl, msg->status);

	switch (msg->msg_id) {
	case VPU_IPIMSG_H264_ENC_INIT_DONE:
		handle_h264_enc_init_msg(hndl, data);
		break;
	case VPU_IPIMSG_H264_ENC_SET_PARAM_DONE:
		break;
	case VPU_IPIMSG_H264_ENC_ENCODE_DONE:
		handle_h264_enc_encode_msg(hndl, data);
		break;
	case VPU_IPIMSG_H264_ENC_DEINIT_DONE:
		break;
	default:
		mtk_vcodec_err(hndl, "unknown msg id %x", msg->msg_id);
		break;
	}

	hndl->vpu_inst.signaled = 1;
	hndl->vpu_inst.failure = (msg->status != VENC_IPI_MSG_STATUS_OK);
	wake_up_interruptible(&hndl->vpu_inst.wq_hd);

	mtk_vcodec_debug_leave(hndl);
}

static int h264_enc_vpu_wait_ack(struct venc_h264_handle *hndl,
				 unsigned int timeout_ms)
{
	int ret;

	mtk_vcodec_debug_enter(hndl);

	ret = wait_event_interruptible_timeout(hndl->vpu_inst.wq_hd,
					       hndl->vpu_inst.signaled == 1,
					       msecs_to_jiffies(timeout_ms));
	if (0 == ret) {
		mtk_vcodec_err(hndl, "wait vpu ack time out !");
		return -EINVAL;
	}
	if (-ERESTARTSYS == ret) {
		mtk_vcodec_err(hndl,
			       "wait vpu ack interrupted by a signal");
		return -EINVAL;
	}

	hndl->vpu_inst.signaled = 0;

	mtk_vcodec_debug_leave(hndl);
	return 0;
}

static int h264_enc_vpu_send_msg(struct venc_h264_handle *hndl, void *msg,
				 int len, int wait_ack)
{
	int status;

	mtk_vcodec_debug_enter(hndl);

	status = vpu_ipi_send(hndl->dev, IPI_VENC_H264, msg, len, 1);
	if (status) {
		mtk_vcodec_err(hndl, "vpu_ipi_send msg %x len %d fail %d",
			       *(unsigned int *)msg, len, status);
		return -EINVAL;
	}
	mtk_vcodec_debug(hndl, "vpu_ipi_send msg %x success",
			 *(unsigned int *)msg);

	if (wait_ack && h264_enc_vpu_wait_ack(hndl, 2000)) {
		mtk_vcodec_err(hndl, "vp8_enc_vpu_wait_ack failed");
		return -EINVAL;
	}

	mtk_vcodec_debug_leave(hndl);
	return 0;
}

int h264_enc_vpu_init(void *handle)
{
	int status;
	struct venc_h264_handle *hndl = handle;
	struct venc_ap_ipi_msg_init out;

	mtk_vcodec_debug_enter(hndl);

	init_waitqueue_head(&hndl->vpu_inst.wq_hd);
	hndl->vpu_inst.signaled = 0;
	hndl->vpu_inst.failure = 0;

	status = vpu_ipi_registration(hndl->dev, IPI_VENC_H264,
				      h264_enc_vpu_ipi_handler,
				      "h264_enc", NULL);
	if (status) {
		mtk_vcodec_err(hndl, "vpu_ipi_registration fail %d",
			       status);
		return -EINVAL;
	}
	mtk_vcodec_debug(hndl, "vpu_ipi_registration success");

	out.msg_id = AP_IPIMSG_H264_ENC_INIT;
	out.venc_inst = (unsigned long)hndl;
	if (h264_enc_vpu_send_msg(hndl, &out, sizeof(out), 1) ||
	    hndl->vpu_inst.failure) {
		mtk_vcodec_err(hndl, "AP_IPIMSG_H264_ENC_INIT failed");
		return -EINVAL;
	}

	mtk_vcodec_debug_leave(hndl);

	return 0;
}

int h264_enc_vpu_set_param(void *handle, unsigned int id, void *param)
{
	struct venc_h264_handle *hndl = handle;
	struct venc_ap_ipi_msg_set_param out;

	mtk_vcodec_debug(hndl, "id %d ->", id);

	out.msg_id = AP_IPIMSG_H264_ENC_SET_PARAM;
	out.inst_id = hndl->vpu_inst.id;
	out.param_id = id;
	switch (id) {
	case VENC_SET_PARAM_ENC: {
		struct venc_enc_prm *enc_param = param;

		hndl->vpu_inst.drv->config.yuv_fmt = enc_param->yuv_fmt;
		hndl->vpu_inst.drv->config.bitrate = enc_param->bitrate;
		hndl->vpu_inst.drv->config.pic_w = enc_param->width;
		hndl->vpu_inst.drv->config.pic_h = enc_param->height;
		hndl->vpu_inst.drv->config.buf_w = enc_param->buf_width;
		hndl->vpu_inst.drv->config.buf_h = enc_param->buf_height;
		hndl->vpu_inst.drv->config.intra_period =
			enc_param->intra_period;
		hndl->vpu_inst.drv->config.framerate = enc_param->frm_rate;
		hndl->vpu_inst.drv->config.profile =
			h264_get_profile(enc_param->h264_profile);
		hndl->vpu_inst.drv->config.level =
			h264_get_level(enc_param->h264_level);
		hndl->vpu_inst.drv->config.wfd = 0;
		out.data_item = 0;
		break;
	}
	case VENC_SET_PARAM_FORCE_INTRA:
		out.data_item = 0;
		break;
	case VENC_SET_PARAM_ADJUST_BITRATE:
		out.data_item = 1;
		out.data[0] = *(unsigned int *)param;
		break;
	case VENC_SET_PARAM_ADJUST_FRAMERATE:
		out.data_item = 1;
		out.data[0] = *(unsigned int *)param;
		break;
	case VENC_SET_PARAM_I_FRAME_INTERVAL:
		out.data_item = 1;
		out.data[0] = *(unsigned int *)param;
		break;
	case VENC_SET_PARAM_SKIP_FRAME:
		out.data_item = 0;
		break;
	}
	if (h264_enc_vpu_send_msg(hndl, &out, sizeof(out), 1) ||
	    hndl->vpu_inst.failure) {
		mtk_vcodec_err(hndl,
			       "AP_IPIMSG_H264_ENC_SET_PARAM %d fail", id);
		return -EINVAL;
	}

	mtk_vcodec_debug(hndl, "id %d <-", id);

	return 0;
}

int h264_enc_vpu_encode(void *handle, unsigned int bs_mode,
			struct venc_frm_buf *frm_buf,
			struct mtk_vcodec_mem *bs_buf,
			unsigned int *bs_size)
{
	struct venc_h264_handle *hndl = handle;
	struct venc_ap_ipi_msg_enc out;

	mtk_vcodec_debug(hndl, "bs_mode %d ->", bs_mode);

	out.msg_id = AP_IPIMSG_H264_ENC_ENCODE;
	out.inst_id = hndl->vpu_inst.id;
	out.bs_mode = bs_mode;
	if (frm_buf) {
		out.input_addr[0] = frm_buf->fb_addr.dma_addr;
		out.input_addr[1] = frm_buf->fb_addr1.dma_addr;
		out.input_addr[2] = frm_buf->fb_addr2.dma_addr;
	} else {
		out.input_addr[0] = 0;
		out.input_addr[1] = 0;
		out.input_addr[2] = 0;
	}
	if (bs_buf) {
		out.bs_addr = bs_buf->dma_addr;
		out.bs_size = bs_buf->size;
	} else {
		out.bs_addr = 0;
		out.bs_size = 0;
	}
	if (h264_enc_vpu_send_msg(hndl, &out, sizeof(out), 1) ||
	    hndl->vpu_inst.failure) {
		mtk_vcodec_err(hndl, "AP_IPIMSG_H264_ENC_ENCODE %d fail",
			       bs_mode);
		return -EINVAL;
	}

	mtk_vcodec_debug(hndl, "state %d size %d key_frm %d",
			 hndl->vpu_inst.state, hndl->vpu_inst.bs_size,
			 hndl->is_key_frm);
	hndl->vpu_inst.wait_int = 1;
	if (hndl->vpu_inst.state == VEN_IPI_MSG_ENC_STATE_SKIP) {
		*bs_size = hndl->vpu_inst.bs_size;
		memcpy(bs_buf->va,
		       hndl->work_buf[VENC_H264_VPU_WORK_BUF_SKIP_FRAME].va,
		       *bs_size);
		hndl->vpu_inst.wait_int = 0;
	}

	mtk_vcodec_debug(hndl, "bs_mode %d ->", bs_mode);

	return 0;
}

int h264_enc_vpu_deinit(void *handle)
{
	struct venc_h264_handle *hndl = handle;
	struct venc_ap_ipi_msg_deinit out;

	mtk_vcodec_debug_enter(hndl);

	out.msg_id = AP_IPIMSG_H264_ENC_DEINIT;
	out.inst_id = hndl->vpu_inst.id;
	if (h264_enc_vpu_send_msg(hndl, &out, sizeof(out), 1) ||
	    hndl->vpu_inst.failure) {
		mtk_vcodec_err(hndl, "AP_IPIMSG_H264_ENC_DEINIT fail");
		return -EINVAL;
	}

	mtk_vcodec_debug_leave(hndl);

	return 0;
}
