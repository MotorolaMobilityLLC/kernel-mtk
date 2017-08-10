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
unsigned int gEnableUartLog = 0;
unsigned int gMobilelog = 1;
unsigned int gFencelog = 0; /*Fence Log*/
unsigned int gLoglevel = 3; /*DISPMSG level is DEFAULT_LEVEL==3*/
unsigned int gRcdlevel = 0;
unsigned char pq_debug_flag = 0;
unsigned char aal_debug_flag = 0;
unsigned int gUltraEnable = 1;
int lcm_mode_status = 0;
int bypass_blank = 0;
struct dentry *disp_debugDir;

/* --------------------------------------------------------------------------- */
/* Local variable declarations */
/* --------------------------------------------------------------------------- */
static struct dentry *dispsys_debugfs;
static struct dentry *dump_debugfs;
static struct dentry *mtkfb_debugfs;
static int debug_init;
static unsigned int dbg_log_level;
static unsigned int irq_log_level;
static unsigned int dump_to_buffer;
static char debug_buffer[4096 + DPREC_ERROR_LOG_BUFFER_LENGTH];
static char DDP_STR_HELP[] =
	"USAGE:\n"
	"       echo [ACTION]>/d/dispsys\n"
	"ACTION:\n"
	"       regr:addr\n              :regr:0xf400c000\n"
	"       regw:addr,value          :regw:0xf400c000,0x1\n"
	"       dbg_log:0|1|2            :0 off, 1 dbg, 2 all\n"
	"       irq_log:0|1              :0 off, !0 on\n"
	"       met_on:[0|1],[0|1],[0|1] :fist[0|1]on|off,other [0|1]direct|decouple\n"
	"       backlight:level\n"
	"       dump_aal:arg\n"
	"       mmp\n"
	"       dump_reg:moduleID\n"
	"       dump_path:mutexID\n"
	"       dpfd_ut1:channel\n";

/* --------------------------------------------------------------------------- */
/* DDP debugfs functions */
/* --------------------------------------------------------------------------- */
static char dbg_buf[2048];

static unsigned int is_reg_addr_valid(unsigned int isVa, unsigned long addr)
{
	unsigned int i = 0;

	for (i = 0; i < DISP_REG_NUM; i++) {
		if ((isVa == 1) && (addr > dispsys_reg[i]) && (addr < dispsys_reg[i] + 0x1000))
			break;
		if ((isVa == 0) && (addr > ddp_reg_pa_base[i])
		    && (addr < ddp_reg_pa_base[i] + 0x1000))
			break;
	}

	if (i < DISP_REG_NUM) {
		DISPMSG("addr valid, isVa=0x%x, addr=0x%lx, module=%s!\n", isVa, addr,
		       ddp_get_reg_module_name(i));
		return 1;
	}

	DISPERR("is_reg_addr_valid return fail, isVa=0x%x, addr=0x%lx!\n", isVa, addr);
	return 0;
}

static void ddp_process_dbg_opt(const char *opt)
{
	int ret = 0;
	char *buf = dbg_buf + strlen(dbg_buf);

	if (0 == strncmp(opt, "regr:", 5)) {
		char *p = (char *)opt + 5;
		unsigned long addr = 0;

		ret = kstrtoul(p, 16, &addr);
		if (ret) {
			snprintf(buf, 50, "error to parse cmd %s\n", opt);
			return;
		}

		if (is_reg_addr_valid(1, addr) == 1) {
			unsigned int regVal = DISP_REG_GET(addr);

			DISPMSG("regr: 0x%lx = 0x%08X\n", addr, regVal);
			sprintf(buf, "regr: 0x%lx = 0x%08X\n", addr, regVal);
		} else {
			sprintf(buf, "regr, invalid address 0x%lx\n", addr);
			goto Error;
		}
	} else if (0 == strncmp(opt, "lfr_update", 3)) {
		DSI_LFR_UPDATE(DISP_MODULE_DSI0, NULL);
	} else if (0 == strncmp(opt, "regw:", 5)) {
		unsigned long addr;
		unsigned int val;

		ret = sscanf(opt, "regw:0x%lx,0x%x\n", &addr, &val);
		if (ret != 2) {
			snprintf(buf, 50, "error to parse cmd %s\n", opt);
			return;
		}

		if (is_reg_addr_valid(1, addr) == 1) {
			unsigned int regVal;

			DISP_CPU_REG_SET(addr, val);
			regVal = DISP_REG_GET(addr);
			DISPMSG("regw: 0x%lx, 0x%08X = 0x%08X\n", addr, val, regVal);
			sprintf(buf, "regw: 0x%lx, 0x%08X = 0x%08X\n", addr, val, regVal);
		} else {
			sprintf(buf, "regw, invalid address 0x%lx\n", addr);
			goto Error;
		}
	} else if (0 == strncmp(opt, "dbg_log:", 8)) {
		char *p = (char *)opt + 8;
		unsigned int enable;

		ret = kstrtouint(p, 0, &enable);
		if (ret) {
			snprintf(buf, 50, "error to parse cmd %s\n", opt);
			return;
		}

		if (enable) {
			dbg_log_level = 1;
			gFencelog = 1;
			gLoglevel = 5;
		} else {
			dbg_log_level = 0;
			gFencelog = 0;
			gLoglevel = 3;
		}

		sprintf(buf, "dbg_log: %d\n", dbg_log_level);
	} else if (0 == strncmp(opt, "irq_log:", 8)) {
		char *p = (char *)opt + 8;
		unsigned int enable;

		ret = kstrtouint(p, 0, &enable);
		if (ret) {
			snprintf(buf, 50, "error to parse cmd %s\n", opt);
			return;
		}
		if (enable) {
			irq_log_level = 1;
			gLoglevel = 6;
		} else {
			irq_log_level = 0;
			gLoglevel = 3;
		}

		sprintf(buf, "irq_log: %d\n", irq_log_level);
	} else if (0 == strncmp(opt, "met_on:", 7)) {
		int met_on, rdma0_mode, rdma1_mode;

		ret = sscanf(opt, "met_on:%d,%d,%d\n", &met_on, &rdma0_mode, &rdma1_mode);
		if (ret != 3) {
			snprintf(buf, 50, "error to parse cmd %s\n", opt);
			return;
		}
		ddp_init_met_tag(met_on, rdma0_mode, rdma1_mode);
		DISPMSG("process_dbg_opt, met_on=%d,rdma0_mode %d, rdma1 %d\n", met_on, rdma0_mode,
		       rdma1_mode);
		sprintf(buf, "met_on:%d,rdma0_mode:%d,rdma1_mode:%d\n", met_on, rdma0_mode,
			rdma1_mode);
	} else if (0 == strncmp(opt, "backlight:", 10)) {
		char *p = (char *)opt + 10;
		unsigned int level;

		ret = kstrtouint(p, 0, &level);
		if (ret) {
			snprintf(buf, 50, "error to parse cmd %s\n", opt);
			return;
		}

		if (level) {
			/*disp_bls_set_backlight(level);*/
			sprintf(buf, "backlight: %d\n", level);
		} else {
			goto Error;
		}
	} else if (0 == strncmp(opt, "pwm0:", 5) || 0 == strncmp(opt, "pwm1:", 5)) {
		char *p = (char *)opt + 5;
		unsigned int level;

		ret = kstrtouint(p, 0, &level);
		if (ret) {
			snprintf(buf, 50, "error to parse cmd %s\n", opt);
			return;
		}

		if (level) {
			disp_pwm_id_t pwm_id = DISP_PWM0;

			if (opt[3] == '1')
				pwm_id = DISP_PWM1;

			/*disp_pwm_set_backlight(pwm_id, level);*/
			sprintf(buf, "PWM 0x%x : %d\n", pwm_id, level);
		} else {
			goto Error;
		}
	} else if (0 == strncmp(opt, "rdma_color:", 11)) {
			unsigned int red, green, blue;

			rdma_color_matrix matrix;
			rdma_color_pre pre = { 0 };
			rdma_color_post post = { 255, 0, 0 };

			memset(&matrix, 0, sizeof(matrix));

			ret = sscanf(opt, "rdma_color:%d,%d,%d\n", &red, &green, &blue);
			if (ret != 3) {
				snprintf(buf, 50, "error to parse cmd %s\n", opt);
				return;
			}

			post.ADD0 = red;
			post.ADD1 = green;
			post.ADD2 = blue;
			rdma_set_color_matrix(DISP_MODULE_RDMA0, &matrix, &pre, &post);
			rdma_enable_color_transform(DISP_MODULE_RDMA0);
	} else if (0 == strncmp(opt, "rdma_color:off", 14)) {
		rdma_disable_color_transform(DISP_MODULE_RDMA0);
	} else if (0 == strncmp(opt, "aal_dbg:", 8)) {
		char *p = (char *)opt + 8;

		ret = kstrtouint(p, 0, &aal_dbg_en);
		if (ret) {
				snprintf(buf, 50, "error to parse cmd %s\n", opt);
				return;
		}
		sprintf(buf, "aal_dbg_en = 0x%x\n", aal_dbg_en);
	} else if (0 == strncmp(opt, "aal_test:", 9)) {
		aal_test(opt + 9, buf);
	} else if (0 == strncmp(opt, "pwm_test:", 9)) {
		disp_pwm_test(opt + 9, buf);
	} else if (0 == strncmp(opt, "dither_test:", 12)) {
		dither_test(opt + 12, buf);
	} else if (0 == strncmp(opt, "corr_dbg:", 9)) {
		int i;

		i = 0;
	} else if (0 == strncmp(opt, "dump_reg:", 9)) {
		char *p = (char *)opt + 9;
		unsigned int module;

		ret = kstrtouint(p, 0, &module);
		if (ret) {
			snprintf(buf, 50, "error to parse cmd %s\n", opt);
			return;
		}

		DISPMSG("process_dbg_opt, module=%d\n", module);
		if (module < DISP_MODULE_NUM) {
			ddp_dump_reg(module);
			sprintf(buf, "dump_reg: %d\n", module);
		} else {
			DISPMSG("process_dbg_opt2, module=%d\n", module);
			goto Error;
		}
	} else if (0 == strncmp(opt, "dump_path:", 10)) {
		char *p = (char *)opt + 10;
		unsigned int mutex_idx;

		ret = kstrtouint(p, 0, &mutex_idx);
		if (ret) {
			snprintf(buf, 50, "error to parse cmd %s\n", opt);
			return;
		}

		DISPMSG("process_dbg_opt, path mutex=%d\n", mutex_idx);
		dpmgr_debug_path_status(mutex_idx);
		sprintf(buf, "dump_path: %d\n", mutex_idx);

	} else if (0 == strncmp(opt, "get_module_addr", 15)) {
		unsigned int i = 0;
		char *buf_temp = buf;

		for (i = 0; i < DISP_REG_NUM; i++) {
			DISPDMP("i=%d, module=%s, va=0x%lx, pa=0x%lx, irq(%d)\n",
				i, ddp_get_reg_module_name(i), dispsys_reg[i],
				ddp_reg_pa_base[i], dispsys_irq[i]);
			sprintf(buf_temp,
				"i=%d, module=%s, va=0x%lx, pa=0x%lx, irq(%d)\n", i,
				ddp_get_reg_module_name(i), dispsys_reg[i],
				ddp_reg_pa_base[i], dispsys_irq[i]);
			buf_temp += strlen(buf_temp);
		}

	} else if (0 == strncmp(opt, "debug:", 6)) {
		char *p = (char *)opt + 6;
		unsigned int enable;

		ret = kstrtouint(p, 0, &enable);
		if (ret) {
			snprintf(buf, 50, "error to parse cmd %s\n", opt);
			return;
		}

		if (enable == 1) {
			DISPMSG("[DDP] debug=1, trigger AEE\n");
			/* aee_kernel_exception("DDP-TEST-ASSERT", "[DDP] DDP-TEST-ASSERT"); */
		} else if (enable == 2) {
			ddp_mem_test();
		} else if (enable == 3) {
			ddp_lcd_test();
		} else if (enable == 4) {
			/* DISPAEE("test 4"); */
		} else if (enable == 12) {
			if (gUltraEnable == 0)
				gUltraEnable = 1;
			else
				gUltraEnable = 0;
			sprintf(buf, "gUltraEnable: %d\n", gUltraEnable);
		}
	} else if (0 == strncmp(opt, "mmp", 3)) {
		init_ddp_mmp_events();
	} else {
		dbg_buf[0] = '\0';
		goto Error;
	}

	return;

Error:
	DISPERR("parse command error!\n%s\n\n%s", opt, DDP_STR_HELP);
}

static void ddp_process_dbg_cmd(char *cmd)
{
	char *tok;

	DISPMSG("cmd: %s\n", cmd);
	memset(dbg_buf, 0, sizeof(dbg_buf));
	while ((tok = strsep(&cmd, " ")) != NULL)
		ddp_process_dbg_opt(tok);
}

static int ddp_debug_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static char cmd_buf[512];

static ssize_t ddp_debug_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
	if (strlen(dbg_buf))
		return simple_read_from_buffer(ubuf, count, ppos, dbg_buf, strlen(dbg_buf));
	else
		return simple_read_from_buffer(ubuf, count, ppos, DDP_STR_HELP, strlen(DDP_STR_HELP));

}

static ssize_t ddp_debug_write(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos)
{
	const int debug_bufmax = sizeof(cmd_buf) - 1;
	size_t ret;

	ret = count;

	if (count > debug_bufmax)
		count = debug_bufmax;

	if (copy_from_user(&cmd_buf, ubuf, count))
		return -EFAULT;

	cmd_buf[count] = 0;

	ddp_process_dbg_cmd(cmd_buf);

	return ret;
}

static const struct file_operations ddp_debug_fops = {
	.read = ddp_debug_read,
	.write = ddp_debug_write,
	.open = ddp_debug_open,
};

unsigned int ddp_debug_dbg_log_level(void)
{
	return dbg_log_level;
}

unsigned int ddp_debug_irq_log_level(void)
{
	return irq_log_level;
}

/* --------------------------------------------------------------------------- */
/* DUMP debugfs functions */
/* --------------------------------------------------------------------------- */
static ssize_t dump_debug_read(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
	char *str = "idlemgr disable mtcmos now, all the regs may 0x00000000\n";

	if (is_mipi_enterulps())
		return simple_read_from_buffer(buf, size, ppos, str, strlen(str));

	dprec_logger_dump_reset();
	dump_to_buffer = 1;

	/* dump all */
	dpmgr_debug_path_status(-1);
	dump_to_buffer = 0;
	return simple_read_from_buffer(buf, size, ppos, dprec_logger_get_dump_addr(),
			       dprec_logger_get_dump_len());

}

static const struct file_operations dump_debug_fops = {
	.read = dump_debug_read,
};

unsigned int ddp_debug_analysis_to_buffer(void)
{
	return dump_to_buffer;
}

/* --------------------------------------------------------------------------- */
/* MTKFB debugfs functions */
/* --------------------------------------------------------------------------- */
static void mtkfb_process_dbg_opt(const char *opt)
{
	int ret;

	if (0 == strncmp(opt, "helper", 6)) {
		/*ex: echo helper:DISP_OPT_BYPASS_OVL,0 > /d/mtkfb */
		char option[100] = "";
		char *tmp;
		int value, i;

		tmp = (char *)opt + 7;
		for (i = 0; i < 100; i++) {
			if (tmp[i] != ',' && tmp[i] != ' ')
				option[i] = tmp[i];
			else
				break;
		}
		tmp += i + 1;
		ret = sscanf(tmp, "%d\n", &value);
		if (ret != 1) {
			pr_err("error to parse cmd %s: %s %s ret=%d\n", opt, option, tmp, ret);
			return;
		}

		DISPMSG("will set option %s to %d\n", option, value);
		disp_helper_set_option_by_name(option, value);
	} else if (0 == strncmp(opt, "switch_mode:", 12)) {
		int session_id = MAKE_DISP_SESSION(DISP_SESSION_PRIMARY, 0);
		int sess_mode;

		ret = sscanf(opt, "switch_mode:%d\n", &sess_mode);
		if (ret != 1) {
			pr_err("error to parse cmd %s\n", opt);
			return;
		}

		primary_display_switch_mode(sess_mode, session_id, 1);
	} else if (0 == strncmp(opt, "dsi_mode:cmd", 12)) {
		lcm_mode_status = 1;
		DISPMSG("switch cmd\n");
	} else if (0 == strncmp(opt, "dsi_mode:vdo", 12)) {
		DISPMSG("switch vdo\n");
		lcm_mode_status = 2;
	} else if (0 == strncmp(opt, "clk_change:", 11)) {
		char *p = (char *)opt + 11;
		unsigned int clk = 0;

		ret = kstrtouint(p, 0, &clk);
		if (ret) {
			pr_err("error to parse cmd %s\n", opt);
			return;
		}
		DISPMSG("clk_change:%d\n", clk);
		primary_display_mipi_clk_change(clk);
	} else if (0 == strncmp(opt, "dsipattern", 10)) {
		char *p = (char *)opt + 11;
		unsigned int pattern;

		ret = kstrtouint(p, 0, &pattern);
		if (ret) {
			pr_err("error to parse cmd %s\n", opt);
			return;
		}

		if (pattern) {
			DSI_BIST_Pattern_Test(DISP_MODULE_DSI0, NULL, true, pattern);
			DISPMSG("enable dsi pattern: 0x%08x\n", pattern);
		} else {
			primary_display_manual_lock();
			DSI_BIST_Pattern_Test(DISP_MODULE_DSI0, NULL, false, 0);
			primary_display_manual_unlock();
			return;
		}
	} else if (0 == strncmp(opt, "bypass_blank:", 13)) {
		char *p = (char *)opt + 13;
		unsigned int blank;

		ret = kstrtouint(p, 0, &blank);
		if (ret) {
			pr_err("error to parse cmd %s\n", opt);
			return;
		}
		if (blank)
			bypass_blank = 1;
		else
			bypass_blank = 0;

	} else if (0 == strncmp(opt, "force_fps:", 9)) {
		unsigned int keep;
		unsigned int skip;

		ret = sscanf(opt, "force_fps:%d,%d\n", &keep, &skip);
		if (ret != 2) {
			pr_err("error to parse cmd %s\n", opt);
			return;
		}

		DISPMSG("force set fps, keep %d, skip %d\n", keep, skip);
		primary_display_force_set_fps(keep, skip);
	} else if (0 == strncmp(opt, "AAL_trigger", 11)) {
		int i = 0;
		disp_session_vsync_config vsync_config;

		for (i = 0; i < 1200; i++) {
			primary_display_wait_for_vsync(&vsync_config);
			dpmgr_module_notify(DISP_MODULE_AAL, DISP_PATH_EVENT_TRIGGER);
		}
	} else if (0 == strncmp(opt, "diagnose", 8)) {
		primary_display_diagnose();
		return;
	} else if (0 == strncmp(opt, "_efuse_test", 11)) {
		primary_display_check_test();
	} else if (0 == strncmp(opt, "dprec_reset", 11)) {
		dprec_logger_reset_all();
		return;
	} else if (0 == strncmp(opt, "suspend", 7)) {
		primary_display_suspend();
		return;
	} else if (0 == strncmp(opt, "resume", 6)) {
		primary_display_resume();
	} else if (0 == strncmp(opt, "ata", 3)) {
		mtkfb_fm_auto_test();
		return;
	} else if (0 == strncmp(opt, "dalprintf", 9)) {
		DAL_Printf("display aee layer test\n");
	} else if (0 == strncmp(opt, "dalclean", 8)) {
		DAL_Clean();
	} else if (0 == strncmp(opt, "daltest", 7)) {
		int i = 1000;

		while (i--) {
			DAL_Printf("display aee layer test\n");
			msleep(20);
			DAL_Clean();
			msleep(20);
		}
	} else if (0 == strncmp(opt, "lfr_setting:", 12)) {
		unsigned int enable;
		unsigned int mode;
		unsigned int type = 0;
		unsigned int skip_num = 1;

		ret = sscanf(opt, "lfr_setting:%d,%d\n", &enable, &mode);
		if (ret != 2) {
			pr_err("error to parse cmd %s\n", opt);
			return;
		}

		DISPMSG("--------------enable/disable lfr--------------\n");
		if (enable) {
			DISPMSG("lfr enable %d mode =%d\n", enable, mode);
			enable = 1;
			DSI_Set_LFR(DISP_MODULE_DSI0, NULL, mode, type, enable, skip_num);
		} else {
			DISPMSG("lfr disable %d mode=%d\n", enable, mode);
			enable = 0;
			DSI_Set_LFR(DISP_MODULE_DSI0, NULL, mode, type, enable, skip_num);
		}
	} else if (0 == strncmp(opt, "vsync_switch:", 13)) {
		char *p = (char *)opt + 13;
		unsigned int method = 0;

		ret = kstrtouint(p, 0, &method);
		if (ret) {
			pr_err("error to parse cmd %s\n", opt);
			return;
		}
		primary_display_vsync_switch(method);

	} else if (0 == strncmp(opt, "dsi0_clk:", 9)) {
		char *p = (char *)opt + 9;
		uint32_t clk;

		ret = kstrtouint(p, 0, &clk);
		if (ret) {
			pr_err("error to parse cmd %s\n", opt);
			return;
		}
	} else if (0 == strncmp(opt, "detect_recovery", 15)) {
		DISPMSG("primary_display_signal_recovery\n");
		primary_display_signal_recovery();
	} else if (0 == strncmp(opt, "dst_switch:", 11)) {
		char *p = (char *)opt + 11;
		uint32_t mode;

		ret = kstrtouint(p, 0, &mode);
		if (ret) {
			pr_err("error to parse cmd %s\n", opt);
			return;
		}
		primary_display_switch_dst_mode(mode % 2);
		return;
	} else if (0 == strncmp(opt, "cmmva_dprec", 11)) {
		dprec_handle_option(0x7);
	} else if (0 == strncmp(opt, "cmmpa_dprec", 11)) {
		dprec_handle_option(0x3);
	} else if (0 == strncmp(opt, "dprec", 5)) {
		char *p = (char *)opt + 6;
		unsigned int option;

		ret = kstrtouint(p, 0, &option);
		if (ret) {
			pr_err("error to parse cmd %s\n", opt);
			return;
		}
		dprec_handle_option(option);
	} else if (0 == strncmp(opt, "maxlayer", 8)) {
		char *p = (char *)opt + 9;
		unsigned int maxlayer;

		ret = kstrtouint(p, 0, &maxlayer);
		if (ret) {
			pr_err("error to parse cmd %s\n", opt);
			return;
		}

		if (maxlayer)
			primary_display_set_max_layer(maxlayer);
		else
			DISPERR("can't set max layer to 0\n");
	} else if (0 == strncmp(opt, "primary_reset", 13)) {
		primary_display_reset();
	} else if (0 == strncmp(opt, "esd_check", 9)) {
		char *p = (char *)opt + 10;
		unsigned int enable;

		ret = kstrtouint(p, 0, &enable);
		if (ret) {
			pr_err("error to parse cmd %s\n", opt);
			return;
		}
		primary_display_esd_check_enable(enable);
	} else if (0 == strncmp(opt, "esd_recovery", 12)) {
		primary_display_esd_recovery();
	} else if (0 == strncmp(opt, "lcm0_reset", 10)) {
		DISPMSG("lcm0_reset\n");
#if 1
		DISP_CPU_REG_SET(DISPSYS_CONFIG_BASE + 0x150, 1);
		msleep(20);
		DISP_CPU_REG_SET(DISPSYS_CONFIG_BASE + 0x150, 0);
		msleep(20);
		DISP_CPU_REG_SET(DISPSYS_CONFIG_BASE + 0x150, 1);
#else
#ifdef CONFIG_MTK_LEGACY
		mt_set_gpio_mode(GPIO158 | 0x80000000, GPIO_MODE_00);
		mt_set_gpio_dir(GPIO158 | 0x80000000, GPIO_DIR_OUT);
		mt_set_gpio_out(GPIO158 | 0x80000000, GPIO_OUT_ONE);
		msleep(20);
		mt_set_gpio_out(GPIO158 | 0x80000000, GPIO_OUT_ZERO);
		msleep(20);
		mt_set_gpio_out(GPIO158 | 0x80000000, GPIO_OUT_ONE);
#else
		ret = disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_RST_OUT1);
		msleep(20);
		ret |= disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_RST_OUT0);
		msleep(20);
		ret |= disp_dts_gpio_select_state(DTS_GPIO_STATE_LCM_RST_OUT1);
#endif
#endif
	} else if (0 == strncmp(opt, "lcm0_reset0", 11)) {
		DISP_CPU_REG_SET(DDP_REG_BASE_MMSYS_CONFIG + 0x150, 0);
	} else if (0 == strncmp(opt, "lcm0_reset1", 11)) {
		DISP_CPU_REG_SET(DDP_REG_BASE_MMSYS_CONFIG + 0x150, 1);
	} else if (0 == strncmp(opt, "dump_layer:", 11)) {
		if (0 == strncmp(opt + 11, "on", 2)) {
			ret = sscanf(opt, "dump_layer:on,%d,%d,%d\n",
				     &gCapturePriLayerDownX, &gCapturePriLayerDownY, &gCapturePriLayerNum);
			if (ret != 3) {
				pr_err("error to parse cmd %s\n", opt);
				return;
			}

			gCapturePriLayerEnable = 1;
			gCaptureWdmaLayerEnable = 0;
			if (gCapturePriLayerDownX == 0)
				gCapturePriLayerDownX = 20;
			if (gCapturePriLayerDownY == 0)
				gCapturePriLayerDownY = 20;
			DISPMSG("dump_layer En %d DownX %d DownY %d,Num %d", gCapturePriLayerEnable,
			       gCapturePriLayerDownX, gCapturePriLayerDownY, gCapturePriLayerNum);

		} else if (0 == strncmp(opt + 11, "off", 3)) {
			gCapturePriLayerEnable = 0;
			gCaptureWdmaLayerEnable = 0;
			gCapturePriLayerNum = TOTAL_OVL_LAYER_NUM;
			DISPMSG("dump_layer En %d\n", gCapturePriLayerEnable);
		}

	} else if (0 == strncmp(opt, "dump_wdma_layer:", 16)) {
		if (0 == strncmp(opt + 16, "on", 2)) {
			ret = sscanf(opt, "dump_wdma_layer:on,%d,%d\n",
				     &gCapturePriLayerDownX, &gCapturePriLayerDownY);
			if (ret != 2) {
				pr_err("error to parse cmd %s\n", opt);
				return;
			}

			gCaptureWdmaLayerEnable = 1;
			if (gCapturePriLayerDownX == 0)
				gCapturePriLayerDownX = 20;
			if (gCapturePriLayerDownY == 0)
				gCapturePriLayerDownY = 20;
			DISPMSG("dump_wdma_layer En %d DownX %d DownY %d", gCaptureWdmaLayerEnable,
			       gCapturePriLayerDownX, gCapturePriLayerDownY);

		} else if (0 == strncmp(opt + 16, "off", 3)) {
			gCaptureWdmaLayerEnable = 0;
			DISPMSG("dump_layer En %d\n", gCaptureWdmaLayerEnable);
		}
	} else if (0 == strncmp(opt, "dump_rdma_layer:", 16)) {
		if (0 == strncmp(opt + 16, "on", 2)) {
			ret = sscanf(opt, "dump_rdma_layer:on,%d,%d\n",
				     &gCapturePriLayerDownX, &gCapturePriLayerDownY);
			if (ret != 2) {
				pr_err("error to parse cmd %s\n", opt);
				return;
			}

			gCaptureRdmaLayerEnable = 1;
			if (gCapturePriLayerDownX == 0)
				gCapturePriLayerDownX = 20;
			if (gCapturePriLayerDownY == 0)
				gCapturePriLayerDownY = 20;
			DISPMSG("dump_wdma_layer En %d DownX %d DownY %d", gCaptureRdmaLayerEnable,
			       gCapturePriLayerDownX, gCapturePriLayerDownY);

		} else if (0 == strncmp(opt + 16, "off", 3)) {
			gCaptureRdmaLayerEnable = 0;
			DISPMSG("dump_layer En %d\n", gCaptureRdmaLayerEnable);
		}
	} else if (0 == strncmp(opt, "enable_idlemgr:", 15)) {
		char *p = (char *)opt + 15;
		uint32_t flg;

		ret = kstrtouint(p, 0, &flg);
		if (ret) {
			pr_err("error to parse cmd %s\n", opt);
			return;
		}
		enable_idlemgr(flg);
	}

	if (0 == strncmp(opt, "primary_basic_test:", 19)) {
		int layer_num, w, h, fmt, frame_num, vsync;

		ret = sscanf(opt, "primary_basic_test:%d,%d,%d,%d,%d,%d\n",
			     &layer_num, &w, &h, &fmt, &frame_num, &vsync);
		if (ret != 6) {
			pr_err("error to parse cmd %s\n", opt);
			return;
		}

		if (fmt == 0)
			fmt = DISP_FORMAT_RGBA8888;
		else if (fmt == 1)
			fmt = DISP_FORMAT_RGB888;
		else if (fmt == 2)
			fmt = DISP_FORMAT_RGB565;

		/*primary_display_basic_test(layer_num, w, h, fmt, frame_num, vsync);*/
	}

	if (0 == strncmp(opt, "pan_disp_test:", 13)) {
		int frame_num;
		int bpp;

		ret = sscanf(opt, "pan_disp_test:%d,%d\n", &frame_num, &bpp);
		if (ret != 2) {
			pr_err("error to parse cmd %s\n", opt);
			return;
		}

		pan_display_test(frame_num, bpp);
	}

}

static void mtkfb_process_dbg_cmd(char *cmd)
{
	char *tok;

	DISPMSG("[mtkfb_dbg] %s\n", cmd);

	while ((tok = strsep(&cmd, " ")) != NULL)
		mtkfb_process_dbg_opt(tok);
}

static int mtkfb_debug_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

int debug_get_info(unsigned char *stringbuf, int buf_len)
{
	int n = 0;

	DISPFUNC();

	n += mtkfb_get_debug_state(stringbuf + n, buf_len - n);
	DISPMSG("%s,%d, n=%d\n", __func__, __LINE__, n);

	n += primary_display_get_debug_state(stringbuf + n, buf_len - n);
	DISPMSG("%s,%d, n=%d\n", __func__, __LINE__, n);

	n += disp_sync_get_debug_info(stringbuf + n, buf_len - n);
	DISPMSG("%s,%d, n=%d\n", __func__, __LINE__, n);

	n += dprec_logger_get_result_string_all(stringbuf + n, buf_len - n);
	DISPMSG("%s,%d, n=%d\n", __func__, __LINE__, n);

	n += disp_helper_get_option_list(stringbuf + n, buf_len - n);
	DISPMSG("%s,%d, n=%d\n", __func__, __LINE__, n);

	n += dprec_logger_get_buf(DPREC_LOGGER_ERROR, stringbuf + n, buf_len - n);
	DISPMSG("%s,%d, n=%d\n", __func__, __LINE__, n);

	n += dprec_logger_get_buf(DPREC_LOGGER_FENCE, stringbuf + n, buf_len - n);
	DISPMSG("%s,%d, n=%d\n", __func__, __LINE__, n);

	stringbuf[n++] = 0;
	return n;
}

void debug_info_dump_to_printk(char *buf, int buf_len)
{
	int i = 0;
	int n = buf_len;

	for (i = 0; i < n; i += 256)
		DISPMSG("%s", buf + i);
}

static ssize_t mtkfb_debug_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
	const int debug_bufmax = sizeof(debug_buffer) - 1;
	int n = 0;

	/* Debugfs read only fetch 4096 byte each time, thus whole ringbuffer need massive
	 * iteration. We only copy ringbuffer content to debugfs buffer at first time (*ppos = 0)
	 */
	if (*ppos != 0)
		goto out;

	DISPFUNC();

	n = mtkfb_get_debug_state(debug_buffer + n, debug_bufmax - n);

	n += primary_display_get_debug_state(debug_buffer + n, debug_bufmax - n);

	n += disp_sync_get_debug_info(debug_buffer + n, debug_bufmax - n);

	n += dprec_logger_get_result_string_all(debug_buffer + n, debug_bufmax - n);

	n += dprec_logger_get_buf(DPREC_LOGGER_ERROR, debug_buffer + n, debug_bufmax - n);

	n += dprec_logger_get_buf(DPREC_LOGGER_FENCE, debug_buffer + n, debug_bufmax - n);

	n += dprec_logger_get_buf(DPREC_LOGGER_DUMP, debug_buffer + n, debug_bufmax - n);

	n += dprec_logger_get_buf(DPREC_LOGGER_DEBUG, debug_buffer + n, debug_bufmax - n);

out:
	return simple_read_from_buffer(ubuf, count, ppos, debug_buffer, n);
}

static ssize_t mtkfb_debug_write(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos)
{
	const int debug_bufmax = sizeof(debug_buffer) - 1;
	size_t ret;

	ret = count;

	if (count > debug_bufmax)
		count = debug_bufmax;

	if (copy_from_user(&debug_buffer, ubuf, count))
		return -EFAULT;

	debug_buffer[count] = 0;

	mtkfb_process_dbg_cmd(debug_buffer);

	return ret;
}

static const struct file_operations mtkfb_debug_fops = {
	.read = mtkfb_debug_read,
	.write = mtkfb_debug_write,
	.open = mtkfb_debug_open,
};

/* --------------------------------------------------------------------------- */
/* Debugfs register in debug system */
/* --------------------------------------------------------------------------- */
void DBG_Init(void)
{
	if (!debug_init) {
		debug_init = 1;
		mtkfb_debugfs = debugfs_create_file("mtkfb",
						  S_IFREG | S_IRUGO, NULL, (void *)0, &mtkfb_debug_fops);

		dispsys_debugfs = debugfs_create_file("dispsys",
						      S_IFREG | S_IRUGO, NULL, (void *)0, &ddp_debug_fops);

		disp_debugDir = debugfs_create_dir("disp", NULL);
		if (disp_debugDir) {
			dump_debugfs = debugfs_create_file("dump",
							   S_IFREG | S_IRUGO, disp_debugDir, NULL,
							   &dump_debug_fops);

			/* by Chip, sub debugfs define, and sub debugfs must be in disp folder. */
			sub_debug_init();
		}
	}
}

void DBG_Deinit(void)
{
	/* by Chip, sub debugfs remove */
	sub_debug_deinit();

	debugfs_remove(mtkfb_debugfs);
	debugfs_remove(dispsys_debugfs);
	debugfs_remove(dump_debugfs);
	debugfs_remove_recursive(disp_debugDir);
	debug_init = 0;
}
