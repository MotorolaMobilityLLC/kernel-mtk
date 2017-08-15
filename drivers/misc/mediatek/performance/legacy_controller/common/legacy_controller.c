/*
 * Copyright (C) 2017 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/kallsyms.h>
#include <linux/utsname.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/uaccess.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <linux/topology.h>
#include <linux/slab.h>

#include <linux/platform_device.h>
#include <linux/cpumask.h>
#include <mach/mt_lbc.h>
#include "mt_hotplug_strategy.h"
#include "mt_cpufreq.h"

#define TAG "[boost_controller]"

#define MAX(a, b) (((a) > (b)) ? (a) : (b))


static struct mutex boost_freq;
static struct mutex boost_core;
static struct ppm_limit_data current_core;
static struct ppm_limit_data current_freq;
static struct ppm_limit_data core_set[PPM_MAX_KIR];
static struct ppm_limit_data freq_set[PPM_MAX_KIR];
static int nr_ppm_clusters;
static int nr_cpu;
static int log_enable;

#define legacy_debug(enable, fmt, x...)\
	do {\
		if (enable)\
			pr_debug(fmt, ##x);\
	} while (0)

static char *lbc_copy_from_user_for_proc(const char __user *buffer, size_t count)
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

/*************************************************************************************/
int update_userlimit_cpu_core(int kicker, int num_cluster, struct ppm_limit_data *core_limit)
{
	struct ppm_limit_data final_core;
	int retval = 0;
	int i;

	mutex_lock(&boost_core);

	if (num_cluster != nr_ppm_clusters) {
		pr_debug(TAG"num_cluster : %d nr_ppm_clusters: %d, doesn't match",
			 num_cluster, nr_ppm_clusters);
		retval = -1;
		goto ret_update;
	}

	for (i = 0; i < nr_ppm_clusters; i++) {
		final_core.min = -1;
		final_core.max = -1;
	}

#if 0
	for (i = 0; i < nr_ppm_clusters; i++) {
		legacy_debug(log_enable, TAG"cluster %d, core_limit_min %d core_limit_max %d\n",
			 i, core_limit[i].min, core_limit[i].max);
	}
#endif

	core_set[kicker].min = core_limit->min >= -1 ? core_limit->min : -1;
	core_set[kicker].max = core_limit->max >= -1 ? core_limit->max : -1;
	legacy_debug(log_enable, TAG"kicker %d, core_set_min %d core_set_max %d\n",
		 kicker, core_set[kicker].min, core_set[kicker].max);

	for (i = 0; i < PPM_MAX_KIR; i++) {
		final_core.min = MAX(core_set[i].min, final_core.min);
		final_core.max = MAX(core_set[i].max, final_core.max);
		if (final_core.min > final_core.max && final_core.max != -1)
			final_core.max = final_core.min;
	}

	current_core.min = final_core.min;
	current_core.max = final_core.max;
	legacy_debug(log_enable, TAG"current_core_min %d current_core_max %d\n",
		 current_core.min, current_core.max);

#if 0
	hps_set_cpu_num_base(BASE_PERF_SERV, current_core.min == -1 ? 1 : current_core.min, 0);
	hps_set_cpu_num_limit(LIMIT_POWER_SERV, current_core.max == -1 ? nr_cpu : current_core.max, 0);
#endif

ret_update:
	mutex_unlock(&boost_core);
	return retval;

	return 0;
}

/*************************************************************************************/
int update_userlimit_cpu_freq(int kicker, int num_cluster, struct ppm_limit_data *freq_limit)
{
	struct ppm_limit_data final_freq;
	int retval = 0;
	int i;

	mutex_lock(&boost_freq);

	if (num_cluster != nr_ppm_clusters) {
		pr_debug(TAG"num_cluster : %d nr_ppm_clusters: %d, doesn't match\n",
			 num_cluster, nr_ppm_clusters);
		retval = -1;
		goto ret_update;
	}


	for (i = 0; i < nr_ppm_clusters; i++) {
		final_freq.min = -1;
		final_freq.max = -1;
	}
#if 0
	for (i = 0; i < nr_ppm_clusters; i++) {
		legacy_debug(log_enable, TAG"cluster %d, freq_limit_min %d freq_limit_max %d\n",
			 i, freq_limit[i].min, freq_limit[i].max);
	}
#endif


	freq_set[kicker].min = freq_limit->min >= -1 ? freq_limit->min : -1;
	freq_set[kicker].max = freq_limit->max >= -1 ? freq_limit->max : -1;
	legacy_debug(log_enable, TAG"kicker %d, freq_set_min %d freq_set_max %d\n",
		 kicker, freq_set[kicker].min, freq_set[kicker].max);

	for (i = 0; i < PPM_MAX_KIR; i++) {
		final_freq.min = MAX(freq_set[i].min, final_freq.min);
		final_freq.max = MAX(freq_set[i].max, final_freq.max);
		if (final_freq.min > final_freq.max && final_freq.max != -1)
			final_freq.max = final_freq.min;
	}

	current_freq.min = final_freq.min;
	current_freq.max = final_freq.max;
	legacy_debug(log_enable, TAG"freq_min %d freq_max %d\n",
		current_freq.min, current_freq.max);

#if 0
	mt_cpufreq_set_min_freq(MT_CPU_DVFS_LITTLE, current_freq.min == -1 ? 0 : current_freq.min);
	mt_cpufreq_set_max_freq(MT_CPU_DVFS_LITTLE, current_freq.max == -1 ? 0 : current_freq.max);
#endif

ret_update:
	mutex_unlock(&boost_freq);
	return retval;

	return 0;
}

/*************************************************************************************/
static ssize_t perfmgr_perfserv_core_write(struct file *filp, const char __user *ubuf,
		size_t cnt, loff_t *pos)
{
	int i = 0, data;
	struct ppm_limit_data core_limit;
	unsigned int arg_num = nr_ppm_clusters * 2; /* for min and max */
	char *tok, *tmp;
	char *buf = lbc_copy_from_user_for_proc(ubuf, cnt);

	tmp = buf;
	while ((tok = strsep(&tmp, " ")) != NULL) {
		if (i == arg_num) {
			pr_crit(TAG"@%s: number of arguments > %d!\n", __func__, arg_num);
			goto out;
		}

		if (kstrtoint(tok, 10, &data)) {
			pr_crit(TAG"@%s: Invalid input: %s\n", __func__, tok);
			goto out;
		} else {
			if (i % 2) /* max */
				core_limit.max = data;
			else /* min */
				core_limit.min = data;
			i++;
		}
	}

	if (i < arg_num)
		pr_crit(TAG"@%s: number of arguments < %d!\n", __func__, arg_num);
	else
		update_userlimit_cpu_core(PPM_KIR_PERF, nr_ppm_clusters, &core_limit);

out:
	free_page((unsigned long)buf);
	return cnt;

}

static int perfmgr_perfserv_core_show(struct seq_file *m, void *v)
{
	seq_printf(m, "min:%d max:%d\n",
		core_set[PPM_KIR_PERF].min, core_set[PPM_KIR_PERF].max);

	return 0;
}

static int perfmgr_perfserv_core_open(struct inode *inode, struct file *file)
{
	return single_open(file, perfmgr_perfserv_core_show, inode->i_private);
}

static const struct file_operations perfmgr_perfserv_core_fops = {
	.open = perfmgr_perfserv_core_open,
	.write = perfmgr_perfserv_core_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

/*************************************************************************************/
static ssize_t perfmgr_perfserv_freq_write(struct file *filp, const char __user *ubuf,
		size_t cnt, loff_t *pos)
{
	int i = 0, data;
	struct ppm_limit_data freq_limit;
	unsigned int arg_num = nr_ppm_clusters * 2; /* for min and max */
	char *tok, *tmp;
	char *buf = lbc_copy_from_user_for_proc(ubuf, cnt);

	tmp = buf;
	while ((tok = strsep(&tmp, " ")) != NULL) {
		if (i == arg_num) {
			pr_crit(TAG"@%s: number of arguments > %d!\n", __func__, arg_num);
			goto out;
		}

		if (kstrtoint(tok, 10, &data)) {
			pr_crit(TAG"@%s: Invalid input: %s\n", __func__, tok);
			goto out;
		} else {
			if (i % 2) /* max */
				freq_limit.max = data;
			else /* min */
				freq_limit.min = data;
			i++;
		}
	}

	if (i < arg_num)
		pr_crit(TAG"@%s: number of arguments < %d!\n", __func__, arg_num);
	else
		update_userlimit_cpu_freq(PPM_KIR_PERF, nr_ppm_clusters, &freq_limit);

out:
	free_page((unsigned long)buf);
	return cnt;

}

static int perfmgr_perfserv_freq_show(struct seq_file *m, void *v)
{
	seq_printf(m, "min:%d max:%d\n",
		freq_set[PPM_KIR_PERF].min, freq_set[PPM_KIR_PERF].max);

	return 0;
}

static int perfmgr_perfserv_freq_open(struct inode *inode, struct file *file)
{
	return single_open(file, perfmgr_perfserv_freq_show, inode->i_private);
}

static const struct file_operations perfmgr_perfserv_freq_fops = {
	.open = perfmgr_perfserv_freq_open,
	.write = perfmgr_perfserv_freq_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

/*************************************************************************************/
static int perfmgr_current_core_show(struct seq_file *m, void *v)
{
	seq_printf(m, "min:%d max:%d\n",
		current_core.min, current_core.max);

	return 0;
}

static int perfmgr_current_core_open(struct inode *inode, struct file *file)
{
	return single_open(file, perfmgr_current_core_show, inode->i_private);
}

static const struct file_operations perfmgr_current_core_fops = {
	.open = perfmgr_current_core_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
/*************************************************************************************/
static int perfmgr_current_freq_show(struct seq_file *m, void *v)
{
	seq_printf(m, "min:%d max:%d\n",
		current_freq.min, current_freq.max);

	return 0;
}

static int perfmgr_current_freq_open(struct inode *inode, struct file *file)
{
	return single_open(file, perfmgr_current_freq_show, inode->i_private);
}

static const struct file_operations perfmgr_current_freq_fops = {
	.open = perfmgr_current_freq_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

/*************************************************************************************/
static ssize_t perfmgr_log_write(struct file *filp, const char __user *ubuf,
		size_t cnt, loff_t *pos)
{
	char buf[64];
	unsigned long val;
	int ret;

	if (cnt >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(buf, ubuf, cnt))
		return -EFAULT;
	buf[cnt] = 0;
	ret = kstrtoul(buf, 10, &val);
	if (ret < 0)
		return ret;

	if (val)
		log_enable = 1;
	else
		log_enable = 0;

	return cnt;
}

static int perfmgr_log_show(struct seq_file *m, void *v)
{
		seq_printf(m, "%d\n",
				log_enable);

	return 0;
}

static int perfmgr_log_open(struct inode *inode, struct file *file)
{
	return single_open(file, perfmgr_log_show, inode->i_private);
}

static const struct file_operations perfmgr_log_fops = {
	.open = perfmgr_log_open,
	.write = perfmgr_log_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

/*************************************************************************************/
static int __init perfmgr_legacy_boost_init(void)
{
	int i;
	struct proc_dir_entry *boost_dir = NULL;

	mutex_init(&boost_core);
	mutex_init(&boost_freq);
	boost_dir = proc_mkdir("perfmgr/legacy", NULL);

	nr_ppm_clusters = 1;
	nr_cpu = num_possible_cpus();
	proc_create("perfserv_core", 0644, boost_dir, &perfmgr_perfserv_core_fops);
	proc_create("current_core", 0644, boost_dir, &perfmgr_current_core_fops);
	proc_create("perfserv_freq", 0644, boost_dir, &perfmgr_perfserv_freq_fops);
	proc_create("current_freq", 0644, boost_dir, &perfmgr_current_freq_fops);
	proc_create("perfmgr_log", 0644, boost_dir, &perfmgr_log_fops);

	current_core.min = -1;
	current_core.max = -1;
	current_freq.min = -1;
	current_freq.max = -1;
	for (i = 0; i < PPM_MAX_KIR; i++) {
		core_set[i].min = -1;
		core_set[i].max = -1;
		freq_set[i].min = -1;
		freq_set[i].max = -1;
	}

	return 0;

}

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MediaTek LBC");
module_init(perfmgr_legacy_boost_init);
