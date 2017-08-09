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

#ifndef _VENC_DRV_IF_H_
#define _VENC_DRV_IF_H_

#include "mtk_vcodec_util.h"

/*
 * enum venc_yuv_fmt - The type of input yuv format
 * (VPU related: If you change the order, you must also update the VPU codes.)
 * @VENC_YUV_FORMAT_NONE: Default value (not used)
 * @VENC_YUV_FORMAT_GRAY: GRAY YUV format
 * @VENC_YUV_FORMAT_422: 422 YUV format
 * @VENC_YUV_FORMAT_420: 420 YUV format
 * @VENC_YUV_FORMAT_411: 411 YUV format
 * @VENC_YUV_FORMAT_YV12: YV12 YUV format
 * @VENC_YUV_FORMAT_NV12: NV12 YUV format
 * @VENC_YUV_FORMAT_NV21: NV21 YUV format
 * @VENC_YUV_FORMAT_BLK16X32: Block 16x32 YUV format
 * @VENC_YUV_FORMAT_BLK64X32: Block 64x32 YUV format
 * @VENC_YUV_FORMAT_YV12_1688: YV12 YUV format
 */
enum venc_yuv_fmt {
	VENC_YUV_FORMAT_NONE,
	VENC_YUV_FORMAT_GRAY,
	VENC_YUV_FORMAT_422,
	VENC_YUV_FORMAT_420,
	VENC_YUV_FORMAT_411,
	VENC_YUV_FORMAT_YV12,
	VENC_YUV_FORMAT_NV12,
	VENC_YUV_FORMAT_NV21,
	VENC_YUV_FORMAT_BLK16X32,
	VENC_YUV_FORMAT_BLK64X32,
	VENC_YUV_FORMAT_YV12_1688,
};

/*
 * enum venc_start_opt - encode frame option used in venc_if_encode()
 * @VENC_START_OPT_ENCODE_SEQUENCE_HEADER: encode SPS/PPS for H264
 * @VENC_START_OPT_ENCODE_FRAME: encode normal frame
 */
enum venc_start_opt {
	VENC_START_OPT_ENCODE_SEQUENCE_HEADER,
	VENC_START_OPT_ENCODE_FRAME,
};

/*
 * enum venc_drv_msg - The type of encode frame status used in venc_if_encode()
 * @VENC_MESSAGE_OK: encode ok
 * @VENC_MESSAGE_ERR: encode error
 * @VENC_MESSAGE_PARTIAL: encode partial frame (ok means EOF)
 */
enum venc_drv_msg {
	VENC_MESSAGE_OK,
	VENC_MESSAGE_ERR,
	VENC_MESSAGE_PARTIAL,
};

/*
 * enum venc_set_param_type - The type of set parameter used in venc_if_set_param()
 * (VPU related: If you change the order, you must also update the VPU codes.)
 * @VENC_SET_PARAM_ENC: set encoder parameters
 * @VENC_SET_PARAM_FORCE_INTRA: set force intra frame
 * @VENC_SET_PARAM_ADJUST_BITRATE: set to adjust bitrate (in bps)
 * @VENC_SET_PARAM_ADJUST_FRAMERATE: set frame rate
 * @VENC_SET_PARAM_I_FRAME_INTERVAL: set I frame interval
 * @VENC_SET_PARAM_SKIP_FRAME: set H264 skip one frame
 * @VENC_SET_PARAM_PREPEND_HEADER: set H264 prepend SPS/PPS before IDR
 * @VENC_SET_PARAM_TS_MODE: set VP8 temporal scalability mode
 * @VENC_SET_PARAM_WFD_MODE: set H264 wifi-display mode
 * @VENC_SET_PARAM_NON_REF_P: set H264 non reference P frame mode
 */
enum venc_set_param_type {
	VENC_SET_PARAM_ENC,
	VENC_SET_PARAM_FORCE_INTRA,
	VENC_SET_PARAM_ADJUST_BITRATE,
	VENC_SET_PARAM_ADJUST_FRAMERATE,
	VENC_SET_PARAM_I_FRAME_INTERVAL,
	VENC_SET_PARAM_SKIP_FRAME,
	VENC_SET_PARAM_PREPEND_HEADER,
	VENC_SET_PARAM_TS_MODE,
	VENC_SET_PARAM_WFD_MODE,
	VENC_SET_PARAM_NON_REF_P,
};

/*
 * struct venc_enc_prm - encoder settings for VENC_SET_PARAM_ENC used in venc_if_set_param()
 * @yuv_fmt: input yuv format
 * @h264_profile: V4L2 defined H.264 profile
 * @h264_level: V4L2 defined H.264 level
 * @width: image width
 * @height: image height
 * @buf_width: buffer width
 * @buf_height: buffer height
 * @frm_rate: frame rate
 * @intra_period: intra frame period
 * @bitrate: target bitrate in kbps
 */
struct venc_enc_prm {
	enum venc_yuv_fmt yuv_fmt;
	unsigned int h264_profile;
	unsigned int h264_level;
	unsigned int width;
	unsigned int height;
	unsigned int buf_width;
	unsigned int buf_height;
	unsigned int frm_rate;
	unsigned int intra_period;
	unsigned int bitrate;
};

/*
 * struct venc_frm_buf - frame buffer information used in venc_if_encode()
 * @fb_addr: plane 0 frame buffer address
 * @fb_addr1: plane 1 frame buffer address
 * @fb_addr2: plane 2 frame buffer address
 */
struct venc_frm_buf {
	struct mtk_vcodec_mem fb_addr;
	struct mtk_vcodec_mem fb_addr1;
	struct mtk_vcodec_mem fb_addr2;
};

/*
 * struct venc_done_result - This is return information used in venc_if_encode()
 * @msg: message, such as success or error code
 * @bs_size: output bitstream size
 * @is_key_frm: output is key frame or not
 */
struct venc_done_result {
	enum venc_drv_msg msg;
	unsigned int bs_size;
	bool is_key_frm;
};

/*
 * venc_if_create - Create the driver handle
 * @ctx: device context
 * @fourcc: encoder output format
 * @handle: driver handle
 * Return: 0 if creating handle successfully, otherwise it is failed.
 */
int venc_if_create(void *ctx, unsigned int fourcc, unsigned long *handle);

/*
 * venc_if_release - Release the driver handle
 * @handle: driver handle
 * Return: 0 if releasing handle successfully, otherwise it is failed.
 */
int venc_if_release(unsigned long handle);

/*
 * venc_if_init - Init the driver setting, alloc working memory ... etc.
 * @handle: driver handle
 * Return: 0 if init handle successfully, otherwise it is failed.
 */
int venc_if_init(unsigned long handle);

/*
 * venc_if_deinit - DeInit the driver setting, free working memory ... etc.
 * @handle: driver handle
 * Return: 0 if deinit handle successfully, otherwise it is failed.
 */
int venc_if_deinit(unsigned long handle);

/*
 * venc_if_set_param - Set parameter to driver
 * @handle: driver handle
 * @type: set type
 * @in: input parameter
 * @out: output parameter
 * Return: 0 if setting param successfully, otherwise it is failed.
 */
int venc_if_set_param(unsigned long handle,
		      enum venc_set_param_type type,
		      void *in);

/*
 * venc_if_encode - Encode frame
 * @handle: driver handle
 * @opt: encode frame option
 * @frm_buf: input frame buffer information
 * @bs_buf: output bitstream buffer infomraiton
 * @result: encode result
 * Return: 0 if encoding frame successfully, otherwise it is failed.
 */
int venc_if_encode(unsigned long handle,
		   enum venc_start_opt opt,
		   struct venc_frm_buf *frm_buf,
		   struct mtk_vcodec_mem *bs_buf,
		   struct venc_done_result *result);

#endif /* _VENC_DRV_IF_H_ */
