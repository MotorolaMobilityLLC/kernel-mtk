/*
 * Copyright (C) 2018 MediaTek Inc.
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

#define pr_fmt(fmt) "[topo_ctrl]"fmt

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/topology.h>

#include "tchbst.h"

static int clu_num, s_clstr_core, b_clstr_core;
static int *calc_cpu_cap, *calc_cpu_num;

int topo_ctrl_get_nr_clusters(void)
{
	return clu_num;
}
EXPORT_SYMBOL(topo_ctrl_get_nr_clusters);

/***************************************/
static void calc_min_cpucap(void)
{

	int i, cpu_num = 0, min = INT_MAX, max = 0;
	struct cpumask cpus;
	struct cpumask cpu_online_cpumask;

	for (i = 0; i < clu_num ; i++) {
		arch_get_cluster_cpus(&cpus, i);
		cpumask_and(&cpu_online_cpumask,
			&cpus, cpu_possible_mask);

		calc_cpu_num[i] = cpumask_weight(&cpu_online_cpumask);
		calc_cpu_cap[i] = arch_get_max_cpu_capacity(cpu_num);

		if (calc_cpu_cap[i] < min) {
			s_clstr_core = i;
			min = calc_cpu_cap[i];
		}
		if (calc_cpu_cap[i] > max) {
			b_clstr_core = i;
			max = calc_cpu_cap[i];
		}
		cpu_num += calc_cpu_num[i];
	}
}

int get_min_clstr_cap(void)
{
	/*get smallest cluster num*/
	return s_clstr_core;
}

int get_max_clstr_cap(void)
{
	/*get biggest cluster num*/
	return b_clstr_core;
}

static int glb_info_show(struct seq_file *m, void *v)
{
	int i;

	seq_printf(m, "clu_num : %d\n", clu_num);

	for (i = 0 ; i < clu_num ; i++) {
		seq_printf(m, "calc_cpu_cap[%d]:%d\n", i, calc_cpu_cap[i]);
		seq_printf(m, "calc_cpu_num[%d]:%d\n", i, calc_cpu_num[i]);
	}
	seq_printf(m, "s_clstr_core : %d\n", s_clstr_core);
	seq_printf(m, "b_clstr_core : %d\n", b_clstr_core);
	return 0;
}

static int glb_info_open(struct inode *inode, struct file *file)
{
	return single_open(file, glb_info_show, inode->i_private);
}

static const struct file_operations glb_info_fops = {
	.open = glb_info_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};


static int topo_ctrl_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", clu_num);
	return 0;
}

static int topo_ctrl_open(struct inode *inode, struct file *file)
{
	return single_open(file, topo_ctrl_show, inode->i_private);
}

static const struct file_operations topo_ctrl_fops = {
	.open = topo_ctrl_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

/************************************************/
int topo_ctrl_init(struct proc_dir_entry *parent)
{
	struct proc_dir_entry *topo_dir = NULL;

	topo_dir = proc_mkdir("topo_ctrl", parent);

	if (!topo_dir)
		pr_debug("topo_dir null\n ");

	proc_create("clstr_num", 0644, topo_dir,
			 &topo_ctrl_fops);

	proc_create("glbinfo", 0644, topo_dir,
			 &glb_info_fops);


	clu_num = arch_get_nr_clusters();
	calc_cpu_cap =  kcalloc(clu_num, sizeof(int), GFP_KERNEL);
	calc_cpu_num =  kcalloc(clu_num, sizeof(int), GFP_KERNEL);
	calc_min_cpucap();

	return 0;

}

void topo_ctrl_exit(void)
{
	kfree(calc_cpu_cap);
	kfree(calc_cpu_num);
}
