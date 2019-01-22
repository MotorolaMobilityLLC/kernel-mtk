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

#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/hardirq.h>
#include <linux/init.h>
#include <linux/kallsyms.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/workqueue.h>
#include <linux/kthread.h>
#include <mt-plat/aee.h>

#include "fbt_error.h"
#include "fbt_base.h"
#include "fbt_notifier.h"
#include "fbt_cpu.h"

static void fbt_exit(void)
{
	fbt_cpu_exit();

	fbt_notifier_exit();
}

static int __init fbt_init(void)
{
	int vRet;

	vRet = fbt_notifier_init();

	if (vRet)
		goto ERROR;

	vRet = fbt_cpu_init();

	return 0;

ERROR:
	fbt_exit();

	return -EFAULT;
}

module_init(fbt_init);
module_exit(fbt_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MediaTek FBT");
MODULE_AUTHOR("MediaTek Inc.");
