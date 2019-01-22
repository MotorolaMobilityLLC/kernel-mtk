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

#include "sched_power.h"

/*
 * V1.04 support hybrid(EAS+HMP)
 * V1.05 if system is balanced, select CPU with max sparse capacity when wakeup
 * V1.06 add "turning" and "watershed" points to help HPS
 */

/* In default we choose energy_aware scheduling */
static SCHED_LB_TYPE sched_type = SCHED_EAS_LB;

/* watersched is updated by unified power table. */
static struct power_tuning_t power_tuning = {DEFAULT_TURNINGPOINT, DEFAULT_WATERSHED};

struct eas_data {
	struct attribute_group *attr_group;
	struct kobject *kobj;
	int init;
};

struct eas_data eas_info;

static int ver_major = 1;
static int ver_minor = 6;
static const char module_name[32] = "arctic";


#define VOLT_SCALE 10

struct power_tuning_t *get_eas_power_setting(void)
{
	return &power_tuning;
}


bool is_eas_enabled(void)
{
	return (sched_type) ? true : false;
}

bool is_hybrid_enabled(void)
{
	return (sched_type == SCHED_HYBRID_LB) ? true : false;
}

/* cluster 0 & 1 is buck shared. */
bool is_share_buck(int cid, int *co_buck_cid)
{
	bool ret = false;

	switch (cid) {
	case 0:
		*co_buck_cid = 1;
		ret = true;
		break;
	case 1:
		*co_buck_cid = 0;
		ret = true;
		break;
	case 2:
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

	if (cpu == -1) /* maybe no online CPU */
		return -1;

	sd = rcu_dereference_check_sched_domain(cpu_rq(cpu)->sd);
	if (sd) {
		sg = sd->groups;
		sge = sg->sge;
	} else
		return -1;

	for (idx = 0; idx < sge->nr_cap_states; idx++) {
		if (sge->cap_states[idx].cap >= util) {
			sel_idx = idx;
			break;
		}
	}

#if 1
	mt_sched_printf(sched_eas_energy_calc,
			"%s: cid=%d max_cpu=%d (util=%ld) max_idx=%d (cap=%ld)",
			__func__, cid, cpu, util, sel_idx, sge->cap_states[sel_idx].cap);
#endif

	return sel_idx;
}

inline
const int mtk_idle_power(int idle_state, int cpu, void *argu, int sd_level)
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
				"[share buck] %s cap_idx=%d is max(cid[%d]=%d,cid[%d]=%d)",
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

	switch (idle_state) {
	case 0: /* active idle: WFI */
		if (only_lv1) {
			/* idle: core->leask_power + cluster->leak_power */
			energy_cost = sge_core->cap_states[cap_idx].leak_power +
			sge_clus->cap_states[cap_idx].leak_power;

			mt_sched_printf(sched_eas_energy_calc,
				"%s: %s lv=%d tlb_cpu[%d].leak=%ld tlb_clu[%d].leak=%ld total=%d",
				__func__, "WFI", sd_level,
				cap_idx, sge_core->cap_states[cap_idx].leak_power,
				cap_idx, sge_clus->cap_states[cap_idx].leak_power,
				energy_cost);
		} else {
			energy_cost = _sge->cap_states[cap_idx].leak_power;

			mt_sched_printf(sched_eas_energy_calc,
				"%s: %s lv=%d tlb[%d].leak=%ld total=%d",
				__func__, "WFI", sd_level,
				cap_idx, _sge->cap_states[cap_idx].leak_power,
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
const int mtk_busy_power(int cpu, void *argu, int sd_level)
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

			cpu_dyn_pwr = sge_core->cap_states[cap_idx].power;
			clu_dyn_pwr = sge_clus->cap_states[cap_idx].power;

			energy_cost = ((cpu_dyn_pwr+clu_dyn_pwr)*volt_factor) >> VOLT_SCALE;

			/* + leak power of co_buck_cid's opp */
			cpu_leak_pwr = sge_core->cap_states[co_cap_idx].leak_power;
			clu_leak_pwr = sge_clus->cap_states[co_cap_idx].leak_power;
			energy_cost += (cpu_leak_pwr + clu_leak_pwr);

			mt_sched_printf(sched_eas_energy_calc,
					"%s: %s lv=%d tlb[%d].pwr=(cpu:%ld,clu:%ld) tlb[%d].leak=(cpu:%ld,clu:%ld) vlt_f=%ld total=%d",
					__func__, "share_buck/only1CPU", sd_level,
					cap_idx,
					sge_core->cap_states[cap_idx].power,
					sge_clus->cap_states[cap_idx].power,
					co_cap_idx,
					sge_core->cap_states[co_cap_idx].leak_power,
					sge_clus->cap_states[co_cap_idx].leak_power,
					volt_factor, energy_cost);
		} else {
			energy_cost = sge_core->cap_states[cap_idx].power +
				sge_core->cap_states[cap_idx].leak_power;

			energy_cost += sge_clus->cap_states[cap_idx].power +
				sge_clus->cap_states[cap_idx].leak_power;

			mt_sched_printf(sched_eas_energy_calc,
					"%s: %s lv=%d tlb_core[%d].pwr=(%ld,%ld) tlb_clu[%d]=(%ld,%ld) total=%d",
					__func__, "only1CPU", sd_level,
					cap_idx,
					sge_core->cap_states[cap_idx].power,
					sge_core->cap_states[cap_idx].leak_power,
					cap_idx,
					sge_clus->cap_states[cap_idx].power,
					sge_clus->cap_states[cap_idx].leak_power,
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

			dyn_pwr = (_sge->cap_states[cap_idx].power*volt_factor) >> VOLT_SCALE;
			leak_pwr = _sge->cap_states[co_cap_idx].leak_power;
			energy_cost = dyn_pwr + leak_pwr;

			mt_sched_printf(sched_eas_energy_calc,
					"%s: %s lv=%d  tlb[%d].pwr=%ld volt_f=%ld buck.pwr=%ld tlb[%d].leak=(%ld) total=%d",
					__func__, "share_buck", sd_level,
					cap_idx,
					_sge->cap_states[cap_idx].power,
					volt_factor, dyn_pwr,
					co_cap_idx,
					_sge->cap_states[co_cap_idx].leak_power,
					energy_cost);

		} else {
			/* No share buck impact */
			unsigned long dyn_pwr = _sge->cap_states[cap_idx].power;
			unsigned long leak_pwr = _sge->cap_states[cap_idx].leak_power;

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

	for_each_possible_cpu(cpu)
		len += snprintf(buf+len, buf_size-len, "cpu=%d orig_capacity=%lu capacity=%lu\n",
				cpu, cpu_rq(cpu)->cpu_capacity_orig,
				cpu_online(cpu)?cpu_rq(cpu)->cpu_capacity:0);

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
		if (val < SCHED_UNKNOWN_LB)
			sched_type = val;
	}

	return count;
}

static struct kobj_attribute eas_knob_attr =
__ATTR(enable, S_IWUSR | S_IRUSR, show_eas_knob,
		store_eas_knob);

/* For read version for EAS modification */
static ssize_t show_eas_version_attr(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	unsigned int len = 0;
	unsigned int max_len = 4096;

	len += snprintf(buf, max_len, "version=%d.%d(%s)\n\n", ver_major, ver_minor, module_name);
	len += show_cpu_capacity(buf+len, max_len - len);

	len += snprintf(buf+len, max_len - len, "\nwatershed=%d\n", power_tuning.watershed);
	len += snprintf(buf+len, max_len - len, "turning_point=%d\n", power_tuning.turning_point);

	return len;
}

static struct kobj_attribute eas_version_attr =
__ATTR(version, S_IRUSR, show_eas_version_attr, NULL);

static struct attribute *eas_attrs[] = {
	&eas_version_attr.attr,
	&eas_knob_attr.attr,
	&eas_watershed_attr.attr,
	&eas_turning_point_attr.attr,
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

	return ret;
}
late_initcall(eas_stats_init);
