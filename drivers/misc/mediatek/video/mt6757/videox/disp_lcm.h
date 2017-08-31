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

#ifndef _DISP_LCM_H_
#define _DISP_LCM_H_
#include "lcm_drv.h"

#define MAX_LCM_NUMBER	2

extern unsigned char lcm_name_list[][128];
extern LCM_DRIVER lcm_common_drv;


typedef struct {
	LCM_PARAMS *params;
	LCM_DRIVER *drv;
	LCM_INTERFACE_ID lcm_if_id;
	int module;
	int is_inited;
	unsigned int lcm_original_width;
	unsigned int lcm_original_height;
	int index;
} disp_lcm_handle, *pdisp_lcm_handle;


/* these 2 variables are defined in mt65xx_lcm_list.c */
extern LCM_DRIVER *lcm_driver_list[];
extern unsigned int lcm_count;


disp_lcm_handle *disp_lcm_probe(char *plcm_name, LCM_INTERFACE_ID lcm_id, int is_lcm_inited);
int disp_lcm_init(disp_lcm_handle *plcm, int force);
LCM_PARAMS *disp_lcm_get_params(disp_lcm_handle *plcm);
LCM_INTERFACE_ID disp_lcm_get_interface_id(disp_lcm_handle *plcm);
int disp_lcm_update(disp_lcm_handle *plcm, int x, int y, int w, int h, int force);
int disp_lcm_esd_check(disp_lcm_handle *plcm);
int disp_lcm_esd_recover(disp_lcm_handle *plcm);
int disp_lcm_suspend(disp_lcm_handle *plcm);
int disp_lcm_resume(disp_lcm_handle *plcm);
int disp_lcm_is_support_adjust_fps(disp_lcm_handle *plcm);
int disp_lcm_adjust_fps(void *cmdq, disp_lcm_handle *plcm, int fps);
int disp_lcm_set_backlight(disp_lcm_handle *plcm, void *handle, int level);
int disp_lcm_read_fb(disp_lcm_handle *plcm);
int disp_lcm_ioctl(disp_lcm_handle *plcm, LCM_IOCTL ioctl, unsigned int arg);
int disp_lcm_is_video_mode(disp_lcm_handle *plcm);
int disp_lcm_is_inited(disp_lcm_handle *plcm);
unsigned int disp_lcm_ATA(disp_lcm_handle *plcm);
void *disp_lcm_switch_mode(disp_lcm_handle *plcm, int mode);
int disp_lcm_set_lcm_cmd(disp_lcm_handle *plcm, void *cmdq_handle, unsigned int *lcm_cmd,
			 unsigned int *lcm_count, unsigned int *lcm_value);

int disp_lcm_is_partial_support(disp_lcm_handle *plcm);
int disp_lcm_validate_roi(disp_lcm_handle *plcm, int *x, int *y, int *w, int *h);
#endif
