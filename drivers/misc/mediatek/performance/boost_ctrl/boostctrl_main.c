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
#include "eas_ctrl.h"

static int __init init_boostctrl(void)
{
	struct proc_dir_entry *hps_dir = NULL;

	pr_debug("__init init_boostctrl\n");


	hps_dir = proc_mkdir("perfmgr/boost_ctrl", NULL);

	init_perfmgr_eas_controller();

	return 0;
}
device_initcall(init_boostctrl);

/*MODULE_LICENSE("GPL");*/
/*MODULE_AUTHOR("MTK");*/
/*MODULE_DESCRIPTION("The boost_ctrl file");*/
