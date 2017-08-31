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
#include <mt-plat/upmu_common.h>
#include <mt-plat/mtk_battery.h>
#include <linux/list.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/wakelock.h>

static struct list_head fgtimer_head = LIST_HEAD_INIT(fgtimer_head);
static struct mutex fgtimer_lock;
static unsigned long reset_time;
static spinlock_t slock;
static struct wake_lock wlock;
static wait_queue_head_t wait_que;
static bool fgtimer_thread_timeout;

void mutex_fgtimer_lock(void)
{
	mutex_lock(&fgtimer_lock);
}

void mutex_fgtimer_unlock(void)
{
	mutex_unlock(&fgtimer_lock);
}

void fgtimer_init(struct fgtimer *timer, struct device *dev, char *name)
{
	timer->name = name;
	INIT_LIST_HEAD(&timer->list);
	timer->dev = dev;
}
void fgtimer_dump_list(void)
{
	struct list_head *pos;
	struct list_head *phead = &fgtimer_head;
	struct fgtimer *ptr;

	pr_debug("dump list start\n");
	list_for_each(pos, phead) {
		ptr = container_of(pos, struct fgtimer, list);
		pr_debug("dump list name:%s time:%ld %ld int:%d\n", dev_name(ptr->dev),
		ptr->stime, ptr->endtime, ptr->interval);
	}
	pr_debug("dump list end\n");
}


void fgtimer_before_reset(void)
{
	reset_time = battery_meter_get_fg_time();
}

void fgtimer_after_reset(void)
{
	struct list_head *pos;
	struct list_head *phead = &fgtimer_head;
	struct fgtimer *ptr;
	unsigned long now = reset_time;
	unsigned long duraction;

	mutex_fgtimer_lock();
	battery_meter_set_fg_timer_interrupt(0);
	pr_debug("fgtimer_reset\n");
	fgtimer_dump_list();
	list_for_each(pos, phead) {
		ptr = container_of(pos, struct fgtimer, list);
		if (ptr->endtime > now) {
			ptr->stime = 0;
			duraction = ptr->endtime - now;
			if (duraction <= 0)
				duraction = 1;
			ptr->endtime = duraction;
			ptr->interval = duraction;
		} else {
			ptr->stime = 0;
			ptr->endtime = 1;
			ptr->interval = 1;
		}
	}

	pos = fgtimer_head.next;
	if (list_empty(pos) != true) {
		ptr = container_of(pos, struct fgtimer, list);
		battery_meter_set_fg_timer_interrupt(ptr->endtime);
	}
	fgtimer_dump_list();
	mutex_fgtimer_unlock();
}

void fgtimer_start(struct fgtimer *timer, int sec)
{
	struct list_head *pos;
	struct list_head *phead = &fgtimer_head;
	struct fgtimer *ptr;
	unsigned long now;

	mutex_fgtimer_lock();
	timer->stime = battery_meter_get_fg_time();
	timer->endtime = timer->stime + sec;
	timer->interval = sec;
	now = timer->stime;

	pr_debug("fgtimer_start dev:%s name:%s %ld %ld %d\n",
	dev_name(timer->dev), timer->name, timer->stime, timer->endtime,
	timer->interval);


	if (list_empty(&timer->list) != true) {
		pr_debug("fgtimer_start dev:%s name:%s time:%ld %ld int:%d is not empty\n",
		dev_name(timer->dev), timer->name,
		timer->stime, timer->endtime, timer->interval);
		list_del_init(&timer->list);
	}

	list_for_each(pos, phead) {
		ptr = container_of(pos, struct fgtimer, list);
		if (timer->endtime < ptr->endtime)
			break;
	}

	list_add(&timer->list, pos->prev);

	pos = fgtimer_head.next;
	if (list_empty(pos) != true) {
		ptr = container_of(pos, struct fgtimer, list);
		if (ptr->endtime > now) {
			int new_sec;

			new_sec = ptr->endtime - now;
			if (new_sec <= 0)
				new_sec = 1;

			battery_meter_set_fg_timer_interrupt(new_sec);
		}
	}
	mutex_fgtimer_unlock();

}

void fgtimer_stop(struct fgtimer *timer)
{

	pr_debug("fgtimer_stop node:%s %ld %ld %d\n",
	dev_name(timer->dev), timer->stime, timer->endtime,
	timer->interval);

	mutex_fgtimer_lock();
	list_del_init(&timer->list);
	mutex_fgtimer_unlock();

}

static struct timespec sstime[10];
void fg_time_int_handler(void)
{
	unsigned int time;
	struct list_head *pos = fgtimer_head.next;
	struct list_head *phead = &fgtimer_head;
	struct fgtimer *ptr;

	get_monotonic_boottime(&sstime[0]);
	time = battery_meter_get_fg_time();
	pr_debug("[fg_time_int_handler] time:%d\n", time);

	get_monotonic_boottime(&sstime[1]);
	for (pos = phead->next; pos != phead;) {
		struct list_head *ptmp;

		ptr = container_of(pos, struct fgtimer, list);
		if (ptr->endtime <= time) {
			ptmp = pos;
			pos = pos->next;
			list_del_init(ptmp);
			pr_debug("[fg_time_int_handler] %s %ld %ld %d timeout\n", dev_name(ptr->dev),
			ptr->stime, ptr->endtime, ptr->interval);
			if (ptr->callback)
				ptr->callback(ptr);
		} else
			pos = pos->next;
	}
	get_monotonic_boottime(&sstime[2]);

	pos = fgtimer_head.next;
	if (list_empty(pos) != true) {
		ptr = container_of(pos, struct fgtimer, list);
		if (ptr->endtime > time) {
			int new_sec;

			new_sec = ptr->endtime - time;
			if (new_sec <= 0)
				new_sec = 1;
			pr_debug("[fg_time_int_handler] next is %s %ld %ld %d now:%d new_sec:%d\n", dev_name(ptr->dev),
				ptr->stime, ptr->endtime, ptr->interval, time, new_sec);
			battery_meter_set_fg_timer_interrupt(new_sec);
		}
	}
	get_monotonic_boottime(&sstime[3]);
	sstime[0] = timespec_sub(sstime[1], sstime[0]);
	sstime[1] = timespec_sub(sstime[2], sstime[1]);
	sstime[2] = timespec_sub(sstime[3], sstime[2]);
}

static int fgtime_thread(void *arg)
{
	unsigned long flags;
	struct timespec stime, endtime, duraction;

	while (1) {
		wait_event(wait_que, (fgtimer_thread_timeout == true));
		fgtimer_thread_timeout = false;
		get_monotonic_boottime(&stime);
		mutex_fgtimer_lock();
		fg_time_int_handler();

		spin_lock_irqsave(&slock, flags);
		wake_unlock(&wlock);
		spin_unlock_irqrestore(&slock, flags);

		mutex_fgtimer_unlock();
		get_monotonic_boottime(&endtime);
		duraction = timespec_sub(endtime, stime);

		if ((duraction.tv_nsec / 1000000) > 50)
			pr_err("fgtime_thread time:%d ms %d %d %d\n", (int)(duraction.tv_nsec / 1000000),
				(int)(sstime[0].tv_nsec / 1000000),
				(int)(sstime[1].tv_nsec / 1000000),
				(int)(sstime[2].tv_nsec / 1000000));
	}

	return 0;
}


void wake_up_fgtimer(void)
{
	unsigned long flags;

	spin_lock_irqsave(&slock, flags);
	if (wake_lock_active(&wlock) == 0)
		wake_lock(&wlock);
	spin_unlock_irqrestore(&slock, flags);

	fgtimer_thread_timeout = true;
	wake_up(&wait_que);
}

void fgtimer_service_init(void)
{
	pr_debug("fgtimer_service_init\n");
	mutex_init(&fgtimer_lock);
	spin_lock_init(&slock);
	wake_lock_init(&wlock, WAKE_LOCK_SUSPEND, "fg timer wakelock");
	init_waitqueue_head(&wait_que);
	kthread_run(fgtime_thread, NULL, "fg_timer_thread");

	pmic_register_interrupt_callback(FG_TIME_NO, wake_up_fgtimer);
}
