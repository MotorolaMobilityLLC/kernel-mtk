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

#ifndef __H_MTK_DISP_MGR__
#define __H_MTK_DISP_MGR__

extern unsigned int is_hwc_enabled;
char *disp_session_mode_spy(unsigned int session_id);
long mtk_disp_mgr_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

#endif
