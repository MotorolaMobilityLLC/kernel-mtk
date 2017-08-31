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

#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <aee.h>
#include <linux/timer.h>
/* #include <asm/system.h> */
#include <asm-generic/irq_regs.h>
/* #include <asm/mach/map.h> */
#include <sync_write.h>
/*#include <mach/irqs.h>*/
#include <asm/cacheflush.h>
#include <linux/string.h>
#include <linux/time.h>
#include <linux/fb.h>
#include <linux/debugfs.h>
#include <m4u.h>
#include "mtk_smi.h"

#include "smi_common.h"
#include "smi_reg.h"

#define SMI_LOG_TAG "smi"

bool smi_clk_always_on;

static char debug_buffer[4096];

/* --------------------------------------------------------------------------- */
/* Debug FileSystem Routines */
/* --------------------------------------------------------------------------- */

struct dentry *smi_dbgfs;


static int debug_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static ssize_t debug_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
	return simple_read_from_buffer(ubuf, count, ppos, debug_buffer, strlen(debug_buffer));
}

static const struct file_operations debug_fops = {
	.read = debug_read,
	.open = debug_open,
};


void SMI_DBG_Init(void)
{
	smi_dbgfs = debugfs_create_file("smi", S_IFREG | S_IRUGO, NULL, (void *)0, &debug_fops);
	memset(debug_buffer, 0, sizeof(debug_buffer));
}


void SMI_DBG_Deinit(void)
{
	debugfs_remove(smi_dbgfs);
}
