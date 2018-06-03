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

#ifdef CONFIG_MTK_SCHED_VIP_TASKS
#include <linux/sched.h>
#include <linux/stop_machine.h>
#include <linux/kthread.h>
#include <trace/events/sched.h>

#include <mt-plat/met_drv.h>

static DEFINE_SPINLOCK(vip_force_migration);
static DEFINE_MUTEX(VIP_MUTEX_LOCK);

bool is_vip_task(struct task_struct *tsk)
{
	if (tsk->flags & PF_VIP_TASK)
		return true;
	else
		return false;
}

static int vip_ref_count;
static int vip_pid[4] = {0};
static int vip_task_apply(int pid, bool set_vip);

/*
 *  mtk: set vip tasks at kernel space
 *  @pid: thread id
 *  @set_vip: 0-default, 1-vip
 *  Success: return 0
 */
int vip_task_set(int pid, bool set_vip)
{
	int i;
	int free_idx = -1;

	mutex_lock(&VIP_MUTEX_LOCK);
	if (set_vip)
		vip_ref_count++;
	else
		vip_ref_count--;

	/* visit vip task table */
	for (i = 0; i < 4; i++) {
		if (free_idx < 0 && !vip_pid[i])
			free_idx = i;

		if (vip_pid[i] == pid) {
			/*
			 * find a matched task.
			 * 1. if to unset, remove it from table.
			 * 2. if to set, double apply is fine.
			 */
			vip_task_apply(pid, set_vip);
			if (!set_vip)
				vip_pid[i] = 0;
			mutex_unlock(&VIP_MUTEX_LOCK);
			return 0;
		}
	}

	/* unset should not arrive here. */
	if (set_vip && free_idx >= 0) {
		vip_task_apply(pid, true);
		vip_pid[free_idx] = pid;
	}

	mutex_unlock(&VIP_MUTEX_LOCK);
	return 0;
}

static int vip_task_apply(int pid, bool set_vip)
{
	struct task_struct *vip_task = NULL;

	rcu_read_lock();
	vip_task = find_task_by_vpid(pid);
	if (vip_task) {
		get_task_struct(vip_task);
		if (set_vip) /* task do force boost */
			vip_task->flags |= PF_VIP_TASK;
		else
			vip_task->flags = vip_task->flags & ~PF_VIP_TASK;
	}
	rcu_read_unlock();

	if (vip_task)
		put_task_struct(vip_task);

	return 0;
}

/*
 * for access below debug node
 * /sys/devices/system/cpu/eas/vip_tasks
 */
void store_vip(int pid)
{
	int i;
	int free_idx = -1;

	mutex_lock(&VIP_MUTEX_LOCK);
	/* visit vip task table */
	for (i = 0; i < 4; i++) {
		if (free_idx < 0 && !vip_pid[i])
			free_idx = i;

		if (vip_pid[i] == pid) {
			vip_task_apply(pid, false);
			vip_pid[i] = 0;
			vip_ref_count--;
			mutex_unlock(&VIP_MUTEX_LOCK);
			return;
		}
	} /* end of loop */

	if (free_idx >= 0) {
		vip_task_apply(pid, true);
		vip_pid[free_idx] = pid;
		vip_ref_count++;
	}
	mutex_unlock(&VIP_MUTEX_LOCK);
}

int get_vip_pid(int idx)
{
	return vip_pid[idx];
}

int get_vip_ref_count(void)
{
	return vip_ref_count;
}

/*
 * MTK: Find most efficiency idle cpu for VIP tasks in wake-up path
 * @p: the task want to be located at.
 *
 * Return:
 *
 * cpu id or
 * -1 if target CPU is not found
 */
int find_idle_vip_cpu(struct task_struct *p)
{
	int iter_cpu;
	unsigned long task_util_boosted, new_util;
	int big_idle_cpu = -1;
	int best_idle_cpu = -1;

	task_util_boosted = boosted_task_util(p);

	/* no vip task */
	if (!vip_ref_count)
		return -1;

	for (iter_cpu = 0; iter_cpu < nr_cpu_ids; iter_cpu++) {

		/* VIP task prefer most efficiency idle cpu */
		int i = iter_cpu;

		new_util = cpu_util(i) + task_util_boosted;

		if (!cpu_online(i) || !cpumask_test_cpu(i, tsk_cpus_allowed(p)))
			continue;

#ifdef CONFIG_MTK_SCHED_INTEROP
		if (cpu_rq(i)->rt.rt_nr_running && likely(!is_rt_throttle(i)))
			continue;
#endif

		/* favoring tasks that prefer idle cpus to improve latency. */
		if (idle_cpu(i)) {
			/* as fallback */
			big_idle_cpu = i;

			/* Ensure minimum capacity to grant the required boost */
			if (new_util <= capacity_orig_of(i)) {
				best_idle_cpu = i;
				return best_idle_cpu;
			}
		}
	}

	return big_idle_cpu;
}

/*
 * move_task - move a task from one runqueue to another runqueue.
 * Both runqueues must be locked.
 */
static void move_vip_task(struct task_struct *p, struct lb_env *env)
{
	deactivate_task(env->src_rq, p, 0);
	set_task_cpu(p, env->dst_cpu);
	activate_task(env->dst_rq, p, 0);

	/* trigger freq switch for vip */
	if (sched_freq())
		set_cfs_cpu_capacity(cpu_of(env->dst_rq), true, 0, SCHE_ONESHOT);

	check_preempt_curr(env->dst_rq, p, 0);
}

static int vip_can_migrate_task(struct task_struct *p, struct lb_env *env)
{
	/*
	 * We do not migrate tasks that are:
	 * 1) running (obviously), or
	 * 2) cannot be migrated to this CPU due to cpus_allowed
	 */
	if (!cpumask_test_cpu(env->dst_cpu, tsk_cpus_allowed(p))) {
		schedstat_inc(p, se.statistics.nr_failed_migrations_affine);
		return 0;
	}
	env->flags &= ~LBF_ALL_PINNED;

	if (task_running(env->src_rq, p)) {
		schedstat_inc(p, se.statistics.nr_failed_migrations_running);
		return 0;
	}

	return 1;
}

/*
 * move_specific_task - tries to move a specific task.
 * Returns 1 if successful and 0 otherwise.
 * Called with both runqueues locked.
 */
static int move_specific_vip_task(struct lb_env *env, struct task_struct *pm)
{
	struct task_struct *p, *n;

	list_for_each_entry_safe(p, n, &env->src_rq->cfs_tasks, se.group_node) {

		if (throttled_lb_pair(task_group(p), env->src_rq->cpu,
					env->dst_cpu))
			continue;

		if (!vip_can_migrate_task(p, env)) {
			mt_sched_printf(sched_lb, "%s: vip=%d ",
					__func__, p->pid);

			continue;
		}

		/* Check if we found the right task */
		if (p != pm) {
			mt_sched_printf(sched_lb, "%s: p!= pm vip=%d ",
			__func__, p->pid);
			continue;
		}
		move_vip_task(p, env);

		return 1;
	}
	return 0;
}

/*
 * vip_active_task_migration_cpu_stop -
 * is run by cpu stopper and used to
 * migrate a specific task from one runqueue to another.
 * vip_task_migrate uses this to push a currently running task
 * off a runqueue.
 * Based on active_load_balance_stop_cpu and can potentially be merged.
 */
static int vip_active_task_migration_cpu_stop(void *data)
{
	struct rq *busiest_rq = data;
	struct task_struct *p = NULL;
	int busiest_cpu = cpu_of(busiest_rq);
	int target_cpu = busiest_rq->push_cpu;
	struct rq *target_rq = cpu_rq(target_cpu);
	struct sched_domain *sd;

	raw_spin_lock_irq(&busiest_rq->lock);
	p = busiest_rq->migrate_task;
	/* make sure the requested cpu hasn't gone down in the meantime */
	if (unlikely(busiest_cpu != smp_processor_id() ||
				!busiest_rq->active_balance)) {
		goto out_unlock;
	}
	/* Is there any task to move? */
	if (busiest_rq->nr_running <= 1)
		goto out_unlock;
	/* Are both target and busiest cpu online */
	if (!cpu_online(busiest_cpu) || !cpu_online(target_cpu))
		goto out_unlock;
	/* Task has migrated meanwhile, abort forced migration */
	if ((!p) || (task_rq(p) != busiest_rq))
		goto out_unlock;
	/*
	 * This condition is "impossible", if it occurs
	 * we need to fix it. Originally reported by
	 * Bjorn Helgaas on a 128-cpu setup.
	 */
	if (busiest_rq == target_rq)
		goto out_unlock;

	/* move a task from busiest_rq to target_rq */
	double_lock_balance(busiest_rq, target_rq);

	/* Search for an sd spanning us and the target CPU. */
	rcu_read_lock();
	for_each_domain(target_cpu, sd) {
		if (cpumask_test_cpu(busiest_cpu, sched_domain_span(sd)))
			break;
	}

	if (likely(sd)) {
		struct lb_env env = {
			.sd             = sd,
			.dst_cpu        = target_cpu,
			.dst_rq         = target_rq,
			.src_cpu        = busiest_rq->cpu,
			.src_rq         = busiest_rq,
			.idle           = CPU_IDLE,
			.tasks          = LIST_HEAD_INIT(env.tasks),
		};

		schedstat_inc(sd, alb_count);

		if (move_specific_vip_task(&env, p)) {
			schedstat_inc(sd, alb_pushed);
			mt_sched_printf(sched_lb, "%s: iter_cpu: vip=%d, i=%d", __func__, p->pid, task_rq(p)->cpu);
			met_tag_oneshot(0, "sched_vip_force", busiest_rq->cpu);

		} else
			schedstat_inc(sd, alb_failed);
	}
	rcu_read_unlock();
	double_unlock_balance(busiest_rq, target_rq);
out_unlock:
	busiest_rq->active_balance = 0;
	raw_spin_unlock_irq(&busiest_rq->lock);

	put_task_struct(p);
	return 0;
}

/* Any idle cpu pull runable VIP task in idle_balance */
int vip_idle_pull(int this_cpu)
{
	struct task_struct *p = NULL;
	struct rq *busiest = NULL;
	int iter_cpu;
	unsigned long flags;
	struct rq *this_rq = cpu_rq(this_cpu);
	struct sched_domain *sd = NULL;
	int pulled_task = 0;
	int reason = 0;
	int vip_pid = 0;
	struct lb_env env = {
		.sd             = sd,
		.dst_cpu        = this_cpu,
		.dst_rq         = this_rq,
		.dst_grpmask    = NULL,
		.idle           = CPU_NEWLY_IDLE,
		.tasks          = LIST_HEAD_INIT(env.tasks),
	};

	/* no vip task */
	if (!vip_ref_count)
		return 0;

	/* find a vip tasks */
	for (iter_cpu = 0; iter_cpu < (nr_cpu_ids); iter_cpu++) {

		if (!cpu_online(iter_cpu))
			continue;

		if (iter_cpu == this_cpu)
			continue;

		busiest = cpu_rq(iter_cpu);
		raw_spin_lock_irqsave(&busiest->lock, flags);

		if (busiest->vip_cache) {
			if (is_vip_task(busiest->vip_cache)) {
				p = busiest->vip_cache;
				get_task_struct(p);
			}
		}

		raw_spin_unlock_irqrestore(&busiest->lock, flags);

		if (p)
			break;
	}


	if (!p) {
		reason = 1; /* no vip */
		goto done;
	}

	env.src_cpu = busiest->cpu;
	env.src_rq = busiest;

#if 0
	raw_spin_lock_irqsave(&busiest->lock, flags);
	detach_task(p, env);
	list_add(&p->se.group_node, &env->tasks);
	pulled_task = 1;
	raw_spin_unlock(&busiest->lock);

	raw_spin_lock(&env.dst_rq->lock);
	list_del_init(&p->se.group_node);
	attach_task(env.dst_rq, p);
	raw_spin_unlock(&env.dst_rq->lock);

	local_irq_restore(flags);
#else
	raw_spin_lock_irqsave(&this_rq->lock, flags);

	 /* move a task from busiest_rq to this_rq */
	double_lock_balance(this_rq, busiest);
	if (busiest->nr_running > 1) {
		/*
		 * We do not migrate tasks that are:
		 * 1. running (obviously), or
		 * 2. cannot be migrated to this CPU due to cpus_allowed
		 */
		pulled_task = move_specific_vip_task(&env, p);
	}
	double_unlock_balance(this_rq, busiest);
	raw_spin_unlock_irqrestore(&this_rq->lock, flags);
#endif

	if (!pulled_task)
		reason = 2; /* failed to pull */

done:
	if (p) {
		vip_pid = p->pid;
		put_task_struct(p);
	}

	/*
	 * reason:
	 * 0: pulled
	 * 1: no vip
	 * 2: failed to pull
	 */
	mt_sched_printf(sched_lb, "%s: vip=%d from %d to %d reason=%d",
			__func__, vip_pid, (busiest) ? busiest->cpu : -1, this_cpu, reason);

	return pulled_task;
}

/* For boost in time */
int vip_task_force_migrate(void)
{
	struct task_struct *p = NULL;
	struct rq *target = NULL;
	int push_cpu = -1;
	int force = 0;
	int iter_cpu;
	unsigned long flags;
	int vip_pid = 0;
	int reason = 0;

	/* no vip task */
	if (!vip_ref_count)
		return 0;

	if (!spin_trylock(&vip_force_migration))
		return 0;

	/* find a vip tasks */
	for (iter_cpu = 0; iter_cpu < (nr_cpu_ids); iter_cpu++) {

		if (!cpu_online(iter_cpu))
			continue;

		target = cpu_rq(iter_cpu);
		raw_spin_lock_irqsave(&target->lock, flags);

		if (target->vip_cache) {
			if (is_vip_task(target->vip_cache)) {
				p = target->vip_cache;
				get_task_struct(p); /* get task+ */
			}
		}

		raw_spin_unlock_irqrestore(&target->lock, flags);

		if (p)
			break;
	}


	if (!p) {
		reason = 1; /* no vip */
		goto done;
	}

	/* find a higher idle cpu */
	for (iter_cpu = 0; iter_cpu < (nr_cpu_ids); iter_cpu++) {
		int i = nr_cpu_ids - iter_cpu - 1;

		mt_sched_printf(sched_lb, "%s:iter_cpu: vip=%d, i=%d",
				__func__, p->pid, i);

		if (!cpu_online(i) || !cpumask_test_cpu(i, tsk_cpus_allowed(p))) {
			mt_sched_printf(sched_lb, "%s:!cpu_online: vip=%d, i=%d",
					__func__, p->pid, i);
			continue;
		}

		if (idle_cpu(i)) {
			mt_sched_printf(sched_lb, "%s:idle_cpu: vip=%d, i=%d",
					__func__, p->pid, i);

			if (task_running(task_rq(p), p)) {
				if (capacity_of(i) > capacity_of(cpu_of(target))) {
					push_cpu = i;
					mt_sched_printf(sched_lb, "%s:task_running: vip=%d, i=%d, target=%d",
							__func__, p->pid, i, cpu_of(target));
					break;
				}
			} else {
				push_cpu = i;
				mt_sched_printf(sched_lb, "%s:task_runnable: vip=%d, i=%d, target=%d",
						__func__, p->pid, i, cpu_of(target));
				break;
			}
		}
	}

	if (push_cpu < 0) {
		reason = 2; /* no push_cpu */
		goto done;
	}

	/* now we have a vip task */
	raw_spin_lock_irqsave(&target->lock, flags);
	if (!target->active_balance && (task_rq(p) == target && !cpu_park(cpu_of(target)))) {
		if (p->state != TASK_DEAD) {
			get_task_struct(p); /* get task++ */
			target->push_cpu = push_cpu;
			target->migrate_task = p;
			target->active_balance = 1;
			force = 1;
		}
	}
	raw_spin_unlock_irqrestore(&target->lock, flags);

	if (force) {
		/* vip migration: push vip task to push_cpu from target */
		if (stop_one_cpu_dispatch(cpu_of(target),
					vip_active_task_migration_cpu_stop,
					target, &target->active_balance_work)) {
			put_task_struct(p); /* put task- must out of rq->lock */
			raw_spin_lock_irqsave(&target->lock, flags);
			target->active_balance = 0;
			force = 0;
			raw_spin_unlock_irqrestore(&target->lock, flags);
		}
	}

	if (!force)
		reason = 3; /* fail to force migrating */

done:
	spin_unlock(&vip_force_migration);
	if (p) {
		vip_pid = p->pid;
		put_task_struct(p); /* put task-- */
	}

	/*
	 * reason:
	 * 0: pushed
	 * 1: no vip
	 * 2: no push_cpu
	 * 3: fail to force migrating
	 */
	mt_sched_printf(sched_lb, "%s: vip=%d forcing migrate from %d to %d reason=%d",
				__func__, vip_pid, (target) ? cpu_of(target) : -1, push_cpu, reason);

	return force;
}
EXPORT_SYMBOL(vip_task_force_migrate);
#else

bool is_vip_task(struct task_struct *tsk) { return false; }

#endif /* CONFIG_MTK_SCHED_VIP_TASKS */
