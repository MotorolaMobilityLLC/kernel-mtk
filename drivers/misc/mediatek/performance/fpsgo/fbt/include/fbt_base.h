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

#ifndef __FBT_BASE_H__
#define __FBT_BASE_H__

#include <linux/compiler.h>

#ifdef FBT_DEBUG
#define FBT_LOGI(...)	pr_debug("FBT:" __VA_ARGS__)
#else
#define FBT_LOGI(...)
#endif
#define FBT_LOGE(...)	pr_debug("FBT:" __VA_ARGS__)
#define FBT_CONTAINER_OF(ptr, type, member) ((type *)(((char *)ptr) - offsetof(type, member)))

unsigned long ged_copy_to_user(void __user *pvTo, const void *pvFrom, unsigned long ulBytes);

unsigned long ged_copy_from_user(void *pvTo, const void __user *pvFrom, unsigned long ulBytes);

void *fbt_alloc(int i32Size);

void *fbt_alloc_atomic(int i32Size);

void fbt_free(void *pvBuf, int i32Size);

long ged_get_pid(void);

unsigned long long ged_get_time(void);

#endif
