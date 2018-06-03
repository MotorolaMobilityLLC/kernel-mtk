/*
 * Copyright (C) 2018 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include <linux/workqueue.h>
#include <mt-plat/aee.h>

#include "mtk_gpu_log.h"

static struct workqueue_struct *g_aee_workqueue;
static struct work_struct g_aee_work;

static void aee_Handle(struct work_struct *_psWork)
{
	/* avoid the build warnning */
	_psWork = _psWork;

	aee_kernel_exception("gpulog", "aee dump gpulog");
}

void mtk_gpu_log_trigger_aee(const char *msg)
{
	static int once;

	GPULOG("trigger aee [%s]", msg);

	if (g_aee_workqueue && once == 0) {
		queue_work(g_aee_workqueue, &g_aee_work);
		once = 1;
	}
}

void mtk_gpu_log_init(void)
{
	g_aee_workqueue = alloc_ordered_workqueue("gpu_aee_wq", WQ_FREEZABLE | WQ_MEM_RECLAIM);
	INIT_WORK(&g_aee_work, aee_Handle);

	/* init log hnd */
	ged_log_buf_get_early("fence_trace", &_mtk_gpu_log_hnd);
}

