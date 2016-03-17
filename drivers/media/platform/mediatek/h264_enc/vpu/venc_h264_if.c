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

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/interrupt.h>

#include "mtk_vcodec_drv.h"
#include "mtk_vcodec_util.h"
#include "mtk_vcodec_intr.h"
#include "mtk_vcodec_enc.h"
#include "mtk_vcodec_pm.h"
#include "mtk_vpu_core.h"

#include "venc_h264_if.h"
#include "venc_h264_vpu.h"

#define h264_write_reg(h, addr, val)	writel(val, h->hw_base + addr)
#define h264_read_reg(h, addr) readl(h->hw_base + addr)

#define VENC_PIC_BITSTREAM_BYTE_CNT 0x0098

enum venc_h264_irq_status {
	H264_IRQ_STATUS_ENC_SPS_INT = (1 << 0),
	H264_IRQ_STATUS_ENC_PPS_INT = (1 << 1),
	H264_IRQ_STATUS_ENC_FRM_INT = (1 << 2),
};

static int h264_alloc_work_buf(struct venc_h264_handle *handle)
{
	int i, j;
	int ret = 0;
	struct venc_h264_vpu_buf *wb = handle->vpu_inst.drv->wb;

	mtk_vcodec_debug_enter(handle);

	for (i = 0; i < VENC_H264_VPU_WORK_BUF_MAX; i++) {
		/*
		 * This 'wb' structure is set by VPU side and shared to AP for
		 * buffer allocation and physical addr mapping. For most of
		 * the buffers, AP will allocate the buffer according to 'size'
		 * field and store the physical addr in 'pa' field. There are two
		 * exceptions:
		 * (1) RC_CODE buffer, it's pre-allocated in the VPU side, and
		 * save the VPU addr in the 'vpua' field. The AP will translate
		 * the VPU addr to the corresponding physical addr and store
		 * in 'pa' field for reg setting in VPU side.
		 * (2) SKIP_FRAME buffer, it's pre-allocated in the VPU side, and
		 * save the VPU addr in the 'vpua' field. The AP will translate
		 * the VPU addr to the corresponding AP side virtual address and
		 * do some memcpy access to move to bitstream buffer assigned
		 * by v4l2 layer.
		 */
		if (i == VENC_H264_VPU_WORK_BUF_RC_CODE) {
			handle->work_buf[i].size = wb[i].size;
			handle->work_buf[i].va = vpu_mapping_dm_addr(
				handle->dev, (uintptr_t *)(unsigned long)
				wb[i].vpua);
#ifdef CONFIG_MTK_IOMMU
			handle->work_buf[i].dma_addr =
				(dma_addr_t)vpu_mapping_iommu_dm_addr(
				handle->dev, (uintptr_t *)(unsigned long)
				wb[i].vpua);
#else
			handle->work_buf[i].dma_addr = (dma_addr_t)
				vpu_mapping_ext_mem_addr(handle->dev,
							 handle->work_buf[i].va);
#endif
			wb[i].pa = handle->work_buf[i].dma_addr;
		} else if (i == VENC_H264_VPU_WORK_BUF_SKIP_FRAME) {
			handle->work_buf[i].size = wb[i].size;
			handle->work_buf[i].va = vpu_mapping_dm_addr(
				handle->dev, (uintptr_t *)(unsigned long)
				wb[i].vpua);
			handle->work_buf[i].dma_addr = 0;
			wb[i].pa = handle->work_buf[i].dma_addr;
		} else {
			handle->work_buf[i].size = wb[i].size;
			if (mtk_vcodec_mem_alloc(handle->ctx,
						 &handle->work_buf[i])) {
				mtk_vcodec_err(handle, "cannot allocate buf %d", i);
				ret = -ENOMEM;
				goto err_alloc;
			}
			wb[i].pa = handle->work_buf[i].dma_addr;
		}
		mtk_vcodec_debug(handle, "buf[%d] va=0x%p pa=0x%p size=0x%zx", i,
				 handle->work_buf[i].va,
				 (void *)handle->work_buf[i].dma_addr,
				 handle->work_buf[i].size);
	}

	/* the pps_buf is used by AP side only */
	handle->pps_buf.size = 128;
	if (mtk_vcodec_mem_alloc(handle->ctx,
				 &handle->pps_buf)) {
		mtk_vcodec_err(handle, "cannot allocate pps_buf");
		ret = -ENOMEM;
		goto err_alloc;
	}
	mtk_vcodec_debug_leave(handle);

	return ret;

err_alloc:
	for (j = 0; j < i; j++) {
		if ((j != VENC_H264_VPU_WORK_BUF_RC_CODE) &&
		    (j != VENC_H264_VPU_WORK_BUF_SKIP_FRAME))
			mtk_vcodec_mem_free(handle->ctx, &handle->work_buf[j]);
	}

	return ret;
}

static void h264_free_work_buf(struct venc_h264_handle *handle)
{
	int i;

	mtk_vcodec_debug_enter(handle);
	for (i = 0; i < VENC_H264_VPU_WORK_BUF_MAX; i++) {
		if ((i != VENC_H264_VPU_WORK_BUF_RC_CODE) &&
		    (i != VENC_H264_VPU_WORK_BUF_SKIP_FRAME))
			mtk_vcodec_mem_free(handle->ctx, &handle->work_buf[i]);
	}
	mtk_vcodec_mem_free(handle->ctx, &handle->pps_buf);
	mtk_vcodec_debug_leave(handle);
}

static unsigned int h264_wait_venc_done(struct venc_h264_handle *handle)
{
	unsigned int irq_status = 0;
	struct mtk_vcodec_ctx *pctx = handle->ctx;

	mtk_vcodec_debug_enter(handle);
	mtk_vcodec_wait_for_done_ctx(pctx, MTK_INST_IRQ_RECEIVED, 1000, true);
	irq_status = pctx->irq_status;
	mtk_vcodec_debug(handle, "irq_status %x <-", irq_status);

	return irq_status;
}

static int h264_encode_sps(struct venc_h264_handle *handle,
			   struct mtk_vcodec_mem *bs_buf,
			   unsigned int *bs_size)
{
	unsigned int irq_status;

	mtk_vcodec_debug_enter(handle);

	if (h264_enc_vpu_encode(handle, H264_BS_MODE_SPS, NULL,
				bs_buf, bs_size)) {
		mtk_vcodec_err(handle, "h264_enc_vpu_encode sps failed");
		return -EINVAL;
	}

	irq_status = h264_wait_venc_done(handle);
	if (irq_status != H264_IRQ_STATUS_ENC_SPS_INT) {
		mtk_vcodec_err(handle, "expect irq status %d",
			       H264_IRQ_STATUS_ENC_SPS_INT);
		return -EINVAL;
	}

	*bs_size = h264_read_reg(handle, VENC_PIC_BITSTREAM_BYTE_CNT);
	mtk_vcodec_debug(handle, "bs size %d <-", *bs_size);

	return 0;
}

static int h264_encode_pps(struct venc_h264_handle *handle,
			   struct mtk_vcodec_mem *bs_buf,
			   unsigned int *bs_size)
{
	unsigned int irq_status;

	mtk_vcodec_debug_enter(handle);

	if (h264_enc_vpu_encode(handle, H264_BS_MODE_PPS, NULL,
				bs_buf, bs_size)) {
		mtk_vcodec_err(handle, "h264_enc_vpu_encode pps failed");
		return -EINVAL;
	}

	irq_status = h264_wait_venc_done(handle);
	if (irq_status != H264_IRQ_STATUS_ENC_PPS_INT) {
		mtk_vcodec_err(handle, "expect irq status %d",
			       H264_IRQ_STATUS_ENC_PPS_INT);
		return -EINVAL;
	}

	*bs_size = h264_read_reg(handle, VENC_PIC_BITSTREAM_BYTE_CNT);
	mtk_vcodec_debug(handle, "bs size %d <-", *bs_size);

	return 0;
}

static int h264_encode_frame(struct venc_h264_handle *handle,
			     struct venc_frm_buf *frm_buf,
			     struct mtk_vcodec_mem *bs_buf,
			     unsigned int *bs_size)
{
	unsigned int irq_status;

	mtk_vcodec_debug_enter(handle);

	if (h264_enc_vpu_encode(handle, H264_BS_MODE_FRAME, frm_buf,
				bs_buf, bs_size)) {
		mtk_vcodec_err(handle, "h264_enc_vpu_encode frame failed");
		return -EINVAL;
	}

	/*
	 * skip frame case: The skip frame buffer is composed by vpu side only,
	 * it does not trigger the hw, so skip the wait interrupt operation.
	 */
	if (!handle->vpu_inst.wait_int) {
		++handle->frm_cnt;
		return 0;
	}

	irq_status = h264_wait_venc_done(handle);
	if (irq_status != H264_IRQ_STATUS_ENC_FRM_INT) {
		mtk_vcodec_err(handle, "irq_status=%d failed", irq_status);
		return -EINVAL;
	}

	*bs_size = h264_read_reg(handle,
				 VENC_PIC_BITSTREAM_BYTE_CNT);
	++handle->frm_cnt;
	mtk_vcodec_debug(handle, "frm %d bs size %d key_frm %d <-",
			 handle->frm_cnt,
			 *bs_size, handle->is_key_frm);

	return 0;
}

static void h264_encode_filler(struct venc_h264_handle *handle, void *buf, int size)
{
	unsigned char *p = buf;

	*p++ = 0x0;
	*p++ = 0x0;
	*p++ = 0x0;
	*p++ = 0x1;
	*p++ = 0xc;
	size -= 5;
	while (size) {
		*p++ = 0xff;
		size -= 1;
	}
}

int h264_enc_init(struct mtk_vcodec_ctx *ctx, unsigned long *handle)
{
	struct venc_h264_handle *h;

	h = kzalloc(sizeof(*h), GFP_KERNEL);
	if (!h)
		return -ENOMEM;

	h->ctx = ctx;
	h->dev = mtk_vcodec_get_plat_dev(ctx);
	h->hw_base = mtk_vcodec_get_reg_addr(h->ctx, VENC_SYS);

	if (h264_enc_vpu_init(h)) {
		mtk_vcodec_err(h, "h264_enc_init failed");
		return -EINVAL;
	}

	(*handle) = (unsigned long)h;

	return 0;
}

int h264_enc_encode(unsigned long handle,
		    enum venc_start_opt opt,
		    struct venc_frm_buf *frm_buf,
		    struct mtk_vcodec_mem *bs_buf,
		    struct venc_done_result *result)
{
	int ret = 0;
	struct venc_h264_handle *h = (struct venc_h264_handle *)handle;

	mtk_vcodec_debug(h, "opt %d ->", opt);

	switch (opt) {
	case VENC_START_OPT_ENCODE_SEQUENCE_HEADER: {
		unsigned int bs_size_sps;
		unsigned int bs_size_pps;

		memset(bs_buf->va, 0x38, 20);
		if (h264_encode_sps(h, bs_buf, &bs_size_sps)) {
			mtk_vcodec_err(h, "h264_encode_sps failed");
			ret = -EINVAL;
			goto encode_err;
		}
		memset(h->pps_buf.va, 0x49, 20);

		if (h264_encode_pps(h, &h->pps_buf, &bs_size_pps)) {
			mtk_vcodec_err(h, "h264_encode_pps failed");
			ret = -EINVAL;
			goto encode_err;
		}

		memcpy(bs_buf->va + bs_size_sps,
		       h->pps_buf.va,
		       bs_size_pps);
		result->bs_size = bs_size_sps + bs_size_pps;
		result->is_key_frm = false;
	}
	break;

	case VENC_START_OPT_ENCODE_FRAME:
		if (h->prepend_hdr) {
			int hdr_sz;
			int hdr_sz_ext;
			int bs_alignment = 128;
			int filler_sz = 0;
			struct mtk_vcodec_mem tmp_bs_buf;
			unsigned int bs_size_sps;
			unsigned int bs_size_pps;
			unsigned int bs_size_frm;

			mtk_vcodec_debug(h,
					 "h264_encode_frame prepend SPS/PPS");
			if (h264_encode_sps(h, bs_buf, &bs_size_sps)) {
				mtk_vcodec_err(h,
					       "h264_encode_sps failed");
				ret = -EINVAL;
				goto encode_err;
			}

			if (h264_encode_pps(h, &h->pps_buf, &bs_size_pps)) {
				mtk_vcodec_err(h,
					       "h264_encode_pps failed");
				ret = -EINVAL;
				goto encode_err;
			}
			memcpy(bs_buf->va + bs_size_sps,
			       h->pps_buf.va,
			       bs_size_pps);

			hdr_sz = bs_size_sps + bs_size_pps;
			hdr_sz_ext = (hdr_sz & (bs_alignment - 1));
			if (hdr_sz_ext) {
				filler_sz = bs_alignment - hdr_sz_ext;
				if (hdr_sz_ext + 5 > bs_alignment)
					filler_sz += bs_alignment;
				h264_encode_filler(
					h, bs_buf->va + hdr_sz,
					filler_sz);
			}

			tmp_bs_buf.va = bs_buf->va + hdr_sz +
				filler_sz;
			tmp_bs_buf.dma_addr = bs_buf->dma_addr + hdr_sz +
				filler_sz;
			tmp_bs_buf.size = bs_buf->size -
				(hdr_sz + filler_sz);

			if (h264_encode_frame(h, frm_buf,
					      &tmp_bs_buf,
					      &bs_size_frm)) {
				mtk_vcodec_err(h,
					       "h264_encode_frame failed");
				ret = -EINVAL;
				goto encode_err;
			}

			result->bs_size = hdr_sz + filler_sz + bs_size_frm;
			mtk_vcodec_debug(h,
					 "hdr %d filler %d frame %d bs %d",
					 hdr_sz, filler_sz, bs_size_frm,
					 result->bs_size);

			h->prepend_hdr = 0;
		} else {
			if (h264_encode_frame(h, frm_buf, bs_buf,
					      &result->bs_size)) {
				mtk_vcodec_err(h,
					       "h264_encode_frame failed");
				ret = -EINVAL;
				goto encode_err;
			}
		}
		result->is_key_frm = h->is_key_frm;
		break;

	default:
		mtk_vcodec_err(h, "venc_start_opt %d not supported",
			       opt);
		ret = -EINVAL;
		break;
	}

encode_err:
	if (ret)
		result->msg = VENC_MESSAGE_ERR;
	else
		result->msg = VENC_MESSAGE_OK;

	mtk_vcodec_debug(h, "opt %d <-", opt);
	return ret;
}

int h264_enc_set_param(unsigned long handle,
		       enum venc_set_param_type type, void *in)
{
	int ret = 0;
	struct venc_h264_handle *h = (struct venc_h264_handle *)handle;
	struct venc_enc_prm *enc_prm;

	mtk_vcodec_debug(h, "->type=%d", type);

	switch (type) {
	case VENC_SET_PARAM_ENC:
		enc_prm = in;
		if (h264_enc_vpu_set_param(h,
					   VENC_SET_PARAM_ENC,
					   enc_prm)) {
			ret = -EINVAL;
			break;
		}
		if (h->work_buf_alloc == 0) {
			if (h264_alloc_work_buf(h)) {
				mtk_vcodec_err(h,
					       "h264_alloc_work_buf failed");
				ret = -ENOMEM;
			} else {
				h->work_buf_alloc = 1;
			}
		}
		break;

	case VENC_SET_PARAM_FORCE_INTRA:
		if (h264_enc_vpu_set_param(h,
					   VENC_SET_PARAM_FORCE_INTRA, 0)) {
			mtk_vcodec_err(h, "force intra failed");
			ret = -EINVAL;
		}
		break;

	case VENC_SET_PARAM_ADJUST_BITRATE:
		enc_prm = in;
		if (h264_enc_vpu_set_param(h, VENC_SET_PARAM_ADJUST_BITRATE,
					   &enc_prm->bitrate)) {
			mtk_vcodec_err(h, "adjust bitrate failed");
			ret = -EINVAL;
		}
		break;

	case VENC_SET_PARAM_ADJUST_FRAMERATE:
		enc_prm = in;
		if (h264_enc_vpu_set_param(h, VENC_SET_PARAM_ADJUST_FRAMERATE,
					   &enc_prm->frm_rate)) {
			mtk_vcodec_err(h, "adjust frame rate failed");
			ret = -EINVAL;
		}
		break;

	case VENC_SET_PARAM_I_FRAME_INTERVAL:
		if (h264_enc_vpu_set_param(h,
					   VENC_SET_PARAM_I_FRAME_INTERVAL,
					   in)) {
			mtk_vcodec_err(h, "set I frame interval failed");
			ret = -EINVAL;
		}
		break;

	case VENC_SET_PARAM_SKIP_FRAME:
		if (h264_enc_vpu_set_param(h, VENC_SET_PARAM_SKIP_FRAME, 0)) {
			mtk_vcodec_err(h, "skip frame failed");
			ret = -EINVAL;
		}
		break;

	case VENC_SET_PARAM_PREPEND_HEADER:
		h->prepend_hdr = 1;
		mtk_vcodec_debug(h, "set prepend header mode");
		break;

	default:
		mtk_vcodec_err(h, "type %d not supported", type);
		ret = -EINVAL;
		break;
	}

	mtk_vcodec_debug_leave(h);
	return ret;
}

int h264_enc_deinit(unsigned long handle)
{
	int ret = 0;
	struct venc_h264_handle *h = (struct venc_h264_handle *)handle;

	mtk_vcodec_debug_enter(h);

	if (h264_enc_vpu_deinit(h)) {
		mtk_vcodec_err(h, "h264_enc_vpu_deinit failed");
		ret = -EINVAL;
	}

	if (h->work_buf_alloc)
		h264_free_work_buf(h);

	mtk_vcodec_debug_leave(h);
	kfree(h);

	return ret;
}

struct venc_common_if venc_h264_if = {
	h264_enc_init,
	h264_enc_encode,
	h264_enc_set_param,
	h264_enc_deinit,
};

struct venc_common_if *get_h264_enc_comm_if(void)
{
	return &venc_h264_if;
}
