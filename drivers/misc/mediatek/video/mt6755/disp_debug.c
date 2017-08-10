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

#include <linux/string.h>
#include <linux/time.h>
#include <linux/uaccess.h>
#include <linux/fb.h>
#include <linux/vmalloc.h>
#include <linux/sched.h>
#include <linux/debugfs.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/types.h>
#include <mt-plat/aee.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#ifdef CONFIG_MTK_LEGACY
#include <mach/mt_gpio.h>
#include <cust_gpio_usage.h>
#else
#include "disp_dts_gpio.h"
#endif

#include "m4u.h"
#include "cmdq_def.h"
#include "cmdq_record.h"
#include "cmdq_reg.h"
#include "cmdq_core.h"

#include "lcm_drv.h"
#include "ddp_path.h"
#include "ddp_reg.h"
#include "ddp_drv.h"
#include "ddp_wdma.h"
#include "ddp_hal.h"
#include "ddp_aal.h"
#include "ddp_pwm.h"
#include "ddp_dither.h"
#include "ddp_info.h"
#include "ddp_dsi.h"
#include "ddp_rdma.h"
#include "ddp_manager.h"
#include "ddp_met.h"
#include "disp_log.h"
#include "disp_debug.h"
#include "disp_helper.h"
#include "disp_drv_ddp.h"
#include "disp_recorder.h"
#include "disp_session.h"
#include "disp_lowpower.h"
#include "disp_recovery.h"
#include "disp_assert_layer.h"
#include "mtkfb.h"
#include "mtkfb_fence.h"
#include "mtkfb_debug.h"
#include "primary_display.h"

#pragma GCC optimize("O0")

/* --------------------------------------------------------------------------- */
/* Global variable declarations */
/* --------------------------------------------------------------------------- */

/* --------------------------------------------------------------------------- */
/* Local variable declarations */
/* --------------------------------------------------------------------------- */
static char dbg_buf[2048];
static struct dentry *lowpowermode_debugfs;
static struct dentry *kickdump_debugfs;
static int low_power_cust_mode = LP_CUST_DISABLE;
static unsigned int vfp_backup;
static char STR_HELP[] =
	"USAGE:\n"
	"       echo [ACTION]>/d/disp/lowpowermode\n"
	"ACTION:\n"
	"       low_power_mode:Mode\n"
	"		Mode:0(LP_CUST_DISABLE)|1(LOW_POWER_MODE)|2(JUST_MAKE_MODE)|3(PERFORMANC_MODE)\n";

/* --------------------------------------------------------------------------- */
/* Command Processor */
/* --------------------------------------------------------------------------- */
int get_lp_cust_mode(void)
{
	return low_power_cust_mode;
}
void backup_vfp_for_lp_cust(unsigned int vfp)
{
	vfp_backup = vfp;
}
unsigned int get_backup_vfp(void)
{
	return vfp_backup;
}

static void lp_cust_process_dbg_opt(const char *opt)
{
	int ret = 0;
	char *buf = dbg_buf + strlen(dbg_buf);

	if (0 == strncmp(opt, "low_power_mode:", 15)) {
		char *p = (char *)opt + 15;
		unsigned int mode;

		ret = kstrtouint(p, 0, &mode);

		if (ret) {
			snprintf(buf, 50, "error to parse cmd %s\n", opt);
			return;
		}

		low_power_cust_mode = mode;

	} else {
		dbg_buf[0] = '\0';
		goto Error;
	}

	return;

Error:
	DISPERR("parse command error!\n%s\n\n%s", opt, STR_HELP);
}

static void lp_cust_process_dbg_cmd(char *cmd)
{
	char *tok;

	DISPMSG("cmd: %s\n", cmd);
	memset(dbg_buf, 0, sizeof(dbg_buf));
	while ((tok = strsep(&cmd, " ")) != NULL)
		lp_cust_process_dbg_opt(tok);
}

static char cmd_buf[512];

static ssize_t lp_cust_write(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos)
{
	const int debug_bufmax = sizeof(cmd_buf) - 1;
	size_t ret;

	ret = count;

	if (count > debug_bufmax)
		count = debug_bufmax;

	if (copy_from_user(&cmd_buf, ubuf, count))
		return -EFAULT;

	cmd_buf[count] = 0;

	lp_cust_process_dbg_cmd(cmd_buf);

	return ret;
}

static ssize_t lp_cust_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
	char *mode0 = "low power mode(1)\n";
	char *mode1 = "just make mode(2)\n";
	char *mode2 = "performance mode(3)\n";
	char *mode4 = "unknown mode(n)\n";

	switch (low_power_cust_mode) {
	case LOW_POWER_MODE:
		return simple_read_from_buffer(ubuf, count, ppos, mode0, strlen(mode0));
	case JUST_MAKE_MODE:
		return simple_read_from_buffer(ubuf, count, ppos, mode1, strlen(mode1));
	case PERFORMANC_MODE:
		return simple_read_from_buffer(ubuf, count, ppos, mode2, strlen(mode2));
	default:
		return simple_read_from_buffer(ubuf, count, ppos, mode4, strlen(mode4));
	}

}

static int lp_cust_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static const struct file_operations low_power_cust_fops = {
	.read = lp_cust_read,
	.write = lp_cust_write,
	.open = lp_cust_open,
};

int ddp_mem_test(void)
{
	return -1;
}

int ddp_lcd_test(void)
{
	return -1;
}

static ssize_t kick_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
	return simple_read_from_buffer(ubuf, count, ppos, get_kick_dump(), get_kick_dump_size());
}

static const struct file_operations kickidle_fops = {
	.read = kick_read,
};

void sub_debug_init(void)
{
	lowpowermode_debugfs = debugfs_create_file("lowpowermode",
						   S_IFREG | S_IRUGO, disp_debugDir, NULL, &low_power_cust_fops);
	if (!lowpowermode_debugfs)
		DISPERR("create debug file disp/lowpowermode fail!\n");

	kickdump_debugfs = debugfs_create_file("kickdump",
					       S_IFREG | S_IRUGO, disp_debugDir, NULL, &kickidle_fops);
	if (!kickdump_debugfs)
		DISPERR("create debug file disp/kickdump fail!\n");
}

void sub_debug_deinit(void)
{
	debugfs_remove(lowpowermode_debugfs);
	debugfs_remove(kickdump_debugfs);
}

