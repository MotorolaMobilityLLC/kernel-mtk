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

#ifndef __FREQHOPPING_DRV_H
#define __FREQHOPPING_DRV_H

#include <linux/proc_fs.h>
#include "mt_freqhopping.h"

struct mt_fh_hal_driver {
	struct fh_pll_t *fh_pll;
	void (*mt_fh_lock)(unsigned long *);
	void (*mt_fh_unlock)(unsigned long *);
	int (*mt_fh_hal_init)(void);
	int (*mt_fh_get_init)(void);
	int (*mt_fh_hal_ctrl)(struct freqhopping_ioctl *, bool);
	void (*mt_fh_hal_default_conf)(void);
	int (*mt_dfs_armpll)(unsigned int, unsigned int);
	int (*mt_fh_hal_dumpregs_read)(struct seq_file *m, void *v);
	int (*mt_fh_hal_slt_start)(void);
};

struct mt_fh_hal_driver *mt_get_fh_hal_drv(void);

int mt_dfs_armpll(unsigned int pll, unsigned int dds);

#endif
