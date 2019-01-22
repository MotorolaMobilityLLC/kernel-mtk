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

#ifndef __VPU_DBG_H__
#define __VPU_DBG_H__

/*
 * level 2: open log to monitor d2d period
 * level 3: open log to monitor load algp period
 * level 5: open per-frame log
 * level 9: dump log buffer
 */
extern int g_vpu_log_level;
extern unsigned int g_func_mask;

enum VpuFuncMask {
	VFM_SKIP_WAIT_VCORE	= 0x1,
	VFM_ROUTINE_PRT_SYSLOG = 0x2
};

#endif
