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

#ifndef _DDP_RSZ_H_
#define _DDP_RSZ_H_

#include "ddp_hal.h"
#include "ddp_info.h"

int rsz_calc_tile_params(u32 frm_in_len, u32 frm_out_len,
			 bool tile_mode, struct rsz_tile_params *t);

void rsz_dump_analysis(enum DISP_MODULE_ENUM module);
void rsz_dump_reg(enum DISP_MODULE_ENUM module);

#endif
