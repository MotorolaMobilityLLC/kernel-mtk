/*
 *  Copyright (C)  2015 Michael Turquette <mturquette@linaro.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/cpufreq.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/percpu.h>
#include <linux/irq_work.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <trace/events/sched.h>

#include "sched.h"
#include "cpufreq_sched.h"

/* next throttling period expiry if increasing OPP */
#define THROTTLE_DOWN_NSEC     2000000 /* 2ms default */
/* next throttling period expiry if decreasing OPP */
#define THROTTLE_UP_NSEC       500000  /* 500us default */

#define THROTTLE_NSEC          2000000 /* 2ms default */

#define MAX_CLUSTER_NR 3

#ifdef CONFIG_CPU_FREQ_SCHED_ASSIST
struct static_key __read_mostly __sched_freq = STATIC_KEY_INIT_TRUE;
static DEFINE_PER_CPU(unsigned long, freq_scale) = SCHED_CAPACITY_SCALE;
#else /* GOV_SCHED */
struct static_key __read_mostly __sched_freq = STATIC_KEY_INIT_FALSE;
/* To confirm kthread if created */
static bool g_inited[MAX_CLUSTER_NR] = {false};
#endif

static bool __read_mostly cpufreq_driver_slow;

#ifndef CONFIG_CPU_FREQ_DEFAULT_GOV_SCHED
static struct cpufreq_governor cpufreq_gov_sched;
#endif

static DEFINE_PER_CPU(unsigned long, enabled);
DEFINE_PER_CPU(struct sched_capacity_reqs, cpu_sched_capacity_reqs);

/* keep goverdata as gloabl variable */
static struct gov_data *g_gd[MAX_CLUSTER_NR] = { NULL };


#define DEBUG 0
#define DEBUG_KLOG 0

#if DEBUG_KLOG
#define printk_dbg(f, a...) printk_deferred("[scheddvfs] "f, ##a)
#else
#define printk_dbg(f, a...) do {} while (0)
#endif

#include <mt-plat/met_drv.h>

struct sugov_cpu {
	struct sugov_policy *sg_policy;

	unsigned int cached_raw_freq;
	unsigned long iowait_boost;
	unsigned long iowait_boost_max;
	u64 last_update;

	/* The fields below are only needed when sharing a policy. */
	unsigned long util;
	unsigned long max;
	unsigned int flags;
	int idle;
};

static DEFINE_PER_CPU(struct sugov_cpu, sugov_cpu);

static void sugov_set_iowait_boost(struct sugov_cpu *sg_cpu, u64 time,
		unsigned int flags)
{
	if (flags == SCHE_IOWAIT)
		sg_cpu->iowait_boost = sg_cpu->iowait_boost_max;
	else if (sg_cpu->iowait_boost) {
		s64 delta_ns = time - sg_cpu->last_update;

		/* Clear iowait_boost if the CPU apprears to have been idle. */
		if (delta_ns > TICK_NSEC)
			sg_cpu->iowait_boost = 0;
	}
}

static char met_iowait_info[10][32] = {
	"sched_ioboost_cpu0",
	"sched_ioboost_cpu1",
	"sched_ioboost_cpu2",
	"sched_ioboost_cpu3",
	"sched_ioboost_cpu4",
	"sched_ioboost_cpu5",
	"sched_ioboost_cpu6",
	"sched_ioboost_cpu7",
	"NULL",
	"NULL"
};

static char met_dvfs_info[5][16] = {
	"sched_dvfs_cid0",
	"sched_dvfs_cid1",
	"sched_dvfs_cid2",
	"NULL",
	"NULL"
};

unsigned long int min_boost_freq[3] = {0};

void (*cpufreq_notifer_fp)(int cluster_id, unsigned long freq);
EXPORT_SYMBOL(cpufreq_notifer_fp);

unsigned int capacity_margin_dvfs = DEFAULT_CAP_MARGIN_DVFS;
int dbg_id = DEBUG_FREQ_DISABLED;

/**
 * gov_data - per-policy data internal to the governor
 * @up_throttle: next throttling period expiry if increasing OPP
 * @down_throttle: next throttling period expiry if decreasing OPP
 * @up_throttle_nsec: throttle period length in nanoseconds if increasing OPP
 * @down_throttle_nsec: throttle period length in nanoseconds if decreasing OPP
 * @task: worker thread for dvfs transition that may block/sleep
 * @irq_work: callback used to wake up worker thread
 * @requested_freq: last frequency requested by the sched governor
 *
 * struct gov_data is the per-policy cpufreq_sched-specific data structure. A
 * per-policy instance of it is created when the cpufreq_sched governor receives
 * the CPUFREQ_GOV_START condition and a pointer to it exists in the gov_data
 * member of struct cpufreq_policy.
 *
 * Readers of this data must call down_read(policy->rwsem). Writers must
 * call down_write(policy->rwsem).
 */
struct gov_data {
	ktime_t throttle;
	ktime_t up_throttle;
	ktime_t down_throttle;
	unsigned int up_throttle_nsec;
	unsigned int down_throttle_nsec;
	unsigned int throttle_nsec;
	struct task_struct *task;
	struct irq_work irq_work;
	unsigned int requested_freq;
	struct cpufreq_policy *policy;
	int target_cpu;
	int cid;
	enum throttle_type thro_type; /* throttle up or down */
	u64 last_freq_update_time;
};

void show_freq_kernel_log(int dbg_id, int cid, unsigned int freq)
{
	if (dbg_id == cid || dbg_id == DEBUG_FREQ_ALL)
		printk_deferred("[name:sched_power&] cid=%d freq=%u\n", cid, freq);
}

#ifdef CONFIG_CPU_FREQ_SCHED_ASSIST
unsigned long cpufreq_scale_freq_capacity(struct sched_domain *sd, int cpu)
{
	return per_cpu(freq_scale, cpu);
}
#endif

static void cpufreq_sched_try_driver_target(int target_cpu, struct cpufreq_policy *policy,
					    unsigned int freq, int type)
{
	struct gov_data *gd;
	int cid;
	unsigned int boost_min;
	unsigned long scale;
	struct cpumask cls_cpus;
	int cpu = target_cpu;
	unsigned int max, min;

	cid = arch_get_cluster_id(target_cpu);

	if (cid >= MAX_CLUSTER_NR || cid < 0) {
		WARN_ON(1);
		return;
	}

	/* policy may be NOT trusted!!! */
	gd = g_gd[cid];

	/* SSPM should support! */
	if (dbg_id  < DEBUG_FREQ_DISABLED)
		show_freq_kernel_log(dbg_id, cid, freq);

	/* Carefully! platform related */
	freq = mt_cpufreq_find_close_freq(cid, freq);

	/* clamp frequency for governor limit */
	max = arch_scale_get_max_freq(target_cpu);
	min = arch_scale_get_min_freq(target_cpu);

	freq = clamp(freq, min, max);

	/* clamp frequency by boost limit */
	boost_min = min_boost_freq[target_cpu >> 2];
	freq = clamp(freq, boost_min, max);

	if (boost_min && freq > boost_min)
		if (cpufreq_notifer_fp)
			cpufreq_notifer_fp(cid, freq);

	/* no freq = 0 case */
	if (!freq)
		return;

	scale = (freq << SCHED_CAPACITY_SHIFT) / max;

	arch_get_cluster_cpus(&cls_cpus, cid);

	for_each_cpu(cpu, &cls_cpus) {
		#ifdef CONFIG_CPU_FREQ_SCHED_ASSIST
		/* update current freq immediately if sched_assisted */
		per_cpu(freq_scale, cpu) = scale;
		arch_scale_set_curr_freq(cpu, freq);
		#endif

		#ifdef CONFIG_SCHED_WALT
		walt_cpufreq_notifier_trans(cpu, freq);
		#endif
	}

	printk_dbg("%s: cid=%d cpu=%d freq=%u  max_freq=%u\n",
			__func__,
			cid, gd->target_cpu, freq, max);

#ifdef CONFIG_CPU_FREQ_SCHED_ASSIST
	mt_cpufreq_set_by_schedule_load_cluster(cid, freq);
#else
	policy = cpufreq_cpu_get(gd->target_cpu);

	if (IS_ERR_OR_NULL(policy))
		return;

	if (policy->governor != &cpufreq_gov_sched ||
		!policy->governor_data)
		return;

	/* avoid race with cpufreq_sched_stop */
	if (!down_write_trylock(&policy->rwsem))
		return;

	__cpufreq_driver_target(policy, freq, CPUFREQ_RELATION_L);

	up_write(&policy->rwsem);

	if (policy)
		cpufreq_cpu_put(policy);
#endif

	met_tag_oneshot(0, met_dvfs_info[cid], freq);

	/* avoid inteference betwewn increasing/decreasing OPP */
	if (gd->thro_type == DVFS_THROTTLE_UP)
		gd->up_throttle = ktime_add_ns(ktime_get(), gd->up_throttle_nsec);
	else
		gd->down_throttle = ktime_add_ns(ktime_get(), gd->down_throttle_nsec);

	gd->throttle = ktime_add_ns(ktime_get(), gd->throttle_nsec);
}

#if 0
static bool finish_last_request(struct gov_data *gd)
{
	ktime_t now = ktime_get();

	if (ktime_after(now, gd->throttle))
		return false;

	while (1) {
		int usec_left = ktime_to_ns(ktime_sub(gd->throttle, now));

		usec_left /= NSEC_PER_USEC;
		usleep_range(usec_left, usec_left + 100);
		now = ktime_get();
		if (ktime_after(now, gd->throttle))
			return true;
	}
}
#endif


#ifndef CONFIG_CPU_FREQ_SCHED_ASSIST
/*
 * we pass in struct cpufreq_policy. This is safe because changing out the
 * policy requires a call to __cpufreq_governor(policy, CPUFREQ_GOV_STOP),
 * which tears down all of the data structures and __cpufreq_governor(policy,
 * CPUFREQ_GOV_START) will do a full rebuild, including this kthread with the
 * new policy pointer
 */
static int cpufreq_sched_thread(void *data)
{
	struct cpufreq_policy *policy;
	struct gov_data *gd;
	/* unsigned int new_request = 0; */
	int cpu;
	/* unsigned int last_request = 0; */

	policy = (struct cpufreq_policy *) data;
	gd = policy->governor_data;
	cpu = g_gd[gd->cid]->target_cpu;

	do {
		set_current_state(TASK_INTERRUPTIBLE);
		schedule();

		if (kthread_should_stop())
			break;

		cpufreq_sched_try_driver_target(cpu, policy, g_gd[gd->cid]->requested_freq, SCHE_INVALID);
#if 0
		new_request = gd->requested_freq;
		if (new_request == last_request) {
			set_current_state(TASK_INTERRUPTIBLE);
			schedule();
		} else {
			/*
			 * if the frequency thread sleeps while waiting to be
			 * unthrottled, start over to check for a newer request
			 */
			if (finish_last_request(gd))
				continue;
			last_request = new_request;
			cpufreq_sched_try_driver_target(-1, policy, new_request);
		}
#endif
	} while (!kthread_should_stop());

	return 0;
}

static void cpufreq_sched_irq_work(struct irq_work *irq_work)
{
	struct gov_data *gd;

	if (!irq_work)
		return;

	gd = container_of(irq_work, struct gov_data, irq_work);

	wake_up_process(gd->task);
}
#endif

static void update_fdomain_capacity_request(int cpu, int type)
{
	unsigned int freq_new, cpu_tmp;
	struct gov_data *gd;
	unsigned long capacity = 0;
	int cid = arch_get_cluster_id(cpu);
	struct cpumask cls_cpus;
	s64 delta_ns;
	unsigned long arch_max_freq = arch_scale_get_max_freq(cpu);
	u64 time = cpu_rq(cpu)->clock;
	struct cpufreq_policy *policy = NULL;
	ktime_t throttle, now;
	unsigned int cur_freq;

	/*
	 * Avoid grabbing the policy if possible. A test is still
	 * required after locking the CPU's policy to avoid racing
	 * with the governor changing.
	 */
	if (!per_cpu(enabled, cpu))
		return;

	gd = g_gd[cid];

#ifdef CONFIG_CPU_FREQ_SCHED_ASSIST
	if (!mt_cpufreq_get_sched_enable())
		goto out;
#endif

	arch_get_cluster_cpus(&cls_cpus, cid);

	/* find max capacity requested by cpus in this policy */
	for_each_cpu(cpu_tmp, &cls_cpus) {
		struct sched_capacity_reqs *scr;
		unsigned long boosted_util = 0;
		struct sugov_cpu *sg_cpu = &per_cpu(sugov_cpu, cpu_tmp);

		if (!cpu_online(cpu_tmp))
			continue;

		/* convert IO boosted freq to capacity */
		boosted_util = (sg_cpu->iowait_boost << SCHED_CAPACITY_SHIFT) /
					arch_max_freq;

		/* iowait boost */
		if (cpu_tmp == cpu) {
			/* IO boosting only for CFS */
			if (type != SCHE_RT && type != SCHE_DL) {

				/* update iowait_boost */
				sugov_set_iowait_boost(sg_cpu, time, type);

				 /* convert IO boosted freq to capacity */
				boosted_util = (sg_cpu->iowait_boost << SCHED_CAPACITY_SHIFT) /
							arch_max_freq;

				met_tag_oneshot(0, met_iowait_info[cpu_tmp], sg_cpu->iowait_boost);

				/* the boost is reduced by half during each following update */
				sg_cpu->iowait_boost >>= 1;
			}
			sg_cpu->last_update = time;
		}
		scr = &per_cpu(cpu_sched_capacity_reqs, cpu_tmp);

		/*
		 * If the CPU utilization was last updated before the previous
		 * frequency update and the time elapsed between the last update
		 * of the CPU utilization and the last frequency update is long
		 * enough, don't take the CPU into account as it probably is
		 * idle now (and clear iowait_boost for it).
		 */
		delta_ns = gd->last_freq_update_time - cpu_rq(cpu_tmp)->clock;

		if (delta_ns > TICK_NSEC * 2) {/* 2tick */
			sg_cpu->iowait_boost = 0;
			sg_cpu->idle = 1;
			continue;
		}

		sg_cpu->idle = 0;

		/* check if IO boosting */
		if (boosted_util > scr->total)
			capacity = max(capacity, boosted_util);
		else
			capacity = max(capacity, scr->total);
	}

	freq_new = capacity * arch_max_freq >> SCHED_CAPACITY_SHIFT;

	/* check throttle time */

	now = ktime_get();

	cur_freq =  gd->requested_freq;

	gd->requested_freq = freq_new;
	gd->target_cpu = cpu;

	throttle = gd->requested_freq < cur_freq ?
			gd->down_throttle : gd->up_throttle;

	gd->thro_type = gd->requested_freq < cur_freq ?
			DVFS_THROTTLE_DOWN : DVFS_THROTTLE_UP;

	if (ktime_before(now, throttle))
		goto out;

	gd->last_freq_update_time = time;

	/*
	 * Throttling is not yet supported on platforms with fast cpufreq
	 * drivers.
	 */
	if (cpufreq_driver_slow)
		irq_work_queue_on(&gd->irq_work, cpu);
	else
		cpufreq_sched_try_driver_target(cpu, policy, freq_new, type);

out:
	if (policy)
		cpufreq_cpu_put(policy);
}

void update_cpu_capacity_request(int cpu, bool request, int type)
{
	unsigned long new_capacity;
	struct sched_capacity_reqs *scr;

	/* The rq lock serializes access to the CPU's sched_capacity_reqs. */
	lockdep_assert_held(&cpu_rq(cpu)->lock);

	scr = &per_cpu(cpu_sched_capacity_reqs, cpu);

	new_capacity = scr->cfs + scr->rt;
	new_capacity = new_capacity * capacity_margin_dvfs
		/ SCHED_CAPACITY_SCALE;
	new_capacity += scr->dl;

#ifndef CONFIG_CPU_FREQ_SCHED_ASSIST
	if (new_capacity == scr->total)
		return;
#endif

	scr->total = new_capacity;
	if (request || type == SCHE_IOWAIT)
		update_fdomain_capacity_request(cpu, type);
}

static inline void set_sched_freq(void)
{
	static_key_slow_inc(&__sched_freq);
}

static inline void clear_sched_freq(void)
{
	static_key_slow_dec(&__sched_freq);
}

static int cpufreq_sched_policy_init(struct cpufreq_policy *policy)
{

#ifdef CONFIG_CPU_FREQ_SCHED_ASSIST
	return 0;
#else
	struct gov_data *gd_ptr;
	int cpu;
	int cid = arch_get_cluster_id(policy->cpu);

	/* if kthread is created, return */
	if (g_inited[cid]) {
		policy->governor_data = g_gd[cid];

		/* [MUST] backup policy, because it changed */
		g_gd[cid]->policy = policy;

		for_each_cpu(cpu, policy->cpus)
			memset(&per_cpu(cpu_sched_capacity_reqs, cpu), 0,
				sizeof(struct sched_capacity_reqs));

		set_sched_freq();

		return 0;
	}

	/* keep goverdata as gloabl variable */
	gd_ptr = g_gd[cid];

	/* [MUST] backup policy in first time */
	gd_ptr->policy = policy;

	/* [MUST] backup target_cpu */
	gd_ptr->target_cpu = policy->cpu;

	policy->governor_data = gd_ptr;

	for_each_cpu(cpu, policy->cpus)
		memset(&per_cpu(cpu_sched_capacity_reqs, cpu), 0,
		       sizeof(struct sched_capacity_reqs));

	pr_debug("%s: throttle threshold = %u [ns]\n",
		  __func__, gd_ptr->throttle_nsec);

	if (cpufreq_driver_is_slow()) {
		int ret;
		struct sched_param param;

		cpufreq_driver_slow = true;
		gd_ptr->task = kthread_create(cpufreq_sched_thread, policy,
					  "kschedfreq:%d",
					  cpumask_first(policy->related_cpus));
		if (IS_ERR_OR_NULL(gd_ptr->task)) {
			pr_err("%s: failed to create kschedfreq thread\n",
			       __func__);
			goto err;
		}

		param.sched_priority = 50;
		ret = sched_setscheduler_nocheck(gd_ptr->task, SCHED_FIFO, &param);
		if (ret) {
			pr_warn("%s: failed to set SCHED_FIFO\n", __func__);
			goto err;
		} else {
			pr_debug("%s: kthread (%d) set to SCHED_FIFO\n",
					__func__, gd_ptr->task->pid);
		}

		/* Task never die???? */
		get_task_struct(gd_ptr->task);
		kthread_bind_mask(gd_ptr->task, policy->related_cpus);
		wake_up_process(gd_ptr->task);
		init_irq_work(&gd_ptr->irq_work, cpufreq_sched_irq_work);
	}

	/* To confirm kthread is created. */
	g_inited[cid] = true;

	set_sched_freq();

	return 0;

err:
	kfree(gd_ptr);
	WARN_ON(1);
	return -ENOMEM;
#endif
}

static int cpufreq_sched_policy_exit(struct cpufreq_policy *policy)
{
	/* struct gov_data *gd = policy->governor_data; */

#ifdef CONFIG_CPU_FREQ_SCHED_ASSIST
	return 0;
#else
	clear_sched_freq();

	policy->governor_data = NULL;

	/* kfree(gd); */
	return 0;
#endif
}

static int cpufreq_sched_start(struct cpufreq_policy *policy)
{
#ifdef CONFIG_CPU_FREQ_SCHED_ASSIST
	return 0;
#else
	int cpu;

	for_each_cpu(cpu, policy->cpus)
		per_cpu(enabled, cpu) = 1;

	return 0;
#endif
}

static int cpufreq_sched_stop(struct cpufreq_policy *policy)
{
#ifdef CONFIG_CPU_FREQ_SCHED_ASSIST
	return 0;
#else
	struct gov_data *gd = policy->governor_data;
	int cpu;

	irq_work_sync(&gd->irq_work);

	for_each_cpu(cpu, policy->cpus)
		per_cpu(enabled, cpu) = 0;

	return 0;
#endif
}

static int cpufreq_sched_setup(struct cpufreq_policy *policy,
			       unsigned int event)
{
#if DEBUG
	int cpu = -1;
	int target_cpu = -1;
	char str[256] = {0};
	int len = 0;

	for_each_cpu(cpu, policy->cpus) {
		if (target_cpu == -1)
			target_cpu = cpu;
		len += snprintf(str+len, 256, ",%d", cpu);
	}
#endif

	switch (event) {
	case CPUFREQ_GOV_POLICY_INIT:
		printk_dbg("%s cpu=%d (%s) init\n", __func__, target_cpu, str);
		return cpufreq_sched_policy_init(policy);
	case CPUFREQ_GOV_POLICY_EXIT:
		printk_dbg("%s cpu=%d exit\n", __func__, target_cpu);
		return cpufreq_sched_policy_exit(policy);
	case CPUFREQ_GOV_START:
		printk_dbg("%s cpu=%d (%s) start\n", __func__, target_cpu, str);
		return cpufreq_sched_start(policy);
	case CPUFREQ_GOV_STOP:
		printk_dbg("%s cpu=%d (%s) stop\n", __func__, target_cpu, str);
		return cpufreq_sched_stop(policy);
	case CPUFREQ_GOV_LIMITS:
		break;
	}

	return 0;
}


static struct notifier_block cpu_hotplug;

static int cpu_hotplug_handler(struct notifier_block *nb,
		unsigned long val, void *data)
{
	int cpu = (unsigned long)data;

	switch (val) {
	case CPU_ONLINE:
		printk_dbg("%s cpu=%d online\n", __func__, cpu);
		break;
	case CPU_ONLINE_FROZEN:
		break;
	case CPU_UP_PREPARE:
#ifdef CONFIG_CPU_FREQ_SCHED_ASSIST
		per_cpu(enabled, cpu) = 1;
#endif
		printk_dbg("%s cpu=%d up_prepare\n", __func__, cpu);
		break;
	case CPU_DOWN_PREPARE:
		per_cpu(enabled, cpu) = 0;

		printk_dbg("%s cpu=%d down_prepare\n", __func__, cpu);
		break;
	}
	return NOTIFY_OK;
}


#ifndef CONFIG_CPU_FREQ_DEFAULT_GOV_SCHED
static
#endif
struct cpufreq_governor cpufreq_gov_sched = {
	.name			= "sched",
	.governor		= cpufreq_sched_setup,
	.owner			= THIS_MODULE,
};

static int __init cpufreq_sched_init(void)
{
	int cpu;
	int i;

	for_each_cpu(cpu, cpu_possible_mask) {
		struct sugov_cpu *sg_cpu = &per_cpu(sugov_cpu, cpu);
		int cid = arch_get_cluster_id(cpu);

		memset(&per_cpu(cpu_sched_capacity_reqs, cpu), 0,
				sizeof(struct sched_capacity_reqs));

		sg_cpu->util = 0;
		sg_cpu->max = 0;
		sg_cpu->flags = 0;
		sg_cpu->last_update = 0;
		sg_cpu->cached_raw_freq = 0;
		sg_cpu->iowait_boost = 0;
		sg_cpu->iowait_boost_max = mt_cpufreq_get_freq_by_idx(cid, 0);
		sg_cpu->iowait_boost_max >>= 1; /* limit max to half */
	}

	for (i = 0; i < MAX_CLUSTER_NR; i++) {
		g_gd[i] = kzalloc(sizeof(struct gov_data), GFP_KERNEL);
		if (!g_gd[i]) {
			WARN_ON(1);
			return -ENOMEM;
		}
		g_gd[i]->up_throttle_nsec = THROTTLE_UP_NSEC;
		g_gd[i]->down_throttle_nsec = THROTTLE_DOWN_NSEC;
		g_gd[i]->throttle_nsec = THROTTLE_NSEC;
		g_gd[i]->last_freq_update_time = 0;
		/* keep cid needed */
		g_gd[i]->cid = i;
	}

#ifdef CONFIG_CPU_FREQ_SCHED_ASSIST
	for_each_cpu(cpu, cpu_possible_mask)
		per_cpu(enabled, cpu) = 1;

#else
	for_each_cpu(cpu, cpu_possible_mask)
		per_cpu(enabled, cpu) = 0;
#endif

	cpu_hotplug.notifier_call = cpu_hotplug_handler;
	register_hotcpu_notifier(&cpu_hotplug);

	return cpufreq_register_governor(&cpufreq_gov_sched);
}

#ifdef CONFIG_CPU_FREQ_SCHED_ASSIST
static int cpufreq_callback(struct notifier_block *nb,
		unsigned long val, void *data)
{
	struct cpufreq_freqs *freq = data;
	int cpu = freq->cpu;
	struct cpumask cls_cpus;
	int id;
	int cid = arch_get_cluster_id(cpu);
	ktime_t throttle = g_gd[cid]->throttle;
	bool sched_dvfs;

	if (freq->flags & CPUFREQ_CONST_LOOPS)
		return NOTIFY_OK;

	sched_dvfs = mt_cpufreq_get_sched_enable();

	if (val == CPUFREQ_PRECHANGE) {
		/* consider DVFS has been changed by PPM or other governors */
		if (!sched_dvfs ||
		    !ktime_before(ktime_get(), ktime_add_ns(throttle, (20000000 - THROTTLE_NSEC)/*20ms*/))) {
			arch_get_cluster_cpus(&cls_cpus, cid);
			for_each_cpu(id, &cls_cpus)
				arch_scale_set_curr_freq(id, freq->new);
		}
	}

	return NOTIFY_OK;
}


static struct notifier_block cpufreq_notifier = {
	.notifier_call = cpufreq_callback,
};

static int __init register_cpufreq_notifier(void)
{
	return cpufreq_register_notifier(&cpufreq_notifier,
			CPUFREQ_TRANSITION_NOTIFIER);
}

core_initcall(register_cpufreq_notifier);
/* sched-assist dvfs is NOT a governor. */
late_initcall(cpufreq_sched_init);
#else
/* Try to make this the default governor */
fs_initcall(cpufreq_sched_init);
#endif
