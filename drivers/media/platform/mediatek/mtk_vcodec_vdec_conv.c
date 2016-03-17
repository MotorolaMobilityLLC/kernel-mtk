/*
 * Copyright (c) 2015 MediaTek Inc.
 * Author: Ming Hsiu Tsai <minghsiu.tsai@mediatek.com>
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

#include <linux/kernel.h>
#include <linux/slab.h>
#include "mtk_vcodec_vdec_conv.h"
#include "mtk_vcodec_vdec_conv_ipi.h"
#include "mtk_vpu_core.h"

#define VDEC_CONV_TIMEOUT_MS		2000

struct mtk_vdec_conv {
	struct platform_device		*pdev;
	atomic_t			event;
	wait_queue_head_t		wq;
	unsigned int			failure;
	unsigned int			msg_id;
	unsigned int			vpu_handle;

	struct vdec_conv_ipi_get	get;
	struct vdec_conv_ipi_release	release;
	struct vdec_conv_ipi_convert	convert;
	struct vdec_conv_fmt_param	*shmem_va;

	unsigned int			fmt;
	unsigned char			inited;
};

static unsigned int g_vdec_conv_reg;

static int mtk_vdec_conv_wait_ack(struct mtk_vdec_conv *hdl, unsigned int ms)
{
	int ret;

	ret = wait_event_interruptible_timeout(hdl->wq,
					       atomic_read(&hdl->event),
					       msecs_to_jiffies(ms));
	if (ret == 0) {
		dev_err(&hdl->pdev->dev, "wait %x ack timeout %dms !\n",
			hdl->msg_id, ms);
		return 1;
	} else if (ret == -ERESTARTSYS) {
		dev_err(&hdl->pdev->dev,
			"wait %x ack interrupted by a signal.\n",
			hdl->msg_id);
		return 1;
	}

	atomic_set(&hdl->event, 0);

	return 0;
}

static int mtk_vdec_conv_send_msg(struct mtk_vdec_conv *hdl, void *msg,
						 int len, int wait_ack)
{
	int err;

	err = vpu_ipi_send(hdl->pdev, IPI_CLR_CONV, msg, len, 1);
	if (err != 0) {
		dev_err(&hdl->pdev->dev, "ipi_send failed, err:%d\n", err);
		return -EBUSY;
	}

	if (wait_ack && mtk_vdec_conv_wait_ack(hdl, VDEC_CONV_TIMEOUT_MS))
		return -EBUSY;

	return 0;
}

static void mtk_vdec_conv_handle_get_ack(struct vdec_conv_ipi_get_ack *msg)
{
	struct mtk_vdec_conv *hdl = (struct mtk_vdec_conv *)msg->drv_handle;
	unsigned long vpu_addr;

	/* mapping VPU address to kernel virtual address */
	vpu_addr = msg->shmem_addr;
	hdl->shmem_va = vpu_mapping_dm_addr(hdl->pdev, (void *)vpu_addr);
	hdl->vpu_handle = msg->vpu_handle;
	hdl->inited = 1;
}

static void mtk_vdec_conv_ipi_handler(void *data, unsigned int len, void *priv)
{
	struct vdec_conv_ipi_get_ack *msg =
					(struct vdec_conv_ipi_get_ack *)data;
	struct mtk_vdec_conv *hdl = (struct mtk_vdec_conv *)msg->drv_handle;

	if (hdl == NULL)
		return;
	hdl->failure = msg->status;
	if (msg->status == VDEC_CONV_IPIMSG_STATUS_OK) {
		switch (msg->msg_id) {
		case VPU_VDEC_CONV_GET_ACK:
			mtk_vdec_conv_handle_get_ack(msg);
			break;
		case VPU_VDEC_CONV_RELEASE_ACK:
			hdl->inited = 0;
			break;
		case VPU_VDEC_CONV_VDEC_ACK:
		case VPU_VDEC_CONV_JPEG_ACK:
			break;
		default:
			dev_err(&hdl->pdev->dev, "unknown msg %x!\n",
							msg->msg_id);
			break;
		}
	}

	atomic_set(&hdl->event, 1);
	wake_up_interruptible(&hdl->wq);
}

int mtk_vdec_conv_create(struct platform_device *pdev, unsigned long *h_conv,
						      enum vdec_conv_fmt fmt)
{
	struct mtk_vdec_conv *hdl;

	hdl = kzalloc(sizeof(*hdl), GFP_KERNEL);
	if (!hdl)
		return -ENOMEM;

	memset(hdl, 0, sizeof(*hdl));
	atomic_set(&hdl->event, 0);
	init_waitqueue_head(&hdl->wq);
	hdl->pdev = vpu_get_plat_device(pdev);

	switch (fmt) {
	case VDEC_CONV_FMT_VIDEO:
		hdl->fmt = AP_VDEC_CONV_FMT_VDEC;
		break;
	case VDEC_CONV_FMT_JPEG:
		hdl->fmt = AP_VDEC_CONV_FMT_JPEG;
		break;
	default:
		kfree(hdl);
		return -EINVAL;
	}
	*h_conv = (unsigned long)hdl;

	return 0;
}

int mtk_vdec_conv_destroy(unsigned long h_conv)
{
	struct mtk_vdec_conv *hdl = (struct mtk_vdec_conv *)h_conv;

	if (hdl == NULL)
		return -EINVAL;
	kfree(hdl);
	return 0;
}

int mtk_vdec_conv_init(unsigned long h_conv)
{
	struct mtk_vdec_conv *hdl = (struct mtk_vdec_conv *)h_conv;
	struct vdec_conv_ipi_get *msg;
	int err;

	if (hdl == NULL)
		return -EINVAL;
	if (!g_vdec_conv_reg) {
		err = vpu_ipi_registration(hdl->pdev,
					   IPI_CLR_CONV,
					   mtk_vdec_conv_ipi_handler,
					   "VDEC_CONV",
					   NULL);
		if (err) {
			dev_err(&hdl->pdev->dev,
				"registration failed, err:%d\n", err);
			return -EINVAL;
		}
		g_vdec_conv_reg = 1;
	}

	if (hdl->inited)
		return 0;

	msg = &hdl->get;
	msg->msg_id = hdl->fmt | AP_VDEC_CONV_GET;
	msg->drv_handle = (uint64_t)hdl;
	err = mtk_vdec_conv_send_msg(hdl, (void *)msg, sizeof(*msg), 1);
	if (err)
		return -EINVAL;
	if (hdl->failure != VDEC_CONV_IPIMSG_STATUS_OK) {
		err = hdl->failure;
		dev_err(&hdl->pdev->dev, "msg_ack failed, err:%d\n", err);
		return -EINVAL;
	}

	return 0;
}

int mtk_vdec_conv_deinit(unsigned long h_conv)
{
	struct mtk_vdec_conv *hdl = (struct mtk_vdec_conv *)h_conv;
	struct vdec_conv_ipi_release *msg;
	int err;

	if (hdl == NULL)
		return -EINVAL;
	if (!hdl->inited)
		return -EINVAL;

	msg = &hdl->release;
	msg->msg_id = hdl->fmt | AP_VDEC_CONV_RELEASE;
	msg->vpu_handle = hdl->vpu_handle;
	msg->drv_handle = h_conv;
	err = mtk_vdec_conv_send_msg(hdl, (void *)msg, sizeof(*msg), 1);
	if (!err && hdl->failure != VDEC_CONV_IPIMSG_STATUS_OK) {
		err = hdl->failure;
		dev_err(&hdl->pdev->dev, "msg_ack failed, err:%d\n", err);
	}

	return err;
}

static int mtk_vdec_conv_start(unsigned long h_conv, unsigned int msg_id)
{
	struct mtk_vdec_conv *hdl = (struct mtk_vdec_conv *)h_conv;
	struct vdec_conv_ipi_convert *msg;
	int err;

	if (!hdl->inited)
		return -EINVAL;

	msg = &hdl->convert;
	msg->msg_id = hdl->fmt | msg_id;
	msg->vpu_handle = hdl->vpu_handle;
	msg->drv_handle = h_conv;
	err = mtk_vdec_conv_send_msg(hdl, (void *)msg, sizeof(*msg), 1);
	if (!err && hdl->failure != VDEC_CONV_IPIMSG_STATUS_OK) {
		err = hdl->failure;
		dev_err(&hdl->pdev->dev, "msg_ack failed, err:%d\n", err);
	}

	return err;
}

int mtk_vdec_conv_nv12(unsigned long h_conv, uint64_t y_srcaddr,
			uint64_t uv_srcaddr, uint32_t width,
			uint32_t height, uint64_t dstaddr)
{
	struct mtk_vdec_conv *hdl = (struct mtk_vdec_conv *)h_conv;
	struct vdec_conv_fmt_param *param = hdl->shmem_va;

	if (hdl == NULL)
		return -EINVAL;
	if (hdl->shmem_va == NULL) {
		dev_err(&hdl->pdev->dev, "shmem_va is invalid\n");
		return -EINVAL;
	}

	param->src_w = width;
	param->src_h = height;
	param->src_addr[0] = y_srcaddr;
	param->src_addr[1] = uv_srcaddr;
	param->src_plane_size[0] = width * height;
	param->src_plane_size[1] = param->src_plane_size[0] >> 1;
	param->src_plane_num = 2;
	param->src_roi_x = 0;
	param->src_roi_y = 0;
	param->src_roi_w = 0;
	param->src_roi_h = 0;
	param->src_y_pitch = 0; /* use default */
	param->src_uv_pitch = 0; /* use default */
	param->dst_w = width;
	param->dst_h = height;
	param->dst_addr[0] = dstaddr;
	param->dst_addr[1] = dstaddr + param->src_plane_size[0];
	param->dst_plane_size[0] = param->src_plane_size[0];
	param->dst_plane_size[1] = param->src_plane_size[1];
	param->dst_plane_num = 2;

	return mtk_vdec_conv_start(h_conv, AP_VDEC_CONV_VDEC);
}

int mtk_vdec_conv_i420(unsigned long h_conv, uint64_t y_srcaddr,
			uint64_t u_srcaddr, uint64_t v_srcaddr,
			uint32_t src_width, uint32_t src_height,
			uint64_t y_dstaddr, uint64_t u_dstaddr,
			uint64_t v_dstaddr, uint32_t dst_width,
			uint32_t dst_height, bool src_is_422)
{
	struct mtk_vdec_conv *hdl = (struct mtk_vdec_conv *)h_conv;
	struct vdec_conv_fmt_param *param = hdl->shmem_va;

	if (hdl == NULL)
		return -EINVAL;
	if (hdl->shmem_va == NULL) {
		dev_err(&hdl->pdev->dev, "shmem_va is invalid\n");
		return -EINVAL;
	}

	param->src_w = src_width;
	param->src_h = src_height;
	param->src_addr[0] = y_srcaddr;
	param->src_addr[1] = u_srcaddr;
	param->src_addr[2] = v_srcaddr;
	param->src_plane_size[0] = src_width * src_height;
	param->src_plane_size[1] = param->src_plane_size[0] >>
				   (src_is_422 ? 1 : 2);
	param->src_plane_size[2] = param->src_plane_size[1];
	param->src_plane_num = 3;
	param->src_roi_x = 0;
	param->src_roi_y = 0;
	param->src_roi_w = dst_width;
	param->src_roi_h = dst_height;
	param->src_y_pitch = src_width;
	param->src_uv_pitch = ((src_width >> 1) + 15) & ~15;
	param->dst_w = dst_width;
	param->dst_h = dst_height;
	param->dst_addr[0] = y_dstaddr;
	param->dst_addr[1] = u_dstaddr;
	param->dst_addr[2] = v_dstaddr;
	param->dst_plane_size[0] = dst_width * dst_height;
	param->dst_plane_size[1] = param->dst_plane_size[0] >> 2;
	param->dst_plane_size[2] = param->dst_plane_size[1];
	param->dst_plane_num = 3;

	return mtk_vdec_conv_start(h_conv, AP_VDEC_CONV_JPEG);
}
