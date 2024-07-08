// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 MediaTek Inc.
 */
#include <linux/sched.h>
#include <linux/sched/cputime.h>
#include <sched/sched.h>
#include <trace/hooks/sched.h>
#include <trace/hooks/cgroup.h>
#include <linux/arch_topology.h>
#include "common.h"
#include "vip.h"
#include "eas_trace.h"
#include "eas_plus.h"

unsigned int ls_vip_threshold                   =  DEFAULT_VIP_PRIO_THRESHOLD;
bool vip_enable;

DEFINE_PER_CPU(struct vip_rq, vip_rq);

inline unsigned int num_vvip_in_cpu(int cpu)
{
	struct vip_rq *vrq = &per_cpu(vip_rq, cpu);

	return vrq->num_vvip_tasks;
}

inline unsigned int num_vip_in_cpu(int cpu)
{
	struct vip_rq *vrq = &per_cpu(vip_rq, cpu);

	return vrq->num_vip_tasks;
}

static inline struct cfs_rq *cfs_rq_of(struct sched_entity *se)
{
	return se->cfs_rq;
}

bool balance_vvip_overutilied;
void turn_on_vvip_balance_overutilized(void)
{
	balance_vvip_overutilied = true;
}
EXPORT_SYMBOL_GPL(turn_on_vvip_balance_overutilized);

void turn_off_vvip_balance_overutilized(void)
{
	balance_vvip_overutilied = false;
}
EXPORT_SYMBOL_GPL(turn_off_vvip_balance_overutilized);

int arch_get_nr_clusters(void)
{
	int __arch_nr_clusters = -1;
	int max_id = 0;
	unsigned int cpu;

	/* assume socket id is monotonic increasing without gap. */
	for_each_possible_cpu(cpu) {
		struct cpu_topology *cpu_topo = &cpu_topology[cpu];
		if (cpu_topo->package_id > max_id){
			max_id = cpu_topo->package_id;
		}
	}
	__arch_nr_clusters = max_id + 1;
	return __arch_nr_clusters;
}

int find_imbalanced_vvip_gear(void)
{
	int gear = -1;
	struct cpumask cpus;
	int cpu;
	struct root_domain *rd = cpu_rq(smp_processor_id())->rd;
	struct perf_domain *pd;
	int num_vvip_in_gear = 0;
	int num_cpu = 0;
	int num_sched_clusters = arch_get_nr_clusters();

	rcu_read_lock();
	pd = rcu_dereference(rd->pd);
	if (!pd)
		goto out;
	for (gear = num_sched_clusters-1; gear >= 0 ; gear--) {
		cpumask_and(&cpus, perf_domain_span(pd), cpu_active_mask);
		for_each_cpu(cpu, &cpus) {
			num_vvip_in_gear += num_vvip_in_cpu(cpu);
			num_cpu += 1;
			if (trace_sched_find_imbalanced_vvip_gear_enabled())
				trace_sched_find_imbalanced_vvip_gear(cpu, num_vvip_in_gear);
		}

		/* Choice it since it's beggiest gaar without VVIP*/
		if (num_vvip_in_gear == 0)
			goto out;

		/* Choice it since it's biggest imbalanced gear */
		if (num_vvip_in_gear % num_cpu != 0)
			goto out;

		num_vvip_in_gear = 0;
		num_cpu = 0;
		pd = pd->next;
	}

	/* choice biggest gear when all gear balanced and have VVIP*/
	gear = num_sched_clusters - 1;

out:
	rcu_read_unlock();
	return gear;
}

struct task_struct *vts_to_ts(struct vip_task_struct *vts)
{
	struct mtk_task *mts = container_of(vts, struct mtk_task, vip_task);
	struct task_struct *ts = mts_to_ts(mts);
	return ts;
}

pid_t list_head_to_pid(struct list_head *lh)
{
	pid_t pid = vts_to_ts(container_of(lh, struct vip_task_struct, vip_list))->pid;

	/* means list_head is from rq */
	if (!pid)
		pid = 0;
	return pid;
}

struct task_struct *next_vip_runnable_in_cpu(struct rq *rq, int type)
{
	struct vip_rq *vrq = &per_cpu(vip_rq, cpu_of(rq));
	struct list_head *pos;
	struct task_struct *p;

	list_for_each(pos, &vrq->vip_tasks) {
		struct vip_task_struct *tmp_vts = container_of(pos, struct vip_task_struct,
								vip_list);

		if (tmp_vts->vip_prio != type)
			continue;

		p = vts_to_ts(tmp_vts);
		/* we should pull runnable here, so don't pull curr*/
		if (!rq->curr || p->pid != rq->curr->pid)
			return p;
	}

	return NULL;
}

bool task_is_vip(struct task_struct *p, int type)
{
	struct vip_task_struct *vts = &((struct mtk_task *) p->android_vendor_data1)->vip_task;
	if (type == VVIP)
		return (vts->vip_prio == VVIP);


	return (vts->vip_prio != NOT_VIP);
}

static inline unsigned int vip_task_limit(struct task_struct *p)
{
    return VIP_TIME_LIMIT;
}

static void insert_vip_task(struct rq *rq, struct vip_task_struct *vts,
					bool at_front, bool requeue)
{
	struct list_head *pos;
	struct vip_rq *vrq = &per_cpu(vip_rq, cpu_of(rq));

	list_for_each(pos, &vrq->vip_tasks) {
		struct vip_task_struct *tmp_vts = container_of(pos, struct vip_task_struct,
								vip_list);
		if (at_front) {
			if (vts->vip_prio >= tmp_vts->vip_prio)
				break;
		} else {
			if (vts->vip_prio > tmp_vts->vip_prio)
				break;
		}
	}
	list_add(&vts->vip_list, pos->prev);
	if (!requeue){
		vrq->num_vip_tasks += 1;
		if (vts->vip_prio == VVIP)
			vrq->num_vvip_tasks += 1;
	}

	/* vip inserted trace event */
	if (trace_sched_insert_vip_task_enabled()) {
		pid_t prev_pid = list_head_to_pid(vts->vip_list.prev);
		pid_t next_pid = list_head_to_pid(vts->vip_list.next);
		bool is_first_entry = (prev_pid == 0) ? true : false;
		struct task_struct *p = vts_to_ts(vts);

		trace_sched_insert_vip_task(p, cpu_of(rq), vrq->num_vip_tasks, vts->vip_prio,
			at_front, prev_pid, next_pid, requeue, is_first_entry);
	}
}

void __set_group_vip_prio(struct task_group *tg, unsigned int prio)
{
	struct vip_task_group *vtg;

	if (tg == &root_task_group)
		return;

	vtg = &((struct mtk_tg *) tg->android_vendor_data1)->vtg;
	vtg->threshold = prio;
}

static inline struct task_group *css_tg(struct cgroup_subsys_state *css)
{
	return css ? container_of(css, struct task_group, css) : NULL;
}

struct task_group *search_tg_by_cpuctl_id(unsigned int cpuctl_id)
{
	struct cgroup_subsys_state *css = &root_task_group.css;
	struct cgroup_subsys_state *top_css = css;
	int ret = 0;

	rcu_read_lock();
	css_for_each_child(css, top_css)
		if (css->id == cpuctl_id) {
			ret = 1;
			break;
		}
	rcu_read_unlock();

	if (ret)
		return css_tg(css);

	return &root_task_group;
}

int unset_group_vip_prio(unsigned int cpuctl_id)
{
	struct task_group *tg = search_tg_by_cpuctl_id(cpuctl_id);

	if (tg == &root_task_group)
		return 0;

	__set_group_vip_prio(tg, DEFAULT_VIP_PRIO_THRESHOLD);
	return 1;
}
EXPORT_SYMBOL_GPL(unset_group_vip_prio);

struct task_group *search_tg_by_name(char *group_name)
{
	struct cgroup_subsys_state *css = &root_task_group.css;
	struct cgroup_subsys_state *top_css = css;
	int ret = 0;

	rcu_read_lock();
	css_for_each_child(css, top_css)
		if (!strcmp(css->cgroup->kn->name, group_name)) {
			ret = 1;
			break;
		}
	rcu_read_unlock();

	if (ret)
		return css_tg(css);

	return &root_task_group;
}

void set_group_vip_prio_by_name(char *group_name, unsigned int prio)
{
	struct task_group *tg = search_tg_by_name(group_name);

	if (tg == &root_task_group)
		return;

	__set_group_vip_prio(tg, prio);
}

int set_group_vip_prio(unsigned int cpuctl_id, unsigned int prio)
{
	struct task_group *tg = search_tg_by_cpuctl_id(cpuctl_id);

	if (tg == &root_task_group)
		return 0;

	__set_group_vip_prio(tg, prio);
	return 1;
}
EXPORT_SYMBOL_GPL(set_group_vip_prio);

/* top-app interface */
void set_top_app_vip(unsigned int prio)
{
	set_group_vip_prio_by_name("top-app", prio);
}
EXPORT_SYMBOL_GPL(set_top_app_vip);

void unset_top_app_vip(void)
{
	set_group_vip_prio_by_name("top-app", DEFAULT_VIP_PRIO_THRESHOLD);
}
EXPORT_SYMBOL_GPL(unset_top_app_vip);
/* end of top-app interface */

/* foreground interface */
void set_foreground_vip(unsigned int prio)
{
	set_group_vip_prio_by_name("foreground", prio);
}
EXPORT_SYMBOL_GPL(set_foreground_vip);

void unset_foreground_vip(void)
{
	set_group_vip_prio_by_name("foreground", DEFAULT_VIP_PRIO_THRESHOLD);
}
EXPORT_SYMBOL_GPL(unset_foreground_vip);
/* end of foreground interface */

/* background interface */
void set_background_vip(unsigned int prio)
{
	set_group_vip_prio_by_name("background", prio);
}
EXPORT_SYMBOL_GPL(set_background_vip);

void unset_background_vip(void)
{
	set_group_vip_prio_by_name("background", DEFAULT_VIP_PRIO_THRESHOLD);
}
EXPORT_SYMBOL_GPL(unset_background_vip);
/* end of background interface */

int get_group_threshold(struct task_struct *p)
{
	struct cgroup_subsys_state *css = task_css(p, cpu_cgrp_id);
	struct task_group *tg = container_of(css, struct task_group, css);
	struct vip_task_group *vtg = &((struct mtk_tg *) tg->android_vendor_data1)->vtg;

	if (vtg)
		return vtg->threshold;

	return -1;
}

bool is_VIP_task_group(struct task_struct *p)
{
	if (p->prio <= get_group_threshold(p))
		return true;

	return false;
}

/* ls vip interface */
void set_ls_task_vip(unsigned int prio)
{
	ls_vip_threshold = prio;
}
EXPORT_SYMBOL_GPL(set_ls_task_vip);

void unset_ls_task_vip(void)
{
	ls_vip_threshold = DEFAULT_VIP_PRIO_THRESHOLD;
}
EXPORT_SYMBOL_GPL(unset_ls_task_vip);
/* end of ls vip interface */

bool is_VIP_latency_sensitive(struct task_struct *p)
{
	if (is_task_latency_sensitive(p) && p->prio <= ls_vip_threshold)
		return true;

	return false;
}

int is_VVIP(struct task_struct *p)
{
	struct vip_task_struct *vts = &((struct mtk_task *) p->android_vendor_data1)->vip_task;
	return vts->vvip;
}

void set_task_vvip(int pid)
{
	struct task_struct *p;
	struct vip_task_struct *vts;

	rcu_read_lock();
	p = find_task_by_vpid(pid);
	if (p) {
		get_task_struct(p);
		vts = &((struct mtk_task *) p->android_vendor_data1)->vip_task;
		vts->vvip = true;
		put_task_struct(p);
	}
	rcu_read_unlock();
}
EXPORT_SYMBOL_GPL(set_task_vvip);

void unset_task_vvip(int pid)
{
	struct task_struct *p;
	struct vip_task_struct *vts;

	rcu_read_lock();
	p = find_task_by_vpid(pid);
	if (p) {
		get_task_struct(p);
		vts = &((struct mtk_task *) p->android_vendor_data1)->vip_task;
		vts->vvip = false;
		put_task_struct(p);
	}
	rcu_read_unlock();
}
EXPORT_SYMBOL_GPL(unset_task_vvip);

/* basic vip interace */
void set_task_basic_vip(int pid)
{
	struct task_struct *p;
	struct vip_task_struct *vts;

	rcu_read_lock();
	p = find_task_by_vpid(pid);
	if (p) {
		get_task_struct(p);
		vts = &((struct mtk_task *) p->android_vendor_data1)->vip_task;
		vts->basic_vip = true;
		put_task_struct(p);
	}
	rcu_read_unlock();
}
EXPORT_SYMBOL_GPL(set_task_basic_vip);

void unset_task_basic_vip(int pid)
{
	struct task_struct *p;
	struct vip_task_struct *vts;

	rcu_read_lock();
	p = find_task_by_vpid(pid);
	if (p) {
		get_task_struct(p);
		vts = &((struct mtk_task *) p->android_vendor_data1)->vip_task;
		vts->basic_vip = false;
		put_task_struct(p);
	}
	rcu_read_unlock();
}
EXPORT_SYMBOL_GPL(unset_task_basic_vip);
/* end of basic vip interface */

bool is_VIP_basic(struct task_struct *p)
{
	struct vip_task_struct *vts = &((struct mtk_task *) p->android_vendor_data1)->vip_task;

	return vts->basic_vip;
}

inline int get_vip_task_prio(struct task_struct *p)
{
	int vip_prio = NOT_VIP;
	/* prio = 1 */
	if (is_VVIP(p)) {
		vip_prio = VVIP;
		goto out;
	}

	/* prio = 0 */
	if (is_VIP_task_group(p) || is_VIP_latency_sensitive(p) || is_VIP_basic(p))
		vip_prio = WORKER_VIP;
out:
	if (trace_sched_get_vip_task_prio_enabled()) {
		trace_sched_get_vip_task_prio(p, vip_prio, is_task_latency_sensitive(p),
			ls_vip_threshold, get_group_threshold(p), is_VIP_basic(p));
	}
	return vip_prio;
}

void vip_enqueue_task(struct rq *rq, struct task_struct *p)
{
	struct vip_task_struct *vts = &((struct mtk_task *) p->android_vendor_data1)->vip_task;

	if (vts->vip_prio == NOT_VIP)
		vts->vip_prio = get_vip_task_prio(p);

	if (unlikely(!vip_enable))
		return;

	if (vts->vip_prio == NOT_VIP)
		return;

	/*
	 * This can happen during migration or enq/deq for prio/class change.
	 * it was once VIP but got demoted, it will not be VIP until
	 * it goes to sleep again.
	 */
	if (vts->total_exec > vip_task_limit(p))
		return;

	insert_vip_task(rq, vts, task_running(rq, p), false);

	/*
	 * We inserted the task at the appropriate position. Take the
	 * task runtime snapshot. From now onwards we use this point as a
	 * baseline to enforce the slice and demotion.
	 */
	if (!vts->total_exec) /* queue after sleep */
		vts->sum_exec_snapshot = p->se.sum_exec_runtime;
}

static void deactivate_vip_task(struct task_struct *p, struct rq *rq)
{
	struct vip_task_struct *vts = &((struct mtk_task *) p->android_vendor_data1)->vip_task;
	struct vip_rq *vrq = &per_cpu(vip_rq, cpu_of(rq));
	struct list_head *prev = vts->vip_list.prev;
	struct list_head *next = vts->vip_list.next;

	list_del_init(&vts->vip_list);
	if (vts->vip_prio == VVIP)
		vrq->num_vvip_tasks -= 1;
	vts->vip_prio = NOT_VIP;
	vrq->num_vip_tasks -= 1;

	if (trace_sched_deactivate_vip_task_enabled()) {
		pid_t prev_pid = list_head_to_pid(prev);
		pid_t next_pid = list_head_to_pid(next);

		trace_sched_deactivate_vip_task(p->pid, task_cpu(p), vrq->num_vip_tasks, prev_pid, next_pid);
	}
}

/*
 * VIP task runtime update happens here. Three possibilities:
 *
 * de-activated: The VIP consumed its runtime. Non VIP can preempt.
 * slice expired: VIP slice is expired and other VIP can preempt.
 * slice not expired: This VIP task can continue to run.
 */
static void account_vip_runtime(struct rq *rq, struct task_struct *curr)
{
	struct vip_task_struct *vts = &((struct mtk_task *) curr->android_vendor_data1)->vip_task;
	struct vip_rq *vrq = &per_cpu(vip_rq, cpu_of(rq));
	s64 delta;
	unsigned int limit;

	lockdep_assert_held(&rq->lock);

	/*
	 * RQ clock update happens in tick path in the scheduler.
	 * Since we drop the lock in the scheduler before calling
	 * into vendor hook, it is possible that update flags are
	 * reset by another rq lock and unlock. Do the update here
	 * if required.
	 */
	if (!(rq->clock_update_flags & RQCF_UPDATED))
		update_rq_clock(rq);

	/* sum_exec_snapshot can be ahead. See below increment */
	delta = curr->se.sum_exec_runtime - vts->sum_exec_snapshot;
	if (delta < 0)
		delta = 0;
	else
		delta += rq_clock_task(rq) - curr->se.exec_start;
	/* slice is not expired */
	if (delta < VIP_TIME_SLICE)
		return;

	/*
	 * slice is expired, check if we have to deactivate the
	 * VIP task, otherwise requeue the task in the list so
	 * that other VIP tasks gets a chance.
	 */
	vts->sum_exec_snapshot += delta;
	vts->total_exec += delta;

	limit = vip_task_limit(curr);
	if (vts->total_exec > limit) {
		deactivate_vip_task(curr, rq);
		return;
	}

	/* only this vip task in rq, skip re-queue section */
	if (vrq->num_vip_tasks == 1)
		return;

	/* slice expired. re-queue the task */
	list_del(&vts->vip_list);
	insert_vip_task(rq, vts, false, true);
}

void vip_check_preempt_wakeup(void *unused, struct rq *rq, struct task_struct *p,
				bool *preempt, bool *nopreempt, int wake_flags,
				struct sched_entity *se, struct sched_entity *pse,
				int next_buddy_marked, unsigned int granularity)
{
	struct vip_rq *vrq = &per_cpu(vip_rq, cpu_of(rq));
	struct vip_task_struct *vts_p = &((struct mtk_task *) p->android_vendor_data1)->vip_task;
	struct task_struct *c = rq->curr;
	struct vip_task_struct *vts_c;
	bool resched = false;
	bool p_is_vip, curr_is_vip;

	vts_c = &((struct mtk_task *) rq->curr->android_vendor_data1)->vip_task;

	if (unlikely(!vip_enable))
		return;

	p_is_vip = !list_empty(&vts_p->vip_list) && vts_p->vip_list.next;
	curr_is_vip = !list_empty(&vts_c->vip_list) && vts_c->vip_list.next;
	/*
	 * current is not VIP, so preemption decision
	 * is simple.
	 */
	if (!curr_is_vip) {
		if (p_is_vip)
			goto preempt;
		return; /* CFS decides preemption */
	}

	/*
	 * current is VIP. update its runtime before deciding the
	 * preemption.
	 */
	account_vip_runtime(rq, c);
	resched = (vrq->vip_tasks.next != &vts_c->vip_list);
	/*
	 * current is no longer eligible to run. It must have been
	 * picked (because of VIP) ahead of other tasks in the CFS
	 * tree, so drive preemption to pick up the next task from
	 * the tree, which also includes picking up the first in
	 * the VIP queue.
	 */
	if (resched)
		goto preempt;

	/* current is the first in the queue, so no preemption */
	*nopreempt = true;
	return;
preempt:
	*preempt = true;
}

void vip_cfs_tick(struct rq *rq)
{
	struct vip_rq *vrq = &per_cpu(vip_rq, cpu_of(rq));
	struct vip_task_struct *vts;
	struct rq_flags rf;

	vts = &((struct mtk_task *) rq->curr->android_vendor_data1)->vip_task;

	if (unlikely(!vip_enable))
		return;

	rq_lock(rq, &rf);

	if (list_empty(&vts->vip_list) || (vts->vip_list.next == NULL))
		goto out;
	account_vip_runtime(rq, rq->curr);
	/*
	 * If the current is not VIP means, we have to re-schedule to
	 * see if we can run any other task including VIP tasks.
	 */
	if ((vrq->vip_tasks.next != &vts->vip_list) && rq->cfs.h_nr_running > 1)
		resched_curr(rq);

out:
	rq_unlock(rq, &rf);
}

void vip_lb_tick(struct rq *rq)
{
	vip_cfs_tick(rq);
}

void vip_scheduler_tick(void *unused, struct rq *rq)
{
	struct task_struct *p = rq->curr;

	if (unlikely(!vip_enable))
		return;

	if (!vip_fair_task(p))
		return;

	vip_lb_tick(rq);
}
#if IS_ENABLED(CONFIG_FAIR_GROUP_SCHED)
/* Walk up scheduling entities hierarchy */
#define for_each_sched_entity(se) \
	for (; se; se = se->parent)
#else
#define for_each_sched_entity(se) \
	for (; se; se = NULL)
#endif

void check_vip_num(struct rq *rq)
{
	struct vip_rq *vrq = &per_cpu(vip_rq, cpu_of(rq));

	/* temp patch for counter issue*/
	if (list_empty(&vrq->vip_tasks)) {
		if (vrq->num_vvip_tasks != 0 ) {
			vrq->num_vvip_tasks = 0;
			pr_info("cpu=%d error VVIP number\n", cpu_of(rq));
		}
		if (vrq->num_vip_tasks != 0) {
			vrq->num_vip_tasks = 0;
			pr_info("cpu=%d error VIP number\n", cpu_of(rq));
		}
	}
	/* end of temp patch*/
}

// extern void set_next_entity(struct cfs_rq *cfs_rq, struct sched_entity *se);
void vip_replace_next_task_fair(void *unused, struct rq *rq, struct task_struct **p,
				struct sched_entity **se, bool *repick, bool simple,
				struct task_struct *prev)
{
	struct vip_rq *vrq = &per_cpu(vip_rq, cpu_of(rq));
	struct vip_task_struct *vts;
	struct task_struct *vip;


	if (unlikely(!vip_enable))
		return;

	/* We don't have VIP tasks queued */
	if (list_empty(&vrq->vip_tasks)) {
		/* we should pull VIPs from other CPU */
		return;
	}

	/* Return the first task from VIP queue */
	vts = list_first_entry(&vrq->vip_tasks, struct vip_task_struct, vip_list);
	vip = vts_to_ts(vts);

	*p = vip;
	*se = &vip->se;
	*repick = true;

	// if (simple) {
	// 	for_each_sched_entity((*se))
	// 		set_next_entity(cfs_rq_of(*se), *se);
	// }
}

__no_kcsan
void vip_dequeue_task(void *unused, struct rq *rq, struct task_struct *p)
{
	struct vip_task_struct *vts = &((struct mtk_task *) p->android_vendor_data1)->vip_task;

	if (unlikely(!vip_enable))
		return;

	if (!list_empty(&vts->vip_list) && vts->vip_list.next)
		deactivate_vip_task(p, rq);

	check_vip_num(rq);

	/*
	 * Reset the exec time during sleep so that it starts
	 * from scratch upon next wakeup. total_exec should
	 * be preserved when task is enq/deq while it is on
	 * runqueue.
	 */

	if (READ_ONCE(p->state) != TASK_RUNNING)
		vts->total_exec = 0;
}

inline bool vip_fair_task(struct task_struct *p)
{
	return p->prio >= MAX_RT_PRIO && !is_idle_task(p);
}

void init_vip_task_struct(struct task_struct *p)
{
	struct vip_task_struct *vts = &((struct mtk_task *) p->android_vendor_data1)->vip_task;

	INIT_LIST_HEAD(&vts->vip_list);
	vts->sum_exec_snapshot = 0;
	vts->total_exec = 0;
	vts->vip_prio = NOT_VIP;
	vts->basic_vip = false;
	vts->vvip = false;
	vts->throttle_time = VIP_TIME_LIMIT;
}

static void vip_new_tasks(void *unused, struct task_struct *new)
{
	init_vip_task_struct(new);
}

void __init_vip_group(struct cgroup_subsys_state *css)
{
	struct task_group *tg = container_of(css, struct task_group, css);
	struct vip_task_group *vtg = &((struct mtk_tg *) tg->android_vendor_data1)->vtg;

	vtg->threshold = DEFAULT_VIP_PRIO_THRESHOLD;
}

static void vip_rvh_cpu_cgroup_online(void *unused, struct cgroup_subsys_state *css)
{
	__init_vip_group(css);
}

void init_vip_group(void)
{
	struct cgroup_subsys_state *css = &root_task_group.css;
	struct cgroup_subsys_state *top_css = css;

	rcu_read_lock();
	__init_vip_group(&root_task_group.css);
	css_for_each_child(css, top_css)
		if (css)
			__init_vip_group(css);
	rcu_read_unlock();
}

void register_vip_hooks(void)
{
	int ret = 0;

	ret = register_trace_android_rvh_wake_up_new_task(vip_new_tasks, NULL);
	if (ret)
		pr_info("register wake_up_new_task hooks failed, returned %d\n", ret);

	ret = register_trace_android_rvh_cpu_cgroup_online(vip_rvh_cpu_cgroup_online,
		NULL);
	if (ret)
		pr_info("register cpu_cgroup_online hooks failed, returned %d\n", ret);

	ret = register_trace_android_rvh_check_preempt_wakeup(vip_check_preempt_wakeup, NULL);
	if (ret)
		pr_info("register check_preempt_wakeup hooks failed, returned %d\n", ret);

	ret = register_trace_android_vh_scheduler_tick(vip_scheduler_tick, NULL);
	if (ret)
		pr_info("register scheduler_tick failed\n");

	ret = register_trace_android_rvh_replace_next_task_fair(vip_replace_next_task_fair, NULL);
	if (ret)
		pr_info("register replace_next_task_fair hooks failed, returned %d\n", ret);

	ret = register_trace_android_rvh_after_dequeue_task(vip_dequeue_task, NULL);
	if (ret)
		pr_info("register after_dequeue_task hooks failed, returned %d\n", ret);
}

void vip_init(void)
{
	struct task_struct *g, *p;
	int cpu;

	balance_vvip_overutilied = false;
	/* init vip related value to group*/
	init_vip_group();

	/* init vip related value to exist tasks */

	read_lock(&tasklist_lock);
	do_each_thread(g, p) {
		init_vip_task_struct(p);
	} while_each_thread(g, p);
	read_unlock(&tasklist_lock);

	/* init vip related value to each rq */
	for_each_possible_cpu(cpu) {
		struct vip_rq *vrq = &per_cpu(vip_rq, cpu);

		INIT_LIST_HEAD(&vrq->vip_tasks);
		vrq->num_vip_tasks = 0;
		vrq->num_vvip_tasks = 0;

		/*
		 * init vip related value to idle thread.
		 * some times we'll reference VIP variables from idle process,
		 * so initial it's value to prevent KE.
		 */
		init_vip_task_struct(cpu_rq(cpu)->idle);
	}

	/* init vip related value to newly forked tasks */
	register_vip_hooks();
	vip_enable = true;
}
