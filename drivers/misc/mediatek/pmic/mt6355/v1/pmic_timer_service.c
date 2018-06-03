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


struct list_head fgtimer_head = LIST_HEAD_INIT(fgtimer_head);
struct mutex fgtimer_lock;
unsigned long reset_time;


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

	pr_err("dump list start\n");
	list_for_each(pos, phead) {
		ptr = container_of(pos, struct fgtimer, list);
		pr_err("dump list name:%s time:%ld %ld int:%d\n", dev_name(ptr->dev),
		ptr->stime, ptr->endtime, ptr->interval);
	}
	pr_err("dump list end\n");
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
	pr_err("fgtimer_reset\n");
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

	pr_err("fgtimer_start dev:%s name:%s %ld %ld %d\n",
	dev_name(timer->dev), timer->name, timer->stime, timer->endtime,
	timer->interval);


	if (list_empty(&timer->list) != true) {
		pr_err("fgtimer_start dev:%s name:%s time:%ld %ld int:%d is not empty\n",
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

	pr_err("fgtimer_stop node:%s %ld %ld %d\n",
	dev_name(timer->dev), timer->stime, timer->endtime,
	timer->interval);

	mutex_fgtimer_lock();
	list_del_init(&timer->list);
	mutex_fgtimer_unlock();

}

void fg_time_int_handler(void)
{
	unsigned int time;
	struct list_head *pos = fgtimer_head.next;
	struct list_head *phead = &fgtimer_head;
	struct fgtimer *ptr;

	time = battery_meter_get_fg_time();
	pr_err("[fg_time_int_handler] time:%d\n", time);

	mutex_fgtimer_lock();

	for (pos = phead->next; pos != phead;) {
		struct list_head *ptmp;

		ptr = container_of(pos, struct fgtimer, list);
		if (ptr->endtime <= time) {
			ptmp = pos;
			pos = pos->next;
			list_del_init(ptmp);
			pr_err("[fg_time_int_handler] %s %ld %ld %d timeout\n", dev_name(ptr->dev),
			ptr->stime, ptr->endtime, ptr->interval);
			if (ptr->callback)
				ptr->callback(ptr);
		} else
			pos = pos->next;
	}

	pos = fgtimer_head.next;
	if (list_empty(pos) != true) {
		ptr = container_of(pos, struct fgtimer, list);
		if (ptr->endtime > time) {
			int new_sec;

			new_sec = ptr->endtime - time;
			if (new_sec <= 0)
				new_sec = 1;
			pr_err("[fg_time_int_handler] next is %s %ld %ld %d now:%d new_sec:%d\n", dev_name(ptr->dev),
				ptr->stime, ptr->endtime, ptr->interval, time, new_sec);
			battery_meter_set_fg_timer_interrupt(new_sec);
		}
	}
	mutex_fgtimer_unlock();

}

void fgtimer_service_init(void)
{
	pr_err("fgtimer_service_init\n");
	mutex_init(&fgtimer_lock);
	pmic_register_interrupt_callback(FG_TIME_NO, fg_time_int_handler);
}
