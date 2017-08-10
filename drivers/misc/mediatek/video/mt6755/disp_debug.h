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

#ifndef __DISP_DEBUG_H__
#define __DISP_DEBUG_H__

#include <linux/kernel.h>
#include "ddp_mmp.h"
#include "ddp_dump.h"
#include "disp_log.h"
#include "mmprofile.h"

#define LP_CUST_DISABLE (0)
#define LOW_POWER_MODE (1)
#define JUST_MAKE_MODE (2)
#define PERFORMANC_MODE (3)

int get_lp_cust_mode(void);
unsigned int get_backup_vfp(void);
void backup_vfp_for_lp_cust(unsigned int vfp);

int ddp_mem_test(void);
int ddp_lcd_test(void);

void sub_debug_init(void);
void sub_debug_deinit(void);

#endif /* __DISP_DEBUG_H__ */
