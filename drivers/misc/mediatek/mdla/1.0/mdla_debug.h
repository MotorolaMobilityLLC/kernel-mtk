/*
 * Copyright (C) 2018 MediaTek Inc.
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

#include <linux/types.h>
#include <linux/printk.h>

#ifdef CONFIG_MTK_MDLA_DEBUG
#define mdla_debug pr_debug
void mdla_dump_reg(void);
#else
#define mdla_debug(...)
static inline void mdla_dump_reg(void)
{
}
#endif

