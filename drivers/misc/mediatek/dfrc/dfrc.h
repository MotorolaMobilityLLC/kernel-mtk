/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */
#include <linux/ioctl.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/types.h>

#ifndef __DFRC_H__
#define __DFRC_H__

#define DFRC_ALLOW_VIDEO 0x01
#define DFRC_ALLOW_TOUCH 0x02

int dfrc_allow_rrc_adjust_fps(void);
void dfrc_set_rrc_touch_fps(int fps);
void dfrc_set_rrc_video_fps(int fps);

long dfrc_set_kernel_policy(int api, int fps, int mode);

#endif
