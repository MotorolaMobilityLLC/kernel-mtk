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
#include <linux/module.h>


static int clu_num;

int topo_ctrl_get_nr_clusters(void)
{
	return clu_num;
}
EXPORT_SYMBOL(topo_ctrl_get_nr_clusters);

/***************************************/

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
static int __init topo_ctrl_init(void)
{
	struct proc_dir_entry *topo_dir = NULL;

	topo_dir = proc_mkdir("perfmgr/boost_ctrl/topo_ctrl", NULL);

	if (!topo_dir)
		pr_debug("topo_dir null\n ");

	proc_create("clstr_num", 0644, topo_dir,
			 &topo_ctrl_fops);
	clu_num = arch_get_nr_clusters();
	return 0;

}
module_init(topo_ctrl_init);
