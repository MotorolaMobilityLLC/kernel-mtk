/*
 * Copyright (c) 2015 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef MEDIATEK_DRM_DDP_H
#define MEDIATEK_DRM_DDP_H

void mediatek_ddp_main_path_setup(struct device *dev);
void mediatek_ddp_ext_path_setup(struct device *dev);

void mediatek_ddp_clock_on(struct device *dev);
void mediatek_ddp_clock_off(struct device *dev);

int mediatek_disp_path_get_mutex(int mutexID, struct device *dev);
int mediatek_disp_path_release_mutex(int mutexID, struct device *dev);

#endif /* MEDIATEK_DRM_DDP_H */

