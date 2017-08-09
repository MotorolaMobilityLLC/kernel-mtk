/**
* @file    mt_hotplug_strategy_core.c
* @brief   hotplug strategy(hps) - core
*/

/*============================================================================*/
/* Include files */
/*============================================================================*/
/* system includes */
#include <linux/kernel.h>
#include <linux/slab.h>

/* local includes */
#include "mt_hotplug_strategy_internal.h"
#include "mt_hotplug_strategy.h"

/* forward references */

/*============================================================================*/
/* Macro definition */
/*============================================================================*/

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
/* Local function definition */
/*============================================================================*/
static int cal_base_cores(void)
{
	int i, base_val;

	i = base_val = 0;
	for (i = 0; i < hps_sys.cluster_num; i++)
		base_val += hps_sys.cluster_info[i].base_value;

	return base_val;
}

static int hps_algo_rush_boost(void)
{
	unsigned int val, base_val;

	base_val = cal_base_cores();

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

		return 1;
	}

	return 0;
}

/*
 * update history - up
 */
static int hps_algo_up(void)
{
	unsigned int val, base_val;

	base_val = cal_base_cores();

	if (hps_sys.total_online_cores < num_possible_cpus()) {
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
				return 1;
			}
		}		/* if (hps_ctxt.up_loads_count >= hps_ctxt.up_times) */
	}
	return 0;
}

/*
 * algo - cpu down (inc. quick landing)
 */
static int hps_algo_down(void)
{
	unsigned int val, base_val;

	base_val = cal_base_cores();

	if (hps_sys.total_online_cores > 1) {
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
			return 1;
		}		/* if (hps_ctxt.down_loads_count >= hps_ctxt.down_times) */
	}
	return 0;
}

static int hps_algo_heavytsk_det(void)
{
	return 0;
}

static int hps_algo_perf_indicator(void)
{
	return 0;
}

/* Notice : Sorting function pointer by priority */
static int (*hps_func[]) (void) = {
hps_algo_heavytsk_det, hps_algo_perf_indicator, hps_algo_rush_boost, hps_algo_up,
	    hps_algo_down};
int hps_ops_init(void)
{
	int i;

	hps_sys.func_num = 0;
	hps_sys.func_num = sizeof(hps_func) / sizeof(hps_func[0]);
	hps_sys.hps_sys_ops = kcalloc(hps_sys.func_num, sizeof(*hps_sys.hps_sys_ops), GFP_KERNEL);
	if (!hps_sys.hps_sys_ops) {
		hps_warn("@%s: fail to allocate memory for hps_sys_ops!\n", __func__);
		return -ENOMEM;
	}

	for (i = 0; i < hps_sys.func_num; i++) {
		hps_sys.hps_sys_ops[i].func_id = 0xF00 + i;
		hps_sys.hps_sys_ops[i].enabled = 1;
		hps_sys.hps_sys_ops[i].hps_sys_func_ptr = *hps_func[i];
		hps_warn("%d: func_id %d, enabled %d\n", i, hps_sys.hps_sys_ops[i].func_id,
			 hps_sys.hps_sys_ops[i].enabled);
	}

	return 0;
}
