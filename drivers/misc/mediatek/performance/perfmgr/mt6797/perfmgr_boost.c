/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/kallsyms.h>
#include <linux/utsname.h>
#include <asm/uaccess.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <linux/notifier.h>

#include <linux/platform_device.h>
#include "perfmgr.h"
#include "perfmgr_boost.h"
#include "mt_ppm_api.h"

/*--------------DEFAULT SETTING-------------------*/

#ifdef TAG
#undef TAG
#endif
#define TAG "[PERFMGR]"

#define TARGET_CORE (3)
#define TARGET_FREQ (1066000)

#define CLUSTER_NUM	(3)
#define MAX_CORE (0xff)

#ifdef max
#undef max
#endif
#define max(a, b) (((a) > (b)) ? (a) : (b))

#ifdef min
#undef min
#endif
#define min(a, b) (((a) < (b)) ? (a) : (b))

/*-----------------------------------------------*/
struct scn_data {
	struct forcelimit_data data[CLUSTER_NUM];
};

struct scn_data perfmgr_curr_setting;
struct scn_data perfmgr_scn_list[FORCELIMIT_TYPE_NUM];

static int cpuset_pid;

/*-----------------------------------------------*/
/* prototype                                     */
/*-----------------------------------------------*/


/*-----------------------------------------------*/


int perfmgr_get_target_core(void)
{
	return TARGET_CORE;
}

int perfmgr_get_target_freq(void)
{
	return TARGET_FREQ;
}

void perfmgr_boost(int enable, int core, int freq)
{
	if (enable) {
		mt_ppm_sysboost_core(BOOST_BY_PERFSERV, core);
		mt_ppm_sysboost_freq(BOOST_BY_PERFSERV, freq);
	} else {
		mt_ppm_sysboost_core(BOOST_BY_PERFSERV, 0);
		mt_ppm_sysboost_freq(BOOST_BY_PERFSERV, 0);
	}
}

static ssize_t perfmgr_force_sp_write(struct file *filp, const char *ubuf,
		size_t cnt, loff_t *pos)
{
	int i = 0, data;
	unsigned int arg_num = CLUSTER_NUM * 2; /* for min and max */
	char *tok;
	char buf[128], *ptr;

	if (cnt >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(&buf, ubuf, cnt))
		return -EFAULT;
	buf[cnt] = 0;
	ptr = buf;
	while ((tok = strsep(&ptr, " ")) != NULL) {
		/* pr_debug(TAG"perfmgr_force_sp_write 1, i:%d, arg_num:%d, tok:%s[end]\n", i, arg_num, tok); */
		if (i == arg_num)
			break;

		if (kstrtoint(tok, 10, &data))
			break;

		/* pr_debug(TAG"perfmgr_force_sp_write 2, i:%d, arg_num:%d\n", i, arg_num); */
		if (i % 2) /* max */
			perfmgr_scn_list[FORCELIMIT_TYPE_SP].data[i/2].max_core = data;
		else /* min */
			perfmgr_scn_list[FORCELIMIT_TYPE_SP].data[i/2].min_core = data;

		i++;
	}

	perfmgr_forcelimit_cpu_core(FORCELIMIT_TYPE_SP, CLUSTER_NUM, perfmgr_scn_list[FORCELIMIT_TYPE_SP].data);
	return cnt;
}

static int perfmgr_force_sp_show(struct seq_file *m, void *v)
{
	int i;

	for (i = 0; i < CLUSTER_NUM; i++) {
		seq_printf(m, "cluster %d: min = %d, max = %d\n",
			i, perfmgr_scn_list[FORCELIMIT_TYPE_SP].data[i].min_core,
			perfmgr_scn_list[FORCELIMIT_TYPE_SP].data[i].max_core);
	}
	return 0;
}

static int perfmgr_force_sp_open(struct inode *inode, struct file *file)
{
	return single_open(file, perfmgr_force_sp_show, inode->i_private);
}

static const struct file_operations perfmgr_force_sp_fops = {
	.open = perfmgr_force_sp_open,
	.write = perfmgr_force_sp_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static ssize_t perfmgr_force_vr_write(struct file *filp, const char *ubuf,
		size_t cnt, loff_t *pos)
{
	int i = 0, data;
	unsigned int arg_num = CLUSTER_NUM * 2; /* for min and max */
	char *tok;
	char buf[128], *ptr;

	if (cnt >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(&buf, ubuf, cnt))
		return -EFAULT;
	buf[cnt] = 0;
	ptr = buf;
	while ((tok = strsep(&ptr, " ")) != NULL) {
		if (i == arg_num)
			break;

		if (kstrtoint(tok, 10, &data))
			break;

		if (i % 2) /* max */
			perfmgr_scn_list[FORCELIMIT_TYPE_VR].data[i/2].max_core = data;
		else /* min */
			perfmgr_scn_list[FORCELIMIT_TYPE_VR].data[i/2].min_core = data;

			i++;
	}

	perfmgr_forcelimit_cpu_core(FORCELIMIT_TYPE_VR, CLUSTER_NUM, perfmgr_scn_list[FORCELIMIT_TYPE_VR].data);
	return cnt;
}

static int perfmgr_force_vr_show(struct seq_file *m, void *v)
{
	int i;

	for (i = 0; i < CLUSTER_NUM; i++) {
		seq_printf(m, "cluster %d: min = %d, max = %d\n",
			i, perfmgr_scn_list[FORCELIMIT_TYPE_VR].data[i].min_core,
			perfmgr_scn_list[FORCELIMIT_TYPE_VR].data[i].max_core);
	}
	return 0;
}

static int perfmgr_force_vr_open(struct inode *inode, struct file *file)
{
	return single_open(file, perfmgr_force_vr_show, inode->i_private);
}

static const struct file_operations perfmgr_force_vr_fops = {
	.open = perfmgr_force_vr_open,
	.write = perfmgr_force_vr_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};


static ssize_t perfmgr_force_cpuset_write(struct file *filp, const char *ubuf,
		size_t cnt, loff_t *pos)
{
	int i = 0, data;
	unsigned int arg_num = CLUSTER_NUM * 2; /* for min and max */
	char *tok;
	char buf[128], *ptr;

	if (cnt >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(&buf, ubuf, cnt))
		return -EFAULT;
	buf[cnt] = 0;
	ptr = buf;
	while ((tok = strsep(&ptr, " ")) != NULL) {
		if (i == arg_num)
			break;

		if (kstrtoint(tok, 10, &data))
			break;

		if (i % 2) /* max */
			perfmgr_scn_list[FORCELIMIT_TYPE_CPUSET].data[i/2].max_core = data;
		else /* min */
			perfmgr_scn_list[FORCELIMIT_TYPE_CPUSET].data[i/2].min_core = data;

		i++;
	}

	#if 0
	pr_debug(TAG"perfmgr_force_cpuset_write, data[0] min:%d, max:%d\n",
		perfmgr_scn_list[FORCELIMIT_TYPE_CPUSET].data[0].min_core,
		perfmgr_scn_list[FORCELIMIT_TYPE_CPUSET].data[0].max_core);
	pr_debug(TAG"perfmgr_force_cpuset_write, data[1] min:%d, max:%d\n",
		perfmgr_scn_list[FORCELIMIT_TYPE_CPUSET].data[1].min_core,
		perfmgr_scn_list[FORCELIMIT_TYPE_CPUSET].data[1].max_core);
	#endif
	perfmgr_forcelimit_cpu_core(FORCELIMIT_TYPE_CPUSET, CLUSTER_NUM, perfmgr_scn_list[FORCELIMIT_TYPE_CPUSET].data);
	return cnt;
}

static int perfmgr_force_cpuset_show(struct seq_file *m, void *v)
{
	int i;

	for (i = 0; i < CLUSTER_NUM; i++) {
		seq_printf(m, "cluster %d: min = %d, max = %d\n",
			i, perfmgr_scn_list[FORCELIMIT_TYPE_CPUSET].data[i].min_core,
			perfmgr_scn_list[FORCELIMIT_TYPE_CPUSET].data[i].max_core);
	}
	return 0;
}

static int perfmgr_force_cpuset_open(struct inode *inode, struct file *file)
{
	return single_open(file, perfmgr_force_cpuset_show, inode->i_private);
}

static const struct file_operations perfmgr_force_cpuset_fops = {
	.open = perfmgr_force_cpuset_open,
	.write = perfmgr_force_cpuset_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static ssize_t perfmgr_cpuset_pid_write(struct file *filp, const char *ubuf,
		size_t cnt, loff_t *pos)
{
	char buf[64];
	unsigned long val;
	int ret;

	if (cnt >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(&buf, ubuf, cnt))
		return -EFAULT;
	buf[cnt] = 0;
	ret = kstrtoul(buf, 10, &val);
	if (ret < 0)
		return ret;

	cpuset_pid = val;

#if defined(CONFIG_CPUSETS) && !defined(CONFIG_MTK_ACAO)
	save_set_exclusive_task(cpuset_pid);
#endif

	/* for debug */
	if (cpuset_pid == 0)
		perfmgr_forcelimit_cpuset_cancel();

	return cnt;
}

static int perfmgr_cpuset_pid_show(struct seq_file *m, void *v)
{
	seq_printf(m, "cpuset_pid: %d\n", cpuset_pid);
	return 0;
}

static int perfmgr_cpuset_pid_open(struct inode *inode, struct file *file)
{
	return single_open(file, perfmgr_cpuset_pid_show, inode->i_private);
}

static const struct file_operations perfmgr_cpuset_pid_fops = {
	.open = perfmgr_cpuset_pid_open,
	.write = perfmgr_cpuset_pid_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

void perfmgr_forcelimit_init(void)
{
	int i, j;
	struct proc_dir_entry *boost_dir = NULL;

	boost_dir = proc_mkdir("perfmgr/boost", NULL);

	/* touch */
	proc_create("force_sp", 0644, boost_dir, &perfmgr_force_sp_fops);
	proc_create("force_vr", 0644, boost_dir, &perfmgr_force_vr_fops);
	proc_create("force_cpuset", 0644, boost_dir, &perfmgr_force_cpuset_fops);
	proc_create("cpuset_pid", 0644, boost_dir, &perfmgr_cpuset_pid_fops);

	for (i = 0; i < CLUSTER_NUM; i++) {
		perfmgr_curr_setting.data[i].min_core = -1;
		perfmgr_curr_setting.data[i].max_core = MAX_CORE;

		for (j = 0; j < FORCELIMIT_TYPE_NUM; j++) {
			perfmgr_scn_list[j].data[i].min_core = -1;
			perfmgr_scn_list[j].data[i].max_core = MAX_CORE;
		}
	}
}

void perfmgr_forcelimit_cpuset_cancel(void)
{
	int i;

	pr_debug(TAG"perfmgr_forcelimit_cpuset_cancel\n");

	for (i = 0; i < CLUSTER_NUM; i++) {
		perfmgr_scn_list[FORCELIMIT_TYPE_CPUSET].data[i].min_core = -1;
		perfmgr_scn_list[FORCELIMIT_TYPE_CPUSET].data[i].max_core = -1;
	}
	perfmgr_forcelimit_cpu_core(FORCELIMIT_TYPE_CPUSET, CLUSTER_NUM, perfmgr_scn_list[FORCELIMIT_TYPE_CPUSET].data);
}

void perfmgr_forcelimit_cpu_core(int type, int cluster_num, struct forcelimit_data *data)
{
	int i, j, minToSet[CLUSTER_NUM], maxToSet[CLUSTER_NUM];
	struct ppm_limit_data ppm_data[CLUSTER_NUM];

	if (type < 0 || type >= FORCELIMIT_TYPE_NUM)
		return;

	/* scan all scenarios */
	for (i = 0; i < CLUSTER_NUM; i++) {
		minToSet[i] = 0;
		maxToSet[i] = MAX_CORE;

		for (j = 0; j < FORCELIMIT_TYPE_NUM; j++) {
			if (perfmgr_scn_list[j].data[i].min_core != -1)
				minToSet[i] = max(minToSet[i], perfmgr_scn_list[j].data[i].min_core);
			if (perfmgr_scn_list[j].data[i].max_core != -1)
				maxToSet[i] = min(maxToSet[i], perfmgr_scn_list[j].data[i].max_core);
			/* pr_debug(TAG"perfmgr_forcelimit_cpu_core, i:%d, j:%d, min:%d, max:%d\n",
			 *	i, j, perfmgr_scn_list[j].data[i].min_core, perfmgr_scn_list[j].data[i].max_core);
			*/
		}

		perfmgr_curr_setting.data[i].min_core = minToSet[i];
		perfmgr_curr_setting.data[i].max_core = maxToSet[i];

		if (minToSet[i] == 0)
			minToSet[i] = -1;
		if (maxToSet[i] == MAX_CORE)
			maxToSet[i] = -1;
		if (maxToSet[i] >= 0 && maxToSet[i] < minToSet[i])
			maxToSet[i] = minToSet[i]; /* minToSet has higher priority */

		ppm_data[i].min = minToSet[i];
		ppm_data[i].max = maxToSet[i];
	}

	#if 1
	for (i = 0; i < CLUSTER_NUM; i++) {
		pr_debug(TAG"perfmgr_forcelimit_cpu_core, PPM, i:%d, min:%d, max:%d\n",
			i, ppm_data[i].min, ppm_data[i].max);
	}
	#endif
	mt_ppm_forcelimit_cpu_core(CLUSTER_NUM, ppm_data);
}

void init_perfmgr_boost(void)
{
	cpuset_pid = 0;
	perfmgr_forcelimit_init();
}

