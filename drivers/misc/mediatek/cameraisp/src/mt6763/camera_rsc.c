/*
 * Copyright (C) 2016 MediaTek Inc.
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

/******************************************************************************
 * camera_RSC.c - Linux RSC Device Driver
 *
 * DESCRIPTION:
 *     This file provid the other drivers RSC relative functions
 *
 ******************************************************************************/
#include <linux/types.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
/* #include <asm/io.h> */
/* #include <asm/tcm.h> */
#include <linux/proc_fs.h>	/* proc file use */
/*  */
#include <linux/slab.h>
#include <linux/spinlock.h>
/* #include <linux/io.h> */
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/atomic.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/seq_file.h>

/*#include <linux/xlog.h>		 For xlog_printk(). */
/*  */
/*#include <mach/hardware.h>*/
/* #include <mach/mt6593_pll.h> */
#include "inc/camera_rsc.h"
/*#include <mach/irqs.h>*/
/* #include <mach/mt_reg_base.h> */
/* #if defined(CONFIG_MTK_LEGACY) */
/* For clock mgr APIS. enable_clock()/disable_clock(). */
/* #include <mach/mt_clkmgr.h> */
/* #endif */
#include <mt-plat/sync_write.h>	/* For mt65xx_reg_sync_writel(). */
/* For spm_enable_sodi()/spm_disable_sodi(). */
/* #include <mach/mt_spm_idle.h> */
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>

#include <m4u.h>
#include <cmdq_core.h>
#include <cmdq_record.h>
#include <smi_public.h>

/* Measure the kernel performance
 * #define __RSC_KERNEL_PERFORMANCE_MEASURE__
 */
#ifdef __RSC_KERNEL_PERFORMANCE_MEASURE__
#include <linux/met_drv.h>
#include <linux/mtk_ftrace.h>
#endif
#if 0
/* Another Performance Measure Usage */
#include <linux/kallsyms.h>
#include <linux/ftrace_event.h>
static unsigned long __read_mostly tracing_mark_write_addr;
#define _kernel_trace_begin(name) {\
	tracing_mark_write_addr = kallsyms_lookup_name("tracing_mark_write");\
	event_trace_printk(tracing_mark_write_addr, \
	"B|%d|%s\n", current->tgid, name);\
}
#define _kernel_trace_end() {\
	event_trace_printk(tracing_mark_write_addr,  "E\n");\
}
/* How to Use */
/* char strName[128]; */
/* sprintf(strName, "TAG_K_WAKEUP (%d)",sof_count[_PASS1]); */
/* _kernel_trace_begin(strName); */
/* _kernel_trace_end(); */
#endif


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

#ifdef CONFIG_COMPAT
/* 64 bit */
#include <linux/fs.h>
#include <linux/compat.h>
#endif

/*  #include "smi_common.h" */

#include <linux/wakelock.h>

/* RSC Command Queue */
/* #include "../../cmdq/mt6797/cmdq_record.h" */
/* #include "../../cmdq/mt6797/cmdq_core.h" */

/* CCF */
#if !defined(CONFIG_MTK_LEGACY) && defined(CONFIG_COMMON_CLK) /*CCF*/
#include <linux/clk.h>
struct RSC_CLK_STRUCT {
/*#define SMI_CLK*/
#ifndef SMI_CLK
	struct clk *CG_SCP_SYS_MM0;
	struct clk *CG_SCP_SYS_ISP;
	struct clk *CG_MM_SMI_COMMON;
	struct clk *CG_MM_SMI_COMMON_2X;
	struct clk *CG_MM_SMI_COMMON_GALS_M0_2X;
	struct clk *CG_MM_SMI_COMMON_GALS_M1_2X;
	struct clk *CG_MM_SMI_COMMON_UPSZ0;
	struct clk *CG_MM_SMI_COMMON_UPSZ1;
	struct clk *CG_MM_SMI_COMMON_FIFO0;
	struct clk *CG_MM_SMI_COMMON_FIFO1;
	struct clk *CG_MM_LARB5;
	struct clk *CG_IMGSYS_LARB;
#endif
	struct clk *CG_IMGSYS_RSC;
};
struct RSC_CLK_STRUCT rsc_clk;
#endif
/* !defined(CONFIG_MTK_LEGACY) && defined(CONFIG_COMMON_CLK)  */

#ifndef MTRUE
#define MTRUE               1
#endif
#ifndef MFALSE
#define MFALSE              0
#endif

#define RSC_DEV_NAME                "camera-rsc"
/*#define EP_NO_CLKMGR*/
#define BYPASS_REG         (0)
/* #define RSC_WAITIRQ_LOG  */
#define RSC_USE_GCE
/*#define RSC_DEBUG_USE*/
#define DUMMY_RSC	   (1)
/* #define RSC_MULTIPROCESS_TIMEING_ISSUE  */
/*I can' test the situation in FPGA due to slow FPGA. */
#define MyTag "[RSC]"
#define IRQTag "KEEPER"

#define LOG_VRB(format,	args...)    pr_debug(MyTag format, ##args)

#ifdef RSC_DEBUG_USE
#define LOG_DBG(format, args...)    pr_info(MyTag format, ##args)
#else
#define LOG_DBG(format, args...)
#endif

#define LOG_INF(format, args...)    pr_info(MyTag format,  ##args)
#define LOG_NOTICE(format, args...) pr_notice(MyTag format,  ##args)
#define LOG_WRN(format, args...)    pr_info(MyTag format,  ##args)
#define LOG_ERR(format, args...)    pr_info(MyTag format,  ##args)
#define LOG_AST(format, args...)    pr_info(MyTag format, ##args)


/*******************************************************************************
 *
 ******************************************************************************/
#define RSC_WR32(addr, data)   mt_reg_sync_writel(data, addr) /* For 89 Only. */
#define RSC_RD32(addr)         ioread32(addr)
/*******************************************************************************
 *
 ******************************************************************************/
/* dynamic log level */
#define RSC_DBG_DBGLOG              (0x00000001)
#define RSC_DBG_INFLOG              (0x00000002)
#define RSC_DBG_INT                 (0x00000004)
#define RSC_DBG_READ_REG            (0x00000008)
#define RSC_DBG_WRITE_REG           (0x00000010)
#define RSC_DBG_TASKLET             (0x00000020)

/*
 *    CAM interrupt status
 */

/* normal siganl : happens to be the same bit as register bit*/
/*#define RSC_INT_ST           (1<<0)*/


/*
 *   IRQ signal mask
 */

#define INT_ST_MASK_RSC     ( \
			RSC_INT_ST)

#define CMDQ_REG_MASK 0xffffffff
#define RSC_START      0x1

#define RSC_ENABLE     0x1

#define RSC_IS_BUSY    0x2


/* static irqreturn_t RSC_Irq_CAM_A(signed int  Irq,void *DeviceId); */
static irqreturn_t ISP_Irq_RSC(signed int Irq, void *DeviceId);
static bool ConfigRSC(void);
static signed int ConfigRSCHW(struct RSC_Config *pRscConfig);
static void RSC_ScheduleWork(struct work_struct *data);



typedef irqreturn_t(*IRQ_CB) (signed int, void *);

struct ISR_TABLE {
	IRQ_CB isr_fp;
	unsigned int int_number;
	char device_name[16];
};

#ifndef CONFIG_OF
const struct ISR_TABLE RSC_IRQ_CB_TBL[RSC_IRQ_TYPE_AMOUNT] = {
	{ISP_Irq_RSC, RSC_IRQ_BIT_ID, "rsc"},
};

#else
/* int number is got from kernel api */
const struct ISR_TABLE RSC_IRQ_CB_TBL[RSC_IRQ_TYPE_AMOUNT] = {
#if DUMMY_RSC
	{ISP_Irq_RSC, 0, "rsc-dummy"},
#else
	{ISP_Irq_RSC, 0, "rsc"},
#endif
};
#endif
/* ///////////////////////////////////////////////////////////// */
/*  */
typedef void (*tasklet_cb) (unsigned long);
struct Tasklet_table {
	tasklet_cb tkt_cb;
	struct tasklet_struct *pRSC_tkt;
};

struct tasklet_struct Rsctkt[RSC_IRQ_TYPE_AMOUNT];

static void ISP_TaskletFunc_RSC(unsigned long data);

static struct Tasklet_table RSC_tasklet[RSC_IRQ_TYPE_AMOUNT] = {
	{ISP_TaskletFunc_RSC, &Rsctkt[RSC_IRQ_TYPE_INT_RSC_ST]},
};

struct wake_lock RSC_wake_lock;


static DEFINE_MUTEX(gRscMutex);
static DEFINE_MUTEX(gRscDequeMutex);

#ifdef CONFIG_OF

struct RSC_device {
	void __iomem *regs;
	struct device *dev;
	int irq;
};

static struct RSC_device *RSC_devs;
static int nr_RSC_devs;

/* Get HW modules' base address from device nodes */
#define RSC_DEV_NODE_IDX 1
#define IMGSYS_DEV_MODE_IDX 0
/* static unsigned long gISPSYS_Reg[RSC_IRQ_TYPE_AMOUNT]; */


#define ISP_RSC_BASE                  (RSC_devs[RSC_DEV_NODE_IDX].regs)
#define ISP_IMGSYS_BASE               (RSC_devs[IMGSYS_DEV_MODE_IDX].regs)

/* #define ISP_RSC_BASE                  (gISPSYS_Reg[RSC_DEV_NODE_IDX]) */



#else
#define ISP_RSC_BASE                        (IMGSYS_BASE + 0x2800)

#endif


static unsigned int g_u4EnableClockCount;

/* maximum number for supporting user to do interrupt operation */
/* index 0 is for all the user that do not do register irq first */
#define IRQ_USER_NUM_MAX 32


enum RSC_FRAME_STATUS_ENUM {
	RSC_FRAME_STATUS_EMPTY,	/* 0 */
	RSC_FRAME_STATUS_ENQUE,	/* 1 */
	RSC_FRAME_STATUS_RUNNING,	/* 2 */
	RSC_FRAME_STATUS_FINISHED,	/* 3 */
	RSC_FRAME_STATUS_TOTAL
};


enum RSC_REQUEST_STATE_ENUM {
	RSC_REQUEST_STATE_EMPTY,	/* 0 */
	RSC_REQUEST_STATE_PENDING,	/* 1 */
	RSC_REQUEST_STATE_RUNNING,	/* 2 */
	RSC_REQUEST_STATE_FINISHED,	/* 3 */
	RSC_REQUEST_STATE_TOTAL
};

struct RSC_REQUEST_STRUCT {
	enum RSC_REQUEST_STATE_ENUM State;
	pid_t processID;	/* caller process ID */
	unsigned int callerID;	/* caller thread ID */

	/* to judge it belongs to which frame package */
	unsigned int enqueReqNum;
	signed int FrameWRIdx;	/* Frame write Index */
	signed int RrameRDIdx;	/* Frame read Index */
	enum RSC_FRAME_STATUS_ENUM RscFrameStatus[
		_SUPPORT_MAX_RSC_FRAME_REQUEST_];
	struct RSC_Config RscFrameConfig[_SUPPORT_MAX_RSC_FRAME_REQUEST_];
};

struct RSC_REQUEST_RING_STRUCT {
	signed int WriteIdx;	/* enque how many request  */
	signed int ReadIdx;		/* read which request index */
	signed int HWProcessIdx;	/* HWWriteIdx */
	struct RSC_REQUEST_STRUCT RSCReq_Struct[
		_SUPPORT_MAX_RSC_REQUEST_RING_SIZE_];
};

struct  RSC_CONFIG_STRUCT {
	struct RSC_Config RscFrameConfig[_SUPPORT_MAX_RSC_FRAME_REQUEST_];
};

static struct RSC_REQUEST_RING_STRUCT g_RSC_ReqRing;
static struct RSC_CONFIG_STRUCT g_RscEnqueReq_Struct;
static struct RSC_CONFIG_STRUCT g_RscDequeReq_Struct;


/*******************************************************************************
 *
 ******************************************************************************/
struct  RSC_USER_INFO_STRUCT {
	pid_t Pid;
	pid_t Tid;
};
enum RSC_PROCESS_ID_ENUM {
	RSC_PROCESS_ID_NONE,
	RSC_PROCESS_ID_RSC,
	RSC_PROCESS_ID_AMOUNT
};


/*******************************************************************************
 *
 ******************************************************************************/
struct RSC_IRQ_INFO_STRUCT {
	unsigned int Status[RSC_IRQ_TYPE_AMOUNT];
	signed int RscIrqCnt;
	pid_t ProcessID[RSC_PROCESS_ID_AMOUNT];
	unsigned int Mask[RSC_IRQ_TYPE_AMOUNT];
};


struct RSC_INFO_STRUCT {
	spinlock_t SpinLockRSCRef;
	spinlock_t SpinLockRSC;
	spinlock_t SpinLockIrq[RSC_IRQ_TYPE_AMOUNT];
	wait_queue_head_t WaitQueueHead;
	struct work_struct ScheduleRscWork;
	unsigned int UserCount;	/* User Count */
	unsigned int DebugMask;	/* Debug Mask */
	signed int IrqNum;
	struct RSC_IRQ_INFO_STRUCT IrqInfo;
	signed int WriteReqIdx;
	signed int ReadReqIdx;
	pid_t ProcessID[_SUPPORT_MAX_RSC_FRAME_REQUEST_];
};


static struct RSC_INFO_STRUCT RSCInfo;

enum eLOG_TYPE {
	/* currently, only used at ipl_buf_ctrl for critical section */
	_LOG_DBG = 0,
	_LOG_INF = 1,
	_LOG_ERR = 2,
	_LOG_MAX = 3,
};

#define NORMAL_STR_LEN (512)
#define ERR_PAGE 2
#define DBG_PAGE 2
#define INF_PAGE 4
/* #define SV_LOG_STR_LEN NORMAL_STR_LEN */

#define LOG_PPNUM 2
static unsigned int m_CurrentPPB;
struct SV_LOG_STR {
	unsigned int _cnt[LOG_PPNUM][_LOG_MAX];
	/* char   _str[_LOG_MAX][SV_LOG_STR_LEN]; */
	char *_str[LOG_PPNUM][_LOG_MAX];
} *PSV_LOG_STR;

static void *pLog_kmalloc;
static struct SV_LOG_STR gSvLog[RSC_IRQ_TYPE_AMOUNT];

/*
 *   for irq used,keep log until IRQ_LOG_PRINTER being involked,
 *   limited:
 *   each log must shorter than 512 bytes
 *  total log length in each irq/logtype can't over 1024 bytes
 */
#if 1
#define IRQ_LOG_KEEPER(irq, ppb, logT, fmt, ...) do {\
	char *ptr; \
	char *pDes;\
	unsigned int *ptr2 = &gSvLog[irq]._cnt[ppb][logT];\
	unsigned int str_leng;\
	if (logT == _LOG_ERR) {\
		str_leng = NORMAL_STR_LEN*ERR_PAGE; \
	} else if (logT == _LOG_DBG) {\
		str_leng = NORMAL_STR_LEN*DBG_PAGE; \
	} else if (logT == _LOG_INF) {\
		str_leng = NORMAL_STR_LEN*INF_PAGE;\
	} else {\
		str_leng = 0;\
	} \
	ptr = \
	pDes = \
	(char *)&(gSvLog[irq]._str[ppb][logT][gSvLog[irq]._cnt[ppb][logT]]);   \
	sprintf((char *)(pDes), fmt, ##__VA_ARGS__);   \
	if ('\0' != gSvLog[irq]._str[ppb][logT][str_leng - 1]) {\
		LOG_ERR("log str over flow(%d)", irq);\
	} \
	while (*ptr++ != '\0') {        \
		(*ptr2)++;\
	}     \
} while (0)
#else
#define IRQ_LOG_KEEPER(irq, ppb, logT, fmt, ...) \
	xlog_printk(ANDROID_LOG_DEBUG,\
	"KEEPER", "[%s] " fmt, __func__, ##__VA_ARGS__)
#endif

#if 1
#define IRQ_LOG_PRINTER(irq, ppb_in, logT_in) do {\
	struct SV_LOG_STR *pSrc = &gSvLog[irq];\
	char *ptr;\
	unsigned int i;\
	signed int ppb = 0;\
	signed int logT = 0;\
	if (ppb_in > 1) {\
		ppb = 1;\
	} else{\
		ppb = ppb_in;\
	} \
	if (logT_in > _LOG_ERR) {\
		logT = _LOG_ERR;\
	} else{\
		logT = logT_in;\
	} \
	ptr = pSrc->_str[ppb][logT];\
	if (pSrc->_cnt[ppb][logT] != 0) {\
		if (logT == _LOG_DBG) {\
			for (i = 0; i < DBG_PAGE; i++) {\
				if (ptr[NORMAL_STR_LEN*(i+1) - 1] != '\0') {\
					ptr[NORMAL_STR_LEN*(i+1) - 1] = '\0';\
					LOG_DBG("%s", &ptr[NORMAL_STR_LEN*i]);\
				} else{\
					LOG_DBG("%s", &ptr[NORMAL_STR_LEN*i]);\
					break;\
				} \
			} \
		} \
	else if (logT == _LOG_INF) {\
		for (i = 0; i < INF_PAGE; i++) {\
			if (ptr[NORMAL_STR_LEN*(i+1) - 1] != '\0') {\
				ptr[NORMAL_STR_LEN*(i+1) - 1] = '\0';\
				LOG_INF("%s", &ptr[NORMAL_STR_LEN*i]);\
			} else{\
				LOG_INF("%s", &ptr[NORMAL_STR_LEN*i]);\
				break;\
			} \
		} \
	} \
	else if (logT == _LOG_ERR) {\
		for (i = 0; i < ERR_PAGE; i++) {\
			if (ptr[NORMAL_STR_LEN*(i+1) - 1] != '\0') {\
				ptr[NORMAL_STR_LEN*(i+1) - 1] = '\0';\
				LOG_ERR("%s", &ptr[NORMAL_STR_LEN*i]);\
			} else{\
				LOG_ERR("%s", &ptr[NORMAL_STR_LEN*i]);\
				break;\
			} \
		} \
	} \
	else {\
		LOG_ERR("N.S.%d", logT);\
	} \
		ptr[0] = '\0';\
		pSrc->_cnt[ppb][logT] = 0;\
	} \
} while (0)


#else
#define IRQ_LOG_PRINTER(irq, ppb, logT)
#endif

#define IMGSYS_REG_CG_CON               (ISP_IMGSYS_BASE + 0x0)
#define IMGSYS_REG_CG_SET               (ISP_IMGSYS_BASE + 0x4)
#define IMGSYS_REG_CG_CLR               (ISP_IMGSYS_BASE + 0x8)

/* RSC unmapped base address macro for GCE to access */
#define RSC_RST_HW                    (RSC_BASE_HW)
#define RSC_START_HW                  (RSC_BASE_HW + 0x04)

#define RSC_DCM_CTL_HW                (RSC_BASE_HW + 0x08)
#define RSC_DCM_STAUS_HW              (RSC_BASE_HW + 0x0C)

#define RSC_INT_CTL_HW                (RSC_BASE_HW + 0x10)
#define RSC_INT_STATUS_HW             (RSC_BASE_HW + 0x14)
#define RSC_CTRL_HW                   (RSC_BASE_HW + 0x18)
#define RSC_SIZE_HW                   (RSC_BASE_HW + 0x1C)

#define RSC_SR_HW                     (RSC_BASE_HW + 0x20)
#define RSC_BR_HW                     (RSC_BASE_HW + 0x24)
#define RSC_MV_OFFSET_HW              (RSC_BASE_HW + 0x28)
#define RSC_GMV_OFFSET_HW             (RSC_BASE_HW + 0x2c)
#define RSC_PREPARE_MV_CTRL_HW        (RSC_BASE_HW + 0x30)
#define RSC_CAND_NUM_HW               (RSC_BASE_HW + 0x34)
#define RSC_EVEN_CAND_SEL_0_HW        (RSC_BASE_HW + 0x38)
#define RSC_EVEN_CAND_SEL_1_HW        (RSC_BASE_HW + 0x3c)
#define RSC_EVEN_CAND_SEL_2_HW        (RSC_BASE_HW + 0x40)
#define RSC_EVEN_CAND_SEL_3_HW        (RSC_BASE_HW + 0x44)
#define RSC_EVEN_CAND_SEL_4_HW        (RSC_BASE_HW + 0x48)
#define RSC_ODD_CAND_SEL_0_HW         (RSC_BASE_HW + 0x4C)
#define RSC_ODD_CAND_SEL_1_HW         (RSC_BASE_HW + 0x50)
#define RSC_ODD_CAND_SEL_2_HW         (RSC_BASE_HW + 0x54)
#define RSC_ODD_CAND_SEL_3_HW         (RSC_BASE_HW + 0x58)
#define RSC_ODD_CAND_SEL_4_HW         (RSC_BASE_HW + 0x5C)
#define RSC_RAND_HORZ_LUT_HW          (RSC_BASE_HW + 0x60)
#define RSC_RAND_VERT_LUT_HW          (RSC_BASE_HW + 0x64)
#define RSC_CURR_BLK_CTRL_HW          (RSC_BASE_HW + 0x68)
#define RSC_SAD_CTRL_HW               (RSC_BASE_HW + 0x6C)
#define RSC_SAD_EDGE_GAIN_CTRL_HW     (RSC_BASE_HW + 0x70)
#define RSC_SAD_CRNR_GAIN_CTRL_HW     (RSC_BASE_HW + 0x74)
#define RSC_STILL_STRIP_CTRL_0_HW     (RSC_BASE_HW + 0x78)
#define RSC_STILL_STRIP_CTRL_1_HW     (RSC_BASE_HW + 0x7C)
#define RSC_MV_PNLTY_CTRL_HW          (RSC_BASE_HW + 0x80)
#define RSC_ZERO_PNLTY_CTRL_HW        (RSC_BASE_HW + 0x84)
#define RSC_RAND_PNLTY_CTRL_HW        (RSC_BASE_HW + 0x88)
#define RSC_RAND_PNLTY_GAIN_CTRL_0_HW (RSC_BASE_HW + 0x8C)
#define RSC_RAND_PNLTY_GAIN_CTRL_1_HW (RSC_BASE_HW + 0x90)
#define RSC_TMPR_PNLTY_GAIN_CTRL_0_HW (RSC_BASE_HW + 0x94)
#define RSC_TMPR_PNLTY_GAIN_CTRL_1_HW (RSC_BASE_HW + 0x98)

#define RSC_IMGI_C_BASE_ADDR_HW       (RSC_BASE_HW + 0x9C)
#define RSC_IMGI_C_STRIDE_HW          (RSC_BASE_HW + 0xA0)
#define RSC_IMGI_P_BASE_ADDR_HW       (RSC_BASE_HW + 0xA4)
#define RSC_IMGI_P_STRIDE_HW          (RSC_BASE_HW + 0xA8)
#define RSC_MVI_BASE_ADDR_HW          (RSC_BASE_HW + 0xAC)
#define RSC_MVI_STRIDE_HW             (RSC_BASE_HW + 0xB0)
#define RSC_APLI_C_BASE_ADDR_HW       (RSC_BASE_HW + 0xB4)
#define RSC_APLI_P_BASE_ADDR_HW       (RSC_BASE_HW + 0xB8)
#define RSC_MVO_BASE_ADDR_HW          (RSC_BASE_HW + 0xBC)
#define RSC_MVO_STRIDE_HW             (RSC_BASE_HW + 0xC0)
#define RSC_BVO_BASE_ADDR_HW          (RSC_BASE_HW + 0xC4)
#define RSC_BVO_STRIDE_HW             (RSC_BASE_HW + 0xC8)

#define RSC_STA_0_HW                  (RSC_BASE_HW + 0x100)
#define RSC_DBG_INFO_00_HW            (RSC_BASE_HW + 0x120)
#define RSC_DBG_INFO_01_HW            (RSC_BASE_HW + 0x124)
#define RSC_DBG_INFO_02_HW            (RSC_BASE_HW + 0x128)
#define RSC_DBG_INFO_03_HW            (RSC_BASE_HW + 0x12C)
#define RSC_DBG_INFO_04_HW            (RSC_BASE_HW + 0x130)
#define RSC_DBG_INFO_05_HW            (RSC_BASE_HW + 0x134)
#define RSC_DBG_INFO_06_HW            (RSC_BASE_HW + 0x138)

#define RSC_SPARE_0_HW                (RSC_BASE_HW + 0x1F8)
#define RSC_SPARE_1_HW                (RSC_BASE_HW + 0x1FC)
#define RSC_DMA_DBG_HW                (RSC_BASE_HW + 0x7F4)
#define RSC_DMA_REQ_STATUS_HW         (RSC_BASE_HW + 0x7F8)
#define RSC_DMA_RDY_STATUS_HW         (RSC_BASE_HW + 0x7FC)

#define RSC_DMA_DMA_SOFT_RSTSTAT_HW   (RSC_BASE_HW + 0x800)
#define RSC_DMA_VERTICAL_FLIP_EN_HW   (RSC_BASE_HW + 0x804)
#define RSC_DMA_DMA_SOFT_RESET_HW     (RSC_BASE_HW + 0x808)
#define RSC_DMA_LAST_ULTRA_HW         (RSC_BASE_HW + 0x80C)
#define RSC_DMA_SPECIAL_FUN_HW        (RSC_BASE_HW + 0x810)
#define RSC_DMA_RSCO_BASE_ADDR_HW     (RSC_BASE_HW + 0x830)
#define RSC_DMA_RSCO_BASE_ADDR_2_HW   (RSC_BASE_HW + 0x834)
#define RSC_DMA_RSCO_OFST_ADDR_HW     (RSC_BASE_HW + 0x838)
#define RSC_DMA_RSCO_OFST_ADDR_2_HW   (RSC_BASE_HW + 0x83C)
#define RSC_DMA_RSCO_XSIZE_HW         (RSC_BASE_HW + 0x840)
#define RSC_DMA_RSCO_YSIZE_HW         (RSC_BASE_HW + 0x844)
#define RSC_DMA_RSCO_STRIDE_HW        (RSC_BASE_HW + 0x848)
#define RSC_DMA_RSCO_CON_HW           (RSC_BASE_HW + 0x84C)
#define RSC_DMA_RSCO_CON2_HW          (RSC_BASE_HW + 0x850)
#define RSC_DMA_RSCO_CON3_HW          (RSC_BASE_HW + 0x854)
#define RSC_DMA_RSCI_BASE_ADDR_HW     (RSC_BASE_HW + 0x890)
#define RSC_DMA_RSCI_BASE_ADDR_2_HW   (RSC_BASE_HW + 0x894)
#define RSC_DMA_RSCI_OFST_ADDR_HW     (RSC_BASE_HW + 0x898)
#define RSC_DMA_RSCI_OFST_ADDR_2_HW   (RSC_BASE_HW + 0x89C)
#define RSC_DMA_RSCI_XSIZE_HW         (RSC_BASE_HW + 0x8A0)
#define RSC_DMA_RSCI_YSIZE_HW         (RSC_BASE_HW + 0x8A4)
#define RSC_DMA_RSCI_STRIDE_HW        (RSC_BASE_HW + 0x8A8)
#define RSC_DMA_RSCI_CON_HW           (RSC_BASE_HW + 0x8AC)
#define RSC_DMA_RSCI_CON2_HW          (RSC_BASE_HW + 0x8B0)
#define RSC_DMA_RSCI_CON3_HW          (RSC_BASE_HW + 0x8B4)
#define RSC_DMA_DMA_ERR_CTRL_HW       (RSC_BASE_HW + 0x900)
#define RSC_DMA_RSCO_ERR_STAT_HW      (RSC_BASE_HW + 0x904)
#define RSC_DMA_RSCI_ERR_STAT_HW      (RSC_BASE_HW + 0x908)
#define RSC_DMA_DMA_DEBUG_ADDR_HW     (RSC_BASE_HW + 0x90C)
#define RSC_DMA_DMA_DEBUG_SEL_HW      (RSC_BASE_HW + 0x928)
#define RSC_DMA_DMA_BW_SELF_TEST_HW   (RSC_BASE_HW + 0x92C)

/*SW Access Registers : using mapped base address from DTS*/
#define RSC_RST_REG                     (ISP_RSC_BASE)
#define RSC_START_REG                  (ISP_RSC_BASE + 0x04)

#define RSC_DCM_CTL_REG                (ISP_RSC_BASE + 0x08)
#define RSC_DCM_STAUS_REG              (ISP_RSC_BASE + 0x0C)

#define RSC_INT_CTL_REG                (ISP_RSC_BASE + 0x10)
#define RSC_INT_STATUS_REG             (ISP_RSC_BASE + 0x14)
#define RSC_CTRL_REG                   (ISP_RSC_BASE + 0x18)
#define RSC_SIZE_REG                   (ISP_RSC_BASE + 0x1C)

#define RSC_SR_REG                     (ISP_RSC_BASE + 0x20)
#define RSC_BR_REG                     (ISP_RSC_BASE + 0x24)
#define RSC_MV_OFFSET_REG              (ISP_RSC_BASE + 0x28)
#define RSC_GMV_OFFSET_REG             (ISP_RSC_BASE + 0x2c)
#define RSC_PREPARE_MV_CTRL_REG        (ISP_RSC_BASE + 0x30)
#define RSC_CAND_NUM_REG               (ISP_RSC_BASE + 0x34)
#define RSC_EVEN_CAND_SEL_0_REG        (ISP_RSC_BASE + 0x38)
#define RSC_EVEN_CAND_SEL_1_REG        (ISP_RSC_BASE + 0x3c)
#define RSC_EVEN_CAND_SEL_2_REG        (ISP_RSC_BASE + 0x40)
#define RSC_EVEN_CAND_SEL_3_REG        (ISP_RSC_BASE + 0x44)
#define RSC_EVEN_CAND_SEL_4_REG        (ISP_RSC_BASE + 0x48)
#define RSC_ODD_CAND_SEL_0_REG         (ISP_RSC_BASE + 0x4C)
#define RSC_ODD_CAND_SEL_1_REG         (ISP_RSC_BASE + 0x50)
#define RSC_ODD_CAND_SEL_2_REG         (ISP_RSC_BASE + 0x54)
#define RSC_ODD_CAND_SEL_3_REG         (ISP_RSC_BASE + 0x58)
#define RSC_ODD_CAND_SEL_4_REG         (ISP_RSC_BASE + 0x5C)
#define RSC_RAND_HORZ_LUT_REG          (ISP_RSC_BASE + 0x60)
#define RSC_RAND_VERT_LUT_REG          (ISP_RSC_BASE + 0x64)
#define RSC_CURR_BLK_CTRL_REG          (ISP_RSC_BASE + 0x68)
#define RSC_SAD_CTRL_REG               (ISP_RSC_BASE + 0x6C)
#define RSC_SAD_EDGE_GAIN_CTRL_REG     (ISP_RSC_BASE + 0x70)
#define RSC_SAD_CRNR_GAIN_CTRL_REG     (ISP_RSC_BASE + 0x74)
#define RSC_STILL_STRIP_CTRL_0_REG     (ISP_RSC_BASE + 0x78)
#define RSC_STILL_STRIP_CTRL_1_REG     (ISP_RSC_BASE + 0x7C)
#define RSC_MV_PNLTY_CTRL_REG          (ISP_RSC_BASE + 0x80)
#define RSC_ZERO_PNLTY_CTRL_REG        (ISP_RSC_BASE + 0x84)
#define RSC_RAND_PNLTY_CTRL_REG        (ISP_RSC_BASE + 0x88)
#define RSC_RAND_PNLTY_GAIN_CTRL_0_REG (ISP_RSC_BASE + 0x8C)
#define RSC_RAND_PNLTY_GAIN_CTRL_1_REG (ISP_RSC_BASE + 0x90)
#define RSC_TMPR_PNLTY_GAIN_CTRL_0_REG (ISP_RSC_BASE + 0x94)
#define RSC_TMPR_PNLTY_GAIN_CTRL_1_REG (ISP_RSC_BASE + 0x98)

#define RSC_IMGI_C_BASE_ADDR_REG       (ISP_RSC_BASE + 0x9C)
#define RSC_IMGI_C_STRIDE_REG          (ISP_RSC_BASE + 0xA0)
#define RSC_IMGI_P_BASE_ADDR_REG       (ISP_RSC_BASE + 0xA4)
#define RSC_IMGI_P_STRIDE_REG          (ISP_RSC_BASE + 0xA8)
#define RSC_MVI_BASE_ADDR_REG          (ISP_RSC_BASE + 0xAC)
#define RSC_MVI_STRIDE_REG             (ISP_RSC_BASE + 0xB0)
#define RSC_APLI_C_BASE_ADDR_REG       (ISP_RSC_BASE + 0xB4)
#define RSC_APLI_P_BASE_ADDR_REG       (ISP_RSC_BASE + 0xB8)
#define RSC_MVO_BASE_ADDR_REG          (ISP_RSC_BASE + 0xBC)
#define RSC_MVO_STRIDE_REG             (ISP_RSC_BASE + 0xC0)
#define RSC_BVO_BASE_ADDR_REG          (ISP_RSC_BASE + 0xC4)
#define RSC_BVO_STRIDE_REG             (ISP_RSC_BASE + 0xC8)

#define RSC_STA_0_REG                  (ISP_RSC_BASE + 0x100)
#define RSC_DBG_INFO_00_REG            (ISP_RSC_BASE + 0x120)
#define RSC_DBG_INFO_01_REG            (ISP_RSC_BASE + 0x124)
#define RSC_DBG_INFO_02_REG            (ISP_RSC_BASE + 0x128)
#define RSC_DBG_INFO_03_REG            (ISP_RSC_BASE + 0x12C)
#define RSC_DBG_INFO_04_REG            (ISP_RSC_BASE + 0x130)
#define RSC_DBG_INFO_05_REG            (ISP_RSC_BASE + 0x134)
#define RSC_DBG_INFO_06_REG            (ISP_RSC_BASE + 0x138)

#define RSC_SPARE_0_REG                (ISP_RSC_BASE + 0x1F8)
#define RSC_SPARE_1_REG                (ISP_RSC_BASE + 0x1FC)
#define RSC_DMA_DBG_REG                (ISP_RSC_BASE + 0x7F4)
#define RSC_DMA_REQ_STATUS_REG         (ISP_RSC_BASE + 0x7F8)
#define RSC_DMA_RDY_STATUS_REG         (ISP_RSC_BASE + 0x7FC)

#define RSC_DMA_DMA_SOFT_RSTSTAT_REG   (ISP_RSC_BASE + 0x800)
#define RSC_DMA_VERTICAL_FLIP_EN_REG   (ISP_RSC_BASE + 0x804)
#define RSC_DMA_DMA_SOFT_RESET_REG     (ISP_RSC_BASE + 0x808)
#define RSC_DMA_LAST_ULTRA_REG         (ISP_RSC_BASE + 0x80C)
#define RSC_DMA_SPECIAL_FUN_REG        (ISP_RSC_BASE + 0x810)
#define RSC_DMA_RSCO_BASE_ADDR_REG     (ISP_RSC_BASE + 0x830)
#define RSC_DMA_RSCO_BASE_ADDR_2_REG   (ISP_RSC_BASE + 0x834)
#define RSC_DMA_RSCO_OFST_ADDR_REG     (ISP_RSC_BASE + 0x838)
#define RSC_DMA_RSCO_OFST_ADDR_2_REG   (ISP_RSC_BASE + 0x83C)
#define RSC_DMA_RSCO_XSIZE_REG         (ISP_RSC_BASE + 0x840)
#define RSC_DMA_RSCO_YSIZE_REG         (ISP_RSC_BASE + 0x844)
#define RSC_DMA_RSCO_STRIDE_REG        (ISP_RSC_BASE + 0x848)
#define RSC_DMA_RSCO_CON_REG           (ISP_RSC_BASE + 0x84C)
#define RSC_DMA_RSCO_CON2_REG          (ISP_RSC_BASE + 0x850)
#define RSC_DMA_RSCO_CON3_REG          (ISP_RSC_BASE + 0x854)
#define RSC_DMA_RSCI_BASE_ADDR_REG     (ISP_RSC_BASE + 0x890)
#define RSC_DMA_RSCI_BASE_ADDR_2_REG   (ISP_RSC_BASE + 0x894)
#define RSC_DMA_RSCI_OFST_ADDR_REG     (ISP_RSC_BASE + 0x898)
#define RSC_DMA_RSCI_OFST_ADDR_2_REG   (ISP_RSC_BASE + 0x89C)
#define RSC_DMA_RSCI_XSIZE_REG         (ISP_RSC_BASE + 0x8A0)
#define RSC_DMA_RSCI_YSIZE_REG         (ISP_RSC_BASE + 0x8A4)
#define RSC_DMA_RSCI_STRIDE_REG        (ISP_RSC_BASE + 0x8A8)
#define RSC_DMA_RSCI_CON_REG           (ISP_RSC_BASE + 0x8AC)
#define RSC_DMA_RSCI_CON2_REG          (ISP_RSC_BASE + 0x8B0)
#define RSC_DMA_RSCI_CON3_REG          (ISP_RSC_BASE + 0x8B4)
#define RSC_DMA_DMA_ERR_CTRL_REG       (ISP_RSC_BASE + 0x900)
#define RSC_DMA_RSCO_ERR_STAT_REG      (ISP_RSC_BASE + 0x904)
#define RSC_DMA_RSCI_ERR_STAT_REG      (ISP_RSC_BASE + 0x908)
#define RSC_DMA_DMA_DEBUG_ADDR_REG     (ISP_RSC_BASE + 0x90C)
#define RSC_DMA_DMA_DEBUG_SEL_REG      (ISP_RSC_BASE + 0x928)
#define RSC_DMA_DMA_BW_SELF_TEST_REG   (ISP_RSC_BASE + 0x92C)

/*******************************************************************************
 *
 ******************************************************************************/
static inline unsigned int RSC_MsToJiffies(unsigned int Ms)
{
	return ((Ms * HZ + 512) >> 10);
}

/*******************************************************************************
 *
 ******************************************************************************/
static inline unsigned int RSC_UsToJiffies(unsigned int Us)
{
	return (((Us / 1000) * HZ + 512) >> 10);
}

/*******************************************************************************
 *
 ******************************************************************************/
static inline unsigned int RSC_GetIRQState(unsigned int type,
	unsigned int userNumber, unsigned int stus,
	enum RSC_PROCESS_ID_ENUM whichReq, int ProcessID)
{
	unsigned int ret = 0;
	unsigned long flags; /* old: unsigned int flags;*/

	/*  */
	spin_lock_irqsave(&(RSCInfo.SpinLockIrq[type]), flags);
#ifdef RSC_USE_GCE

#ifdef RSC_MULTIPROCESS_TIMEING_ISSUE
	if (stus & RSC_INT_ST) {
		ret = ((RSCInfo.IrqInfo.RscIrqCnt > 0)
		       && (RSCInfo.ProcessID[RSCInfo.ReadReqIdx] == ProcessID));
	} else {
#define ERR_STR1 " WaitIRQ StatusErr, type:%d, userNum:%d, status:%d,"
#define ERR_STR2 " whichReq:%d,ProcessID:0x%x, ReadReqIdx:%d\n"
		LOG_ERR(ERR_STR1 ERR_STR2,
			type, userNumber, stus, whichReq, ProcessID,
			RSCInfo.ReadReqIdx);
#undef ERR_STR1
#undef ERR_STR2
	}

#else
	if (stus & RSC_INT_ST) {
		ret = ((RSCInfo.IrqInfo.RscIrqCnt > 0)
		       && (RSCInfo.IrqInfo.ProcessID[whichReq] == ProcessID));
	} else {
#define ERR_STR1 "WaitIRQ Status Error, type:%d, userNumber:%d,"
#define ERR_STR2 " status:%d, whichReq:%d, ProcessID:0x%x\n"
		LOG_ERR(ERR_STR1 ERR_STR2,
			type, userNumber, stus, whichReq, ProcessID);
#undef ERR_STR1
#undef ERR_STR2
	}
#endif
#else
	ret = ((RSCInfo.IrqInfo.Status[type] & stus)
	       && (RSCInfo.IrqInfo.ProcessID[whichReq] == ProcessID));
#endif
	spin_unlock_irqrestore(&(RSCInfo.SpinLockIrq[type]), flags);
	/*  */
	return ret;
}


/*******************************************************************************
 *
 ******************************************************************************/
static inline unsigned int RSC_JiffiesToMs(unsigned int Jiffies)
{
	return ((Jiffies * 1000) / HZ);
}

static bool ConfigRSCRequest(signed int ReqIdx)
{
#ifdef RSC_USE_GCE
	unsigned int j;
	unsigned long flags; /* old: unsigned int flags;*/


	spin_lock_irqsave(&(RSCInfo.SpinLockIrq[RSC_IRQ_TYPE_INT_RSC_ST]),
		flags);
	if (g_RSC_ReqRing.RSCReq_Struct[ReqIdx].State ==
		RSC_REQUEST_STATE_PENDING) {
		g_RSC_ReqRing.RSCReq_Struct[ReqIdx].State =
			RSC_REQUEST_STATE_RUNNING;
		for (j = 0; j < _SUPPORT_MAX_RSC_FRAME_REQUEST_; j++) {
			if (RSC_FRAME_STATUS_ENQUE ==
				g_RSC_ReqRing.RSCReq_Struct[ReqIdx].
				RscFrameStatus[j]) {

				g_RSC_ReqRing.RSCReq_Struct[ReqIdx].
					RscFrameStatus[j] =
					RSC_FRAME_STATUS_RUNNING;

				spin_unlock_irqrestore(
					&(RSCInfo.SpinLockIrq[
					RSC_IRQ_TYPE_INT_RSC_ST]), flags);

				ConfigRSCHW(
					&g_RSC_ReqRing.RSCReq_Struct[ReqIdx].
					RscFrameConfig[j]);

				spin_lock_irqsave(
					&(RSCInfo.SpinLockIrq[
					RSC_IRQ_TYPE_INT_RSC_ST]), flags);
			}
		}
	} else {
#define ERR_STR "%s state machine error!!, ReqIdx:%d, State:%d\n"
		LOG_ERR(ERR_STR, __func__,
			ReqIdx, g_RSC_ReqRing.RSCReq_Struct[ReqIdx].State);
#undef ERR_STR
	}
	spin_unlock_irqrestore(&(RSCInfo.SpinLockIrq[RSC_IRQ_TYPE_INT_RSC_ST]),
		flags);


	return MTRUE;
#else
	LOG_ERR("%s don't support this mode.!!\n", __func__);
	return MFALSE;
#endif
}


static bool ConfigRSC(void)
{
#ifdef RSC_USE_GCE

	unsigned int i, j, k;
	unsigned long flags; /* old: unsigned int flags;*/

	spin_lock_irqsave(&(RSCInfo.SpinLockIrq[RSC_IRQ_TYPE_INT_RSC_ST]),
		flags);
	for (k = 0; k < _SUPPORT_MAX_RSC_REQUEST_RING_SIZE_; k++) {
		i = (g_RSC_ReqRing.HWProcessIdx + k) %
			_SUPPORT_MAX_RSC_REQUEST_RING_SIZE_;
		if (g_RSC_ReqRing.RSCReq_Struct[i].State ==
			RSC_REQUEST_STATE_PENDING) {
			g_RSC_ReqRing.RSCReq_Struct[i].State =
			    RSC_REQUEST_STATE_RUNNING;
			for (j = 0; j < _SUPPORT_MAX_RSC_FRAME_REQUEST_; j++) {
				if (g_RSC_ReqRing.RSCReq_Struct[i].
					RscFrameStatus[j] ==
					RSC_FRAME_STATUS_ENQUE) {
					/* break; */
					g_RSC_ReqRing.RSCReq_Struct[i].
						RscFrameStatus[j] =
						RSC_FRAME_STATUS_RUNNING;

					spin_unlock_irqrestore(
						&(RSCInfo.SpinLockIrq[
						RSC_IRQ_TYPE_INT_RSC_ST]),
						flags);

					ConfigRSCHW(
						&g_RSC_ReqRing.RSCReq_Struct[i].
						RscFrameConfig[j]);

					spin_lock_irqsave(
						&(RSCInfo.SpinLockIrq[
						RSC_IRQ_TYPE_INT_RSC_ST]),
						flags);
				}
			}
			/* LOG_DBG("ConfigRSC idx j:%d\n",j); */
			if (j != _SUPPORT_MAX_RSC_FRAME_REQUEST_) {
#define ERR_STR1 "RSC Config State is wrong!"
#define ERR_STR2 "  idx j(%d), HWProcessIdx(%d), State(%d)\n"
				LOG_ERR(ERR_STR1 ERR_STR2, j,
					g_RSC_ReqRing.HWProcessIdx,
					g_RSC_ReqRing.RSCReq_Struct[i].State);
#undef ERR_STR1
#undef ERR_STR2
				return MFALSE;
			}
		}
	}
	spin_unlock_irqrestore(&(RSCInfo.SpinLockIrq[RSC_IRQ_TYPE_INT_RSC_ST]),
		flags);
	if (k == _SUPPORT_MAX_RSC_REQUEST_RING_SIZE_)
		LOG_DBG("No any RSC Request in Ring!!\n");

	return MTRUE;


#else				/* #ifdef RSC_USE_GCE */

	unsigned int i, j, k;
	unsigned int flags;

	for (k = 0; k < _SUPPORT_MAX_RSC_REQUEST_RING_SIZE_; k++) {
		i = (g_RSC_ReqRing.HWProcessIdx + k) %
			_SUPPORT_MAX_RSC_REQUEST_RING_SIZE_;
		if (g_RSC_ReqRing.RSCReq_Struct[i].State ==
			RSC_REQUEST_STATE_PENDING) {
			for (j = 0; j < _SUPPORT_MAX_RSC_FRAME_REQUEST_; j++) {
				if (RSC_FRAME_STATUS_ENQUE ==
					g_RSC_ReqRing.RSCReq_Struct[i].
					RscFrameStatus[j]) {
					break;
				}
			}
			LOG_DBG("%s idx j:%d\n", __func__, j);
			if (j != _SUPPORT_MAX_RSC_FRAME_REQUEST_) {
				g_RSC_ReqRing.RSCReq_Struct[i].
					RscFrameStatus[j] =
					RSC_FRAME_STATUS_RUNNING;
				ConfigRSCHW(&g_RSC_ReqRing.RSCReq_Struct[i].
					     RscFrameConfig[j]);
				return MTRUE;
			}
			/*else {*/
#define ERR_STR "RSC Config State is wrong! HWProcessIdx(%d), State(%d)\n"
			LOG_ERR(ERR_STR,
			     g_RSC_ReqRing.HWProcessIdx,
			     g_RSC_ReqRing.RSCReq_Struct[i].State);
#undef ERR_STR
			g_RSC_ReqRing.HWProcessIdx =
			    (g_RSC_ReqRing.HWProcessIdx +
			     1) % _SUPPORT_MAX_RSC_REQUEST_RING_SIZE_;
			/*}*/
		}
	}
	if (k == _SUPPORT_MAX_RSC_REQUEST_RING_SIZE_)
		LOG_DBG("No any RSC Request in Ring!!\n");

	return MFALSE;

#endif				/* #ifdef RSC_USE_GCE */



}


static bool UpdateRSC(pid_t *ProcessID)
{
#ifdef RSC_USE_GCE
	unsigned int i, j, next_idx;
	bool bFinishRequest = MFALSE;

	for (i = g_RSC_ReqRing.HWProcessIdx;
		i < _SUPPORT_MAX_RSC_REQUEST_RING_SIZE_; i++) {
		if (g_RSC_ReqRing.RSCReq_Struct[i].State ==
			RSC_REQUEST_STATE_RUNNING) {
			for (j = 0; j < _SUPPORT_MAX_RSC_FRAME_REQUEST_; j++) {
				if (RSC_FRAME_STATUS_RUNNING ==
					g_RSC_ReqRing.RSCReq_Struct[i].
						RscFrameStatus[j]) {
					break;
				}
			}
			IRQ_LOG_KEEPER(RSC_IRQ_TYPE_INT_RSC_ST, m_CurrentPPB,
				_LOG_DBG, "%s idx j:%d\n", __func__, j);
			if (j != _SUPPORT_MAX_RSC_FRAME_REQUEST_) {
				next_idx = j + 1;
				g_RSC_ReqRing.RSCReq_Struct[i].
					RscFrameStatus[j] =
					RSC_FRAME_STATUS_FINISHED;

				if ((next_idx ==
					_SUPPORT_MAX_RSC_FRAME_REQUEST_) ||
					((next_idx <
					_SUPPORT_MAX_RSC_FRAME_REQUEST_) &&
					(RSC_FRAME_STATUS_EMPTY ==
					g_RSC_ReqRing.RSCReq_Struct[i].
					RscFrameStatus[next_idx]))) {

					bFinishRequest = MTRUE;
					(*ProcessID) =
						g_RSC_ReqRing.RSCReq_Struct[i].
						processID;
					g_RSC_ReqRing.RSCReq_Struct[i].State =
					    RSC_REQUEST_STATE_FINISHED;
					g_RSC_ReqRing.HWProcessIdx =
					    (g_RSC_ReqRing.HWProcessIdx + 1) %
					    _SUPPORT_MAX_RSC_REQUEST_RING_SIZE_;
#define TMP_STR "Finish RSC Request i:%d, j:%d, HWProcessIdx:%d\n"
					IRQ_LOG_KEEPER(RSC_IRQ_TYPE_INT_RSC_ST,
						m_CurrentPPB, _LOG_INF, TMP_STR,
						i, j,
						g_RSC_ReqRing.HWProcessIdx);
#undef TMP_STR
				} else {
#define TMP_STR "Finish RSC Frame i:%d, j:%d, HWProcessIdx:%d\n"
					IRQ_LOG_KEEPER(RSC_IRQ_TYPE_INT_RSC_ST,
						m_CurrentPPB, _LOG_DBG, TMP_STR,
						i, j,
						g_RSC_ReqRing.HWProcessIdx);
#undef TMP_STR
				}
				break;
			}
#define ERR_STR "RSC State Machine is wrong! HWProcessIdx(%d), State(%d)\n"
			IRQ_LOG_KEEPER(RSC_IRQ_TYPE_INT_RSC_ST, m_CurrentPPB,
				_LOG_ERR, ERR_STR, g_RSC_ReqRing.HWProcessIdx,
				g_RSC_ReqRing.RSCReq_Struct[i].State);
#undef ERR_STR
			g_RSC_ReqRing.RSCReq_Struct[i].State =
			    RSC_REQUEST_STATE_FINISHED;
			g_RSC_ReqRing.HWProcessIdx =
			    (g_RSC_ReqRing.HWProcessIdx +
			     1) % _SUPPORT_MAX_RSC_REQUEST_RING_SIZE_;
			break;

		}
	}

	return bFinishRequest;


#else				/* #ifdef RSC_USE_GCE */
	unsigned int i, j, next_idx;
	bool bFinishRequest = MFALSE;

	for (i = g_RSC_ReqRing.HWProcessIdx;
		i < _SUPPORT_MAX_RSC_REQUEST_RING_SIZE_; i++) {
		if (g_RSC_ReqRing.RSCReq_Struct[i].State ==
			RSC_REQUEST_STATE_PENDING) {
			for (j = 0; j < _SUPPORT_MAX_RSC_FRAME_REQUEST_; j++) {
				if (RSC_FRAME_STATUS_RUNNING ==
					g_RSC_ReqRing.RSCReq_Struct[i].
					RscFrameStatus[j]) {
					break;
				}
			}
			IRQ_LOG_KEEPER(RSC_IRQ_TYPE_INT_RSC_ST, m_CurrentPPB,
				_LOG_DBG, "%s idx j:%d\n", __func__, j);
			if (j != _SUPPORT_MAX_RSC_FRAME_REQUEST_) {
				next_idx = j + 1;
				g_RSC_ReqRing.RSCReq_Struct[i].
					RscFrameStatus[j] =
					RSC_FRAME_STATUS_FINISHED;
				if ((_SUPPORT_MAX_RSC_FRAME_REQUEST_ ==
					next_idx) ||
					((_SUPPORT_MAX_RSC_FRAME_REQUEST_ >
					next_idx) && (RSC_FRAME_STATUS_EMPTY ==
					g_RSC_ReqRing.RSCReq_Struct[i].
					RscFrameStatus[next_idx]))) {
					bFinishRequest = MTRUE;
					(*ProcessID) = g_RSC_ReqRing.
						RSCReq_Struct[i].processID;

					g_RSC_ReqRing.RSCReq_Struct[i].State =
					    RSC_REQUEST_STATE_FINISHED;

				g_RSC_ReqRing.HWProcessIdx =
					(g_RSC_ReqRing.HWProcessIdx + 1) %
					_SUPPORT_MAX_RSC_REQUEST_RING_SIZE_;

#define TMP_STR "Finish RSC Request i:%d, j:%d, HWProcessIdx:%d\n"
					IRQ_LOG_KEEPER(RSC_IRQ_TYPE_INT_RSC_ST,
						m_CurrentPPB, _LOG_INF, TMP_STR,
						i, j,
						g_RSC_ReqRing.HWProcessIdx);
#undef TMP_STR
				} else {
#define TMP_STR "Finish RSC Frame i:%d, j:%d, HWProcessIdx:%d\n"
					IRQ_LOG_KEEPER(RSC_IRQ_TYPE_INT_RSC_ST,
						m_CurrentPPB, _LOG_DBG, TMP_STR,
						i, j,
						g_RSC_ReqRing.HWProcessIdx);
#undef TMP_STR
				}
				break;
			}

#define ERR_STR "RSC State Machine is wrong! HWProcessIdx(%d), State(%d)\n"
			IRQ_LOG_KEEPER(RSC_IRQ_TYPE_INT_RSC_ST, m_CurrentPPB,
				_LOG_ERR, ERR_STR, g_RSC_ReqRing.HWProcessIdx,
				g_RSC_ReqRing.RSCReq_Struct[i].State);
#undef ERR_STR
			g_RSC_ReqRing.RSCReq_Struct[i].State =
				RSC_REQUEST_STATE_FINISHED;
			g_RSC_ReqRing.HWProcessIdx =
				(g_RSC_ReqRing.HWProcessIdx + 1) %
				_SUPPORT_MAX_RSC_REQUEST_RING_SIZE_;
			break;

		}
	}

	return bFinishRequest;

#endif				/* #ifdef RSC_USE_GCE */


}

static signed int ConfigRSCHW(struct RSC_Config *pRscConfig)
#if !BYPASS_REG
{
#ifdef RSC_USE_GCE
		struct cmdqRecStruct *handle;
		uint64_t engineFlag = (1L << CMDQ_ENG_RSC);
#endif

	if (RSC_DBG_DBGLOG == (RSC_DBG_DBGLOG & RSCInfo.DebugMask)) {

		LOG_DBG("ConfigRSCHW Start!\n");
		LOG_DBG("RSC_CTRL_REG:0x%x!\n",
			pRscConfig->RSC_CTRL);
		LOG_DBG("RSC_SIZE_REG:0x%x!\n",
			pRscConfig->RSC_SIZE);
		LOG_DBG("RSC_IMGI_C_BASE_ADDR_REG:0x%x!\n",
			pRscConfig->RSC_IMGI_C_BASE_ADDR);
		LOG_DBG("RSC_IMGI_C_STRIDE_REG:0x%x!\n",
			pRscConfig->RSC_IMGI_C_STRIDE);
		LOG_DBG("RSC_IMGI_P_BASE_ADDR_REG:0x%x!\n",
			pRscConfig->RSC_IMGI_P_BASE_ADDR);
		LOG_DBG("RSC_IMGI_P_STRIDE_REG:0x%x!\n",
			pRscConfig->RSC_IMGI_P_STRIDE);
		LOG_DBG("RSC_MVI_C_BASE_ADDR_REG:0x%x!\n",
			pRscConfig->RSC_MVI_BASE_ADDR);
		LOG_DBG("RSC_MVI_C_STRIDE_REG:0x%x!\n",
			pRscConfig->RSC_MVI_STRIDE);
		LOG_DBG("RSC_MVO_BASE_ADDR_REG:0x%x!\n",
			pRscConfig->RSC_MVO_BASE_ADDR);
		LOG_DBG("RSC_MVO_STRIDE_REG:0x%x!\n",
			pRscConfig->RSC_MVO_STRIDE);
		LOG_DBG("RSC_BVO_BASE_ADDR_REG:0x%x!\n",
			pRscConfig->RSC_BVO_BASE_ADDR);
		LOG_DBG("RSC_BVO_STRIDE_REG:0x%x!\n",
			pRscConfig->RSC_BVO_STRIDE);
		LOG_DBG("RSC_APLI_C_BASE_ADDR_REG:0x%x!\n",
			pRscConfig->RSC_APLI_C_BASE_ADDR);
		LOG_DBG("RSC_APLI_P_BASE_ADDR_REG:0x%x!\n",
			pRscConfig->RSC_APLI_P_BASE_ADDR);
	}
#ifdef RSC_USE_GCE

#ifdef __RSC_KERNEL_PERFORMANCE_MEASURE__
	mt_kernel_trace_begin("ConfigRSCHW");
#endif

	cmdqRecCreate(CMDQ_SCENARIO_KERNEL_CONFIG_GENERAL, &handle);
	/* CMDQ driver dispatches CMDQ HW thread and HW thread's priority
	 * according to scenario
	 */

	cmdqRecSetEngine(handle, engineFlag);

	cmdqRecReset(handle);

	/* Use command queue to write register */
	/* RSC Interrupt read-clear mode */
	cmdqRecWrite(handle, RSC_INT_CTL_HW, 0x1, CMDQ_REG_MASK);

	cmdqRecWrite(handle, RSC_CTRL_HW, pRscConfig->RSC_CTRL, CMDQ_REG_MASK);
	cmdqRecWrite(handle, RSC_SIZE_HW, pRscConfig->RSC_SIZE, CMDQ_REG_MASK);

	cmdqRecWrite(handle, RSC_APLI_C_BASE_ADDR_HW,
		pRscConfig->RSC_APLI_C_BASE_ADDR, CMDQ_REG_MASK);
	cmdqRecWrite(handle, RSC_APLI_P_BASE_ADDR_HW,
		pRscConfig->RSC_APLI_P_BASE_ADDR, CMDQ_REG_MASK);
	cmdqRecWrite(handle, RSC_IMGI_C_BASE_ADDR_HW,
		pRscConfig->RSC_IMGI_C_BASE_ADDR, CMDQ_REG_MASK);
	cmdqRecWrite(handle, RSC_IMGI_P_BASE_ADDR_HW,
		pRscConfig->RSC_IMGI_P_BASE_ADDR, CMDQ_REG_MASK);
	cmdqRecWrite(handle, RSC_IMGI_C_STRIDE_HW,
		pRscConfig->RSC_IMGI_C_STRIDE, CMDQ_REG_MASK);
	cmdqRecWrite(handle, RSC_IMGI_P_STRIDE_HW,
		pRscConfig->RSC_IMGI_P_STRIDE, CMDQ_REG_MASK);

	cmdqRecWrite(handle, RSC_MVI_BASE_ADDR_HW,
		pRscConfig->RSC_MVI_BASE_ADDR, CMDQ_REG_MASK);
	cmdqRecWrite(handle, RSC_MVI_STRIDE_HW,
		pRscConfig->RSC_MVI_STRIDE, CMDQ_REG_MASK);

	cmdqRecWrite(handle, RSC_MVO_BASE_ADDR_HW,
		pRscConfig->RSC_MVO_BASE_ADDR, CMDQ_REG_MASK);
	cmdqRecWrite(handle, RSC_MVO_STRIDE_HW,
		pRscConfig->RSC_MVO_STRIDE, CMDQ_REG_MASK);
	cmdqRecWrite(handle, RSC_BVO_BASE_ADDR_HW,
		pRscConfig->RSC_BVO_BASE_ADDR, CMDQ_REG_MASK);
	cmdqRecWrite(handle, RSC_BVO_STRIDE_HW,
		pRscConfig->RSC_BVO_STRIDE, CMDQ_REG_MASK);
#ifdef RSC_TUNABLE
	cmdqRecWrite(handle, RSC_MV_OFFSET_HW,
		pRscConfig->RSC_MV_OFFSET, CMDQ_REG_MASK);
	cmdqRecWrite(handle, RSC_GMV_OFFSET_HW,
		pRscConfig->RSC_GMV_OFFSET, CMDQ_REG_MASK);
	cmdqRecWrite(handle, RSC_CAND_NUM_HW,
		pRscConfig->RSC_CAND_NUM, CMDQ_REG_MASK);
	cmdqRecWrite(handle, RSC_RAND_HORZ_LUT_HW,
		pRscConfig->RSC_RAND_HORZ_LUT, CMDQ_REG_MASK);
	cmdqRecWrite(handle, RSC_RAND_VERT_LUT_HW,
		pRscConfig->RSC_RAND_VERT_LUT, CMDQ_REG_MASK);
	cmdqRecWrite(handle, RSC_SAD_CTRL_HW,
		pRscConfig->RSC_SAD_CTRL, CMDQ_REG_MASK);
	cmdqRecWrite(handle, RSC_SAD_EDGE_GAIN_CTRL_HW,
		pRscConfig->RSC_SAD_EDGE_GAIN_CTRL, CMDQ_REG_MASK);
	cmdqRecWrite(handle, RSC_SAD_CRNR_GAIN_CTRL_HW,
		pRscConfig->RSC_SAD_CRNR_GAIN_CTRL, CMDQ_REG_MASK);
	cmdqRecWrite(handle, RSC_STILL_STRIP_CTRL_0_HW,
		pRscConfig->RSC_STILL_STRIP_CTRL0, CMDQ_REG_MASK);
	cmdqRecWrite(handle, RSC_STILL_STRIP_CTRL_1_HW,
		pRscConfig->RSC_STILL_STRIP_CTRL1, CMDQ_REG_MASK);
	cmdqRecWrite(handle, RSC_RAND_PNLTY_CTRL_HW,
		pRscConfig->RSC_RAND_PNLTY_CTRL, CMDQ_REG_MASK);
	cmdqRecWrite(handle, RSC_RAND_PNLTY_GAIN_CTRL_0_HW,
		pRscConfig->RSC_RAND_PNLTY_GAIN_CTRL0, CMDQ_REG_MASK);
	cmdqRecWrite(handle, RSC_RAND_PNLTY_GAIN_CTRL_1_HW,
		pRscConfig->RSC_RAND_PNLTY_GAIN_CTRL1, CMDQ_REG_MASK);
#endif
	/* RSC Interrupt read-clear mode */
	cmdqRecWrite(handle, RSC_START_HW, 0x1, CMDQ_REG_MASK);

	cmdqRecWait(handle, CMDQ_EVENT_RSC_EOF);

	/* RSC Interrupt read-clear mode */
	cmdqRecWrite(handle, RSC_START_HW, 0x0, CMDQ_REG_MASK);

	/* non-blocking API, Please  use cmdqRecFlushAsync() */
	cmdqRecFlushAsync(handle);

	/* if you want to re-use the handle, please reset the handle */
	cmdqRecReset(handle);
	cmdqRecDestroy(handle);	/* recycle the memory */

#ifdef __RSC_KERNEL_PERFORMANCE_MEASURE__
	mt_kernel_trace_end();
#endif

#else

#ifdef __RSC_KERNEL_PERFORMANCE_MEASURE__
	mt_kernel_trace_begin("ConfigRSCHW");
#endif
	/* RSC Interrupt enabled in read-clear mode */
	RSC_WR32(RSC_INT_CTL_REG, 0x1);

	RSC_WR32(RSC_CTRL_REG, pRscConfig->RSC_CTRL);
	RSC_WR32(RSC_SIZE_REG, pRscConfig->RSC_SIZE);

	RSC_WR32(RSC_APLI_C_BASE_ADDR_REG, pRscConfig->RSC_APLI_C_BASE_ADDR);
	RSC_WR32(RSC_APLI_P_BASE_ADDR_REG, pRscConfig->RSC_APLI_P_BASE_ADDR);
	RSC_WR32(RSC_IMGI_C_BASE_ADDR_REG, pRscConfig->RSC_IMGI_C_BASE_ADDR);
	RSC_WR32(RSC_IMGI_P_BASE_ADDR_REG, pRscConfig->RSC_IMGI_P_BASE_ADDR);
	RSC_WR32(RSC_IMGI_C_STRIDE_REG, pRscConfig->RSC_IMGI_C_STRIDE);
	RSC_WR32(RSC_IMGI_P_STRIDE_REG, pRscConfig->RSC_IMGI_P_STRIDE);

	RSC_WR32(RSC_MVI_BASE_ADDR_REG, pRscConfig->RSC_MVI_BASE_ADDR);
	RSC_WR32(RSC_MVI_STRIDE_REG, pRscConfig->RSC_MVI_STRIDE);

	RSC_WR32(RSC_MVO_BASE_ADDR_REG, pRscConfig->RSC_MVO_BASE_ADDR);
	RSC_WR32(RSC_MVO_STRIDE_REG, pRscConfig->RSC_MVO_STRIDE);
	RSC_WR32(RSC_BVO_BASE_ADDR_REG, pRscConfig->RSC_BVO_BASE_ADDR);
	RSC_WR32(RSC_BVO_STRIDE_REG, pRscConfig->RSC_BVO_STRIDE);

	RSC_WR32(RSC_START_REG, 0x1);	/* RSC Interrupt read-clear mode */

#ifdef __RSC_KERNEL_PERFORMANCE_MEASURE__
	mt_kernel_trace_end();
#endif

#endif
	return 0;
}
#else
{
	return 0;
}
#endif

#define RSC_IS_BUSY    0x2

#ifndef RSC_USE_GCE

static bool Check_RSC_Is_Busy(void)
#if !BYPASS_REG
{
	unsigned int Ctrl _Fsm;
	unsigned int Rsc_Start;

	Ctrl_Fsm = RSC_RD32(RSC_DBG_INFO_00_REG);
	Rsc_Start = RSC_RD32(RSC_START_REG);
	if ((RSC_IS_BUSY == (Ctrl_Fsm & RSC_IS_BUSY)) ||
		(RSC_START == (Rsc_Start & RSC_START)))
		return MTRUE;

	return MFALSE;
}
#else
{
	return MFLASE;
}
#endif
#endif


/*
 *
 */
static signed int RSC_DumpReg(void)
#if BYPASS_REG
{
	return 0;
}
#else
{
	signed int Ret = 0;
	unsigned int i, j;
	/*  */
	LOG_INF("- E.");
	/*  */
	LOG_INF("RSC Config Info\n");
	/* RSC Config0 */
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(RSC_CTRL_HW),
		(unsigned int)RSC_RD32(RSC_CTRL_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(RSC_SIZE_HW),
		(unsigned int)RSC_RD32(RSC_SIZE_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(RSC_IMGI_C_BASE_ADDR_HW),
		(unsigned int)RSC_RD32(RSC_IMGI_C_BASE_ADDR_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(RSC_IMGI_C_STRIDE_HW),
		(unsigned int)RSC_RD32(RSC_IMGI_C_STRIDE_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(RSC_IMGI_P_BASE_ADDR_HW),
		(unsigned int)RSC_RD32(RSC_IMGI_P_BASE_ADDR_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(RSC_IMGI_P_STRIDE_HW),
		(unsigned int)RSC_RD32(RSC_IMGI_P_STRIDE_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(RSC_MVI_BASE_ADDR_HW),
		(unsigned int)RSC_RD32(RSC_MVI_BASE_ADDR_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(RSC_MVI_STRIDE_HW),
		(unsigned int)RSC_RD32(RSC_MVI_STRIDE_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(RSC_APLI_C_BASE_ADDR_HW),
		(unsigned int)RSC_RD32(RSC_APLI_C_BASE_ADDR_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(RSC_APLI_P_BASE_ADDR_HW),
		(unsigned int)RSC_RD32(RSC_APLI_P_BASE_ADDR_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(RSC_MVO_BASE_ADDR_HW),
		(unsigned int)RSC_RD32(RSC_MVO_BASE_ADDR_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(RSC_MVO_STRIDE_HW),
		(unsigned int)RSC_RD32(RSC_MVO_STRIDE_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(RSC_BVO_BASE_ADDR_HW),
		(unsigned int)RSC_RD32(RSC_BVO_BASE_ADDR_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(RSC_BVO_STRIDE_HW),
		(unsigned int)RSC_RD32(RSC_BVO_STRIDE_REG));




	LOG_INF("RSC Debug Info\n");
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(RSC_DBG_INFO_00_HW),
		(unsigned int)RSC_RD32(RSC_DBG_INFO_00_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(RSC_DBG_INFO_01_HW),
		(unsigned int)RSC_RD32(RSC_DBG_INFO_01_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(RSC_DBG_INFO_02_HW),
		(unsigned int)RSC_RD32(RSC_DBG_INFO_02_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(RSC_DBG_INFO_03_HW),
		(unsigned int)RSC_RD32(RSC_DBG_INFO_03_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(RSC_DBG_INFO_04_HW),
		(unsigned int)RSC_RD32(RSC_DBG_INFO_04_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(RSC_DBG_INFO_05_HW),
		(unsigned int)RSC_RD32(RSC_DBG_INFO_05_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(RSC_DBG_INFO_06_HW),
		(unsigned int)RSC_RD32(RSC_DBG_INFO_06_REG));

	LOG_INF("[0x%08X %08X]\n", (unsigned int)(RSC_DMA_DBG_HW),
		(unsigned int)RSC_RD32(RSC_DMA_DBG_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(RSC_DMA_REQ_STATUS_HW),
		(unsigned int)RSC_RD32(RSC_DMA_REQ_STATUS_REG));
	LOG_INF("[0x%08X %08X]\n", (unsigned int)(RSC_DMA_RDY_STATUS_HW),
		(unsigned int)RSC_RD32(RSC_DMA_RDY_STATUS_REG));


	LOG_INF("RSC:HWProcessIdx:%d, WriteIdx:%d, ReadIdx:%d\n",
		g_RSC_ReqRing.HWProcessIdx,
		g_RSC_ReqRing.WriteIdx, g_RSC_ReqRing.ReadIdx);

	for (i = 0; i < _SUPPORT_MAX_RSC_REQUEST_RING_SIZE_; i++) {
#define TMP_STR1 "RSC Req:State:%d, procID:0x%08X, callerID:0x%08X,"
#define TMP_STR2 " enqueReqNum:%d, FrameWRIdx:%d, RrameRDIdx:%d\n"
		LOG_INF(TMP_STR1 TMP_STR2,
		     g_RSC_ReqRing.RSCReq_Struct[i].State,
		     g_RSC_ReqRing.RSCReq_Struct[i].processID,
		     g_RSC_ReqRing.RSCReq_Struct[i].callerID,
		     g_RSC_ReqRing.RSCReq_Struct[i].enqueReqNum,
		     g_RSC_ReqRing.RSCReq_Struct[i].FrameWRIdx,
		     g_RSC_ReqRing.RSCReq_Struct[i].RrameRDIdx);
#undef TMP_STR1
#undef TMP_STR2

		for (j = 0; j < _SUPPORT_MAX_RSC_FRAME_REQUEST_;) {
#define TMP_STR1 "RSC:FrameStatus[%d]:%d, FrameStatus[%d]:%d,"
#define TMP_STR2 " FrameStatus[%d]:%d, FrameStatus[%d]:%d\n"
			LOG_INF(TMP_STR1 TMP_STR2,
				j,
			g_RSC_ReqRing.RSCReq_Struct[i].RscFrameStatus[j],
				j + 1,
			g_RSC_ReqRing.RSCReq_Struct[i].RscFrameStatus[j + 1],
				j + 2,
			g_RSC_ReqRing.RSCReq_Struct[i].RscFrameStatus[j + 2],
				j + 3,
			g_RSC_ReqRing.RSCReq_Struct[i].RscFrameStatus[j + 3]);
#undef TMP_STR1
#undef TMP_STR2
			j = j + 4;
		}

	}



	LOG_INF("- X.");
	/*  */
	return Ret;
}
#endif
#if !defined(CONFIG_MTK_LEGACY) && defined(CONFIG_COMMON_CLK) /*CCF*/

static inline void RSC_Prepare_Enable_ccf_clock(void)
{
	int ret;
	/* must keep this clk open order:
	 * CG_SCP_SYS_MM0-> CG_MM_SMI_COMMON -> CG_SCP_SYS_ISP -> RSC clk
	 */
#ifdef SMI_CLK
	smi_bus_enable(SMI_LARB_IMGSYS1, "camera_rsc");
#else
	ret = clk_prepare_enable(rsc_clk.CG_SCP_SYS_MM0);
	if (ret)
		LOG_ERR("cannot prepare and enable CG_SCP_SYS_MM0 clock\n");


	ret = clk_prepare_enable(rsc_clk.CG_MM_SMI_COMMON);
	if (ret)
		LOG_ERR("cannot prepare and enable CG_MM_SMI_COMMON clock\n");

	ret = clk_prepare_enable(rsc_clk.CG_MM_SMI_COMMON_2X);
	if (ret)
		LOG_ERR("cannot prepare and enable CG_MM_SMI_COMMON_2X clk\n");

	ret = clk_prepare_enable(rsc_clk.CG_MM_SMI_COMMON_GALS_M0_2X);
	if (ret)
		LOG_ERR("prepare enable CG_MM_SMI_COMMON_GALS_M0_2X error\n");

	ret = clk_prepare_enable(rsc_clk.CG_MM_SMI_COMMON_GALS_M1_2X);
	if (ret)
		LOG_ERR("prepare enable CG_MM_SMI_COMMON_GALS_M1_2X error\n");

	ret = clk_prepare_enable(rsc_clk.CG_MM_SMI_COMMON_UPSZ0);
	if (ret)
		LOG_ERR("clk_prepare_enable CG_MM_SMI_COMMON_UPSZ0 error\n");

	ret = clk_prepare_enable(rsc_clk.CG_MM_SMI_COMMON_UPSZ1);
	if (ret)
		LOG_ERR("clk_prepare_enable CG_MM_SMI_COMMON_UPSZ1 error\n");

	ret = clk_prepare_enable(rsc_clk.CG_MM_SMI_COMMON_FIFO0);
	if (ret)
		LOG_ERR("clk_prepare_enable CG_MM_SMI_COMMON_FIFO0 error\n");

	ret = clk_prepare_enable(rsc_clk.CG_MM_SMI_COMMON_FIFO1);
	if (ret)
		LOG_ERR("clk_prepare_enable CG_MM_SMI_COMMON_FIFO1 error\n");

	ret = clk_prepare_enable(rsc_clk.CG_MM_LARB5);
	if (ret)
		LOG_ERR("cannot prepare and enable CG_MM_LARB5 clock\n");

	ret = clk_prepare_enable(rsc_clk.CG_SCP_SYS_ISP);
	if (ret)
		LOG_ERR("cannot prepare and enable CG_SCP_SYS_ISP clock\n");

	ret = clk_prepare_enable(rsc_clk.CG_IMGSYS_LARB);
	if (ret)
		LOG_ERR("cannot prepare and enable CG_IMGSYS_LARB clock\n");
#endif
	ret = clk_prepare_enable(rsc_clk.CG_IMGSYS_RSC);
	if (ret)
		LOG_ERR("cannot prepare and enable CG_IMGSYS_RSC clock\n");

}

static inline void RSC_Disable_Unprepare_ccf_clock(void)
{
	/* must keep this clk close order:
	 * RSC clk -> CG_SCP_SYS_ISP -> CG_MM_SMI_COMMON -> CG_SCP_SYS_MM0
	 */
	clk_disable_unprepare(rsc_clk.CG_IMGSYS_RSC);
#ifdef SMI_CLK
	smi_bus_disable(SMI_LARB_IMGSYS1, "camera_rsc");
#else
	clk_disable_unprepare(rsc_clk.CG_IMGSYS_LARB);
	clk_disable_unprepare(rsc_clk.CG_SCP_SYS_ISP);
	clk_disable_unprepare(rsc_clk.CG_MM_LARB5);
	clk_disable_unprepare(rsc_clk.CG_MM_SMI_COMMON_FIFO1);
	clk_disable_unprepare(rsc_clk.CG_MM_SMI_COMMON_FIFO0);
	clk_disable_unprepare(rsc_clk.CG_MM_SMI_COMMON_UPSZ1);
	clk_disable_unprepare(rsc_clk.CG_MM_SMI_COMMON_UPSZ0);
	clk_disable_unprepare(rsc_clk.CG_MM_SMI_COMMON_GALS_M1_2X);
	clk_disable_unprepare(rsc_clk.CG_MM_SMI_COMMON_GALS_M0_2X);
	clk_disable_unprepare(rsc_clk.CG_MM_SMI_COMMON_2X);
	clk_disable_unprepare(rsc_clk.CG_MM_SMI_COMMON);
	clk_disable_unprepare(rsc_clk.CG_SCP_SYS_MM0);
#endif
}
#endif

/*******************************************************************************
 *
 ******************************************************************************/
static void RSC_EnableClock(bool En)
{
#if defined(EP_NO_CLKMGR)
	unsigned int setReg;
#endif

	if (En) {		/* Enable clock. */
		switch (g_u4EnableClockCount) {
		case 0:
#if !defined(CONFIG_MTK_LEGACY) && defined(CONFIG_COMMON_CLK) /*CCF*/
#ifndef EP_NO_CLKMGR
			RSC_Prepare_Enable_ccf_clock();
#else
			/* Enable clock by hardcode:
			 * 1. CAMSYS_CG_CLR (0x1A000008) = 0xffffffff;
			 * 2. IMG_CG_CLR (0x15000008) = 0xffffffff;
			 */
			setReg = 0xFFFFFFFF;
			RSC_WR32(IMGSYS_REG_CG_CLR, setReg);
#endif
#else
			enable_clock(MT_CG_DRSC0_SMI_COMMON, "CAMERA");
			enable_clock(MT_CG_IMAGE_CAM_SMI, "CAMERA");
			enable_clock(MT_CG_IMAGE_CAM_CAM, "CAMERA");
			enable_clock(MT_CG_IMAGE_SEN_TG, "CAMERA");
			enable_clock(MT_CG_IMAGE_SEN_CAM, "CAMERA");
			enable_clock(MT_CG_IMAGE_CAM_SV, "CAMERA");
			/* enable_clock(MT_CG_IMAGE_FD, "CAMERA"); */
			enable_clock(MT_CG_IMAGE_LARB2_SMI, "CAMERA");
#endif	/* #if !defined(CONFIG_MTK_LEGACY) && defined(CONFIG_COMMON_CLK)  */
			break;
		default:
			break;
		}
		spin_lock(&(RSCInfo.SpinLockRSC));
		g_u4EnableClockCount++;
		spin_unlock(&(RSCInfo.SpinLockRSC));
	} else {		/* Disable clock. */

		spin_lock(&(RSCInfo.SpinLockRSC));
		g_u4EnableClockCount--;
		spin_unlock(&(RSCInfo.SpinLockRSC));
		switch (g_u4EnableClockCount) {
		case 0:
#if !defined(CONFIG_MTK_LEGACY) && defined(CONFIG_COMMON_CLK) /*CCF*/
#ifndef EP_NO_CLKMGR
			RSC_Disable_Unprepare_ccf_clock();
#else
			/* Disable clock by hardcode:
			 *  1. CAMSYS_CG_SET (0x1A000004) = 0xffffffff;
			 *  2. IMG_CG_SET (0x15000004) = 0xffffffff;
			 */
			setReg = 0xFFFFFFFF;
			RSC_WR32(IMGSYS_REG_CG_SET, setReg);
#endif
#else
			/* do disable clock */
			disable_clock(MT_CG_IMAGE_CAM_SMI, "CAMERA");
			disable_clock(MT_CG_IMAGE_CAM_CAM, "CAMERA");
			disable_clock(MT_CG_IMAGE_SEN_TG, "CAMERA");
			disable_clock(MT_CG_IMAGE_SEN_CAM, "CAMERA");
			disable_clock(MT_CG_IMAGE_CAM_SV, "CAMERA");
			/* disable_clock(MT_CG_IMAGE_FD, "CAMERA"); */
			disable_clock(MT_CG_IMAGE_LARB2_SMI, "CAMERA");
			disable_clock(MT_CG_DRSC0_SMI_COMMON, "CAMERA");
#endif	/* #if !defined(CONFIG_MTK_LEGACY) && defined(CONFIG_COMMON_CLK) */
			break;
		default:
			break;
		}
	}
}

/*******************************************************************************
 *
 ******************************************************************************/
static inline void RSC_Reset(void)
{
	LOG_DBG("- E.");

	LOG_DBG(" RSC Reset start!\n");
	spin_lock(&(RSCInfo.SpinLockRSCRef));

	if (RSCInfo.UserCount > 1) {
		spin_unlock(&(RSCInfo.SpinLockRSCRef));
		LOG_DBG("Curr UserCount(%d) users exist", RSCInfo.UserCount);
	} else {
		spin_unlock(&(RSCInfo.SpinLockRSCRef));

		/* Reset RSC flow */
		RSC_WR32(RSC_RST_REG, 0x1);
		while ((RSC_RD32(RSC_RST_REG) & 0x02) != 0x2)
			LOG_DBG("RSC resetting...\n");

		RSC_WR32(RSC_RST_REG, 0x11);
		RSC_WR32(RSC_RST_REG, 0x10);
		RSC_WR32(RSC_RST_REG, 0x0);
		RSC_WR32(RSC_START_REG, 0);
		LOG_DBG(" RSC Reset end!\n");
	}

}

/*******************************************************************************
 *
 ******************************************************************************/
static signed int RSC_ReadReg(struct RSC_REG_IO_STRUCT *pRegIo)
{
	unsigned int i;
	signed int Ret = 0;
	/*  */
	struct RSC_REG_STRUCT reg;
	/* unsigned int* pData = (unsigned int*)pRegIo->Data; */
	struct RSC_REG_STRUCT *pData = (struct RSC_REG_STRUCT *) pRegIo->pData;

	for (i = 0; i < pRegIo->Count; i++) {
		if (get_user(reg.Addr, (unsigned int *) &pData->Addr) != 0) {
			LOG_ERR("get_user failed");
			Ret = -EFAULT;
			goto EXIT;
		}
		/* pData++; */
		/*  */
		if ((ISP_RSC_BASE + reg.Addr >= ISP_RSC_BASE) &&
			(ISP_RSC_BASE + reg.Addr <
			(ISP_RSC_BASE + RSC_REG_RANGE))) {
			reg.Val = RSC_RD32(ISP_RSC_BASE + reg.Addr);
		} else {
			LOG_ERR("Wrong address(%p)",
				(ISP_RSC_BASE + reg.Addr));
			reg.Val = 0;
		}

		if (put_user(reg.Val, (unsigned int *) &(pData->Val)) != 0) {
			LOG_ERR("put_user failed");
			Ret = -EFAULT;
			goto EXIT;
		}
		pData++;
		/*  */
	}
	/*  */
EXIT:
	return Ret;
}


/*******************************************************************************
 *
 ******************************************************************************/
/* Can write sensor's test model only, if need to write to other modules, need
 * modify current code flow.
 */
static signed int RSC_WriteRegToHw(struct RSC_REG_STRUCT *pReg,
	unsigned int Count)
{
	signed int Ret = 0;
	unsigned int i;
	bool dbgWriteReg;

	/* Use local variable to store RSCInfo.DebugMask & RSC_DBG_WRITE_REG
	 * for saving lock time.
	 */
	spin_lock(&(RSCInfo.SpinLockRSC));
	dbgWriteReg = RSCInfo.DebugMask & RSC_DBG_WRITE_REG;
	spin_unlock(&(RSCInfo.SpinLockRSC));

	/*  */
	if (dbgWriteReg)
		LOG_DBG("- E.\n");

	/*  */
	for (i = 0; i < Count; i++) {
		if (dbgWriteReg) {
			LOG_DBG("Addr(0x%lx), Val(0x%x)\n",
				(unsigned long)(ISP_RSC_BASE + pReg[i].Addr),
				(unsigned int) (pReg[i].Val));
		}

		if (((ISP_RSC_BASE + pReg[i].Addr) <
			(ISP_RSC_BASE + RSC_REG_RANGE))) {
			RSC_WR32(ISP_RSC_BASE + pReg[i].Addr, pReg[i].Val);
		} else {
			LOG_ERR("wrong address(0x%lx)\n",
				(unsigned long)(ISP_RSC_BASE + pReg[i].Addr));
		}
	}

	/*  */
	return Ret;
}



/*******************************************************************************
 *
 ******************************************************************************/
static signed int RSC_WriteReg(struct RSC_REG_IO_STRUCT *pRegIo)
{
	signed int Ret = 0;
	/*
	 *  signed int TimeVd = 0;
	 *  signed int TimeExpdone = 0;
	 *  signed int TimeTasklet = 0;
	 */
	/* unsigned char* pData = NULL; */
	struct RSC_REG_STRUCT *pData = NULL;
	/*  */
	if (RSCInfo.DebugMask & RSC_DBG_WRITE_REG)
		LOG_DBG("Data(0x%p), Count(%d)\n", (pRegIo->pData),
			(pRegIo->Count));

	pData = kmalloc((pRegIo->Count) * sizeof(struct RSC_REG_STRUCT),
		GFP_ATOMIC);
	if (pData == NULL) {
		LOG_DBG("ERROR: kmalloc fail, (proc, pid, tgid)=(%s, %d, %d)\n",
			current->comm, current->pid, current->tgid);
		Ret = -ENOMEM;
		goto EXIT;
	}
	/*  */
	if ((pRegIo->pData == NULL) || (pRegIo->Count == 0)) {
		LOG_ERR("ERROR: pRegIo->pData is NULL or Count:%d\n",
			pRegIo->Count);
		Ret = -EFAULT;
		goto EXIT;
	}
	if (copy_from_user(pData, (void __user *)(pRegIo->pData),
		pRegIo->Count * sizeof(struct RSC_REG_STRUCT)) != 0) {
		LOG_ERR("copy_from_user failed\n");
		Ret = -EFAULT;
		goto EXIT;
	}
	/*  */
	Ret = RSC_WriteRegToHw(pData, pRegIo->Count);
	/*  */
EXIT:
	if (pData != NULL) {
		kfree(pData);
		pData = NULL;
	}
	return Ret;
}


/*******************************************************************************
 *
 ******************************************************************************/
static signed int RSC_WaitIrq(struct RSC_WAIT_IRQ_STRUCT *WaitIrq)
{

	signed int Ret = 0;
	signed int Timeout = WaitIrq->Timeout;
	enum RSC_PROCESS_ID_ENUM whichReq = RSC_PROCESS_ID_NONE;

	/*unsigned int i;*/
	unsigned long flags; /* old: unsigned int flags;*/
	unsigned int irqStatus;
	/*int cnt = 0;*/
	struct timeval time_getrequest;
	unsigned long long sec = 0;
	unsigned long usec = 0;

	/* do_gettimeofday(&time_getrequest); */
	sec = cpu_clock(0);	/* ns */
	do_div(sec, 1000);	/* usec */
	usec = do_div(sec, 1000000);	/* sec and usec */
	time_getrequest.tv_usec = usec;
	time_getrequest.tv_sec = sec;


	/* Debug interrupt */
	if (RSCInfo.DebugMask & RSC_DBG_INT) {
		if (WaitIrq->Status & RSCInfo.IrqInfo.Mask[WaitIrq->Type]) {
			if (WaitIrq->UserKey > 0) {
#define TMP_STR1 "+WaitIrq clr(%d), Type(%d), Stat(0x%08X), Timeout(%d),"
#define TMP_STR2 " usr(%d), ProcID(%d)\n"
				LOG_DBG(TMP_STR1 TMP_STR2,
					WaitIrq->Clear, WaitIrq->Type,
					WaitIrq->Status, WaitIrq->Timeout,
					WaitIrq->UserKey, WaitIrq->ProcessID);
#undef TMP_STR1
#undef TMP_STR2
			}
		}
	}


	/* 1. wait type update */
	if (WaitIrq->Clear == RSC_IRQ_CLEAR_STATUS) {
		spin_lock_irqsave(&(RSCInfo.SpinLockIrq[WaitIrq->Type]), flags);

		RSCInfo.IrqInfo.Status[WaitIrq->Type] &= (~WaitIrq->Status);
		spin_unlock_irqrestore(&(RSCInfo.SpinLockIrq[WaitIrq->Type]),
			flags);
		return Ret;
	}

	if (WaitIrq->Clear == RSC_IRQ_CLEAR_WAIT) {
		spin_lock_irqsave(&(RSCInfo.SpinLockIrq[WaitIrq->Type]), flags);
		if (RSCInfo.IrqInfo.Status[WaitIrq->Type] & WaitIrq->Status)
			RSCInfo.IrqInfo.Status[WaitIrq->Type] &=
				(~WaitIrq->Status);

		spin_unlock_irqrestore(&(RSCInfo.SpinLockIrq[WaitIrq->Type]),
			flags);
	} else if (WaitIrq->Clear == RSC_IRQ_CLEAR_ALL) {
		spin_lock_irqsave(&(RSCInfo.SpinLockIrq[WaitIrq->Type]), flags);

		RSCInfo.IrqInfo.Status[WaitIrq->Type] = 0;
		spin_unlock_irqrestore(&(RSCInfo.SpinLockIrq[WaitIrq->Type]),
			flags);
	}
	/* RSC_IRQ_WAIT_CLEAR ==> do nothing */


	/* Store irqinfo status in here to redeuce time of spin_lock_irqsave */
	spin_lock_irqsave(&(RSCInfo.SpinLockIrq[WaitIrq->Type]), flags);
	irqStatus = RSCInfo.IrqInfo.Status[WaitIrq->Type];
	spin_unlock_irqrestore(&(RSCInfo.SpinLockIrq[WaitIrq->Type]), flags);

	if (WaitIrq->Status & RSC_INT_ST) {
		whichReq = RSC_PROCESS_ID_RSC;
	} else {
#define ERR_STR1 "No Such Stats can be waited!!"
#define ERR_STR2 " irq Type/User/Sts/Pid(0x%x/%d/0x%x/%d)\n"
		LOG_ERR(ERR_STR1 ERR_STR2,
			WaitIrq->Type, WaitIrq->UserKey, WaitIrq->Status,
			WaitIrq->ProcessID);
#undef ERR_STR1
#undef ERR_STR2
	}


#ifdef RSC_WAITIRQ_LOG
#define TMP_STR1 "before wait_event:Tout(%d), Clear(%d), Type(%d),"
#define TMP_STR2 " IrqStat(0x%08X), WaitStat(0x%08X), usrKey(%d)\n"
	LOG_INF(TMP_STR1 TMP_STR2,
		WaitIrq->Timeout, WaitIrq->Clear, WaitIrq->Type, irqStatus,
		WaitIrq->Status, WaitIrq->UserKey);
#undef TMP_STR1
#undef TMP_STR2
#define TMP_STR1 "before wait_event:ProcID(%d), RscIrq(0x%08X),"
#define TMP_STR2 " WriteReq(0x%08X), ReadReq(0x%08X), whichReq(%d)\n"
	LOG_INF(TMP_STR1 TMP_STR2,
		WaitIrq->ProcessID, RSCInfo.IrqInfo.RscIrqCnt,
		RSCInfo.WriteReqIdx, RSCInfo.ReadReqIdx, whichReq);
#undef TMP_STR1
#undef TMP_STR2
#endif

	/* 2. start to wait signal */
	Timeout = wait_event_interruptible_timeout(RSCInfo.WaitQueueHead,
		RSC_GetIRQState(WaitIrq->Type, WaitIrq->UserKey,
		WaitIrq->Status, whichReq, WaitIrq->ProcessID),
		RSC_MsToJiffies(WaitIrq->Timeout));

	/* check if user is interrupted by system signal */
	if ((Timeout != 0) &&
		(!RSC_GetIRQState(WaitIrq->Type, WaitIrq->UserKey,
		WaitIrq->Status, whichReq, WaitIrq->ProcessID))) {
#define TMP_STR1 "interrupted by system, timeout(%d),"
#define TMP_STR2 "irq Type/User/Sts/whichReq/Pid(0x%x/%d/0x%x/%d/%d)\n"

		LOG_DBG(TMP_STR1 TMP_STR2,
			Timeout, WaitIrq->Type, WaitIrq->UserKey,
			WaitIrq->Status, whichReq, WaitIrq->ProcessID);
#undef TMP_STR1
#undef TMP_STR2
		Ret = -ERESTARTSYS;	/* actually it should be -ERESTARTSYS */
		goto EXIT;
	}
	/* timeout */
	if (Timeout == 0) {
		/* Store irqinfo status in here to redeuce time of
		 * spin_lock_irqsave.
		 */
		spin_lock_irqsave(&(RSCInfo.SpinLockIrq[WaitIrq->Type]), flags);
		irqStatus = RSCInfo.IrqInfo.Status[WaitIrq->Type];
		spin_unlock_irqrestore(&(RSCInfo.SpinLockIrq[WaitIrq->Type]),
			flags);
#define ERR_STR1 "WaitIrq Timeout:Tout(%d) clr(%d) Type(%d) IrqStat(0x%08X)"
#define ERR_STR2 " WaitStat(0x%08X) usrKey(%d)\n"
		LOG_ERR(ERR_STR1 ERR_STR2,
		     WaitIrq->Timeout, WaitIrq->Clear, WaitIrq->Type, irqStatus,
		     WaitIrq->Status, WaitIrq->UserKey);
#undef ERR_STR1
#undef ERR_STR2

#define ERR_STR1 "WaitIrq Timeout:whichReq(%d),ProcID(%d) RscIrqCnt(0x%08X)"
#define ERR_STR2 " WriteReq(0x%08X) ReadReq(0x%08X)\n"

		LOG_ERR(ERR_STR1 ERR_STR2,
		     whichReq, WaitIrq->ProcessID, RSCInfo.IrqInfo.RscIrqCnt,
		     RSCInfo.WriteReqIdx, RSCInfo.ReadReqIdx);
#undef ERR_STR1
#undef ERR_STR2
		if (WaitIrq->bDumpReg)
			RSC_DumpReg();

		Ret = -EFAULT;
		goto EXIT;
	} else {
		/* Store irqinfo status in here to redeuce time of
		 * spin_lock_irqsave.
		 */
#ifdef __RSC_KERNEL_PERFORMANCE_MEASURE__
		/* mt_kernel_trace_begin("RSC_WaitIrq");*/
		mt_kernel_trace_begin(__func__);
#endif

		spin_lock_irqsave(&(RSCInfo.SpinLockIrq[WaitIrq->Type]), flags);
		irqStatus = RSCInfo.IrqInfo.Status[WaitIrq->Type];
		spin_unlock_irqrestore(&(RSCInfo.SpinLockIrq[WaitIrq->Type]),
			flags);

		if (WaitIrq->Clear == RSC_IRQ_WAIT_CLEAR) {
			spin_lock_irqsave(&(RSCInfo.SpinLockIrq[WaitIrq->Type]),
				flags);
#ifdef RSC_USE_GCE

#ifdef RSC_MULTIPROCESS_TIMEING_ISSUE
			RSCInfo.ReadReqIdx = (RSCInfo.ReadReqIdx + 1) %
				_SUPPORT_MAX_RSC_FRAME_REQUEST_;
			/* actually, it doesn't happen the timging issue!! */
			/* wake_up_interruptible(&RSCInfo.WaitQueueHead); */
#endif
			if (WaitIrq->Status & RSC_INT_ST) {
				RSCInfo.IrqInfo.RscIrqCnt--;
				if (RSCInfo.IrqInfo.RscIrqCnt == 0)
					RSCInfo.IrqInfo.Status[WaitIrq->Type] &=
						(~WaitIrq->Status);
			} else {
#define ERR_STR "RSC_IRQ_WAIT_CLEAR Error, Type(%d), WaitStatus(0x%08X)"
				LOG_ERR(ERR_STR,
					WaitIrq->Type, WaitIrq->Status);
#undef ERR_STR
			}
#else
			if (RSCInfo.IrqInfo.Status[WaitIrq->Type] &
				WaitIrq->Status)
				RSCInfo.IrqInfo.Status[WaitIrq->Type] &=
					(~WaitIrq->Status);
#endif
			spin_unlock_irqrestore(
				&(RSCInfo.SpinLockIrq[WaitIrq->Type]), flags);
		}

#ifdef RSC_WAITIRQ_LOG
#define TMP_STR1 "no Timeout:Tout(%d), clr(%d), Type(%d), IrqStat(0x%08X),"
#define TMP_STR2 " WaitStat(0x%08X), usrKey(%d)\n"
		LOG_INF(TMP_STR1 TMP_STR2,
			WaitIrq->Timeout, WaitIrq->Clear, WaitIrq->Type,
			irqStatus, WaitIrq->Status, WaitIrq->UserKey);
#undef TMP_STR1
#undef TMP_STR2
#define TMP_STR1 "no Timeout:ProcID(%d),RscIrq(0x%08X), WriteReq(0x%08X),"
#define TMP_STR2 " ReadReq(0x%08X),whichReq(%d)\n"
		LOG_INF(TMP_STR1 TMP_STR2,
			WaitIrq->ProcessID, RSCInfo.IrqInfo.RscIrqCnt,
			RSCInfo.WriteReqIdx, RSCInfo.ReadReqIdx, whichReq);
#undef TMP_STR1
#undef TMP_STR2
#endif

#ifdef __RSC_KERNEL_PERFORMANCE_MEASURE__
		mt_kernel_trace_end();
#endif

	}


EXIT:


	return Ret;
}


/*******************************************************************************
 *
 ******************************************************************************/
static long RSC_ioctl(struct file *pFile, unsigned int Cmd, unsigned long Param)
{
	signed int Ret = 0;

	/*unsigned int pid = 0;*/
	struct RSC_REG_IO_STRUCT RegIo;
	struct RSC_WAIT_IRQ_STRUCT IrqInfo;
	struct RSC_CLEAR_IRQ_STRUCT ClearIrq;
	struct RSC_Config rsc_RscConfig;
	struct RSC_Request rsc_RscReq;
	signed int RscWriteIdx = 0;
	int idx;
	struct RSC_USER_INFO_STRUCT *pUserInfo;
	int enqueNum;
	int dequeNum;
	unsigned long flags; /* old: unsigned int flags;*/



	/*  */
	if (pFile->private_data == NULL) {
		LOG_WRN("private_data is NULL,(proc, pid, tgid)=(%s, %d, %d)",
			current->comm,
			current->pid, current->tgid);
		return -EFAULT;
	}
	/*  */
	pUserInfo = (struct RSC_USER_INFO_STRUCT *) (pFile->private_data);
	/*  */
	switch (Cmd) {
	case RSC_RESET:
	{
		spin_lock(&(RSCInfo.SpinLockRSC));
		RSC_Reset();
		spin_unlock(&(RSCInfo.SpinLockRSC));
		break;
	}

	/*  */
	case RSC_DUMP_REG:
	{
		Ret = RSC_DumpReg();
		break;
	}
	case RSC_DUMP_ISR_LOG:
	{
		unsigned int currentPPB = m_CurrentPPB;

		spin_lock_irqsave(
			&(RSCInfo.SpinLockIrq[RSC_IRQ_TYPE_INT_RSC_ST]), flags);
		m_CurrentPPB = (m_CurrentPPB + 1) % LOG_PPNUM;
		spin_unlock_irqrestore(
			&(RSCInfo.SpinLockIrq[RSC_IRQ_TYPE_INT_RSC_ST]), flags);

		IRQ_LOG_PRINTER(RSC_IRQ_TYPE_INT_RSC_ST, currentPPB, _LOG_INF);
		IRQ_LOG_PRINTER(RSC_IRQ_TYPE_INT_RSC_ST, currentPPB, _LOG_ERR);
		break;
	}
	case RSC_READ_REGISTER:
	{
		if (copy_from_user(&RegIo, (void *)Param,
			sizeof(struct RSC_REG_IO_STRUCT)) == 0) {
			/* 2nd layer behavoir of copy from user is implemented
			 * in RSC_ReadReg(...)
			 */
			Ret = RSC_ReadReg(&RegIo);
		} else {
			LOG_ERR("RSC_READ_REGISTER copy_from_user failed");
			Ret = -EFAULT;
		}
		break;
	}
	case RSC_WRITE_REGISTER:
	{
		if (copy_from_user(&RegIo, (void *)Param,
			sizeof(struct RSC_REG_IO_STRUCT)) == 0) {
			/* 2nd layer behavoir of copy from user is implemented
			 * in RSC_WriteReg(...)
			 */
			Ret = RSC_WriteReg(&RegIo);
		} else {
			LOG_ERR("RSC_WRITE_REGISTER copy_from_user failed");
			Ret = -EFAULT;
		}
		break;
	}
	case RSC_WAIT_IRQ:
	{
		if (copy_from_user(&IrqInfo, (void *)Param,
			sizeof(struct RSC_WAIT_IRQ_STRUCT)) == 0) {
			/*  */
			if ((IrqInfo.Type >= RSC_IRQ_TYPE_AMOUNT) ||
				(IrqInfo.Type < 0)) {
				Ret = -EFAULT;
				LOG_ERR("invalid type(%d)", IrqInfo.Type);
				goto EXIT;
			}

			if ((IrqInfo.UserKey >= IRQ_USER_NUM_MAX) ||
				(IrqInfo.UserKey < 0)) {
#define ERR_STR "invalid userKey(%d), max(%d), force userkey = 0\n"
				LOG_ERR(ERR_STR,
					IrqInfo.UserKey, IRQ_USER_NUM_MAX);
#undef ERR_STR
				IrqInfo.UserKey = 0;
			}
#define TMP_STR1 "IRQ clear(%d), type(%d), userKey(%d), timeout(%d),"
#define TMP_STR2 " status(%d)\n"
			LOG_INF(TMP_STR1 TMP_STR2,
			     IrqInfo.Clear, IrqInfo.Type, IrqInfo.UserKey,
			     IrqInfo.Timeout, IrqInfo.Status);
#undef TMP_STR1
#undef TMP_STR2
			IrqInfo.ProcessID = pUserInfo->Pid;
			Ret = RSC_WaitIrq(&IrqInfo);

			if (copy_to_user((void *)Param, &IrqInfo,
				sizeof(struct RSC_WAIT_IRQ_STRUCT)) != 0) {
				LOG_ERR("copy_to_user failed\n");
				Ret = -EFAULT;
			}
		} else {
			LOG_ERR("RSC_WAIT_IRQ copy_from_user failed");
			Ret = -EFAULT;
		}
		break;
	}
	case RSC_CLEAR_IRQ:
	{
		if (copy_from_user(&ClearIrq, (void *)Param,
			sizeof(struct RSC_CLEAR_IRQ_STRUCT)) == 0) {
			LOG_DBG("RSC_CLEAR_IRQ Type(%d)", ClearIrq.Type);

			if ((ClearIrq.Type >= RSC_IRQ_TYPE_AMOUNT) ||
				(ClearIrq.Type < 0)) {
				Ret = -EFAULT;
				LOG_ERR("invalid type(%d)", ClearIrq.Type);
				goto EXIT;
			}

			/*  */
			if ((ClearIrq.UserKey >= IRQ_USER_NUM_MAX)
			    || (ClearIrq.UserKey < 0)) {
				LOG_ERR("errUserEnum(%d)", ClearIrq.UserKey);
				Ret = -EFAULT;
				goto EXIT;
			}

#define TMP_STR "RSC_CLEAR_IRQ:Type(%d),Status(0x%08X),IrqStatus(0x%08X)\n"
			LOG_DBG(TMP_STR,
				ClearIrq.Type, ClearIrq.Status,
				RSCInfo.IrqInfo.Status[ClearIrq.Type]);
#undef TMP_STR
			spin_lock_irqsave(&(RSCInfo.SpinLockIrq[ClearIrq.Type]),
				flags);
			RSCInfo.IrqInfo.Status[ClearIrq.Type] &=
				(~ClearIrq.Status);
			spin_unlock_irqrestore(
				&(RSCInfo.SpinLockIrq[ClearIrq.Type]), flags);
		} else {
			LOG_ERR("RSC_CLEAR_IRQ copy_from_user failed\n");
			Ret = -EFAULT;
		}
		break;
	}
	case RSC_ENQNUE_NUM:
	{
		/* enqueNum */
		if (copy_from_user(&enqueNum,
			(void *)Param, sizeof(int)) == 0) {
			if (RSC_REQUEST_STATE_EMPTY ==
			    g_RSC_ReqRing.RSCReq_Struct[g_RSC_ReqRing.WriteIdx].
			    State) {
				spin_lock_irqsave(&(RSCInfo.SpinLockIrq[
					RSC_IRQ_TYPE_INT_RSC_ST]), flags);

				g_RSC_ReqRing.RSCReq_Struct[
					g_RSC_ReqRing.WriteIdx].processID =
					pUserInfo->Pid;

				g_RSC_ReqRing.RSCReq_Struct[
					g_RSC_ReqRing.WriteIdx].enqueReqNum =
					enqueNum;

				spin_unlock_irqrestore(&(RSCInfo.SpinLockIrq[
					RSC_IRQ_TYPE_INT_RSC_ST]), flags);

				if (enqueNum >
					_SUPPORT_MAX_RSC_FRAME_REQUEST_) {
#define ERR_STR "RSC Enque Num is bigger than enqueNum:%d\n"
					LOG_ERR(ERR_STR, enqueNum);
#undef ERR_STR
				}
				LOG_DBG("RSC_ENQNUE_NUM:%d\n", enqueNum);
			} else {
#define ERR_STR1 "WFME Enque request state is not empty:%d, writeIdx:%d,"
#define ERR_STR2 " readIdx:%d\n"
				LOG_ERR(ERR_STR1 ERR_STR2,
					g_RSC_ReqRing.RSCReq_Struct[
						g_RSC_ReqRing.WriteIdx].State,
					g_RSC_ReqRing.WriteIdx,
					g_RSC_ReqRing.ReadIdx);
#undef ERR_STR1
#undef ERR_STR2
			}
		} else {
			LOG_ERR("RSC_EQNUE_NUM copy_from_user failed\n");
			Ret = -EFAULT;
		}
			break;
	}
	/* struct RSC_Config */
	case RSC_ENQUE:
	{
		if (copy_from_user(&rsc_RscConfig, (void *)Param,
			sizeof(struct RSC_Config)) == 0) {

			spin_lock_irqsave(
				&(RSCInfo.SpinLockIrq[RSC_IRQ_TYPE_INT_RSC_ST]),
				flags);
			if ((g_RSC_ReqRing.RSCReq_Struct[
				g_RSC_ReqRing.WriteIdx].State ==
				RSC_REQUEST_STATE_EMPTY) &&
				(g_RSC_ReqRing.RSCReq_Struct[
				g_RSC_ReqRing.WriteIdx].FrameWRIdx <
				g_RSC_ReqRing.RSCReq_Struct[
				g_RSC_ReqRing.WriteIdx].enqueReqNum)) {

				g_RSC_ReqRing.RSCReq_Struct[
					g_RSC_ReqRing.WriteIdx].RscFrameStatus[
					g_RSC_ReqRing.RSCReq_Struct[
					g_RSC_ReqRing.WriteIdx].FrameWRIdx] =
					RSC_FRAME_STATUS_ENQUE;

				memcpy(&g_RSC_ReqRing.RSCReq_Struct[
					g_RSC_ReqRing.WriteIdx].RscFrameConfig[
					g_RSC_ReqRing.RSCReq_Struct[
					g_RSC_ReqRing.WriteIdx].FrameWRIdx++],
					&rsc_RscConfig,
					sizeof(struct RSC_Config));

				if (g_RSC_ReqRing.RSCReq_Struct[
					g_RSC_ReqRing.WriteIdx].FrameWRIdx ==
					g_RSC_ReqRing.RSCReq_Struct[
					g_RSC_ReqRing.WriteIdx].enqueReqNum) {

					g_RSC_ReqRing.RSCReq_Struct[
						g_RSC_ReqRing.WriteIdx].State =
						RSC_REQUEST_STATE_PENDING;

					g_RSC_ReqRing.WriteIdx =
						(g_RSC_ReqRing.WriteIdx + 1) %
					_SUPPORT_MAX_RSC_REQUEST_RING_SIZE_;

					LOG_DBG("RSC enque done!!\n");
				} else {
					LOG_DBG("RSC enque frame!!\n");
				}
			} else {
#define ERR_STR1 "No Buffer! WriteIdx(%d), Stat(%d), FrameWRIdx(%d),"
#define ERR_STR2 " enqueReqNum(%d)\n"
				LOG_ERR(ERR_STR1 ERR_STR2,
					g_RSC_ReqRing.WriteIdx,
					g_RSC_ReqRing.RSCReq_Struct[
						g_RSC_ReqRing.WriteIdx].State,
					g_RSC_ReqRing.RSCReq_Struct[
						g_RSC_ReqRing.WriteIdx].
						FrameWRIdx,
					g_RSC_ReqRing.RSCReq_Struct[
						g_RSC_ReqRing.WriteIdx].
						enqueReqNum);
			}
#ifdef RSC_USE_GCE
			spin_unlock_irqrestore(
				&(RSCInfo.SpinLockIrq[RSC_IRQ_TYPE_INT_RSC_ST]),
				flags);
			LOG_DBG("ConfigRSC!!\n");
			ConfigRSC();
#else
			/* check the hw is running or not ? */
			if (Check_RSC_Is_Busy() == MFALSE) {
				/* config the rsc hw and run */
				LOG_DBG("ConfigRSC\n");
				ConfigRSC();
			} else {
				LOG_INF("RSC HW is busy!!\n");
			}
			spin_unlock_irqrestore(
				&(RSCInfo.SpinLockIrq[RSC_IRQ_TYPE_INT_RSC_ST]),
				flags);
#endif


		} else {
			LOG_ERR("RSC_ENQUE copy_from_user failed\n");
			Ret = -EFAULT;
		}

		break;
	}
	case RSC_ENQUE_REQ:
	{
		if (copy_from_user(&rsc_RscReq, (void *)Param,
			sizeof(struct RSC_Request)) == 0) {
			LOG_DBG("RSC_ENQNUE_NUM:%d, pid:%d\n",
				rsc_RscReq.m_ReqNum, pUserInfo->Pid);
			if (rsc_RscReq.m_ReqNum >
				_SUPPORT_MAX_RSC_FRAME_REQUEST_) {
#define ERR_STR "RSC Enque Num is bigger than enqueNum:%d\n"
				LOG_ERR(ERR_STR, rsc_RscReq.m_ReqNum);
#undef ERR_STR
				Ret = -EFAULT;
				goto EXIT;
			}
			if (copy_from_user(g_RscEnqueReq_Struct.RscFrameConfig,
				(void *)rsc_RscReq.m_pRscConfig,
				rsc_RscReq.m_ReqNum *
				sizeof(struct RSC_Config)) != 0) {
				LOG_ERR("copy RSCConfig from req is fail!!\n");
				Ret = -EFAULT;
				goto EXIT;
			}

			mutex_lock(&gRscMutex);	/* Protect the Multi Process */

			spin_lock_irqsave(
				&(RSCInfo.SpinLockIrq[RSC_IRQ_TYPE_INT_RSC_ST]),
				flags);
			if (RSC_REQUEST_STATE_EMPTY ==
				g_RSC_ReqRing.RSCReq_Struct[
					g_RSC_ReqRing.WriteIdx].State) {

				g_RSC_ReqRing.RSCReq_Struct[
					g_RSC_ReqRing.WriteIdx].processID =
					pUserInfo->Pid;

				g_RSC_ReqRing.RSCReq_Struct[
					g_RSC_ReqRing.WriteIdx].enqueReqNum =
					rsc_RscReq.m_ReqNum;

				for (idx = 0;
					idx < rsc_RscReq.m_ReqNum; idx++) {

					g_RSC_ReqRing.RSCReq_Struct[
						g_RSC_ReqRing.WriteIdx].
						RscFrameStatus[
						g_RSC_ReqRing.RSCReq_Struct[
						g_RSC_ReqRing.WriteIdx].
						FrameWRIdx] =
						RSC_FRAME_STATUS_ENQUE;

					memcpy(&g_RSC_ReqRing.RSCReq_Struct[
						g_RSC_ReqRing.WriteIdx].
						RscFrameConfig[g_RSC_ReqRing.
						RSCReq_Struct[
						g_RSC_ReqRing.WriteIdx].
						FrameWRIdx++],
						&g_RscEnqueReq_Struct.
						RscFrameConfig[idx],
						sizeof(struct RSC_Config));

				}
				g_RSC_ReqRing.RSCReq_Struct[
					g_RSC_ReqRing.WriteIdx].State =
					RSC_REQUEST_STATE_PENDING;
				RscWriteIdx = g_RSC_ReqRing.WriteIdx;
				g_RSC_ReqRing.WriteIdx =
				    (g_RSC_ReqRing.WriteIdx +
				     1) % _SUPPORT_MAX_RSC_REQUEST_RING_SIZE_;
				LOG_DBG("RSC request enque done!!\n");
			} else {
#define ERR_STR1 "Enque req NG: WriteIdx(%d) Stat(%d) FrameWRIdx(%d)"
#define ERR_STR2 " enqueReqNum(%d)\n"
				LOG_ERR(ERR_STR1 ERR_STR2,
					g_RSC_ReqRing.WriteIdx,
					g_RSC_ReqRing.RSCReq_Struct[
						g_RSC_ReqRing.WriteIdx].State,
					g_RSC_ReqRing.RSCReq_Struct[
						g_RSC_ReqRing.WriteIdx].
						FrameWRIdx,
					g_RSC_ReqRing.RSCReq_Struct[
						g_RSC_ReqRing.WriteIdx].
						enqueReqNum);
#undef ERR_STR1
#undef ERR_STR2
			}
			spin_unlock_irqrestore(
				&(RSCInfo.SpinLockIrq[RSC_IRQ_TYPE_INT_RSC_ST]),
				flags);
			LOG_DBG("ConfigRSC Request!!\n");
			ConfigRSCRequest(RscWriteIdx);

			mutex_unlock(&gRscMutex);
		} else {
			LOG_ERR("RSC_ENQUE_REQ copy_from_user failed\n");
			Ret = -EFAULT;
		}

		break;
	}
	case RSC_DEQUE_NUM:
	{
		if (RSC_REQUEST_STATE_FINISHED ==
		    g_RSC_ReqRing.RSCReq_Struct[g_RSC_ReqRing.ReadIdx].
		    State) {
			dequeNum =
			    g_RSC_ReqRing.RSCReq_Struct[g_RSC_ReqRing.ReadIdx].
			    enqueReqNum;
			LOG_DBG("RSC_DEQUE_NUM(%d)\n", dequeNum);
		} else {
			dequeNum = 0;
#define ERR_STR1 "DEQUE_NUM:No Buffer: ReadIdx(%d) State(%d) RrameRDIdx(%d)"
#define ERR_STR2 " enqueReqNum(%d)\n"
			LOG_ERR(ERR_STR1 ERR_STR2,
				g_RSC_ReqRing.ReadIdx,
				g_RSC_ReqRing.RSCReq_Struct[
					g_RSC_ReqRing.ReadIdx].State,
				g_RSC_ReqRing.RSCReq_Struct[
					g_RSC_ReqRing.ReadIdx].RrameRDIdx,
				g_RSC_ReqRing.RSCReq_Struct[
					g_RSC_ReqRing.ReadIdx].enqueReqNum);
#undef ERR_STR1
#undef ERR_STR2
		}
		if (copy_to_user((void *)Param, &dequeNum,
			sizeof(unsigned int)) != 0) {
			LOG_ERR("RSC_DEQUE_NUM copy_to_user failed\n");
			Ret = -EFAULT;
		}

		break;
	}

	case RSC_DEQUE:
	{
		spin_lock_irqsave(
			&(RSCInfo.SpinLockIrq[RSC_IRQ_TYPE_INT_RSC_ST]), flags);
		if ((RSC_REQUEST_STATE_FINISHED ==
			g_RSC_ReqRing.RSCReq_Struct[g_RSC_ReqRing.ReadIdx].
			State) && (g_RSC_ReqRing.RSCReq_Struct[
			g_RSC_ReqRing.ReadIdx].RrameRDIdx <
			g_RSC_ReqRing.RSCReq_Struct[g_RSC_ReqRing.ReadIdx].
			enqueReqNum)) {

			if (RSC_FRAME_STATUS_FINISHED ==
				g_RSC_ReqRing.RSCReq_Struct[
				g_RSC_ReqRing.ReadIdx].RscFrameStatus[
				g_RSC_ReqRing.RSCReq_Struct[
				g_RSC_ReqRing.ReadIdx].RrameRDIdx]) {

				memcpy(&rsc_RscConfig,
					&g_RSC_ReqRing.RSCReq_Struct[
					g_RSC_ReqRing.ReadIdx].RscFrameConfig[
					g_RSC_ReqRing.RSCReq_Struct[
					g_RSC_ReqRing.ReadIdx].RrameRDIdx],
					sizeof(struct RSC_Config));

				g_RSC_ReqRing.RSCReq_Struct[
					g_RSC_ReqRing.ReadIdx].RscFrameStatus[
					g_RSC_ReqRing.RSCReq_Struct[
					g_RSC_ReqRing.ReadIdx].RrameRDIdx++] =
					RSC_FRAME_STATUS_EMPTY;
			}
			if (g_RSC_ReqRing.RSCReq_Struct[g_RSC_ReqRing.ReadIdx].
				RrameRDIdx ==
				g_RSC_ReqRing.RSCReq_Struct[
				g_RSC_ReqRing.ReadIdx].enqueReqNum) {

				g_RSC_ReqRing.RSCReq_Struct[
					g_RSC_ReqRing.ReadIdx].State =
					RSC_REQUEST_STATE_EMPTY;

				g_RSC_ReqRing.RSCReq_Struct[
					g_RSC_ReqRing.ReadIdx].FrameWRIdx = 0;

				g_RSC_ReqRing.RSCReq_Struct[
					g_RSC_ReqRing.ReadIdx].RrameRDIdx = 0;

				g_RSC_ReqRing.RSCReq_Struct[
					g_RSC_ReqRing.ReadIdx].enqueReqNum = 0;

				g_RSC_ReqRing.ReadIdx =
				    (g_RSC_ReqRing.ReadIdx +
				     1) % _SUPPORT_MAX_RSC_REQUEST_RING_SIZE_;
				LOG_DBG("RSC ReadIdx(%d)\n",
					g_RSC_ReqRing.ReadIdx);
			}
			spin_unlock_irqrestore(
				&(RSCInfo.SpinLockIrq[RSC_IRQ_TYPE_INT_RSC_ST]),
				flags);
			if (copy_to_user((void *)Param,
				&g_RSC_ReqRing.RSCReq_Struct[
				g_RSC_ReqRing.ReadIdx].RscFrameConfig[
				g_RSC_ReqRing.RSCReq_Struct[g_RSC_ReqRing.
				ReadIdx].RrameRDIdx],
				sizeof(struct RSC_Config)) != 0) {
				LOG_ERR("RSC_DEQUE copy_to_user failed\n");
				Ret = -EFAULT;
			}

		} else {
			spin_unlock_irqrestore(
				&(RSCInfo.SpinLockIrq[RSC_IRQ_TYPE_INT_RSC_ST]),
				flags);
#define ERR_STR1 "RSC_DEQUE No Buffer: ReadIdx(%d) State(%d) RrameRDIdx(%d),"
#define ERR_STR2 " enqueReqNum(%d)\n"

			LOG_ERR(ERR_STR1 ERR_STR2,
				g_RSC_ReqRing.ReadIdx,
				g_RSC_ReqRing.RSCReq_Struct[
					g_RSC_ReqRing.ReadIdx].State,
				g_RSC_ReqRing.RSCReq_Struct[
					g_RSC_ReqRing.ReadIdx].RrameRDIdx,
				g_RSC_ReqRing.RSCReq_Struct[
					g_RSC_ReqRing.ReadIdx].enqueReqNum);
#undef ERR_STR1
#undef ERR_STR2
		}

		break;
	}
	case RSC_DEQUE_REQ:
	{
		if (copy_from_user(&rsc_RscReq, (void *)Param,
			sizeof(struct RSC_Request)) == 0) {
			/* Protect the Multi Process */
			mutex_lock(&gRscDequeMutex);
			spin_lock_irqsave(
				&(RSCInfo.SpinLockIrq[RSC_IRQ_TYPE_INT_RSC_ST]),
				flags);
			if (RSC_REQUEST_STATE_FINISHED ==
			    g_RSC_ReqRing.RSCReq_Struct[g_RSC_ReqRing.ReadIdx].
			    State) {
				dequeNum =
					g_RSC_ReqRing.RSCReq_Struct[
					g_RSC_ReqRing.ReadIdx].enqueReqNum;
				LOG_DBG("RSC_DEQUE_REQ(%d)\n", dequeNum);
			} else {
				dequeNum = 0;
#define ERR_STR1 "DEQUE_REQ no buf:RIdx(%d) Stat(%d) RrameRDIdx(%d)"
#define ERR_STR2 " enqueReqNum(%d)\n"
				LOG_ERR(ERR_STR1 ERR_STR2,
					g_RSC_ReqRing.ReadIdx,
					g_RSC_ReqRing.RSCReq_Struct[
						g_RSC_ReqRing.ReadIdx].State,
					g_RSC_ReqRing.RSCReq_Struct[
						g_RSC_ReqRing.ReadIdx].
							RrameRDIdx,
					g_RSC_ReqRing.RSCReq_Struct[
						g_RSC_ReqRing.ReadIdx].
							enqueReqNum);
#undef ERR_STR1
#undef ERR_STR2
			}
			rsc_RscReq.m_ReqNum = dequeNum;

			for (idx = 0; idx < dequeNum; idx++) {
				if (RSC_FRAME_STATUS_FINISHED ==
					g_RSC_ReqRing.RSCReq_Struct[
					g_RSC_ReqRing.ReadIdx].RscFrameStatus[
					g_RSC_ReqRing.RSCReq_Struct[
					g_RSC_ReqRing.ReadIdx].RrameRDIdx]) {

					memcpy(&g_RscDequeReq_Struct.
						RscFrameConfig[idx],
					       &g_RSC_ReqRing.
					       RSCReq_Struct[g_RSC_ReqRing.
					       ReadIdx].RscFrameConfig[
					       g_RSC_ReqRing.RSCReq_Struct[
					       g_RSC_ReqRing.ReadIdx].
					       RrameRDIdx],
					       sizeof(struct RSC_Config));

					g_RSC_ReqRing.RSCReq_Struct[
						g_RSC_ReqRing.ReadIdx].
						RscFrameStatus[g_RSC_ReqRing.
						RSCReq_Struct[g_RSC_ReqRing.
						ReadIdx].RrameRDIdx++] =
						RSC_FRAME_STATUS_EMPTY;

				} else {
#define ERR_STR "deq err idx(%d) dequNum(%d) Rd(%d) RrameRD(%d) FrmStat(%d)\n"
					LOG_ERR(ERR_STR, idx, dequeNum,
						g_RSC_ReqRing.ReadIdx,
		g_RSC_ReqRing.RSCReq_Struct[g_RSC_ReqRing.ReadIdx].RrameRDIdx,
		g_RSC_ReqRing.RSCReq_Struct[g_RSC_ReqRing.ReadIdx].
			RscFrameStatus[g_RSC_ReqRing.RSCReq_Struct[
				g_RSC_ReqRing.ReadIdx].RrameRDIdx]);
#undef ERR_STR
				}
			}
			g_RSC_ReqRing.RSCReq_Struct[g_RSC_ReqRing.ReadIdx].
			    State = RSC_REQUEST_STATE_EMPTY;
			g_RSC_ReqRing.RSCReq_Struct[g_RSC_ReqRing.ReadIdx].
			    FrameWRIdx = 0;
			g_RSC_ReqRing.RSCReq_Struct[g_RSC_ReqRing.ReadIdx].
			    RrameRDIdx = 0;
			g_RSC_ReqRing.RSCReq_Struct[g_RSC_ReqRing.ReadIdx].
			    enqueReqNum = 0;
			g_RSC_ReqRing.ReadIdx =
			    (g_RSC_ReqRing.ReadIdx +
			     1) % _SUPPORT_MAX_RSC_REQUEST_RING_SIZE_;
			LOG_DBG("RSC Request ReadIdx(%d)\n",
				g_RSC_ReqRing.ReadIdx);

			spin_unlock_irqrestore(
				&(RSCInfo.SpinLockIrq[RSC_IRQ_TYPE_INT_RSC_ST]),
				flags);

			mutex_unlock(&gRscDequeMutex);
			if (rsc_RscReq.m_pRscConfig == NULL) {
				LOG_ERR("NULL pointer:rsc_RscReq.m_pRscConfig");
				Ret = -EFAULT;
				goto EXIT;
			}
			if (copy_to_user((void *)rsc_RscReq.m_pRscConfig,
			     &g_RscDequeReq_Struct.RscFrameConfig[0],
			     dequeNum * sizeof(struct RSC_Config)) != 0) {
#define ERR_STR "RSC_DEQUE_REQ copy_to_user frameconfig failed\n"
				LOG_ERR(ERR_STR);
#undef ERR_STR
				Ret = -EFAULT;
			}
			if (copy_to_user((void *)Param, &rsc_RscReq,
				sizeof(struct RSC_Request)) != 0) {
				LOG_ERR("RSC_DEQUE_REQ copy_to_user failed\n");
				Ret = -EFAULT;
			}
		} else {
#define ERR_STR "RSC_CMD_RSC_DEQUE_REQ copy_from_user failed\n"
			LOG_ERR(ERR_STR);
#undef ERR_STR
			Ret = -EFAULT;
		}

		break;
	}
	default:
	{
		LOG_ERR("Unknown Cmd(%d)", Cmd);
#define ERR_STR "Fail, Cmd(%d), Dir(%d), Type(%d), Nr(%d),Size(%d)\n"
		LOG_ERR(ERR_STR, Cmd, _IOC_DIR(Cmd),
			_IOC_TYPE(Cmd), _IOC_NR(Cmd), _IOC_SIZE(Cmd));
#undef ERR_STR
		Ret = -EPERM;
		break;
	}
}
	/*  */
EXIT:
	if (Ret != 0) {
		LOG_ERR("Fail, Cmd(%d),Pid(%d), (proc, pid, tgid)=(%s, %d, %d)",
			Cmd, pUserInfo->Pid,
			current->comm, current->pid, current->tgid);
	}
	/*  */
	return Ret;
}

#ifdef CONFIG_COMPAT

/*******************************************************************************
 *
 ******************************************************************************/
static int compat_get_RSC_read_register_data(
	struct compat_RSC_REG_IO_STRUCT __user *data32,
	struct RSC_REG_IO_STRUCT __user *data)
{
	compat_uint_t count;
	compat_uptr_t uptr;
	int err;

	err = get_user(uptr, &data32->pData);
	err |= put_user(compat_ptr(uptr), &data->pData);
	err |= get_user(count, &data32->Count);
	err |= put_user(count, &data->Count);
	return err;
}

static int compat_put_RSC_read_register_data(
	struct compat_RSC_REG_IO_STRUCT __user *data32,
	struct RSC_REG_IO_STRUCT __user *data)
{
	compat_uint_t count;
	/*compat_uptr_t uptr;*/
	int err = 0;
	/* Assume data pointer is unchanged. */
	/* err = get_user(compat_ptr(uptr), &data->pData); */
	/* err |= put_user(uptr, &data32->pData); */
	err |= get_user(count, &data->Count);
	err |= put_user(count, &data32->Count);
	return err;
}

static int compat_get_RSC_enque_req_data(
	struct compat_RSC_Request __user *data32,
	struct RSC_Request __user *data)
{
	compat_uint_t count;
	compat_uptr_t uptr;
	int err = 0;

	err = get_user(uptr, &data32->m_pRscConfig);
	err |= put_user(compat_ptr(uptr), &data->m_pRscConfig);
	err |= get_user(count, &data32->m_ReqNum);
	err |= put_user(count, &data->m_ReqNum);
	return err;
}


static int compat_put_RSC_enque_req_data(
	struct compat_RSC_Request __user *data32,
	struct RSC_Request __user *data)
{
	compat_uint_t count;
	/*compat_uptr_t uptr;*/
	int err = 0;
	/* Assume data pointer is unchanged. */
	/* err = get_user(compat_ptr(uptr), &data->m_pDpeConfig); */
	/* err |= put_user(uptr, &data32->m_pDpeConfig); */
	err |= get_user(count, &data->m_ReqNum);
	err |= put_user(count, &data32->m_ReqNum);
	return err;
}


static int compat_get_RSC_deque_req_data(
	struct compat_RSC_Request __user *data32,
	struct RSC_Request __user *data)
{
	compat_uint_t count;
	compat_uptr_t uptr;
	int err = 0;

	err = get_user(uptr, &data32->m_pRscConfig);
	err |= put_user(compat_ptr(uptr), &data->m_pRscConfig);
	err |= get_user(count, &data32->m_ReqNum);
	err |= put_user(count, &data->m_ReqNum);
	return err;
}


static int compat_put_RSC_deque_req_data(
	struct compat_RSC_Request __user *data32,
	struct RSC_Request __user *data)
{
	compat_uint_t count;
	/*compat_uptr_t uptr;*/
	int err = 0;
	/* Assume data pointer is unchanged. */
	/* err = get_user(compat_ptr(uptr), &data->m_pDpeConfig); */
	/* err |= put_user(uptr, &data32->m_pDpeConfig); */
	err |= get_user(count, &data->m_ReqNum);
	err |= put_user(count, &data32->m_ReqNum);
	return err;
}

static long RSC_ioctl_compat(struct file *filp, unsigned int cmd,
	unsigned long arg)
{
	long ret;


	if (!filp->f_op || !filp->f_op->unlocked_ioctl) {
		LOG_ERR("no f_op !!!\n");
		return -ENOTTY;
	}
	switch (cmd) {
	case COMPAT_RSC_READ_REGISTER:
	{
		struct compat_RSC_REG_IO_STRUCT __user *data32;
		struct RSC_REG_IO_STRUCT __user *data;
		int err;

		data32 = compat_ptr(arg);
		data = compat_alloc_user_space(sizeof(*data));
		if (data == NULL)
			return -EFAULT;

		err = compat_get_RSC_read_register_data(data32, data);
		if (err) {
			LOG_INF("compat_get_RSC_read_register_data error!!!\n");
			return err;
		}
		ret =
		    filp->f_op->unlocked_ioctl(filp, RSC_READ_REGISTER,
					       (unsigned long)data);
		err = compat_put_RSC_read_register_data(data32, data);
		if (err) {
			LOG_INF("compat_put_RSC_read_register_data error!!!\n");
			return err;
		}
		return ret;
	}
	case COMPAT_RSC_WRITE_REGISTER:
	{
		struct compat_RSC_REG_IO_STRUCT __user *data32;
		struct RSC_REG_IO_STRUCT __user *data;
		int err;

		data32 = compat_ptr(arg);
		data = compat_alloc_user_space(sizeof(*data));
		if (data == NULL)
			return -EFAULT;

		err = compat_get_RSC_read_register_data(data32, data);
		if (err) {
			LOG_INF("COMPAT_RSC_WRITE_REGISTER error!!!\n");
			return err;
		}
		ret =
		    filp->f_op->unlocked_ioctl(filp, RSC_WRITE_REGISTER,
					       (unsigned long)data);
		return ret;
	}
	case COMPAT_RSC_ENQUE_REQ:
	{
		struct compat_RSC_Request __user *data32;
		struct RSC_Request __user *data;
		int err;

		data32 = compat_ptr(arg);
		data = compat_alloc_user_space(sizeof(*data));
		if (data == NULL)
			return -EFAULT;

		err = compat_get_RSC_enque_req_data(data32, data);
		if (err) {
			LOG_INF("COMPAT_RSC_ENQUE_REQ error!!!\n");
			return err;
		}
		ret =
		    filp->f_op->unlocked_ioctl(filp, RSC_ENQUE_REQ,
					       (unsigned long)data);
		err = compat_put_RSC_enque_req_data(data32, data);
		if (err) {
			LOG_INF("COMPAT_RSC_ENQUE_REQ error!!!\n");
			return err;
		}
		return ret;
	}
	case COMPAT_RSC_DEQUE_REQ:
	{
		struct compat_RSC_Request __user *data32;
		struct RSC_Request __user *data;
		int err;

		data32 = compat_ptr(arg);
		data = compat_alloc_user_space(sizeof(*data));
		if (data == NULL)
			return -EFAULT;

		err = compat_get_RSC_deque_req_data(data32, data);
		if (err) {
			LOG_INF("COMPAT_RSC_DEQUE_REQ error!!!\n");
			return err;
		}
		ret =
		    filp->f_op->unlocked_ioctl(filp, RSC_DEQUE_REQ,
					       (unsigned long)data);
		err = compat_put_RSC_deque_req_data(data32, data);
		if (err) {
			LOG_INF("COMPAT_RSC_DEQUE_REQ error!!!\n");
			return err;
		}
		return ret;
	}

	case RSC_WAIT_IRQ:
	case RSC_CLEAR_IRQ:	/* structure (no pointer) */
	case RSC_ENQNUE_NUM:
	case RSC_ENQUE:
	case RSC_DEQUE_NUM:
	case RSC_DEQUE:
	case RSC_RESET:
	case RSC_DUMP_REG:
	case RSC_DUMP_ISR_LOG:
		return filp->f_op->unlocked_ioctl(filp, cmd, arg);
	default:
		return -ENOIOCTLCMD;
		/* return RSC_ioctl(filep, cmd, arg); */
	}
}

#endif

/*******************************************************************************
 *
 ******************************************************************************/
static signed int RSC_open(struct inode *pInode, struct file *pFile)
{
	signed int Ret = 0;
	unsigned int i, j;
	/*int q = 0, p = 0;*/
	struct RSC_USER_INFO_STRUCT *pUserInfo;

	LOG_DBG("- E. UserCount: %d.", RSCInfo.UserCount);


	/*  */
	spin_lock(&(RSCInfo.SpinLockRSCRef));

	pFile->private_data = NULL;
	pFile->private_data = kmalloc(sizeof(struct RSC_USER_INFO_STRUCT),
		GFP_ATOMIC);
	if (pFile->private_data == NULL) {
		LOG_DBG("ERROR: kmalloc failed, (proc, pid, tgid)=(%s, %d, %d)",
			current->comm,
			current->pid, current->tgid);
		Ret = -ENOMEM;
	} else {
		pUserInfo = (struct RSC_USER_INFO_STRUCT *) pFile->private_data;
		pUserInfo->Pid = current->pid;
		pUserInfo->Tid = current->tgid;
	}
	/*  */
	if (RSCInfo.UserCount > 0) {
		RSCInfo.UserCount++;
		spin_unlock(&(RSCInfo.SpinLockRSCRef));
#define TMP_STR "Cur UserCount(%d), (proc, pid, tgid)=(%s, %d, %d), users exist"
		LOG_DBG(TMP_STR,
			RSCInfo.UserCount, current->comm, current->pid,
			current->tgid);
#undef TMP_STR
		goto EXIT;
	} else {
		RSCInfo.UserCount++;
		spin_unlock(&(RSCInfo.SpinLockRSCRef));
#define TMP_STR "Curr UserCount(%d), (proc, pid, tgid)=(%s, %d, %d), first user"
		LOG_DBG(TMP_STR,
			RSCInfo.UserCount, current->comm, current->pid,
			current->tgid);
#undef TMP_STR
	}

	/* do wait queue head init when re-enter in camera */
	/*  */
	for (i = 0; i < _SUPPORT_MAX_RSC_REQUEST_RING_SIZE_; i++) {
		/* RSC */
		g_RSC_ReqRing.RSCReq_Struct[i].processID = 0x0;
		g_RSC_ReqRing.RSCReq_Struct[i].callerID = 0x0;
		g_RSC_ReqRing.RSCReq_Struct[i].enqueReqNum = 0x0;
		/* g_RSC_ReqRing.RSCReq_Struct[i].enqueIdx = 0x0; */
		g_RSC_ReqRing.RSCReq_Struct[i].State = RSC_REQUEST_STATE_EMPTY;
		g_RSC_ReqRing.RSCReq_Struct[i].FrameWRIdx = 0x0;
		g_RSC_ReqRing.RSCReq_Struct[i].RrameRDIdx = 0x0;
		for (j = 0; j < _SUPPORT_MAX_RSC_FRAME_REQUEST_; j++) {
			g_RSC_ReqRing.RSCReq_Struct[i].RscFrameStatus[j] =
			    RSC_FRAME_STATUS_EMPTY;
		}

	}
	g_RSC_ReqRing.WriteIdx = 0x0;
	g_RSC_ReqRing.ReadIdx = 0x0;
	g_RSC_ReqRing.HWProcessIdx = 0x0;

	/* Enable clock */
	RSC_EnableClock(MTRUE);
	LOG_DBG("RSC open g_u4EnableClockCount: %d", g_u4EnableClockCount);
	/*  */

	for (i = 0; i < RSC_IRQ_TYPE_AMOUNT; i++)
		RSCInfo.IrqInfo.Status[i] = 0;

	for (i = 0; i < _SUPPORT_MAX_RSC_FRAME_REQUEST_; i++)
		RSCInfo.ProcessID[i] = 0;

	RSCInfo.WriteReqIdx = 0;
	RSCInfo.ReadReqIdx = 0;
	RSCInfo.IrqInfo.RscIrqCnt = 0;

/*#define KERNEL_LOG*/
#ifdef KERNEL_LOG
    /* In EP, Add RSC_DBG_WRITE_REG for debug. Should remove it after EP */
	RSCInfo.DebugMask = (RSC_DBG_INT | RSC_DBG_DBGLOG | RSC_DBG_WRITE_REG);
#endif
	/*  */
EXIT:




	LOG_DBG("- X. Ret: %d. UserCount: %d.", Ret, RSCInfo.UserCount);
	return Ret;

}

/*******************************************************************************
 *
 ******************************************************************************/
static signed int RSC_release(struct inode *pInode, struct file *pFile)
{
	struct RSC_USER_INFO_STRUCT *pUserInfo;
	/*unsigned int Reg;*/

	LOG_DBG("- E. UserCount: %d.", RSCInfo.UserCount);

	/*  */
	if (pFile->private_data != NULL) {
		pUserInfo =
			(struct  RSC_USER_INFO_STRUCT *) pFile->private_data;
		kfree(pFile->private_data);
		pFile->private_data = NULL;
	}
	/*  */
	spin_lock(&(RSCInfo.SpinLockRSCRef));
	RSCInfo.UserCount--;

	if (RSCInfo.UserCount > 0) {
		spin_unlock(&(RSCInfo.SpinLockRSCRef));
#define TMP_STR1 "Curr UserCount(%d), (process, pid, tgid)=(%s, %d, %d),"
#define TMP_STR2 " users exist"
		LOG_DBG(TMP_STR1 TMP_STR2,
			RSCInfo.UserCount, current->comm, current->pid,
			current->tgid);
#undef TMP_STR1
#undef TMP_STR2
		goto EXIT;
	} else
		spin_unlock(&(RSCInfo.SpinLockRSCRef));
	/*  */
	LOG_DBG("Curr UserCount(%d), (proc, pid, tgid)=(%s, %d, %d), last user",
		RSCInfo.UserCount, current->comm, current->pid, current->tgid);


	/* Disable clock. */
	RSC_EnableClock(MFALSE);
	LOG_DBG("RSC release g_u4EnableClockCount: %d", g_u4EnableClockCount);

	/*  */
EXIT:


	LOG_DBG("- X. UserCount: %d.", RSCInfo.UserCount);
	return 0;
}


/*******************************************************************************
 *
 ******************************************************************************/
static signed int RSC_mmap(struct file *pFile, struct vm_area_struct *pVma)
{
	unsigned long length = 0;
	unsigned int pfn = 0x0;

	length = pVma->vm_end - pVma->vm_start;
	/*  */
	pVma->vm_page_prot = pgprot_noncached(pVma->vm_page_prot);
	pfn = pVma->vm_pgoff << PAGE_SHIFT;

#define TMP_STR1 "%s:vm_pgoff(0x%lx) pfn(0x%x) phy(0x%lx) vm_start(0x%lx)"
#define TMP_STR2 " vm_end(0x%lx) length(0x%lx)"
	LOG_INF(TMP_STR1 TMP_STR2,
	     __func__, pVma->vm_pgoff, pfn, pVma->vm_pgoff << PAGE_SHIFT,
	     pVma->vm_start, pVma->vm_end, length);
#undef TMP_STR1
#undef TMP_STR2

	switch (pfn) {
	case RSC_BASE_HW:
		if (length > RSC_REG_RANGE) {
#define TMP_STR1 "mmap range error :module:0x%x length(0x%lx),"
#define TMP_STR2 " RSC_REG_RANGE(0x%x)!"
			LOG_ERR(TMP_STR1 TMP_STR2,
				pfn, length, RSC_REG_RANGE);
#undef TMP_STR1
#undef TMP_STR2
			return -EAGAIN;
		}
		break;
	default:
		LOG_ERR("Illegal starting HW addr for mmap!");
		return -EAGAIN;
	}
	if (remap_pfn_range(pVma, pVma->vm_start, pVma->vm_pgoff,
		pVma->vm_end - pVma->vm_start, pVma->vm_page_prot)) {
		return -EAGAIN;
	}
	/*  */
	return 0;
}

/*******************************************************************************
 *
 ******************************************************************************/

static dev_t RSCDevNo;
static struct cdev *pRSCCharDrv;
static struct class *pRSCClass;

static const struct file_operations RSCFileOper = {
	.owner = THIS_MODULE,
	.open = RSC_open,
	.release = RSC_release,
	/* .flush   = mt_RSC_flush, */
	.mmap = RSC_mmap,
	.unlocked_ioctl = RSC_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = RSC_ioctl_compat,
#endif
};

/*******************************************************************************
 *
 ******************************************************************************/
static inline void RSC_UnregCharDev(void)
{
	LOG_DBG("- E.");
	/*  */
	/* Release char driver */
	if (pRSCCharDrv != NULL) {
		cdev_del(pRSCCharDrv);
		pRSCCharDrv = NULL;
	}
	/*  */
	unregister_chrdev_region(RSCDevNo, 1);
}

/*******************************************************************************
 *
 ******************************************************************************/
static inline signed int RSC_RegCharDev(void)
{
	signed int Ret = 0;
	/*  */
	LOG_DBG("- E.");
	/*  */
	Ret = alloc_chrdev_region(&RSCDevNo, 0, 1, RSC_DEV_NAME);
	if (Ret < 0) {
		LOG_ERR("alloc_chrdev_region failed, %d", Ret);
		return Ret;
	}
	/* Allocate driver */
	pRSCCharDrv = cdev_alloc();
	if (pRSCCharDrv == NULL) {
		LOG_ERR("cdev_alloc failed");
		Ret = -ENOMEM;
		goto EXIT;
	}
	/* Attatch file operation. */
	cdev_init(pRSCCharDrv, &RSCFileOper);
	/*  */
	pRSCCharDrv->owner = THIS_MODULE;
	/* Add to system */
	Ret = cdev_add(pRSCCharDrv, RSCDevNo, 1);
	if (Ret < 0) {
		LOG_ERR("Attatch file operation failed, %d", Ret);
		goto EXIT;
	}
	/*  */
EXIT:
	if (Ret < 0)
		RSC_UnregCharDev();

	/*  */

	LOG_DBG("- X.");
	return Ret;
}

/*******************************************************************************
 *
 ******************************************************************************/
static signed int RSC_probe(struct platform_device *pDev)
{
	signed int Ret = 0;
	/*struct resource *pRes = NULL;*/
	signed int i = 0;
	unsigned char n;
	unsigned int irq_info[3]; /* Record interrupts info from device tree */
	struct device *dev = NULL;
	struct RSC_device *_rsc_dev;


#ifdef CONFIG_OF
	struct RSC_device *RSC_dev;
#endif

	LOG_INF("- E. RSC driver probe.\n");

	/* Check platform_device parameters */
#ifdef CONFIG_OF

	if (pDev == NULL) {
		dev_info(&pDev->dev, "Error: pDev is NULL");
		return -ENXIO;
	}

	nr_RSC_devs += 1;
	_rsc_dev = krealloc(RSC_devs, sizeof(struct RSC_device) * nr_RSC_devs,
		GFP_KERNEL);
	if (!_rsc_dev) {
		dev_info(&pDev->dev, "Error: Unable to allocate RSC_devs\n");
		return -ENOMEM;
	}
	RSC_devs = _rsc_dev;

	RSC_dev = &(RSC_devs[nr_RSC_devs - 1]);
	RSC_dev->dev = &pDev->dev;

	/* iomap registers */
	RSC_dev->regs = of_iomap(pDev->dev.of_node, 0);
	/* gISPSYS_Reg[nr_RSC_devs - 1] = RSC_dev->regs; */

	if (!RSC_dev->regs) {
#define TMP_STR1 "Error: Unable to ioremap registers, of_iomap fail,"
#define TMP_STR2 " nr_RSC_devs=%d, devnode(%s).\n"
		dev_info(&pDev->dev,
			TMP_STR1 TMP_STR2,
			nr_RSC_devs, pDev->dev.of_node->name);
#undef TMP_STR1
#undef TMP_STR2
		return -ENOMEM;
	}

	LOG_INF("nr_RSC_devs=%d, devnode(%s), map_addr=0x%lx\n", nr_RSC_devs,
		pDev->dev.of_node->name, (unsigned long)RSC_dev->regs);

	/* get IRQ ID and request IRQ */
	RSC_dev->irq = irq_of_parse_and_map(pDev->dev.of_node, 0);

	if (RSC_dev->irq > 0) {
		/* Get IRQ Flag from device node */
		if (of_property_read_u32_array(pDev->dev.of_node, "interrupts",
			irq_info, ARRAY_SIZE(irq_info))) {
			dev_info(&pDev->dev,
				"Error: get irq flags from DTS fail!!\n");
			return -ENODEV;
		}

		for (i = 0; i < RSC_IRQ_TYPE_AMOUNT; i++) {
			if (strcmp(pDev->dev.of_node->name,
				RSC_IRQ_CB_TBL[i].device_name) == 0) {
				Ret = request_irq(RSC_dev->irq,
					(irq_handler_t)RSC_IRQ_CB_TBL[i].isr_fp,
					irq_info[2],
				(const char *)RSC_IRQ_CB_TBL[i].device_name,
					NULL);

				if (Ret) {
#define TMP_STR1 "Error: Unable to request IRQ, request_irq fail,"
#define TMP_STR2 " nr_RSC_devs=%d, devnode(%s), irq=%d, ISR: %s\n"
					dev_info(&pDev->dev,
						TMP_STR1 TMP_STR2,
						nr_RSC_devs,
						pDev->dev.of_node->name,
						RSC_dev->irq,
						RSC_IRQ_CB_TBL[i].device_name);
#undef TMP_STR1
#undef TMP_STR2
					return Ret;
				}
#define TMP_STR "nr_RSC_devs=%d, devnode(%s), irq=%d, ISR: %s\n"
				LOG_INF(TMP_STR, nr_RSC_devs,
					pDev->dev.of_node->name, RSC_dev->irq,
					RSC_IRQ_CB_TBL[i].device_name);
#undef TMP_STR
				break;
			}
		}

		if (i >= RSC_IRQ_TYPE_AMOUNT) {
#define TMP_STR "No corresponding ISR!!: nr_RSC_devs=%d, devnode(%s), irq=%d\n"
			LOG_INF(TMP_STR, nr_RSC_devs, pDev->dev.of_node->name,
			RSC_dev->irq);
#undef TMP_STR
		}


	} else {
		LOG_INF("No IRQ!!: nr_RSC_devs=%d, devnode(%s), irq=%d\n",
			nr_RSC_devs,
			pDev->dev.of_node->name, RSC_dev->irq);
	}


#endif

	/* Only register char driver in the 1st time */
	if (nr_RSC_devs == 2) {

		/* Register char driver */
		Ret = RSC_RegCharDev();
		if (Ret) {
			dev_info(&pDev->dev, "Error: register char failed");
			return Ret;
		}
#ifndef EP_NO_CLKMGR
#if !defined(CONFIG_MTK_LEGACY) && defined(CONFIG_COMMON_CLK) /*CCF*/
		    /*CCF: Grab clock pointer (struct clk*) */
#ifndef SMI_CLK
		rsc_clk.CG_SCP_SYS_MM0 =
			devm_clk_get(&pDev->dev, "RSC_SCP_SYS_MM0");
		rsc_clk.CG_SCP_SYS_ISP =
			devm_clk_get(&pDev->dev, "RSC_SCP_SYS_ISP");
		rsc_clk.CG_MM_SMI_COMMON =
			devm_clk_get(&pDev->dev, "RSC_CLK_MM_CG2_B11");
		rsc_clk.CG_MM_SMI_COMMON_2X =
			devm_clk_get(&pDev->dev, "RSC_CLK_MM_CG2_B12");
		rsc_clk.CG_MM_SMI_COMMON_GALS_M0_2X =
			devm_clk_get(&pDev->dev, "RSC_CLK_MM_CG1_B12");
		rsc_clk.CG_MM_SMI_COMMON_GALS_M1_2X =
			devm_clk_get(&pDev->dev, "RSC_CLK_MM_CG1_B13");
		rsc_clk.CG_MM_SMI_COMMON_UPSZ0 =
			devm_clk_get(&pDev->dev, "RSC_CLK_MM_CG1_B14");
		rsc_clk.CG_MM_SMI_COMMON_UPSZ1 =
			devm_clk_get(&pDev->dev, "RSC_CLK_MM_CG1_B15");
		rsc_clk.CG_MM_SMI_COMMON_FIFO0 =
			devm_clk_get(&pDev->dev, "RSC_CLK_MM_CG1_B16");
		rsc_clk.CG_MM_SMI_COMMON_FIFO1 =
			devm_clk_get(&pDev->dev, "RSC_CLK_MM_CG1_B17");
		rsc_clk.CG_MM_LARB5 =
			devm_clk_get(&pDev->dev, "RSC_CLK_MM_CG1_B10");
		rsc_clk.CG_IMGSYS_LARB =
			devm_clk_get(&pDev->dev, "RSC_CLK_IMG_LARB");
#endif
		rsc_clk.CG_IMGSYS_RSC =
			devm_clk_get(&pDev->dev, "RSC_CLK_IMG_RSC");

#ifndef SMI_CLK
		if (IS_ERR(rsc_clk.CG_SCP_SYS_MM0)) {
			LOG_ERR("cannot get CG_SCP_SYS_MM0 clock\n");
			return PTR_ERR(rsc_clk.CG_SCP_SYS_MM0);
		}
		if (IS_ERR(rsc_clk.CG_SCP_SYS_ISP)) {
			LOG_ERR("cannot get CG_SCP_SYS_ISP clock\n");
			return PTR_ERR(rsc_clk.CG_SCP_SYS_ISP);
		}
		if (IS_ERR(rsc_clk.CG_MM_SMI_COMMON)) {
			LOG_ERR("cannot get CG_MM_SMI_COMMON clock\n");
			return PTR_ERR(rsc_clk.CG_MM_SMI_COMMON);
		}
		if (IS_ERR(rsc_clk.CG_MM_SMI_COMMON_2X)) {
			LOG_ERR("cannot get CG_MM_SMI_COMMON_2X clock\n");
			return PTR_ERR(rsc_clk.CG_MM_SMI_COMMON_2X);
		}
		if (IS_ERR(rsc_clk.CG_MM_SMI_COMMON_GALS_M0_2X)) {
			LOG_ERR("cannot get CG_MM_SMI_COMMON_GALS_M0_2X clk\n");
			return PTR_ERR(rsc_clk.CG_MM_SMI_COMMON_GALS_M0_2X);
		}
		if (IS_ERR(rsc_clk.CG_MM_SMI_COMMON_GALS_M1_2X)) {
			LOG_ERR("cannot get CG_MM_SMI_COMMON_GALS_M1_2X clk\n");
			return PTR_ERR(rsc_clk.CG_MM_SMI_COMMON_GALS_M1_2X);
		}
		if (IS_ERR(rsc_clk.CG_MM_SMI_COMMON_UPSZ0)) {
			LOG_ERR("cannot get CG_MM_SMI_COMMON_UPSZ0 clock\n");
			return PTR_ERR(rsc_clk.CG_MM_SMI_COMMON_UPSZ0);
		}
		if (IS_ERR(rsc_clk.CG_MM_SMI_COMMON_UPSZ1)) {
			LOG_ERR("cannot get CG_MM_SMI_COMMON_UPSZ1 clock\n");
			return PTR_ERR(rsc_clk.CG_MM_SMI_COMMON_UPSZ1);
		}
		if (IS_ERR(rsc_clk.CG_MM_SMI_COMMON_FIFO0)) {
			LOG_ERR("cannot get CG_MM_SMI_COMMON_FIFO0 clock\n");
			return PTR_ERR(rsc_clk.CG_MM_SMI_COMMON_FIFO0);
		}
		if (IS_ERR(rsc_clk.CG_MM_SMI_COMMON_FIFO1)) {
			LOG_ERR("cannot get CG_MM_SMI_COMMON_FIFO1 clock\n");
			return PTR_ERR(rsc_clk.CG_MM_SMI_COMMON_FIFO1);
		}
		if (IS_ERR(rsc_clk.CG_MM_LARB5)) {
			LOG_ERR("cannot get CG_MM_LARB5 clock\n");
			return PTR_ERR(rsc_clk.CG_MM_LARB5);
		}
		if (IS_ERR(rsc_clk.CG_IMGSYS_LARB)) {
			LOG_ERR("cannot get CG_IMGSYS_LARB clock\n");
			return PTR_ERR(rsc_clk.CG_IMGSYS_LARB);
		}
#endif
		if (IS_ERR(rsc_clk.CG_IMGSYS_RSC)) {
			LOG_ERR("cannot get CG_IMGSYS_RSC clock\n");
			return PTR_ERR(rsc_clk.CG_IMGSYS_RSC);
		}
#endif	/* !defined(CONFIG_MTK_LEGACY) && defined(CONFIG_COMMON_CLK)  */
#endif

		/* Create class register */
		pRSCClass = class_create(THIS_MODULE, "RSCdrv");
		if (IS_ERR(pRSCClass)) {
			Ret = PTR_ERR(pRSCClass);
			LOG_ERR("Unable to create class, err = %d", Ret);
			goto EXIT;
		}

		dev = device_create(pRSCClass, NULL, RSCDevNo, NULL,
			RSC_DEV_NAME);
		if (IS_ERR(dev)) {
			Ret = PTR_ERR(dev);
			dev_info(&pDev->dev,
				"Failed to create device: /dev/%s, err = %d",
				RSC_DEV_NAME, Ret);
			goto EXIT;
		}

		/* Init spinlocks */
		spin_lock_init(&(RSCInfo.SpinLockRSCRef));
		spin_lock_init(&(RSCInfo.SpinLockRSC));
		for (n = 0; n < RSC_IRQ_TYPE_AMOUNT; n++)
			spin_lock_init(&(RSCInfo.SpinLockIrq[n]));

		/*  */
		init_waitqueue_head(&RSCInfo.WaitQueueHead);
		INIT_WORK(&RSCInfo.ScheduleRscWork, RSC_ScheduleWork);

		wake_lock_init(&RSC_wake_lock, WAKE_LOCK_SUSPEND,
			"rsc_lock_wakelock");

		for (i = 0; i < RSC_IRQ_TYPE_AMOUNT; i++)
			tasklet_init(RSC_tasklet[i].pRSC_tkt,
				RSC_tasklet[i].tkt_cb, 0);




		/* Init RSCInfo */
		spin_lock(&(RSCInfo.SpinLockRSCRef));
		RSCInfo.UserCount = 0;
		spin_unlock(&(RSCInfo.SpinLockRSCRef));
		/*  */
		RSCInfo.IrqInfo.Mask[RSC_IRQ_TYPE_INT_RSC_ST] = INT_ST_MASK_RSC;

	}

EXIT:
	if (Ret < 0)
		RSC_UnregCharDev();


	LOG_INF("- X. RSC driver probe.");

	return Ret;
}

/*******************************************************************************
 * Called when the device is being detached from the driver
 ******************************************************************************/
static signed int RSC_remove(struct platform_device *pDev)
{
	/*struct resource *pRes;*/
	signed int IrqNum;
	int i;
	/*  */
	LOG_DBG("- E.");
	/* unregister char driver. */
	RSC_UnregCharDev();

	/* Release IRQ */
	disable_irq(RSCInfo.IrqNum);
	IrqNum = platform_get_irq(pDev, 0);
	free_irq(IrqNum, NULL);

	/* kill tasklet */
	for (i = 0; i < RSC_IRQ_TYPE_AMOUNT; i++)
		tasklet_kill(RSC_tasklet[i].pRSC_tkt);

	/*  */
	device_destroy(pRSCClass, RSCDevNo);
	/*  */
	class_destroy(pRSCClass);
	pRSCClass = NULL;
	/*  */
	return 0;
}

/*******************************************************************************
 *
 ******************************************************************************/
static signed int bPass1_On_In_Resume_TG1;

static signed int RSC_suspend(struct platform_device *pDev, pm_message_t Mesg)
{
	/*signed int ret = 0;*/

	LOG_DBG("bPass1_On_In_Resume_TG1(%d)\n", bPass1_On_In_Resume_TG1);

	bPass1_On_In_Resume_TG1 = 0;


	return 0;
}

/*******************************************************************************
 *
 ******************************************************************************/
static signed int RSC_resume(struct platform_device *pDev)
{
	LOG_DBG("bPass1_On_In_Resume_TG1(%d).\n", bPass1_On_In_Resume_TG1);

	return 0;
}

/*---------------------------------------------------------------------------*/
#ifdef CONFIG_PM
/*---------------------------------------------------------------------------*/
int RSC_pm_suspend(struct device *device)
{
	struct platform_device *pdev = to_platform_device(device);

	WARN_ON(pdev == NULL);

	pr_debug("calling %s()\n", __func__);

	return RSC_suspend(pdev, PMSG_SUSPEND);
}

int RSC_pm_resume(struct device *device)
{
	struct platform_device *pdev = to_platform_device(device);

	WARN_ON(pdev == NULL);

	pr_debug("calling %s()\n", __func__);

	return RSC_resume(pdev);
}
#ifndef CONFIG_OF
/*extern void mt_irq_set_sens(unsigned int irq, unsigned int sens);*/
/*extern void mt_irq_set_polarity(unsigned int irq, unsigned int polarity);*/
#endif
int RSC_pm_restore_noirq(struct device *device)
{
	pr_debug("calling %s()\n", __func__);
#ifndef CONFIG_OF
/*	mt_irq_set_sens(RSC_IRQ_BIT_ID, MT_LEVEL_SENSITIVE);*/
/*	mt_irq_set_polarity(RSC_IRQ_BIT_ID, MT_POLARITY_LOW);*/
#endif
	return 0;

}

/*---------------------------------------------------------------------------*/
#else				/*CONFIG_PM */
/*---------------------------------------------------------------------------*/
#define RSC_pm_suspend NULL
#define RSC_pm_resume  NULL
#define RSC_pm_restore_noirq NULL
/*---------------------------------------------------------------------------*/
#endif				/*CONFIG_PM */
/*---------------------------------------------------------------------------*/
#ifdef CONFIG_OF
/*
 * Note!!!
 * The order and member of .compatible must be the same with RSC_DEV_NODE_IDX
 */
static const struct of_device_id RSC_of_ids[] = {
	{.compatible = "mediatek,imgsyscq",},
	{.compatible = "mediatek,rsc",},
	{}
};
#endif

const struct dev_pm_ops RSC_pm_ops = {
	.suspend = RSC_pm_suspend,
	.resume = RSC_pm_resume,
	.freeze = RSC_pm_suspend,
	.thaw = RSC_pm_resume,
	.poweroff = RSC_pm_suspend,
	.restore = RSC_pm_resume,
	.restore_noirq = RSC_pm_restore_noirq,
};


/*******************************************************************************
 *
 ******************************************************************************/
static struct platform_driver RSCDriver = {
	.probe = RSC_probe,
	.remove = RSC_remove,
	.suspend = RSC_suspend,
	.resume = RSC_resume,
	.driver = {
		   .name = RSC_DEV_NAME,
		   .owner = THIS_MODULE,
#ifdef CONFIG_OF
		   .of_match_table = RSC_of_ids,
#endif
#ifdef CONFIG_PM
		   .pm = &RSC_pm_ops,
#endif
	}
};


static int rsc_dump_read(struct seq_file *m, void *v)
{
	int i, j;

	seq_puts(m, "\n============ rsc dump register============\n");
	seq_puts(m, "RSC Config Info\n");

	for (i = 0x2C; i < 0x8C; i = i + 4) {
		seq_printf(m, "[0x%08X %08X]\n",
			(unsigned int)(RSC_BASE_HW + i),
			(unsigned int)RSC_RD32(ISP_RSC_BASE + i));
	}
	seq_puts(m, "RSC Debug Info\n");
	for (i = 0x120; i < 0x148; i = i + 4) {
		seq_printf(m, "[0x%08X %08X]\n",
			(unsigned int)(RSC_BASE_HW + i),
			(unsigned int)RSC_RD32(ISP_RSC_BASE + i));
	}

	seq_puts(m, "RSC Config Info\n");
	for (i = 0x230; i < 0x2D8; i = i + 4) {
		seq_printf(m, "[0x%08X %08X]\n",
			(unsigned int)(RSC_BASE_HW + i),
			(unsigned int)RSC_RD32(ISP_RSC_BASE + i));
	}
	seq_puts(m, "RSC Debug Info\n");
	for (i = 0x2F4; i < 0x30C; i = i + 4) {
		seq_printf(m, "[0x%08X %08X]\n",
			(unsigned int)(RSC_BASE_HW + i),
			(unsigned int)RSC_RD32(ISP_RSC_BASE + i));
	}

	seq_puts(m, "\n");
	seq_printf(m, "Dpe Clock Count:%d\n", g_u4EnableClockCount);

	seq_printf(m, "[0x%08X %08X]\n", (unsigned int)(RSC_DMA_DBG_HW),
		   (unsigned int)RSC_RD32(RSC_DMA_DBG_REG));
	seq_printf(m, "[0x%08X %08X]\n", (unsigned int)(RSC_DMA_REQ_STATUS_HW),
		   (unsigned int)RSC_RD32(RSC_DMA_REQ_STATUS_REG));
	seq_printf(m, "[0x%08X %08X]\n", (unsigned int)(RSC_DMA_RDY_STATUS_HW),
		   (unsigned int)RSC_RD32(RSC_DMA_RDY_STATUS_REG));

	seq_printf(m, "[0x%08X %08X]\n", (unsigned int)(RSC_DMA_RDY_STATUS_HW),
		   (unsigned int)RSC_RD32(RSC_DMA_RDY_STATUS_REG));


	seq_printf(m, "RSC:HWProcessIdx:%d, WriteIdx:%d, ReadIdx:%d\n",
		   g_RSC_ReqRing.HWProcessIdx, g_RSC_ReqRing.WriteIdx,
		   g_RSC_ReqRing.ReadIdx);

	for (i = 0; i < _SUPPORT_MAX_RSC_REQUEST_RING_SIZE_; i++) {
#define TMP_STR1 "RSC:State:%d, processID:0x%08X, callerID:0x%08X,"
#define TMP_STR2 " enqueReqNum:%d, FrameWRIdx:%d, RrameRDIdx:%d\n"
		seq_printf(m, TMP_STR1 TMP_STR2,
			   g_RSC_ReqRing.RSCReq_Struct[i].State,
			   g_RSC_ReqRing.RSCReq_Struct[i].processID,
			   g_RSC_ReqRing.RSCReq_Struct[i].callerID,
			   g_RSC_ReqRing.RSCReq_Struct[i].enqueReqNum,
			   g_RSC_ReqRing.RSCReq_Struct[i].FrameWRIdx,
			   g_RSC_ReqRing.RSCReq_Struct[i].RrameRDIdx);
#undef TMP_STR1
#undef TMP_STR2

		for (j = 0; j < _SUPPORT_MAX_RSC_FRAME_REQUEST_;) {
#define TMP_STR1 "RSC:FrameStatus[%d]:%d, FrameStatus[%d]:%d,"
#define TMP_STR2 " FrameStatus[%d]:%d, FrameStatus[%d]:%d\n"
		seq_printf(m, TMP_STR1 TMP_STR2,
			j,
			g_RSC_ReqRing.RSCReq_Struct[i].RscFrameStatus[j],
			j + 1,
			g_RSC_ReqRing.RSCReq_Struct[i].RscFrameStatus[j + 1],
			j + 2,
			g_RSC_ReqRing.RSCReq_Struct[i].RscFrameStatus[j + 2],
			j + 3,
			g_RSC_ReqRing.RSCReq_Struct[i].RscFrameStatus[j + 3]);
#undef TMP_STR1
#undef TMP_STR2
			j = j + 4;
		}
	}

	seq_puts(m, "\n============ rsc dump debug ============\n");

	return 0;
}


static int proc_rsc_dump_open(struct inode *inode, struct file *file)
{
	return single_open(file, rsc_dump_read, NULL);
}

static const struct file_operations rsc_dump_proc_fops = {
	.owner = THIS_MODULE,
	.open = proc_rsc_dump_open,
	.read = seq_read,
};


static int rsc_reg_read(struct seq_file *m, void *v)
{
	unsigned int i;

	seq_puts(m, "======== read rsc register ========\n");

	for (i = 0x1C; i <= 0x308; i = i + 4) {
		seq_printf(m, "[0x%08X 0x%08X]\n",
			(unsigned int)(RSC_BASE_HW + i),
			(unsigned int)RSC_RD32(ISP_RSC_BASE + i));
	}

	seq_printf(m, "[0x%08X 0x%08X]\n", (unsigned int)(RSC_DMA_DBG_HW),
		   (unsigned int)RSC_RD32(RSC_DMA_DBG_REG));

	seq_printf(m, "[0x%08X 0x%08X]\n",
		(unsigned int)(RSC_DMA_REQ_STATUS_HW),
		(unsigned int)RSC_RD32(RSC_DMA_REQ_STATUS_REG));

	seq_printf(m, "[0x%08X 0x%08X]\n", (unsigned int)(RSC_BASE_HW + 0x7FC),
		   (unsigned int)RSC_RD32(RSC_DMA_RDY_STATUS_REG));

	return 0;
}


static ssize_t rsc_reg_write(struct file *file, const char __user *buffer,
	size_t count, loff_t *data)
{
	char desc[128];
	int len = 0;
	/*char *pEnd;*/
	char addrSzBuf[24];
	char valSzBuf[24];
	char *pszTmp;
	int addr = 0, val = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;

	desc[len] = '\0';

	if (sscanf(desc, "%23s %23s", addrSzBuf, valSzBuf) == 2) {
		pszTmp = strstr(addrSzBuf, "0x");
		if (pszTmp == NULL) {
			/*if (1 != sscanf(addrSzBuf, "%d", &addr))*/
			if (kstrtoint(addrSzBuf, 0, &addr) != 0)
				LOG_ERR("scan decimal addr is wrong !!:%s",
					addrSzBuf);
		} else {
			if (strlen(addrSzBuf) > 2) {
				if (sscanf(addrSzBuf + 2, "%x", &addr) != 1)
					LOG_ERR("scan hex addr is wrong !!:%s",
						addrSzBuf);
			} else {
				LOG_INF("RSC Write Addr Error!!:%s", addrSzBuf);
			}
		}

		pszTmp = strstr(valSzBuf, "0x");
		if (pszTmp == NULL) {
			/*if (1 != sscanf(valSzBuf, "%d", &val))*/
			if (kstrtoint(valSzBuf, 0, &val) != 0)
				LOG_ERR("scan decimal value is wrong !!:%s",
					valSzBuf);
		} else {
			if (strlen(valSzBuf) > 2) {
				if (sscanf(valSzBuf + 2, "%x", &val) != 1)
					LOG_ERR("scan hex value is wrong !!:%s",
						valSzBuf);
			} else {
				LOG_INF("RSC Write Value Error!!:%s\n",
					valSzBuf);
			}
		}

		if ((addr >= RSC_BASE_HW) && (addr <= RSC_DMA_RDY_STATUS_HW)) {
			LOG_INF("Write Request - addr:0x%x, value:0x%x\n",
				addr, val);
			RSC_WR32((ISP_RSC_BASE + (addr - RSC_BASE_HW)), val);
		} else {
#define TMP_STR1 "Write-Address Range exceeds the size of hw rsc!!"
#define TMP_STR2 " addr:0x%x, value:0x%x\n"
			LOG_INF(TMP_STR1 TMP_STR2, addr, val);
#undef TMP_STR1
#undef TMP_STR2
		}

	} else if (sscanf(desc, "%23s", addrSzBuf) == 1) {
		pszTmp = strstr(addrSzBuf, "0x");
		if (pszTmp == NULL) {
			/*if (1 != sscanf(addrSzBuf, "%d", &addr))*/
			if (kstrtoint(addrSzBuf, 0, &addr) != 0)
				LOG_ERR("scan decimal addr is wrong !!:%s",
					addrSzBuf);
		} else {
			if (strlen(addrSzBuf) > 2) {
				if (sscanf(addrSzBuf + 2, "%x", &addr) != 1)
					LOG_ERR("scan hex addr is wrong !!:%s",
						addrSzBuf);
			} else {
				LOG_INF("RSC Read Addr Error!!:%s", addrSzBuf);
			}
		}

		if ((addr >= RSC_BASE_HW) && (addr <= RSC_DMA_RDY_STATUS_HW)) {
			val = RSC_RD32((ISP_RSC_BASE + (addr - RSC_BASE_HW)));
			LOG_INF("Read Request - addr:0x%x,value:0x%x\n",
				addr, val);
		} else {
#define TMP_STR1 "Read-Address Range exceeds the size of hw rsc!!"
#define TMP_STR2 " addr:0x%x, value:0x%x\n"
			LOG_INF(TMP_STR1 TMP_STR2, addr, val);
#undef TMP_STR1
#undef TMP_STR2
		}

	}


	return count;
}

static int proc_rsc_reg_open(struct inode *inode, struct file *file)
{
	return single_open(file, rsc_reg_read, NULL);
}

static const struct file_operations rsc_reg_proc_fops = {
	.owner = THIS_MODULE,
	.open = proc_rsc_reg_open,
	.read = seq_read,
	.write = rsc_reg_write,
};


/*******************************************************************************
 *
 ******************************************************************************/

int32_t RSC_ClockOnCallback(uint64_t engineFlag)
{
	/* LOG_DBG("RSC_ClockOnCallback"); */
	/* LOG_DBG("+CmdqEn:%d", g_u4EnableClockCount); */
	/* RSC_EnableClock(MTRUE); */

	return 0;
}

int32_t RSC_DumpCallback(uint64_t engineFlag, int level)
{
	LOG_DBG("%s", __func__);

	RSC_DumpReg();

	return 0;
}

int32_t RSC_ResetCallback(uint64_t engineFlag)
{
	LOG_DBG("%s", __func__);
	RSC_Reset();

	return 0;
}

int32_t RSC_ClockOffCallback(uint64_t engineFlag)
{
	/* LOG_DBG("RSC_ClockOffCallback"); */
	/* RSC_EnableClock(MFALSE); */
	/* LOG_DBG("-CmdqEn:%d", g_u4EnableClockCount); */
	return 0;
}


static signed int __init RSC_Init(void)
{
	signed int Ret = 0, j;
	void *tmp;
	/* FIX-ME: linux-3.10 procfs API changed */
	/* use proc_create */
	struct proc_dir_entry *proc_entry;
	struct proc_dir_entry *isp_rsc_dir;


	int i;
	/*  */
	LOG_DBG("- E.");
	/*  */
	Ret = platform_driver_register(&RSCDriver);
	if (Ret < 0) {
		LOG_ERR("platform_driver_register fail");
		return Ret;
	}

#if 0
	struct device_node *node = NULL;

	node = of_find_compatible_node(NULL, NULL, "mediatek,RSC");
	if (!node) {
		LOG_ERR("find mediatek,RSC node failed!!!\n");
		return -ENODEV;
	}
	ISP_RSC_BASE = of_iomap(node, 0);
	if (!ISP_RSC_BASE) {
		LOG_ERR("unable to map ISP_RSC_BASE registers!!!\n");
		return -ENODEV;
	}
	LOG_DBG("ISP_RSC_BASE: %lx\n", ISP_RSC_BASE);
#endif

	isp_rsc_dir = proc_mkdir("rsc", NULL);
	if (!isp_rsc_dir) {
		LOG_ERR("[%s]: fail to mkdir /proc/rsc\n", __func__);
		return 0;
	}

	proc_entry = proc_create("rsc_dump", 0444, isp_rsc_dir,
		&rsc_dump_proc_fops);

	proc_entry = proc_create("rsc_reg", 0644, isp_rsc_dir,
		&rsc_reg_proc_fops);


	/* isr log */
	if (PAGE_SIZE < ((RSC_IRQ_TYPE_AMOUNT * NORMAL_STR_LEN *
		((DBG_PAGE + INF_PAGE + ERR_PAGE) + 1)) * LOG_PPNUM)) {
		i = 0;
		while (i < ((RSC_IRQ_TYPE_AMOUNT * NORMAL_STR_LEN *
			((DBG_PAGE + INF_PAGE + ERR_PAGE) + 1)) * LOG_PPNUM)) {
			i += PAGE_SIZE;
		}
	} else {
		i = PAGE_SIZE;
	}
	pLog_kmalloc = kmalloc(i, GFP_KERNEL);
	if (pLog_kmalloc == NULL) {
		LOG_ERR("log mem not enough\n");
		return -ENOMEM;
	}
	memset(pLog_kmalloc, 0x00, i);
	tmp = pLog_kmalloc;
	for (i = 0; i < LOG_PPNUM; i++) {
		for (j = 0; j < RSC_IRQ_TYPE_AMOUNT; j++) {
			gSvLog[j]._str[i][_LOG_DBG] = (char *)tmp;
			tmp = (void *)((char *)tmp +
				(NORMAL_STR_LEN * DBG_PAGE));
			gSvLog[j]._str[i][_LOG_INF] = (char *)tmp;
			tmp = (void *)((char *)tmp +
				(NORMAL_STR_LEN * INF_PAGE));
			gSvLog[j]._str[i][_LOG_ERR] = (char *)tmp;
			tmp = (void *)((char *)tmp +
				(NORMAL_STR_LEN * ERR_PAGE));
		}

		/* log buffer ,in case of overflow */
		tmp = (void *)((char *)tmp + NORMAL_STR_LEN);
	}


#if 1
	/* Cmdq */
	/* Register RSC callback */
	LOG_DBG("register rsc callback for CMDQ");
	cmdqCoreRegisterCB(CMDQ_GROUP_RSC, RSC_ClockOnCallback,
		RSC_DumpCallback, RSC_ResetCallback, RSC_ClockOffCallback);
#endif

	LOG_DBG("- X. Ret: %d.", Ret);
	return Ret;
}

/*******************************************************************************
 *
 ******************************************************************************/
static void __exit RSC_Exit(void)
{
	/*int i;*/

	LOG_DBG("- E.");
	/*  */
	platform_driver_unregister(&RSCDriver);
	/*  */
#if 1
	/* Cmdq */
	/* Unregister RSC callback */
	cmdqCoreRegisterCB(CMDQ_GROUP_RSC, NULL, NULL, NULL, NULL);
#endif

	kfree(pLog_kmalloc);

	/*  */
}


/*******************************************************************************
 *
 ******************************************************************************/
void RSC_ScheduleWork(struct work_struct *data)
{
	if (RSC_DBG_DBGLOG & RSCInfo.DebugMask)
		LOG_DBG("- E.");

#ifdef RSC_USE_GCE
#else
	ConfigRSC();
#endif
}


static irqreturn_t ISP_Irq_RSC(signed int Irq, void *DeviceId)
{
	unsigned int RscStatus;
	bool bResulst = MFALSE;
	pid_t ProcessID;

	RscStatus = RSC_RD32(RSC_INT_STATUS_REG);	/* RSC Status */

	spin_lock(&(RSCInfo.SpinLockIrq[RSC_IRQ_TYPE_INT_RSC_ST]));

	if (RSC_INT_ST == (RSC_INT_ST & RscStatus)) {
		/* Update the frame status. */
#ifdef __RSC_KERNEL_PERFORMANCE_MEASURE__
		mt_kernel_trace_begin("rsc_irq");
#endif

#ifndef RSC_USE_GCE
		RSC_WR32(RSC_START_REG, 0);
#endif
		bResulst = UpdateRSC(&ProcessID);
		/* ConfigRSC(); */
		if (bResulst == MTRUE) {
			schedule_work(&RSCInfo.ScheduleRscWork);
#ifdef RSC_USE_GCE
			RSCInfo.IrqInfo.Status[RSC_IRQ_TYPE_INT_RSC_ST] |=
				RSC_INT_ST;
			RSCInfo.IrqInfo.ProcessID[RSC_PROCESS_ID_RSC] =
				ProcessID;
			RSCInfo.IrqInfo.RscIrqCnt++;
			RSCInfo.ProcessID[RSCInfo.WriteReqIdx] = ProcessID;
			RSCInfo.WriteReqIdx = (RSCInfo.WriteReqIdx + 1) %
			    _SUPPORT_MAX_RSC_FRAME_REQUEST_;
#ifdef RSC_MULTIPROCESS_TIMEING_ISSUE
			/* check the write value is equal to read value ? */
			/* actually, it doesn't happen!! */
			if (RSCInfo.WriteReqIdx == RSCInfo.ReadReqIdx) {
#define TMP_STR "%s Err!!, WriteReqIdx:0x%x, ReadReqIdx:0x%x\n"
				IRQ_LOG_KEEPER(RSC_IRQ_TYPE_INT_RSC_ST,
					m_CurrentPPB, _LOG_ERR,
					TMP_STR, __func__, RSCInfo.WriteReqIdx,
					RSCInfo.ReadReqIdx);
#undef TMP_STR
			}
#endif

#else
			RSCInfo.IrqInfo.Status[RSC_IRQ_TYPE_INT_RSC_ST] |=
				RSC_INT_ST;
			RSCInfo.IrqInfo.ProcessID[RSC_PROCESS_ID_RSC] =
				ProcessID;
#endif
		}
#ifdef __RSC_KERNEL_PERFORMANCE_MEASURE__
		mt_kernel_trace_end();
#endif
		/* Config the Next frame */
	}
	spin_unlock(&(RSCInfo.SpinLockIrq[RSC_IRQ_TYPE_INT_RSC_ST]));
	if (bResulst == MTRUE)
		wake_up_interruptible(&RSCInfo.WaitQueueHead);

	/* dump log, use tasklet */
#define TMP_STR1 "%s:%d, reg 0x%x : 0x%x, bResulst:%d, RscHWSta:0x%x,"
#define TMP_STR2 " RscIrqCnt:0x%x, WriteReqIdx:0x%x, ReadReqIdx:0x%x\n"

	IRQ_LOG_KEEPER(RSC_IRQ_TYPE_INT_RSC_ST, m_CurrentPPB, _LOG_INF,
		       TMP_STR1 TMP_STR2,
		       __func__, Irq, RSC_INT_STATUS_HW, RscStatus, bResulst,
		       RscStatus, RSCInfo.IrqInfo.RscIrqCnt,
		       RSCInfo.WriteReqIdx, RSCInfo.ReadReqIdx);
#undef TMP_STR1
#undef TMP_STR2

	if (RscStatus & RSC_INT_ST)
		tasklet_schedule(RSC_tasklet[RSC_IRQ_TYPE_INT_RSC_ST].pRSC_tkt);

	return IRQ_HANDLED;
}

static void ISP_TaskletFunc_RSC(unsigned long data)
{
	IRQ_LOG_PRINTER(RSC_IRQ_TYPE_INT_RSC_ST, m_CurrentPPB, _LOG_DBG);
	IRQ_LOG_PRINTER(RSC_IRQ_TYPE_INT_RSC_ST, m_CurrentPPB, _LOG_INF);
	IRQ_LOG_PRINTER(RSC_IRQ_TYPE_INT_RSC_ST, m_CurrentPPB, _LOG_ERR);

}


/*******************************************************************************
 *
 ******************************************************************************/
module_init(RSC_Init);
module_exit(RSC_Exit);
MODULE_DESCRIPTION("Camera RSC driver");
MODULE_AUTHOR("MM3SW2");
MODULE_LICENSE("GPL");
