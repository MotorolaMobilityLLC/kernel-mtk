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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/proc_fs.h>

#include <mach/mt_clkmgr.h>

int enable_clock(enum cg_clk_id id, char *name)
{
	int err = 0;

	err = mt_enable_clock(id, name);

	return err;
}
EXPORT_SYMBOL(enable_clock);


int disable_clock(enum cg_clk_id id, char *name)
{
	int err = 0;

	err = mt_disable_clock(id, name);

	return err;
}
EXPORT_SYMBOL(disable_clock);

