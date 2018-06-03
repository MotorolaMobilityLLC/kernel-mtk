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
#ifdef MMDVFS_DEBUG_MODE
#define MMDVFSDEBUG(string, args...) pr_debug("[pid=%d]"string, current->tgid, ##args)
#define MMDVFSMSG2(string, args...) pr_debug(string, ##args)
#else
	#define MMDVFSDEBUG(string, args...)
	#define MMDVFSMSG2(string, args...)
#endif /* MMDVFS_DEBUG_MODE */

#define MMDVFSTMP(string, args...) pr_debug("[pid=%d]"string, current->tgid, ##args)
#define MMDVFSERR(string, args...) \
		do {\
			pr_debug("error: "string, ##args); \
			aee_kernel_warning(MMDVFS_LOG_TAG, "error: "string, ##args);  \
		} while (0)

extern void mmdvfs_internal_handle_state_change(struct mmdvfs_state_change_event *event);
extern void mmdvfs_internal_notify_vcore_calibration(struct mmdvfs_prepare_action_event *event);

#endif				/* __MMDVFS_INTERNAL_H__ */
