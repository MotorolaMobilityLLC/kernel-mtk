
#include "mt_ppm_internal.h"

/* APIs */
void mt_ppm_set_dvfs_table(unsigned int cpu, struct cpufreq_frequency_table *tbl,
	unsigned int num, enum dvfs_table_type type)
{
	int i, j;

	FUNC_ENTER(FUNC_LV_API);

	for_each_ppm_clusters(i) {
		if (ppm_main_info.cluster_info[i].cpu_id == cpu) {
			/* return if table is existed */
			if (ppm_main_info.cluster_info[i].dvfs_tbl)
				return;

			ppm_lock(&ppm_main_info.lock);

			ppm_main_info.dvfs_tbl_type = type;
			ppm_main_info.cluster_info[i].dvfs_tbl = tbl;
			ppm_main_info.cluster_info[i].dvfs_opp_num = num;
			/* dump dvfs table */
			ppm_info("DVFS table type = %d\n", type);
			ppm_info("DVFS table of cluster %d:\n", ppm_main_info.cluster_info[i].cluster_id);
			for (j = 0; j < num; j++)
				ppm_info("%d: %d KHz\n", j, ppm_main_info.cluster_info[i].dvfs_tbl[j].frequency);

			ppm_unlock(&ppm_main_info.lock);

			FUNC_EXIT(FUNC_LV_API);
			return;
		}
	}

	if (i == ppm_main_info.cluster_num)
		ppm_err("@%s: cpu_id(%d) not found!\n", __func__, cpu);

	FUNC_EXIT(FUNC_LV_API);
}

void mt_ppm_register_client(enum ppm_client client, void (*limit)(struct ppm_client_req req))
{
	FUNC_ENTER(FUNC_LV_API);

	ppm_lock(&ppm_main_info.lock);

	/* init client */
	ppm_main_info.client_info[client].client = client;
	ppm_main_info.client_info[client].limit_cb = limit;

	ppm_unlock(&ppm_main_info.lock);

	FUNC_EXIT(FUNC_LV_API);
}

met_set_ppm_state_funcMET g_pSet_PPM_State;

void mt_set_ppm_state_registerCB(met_set_ppm_state_funcMET pCB)
{
	g_pSet_PPM_State = pCB;
}
EXPORT_SYMBOL(mt_set_ppm_state_registerCB);

