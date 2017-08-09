#ifndef _DDP_WDMA_H_
#define _DDP_WDMA_H_

#include "ddp_hal.h"
#include "ddp_info.h"

/* start module */
int wdma_start(DISP_MODULE_ENUM module, void *handle);

/* stop module */
int wdma_stop(DISP_MODULE_ENUM module, void *handle);

/* reset module */
int wdma_reset(DISP_MODULE_ENUM module, void *handle);

/* common interface */
unsigned int wdma_index(DISP_MODULE_ENUM module);
unsigned int ddp_wdma_get_cur_addr(void);
void wdma_dump_analysis(DISP_MODULE_ENUM module);
void wdma_dump_reg(DISP_MODULE_ENUM module);

#endif
