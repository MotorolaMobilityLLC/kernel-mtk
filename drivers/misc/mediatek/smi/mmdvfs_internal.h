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

#ifndef __MMDVFS_INTERNAL_H__
#define __MMDVFS_INTERNAL_H__

#include "mmdvfs_mgr.h"

#define MMDVFS_LOG_TAG	"MMDVFS"

#define MMDVFSMSG(string, args...) pr_warn("[pid=%d]"string, current->tgid, ##args)

#define MMDVFSDEBUG(level, x...)            \
		do {                        \
			if (g_mmvfs_debug_level && (*g_mmvfs_debug_level) >= (level))    \
				MMDVFSMSG(x);            \
		} while (0)

#define MMDVFSMSG2(string, args...) pr_debug(string, ##args)

#define MMDVFSTMP(string, args...) pr_debug("[pid=%d]"string, current->tgid, ##args)

#define MMDVFSERR(string, args...) \
		do {\
			pr_debug("error: "string, ##args); \
			aee_kernel_warning(MMDVFS_LOG_TAG, "error: "string, ##args);  \
		} while (0)

extern void mmdvfs_internal_handle_state_change(struct mmdvfs_state_change_event *event);
extern void mmdvfs_internal_notify_vcore_calibration(struct mmdvfs_prepare_action_event *event);
extern unsigned int *g_mmvfs_debug_level;
extern unsigned int *g_mmdvfs_rt_debug_disable_mask;
extern int mmdvfs_internal_set_vpu_step(int current_step, int update_step);
#endif				/* __MMDVFS_INTERNAL_H__ */
