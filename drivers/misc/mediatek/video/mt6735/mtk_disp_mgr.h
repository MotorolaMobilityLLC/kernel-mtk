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
#include "primary_display.h"

long mtk_disp_mgr_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
extern struct mutex disp_trigger_lock;

int disp_create_session(disp_session_config *config);
int disp_destroy_session(disp_session_config *config);
int disp_get_session_number(void);
char *disp_session_mode_spy(unsigned int session_id);

extern struct mutex session_config_mutex;
extern disp_session_input_config *captured_session_input;
extern disp_session_input_config *cached_session_input;
extern disp_mem_output_config *captured_session_output;
extern disp_mem_output_config *cached_session_output;
extern unsigned int ext_session_id;
#ifdef CONFIG_SINGLE_PANEL_OUTPUT
extern struct SWITCH_MODE_INFO_STRUCT path_info;
#endif

#endif
