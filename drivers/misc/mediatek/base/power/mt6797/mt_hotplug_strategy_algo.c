/**
* @file    mt_hotplug_strategy_algo.c
* @brief   hotplug strategy(hps) - algo
*/

/*============================================================================*/
/* Include files */
/*============================================================================*/
/* system includes */
#include <linux/kernel.h>	/* printk */
#include <linux/module.h>	/* MODULE_DESCRIPTION, MODULE_LICENSE */
#include <linux/init.h>		/* module_init, module_exit */
#include <linux/cpu.h>		/* cpu_up */
#include <linux/kthread.h>	/* kthread_create */
#include <linux/wakelock.h>	/* wake_lock_init */
#include <linux/delay.h>	/* msleep */
#include <asm-generic/bug.h>	/* BUG_ON */

/* local includes */
#include "mt_hotplug_strategy_internal.h"

/* forward references */

/*============================================================================*/
/* Macro definition */
/*============================================================================*/
/*
 * static
 */
#define STATIC
/* #define STATIC static */

/*
 * config
 */

/*============================================================================*/
/* Local type definition */
/*============================================================================*/

/*============================================================================*/
/* Local function declarition */
/*============================================================================*/

/*============================================================================*/
/* Local variable definition */
/*============================================================================*/

/*============================================================================*/
/* Global variable definition */
/*============================================================================*/

/*============================================================================*/
/* Local function definition */
/*============================================================================*/

/*============================================================================*/
/* Gobal function definition */
/*============================================================================*/

/* =============================================================================================== */
/*
* New hotpug strategy
*/
static void hps_algo_do_cluster_action(unsigned int cluster_id)
{
	unsigned int cpu, target_cores, online_cores, cpu_id_min, cpu_id_max;

	target_cores = hps_sys.cluster_info[cluster_id].target_core_num;
	online_cores = hps_sys.cluster_info[cluster_id].online_core_num;
	cpu_id_min = hps_sys.cluster_info[cluster_id].cpu_id_min;
	cpu_id_max = hps_sys.cluster_info[cluster_id].cpu_id_max;

	if (target_cores > online_cores) {	/*Power upcpus */
		for (cpu = cpu_id_min; cpu <= cpu_id_max; ++cpu) {
			if (!cpu_online(cpu)) {	/* For CPU offline */
				if (cpu_up(cpu))
					hps_warn("CPU %d power on Fail\n", cpu);
				++online_cores;
			}
			if (target_cores == online_cores)
				break;
		}

	} else {		/*Power downcpus */
		for (cpu = cpu_id_max; cpu >= cpu_id_min; --cpu) {
			if (cpu_online(cpu)) {
				if (cpu_down(cpu))
					hps_warn("CPU %d power down Fail\n", cpu);
				--online_cores;
			}
			if (target_cores == online_cores)
				break;
		}
	}
}

unsigned int get_cluster_cpus(unsigned int cluster_id)
{
	struct cpumask cls_cpus, cpus;

	arch_get_cluster_cpus(&cls_cpus, cluster_id);
	cpumask_and(&cpus, cpu_online_mask, &cls_cpus);
	return cpumask_weight(&cpus);
}

void hps_check_base_limit(struct hps_sys_struct *hps_sys)
{
	int i;

	for (i = 0; i < hps_sys->cluster_num; i++) {
		if (hps_sys->cluster_info[i].target_core_num < hps_sys->cluster_info[i].base_value)
			hps_sys->cluster_info[i].target_core_num =
			    hps_sys->cluster_info[i].base_value;
		if (hps_sys->cluster_info[i].target_core_num > hps_sys->cluster_info[i].limit_value)
			hps_sys->cluster_info[i].target_core_num =
			    hps_sys->cluster_info[i].limit_value;
	}
}

int hps_cal_core_num(struct hps_sys_struct *hps_sys, unsigned int core_val, unsigned int base_val)
{
	int i, cpu, root_cluster;

	/* Process root cluster */
	root_cluster = hps_sys->root_cluster_id;
	for (cpu = hps_sys->cluster_info[root_cluster].base_value;
	     cpu < hps_sys->cluster_info[root_cluster].limit_value;
	     cpu++, hps_sys->cluster_info[root_cluster].target_core_num++, core_val--) {
		if (core_val <= 0)
			goto out;
	}

	for (i = 0; i < hps_sys->cluster_num; i++) {
		if (hps_sys->cluster_info[i].is_root == 1)	/* Skip root cluster */
			continue;
		for (cpu = hps_sys->cluster_info[i].base_value;
		     cpu < hps_sys->cluster_info[i].limit_value;
		     cpu++, hps_sys->cluster_info[i].target_core_num++, core_val--) {
			if (core_val <= 0)
				goto out;
		}
	}
out:				/* Add base value of per-cluster by default */
	for (i = 0; i < hps_sys->cluster_num; i++)
		hps_sys->cluster_info[i].target_core_num += hps_sys->cluster_info[i].base_value;
	return 0;
}

void hps_define_root_cluster(struct hps_sys_struct *hps_sys)
{
	int i;
	/*Determine root cluster. */
	if (hps_sys->cluster_info[hps_sys->root_cluster_id].limit_value > 0)
		return;
	for (i = 0; i < hps_sys->cluster_num; i++) {
		if (hps_sys->cluster_info[i].limit_value > 0) {
			hps_sys->root_cluster_id = i;
			break;
		}
	}
}

void hps_algo_main(void)
{
	unsigned int i, val, base_val, action_print, origin_root;
	char str_online[64], str_criteria_limit[64], str_criteria_base[64], str_target[64];
	char *online_ptr = str_online;
	char *criteria_limit_ptr = str_criteria_limit;
	char *criteria_base_ptr = str_criteria_base;
	char *target_ptr = str_target;

	/*
	 * run algo or not by hps_ctxt.enabled
	 */
	if (!hps_ctxt.enabled) {
		atomic_set(&hps_ctxt.is_ondemand, 0);
		return;
	}

	/*
	 * algo - begin
	 */
	mutex_lock(&hps_ctxt.lock);
	hps_ctxt.action = ACTION_NONE;
	atomic_set(&hps_ctxt.is_ondemand, 0);

	/* Initial value */
	base_val = action_print = hps_sys.total_online_cores = 0;
	hps_sys.up_load_avg = hps_sys.down_load_avg = hps_sys.tlp_avg = hps_sys.rush_cnt = 0;
	hps_sys.action_id = 0;
	/* Calculate base core num and reset data structure */
	for (i = 0; i < hps_sys.cluster_num; i++) {
		base_val += hps_sys.cluster_info[i].base_value;
		hps_sys.cluster_info[i].target_core_num = hps_sys.cluster_info[i].online_core_num =
		    0;
		hps_sys.cluster_info[i].online_core_num =
		    get_cluster_cpus(hps_sys.cluster_info[i].cluster_id);
		hps_sys.total_online_cores += hps_sys.cluster_info[i].online_core_num;
	}

	/* Determine root cluster */
	origin_root = hps_sys.root_cluster_id;
	hps_define_root_cluster(&hps_sys);
	if (origin_root != hps_sys.root_cluster_id)
		hps_sys.action_id = HPS_SYS_CHANGE_ROOT;

	/*
	 * update history - tlp
	 */
	val = hps_ctxt.tlp_history[hps_ctxt.tlp_history_index];
	hps_ctxt.tlp_history[hps_ctxt.tlp_history_index] = hps_ctxt.cur_tlp;
	hps_ctxt.tlp_sum += hps_ctxt.cur_tlp;
	hps_ctxt.tlp_history_index =
	    (hps_ctxt.tlp_history_index + 1 ==
	     hps_ctxt.tlp_times) ? 0 : hps_ctxt.tlp_history_index + 1;
	++hps_ctxt.tlp_count;
	if (hps_ctxt.tlp_count > hps_ctxt.tlp_times) {
		BUG_ON(hps_ctxt.tlp_sum < val);
		hps_ctxt.tlp_sum -= val;
		hps_ctxt.tlp_avg = hps_ctxt.tlp_sum / hps_ctxt.tlp_times;
	} else {
		hps_ctxt.tlp_avg = hps_ctxt.tlp_sum / hps_ctxt.tlp_count;
	}
	if (hps_ctxt.stats_dump_enabled)
		hps_ctxt_print_algo_stats_tlp(0);
#if 0
/* ALGO_RUSH_BOOST: */
	/*
	 * algo - rush boost
	 */
	if (hps_ctxt.rush_boost_enabled) {
		if (hps_ctxt.cur_loads > hps_ctxt.rush_boost_threshold * hps_sys.total_online_cores)
			++hps_ctxt.rush_count;
		else
			hps_ctxt.rush_count = 0;
		if (hps_ctxt.rush_boost_times == 1)
			hps_ctxt.tlp_avg = hps_ctxt.cur_tlp;
		if ((hps_ctxt.rush_count >= hps_ctxt.rush_boost_times) &&
		    (hps_sys.total_online_cores * 100 < hps_ctxt.tlp_avg)) {
			val = hps_ctxt.tlp_avg / 100 + (hps_ctxt.tlp_avg % 100 ? 1 : 0);
			BUG_ON(!(val > hps_sys.total_online_cores));
			if (val > num_possible_cpus())
				val = num_possible_cpus();
			val -= base_val;
			hps_sys.tlp_avg = hps_ctxt.tlp_avg;
			hps_sys.rush_cnt = hps_ctxt.rush_count;
			hps_cal_core_num(&hps_sys, val, base_val);
			goto ALGO_END_WITH_ACTION;
		}
	}


	/* if (hps_ctxt.rush_boost_enabled) */
	/* ALGO_UP: */
	/*
	 * algo - cpu up
	 */
	if (hps_sys.total_online_cores < num_possible_cpus()) {
		/*
		 * update history - up
		 */
		val = hps_ctxt.up_loads_history[hps_ctxt.up_loads_history_index];
		hps_ctxt.up_loads_history[hps_ctxt.up_loads_history_index] = hps_ctxt.cur_loads;
		hps_ctxt.up_loads_sum += hps_ctxt.cur_loads;
		hps_ctxt.up_loads_history_index =
		    (hps_ctxt.up_loads_history_index + 1 ==
		     hps_ctxt.up_times) ? 0 : hps_ctxt.up_loads_history_index + 1;
		++hps_ctxt.up_loads_count;
		/* XXX: use >= or >, which is benifit? use > */
		if (hps_ctxt.up_loads_count > hps_ctxt.up_times) {
			BUG_ON(hps_ctxt.up_loads_sum < val);
			hps_ctxt.up_loads_sum -= val;
		}
		if (hps_ctxt.stats_dump_enabled)
			hps_ctxt_print_algo_stats_up(0);
		if (hps_ctxt.up_times == 1)
			hps_ctxt.up_loads_sum = hps_ctxt.cur_loads;
		if (hps_ctxt.up_loads_count >= hps_ctxt.up_times) {
			if (hps_ctxt.up_loads_sum >
			    hps_ctxt.up_threshold * hps_ctxt.up_times *
			    hps_sys.total_online_cores) {
				val = hps_sys.total_online_cores + 1;
				val -= base_val;
				hps_sys.up_load_avg = hps_ctxt.up_loads_sum / hps_ctxt.up_times;
				hps_cal_core_num(&hps_sys, val, base_val);
				goto ALGO_END_WITH_ACTION;
			}
		}		/* if (hps_ctxt.up_loads_count >= hps_ctxt.up_times) */
	}

/* ALGO_DOWN: */
	/*
	 * algo - cpu down (inc. quick landing)
	 */
	if (hps_sys.total_online_cores > 1) {
		/*
		 * update history - down
		 */
		val = hps_ctxt.down_loads_history[hps_ctxt.down_loads_history_index];
		hps_ctxt.down_loads_history[hps_ctxt.down_loads_history_index] = hps_ctxt.cur_loads;
		hps_ctxt.down_loads_sum += hps_ctxt.cur_loads;
		hps_ctxt.down_loads_history_index =
		    (hps_ctxt.down_loads_history_index + 1 ==
		     hps_ctxt.down_times) ? 0 : hps_ctxt.down_loads_history_index + 1;
		++hps_ctxt.down_loads_count;
		/* XXX: use >= or >, which is benifit? use > */
		if (hps_ctxt.down_loads_count > hps_ctxt.down_times) {
			BUG_ON(hps_ctxt.down_loads_sum < val);
			hps_ctxt.down_loads_sum -= val;
		}
		if (hps_ctxt.stats_dump_enabled)
			hps_ctxt_print_algo_stats_down(0);
		if (hps_ctxt.down_times == 1)
			hps_ctxt.down_loads_sum = hps_ctxt.cur_loads;
		if (hps_ctxt.down_loads_count >= hps_ctxt.down_times) {
			unsigned int down_threshold = hps_ctxt.down_threshold * hps_ctxt.down_times;

			val = hps_sys.total_online_cores;
			while (hps_ctxt.down_loads_sum < down_threshold * (val - 1))
				--val;
			BUG_ON(val < 0);
			val -= base_val;
			hps_sys.down_load_avg = hps_ctxt.down_loads_sum / hps_ctxt.down_times;
			hps_cal_core_num(&hps_sys, val, base_val);
			goto ALGO_END_WITH_ACTION;
		}		/* if (hps_ctxt.down_loads_count >= hps_ctxt.down_times) */
	}
#else
	for (i = 0; i < hps_sys.func_num; i++) {
		if (hps_sys.hps_sys_ops[i].enabled == 1) {
			if (hps_sys.hps_sys_ops[i].hps_sys_func_ptr()) {
				hps_sys.action_id = hps_sys.hps_sys_ops[i].func_id;
				break;
			}
		}
	}
	if (hps_sys.action_id)
		goto ALGO_END_WITH_ACTION;
#endif
	goto ALGO_END_WO_ACTION;

	/*
	 * algo - end
	 */
ALGO_END_WITH_ACTION:
	/*Base and limit check */
	hps_check_base_limit(&hps_sys);

	/* Ensure that root cluster must one online cpu at less */
	if (hps_sys.cluster_info[hps_sys.root_cluster_id].target_core_num <= 0)
		hps_sys.cluster_info[hps_sys.root_cluster_id].target_core_num = 1;

	/*Process root cluster first */
	if (hps_sys.cluster_info[hps_sys.root_cluster_id].target_core_num !=
	    hps_sys.cluster_info[hps_sys.root_cluster_id].online_core_num) {
		hps_algo_do_cluster_action(hps_sys.root_cluster_id);
		action_print = 1;
	}

	for (i = 0; i < hps_sys.cluster_num; i++) {
		if (i == hps_sys.root_cluster_id)
			continue;
		if (hps_sys.cluster_info[i].target_core_num !=
		    hps_sys.cluster_info[i].online_core_num) {
			hps_algo_do_cluster_action(i);
			action_print = 1;
		}
	}

	if (action_print) {
		int online, target, criteria_limit, criteria_base;

		online = target = criteria_limit = criteria_base = 0;
		for (i = 0; i < hps_sys.cluster_num; i++) {
			if (i == origin_root)
				online =
				    sprintf(online_ptr, "<%d>",
					    hps_sys.cluster_info[i].online_core_num);
			else
				online =
				    sprintf(online_ptr, "(%d)",
					    hps_sys.cluster_info[i].online_core_num);

			if (i == hps_sys.root_cluster_id)
				target =
				    sprintf(target_ptr, "<%d>",
					    hps_sys.cluster_info[i].target_core_num);
			else
				target =
				    sprintf(target_ptr, "(%d)",
					    hps_sys.cluster_info[i].target_core_num);

			criteria_limit =
			    sprintf(criteria_limit_ptr, "(%d)",
				    hps_sys.cluster_info[i].limit_value);
			criteria_base =
			    sprintf(criteria_base_ptr, "(%d)", hps_sys.cluster_info[i].base_value);

			online_ptr += online;
			target_ptr += target;
			criteria_limit_ptr += criteria_limit;
			criteria_base_ptr += criteria_base;
		}
		hps_warn("(0x%X)%s action end (%u)(%u)(%u)(%u) %s%s (%u)(%u)(%u)(%u) %s\n",
			 hps_sys.action_id, str_online, hps_ctxt.cur_loads, hps_ctxt.cur_tlp,
			 hps_ctxt.cur_iowait, hps_ctxt.cur_nr_heavy_task, str_criteria_limit,
			 str_criteria_base, hps_sys.up_load_avg, hps_sys.down_load_avg,
			 hps_sys.tlp_avg, hps_sys.rush_cnt, str_target);
	}
	hps_ctxt_reset_stas_nolock();

ALGO_END_WO_ACTION:
	action_print = 0;
	mutex_unlock(&hps_ctxt.lock);
}
