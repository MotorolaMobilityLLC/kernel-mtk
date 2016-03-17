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

#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/slab.h>

#include "mtk_vcodec_drv.h"
#include "mtk_vcodec_dec.h"
#include "mtk_vcodec_pm.h"
#include "mtk_vcodec_util.h"

#include "vdec_drv_base.h"
#include "vdec_drv_if.h"
#include "vdec_h264_if.h"
#include "vdec_vp8_if.h"
#include "vdec_drv_vp9_if.h"
#include "vdec_mpeg4_if.h"

int vdec_if_create(void *ctx, unsigned int fourcc, unsigned long *handle)
{
	struct vdec_handle *h_vdec;
	char str[10];

	mtk_vcodec_fmt2str(fourcc, str);

	h_vdec = kzalloc(sizeof(*h_vdec), GFP_KERNEL);
	if (!h_vdec)
		return -ENOMEM;

	h_vdec->fourcc = fourcc;
	h_vdec->drv_init_flag = false;
	h_vdec->ctx = ctx;

	mtk_vcodec_debug(h_vdec, "fmt=%s handle=%p", str, h_vdec);

	switch (fourcc) {
	case V4L2_PIX_FMT_H264:
		h_vdec->dec_if = get_h264_dec_comm_if();
		break;
	case V4L2_PIX_FMT_VP8:
		h_vdec->dec_if = get_vp8_dec_comm_if();
		break;
	case V4L2_PIX_FMT_VP9:
		h_vdec->dec_if = get_vp9_dec_comm_if();
		break;
	case V4L2_PIX_FMT_MPEG4:
		h_vdec->dec_if = get_mpeg4_dec_comm_if();
		break;
	default:
		mtk_vcodec_err(h_vdec, "invalid fmt=%s", str);
		goto err_out;
	}

	*handle = (unsigned long)h_vdec;
	return 0;

err_out:
	kfree(h_vdec);
	return -EINVAL;
}

int vdec_if_init(unsigned long handle, struct mtk_vcodec_mem *bs,
		 struct vdec_pic_info *pic)
{
	struct vdec_handle *h = ((struct vdec_handle *)handle);
	char str[10];
	int ret = 0;

	mtk_vcodec_fmt2str(h->fourcc, str);
	mtk_vcodec_debug(h, "+ fmt=%s", str);

	if (h->fourcc != V4L2_PIX_FMT_H264 &&
	    h->fourcc != V4L2_PIX_FMT_VP8 &&
	    h->fourcc != V4L2_PIX_FMT_VP9 &&
	    h->fourcc != V4L2_PIX_FMT_MPEG4) {
		mtk_vcodec_err(h, "invalid fmt=%s", str);
		return -EINVAL;
	}

	mtk_vdec_lock(h->ctx);
	mtk_vcodec_dec_pw_on();
	ret = h->dec_if->init(h->ctx, bs, &h->drv_handle, pic);
	mtk_vcodec_dec_pw_off();
	mtk_vdec_unlock(h->ctx);

	if (ret != 0)
		h->drv_init_flag = false;
	else
		h->drv_init_flag = true;

	mtk_vcodec_debug(h, "- ret=%d", ret);
	return ret;
}

int vdec_if_decode(unsigned long handle, struct mtk_vcodec_mem *bs,
		   struct vdec_fb *fb, bool *res_chg)
{
	int ret = 0;
	struct vdec_handle *h = (struct vdec_handle *)handle;
	char str[10];

	mtk_vcodec_fmt2str(h->fourcc, str);
	mtk_vcodec_debug(h, "+ fmt=%s", str);

	if (bs) {
		if (((unsigned long)bs->va & 63) != 0 ||
		    (bs->dma_addr & 63) != 0) {
			mtk_vcodec_err(h, "in buf not 64 align, %p, %llx",
				       bs->va, (u64)bs->dma_addr);
			return -EINVAL;
		}

#ifdef Y_C_SEPARATE
		if (((unsigned long)fb->base_y.va & 511) != 0 ||
		    (fb->base_y.dma_addr & 511) != 0) {
			mtk_vcodec_err(h, "out buf not 512 align, %p, %llx",
				       fb->base_y.va, (u64)fb->base_y.dma_addr);
			return -EINVAL;
		}
#else
		if (((unsigned long)fb->base.va & 511) != 0 ||
		    (fb->base.dma_addr & 511) != 0) {
			mtk_vcodec_err(h, "out buf not 512 align, %p, %llx",
				       fb->base.va, (u64)fb->base.dma_addr);
			return -EINVAL;
		}
#endif
	}

	mtk_vdec_lock(h->ctx);
	mtk_vcodec_dec_pw_on();
	enable_irq(h->ctx->dev->dec_irq);
	ret = h->dec_if->decode(h->drv_handle, bs, fb, res_chg);
	disable_irq(h->ctx->dev->dec_irq);
	mtk_vcodec_dec_pw_off();
	mtk_vdec_unlock(h->ctx);

	mtk_vcodec_debug(h, "- ret=%d", ret);

	return ret;
}

int vdec_if_get_param(unsigned long handle, enum vdec_get_param_type type,
		      void *out)
{
	int ret = 0;
	struct vdec_handle *h = (struct vdec_handle *)handle;

	mtk_vdec_lock(h->ctx);
	ret = h->dec_if->get_param(h->drv_handle, type, out);
	mtk_vdec_unlock(h->ctx);

	return ret;
}

int vdec_if_deinit(unsigned long handle)
{
	int ret = 0;
	struct vdec_handle *h = (struct vdec_handle *)handle;

	mtk_vcodec_debug_enter(h);

	if (h->drv_init_flag) {
		mtk_vdec_lock(h->ctx);
		mtk_vcodec_dec_pw_on();
		ret = h->dec_if->deinit(h->drv_handle);
		mtk_vcodec_dec_pw_off();
		mtk_vdec_unlock(h->ctx);
		h->drv_init_flag = false;
	}

	mtk_vcodec_debug_leave(h);
	return ret;
}

void vdec_if_release(unsigned long handle)
{
	struct vdec_handle *h = (struct vdec_handle *)handle;

	mtk_vcodec_debug_enter(h);
	kfree(h);
}
