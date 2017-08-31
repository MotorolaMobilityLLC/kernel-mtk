/*
 * Copyright (C) 2011-2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/types.h>
#include <linux/module.h>       /* needed by all modules */
#include <linux/init.h>         /* needed by module macros */
#include <linux/fs.h>           /* needed by file_operations* */
#include <linux/miscdevice.h>   /* needed by miscdevice* */
#include <linux/device.h>       /* needed by device_* */
#include <linux/vmalloc.h>      /* needed by kmalloc */
#include <linux/uaccess.h>      /* needed by copy_to_user */
#include <linux/fs.h>           /* needed by file_operations* */
#include <linux/slab.h>         /* needed by kmalloc */
#include <linux/poll.h>         /* needed by poll */
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/suspend.h>
#include <linux/timer.h>
#include <linux/notifier.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_fdt.h>
#include <linux/ioport.h>
#include <linux/io.h>
#include <linux/atomic.h>
#include <mt-plat/sync_write.h>
#include <mt-plat/aee.h>
#include "sspm_define.h"
#include "sspm_helper.h"
#include "sspm_ipi.h"
#include "sspm_excep.h"
#include "sspm_reservedmem.h"
#include "sspm_reservedmem_define.h"
#include "sspm_logger.h"
#include "sspm_timesync.h"

#ifdef SSPM_PLT_SERV_SUPPORT

struct plt_ctrl_s {
	unsigned int magic;
	unsigned int size;
	unsigned int mem_sz;
#ifdef SSPM_LOGGER_SUPPORT
	unsigned int logger_ofs;
#endif
#ifdef SSPM_COREDUMP_SUPPORT
	unsigned int coredump_ofs;
#endif
#ifdef SSPM_TIMESYNC_SUPPORT
	unsigned int ts_ofs;
#endif

};

static struct plt_ctrl_s *plt_ctl;
static struct ipi_action dev;
static struct task_struct *sspm_task;

/* dont send any IPI from this thread, use workqueue instead */
static int sspm_recv_thread(void *userdata)
{
	struct plt_ipi_data_s data;
	unsigned int rdata, ret;

	dev.data = &data;

	ret = sspm_ipi_recv_registration(IPI_ID_PLATFORM, &dev);

	do {
		rdata = 0;
		sspm_ipi_recv_wait(IPI_ID_PLATFORM);

		switch (data.cmd) {
#ifdef SSPM_LASTK_SUPPORT
		case PLT_LASTK_READY:
			sspm_log_lastk_recv(data.u.logger.enable);
			break;
#endif
#if defined(SSPM_COREDUMP_SUPPORT) && defined(DEBUG)
		case PLT_COREDUMP_READY:
			sspm_log_coredump_recv(data.u.coredump.exists);
			break;
#endif
		}
		sspm_ipi_send_ack(IPI_ID_PLATFORM, &rdata);
	} while (!kthread_should_stop());

	return 0;
}

int __init sspm_plt_init(void)
{
	phys_addr_t phys_addr, virt_addr, mem_sz;
	struct plt_ipi_data_s ipi_data;
	int ret, ackdata;
	unsigned int last_ofs, last_sz;
	unsigned int *mark;
	unsigned char *b;

	phys_addr = sspm_reserve_mem_get_phys(SSPM_MEM_ID);
	if (phys_addr == 0) {
		pr_err("SSPM: Can't get logger phys mem\n");
		goto error;
	}

	virt_addr = sspm_reserve_mem_get_virt(SSPM_MEM_ID);
	if (virt_addr == 0) {
		pr_err("SSPM: Can't get logger virt mem\n");
		goto error;
	}

	mem_sz = sspm_reserve_mem_get_size(SSPM_MEM_ID);
	if (mem_sz == 0) {
		pr_err("SSPM: Can't get logger mem size\n");
		goto error;
	}

	sspm_task = kthread_run(sspm_recv_thread, NULL, "sspm_recv");

	b = (unsigned char *) virt_addr;
	for (last_ofs = 0; last_ofs < sizeof(*plt_ctl); last_ofs++)
		b[last_ofs] = 0x0;

	mark = (unsigned int *) virt_addr;
	*mark = PLT_INIT;
	mark = (unsigned int *) ((unsigned char *) virt_addr + mem_sz - 4);
	*mark = PLT_INIT;

	plt_ctl = (struct plt_ctrl_s *) virt_addr;
	plt_ctl->magic = PLT_INIT;
	plt_ctl->size = sizeof(*plt_ctl);
	plt_ctl->mem_sz = mem_sz;

	last_ofs = plt_ctl->size;


	pr_debug("SSPM: %s(): after plt, ofs=%u\n", __func__, last_ofs);

#ifdef SSPM_LOGGER_SUPPORT
	plt_ctl->logger_ofs = last_ofs;
	last_sz = sspm_logger_init(virt_addr + last_ofs, mem_sz - last_ofs);

	if (last_sz == 0) {
		pr_err("SSPM: sspm_logger_init return fail\n");
		goto error;
	}

	last_ofs += last_sz;
	pr_debug("SSPM: %s(): after logger, ofs=%u\n", __func__, last_ofs);
#endif

#ifdef SSPM_COREDUMP_SUPPORT
	plt_ctl->coredump_ofs = last_ofs;
	last_sz = sspm_coredump_init(virt_addr + last_ofs, mem_sz - last_ofs);

	if (last_sz == 0) {
		pr_err("SSPM: sspm_coredump_init return fail\n");
		goto error;
	}

	last_ofs += last_sz;
	pr_debug("SSPM: %s(): after coredump, ofs=%u\n", __func__, last_ofs);
#endif

#ifdef SSPM_TIMESYNC_SUPPORT
	plt_ctl->ts_ofs = last_ofs;
	last_sz = sspm_timesync_init(virt_addr + last_ofs, mem_sz - last_ofs);

	if (last_sz == 0) {
		pr_err("SSPM: sspm_timesync_init return fail\n");
		goto error;
	}

	last_ofs += last_sz;
	pr_debug("SSPM: %s(): after timesync, ofs=%u\n", __func__, last_ofs);
#endif

	ipi_data.cmd = PLT_INIT;
	ipi_data.u.ctrl.phys = phys_addr;
	ipi_data.u.ctrl.size = mem_sz;

	ret = sspm_ipi_send_sync(IPI_ID_PLATFORM, IPI_OPT_LOCK_BUSY, &ipi_data,
			sizeof(ipi_data) / MBOX_SLOT_SIZE, &ackdata);
	if (ret != 0) {
		pr_err("SSPM: logger IPI fail ret=%d\n", ret);
		goto error;
	}

	if (!ackdata) {
		pr_err("SSPM: logger IPI init fail, ret=%d\n", ackdata);
		goto error;
	}

#ifdef SSPM_TIMESYNC_SUPPORT
	sspm_timesync_init_done();
#endif
	sspm_coredump_init_done();
#ifdef SSPM_LOGGER_SUPPORT
	sspm_logger_init_done();
#endif

	return 0;

error:
	return -1;
}
#endif
