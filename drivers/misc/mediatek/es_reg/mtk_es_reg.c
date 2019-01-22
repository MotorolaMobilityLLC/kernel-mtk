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
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

static struct proc_dir_entry *es_reg_dump_file;

void  __attribute__((weak)) mtk_es_reg_dump(struct seq_file *m, void *v) {}
int __attribute__((weak)) mtk_es_reg_init(void) { return 0; }

static int es_reg_dump_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%s model:\n", CONFIG_MTK_PLATFORM);
	mtk_es_reg_dump(m, v);
	return 0;
}

static int es_reg_dump_open(struct inode *inode, struct file *file)
{
	return single_open(file, es_reg_dump_show, NULL);
}

static const struct file_operations es_reg_dump_fops = {
	.owner	= THIS_MODULE,
	.open	= es_reg_dump_open,
	.read	= seq_read,
	.llseek	= seq_lseek,
	.release = single_release,
};

static int __init es_reg_dump_init(void)
{
	int ret;

	ret = mtk_es_reg_init();
	if (unlikely(ret))
		return ret;

	es_reg_dump_file = proc_create("mtk_es_reg_dump", 0, NULL, &es_reg_dump_fops);
	if (unlikely(!es_reg_dump_file))
		return -ENOMEM;

	return 0;
}

static void __exit es_reg_dump_exit(void)
{
	remove_proc_entry("mtk_es_reg_dump", NULL);
}

module_init(es_reg_dump_init);
module_exit(es_reg_dump_exit);

MODULE_LICENSE("GPL");
