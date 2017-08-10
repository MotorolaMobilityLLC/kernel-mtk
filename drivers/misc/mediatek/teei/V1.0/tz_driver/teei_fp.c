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

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/semaphore.h>
#include <linux/delay.h>
#include <linux/cpu.h>
#include "teei_fp.h"
#include "teei_id.h"
#include "sched_status.h"
#include "nt_smc_call.h"
#include "teei_common.h"
#include "switch_queue.h"
#include "teei_client_main.h"
#include "utdriver_macro.h"
#include "backward_driver.h"
#include <imsg_log.h>
unsigned long fp_buff_addr = 0;

unsigned long create_fp_fdrv(int buff_size)
{
	long retVal = 0;
	unsigned long temp_addr = 0;
	struct message_head msg_head;
	struct create_fdrv_struct msg_body;
	struct ack_fast_call_struct msg_ack;

	if ((unsigned char *)message_buff == NULL) {
		IMSG_ERROR("[%s][%d]: There is NO command buffer!.\n", __func__, __LINE__);
		return (unsigned long)NULL;
	}


	if (buff_size > VDRV_MAX_SIZE) {
		IMSG_ERROR("[%s][%d]: FP Drv buffer is too large, Can NOT create it.\n", __FILE__, __LINE__);
		return (unsigned long)NULL;
	}

#ifdef UT_DMA_ZONE
	temp_addr = (unsigned long) __get_free_pages(GFP_KERNEL | GFP_DMA, get_order(ROUND_UP(buff_size, SZ_4K)));
#else
	temp_addr = (unsigned long) __get_free_pages(GFP_KERNEL, get_order(ROUND_UP(buff_size, SZ_4K)));
#endif

	if ((unsigned char *)temp_addr == NULL) {
		IMSG_ERROR("[%s][%d]: kmalloc fp drv buffer failed.\n", __FILE__, __LINE__);
		return (unsigned long)NULL;
	}

	memset((void *)(&msg_head), 0, sizeof(struct message_head));
	memset((void *)(&msg_body), 0, sizeof(struct create_fdrv_struct));
	memset((void *)(&msg_ack), 0, sizeof(struct ack_fast_call_struct));

	msg_head.invalid_flag = VALID_TYPE;
	msg_head.message_type = FAST_CALL_TYPE;
	msg_head.child_type = FAST_CREAT_FDRV;
	msg_head.param_length = sizeof(struct create_fdrv_struct);

	msg_body.fdrv_type = FP_SYS_NO;
	msg_body.fdrv_phy_addr = virt_to_phys((void *)temp_addr);
	msg_body.fdrv_size = buff_size;

	/* Notify the T_OS that there is ctl_buffer to be created. */
	memcpy((void *)message_buff, (void *)(&msg_head), sizeof(struct message_head));
	memcpy((void *)(message_buff + sizeof(struct message_head)), (void *)(&msg_body),
			sizeof(struct create_fdrv_struct));
	Flush_Dcache_By_Area((unsigned long)message_buff, (unsigned long)message_buff + MESSAGE_SIZE);

	/* Call the smc_fast_call */
	down(&(smc_lock));

	invoke_fastcall();

	down(&(boot_sema));

	Invalidate_Dcache_By_Area((unsigned long)message_buff, (unsigned long)message_buff + MESSAGE_SIZE);
	memcpy((void *)(&msg_head), (void *)(message_buff), sizeof(struct message_head));
	memcpy((void *)(&msg_ack), (void *)(message_buff + sizeof(struct message_head)),
			sizeof(struct ack_fast_call_struct));

	/*local_irq_restore(irq_flag);*/

	/* Check the response from T_OS. */
	if ((msg_head.message_type == FAST_CALL_TYPE) && (msg_head.child_type == FAST_ACK_CREAT_FDRV)) {
		retVal = msg_ack.retVal;

		if (retVal == 0)
			return temp_addr;

	} else {
		retVal = (unsigned long)NULL;
	}

	/* Release the resource and return. */
	free_pages((unsigned long)temp_addr, get_order(ROUND_UP(buff_size, SZ_4K)));

	IMSG_ERROR("[%s][%d]: %s failed!\n", __func__, __LINE__, __func__);
	return retVal;
}

void set_fp_command(unsigned long memory_size)
{
	struct fdrv_message_head fdrv_msg_head;

	memset((void *)(&fdrv_msg_head), 0, sizeof(struct fdrv_message_head));

	fdrv_msg_head.driver_type = FP_SYS_NO;
	fdrv_msg_head.fdrv_param_length = sizeof(unsigned int);

	memcpy((void *)fdrv_message_buff, (void *)(&fdrv_msg_head), sizeof(struct fdrv_message_head));

	Flush_Dcache_By_Area((unsigned long)fdrv_message_buff, (unsigned long)fdrv_message_buff + MESSAGE_SIZE);
}

int __send_fp_command(unsigned long share_memory_size)
{
	uint64_t smc_type = 2;

	set_fp_command(share_memory_size);
	Flush_Dcache_By_Area((unsigned long)fp_buff_addr, fp_buff_addr + FP_BUFF_SIZE);

	fp_call_flag = GLSCH_HIGH;
	n_invoke_t_drv(&smc_type, 0, 0);

	while (smc_type == 0x54)
		nt_sched_t(&smc_type);

	return 0;
}

int send_fp_command(unsigned long share_memory_size)
{

	struct fdrv_call_struct fdrv_ent;

	int retVal = 0;

	down(&fdrv_lock);
	ut_pm_mutex_lock(&pm_mutex);
	down(&smc_lock);
	IMSG_DEBUG("send_fp_command start\n");

	if (teei_config_flag == 1)
		complete(&global_down_lock);

	fdrv_ent.fdrv_call_type = FP_SYS_NO;
	fdrv_ent.fdrv_call_buff_size = share_memory_size;

	/* with a wmb() */

	Flush_Dcache_By_Area((unsigned long)&fdrv_ent, (unsigned long)&fdrv_ent + sizeof(struct fdrv_call_struct));
	retVal = add_work_entry(FDRV_CALL, (unsigned char *)(&fdrv_ent));

	if (retVal != 0) {
		up(&smc_lock);
		ut_pm_mutex_unlock(&pm_mutex);
		up(&fdrv_lock);
		return retVal;
	}

	down(&fdrv_sema);
	IMSG_DEBUG("send_fp_command end\n");
	/* with a rmb() */

	Invalidate_Dcache_By_Area((unsigned long)fp_buff_addr, (unsigned long)fp_buff_addr + FP_BUFF_SIZE);

	ut_pm_mutex_unlock(&pm_mutex);
	up(&fdrv_lock);


	return fdrv_ent.retVal;
}
