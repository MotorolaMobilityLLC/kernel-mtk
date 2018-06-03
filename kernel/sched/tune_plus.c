/*
 * Copyright (C) 2017 MediaTek Inc.
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

int sys_boosted;

int stune_task_threshold_for_perf_idx(bool filter)
{
	if (!default_stune_threshold)
		return -EINVAL;

	if (filter)
		stune_task_threshold = default_stune_threshold;
	else
		stune_task_threshold = 0;

#if MET_STUNE_DEBUG
	met_tag_oneshot(0, "sched_stune_filter", stune_task_threshold);
#endif
	return 0;
}

int capacity_min_write_for_perf_idx(int idx, int capacity_min)
{
	struct schedtune *ct = allocated_group[idx];

	if (!ct)
		return -EINVAL;

	if (capacity_min < 0 || capacity_min > 1024) {
		printk_deferred("warning: capacity_min should be 0~1024\n");
		if (capacity_min > 1024)
			capacity_min = 1024;
		else if (capacity_min < 0)
			capacity_min = 0;
	}

#if 0 /* ifdef CONFIG_CPU_FREQ_GOV_SCHEDPLUS */
	set_cap_min_freq(capacity_min);
#endif
	rcu_read_lock();
	ct->capacity_min = capacity_min;

	/* Update CPU capacity_min */
	schedtune_boostgroup_update_capacity_min(ct->idx, ct->capacity_min);
	rcu_read_unlock();

#if MET_STUNE_DEBUG
	/* top-app */
	if (ct->idx == 3)
		met_tag_oneshot(0, "sched_boost_top_capacity_min",
				ct->capacity_min);
#endif

	return 0;
}

int boost_write_for_perf_idx(int group_idx, int boost_value)
{
	struct schedtune *ct;
	unsigned int threshold_idx;
	int boost_pct;
	int idx = 0;
	int ctl_no = div64_s64(boost_value, 1000);
	int cluster;
	int cap_min = 0;
#if 0 /* ifdef CONFIG_CPU_FREQ_GOV_SCHEDPLUS */
	bool dvfs_on_demand = false;
	int floor = 0;
	int i;
	int c0, c1;
	int dvfs_floor;
#endif

	switch (ctl_no) {
	case 4:
		/* min cpu capacity request */
		boost_value -= ctl_no * 1000;

		if (boost_value < 0 || boost_value > 100) {
			printk_deferred("warning: boost for capacity_min should be 0~100\n");
			if (boost_value > 100)
				boost_value = 100;
			else if (boost_value < 0)
				boost_value = 0;
		}

		ct = allocated_group[group_idx];
		if (ct) {
			cap_min = div64_s64(boost_value * 1024, 100);

#if 0 /*#ifdef CONFIG_CPU_FREQ_GOV_SCHEDPLUS */
			set_cap_min_freq(cap_min);
#endif
			rcu_read_lock();
			ct->capacity_min = cap_min;
			/* Update CPU capacity_min */
			schedtune_boostgroup_update_capacity_min(
						ct->idx, ct->capacity_min);
			rcu_read_unlock();

#if MET_STUNE_DEBUG
			/* top-app */
			if (ct->idx == 3)
				met_tag_oneshot(
					0, "sched_boost_top_capacity_min",
					ct->capacity_min);
#endif
		}

		/* boost4xxx: no boost only capacity_min */
		boost_value = 0;

		stune_task_threshold = default_stune_threshold;
		break;
	case 3:
		/* a floor of cpu frequency */
		boost_value -= ctl_no * 1000;
		cluster = (int)boost_value / 100;
		boost_value = (int)boost_value % 100;

#if 0 /* ifdef CONFIG_CPU_FREQ_GOV_SCHEDPLUS */
		if (cluster > 0 && cluster <= 0x2) { /* only two cluster */
			floor = 1;
			c0 = cluster & 0x1;
			c1 = cluster & 0x2;

			/* cluster 0 */
			if (c0)
				set_min_boost_freq(boost_value, 0);
			else
				min_boost_freq[0] = 0;

			/* cluster 1 */
			if (c1)
				set_min_boost_freq(boost_value, 1);
			else
				min_boost_freq[1] = 0;
		}
#endif
		stune_task_threshold = default_stune_threshold;
		break;
	case 2:
		/* dvfs short cut */
		boost_value -= 2000;
		stune_task_threshold = default_stune_threshold;
#if 0 /* ifdef CONFIG_CPU_FREQ_GOV_SCHEDPLUS */
		dvfs_on_demand = true;
#endif
		break;
	case 1:
		/* boost all tasks */
		boost_value -= 1000;
		stune_task_threshold = 0;
		break;
	case 0:
		/* boost big task only */
		stune_task_threshold = default_stune_threshold;
		break;
	default:
		printk_deferred("warning: perf ctrl no should be 0~1\n");
		return -EINVAL;
	}

	if (boost_value < -100 || boost_value > 100)
		printk_deferred("warning: perf boost value should be -100~100\n");

	if (boost_value >= 100)
		boost_value = 100;
	else if (boost_value <= -100)
		boost_value = -100;

#if 0 /* ifdef CONFIG_CPU_FREQ_GOV_SCHEDPLUS */
	for (i = 0; i < cpu_cluster_nr; i++) {
		if (!floor)
			min_boost_freq[i] = 0;
		if (!cap_min)
			cap_min_freq[i] = 0;

#if MET_STUNE_DEBUG
		met_tag_oneshot(0, met_dvfs_info2[i], min_boost_freq[i]);
		met_tag_oneshot(0, met_dvfs_info3[i], cap_min_freq[i]);
#endif
	}

	if (!cap_min) {
		ct = allocated_group[group_idx];
		if (ct) {
			rcu_read_lock();
			ct->capacity_min = 0;
			/* Update CPU capacity_min */
			schedtune_boostgroup_update_capacity_min(
					ct->idx, ct->capacity_min);
			rcu_read_unlock();

#if MET_STUNE_DEBUG
			/* top-app */
			if (ct->idx == 3)
				met_tag_oneshot(
					0, "sched_boost_top_capacity_min",
					ct->capacity_min);
#endif
		}
	}
#endif

	if (boost_value < 0)
		global_negative_flag = true; /* set all group negative */
	else
		global_negative_flag = false;

	sys_boosted = boost_value;

	for (idx = 0; idx < BOOSTGROUPS_COUNT; idx++) {
		/* positive path or google original path */
		if (!global_negative_flag)
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
				met_tag_oneshot(
					0, "sched_boost_top", ct->boost);
#endif

			if (!global_negative_flag)
				break;

		} else {
			/* positive path or google original path */
			if (!global_negative_flag) {
				printk_deferred("error: perf boost for stune group no exist: idx=%d\n",
						idx);
				return -EINVAL;
			}

			break;
		}
	}

#if 0 /* ifdef CONFIG_CPU_FREQ_GOV_SCHEDPLUS */
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

#ifdef CONFIG_MTK_SCHED_RQAVG_KS
/* mtk: a linear boost value for tuning */
int linear_real_boost(int linear_boost)
{
	int target_cpu, usage;
	int boost;
	int ta_org_cap;

	sched_max_util_task(&target_cpu, NULL, &usage, NULL);

	ta_org_cap = capacity_orig_of(target_cpu);

	if (usage >= SCHED_CAPACITY_SCALE)
		usage = SCHED_CAPACITY_SCALE;

	/*
	 * Conversion Formula of Linear Boost:
	 *
	 *   margin = (usage * linear_boost)/100;
	 *   margin = (original_cap - usage) * boost/100;
	 *   so
	 *   boost = (usage * linear_boost) / (original_cap - usage)
	 */
	if (ta_org_cap <= usage) {
		/* If target cpu is saturated, consider bigger one */
		boost = (SCHED_CAPACITY_SCALE - usage) ?
		   (usage * linear_boost)/(SCHED_CAPACITY_SCALE - usage) : 0;
	} else
		boost = (usage * linear_boost)/(ta_org_cap - usage);

	return boost;
}
EXPORT_SYMBOL(linear_real_boost);
#endif
