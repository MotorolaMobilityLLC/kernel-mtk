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
#include "disp_session.h"
#include <linux/fs.h>

typedef enum {
	PREPARE_INPUT_FENCE,
	PREPARE_OUTPUT_FENCE,
	PREPARE_PRESENT_FENCE
} ePREPARE_FENCE_TYPE;

long mtk_disp_mgr_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
int disp_create_session(disp_session_config *config);
int disp_destroy_session(disp_session_config *config);
int set_session_mode(disp_session_config *config_info, int force);
char *disp_session_mode_spy(unsigned int session_id);

#endif
