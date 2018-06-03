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

#ifndef __FPSGO_BASE_H__
#define __FPSGO_BASE_H__

#include <linux/compiler.h>

#ifdef FPSGO_DEBUG
#define FPSGO_LOGI(...)	pr_debug("FPSGO:" __VA_ARGS__)
#else
#define FPSGO_LOGI(...)
#endif
#define FPSGO_LOGE(...)	pr_debug("FPSGO:" __VA_ARGS__)
#define FPSGO_CONTAINER_OF(ptr, type, member) ((type *)(((char *)ptr) - offsetof(type, member)))

void *fpsgo_alloc_atomic(int i32Size);
void fpsgo_free(void *pvBuf, int i32Size);
unsigned long long fpsgo_get_time(void);

enum FPSGO_ERROR {
	FPSGO_OK,
	FPSGO_ERROR_FAIL,
	FPSGO_ERROR_OOM,
	FPSGO_ERROR_OUT_OF_FD,
	FPSGO_ERROR_FAIL_WITH_LIMIT,
	FPSGO_ERROR_TIMEOUT,
	FPSGO_ERROR_CMD_NOT_PROCESSED,
	FPSGO_ERROR_INVALID_PARAMS,
	FPSGO_INTENTIONAL_BLOCK
};

enum FRAME_TYPE {
	NON_VSYNC_ALIGNED_TYPE = 0,
	VSYNC_ALIGNED_TYPE = 1,
	BY_PASS_TYPE = 2,
};

enum {
	WINDOW_DISCONNECT = 0,
	NATIVE_WINDOW_API_EGL = 1,
	NATIVE_WINDOW_API_CPU = 2,
	NATIVE_WINDOW_API_MEDIA = 3,
	NATIVE_WINDOW_API_CAMERA = 4,
};

int init_fpsgo_common(void);

#endif

