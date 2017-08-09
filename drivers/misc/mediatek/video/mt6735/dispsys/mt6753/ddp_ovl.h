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

#ifndef _DDP_OVL_H_
#define _DDP_OVL_H_

#include "ddp_hal.h"
#include "DpDataType.h"
#include "ddp_info.h"


#define OVL_MAX_WIDTH  (4095)
#define OVL_MAX_HEIGHT (4095)

#ifdef OVL_CASCADE_SUPPORT
#define OVL_LAYER_NUM  (6)
#else
#define OVL_LAYER_NUM  (4)
#endif

#define OVL_LAYER_NUM_PER_OVL 4

typedef enum {
	DDP_OVL1_STATUS_IDLE = 0,
	DDP_OVL1_STATUS_PRIMARY = 1,	/* used for primary 8 layer blending */
	DDP_OVL1_STATUS_SUB = 2,	/* used for sub 4 layer bleding */
	DDP_OVL1_STATUS_SUB_REQUESTING = 3,	/* sub request to use OVL1 */
	DDP_OVL1_STATUS_PRIMARY_RELEASED = 4,
	DDP_OVL1_STATUS_PRIMARY_RELEASING = 5,
	DDP_OVL1_STATUS_PRIMARY_DISABLE = 6
} DISP_OVL1_STATUS;


/* start overlay module */
int ovl_start(DISP_MODULE_ENUM module, void *handle);

/* stop overlay module */
int ovl_stop(DISP_MODULE_ENUM module, void *handle);

/* reset overlay module */
int ovl_reset(DISP_MODULE_ENUM module, void *handle);

/* set region of interest */
int ovl_roi(DISP_MODULE_ENUM module, unsigned int bgW, unsigned int bgH,	/* region size */
	    unsigned int bgColor,	/* border color */

	    void *handle);

/* switch layer on/off */
int ovl_layer_switch(DISP_MODULE_ENUM module, unsigned layer, unsigned int en, void *handle);
/* get ovl input address */
void ovl_get_address(DISP_MODULE_ENUM module, unsigned long *add);

int ovl_3d_config(DISP_MODULE_ENUM module,
		  unsigned int layer_id,
		  unsigned int en_3d, unsigned int landscape, unsigned int r_first, void *handle);

void ovl_dump_analysis(DISP_MODULE_ENUM module);
void ovl_dump_reg(DISP_MODULE_ENUM module);

void ovl_get_info(int idx, void *data);
void ovl_reset_by_cmdq(void *handle, DISP_MODULE_ENUM module);

DISP_OVL1_STATUS ovl_get_status(void);
void ovl_set_status(DISP_OVL1_STATUS status);
unsigned int ddp_ovl_get_cur_addr(bool rdma_mode, int layerid);

struct DDP_MODULE_DRIVER;

#endif
