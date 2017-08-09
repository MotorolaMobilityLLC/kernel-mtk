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

#ifndef _VDEC_DRV_IF_H_
#define _VDEC_DRV_IF_H_

#include <linux/videodev2.h>

#include "mtk_vcodec_util.h"

enum vdec_fb_status {
	FB_ST_NORMAL		= 0,
	FB_ST_DISPLAY		= (1 << 0),
	FB_ST_FREE		= (1 << 1)
};

enum vdec_get_param_type {
	GET_PARAM_DISP_FRAME_BUFFER,
	GET_PARAM_FREE_FRAME_BUFFER,
	GET_PARAM_PIC_INFO,
	GET_PARAM_CROP_INFO,
	GET_PARAM_DPB_SIZE
};

#ifdef Y_C_SEPARATE
struct vdec_fb {
	struct mtk_vcodec_mem base_y;
	struct mtk_vcodec_mem base_c;
	unsigned int status;
};
#else
struct vdec_fb {
	struct mtk_vcodec_mem base;
	unsigned int buf_w;
	unsigned int buf_h;
	unsigned int status;
};
#endif

struct vdec_fb_node {
	struct list_head list;
	void *fb;
};

/**
 * struct vdec_pic_info  - picture size information
 * @pic_w   : picture width
 * @pic_h   : picture height
 * @buf_w   : picture buffer width (16 aligned up from pic_w)
 * @buf_h   : picture buffer heiht (32 aligned up from pic_h)
 * @y_bs_sz : Y bitstream size
 * @c_bs_sz : CbCr bitstream size
 * @y_len_sz: Y length size
 * @c_len_sz: CbCr length size
 * E.g. suppose picture size is 176x144,
 *      buffer size will be aligned to 176x160.
*/
struct vdec_pic_info {
	unsigned int pic_w;
	unsigned int pic_h;
	unsigned int buf_w;
	unsigned int buf_h;
	unsigned int y_bs_sz;
	unsigned int c_bs_sz;
	unsigned int y_len_sz;
	unsigned int c_len_sz;
};

/**
 * For the same vdec_handle, these functions below are not thread safe.
 * For different vdec_handle, these functions can be called at the same time.
 */

/**
 * vdec_if_create() - create video decode handle according video format
 * @ctx    : [in] v4l2 context
 * @fmt    : [in] video format fourcc, V4L2_PIX_FMT_H264/VP8/VP9..
 * @handle : [out] video decode handle
 */
int vdec_if_create(void *ctx, unsigned int fourcc, unsigned long *handle);

/**
 * vdec_if_release() - release decode driver.
 * @handle : [in] video decode handle to be released
 *
 * need to perform driver deinit before driver release.
 */
void vdec_if_release(unsigned long handle);

/**
 * vdec_if_init() - initialize decode driver
 * @handle   : [in] video decode handle
 * @bs       : [in] input bitstream
 * @pic	     : [out] width and height of bitstream
 */
int vdec_if_init(unsigned long handle, struct mtk_vcodec_mem *bs,
		 struct vdec_pic_info *pic);

/**
 * vdec_if_deinit() - deinitialize driver.
 * @handle : [in] video decode handle to be deinit
 */
int vdec_if_deinit(unsigned long handle);

/**
 * vdec_if_get_param() - get driver's parameter
 * @handle : [in] video decode handle
 * @type   : [in] input parameter type
 * @out    : [out] buffer to store query result
 */
int vdec_if_get_param(unsigned long handle, enum vdec_get_param_type type,
		      void *out);

/**
 * vdec_if_decode() - trigger decode
 * @handle  : [in] video decode handle
 * @bs      : [in] input bitstream
 * @fb      : [in] frame buffer to store decoded frame
 * @res_chg : [out] resolution change happen
 *
 * while EOF flush decode, need to set input bitstream as NULL
 */
int vdec_if_decode(unsigned long handle, struct mtk_vcodec_mem *bs,
		   struct vdec_fb *fb, bool *res_chg);

#endif
