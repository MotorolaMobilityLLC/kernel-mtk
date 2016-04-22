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

/* power on_off cpus of cluster*/
/*Use for Fast hotplug.*/
#ifdef CONFIG_CPU_ISOLATION
void hps_algo_cpu_cluster_action(unsigned int online_cores, unsigned int target_cores,
				 unsigned int cpu_id_min, unsigned int cpu_id_max,
				 unsigned int cluster)
{
	unsigned int cpu;
	struct cpumask target_cpu_up_cpumask, target_cpu_down_cpumask, target_cpu_isolate_mask;

	cpumask_clear(&target_cpu_up_cpumask);
	cpumask_clear(&target_cpu_down_cpumask);
	cpumask_copy(&target_cpu_isolate_mask, cpu_isolate_mask);

	if (target_cores > online_cores) {	/*Power upcpus */
		for (cpu = cpu_id_min; cpu <= cpu_id_max; ++cpu) {
			if (!cpumask_test_cpu(cpu, cpu_online_mask)) {	/* For CPU offline */
				cpumask_set_cpu(cpu, &target_cpu_up_cpumask);
				++online_cores;
			} else if (cpumask_test_cpu(cpu, cpu_isolate_mask)) {
				cpumask_clear_cpu(cpu, &target_cpu_isolate_mask);
				++online_cores;
			}
			if (target_cores == online_cores)
				break;
		}
		cpu_up_by_mask(&target_cpu_up_cpumask);
		cpu_isolate_set(&target_cpu_isolate_mask);

	} else {		/*Power downcpus */

		for (cpu = cpu_id_max; cpu >= cpu_id_min; --cpu) {
			if (cpumask_test_cpu(cpu, cpu_online_mask)) {
				if (!cpumask_test_cpu(cpu, cpu_isolate_mask))
					cpumask_set_cpu(cpu, &target_cpu_isolate_mask);
				else
					continue;
				--online_cores;
			}
			if (target_cores == online_cores)
				break;
		}
		cpu_isolate_set(&target_cpu_isolate_mask);
	}
}
#else
/*
*power on/off cpus of cluster
*For legacy hotplug use
*/
static void hps_algo_cpu_cluster_action(unsigned int online_cores, unsigned int target_cores,
					unsigned int cpu_id_min, unsigned int cpu_id_max,
					unsigned int cluster)
{
	unsigned int cpu;

	if (target_cores > online_cores) {	/*Power upcpus */
		for (cpu = cpu_id_min; cpu <= cpu_id_max; ++cpu) {
			if (!cpu_online(cpu)) {	/* For CPU offline */
				cpu_up(cpu);
				++online_cores;
			}
			if (target_cores == online_cores)
				break;
		}

	} else {		/*Power downcpus */

		for (cpu = cpu_id_max; cpu >= cpu_id_min; --cpu) {
			if (cpu_online(cpu)) {
				cpu_down(cpu);
				--online_cores;
			}
			if (target_cores == online_cores)
				break;
		}
	}
}
#endif

/* =============================================================================================== */
/*
* New hotpug strategy
*/

void hps_cal_cores(struct hps_func_data *hps_func)
{
	int cpu;

	switch (hps_func->target_root_cpu) {
	case 0:
		for (cpu = hps_func->base_LL;
		     cpu < hps_func->limit_LL; cpu++, hps_func->target_LL++, hps_func->cores--) {
			if (hps_func->cores <= 0)
				break;
		}
		if (hps_func->cores > 0) {
			for (cpu = hps_func->base_L; cpu < hps_func->limit_LL; cpu++) {
				hps_func->target_L++;
				hps_func->cores--;
			}
		}
		set_bit(hps_func->action_LL, (unsigned long *)&hps_ctxt.action);
		break;

	case 4:
		for (cpu = hps_func->base_L;
		     cpu < hps_func->limit_L; cpu++, hps_func->target_L++, hps_func->cores--) {
			if (hps_func->cores <= 0)
				break;
		}
		if (hps_func->cores > 0) {
			for (cpu = hps_func->base_L; cpu < hps_func->limit_LL; cpu++) {
				hps_func->target_LL++;
				hps_func->cores--;
			}
		}
		set_bit(hps_func->action_L, (unsigned long *)&hps_ctxt.action);
		break;
	default:
		break;
	}
}

void hps_algo_amp(void)
{
	int val, base_val, target_little_cores, target_big_cores;
	struct cpumask little_online_cpumask;
	struct cpumask big_online_cpumask;
	int little_num_base, little_num_limit, little_num_online;
	int big_num_base, big_num_limit, big_num_online;
	int target_root_cpu, state_tran_active;
	struct hps_func_data hps_func;
	static unsigned int hps_disable_cnt;
	/*
	 * run algo or not by hps_ctxt.enabled
	 */
	if (!hps_ctxt.enabled) {
		if ((hps_disable_cnt % 0x80) == 0)
			hps_warn("[WARN] HPS is disabled!!!\n");
		atomic_set(&hps_ctxt.is_ondemand, 0);
		hps_disable_cnt++;
		return;
	}

	/*
	 * algo - begin
	 */
	mutex_lock(&hps_ctxt.lock);
	hps_ctxt.action = ACTION_NONE;
	atomic_set(&hps_ctxt.is_ondemand, 0);

	/*
	 * algo - get boundary
	 */
	little_num_limit = hps_ctxt.little_num_limit_power_serv;
	little_num_base = hps_ctxt.little_num_base_perf_serv;
	cpumask_and(&little_online_cpumask, &hps_ctxt.little_cpumask, cpu_online_mask);

#ifdef CONFIG_CPU_ISOLATION
	cpumask_andnot(&little_online_cpumask, &little_online_cpumask, cpu_isolate_mask);
#endif
	little_num_online = cpumask_weight(&little_online_cpumask);

	big_num_limit = hps_ctxt.big_num_limit_power_serv;
	big_num_base = hps_ctxt.big_num_base_perf_serv;

	cpumask_and(&big_online_cpumask, &hps_ctxt.big_cpumask, cpu_online_mask);

#ifdef CONFIG_CPU_ISOLATION
	cpumask_andnot(&big_online_cpumask, &big_online_cpumask, cpu_isolate_mask);
#endif
	big_num_online = cpumask_weight(&big_online_cpumask);

	base_val = little_num_base + big_num_base;
	target_little_cores = little_num_online;
	target_big_cores = big_num_online;

	if (hps_ctxt.cur_dump_enabled) {
		hps_debug(
		"loads(%u), hvy_tsk(%u), tlp(%u), iowait(%u), limit_t(%u)(%u), limit_lb(%u)(%u), limit_ups(%u)(%u), limit_pos(%u)(%u), base_pes(%u)(%u)\n",
		     hps_ctxt.cur_loads, hps_ctxt.cur_nr_heavy_task, hps_ctxt.cur_tlp,
		     hps_ctxt.cur_iowait, hps_ctxt.little_num_limit_thermal,
		     hps_ctxt.big_num_limit_thermal, hps_ctxt.little_num_limit_low_battery,
		     hps_ctxt.big_num_limit_low_battery,
		     hps_ctxt.little_num_limit_ultra_power_saving,
		     hps_ctxt.big_num_limit_ultra_power_saving,
		     hps_ctxt.little_num_limit_power_serv, hps_ctxt.big_num_limit_power_serv,
		     hps_ctxt.little_num_base_perf_serv, hps_ctxt.big_num_base_perf_serv);
	}
	/* Determine target_root_cpu */
	target_root_cpu = hps_ctxt.root_cpu;
	state_tran_active = 0;
	if ((hps_ctxt.root_cpu == 0) && (little_num_base == 0) && (big_num_base > 0))
		target_root_cpu = 4;
	if ((hps_ctxt.root_cpu == 4) && (big_num_base == 0) && (little_num_base > 0))
		target_root_cpu = 0;

	if ((hps_ctxt.root_cpu == 0) && (little_num_base == 0) && (little_num_limit == 0))
		target_root_cpu = 4;
	if ((hps_ctxt.root_cpu == 4) && (big_num_base == 0) && (big_num_limit == 0))
		target_root_cpu = 0;
	if (hps_ctxt.root_cpu != target_root_cpu)
		state_tran_active = 1;
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

	hps_func.limit_LL = hps_ctxt.little_num_limit_power_serv;
	hps_func.limit_L = hps_ctxt.big_num_limit_power_serv;
	hps_func.base_LL = hps_ctxt.little_num_base_perf_serv;
	hps_func.base_L = hps_ctxt.big_num_base_perf_serv;
	hps_func.target_root_cpu = target_root_cpu;
/* ALGO_RUSH_BOOST: */
	/*
	 * algo - rush boost
	 */
	if (hps_ctxt.rush_boost_enabled) {
		if (hps_ctxt.cur_loads >
		    hps_ctxt.rush_boost_threshold * (little_num_online + big_num_online))
			++hps_ctxt.rush_count;
		else
			hps_ctxt.rush_count = 0;
		if (hps_ctxt.rush_boost_times == 1)
			hps_ctxt.tlp_avg = hps_ctxt.cur_tlp;
		if ((hps_ctxt.rush_count >= hps_ctxt.rush_boost_times) &&
		    ((little_num_online + big_num_online) * 100 < hps_ctxt.tlp_avg)) {
			val = hps_ctxt.tlp_avg / 100 + (hps_ctxt.tlp_avg % 100 ? 1 : 0);

			BUG_ON(!(val > little_num_online + big_num_online));
			if (val > num_possible_cpus())
				val = num_possible_cpus();
			target_little_cores = target_big_cores = 0;
			val -= base_val;

			hps_func.cores = val;
			hps_func.action_LL = ACTION_RUSH_BOOST_LITTLE;
			hps_func.action_L = ACTION_RUSH_BOOST_BIG;
			hps_func.target_LL = hps_func.target_L = 0;
			hps_cal_cores(&hps_func);
			target_little_cores = hps_func.target_LL;
			target_big_cores = hps_func.target_L;
		}
	}			/* if (hps_ctxt.rush_boost_enabled) */
	if (hps_ctxt.action) {
		target_little_cores += hps_ctxt.little_num_base_perf_serv;
		target_big_cores += hps_ctxt.big_num_base_perf_serv;
		if (!((little_num_online == target_little_cores)
		      && (big_num_online == target_big_cores)))
			goto ALGO_END_WITH_ACTION;
		else
			hps_ctxt.action = ACTION_NONE;
	}

/* ALGO_UP: */
	/*
	 * algo - cpu up
	 */
	if ((little_num_online + big_num_online) < num_possible_cpus()) {

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
			target_little_cores = target_big_cores = 0;
			if (hps_ctxt.up_loads_sum >
			    hps_ctxt.up_threshold * hps_ctxt.up_times * (little_num_online +
									 big_num_online)) {
				val = little_num_online + big_num_online + 1;
				target_little_cores = target_big_cores = 0;
				val -= base_val;

				hps_func.cores = val;
				hps_func.action_LL = ACTION_UP_LITTLE;
				hps_func.action_L = ACTION_UP_BIG;
				hps_func.target_LL = hps_func.target_L = 0;
				hps_cal_cores(&hps_func);
				target_little_cores = hps_func.target_LL;
				target_big_cores = hps_func.target_L;
			}
		}		/* if (hps_ctxt.up_loads_count >= hps_ctxt.up_times) */
	}
	/* if ((little_num_online + big_num_online) < num_possible_cpus()) */
	if (hps_ctxt.action) {
		target_little_cores += hps_ctxt.little_num_base_perf_serv;
		target_big_cores += hps_ctxt.big_num_base_perf_serv;
		if (!((little_num_online == target_little_cores)
		      && (big_num_online == target_big_cores)))
			goto ALGO_END_WITH_ACTION;
		else
			hps_ctxt.action = ACTION_NONE;
	}
/* ALGO_DOWN: */
	/*
	 * algo - cpu down (inc. quick landing)
	 */
	if (little_num_online + big_num_online > 1) {
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
		if (hps_ctxt.up_times == 1)
			hps_ctxt.down_loads_sum = hps_ctxt.cur_loads;
		if (hps_ctxt.down_loads_count >= hps_ctxt.down_times) {
			unsigned int down_threshold = hps_ctxt.down_threshold * hps_ctxt.down_times;

			val = little_num_online + big_num_online;
			while (hps_ctxt.down_loads_sum < down_threshold * (val - 1))
				--val;
			BUG_ON(val < 0);
			target_little_cores = target_big_cores = 0;
			val -= base_val;

			hps_func.cores = val;
			hps_func.action_LL = ACTION_DOWN_LITTLE;
			hps_func.action_L = ACTION_DOWN_BIG;
			hps_func.target_LL = hps_func.target_L = 0;
			hps_cal_cores(&hps_func);
			target_little_cores = hps_func.target_LL;
			target_big_cores = hps_func.target_L;

		}		/* if (hps_ctxt.down_loads_count >= hps_ctxt.down_times) */
	}

	/* if (little_num_online + big_num_online > 1) */
	if (hps_ctxt.action) {
		target_little_cores += hps_ctxt.little_num_base_perf_serv;
		target_big_cores += hps_ctxt.big_num_base_perf_serv;
		if (!((little_num_online == target_little_cores)
		      && (big_num_online == target_big_cores)))
			goto ALGO_END_WITH_ACTION;
		else
			hps_ctxt.action = ACTION_NONE;
	}
/*ACTION_ROOT_TRAN: */
	/* Process "ONLY" root cpu transition */
	if (state_tran_active) {
		target_little_cores = target_big_cores = 0;
		val = little_num_online + big_num_online;
		val -= base_val;

		hps_func.cores = val;
		hps_func.action_LL = ACTION_ROOT_2_LITTLE;
		hps_func.action_L = ACTION_ROOT_2_BIG;
		hps_func.target_LL = hps_func.target_L = 0;
		hps_cal_cores(&hps_func);
		target_little_cores = hps_func.target_LL;
		target_big_cores = hps_func.target_L;
		state_tran_active = 0;
	}

	if (hps_ctxt.action) {
		target_little_cores += hps_ctxt.little_num_base_perf_serv;
		target_big_cores += hps_ctxt.big_num_base_perf_serv;
		if (!((little_num_online == target_little_cores)
		      && (big_num_online == target_big_cores)))
			goto ALGO_END_WITH_ACTION;
		else
			hps_ctxt.action = ACTION_NONE;
	}

/*Base and limit check*/
	if (target_little_cores < hps_ctxt.little_num_base_perf_serv)
		target_little_cores = hps_ctxt.little_num_base_perf_serv;
	if (target_little_cores > hps_ctxt.little_num_limit_power_serv)
		target_little_cores = hps_ctxt.little_num_limit_power_serv;

	if (target_big_cores < hps_ctxt.big_num_base_perf_serv)
		target_big_cores = hps_ctxt.big_num_base_perf_serv;
	if (target_big_cores > hps_ctxt.big_num_limit_power_serv)
		target_big_cores = hps_ctxt.big_num_limit_power_serv;

	if (!((little_num_online == target_little_cores) && (big_num_online == target_big_cores)))
		goto ALGO_END_WITH_ACTION;
	else
		hps_ctxt.action = ACTION_NONE;


	if (!hps_ctxt.action)
		goto ALGO_END_WO_ACTION;


	/*
	 * algo - end
	 */
ALGO_END_WITH_ACTION:

/*Base and limit check*/
	if (target_little_cores < hps_ctxt.little_num_base_perf_serv)
		target_little_cores = hps_ctxt.little_num_base_perf_serv;
	if (target_little_cores > hps_ctxt.little_num_limit_power_serv)
		target_little_cores = hps_ctxt.little_num_limit_power_serv;

	if (target_big_cores < hps_ctxt.big_num_base_perf_serv)
		target_big_cores = hps_ctxt.big_num_base_perf_serv;
	if (target_big_cores > hps_ctxt.big_num_limit_power_serv)
		target_big_cores = hps_ctxt.big_num_limit_power_serv;


	if (target_root_cpu == 4) {
		if (target_big_cores == 0)
			target_big_cores++;	/*Root cpu must alive!! */
		/*Process big cluster */
		if (big_num_online != target_big_cores)
			hps_algo_cpu_cluster_action(big_num_online, target_big_cores,
						    hps_ctxt.big_cpu_id_min,
						    hps_ctxt.big_cpu_id_max, 1);

		/*process little cluster */
		if (little_num_online != target_little_cores)
			hps_algo_cpu_cluster_action(little_num_online, target_little_cores,
						    hps_ctxt.little_cpu_id_min,
						    hps_ctxt.little_cpu_id_max, 0);
	} else if (target_root_cpu == 0) {
		if (target_little_cores == 0)
			target_little_cores++;	/*Root cpu must alive!! */

		/*Process little cluster */
		if (little_num_online != target_little_cores)
			hps_algo_cpu_cluster_action(little_num_online, target_little_cores,
						    hps_ctxt.little_cpu_id_min,
						    hps_ctxt.little_cpu_id_max, 0);

		/*process big cluster */
		if (big_num_online != target_big_cores)
			hps_algo_cpu_cluster_action(big_num_online, target_big_cores,
						    hps_ctxt.big_cpu_id_min,
						    hps_ctxt.big_cpu_id_max, 1);
	} else
		hps_warn("ERROR! root cpu %d\n", target_root_cpu);
	if (!((little_num_online == target_little_cores) && (big_num_online == target_big_cores))) {
		hps_warn(
		"END :(%04lx)(%u)(%u) action end(%u)(%u)(%u)(%u) (%u)(%u)(%u)(%u) (%u)(%u)(%u) (%u)(%u)(%u) (%u)(%u)(%u)(%u)(%u) (%u)(%u)(%u)\n",
	hps_ctxt.action, little_num_online, big_num_online, hps_ctxt.cur_loads, hps_ctxt.cur_tlp,
	hps_ctxt.cur_iowait, hps_ctxt.cur_nr_heavy_task, hps_ctxt.little_num_limit_power_serv,
	hps_ctxt.big_num_limit_power_serv, hps_ctxt.little_num_base_perf_serv, hps_ctxt.big_num_base_perf_serv,
	hps_ctxt.up_loads_sum, hps_ctxt.up_loads_count, hps_ctxt.up_loads_history_index, hps_ctxt.down_loads_sum,
	hps_ctxt.down_loads_count, hps_ctxt.down_loads_history_index, hps_ctxt.rush_count, hps_ctxt.tlp_sum,
	hps_ctxt.tlp_count, hps_ctxt.tlp_history_index, hps_ctxt.tlp_avg, target_root_cpu, target_little_cores,
	target_big_cores);
	}
	hps_ctxt_reset_stas_nolock();

ALGO_END_WO_ACTION:

	mutex_unlock(&hps_ctxt.lock);
}
