/*
 * Copyright (c) 2013-2017 TRUSTONIC LIMITED
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#ifndef _MC_OPS_H_
#define _MC_OPS_H_

#include <linux/workqueue.h>
#include "fastcall.h"

int mc_yield(void);
int mc_nsiq(void);
int _nsiq(void);
u32 mc_get_version(void);

int mc_info(u32 ext_info_id, u32 *state, u32 *ext_info);
int mc_init(phys_addr_t base, u32  nq_length, u32 mcp_offset,
	    u32  mcp_length);
#ifdef TBASE_CORE_SWITCHER
int mc_switch_core(u32 core_num);
#endif

bool mc_fastcall(void *data);

int mc_fastcall_init(struct mc_context *context);
void mc_fastcall_destroy(void);

#endif /* _MC_OPS_H_ */
