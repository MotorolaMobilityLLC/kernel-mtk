/*
 * Copyright (c) 2015 MediaTek Inc.
 * Author: Houlong Wei <houlong.wei@mediatek.com>
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

#ifndef __MTK_VCODEC_VDEC_CONV_IPI_H__
#define __MTK_VCODEC_VDEC_CONV_IPI_H__


enum vdec_conv_ipi_msgid {
	AP_VDEC_CONV_FMT_VDEC		= 0x00000000,
	AP_VDEC_CONV_FMT_JPEG		= 0x00000001,

	AP_VDEC_CONV_GET		= 0x00D00000,
	AP_VDEC_CONV_RELEASE		= 0x00D00100,
	AP_VDEC_CONV_VDEC		= 0x00D00200,
	AP_VDEC_CONV_JPEG		= 0x00D00300,

	VPU_VDEC_CONV_GET_ACK		= 0x00E00000,
	VPU_VDEC_CONV_RELEASE_ACK	= 0x00E00100,
	VPU_VDEC_CONV_VDEC_ACK		= 0x00E00200,
	VPU_VDEC_CONV_JPEG_ACK		= 0x00E00300
};

enum vdec_conv_ipi_msg_status {
	VDEC_CONV_IPIMSG_STATUS_OK	= 0,
	VDEC_CONV_IPIMSG_STATUS_FAIL	= -1,
	VDEC_CONV_IPIMSG_TIMEOUT	= -2
};

struct vdec_conv_ipi_get {
	uint32_t msg_id;
	uint32_t alignment;
	uint64_t drv_handle;
};

struct vdec_conv_ipi_get_ack {
	uint32_t msg_id;
	int32_t status;
	uint64_t drv_handle;
	uint32_t vpu_handle;
	uint32_t shmem_addr;
};

struct vdec_conv_ipi_release {
	uint32_t msg_id;
	uint32_t vpu_handle;
	uint64_t drv_handle;
};


struct vdec_conv_ipi_release_ack {
	uint32_t msg_id;
	int32_t status;
	uint64_t drv_handle;
};

struct vdec_conv_ipi_convert {
	uint32_t msg_id;
	uint32_t vpu_handle;
	uint64_t drv_handle;
};

struct vdec_conv_ipi_convert_ack {
	uint32_t msg_id;
	int32_t status;
	uint64_t drv_handle;
};

struct vdec_conv_fmt_param {
	int32_t src_w;
	int32_t src_h;
	uint64_t src_addr[3];
	uint32_t src_plane_size[3];
	int32_t src_plane_num;
	/* src pitch used for scan line format */
	int32_t src_y_pitch;
	int32_t src_uv_pitch;
	/* ROI: region of interesting */
	int32_t src_roi_x;
	int32_t src_roi_y;
	int32_t src_roi_w;
	int32_t src_roi_h;

	int32_t dst_w;
	int32_t dst_h;
	uint64_t dst_addr[3];
	uint32_t dst_plane_size[3];
	int32_t dst_plane_num;
};

#endif

