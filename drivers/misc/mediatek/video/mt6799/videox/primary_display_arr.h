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

#ifndef _PRIMARY_DISPLAY_ARR_H_
#define _PRIMARY_DISPLAY_ARR_H_

/* used by ARR2 */
void primary_display_arr30_set_fps_window(unsigned int fps);
void primary_display_set_fake_vsync_fps(unsigned int fps);
int primary_display_set_sigle_frame_mode(int enable);
int primary_display_arr20_set_refresh_rate(unsigned int refresh_rate);
unsigned int primary_display_arr20_current_get_refresh_rate(unsigned int needLock);
unsigned int primary_display_arr20_get_max_refresh_rate(unsigned int needLock);
unsigned int primary_display_arr20_get_min_refresh_rate(unsigned int needLock);

#endif /* _PRIMARY_DISPLAY_ARR_H_ */
