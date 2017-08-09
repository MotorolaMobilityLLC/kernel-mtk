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
	UINT32 C00;
	UINT32 C01;
	UINT32 C02;
	UINT32 C10;
	UINT32 C11;
	UINT32 C12;
	UINT32 C20;
	UINT32 C21;
	UINT32 C22;
} rdma_color_matrix;

typedef struct _rdma_color_pre {
	UINT32 ADD0;
	UINT32 ADD1;
	UINT32 ADD2;
} rdma_color_pre;

typedef struct _rdma_color_post {
	UINT32 ADD0;
	UINT32 ADD1;
	UINT32 ADD2;
} rdma_color_post;


/* start module */
int rdma_start(DISP_MODULE_ENUM module, void *handle);

/* stop module */
int rdma_stop(DISP_MODULE_ENUM module, void *handle);

/* reset module */
int rdma_reset(DISP_MODULE_ENUM module, void *handle);

void rdma_set_target_line(DISP_MODULE_ENUM module, unsigned int line, void *handle);

void rdma_get_address(DISP_MODULE_ENUM module, unsigned long *data);
void rdma_dump_reg(DISP_MODULE_ENUM module);
void rdma_dump_analysis(DISP_MODULE_ENUM module);
void rdma_dump_golden_setting_context(DISP_MODULE_ENUM module);
void rdma_get_info(int idx, RDMA_BASIC_STRUCT *info);

void rdma_enable_color_transform(DISP_MODULE_ENUM module);
void rdma_disable_color_transform(DISP_MODULE_ENUM module);
void rdma_set_color_matrix(DISP_MODULE_ENUM module,
			   rdma_color_matrix *matrix,
			   rdma_color_pre *pre, rdma_color_post *post);

extern unsigned long long rdma_start_time[2];
extern unsigned long long rdma_end_time[2];
extern unsigned int rdma_start_irq_cnt[2];
extern unsigned int rdma_done_irq_cnt[2];
extern unsigned int rdma_underflow_irq_cnt[2];
extern unsigned int rdma_targetline_irq_cnt[2];

#endif
