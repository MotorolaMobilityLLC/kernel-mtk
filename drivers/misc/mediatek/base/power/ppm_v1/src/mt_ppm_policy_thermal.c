
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <asm/uaccess.h>

#include "mt_ppm_internal.h"


static void ppm_thermal_update_limit_cb(enum ppm_power_state new_state);
static void ppm_thermal_status_change_cb(bool enable);
static void ppm_thermal_mode_change_cb(enum ppm_mode mode);

/* other members will init by ppm_main */
static struct ppm_policy_data thermal_policy = {
	.name			= __stringify(PPM_POLICY_THERMAL),
	.lock			= __MUTEX_INITIALIZER(thermal_policy.lock),
	.policy			= PPM_POLICY_THERMAL,
	.priority		= PPM_POLICY_PRIO_POWER_BUDGET_BASE,
	.get_power_state_cb	= NULL,	/* decide in ppm main via min power budget */
	.update_limit_cb	= ppm_thermal_update_limit_cb,
	.status_change_cb	= ppm_thermal_status_change_cb,
	.mode_change_cb		= ppm_thermal_mode_change_cb,
};

void mt_ppm_cpu_thermal_protect(unsigned int limited_power)
{
	FUNC_ENTER(FUNC_LV_POLICY);

	ppm_info("Get budget from thermal => limited_power = %d\n", limited_power);

	ppm_lock(&thermal_policy.lock);

	if (!thermal_policy.is_enabled) {
		ppm_warn("@%s: thermal policy is not enabled!\n", __func__);
		ppm_unlock(&thermal_policy.lock);
		goto end;
	}

	thermal_policy.req.power_budget = limited_power;
	thermal_policy.is_activated = (limited_power) ? true : false;
	ppm_unlock(&thermal_policy.lock);
	ppm_task_wakeup();

end:
	FUNC_EXIT(FUNC_LV_POLICY);
}

unsigned int mt_ppm_thermal_get_min_power(void)
{
	struct ppm_power_tbl_data power_table = ppm_get_power_table();
	unsigned int size = power_table.nr_power_tbl;

	return power_table.power_tbl[size - 1].power_idx;
}

unsigned int mt_ppm_thermal_get_max_power(void)
{
	return ppm_get_power_table().power_tbl[0].power_idx;
}

unsigned int mt_ppm_thermal_get_cur_power(void)
{
	unsigned int budget, idx;
	enum ppm_power_state cur_state;
	struct ppm_power_tbl_data power_table = ppm_get_power_table();

	ppm_lock(&ppm_main_info.lock);
	budget = ppm_main_info.min_power_budget;
	cur_state = ppm_main_info.cur_power_state;
	ppm_unlock(&ppm_main_info.lock);

	idx = ppm_hica_get_table_idx_by_pwr(cur_state, budget);

	return power_table.power_tbl[idx].power_idx;
}

static void ppm_thermal_update_limit_cb(enum ppm_power_state new_state)
{
	FUNC_ENTER(FUNC_LV_POLICY);

	ppm_ver("@%s: thermal policy update limit for new state = %s\n",
		__func__, ppm_get_power_state_name(new_state));

	ppm_hica_set_default_limit_by_state(new_state, &thermal_policy);

	/* update limit according to power budget */
	ppm_main_update_req_by_pwr(new_state, &thermal_policy.req);

	FUNC_EXIT(FUNC_LV_POLICY);
}

static void ppm_thermal_status_change_cb(bool enable)
{
	FUNC_ENTER(FUNC_LV_POLICY);

	ppm_ver("@%s: thermal policy status changed to %d\n", __func__, enable);

	FUNC_EXIT(FUNC_LV_POLICY);
}

static void ppm_thermal_mode_change_cb(enum ppm_mode mode)
{
	FUNC_ENTER(FUNC_LV_POLICY);

	ppm_ver("@%s: ppm mode changed to %d\n", __func__, mode);

	FUNC_EXIT(FUNC_LV_POLICY);
}

static int ppm_thermal_limit_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "limited power = %d\n", thermal_policy.req.power_budget);
	seq_printf(m, "PPM thermal activate = %d\n", thermal_policy.is_activated);

	return 0;
}

static ssize_t ppm_thermal_limit_proc_write(struct file *file, const char __user *buffer,
					size_t count, loff_t *pos)
{
	unsigned int limited_power;

	char *buf = ppm_copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	if (!kstrtouint(buf, 10, &limited_power))
		mt_ppm_cpu_thermal_protect(limited_power);
	else
		ppm_err("@%s: Invalid input!\n", __func__);

	free_page((unsigned long)buf);
	return count;
}

PROC_FOPS_RW(thermal_limit);

static int __init ppm_thermal_policy_init(void)
{
	int i, ret = 0;

	struct pentry {
		const char *name;
		const struct file_operations *fops;
	};

	const struct pentry entries[] = {
		PROC_ENTRY(thermal_limit),
	};

	FUNC_ENTER(FUNC_LV_POLICY);

	/* create procfs */
	for (i = 0; i < ARRAY_SIZE(entries); i++) {
		if (!proc_create(entries[i].name, S_IRUGO | S_IWUSR | S_IWGRP, policy_dir, entries[i].fops)) {
			ppm_err("%s(), create /proc/ppm/policy/%s failed\n", __func__, entries[i].name);
			ret = -EINVAL;
			goto out;
		}
	}

	if (ppm_main_register_policy(&thermal_policy)) {
		ppm_err("@%s: thermal policy register failed\n", __func__);
		ret = -EINVAL;
		goto out;
	}

	ppm_info("@%s: register %s done!\n", __func__, thermal_policy.name);

out:
	FUNC_EXIT(FUNC_LV_POLICY);

	return ret;
}

static void __exit ppm_thermal_policy_exit(void)
{
	FUNC_ENTER(FUNC_LV_POLICY);

	ppm_main_unregister_policy(&thermal_policy);

	FUNC_EXIT(FUNC_LV_POLICY);
}

module_init(ppm_thermal_policy_init);
module_exit(ppm_thermal_policy_exit);

