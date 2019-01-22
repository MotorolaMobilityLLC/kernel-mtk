#include <linux/cgroup.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/percpu.h>
#include <linux/printk.h>
#include <linux/rcupdate.h>
#include <linux/slab.h>

#include <trace/events/sched.h>

#include "sched.h"
#include "tune.h"

#define MET_STUNE_DEBUG 1

#if MET_STUNE_DEBUG
#include <mt-plat/met_drv.h>
#endif

unsigned int sysctl_sched_cfs_boost __read_mostly;

static int default_stune_threshold;

/* Performance Boost region (B) threshold params */
static int perf_boost_idx;

/* Performance Constraint region (C) threshold params */
static int perf_constrain_idx;

/**
 * Performance-Energy (P-E) Space thresholds constants
 */
struct threshold_params {
	int nrg_gain;
	int cap_gain;
};

/*
 * System specific P-E space thresholds constants
 */
static struct threshold_params
threshold_gains[] = {
	{ 0, 5 }, /*   < 10% */
	{ 1, 5 }, /*   < 20% */
	{ 2, 5 }, /*   < 30% */
	{ 3, 5 }, /*   < 40% */
	{ 4, 5 }, /*   < 50% */
	{ 5, 4 }, /*   < 60% */
	{ 5, 3 }, /*   < 70% */
	{ 5, 2 }, /*   < 80% */
	{ 5, 1 }, /*   < 90% */
	{ 5, 0 }  /* <= 100% */
};

bool global_negative_flag;

static int
__schedtune_accept_deltas(int nrg_delta, int cap_delta,
			  int perf_boost_idx, int perf_constrain_idx)
{
	int payoff = -INT_MAX;
	int gain_idx = -1;
	int region = 0;

	/* Performance Boost (B) region */
	if (nrg_delta >= 0 && cap_delta > 0) {
		gain_idx = perf_boost_idx;
		region = 8;
	}
	/* Performance Constraint (C) region */
	else if (nrg_delta < 0 && cap_delta <= 0) {
		gain_idx = perf_constrain_idx;
		region = 6;
	}

	/* Default: reject schedule candidate */
	if (gain_idx == -1)
		return payoff;

	/*
	 * Evaluate "Performance Boost" vs "Energy Increase"
	 *
	 * - Performance Boost (B) region
	 *
	 *   Condition: nrg_delta > 0 && cap_delta > 0
	 *   Payoff criteria:
	 *     cap_gain / nrg_gain  < cap_delta / nrg_delta =
	 *     cap_gain * nrg_delta < cap_delta * nrg_gain
	 *   Note that since both nrg_gain and nrg_delta are positive, the
	 *   inequality does not change. Thus:
	 *
	 *     payoff = (cap_delta * nrg_gain) - (cap_gain * nrg_delta)
	 *
	 * - Performance Constraint (C) region
	 *
	 *   Condition: nrg_delta < 0 && cap_delta < 0
	 *   payoff criteria:
	 *     cap_gain / nrg_gain  > cap_delta / nrg_delta =
	 *     cap_gain * nrg_delta < cap_delta * nrg_gain
	 *   Note that since nrg_gain > 0 while nrg_delta < 0, the
	 *   inequality change. Thus:
	 *
	 *     payoff = (cap_delta * nrg_gain) - (cap_gain * nrg_delta)
	 *
	 * This means that, in case of same positive defined {cap,nrg}_gain
	 * for both the B and C regions, we can use the same payoff formula
	 * where a positive value represents the accept condition.
	 */
	payoff  = cap_delta * threshold_gains[gain_idx].nrg_gain;
	payoff -= nrg_delta * threshold_gains[gain_idx].cap_gain;

	trace_sched_tune_filter(
				nrg_delta, cap_delta,
				threshold_gains[gain_idx].nrg_gain,
				threshold_gains[gain_idx].cap_gain,
				payoff, region);

	return payoff;
}

#ifdef CONFIG_CGROUP_SCHEDTUNE
/*
 * EAS scheduler tunables for task groups.
 */
/* SchdTune tunables for a group of tasks */
struct schedtune {
	/* SchedTune CGroup subsystem */
	struct cgroup_subsys_state css;

	/* Boost group allocated ID */
	int idx;

	/* Boost value for tasks on that SchedTune CGroup */
	int boost;

	/* Performance Boost (B) region threshold params */
	int perf_boost_idx;

	/* Performance Constraint (C) region threshold params */
	int perf_constrain_idx;

	/*
	 * Hint to bias scheduling of tasks on that SchedTune CGroup
	 * towards idle CPUs
	 */
	int prefer_idle;
};

static inline struct schedtune *css_st(struct cgroup_subsys_state *css)
{
	return css ? container_of(css, struct schedtune, css) : NULL;
}

static inline struct schedtune *task_schedtune(struct task_struct *tsk)
{
	return css_st(task_css(tsk, schedtune_cgrp_id));
}

static inline struct schedtune *parent_st(struct schedtune *st)
{
	return css_st(st->css.parent);
}

/*
 * SchedTune root control group
 * The root control group is used to defined a system-wide boosting tuning,
 * which is applied to all tasks in the system.
 * Task specific boost tuning could be specified by creating and
 * configuring a child control group under the root one.
 * By default, system-wide boosting is disabled, i.e. no boosting is applied
 * to tasks which are not into a child control group.
 */
static struct schedtune
root_schedtune = {
	.boost	= 0,
	.perf_boost_idx = 0,
	.perf_constrain_idx = 0,
	.prefer_idle = 0,
};

int
schedtune_accept_deltas(int nrg_delta, int cap_delta,
			struct task_struct *task)
{
	struct schedtune *ct;
	int perf_boost_idx;
	int perf_constrain_idx;

	/* Optimal (O) region */
	if (nrg_delta < 0 && cap_delta > 0) {
		trace_sched_tune_filter(nrg_delta, cap_delta, 0, 0, 1, 0);
		return INT_MAX;
	}

	/* Suboptimal (S) region */
	if (nrg_delta > 0 && cap_delta < 0) {
		trace_sched_tune_filter(nrg_delta, cap_delta, 0, 0, -1, 5);
		return -INT_MAX;
	}

	/* Get task specific perf Boost/Constraints indexes */
	rcu_read_lock();
	ct = task_schedtune(task);
	perf_boost_idx = ct->perf_boost_idx;
	perf_constrain_idx = ct->perf_constrain_idx;
	rcu_read_unlock();

	return __schedtune_accept_deltas(nrg_delta, cap_delta,
			perf_boost_idx, perf_constrain_idx);
}

/*
 * Maximum number of boost groups to support
 * When per-task boosting is used we still allow only limited number of
 * boost groups for two main reasons:
 * 1. on a real system we usually have only few classes of workloads which
 *    make sense to boost with different values (e.g. background vs foreground
 *    tasks, interactive vs low-priority tasks)
 * 2. a limited number allows for a simpler and more memory/time efficient
 *    implementation especially for the computation of the per-CPU boost
 *    value
 */
#define BOOSTGROUPS_COUNT 4

/* Array of configured boostgroups */
static struct schedtune *allocated_group[BOOSTGROUPS_COUNT] = {
	&root_schedtune,
	NULL,
};

/* SchedTune boost groups
 * Keep track of all the boost groups which impact on CPU, for example when a
 * CPU has two RUNNABLE tasks belonging to two different boost groups and thus
 * likely with different boost values.
 * Since on each system we expect only a limited number of boost groups, here
 * we use a simple array to keep track of the metrics required to compute the
 * maximum per-CPU boosting value.
 */
struct boost_groups {
	/* Maximum boost value for all RUNNABLE tasks on a CPU */
	bool idle;
	int boost_max;
	struct {
		/* The boost for tasks on that boost group */
		int boost;
		/* Count of RUNNABLE tasks on that boost group */
		unsigned tasks;
	} group[BOOSTGROUPS_COUNT];
};

/* Boost groups affecting each CPU in the system */
DEFINE_PER_CPU(struct boost_groups, cpu_boost_groups);

static void
schedtune_cpu_update(int cpu)
{
	struct boost_groups *bg;
	int boost_max;
	int idx;

	bg = &per_cpu(cpu_boost_groups, cpu);

	/* The root boost group is always active */
	boost_max = bg->group[0].boost;
	for (idx = 1; idx < BOOSTGROUPS_COUNT; ++idx) {
		/*
		 * A boost group affects a CPU only if it has
		 * RUNNABLE tasks on that CPU
		 */
		if (bg->group[idx].tasks == 0)
			continue;
		boost_max = max(boost_max, bg->group[idx].boost);
	}
	/* Ensures boost_max is non-negative when all cgroup boost values
	 * are neagtive. Avoids under-accounting of cpu capacity which may cause
	 * task stacking and frequency spikes.
	 */
	/* mtk:
	 * If original path, max(boost_max, 0)
	 * If use mtk perfservice kernel API to update negative boost,
	 * when all group are neagtive, boost_max should lower than 0
	 * and it can decrease frequency.
	 */
	if (!global_negative_flag)
		boost_max = max(boost_max, 0);

	bg->boost_max = boost_max;
}

static int
schedtune_boostgroup_update(int idx, int boost)
{
	struct boost_groups *bg;
	int cur_boost_max;
	int old_boost;
	int cpu;

	/* Update per CPU boost groups */
	for_each_possible_cpu(cpu) {
		bg = &per_cpu(cpu_boost_groups, cpu);

		/*
		 * Keep track of current boost values to compute the per CPU
		 * maximum only when it has been affected by the new value of
		 * the updated boost group
		 */
		cur_boost_max = bg->boost_max;
		old_boost = bg->group[idx].boost;

		/* Update the boost value of this boost group */
		bg->group[idx].boost = boost;

		/* Check if this update increase current max */
		if (boost > cur_boost_max && bg->group[idx].tasks) {
			bg->boost_max = boost;
			trace_sched_tune_boostgroup_update(cpu, 1, bg->boost_max);
			continue;
		}

		/* Check if this update has decreased current max */
		if (cur_boost_max == old_boost && old_boost > boost) {
			schedtune_cpu_update(cpu);
			trace_sched_tune_boostgroup_update(cpu, -1, bg->boost_max);
			continue;
		}

		trace_sched_tune_boostgroup_update(cpu, 0, bg->boost_max);
	}

	return 0;
}

#define ENQUEUE_TASK  1
#define DEQUEUE_TASK -1

static inline void
schedtune_tasks_update(struct task_struct *p, int cpu, int idx, int task_count)
{
	struct boost_groups *bg = &per_cpu(cpu_boost_groups, cpu);
	int tasks = bg->group[idx].tasks + task_count;

	/* Update boosted tasks count while avoiding to make it negative */
	bg->group[idx].tasks = max(0, tasks);

	trace_sched_tune_tasks_update(p, cpu, tasks, idx,
			bg->group[idx].boost, bg->boost_max);

	/* Boost group activation or deactivation on that RQ */
	if (tasks == 1 || tasks == 0)
		schedtune_cpu_update(cpu);
}

/*
 * NOTE: This function must be called while holding the lock on the CPU RQ
 */
void schedtune_enqueue_task(struct task_struct *p, int cpu)
{
	struct schedtune *st;
	int idx;

	/*
	 * When a task is marked PF_EXITING by do_exit() it's going to be
	 * dequeued and enqueued multiple times in the exit path.
	 * Thus we avoid any further update, since we do not want to change
	 * CPU boosting while the task is exiting.
	 */
	if (p->flags & PF_EXITING)
		return;

	/* Get task boost group */
	rcu_read_lock();
	st = task_schedtune(p);
	idx = st->idx;
	rcu_read_unlock();

	schedtune_tasks_update(p, cpu, idx, ENQUEUE_TASK);
}

/*
 * NOTE: This function must be called while holding the lock on the CPU RQ
 */
void schedtune_dequeue_task(struct task_struct *p, int cpu)
{
	struct schedtune *st;
	int idx;

	/*
	 * When a task is marked PF_EXITING by do_exit() it's going to be
	 * dequeued and enqueued multiple times in the exit path.
	 * Thus we avoid any further update, since we do not want to change
	 * CPU boosting while the task is exiting.
	 * The last dequeue will be done by cgroup exit() callback.
	 */
	if (p->flags & PF_EXITING)
		return;

	/* Get task boost group */
	rcu_read_lock();
	st = task_schedtune(p);
	idx = st->idx;
	rcu_read_unlock();

	schedtune_tasks_update(p, cpu, idx, DEQUEUE_TASK);
}

int schedtune_cpu_boost(int cpu)
{
	struct boost_groups *bg;

	bg = &per_cpu(cpu_boost_groups, cpu);
	return bg->boost_max;
}

int schedtune_task_boost(struct task_struct *p)
{
	struct schedtune *st;
	int task_boost;

	/* Get task boost value */
	rcu_read_lock();
	st = task_schedtune(p);
	task_boost = st->boost;
	rcu_read_unlock();

	return task_boost;
}
#ifdef CONFIG_CPU_FREQ_GOV_SCHED

#if MET_STUNE_DEBUG
static char met_dvfs_info[5][16] = {
	"sched_dvfs_cid0",
	"sched_dvfs_cid1",
	"sched_dvfs_cid2",
	"NULL",
	"NULL"
};
#endif

static void update_freq_fastpath(void)
{
	int cid;

	if (!sched_freq())
		return;

#if MET_STUNE_DEBUG
	met_tag_oneshot(0, "sched_dvfs_fastpath", 1);
#endif

#ifdef CONFIG_MTK_SCHED_VIP_TASKS
	/* force migrating vip task to higher idle cpu */
	vip_task_force_migrate();
#endif

	/* for each cluster*/
	for (cid = 0; cid < arch_get_nr_clusters(); cid++) {
		unsigned long capacity = 0;
		int cpu;
		struct cpumask cls_cpus;
		int first_cpu = -1;
		unsigned int freq_new = 0;
		unsigned long req_cap = 0;

		arch_get_cluster_cpus(&cls_cpus, cid);

		for_each_cpu(cpu, &cls_cpus) {
			struct sched_capacity_reqs *scr;

			if (!cpu_online(cpu))
				continue;

			if (first_cpu < 0)
				first_cpu = cpu;

			scr = &per_cpu(cpu_sched_capacity_reqs, cpu);

			/* find boosted util per cpu.  */
			req_cap = boosted_cpu_util(cpu);

			/* Convert scale-invariant capacity to cpu. */
			req_cap = req_cap * SCHED_CAPACITY_SCALE / capacity_orig_of(cpu);

			req_cap += scr->rt;

			/* Add DVFS margin. */
			req_cap = req_cap * capacity_margin_dvfs / SCHED_CAPACITY_SCALE;

			req_cap += scr->dl;

			/* find max boosted util */
			capacity = max(capacity, req_cap);

			trace_sched_cpufreq_fastpath_request(cpu, req_cap, cpu_util(cpu),
					boosted_cpu_util(cpu), (int)scr->rt);
		}

		if (capacity > 0) { /* update freq in fast path */
			freq_new = capacity * arch_scale_get_max_freq(first_cpu) >> SCHED_CAPACITY_SHIFT;

			trace_sched_cpufreq_fastpath(cid, req_cap, freq_new);

			mt_cpufreq_set_by_schedule_load_cluster(cid, freq_new);
#if MET_STUNE_DEBUG
			met_tag_oneshot(0, met_dvfs_info[cid], freq_new);
#endif
		}

	}
#if MET_STUNE_DEBUG
	met_tag_oneshot(0, "sched_dvfs_fastpath", 0);
#endif
}
#endif

#if 0 /* no use */
int boost_write_for_perf_pid(int pid, int boost_value)
{
	struct task_struct *boost_task;
	struct schedtune *ct;
	unsigned threshold_idx;
	int boost_pct;

	if (boost_value >= 1000) {
		boost_value -= 1000;
		stune_task_threshold = 0;
	} else {
		stune_task_threshold = default_stune_threshold;
	}

	if (boost_value < -100 || boost_value > 100)
		printk_deferred("warning: perf boost value should be -100~100\n");

	if (boost_value >= 100)
		boost_value = 100;
	else if (boost_value <= -100)
		boost_value = -100;

	rcu_read_lock();

	boost_task = find_task_by_vpid(pid);
	if (boost_task) {
		ct = task_schedtune(boost_task);

		if (ct->idx == 0) {
			printk_deferred("error: don't boost perf task at root idx=%d, pid:%d\n", ct->idx, pid);
			rcu_read_unlock();
			return -EINVAL;
		}

		boost_pct = boost_value;

		/*
		 * Update threshold params for Performance Boost (B)
		 * and Performance Constraint (C) regions.
		 * The current implementatio uses the same cuts for both
		 * B and C regions.
		 */
		threshold_idx = clamp(boost_pct, 0, 99) / 10;
		ct->perf_boost_idx = threshold_idx;
		ct->perf_constrain_idx = threshold_idx;

		ct->boost = boost_value;

		/* Update CPU boost */
		schedtune_boostgroup_update(ct->idx, ct->boost);
	} else {
		printk_deferred("error: perf task no exist: pid=%d, boost=%d\n", pid, boost_value);
		rcu_read_unlock();
		return -EINVAL;
	}

	trace_sched_tune_config(ct->boost);

#if MET_STUNE_DEBUG
	/* perf: foreground with pid */
	if (ct->idx == 1)
		met_tag_oneshot(0, "sched_boost_fg", ct->boost);
#endif

	rcu_read_unlock();
	return 0;
}
#endif

int boost_write_for_perf_idx(int group_idx, int boost_value)
{
	struct schedtune *ct;
	unsigned threshold_idx;
	int boost_pct;
	bool dvfs_on_demand = false;
	int idx = 0;

	if (boost_value >= 2000) { /* dvfs short cut */
		boost_value -= 2000;
		stune_task_threshold = default_stune_threshold;

		dvfs_on_demand = true;
	} else if (boost_value >= 1000) { /* boost all tasks */
		boost_value -= 1000;
		stune_task_threshold = 0;
	} else { /* boost big task only */
		stune_task_threshold = default_stune_threshold;
	}

	if (boost_value < -100 || boost_value > 100)
		printk_deferred("warning: perf boost value should be -100~100\n");

	if (boost_value >= 100)
		boost_value = 100;
	else if (boost_value <= -100)
		boost_value = -100;

	if (boost_value < 0)
		global_negative_flag = true; /* set all group negative */
	else
		global_negative_flag = false;

	for (idx = 0; idx < BOOSTGROUPS_COUNT; idx++) {
		if (!global_negative_flag) /* positive path or google original path */
			idx = group_idx;

		ct = allocated_group[idx];

		if (ct) {
			rcu_read_lock();

			boost_pct = boost_value;

			/*
			 * Update threshold params for Performance Boost (B)
			 * and Performance Constraint (C) regions.
			 * The current implementatio uses the same cuts for both
			 * B and C regions.
			 */
			threshold_idx = clamp(boost_pct, 0, 99) / 10;
			ct->perf_boost_idx = threshold_idx;
			ct->perf_constrain_idx = threshold_idx;

			ct->boost = boost_value;

			trace_sched_tune_config(ct->boost);

			/* Update CPU boost */
			schedtune_boostgroup_update(ct->idx, ct->boost);
			rcu_read_unlock();

#if MET_STUNE_DEBUG
			/* foreground */
			if (ct->idx == 1)
				met_tag_oneshot(0, "sched_boost_fg", ct->boost);
			/* top-app */
			if (ct->idx == 3)
				met_tag_oneshot(0, "sched_boost_top", ct->boost);
#endif

			if (!global_negative_flag)
				break;

		} else {
			if (!global_negative_flag) { /* positive path or google original path */
				printk_deferred("error: perf boost for stune group no exist: idx=%d\n",
						idx);
				return -EINVAL;
			}

			break;
		}
	}

#ifdef CONFIG_CPU_FREQ_GOV_SCHED
	if (dvfs_on_demand)
		update_freq_fastpath();
#endif

	return 0;
}

int group_boost_read(int group_idx)
{
	struct schedtune *ct;
	int boost = 0;

	ct = allocated_group[group_idx];
	if (ct) {
		rcu_read_lock();
		boost = ct->boost;
		rcu_read_unlock();
	}

	return boost;
}
EXPORT_SYMBOL(group_boost_read);

/* mtk: a linear bosot value for tuning */
int linear_real_boost(int linear_boost)
{
	int target_cpu, usage;
	int boost;

	sched_max_util_task(&target_cpu, NULL, &usage, NULL);

	/* margin = (usage*linear_boost)/100; */
	/* (original_cap - usage)*boost/100 = margin; */
	boost = (usage*linear_boost)/(capacity_orig_of(target_cpu) - usage);

#if 0
	printk_deferred("Michael: (%d->%d) target_cpu=%d orig_cap=%ld usage=%ld\n",
			linear_boost, boost, target_cpu, capacity_orig_of(target_cpu), usage);
#endif

	return boost;
}
EXPORT_SYMBOL(linear_real_boost);

/* mtk: a linear bosot value for tuning */
int linear_real_boost_pid(int linear_boost, int pid)
{
	struct task_struct *boost_task = find_task_by_vpid(pid);
	int target_cpu;
	unsigned long usage;
	int boost;

	if (!boost_task)
		return linear_real_boost(linear_boost);

	usage = boost_task->se.avg.util_avg;
	target_cpu = task_cpu(boost_task);

	boost = (usage*linear_boost)/(capacity_orig_of(target_cpu) - usage);

#if 0
	printk_deferred("Michael2: (%d->%d) target_cpu=%d orig_cap=%ld usage=%ld pid=%d\n",
			linear_boost, boost, target_cpu, capacity_orig_of(target_cpu), usage, pid);
#endif

	return boost;
}
EXPORT_SYMBOL(linear_real_boost_pid);

int schedtune_prefer_idle(struct task_struct *p)
{
	struct schedtune *st;
	int prefer_idle;

	/* Get prefer_idle value */
	rcu_read_lock();
	st = task_schedtune(p);
	prefer_idle = st->prefer_idle;
	rcu_read_unlock();

	return prefer_idle;
}

static u64
prefer_idle_read(struct cgroup_subsys_state *css, struct cftype *cft)
{
	struct schedtune *st = css_st(css);

	return st->prefer_idle;
}

static int
prefer_idle_write(struct cgroup_subsys_state *css, struct cftype *cft,
	    u64 prefer_idle)
{
	struct schedtune *st = css_st(css);

	st->prefer_idle = prefer_idle;

	return 0;
}

static s64
boost_read(struct cgroup_subsys_state *css, struct cftype *cft)
{
	struct schedtune *st = css_st(css);

	return st->boost;
}

static int
boost_write(struct cgroup_subsys_state *css, struct cftype *cft,
	    s64 boost)
{
	struct schedtune *st = css_st(css);
	unsigned threshold_idx;
	int boost_pct;
	bool dvfs_on_demand = false;

	if (boost >= 2000) { /* dvfs short cut */
		boost -= 2000;
		stune_task_threshold = default_stune_threshold;

		dvfs_on_demand = true;
	} else if (boost >= 1000) {
		boost -= 1000;
		stune_task_threshold = 0;
	} else
		stune_task_threshold = default_stune_threshold;

	if (boost < -100 || boost > 100) {
		stune_task_threshold = default_stune_threshold;
		return -EINVAL;
	}

	global_negative_flag = false;

	boost_pct = boost;

	/*
	 * Update threshold params for Performance Boost (B)
	 * and Performance Constraint (C) regions.
	 * The current implementatio uses the same cuts for both
	 * B and C regions.
	 */
	threshold_idx = clamp(boost_pct, 0, 99) / 10;
	st->perf_boost_idx = threshold_idx;
	st->perf_constrain_idx = threshold_idx;

	st->boost = boost;
	if (css == &root_schedtune.css) {
		sysctl_sched_cfs_boost = boost;
		perf_boost_idx  = threshold_idx;
		perf_constrain_idx  = threshold_idx;
	}

	/* Update CPU boost */
	schedtune_boostgroup_update(st->idx, st->boost);

#ifdef CONFIG_CPU_FREQ_GOV_SCHED
	if (dvfs_on_demand)
		update_freq_fastpath();
#endif

	trace_sched_tune_config(st->boost);

#if MET_STUNE_DEBUG
	/* user: foreground */
	if (st->idx == 1)
		met_tag_oneshot(0, "sched_boost_user_fg", st->boost);
	/* user: top-app */
	if (st->idx == 3)
		met_tag_oneshot(0, "sched_boost_user_top", st->boost);
#endif

	return 0;
}

static struct cftype files[] = {
	{
		.name = "boost",
		.read_s64 = boost_read,
		.write_s64 = boost_write,
	},
	{
		.name = "prefer_idle",
		.read_u64 = prefer_idle_read,
		.write_u64 = prefer_idle_write,
	},
	{ }	/* terminate */
};

static int
schedtune_boostgroup_init(struct schedtune *st)
{
	struct boost_groups *bg;
	int cpu;

	/* Keep track of allocated boost groups */
	allocated_group[st->idx] = st;

	/* Initialize the per CPU boost groups */
	for_each_possible_cpu(cpu) {
		bg = &per_cpu(cpu_boost_groups, cpu);
		bg->group[st->idx].boost = 0;
		bg->group[st->idx].tasks = 0;
	}

	return 0;
}

static struct cgroup_subsys_state *
schedtune_css_alloc(struct cgroup_subsys_state *parent_css)
{
	struct schedtune *st;
	int idx;

	if (!parent_css)
		return &root_schedtune.css;

	/* Allow only single level hierachies */
	if (parent_css != &root_schedtune.css) {
		pr_err("Nested SchedTune boosting groups not allowed\n");
		return ERR_PTR(-ENOMEM);
	}

	/* Allow only a limited number of boosting groups */
	for (idx = 1; idx < BOOSTGROUPS_COUNT; ++idx)
		if (!allocated_group[idx])
			break;
	if (idx == BOOSTGROUPS_COUNT) {
		pr_err("Trying to create more than %d SchedTune boosting groups\n",
		       BOOSTGROUPS_COUNT);
		return ERR_PTR(-ENOSPC);
	}

	st = kzalloc(sizeof(*st), GFP_KERNEL);
	if (!st)
		goto out;

	/* Initialize per CPUs boost group support */
	st->idx = idx;
	if (schedtune_boostgroup_init(st))
		goto release;

	return &st->css;

release:
	kfree(st);
out:
	return ERR_PTR(-ENOMEM);
}

static void
schedtune_boostgroup_release(struct schedtune *st)
{
	/* Reset this boost group */
	schedtune_boostgroup_update(st->idx, 0);

	/* Keep track of allocated boost groups */
	allocated_group[st->idx] = NULL;
}

static void
schedtune_css_free(struct cgroup_subsys_state *css)
{
	struct schedtune *st = css_st(css);

	schedtune_boostgroup_release(st);
	kfree(st);
}

struct cgroup_subsys schedtune_cgrp_subsys = {
	.css_alloc	= schedtune_css_alloc,
	.css_free	= schedtune_css_free,
	.legacy_cftypes	= files,
	.early_init	= 1,
};

static inline void
schedtune_init_cgroups(void)
{
	struct boost_groups *bg;
	int cpu;

	/* Initialize the per CPU boost groups */
	for_each_possible_cpu(cpu) {
		bg = &per_cpu(cpu_boost_groups, cpu);
		memset(bg, 0, sizeof(struct boost_groups));
	}

	pr_info("schedtune: configured to support %d boost groups\n",
		BOOSTGROUPS_COUNT);
}

#else /* CONFIG_CGROUP_SCHEDTUNE */

int
schedtune_accept_deltas(int nrg_delta, int cap_delta,
			struct task_struct *task)
{
	/* Optimal (O) region */
	if (nrg_delta < 0 && cap_delta > 0) {
		trace_sched_tune_filter(nrg_delta, cap_delta, 0, 0, 1, 0);
		return INT_MAX;
	}

	/* Suboptimal (S) region */
	if (nrg_delta > 0 && cap_delta < 0) {
		trace_sched_tune_filter(nrg_delta, cap_delta, 0, 0, -1, 5);
		return -INT_MAX;
	}

	return __schedtune_accept_deltas(nrg_delta, cap_delta,
			perf_boost_idx, perf_constrain_idx);
}

#endif /* CONFIG_CGROUP_SCHEDTUNE */

int
sysctl_sched_cfs_boost_handler(struct ctl_table *table, int write,
			       void __user *buffer, size_t *lenp,
			       loff_t *ppos)
{
	int ret = proc_dointvec_minmax(table, write, buffer, lenp, ppos);
	unsigned threshold_idx;
	int boost_pct;

	if (ret || !write)
		return ret;

	if (sysctl_sched_cfs_boost < -100 || sysctl_sched_cfs_boost > 100)
		return -EINVAL;
	boost_pct = sysctl_sched_cfs_boost;

	/*
	 * Update threshold params for Performance Boost (B)
	 * and Performance Constraint (C) regions.
	 * The current implementatio uses the same cuts for both
	 * B and C regions.
	 */
	threshold_idx = clamp(boost_pct, 0, 99) / 10;
	perf_boost_idx = threshold_idx;
	perf_constrain_idx = threshold_idx;

	return 0;
}

#ifdef CONFIG_SCHED_DEBUG
static void
schedtune_test_nrg(unsigned long delta_pwr)
{
	unsigned long test_delta_pwr;
	unsigned long test_norm_pwr;
	int idx;

	/*
	 * Check normalization constants using some constant system
	 * energy values
	 */
	pr_info("schedtune: verify normalization constants...\n");
	for (idx = 0; idx < 6; ++idx) {
		test_delta_pwr = delta_pwr >> idx;

		/* Normalize on max energy for target platform */
		test_norm_pwr = reciprocal_divide(
					test_delta_pwr << SCHED_LOAD_SHIFT,
					schedtune_target_nrg.rdiv);

		pr_info("schedtune: max_pwr/2^%d: %4lu => norm_pwr: %5lu\n",
			idx, test_delta_pwr, test_norm_pwr);
	}
}
#else
#define schedtune_test_nrg(delta_pwr)
#endif

void show_ste_info(void)
{
	struct target_nrg *ste = &schedtune_target_nrg;
	int cluster_nr = arch_get_nr_clusters();
	int i;

	printk_deferred("STE: min=%lu max=%lu\n",
			ste->max_power, ste->min_power);

	for (i = 0; i < cluster_nr; i++) {
		printk_deferred("STE: cid=%d max_dync=%lu max_stc=%lu\n",
				i, ste->max_dyn_pwr[i], ste->max_stc_pwr[i]);
	}
}

void show_pwr_info(void)
{
	struct cpumask cluster_cpus;
	char str[32];
	unsigned long dyn_pwr;
	unsigned long stc_pwr;
	const struct sched_group_energy *cluster_energy, *core_energy;
	int cpu;
	int cluster_id = 0;
	int i = 0;
	int cluster_first_cpu = 0;

	/* Get num of all clusters */
	cluster_id = arch_get_nr_clusters();
	for (i = 0; i < cluster_id; i++) {
		arch_get_cluster_cpus(&cluster_cpus, i);
		cluster_first_cpu = cpumask_first(&cluster_cpus);

		snprintf(str, 32, "CLUSTER[%*pbl]",
				cpumask_pr_args(&cluster_cpus));

		/* Get Cluster energy using EM data of first CPU in this cluster */
		cluster_energy = cpu_cluster_energy(cluster_first_cpu);
		dyn_pwr = cluster_energy->cap_states[cluster_energy->nr_cap_states - 1].dyn_pwr;
		stc_pwr = cluster_energy->cap_states[cluster_energy->nr_cap_states - 1].lkg_pwr[0];
		pr_info("pwr_tlb: %-17s dyn_pwr: %5lu stc_pwr: %5lu\n",
				str, dyn_pwr, stc_pwr);

		/* Get CPU energy using EM data for each CPU in this cluster */
		for_each_cpu(cpu, &cluster_cpus) {
			core_energy = cpu_core_energy(cpu);
			dyn_pwr = core_energy->cap_states[core_energy->nr_cap_states - 1].dyn_pwr;
			stc_pwr = core_energy->cap_states[core_energy->nr_cap_states - 1].lkg_pwr[0];

			snprintf(str, 32, "CPU[%d]", cpu);
			pr_info("pwr_tlb: %-17s dyn_pwr: %5lu stc_pwr: %5lu\n",
					str, dyn_pwr, stc_pwr);
		}
	}

}


#ifndef CONFIG_MTK_ACAO
/*
 * mtk: Because system only eight cores online when init, we compute
 * the min/max power consumption of all possible clusters and CPUs.
 */
static void
schedtune_add_cluster_nrg_hotplug(struct target_nrg *ste, struct sched_group *sg)
{
	struct cpumask cluster_cpus;
	char str[32];
	unsigned long min_pwr;
	unsigned long max_pwr;
	const struct sched_group_energy *cluster_energy, *core_energy;
	int cpu;
	int cluster_id = 0;
	int i = 0;
	int cluster_first_cpu = 0;

	/* Get num of all clusters */
	cluster_id = arch_get_nr_clusters();
	for (i = 0; i < cluster_id ; i++) {
		arch_get_cluster_cpus(&cluster_cpus, i);
		cluster_first_cpu = cpumask_first(&cluster_cpus);

		snprintf(str, 32, "CLUSTER[%*pbl]",
			cpumask_pr_args(&cluster_cpus));

		/* Get Cluster energy using EM data of first CPU in this cluster */
		cluster_energy = cpu_cluster_energy(cluster_first_cpu);
		min_pwr = cluster_energy->idle_states[cluster_energy->nr_idle_states - 1].power;
		max_pwr = (cluster_energy->cap_states[cluster_energy->nr_cap_states - 1].dyn_pwr +
			   cluster_energy->cap_states[cluster_energy->nr_cap_states - 1].lkg_pwr[0]);
		pr_info("schedtune: %-17s min_pwr: %5lu max_pwr: %5lu\n",
			str, min_pwr, max_pwr);

		ste->min_power += min_pwr;
		ste->max_power += max_pwr;

		/* Get CPU energy using EM data for each CPU in this cluster */
		for_each_cpu(cpu, &cluster_cpus) {
			core_energy = cpu_core_energy(cpu);
			min_pwr = core_energy->idle_states[core_energy->nr_idle_states - 1].power;
			max_pwr = (core_energy->cap_states[core_energy->nr_cap_states - 1].dyn_pwr +
				   core_energy->cap_states[core_energy->nr_cap_states - 1].lkg_pwr[0]);

			ste->max_dyn_pwr[i] = core_energy->cap_states[core_energy->nr_cap_states - 1].dyn_pwr;
			ste->max_stc_pwr[i] = core_energy->cap_states[core_energy->nr_cap_states - 1].lkg_pwr[0];

			ste->min_power += min_pwr;
			ste->max_power += max_pwr;

			snprintf(str, 32, "CPU[%d]", cpu);
			pr_info("schedtune: %-17s min_pwr: %5lu max_pwr: %5lu\n",
				str, min_pwr, max_pwr);
		}
	}
}
#else
/*
 * Compute the min/max power consumption of a cluster and all its CPUs
 */
static void
schedtune_add_cluster_nrg(
		struct sched_domain *sd,
		struct sched_group *sg,
		struct target_nrg *ste)
{
	struct sched_domain *sd2;
	struct sched_group *sg2;

	struct cpumask *cluster_cpus;
	char str[32];

	unsigned long min_pwr;
	unsigned long max_pwr;
	int cpu;

	/* Get Cluster energy using EM data for the first CPU */
	cluster_cpus = sched_group_cpus(sg);
	snprintf(str, 32, "CLUSTER[%*pbl]",
		 cpumask_pr_args(cluster_cpus));

	min_pwr = sg->sge->idle_states[sg->sge->nr_idle_states - 1].power;
	max_pwr = sg->sge->cap_states[sg->sge->nr_cap_states - 1].dyn_pwr;
	pr_info("schedtune: %-17s min_pwr: %5lu max_pwr: %5lu\n",
		str, min_pwr, max_pwr);

	/*
	 * Keep track of this cluster's energy in the computation of the
	 * overall system energy
	 */
	ste->min_power += min_pwr;
	ste->max_power += max_pwr;

	/* Get CPU energy using EM data for each CPU in the group */
	for_each_cpu(cpu, cluster_cpus) {
		/* Get a SD view for the specific CPU */
		for_each_domain(cpu, sd2) {
			/* Get the CPU group */
			sg2 = sd2->groups;
			min_pwr = sg2->sge->idle_states[sg2->sge->nr_idle_states - 1].power;
			max_pwr = sg2->sge->cap_states[sg2->sge->nr_cap_states - 1].dyn_pwr;

			ste->min_power += min_pwr;
			ste->max_power += max_pwr;

			snprintf(str, 32, "CPU[%d]", cpu);
			pr_info("schedtune: %-17s min_pwr: %5lu max_pwr: %5lu\n",
				str, min_pwr, max_pwr);

			/*
			 * Assume we have EM data only at the CPU and
			 * the upper CLUSTER level
			 */
			BUG_ON(!cpumask_equal(
				sched_group_cpus(sg),
				sched_group_cpus(sd2->parent->groups)
				));
			break;
		}
	}
}
#endif /* !define CONFIG_MTK_ACAO */


/*
 * Initialize the constants required to compute normalized energy.
 * The values of these constants depends on the EM data for the specific
 * target system and topology.
 * Thus, this function is expected to be called by the code
 * that bind the EM to the topology information.
 */
static int
schedtune_init(void)
{
	struct target_nrg *ste = &schedtune_target_nrg;
	unsigned long delta_pwr = 0;
#ifndef CONFIG_MTK_ACAO
	const struct sched_group_energy *sge_core;
#else
	struct sched_domain *sd;
	struct sched_group *sg;
#endif
	pr_info("schedtune: init normalization constants...\n");
	ste->max_power = 0;
	ste->min_power = 0;
	memset(ste->max_dyn_pwr, 0, sizeof(ste->max_dyn_pwr));
	memset(ste->max_stc_pwr, 0, sizeof(ste->max_stc_pwr));

	rcu_read_lock();

	/*
	 * When EAS is in use, we always have a pointer to the highest SD
	 * which provides EM data.
	 */
#ifndef CONFIG_MTK_ACAO
	sge_core = cpu_core_energy(0); /* first CPU in system */

	if (!sge_core) {
		pr_info("schedtune: no energy model data\n");
		goto nodata;
	}

	default_stune_threshold = sge_core->cap_states[0].cap;
#else
	sd = rcu_dereference(per_cpu(sd_ea, cpumask_first(cpu_online_mask)));

	if (!sd) {
		pr_info("schedtune: no energy model data\n");
		goto nodata;
	}

	sg = sd->groups;

	default_stune_threshold = sg->sge->cap_states[0].cap;
#endif

#ifndef CONFIG_MTK_ACAO
	/* mtk: compute max_power & min_power of all possible cores, not only online cores. */
	schedtune_add_cluster_nrg_hotplug(ste, NULL);
#else
	do {
		schedtune_add_cluster_nrg(sd, sg, ste);
	} while (sg = sg->next, sg != sd->groups);
#endif
	rcu_read_unlock();

	pr_info("schedtune: %-17s min_pwr: %5lu max_pwr: %5lu\n",
		"SYSTEM", ste->min_power, ste->max_power);

	/* Compute normalization constants */
	delta_pwr = ste->max_power - ste->min_power;
	if (delta_pwr > 0)
		ste->rdiv = reciprocal_value(delta_pwr);
	else {
		ste->rdiv.m = 0;
		ste->rdiv.sh1 = 0;
		ste->rdiv.sh2 = 0;
	}
	pr_info("schedtune: using normalization constants mul: %u sh1: %u sh2: %u\n",
		ste->rdiv.m, ste->rdiv.sh1, ste->rdiv.sh2);

	schedtune_test_nrg(delta_pwr);

#ifdef CONFIG_CGROUP_SCHEDTUNE
	schedtune_init_cgroups();
#else
	pr_info("schedtune: configured to support global boosting only\n");
#endif

	return 0;

nodata:
	pr_warn("schedtune: disabled!\n");
	rcu_read_unlock();
	return -EINVAL;
}

late_initcall_sync(schedtune_init);
