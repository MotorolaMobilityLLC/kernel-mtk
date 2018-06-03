/*
 * Copyright (c) 2016-2018, Watchdata Limited
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/miscdevice.h>
#include <linux/moduleparam.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/device.h>

#define TEE_LOG_POOL_SIZE  4096
typedef unsigned int u32;
typedef unsigned char u8;
struct tee_log_head {
	int write_pos;
	int read_pos;
};

u8 *plog;

struct task_struct *log_thread;

char wdtee_log_line[TEE_LOG_POOL_SIZE];

static int log_worker(void *p);

struct tee_log_head *g_tee_log;
int log_pos;
struct mutex log_mutex;

static int diff_of_wr(void)
{
	int diff;

	diff = g_tee_log->write_pos - g_tee_log->read_pos;
	if (diff < 0)
		diff += TEE_LOG_POOL_SIZE - sizeof(struct tee_log_head);
	return diff;
}

int read_log_char(char *buf)
{
	int cnt = 0, len = 0;

	while (diff_of_wr() != 0) {

		buf[log_pos] = (char)plog[g_tee_log->read_pos];

		if (g_tee_log->read_pos + 1 == TEE_LOG_POOL_SIZE)
			g_tee_log->read_pos = sizeof(struct tee_log_head);
		else
			g_tee_log->read_pos++;

		if (log_pos >= TEE_LOG_POOL_SIZE - 2) {
			wdtee_log_line[log_pos + 1] = '\0';
			pr_err("wdtee log out of line:\n");
			log_pos = 0;
			cnt++;
			len = cnt;
			break;
		}

		if (buf[log_pos] == '\0')
			continue;
		else if (buf[log_pos] == '\n') {
			buf[log_pos + 1] = '\0';
			log_pos = 0;
			cnt++;
			len = cnt;
			break;
		}
		log_pos++;
		cnt++;
	}
	return len;
}

void wd_log_read(void)
{

	if (mutex_trylock(&log_mutex)) {
		while (read_log_char(wdtee_log_line) > 0)
			pr_err("wdtee tz %s", wdtee_log_line);
	}
	dsb(sy);
	mutex_unlock(&log_mutex);
}

int log_kthread(void *pshmaddr)
{
	int ret;
	struct sched_param param = {.sched_priority = 1 };

	mutex_init(&log_mutex);
	log_thread = kthread_create(log_worker, NULL, "wdtee_log");
	if (IS_ERR(log_thread)) {
		pr_err("wdtee_log thread creation failed!");
		ret = -EFAULT;
		goto err;
	}
	sched_setscheduler(log_thread, SCHED_RR, &param);

	set_task_state(log_thread, TASK_INTERRUPTIBLE);
	g_tee_log = (struct tee_log_head *)pshmaddr;
	plog = (u8 *)g_tee_log;

	wake_up_process(log_thread);
err:
	return 0;
}

u8 read_char(void)
{
	u8 ch;

	ch = plog[g_tee_log->read_pos];
	if (g_tee_log->read_pos + 1 == TEE_LOG_POOL_SIZE)
		g_tee_log->read_pos = sizeof(struct tee_log_head);
	else
		g_tee_log->read_pos++;
	dsb(sy);
	return ch;
}

static int log_worker(void *p)
{
	u8 ch;
	int cnt = 0;

	pr_info("wdtee_log kthread run\n");

	mutex_lock(&log_mutex);
	while (!kthread_should_stop()) {
		while (diff_of_wr() == 0) {
			if (cnt > 1000) {
				cnt = 0;
				dsb(sy);
				mutex_unlock(&log_mutex);
				if (diff_of_wr() == 0) {
					schedule_timeout_interruptible
					    (MAX_SCHEDULE_TIMEOUT);
				}
				mutex_lock(&log_mutex);
			} else {
				dsb(sy);
				mutex_unlock(&log_mutex);
				if (diff_of_wr() == 0)
					schedule_timeout_interruptible(1);
				mutex_lock(&log_mutex);
				cnt++;
			}
			continue;
		}
		ch = read_char();
		if (log_pos < TEE_LOG_POOL_SIZE - 2) {
			wdtee_log_line[log_pos] = ch;
		} else {
			wdtee_log_line[log_pos + 1] = '\0';
			pr_err("wdtee log out of line: %s\n",
			       wdtee_log_line);
			log_pos = 0;
			continue;
		}

		if (wdtee_log_line[log_pos] == '\0') {
			continue;
		} else if (wdtee_log_line[log_pos] == '\n') {
			wdtee_log_line[log_pos + 1] = '\0';
			pr_err("wdtee th: %s", wdtee_log_line);
			log_pos = 0;
			continue;
		}
		log_pos++;
	}

	mutex_unlock(&log_mutex);
	return 0;
}

void wdtee_log_thread_read(void)
{
	if (log_thread == NULL || IS_ERR(log_thread))
		return;
	wake_up_process(log_thread);
}
