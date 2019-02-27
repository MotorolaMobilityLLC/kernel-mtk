/*
 * Copyright (c) 2014-2017 TRUSTONIC LIMITED
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef _TUI_HAL_H_
#define _TUI_HAL_H_

#include <linux/types.h>

u32 hal_tui_init(void);
void hal_tui_exit(void);
u32 hal_tui_alloc(struct tui_alloc_buffer_t allocbuffer[MAX_DCI_BUFFER_NUMBER],
		  size_t allocsize, u32 number);
void hal_tui_free(void);
u32 hal_tui_deactivate(void);
u32 hal_tui_activate(void);

#endif
