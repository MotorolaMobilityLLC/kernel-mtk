
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

static unsigned int ppm_dlpt_calc_trans_precentage(void);
static unsigned int ppm_dlpt_pwr_budget_preprocess(unsigned int budget);
#if PPM_HW_OCP_SUPPORT
static unsigned int ppm_dlpt_pwr_budget_postprocess(unsigned int budget, unsigned int pwr_idx);
#else
static unsigned int ppm_dlpt_pwr_budget_postprocess(unsigned int budget);
#endif
static void ppm_dlpt_update_limit_cb(enum ppm_power_state new_state);
static void ppm_dlpt_status_change_cb(bool enable);
static void ppm_dlpt_mode_change_cb(enum ppm_mode mode);

static unsigned int dlpt_percentage_to_real_power;
static enum PPM_DLPT_MODE dlpt_mode;

/* other members will init by ppm_main */
static struct ppm_policy_data dlpt_policy = {
	.name			= __stringify(PPM_POLICY_DLPT),
	.lock			= __MUTEX_INITIALIZER(dlpt_policy.lock),
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
	unsigned int budget = 0;
	int i;

	FUNC_ENTER(FUNC_LV_POLICY);

	/* find power budget in table, skip this round if idx not found in table */
	power_idx = ppm_find_pwr_idx(cluster_status);
	if (power_idx < 0)
		goto end;

#if PPM_HW_OCP_SUPPORT
	for (i = 0; i < cluster_num; i++) {
		int leakage, total, clkpct;

		total_core += cluster_status[i].core_num;
		max_volt = MAX(max_volt, cluster_status[i].volt);

		/* read power meter for total power calculation */
		if (cluster_status[i].core_num) {
			if (i == PPM_CLUSTER_B) {
				BigOCPCapture(1, 1, 0, 15);
				BigOCPCaptureStatus(&leakage, &total, &clkpct);
			} else {
				LittleOCPCapture(i, 1, 1, 0, 15);
				LittleOCPCaptureGet(i, &leakage, &total, &clkpct);
			}
			ppm_ver("ocp capture(%d): %d, %d, %d\n", i, leakage, total, clkpct);
			budget += total;
		}
	}
	budget = ppm_dlpt_pwr_budget_postprocess(budget, (unsigned int)power_idx);
#else
	for (i = 0; i < cluster_num; i++) {
		total_core += cluster_status[i].core_num;
		max_volt = MAX(max_volt, cluster_status[i].volt);
	}
	budget = ppm_dlpt_pwr_budget_postprocess((unsigned int)power_idx);
#endif

	ppm_ver("budget = %d(%d), total_core = %d, max_volt = %d\n",
		budget, power_idx, total_core, max_volt);

#ifndef DISABLE_PBM_FEATURE
	kicker_pbm_by_cpu(budget, total_core, max_volt);
#endif

end:
	FUNC_EXIT(FUNC_LV_POLICY);
}

void mt_ppm_dlpt_set_limit_by_pbm(unsigned int limited_power)
{
	unsigned int budget;

	FUNC_ENTER(FUNC_LV_POLICY);

	budget = ppm_dlpt_pwr_budget_preprocess(limited_power);

	ppm_ver("Get PBM notifier => budget = %d(%d)\n", budget, limited_power);

	ppm_lock(&dlpt_policy.lock);

	if (!dlpt_policy.is_enabled) {
		ppm_warn("@%s: dlpt policy is not enabled!\n", __func__);
		ppm_unlock(&dlpt_policy.lock);
		goto end;
	}

	switch (dlpt_mode) {
	case SW_MODE:
	case HYBRID_MODE:
#if PPM_HW_OCP_SUPPORT
		dlpt_policy.req.power_budget = (dlpt_mode == SW_MODE)
			? budget : ppm_set_ocp(budget, dlpt_percentage_to_real_power);
#else
		dlpt_policy.req.power_budget = budget;
#endif
		dlpt_policy.is_activated = (budget) ? true : false;
		ppm_unlock(&dlpt_policy.lock);
		ppm_task_wakeup();

		break;
	case HW_MODE:	/* TBD */
	default:
		break;
	}

end:
	FUNC_EXIT(FUNC_LV_POLICY);
}

static unsigned int ppm_dlpt_calc_trans_precentage(void)
{
	struct ppm_power_tbl_data power_table = ppm_get_power_table();
	unsigned int max_pwr_idx = power_table.power_tbl[0].power_idx;

	/* dvfs table is null means ppm doesn't know chip type now */
	/* return 100 to make default ratio is 1 and check real ratio next time */
	if (!ppm_main_info.cluster_info[0].dvfs_tbl)
		return 100;

	dlpt_percentage_to_real_power = (ppm_main_info.dvfs_tbl_type == DVFS_TABLE_TYPE_FY)
		? (max_pwr_idx * 100 + (DLPT_MAX_REAL_POWER_FY - 1)) / DLPT_MAX_REAL_POWER_FY
		: (max_pwr_idx * 100 + (DLPT_MAX_REAL_POWER_SB - 1)) / DLPT_MAX_REAL_POWER_SB;

	return dlpt_percentage_to_real_power;
}

static unsigned int ppm_dlpt_pwr_budget_preprocess(unsigned int budget)
{
	unsigned int percentage = dlpt_percentage_to_real_power;

	if (!percentage)
		percentage = ppm_dlpt_calc_trans_precentage();

	return (budget * percentage + (100 - 1)) / 100;
}


#if PPM_HW_OCP_SUPPORT
static unsigned int ppm_dlpt_pwr_budget_postprocess(unsigned int budget, unsigned int pwr_idx)
{
	/* just calculate new ratio */
	dlpt_percentage_to_real_power = (pwr_idx * 100 + (budget - 1)) / budget;
	ppm_ver("new dlpt ratio = %d (%d/%d)\n", dlpt_percentage_to_real_power, pwr_idx, budget);

	return budget;
}
#else
static unsigned int ppm_dlpt_pwr_budget_postprocess(unsigned int budget)
{
	unsigned int percentage = dlpt_percentage_to_real_power;

	if (!percentage)
		percentage = ppm_dlpt_calc_trans_precentage();

	return (budget * 100 + (percentage - 1)) / percentage;
}
#endif

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

static int ppm_dlpt_budget_trans_percentage_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "trans_percentage = %d\n", dlpt_percentage_to_real_power);
	seq_printf(m, "PPM DLPT activate = %d\n", dlpt_policy.is_activated);

	return 0;
}

static ssize_t ppm_dlpt_budget_trans_percentage_proc_write(struct file *file, const char __user *buffer,
					size_t count, loff_t *pos)
{
	unsigned int percentage;

	char *buf = ppm_copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	if (!kstrtouint(buf, 10, &percentage)) {
		if (!percentage)
			ppm_err("@%s: percentage should not be 0!\n", __func__);
		else
			dlpt_percentage_to_real_power = percentage;
	} else
		ppm_err("@%s: Invalid input!\n", __func__);

	free_page((unsigned long)buf);
	return count;
}

PROC_FOPS_RW(dlpt_limit);
PROC_FOPS_RW(dlpt_budget_trans_percentage);

static int __init ppm_dlpt_policy_init(void)
{
	int i, ret = 0;

	struct pentry {
		const char *name;
		const struct file_operations *fops;
	};

	const struct pentry entries[] = {
		PROC_ENTRY(dlpt_limit),
		PROC_ENTRY(dlpt_budget_trans_percentage),
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

#if PPM_HW_OCP_SUPPORT
	/* enable OCP */
	/* TBD: move these to cpu hotplug? */
#if 0
	BigiDVFSEnable(2500, 110000, 120000);	/* idvfs enable 2500MHz, Vproc_x100mv, Vsram_x100mv */
	BigiDVFSChannel(1, 1);			/* ocp channel enable */
	BigOCPConfig(300, 10000);		/* cluster 2 Voffset=0.3v_x1000, Vstep=10mv_x1000000 */
	BigOCPSetTarget(3, 127000);		/* cluster 2 set OCPI/FPI, Target=127 W */
	BigOCPEnable(3, 1, 625, 0);		/* cluster 2 set OCPI/FPI, Target_unit=mW, CG=6.25%_x100 */
#endif
	LittleOCPConfig(300, 10000);		/* cluster 0/1 Voffset=0.5v_x1000, Vstep=6.25mv_x1000000 */
	LittleOCPSetTarget(0, 127000);		/* cluster 0 Target=127W_x1000 */
	LittleOCPEnable(0, 1, 625);		/* cluster 0 Target_unit=mW, CG=6.25_x100 */
	LittleOCPSetTarget(1, 127000);		/* cluster 1 Target=127W_x1000 */
	LittleOCPEnable(1, 1, 625);		/* cluster 1 Target_unit=mW, CG=6.25_x100 */
#endif

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
#if PPM_HW_OCP_SUPPORT
/* should not init before OCP driver */
late_initcall(ppm_dlpt_policy_init);
#else
module_init(ppm_dlpt_policy_init);
#endif
module_exit(ppm_dlpt_policy_exit);

