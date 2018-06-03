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

#include <linux/proc_fs.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

#include <linux/platform_device.h>

static int __init init_tchbst(void)
{
	struct proc_dir_entry *tchbst_dir = NULL;

	pr_debug("__init init_tchbst\n");


	tchbst_dir = proc_mkdir("perfmgr/tchbst", NULL);


	return 0;
}
device_initcall(init_tchbst);

/*MODULE_LICENSE("GPL");*/
/*MODULE_AUTHOR("MTK");*/
/*MODULE_DESCRIPTION("The tchbst file");*/
