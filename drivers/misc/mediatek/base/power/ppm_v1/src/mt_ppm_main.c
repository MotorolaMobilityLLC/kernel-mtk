/* system includes */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cpu.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/cpumask.h>
#include <linux/cpufreq.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/sched/rt.h>
#include <linux/string.h>
#include <asm/topology.h>

#include "mt_ppm_internal.h"


/* project includes */
#if 0
#include "mach/mt_static_power.h"
#endif


#define PPM_TIMER_INTERVAL_MS		(1000)

/*==============================================================*/
/* Local function declarition					*/
/*==============================================================*/
static int ppm_main_suspend(struct device *dev);
static int ppm_main_resume(struct device *dev);
static int ppm_main_pdrv_probe(struct platform_device *pdev);
static int ppm_main_pdrv_remove(struct platform_device *pdev);

/*==============================================================*/
/* Global variables						*/
/*==============================================================*/
struct ppm_data ppm_main_info = {
	.is_enabled = true,
	.is_in_suspend = false,

	.cur_mode = PPM_MODE_PERFORMANCE,
	.cur_power_state = PPM_POWER_STATE_NONE,
	.fixed_root_cluster = -1,
	.min_power_budget = ~0,

	.dvfs_tbl_type = DVFS_TABLE_TYPE_FY,

	.ppm_pm_ops = {
		.suspend	= ppm_main_suspend,
		.resume	= ppm_main_resume,
		.freeze	= ppm_main_suspend,
		.thaw	= ppm_main_resume,
		.restore	= ppm_main_resume,
	},
	.ppm_pdev = {
		.name	= "mt-ppm",
		.id	= -1,
	},
	.ppm_pdrv = {
		.probe	= ppm_main_pdrv_probe,
		.remove	= ppm_main_pdrv_remove,
		.driver	= {
			.name	= "mt-ppm",
			.pm	= &ppm_main_info.ppm_pm_ops,
			.owner	= THIS_MODULE,
		},
	},

	.lock = __MUTEX_INITIALIZER(ppm_main_info.lock),
	.policy_list = LIST_HEAD_INIT(ppm_main_info.policy_list),
	.ppm_task = NULL,
	.ppm_wq = __WAIT_QUEUE_HEAD_INITIALIZER(ppm_main_info.ppm_wq),
	.ppm_event = ATOMIC_INIT(0),
};


void ppm_main_update_req_by_pwr(enum ppm_power_state new_state, struct ppm_policy_req *req)
{
	struct ppm_power_tbl_data power_table = ppm_get_power_table();
	unsigned int index, i;

	index = ppm_hica_get_table_idx_by_pwr(new_state, req->power_budget);
	if (index != -1) {
		for (i = 0; i < req->cluster_num; i++) {
			req->limit[i].max_cpu_core =
				power_table.power_tbl[index].cluster_cfg[i].core_num;
			if (power_table.power_tbl[index].cluster_cfg[i].core_num == 0)
				req->limit[i].min_cpu_core = 0;
			else
				req->limit[i].max_cpufreq_idx =	power_table.power_tbl[index].cluster_cfg[i].opp_lv;

			/* error check */
			if (req->limit[i].max_cpu_core < req->limit[i].min_cpu_core)
				req->limit[i].min_cpu_core = req->limit[i].max_cpu_core;
			if (req->limit[i].max_cpufreq_idx > req->limit[i].min_cpufreq_idx)
				req->limit[i].min_cpufreq_idx = req->limit[i].max_cpufreq_idx;
		}
	} else
		ppm_dbg("@%s: index not found!", __func__);
}

int ppm_main_freq_to_idx(unsigned int cluster_id,
					unsigned int freq, unsigned int relation)
{
	int i, size;
	int idx = -1;

	FUNC_ENTER(FUNC_LV_MAIN);

	if (!ppm_main_info.cluster_info[cluster_id].dvfs_tbl) {
		ppm_err("@%s: DVFS table of cluster %d is not exist!\n", __func__, cluster_id);
		idx = (relation == CPUFREQ_RELATION_L)
			? get_cluster_min_cpufreq_idx(cluster_id) : get_cluster_max_cpufreq_idx(cluster_id);
		return idx;
	}

	size = ppm_main_info.cluster_info[cluster_id].dvfs_opp_num;

	/* error handle */
	if (freq > get_cluster_max_cpufreq(cluster_id))
		freq = ppm_main_info.cluster_info[cluster_id].dvfs_tbl[0].frequency;

	if (freq < get_cluster_min_cpufreq(cluster_id))
		freq = ppm_main_info.cluster_info[cluster_id].dvfs_tbl[size-1].frequency;

	/* search idx */
	if (relation == CPUFREQ_RELATION_L) {
		for (i = (signed)(size - 1); i >= 0; i--) {
			if (ppm_main_info.cluster_info[cluster_id].dvfs_tbl[i].frequency >= freq) {
				idx = i;
				break;
			}
		}
	} else { /* CPUFREQ_RELATION_H */
		for (i = 0; i < (signed)size; i++) {
			if (ppm_main_info.cluster_info[cluster_id].dvfs_tbl[i].frequency <= freq) {
				idx = i;
				break;
			}
		}
	}

	if (idx == -1) {
		ppm_err("@%s: freq %d KHz not found in DVFS table of cluster %d\n", __func__, freq, cluster_id);
		idx = (relation == CPUFREQ_RELATION_L)
			? get_cluster_min_cpufreq_idx(cluster_id)
			: get_cluster_max_cpufreq_idx(cluster_id);
	} else
		ppm_ver("@%s: The idx of %d KHz in cluster %d is %d\n", __func__, freq, cluster_id, idx);

	FUNC_EXIT(FUNC_LV_MAIN);

	return idx;
}

void ppm_main_clear_client_req(struct ppm_client_req *c_req)
{
	int i;

	for (i = 0; i < c_req->cluster_num; i++) {
		c_req->cpu_limit[i].min_cpufreq_idx = get_cluster_min_cpufreq_idx(i);
		c_req->cpu_limit[i].max_cpufreq_idx = get_cluster_max_cpufreq_idx(i);
		c_req->cpu_limit[i].min_cpu_core = get_cluster_min_cpu_core(i);
		c_req->cpu_limit[i].max_cpu_core = get_cluster_max_cpu_core(i);
		c_req->cpu_limit[i].has_advise_freq = false;
		c_req->cpu_limit[i].advise_cpufreq_idx = -1;
		c_req->cpu_limit[i].has_advise_core = false;
		c_req->cpu_limit[i].advise_cpu_core = -1;
	}
}

void ppm_task_wakeup(void)
{
	FUNC_ENTER(FUNC_LV_MAIN);

#if 0
	ppm_lock(&ppm_main_info.lock);
	if (ppm_main_info.ppm_task) {
		atomic_set(&ppm_main_info.ppm_event, 1);
		wake_up(&ppm_main_info.ppm_wq);
	}
	ppm_unlock(&ppm_main_info.lock);
#else
	mt_ppm_main();
#endif

	FUNC_EXIT(FUNC_LV_MAIN);
}

int ppm_main_register_policy(struct ppm_policy_data *policy)
{
	struct list_head *pos;
	int ret = 0, i;

	FUNC_ENTER(FUNC_LV_MAIN);

	ppm_lock(&ppm_main_info.lock);

	/* init remaining members in policy data */
	policy->req.limit = kcalloc(ppm_main_info.cluster_num, sizeof(*policy->req.limit), GFP_KERNEL);
	if (!policy->req.limit) {
		ret = -ENOMEM;
		goto out;
	}
	policy->req.cluster_num = ppm_main_info.cluster_num;
	policy->req.power_budget = 0;
	policy->req.perf_idx = 0;
	/* init default limit */
	for (i = 0; i < policy->req.cluster_num; i++) {
		policy->req.limit[i].min_cpufreq_idx = get_cluster_min_cpufreq_idx(i);
		policy->req.limit[i].max_cpufreq_idx = get_cluster_max_cpufreq_idx(i);
		policy->req.limit[i].min_cpu_core = get_cluster_min_cpu_core(i);
		policy->req.limit[i].max_cpu_core = get_cluster_max_cpu_core(i);
	}

	/* insert into global policy_list according to its priority */
	list_for_each(pos, &ppm_main_info.policy_list) {
		struct ppm_policy_data *data;

		data = list_entry(pos, struct ppm_policy_data, link);
		if (policy->priority > data->priority  ||
			(policy->priority == data->priority && policy->policy > data->policy))
			break;
	}
	list_add_tail(&policy->link, pos);

	policy->is_enabled = true;

out:
	ppm_unlock(&ppm_main_info.lock);

	FUNC_ENTER(FUNC_LV_MAIN);

	return ret;
}

void ppm_main_unregister_policy(struct ppm_policy_data *policy)
{
	FUNC_ENTER(FUNC_LV_MAIN);

	ppm_lock(&ppm_main_info.lock);
	kfree(policy->req.limit);
	list_del(&policy->link);
	policy->is_enabled = false;
	policy->is_activated = false;
	ppm_unlock(&ppm_main_info.lock);

	FUNC_EXIT(FUNC_LV_MAIN);
}

static void ppm_main_update_limit(struct ppm_policy_data *p,
			struct ppm_client_limit *c_limit, struct ppm_cluster_limit *p_limit)
{
	FUNC_ENTER(FUNC_LV_MAIN);

	ppm_ver("Policy --> (%d)(%d)(%d)(%d)\n", p_limit->min_cpufreq_idx,
		p_limit->max_cpufreq_idx, p_limit->min_cpu_core, p_limit->max_cpu_core);
	ppm_ver("Original --> (%d)(%d)(%d)(%d) (%d)(%d)(%d)(%d)\n",
		c_limit->min_cpufreq_idx, c_limit->max_cpufreq_idx, c_limit->min_cpu_core,
		c_limit->max_cpu_core, c_limit->has_advise_freq, c_limit->advise_cpufreq_idx,
		c_limit->has_advise_core, c_limit->advise_cpu_core);

	switch (p->policy) {
	/* fix freq */
	case PPM_POLICY_PTPOD:
		c_limit->has_advise_freq = true;
		c_limit->advise_cpufreq_idx = p_limit->max_cpufreq_idx;
		break;
	/* fix freq and core */
	case PPM_POLICY_UT:
		if (p_limit->min_cpufreq_idx == p_limit->max_cpufreq_idx) {
			c_limit->has_advise_freq = true;
			c_limit->advise_cpufreq_idx = p_limit->max_cpufreq_idx;
			c_limit->min_cpufreq_idx = c_limit->max_cpufreq_idx = p_limit->max_cpufreq_idx;
		}

		if (p_limit->min_cpu_core == p_limit->max_cpu_core) {
			c_limit->has_advise_core = true;
			c_limit->advise_cpu_core = p_limit->max_cpu_core;
			c_limit->min_cpu_core = c_limit->max_cpu_core = p_limit->max_cpu_core;
		}
		break;
	case PPM_POLICY_LCM_OFF:
	case PPM_POLICY_DLPT:
	case PPM_POLICY_PWR_THRO:
	case PPM_POLICY_THERMAL:
	case PPM_POLICY_PERF_SERV:
	case PPM_POLICY_USER_LIMIT:
		/* out of range! use policy's min/max cpufreq idx setting */
		if (c_limit->min_cpufreq_idx <  p_limit->max_cpufreq_idx ||
			c_limit->max_cpufreq_idx >  p_limit->min_cpufreq_idx) {
			c_limit->min_cpufreq_idx = p_limit->min_cpufreq_idx;
			c_limit->max_cpufreq_idx = p_limit->max_cpufreq_idx;
		} else {
			c_limit->min_cpufreq_idx = MIN(c_limit->min_cpufreq_idx, p_limit->min_cpufreq_idx);
			c_limit->max_cpufreq_idx = MAX(c_limit->max_cpufreq_idx, p_limit->max_cpufreq_idx);
		}

		/* out of range! use policy's min/max cpu core setting */
		if (c_limit->min_cpu_core >  p_limit->max_cpu_core ||
			c_limit->max_cpu_core <  p_limit->min_cpu_core) {
			c_limit->min_cpu_core = p_limit->min_cpu_core;
			c_limit->max_cpu_core = p_limit->max_cpu_core;
		} else {
			c_limit->min_cpu_core = MAX(c_limit->min_cpu_core, p_limit->min_cpu_core);
			c_limit->max_cpu_core = MIN(c_limit->max_cpu_core, p_limit->max_cpu_core);
		}

		/* clear previous advise if it is not in current limit range */
		if (c_limit->has_advise_freq &&
			(c_limit->advise_cpufreq_idx > c_limit->min_cpufreq_idx ||
			c_limit->advise_cpufreq_idx < c_limit->max_cpufreq_idx)) {
			c_limit->has_advise_freq = false;
			c_limit->advise_cpufreq_idx = -1;
		}
		if (c_limit->has_advise_core &&
			(c_limit->advise_cpu_core < c_limit->min_cpu_core ||
			c_limit->advise_cpu_core > c_limit->max_cpu_core)) {
			c_limit->has_advise_core = false;
			c_limit->advise_cpu_core = -1;
		}

		break;
	/* base */
	case PPM_POLICY_HICA:
	default:
		c_limit->min_cpufreq_idx = p_limit->min_cpufreq_idx;
		c_limit->max_cpufreq_idx = p_limit->max_cpufreq_idx;
		c_limit->min_cpu_core = p_limit->min_cpu_core;
		c_limit->max_cpu_core = p_limit->max_cpu_core;
		/* reset advise */
		c_limit->has_advise_freq = false;
		c_limit->advise_cpufreq_idx = -1;
		c_limit->has_advise_core = false;
		c_limit->advise_cpu_core = -1;
		break;
	}

	/* error check */
	if (c_limit->min_cpu_core > c_limit->max_cpu_core) {
		ppm_warn("mismatch! min_core(%d) > max_core(%d)\n",
			c_limit->min_cpu_core, c_limit->max_cpu_core);
		c_limit->min_cpu_core = c_limit->max_cpu_core;
	}
	if (c_limit->min_cpufreq_idx < c_limit->max_cpufreq_idx) {
		ppm_warn("mismatch! min_freq_idx(%d) < max_freq_idx(%d)\n",
			c_limit->min_cpufreq_idx, c_limit->max_cpufreq_idx);
		c_limit->min_cpufreq_idx = c_limit->max_cpufreq_idx;
	}

	ppm_ver("Result --> (%d)(%d)(%d)(%d) (%d)(%d)(%d)(%d)\n",
		c_limit->min_cpufreq_idx, c_limit->max_cpufreq_idx, c_limit->min_cpu_core,
		c_limit->max_cpu_core, c_limit->has_advise_freq, c_limit->advise_cpufreq_idx,
		c_limit->has_advise_core, c_limit->advise_cpu_core);

	FUNC_EXIT(FUNC_LV_MAIN);
}

static void ppm_main_calc_new_limit(void)
{
	struct ppm_policy_data *pos;
	int i, active_cnt = 0;
	struct ppm_client_req *c_req = &(ppm_main_info.client_req);

	FUNC_ENTER(FUNC_LV_MAIN);

	/* traverse policy list to get the final limit to client */
	list_for_each_entry(pos, &ppm_main_info.policy_list, link) {
		ppm_lock(&pos->lock);

		if ((pos->is_enabled && pos->is_activated) || pos->policy == PPM_POLICY_HICA) {
			for_each_ppm_clusters(i) {
				ppm_ver("@%s: applying policy %s cluster %d limit...\n", __func__, pos->name, i);
				ppm_main_update_limit(pos,
					&c_req->cpu_limit[i], &pos->req.limit[i]);
			}

			active_cnt++;
		}

		ppm_unlock(&pos->lock);
	}

	/* send default limit to client */
	if (active_cnt == 0)
		ppm_main_clear_client_req(c_req);

	/* set freq idx to -1 if nr_cpu in the cluster is 0 */
	for (i = 0; i < c_req->cluster_num; i++) {
		if ((!c_req->cpu_limit[i].min_cpu_core && !c_req->cpu_limit[i].max_cpu_core)
			|| (c_req->cpu_limit[i].has_advise_core && !c_req->cpu_limit[i].advise_cpu_core)) {
			c_req->cpu_limit[i].min_cpufreq_idx = -1;
			c_req->cpu_limit[i].max_cpufreq_idx = -1;
			c_req->cpu_limit[i].has_advise_freq = false;
			c_req->cpu_limit[i].advise_cpufreq_idx = -1;
		}

		ppm_ver("Final Result: [%d] --> (%d)(%d)(%d)(%d) (%d)(%d)(%d)(%d)\n",
			i,
			c_req->cpu_limit[i].min_cpufreq_idx,
			c_req->cpu_limit[i].max_cpufreq_idx,
			c_req->cpu_limit[i].min_cpu_core,
			c_req->cpu_limit[i].max_cpu_core,
			c_req->cpu_limit[i].has_advise_freq,
			c_req->cpu_limit[i].advise_cpufreq_idx,
			c_req->cpu_limit[i].has_advise_core,
			c_req->cpu_limit[i].advise_cpu_core
		);
	}

	/* fill root cluster */
	c_req->root_cluster = ppm_get_root_cluster_by_state(ppm_main_info.cur_power_state);

	FUNC_EXIT(FUNC_LV_MAIN);
}

static enum ppm_power_state ppm_main_hica_state_decision(void)
{
	enum ppm_power_state cur_hica_state = ppm_hica_get_cur_state();
	enum ppm_power_state final_state;
	struct ppm_policy_data *pos;

	FUNC_ENTER(FUNC_LV_MAIN);

	ppm_ver("@%s: Current state = %s\n", __func__, ppm_get_power_state_name(cur_hica_state));

	final_state = cur_hica_state;

	ppm_main_info.min_power_budget = ~0;

	/* For power budget related policy: find the min power budget */
	/* For other policies: use callback to decide the state for each policy */
	list_for_each_entry(pos, &ppm_main_info.policy_list, link) {
		ppm_lock(&pos->lock);
		if (pos->is_activated) {
			switch (pos->policy) {
			case PPM_POLICY_THERMAL:
			case PPM_POLICY_DLPT:
			case PPM_POLICY_PWR_THRO:
				if (pos->req.power_budget)
					ppm_main_info.min_power_budget =
						MIN(pos->req.power_budget, ppm_main_info.min_power_budget);
				break;
			case PPM_POLICY_PTPOD:
				/* skip power budget related policy check if PTPOD policy is activated */
				if (pos->get_power_state_cb) {
					final_state = pos->get_power_state_cb(cur_hica_state);
					ppm_unlock(&pos->lock);
					goto skip_pwr_check;
				}
				break;
			default:
				/* overwrite original HICA state */
				if (pos->get_power_state_cb) {
					enum ppm_power_state state = pos->get_power_state_cb(cur_hica_state);

					if (state != cur_hica_state)
						final_state = state;
				}
				break;
			}
		}
		ppm_unlock(&pos->lock);
	}

	ppm_ver("@%s: final state (before) = %s, min_power_budget = %d\n",
		__func__, ppm_get_power_state_name(final_state), ppm_main_info.min_power_budget);

	/* decide final state */
	if (ppm_main_info.min_power_budget != ~0) {
		enum ppm_power_state state_by_pwr_idx =
			ppm_hica_get_state_by_pwr_budget(final_state, ppm_main_info.min_power_budget);
		final_state = MIN(final_state, state_by_pwr_idx);
	}

skip_pwr_check:
	ppm_ver("@%s: final state (after) = %s\n", __func__, ppm_get_power_state_name(final_state));

	FUNC_EXIT(FUNC_LV_MAIN);

	return final_state;
}

int mt_ppm_main(void)
{
	struct ppm_policy_data *pos;
	struct ppm_client_req *c_req = &(ppm_main_info.client_req);
	struct ppm_client_req *last_req = &(ppm_main_info.last_req);
	enum ppm_power_state prev_state;
	enum ppm_power_state next_state;
	int i;

	FUNC_ENTER(FUNC_LV_MAIN);

	ppm_lock(&ppm_main_info.lock);
	/* atomic_set(&ppm_main_info.ppm_event, 0); */

	if (!ppm_main_info.is_enabled || ppm_main_info.is_in_suspend)
		goto end;

	prev_state = ppm_main_info.cur_power_state;

	/* select new state */
	next_state = ppm_main_hica_state_decision();
	ppm_main_info.cur_power_state = next_state;

	/* update active policy's limit according to current state */
	list_for_each_entry(pos, &ppm_main_info.policy_list, link) {
		if (pos->is_activated && pos->update_limit_cb) {
			ppm_lock(&pos->lock);
			pos->update_limit_cb(next_state);
			ppm_unlock(&pos->lock);
		}
	}

	/* calculate final limit and fill-in client request structure */
	ppm_main_calc_new_limit();

	/* notify client and print debug message if limits are changed */
	if (memcmp(c_req->cpu_limit, last_req->cpu_limit,
		ppm_main_info.cluster_num * sizeof(*c_req->cpu_limit))) {
		char buf[128];
		char *ptr = buf;

		/* send request to client */
		for_each_ppm_clients(i) {
			if (ppm_main_info.client_info[i].limit_cb)
				ppm_main_info.client_info[i].limit_cb(*c_req);
		}

		/* print debug message */
		if (prev_state != next_state)
			ptr += sprintf(ptr, "[%s]->[%s]: ", ppm_get_power_state_name(prev_state),
						ppm_get_power_state_name(next_state));
		else
			ptr += sprintf(ptr, "[%s]: ", ppm_get_power_state_name(next_state));

		for (i = 0; i < c_req->cluster_num; i++) {
			ptr += sprintf(ptr, "(%d)(%d)(%d)(%d) ",
				c_req->cpu_limit[i].min_cpufreq_idx,
				c_req->cpu_limit[i].max_cpufreq_idx,
				c_req->cpu_limit[i].min_cpu_core,
				c_req->cpu_limit[i].max_cpu_core
			);

			if (c_req->cpu_limit[i].has_advise_freq || c_req->cpu_limit[i].has_advise_core)
				ptr += sprintf(ptr, "[(%d)(%d)(%d)(%d)] ",
					c_req->cpu_limit[i].has_advise_freq,
					c_req->cpu_limit[i].advise_cpufreq_idx,
					c_req->cpu_limit[i].has_advise_core,
					c_req->cpu_limit[i].advise_cpu_core
				);
		}

		ppm_dbg("(%d)%s\n", c_req->root_cluster, buf);

		memcpy(last_req->cpu_limit, c_req->cpu_limit,
			ppm_main_info.cluster_num * sizeof(*c_req->cpu_limit));
	}

#if PPM_UPDATE_STATE_DIRECT_TO_MET
	if (NULL != g_pSet_PPM_State && prev_state != next_state)
		g_pSet_PPM_State((unsigned int)next_state);
#endif

end:
	ppm_unlock(&ppm_main_info.lock);

	FUNC_EXIT(FUNC_LV_MAIN);

	return 0;
}

#if 0
static int ppm_main_task(void *data)
{
	return 0;
	struct ppm_policy_data *pos;

	while (1) {
		if (ppm_main_info.is_enabled) {
			enum ppm_power_state state;

			ppm_lock(&ppm_main_info.lock);

			atomic_set(&ppm_main_info.ppm_event, 0);

			/* select new state */
			state = ppm_main_hica_state_decision();
			ppm_main_info.cur_hica_state = state;

			/* update active policy's limit according to current state */
			list_for_each_entry(pos, &ppm_main_info.policy_list, link) {
				if (pos->is_activated)
					ppm_hica_update_limit_by_state(state, pos);
			}

			/* calculate final limit and send to clients */
			ppm_main_calc_and_send_new_limit();

			ppm_unlock(&ppm_main_info.lock);
		}

		wait_event_timeout(ppm_main_info.ppm_wq,
			atomic_read(&ppm_main_info.ppm_event) != 0,
			msecs_to_jiffies(PPM_TIMER_INTERVAL_MS));

		if (kthread_should_stop())
			break;
	}

	return 0;
}
#endif

static void ppm_main_send_request_for_suspend(void)
{
	struct ppm_client_req *c_req = &(ppm_main_info.client_req);
	struct ppm_client_req *last_req = &(ppm_main_info.last_req);
	int i;

	FUNC_ENTER(FUNC_LV_MAIN);

	ppm_ver("@%s:\n", __func__);

	/* modify advise freq to DVFS */
	for (i = 0; i < c_req->cluster_num; i++) {
		c_req->cpu_limit[i].has_advise_freq = true;
		c_req->cpu_limit[i].advise_cpufreq_idx =
			ppm_main_freq_to_idx(i, get_cluster_suspend_fix_freq(i), CPUFREQ_RELATION_L);

		ppm_ver("Result: [%d] --> (%d)(%d)(%d)(%d) (%d)(%d)(%d)(%d)\n",
			i,
			c_req->cpu_limit[i].min_cpufreq_idx,
			c_req->cpu_limit[i].max_cpufreq_idx,
			c_req->cpu_limit[i].min_cpu_core,
			c_req->cpu_limit[i].max_cpu_core,
			c_req->cpu_limit[i].has_advise_freq,
			c_req->cpu_limit[i].advise_cpufreq_idx,
			c_req->cpu_limit[i].has_advise_core,
			c_req->cpu_limit[i].advise_cpu_core
		);
	}

	/* send request to DVFS only */
	for_each_ppm_clients(i) {
		if (ppm_main_info.client_info[i].limit_cb
			&& ppm_main_info.client_info[i].client == PPM_CLIENT_DVFS)
			ppm_main_info.client_info[i].limit_cb(*c_req);
	}

	memcpy(last_req->cpu_limit, c_req->cpu_limit,
		ppm_main_info.cluster_num * sizeof(*c_req->cpu_limit));

	ppm_info("send fix idx to DVFS before suspend!\n");

	FUNC_EXIT(FUNC_LV_MAIN);
}

static int ppm_main_suspend(struct device *dev)
{
	FUNC_ENTER(FUNC_LV_MODULE);

	ppm_info("%s: suspend callback in\n", __func__);

	ppm_lock(&ppm_main_info.lock);
	ppm_main_send_request_for_suspend();
	ppm_main_info.is_in_suspend = true;
	ppm_unlock(&ppm_main_info.lock);

	FUNC_EXIT(FUNC_LV_MODULE);

	return 0;
}

static int ppm_main_resume(struct device *dev)
{
	FUNC_ENTER(FUNC_LV_MODULE);

	ppm_info("%s: resume callback in\n", __func__);

	ppm_lock(&ppm_main_info.lock);
	ppm_main_info.is_in_suspend = false;
	ppm_unlock(&ppm_main_info.lock);

	FUNC_EXIT(FUNC_LV_MODULE);

	return 0;
}

static int ppm_main_data_init(void)
{
	struct cpumask cpu_mask;
#if 0
	struct sched_param param = { .sched_priority = MAX_RT_PRIO-5 };
#endif
	int ret = 0;
	int i;

	FUNC_ENTER(FUNC_LV_MAIN);

	/* init static power table */
#if 0
	mt_spower_init();
#endif

	/* get cluster num */
	ppm_main_info.cluster_num = (unsigned int)arch_get_nr_clusters();
	ppm_info("@%s: cluster_num = %d\n", __func__, ppm_main_info.cluster_num);

	/* init cluster info (DVFS table will be updated after DVFS driver registered) */
	ppm_main_info.cluster_info =
		kzalloc(ppm_main_info.cluster_num * sizeof(*ppm_main_info.cluster_info), GFP_KERNEL);
	if (!ppm_main_info.cluster_info) {
		ppm_err("@%s: fail to allocate memory for cluster_info!\n", __func__);
		ret = -ENOMEM;
		goto out;
	}
	for_each_ppm_clusters(i) {
		ppm_main_info.cluster_info[i].cluster_id = i;
		/* OPP num will update after DVFS set table */
		ppm_main_info.cluster_info[i].dvfs_opp_num = DVFS_OPP_NUM;

		/* get topology info */
		arch_get_cluster_cpus(&cpu_mask, i);
		ppm_main_info.cluster_info[i].core_num = cpumask_weight(&cpu_mask);
		ppm_main_info.cluster_info[i].cpu_id = cpumask_first(&cpu_mask);
		ppm_info("@%s: ppm cluster %d -> core_num = %d, cpu_id = %d\n",
				__func__,
				ppm_main_info.cluster_info[i].cluster_id,
				ppm_main_info.cluster_info[i].core_num,
				ppm_main_info.cluster_info[i].cpu_id
				);
	}

	/* init client request */
	ppm_main_info.client_req.cpu_limit =
		kzalloc(ppm_main_info.cluster_num * sizeof(*ppm_main_info.client_req.cpu_limit), GFP_KERNEL);
	if (!ppm_main_info.client_req.cpu_limit) {
		ppm_err("@%s: fail to allocate memory client_req!\n", __func__);
		ret = -ENOMEM;
		goto allocate_req_mem_fail;
	}

	ppm_main_info.last_req.cpu_limit =
		kzalloc(ppm_main_info.cluster_num * sizeof(*ppm_main_info.last_req.cpu_limit), GFP_KERNEL);
	if (!ppm_main_info.last_req.cpu_limit) {
		ppm_err("@%s: fail to allocate memory for last_req!\n", __func__);
		kfree(ppm_main_info.client_req.cpu_limit);
		ret = -ENOMEM;
		goto allocate_req_mem_fail;
	}

	for_each_ppm_clusters(i) {
		ppm_main_info.client_req.cluster_num = ppm_main_info.cluster_num;
		ppm_main_info.client_req.root_cluster = 0;
		ppm_main_info.client_req.cpu_limit[i].cluster_id = i;
		ppm_main_info.client_req.cpu_limit[i].cpu_id = ppm_main_info.cluster_info[i].cpu_id;

		ppm_main_info.last_req.cluster_num = ppm_main_info.cluster_num;
	}
#if 0
	ppm_main_info.ppm_task = kthread_create(ppm_main_task, NULL, "ppm_main");
	if (IS_ERR(ppm_main_info.ppm_task)) {
		ppm_err("@%s: ppm task create failed\n", __func__);
		ret = PTR_ERR(ppm_main_info.ppm_task);
		goto task_create_fail;
	}

	sched_setscheduler_nocheck(ppm_main_info.ppm_task, SCHED_FIFO, &param);
	get_task_struct(ppm_main_info.ppm_task);
	wake_up_process(ppm_main_info.ppm_task);
	ppm_info("@%s: ppm task start success, pid: %d\n", __func__,
			ppm_main_info.ppm_task->pid);
#endif

	ppm_info("@%s: done!\n", __func__);

	FUNC_EXIT(FUNC_LV_MAIN);

	return ret;

#if 0
task_create_fail:
	kfree(ppm_main_info.client_req.cpu_limit);
	kfree(ppm_main_info.last_req.cpu_limit);
#endif

allocate_req_mem_fail:
	kfree(ppm_main_info.cluster_info);

out:
	FUNC_EXIT(FUNC_LV_MODULE);

	return ret;
}

static void ppm_main_data_deinit(void)
{
	struct ppm_policy_data *pos;

	FUNC_ENTER(FUNC_LV_MAIN);

	ppm_main_info.is_enabled = false;

	if (ppm_main_info.ppm_task) {
		kthread_stop(ppm_main_info.ppm_task);
		put_task_struct(ppm_main_info.ppm_task);
		ppm_main_info.ppm_task = NULL;
	}

	/* free policy req mem */
	list_for_each_entry(pos, &ppm_main_info.policy_list, link) {
		kfree(pos->req.limit);
	}

	kfree(ppm_main_info.client_req.cpu_limit);
	kfree(ppm_main_info.last_req.cpu_limit);
	kfree(ppm_main_info.cluster_info);

	FUNC_EXIT(FUNC_LV_MAIN);
}

static int ppm_main_pdrv_probe(struct platform_device *pdev)
{
	FUNC_ENTER(FUNC_LV_MODULE);

	ppm_info("@%s: ppm probe done!\n", __func__);

	FUNC_EXIT(FUNC_LV_MODULE);

	return 0;
}

static int ppm_main_pdrv_remove(struct platform_device *pdev)
{
	FUNC_ENTER(FUNC_LV_MODULE);

	FUNC_EXIT(FUNC_LV_MODULE);

	return 0;
}

static int __init ppm_main_init(void)
{
	int ret = 0;

	FUNC_ENTER(FUNC_LV_MODULE);

	/* ppm data init */
	ret = ppm_main_data_init();
	if (ret) {
		ppm_err("fail to init ppm data @ %s()\n", __func__);
		goto fail;
	}

#ifdef CONFIG_PROC_FS
	/* init proc */
	ret = ppm_procfs_init();
	if (ret) {
		ppm_err("fail to create ppm procfs @ %s()\n", __func__);
		goto fail;
	}
#endif /* CONFIG_PROC_FS */

	/* register platform device/driver */
	ret = platform_device_register(&ppm_main_info.ppm_pdev);
	if (ret) {
		ppm_err("fail to register ppm device @ %s()\n", __func__);
		goto fail;
	}

	ret = platform_driver_register(&ppm_main_info.ppm_pdrv);
	if (ret) {
		ppm_err("fail to register ppm driver @ %s()\n", __func__);
		goto reg_platform_driver_fail;
	}

	ppm_info("ppm driver init done!\n");

	return ret;


reg_platform_driver_fail:
	platform_device_unregister(&ppm_main_info.ppm_pdev);

fail:
	ppm_main_info.is_enabled = false;
	ppm_err("ppm driver init fail!\n");

	FUNC_EXIT(FUNC_LV_MODULE);

	return ret;
}

static void __exit ppm_main_exit(void)
{
	FUNC_ENTER(FUNC_LV_MODULE);

	platform_driver_unregister(&ppm_main_info.ppm_pdrv);
	platform_device_unregister(&ppm_main_info.ppm_pdev);

	ppm_main_data_deinit();

	FUNC_EXIT(FUNC_LV_MODULE);
}

arch_initcall(ppm_main_init);
module_exit(ppm_main_exit);

MODULE_DESCRIPTION("MediaTek PPM Driver v0.1");
MODULE_LICENSE("GPL");

