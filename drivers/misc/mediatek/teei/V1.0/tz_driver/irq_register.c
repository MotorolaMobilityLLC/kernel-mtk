/*
 * Copyright (c) 2015-2017 MICROTRUST Incorporated
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

#include <linux/irq.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/freezer.h>
#include <linux/semaphore.h>
#include <linux/interrupt.h>
#include <linux/of_irq.h>
#include <linux/cpu.h>
#include "teei_id.h"
#include "sched_status.h"
#include "irq_register.h"
#include "nt_smc_call.h"
#include "teei_id.h"
#include "teei_common.h"
#include "utdriver_macro.h"
#include "notify_queue.h"
#include "switch_queue.h"
#include "teei_client_main.h"
#include "utdriver_irq.h"
#include <tlog.h>
#include "notify_queue.h"
#include "global_function.h"
#include <imsg_log.h>
static struct load_soter_entry load_ent;
static struct work_entry work_ent;
static struct work_entry sched_work_ent[SCHED_ENT_CNT];

/*extern struct timeval stime;*/

static struct teei_smc_cmd *get_response_smc_cmd(void)
{
	struct NQ_entry *nq_ent = NULL;

	nq_ent = (struct NQ_entry *)get_nq_entry((unsigned char *)t_nt_buffer);

	if (nq_ent == NULL)
		return NULL;

	return (struct teei_smc_cmd *)phys_to_virt((unsigned long)(nq_ent->buffer_addr));
}

void init_sched_work_ent(void)
{
	int i = 0;

	for (i = 0; i < SCHED_ENT_CNT; i++)
		sched_work_ent[i].in_use = 0;

}

void nt_sched_t_call(void)
{
	int retVal = 0;

	retVal = add_work_entry(SCHED_CALL, NULL);
	if (retVal != 0)
		IMSG_ERROR("[%s][%d] add_work_entry function failed!\n", __func__, __LINE__);


}


void sched_func(struct work_struct *entry)
{
	struct work_entry *md = container_of(entry, struct work_entry, work);

	down(&(smc_lock));
	nt_sched_t_call();
	md->in_use = 0;
}

struct work_entry *get_unused_work_entry(void)
{
	int i = 0;

	for (i = 0; i < SCHED_ENT_CNT; i++) {
		if (sched_work_ent[i].in_use == 0) {
			sched_work_ent[i].in_use = 1;
			return &(sched_work_ent[i]);
		}
	}

	return NULL;
}

void add_sched_queue(void)
{
	struct work_entry *curr_entry = NULL;

	curr_entry = get_unused_work_entry();

	if (curr_entry == NULL) {
		IMSG_ERROR("[%s][%d] Can NOT get unused schedule work_entry!", __func__, __LINE__);
		return;
	}

	INIT_WORK(&(curr_entry->work), sched_func);
	queue_work(secure_wq, &(curr_entry->work));
}

static irqreturn_t nt_sched_irq_handler(void)
{
	add_sched_queue();
	up(&(smc_lock));
	return IRQ_HANDLED;
}

static irqreturn_t nt_soter_irq_handler(int irq, void *dev)
{
	irq_call_flag = GLSCH_HIGH;

	up(&smc_lock);
	if (teei_config_flag == 1)
		complete(&global_down_lock);

	return IRQ_HANDLED;
}


int register_soter_irq_handler(int irq)
{
	return request_irq(irq, nt_soter_irq_handler, 0, "tz_drivers_service", NULL);
}

static irqreturn_t nt_error_irq_handler(void)
{
	unsigned long error_num = 0;

	nt_get_secure_os_state((uint64_t *)(&error_num));
	IMSG_ERROR("secure system ERROR ! error_num = %ld\n", (unsigned long)(error_num - 4294967296));
	soter_error_flag = 1;
	up(&(boot_sema));
	up(&smc_lock);
	return IRQ_HANDLED;
}

static irqreturn_t nt_fp_ack_handler(void)
{
	fp_call_flag = GLSCH_NONE;
	up(&fdrv_sema);
	up(&smc_lock);
	return IRQ_HANDLED;
}

int get_bdrv_id(void)
{
	int driver_id = 0;

	Invalidate_Dcache_By_Area(bdrv_message_buff, bdrv_message_buff + MESSAGE_LENGTH);
	driver_id = *((int *)bdrv_message_buff);
	return driver_id;
}

void add_bdrv_queue(int bdrv_id)
{
	work_ent.call_no = bdrv_id;
	INIT_WORK(&(work_ent.work), work_func);
	queue_work(bdrv_wq, &(work_ent.work));
}

static irqreturn_t nt_bdrv_handler(void)
{
	int bdrv_id = 0;

	up(&(smc_lock));
	bdrv_id = get_bdrv_id();
	add_bdrv_queue(bdrv_id);

	return IRQ_HANDLED;
}

static irqreturn_t nt_boot_irq_handler(void)
{
	if (boot_soter_flag == START_STATUS) {
		IMSG_DEBUG("boot irq  handler if\n");
		boot_soter_flag = END_STATUS;
		up(&smc_lock);
		up(&(boot_sema));
	} else {
		IMSG_DEBUG("boot irq hanler else\n");
		forward_call_flag = GLSCH_NONE;
		up(&smc_lock);
		up(&(boot_sema));
	}
	return IRQ_HANDLED;
}

void secondary_load_func(void)
{
	uint64_t smc_type = 2;

	Flush_Dcache_By_Area((unsigned long)boot_vfs_addr, (unsigned long)boot_vfs_addr + VFS_SIZE);
	IMSG_DEBUG("[%s][%d]: %s end.\n", __func__, __LINE__, __func__);
	n_ack_t_load_img(&smc_type, 0, 0);
	while (smc_type == 0x54)
		nt_sched_t(&smc_type);
}


void load_func(struct work_struct *entry)
{
	int retVal = 0;

	vfs_thread_function(boot_vfs_addr, (unsigned long)NULL, (unsigned long)NULL);
	down(&smc_lock);
	retVal = add_work_entry(LOAD_FUNC, NULL);
}

void work_func(struct work_struct *entry)
{
	struct work_entry *md = container_of(entry, struct work_entry, work);

	int sys_call_num = md->call_no;

	if (sys_call_num == reetime.sysno)
		reetime.handle(&reetime);
	else if (sys_call_num == vfs_handler.sysno)
		vfs_handler.handle(&vfs_handler);
	else
		IMSG_ERROR("[%s][%d] ============ ERROR =============\n", __func__, __LINE__);

}

static irqreturn_t nt_switch_irq_handler(void)
{
	struct teei_smc_cmd *command = NULL;
	struct semaphore *cmd_sema = NULL;
	struct message_head *msg_head = NULL;

	if (boot_soter_flag == START_STATUS) {
		/* IMSG_DEBUG("[%s][%d] ==== boot_soter_flag == START_STATUS ========\n", __func__, __LINE__); */
		INIT_WORK(&(load_ent.work), load_func);
		queue_work(secure_wq, &(load_ent.work));
		up(&smc_lock);

		return IRQ_HANDLED;

	} else {
		Invalidate_Dcache_By_Area(message_buff, message_buff + MESSAGE_LENGTH);
		msg_head = (struct message_head *)message_buff;

		if (msg_head->message_type == FAST_CALL_TYPE) {
			/* IMSG_DEBUG("[%s][%d] ==== FAST_CALL_TYPE ACK ========\n", __func__, __LINE__); */
			return IRQ_HANDLED;
		} else if (msg_head->message_type == STANDARD_CALL_TYPE) {
			/* Get the smc_cmd struct */
			if (msg_head->child_type == VDRV_CALL_TYPE) {
				/* IMSG_DEBUG("[%s][%d] ==== VDRV_CALL_TYPE ========\n", __func__, __LINE__); */
				work_ent.call_no = msg_head->param_length;
				INIT_WORK(&(work_ent.work), work_func);
				queue_work(secure_wq, &(work_ent.work));
				up(&smc_lock);
			} else if (msg_head->child_type == NQ_CALL_TYPE) {
				/* IMSG_DEBUG("[%s][%d] ==== STANDARD_CALL_TYPE ACK ========\n", __func__, __LINE__); */

				forward_call_flag = GLSCH_NONE;
				command = get_response_smc_cmd();

				if (command == NULL)
					return IRQ_NONE;

				Invalidate_Dcache_By_Area((unsigned long)command,
								(unsigned long)command + MESSAGE_LENGTH);
				/* Get the semaphore */
				cmd_sema = (struct semaphore *)(command->teei_sema);
			/*
			*	IMSG_DEBUG("[%s][%d] After smc_call cmd_sema = %p\n", __func__, __LINE__, cmd_sema);
			*/
				/* Up the semaphore */
				up(cmd_sema);
				up(&smc_lock);
#ifdef TUI_SUPPORT
			} else if (msg_head->child_type == TUI_NOTICE_SYS_NO) {
				forward_call_flag = GLSCH_NONE;
				up(&(tui_notify_sema));
				up(&(smc_lock));
#endif
			} else
				IMSG_ERROR("[%s][%d] ==== Unknown child_type ========\n", __func__, __LINE__);

			return IRQ_HANDLED;
		}
		IMSG_ERROR("[%s][%d] ==== Unknown IRQ ========\n", __func__, __LINE__);
		return IRQ_NONE;
	}
}

static irqreturn_t ut_drv_irq_handler(int irq, void *dev)
{
	uint64_t irq_id = 0;
	int retVal = 0;

	/* Get the interrupt ID */
	nt_get_non_irq_num(&irq_id);

	switch (irq_id) {
	case SCHED_IRQ:
		retVal = nt_sched_irq_handler();
		break;
	case SWITCH_IRQ:
		retVal = nt_switch_irq_handler();
		break;
	case BDRV_IRQ:
		retVal = nt_bdrv_handler();
		break;
#ifndef CONFIG_MICROTRUST_TZ_LOG
	case TEEI_LOG_IRQ:
		retVal = tlog_handler();
		break;
#endif
	case FP_ACK_IRQ:
		retVal = nt_fp_ack_handler();
		break;
	case SOTER_ERROR_IRQ:
		retVal = nt_error_irq_handler();
		break;
	case BOOT_IRQ:
		retVal = nt_boot_irq_handler();
		break;
	default:
		retVal = -EINVAL;
		IMSG_ERROR("get undefine IRQ from secure OS!\n");
	}

	return retVal;
}

int register_ut_irq_handler(int irq)
{
	return request_irq(irq, ut_drv_irq_handler, 0, "tz_drivers_service", NULL);
}
