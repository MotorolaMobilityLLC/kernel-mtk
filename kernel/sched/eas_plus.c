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

static bool is_intra_domain(int prev, int target);
static int select_prefer_idle_cpu(struct task_struct *p);
static int select_max_spare_capacity_cpu(struct task_struct *p, int target);
static int select_energy_cpu_plus(struct task_struct *p, int target,  bool prefer_idle);
static int __energy_diff(struct energy_env *eenv);
int idle_prefer_mode;

#ifdef CONFIG_SCHED_TUNE
static inline int energy_diff(struct energy_env *eenv);
#else
#define energy_diff(eenv) __energy_diff(eenv)
#endif


static bool is_intra_domain(int prev, int target)
{
#ifdef CONFIG_ARM64
	return (cpu_topology[prev].cluster_id ==
			cpu_topology[target].cluster_id);
#else
	return (cpu_topology[prev].socket_id ==
			cpu_topology[target].socket_id);
#endif
}

/* To find a CPU with max spare capacity in the same cluster with target */
	static
int select_max_spare_capacity_cpu(struct task_struct *p, int target)
{
	unsigned long int max_spare_capacity = 0;
	int max_spare_cpu = -1;
	struct cpumask cls_cpus;
	int cid = arch_get_cluster_id(target); /* cid of target CPU */
	int cpu = task_cpu(p);
	struct cpumask *tsk_cpus_allow = tsk_cpus_allowed(p);

	/* If the prevous cpu is cache affine and idle, choose it first. */
	if (cpu != l_plus_cpu && cpu != target && cpus_share_cache(cpu, target) && idle_cpu(cpu) &&
		!cpu_isolated(cpu))
		return cpu;

	arch_get_cluster_cpus(&cls_cpus, cid);

	/* Otherwise, find a CPU with max spare-capacity in cluster */
	for_each_cpu_and(cpu, tsk_cpus_allow, &cls_cpus) {
		unsigned long int new_usage;
		unsigned long int spare_cap;

		if (!cpu_online(cpu))
			continue;

		if (cpu_isolated(cpu))
			continue;

#ifdef CONFIG_MTK_SCHED_INTEROP
		if (cpu_rq(cpu)->rt.rt_nr_running && likely(!is_rt_throttle(cpu)))
			continue;
#endif

#ifdef CONFIG_SCHED_WALT
		if (walt_cpu_high_irqload(cpu))
			continue;
#endif

		if (idle_cpu(cpu))
			return cpu;

		new_usage = cpu_util(cpu) + task_util(p);

		if (new_usage >= capacity_of(cpu))
			spare_cap = 0;
		else    /* consider RT/IRQ capacity reduction */
			spare_cap = (capacity_of(cpu) - new_usage);

		/* update CPU with max spare capacity */
		if ((long int)spare_cap > (long int)max_spare_capacity) {
			max_spare_cpu = cpu;
			max_spare_capacity = spare_cap;
		}
	}

	/* if max_spare_cpu exist, choose it. */
	if (max_spare_cpu > -1)
		return max_spare_cpu;
	else
		return task_cpu(p);
}

bool idle_prefer_need(void)
{
	if (idle_prefer_mode)
		return true;
	else
		return false;
}

static
int select_prefer_idle_cpu(struct task_struct *p)
{
	unsigned long min_util = boosted_task_util(p);
	int prev_cpu = task_cpu(p);
	int best_idle_cpu = -1;
	int iter_cpu;
#ifdef CONFIG_CGROUP_SCHEDTUNE
	bool boosted = schedtune_task_boost(p) > 0;
#else
	bool boosted = 0;
#endif
	int fallback = -1;
	unsigned long new_util, max_util = SCHED_CAPACITY_SCALE;
	struct cpumask *tsk_cpus_allow = tsk_cpus_allowed(p);

	/* force boosted if idle prefer mode is on */
	if (idle_prefer_need())
		boosted = (boosted) ? boosted : 1;

	for (iter_cpu = 0; iter_cpu < nr_cpu_ids; iter_cpu++) {
		/*
		 * Iterate from higher cpus for boosted tasks.
		 */
		int i = boosted ? nr_cpu_ids - iter_cpu - 1 : iter_cpu;

		if (!cpu_online(i) || !cpumask_test_cpu(i, tsk_cpus_allow) || cpu_isolated(i))
			continue;

		new_util = cpu_util(i) + min_util;
		new_util = min(new_util, max_util);

		if (new_util > capacity_orig_of(i))
			continue;

		/*
		 * Unconditionally favoring tasks that prefer idle cpus to
		 * improve latency.
		 */
		if (idle_cpu(i)) {
			if (best_idle_cpu < 0) {
				if (i != l_plus_cpu) {
					best_idle_cpu = i;
					break;
				}
			}
			if (fallback < 0)
				fallback = i;
		}
	}

	if ((best_idle_cpu >= 0) && idle_cpu(prev_cpu) &&
		is_intra_domain(prev_cpu, best_idle_cpu)) {
		best_idle_cpu = prev_cpu;
	}

	return (best_idle_cpu >= 0) ? best_idle_cpu : fallback;
}

/*
 * @p: the task want to be located at.
 *
 * Return:
 *
 * cpu id or
 * -1 if target CPU is not found
 */
int find_best_idle_cpu(struct task_struct *p, bool prefer_idle)
{
	int iter_cpu;
	int best_idle_cpu = -1;
	struct cpumask *tsk_cpus_allow = tsk_cpus_allowed(p);

	for (iter_cpu = 0; iter_cpu < nr_cpu_ids; iter_cpu++) {
		/* foreground task prefer idle to find bigger idle cpu */
		int i = (sched_boost() || (prefer_idle && (task_util(p) > stune_task_threshold))) ?
			nr_cpu_ids-iter_cpu-1 : iter_cpu;

		if (!cpu_online(i) || !cpumask_test_cpu(i, tsk_cpus_allow) || cpu_isolated(i))
			continue;


#ifdef CONFIG_MTK_SCHED_INTEROP
		if (cpu_rq(i)->rt.rt_nr_running && likely(!is_rt_throttle(i)))
			continue;
#endif

		/* favoring tasks that prefer idle cpus to improve latency. */
		if (idle_cpu(i)) {
			best_idle_cpu = i;
			break;
		}
	}

	return best_idle_cpu;
}

static int select_energy_cpu_plus(struct task_struct *p, int target, bool prefer_idle)
{
	int target_max_cap = INT_MAX;
	int target_cpu = task_cpu(p);
	int prev_cpu;
	int i, cpu;
	bool is_tiny = false;
	int nrg_diff = 0;
	int cluster_id = 0;
	struct cpumask cluster_cpus;
	int max_cap_cpu = 0;
	int best_cpu = 0;
	struct cpumask *tsk_cpus_allow = tsk_cpus_allowed(p);
	bool over_util = false;

	/* prefer idle for stune */
	if (prefer_idle) {
		cpu = select_prefer_idle_cpu(p);
		if (cpu > 0)
			return cpu;
	}

	/*
	 * Find group with sufficient capacity. We only get here if no cpu is
	 * overutilized. We may end up overutilizing a cpu by adding the task,
	 * but that should not be any worse than select_idle_sibling().
	 * load_balance() should sort it out later as we get above the tipping
	 * point.
	 */
	cluster_id = arch_get_nr_clusters();
	for (i = 0; i < cluster_id; i++) {
		arch_get_cluster_cpus(&cluster_cpus, i);
		max_cap_cpu = cpumask_first(&cluster_cpus);

		/* Assuming all cpus are the same in group */
		for_each_cpu(cpu, &cluster_cpus) {

			if (!cpu_online(cpu))
				continue;

			if (cpu_isolated(cpu))
				continue;

			if (!cpumask_test_cpu(cpu, tsk_cpus_allow))
				continue;

			if (capacity_of(max_cap_cpu) < target_max_cap &&
					task_fits_max(p, max_cap_cpu)) {
				best_cpu = cpu;
				target_max_cap = capacity_of(max_cap_cpu);
			}
			break;
		}
	}
	/* Find cpu with sufficient capacity */
	target_cpu = select_max_spare_capacity_cpu(p, best_cpu);

	prev_cpu = task_cpu(p);
	/* no need energy calculation if the same domain */
	if (is_intra_domain(prev_cpu, target_cpu) && target_cpu != l_plus_cpu) {

		if (idle_cpu(prev_cpu) && idle_cpu(target_cpu)) {
			struct rq *prev_rq, *target_rq;
			int prev_idle_idx;
			int target_idle_idx;

			prev_rq = cpu_rq(prev_cpu);
			target_rq = cpu_rq(target_cpu);

			rcu_read_lock();
			prev_idle_idx = idle_get_state_idx(prev_rq);
			target_idle_idx = idle_get_state_idx(target_rq);
			rcu_read_unlock();

			/* favoring shallowest idle states */
			if ((prev_idle_idx <= target_idle_idx) ||
					target_idle_idx == -1)
				target_cpu = prev_cpu;
		}

		return target_cpu;
	}

	if (task_util(p) <= 0)
		return target_cpu;

	rcu_read_lock();

	/* no energy comparison if the same cluster */
	if (target_cpu != task_cpu(p)) {
		int delta = 0;
		struct energy_env eenv = {
			.util_delta     = task_util(p),
			.src_cpu        = task_cpu(p),
			.dst_cpu        = target_cpu,
			.task           = p,
			.trg_cpu        = target_cpu,
		};


#ifdef CONFIG_SCHED_WALT
               if (!walt_disabled && sysctl_sched_use_walt_cpu_util)
                       delta = task_util(p);
#endif
		/* Not enough spare capacity on previous cpu */
		if (__cpu_overutilized(task_cpu(p), delta)) {
			over_util = true;
			goto unlock;
		}

		nrg_diff = energy_diff(&eenv);
		if (nrg_diff >= 0 && !cpu_isolated(task_cpu(p))) {
			/* if previous cpu not idle, choose better another silbing */
			if (idle_cpu(task_cpu(p)))
				target_cpu = task_cpu(p);
			else
				target_cpu =  select_max_spare_capacity_cpu(p, task_cpu(p));
		}
	}

unlock:
	rcu_read_unlock();

	trace_energy_aware_wake_cpu(p, task_cpu(p), target_cpu,
			(int)task_util(p), nrg_diff, over_util, is_tiny);

	return target_cpu;
}
