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
#include "ddp_info.h"

#define OVL_MAX_WIDTH  (4095)
#define OVL_MAX_HEIGHT (4095)

#define TOTAL_OVL_LAYER_NUM		(12)
#define OVL_NUM					(4)
#define PRIMARY_THREE_OVL_CASCADE

/* start overlay module */
int ovl_start(DISP_MODULE_ENUM module, void *handle);

/* stop overlay module */
int ovl_stop(DISP_MODULE_ENUM module, void *handle);

/* reset overlay module */
int ovl_reset(DISP_MODULE_ENUM module, void *handle);

/* set region of interest */
int ovl_roi(DISP_MODULE_ENUM module, unsigned int bgW, unsigned int bgH,
		unsigned int bgColor,
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
unsigned long ovl_base_addr(DISP_MODULE_ENUM module);
unsigned long ovl_to_index(DISP_MODULE_ENUM module);

void ovl_get_info(DISP_MODULE_ENUM module, void *data);
unsigned int ddp_ovl_get_cur_addr(bool rdma_mode, int layerid);

#endif
