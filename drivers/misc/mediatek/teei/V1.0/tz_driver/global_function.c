/*
 * Copyright (c) 2015-2016 MICROTRUST Incorporated
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/types.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/freezer.h>
#include <linux/semaphore.h>

#include "nt_smc_call.h"
#include "global_function.h"
#include "sched_status.h"
#include "utdriver_macro.h"
#define SCHED_CALL      0x04

extern int add_work_entry(int work_type, unsigned char *buff);

static void secondary_nt_sched_t(void *info)
{
	unsigned long smc_type = 2;

	nt_sched_t((uint64_t *)(&smc_type));

	while (smc_type == 1) {
		udelay(IRQ_DELAY);
		nt_sched_t((uint64_t *)(&smc_type));
	}
}


void nt_sched_t_call(void)
{
	int retVal = 0;

	retVal = add_work_entry(SCHED_CALL, NULL);
	if (retVal != 0)
		pr_err("[%s][%d] add_work_entry function failed!\n", __func__, __LINE__);


	return;
}


int global_fn(void)
{
	int retVal = 0;

	while (1) {
		set_freezable();
		set_current_state(TASK_INTERRUPTIBLE);
#if 1
		if (teei_config_flag == 1) {
			retVal = wait_for_completion_interruptible(&global_down_lock);
			if (retVal == -ERESTARTSYS) {
				pr_err("[%s][%d]*********down &global_down_lock failed *****************\n", __func__, __LINE__);
				continue;
			}
		}
#endif
		retVal = down_interruptible(&smc_lock);
		if (retVal != 0) {
			pr_err("[%s][%d]*********down &smc_lock failed *****************\n", __func__, __LINE__);
			complete(&global_down_lock);
			continue;
		}

		if (forward_call_flag == GLSCH_FOR_SOTER) {
			forward_call_flag = GLSCH_NONE;
			msleep(10);
			nt_sched_t_call();
		} else if (irq_call_flag == GLSCH_HIGH) {
			/* pr_debug("[%s][%d]**************************\n", __func__, __LINE__ ); */
			irq_call_flag = GLSCH_NONE;
			nt_sched_t_call();
			/*msleep_interruptible(10);*/
		} else if (fp_call_flag == GLSCH_HIGH) {
			/* pr_debug("[%s][%d]**************************\n", __func__, __LINE__ ); */
			if (teei_vfs_flag == 0) {
				nt_sched_t_call();
			} else {
				up(&smc_lock);
				msleep_interruptible(1);
			}
		} else if (forward_call_flag == GLSCH_LOW) {
			/* pr_debug("[%s][%d]**************************\n", __func__, __LINE__ ); */
			if (teei_vfs_flag == 0) {
				nt_sched_t_call();
			} else {
				up(&smc_lock);
				msleep_interruptible(1);
			}
		} else {
			/* pr_debug("[%s][%d]**************************\n", __func__, __LINE__ ); */
			up(&smc_lock);
			msleep_interruptible(1);
		}
	}
}
