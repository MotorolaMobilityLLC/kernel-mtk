/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/spinlock.h>
#include <linux/kthread.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/interrupt.h>
#include <linux/syscore_ops.h>
#include <linux/platform_device.h>
#include <linux/completion.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/io.h>

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_fdt.h>
#include <mt-plat/aee.h>
#endif

#include "mtk_eem.h"

#define PICACHU_BASE	0x0012A250
#define PICACHU_SIZE	0x40

#define PARA_PATH       "/dev/block/platform/bootdevice/by-name/para"
#define CFG_ENV_SIZE    0x1000
#define CFG_ENV_OFFSET  0x40000

#define NR_CAL_OPPS         1

#undef TAG
#define TAG     "[Picachu] "

#define picachu_err(fmt, args...)		\
	pr_err(TAG"[ERROR]"fmt, ##args)
#define picachu_warn(fmt, args...)		\
	pr_warn(TAG"[WARNING]"fmt, ##args)
#define picachu_info(fmt, args...)		\
	pr_warn(TAG""fmt, ##args)
#define picachu_dbg(fmt, args...)		\
	do {				\
		if (picachu_debug)		\
			picachu_info(fmt, ##args);		\
		else			\
			pr_debug(TAG""fmt, ##args);	\
	} while (0)
#define picachu_ver(fmt, args...)		\
	do {				\
		if (picachu_debug == 1)	\
			picachu_info(fmt, ##args);		\
	} while (0)
#define picachu_cont(fmt, args...)		\
	pr_cont(fmt, ##args)

#define PICACHU_PTP1_MTDES_START_BIT    0
#define PICACHU_PTP1_BDES_START_BIT     8
#define PICACHU_PTP1_MDES_START_BIT     16

/*
 * This dedinition indicates that VPROC1 has three clusters:
 *	BIG/Little-little/CCI.
 */
#define NR_CLUSTERS_VPROC      3

#define PICACHU_OP_ENABLE       (1 << 0)
#define PICACHU_OP_KE           (1 << 1)
#define PICACHU_OP_CLEAR_EMMC   (1 << 2)

struct picachu_sram_info {
	unsigned int magic;
	int vmin[NR_CAL_OPPS];
	unsigned int timestamp;
	unsigned int checksum;

	/*
	 * Bit[7:0]: MTDES
	 * Bit[15:8]: BDES
	 * Bit[23:16]: MDES
	 */
	unsigned int ptp1_efuse[NR_CLUSTERS_VPROC];

	unsigned int op : 8;
	unsigned int wfe_status : 8;
	unsigned int volt : 8;
	int index : 8;
};

enum mt_vproc_id {
	MT_VPROC1,
	MT_VPROC2,

	NR_VPROC,
};

struct picachu_proc {
	char *name;
	int vproc_id;
	umode_t mode;
};

static struct picachu_sram_info *picachu_data;
static unsigned int picachu_debug;

#if defined(CONFIG_MTK_DISABLE_PICACHU)
static int picachu_enable;
#else
static int picachu_enable = 1;
#endif

static void dump_picachu_info(struct seq_file *m, struct picachu_sram_info *info)
{
	int i;

	seq_printf(m, "0x%X\n", info->magic);
	for (i = 0; i < NR_CAL_OPPS; i++)
		seq_printf(m, "0x%X\n", info->vmin[i]);

	seq_printf(m, "0x%X\n", info->timestamp);
	seq_printf(m, "0x%X\n", info->checksum);
	for (i = 0; i < NR_CLUSTERS_VPROC; i++)
		seq_printf(m, "0x%X\n", info->ptp1_efuse[i]);
	seq_printf(m, "0x%X\n", info->op);
}

static int picachu_enable_proc_show(struct seq_file *m, void *v)
{
	struct picachu_sram_info *p = (struct picachu_sram_info *) m->private;

	if (p != NULL)
		seq_printf(m, "0x%X\n", !!(p->op & PICACHU_OP_ENABLE));

	return 0;
}

static ssize_t picachu_enable_proc_write(struct file *file,
				     const char __user *buffer, size_t count, loff_t *pos)
{
	char *buf = (char *) __get_free_page(GFP_USER);
	struct picachu_sram_info *p;
	int enable;
	int ret;

	if (!buf)
		return -ENOMEM;

	p = (struct picachu_sram_info *) PDE_DATA(file_inode(file));
	if (!p)
		return -EINVAL;

	ret = -EINVAL;

	if (count >= PAGE_SIZE)
		goto out;

	ret = -EFAULT;

	if (copy_from_user(buf, buffer, count))
		goto out;

	buf[count] = '\0';

	if (kstrtoint(buf, 10, &enable)) {
		ret = -EINVAL;
		picachu_info("bad argument! Should be 0/1\n");
	} else {
		ret = 0;
		if (!enable)
			p->op &= ~PICACHU_OP_ENABLE;
		else
			p->op |= PICACHU_OP_ENABLE;
	}

out:
	free_page((unsigned long)buf);

	return (ret < 0) ? ret : count;
}

static int picachu_clear_emmc_proc_show(struct seq_file *m, void *v)
{
	struct picachu_sram_info *p = (struct picachu_sram_info *) m->private;

	if (p != NULL)
		seq_printf(m, "0x%X\n", !!(p->op & PICACHU_OP_CLEAR_EMMC));

	return 0;
}

static ssize_t picachu_clear_emmc_proc_write(struct file *file,
					     const char __user *buffer,
					     size_t count, loff_t *pos)
{
	char *buf = (char *) __get_free_page(GFP_USER);
	struct picachu_sram_info *p;
	int clear_emmc;
	int ret;

	if (!buf)
		return -ENOMEM;

	p = (struct picachu_sram_info *) PDE_DATA(file_inode(file));
	if (!p)
		return -EINVAL;

	ret = -EINVAL;

	if (count >= PAGE_SIZE)
		goto out;

	ret = -EFAULT;

	if (copy_from_user(buf, buffer, count))
		goto out;

	buf[count] = '\0';

	if (kstrtoint(buf, 10, &clear_emmc)) {
		ret = -EINVAL;
		picachu_dbg("bad argument!! Should be 0/1\n");
	} else {
		ret = 0;
		if (!clear_emmc)
			p->op &= ~PICACHU_OP_CLEAR_EMMC;
		else
			p->op |= PICACHU_OP_CLEAR_EMMC;
	}

out:
	free_page((unsigned long)buf);

	return (ret < 0) ? ret : count;
}

static int picachu_dump_proc_show(struct seq_file *m, void *v)
{
	struct picachu_sram_info *p = (struct picachu_sram_info *) m->private;

	if (p)
		dump_picachu_info(m, p);

	return 0;
}

#define PROC_FOPS_RW(name)					\
	static int name ## _proc_open(struct inode *inode,	\
		struct file *file)				\
	{							\
		return single_open(file, name ## _proc_show,	\
			PDE_DATA(inode));			\
	}							\
	static const struct file_operations name ## _proc_fops = {	\
		.owner          = THIS_MODULE,				\
		.open           = name ## _proc_open,			\
		.read           = seq_read,				\
		.llseek         = seq_lseek,				\
		.release        = single_release,			\
		.write          = name ## _proc_write,			\
	}

#define PROC_FOPS_RO(name)					\
	static int name ## _proc_open(struct inode *inode,	\
		struct file *file)				\
	{							\
		return single_open(file, name ## _proc_show,	\
			PDE_DATA(inode));			\
	}							\
	static const struct file_operations name ## _proc_fops = {	\
		.owner          = THIS_MODULE,				\
		.open           = name ## _proc_open,			\
		.read           = seq_read,				\
		.llseek         = seq_lseek,				\
		.release        = single_release,			\
	}

#define PROC_ENTRY(name)	{__stringify(name), &name ## _proc_fops}

PROC_FOPS_RW(picachu_enable);
PROC_FOPS_RW(picachu_clear_emmc);
PROC_FOPS_RO(picachu_dump);

#define PICACHU_PROC_ENTRY_ATTR	(S_IRUGO | S_IWUSR | S_IWGRP)

static struct picachu_proc picachu_proc_list[] = {
	{"big_2l", MT_VPROC1, PICACHU_PROC_ENTRY_ATTR},
	{"little", MT_VPROC2, PICACHU_PROC_ENTRY_ATTR},
	{0},
};

static int create_procfs_entries(struct proc_dir_entry *dir,
				 struct picachu_proc *proc)
{
	int i, num;

	struct pentry {
		const char *name;
		const struct file_operations *fops;
	};

	struct pentry entries[] = {
		PROC_ENTRY(picachu_enable),
		PROC_ENTRY(picachu_clear_emmc),
		PROC_ENTRY(picachu_dump),
	};

	num = ARRAY_SIZE(entries);

	for (i = 0; i < num; i++) {
		if (!proc_create_data(entries[i].name, proc->mode, dir,
				entries[i].fops,
				(void *) &picachu_data[proc->vproc_id])) {
			picachu_info("[%s]: create /proc/picachu/%s failed\n", __func__, entries[i].name);
			return -ENOMEM;
		}
	}

	return 0;
}

static int create_procfs(void)
{
	struct picachu_proc *proc;
	struct proc_dir_entry *root, *dir;
	int ret;

	root = proc_mkdir("picachu", NULL);
	if (!root) {
		picachu_info("[%s]: mkdir /proc/picachu failed\n", __func__);
		return -ENOMEM;
	}

	for (proc = picachu_proc_list; proc->name; proc++) {
		dir = proc_mkdir(proc->name, root);
		if (!dir) {
			picachu_info("[%s]: mkdir /proc/picachu/%s failed\n",
					__func__, proc->name);
			return -ENOMEM;
		}

		ret = create_procfs_entries(dir, proc);
		if (ret)
			return ret;
	}

	return 0;
}

static void picachu_update_ptp1_efuse(int *eem_ctrl_id_ptr,
				     unsigned int *ptp1_efuse)
{
	unsigned int i = 0;

	for (i = 0; i < NR_CLUSTERS_VPROC; i++) {

		if (eem_ctrl_id_ptr[i] == -1)
			break;

		/* FIXME: The API should be exported by EEM driver. */
		/* eem_set_pi_efuse(eem_ctrl_id_ptr[i], ptp1_efuse[i]); */
	}

}

static int __init picachu_init(void)
{
	int i;
	struct picachu_sram_info *pd;

	int eem_ctrl_id_by_vproc[NR_VPROC][NR_CLUSTERS_VPROC] = {
		{EEM_CTRL_BIG, EEM_CTRL_CCI, EEM_CTRL_2L},
		{EEM_CTRL_L, -1, -1}
	};

	picachu_data = (struct picachu_sram_info *)
			ioremap_nocache(PICACHU_BASE, PICACHU_SIZE);

	if (!picachu_data)
		return -ENOMEM;

	for (i = 0; i < NR_VPROC; i++) {
		pd = &picachu_data[i];
		if (!pd) {
			picachu_info("Error: pd is NULL, cluster id: %d!\n", i);
			continue;
		}

		if (picachu_enable)
			pd->op |= PICACHU_OP_ENABLE;
		else
			pd->op &= ~PICACHU_OP_ENABLE;

		picachu_update_ptp1_efuse(&eem_ctrl_id_by_vproc[i][0],
					 &pd->ptp1_efuse[0]);
	}

	create_procfs();

	return 0;
}

static void __exit picachu_exit(void)
{
	picachu_dbg("Picachu de-initialization\n");

	if (picachu_data)
		iounmap(picachu_data);

}

module_init(picachu_init);
module_exit(picachu_exit);

MODULE_DESCRIPTION("MediaTek Picachu Driver v0.1");
MODULE_LICENSE("GPL");
