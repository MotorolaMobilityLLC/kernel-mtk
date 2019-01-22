/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef __VPU_DRV_H__
#define __VPU_DRV_H__

#include <linux/types.h>

#define VPU_MAX_NUM_PORTS 32
#define VPU_MAX_NUM_PROPS 32
typedef uint8_t vpu_id_t;

/* the last byte of string must be '/0' */
typedef char vpu_name_t[32];


/*---------------------------------------------------------------------------*/
/*  VPU Property                                                             */
/*---------------------------------------------------------------------------*/
enum vpu_prop_type {
	VPU_PROP_TYPE_CHAR,
	VPU_PROP_TYPE_INT32,
	VPU_PROP_TYPE_INT64,
	VPU_PROP_TYPE_FLOAT,
	VPU_PROP_TYPE_DOUBLE,
	VPU_NUM_PROP_TYPES
};

enum vpu_prop_access {
	VPU_PROP_ACCESS_RDONLY,
	VPU_PROP_ACCESS_RDWR
};

/*
 * The description of properties contains the information about property values,
 * which are stored as compact memory. With the offset, it can get the specific value
 * from compact data.
 *
 * The example of struct vpu_prop_desc is as follows:
 *   +--------+---------------------+--------+--------+-------+--------+
 *   |   id   | name                | offset | type   | count | access |
 *   +--------+---------------------+--------+--------+-------+--------+
 *   |   0    | reserved            | 0      | int32  | 64    | RDONLY |
 *   +--------+---------------------+--------+--------+-------+--------+
 *   |   1    | candidateSize       | 256    | int32  | 2     | RDWR   |
 *   +--------+---------------------+--------+--------+-------+--------+
 *
 * Use a buffer to store all property data, which is a compact-format data.
 * The buffer's layout is described by prop_desc, using the offset could get the specific data.
 *
 * The example of compact-format memory is as follows:
 *   +--------+--------+--------+--------+--------+--------+--------+--------+
 *   |    0   |    8   |   16   |   24   |   32   |   40   |   48   |   56   |
 *   +--------+--------+--------+--------+--------+--------+--------+--------+
 *   | alg-ver|       working-buffer-size         | cnt-mem|                 |
 *   +--------+--------+--------+--------+--------+--------+--------+--------+
 *
 */
struct vpu_prop_desc {
	vpu_id_t id;
	vpu_name_t name;
	/* offset = previous offset + previous size */
	uint32_t offset;
	/* size = sizeof(type) x count */
	enum vpu_prop_type type;
	uint32_t count;
	/* directional data exchange*/
	enum vpu_prop_access access;
};

/*---------------------------------------------------------------------------*/
/*  VPU Ports                                                                */
/*---------------------------------------------------------------------------*/
enum vpu_buf_type {
	VPU_BUF_TYPE_IMAGE,
	VPU_BUF_TYPE_DATA,
	VPU_NUM_BUF_TYPES
};

enum vpu_port_dir {
	VPU_PORT_DIR_IN,
	VPU_PORT_DIR_OUT,
	VPU_PORT_DIR_IN_OUT,
	VPU_NUM_PORT_DIRS
};

/*
 * The ports contains the information about algorithm's input and output.
 * The each buffer on the vpu_request should be assigned a port id,
 * to let algorithm recognize every buffer's purpose.
 *
 * The example of vpu_port table is as follows:
 *   +--------+---------------------+--------+--------+
 *   |   id   | name                | type   | dir    |
 *   +--------+---------------------+--------+--------+
 *   |   0    | image-in            | IMAGE  | IN     |
 *   +--------+---------------------+--------+--------+
 *   |   1    | data-in             | DATA   | IN     |
 *   +--------+---------------------+--------+--------+
 *   |   2    | image-out           | IMAGE  | OUT    |
 *   +--------+---------------------+--------+--------+
 *   |   3    | image-temp          | IMAGE  | INOUT  |
 *   +--------+---------------------+--------+--------+
 *
 */
struct vpu_port {
	vpu_id_t id;
	vpu_name_t name;
	enum vpu_buf_type type;
	enum vpu_port_dir dir;
};

/*---------------------------------------------------------------------------*/
/*  VPU Algo                                                                 */
/*---------------------------------------------------------------------------*/
struct vpu_algo {
	vpu_id_t id;
	vpu_name_t name;
	/* the pointer to property data buffer */
	uint64_t info_ptr;
	/* the size of property data buffer */
	uint32_t info_length;
	uint8_t info_desc_count;
	struct vpu_prop_desc info_descs[VPU_MAX_NUM_PROPS];
	uint32_t sett_length;
	uint8_t sett_desc_count;
	struct vpu_prop_desc sett_descs[VPU_MAX_NUM_PROPS];
	uint8_t port_count;
	struct vpu_port ports[VPU_MAX_NUM_PORTS];
	/* mva of kernel loader, the address is VPU accessible */
	uint64_t bin_ptr;
	uint32_t bin_length;
};

/*---------------------------------------------------------------------------*/
/*  VPU Register                                                             */
/*---------------------------------------------------------------------------*/
struct vpu_reg_value {
	uint32_t field;
	uint32_t value;
};

struct vpu_reg_values {
	struct vpu_reg_value *regs;
	uint8_t reg_count;
};


/*---------------------------------------------------------------------------*/
/*  VPU Power                                                                */
/*---------------------------------------------------------------------------*/
struct vpu_power {
	uint32_t freq;
	uint32_t power;
};


/*---------------------------------------------------------------------------*/
/*  VPU Plane                                                                */
/*---------------------------------------------------------------------------*/
struct vpu_plane {
	/* if buffer type is image */
	uint32_t stride;
	/* mva, the address is VPU accessible */
	uint64_t ptr;
	uint32_t length;
};

enum vpu_buf_format {
	VPU_BUF_FORMAT_DATA,
	VPU_BUF_FORMAT_IMG_Y8,
	VPU_BUF_FORMAT_IMG_YV12,
	VPU_BUF_FORMAT_IMG_NV21,
	VPU_BUF_FORMAT_IMG_YUY2,
};

struct vpu_buffer {
	uint8_t plane_count;
	struct vpu_plane planes[3];
	enum vpu_buf_format format;
	uint32_t width;
	uint32_t height;
	vpu_id_t port_id;
};


enum vpu_req_status {
	VPU_REQ_STATUS_SUCCESS,
	VPU_REQ_STATUS_BUSY,
	VPU_REQ_STATUS_TIMEOUT,
	VPU_REQ_STATUS_INVALID,
	VPU_REQ_STATUS_FLUSH,
	VPU_REQ_STATUS_FAILURE,
};

struct vpu_request {
	uint8_t buffer_count;
	struct vpu_buffer buffers[VPU_MAX_NUM_PORTS];
	vpu_id_t algo_id;
	enum vpu_req_status status;
	/* pointer to the request setting */
	uint64_t sett_ptr;
	uint32_t sett_length;
	/* reserved for user */
	uint64_t priv;
};

/*---------------------------------------------------------------------------*/
/*  IOCTL Command                                                            */
/*---------------------------------------------------------------------------*/
#define VPU_MAGICNO             'v'
#define VPU_IOCTL_SET_POWER         _IOW(VPU_MAGICNO,   0, int)
#define VPU_IOCTL_ENQUE_REQUEST     _IOW(VPU_MAGICNO,   1, int)
#define VPU_IOCTL_DEQUE_REQUEST     _IOWR(VPU_MAGICNO,  2, int)
#define VPU_IOCTL_FLUSH_REQUEST     _IOW(VPU_MAGICNO,   3, int)
#define VPU_IOCTL_LOAD_ALG          _IOW(VPU_MAGICNO,   4, int)
#define VPU_IOCTL_GET_ALGO_INFO     _IOWR(VPU_MAGICNO,  5, int)
#define VPU_IOCTL_REG_WRITE         _IOW(VPU_MAGICNO,   6, int)
#define VPU_IOCTL_REG_READ          _IOWR(VPU_MAGICNO,  7, int)


int vpu_set_power(struct vpu_power *power);

int vpu_write_register(struct vpu_reg_values regs);

#endif
