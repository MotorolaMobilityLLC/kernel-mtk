/*
 * Copyright (C) 2017 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/ktime.h>
#include <linux/hrtimer.h>
#include <linux/sched.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/rbtree.h>
#include <linux/preempt.h>
#include <linux/proc_fs.h>
#include <linux/trace_events.h>
#include <linux/debugfs.h>

#include "../../../perf_ioctl/perf_ioctl.h"
#include <mt-plat/fpsgo_common.h>

#define CREATE_TRACE_POINTS
#include <trace/events/fpsgo.h>

#include "xgf.h"

static DEFINE_MUTEX(xgf_main_lock);
static HLIST_HEAD(xgf_procs);
static int xgf_enable;
static int exit_game_mode;
int game_ppid;
static struct dentry *debugfs_xgf_dir;
static unsigned long long last_blacked_recycle_ts;

static unsigned long long deqstr;
static unsigned long long deqend;

int (*xgf_est_slptime_fp)(
	struct xgf_proc *proc,
	unsigned long long *slptime,
	struct xgf_tick *ref,
	struct xgf_tick *now,
	pid_t r_pid);
EXPORT_SYMBOL(xgf_est_slptime_fp);

static inline void xgf_lock(const char *tag)
{
	mutex_lock(&xgf_main_lock);
}

static inline void xgf_unlock(const char *tag)
{
	mutex_unlock(&xgf_main_lock);
}

void xgf_lockprove(const char *tag)
{
	WARN_ON(!mutex_is_locked(&xgf_main_lock));
}
EXPORT_SYMBOL(xgf_lockprove);

void xgf_trace(const char *fmt, ...)
{
	char log[256];
	va_list args;


	memset(log, ' ', sizeof(log));
	va_start(args, fmt);
	vsnprintf(log, sizeof(log), fmt, args);
	va_end(args);
	trace_xgf_log(log);
}
EXPORT_SYMBOL(xgf_trace);

static inline void xgf_timer_systrace(const void * const timer,
				      int value)
{
	xgf_lockprove(__func__);

	fpsgo_systrace_c_xgf(task_tgid_nr(current), value,
			     "%d:%s timer-%p", task_pid_nr(current),
			     current->comm, timer);
}

static inline int xgf_is_enable(void)
{
	xgf_lockprove(__func__);

	if (exit_game_mode)
		return 0;

	xgf_trace("exit_game_mode", exit_game_mode);

	if (!xgf_enable)
		return 0;

	return 1;
}

/**
 * TODO: should get function pointer while xgf_enable
 */
static unsigned long long ged_get_time(void)
{
	unsigned long long temp;

	preempt_disable();
	temp = cpu_clock(smp_processor_id());
	preempt_enable();
	return temp;
}

static void xgf_update_tick(struct xgf_proc *proc, struct xgf_tick *tick,
			    unsigned long long ts)
{
	struct task_struct *p;

	xgf_lockprove(__func__);

	/* render thread is not set yet */
	if (!proc->render)
		return;

	tick->ts = ts ? ts : ged_get_time();

	/* gather render exact runtime */
	if (proc->render == task_pid_nr(current)) {
		get_task_struct(current);
		tick->runtime = task_sched_runtime(current);
		put_task_struct(current);
		return;
	}

	/* TODO: do not traverse every time */
	rcu_read_lock();
	p = find_task_by_vpid(proc->render);
	if (p)
		get_task_struct(p);
	rcu_read_unlock();
	if (!p) {
		pr_notice("%s: get task %d failed\n", __func__,
			  proc->render);
		/*
		 * avoid hacking exit callpath, reset render.
		 * however, we cannot clean timer list, which
		 * we might be traversing.
		 */
		proc->render = 0;
		return;
	}

	tick->runtime = task_sched_runtime(p);
	put_task_struct(p);
}

static struct xgf_timer *xgf_get_timer_rec(
		const void * const timer,
		struct xgf_proc *proc, int force)
{
	struct rb_node **p = &proc->timer_rec.rb_node;
	struct rb_node *parent = NULL;
	struct xgf_timer *xt = NULL;
	const void *ref;

	xgf_lockprove(__func__);

	while (*p) {
		parent = *p;
		xt = rb_entry(parent, struct xgf_timer, rb_node);

		ref = xt->hrtimer;
		if (timer < ref)
			p = &(*p)->rb_left;
		else if (timer > ref)
			p = &(*p)->rb_right;
		else
			return xt;
	}

	if (!force)
		return NULL;

	xt = kzalloc(sizeof(*xt), GFP_KERNEL);
	if (!xt)
		return NULL;

	xt->hrtimer = timer;

	rb_link_node(&xt->rb_node, parent, p);
	rb_insert_color(&xt->rb_node, &proc->timer_rec);
	return xt;
}

static inline void xgf_blacked_recycle(struct rb_root *root,
				       unsigned long long now_ts)
{
	struct rb_node *n;
	long long diff;

	xgf_lockprove(__func__);

	diff = (long long)now_ts - (long long)last_blacked_recycle_ts;

	/* has not been four seconds since last recycle */
	if (diff < 0LL || diff < (NSEC_PER_SEC << 2))
		return;

	n = rb_first(root);
	while (n) {
		struct xgf_timer *iter;

		iter = rb_entry(n, struct xgf_timer, rb_node);

		diff = (long long)now_ts - (long long)iter->fire.ts;
		if (!iter->expire.ts && diff < NSEC_PER_SEC) {
			n = rb_next(n);
			continue;
		}

		/* clean activity over two seconds ago */
		diff = (long long)now_ts - (long long)iter->expire.ts;
		if (diff < 0LL || diff < (NSEC_PER_SEC << 1)) {
			n = rb_next(n);
			continue;
		}

		rb_erase(n, root);
		kfree(iter);

		n = rb_first(root);
	}

	last_blacked_recycle_ts = now_ts;
}

/**
 * is_valid_sleeper
 */
static int is_valid_sleeper(const void * const timer,
			    struct xgf_proc *proc,
			    unsigned long long now_ts)
{
	struct xgf_timer *xt;

	xgf_lockprove(__func__);

	xt = xgf_get_timer_rec(timer, proc, 1);
	if (!xt)
		return 0;

	/* a new record */
	if (!xt->fire.ts) {
		xt->fire.ts = now_ts;
		return 1;
	}

	xt->fire.ts = now_ts;

	if (!xt->expire.ts || xt->expire.ts >= now_ts)
		return 0;

	if ((now_ts - xt->expire.ts) < NSEC_PER_MSEC) {
		if (unlikely(xt->blacked == INT_MAX))
			xt->blacked = 100;
		else
			xt->blacked++;
	} else {
		if (unlikely(xt->blacked == INT_MIN))
			xt->blacked = -100;
		else
			xt->blacked--;
	}

	return (xt->blacked > 0) ? 0 : 1;
}

/**
 * xgf_timer_fire - called when timer invocation
 */
static void xgf_timer_fire(const void * const timer,
			   struct xgf_proc *proc)
{
	struct xgf_timer *xt;
	unsigned long long now_ts;

	xgf_lockprove(__func__);
	xgf_timer_systrace(timer, 1);

	now_ts = ged_get_time();

	if (!is_valid_sleeper(timer, proc, now_ts))
		return;

	/* for sleep time estimation */
	xgf_trace("valid timer=%p\n", timer);
	xt = kzalloc(sizeof(*xt), GFP_KERNEL);
	if (!xt)
		return;

	xt->hrtimer = timer;
	xt->expired = 0;
	xgf_update_tick(proc, &xt->fire, now_ts);

	hlist_add_head(&xt->hlist, &proc->timer_head);
}

static void xgf_update_timer_rec(const void * const timer,
		struct xgf_proc *proc, unsigned long long now_ts)
{
	struct xgf_timer *xt;

	xgf_lockprove(__func__);

	xt = xgf_get_timer_rec(timer, proc, 0);
	if (!xt)
		return;

	xt->expire.ts = now_ts;
}

/**
 * xgf_timer_expire - called when timer expires
 */
static void xgf_timer_expire(const void * const timer,
			     struct xgf_proc *proc)
{
	struct xgf_timer *iter;
	unsigned long long now_ts;

	xgf_lockprove(__func__);
	xgf_timer_systrace(timer, 0);

	now_ts = ged_get_time();

	xgf_update_timer_rec(timer, proc, now_ts);

	hlist_for_each_entry(iter, &proc->timer_head, hlist) {
		if (timer != iter->hrtimer)
			continue;

		if (iter->expired)
			continue;

		xgf_update_tick(proc, &iter->expire, now_ts);
		iter->expired = 1;
		return;
	}
}

static void xgf_timer_remove(const void * const timer,
			     struct xgf_proc *proc)
{
	struct xgf_timer *iter;
	struct hlist_node *n;

	xgf_lockprove(__func__);
	xgf_timer_systrace(timer, -1);

	hlist_for_each_entry_safe(iter, n, &proc->timer_head, hlist) {
		if (timer != iter->hrtimer)
			continue;

		if (iter->expired)
			xgf_trace("XXX remove expired timer=%p", timer);

		hlist_del(&iter->hlist);
		kfree(iter);
		return;
	}
	xgf_trace("XXX remove a ghost(?) timer=%p", timer);
}

/**
 * xgf_igather_timer - called for intelligence gathering of timer
 */
void xgf_igather_timer(const void * const timer, int fire)
{
	struct xgf_proc *iter;
	pid_t tpid;

	tpid = task_tgid_nr(current);

	xgf_lock(__func__);

	if (!xgf_is_enable()) {
		xgf_unlock(__func__);
		return;
	}

	hlist_for_each_entry(iter, &xgf_procs, hlist) {
		if (iter->parent != tpid)
			continue;

		switch (fire) {
		case 1:
			xgf_timer_fire(timer, iter);
			break;
		case 0:
			xgf_timer_expire(timer, iter);
			break;
		case -1:
		default:
			xgf_timer_remove(timer, iter);
			break;
		}
		break;
	}

	xgf_unlock(__func__);
}

void xgf_epoll_igather_timer(const void * const timer,
		ktime_t *expires, int fire)
{
	if (expires && expires->tv64)
		xgf_igather_timer(timer, fire);
}

/**
 * xgf_reset_render - called while rendering switch
 */
void xgf_reset_render(struct xgf_proc *proc)
{
	struct xgf_timer *iter;
	struct hlist_node *t;
	struct rb_node *n;

	xgf_lockprove(__func__);

	hlist_for_each_entry_safe(iter, t, &proc->timer_head, hlist) {
		hlist_del(&iter->hlist);
		kfree(iter);
	}
	INIT_HLIST_HEAD(&proc->timer_head);

	while ((n = rb_first(&proc->timer_rec))) {
		rb_erase(n, &proc->timer_rec);

		iter = rb_entry(n, struct xgf_timer, rb_node);
		kfree(iter);
	}
}
EXPORT_SYMBOL(xgf_reset_render);

/**
 * xgf_kzalloc
 */
void *xgf_kzalloc(size_t size)
{
	return kzalloc(size, GFP_KERNEL);
}
EXPORT_SYMBOL(xgf_kzalloc);

/**
 * xgf_kfree
 */
void xgf_kfree(const void *block)
{
	kfree(block);
}
EXPORT_SYMBOL(xgf_kfree);

static void xgf_reset_procs(void)
{
	struct xgf_proc *iter;
	struct hlist_node *tmp;

	xgf_lockprove(__func__);

	hlist_for_each_entry_safe(iter, tmp, &xgf_procs, hlist) {
		xgf_reset_render(iter);

		hlist_del(&iter->hlist);
		kfree(iter);
	}
}

/**
 * xgf_get_proc -
 */
static int xgf_get_proc(pid_t ppid, struct xgf_proc **ret, int force)
{
	struct xgf_proc *iter;

	xgf_lockprove(__func__);

	hlist_for_each_entry(iter, &xgf_procs, hlist) {
		if (iter->parent != ppid)
			continue;

		if (ret)
			*ret = iter;
		return 0;
	}

	/* ensure dequeue is observed first */
	if (!force)
		return -EINVAL;

	iter = kzalloc(sizeof(*iter), GFP_KERNEL);
	if (iter == NULL)
		return -ENOMEM;

	iter->parent = ppid;
	iter->render = 0;

	INIT_HLIST_HEAD(&iter->timer_head);
	hlist_add_head(&iter->hlist, &xgf_procs);

	if (ret)
		*ret = iter;
	return 0;
}

int xgf_self_ctrl_enable(int enable, pid_t ppid)
{
	xgf_lock(__func__);

	if (exit_game_mode) {
		xgf_unlock(__func__);
		return 0;
	}

	/*
	 * with some operations, ex: power key resume to game, game
	 * mode switches but mis-recognizes the most active process
	 * as game, ex: systemui; however, game mode will update the
	 * @ppid in the near future but keeping @enable = 1 once he
	 * thinks the game process is more active than previous one.
	 * so, even @enable keeps, once process changes, need to
	 * create associated structure for the new process, the game,
	 * or the game sleeping time estimation is off.
	 *
	 * it's a workaround.
	 */
	if (!!enable == xgf_enable) {
		if (!xgf_enable || (xgf_enable && ppid == game_ppid)) {
			xgf_unlock(__func__);
			return 0;
		}
	}

	xgf_enable = !!enable;
	fpsgo_systrace_c_log(xgf_enable, "xgf enable");

	if (xgf_enable) {
		int ret = 0;

		ret = xgf_get_proc(ppid, NULL, 1);
		if (ret) {
			xgf_unlock(__func__);
			return ret;
		}
		game_ppid = ppid;
	} else {
		game_ppid = 0;
		xgf_reset_procs();
	}

	xgf_unlock(__func__);
	return 0;
}

static unsigned long long xgf_qudeq_enter(struct xgf_proc *proc,
					  struct xgf_tick *ref,
					  struct xgf_tick *now)
{
	int ret;
	unsigned long long slptime;

	xgf_lockprove(__func__);

	xgf_update_tick(proc, now, 0);

	/* first frame of each process */
	if (!ref->ts) {
		/* assume invoked by render thread only */
		pid_t rpid = task_pid_nr(current);

		proc->render = rpid;
		fpsgo_systrace_c_xgf(proc->parent, rpid, "render thrd");
		return 0ULL;
	}

	trace_xgf_intvl("enter", NULL, &ref->ts, &now->ts);

	BUG_ON(!xgf_est_slptime_fp);
	if (xgf_est_slptime_fp) {
		pid_t rpid = task_pid_nr(current);

		ret = xgf_est_slptime_fp(proc, &slptime, ref, now, rpid);
	}
	else
		ret = -ENOENT;

	if (ret)
		return 0ULL;

	return slptime;
}

static void xgf_qudeq_exit(struct xgf_proc *proc, struct xgf_tick *ts,
			   unsigned long long *time)
{
	unsigned long long start;

	xgf_lockprove(__func__);

	start = ts->ts;
	xgf_update_tick(proc, ts, 0);
	if (ts->ts <= start)
		*time = 0ULL;
	else
		*time = ts->ts - start;
	fpsgo_systrace_c_xgf(proc->parent, *time, "renew qudeq time");
}

void xgf_game_mode_exit(int val)
{
	xgf_lock(__func__);
	exit_game_mode = val;
	if (val) {
		xgf_enable = 0;
		game_ppid = 0;

		xgf_reset_procs();
	}

	xgf_unlock(__func__);
}

static inline void xgf_ioctl_notify(unsigned int cmd, unsigned long arg)
{
	xgf_lockprove(__func__);

	/* only allow game process to proceed */
	if (task_tgid_nr(current) != game_ppid)
		return;

	/* mainly for loading estimation */
	if (cmd == FPSGO_QUEUE || cmd == FPSGO_DEQUEUE)
		xgf_dequeuebuffer(arg);
}

void xgf_qudeq_notify(unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct xgf_proc *p, **pproc;
	struct xgf_proc *iter;
	int proc_cnt = 0;
	int timer_cnt = 0;

	/* filter out main thread to queue/dequeue */
	if (task_tgid_nr(current) == task_pid_nr(current))
		return;

	xgf_lock(__func__);
	if (!xgf_is_enable()) {
		xgf_unlock(__func__);
		return;
	}

	xgf_ioctl_notify(cmd, arg);

	switch (cmd) {
	case FPSGO_QUEUE:
		pproc = &p;

		ret = xgf_get_proc(task_tgid_nr(current), pproc, 0);
		if (ret)
			goto ioctl_err;

		if (!!arg) {
			unsigned long long dur;
			struct xgf_timer *timer_iter;

			hlist_for_each_entry(timer_iter, &p->timer_head, hlist)
				timer_cnt++;
			fpsgo_systrace_c_log(timer_cnt, "timer_cnt");

			dur = xgf_qudeq_enter(p, &p->deque, &p->queue);
			fpsgo_systrace_c_xgf(p->parent, dur,
					"renew sleep time");
			p->slptime += dur;
			fpsgo_systrace_c_log(p->slptime,
					"frame sleep time");
			p->slptime_ged = p->slptime;

			hlist_for_each_entry(iter, &xgf_procs, hlist)
				proc_cnt++;
			if (proc_cnt > 5)
				xgf_reset_procs();
			fpsgo_systrace_c_log(proc_cnt, "proc_cnt");
		} else {
			xgf_qudeq_exit(p, &p->queue, &p->quetime);
			/* reset for safety */
			p->slptime = 0;
		}
		break;

	case FPSGO_DEQUEUE:
		pproc = &p;

		if (!!arg)
			deqstr = ged_get_time();
		else
			deqend = ged_get_time();
		xgf_trace("start=%d deq str=%llu end=%llu", !!arg, deqstr, deqend);


		ret = xgf_get_proc(task_tgid_nr(current), pproc, 0);
		if (ret)
			goto ioctl_err;

		if (!!arg) {
			unsigned long long dur;

			dur = xgf_qudeq_enter(p, &p->queue, &p->deque);
			fpsgo_systrace_c_xgf(p->parent, dur,
					     "renew sleep time");
			p->slptime += dur;
			xgf_blacked_recycle(&p->timer_rec, deqstr);
		} else {
			xgf_get_deqend_time();
			xgf_qudeq_exit(p, &p->deque, &p->deqtime);
		}
		break;

	default:
		break;
	}

ioctl_err:
	xgf_unlock(__func__);
	return;
}

int xgf_query_blank_time(pid_t ppid, unsigned long long *deqtime,
			 unsigned long long *slptime)
{
	int ret = 0;
	struct xgf_proc *proc, **pproc;
	unsigned long long deqtime_t = 0ULL;

	pproc = &proc;

	if (deqend > deqstr)
		deqtime_t = deqend - deqstr;
	*deqtime = deqtime_t;

	xgf_lock(__func__);
	if (!xgf_is_enable()) {
		*slptime = 0;
		xgf_unlock(__func__);
		return 0;
	}

	ret = xgf_get_proc(ppid, pproc, 0);
	if (ret)
		goto query_err;

	*slptime = proc->slptime_ged;

query_err:
	xgf_unlock(__func__);
	return ret;
}

void fpsgo_update_render_dep(struct task_struct *p) { }

#define FPSGO_DEBUGFS_ENTRY(name) \
static int fpsgo_##name##_open(struct inode *i, struct file *file) \
{ \
	return single_open(file, fpsgo_##name##_show, i->i_private); \
} \
\
static const struct file_operations fpsgo_##name##_fops = { \
	.owner = THIS_MODULE, \
	.open = fpsgo_##name##_open, \
	.read = seq_read, \
	.write = fpsgo_##name##_write, \
	.llseek = seq_lseek, \
	.release = single_release, \
}

static int fpsgo_black_show(struct seq_file *m, void *unused)
{
	struct rb_node *n;
	struct xgf_proc *proc;
	struct xgf_timer *iter;
	struct rb_root *r;

	xgf_lock(__func__);
	hlist_for_each_entry(proc, &xgf_procs, hlist) {
		seq_printf(m, " proc:%d:%d %llu\n", proc->parent,
			   proc->render, ged_get_time());

		r = &proc->timer_rec;
		for (n = rb_first(r); n != NULL; n = rb_next(n)) {
			iter = rb_entry(n, struct xgf_timer, rb_node);
			seq_printf(m, "%p fire:%llu expire:%llu black:%d\n",
				   iter->hrtimer, iter->fire.ts,
				   iter->expire.ts, iter->blacked);
		}
	}
	xgf_unlock(__func__);
	return 0;
}

static ssize_t fpsgo_black_write(struct file *flip,
			const char *ubuf, size_t cnt, loff_t *data)
{
	return cnt;
}

FPSGO_DEBUGFS_ENTRY(black);

static int __init init_xgf(void)
{
	if (!fpsgo_debugfs_dir)
		return -ENODEV;

	debugfs_xgf_dir = debugfs_create_dir("xgf",
					     fpsgo_debugfs_dir);
	if (!debugfs_xgf_dir)
		return -ENODEV;

	debugfs_create_file("black",
			    S_IRUGO | S_IWUGO,
			    debugfs_xgf_dir,
			    NULL,
			    &fpsgo_black_fops);

	return 0;
}

late_initcall(init_xgf);
