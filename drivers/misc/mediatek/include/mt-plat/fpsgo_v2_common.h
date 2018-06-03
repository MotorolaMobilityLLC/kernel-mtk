/*
 * Copyright (C) 2017 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _FPSGO_V2_COMMON_H_
#define _FPSGO_V2_COMMON_H_

#include "fpsgo_common.h"

#define fpsgo_systrace_c_fbt(pid, val, fmt...) \
	fpsgo_systrace_c(FPSGO_DEBUG_MANDATORY, pid, val, fmt)

void fpsgo_switch_enable(int enable);
void fpsgo_switch_enable_keep(int enable);
int fpsgo_is_enable(void);

#endif
