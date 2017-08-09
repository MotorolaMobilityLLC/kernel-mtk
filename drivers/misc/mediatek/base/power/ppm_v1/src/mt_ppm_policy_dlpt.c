
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <asm/uaccess.h>

#include "mt_ppm_internal.h"
#include "mt_ppm_platform.h"
#include "mach/mt_pbm.h"


enum PPM_DLPT_MODE {
	SW_MODE,	/* use SW DLPT only */
	HYBRID_MODE,	/* use SW DLPT + HW OCP */
	HW_MODE,	/* use HW OCP only */
};

static void ppm_dlpt_update_limit_cb(enum ppm_power_state new_state);
static void ppm_dlpt_status_change_cb(bool enable);
static void ppm_dlpt_mode_change_cb(enum ppm_mode mode);

static enum PPM_DLPT_MODE dlpt_mode;

/* other members will init by ppm_main */
static struct ppm_policy_data dlpt_policy = {
	.name			= __stringify(PPM_POLICY_DLPT),
	.policy			= PPM_POLICY_DLPT,
	.priority		= PPM_POLICY_PRIO_POWER_BUDGET_BASE,
	.get_power_state_cb	= NULL,	/* decide in ppm main via min power budget */
	.update_limit_cb	= ppm_dlpt_update_limit_cb,
	.status_change_cb	= ppm_dlpt_status_change_cb,
	.mode_change_cb		= ppm_dlpt_mode_change_cb,
};

void mt_ppm_dlpt_kick_PBM(struct ppm_cluster_status *cluster_status, unsigned int cluster_num)
{
	int power_idx;
	unsigned int total_core = 0;
	unsigned int max_volt = 0;
	int i;

	FUNC_ENTER(FUNC_LV_POLICY);

	/* find power budget in table, skip this round if idx not found in table */
	power_idx = ppm_find_pwr_idx(cluster_status);
	if (power_idx == -1)
		goto end;

	for (i = 0; i < cluster_num; i++) {
		total_core += cluster_status[i].core_num;
		max_volt = MAX(max_volt, cluster_status[i].volt);
	}

	ppm_ver("total_core = %d, max_volt = %d\n", total_core, max_volt);

#ifndef DISABLE_PBM_FEATURE
	kicker_pbm_by_cpu((unsigned int)power_idx, total_core, max_volt);
#endif

end:
	FUNC_EXIT(FUNC_LV_POLICY);
}

void mt_ppm_dlpt_set_limit_by_pbm(unsigned int limited_power)
{
	FUNC_ENTER(FUNC_LV_POLICY);

	ppm_ver("Get PBM notifier => limited_power = %d\n", limited_power);

	ppm_lock(&dlpt_policy.lock);

	switch (dlpt_mode) {
	case SW_MODE:
	case HYBRID_MODE:
#if PPM_HW_OCP_SUPPORT
		dlpt_policy.req.power_budget = (dlpt_mode == SW_MODE)
			? limited_power : ppm_set_ocp(limited_power);
#else
		dlpt_policy.req.power_budget = limited_power;
#endif

		if (dlpt_policy.is_enabled) {
			dlpt_policy.is_activated = (limited_power) ? true : false;
			ppm_unlock(&dlpt_policy.lock);
			ppm_task_wakeup();
		} else
			ppm_unlock(&dlpt_policy.lock);

		break;
	case HW_MODE:	/* TBD */
	default:
		break;
	}

	FUNC_EXIT(FUNC_LV_POLICY);
}

static void ppm_dlpt_update_limit_cb(enum ppm_power_state new_state)
{
	FUNC_ENTER(FUNC_LV_POLICY);

	ppm_ver("@%s: dlpt policy update limit for new state = %s\n",
		__func__, ppm_get_power_state_name(new_state));

	ppm_hica_set_default_limit_by_state(new_state, &dlpt_policy);

	/* update limit according to power budget */
	ppm_main_update_req_by_pwr(new_state, &dlpt_policy.req);

	FUNC_EXIT(FUNC_LV_POLICY);
}

static void ppm_dlpt_status_change_cb(bool enable)
{
	FUNC_ENTER(FUNC_LV_POLICY);

	ppm_ver("@%s: dlpt policy status changed to %d\n", __func__, enable);

	FUNC_EXIT(FUNC_LV_POLICY);
}

static void ppm_dlpt_mode_change_cb(enum ppm_mode mode)
{
	FUNC_ENTER(FUNC_LV_POLICY);

	ppm_ver("@%s: ppm mode changed to %d\n", __func__, mode);

	FUNC_EXIT(FUNC_LV_POLICY);
}

static int ppm_dlpt_limit_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "limited power = %d\n", dlpt_policy.req.power_budget);
	seq_printf(m, "PPM DLPT activate = %d\n", dlpt_policy.is_activated);

	return 0;
}

static ssize_t ppm_dlpt_limit_proc_write(struct file *file, const char __user *buffer,
					size_t count, loff_t *pos)
{
	unsigned int limited_power;

	char *buf = ppm_copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	if (!kstrtouint(buf, 10, &limited_power))
		mt_ppm_dlpt_set_limit_by_pbm(limited_power);
	else
		ppm_err("@%s: Invalid input!\n", __func__);

	free_page((unsigned long)buf);
	return count;
}

PROC_FOPS_RW(dlpt_limit);

static int __init ppm_dlpt_policy_init(void)
{
	int i, ret = 0;

	struct pentry {
		const char *name;
		const struct file_operations *fops;
	};

	const struct pentry entries[] = {
		PROC_ENTRY(dlpt_limit),
	};

	FUNC_ENTER(FUNC_LV_POLICY);

#ifdef DISABLE_DLPT_FEATURE
	goto out;
#endif

	/* create procfs */
	for (i = 0; i < ARRAY_SIZE(entries); i++) {
		if (!proc_create(entries[i].name, S_IRUGO | S_IWUSR | S_IWGRP, policy_dir, entries[i].fops)) {
			ppm_err("%s(), create /proc/ppm/policy/%s failed\n", __func__, entries[i].name);
			ret = -EINVAL;
			goto out;
		}
	}
	if (ppm_main_register_policy(&dlpt_policy)) {
		ppm_err("@%s: dlpt policy register failed\n", __func__);
		ret = -EINVAL;
		goto out;
	}

	dlpt_mode = PPM_DLPT_DEFAULT_MODE;

	ppm_info("@%s: register %s done! dlpt mode = %d\n", __func__, dlpt_policy.name, dlpt_mode);

out:
	FUNC_EXIT(FUNC_LV_POLICY);

	return ret;
}

static void __exit ppm_dlpt_policy_exit(void)
{
	FUNC_ENTER(FUNC_LV_POLICY);

	ppm_main_unregister_policy(&dlpt_policy);

	FUNC_EXIT(FUNC_LV_POLICY);
}

module_init(ppm_dlpt_policy_init);
module_exit(ppm_dlpt_policy_exit);

