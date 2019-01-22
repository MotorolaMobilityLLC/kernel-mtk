/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/kallsyms.h>
#include <linux/utsname.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/uaccess.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <linux/notifier.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/input.h>

#include <linux/platform_device.h>
#include "fbc_touch.h"

/*--------------------------------------------*/

#undef TAG
#define TAG "[FBC_TOUCH]"


struct touch_boost {
	spinlock_t touch_lock;
	wait_queue_head_t wq;
	struct task_struct *thread;
	int touch_event;
	atomic_t event;
};

/*--------------------------------------------*/

static struct touch_boost tboost;

/*--------------------FUNCTION----------------*/

static int tboost_thread(void *ptr)
{
	int event;
	unsigned long flags;

	set_user_nice(current, -10);

	while (!kthread_should_stop()) {

		while (!atomic_read(&tboost.event))
			wait_event(tboost.wq, atomic_read(&tboost.event));
		atomic_dec(&tboost.event);

		spin_lock_irqsave(&tboost.touch_lock, flags);
		event = tboost.touch_event;
		spin_unlock_irqrestore(&tboost.touch_lock, flags);

		mutex_lock(&notify_lock);
		if (!fbc_game)
			notify_touch(event);
		mutex_unlock(&notify_lock);
	}
	return 0;
}

static void dbs_input_event(struct input_handle *handle, unsigned int type,
			    unsigned int code, int value)
{
	unsigned long flags;

	if ((type == EV_KEY) && (code == BTN_TOUCH)) {
		pr_debug(TAG"input cb, type:%d, code:%d, value:%d\n", type, code, value);
		spin_lock_irqsave(&tboost.touch_lock, flags);
		tboost.touch_event = value;
		spin_unlock_irqrestore(&tboost.touch_lock, flags);

		atomic_inc(&tboost.event);
		wake_up(&tboost.wq);
	}
}

static int dbs_input_connect(struct input_handler *handler,
			     struct input_dev *dev, const struct input_device_id *id)
{
	struct input_handle *handle;
	int error;

	handle = kzalloc(sizeof(struct input_handle), GFP_KERNEL);

	if (!handle)
		return -ENOMEM;

	handle->dev = dev;
	handle->handler = handler;
	handle->name = "fbc";

	error = input_register_handle(handle);

	if (error)
		goto err2;

	error = input_open_device(handle);

	if (error)
		goto err1;

	return 0;
 err1:
	input_unregister_handle(handle);
 err2:
	kfree(handle);
	return error;
}

static void dbs_input_disconnect(struct input_handle *handle)
{
	input_close_device(handle);
	input_unregister_handle(handle);
	kfree(handle);
}

static const struct input_device_id dbs_ids[] = {
	{.driver_info = 1},
	{},
};

static struct input_handler dbs_input_handler = {
	.event = dbs_input_event,
	.connect = dbs_input_connect,
	.disconnect = dbs_input_disconnect,
	.name = "cpufreq_ond",
	.id_table = dbs_ids,
};

/*--------------------INIT------------------------*/

int init_fbc_touch(void)
{
	int handle;

	spin_lock_init(&tboost.touch_lock);
	init_waitqueue_head(&tboost.wq);
	atomic_set(&tboost.event, 0);
	tboost.thread = (struct task_struct *)kthread_run(tboost_thread, &tboost, "fbc_touch");
	if (IS_ERR(tboost.thread))
		return -EINVAL;

	handle = input_register_handler(&dbs_input_handler);

	return 0;
}

/*MODULE_LICENSE("GPL");*/
/*MODULE_AUTHOR("MTK");*/
/*MODULE_DESCRIPTION("The fliper function");*/
