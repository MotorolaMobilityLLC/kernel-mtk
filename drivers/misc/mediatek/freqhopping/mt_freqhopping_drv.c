/*
 * Copyright (C) 2011 MediaTek, Inc.
 *
 * Author: Holmes Chiou <holmes.chiou@mediatek.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/vmalloc.h>
#include <linux/dma-mapping.h>
#include <linux/seq_file.h>
#include "mt_freqhopping_drv.h"

#define SUPPORT_SLT_TEST 0

static struct mt_fh_hal_driver *g_p_fh_hal_drv;
static struct fh_pll_t *g_fh_drv_pll;

#if SUPPORT_SLT_TEST
static int fhctl_slt_test_result = -2;

static int freqhopping_slt_test_proc_read(struct seq_file *m, void *v)
{
	/* This FH_MSG_NOTICE doesn't need to be replaced by seq_file *m
	 * because userspace parser only needs to know fhctl_slt_test_result
	 */
	pr_info("[FH] %s()\n", __func__);
	pr_info("[FH] fhctl slt result info\n");
	pr_info("[FH] return 0 => [SLT]FHCTL Test Pass!\n");
	pr_info("[FH] return -1 => [SLT]FHCTL Test Fail!\n");
	pr_info("[FH] return (result != 0 && result != -1) ");
	pr_info("[FH] => [SLT]FHCTL Test Untested!\n");

	seq_printf(m, "%d\n", fhctl_slt_test_result);

	fhctl_slt_test_result = -2; /* reset fhctl slt test */

	return 0;
}

static ssize_t freqhopping_slt_test_proc_write(struct file *file,
	const char *buffer, size_t count, loff_t *data)
{
	char *cmd = (char *)__get_free_page(GFP_USER);

	pr_info("[FH] %s()\n", __func__);

	if (cmd == NULL)
		return -ENOMEM;

	if (copy_from_user(cmd, buffer, count))
		goto out;

	cmd[count-1] = '\0';

	if (!strcmp(cmd, "slt_start"))
		fhctl_slt_test_result = g_p_fh_hal_drv->mt_fh_hal_slt_start();
	else
		pr_info("[FH]unknown cmd = %s\n", cmd);
out:
	free_page((unsigned long)cmd);

	return count;
}

static int freqhopping_slt_test_proc_open(struct inode *inode
	, struct file *file)
{
	return single_open(file, freqhopping_slt_test_proc_read, NULL);
}

static const struct file_operations slt_test_fops = {
	.owner = THIS_MODULE,
	.open = freqhopping_slt_test_proc_open,
	.read = seq_read,
	.write = freqhopping_slt_test_proc_write,
	.release = single_release,
};
#endif /* SUPPORT_SLT_TEST */

static int freqhopping_dumpregs_proc_open(struct inode *inode
	, struct file *file)
{
	return single_open(file, g_p_fh_hal_drv->mt_fh_hal_dumpregs_read, NULL);
}

static const struct file_operations dumpregs_fops = {
	.owner = THIS_MODULE,
	.open = freqhopping_dumpregs_proc_open,
	.read = seq_read,
	.release = single_release,
};

int mt_dfs_armpll(unsigned int pll, unsigned int dds)
{
	if (!g_p_fh_hal_drv) {
		pr_info("[FH] [%s]: g_p_fh_hal_drv is uninitialized."
			, __func__);
		return 1;
	}

	return g_p_fh_hal_drv->mt_dfs_armpll(pll, dds);
}

static int freqhopping_debug_proc_init(void)
{
	struct proc_dir_entry *prdumpregentry;
#if SUPPORT_SLT_TEST
	struct proc_dir_entry *prslttestentry;
#endif /* SUPPORT_SLT_TEST */
	struct proc_dir_entry *fh_proc_dir = NULL;

	pr_info("[FH] %s", __func__);

	fh_proc_dir = proc_mkdir("freqhopping", NULL);
	if (fh_proc_dir == NULL) {
		pr_info("[FH]proc_mkdir fail!");
		return -EINVAL;
	}

	/* /proc/freqhopping/dumpregs */
	prdumpregentry
		= proc_create("dumpregs", 0664, fh_proc_dir, &dumpregs_fops);
	if (prdumpregentry == NULL) {
		pr_info("[FH][%s]: failed to create /proc/freqhopping/dumpregs"
			, __func__);
		return -EINVAL;
	}
#if SUPPORT_SLT_TEST
	/* /proc/freqhopping/slt_test */
	prslttestentry
		= proc_create("slt_test", 0664, fh_proc_dir, &slt_test_fops);
	if (prslttestentry == NULL) {
		pr_info("[FH][%s]: failed to create /proc/freqhopping/slt_test"
			, __func__);
		return -EINVAL;
	}
#endif /* SUPPORT_SLT_TEST */
	return 0;
}

int freqhopping_config(unsigned int pll_id
	, unsigned long vco_freq, unsigned int enable)
{
	struct freqhopping_ioctl fh_ctl;
	unsigned int fh_status;
	unsigned long flags = 0;
	unsigned int skip_flag = 0;

	if ((g_p_fh_hal_drv->mt_fh_get_init()) == 0) {
		pr_info("[FH]Not init yet, init first.");
		return 1;
	}

	g_p_fh_hal_drv->mt_fh_lock(&flags);

	/* backup */
	fh_status = g_fh_drv_pll[pll_id].fh_status;

	g_fh_drv_pll[pll_id].curr_freq = vco_freq;

	g_fh_drv_pll[pll_id].pll_status
		= (enable > 0) ? FH_PLL_ENABLE : FH_PLL_DISABLE;

	/* prepare freqhopping_ioctl */
	fh_ctl.pll_id = pll_id;

	if (g_fh_drv_pll[pll_id].fh_status != FH_FH_DISABLE)
		g_p_fh_hal_drv->mt_fh_hal_ctrl(&fh_ctl, enable);
	else
		skip_flag = 1;

	/* restore */
	g_fh_drv_pll[pll_id].fh_status = fh_status;

	g_p_fh_hal_drv->mt_fh_unlock(&flags);

	return 0;
}

static int mt_freqhopping_init(void)
{
	int ret;

	g_p_fh_hal_drv = mt_get_fh_hal_drv();
	if (g_p_fh_hal_drv == NULL) {
		pr_info("[FH]No fh driver is found\n");
		return -EINVAL;
	}

	ret = g_p_fh_hal_drv->mt_fh_hal_init();
	if (ret != 0)
		return ret;

	g_fh_drv_pll = g_p_fh_hal_drv->fh_pll;

	g_p_fh_hal_drv->mt_fh_hal_default_conf();

	ret = freqhopping_debug_proc_init();
	if (ret != 0)
		return ret;

	return 0;
}

static int __init mt_freqhopping_drv_init(void)
{
	int ret;

	ret = mt_freqhopping_init();

	return ret;
}

subsys_initcall(mt_freqhopping_drv_init);

