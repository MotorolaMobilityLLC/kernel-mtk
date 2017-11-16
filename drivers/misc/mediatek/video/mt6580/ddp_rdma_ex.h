/*
 * Copyright (C) 2015 MediaTek Inc.
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

#ifndef _DDP_RDMA_EX_H_
#define _DDP_RDMA_EX_H_

#include <mt-plat/sync_write.h>
#include <linux/types.h>
/* #include <mach/mt_reg_base.h> */
#include "ddp_info.h"
#include "ddp_hal.h"

#define RDMA_INSTANCES  1
#define RDMA_MAX_WIDTH  4095
#define RDMA_MAX_HEIGHT 4095

enum RDMA_INPUT_FORMAT {
	RDMA_INPUT_FORMAT_BGR565 = 0,
	RDMA_INPUT_FORMAT_RGB888 = 1,
	RDMA_INPUT_FORMAT_RGBA8888 = 2,
	RDMA_INPUT_FORMAT_ARGB8888 = 3,
	RDMA_INPUT_FORMAT_VYUY = 4,
	RDMA_INPUT_FORMAT_YVYU = 5,

	RDMA_INPUT_FORMAT_RGB565 = 6,
	RDMA_INPUT_FORMAT_BGR888 = 7,
	RDMA_INPUT_FORMAT_BGRA8888 = 8,
	RDMA_INPUT_FORMAT_ABGR8888 = 9,
	RDMA_INPUT_FORMAT_UYVY = 10,
	RDMA_INPUT_FORMAT_YUYV = 11,

	RDMA_INPUT_FORMAT_UNKNOWN = 32,
};

enum RDMA_OUTPUT_FORMAT {
	RDMA_OUTPUT_FORMAT_ARGB = 0,
	RDMA_OUTPUT_FORMAT_YUV444 = 1,
};

enum RDMA_MODE {
	RDMA_MODE_DIRECT_LINK = 0,
	RDMA_MODE_MEMORY = 1,
};

typedef struct _rdma_color_matrix {
	uint32_t C00;
	uint32_t C01;
	uint32_t C02;
	uint32_t C10;
	uint32_t C11;
	uint32_t C12;
	uint32_t C20;
	uint32_t C21;
	uint32_t C22;
} rdma_color_matrix;

typedef struct _rdma_color_pre {
	uint32_t ADD0;
	uint32_t ADD1;
	uint32_t ADD2;
} rdma_color_pre;

typedef struct _rdma_color_post {
	uint32_t ADD0;
	uint32_t ADD1;
	uint32_t ADD2;
} rdma_color_post;

int rdma_clock_on(DISP_MODULE_ENUM module, void *handle);
int rdma_clock_off(DISP_MODULE_ENUM module, void *handle);

void rdma_enable_color_transform(DISP_MODULE_ENUM module);
void rdma_disable_color_transform(DISP_MODULE_ENUM module);
void rdma_set_color_matrix(DISP_MODULE_ENUM module,
			   rdma_color_matrix *matrix,
			   rdma_color_pre *pre, rdma_color_post *post);
#endif
