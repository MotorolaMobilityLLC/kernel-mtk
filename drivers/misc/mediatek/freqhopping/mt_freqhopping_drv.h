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

/* move to /mediatek/platform/prj, can config. by prj. */
/* #define MEMPLL_SSC 0 */
/* #define MAINPLL_SSC 1 */

struct mt_fh_hal_driver {
	int (*mt_fh_hal_init)(void);
	void (*mt_fh_hal_default_conf)(void);
	int (*mt_fh_hal_dumpregs_read)(struct seq_file *m, void *v);
	int (*mt_fh_hal_slt_start)(void);
};

struct mt_fh_hal_driver *mt_get_fh_hal_drv(void);

#define FH_BUG_ON(x) \
do {		 \
	if ((x)) \
		pr_err("BUGON %s:%d %s:%d\n", __func__, __LINE__, current->comm, current->pid); \
} while (0)

#endif
