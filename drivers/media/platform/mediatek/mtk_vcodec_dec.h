/*
* Copyright (c) 2015 MediaTek Inc.
* Author: PC Chen <pc.chen@mediatek.com>
*         Tiffany Lin <tiffany.lin@mediatek.com>
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

#ifndef _MEDIA_PLATFORM_MEDIATEK_MTK_VCODEC_DEC_H_
#define _MEDIA_PLATFORM_MEDIATEK_MTK_VCODEC_DEC_H_

#include <media/v4l2-ioctl.h>
#include <media/videobuf2-core.h>

/*
 * mtk_vdec_lock/mtk_vdec_unlock are for ctx instance to
 * get/release lock before/after access decoder hw.
 * mtk_vdec_lock get decoder hw lock and set curr_ctx
 * to idx of ctx instance that get lock
 */
int mtk_vdec_unlock(struct mtk_vcodec_ctx *ctx);
int mtk_vdec_lock(struct mtk_vcodec_ctx *ctx);
int m2mctx_vdec_queue_init(void *priv, struct vb2_queue *src_vq,
			   struct vb2_queue *dst_vq);
void mtk_vcodec_dec_set_default_params(struct mtk_vcodec_ctx *ctx);
void mtk_vcodec_vdec_release(struct mtk_vcodec_ctx *ctx);
int mtk_vdec_ctrls_setup(struct mtk_vcodec_ctx *ctx);
void mtk_vdec_ctrls_free(struct mtk_vcodec_ctx *ctx);

#endif /* _MEDIA_PLATFORM_MEDIATEK_MTK_VCODEC_DEC_H_ */
