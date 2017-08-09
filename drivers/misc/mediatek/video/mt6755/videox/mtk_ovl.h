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


#ifndef __MTK_OVL_H__
#define __MTK_OVL_H__
#include "primary_display.h"
void ovl2mem_setlayernum(int layer_num);
int ovl2mem_get_info(void *info);
int get_ovl2mem_ticket(void);
int ovl2mem_init(unsigned int session);
int ovl2mem_input_config(disp_session_input_config *input);
int ovl2mem_output_config(disp_mem_output_config *out);
int ovl2mem_trigger(int blocking, void *callback, unsigned int userdata);
void ovl2mem_wait_done(void);
int ovl2mem_deinit(void);

#endif
