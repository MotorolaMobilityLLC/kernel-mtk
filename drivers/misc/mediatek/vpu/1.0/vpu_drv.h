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
 *   |   0    | algo_version        | 0      | int32  | 1     | RDONLY |
 *   +--------+---------------------+--------+--------+-------+--------+
 *   |   1    | field_1             | 4      | int32  | 2     | RDWR   |
 *   +--------+---------------------+--------+--------+-------+--------+
 *   |   1    | field_2             | 12     | int64  | 1     | RDWR   |
 *   +--------+---------------------+--------+--------+-------+--------+
 *
 * Use a buffer to store all property data, which is a compact-format data.
 * The buffer's layout is described by prop_desc, using the offset could get the specific data.
 *
 * The example of compact-format memory is as follows:
 *   +--------+--------+--------+--------+--------+
 *   |  0~3   |  4~7   |  8~11  |  12~15 |  16~23 |
 *   +--------+--------+--------+--------+--------+
 *   |alg_vers|    field_1      |    field_2      |
 *   +--------+--------+--------+--------+--------+
 *
 */
struct vpu_prop_desc {
	vpu_id_t id;
	uint8_t type;      /* the property's data type */
	uint8_t access;    /* directional data exchange */
	uint32_t offset;   /* offset = previous offset + previous size */
	uint32_t count;    /* size = sizeof(type) x count */
	vpu_name_t name;
};

/*---------------------------------------------------------------------------*/
/*  VPU Ports                                                                */
/*---------------------------------------------------------------------------*/
enum vpu_port_usage {
	VPU_PORT_USAGE_IMAGE,
	VPU_PORT_USAGE_DATA,
	VPU_NUM_PORT_USAGES
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
	uint8_t usage;
	uint8_t dir;
	vpu_name_t name;
};

/*---------------------------------------------------------------------------*/
/*  VPU Algo                                                                 */
/*---------------------------------------------------------------------------*/
struct vpu_algo {
	vpu_id_t id;
	uint8_t port_count;
	uint8_t info_desc_count;
	uint8_t sett_desc_count;
	uint32_t info_length;    /* the size of info data buffer */
	uint32_t sett_length;
	uint32_t bin_length;
	uint64_t info_ptr;       /* the pointer to info data buffer */
	uint64_t bin_ptr;        /* mva of algo bin, which is accessible by VPU */
	vpu_name_t name;
	struct vpu_prop_desc info_descs[VPU_MAX_NUM_PROPS];
	struct vpu_prop_desc sett_descs[VPU_MAX_NUM_PROPS];
	struct vpu_port ports[VPU_MAX_NUM_PORTS];
};

/*---------------------------------------------------------------------------*/
/*  VPU Register                                                             */
/*---------------------------------------------------------------------------*/
struct vpu_reg_value {
	uint32_t field;
	uint32_t value;
};

struct vpu_reg_values {
	uint8_t reg_count;
	struct vpu_reg_value *regs;
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
	uint32_t stride;         /* if buffer type is image */
	uint32_t length;
	uint64_t ptr;            /* mva which is accessible by VPU */
};

enum vpu_buf_format {
	VPU_BUF_FORMAT_DATA,
	VPU_BUF_FORMAT_IMG_Y8,
	VPU_BUF_FORMAT_IMG_YV12,
	VPU_BUF_FORMAT_IMG_NV21,
	VPU_BUF_FORMAT_IMG_YUY2,
	VPU_BUF_FORMAT_IMPL_DEFINED = 0xFF,
};

struct vpu_buffer {
	vpu_id_t port_id;
	uint8_t format;
	uint8_t plane_count;
	uint32_t width;
	uint32_t height;
	struct vpu_plane planes[3];
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
	vpu_id_t algo_id;
	uint8_t status;
	uint8_t buffer_count;
	uint32_t sett_length;
	uint64_t sett_ptr;       /* pointer to the request setting */
	uint64_t priv;           /* reserved for user */
	struct vpu_buffer buffers[VPU_MAX_NUM_PORTS];
};

/*---------------------------------------------------------------------------*/
/*  IOCTL Command                                                            */
/*---------------------------------------------------------------------------*/
#define VPU_MAGICNO                 'v'
#define VPU_IOCTL_SET_POWER         _IOW(VPU_MAGICNO,   0, int)
#define VPU_IOCTL_ENQUE_REQUEST     _IOW(VPU_MAGICNO,   1, int)
#define VPU_IOCTL_DEQUE_REQUEST     _IOWR(VPU_MAGICNO,  2, int)
#define VPU_IOCTL_FLUSH_REQUEST     _IOW(VPU_MAGICNO,   3, int)
#define VPU_IOCTL_GET_ALGO_INFO     _IOWR(VPU_MAGICNO,  4, int)
#define VPU_IOCTL_LOCK              _IOW(VPU_MAGICNO,   5, int)
#define VPU_IOCTL_UNLOCK            _IOW(VPU_MAGICNO,   6, int)
#define VPU_IOCTL_LOAD_ALG          _IOW(VPU_MAGICNO,   7, int)
#define VPU_IOCTL_REG_WRITE         _IOW(VPU_MAGICNO,   8, int)
#define VPU_IOCTL_REG_READ          _IOWR(VPU_MAGICNO,  9, int)

int vpu_set_power(struct vpu_power *power);

int vpu_write_register(struct vpu_reg_values regs);

#endif
