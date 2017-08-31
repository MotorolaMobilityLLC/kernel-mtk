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

#include <linux/sched.h>
#include <linux/math64.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <trace/events/sched.h>
#include <linux/topology.h>
#include <linux/cpufreq.h>

#include "sched_power.h"

#include <mt-plat/met_drv.h>

/*
 * V1.04 support hybrid(EAS+HMP)
 * V1.05 if system is balanced, select CPU with max sparse capacity when wakeup
 * V1.06 add "turning" and "watershed" points to help HPS
 */

/* default scheduling */
static SCHED_LB_TYPE sched_type = SCHED_HYBRID_LB;

/* watersched is updated by unified power table. */
static struct power_tuning_t power_tuning = {DEFAULT_TURNINGPOINT, DEFAULT_WATERSHED};

struct eas_data {
	struct attribute_group *attr_group;
	struct kobject *kobj;
	int init;
};

struct eas_data eas_info;

static int ver_major = 1;
static int ver_minor = 8;

#ifdef CONFIG_CPU_FREQ_SCHED_ASSIST
static const char module_name[64] = "arctic, sched-assist dvfs,hps";
#else
static const char module_name[64] = "arctic";
#endif

/* To define limit of SODI */
int sodi_limit = DEFAULT_SODI_LIMIT;

/* A lock for scheduling switcher */
DEFINE_SPINLOCK(sched_switch_lock);

#define VOLT_SCALE 10

int sched_scheduler_switch(SCHED_LB_TYPE new_sched)
{
	unsigned long flags;

	if (sched_type >= SCHED_UNKNOWN_LB)
		return -1;

	spin_lock_irqsave(&sched_switch_lock, flags);
	sched_type = new_sched;
	spin_unlock_irqrestore(&sched_switch_lock, flags);

	return 0;
}
EXPORT_SYMBOL(sched_scheduler_switch);

struct power_tuning_t *get_eas_power_setting(void)
{
	return &power_tuning;
}

bool is_game_mode;

void game_hint_notifier(int is_game)
{
	if (is_game) {
		capacity_margin_dvfs = 1024;
		sodi_limit = 120;
		is_game_mode = true;
	} else {
		capacity_margin_dvfs = DEFAULT_CAP_MARGIN_DVFS;
		sodi_limit = DEFAULT_SODI_LIMIT;
		is_game_mode = false;
	}
}

bool is_eas_enabled(void)
{
	return (sched_type) ? true : false;
}

bool is_hybrid_enabled(void)
{
	return (sched_type == SCHED_HYBRID_LB) ? true : false;
}

/* MT6799: cluster 0 & 2 is buck shared. */
bool is_share_buck(int cid, int *co_buck_cid)
{
	bool ret = false;

	switch (cid) {
	case 0:
		*co_buck_cid = 2;
		ret = true;
		break;
	case 2:
		*co_buck_cid = 0;
		ret = true;
		break;
	case 1:
		ret = false;
		break;
	default:
		WARN_ON(1);
	}


	return ret;
}

static unsigned long mtk_cluster_max_usage(int cid, struct energy_env *eenv, int *max_cpu)
{
	unsigned long max_usage = 0;
	int cpu = -1;
	struct cpumask cls_cpus;
	int delta;

	*max_cpu = -1;

	arch_get_cluster_cpus(&cls_cpus, cid);

	for_each_cpu(cpu, &cls_cpus) {
		int cpu_usage = 0;

		if (likely(!cpu_online(cpu)))
			continue;

		delta = calc_util_delta(eenv, cpu);
		cpu_usage = __cpu_util(cpu, delta);

		if (cpu_usage >= max_usage) {
			max_usage = cpu_usage;
			*max_cpu = cpu;
		}
	}

	return max_usage;
}

int mtk_cluster_capacity_idx(int cid, struct energy_env *eenv)
{
	int idx;
	int cpu;
	unsigned long util = mtk_cluster_max_usage(cid, eenv, &cpu);
	const struct sched_group_energy *sge;
	struct sched_group *sg;
	struct sched_domain *sd;
	int sel_idx = -1; /* final selected index */
	unsigned long new_capacity = util;

	if (cpu == -1) /* maybe no online CPU */
		return -1;

	sd = rcu_dereference_check_sched_domain(cpu_rq(cpu)->sd);
	if (sd) {
		sg = sd->groups;
		sge = sg->sge;
	} else
		return -1;

#ifdef CONFIG_CPU_FREQ_GOV_SCHED
	/* OPP idx to refer capacity margin */
	new_capacity = util * capacity_margin_dvfs >> SCHED_CAPACITY_SHIFT;
#endif

	for (idx = 0; idx < sge->nr_cap_states; idx++) {
		if (sge->cap_states[idx].cap >= new_capacity) {
			sel_idx = idx;
			break;
		}
	}

#if 1
	mt_sched_printf(sched_eas_energy_calc,
			"cid=%d max_cpu=%d (util=%ld new=%ld) opp_idx=%d (cap=%lld)",
				cid, cpu, util, new_capacity, sel_idx, sge->cap_states[sel_idx].cap);
#endif

	return sel_idx;
}

inline
int mtk_idle_power(int idle_state, int cpu, void *argu, int sd_level)
{
	int energy_cost = 0;
	struct sched_domain *sd;
	const struct sched_group_energy *sge, *_sge, *sge_core, *sge_clus;
#ifdef CONFIG_ARM64
	int cid = cpu_topology[cpu].cluster_id;
#else
	int cid = cpu_topology[cpu].socket_id;
#endif
	struct energy_env *eenv = (struct energy_env *)argu;
	int only_lv1 = 0;
	int cap_idx = eenv->opp_idx[cid];
	int co_buck_cid = -1;

	sd = rcu_dereference_check_sched_domain(cpu_rq(cpu)->sd);

	/* [FIXME] racing with hotplug */
	if (!sd)
		return 0;

	/* [FIXME] racing with hotplug */
	if (cap_idx == -1)
		return 0;

	if (is_share_buck(cid, &co_buck_cid)) {
		cap_idx = max(eenv->opp_idx[cid], eenv->opp_idx[co_buck_cid]);

		mt_sched_printf(sched_eas_energy_calc,
				"[share buck] %s cap_idx=%d is via max_opp(cid%d=%d,cid%d=%d)",
				__func__,
				cap_idx, cid, eenv->opp_idx[cid],
				co_buck_cid, eenv->opp_idx[co_buck_cid]);
	}

	sge = sd->groups->sge;

	sge = _sge = sge_core = sge_clus = NULL;

	/* To handle only 1 CPU in cluster by HPS */
	if (unlikely(!sd->child && (rcu_dereference(per_cpu(sd_scs, cpu)) == NULL))) {
		sge_core = cpu_core_energy(cpu);
		sge_clus = cpu_cluster_energy(cpu);

		only_lv1 = 1;
	} else {
		if (sd_level == 0)
			_sge = cpu_core_energy(cpu); /* for cpu */
		else
			_sge = cpu_cluster_energy(cpu); /* for cluster */
	}

	/* [DEBUG ] wfi always??? */
	idle_state = 0;

	switch (idle_state) {
	case 0: /* active idle: WFI */
		if (only_lv1) {
			/* idle: core->leask_power + cluster->lkg_pwr */
			energy_cost = sge_core->cap_states[cap_idx].lkg_pwr[sge_core->lkg_idx] +
			sge_clus->cap_states[cap_idx].lkg_pwr[sge_clus->lkg_idx];

			mt_sched_printf(sched_eas_energy_calc,
				"%s: %s lv=%d tlb_cpu[%d].leak=%d tlb_clu[%d].leak=%d total=%d",
				__func__, "WFI", sd_level,
				cap_idx, sge_core->cap_states[cap_idx].lkg_pwr[sge_core->lkg_idx],
				cap_idx, sge_clus->cap_states[cap_idx].lkg_pwr[sge_clus->lkg_idx],
				energy_cost);
		} else {
			energy_cost = _sge->cap_states[cap_idx].lkg_pwr[_sge->lkg_idx];

			mt_sched_printf(sched_eas_energy_calc,
				"%s: %s lv=%d tlb[%d].leak=%d total=%d",
				__func__, "WFI", sd_level,
				cap_idx, _sge->cap_states[cap_idx].lkg_pwr[_sge->lkg_idx],
				energy_cost);
		}

	break;
	case 6: /* SPARK: */
		if (only_lv1) {
			/* idle: core->idle_power[spark] + cluster->idle_power[spark] */
			energy_cost = sge_core->idle_states[idle_state].power +
				sge_clus->idle_states[idle_state].power;

			mt_sched_printf(sched_eas_energy_calc,
				"%s: %s lv=%d idle_cpu[%d].power=%ld idle_clu[%d].power=(%ld) total=%d",
				__func__, "SPARK", sd_level,
				idle_state, sge_core->idle_states[idle_state].power,
				idle_state, sge_clus->idle_states[idle_state].power,
				energy_cost);
		} else {
			energy_cost = _sge->idle_states[idle_state].power;
			mt_sched_printf(sched_eas_energy_calc,
				"%s: %s lv=%d idle[%d].power=(%ld) total=%d",
				__func__, "SPARK", sd_level,
				idle_state, _sge->idle_states[idle_state].power,
				energy_cost);
		}

	break;
	case 4: /* MCDI: */
		energy_cost = 0;

		mt_sched_printf(sched_eas_energy_calc,
			"%s: %s idle:%d total=%d", __func__, "MCDI",
			idle_state, energy_cost);
	break;
	default: /* SODI, deep_idle: */
		energy_cost = 0;

		mt_sched_printf(sched_eas_energy_calc, "%s: unknown idle state=%d\n", __func__,  idle_state);
	break;
	}

	return energy_cost;
}

inline
int mtk_busy_power(int cpu, void *argu, int sd_level)
{
	struct energy_env *eenv = (struct energy_env *)argu;
	struct sched_domain *sd = rcu_dereference_check_sched_domain(cpu_rq(cpu)->sd);
	const struct sched_group_energy *sge;
	int energy_cost = 0;
#ifdef CONFIG_ARM64
	int cid = cpu_topology[cpu].cluster_id;
#else
	int cid = cpu_topology[cpu].socket_id;
#endif
	int cap_idx = eenv->opp_idx[cid];
	int co_buck_cid = -1;
	unsigned long int volt_factor = 1;
	int shared = 0;

	/* [FIXME] racing with hotplug */
	if (!sd)
		return 0;

	/* [FIXME] racing with hotplug */
	if (cap_idx == -1)
		return 0;

	sge = sd->groups->sge;

	if (is_share_buck(cid, &co_buck_cid)) {

		mt_sched_printf(sched_eas_energy_calc,
				"[share buck] cap_idx of clu%d=%d cap_idx of clu%d=%d",
				cid, eenv->opp_idx[cid], co_buck_cid, eenv->opp_idx[co_buck_cid]);
		shared = 1;
	}

	if (unlikely(!sd->child && (rcu_dereference(per_cpu(sd_scs, cpu)) == NULL))) {
		/* fix HPS defeats: only one CPU in this cluster */
		const struct sched_group_energy *sge_core = cpu_core_energy(cpu);
		const struct sched_group_energy *sge_clus = cpu_cluster_energy(cpu);
		int co_cap_idx = eenv->opp_idx[co_buck_cid];

		if (shared && (co_cap_idx > cap_idx)) {
			unsigned long v_max = sge_core->cap_states[co_cap_idx].volt;
			unsigned long v_min = sge_core->cap_states[cap_idx].volt;
			unsigned long clu_dyn_pwr, cpu_dyn_pwr;
			unsigned long clu_leak_pwr, cpu_leak_pwr;
			/*
			 * dynamic power = F*V^2
			 *
			 * dyn_pwr  = current_power * (v_max/v_min)^2
			 * leak_pwr = tlb[idx of v_max].leak;
			 *
			 */
			volt_factor = ((v_max*v_max) << VOLT_SCALE) / (v_min*v_min);

			cpu_dyn_pwr = sge_core->cap_states[cap_idx].dyn_pwr;
			clu_dyn_pwr = sge_clus->cap_states[cap_idx].dyn_pwr;

			energy_cost = ((cpu_dyn_pwr+clu_dyn_pwr)*volt_factor) >> VOLT_SCALE;

			/* + leak power of co_buck_cid's opp */
			cpu_leak_pwr = sge_core->cap_states[co_cap_idx].lkg_pwr[sge_core->lkg_idx];
			clu_leak_pwr = sge_clus->cap_states[co_cap_idx].lkg_pwr[sge_clus->lkg_idx];
			energy_cost += (cpu_leak_pwr + clu_leak_pwr);

			mt_sched_printf(sched_eas_energy_calc,
					"%s: %s lv=%d tlb[%d].dyn_pwr=(cpu:%d,clu:%d) tlb[%d].leak=(cpu:%d,clu:%d) vlt_f=%ld",
					__func__, "share_buck/only1CPU", sd_level,
					cap_idx,
					sge_core->cap_states[cap_idx].dyn_pwr,
					sge_clus->cap_states[cap_idx].dyn_pwr,
					co_cap_idx,
					sge_core->cap_states[co_cap_idx].lkg_pwr[sge_core->lkg_idx],
					sge_clus->cap_states[co_cap_idx].lkg_pwr[sge_clus->lkg_idx],
					volt_factor);
			mt_sched_printf(sched_eas_energy_calc, "%s: %s total=%d",
						__func__, "share_buck/only1CPU", energy_cost);
		} else {
			energy_cost = sge_core->cap_states[cap_idx].dyn_pwr+
				sge_core->cap_states[cap_idx].lkg_pwr[sge_core->lkg_idx];

			energy_cost += sge_clus->cap_states[cap_idx].dyn_pwr+
				sge_clus->cap_states[cap_idx].lkg_pwr[sge_clus->lkg_idx];

			mt_sched_printf(sched_eas_energy_calc,
					"%s: %s lv=%d tlb_core[%d].dyn_pwr=(%d,%d) tlb_clu[%d]=(%d,%d) total=%d",
					__func__, "only1CPU", sd_level,
					cap_idx,
					sge_core->cap_states[cap_idx].dyn_pwr,
					sge_core->cap_states[cap_idx].lkg_pwr[sge_core->lkg_idx],
					cap_idx,
					sge_clus->cap_states[cap_idx].dyn_pwr,
					sge_clus->cap_states[cap_idx].lkg_pwr[sge_clus->lkg_idx],
					energy_cost);
		}
	} else {
		const struct sched_group_energy *_sge;
		int co_cap_idx = eenv->opp_idx[co_buck_cid];

		if (sd_level == 0)
			_sge = cpu_core_energy(cpu); /* for CPU */
		else
			_sge = cpu_cluster_energy(cpu); /* for cluster */

		if (shared && (co_cap_idx > cap_idx)) {
			/*
			 * calculated power with share-buck impact
			 *
			 * dynamic power = F*V^2
			 *
			 * dyn_pwr  = current_power * (v_max/v_min)^2
			 * leak_pwr = tlb[idx of v_max].leak;
			 */
			unsigned long v_max = _sge->cap_states[co_cap_idx].volt;
			unsigned long v_min = _sge->cap_states[cap_idx].volt;
			unsigned long dyn_pwr;
			unsigned long leak_pwr;

			volt_factor = ((v_max*v_max) << VOLT_SCALE) / (v_min*v_min);

			dyn_pwr = (_sge->cap_states[cap_idx].dyn_pwr*volt_factor) >> VOLT_SCALE;
			leak_pwr = _sge->cap_states[co_cap_idx].lkg_pwr[_sge->lkg_idx];
			energy_cost = dyn_pwr + leak_pwr;

			mt_sched_printf(sched_eas_energy_calc,
					"%s: %s lv=%d  tlb[%d].pwr=%d volt_f=%ld buck.pwr=%ld tlb[%d].leak=(%d) total=%d",
					__func__, "share_buck", sd_level,
					cap_idx,
					_sge->cap_states[cap_idx].dyn_pwr,
					volt_factor, dyn_pwr,
					co_cap_idx,
					_sge->cap_states[co_cap_idx].lkg_pwr[_sge->lkg_idx],
					energy_cost);

		} else {
			/* No share buck impact */
			unsigned long dyn_pwr = _sge->cap_states[cap_idx].dyn_pwr;
			unsigned long leak_pwr = _sge->cap_states[cap_idx].lkg_pwr[_sge->lkg_idx];

			energy_cost = dyn_pwr + leak_pwr;

			mt_sched_printf(sched_eas_energy_calc,
					"%s: lv=%d tlb[%d].pwr=%ld tlb[%d].leak=%ld total=%d",
					__func__, sd_level,
					cap_idx, dyn_pwr,
					cap_idx, leak_pwr,
					energy_cost);
		}
	}

	return energy_cost;
}
/* Try to get original capacity of all CPUs */
int show_cpu_capacity(char *buf, int buf_size)
{
	int cpu;
	int len = 0;

	for_each_possible_cpu(cpu) {
		struct sched_capacity_reqs *scr;

		scr = &per_cpu(cpu_sched_capacity_reqs, cpu);
		len += snprintf(buf+len, buf_size-len, "cpu=%d orig_cap=%lu cap=%lu max_cap=%lu max=%lu min=%lu ",
				cpu,
				cpu_rq(cpu)->cpu_capacity_orig,
				cpu_online(cpu)?cpu_rq(cpu)->cpu_capacity:0,
				cpu_online(cpu)?cpu_rq(cpu)->rd->max_cpu_capacity.val:0,
				/* limited frequency */
				cpu_online(cpu)?arch_scale_get_max_freq(cpu) / 1000 : 0,
				cpu_online(cpu)?arch_scale_get_min_freq(cpu) / 1000 : 0
				);

		len += snprintf(buf+len, buf_size-len, "cur_freq=%luMHZ, cur=%lu util=%lu cfs=%lu rt=%lu (%s)\n",
				/* current frequency */
				cpu_online(cpu)?capacity_curr_of(cpu) *
				arch_scale_get_max_freq(cpu) /
				cpu_rq(cpu)->cpu_capacity_orig / 1000 : 0,

				/* current capacity */
				cpu_online(cpu)?capacity_curr_of(cpu):0,

				/* cpu utilization */
				cpu_online(cpu)?cpu_util(cpu):0,
				scr->cfs,
				scr->rt,
				cpu_online(cpu)?"on":"off"
				);
	}

	return len;
}

/* read/write for watershed */
static ssize_t store_eas_watershed(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned int val = 0;

	if (sscanf(buf, "%iu", &val) != 0)
		power_tuning.watershed = val;

	return count;
}
static struct kobj_attribute eas_watershed_attr =
__ATTR(watershed, S_IWUSR | S_IRUSR, NULL,
		store_eas_watershed);

/* read/write for turning_point */
static ssize_t store_eas_turning_point(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned int val = 0;

	if (sscanf(buf, "%iu", &val) != 0 && (val < 100))
		power_tuning.turning_point = val;

	return count;
}
static struct kobj_attribute eas_turning_point_attr =
__ATTR(turning_point, S_IWUSR | S_IRUSR, NULL,
		store_eas_turning_point);

/* A knob for turn on/off energe-aware scheduling */
static ssize_t show_eas_knob(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	unsigned int len = 0;
	unsigned int max_len = 4096;

	switch (sched_type) {
	case SCHED_HMP_LB:
		len += snprintf(buf, max_len, "scheduler= HMP\n\n");
		break;
	case SCHED_EAS_LB:
		len += snprintf(buf, max_len, "scheduler= EAS\n\n");
		break;
	case SCHED_HYBRID_LB:
		len += snprintf(buf, max_len, "scheduler= hybrid\n\n");
		break;
	default:
		len += snprintf(buf, max_len, "scheduler= unknown\n\n");
		break;
	}

	return len;
}

static ssize_t store_eas_knob(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned int val = 0;

	/*
	 * 0: HMP
	 * 1: EAS
	 * 2: Hybrid
	 */
	if (sscanf(buf, "%iu", &val) != 0) {
		if (val < SCHED_UNKNOWN_LB) {
			unsigned long flags;

			spin_lock_irqsave(&sched_switch_lock, flags);
			sched_type = val;
			spin_unlock_irqrestore(&sched_switch_lock, flags);
		}
	}

	return count;
}

static struct kobj_attribute eas_knob_attr =
__ATTR(enable, S_IWUSR | S_IRUSR, show_eas_knob,
		store_eas_knob);

/* For read info for EAS */
static ssize_t show_eas_info_attr(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	unsigned int len = 0;
	unsigned int max_len = 4096;
	int max_cpu = -1, max_pid = 0, max_util = 0, boost;
	struct task_struct *task = NULL;

	len += snprintf(buf, max_len, "version=%d.%d(%s)\n\n", ver_major, ver_minor, module_name);
	len += show_cpu_capacity(buf+len, max_len - len);

	len += snprintf(buf+len, max_len - len, "max_cap_cpu=%d\n", cpu_rq(0)->rd->max_cpu_capacity.cpu);
	len += snprintf(buf+len, max_len - len, "cpufreq slow=%d\n", cpufreq_driver_is_slow());

	len += snprintf(buf+len, max_len - len, "\nwatershed=%d\n", power_tuning.watershed);
	len += snprintf(buf+len, max_len - len, "turning_point=%d\n", power_tuning.turning_point);

#ifdef CONFIG_MTK_SCHED_RQAVG_KS
	sched_max_util_task(&max_cpu, &max_pid, &max_util, &boost);
#endif

	task = find_task_by_vpid(max_pid);

	len += snprintf(buf+len, max_len - len, "\nheaviest task pid=%d (%s) util=%d boost=%d run in cpu%d\n\n",
				max_pid, (task)?task->comm:"NULL", max_util, boost, max_cpu);

	len += snprintf(buf+len, max_len - len, "foreground boost=%d\n", group_boost_read(1));

	return len;
}

/* To identify min capacity for stune to boost */
static ssize_t store_stune_task_thresh_knob(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	int val = 0;

	if (sscanf(buf, "%iu", &val) != 0) {
		if (val < 1024 || val >= 0)
			stune_task_threshold = val;
	}

	met_tag_oneshot(0, "sched_stune_filter", stune_task_threshold);

	return count;
}

static ssize_t show_stune_task_thresh_knob(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	unsigned int len = 0;
	unsigned int max_len = 4096;

	len += snprintf(buf, max_len, "stune_task_thresh=%d\n", stune_task_threshold);

	met_tag_oneshot(0, "sched_stune_filter", stune_task_threshold);

	return len;
}

static struct kobj_attribute eas_stune_task_thresh_attr =
__ATTR(stune_task_thresh, S_IWUSR | S_IRUSR, show_stune_task_thresh_knob,
		store_stune_task_thresh_knob);

static struct kobj_attribute eas_info_attr =
__ATTR(info, S_IRUSR, show_eas_info_attr, NULL);

/* To define capacity_margin */
static ssize_t store_cap_margin_knob(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	int val = 0;

	if (sscanf(buf, "%iu", &val) != 0)
		capacity_margin_dvfs = val;

	return count;
}

static ssize_t show_cap_margin_knob(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	unsigned int len = 0;
	unsigned int max_len = 4096;

	len += snprintf(buf, max_len, "capacity_margin_dvfs=%d\n", capacity_margin_dvfs);

	return len;
}

static struct kobj_attribute eas_cap_margin_dvfs_attr =
__ATTR(cap_margin_dvfs, S_IWUSR | S_IRUSR, show_cap_margin_knob,
		store_cap_margin_knob);

/* To set limit of SODI */
static ssize_t store_sodi_limit_knob(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	int val = 0;

	if (sscanf(buf, "%iu", &val) != 0)
		sodi_limit = val;

	return count;
}

static ssize_t show_sodi_limit_knob(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	unsigned int len = 0;
	unsigned int max_len = 4096;

	len += snprintf(buf, max_len, "sodi limit=%d\n", sodi_limit);

	return len;
}

static struct kobj_attribute eas_sodi_limit_attr =
__ATTR(sodi_limit, S_IWUSR | S_IRUSR, show_sodi_limit_knob,
		store_sodi_limit_knob);

#ifdef CONFIG_CPU_FREQ_SCHED_ASSIST
/* for scheddvfs debug */
static ssize_t store_dvfs_debug_knob(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	int val = 0;

	if (sscanf(buf, "%iu", &val) != 0)
		dbg_id = val;

	return count;
}

static ssize_t show_dvfs_debug_knob(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	unsigned int len = 0;
	unsigned int max_len = 4096;

	len += snprintf(buf, max_len, "dbg_id=%d\n", dbg_id);

	return len;
}

static struct kobj_attribute eas_dvfs_debug_attr =
__ATTR(dvfs_debug, S_IWUSR | S_IRUSR, show_dvfs_debug_knob,
		store_dvfs_debug_knob);
#endif

static struct attribute *eas_attrs[] = {
	&eas_info_attr.attr,
	&eas_knob_attr.attr,
	&eas_watershed_attr.attr,
	&eas_turning_point_attr.attr,
	&eas_stune_task_thresh_attr.attr,
	&eas_cap_margin_dvfs_attr.attr,
	&eas_sodi_limit_attr.attr,
#ifdef CONFIG_CPU_FREQ_SCHED_ASSIST
	&eas_dvfs_debug_attr.attr,
#endif
	NULL,
};

static struct attribute_group eas_attr_group = {
	.attrs = eas_attrs,
};

static int init_eas_attribs(void)
{
	int err;

	eas_info.attr_group = &eas_attr_group;

	/* Create /sys/devices/system/cpu/eas/... */
	eas_info.kobj = kobject_create_and_add("eas", &cpu_subsys.dev_root->kobj);
	if (!eas_info.kobj)
		return -ENOMEM;

	err = sysfs_create_group(eas_info.kobj, eas_info.attr_group);
	if (err)
		kobject_put(eas_info.kobj);
	else
		kobject_uevent(eas_info.kobj, KOBJ_ADD);

	return err;
}

static int __init eas_stats_init(void)
{
	int ret = 0;

	eas_info.init = 0;

	ret = init_eas_attribs();

	if (ret)
		eas_info.init = 1;

	ged_kpi_set_game_hint_value_fp = game_hint_notifier;

	return ret;
}
late_initcall(eas_stats_init);
