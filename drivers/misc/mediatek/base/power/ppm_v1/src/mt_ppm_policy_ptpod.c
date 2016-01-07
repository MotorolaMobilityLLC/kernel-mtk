
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>

#include "mt_ppm_internal.h"


static enum ppm_power_state ppm_ptpod_get_power_state_cb(enum ppm_power_state cur_state);
static void ppm_ptpod_update_limit_cb(enum ppm_power_state new_state);
static void ppm_ptpod_status_change_cb(bool enable);
static void ppm_ptpod_mode_change_cb(enum ppm_mode mode);

/* other members will init by ppm_main */
static struct ppm_policy_data ptpod_policy = {
	.name			= __stringify(PPM_POLICY_PTPOD),
	.lock			= __MUTEX_INITIALIZER(ptpod_policy.lock),
	.policy			= PPM_POLICY_PTPOD,
	.priority		= PPM_POLICY_PRIO_HIGHEST,
	.get_power_state_cb	= ppm_ptpod_get_power_state_cb,
	.update_limit_cb	= ppm_ptpod_update_limit_cb,
	.status_change_cb	= ppm_ptpod_status_change_cb,
	.mode_change_cb		= ppm_ptpod_mode_change_cb,
};


void mt_ppm_ptpod_policy_activate(void)
{
	unsigned int i;

	FUNC_ENTER(FUNC_LV_API);

	ppm_lock(&ptpod_policy.lock);

	if (!ptpod_policy.is_enabled) {
		ppm_warn("@%s: ptpod policy is not enabled!\n", __func__);
		ppm_unlock(&ptpod_policy.lock);
		goto end;
	}

	ptpod_policy.is_activated = true;
	for (i = 0; i < ptpod_policy.req.cluster_num; i++) {
		/* FREQ is the same for each cluster? */
		ptpod_policy.req.limit[i].min_cpufreq_idx =
			get_cluster_ptpod_fix_freq_idx(i);
		ptpod_policy.req.limit[i].max_cpufreq_idx =
			get_cluster_ptpod_fix_freq_idx(i);
	}
	ppm_unlock(&ptpod_policy.lock);
	ppm_task_wakeup();

end:
	FUNC_EXIT(FUNC_LV_API);
}

void mt_ppm_ptpod_policy_deactivate(void)
{
	unsigned int i;

	FUNC_ENTER(FUNC_LV_API);

	ppm_lock(&ptpod_policy.lock);

	/* deactivate ptpod policy */
	if (ptpod_policy.is_activated) {
		ptpod_policy.is_activated = false;

		/* restore to default setting */
		for (i = 0; i < ptpod_policy.req.cluster_num; i++) {
			ptpod_policy.req.limit[i].min_cpufreq_idx = get_cluster_min_cpufreq_idx(i);
			ptpod_policy.req.limit[i].max_cpufreq_idx = get_cluster_max_cpufreq_idx(i);
		}

		ppm_unlock(&ptpod_policy.lock);

		ppm_task_wakeup();
	} else
		ppm_unlock(&ptpod_policy.lock);

	FUNC_EXIT(FUNC_LV_API);
}

static enum ppm_power_state ppm_ptpod_get_power_state_cb(enum ppm_power_state cur_state)
{
	return PPM_POWER_STATE_4LL_L;
}

static void ppm_ptpod_update_limit_cb(enum ppm_power_state new_state)
{
	FUNC_ENTER(FUNC_LV_POLICY);

	ppm_ver("@%s: ptpod policy update limit for new state = %s\n",
		__func__, ppm_get_power_state_name(new_state));

	FUNC_EXIT(FUNC_LV_POLICY);
}

static void ppm_ptpod_status_change_cb(bool enable)
{
	FUNC_ENTER(FUNC_LV_POLICY);

	ppm_ver("@%s: ptpod policy status changed to %d\n", __func__, enable);

	FUNC_EXIT(FUNC_LV_POLICY);
}

static void ppm_ptpod_mode_change_cb(enum ppm_mode mode)
{
	FUNC_ENTER(FUNC_LV_POLICY);

	ppm_ver("@%s: ppm mode changed to %d\n", __func__, mode);

	FUNC_EXIT(FUNC_LV_POLICY);
}

static int __init ppm_ptpod_policy_init(void)
{
	int ret = 0;

	FUNC_ENTER(FUNC_LV_POLICY);

	if (ppm_main_register_policy(&ptpod_policy)) {
		ppm_err("@%s: ptpod policy register failed\n", __func__);
		ret = -EINVAL;
		goto out;
	}

	ppm_info("@%s: register %s done!\n", __func__, ptpod_policy.name);

out:
	FUNC_EXIT(FUNC_LV_POLICY);

	return ret;
}

static void __exit ppm_ptpod_policy_exit(void)
{
	FUNC_ENTER(FUNC_LV_POLICY);

	ppm_main_unregister_policy(&ptpod_policy);

	FUNC_EXIT(FUNC_LV_POLICY);
}

module_init(ppm_ptpod_policy_init);
module_exit(ppm_ptpod_policy_exit);

