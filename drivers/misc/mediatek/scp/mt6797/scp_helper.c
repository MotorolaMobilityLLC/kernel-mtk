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
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/suspend.h>
#include <linux/timer.h>
#include <linux/notifier.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/ioport.h>

#include <asm/io.h>

#include <mt-plat/sync_write.h>
#include <mt-plat/aee.h>
#include "scp_ipi.h"
#include "scp_helper.h"
#include "scp_excep.h"



#define TIMEOUT 5000
/*
#define LOG_TO_AP_UART
*/
#define IS_CLK_DMA_EN 0x40000
#define SCP_READY_TIMEOUT (2 * HZ) /* 2 seconds*/

struct scp_regs scpreg;

unsigned char *scp_send_buff;
unsigned char *scp_recv_buff;

static struct workqueue_struct *scp_workqueue;
static unsigned int scp_ready;
static struct timer_list scp_ready_timer;
static struct scp_work_struct scp_notify_work;
static struct mutex scp_notify_mutex;

unsigned char **scp_swap_buf;

void memcpy_to_scp(void __iomem *trg, const void *src, int size)
{
	int i;
	u32 __iomem *t = trg;
	const u32 *s = src;

	for (i = 0; i < ((size + 3) >> 2); i++)
		*t++ = *s++;
}

void memcpy_from_scp(void *trg, const void __iomem *src, int size)
{
	int i;
	u32 *t = trg;
	const u32 __iomem *s = src;

	for (i = 0; i < ((size + 3) >> 2); i++)
		*t++ = *s++;
}

/*
 * acquire a hardware semaphore
 * @param flag: semaphore id
 */
int get_scp_semaphore(int flag)
{
	int read_back;
	int count = 0;
	int ret = -1;

	flag = (flag * 2) + 1;

	read_back = (readl(SCP_SEMAPHORE) >> flag) & 0x1;

	if (read_back == 0) {
		writel((1 << flag), SCP_SEMAPHORE);

		while (count != TIMEOUT) {
			/* repeat test if we get semaphore */
			read_back = (readl(SCP_SEMAPHORE) >> flag) & 0x1;
			if (read_back == 1) {
				ret = 1;
				break;
			}
			writel((1 << flag), SCP_SEMAPHORE);
			count++;
		}

		if (ret < 0)
			pr_debug("[SCP] get scp semaphore %d TIMEOUT...!\n", flag);
	} else {
		pr_debug("[SCP] already hold scp semaphore %d\n", flag);
	}

	return ret;
}
EXPORT_SYMBOL_GPL(get_scp_semaphore);

/*
 * release a hardware semaphore
 * @param flag: semaphore id
 */
int release_scp_semaphore(int flag)
{
	int read_back;
	int ret = -1;

	flag = (flag * 2) + 1;

	read_back = (readl(SCP_SEMAPHORE) >> flag) & 0x1;

	if (read_back == 1) {
		/* Write 1 clear */
		writel((1 << flag), SCP_SEMAPHORE);
		read_back = (readl(SCP_SEMAPHORE) >> flag) & 0x1;
		if (read_back == 0)
			ret = 1;
		else
			pr_debug("[SCP] release scp semaphore %d failed!\n", flag);
	} else {
		pr_debug("[SCP] try to release semaphore %d not own by me\n", flag);
	}

	return ret;
}
EXPORT_SYMBOL_GPL(release_scp_semaphore);

static BLOCKING_NOTIFIER_HEAD(scp_notifier_list);

/*
 * register apps notification
 * NOTE: this function may be blocked and should not be called in interrupt context
 * @param nb:   notifier block struct
 */
void scp_register_notify(struct notifier_block *nb)
{
	mutex_lock(&scp_notify_mutex);
	blocking_notifier_chain_register(&scp_notifier_list, nb);

	pr_debug("[SCP] invoke callback now\n");

	if (is_scp_ready())
		nb->notifier_call(nb, SCP_EVENT_READY, NULL);
	mutex_unlock(&scp_notify_mutex);
}
EXPORT_SYMBOL_GPL(scp_register_notify);

/*
 * unregister apps notification
 * NOTE: this function may be blocked and should not be called in interrupt context
 * @param nb:     notifier block struct
 */
void scp_unregister_notify(struct notifier_block *nb)
{
	mutex_lock(&scp_notify_mutex);
	blocking_notifier_chain_unregister(&scp_notifier_list, nb);
	mutex_unlock(&scp_notify_mutex);
}
EXPORT_SYMBOL_GPL(scp_unregister_notify);

void scp_schedule_work(struct scp_work_struct *scp_ws)
{
	queue_work(scp_workqueue, &scp_ws->work);
}

/*
 * callback function for work struct
 * notify apps to start their tasks or generate an exception according to flag
 * NOTE: this function may be blocked and should not be called in interrupt context
 * @param ws:   work struct
 */
static void scp_notify_ws(struct work_struct *ws)
{
	struct scp_work_struct *sws = container_of(ws, struct scp_work_struct, work);
	unsigned int scp_notify_flag = sws->flags;

	if (scp_notify_flag) {
		mutex_lock(&scp_notify_mutex);
		blocking_notifier_call_chain(&scp_notifier_list, SCP_EVENT_READY, NULL);
		mutex_unlock(&scp_notify_mutex);
	}

	scp_ready = scp_notify_flag;

	if (!scp_ready)
		scp_aed(EXCEP_RESET);

}

/*
 * mark notify flag to 1 to notify apps to start their tasks
 */
static void scp_set_ready(void)
{
	pr_debug("%s()\n", __func__);
	del_timer(&scp_ready_timer);
	scp_notify_work.flags = 1;
	scp_schedule_work(&scp_notify_work);
}

/*
 * callback for reset timer
 * mark notify flag to 0 to generate an exception
 * @param data: unuse
 */
static void scp_wait_ready_timeout(unsigned long data)
{
	pr_debug("%s()\n", __func__);
	scp_notify_work.flags = 0;
	scp_schedule_work(&scp_notify_work);
}

/*
 * handle notification from scp
 * mark scp is ready for running tasks
 * @param id:   ipi id
 * @param data: ipi data
 * @param len:  length of ipi data
 */
static void scp_ready_ipi_handler(int id, void *data, unsigned int len)
{
	if (!scp_ready)
		scp_set_ready();
}

/*
 * @return: 1 if scp is ready for running tasks
 */
unsigned int is_scp_ready(void)
{
	if (scp_ready)
		return 1;
	else
		return 0;
}
EXPORT_SYMBOL_GPL(is_scp_ready);

/*
 * reset scp and create a timer waiting for scp notify
 * notify apps to stop their tasks if needed
 * generate error if reset fail
 * NOTE: this function may be blocked and should not be called in interrupt context
 * @param reset:    0 for first boot, 1 for reset
 * @return:         0 if success
 */
int reset_scp(int reset)
{
	unsigned int prev_ready;
	int ret = -1;
	unsigned int *reg;

	del_timer(&scp_ready_timer);

	prev_ready = scp_ready;
	scp_ready = 0;

	if (reset && prev_ready) {
		mutex_lock(&scp_notify_mutex);
		blocking_notifier_call_chain(&scp_notifier_list, SCP_EVENT_STOP, NULL);
		mutex_unlock(&scp_notify_mutex);
	}

	scp_logger_stop();


	pr_debug("[SCP] reset scp\n");
	reg = (unsigned int *)scpreg.cfg;

	if (reset) {
		*(unsigned int *)reg = 0x0;
		dsb(SY);
	}
	*(unsigned int *)reg = 0x1;
	ret = 0;


	if (ret == 0) {
		init_timer(&scp_ready_timer);
		scp_ready_timer.expires = jiffies + SCP_READY_TIMEOUT;
		scp_ready_timer.function = &scp_wait_ready_timeout;
		scp_ready_timer.data = (unsigned long) 0;
		add_timer(&scp_ready_timer);
	} else {
		scp_aed(EXCEP_RESET);
	}

	return ret;
}

/*
 * parse device tree and mapping iomem
 * @return: 0 if success
 */
static int scp_dt_init(void)
{
	int ret = 0;
	const char *status;
	struct device_node *node = NULL;
	struct resource res;

	node = of_find_compatible_node(NULL, NULL, "mediatek,scp");
	if (!node) {
		pr_err("[SCP] Can't find node: mediatek,scp\n");
		return -1;
	}

	status = of_get_property(node, "status", NULL);
	if (status == NULL || strcmp(status, "okay") != 0) {
		if (strcmp(status, "fail") == 0)
			scp_aed(EXCEP_LOAD_FIRMWARE);
		return -1;
	}

	scpreg.sram = of_iomap(node, 0);
	pr_debug("[SCP] scp sram base=0x%p\n", scpreg.sram);

	if (!scpreg.sram) {
		pr_err("[SCP] Unable to ioremap mregisters\n");
		return -1;
	}

	scpreg.cfg = of_iomap(node, 1);
	pr_debug("[SCP] scp irq=%d, cfgreg=0x%p\n", scpreg.irq, scpreg.cfg);

	if (!scpreg.cfg) {
		pr_err("[SCP] Unable to ioremap registers\n");
		return -1;
	}

	scpreg.clkctrl = of_iomap(node, 2);
	if (!scpreg.clkctrl) {
		pr_err("[SCP] Unable to ioremap registers\n");
		return -1;
	}

	scpreg.irq = irq_of_parse_and_map(node, 0);

	if (of_address_to_resource(node, 0, &res)) {
		pr_err("[SCP] Unable to get dts resource (TCM Size)\n");
		return -1;
	}
	scpreg.tcmsize = (unsigned int)resource_size(&res);

	if (of_address_to_resource(node, 1, &res)) {
		pr_err("[SCP] Unable to get dts resource (CFGREG Size)\n");
		return -1;
	}
	scpreg.cfgregsize = (unsigned int)resource_size(&res);

	return ret;
}

/*
 * TODO: what should we do when hibernation ?
 */
static int scp_pm_event(struct notifier_block *notifier, unsigned long pm_event, void *unused)
{
	int retval;

		switch (pm_event) {
		case PM_POST_HIBERNATION:
			pr_debug("[SCP] scp_pm_event SCP reboot\n");
			retval = reset_scp(1);
			if (retval < 0) {
				retval = -EINVAL;
				pr_debug("[SCP] scp_pm_event SCP reboot Fail\n");
			}
			return NOTIFY_DONE;
	}
	return NOTIFY_OK;
}

static struct notifier_block scp_pm_notifier_block = {
	.notifier_call = scp_pm_event,
	.priority = 0,
};


static inline ssize_t scp_status_show(struct device *kobj, struct device_attribute *attr, char *buf)
{
	if (scp_ready)
		return sprintf(buf, "SCP is ready");
	else
		return sprintf(buf, "SCP is not ready");
}

DEVICE_ATTR(scp_status, 0444, scp_status_show, NULL);

static struct miscdevice scp_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "scp",
	.fops = &scp_file_ops
};

/*
 * register /dev and /sys files
 * @return:     0: success, otherwise: fail
 */
static int create_files(void)
{
	int ret;

	ret = misc_register(&scp_device);

	if (unlikely(ret != 0)) {
		pr_err("[SCP] misc register failed\n");
		return ret;
	}

	ret = device_create_file(scp_device.this_device, &dev_attr_scp_log_len);

	if (unlikely(ret != 0))
		return ret;

	ret = device_create_file(scp_device.this_device, &dev_attr_scp_mobile_log);

	if (unlikely(ret != 0))
		return ret;

	ret = device_create_file(scp_device.this_device, &dev_attr_scp_log_flush);

	if (unlikely(ret != 0))
		return ret;

	ret = device_create_file(scp_device.this_device, &dev_attr_scp_status);

	if (unlikely(ret != 0))
		return ret;

	return 0;
}

/*
 * driver initialization entry point
 */
static int __init scp_init(void)
{

	int ret = 0;

	/* static initialise */
	scp_ready = 0;

	mutex_init(&scp_notify_mutex);

	scp_workqueue = create_workqueue("SCP_WQ");
	scp_excep_init();
	ret = scp_dt_init();
	if (ret) {
		pr_err("[SCP] Device Init Fail\n");
		return -1;
	}

	scp_send_buff = kmalloc((size_t) 64, GFP_KERNEL);
	if (!scp_send_buff)
		return -1;

	scp_recv_buff = kmalloc((size_t) 64, GFP_KERNEL);
	if (!scp_recv_buff)
		return -1;

	INIT_WORK(&scp_notify_work.work, scp_notify_ws);

	scp_irq_init();
	scp_ipi_init();

	scp_ipi_registration(IPI_SCP_READY, scp_ready_ipi_handler, "scp_ready");

	pr_debug("[SCP] helper init\n");

	if (scp_logger_init() == -1) {
		pr_err("[SCP] scp_logger_init_fail\n");
		return -1;
	}

	ret = register_pm_notifier(&scp_pm_notifier_block);

	if (ret)
		pr_err("[SCP] failed to register PM notifier %d\n", ret);

	ret = create_files();

	if (unlikely(ret != 0)) {
		pr_err("[SCP] create files failed\n");
		return -1;
	}

	ret = request_irq(scpreg.irq, scp_irq_handler, IRQF_TRIGGER_NONE, "SCP IPC_MD2HOST", NULL);
	if (ret) {
		pr_err("[SCP] require irq failed\n");
		return -1;
	}

	reset_scp(0);

	return ret;
}

/*
 * driver exit point
 */
static void __exit scp_exit(void)
{
	free_irq(scpreg.irq, NULL);
	misc_deregister(&scp_device);
	flush_workqueue(scp_workqueue);
	scp_logger_cleanup();
	destroy_workqueue(scp_workqueue);
	del_timer(&scp_ready_timer);
	kfree(scp_swap_buf);
}

module_init(scp_init);
module_exit(scp_exit);
