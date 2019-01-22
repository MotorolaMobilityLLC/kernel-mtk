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

#ifdef pr_fmt
#undef pr_fmt
#endif

#define pr_fmt(fmt) "["KBUILD_MODNAME"]" fmt

#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <mt-plat/mtk_gpt.h>
#include <linux/io.h>
#include <linux/scatterlist.h>

#include "ufs-dbg.h"
#include "ufs-mtk.h"


#ifndef FPGA_PLATFORM
#include <mt-plat/upmu_common.h>
#endif

static char cmd_buf[256];

static int ufs_help_proc_show(struct seq_file *m, void *v)
{
	seq_puts(m, "\n===============[ufs_help]================\n");

	seq_printf(m, "\n   Commands dump:        echo %x [host_id] > ufs_debug\n",
		UFS_CMDS_DUMP);

	seq_printf(m, "\n   Get Power Mode Status:        echo %x [host_id] > ufs_debug\n",
		UFS_GET_PWR_MODE);

	seq_puts(m, "\n   NOTE: All input data is Hex number!\n");

	seq_puts(m, "\n=============================================\n\n");

	return 0;
}

int ufs_g_count;
/* ========== driver proc interface =========== */
static int ufs_debug_proc_show(struct seq_file *m, void *v)
{
	int cmd = -1;
	int sscanf_num;
	int p1, p2, p3, p4, p5, p6, p7, p8;

	p1 = p2 = p3 = p4 = p5 = p6 = p7 = p8 = -1;

	cmd_buf[ufs_g_count] = '\0';
	seq_printf(m, "Debug Command:  %s\n", cmd_buf);

	sscanf_num = sscanf(cmd_buf, "%x %x %x %x %x %x %x %x %x", &cmd,
		&p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8);

	if (cmd == -1) {
		seq_puts(m, "Please run: echo cmd [p1] [p2] [p3] [p4] [p5] [p6] [p7] [p8]> /proc/ufs_debug\n");
		seq_puts(m, "For help run: cat /proc/ufs_help\n");
		return 0;
	}

	if (cmd == UFS_CMDS_DUMP) {
		ufs_mtk_dbg_proc_dump(m);
	} else if (cmd == UFS_GET_PWR_MODE) {
		seq_printf(m, "Power Mode: tx 0x%x rx 0x%x (1:FAST 2:SLOW 4:FAST_AUTO 5:SLOW_AUTO 7:UNCHANGE)\n"
			, ufs_mtk_hba->pwr_info.pwr_tx, ufs_mtk_hba->pwr_info.pwr_rx);
		seq_printf(m, "Gear: tx 0x%x rx 0x%x\n", ufs_mtk_hba->pwr_info.gear_tx
			, ufs_mtk_hba->pwr_info.gear_rx);
		seq_printf(m, "HS Rate: 0x%x (1:HS_A 2:HS_B)\n", ufs_mtk_hba->pwr_info.hs_rate);
		seq_printf(m, "Lanes: tx 0x%x rx 0x%x\n", ufs_mtk_hba->pwr_info.lane_tx
			, ufs_mtk_hba->pwr_info.lane_rx);
	}

	return 0;
}

static ssize_t ufs_debug_proc_write(struct file *file, const char *buf,
	size_t count, loff_t *data)
{
	int ret;

	if (count == 0)
		return -1;
	if (count > 255)
		count = 255;
	ufs_g_count = count;
	ret = copy_from_user(cmd_buf, buf, count);
	if (ret < 0)
		return -1;
	return count;
}

static int ufs_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, ufs_debug_proc_show, inode->i_private);
}

static const struct file_operations ufs_proc_fops = {
	.open = ufs_proc_open,
	.write = ufs_debug_proc_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int ufs_help_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, ufs_help_proc_show, inode->i_private);
}
static const struct file_operations ufs_help_fops = {
	.open = ufs_help_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

#ifndef USER_BUILD_KERNEL
#define PROC_PERM		0660
#else
#define PROC_PERM		0440
#endif

int ufs_mtk_debug_proc_init(void)
{
	struct proc_dir_entry *prEntry;
	kuid_t uid;
	kgid_t gid;

	uid = make_kuid(&init_user_ns, 0);
	gid = make_kgid(&init_user_ns, 1001);

	prEntry = proc_create("ufs_debug", PROC_PERM, NULL, &ufs_proc_fops);

	if (prEntry)
		proc_set_user(prEntry, uid, gid);
	else
		pr_info("[%s]: failed to create /proc/ufs_debug\n", __func__);

	prEntry = proc_create("ufs_help", PROC_PERM, NULL, &ufs_help_fops);

	if (!prEntry)
		pr_info("[%s]: failed to create /proc/ufs_help\n", __func__);

	return 0;
}
EXPORT_SYMBOL_GPL(ufs_mtk_debug_proc_init);
