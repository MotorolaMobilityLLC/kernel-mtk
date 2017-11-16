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

#include <backward_driver.h>
#include <isee_kernel_api.h>
#include <tz_dcih.h>
#include <teei_common.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/gfp.h>
#include <linux/semaphore.h>
#include <nt_smc_call.h>
#include <sched_status.h>
#include <switch_queue.h>
#include <teei_client_main.h>
#include <teei_id.h>
#include <utdriver_macro.h>

#define IMSG_TAG "[tz_dcih]"
#include <imsg_log.h>

static struct drm_dcih_info drm_dcih_req[TOTAL_DRM_DRIVER_NUM];
static unsigned int drm_driver_id;

static unsigned long create_dcih_buffer(unsigned int dcih_id, unsigned int driver_id, unsigned int buff_size)
{
	long retVal = 0;
	unsigned long temp_addr = 0;
	struct message_head msg_head;
	struct create_fdrv_struct msg_body;
	struct ack_fast_call_struct msg_ack;

	if ((unsigned char *)message_buff == NULL) {
		IMSG_ERROR("[%s][%d]: There is NO command buffer!.\n", __func__, __LINE__);
		return -ENOMEM;
	}

	if (buff_size > VDRV_MAX_SIZE) {
		IMSG_ERROR("[%s][%d]: Drv buffer is too large, Can NOT create it.\n", __FILE__, __LINE__);
		return -ENOMEM;
	}

#ifdef UT_DMA_ZONE
	temp_addr = (unsigned long) __get_free_pages(GFP_KERNEL | GFP_DMA, get_order(ROUND_UP(buff_size, SZ_4K)));
#else
	temp_addr = (unsigned long) __get_free_pages(GFP_KERNEL, get_order(ROUND_UP(buff_size, SZ_4K)));
#endif
	if ((unsigned char *)temp_addr == NULL) {
		IMSG_ERROR("[%s][%d]: kmalloc keymaster drv buffer failed.\n", __FILE__, __LINE__);
		return -ENOMEM;
	}

	memset(&msg_head, 0, sizeof(struct message_head));
	memset(&msg_body, 0, sizeof(struct create_fdrv_struct));
	memset(&msg_ack, 0, sizeof(struct ack_fast_call_struct));

	msg_head.invalid_flag = VALID_TYPE;
	msg_head.message_type = FAST_CALL_TYPE;
	msg_head.child_type = FAST_CREAT_FDRV;
	msg_head.param_length = sizeof(struct create_fdrv_struct);

	msg_body.fdrv_type = driver_id;
	msg_body.fdrv_phy_addr = virt_to_phys((void *)temp_addr);
	msg_body.fdrv_size = buff_size;

	drm_dcih_req[dcih_id].virt_addr = temp_addr;
	drm_dcih_req[dcih_id].phy_addr = virt_to_phys((void *)temp_addr);
	drm_dcih_req[dcih_id].buf_size = buff_size;

	/* Notify the T_OS that there is ctl_buffer to be created. */
	memcpy((void *)message_buff, (void *)&msg_head, sizeof(struct message_head));
	memcpy((void *)(message_buff + sizeof(struct message_head)), (void *)&msg_body,
			sizeof(struct create_fdrv_struct));
	Flush_Dcache_By_Area((unsigned long)message_buff, (unsigned long)message_buff + MESSAGE_SIZE);

	/* Call the smc_fast_call */
	down(&(smc_lock));
	invoke_fastcall();
	down(&(boot_sema));

	Invalidate_Dcache_By_Area((unsigned long)message_buff, (unsigned long)message_buff + MESSAGE_SIZE);
	memcpy((void *)&msg_head, (void *)message_buff, sizeof(struct message_head));
	memcpy((void *)&msg_ack, (void *)(message_buff + sizeof(struct message_head)),
			sizeof(struct ack_fast_call_struct));

	/* Check the response from T_OS. */
	if ((msg_head.message_type == FAST_CALL_TYPE) && (msg_head.child_type == FAST_ACK_CREAT_FDRV)) {
		retVal = msg_ack.retVal;

		if (retVal == 0) {
			/* IMSG_DEBUG("[%s][%d]: %s end.\n", __func__, __LINE__, __func__); */
			return retVal;
		}
	} else
		retVal = -ENOMEM;

	/* Release the resource and return. */
	free_pages(temp_addr, get_order(ROUND_UP(buff_size, SZ_4K)));

	IMSG_ERROR("[%s][%d]: %s failed!\n", __func__, __LINE__, __func__);
	return retVal;
}

unsigned long tz_create_share_buffer(unsigned int driver_id, unsigned int buff_size, enum drm_dcih_buf_mode mode)
{
	int i = 0;
	int retVal = 0;

	if (buff_size > SZ_4K) {
		IMSG_ERROR("[%s][%d]: buffer size too large!\n", __func__, __LINE__);
		return -1;
	}

	for (i = 0; i < TOTAL_DRM_DRIVER_NUM; i++) {
		if (drm_dcih_req[i].dcih_id != driver_id && !drm_dcih_req[i].is_inited)
			break;
	}

	if (i >= TOTAL_DRM_DRIVER_NUM) {
		IMSG_ERROR("[%s][%d]: too many driver_id, only support 8 services!\n", __func__, __LINE__);
		return -1;
	}

	drm_dcih_req[i].dcih_id = driver_id;
	drm_dcih_req[i].buf_mode = mode;
	retVal = create_dcih_buffer(i, driver_id, SZ_4K);
	if (retVal) {
		IMSG_ERROR("[%s][%d] create DCIH buffer failed!\n", __func__, __LINE__);
		return -ENOMEM;
	}
	drm_dcih_req[i].is_inited = 1;
	IMSG_DEBUG("[%s][%d][%d] create DCIH driver %d buffer addr is 0x%lx!\n", __func__, __LINE__,
			i, drm_dcih_req[i].dcih_id, drm_dcih_req[i].phy_addr);
	return 0;
}

void init_dcih_service(void)
{
	int i = 0;

	memset(&drm_dcih_req[0], 0x0, sizeof(struct drm_dcih_info)*TOTAL_DRM_DRIVER_NUM);
	for (i = 0; i < TOTAL_DRM_DRIVER_NUM; i++) {
		init_completion(&drm_dcih_req[i].tee_signal);
		init_completion(&drm_dcih_req[i].ree_signal);
	}

	/* In current stage, only 2 SVP drivers needs DCIH support for WVL1 modular DRM feature.
	 * Only extend when there are more needs.
	 */
	tz_create_share_buffer(DRM_M4U_DRV_DRIVER_ID, SZ_4K, DRM_DCIH_BUF_MODE_FORWARD);
	tz_create_share_buffer(DRM_WVL1_MODULAR_DRV_DRIVER_ID, SZ_4K, DRM_DCIH_BUF_MODE_BACKWARD);
}

static int tz_get_notify_driver_id(void)
{
	return drm_driver_id;
}

static void send_drm_command(unsigned int driver_id)
{
	struct fdrv_message_head fdrv_msg_head;

	IMSG_INFO("[%s][%d]", __func__, __LINE__);

	memset(&fdrv_msg_head, 0, sizeof(struct fdrv_message_head));
	fdrv_msg_head.driver_type = driver_id;
	fdrv_msg_head.fdrv_param_length = sizeof(unsigned int);
	memcpy((void *)fdrv_message_buff, (void *)&fdrv_msg_head, sizeof(struct fdrv_message_head));
	Flush_Dcache_By_Area((unsigned long)fdrv_message_buff, (unsigned long)fdrv_message_buff + MESSAGE_SIZE);
}

int __send_drm_command(unsigned long share_memory_size)
{
	unsigned long smc_type = 2;
	unsigned int drm_id;
	int i;

	drm_id = tz_get_notify_driver_id();
	for (i = 0; i < TOTAL_DRM_DRIVER_NUM; i++) {
		if (drm_dcih_req[i].dcih_id == drm_id) {
			send_drm_command(drm_id);
			Flush_Dcache_By_Area((unsigned long)(drm_dcih_req[i].virt_addr),
					(drm_dcih_req[i].virt_addr) + SZ_4K);

			fp_call_flag = GLSCH_HIGH;
			n_invoke_t_drv((uint64_t *)&smc_type, 0, 0);

			while (smc_type == 0x54) {
				nt_sched_t((uint64_t *)&smc_type);
			}
			IMSG_DEBUG("[%s][%d] driver id %d buffer addr is 0x%lx!\n", __func__, __LINE__,
					drm_dcih_req[i].dcih_id, drm_dcih_req[i].virt_addr);
		}
	}

	return 0;
}

static int send_drm_command_queue(unsigned int driver_id, unsigned long share_memory)
{

	struct fdrv_call_struct fdrv_ent;
	int retVal = 0;

	down(&fdrv_lock);
	ut_pm_mutex_lock(&pm_mutex);

	down(&smc_lock);

	if (teei_config_flag == 1)
		complete(&global_down_lock);

	fdrv_ent.fdrv_call_type = DRM_SYS_NO;
	fdrv_ent.fdrv_call_buff_size = SZ_4K;

	/* Set the driver ID for later reference by worker */
	drm_driver_id = driver_id;

	/* with a wmb() */
	wmb();

	Flush_Dcache_By_Area((unsigned long)&fdrv_ent, (unsigned long)&fdrv_ent + sizeof(struct fdrv_call_struct));
	retVal = add_work_entry(FDRV_CALL, (unsigned long)&fdrv_ent);
	if (retVal != 0) {
		ut_pm_mutex_unlock(&pm_mutex);
		up(&fdrv_lock);
		return retVal;
	}

	down(&fdrv_sema);

	/* with a rmb() */
	rmb();

	Invalidate_Dcache_By_Area((unsigned long)share_memory, share_memory + SZ_4K);

	ut_pm_mutex_unlock(&pm_mutex);
	up(&fdrv_lock);

	return fdrv_ent.retVal;
}

unsigned long tz_get_share_buffer(unsigned int driver_id)
{
	int i;

	for (i = 0; i < TOTAL_DRM_DRIVER_NUM; i++) {
		if (drm_dcih_req[i].dcih_id == driver_id && drm_dcih_req[i].is_inited)
			return drm_dcih_req[i].virt_addr;
	}

	IMSG_DEBUG("[%s][%d]: driver buffer 0x%x is still not initialized!\n", __func__, __LINE__, driver_id);
	return (unsigned long)NULL;
}
EXPORT_SYMBOL(tz_get_share_buffer);

int tz_notify_driver(unsigned int driver_id)
{
	int i;

	for (i = 0; i < TOTAL_DRM_DRIVER_NUM; i++) {
		if (drm_dcih_req[i].dcih_id == driver_id && drm_dcih_req[i].is_inited) {
			if (!drm_dcih_req[i].is_shared) {
				/* Always allow the first notify call to init the sharing with secure drivers */
				drm_dcih_req[i].is_shared = 1;
				/* This is for first time establish share memory connection with secure drivers */
				send_drm_command_queue(driver_id, drm_dcih_req[i].virt_addr);
			} else if (drm_dcih_req[i].buf_mode == DRM_DCIH_BUF_MODE_BACKWARD) {
				Flush_Dcache_By_Area((unsigned long)drm_dcih_req[i].virt_addr,
						drm_dcih_req[i].virt_addr + SZ_4K);
				complete(&drm_dcih_req[i].ree_signal);
			} else if ((unsigned char *)drm_dcih_req[i].virt_addr != NULL) {
				send_drm_command_queue(driver_id, drm_dcih_req[i].virt_addr);
				Invalidate_Dcache_By_Area((unsigned long)drm_dcih_req[i].virt_addr,
						drm_dcih_req[i].virt_addr + SZ_4K);
			}
			return 0;
		}
	}

	IMSG_DEBUG("[%s][%d]: driver buffer 0x%x is still not initialized!\n", __func__, __LINE__, driver_id);
	return -EIO;
}
EXPORT_SYMBOL(tz_notify_driver);

/* This API is used only by backward driver (TEE->REE) */
int tz_wait_for_notification(unsigned int driver_id)
{
	int i;

	for (i = 0; i < TOTAL_DRM_DRIVER_NUM; i++) {
		if (drm_dcih_req[i].dcih_id == driver_id && drm_dcih_req[i].is_inited &&
				drm_dcih_req[i].buf_mode == DRM_DCIH_BUF_MODE_BACKWARD) {
			wait_for_completion_interruptible(&drm_dcih_req[i].tee_signal);
			return 0;
		}
	}

	IMSG_DEBUG("[%s][%d]: driver 0x%x wait for notification failed!\n", __func__, __LINE__, driver_id);
	return -EIO;
}
EXPORT_SYMBOL(tz_wait_for_notification);

/* This API is used only by backward driver (TEE->REE) */
int tz_sec_drv_notification(unsigned int driver_id)
{
	int i, ret;

	for (i = 0; i < TOTAL_DRM_DRIVER_NUM; i++) {
		if (drm_dcih_req[i].dcih_id == driver_id && drm_dcih_req[i].is_inited &&
				drm_dcih_req[i].buf_mode == DRM_DCIH_BUF_MODE_BACKWARD) {
			complete(&drm_dcih_req[i].tee_signal);
			/* Set 5s timeout in case if ree driver is not waiting for the event */
			ret = wait_for_completion_interruptible_timeout(&drm_dcih_req[i].ree_signal, 5*HZ);
			if (ret < 0) {
				return -ERESTARTSYS;
			} else if (ret == 0) {
				IMSG_ERROR("[%s][%d]: sec drv notify for driver 0x%x timeout!\n", __func__,
						__LINE__, driver_id);
				return -ETIMEDOUT;
			}
			return 0;
		}
	}

	IMSG_ERROR("[%s][%d]: sec drv notify for driver 0x%x failed!\n", __func__, __LINE__, driver_id);
	return -EIO;
}
