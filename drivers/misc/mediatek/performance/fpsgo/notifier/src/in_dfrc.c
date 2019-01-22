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

#include <mach/mtk_ppm_api.h>
#include <fpsgo_common.h>

#include "fbt_error.h"
#include "fbt_base.h"
#include "fbt_usedext.h"
#include "fbt_notifier.h"

#ifdef CONFIG_MTK_DYNAMIC_FPS_FRAMEWORK_SUPPORT
#include "dfrc.h"
#include "dfrc_drv.h"
#endif

#define TARGET_UNLIMITED_FPS 60

#ifdef CONFIG_MTK_DYNAMIC_FPS_FRAMEWORK_SUPPORT
void dfrc_fps_limit_cb(int fps_limit)
{
	unsigned int vTmp = TARGET_UNLIMITED_FPS;

	if (fps_limit != DFRC_DRV_FPS_NON_ASSIGN)
		vTmp = fps_limit;

	fbt_notifier_push_dfrc_fps_limit(vTmp);
}
EXPORT_SYMBOL(dfrc_fps_limit_cb);
#endif

