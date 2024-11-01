// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#if IS_ENABLED(CONFIG_PD_DBG_INFO)
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/sched/clock.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include "inc/pd_dbg_info.h"

struct msg_node {
	struct list_head list;
	char msg[];
};

/* line limits per second, 0: no limit, 1: disable dbg log */
static unsigned int dbg_log_limit = 200;
module_param(dbg_log_limit, uint, 0644);
static unsigned int mn_limit = 10000;
module_param(mn_limit, uint, 0644);

static DEFINE_MUTEX(list_lock);
static LIST_HEAD(msg_list);
static size_t mn_cnt;
static void print_out_dwork_fn(struct work_struct *work);
static DECLARE_DELAYED_WORK(print_out_dwork, print_out_dwork_fn);

static bool dbg_log_en = true;
module_param(dbg_log_en, bool, 0644);

static void clean_up_list(void)
{
	struct msg_node *mn = NULL, *mnn = NULL;

	mutex_lock(&list_lock);
	list_for_each_entry_safe(mn, mnn, &msg_list, list) {
		list_del(&mn->list);
		kfree(mn);
	}
	mn_cnt = 0;
	mutex_unlock(&list_lock);
}

static void print_out_dwork_fn(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct msg_node *mn = NULL;
	unsigned long j = jiffies;
	bool empty = true, loop = false;
	static unsigned long begin;
	static int printed;

	if (dbg_log_limit == 1) {
		clean_up_list();
		return;
	}

	if (!begin)
		begin = j;

	if (time_after(j, begin + HZ)) {
		begin = j;
		printed = 0;
	}

	if (dbg_log_limit && printed >= dbg_log_limit && mn_cnt <= mn_limit) {
		mod_delayed_work(system_wq, dwork, begin + HZ - j + 1);
		return;
	}
loop:
	mutex_lock(&list_lock);
	if (list_empty(&msg_list))
		goto list_unlock;
	mn = list_first_entry(&msg_list, struct msg_node, list);
	list_del(&mn->list);
	empty = list_empty(&msg_list);
	if (empty) {
		mn_cnt = 0;
		loop = false;
	} else
		loop = --mn_cnt > mn_limit;
list_unlock:
	mutex_unlock(&list_lock);
	if (!mn)
		return;
	pr_notice("%s", mn->msg);
	kfree(mn);
	mn = NULL;
	printed++;
	if (!empty) {
		if (loop)
			goto loop;
		else
			schedule_delayed_work(dwork, 0);
	}
}

int pd_dbg_info(const char *fmt, ...)
{
	size_t ts_size = 0, msg_size = 0, size = 0;
	struct msg_node *mn = NULL;
	u64 ts = 0;
	unsigned long rem_msec = 0;
	va_list args;

	if (dbg_log_limit == 1) {
		clean_up_list();
		return -EPERM;
	}

	ts = local_clock();
	rem_msec = do_div(ts, NSEC_PER_SEC) / NSEC_PER_MSEC;
	ts_size = snprintf(NULL, 0, "<%5lu.%03lu>", (unsigned long)ts, rem_msec);
	va_start(args, fmt);
	msg_size = vsnprintf(NULL, 0, fmt, args);
	va_end(args);

	mn = kzalloc(sizeof(*mn) + ts_size + msg_size + 1, GFP_KERNEL);
	if (!mn)
		return -ENOMEM;

	size = snprintf(mn->msg, ts_size + 1, "<%5lu.%03lu>", (unsigned long)ts, rem_msec);
	WARN(ts_size != size, "different return values (%zu and %zu) from %s()",
	     ts_size, size, __func__);

	va_start(args, fmt);
	size = vsnprintf(mn->msg + ts_size, msg_size + 1, fmt, args);
	va_end(args);
	WARN(msg_size != size,
	     "different return values (%zu and %zu) from %s(\"%s\", ...)",
	     msg_size, size, __func__, fmt);

	mutex_lock(&list_lock);
	list_add_tail(&mn->list, &msg_list);
	mn_cnt++;
	mutex_unlock(&list_lock);
	schedule_delayed_work(&print_out_dwork, 0);

	return ts_size + msg_size;
}
EXPORT_SYMBOL(pd_dbg_info);

static int __init pd_dbg_info_init(void)
{
	pr_info("%s ++\n", __func__);
	return 0;
}

static void __exit pd_dbg_info_exit(void)
{
	pr_info("%s --\n", __func__);
	flush_delayed_work(&print_out_dwork);
	clean_up_list();
}

subsys_initcall(pd_dbg_info_init);
module_exit(pd_dbg_info_exit);

MODULE_DESCRIPTION("PD Debug Info Module");
MODULE_AUTHOR("Lucas Tsai <lucas_tsai@richtek.com>");
MODULE_LICENSE("GPL");
#endif	/* CONFIG_PD_DBG_INFO */
