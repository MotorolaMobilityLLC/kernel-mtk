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
static inline unsigned long task_util(struct task_struct *p);
static int select_max_spare_capacity(struct task_struct *p, int target);
static int __energy_diff(struct energy_env *eenv);
#ifdef CONFIG_SCHED_TUNE
static inline int energy_diff(struct energy_env *eenv);
#else
#define energy_diff(eenv) __energy_diff(eenv)
#endif

#ifndef cpu_isolated
#define cpu_isolated(cpu) 0
#endif

#ifndef CONFIG_MTK_SCHED_EAS_POWER_SUPPORT
static int l_plus_cpu = -1;
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
int select_max_spare_capacity(struct task_struct *p, int target)
{
	unsigned long int max_spare_capacity = 0;
	int max_spare_cpu = -1;
	struct cpumask cls_cpus;
	int cid = arch_get_cluster_id(target); /* cid of target CPU */
	int cpu = task_cpu(p);
	struct cpumask *tsk_cpus_allow = tsk_cpus_allowed(p);

	/* If the prevous cpu is cache affine and idle, choose it first. */
	if (cpu != l_plus_cpu && cpu != target &&
		cpus_share_cache(cpu, target) &&
		idle_cpu(cpu) && !cpu_isolated(cpu))
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
		if (cpu_rq(cpu)->rt.rt_nr_running &&
			likely(!is_rt_throttle(cpu)))
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
		int i = (sched_boost() ||
			(prefer_idle && (task_util(p) > stune_task_threshold)))
			?  nr_cpu_ids-iter_cpu-1 : iter_cpu;

		if (!cpu_online(i) || !cpumask_test_cpu(i, tsk_cpus_allow) ||
			cpu_isolated(i))
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

/*
 * Add a system-wide over-utilization indicator which
 * is updated in load-balance.
 */
static bool system_overutil;

static inline bool system_overutilized(int cpu)
{
	return system_overutil;
}

static inline unsigned long
__src_cpu_util(int cpu, int delta, unsigned long task_delta)
{
	unsigned long util = cpu_rq(cpu)->cfs.avg.util_avg;
	unsigned long capacity = capacity_orig_of(cpu);

#ifdef CONFIG_SCHED_WALT
	if (!walt_disabled && sysctl_sched_use_walt_cpu_util)
		util = (cpu_rq(cpu)->prev_runnable_sum << SCHED_LOAD_SHIFT) /
			walt_ravg_window;
#endif
	util = max(util, task_delta);
	delta += util;
	if (delta < 0)
		return 0;

	return (delta >= capacity) ? capacity : delta;
}

static unsigned long
__src_cpu_norm_util(int cpu, unsigned long capacity, int delta, int task_delta)
{
	int util = __src_cpu_util(cpu, delta, task_delta);

	if (util >= capacity)
		return SCHED_CAPACITY_SCALE;

	return (util << SCHED_CAPACITY_SHIFT)/capacity;
}

