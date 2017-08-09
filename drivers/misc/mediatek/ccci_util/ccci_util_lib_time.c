#include <linux/module.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/kfifo.h>

#include <linux/firmware.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/of.h>
#ifdef CONFIG_OF
#include <linux/of_fdt.h>
#endif
#include <asm/setup.h>
#include <linux/atomic.h>
#include <mt-plat/mt_boot_common.h>

#include <mt-plat/mt_ccci_common.h>

static wait_queue_head_t time_update_notify_queue_head;
static spinlock_t wait_count_lock;
static volatile unsigned int wait_count;
static volatile unsigned int get_update;
static volatile unsigned int api_ready;

void ccci_timer_for_md_init(void)
{
	init_waitqueue_head(&time_update_notify_queue_head);
	spin_lock_init(&wait_count_lock);
	wait_count = 0;
	get_update = 0;
	mb();
	api_ready = 1;
}

int wait_time_update_notify(void)
{				/* Only support one wait currently */
	int ret = -1;
	unsigned long flags;

	if (api_ready) {
		/* Update wait count ++ */
		spin_lock_irqsave(&wait_count_lock, flags);
		wait_count++;
		spin_unlock_irqrestore(&wait_count_lock, flags);

		ret = wait_event_interruptible(time_update_notify_queue_head, get_update);
		if (ret != -ERESTARTSYS)
			get_update = 0;

		/* Update wait count -- */
		spin_lock_irqsave(&wait_count_lock, flags);
		wait_count--;
		spin_unlock_irqrestore(&wait_count_lock, flags);
	}

	return ret;
}

void notify_time_update(void)
{
	unsigned long flags;

	if (!api_ready)
		return;		/* API not ready */
	get_update = 1;
	spin_lock_irqsave(&wait_count_lock, flags);
	if (wait_count)
		wake_up_all(&time_update_notify_queue_head);
	spin_unlock_irqrestore(&wait_count_lock, flags);
}
