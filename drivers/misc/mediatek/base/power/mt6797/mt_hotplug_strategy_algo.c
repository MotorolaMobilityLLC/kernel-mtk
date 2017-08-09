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
#include "mt_ptp.h"
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
static int hps_algo_heavytsk_det(void)
{
	int i, j, ret, sys_cores, hvy_cores;

	i = j = ret = sys_cores = hvy_cores = 0;
	/*sys_cores = hps_cal_cores(); */
	/* Check heavy task value of last cluster if needed */
#if 0
	if (hps_sys.cluster_info[hps_sys.cluster_num - 1].hvyTsk_value)
		ret = 1;
#endif

	for (i = hps_sys.cluster_num - 1; i > 0; i--) {
		if (hps_sys.cluster_info[i - 1].hvyTsk_value)
			ret = 1;
		hvy_cores += hps_sys.cluster_info[i].hvyTsk_value;

		if (i == hps_sys.cluster_num - 1)
			hps_sys.cluster_info[i].target_core_num =
			    max3(hps_sys.cluster_info[i].target_core_num,
				 hps_sys.cluster_info[i - 1].hvyTsk_value,
				 hps_sys.cluster_info[i].hvyTsk_value);
		else
			hps_sys.cluster_info[i].target_core_num =
			    max(hps_sys.cluster_info[i].target_core_num,
				hps_sys.cluster_info[i - 1].hvyTsk_value);
		if (hps_sys.cluster_info[i].target_core_num > hps_sys.cluster_info[i].limit_value)
			hps_sys.cluster_info[i].target_core_num =
			    hps_sys.cluster_info[i].limit_value;
		sys_cores += hps_sys.cluster_info[i].target_core_num;
	}
#if 1
	hvy_cores += hps_sys.cluster_info[0].hvyTsk_value;
	sys_cores += hps_sys.cluster_info[0].target_core_num;
	if (sys_cores < hvy_cores) {
		for (i = hps_sys.cluster_num - 1; i >= 0; i--) {
			for (j = hps_sys.cluster_info[i].target_core_num;
			     j < hps_sys.cluster_info[i].limit_value; j++) {
				if (sys_cores >= hvy_cores)
					break;
				hps_sys.cluster_info[i].target_core_num++;
				hvy_cores--;
				ret = 1;
			}
		}
	}
#endif
	return ret;
}

static void hps_algo_do_cluster_action(unsigned int cluster_id)
{
	unsigned int cpu, target_cores, online_cores, cpu_id_min, cpu_id_max;

	target_cores = hps_sys.cluster_info[cluster_id].target_core_num;
	online_cores = hps_sys.cluster_info[cluster_id].online_core_num;
	cpu_id_min = hps_sys.cluster_info[cluster_id].cpu_id_min;
	cpu_id_max = hps_sys.cluster_info[cluster_id].cpu_id_max;

	if (target_cores > online_cores) {	/*Power up cpus */
		for (cpu = cpu_id_min; cpu <= cpu_id_max; ++cpu) {
			if (!cpu_online(cpu)) {	/* For CPU offline */
				if (cpu_up(cpu))
					hps_warn("[Info]CPU %d ++!\n", cpu);
				++online_cores;
			}
			if (target_cores == online_cores)
				break;
		}

	} else {		/*Power down cpus */
		for (cpu = cpu_id_max; cpu >= cpu_id_min; --cpu) {
			if (cpu_online(cpu)) {
				if (cpu_down(cpu))
					hps_warn("[Info]CPU %d --!\n", cpu);
				--online_cores;
			}
			if (target_cores == online_cores)
				break;
		}
	}
}

unsigned int hps_get_cluster_cpus(unsigned int cluster_id)
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

int hps_cal_core_num(struct hps_sys_struct *hps_sys, int core_val, int base_val)
{
	int i, cpu, root_cluster;

	if (core_val == 0)
		goto out;
	for (i = 0; i < hps_sys->cluster_num; i++)
		hps_sys->cluster_info[i].target_core_num = 0;

	/* Process root cluster */
	root_cluster = hps_sys->root_cluster_id;
	for (cpu = hps_sys->cluster_info[root_cluster].base_value;
		cpu <= hps_sys->cluster_info[root_cluster].limit_value;
		cpu++) {
		if (core_val <= 0)
			goto out;
		else {
			hps_sys->cluster_info[root_cluster].target_core_num++;
			core_val--;
		}
	}

	for (i = 0; i < hps_sys->cluster_num; i++) {
		if (root_cluster == i)	/* Skip root cluster */
			continue;
		for (cpu = hps_sys->cluster_info[i].base_value;
			cpu <= hps_sys->cluster_info[i].limit_value;
			cpu++) {
			if (core_val <= 0)
				goto out;
			else {
				hps_sys->cluster_info[root_cluster].target_core_num++;
				core_val--;
			}
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
	char str_online[64], str_criteria_limit[64], str_criteria_base[64], str_target[64],
	    str_hvytsk[64];
	char *online_ptr = str_online;
	char *criteria_limit_ptr = str_criteria_limit;
	char *criteria_base_ptr = str_criteria_base;
	char *hvytsk_ptr = str_hvytsk;
	char *target_ptr = str_target;
	static unsigned int hrtbt_dbg;
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
	hrtbt_dbg++;
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
		    hps_get_cluster_cpus(hps_sys.cluster_info[i].cluster_id);
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
	for (i = 0; i < hps_sys.func_num; i++) {
		if (hps_sys.hps_sys_ops[i].enabled == 1) {
			if (hps_sys.hps_sys_ops[i].hps_sys_func_ptr()) {
				hps_sys.action_id = hps_sys.hps_sys_ops[i].func_id;
				break;
			}
		}
	}
	if (get_efuse_status() != 0) {
		if (hps_algo_heavytsk_det())
			hps_sys.action_id = 0xE1;
	}
	/*
	 * algo - end
	 */
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

	if (action_print || hrtbt_dbg >= HPS_HRT_BT_DBG) {
		int online, target, criteria_limit, criteria_base, hvytsk;

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
			hvytsk = sprintf(hvytsk_ptr, "(%d)", hps_sys.cluster_info[i].hvyTsk_value);

			online_ptr += online;
			target_ptr += target;
			criteria_limit_ptr += criteria_limit;
			criteria_base_ptr += criteria_base;
			hvytsk_ptr += hvytsk;
		}
		if (action_print) {
			hps_warn("(0x%X)%s action end (%u)(%u)(%u) %s %s%s (%u)(%u)(%u)(%u) %s\n",
				 hps_sys.action_id, str_online, hps_ctxt.cur_loads,
				 hps_ctxt.cur_tlp, hps_ctxt.cur_iowait, str_hvytsk,
				 str_criteria_limit, str_criteria_base, hps_sys.up_load_avg,
				 hps_sys.down_load_avg, hps_sys.tlp_avg, hps_sys.rush_cnt,
				 str_target);
			hps_ctxt_reset_stas_nolock();
		}
	}
#if HPS_HRT_BT_EN
	if (hrtbt_dbg >= HPS_HRT_BT_DBG) {
		hps_warn("(0x%X)%s HRT_BT_DBG (%u)(%u)(%u) %s %s%s (%u)(%u)(%u)(%u) %s\n",
			 hps_sys.action_id, str_online, hps_ctxt.cur_loads, hps_ctxt.cur_tlp,
			 hps_ctxt.cur_iowait, str_hvytsk, str_criteria_limit,
			 str_criteria_base, hps_sys.up_load_avg, hps_sys.down_load_avg,
			 hps_sys.tlp_avg, hps_sys.rush_cnt, str_target);
		hrtbt_dbg = 0;
	}
#endif
	action_print = 0;
	mutex_unlock(&hps_ctxt.lock);
}
