/*
 * Copyright (c) 2015 MediaTek Inc.
 * Author: Daniel Hsiao <daniel.hsiao@mediatek.com>
 *             Kai-Sean Yang <kai-sean.yang@mediatek.com>
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

#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/delay.h>
#include <linux/time.h>

#include "vdec_drv_vp9_if.h"
#include "vdec_drv_vp9.h"
#include "vdec_drv_vp9_utility.h"

static void init_list(struct vdec_vp9_inst *inst)
{
	int i;

	INIT_LIST_HEAD(&inst->available_fb_node_list);
	INIT_LIST_HEAD(&inst->fb_use_list);
	INIT_LIST_HEAD(&inst->fb_free_list);
	INIT_LIST_HEAD(&inst->fb_disp_list);

	for (i = 0; i < VP9_MAX_FRM_BUFF_NODE_NUM; i++) {
		INIT_LIST_HEAD(&inst->dec_fb[i].list);
		inst->dec_fb[i].fb = NULL;
		list_add_tail(&inst->dec_fb[i].list, &inst->available_fb_node_list);
	}
}


static void get_pic_info(struct vdec_vp9_inst *inst, struct vdec_pic_info *pic)
{
#ifdef VDEC_UFO_SUPPORT
	unsigned int pic_w = ((inst->frm_hdr.width + 63) >> 6) << 6;
	unsigned int pic_h = ((inst->frm_hdr.height + 63) >> 6) << 6;

#ifdef Y_C_SEPARATE
	unsigned int pic_sz_y = ((pic_w * pic_h + 511) >> 9) << 9;  /* 512 alignment */
	unsigned int pic_sz_y_bs = pic_sz_y;
#else
	unsigned int pic_sz_y = pic_w * pic_h;
	unsigned int pic_sz_y_bs = (((pic_sz_y + 4095) >> 12) << 12);  /* 4K alignement */
#endif
	unsigned int pic_sz_c = pic_sz_y >> 1;
	unsigned int pic_sz_c_bs = pic_sz_c;
	unsigned int ufo_len_sz_y = ((((pic_sz_y + 255) >> 8) + 63 + (16*8)) >> 6) << 6;
	unsigned int ufo_len_sz_c = (((ufo_len_sz_y >> 1) + 15 + (16 * 8)) >> 4) << 4;

	pic->y_bs_sz = pic_sz_y_bs;
	pic->c_bs_sz = pic_sz_c_bs;
	pic->y_len_sz = ufo_len_sz_y;
	pic->c_len_sz = ufo_len_sz_c;

	inst->work_buf.pic_sz_y_bs = pic_sz_y_bs;
	inst->work_buf.pic_sz_c_bs = pic_sz_c_bs;
	inst->work_buf.ufo_len_sz_y = ufo_len_sz_y;
	inst->work_buf.ufo_len_sz_c = ufo_len_sz_c;
#else
	pic->y_bs_sz = inst->work_buf.frmbuf_width *
		inst->work_buf.frmbuf_height;
	pic->c_bs_sz = pic->y_bs_sz >> 1;
	pic->y_len_sz = 0;
	pic->c_len_sz = 0;
#endif

	pic->pic_w = inst->frm_hdr.width;
	pic->pic_h = inst->frm_hdr.height;
	pic->buf_w = inst->work_buf.frmbuf_width;
	pic->buf_h = inst->work_buf.frmbuf_height;
}

static void get_disp_fb(struct vdec_vp9_inst *inst, struct vdec_fb **out_fb)
{
#if VP9_DECODE_BENCHMARK
	struct timeval begin, end;

	do_gettimeofday(&begin);
#endif

	mtk_vcodec_debug_enter(inst);

	*out_fb = vp9_rm_from_fb_disp_list(inst);
	if (*out_fb)
		(*out_fb)->status |= FB_ST_DISPLAY;

#if VP9_DECODE_BENCHMARK
	do_gettimeofday(&end);
	inst->total_get_dispbuf_time += ((end.tv_sec - begin.tv_sec) *
		1000000 + end.tv_usec - begin.tv_usec);
#endif
}

static void get_free_fb(struct vdec_vp9_inst *inst, struct vdec_fb **out_fb)
{
	struct vdec_fb_node *node;
	struct vdec_fb *fb = NULL;

#if VP9_DECODE_BENCHMARK
	struct timeval begin, end;

	do_gettimeofday(&begin);
#endif

	node = list_first_entry_or_null(&inst->fb_free_list,
					struct vdec_fb_node, list);
	if (node) {
		list_move_tail(&node->list, &inst->available_fb_node_list);
		fb = (struct vdec_fb *)node->fb;
		fb->status |= FB_ST_FREE;
		mtk_vcodec_debug(inst, "[FB] get free fb %p st=%d",
				 node->fb, fb->status);
	} else {
		fb = NULL;
		mtk_vcodec_debug(inst, "[FB] there is no free fb");
	}

	*out_fb = fb;

#if VP9_DECODE_BENCHMARK
	do_gettimeofday(&end);
	inst->total_get_freebuf_time += ((end.tv_sec - begin.tv_sec) *
		1000000 + end.tv_usec - begin.tv_usec);
#endif
}

static int vdec_vp9_init(struct mtk_vcodec_ctx *ctx, struct mtk_vcodec_mem *bs,
		 unsigned long *h_vdec, struct vdec_pic_info *pic_info)
{
	struct vdec_vp9_inst *inst;
	unsigned int ipi_data[3];
#if VP9_DECODE_BENCHMARK
	struct timeval begin, end;

	do_gettimeofday(&begin);
#endif

	if ((bs == NULL) || (pic_info == NULL))
		return -EINVAL;

	inst = vp9_alloc_inst(ctx);
	if (!inst)
		return -ENOMEM;

	inst->frm_cnt = 0;
	inst->total_frm_cnt = 0;
	inst->ctx = ctx;
	inst->dev = mtk_vcodec_get_plat_dev(ctx);
	(*h_vdec) = (unsigned long)inst;

	mtk_vcodec_debug(inst, "Input BS Size = %zd\n", bs->size);

	ipi_data[0] = *((unsigned int *)bs->va);
	ipi_data[1] = *((unsigned int *)(bs->va + 4));
	ipi_data[2] = *((unsigned int *)(bs->va + 8));
	if (0 != vp9_dec_vpu_init(inst, (unsigned int *)ipi_data, 3)) {
		mtk_vcodec_err(inst, "[E]vp9_dec_vpu_init -");
		return -EINVAL;
	}

	if (vp9_get_hw_reg_base(inst) != true) {
		mtk_vcodec_err(inst, "vp9_get_hw_reg_base");
		return -EINVAL;
	}

	if (vp9_init_proc(inst, pic_info) != true) {
		mtk_vcodec_err(inst, "vp9_init_proc");
		return -EINVAL;
	}

	get_pic_info(inst, pic_info);
	init_list(inst);

	if (vp9_alloc_work_buf(inst) != true) {
		mtk_vcodec_err(inst, "vp9_alloc_work_buf");
		return -EINVAL;
	}

#if (VP9_CRC_ENABLE || VP9_DECODE_BENCHMARK)
	vp9_dec_open_log_file(inst);
#endif

#if VP9_DECODE_BENCHMARK
	do_gettimeofday(&end);
	inst->total_init_time += ((end.tv_sec - begin.tv_sec) * 1000000 +
		end.tv_usec - begin.tv_usec);
#endif

	mtk_vcodec_debug_leave(inst);

	return 0;
}

static int vdec_vp9_decode(unsigned long h_vdec, struct mtk_vcodec_mem *bs,
		   struct vdec_fb *fb, bool *res_chg)
{
	int ret = 0;
	struct vdec_vp9_inst *inst = (struct vdec_vp9_inst *)h_vdec;
	struct vdec_vp9_frm_hdr *frm_hdr = &inst->frm_hdr;
#if VP9_DECODE_BENCHMARK
	struct timeval begin, end, begin2, end2;

	do_gettimeofday(&begin);
#endif
	mtk_vcodec_debug_enter(inst);

	*res_chg = false;

	if ((bs == NULL) && (fb == NULL)) {
		mtk_vcodec_debug(inst, "[EOS]");
		vp9_reset(inst);
		return ret;
	}

	if (bs != NULL) {
		mtk_vcodec_debug(inst, "Input BS Size = %zd", bs->size);
#if VP9_DECODE_BENCHMARK
		inst->total_input_bs_size += bs->size;
#endif
	}

#if VP9_RES_CHG_FLOW
	memcpy((void *)inst + sizeof(*inst), (void *)inst,
	       sizeof(*inst));
#endif

	while (1) {
#if VP9_RES_CHG_FLOW
		frm_hdr->resolution_changed = false;
#endif

#if VP9_DECODE_BENCHMARK
		do_gettimeofday(&begin2);
#endif
		if (vp9_dec_proc(inst, bs, fb) != true) {
			mtk_vcodec_err(inst, "vp9_dec_proc");
			ret = -EINVAL;
#if VP9_DECODE_BENCHMARK
			do_gettimeofday(&end2);
			inst->total_vp9_dec_time +=
				((end2.tv_sec - begin2.tv_sec) * 1000000 + end2.tv_usec - begin2.tv_usec);
#endif
			goto DECODE_ERROR;
		}
#if VP9_DECODE_BENCHMARK
		do_gettimeofday(&end2);
		inst->total_vp9_dec_time +=
			((end2.tv_sec - begin2.tv_sec) * 1000000 + end2.tv_usec - begin2.tv_usec);
#endif

#if VP9_RES_CHG_FLOW
		if (frm_hdr->resolution_changed) {
			unsigned int width = inst->frm_hdr.width;
			unsigned int height = inst->frm_hdr.height;
			unsigned int frmbuf_width =
				inst->work_buf.frmbuf_width;
			unsigned int frmbuf_height =
				inst->work_buf.frmbuf_height;
			struct mtk_vcodec_mem tmp_buf =	inst->work_buf.mv_buf;
			struct vp9_dram_buf tmp_buf2 = inst->vpu.drv->mv_buf;
#if VP9_SUPER_FRAME_SUPPORT
#ifdef Y_C_SEPARATE
			struct vdec_fb tmp_buf3 =
#else
			struct mtk_vcodec_mem tmp_buf3 =
#endif
				inst->work_buf.sf_ref_buf[0];
#endif

			memcpy((void *)inst, (void *)inst + sizeof(*inst),
			       sizeof(*inst));

			inst->frm_hdr.width = width;
			inst->frm_hdr.height = height;
			inst->work_buf.frmbuf_width = frmbuf_width;
			inst->work_buf.frmbuf_height = frmbuf_height;
			inst->work_buf.mv_buf = tmp_buf;
			inst->vpu.drv->mv_buf = tmp_buf2;
#if VP9_SUPER_FRAME_SUPPORT
			inst->work_buf.sf_ref_buf[0] = tmp_buf3;
#endif

			*res_chg = true;
			mtk_vcodec_debug(inst, "VDEC_ST_RESOLUTION_CHANGED");
			vp9_add_to_fb_free_list(inst, fb);
			ret = 0;
			goto DECODE_ERROR;
		}
#endif

		/* ToDo:  Resolution Change flow during Super Frame? */
#if VP9_DECODE_BENCHMARK
		do_gettimeofday(&begin2);
#endif
		if (vp9_check_proc(inst) != true) {
			mtk_vcodec_err(inst, "vp9_check_proc");
			ret = -EINVAL;
#if VP9_DECODE_BENCHMARK
			do_gettimeofday(&end2);
			inst->total_vp9_check_time +=
				((end2.tv_sec - begin2.tv_sec) * 1000000 + end2.tv_usec - begin2.tv_usec);
#endif
			goto DECODE_ERROR;
		}
#if VP9_DECODE_BENCHMARK
		do_gettimeofday(&end2);
		inst->total_vp9_check_time +=
			((end2.tv_sec - begin2.tv_sec) * 1000000 + end2.tv_usec - begin2.tv_usec);
#endif

		inst->total_frm_cnt++;

		if (vp9_is_last_sub_frm(inst))
			break;

#if VP9_RES_CHG_FLOW
		/* for resolution change backup */
		memcpy((void *)inst + sizeof(*inst), (void *)inst,
		       sizeof(*inst));
#endif
	}
	inst->frm_cnt++;

DECODE_ERROR:
	if (ret < 0)
		vp9_add_to_fb_free_list(inst, fb);

#if VP9_DECODE_BENCHMARK
	do_gettimeofday(&end);
	inst->total_decode_time +=
		((end.tv_sec - begin.tv_sec) * 1000000 + end.tv_usec -
		 begin.tv_usec);
#endif

	mtk_vcodec_debug_leave(inst);

	return ret;
}


static int vdec_vp9_get_param(unsigned long h_vdec,
			enum vdec_get_param_type type, void *out)
{
	struct vdec_vp9_inst *inst = (struct vdec_vp9_inst *)h_vdec;
	int ret = 0;

#if VP9_DECODE_BENCHMARK
	struct timeval begin, end;

	do_gettimeofday(&begin);
#endif

	mtk_vcodec_debug_enter(inst);

	switch (type) {
	case GET_PARAM_DISP_FRAME_BUFFER:
		get_disp_fb(inst, out);
		break;

	case GET_PARAM_FREE_FRAME_BUFFER:
		get_free_fb(inst, out);
		break;

	case GET_PARAM_PIC_INFO:
		get_pic_info(inst, out);
		break;

	case GET_PARAM_DPB_SIZE:
		*((unsigned int *)out) = 9;
		break;

	default:
		mtk_vcodec_err(inst, "not support type %d", type);
		ret = -EINVAL;
		break;
	}

	mtk_vcodec_debug_leave(inst);

#if VP9_DECODE_BENCHMARK
	do_gettimeofday(&end);
	inst->total_get_param_time += ((end.tv_sec - begin.tv_sec) * 1000000 +
		end.tv_usec - begin.tv_usec);
#endif

	return ret;
}

static int vdec_vp9_deinit(unsigned long h_vdec)
{
	int ret = 0;
	struct vdec_vp9_inst *inst = (struct vdec_vp9_inst *)h_vdec;
#if VP9_DECODE_BENCHMARK
	struct timeval begin, end;
	unsigned long total_elapsed_time;

	do_gettimeofday(&begin);
#endif

	mtk_vcodec_debug_enter(inst);

	if (0 != vp9_dec_vpu_deinit(inst)) {
		mtk_vcodec_err(inst, "[E]vp9_dec_vpu_deinit");
		ret = -EINVAL;
	}

	if (vp9_free_work_buf(inst) != true) {
		mtk_vcodec_err(inst, "vp9_free_work_buf");
		ret = -EINVAL;
	}

#if VP9_DECODE_BENCHMARK
	do_gettimeofday(&end);
	inst->total_deinit_time += ((end.tv_sec - begin.tv_sec) * 1000000 +
		end.tv_usec - begin.tv_usec);
	total_elapsed_time = (inst->total_init_time +
		inst->total_decode_time + inst->total_get_dispbuf_time +
		inst->total_get_freebuf_time + inst->total_get_param_time +
		inst->total_set_param_time + inst->total_deinit_time);
	vp9_dec_fprintf(inst->log, "frm_cnt: %d\n", inst->frm_cnt);
	vp9_dec_fprintf(inst->log, "width: %d/%d height: %d/%d\n",
			inst->frm_hdr.width, inst->work_buf.frmbuf_width,
			inst->frm_hdr.height, inst->work_buf.frmbuf_height);
	vp9_dec_fprintf(inst->log, "total_time: %ld us\n",
			total_elapsed_time);
	vp9_dec_fprintf(inst->log, "  init_time: %ld us\n",
			inst->total_init_time);
	vp9_dec_fprintf(inst->log, "  decode_time: %ld us\n",
			inst->total_decode_time);
	vp9_dec_fprintf(inst->log, "	    vp9_dec_time: %ld us\n", inst->total_vp9_dec_time);
	vp9_dec_fprintf(inst->log, "	    vp9_check_time: %ld us\n", inst->total_vp9_check_time);
	vp9_dec_fprintf(inst->log, "  get_dispbuf_time: %ld us\n",
			inst->total_get_dispbuf_time);
	vp9_dec_fprintf(inst->log, "  get_freebuf_time: %ld us\n",
			inst->total_get_freebuf_time);
	vp9_dec_fprintf(inst->log, "  get_param_time: %ld us\n",
			inst->total_get_param_time);
	vp9_dec_fprintf(inst->log, "  set_param_time: %ld us\n",
			inst->total_set_param_time);
	vp9_dec_fprintf(inst->log, "  deinit_time: %ld us\n",
			inst->total_deinit_time);
	vp9_dec_fprintf(inst->log, "FPS: %d.%02d\n",
			(inst->frm_cnt * 1000000L / total_elapsed_time),
			(inst->frm_cnt * 100000000L / total_elapsed_time) -
			((inst->frm_cnt * 1000000L / total_elapsed_time) *
			100));
	vp9_dec_fprintf(inst->log, "Bitrate: %d\n",
			(inst->total_input_bs_size * 8 * 30 /
			inst->frm_cnt));
#endif

#if (VP9_CRC_ENABLE || VP9_DECODE_BENCHMARK)
	vp9_dec_close_log_file(inst);
#endif

	mtk_vcodec_debug_leave(inst);
	vp9_free_handle(inst);

	return ret;
}

static struct vdec_common_if vdec_vp9_if = {
	vdec_vp9_init,
	vdec_vp9_decode,
	vdec_vp9_get_param,
	vdec_vp9_deinit,
};

struct vdec_common_if *get_vp9_dec_comm_if(void)
{
	return &vdec_vp9_if;
}
