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
#include <linux/types.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <mt-plat/aee.h>
#include <mach/mt_clkmgr.h>
#ifdef CONFIG_MTK_LEGACY
#include <mt-plat/mt_gpio.h>
#else
#include "disp_dts_gpio.h"
#endif

#include "m4u.h"
#include "cmdq_def.h"
#include "cmdq_record.h"
#include "cmdq_reg.h"
#include "cmdq_core.h"


#include "lcm_drv.h"
#include "ddp_ovl.h"
#include "ddp_dsi.h"
#include "ddp_wdma.h"
#include "ddp_wdma_ex.h"
#include "ddp_aal.h"
#include "ddp_pwm.h"
#include "ddp_dither.h"
#include "ddp_path.h"
#include "ddp_manager.h"
#include "ddp_reg.h"
#include "ddp_drv.h"
#include "ddp_hal.h"
#include "ddp_info.h"
#include "ddp_mmp.h"
#include "ddp_met.h"
#include "ddp_dump.h"
#include "disp_log.h"
#include "disp_debug.h"
#include "disp_recorder.h"
#include "disp_drv_ddp.h"
#include "disp_drv_platform.h"
#include "disp_assert_layer.h"
#include "disp_helper.h"
#include "disp_session.h"
#include "mtk_disp_mgr.h"
#include "mtkfb.h"
#include "mtkfb_fence.h"
#include "primary_display.h"






#pragma GCC optimize("O0")

/* --------------------------------------------------------------------------- */
/* External variable declarations */
/* --------------------------------------------------------------------------- */
/* --------------------------------------------------------------------------- */
/* Debug Options */
/* --------------------------------------------------------------------------- */

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

/* for video mode */
unsigned int gEnableUartLog = 0;
unsigned int gMobilelog = 1;
unsigned int gLoglevel = 3; /*DISPMSG level is DEFAULT_LEVEL==3*/
unsigned int gRcdlevel = 0;

/* for video mode */
unsigned int gEnableMutexRisingEdge = 0;
unsigned int gPrefetchControl = 1;
unsigned int gOVLBackground = 0xFF000000;
unsigned int gUltraEnable = 1;
unsigned int gEnableDSIStateCheck = 0;
unsigned int gMutexFreeRun = 1;
unsigned int disp_low_power_lfr = 0;

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
/* Command Processor */
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

	/* static disp_session_config config; */

	if (0 == strncmp(opt, "regr:", 5)) {
		char *p = (char *)opt + 5;
		unsigned long addr = 0;

		ret = kstrtoul(p, 16, (unsigned long *)&addr);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);
		if (is_reg_addr_valid(1, addr) == 1) {
			unsigned int regVal = DISP_REG_GET(addr);

			DISPMSG("regr: 0x%lx = 0x%08X\n", addr, regVal);
			sprintf(buf, "regr: 0x%lx = 0x%08X\n", addr, regVal);
		} else {
			sprintf(buf, "regr, invalid address 0x%lx\n", addr);
			goto Error;
		}
	} else if (0 == strncmp(opt, "lfr_update", 10)) {
		DSI_LFR_UPDATE(DISP_MODULE_DSI0, NULL);

	} else if (0 == strncmp(opt, "regw:", 5)) {
		char *p = (char *)opt + 5;
		unsigned long addr = 0;
		unsigned int val = 0;

		ret = kstrtoul(p, 16, (unsigned long *)&addr);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);
		ret = kstrtoul(p + 1, 16, (unsigned long *)&val);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);
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
		unsigned int enable = 0;

		ret = kstrtoul(p, 10, (unsigned long *)&enable);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);

		if (enable)
			dbg_log_level = 1;
		else
			dbg_log_level = 0;

		sprintf(buf, "dbg_log: %d\n", dbg_log_level);
	} else if (0 == strncmp(opt, "irq_log:", 8)) {
		char *p = (char *)opt + 8;
		unsigned int enable = 0;

		ret = kstrtoul(p, 10, (unsigned long *)&enable);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);

		if (enable)
			irq_log_level = 1;
		else
			irq_log_level = 0;

		sprintf(buf, "irq_log: %d\n", irq_log_level);
	} else if (0 == strncmp(opt, "met_on:", 7)) {
		char *p = (char *)opt + 7;
		int met_on = 0;
		int rdma0_mode = 0;
		int rdma1_mode = 0;

		ret = kstrtoul(p, 10, (unsigned long *)&met_on);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);
		ret = kstrtoul(p + 1, 10, (unsigned long *)&rdma0_mode);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);
		ret = kstrtoul(p + 1, 10, (unsigned long *)&rdma1_mode);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);

		ddp_init_met_tag(met_on, rdma0_mode, rdma1_mode);
		DISPMSG("process_dbg_opt, met_on=%d,rdma0_mode %d, rdma1 %d\n", met_on, rdma0_mode, rdma1_mode);
		sprintf(buf, "met_on:%d,rdma0_mode:%d,rdma1_mode:%d\n", met_on, rdma0_mode, rdma1_mode);
	} else if (0 == strncmp(opt, "backlight:", 10)) {
		char *p = (char *)opt + 10;
		unsigned int level = 0;

		ret = kstrtoul(p, 10, (unsigned long *)&level);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);

		if (level) {
			disp_bls_set_backlight(level);
			sprintf(buf, "backlight: %d\n", level);
		} else {
			goto Error;
		}
	} else if (0 == strncmp(opt, "pwm0:", 5) || 0 == strncmp(opt, "pwm1:", 5)) {
		char *p = (char *)opt + 5;
		unsigned int level = 0;

		ret = kstrtoul(p, 10, (unsigned long *)&level);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);

		if (level) {
			disp_pwm_id_t pwm_id = DISP_PWM0;

			if (opt[3] == '1')
				pwm_id = DISP_PWM1;

			disp_pwm_set_backlight(pwm_id, level);
			sprintf(buf, "PWM 0x%x : %d\n", pwm_id, level);
		} else {
			goto Error;
		}
	} else if (0 == strncmp(opt, "aal_dbg:", 8)) {
		aal_dbg_en = 0;
		ret = kstrtoul(opt + 8, 10, (unsigned long *)&aal_dbg_en);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);
		sprintf(buf, "aal_dbg_en = 0x%x\n", aal_dbg_en);
	} else if (0 == strncmp(opt, "aal_test:", 9)) {
		aal_test(opt + 9, buf);
	} else if (0 == strncmp(opt, "pwm_test:", 9)) {
		disp_pwm_test(opt + 9, buf);
	} else if (0 == strncmp(opt, "dither_test:", 12)) {
		dither_test(opt + 12, buf);
	} else if (0 == strncmp(opt, "dump_reg:", 9)) {
		char *p = (char *)opt + 9;
		unsigned int module = 0;

		ret = kstrtoul(p, 10, (unsigned long *)&module);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);
		DISPMSG("ddp_process_dbg_opt, module=%d\n", module);
		if (module < DISP_MODULE_NUM) {
			ddp_dump_reg(module);
			sprintf(buf, "dump_reg: %d\n", module);
		} else {
			DISPMSG("process_dbg_opt2, module=%d\n", module);
			goto Error;
		}
	} else if (0 == strncmp(opt, "dump_path:", 10)) {
		char *p = (char *)opt + 10;
		unsigned int mutex_idx = 0;

		ret = kstrtoul(p, 10, (unsigned long *)&mutex_idx);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);

		DISPMSG("ddp_process_dbg_opt, path mutex=%d\n", mutex_idx);
		dpmgr_debug_path_status(mutex_idx);
		sprintf(buf, "dump_path: %d\n", mutex_idx);
	} else if (0 == strncmp(opt, "debug:", 6)) {
		char *p = (char *)opt + 6;
		unsigned int enable = 0;

		ret = kstrtoul(p, 10, (unsigned long *)&enable);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);

		if (enable == 1) {
			DISPMSG("[DISP] debug=1, trigger AEE\n");
			/* aee_kernel_exception("DDP-TEST-ASSERT", "[DDP] DDP-TEST-ASSERT"); */
		} else if (enable == 2) {
			ddp_mem_test();
		} else if (enable == 3) {
			ddp_lcd_test();
		} else if (enable == 4) {
			/* DDPAEE("test 4"); */
		} else if (enable == 5) {
			;
		} else if (enable == 6) {
			unsigned int i = 0;
			int *modules = ddp_get_scenario_list(DDP_SCENARIO_PRIMARY_DISP);
			int module_num = ddp_get_module_num(DDP_SCENARIO_PRIMARY_DISP);

			DISPMSG("dump path status:");
			for (i = 0; i < module_num; i++)
				DISPMSG("%s-", ddp_get_module_name(modules[i]));

			DISPMSG("\n");

			ddp_dump_analysis(DISP_MODULE_CONFIG);
			ddp_dump_analysis(DISP_MODULE_MUTEX);
			for (i = 0; i < module_num; i++)
				ddp_dump_analysis(modules[i]);

			ddp_dump_reg(DISP_MODULE_CONFIG);
			ddp_dump_reg(DISP_MODULE_MUTEX);
			for (i = 0; i < module_num; i++)
				ddp_dump_reg(modules[i]);

		} else if (enable == 7) {
			if (dbg_log_level < 3)
				dbg_log_level++;
			else
				dbg_log_level = 0;

			DISPMSG("DISP: dbg_log_level=%d\n", dbg_log_level);
			sprintf(buf, "dbg_log_level: %d\n", dbg_log_level);
		} else if (enable == 9) {
			gOVLBackground = 0xFF0000FF;
			DISPMSG("DISP: gOVLBackground=%d\n", gOVLBackground);
			sprintf(buf, "gOVLBackground: %d\n", gOVLBackground);
		} else if (enable == 10) {
			gOVLBackground = 0xFF000000;
			DISPMSG("DISP: gOVLBackground=%d\n", gOVLBackground);
			sprintf(buf, "gOVLBackground: %d\n", gOVLBackground);
		} else if (enable == 11) {
			unsigned int i = 0;
			char *buf_temp = buf;

			for (i = 0; i < DISP_REG_NUM; i++) {
				DISPDMP("i=%d, module=%s, va=0x%lx, pa=0x%lx, irq(%d,%d)\n",
					i, ddp_get_reg_module_name(i), dispsys_reg[i],
					ddp_reg_pa_base[i], dispsys_irq[i], ddp_irq_num[i]);
				sprintf(buf_temp,
					"i=%d, module=%s, va=0x%lx, pa=0x%lx, irq(%d,%d)\n", i,
					ddp_get_reg_module_name(i), dispsys_reg[i],
					ddp_reg_pa_base[i], dispsys_irq[i], ddp_irq_num[i]);
				buf_temp += strlen(buf_temp);
			}
		} else if (enable == 12) {
			if (gUltraEnable == 0)
				gUltraEnable = 1;
			else
				gUltraEnable = 0;

			DISPMSG("DISP: gUltraEnable=%d\n", gUltraEnable);
			sprintf(buf, "gUltraEnable: %d\n", gUltraEnable);
		} else if (enable == 13) {
			if (disp_low_power_lfr == 0) {
				disp_low_power_lfr = 1;
				DISPMSG("DISP: disp_low_power_lfr=%d\n", disp_low_power_lfr);
			} else {
				disp_low_power_lfr = 0;
				DISPMSG("DISP: disp_low_power_lfr=%d\n", disp_low_power_lfr);
			}
		} else if (enable == 14) {
			if (gEnableDSIStateCheck == 0)
				gEnableDSIStateCheck = 1;
			else
				gEnableDSIStateCheck = 0;

			DISPMSG("DISP: gEnableDSIStateCheck=%d\n", gEnableDSIStateCheck);
			sprintf(buf, "gEnableDSIStateCheck: %d\n", gEnableDSIStateCheck);
		} else if (enable == 15) {
			if (gMutexFreeRun == 0)
				gMutexFreeRun = 1;
			else
				gMutexFreeRun = 0;

			DISPMSG("DISP: gMutexFreeRun=%d\n", gMutexFreeRun);
			sprintf(buf, "gMutexFreeRun: %d\n", gMutexFreeRun);
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


/* --------------------------------------------------------------------------- */
/* Debug FileSystem Routines */
/* --------------------------------------------------------------------------- */

static ssize_t ddp_debug_open(struct inode *inode, struct file *file)
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


static ssize_t dump_debug_read(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{

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

void ddp_debug_init(void)
{
	if (!debug_init) {
		debug_init = 1;
		debugfs = debugfs_create_file("dispsys",
					      S_IFREG | S_IRUGO, NULL, (void *)0, &ddp_debug_fops);


		debugDir = debugfs_create_dir("disp", NULL);
		if (debugDir) {

			debugfs_dump = debugfs_create_file("dump",
							   S_IFREG | S_IRUGO, debugDir, NULL,
							   &dump_debug_fops);
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

unsigned int ddp_dump_reg_to_buf(unsigned int start_module, unsigned long *addr)
{
	unsigned int cnt = 0;
	unsigned long reg_addr;

	switch (start_module) {
	case 0:
		/* DISP_MODULE_WDMA0: */
		reg_addr = DISP_REG_WDMA_INTEN;

		while (reg_addr <= DISP_REG_WDMA_PRE_ADD2) {
			addr[cnt++] = DISP_REG_GET(reg_addr);
			reg_addr += 4;
		}
		/* fallthrough */
	case 1:
		/* DISP_MODULE_OVL: */
		reg_addr = DISP_REG_OVL_STA;

		while (reg_addr <= DISP_REG_OVL_L3_PITCH) {
			addr[cnt++] = DISP_REG_GET(reg_addr);
			reg_addr += 4;
		}
		/* fallthrough */
	case 2:
		/* DISP_MODULE_RDMA: */
		reg_addr = DISP_REG_RDMA_INT_ENABLE;

		while (reg_addr <= DISP_REG_RDMA_PRE_ADD_1) {
			addr[cnt++] = DISP_REG_GET(reg_addr);
			reg_addr += 4;
		}
		break;
	}
	return cnt * sizeof(unsigned long);
}









/******************************************************************************************
above is ddp_debug.c
below is debug.c
******************************************************************************************/











struct MTKFB_MMP_Events_t MTKFB_MMP_Events;

/* extern LCM_DRIVER *lcm_drv; */
/* extern unsigned int EnableVSyncLog; */

#define MTKFB_DEBUG_FS_CAPTURE_LAYER_CONTENT_SUPPORT

/* --------------------------------------------------------------------------- */
/* External variable declarations */
/* --------------------------------------------------------------------------- */

/* extern long tpd_last_down_time; */
/* extern int tpd_start_profiling; */
/* extern void mtkfb_log_enable(int enable); */
/* extern void disp_log_enable(int enable); */
/* extern void mtkfb_vsync_log_enable(int enable); */
/* extern void mtkfb_capture_fb_only(bool enable); */
/* extern void esd_recovery_pause(BOOL en); */
/* extern int mtkfb_set_backlight_mode(unsigned int mode); */
/* extern void mtkfb_pan_disp_test(void); */
/* extern void mtkfb_show_sem_cnt(void); */
/* extern void mtkfb_hang_test(bool en); */
/* extern void mtkfb_switch_normal_to_factory(void); */
/* extern void mtkfb_switch_factory_to_normal(void); */
/* extern int mtkfb_get_debug_state(char *stringbuf, int buf_len); */
/* extern unsigned int mtkfb_fm_auto_test(void); */
/*
extern void DSI_Set_LFR(DISP_MODULE_ENUM module, cmdqRecHandle cmdq,
			unsigned int mode, unsigned int type,
			unsigned int enable, unsigned int skip_num);
extern int primary_display_vsync_switch(int method);

extern unsigned int gCaptureLayerEnable;
extern unsigned int gCaptureLayerDownX;
extern unsigned int gCaptureLayerDownY;

extern unsigned int gCaptureOvlThreadEnable;
extern unsigned int gCaptureOvlDownX;
extern unsigned int gCaptureOvlDownY;
extern struct task_struct *captureovl_task;
*/
/* extern unsigned int gCaptureFBEnable; */
/* extern unsigned int gCaptureFBDownX; */
/* extern unsigned int gCaptureFBDownY; */
/* extern unsigned int gCaptureFBPeriod; */
/* extern struct task_struct *capturefb_task; */
/* extern wait_queue_head_t gCaptureFBWQ; */

/* extern unsigned int gCapturePriLayerEnable; */
/* extern unsigned int gCaptureWdmaLayerEnable; */
/* extern unsigned int gCapturePriLayerDownX; */
/* extern unsigned int gCapturePriLayerDownY; */
/* extern unsigned int gCapturePriLayerNum; */
#ifdef MTKFB_DEBUG_FS_CAPTURE_LAYER_CONTENT_SUPPORT
struct dentry *mtkfb_layer_dbgfs[DDP_OVL_LAYER_MUN];

/* extern OVL_CONFIG_STRUCT cached_layer_config[DDP_OVL_LAYER_MUN]; */

typedef struct {
	uint32_t layer_index;
	unsigned long working_buf;
	uint32_t working_size;
} MTKFB_LAYER_DBG_OPTIONS;

MTKFB_LAYER_DBG_OPTIONS mtkfb_layer_dbg_opt[DDP_OVL_LAYER_MUN];

#endif

/* --------------------------------------------------------------------------- */
/* Debug Options */
/* --------------------------------------------------------------------------- */
typedef struct {
	unsigned int en_fps_log;
	unsigned int en_touch_latency_log;
	unsigned int log_fps_wnd_size;
	unsigned int force_dis_layers;
} DBG_OPTIONS;

static DBG_OPTIONS dbg_opt = { 0 };

static bool enable_ovl1_to_mem = true;

static char MTKFB_STR_HELP[] =
	"\n"
	"USAGE\n"
	"        echo [ACTION]... > /d/mtkfb\n"
	"\n"
	"ACTION\n"
	"        mtkfblog:[on|off]\n"
	"             enable/disable [MTKFB] log\n"
	"\n"
	"        displog:[on|off]\n"
	"             enable/disable [DISP] log\n"
	"\n"
	"        mtkfb_vsynclog:[on|off]\n"
	"             enable/disable [VSYNC] log\n"
	"\n"
	"        log:[on|off]\n"
	"             enable/disable above all log\n"
	"\n"
	"        fps:[on|off]\n"
	"             enable fps and lcd update time log\n"
	"\n"
	"        tl:[on|off]\n"
	"             enable touch latency log\n"
	"\n"
	"        layer\n"
	"             dump lcd layer information\n"
	"\n"
	"        suspend\n"
	"             enter suspend mode\n"
	"\n"
	"        resume\n"
	"             leave suspend mode\n"
	"\n"
	"        lcm:[on|off|init]\n"
	"             power on/off lcm\n"
	"\n"
	"        cabc:[ui|mov|still]\n"
	"             cabc mode, UI/Moving picture/Still picture\n"
	"\n"
	"        lcd:[on|off]\n"
	"             power on/off display engine\n"
	"\n"
	"        te:[on|off]\n"
	"             turn on/off tearing-free control\n"
	"\n"
	"        tv:[on|off]\n"
	"             turn on/off tv-out\n"
	"\n"
	"        tvsys:[ntsc|pal]\n"
	"             switch tv system\n"
	"\n"
	"        reg:[lcd|dpi|dsi|tvc|tve]\n"
	"             dump hw register values\n"
	"\n"
	"        regw:addr=val\n"
	"             write hw register\n"
	"\n"
	"        regr:addr\n"
	"             read hw register\n"
	"\n"
	"       cpfbonly:[on|off]\n"
	"             capture UI layer only on/off\n"
	"\n"
	"       esd:[on|off]\n"
	"             esd kthread on/off\n"
	"       HQA:[NormalToFactory|FactoryToNormal]\n"
	"             for HQA requirement\n"
	"\n"
	"       mmp\n"
	"             Register MMProfile events\n"
	"\n"
	"       dump_fb:[on|off[,down_sample_x[,down_sample_y,[delay]]]]\n"
	"             Start/end to capture framebuffer every delay(ms)\n"
	"\n"
	"       dump_ovl:[on|off[,down_sample_x[,down_sample_y]]]\n"
	"             Start to capture OVL only once\n"
	"\n"
	"       dump_layer:[on|off[,down_sample_x[,down_sample_y]][,layer(0:L0,1:L1,2:L2,3:L3,4:L0-3)]\n"
	"             Start/end to capture current enabled OVL layer every frame\n";

/* --------------------------------------------------------------------------- */
/* Information Dump Routines */
/* --------------------------------------------------------------------------- */

void init_mtkfb_mmp_events(void)
{
	if (MTKFB_MMP_Events.MTKFB == 0) {
		MTKFB_MMP_Events.MTKFB =
		    MMProfileRegisterEvent(MMP_RootEvent, "MTKFB");
		MTKFB_MMP_Events.PanDisplay =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB,
					   "PanDisplay");
		MTKFB_MMP_Events.CreateSyncTimeline =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB,
					   "CreateSyncTimeline");
		MTKFB_MMP_Events.SetOverlayLayer =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB,
					   "SetOverlayLayer");
		MTKFB_MMP_Events.SetOverlayLayers =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB,
					   "SetOverlayLayers");
		MTKFB_MMP_Events.SetMultipleLayers =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB,
					   "SetMultipleLayers");
		MTKFB_MMP_Events.CreateSyncFence =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB,
					   "CreateSyncFence");
		MTKFB_MMP_Events.IncSyncTimeline =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB,
					   "IncSyncTimeline");
		MTKFB_MMP_Events.SignalSyncFence =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB,
					   "SignalSyncFence");
		MTKFB_MMP_Events.TrigOverlayOut =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB,
					   "TrigOverlayOut");
		MTKFB_MMP_Events.UpdateScreenImpl =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB,
					   "UpdateScreenImpl");
		MTKFB_MMP_Events.VSync =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "VSync");
		MTKFB_MMP_Events.UpdateConfig =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB,
					   "UpdateConfig");
		MTKFB_MMP_Events.EsdCheck =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.UpdateConfig,
					   "EsdCheck");
		MTKFB_MMP_Events.ConfigOVL =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.UpdateConfig,
					   "ConfigOVL");
		MTKFB_MMP_Events.ConfigAAL =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.UpdateConfig,
					   "ConfigAAL");
		MTKFB_MMP_Events.ConfigMemOut =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.UpdateConfig,
					   "ConfigMemOut");
		MTKFB_MMP_Events.ScreenUpdate =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB,
					   "ScreenUpdate");
		MTKFB_MMP_Events.CaptureFramebuffer =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "CaptureFB");
		MTKFB_MMP_Events.RegUpdate =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "RegUpdate");
		MTKFB_MMP_Events.EarlySuspend =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB,
					   "EarlySuspend");
		MTKFB_MMP_Events.DispDone =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "DispDone");
		MTKFB_MMP_Events.DSICmd =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "DSICmd");
		MTKFB_MMP_Events.DSIIRQ =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "DSIIrq");
		MTKFB_MMP_Events.WaitVSync =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "WaitVSync");
		MTKFB_MMP_Events.LayerDump =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "LayerDump");
		MTKFB_MMP_Events.Layer[0] =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.LayerDump,
					   "Layer0");
		MTKFB_MMP_Events.Layer[1] =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.LayerDump,
					   "Layer1");
		MTKFB_MMP_Events.Layer[2] =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.LayerDump,
					   "Layer2");
		MTKFB_MMP_Events.Layer[3] =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.LayerDump,
					   "Layer3");
		MTKFB_MMP_Events.OvlDump =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "OvlDump");
		MTKFB_MMP_Events.FBDump =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "FBDump");
		MTKFB_MMP_Events.DSIRead =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "DSIRead");
		MTKFB_MMP_Events.GetLayerInfo =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB,
					   "GetLayerInfo");
		MTKFB_MMP_Events.LayerInfo[0] =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.GetLayerInfo,
					   "LayerInfo0");
		MTKFB_MMP_Events.LayerInfo[1] =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.GetLayerInfo,
					   "LayerInfo1");
		MTKFB_MMP_Events.LayerInfo[2] =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.GetLayerInfo,
					   "LayerInfo2");
		MTKFB_MMP_Events.LayerInfo[3] =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.GetLayerInfo,
					   "LayerInfo3");
		MTKFB_MMP_Events.IOCtrl =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "IOCtrl");
		MTKFB_MMP_Events.Debug =
		    MMProfileRegisterEvent(MTKFB_MMP_Events.MTKFB, "Debug");
		MMProfileEnableEventRecursive(MTKFB_MMP_Events.MTKFB, 1);
	}
}

static inline int is_layer_enable(unsigned int roi_ctl, unsigned int layer)
{
	return (roi_ctl >> (31 - layer)) & 0x1;
}

/* --------------------------------------------------------------------------- */
/* FPS Log */
/* --------------------------------------------------------------------------- */

typedef struct {
	long int current_lcd_time_us;
	long int current_te_delay_time_us;
	long int total_lcd_time_us;
	long int total_te_delay_time_us;
	long int start_time_us;
	long int trigger_lcd_time_us;
	unsigned int trigger_lcd_count;

	long int current_hdmi_time_us;
	long int total_hdmi_time_us;
	long int hdmi_start_time_us;
	long int trigger_hdmi_time_us;
	unsigned int trigger_hdmi_count;
} FPS_LOGGER;

static FPS_LOGGER fps = { 0 };
static FPS_LOGGER hdmi_fps = { 0 };

static long int get_current_time_us(void)
{
	struct timeval t;

	do_gettimeofday(&t);
	return (t.tv_sec & 0xFFF) * 1000000 + t.tv_usec;
}

static inline void reset_fps_logger(void)
{
	memset(&fps, 0, sizeof(fps));
}

static inline void reset_hdmi_fps_logger(void)
{
	memset(&hdmi_fps, 0, sizeof(hdmi_fps));
}

void DBG_OnTriggerLcd(void)
{
	if (!dbg_opt.en_fps_log && !dbg_opt.en_touch_latency_log)
		return;

	fps.trigger_lcd_time_us = get_current_time_us();
	if (fps.trigger_lcd_count == 0)
		fps.start_time_us = fps.trigger_lcd_time_us;
}

void DBG_OnTriggerHDMI(void)
{
	if (!dbg_opt.en_fps_log && !dbg_opt.en_touch_latency_log)
		return;

	hdmi_fps.trigger_hdmi_time_us = get_current_time_us();
	if (hdmi_fps.trigger_hdmi_count == 0)
		hdmi_fps.hdmi_start_time_us = hdmi_fps.trigger_hdmi_time_us;
}

void DBG_OnTeDelayDone(void)
{
	long int time;

	if (!dbg_opt.en_fps_log && !dbg_opt.en_touch_latency_log)
		return;

	time = get_current_time_us();
	fps.current_te_delay_time_us = (time - fps.trigger_lcd_time_us);
	fps.total_te_delay_time_us += fps.current_te_delay_time_us;
}

void DBG_OnLcdDone(void)
{
	long int time;

	if (!dbg_opt.en_fps_log && !dbg_opt.en_touch_latency_log)
		return;

	/* deal with touch latency log */

	time = get_current_time_us();
	fps.current_lcd_time_us = (time - fps.trigger_lcd_time_us);

#if 0				/* FIXME */
	if (dbg_opt.en_touch_latency_log && tpd_start_profiling) {

		DISP_LOG_PRINT(ANDROID_LOG_INFO, "DBG",
			       "Touch Latency: %ld ms\n",
			       (time - tpd_last_down_time) / 1000);

		DISP_LOG_PRINT(ANDROID_LOG_INFO, "DBG",
			       "LCD update time %ld ms (TE delay %ld ms + LCD %ld ms)\n",
			       fps.current_lcd_time_us / 1000,
			       fps.current_te_delay_time_us / 1000,
			       (fps.current_lcd_time_us -
				fps.current_te_delay_time_us) / 1000);

		tpd_start_profiling = 0;
	}
#endif

	if (!dbg_opt.en_fps_log)
		return;

	/* deal with fps log */

	fps.total_lcd_time_us += fps.current_lcd_time_us;
	++fps.trigger_lcd_count;

	if (fps.trigger_lcd_count >= dbg_opt.log_fps_wnd_size) {

		long int f = fps.trigger_lcd_count * 100 * 1000 * 1000 / (time - fps.start_time_us);

		long int update = fps.total_lcd_time_us * 100 / (1000 * fps.trigger_lcd_count);

		long int te = fps.total_te_delay_time_us * 100 / (1000 * fps.trigger_lcd_count);

		long int lcd = (fps.total_lcd_time_us - fps.total_te_delay_time_us) * 100
		    / (1000 * fps.trigger_lcd_count);

		pr_debug("DISP/DBG " "MTKFB FPS: %ld.%02ld, Avg. update time: %ld.%02ld ms,\n",
			 f / 100, f % 100, update / 100, update % 100);
		pr_debug("(TE delay %ld.%02ld ms, LCD %ld.%02ld ms)\n", te / 100, te % 100, lcd / 100, lcd % 100);
		reset_fps_logger();
	}
}

void DBG_OnHDMIDone(void)
{
	long int time;

	if (!dbg_opt.en_fps_log && !dbg_opt.en_touch_latency_log)
		return;

	/* deal with touch latency log */

	time = get_current_time_us();
	hdmi_fps.current_hdmi_time_us = (time - hdmi_fps.trigger_hdmi_time_us);

	if (!dbg_opt.en_fps_log)
		return;

	/* deal with fps log */

	hdmi_fps.total_hdmi_time_us += hdmi_fps.current_hdmi_time_us;
	++hdmi_fps.trigger_hdmi_count;

	if (hdmi_fps.trigger_hdmi_count >= dbg_opt.log_fps_wnd_size) {

		long int f = hdmi_fps.trigger_hdmi_count * 100 * 1000 * 1000
		    / (time - hdmi_fps.hdmi_start_time_us);

		long int update = hdmi_fps.total_hdmi_time_us * 100
		    / (1000 * hdmi_fps.trigger_hdmi_count);

		pr_debug("DISP/DBG " "[HDMI] FPS: %ld.%02ld, Avg. update time: %ld.%02ld ms\n",
			 f / 100, f % 100, update / 100, update % 100);

		reset_hdmi_fps_logger();
	}
}

/* --------------------------------------------------------------------------- */
/* Command Processor */
/* --------------------------------------------------------------------------- */
bool get_ovl1_to_mem_on(void)
{
	return enable_ovl1_to_mem;
}

void switch_ovl1_to_mem(bool on)
{
	enable_ovl1_to_mem = on;
	pr_debug("DISP/DBG " "switch_ovl1_to_mem %d\n", enable_ovl1_to_mem);
}

static int _draw_line(unsigned long addr, int l, int t, int r, int b,
		      int linepitch, unsigned int color)
{
	int i = 0;

	if (l > r || b < t)
		return -1;

	if (l == r) {		/* vertical line */
		for (i = 0; i < (b - t); i++) {
			*(unsigned long *)(addr + (t + i) * linepitch + l * 4) =
			    color;
		}
	} else if (t == b) {	/* horizontal line */
		for (i = 0; i < (r - l); i++) {
			*(unsigned long *)(addr + t * linepitch + (l + i) * 4) =
			    color;
		}
	} else {		/* tile line, not support now */
		return -1;
	}

	return 0;
}

static int _draw_rect(unsigned long addr, int l, int t, int r, int b, unsigned int linepitch,
		      unsigned int color)
{
	int ret = 0;

	ret += _draw_line(addr, l, t, r, t, linepitch, color);
	ret += _draw_line(addr, l, t, l, b, linepitch, color);
	ret += _draw_line(addr, r, t, r, b, linepitch, color);
	ret += _draw_line(addr, l, b, r, b, linepitch, color);
	return ret;
}

static void _draw_block(unsigned long addr, unsigned int x, unsigned int y,
			unsigned int w, unsigned int h, unsigned int linepitch,
			unsigned int color)
{
	int i = 0;
	int j = 0;
	unsigned long start_addr = addr + linepitch * y + x * 4;

	DISPMSG
	    ("addr=0x%lx, start_addr=0x%lx, x=%d,y=%d,w=%d,h=%d,linepitch=%d, color=0x%08x\n",
	     addr, start_addr, x, y, w, h, linepitch, color);
	for (j = 0; j < h; j++) {
		for (i = 0; i < w; i++) {
			*(unsigned long *)(start_addr + i * 4 + j * linepitch) =
			    color;
		}
	}
}


static int g_display_debug_pattern_index;
void _debug_pattern(unsigned long mva, unsigned long va, unsigned int w, unsigned int h,
		    unsigned int linepitch, unsigned int color, unsigned int layerid,
		    unsigned int bufidx)
{
	unsigned long addr = 0;
	unsigned int layer_size = 0;
	unsigned int mapped_size = 0;
	unsigned int bcolor = 0xff808080;

	if (g_display_debug_pattern_index == 0)
		return;

	if (layerid == 0)
		bcolor = 0x0000ffff;
	else if (layerid == 1)
		bcolor = 0x00ff00ff;
	else if (layerid == 2)
		bcolor = 0xff0000ff;
	else if (layerid == 3)
		bcolor = 0xffff00ff;

	if (va) {
		addr = va;
	} else {
		layer_size = linepitch * h;
		m4u_mva_map_kernel(mva, layer_size, &addr, &mapped_size);
		if (mapped_size == 0) {
			DISPERR("m4u_mva_map_kernel failed\n");
			return;
		}
	}

	switch (g_display_debug_pattern_index) {
	case 1:
		{
			unsigned int resize_factor = layerid + 1;

			_draw_rect(addr, w / 10 * resize_factor + 0,
				   h / 10 * resize_factor + 0,
				   w / 10 * (10 - resize_factor) - 0,
				   h / 10 * (10 - resize_factor) - 0, linepitch,
				   bcolor);
			_draw_rect(addr, w / 10 * resize_factor + 1,
				   h / 10 * resize_factor + 1,
				   w / 10 * (10 - resize_factor) - 1,
				   h / 10 * (10 - resize_factor) - 1, linepitch,
				   bcolor);
			_draw_rect(addr, w / 10 * resize_factor + 2,
				   h / 10 * resize_factor + 2,
				   w / 10 * (10 - resize_factor) - 2,
				   h / 10 * (10 - resize_factor) - 2, linepitch,
				   bcolor);
			_draw_rect(addr, w / 10 * resize_factor + 3,
				   h / 10 * resize_factor + 3,
				   w / 10 * (10 - resize_factor) - 3,
				   h / 10 * (10 - resize_factor) - 3, linepitch,
				   bcolor);
			break;
		}
	case 2:
		{
			int bw = 20;
			int bh = 20;

			_draw_block(addr, bufidx % (w / bw) * bw,
				    bufidx % (w * h / bh / bh) / (w / bh) * bh,
				    bw, bh, linepitch, bcolor);
			break;
		}
	}
#ifndef CONFIG_MTK_FPGA
	/* smp_inner_dcache_flush_all(); */
	/* outer_flush_all();//remove in early porting */
#endif
	if (mapped_size)
		m4u_mva_unmap_kernel(addr, layer_size, addr);
}


static void mtkfb_process_dbg_opt(const char *opt)
{
	int ret = 0;

	if (0 == strncmp(opt, "stop_trigger_loop", 17)) {
		_cmdq_stop_trigger_loop();
		return;
	} else if (0 == strncmp(opt, "start_trigger_loop", 18)) {
		_cmdq_start_trigger_loop();
		return;
	} else if (0 == strncmp(opt, "cmdqregw:", 9)) {
		char *p = (char *)opt + 9;
		unsigned int addr = 0;
		unsigned int val = 0;

		ret = kstrtoul(p, 16, (unsigned long int *)&addr);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);
		ret = kstrtoul(p + 1, 16, (unsigned long int *)&val);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);

		if (addr)
			primary_display_cmdq_set_reg(addr, val);
		else
			return;
	} else if (0 == strncmp(opt, "idle_switch_DC", 14)) {
		if (0 == strncmp(opt + 14, "on", 2)) {
			enable_screen_idle_switch_decouple();
			DISPMSG("enable screen_idle_switch_decouple\n");
		} else if (0 == strncmp(opt + 14, "off", 3)) {
			disable_screen_idle_switch_decouple();
			DISPMSG("disable screen_idle_switch_decouple\n");
		}
	} else if (0 == strncmp(opt, "shortpath", 9)) {
		char *p = (char *)opt + 10;
		int s = 0;

		ret = kstrtoul(p, 10, (unsigned long int *)&s);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);

		DISPMSG("will %s use shorter decouple path\n", s ? "" : "not");
		disp_helper_set_option
		    (DISP_HELPER_OPTION_TWO_PIPE_INTERFACE_PATH, s);
	} else if (0 == strncmp(opt, "helper", 6)) {
		char *p = (char *)opt + 7;
		int option = 0;
		int value = 0;

		ret = kstrtoul(p, 10, (unsigned long int *)&option);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);
		ret = kstrtoul(p + 1, 10, (unsigned long int *)&value);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);

		DISPMSG("will set option %d to %d\n", option, value);
		disp_helper_set_option(option, value);
	} else if (0 == strncmp(opt, "dc565", 5)) {
		char *p = (char *)opt + 6;
		int s = 0;

		ret = kstrtoul(p, 10, (unsigned long int *)&s);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);

		DISPMSG("will %s use RGB565 decouple path\n", s ? "" : "not");
		disp_helper_set_option
		    (DISP_HELPER_OPTION_DECOUPLE_MODE_USE_RGB565, s);
	} else if (0 == strncmp(opt, "switch_mode:", 12)) {
		int session_id = MAKE_DISP_SESSION(DISP_SESSION_PRIMARY, 0);
		char *p = (char *)opt + 12;
		unsigned long sess_mode = 0;

		ret = kstrtoul(p, 10, (unsigned long int *)&sess_mode);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);

		primary_display_switch_mode(sess_mode, session_id, 1);
	} else if (0 == strncmp(opt, "dsipattern", 10)) {
		char *p = (char *)opt + 11;
		unsigned int pattern = 0;

		ret = kstrtoul(p, 16, (unsigned long int *)&pattern);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);

		if (pattern) {
			DSI_BIST_Pattern_Test(DISP_MODULE_DSI0, NULL, true,
					      pattern);
			DISPMSG("enable dsi pattern: 0x%08x\n", pattern);
		} else {
			primary_display_manual_lock();
			DSI_BIST_Pattern_Test(DISP_MODULE_DSI0, NULL, false, 0);
			primary_display_manual_unlock();
			return;
		}
	} else if (0 == strncmp(opt, "mobile:", 7)) {
		if (0 == strncmp(opt + 7, "on", 2))
			gMobilelog = 1;
		else if (0 == strncmp(opt + 7, "off", 3))
			gMobilelog = 0;
	} else if (0 == strncmp(opt, "diagnose", 8)) {
		primary_display_diagnose();
		return;
	} else if (0 == strncmp(opt, "_efuse_test", 11)) {
		primary_display_check_test();
	} else if (0 == strncmp(opt, "dprec_reset", 11)) {
		dprec_logger_reset_all();
		return;
	} else if (0 == strncmp(opt, "suspend", 4)) {
		primary_display_suspend();
		return;
	} else if (0 == strncmp(opt, "ata", 3)) {
		mtkfb_fm_auto_test();
		return;
	} else if (0 == strncmp(opt, "resume", 4)) {
		primary_display_resume();
	} else if (0 == strncmp(opt, "dalprintf", 9)) {
		DAL_Printf("display aee layer test\n");
	} else if (0 == strncmp(opt, "dalclean", 8)) {
		DAL_Clean();
	} else if (0 == strncmp(opt, "lfr_setting:", 12)) {
		char *p = (char *)opt + 12;
		unsigned int enable = 0;
		unsigned int mode = 0;
		unsigned int type = 0;
		unsigned int skip_num = 1;

		ret = kstrtoul(p, 12, (unsigned long int *)&enable);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);
		ret = kstrtoul(p + 1, 12, (unsigned long int *)&mode);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);

		DISPMSG("--------------enable/disable lfr--------------\n");
		if (enable) {
			DISPMSG("lfr enable %d mode =%d\n", enable, mode);
			enable = 1;
			DSI_Set_LFR(DISP_MODULE_DSI0, NULL, mode, type, enable,
				    skip_num);
		} else {
			DISPMSG("lfr disable %d mode=%d\n", enable, mode);
			enable = 0;
			DSI_Set_LFR(DISP_MODULE_DSI0, NULL, mode, type, enable,
				    skip_num);
		}
	} else if (0 == strncmp(opt, "vsync_switch:", 13)) {
		char *p = (char *)opt + 13;
		unsigned int method = 0;

		ret = kstrtoul(p, 13, (unsigned long int *)&method);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);
		primary_display_vsync_switch(method);

	} else if (0 == strncmp(opt, "DP", 2)) {
		char *p = (char *)opt + 3;
		unsigned int pattern = 0;

		ret = kstrtoul(p, 16, (unsigned long int *)&pattern);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);
		g_display_debug_pattern_index = pattern;
		return;
	} else if (0 == strncmp(opt, "dsi0_clk:", 9)) {
		char *p = (char *)opt + 9;
		uint32_t clk = 0;

		ret = kstrtoul(p, 10, (unsigned long int *)&clk);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);
	/* DSI_ChangeClk(DISP_MODULE_DSI0,clk); */
	} else if (0 == strncmp(opt, "diagnose", 8)) {
		primary_display_diagnose();
		return;
	} else if (0 == strncmp(opt, "switch:", 7)) {
		char *p = (char *)opt + 7;
		uint32_t mode = 0;

		ret = kstrtoul(p, 10, (unsigned long int *)&mode);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);

		primary_display_switch_dst_mode(mode % 2);
		return;
	} else if (0 == strncmp(opt, "regw:", 5)) {
		char *p = (char *)opt + 5;
		unsigned long addr = 0;
		unsigned long val = 0;

		ret = kstrtoul(p, 16, (unsigned long int *)&addr);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);
		ret = kstrtoul(p + 1, 16, (unsigned long int *)&val);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);

		if (addr)
			OUTREG32(addr, val);
		else
			return;

	} else if (0 == strncmp(opt, "regr:", 5)) {
		char *p = (char *)opt + 5;
		unsigned long addr = 0;

		ret = kstrtoul(p, 16, (unsigned long int *)&addr);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);

		if (addr) {
			pr_debug("Read register 0x%lx: 0x%08x\n", addr,
			       INREG32(addr));
		} else {
			return;
		}
	} else if (0 == strncmp(opt, "cmmva_dprec", 11)) {
		dprec_handle_option(0x7);
	} else if (0 == strncmp(opt, "cmmpa_dprec", 11)) {
		dprec_handle_option(0x3);
	} else if (0 == strncmp(opt, "dprec", 5)) {
		char *p = (char *)opt + 6;
		unsigned int option = 0;

		ret = kstrtoul(p, 16, (unsigned long int *)&option);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);

		dprec_handle_option(option);
	} else if (0 == strncmp(opt, "cmdq", 4)) {
		char *p = (char *)opt + 5;
		unsigned int option = 0;

		ret = kstrtoul(p, 16, (unsigned long int *)&option);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);

		if (option)
			primary_display_switch_cmdq_cpu(CMDQ_ENABLE);
		else
			primary_display_switch_cmdq_cpu(CMDQ_DISABLE);
	} else if (0 == strncmp(opt, "maxlayer", 8)) {
		char *p = (char *)opt + 9;
		unsigned int maxlayer = 0;

		ret = kstrtoul(p, 10, (unsigned long int *)&maxlayer);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);

		if (maxlayer)
			primary_display_set_max_layer(maxlayer);
		else
			DISPERR("can't set max layer to 0\n");
	} else if (0 == strncmp(opt, "primary_reset", 13)) {
		primary_display_reset();
	} else if (0 == strncmp(opt, "esd_check", 9)) {
		char *p = (char *)opt + 10;
		unsigned int enable = 0;

		ret = kstrtoul(p, 10, (unsigned long int *)&enable);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);

		primary_display_esd_check_enable(enable);
	} else if (0 == strncmp(opt, "esd_recovery", 12)) {
		primary_display_esd_recovery();
	} else if (0 == strncmp(opt, "lcm0_reset", 10)) {
#if 1
		DISP_CPU_REG_SET(DISPSYS_CONFIG_BASE + 0x150, 1);
		msleep(20);
		DISP_CPU_REG_SET(DISPSYS_CONFIG_BASE + 0x150, 0);
		msleep(20);
		DISP_CPU_REG_SET(DISPSYS_CONFIG_BASE + 0x150, 1);
#else
#ifdef CONFIG_MTK_LEGACY
		mt_set_gpio_mode(GPIO106 | 0x80000000, GPIO_MODE_00);
		mt_set_gpio_dir(GPIO106 | 0x80000000, GPIO_DIR_OUT);
		mt_set_gpio_out(GPIO106 | 0x80000000, GPIO_OUT_ONE);
		msleep(20);
		mt_set_gpio_out(GPIO106 | 0x80000000, GPIO_OUT_ZERO);
		msleep(20);
		mt_set_gpio_out(GPIO106 | 0x80000000, GPIO_OUT_ONE);
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
	} else if (0 == strncmp(opt, "cg", 2)) {
		char *p = (char *)opt + 2;
		unsigned int enable = 0;

		ret = kstrtoul(p, 10, (unsigned long int *)&enable);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);

		primary_display_enable_path_cg(enable);
	} else if (0 == strncmp(opt, "ovl2mem:", 8)) {
		if (0 == strncmp(opt + 8, "on", 2))
			switch_ovl1_to_mem(true);
		else
			switch_ovl1_to_mem(false);
	} else if (0 == strncmp(opt, "dump_layer:", 11)) {
		if (0 == strncmp(opt + 11, "on", 2)) {
			char *p = (char *)opt + 14;

			ret = kstrtoul(p, 10, (unsigned long int *)&gCapturePriLayerDownX);
			if (ret)
				pr_err("DISP/%s: errno %d\n", __func__, ret);
			ret = kstrtoul(p + 1, 10, (unsigned long int *)&gCapturePriLayerDownY);
			if (ret)
				pr_err("DISP/%s: errno %d\n", __func__, ret);
			ret = kstrtoul(p + 1, 10, (unsigned long int *)&gCapturePriLayerNum);
			if (ret)
				pr_err("DISP/%s: errno %d\n", __func__, ret);
			gCapturePriLayerEnable = 1;
			gCaptureWdmaLayerEnable = 1;
			if (gCapturePriLayerDownX == 0)
				gCapturePriLayerDownX = 20;
			if (gCapturePriLayerDownY == 0)
				gCapturePriLayerDownY = 20;
			pr_debug("dump_layer En %d DownX %d DownY %d,Num %d",
			       gCapturePriLayerEnable, gCapturePriLayerDownX,
			       gCapturePriLayerDownY, gCapturePriLayerNum);

		} else if (0 == strncmp(opt + 11, "off", 3)) {
			gCapturePriLayerEnable = 0;
			gCaptureWdmaLayerEnable = 0;
			gCapturePriLayerNum = OVL_LAYER_NUM;
			pr_debug("dump_layer En %d\n", gCapturePriLayerEnable);
		}
	} else if (0 == strncmp(opt, "fps:", 4)) {
		if (0 == strncmp(opt + 4, "on", 2))
			dbg_opt.en_fps_log = 1;
		else if (0 == strncmp(opt + 4, "off", 3))
			dbg_opt.en_fps_log = 0;
		else
			goto Error;

		reset_fps_logger();
	} else if (0 == strncmp(opt, "bkl:", 4)) {
		char *p = (char *)opt + 4;
		unsigned long int level = 0;

		ret = kstrtoul(p, 10, (unsigned long int *)&level);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);

		pr_debug("mtkfb_process_dbg_opt(), set backlight level = %ld\n",
			       level);
		primary_display_setbacklight(level);
	}

	return;
Error:
	pr_debug("parse command error!\n\n%s",
		       MTKFB_STR_HELP);
}

static void mtkfb_process_dbg_cmd(char *cmd)
{
	char *tok;

	pr_debug("DISP/DBG " "[mtkfb_dbg] %s\n", cmd);

	while ((tok = strsep(&cmd, " ")) != NULL)
		mtkfb_process_dbg_opt(tok);
}

/* --------------------------------------------------------------------------- */
/* Debug FileSystem Routines */
/* --------------------------------------------------------------------------- */

struct dentry *mtkfb_dbgfs = NULL;


static int mtkfb_debug_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static char debug_buffer[4096 + 30 * 16 * 1024];

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
	static int n;

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

	n += primary_display_check_path(debug_buffer + n, debug_bufmax - n);

	n += dprec_logger_get_buf(DPREC_LOGGER_ERROR, debug_buffer + n, debug_bufmax - n);

	n += dprec_logger_get_buf(DPREC_LOGGER_FENCE, debug_buffer + n, debug_bufmax - n);

	n += dprec_logger_get_buf(DPREC_LOGGER_DUMP, debug_buffer + n, debug_bufmax - n);

	n += dprec_logger_get_buf(DPREC_LOGGER_DEBUG, debug_buffer + n, debug_bufmax - n);

out:
	return simple_read_from_buffer(ubuf, count, ppos, debug_buffer, n);
}

static ssize_t mtkfb_debug_write(struct file *file,
			   const char __user *ubuf, size_t count,
			   loff_t *ppos)
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

#ifdef MTKFB_DEBUG_FS_CAPTURE_LAYER_CONTENT_SUPPORT

static ssize_t layer_debug_open(struct inode *inode, struct file *file)
{
	MTKFB_LAYER_DBG_OPTIONS *dbgopt;
	/* record the private data */
	file->private_data = inode->i_private;
	dbgopt = (MTKFB_LAYER_DBG_OPTIONS *) file->private_data;

	dbgopt->working_size =
	    DISP_GetScreenWidth() * DISP_GetScreenHeight() * 2 + 32;
	dbgopt->working_buf = (unsigned long)vmalloc(dbgopt->working_size);
	if (dbgopt->working_buf == 0)
		pr_debug("DISP/DBG Vmalloc to get temp buffer failed\n");

	return 0;
}

static ssize_t layer_debug_read(struct file *file,
				char __user *ubuf, size_t count, loff_t *ppos)
{
	return 0;
}

static ssize_t layer_debug_write(struct file *file,
				 const char __user *ubuf, size_t count,
				 loff_t *ppos)
{
	MTKFB_LAYER_DBG_OPTIONS *dbgopt =
	    (MTKFB_LAYER_DBG_OPTIONS *) file->private_data;

	pr_debug("DISP/DBG " "mtkfb_layer%d write is not implemented yet\n",
		       dbgopt->layer_index);

	return count;
}

static int layer_debug_release(struct inode *inode, struct file *file)
{
	MTKFB_LAYER_DBG_OPTIONS *dbgopt;

	dbgopt = (MTKFB_LAYER_DBG_OPTIONS *) file->private_data;

	if (dbgopt->working_buf != 0)
		vfree((void *)dbgopt->working_buf);

	dbgopt->working_buf = 0;

	return 0;
}

static const struct file_operations layer_debug_fops = {
	.read = layer_debug_read,
	.write = layer_debug_write,
	.open = layer_debug_open,
	.release = layer_debug_release,
};

#endif

void DBG_Init(void)
{
	mtkfb_dbgfs = debugfs_create_file("mtkfb",
					  S_IFREG | S_IRUGO, NULL, (void *)0,
					  &mtkfb_debug_fops);

	memset(&dbg_opt, 0, sizeof(dbg_opt));
	memset(&fps, 0, sizeof(fps));
	dbg_opt.log_fps_wnd_size = DEFAULT_LOG_FPS_WND_SIZE;
	/* enable fps log by default */
	dbg_opt.en_fps_log = 1;

#ifdef MTKFB_DEBUG_FS_CAPTURE_LAYER_CONTENT_SUPPORT
	{
		unsigned int i;
		unsigned char a[13];

		a[0] = 'm';
		a[1] = 't';
		a[2] = 'k';
		a[3] = 'f';
		a[4] = 'b';
		a[5] = '_';
		a[6] = 'l';
		a[7] = 'a';
		a[8] = 'y';
		a[9] = 'e';
		a[10] = 'r';
		a[11] = '0';
		a[12] = '\0';

		for (i = 0; i < DDP_OVL_LAYER_MUN; i++) {
			a[11] = '0' + i;
			mtkfb_layer_dbg_opt[i].layer_index = i;
			mtkfb_layer_dbgfs[i] = debugfs_create_file(a,
								   S_IFREG |
								   S_IRUGO,
								   NULL,
								   (void *)
								   &mtkfb_layer_dbg_opt[i], &layer_debug_fops);
		}
	}
#endif
}

void DBG_Deinit(void)
{
	debugfs_remove(mtkfb_dbgfs);
#ifdef MTKFB_DEBUG_FS_CAPTURE_LAYER_CONTENT_SUPPORT
	{
		unsigned int i;

		for (i = 0; i < DDP_OVL_LAYER_MUN; i++)
			debugfs_remove(mtkfb_layer_dbgfs[i]);
	}
#endif
}
