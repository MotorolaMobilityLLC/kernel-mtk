/*
 * Copyright (c) 2015 MediaTek Inc.
 * Author: Daniel Hsiao <daniel.hsiao@mediatek.com>
 *         Jungchang Tsao <jungchang.tsao@mediatek.com>
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

#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/slab.h>

#include "mtk_vcodec_drv.h"
#include "mtk_vcodec_enc.h"
#include "mtk_vcodec_pm.h"
#include "mtk_vcodec_util.h"

#include "venc_drv_if.h"
#include "venc_drv_base.h"
#include "venc_h264_if.h"
#include "venc_vp8_if.h"

int venc_if_create(void *ctx, unsigned int fourcc, unsigned long *handle)
{
	struct venc_handle *h;
	char str[10];

	mtk_vcodec_fmt2str(fourcc, str);

	h = kzalloc(sizeof(*h), GFP_KERNEL);
	if (!h)
		return -ENOMEM;

	h->fourcc = fourcc;
	h->ctx = ctx;
	mtk_vcodec_debug(h, "fmt = %s handle = %p", str, h);

	switch (fourcc) {
	case V4L2_PIX_FMT_H264:
		h->enc_if = get_h264_enc_comm_if();
		break;
	case V4L2_PIX_FMT_VP8:
		h->enc_if = get_vp8_enc_comm_if();
		break;
	default:
		mtk_vcodec_err(h, "invalid format %s", str);
		goto err_out;
	}

	*handle = (unsigned long)h;
	return 0;

err_out:
	kfree(h);
	return -EINVAL;
}

int venc_if_init(unsigned long handle)
{
	int ret = 0;
	struct venc_handle *h = (struct venc_handle *)handle;

	mtk_vcodec_debug_enter(h);

	mtk_venc_lock(h->ctx);
	mtk_vcodec_enc_clock_on();
	ret = h->enc_if->init(h->ctx, (unsigned long *)&h->drv_handle);
	mtk_vcodec_enc_clock_off();
	mtk_venc_unlock(h->ctx);

	return ret;
}

int venc_if_set_param(unsigned long handle,
		      enum venc_set_param_type type, void *in)
{
	int ret = 0;
	struct venc_handle *h = (struct venc_handle *)handle;

	mtk_vcodec_debug(h, "type=%d", type);

	mtk_venc_lock(h->ctx);
	mtk_vcodec_enc_clock_on();
	ret = h->enc_if->set_param(h->drv_handle, type, in);
	mtk_vcodec_enc_clock_off();
	mtk_venc_unlock(h->ctx);

	return ret;
}

int venc_if_encode(unsigned long handle,
		   enum venc_start_opt opt, struct venc_frm_buf *frm_buf,
		   struct mtk_vcodec_mem *bs_buf, struct venc_done_result *result)
{
	int ret = 0;
	struct venc_handle *h = (struct venc_handle *)handle;
	char str[10];

	mtk_vcodec_fmt2str(h->fourcc, str);
	mtk_vcodec_debug(h, "fmt=%s", str);

	mtk_venc_lock(h->ctx);
	mtk_vcodec_enc_clock_on();
	enable_irq((h->fourcc == V4L2_PIX_FMT_H264) ? h->ctx->dev->enc_irq : h->ctx->dev->enc_lt_irq);
	ret = h->enc_if->encode(h->drv_handle, opt, frm_buf, bs_buf, result);
	disable_irq((h->fourcc == V4L2_PIX_FMT_H264) ? h->ctx->dev->enc_irq : h->ctx->dev->enc_lt_irq);
	mtk_vcodec_enc_clock_off();
	mtk_venc_unlock(h->ctx);

	mtk_vcodec_debug(h, "ret=%d", ret);

	return ret;
}

int venc_if_deinit(unsigned long handle)
{
	int ret = 0;
	struct venc_handle *h = (struct venc_handle *)handle;

	mtk_vcodec_debug_enter(h);

	mtk_venc_lock(h->ctx);
	mtk_vcodec_enc_clock_on();
	ret = h->enc_if->deinit(h->drv_handle);
	mtk_vcodec_enc_clock_off();
	mtk_venc_unlock(h->ctx);

	return ret;
}

int venc_if_release(unsigned long handle)
{
	struct venc_handle *h = (struct venc_handle *)handle;

	mtk_vcodec_debug_enter(h);
	kfree(h);

	return 0;
}
