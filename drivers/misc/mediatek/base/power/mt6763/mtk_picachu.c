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

#define PICACHU_BASE	0x0011C1C0
#define PICACHU_SIZE	0x40

#define PICACHU_BARRIER_START	0x0011C200
#define PICACHU_BARRIER_END	0x0011C210
#define PICACHU_BARRIER_SIZE	(PICACHU_BARRIER_END - PICACHU_BARRIER_START)

#define PARA_PATH       "/dev/block/platform/bootdevice/by-name/para"
#define CFG_ENV_SIZE    0x1000
#define CFG_ENV_OFFSET  0x40000

#define NR_OPPS         1

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


struct picachu_info {
	unsigned int magic;
	int vmin[NR_OPPS];
	int offset;
	unsigned int wfe_status;
	unsigned int timestamp;
	unsigned int checksum;
	unsigned int udi_mbist_max_cpus;
	unsigned int clear_emmc;
	int enable;
};

static struct picachu_info *picachu_data;
static unsigned int picachu_debug;

#if defined(CONFIG_MTK_DISABLE_PICACHU)
static int picachu_enable;
#else
/* FIXME: Disable Picachu temporarily. */
/* static int picachu_enable = 1; */
static int picachu_enable;
#endif

static void dump_picachu_info(struct seq_file *m, struct picachu_info *info)
{
	int i;

	seq_printf(m, "0x%X\n", info->magic);
	for (i = 0; i < NR_OPPS; i++)
		seq_printf(m, "0x%X\n", info->vmin[i]);

	seq_printf(m, "0x%X\n", info->offset);
	seq_printf(m, "0x%X\n", info->wfe_status);
	seq_printf(m, "0x%X\n", info->timestamp);
	seq_printf(m, "0x%X\n", info->checksum);
	seq_printf(m, "0x%X\n", info->clear_emmc);
	seq_printf(m, "0x%X\n", info->enable);
	seq_printf(m, "0x%X\n", info->udi_mbist_max_cpus);
}

static int picachu_enable_proc_show(struct seq_file *m, void *v)
{
	if (picachu_data != NULL)
		seq_printf(m, "0x%X\n", picachu_data->enable);

	return 0;
}

static int picachu_clear_emmc_proc_show(struct seq_file *m, void *v)
{
	if (picachu_data != NULL)
		seq_printf(m, "0x%X\n", picachu_data->clear_emmc);

	return 0;
}

static ssize_t picachu_enable_proc_write(struct file *file,
					 const char __user *buffer,
					 size_t count, loff_t *pos)
{
	char *buf = (char *) __get_free_page(GFP_USER);
	int enable = 0;
	int ret;

	if (!buf)
		return -ENOMEM;

	ret = -EINVAL;

	if (count >= PAGE_SIZE)
		goto out;

	ret = -EFAULT;

	if (copy_from_user(buf, buffer, count))
		goto out;

	buf[count] = '\0';

	if (kstrtoint(buf, 10, &enable)) {
		ret = -EINVAL;
		picachu_dbg("bad argument_1!! argument should be 0/1\n");
	} else {
		ret = 0;
		if (picachu_data != NULL)
			picachu_data->enable = enable;
	}

out:
	free_page((unsigned long)buf);

	return (ret < 0) ? ret : count;
}

static ssize_t picachu_clear_emmc_proc_write(struct file *file,
					     const char __user *buffer,
					     size_t count, loff_t *pos)
{
	char *buf = (char *) __get_free_page(GFP_USER);
	int clear_emmc = 0;
	int ret;

	if (!buf)
		return -ENOMEM;

	ret = -EINVAL;

	if (count >= PAGE_SIZE)
		goto out;

	ret = -EFAULT;

	if (copy_from_user(buf, buffer, count))
		goto out;

	buf[count] = '\0';

	if (kstrtoint(buf, 10, &clear_emmc)) {
		ret = -EINVAL;
		picachu_dbg("bad argument_1!! argument should be 0/1\n");
	} else {
		ret = 0;
		if (picachu_data != NULL)
			picachu_data->clear_emmc = clear_emmc;
	}

out:
	free_page((unsigned long)buf);

	return (ret < 0) ? ret : count;
}
static int picachu_offset_proc_show(struct seq_file *m, void *v)
{
	if (picachu_data != NULL)
		seq_printf(m, "0x%X\n", picachu_data->offset);

	return 0;
}

static ssize_t picachu_offset_proc_write(struct file *file,
				     const char __user *buffer, size_t count, loff_t *pos)
{
	char *buf = (char *) __get_free_page(GFP_USER);
	int offset = 0;
	int ret;

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
		picachu_dbg("bad argument_1!! argument should be 0/1\n");
	} else {
		ret = 0;
		if (picachu_data != NULL) {
			picachu_data->offset = offset;
			eem_set_pi_offset(EEM_CTRL_2L, offset);
			eem_set_pi_offset(EEM_CTRL_L, offset);
		}
	}

out:
	free_page((unsigned long)buf);

	return (ret < 0) ? ret : count;
}

static int picachu_dump_proc_show(struct seq_file *m, void *v)
{
	if (picachu_data != NULL)
		dump_picachu_info(m, picachu_data);

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
PROC_FOPS_RW(picachu_clear_emmc);
PROC_FOPS_RO(picachu_dump);

static int create_procfs(void)
{
	int i;
	struct proc_dir_entry *dir = NULL;
	struct pentry {
		const char *name;
		const struct file_operations *fops;
	};

	struct pentry entries[] = {
		PROC_ENTRY(picachu_enable),
		PROC_ENTRY(picachu_offset),
		PROC_ENTRY(picachu_dump),
		PROC_ENTRY(picachu_clear_emmc),
	};

	dir = proc_mkdir("picachu", NULL);

	if (!dir) {
		picachu_dbg("[%s]: mkdir /proc/picachu failed\n", __func__);
		return -1;
	}

	for (i = 0; i < ARRAY_SIZE(entries); i++) {
		if (!proc_create(entries[i].name, S_IRUGO | S_IWUSR | S_IWGRP, dir, entries[i].fops)) {
			picachu_dbg("[%s]: create /proc/picachu/%s failed\n", __func__, entries[i].name);
			return -3;
		}
	}

	return 0;
}

static int __init picachu_init(void)
{
	int offset = 0;

	picachu_data = (struct picachu_info *) ioremap_nocache(PICACHU_BASE,
								PICACHU_SIZE);

	if (!picachu_data) {
		picachu_err("cannot allocate resource for picachu data\n");
		return -ENOMEM;
	}

	picachu_data->enable = picachu_enable;

	if (picachu_enable == 1) {
		offset = picachu_data->offset;
		picachu_info("pi_off = %d\n", picachu_data->offset);
	}

	eem_set_pi_offset(EEM_CTRL_2L, offset);
	eem_set_pi_offset(EEM_CTRL_L, offset);

	create_procfs();

	if (picachu_data->udi_mbist_max_cpus == 4)
		aee_kernel_warning("PICACHU", "CPU truncate to 4 only");

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
