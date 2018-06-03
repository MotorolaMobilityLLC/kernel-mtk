/*
 * Copyright (C) 2017 MediaTek Inc.
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

#define PICACHU_SIGNATURE		(0xA5)
#define PICACHU_PTP1_EFUSE_MASK		(0xFFFFFF)
#define PICACHU_VMIN_SHIFT_BIT          (0)
#define PICACHU_OFFSET_SHIFT_BIT        (8)
#define PICACHU_WFE_STATUS_SHIFT_BIT    (16)
#define PICACHU_SIGNATURE_SHIFT_BIT     (24)

#define PICACHU_SUPPORT_CLUSTERS	3

#define EEM_BASEADDR	(0x1100B000)
#define EEM_SIZE	(0x1000)

#define EEMSPARE0	(eem_base_addr + 0xF20)
#define EEMSPARE1	(eem_base_addr + 0xF24)
#define EEMSPARE2	(eem_base_addr + 0xF28)
#define EEMSPARE3	(eem_base_addr + 0xF2C)

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


#define picachu_read(addr)		__raw_readl((void __iomem *)(addr))
#define picachu_write(addr, val)	mt_reg_sync_writel(val, addr)

struct picachu_info {
	int vmin;
	int offset;
	unsigned int wfe_status;

	/*
	 * Bit[7:0]: MTDES
	 * Bit[15:8]: BDES
	 * Bit[23:16]: MDES
	 */
	unsigned int ptp1_efuse[PICACHU_SUPPORT_CLUSTERS]; /* 2L, L and CCI */
};

struct pentry {
	const char *name;
	const struct file_operations *fops;
};

static struct picachu_info picachu_data;
static unsigned int picachu_debug;

static void __iomem *eem_base_addr;

static void dump_picachu_info(struct seq_file *m, struct picachu_info *info)
{
	unsigned int i;

	seq_printf(m, "0x%X\n", info->vmin);
	seq_printf(m, "0x%X\n", info->offset);
	seq_printf(m, "0x%X\n", info->wfe_status);

	for (i = EEM_CTRL_2L; i <= EEM_CTRL_CCI; i++)
		seq_printf(m, "0x%X\n", info->ptp1_efuse[i]);

}

static int picachu_dump_proc_show(struct seq_file *m, void *v)
{
	dump_picachu_info(m, &picachu_data);

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

PROC_FOPS_RO(picachu_dump);

static int create_procfs(void)
{
	int i;
	struct proc_dir_entry *dir = NULL;

	struct pentry entries[] = {
		PROC_ENTRY(picachu_dump),
	};

	dir = proc_mkdir("picachu", NULL);

	if (!dir) {
		picachu_dbg("[%s]: mkdir /proc/picachu failed\n", __func__);
		return -ENOMEM;
	}

	for (i = 0; i < ARRAY_SIZE(entries); i++) {
		if (!proc_create(entries[i].name, S_IRUGO | S_IWUSR | S_IWGRP, dir, entries[i].fops)) {
			picachu_dbg("[%s]: create /proc/picachu/%s failed\n", __func__, entries[i].name);
			return -ENOMEM;
		}
	}

	return 0;
}

static void picachu_get_data(void)
{
	unsigned int i, val, tmp;

	for (i = 0; i < PICACHU_SUPPORT_CLUSTERS; i++) {

		val = picachu_read(EEMSPARE0 + (i << 2));

		tmp = (val >> PICACHU_SIGNATURE_SHIFT_BIT) & 0xff;
		if (tmp != PICACHU_SIGNATURE)
			continue;

		picachu_data.ptp1_efuse[i] = val & PICACHU_PTP1_EFUSE_MASK;
	}

	val = picachu_read(EEMSPARE3);

	tmp = (val >> PICACHU_SIGNATURE_SHIFT_BIT) & 0xff;
	if (tmp != PICACHU_SIGNATURE)
		return;

	picachu_data.vmin = (val >> PICACHU_VMIN_SHIFT_BIT) & 0xff;
	picachu_data.offset = (val >> PICACHU_OFFSET_SHIFT_BIT) & 0xff;
	picachu_data.wfe_status = (val >> PICACHU_WFE_STATUS_SHIFT_BIT) & 0xff;
}

static int __init picachu_init(void)
{
	unsigned int i;

	eem_base_addr = ioremap(EEM_BASEADDR, EEM_SIZE);

	if (!eem_base_addr) {
		picachu_info("eem_base_addr NULL!\n");
		return -ENOMEM;
	}

	picachu_get_data();

	create_procfs();

	/* Update Picachu calibration data if the data is valid. */
	for (i = EEM_CTRL_2L; i <= EEM_CTRL_CCI; i++) {

		if (!picachu_data.ptp1_efuse[i])
			continue;

		picachu_info("ptp1_efuse[%d]: 0x%x\n", i,
						picachu_data.ptp1_efuse[i]);

		eem_set_pi_efuse(i, picachu_data.ptp1_efuse[i]);
	}

	return 0;
}

static void __exit picachu_exit(void)
{
	picachu_dbg("Picachu de-initialization\n");

	if (eem_base_addr)
		iounmap(eem_base_addr);

}

subsys_initcall(picachu_init);

MODULE_DESCRIPTION("MediaTek Picachu Driver v0.1");
MODULE_LICENSE("GPL");
