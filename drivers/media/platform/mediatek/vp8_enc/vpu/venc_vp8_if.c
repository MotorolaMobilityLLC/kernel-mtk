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

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/time.h>

#include "mtk_vcodec_drv.h"
#include "mtk_vcodec_util.h"
#include "mtk_vcodec_intr.h"
#include "mtk_vcodec_enc.h"
#include "mtk_vcodec_pm.h"
#include "mtk_vpu_core.h"

#include "venc_vp8_if.h"
#include "venc_vp8_vpu.h"

#define vp8_enc_write_reg(h, addr, val)	writel(val, h->hw_base + addr)
#define vp8_enc_read_reg(h, addr) readl(h->hw_base + addr)

#define VENC_PIC_BITSTREAM_BYTE_CNT 0x0098
#define VENC_PIC_BITSTREAM_BYTE_CNT1 0x00E8
#define VENC_IRQ_STATUS_ENC_FRM_INT 0x04

static int vp8_enc_alloc_work_buf(struct venc_vp8_handle *hndl)
{
	int i, j;
	int ret = 0;
	struct venc_vp8_vpu_buf *wb = hndl->vpu_inst.drv->wb;

	mtk_vcodec_debug_enter(hndl);
	for (i = 0; i < VENC_VP8_VPU_WORK_BUF_MAX; i++) {
		/*
		 * Only temporal scalability mode will use RC_CODE2 & RC_CODE3
		 * Each three temporal layer has its own rate control code.
		 */
		if ((i >= VENC_VP8_VPU_WORK_BUF_RC_CODE2) && !hndl->ts_mode)
			break;

		/*
		 * This 'wb' structure is set by VPU side and shared to AP for
		 * buffer allocation and physical addr mapping. For most of
		 * the buffers, AP will allocate the buffer according to 'size'
		 * field and store the physical addr in 'pa' field. For the
		 * RC_CODEx buffers, they are pre-allocated in the VPU side,
		 * and save the VPU addr in the 'vpua' field. The AP will
		 * translate the VPU addr to the corresponding physical addr
		 * and store in 'pa' field.
		 */
		if (i < VENC_VP8_VPU_WORK_BUF_RC_CODE) {
			hndl->work_buf[i].size = wb[i].size;
			if (mtk_vcodec_mem_alloc(hndl->ctx,
						 &hndl->work_buf[i])) {
				mtk_vcodec_err(hndl,
					       "cannot alloc work_buf[%d]", i);
				ret = -ENOMEM;
				goto err_alloc;
			}
			wb[i].pa = hndl->work_buf[i].dma_addr;

			mtk_vcodec_debug(hndl,
					 "work_buf[%d] va=0x%p,pa=0x%p,size=0x%zx",
					 i, hndl->work_buf[i].va,
					 (void *)hndl->work_buf[i].dma_addr,
					 hndl->work_buf[i].size);
		} else {
			hndl->work_buf[i].size = wb[i].size;
			hndl->work_buf[i].va =
				vpu_mapping_dm_addr(hndl->dev,
						    (uintptr_t *)
						    (unsigned long)wb[i].vpua);
#ifdef CONFIG_MTK_IOMMU
			hndl->work_buf[i].dma_addr =
				(dma_addr_t)vpu_mapping_iommu_dm_addr(hndl->dev,
					(uintptr_t *)(unsigned long)wb[i].vpua);
#else
			hndl->work_buf[i].dma_addr =
				(dma_addr_t)vpu_mapping_ext_mem_addr(hndl->dev,
					hndl->work_buf[i].va);
#endif
			wb[i].pa = hndl->work_buf[i].dma_addr;
		}
	}
	mtk_vcodec_debug_leave(hndl);

	return ret;

err_alloc:
	for (j = 0; j < i; j++)
		mtk_vcodec_mem_free(hndl->ctx,
				    &hndl->work_buf[j]);
	return ret;
}

static void vp8_enc_free_work_buf(struct venc_vp8_handle *hndl)
{
	int i;

	mtk_vcodec_debug_enter(hndl);

	/* Except the RC_CODEx buffers, other buffers need to be freed by AP. */
	for (i = 0; i < VENC_VP8_VPU_WORK_BUF_RC_CODE; i++)
		mtk_vcodec_mem_free(hndl->ctx, &hndl->work_buf[i]);

	mtk_vcodec_debug_leave(hndl);
}

static unsigned int vp8_enc_wait_venc_done(struct venc_vp8_handle *hndl)
{
	struct mtk_vcodec_ctx *ctx = (struct mtk_vcodec_ctx *)hndl->ctx;
	unsigned int irq_status;

	mtk_vcodec_wait_for_done_ctx(ctx, MTK_INST_IRQ_RECEIVED, 1000, true);
	irq_status = ctx->irq_status;
	mtk_vcodec_debug(hndl, "isr return %x", irq_status);

	return irq_status;
}

/*
 * Compose ac_tag, bitstream header and bitstream payload into
 * one bitstream buffer.
 */
static int vp8_enc_compose_one_frame(struct venc_vp8_handle *hndl,
				     struct mtk_vcodec_mem *bs_buf,
				     unsigned int *bs_size)
{
	unsigned int is_key;
	unsigned int tag_sz;
	unsigned int bs_size_frm;
	unsigned int bs_hdr_len;
	unsigned char ac_tag[10];

	bs_size_frm = vp8_enc_read_reg(hndl,
				       VENC_PIC_BITSTREAM_BYTE_CNT);
	bs_hdr_len = vp8_enc_read_reg(hndl,
				      VENC_PIC_BITSTREAM_BYTE_CNT1);

	/* if a frame is key frame, is_key is 0 */
	is_key = (hndl->frm_cnt %
		  hndl->vpu_inst.drv->config.intra_period) ? 1 : 0;
	*(unsigned int *)ac_tag = (bs_hdr_len << 5) | 0x10 | is_key;
	/* key frame */
	if (is_key == 0) {
		ac_tag[3] = 0x9d;
		ac_tag[4] = 0x01;
		ac_tag[5] = 0x2a;
		ac_tag[6] = hndl->vpu_inst.drv->config.pic_w;
		ac_tag[7] = hndl->vpu_inst.drv->config.pic_w >> 8;
		ac_tag[8] = hndl->vpu_inst.drv->config.pic_h;
		ac_tag[9] = hndl->vpu_inst.drv->config.pic_h >> 8;
	}
	tag_sz = (is_key == 0) ? 10 : 3;

	if (bs_buf->size <= bs_hdr_len + bs_size_frm + tag_sz) {
		mtk_vcodec_err(hndl, "bitstream buf size is too small(%zd)",
			       bs_buf->size);
		return -EINVAL;
	}
	memmove(bs_buf->va + bs_hdr_len + tag_sz,
		bs_buf->va, bs_size_frm);
	memcpy(bs_buf->va + tag_sz,
	       hndl->work_buf[VENC_VP8_VPU_WORK_BUF_BS_HD].va,
	       bs_hdr_len);
	memcpy(bs_buf->va, ac_tag, tag_sz);
	*bs_size = bs_size_frm + bs_hdr_len + tag_sz;
#if VP8_ENCODE_BENCHMARK
	hndl->total_bs_size += *bs_size;
#endif

	return 0;
}

static int vp8_enc_encode_frame(struct venc_vp8_handle *hndl,
				struct venc_frm_buf *frm_buf,
				struct mtk_vcodec_mem *bs_buf,
				unsigned int *bs_size)
{
	unsigned int irq_status;
#if VP8_ENCODE_BENCHMARK
	struct timeval begin, end;

	do_gettimeofday(&begin);
#endif

	mtk_vcodec_debug(hndl, "->frm_cnt=%d", hndl->frm_cnt);

	if (vp8_enc_vpu_encode(hndl, frm_buf, bs_buf)) {
		mtk_vcodec_err(hndl, "vp8_enc_vpu_encode failed");
		return -EINVAL;
	}

#if VP8_ENCODE_BENCHMARK
	do_gettimeofday(&end);
	hndl->total_encode_time_1 +=
		((end.tv_sec - begin.tv_sec) * 1000000 + end.tv_usec - begin.tv_usec);
#endif

#if VP8_ENCODE_BENCHMARK
	do_gettimeofday(&begin);
#endif

	irq_status = vp8_enc_wait_venc_done(hndl);
	if (irq_status != VENC_IRQ_STATUS_ENC_FRM_INT) {
		mtk_vcodec_err(hndl, "irq_status=%d failed", irq_status);
		return -EINVAL;
	}

#if VP8_ENCODE_BENCHMARK
	do_gettimeofday(&end);
	hndl->total_encode_time_2 +=
		((end.tv_sec - begin.tv_sec) * 1000000 + end.tv_usec - begin.tv_usec);
#endif

#if VP8_ENCODE_BENCHMARK
	do_gettimeofday(&begin);
#endif

	if (vp8_enc_compose_one_frame(hndl, bs_buf, bs_size)) {
		mtk_vcodec_err(hndl, "vp8_enc_compose_one_frame failed");
		return -EINVAL;
	}

#if VP8_ENCODE_BENCHMARK
	do_gettimeofday(&end);
	hndl->total_encode_time_3 +=
		((end.tv_sec - begin.tv_sec) * 1000000 + end.tv_usec - begin.tv_usec);
	++hndl->total_frm_cnt;
#endif

	hndl->frm_cnt++;
	mtk_vcodec_debug(hndl, "<-size=%d", *bs_size);

	return 0;
}

int vp8_enc_init(struct mtk_vcodec_ctx *ctx, unsigned long *handle)
{
	struct venc_vp8_handle *h;
#if VP8_ENCODE_BENCHMARK
	struct timeval begin, end;

	do_gettimeofday(&begin);
#endif

	h = kzalloc(sizeof(*h), GFP_KERNEL);
	if (!h)
		return -ENOMEM;

	h->ctx = ctx;
	h->dev = mtk_vcodec_get_plat_dev(ctx);
	h->hw_base = mtk_vcodec_get_reg_addr(h->ctx, VENC_LT_SYS);

	if (vp8_enc_vpu_init(h)) {
		mtk_vcodec_err(h, "vp8_enc_vpu_init failed");
		return -EINVAL;
	}

	(*handle) = (unsigned long)h;
#if VP8_ENCODE_BENCHMARK
	do_gettimeofday(&end);
	h->total_init_time +=
		((end.tv_sec - begin.tv_sec) * 1000000 + end.tv_usec - begin.tv_usec);
#endif

	return 0;
}

int vp8_enc_encode(unsigned long handle,
		   enum venc_start_opt opt,
		   struct venc_frm_buf *frm_buf,
		   struct mtk_vcodec_mem *bs_buf,
		   struct venc_done_result *result)
{
	int ret = 0;
	struct venc_vp8_handle *h = (struct venc_vp8_handle *)handle;
#if VP8_ENCODE_BENCHMARK
	struct timeval begin, end;

	do_gettimeofday(&begin);
#endif

	mtk_vcodec_debug_enter(h);

	switch (opt) {
	case VENC_START_OPT_ENCODE_FRAME:
		if (vp8_enc_encode_frame(h, frm_buf, bs_buf, &result->bs_size)) {
			mtk_vcodec_err(h, "vp8_enc_encode_frame failed");
			ret = -EINVAL;
			result->msg = VENC_MESSAGE_ERR;
		} else {
			result->msg = VENC_MESSAGE_OK;
		}
		result->is_key_frm = ((*((unsigned char *)bs_buf->va) &
				       0x01) == 0);
		break;

	default:
		mtk_vcodec_err(h, "opt not support:%d", opt);
		ret = -EINVAL;
		break;
	}

	mtk_vcodec_debug_leave(h);
#if VP8_ENCODE_BENCHMARK
	do_gettimeofday(&end);
	h->total_encode_time +=
		((end.tv_sec - begin.tv_sec) * 1000000 + end.tv_usec - begin.tv_usec);
#endif

	return ret;
}

int vp8_enc_set_param(unsigned long handle,
		      enum venc_set_param_type type, void *in)
{
	int ret = 0;
	struct venc_vp8_handle *h = (struct venc_vp8_handle *)handle;
	struct venc_enc_prm *enc_prm;
#if VP8_ENCODE_BENCHMARK
	struct timeval begin, end;

	do_gettimeofday(&begin);
#endif

	mtk_vcodec_debug(h, "->type=%d", type);

	switch (type) {
	case VENC_SET_PARAM_ENC:
		enc_prm = in;
		if (vp8_enc_vpu_set_param(h,
					  VENC_SET_PARAM_ENC,
					  enc_prm)) {
			mtk_vcodec_err(h, "set param failed");
			ret = -EINVAL;
			break;
		}
		if (h->work_buf_alloc == 0) {
			if (vp8_enc_alloc_work_buf(h)) {
				mtk_vcodec_err(h,
					       "vp8_enc_alloc_work_buf failed");
				ret = -ENOMEM;
			} else {
				h->work_buf_alloc = 1;
			}
		}
		break;

	case VENC_SET_PARAM_FORCE_INTRA:
		if (vp8_enc_vpu_set_param(h,
					  VENC_SET_PARAM_FORCE_INTRA, 0)) {
			mtk_vcodec_err(h, "force intra failed");
			ret = -EINVAL;
		} else {
			h->frm_cnt = 0;
		}
		break;

	case VENC_SET_PARAM_ADJUST_BITRATE:
		enc_prm = in;
		if (vp8_enc_vpu_set_param(h, VENC_SET_PARAM_ADJUST_BITRATE,
					  &enc_prm->bitrate)) {
			mtk_vcodec_err(h, "adjust bitrate failed");
			ret = -EINVAL;
		}
		break;

	case VENC_SET_PARAM_ADJUST_FRAMERATE:
		enc_prm = in;
		if (vp8_enc_vpu_set_param(h, VENC_SET_PARAM_ADJUST_FRAMERATE,
					  &enc_prm->frm_rate)) {
			mtk_vcodec_err(h, "adjust frame rate failed");
			ret = -EINVAL;
		}
		break;

	case VENC_SET_PARAM_I_FRAME_INTERVAL:
		if (vp8_enc_vpu_set_param(h,
					  VENC_SET_PARAM_I_FRAME_INTERVAL,
					  in)) {
			mtk_vcodec_err(h, "set I frame interval failed");
			ret = -EINVAL;
		} else {
			h->frm_cnt = 0; /* reset counter */
		}
		break;

	/*
	 * VENC_SET_PARAM_TS_MODE must be called before
	 * VENC_SET_PARAM_ENC
	 */
	case VENC_SET_PARAM_TS_MODE:
		h->ts_mode = 1;
		mtk_vcodec_debug(h, "set ts_mode");
		break;

	default:
		mtk_vcodec_err(h, "type not support:%d", type);
		ret = -EINVAL;
		break;
	}

	mtk_vcodec_debug_leave(h);
#if VP8_ENCODE_BENCHMARK
	do_gettimeofday(&end);
	h->total_set_param_time +=
		((end.tv_sec - begin.tv_sec) * 1000000 + end.tv_usec - begin.tv_usec);
#endif

	return ret;
}

int vp8_enc_deinit(unsigned long handle)
{
	int ret = 0;
	struct venc_vp8_handle *h = (struct venc_vp8_handle *)handle;
#if VP8_ENCODE_BENCHMARK
	struct timeval begin, end;

	do_gettimeofday(&begin);
#endif

	mtk_vcodec_debug_enter(h);

	if (vp8_enc_vpu_deinit(h)) {
		mtk_vcodec_err(h, "vp8_enc_vpu_deinit failed");
		ret = -EINVAL;
	}

	if (h->work_buf_alloc)
		vp8_enc_free_work_buf(h);

#if VP8_ENCODE_BENCHMARK
	do_gettimeofday(&end);
	h->total_deinit_time +=
		((end.tv_sec - begin.tv_sec) * 1000000 + end.tv_usec - begin.tv_usec);

	mtk_vcodec_err(h, "total_frm_cnt: %d", h->total_frm_cnt);
	mtk_vcodec_err(h, "total_bs_size: %d", h->total_bs_size);
	mtk_vcodec_err(h, "total_time: %d us",
		       h->total_init_time +
		       h->total_set_param_time +
		       h->total_encode_time +
		       h->total_deinit_time);
	mtk_vcodec_err(h, "  init_time: %d us", h->total_init_time);
	mtk_vcodec_err(h, "  set_param_time: %d us", h->total_set_param_time);
	mtk_vcodec_err(h, "  encode_time: %d us", h->total_encode_time);
	mtk_vcodec_err(h, "    encode_time_1: %d us", h->total_encode_time_1);
	mtk_vcodec_err(h, "    encode_time_2: %d us", h->total_encode_time_2);
	mtk_vcodec_err(h, "    encode_time_3: %d us", h->total_encode_time_3);
	mtk_vcodec_err(h, "  deinit_time: %d us", h->total_deinit_time);
	mtk_vcodec_err(h, "  FPS: %d",
		       h->total_frm_cnt * 1000000L /
		       (h->total_init_time + h->total_set_param_time +
		       h->total_encode_time + h->total_deinit_time));
#endif

	mtk_vcodec_debug_leave(h);
	kfree(h);

	return ret;
}

struct venc_common_if venc_vp8_if = {
	vp8_enc_init,
	vp8_enc_encode,
	vp8_enc_set_param,
	vp8_enc_deinit,
};

struct venc_common_if *get_vp8_enc_comm_if(void)
{
	return &venc_vp8_if;
}
