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

#include "sched.h"
#include "cpufreq_sched.h"

#ifdef CONFIG_CPU_FREQ_SCHED_ASSIST
#define THROTTLE_NSEC	2000000 /* 2ms default */
#else
#define THROTTLE_NSEC	3000000 /* 3ms default */
#endif

#ifdef CONFIG_CPU_FREQ_SCHED_ASSIST
struct static_key __read_mostly __sched_freq = STATIC_KEY_INIT_TRUE;
#else
struct static_key __read_mostly __sched_freq = STATIC_KEY_INIT_FALSE;
#endif

static bool __read_mostly cpufreq_driver_slow;

#ifndef CONFIG_CPU_FREQ_DEFAULT_GOV_SCHED
static struct cpufreq_governor cpufreq_gov_sched;
#endif

static DEFINE_PER_CPU(unsigned long, enabled);
DEFINE_PER_CPU(struct sched_capacity_reqs, cpu_sched_capacity_reqs);

#define MAX_CLUSTER_NR 3

static struct gov_data *g_gd[MAX_CLUSTER_NR] = { NULL };

#ifndef CONFIG_CPU_FREQ_SCHED_ASSIST
static bool g_inited[MAX_CLUSTER_NR] = {false};
#else
static DEFINE_PER_CPU(unsigned long, freq_scale) = SCHED_CAPACITY_SCALE;
#endif

#define DEBUG 0
#define DEBUG_KLOG 0

#if DEBUG_KLOG
#define printk_dbg(f, a...) printk(dev "[scheddvfs] "f, ##a)
#else
#define printk_dbg(f, a...) do {} while (0)
#endif

#include <mt-plat/met_drv.h>


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
 * @throttle: next throttling period expiry. Derived from throttle_nsec
 * @throttle_nsec: throttle period length in nanoseconds
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
	unsigned int throttle_nsec;
	struct task_struct *task;
	struct irq_work irq_work;
	unsigned int requested_freq;
	struct cpufreq_policy *policy;
	int target_cpu;
	int cid;
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
#ifdef CONFIG_CPU_FREQ_SCHED_ASSIST
	unsigned long scale;
	struct cpumask cls_cpus;
	int cpu = target_cpu;
	unsigned int max, min;
#endif
	unsigned int boost_min;

	cid = arch_get_cluster_id(target_cpu);

	if (cid >= MAX_CLUSTER_NR || cid < 0) {
		WARN_ON(1);
		return;
	}

	/* policy is NOT trusted!!! here??? */
	gd = g_gd[cid];

#ifdef CONFIG_CPU_FREQ_SCHED_ASSIST
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
		per_cpu(freq_scale, cpu) = scale;
		arch_scale_set_curr_freq(cpu, freq); /* per_cpu(cpu_freq_capacity) */

#ifdef CONFIG_SCHED_WALT
		walt_cpufreq_notifier_trans(cpu, freq);
#endif
	}

	mt_cpufreq_set_by_schedule_load_cluster(cid, freq);

	met_tag_oneshot(0, met_dvfs_info[cid], freq);

	gd->throttle = ktime_add_ns(ktime_get(), gd->throttle_nsec);
#else
	policy = gd->policy;

	if (IS_ERR_OR_NULL(policy))
		return;

	if (policy->governor != &cpufreq_gov_sched ||
		!policy->governor_data)
		return;

	/* avoid race with cpufreq_sched_stop */
	if (!down_write_trylock(&policy->rwsem))
		return;

	printk_dbg("%s: cid=%d cpu=%d max_freq=%u +\n", __func__, cid, policy->cpu, policy->max);
	__cpufreq_driver_target(policy, freq, CPUFREQ_RELATION_L);
	printk_dbg("%s: cid=%d cpu=%d max_freq=%u -\n", __func__, cid, policy->cpu, policy->max);

	gd->throttle = ktime_add_ns(ktime_get(), gd->throttle_nsec);
	up_write(&policy->rwsem);
#endif
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
	unsigned int new_request = 0;
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

		cpufreq_sched_try_driver_target(cpu, policy, new_request, SCHE_INVALID);
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
#ifdef CONFIG_CPU_FREQ_SCHED_ASSIST
	int cid = arch_get_cluster_id(cpu);
	struct cpumask cls_cpus;
#endif
	struct cpufreq_policy *policy = NULL;

	/*
	 * Avoid grabbing the policy if possible. A test is still
	 * required after locking the CPU's policy to avoid racing
	 * with the governor changing.
	 */
	if (!per_cpu(enabled, cpu))
		return;

#ifdef CONFIG_CPU_FREQ_SCHED_ASSIST
	gd = g_gd[cid];

	/* bail early if we are throttled */
	if (ktime_before(ktime_get(), gd->throttle))
		goto out;

	arch_get_cluster_cpus(&cls_cpus, cid);

	/* find max capacity requested by cpus in this policy */
	for_each_cpu(cpu_tmp, &cls_cpus) {
		struct sched_capacity_reqs *scr;

		if (!cpu_online(cpu_tmp))
			continue;

		scr = &per_cpu(cpu_sched_capacity_reqs, cpu_tmp);
		capacity = max(capacity, scr->total);
	}

	freq_new = capacity * arch_scale_get_max_freq(cpu) >> SCHED_CAPACITY_SHIFT;
#else
	if (likely(cpu_online(cpu)))
		policy = cpufreq_cpu_get(cpu);

	if (IS_ERR_OR_NULL(policy))
		return;

	if (policy->governor != &cpufreq_gov_sched ||
	    !policy->governor_data)
		goto out;

	gd = policy->governor_data;

	/* bail early if we are throttled */
	if (ktime_before(ktime_get(), gd->throttle))
		goto out;

	/* find max capacity requested by cpus in this policy */
	for_each_cpu(cpu_tmp, policy->cpus) {
		struct sched_capacity_reqs *scr;

		scr = &per_cpu(cpu_sched_capacity_reqs, cpu_tmp);
		capacity = max(capacity, scr->total);
	}

	/* Convert the new maximum capacity request into a cpu frequency */
	freq_new = capacity * policy->max >> SCHED_CAPACITY_SHIFT;

	if (freq_new == gd->requested_freq)
		goto out;

#endif /* !CONFIG_CPU_FREQ_SCHED_ASSIST */

	gd->requested_freq = freq_new;
	gd->target_cpu = cpu;

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
	if (request)
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
	int cid;

	cid = arch_get_cluster_id(policy->cpu);

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
		memset(&per_cpu(cpu_sched_capacity_reqs, cpu), 0,
				sizeof(struct sched_capacity_reqs));
	}

	for (i = 0; i < MAX_CLUSTER_NR; i++) {
		g_gd[i] = kzalloc(sizeof(struct gov_data), GFP_KERNEL);
		if (!g_gd[i]) {
			WARN_ON(1);
			return -ENOMEM;
		}
		g_gd[i]->throttle_nsec = THROTTLE_NSEC;

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
/* sched-assist dvfs is NOT a governor. */
late_initcall(cpufreq_sched_init);
#else
/* Try to make this the default governor */
fs_initcall(cpufreq_sched_init);
#endif
