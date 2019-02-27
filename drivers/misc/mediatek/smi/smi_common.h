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

#ifndef __SMI_COMMON_H__
#define __SMI_COMMON_H__

#include "smi_configuration.h"

/* use function instead gLarbBaseAddr to prevent NULL pointer access error */
unsigned long get_larb_base_addr(int larb_id);
unsigned long get_common_base_addr(void);
unsigned int smi_clk_get_ref_count(const unsigned int reg_indx);

int smi_bus_regs_setting(int larb_id, int profile,
	struct SMI_SETTING *settings);
int smi_common_setting(struct SMI_SETTING *settings);
int smi_larb_setting(int larb_id, struct SMI_SETTING *settings);
#endif
