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

#if defined(CONFIG_MTK_HDMI_SUPPORT)
#include <linux/string.h>
#include <linux/time.h>
#include <linux/uaccess.h>
#include <linux/debugfs.h>

/*#include <mach/mt_typedefs.h>*/
#include <linux/types.h>
/*#include <mach/mt_gpio.h>*/
/*#include <mt-plat/mt_gpio.h>*/
/* #include <cust_gpio_usage.h> */


#include "ddp_hal.h"
#include "ddp_reg.h"
#include "ddp_info.h"
#include "extd_hdmi.h"
#include "external_display.h"

/*extern DDP_MODULE_DRIVER *ddp_modules_driver[DISP_MODULE_NUM];*/
/* --------------------------------------------------------------------------- */
/* External variable declarations */
/* --------------------------------------------------------------------------- */

/* extern LCM_DRIVER *lcm_drv; */
/* --------------------------------------------------------------------------- */
/* Debug Options */
/* --------------------------------------------------------------------------- */


static const char STR_HELP[] =
	"\n"
	"USAGE\n"
	"        echo [ACTION]... > /d/extd\n"
	"\n"
	"ACTION\n"
	"        fakecablein:[enable|disable]\n"
	"	fake mhl cable in/out\n"
	"\n"
	"        echo [ACTION]... > /d/extd\n"
	"\n"
	"ACTION\n"
	"        force_res:0xffff\n"
	"	fix resolution or 3d enable(high 16 bit)\n"
	"\n";

/* TODO: this is a temp debug solution */
/* extern void hdmi_cable_fake_plug_in(void); */
/* extern int hdmi_drv_init(void); */
static void process_dbg_opt(const char *opt)
{
	char *p = NULL;
	unsigned int res = 0;
	int ret = 0;

	if (strncmp(opt, "on", 2) == 0)
		hdmi_power_on();
	else if (strncmp(opt, "off", 3) == 0)
		hdmi_power_off();
	else if (strncmp(opt, "suspend", 7) == 0)
		hdmi_suspend();
	else if (strncmp(opt, "resume", 6) == 0)
		hdmi_resume();
	else if (strncmp(opt, "fakecablein:", 12) == 0) {
		if (strncmp(opt + 12, "enable", 6) == 0)
			hdmi_cable_fake_plug_in();
		else if (strncmp(opt + 12, "disable", 7) == 0)
			hdmi_cable_fake_plug_out();
		else
			goto Error;
	} else if (strncmp(opt, "force_res:", 10) == 0) {
		p = (char *)opt + 10;
		ret = kstrtouint(p, 0, &res);
		hdmi_force_resolution(res);
	} else if (strncmp(opt, "hdmireg", 7) == 0)
		ext_disp_diagnose();
	else if (strncmp(opt, "I2S1:", 5) == 0) {
#ifdef GPIO_MHL_I2S_OUT_WS_PIN
		if (strncmp(opt + 5, "on", 2) == 0) {
			pr_debug("[hdmi][Debug] Enable I2S1\n");
			mt_set_gpio_mode(GPIO_MHL_I2S_OUT_WS_PIN, GPIO_MODE_01);
			mt_set_gpio_mode(GPIO_MHL_I2S_OUT_CK_PIN, GPIO_MODE_01);
			mt_set_gpio_mode(GPIO_MHL_I2S_OUT_DAT_PIN, GPIO_MODE_01);
		} else if (strncmp(opt + 5, "off", 3) == 0) {
			pr_debug("[hdmi][Debug] Disable I2S1\n");
			mt_set_gpio_mode(GPIO_MHL_I2S_OUT_WS_PIN, GPIO_MODE_02);
			mt_set_gpio_mode(GPIO_MHL_I2S_OUT_CK_PIN, GPIO_MODE_01);
			mt_set_gpio_mode(GPIO_MHL_I2S_OUT_DAT_PIN, GPIO_MODE_02);
		}
#endif
	} else if (strncmp(opt, "forceon", 7) == 0)
		hdmi_force_on(false);
	else if (strncmp(opt, "extd_vsync:", 11) == 0) {
		if (strncmp(opt + 11, "on", 2) == 0)
			hdmi_wait_vsync_debug(1);
		else if (strncmp(opt + 11, "off", 3) == 0)
			hdmi_wait_vsync_debug(0);
	} else if (strncmp(opt, "dumpReg", 7) == 0)
		hdmi_dump_vendor_chip_register();
	else if (strncmp(opt, "extd_ut:", 8) == 0) {
		if (strncmp(opt + 8, "on", 2) == 0)
			enable_ut = 1;
		else if (strncmp(opt + 8, "off", 3) == 0)
			enable_ut = 0;
	} else {
		goto Error;
	}
	return;

 Error:
	pr_debug("[extd] parse command error!\n\n%s", STR_HELP);
}

static void process_dbg_cmd(char *cmd)
{
	char *tok;

	pr_debug("[extd] %s\n", cmd);

	while ((tok = strsep(&cmd, " ")) != NULL)
		process_dbg_opt(tok);
}

/* --------------------------------------------------------------------------- */
/* Debug FileSystem Routines */
/* --------------------------------------------------------------------------- */
struct dentry *extd_dbgfs;

static int debug_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static char debug_buffer[2048];

static ssize_t debug_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
	const int debug_bufmax = sizeof(debug_buffer) - 1;
	int n = 0;

	n += scnprintf(debug_buffer + n, debug_bufmax - n, STR_HELP);
	debug_buffer[n++] = 0;

	return simple_read_from_buffer(ubuf, count, ppos, debug_buffer, n);
}

static ssize_t debug_write(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos)
{
	const int debug_bufmax = sizeof(debug_buffer) - 1;
	size_t ret;

	ret = count;

	if (count > debug_bufmax)
		count = debug_bufmax;

	if (copy_from_user(&debug_buffer, ubuf, count))
		return -EFAULT;

	debug_buffer[count] = 0;

	process_dbg_cmd(debug_buffer);

	return ret;
}


static const struct file_operations debug_fops = {
	.read = debug_read,
	.write = debug_write,
	.open = debug_open,
};


void Extd_DBG_Init(void)
{
	extd_dbgfs = debugfs_create_file("extd", S_IFREG | S_IRUGO, NULL, (void *)0, &debug_fops);
}


void Extd_DBG_Deinit(void)
{
	debugfs_remove(extd_dbgfs);
}

#endif
