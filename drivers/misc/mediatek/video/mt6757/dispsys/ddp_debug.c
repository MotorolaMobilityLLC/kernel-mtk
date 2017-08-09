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

#define LOG_TAG "DEBUG"

#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/debugfs.h>
#include "mt-plat/aee.h"
#include "disp_assert_layer.h"
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/time.h>

#include "m4u.h"
#include "m4u_port.h"

#include "disp_drv_ddp.h"

#include "ddp_debug.h"
#include "ddp_reg.h"
#include "ddp_drv.h"
#include "ddp_wdma.h"
#include "ddp_wdma_ex.h"
#include "ddp_hal.h"
#include "ddp_path.h"
#include "ddp_aal.h"
#include "ddp_pwm.h"
#include <ddp_od.h>
#include "ddp_dither.h"
#include "ddp_info.h"
#include "ddp_dsi.h"
#include "ddp_rdma.h"
#include "ddp_rdma_ex.h"
#include "ddp_manager.h"
#include "ddp_log.h"
#include "ddp_met.h"
#include "display_recorder.h"
#include "disp_session.h"
#include "disp_lowpower.h"

static struct dentry *debugfs;
static struct dentry *debugDir;


static struct dentry *debugfs_dump;

static const long int DEFAULT_LOG_FPS_WND_SIZE = 30;
static int debug_init;


unsigned char pq_debug_flag = 0;
unsigned char aal_debug_flag = 0;

static unsigned int dbg_log_level;
static unsigned int irq_log_level;
static unsigned int dump_to_buffer;

static int dbg_force_roi;
static int dbg_partial_x;
static int dbg_partial_y;
static int dbg_partial_w;
static int dbg_partial_h;
static int dbg_partial_statis;

unsigned int gUltraEnable = 1;
static char STR_HELP[] =
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
	"       dump_reg:moduleID\n" "       dump_path:mutexID\n" "       dpfd_ut1:channel\n";


/* --------------------------------------------------------------------------- */
/* Command Processor */
/* --------------------------------------------------------------------------- */
static int low_power_cust_mode = LP_CUST_DISABLE;
static unsigned int vfp_backup;

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
		DDPMSG("addr valid, isVa=0x%x, addr=0x%lx, module=%s!\n", isVa, addr,
		       ddp_get_reg_module_name(i));
		return 1;
	}

	DDPERR("is_reg_addr_valid return fail, isVa=0x%x, addr=0x%lx!\n", isVa, addr);
	return 0;

}


static void process_dbg_opt(const char *opt)
{
	char *buf = dbg_buf + strlen(dbg_buf);
	/* static disp_session_config config; */
	int ret;

	if (0 == strncmp(opt, "regr:", 5)) {
		char *p = (char *)opt + 5;
		unsigned long addr;

		ret = kstrtoul(p, 16, &addr);
		if (ret) {
			snprintf(buf, 50, "error to parse cmd %s\n", opt);
			return;
		}

		if (is_reg_addr_valid(1, addr) == 1) {
			unsigned int regVal = DISP_REG_GET(addr);

			DDPMSG("regr: 0x%lx = 0x%08X\n", addr, regVal);
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
			DDPMSG("regw: 0x%lx, 0x%08X = 0x%08X\n", addr, val, regVal);
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

		if (enable)
			dbg_log_level = 1;
		else
			dbg_log_level = 0;

		sprintf(buf, "dbg_log: %d\n", dbg_log_level);
	} else if (0 == strncmp(opt, "irq_log:", 8)) {
		char *p = (char *)opt + 8;
		unsigned int enable;

		ret = kstrtouint(p, 0, &enable);
		if (ret) {
			snprintf(buf, 50, "error to parse cmd %s\n", opt);
			return;
		}
		if (enable)
			irq_log_level = 1;
		else
			irq_log_level = 0;

		sprintf(buf, "irq_log: %d\n", irq_log_level);
	} else if (0 == strncmp(opt, "met_on:", 7)) {
		int met_on, rdma0_mode, rdma1_mode;

		ret = sscanf(opt, "met_on:%d,%d,%d\n", &met_on, &rdma0_mode, &rdma1_mode);
		if (ret != 3) {
			snprintf(buf, 50, "error to parse cmd %s\n", opt);
			return;
		}
		ddp_init_met_tag(met_on, rdma0_mode, rdma1_mode);
		DDPMSG("process_dbg_opt, met_on=%d,rdma0_mode %d, rdma1 %d\n", met_on, rdma0_mode,
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
			disp_bls_set_backlight(level);
			sprintf(buf, "backlight: %d\n", level);
		} else {
			goto Error;
		}
	} else if (0 == strncmp(opt, "partial:", 8)) {
		ret = sscanf(opt, "partial:%d,%d,%d,%d,%d\n", &dbg_force_roi,
			&dbg_partial_x, &dbg_partial_y, &dbg_partial_w, &dbg_partial_h);
		if (ret != 5) {
			snprintf(buf, 50, "error to parse cmd %s\n", opt);
			return;
		}
		DDPMSG("process_dbg_opt, partial force=%d (%d,%d,%d,%d)\n", dbg_force_roi,
			dbg_partial_x, dbg_partial_y, dbg_partial_w, dbg_partial_h);
	} else if (0 == strncmp(opt, "partial_s:", 10)) {
		ret = sscanf(opt, "partial_s:%d\n", &dbg_partial_statis);
		if (ret != 5) {
			snprintf(buf, 50, "error to parse cmd %s\n", opt);
			return;
		}
		DDPMSG("process_dbg_opt, partial_s:%d\n", dbg_partial_statis);
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

			disp_pwm_set_backlight(pwm_id, level);
			sprintf(buf, "PWM 0x%x : %d\n", pwm_id, level);
		} else {
			goto Error;
		}
	} else if (0 == strncmp(opt, "rdma_color:", 11)) {
		if (0 == strncmp(opt + 11, "on", 2)) {
			/* char *p = (char *)opt + 14; */
			unsigned int red, green, blue;

			rdma_color_matrix matrix = { 0 };
			rdma_color_pre pre = { 0 };
			rdma_color_post post = { 255, 0, 0 };

			ret = sscanf(opt, "rdma_color:on,%d,%d,%d\n", &red, &green, &blue);
			if (ret != 3) {
				snprintf(buf, 50, "error to parse cmd %s\n", opt);
				pr_err("error to parse cmd %s\n", opt);
				return;
			}

			post.ADD0 = red;
			post.ADD1 = green;
			post.ADD2 = blue;
			rdma_set_color_matrix(DISP_MODULE_RDMA0, &matrix, &pre, &post);
			rdma_enable_color_transform(DISP_MODULE_RDMA0);
		} else if (0 == strncmp(opt + 11, "off", 3)) {
			rdma_disable_color_transform(DISP_MODULE_RDMA0);
		}
	} else if (0 == strncmp(opt, "aal_dbg:", 8)) {
		char *p = (char *)opt + 8;

		ret = kstrtouint(p, 0, &aal_dbg_en);
		if (ret) {
			snprintf(buf, 50, "error to parse cmd %s\n", opt);
			return;
		}

		sprintf(buf, "aal_dbg_en = 0x%x\n", aal_dbg_en);
#if 0 /* FIXME: tmp comment */
	} else if (0 == strncmp(opt, "corr_dbg:", 9)) {
		char *p = (char *)opt + 9;

		ret = kstrtouint(p, 0, &corr_dbg_en);
		if (ret) {
			snprintf(buf, 50, "error to parse cmd %s\n", opt);
			return;
		}

		sprintf(buf, "corr_dbg_en = 0x%x\n", corr_dbg_en);
#endif
	} else if (0 == strncmp(opt, "aal_test:", 9)) {
		aal_test(opt + 9, buf);
	} else if (0 == strncmp(opt, "pwm_test:", 9)) {
		disp_pwm_test(opt + 9, buf);
	} else if (0 == strncmp(opt, "dither_test:", 12)) {
		/*dither_test(opt + 12, buf);*/
	} else if (0 == strncmp(opt, "ccorr_test:", 11)) {
		/*ccorr_test(opt + 11, buf);*/
	} else if (0 == strncmp(opt, "od_test:", 8)) {
		od_test(opt + 8, buf);
	} else if (0 == strncmp(opt, "dump_reg:", 9)) {
		char *p = (char *)opt + 9;
		unsigned int module;

		ret = kstrtouint(p, 0, &module);
		if (ret) {
			snprintf(buf, 50, "error to parse cmd %s\n", opt);
			return;
		}

		DDPMSG("process_dbg_opt, module=%d\n", module);
		if (module < DISP_MODULE_NUM) {
			ddp_dump_reg(module);
			sprintf(buf, "dump_reg: %d\n", module);
		} else {
			DDPMSG("process_dbg_opt2, module=%d\n", module);
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

		DDPMSG("process_dbg_opt, path mutex=%d\n", mutex_idx);
		dpmgr_debug_path_status(mutex_idx);
		sprintf(buf, "dump_path: %d\n", mutex_idx);

	} else if (0 == strncmp(opt, "get_module_addr", 15)) {
		unsigned int i = 0;
		char *buf_temp = buf;

		for (i = 0; i < DISP_REG_NUM; i++) {
			DDPDUMP("i=%d, module=%s, va=0x%lx, pa=0x%lx, irq(%d)\n",
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
			DDPMSG("[DDP] debug=1, trigger AEE\n");
			/* aee_kernel_exception("DDP-TEST-ASSERT", "[DDP] DDP-TEST-ASSERT"); */
		} else if (enable == 2) {
			ddp_mem_test();
		} else if (enable == 3) {
			ddp_lcd_test();
		} else if (enable == 4) {
			/* DDPAEE("test 4"); */
		} else if (enable == 12) {
			if (gUltraEnable == 0)
				gUltraEnable = 1;
			else
				gUltraEnable = 0;
			sprintf(buf, "gUltraEnable: %d\n", gUltraEnable);
		}
	} else if (0 == strncmp(opt, "mmp", 3)) {
		init_ddp_mmp_events();
	} else if (0 == strncmp(opt, "low_power_mode:", 15)) {
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
	DDPERR("parse command error!\n%s\n\n%s", opt, STR_HELP);
}


static void process_dbg_cmd(char *cmd)
{
	char *tok;

	DDPDBG("cmd: %s\n", cmd);
	memset(dbg_buf, 0, sizeof(dbg_buf));
	while ((tok = strsep(&cmd, " ")) != NULL)
		process_dbg_opt(tok);
}


/* --------------------------------------------------------------------------- */
/* Debug FileSystem Routines */
/* --------------------------------------------------------------------------- */

static int debug_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}


static char cmd_buf[512];

static ssize_t debug_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
	if (strlen(dbg_buf))
		return simple_read_from_buffer(ubuf, count, ppos, dbg_buf, strlen(dbg_buf));
	else
		return simple_read_from_buffer(ubuf, count, ppos, STR_HELP, strlen(STR_HELP));

}


static ssize_t debug_write(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos)
{
	const int debug_bufmax = sizeof(cmd_buf) - 1;
	size_t ret;

	ret = count;

	if (count > debug_bufmax)
		count = debug_bufmax;

	if (copy_from_user(&cmd_buf, ubuf, count))
		return -EFAULT;

	cmd_buf[count] = 0;

	process_dbg_cmd(cmd_buf);

	return ret;
}


static const struct file_operations debug_fops = {
	.read = debug_read,
	.write = debug_write,
	.open = debug_open,
};

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

static const struct file_operations low_power_cust_fops = {
	.read = lp_cust_read,
};

static ssize_t debug_dump_read(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
	char *str = "idlemgr disable mtcmos now, all the regs may 0x00000000\n";

	dprec_logger_dump_reset();
	dump_to_buffer = 1;
	/* dump all */
	dpmgr_debug_path_status(-1);
	dump_to_buffer = 0;
	if (is_mipi_enterulps())
		return simple_read_from_buffer(buf, size, ppos, str, strlen(str));
	else
		return simple_read_from_buffer(buf, size, ppos, dprec_logger_get_dump_addr(),
				       dprec_logger_get_dump_len());
}


static const struct file_operations debug_fops_dump = {
	.read = debug_dump_read,
};

void ddp_debug_init(void)
{
	struct dentry *d;

	if (!debug_init) {
		debug_init = 1;
		debugfs = debugfs_create_file("dispsys",
					      S_IFREG | S_IRUGO, NULL, (void *)0, &debug_fops);


		debugDir = debugfs_create_dir("disp", NULL);
		if (debugDir) {

			debugfs_dump = debugfs_create_file("dump",
							   S_IFREG | S_IRUGO, debugDir, NULL,
							   &debug_fops_dump);
			d = debugfs_create_file("lowpowermode", S_IFREG | S_IRUGO, debugDir, NULL,
						&low_power_cust_fops);
			/*
			if (!d)
				return -ENOMEM;
			*/
		}
	}
}

unsigned int ddp_debug_analysis_to_buffer(void)
{
	return dump_to_buffer;
}

unsigned int ddp_debug_dbg_log_level(void)
{
	return dbg_log_level;
}

unsigned int ddp_debug_irq_log_level(void)
{
	return irq_log_level;
}

int ddp_debug_force_roi(void)
{
	return dbg_force_roi;
}

int ddp_debug_partial_statistic(void)
{
	return dbg_partial_statis;
}

int ddp_debug_force_roi_x(void)
{
	return dbg_partial_x;
}

int ddp_debug_force_roi_y(void)
{
	return dbg_partial_y;
}

int ddp_debug_force_roi_w(void)
{
	return dbg_partial_w;
}

int ddp_debug_force_roi_h(void)
{
	return dbg_partial_h;
}

void ddp_debug_exit(void)
{
	debugfs_remove(debugfs);
	debugfs_remove(debugfs_dump);
	debug_init = 0;
}

int ddp_mem_test(void)
{
	return -1;
}

int ddp_lcd_test(void)
{
	return -1;
}
