/*
* Copyright (C) 2016 MediaTek Inc.
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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/unistd.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/errno.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/string.h>
#include <linux/random.h>
#include <linux/memory.h>
#include <linux/io.h>
#include <linux/proc_fs.h>

#include <mt_vcorefs_manager.h>

#ifdef CONFIG_TRUSTONIC_TEE_SUPPORT
#include "mobicore_driver_api.h"
#include "drspi_Api.h"

static struct mc_uuid_t spi_uuid = {DRV_DBG_UUID};
static struct mc_session_handle spi_session = {0};
static u32 spi_devid = MC_DEVICE_ID_DEFAULT;
static dciMessage_t *spi_dci;
#endif


#define EX_NAME "example"

#define DEFAULT_HANDLES_NUM (64)
#define MAX_OPEN_SESSIONS (0xffffffff - 1)

/* Debug message event */
#define	DBG_EVT_NONE	(0) /* No event */
#define	DBG_EVT_CMD		(1 << 0)/* SEC CMD related event */
#define	DBG_EVT_FUNC	(1 << 1)/* SEC function event */
#define	DBG_EVT_INFO	(1 << 2)/* SEC information event */
#define	DBG_EVT_WRN		(1 << 30) /* Warning event */
#define	DBG_EVT_ERR		(1 << 31) /* Error event */
#define	DBG_EVT_ALL		(0xffffffff)

#define DBG_EVT_MASK (DBG_EVT_ERR)

#define MSG(evt, fmt, args...) \
do {\
	if ((DBG_EVT_##evt) & DBG_EVT_MASK) { \
		pr_err("[%s] "fmt, EX_NAME, ##args); \
	} \
} while (0)


struct task_struct *spiOpen_th;
struct task_struct *spiDci_th;

static int spi_execute(u32 cmdId)
{
	switch (cmdId) {
	case DCI_SPI_CMD_RELEASE_DVFS:
		MSG(INFO, "%s: DCI_spi_CMD_REL_DVFS.\n", __func__);
		vcorefs_request_dvfs_opp(KIR_TEESPI, OPPI_UNREQ);
		break;

	case DCI_SPI_CMD_SET_DVFS:
		MSG(INFO, "%s: DCI_spi_CMD_SET_DVFS.\n", __func__);
		vcorefs_request_dvfs_opp(KIR_TEESPI, OPPI_PERF);
		break;

	default:
		MSG(ERR, "%s: receive an unknown command id(%d).\n", __func__, cmdId);
		break;
	}

	return 0;
}

int spi_listenDci(void *data)
{
	enum mc_result mc_ret;
	u32 cmdId;

	MSG(INFO, "%s: DCI listener.\n", __func__);
	for (;;) {
		MSG(INFO, "%s: Waiting for notification\n", __func__);
		/* Wait for notification from SWd */
		mc_ret = mc_wait_notification(&spi_session, MC_INFINITE_TIMEOUT);
		if (mc_ret != MC_DRV_OK) {
			MSG(ERR, "%s: mcWaitNotification failed, mc_ret=%d\n", __func__, mc_ret);
			break;
		}
		cmdId = spi_dci->cmd_spi.header.commandId;
		MSG(INFO, "%s: wait notification done!! cmdId = %x\n", __func__, cmdId);
		/* Received exception. */
		mc_ret = spi_execute(cmdId);

		/* Notify the STH*/
		mc_ret = mc_notify(&spi_session);
		if (mc_ret != MC_DRV_OK) {
			MSG(ERR, "%s: mcNotify returned: %d\n", __func__, mc_ret);
			break;
		}
	}
	return 0;
}


static int spi_open_session(void *context)
{
	int cnt = 0;
	enum mc_result mc_ret = MC_DRV_ERR_UNKNOWN;

	MSG(INFO, "%s start\n", __func__);
	do {
		msleep(2000);

		/* open device */
		mc_ret = mc_open_device(spi_devid);
		if (mc_ret != MC_DRV_OK) {
			MSG(ERR, "%s, mc_open_device failed: %d\n", __func__, mc_ret);
			cnt++;
			continue;
		}
		MSG(INFO, "%s, mc_open_device success.\n", __func__);
		/* allocating WSM for DCI */
		mc_ret = mc_malloc_wsm(spi_devid, 0, sizeof(dciMessage_t), (uint8_t **)&spi_dci, 0);
		if (mc_ret != MC_DRV_OK) {
			mc_close_device(spi_devid);
			MSG(ERR, "%s, mc_malloc_wsm failed: %d\n", __func__, mc_ret);
			cnt++;
			continue;
		}

		MSG(INFO, "%s, mc_malloc_wsm success.\n", __func__);
		MSG(INFO, "uuid[0]=%d, uuid[1]=%d, uuid[2]=%d, uuid[3]=%d\n",
			spi_uuid.value[0], spi_uuid.value[1], spi_uuid.value[2], spi_uuid.value[3]);

		spi_session.device_id = spi_devid;

		/* open session */
		mc_ret = mc_open_session(&spi_session,
					 &spi_uuid,
					 (uint8_t *) spi_dci,
					 sizeof(dciMessage_t));

		if (mc_ret != MC_DRV_OK) {
			MSG(ERR, "%s, mc_open_session failed. cnt = %d, mc_ret = %d\n", __func__, cnt, mc_ret);

			mc_ret = mc_free_wsm(spi_devid, (uint8_t *)spi_dci);
			MSG(ERR, "%s, free wsm result (%d)\n", __func__, mc_ret);

			mc_ret = mc_close_device(spi_devid);
			MSG(ERR, "%s, try free wsm and close device\n", __func__);
			cnt++;
			continue;
		}
		/* create a thread for listening DCI signals */
		spiDci_th = kthread_run(spi_listenDci, NULL, "spi_Dci");
		if (IS_ERR(spiDci_th))
			MSG(ERR, "%s, init kthread_run failed!\n", __func__);
		else
			break;

	} while (cnt < 30);
	if (cnt >= 30)
		MSG(ERR, "%s, open session failed!!!\n", __func__);

	MSG(INFO, "%s end, mc_ret = %x\n", __func__, mc_ret);
	return mc_ret;
}

static int __init example_init(void)
{
	MSG(INFO, "%s start\n", __func__);
#ifdef CONFIG_TRUSTONIC_TEE_SUPPORT
	spiOpen_th = kthread_run(spi_open_session, NULL, "ex_open");
	if (IS_ERR(spiOpen_th))
		MSG(ERR, "%s, init kthread_run failed!\n", __func__);
#endif
	MSG(INFO, "%s end!!!!\n", __func__);

	return 0;
}
late_initcall(example_init);
