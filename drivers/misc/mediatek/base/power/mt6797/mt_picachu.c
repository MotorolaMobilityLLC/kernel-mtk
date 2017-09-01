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

#include "mt_ptp.h"

#define PICACHU_BASE	0x0012A250
#define PICACHU_SIZE	0x40

#define PARA_PATH       "/dev/block/platform/mtk-msdc.0/11230000.msdc0/by-name/para"
#define CFG_ENV_SIZE    0x40
#define CFG_ENV_OFFSET  0x40000

#define NR_CAL_OPPS	1

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

#define PICACHU_MAGIC	(0xFF4455EF)

struct picachu_sram_info {
	unsigned int magic;
	int vmin[NR_CAL_OPPS];
	int offset;
	unsigned int timestamp;
	unsigned int checksum;

	/*
	 * Bit[7:0]: MTDES
	 * Bit[15:8]: BDES
	 * Bit[23:16]: MDES
	 */
	unsigned int ptp1_efuse;

	int enable;
	unsigned int ke : 8;
	unsigned int index : 8;
	unsigned int volt : 8;
	unsigned int cluster_id:8;
};

enum mt_cluster_id {
	MT_CLUSTER_LITTLE,
	MT_CLUSTER_BIG,

	NR_CLUSTERS,
};

struct picachu_proc {
	char *name;
	int cluster_id;
	umode_t mode;
};

static struct picachu_sram_info *picachu_data;
static unsigned int picachu_debug;

/* #if defined(CONFIG_MTK_DISABLE_PICACHU) */
/* Disable picachu by default in Everest*/
#if 1
static int picachu_enable;
#else
static int picachu_enable = 1;
#endif

static void picachu_update_offset(int cluster_id, int offset)
{
	switch (cluster_id) {
	case MT_CLUSTER_LITTLE:
		eem_set_pi_offset(EEM_CTRL_L, offset);
		eem_set_pi_offset(EEM_CTRL_2L, offset);
		break;
	case MT_CLUSTER_BIG:
		eem_set_pi_offset(EEM_CTRL_BIG, offset);
		break;
	default:
		break;
	}
}

static void dump_picachu_info(struct seq_file *m, struct picachu_sram_info *info)
{
	int i;

	seq_printf(m, "0x%X\n", info->magic);
	for (i = 0; i < NR_CAL_OPPS; i++)
		seq_printf(m, "0x%X\n", info->vmin[i]);

	seq_printf(m, "0x%X\n", info->offset);
	seq_printf(m, "0x%X\n", info->timestamp);
	seq_printf(m, "0x%X\n", info->checksum);
	seq_printf(m, "0x%X\n", info->ptp1_efuse);
	seq_printf(m, "0x%X\n", info->enable);
}

static void read_picachu_emmc(char *pi_data, unsigned int size,
			      unsigned int offset)
{
	int result = 0;
	int ret = 0;
	loff_t pos = 0;
	struct file *read_fp;

	read_fp = filp_open(PARA_PATH, O_RDONLY, 0);
	if (IS_ERR(read_fp)) {
		result = PTR_ERR(read_fp);
		picachu_err("File open return fail,result=%d,file=%p\n", result, read_fp);

		return;
	}

	pos += (CFG_ENV_OFFSET + offset);
	ret = kernel_read(read_fp, pos, (char *)pi_data, size);
	if (ret < 0)
		picachu_err("Kernel read env fail\n");

	filp_close(read_fp, 0);
}

static int reset_picachu_emmc(unsigned int size, unsigned int offset)
{
	int result = 0;
	int ret = 0;
	loff_t pos = 0;
	struct file *write_fp;
	mm_segment_t old_fs;
	char *pi_data;

	pi_data = kzalloc(size, GFP_KERNEL);
	if (pi_data == NULL)
		return -ENOMEM;

	write_fp = filp_open(PARA_PATH, O_RDWR, 0);
	if (IS_ERR(write_fp)) {
		result = PTR_ERR(write_fp);
		picachu_err("File open return fail,result=%d,file=%p\n",
			     result, write_fp);
		kfree(pi_data);
		return -ENOENT;
	}

	pos += (CFG_ENV_OFFSET + offset);
	ret = kernel_write(write_fp, (char *)pi_data, size, pos);
	if (ret < 0)
		picachu_err("Kernel write env fail\n");

	old_fs = get_fs();
	set_fs(get_ds());
	ret = vfs_fsync(write_fp, 0);
	if (ret < 0)
		picachu_warn("Kernel write env sync fail\n");

	set_fs(old_fs);

	filp_close(write_fp, 0);
	kfree(pi_data);

	return 0;
}

static int picachu_enable_proc_show(struct seq_file *m, void *v)
{
	struct picachu_sram_info *p = (struct picachu_sram_info *) m->private;

	if (p != NULL)
		seq_printf(m, "0x%X\n", p->enable);

	return 0;
}

static ssize_t picachu_enable_proc_write(struct file *file,
				     const char __user *buffer, size_t count, loff_t *pos)
{
	char *buf = (char *) __get_free_page(GFP_USER);
	struct picachu_sram_info *p;
	int enable = 0;
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
		picachu_info("bad argument_1!! argument should be 0/1\n");
	} else {
		ret = 0;
		p->enable = enable;
	}

out:
	free_page((unsigned long)buf);

	return (ret < 0) ? ret : count;
}

static int picachu_offset_proc_show(struct seq_file *m, void *v)
{
	struct picachu_sram_info *p = (struct picachu_sram_info *) m->private;

	if (p != NULL)
		seq_printf(m, "0x%X\n", p->offset);

	return 0;
}

static ssize_t picachu_offset_proc_write(struct file *file,
				     const char __user *buffer, size_t count, loff_t *pos)
{
	char *buf;
	struct picachu_sram_info *p;
	int offset = 0;
	int ret;

	p = (struct picachu_sram_info *) PDE_DATA(file_inode(file));
	if (!p)
		return -EFAULT;

	buf = (char *) __get_free_page(GFP_USER);
	if (!buf)
		return -ENOMEM;

	ret = -EINVAL;

	if (count >= PAGE_SIZE)
		goto out;

	ret = -EFAULT;

	if (copy_from_user(buf, buffer, count))
		goto out;

	buf[count] = '\0';

	if (kstrtoint(buf, 10, &offset)) {
		ret = -EINVAL;
		picachu_info("bad argument_1!! argument should be 0/1\n");
	} else {
		ret = 0;
		p->offset = offset;

		picachu_update_offset(p->cluster_id, offset);
	}

out:
	free_page((unsigned long)buf);

	return (ret < 0) ? ret : count;
}

static int picachu_emmc_proc_show(struct seq_file *m, void *v)
{
	char *pi_data = NULL;
	struct picachu_sram_info *p = (struct picachu_sram_info *) m->private;
	unsigned int size, offset;

	size = sizeof(*p);

	pi_data = kzalloc(size, GFP_KERNEL);
	if (pi_data == NULL)
		return -ENOMEM;

	offset = size * p->cluster_id;
	read_picachu_emmc(pi_data, size, offset);
	dump_picachu_info(m, (struct picachu_sram_info *)pi_data);
	kfree(pi_data);

	return 0;
}

static ssize_t picachu_emmc_proc_write(struct file *file,
				     const char __user *buffer, size_t count, loff_t *pos)
{
	int ret = 0;
	char *buf = (char *) __get_free_page(GFP_USER);
	struct picachu_sram_info *p;
	int offset = 0;

	p = (struct picachu_sram_info *) PDE_DATA(file_inode(file));
	if (!p) {
		picachu_info("SRAM info is NULL.\n");
		return -EFAULT;
	}

	if (!buf)
		return -ENOMEM;


	if (count >= PAGE_SIZE) {
		ret = -EINVAL;
		goto out;
	}

	if (copy_from_user(buf, buffer, count)) {
		ret = -EFAULT;
		goto out;
	}

	buf[count] = '\0';

	if (kstrtoint(buf, 10, &offset) || offset != -1) {
		ret = -EINVAL;
		goto out;
	}

	reset_picachu_emmc(sizeof(*p), sizeof(*p) * p->cluster_id);

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
PROC_FOPS_RW(picachu_offset);
PROC_FOPS_RW(picachu_emmc);
PROC_FOPS_RO(picachu_dump);

#define PICACHU_PROC_ENTRY_ATTR	(S_IRUGO | S_IWUSR | S_IWGRP)

static struct picachu_proc picachu_proc_list[] = {
	{"little", MT_CLUSTER_LITTLE, PICACHU_PROC_ENTRY_ATTR},
	{"big", MT_CLUSTER_BIG, PICACHU_PROC_ENTRY_ATTR},
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
		PROC_ENTRY(picachu_offset),
		PROC_ENTRY(picachu_emmc),
		PROC_ENTRY(picachu_dump),
	};

	num = ARRAY_SIZE(entries);

	for (i = 0; i < num; i++) {
		if (!proc_create_data(entries[i].name, proc->mode, dir,
				entries[i].fops,
				(void *) &picachu_data[proc->cluster_id])) {
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

static int __init picachu_init(void)
{
	struct picachu_sram_info *pd;
	int i;

	picachu_data = (struct picachu_sram_info *)
				ioremap_nocache(PICACHU_BASE, PICACHU_SIZE);
	if (!picachu_data)
		return -ENOMEM;

	for (i = 0; i < NR_CLUSTERS; i++) {
		pd = &picachu_data[i];
		if (!pd) {
			picachu_info("Error: pd is NULL, cluster id: %d!\n", i);
			continue;
		}

		pd->cluster_id = i;

		pd->enable = picachu_enable;

		if (pd->magic != PICACHU_MAGIC)
			continue;

		picachu_dbg("cluster id: %d, pi_off = %d, pi_efuse: 0x%x\n",
					i, pd->offset, pd->ptp1_efuse);

		/* For little cluster. */
		if (pd->cluster_id == MT_CLUSTER_LITTLE) {
			picachu_update_offset(i, pd->offset);
			continue;
		}

		/* Big cluster. */
		eem_set_big_efuse(EEM_CTRL_BIG, pd->ptp1_efuse);
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

late_initcall(picachu_init);

MODULE_DESCRIPTION("MediaTek Picachu Driver v0.1");
MODULE_LICENSE("GPL");
