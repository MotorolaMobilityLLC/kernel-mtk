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
#include "sspm_sysfs.h"
#include "sspm_timesync.h"
#include "sspm_reservedmem.h"

#define SEM_TIMEOUT		5000
#define SSPM_INIT_FLAG	0x1

static int __init sspm_init(void);

struct sspm_regs sspmreg;
static struct workqueue_struct *sspm_workqueue;
static atomic_t sspm_inited = ATOMIC_INIT(0);
static unsigned int sspm_ready;

void memcpy_to_sspm(void __iomem *trg, const void *src, int size)
{
	int i, len;
	u32 __iomem *t = trg;
	const u32 *s = src;

	len = (size + 3) >> 2;

	for (i = 0; i < len; i++)
		*t++ = *s++;
}
EXPORT_SYMBOL_GPL(memcpy_to_sspm);

void memcpy_from_sspm(void *trg, const void __iomem *src, int size)
{
	int i, len;
	u32 *t = trg;
	const u32 __iomem *s = src;

	len = (size + 3) >> 2;

	for (i = 0; i < len; i++)
		*t++ = *s++;
}
EXPORT_SYMBOL_GPL(memcpy_from_sspm);

/*
 * acquire a hardware semaphore
 * @param flag: semaphore id
 */
int get_sspm_semaphore(int flag)
{
	void __iomem *sema = sspmreg.cfg + SSPM_CFG_OFS_SEMA;
	int read_back;
	int count = 0;
	int ret = -1;

	flag = (flag * 2) + 1;

	read_back = (readl(sema) >> flag) & 0x1;

	if (read_back == 0) {
		writel((1 << flag), sema);

		while (count != SEM_TIMEOUT) {
			/* repeat test if we get semaphore */
			read_back = (readl(sema) >> flag) & 0x1;
			if (read_back == 1) {
				ret = 1;
				break;
			}
			writel((1 << flag), sema);
			count++;
		}

		if (ret < 0)
			pr_debug("[SSPM] get semaphore %d TIMEOUT...!\n", flag);
	} else {
		pr_debug("[SSPM] already hold semaphore %d\n", flag);
	}

	return ret;
}
EXPORT_SYMBOL_GPL(get_sspm_semaphore);

/*
 * release a hardware semaphore
 * @param flag: semaphore id
 */
int release_sspm_semaphore(int flag)
{
	void __iomem *sema = sspmreg.cfg + SSPM_CFG_OFS_SEMA;
	int read_back;
	int ret = -1;

	flag = (flag * 2) + 1;

	read_back = (readl(sema) >> flag) & 0x1;

	if (read_back == 1) {
		/* Write 1 clear */
		writel((1 << flag), sema);
		read_back = (readl(sema) >> flag) & 0x1;
		if (read_back == 0)
			ret = 1;
		else
			pr_debug("[SSPM] release semaphore %d failed!\n", flag);
	} else {
		pr_debug("[SSPM] try to release semaphore %d not own by me\n", flag);
	}

	return ret;
}
EXPORT_SYMBOL_GPL(release_sspm_semaphore);

/*
 * schedule a work on sspm's work queue
 * @param sspm_ws: work_struct to schedule
 */
void sspm_schedule_work(struct sspm_work_struct *sspm_ws)
{
	queue_work(sspm_workqueue, &sspm_ws->work);
}
EXPORT_SYMBOL_GPL(sspm_schedule_work);

/*
 * @return: 1 if sspm is ready for running tasks
 */
unsigned int is_sspm_ready(void)
{
	if (sspm_ready)
		return 1;
	else
		return 0;
}
EXPORT_SYMBOL_GPL(is_sspm_ready);

/*
 * parse device tree and mapping iomem
 * @return: 0 if success
 */
static int sspm_dt_init(void)
{
	int ret = 0;
	struct device_node *node = NULL;
	struct resource res;

	node = of_find_compatible_node(NULL, NULL, "mediatek,sspm");
	if (!node) {
		pr_err("[SSPM] Can't find node: mediatek,sspm\n");
		return -1;
	}

	sspmreg.cfg = of_iomap(node, 0);

	if (!sspmreg.cfg) {
		pr_err("[SSPM] Unable to ioremap registers\n");
		return -1;
	}

	if (of_address_to_resource(node, 0, &res)) {
		pr_err("[SSPM] Unable to get dts resource (CFGREG Size)\n");
		return -1;
	}
	sspmreg.cfgregsize = (unsigned int) resource_size(&res);

	sspmreg.irq = irq_of_parse_and_map(node, 0);

	if (!sspmreg.irq) {
		pr_err("[SSPM] Unable to get IRQ\n");
		return -1;
	}

	pr_debug("[SSPM] sspm irq=%d, cfgreg=0x%p\n", sspmreg.irq, sspmreg.cfg);

	return ret;
}

static inline ssize_t sspm_status_show(struct device *kobj, struct device_attribute *attr, char *buf)
{
		return snprintf(buf, PAGE_SIZE, "SSPM is %s ready", sspm_ready ? "" : "not");
}

DEVICE_ATTR(sspm_status, S_IRUSR | S_IRGRP | S_IROTH, sspm_status_show, NULL);

void sspm_lazy_init(void)
{
	if (unlikely(atomic_read(&sspm_inited) == 0))
		sspm_init();
}
EXPORT_SYMBOL_GPL(sspm_lazy_init);

/*
 * driver initialization entry point
 */
static int __init sspm_init(void)
{
	if (atomic_inc_return(&sspm_inited) != 1)
		return 0;

	/* static initialise */
	sspm_ready = 0;

	sspm_workqueue = create_workqueue("SSPM_WQ");

	if (!sspm_workqueue) {
		pr_err("[SSPM] Workqueue Create Failed\n");
		goto error;
	}

	if (sspm_excep_init()) {
		pr_err("[SSPM] Excep Init Failed\n");
		goto error;
	}

	if (sspm_dt_init()) {
		pr_err("[SSPM] Device Init Failed\n");
		goto error;
	}

	if (sspm_ipi_init()) {
		pr_err("[SSPM] IPI Init Failed\n");
		goto error;
	}

#ifdef CONFIG_OF_RESERVED_MEM
	if (sspm_reserve_memory_init()) {
		pr_err("[SSPM] Reserved Memory Failed\n");
		goto error;
	}
#endif

	pr_debug("[SSPM] Helper Init\n");

#if 0
	if (sspm_irq_init(sspmreg.irq)) {
		pr_err("[SSPM] IRQ Init Failed\n");
		return -1;
	}
#endif
	sspm_ready = 1;

	atomic_set(&sspm_inited, 1);
	return 0;

error:
	atomic_set(&sspm_inited, 1);
	return -1;
}

static int __init sspm_module_init(void)
{
	if (sspm_sysfs_init()) {
		pr_err("[SSPM] Sysfs Init Failed\n");
		return -1;
	}

#if SSPM_PLT_SERV_SUPPORT
	if (sspm_plt_init()) {
		pr_err("[SSPM] Platform Init Failed\n");
		return -1;
	}
#endif

	sspm_lock_emi_mpu();

	return 0;
}

early_initcall(sspm_init);
module_init(sspm_module_init);
