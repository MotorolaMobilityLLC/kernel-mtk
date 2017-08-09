#ifndef _DDP_RDMA_EX_H_
#define _DDP_RDMA_EX_H_
#include "ddp_info.h"

#define RDMA_INSTANCES  2
#define RDMA_MAX_WIDTH  4095
#define RDMA_MAX_HEIGHT 4095

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
