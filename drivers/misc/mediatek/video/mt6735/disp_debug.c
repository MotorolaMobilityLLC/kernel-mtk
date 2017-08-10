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
#include <mt-plat/aee.h>
#include "disp_assert_layer.h"
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/vmalloc.h>

#include "m4u.h"

#include "cmdq_def.h"
#include "cmdq_record.h"
#include "cmdq_reg.h"
#include "cmdq_core.h"

#include "disp_drv_ddp.h"

#include "ddp_reg.h"
#include "ddp_drv.h"
#include "ddp_wdma.h"
#include "ddp_wdma_ex.h"
#include "ddp_hal.h"
#include "ddp_path.h"
#include "ddp_color.h"
#include "ddp_aal.h"
#include "ddp_pwm.h"
#include "ddp_info.h"
#include "ddp_dsi.h"
#include "ddp_ovl.h"

#include "ddp_manager.h"
#include "disp_log.h"
#include "ddp_met.h"
#include "display_recorder.h"
#include "disp_session.h"
#include "primary_display.h"
#include "ddp_irq.h"
#include "mtk_disp_mgr.h"
#include "disp_drv_platform.h"

#pragma GCC optimize("O0")

#ifndef DISP_NO_AEE
#define ddp_aee_print(string, args...) do {							\
	char ddp_name[100];									\
	snprintf(ddp_name, 100, "[DDP]"string, ##args);						\
	aee_kernel_warning_api(__FILE__, __LINE__, DB_OPT_MMPROFILE_BUFFER,			\
			       ddp_name, "[DDP] error"string, ##args);				\
	pr_err("DDP " "error: "string, ##args);							\
} while (0)
#else
#define ddp_aee_print(string, args...) pr_err("DDP " "error: "string, ##args)
#endif

/* --------------------------------------------------------------------------- */
/* External variable declarations */
/* --------------------------------------------------------------------------- */
/* --------------------------------------------------------------------------- */
/* Debug Options */
/* --------------------------------------------------------------------------- */
static const long int DEFAULT_LOG_FPS_WND_SIZE = 30;
unsigned int gOVLBackground = 0x0;
unsigned int gDumpMemoutCmdq = 0;
unsigned int gEnableUnderflowAEE = 0;

unsigned int disp_low_power_enlarge_blanking = 0;
unsigned int disp_low_power_disable_ddp_clock = 0;
unsigned int disp_low_power_disable_fence_thread = 0;
unsigned int disp_low_power_remove_ovl = 1;
unsigned int gSkipIdleDetect = 0;
unsigned int gDumpClockStatus = 1;
#ifdef DISP_ENABLE_SODI_FOR_VIDEO_MODE
unsigned int gEnableSODIControl = 1;
  /* workaround for SVP IT, todo: please K fix it */
#if defined(CONFIG_TRUSTONIC_TEE_SUPPORT) && defined(CONFIG_MTK_SEC_VIDEO_PATH_SUPPORT)
unsigned int gPrefetchControl = 0;
#else
unsigned int gPrefetchControl = 1;
#endif
#else
unsigned int gEnableSODIControl = 0;
unsigned int gPrefetchControl = 0;
#endif

/* mutex SOF at raing edge of vsync, can save more time for cmdq config */
unsigned int gEnableMutexRisingEdge = 0;
/* only write dirty register, reduce register number write by cmdq */
unsigned int gEnableReduceRegWrite = 0;

unsigned int gDumpConfigCMD = 0;
unsigned int gDumpESDCMD = 0;

unsigned int gESDEnableSODI = 1;
unsigned int gEnableOVLStatusCheck = 0;
unsigned int gEnableDSIStateCheck = 0;

unsigned int gResetRDMAEnable = 1;
unsigned int gEnableSWTrigger = 0;
unsigned int gMutexFreeRun = 1;

unsigned int gResetOVLInAALTrigger = 0;
unsigned int gDisableOVLTF = 0;

unsigned long int gRDMAUltraSetting = 0;	/* so we can modify RDMA ultra at run-time */
unsigned long int gRDMAFIFOLen = 32;
static bool enable_ovl1_to_mem = true;

#ifdef _MTK_USER_
unsigned int gEnableIRQ = 0;
/* #error eng_error */
#else
unsigned int gEnableIRQ = 1;
/* #error user_error */
#endif
unsigned int gDisableSODIForTriggerLoop = 1;
struct MTKFB_MMP_Events_t MTKFB_MMP_Events;
char DDP_STR_HELP[] =
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

char MTKFB_STR_HELP[] = "";

struct dentry *mtkfb_layer_dbgfs[DDP_OVL_LAYER_MUN];
typedef struct {
	uint32_t layer_index;
	unsigned long working_buf;
	uint32_t working_size;
} MTKFB_LAYER_DBG_OPTIONS;

MTKFB_LAYER_DBG_OPTIONS mtkfb_layer_dbg_opt[DDP_OVL_LAYER_MUN];

/* --------------------------------------------------------------------------- */
/* Command Processor */
/* --------------------------------------------------------------------------- */
static void process_dbg_debug(const char *opt)
{
	static disp_session_config config;
	unsigned long int enable = 0;
	char *p;
	char *buf = dbg_buf + strlen(dbg_buf);
	int ret;

	p = (char *)opt + 6;
	ret = kstrtoul(p, 10, &enable);
	if (ret)
		pr_err("DISP/%s: errno %d\n", __func__, ret);

	if (enable == 1) {
		DISPMSG("[DDP] debug=1, trigger AEE\n");
		/* aee_kernel_exception("DDP-TEST-ASSERT", "[DDP] DDP-TEST-ASSERT"); */
	} else if (enable == 2) {
		ddp_mem_test();
	} else if (enable == 3) {
		ddp_lcd_test();
	} else if (enable == 4) {
		DISPAEE("test enable=%d\n", (unsigned int)enable);
		sprintf(buf, "test enable=%d\n", (unsigned int)enable);
	} else if (enable == 5) {

		if (gDDPError == 0)
			gDDPError = 1;
		else
			gDDPError = 0;

		sprintf(buf, "bypass PQ: %d\n", gDDPError);
		DISPMSG("bypass PQ: %d\n", gDDPError);
	} else if (enable == 6) {
		unsigned int i = 0;
		int *modules = ddp_get_scenario_list(DDP_SCENARIO_PRIMARY_DISP);
		int module_num = ddp_get_module_num(DDP_SCENARIO_PRIMARY_DISP);

		pr_debug("dump path status:");
		for (i = 0; i < module_num; i++)
			pr_debug("%s-", ddp_get_module_name(modules[i]));

		pr_debug("\n");

		ddp_dump_analysis(DISP_MODULE_CONFIG);
		ddp_dump_analysis(DISP_MODULE_MUTEX);
		for (i = 0; i < module_num; i++)
			ddp_dump_analysis(modules[i]);

		if (primary_display_is_decouple_mode()) {
			ddp_dump_analysis(DISP_MODULE_OVL0);
#if defined(OVL_CASCADE_SUPPORT)
			ddp_dump_analysis(DISP_MODULE_OVL1);
#endif
			ddp_dump_analysis(DISP_MODULE_WDMA0);
		}

		ddp_dump_reg(DISP_MODULE_CONFIG);
		ddp_dump_reg(DISP_MODULE_MUTEX);

		if (primary_display_is_decouple_mode()) {
			ddp_dump_reg(DISP_MODULE_OVL0);
			ddp_dump_reg(DISP_MODULE_OVL1);
			ddp_dump_reg(DISP_MODULE_WDMA0);
		}

		for (i = 0; i < module_num; i++)
			ddp_dump_reg(modules[i]);

	} else if (enable == 7) {
		if (dbg_log_level < 3)
			dbg_log_level++;
		else
			dbg_log_level = 0;

		pr_debug("DDP: dbg_log_level=%d\n", dbg_log_level);
		sprintf(buf, "dbg_log_level: %d\n", dbg_log_level);
	} else if (enable == 8) {
		DISPDMP("clock_mm setting:%u\n", DISP_REG_GET(DISP_REG_CONFIG_C11));
		if ((DISP_REG_GET(DISP_REG_CONFIG_C11) & 0xff000000) != 0xff000000)
			DISPDMP("error, MM clock bit 24~bit31 should be 1, but real value=0x%x",
				DISP_REG_GET(DISP_REG_CONFIG_C11));

	} else if (enable == 9) {
		gOVLBackground = 0xFF0000FF;
		pr_debug("DDP: gOVLBackground=%d\n", gOVLBackground);
		sprintf(buf, "gOVLBackground: %d\n", gOVLBackground);
	} else if (enable == 10) {
		gOVLBackground = 0xFF000000;
		pr_debug("DDP: gOVLBackground=%d\n", gOVLBackground);
		sprintf(buf, "gOVLBackground: %d\n", gOVLBackground);
	} else if (enable == 11) {
		unsigned int i = 0;
		char *buf_temp = buf;
		unsigned int size = sizeof(dbg_buf) - strlen(dbg_buf);

		for (i = 0; i < DISP_REG_NUM; i++) {
			DISPDMP("i=%d, module=%s, va=0x%lx, pa=0x%x, irq(%d,%d)\n",
				i, ddp_get_reg_module_name(i), dispsys_reg[i],
				ddp_reg_pa_base[i], dispsys_irq[i], ddp_irq_num[i]);
			snprintf(buf_temp, size,  "i=%d, module=%s, va=0x%lx, pa=0x%x, irq(%d,%d)\n", i,
				ddp_get_reg_module_name(i), dispsys_reg[i],
				ddp_reg_pa_base[i], dispsys_irq[i], ddp_irq_num[i]);
			buf_temp += strlen(buf_temp);
		}
	} else if (enable == 12) {
		if (gUltraEnable == 0)
			gUltraEnable = 1;
		else
			gUltraEnable = 0;

		pr_debug("DDP: gUltraEnable=%d\n", gUltraEnable);
		sprintf(buf, "gUltraEnable: %d\n", gUltraEnable);
	} else if (enable == 13) {
		int ovl_status = ovl_get_status();

		config.type = DISP_SESSION_MEMORY;
		config.device_id = 0;
		disp_create_session(&config);
		pr_debug("old status=%d, ovl1 status=%d\n", ovl_status, ovl_get_status());
		sprintf(buf, "old status=%d, ovl1 status=%d\n", ovl_status,
			ovl_get_status());
	} else if (enable == 14) {
		int ovl_status = ovl_get_status();

		disp_destroy_session(&config);
		pr_debug("old status=%d, ovl1 status=%d\n", ovl_status, ovl_get_status());
		sprintf(buf, "old status=%d, ovl1 status=%d\n", ovl_status,
			ovl_get_status());
	} else if (enable == 15) {
		/* extern smi_dumpDebugMsg(void); */
		ddp_dump_analysis(DISP_MODULE_CONFIG);
		ddp_dump_analysis(DISP_MODULE_RDMA0);
		ddp_dump_analysis(DISP_MODULE_OVL0);
#if defined(OVL_CASCADE_SUPPORT)
		ddp_dump_analysis(DISP_MODULE_OVL1);
#endif

		/* dump ultra/preultra related regs */
		DISPMSG("wdma_con1(2c)=0x%x, wdma_con2(0x38)=0x%x,\n",
			DISP_REG_GET(DISP_REG_WDMA_BUF_CON1),
			DISP_REG_GET(DISP_REG_WDMA_BUF_CON2));
		DISPMSG("rdma_gmc0(30)=0x%x, rdma_gmc1(38)=0x%x, fifo_con(40)=0x%x\n",
			DISP_REG_GET(DISP_REG_RDMA_MEM_GMC_SETTING_0),
			DISP_REG_GET(DISP_REG_RDMA_MEM_GMC_SETTING_1),
			DISP_REG_GET(DISP_REG_RDMA_FIFO_CON));
		DISPMSG("ovl0_gmc: 0x%x, 0x%x, 0x%x, 0x%x, ovl1_gmc: 0x%x, 0x%x, 0x%x, 0x%x,\n",
			DISP_REG_GET(DISP_REG_OVL_RDMA0_MEM_GMC_SETTING),
			DISP_REG_GET(DISP_REG_OVL_RDMA1_MEM_GMC_SETTING),
			DISP_REG_GET(DISP_REG_OVL_RDMA2_MEM_GMC_SETTING),
			DISP_REG_GET(DISP_REG_OVL_RDMA3_MEM_GMC_SETTING),
			DISP_REG_GET(DISP_REG_OVL_RDMA0_MEM_GMC_SETTING +
					DISP_OVL_INDEX_OFFSET),
			DISP_REG_GET(DISP_REG_OVL_RDMA1_MEM_GMC_SETTING +
					DISP_OVL_INDEX_OFFSET),
			DISP_REG_GET(DISP_REG_OVL_RDMA2_MEM_GMC_SETTING +
					DISP_OVL_INDEX_OFFSET),
			DISP_REG_GET(DISP_REG_OVL_RDMA3_MEM_GMC_SETTING +
					DISP_OVL_INDEX_OFFSET));

		/* dump smi regs */
		/* smi_dumpDebugMsg(); */

	} else if (enable == 16) {
		if (gDumpMemoutCmdq == 0)
			gDumpMemoutCmdq = 1;
		else
			gDumpMemoutCmdq = 0;

		pr_debug("DDP: gDumpMemoutCmdq=%d\n", gDumpMemoutCmdq);
		sprintf(buf, "gDumpMemoutCmdq: %d\n", gDumpMemoutCmdq);
	} else if (enable == 21) {
		if (gEnableSODIControl == 0)
			gEnableSODIControl = 1;
		else
			gEnableSODIControl = 0;

		pr_debug("DDP: gEnableSODIControl=%d\n", gEnableSODIControl);
		sprintf(buf, "gEnableSODIControl: %d\n", gEnableSODIControl);
	} else if (enable == 22) {
		if (gPrefetchControl == 0)
			gPrefetchControl = 1;
		else
			gPrefetchControl = 0;

		pr_debug("DDP: gPrefetchControl=%d\n", gPrefetchControl);
		sprintf(buf, "gPrefetchControl: %d\n", gPrefetchControl);
	} else if (enable == 23) {
		if (disp_low_power_enlarge_blanking == 0)
			disp_low_power_enlarge_blanking = 1;
		else
			disp_low_power_enlarge_blanking = 0;

		pr_debug("DDP: disp_low_power_enlarge_blanking=%d\n",
			 disp_low_power_enlarge_blanking);
		sprintf(buf, "disp_low_power_enlarge_blanking: %d\n",
			disp_low_power_enlarge_blanking);

	} else if (enable == 24) {
		if (disp_low_power_disable_ddp_clock == 0)
			disp_low_power_disable_ddp_clock = 1;
		else
			disp_low_power_disable_ddp_clock = 0;

		pr_debug("DDP: disp_low_power_disable_ddp_clock=%d\n",
			 disp_low_power_disable_ddp_clock);
		sprintf(buf, "disp_low_power_disable_ddp_clock: %d\n",
			disp_low_power_disable_ddp_clock);

	} else if (enable == 25) {
		if (disp_low_power_disable_fence_thread == 0)
			disp_low_power_disable_fence_thread = 1;
		else
			disp_low_power_disable_fence_thread = 0;

		pr_debug("DDP: disp_low_power_disable_fence_thread=%d\n",
			 disp_low_power_disable_fence_thread);
		sprintf(buf, "disp_low_power_disable_fence_thread: %d\n",
			disp_low_power_disable_fence_thread);

	} else if (enable == 26) {
		if (disp_low_power_remove_ovl == 0)
			disp_low_power_remove_ovl = 1;
		else
			disp_low_power_remove_ovl = 0;

		pr_debug("DDP: disp_low_power_remove_ovl=%d\n", disp_low_power_remove_ovl);
		sprintf(buf, "disp_low_power_remove_ovl: %d\n", disp_low_power_remove_ovl);

	} else if (enable == 27) {
		if (gSkipIdleDetect == 0)
			gSkipIdleDetect = 1;
		else
			gSkipIdleDetect = 0;

		pr_debug("DDP: gSkipIdleDetect=%d\n", gSkipIdleDetect);
		sprintf(buf, "gSkipIdleDetect: %d\n", gSkipIdleDetect);

	} else if (enable == 28) {
		if (gDumpClockStatus == 0)
			gDumpClockStatus = 1;
		else
			gDumpClockStatus = 0;

		pr_debug("DDP: gDumpClockStatus=%d\n", gDumpClockStatus);
		sprintf(buf, "gDumpClockStatus: %d\n", gDumpClockStatus);

	} else if (enable == 29) {
		if (g_enable_uart_log == 0)
			g_enable_uart_log = 1;
		else
			g_enable_uart_log = 0;

		pr_debug("DDP: g_enable_uart_log=%d\n", g_enable_uart_log);
		sprintf(buf, "g_enable_uart_log: %d\n", g_enable_uart_log);

	} else if (enable == 30) {
		if (gEnableMutexRisingEdge == 0) {
			gEnableMutexRisingEdge = 1;
			DISP_REG_SET_FIELD(0, SOF_FLD_MUTEX0_SOF_TIMING,
					   DISP_REG_CONFIG_MUTEX0_SOF, 1);
		} else {
			gEnableMutexRisingEdge = 0;
			DISP_REG_SET_FIELD(0, SOF_FLD_MUTEX0_SOF_TIMING,
					   DISP_REG_CONFIG_MUTEX0_SOF, 0);
		}

		pr_debug("DDP: gEnableMutexRisingEdge=%d\n", gEnableMutexRisingEdge);
		sprintf(buf, "gEnableMutexRisingEdge: %d\n", gEnableMutexRisingEdge);

	} else if (enable == 31) {
		if (gEnableReduceRegWrite == 0)
			gEnableReduceRegWrite = 1;
		else
			gEnableReduceRegWrite = 0;

		pr_debug("DDP: gEnableReduceRegWrite=%d\n", gEnableReduceRegWrite);
		sprintf(buf, "gEnableReduceRegWrite: %d\n", gEnableReduceRegWrite);

	} else if (enable == 32) {
		DISPAEE("DDP: (32)gEnableReduceRegWrite=%d\n", gEnableReduceRegWrite);
	} else if (enable == 33) {
		if (gDumpConfigCMD == 0)
			gDumpConfigCMD = 1;
		else
			gDumpConfigCMD = 0;

		pr_debug("DDP: gDumpConfigCMD=%d\n", gDumpConfigCMD);
		sprintf(buf, "gDumpConfigCMD: %d\n", gDumpConfigCMD);

	} else if (enable == 34) {
		if (gESDEnableSODI == 0)
			gESDEnableSODI = 1;
		else
			gESDEnableSODI = 0;

		pr_debug("DDP: gESDEnableSODI=%d\n", gESDEnableSODI);
		sprintf(buf, "gESDEnableSODI: %d\n", gESDEnableSODI);

	} else if (enable == 35) {
		if (gEnableOVLStatusCheck == 0)
			gEnableOVLStatusCheck = 1;
		else
			gEnableOVLStatusCheck = 0;

		pr_debug("DDP: gEnableOVLStatusCheck=%d\n", gEnableOVLStatusCheck);
		sprintf(buf, "gEnableOVLStatusCheck: %d\n", gEnableOVLStatusCheck);

	} else if (enable == 36) {
		if (gResetRDMAEnable == 0)
			gResetRDMAEnable = 1;
		else
			gResetRDMAEnable = 0;

		pr_debug("DDP: gResetRDMAEnable=%d\n", gResetRDMAEnable);
		sprintf(buf, "gResetRDMAEnable: %d\n", gResetRDMAEnable);
	} else if (enable == 37) {
		unsigned int reg_value = 0;

		if (gEnableIRQ == 0) {
			gEnableIRQ = 1;

			DISP_CPU_REG_SET(DISP_REG_OVL_INTEN, 0x1e2);
			DISP_CPU_REG_SET(DISP_REG_OVL_INTEN + DISP_OVL_INDEX_OFFSET, 0x1e2);

			reg_value = DISP_REG_GET(DISP_REG_CONFIG_MUTEX_INTEN);
			DISP_CPU_REG_SET(DISP_REG_CONFIG_MUTEX_INTEN,
					 reg_value | (1 << 0) | (1 << DISP_MUTEX_TOTAL));
		} else {
			gEnableIRQ = 0;

			DISP_CPU_REG_SET(DISP_REG_OVL_INTEN, 0x1e0);
			DISP_CPU_REG_SET(DISP_REG_OVL_INTEN + DISP_OVL_INDEX_OFFSET, 0x1e0);

			reg_value = DISP_REG_GET(DISP_REG_CONFIG_MUTEX_INTEN);
			DISP_CPU_REG_SET(DISP_REG_CONFIG_MUTEX_INTEN,
					 reg_value & (~(1 << 0)) &
					 (~(1 << DISP_MUTEX_TOTAL)));

		}

		pr_debug("DDP: gEnableIRQ=%d\n", gEnableIRQ);
		sprintf(buf, "gEnableIRQ: %d\n", gEnableIRQ);

	} else if (enable == 38) {
		if (gDisableSODIForTriggerLoop == 0)
			gDisableSODIForTriggerLoop = 1;
		else
			gDisableSODIForTriggerLoop = 0;

		pr_debug("DDP: gDisableSODIForTriggerLoop=%d\n",
			 gDisableSODIForTriggerLoop);
		sprintf(buf, "gDisableSODIForTriggerLoop: %d\n",
			gDisableSODIForTriggerLoop);

	} else if (enable == 39) {
		cmdqCoreSetEvent(CMDQ_SYNC_TOKEN_STREAM_EOF);
		cmdqCoreSetEvent(CMDQ_EVENT_DISP_RDMA0_EOF);
		sprintf(buf, "enable=%d\n", (unsigned int)enable);
	} else if (enable == 41) {
		if (gResetOVLInAALTrigger == 0)
			gResetOVLInAALTrigger = 1;
		else
			gResetOVLInAALTrigger = 0;

		pr_debug("DDP: gResetOVLInAALTrigger=%d\n", gResetOVLInAALTrigger);
		sprintf(buf, "gResetOVLInAALTrigger: %d\n", gResetOVLInAALTrigger);

	} else if (enable == 42) {
		if (gDisableOVLTF == 0)
			gDisableOVLTF = 1;
		else
			gDisableOVLTF = 0;

		pr_debug("DDP: gDisableOVLTF=%d\n", gDisableOVLTF);
		sprintf(buf, "gDisableOVLTF: %d\n", gDisableOVLTF);

	} else if (enable == 43) {
		if (gDumpESDCMD == 0)
			gDumpESDCMD = 1;
		else
			gDumpESDCMD = 0;

		pr_debug("DDP: gDumpESDCMD=%d\n", gDumpESDCMD);
		sprintf(buf, "gDumpESDCMD: %d\n", gDumpESDCMD);

	} else if (enable == 44) {
		/* extern void disp_dump_emi_status(void); */
		disp_dump_emi_status();
		sprintf(buf, "dump emi status!\n");
	} else if (enable == 40) {
		sprintf(buf, "version: %d\n", 7);
	} else if (enable == 45) {
		ddp_aee_print("DDP AEE DUMP!!\n");
	} else if (enable == 46) {
		ASSERT(0);
	} else if (enable == 47) {
		if (gEnableDSIStateCheck == 0)
			gEnableDSIStateCheck = 1;
		else
			gEnableDSIStateCheck = 0;

		pr_debug("DDP: gEnableDSIStateCheck=%d\n", gEnableDSIStateCheck);
		sprintf(buf, "gEnableDSIStateCheck: %d\n", gEnableDSIStateCheck);
	} else if (enable == 48) {
		if (gMutexFreeRun == 0)
			gMutexFreeRun = 1;
		else
			gMutexFreeRun = 0;

		pr_debug("DDP: gMutexFreeRun=%d\n", gMutexFreeRun);
		sprintf(buf, "gMutexFreeRun: %d\n", gMutexFreeRun);
	}
}

int get_ovl1_to_mem_on(void)
{
	return enable_ovl1_to_mem;
}

void switch_ovl1_to_mem(bool on)
{
	enable_ovl1_to_mem = on;
	pr_debug("DISP/DBG " "switch_ovl1_to_mem %d\n", enable_ovl1_to_mem);
}

void ddp_process_dbg_opt(const char *opt)
{
	char *buf = dbg_buf + strlen(dbg_buf);
	int ret = 0;
	char *p;

	if (0 == strncmp(opt, "regr:", 5)) {
		unsigned long addr = 0;

		p = (char *)opt + 5;
		ret = kstrtoul(p, 16, (unsigned long int *)&addr);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);

		if (is_reg_addr_valid(1, addr) == 1) {	/* (addr >= 0xf0000000U && addr <= 0xff000000U) */
			unsigned int regVal = DISP_REG_GET(addr);

			DISPMSG("regr: 0x%lx = 0x%08X\n", addr, regVal);
			sprintf(buf, "regr: 0x%lx = 0x%08X\n", addr, regVal);
		} else {
			sprintf(buf, "regr, invalid address 0x%lx\n", addr);
			goto Error;
		}
	} else if (0 == strncmp(opt, "regw:", 5)) {
		unsigned long addr = 0;
		unsigned long val = 0;

		p = (char *)opt + 5;
		ret = kstrtoul(p, 16, (unsigned long int *)&addr);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);
		ret = kstrtoul(p + 1, 16, (unsigned long int *)&val);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);


		if (is_reg_addr_valid(1, addr) == 1) {	/* (addr >= 0xf0000000U && addr <= 0xff000000U) */
			unsigned int regVal;

			DISP_CPU_REG_SET(addr, val);
			regVal = DISP_REG_GET(addr);
			DISPMSG("regw: 0x%lx, 0x%08X = 0x%08X\n", addr, (int) val, regVal);
			sprintf(buf, "regw: 0x%lx, 0x%08X = 0x%08X\n", addr, (int) val, regVal);
		} else {
			sprintf(buf, "regw, invalid address 0x%lx\n", addr);
			goto Error;
		}
	} else if (0 == strncmp(opt, "rdma_ultra:", 11)) {
		p = (char *)opt + 11;
		ret = kstrtoul(p, 16, &gRDMAUltraSetting);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);

		DISP_CPU_REG_SET(DISP_REG_RDMA_MEM_GMC_SETTING_0, gRDMAUltraSetting);
		sprintf(buf, "rdma_ultra, gRDMAUltraSetting=0x%x, reg=0x%x\n",
			(unsigned int)gRDMAUltraSetting, DISP_REG_GET(DISP_REG_RDMA_MEM_GMC_SETTING_0));
	} else if (0 == strncmp(opt, "rdma_fifo:", 10)) {
		p = (char *)opt + 10;
		ret = kstrtoul(p, 16, &gRDMAFIFOLen);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);

		DISP_CPU_REG_SET_FIELD(FIFO_CON_FLD_OUTPUT_VALID_FIFO_THRESHOLD,
				       DISP_REG_RDMA_FIFO_CON, gRDMAFIFOLen);
		sprintf(buf, "rdma_fifo, gRDMAFIFOLen=0x%x, reg=0x%x\n",
			(unsigned int)gRDMAFIFOLen, DISP_REG_GET(DISP_REG_RDMA_FIFO_CON));
	} else if (0 == strncmp(opt, "g_regr:", 7)) {
		unsigned int reg_va_before;
		unsigned long reg_va;
		unsigned long reg_pa = 0;
		unsigned long size;

		p = (char *)opt + 7;
		ret = kstrtoul(p, 16, (unsigned long int *)&reg_pa);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);

		if (reg_pa < 0x10000000 || reg_pa > 0x18000000) {
			sprintf(buf, "g_regr, invalid pa=0x%lx\n", reg_pa);
		} else {
			size = sizeof(unsigned long);
			reg_va = (unsigned long)ioremap_nocache(reg_pa, size);
			reg_va_before = DISP_REG_GET(reg_va);
			pr_debug("g_regr, pa=%lx, va=0x%lx, reg_val=0x%x\n",
				 reg_pa, reg_va, reg_va_before);
			sprintf(buf, "g_regr, pa=%lx, va=0x%lx, reg_val=0x%x\n",
				reg_pa, reg_va, reg_va_before);

			iounmap((void *)reg_va);
		}
	} else if (0 == strncmp(opt, "g_regw:", 7)) {
		unsigned int reg_va_before;
		unsigned int reg_va_after;
		unsigned long int val;
		unsigned long reg_va;
		unsigned long reg_pa = 0;
		unsigned long size;

		p = (char *)opt + 7;
		ret = kstrtoul(p, 16, (unsigned long int *)&reg_pa);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);

		if (reg_pa < 0x10000000 || reg_pa > 0x18000000) {
			sprintf(buf, "g_regw, invalid pa=0x%lx\n", reg_pa);
		} else {
			ret = kstrtoul(p + 1, 16, &val);
			if (ret)
				pr_err("DISP/%s: errno %d\n", __func__, ret);

			size = sizeof(unsigned long);
			reg_va = (unsigned long)ioremap_nocache(reg_pa, size);
			reg_va_before = DISP_REG_GET(reg_va);
			DISP_CPU_REG_SET(reg_va, val);
			reg_va_after = DISP_REG_GET(reg_va);

			pr_debug("g_regw, pa=%lx, va=0x%lx, value=0x%x, reg_val_before=0x%x, reg_val_after=0x%x\n",
				 reg_pa, reg_va, (unsigned int)val, reg_va_before, reg_va_after);
			sprintf(buf, "g_regw, pa=%lx, va=0x%lx, value=0x%x, reg_val_before=0x%x, reg_val_after=0x%x\n",
				reg_pa, reg_va, (unsigned int)val, reg_va_before, reg_va_after);

			iounmap((void *)reg_va);
		}
	} else if (0 == strncmp(opt, "dbg_log:", 8)) {
		unsigned long int enable = 0;

		p = (char *)opt + 8;
		ret = kstrtoul(p, 10, &enable);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);

		if (enable)
			dbg_log_level = 1;
		else
			dbg_log_level = 0;

		sprintf(buf, "dbg_log: %d\n", dbg_log_level);
	} else if (0 == strncmp(opt, "irq_log:", 8)) {
		unsigned long int enable = 0;

		p = (char *)opt + 8;
		ret = kstrtoul(p, 10, &enable);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);

		if (enable)
			irq_log_level = 1;
		else
			irq_log_level = 0;

		sprintf(buf, "irq_log: %d\n", irq_log_level);
	} else if (0 == strncmp(opt, "met_on:", 7)) {
		unsigned long int met_on = 0;
		int rdma0_mode = 0;
		int rdma1_mode = 0;

		p = (char *)opt + 7;
		ret = kstrtoul(p, 10, &met_on);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);
		if (0 == strncmp(p, "1", 1))
			met_on = 1;

		ddp_init_met_tag((unsigned int)met_on, rdma0_mode, rdma1_mode);
		DISPMSG("process_dbg_opt, met_on=%d,rdma0_mode %d, rdma1 %d\n",
			(unsigned int)met_on, rdma0_mode, rdma1_mode);
		sprintf(buf, "met_on:%d,rdma0_mode:%d,rdma1_mode:%d\n",
			(unsigned int)met_on, rdma0_mode, rdma1_mode);
	} else if (0 == strncmp(opt, "backlight:", 10)) {
		unsigned long level = 0;

		p = (char *)opt + 10;
		ret = kstrtoul(p, 10, (unsigned long int *)&level);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);

		if (level) {
			disp_bls_set_backlight(level);
			sprintf(buf, "backlight: %d\n", (int) level);
		} else {
			goto Error;
		}
	} else if (0 == strncmp(opt, "pwm0:", 5) || 0 == strncmp(opt, "pwm1:", 5)) {
		unsigned long int level = 0;

		p = (char *)opt + 5;
		ret = kstrtoul(p, 10, &level);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);

		if (level) {
			disp_pwm_id_t pwm_id = DISP_PWM0;

			if (opt[3] == '1')
				pwm_id = DISP_PWM1;

			disp_pwm_set_backlight(pwm_id, (unsigned int)level);
			sprintf(buf, "PWM 0x%x : %d\n", pwm_id, (unsigned int)level);
		} else {
			goto Error;
		}
	} else if (0 == strncmp(opt, "aal_dbg:", 8)) {
		unsigned long int tmp;

		ret = kstrtoul(opt + 8, 10, &tmp);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);
		aal_dbg_en = (int)tmp;
		sprintf(buf, "aal_dbg_en = 0x%x\n", aal_dbg_en);
	}  else if (strncmp(opt, "color_dbg:", 10) == 0) {
		char *p = (char *)opt + 10;
		unsigned int debug_level;

		ret = kstrtouint(p, 0, &debug_level);
		if (ret) {
			snprintf(buf, 50, "error to parse cmd %s\n", opt);
			return;
		}

		disp_color_dbg_log_level(debug_level);

		sprintf(buf, "color_dbg_en = 0x%x\n", debug_level);
	} else if (0 == strncmp(opt, "aal_test:", 9)) {
		aal_test(opt + 9, buf);
	} else if (0 == strncmp(opt, "pwm_test:", 9)) {
		disp_pwm_test(opt + 9, buf);
	} else if (0 == strncmp(opt, "dump_reg:", 9)) {
		unsigned long int module = 0;

		p = (char *)opt + 9;
		ret = kstrtoul(p, 10, &module);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);

		DISPMSG("process_dbg_opt, module=%d\n", (unsigned int)module);
		if (module < DISP_MODULE_NUM) {
			ddp_dump_reg(module);
			sprintf(buf, "dump_reg: %d\n", (unsigned int)module);
		} else {
			DISPMSG("process_dbg_opt2, module=%d\n", (unsigned int)module);
			goto Error;
		}

	} else if (0 == strncmp(opt, "dump_path:", 10)) {
		unsigned long int mutex_idx = 0;

		p = (char *)opt + 10;
		ret = kstrtoul(p, 10, &mutex_idx);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);

		DISPMSG("process_dbg_opt, path mutex=%d\n", (unsigned int)mutex_idx);
		dpmgr_debug_path_status((unsigned int)mutex_idx);
		sprintf(buf, "dump_path: %d\n", (unsigned int)mutex_idx);

	} else if (0 == strncmp(opt, "debug:", 6)) {
		process_dbg_debug(opt);
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

void mtkfb_process_dbg_opt(const char *opt)
{
	int ret = 0;

	if (0 == strncmp(opt, "dsipattern", 10)) {
		char *p = (char *)opt + 11;
		unsigned long int pattern = 0;

		ret = kstrtoul(p, 16, &pattern);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);

		if (pattern) {
			DSI_BIST_Pattern_Test(DISP_MODULE_DSI0, NULL, true, pattern);
			DISPMSG("enable dsi pattern: 0x%08lx\n", pattern);
		} else {
			primary_display_manual_lock();
			DSI_BIST_Pattern_Test(DISP_MODULE_DSI0, NULL, false, 0);
			primary_display_manual_unlock();
			return;
		}
	} else if (0 == strncmp(opt, "dvfs_test:", 10)) {
		char *p = (char *)opt + 10;
		unsigned long int val;

		ret = kstrtoul(p, 16, &val);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);

		switch (val) {
		case 0:
		case 1:
		case 2:
			/* normal test */
			primary_display_switch_mmsys_clk(dvfs_test, val);
			break;

		default:
			/* finish */
			break;
		}

		pr_err("DISP/ERROR " "DVFS mode:%d->%ld\n", dvfs_test, val);

		dvfs_test = val;
	} else if (0 == strncmp(opt, "mobile:", 7)) {
		if (0 == strncmp(opt + 7, "on", 2))
			g_mobilelog = 1;
		else if (0 == strncmp(opt + 7, "off", 3))
			g_mobilelog = 0;
	} else if (0 == strncmp(opt, "freeze:", 7)) {
		if (0 == strncmp(opt + 7, "on", 2))
			display_freeze_mode(1, 1);
		else if (0 == strncmp(opt + 7, "off", 3))
			display_freeze_mode(0, 1);
	} else if (0 == strncmp(opt, "diagnose", 8)) {
		primary_display_diagnose();
		return;
	} else if (0 == strncmp(opt, "_efuse_test", 11)) {
		primary_display_check_test();
	} else if (0 == strncmp(opt, "trigger", 7)) {
		display_primary_path_context *ctx = primary_display_path_lock("debug");

		if (ctx)
			dpmgr_signal_event(ctx->dpmgr_handle, DISP_PATH_EVENT_TRIGGER);

		primary_display_path_unlock("debug");

		return;
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
	} else if (0 == strncmp(opt, "DP", 2)) {
		char *p = (char *)opt + 3;
		unsigned long int pattern = 0;

		ret = kstrtoul(p, 16, &pattern);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);

		g_display_debug_pattern_index = (int)pattern;
		return;
	} else if (0 == strncmp(opt, "dsi0_clk:", 9)) {
		char *p = (char *)opt + 9;
		unsigned long int clk = 0;

		ret = kstrtoul(p, 10, &clk);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);

		DSI_ChangeClk(DISP_MODULE_DSI0, (uint32_t)clk);
	} else if (0 == strncmp(opt, "switch:", 7)) {
		char *p = (char *)opt + 7;
		unsigned long int mode = 0;

		ret = kstrtoul(p, 10, &mode);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);

		primary_display_switch_dst_mode((uint32_t)mode % 2);
		return;
	} else if (0 == strncmp(opt, "disp_mode:", 10)) {
		char *p = (char *)opt + 10;
		unsigned long int disp_mode = 0;

		ret = kstrtoul(p, 10, &disp_mode);
		gTriggerDispMode = (int)disp_mode;
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);

		DISPMSG("DDP: gTriggerDispMode=%d\n", gTriggerDispMode);
	} else if (0 == strncmp(opt, "regw:", 5)) {
		char *p = (char *)opt + 5;
		unsigned long addr = 0;
		unsigned long val = 0;

		ret = kstrtoul(p, 16, &addr);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);
		ret = kstrtoul(p + 1, 16, &val);
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

		if (addr)
			pr_debug("Read register 0x%lx: 0x%08x\n", addr, INREG32(addr));
		else
			return;

	} else if (0 == strncmp(opt, "cmmva_dprec", 11)) {
		dprec_handle_option(0x7);
	} else if (0 == strncmp(opt, "cmmpa_dprec", 11)) {
		dprec_handle_option(0x3);
	} else if (0 == strncmp(opt, "dprec", 5)) {
		char *p = (char *)opt + 6;
		unsigned long int option = 0;

		ret = kstrtoul(p, 16, (unsigned long int *)&option);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);

		dprec_handle_option((int)option);
	} else if (0 == strncmp(opt, "cmdq", 4)) {
		char *p = (char *)opt + 5;
		unsigned long int option = 0;

		ret = kstrtoul(p, 16, &option);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);

		if (option)
			primary_display_switch_cmdq_cpu(CMDQ_ENABLE);
		else
			primary_display_switch_cmdq_cpu(CMDQ_DISABLE);
	} else if (0 == strncmp(opt, "maxlayer", 8)) {
		char *p = (char *)opt + 9;
		unsigned long int maxlayer = 0;

		ret = kstrtoul(p, 10, &maxlayer);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);

		if (maxlayer)
			primary_display_set_max_layer((int)maxlayer);
		else
			DISPERR("can't set max layer to 0\n");
	} else if (0 == strncmp(opt, "primary_reset", 13)) {
		primary_display_reset();
	} else if (0 == strncmp(opt, "esd_check", 9)) {
		char *p = (char *)opt + 10;
		unsigned long int enable = 0;

		ret = kstrtoul(p, 10, &enable);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);

		primary_display_esd_check_enable((int)enable);
	} else if (0 == strncmp(opt, "cmd:", 4)) {
		char *p = (char *)opt + 4;
		unsigned long int value = 0;
		int lcm_cmd[5];
		unsigned int cmd_num = 1;

		ret = kstrtoul(p, 5, &value);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);

		lcm_cmd[0] = (int)value;
		primary_display_set_cmd(lcm_cmd, cmd_num);
	} else if (0 == strncmp(opt, "esd_recovery", 12)) {
		primary_display_esd_recovery();
	} else if (0 == strncmp(opt, "lcm0_reset", 10)) {
#if 1
		DISP_CPU_REG_SET(DDP_REG_BASE_MMSYS_CONFIG + 0x150, 1);
		/* msleep(10); */
		usleep_range(10000, 11000);
		DISP_CPU_REG_SET(DDP_REG_BASE_MMSYS_CONFIG + 0x150, 0);
		/* msleep(10); */
		usleep_range(10000, 11000);
		DISP_CPU_REG_SET(DDP_REG_BASE_MMSYS_CONFIG + 0x150, 1);

#else
#if 0
		mt_set_gpio_mode(GPIO106 | 0x80000000, GPIO_MODE_00);
		mt_set_gpio_dir(GPIO106 | 0x80000000, GPIO_DIR_OUT);
		mt_set_gpio_out(GPIO106 | 0x80000000, GPIO_OUT_ONE);
		/* msleep(10); */
		usleep_range(10000, 11000);
		mt_set_gpio_out(GPIO106 | 0x80000000, GPIO_OUT_ZERO);
		/* msleep(10); */
		usleep_range(10000, 11000);
		mt_set_gpio_out(GPIO106 | 0x80000000, GPIO_OUT_ONE);
#endif
#endif
	} else if (0 == strncmp(opt, "lcm0_reset0", 11)) {
		DISP_CPU_REG_SET(DDP_REG_BASE_MMSYS_CONFIG + 0x150, 0);
	} else if (0 == strncmp(opt, "lcm0_reset1", 11)) {
		DISP_CPU_REG_SET(DDP_REG_BASE_MMSYS_CONFIG + 0x150, 1);
	} else if (0 == strncmp(opt, "cg", 2)) {
		char *p = (char *)opt + 2;
		unsigned long int enable = 0;

		ret = kstrtoul(p, 10, &enable);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);

		primary_display_enable_path_cg((int)enable);
	} else if (0 == strncmp(opt, "ovl2mem:", 8)) {
		if (0 == strncmp(opt + 8, "on", 2))
			switch_ovl1_to_mem(true);
		else
			switch_ovl1_to_mem(false);
	} else if (0 == strncmp(opt, "dump_layer:", 11)) {
		if (0 == strncmp(opt + 11, "on", 2)) {
			char *p = (char *)opt + 14;
			unsigned long int temp;

			ret = kstrtoul(p, 10, &temp);
			gCapturePriLayerDownX = (int)temp;
			if (ret)
				pr_err("DISP/%s: errno %d\n", __func__, ret);
			ret = kstrtoul(p + 1, 10, (unsigned long int *)&temp);
			gCapturePriLayerDownY = (int)temp;
			if (ret)
				pr_err("DISP/%s: errno %d\n", __func__, ret);
			ret = kstrtoul(p + 1, 10, (unsigned long int *)&temp);
			gCapturePriLayerNum = (int)temp;
			if (ret)
				pr_err("DISP/%s: errno %d\n", __func__, ret);


			gCapturePriLayerEnable = 1;
			if (gCapturePriLayerDownX == 0)
				gCapturePriLayerDownX = 20;
			if (gCapturePriLayerDownY == 0)
				gCapturePriLayerDownY = 20;
			pr_debug("dump_layer En %d DownX %d DownY %d,Num %d",
				 gCapturePriLayerEnable, gCapturePriLayerDownX,
				 gCapturePriLayerDownY, gCapturePriLayerNum);

		} else if (0 == strncmp(opt + 11, "off", 3)) {
			gCapturePriLayerEnable = 0;
			gCapturePriLayerNum = OVL_LAYER_NUM;
			pr_debug("dump_layer En %d\n", gCapturePriLayerEnable);
		}
	} else if (0 == strncmp(opt, "dump_decouple:", 14)) {
		if (0 == strncmp(opt + 14, "on", 2)) {
			char *p = (char *)opt + 17;
			unsigned long int temp;

			ret = kstrtoul(p, 10, &temp);
			gCapturePriLayerDownX = (int)temp;
			if (ret)
				pr_err("DISP/%s: errno %d\n", __func__, ret);
			ret = kstrtoul(p + 1, 10, &temp);
			gCapturePriLayerDownY = (int)temp;
			if (ret)
				pr_err("DISP/%s: errno %d\n", __func__, ret);
			ret = kstrtoul(p + 1, 10, &temp);
			gCapturePriLayerNum = (int)temp;
			if (ret)
				pr_err("DISP/%s: errno %d\n", __func__, ret);

			gCaptureWdmaLayerEnable = 1;
			if (gCapturePriLayerDownX == 0)
				gCapturePriLayerDownX = 20;
			if (gCapturePriLayerDownY == 0)
				gCapturePriLayerDownY = 20;
			pr_debug("dump_decouple En %d DownX %d DownY %d,Num %d",
				 gCaptureWdmaLayerEnable, gCapturePriLayerDownX,
				 gCapturePriLayerDownY, gCapturePriLayerNum);

		} else if (0 == strncmp(opt + 14, "off", 3)) {
			gCaptureWdmaLayerEnable = 0;
			pr_debug("dump_decouple En %d\n", gCaptureWdmaLayerEnable);
		}
	} else if (0 == strncmp(opt, "bkl:", 4)) {
		char *p = (char *)opt + 4;
		unsigned long int level = 0;

		ret = kstrtoul(p, 10, &level);
		if (ret)
			pr_err("DISP/%s: errno %d\n", __func__, ret);

		pr_debug("process_dbg_opt(), set backlight level = %ld\n", level);
		primary_display_setbacklight(level);
	}
}

#define DEBUG_FPS_METER_SHOW_COUNT	60
static int _fps_meter_array[DEBUG_FPS_METER_SHOW_COUNT] = { 0 };

static unsigned long long _last_ts;

static void _draw_block(unsigned long addr, unsigned int x, unsigned int y, unsigned int w,
			unsigned int h, unsigned int linepitch, unsigned int color)
{
	int i = 0;
	int j = 0;
	unsigned long start_addr = addr + linepitch * y + x * 4;

	DISPMSG("addr=0x%lx, start_addr=0x%lx, x=%d,y=%d,w=%d,h=%d,linepitch=%d, color=0x%08x\n",
		addr, start_addr, x, y, w, h, linepitch, color);
	for (j = 0; j < h; j++) {
		for (i = 0; i < w; i++)
			*(unsigned long *)(start_addr + i * 4 + j * linepitch) = color;
	}
}

void _debug_fps_meter(unsigned long mva, unsigned long va, unsigned int w, unsigned int h,
		      unsigned int linepitch, unsigned int color, unsigned int layerid,
		      unsigned int bufidx)
{
	int i = 0;
	unsigned long addr = 0;
	unsigned int layer_size = 0;
	unsigned int mapped_size = 0;
	unsigned long long current_ts = sched_clock();
	unsigned long long t = current_ts;
	unsigned long mod = 0;
	unsigned long current_idx = 0;
	unsigned long long l = _last_ts;
	int ret;

	if (g_display_debug_pattern_index != 3)
		return;
	DISPMSG("layerid=%d\n", layerid);
	_last_ts = current_ts;

	if (va) {
		addr = va;
	} else {
		layer_size = linepitch * h;
		ret = m4u_mva_map_kernel(mva, layer_size, &addr, &mapped_size);
		if (mapped_size == 0 || ret < 0) {
			DISPERR("%s m4u_mva_map_kernel failed, mapped_size:%d, ret:%d\n",
				__func__, mapped_size, ret);
			return;
		}
	}

	mod = do_div(t, 1000 * 1000 * 1000);
	do_div(l, 1000 * 1000 * 1000);
	if (t != l) {
		memset((void *)_fps_meter_array, 0, sizeof(_fps_meter_array));
		_draw_block(addr, 0, 10, w, 36, linepitch, 0x00000000);
	}

	current_idx = mod / 1000 / 16666;
	DISPMSG("mod=%ld, current_idx=%ld\n", mod, current_idx);
	_fps_meter_array[current_idx]++;
	for (i = 0; i < DEBUG_FPS_METER_SHOW_COUNT; i++) {
		if (_fps_meter_array[i])
			_draw_block(addr, i * 18, 10, 18, 18 * _fps_meter_array[i], linepitch,
				    0xff0000ff);
		else
			; /* _draw_block(addr, i*18, 10, 18, 18, linepitch, 0x00000000); */

	}

/* smp_inner_dcache_flush_all(); */
/* outer_flush_all(); */
	if (mapped_size)
		m4u_mva_unmap_kernel(addr, layer_size, addr);
}

/* --------------------------------------------------------------------------- */
/* Debug FileSystem Routines */
/* --------------------------------------------------------------------------- */

char *disp_get_fmt_name(DP_COLOR_ENUM color)
{
	switch (color) {
	case DP_COLOR_FULLG8:
		return "fullg8";
	case DP_COLOR_FULLG10:
		return "fullg10";
	case DP_COLOR_FULLG12:
		return "fullg12";
	case DP_COLOR_FULLG14:
		return "fullg14";
	case DP_COLOR_UFO10:
		return "ufo10";
	case DP_COLOR_BAYER8:
		return "bayer8";
	case DP_COLOR_BAYER10:
		return "bayer10";
	case DP_COLOR_BAYER12:
		return "bayer12";
	case DP_COLOR_RGB565:
		return "rgb565";
	case DP_COLOR_BGR565:
		return "bgr565";
	case DP_COLOR_RGB888:
		return "rgb888";
	case DP_COLOR_BGR888:
		return "bgr888";
	case DP_COLOR_RGBA8888:
		return "rgba";
	case DP_COLOR_BGRA8888:
		return "bgra";
	case DP_COLOR_ARGB8888:
		return "argb";
	case DP_COLOR_ABGR8888:
		return "abgr";
	case DP_COLOR_I420:
		return "i420";
	case DP_COLOR_YV12:
		return "yv12";
	case DP_COLOR_NV12:
		return "nv12";
	case DP_COLOR_NV21:
		return "nv21";
	case DP_COLOR_I422:
		return "i422";
	case DP_COLOR_YV16:
		return "yv16";
	case DP_COLOR_NV16:
		return "nv16";
	case DP_COLOR_NV61:
		return "nv61";
	case DP_COLOR_YUYV:
		return "yuyv";
	case DP_COLOR_YVYU:
		return "yvyu";
	case DP_COLOR_UYVY:
		return "uyvy";
	case DP_COLOR_VYUY:
		return "vyuy";
	case DP_COLOR_I444:
		return "i444";
	case DP_COLOR_YV24:
		return "yv24";
	case DP_COLOR_IYU2:
		return "iyu2";
	case DP_COLOR_NV24:
		return "nv24";
	case DP_COLOR_NV42:
		return "nv42";
	case DP_COLOR_GREY:
		return "grey";
	default:
		return "undefined";
	}

}

/* --------------------------------------------------------------------------- */
/* Local Debugfs */
/* --------------------------------------------------------------------------- */
static int layer_debug_open(struct inode *inode, struct file *file)
{
	MTKFB_LAYER_DBG_OPTIONS *dbgopt;
	/* record the private data */
	file->private_data = inode->i_private;
	dbgopt = (MTKFB_LAYER_DBG_OPTIONS *) file->private_data;

	dbgopt->working_size =
	    DISP_GetScreenWidth() * DISP_GetScreenHeight() * 2 + 32;
	dbgopt->working_buf = (unsigned long)vmalloc(dbgopt->working_size);
	if (dbgopt->working_buf == 0)
		DISPMSG("DISP/DBG Vmalloc to get temp buffer failed\n");

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

	DISPMSG("DISP/DBG " "mtkfb_layer%d write is not implemented yet\n",
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

void sub_debug_init(void)
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
							   S_IFREG | S_IRUGO,
							   disp_debugDir,
							   (void *)&mtkfb_layer_dbg_opt[i],
							   &layer_debug_fops);
	}
}

void sub_debug_deinit(void)
{
	unsigned int i;

	for (i = 0; i < DDP_OVL_LAYER_MUN; i++)
		debugfs_remove(mtkfb_layer_dbgfs[i]);
}

unsigned int ddp_dump_reg_to_buf(unsigned int start_module, unsigned long *addr)
{
	unsigned int cnt = 0;
	unsigned long reg_addr;

	switch (start_module) {
	case 0:	/* DISP_MODULE_WDMA0: */
		reg_addr = DISP_REG_WDMA_INTEN;

		while (reg_addr <= DISP_REG_WDMA_PRE_ADD2) {
			addr[cnt++] = DISP_REG_GET(reg_addr);
			reg_addr += 4;
		}
		/* fallthrough */
	case 1:	/* DISP_MODULE_OVL: */
		reg_addr = DISP_REG_OVL_STA;

		while (reg_addr <= DISP_REG_OVL_L3_PITCH) {
			addr[cnt++] = DISP_REG_GET(reg_addr);
			reg_addr += 4;
		}
		/* fallthrough */
	case 2:		/* DISP_MODULE_RDMA: */
		reg_addr = DISP_REG_RDMA_INT_ENABLE;

		while (reg_addr <= DISP_REG_RDMA_PRE_ADD_1) {
			addr[cnt++] = DISP_REG_GET(reg_addr);
			reg_addr += 4;
		}
		break;
	}
	return cnt * sizeof(unsigned long);
}

unsigned int ddp_dump_lcm_param_to_buf(unsigned int start_module, unsigned long *addr)
{
	unsigned int cnt = 0;

	if (start_module == 3) {/*3 correspond dbg4*/
		addr[cnt++] = primary_display_get_width();
		addr[cnt++] = primary_display_get_height();
	}

	return cnt * sizeof(unsigned long);
}
