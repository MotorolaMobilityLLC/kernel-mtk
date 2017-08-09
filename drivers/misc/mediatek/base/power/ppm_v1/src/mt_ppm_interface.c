
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <asm/uaccess.h>

#include "mt_ppm_internal.h"


#define PPM_MODE_NAME_LEN	16

/* procfs dir for policies */
struct proc_dir_entry *policy_dir = NULL;
unsigned int ppm_debug = 0;
#if 0
unsigned int ppm_func_lv_mask = (FUNC_LV_MODULE  | FUNC_LV_API | FUNC_LV_MAIN | FUNC_LV_POLICY);
#else
unsigned int ppm_func_lv_mask = 0;
#endif


char *ppm_copy_from_user_for_proc(const char __user *buffer, size_t count)
{
	char *buf = (char *)__get_free_page(GFP_USER);

	if (!buf)
		return NULL;

	if (count >= PAGE_SIZE)
		goto out;

	if (copy_from_user(buf, buffer, count))
		goto out;

	buf[count] = '\0';

	return buf;

out:
	free_page((unsigned long)buf);

	return NULL;
}

static int ppm_func_debug_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "ppm func lv debug mask = 0x%x\n", ppm_func_lv_mask);

	return 0;
}

static ssize_t ppm_func_debug_proc_write(struct file *file, const char __user *buffer, size_t count,
					loff_t *pos)
{
	unsigned int func_dbg_lv;

	char *buf = ppm_copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	if (!kstrtouint(buf, 10, &func_dbg_lv))
		ppm_func_lv_mask = func_dbg_lv;
	else
		ppm_err("echo func_dbg_lv (dec) > /proc/ppm/func_debug\n");

	free_page((unsigned long)buf);
	return count;
}

static int ppm_debug_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "ppm debug (log level) = %d\n", ppm_debug);

	return 0;
}

static ssize_t ppm_debug_proc_write(struct file *file, const char __user *buffer, size_t count,
					loff_t *pos)
{
	unsigned int dbg_lv;

	char *buf = ppm_copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	if (!kstrtouint(buf, 10, &dbg_lv))
		ppm_debug = dbg_lv;
	else
		ppm_err("echo dbg_lv (dec) > /proc/ppm/debug\n");

	free_page((unsigned long)buf);
	return count;
}

static int ppm_enabled_proc_show(struct seq_file *m, void *v)
{
	if (ppm_main_info.is_enabled == true)
		seq_puts(m, "ppm is enabled\n");
	else
		seq_puts(m, "ppm is disabled\n");

	return 0;
}

static ssize_t ppm_enabled_proc_write(struct file *file, const char __user *buffer, size_t count,
					loff_t *pos)
{
	unsigned int enabled;

	char *buf = ppm_copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	ppm_lock(&ppm_main_info.lock);

	if (!kstrtouint(buf, 10, &enabled)) {
		ppm_main_info.is_enabled = (enabled) ? true : false;
		if (!ppm_main_info.is_enabled) {
			int i;
			struct ppm_client_req *c_req = &(ppm_main_info.client_req);
			struct ppm_client_req *last_req = &(ppm_main_info.last_req);

			/* send default limit to client */
			ppm_main_clear_client_req(c_req);
			for_each_ppm_clients(i) {
				if (ppm_main_info.client_info[i].limit_cb)
					ppm_main_info.client_info[i].limit_cb(*c_req);
			}
			memcpy(last_req->cpu_limit, c_req->cpu_limit,
				ppm_main_info.cluster_num * sizeof(*c_req->cpu_limit));

			ppm_info("send no limit to clinet since ppm is disabled!\n");
		}
	} else
		ppm_err("echo [0/1] > /proc/ppm/enabled\n");

	ppm_unlock(&ppm_main_info.lock);

	free_page((unsigned long)buf);
	return count;
}

static int ppm_dump_power_table_proc_show(struct seq_file *m, void *v)
{
	unsigned int i;
	struct ppm_power_tbl_data power_table = ppm_get_power_table();

	for_each_pwr_tbl_entry(i, power_table) {
		seq_printf(m, "[%d]\t= %d,\t%d,\t%d,\t%d,\t%d,\t%d\n",
			power_table.power_tbl[i].index,
			power_table.power_tbl[i].cluster_cfg[0].opp_lv,
			power_table.power_tbl[i].cluster_cfg[0].core_num,
			power_table.power_tbl[i].cluster_cfg[1].opp_lv,
			power_table.power_tbl[i].cluster_cfg[1].core_num,
			power_table.power_tbl[i].perf_idx,
			power_table.power_tbl[i].power_idx
		);
	}

	return 0;
}

static int ppm_dump_policy_list_proc_show(struct seq_file *m, void *v)
{
	struct ppm_policy_data *pos;
	unsigned int i = 0, j = 0;

	ppm_lock(&ppm_main_info.lock);
	seq_printf(m, "Current state = %s\n\n", ppm_get_power_state_name(ppm_main_info.cur_power_state));

	list_for_each_entry(pos, &ppm_main_info.policy_list, link) {
		ppm_lock(&pos->lock);

		seq_printf(m, "[%d] %s (priority: %d)\n", i, pos->name, pos->priority);
		seq_printf(m, "is_enabled = %d, is_activated = %d\n",
				pos->is_enabled, pos->is_activated);
		seq_printf(m, "req_perf_idx = %d, req_power_budget = %d\n",
				pos->req.perf_idx, pos->req.power_budget);
		for_each_ppm_clusters(j) {
			seq_printf(m, "cluster %d: (%d)(%d)(%d)(%d)\n", j,
				pos->req.limit[j].min_cpufreq_idx, pos->req.limit[j].max_cpufreq_idx,
				pos->req.limit[j].min_cpu_core, pos->req.limit[j].max_cpu_core);
		}
		seq_puts(m, "\n");
		ppm_unlock(&pos->lock);

		i++;
	}
	ppm_unlock(&ppm_main_info.lock);

	return 0;
}

static int ppm_policy_status_proc_show(struct seq_file *m, void *v)
{
	struct ppm_policy_data *pos;

	list_for_each_entry_reverse(pos, &ppm_main_info.policy_list, link)
		seq_printf(m, "[%d] %s: %s\n", pos->policy, pos->name,
				(pos->is_enabled) ? "enabled" : "disabled");

	seq_puts(m, "\nUsage: echo <policy_idx> <1:enable/0:disable> > /proc/ppm/policy_status\n\n");

	return 0;
}

static ssize_t ppm_policy_status_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos)
{
	struct ppm_policy_data *list_pos;
	unsigned int policy_idx, enabled;

	char *buf = ppm_copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	if (sscanf(buf, "%d %d", &policy_idx, &enabled) == 2) {
		if (enabled > 1)
			enabled = 1;

		/* set target mode status and notify the policy via status change callback */
		list_for_each_entry(list_pos, &ppm_main_info.policy_list, link) {
			if (list_pos->policy == policy_idx && list_pos->is_enabled != enabled) {
				ppm_lock(&list_pos->lock);
				list_pos->is_enabled = (enabled) ? true : false;
				if (!list_pos->is_enabled)
					list_pos->is_activated = false;
				if (list_pos->status_change_cb)
					list_pos->status_change_cb(list_pos->is_enabled);
				ppm_unlock(&list_pos->lock);

				ppm_task_wakeup();
				break;
			}
		}
	} else
		ppm_err("Invalid input! Usage: echo <policy_idx> <1(enable)/0(disable)> > /proc/ppm/policy_status\n");

	free_page((unsigned long)buf);
	return count;
}

static int ppm_mode_proc_show(struct seq_file *m, void *v)
{
	char *mode = "none";

	switch (ppm_main_info.cur_mode) {
	case PPM_MODE_LOW_POWER:
		mode = "Low_Power";
		break;
	case PPM_MODE_JUST_MAKE:
		mode = "Just_Make";
		break;
	case PPM_MODE_PERFORMANCE:
		mode = "Performance";
		break;
	default:
		BUG();
	}

	seq_printf(m, "Current PPM mode = %s\n", mode);

	seq_puts(m, "\nUsage: echo <mode_name> > /proc/ppm/mode\n\n");

	return 0;
}

static ssize_t ppm_mode_proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *pos)
{
	struct ppm_policy_data *list_pos;
	char	str_mode[PPM_MODE_NAME_LEN];
	enum ppm_mode mode;

	char *buf = ppm_copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	if (sscanf(buf, "%15s", str_mode) == 1) {
		if (!strnicmp(str_mode, "low_power", PPM_MODE_NAME_LEN))
			mode = PPM_MODE_LOW_POWER;
		else if (!strnicmp(str_mode, "just_make", PPM_MODE_NAME_LEN))
			mode = PPM_MODE_JUST_MAKE;
		else if (!strnicmp(str_mode, "performance", PPM_MODE_NAME_LEN))
			mode = PPM_MODE_PERFORMANCE;
		else {
			ppm_err("Invalid input! Usage: echo <mode_name> > /proc/ppm/mode\n");
			goto out;
		}

		if (mode != ppm_main_info.cur_mode) {
			ppm_lock(&ppm_main_info.lock);
			ppm_main_info.cur_mode = mode;
			ppm_unlock(&ppm_main_info.lock);

			/* notify all policy that PPM mode has been changed */
			list_for_each_entry(list_pos, &ppm_main_info.policy_list, link) {
				ppm_lock(&list_pos->lock);
				if (list_pos->mode_change_cb)
					list_pos->mode_change_cb(mode);
				ppm_unlock(&list_pos->lock);
			}
			ppm_task_wakeup();
		}
	} else
		ppm_err("Invalid input! Usage: echo <mode_name> > /proc/ppm/mode\n");

out:
	free_page((unsigned long)buf);
	return count;
}

static int ppm_root_cluster_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", ppm_main_info.fixed_root_cluster);

	return 0;
}

static ssize_t ppm_root_cluster_proc_write(struct file *file, const char __user *buffer,
					size_t count, loff_t *pos)
{
	int cluster;

	char *buf = ppm_copy_from_user_for_proc(buffer, count);

	if (!buf)
		return -EINVAL;

	if (!kstrtoint(buf, 10, &cluster)) {
		ppm_lock(&ppm_main_info.lock);
		ppm_main_info.fixed_root_cluster = (cluster >= (int)ppm_main_info.cluster_num) ? -1 : cluster;
		ppm_unlock(&ppm_main_info.lock);

		if (ppm_main_info.fixed_root_cluster != -1)
			ppm_hica_fix_root_cluster_changed(ppm_main_info.fixed_root_cluster);
	} else
		ppm_err("echo (cluster_id) > /proc/ppm/policy/hica_root_cluster\n");

	free_page((unsigned long)buf);
	return count;
}

static int ppm_dump_dvfs_table_proc_show(struct seq_file *m, void *v)
{
	struct ppm_cluster_info *info = (struct ppm_cluster_info *)m->private;
	unsigned int i;

	if (!info->dvfs_tbl) {
		ppm_err("DVFS table for cluster %d is NULL!\n", info->cluster_id);
		goto end;
	}

	for (i = 0; i < info->dvfs_opp_num; i++)
		seq_printf(m, "%d ", info->dvfs_tbl[i].frequency);

	seq_puts(m, "\n");

end:
	return 0;
}

PROC_FOPS_RW(func_debug);
PROC_FOPS_RW(debug);
PROC_FOPS_RW(enabled);
PROC_FOPS_RO(dump_power_table);
PROC_FOPS_RO(dump_policy_list);
PROC_FOPS_RW(policy_status);
PROC_FOPS_RW(mode);
PROC_FOPS_RW(root_cluster);
PROC_FOPS_RO(dump_dvfs_table);

int ppm_procfs_init(void)
{
	struct proc_dir_entry *dir = NULL;
	int i;
	char str[32];

	struct pentry {
		const char *name;
		const struct file_operations *fops;
	};

	const struct pentry entries[] = {
		PROC_ENTRY(func_debug),
		PROC_ENTRY(debug),
		PROC_ENTRY(enabled),
		PROC_ENTRY(dump_power_table),
		PROC_ENTRY(dump_policy_list),
		PROC_ENTRY(policy_status),
		PROC_ENTRY(mode),
		PROC_ENTRY(root_cluster),
	};

	dir = proc_mkdir("ppm", NULL);
	if (!dir) {
		ppm_err("@%s: fail to create /proc/ppm dir\n", __func__);
		return -ENOMEM;
	}

	/* mkdir for policies */
	policy_dir = proc_mkdir("policy", dir);
	if (!policy_dir) {
		ppm_err("@%s: fail to create /proc/ppm/policy dir\n", __func__);
		return -ENOMEM;
	}

	for (i = 0; i < ARRAY_SIZE(entries); i++) {
		if (!proc_create(entries[i].name, S_IRUGO | S_IWUSR | S_IWGRP, dir, entries[i].fops))
			ppm_err("%s(), create /proc/ppm/%s failed\n", __func__, entries[i].name);
	}

	for_each_ppm_clusters(i) {
		sprintf(str, "dump_cluster_%d_dvfs_table", i);

		if (!proc_create_data(str, S_IRUGO | S_IWUSR | S_IWGRP,
			dir, &ppm_dump_dvfs_table_proc_fops, &ppm_main_info.cluster_info[i]))
			ppm_err("%s(), create /proc/ppm/%s failed\n", __func__, str);
	}

	return 0;
}

